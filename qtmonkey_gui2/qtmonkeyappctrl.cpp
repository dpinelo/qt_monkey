#include "qtmonkeyappctrl.h"
#include "common.hpp"
#include "json11.hpp"
#include "qtmonkey_app_api.hpp"

QtMonkeyAppCtrl::QtMonkeyAppCtrl(const QString &appPath,
                                 const QStringList &appArgs,
                                 QObject *parent)
    : QObject(parent)
{
    const QString appDirPath = QCoreApplication::applicationDirPath();
    const QString monkeyAppFileName = QFile::decodeName("qtmonkey_app");
    const QString monkeyAppPath = appDirPath + QDir::separator() + monkeyAppFileName;
    const QFileInfo monkeyAppFileInfo{monkeyAppPath};

    if (!(monkeyAppFileInfo.exists() && monkeyAppFileInfo.isFile()
          && monkeyAppFileInfo.isExecutable()))
        throw std::runtime_error(
            T_("Can not find %1").arg(monkeyAppPath).toStdString());

    connect(&qtmonkeyApp_, SIGNAL(error(QProcess::ProcessError)), this,
            SLOT(monkeyAppError(QProcess::ProcessError)));
    connect(&qtmonkeyApp_, SIGNAL(finished(int, QProcess::ExitStatus)), this,
            SLOT(monkeyAppFinished(int, QProcess::ExitStatus)));
    connect(&qtmonkeyApp_, SIGNAL(readyReadStandardOutput()), this,
            SLOT(monkeyAppNewOutput()));
    connect(&qtmonkeyApp_, SIGNAL(readyReadStandardError()), this,
            SLOT(monkeyAppNewErrOutput()));

    QStringList args;
    args << QStringLiteral("--user-app") << appPath << appArgs;
    qtmonkeyApp_.start(monkeyAppPath, args);
}

void QtMonkeyAppCtrl::monkeyAppError(QProcess::ProcessError err)
{
    qDebug("%s: err %d", Q_FUNC_INFO, static_cast<int>(err));
    emit monkeyAppFinishedSignal(qt_monkey_common::processErrorToString(err));
}

void QtMonkeyAppCtrl::monkeyAppFinished(int exitCode,
                                        QProcess::ExitStatus exitStatus)
{
    qDebug("%s: begin exitCode %d, exitStatus %d", Q_FUNC_INFO, exitCode,
           static_cast<int>(exitStatus));
    if (exitCode != EXIT_SUCCESS)
        emit monkeyAppFinishedSignal(T_("monkey app exit status not %1: %2")
                                         .arg(EXIT_SUCCESS)
                                         .arg(exitCode));
    else
        emit monkeyAppFinishedSignal(QString());
}

void QtMonkeyAppCtrl::monkeyAppNewOutput()
{
    qDebug("%s: begin", Q_FUNC_INFO);
    const QByteArray out = qtmonkeyApp_.readAllStandardOutput();
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
            qtmonkeyApp_.kill();
            emit monkeyAppFinishedSignal(
                T_("Internal Error: problem with monkey<->gui protocol: %1")
                    .arg(data));
        });

    if (parserStopPos != 0)
        jsonFromMonkey_.remove(0, parserStopPos);
}

void QtMonkeyAppCtrl::monkeyAppNewErrOutput()
{
    const QString errOut
        = QString::fromLocal8Bit(qtmonkeyApp_.readAllStandardError());
    qDebug("MONKEY: %s", qPrintable(errOut));
}

void QtMonkeyAppCtrl::runScript(const QString &script,
                                const QString &scriptFileName)
{
    const std::string data
        = qt_monkey_app::createPacketFromRunScript(script, scriptFileName)
          + '\n';
    quint64 sentBytes = 0;
    do {
        qint64 nbytes = qtmonkeyApp_.write(data.data() + sentBytes,
                                           data.size() - sentBytes);
        if (nbytes < 0) {
            emit criticalError(T_("Can not send data to application"));
            return;
        }
        sentBytes += nbytes;
    } while (sentBytes < data.size());
}

