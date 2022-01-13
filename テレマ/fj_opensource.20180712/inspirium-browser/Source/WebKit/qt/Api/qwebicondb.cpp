/*** COPYRIGHT FUJITSU LIMITED 2017 ***/

#include "config.h"
#include "qwebicondb.h"
#include "ReadyToWriteCallBackManager.h"
#include "SQLiteDatabase.h"

using namespace WebCore;

QWebIconDB::QWebIconDB()
{
}

QWebIconDB::~QWebIconDB()
{
}

void QWebIconDB::acquire()
{
    SQLiteDatabase* db = ReadyToWriteCallBackManager::getInstance()->getIconSQLiteDB();
    if (db)
    {
        db->databaseMutex().lock();
    }
}

void QWebIconDB::release()
{
    SQLiteDatabase* db = ReadyToWriteCallBackManager::getInstance()->getIconSQLiteDB();
    if (db)
    {
        db->databaseMutex().unlock();
    }
}



