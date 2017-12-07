#ifndef TESTSUITESDLG_H
#define TESTSUITESDLG_H

#include <QtWidgets>

namespace Ui {
class TestSuitesDlg;
}

class TestSuitesDlgPrivate;

class TestSuitesDlg : public QMainWindow
{
    Q_OBJECT

    friend class TestSuitesDlgPrivate;

public:
    explicit TestSuitesDlg(QWidget *parent = 0,  Qt::WindowFlags f = Qt::WindowFlags());
    ~TestSuitesDlg();

public slots:
    void on_pbAdd_clicked();
    void on_pbEdit_clicked();
    void on_pbRemove_clicked();
    void on_pbRun_clicked();

private:
    Ui::TestSuitesDlg *ui;
    TestSuitesDlgPrivate *d;
};

#endif // TESTSUITESDLG_H
