#ifndef DATABASE_H
#define DATABASE_H

#include <QtCore>
#include <QtSql>

/**
    Clase abstracta que envuelve los métodos para la apertura de conexiones a base de datos.
    Se utiliza el pool propio que implementa QSqlDatabase.
*/
class Database
{

public:
    static QString m_lastErrorMessage;
    Database();

    static const QString dbPath();
    static bool openSQLite();
    static bool initDatabase();
};

#endif
