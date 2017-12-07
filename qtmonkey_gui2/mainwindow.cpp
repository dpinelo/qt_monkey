#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "testsuitesdlg.h"
#include "database.h"

class MainWindowPrivate
{
public:
    MainWindow *q_ptr;

    MainWindowPrivate(MainWindow *qq) : q_ptr(qq)
    {

    }

    bool initDatabase();
};

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    d(new MainWindowPrivate(this))
{
    ui->setupUi(this);

    if ( !d->initDatabase() )
    {
        close();
    }
}

MainWindow::~MainWindow()
{
    delete d;
    delete ui;
}

void MainWindow::on_pbTestSuites_clicked()
{
    QScopedPointer<TestSuitesDlg> dlg (new TestSuitesDlg(this));
    dlg->setModal(true);
    dlg->exec();
}

void MainWindow::on_pbPreferences_clicked()
{

}

void MainWindow::on_pbQuit_clicked()
{
    close();
}

bool MainWindowPrivate::initDatabase()
{
    const QString dbPath = Database::dbPath();
    QFileInfo fi(dbPath);
    bool databaseExist = fi.exists();
    if ( !Database::openSQLite() )
    {
        QMessageBox::warning(q_ptr,
                             qApp->applicationName(),
                             QObject::tr("Can not open database.\Error: %1").arg(Database::m_lastErrorMessage),
                             QMessageBox::Ok);
        return false;
    }
    if ( !databaseExist && !Database::initDatabase() )
    {
        QMessageBox::warning(q_ptr,
                             qApp->applicationName(),
                             QObject::tr("Can not init database.\Error: %1").arg(Database::m_lastErrorMessage),
                             QMessageBox::Ok);
        return false;
    }
    return true;
}
