#ifndef TESTEDITDLG_H
#define TESTEDITDLG_H

#include <QtWidgets>
#include "baseeditdlg.h"

namespace Ui {
class TestEditDlg;
}

class TestEditDlgPrivate;

class TestEditDlg : public BaseEditDlg
{
    Q_OBJECT

    friend class TestEditDlgPrivate;

public:
    explicit TestEditDlg(int idTestSuite, QWidget *parent = 0);
    ~TestEditDlg();

    enum class State {
        DoNothing,
        RecordEvents,
        PlayingEvents,
    };
    enum class MsgType {
        Default,
        Error,
        Protocol,
    };

    const QVariantMap editedData() Q_DECL_OVERRIDE;

public slots:
    void setEditedData(const QVariantMap &editedData);
    void accept() Q_DECL_OVERRIDE Q_DECL_FINAL;
    void reject() Q_DECL_OVERRIDE Q_DECL_FINAL;
    void on_pbRecord_clicked();
    void on_pbReplay_clicked();
    void setPath(const QString &path);
    void setArguments(const QString &arguments);
    void onMonkeyAppFinishedSignal(const QString &msg);
    void onMonkeyAppNewEvent(const QString &scriptLine);
    void onMonkeyUserAppError(const QString &errMsg);
    void onMonkeyScriptEnd();
    void onMonkeyScriptLog(const QString &msg);
    void onMonkeyScriptError(const QString &msg);
    void logNewLine(MsgType msgType, const QString &msg);
    void changeState(State val);
    void showError(const QString &msg);

private:
    Ui::TestEditDlg *ui;
    TestEditDlgPrivate *d;
};

#endif // TESTEDITDLG_H
