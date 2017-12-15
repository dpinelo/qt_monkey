#ifndef TESTSUITESEDITDLG_H
#define TESTSUITESEDITDLG_H

#include <QtWidgets>
#include "baseeditdlg.h"

namespace Ui {
class TestSuitesEditDlg;
}

class TestSuitesEditDlgPrivate;

class TestSuitesEditDlg : public BaseEditDlg
{
    Q_OBJECT

    friend class TestSuitesEditDlgPrivate;

public:
    explicit TestSuitesEditDlg(QWidget *parent = 0, Qt::WindowFlags fl = Qt::WindowFlags());
    ~TestSuitesEditDlg();

    enum class State {
        DoNothing,
        PlayingEvents,
    };

public slots:
    void setEditedData(const QVariantMap &data) Q_DECL_OVERRIDE;
    void accept() Q_DECL_OVERRIDE;
    void on_pbSearch_clicked();
    void on_pbAdd_clicked();
    void on_pbEdit_clicked();
    void on_pbRemove_clicked();
    void on_pbPlayTest_clicked();
    void on_pbPlaySuite_clicked();
    void onMonkeyAppFinishedSignal(const QString &msg);
    void onMonkeyError(const QString &errMsg);
    void onMonkeyScriptEnd();
    void setState(State state);

signals:
    void testPlayed();

private:
    Ui::TestSuitesEditDlg *ui;
    TestSuitesEditDlgPrivate *d;
};

#endif // TESTSUITESTESTDLG_H
