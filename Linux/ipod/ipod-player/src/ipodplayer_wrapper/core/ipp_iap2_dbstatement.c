#include "ipp_iap2_database.h"
#include "ipp_iap2_dbstatement.h"
#include "iPodPlayerCoreCfg.h"

S32 ippiAP2CreateDBStatement(IPOD_PLAYER_DB_TYPE type, U8 **statement, U8 **orderstate, U16 length, U32 *mediaType, IPOD_PLAYER_IAP2_DB_CATLIST *catList)
{
    S32 rc = IPOD_PLAYER_OK;

    switch(type)
    {
    case IPOD_PLAYER_DB_TYPE_PLAYLIST:
        strncat((char *)*statement, (const char*)IPOD_PLAYER_IAP2_DB_QUERY_LIST_SELECT_PLAYLIST, length);
        *orderstate = (U8 *)IPOD_PLAYER_IAP2_DB_ORDER_PLAYLIST;
        break;
        
    case IPOD_PLAYER_DB_TYPE_ARTIST:
        strncat((char *)*statement, (const char*)IPOD_PLAYER_IAP2_DB_QUERY_LIST_SELECT_ARTIST, length);
        *orderstate = (U8 *)IPOD_PLAYER_IAP2_DB_ORDER_ARTIST;
        break;
        
    case IPOD_PLAYER_DB_TYPE_ALBUM:
        strncat((char *)*statement, (const char*)IPOD_PLAYER_IAP2_DB_QUERY_LIST_SELECT_ALBUM, length);
        *orderstate = (U8 *)IPOD_PLAYER_IAP2_DB_ORDER_ALBUM;
        break;
        
    case IPOD_PLAYER_DB_TYPE_GENRE:
        strncat((char *)*statement, (const char*)IPOD_PLAYER_IAP2_DB_QUERY_LIST_SELECT_GENRE, length);
        *orderstate = (U8 *)IPOD_PLAYER_IAP2_DB_ORDER_GENRE;
        break;
    
    case IPOD_PLAYER_DB_TYPE_TRACK:
        /* The category list is checked playlist category */
        if(ippiAP2DBCheckCategory(catList, "PlaylistID"))
        {
            strncat((char *)*statement, (const char*)IPOD_PLAYER_IAP2_DB_QUERY_LIST_SELECT_TRACK_FROM_PLAYLIST, length);
            *orderstate = (U8 *)IPOD_PLAYER_IAP2_DB_ORDER_PLAYLISTID_TRACKINDEX;
        }
        else
        {
            strncat((char *)*statement, (const char*)IPOD_PLAYER_IAP2_DB_QUERY_LIST_SELECT_TRACK, length);
            if(ippiAP2DBCheckCategory(catList, "AlbumID"))
            {
                *orderstate = (U8 *)IPOD_PLAYER_IAP2_DB_ORDER_ALBUMTRACKCOUNT_TRACK;
            }
            else
            {
                *orderstate = (U8 *)IPOD_PLAYER_IAP2_DB_ORDER_TRACK;
            }

            /* The category list is checked other than music category */
            if(!ippiAP2DBCheckCategory(catList, "MediaType"))
            {
                *mediaType = IPP_MEDIA_TYPE_MUSIC;   /* set Music type */
            }
        }
        break;
        
    case IPOD_PLAYER_DB_TYPE_COMPOSER:
        strncat((char *)*statement, (const char*)IPOD_PLAYER_IAP2_DB_QUERY_LIST_SELECT_COMPOSER, length);
        *orderstate = (U8 *)IPOD_PLAYER_IAP2_DB_ORDER_COMPOSER;
        break;
        
    case IPOD_PLAYER_DB_TYPE_AUDIOBOOK:
        strncat((char *)*statement, (const char*)IPOD_PLAYER_IAP2_DB_QUERY_LIST_SELECT_TRACK, length);
        *orderstate = (U8 *)IPOD_PLAYER_IAP2_DB_ORDER_TRACK;
        *mediaType = IPP_MEDIA_TYPE_AUDIOBOOK;   /* set Audiobook type */
        break;
        
    case IPOD_PLAYER_DB_TYPE_PODCAST:
        strncat((char *)*statement, (const char*)IPOD_PLAYER_IAP2_DB_QUERY_LIST_SELECT_PODCAST, length);
        *orderstate = (U8 *)IPOD_PLAYER_IAP2_DB_ORDER_ALBUM;
        *mediaType = IPP_MEDIA_TYPE_PODCAST;     /* set Podcast type */
        break;
        
    case IPOD_PLAYER_DB_TYPE_ITUNESU:
        strncat((char *)*statement, (const char*)IPOD_PLAYER_IAP2_DB_QUERY_LIST_SELECT_TRACK, length);
        *orderstate = (U8 *)IPOD_PLAYER_IAP2_DB_ORDER_TRACK;
        *mediaType = IPP_MEDIA_TYPE_ITUNESU;     /* set iTunesU type */
        break;
        
    /* Todo */
    case IPOD_PLAYER_DB_TYPE_ALL:
    case IPOD_PLAYER_DB_TYPE_NESTED_PLAYLIST:
    case IPOD_PLAYER_DB_TYPE_INTELLIGENT:
    default:
        rc = IPOD_PLAYER_ERR_INVALID_PARAMETER;
        break;
    }

    return rc;
}


void ippiAP2GenerateStatement(U32 length, U8 *statement, iAP2PlaybackAttributes *item)
{
    strncat((char *)statement, (const char*)IPOD_PLAYER_IAP2_DB_IPODINFO_UPDATE_PREPARE_START, length);
    
    strncat((char *)statement, (const char*)IPOD_PLAYER_IAP2_DB_COLUMN_SPEED, length);
    strncat((char *)statement, (const char*)IPOD_PLAYER_IAP2_DB_PREPARE_EQUALE, length);
    strncat((char *)statement, (const char*)IPOD_PLAYER_IAP2_DB_PREPARE_PARAM, length);
    
    if(item->iAP2PlaybackStatus_count > 0)
    {
        strncat((char *)statement, (const char*)IPOD_PLAYER_IAP2_DB_PREPARE_COMMA, length);
        strncat((char *)statement, (const char*)IPOD_PLAYER_IAP2_DB_COLUMN_PLAYBACKSTATUS, length);
        strncat((char *)statement, (const char*)IPOD_PLAYER_IAP2_DB_PREPARE_EQUALE, length);
        strncat((char *)statement, (const char*)IPOD_PLAYER_IAP2_DB_PREPARE_PARAM, length);
    }
    
    if(item->iAP2PlaybackShuffleMode_count > 0)
    {
        strncat((char *)statement, (const char*)IPOD_PLAYER_IAP2_DB_PREPARE_COMMA, length);
        strncat((char *)statement, (const char*)IPOD_PLAYER_IAP2_DB_COLUMN_SHUFFLE, length);
        strncat((char *)statement, (const char*)IPOD_PLAYER_IAP2_DB_PREPARE_EQUALE, length);
        strncat((char *)statement, (const char*)IPOD_PLAYER_IAP2_DB_PREPARE_PARAM, length);
    }
    
    if(item->iAP2PlaybackRepeatMode_count > 0)
    {
        strncat((char *)statement, (const char*)IPOD_PLAYER_IAP2_DB_PREPARE_COMMA, length);
        strncat((char *)statement, (const char*)IPOD_PLAYER_IAP2_DB_COLUMN_REPEAT, length);
        strncat((char *)statement, (const char*)IPOD_PLAYER_IAP2_DB_PREPARE_EQUALE, length);
        strncat((char *)statement, (const char*)IPOD_PLAYER_IAP2_DB_PREPARE_PARAM, length);
    }
    
    if(item->iAP2PlaybackAppName_count > 0)
    {
        strncat((char *)statement, (const char*)IPOD_PLAYER_IAP2_DB_PREPARE_COMMA, length);
        strncat((char *)statement, (const char*)IPOD_PLAYER_IAP2_DB_COLUMN_APPNAME, length);
        strncat((char *)statement, (const char*)IPOD_PLAYER_IAP2_DB_PREPARE_EQUALE, length);
        strncat((char *)statement, (const char*)IPOD_PLAYER_IAP2_DB_PREPARE_PARAM, length);
    }

    if(item->iAP2PlaybackAppBundleID_count > 0)
    {
        strncat((char *)statement, (const char*)IPOD_PLAYER_IAP2_DB_PREPARE_COMMA, length);
        strncat((char *)statement, (const char*)IPOD_PLAYER_IAP2_DB_COLUMN_APPBUNDLEID, length);
        strncat((char *)statement, (const char*)IPOD_PLAYER_IAP2_DB_PREPARE_EQUALE, length);
        strncat((char *)statement, (const char*)IPOD_PLAYER_IAP2_DB_PREPARE_PARAM, length);
    }

    if(item->iAP2PlaybackElapsedTimeInMilliseconds_count> 0)
    {
        strncat((char *)statement, (const char*)IPOD_PLAYER_IAP2_DB_PREPARE_COMMA, length);
        strncat((char *)statement, (const char*)IPOD_PLAYER_IAP2_DB_COLUMN_ELAPSEDTIME, length);
        strncat((char *)statement, (const char*)IPOD_PLAYER_IAP2_DB_PREPARE_EQUALE, length);
        strncat((char *)statement, (const char*)IPOD_PLAYER_IAP2_DB_PREPARE_PARAM, length);
    }

    if(item->iAP2PBMediaLibraryUniqueIdentifier_count > 0)
    {
        strncat((char *)statement, (const char*)IPOD_PLAYER_IAP2_DB_PREPARE_COMMA, length);
        strncat((char *)statement, (const char*)IPOD_PLAYER_IAP2_DB_COLUMN_MEDIALIBRARYUID, length);
        strncat((char *)statement, (const char*)IPOD_PLAYER_IAP2_DB_PREPARE_EQUALE, length);
        strncat((char *)statement, (const char*)IPOD_PLAYER_IAP2_DB_PREPARE_PARAM, length);
    }

    if(item->iAP2PBAppleMusicRadioAd_count > 0)
    {
        strncat((char *)statement, (const char*)IPOD_PLAYER_IAP2_DB_PREPARE_COMMA, length);
        strncat((char *)statement, (const char*)IPOD_PLAYER_IAP2_DB_COLUMN_RADIOAD, length);
        strncat((char *)statement, (const char*)IPOD_PLAYER_IAP2_DB_PREPARE_EQUALE, length);
        strncat((char *)statement, (const char*)IPOD_PLAYER_IAP2_DB_PREPARE_PARAM, length);
    }
    
    if(item->iAP2PBAppleMusicRadioStationName_count > 0)
    {
        strncat((char *)statement, (const char*)IPOD_PLAYER_IAP2_DB_PREPARE_COMMA, length);
        strncat((char *)statement, (const char*)IPOD_PLAYER_IAP2_DB_COLUMN_RADIOSTATIONNAME, length);
        strncat((char *)statement, (const char*)IPOD_PLAYER_IAP2_DB_PREPARE_EQUALE, length);
        strncat((char *)statement, (const char*)IPOD_PLAYER_IAP2_DB_PREPARE_PARAM, length);
    }

    if(item->iAP2PBAppeMusicRadioStationMediaPlaylistPersistentID_count > 0)
    {
        strncat((char *)statement, (const char*)IPOD_PLAYER_IAP2_DB_PREPARE_COMMA, length);
        strncat((char *)statement, (const char*)IPOD_PLAYER_IAP2_DB_COLUMN_RADIOSTATIONID, length);
        strncat((char *)statement, (const char*)IPOD_PLAYER_IAP2_DB_PREPARE_EQUALE, length);
        strncat((char *)statement, (const char*)IPOD_PLAYER_IAP2_DB_PREPARE_PARAM, length);
    }

    if(item->iAP2PlaybackQueueChapterIndex_count > 0)
    {
        strncat((char *)statement, (const char*)IPOD_PLAYER_IAP2_DB_PREPARE_COMMA, length);
        strncat((char *)statement, (const char*)IPOD_PLAYER_IAP2_DB_COLUMN_CHAPTERINDEX, length);
        strncat((char *)statement, (const char*)IPOD_PLAYER_IAP2_DB_PREPARE_EQUALE, length);
        strncat((char *)statement, (const char*)IPOD_PLAYER_IAP2_DB_PREPARE_PARAM, length);
    }
    
    if(item->iAP2PlaybackQueueIndex_count > 0)
    {
        strncat((char *)statement, (const char*)IPOD_PLAYER_IAP2_DB_PREPARE_COMMA, length);
        strncat((char *)statement, (const char*)IPOD_PLAYER_IAP2_DB_COLUMN_INDEX, length);
        strncat((char *)statement, (const char*)IPOD_PLAYER_IAP2_DB_PREPARE_EQUALE, length);
        strncat((char *)statement, (const char*)IPOD_PLAYER_IAP2_DB_PREPARE_PARAM, length);
    }
    
    if(item->iAP2PlaybackQueueCount_count > 0)
    {
        strncat((char *)statement, (const char*)IPOD_PLAYER_IAP2_DB_PREPARE_COMMA, length);
        strncat((char *)statement, (const char*)IPOD_PLAYER_IAP2_DB_COLUMN_QUEUECOUNT, length);
        strncat((char *)statement, (const char*)IPOD_PLAYER_IAP2_DB_PREPARE_EQUALE, length);
        strncat((char *)statement, (const char*)IPOD_PLAYER_IAP2_DB_PREPARE_PARAM, length);
    }

    if(item->iAP2PlaybackQueueListAvail_count > 0)
    {
        strncat((char *)statement, (const char*)IPOD_PLAYER_IAP2_DB_PREPARE_COMMA, length);
        strncat((char *)statement, (const char*)IPOD_PLAYER_IAP2_DB_COLUMN_QUEUELISTAVAIL, length);
        strncat((char *)statement, (const char*)IPOD_PLAYER_IAP2_DB_PREPARE_EQUALE, length);
        strncat((char *)statement, (const char*)IPOD_PLAYER_IAP2_DB_PREPARE_PARAM, length);
    }

    strncat((char *)statement, (const char*)IPOD_PLAYER_IAP2_DB_QUERY_WHERE, length);
    strncat((char *)statement, (const char*)IPOD_PLAYER_IAP2_DB_COLUMN_IPODID, length);
    strncat((char *)statement, (const char*)IPOD_PLAYER_IAP2_DB_PREPARE_EQUALE, length);
    strncat((char *)statement, (const char*)IPOD_PLAYER_IAP2_DB_PREPARE_PARAM, length);
    
    strncat((char *)statement, (const char*)IPOD_PLAYER_IAP2_DB_IPODINFO_UPDATE_PREPARE_END, length);
    
    return;
}

U8 *ippiAP2DBGenerateGetPlaybackStatusStatement()
{
    U8 *statement = NULL;
    U32 length = 0;
    
    length = sizeof(IPOD_PLAYER_IAP2_DB_QUERY_PLAYBACK_STATUS);
    
    statement = calloc(length + IPOD_PLAYER_IAP2_DB_NULL_LEN, sizeof(U8));
    if(statement != NULL)
    {
        strncpy((char *)statement, (const char*)IPOD_PLAYER_IAP2_DB_QUERY_PLAYBACK_STATUS, length);
    }
    
    return statement;
}

sqlite3_stmt *ippiAP2DBGenerateSetMediaLibraryInformationStatement(sqlite3* handle, const U8 *key, iAP2MediaLibraryInformationSubParameter *media)
{
    int ret = -1;
    sqlite3_stmt *statement = NULL;
    U32 i = 1;
    
    if((handle == NULL) || (key == NULL) || (media == NULL))
    {
        return NULL;
    }
    
    ret = sqlite3_prepare(handle, IPOD_PLAYER_IAP2_DB_REPLACE_MEDIAINFO, strlen(IPOD_PLAYER_IAP2_DB_REPLACE_MEDIAINFO), &statement, NULL);
    if(ret == SQLITE_OK)
    {
        ret = sqlite3_bind_text(statement, i, (const char *)media->iAP2MediaUniqueIdentifier[0], -1, NULL);
        if(ret == SQLITE_OK)
        {
            i++;
            ret = sqlite3_bind_text(statement, i, (const char *)media->iAP2MediaLibraryName[0], -1, NULL);
            if(ret == SQLITE_OK)
            {
                i++;
                ret = sqlite3_bind_int(statement, i, *media->iAP2MediaLibraryType);
                if(ret == SQLITE_OK)
                {
                    i++;
                    ret = sqlite3_bind_text(statement, i, (const char *)key, -1, NULL);
                }
            }
        }
    }
    
    if((ret != SQLITE_OK) && (statement  != NULL))
    {
        sqlite3_finalize(statement);
        statement = NULL;
    }
    
    return statement;
}



U8 *ippiAP2DBGenerateGetMediaLibraryInformationStatement(sqlite3* handle, const U8 *key)
{
    U32 length = 0;
    U8 *statement = NULL;
    
    if((handle == NULL) || (key == NULL))
    {
        return NULL;
    }
    
    length = sizeof(IPOD_PLAYER_IAP2_DB_QUERY_MEDIAINFO) + strlen((const char*)key);
    statement = calloc(length + IPOD_PLAYER_IAP2_DB_NULL_LEN, sizeof(U8));
    
    snprintf((char *)statement, length, (const char*)IPOD_PLAYER_IAP2_DB_QUERY_MEDIAINFO, key);
    
    return statement;
}

sqlite3_stmt *ippiAP2DBGenerateSetRevisionStatement(sqlite3* handle, const U8 *key, const U8 *revision, U32 progress)
{
    int ret = -1;
    sqlite3_stmt *statement = NULL;
    U32 i = 1;
    
    if((handle == NULL) || (key == NULL))
    {
        return NULL;
    }
    
    if(revision != NULL)
    {
        /* set revision and progress to MediaInfo DB. */
        ret = sqlite3_prepare(handle, IPOD_PLAYER_IAP2_DB_UPDATE_REVISION, strlen(IPOD_PLAYER_IAP2_DB_UPDATE_REVISION), &statement, NULL);
        if(ret == SQLITE_OK)
        {
            ret = sqlite3_bind_text(statement, i, (const char *)revision, -1, NULL);
            if(ret == SQLITE_OK)
            {
                i++;
                ret = sqlite3_bind_int(statement, i, progress);
                if(ret == SQLITE_OK)
                {
                    i++;
                    ret = sqlite3_bind_text(statement, i, (const char *)key, -1, NULL);
                }
            }
        }
    }
    else
    {
        /* set only progress to MediaInfo DB. */
        ret = sqlite3_prepare(handle, IPOD_PLAYER_IAP2_DB_PROGRESS_MEDIAID, strlen(IPOD_PLAYER_IAP2_DB_PROGRESS_MEDIAID), &statement, NULL);
        if(ret == SQLITE_OK)
        {
            ret = sqlite3_bind_int(statement, i, progress);
            if(ret == SQLITE_OK)
            {
                i++;
                ret = sqlite3_bind_text(statement, i, (const char *)key, -1, NULL);
            }
        }
    }

    if((ret != SQLITE_OK) && (statement  != NULL))
    {
        sqlite3_finalize(statement);
        statement = NULL;
    }
    
    return statement;
}

U8* ippiAP2DBGenerateGetCountStatement(const U8 *table)
{
    U8 *statement = NULL;
    U16 length = 0;
    
    if(table == NULL)
    {
        return NULL;
    }
    
    length = sizeof(IPOD_PLAYER_IAP2_DB_QUERY_SELECT_COUNT) + strlen((const char*)table);
    
    statement = calloc(length + IPOD_PLAYER_IAP2_DB_NULL_LEN, sizeof(U8));
    if(statement != NULL)
    {
        snprintf((char *)statement, length, (const char*)IPOD_PLAYER_IAP2_DB_QUERY_SELECT_COUNT, table);
    }
    
    return statement;
    
}

BOOL ippiAP2DBCheckCategory(IPOD_PLAYER_IAP2_DB_CATLIST *catList, char *checkCategory)
{
    U32 i = 0;
    BOOL ret = FALSE;

    for(i = 0; i < catList->count; i++)
    {
        if(strstr((const char*)catList->categories[i].category, (const char*)checkCategory) != NULL)
        {
            ret = TRUE;
        }
    }
    
    return ret;
}

/* get media type */
U32 ippiAP2GetMediaType(IPOD_PLAYER_IAP2_DB_CATLIST *catList)
{
    S32 mediaType = IPP_MEDIA_TYPE_MUSIC;
    struct _mediaTypeTable
    {   char    *catstr;
        U32     catNum;
    } mtable[] = {
        {  "MediaType=0", IPP_MEDIA_TYPE_MUSIC      }, 
        {  "MediaType=1", IPP_MEDIA_TYPE_PODCAST    }, 
        {  "MediaType=2", IPP_MEDIA_TYPE_AUDIOBOOK  }, 
        {  "MediaType=3", IPP_MEDIA_TYPE_ITUNESU    }};
    unsigned int i = 0;

    for(i = 0; i < sizeof(mtable)/sizeof(mtable[0]); i++ )
    {
        if(ippiAP2DBCheckCategory(catList, mtable[i].catstr))
        {
            mediaType = mtable[i].catNum;
            break;
        }
    }    
    return mediaType;
}

void ippiAP2DBAddCategory(char *statement, IPOD_PLAYER_IAP2_DB_CATLIST *catList, char *temp, U16 length)
{
    BOOL setWhere = FALSE;
    U32 i = 0;

    memset(temp, 0, length);    /* buffer clear */

    /* check "WHERE" keyword */
    if(strstr((char *)statement, "WHERE") != NULL)
    {
        setWhere = TRUE;
    }

    for(i = 0; i < catList->count; i++)
    {
        if(i == 0 && setWhere == FALSE)
        {
            /* create WHERE keyword */
            snprintf((char *)temp, length, (const char *)IPOD_PLAYER_IAP2_DB_QUERY_LIST_WHERE, catList->categories[i].category, catList->categories[i].catID);
        }
        else
        {
            /* add AND keyword */
            snprintf((char *)temp, length, (const char *)IPOD_PLAYER_IAP2_DB_QUERY_LIST_AND, catList->categories[i].category, catList->categories[i].catID);
        }
        
        strncat((char *)statement, (const char*)temp, length);
    }
}

void ippiAP2DBAddMediaType(char *statement, U32 mediaType, char *temp, U16 length)
{
    BOOL setWhere = FALSE;

    if(mediaType != IPP_NO_MEDIA_TYPE)
    {
        memset(temp, 0, length);    /* buffer clear */

        /* check "WHERE" keyword */
        if(strstr((char *)statement, "WHERE") != NULL)
        {
            setWhere = TRUE;
        }
        if(setWhere != TRUE)
        {
            /* create WHERE keyword */
            snprintf(temp, length, (const char *)IPOD_PLAYER_IAP2_DB_QUERY_LIST_WHERE_32, "MediaType", mediaType);
        }
        else
        {
            /* add AND keyword */
            snprintf(temp, length, (const char *)IPOD_PLAYER_IAP2_DB_QUERY_LIST_AND_32, "MediaType", mediaType);
        }

        strncat(statement, (const char*)temp, length);
    }
}

U8 *ippiAP2DBGenerateGetListStatement(IPOD_PLAYER_DB_TYPE type, IPOD_PLAYER_IAP2_DB_CATLIST *catList, U32 start, S32 count)
{
    S32 rc = IPOD_PLAYER_ERROR;
    U8 *statement = NULL;
    U8 *orderstate = NULL;
    U8 *temp = NULL;
    U16 length = IPOD_PLAYER_IAP2_DB_STATEMENT_MAX;
    U32 mediaType = IPP_NO_MEDIA_TYPE;
    
    
    if(catList == NULL)
    {
        return NULL;
    }
    
    statement = calloc(length + IPOD_PLAYER_IAP2_DB_NULL_LEN, sizeof(U8));
    if(statement != NULL)
    {
        temp = calloc(length + IPOD_PLAYER_IAP2_DB_NULL_LEN, sizeof(U8));
    }
    
    if((statement != NULL) && (temp != NULL))
    {
        rc = ippiAP2CreateDBStatement(type, &statement, &orderstate, length, &mediaType, catList);
        if(rc == IPOD_PLAYER_OK)
        {
        /* MediaType */
            ippiAP2DBAddMediaType((char *)statement, mediaType, (char *)temp, length);

        /* Categories */
            /* Podcast is not impacted from category list. */
            ippiAP2DBAddCategory((char *)statement, catList, (char *)temp, length);

        }
        memset(temp, 0, length);    /* buffer clear */
        
        /* Order */
        if((rc == IPOD_PLAYER_OK) && (orderstate != NULL))
        {
            strncat((char *)statement, (const char*)orderstate, length);
        }
        
        if((rc == IPOD_PLAYER_OK) && (count != -1))
        {
            snprintf((char *)temp, length, (const char*)IPOD_PLAYER_IAP2_DB_QUERY_LIST_LIMIT_OFFSET, count, (U64)start);
            strncat((char *)statement, (const char*)temp, length);
        }
    }
    else
    {
        rc = IPOD_PLAYER_ERR_NOMEM;
    }
    
    if(temp != NULL)
    {
        free(temp);
    }
    
    return statement;
    
}

static S32 ippiAP2DBSetFirstStatement(IPOD_PLAYER_DB_TYPE type, U32 length, U8 *statement, const U8 *table)
{
    S32 rc = IPOD_PLAYER_ERROR;
    char *temp = NULL;
    
    if(statement == NULL)
    {
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    switch(type)
    {
    case IPOD_PLAYER_DB_TYPE_ALL:
        /* Todo */
        rc = IPOD_PLAYER_ERR_INVALID_PARAMETER;
        break;
        
    case IPOD_PLAYER_DB_TYPE_PLAYLIST:
        temp = IPOD_PLAYER_IAP2_DB_COLUMN_PLAYLIST;
        rc = IPOD_PLAYER_OK;
        break;
        
    case IPOD_PLAYER_DB_TYPE_ARTIST:
        // temp =  IPOD_PLAYER_IAP2_DB_COLUMN_ALBUMARTISTID;
        temp = IPOD_PLAYER_IAP2_DB_COLUMN_ARTISTID;
        rc = IPOD_PLAYER_OK;
        break;
        
    case IPOD_PLAYER_DB_TYPE_ALBUM:
        temp = IPOD_PLAYER_IAP2_DB_COLUMN_ALBUMID;
        rc = IPOD_PLAYER_OK;
        break;
        
    case IPOD_PLAYER_DB_TYPE_GENRE:
        temp = IPOD_PLAYER_IAP2_DB_COLUMN_GENREID;
        rc = IPOD_PLAYER_OK;
        break;
        
    case IPOD_PLAYER_DB_TYPE_TRACK:
        temp = IPOD_PLAYER_IAP2_DB_COLUMN_TRACKID;
        rc = IPOD_PLAYER_OK;
        break;
        
    case IPOD_PLAYER_DB_TYPE_COMPOSER:
        temp = IPOD_PLAYER_IAP2_DB_COLUMN_COMPOSERID;
        rc = IPOD_PLAYER_OK;
        break;
        
    case IPOD_PLAYER_DB_TYPE_AUDIOBOOK:
        temp = IPOD_PLAYER_IAP2_DB_COLUMN_TRACKID;
        rc = IPOD_PLAYER_OK;
        break;
        
    case IPOD_PLAYER_DB_TYPE_PODCAST:
        temp = IPOD_PLAYER_IAP2_DB_COLUMN_ALBUMID;
        rc = IPOD_PLAYER_OK;
        break;
        
    case IPOD_PLAYER_DB_TYPE_ITUNESU:
        temp = IPOD_PLAYER_IAP2_DB_COLUMN_TRACKID;
        rc = IPOD_PLAYER_OK;
        break;
        
    case IPOD_PLAYER_DB_TYPE_NESTED_PLAYLIST:
    case IPOD_PLAYER_DB_TYPE_INTELLIGENT:
        rc = IPOD_PLAYER_ERR_INVALID_PARAMETER;     /* Todo */
        break;
        
    default:
        rc = IPOD_PLAYER_ERR_INVALID_PARAMETER;
        break;
    }
    
    if((rc == IPOD_PLAYER_OK) && (temp != NULL))
    {
        snprintf((char *)statement, length, IPOD_PLAYER_IAP2_DB_SELECT_COUNT, temp, table);
    }
    
    return rc;
}

static S32 ippiAP2DBSetConditionStatement(U32 length, U8 *statement, const U8 *table, U32 mediaType, IPOD_PLAYER_IAP2_DB_CATLIST *catList, IPOD_PLAYER_DB_TYPE type)
{
    S32 rc = IPOD_PLAYER_OK;
    U8 temp[IPOD_PLAYER_IAP2_DB_STATEMENT_MAX + IPOD_PLAYER_IAP2_DB_NULL_LEN] = {0};
    
    if((statement == NULL) || (table == NULL) || (catList == NULL))
    {
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* check category list to use playlist table to statement */
    if(ippiAP2DBCheckCategory(catList, IPOD_PLAYER_IAP2_DB_COLUMN_PLAYLIST))
    {
        if(type == IPOD_PLAYER_DB_TYPE_PLAYLIST)
        {
            strncpy((char *)statement, (const char*)IPOD_PLAYER_IAP2_DB_QUERY_DBLIST_COUNT_PLAYLIST_FROM_PLAYLIST, length);
        } 
        else if(type == IPOD_PLAYER_DB_TYPE_TRACK)
        {
            strncpy((char *)statement, (const char*)IPOD_PLAYER_IAP2_DB_QUERY_DBLIST_COUNT_TRACK_FROM_PLAYLIST, length);
        }
        else
        {
            rc = IPOD_PLAYER_ERR_INVALID_PARAMETER;
        }
    }
    
    if(strncmp("MediaItem", (const char*)table, sizeof("MediaItem")) == 0)
    {
        /* Podcast isn't impacted from category list. */
        /* add categories */
        ippiAP2DBAddCategory((char *)statement, catList, (char *)temp, length);
    
        /* add media type */
        ippiAP2DBAddMediaType((char *)statement, mediaType, (char *)temp, length);
    }

    if(strncmp("Playlist", (const char*)table, sizeof("Playlist")) == 0)
    {
        /* Podcast isn't impacted from category list. */
        /* add categories */
        ippiAP2DBAddCategory((char *)statement, catList, (char *)temp, length);
    
        /* add media type */
        ippiAP2DBAddMediaType((char *)statement, mediaType, (char *)temp, length);
    }
    
    return rc;
}


U8 *ippiAP2DBGenerateCountStatement(const U8 *table, IPOD_PLAYER_DB_TYPE type, IPOD_PLAYER_IAP2_DB_CATLIST *catList)
{
    S32 rc = IPOD_PLAYER_ERROR;
    U8 *statement = NULL;
    U16 length = IPOD_PLAYER_IAP2_DB_STATEMENT_MAX;
    U32 mediaType = 0;
    
    if((table == NULL) || (catList == NULL))
    {
        return NULL;
    }
    
    statement = calloc(length + IPOD_PLAYER_IAP2_DB_NULL_LEN, sizeof(U8));
    if(statement == NULL)
    {
        return NULL;
    }
    
    rc = ippiAP2DBSetFirstStatement(type, length, statement, table);
    if(rc == IPOD_PLAYER_OK)
    {
        if(type == IPOD_PLAYER_DB_TYPE_AUDIOBOOK)
        {
            mediaType = IPP_MEDIA_TYPE_AUDIOBOOK;   /* AudioBook type */
        }
        else if(type == IPOD_PLAYER_DB_TYPE_PODCAST)
        {
            mediaType = IPP_MEDIA_TYPE_PODCAST;     /* Podcast type */
        }
        else if(type == IPOD_PLAYER_DB_TYPE_ITUNESU)
        {
            mediaType = IPP_MEDIA_TYPE_ITUNESU;     /* iTunes type */
        }
        else if(type == IPOD_PLAYER_DB_TYPE_TRACK)
        {
            /* Check category type */
            if(ippiAP2DBCheckCategory(catList, IPOD_PLAYER_IAP2_DB_COLUMN_MEDIATYPE) ||
               ippiAP2DBCheckCategory(catList, IPOD_PLAYER_IAP2_DB_COLUMN_PLAYLIST))
            {
                mediaType = IPP_NO_MEDIA_TYPE;
            }
            else
            {
                mediaType = IPP_MEDIA_TYPE_MUSIC;       /* Music type */
            }
        }
        else if((type == IPOD_PLAYER_DB_TYPE_GENRE)  ||
                (type == IPOD_PLAYER_DB_TYPE_ALBUM)  ||
                (type == IPOD_PLAYER_DB_TYPE_ARTIST) ||
                (type == IPOD_PLAYER_DB_TYPE_COMPOSER))
        {
            mediaType = IPP_MEDIA_TYPE_MUSIC;       /* Music type */
        }
        else
        {
            mediaType = IPP_NO_MEDIA_TYPE;
        }
        
        rc = ippiAP2DBSetConditionStatement(length, statement, table, mediaType, catList, type);
    }
    
    return statement;

}

U8 *ippiAP2DBGenerateGetCategoryIDStatement(IPOD_PLAYER_DB_TYPE type, U32 catIndex, IPOD_PLAYER_IAP2_DB_CATLIST *catList)
{
    S32 rc = IPOD_PLAYER_ERROR;
    U8 *statement = NULL;
    U8 *temp = NULL;
    U16 length = IPOD_PLAYER_IAP2_DB_STATEMENT_MAX;
    U8 *orderstate = NULL;
    U32 mediaType = IPP_NO_MEDIA_TYPE;
    
    if(catList == NULL)
    {
        return NULL;
    }
    
    statement = calloc(length + IPOD_PLAYER_IAP2_DB_NULL_LEN, sizeof(U8));
    if(statement != NULL)
    {
        temp = calloc(length + IPOD_PLAYER_IAP2_DB_NULL_LEN, sizeof(U8));
    }
    
    if((statement != NULL) && (temp != NULL))
    {
        if((type == IPOD_PLAYER_DB_TYPE_PLAYLIST) && (catList->count != 0))
        {
            rc = IPOD_PLAYER_ERR_INVALID_PARAMETER;
        }
        else
        {
            rc = ippiAP2CreateDBStatement(type, &statement, &orderstate, length, &mediaType, catList);
            if(rc == IPOD_PLAYER_OK)
            {
            /* MediaType */
                ippiAP2DBAddMediaType((char *)statement, mediaType, (char *)temp, length);

            /* Categories */
                /* Podcast is not impacted from category list. */
                ippiAP2DBAddCategory((char *)statement, catList, (char *)temp, length);

            }
        }

        memset(temp, 0, length);    /* buffer clear */
        
        /* add order statement */
        if((rc == IPOD_PLAYER_OK) && (orderstate != NULL))
        {
            strncat((char *)statement, (const char*)orderstate, length);
        }
        
        /* add limit and offset to statement. */
        if(rc == IPOD_PLAYER_OK)
        {
            snprintf((char *)temp, length, (const char*)IPOD_PLAYER_IAP2_DB_QUERY_LIST_LIMIT_OFFSET, 1, (U64)catIndex);
            strncat((char *)statement, (const char*)temp, length);
        }
    }
    else
    {
        rc = IPOD_PLAYER_ERR_NOMEM;
    }
    
    if(temp != NULL)
    {
        free(temp);
    }
    
    return statement;
    
}

U8 *ippiAP2DBGenerateSetCategoryIDStatement(IPOD_PLAYER_DB_TYPE type, U32 catIndex, U64 id)
{
    U8 *statement = NULL;
    U8 *temp = NULL;
    U32 length = 0;
    
    switch(type)
    {
    case IPOD_PLAYER_DB_TYPE_ALL:
        /* Todo */
        break;
        
    case IPOD_PLAYER_DB_TYPE_PLAYLIST:
        temp = (U8 *)IPOD_PLAYER_IAP2_DB_INSERT_CAT_PLAYLISTID;
        length = sizeof(IPOD_PLAYER_IAP2_DB_INSERT_CAT_PLAYLISTID);
        break;
        
    case IPOD_PLAYER_DB_TYPE_ARTIST:
        temp = (U8 *)IPOD_PLAYER_IAP2_DB_INSERT_CAT_ARTISTID;
        length = sizeof(IPOD_PLAYER_IAP2_DB_INSERT_CAT_ARTISTID);
        break;
        
    case IPOD_PLAYER_DB_TYPE_ALBUM:
        temp = (U8 *)IPOD_PLAYER_IAP2_DB_INSERT_CAT_ALBUMID;
        length = sizeof(IPOD_PLAYER_IAP2_DB_INSERT_CAT_ALBUMID);
        break;
        
    case IPOD_PLAYER_DB_TYPE_GENRE:
        temp = (U8 *)IPOD_PLAYER_IAP2_DB_INSERT_CAT_GENREID;
        length = sizeof(IPOD_PLAYER_IAP2_DB_INSERT_CAT_GENREID);
        break;
        
    case IPOD_PLAYER_DB_TYPE_TRACK:
        temp = (U8 *)IPOD_PLAYER_IAP2_DB_INSERT_CAT_TRACKID;
        length = sizeof(IPOD_PLAYER_IAP2_DB_INSERT_CAT_TRACKID);
        break;
        
    case IPOD_PLAYER_DB_TYPE_COMPOSER:
        temp = (U8 *)IPOD_PLAYER_IAP2_DB_INSERT_CAT_COMPOSERID;
        length = sizeof(IPOD_PLAYER_IAP2_DB_INSERT_CAT_COMPOSERID);
        break;
        
    case IPOD_PLAYER_DB_TYPE_AUDIOBOOK:
        temp = (U8 *)IPOD_PLAYER_IAP2_DB_INSERT_CAT_AUDIOBOOKID;
        length = sizeof(IPOD_PLAYER_IAP2_DB_INSERT_CAT_AUDIOBOOKID);
        break;
        
    case IPOD_PLAYER_DB_TYPE_PODCAST:
        temp = (U8 *)IPOD_PLAYER_IAP2_DB_INSERT_CAT_PODCASTID;
        length = sizeof(IPOD_PLAYER_IAP2_DB_INSERT_CAT_PODCASTID);
        break;
        
    case IPOD_PLAYER_DB_TYPE_NESTED_PLAYLIST:
        /* Todo */
        break;
        
    case IPOD_PLAYER_DB_TYPE_INTELLIGENT:
        /* Todo */
        break;
        
    case IPOD_PLAYER_DB_TYPE_ITUNESU:
        temp = (U8 *)IPOD_PLAYER_IAP2_DB_INSERT_CAT_ARTISTID;
        length = sizeof(IPOD_PLAYER_IAP2_DB_INSERT_CAT_ARTISTID);
        break;
        
    default:
        break;
    }
    
    if(length > 0)
    {
        length += IPOD_PLAYER_IAP2_DB_U32_MAX_DIGIT + IPOD_PLAYER_IAP2_DB_U64_MAX_DIGIT;
        statement = calloc(length + IPOD_PLAYER_IAP2_DB_NULL_LEN, sizeof(U8));
        if(statement != NULL)
        {
            snprintf((char *)statement, length, (const char*)temp, catIndex, id);
        }
    }
    
    return statement;
}

U8 *ippiAP2DBGenerateClearSelectingCategory(IPOD_PLAYER_DB_TYPE type)
{
    U8 *statement = NULL;
    U32 length = IPOD_PLAYER_IAP2_DB_STATEMENT_MAX;
    
    statement = calloc(length + IPOD_PLAYER_IAP2_DB_NULL_LEN, sizeof(U8));
    if(statement == NULL)
    {
        return NULL;
    }
    
    switch(type)
    {
    case IPOD_PLAYER_DB_TYPE_ALL:
        strncpy((char *)statement, (const char *)IPOD_PLAYER_IAP2_DB_DELETE_CAT_ALL, length);
        break;
        
    case IPOD_PLAYER_DB_TYPE_PLAYLIST:
        strncpy((char *)statement, (const char *)IPOD_PLAYER_IAP2_DB_DELETE_CAT_PLAYLIST, length);
        break;
        
    case IPOD_PLAYER_DB_TYPE_ARTIST:
        strncpy((char *)statement, (const char *)IPOD_PLAYER_IAP2_DB_DELETE_CAT_ARTIST, length);
        break;
        
    case IPOD_PLAYER_DB_TYPE_ALBUM:
        strncpy((char *)statement, (const char *)IPOD_PLAYER_IAP2_DB_DELETE_CAT_ALBUM, length);
        break;
        
    case IPOD_PLAYER_DB_TYPE_GENRE:
        strncpy((char *)statement, (const char *)IPOD_PLAYER_IAP2_DB_DELETE_CAT_GENRE, length);
        break;
        
    case IPOD_PLAYER_DB_TYPE_TRACK:
        strncpy((char *)statement, (const char *)IPOD_PLAYER_IAP2_DB_DELETE_CAT_TRACK, length);
        break;
        
    case IPOD_PLAYER_DB_TYPE_COMPOSER:
        strncpy((char *)statement, (const char *)IPOD_PLAYER_IAP2_DB_DELETE_CAT_COMPOSER, length);
        break;
        
    case IPOD_PLAYER_DB_TYPE_AUDIOBOOK:
        strncpy((char *)statement, (const char *)IPOD_PLAYER_IAP2_DB_DELETE_CAT_AUDIOBOOK, length);
        break;
        
    case IPOD_PLAYER_DB_TYPE_PODCAST:
        strncpy((char *)statement, (const char *)IPOD_PLAYER_IAP2_DB_DELETE_CAT_PODCAST, length);
        break;
        
    case IPOD_PLAYER_DB_TYPE_ITUNESU:
        strncpy((char *)statement, (const char *)IPOD_PLAYER_IAP2_DB_DELETE_CAT_ITUNEU, length);
        break;
        
    case IPOD_PLAYER_DB_TYPE_NESTED_PLAYLIST:
    case IPOD_PLAYER_DB_TYPE_INTELLIGENT:
    default:
        break;
    }
    
    return statement;
}

U8 *ippiAP2DBGenerateGetMediaItemIDFromMediaItemStatement(IPOD_PLAYER_DB_TYPE type, IPOD_PLAYER_IAP2_DB_CATLIST *catList)
{
    S32 rc = IPOD_PLAYER_ERROR;
    U8 *statement = NULL;
    U8 *temp = NULL;
    U16 length = IPOD_PLAYER_IAP2_DB_STATEMENT_MAX;
    U8 *orderstate = NULL;
    U32 mediaType = IPP_NO_MEDIA_TYPE;
    
    if(catList == NULL)
    {
        return NULL;
    }
    
    statement = calloc(length + IPOD_PLAYER_IAP2_DB_NULL_LEN, sizeof(U8));
    if(statement != NULL)
    {
        temp = calloc(length + IPOD_PLAYER_IAP2_DB_NULL_LEN, sizeof(U8));
    }
    
    if((statement != NULL) && (temp != NULL))
    {
        rc = ippiAP2CreateDBStatement(type, &statement, &orderstate, length, &mediaType, catList);
        if(rc == IPOD_PLAYER_OK)
        {
        /* add media type */
            ippiAP2DBAddMediaType((char *)statement, mediaType, (char *)temp, length);
 
        /* add categories */
            ippiAP2DBAddCategory((char *)statement, catList, (char *)temp, length);
        }

        /* add order statement */
        if((rc == IPOD_PLAYER_OK) && (orderstate != NULL))
        {
            strncat((char *)statement, (const char*)orderstate, length);
        }
    }
    
    if(temp != NULL)
    {
        free(temp);
    }
    
    return statement;
    
}

U8 *ippiAP2DBGenerateGetMediaItemIDFromNowPlayingStatement(IPOD_PLAYER_DB_TYPE type)
{
    U8 *statement = NULL;
    U16 length = IPOD_PLAYER_IAP2_DB_STATEMENT_MAX;
    U8 *orderstate = NULL;
    
    statement = calloc(length + IPOD_PLAYER_IAP2_DB_NULL_LEN, sizeof(U8));
    if(statement == NULL)
    {
        return NULL;
    }
    
    switch(type)
    {
    case IPOD_PLAYER_DB_TYPE_ALL:
        /* Todo */
        break;
        
    case IPOD_PLAYER_DB_TYPE_PLAYLIST:
        /* Todo */
        break;
        
    case IPOD_PLAYER_DB_TYPE_ARTIST:
        strncat((char *)statement, (const char*)IPOD_PLAYER_IAP2_DB_QUERY_ARTISTID_FROM_NOWPLAYING, length);
        orderstate = (U8 *)IPOD_PLAYER_IAP2_DB_ORDER_ARTIST;
        break;
        
    case IPOD_PLAYER_DB_TYPE_ALBUM:
        strncat((char *)statement, (const char*)IPOD_PLAYER_IAP2_DB_QUERY_ALBUMID_FROM_NOWPLAYING, length);
        orderstate = (U8 *)IPOD_PLAYER_IAP2_DB_ORDER_ALBUM;
        break;
        
    case IPOD_PLAYER_DB_TYPE_GENRE:
        strncat((char *)statement, (const char*)IPOD_PLAYER_IAP2_DB_QUERY_GENREID_FROM_NOWPLAYING, length);
        orderstate = (U8 *)IPOD_PLAYER_IAP2_DB_ORDER_GENRE;
        break;
        
    case IPOD_PLAYER_DB_TYPE_TRACK:
        strncat((char *)statement, (const char*)IPOD_PLAYER_IAP2_DB_QUERY_TRACKID_FROM_NOWPLAYING, length);
        break;
        
    case IPOD_PLAYER_DB_TYPE_COMPOSER:
        strncat((char *)statement, (const char*)IPOD_PLAYER_IAP2_DB_QUERY_COMPOSERID_FROM_NOWPLAYING, length);
        orderstate = (U8 *)IPOD_PLAYER_IAP2_DB_ORDER_COMPOSER;
        break;
        
    case IPOD_PLAYER_DB_TYPE_AUDIOBOOK:
        strncat((char *)statement, (const char*)IPOD_PLAYER_IAP2_DB_QUERY_TRACKID_FROM_NOWPLAYING, length);
        orderstate = (U8 *)IPOD_PLAYER_IAP2_DB_ORDER_TRACK;
        break;
        
    case IPOD_PLAYER_DB_TYPE_PODCAST:
        strncat((char *)statement, (const char*)IPOD_PLAYER_IAP2_DB_QUERY_TRACKID_FROM_NOWPLAYING, length);
        orderstate = (U8 *)IPOD_PLAYER_IAP2_DB_ORDER_TRACK;
        break;
        
    case IPOD_PLAYER_DB_TYPE_NESTED_PLAYLIST:
        /* Todo */
        break;
        
    case IPOD_PLAYER_DB_TYPE_INTELLIGENT:
        /* Todo */
        break;
        
    case IPOD_PLAYER_DB_TYPE_ITUNESU:
        strncat((char *)statement, (const char*)IPOD_PLAYER_IAP2_DB_QUERY_TRACKID_FROM_NOWPLAYING, length);
        orderstate = (U8 *)IPOD_PLAYER_IAP2_DB_ORDER_TRACK;
        break;
        
    default:
        break;
    }
    
    if(orderstate != NULL)
    {
        strncat((char *)statement, (const char*)orderstate, length);
    }
    
    return statement;
    
}

sqlite3_stmt *ippiAP2DBGenerateSetNowPlayingItemID(sqlite3* handle)
{
    int ret = -1;
    sqlite3_stmt *statement = NULL;
    
    if(handle == NULL)
    {
        return NULL;
    }
    
    ret = sqlite3_prepare(handle, IPOD_PLAYER_IAP2_DB_INSERT_NOWPLAYING, strlen(IPOD_PLAYER_IAP2_DB_INSERT_NOWPLAYING), &statement, NULL);
    
    if(ret != SQLITE_OK)
    {
        sqlite3_finalize(statement);
        statement = NULL;
    }
    
    return statement;
}

sqlite3_stmt *ippiAP2DBGenerateDeleteMediaItemStatement(sqlite3 *handle)
{
    int ret = -1;
    sqlite3_stmt *statement = NULL;
    
    if(handle == NULL)
    {
        return NULL;
    }
    
    ret = sqlite3_prepare(handle, IPOD_PLAYER_IAP2_DB_DELETE_MEDIA_ITEM, strlen(IPOD_PLAYER_IAP2_DB_DELETE_MEDIA_ITEM), &statement, NULL);
    if(ret != SQLITE_OK)
    {
        sqlite3_finalize(statement);
        statement = NULL;
    }
    
    return statement;
}

sqlite3_stmt *ippiAP2DBGenerateSetPlaylistStatement(sqlite3* handle)
{
    int ret = -1;
    sqlite3_stmt *statement = NULL;
    
    if(handle == NULL)
    {
        return NULL;
    }
    
    ret = sqlite3_prepare(handle, IPOD_PLAYER_IAP2_DB_INSERT_PLAYLIST, strlen(IPOD_PLAYER_IAP2_DB_INSERT_PLAYLIST), &statement, NULL);
    if(ret != SQLITE_OK)
    {
        sqlite3_finalize(statement);
        statement = NULL;
    }
    
    return statement;
}

sqlite3_stmt *ippiAP2DBGenerateSetIsHidingStatement(sqlite3* handle, const U8 *key, U8 isHiding)
{
    int ret = -1;
    sqlite3_stmt *statement = NULL;
    U32 i = 1;
    
    if((handle == NULL) || (key == NULL))
    {
        return NULL;
    }
    
    ret = sqlite3_prepare(handle, IPOD_PLAYER_IAP2_DB_UPDATE_HIDING, strlen(IPOD_PLAYER_IAP2_DB_UPDATE_HIDING), &statement, NULL);
    if(ret == SQLITE_OK)
    {
        ret = sqlite3_bind_int(statement, i, (U32)isHiding);
        if(ret == SQLITE_OK)
        {
            i++;
            ret = sqlite3_bind_text(statement, i, (const char *)key, -1, NULL);
        }
    }
    
    if((ret != SQLITE_OK) && (statement  != NULL))
    {
        sqlite3_finalize(statement);
        statement = NULL;
    }
    
    return statement;
}

sqlite3_stmt *ippiAP2DBGenerateDeletePlaylistStatement(sqlite3 *handle)
{
    int ret = -1;
    sqlite3_stmt *statement = NULL;
    
    if(handle == NULL)
    {
        return NULL;
    }
    
    ret = sqlite3_prepare(handle, IPOD_PLAYER_IAP2_DB_DELETE_PLAYLIST, strlen(IPOD_PLAYER_IAP2_DB_DELETE_PLAYLIST), &statement, NULL);
    if(ret != SQLITE_OK)
    {
        sqlite3_finalize(statement);
        statement = NULL;
    }
    
    return statement;
}

sqlite3_stmt *ippiAP2DBGenerateSetPlaylistTracksStatement(sqlite3* handle)
{
    int ret = -1;
    sqlite3_stmt *statement = NULL;
    
    if(handle == NULL)
    {
        return NULL;
    }
    
    ret = sqlite3_prepare(handle, IPOD_PLAYER_IAP2_DB_INSERT_PLAYLIST_TRACKS, strlen(IPOD_PLAYER_IAP2_DB_INSERT_PLAYLIST_TRACKS), &statement, NULL);
    if(ret != SQLITE_OK)
    {
        sqlite3_finalize(statement);
        statement = NULL;
    }
    
    return statement;
}

U8 *ippiAP2DBGenerateSetDeviceNameStatement(const U8 *key, const U8 *name)
{
    U8 *statement;
    U32 length = IPOD_PLAYER_IAP2_DB_STATEMENT_MAX;
    
    if((key == NULL) || (name == NULL))
    {
        return NULL;
    }
    
    statement = calloc(length + IPOD_PLAYER_IAP2_DB_NULL_LEN, sizeof(U8));
    if(statement != NULL)
    {
        snprintf((char *)statement, length, (const char*)IPOD_PLAYER_IAP2_DB_UPDATE_MEDIAINFO_NAME, name, key);
    }
    
    return statement;
}

U8 *ippiAP2DBGenerateSetSampleStatement(U32 rate)
{
    U8 *statement;
    U32 length = 0;
    
    length = sizeof(IPOD_PLAYER_IAP2_DB_UPDATE_SAMPLE_RATE) + IPOD_PLAYER_IAP2_DB_U32_MAX_DIGIT;
    
    statement = calloc(length + IPOD_PLAYER_IAP2_DB_NULL_LEN, sizeof(U8));
    if(statement != NULL)
    {
        snprintf((char *)statement, length, (const char*)IPOD_PLAYER_IAP2_DB_UPDATE_SAMPLE_RATE, rate);
    }
    
    return statement;
}

U8 *ippiAP2DBGenerateSetAssistiveStatement(U32 assistiveID, IPOD_PLAYER_DEVICE_EVENT_ASSISTIVE_STATUS assistiveStatus)
{
    U8 *statement;
    U32 length = 0;
    
    length = sizeof(IPOD_PLAYER_IAP2_DB_ASSISTIVE_PREPARE) + IPOD_PLAYER_IAP2_DB_U32_MAX_DIGIT;
    
    statement = calloc(length + IPOD_PLAYER_IAP2_DB_NULL_LEN, sizeof(U8));
    if(statement != NULL)
    {
        snprintf((char *)statement, length, (const char*)IPOD_PLAYER_IAP2_DB_ASSISTIVE_PREPARE, assistiveID, assistiveStatus);
    }
    
    return statement;
}

U8 *ippiAP2DBGenerateGetAssistiveStatusStatement(U32 assistiveID)
{
    U8 *statement = NULL;
    U16 length = 0;
    
    length = sizeof(IPOD_PLAYER_IAP2_DB_QUERY_ASSISTIVE_STATUS) + IPOD_PLAYER_IAP2_DB_U32_MAX_DIGIT;
    
    statement = calloc(length + IPOD_PLAYER_IAP2_DB_NULL_LEN, sizeof(U8));
    if(statement == NULL)
    {
        return NULL;
    }
    
    snprintf((char *)statement, length, (const char *)IPOD_PLAYER_IAP2_DB_QUERY_ASSISTIVE_STATUS, assistiveID);
    
    return statement;
    
}

U8 *ippiAP2DBGenerateGetTrackIDListFromNowPlayingStatement(U64 trackIndex, U32 count)
{
    U8 *statement = NULL;
    U8 temp[IPOD_PLAYER_IAP2_DB_STATEMENT_MAX + IPOD_PLAYER_IAP2_DB_NULL_LEN] = {0};
    U16 length = IPOD_PLAYER_IAP2_DB_STATEMENT_MAX;
    
    statement = calloc(length + IPOD_PLAYER_IAP2_DB_NULL_LEN, sizeof(U8));
    if(statement == NULL)
    {
        return NULL;
    }
    
    
    /* Generate select track id statement */
    strncat((char *)statement, (const char*)IPOD_PLAYER_IAP2_DB_QUERY_TRACKID_FROM_NOWPLAYING, length);
    snprintf((char *)temp, length, (const char*)IPOD_PLAYER_IAP2_DB_QUERY_LIST_LIMIT_OFFSET, count, trackIndex);
    strncat((char *)statement, (const char*)temp, length);
    return statement;
}

U8 *ippiAP2DBGenerateGetTrackInfoListStatement(IPOD_PLAYER_IAP2_DB_HANDLE *dbHandle, U64 trackIndex, U32 count, U32 mediaType)
{
    S32 rc = IPOD_PLAYER_ERROR;
    U8 *statement = NULL;
    U16 length = IPOD_PLAYER_IAP2_DB_STATEMENT_MAX;
    U8 *temp = NULL;
    IPOD_PLAYER_IAP2_DB_CATLIST *catList = NULL;
    
    statement = calloc(length + IPOD_PLAYER_IAP2_DB_NULL_LEN, sizeof(U8));
    if(statement == NULL)
    {
        return NULL;            /* leave function */
    }
    else
    {
        temp = calloc(length + IPOD_PLAYER_IAP2_DB_NULL_LEN, sizeof(U8));
        if(temp == NULL)
        {
            free(statement);    /* free statement because temp could not be allocated. */
            return NULL;        /* leave function */
        }
        else
        {
            strncat((char *)statement, (const char*)IPOD_PLAYER_IAP2_DB_QUERY_TRACKINFO_FROM_MEDIAITEM, length);

            /* Get current selecting categories */
            rc = ippiAP2DBGetSelectingCategories(dbHandle, &catList);
            if(rc == IPOD_PLAYER_OK)
            {
                 /* add media type */
                ippiAP2DBAddMediaType((char *)statement, mediaType, (char *)temp, length);

                /* add categories */
                ippiAP2DBAddCategory((char *)statement, catList, (char *)temp, length);

                /* free current selecting categories */
                ippiAP2DBFreeSelectingCategories(catList);
           
                /* add sort order */
                strncat((char *)statement, (const char*)IPOD_PLAYER_IAP2_DB_ORDER_ARTISTID_ALBUMID_ALBUMTRACKCOUNT_TRACKID, length);

                /* add index and count */
                snprintf((char *)temp, length, (const char*)IPOD_PLAYER_IAP2_DB_QUERY_LIST_LIMIT_OFFSET, count, trackIndex);
                strncat((char *)statement, (const char*)temp, length);
            }

            free(temp);
        }
    }
    
    return statement;
}

U8 *ippiAP2DBGenerateGetTrackInfoTrackIDStatement(U64 *mediaId)
{
    U8 *statement = NULL;
    U16 length = IPOD_PLAYER_IAP2_DB_STATEMENT_MAX;
    U8 where[IPOD_PLAYER_IAP2_DB_STATEMENT_MAX + IPOD_PLAYER_IAP2_DB_NULL_LEN] = {0};
    
    if(mediaId == NULL)
    {
        return NULL;    /* leave function */
    }

    statement = calloc(length + IPOD_PLAYER_IAP2_DB_NULL_LEN, sizeof(U8));
    if(statement == NULL)
    {
        return NULL;    /* leave function */
    }
    
    strncat((char *)statement, (const char*)IPOD_PLAYER_IAP2_DB_QUERY_TRACKINFO_FROM_MEDIAITEM, length);
    snprintf((char *)where, length, (const char *)IPOD_PLAYER_IAP2_DB_QUERY_LIST_WHERE, IPOD_PLAYER_IAP2_DB_COLUMN_TRACKID, *mediaId);
    strncat((char *)statement, (const char*)where, length);
    
    return statement;
    
}

U8 *ippiAP2DBGenerateGetMediaTypeStatement(U64 *mediaId)
{
    U8 *statement = NULL;
    U16 length = IPOD_PLAYER_IAP2_DB_STATEMENT_MAX;
    U8 where[IPOD_PLAYER_IAP2_DB_STATEMENT_MAX + IPOD_PLAYER_IAP2_DB_NULL_LEN] = {0};
    
    if(mediaId == NULL)
    {
        return NULL;    /* leave function */
    }

    statement = calloc(length + IPOD_PLAYER_IAP2_DB_NULL_LEN, sizeof(U8));
    if(statement == NULL)
    {
        return NULL;    /* leave function */
    }
    
    strncat((char *)statement, (const char*)IPOD_PLAYER_IAP2_DB_QUERY_MEDIATYPE_FROM_MEDIAITEM, length);
    snprintf((char *)where, length, (const char *)IPOD_PLAYER_IAP2_DB_QUERY_LIST_WHERE, IPOD_PLAYER_IAP2_DB_COLUMN_TRACKID, *mediaId);
    strncat((char *)statement, (const char*)where, length);
    
    return statement;
    
}

U8 *ippiAP2DBGenerateSetBluetoothStatusStatement(IAP2_BLUETOOTH_INFO *btInfo)
{
    U8 *statement = NULL;
    U16 length = IPOD_PLAYER_IAP2_DB_STATEMENT_MAX;
    
    if(btInfo == NULL)
    {
        return NULL;
    }
    
    statement = calloc(length + IPOD_PLAYER_IAP2_DB_NULL_LEN, sizeof(U8));
    if(statement == NULL)
    {
        return NULL;
    }
    
    snprintf((char *)statement, length, (const char *)IPOD_PLAYER_IAP2_DB_BT_STATUS_PREPARE, btInfo->id, btInfo->profile);
    
    return statement;
    
}

U8 *ippiAP2DBGenerateGetBluetoothStatusStatement(U16 btID)
{
    U8 *statement = NULL;
    U16 length = IPOD_PLAYER_IAP2_DB_STATEMENT_MAX;
    
    statement = calloc(length + IPOD_PLAYER_IAP2_DB_NULL_LEN, sizeof(U8));
    if(statement == NULL)
    {
        return NULL;
    }
    
    snprintf((char *)statement, length, (const char *)IPOD_PLAYER_IAP2_DB_QUERY_BT_STATUS, btID);
    
    return statement;
    
}

sqlite3_stmt *ippiAP2DBGenerateDeletePlaylistTracksStatement(sqlite3 *handle)
{
    int ret = -1;
    sqlite3_stmt *statement = NULL;
    
    if(handle == NULL)
    {
        return NULL;
    }
    
    ret = sqlite3_prepare(handle, IPOD_PLAYER_IAP2_DB_DELETE_PLAYLIST_TRACKS, strlen(IPOD_PLAYER_IAP2_DB_DELETE_PLAYLIST_TRACKS), &statement, NULL);
    if(ret != SQLITE_OK)
    {
        sqlite3_finalize(statement);
        statement = NULL;
    }
    
    return statement;
}

