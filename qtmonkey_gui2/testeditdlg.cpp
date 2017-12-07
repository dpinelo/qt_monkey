#include "testeditdlg.h"
#include "ui_testeditdlg.h"
#include "baseeditdlg.h"
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
        QObject::connect(m_monkeyCtrl, SIGNAL(monkeyAppNewEvent(const QString &)), q_ptr,
                SLOT(onMonkeyAppNewEvent(const QString &)));
        QObject::connect(m_monkeyCtrl, SIGNAL(monkeyUserAppError(const QString &)), q_ptr,
                SLOT(onMonkeyUserAppError(const QString &)));
        QObject::connect(m_monkeyCtrl, SIGNAL(monkeyScriptEnd()), q_ptr,
                SLOT(onMonkeyScriptEnd()));
        QObject::connect(m_monkeyCtrl, SIGNAL(monkeScriptLog(const QString &)), q_ptr,
                SLOT(onMonkeScriptLog(const QString &)));
        QObject::connect(m_monkeyCtrl, SIGNAL(criticalError(const QString &)), q_ptr,
                SLOT(showError(const QString &)));
    }

    void setPosition();
    void record();
};

TestEditDlg::TestEditDlg(int idTestSuite, QWidget *parent) :
    BaseEditDlg(parent),
    ui(new Ui::TestEditDlg),
    d(new TestEditDlgPrivate(this))
{
    ui->setupUi(this);
    d->m_idTestSuite = idTestSuite;

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
        QMessageBox::critical(this,
                              qApp->applicationName(),
                              msg);
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
}

void TestEditDlg::onMonkeScriptLog(const QString &msg)
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
        break;
    case State::RecordEvents:
        ui->pbRecord->setEnabled(false);
        ui->buttonBox->setEnabled(false);
        ui->qsciCode->setEnabled(true);
        break;
    case State::PlayingEvents:
        ui->qsciCode->setEnabled(false);
        ui->pbRecord->setEnabled(false);
        ui->buttonBox->setEnabled(false);
        break;
    }
}

void TestEditDlg::showError(const QString &msg)
{
    QMessageBox::critical(this,
                          qApp->applicationName(),
                          msg);
}

static QStringList splitCommandLine(const QString &cmdLine)
{
    QStringList list;
    QString arg;
    bool escape = false;
    enum { Idle, Arg, QuotedArg } state = Idle;
    for (QChar const c : cmdLine) {
        if (!escape && c == '\\') {
            escape = true;
            continue;
        }
        switch (state) {
        case Idle:
            if (!escape && c == '"')
                state = QuotedArg;
            else if (escape || !c.isSpace()) {
                arg += c;
                state = Arg;
            }
            break;
        case Arg:
            if (!escape && c == '"') {
                state = QuotedArg;
            } else if (escape || !c.isSpace()) {
                arg += c;
            } else {
                list << arg;
                arg.clear();
                state = Idle;
            }
            break;
        case QuotedArg:
            if (!escape && c == '"')
                state = arg.isEmpty() ? Idle : Arg;
            else
                arg += c;
            break;
        }
        escape = false;
    }
    if (!arg.isEmpty())
        list << arg;
    return list;
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
    q_ptr->ui->lePosition->setText(QString("%1").arg(qry.value("c").toInt() + 1));
}

void TestEditDlgPrivate::record()
{
    m_monkeyCtrl->recordTest(m_path,
                             splitCommandLine(m_arguments));
}
