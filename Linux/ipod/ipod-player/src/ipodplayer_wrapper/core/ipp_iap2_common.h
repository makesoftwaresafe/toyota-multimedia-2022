/****************************************************
 *  ipp_iap2_common.h
 *  Created on: 2014/01/22 17:32:00
 *  Implementation of the Class ipp_iap2_common
 *  Original author: madachi
 ****************************************************/

#ifndef __ipp_iap2_common_h__
#define __ipp_iap2_common_h__

#include <iap2_service_init.h>
#include <adit_dlt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sqlite3.h>

#include "adit_typedef_linux.h"
#include "iap2_defines.h"
#include "iap2_commands.h"
#include "iap2_parameters.h"
#include "iap2_parameter_free.h"
#include "iPodPlayerDef.h"
#include "iPodPlayerUtilityLog.h"

#ifdef DUMP_CHECK
void ipp_dump(U8 *pSrc, int byte);
#define MEM_DUMP(s,c) ipp_dump(s,c)
#else
#define MEM_DUMP(s,c)
#endif  /* DUMP_CHECK */

/* common definition */
/* Device identification */
#define IAP2_VENDOR_ID          0x01    /* vendor id */
#define IAP2_PRODUCT_ID         0x02    /* product id*/

/* iAP2 Library type table */
#define IPP_LIBTYPE_VAR             0xfe    /* type of variable size */
#define IPP_LIBTYPE_STOP            0xff    /* type table stop */

#define IPP_STRING_MAX              256     /* max string length */
#define IPP_CHAR_NULL_LEN           1       /* length for NULL strings */

#define IPP_IAP2_WAIT_100MS         100000000   /* 100ms wait for nanosleep */

#define IPP_IAP2_ASSISTIVE_ID       1       /* assistive id */

#define IPP_IAP2_ENABLE_AUTO_DETECT "1"     /* auto detect enable */


#define IPOD_PLAYER_IAP2_MAX_PROGRESS 100
#define IPOD_PLAYER_IAP2_TIMER_FOR_PROGRESS 200

#define IPOD_PLAYER_IAP2_DB_MAX_REVISION_LEN 256
#define IPOD_PLAYER_IAP2_DB_MAX_FILENAME_LEN 512

/************************************** Data mask ****************************/
#define IPP_IAP2_DATA_MASK_CONNECTION_STATUS                0x00000001
#define IPP_IAP2_DATA_MASK_PLAYBACK_STATUS                  0x00000002
#define IPP_IAP2_DATA_MASK_PLAYBACK_CHANGE                  0x00000004
#define IPP_IAP2_DATA_MASK_DEVICE_EVENT                     0x00000008
#define IPP_IAP2_DATA_MASK_COVERART_DATA                    0x00000010
#define IPP_IAP2_DATA_MASK_DB_ENTRIES                       0x00000020
#define IPP_IAP2_DATA_MASK_TRACK_INFO                       0x00000040
#define IPP_IAP2_DATA_MASK_IOSAPP_OPEN                      0x00000080
#define IPP_IAP2_DATA_MASK_IOSAPP_RECEIVE                   0x00000100
#define IPP_IAP2_DATA_MASK_IOSAPP_CLOSE                     0x00000200
#define IPP_IAP2_DATA_MASK_PLAYING_RATE                     0x00000400

#define IPP_IAP2_COVERART_FORMAT                            1      /* coverart format id */
#define IPP_IAP2_SUPPORTED_FEATURE_MASK                     (IPOD_PLAYER_FEATURE_MASK_IOSAPP | IPOD_PLAYER_FEATURE_MASK_DEVICE_NOTIFICATION | IPOD_PLAYER_FEATURE_MASK_APP_START)

#define IPP_IAP2_DEVICE_EVENT_MASK                          (IPOD_PLAYER_EVENT_MASK_RADIO_TAGGING | IPOD_PLAYER_EVENT_MASK_CAMERA      | IPOD_PLAYER_EVENT_MASK_CHARGING | \
                                                             IPOD_PLAYER_EVENT_MASK_DATABASE      | IPOD_PLAYER_EVENT_MASK_IOSAPP      | IPOD_PLAYER_EVENT_MASK_OUT      | \
                                                             IPOD_PLAYER_EVENT_MASK_BT            | IPOD_PLAYER_EVENT_MASK_IOSAPP_FULL | IPOD_PLAYER_EVENT_MASK_ASSISTIVE |\
                                                             IPOD_PLAYER_EVENT_MASK_SAMPLE_RATE   | IPOD_PLAYER_EVENT_MASK_STORE_DB    | IPOD_PLAYER_EVENT_MASK_UPDATE_PBLIST |\
                                                             IPOD_PLAYER_EVENT_MASK_DATABASE_AMR  | IPOD_PLAYER_EVENT_MASK_STORE_DB_AMR)
                                                            
#define IPP_IAP2_SUPPORTED_DEVICE_EVENT_MASK                (IPOD_PLAYER_EVENT_MASK_DATABASE    | IPOD_PLAYER_EVENT_MASK_BT         | IPOD_PLAYER_EVENT_MASK_IOSAPP_FULL |\
                                                             IPOD_PLAYER_EVENT_MASK_SAMPLE_RATE | IPOD_PLAYER_EVENT_MASK_ASSISTIVE  | IPOD_PLAYER_EVENT_MASK_STORE_DB    |\
                                                             IPOD_PLAYER_EVENT_MASK_UPDATE_PBLIST | IPOD_PLAYER_EVENT_MASK_DATABASE_AMR | IPOD_PLAYER_EVENT_MASK_STORE_DB_AMR)

#define IPP_IAP2_TRACK_INFO_MASK                            (IPOD_PLAYER_TRACK_INFO_MASK_TRACK_NAME  | IPOD_PLAYER_TRACK_INFO_MASK_ALBUM_NAME  | IPOD_PLAYER_TRACK_INFO_MASK_ARTIST_NAME   | \
                                                             IPOD_PLAYER_TRACK_INFO_MASK_DESCRIPTION | IPOD_PLAYER_TRACK_INFO_MASK_LYRIC       | IPOD_PLAYER_TRACK_INFO_MASK_PODCAST_NAME  | \
                                                             IPOD_PLAYER_TRACK_INFO_MASK_GENRE       | IPOD_PLAYER_TRACK_INFO_MASK_COMPOSER    | IPOD_PLAYER_TRACK_INFO_MASK_RELEASE_DATE  | \
                                                             IPOD_PLAYER_TRACK_INFO_MASK_CAPABILITY  | IPOD_PLAYER_TRACK_INFO_MASK_TRACKLENGTH | IPOD_PLAYER_TRACK_INFO_MASK_CHAPTER_COUNT | \
                                                             IPOD_PLAYER_TRACK_INFO_MASK_UID         | IPOD_PLAYER_TRACK_INFO_MASK_TRACK_KIND  | IPOD_PLAYER_TRACK_INFO_MASK_COVERART)
                                                            
#define IPP_IAP2_SUPPORTED_TRACK_INFO_MASK                  (IPOD_PLAYER_TRACK_INFO_MASK_TRACK_NAME   | IPOD_PLAYER_TRACK_INFO_MASK_ALBUM_NAME | IPOD_PLAYER_TRACK_INFO_MASK_ARTIST_NAME | \
                                                             IPOD_PLAYER_TRACK_INFO_MASK_PODCAST_NAME | IPOD_PLAYER_TRACK_INFO_MASK_GENRE      | IPOD_PLAYER_TRACK_INFO_MASK_COMPOSER    | \
                                                             IPOD_PLAYER_TRACK_INFO_MASK_TRACKLENGTH  | IPOD_PLAYER_TRACK_INFO_MASK_COVERART   | IPOD_PLAYER_TRACK_INFO_MASK_CHAPTER_COUNT | \
                                                             IPOD_PLAYER_TRACK_INFO_MASK_CAPABILITY)

typedef enum
{
    IPP_IAP2_NOTIFY_TYPE_DEVEVENT = 0x0001,
    IPP_IAP2_NOTIFY_TYPE_TRACKINFO,
    IPP_IAP2_NOTIFY_TYPE_UNKONWN
} IPP_IAP2_NOTIFY_TYPE;

#define IPP_SENDTOAPP_PAYLOAD_SIZE  10000       /* payload size for SendToApp API */


typedef struct _ippiAP2TypeTbl_t
{
    iAP2_Type   type;
    size_t      size;
} iAP2Type_t, *PiAP2Type_t;

typedef struct _TrackInfoMask_t
{
    U32     mask;       /* mask pattern */
    U16     formatId;   /* format ID */
} TrackInfoMask_t, *PTrackInfoMask_t;

typedef struct _IPP_IAP2_UPDATE_DATA_MASK
{
    U32 dataMask;
    TrackInfoMask_t trackInfoMask;
    U32 deviceEventMask;
    U32 blueToothIDMask;
} IPP_IAP2_UPDATE_DATA_MASK;


typedef struct _IPOD_PLAYER_CORE_IAP2_MEDIA_LIB_PARAM
{
    IPOD_PLAYER_MEDIA_LIB_INFO infoOfLocalDevice;
    IPOD_PLAYER_MEDIA_LIB_INFO infoOfAppleMusicRadio;
} IPOD_PLAYER_CORE_IAP2_MEDIA_LIB_PARAM;

typedef enum
{
    IPOD_PLAYER_XFER_TYPE_COVERART = 0,
    IPOD_PLAYER_XFER_TYPE_PLAYLIST_LOCAL,
    IPOD_PLAYER_XFER_TYPE_PLAYLIST_AMR,
    IPOD_PLAYER_XFER_TYPE_QUEUELIST,
    IPOD_PLAYER_XFER_TYPE_UNKNOWN = 0xFF
} IPOD_PLAYER_CORE_IAP2_FILE_XFER_TYPE;

typedef enum
{
    IPOD_PLAYER_XFER_STATUS_NONE = 0,
    IPOD_PLAYER_XFER_STATUS_START,
    IPOD_PLAYER_XFER_STATUS_DONE,
    IPOD_PLAYER_XFER_STATUS_CANCEL,
    IPOD_PLAYER_XFER_STATUS_FAIL
} IPOD_PLAYER_CORE_IAP2_FILE_XFER_STATUS;

typedef struct IAP2_FILE_XFER_TABLE_ST
{
    U8* Buffer;
    U8* CurPos;
    U64 CurReceived;
    U8 FileID;
    U64 FileSize;
}IAP2_FILE_XFER_TABLE;

typedef struct
{
    IPOD_PLAYER_CORE_IAP2_FILE_XFER_STATUS status;
    IPOD_PLAYER_CORE_IAP2_FILE_XFER_TYPE type;
    U64 targetID;
    U8 fileID;
    U64 totalSize;
    U64 curSize;
    U8 *buf;
} IPOD_PLAYER_CORE_IAP2_FILE_XFER_INFO;
    
typedef struct _IPOD_PLAYER_CORE_IAP2_FILE_XFER_LIST
{
    IPOD_PLAYER_CORE_IAP2_FILE_XFER_INFO info;
    struct _IPOD_PLAYER_CORE_IAP2_FILE_XFER_LIST *next;
    struct _IPOD_PLAYER_CORE_IAP2_FILE_XFER_LIST *prev;
} IPOD_PLAYER_CORE_IAP2_FILE_XFER_LIST;

typedef struct _IPOD_PLAYER_CORE_IAP2_PB_QUEUE_LIST
{
    U64 *trackIDBuff;
    int count;
    U8 FileID;
} IPOD_PLAYER_CORE_IAP2_PB_QUEUE_LIST;

typedef struct _IPOD_PLAYER_CORE_IAP2_POWER_INFO
{
    IPOD_PLAYER_CURRENT powermA;
    U8                  chargeButtery;
} IPOD_PLAYER_CORE_IAP2_POWER_INFO;

typedef struct
{
    sqlite3 *memoryHandle;
    sqlite3 *fileHandle;
    U8   fileName[IPOD_PLAYER_IAP2_DB_MAX_FILENAME_LEN];
} IPOD_PLAYER_IAP2_DB_MEDIA_LIBRARY_HANDLE;

typedef struct
{
    IPOD_PLAYER_IAP2_DB_MEDIA_LIBRARY_HANDLE localDevice;
    IPOD_PLAYER_IAP2_DB_MEDIA_LIBRARY_HANDLE appleMusicRadio;
    sqlite3 *iPodHandle;
} IPOD_PLAYER_IAP2_DB_HANDLE;

typedef enum
{
    IPOD_PLAYER_MEDIA_LIBRARY_TYPE_LOCAL = 0,
    IPOD_PLAYER_MEDIA_LIBRARY_TYPE_AMR
} IPOD_PLAYER_CORE_IAP2_MEDIA_LIBRARY_TYPE;

typedef struct
{
    IPOD_PLAYER_CORE_IAP2_MEDIA_LIBRARY_TYPE mediaLibraryType;
    U8   mediaUniqueIdentifier[IPOD_PLAYER_STRING_UID_LEN_MAX];
    U8   dbRevision[IPOD_PLAYER_IAP2_DB_MAX_REVISION_LEN];
    BOOL mediaChange;
    BOOL startMediaLib;
    BOOL mediaLibraryReset;
    BOOL dbUpdateProgress;
    BOOL dbUpdatePlaylist;
} IPOD_PLAYER_CORE_IAP2_DB_STATUS;

typedef struct _IPOD_PLAYER_CORE_IAP2_PARAM
{
    iAP2InitParam_t *initParam;
    uint32_t deviceId;
    iAP2Device_t *device;
    iAP2GetPollFDs_t pollFDs;
    IPOD_PLAYER_CORE_IAP2_MEDIA_LIB_PARAM *iap2MediaParam;
    IPOD_PLAYER_IAP2_DB_HANDLE *dbHandle;
    void *notifyFuncTable;
    IPP_IAP2_UPDATE_DATA_MASK updateDataMask;
    TrackInfoMask_t notifyTrackInfoMask;
    U32 notifyDeviceEventMask;
    IPOD_PLAYER_CORE_IAP2_FILE_XFER_LIST *xferList;
    U32 powerUpdateEventStatus;
    IPOD_PLAYER_CORE_IAP2_POWER_INFO powerInfo;
    IPOD_PLAYER_POWER_NOTIFY powerNotify;
    U32 playbackStatusActiveMask;
    U32 playbackStatusSetMask;
    U32  sampleRate;
    BOOL sampleRateStatus;
    IPOD_PLAYER_CORE_IAP2_DB_STATUS dbStatusOfLocalDevice;
    IPOD_PLAYER_CORE_IAP2_DB_STATUS dbStatusOfAppleMusicRadio;
}  IPOD_PLAYER_CORE_IAP2_PARAM;

#define IPP_IAP2_IS_MASK_SET(opt, mask) ((opt & mask) == mask )   /* Check wether the mask is set     */

BOOL ippiAP2CheckNullParameter(void *para_c, void *para_p);
BOOL ippiAP2CheckConnectionReady(IPOD_PLAYER_CONNECTION_STATUS *dc);
S32 ippiAP2AllocateandUpdateBlob(void *dstp, void *srcp, U16 *dstc, U16 srcc);
S32 ippiAP2AllocateandUpdateUtf8(void *dstp, void *srcp, U16 *dstc, U16 srcc);
S32 ippiAP2AllocateandUpdateData(void* dest_ptr, void* src_ptr, U16* dest_count, U16 src_count, iAP2_Type iAP2Type);
S32 ippiAP2RetConvertToiPP(S32 errcode);
S32 ippiAP2Wait100ms(int mstime);

S32 ippiAP2FileXferInit(IPOD_PLAYER_CORE_IAP2_FILE_XFER_LIST **list, IPOD_PLAYER_CORE_IAP2_FILE_XFER_TYPE type, U64 targetID, U8 fileID);
S32 ippiAP2FileXferDeinit(IPOD_PLAYER_CORE_IAP2_FILE_XFER_LIST **list, U8 fileID);
S32 ippiAP2FileXferDeinitAll(IPOD_PLAYER_CORE_IAP2_FILE_XFER_LIST **list);
IPOD_PLAYER_CORE_IAP2_FILE_XFER_INFO *ippiAP2FileXferGetInfo(IPOD_PLAYER_CORE_IAP2_FILE_XFER_LIST *list, U8 fileID);

#endif /*__ipp_iap2_common_h__*/
 
