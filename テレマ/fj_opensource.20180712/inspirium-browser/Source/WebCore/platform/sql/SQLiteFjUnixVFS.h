/***                           COPYRIGHT FUJITSU LIMITED 2016- */

#ifndef SQLiteFjUnixVFS_h
#define SQLiteFjUnixVFS_h
#include "config.h"
#include <sqlite3.h>

#if USE(FJIB_HOOK_SQLITE3_UNIX_VFS)

namespace WebCore {

// sqlite3_vfs_xOpen
typedef int (*sqliteVfsOpen)(sqlite3_vfs*, const char *zName, sqlite3_file*, int flags, int *pOutFlags);

// sqlite3_vfs_xDelete
typedef int (*sqliteVfsDelete)(sqlite3_vfs*, const char *zName, int syncDir);

// sqlite3_io_methods_xClose
typedef int (*sqliteIOMethodClose)(sqlite3_file*);

// sqlite3_io_methods_xSync
typedef int (*sqliteIOMethodSync)(sqlite3_file*, int flags);

typedef struct {
        sqliteIOMethodClose xClose;      // original sqlite3_io_methods's xClose handler.
        sqliteIOMethodSync xSync;        // original sqlite3_io_methods's xSync handler.
        sqlite3_io_methods* ioMethods;   // save new sqlite3_file's sqlite3_io_methods that need delete later.
        const char* fileName;            // sqlite3_file's name.
} FjOrgIOMethods;

void initSQLiteFjUnixVFS();
}

#endif// USE(FJIB_HOOK_SQLITE3_UNIX_VFS)
#endif 


