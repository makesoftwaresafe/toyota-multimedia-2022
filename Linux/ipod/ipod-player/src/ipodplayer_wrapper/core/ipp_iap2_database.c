#include "ipp_iap2_database.h"
#include "ipp_iap2_dbstatement.h"
#include "iPodPlayerCoreCfg.h"
#include "ipp_iap2_observer.h"


//#define SQL_STATEMENT_LOG
#if 0
static U32 ippGetTime()
{
    S32 rc = -1;
    U32 iPodTime = 0;
    struct timespec rtime;
    
    /* Initialize the structure */
    memset(&rtime, 0, sizeof(rtime));
    
    rc = clock_gettime(CLOCK_MONOTONIC, &rtime);
    if(rc == 0)
    {
        iPodTime = ((rtime.tv_sec % 10000) * 1000)  + (rtime.tv_nsec / 1000000);
    }
    
    return iPodTime;
}
#endif

static int ippiAP2DBSetBind(sqlite3_stmt *stmt, IPP_IAP2_DB_BIND_TYPE type, U32 count, void *data, U32 pos)
{
    int ret = -1;
    S32 intData = 0;
    S64 longData = 0;
    U8 *strData = NULL;
    
    /* Parameter check */
    if(stmt == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, stmt);
        return ret;
    }
    
    /* Check bind type */
    switch(type)
    {
    /* bind is U8  */
    case IPP_IAP2_DB_BIND_U8:
        if((count > 0) && (data != NULL))
        {
            intData = (S32)*((U8 *)data);
        }
        else
        {
            intData = IPOD_PLAYER_IAP2_DB_MEDIA_ITEM_EMPTY_INT;
        }
        ret = sqlite3_bind_int(stmt, pos, intData);
        break;
        
    /* bind is U16  */
    case IPP_IAP2_DB_BIND_U16:
        if((count > 0) && (data != NULL))
        {
            intData = (S32)*((U16 *)data);
        }
        else
        {
            intData = IPOD_PLAYER_IAP2_DB_MEDIA_ITEM_EMPTY_INT;
        }
        ret = sqlite3_bind_int(stmt, pos, intData);
        break;
        
    /* bind is U32  */
    case IPP_IAP2_DB_BIND_U32:
        if((count > 0) && (data != NULL))
        {
            intData = (S32)*((U32 *)data);
        }
        else
        {
            intData = IPOD_PLAYER_IAP2_DB_MEDIA_ITEM_EMPTY_INT;
        }
        ret = sqlite3_bind_int(stmt, pos, intData);
        break;
        
    /* bind is long */
    case IPP_IAP2_DB_BIND_U64:
        if((count > 0) && (data != NULL))
        {
            longData = (S64)*((U64 *)data);
        }
        else
        {
            longData = IPOD_PLAYER_IAP2_DB_MEDIA_ITEM_EMPTY_INT;
        }
        ret = sqlite3_bind_int64(stmt, pos, longData);
        break;
        
    /* bind is string */
    case IPP_IAP2_DB_BIND_STR:
        if((count > 0) && (data != NULL))
        {
            if(((U8 **)data)[0] != NULL)
            {
                strData = ((U8 **)data)[0];
            }
            else
            {
                strData = (U8 *)IPOD_PLAYER_IAP2_DB_MEDIA_ITEM_EMPTY_TXT;
            }
        }
        else
        {
            strData = (U8 *)IPOD_PLAYER_IAP2_DB_MEDIA_ITEM_EMPTY_TXT;
        }
        ret = sqlite3_bind_text(stmt, pos, (const char*)strData, -1, NULL);
        break;
        
    /* bind is unknown */
    default:
        break;
    }
    
    return ret;
}

static int ippiAP2DBGetCountCallback( void *arg, int argc, char **argv, char **column )
{
    U32 *count = NULL;
    
    /* Parameter check */
    if((arg == NULL) || (argc != 1) || (argv == NULL) || (column == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, arg, argc, argv, column);
        return SQLITE_ERROR;
    }
    
    /* For compiler warnning*/
    argc = argc;
    column = column;
    
    count = (U32 *)arg;
    
    if(argv[0] != NULL)
    {
        /* Count should be gotten only once and only one column */
        *count = atoi(argv[0]);
    }
    
    return SQLITE_OK;
}

static int ippiAP2DBGetIntegerCallback( void *arg, int argc, char **argv, char **column )
{
    S32 *count = NULL;
    
    /* Parameter check */
    if((arg == NULL) || (argc != 1) || (argv == NULL) || (column == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, arg, argc, argv, column);
        return SQLITE_ERROR;
    }
    
    /* For compiler warnning*/
    argc = argc;
    column = column;
    
    count = (S32 *)arg;
    
    if(argv[0] != NULL)
    {
        /* Count should be gotten only once and only one column */
        *count = atoi(argv[0]);
    }
    
    return SQLITE_OK;
}

static int ippiAP2DBGetStringCallback( void *arg, int argc, char **argv, char **column )
{
    /* Parameter check */
    if((arg == NULL) || (argc != 1) || (argv == NULL) || (argv[0] == NULL) || (column == NULL))
    {
        //IPOD_DLT_WARN("[DBG]Failed to get character string from DB.:arg=%p, argc=%d, argv=%p, argv[0]=%p, column=%p" ,arg, argc, argv, argv[0], column);
        return SQLITE_ERROR;
    }
    
    /* Copy selected string to arg */
    strncpy((char *)arg, (const char *)argv[0], IPOD_PLAYER_IAP2_DB_STRING_MAX_LEN);
    
    return SQLITE_OK;
}

static int ippiAP2DBPlaybackStatusCallback(void *arg, int cnt, char ** argv, char **column)
{
    int ret = -1;
    IPOD_PLAYER_PLAYBACK_STATUS *status = NULL;
    U32 i = 0;
    U32 statusActiveMask = 0;
    
    /* Parameter check */
    if((arg == NULL) || (argv == NULL) || (column == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, arg, cnt, argv, column);
        return SQLITE_ERROR;
    }
    
    status = (IPOD_PLAYER_PLAYBACK_STATUS *)arg;
    statusActiveMask = status->playbackStatusActiveMask;
    
    for(i = 0; i < (U32)cnt; i++)
    {
        if((argv[i] != NULL) && (column[i] != NULL))
        {
            /* Set playback status if column is "PlaybackStatus" */
            if(strncmp((const char*)IPOD_PLAYER_IAP2_DB_COLUMN_PLAYBACKSTATUS, column[i],
                       sizeof(IPOD_PLAYER_IAP2_DB_COLUMN_PLAYBACKSTATUS)) == 0)
            {
                if(statusActiveMask & IPOD_PLAYER_PLAY_ACT_STATUS)
                {
                    status->status = (IPOD_PLAYER_PLAY_STATUS)atoi(argv[i]);
                }
            }
            /* Set track index if column is "TrackIndex" */
            else if(strncmp((const char*)IPOD_PLAYER_IAP2_DB_COLUMN_INDEX, column[i],
                       sizeof(IPOD_PLAYER_IAP2_DB_COLUMN_INDEX)) == 0)
            {
                if(statusActiveMask & IPOD_PLAYER_PLAY_ACT_TRACK_INDEX)
                {
                    status->track.index = atoi(argv[i]);
                }
            }
            /* Set chapter index if column is "ChapterIndex" */
            else if(strncmp((const char*)IPOD_PLAYER_IAP2_DB_COLUMN_CHAPTERINDEX, column[i],
                       sizeof(IPOD_PLAYER_IAP2_DB_COLUMN_CHAPTERINDEX)) == 0)
            {
                if(statusActiveMask & IPOD_PLAYER_PLAY_ACT_CHAPTER_INDEX)
                {
                    status->chapter.index = atoi(argv[i]);
                }
            }
            /* Set elapsed time if column is "ElapsedTime" */
            else if(strncmp((const char*)IPOD_PLAYER_IAP2_DB_COLUMN_ELAPSEDTIME, column[i],
                       sizeof(IPOD_PLAYER_IAP2_DB_COLUMN_ELAPSEDTIME)) == 0)
            {
                if(statusActiveMask & IPOD_PLAYER_PLAY_ACT_TIME)
                {
                    status->track.time = atoi(argv[i]);
                }
            }
            /* Set app name if column is "AppName" */
            else if(strncmp((const char*)IPOD_PLAYER_IAP2_DB_COLUMN_APPNAME, column[i],
                       sizeof(IPOD_PLAYER_IAP2_DB_COLUMN_APPNAME)) == 0)
            {
                if(statusActiveMask & IPOD_PLAYER_PLAY_ACT_APP_NAME)
                {
                    strncpy((char *)status->appName, (const char*)argv[i], sizeof(status->appName));
                    status->appName[sizeof(status->appName) - 1] = '\0';
                }
            }
            /* Set app bundle id if column is "AppBundleID" */
            else if(strncmp((const char*)IPOD_PLAYER_IAP2_DB_COLUMN_APPBUNDLEID, column[i],
                       sizeof(IPOD_PLAYER_IAP2_DB_COLUMN_APPBUNDLEID)) == 0)
            {
                if(statusActiveMask & IPOD_PLAYER_PLAY_ACT_APP_BUNDLE_ID)
                {
                    strncpy((char *)status->appBundleID, (const char*)argv[i], sizeof(status->appBundleID));
                    status->appBundleID[sizeof(status->appBundleID) - 1] = '\0';
                }
            }
            /* Set Media Library UID if column is "MediaLibraryUID" */
            else if(strncmp((const char*)IPOD_PLAYER_IAP2_DB_COLUMN_MEDIALIBRARYUID, column[i],
                       sizeof(IPOD_PLAYER_IAP2_DB_COLUMN_MEDIALIBRARYUID)) == 0)
            {
                if(statusActiveMask & IPOD_PLAYER_PLAY_ACT_MEDIA_LIBRARY_UID)
                {
                    strncpy((char *)status->mediaLibraryUID, (const char*)argv[i], sizeof(status->mediaLibraryUID));
                    status->mediaLibraryUID[sizeof(status->mediaLibraryUID) - 1] = '\0';
                }
            }
            /* Set Apple Music Radio station name if column is "RadioStationName" */
            else if(strncmp((const char*)IPOD_PLAYER_IAP2_DB_COLUMN_RADIOSTATIONNAME, column[i],
                       sizeof(IPOD_PLAYER_IAP2_DB_COLUMN_RADIOSTATIONNAME)) == 0)
            {
                if(statusActiveMask & IPOD_PLAYER_PLAY_ACT_AMR_STATION_NAME)
                {
                    strncpy((char *)status->AmrStationName, (const char*)argv[i], sizeof(status->AmrStationName));
                    status->AmrStationName[sizeof(status->AmrStationName) - 1] = '\0';
                }
            }
            /* Set playback count if column is "QueueCount" */
            else if(strncmp((const char*)IPOD_PLAYER_IAP2_DB_COLUMN_QUEUECOUNT, column[i],
                       sizeof(IPOD_PLAYER_IAP2_DB_COLUMN_QUEUECOUNT)) == 0)
            {
                if(statusActiveMask & IPOD_PLAYER_PLAY_ACT_QUEUE_COUNT)
                {
                    status->queueCount = atoi(argv[i]);
                }
            }
            /* Set playback queue list avail if column is "QueueListAvail" */
            else if(strncmp((const char*)IPOD_PLAYER_IAP2_DB_COLUMN_QUEUELISTAVAIL, column[i],
                       sizeof(IPOD_PLAYER_IAP2_DB_COLUMN_QUEUELISTAVAIL)) == 0)
            {
                if(statusActiveMask & IPOD_PLAYER_PLAY_ACT_QUEUE_LIST_AVAIL)
                {
                    status->queueListAvail = atoi(argv[i]);
                }
            }

            else
            {
                /* Nothing to set */
            }
        }
    }
    
    ret = SQLITE_OK;
    
    return ret;
}

static int ippiAP2DBGetMediaLibraryInformationCB(void *arg, int cnt, char ** argv, char **column)
{
    int ret = -1;
    IPOD_PLAYER_IAP2_DB_MEDIAINFO *mediaInfo = NULL;
    U32 i = 0;
    
    /* Parameter check */
    if((arg == NULL) || (argv == NULL) || (column == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, arg, cnt, argv, column);
        return SQLITE_ERROR;
    }
    
    mediaInfo = (IPOD_PLAYER_IAP2_DB_MEDIAINFO *)arg;
    
    for(i = 0; i < (U32)cnt; i++)
    {
        if(argv[i] != NULL)
        {
            /* Compare with "DeviceID" */
            if(strncmp((const char *)IPOD_PLAYER_IAP2_DB_MEDIAINFO_COLUMN_DEVICEID, (const char*)column[i],
                       strlen((const char*)IPOD_PLAYER_IAP2_DB_MEDIAINFO_COLUMN_DEVICEID)) == 0)
            {
                /* Nothing do */
            }
            /* Compare with "MediaID" */
            else if(strncmp((const char *)IPOD_PLAYER_IAP2_DB_MEDIAINFO_COLUMN_MEDIAID, (const char*)column[i],
                            strlen((const char*)IPOD_PLAYER_IAP2_DB_MEDIAINFO_COLUMN_MEDIAID)) == 0)
            {
                strncpy((char *)mediaInfo->mediaID, (const char*)argv[i], sizeof(mediaInfo->mediaID));
                mediaInfo->mediaID[sizeof(mediaInfo->mediaID) - 1] = '\0';
            }
            /* Compare with "MediaName" */
            else if(strncmp((const char *)IPOD_PLAYER_IAP2_DB_MEDIAINFO_COLUMN_MEDIANAME, (const char*)column[i],
                            strlen((const char*)IPOD_PLAYER_IAP2_DB_MEDIAINFO_COLUMN_MEDIANAME)) == 0)
            {
                strncpy((char *)mediaInfo->mediaName, (const char*)argv[i], sizeof(mediaInfo->mediaName));
                mediaInfo->mediaName[sizeof(mediaInfo->mediaName) - 1] = '\0';
            }
            /* Compare with "MediaType" */
            else if(strncmp((const char *)IPOD_PLAYER_IAP2_DB_MEDIAINFO_COLUMN_MEDIATYPE, (const char*)column[i],
                            strlen((const char*)IPOD_PLAYER_IAP2_DB_MEDIAINFO_COLUMN_MEDIATYPE)) == 0)
            {
                mediaInfo->mediaType = atoi(argv[i]);
            }
            /* Compare with "Revision" */
            else if(strncmp((const char *)IPOD_PLAYER_IAP2_DB_MEDIAINFO_COLUMN_REVISION, (const char*)column[i],
                            strlen((const char*)IPOD_PLAYER_IAP2_DB_MEDIAINFO_COLUMN_REVISION)) == 0)
            {
                strncpy((char *)mediaInfo->mediaRevision, (const char*)argv[i], sizeof(mediaInfo->mediaRevision));
                mediaInfo->mediaRevision[sizeof(mediaInfo->mediaRevision) - 1] = '\0';
            }
            else
            {
            }
        }
    }
    
    ret = SQLITE_OK;
    
    return ret;
}

static int ippiAP2DBGetCatListCB(void *arg, int cnt, char ** argv, char **column)
{
    int ret = -1;
    IPOD_PLAYER_IAP2_DB_CATLIST *catList = NULL;
    U32 i = 0;
    U32 catIndex = 0;
    U8 *category = NULL;
    U64 catValue = 0;
    
    if((arg == NULL) || (argv == NULL) || (column == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, arg, cnt, argv, column);
        return SQLITE_ERROR;
    }
    
    catList = (IPOD_PLAYER_IAP2_DB_CATLIST *)arg;
    
    for(i = 0; i < (U32)cnt; i++)
    {
        if(argv[i] != NULL)
        {
            /* Column is Index */
            if(strncmp((const char *)IPOD_PLAYER_IAP2_DB_COLUMN_CAT_INDEX, (const char*)column[i],
                       strlen((const char*)IPOD_PLAYER_IAP2_DB_COLUMN_CAT_INDEX)) == 0)
            {
                catIndex = atoi(argv[i]);
                if(catIndex > 0)
                {
                    catIndex--;
                }
            }
            /* Column is Category */
            else if(strncmp((const char *)IPOD_PLAYER_IAP2_DB_COLUMN_CAT_CATEGORY, (const char*)column[i],
                            strlen((const char*)IPOD_PLAYER_IAP2_DB_COLUMN_CAT_CATEGORY)) == 0)
            {
                category = (U8 *)argv[i];
            }
            /* Colujmn is Value */
            else if(strncmp((const char *)IPOD_PLAYER_IAP2_DB_COLUMN_CAT_VALUE, (const char*)column[i],
                            strlen((const char*)IPOD_PLAYER_IAP2_DB_COLUMN_CAT_VALUE)) == 0)
            {
                catValue = atoll((const char*)argv[i]);
            }
        }
    }
    
    if((category != NULL) && (catIndex < catList->count))
    {
        strncpy((char *)catList->categories[catIndex].category, (const char *)category, sizeof(catList->categories[catIndex].category));
        catList->categories[catIndex].category[sizeof(catList->categories[catIndex].category) - 1] = '\0';
        catList->categories[catIndex].catID = catValue;
    }
    
    ret = SQLITE_OK;
    
    return ret;
}

static int ippiAP2DBGetDBEntriesCB(void *arg, int cnt, char ** argv, char **column)
{
    int ret = 0;
    U32 i = 0;
    IPOD_PLAYER_ENTRY_LIST *entryList = NULL;
    IPOD_PLAYER_ENTRY_LIST_INT  *entryListInt; 
    
    
    /* Parameter check */
    if((arg == NULL) || (argv == NULL) || (column == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, arg, cnt, argv, column);
        return SQLITE_ERROR;
    }

    entryListInt = (IPOD_PLAYER_ENTRY_LIST_INT *)arg;
    entryList = (IPOD_PLAYER_ENTRY_LIST *)entryListInt->entryList;
    for(i = 0; i < (U32)cnt; i++)
    {
        if((i == 1) && (argv[0] != NULL))
        {
            if(entryListInt->setCnt < IPOD_PLAYER_ENTRIES_ARRYA_MAX)
            {
                strncpy((char *)entryList[entryListInt->setCnt].name, (const char*)argv[i], sizeof(entryList[entryListInt->setCnt].name));
                entryList[entryListInt->setCnt].name[sizeof(entryList[entryListInt->setCnt].name) - 1] = '\0';
                entryListInt->setCnt++;
            }
        }
    }
    
    return ret;
}

static int ippiAP2DBGetCategoryIDCB(void *arg, int cnt, char ** argv, char **column)
{
    int ret = 0;
    U32 i = 0;
    U64 *id = NULL;
    
    if((arg == NULL) || (argv == NULL) || (column == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, arg, cnt, argv, column);
        return SQLITE_ERROR;
    }
    
    id = (U64 *)arg;
    for(i = 0; i < (U32)cnt; i++)
    {
        if((i == 0) && (argv[i] != NULL))
        {
            *id = atoll((const char *)argv[i]);
        }
    }
    
    return ret;
}

static int ippiAP2DBGetMediaItemIDCB(void *arg, int cnt, char ** argv, char **column)
{
    int ret = 0;
    U32 i = 0;
    U32 idIndex = 0;
    
    IPOD_PLAYER_IAP2_DB_IDLIST *idList = NULL;
    
    cnt =cnt;
    if((arg == NULL) || (argv == NULL) || (column == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, arg, cnt, argv, column);
        return SQLITE_ERROR;
    }
    
    idList = (IPOD_PLAYER_IAP2_DB_IDLIST *)arg;
    
    for(i = 0; i < idList->count; i++)
    {
        if(idList->mediaId[i] == 0)
        {
            idIndex = i;
            break;
        }
    }
    
    if((idIndex < idList->count) && (argv[0] != NULL))
    {
        idList->mediaId[idIndex] = atoll((const char*)argv[0]);
    }
    
    return ret;
}

static int ippiAP2DBGetMediaLibraryIDCB( void *arg, int argc, char **argv, char **column )
{
    PUniqueId_t uid = NULL;
    
    /* Parameter check */
    if((arg == NULL) || (argv == NULL) || (column == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, arg, argc, argv, column);
        return SQLITE_ERROR;
    }
    
    column = column;
    argc = argc;
    uid = (PUniqueId_t)arg;
    if(argv[0] != NULL)
    {
        strncpy((char *)uid->id, (const char *)argv[0], uid->len);
    }
    else
    {
        IPOD_DLT_WARN("Uid is null.");
        return SQLITE_ERROR;
    }
    
    return SQLITE_OK;
}

static int ippiAP2DBGetStringCB( void *arg, int argc, char **argv, char **column )
{
    ippiAP2DBString_t *str = NULL;
    
    /* Parameter check */
    if((arg == NULL) || (argv == NULL) || (column == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, arg, argc, argv, column);
        return SQLITE_ERROR;
    }
    
    column = column;
    
    if((argc > 0) && (argv[0] != NULL))
    {
        str = (ippiAP2DBString_t *)arg;
        strncpy((char *)str->str, (const char *)argv[0], str->len);
    }
    
    return SQLITE_OK;
}

static S32 ippiAP2DBCopyDB(sqlite3 *destHandle, sqlite3 *srcHandle)
{
    S32 rc = IPOD_PLAYER_ERROR;
    int ret = -1;
    sqlite3_backup *backupHandle = NULL;

    /* Parameter check */
    if((destHandle == NULL) || (srcHandle == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, destHandle, srcHandle);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    IPOD_DLT_INFO("destHandle=%p, srcHandle=%p", destHandle, srcHandle);

    /* Initialize the backup handle */
    backupHandle = sqlite3_backup_init(destHandle, IPOD_PLAYER_IAP2_DB_MAIN, srcHandle, IPOD_PLAYER_IAP2_DB_MAIN);
    if(backupHandle != NULL)
    {
        /* All database pages will be backed up */
        ret = sqlite3_backup_step(backupHandle, IPOD_PLAYER_IAP2_DB_BACKUP_ALL_PAGES);
        if(ret == SQLITE_DONE)
        {
            rc = IPOD_PLAYER_OK;
        }
        else
        {
            IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, ret);
            rc = IPOD_PLAYER_ERR_DB_BACKUP;
        }
        
        sqlite3_backup_finish(backupHandle);
    }
    else
    {
        rc = IPOD_PLAYER_ERR_DB_BACKUP;
    }
    
    return rc;
}

static S32 ippiAP2DBMemoryDBInit(sqlite3 *handle, const U8 *deviceID)
{
    S32 rc = IPOD_PLAYER_ERROR;
    int ret = -1;
    U8 statement[IPOD_PLAYER_IAP2_DB_STATEMENT_MAX] = {0};
    U32 count = 0;
    
    /* Parameter check */
    if((handle == NULL) || (deviceID == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, handle, deviceID);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    IPOD_DLT_VERBOSE("handle=%p, deviceID=%s", handle, deviceID);

    /* Check whether MediaItem table has been already created. */
    ret = sqlite3_exec(handle, IPOD_PLAYER_IAP2_DB_QUERY_TABLE_COUNT, ippiAP2DBGetCountCallback, &count, NULL);
    if(ret == SQLITE_OK)
    {
        /* Count is 0 means that new database was created */
        if(count == 0)
        {
            /* Create mediainfo table */
            ret = sqlite3_exec(handle, IPOD_PLAYER_IAP2_DB_CREATE_MEDIAINFO_TABLE, NULL, NULL, NULL);
            if(ret == SQLITE_OK)
            {
                snprintf((char *)statement, sizeof(statement), (const char*)IPOD_PLAYER_IAP2_DB_INSERT_DEVICEID, deviceID);
                ret = sqlite3_exec(handle, (const char*)statement, NULL, NULL, NULL);
                if(ret == SQLITE_OK)
                {
                    /* Create the new table */
                    ret = sqlite3_exec(handle, IPOD_PLAYER_IAP2_DB_CREATE_MEDIAITEM_TABLE, NULL, NULL, NULL);
                    if(ret == SQLITE_OK)
                    {
                        /* Create index for track title */
                        ret = sqlite3_exec(handle, IPOD_PLAYER_IAP2_DB_CREATE_TRACKTITLE_INDEX, NULL, NULL, NULL);
                        if(ret == SQLITE_OK)
                        {
                            /* Create index for Album title */
                            ret = sqlite3_exec(handle, IPOD_PLAYER_IAP2_DB_CREATE_ALBUMTITLE_INDEX, NULL, NULL, NULL);
                            if(ret == SQLITE_OK)
                            {
                                /* Create index for Artist title */
                                ret = sqlite3_exec(handle, IPOD_PLAYER_IAP2_DB_CREATE_ARTISTTITLE_INDEX, NULL, NULL, NULL);
                                if(ret == SQLITE_OK)
                                {
                                    /* Create index for Artist title */
                                    ret = sqlite3_exec(handle, IPOD_PLAYER_IAP2_DB_CREATE_PLAYLIST_TABLE, NULL, NULL, NULL);
                                    if(ret == SQLITE_OK)
                                    {
                                        /* Create index for Artist title */
                                        ret = sqlite3_exec(handle, IPOD_PLAYER_IAP2_DB_CREATE_PLAYLIST_TRACKS_TABLE, NULL, NULL, NULL);
                                        if(ret == SQLITE_OK)
                                        {
                                            rc = IPOD_PLAYER_OK;
                                        }
                                        else
                                        {
                                            IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, ret, handle);
                                            rc = IPOD_PLAYER_ERR_DB_OPEN;
                                        }
                                    }
                                    else
                                    {
                                        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, ret, handle);
                                        rc = IPOD_PLAYER_ERR_DB_OPEN;
                                    }
                                }
                                else
                                {
                                    IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, ret, handle);
                                    rc = IPOD_PLAYER_ERR_DB_OPEN;
                                }
                            }
                            else
                            {
                                IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, ret, handle);
                                rc = IPOD_PLAYER_ERR_DB_OPEN;
                            }
                        }
                        else
                        {
                            IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, ret, handle);
                            rc = IPOD_PLAYER_ERR_DB_OPEN;
                        }
                    }
                    else
                    {
                        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, ret, handle);
                        rc = IPOD_PLAYER_ERR_DB_OPEN;
                    }
                }
                else
                {
                    IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, ret, handle);
                    rc = IPOD_PLAYER_ERR_DB_OPEN;
                }
            }
            else
            {
                IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, ret, handle);
                rc = IPOD_PLAYER_ERR_DB_OPEN;
            }
        }
        else
        {
            /* Count is not 0 means that datase has been already created before */
            rc = IPOD_PLAYER_OK;
        }
    }
    else
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, ret, handle);
        rc = IPOD_PLAYER_ERR_DB_OPEN;
    }
    
    return rc;
}

static S32 ippiAP2DBiPodInfoDBInit(sqlite3 *handle)
{
    S32 rc = IPOD_PLAYER_ERROR;
    int ret = -1;
    
    /* Parameter check */
    if(handle == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, handle);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }

    IPOD_DLT_VERBOSE("handle=%p", handle);

    /* Create table for iPodInfo */
    ret = sqlite3_exec(handle, IPOD_PLAYER_IAP2_DB_CREATE_IPODINFO_TABLE, NULL, NULL, NULL);
    if(ret == SQLITE_OK)
    {
        /* Create table for NowPlaying list */
        ret = sqlite3_exec(handle, IPOD_PLAYER_IAP2_DB_CREATE_NOWPLAYING_TABLE, NULL, NULL, NULL);
        if(ret == SQLITE_OK)
        {
            /* Create table for connection status */
            ret = sqlite3_exec(handle, IPOD_PLAYER_IAP2_DB_CREATE_CONNECTIONSTATUS_TABLE, NULL, NULL, NULL);
            if(ret == SQLITE_OK)
            {
                /* Create table for current playing track information */
                ret = sqlite3_exec(handle, IPOD_PLAYER_IAP2_DB_CREATE_PLAYING_ITEM_TABLE, NULL, NULL, NULL);
                if(ret == SQLITE_OK)
                {
                    /* Create table for current selecting category */
                    ret = sqlite3_exec(handle, IPOD_PLAYER_IAP2_DB_CREATE_CATEGORY_TABLE, NULL, NULL, NULL);
                    if(ret == SQLITE_OK)
                    {
                        /* Create table for bluetooth status */
                        ret = sqlite3_exec(handle, IPOD_PLAYER_IAP2_DB_CREATE_BT_STATUS_TABLE, NULL, NULL, NULL);
                        if(ret == SQLITE_OK)
                        {
                            ret = sqlite3_exec(handle, IPOD_PLAYER_IAP2_DB_CREATE_ASSISTIVE_TABLE, NULL, NULL, NULL);
                            if(ret == SQLITE_OK)
                            {
                                rc = IPOD_PLAYER_OK;
                            }
                            else
                            {
                                IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, ret, handle);
                                rc = IPOD_PLAYER_ERR_DB_OPEN;
                            }
                        }
                        else
                        {
                            IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, ret, handle);
                            rc = IPOD_PLAYER_ERR_DB_OPEN;
                        }
                    }
                    else
                    {
                        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, ret, handle);
                        rc = IPOD_PLAYER_ERR_DB_OPEN;
                    }
                }
                else
                {
                    IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, ret, handle);
                    rc = IPOD_PLAYER_ERR_DB_OPEN;
                }
            }
            else
            {
                IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, ret, handle);
                rc = IPOD_PLAYER_ERR_DB_OPEN;
            }
        }
        else
        {
            IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, ret, handle);
            rc = IPOD_PLAYER_ERR_DB_OPEN;
        }
    }
    else
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, ret, handle);
        rc = IPOD_PLAYER_ERR_DB_OPEN;
    }
    
    return rc;
}

static S32 ippiAP2CheckCorrectDB(sqlite3 *handle, const U8 *deviceID)
{
    S32 rc = -1;
    U8 mediaID[IPOD_PLAYER_IAP2_DB_MAX_DEVICEID_LEN] = {0};
    
    /* Parameter check */
    if((handle == NULL) || (deviceID == NULL))
    {
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Get the deviceID of current database */
    rc = sqlite3_exec(handle, IPOD_PLAYER_IAP2_DB_QUERY_DEVICEID, ippiAP2DBGetStringCallback, mediaID, NULL);
    
    /* Compare the deviceID and deviceID of database */
    if(strncmp((const char*)mediaID, (const char*)deviceID, sizeof(mediaID)) == 0)
    {
        /* Opened database is for connected Apple device */
        rc = IPOD_PLAYER_OK;
    }
    else
    {
        /* Opened database is not for connected Apple device */
        rc = IPOD_PLAYER_ERROR;
    }
    
    return rc;
}

static sqlite3 *ippiAP2OpenFileDB(const U8 *prefix, const U8 *deviceID, BOOL appleMusicRadio, U8 *dbFileName)
{
    S32 rc = -1;
    U32 maxNum = 0;
    sqlite3 *handle = NULL;
    U8 fileName[IPOD_PLAYER_IAP2_DB_MAX_FILENAME_LEN] = {0};
    U8 oldFile[IPOD_PLAYER_IAP2_DB_MAX_FILENAME_LEN] = {0};
    U32 i = 0;
    S32 empty = -1;
    struct stat stat_buf;
    time_t oldTime = 0;
    
    /* Parameter check */
    if((prefix == NULL) || (deviceID == NULL) || (dbFileName == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, prefix, deviceID);
        return NULL;
    }

    IPOD_DLT_INFO("prefix=%s, deviceID=%s, appleMusicRadio=%d", prefix, deviceID, appleMusicRadio);

    maxNum = iPodCoreGetCfn(IPOD_PLAYER_CFGNUM_MAX_DATABASE_NUM);
    
    for(i = 0; i < maxNum; i++)
    {
        /* Clear fileName and stat buf */
        memset(fileName, 0, sizeof(fileName));
        memset(&stat_buf, 0, sizeof(stat_buf));
        
        /* Set file name */
        if(appleMusicRadio)
        {
            snprintf((char *)fileName, sizeof(fileName), (const char*)IPOD_PLAYER_IAP2_DB_APPLE_MUSIC_RADIO_FILE_NAME, prefix, i);
        }
        else
        {
            snprintf((char *)fileName, sizeof(fileName), (const char*)IPOD_PLAYER_IAP2_DB_LOCAL_DEVICE_FILE_NAME, prefix, i);
        }

        /* Check whether file exsists or not */
        rc = stat((char *)fileName, &stat_buf);
        if(rc == 0)
        {
            /* Check the time which file is updated at last */
            if((oldTime == 0) || (oldTime > stat_buf.st_mtime))
            {
                /* This file is older than previous file */
                oldTime = stat_buf.st_mtime;
                strncpy((char *)oldFile, (const char *)fileName, sizeof(oldFile));
            }
            
            /* Open database */
            rc = sqlite3_open((const char *)fileName, &handle);
            if(rc == SQLITE_OK)
            {
                rc = sqlite3_busy_timeout(handle, IPOD_PLAYER_IAP2_DB_TIMEOUT);
                if(rc == SQLITE_OK)
                {
                    /* Check whehter this database is for connected Apple device or not */
                    rc = ippiAP2CheckCorrectDB(handle, deviceID);
                    if(rc == IPOD_PLAYER_OK)
                    {
                        /* Database file of connected Apple Device was found. */
                        IPOD_DLT_INFO("Matched :fileName=%s", fileName);
                        strncpy((char *)dbFileName, (const char *)fileName, IPOD_PLAYER_IAP2_DB_MAX_FILENAME_LEN);
                        break;
                    }
                    else
                    {
                        /* It is not database file of connected Apple Device. */
                        sqlite3_close(handle);
                        handle = NULL;
                    }
                }
                else
                {
                    sqlite3_close(handle);
                    handle = NULL;
                    IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERROR, rc);
                    rc = IPOD_PLAYER_ERROR;
                }
            }
            else
            {
                rc = IPOD_PLAYER_ERROR;
            }
        }
        else
        {
            /* This file dose not exist */
            if(empty == -1)
            {
                empty = (S32)i;
            }
        }
    }
    
    /* Database for connected Apple device is not found out */
    /* New database will be created */
    if(rc != IPOD_PLAYER_OK)
    {
        /* All file is created */
        if(empty == -1)
        {
            /* Remove old one */
            IPOD_DLT_INFO("Remove :fileName=%s", oldFile);
            remove((const char*)oldFile);
        }
        else
        {
            /* Set empty file name */
            if(appleMusicRadio)
            {
                snprintf((char *)oldFile, sizeof(oldFile), (const char*)IPOD_PLAYER_IAP2_DB_APPLE_MUSIC_RADIO_FILE_NAME, prefix, empty);
            }
            else
            {
                snprintf((char *)oldFile, sizeof(oldFile), (const char*)IPOD_PLAYER_IAP2_DB_LOCAL_DEVICE_FILE_NAME, prefix, empty);
            }
        }
        
        /* Open new database */
        IPOD_DLT_INFO("Create :fileName=%s", oldFile);
        rc = sqlite3_open((char *)oldFile, &handle);
        if(rc == SQLITE_OK)
        {
            rc = sqlite3_busy_timeout(handle, IPOD_PLAYER_IAP2_DB_TIMEOUT);
            if(rc == SQLITE_OK)
            {
                rc = IPOD_PLAYER_OK;
                strncpy((char *)dbFileName, (const char *)oldFile, IPOD_PLAYER_IAP2_DB_MAX_FILENAME_LEN);
            }
            else
            {
                sqlite3_close(handle);
                handle = NULL;
                IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERROR, rc);
                rc = IPOD_PLAYER_ERROR;
            }
        }
        else
        {
            handle = NULL;
            rc = IPOD_PLAYER_ERROR;
        }
    }
    
    return handle;
}

static int ippiAP2DBSetMediaItemStatementBind(sqlite3_stmt *stmt, iAP2MediaItem *item)
{
    int ret = -1;
    U32 i = 1;
    
    /* Parameter check */
    if((stmt == NULL) || (item == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, stmt, item);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Reset statement */
    ret = sqlite3_reset(stmt);
    if(ret == SQLITE_OK)
    {
        ret = ippiAP2DBSetBind(stmt, IPP_IAP2_DB_BIND_U64, item->iAP2MediaItemPersistentIdentifier_count, item->iAP2MediaItemPersistentIdentifier, i);
        i++;
    }
    
    if(ret == SQLITE_OK)
    {
        ret = ippiAP2DBSetBind(stmt, IPP_IAP2_DB_BIND_STR, item->iAP2MediaItemTitle_count, item->iAP2MediaItemTitle, i);
        i++;
    }
    
    if(ret == SQLITE_OK)
    {
        ret = ippiAP2DBSetBind(stmt, IPP_IAP2_DB_BIND_U64, item->iAP2MediaItemAlbumPersistentIdentifier_count, item->iAP2MediaItemAlbumPersistentIdentifier, i);
        i++;
    }
    
    if(ret == SQLITE_OK)
    {
        ret = ippiAP2DBSetBind(stmt, IPP_IAP2_DB_BIND_STR, item->iAP2MediaItemAlbumTitle_count, item->iAP2MediaItemAlbumTitle, i);
        i++;
    }
    
    if(ret == SQLITE_OK)
    {
        ret = ippiAP2DBSetBind(stmt, IPP_IAP2_DB_BIND_U16, item->iAP2MediaItemAlbumDiscCount_count, item->iAP2MediaItemAlbumDiscCount, i);
        i++;
    }
    
    if(ret == SQLITE_OK)
    {
        ret = ippiAP2DBSetBind(stmt, IPP_IAP2_DB_BIND_U16, item->iAP2MediaItemAlbumDiscNumber_count, item->iAP2MediaItemAlbumDiscNumber, i);
        i++;
    }
    
    if(ret == SQLITE_OK)
    {
        ret = ippiAP2DBSetBind(stmt, IPP_IAP2_DB_BIND_U64, item->iAP2MediaItemAlbumArtistPersistentIdentifier_count, item->iAP2MediaItemAlbumArtistPersistentIdentifier, i);
        i++;
    }
    
    if(ret == SQLITE_OK)
    {
        ret = ippiAP2DBSetBind(stmt, IPP_IAP2_DB_BIND_STR, item->iAP2MediaItemAlbumArtist_count, item->iAP2MediaItemAlbumArtist, i);
        i++;
    }
    
    if(ret == SQLITE_OK)
    {
        ret = ippiAP2DBSetBind(stmt, IPP_IAP2_DB_BIND_U16, item->iAP2MediaItemAlbumTrackCount_count, item->iAP2MediaItemAlbumTrackCount, i);
        i++;
    }
    
    if(ret == SQLITE_OK)
    {
        ret = ippiAP2DBSetBind(stmt, IPP_IAP2_DB_BIND_U16, item->iAP2MediaItemAlbumTrackNumber_count, item->iAP2MediaItemAlbumTrackNumber, i);
        i++;
    }
    
    if(ret == SQLITE_OK)
    {
        ret = ippiAP2DBSetBind(stmt, IPP_IAP2_DB_BIND_U64, item->iAP2MediaItemArtistPersistentIdentifier_count, item->iAP2MediaItemArtistPersistentIdentifier, i);
        i++;
    }
    
    if(ret == SQLITE_OK)
    {
        ret = ippiAP2DBSetBind(stmt, IPP_IAP2_DB_BIND_STR, item->iAP2MediaItemArtist_count, item->iAP2MediaItemArtist, i);
        i++;
    }
    
    if(ret == SQLITE_OK)
    {
        ret = ippiAP2DBSetBind(stmt, IPP_IAP2_DB_BIND_U16, item->iAP2MediaItemArtworkFileTransferIdentifier_count, item->iAP2MediaItemArtworkFileTransferIdentifier, i);
        i++;
    }
    
    if(ret == SQLITE_OK)
    {
        ret = ippiAP2DBSetBind(stmt, IPP_IAP2_DB_BIND_U64, item->iAP2MediaItemComposerPersistentIdentifier_count, item->iAP2MediaItemComposerPersistentIdentifier, i);
        i++;
    }
    
    if(ret == SQLITE_OK)
    {
        ret = ippiAP2DBSetBind(stmt, IPP_IAP2_DB_BIND_STR, item->iAP2MediaItemComposer_count, item->iAP2MediaItemComposer, i);
        i++;
    }
    
    if(ret == SQLITE_OK)
    {
        ret = ippiAP2DBSetBind(stmt, IPP_IAP2_DB_BIND_U64, item->iAP2MediaItemGenrePersistentIdentifier_count, item->iAP2MediaItemGenrePersistentIdentifier, i);
        i++;
    }
    
    if(ret == SQLITE_OK)
    {
        ret = ippiAP2DBSetBind(stmt, IPP_IAP2_DB_BIND_STR, item->iAP2MediaItemGenre_count, item->iAP2MediaItemGenre, i);
        i++;
    }
    
    if(ret == SQLITE_OK)
    {
        ret = ippiAP2DBSetBind(stmt, IPP_IAP2_DB_BIND_U8, item->iAP2MediaItemIsBanSupported_count, item->iAP2MediaItemIsBanSupported, i);
        i++;
    }
    
    if(ret == SQLITE_OK)
    {
        ret = ippiAP2DBSetBind(stmt, IPP_IAP2_DB_BIND_U8, item->iAP2MediaItemIsBanned_count, item->iAP2MediaItemIsBanned, i);
        i++;
    }
    
    if(ret == SQLITE_OK)
    {
        ret = ippiAP2DBSetBind(stmt, IPP_IAP2_DB_BIND_U8, item->iAP2MediaItemIsLikeSupported_count, item->iAP2MediaItemIsLikeSupported, i);
        i++;
    }
    
    if(ret == SQLITE_OK)
    {
        ret = ippiAP2DBSetBind(stmt, IPP_IAP2_DB_BIND_U8, item->iAP2MediaItemIsLiked_count, item->iAP2MediaItemIsLiked, i);
        i++;
    }
    
    if(ret == SQLITE_OK)
    {
        ret = ippiAP2DBSetBind(stmt, IPP_IAP2_DB_BIND_U8, item->iAP2MediaItemIsResidentOnDevice_count, item->iAP2MediaItemIsResidentOnDevice, i);
        i++;
    }
    
    if(ret == SQLITE_OK)
    {
        ret = ippiAP2DBSetBind(stmt, IPP_IAP2_DB_BIND_U8, item->iAP2MediaItemIsPartOfCompilation_count, item->iAP2MediaItemIsPartOfCompilation, i);
        i++;
    }
    
    if(ret == SQLITE_OK)
    {
        ret = ippiAP2DBSetBind(stmt, IPP_IAP2_DB_BIND_U32, item->iAP2MediaItemMediaType_count, item->iAP2MediaItemMediaType, i);
        i++;
    }
    
    if(ret == SQLITE_OK)
    {
        ret = ippiAP2DBSetBind(stmt, IPP_IAP2_DB_BIND_U8, item->iAP2MediaItemRating_count, item->iAP2MediaItemRating, i);
        i++;
    }
    
    if(ret == SQLITE_OK)
    {
        ret = ippiAP2DBSetBind(stmt, IPP_IAP2_DB_BIND_U32, item->iAP2MediaItemPlaybackDurationInMilliseconds_count, item->iAP2MediaItemPlaybackDurationInMilliseconds, i);
        i++;
    }
    
    if(ret == SQLITE_OK)
    {
        ret = ippiAP2DBSetBind(stmt, IPP_IAP2_DB_BIND_U16, item->iAP2MediaItemChapterCount_count, item->iAP2MediaItemChapterCount, i);
        i++;
    }
    
    return ret;
}

static S32 ippiAP2SetNowPlayingInitDB(IPOD_PLAYER_IAP2_DB_HANDLE *dbHandle)
{
    S32 rc = IPOD_PLAYER_ERROR;
    int ret = -1;
    sqlite3 *handle = NULL;
    sqlite3_stmt *stmt = NULL;
    iAP2MediaItem item;
    
    /* Parameter check */
    if(dbHandle == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, dbHandle);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    memset(&item, 0, sizeof(item));
    handle = dbHandle->iPodHandle;
    /* Compile the sql statement to byte code */
    ret = sqlite3_prepare(handle, IPOD_PLAYER_IAP2_DB_PLAYING_ITEM_PREPARE, strlen(IPOD_PLAYER_IAP2_DB_PLAYING_ITEM_PREPARE), &stmt, NULL);
    if((ret == SQLITE_OK) && (stmt != NULL))
    {
        ret = sqlite3_exec(handle, IPOD_PLAYER_IAP2_DB_TRANSACTION_BEGIN, NULL, NULL, NULL);
        if(ret == SQLITE_OK)
        {
            ret = ippiAP2DBSetMediaItemStatementBind(stmt, &item);
            if(ret == SQLITE_OK)
            {
                ret = sqlite3_step(stmt);
                if(ret == SQLITE_DONE)
                {
                    rc = IPOD_PLAYER_OK;
                }
                else
                {
                    IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, ret, stmt);
                    /* Todo new error */
                    rc = IPOD_PLAYER_ERROR;
                }
            }
            else
            {
                IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, ret, stmt);
                /* Todo new error */
                rc = IPOD_PLAYER_ERROR;
            }
        }
        else
        {
            IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, ret, stmt);
            /* Todo new error */
            rc = IPOD_PLAYER_ERROR;
        }
        
        /* Todo */
        ret = sqlite3_exec(handle, IPOD_PLAYER_IAP2_DB_TRANSACTION_COMMIT, NULL, NULL, NULL);
    }
    else
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, ret, stmt);
        /* Todo new error */
        rc = IPOD_PLAYER_ERROR;
    }
    
    if(stmt != NULL)
    {
        /* Free the prepared statement */
        sqlite3_finalize(stmt);
    }
    
    return rc;
}

/* Create the Database */
void* ippiAP2CreateDB(const U8 *name, const U8 *deviceID, BOOL bluetoothConnection)
{
    S32 rc = IPOD_PLAYER_ERROR;
    int ret = -1;
    IPOD_PLAYER_IAP2_DB_HANDLE *dbHandle = NULL;

    /* Parameter check */
    if((name == NULL) || (deviceID == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, name, deviceID);
        return NULL;
    }
    
    /* Allocate database handles */
    dbHandle = calloc(1, sizeof(*dbHandle));
    if(dbHandle == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_NOMEM);
        return NULL;
    }

    /* Open Local Device media library database */
    /* Open database with file */
    if(!bluetoothConnection)
    {
        dbHandle->localDevice.fileHandle = ippiAP2OpenFileDB(name, deviceID, FALSE, (U8*)&dbHandle->localDevice.fileName);
        if(dbHandle->localDevice.fileHandle != NULL)
        {
            /* Open database with memory */
            ret = sqlite3_open((const char *)IPOD_PLAYER_IAP2_IN_MEMORY_DB, &dbHandle->localDevice.memoryHandle);
            if(ret == SQLITE_OK)
            {
                rc = IPOD_PLAYER_OK;
            }
            else
            {
                IPOD_DLT_ERROR("Local Device memoryHandle open failed. :rc=%d, memoryHandle=%p", rc, dbHandle->localDevice.memoryHandle);
                rc = IPOD_PLAYER_ERR_DB_OPEN;
            }
        }
        else
        {
            IPOD_DLT_ERROR("Local Device fileHandle open failed. :rc=%d", rc);
            rc = IPOD_PLAYER_ERR_DB_OPEN;
        }

        if(rc == IPOD_PLAYER_OK)
        {
            /* Copy the backup data to memory based database */
            rc = ippiAP2DBCopyDB(dbHandle->localDevice.memoryHandle, dbHandle->localDevice.fileHandle);
            if(rc == IPOD_PLAYER_OK)
            {
                /* Initialize the database of memory database */
                rc = ippiAP2DBMemoryDBInit(dbHandle->localDevice.memoryHandle, deviceID);
                if(rc != IPOD_PLAYER_OK)
                {
                    IPOD_DLT_ERROR("Local Device memoryHandle initilize failed. :rc=%d", rc);
                }
            }
            else
            {
                IPOD_DLT_ERROR("Local Device database copy failed. :rc=%d", rc);
            }
        }
    }
    else
    {
        dbHandle->localDevice.fileHandle = NULL;
        /* Open database with memory */
        ret = sqlite3_open((const char *)IPOD_PLAYER_IAP2_IN_MEMORY_DB, &dbHandle->localDevice.memoryHandle);
        if(ret == SQLITE_OK)
        {
            rc = IPOD_PLAYER_OK;
        }
        else
        {
            IPOD_DLT_ERROR("Local Device memoryHandle open failed. :rc=%d, memoryHandle=%p", rc, dbHandle->localDevice.memoryHandle);
            rc = IPOD_PLAYER_ERR_DB_OPEN;
        }

        if(rc == IPOD_PLAYER_OK)
        {
            /* Initialize the database of memory database */
            rc = ippiAP2DBMemoryDBInit(dbHandle->localDevice.memoryHandle, deviceID);
            if(rc != IPOD_PLAYER_OK)
            {
                IPOD_DLT_ERROR("Local Device memoryHandle initilize failed. :rc=%d", rc);
            }
        }
    }

    /* Open Apple Music Radio media library database */
    /* Open database with file */
    if(!bluetoothConnection)
    {
        dbHandle->appleMusicRadio.fileHandle = ippiAP2OpenFileDB(name, deviceID, TRUE, (U8*)&dbHandle->appleMusicRadio.fileName);
        if(dbHandle->appleMusicRadio.fileHandle != NULL)
        {
            /* Open database with memory */
            ret = sqlite3_open((const char *)IPOD_PLAYER_IAP2_IN_MEMORY_DB, &dbHandle->appleMusicRadio.memoryHandle);
            if(ret == SQLITE_OK)
            {
                rc = IPOD_PLAYER_OK;
            }
            else
            {
                IPOD_DLT_ERROR("Apple Music Radio memoryHandle open failed. :rc=%d, memoryHandle=%p", rc, dbHandle->appleMusicRadio.memoryHandle);
                rc = IPOD_PLAYER_ERR_DB_OPEN;
            }
        }
        else
        {
            IPOD_DLT_ERROR("Apple Music Radio fileHandle open failed. :rc=%d", rc);
            rc = IPOD_PLAYER_ERR_DB_OPEN;
        }

        if(rc == IPOD_PLAYER_OK)
        {
            /* Copy the backup data to memory based database */
            rc = ippiAP2DBCopyDB(dbHandle->appleMusicRadio.memoryHandle, dbHandle->appleMusicRadio.fileHandle);
            if(rc == IPOD_PLAYER_OK)
            {
                /* Initialize the database of memory database */
                rc = ippiAP2DBMemoryDBInit(dbHandle->appleMusicRadio.memoryHandle, deviceID);
                if(rc != IPOD_PLAYER_OK)
                {
                    IPOD_DLT_ERROR("Apple Music Radio memoryHandle initilize failed. :rc=%d", rc);
                }
            }
            else
            {
                IPOD_DLT_ERROR("Apple Music Radio database copy failed. :rc=%d", rc);
            }
        }
    }
    else
    {
        dbHandle->appleMusicRadio.fileHandle = NULL;
        /* Open database with memory */
        ret = sqlite3_open((const char *)IPOD_PLAYER_IAP2_IN_MEMORY_DB, &dbHandle->appleMusicRadio.memoryHandle);
        if(ret == SQLITE_OK)
        {
            rc = IPOD_PLAYER_OK;
        }
        else
        {
            IPOD_DLT_ERROR("Apple Music Radio memoryHandle open failed. :rc=%d, memoryHandle=%p", rc, dbHandle->appleMusicRadio.memoryHandle);
            rc = IPOD_PLAYER_ERR_DB_OPEN;
        }

        if(rc == IPOD_PLAYER_OK)
        {
            /* Initialize the database of memory database */
            rc = ippiAP2DBMemoryDBInit(dbHandle->appleMusicRadio.memoryHandle, deviceID);
            if(rc != IPOD_PLAYER_OK)
            {
                IPOD_DLT_ERROR("Apple Music Radio memoryHandle initilize failed. :rc=%d", rc);
            }
        }
    }

    /* Open iPod information database with memory */
    ret = sqlite3_open((const char*)IPOD_PLAYER_IAP2_IN_MEMORY_DB, &dbHandle->iPodHandle);
    if(ret == SQLITE_OK)
    {
        /* Initialize the database of ipod info database */
        rc = ippiAP2DBiPodInfoDBInit(dbHandle->iPodHandle);
        if(rc != IPOD_PLAYER_OK)
        {
            IPOD_DLT_ERROR("iPodHandle initialize failed. :rc=%d", rc);
        }
    }
    else
    {
        IPOD_DLT_ERROR("iPodHandle open failed. :rc=%d, iPodHandle=%p", rc, dbHandle->iPodHandle);
        rc = IPOD_PLAYER_ERR_DB_OPEN;
    }

    if(rc == IPOD_PLAYER_OK)
    {
        rc = ippiAP2SetNowPlayingInitDB(dbHandle);
        if(rc != IPOD_PLAYER_OK)
        {
            IPOD_DLT_ERROR("MediaItem database initialize failed. :rc=%d", rc);
        }
    }

    IPOD_DLT_INFO("localDevice.fileName=%s", dbHandle->localDevice.fileName);
    IPOD_DLT_INFO("localDevice.fileHandle=%p", dbHandle->localDevice.fileHandle);
    IPOD_DLT_INFO("localDevice.memoryHandle=%p", dbHandle->localDevice.memoryHandle);
    IPOD_DLT_INFO("appleMusicRadio.fileName=%s", dbHandle->appleMusicRadio.fileName);
    IPOD_DLT_INFO("appleMusicRadio.fileHandle=%p", dbHandle->appleMusicRadio.fileHandle);
    IPOD_DLT_INFO("appleMusicRadio.memoryHandle=%p", dbHandle->appleMusicRadio.memoryHandle);
    IPOD_DLT_INFO("iPodHandle=%p", dbHandle->iPodHandle);

    if(rc != IPOD_PLAYER_OK)
    {
        if(dbHandle->localDevice.fileHandle != NULL)
        {
            sqlite3_close(dbHandle->localDevice.fileHandle);
        }
        if(dbHandle->localDevice.memoryHandle != NULL)
        {
            sqlite3_close(dbHandle->localDevice.memoryHandle);
        }
        if(dbHandle->appleMusicRadio.fileHandle != NULL)
        {
            sqlite3_close(dbHandle->appleMusicRadio.fileHandle);
        }
        if(dbHandle->appleMusicRadio.memoryHandle != NULL)
        {
            sqlite3_close(dbHandle->appleMusicRadio.memoryHandle);
        }
        if(dbHandle->iPodHandle != NULL)
        {
            sqlite3_close(dbHandle->iPodHandle);
        }
        free(dbHandle);
        dbHandle = NULL;
    }

    return (void *)dbHandle;
}

void ippiAP2DBGetRevision(IPOD_PLAYER_IAP2_DB_MEDIA_LIBRARY_HANDLE *dbHandleOfMediaLibrary, U8 *memoryRevision)
{
    int ret = -1;

    if((dbHandleOfMediaLibrary == NULL) || (memoryRevision == NULL))
    {
        IPOD_DLT_ERROR("Parameter error. :dbHandleOfMediaLibrary=%p, memoryRevision=%p", dbHandleOfMediaLibrary, memoryRevision);
        return;
    }

    /* Get revision from memory database */
    if(dbHandleOfMediaLibrary->memoryHandle != NULL)
    {
        ret = sqlite3_exec(dbHandleOfMediaLibrary->memoryHandle, IPOD_PLAYER_IAP2_DB_QUERY_REVISION, ippiAP2DBGetStringCallback, memoryRevision, NULL);
        if(ret != SQLITE_OK)
        {
            IPOD_DLT_WARN("Could not get revision from memory DB");
        }
    }
    else
    {
        IPOD_DLT_ERROR("Invalid parameter memory DB handle");
    }
    
    return;
}

BOOL ippiAP2DBCheckRevision(IPOD_PLAYER_IAP2_DB_MEDIA_LIBRARY_HANDLE *dbHandleOfMediaLibrary, IPOD_PLAYER_CORE_IAP2_DB_STATUS *dbStatus)
{
    S32 rc = IPOD_PLAYER_ERROR;
    int ret = -1;
    BOOL revision = FALSE;
    U8 fileRevision[IPOD_PLAYER_IAP2_DB_MAX_REVISION_LEN] = {0};
    U8 memoryRevision[IPOD_PLAYER_IAP2_DB_MAX_REVISION_LEN] = {0};

    if((dbHandleOfMediaLibrary == NULL) || (dbStatus == NULL))
    {
        IPOD_DLT_ERROR("Parameter error. :dbHandleOfMediaLibrary=%p, dbStatus=%p", dbHandleOfMediaLibrary, dbStatus);
        return FALSE;
    }

    /* Current revision */
    strncpy((char *)memoryRevision, (const char *)dbStatus->dbRevision, sizeof(memoryRevision));

    /* Get revision from file database */
    if(dbHandleOfMediaLibrary->fileHandle != NULL)
    {
        ret = sqlite3_exec(dbHandleOfMediaLibrary->fileHandle, IPOD_PLAYER_IAP2_DB_QUERY_REVISION, ippiAP2DBGetStringCallback, fileRevision, NULL);
        if(ret == SQLITE_OK)
        {
            rc = IPOD_PLAYER_OK;
        }
        else
        {
            rc = IPOD_PLAYER_ERROR;
            //IPOD_DLT_WARN("[DBG]Could not get revision from file DB.");
        }
    }
    else
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, dbHandleOfMediaLibrary->fileHandle);
        rc = IPOD_PLAYER_ERROR;
    }
    
    if(rc == IPOD_PLAYER_OK)
    {
        //IPOD_DLT_INFO("[DBG]fileRevision=%s, memoryRevision=%s", fileRevision, memoryRevision);
        /* Compare if two revision is same */
        if(strncmp((const char *)fileRevision, (const char*)memoryRevision, IPOD_PLAYER_IAP2_DB_MAX_REVISION_LEN) == 0)
        {
            revision = TRUE;    /* same */
        }
        else
        {
            revision = FALSE;   /* different */
        }
    }
    
    return revision;
}

static S32 ippiAP2DBUpdate(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_IAP2_DB_MEDIA_LIBRARY_HANDLE *dbHandleOfMediaLibrary)
{
    S32 rc = IPOD_PLAYER_OK;

    if((iPodCtrlCfg != NULL) && (dbHandleOfMediaLibrary != NULL))
    {
        if((dbHandleOfMediaLibrary->fileHandle != NULL) && (dbHandleOfMediaLibrary->memoryHandle != NULL))
        {
            /* Memory based database will be backed up to file */
            rc = ippiAP2DBCopyDB(dbHandleOfMediaLibrary->fileHandle, dbHandleOfMediaLibrary->memoryHandle);
            if(rc == IPOD_PLAYER_OK)
            {
                /* store database event mask update */
                if(&iPodCtrlCfg->iap2Param.dbHandle->localDevice == dbHandleOfMediaLibrary)
                {
                    iPodCoreObserverSetUpdateDataMask(iPodCtrlCfg, IPP_IAP2_DATA_MASK_DEVICE_EVENT, IPOD_PLAYER_EVENT_MASK_STORE_DB, 0);
                    IPOD_DLT_INFO("Updated and stored database file of local device media library. :revision=%s", iPodCtrlCfg->iap2Param.dbStatusOfLocalDevice.dbRevision);
                }
                else
                {
                    iPodCoreObserverSetUpdateDataMask(iPodCtrlCfg, IPP_IAP2_DATA_MASK_DEVICE_EVENT, IPOD_PLAYER_EVENT_MASK_STORE_DB_AMR, 0);
                    IPOD_DLT_INFO("Updated and stored database file of Apple Music Radio media library. :revision=%s", iPodCtrlCfg->iap2Param.dbStatusOfAppleMusicRadio.dbRevision);
                }
            }
            else
            {
                IPOD_DLT_ERROR("Could not update internal Database file. :rc=%d", rc);
                rc = IPOD_PLAYER_ERR_DB_BACKUP;
            }
        }
        else
        {
            IPOD_DLT_ERROR("dbHandle is invalid. :fileHandle=%p, memoryHandle=%p", dbHandleOfMediaLibrary->fileHandle, dbHandleOfMediaLibrary->memoryHandle);
            rc = IPOD_PLAYER_ERR_INVALID_PARAMETER;
        }
    }
    else
    {
        IPOD_DLT_ERROR("Parameter error. :iPodCtrlCfg=%p, dbHandleOfMediaLibrary=%p", iPodCtrlCfg, dbHandleOfMediaLibrary);
        rc = IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    return rc;
}

void ippiAP2PlaylistTrackCheck(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_IAP2_DB_MEDIA_LIBRARY_HANDLE *dbHandleOfMediaLibrary, IPOD_PLAYER_CORE_IAP2_DB_STATUS *dbStatus, IPOD_PLAYER_CORE_IAP2_FILE_XFER_LIST **list, BOOL *setflag)
{
    U32 progress = 0;
    S32 rc = IPOD_PLAYER_OK;
    int listCount = 0;
    UniqueId_t UniqueId;

    if((iPodCtrlCfg == NULL) || (dbHandleOfMediaLibrary == NULL) || (dbStatus == NULL) || (list == NULL) || (setflag ==NULL))
    {
        IPOD_DLT_ERROR("Parameter error. :iPodCtrlCfg=%p, dbHandleOfMediaLibrary=%p, dbStatus=%p, list=%p, setflag=%p", iPodCtrlCfg, dbHandleOfMediaLibrary, dbStatus, list, setflag);
        return;
    }

    *setflag = FALSE;
    
    /* Get the progress from database */
    rc = ippiAP2DBGetProgress(dbHandleOfMediaLibrary, &progress);
    if(rc == IPOD_PLAYER_OK)
    {
        /* Progress is max(100%). It means that all database has been already retrieved from Apple devoce */
        if(progress == IPOD_PLAYER_IAP2_MAX_PROGRESS)
        {
            //IPOD_DLT_INFO("[DBG]mediaLibraryType=%d", dbStatus->mediaLibraryType);
            if(dbStatus->mediaLibraryType == IPOD_PLAYER_MEDIA_LIBRARY_TYPE_LOCAL)
            {
                listCount = iPodCoreObserverGetFileXferLocalDevicePlaylistCount(list);
            }
            else
            {
                listCount = iPodCoreObserverGetFileXferAppleMusicRadioPlaylistCount(list);
            }
            if(listCount == 0)
            {
                memset(&UniqueId, 0, sizeof(UniqueId));
                UniqueId.len = IPP_IAP2_UNIQUE_ID_MAX;
                /* Get Media Library UniqueId */
                rc = ippiAP2DBGetMediaLibraryID(dbHandleOfMediaLibrary, &UniqueId);
                if(rc == IPOD_PLAYER_OK)
                {
                    /* set revision to iPodInfo DB */
                    rc = ippiAP2DBSetMediaLibraryRevision(dbHandleOfMediaLibrary, UniqueId.id, dbStatus->dbRevision, progress);
                    if(rc == IPOD_PLAYER_OK)
                    {
                        *setflag = TRUE;
                    }
                    else
                    {
                        IPOD_DLT_ERROR("Could not set media library revision to DB. (rc = %d)", rc);
                    }
                }
                else
                {
                    IPOD_DLT_ERROR("Could not get Media library ID rc = %d.", rc);   
                }
            }
        }
    }
    else
    {
        IPOD_DLT_ERROR("Could not get Media library progress rc = %d.", rc);   
    }

    return;
}

S32 ippiAP2ProgressCheckDBUpdate(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_IAP2_DB_MEDIA_LIBRARY_HANDLE *dbHandleOfMediaLibrary)
{
    U32 progress = 0;
    S32 rc = IPOD_PLAYER_OK;
    
    if((iPodCtrlCfg == NULL) || (dbHandleOfMediaLibrary == NULL))
    {
        IPOD_DLT_ERROR("Parameter error. :iPodCtrlCfg=%p, dbHandleOfMediaLibrary=%p", iPodCtrlCfg, dbHandleOfMediaLibrary);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }

    /* Get the progress from database */
    rc = ippiAP2DBGetProgress(dbHandleOfMediaLibrary, &progress);
    if(rc == IPOD_PLAYER_OK)
    {
        /* Progress is max. It means that all database has been already retrieved from Apple devoce */
        if(progress == IPOD_PLAYER_IAP2_MAX_PROGRESS)
        {
            rc = ippiAP2DBUpdate(iPodCtrlCfg, dbHandleOfMediaLibrary);
        }
        else
        {
            rc = IPOD_PLAYER_ERR_DB_NOT_UPDATE;
        }
    }

    return rc;
}

void ippiAP2CloseDB(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg)
{
    IPOD_PLAYER_IAP2_DB_HANDLE *handle = NULL;
    BOOL bluetoothConnection = FALSE;
    S32 rcs = 0;
    struct stat stat_buf;

    /* Parameter check  */
    if(iPodCtrlCfg == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER);
        return;
    }

    handle = iPodCtrlCfg->iap2Param.dbHandle;
    if(handle == NULL)
    {
        IPOD_DLT_ERROR("dbHandle is null.");
        return;
    }

    IPOD_DLT_INFO("devType=%d", iPodCtrlCfg->threadInfo->nameInfo.devType);
    if(iPodCtrlCfg->threadInfo->nameInfo.devType == IPOD_PLAYER_DEVICE_TYPE_BT)
    {
        bluetoothConnection = TRUE;
    }

    /* Close Local Device database */
    if(!bluetoothConnection)
    {
        if(handle->localDevice.fileHandle != NULL)
        {
            /* Close database of file based */
            sqlite3_close(handle->localDevice.fileHandle);
            IPOD_DLT_INFO("Closed Local Device media library database.");
            rcs = stat((char *)handle->localDevice.fileName, &stat_buf);
            if(rcs == 0)
            {
                if(stat_buf.st_size == 0)
                {
                    remove((const char*)handle->localDevice.fileName);
                    IPOD_DLT_INFO("DB file remove for empty. :%s", handle->localDevice.fileName);
                }
            }
            else
            {
                IPOD_DLT_ERROR("stat failed. :%s", handle->localDevice.fileName);
            }
        }
    }

    if(handle->localDevice.memoryHandle != NULL)
    {
        /* Close database of memory based */
        sqlite3_close(handle->localDevice.memoryHandle);
    }

    /* Close Apple Music Radio database */
    if(!bluetoothConnection)
    {
        if(handle->appleMusicRadio.fileHandle != NULL)
        {
            /* Close database of file based */
            sqlite3_close(handle->appleMusicRadio.fileHandle);
            IPOD_DLT_INFO("Closed Apple Music Radio media library database.");
            rcs = stat((char *)handle->appleMusicRadio.fileName, &stat_buf);
            if(rcs == 0)
            {
                if(stat_buf.st_size == 0)
                {
                    remove((const char*)handle->appleMusicRadio.fileName);
                    IPOD_DLT_INFO("DB file remove for empty. :%s", handle->appleMusicRadio.fileName);
                }
            }
            else
            {
                IPOD_DLT_ERROR("stat failed. :%s", handle->appleMusicRadio.fileName);
            }
        }
    }

    if(handle->appleMusicRadio.memoryHandle != NULL)
    {
        /* Close database of memory based */
        sqlite3_close(handle->appleMusicRadio.memoryHandle);
    }

#if 0
    /* Todo */
    {
        sqlite3 *test = NULL;
        sqlite3_open((const char*)"/tmp/test.db", &test);
        ippiAP2DBCopyDB(test, handle->iPodHandle);
        sqlite3_close(test);
    }
#endif
    
    /* Close iPod information database */
    if(handle->iPodHandle != NULL)
    {
        /* Close database of iPod information */
        sqlite3_close(handle->iPodHandle);
    }
    
    free(handle);

    return;
}

static IPOD_PLAYER_PLAY_STATUS ippiAP2ConvertPlaybackStatus(iAP2PlaybackStatus status)
{
    IPOD_PLAYER_PLAY_STATUS playStatus = IPOD_PLAYER_PLAY_STATUS_UNKNOWN;
    
    switch(status)
    {
    case IAP2_PLAYBACK_STATUS_STOPPED:
        playStatus = IPOD_PLAYER_PLAY_STATUS_STOP;
        break;
        
    case IAP2_PLAYBACK_STATUS_PLAYING:
        playStatus = IPOD_PLAYER_PLAY_STATUS_PLAY;
        break;
        
    case IAP2_PLAYBACK_STATUS_PAUSED:
        playStatus = IPOD_PLAYER_PLAY_STATUS_PAUSE;
        break;
        
    case IAP2_PLAYBACK_STATUS_SEEK_FORWARD:
        playStatus = IPOD_PLAYER_PLAY_STATUS_FF;
        break;
        
    case IAP2_PLAYBACK_STATUS_SEEK_BACKWARD:
        playStatus = IPOD_PLAYER_PLAY_STATUS_RW;
        break;
        
    default:
        break;
    }
    
    return playStatus;
}

static IPOD_PLAYER_REPEAT_STATUS ippiAP2ConvertRepeatStatus(iAP2PlaybackRepeat repeat)
{
    IPOD_PLAYER_REPEAT_STATUS repeatStatus = IPOD_PLAYER_REPEAT_UNKNOWN;
    
    switch(repeat)
    {
    case IAP2_REPEAT_OFF:
        repeatStatus = IPOD_PLAYER_REPEAT_OFF;
        break;
        
    case IAP2_REPEAT_ONE:
        repeatStatus = IPOD_PLAYER_REPEAT_ONE;
        break;
        
    case IAP2_REPEAT_ALL:
        repeatStatus = IPOD_PLAYER_REPEAT_ALL;
        break;
        
    default:
        break;
    }
    
    return repeatStatus;
}

static IPOD_PLAYER_SHUFFLE_STATUS ippiAP2ConvertShuffleStatus(iAP2PlaybackShuffle shuffle)
{
    IPOD_PLAYER_SHUFFLE_STATUS shuffleStatus = IPOD_PLAYER_SHUFFLE_STATUS_UNKNOWN;
    
    switch(shuffle)
    {
    case IAP2_SHUFFLE_OFF:
        shuffleStatus = IPOD_PLAYER_SHUFFLE_OFF;
        break;
        
    case IAP2_SHUFFLE_SONGS:
        shuffleStatus = IPOD_PLAYER_SHUFFLE_TRACKS;
        break;
        
    case IAP2_SHUFFLE_ALBUMS:
        shuffleStatus = IPOD_PLAYER_SHUFFLE_ALBUMS;
        break;
        
    default:
        break;
    }
    
    return shuffleStatus;
}

static int ippiAP2InsertPlaybackAttribute(sqlite3_stmt *stmt, const U8*iPodID, iAP2PlaybackAttributes *item)
{
    int ret = -1;
    U32 i = 1;
    IPOD_PLAYER_PLAY_STATUS playbackStatus = IPOD_PLAYER_PLAY_STATUS_UNKNOWN;
    IPOD_PLAYER_REPEAT_STATUS repeatStatus = IPOD_PLAYER_REPEAT_UNKNOWN;
    IPOD_PLAYER_SHUFFLE_STATUS shuffleStatus = IPOD_PLAYER_SHUFFLE_STATUS_UNKNOWN;
    
    /* Parameter check */
    if((stmt == NULL) || (item == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, stmt, item);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    ret = sqlite3_reset(stmt);
    if(ret == SQLITE_OK)
    {
        /* Set the first parameter to prepared statement */
        ret = sqlite3_bind_text(stmt, i, (const char*)iPodID, -1, NULL);
        i++;
    }
    
    if(ret == SQLITE_OK)
    {
        if((item->iAP2PlaybackStatus_count > 0) && (item->iAP2PlaybackStatus != NULL))
        {
            playbackStatus = ippiAP2ConvertPlaybackStatus(*item->iAP2PlaybackStatus);
            /* Set the second parameter to prepared statement */
            ret = sqlite3_bind_int(stmt, i, playbackStatus);
            i++;
        }
        else
        {
            ret = sqlite3_bind_int(stmt, i, IPOD_PLAYER_IAP2_DB_MEDIA_ITEM_EMPTY_INT);
            i++;
        }
    }
    
    if(ret == SQLITE_OK)
    {
        /* Set the first parameter to prepared statement */
        /* Todo AudioBook Speed */
        ret = sqlite3_bind_int(stmt, i, -1);
        i++;
    }
    
    if(ret == SQLITE_OK)
    {
        if((item->iAP2PlaybackShuffleMode_count > 0) && (item->iAP2PlaybackShuffleMode != NULL))
        {
            shuffleStatus = ippiAP2ConvertShuffleStatus(*item->iAP2PlaybackShuffleMode);
            /* Set the second parameter to prepared statement */
            ret = sqlite3_bind_int(stmt, i, shuffleStatus);
            i++;
        }
        else
        {
            ret = sqlite3_bind_int(stmt, i, IPOD_PLAYER_IAP2_DB_MEDIA_ITEM_EMPTY_INT);
            i++;
        }
    }
    
    if(ret == SQLITE_OK)
    {
        if((item->iAP2PlaybackRepeatMode_count > 0) && (item->iAP2PlaybackRepeatMode != NULL))
        {
            repeatStatus = ippiAP2ConvertRepeatStatus(*item->iAP2PlaybackRepeatMode);
            /* Set the second parameter to prepared statement */
            ret = sqlite3_bind_int(stmt, i, repeatStatus);
            i++;
        }
        else
        {
            ret = sqlite3_bind_int(stmt, i, IPOD_PLAYER_IAP2_DB_MEDIA_ITEM_EMPTY_INT);
            i++;
        }
    }
    
    if(ret == SQLITE_OK)
    {
        if((item->iAP2PlaybackAppName_count > 0) && (item->iAP2PlaybackAppName != NULL) && (item->iAP2PlaybackAppName[0] != NULL))
        {
            /* Set the second parameter to prepared statement */
            ret = sqlite3_bind_text(stmt, i, (const char*)item->iAP2PlaybackAppName[0], -1, NULL);
            i++;
        }
        else
        {
            ret = sqlite3_bind_text(stmt, i, (const char*)IPOD_PLAYER_IAP2_DB_MEDIA_ITEM_EMPTY_TXT, -1, NULL);
            i++;
        }
    }

    if(ret == SQLITE_OK)
    {
        if((item->iAP2PlaybackAppBundleID_count > 0) && (item->iAP2PlaybackAppBundleID != NULL) && (item->iAP2PlaybackAppBundleID[0] != NULL))
        {
            /* Set the second parameter to prepared statement */
            ret = sqlite3_bind_text(stmt, i, (const char*)item->iAP2PlaybackAppBundleID[0], -1, NULL);
            i++;
        }
        else
        {
            ret = sqlite3_bind_text(stmt, i, (const char*)IPOD_PLAYER_IAP2_DB_MEDIA_ITEM_EMPTY_TXT, -1, NULL);
            i++;
        }
    }

    if(ret == SQLITE_OK)
    {
        if((item->iAP2PlaybackElapsedTimeInMilliseconds_count> 0) && (item->iAP2PlaybackElapsedTimeInMilliseconds != NULL))
        {
            /* Set the second parameter to prepared statement */
            ret = sqlite3_bind_int(stmt, i, *item->iAP2PlaybackElapsedTimeInMilliseconds);
            i++;
        }
        else
        {
            ret = sqlite3_bind_int(stmt, i, IPOD_PLAYER_IAP2_DB_MEDIA_ITEM_EMPTY_INT);
            i++;
        }
    }

    if(ret == SQLITE_OK)
    {
        if((item->iAP2PBMediaLibraryUniqueIdentifier_count > 0) && (item->iAP2PBMediaLibraryUniqueIdentifier != NULL) && (item->iAP2PBMediaLibraryUniqueIdentifier[0] != NULL))
        {
            /* Set the second parameter to prepared statement */
            ret = sqlite3_bind_text(stmt, i, (const char*)item->iAP2PBMediaLibraryUniqueIdentifier[0], -1, NULL);
            i++;
        }
        else
        {
            ret = sqlite3_bind_text(stmt, i, IPOD_PLAYER_IAP2_DB_MEDIA_ITEM_EMPTY_TXT, -1, NULL);
            i++;
        }
    }

    if(ret == SQLITE_OK)
    {
        if((item->iAP2PBAppleMusicRadioAd_count > 0) && (item->iAP2PBAppleMusicRadioAd != NULL))
        {
            /* Set the second parameter to prepared statement */
            ret = sqlite3_bind_int(stmt, i, *item->iAP2PBAppleMusicRadioAd);
            i++;
        }
        else
        {
            ret = sqlite3_bind_int(stmt, i, IPOD_PLAYER_IAP2_DB_MEDIA_ITEM_EMPTY_INT);
            i++;
        }
    }
    
    if(ret == SQLITE_OK)
    {
        if((item->iAP2PBAppleMusicRadioStationName_count > 0) && (item->iAP2PBAppleMusicRadioStationName != NULL) && (item->iAP2PBAppleMusicRadioStationName[0] != NULL))
        {
            /* Set the second parameter to prepared statement */
            ret = sqlite3_bind_text(stmt, i, (const char*)item->iAP2PBAppleMusicRadioStationName[0], -1, NULL);
            i++;
        }
        else
        {
            ret = sqlite3_bind_text(stmt, i, (const char*)IPOD_PLAYER_IAP2_DB_MEDIA_ITEM_EMPTY_TXT, -1, NULL);
            i++;
        }
    }

    if(ret == SQLITE_OK)
    {
        if((item->iAP2PBAppeMusicRadioStationMediaPlaylistPersistentID_count > 0) && (item->iAP2PBAppeMusicRadioStationMediaPlaylistPersistentID != NULL))
        {
            /* Set the second parameter to prepared statement */
            ret = sqlite3_bind_int64(stmt, i, *item->iAP2PBAppeMusicRadioStationMediaPlaylistPersistentID);
            i++;
        }
        else
        {
            ret = sqlite3_bind_int64(stmt, i, IPOD_PLAYER_IAP2_DB_MEDIA_ITEM_EMPTY_INT);
            i++;
        }
    }

    if(ret == SQLITE_OK)
    {
        if(item->iAP2PlaybackQueueChapterIndex_count > 0)
        {
            /* Set the second parameter to prepared statement */
            ret = sqlite3_bind_int(stmt, i, *item->iAP2PlaybackQueueChapterIndex);
            i++;
        }
        else
        {
            ret = sqlite3_bind_int(stmt, i, IPOD_PLAYER_IAP2_DB_MEDIA_ITEM_EMPTY_INT);
            i++;
        }
    }

    if(ret == SQLITE_OK)
    {
        if(item->iAP2PlaybackQueueIndex_count > 0)
        {
            /* Set the second parameter to prepared statement */
            ret = sqlite3_bind_int(stmt, i, *item->iAP2PlaybackQueueIndex);
            i++;
        }
        else
        {
            ret = sqlite3_bind_int(stmt, i, IPOD_PLAYER_IAP2_DB_MEDIA_ITEM_EMPTY_INT);
            i++;
        }
    }

    if(ret == SQLITE_OK)
    {
        if(item->iAP2PlaybackQueueCount_count > 0)
        {
            /* Set the second parameter to prepared statement */
            ret = sqlite3_bind_int(stmt, i, *item->iAP2PlaybackQueueCount);
            i++;
        }
        else
        {
            ret = sqlite3_bind_int(stmt, i, IPOD_PLAYER_IAP2_DB_MEDIA_ITEM_EMPTY_INT);
            i++;
        }
    }

    if(ret == SQLITE_OK)
    {
        if((item->iAP2PlaybackQueueListAvail_count > 0) && (item->iAP2PlaybackQueueListAvail != NULL))
        {
            /* Set the second parameter to prepared statement */
            ret = sqlite3_bind_int(stmt, i, *item->iAP2PlaybackQueueListAvail);
            i++;
        }
        else
        {
            ret = sqlite3_bind_int(stmt, i, IPOD_PLAYER_IAP2_DB_MEDIA_ITEM_EMPTY_INT);
            i++;
        }
    }

    return ret;
}

static int ippiAP2UpdatePlaybackAttribute(sqlite3_stmt *stmt, const U8 *iPodID, iAP2PlaybackAttributes *item)
{
    int ret = -1;
    U32 i = 1;
    static U32 speed = 0;
    IPOD_PLAYER_PLAY_STATUS playbackStatus = IPOD_PLAYER_PLAY_STATUS_UNKNOWN;
    IPOD_PLAYER_REPEAT_STATUS repeatStatus = IPOD_PLAYER_REPEAT_UNKNOWN;
    IPOD_PLAYER_SHUFFLE_STATUS shuffleStatus = IPOD_PLAYER_SHUFFLE_STATUS_UNKNOWN;

    
    /* Parameter check */
    if((stmt == NULL) || (item == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, stmt, iPodID, item);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    ret = sqlite3_reset(stmt);
    if(ret == SQLITE_OK)
    {
        /* Set the first parameter to prepared statement */
        /* Todo AudioBook Speed */
        ret = sqlite3_bind_int(stmt, i, speed);
        speed++;
        i++;
    }
    
    if(ret == SQLITE_OK)
    {
        if(item->iAP2PlaybackStatus_count > 0)
        {
            playbackStatus = ippiAP2ConvertPlaybackStatus(*item->iAP2PlaybackStatus);
            /* Set the second parameter to prepared statement */
            ret = sqlite3_bind_int(stmt, i, playbackStatus);
            i++;
        }
    }
    
    if(ret == SQLITE_OK)
    {
        if(item->iAP2PlaybackShuffleMode_count > 0)
        {
            shuffleStatus = ippiAP2ConvertShuffleStatus(*item->iAP2PlaybackShuffleMode);
            /* Set the second parameter to prepared statement */
            ret = sqlite3_bind_int(stmt, i, shuffleStatus);
            i++;
        }
    }
    
    if(ret == SQLITE_OK)
    {
        if(item->iAP2PlaybackRepeatMode_count > 0)
        {
            repeatStatus = ippiAP2ConvertRepeatStatus(*item->iAP2PlaybackRepeatMode);
            /* Set the second parameter to prepared statement */
            ret = sqlite3_bind_int(stmt, i, repeatStatus);
            i++;
        }
    }
    
    if(ret == SQLITE_OK)
    {
        if(item->iAP2PlaybackAppName_count > 0)
        {
            /* Set the second parameter to prepared statement */
            ret = sqlite3_bind_text(stmt, i, (const char*)item->iAP2PlaybackAppName[0], -1, NULL);
            i++;
        }
    }

    if(ret == SQLITE_OK)
    {
        if(item->iAP2PlaybackAppBundleID_count > 0)
        {
            /* Set the second parameter to prepared statement */
            ret = sqlite3_bind_text(stmt, i, (const char*)item->iAP2PlaybackAppBundleID[0], -1, NULL);
            i++;
        }
    }

    if(ret == SQLITE_OK)
    {
        if(item->iAP2PlaybackElapsedTimeInMilliseconds_count> 0)
        {
            /* Set the second parameter to prepared statement */
            ret = sqlite3_bind_int(stmt, i, *item->iAP2PlaybackElapsedTimeInMilliseconds);
            i++;
        }
    }

    if(ret == SQLITE_OK)
    {
        if(item->iAP2PBMediaLibraryUniqueIdentifier_count > 0)
        {
            /* Set the second parameter to prepared statement */
            ret = sqlite3_bind_text(stmt, i, (const char*)item->iAP2PBMediaLibraryUniqueIdentifier[0], -1, NULL);
            i++;
        }
    }

    if(ret == SQLITE_OK)
    {
        if(item->iAP2PBAppleMusicRadioAd_count > 0)
        {
            /* Set the second parameter to prepared statement */
            ret = sqlite3_bind_int(stmt, i, *item->iAP2PBAppleMusicRadioAd);
            i++;
        }
    }
    
    if(ret == SQLITE_OK)
    {
        if(item->iAP2PBAppleMusicRadioStationName_count > 0)
        {
            /* Set the second parameter to prepared statement */
            ret = sqlite3_bind_text(stmt, i, (const char*)item->iAP2PBAppleMusicRadioStationName[0], -1, NULL);
            i++;
        }
    }

    if(ret == SQLITE_OK)
    {
        if(item->iAP2PBAppeMusicRadioStationMediaPlaylistPersistentID_count > 0)
        {
            /* Set the second parameter to prepared statement */
            ret = sqlite3_bind_int64(stmt, i, *item->iAP2PBAppeMusicRadioStationMediaPlaylistPersistentID);
            i++;
        }
    }

    if(ret == SQLITE_OK)
    {
        if(item->iAP2PlaybackQueueChapterIndex_count > 0)
        {
            /* Set the second parameter to prepared statement */
            ret = sqlite3_bind_int(stmt, i, *item->iAP2PlaybackQueueChapterIndex);
            i++;
        }
    }

    if(ret == SQLITE_OK)
    {
        if(item->iAP2PlaybackQueueIndex_count > 0)
        {
            /* Set the second parameter to prepared statement */
            ret = sqlite3_bind_int(stmt, i, *item->iAP2PlaybackQueueIndex);
            i++;
        }
    }

    if(ret == SQLITE_OK)
    {
        if(item->iAP2PlaybackQueueCount_count > 0)
        {
            /* Set the second parameter to prepared statement */
            ret = sqlite3_bind_int(stmt, i, *item->iAP2PlaybackQueueCount);
            i++;
        }
    }

    if(ret == SQLITE_OK)
    {
        if(item->iAP2PlaybackQueueListAvail_count > 0)
        {
            /* Set the second parameter to prepared statement */
            ret = sqlite3_bind_int(stmt, i, *item->iAP2PlaybackQueueListAvail);
            i++;
        }
    }

    if(ret == SQLITE_OK)
    {
        /* Set the second parameter to prepared statement */
        ret = sqlite3_bind_text(stmt, i, (const char*)iPodID, -1, NULL);
        i++;
    }
    
    return ret;
    
}

S32 ippiAP2SetMediaItemDB(IPOD_PLAYER_IAP2_DB_MEDIA_LIBRARY_HANDLE *dbHandleOfMediaLibrary, U32 count, iAP2MediaItem *items)
{
    S32 rc = IPOD_PLAYER_ERROR;
    int ret = -1;
    U32 i = 0;
    sqlite3 *handle = NULL;
    sqlite3_stmt *stmt = NULL;
    
    /* Parameter check */
    if((dbHandleOfMediaLibrary == NULL) || (items == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, dbHandleOfMediaLibrary, items);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    handle = dbHandleOfMediaLibrary->memoryHandle;
    if(handle == NULL)
    {
        IPOD_DLT_ERROR("memoryHandle is null.");
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }

    /* Compile the sql statement to byte code */
    ret = sqlite3_prepare(handle, IPOD_PLAYER_IAP2_DB_INSERT_ITEM_PREPARE, strlen(IPOD_PLAYER_IAP2_DB_INSERT_ITEM_PREPARE), &stmt, NULL);
    if((ret == SQLITE_OK) && (stmt != NULL))
    {
        ret = sqlite3_exec(handle, IPOD_PLAYER_IAP2_DB_TRANSACTION_BEGIN, NULL, NULL, NULL);
        
        for(i = 0; i < count; i++)
        {
            ret = ippiAP2DBSetMediaItemStatementBind(stmt, &items[i]);
            if(ret == SQLITE_OK)
            {
                ret = sqlite3_step(stmt);
                if(ret == SQLITE_DONE)
                {
                    rc = IPOD_PLAYER_OK;
                }
                else
                {
                    IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, ret, stmt);
                    /* Todo new error */
                    rc = IPOD_PLAYER_ERROR;
                }
            }
            else
            {
                IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, ret, stmt);
                /* Todo new error */
                rc = IPOD_PLAYER_ERROR;
            }
        }
        
        /* Todo */
        ret = sqlite3_exec(handle, IPOD_PLAYER_IAP2_DB_TRANSACTION_COMMIT, NULL, NULL, NULL);
    }
    else
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, ret, stmt);
        /* Todo new error */
        rc = IPOD_PLAYER_ERROR;
    }
    
    if(stmt != NULL)
    {
        /* Free the prepared statement */
        sqlite3_finalize(stmt);
    }
    
    return rc;
}

static S32 ippiAP2SetNowPlayingItemDB(sqlite3 *handle, const U8 *content, IPP_IAP2_DB_BIND_TYPE type, void *data)
{
    S32 rc = IPOD_PLAYER_ERROR;
    int ret = -1;
    sqlite3_stmt *stmt = NULL;
    char s[IPOD_PLAYER_IAP2_DB_STRING_MAX_LEN] = IPOD_PLAYER_IAP2_DB_UPDATE_PLAYING_ITEM_PREPARE_START;

    //IPOD_DLT_INFO("[DBG]enter:content=%s", content);
    /* Parameter check */
    if((content == NULL) || (data == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, content, data);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    if((sizeof(IPOD_PLAYER_IAP2_DB_UPDATE_PLAYING_ITEM_PREPARE_START) + sizeof(IPOD_PLAYER_IAP2_DB_UPDATE_PLAYING_ITEM_PREPARE_PARAM) + strlen((const char *)content)) > sizeof(s))
    {
        IPOD_DLT_ERROR("content string too long. length=%zu", strlen((const char *)content));
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }

    strcat(s, (const char *)content);
    strcat(s, IPOD_PLAYER_IAP2_DB_UPDATE_PLAYING_ITEM_PREPARE_PARAM);
    ret = sqlite3_prepare(handle, s, strlen(s), &stmt, NULL);
    if((ret == SQLITE_OK) && (stmt != NULL))
    {
        ret = sqlite3_exec(handle, IPOD_PLAYER_IAP2_DB_TRANSACTION_BEGIN, NULL, NULL, NULL);
        if(ret == SQLITE_OK)
        {
            ret = ippiAP2DBSetBind(stmt, type, 1, data, 1);
            if(ret == SQLITE_OK)
            {
                ret = sqlite3_step(stmt);
                if(ret == SQLITE_DONE)
                {
                    ret = sqlite3_exec(handle, IPOD_PLAYER_IAP2_DB_TRANSACTION_COMMIT, NULL, NULL, NULL);
                    if(ret == SQLITE_OK)
                    {
                        rc = IPOD_PLAYER_OK;
                    }
                    else
                    {
                        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, ret, stmt);
                        rc = IPOD_PLAYER_ERROR;
                    }
                }
                else
                {
                    IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, ret, stmt);
                    rc = IPOD_PLAYER_ERROR;
                }
            }
            else
            {
                IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, ret, stmt);
                rc = IPOD_PLAYER_ERROR;
            }
        }
        else
        {
            IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, ret, stmt);
            rc = IPOD_PLAYER_ERROR;
        }
    }
    else
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, ret, stmt);
        rc = IPOD_PLAYER_ERROR;
    }

    if(stmt != NULL)
    {
        /* Free the prepared statement */
        sqlite3_finalize(stmt);
    }

    return rc;
}

S32 ippiAP2SetNowPlayingDB(IPOD_PLAYER_IAP2_DB_HANDLE *dbHandle, iAP2MediaItem *item)
{
    S32 rc = IPOD_PLAYER_OK;
    sqlite3 *handle = NULL;

    /* Parameter check */
    if((dbHandle == NULL) || (item == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, dbHandle, item);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }

    handle = dbHandle->iPodHandle;
    if(item->iAP2MediaItemPersistentIdentifier_count > 0)
    {
        rc = ippiAP2SetNowPlayingItemDB(handle, (const U8*)IPOD_PLAYER_IAP2_DB_COLUMN_TRACKID, IPP_IAP2_DB_BIND_U64, item->iAP2MediaItemPersistentIdentifier);
    }
    if((rc == IPOD_PLAYER_OK) && (item->iAP2MediaItemTitle_count > 0))
    {
        rc = ippiAP2SetNowPlayingItemDB(handle, (const U8*)IPOD_PLAYER_IAP2_DB_COLUMN_TRACKTITLE, IPP_IAP2_DB_BIND_STR, item->iAP2MediaItemTitle);
    }
    if((rc == IPOD_PLAYER_OK) && (item->iAP2MediaItemAlbumPersistentIdentifier_count > 0))
    {
        rc = ippiAP2SetNowPlayingItemDB(handle, (const U8*)IPOD_PLAYER_IAP2_DB_COLUMN_ALBUMID, IPP_IAP2_DB_BIND_U64, item->iAP2MediaItemAlbumPersistentIdentifier);
    }
    if((rc == IPOD_PLAYER_OK) && (item->iAP2MediaItemAlbumTitle_count > 0))
    {
        rc = ippiAP2SetNowPlayingItemDB(handle, (const U8*)IPOD_PLAYER_IAP2_DB_COLUMN_ALBUMTITLE, IPP_IAP2_DB_BIND_STR, item->iAP2MediaItemAlbumTitle);
    }
    if((rc == IPOD_PLAYER_OK) && (item->iAP2MediaItemAlbumDiscCount_count > 0))
    {
        rc = ippiAP2SetNowPlayingItemDB(handle, (const U8*)IPOD_PLAYER_IAP2_DB_COLUMN_ALBUMDISCCOUNT, IPP_IAP2_DB_BIND_U16, item->iAP2MediaItemAlbumDiscCount);
    }
    if((rc == IPOD_PLAYER_OK) && (item->iAP2MediaItemAlbumDiscNumber_count > 0))
    {
        rc = ippiAP2SetNowPlayingItemDB(handle, (const U8*)IPOD_PLAYER_IAP2_DB_COLUMN_ALBUMDISCNUMBER, IPP_IAP2_DB_BIND_U16, item->iAP2MediaItemAlbumDiscNumber);
    }
    if((rc == IPOD_PLAYER_OK) && (item->iAP2MediaItemAlbumArtistPersistentIdentifier_count > 0))
    {
        rc = ippiAP2SetNowPlayingItemDB(handle, (const U8*)IPOD_PLAYER_IAP2_DB_COLUMN_ALBUMARTISTID, IPP_IAP2_DB_BIND_U64, item->iAP2MediaItemAlbumArtistPersistentIdentifier);
    }
    if((rc == IPOD_PLAYER_OK) && (item->iAP2MediaItemAlbumArtist_count > 0))
    {
        rc = ippiAP2SetNowPlayingItemDB(handle, (const U8*)IPOD_PLAYER_IAP2_DB_COLUMN_ALBUMARTIST, IPP_IAP2_DB_BIND_STR, item->iAP2MediaItemAlbumArtist);
    }
    if((rc == IPOD_PLAYER_OK) && (item->iAP2MediaItemAlbumTrackCount_count > 0))
    {
        rc = ippiAP2SetNowPlayingItemDB(handle, (const U8*)IPOD_PLAYER_IAP2_DB_COLUMN_ALBUMTRACKCOUNT, IPP_IAP2_DB_BIND_U16, item->iAP2MediaItemAlbumTrackCount);
    }
    if((rc == IPOD_PLAYER_OK) && (item->iAP2MediaItemAlbumTrackNumber_count > 0))
    {
        rc = ippiAP2SetNowPlayingItemDB(handle, (const U8*)IPOD_PLAYER_IAP2_DB_COLUMN_ALBUMTRACKNUMBER, IPP_IAP2_DB_BIND_U16, item->iAP2MediaItemAlbumTrackNumber);
    }
    if((rc == IPOD_PLAYER_OK) && (item->iAP2MediaItemArtistPersistentIdentifier_count > 0))
    {
        rc = ippiAP2SetNowPlayingItemDB(handle, (const U8*)IPOD_PLAYER_IAP2_DB_COLUMN_ARTISTID, IPP_IAP2_DB_BIND_U64, item->iAP2MediaItemArtistPersistentIdentifier);
    }
    if((rc == IPOD_PLAYER_OK) && (item->iAP2MediaItemArtist_count > 0))
    {
        rc = ippiAP2SetNowPlayingItemDB(handle, (const U8*)IPOD_PLAYER_IAP2_DB_COLUMN_ARTIST, IPP_IAP2_DB_BIND_STR, item->iAP2MediaItemArtist);
    }
    if((rc == IPOD_PLAYER_OK) && (item->iAP2MediaItemArtworkFileTransferIdentifier_count > 0))
    {
        rc = ippiAP2SetNowPlayingItemDB(handle, (const U8*)IPOD_PLAYER_IAP2_DB_COLUMN_ARTWORKFILEID, IPP_IAP2_DB_BIND_U16, item->iAP2MediaItemArtworkFileTransferIdentifier);
    }
    if((rc == IPOD_PLAYER_OK) && (item->iAP2MediaItemComposerPersistentIdentifier_count > 0))
    {
        rc = ippiAP2SetNowPlayingItemDB(handle, (const U8*)IPOD_PLAYER_IAP2_DB_COLUMN_COMPOSERID, IPP_IAP2_DB_BIND_U64, item->iAP2MediaItemComposerPersistentIdentifier);
    }
    if((rc == IPOD_PLAYER_OK) && (item->iAP2MediaItemComposer_count > 0))
    {
        rc = ippiAP2SetNowPlayingItemDB(handle, (const U8*)IPOD_PLAYER_IAP2_DB_COLUMN_COMPOSER, IPP_IAP2_DB_BIND_STR, item->iAP2MediaItemComposer);
    }
    if((rc == IPOD_PLAYER_OK) && (item->iAP2MediaItemGenrePersistentIdentifier_count > 0))
    {
        rc = ippiAP2SetNowPlayingItemDB(handle, (const U8*)IPOD_PLAYER_IAP2_DB_COLUMN_GENREID, IPP_IAP2_DB_BIND_U64, item->iAP2MediaItemGenrePersistentIdentifier);
    }
    if((rc == IPOD_PLAYER_OK) && (item->iAP2MediaItemGenre_count > 0))
    {
        rc = ippiAP2SetNowPlayingItemDB(handle, (const U8*)IPOD_PLAYER_IAP2_DB_COLUMN_GENRE, IPP_IAP2_DB_BIND_STR, item->iAP2MediaItemGenre);
    }
    if((rc == IPOD_PLAYER_OK) && (item->iAP2MediaItemIsBanSupported_count > 0))
    {
        rc = ippiAP2SetNowPlayingItemDB(handle, (const U8*)IPOD_PLAYER_IAP2_DB_COLUMN_ISBANSUPPORTED, IPP_IAP2_DB_BIND_U8, item->iAP2MediaItemIsBanSupported);
    }
    if((rc == IPOD_PLAYER_OK) && (item->iAP2MediaItemIsBanned_count > 0))
    {
        rc = ippiAP2SetNowPlayingItemDB(handle, (const U8*)IPOD_PLAYER_IAP2_DB_COLUMN_ISBANNED, IPP_IAP2_DB_BIND_U8, item->iAP2MediaItemIsBanned);
    }
    if((rc == IPOD_PLAYER_OK) && (item->iAP2MediaItemIsLikeSupported_count > 0))
    {
        rc = ippiAP2SetNowPlayingItemDB(handle, (const U8*)IPOD_PLAYER_IAP2_DB_COLUMN_ISLIKESUPPORTED, IPP_IAP2_DB_BIND_U8, item->iAP2MediaItemIsLikeSupported);
    }
    if((rc == IPOD_PLAYER_OK) && (item->iAP2MediaItemIsLiked_count > 0))
    {
        rc = ippiAP2SetNowPlayingItemDB(handle, (const U8*)IPOD_PLAYER_IAP2_DB_COLUMN_ISLIKED, IPP_IAP2_DB_BIND_U8, item->iAP2MediaItemIsLiked);
    }
    if((rc == IPOD_PLAYER_OK) && (item->iAP2MediaItemIsResidentOnDevice_count > 0))
    {
        rc = ippiAP2SetNowPlayingItemDB(handle, (const U8*)IPOD_PLAYER_IAP2_DB_COLUMN_REGIDENTONDEVICE, IPP_IAP2_DB_BIND_U8, item->iAP2MediaItemIsResidentOnDevice);
    }
    if((rc == IPOD_PLAYER_OK) && (item->iAP2MediaItemIsPartOfCompilation_count > 0))
    {
        rc = ippiAP2SetNowPlayingItemDB(handle, (const U8*)IPOD_PLAYER_IAP2_DB_COLUMN_ISPARTOFCOMPILATION, IPP_IAP2_DB_BIND_U8, item->iAP2MediaItemIsPartOfCompilation);
    }
    if((rc == IPOD_PLAYER_OK) && (item->iAP2MediaItemMediaType_count > 0))
    {
        rc = ippiAP2SetNowPlayingItemDB(handle, (const U8*)IPOD_PLAYER_IAP2_DB_COLUMN_MEDIATYPE, IPP_IAP2_DB_BIND_U32, item->iAP2MediaItemMediaType);
    }
    if((rc == IPOD_PLAYER_OK) && (item->iAP2MediaItemRating_count > 0))
    {
        rc = ippiAP2SetNowPlayingItemDB(handle, (const U8*)IPOD_PLAYER_IAP2_DB_COLUMN_MEDIARATING, IPP_IAP2_DB_BIND_U8, item->iAP2MediaItemRating);
    }
    if((rc == IPOD_PLAYER_OK) && (item->iAP2MediaItemPlaybackDurationInMilliseconds_count > 0))
    {
        rc = ippiAP2SetNowPlayingItemDB(handle, (const U8*)IPOD_PLAYER_IAP2_DB_COLUMN_MEDIADURATIONMS, IPP_IAP2_DB_BIND_U32, item->iAP2MediaItemPlaybackDurationInMilliseconds);
    }
    if((rc == IPOD_PLAYER_OK) && (item->iAP2MediaItemChapterCount_count > 0))
    {
        rc = ippiAP2SetNowPlayingItemDB(handle, (const U8*)IPOD_PLAYER_IAP2_DB_COLUMN_CHAPTERCOUNT, IPP_IAP2_DB_BIND_U16, item->iAP2MediaItemChapterCount);
    }
    if(rc != IPOD_PLAYER_OK)
    {
        IPOD_DLT_ERROR("ippiAP2SetNowPlayingDB error. rc=%d", rc);
    }

    return rc;
}

static int ippiAP2InsertiPodInfo(sqlite3 *handle, const U8 *deviceID, iAP2PlaybackAttributes *item)
{
    S32 rc = IPOD_PLAYER_ERROR;
    int ret = -1;
    sqlite3_stmt *stmt = NULL;
    
    /* Parameter check */
    if((handle == NULL) || (item == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, handle, item);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Compile the sql statement to byte code */
    ret = sqlite3_prepare(handle, IPOD_PLAYER_IAP2_DB_IPODINFO_ITEM_PREPARE, strlen(IPOD_PLAYER_IAP2_DB_IPODINFO_ITEM_PREPARE), &stmt, NULL);
    if((ret == SQLITE_OK) && (stmt != NULL))
    {
        ret = sqlite3_exec(handle, IPOD_PLAYER_IAP2_DB_TRANSACTION_BEGIN, NULL, NULL, NULL);
        if(ret == SQLITE_OK)
        {
            ret = ippiAP2InsertPlaybackAttribute(stmt, deviceID, item);
            if(ret == SQLITE_OK)
            {
                ret = sqlite3_step(stmt);
                if(ret != SQLITE_DONE)
                {
                    IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, ret, stmt);
                }
            }
            else
            {
                IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, ret, stmt);
            }
            sqlite3_exec(handle, IPOD_PLAYER_IAP2_DB_TRANSACTION_COMMIT, NULL, NULL, NULL);
        }
        else
        {
            IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, ret, stmt);
        }
    }
    else
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, ret, stmt);
    }
    
    if(stmt != NULL)
    {
        /* Free the prepared statement */
        sqlite3_finalize(stmt);
    }
    
    if(ret == SQLITE_DONE)
    {
        rc = IPOD_PLAYER_OK;
    }
    else
    {
        rc = IPOD_PLAYER_ERROR;
    }
    
    return rc;
}

static int ippiAP2UpdateiPodInfo(sqlite3 *handle, const U8 *iPodID, const U8 *statement, iAP2PlaybackAttributes *item)
{
    S32 rc = IPOD_PLAYER_ERROR;
    int ret = -1;
    sqlite3_stmt *stmt = NULL;
    
    /* Parameter check */
    if((handle == NULL) || (statement == NULL) || (item == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, handle, item);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Compile the sql statement to byte code */
    ret = sqlite3_prepare(handle, (const char*)statement, strlen((const char*)statement), &stmt, NULL);
    if((ret == SQLITE_OK) && (stmt != NULL))
    {
        ret = sqlite3_exec(handle, IPOD_PLAYER_IAP2_DB_TRANSACTION_BEGIN, NULL, NULL, NULL);
        if(ret == SQLITE_OK)
        {
            ret = ippiAP2UpdatePlaybackAttribute(stmt, iPodID, item);
            if(ret == SQLITE_OK)
            {
                ret = sqlite3_step(stmt);
                if(ret != SQLITE_DONE)
                {
                    IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, ret, stmt);
                }
            }
            else
            {
                IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, ret, stmt);
            }
            
            sqlite3_exec(handle, IPOD_PLAYER_IAP2_DB_TRANSACTION_COMMIT, NULL, NULL, NULL);
        }
        else
        {
            IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, ret, stmt);
        }
        
    }
    else
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, ret, stmt);
        /* Todo new error */
        rc = IPOD_PLAYER_ERROR;
    }
    
    if(stmt != NULL)
    {
        /* Free the prepared statement */
        sqlite3_finalize(stmt);
    }
    
    if(ret == SQLITE_DONE)
    {
        rc = IPOD_PLAYER_OK;
    }
    else
    {
        rc = IPOD_PLAYER_ERROR;
    }
    
    return rc;
}

static S32 ippiAP2GetMemberCount(sqlite3 *handle, const U8 *tableName, const U8 *key, const U8 *name)
{
    S32 rc = IPOD_PLAYER_ERROR;
    int ret = -1;
    sqlite3_stmt *stmt = NULL;
    U32 i = 1;
    U8 *statement = NULL;
    U32 length = 0;
    
    /* Parameter check */
    if((handle == NULL) || (tableName == NULL) || (key == NULL) || (name == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, handle, statement, key, name);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    length = strlen((const char *)IPOD_PLAYER_IAP2_DB_QUERY_COUNT_PREPARE) + strlen((const char*)tableName) + strlen((const char *)key) + 1;
    
    statement = calloc(length, sizeof(U8));
    if(statement != NULL)
    {
        snprintf((char *)statement, length, (const char *)IPOD_PLAYER_IAP2_DB_QUERY_COUNT_PREPARE, tableName, key);
        /* Compile the sql statement to byte code */
        ret = sqlite3_prepare(handle, (const char *)statement, strlen((const char*)statement), &stmt, NULL);
        if((ret == SQLITE_OK) && (stmt != NULL))
        {
            ret = sqlite3_bind_text(stmt, i, (const char*)name, -1, NULL);
        }
        
        if(ret == SQLITE_OK)
        {
            ret = sqlite3_step(stmt);
            if(ret == SQLITE_ROW)
            {
                ret = sqlite3_column_count(stmt);
                if(ret > 0)
                {
                    ret = sqlite3_column_int(stmt, 0);
                    rc = ret;
                }
                else
                {
                    rc = IPOD_PLAYER_ERROR;
                }
            }
            else
            {
                rc = IPOD_PLAYER_ERROR;
            }
        }
        else
        {
            rc = IPOD_PLAYER_ERROR;
        }
        
        free(statement);
    }
    else
    {
        rc = IPOD_PLAYER_ERR_NOMEM;
    }
    
    if(stmt != NULL)
    {
        sqlite3_finalize(stmt);
    }

    return rc;
}

S32 ippiAP2SetiPodInfoDB(IPOD_PLAYER_IAP2_DB_HANDLE *dbHandle, const U8 *deviceID, iAP2PlaybackAttributes *item)
{
    S32 rc = IPOD_PLAYER_ERROR;
    sqlite3 *handle = NULL;
    
    /* Parameter check */
    if((dbHandle == NULL) || (deviceID == NULL) || (item == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, dbHandle, deviceID, item);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    handle = dbHandle->iPodHandle;
    rc = ippiAP2GetMemberCount(handle, (const U8*)"iPodInfo", (const U8*)"iPodID", deviceID);
    if(rc == 0)
    {
        rc = ippiAP2InsertiPodInfo(handle, deviceID, item);
    }
    else
    {
        U8 temp[4096] = {0};
        ippiAP2GenerateStatement(4096, temp, item);
        //IPOD_DLT_INFO("[DBG]statement = %s", temp);
        rc = ippiAP2UpdateiPodInfo(handle, deviceID, temp, item);
    }
    
    return rc;
}

static void ippiAP2DBFreeStatement(U8 *statement, U8 execType)
{
    /* Parameter check */
    if(statement == NULL)
    {
        return;
    }
    
    if(execType == 0)
    {
        free(statement);
    }
    else
    {
        sqlite3_finalize((sqlite3_stmt *)statement);
    }
    
    return;
}


static S32 ippiAP2DBExecuteStatement(sqlite3 *handle, U8 execType, const U8 *statement, void *callback, void *arg)
{
    S32 rc = IPOD_PLAYER_ERROR;
    int ret = -1;
    char *err = NULL;
    
    /* Parameter check */
    if((handle == NULL) || (statement == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, handle, statement, callback, arg);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
#ifdef SQL_STATEMENT_LOG
    IPOD_DLT_INFO("ippiAP2DBExecuteStatement = %s", statement);
#endif /* #ifdef SQL_STATEMENT_LOG */

    if(execType == 0)
    {
        ret = sqlite3_exec(handle, (const char *)statement, callback, arg, &err);
        if(ret == SQLITE_OK)
        {
            rc = IPOD_PLAYER_OK;
        }
        else
        {
            if(err != NULL)
            {
                IPOD_DLT_WARN("sqlite3_exec failed :%d, statement=%s, err=%s", ret, statement, err);
                sqlite3_free(err);
            }
            else
            {
                IPOD_DLT_WARN("sqlite3_exec failed :%d, statement=%s", ret, statement);
            }
        }
    }
    else
    {
        ret = sqlite3_step((sqlite3_stmt *)statement);
        if((ret == SQLITE_DONE) || (ret == SQLITE_ROW))
        {
            rc = IPOD_PLAYER_OK;
        }
        else
        {
            IPOD_ERR_WRITE_PTR(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, ret, statement);
        }
    }
    
    return rc;
}

S32 ippiAP2DBGetPlaybackStatus(IPOD_PLAYER_IAP2_DB_HANDLE *dbHandle, U32 dataSize, U8 *data)
{
    S32 rc = IPOD_PLAYER_ERROR;
    sqlite3 *handle = NULL;
    U8 *statement = NULL;
    
    dataSize = dataSize;
    
    /* Parameter check */
    if((dbHandle == NULL) || (data == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, dbHandle, data);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }

    statement = ippiAP2DBGenerateGetPlaybackStatusStatement();
    if(statement != NULL)
    {
        handle = dbHandle->iPodHandle;
        rc = ippiAP2DBExecuteStatement(handle, 0, statement, ippiAP2DBPlaybackStatusCallback, data);
        ippiAP2DBFreeStatement(statement, 0);
    }
    
    return rc;
}

S32 ippiAP2DBSetMediaLibraryInformation(IPOD_PLAYER_IAP2_DB_MEDIA_LIBRARY_HANDLE *dbHandleOfMediaLibrary, const U8 *key, iAP2MediaLibraryInformationSubParameter *media)
{
    S32 rc = IPOD_PLAYER_ERROR;
    sqlite3 *handle = NULL;
    sqlite3_stmt *statement = NULL;
    
    /* Parameter check */
    if((dbHandleOfMediaLibrary == NULL) || (key == NULL) || (media == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, dbHandleOfMediaLibrary, key, media);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    if((media->iAP2MediaLibraryName != NULL) &&
       (media->iAP2MediaLibraryName[0] != NULL) &&
       (media->iAP2MediaUniqueIdentifier != NULL) &&
       (media->iAP2MediaUniqueIdentifier[0] != NULL))
    {
        handle = dbHandleOfMediaLibrary->memoryHandle;
        statement = ippiAP2DBGenerateSetMediaLibraryInformationStatement(handle, key, media);
        if(statement != NULL)
        {
            rc = ippiAP2DBExecuteStatement(handle, 1, (U8 *)statement, NULL, NULL);
            ippiAP2DBFreeStatement((U8 *)statement, 1);
        }
    }
    
    return rc;
}

S32 ippiAP2DBGetMediaLibraryInformation(IPOD_PLAYER_IAP2_DB_MEDIA_LIBRARY_HANDLE *dbHandleOfMediaLibrary, const U8 *key, IPOD_PLAYER_IAP2_DB_MEDIAINFO *mediaInfo)
{
    S32 rc = IPOD_PLAYER_ERROR;
    sqlite3 *handle = NULL;
    U8 *statement = NULL;
    
    /* Parameter check */
    if((dbHandleOfMediaLibrary == NULL) || (key == NULL) || (mediaInfo == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, dbHandleOfMediaLibrary, key, mediaInfo);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    handle = dbHandleOfMediaLibrary->memoryHandle;
    statement = ippiAP2DBGenerateGetMediaLibraryInformationStatement(handle, key);
    if(statement != NULL)
    {
        rc = ippiAP2DBExecuteStatement(handle, 0, (U8 *)statement, ippiAP2DBGetMediaLibraryInformationCB, mediaInfo);
        ippiAP2DBFreeStatement((U8 *)statement, 0);
    }
    
    return rc;
}

S32 ippiAP2DBSetMediaLibraryRevision(IPOD_PLAYER_IAP2_DB_MEDIA_LIBRARY_HANDLE *dbHandleOfMediaLibrary, const U8 *mediaID, const U8 *revision, U32 progress)
{
    S32 rc = IPOD_PLAYER_ERROR;
    sqlite3 *handle = NULL;
    sqlite3_stmt *statement = NULL;
    
    /* Parameter check */
    /* Note: revision may be NULL */
    if((dbHandleOfMediaLibrary == NULL) || (mediaID == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, dbHandleOfMediaLibrary, mediaID, revision);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    handle = dbHandleOfMediaLibrary->memoryHandle;
    statement = ippiAP2DBGenerateSetRevisionStatement(handle, mediaID, revision, progress);
    if(statement != NULL)
    {
        rc = ippiAP2DBExecuteStatement(handle, 1, (U8 *)statement, NULL, NULL);
        ippiAP2DBFreeStatement((U8 *)statement, 1);
    }
    
    return rc;
}

S32 ippiAP2DBGetMediaItem(IPOD_PLAYER_IAP2_DB_MEDIA_LIBRARY_HANDLE *dbHandleOfMediaLibrary, IPOD_PLAYER_DB_TYPE type, U32 start, U32 count, IPOD_PLAYER_IAP2_DB_CATLIST *catList, IPOD_PLAYER_ENTRY_LIST *entryList)
{
    S32 rc = IPOD_PLAYER_ERROR;
    sqlite3 *handle = NULL;
    U8 *statement = NULL;
    IPOD_PLAYER_ENTRY_LIST_INT  entryListInt; 
    
    /* Parameter check */
    if((dbHandleOfMediaLibrary == NULL) || (catList == NULL) || (entryList == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, dbHandleOfMediaLibrary, catList, entryList);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    handle = dbHandleOfMediaLibrary->memoryHandle;
    /* Get entries by selecting category  */
    statement = ippiAP2DBGenerateGetListStatement(type, catList, start, count);
    if(statement != NULL)
    {
        ippiAP2DBExecuteStatement(handle, 0, (U8 *)IPOD_PLAYER_IAP2_DB_TRANSACTION_BEGIN, NULL, NULL);

        entryListInt.entryList = entryList;
        entryListInt.setCnt = 0;
        rc = ippiAP2DBExecuteStatement(handle, 0, (const U8*)statement, ippiAP2DBGetDBEntriesCB, &entryListInt);
        ippiAP2DBFreeStatement(statement, 0);
        ippiAP2DBExecuteStatement(handle, 0, (U8 *)IPOD_PLAYER_IAP2_DB_TRANSACTION_COMMIT, NULL, NULL);
    }
    
    return rc;
}

S32 ippiAP2DBGetSelectingCategoryList(IPOD_PLAYER_IAP2_DB_HANDLE *dbHandle, IPOD_PLAYER_IAP2_DB_CATLIST *catList)
{
    S32 rc = IPOD_PLAYER_ERROR;
    sqlite3 *handle = NULL;
    
    if((dbHandle == NULL) || (catList == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, dbHandle, catList);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    handle = dbHandle->iPodHandle;
    rc = ippiAP2DBExecuteStatement(handle, 0, (const U8*)IPOD_PLAYER_IAP2_DB_QUERY_CAT_LIST, ippiAP2DBGetCatListCB, catList);
    
    return rc;
}

S32 ippiAP2DBGetSelectingCategoryCount(IPOD_PLAYER_IAP2_DB_HANDLE *dbHandle, U32 *count)
{
    S32 rc = IPOD_PLAYER_ERROR;
    sqlite3 *handle = NULL;
    U8 *statement = NULL;
    
    /* Parameter check */
    if((dbHandle == NULL) || (count == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, dbHandle, count);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Get handle */
    handle = dbHandle->iPodHandle;
    /* Get count of selecting category */
    statement = ippiAP2DBGenerateGetCountStatement((const U8 *)IPOD_PLAYER_IAP2_DB_TABLE_CATEGORY);
    if(statement != NULL)
    {
        rc = ippiAP2DBExecuteStatement(handle, 0, (U8 *)statement, ippiAP2DBGetCountCallback, count);
        ippiAP2DBFreeStatement(statement, 0);
    }
    
    return rc;
}

S32 ippiAP2DBGetCategoryCount(IPOD_PLAYER_IAP2_DB_HANDLE *dbHandle, IPOD_PLAYER_IAP2_DB_MEDIA_LIBRARY_HANDLE *dbHandleOfMediaLibrary, IPOD_PLAYER_TRACK_TYPE trackType, IPOD_PLAYER_DB_TYPE dbType, IPOD_PLAYER_IAP2_DB_CATLIST *catList)
{
    S32 rc = IPOD_PLAYER_ERROR;
    sqlite3 *handle = NULL;
    U32 num = 0;
    U8 *statement = NULL;
    
    if((dbHandle == NULL) || (dbHandleOfMediaLibrary == NULL) || (catList == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, dbHandle, dbHandleOfMediaLibrary, catList);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    if(trackType == IPOD_PLAYER_TRACK_TYPE_PLAYBACK)
    {
        handle = dbHandle->iPodHandle;
        statement = ippiAP2DBGenerateCountStatement((const U8*)"NowPlaying", dbType, catList);
    }
    else if(trackType ==IPOD_PLAYER_TRACK_TYPE_DATABASE)
    {
        handle = dbHandleOfMediaLibrary->memoryHandle;
        if(dbType == IPOD_PLAYER_DB_TYPE_PLAYLIST)
        {
            statement = ippiAP2DBGenerateCountStatement((const U8*)"Playlist", dbType, catList);
        }
        else if( dbType < IPOD_PLAYER_DB_TYPE_UNKNOWN )
        {
            statement = ippiAP2DBGenerateCountStatement((const U8*)"MediaItem", dbType, catList);
        }
        else
        {
        }
    }
    else if(trackType ==IPOD_PLAYER_TRACK_TYPE_UID)
    {
    }
    
    if(statement != NULL)
    {
        rc = ippiAP2DBExecuteStatement(handle, 0, (U8 *)statement, ippiAP2DBGetCountCallback, &num);
        if(rc == IPOD_PLAYER_OK)
        {
            rc = num;
        }
        ippiAP2DBFreeStatement(statement, 0);
    }
    else
    {
        rc = IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    return rc;
}

S32 ippiAP2DBGetCategoryID(IPOD_PLAYER_IAP2_DB_MEDIA_LIBRARY_HANDLE *dbHandleOfMediaLibrary, IPOD_PLAYER_DB_TYPE type, U32 catIndex, U64 *id, IPOD_PLAYER_IAP2_DB_CATLIST *catList)
{
    S32 rc = IPOD_PLAYER_ERROR;
    sqlite3 *handle = NULL;
    U8 *statement = NULL;
    
    /* Parameter check */
    if((dbHandleOfMediaLibrary == NULL) || (id == NULL) || (catList == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, dbHandleOfMediaLibrary, id, catList);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    handle = dbHandleOfMediaLibrary->memoryHandle;
    statement = ippiAP2DBGenerateGetCategoryIDStatement(type, catIndex, catList);
    if(statement != NULL)
    {
        rc = ippiAP2DBExecuteStatement(handle, 0, (U8 *)statement, ippiAP2DBGetCategoryIDCB, id);
        ippiAP2DBFreeStatement(statement, 0);
    }
    
    return rc;
}

S32 ippiAP2DBSetCategoryID(IPOD_PLAYER_IAP2_DB_HANDLE *dbHandle, IPOD_PLAYER_DB_TYPE type, U64 id)
{
    S32 rc = IPOD_PLAYER_ERROR;
    sqlite3 *handle = NULL;
    U8 *statement = NULL;
    U32 catIndex = 0;
    
    if(dbHandle == NULL)
    {
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    handle = dbHandle->iPodHandle;
    rc = ippiAP2DBExecuteStatement(handle, 0, (U8 *)IPOD_PLAYER_IAP2_DB_QUERY_CAT_COUNT, ippiAP2DBGetCountCallback, &catIndex);
    if(rc == IPOD_PLAYER_OK)
    {
        statement = ippiAP2DBGenerateSetCategoryIDStatement(type, catIndex + 1, id);
        if(statement != NULL)
        {
            rc = ippiAP2DBExecuteStatement(handle, 0, (U8 *)statement, NULL, NULL);
            ippiAP2DBFreeStatement(statement, 0);
        }
    }
    
    return rc;
}

S32 ippiAP2DBDeleteSelect(IPOD_PLAYER_IAP2_DB_HANDLE *dbHandle, IPOD_PLAYER_DB_TYPE type)
{
    S32 rc = IPOD_PLAYER_ERROR;
    sqlite3 *handle = NULL;
    U8 *statement = NULL;
    U32 catIndex = 0;
    char temp[40];
    
    if(dbHandle == NULL)
    {
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    handle = dbHandle->iPodHandle;
    /* get category count */
    rc = ippiAP2DBExecuteStatement(handle, 0, (U8 *)IPOD_PLAYER_IAP2_DB_QUERY_CAT_COUNT, ippiAP2DBGetCountCallback, &catIndex);
    if(rc == IPOD_PLAYER_OK)
    {
        /* generate statement that clear selecting category */
        statement = ippiAP2DBGenerateClearSelectingCategory(type);
        if(statement != NULL)
        {
            /* add selecting category index */
            snprintf(temp, sizeof(temp), IPOD_PLAYER_IAP2_DB_QUERY_LIST_AND_32, IPOD_PLAYER_IAP2_DB_COLUMN_CAT_INDEX, catIndex);
            strcat((char *)statement, temp);

            rc = ippiAP2DBExecuteStatement(handle, 0, (U8 *)statement, NULL, NULL);
            ippiAP2DBFreeStatement(statement, 0);
        }
    }
    
    return rc;
}

S32 ippiAP2DBClearSelectingCategory(IPOD_PLAYER_IAP2_DB_HANDLE *dbHandle, IPOD_PLAYER_DB_TYPE type)
{
    S32 rc = IPOD_PLAYER_ERROR;
    sqlite3 *handle = NULL;
    U8 *statement = NULL;
    
    if(dbHandle == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, dbHandle);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    handle = dbHandle->iPodHandle;
    statement = ippiAP2DBGenerateClearSelectingCategory(type);
    if(statement != NULL)
    {
        rc = ippiAP2DBExecuteStatement(handle, 0, (U8 *)statement, NULL, NULL);
        ippiAP2DBFreeStatement(statement, 0);
    }
    
    return rc;
}

S32 ippiAP2DBGetMediaItemID(IPOD_PLAYER_IAP2_DB_HANDLE *dbHandle, IPOD_PLAYER_IAP2_DB_MEDIA_LIBRARY_HANDLE *dbHandleOfMediaLibrary, IPOD_PLAYER_TRACK_TYPE trackType, IPOD_PLAYER_DB_TYPE dbType, IPOD_PLAYER_IAP2_DB_CATLIST *catList, IPOD_PLAYER_IAP2_DB_IDLIST *idList)
{
    S32 rc = IPOD_PLAYER_ERROR;
    sqlite3 *handle = NULL;
    U8 *statement = NULL;
    
    /* Parameter check */
    if((dbHandle == NULL) || (dbHandleOfMediaLibrary == NULL) || (catList == NULL) || (idList == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, dbHandle, dbHandleOfMediaLibrary, catList, idList);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    if(trackType == IPOD_PLAYER_TRACK_TYPE_PLAYBACK)
    {
        handle = dbHandle->iPodHandle;
        statement = ippiAP2DBGenerateGetMediaItemIDFromNowPlayingStatement(dbType);
    }
    else if(trackType ==IPOD_PLAYER_TRACK_TYPE_DATABASE)
    {
        handle = dbHandleOfMediaLibrary->memoryHandle;
        statement = ippiAP2DBGenerateGetMediaItemIDFromMediaItemStatement(dbType, catList);
    }
    else if(trackType ==IPOD_PLAYER_TRACK_TYPE_UID)
    {
    }
    
    if(statement != NULL)
    {
        rc = ippiAP2DBExecuteStatement(handle, 0, (U8 *)statement, ippiAP2DBGetMediaItemIDCB, idList);
        ippiAP2DBFreeStatement(statement, 0);
    }
    
    return rc;
}

S32 ippiAP2DBSetNowPlayingItemID(IPOD_PLAYER_IAP2_DB_HANDLE *dbHandle, IPOD_PLAYER_IAP2_DB_IDLIST *idList)
{
    int ret = -1;
    S32 rc = IPOD_PLAYER_ERROR;
    sqlite3 *handle = NULL;
    sqlite3_stmt *statement = NULL;
    U32 i = 0;
    
    /* Paramteter check */
    if((dbHandle == NULL) || (idList == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, dbHandle, idList);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    handle = dbHandle->iPodHandle;
    statement = ippiAP2DBGenerateSetNowPlayingItemID(handle);
    if(statement != NULL)
    {
        rc = ippiAP2DBExecuteStatement(handle, 0, (U8 *)IPOD_PLAYER_IAP2_DB_TRANSACTION_BEGIN, NULL, NULL);
        if(rc == IPOD_PLAYER_OK)
        {
            rc = ippiAP2DBExecuteStatement(handle, 0, (U8 *)IPOD_PLAYER_IAP2_DB_DELETE_NOWPLAYING, NULL, NULL);
        }
        
        if(rc == IPOD_PLAYER_OK)
        {
            for(i = 0; i < idList->count; i++)
            {
                ret = sqlite3_reset(statement);
                if(ret == SQLITE_OK)
                {
                    ret = sqlite3_bind_int(statement, 1, i);
                    if(ret == SQLITE_OK)
                    {
                        ret = sqlite3_bind_int64(statement, 2, idList->mediaId[i]);
                        if(ret == SQLITE_OK)
                        {
                            rc = IPOD_PLAYER_OK;
                        }
                    }
                }
                
                if(rc == IPOD_PLAYER_OK)
                {
                    rc = ippiAP2DBExecuteStatement(handle, 1, (U8 *)statement, ippiAP2DBGetMediaItemIDCB, idList);
                }
                else
                {
                    break;
                }
            }
        }
        
        ippiAP2DBExecuteStatement(handle, 0, (U8 *)IPOD_PLAYER_IAP2_DB_TRANSACTION_COMMIT, NULL, NULL);
        ippiAP2DBFreeStatement((U8 *)statement, 1);
    }
    
    return rc;
}

S32 ippiAP2DBGetNowPlayingCount(IPOD_PLAYER_IAP2_DB_HANDLE *dbHandle, U32 *count)
{
    S32 rc = IPOD_PLAYER_ERROR;
    sqlite3 *handle = NULL;
    U8 *statement = NULL;

    /* Parameter check */
    if((dbHandle == NULL) || (count == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, dbHandle, count);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }

    /* Get handle */
    handle = dbHandle->iPodHandle;
    /* Get count of selecting category */
    statement = ippiAP2DBGenerateGetCountStatement((const U8 *)IPOD_PLAYER_IAP2_DB_TABLE_NOWPLAYING);
    if(statement != NULL)
    {
        rc = ippiAP2DBExecuteStatement(handle, 0, (U8 *)statement, ippiAP2DBGetCountCallback, count);
        ippiAP2DBFreeStatement(statement, 0);
    }

    return rc;
}

S32 ippiAP2DBGetMediaLibraryID(IPOD_PLAYER_IAP2_DB_MEDIA_LIBRARY_HANDLE *dbHandleOfMediaLibrary, PUniqueId_t id)
{
    S32 rc = IPOD_PLAYER_ERROR;
    sqlite3 *handle = NULL;
    
    /* Parameter check */
    if((dbHandleOfMediaLibrary == NULL) || (id == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, dbHandleOfMediaLibrary, id);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }

    handle = dbHandleOfMediaLibrary->memoryHandle;
    rc = ippiAP2DBExecuteStatement(handle, 0, (U8 *)IPOD_PLAYER_IAP2_DB_QUERY_MEDIAINFO_ID, ippiAP2DBGetMediaLibraryIDCB, id);
    
    return rc;
}

S32 ippiAP2DBGetQueueCount(IPOD_PLAYER_IAP2_DB_HANDLE *dbHandle, U32 *count)
{
    S32 rc = IPOD_PLAYER_ERROR;
    sqlite3 *handle = NULL;
    
    /* Parameter check */
    if((dbHandle == NULL) || (count == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, dbHandle, (S32 *)count);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    handle = dbHandle->iPodHandle;
    rc = ippiAP2DBExecuteStatement(handle, 0, (U8 *)IPOD_PLAYER_IAP2_DB_QUERY_QUEUE_COUNT, ippiAP2DBGetIntegerCallback, (S32 *)count);
    
    return rc;
}

S32 ippiAP2DBGetShuffle(IPOD_PLAYER_IAP2_DB_HANDLE *dbHandle, IPOD_PLAYER_SHUFFLE_STATUS* shuffle)
{
    S32 rc = IPOD_PLAYER_ERROR;
    sqlite3 *handle = NULL;
    
    /* Parameter check */
    if((dbHandle == NULL) || (shuffle == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, dbHandle, shuffle);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    handle = dbHandle->iPodHandle;
    rc = ippiAP2DBExecuteStatement(handle, 0, (U8 *)IPOD_PLAYER_IAP2_DB_QUERY_SHUFFLE_STATUS, ippiAP2DBGetIntegerCallback, shuffle);
    
    return rc;
}

S32 ippiAP2DBGetRepeat(IPOD_PLAYER_IAP2_DB_HANDLE *dbHandle, IPOD_PLAYER_REPEAT_STATUS* repeat)
{
    S32 rc = IPOD_PLAYER_ERROR;
    sqlite3 *handle = NULL;
    
    /* Parameter check */
    if((dbHandle == NULL) || (repeat == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, dbHandle, repeat);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    handle = dbHandle->iPodHandle;
    rc = ippiAP2DBExecuteStatement(handle, 0, (U8 *)IPOD_PLAYER_IAP2_DB_QUERY_REPEAT_STATUS, ippiAP2DBGetIntegerCallback, repeat);
    
    return rc;
}

S32 ippiAP2DBGetProgress(IPOD_PLAYER_IAP2_DB_MEDIA_LIBRARY_HANDLE *dbHandleOfMediaLibrary, U32 *progress)
{
    S32 rc = IPOD_PLAYER_ERROR;
    sqlite3 *handle = NULL;
    
    /* Parameter check */
    if((dbHandleOfMediaLibrary == NULL) || (progress == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, dbHandleOfMediaLibrary, progress);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    handle = dbHandleOfMediaLibrary->memoryHandle;
    rc = ippiAP2DBExecuteStatement(handle, 0, (U8 *)IPOD_PLAYER_IAP2_DB_QUERY_MEDIAINFO_PROGRESS, ippiAP2DBGetCountCallback, progress);
    
    return rc;
}

S32 ippiAP2DBDeleteMediaItem(IPOD_PLAYER_IAP2_DB_MEDIA_LIBRARY_HANDLE *dbHandleOfMediaLibrary, U16 count, U64 *trackId)
{
    S32 rc = IPOD_PLAYER_ERROR;
    int ret = -1;
    U32 i = 0;
    sqlite3 *handle = NULL;
    sqlite3_stmt *statement = NULL;
    
    /* Parameter check */
    if((dbHandleOfMediaLibrary == NULL) || (trackId == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, dbHandleOfMediaLibrary, trackId);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Set handle */
    handle = dbHandleOfMediaLibrary->memoryHandle;
    /* Generate prepared DELETE statement to delete media item */
    statement = ippiAP2DBGenerateDeleteMediaItemStatement(handle);
    if(statement != NULL)
    {
        /* Begin Transaction */
        rc = ippiAP2DBExecuteStatement(handle, 0, (U8 *)IPOD_PLAYER_IAP2_DB_TRANSACTION_BEGIN, NULL, NULL);
        if(rc == IPOD_PLAYER_OK)
        {
            for(i = 0; i < count; i++)
            {
                /* Reset prepared statement */
                ret = sqlite3_reset(statement);
                if(ret == SQLITE_OK)
                {
                    /* Set the value */
                    ret = sqlite3_bind_int64(statement, 1, trackId[i]);
                    if(ret == SQLITE_OK)
                    {
                        /* Execute statement */
                        rc = ippiAP2DBExecuteStatement(handle, 1, (U8 *)statement, NULL, NULL);
                    }
                    else
                    {
                        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, ret, handle, trackId[i]);
                        rc = IPOD_PLAYER_ERROR;
                    }
                }
                else
                {
                    IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, ret);
                    rc = IPOD_PLAYER_ERROR;
                    break;
                }
            }
            
            /* Commit transaction if begin suceeded */
            ippiAP2DBExecuteStatement(handle, 0, (U8 *)IPOD_PLAYER_IAP2_DB_TRANSACTION_COMMIT, NULL, NULL);
        }
        /* Free statement */
        ippiAP2DBFreeStatement((U8 *)statement, 1);
    }
    else
    {
        rc = IPOD_PLAYER_ERROR;
    }
    
    return rc;
    
}

static S32 ippiAP2DBBindPlaylistStatement(sqlite3_stmt *stmt, iAP2MediaPlayList *playlist)
{
    S32 rc = IPOD_PLAYER_ERROR;
    int ret = -1;
    U32 i = 1;
    
    /* Parameter check */
    if((stmt == NULL) || (playlist == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, stmt, playlist);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    ret = sqlite3_reset(stmt);
    if(ret == SQLITE_OK)
    {
        if(playlist->iAP2MediaPlaylistPersistentIdentifier_count > 0)
        {
            /* Set the first parameter to prepared statement */
            ret = sqlite3_bind_int64(stmt, i, *playlist->iAP2MediaPlaylistPersistentIdentifier);
            i++;
        }
        else
        {
            ret = sqlite3_bind_int64(stmt, i, IPOD_PLAYER_IAP2_DB_MEDIA_ITEM_EMPTY_INT);
            i++;
        }
    }
    
    if(ret == SQLITE_OK)
    {
        if(playlist->iAP2MediaPlaylistName_count > 0)
        {
            /* Set the second parameter to prepared statement */
            ret = sqlite3_bind_text(stmt, i, (const char*)playlist->iAP2MediaPlaylistName[0], -1, NULL);
            i++;
        }
        else
        {
            ret = sqlite3_bind_text(stmt, i, (const char*)IPOD_PLAYER_IAP2_DB_MEDIA_ITEM_EMPTY_TXT, -1, NULL);
            i++;
        }
    }
    
    if(ret == SQLITE_OK)
    {
        if(playlist->iAP2MediaPlaylistParentPersistentIdentifer_count > 0)
        {
            /* Set the second parameter to prepared statement */
            ret = sqlite3_bind_int64(stmt, i, *playlist->iAP2MediaPlaylistParentPersistentIdentifer);
            i++;
        }
        else
        {
            ret = sqlite3_bind_int64(stmt, i, IPOD_PLAYER_IAP2_DB_MEDIA_ITEM_EMPTY_INT);
            i++;
        }
    }
    
    if(ret == SQLITE_OK)
    {
        if(playlist->iAP2MediaPlaylistIsGeniusMix_count > 0)
        {
            /* Set the second parameter to prepared statement */
            ret = sqlite3_bind_int(stmt, i, *playlist->iAP2MediaPlaylistIsGeniusMix);
            i++;
        }
        else
        {
            ret = sqlite3_bind_int(stmt, i, IPOD_PLAYER_IAP2_DB_MEDIA_ITEM_EMPTY_INT);
            i++;
        }
    }
    
    if(ret == SQLITE_OK)
    {
        if(playlist->iAP2MediaPlaylistIsFolder_count > 0)
        {
            /* Set the second parameter to prepared statement */
            ret = sqlite3_bind_int(stmt, i, *playlist->iAP2MediaPlaylistIsFolder);
            i++;
        }
        else
        {
            ret = sqlite3_bind_int(stmt, i, IPOD_PLAYER_IAP2_DB_MEDIA_ITEM_EMPTY_INT);
            i++;
        }
    }
    
    if(ret == SQLITE_OK)
    {
        if(playlist->iAP2MediaPlaylistContainedMediaItemsFileTransferIdentifier_count > 0)
        {
            /* Set the second parameter to prepared statement */
            ret = sqlite3_bind_int(stmt, i, *playlist->iAP2MediaPlaylistContainedMediaItemsFileTransferIdentifier);
            i++;
        }
        else
        {
            ret = sqlite3_bind_int(stmt, i, IPOD_PLAYER_IAP2_DB_MEDIA_ITEM_EMPTY_INT);
            i++;
        }
    }
    
    if(ret == SQLITE_OK)
    {
        if(playlist->iAP2MediaPlaylistIsAppleMusicRadioStation_count > 0)
        {
            /* Set the second parameter to prepared statement */
            ret = sqlite3_bind_int(stmt, i, *playlist->iAP2MediaPlaylistIsAppleMusicRadioStation);
            i++;
        }
        else
        {
            ret = sqlite3_bind_int(stmt, i, IPOD_PLAYER_IAP2_DB_MEDIA_ITEM_EMPTY_INT);
            i++;
        }
    }
    
    if(ret == SQLITE_OK)
    {
        rc = IPOD_PLAYER_OK;
    }
    else
    {
        rc = IPOD_PLAYER_ERROR;
    }
    
    return rc;
}

S32 ippiAP2DBSetPlaylist(IPOD_PLAYER_IAP2_DB_MEDIA_LIBRARY_HANDLE *dbHandleOfMediaLibrary, U16 count, iAP2MediaPlayList *playlist)
{
    S32 rc = IPOD_PLAYER_ERROR;
    U32 i = 0;
    sqlite3 *handle = NULL;
    sqlite3_stmt *statement = NULL;
    
    /* Parameter check */
    if((dbHandleOfMediaLibrary == NULL) || (playlist == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, dbHandleOfMediaLibrary, playlist);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Set handle */
    handle = dbHandleOfMediaLibrary->memoryHandle;
    /* Generate prepared DELETE statement to delete media item */
    statement = ippiAP2DBGenerateSetPlaylistStatement(handle);
    if(statement != NULL)
    {
        /* Begin Transaction */
        rc = ippiAP2DBExecuteStatement(handle, 0, (U8 *)IPOD_PLAYER_IAP2_DB_TRANSACTION_BEGIN, NULL, NULL);
        if(rc == IPOD_PLAYER_OK)
        {
            for(i = 0; i < count; i++)
            {
                rc = ippiAP2DBBindPlaylistStatement(statement, &playlist[i]);
                if(rc == IPOD_PLAYER_OK)
                {
                    /* Execute statement */
                    rc = ippiAP2DBExecuteStatement(handle, 1, (U8 *)statement, NULL, NULL);
                }
            }
            
            /* Commit transaction if begin suceeded */
            ippiAP2DBExecuteStatement(handle, 0, (U8 *)IPOD_PLAYER_IAP2_DB_TRANSACTION_COMMIT, NULL, NULL);
        }
        /* Free statement */
        ippiAP2DBFreeStatement((U8 *)statement, 1);
    }
    else
    {
        rc = IPOD_PLAYER_ERROR;
    }
    
    return rc;
    
}

S32 ippiAP2DBSetIsHiding(IPOD_PLAYER_IAP2_DB_MEDIA_LIBRARY_HANDLE *dbHandleOfMediaLibrary, const U8 *mediaID, U8 isHiding)
{
    S32 rc = IPOD_PLAYER_ERROR;
    sqlite3 *handle = NULL;
    sqlite3_stmt *statement = NULL;
    
    /* Parameter check */
    if((dbHandleOfMediaLibrary == NULL) || (mediaID == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, dbHandleOfMediaLibrary, mediaID);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    handle = dbHandleOfMediaLibrary->memoryHandle;
    statement = ippiAP2DBGenerateSetIsHidingStatement(handle, mediaID, isHiding);
    if(statement != NULL)
    {
        rc = ippiAP2DBExecuteStatement(handle, 1, (U8 *)statement, NULL, NULL);
        ippiAP2DBFreeStatement((U8 *)statement, 1);
    }
    
    return rc;
}

static S32 ippiAP2DBDeletePlaylistTracks(sqlite3 *handle, U64 playlistId)
{
    S32 rc = IPOD_PLAYER_ERROR;
    int ret = -1;
    sqlite3_stmt *statement = NULL;
    
    /* Parameter check */
    if(handle == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, handle);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    statement = ippiAP2DBGenerateDeletePlaylistTracksStatement(handle);
    if(statement != NULL)
    {
        /* Reset prepared statement */
        ret = sqlite3_reset(statement);
        if(ret == SQLITE_OK)
        {
            /* Set the value */
            ret = sqlite3_bind_int64(statement, 1, playlistId);
            if(ret == SQLITE_OK)
            {
                /* Execute statement */
                rc = ippiAP2DBExecuteStatement(handle, 1, (U8 *)statement, NULL, NULL);
            }
            else
            {
                IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, ret, handle,playlistId);
                rc = IPOD_PLAYER_ERROR;
            }
        }
        else
        {
            IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, ret);
            rc = IPOD_PLAYER_ERROR;
        }
        /* Free statement */
        ippiAP2DBFreeStatement((U8 *)statement, 1);
    }
    
    return rc;
}

S32 ippiAP2DBDeletePlaylist(IPOD_PLAYER_IAP2_DB_MEDIA_LIBRARY_HANDLE *dbHandleOfMediaLibrary, U16 count, U64 *playlistId)
{
    S32 rc = IPOD_PLAYER_ERROR;
    int ret = -1;
    U32 i = 0;
    sqlite3 *handle = NULL;
    sqlite3_stmt *statement = NULL;
    
    /* Parameter check */
    if((dbHandleOfMediaLibrary == NULL) || (playlistId == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, dbHandleOfMediaLibrary, playlistId);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Set handle */
    handle = dbHandleOfMediaLibrary->memoryHandle;
    /* Generate prepared DELETE statement to delete media item */
    statement = ippiAP2DBGenerateDeletePlaylistStatement(handle);
    if(statement != NULL)
    {
        /* Begin Transaction */
        rc = ippiAP2DBExecuteStatement(handle, 0, (U8 *)IPOD_PLAYER_IAP2_DB_TRANSACTION_BEGIN, NULL, NULL);
        if(rc == IPOD_PLAYER_OK)
        {
            for(i = 0; i < count; i++)
            {
                /* Reset prepared statement */
                ret = sqlite3_reset(statement);
                if(ret == SQLITE_OK)
                {
                    /* Set the value */
                    ret = sqlite3_bind_int64(statement, 1, playlistId[i]);
                    if(ret == SQLITE_OK)
                    {
                        /* Execute statement */
                        rc = ippiAP2DBExecuteStatement(handle, 1, (U8 *)statement, NULL, NULL);
                    }
                    else
                    {
                        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, ret, handle, playlistId[i]);
                        rc = IPOD_PLAYER_ERROR;
                    }
                }
                else
                {
                    IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, ret);
                    rc = IPOD_PLAYER_ERROR;
                    break;
                }
                
                if(rc == IPOD_PLAYER_OK)
                {
                    rc = ippiAP2DBDeletePlaylistTracks(handle, playlistId[i]);
                }
                
            }
            
            /* Commit transaction if begin suceeded */
            ippiAP2DBExecuteStatement(handle, 0, (U8 *)IPOD_PLAYER_IAP2_DB_TRANSACTION_COMMIT, NULL, NULL);
        }
        /* Free statement */
        ippiAP2DBFreeStatement((U8 *)statement, 1);
    }
    else
    {
        rc = IPOD_PLAYER_ERROR;
    }
    
    return rc;
    
}

S32 ippiAP2DBDeleteAllItems(IPOD_PLAYER_IAP2_DB_MEDIA_LIBRARY_HANDLE *dbHandleOfMediaLibrary)
{
    S32 rc = IPOD_PLAYER_ERROR;
    sqlite3 *handle = NULL;
    
    /* Parameter check */
    if(dbHandleOfMediaLibrary == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, dbHandleOfMediaLibrary);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    handle = dbHandleOfMediaLibrary->memoryHandle;
    rc = ippiAP2DBExecuteStatement(handle, 0, (U8 *)IPOD_PLAYER_IAP2_DB_DELETE_ALL_MEDIA, NULL, NULL);
    if(rc == IPOD_PLAYER_OK)
    {
        rc = ippiAP2DBExecuteStatement(handle, 0, (U8 *)IPOD_PLAYER_IAP2_DB_DELETE_ALL_PLAYLIST, NULL, NULL);
        if(rc == IPOD_PLAYER_OK)
        {
            rc = ippiAP2DBExecuteStatement(handle, 0, (U8 *)IPOD_PLAYER_IAP2_DB_DELETE_ALL_PLAYLIST_TRACKS, NULL, NULL);
        }
    }
    
    return rc;
}

static S32 ippiAP2DBBindPlaylistTracksStatement(sqlite3_stmt *stmt, U32 trackIndex, U64 plyalistId, U64 trackId)
{
    S32 rc = IPOD_PLAYER_ERROR;
    int ret = -1;
    U32 i = 1;
    
    /* Parameter check */
    if(stmt == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, stmt);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    ret = sqlite3_reset(stmt);
    if(ret == SQLITE_OK)
    {
        /* Set the first parameter to prepared statement */
        ret = sqlite3_bind_int(stmt, i, trackIndex);
        if(ret == SQLITE_OK)
        {
            i++;
            /* Set the second parameter to prepared statement */
            ret = sqlite3_bind_int64(stmt, i, plyalistId);
            if(ret == SQLITE_OK)
            {
                i++;
                /* Set the second parameter to prepared statement */
                ret = sqlite3_bind_int64(stmt, i, trackId);
            }
        }
    }
    
    if(ret == SQLITE_OK)
    {
        rc = IPOD_PLAYER_OK;
    }
    else
    {
        rc = IPOD_PLAYER_ERROR;
    }
    
    return rc;
}

S32 ippiAP2DBSetPlaylistTracks(IPOD_PLAYER_IAP2_DB_MEDIA_LIBRARY_HANDLE *dbHandleOfMediaLibrary, U64 playlistId, U16 count, U64 *trackId)
{
    S32 rc = IPOD_PLAYER_ERROR;
    U32 i = 0;
    sqlite3 *handle = NULL;
    sqlite3_stmt *statement = NULL;
    
    /* Parameter check */
    if((dbHandleOfMediaLibrary == NULL) || (trackId == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, dbHandleOfMediaLibrary, trackId);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Set handle */
    handle = dbHandleOfMediaLibrary->memoryHandle;
    /* Generate prepared DELETE statement to delete media item */
    ippiAP2DBDeletePlaylistTracks(handle, playlistId);
    statement = ippiAP2DBGenerateSetPlaylistTracksStatement(handle);
    if(statement != NULL)
    {
        /* Begin Transaction */
        rc = ippiAP2DBExecuteStatement(handle, 0, (U8 *)IPOD_PLAYER_IAP2_DB_TRANSACTION_BEGIN, NULL, NULL);
        if(rc == IPOD_PLAYER_OK)
        {
            for(i = 0; i < count; i++)
            {
                rc = ippiAP2DBBindPlaylistTracksStatement(statement, i, playlistId, trackId[i]);
                if(rc == IPOD_PLAYER_OK)
                {
                    /* Execute statement */
                    rc = ippiAP2DBExecuteStatement(handle, 1, (U8 *)statement, NULL, NULL);
                }
            }
            
            /* Commit transaction if begin suceeded */
            ippiAP2DBExecuteStatement(handle, 0, (U8 *)IPOD_PLAYER_IAP2_DB_TRANSACTION_COMMIT, NULL, NULL);
        }
        /* Free statement */
        ippiAP2DBFreeStatement((U8 *)statement, 1);
    }
    else
    {
        rc = IPOD_PLAYER_ERROR;
    }
    
    return rc;
}

S32 ippiAP2DBSetSample(IPOD_PLAYER_IAP2_DB_HANDLE *dbHandle, U32 rate)
{
    S32 rc = IPOD_PLAYER_ERROR;
    sqlite3 *handle = NULL;
    U8 *statement = NULL;
    
    /* Parameter check */
    if(dbHandle == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, dbHandle);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    handle = dbHandle->iPodHandle;
    statement = ippiAP2DBGenerateSetSampleStatement(rate);
    if(statement != NULL)
    {
        rc = ippiAP2DBExecuteStatement(handle, 0, (U8 *)statement, NULL, NULL);
        ippiAP2DBFreeStatement(statement, 0);
    }
    
    return rc;
}

S32 ippiAP2DBGetSample(IPOD_PLAYER_IAP2_DB_HANDLE *dbHandle, U32 *rate)
{
    S32 rc = IPOD_PLAYER_ERROR;
    sqlite3 *handle = NULL;
    
    /* Parameter check */
    if((dbHandle == NULL) || (rate == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, dbHandle);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    handle = dbHandle->iPodHandle;
    rc = ippiAP2DBExecuteStatement(handle, 0, (U8 *)IPOD_PLAYER_IAP2_DB_QUERY_SAMPLE_RATE, ippiAP2DBGetCountCallback, rate);
    
    return rc;
}

S32 ippiAP2DBSetDeviceName(IPOD_PLAYER_IAP2_DB_MEDIA_LIBRARY_HANDLE *dbHandleOfMediaLibrary, const U8 *key, const U8 *name)
{
    S32 rc = IPOD_PLAYER_ERROR;
    sqlite3 *handle = NULL;
    U8 *statement = NULL;
    
    /* Parameter check */
    if((dbHandleOfMediaLibrary == NULL) || (key == NULL) || (name == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, dbHandleOfMediaLibrary, key, name);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }

    handle = dbHandleOfMediaLibrary->memoryHandle;
    statement = ippiAP2DBGenerateSetDeviceNameStatement(key, name);
    if(statement != NULL)
    {
        rc = ippiAP2DBExecuteStatement(handle, 0, (U8 *)statement, NULL, NULL);
        ippiAP2DBFreeStatement(statement, 0);
    }
    
    return rc;
}

S32 ippiAP2DBGetDeviceName(IPOD_PLAYER_IAP2_DB_MEDIA_LIBRARY_HANDLE *dbHandleOfMediaLibrary, U8 *name)
{
    S32 rc = IPOD_PLAYER_ERROR;
    int ret = -1;

    /* Parameter check */
    if((dbHandleOfMediaLibrary == NULL) || (name == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, dbHandleOfMediaLibrary, name);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }

    if(dbHandleOfMediaLibrary->memoryHandle != NULL)
    {
        ret = sqlite3_exec(dbHandleOfMediaLibrary->memoryHandle, IPOD_PLAYER_IAP2_DB_QUERY_MEDIAINFO_NAME, ippiAP2DBGetStringCallback, name, NULL);
        if(ret == SQLITE_OK)
        {
            rc = IPOD_PLAYER_OK;
        }
        else
        {
            IPOD_DLT_WARN("Could not get device name from memory DB");
            rc = IPOD_PLAYER_ERROR;
        }
    }
    else
    {
        IPOD_DLT_ERROR("Invalid parameter memory DB handle");
        rc = IPOD_PLAYER_ERROR;
    }

    return rc;
}

S32 ippiAP2DBSetAssistiveStatus(IPOD_PLAYER_IAP2_DB_HANDLE *dbHandle, U32 assistiveID, IPOD_PLAYER_DEVICE_EVENT_ASSISTIVE_STATUS assistiveStatus)
{
    S32 rc = IPOD_PLAYER_ERROR;
    sqlite3 *handle = NULL;
    U8 *statement = NULL;
    
    /* Parameter check */
    if(dbHandle == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, dbHandle);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }

    handle = dbHandle->iPodHandle;
    statement = ippiAP2DBGenerateSetAssistiveStatement(assistiveID, assistiveStatus);
    if(statement != NULL)
    {
        rc = ippiAP2DBExecuteStatement(handle, 0, (U8 *)statement, NULL, NULL);
        ippiAP2DBFreeStatement(statement, 0);
    }
    
    return rc;
}

S32 ippiAP2DBGetAssistiveStatus(IPOD_PLAYER_IAP2_DB_HANDLE *dbHandle, U32 assistiveID, IPOD_PLAYER_DEVICE_EVENT_ASSISTIVE_STATUS *assistiveStatus)
{
    S32 rc = IPOD_PLAYER_ERROR;
    U8 *statement = NULL;
    sqlite3 *handle = NULL;
    
    /* Parameter check */
    if((dbHandle == NULL) || (assistiveStatus == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, dbHandle);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    statement = ippiAP2DBGenerateGetAssistiveStatusStatement(assistiveID);
    handle = dbHandle->iPodHandle;
    rc = ippiAP2DBExecuteStatement(handle, 0, (U8 *)statement, ippiAP2DBGetIntegerCallback, assistiveStatus);
    ippiAP2DBFreeStatement(statement, 0);
    
    return rc;
}

S32 ippiAP2DBGetiOSAppName(IPOD_PLAYER_IAP2_DB_HANDLE *dbHandle, U32 length, U8 *name)
{
    S32 rc = IPOD_PLAYER_ERROR;
    ippiAP2DBString_t appName;
    sqlite3 *handle = NULL;
    
    /* Parameter check */
    if((dbHandle == NULL) || (name == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, dbHandle, name);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    appName.str = name;
    appName.len = length;
    handle = dbHandle->iPodHandle;
    rc = ippiAP2DBExecuteStatement(handle, 0, (U8 *)IPOD_PLAYER_IAP2_DB_QUERY_IOSAPP_NAME, ippiAP2DBGetStringCB, &appName);
    
    return rc;
}

S32 ippiAP2DBGetTrackIDListFromNowPlaying(IPOD_PLAYER_IAP2_DB_HANDLE *dbHandle, U64 trackIndex, U32 count, IPOD_PLAYER_IAP2_DB_IDLIST *idList)
{
    S32 rc = IPOD_PLAYER_ERROR;
    U8 *statement = NULL;
    sqlite3 *handle = NULL;
    
    /* Parameter check */
    if((dbHandle == NULL) || (idList == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, dbHandle, idList);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }

    handle = dbHandle->iPodHandle;
    statement = ippiAP2DBGenerateGetTrackIDListFromNowPlayingStatement(trackIndex, count);
    if(statement != NULL)
    {
        idList->count = count;
        rc = ippiAP2DBExecuteStatement(handle, 0, (U8 *)statement, ippiAP2DBGetMediaItemIDCB, idList);
        ippiAP2DBFreeStatement(statement, 0);
    }
    
    return rc;
}

void ippiAP2DBSetCapability(S32 mediaType, U32 *capability)
{
    if((mediaType >= 0) && (mediaType <= 2) && (capability != NULL))
    {
        U32 CapaTable[3] = {
            0,                                  /* songs */
            IPOD_PLAYER_CAP_MASK_IS_PODCAST,    /* podcast */
            IPOD_PLAYER_CAP_MASK_IS_AUDIOBOOK   /* audioBook */
        };

        *capability |= CapaTable[mediaType];
    }
    else
    {
        //IPOD_DLT_WARN("[DBG]Invalid parameter. :mediaType=%d, capability=%p", mediaType, capability);
    }
}

static int ippiAP2DBGetTrackInfoListCB(void *arg, int cnt, char ** argv, char **column)
{
    int ret = -1;
    IPOD_PLAYER_IAP2_DB_TRACKLIST *trackList = NULL;
    U32 i = 0;
    static U32 trackIndex = 0;
    U32 reqMask = 0;
    U32 retMask = 0;
    
    /* Parameter check */
    if((arg == NULL) || (argv == NULL) || (column == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, arg, argv, column);
        return SQLITE_ERROR;
    }
    
    trackList = (IPOD_PLAYER_IAP2_DB_TRACKLIST *)arg;
    reqMask = trackList->trackInfo[trackIndex].trackInfoMask;
    //IPOD_DLT_INFO("[DBG]arg=%p, cnt=%d, reqMask=%x(%u)", arg, cnt, reqMask, reqMask);
    
    if(trackIndex < trackList->count)
    {
        for(i = 0; i < (U32)cnt; i++)
        {
            if(argv[i] != NULL)
            {
                //IPOD_DLT_INFO("[DBG]i=%u, column[i]=%s, argv[i]=%s", i, column[i], argv[i]);
                if(((reqMask & IPOD_PLAYER_TRACK_INFO_MASK_TRACK_NAME) == IPOD_PLAYER_TRACK_INFO_MASK_TRACK_NAME) &&
                   (strncmp((const char *)IPOD_PLAYER_IAP2_DB_COLUMN_TRACKTITLE, (const char*)column[i], strlen((const char*)IPOD_PLAYER_IAP2_DB_COLUMN_TRACKTITLE)) == 0))
                {
                    strncpy((char *)trackList->trackInfo[trackIndex].trackName, (const char*)argv[i], sizeof(trackList->trackInfo[trackIndex].trackName));
                    trackList->trackInfo[trackIndex].trackName[sizeof(trackList->trackInfo[trackIndex].trackName) - 1] = '\0';
                    retMask |= IPOD_PLAYER_TRACK_INFO_MASK_TRACK_NAME;
                }
                if(((reqMask & IPOD_PLAYER_TRACK_INFO_MASK_ALBUM_NAME) == IPOD_PLAYER_TRACK_INFO_MASK_ALBUM_NAME) &&
                   (strncmp((const char *)IPOD_PLAYER_IAP2_DB_COLUMN_ALBUMTITLE, (const char*)column[i], strlen((const char*)IPOD_PLAYER_IAP2_DB_COLUMN_ALBUMTITLE)) == 0))
                {
                    strncpy((char *)trackList->trackInfo[trackIndex].albumName, (const char*)argv[i], sizeof(trackList->trackInfo[trackIndex].albumName));
                    trackList->trackInfo[trackIndex].albumName[sizeof(trackList->trackInfo[trackIndex].albumName) - 1] = '\0';
                    retMask |= IPOD_PLAYER_TRACK_INFO_MASK_ALBUM_NAME;
                }
                if(((reqMask & IPOD_PLAYER_TRACK_INFO_MASK_ARTIST_NAME) == IPOD_PLAYER_TRACK_INFO_MASK_ARTIST_NAME) &&
                   (strncmp((const char *)IPOD_PLAYER_IAP2_DB_COLUMN_ARTIST, (const char*)column[i], strlen((const char*)IPOD_PLAYER_IAP2_DB_COLUMN_ARTIST)) == 0))
                {
                    strncpy((char *)trackList->trackInfo[trackIndex].artistName, (const char*)argv[i], sizeof(trackList->trackInfo[trackIndex].artistName));
                    trackList->trackInfo[trackIndex].artistName[sizeof(trackList->trackInfo[trackIndex].artistName) - 1] = '\0';
                    retMask |= IPOD_PLAYER_TRACK_INFO_MASK_ARTIST_NAME;
                }
                if(((reqMask & IPOD_PLAYER_TRACK_INFO_MASK_GENRE) == IPOD_PLAYER_TRACK_INFO_MASK_GENRE) &&
                   (strncmp((const char *)IPOD_PLAYER_IAP2_DB_COLUMN_GENRE, (const char*)column[i], strlen((const char*)IPOD_PLAYER_IAP2_DB_COLUMN_GENRE)) == 0))
                {
                    strncpy((char *)trackList->trackInfo[trackIndex].genre, (const char*)argv[i], sizeof(trackList->trackInfo[trackIndex].genre));
                    trackList->trackInfo[trackIndex].genre[sizeof(trackList->trackInfo[trackIndex].genre) - 1] = '\0';
                    retMask |= IPOD_PLAYER_TRACK_INFO_MASK_GENRE;
                }
                if(((reqMask & IPOD_PLAYER_TRACK_INFO_MASK_COMPOSER) == IPOD_PLAYER_TRACK_INFO_MASK_COMPOSER) &&
                   (strncmp((const char *)IPOD_PLAYER_IAP2_DB_COLUMN_COMPOSER, (const char*)column[i], strlen((const char*)IPOD_PLAYER_IAP2_DB_COLUMN_COMPOSER)) == 0))
                {
                    strncpy((char *)trackList->trackInfo[trackIndex].composer, (const char*)argv[i], sizeof(trackList->trackInfo[trackIndex].composer));
                    trackList->trackInfo[trackIndex].composer[sizeof(trackList->trackInfo[trackIndex].composer) - 1] = '\0';
                    retMask |= IPOD_PLAYER_TRACK_INFO_MASK_COMPOSER;
                }
                if(((reqMask & IPOD_PLAYER_TRACK_INFO_MASK_TRACKLENGTH) == IPOD_PLAYER_TRACK_INFO_MASK_TRACKLENGTH) &&
                   (strncmp((const char *)IPOD_PLAYER_IAP2_DB_COLUMN_MEDIADURATIONMS, (const char*)column[i], strlen((const char*)IPOD_PLAYER_IAP2_DB_COLUMN_MEDIADURATIONMS)) == 0))
                {
                    trackList->trackInfo[trackIndex].length = atoll((const char*)argv[i]);
                    retMask |= IPOD_PLAYER_TRACK_INFO_MASK_TRACKLENGTH;
                }
                if(((reqMask & IPOD_PLAYER_TRACK_INFO_MASK_CHAPTER_COUNT) == IPOD_PLAYER_TRACK_INFO_MASK_CHAPTER_COUNT) &&
                   (strncmp((const char *)IPOD_PLAYER_IAP2_DB_COLUMN_CHAPTERCOUNT, (const char*)column[i], strlen((const char*)IPOD_PLAYER_IAP2_DB_COLUMN_CHAPTERCOUNT)) == 0))
                {
                    trackList->trackInfo[trackIndex].chapterCount = atoll((const char*)argv[i]);
                    retMask |= IPOD_PLAYER_TRACK_INFO_MASK_CHAPTER_COUNT;
                }
                if(((reqMask & IPOD_PLAYER_TRACK_INFO_MASK_CAPABILITY) == IPOD_PLAYER_TRACK_INFO_MASK_CAPABILITY) &&
                   (strncmp((const char *)IPOD_PLAYER_IAP2_DB_COLUMN_TRACKID, (const char*)column[i], strlen((const char*)IPOD_PLAYER_IAP2_DB_COLUMN_TRACKID)) == 0))
                {
                    trackList->trackInfo[trackIndex].trackID = atoll((const char*)argv[i]);
                    retMask |= IPOD_PLAYER_TRACK_INFO_MASK_CAPABILITY;
                }
                if(((reqMask & IPOD_PLAYER_TRACK_INFO_MASK_CAPABILITY) == IPOD_PLAYER_TRACK_INFO_MASK_CAPABILITY) &&
                   (strncmp((const char *)IPOD_PLAYER_IAP2_DB_COLUMN_MEDIATYPE, (const char*)column[i], strlen((const char*)IPOD_PLAYER_IAP2_DB_COLUMN_MEDIATYPE)) == 0))
                {
                    trackList->trackInfo[trackIndex].mediaType = atoll((const char*)argv[i]);
                    retMask |= IPOD_PLAYER_TRACK_INFO_MASK_CAPABILITY;
                }
            }
        }
    }
    
    trackList->trackInfo[trackIndex].trackInfoMask = retMask;
    trackIndex++;
    if(trackIndex >= trackList->count)
    {
        trackIndex = 0;
    }
    
    ret = SQLITE_OK;
    
    return ret;
}

static int ippiAP2DBGetMediaTypeCB(void *arg, int cnt, char ** argv, char **column)
{
    S32 *mediaType = NULL;
    int i = 0;
    
    /* Parameter check */
    if((arg == NULL) || (argv == NULL) || (column == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, arg, argv, column);
        return SQLITE_ERROR;
    }
    
    mediaType = (S32 *)arg;
    
    for(i = 0; i < cnt; i++)
    {
        if(argv[i] != NULL)
        {
            if(strncmp((const char *)IPOD_PLAYER_IAP2_DB_MEDIAINFO_COLUMN_MEDIATYPE, (const char*)column[i],
                                                strlen((const char*)IPOD_PLAYER_IAP2_DB_MEDIAINFO_COLUMN_MEDIATYPE)) == 0)
            {
                *mediaType = atoi(argv[i]);
            }
        }
    }
    
    return SQLITE_OK;
}

S32 ippiAP2DBGetTrackInfoList(IPOD_PLAYER_IAP2_DB_HANDLE *dbHandle, IPOD_PLAYER_IAP2_DB_MEDIA_LIBRARY_HANDLE *dbHandleOfMediaLibrary, IPOD_PLAYER_TRACK_TYPE type, U32 trackInfoMask, U64 trackIndex, U32 count, IPOD_PLAYER_IAP2_DB_TRACKLIST *trackList, U32 mediaType)
{
    S32 rc = IPOD_PLAYER_ERROR;
    U32 i = 0;
    sqlite3 *handle = NULL;
    U8 *statement = NULL;
    IPOD_PLAYER_IAP2_DB_IDLIST idList;
    IPOD_PLAYER_IAP2_DB_TRACKLIST SingleTrackList;
    IPOD_PLAYER_TRACK_INFO trackInfo;

    /* Parameter check */
    if((dbHandle == NULL) || (dbHandleOfMediaLibrary == NULL) || (trackList == NULL) || (type > IPOD_PLAYER_TRACK_TYPE_UID))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, dbHandle, dbHandleOfMediaLibrary, trackList, type);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }

    /* set trackInfo */
    trackList->count = count;

    for(i = 0; i < count; i++)
    {
        trackList->trackInfo[i].trackInfoMask = trackInfoMask;
    }
    
    memset(&idList, 0, sizeof(idList));
    handle = dbHandleOfMediaLibrary->memoryHandle;
    if(type == IPOD_PLAYER_TRACK_TYPE_PLAYBACK)
    {
        idList.mediaId = calloc(count, sizeof(U64));
        if(idList.mediaId != NULL)
        {
            rc = ippiAP2DBGetTrackIDListFromNowPlaying(dbHandle, trackIndex, count, &idList);
            if(rc == IPOD_PLAYER_OK)
            {
                SingleTrackList.trackInfo = &trackInfo;
                SingleTrackList.count = 1;
                for(i = 0; i < count; i++)
                {
                    statement = ippiAP2DBGenerateGetTrackInfoTrackIDStatement(&(idList.mediaId[i]));
                    memcpy(&trackInfo, &(trackList->trackInfo[i]), sizeof(IPOD_PLAYER_TRACK_INFO));
                    trackInfo.mediaType = -1;
                    trackInfo.capa = 0;

                    rc = ippiAP2DBExecuteStatement(handle, 0, (U8 *)statement, ippiAP2DBGetTrackInfoListCB, &SingleTrackList);
                    ippiAP2DBFreeStatement(statement, 0);
                    if(rc == IPOD_PLAYER_OK)
                    {
                        ippiAP2DBSetCapability(trackInfo.mediaType, &(trackInfo.capa));
                        memcpy(&(trackList->trackInfo[i]), &trackInfo, sizeof(IPOD_PLAYER_TRACK_INFO));
                    }
                    else
                    {
                        IPOD_DLT_ERROR("Could not get track info DB by TrackID handle = %p count = %d", handle, i);
                        rc = IPOD_PLAYER_ERROR;
                        break;
                    }
                }
            }
        }
        else
        {
            rc = IPOD_PLAYER_ERR_NOMEM;
        }
    }
    else if(type == IPOD_PLAYER_TRACK_TYPE_DATABASE)
    {
        statement = ippiAP2DBGenerateGetTrackInfoListStatement(dbHandle, trackIndex, count, mediaType);
        if(statement != NULL)
        {
            for(i = 0; i < trackList->count; i++)
            {
                trackList->trackInfo[i].mediaType = -1;
                trackList->trackInfo[i].capa = 0;
            }
            rc = ippiAP2DBExecuteStatement(handle, 0, (U8 *)statement, ippiAP2DBGetTrackInfoListCB, trackList);
            ippiAP2DBFreeStatement(statement, 0);
            if(rc == IPOD_PLAYER_OK)
            {
                for(i = 0; i < trackList->count; i++)
                {
                    ippiAP2DBSetCapability(trackList->trackInfo[i].mediaType, &(trackList->trackInfo[i].capa));
                }
            }
            else
            {
                IPOD_DLT_ERROR("Could not get track info DB by TrackID handle = %p", handle);
                rc = IPOD_PLAYER_ERROR;
            }
        }
        else
        {
            IPOD_DLT_ERROR("Could not get resource to use access DB of playback list.(media type = %d)", mediaType);
            rc = IPOD_PLAYER_ERR_NOMEM;
        }
    }
    else
    {
        rc = IPOD_PLAYER_ERR_NOT_SUPPORT;
    }
    
    if(idList.mediaId != NULL)
    {
        free(idList.mediaId);
        idList.mediaId = NULL;
    }
    
    return rc;
}

S32 ippiAP2DBGetMediaType(IPOD_PLAYER_IAP2_DB_MEDIA_LIBRARY_HANDLE *dbHandleOfMediaLibrary, U64 trackID, S32 *mediaType)
{
    S32 rc = IPOD_PLAYER_ERROR;
    sqlite3 *handle = NULL;
    U8 *statement = NULL;
    
    /* Parameter check */
    if((dbHandleOfMediaLibrary == NULL) || (mediaType == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, dbHandleOfMediaLibrary, mediaType);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }

    handle = dbHandleOfMediaLibrary->memoryHandle;
    statement = ippiAP2DBGenerateGetMediaTypeStatement(&trackID);
    rc = ippiAP2DBExecuteStatement(handle, 0, (U8 *)statement, ippiAP2DBGetMediaTypeCB, mediaType);
    ippiAP2DBFreeStatement(statement, 0);
    if(rc != IPOD_PLAYER_OK)
    {
        IPOD_DLT_ERROR("Could not get media type DB by TrackID handle = %p", handle);
    }
    
    return rc;
}

S32 ippiAP2DBGetNowPlayingUpdateTrackInfo(IPOD_PLAYER_IAP2_DB_HANDLE *dbHandle, U32 trackInfoMask, IPOD_PLAYER_TRACK_INFO *trackInfo)
{
    S32 rc = IPOD_PLAYER_ERROR;
    sqlite3 *handle = NULL;
    IPOD_PLAYER_IAP2_DB_TRACKLIST trackList;
    
    /* Parameter check */
    if((dbHandle == NULL) || (trackInfo == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, dbHandle, trackInfo);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    memset(&trackList, 0, sizeof(trackList));
    trackList.trackInfo = trackInfo;
    trackList.count = 1;
    trackList.trackInfo[0].trackInfoMask = trackInfoMask;
    
    handle = dbHandle->iPodHandle;
    rc = ippiAP2DBExecuteStatement(handle, 0, (U8 *)IPOD_PLAYER_IAP2_DB_QUERY_TRACKINFO_FROM_PLAYINGITEM, ippiAP2DBGetTrackInfoListCB, &trackList);
    
    return rc;
}

/* Get track count by type */
S32 ippiAP2DBGetTrackTotalCount(IPOD_PLAYER_IAP2_DB_HANDLE *dbHandle, IPOD_PLAYER_IAP2_DB_MEDIA_LIBRARY_HANDLE *dbHandleOfMediaLibrary, IPOD_PLAYER_TRACK_TYPE type, U32 *count, U32 *mediaType)
{
    S32 rc = IPOD_PLAYER_ERROR;                        /* for return code  */
    U32 num = 0;
    IPOD_PLAYER_IAP2_DB_CATLIST catList;
   
    /* Note: mediaType may be NULL */
    if((dbHandle == NULL) || (dbHandleOfMediaLibrary == NULL) || (count == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, dbHandle, dbHandleOfMediaLibrary, count);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Initialize the structure */
    memset(&catList, 0, sizeof(catList));
    
    rc = ippiAP2DBGetSelectingCategoryCount(dbHandle, &num);
    if(rc == IPOD_PLAYER_OK)
    {
        if(num > 0)
        {
            catList.categories = calloc(num, sizeof(*catList.categories));
            if(catList.categories != NULL)
            {
                catList.count = num;
                rc = ippiAP2DBGetSelectingCategoryList(dbHandle, &catList);
            }
        }
    }
    
    /* Get Category count */
    if(rc == IPOD_PLAYER_OK)
    {
        rc = ippiAP2DBGetCategoryCount(dbHandle, dbHandleOfMediaLibrary, type, IPOD_PLAYER_DB_TYPE_TRACK, &catList);
        if(rc >= 0)
        {
            *count = rc;
            rc = IPOD_PLAYER_OK;
        }
    }

    /* Set Media Type */
    if(mediaType != NULL)
    {
        *mediaType = ippiAP2GetMediaType(&catList);
    }
    
    if(catList.categories != NULL)
    {
        free(catList.categories);
    }
    
    return rc;
}

S32 ippiAP2DBSetBluetoothStatus(IPOD_PLAYER_IAP2_DB_HANDLE *dbHandle, IAP2_BLUETOOTH_INFO *btInfo)
{
    S32 rc = IPOD_PLAYER_ERROR;
    U8 *statement = NULL;
    sqlite3 *handle = NULL;
    
    /* Parameter check */
    if((dbHandle == NULL) || (btInfo == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, dbHandle, btInfo);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }

    handle = dbHandle->iPodHandle;
    statement = ippiAP2DBGenerateSetBluetoothStatusStatement(btInfo);
    if(statement != NULL)
    {
        rc = ippiAP2DBExecuteStatement(handle, 0, (U8 *)statement, NULL, NULL);
        ippiAP2DBFreeStatement(statement, 0);
    }
    
    return rc;
}

S32 ippiAP2DBGetBluetoothStatus(IPOD_PLAYER_IAP2_DB_HANDLE *dbHandle, U16 btID, U32 *profile)
{
    S32 rc = IPOD_PLAYER_ERROR;
    U8 *statement = NULL;
    sqlite3 *handle = NULL;
    
    /* Parameter check */
    if((dbHandle == NULL) || (profile == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, dbHandle, profile);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }

    handle = dbHandle->iPodHandle;
    statement = ippiAP2DBGenerateGetBluetoothStatusStatement(btID);
    if(statement != NULL)
    {
        rc = ippiAP2DBExecuteStatement(handle, 0, (U8 *)statement, ippiAP2DBGetCountCallback, profile);
        ippiAP2DBFreeStatement(statement, 0);
    }
    
    return rc;
}

S32 ippiAP2DBGetSelectingCategories(IPOD_PLAYER_IAP2_DB_HANDLE *dbHandle, IPOD_PLAYER_IAP2_DB_CATLIST **curList)
{
    S32 rc = IPOD_PLAYER_ERROR;
    IPOD_PLAYER_IAP2_DB_CATLIST *catList = NULL;
    U32 count = 0;
    
    /* Parameter check */
    if((dbHandle == NULL) || (curList == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, dbHandle, curList);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    catList = calloc(1, sizeof(*catList));
    if(catList == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_NOMEM, catList);
        return IPOD_PLAYER_ERR_NOMEM;
    }
    
    /* Get current selecting count */
    rc = ippiAP2DBGetSelectingCategoryCount(dbHandle, &count);
    if(rc == IPOD_PLAYER_OK)
    {
        if(count > 0)
        {
            /* Allocate for number of category list */
            catList->categories = calloc(count, sizeof(*catList->categories));
            if(catList->categories != NULL)
            {
                catList->count = count;
                /* Get category list */
                rc = ippiAP2DBGetSelectingCategoryList(dbHandle, catList);
            }
            else
            {
                IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_NOMEM, dbHandle);
                rc = IPOD_PLAYER_ERR_NOMEM;
            }
        }
    }
    
    if(rc == IPOD_PLAYER_OK)
    {
        *curList = catList;
    }
    else
    {
        if(catList->categories != NULL)
        {
            free(catList->categories);
            catList->categories = NULL;
        }
        free(catList);
        catList = NULL;
    }
    
    return rc;
}

void ippiAP2DBFreeSelectingCategories(IPOD_PLAYER_IAP2_DB_CATLIST *curList)
{
    if(curList == NULL)
    {
        return;
    }
    
    if(curList->categories != NULL)
    {
        free(curList->categories);
    }
    
    free(curList);
    
    return;
}
