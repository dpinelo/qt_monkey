#include "testeditdlg.h"
#include "ui_testeditdlg.h"
#include "baseeditdlg.h"
#include "common.h"
#include "qtmonkeyappctrl.h"
#include "Qsci/qsciapis.h"
#include "Qsci/qscilexerjavascript.h"

class TestEditDlgPrivate
{
public:
    TestEditDlg *q_ptr;
    TestEditDlg::State m_state { TestEditDlg::State::DoNothing };
    QPointer<QtMonkeyAppCtrl> m_monkeyCtrl;
    QString m_path;
    QString m_arguments;
    QPointer<QsciLexer> m_lexer;
    QPointer<QsciAPIs> m_api;
    int m_idTestSuite { -1 };

    TestEditDlgPrivate(TestEditDlg *qq) : q_ptr(qq)
    {
        m_monkeyCtrl = new QtMonkeyAppCtrl(q_ptr);
        QObject::connect(m_monkeyCtrl, SIGNAL(monkeyAppFinishedSignal(QString)), q_ptr,
                SLOT(onMonkeyAppFinishedSignal(QString)));
        QObject::connect(m_monkeyCtrl, SIGNAL(monkeyAppNewEvent(QString)), q_ptr,
                SLOT(onMonkeyAppNewEvent(QString)));
        QObject::connect(m_monkeyCtrl, SIGNAL(monkeyUserAppError(QString)), q_ptr,
                SLOT(onMonkeyUserAppError(QString)));
        QObject::connect(m_monkeyCtrl, SIGNAL(monkeyScriptEnd()), q_ptr,
                SLOT(onMonkeyScriptEnd()));
        QObject::connect(m_monkeyCtrl, SIGNAL(monkeyScriptLog(QString)), q_ptr,
                SLOT(onMonkeyScriptLog(QString)));
        QObject::connect(m_monkeyCtrl, SIGNAL(criticalError(QString)), q_ptr,
                SLOT(showError(const QString &)));
        QObject::connect(m_monkeyCtrl, SIGNAL(newScriptError(QString)), q_ptr,
                SLOT(onMonkeyScriptLog(QString)));
        QObject::connect(m_monkeyCtrl, SIGNAL(newScriptError(QString)), q_ptr,
                SLOT(onMonkeyScriptError(QString)));
    }

    void setPosition();
    void record();
    void play();
};

TestEditDlg::TestEditDlg(int idTestSuite, QWidget *parent) :
    BaseEditDlg(parent),
    ui(new Ui::TestEditDlg),
    d(new TestEditDlgPrivate(this))
{
    ui->setupUi(this);
    d->m_idTestSuite = idTestSuite;

    ui->teScriptError->setVisible(false);

    ui->qsciCode->setAutoCompletionSource(QsciScintilla::AcsAll);
    ui->qsciCode->setAutoCompletionCaseSensitivity(false);
    ui->qsciCode->setAutoCompletionReplaceWord(true);
    ui->qsciCode->setAutoCompletionThreshold(1);
    ui->qsciCode->setBraceMatching(QsciScintilla::SloppyBraceMatch);
    ui->qsciCode->setFolding(QsciScintilla::BoxedTreeFoldStyle);
    ui->qsciCode->setIndentationsUseTabs(false);
    ui->qsciCode->setIndentationWidth(4);
    ui->qsciCode->setTabIndents(true);
    ui->qsciCode->setIndentationGuides(true);
    ui->qsciCode->setAutoIndent(true);
    ui->qsciCode->setUtf8(true);
    ui->qsciCode->setMarginType(1, QsciScintilla::NumberMargin);
    QString str = QString("0000%1").arg(ui->qsciCode->lines());
    ui->qsciCode->setMarginWidth(1, str);

    d->m_lexer = new QsciLexerJavaScript(this);
    d->m_api = new QsciAPIs(d->m_lexer.data());
    d->m_lexer->setAPIs(d->m_api.data());
    d->m_api->prepare();
    ui->qsciCode->setLexer(d->m_lexer);

    d->setPosition();
}

TestEditDlg::~TestEditDlg()
{
    delete d;
    delete ui;
}

const QVariantMap TestEditDlg::editedData()
{
    BaseEditDlg::editedData();
    m_data["code"] = ui->qsciCode->text();
    return m_data;
}

void TestEditDlg::setEditedData(const QVariantMap &data)
{
    BaseEditDlg::setEditedData(data);
    ui->qsciCode->setText(m_data.value(QStringLiteral("code")).toString());
}

void TestEditDlg::accept()
{
    if ( ui->leName->text().trimmed().isEmpty() )
    {
        QMessageBox::warning(this,
                             qApp->applicationName(),
                             tr("You must give a name test"));
        return;
    }
    BaseEditDlg::accept();
}

void TestEditDlg::reject()
{
    BaseEditDlg::reject();
}

void TestEditDlg::on_pbRecord_clicked()
{
    changeState(State::RecordEvents);
    d->record();
}

void TestEditDlg::on_pbReplay_clicked()
{
    changeState(State::PlayingEvents);
    d->play();
}

void TestEditDlg::setPath(const QString &path)
{
    d->m_path = path;
}

void TestEditDlg::setArguments(const QString &arguments)
{
    d->m_arguments = arguments;
}

void TestEditDlg::onMonkeyAppFinishedSignal(const QString &msg)
{
    if (!msg.isEmpty())
    {
        ui->teScriptError->append(msg);
        ui->teScriptError->setVisible(true);
    }
    else
    {
        logNewLine(MsgType::Default, QStringLiteral("The application has exited"));
    }
    changeState(State::DoNothing);
}

void TestEditDlg::onMonkeyAppNewEvent(const QString &scriptLine)
{
    if (d->m_state == State::RecordEvents)
    {
        QString text = ui->qsciCode->text();
        text.append(scriptLine).append("\n");
        ui->qsciCode->setText(text);
    }
    else if (d->m_state == State::PlayingEvents)
    {
        logNewLine(MsgType::Protocol, scriptLine);
    }
}

void TestEditDlg::onMonkeyUserAppError(const QString &errMsg)
{
    logNewLine(MsgType::Error, errMsg);
}

void TestEditDlg::onMonkeyScriptEnd()
{
    changeState(State::DoNothing);
    ui->teScriptError->setVisible(true);
    ui->teScriptError->append(tr("Script ejecutado con éxito"));
}

void TestEditDlg::onMonkeyScriptError(const QString &msg)
{
    ui->teScriptError->setVisible(true);
    ui->teScriptError->append(msg);
    d->m_monkeyCtrl->killApp();
}

void TestEditDlg::onMonkeyScriptLog(const QString &msg)
{
    logNewLine(MsgType::Default, msg);
}

void TestEditDlg::logNewLine(TestEditDlg::MsgType msgType, const QString &msg)
{
    QString type;
    switch (msgType) {
    case MsgType::Default:
        break;
    case MsgType::Error:
        type = QStringLiteral("Error");
        break;
    case MsgType::Protocol:
        type = QStringLiteral("Protocol");
        break;
    }
    QString text = msg;
    if (!type.isEmpty())
    {
        text = type.append(": ").append(msg);
    }
    ui->teLog->appendPlainText(text);
}

void TestEditDlg::changeState(TestEditDlg::State val)
{
    d->m_state = val;
    switch (d->m_state) {
    case State::DoNothing:
        ui->pbRecord->setEnabled(true);
        ui->buttonBox->setEnabled(true);
        ui->qsciCode->setEnabled(true);
        ui->pbReplay->setEnabled(true);
        break;
    case State::RecordEvents:
        ui->pbRecord->setEnabled(false);
        ui->buttonBox->setEnabled(false);
        ui->qsciCode->setEnabled(true);
        ui->pbReplay->setEnabled(false);
        break;
    case State::PlayingEvents:
        ui->qsciCode->setEnabled(false);
        ui->pbRecord->setEnabled(false);
        ui->buttonBox->setEnabled(false);
        ui->pbReplay->setEnabled(false);
        break;
    }
}

void TestEditDlg::showError(const QString &msg)
{
    QMessageBox::critical(this,
                          qApp->applicationName(),
                          msg);
}

void TestEditDlgPrivate::setPosition()
{
    QSqlDatabase db = QSqlDatabase::database();
    const QString sql = "SELECT coalesce(max(position), 0) as c FROM testsuitestest WHERE idtestsuite=:idTestSuite";
    QSqlQuery qry(db);
    if ( !qry.prepare(sql) )
    {
        qDebug() << Q_FUNC_INFO << qry.lastError().text();
        return;
    }
    qry.bindValue(":idTestSuite", m_idTestSuite);
    if ( !qry.exec() )
    {
        qDebug() << Q_FUNC_INFO << qry.lastError().text();
        return;
    }
    qry.first();
    q_ptr->ui->lePosition->setText(QString("%1").arg(qry.value(QStringLiteral("c")).toInt() + 1));
}

void TestEditDlgPrivate::record()
{
    q_ptr->ui->teScriptError->clear();
    q_ptr->ui->teScriptError->setVisible(false);
    q_ptr->ui->teLog->clear();
    m_monkeyCtrl->recordTest(m_path,
                             splitCommandLine(m_arguments));
}

void TestEditDlgPrivate::play()
{
    q_ptr->ui->teScriptError->clear();
    q_ptr->ui->teScriptError->setVisible(false);
    q_ptr->ui->teLog->clear();
    m_monkeyCtrl->runScript(m_path,
                            splitCommandLine(m_arguments),
                            q_ptr->ui->qsciCode->text(),
                            "test.js");
}
