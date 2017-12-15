#ifndef QTMONKEYAPPCTRL_H
#define QTMONKEYAPPCTRL_H

#include <QtCore>
#include "qtmonkey.hpp"

class QtMonkeyAppCtrl : public QObject
{
    Q_OBJECT

public:
    explicit QtMonkeyAppCtrl(QObject *parent = nullptr);
    virtual ~QtMonkeyAppCtrl();

public slots:
    void recordTest(const QString &appPath,
                    const QStringList &appArgs);
    void runScript(const QString &appPath,
                   const QStringList &appArgs,
                   const QString &script,
                   const QString &scriptFilename = QString());
    void killApp();

signals:
    void monkeyAppFinishedSignal(QString msg);
    void monkeyAppNewEvent(QString scriptLine);
    void monkeyUserAppError(QString errMsg);
    void monkeyScriptEnd();
    void monkeyScriptLog(QString);
    void criticalError(QString);
    void newScriptError(QString);

private slots:
    void monkeyAppError(QProcess::ProcessError);
    void monkeyAppFinished(int, QProcess::ExitStatus);
    void monkeyAppNewOutput(const QString &msg);
    void monkeyAppNewErrOutput(const QString &msg);

private:
    QPointer<qt_monkey_app::QtMonkey> m_monkey;
    QByteArray jsonFromMonkey_;
};

#endif // QTMONKEYAPPCTRL_H
