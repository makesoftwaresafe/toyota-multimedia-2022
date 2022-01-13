/*
 * \file ipodcommon.h
 *
 * \version:
 *
 * \component:
 *
 * \author:
 *
 */

#ifndef AIPODW_COMMON_H
#define AIPODW_COMMON_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef IPOD_FAKE_AUTHENTICATE
#   define IPOD_DELAY_AUTHENTICATION    75000
#   define IPOD_DELAY_5SECONDS          5000
#endif

#define IPOD_TIMEOUT_80MS 80
#define IPOD_TIMEOUT_20MS 20
#define IPOD_RESPONSE_BUF_SIZE 256 /* must be between 128 and 65529 */
#define IPOD_START_LENGTH 2
#define IPOD_SEND_LONG_MARKER 2
#define IPOD_START_PACKET 0x55
#define IPOD_8BITS  8
#define IPOD_SHORT_WRITE_SIZE 256
#define IPOD_LONG_WRITE_SIZE 65536
#define IPOD_DC_NAME_MAX_SIZE 7

#define IPOD_MIN_NOTIFICATION_MASK 0x0000000000000004; /* at least try to activate Flow Control notifications */

    /* If the Apple device does not reply to the get command, */
    /* use this value as maximum payload size for sending commands to the Apple device */
#define IPOD_TRANSPORT_DEFAULT_MAX_PAYLOAD_SIZE 506

    /* wait for 10 ms to check whether ReaderTask took read handshake flag */
#define IPOD_WAIT_FAILED_READ_TIME 10
    /* wait for 1000 ms to check whether ReaderTask finally has result ready */
#define IPOD_WAIT_READ_RETRY_TIME 1000
#define IPOD_MAX_TRY_COUNT 3

#define IPOD_POS0   0
#define IPOD_POS1   1
#define IPOD_POS2   2
#define IPOD_POS3   3
#define IPOD_POS4   4
#define IPOD_POS5   5
#define IPOD_POS6   6
#define IPOD_POS7   7
#define IPOD_POS8   8
#define IPOD_POS9   9
#define IPOD_POS10 10
#define IPOD_POS11 11
#define IPOD_POS12 12
#define IPOD_POS13 13
#define IPOD_POS14 14
#define IPOD_POS15 15
#define IPOD_POS16 16
#define IPOD_POS17 17
#define IPOD_POS18 18
#define IPOD_POS19 19
#define IPOD_POS20 20
#define IPOD_POS21 21

#define IPOD_NO_WAIT_ACK 0
#define IPOD_WAIT_ACK_SHORT 1
#define IPOD_WAIT_ACK_LONG 2
#define IPOD_NO_WAIT_ACK_LONG 3
#define IPOD_WAIT_ACK_REPLY 4

#define IPOD_MASK_FF000000 0xFF000000
#define IPOD_MASK_00FF0000 0x00FF0000
#define IPOD_MASK_0000FF00 0x0000FF00
#define IPOD_MASK_000000FF 0x000000FF

#define IPOD_BITS_56_63    0xFF00000000000000ULL
#define IPOD_BITS_48_55    0x00FF000000000000ULL
#define IPOD_BITS_40_47    0x0000FF0000000000ULL
#define IPOD_BITS_32_39    0x000000FF00000000ULL
#define IPOD_BITS_24_31    0xFF000000
#define IPOD_BITS_16_23    0x00FF0000
#define IPOD_BITS_8_15     0x0000FF00
#define IPOD_BITS_0_7      0x000000FF

#define IPOD_SHIFT_1    1
#define IPOD_SHIFT_2    2
#define IPOD_SHIFT_3    3
#define IPOD_SHIFT_4    4
#define IPOD_SHIFT_8    8
#define IPOD_SHIFT_16  16
#define IPOD_SHIFT_24  24
#define IPOD_SHIFT_32  32
#define IPOD_SHIFT_40  40
#define IPOD_SHIFT_48  48
#define IPOD_SHIFT_56  56
#define IPOD_SHIFT_64  64

#define IPOD_CHECKSUM_BYTE_LEN 1
/* IDPS commands */
#define IPOD_TRANSID_LENGTH 2
#define IPOD_PREV_ACK_LENGTH 4
#define IPOD_NEW_ACK_LENGTH 6

#define IPOD_TMO_FEVR -1
/**
 * \addtogroup IPOD_TRACK_CAPABILITY
 */
/*\{*/

#define IPOD_TRACK_CAP_IS_AUDIOBOOK                 0x01    /*!< Track is audiobook */
#define IPOD_TRACK_CAP_HAS_CHAPTERS                 0x02    /*!< Track has chapters */
#define IPOD_TRACK_CAP_HAS_ALBUM_ARTWORK            0x04    /*!< Track has album artwork */
#define IPOD_TRACK_CAP_HAS_SONG_LYRICS              0x08    /*!< Track has song lyrics */
#define IPOD_TRACK_CAP_IS_PODCAST_EPISODE           0x10    /*!< Track is a podcast episode */
#define IPOD_TRACK_CAP_HAS_RELEASE_DATE             0x20    /*!< Track has release date */
#define IPOD_TRACK_CAP_HAS_DESCRIPTION              0x40    /*!< Track has description */
#define IPOD_TRACK_CAP_CONTAINS_VIDEO               0x80    /*!< Track contains video */
#define IPOD_TRACK_CAP_IS_QUEUED_TO_PLAY_AS_VIDEO   0x100    /*!< Track is currently queued to play as video */
/*\}*/

/******************************************************************
*                       error codes                               *
*******************************************************************/

/*! Completed successfully */
#define IPOD_OK                                         0
/*! Command failed */
#define IPOD_ERROR                                      -1

/*! Apple Authentication failed because of wrong cert length */
#define IPOD_CERT_LENGTH_INCORRECT                      -98
/*! communication with Apple Authentication CP failed */
#define IPOD_ERR_APPLE_CP                               -97
/*! authentication of iPod failed */
#define IPOD_ERR_AUTHENTICATION                         -96
/*! the Operation can't executed in the selected mode! */
#define IPOD_ERR_WRONG_MODE                             -95
/*! The Operation is only in the advanced mode available! */
#define IPOD_ERR_ONLY_IN_ADVANCEDMODE                   -94
/*! Getting response message failed! */
#define IPOD_ERR_GET_RESPONSE_MSG_FAILED                -93
/*! All tasks were stopped */
#define IPOD_TASK_STOPPED                               -92
/*! The Ipod is already connected */
#define IPOD_ALREADY_CONNECTED                          -91
/*! The Ipod is not connected */
#define IPOD_NOT_CONNECTED                              -90


/** 
 * \addtogroup AckErrorCode
 */
/*\{*/

/*! Unknown database category */
#define IPOD_UNKNOWN_DATABASE_CATEGORY                  -89
/*! Command failed */
#define IPOD_COMMAND_FAILED                             -88
/*! Out of resources */
#define IPOD_OUT_OF_RESOURCES                           -87
/*! Bad parameter */
#define IPOD_BAD_PARAMETER                              -86
/*! Unkown ID */
#define IPOD_UNKNOWN_ID                                 -85
/*! redefine the Unkown ID*/
/*! unsupported device */
#define IPOD_ERR_UNSUP_DEV                              -84
/*! iPod detected however not yet ready to use (only used if MC application does Audio Streaming) */
#define IPOD_DETECTED                                   1
#define IPOD_UNKOWN_ID                                  IPOD_UNKNOWN_ID
/*! Accessory not authenticated */
#define IPOD_ACCESSORY_NOT_AUTHENTICATED                -83
/*! Bad authentication version */
#define IPOD_BAD_AUTHENTICATION_VERSION                 -82
/*! Accessory power mode request failed */
#define IPOD_ACC_POWER_MODE_FAILED                      -81
/*! Certificate invalid */
#define IPOD_CERTIFICATE_INVALID                        -80
/*! Certificate permissions invalid */
#define IPOD_CERTIFICATE_PERMISSIONS_INVALID            -79
/*! File is in use */
#define IPOD_FILE_IS_IN_USE                             -78
/*! Invalid file handle*/
#define IPOD_INVALID_FILE_HANDLE                        -77
/*! Directory not empty */
#define IPOD_DIR_NOT_EMPTY                              -76
/*! Operation timed out */
#define IPOD_OPERATION_TMOUT                            -75
/*! Command unavailable in this iPod mode */
#define IPOD_UNAVAILABLE_MODE                           -74
/*! Invalid accessory resistor ID value */
#define IPOD_INVALID_ACC_RESID_VALUE                    -73
/*! Maximum number of accessory connections already reached */
#define IPOD_MAX_CONNECT                                -69
/*! Session Writing is failed */
#define IPOD_SESSION_WRITE_FAILURE                      -68
/*\}*/

/*! Apple Authentication CP busy */
#define IPOD_ERR_APPLE_CP_BUSY                          -72
/*! Command is not supported in this version */
#define IPOD_ERR_COMMANDS_NOT_SUPPORTED                 -71
/*! An unexpected error */
#define IPOD_ERR_UNEXPECTED_ERROR                       -70
/*! Open mode is invalid (write not permitted) */
#define IPOD_ERR_RONLY                                  -67
/*! Processing aborted */
#define IPOD_ERR_ABORT                                  -66
/*! Data is lost */
#define IPOD_ERR_DATA_LOST                              -65
/*! Data send failed */
#define IPOD_ERR_WRITE_FAIL                             -64
/*! Wait released by wait disabled state */
#define IPOD_ERR_DISWAI                                 -52
/*! The object being waited for was deleted
   (the specified semaphore was deleted while waiting)*/
#define IPOD_ERR_DLT                                    -51
/*! Busy processing other requests */
#define IPOD_ERR_TMOUT                                  -50
/*! Wait state released (tk rel wai received in wait state) */
#define IPOD_ERR_RLWAI                                  -49
/*! Object does not exist */
#define IPOD_ERR_NOEXS                                  -42
/*! Incorrect object state */
#define IPOD_ERR_OBJ                                    -41
/*! Read-only device */
#define IPOD_ERR_LIMIT                                  -34
/*! Insufficient memory */
#define IPOD_ERR_NOMEM                                  -33
/*! Exceeds system limits */
#define IPOD_ERR_OACV                                   -27
/*! Context error (issued from task-independent portion
   or in dispatch disabled state) */
#define IPOD_ERR_CTX                                    -25
/*! dd is invalid or not open */
#define IPOD_ERR_ID                                     -18
/*! Parameter error (tmout . (i2), cnt . 0) */
#define IPOD_ERR_PAR                                    -17

/* ----------------------end of error codes----------------------- */


#ifdef __cplusplus
}
#endif

#endif

