/*** COPYRIGHT FUJITSU LIMITED 2017 ***/

#include "config.h"
#include "qwebappcachedb.h"
#include "ReadyToWriteCallBackManager.h"
#include "SQLiteDatabase.h"

using namespace WebCore;

QWebAppCacheDB::QWebAppCacheDB()
{
}

QWebAppCacheDB::~QWebAppCacheDB()
{
}

void QWebAppCacheDB::acquire()
{
    SQLiteDatabase* db = ReadyToWriteCallBackManager::getInstance()->getAppCacheSQLiteDB();
    if (db)
    {
        db->databaseMutex().lock();
    }
}

void QWebAppCacheDB::release()
{
    SQLiteDatabase* db = ReadyToWriteCallBackManager::getInstance()->getAppCacheSQLiteDB();
    if (db)
    {
        db->databaseMutex().unlock();
    }
}



