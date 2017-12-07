#include "testsuitesdlg.h"
#include "ui_testsuitesdlg.h"
#include <QtSql>
#include "testsuiteseditdlg.h"
#include "baseeditdlg.h"
#include "database.h"

class TestSuitesDlgPrivate
{
public:
    TestSuitesDlg *q_ptr;
    QPointer<QSqlTableModel> m_model;

    TestSuitesDlgPrivate(TestSuitesDlg *qq) : q_ptr(qq)
    {

    }

    bool initDatabase();
    bool initTableView();
};

bool TestSuitesDlgPrivate::initDatabase()
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

TestSuitesDlg::TestSuitesDlg(QWidget *parent, Qt::WindowFlags fl) :
    QMainWindow(parent, fl),
    ui(new Ui::TestSuitesDlg),
    d(new TestSuitesDlgPrivate(this))
{
    ui->setupUi(this);

    if ( !d->initDatabase() )
    {
        close();
    }

    if ( !d->initTableView() )
    {
        close();
    }
}

TestSuitesDlg::~TestSuitesDlg()
{
    delete d;
    delete ui;
}

void TestSuitesDlg::on_pbAdd_clicked()
{
    QScopedPointer<TestSuitesEditDlg> dlg (new TestSuitesEditDlg(this));
    if ( dlg->exec() == QDialog::Rejected )
    {
        return;
    }
    const QVariantMap data = dlg->editedData();
    const int row = d->m_model->rowCount();
    BaseEditDlg::addRecord(d->m_model, data, row);
}

void TestSuitesDlg::on_pbEdit_clicked()
{
    const QModelIndexList selectedRows = ui->tableView->selectionModel()->selectedRows();
    if ( selectedRows.isEmpty() )
    {
        return;
    }
    QScopedPointer<TestSuitesEditDlg> dlg (new TestSuitesEditDlg(this));
    const int row = selectedRows.first().row();
    BaseEditDlg::editRecord(d->m_model, row, dlg.data());
}

void TestSuitesDlg::on_pbRemove_clicked()
{
    const QModelIndexList selectedRows = ui->tableView->selectionModel()->selectedRows();
    if ( selectedRows.isEmpty() )
    {
        return;
    }
    int ret = QMessageBox::question(this,
                                    qApp->applicationName(),
                                    tr("Are you sure you want to delete selected record?"),
                                    QMessageBox::Yes | QMessageBox::No);
    if ( ret == QMessageBox::No )
    {
        return;
    }
    int row = selectedRows.first().row();
    if ( !d->m_model->removeRow(row) || !d->m_model->submitAll() )
    {
        d->m_model->revertAll();
        QMessageBox::warning(this,
                             qApp->applicationName(),
                             tr("An error ocurred trying to delete test suite.\nError: %1").arg(d->m_model->lastError().text()));
        return;
    }
}

void TestSuitesDlg::on_pbRun_clicked()
{

}

bool TestSuitesDlgPrivate::initTableView()
{
    QSqlDatabase db = QSqlDatabase::database();
    m_model = new QSqlTableModel(q_ptr, db);
    m_model->setTable("testsuites");
    m_model->setSort(m_model->fieldIndex("name"), Qt::AscendingOrder);
    m_model->setEditStrategy(QSqlTableModel::OnManualSubmit);
    q_ptr->ui->tableView->setModel(m_model);

    QHash<QString, QString> columnNames;
    columnNames[QStringLiteral("name")] = "Name";
    columnNames[QStringLiteral("executablepath")] = "Target path";
    columnNames[QStringLiteral("arguments")] = "Arguments";
    q_ptr->ui->tableView->hideColumn(m_model->fieldIndex("id"));
    QHashIterator<QString, QString> it(columnNames);
    while (it.hasNext())
    {
        it.next();
        m_model->setHeaderData(m_model->fieldIndex(it.key()), Qt::Horizontal, it.value());
    }
    if ( !m_model->select() )
    {
        QMessageBox::warning(q_ptr,
                             qApp->applicationName(),
                             QObject::tr("An error ocurred trying to access test suites.\nError: %1").arg(m_model->lastError().text()));
        return false;
    }
    q_ptr->ui->tableView->resizeColumnsToContents();
    return true;
}
