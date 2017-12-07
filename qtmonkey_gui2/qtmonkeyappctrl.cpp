#include "qtmonkeyappctrl.h"
#include "common.hpp"
#include "json11.hpp"
#include "qtmonkey_app_api.hpp"
#include "qtmonkey.hpp"

QtMonkeyAppCtrl::QtMonkeyAppCtrl(QObject *parent)
    : QObject(parent)
{
}

void QtMonkeyAppCtrl::recordTest(const QString &appPath, const QStringList &appArgs)
{
    m_monkey = new qt_monkey_app::QtMonkey(/*exitOnScriptError*/ true, this);

    connect(m_monkey, SIGNAL(newUserAppError(QProcess::ProcessError)), this,
            SLOT(monkeyAppError(QProcess::ProcessError)));
    connect(m_monkey, SIGNAL(newUserAppFinished(int, QProcess::ExitStatus)), this,
            SLOT(monkeyAppFinished(int, QProcess::ExitStatus)));
    connect(m_monkey, SIGNAL(newUserAppOutput(QString)), this,
            SLOT(monkeyAppNewOutput(QString)));
    connect(m_monkey, SIGNAL(newUserAppErrorOutput(QString)), this,
            SLOT(monkeyAppNewErrOutput(QString)));
    connect(m_monkey, SIGNAL(newScriptError(QString)), this,
            SLOT(monkeyAppNewOutput(QString)));

    m_monkey->runApp(appPath, appArgs);
}

void QtMonkeyAppCtrl::monkeyAppError(QProcess::ProcessError err)
{
    qDebug("%s: err %d", Q_FUNC_INFO, static_cast<int>(err));
    emit monkeyAppFinishedSignal(qt_monkey_common::processErrorToString(err));
    m_monkey->deleteLater();
}

void QtMonkeyAppCtrl::monkeyAppFinished(int exitCode,
                                        QProcess::ExitStatus exitStatus)
{
    qDebug("%s: begin exitCode %d, exitStatus %d", Q_FUNC_INFO, exitCode,
           static_cast<int>(exitStatus));
    if (exitCode != EXIT_SUCCESS)
    {
        emit monkeyAppFinishedSignal(T_("monkey app exit status not %1: %2")
                                         .arg(EXIT_SUCCESS)
                                         .arg(exitCode));
    }
    else
    {
        emit monkeyAppFinishedSignal(QString());
    }
    m_monkey->deleteLater();
}

void QtMonkeyAppCtrl::monkeyAppNewOutput(const QString &msg)
{
    qDebug("%s: begin", Q_FUNC_INFO);
    const QByteArray out = msg.toLocal8Bit();
    jsonFromMonkey_.append(out);
    qDebug("%s: json |%s|", Q_FUNC_INFO, qPrintable(jsonFromMonkey_));

    std::string::size_type parserStopPos;
    qt_monkey_app::parseOutputFromMonkeyApp(
        {jsonFromMonkey_.constData(),
         static_cast<size_t>(jsonFromMonkey_.size())},
        parserStopPos,
        [this](QString eventScriptLines) {
            emit monkeyAppNewEvent(std::move(eventScriptLines));
        },
        [this](QString userAppErrors) {
            emit monkeyUserAppError(std::move(userAppErrors));
        },
        [this]() { // on script end
            emit monkeyScriptEnd();
        },
        [this](QString scriptLog) { emit monkeScriptLog(scriptLog); },
        [this](QString data) {
            emit monkeyAppFinishedSignal(
                T_("Internal Error: problem with monkey<->gui protocol: %1")
                    .arg(data));
        });

    if (parserStopPos != 0)
        jsonFromMonkey_.remove(0, parserStopPos);
}

void QtMonkeyAppCtrl::monkeyAppNewErrOutput(const QString &msg)
{
    const QString errOut
        = QString::fromLocal8Bit(msg.toLocal8Bit());
    qDebug("MONKEY: %s", qPrintable(errOut));
}

void QtMonkeyAppCtrl::runScript(const QString &appPath,
                                const QStringList &appArgs,
                                const QString &script,
                                const QString &scriptFileName)
{
    recordTest(appPath, appArgs);

    const std::string data
        = qt_monkey_app::createPacketFromRunScript(script, scriptFileName)
          + '\n';
    quint64 sentBytes = 0;
    do {
        QBuffer buffer;
        buffer.open(QIODevice::WriteOnly);
        qint64 nbytes = buffer.write(data.data() + sentBytes,
                                     data.size() - sentBytes);
        if (nbytes < 0) {
            emit criticalError(T_("Can not send data to application"));
            return;
        }
        m_monkey->command(buffer.buffer());
        sentBytes += nbytes;
    } while (sentBytes < data.size());
}

