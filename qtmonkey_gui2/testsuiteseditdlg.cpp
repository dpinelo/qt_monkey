#include "testsuiteseditdlg.h"
#include "ui_testsuiteseditdlg.h"
#include <QtWidgets>
#include <QtSql>
#include "testeditdlg.h"
#include "baseeditdlg.h"

class TestSuitesEditDlgPrivate
{
public:
    TestSuitesEditDlg *q_ptr;
    QPointer<QSqlTableModel> m_model;

    TestSuitesEditDlgPrivate(TestSuitesEditDlg *qq) : q_ptr(qq)
    {
    }

    bool initTestModel();
};


TestSuitesEditDlg::TestSuitesEditDlg(QWidget *parent, Qt::WindowFlags fl) :
    BaseEditDlg(parent, fl),
    ui(new Ui::TestSuitesEditDlg),
    d(new TestSuitesEditDlgPrivate(this))
{
    ui->setupUi(this);

    ui->pbAdd->setEnabled(false);
    ui->pbEdit->setEnabled(false);
    ui->pbRemove->setEnabled(false);
}

TestSuitesEditDlg::~TestSuitesEditDlg()
{
    delete d;
    delete ui;
}

void TestSuitesEditDlg::setData(const QVariantMap &data)
{
    BaseEditDlg::setData(data);
    d->initTestModel();
    ui->pbAdd->setEnabled(true);
    ui->pbEdit->setEnabled(true);
    ui->pbRemove->setEnabled(true);
}

void TestSuitesEditDlg::accept()
{
    if ( ui->leExecutablePath->text().trimmed().isEmpty() )
    {
        QMessageBox::warning(this,
                             qApp->applicationName(),
                             tr("You must set an executable path"));
        ui->leExecutablePath->setFocus();
        return;
    }
    if ( ui->leName->text().trimmed().isEmpty() )
    {
        QMessageBox::warning(this,
                             qApp->applicationName(),
                             tr("You must set a name for test suite"));
        ui->leExecutablePath->setFocus();
        return;
    }
    BaseEditDlg::accept();
}

void TestSuitesEditDlg::on_pbSearch_clicked()
{
    const QString executablePath = QFileDialog::getOpenFileName(this,
                                                                tr("Search for target executable"));
    if ( executablePath.isEmpty() )
    {
        return;
    }
    ui->leExecutablePath->setText(executablePath);
}

void TestSuitesEditDlg::on_pbAdd_clicked()
{
    if ( ui->leExecutablePath->text().trimmed().isEmpty() )
    {
        QMessageBox::warning(this,
                             qApp->applicationName(),
                             tr("You must set executable path first."));
        return;
    }
    QScopedPointer<TestEditDlg> dlg (new TestEditDlg(this));
    dlg->setPath(ui->leExecutablePath->text());
    dlg->setArguments(ui->leArguments->text());
    if ( dlg->exec() == QDialog::Rejected )
    {
        return;
    }
    const QVariantMap data = dlg->data();
    const int row = d->m_model->rowCount();
    BaseEditDlg::addRecord(d->m_model, data, row);
}

void TestSuitesEditDlg::on_pbEdit_clicked()
{
    if ( ui->leExecutablePath->text().trimmed().isEmpty() )
    {
        QMessageBox::warning(this,
                             qApp->applicationName(),
                             tr("You must set executable path first."));
        return;
    }
    const QModelIndexList selectedRows = ui->tableView->selectionModel()->selectedRows();
    if ( selectedRows.isEmpty() )
    {
        return;
    }
    QScopedPointer<TestEditDlg> dlg (new TestEditDlg(this));
    dlg->setPath(ui->leExecutablePath->text());
    dlg->setArguments(ui->leArguments->text());
    const int row = selectedRows.first().row();
    BaseEditDlg::editRecord(d->m_model, row, dlg.data());
}

void TestSuitesEditDlg::on_obRemove_clicked()
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
                             tr("An error ocurred trying to delete test.\nError: %1").arg(d->m_model->lastError().text()));
        return;
    }
}

bool TestSuitesEditDlgPrivate::initTestModel()
{
    QSqlDatabase db = QSqlDatabase::database();
    m_model = new QSqlTableModel(q_ptr, db);
    m_model->setTable("testsuitestest");
    const QString filter = QStringLiteral("idtestsuite=%1").arg(q_ptr->data().value(QStringLiteral("id")).toString());
    m_model->setFilter(filter);
    m_model->setSort(m_model->fieldIndex("position"), Qt::AscendingOrder);
    m_model->setEditStrategy(QSqlTableModel::OnManualSubmit);
    q_ptr->ui->tableView->setModel(m_model);
    q_ptr->ui->tableView->resizeColumnsToContents();

    QHash<QString, QString> columnNames;
    columnNames[QStringLiteral("name")] = "Name";
    columnNames[QStringLiteral("position")] = "Execution Order";
    columnNames[QStringLiteral("code")] = "Code";
    q_ptr->ui->tableView->hideColumn(m_model->fieldIndex("id"));
    q_ptr->ui->tableView->hideColumn(m_model->fieldIndex("idtestsuite"));
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
                             QObject::tr("An error ocurred trying to access test.\nError: %1").arg(m_model->lastError().text()));
        return false;
    }
    return true;
}
