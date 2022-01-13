#include <stdio.h>
#include <adit_typedef.h>
#include "iap2_init.h"
#include "iap2_usb_role_switch.h"
#include "iap2_test_utility.h"
#include "iap2_test_config.h"
#include "iap2_test_defines.h"
#include "iap2_usb_gadget_load_modules.h"
#include <mqueue.h>

#define IAP2_USE_CONFIGFS

#define FUNCTION_FS_NAME                        "ffs"
#define FUNCTION_FS_PATH                        "/dev/ffs"
#define FUNCTION_FS_TYPE                        "functionfs"

#define MAX_DEVICES                                             8

#define IAP2_ACC_CONFG_TRANS_USB_OTG_GPIO_POWER_AI          "425"
#define IAP2_ACC_CONFG_TRANS_USB_OTG_GPIO_POWER_SD          "86"

/* g_ffs load module parameter */
#define STR_ACC_INFO_NAME                       "AmazingProduct"
#define STR_ACC_INFO_MANUFACTURER               "ADIT"
#define STR_ACC_INFO_SERIAL_NUM                 "12345678"
#define STR_ACC_INFO_VENDOR_ID                  "44311"
#define STR_ACC_INFO_PRODUCT_ID                 "1111"
#define STR_ACC_INFO_BCD_DEVICE                 "1"
#define STR_ACC_INFO_QMULT                      "1"


#define IAP2_HOST_DEVICE_SWITCH_CMD__SWITCH_HOST   0
#define IAP2_HOST_DEVICE_SWITCH_CMD__SWITCH_DEVICE 1

#define IAP2_HOST_DEVICE_SWITCH_UDEV_PATH_MAX_LEN 1024

#define IAP2APPSENDSWITCHMSG_STRING_CP(dest, src) \
{ \
    if (strlen((char*)src) >= (sizeof(dest)/sizeof(dest[0]))) { \
        printf ("%s is to large to be copied to dest\n", #src ); \
        rc = IAP2_CTL_ERROR; \
    } else {\
        strcpy ((void*)dest, (void*)src); \
    } \
}


typedef struct
{
    int cmd;
    char udev_path[IAP2_HOST_DEVICE_SWITCH_UDEV_PATH_MAX_LEN];
    BOOL iAP2iOSintheCar;
    iap2LoadModuleParameters iap2GadgetParams;
} iAP2_host_device_switch_cmd_msg_t;

typedef struct
{
    S32 result;
} iAP2_host_device_switch_result_msg_t;

typedef enum
{
    ERROR = 0x00,
    STOPPED,
    RUNNING,
    MEDIA_LIB_INFO_RECV,
    MEDIA_LIB_UPDATE_RECV,
    NOW_PLAYING_UPDATE_RECV,
    BT_CONNECTION_UPDATE_RECV,
    PLAY_MEDIA_LIB_ITEM,
    USB_DEVICE_MODE_AUDIO_INFO_RECV,
    FILE_TRANSFER_DATA_RECV,
    FILE_TRANSFER_SUCCESS_RECV,
    FILE_TRANSFER_CANCELED_RECV,
    FILE_TRANSFER_FAILED_RECV,
    FILE_TRANSFER_PAUSED_RECV,
    FILE_TRANSFER_RESUMED_RECV,
    EAP_SESSION_START_RECV,
    EAP_SESSION_STOP_RECV,
    EAP_SESSION_DATA_RECV,
    START_LOC_INFO,
    STOP_LOC_INFO,
    VEHICLE_STATUS_UPDT_RECV,
    EA_NATIVE_TRANSPORT_START_RECV,
    EA_NATIVE_TRANSPORT_STOP_RECV,
    MAX_ENUM_VALUE
} iap2TestStates_t;


typedef struct
{
    U8* Buffer;
    U8* CurPos;
    U64 CurReceived;
    U8  FileID;
    U64 FileSize;
} iap2FileXferBuf;

typedef struct
{
    U8*  Buffer;
    U8*  CurPos;
    U64  CurReceived;
    U8   TransferID;
    U64  Size;
    BOOL transferred;
} iap2PlaybackQueueList_t;

typedef struct
{
    TEST_THREAD_ID AppTskID;
    mqd_t mqAppTskFd;
    mqd_t mqComTskFd;

    /* iAP2 library device structure */
    iAP2Device_t* iap2Device;
    /* Serial number of the connected device */
    U8* SerialNumber;
    /* Product name of the connected device */
    U8* ProductName;
    /* structure includes serial number and product name */
    iap2UdevDevice_t udevDevice;

    /* Mutex to protect access to MediaItem database */
    pthread_mutex_t testMediaLibMutex;
    /* Media Library Information Identifier */
    U8* testMediaLibInfoID;
    /* Number of MediaItem after MediaLibraryUpdates completed */
    U16 testMediaItemCnt;
    /* MediaItem database after MediaLibraryUpdates completed */
    iAP2MediaItem* testMediaItem;
    /* Percentage completion of MediaLibraryUpdates */
    U8 testMediaLibUpdateProgress;

    /* Number of MediaItem during MediaLibraryUpdates */
    U16 tmpMediaItemCnt;
    /* MediaItem database during MediaLibraryUpdates */
    iAP2MediaItem* tmpMediaItem;

    /* Buffer for CoverArt during file transfer */
    iap2FileXferBuf coverArtBuf;

    /* Test states */
    BOOL testStates[MAX_ENUM_VALUE];
    iAP2DeviceState_t testDeviceState;
    BOOL iap2USBHostMode;
    U8 EaProtocolID;
    U16 EAPSessionIdentifier;
    U8* AppBundleID;

    U8 EANativeTransportAppId;
    U8 SinkEndpoint;
    U8 SourceEndpoint;
    U8* testMediaLibLastKnownRev;
    U16 HIDComponentIdentifier;
    BOOL iap2PlayAllSongsCapable;
    BOOL iap2SetElapsedTimeAvailable;
    U32  iap2PlaybackQueueIndex;
    BOOL iap2TestiOS8;

    U32 DataBaseTransfer_StartTime_ms;
    U32 FirstDataBaseTransfer_ReceivedTime_ms;
    U32 PlayMediLibraryItems_StartTime_ms;
    usbConnectStateType_t type;

    iap2PlaybackQueueList_t playbackQueueList;
} iap2TestAppleDevice_t;


typedef struct
{
    U8 EAPid;
    U8* EAPmsg;
    U32 EAPmsglen;
} iap2TestEAP_t;

/* add file descriptor to array */
S32 iap2AddFDToPollFDs(iAP2PollFDs_t* getPollFDs, S32 numFDs,
                       int fdToAdd, S16 eventToAdd);
/* add file descriptors to FD_SET */
S32 iap2AddFDsToFDset(iAP2PollFDs_t* getPollFDs, S32 numFDs, S32* maxfd,
                            fd_set* to_readfds, fd_set* to_writefds);

void SetGlobPtr(iap2TestAppleDevice_t *iap2TestAppleDevice);
void iap2SetTestState(iap2TestStates_t setState, BOOL value);
BOOL iap2GetTestState(iap2TestStates_t getState);
void iap2SetiOS8testing(BOOL value);
BOOL iap2GetiOS8testing(void);
void iap2SetTestStateError(BOOL value);
BOOL iap2GetTestStateError(void);
void iap2SetTestDeviceState(iAP2DeviceState_t testDeviceState);
iAP2DeviceState_t iap2GetTestDeviceState(void);
void signalHandlerAppProcess(int signo);
void iap2SetGlobalQuit(BOOL value);
usbConnectStateType_t iap2ConnectionType(void);
BOOL iap2GetGlobalQuit(void);
void iap2SetEAtesting(BOOL value);
BOOL iap2GetEAtesting(void);
S32 iap2AppSendSwitchMsg (int cmd, int socket_fd, const U8* udevPath, const iAP2InitParam_t* iAP2InitParameter);
void host_device_switch_process_main (int socket_fd, iap2UserConfig_t *iap2UserConfig);
int iap2GetArguments(int argc, const char** argv, iap2UserConfig_t *iap2UserConfig);
int iap2InitDev(iAP2InitParam_t* iAP2InitParameter);
void iap2DeinitDev(iAP2InitParam_t* iAP2InitParameter);
void iap2USBSwitchOffOn(U8* udevPath);
BOOL iap2InHostMode(void);
