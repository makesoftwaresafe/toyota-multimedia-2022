/* -----------------------------------------------------------------------------
 * An invalid character is automatically inserted by cvs when the file is
 * commited. We can not do anything about it.
 * -----------------------------------------------------------------------------
 */

#ifndef IPOD_USB_PACKETIZER_H
#define IPOD_USB_PACKETIZER_H
 
#include <sys/un.h>
#include <sys/unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "iPodDataCom.h"
#include "ipodcommon.h"
#include "iap_types.h"
#include "iap_devconf.h"
#include "iap_transport_os.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================== */
/* defines used by the serial reader task                                     */
/* ========================================================================== */

#define AUDIO_TSK                       0x01
#define IPOD_SEM                        0x02

#define IPOD_ARTWORK_DATA_OFFSET_1      19
#define IPOD_ARTWORK_DATA_OFFSET_2      2
#define IPOD_DEINIT_TIMES 20

#define IPOD_LINGO_GENERAL_MIN_SUPPORT 5
#define IPOD_LINGO_SIMPLE_MIN_SUPPORT 2
#define IPOD_LINGO_DISPLAY_MIN_SUPPORT 0
#define IPOD_LINGO_EXTENDED_MIN_SUPPORT 10
#define IPOD_LINGO_DIGITAL_MIN_SUPPORT 1
#define IPOD_LINGO_STORAGE_MIN_SUPPORT 1
#define IPOD_LINGO_OUT_MIN_SUPPORT 0
#define IPOD_LINGO_LOCATION_MIN_SUPPORT 0
#define IPOD_LINGO_SUPPORT_MAJ_VERSION1 1

/* the different lingo ids ---------------- */
#define IPOD_GENERAL_LINGO              0x00
#define IPOD_SIMPLE_LINGO               0x02
#define IPOD_DISPLAY_REMOTE_LINGO       0x03
#define IPOD_EXTENDED_LINGO             0x04
#define IPOD_AUDIO_LINGO                0x0A
#define IPOD_STORAGE_LINGO              0x0C
#define IPOD_IPODOUT_LINGO              0x0D
#define IPOD_LOCATION_LINGO             0x0E
#define IPOD_AUDIO_ACC_SAMPLE_RATE_CMD  0x03
#define IPOD_AUDIO_ACC_SAMPLE_RATE_LEN  0x0E
#define IPOD_NUMBER_AUDIOCHANNELS       2
#define IPOD_NUMBER_WRITE_BUFFERS       2
#define IPOD_LINEINFOBUF_SIZE           256
#define IPOD_SETUP_AUDIOROUTER          -104

#define IPOD_ACK_WAIT_FOR_ACK           0x06

#define IPOD_CMD_HEADER_LEN             1 /* size of checksum byte */

#define IPOD_CMD_SEND_CERT_HEAD_LEN     6
#define IPOD_AUTH_MAJ_PROT_VER          0x02
#define IPOD_AUTH_MIN_PROT_VER          0x00

/* Storage lingo command ids--------------- */
#define IPOD_CMD_STORAGE_ACK            0x00
/* ========================================================================== */
/* common defines                                                             */
/* ========================================================================== */
#define IPOD_HEADER_SIZE5               5
#define IPOD_HEADER_SIZE6               6
#define IPOD_HEADER_SIZE7               7
#define IPOD_HEADER_SIZE8               8
#define IPOD_HEADER_SIZE10              10

#define IPOD_BUF_START_ADR              0

#define IPOD_SYNC_BYTE                  0xFF

#define LENGTH_LINGO_COMMAND_BYTES_3    3
#define LENGTH_LINGO_COMMAND_BYTES_2    2
#define CHECKSUM_BASE                   0x100

#define IPOD_START_OF_MSG_POS           2

#define IPOD_DEFAULT_CMD_ID             0xffff
#define IPOD_DEFAULT_LINGO              0xff
#define IPOD_DEFAULT_ERROR              0xFF

#define IPOD_USB_SEND_BUFFER            {0}

#define IPOD_USB_REPORT_LEN             255
#define IPOD_USB_MAX_TRY_COUNT          3

#define IPOD_USB_REPORT_MSG_LEN         200

#define IPOD_ID_DEFAULT                 0xee
#define IPOD_ACKID_DEFAULT              0xef
#define IPOD_LINGO_DEFAULT              0xee
#define IPOD_START_OF_PACKET            0x55

#define IPOD_DELAY_10MS                 10
#define IPOD_DELAY_20MS                 20
#define IPOD_DELAY_30MS                 30
#define IPOD_DELAY_50MS                 50
#define IPOD_DELAY_100MS                100
#define IPOD_DELAY_200MS                200
#define IPOD_DELAY_500MS                500
#define IPOD_DELAY_1000MS               1000

#define IPOD_TIME_OUT                   20000

#define IPOD_REPORT_ID_TIME_OUT         1000
#define IPOD_MODESWITCH_TIME_OUT        20000

#define IPOD_SEM_RETRY_COUNT            3

#define USB_AUDIO_SHUTDOWN_DELAY        100

#define USB_PLUG_EVENT                  0xffff

#define IPOD_HIGH_MASK                  0xFF00
#define IPOD_LOW_MASK                   0x00FF
#define IPOD_U32_LOW_MASK               0x000000FF
#define IPOD_U32_LOW_0_MASK             0xFFFFFF00
#define IPOD_CHKSUM_MASK                0x000000FF
#define IPOD_CHKSUM_BASE                0x100

#define IPOD_CHKSUM_OFFSET              3


#define IPOD_STOP_AUDIOTASK             0x00000001  /* set bit 1 */
#define IPOD_STOP_WORKERTASK            0x00000080  /* set bit 8 */

#define IPOD_STOP_READERTASK            0x00000008  /* set bit 4 */
#define IPOD_STOP_AUTHENTICATE          0x00000010U /* set bit 5 */
#define IPOD_STOP_STREAM_AUDIOTASK      0x00000020  /* set bit 6 */ //@

#define IPOD_READ_HANDSHAKE_READ        0x00000001  /* set bit 1 */
#define IPOD_READ_HANDSHAKE_CB          0x00000002  /* set bit 2 */
#define IPOD_READ_HANDSHAKE_PICKUP      0x00000004  /* set bit 3 */
#define IPOD_WORKER_TASK_START          0x00000001  /* set bit 1 */
#define IPOD_WORKER_IDENTIFY            0x00000002  /* set bit 2 */
#define IPOD_WORKER_SIGNATURE           0x00000004  /* set bit 3 */
#define IPOD_WORKER_PREFERENCE          0x00000008  /* set bit 4 */
#define IPOD_WORKER_CERTIFICATE         0x00000010  /* set bit 5 */
#define IPOD_WORKER_SECOND_IDENTIFY     0x00000020  /* set bit 6 */
#define IPOD_WORKER_REIDENTIFY          0x00000040  /* set bit 7 */
#define IPOD_WORKER_REQUESTIDENTIFY     0x00000100  /* set bit 9 */
#define IPOD_WORKER_WAIT_FLAG           0xFFFFFFFF
#define IPOD_USB_LINK_CTRL_BYTE_SIZE 1
#define IPOD_USB_HEADER_SIZE 3

#define IPOD_LINK_CTRL_ONLY_ONE         0x00
#define IPOD_LINK_CTRL_FRST_FOLLOW      0x02
#define IPOD_LINK_CTRL_MIDDLE           0x03
#define IPOD_LINK_CTRL_LAST             0x01

#define IPOD_DEVICEN_NAME_LEN_MAX 256
/* Extended lingo short message: 0x55, length byte, lingo byte, two byte command ID = 5 */
#define IPOD_TRANSLEN_WITH_EXTEND_ACK_SHORT 5
/* Other lingo short message: 0x55, length byte, lingo byte, one byte command ID = 4 */
#define IPOD_TRANSLEN_WITH_ACK_SHORT 4
/* Extended lingo long  message: 0x55, three length bytes, lingo byte, two byte command ID = 7 
   (-1 for other Lingo) */
#define IPOD_TRANSLEN_WITH_ACK_LONG  7
/* Other lingo short message: 0x55, length byte, lingo byte, one byte command ID = 4 */
#define IPOD_TRANSLEN_WITH_NO_ACK_SHORT 4
/* Other lingo long message: 0x55, length byte, three lingo bytes, one byte command ID = 6 
   (does not apply for Extended lingo, as Extended lingo always includes reply) */
#define IPOD_TRANSLEN_WITH_NO_ACK_LONG 6

#define IPOD_READ_RETRY_MAX 10
#define IPOD_READ_SIZE_1    1
#define IPOD_READ_SIZE_2    2

typedef struct _IPOD_INSTANCE IPOD_INSTANCE;

typedef enum _EVT_FLG
{
    IPOD_ALL_EVTS = 0,
    IPOD_WORK_EVT,
    IPOD_READ_EVT
} IPOD_EVT_FLG;

typedef enum
{
    IPOD_RCV_ERROR = 0xFF,
    IPOD_ATTACH = 0x00,
    IPOD_DETACH,
    IPOD_MESSAGE,
    IPOD_MESSAGE_DEFECT
} IPOD_TELEGRAM_TYPE;

typedef struct _IPOD_RCVMSGINFO
{
    U16 sendCmdId;
    U8  expectedLingo;
    U16 expectedCmdId;
    U16 expectedTransID;
    S32 waitingCmdCnt;
    U16 iPodTransID;
    U16 accTransID;
    U8 startIDPS;
    U32 flowControlWait;
    U8 supportCancelCmd;
} IPOD_RCVMSGINFO;

typedef struct _IPOD_SENDMSGINFO
{
    U32 msgLenTotal;
    U16 msgLen;
    U8 checksum;
    U8 waitFlg;
    U16 transID;
} IPOD_SENDMSGINFO;

typedef struct _IPOD_PICKUPMSGINFO
{
    U8*  pickup_message;
    U16  pickup_len;
    U16  cmdId;
    U8   lingo;
    U8   error;
    U16  transID;
} IPOD_PICKUPMSGINFO;

typedef struct CertificateInfoType_
{
    U16 cert_len;
    U8  *cert_data;
} CertificateInfoType;

typedef struct MessageHeaderInfoType_
{
    U8  *msgBuffer;
    U16 telegramLen;
    U16 telegramCmdId;
    U8  telegramLingo;
    U8  telegramErrorCode;
    U16 ackCmdId;
    U8* iPodResponseBuffer;
    U16 transID;
    IPOD_TELEGRAM_TYPE telegramType;
} MessageHeaderInfoType;

typedef struct _IPOD_OPTIONS
{
    U64         iPodOptions;
    S32         rc;
    VERSION     supported;
    VERSION     version;
} IPOD_OPTIONS;

typedef struct _IPOD_TRANSPORT
{
    S32 devInfo;

    IPOD_OPTIONS options[IPOD_LINGO_MAX];
    MessageHeaderInfoType MessageHeaderInfo;
    U16 transportMaxPayloadSize;

    IPOD_INSTANCE* iPodHndl;
    IPOD_DATACOM_FUNC_TABLE data_com_functions;
} IPOD_TRANSPORT;

typedef struct _IPOD_ARTWORK
{
    IPOD_ARTWORK_DATA data;
    U16  transID;
    U8   saveAsBMP;
    U8*  imagePath;
} IPOD_ARTWORK;

struct _IPOD_INSTANCE
{
    U8   name[IPOD_DEVICEN_NAME_LEN_MAX];                      /* device name */
    IPOD_CONNECTION_TYPE connection;    /* what is the connection medium, e.g. USB */
    S32  id;                            /* context id */

    U8   isConnected;                   /* TRUE: connected */
    U8   detected;                      /* TRUE: detected  */
    U8   isAPIReady;                    /* TRUE: API is ready for use */

    IPOD_RCVMSGINFO rcvMsgInfo;         /* received iAP message */
    IPOD_PICKUPMSGINFO pickupInfo;      /* copy of iAP message for hand shake */
    IPOD_TRANSPORT transport;           /* transport medium [USB or BT] */
    IPOD_SWITCH_AUDIO_OUT audioSwitch;  /* digital or analog audio switch */
    IPOD_ARTWORK artwork;               /* artwork specific callback data */

    IPOD_TASK_ID  readerTaskId;                  /* reader task id */
    IPOD_FLG_ID flgReaderHandShakeID;   /* event flag to handle synchronization of reader task (hand shake) */
    IPOD_SEM_ID semModeSwitchId;        /* semaphore to handle simple and extended interface mode */
    U8 *packet;
    U16 iAP1MaxPayloadSize;
    U8 *iAP1Buf;
    U8 *iPodRetAccInfoData;
    U8 iPodRetAccInfoFlg;
};

typedef struct _IPOD_IOS_INSTANCE
{
    U8 name[IPOD_DEVICEN_NAME_LEN_MAX];
    U8 numApps;
    IPOD_IOS_APP *appInfo;
} IPOD_IOS_INSTANCE;



/* ========================================================================== */
/* function prototypes                                                        */
/* ========================================================================== */

S32     iPodTransportInitConnection(U8* devicename, IPOD_CONNECTION_TYPE connection);
void    iPodTransportDisconnect(U32 iPodID);

S32 iPodCheckConnected(U8* devicename);

void    iPodSetExpectedCmdId(IPOD_INSTANCE* iPodHndl, U16 cmdId, U8 lingo);
void    iPodSetExpectedCmdIdEx(IPOD_INSTANCE* iPodHndl, U16 cmdId, U8 lingo, U32 cnt);
S32     iPodIncreaseExpectedCmdIdEx(IPOD_INSTANCE* iPodHndl, U16 cmdId, U8  lingo, U32 cnt);

void    iPodUSBReceiveResponse(IPOD_INSTANCE* iPodHndl, U8* buf);
S32     iPodWaitAndGetResponseLength(IPOD_INSTANCE* iPodHndl);
void    iPodGetResponseData(IPOD_INSTANCE* iPodHndl, U8* data);


S32     iPodWaitAndGetValuesOfLenCmdInfo(IPOD_INSTANCE* iPodHndl, 
                                         U16 *telegramLen,
                                         U16 *telegramCmdId,
                                         U8  *telegramLingo);

void    iPodGetValuesOfLenCmdInfo(IPOD_INSTANCE* iPodHndl, 
                                  U16 *telegramLen,
                                  U16 *telegramCmdId,
                                  U8  *telegramLingo);

S32     iPodWaitAndGetResponseFixedSize(IPOD_INSTANCE* iPodHndl, U8 *data);
S32     iPodCreateReaderTask(IPOD_INSTANCE* iPod);


S32     iPodWaitForModeSwitchSemaphore(IPOD_INSTANCE* iPodHndl);
S32     iPodSigModeSwitchSemaphore(IPOD_INSTANCE* iPodHndl);
S32     iPodWaitForModeSwitch(IPOD_INSTANCE* iPodHndl);

S32     iPodWaitForSenderSemaphore(void);
S32     iPodSigSenderSemaphore(void);


void iPodSetAccInfo(MessageHeaderInfoType *info, IPOD_INSTANCE* iPodHndl);

/* IDPS commands */
void iPodSetStartIDPS(IPOD_INSTANCE* iPodHndl, U8 idps);
U8 iPodGetStartIDPS(void);
void iPodSetiPodTransID(U16 transID);
U16 iPodGetiPodTransID(void);
void iPodSetAccTransID(U16 transID);
U16 iPodGetAccTransID(void);

IPOD_INSTANCE* iPodGetHandle(U32 iPodID);

/////////////////////////////////////

S32 iPodSendLongTelegram(IPOD_INSTANCE* iPodHndl, U8 *buf);
S32 iPodSendLongTelegramNoWaitForACK(IPOD_INSTANCE* iPodHndl, U8 *buf);
S32 iPodSendCommand(IPOD_INSTANCE* iPodHndl, U8* buf);
S32 iPodSendCommandNoWaitForACK(IPOD_INSTANCE* iPodHndl, U8 *buf);
S32 iPodSendCommandCertificate(IPOD_INSTANCE* iPodHndl, U8 *buf);
S32 iPodSendSignature(IPOD_INSTANCE* iPodHndl, U8 *buf, U16 transID);

S32 iPodSendMessage(VP iPod, U8 *buf, VP send);

S32 iPodTrnspInitPlugins(void);
S32 iPodTrnspDeinitPlugins(void);

S32 iPodTrnspOpenPlugin(IPOD_INSTANCE* iPod, IPOD_CONNECTION_TYPE connection);
S32 iPodTrnspClosePlugin(IPOD_INSTANCE* iPodHndl);

S32 struct_init(void);

//////////////////////////////////////

#ifdef __cplusplus
}
#endif

#endif

