#ifndef IAP_INIT_H
#define IAP_INIT_H

#include "iap_types.h"
#include "iap_transport_message.h"
#include "iap_transport_configuration.h"

#ifdef __cplusplus
extern "C" {
#endif

#define IPOD_IDPS_TOKEN_VALUE_FAILED 0x01
#define IPOD_IDPS_OPTIONAL_TOKEN_FAILED 0x02
#define IPOD_IDPS_TOKEN_NOT_SUPPORTED 0x03
#define IPOD_DIGITAL_REMOTE_LINGO_BITMASK 0x08
#define IPOD_DIGITAL_LINGO_BITMASK 0x04
#define IPOD_STORAGE_LINGO_BITMASK 0x10

#define IPOD_GENERAL_LINGO_OPTIONS_BITMASK_IOS_APP  0x0000000000002000ULL
#define IPOD_CAPS_IOS_SUPPORT                       0x0000000000000200ULL
#define IPOD_CAPS_CANCEL_SUPPORT                    0x0000000000080000ULL

#define IPOD_TOKEN_MAX_SIZE 9
#define IPOD_TOKEN_BASE_LEN 5
#define IPOD_TOKEN_NO_TYPE 3
#define IPOD_TOKEN_HAS_TYPE 4

#define IPOD_FID_ACCCAPS_LEN 0x0B
#define IPOD_FID_PREFER_LEN 0x06
#define IPOD_FID_PREFER_RESTORE 0x01
#define IPOD_FID_OPTIONAL_LINGO 0x0E

#define IPOD_FID_ID_LEN 0x0C

#define IPOD_FID_LENGTH_BYTE 1
#define IPOD_FID_STR_NULL_BYTE 1
#define IPOD_FID_AUTHENTICATE_OPTION 0x02
#define IPOD_FID_INCOMING_LEN 2
#define IPOD_FID_VERSION_LEN 3
#define IPOD_FID_ACCINFO_MAX 10
#define IPOD_FID_ACCINFO_LEN 3
#define IPOD_FID_SDK_LEN 4
#define IPOD_FID_BUNDLE_LEN 0x03
#define IPOD_FID_STR_NULL_LEN 1
#define IPOD_FID_METADATA_LEN 0x05
#define IPOD_FID_SCREEN_LEN 0x10
#define IPOD_FID_ACC_STATUS 0x04
#define IPOD_FID_RF_CERTIFICATIONS 0x07

/* FID infoByte defines */
#define IPOD_FID_INFO_BYTE_LEN 2
#define IPOD_FID_ID_INFOBYTE        0x00, 0x00
#define IPOD_FID_ACCCAPS_INFOBYTE   0x00, 0x01
#define IPOD_FID_ACCINFO_INFOBYTE   0x00, 0x02
#define IPOD_FID_PREFER_INFOBYTE    0x00, 0x03
#define IPOD_FID_SDK_INFOBYTE       0x00, 0x04
#define IPOD_FID_BUNDLE_INFOBYTE    0x00, 0x05
#define IPOD_FID_SCREEN_TOKEN       0x00, 0x07
#define IPOD_FID_METADATA_TOKEN     0x00, 0x08
#define IPOD_FID_MICRO_INFOBYTE     0x01, 0x00
/* to over-ride the FID_ACC_CAPS audio setting in case of "iPodSwitchAudio" command */
#define IPOD_ACC_CAPS_NO_DIGITAL_AUDIO  0xffffffef /* by AND operation, Digital Audio can be disabled in accessory capabilities */
#define IPOD_ACC_CAPS_DIGITAL_AUDIO     0x00000010 /* by OR operation, Digital Audio can be enabled in accessory capabilities */
#define IPOD_ACC_CAPS_NO_LINE_OUT       0xfffffffe /* by AND operation, Line-Out can be disabled in accessory capabilities */
#define IPOD_ACC_CAPS_LINE_OUT          0x00000001 /* by OR operation, Line-Out can be enabled in accessory capabilities */
#define IPOD_ACC_CAPS_IOS_SUPPORT       0xfffffdff /* by AND operation, iOS support can be disabled */
#define IPOD_ACC_CAPS_CANCEL_SUPPORT    0xfff7ffff /* by AND operation, multi-packet and CancelCmd support can be disabled*/

#define IPOD_SET_IOS_INFO_TIME 10000

S32  iPodInitConnection(void);
S32 iPodInitAccessoryConnection(IPOD_ACC_INFO_CONFIGURATION acc_info);
S32 iPodInitDeviceConnection(U8* devicename, IPOD_CONNECTION_TYPE connection);
void iPodDisconnect(void);
void iPodReidentify(IPOD_INSTANCE* iPodHndl);
void iPodSwitchAudioOutput(U32 iPodID, IPOD_SWITCH_AUDIO_OUT switchAudio);

S32 iPodGetRemoteUIMode_internal(IPOD_INSTANCE* iPodHndl);
S32 iPodGetLingoProtocolVersion_internal(IPOD_INSTANCE* iPodHndl, IPOD_LINGO lingo,
                                         U8* majorVer, U8* minorVer);
S32 iPodGetiPodOptionsForLingo_internal(IPOD_INSTANCE* iPodHndl, 
                                        IPOD_LINGO lingo,
                                        U64 *optionBits);
S32 iPodSetiPodPreferences_internal(IPOD_INSTANCE* iPodHndl,
                                    IPOD_PREFERENCE_CLASS_ID classId,
                                    IPOD_PREFERENCE_SETTING_ID settingId,
                                    U8 restore);
S32 iPodSetConfiOSApp(U8 *devicename, IPOD_IOS_APP *iOSInfo, S8 numApps);

void iPodRequestIdentify(U32 iPodID);

U16 iPodGetMaxPayloadSize(U32 iPodID);

#ifdef __cplusplus
}
#endif

#endif /* IAP_INIT_H */
