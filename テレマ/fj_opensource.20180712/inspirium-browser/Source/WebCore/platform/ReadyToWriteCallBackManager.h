/*** COPYRIGHT FUJITSU LIMITED 2017 ***/

#ifndef ReadyToWriteCallBackManager_h
#define ReadyToWriteCallBackManager_h

#include "config.h"
#include "FjFileAccessNotifierListener.h"
#include <wtf/Vector.h>
#include <wtf/ThreadingPrimitives.h>

namespace WebCore {

class SQLiteDatabase;

class WEBCORE_API ReadyToWriteCallBackManager {
public:
    static ReadyToWriteCallBackManager* getInstance();

    void setIconSQLiteDB(SQLiteDatabase* db);
    void setLocalStorageSQLiteDB(SQLiteDatabase* db);
    void setAppCacheSQLiteDB(SQLiteDatabase* db);

    void removeLocalStorageSQLiteDB(SQLiteDatabase* db);

    SQLiteDatabase* getIconSQLiteDB();
    Vector<SQLiteDatabase*> getLocalStorageSQLiteDBs();
    Mutex* getLocalStorageLocker();
    SQLiteDatabase* getAppCacheSQLiteDB();

    void readyToWriteCallback(FjFileAccessNotifierFileType type);

private:
    ReadyToWriteCallBackManager();
    ~ReadyToWriteCallBackManager();
    SQLiteDatabase* m_iconDB;
    Vector<SQLiteDatabase*> m_localStorageDBs;
    SQLiteDatabase* m_appCacheDB;
    Mutex m_localStorageLocker;
};

} // namespace WebCore

#endif /* ReadyToWriteCallBackManager_h */

