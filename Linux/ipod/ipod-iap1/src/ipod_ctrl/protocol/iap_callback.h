#ifndef IAP_CALLBACK_H
#define IAP_CALLBACK_H

#include <adit_typedef.h>

#include "iap_types.h"
#include "ipodcommon.h"
#include "iap_transport_os.h"
#include "iap_transport_message.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================== */
/* defines                                                                    */
/* ========================================================================== */
#define IPOD_WRITE_BINARY       "wb"
#define IPOD_BMP_FILE_ID        "BM"
#define IPOD_BMP_24BIT           3
#define IPOD_BMP_BIT_COUNT      24
#define IPOD_BMP_HEADER_SIZE    40
#define IPOD_BMP_OFFSET         54
#define IPOD_BMP_ALIGN           4

#define IPOD_RGB_R_MASK  0xF800
#define IPOD_RGB_R_SHIFT 11
#define IPOD_RGB_G_MASK  0x07E0
#define IPOD_RGB_G_SHIFT 5
#define IPOD_RGB_B_MASK  0x001F
#define IPOD_RGB_8 8
#define IPOD_RGB_4 4
#define IPOD_RGB_3 3
#define IPOD_RGB_2 2

#define IPOD_RGB_LENGTH 3

/* ========================================================================== */
/* function prototypes                                                        */
/* ========================================================================== */

/* ========================================================================== */
/* Callback registration functions                                            */
/* ========================================================================== */

S32 iPodRegisterCBUSBAttach(IPOD_CB_USB_ATTACH callback);
S32 iPodRegisterCBUSBDetach(IPOD_CB_USB_DETACH callback);

S32 iPodRegisterCBNotifyStatus(IPOD_CB_NOTIFY const callback);
S32 iPodRegisterCBNotifyStateChange(IPOD_CB_NOTIFY_STATE_CHANGE const callback);
S32 iPodRegisterCBGetAccSampleRateCaps(IPOD_CB_GET_ACC_SAMPLE_RATE_CAPS const callback);
S32 iPodRegisterCBNewiPodTrackInfo(IPOD_CB_NEW_TRACK_INFO const callback);
S32 iPodRegisterCBTrackArtworkData(IPOD_CB_GET_ARTWORK const callback);
S32 iPodRegisterCBLocation(IPOD_CB_LOCATION const callback);
S32 iPodRegisterCBNotification(IPOD_CB_NOTIFICATION const callback);
S32 iPodRegisterCBRemoteEventNotification(IPOD_CB_REMOTE_EVENT_NOTIFICATION const callback);

S32 iPodRegisterCBOpenDataSession(IPOD_CB_OPEN_DATA_SESSION const callback);
S32 iPodRegisterCBCloseDataSession(IPOD_CB_CLOSE_DATA_SESSION const callback);
S32 iPodRegisterCBiPodDataTransfer(IPOD_CB_IPOD_DATA_TRANSFER const callback);
S32 iPodRegisterCBSetAccStatusNotification(IPOD_CB_SET_ACC_STATUS const callback);
S32 iPodExecuteCBOpenDataSession(const IPOD_INSTANCE* iPodHndl, U8 protocolIndex, U16 sessionId);
S32 iPodExecuteCBCloseDataSession(const IPOD_INSTANCE* iPodHndl, U16 sessionId);
S32 iPodExecuteCBiPodDataTransfer(const IPOD_INSTANCE* iPodHndl, U16 sessionId, U8 *data, U16 length);
S32 iPodExecuteCBSetAccStatusNotification(const IPOD_INSTANCE* iPodHndl, U32 statusMask);

void iPodFreeArtworkData(IPOD_INSTANCE* iPodHndl);
void iPodSetTrackArtworkDataImageSaveParams(U32 iPodID, U8 saveAsBMP, U8* imagePath);
/* ========================================================================== */
/* Callback execution functions (called by the iPod response ReaderTask)      */
/* ========================================================================== */

S32 iPodExecuteCBUSBAttach(const IPOD_INSTANCE* iPodHndl, const S32 success);
S32 iPodExecuteCBUSBDetach(const IPOD_INSTANCE* iPodHndl);

S32 iPodExecuteCBNotify(const IPOD_INSTANCE* iPodHndl, IPOD_CHANGED_PLAY_STATUS status, U64 param);
S32 iPodExecuteCBNotifyStateChange(const IPOD_INSTANCE* iPodHndl, IPOD_STATE_CHANGE stateChange);
S32 iPodExecuteCBGetACCSampleRateCaps(const IPOD_INSTANCE* iPodHndl);
S32 iPodExecuteCBNewTrackInfo(const IPOD_INSTANCE* iPodHndl, const U8 *iPodResponseBuffer);
S32 iPodExecuteCBGetTrackArtworkData(IPOD_INSTANCE* iPodHndl, const U8* iPodResponseBuffer, U16 len);
S32 iPodExecuteCBGetTrackArtworkDataEx(IPOD_INSTANCE* iPodHndl, const U8* iPodResponseBuffer, U16 len);

S32 iPodExecuteCBLocation(const IPOD_INSTANCE* iPodHndl, IPOD_LOCATION_CMD locCmd, IPOD_LOCATION_TYPE locType, U8 dataType, U8* locData, U32 size);
S32 iPodExecuteCBNotification(const IPOD_INSTANCE* iPodHndl, IPOD_NOTIFY_TYPE type, IPOD_NOTIFY_STATUS status);
S32 iPodExecuteCBRemoteEventNotify(const IPOD_INSTANCE* iPodHndl, IPOD_STATE_INFO_TYPE eventNum, IPOD_REMOTE_EVENT_NOTIFY_STATUS eventData);


typedef void (*IPOD_CB_INT_USB_ATTACH) (const S32 success, IPOD_CONNECTION_TYPE connection, const S32 id);
typedef void (*IPOD_CB_INT_USB_DETACH) (const S32 id);
typedef void (*IPOD_CB_INT_PLAYING_TRACK_INFO) (IPOD_TRACK_INFO_TYPE infoType,
                                                const IPOD_TRACK_CAP_INFO_DATA* capInfoData,
                                                const IPOD_TRACK_RELEASE_DATE_DATA* releaseData,
                                                const IPOD_TRACK_ARTWORK_COUNT_DATA* artworkCountData,
                                                U8* stringBuf, const S32 id);
typedef void (*IPOD_CB_INT_RETRIEVE_CAT_DB_RECORDS) (U32 index, U8* string, const S32 id);
typedef void (*IPOD_CB_INT_NOTIFY) (IPOD_CHANGED_PLAY_STATUS status, U64 param, const S32 id);
typedef void (*IPOD_CB_INT_GET_ARTWORK) (IPOD_ALBUM_ARTWORK* artworkData, const S32 id);
typedef void (*IPOD_CB_INT_NOTIFY_STATE_CHANGE) (IPOD_STATE_CHANGE stateChange, const S32 id);
typedef void (*IPOD_CB_INT_GET_ACC_SAMPLE_RATE_CAPS) (const S32 id);
typedef void (*IPOD_CB_INT_NEW_TRACK_INFO) (U32 newSampleRate,
                                            S32 newSoundCheckValue,
                                            S32 newTrackVolAdjustment, const S32 id);
typedef void (*IPOD_CB_INT_IPOD_ACK) (IPOD_ACC_ACK_STATUS status, const S32 id);
typedef S32  (*IPOD_CB_INT_LOCATION) (IPOD_LOCATION_CMD locCmd, IPOD_LOCATION_TYPE locType, U8 dataType, U8 *locData, U32 size, const S32 id);
typedef void (*IPOD_CB_INT_NOTIFICATION) (IPOD_NOTIFY_TYPE type, IPOD_NOTIFY_STATUS status, const S32 id);
typedef void (*IPOD_CB_INT_REMOTE_EVENT_NOTIFICATION) (IPOD_STATE_INFO_TYPE eventNum, IPOD_REMOTE_EVENT_NOTIFY_STATUS eventData, const S32 id);
typedef S32  (*IPOD_CB_INT_OPEN_DATA_SESSION) (U8 protocolIndex, U16 sessionId, const S32 id);
typedef void (*IPOD_CB_INT_CLOSE_DATA_SESSION) (U16 sessionId, const S32 id);
typedef S32  (*IPOD_CB_INT_IPOD_DATA_TRANSFER) (U16 sessionId, U8 *data, U16 length, const S32 id);
typedef void (*IPOD_CB_INT_SET_ACC_STATUS) (U32 statusMask, const S32 id);
typedef void (*IPOD_TRACK_INFORMATION_CB_INT) (U64 trackIndex, IPOD_TRACK_INFORMATION_TYPE infoType, IPOD_TRACK_INFORMATION_DATA *infoData, const S32 id);

#ifdef __cplusplus
}
#endif

#endif /* IAP_CALLBACK_H */
