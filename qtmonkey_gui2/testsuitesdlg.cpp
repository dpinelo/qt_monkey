#include "testsuitesdlg.h"
#include "ui_testsuitesdlg.h"
#include <QtSql>
#include "testsuiteseditdlg.h"
#include "baseeditdlg.h"

class TestSuitesDlgPrivate
{
public:
    TestSuitesDlg *q_ptr;
    QPointer<QSqlTableModel> m_model;

    TestSuitesDlgPrivate(TestSuitesDlg *qq) : q_ptr(qq)
    {

    }

    bool initTableView();
};


TestSuitesDlg::TestSuitesDlg(QWidget *parent, Qt::WindowFlags fl) :
    QDialog(parent, fl),
    ui(new Ui::TestSuitesDlg),
    d(new TestSuitesDlgPrivate(this))
{
    ui->setupUi(this);

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
    const QVariantMap data = dlg->data();
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
    q_ptr->ui->tableView->resizeColumnsToContents();

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
    return true;
}
