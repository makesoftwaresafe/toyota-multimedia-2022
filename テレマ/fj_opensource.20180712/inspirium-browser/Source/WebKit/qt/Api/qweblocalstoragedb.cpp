/*** COPYRIGHT FUJITSU LIMITED 2017 ***/

#include "config.h"
#include "qweblocalstoragedb.h"
#include "ReadyToWriteCallBackManager.h"
#include "SQLiteDatabase.h"

using namespace WebCore;

QWebLocalStorageDB::QWebLocalStorageDB()
{
}

QWebLocalStorageDB::~QWebLocalStorageDB()
{
}

void QWebLocalStorageDB::acquire()
{
    ReadyToWriteCallBackManager::getInstance()->getLocalStorageLocker()->lock();
    Vector<SQLiteDatabase*> dbVector = ReadyToWriteCallBackManager::getInstance()->getLocalStorageSQLiteDBs();
    int size = dbVector.size();
    for (int i = 0 ; i < size; i++)
    {
        SQLiteDatabase* db = dbVector.at(i);
        if (db)
            db->databaseMutex().lock();
    }
}

void QWebLocalStorageDB::release()
{
    Vector<SQLiteDatabase*> dbVector = ReadyToWriteCallBackManager::getInstance()->getLocalStorageSQLiteDBs();
    int size = dbVector.size();
    for (int i = 0 ; i < size; i++)
    {
        SQLiteDatabase* db = dbVector.at(i);
        if (db)
            db->databaseMutex().unlock();
    }
    ReadyToWriteCallBackManager::getInstance()->getLocalStorageLocker()->unlock();
}



