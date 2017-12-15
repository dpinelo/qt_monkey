#include "testsuiteseditdlg.h"
#include "ui_testsuiteseditdlg.h"
#include <QtWidgets>
#include <QtSql>
#include "testeditdlg.h"
#include "baseeditdlg.h"
#include "qtmonkeyappctrl.h"
#include "common.h"

class TestSuitesEditDlgPrivate
{
public:
    TestSuitesEditDlg *q_ptr;
    QPointer<QSqlTableModel> m_model;
    TestSuitesEditDlg::State m_state { TestSuitesEditDlg::State::DoNothing };
    QPointer<QtMonkeyAppCtrl> m_monkeyCtrl;
    QPointer<QStandardItemModel> m_reportModel;
    QString m_actualRunningTestName;
    QElapsedTimer m_elapsedTimer;
    bool m_testHasError { false };
    bool m_testScriptEnd { false };
    int m_actualTestRow { -1 };

    TestSuitesEditDlgPrivate(TestSuitesEditDlg *qq) : q_ptr(qq)
    {
        m_reportModel = new QStandardItemModel(q_ptr);
    }

    bool initTestModel();
    void initReportModel();
    void playTest(const QString &testName, const QString &qsCode);
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
    ui->tableViewReport->setVisible(false);
    ui->pbPlaySuite->setEnabled(false);
    ui->pbPlayTest->setEnabled(false);
    d->initReportModel();
}

TestSuitesEditDlg::~TestSuitesEditDlg()
{
    delete d;
    delete ui;
}

void TestSuitesEditDlg::setEditedData(const QVariantMap &data)
{
    BaseEditDlg::setEditedData(data);
    d->initTestModel();
    ui->pbAdd->setEnabled(true);
    ui->pbEdit->setEnabled(true);
    ui->pbRemove->setEnabled(true);
    ui->pbPlaySuite->setEnabled(true);
    ui->pbPlayTest->setEnabled(true);
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
    QScopedPointer<TestEditDlg> dlg (new TestEditDlg(m_data.value(QStringLiteral("id")).toInt(), this));
    dlg->setPath(ui->leExecutablePath->text());
    dlg->setArguments(ui->leArguments->text());
    dlg->setModal(true);
    QVariantMap data;
    data["idtestsuite"] = m_data.value(QStringLiteral("id")).toInt();
    dlg->setEditedData(data);
    if ( dlg->exec() == QDialog::Rejected )
    {
        return;
    }
    data = dlg->editedData();
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
    QScopedPointer<TestEditDlg> dlg (new TestEditDlg(m_data.value(QStringLiteral("id")).toInt(), this));
    dlg->setPath(ui->leExecutablePath->text());
    dlg->setArguments(ui->leArguments->text());
    const int row = selectedRows.first().row();
    BaseEditDlg::editRecord(d->m_model, row, dlg.data());
}

void TestSuitesEditDlg::on_pbRemove_clicked()
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
                             tr("An error ocurred trying to delete test.\nError: %1").arg(d->m_model->lastError().text()));
        return;
    }
}

void TestSuitesEditDlg::on_pbPlayTest_clicked()
{
    const QModelIndexList selectedRows = ui->tableView->selectionModel()->selectedRows();
    if ( selectedRows.isEmpty() )
    {
        QMessageBox::warning(this,
                             qApp->applicationName(),
                             tr("You must select a test to execute."));
        return;
    }
    d->m_actualTestRow = selectedRows.first().row();
    d->m_reportModel->clear();
    d->initReportModel();
    ui->tableViewReport->setVisible(true);
    const QSqlRecord record = d->m_model->record(selectedRows.first().row());
    d->playTest(record.value(QStringLiteral("name")).toString(), record.value(QStringLiteral("code")).toString());
}

void TestSuitesEditDlg::on_pbPlaySuite_clicked()
{
    d->m_reportModel->clear();
    d->initReportModel();
    ui->tableViewReport->setVisible(true);

    QSqlQuery qry(QSqlDatabase::database());
    const QString sql = QStringLiteral("SELECT name, code FROM testsuitestest WHERE idtestsuite=:idtestsuite ORDER BY position");
    if ( !qry.prepare(sql) )
    {
        QMessageBox::warning(this,
                             qApp->applicationName(),
                             tr("An error ocurred trying to access database.\nError: %1").arg(qry.lastError().text()));
        return;
    }
    qry.bindValue(":idtestsuite", m_data.value(QStringLiteral("id")).toInt());
    if ( !qry.exec() )
    {
        QMessageBox::warning(this,
                             qApp->applicationName(),
                             tr("An error ocurred trying to execute query.\nError: %1").arg(qry.lastError().text()));
        return;
    }
    QEventLoop eventLoop;
    connect(this, SIGNAL(testPlayed()), &eventLoop, SLOT(quit()));
    d->m_actualTestRow = 0;
    while (qry.next())
    {
        d->playTest(qry.value(QStringLiteral("name")).toString(), qry.value(QStringLiteral("code")).toString());
        eventLoop.exec();
        d->m_actualTestRow++;
    }
}

void TestSuitesEditDlg::onMonkeyAppFinishedSignal(const QString &msg)
{
    d->m_reportModel->item(d->m_actualTestRow, 1)->setText(msg.isEmpty() && !d->m_testHasError ? tr("Success") : tr("Fail"));
    d->m_reportModel->item(d->m_actualTestRow, 2)->setText(QLocale::system().toString(d->m_elapsedTimer.elapsed() / 1000));
    d->m_reportModel->item(d->m_actualTestRow, 3)->setText(msg);
    setState(TestSuitesEditDlg::State::DoNothing);
    ui->tableViewReport->resizeColumnsToContents();
    emit testPlayed();
    d->m_monkeyCtrl->deleteLater();
}

void TestSuitesEditDlg::onMonkeyError(const QString &errMsg)
{
    d->m_reportModel->item(d->m_actualTestRow, 1)->setText(tr("Fail"));
    d->m_reportModel->item(d->m_actualTestRow, 2)->setText(QLocale::system().toString(d->m_elapsedTimer.elapsed() / 1000));
    d->m_reportModel->item(d->m_actualTestRow, 3)->setText(errMsg);
    setState(TestSuitesEditDlg::State::DoNothing);
    ui->tableViewReport->resizeColumnsToContents();
    d->m_testHasError = true;
}

void TestSuitesEditDlg::onMonkeyScriptEnd()
{
    d->m_testScriptEnd = true;
}

void TestSuitesEditDlg::setState(TestSuitesEditDlg::State state)
{
    d->m_state = state;
    ui->pbAdd->setEnabled(state == TestSuitesEditDlg::State::DoNothing);
    ui->pbEdit->setEnabled(state == TestSuitesEditDlg::State::DoNothing);
    ui->pbRemove->setEnabled(state == TestSuitesEditDlg::State::DoNothing);
    ui->pbSearch->setEnabled(state == TestSuitesEditDlg::State::DoNothing);
    ui->pbPlaySuite->setEnabled(state == TestSuitesEditDlg::State::DoNothing);
    ui->pbPlayTest->setEnabled(state == TestSuitesEditDlg::State::DoNothing);
    ui->leArguments->setEnabled(state == TestSuitesEditDlg::State::DoNothing);
    ui->leExecutablePath->setEnabled(state == TestSuitesEditDlg::State::DoNothing);
    ui->leName->setEnabled(state == TestSuitesEditDlg::State::DoNothing);
}

bool TestSuitesEditDlgPrivate::initTestModel()
{
    QSqlDatabase db = QSqlDatabase::database();
    m_model = new QSqlTableModel(q_ptr, db);
    m_model->setTable("testsuitestest");
    const QString filter = QStringLiteral("idtestsuite=%1").arg(q_ptr->editedData().value(QStringLiteral("id")).toString());
    m_model->setFilter(filter);
    m_model->setSort(m_model->fieldIndex("position"), Qt::AscendingOrder);
    m_model->setEditStrategy(QSqlTableModel::OnManualSubmit);
    q_ptr->ui->tableView->setModel(m_model);

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
    q_ptr->ui->tableView->resizeColumnsToContents();
    return true;
}

void TestSuitesEditDlgPrivate::initReportModel()
{
    QStringList headers;
    headers << QObject::tr("Test name")
            << QObject::tr("Success")
            << QObject::tr("Execution time (secs)")
            << QObject::tr("Error detail");
    m_reportModel->setHorizontalHeaderLabels(headers);
    q_ptr->ui->tableViewReport->setModel(m_reportModel);
}

void TestSuitesEditDlgPrivate::playTest(const QString &testName, const QString &qsCode)
{
    m_testHasError = false;
    m_testScriptEnd = false;
    m_monkeyCtrl = new QtMonkeyAppCtrl(q_ptr);
    QObject::connect(m_monkeyCtrl, SIGNAL(monkeyAppFinishedSignal(QString)), q_ptr,
            SLOT(onMonkeyAppFinishedSignal(QString)));
    QObject::connect(m_monkeyCtrl, SIGNAL(monkeyScriptEnd()), q_ptr,
            SLOT(onMonkeyScriptEnd()));
    QObject::connect(m_monkeyCtrl, SIGNAL(monkeyUserAppError(QString)), q_ptr,
            SLOT(onMonkeyError(QString)));
    QObject::connect(m_monkeyCtrl, SIGNAL(criticalError(QString)), q_ptr,
            SLOT(onMonkeyError(const QString &)));
    QObject::connect(m_monkeyCtrl, SIGNAL(newScriptError(QString)), q_ptr,
            SLOT(onMonkeyError(QString)));


    q_ptr->setState(TestSuitesEditDlg::State::PlayingEvents);
    m_actualRunningTestName = testName;
    m_elapsedTimer.restart();

    QList<QStandardItem *> rowItems;
    rowItems.append(new QStandardItem(m_actualRunningTestName));
    rowItems.append(new QStandardItem(QObject::tr("Running")));
    rowItems.append(new QStandardItem(""));
    rowItems.append(new QStandardItem(""));
    m_actualTestRow = m_reportModel->rowCount();
    m_reportModel->insertRow(m_actualTestRow, rowItems);

    m_monkeyCtrl->runScript(q_ptr->ui->leExecutablePath->text(),
                            splitCommandLine(q_ptr->ui->leArguments->text()),
                            qsCode,
                            "test.js");
}
