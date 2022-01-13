/*** COPYRIGHT FUJITSU LIMITED 2017 ***/

#include "config.h"
#include "ReadyToWriteCallBackManager.h"
#include "FjFileAccessNotifier.h"
#include "SQLiteDatabase.h"
#include <wtf/MainThread.h>
#include <wtf/Functional.h>

namespace WebCore {

ReadyToWriteCallBackManager* ReadyToWriteCallBackManager::getInstance()
{
    static ReadyToWriteCallBackManager gInstance;
    return &gInstance;
}

ReadyToWriteCallBackManager::ReadyToWriteCallBackManager()
    : m_iconDB(NULL)
    , m_appCacheDB(NULL)
{
    m_localStorageDBs.clear();
}

ReadyToWriteCallBackManager::~ReadyToWriteCallBackManager()
{
}

void ReadyToWriteCallBackManager::setIconSQLiteDB(SQLiteDatabase* db)
{
    m_iconDB = db;
}

void ReadyToWriteCallBackManager::setLocalStorageSQLiteDB(SQLiteDatabase* db)
{
    MutexLocker locker(m_localStorageLocker);
    const size_t pos = m_localStorageDBs.find(db);
    if (pos == notFound)
        m_localStorageDBs.append(db);
}

void ReadyToWriteCallBackManager::removeLocalStorageSQLiteDB(SQLiteDatabase* db)
{
    MutexLocker locker(m_localStorageLocker);
    const size_t pos = m_localStorageDBs.find(db);
    if (pos != notFound)
        m_localStorageDBs.remove(pos);
}

void ReadyToWriteCallBackManager::setAppCacheSQLiteDB(SQLiteDatabase* db)
{
    m_appCacheDB = db;
}

SQLiteDatabase* ReadyToWriteCallBackManager::getIconSQLiteDB()
{
    return m_iconDB;
}

Vector<SQLiteDatabase*> ReadyToWriteCallBackManager::getLocalStorageSQLiteDBs()
{
    return m_localStorageDBs;
}

Mutex* ReadyToWriteCallBackManager::getLocalStorageLocker()
{
    return &m_localStorageLocker;
}

SQLiteDatabase* ReadyToWriteCallBackManager::getAppCacheSQLiteDB()
{
    return m_appCacheDB;
}

void readyToWriteCallbackImpl(FjFileAccessNotifierFileType type , bool needLock)
{
    bool result = false;
    CFjFileAccessNotifier* notifier = CFjFileAccessNotifier::getInstance();
    CFjFileAccessNotifierListener* listener = notifier->getListener();
    if (listener) 
    {
        listener->readyToWrite(type, needLock, result);
        if (result == false)
        {
            printf("*** CRASH! because readyToWrite return false. ***\n");
            CRASH();
        }
    }
    else 
    {
        // Do nothing.
    }
}

void ReadyToWriteCallBackManager::readyToWriteCallback(FjFileAccessNotifierFileType type)
{
    if (isMainThread())
    {
        readyToWriteCallbackImpl(type, false); 
    } 
    else 
    {
        callOnMainThread(WTF::bind(&readyToWriteCallbackImpl, type , true));
    }
}

} // namespace WebCore

