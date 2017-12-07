#ifndef QTMONKEYAPPCTRL_H
#define QTMONKEYAPPCTRL_H

#include <QtCore>

class QtMonkeyAppCtrl : public QObject
{
    Q_OBJECT

public:
    explicit QtMonkeyAppCtrl(const QString &appPath,
                             const QStringList &appArgs,
                             QObject *parent = nullptr);

public slots:
    void runScript(const QString &script,
                   const QString &scriptFilename = QString());
signals:
    void monkeyAppFinishedSignal(QString msg);
    void monkeyAppNewEvent(const QString &scriptLine);
    void monkeyUserAppError(const QString &errMsg);
    void monkeyScriptEnd();
    void monkeScriptLog(const QString &);
    void criticalError(const QString &);

private slots:
    void monkeyAppError(QProcess::ProcessError);
    void monkeyAppFinished(int, QProcess::ExitStatus);
    void monkeyAppNewOutput();
    void monkeyAppNewErrOutput();

private:
    QProcess qtmonkeyApp_;
    QByteArray jsonFromMonkey_;
};

#endif // QTMONKEYAPPCTRL_H
