/*                   COPYRIGHT FUJITSU LIMITED 2015-2015					 */

#include "config.h"
#include "ApplicationCacheStorage.h"

#include "ApplicationCache.h"
#include "ApplicationCacheGroup.h"
#include "ApplicationCacheHost.h"
#include "ApplicationCacheResource.h"
#include "FileSystem.h"
#include "URL.h"
#include "NotImplemented.h"
#include "SQLiteStatement.h"
#include "SQLiteTransaction.h"
#include "SecurityOrigin.h"
#include "UUID.h"
#include <wtf/text/CString.h>
#include <wtf/StdLibExtras.h>
#include <wtf/StringExtras.h>
#include <wtf/text/StringBuilder.h>

#ifndef WIN32
/*  getpid() */
#include <sys/types.h>
#include <unistd.h>
/*  readlink() */
#include <fcntl.h>
#endif

using namespace std;

namespace WebCore {

static const char flatFileSubdirectory[] = "ApplicationCache";

/****************************************************************************
*
* Description :Delete Applicationcache Files.
*               
* Arguments :   NONE
*                       
* Return Values : NONE
*
****************************************************************************/

#ifdef WIN32
BOOL PathRemoveFileSpecAppCache(LPWSTR moduleFileNameStr)
{
    if (!*moduleFileNameStr)
        return FALSE;

    LPWSTR lastPos = 0;
    LPWSTR curPos = moduleFileNameStr;
    do {
        if (*curPos == L'/' || *curPos == L'\\')
            lastPos = curPos;
    } while (*++curPos);

    if (lastPos == curPos - 1)
        return FALSE;

    if (lastPos)
        *lastPos = 0;
    else {
        moduleFileNameStr[0] = L'\\';
        moduleFileNameStr[1] = 0;
    }

    return TRUE;
}
#endif

void ApplicationCacheStorage::deleteOfflineWebApplicationCacheFiles()
{

    if(m_cacheDirectory.isNull())
    {
        return;
    }

    //ApplicationCache.db's path
    String AppcacheFilePath = pathByAppendingComponent(m_cacheDirectory,"ApplicationCache.db");

    bool deleteSuccess = false;
	//20131014 added 
	bool openedFlag = false;
	bool existedFlag = false;
    if(fileExists(AppcacheFilePath))
    {

    	existedFlag=true;
    	//20131014 added 
	    if (m_database.isOpen())
	    {
	    	openedFlag=true;
	    	m_database.close();
	    }
    	//20131014

        //Delete ApplicationCache.db
        deleteSuccess = deleteFile(AppcacheFilePath);
        ASSERT(deleteSuccess);
        if(!deleteSuccess)
        {
            LOG_ERROR("Delete ApplicationCache.db failed\n");
        }
    }
    else
    {
        LOG_ERROR("ApplicationCache.db is not exist\n");
    }

    String flatFileDirectory = pathByAppendingComponent(m_cacheDirectory, flatFileSubdirectory);
    Vector<String> paths;
    {
        paths = listDirectory(flatFileDirectory,String("*"));
    }
    //delete file from flatFileDirectory
    Vector<String>::const_iterator end = paths.end();
    for (Vector<String>::const_iterator it = paths.begin(); it != end; ++it) {
        String flatFilePath = *it;
        deleteSuccess = deleteFile(flatFilePath);
		ASSERT(deleteSuccess);
        if(!deleteSuccess)
        {
            LOG_ERROR("Delete flatfile failed\n");
        }
    }
	//Delete flatFileDirectory.
    deleteSuccess = deleteEmptyDirectory(flatFileDirectory);
    ASSERT(deleteSuccess);
    if(!deleteSuccess)
    {
        LOG_ERROR("Delete flatFileDirectory failed");
    }

#ifdef WIN32
	//20131014 added 
    if(existedFlag == true){
    	const int nBufSize = 512;
    	TCHAR chBuf[nBufSize];
    	ZeroMemory(chBuf,nBufSize);
		TCHAR* lpStrPath = NULL;
		if (GetModuleFileName(NULL,chBuf,nBufSize))
		{
			lpStrPath = chBuf;
			PathRemoveFileSpecAppCache(lpStrPath);
		}
		String modulePath(lpStrPath);
    	String InitAppcacheFilePath = pathByAppendingComponent(modulePath,"InitApplicationCache.db");
//    	String InitAppcacheFilePath = pathByAppendingComponent(m_cacheDirectory,"InitApplicationCache.db");

    	if(fileExists(InitAppcacheFilePath))
    	{
        	copyFile(InitAppcacheFilePath , AppcacheFilePath);
			//when copy file from romimg, default is readonly.
        	SetFileAttributes(AppcacheFilePath.charactersWithNullTermination().data(), FILE_ATTRIBUTE_NORMAL);	
		    if(openedFlag == true)
		    {
			    if (m_database.isOpen())
			        return;
			    // The cache directory should never be null, but if it for some weird reason is we bail out.
			    if (m_cacheDirectory.isNull())
			        return;

			    m_cacheFile = pathByAppendingComponent(m_cacheDirectory, "ApplicationCache.db");
			    /*if (!createIfDoesNotExist && !fileExists(m_cacheFile))
			        return;*/

			    makeAllDirectories(m_cacheDirectory);
			    m_database.open(m_cacheFile);
		    	if (!m_database.isOpen())
		        	return;

			    verifySchemaVersion();
		    }
		}
		else
		{
			//if there are no "InitApplicationCache.db" file  , create new db file.
			//but this will take some time ( about 5~10 sec).
			openDatabase(true);
		}
	}
	//20131014
#else
	//20131014 added 
    if(existedFlag == true){
    	//20131213 modified
    	//get exec file path . there are InitApplicationCache.db file . 
    	//if there are not exist its db file or can't get exec file path , do openDataBase(true) it will take 5~10 sec
	    static char buf[1024]={};
	    char path[256];
	    sprintf( path, "/proc/%d/exe", getpid() );
	    if(readlink( path, buf, sizeof(buf)-1 ) == -1){
	        LOG_ERROR("Get exec file path failed");
			openDatabase(true);				//when can't get exec file path , make db by using slow api.
			return;
	    }else{
	    	char* pos = buf;
	    	char* lastSepPos=0;
	    	do{
	    		if(*pos== L'/' || *pos ==L'\\'){
	    			lastSepPos = pos;
	    		}
	    	}while(*++pos);	//find last file seperator.
	    	
	    	if(lastSepPos == pos -1){
		        LOG_ERROR("Get exec dir path failed");
				openDatabase(true);		//when got file path is dir path , make db by using slow api.
				return;
	    	}
	    	
	    	if(lastSepPos){
	    		*lastSepPos = 0;		//change last position to null term.
	    	}else{
	    		buf[0] = L'/';
	    		buf[1] = 0;
	    	}
	    }
    	String modulePath(buf);
  		String InitAppcacheFilePath = pathByAppendingComponent(modulePath,"InitApplicationCache.db");

		//-20131213 modified

    	if(fileExists(InitAppcacheFilePath))
    	{
        	copyFile(InitAppcacheFilePath , AppcacheFilePath);
			/*note:
			// when copy file , Attr may become readonly.
			//if so , need to change attr.
			*/
        	         	
		    if(openedFlag == true)
		    {
			    if (m_database.isOpen()){
			        return;
			    }
			    // The cache directory should never be null, but if it for some weird reason is we bail out.
			    if (m_cacheDirectory.isNull()){
			        return;
			    }

			    m_cacheFile = pathByAppendingComponent(m_cacheDirectory, "ApplicationCache.db");
			    /*if (!createIfDoesNotExist && !fileExists(m_cacheFile))
			        return;*/

			    makeAllDirectories(m_cacheDirectory);
			    m_database.open(m_cacheFile);
		    	if (!m_database.isOpen())
		        	return;
			    verifySchemaVersion();
		    }
		}
		else
		{
			//if there are no "InitApplicationCache.db" file  , create new db file.
			//but this will take some time ( about 5~10 sec).
			openDatabase(true);
		}
	}
	//20131014
#endif

}

} // namespace WebCore
