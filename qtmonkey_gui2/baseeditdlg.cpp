#include "baseeditdlg.h"

#define DATABASE_COLUMN "databaseColumn"

BaseEditDlg::BaseEditDlg(QWidget *parent, Qt::WindowFlags fl) :
    QDialog(parent, fl)
{

}

BaseEditDlg::~BaseEditDlg()
{

}

const QVariantMap BaseEditDlg::editedData()
{
    return m_data;
}

bool BaseEditDlg::addRecord(QSqlTableModel *model, const QVariantMap &data, int row)
{
    if ( !model->insertRow(row) )
    {
        QMessageBox::warning(Q_NULLPTR,
                             qApp->applicationName(),
                             tr("An error ocurred trying to insert new record.\nError: %1").arg(model->lastError().text()));
        return false;
    }
    QMapIterator<QString, QVariant> it(data);
    while (it.hasNext())
    {
        it.next();
        const QModelIndex idx = model->index(row, model->fieldIndex(it.key()));
        model->setData(idx, it.value());
    }

    if ( !model->submitAll() )
    {
        model->revertAll();
        QMessageBox::warning(Q_NULLPTR,
                             qApp->applicationName(),
                             tr("An error ocurred trying to insert new record.\nError: %1").arg(model->lastError().text()));
        return false;
    }
    return true;
}

bool BaseEditDlg::editRecord(QSqlTableModel *model, int row, BaseEditDlg *dlg)
{
    QSqlRecord record = model->record(row);
    QVariantMap data;
    for (int i = 0 ; i < record.count() ; ++i)
    {
        data[record.fieldName(i)] = record.value(i);
    }
    dlg->setEditedData(data);
    if ( dlg->exec() == QDialog::Rejected )
    {
        return false;
    }
    data = dlg->editedData();
    QMapIterator<QString, QVariant> it(data);
    while (it.hasNext())
    {
        it.next();
        const QModelIndex idx = model->index(row, model->fieldIndex(it.key()));
        model->setData(idx, it.value());
    }
    if ( !model->submitAll() )
    {
        model->revertAll();
        QMessageBox::warning(Q_NULLPTR,
                             qApp->applicationName(),
                             tr("An error ocurred trying to update record.\nError: %1").arg(model->lastError().text()));
        return false;
    }
    return true;
}

void BaseEditDlg::setEditedData(const QVariantMap &data)
{
    const QWidgetList list = findChildren<QWidget *>();
    m_data = data;
    for (QWidget *widget : list)
    {
        if ( !widget->property(DATABASE_COLUMN).isValid() )
        {
            continue;
        }
        if ( !data.contains(widget->property(DATABASE_COLUMN).toString()) )
        {
            continue;
        }
        QLineEdit *le = qobject_cast<QLineEdit *>(widget);
        if ( le == Q_NULLPTR )
        {
            continue;
        }
        le->setText(data.value(widget->property(DATABASE_COLUMN).toString()).toString());
    }
}

void BaseEditDlg::accept()
{
    const QWidgetList list = findChildren<QWidget *>();
    for (QWidget *widget : list)
    {
        if ( !widget->property(DATABASE_COLUMN).isValid() )
        {
            continue;
        }
        QLineEdit *le = qobject_cast<QLineEdit *>(widget);
        if ( le == Q_NULLPTR )
        {
            continue;
        }
        m_data[widget->property(DATABASE_COLUMN).toString()] = le->text();
    }
    QDialog::accept();
}

void BaseEditDlg::reject()
{
    QDialog::reject();
}
