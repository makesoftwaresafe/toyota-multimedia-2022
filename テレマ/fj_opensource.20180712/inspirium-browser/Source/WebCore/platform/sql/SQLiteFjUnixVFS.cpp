/***                           COPYRIGHT FUJITSU LIMITED 2016- */

#include "SQLiteFjUnixVFS.h"

#if USE(FJIB_HOOK_SQLITE3_UNIX_VFS)
#include "SQLiteFileSystem.h"
#include "ReadyToWriteCallBackManager.h"
#include <wtf/Functional.h>
#include <wtf/MainThread.h>
#include <wtf/HashMap.h>
#include <wtf/text/WTFString.h>

namespace WebCore {
// save OrgIOMethods  sqlite3_io_methods  sqlite3_file's name
static HashMap<sqlite3_file*, FjOrgIOMethods*> gs_ioMethodsMap;

static const char* gs_vfsName = "fjunix";
static const char* gs_applicationCache_str = "ApplicationCache.db";
static const char* gs_storageTracker_str = "StorageTracker.db";
static const char* gs_localstorage_str = ".localstorage";

FjFileAccessNotifierFileType getFileAccessNotifierFileType(const char* name)
{
    String fileName(name);
    if (fileName.contains(gs_applicationCache_str)) {
        return FileType_ApplicationCache;
    } else if (fileName.contains(gs_storageTracker_str)) {
        return FileType_LocalStorage;
    } else if (fileName.contains(gs_localstorage_str)) {
        return FileType_LocalStorage;
    } else {
        return FileType_Unknown;
    }
}

static int mod_xClose(sqlite3_file* file)
{
    int r = -1;
    FjOrgIOMethods* orgIOMethods = gs_ioMethodsMap.get(file);
    if (orgIOMethods && orgIOMethods->xClose) {
        r = orgIOMethods->xClose(file);
        FjFileAccessNotifierFileType fileType = getFileAccessNotifierFileType(orgIOMethods->fileName);
        ReadyToWriteCallBackManager::getInstance()->readyToWriteCallback(fileType);
    } else {
        printf("*** CRASH! because SQLiteFjUnixVFS's xClose is null. ***\n");
        CRASH();
    }

    if(r == SQLITE_OK)
    {
        gs_ioMethodsMap.remove(file);
        delete orgIOMethods->ioMethods;
        delete orgIOMethods;
    }
    return r;
}

static int mod_xSync(sqlite3_file* file, int flags)
{
    int r = -1;
    FjOrgIOMethods* orgIOMethods = gs_ioMethodsMap.get(file);
    if (orgIOMethods && orgIOMethods->xSync) {
        r = orgIOMethods->xSync(file, flags);
        FjFileAccessNotifierFileType fileType = getFileAccessNotifierFileType(orgIOMethods->fileName);
        ReadyToWriteCallBackManager::getInstance()->readyToWriteCallback(fileType);
    }
    return r;
}

static int mod_xOpen(sqlite3_vfs* vfs, const char *zName, sqlite3_file* file, int flags, int *pOutFlags)
{
    int r = -1;
	sqliteVfsOpen vfsOpen = SQLiteFileSystem::fjUnixVfsOpen();
    if (vfsOpen) {
        r = vfsOpen(vfs, zName, file, flags, pOutFlags);
        if(r == SQLITE_CANTOPEN){
            return r;
        }

        FjOrgIOMethods* orgIOMethods = 0;
        orgIOMethods = gs_ioMethodsMap.get(file);
        if (orgIOMethods) {
            return 0;
        }

        orgIOMethods = new FjOrgIOMethods();
        sqlite3_io_methods* methods = new sqlite3_io_methods();
        // copy original sqlite3_io_methods first.
        memcpy(methods, file->pMethods, sizeof(sqlite3_io_methods));

        // in oeder to delete new methods, save the methods in  FjOrgIOMethods struct.
        orgIOMethods->ioMethods = methods;

        // back up original xClose
        if (file->pMethods->xClose != mod_xClose) {
            orgIOMethods->xClose = file->pMethods->xClose;
        }
        // Override xClose()
        methods->xClose = mod_xClose;

        // back up original xSync
        if (file->pMethods->xSync != mod_xSync) {
            orgIOMethods->xSync = file->pMethods->xSync;
        }
        // Override xSync()
        methods->xSync = mod_xSync;

        file->pMethods = methods;

        orgIOMethods->fileName = zName;
        gs_ioMethodsMap.add(file, orgIOMethods);
    }
    return r;
}

static int mod_xDelete(sqlite3_vfs* vfs, const char *zName, int syncDir)
{
    int r = -1;
	sqliteVfsDelete vfsDelete = SQLiteFileSystem::fjUnixVfsDelete();
    if (vfsDelete) {
        r = vfsDelete(vfs, zName, syncDir);
        FjFileAccessNotifierFileType fileType = getFileAccessNotifierFileType(zName);
        ReadyToWriteCallBackManager::getInstance()->readyToWriteCallback(fileType);
    }
    else {
        printf("CRASH! because SQLiteFjUnixVFS's xDelete is null. ***\n");
        CRASH();
    }
    return r;
}

void initSQLiteFjUnixVFS()
{
    static bool init = false;
    if (init == false) {
#if defined(_WIN32) || defined(_WIN64)
        sqlite3_vfs *vfs = sqlite3_vfs_find(NULL);
#else
        sqlite3_vfs *vfs = sqlite3_vfs_find("unix");
#endif
        if (!vfs) {
            printf("*** CRASH! because not find unix sqlite3_vfs! ***\n");
            CRASH();
        }
        else {
            // copy original sqlite3_vfs first
			sqlite3_vfs& fjUnixVfs = SQLiteFileSystem::fjUnixVfs();
            memcpy(&fjUnixVfs, vfs, sizeof(sqlite3_vfs));

            // backup xOpen
            if (SQLiteFileSystem::fjUnixVfsOpen() == NULL) {
                SQLiteFileSystem::setFjUnixVfsOpen(vfs->xOpen);
                fjUnixVfs.xOpen = mod_xOpen;
            }
            // back up xDelete
            if (SQLiteFileSystem::fjUnixVfsDelete() == NULL) {
                SQLiteFileSystem::setFjUnixVfsDelete(vfs->xDelete);
                fjUnixVfs.xDelete = mod_xDelete;
            }

            fjUnixVfs.zName = gs_vfsName;
            sqlite3_vfs_register(&fjUnixVfs, 1);
            vfs = sqlite3_vfs_find(gs_vfsName);
            if (!vfs) {
                printf("*** CRASH! because not find fjunix sqlite3_vfs!! ***\n");
                CRASH();
            }
        }
        init = true;
    }
}
} // namespace WebCore

#endif
