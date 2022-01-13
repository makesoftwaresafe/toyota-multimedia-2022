/* **********************  includes  ********************** */
#include "iap2_load_test.h"
#include <iap2_parameter_free.h>
#include "iap2_dlt_log.h"

/* **********************  defines   ********************** */

/* **********************  locals    ********************** */
IMPORT iap2UserConfig_t g_iap2UserConfig;
IMPORT int g_Quit;
IMPORT iap2TestAppleDevice_t g_iap2TestDevice;
IMPORT BOOL g_EAtesting;

/* **********************  functions ********************** */

/* helper API to remove multiple MediaItem entries */
LOCAL S32 iap2MediaItemDbCleanUp(U8 updateProgress)
{
    S32 rc = IAP2_OK;
    S32 ret = IAP2_OK;
    U16 i = 0;
    U16 j = 0;
    U16 mediaItemCnt = 0;

    if( updateProgress < 100 )
    {
        printf( " %u ms  iap2MediaItemDbCleanUp():  MediaLibUpdateProgress is only at %d /100 \n",
                iap2CurrTimeMs(), updateProgress );
        return IAP2_CTL_ERROR;
    }
    else if( ( g_iap2TestDevice.tmpMediaItem == NULL ) && ( g_iap2TestDevice.testMediaItem == NULL ) )
    {
        /* updateProgress is 100, but no MediaItem database was created */
        printf( " %u ms  iap2MediaItemDbCleanUp():  No MediaItem database available. tmpMediaItem is NULL \n",
                iap2CurrTimeMs() );
        return IAP2_CTL_ERROR;
    }
    else if( ( g_iap2TestDevice.tmpMediaItem != NULL ) && ( g_iap2TestDevice.testMediaItem == NULL ) )
    {
        pthread_mutex_lock( &g_iap2TestDevice.testMediaLibMutex );
        printf( " %u ms  iap2MediaItemDbCleanUp():  Get pure DB takes a while... \n", iap2CurrTimeMs() );
        g_iap2TestDevice.testMediaItem = calloc( g_iap2TestDevice.tmpMediaItemCnt, sizeof( iAP2MediaItem ) );
        if( g_iap2TestDevice.testMediaItem != NULL )
        {
            mediaItemCnt = 0;
            for( i=0; ( ( i < g_iap2TestDevice.tmpMediaItemCnt ) && ( rc == IAP2_OK ) ); i++ )
            {
                if( g_iap2TestDevice.tmpMediaItem[i].iAP2MediaItemPersistentIdentifier_count > 0 )
                {
                    /* check if PersistentId is still available */
                    for( j=0; ( ( j<mediaItemCnt )&&( ret == IAP2_OK ) ); j++ )
                    {
                        if( g_iap2TestDevice.testMediaItem[j].iAP2MediaItemPersistentIdentifier_count > 0 )
                        {
                            if( *( g_iap2TestDevice.testMediaItem[j].iAP2MediaItemPersistentIdentifier ) == *( g_iap2TestDevice.tmpMediaItem[i].iAP2MediaItemPersistentIdentifier ) )
                            {
                                ret = IAP2_CTL_ERROR;
                            }
                        }
                    }
                    if( ret == IAP2_OK )
                    {
                        mediaItemCnt++;
                        rc = iap2AllocateandUpdateData( &g_iap2TestDevice.testMediaItem[j].iAP2MediaItemPersistentIdentifier,
                                                        g_iap2TestDevice.tmpMediaItem[i].iAP2MediaItemPersistentIdentifier,
                                                        &g_iap2TestDevice.testMediaItem[j].iAP2MediaItemPersistentIdentifier_count,
                                                        g_iap2TestDevice.tmpMediaItem[i].iAP2MediaItemPersistentIdentifier_count,
                                                        iAP2_uint64 );
                        if( ( g_iap2TestDevice.tmpMediaItem[i].iAP2MediaItemTitle_count > 0 ) && ( rc == IAP2_OK ) )
                        {
                            rc = iap2AllocateandUpdateData( &g_iap2TestDevice.testMediaItem[j].iAP2MediaItemTitle,
                                                            g_iap2TestDevice.tmpMediaItem[i].iAP2MediaItemTitle,
                                                            &g_iap2TestDevice.testMediaItem[j].iAP2MediaItemTitle_count,
                                                            g_iap2TestDevice.tmpMediaItem[i].iAP2MediaItemTitle_count,
                                                            iAP2_utf8 );
                        }
                        if( ( g_iap2TestDevice.tmpMediaItem[i].iAP2MediaItemGenre_count > 0 ) && ( rc == IAP2_OK ) )
                        {
                            rc = iap2AllocateandUpdateData( &g_iap2TestDevice.testMediaItem[j].iAP2MediaItemGenre,
                                                            g_iap2TestDevice.tmpMediaItem[i].iAP2MediaItemGenre,
                                                            &g_iap2TestDevice.testMediaItem[j].iAP2MediaItemGenre_count,
                                                            g_iap2TestDevice.tmpMediaItem[i].iAP2MediaItemGenre_count,
                                                            iAP2_utf8 );
                        }
                    }
                    else
                    {
                        ret = IAP2_OK;
                    }
                }
            }
            if( rc == IAP2_OK )
            {
                g_iap2TestDevice.testMediaItemCnt = mediaItemCnt;
                rc = IAP2_OK;
            }
        }
        else
        {
            printf( " %u ms  iap2MediaItemDbCleanUp(): .testMediaItem is NULL \n", iap2CurrTimeMs() );
            rc = IAP2_ERR_NO_MEM;
        }
        /* Free temporary MediaItem database which was created with iAP2MediaLibraryUpdates */
        if( g_iap2TestDevice.tmpMediaItem != NULL )
        {
            for( i=0; i<g_iap2TestDevice.tmpMediaItemCnt; i++ )
            {
                if( &( g_iap2TestDevice.tmpMediaItem[i] ) != NULL )
                {
                    iAP2FreeiAP2MediaItem( &( g_iap2TestDevice.tmpMediaItem[i] ) );
                }
            }
            free( g_iap2TestDevice.tmpMediaItem );
            g_iap2TestDevice.tmpMediaItem = NULL;
            g_iap2TestDevice.tmpMediaItemCnt = 0;
        }
        pthread_mutex_unlock( &g_iap2TestDevice.testMediaLibMutex );
    }
    else
    {
        /* updateProgress is 100 and MediaItem database (testMediaItem) was created */
        /* nothing to do */
        rc = IAP2_OK;
    }

    return rc;
}

/* helper API for MediaLibraryUpdate and database handling */
S32 iap2MediaItemDbWaitForUpdate(U32 waitTime, BOOL waitToFinish)
{
    int rc = IAP2_OK;
    U32 retry_count = 0;
    U32 tmpWaitTime = 0;
    U32 callTime = 0;

    /* iap2SleepMs() set to 10ms
     * If waitTime is 5000, tmpWaitTime must be 500 to wait 5000ms */
    tmpWaitTime = (waitTime / 10);

    while( (rc == IAP2_OK) && (g_iap2TestDevice.testMediaLibUpdateProgress < 100)
            &&(iap2GetTestStateError() != TRUE) && (retry_count < tmpWaitTime) )
    {
        if( iap2GetTestDeviceState() == iAP2NotConnected )
        {
            rc = IAP2_DEV_NOT_CONNECTED;
            break;
        }
        iap2SleepMs( 10 );
        retry_count++;
        /* check if MediaLibraryUpdates callback was called */
        if( iap2GetTestState( MEDIA_LIB_UPDATE_RECV ) == TRUE )
        {
            if( g_iap2TestDevice.testMediaLibUpdateProgress == 100 )
            {
                /* MediaLibUpdate complete */
                iap2SleepMs( 10 );
            }
            else
            {
                /* MediaLibUpdate not completed */
                iap2SetTestState( MEDIA_LIB_UPDATE_RECV, FALSE );
                if( waitToFinish == TRUE )
                {
                    /* reset retry_count in case not all data were received */
                    retry_count = 0;
                }
                else
                {
                    if( g_iap2TestDevice.testMediaItemCnt > 10 )
                    {
                        /* Should not wait until MediaLibUpdate is complete.
                         * Timeout not reached.
                         * Some MediaItems received - done. */
                        rc = IAP2_OK;
                        break;
                    }
                }
            }
        }
    }
    if( g_iap2TestDevice.testMediaLibUpdateProgress < 100 )
    {
        printf( " %u ms  iap2MediaItemDbWaitForUpdate(): retry: %d | progress: (%d / 100)\n",
                iap2CurrTimeMs(), retry_count, g_iap2TestDevice.testMediaLibUpdateProgress );
    }
    else
    {
        /* don't do this in the callback. */
        callTime = iap2CurrTimeMs();
        rc = iap2MediaItemDbCleanUp( g_iap2TestDevice.testMediaLibUpdateProgress );
        printf( " %u ms  iap2MediaItemDbWaitForUpdate(): iap2MediaItemDbCleanUp() takes %u ms \n",
                iap2CurrTimeMs(), ( iap2CurrTimeMs() - callTime ) );
        printf( "\n Number of songs: %d \n", g_iap2TestDevice.testMediaItemCnt );
    }

    return rc;
}

S32 iap2StoreSerialNum(U8* product, U8* serial)
{
    S32 rc = 0;

    g_iap2TestDevice.ProductName = (U8*)strndup( (const char*)product, strnlen((const char*)product, STRING_MAX) );
    g_iap2TestDevice.SerialNumber = (U8*)strndup( (const char*)serial, strnlen((const char*)serial, STRING_MAX) );
    if((g_iap2TestDevice.ProductName == NULL) || (g_iap2TestDevice.SerialNumber == NULL))
    {
        rc = IAP2_ERR_NO_MEM;
        IAP2TESTDLTLOG(DLT_LOG_ERROR, "ProductName = %p, SerialNumber = %p",g_iap2TestDevice.ProductName,g_iap2TestDevice.SerialNumber);
    }

    return rc;
}

