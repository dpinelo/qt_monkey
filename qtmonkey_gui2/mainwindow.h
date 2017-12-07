#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

namespace Ui {
class MainWindow;
}

class MainWindowPrivate;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

public slots:
    void on_pbTestSuites_clicked();
    void on_pbPreferences_clicked();
    void on_pbQuit_clicked();

private:
    Ui::MainWindow *ui;
    MainWindowPrivate *d;
};

#endif // MAINWINDOW_H
