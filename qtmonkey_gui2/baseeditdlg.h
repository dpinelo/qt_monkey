#ifndef BASEEDITDLG_H
#define BASEEDITDLG_H

#include <QtWidgets>
#include <QtSql>

class BaseEditDlg : public QDialog
{
    Q_OBJECT

public:
    BaseEditDlg(QWidget *parent = 0, Qt::WindowFlags fl = Qt::WindowFlags());
    virtual ~BaseEditDlg();

    const QVariantMap data() const;

    static bool addRecord(QSqlTableModel *model, const QVariantMap &data, int row);
    static bool editRecord(QSqlTableModel *model, int row, BaseEditDlg *dlg);

public slots:
    virtual void setData(const QVariantMap &data);
    void accept() Q_DECL_OVERRIDE;
    void reject() Q_DECL_OVERRIDE;

protected:
    QVariantMap m_data;

};

#endif // BASEEDITDLG_H
