
#ifndef IAP2_TEST_UTILITY_H_
#define IAP2_TEST_UTILITY_H_

/* **********************  includes  ********************** */
#include "iap2_test_defines.h"
#include "iap2_init.h"

#include <adit_typedef.h>
#include <sys_time_adit.h>
#include <pthread_adit.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>

#include <sys/syscall.h>
#include <sys/utsname.h>
#include <sys/mount.h>
#include <sys/poll.h>
#include <mqueue.h>

/* used for device detection */
#include <libudev.h>
/* used for device power request */
#include <libusb-1.0/libusb.h>


/* **********************  defines  ********************** */


/* **********************  variables  ********************* */


typedef struct
{
    /* filename of the /sys entry for the Apple device */
    U8* udevPath;
    U8* serialNum;
    U8* productName;
    U16 vendorId;
    U16 productId;
} iap2UdevDevice_t;

typedef enum
{
    MQ_CMD_ERROR = 0xFF,
    MQ_CMD_ACK = 0x00,
    MQ_CMD_EXIT_IAP2_COM_THREAD = 0x01,
    MQ_CMD_INIT_DEVICE_CONNECTION = 0x02,
    MQ_CMD_DISCONNECT_DEVICE = 0x03,         // do not use. otherwise strange behavior like seg-fault.
    MQ_CMD_CANCEL_IAP1_SUPPORT = 0x04,
    MQ_CMD_SEND_EAP_SESSION_MSG = 0x05,
    MQ_CMD_START_NOWPLAYING_UPDATE = 0x06,
    MQ_CMD_STOP_NOWPLAYING_UPDATE = 0x07,
    MQ_CMD_START_MEDIALIB_INFO = 0x08,
    MQ_CMD_STOP_MEDIALIB_INFO = 0x09,
    MQ_CMD_START_MEDIALIB_UPDATE = 0x0A,
    MQ_CMD_STOP_MEDIALIB_UPDATE = 0x0B,
    MQ_CMD_START_HID = 0x0C,
    MQ_CMD_STOP_HID = 0x0D,
    MQ_CMD_SEND_HID_REPORT = 0x0E,
    MQ_CMD_START_USB_DEVICEMODE_AUDIO = 0x0F,
    MQ_CMD_START_PLAY_MEDIALIB_ITEM = 0x10,
    MQ_CMD_APP_LAUNCH = 0x11,
    MQ_CMD_CANCEL_FILE_TRANSFER = 0x12,
    MQ_CMD_PAUSE_FILE_TRANSFER = 0x13,
    MQ_CMD_RESUME_FILE_TRANSFER = 0x14,
    MQ_CMD_POWER_SOURCE_UPDATE = 0x15,
    MQ_CMD_START_POWER_UPDATE = 0x16,
    MQ_CMD_STOP_POWER_UPDATE = 0x17,
    MQ_CMD_PLAY_MEDIA_LIBRARY_SPECIAL = 0x18,
    MQ_CMD_SET_NOW_PLAYING_INFORMATION = 0x19,
    MQ_CMD_STATUS_EXTERNAL_ACCESSORY_PROTOCOL_SESSION = 0x1A,
    MQ_CMD_BT_COMP_INFO = 0x1B,
    MQ_CMD_START_BT_UPDATE = 0x1C,
    MQ_CMD_STOP_BT_UPDATE = 0x1D,
    MQ_CMD_START_TELEPHONY_CALLSTATE_INFO = 0x1E,
    MQ_CMD_START_TELEPHONY_UPDATE = 0x1F,
    MQ_CMD_STOP_TELEPHONY_CALLSTATE_INFO = 0x20,
    MQ_CMD_STOP_TELEPHONY_UPDATES = 0x21,
    MQ_CMD_LOCATION_INFO = 0x22,
    MQ_CMD_VEHICLE_STATUS_UPDATE = 0x23,
    MQ_CMD_START_COMMUNICATIONS_UPDATE = 0x24,
    MQ_CMD_STOP_COMMUNICATIONS_UPDATE = 0x25,
    MQ_CMD_START_CALL_STATE_UPDATES = 0x26,
    MQ_CMD_STOP_CALL_STATE_UPDATES = 0x27,
    MQ_CMD_START_LIST_UPDATES = 0x28,
    MQ_CMD_STOP_LIST_UPDATES = 0x29,
    MQ_CMD_INITIATE_CALL = 0x2A,
    MQ_CMD_ACCEPT_CALL = 0x2B,
    MQ_CMD_MERGE_CALLS = 0x2C,
    MQ_CMD_SWAP_CALLS = 0x2D,
    MQ_CMD_END_CALL = 0x2E,
    MQ_CMD_SEND_DTMF = 0x2F,
    MQ_CMD_HOLD_STATUS_UPDATE = 0x30,
    MQ_CMD_MUTE_STATUS_UPDATE = 0x31,
    MQ_CMD_WIFI_CONFIGURATION_INFORMATION = 0x32,
    MQ_CMD_OOB_BT_PAIRING_ACC_INFO = 0x33,
    MQ_CMD_OOB_BT_PAIRING_COMPLETION_INFO = 0x34,
    MQ_CMD_BT_PAIRING_ACC_INFO = 0X35,
    MQ_CMD_BT_PAIRING_STATUS = 0X36,
    MQ_CMD_START_APP_DISCOVERY = 0x37,
    MQ_CMD_STOP_APP_DISCOVERY = 0x38,
    MQ_CMD_REQUEST_APP_DISCOVERY_UPDATES = 0x39
} iap2ComThreadCmds_t;


typedef struct
{
    iap2ComThreadCmds_t mq_cmd;
    size_t param_size;
    void* param;
} iap2ComThreadMq_t;

/* **********************  functions  ********************* */

void iap2SetUtilityQuit(BOOL value);
BOOL iap2GetUtilityQuit(void);

/* provide the structure with serial nr., product name, vendor and product ID */
S32 iap2GetUdevDevice(U16 index, iap2UdevDevice_t* udevDevice);
/* free the udevDevice structure */
void iap2FreeUdevDevice(U16 index, iap2UdevDevice_t* udevDevice);
/* free allocated memory in the global structure */
void iap2FreeDetectDevice(void);

/* check USB port for connected iPods */
S32 iap2CheckForDevice(struct udev *udev);
/* return the number of detected Apple devices */
S32 iap2DetectDevice(void);

/* time tracking functions */
U32 iap2CurrTimeMs(void);
U32 iap2CurrTimeValToMs(struct timeval* time);

/* sleep in milliseconds */
void iap2SleepMs(U32 sleep_ms);

/* helper function to verify thread id */
S32 iap2VerifyThreadId(TEST_THREAD_ID threadID);
/* helper function to create threads */
TEST_THREAD_ID iap2CreateThread(void* thread_addr, char* thread_name, void* exinf);
/* helper function to create mq */
S32 iap2CreateMq(mqd_t* mq_fd, char* mq_name, int flag);

/* returns zero in case of success */
S32 iap2SendMqRecvAck(mqd_t sndMqFd, S32 rcvMqFd,
                      iap2ComThreadCmds_t mq_cmd, void* param, size_t size);
/* returns zero in case of success */
S32 iap2SendMq(mqd_t mqFD, char* sendBuf, size_t sendBufSize);
/* returns in case of success number of bytes received */
S32 iap2RecvMq(mqd_t mqFD, char* recvBuf, ssize_t recvBufSize);
/* returns in case of success number of bytes received */
S32 iap2RecvMqTmo(mqd_t mqFD, char* recvBuf, U32 recvBufSize, U32 timeout_ms);
void iap2ComThread(void* exinf);
void iap2ServiceComThread(void* exinf);
S32 (*iap2HdlComThreadPollMqEvent) (iAP2Device_t* iap2Device, S32 mqFD, S32 mqFdSendAck, BOOL* b_endComThread);
S32 iap2AllocateandUpdateData(void* dest_ptr, void* src_ptr, U16* dest_count, U16 src_count, iAP2_Type data_type);

/**************************************************************************//**
 * This function is used for freeing the memory.
 *
 * \param input_ptr   the double pointer which needs to be freed
 *
 * \return None
 * \see
 * \note
 ****************************************************************************/
void iap2TestFreePtr(void** input_ptr);

#endif /* IAP2_TEST_UTILITY_H_ */
