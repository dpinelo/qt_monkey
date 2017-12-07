#include <QtCore>
#include <QtSql>
#include "database.h"

QString Database::m_lastErrorMessage;

Database::Database()
{
}

const QString Database::dbPath()
{
    const QString path = QString("%1/qtmonkey_gui2.db3").
            arg(QStandardPaths::writableLocation(QStandardPaths::DataLocation));
    QDir dir(path);
    if ( !dir.exists() )
    {
        dir.mkpath(QStandardPaths::writableLocation(QStandardPaths::DataLocation));
    }
    return path;
}

bool Database::openSQLite()
{
    const QString driverName = "QSQLITE";
    if ( !QSqlDatabase::isDriverAvailable(driverName) )
    {
        const QStringList list = QSqlDatabase::drivers();
        const QString errorDriverNotAvailable = QObject::tr("Driver not available. Available drivers: ").arg(list.join(" - "));
        Database::m_lastErrorMessage = errorDriverNotAvailable;
        return false;
    }

    const QString dbPath = Database::dbPath();
    QSqlDatabase db = QSqlDatabase::addDatabase(driverName);
    db.setDatabaseName(dbPath);
    db.open();
    QSqlError lastError = db.lastError();
    if ( lastError.isValid() && lastError.type() != QSqlError::NoError )
    {
        Database::m_lastErrorMessage = db.lastError().text();
        return false;
    }
    return true;
}

/*!
  Crea las tablas de sistema en la base de datos SQLite que contiene el código QtScript, las
  definiciones de los formularios de usuario... Es la base de datos local, para dar agilidad al sistema
  */
bool Database::initDatabase()
{
    QSqlDatabase db = QSqlDatabase::database();
    if ( !db.isOpen() )
    {
        Database::m_lastErrorMessage = QObject::tr("Database not open");
        return false;
    }
    QFile ddl(":/ddl.sql");
    if ( !ddl.open(QIODevice::ReadOnly) )
    {
        Database::m_lastErrorMessage = QObject::tr("Cannot open DDL file");
        return false;
    }
    const QByteArray baDdlSql = ddl.readAll();
    const QStringList ddls = QString(baDdlSql).replace("\n", "").replace("\r", "").split(";");
    for( const QString &ddlSql : ddls )
    {
        if ( ddlSql.isEmpty() )
        {
            continue;
        }
        QScopedPointer<QSqlQuery> qry (new QSqlQuery(db));
        if ( !qry->exec(ddlSql) )
        {
            Database::m_lastErrorMessage = qry->lastError().text();
            return false;
        }
    }
    return true;
}

