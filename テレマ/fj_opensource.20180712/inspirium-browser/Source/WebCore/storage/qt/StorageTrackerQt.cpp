/*                   COPYRIGHT FUJITSU LIMITED 2015-2015					 */

#include "config.h"
#include "StorageTracker.h"

#include "DatabaseThread.h"
#include "FileSystem.h"
#include "StorageThread.h"
#include "Logging.h"
#include "PageGroup.h"
#include "SQLiteFileSystem.h"
#include "SQLiteStatement.h"
#include "SecurityOrigin.h"
#include "StorageTrackerClient.h"
#include "TextEncoding.h"
#include <wtf/MainThread.h>
#include <wtf/StdLibExtras.h>
#include <wtf/Vector.h>
#include <wtf/text/CString.h>

namespace WebCore {

void StorageTracker::platformInit()
{
	;
}

void StorageTracker::waitForSemaphore()
{
    g_hSemaphore.acquire(1);
}

void StorageTracker::releaseSemaphore()
{
    g_hSemaphore.release(1);
}

}

