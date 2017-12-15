//#define DEBUG_MOD_QTMONKEY
#include "qtmonkey.hpp"

#include <atomic>
#include <cassert>
#include <iostream>

#ifdef _WIN32 // windows both 32 bit and 64 bit
#include <windows.h>
#else
#include <cerrno>
#include <unistd.h>
#endif

#include <QtCore/QCoreApplication>
#include <QtCore/QTextCodec>
#include <QtCore/QThread>

#include "common.hpp"
#include "json11.hpp"
#include "qtmonkey_app_api.hpp"

#define DBGPRINT(fmt, ...) qDebug(fmt, __VA_ARGS__)

using qt_monkey_agent::Private::PacketTypeForAgent;
using qt_monkey_agent::Private::Script;
using qt_monkey_app::Private::StdinReader;
using qt_monkey_app::QtMonkey;
using qt_monkey_common::operator<<;

namespace
{
static constexpr int waitBeforeExitMs = 300;

static inline std::ostream &operator<<(std::ostream &os, const QString &str)
{
    os << str.toUtf8();
    return os;
}
}

QtMonkey::QtMonkey(bool exitOnScriptError, QObject *parent)
    : QObject(parent),
      exitOnScriptError_(exitOnScriptError)
{
    QProcessEnvironment curEnv = QProcessEnvironment::systemEnvironment();
    curEnv.insert(channelWithAgent_.requiredProcessEnvironment().first,
                  channelWithAgent_.requiredProcessEnvironment().second);
    userApp_.setProcessEnvironment(curEnv);

    connect(&userApp_, SIGNAL(error(QProcess::ProcessError)), this,
            SLOT(userAppError(QProcess::ProcessError)));
    connect(&userApp_, SIGNAL(finished(int, QProcess::ExitStatus)), this,
            SLOT(userAppFinished(int, QProcess::ExitStatus)));
    connect(&userApp_, SIGNAL(readyReadStandardOutput()), this,
            SLOT(userAppNewOutput()));
    connect(&userApp_, SIGNAL(readyReadStandardError()), this,
            SLOT(userAppNewErrOutput()));

    connect(&channelWithAgent_, SIGNAL(error(QString)), this,
            SLOT(communicationWithAgentError(const QString &)));
    connect(&channelWithAgent_, SIGNAL(newUserAppEvent(QString)), this,
            SLOT(onNewUserAppEvent(QString)));
    connect(&channelWithAgent_, SIGNAL(scriptError(QString)), this,
            SLOT(onScriptError(QString)));
    connect(&channelWithAgent_, SIGNAL(agentReadyToRunScript()), this,
            SLOT(onAgentReadyToRunScript()));
    connect(&channelWithAgent_, SIGNAL(scriptEnd()), this, SLOT(onScriptEnd()));
    connect(&channelWithAgent_, SIGNAL(scriptLog(QString)), this,
            SLOT(onScriptLog(QString)));
}

QtMonkey::~QtMonkey()
{
    killApp();
}

void QtMonkey::communicationWithAgentError(const QString &errStr)
{
    qWarning("%s: errStr %s", Q_FUNC_INFO, qPrintable(errStr));
}

void QtMonkey::onNewUserAppEvent(QString scriptLines)
{
    std::string output = qt_monkey_app::createPacketFromUserAppEvent(scriptLines);
    std::cout << output
              << std::endl;
    emit newUserAppOutput(QString::fromStdString(output));
}

void QtMonkey::userAppError(QProcess::ProcessError err)
{
    qDebug("%s: begin err %d", Q_FUNC_INFO, static_cast<int>(err));
    emit newUserAppError(err);
}

void QtMonkey::userAppFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    qDebug("%s: begin exitCode %d, exitStatus %d", Q_FUNC_INFO, exitCode,
           static_cast<int>(exitStatus));
    qt_monkey_common::processEventsFor(waitBeforeExitMs);
    setScriptRunningState(false);
    emit newUserAppFinished(exitCode, exitStatus);
}

void QtMonkey::userAppNewOutput()
{
    const QString stdoutStr
        = QString::fromLocal8Bit(userApp_.readAllStandardOutput());
    std::string output = createPacketFromUserAppOutput(stdoutStr);
    std::cout << output << std::endl;
    emit newUserAppOutput(QString::fromStdString(output));
}

void QtMonkey::userAppNewErrOutput()
{
    const QString errOut
        = QString::fromLocal8Bit(userApp_.readAllStandardError());
    std::string output = createPacketFromUserAppErrors(errOut);
    std::cout << output << std::endl;
    emit newUserAppErrorOutput(QString::fromStdString(output));
}

void QtMonkey::command(const QByteArray &command)
{
    auto dataPtr = command;
    size_t parserStopPos;
    parseOutputFromGui(
        {dataPtr.constData(), static_cast<size_t>(dataPtr.size())},
        parserStopPos,
        [this](QString script, QString scriptFileName) {
            auto scripts
                = Script::splitToExecutableParts(scriptFileName, script);
            for (auto &&script : scripts)
                toRunList_.push(std::move(script));
            onAgentReadyToRunScript();
        },
        [this](QString errMsg) {
            std::cerr
                << T_("Can not parse gui<->monkey protocol: %1\n").arg(errMsg);
    });
}

void QtMonkey::killApp()
{
    if (userApp_.state() != QProcess::NotRunning) {
        userApp_.terminate();
        QCoreApplication::processEvents(QEventLoop::AllEvents, 10000 /*ms*/);
        userApp_.kill();
        QCoreApplication::processEvents(QEventLoop::AllEvents, 5000 /*ms*/);
    }
    // so any signals from channel will be disconected
    channelWithAgent_.close();
}

void QtMonkey::onScriptError(QString errMsg)
{
    qDebug("%s: begin %s", Q_FUNC_INFO, qPrintable(errMsg));
    setScriptRunningState(false);
    std::string output = createPacketFromUserAppErrors(errMsg);
    std::cout << output << std::endl;
    emit newScriptError(QString::fromStdString(output));
}

bool QtMonkey::runScriptFromFile(QString codeToRunBeforeAll,
                                 QStringList scriptPathList,
                                 const char *encoding)
{
    if (encoding == nullptr)
        encoding = "UTF-8";

    for (const QString &fn : scriptPathList) {
        QFile f(fn);
        if (!f.open(QIODevice::ReadOnly)) {
            std::cerr << T_("Error: can not open %1\n").arg(fn);
            return false;
        }
        QTextStream t(&f);
        t.setCodec(QTextCodec::codecForName(encoding));
        const QString script = t.readAll();
        auto scripts = Script::splitToExecutableParts(fn, script);
        for (auto &&script : scripts) {
            if (!codeToRunBeforeAll.isEmpty()) {
                DBGPRINT("%s: we add code to run: '%s' to '%s'", Q_FUNC_INFO,
                         qPrintable(codeToRunBeforeAll), qPrintable(fn));
                Script prefs_script{QStringLiteral("<tmp>"), 1,
                                    codeToRunBeforeAll};
                prefs_script.setRunAfterAppStart(!toRunList_.empty());
                toRunList_.push(std::move(prefs_script));
            } else {
                script.setRunAfterAppStart(!toRunList_.empty());
            }
            toRunList_.push(std::move(script));
        }
    }

    return true;
}

void QtMonkey::onAgentReadyToRunScript()
{
    DBGPRINT("%s: begin is connected %s, run list empty %s, script running %s",
             Q_FUNC_INFO,
             channelWithAgent_.isConnectedState() ? "true" : "false",
             toRunList_.empty() ? "true" : "false",
             scriptRunning_ ? "true" : "false");
    if (!channelWithAgent_.isConnectedState() || toRunList_.empty()
        || scriptRunning_)
        return;

    if (toRunList_.front().runAfterAppStart()) {
        if (restartDone_) {
            restartDone_ = false;
        } else {
            DBGPRINT("%s: restartDone false, exiting", Q_FUNC_INFO);
            return;
        }
    }

    Script script = std::move(toRunList_.front());
    toRunList_.pop();
    QString code;
    script.releaseCode(code);
    channelWithAgent_.sendCommand(PacketTypeForAgent::SetScriptFileName,
                                  script.fileName());
    channelWithAgent_.sendCommand(PacketTypeForAgent::RunScript,
                                  std::move(code));
    setScriptRunningState(true);
}

void QtMonkey::onScriptEnd()
{
    setScriptRunningState(false);
    std::string output = createPacketFromScriptEnd();
    std::cout << output << std::endl;
    emit newUserAppOutput(QString::fromStdString(output));
}

void QtMonkey::onScriptLog(QString msg)
{
    std::string output = createPacketFromUserAppScriptLog(msg);
    std::cout << output << std::endl;
    emit newUserAppOutput(QString::fromStdString(output));
}

void QtMonkey::setScriptRunningState(bool val)
{
    scriptRunning_ = val;
    if (!scriptRunning_)
        onAgentReadyToRunScript();
}
