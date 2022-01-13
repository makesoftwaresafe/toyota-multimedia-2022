#ifndef IAP_GENERAL_H
#define IAP_GENERAL_H

#include <adit_typedef.h>

#include "iap_types.h"
#include "iap_transport_message.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================== */
/* defines                                                                    */
/* ========================================================================== */

/* for "accessory info" ----------------------------------------------------- */
#define    IPOD_ACC_BIT_INFO_CAPABILITIES               0x01
#define    IPOD_ACC_BIT_NAME                            0x02
#define    IPOD_ACC_BIT_MIN_SUPP_IPOD_FIRMWARE_VER      0x04
#define    IPOD_ACC_BIT_MIN_SUPP_LINGO_VER              0x08
#define    IPOD_ACC_BIT_FIRMWARE_VERSION                0x10
#define    IPOD_ACC_BIT_HARDWARE_VERSION                0x20
#define    IPOD_ACC_BIT_MANUFACTURER                    0x40
#define    IPOD_ACC_BIT_MODEL_NUMBER                    0x80
#define    IPOD_ACC_BIT_SERIAL_NUMBER                   0x100
#define    IPOD_ACC_BIT_INCOMING_MAX_PAYLOAD_SIZE       0x200
#define IPOD_ACC_BIT_STATUS_TYPES                       0x800

#define IPOD_SUP_GENERALLINGO_MAJ 1
#define IPOD_SUP_GENERALLINGO_MIN 2
#define IPOD_SUP_EXTENDEDLINGO_MAJ 1
#define IPOD_SUP_EXTENDEDLINGO_MIN 5
#define IPOD_SUP_AUDIOLINGO_MAJ 1
#define IPOD_SUP_AUDIOLINGO_MIN 1
#define IPOD_SUP_DEFAULTLINGO_MAJ 1
#define IPOD_SUP_DEFAULTLINGO_MIN 0
#define IPOD_SUP_STORAGELINGO_MAJ 1
#define IPOD_SUP_STORAGELINGO_MIN 2
#define IPOD_SUP_LOCATIONLINGO_MAJ 1
#define IPOD_SUP_LOCATIONLINGO_MIN 0

#define IPOD_SUP_FW_VER_MAJ 0xFF
#define IPOD_SUP_FW_VER_MIN 0xFF
#define IPOD_SUP_FW_VER_REV 0xFF

#define IPOD_CMD_ACK_PENDING 0x06
#define IPOD_MODEL_NUM_BASE_LENGTH 4
#define IPOD_ACC_INFO_BASE_LENGTH 4
#define IPOD_GET_AUTH_LEN 3
#define IPOD_SIG_RETRY_COUNT 2

#define IPOD_ACC_STATUS_BASE_LEN 3
#define IPOD_DEV_DATA_TRANSFER_LEN 4
#define IPOD_REQ_APP_LAUNCH 5
#define IPOD_DEV_DATA_MAX_LEN 500
#define IPOD_MODE_CHANGE_WAIT_500MS 500
#define IPOD_RET_ACCINFO_MAX 0x0C

/* ========================================================================== */
/* function prototypes                                                       */
/* ========================================================================== */
S32  iPodEnterExtendedInterfaceMode(U32 iPodID);
S32  iPodEnterSimpleMode(U32 iPodID);
S32  iPodGetRemoteUIMode(U32 iPodID);
S32  iPodGetModelNum(U32 iPodID, U8* modelString);
S32  iPodGetLingoProtocolVersion(U32 iPodID, IPOD_LINGO lingo, U8* majorVer, U8* minorVer);
S32  iPodGetSoftwareVersion(U32 iPodID, U8* majorVer, U8* minorVer, U8* revisionVer);
S32  iPodGetSerialNumber(U32 iPodID, U8* serialNumber);
S32  iPodGetIPodName(U32 iPodID, U8* iPodName);
void iPodRetAccessoryInfo(IPOD_INSTANCE* iPodHndl, const U8* iPodData);
S32 iPodRetAccStatusNotification(U32 iPodID, U32 statusMask);
S32 iPodAccStatusNotification(U32 iPodID, IPOD_ACC_STATUS_TYPE type, IPOD_ACC_STATUS_PARAM *status, U16 len);
S32 iPodDevDataTransfer(U32 iPodID, U16 sessionId, U8 *data, U16 dataLen);
S32 iPodCancelCommand(U32 iPodID, IPOD_CANCEL_COMMAND_TYPE command);
S32 iPodRequestAppLaunch(U32 iPodID, U8 *bundleId, U8 length);
S32 iPodGetNowPlayingFocusApp(U32 iPodID, U8 *focusApp, U16 length);

S32 iPodIsInAdvancedMode(IPOD_INSTANCE* iPodHndl);
S32 iPodAuthenticateiPod(U32 iPodID);
S32 iPodGetiPodOptions(U32 iPodID, IPOD_OPTIONS_BIT *optionBits);
S32 iPodGetiPodPreferences(U32 iPodID, IPOD_PREFERENCE_CLASS_ID classId, IPOD_PREFERENCE_SETTING_ID *settingId);
S32 iPodSetiPodPreferences(U32 iPodID, IPOD_PREFERENCE_CLASS_ID classId, IPOD_PREFERENCE_SETTING_ID settingId, U8 restore);
S32 iPodSetEventNotification(U32 iPodID, U64 mask);
S32 iPodGetiPodOptionsForLingo(U32 iPodID, IPOD_LINGO lingo, U64 *optionBits);
S32 iPodGetEventNotification(U32 iPodID, U64 *eventMask);
S32 iPodGetSupportedEventNotification(U32 iPodID, U64 *eventMask);
S32 iPodSessionDevAck(IPOD_INSTANCE* iPodHndl, U8 cmdId, U8 status);
S32 iPodRequestTransportMaxPacketSize(U32 iPodID, U16 *size);
S32 iPodGetUIMode(U32 iPodID, U8 *mode);
S32 iPodSetUIMode(U32 iPodID, U8 mode);

#ifdef __cplusplus
}
#endif

#endif /* IAP_GENERAL_H */
