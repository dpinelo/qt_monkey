#pragma once

#include <queue>

#include <QtCore/QFile>
#include <QtCore/QObject>
#include <QtCore/QProcess>

#include "agent_qtmonkey_communication.hpp"
#include "script.hpp"
#include "shared_resource.hpp"

namespace qt_monkey_app
{

namespace Private
{
class StdinReader
#ifndef Q_MOC_RUN
    final
#endif
    : public QObject
{
    Q_OBJECT
signals:
    void error(const QString &msg);
    void dataReady();

public:
    qt_monkey_common::SharedResource<QByteArray> data;

    void emitError(const QString &msg) { emit error(msg); }
    void emitDataReady() { emit dataReady(); }
};
}
//! main class to control agent
class QtMonkey
#ifndef Q_MOC_RUN
    final
#endif
    : public QObject
{
    Q_OBJECT
public:
    explicit QtMonkey(bool exitOnScriptError, QObject *parent = nullptr);
    ~QtMonkey();
    void runApp(QString userAppPath, QStringList userAppArgs)
    {
        userAppPath_ = std::move(userAppPath);
        userAppArgs_ = std::move(userAppArgs);
        userApp_.start(userAppPath_, userAppArgs_);
    }
    bool runScriptFromFile(QString codeToRunBeforeAll,
                           QStringList scriptPathList,
                           const char *encoding = "UTF-8");

public slots:
    void command(const QByteArray &command);

private slots:
    void userAppError(QProcess::ProcessError);
    void userAppFinished(int, QProcess::ExitStatus);
    void userAppNewOutput();
    void userAppNewErrOutput();
    void communicationWithAgentError(const QString &errStr);
    void onNewUserAppEvent(QString scriptLines);
    void onScriptError(QString errMsg);
    void onAgentReadyToRunScript();
    void onScriptEnd();
    void onScriptLog(QString msg);

signals:
    void newUserAppError(QProcess::ProcessError error);
    void newUserAppFinished(int, QProcess::ExitStatus);
    void newUserAppOutput(const QString &message);
    void newUserAppErrorOutput(const QString &message);
    void newScriptError(const QString &errMsg);

private:
    bool scriptRunning_ = false;

    qt_monkey_agent::Private::CommunicationMonkeyPart channelWithAgent_;
    QProcess userApp_;
    std::queue<qt_monkey_agent::Private::Script> toRunList_;
    bool exitOnScriptError_ = false;
    QString userAppPath_;
    QStringList userAppArgs_;
    bool restartDone_ = false;

    void setScriptRunningState(bool val);
};
}
