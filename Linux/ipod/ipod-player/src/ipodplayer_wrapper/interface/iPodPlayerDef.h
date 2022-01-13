/*! \file iPodPlayerDef.h
 *
 * \version: 1.0
 *
 * \author: mshibata
 */
#ifndef IPOD_PLAYER_DEF_H
#define IPOD_PLAYER_DEF_H
#include "adit_typedef.h"

/*! 
 * \mainpage iPodPlayer API Specification Main page
 * \version \b A:   1/Dec/2011<br>
 * \li Initial version.<br>
 * \version \b B01: 22/Feb/2012<br>
 * \li Re-designed the all API.<br>
 * \li Added more description for iOS App Communication and Database Browsing.<br>
 * \li Added the GPS operation API.<br>
 * \li Removed unnecessary data and API.<br>
 * \li Added the detail of error code.<br>
 * \version \b B02: 29/Feb/2012<br>
 * \li Added the descritpion how does API behave if iPod does not support command.<br>
 * \li Fixed some link error and missing API description.<br>
 * \li Added the structure of #IPOD_PLAYER_TAG_INFO.<br>
 * \li Added the more detail for HMI control and Intelligent PL.<br>
 * \li Changed the parameter of #iPodPlayerInit.<br>
 * \li Added the new API of #iPodPlayerSetiOSAppsInfo and #iPodPlayerStartAuthentication.<br>
 * \version \b B03: 29/Mar/2012<br>
 * \li Change the parameter of #IPOD_PLAYER_CB_NOTIFY_CONNECTION_STATUS.(Wrong parameter is used).<br>
 * \li Change the member of structure of #IPOD_PLAYER_TAG_INFO. All members changed to strings. and Maxmum length changed.<br>
 * \version \b B04 05/Apr/2012<br>
 * \li Added the member of #IPOD_PLAYER_VERSION. Member of revision is added.<br>
 * \li Changing the name of stucture member. mask is changed the name for target structure.<br> 
 * also descritpion of this mask added for more datail.<br>
 * Changed structure is #IPOD_PLAYER_VIDEO_SETTING, #IPOD_PLAYER_TRACK_INFO, #IPOD_PLAYER_CHAPTER_INFO, 
 * #IPOD_PLAYER_DEVICE_PROPERTY and #IPOD_PLAYER_DEVICE_STATUS<br>
 * \li Change the parametr of #iPodPlayerGetCoverArt and #iPodPlayerGetCoverArtInfo. 
 * These are added the parameter of #IPOD_PLAYER_TRACK_TYPE. 
 * also These are changed the type of trackindex to U64<br>
 * \li Removed the member of structure #IPOD_PLAYER_HMI_ROTATION_DEVICE. Remomved member is IPOD_PLAYER_HMI_ROTATION_DEVICE_JOG.
 * \li Added the #IPOD_PLAYER_CB_HMI_SET_EVENT_NOTIFICATION_RESULT member in #IPOD_PLAYER_REGISTER_CB_TABLE.(Missing function)<br>
 * \li Change the parameter of #iPodPlayerHMIButtonInput because Application can control a few event at the same time.<br>
 * \li Added the description how to get the all entry by #iPodPlayerGetDBEntries<br>
 * \li Change the maximum size of tag string to 256.<br>
 * \li Change the maximum size of some strings. and Prepared the Macro that indicates a maximum sizes for some strings.<br>
 * \li Added the paramter of #iPodPlayerHMIGetSupportedFeature and #IPOD_PLAYER_CB_HMI_GET_SUPPORTED_FEATURE_RESULT. 
 * It is added for indicate the kind of getting feature.<br>
 * \li Added the new API of #iPodPlayerSetDeviceEventNotification, #IPOD_PLAYER_CB_SET_DEVICE_EVENT_NOTIFICATION_RESULT 
 * and #IPOD_PLAYER_CB_NOTIFY_DEVICE_EVENT.
 * \version \b B: 06/Apr/2012<br>
 * \li Release version.<br>
 * \version \b C01: 10/Apr/2012<br>
 * \li Changed the structure name of IPOD_PLAYER_POWER_SUPPLY_STATUS to #IPOD_PLAYER_POWER_STATUS.<br>
 * \li Added the dummy byte to #IPOD_PLAYER_VERSION for padding<br>
 * \li Changed the define name of macro for mask. Added the "MASK_"<br>
 * \li Changed the paramter named "mask" or "bitmask" for more understandable<br>
 * \li Wrong name is fixed. IPOD_PLAYER_MAC_ADD_LEN_MAX changes to IPOD_PLAYER_MAC_ADDR_LEN_MAX.<br>
 * \li Changed the size of IPOD_PLAYER_LAUNCH_IOSAPP_NAME_LEN_MAX to 256 from 255.
 * \li Define name for maxmum size standardise to "LEN_MAX" or "ARRAY_MAX"<br>
 * \version \b C02: 15/Apr/2012<br>
 * \li Added the unknown value in #IPOD_PLAYER_VIDEO_SETTING<br>
 * \li Added the unknown value in #IPOD_PLAYER_PLAYING_SPEED<br>
 * \li Added the more detail description of how to back to previous type in #iPodPlayerSelectDBEntry.<br>
 * \li Added the new error code named #IPOD_PLAYER_ERR_API_CANCELED.<br>
 * \li Added the restriction to #iPodPlayerGetTrackInfo and #IPOD_PLAYER_CB_NOTIFY_TRACK_INFO<br>
 * \version \b C03: 8/May/2012<br>
 * \li Changed the member of #IPOD_PLAYER_IOSAPP_INFO to fixed size.<br>
 * \version \b C04: 11/June/2012<br>
 * \li Added the list to each function group. It shall describe that each function can be used in which mode.<br>
 * \li Added the detail description of #IPOD_PLAYER_CB_NOTIFY_DB_ENTRIES. It shall describe how iPodPlayer notify of list.<br>
 * \li Added the new interface of #iPodPlayerOpenSongTagFile, #iPodPlayerCloseSongTagFile, 
 * #iPodPlayerSetVolume and #iPodPlayerGetVolume.<br>
 * \li Added the parameter of #iPodPlayerSongTag.<br>
 * \li Added the descritpion to #IPOD_PLAYER_CB_NOTIFY_PLAYBACK_STATUS<br>
 * \version \b C05: 27/July/2012<br>
 * \li Added the warnning of #iPodPlayerSetDisplayImage to specify the enable bitmap image.<br>
 * \li Change the parameter of #IPOD_PLAYER_CB_SEND_TO_APP_RESULT. This function is added the handle parameter.<br>
 * \li Added the new error code of #IPOD_PLAYER_ERR_IPOD_CTRL_ERROR<br>
 * \version \b C06: 07/Aug/2012<br>
 * \li Added the new API. The new API is #iPodPlayerSetDeviceDetection. 
 * also result callback is added. It is defined as #IPOD_PLAYER_CB_DEVICE_DETECTION_RESULT<br>
 * \li Added the new member of in #IPOD_PLAYER_CONNECTION_STATUS. Member name is deviceType. I
 * It is used to specify the type of connected device<br>
 * \version \b C07: 12/Sep/2012<br>
 * \li Added the new error code of #IPOD_PLAYER_ERR_IPOD_CTRL_TMOUT<br>
 * \version \b C08: 21/Dec/2012<br>
 * \li Change the interface of #IPOD_PLAYER_CB_SELECT_DB_ENTRY_RESULT. It was added the parameter of totalCount<br>
 * \version \b C09: 14/Feb/2013<br>
 * \li Changed the description of some notification APIs.<br>
 * \version \b C10: 18/Feb/2013<br>
 * \li Added the new interface of #iPodPlayerSetAudioMode and #IPOD_PLAYER_CB_SET_AUDIO_MODE_RESULT<br>
 * \version \b C11: 15/Apr/2013<br>
 * \li Changed the description of database browsing to correct it.
 * \li Added the description of #iPodPlayerSelectAV.
 *\version \b C13: 19/Jan/2016<br>
 * \li Added the description of #iPodPlayerGetDeviceProperty parameter.
 * \li Changed the Interface of #iPodPlayerSetDeviceDetection.
 *\version \b C14: 03/Mar/2016<br>
 * \li Add the new member of in #IPOD_PLAYER_DEVICE_DETECTION_INFO. The member name is accName.
 *\version \b C15: 14/Jun/2016<br>
 * \li Add the new member of in #IPOD_PLAYER_DEVICE_DETECTION_INFO. The member name is serialNum.
 *\version \b C16: 12/Aug/2016<br>
 * \li Add the new member of in #IPOD_PLAYER_CONNECTION_STATUS. The member name is iapType.
 *\version \b C17: 08/Mar/2017<br>
 * \li Add #iPodPlayerPlayCurrentSelection to support playMediaLibraryCurrentSelection.
 * \li Add \ref playbackStatusActiveMask to #IPOD_PLAYER_CB_NOTIFY_PLAYBACK_STATUS to find notified playback status item.
 * \li Modify #IPOD_PLAYER_CB_NOTIFY_PLAYBACK_STATUS to support notification of shuffle/repeat status.
 * \li Modify #iPodPlayerSetPowerSupply to support iAP2 protocol.
 * \li Modify #iPodPlayerGotoTrackPosition to support SetNowPlayingInformation with iAP2 protocol. 
 * \li Modify #IPOD_PLAYER_CB_NOTIFY_DEVICE_EVENT to support media library update. 
 * \li Modify #IPOD_PLAYER_CB_NOTIFY_DEVICE_EVENT to support power update (IPOD_PLAYER_POWER_NOTIFY). 
 * \li Add command line option. 
 *\version \b C18: 23/Mar/2017<br>
 * \li Add #iPodPlayerSetLocationInformation to support location information.
 * \li Add #iPodPlayerSetVehicleStatus to support Vehicle Status.
 *\version \b C19: 24/Apr/2017<br>
 * \li Modify #IPOD_PLAYER_DEVICE_DETECTION_INFO to modify accessory information by iPodPlayer API.
 * \li For that it's added #IPOD_PLAYER_ACC_INFO_CONFIGURATION.
 *\version \b C19a: 11/Jul/2017<br>
 * \li Modify argument of #IPOD_PLAYER_CB_NOTIFY_COVERART_DATA to show that does not have cover art.
 *\version \b C19b: 18/Jul/2017<br>
 * \li Add #iPodPlayerGetMediaItemInformation API to get media type in media item.
 * \li Add trackID and mediaType in #IPOD_PLAYER_TRACK_INFO to decide media type (songs, podcast, audiobook).
 *\version \b C20: 06/Jun/2017<br>
 * \li Modify #IPOD_PLAYER_DEVICE_DETECTION_INFO to support host mode in iAP2.
 * \li For that it's modified #IPOD_PLAYER_ACC_INFO_CONFIGURATION and it's added #IPOD_PLAYER_IOS_APPINFO.
 *\version \b C21: 01/Aug/2017<br>
 * \li Merged C19a and C19b into C20.
 *\version \b C22: 02/Aug/2017<br>
 * \li add sampleRate member to #IPOD_PLAYER_DEVICE_EVENT_INFO structure.
 *\version \b C23: 02/Oct/2017<br>
 * \li add #IPOD_PLAYER_EVENT_MASK_STORE_DB to #iPodPlayerSetDeviceEventNotification.
 * \li add #IPOD_PLAYER_DEVICE_EVENT_TYPE_STORE_DB to #IPOD_PLAYER_CB_NOTIFY_DEVICE_EVENT.
 *\version \b C24: 30/Nov/2017<br>
 * \li add #iPodPlayerRelease API in order to cancel fast foward and rewind.
 *\version \b C25: 05/Dec/2017<br>
 * \li Modify length of macAddr in #IPOD_PLAYER_DEVICE_EVENT_BT from 8Byte(IPOD_PLAYER_MAC_ADDR_LEN_MAX) to 6Byte(#IPOD_PLAYER_BT_MAC_ADDR_LEN_MAX).
 *\version \b C26: 14/Dec/2017<br>
 * \li Modify mediaType member of #IPOD_PLAYER_CB_GET_MEDIA_ITEM_INFO_RESULT from U32 to S32 (typo correction).
 *\version \b C27: 14/Dec/2017<br>
 * \li Add unknown case to mediaType of #IPOD_PLAYER_TRACK_INFO.
 *\version \b C28: 11/Jan/2018<br>
 * \li Add appName and appBundleID member to #IPOD_PLAYER_PLAYBACK_STATUS.
 * \li Add #IPOD_PLAYER_PLAY_ACT_APP_NAME and #IPOD_PLAYER_PLAY_ACT_APP_BUNDLE_ID to \ref playbackStatusActiveMask.
 *\version \b C29: 25/Jan/2018<br>
 * \li Add queueListAvail member to #IPOD_PLAYER_PLAYBACK_STATUS.
 * \li Add #IPOD_PLAYER_PLAY_ACT_QUEUE_LIST_AVAIL to \ref playbackStatusActiveMask.
 *\version \b C30: 23/Feb/2018<br>
 * \li Add vehicleInfo member to #IPOD_PLAYER_DEVICE_DETECTION_INFO.
 *\version \b C31: 03/Apr/2018<br>
 * \li Add #IPOD_PLAYER_EVENT_MASK_UPDATE_PBLIST to #iPodPlayerSetDeviceEventNotification.
 * \li Add #IPOD_PLAYER_DEVICE_EVENT_TYPE_UPDATE_PBLIST to #IPOD_PLAYER_CB_NOTIFY_DEVICE_EVENT.
 *\version \b C32: 16/May/2018<br>
 * \li Add queueCount member to #IPOD_PLAYER_PLAYBACK_STATUS.
 * \li Add #IPOD_PLAYER_PLAY_ACT_QUEUE_COUNT to \ref playbackStatusActiveMask.
 *\version \b C33: 17/May/2018<br>
 * \li Add mediaLibraryUID and AmrStationName member to #IPOD_PLAYER_PLAYBACK_STATUS.
 * \li Add #IPOD_PLAYER_PLAY_ACT_MEDIA_LIBRARY_UID and #IPOD_PLAYER_PLAY_ACT_AMR_STATION_NAME to \ref playbackStatusActiveMask.
 * \li Add #IPOD_PLAYER_EVENT_MASK_DATABASE_AMR to #iPodPlayerSetDeviceEventNotification.
 * \li Add #IPOD_PLAYER_EVENT_MASK_STORE_DB_AMR to #iPodPlayerSetDeviceEventNotification.
 * \li Add #IPOD_PLAYER_DEVICE_EVENT_TYPE_DATABASE_AMR to #IPOD_PLAYER_CB_NOTIFY_DEVICE_EVENT.
 * \li Add #IPOD_PLAYER_DEVICE_EVENT_TYPE_STORE_DB_AMR to #IPOD_PLAYER_CB_NOTIFY_DEVICE_EVENT.
 *\version \b C34: 20/Jun/2018<br>
 * \li Add to the explanation of #iPodPlayerSetDeviceEventNotification that bit clear operation is invalid for iAP2.
 *\version \b C35: 06/Sep/2019<br>
 * \li Add ProductPlanUUID to #IPOD_PLAYER_ACC_INFO_CONFIGURATION . This parameter is assumed to be set when calling iPodPlayerSetDeviceDetection(). 
 */

/**
 * \page more_info Command line option
 * \b Usage: iPodPlayerCore.out [option]... 
 * 
 * \b Option:
 * \li \b -h <br>
 *     Show command line option help.<br>
 * 
 * \li \b -d application name <br>
 *     The specified application is disabled.It can specify following application name.
 *     - iPodPlayerDeviceDetection <br>    
 *     - iPodPlayerAudio <br>   
*/

/**
 * \addtogroup Define iPodPlayer Define
 */
/*\{*/

#include <stdio.h>
/**
 * \name Error Define
 * \anchor ErrorDefine
 */
 /*\{*/


/* ################################### iPod Player Error Code ########################################## */
/*! Function Success */
#define IPOD_PLAYER_OK                                  0

/*! Invalid parameter is used */
#define IPOD_PLAYER_ERR_INVALID_PARAMETER               -1001

/*! iPodPlayer can not have memory */
#define IPOD_PLAYER_ERR_NOMEM                           -1002

/*! Connected application exceeds the max connection */
#define IPOD_PLAYER_ERR_MAX_APP_CONNECT                 -1003

/*! Invalid device ID is used */
#define IPOD_PLAYER_ERR_INVALID_ID                      -1004

/*! Invalid position is used */
#define IPOD_PLAYER_ERR_INVALID_POSITION                -1005

/*! iPodPlayer can not send the command to Apple device in this mode */
#define IPOD_PLAYER_ERR_INVALID_MODE                    -1006

/*! Current status is invalid status*/
#define IPOD_PLAYER_ERR_INVALID_STATUS                  -1007

/*! Error is occurred by iPodctrl or Apple device */
#define IPOD_PLAYER_ERR_IPOD_CTRL_ERROR                 -1008

/*! Error is occurred by iPodctrl or Apple device */
#define IPOD_PLAYER_ERR_IPOD_CTRL_TMOUT                 -1009

/*! Application is already connected */
#define IPOD_PLAYER_ERR_APP_ALREADY_CONNECT             -1010

/*! Invalid value was set in configuration */
#define IPOD_PLAYER_ERR_INVALID_CFG                     -1011

/*! iPodPlayer is not connected with Apple device */
#define IPOD_PLAYER_ERR_NOT_CONNECT                     -1020

/*! Function or part of parameter is not supported */
#define IPOD_PLAYER_ERR_NOT_SUPPORT                     -1021

/*! Request queue is filled */
#define IPOD_PLAYER_ERR_QUEUE_FULL                      -1022

/*! This feature is not applicable */
#define IPOD_PLAYER_ERR_NOT_APPLICABLE                  -1023

/*! iPodPlayer shut down */
#define IPOD_PLAYER_ERR_SHUTDOWN                        -1101

/*! Track does not have chapter */
#define IPOD_PLAYER_ERR_NO_CHAPTER                      -2001

/*! API is canceled */
#define IPOD_PLAYER_ERR_API_CANCELED                    -2002

/*! Internal error. Receving data is still remaining */
#define IPOD_PLAYER_ERR_MORE_PACKET                     -3001

/*! Internal error. Receive is finished by time out */
#define IPOD_PLAYER_ERR_TMOUT                           -3002

/*! Internal error. Timer has error */
#define IPOD_PLAYER_ERR_TIMER                           -3003

/*! Internal error. Timer has error */
#define IPOD_PLAYER_ERR_AUDIO                           -3004

#define IPOD_PLAYER_ERR_MORE_MEM                        -3005

#define IPOD_PLAYER_ERR_DEVICE_ALREADY_CONNECTED        -3006

#define IPOD_PLAYER_ERR_REQUEST_CONTINUE                -3007

#define IPOD_PLAYER_ERR_NO_REPLY                        -3008

#define IPOD_PLAYER_ERR_BACK_TO_WAIT                    -3009

#define IPOD_PLAYER_ERR_STILL_WAIT                      -3010

#define IPOD_PLAYER_ERR_DISCONNECTED                    -3011

#define IPOD_PLAYER_ERR_NO_IAP2_REQUEST                 -3012

#define IPOD_PLAYER_ERR_IAP1_DETECTED                   -3013

#define IPOD_PLAYER_ERR_NO_INTERNAL_REQUEST             -3014

#define IPOD_PLAYER_ERR_NO_IAP2SARVICECALLBACK_REQUEST  -3015

/* -31XX iAP2 library error */
#define IPOD_PLAYER_ERR_INVALID_MESSAGE                 -3100

#define IPOD_PLAYER_ERR_INVALID_MSG_ID                  -3101

#define IPOD_PLAYER_ERR_EAP_SESSION_ID                  -3102

#define IPOD_PLAYER_ERR_EAP_INVALID_ID                  -3103

#define IPOD_PLAYER_ERR_IPOD_ROLE_SW_UNSUP              -3104

#define IPOD_PLAYER_ERR_IPOD_ROLE_SW_FAILED             -3105

#define IPOD_PLAYER_ERR_FILE_XFER_SETUP_NOT_RECVD       -3106

#define IPOD_PLAYER_ERR_FILE_XFER_SETUP_ALREADY_RECVD   -3107

#define IPOD_PLAYER_ERR_FILE_INVALID_ID                 -3108

#define IPOD_PLAYER_ERR_FILE_XFER_MAX_XFER_REACHED      -3109

#define IPOD_PLAYER_ERR_FILE_XFER_ID_NOT_PRESENT        -3110

/* -400X is database error */
#define IPOD_PLAYER_ERR_DB_OPEN                         -4001

#define IPOD_PLAYER_ERR_DB_BACKUP                       -4002

#define IPOD_PLAYER_ERR_DB_RETRIVE_NOT_COMPLETE         -4003

#define IPOD_PLAYER_ERR_DB_NOT_UPDATE                   -4004

//#define IPOD_PLAYER_ERR_MORE_PACKET                     -3006

/*! Not indicated error */
#define IPOD_PLAYER_ERROR                               -1


/* ##################################################################################################### */

/*\}*/


/*! This is restore setting. If this define is set, current setting value of video remain. */
#define IPOD_PLAYER_RESTORE_OFF                         0

/*! This is restore setting. If this define is set, original setting of Apple device is reverted after Apple device is disconnected. */
#define IPOD_PLAYER_RESTORE_ON                          1

/*! Current Apple device's playing mode is audio */
#define IPOD_PLAYER_AUDIO_MODE                          0
/*! Current Apple device's playing mode is video */
#define IPOD_PLAYER_VIDEO_MODE                          1


/*! Application name maximum size */
#define IPOD_PLAYER_LAUNCH_IOSAPP_NAME_LEN_MAX  256
/*! Max size*/
#define IPOD_PLAYER_DATA_SIZE_MAX               (U32)4192 /* 200kbyte */
/*! String maximum size */
#define IPOD_PLAYER_STRING_LEN_MAX              256

/*! Entry name string maximum size */
#define IPOD_PLAYER_ENTRY_NAME_LEN_MAX          256
/*! Track name string maximum size */
#define IPOD_PLAYER_TRACK_NAME_LEN_MAX          256
/*! Album name string maximum size */
#define IPOD_PLAYER_ALBUM_NAME_LEN_MAX          256
/*! Artist name string maximum size */
#define IPOD_PLAYER_ARTIST_NAME_LEN_MAX         256
/*! Podcast name string maximum size */
#define IPOD_PLAYER_PODCAST_NAME_LEN_MAX        256
/*! Genre name string maximum size */
#define IPOD_PLAYER_GENRE_LEN_MAX               256
/*! Composer name string maximum size */
#define IPOD_PLAYER_COMPOSER_LEN_MAX            256
/*! Chapter name string maximum size */
#define IPOD_PLAYER_CHAPTER_NAME_LEN_MAX        256
/*! Apple device name string maximum size */
#define IPOD_PLAYER_DEVICE_NAME_LEN_MAX         256
/*! iOS Application name string maximum size */
#define IPOD_PLAYER_APP_NAME_LEN_MAX            256
/*! Full iOS Application name string maximum size */
#define IPOD_PLAYER_IOS_FULL_NAME_LEN_MAX       256
/*! Lyric string maximum size */
#define IPOD_PLAYER_LYRIC_LEN_MAX               256
/*! Description string maximum size */
#define IPOD_PLAYER_DESCRIPTION_LEN_MAX         256

/*! iOS Application name string maximum size */
#define IPOD_PLAYER_IOSAPP_NAME_LEN_MAX         256

/*! iOS Application URL string maximum size */
#define IPOD_PLAYER_IOSAPP_URL_LEN_MAX          256

/*! Media library unique identifier string maximum size */
#define IPOD_PLAYER_STRING_UID_LEN_MAX          256

/*! Option data of song tag open maximum size */
#define IPOD_PLAYER_OPEN_TAG_OPTION_LEN_MAX     128

/*! Serial number maximum size */
#define IPOD_PLAYER_SERIAL_LEN_MAX              64
/*! Format array maximum size */
#define IPOD_PLAYER_FORMAT_ARRAY_MAX            32

/*! Coverart count array max size */
#define IPOD_PLAYER_COVERART_ARRAY_MAX          64

/*! Coverart time array max size */
#define IPOD_PLAYER_COVERART_TIME_ARRAY_MAX     64

/*! Entry list array max size */
#define IPOD_PLAYER_ENTRIES_ARRYA_MAX           10

/*! Playback Index is unknown number */
#define IPOD_PLAYER_PLYABACK_INDEX_UNKNOWN      0xFFFFFFFF

/*! Playback time is unknown time */
#define IPOD_PLAYER_PLAYBACK_TIME_UNKNOWN       0xFFFFFFFF

/*! MFI MSGCODE for Identication infomation  */
#define IPOD_PLAYER_IDEN_MSGCODE_MAX            100

/*! timeout for IPC long send */
#define IPOD_PLAYER_LONG_SEND_TMOUT             8000

/*! Bluetooth mac address max */
#define IPOD_PLAYER_BT_MAC_COUNT_MAX            10

/*! number of iOS APP max */
#define IPODCORE_MAX_IOSAPPS_INFO_NUM           128

/*!
 * \enum IPOD_PLAYER_SOUND_MODE
 * This is an enumeration of sound mode.<br>
 * This enumeration is used in #iPodPlayerSetAudioMode.<br>
 */
typedef enum
{
    /*! Sound is off. Audio is not output.<br>*/
    IPOD_PLAYER_SOUND_MODE_OFF = 0,
    
    /*! Sound is on. Audio is output.<br>*/
    IPOD_PLAYER_SOUND_MODE_ON
} IPOD_PLAYER_SOUND_MODE;

/*!
 * \enum IPOD_PLAYER_LOCATION_INFO_STATUS
 * This is event type of Location Information that notify from apple device.<br>
 * This status type is used in #IPOD_PLAYER_CB_NOTIFY_LOCATION_INFO_STATUS.<br>
 */
typedef enum
{
    /*! start location information session.<br>*/
    IPOD_PLAYER_LOCATION_INFO_START = 0,
    
    /*! stop location information session.<br>*/
    IPOD_PLAYER_LOCATION_INFO_STOP,
    
    /*! location information status is unknown.<br>*/
    IPOD_PLAYER_LOCATION_INFO_UNKNOWN = 0xff
    
} IPOD_PLAYER_LOCATION_INFO_STATUS;

/*!
 * \enum IPOD_PLAYER_VEHICLE_STATUS
 * This is event type of vehicle status that notify from apple device.<br>
 * This status type is used in #IPOD_PLAYER_CB_NOTIFY_VEHICLE_STATUS.<br>
 */
typedef enum
{
    /*! start vehicle status session.<br>*/
    IPOD_PLAYER_VEHICLE_STATUS_START = 0,
    
    /*! stop vehicle status session.<br>*/
    IPOD_PLAYER_VEHICLE_STATUS_STOP, 

    /*! location information status is unknown.<br>*/
    IPOD_PLAYER_VEHICLE_STATUS_UNKNOWN = 0xff 
    
} IPOD_PLAYER_VEHICLE_STATUS;

/*!
 * \enum IPOD_PLAYER_STATE_ADJUST
 * This is an enumeration of state adjustment.<br>
 * This enumeration is used in #iPodPlayerSetAudioMode.<br>
 */
typedef enum
{
    /*! Status adjustment is disabled when this value is set */
    IPOD_PLAYER_STATE_ADJUST_DISABLE = 0,
    
    /*! Status adjustment is enabled when this value is set */
    IPOD_PLAYER_STATE_ADJUST_ENABLE
} IPOD_PLAYER_STATE_ADJUST;

/**
 * \name location Information type Mask
 * \anchor ChapterInfoMask
 */
 
/*\{*/

/*! GlobalPositioningSystemFixData - NMEA GPGGA sentences */
#define IPOD_PLAYER_NMEA_MASK_GPGGA     0x00000001

/*! RecommendedMinimumSpecificGPSTransitData - NMEA GPRMC sentences */
#define IPOD_PLAYER_NMEA_MASK_GPRMC     0x00000002

/*! GPSSatellitesInView - NMEA GPGSV sentences */
#define IPOD_PLAYER_NMEA_MASK_GPGSV     0x00000004

/*! VehicleSpeedData - NMEA PASCD sentences */
#define IPOD_PLAYER_NMEA_MASK_PASCD     0x00000008

/*! VehicleGyroData - NMEA PAGCD sentences */
#define IPOD_PLAYER_NMEA_MASK_PAGCD     0x00000010

/*! VehicleAccelerometerData - NMEA PAACD sentences */
#define IPOD_PLAYER_NMEA_MASK_PAACD     0x00000020

/*! VehicleHeadingData - NMEA GPHDT sentences */
#define IPOD_PLAYER_NMEA_MASK_GPHDT     0x00000040

/*\}*/

/**
 * \name Video Setting Defines
 * \anchor VideSettingDefines
 */
 /*\{*/
 
/*! This is video class to decide video output */
#define IPOD_PLAYER_VIDEO_OUTPUT    0x0001

/*! This is video class to decide screen fitness */
#define IPOD_PLAYER_VIDEO_SCREEN    0x0002

/*! This is video class to decide the video signal */
#define IPOD_PLAYER_VIDEO_SIGNAL    0x0004

/*! This is video class to decide audio output */
#define IPOD_PLAYER_VIDEO_LINEOUT   0x0008

/*! This is video class to decide kind of cable */
#define IPOD_PLAYER_VIDEO_CABLE     0x0010

/*! This is video class to decide captioning */
#define IPOD_PLAYER_VIDEO_CAPTION   0x0020

/*! This is video class to decide the aspect ratio */
#define IPOD_PLAYER_VIDEO_ASPECT    0x0040

/*! This is video class to decide the subtitle */
#define IPOD_PLAYER_VIDEO_SUBTITLE  0x0080

/*! This is video class to decide the alternate audio */
#define IPOD_PLAYER_VIDEO_ALTAUDIO  0x0100

/*! This is video class to decide the behavior with power */
#define IPOD_PLAYER_VIDEO_POWER     0x0200

/*! This is video class to decide the voice over setting */
#define IPOD_PLAYER_VIDEO_VOICEOVER 0x0400

/*\}*/

/**
 * \name Track Capability Mask
 * \anchor TrackCapabilityMask
 */
 
/*\{*/

/*! Track is audiobook */
#define IPOD_PLAYER_CAP_MASK_IS_AUDIOBOOK        0x00000001
/*! Track has chapter */
#define IPOD_PLAYER_CAP_MASK_HAS_CHAPTER         0x00000002
/*! Track has coverart */
#define IPOD_PLAYER_CAP_MASK_HAS_COVERART        0x00000004
/*! Track is podcast episode */
#define IPOD_PLAYER_CAP_MASK_IS_PODCAST          0x00000008
/*! Track has release date */
#define IPOD_PLAYER_CAP_MASK_HAS_RELEASE_DATE    0x00000010
/*! Track has description */
#define IPOD_PLAYER_CAP_MASK_HAS_DESCRIPTION     0x00000020
/*! Track contains video */
#define IPOD_PLAYER_CAP_MASK_CONTAINTS_VIDEO     0x00000040
/*! Track is queued as video */
#define IPOD_PLAYER_CAP_MASK_AS_VIDEO            0x00000080
/*! Track can generate intelligent playlist */
#define IPOD_PLAYER_CAP_MASK_INTELLIGENT         0x00000100
/*! Track is iTunesU episode */
#define IPOD_PLAYER_CAP_MASK_ITUENS              0x00000200

/*\}*/

/**
 * \name Apple Device Event Mask
 * \anchor AppleDeviceEventMask
 */
 
/*\{*/

/*! Radio tagging status event */
#define IPOD_PLAYER_EVENT_MASK_RADIO_TAGGING     0x00000001
/*! Camera status event */
#define IPOD_PLAYER_EVENT_MASK_CAMERA            0x00000002
/*! Chaging info event */
#define IPOD_PLAYER_EVENT_MASK_CHARGING          0x00000004
/*! Database change event of Local Device media library */
#define IPOD_PLAYER_EVENT_MASK_DATABASE          0x00000008
/*! Current plyaing iOS Application name status event */
#define IPOD_PLAYER_EVENT_MASK_IOSAPP            0x00000010
/*! Out mode status event */
#define IPOD_PLAYER_EVENT_MASK_OUT               0x00000020
/*! Bluetooth connection status event */
#define IPOD_PLAYER_EVENT_MASK_BT                0x00000040
/*! Current playing iOS Application full name status event. Full name means which is displayed in Apple device's display */
#define IPOD_PLAYER_EVENT_MASK_IOSAPP_FULL       0x00000080
/*! Assistive touch status event */
#define IPOD_PLAYER_EVENT_MASK_ASSISTIVE         0x00000100
/*! power status update */
#define IPOD_PLAYER_EVENT_MASK_POWER             0x00000200
/*! sample rate update */
#define IPOD_PLAYER_EVENT_MASK_SAMPLE_RATE       0x00000400
/*! Store database of Local Device media library */
#define IPOD_PLAYER_EVENT_MASK_STORE_DB          0x00000800
/*! update playback list */
#define IPOD_PLAYER_EVENT_MASK_UPDATE_PBLIST     0x00001000
/*! Database change event of Apple Music Radio media library */
#define IPOD_PLAYER_EVENT_MASK_DATABASE_AMR      0x00002000
/*! Store database of Apple Music Radio media library */
#define IPOD_PLAYER_EVENT_MASK_STORE_DB_AMR      0x00004000

/*\}*/


/**
 * \name Track Info Mask
 * \anchor TrackInfoMask
 */

/*\{*/

/*! Bit of track name */
#define IPOD_PLAYER_TRACK_INFO_MASK_TRACK_NAME       0x00000001
/*! Bit of album name */
#define IPOD_PLAYER_TRACK_INFO_MASK_ALBUM_NAME       0x00000002
/*! Bit of artist name */
#define IPOD_PLAYER_TRACK_INFO_MASK_ARTIST_NAME      0x00000004
/*! Bit of podcast name */
#define IPOD_PLAYER_TRACK_INFO_MASK_PODCAST_NAME     0x00000008
/*! Bit of description */
#define IPOD_PLAYER_TRACK_INFO_MASK_DESCRIPTION      0x00000010
/*! Bit of lyric */
#define IPOD_PLAYER_TRACK_INFO_MASK_LYRIC            0x00000020
/*! Bit of genre */
#define IPOD_PLAYER_TRACK_INFO_MASK_GENRE            0x00000040
/*! Bit of composer */
#define IPOD_PLAYER_TRACK_INFO_MASK_COMPOSER         0x00000080
/*! Bit of release date */
#define IPOD_PLAYER_TRACK_INFO_MASK_RELEASE_DATE     0x00000100
/*! Bit of capability */
#define IPOD_PLAYER_TRACK_INFO_MASK_CAPABILITY       0x00000200
/*! Bit of track length */
#define IPOD_PLAYER_TRACK_INFO_MASK_TRACKLENGTH      0x00000400
/*! Bit of chapter count */
#define IPOD_PLAYER_TRACK_INFO_MASK_CHAPTER_COUNT    0x00000800
/*! Bit of uid */
#define IPOD_PLAYER_TRACK_INFO_MASK_UID              0x00001000
/*! Bit of kind of track */
#define IPOD_PLAYER_TRACK_INFO_MASK_TRACK_KIND       0x00002000
/*! Bit of coverart data */
#define IPOD_PLAYER_TRACK_INFO_MASK_COVERART         0x00004000

/*\}*/


/**
 * \name Chapter Info Mask
 * \anchor ChapterInfoMask
 */
 
/*\{*/

/*! Bit of chapter length */
#define IPOD_PLAYER_CHAPTER_INFO_MASK_LENGTH 0x01
/*! Bit of chapter name */
#define IPOD_PLAYER_CHAPTER_INFO_MASK_NAME   0x02

/*\}*/


/**
 * \name Apple Device Feature mask
 * \anchor AppleDeviceFeatureMask
 */
/*\{*/
/*! Apple device supports video output */
#define IPOD_PLAYER_FEATURE_MASK_VIDEO                                       0x00000001
/*! Apple device supports video browsing. If this bit is set, application can use #iPodPlayerSelectAV */
#define IPOD_PLAYER_FEATURE_MASK_VIDEO_BROWSING                              0x00000002
/*! Apple device supports to communicate with iOS Application. */
#define IPOD_PLAYER_FEATURE_MASK_IOSAPP                                      0x00000004
/*! Apple device supports to notify the event of Apple device. */
#define IPOD_PLAYER_FEATURE_MASK_DEVICE_NOTIFICATION                         0x00000008
/*! Apple device supports to start the iOS Application by application. */
#define IPOD_PLAYER_FEATURE_MASK_APP_START                                   0x00000010
/*! Apple device supports to intelligent playlist. If this bit is set, application can use #iPodPlayerCreateIntelligentPL */
#define IPOD_PLAYER_FEATURE_MASK_INTTELLIGENT_PL                             0x00000020
/*! Apple device supports to display the sending image data.*/
#define IPOD_PLAYER_FEATURE_MASK_DISPLAY_IMAGE                               0x00000040
/*! Apple device supports to set the video delay */
#define IPOD_PLAYER_FEATURE_MASK_VIDEO_DELAY                                 0x00000080
/*! Apple device supports to operate by unique ID of track */
#define IPOD_PLAYER_FEATURE_MASK_UID                                         0x00000100
/*! Apple device supports song tag */
#define IPOD_PLAYER_FEATURE_MASK_SONG_TAG                                    0x00000200
/*! Apple device supports HMI control */
#define IPOD_PLAYER_FEATURE_MASK_HMI_CONTROL                                 0x00000400
/*! Apple device supports GPS control */
#define IPOD_PLAYER_FEATURE_MASK_GPS                                         0x00000800
/*! If this bit is set, #iPodPlayerGetTrackInfo is restricted. it can get only ..... */
#define IPOD_PLAYER_FEATURE_MASK_TRACK_INFO_RESTRICT                         0x00001000
/*! Apple device supports lineout of video */
#define IPOD_PLAYER_FEATURE_MASK_VIDEO_LINE_OUT                              0x00002000
/*! Apple device supports NTSC */
#define IPOD_PLAYER_FEATURE_MASK_VIDEO_NTSC                                  0x00004000
/*! Apple device supports PAL */
#define IPOD_PLAYER_FEATURE_MASK_VIDEO_PAL                                   0x00008000
/*! Apple device supports Composite connection */
#define IPOD_PLAYER_FEATURE_MASK_VIDEO_COMPOSITE                             0x00010000
/*! Apple device supports S-Video connection */
#define IPOD_PLAYER_FEATURE_MASK_VIDEO_SVIDEO                                0x00020000
/*! Apple device supports Component connection */
#define IPOD_PLAYER_FEATURE_MASK_VIDEO_COMPONENT                             0x00040000
/*! Apple device supports closed caption */
#define IPOD_PLAYER_FEATURE_MASK_VIDEO_CLOSED_CAPTION                        0x00080000
/*! Apple device supports aspect ratio that is 4:3 */
#define IPOD_PLAYER_FEATURE_MASK_VIDEO_ASPECT_4_3                            0x00100000
/*! Apple device supports aspect ratio that is 16:9 */
#define IPOD_PLAYER_FEATURE_MASK_VIDEO_ASPECT_16_9                           0x00200000
/*! Apple device supports alternate audio channel */
#define IPOD_PLAYER_FEATURE_MASK_VIDEO_ALT_AUDIO                             0x00400000
/*! Apple device supports subtitle of video */
#define IPOD_PLAYER_FEATURE_MASK_VIDEO_SUBTITLE                              0x00800000

/*\}*/


/**
 * \name Device Status Mask
 * \anchor DeviceStatusMask
 */
 
/*\{*/

/*! Mode of current Apple device */
#define IPOD_PLAYER_IPOD_STATUS_INFO_MASK_MODE               0x00000001
/*! Status of current Apple device */
#define IPOD_PLAYER_IPOD_STATUS_INFO_MASK_STATUS             0x00000002
/*! Notify event of current Apple device */
#define IPOD_PLAYER_IPOD_STATUS_INFO_MASK_NOTIFY_EVENT       0x00000004
/*! Running iOS Application of current Apple device */
#define IPOD_PLAYER_IPOD_STATUS_INFO_MASK_RUNNING_APP        0x00000008

/*\}*/


/**
 * \name HMI Feature Mask
 * \anchor HMIFeatureMask
 */
 
/*\{*/

/*! Apple device can display the audio content in HMI mode. It is including the all meadia excluding the video.<br>
 *  If application call the #iPodPlayerHMISetSupportedFeature, this bit must be set. */
#define IPOD_PLAYER_HMI_FEATURE_MASK_AUDIO_CONTENTS  0x00000001
/*! Apple device can display the phone call in HMI mode */
#define IPOD_PLAYER_HMI_FEATURE_MASK_PHONE_CALL      0x00000002
/*! Apple device can display the SMS/MMS in HMI mode*/
#define IPOD_PLAYER_HMI_FEATURE_MASK_SMS_MMS         0x00000004
/*! Apple device can display the voicemail in HMI mode */
#define IPOD_PLAYER_HMI_FEATURE_MASK_VOICEMAIL       0x00000008
/*! Apple device can display the push notification in HMI mode*/
#define IPOD_PLAYER_HMI_FEATURE_MASK_PUSH_NOTIFY     0x00000010
/*! Apple device can display the clock alarm in HMI mode */
#define IPOD_PLAYER_HMI_FEATURE_MASK_ALARM           0x00000020
/*! Apple device can display the test patern for development<br>
 *  This bit must be set before mode status changes to IPOD_PLAYER_MODE_HMI */
#define IPOD_PLAYER_HMI_FEATURE_MASK_TEST_PATTERN    0x00000040
/*! Apple device can display the minimam display. E.g. it can display over the TV shows
 *  This bit and #IPOD_PLAYER_HMI_FEATURE_MASK_FULL_UI both must not set. */
#define IPOD_PLAYER_HMI_FEATURE_MASK_MINIMAM_UI      0x00000080
/*! Apple device can display the full display. E.g it can display over the TV shows at full.
 *  This bit and #IPOD_PLAYER_HMI_FEATURE_MASK_MINIMAM_UI both must not set. */
#define IPOD_PLAYER_HMI_FEATURE_MASK_FULL_UI         0x00000100

/*\}*/

/**
 * \name Intelligent Property Mask
 * \anchor IntelligentPropertyMask
 */
 /*\{*/
 
 #define IPOD_PLAYER_INTELLIGENT_PROPERTY_INTELLIGENT_LIST 0x00000001
 /*\}*/
 
/**
 * \name Device Property Mask
 * \anchor DevicePropertyMask
 */

/*\{*/

/*! Name of device  */
#define IPOD_PLAYER_DEVICE_MASK_NAME                 0x00000001
/*! Software version of device */
#define IPOD_PLAYER_DEVICE_MASK_SOFT_VERSION         0x00000002
/*! Serial number of device */
#define IPOD_PLAYER_DEVICE_MASK_SERIAL_NUMBER        0x00000004
/*! Max payload size of device */
#define IPOD_PLAYER_DEVICE_MASK_MAX_PAYLOAD_SIZE     0x00000008
/*! Supported feature of device */
#define IPOD_PLAYER_DEVICE_MASK_SUPPORTED_FEATURE    0x00000010
/*! Supportting and current event of device */
#define IPOD_PLAYER_DEVICE_MASK_EVENT                0x00000020
/*! File space of device */
#define IPOD_PLAYER_DEVICE_MASK_FILE_SPACE           0x00000040
/*! Coverart format of device */
#define IPOD_PLAYER_DEVICE_MASK_FORMAT               0x00000080
/*! Monochrome image limit of device */
#define IPOD_PLAYER_DEVICE_MASK_MONO_LIMIT           0x00000100
/*! Color image limit of device */
#define IPOD_PLAYER_DEVICE_MASK_COLOR_LIMIT          0x00000200

/*\}*/

/**
 * \name Song Tag Options Mask
 * \anchor SongTagOptionsMask
 */

/*\{*/

/*! If #iPodPlayerCloseSongTagFile is called or Apple device is disconnected, binaly file data bytes writes to the file.<br>
 * If this bit is set, opetionLen parameter of #iPodPlayerOpenSongTagFile must be set the non zero value and 
 * optionData parameter of #iPodPlayerOpenSongTagFile must be set the non NULL. */
#define IPOD_PLAYER_SONG_TAG_OPTIONS_BINALY         0x00000001
/*! If #iPodPlayerCloseSongTagFile is called or Apple device is disconnected, some XML elements are written to the file.<br>
 * There are open time, close time, model, software version and serial number in XML elements.<br>*/
#define IPOD_PLAYER_SONG_TAG_OPTIONS_DEVICE_INFO    0x00000002
/*! If #iPodPlayerCloseSongTagFile is called or Apple device is disconnected, some XML element is written to the file.<br>
 * There is signature element.<br>
 * If this bit is set, #IPOD_PLAYER_SONG_TAG_OPTIONS_DEVICE_INFO bit must be set. */
#define IPOD_PLAYER_SONG_TAG_OPTIONS_SIGNATURE      0x00000004

/*\}*/

/*! Application uses to communicate with Apple device by USB */
#define IPOD_PLAYER_USE_DEVICE_USB              0x0001

/*! Application uses to communicate with Apple device by Bluetooth */
#define IPOD_PLAYER_USE_DEVICE_BT               0x0002

/*! Application uses to communicate with Apple device by UART*/
#define IPOD_PLAYER_USE_DEVICE_UART             0x0004

/*! If this bit is set, GPS radio power it must be off */
#define IPOD_PLAYER_GPS_POEWR                   0x0001
/*! If this bit is set, Application can send the GPS data to Apple device*/
#define IPOD_PLAYER_GPS_DATA_SEND               0x0002
/*! If this bit is set, NEMA data is filtered by Apple  device. also Application can be filetered the NEMA data.*/
#define IPOD_PLAYER_GPS_NMEA_FILTER             0x0004
/**
 * \name HMI Event Mask
 * \anchor HMIEventMask
 */

/*\{*/
/*! Current track playing time in milliseconds. */
#define IPOD_PLAYER_HMI_STATUS_MASK_CURRENT_TIME     0x00000001
/*! Current playing track Index */
#define IPOD_PLAYER_HMI_STATUS_MASK_TRACK_INDEX      0x00000002
/*! Current playing chapter index */
#define IPOD_PLAYER_HMI_STATUS_MASK_CHAPTER_INDEX    0x00000004
/*! Chapter total number that current playing track has */
#define IPOD_PLAYER_HMI_STATUS_MASK_CHAPTER_COUNT    0x00000008
/*! Current playback status. If this type is acquired, Result value is same as #IPOD_PLAYER_PLAY_STATUS. */
#define IPOD_PLAYER_HMI_STATUS_MASK_PLAYBACK_STATUS  0x00000010
/*! Current mute status. 0 means mute is off, 1 is mute on.*/
#define IPOD_PLAYER_HMI_STATUS_MASK_MUTE_STATUS      0x00000020
/*! Current volume status. This value is between 0 and 255. 0 means minimum volume, 255 means maximum volume. */
#define IPOD_PLAYER_HMI_STATUS_MASK_VOLUME_STATUS    0x00000040
/*! Current equalizer setting. It is indicated only equalizer index number. */
#define IPOD_PLAYER_HMI_STATUS_MASK_EQUALIZER_STATUS 0x00000080
/*! Current shuffle status. If this type is acquired, Result value is same as #IPOD_PLAYER_SHUFFLE_STATUS. */
#define IPOD_PLAYER_HMI_STATUS_MASK_SHUFFLE_STATUS   0x00000100
/*! Current repeat status. If this type is acquired, Result value is same as #IPOD_PLAYER_REPEAT_STATUS */
#define IPOD_PLAYER_HMI_STATUS_MASK_REPEAT_STATUS    0x00000200
/*! Current date.  */
#define IPOD_PLAYER_HMI_STATUS_MASK_CURRENT_DATE     0x00000400
/*! Current backlight level of Apple device. This value is between 0 and 255. 0 means minumum brightness, 255 meens full intensity. */
#define IPOD_PLAYER_HMI_STATUS_MASK_BACKLIGHT_STATUS 0x00000800
/*! Current sound check status. This value is 0 means sound check off, 1 means sound check on.*/
#define IPOD_PLAYER_HMI_STATUS_MASK_SOUND_CHECK      0x00001000
/*! Current speed status. If this type is acquired, Result value is same as #IPOD_PLAYER_PLAYING_SPEED. */
#define IPOD_PLAYER_HMI_STATUS_MASK_SPEED_STATUS     0x00002000
/*! Current absolue volume status. This value is between 0 and 255. 0 means minimum volume, 255 means maximum volume. */
#define IPOD_PLAYER_HMI_STATUS_MASK_ABSOLUTE_VOLUME  0x00004000
/*! Current track capabilities. If this type is set, Return value is same as \ref TrackCapabilityMask */
#define IPOD_PLAYER_HMI_STATUS_MASK_CAPABILITIES     0x00008000

/*\}*/

/**
 * \name Profile List Defines
 * \anchor ProfileListDefines
 */
/*\{*/
/*! Hands Free Profile */
#define IPOD_PLAYER_PROFILE_HFP                 0x0001
/*! Phone Book Access Profile */
#define IPOD_PLAYER_PROFILE_PBAP                0x0002
/*! Audio/Video Remove Control Profile */
#define IPOD_PLAYER_PROFILE_AVRCP               0x0004
/*! Adovanced Audio Distribution Profile */
#define IPOD_PLAYER_PROFILE_A2DP                0x0008
/*! Human Interface Device Profile */
#define IPOD_PLAYER_PROFILE_HID                 0x0010
/*! Apple iAP Profile */
#define IPOD_PLAYER_PROFILE_AiP                 0x0020
/*! PAN-NAP Profile */
#define IPOD_PLAYER_PROFILE_PANA                0x0040
/*! PAN-User Profile */
#define IPOD_PLAYER_PROFILE_PANU                0x0080
/*! Unknown Profile */
#define IPOD_PLAYER_PROFILE_UNKNOWN             0x8000
/*\}*/
/**
 * \name HMI Control Defines
 * \anchor HMIControlDefines
 */
/*\{*/

/*! Bit of button release */
#define IPOD_PLAYER_HMI_BUTTON_EVENT_RELEASE    0x0000
/*! Bit of Up button pressed.*/
#define IPOD_PLAYER_HMI_BUTTON_EVENT_UP         0x0001
/*! Bit of Down button pressed. */
#define IPOD_PLAYER_HMI_BUTTON_EVENT_DOWN       0x0002
/*! Bit of Right button pressed. */
#define IPOD_PLAYER_HMI_BUTTON_EVENT_RIGHT      0x0004
/*! Bit of Left button pressed. */
#define IPOD_PLAYER_HMI_BUTTON_EVENT_LEFT       0x0008
/*! Bit of Select button pressed. */
#define IPOD_PLAYER_HMI_BUTTON_EVENT_SELECT     0x0010
/*! Bit of Menu button pressed. */
#define IPOD_PLAYER_HMI_BUTTON_EVENT_MENU       0x0020

/*!
 * \enum IPOD_PLAYER_HMI_BUTTON_SOURCE
 * This is an enumeration of operation source.<br>
 * This enumeration is used in #iPodPlayerHMIButtonInput
 */
typedef enum
{
    /*! Control from car center console */
    IPOD_PLAYER_HMI_BUTTON_SOURCE_CONSOLE = 0x00,
    /*! Control from steering wheel */
    IPOD_PLAYER_HMI_BUTTON_SOURCE_WHEEL,
    /*! Control from car dashboard */
    IPOD_PLAYER_HMI_BUTTON_SOURCE_DASHBOARD
} IPOD_PLAYER_HMI_BUTTON_SOURCE;

/*!
 * \enum IPOD_PLAYER_HMI_ROTATION_SOURCE
 * This is an enumeration of rotation device.<br>
 * This enumeration is used in #iPodPlayerHMIRotationInput
 */
typedef enum
{
    /*! Rotation source is car center conlose.  */
    IPOD_PLAYER_HMI_ROTATION_SOURCE_CAR_CONSOLE = 0x00,
    /*! Rotation source is steering wheel.  */
    IPOD_PLAYER_HMI_ROTATION_SOURCE_STEERING_WHEEL,
    /*! Rotation source is car dashboard.  */
    IPOD_PLAYER_HMI_ROTATION_SOURCE_CAR_DASHBOARD,
    
    /*! Rotation device is unknown */
    IPOD_PLAYER_HMI_ROTATION_SOURCE_UNKNOWN = 0xFF
} IPOD_PLAYER_HMI_ROTATION_SOURCE;


/*!
 * \enum IPOD_PLAYER_HMI_ROTATION_DEVICE
 * This is an enumeration of rotation device.<br>
 * This enumeration is used in #iPodPlayerHMIRotationInput
 */
typedef enum
{
    /*! Rotation device is free wheel.  */
    IPOD_PLAYER_HMI_ROTATION_DEVICE_FREE = 0x00,
    /*! Rotation device is unknown */
    IPOD_PLAYER_HMI_ROTATION_DEVICE_UNKNOWN = 0xFF
} IPOD_PLAYER_HMI_ROTATION_DEVICE;

/*!
 * \enum IPOD_PLAYER_HMI_ROTATION_DIRECTION
 * This is an enumeration of direction.<br>
 * This enumeration is used in #iPodPlayerHMIRotationInput
 */
typedef enum
{
    /*! Rotation device turns to right(clockwise) */
    IPOD_PLAYER_HMI_ROTATION_DIRECTION_RIGHT = 0x00,
    /*! Rotation device turns to left(counterclockwise) */
    IPOD_PLAYER_HMI_ROTATION_DIRECTION_LEFT,
    /*! Rotation device turns to unknown */
    IPOD_PLAYER_HMI_ROTATION_DIRECTION_UNKNOWN = 0xFF
} IPOD_PLAYER_HMI_ROTATION_DIRECTION;

/*!
 * \enum IPOD_PLAYER_HMI_ROTATION_ACTION
 * This is an enumeration of rotation action.<br>
 * This enumeration is used in #iPodPlayerHMIRotationInput
 */
typedef enum
{
    /*! Rotation device is released */
    IPOD_PLAYER_HMI_ROTATION_ACTION_RELEASE = 0x00,
    /*! Rotation device is turning in progess */
    IPOD_PLAYER_HMI_ROTATION_ACTION_PROGRESS,
    /*! Rotation device's action has nothing */
    IPOD_PLAYER_HMI_ROTATION_ACTION_NOTHING,
    /*! Rotation device's action is unknown */
    IPOD_PLAYER_HMI_ROTATION_ACTION_UNKNOWN = 0xFF
} IPOD_PLAYER_HMI_ROTATION_ACTION;

/*!
 * \enum IPOD_PLAYER_HMI_ROTATION_TYPE
 * This is an enumeration of rotation type.<br>
 * This enumeration is used in #iPodPlayerHMIRotationInput
 */
typedef enum
{
    /*! Rotation type is detent */
    IPOD_PLAYER_HMI_ROTATION_TYPE_DETENT = 0x00,
    /*! Rotation type is degree */
    IPOD_PLAYER_HMI_ROTATION_TYPE_DEGREE,
    /*! Rotation type is unknown */
    IPOD_PLAYER_HMI_ROTATION_TYPE_UNKNOWN = 0xFF
} IPOD_PLAYER_HMI_ROTATION_TYPE;


/*!
 * \struct IPOD_PLAYER_HMI_ROTATION_INFO
 * This is a strucutre of rotation information.<br>
 * This structure is used in #iPodPlayerHMIRotationInput<br>
 */
typedef struct
{
    /*! the source of wheel */
    IPOD_PLAYER_HMI_ROTATION_SOURCE source;
    /*! Intend the using rocation device */
    IPOD_PLAYER_HMI_ROTATION_DEVICE device;
    /*! Intend the rotation direction */
    IPOD_PLAYER_HMI_ROTATION_DIRECTION direction;
    /*! Intend the rotation action */
    IPOD_PLAYER_HMI_ROTATION_ACTION action;
    /*! Intend the rotation type */
    IPOD_PLAYER_HMI_ROTATION_TYPE type;
    /*! Maximum number of detents or degrees. Maximum size must be less than 360 */
    U16 max;
} IPOD_PLAYER_HMI_ROTATION_INFO;

/*!
 * \enum IPOD_PLAYER_HMI_PLAYBACK_EVENT
 * This is an enumeration of playback event.<br>
 * This enumeration is used in #iPodPlayerHMIPlaybackInput<br>
 */
typedef enum
{
    /*! Playback event is play/pause toggle */
    IPOD_PLAYER_HMI_PLAYBACK_EVENT_TOGGLE = 0x00,
    /*! Playback event is play */
    IPOD_PLAYER_HMI_PLAYBACK_EVENT_PLAY,
    /*! Playback event is pause */
    IPOD_PLAYER_HMI_PLAYBACK_EVENT_PAUSE,
    /*! Playback event is fast forward */
    IPOD_PLAYER_HMI_PLAYBACK_EVENT_FF,
    /*! Playback event is rewind */
    IPOD_PLAYER_HMI_PLAYBACK_EVENT_RW,
    /*! Playback event is next track */
    IPOD_PLAYER_HMI_PLAYBACK_EVENT_NEXT_TRACK,
    /*! Playback event is previous track */
    IPOD_PLAYER_HMI_PLAYBACK_EVENT_PREV_TRACK,
    /*! Playback event is release */
    IPOD_PLAYER_HMI_PLAYBACK_EVENT_RELEASE,
    /*! Playback event is unknown */
    IPOD_PLAYER_HMI_PLAYBACK_EVENT_UNKNOWN = 0xFF
} IPOD_PLAYER_HMI_PLAYBACK_EVENT;

/*!
 * \enum IPOD_PLAYER_HMI_APP_STATUS
 * This is an enumeration of application statust.<br>
 * This enumeration is used in #iPodPlayerHMISetApplicationStatus<br>
 */
typedef enum
{
    /*! Application hides the HMI display */
    IPOD_PLAYER_HMI_APP_STATUS_HIDE_HMI_DISPLAY = 0x00,
    /*! Application shows the HMI display */
    IPOD_PLAYER_HMI_APP_STATUS_SHOW_HMI_DISPLAY,
    /*! Application hides the HMI audio */
    IPOD_PLAYER_HMI_APP_STATUS_HIDE_HMI_AUDIO,
    /*! Application shows the HMI audio */
    IPOD_PLAYER_HMI_APP_STATUS_SHOW_HMI_AUDIO,
    /*! Application shows with brightness.(e.g. daytime) */
    IPOD_PLAYER_HMI_APP_STATUS_SHOW_BRIGHT,
    /*! Application shows with dimmed(e.g. nighttime) */
    IPOD_PLAYER_HMI_APP_STATUS_SHOW_DIMMED,
    /*! Application status is unknown */
    IPOD_PLAYER_HMI_APP_STATUS_UNKNONWN
} IPOD_PLAYER_HMI_APP_STATUS;


/*!
 * \enum IPOD_PLAYER_HMI_FEATURE_TYPE
 * This is an enumeration of rotation action.<br>
 * This enumeration is used in #iPodPlayerHMIRotationInput
 */
typedef enum
{
    /*! Current set HMI feature is available */
    IPOD_PLAYER_HMI_FEATURE_TYPE_CURRENT = 0x00,
    /*! Apple device supported all feature is available */
    IPOD_PLAYER_HMI_FEATURE_TYPE_ALL,
    /*! Application hides the HMI display */
    IPOD_PLAYER_HMI_FEATURE_TYPE_UNKNOWN = 0xFF
} IPOD_PLAYER_HMI_FEATURE_TYPE;


/*\}*/

/*!
 * \struct IPOD_PLAYER_AUDIO_SETTING
 * This is a structure of audio mode.<br>
 */
typedef struct
{
    /*! This decides to enable/disable the audio. It can set the #IPOD_PLAYER_SOUND_MODE_OFF or #IPOD_PLAYER_SOUND_MODE_ON.<br>
     *  If this value is #IPOD_PLAYER_SOUND_MODE_OFF, audio is not output even if Apple device is playing.<br>
     *  If this value is #IPOD_PLAYER_SOUND_MODE_ON, audio is output when Apple device is playing.<br> */
    IPOD_PLAYER_SOUND_MODE mode;
    /*! This decides to enable/disable the audio adjustment. 
     *  It can set the #IPOD_PLAYER_STATE_ADJUST_DISABLE or #IPOD_PLAYER_STATE_ADJUST_ENABLE.<br>
     *  If this value is #IPOD_PLAYER_STATE_ADJUST_DISABLE, audio adjustment is disabled.<br>
     *  If this value is #IPOD_PLAYER_STATE_ADJUST_ENABLE, audio adjustment is enabled.<br>
     *  Also this value has an effect when mode is #IPOD_PLAYER_SOUND_MODE_ON.<br> */
    IPOD_PLAYER_STATE_ADJUST adjust;
} IPOD_PLAYER_AUDIO_SETTING;


/*!
 * \struct IPOD_PLAYER_VIDEO_SETTING
 * This is a structure of kind of video setting.<br>
 */
typedef struct
{
    /*! Mask of Setting/Getting. Refer \ref VideSettingDefines<br> 
     *  This is a bit mask showing which members of the structure #IPOD_PLAYER_VIDEO_SETTING have valid information. */
    U32 videoSettingMask;
    /*! Video Output Setting. 0: Disable Video Output, 1: Enable Video Output, 0xFF: Unknown */
    U8 videoOutput;
    /*! Screen Setting of Video. 0: Entire Screen, 1: Fit to Edge, 0xFF: Unknown */
    U8 screenSetting;
    /*! Video Signal Setting. 0: NTSC, 1: PAL, 0xFF: Unknown */
    U8 videoSignal;
    /*! Video Lineout Setting. 0: Digital Output, 1:Analog Output, 0xFF: Unknown */
    U8 videoLineout;
    /*! Video Cable Setting. 0: Default of Apple device, 1: Composite, 2: S-Video, 3: Component, 0xFF: Unknown */
    U8 videoCable;
    /*! Video Caption Setting. 0: Disable, 1: Enable, 0xFF: Unknown */
    U8 videoCaption;
    /*! Aspect Ratio Setting. 0: 4:3, 1: 16:9, 0xFF: Unknown */
    U8 aspectRatio;
    /*! Video Subtitle Setting. 0: Disable, 1: Enable, 0xFF: Unknown */
    U8 subtitle;
    /*! Alternate Audio Setting. 0: Disable, 1:Enable, 0xFF: Unknown */
    U8 altanateAudio;
    /*! Video Power Setting. 0: When USB power is OFF, video is not paused. 1: When USB power on, video is paused, 0xFF: Unknown */
    U8 power;
    /*! Voice Over Setting. 0: Disable, 1:Enable, 0xFF: Unknown */
    U8 voiceOver;
    
} IPOD_PLAYER_VIDEO_SETTING;

/*!
 * \struct IPOD_PLAYER_ENTRY_LIST
 * This is a structure of database entry<br>
 */
typedef struct
{
    /*! Index of entry */
    U32 trackIndex;
    /*! Name of entry */
    U8 name[IPOD_PLAYER_ENTRY_NAME_LEN_MAX];
} IPOD_PLAYER_ENTRY_LIST;

/*!
 * \enum IPOD_PLAYER_PLAYING_SPEED
 * This is an enumeration of audiobook speed.<br>
 */
typedef enum
{
    /*! Audiobook speed set/get as slow. This is used in #iPodPlayerSetPlaySpeed or #iPodPlayerGetPlaySpeed */
    IPOD_PLAYER_PLAYING_SPEED_SLOW = 0x00,

    /*! Audiobook speed set/get as normal. This is used in #iPodPlayerSetPlaySpeed or #iPodPlayerGetPlaySpeed */
    IPOD_PLAYER_PLAYING_SPEED_NORMAL,

    /*! Audiobook speed set/get as fast. This is used in #iPodPlayerSetPlaySpeed or #iPodPlayerGetPlaySpeed */
    IPOD_PLAYER_PLAYING_SPEED_FAST,
    
    /*! Audiobook speed is unknown */
    IPOD_PLAYER_PLAYING_SPEED_UNKNOWN = 0xFF

} IPOD_PLAYER_PLAYING_SPEED;

/*!
 * \struct IPOD_PLAYER_SET_VEHICLE_STATUS
 * This is a structure of setting vehicle status<br>
 */
typedef struct
{
    /*! status of range */
    U16 range;
    /*! status of outside temperature */
    U16 outsideTemperature;
    /*! status of range Warning */
    U8  rangeWarning;
    
} IPOD_PLAYER_SET_VEHICLE_STATUS;

/*!
 * \enum IPOD_PLAYER_WEEKDAY
 * This is an enumeration of weekday.<br>
 */
typedef enum
{
    /*! Sunday of weekday */
    IPOD_PLAYER_WEEK_SUNDAY = 0,
    /*! Moday of weekday */
    IPOD_PLAYER_WEEK_MONDAY,
    /*! Tuesday of weekday */
    IPOD_PLAYER_WEEK_TUESDAY,
    /*! Wednesday of weekday */
    IPOD_PLAYER_WEEK_WEDNESDAY,
    /*! Thursday of weekday */
    IPOD_PLAYER_WEEK_THURSDAY,
    /*! Friday of weekday */
    IPOD_PLAYER_WEEK_FRIDAY,
    /*! Saturday of weekday */
    IPOD_PLAYER_WEEK_SATURDAY,
    /*! Unknown weekday */
    IPOD_PLAYER_WEEK_UNKNOWN = 0xFF
} IPOD_PLAYER_WEEKDAY;

/*!
 * \enum IPOD_PLAYER_POWER_MODE
 * This is an enumeration of accessory power modes.<br>
 */
typedef enum
{
    /*! No Power */
    IPOD_PLAYER_ACC_POWER_NO_POWER = 0,
    /*! Low Power Mode */
    IPOD_PLAYER_ACC_POWER_LOW_POWER,
    /*! Intermittent Hight Power Mode */
    IPOD_PLAYER_ACC_POWER_HIGH_POWER
} IPOD_PLAYER_ACC_POWER_MODES;

/*!
 * \enum IPOD_PLAYER_POWER_STATE
 * This is an enumeration of accessory power state.<br>
 */
typedef enum
{
    /*! Disabled */
    IPOD_PLAYER_CHARGING_STATE_DISABLED = 0,
    /*! Charging */
    IPOD_PLAYER_CHARGING_STATE_CHARGING,
    /*! Charged */
    IPOD_PLAYER_CHARGING_STATE_CHARGED
} IPOD_PLAYER_BATTERY_CHARGING_STATE;

/*!
 * \enum IPOD_PLAYER_POWER_SUPPLY_CURRENT
 * This is an enumeration of current(mA) to charge to device.<br>
 */
typedef enum
{
    /*! charging current (0mA) */
    IPOD_PLAYER_CURRENT_0mA     = 0,
    /*! charging current (500mA) */
    IPOD_PLAYER_CURRENT_500mA   = 500,
    /*! charging current (1000mA) */
    IPOD_PLAYER_CURRENT_1000mA  = 1000,
    /*! charging current (1500mA) */
    IPOD_PLAYER_CURRENT_1500mA  = 1500,
    /*! charging current (2100mA) */
    IPOD_PLAYER_CURRENT_2100mA  = 2100,
    /*! charging current (2400mA) */
    IPOD_PLAYER_CURRENT_2400mA  = 2400
} IPOD_PLAYER_CURRENT;

/*!
 * \enum IPOD_PLAYER_CHARGING_NOTIFY_MASK_
 * This is an enumeration of charging notification mask.<br>
 */
    /*! Current drawn */
    #define IPOD_PLAYER_CHARGING_CURRENT_MASK       0x00000001
    /*! Power mode */
    #define IPOD_PLAYER_POWER_MODE_MASK             0x00000002
    /*! Charging buttery flag */
    #define IPOD_PLAYER_CHARGING_BUTTERY_MASK       0x00000004
    /*! External Charger Connect */
    #define IPOD_PLAYER_EXT_CHARGER_CONNECT_MASK    0x00000008
    /*! Battery Charge State */
    #define IPOD_PLAYER_CHARGE_STATE_MASK           0x00000010
    /*! Battery Charge Level */
    #define IPOD_PLAYER_CHARGE_LEVEL_MASK           0x00000020

/*!
 * \struct IPOD_PLAYER_POWER_NOTIFY
 * This is a structure of power update notification.<br>
 */
typedef struct
{
    /*! power update notification mask */
    U32                                 powerUpdateEventMask;
    /*! maximum current drawn */
    U16                                 maxCurrentDrawn;
    /*! charging buttery flag */
    U8                                  chargeButtery;
    /*! charging level */
    U16                                 chargeLevel;
    /*! power mode */
    IPOD_PLAYER_ACC_POWER_MODES         powerMode;
    /*! buttery charging state */
    IPOD_PLAYER_BATTERY_CHARGING_STATE  butteryState;
} IPOD_PLAYER_POWER_NOTIFY;

/*!
 * \struct IPOD_PLAYER_TRACK_RELEASE_DATE
 * This is a structure of current playing track release date.<br>
 */
typedef struct
{
    /*! Year of release date */
    U16 year;
    /*! Month(1-12) of release date */
    U8 month;
    /*! Day(1-31) of release date */
    U8 day;
    /*! Hours(0-23) of release date */
    U8 hours;
    /*! Minutes(0-59) of release date */
    U8 minutes;
    /*! Seconds(0-59) of release date */
    U8 seconds;
    /*! Weekday(0-6) of release date */
    IPOD_PLAYER_WEEKDAY weekday;
} IPOD_PLAYER_TRACK_RELEASE_DATE;



/*!
 * \struct IPOD_PLAYER_TRACK_INFO
 * This is a structure of current playing track information.<br>
 */
typedef struct
{
    /*! mask of set informaton. See \ref TrackInfoMask<br>
     *  This is a bit mask showing which members of the structure #IPOD_PLAYER_TRACK_INFO have valid information */
    U32 trackInfoMask;
    /*! Track name of current playing or indicated track */
    U8 trackName[IPOD_PLAYER_TRACK_NAME_LEN_MAX];
    /*! Album name of current playing or indicated track */
    U8 albumName[IPOD_PLAYER_ALBUM_NAME_LEN_MAX];
    /*! Artist name of current playing or indicated track */
    U8 artistName[IPOD_PLAYER_ARTIST_NAME_LEN_MAX];
    /*! Podcast name of current playing or indicated track */
    U8 podcastName[IPOD_PLAYER_PODCAST_NAME_LEN_MAX];
    /*! Description of current playing or indicated track */
    U8 description[IPOD_PLAYER_DESCRIPTION_LEN_MAX];
    /*! Lyric of current playing or indicated track */
    U8 lyric[IPOD_PLAYER_LYRIC_LEN_MAX];
    /*! Genre of current playing or indicated track */
    U8 genre[IPOD_PLAYER_GENRE_LEN_MAX];
    /*! Composer of current playing or indicated track */
    U8 composer[IPOD_PLAYER_COMPOSER_LEN_MAX];
    /*! Release date of current playing or indicated track */
    IPOD_PLAYER_TRACK_RELEASE_DATE date;
    /*! Capability of indicated track. See \ref TrackCapabilityMask */
    U32 capa;
    /*! Length of track total time */
    U32 length;
    /*! Chapter total count in track */
    U32 chapterCount;
    /*! unique id of indicated track */
    U64 uID;
    /*! kind of track.(0: Audio track, 1:Video track) */
    U8 trackKind;
    /*! MediaItem Persistent Identifier */
    U64 trackID;
    /*! MediaItem media type (0: songs, 1: podcast, 2: audiobook, -1: In case of unknown because it can not obtain information) */
    S32 mediaType;
} IPOD_PLAYER_TRACK_INFO;

/*!
 * \struct IPOD_PLAYER_CHAPTER_INFO
 * This is a structure of chapter information of current playing track.<br>
 */
typedef struct
{
    /*! Mask of set/get information. See \ref ChapterInfoMask<br>
     *  This is a bit mask showing which members of the structure #IPOD_PLAYER_CHAPTER_INFO have valid information */
    U32 chapterInfoMask;
    /*! Length of total time(ms) in chapter e.g if time is 1m20s100ms, this value is 80100 */
    U32 length;
    /*! Name of current playing chapter */
    U8 chapterName[IPOD_PLAYER_CHAPTER_NAME_LEN_MAX];
} IPOD_PLAYER_CHAPTER_INFO;


/*!
 * \struct IPOD_PLAYER_VERSION
 * This is a structure of version of Apple device.<br>
 */
typedef struct
{
    /*! major version */
    U8 majorVer;
    /*! minor version */
    U8 minorVer;
    /*! revision version */
    U8 revision;
    /*! dummy byte for padding */
    U8 dummy;
} IPOD_PLAYER_VERSION;

/*!
 * \struct IPOD_PLAYER_COVERART_FORMAT
 * This is a structure of format of coverart that Apple device supports.<br>
 */
typedef struct
{
    /*! ID of this image format */
    U16 formatId;
    /*! Support image width */
    U16 imageWidth;
    /*! Support image height */
    U16 imageHeight;
} IPOD_PLAYER_COVERART_FORMAT;

/*!
 * \struct IPOD_PLAYER_COVERART_HEADER
 * This is a structure of indicated track coverart count.<br>
 * This information is used in #iPodPlayerGetCoverArt.
 */
typedef struct
{
    /*! format of coverart */
    U16 formatId;
    /*! Image width in pixels */
    U16 width;
    /*! Image height in pixels */
    U16 height;
    /*! Insert rectangle. top-left point X */
    U16 topLeftX;
    /*! Insert rectangle. top-left point Y */
    U16 topLeftY;
    /*! Insert rectangle. bottom-right point X */
    U16 bottomRightX;
    /*! Insert rectangle. bottom-right point Y */
    U16 bottomRightY;
    /*! Rowsize in byte. */
    U32 rowsize;
} IPOD_PLAYER_COVERART_HEADER;

/*!
 * \struct IPOD_PLAYER_IMAGE_LIMIT
 * This is a structure of display limit of Apple device.<br>
 */
typedef struct
{
    /*! Maximum image width in pixels */
    U16 maxWidth;
    /*! Maximum image height in pixels  */
    U16 maxHeight;
} IPOD_PLAYER_IMAGE_LIMIT;


/*!
 * \enum IPOD_PLAYER_DB_TYPE
 * This is an enumeration of type of database.<br>
 */
typedef enum
{
    /*! Category of top layer */
    IPOD_PLAYER_DB_TYPE_ALL = 0x00,
    /*! Category of playlist */
    IPOD_PLAYER_DB_TYPE_PLAYLIST,
    /*! Category of artist */
    IPOD_PLAYER_DB_TYPE_ARTIST,
    /*! Category of album */
    IPOD_PLAYER_DB_TYPE_ALBUM,
    /*! Category of genre */
    IPOD_PLAYER_DB_TYPE_GENRE,
    /*! Category of track */
    IPOD_PLAYER_DB_TYPE_TRACK,
    /*! Category of composer */
    IPOD_PLAYER_DB_TYPE_COMPOSER,
    /*! Category of audiobook */
    IPOD_PLAYER_DB_TYPE_AUDIOBOOK,
    /*! Category of podcast */
    IPOD_PLAYER_DB_TYPE_PODCAST,
    /*! Category of nested playlist */
    IPOD_PLAYER_DB_TYPE_NESTED_PLAYLIST,
    /*! Category of Intelligent playlist */
    IPOD_PLAYER_DB_TYPE_INTELLIGENT,
    /*! Category of iTunesU */
    IPOD_PLAYER_DB_TYPE_ITUNESU,
    /*! Category of unknown */
    IPOD_PLAYER_DB_TYPE_UNKNOWN
} IPOD_PLAYER_DB_TYPE;

/*!
 * \enum IPOD_PLAYER_ITUENS_TYPE
 * This is an enumeration of typeof itunes.<br>
 */
typedef enum
{
    /*! Type of UniquID */
    IPOD_PLAYER_ITUNES_UID = 0x00,
    /*! Type of Last syncronize date */
    IPOD_PLAYER_ITUNES_LAST_SYNC,
    /*! Type of total number of audio */
    IPOD_PLAYER_ITUENS_TOTAL_AUDIO,
    /*! Type of total number of video */
    IPOD_PLAYER_ITUENS_TOTAL_VIDEO,
    /*! Type of total number of audiobook */
    IPOD_PLAYER_ITUENS_TOTAL_AUDIOBOOK,
    /*! Type of total number of photo */
    IPOD_PLAYER_ITUENS_PHOTO
} IPOD_PLAYER_ITUENS_TYPE;



/*!
 * \struct IPOD_PLAYER_DEVICE_PROPERTY
 * This is a structure of device properties.<br>
 */
typedef struct
{
    /*! Bit mask. see \ref DevicePropertyMask<br>
     *  This is a bit mask showing which members of the structure #IPOD_PLAYER_DEVICE_PROPERTY have valid information */
    U32 devicePropertyMask;
    /*! Name of Apple device */
    U8 name[IPOD_PLAYER_DEVICE_NAME_LEN_MAX];
    /*! Software vresion of Apple device */
    IPOD_PLAYER_VERSION softVer;
    /*! Serial number of Apple device */
    U8 serial[IPOD_PLAYER_SERIAL_LEN_MAX];
    /*! Supported feature of Apple device. See \ref AppleDeviceFeatureMask.*/
    U32 supportedFeatureMask;
    /*! Max size of sending to Apple device */
    U16 maxPayload;
    /*! Current Event notification of Apple device. This bits are set \ref AppleDeviceEventMask */
    U64 curEvent;
    /*! Supporting event of Apple device. This bits are set \ref AppleDeviceEventMask */
    U64 supportEvent;
    /*! File space for tagging of Apple device */
    U64 fileSpace;
    /*! Number of coverart format*/
    U32 formatCount;
    /*! Format of coverart that Apple device has */
    IPOD_PLAYER_COVERART_FORMAT format[IPOD_PLAYER_FORMAT_ARRAY_MAX];
    /*! Limit image size of monochrome image */
    IPOD_PLAYER_IMAGE_LIMIT mono;
    /*! Limit image size of color image */
    IPOD_PLAYER_IMAGE_LIMIT color;
} IPOD_PLAYER_DEVICE_PROPERTY;

/*!
 * \enum IPOD_PLAYER_MODE
 * This is an enumeration of mode of Apple device.<br>
 * This enumeration is used in #iPodPlayerSetMode or #iPodPlayerGetMode.
 */
typedef enum
{
    /*! This mode can operate the Apple device by itself. also this mode can not operate by external device. */
    IPOD_PLAYER_MODE_SELF_CONTROL = 0x00,
    /*! This mode can operate the Apple device by external device. also this mode can not operate the Apple device by itself */
    IPOD_PLAYER_MODE_REMOTE_CONTROL,
    /*! This mode can operate the Apple device by external device. also this mode can not operate the Apple device by itself<br>
     *  Addition to it, this mode can display the HMI of Apple device */
    IPOD_PLAYER_MODE_HMI_CONTROL,
    /*! Mode is unknown */
    IPOD_PLAYER_MODE_UNKNOWN = 0xFF
} IPOD_PLAYER_MODE;


/*!
 * \enum  IPOD_PLAYER_POWER_STATUS
 * This is a enumeration of status of power supply.
 */
typedef enum
{
    /*! Power Status is hibernate */
    IPOD_PLAYER_POWER_HIBERNATE = 0x00,
    /*! Power Status is hibernate deep */
    IPOD_PLAYER_POWER_HIBERNATE_DEEP,
    /*! Power Status is sleep */
    IPOD_PLAYER_POWER_SLEEP,
    /*! Power Status is power on */
    IPOD_PLAYER_POWER_ON,
    /*! Power Status is unknown */
    IPOD_PLAYER_POWER_UNKNOWN = 0xFF
} IPOD_PLAYER_POWER_STATUS;


/*!
 * \struct IPOD_PLAYER_DEVICE_STATUS
 * This is a structure of typeof itunes.<br>
 */
typedef struct
{
    /*! Set/Get bit mask. see \ref DeviceStatusMask<br>
     *  This is a bit mask showing which members of the structure #IPOD_PLAYER_DEVICE_STATUS have valid information */
    U32 deviceStatusMask;
    /*! Current mode of Apple device. */
    IPOD_PLAYER_MODE curMode;
    /*! Current status of Apple device.  */
    IPOD_PLAYER_POWER_STATUS powerStatus;
    /*! Current notification of Apple device.This bits are set \ref AppleDeviceEventMask */
    U64 notifyEvent;
    /*! Running application name of Apple device */
    U8 appName[IPOD_PLAYER_APP_NAME_LEN_MAX];
} IPOD_PLAYER_DEVICE_STATUS;

/*!
 * \enum IPOD_PLAYER_CANCEL_TYPE
 * This is an enumeration of cancel type.<br>
 * This enumeration is used in #iPodPlayerCancel.
 */
typedef enum
{
    /*! Cancel the coverart data acquiring*/
    IPOD_PLAYER_CANCEL_COVERART = 0x00,
    /*! Cancel the database entry acquirering */
    IPOD_PLAYER_CANCEL_DB_ENTRY,
    /*! Cancel type is unknown */
    IPOD_PLAYER_CANCEL_UNKNOWN = 0xFF
} IPOD_PLAYER_CANCEL_TYPE;

/*!
 * \enum IPOD_PLAYER_TAG_TYPE
 * This is an enumeration of song tag type.<br>
 * This enumeration is used in #iPodPlayerSongTag.
 */
typedef enum
{
    /*! Tag is FM radio tag */
    IPOD_PLAYER_TAG_TYPE_FM = 0x00,
    /*! Tag is HD radio tag */
    IPOD_PLAYER_TAG_TYPE_HD,
    /*! Tag is satellite radio tag */
    IPOD_PLAYER_TAG_TYPE_SATELLITE,
    /*! Tag is Unknown */
    IPOD_PLAYER_TAG_TYPE_UNKNOWN
} IPOD_PLAYER_TAG_TYPE;

/*! Length of tag flag is 2 bytes */
#define IPOD_PLAYER_TAG_FLAG_LEN 2
/*! Length of tagged data is 64 bytes */
#define IPOD_PLAYER_TAG_STRING_LEN 256
/*! Length of tagged station URL is 128 bytes */
#define IPOD_PLAYER_TAG_STATION_URL_LEN 256
/*!
 * \struct IPOD_PLAYER_TAG_INFO
 * This is a structure of tag information.<br>
 * It is used in #iPodPlayerSongTag
 */
typedef struct
{
    /*! Ambiguous flag. If it is set to 1, Tag is ambiguous */
    U8 ambiguous[IPOD_PLAYER_TAG_FLAG_LEN];
    /*! If it is set to 1, ambiguous pair was active */
    U8 buttonPressed[IPOD_PLAYER_TAG_FLAG_LEN];
    /*! Name of song */
    U8 name[IPOD_PLAYER_TAG_STRING_LEN];
    /*! Name of arist */
    U8 artist[IPOD_PLAYER_TAG_STRING_LEN];
    /*! Name of album */
    U8 album[IPOD_PLAYER_TAG_STRING_LEN];
    /*! Name of genre */
    U8 genre[IPOD_PLAYER_TAG_STRING_LEN];
    /*! Song identifier */
    U8 songID[IPOD_PLAYER_TAG_STRING_LEN];
    /*! Storefront identifier */
    U8 storefrontID[IPOD_PLAYER_TAG_STRING_LEN];
    /*! Station's requency */
    U8 frequency[IPOD_PLAYER_TAG_STRING_LEN];
    /*! Station call letters */
    U8 callLetter[IPOD_PLAYER_TAG_STRING_LEN];
    /*! Time stamp by ASCII timestamp format */
    U8 timeStamp[IPOD_PLAYER_TAG_STRING_LEN];
    /*! it is used for podcast */
    U8 feedURL[IPOD_PLAYER_TAG_FLAG_LEN];
    /*! If feedURL is set, this is set the broadcast statis URL */
    U8 stationURL[IPOD_PLAYER_TAG_STATION_URL_LEN];
    /*! Status's subchannel */
    U8 programNumber[IPOD_PLAYER_TAG_STRING_LEN];
    /*! software(e.g itunes) affiliate ID */
    U8 affiliateID[IPOD_PLAYER_TAG_STRING_LEN];
    /*! The software station ID */
    U8 softStationID[IPOD_PLAYER_TAG_STRING_LEN];
    /*! unknown data */
    U8 unknownData[IPOD_PLAYER_TAG_STRING_LEN];
}IPOD_PLAYER_TAG_INFO;



/*! Maximum length of device path */
#define IPOD_PLAYER_DETECTION_DEVICE_PATH_LEN_MAX 128
/*! Maximum length of audio path */
#define IPOD_PLAYER_DETECTION_AUDIO_PATH_LEN_MAX 64
/*! Maximum length of MAC address */
#define IPOD_PLAYER_BT_MAC_ADDR_LEN_MAX 6

/*! Maximum length of accessory information string */
#define IPOD_PLAYER_ACC_INFO_STRING_LEN_MAX     64

/*!
 * \enum IPOD_PLAYER_DEVICE_TYPE
 * This is a enumeration of device type .<br>
 */
typedef enum
{
    /*! device type is USB device */
    IPOD_PLAYER_DEVICE_TYPE_USB_DEVICE = 0x00,
    
    /*! device type is USB host */
    IPOD_PLAYER_DEVICE_TYPE_USB_HOST,
    
    /*! device type is BT */
    IPOD_PLAYER_DEVICE_TYPE_BT,
    
    /*! device type is UART */
    IPOD_PLAYER_DEVICE_TYPE_UART,
    
    /*! device type is UNKNOWN */
    IPOD_PLAYER_DEVICE_TYPE_UNKNOWN = 0xFF
} IPOD_PLAYER_DEVICE_TYPE;


/*!
 * \enum IPOD_PLAYER_DETECTION_TYPE
 * This is an enumeration of detection type.<br>
 */
typedef enum
{
    /*! device type is connect */
    IPOD_PLAYER_DETECTION_TYPE_CONNECT = 0x00,
    /*! device type is disconnect */
    IPOD_PLAYER_DETECTION_TYPE_DISCONNECT,
    /*! device type is unknown */
    IPOD_PLAYER_DETECTION_TYPE_UNKNOWN = 0xFF
} IPOD_PLAYER_DETECTION_TYPE;

/*!
 * \struct IPOD_PLAYER_BT_MAC_ADDR
 * This is a structure of Bluetooth MAC address.<br>
 */
typedef struct
{
    /*! Bluetooth MAC address of Accessory */
    U8  addr[IPOD_PLAYER_BT_MAC_ADDR_LEN_MAX];
} IPOD_PLAYER_BT_MAC_ADDR;

/*!
 * \struct IPOD_PLAYER_BT_INFO
 * This is a structure of Bluetooth MAC address infomation.<br>
 */
typedef struct
{
    /*! The number of Bluetooth MAC address */
    U8    macCount;
    /*! See #IPOD_PLAYER_BT_MAC_ADDR */
    IPOD_PLAYER_BT_MAC_ADDR        *macAddr;
} IPOD_PLAYER_DEV_INFO;

/*!
 * \struct IPOD_PLAYER_VEHICLE_INFO
 * This is a structure of vehicle infomation. Only DisplayName supported. <br>
 */
typedef struct
{
    /*! Display name. It is a pointer to NULL terminated UTF-8 string. also It is restricted by 256 bytes. */
    U8 *displayName;
} IPOD_PLAYER_VEHICLE_INFO;

/*!
 * \struct VERSION_NUMBER
 */
typedef struct
{
    U8 Major_Number;
    U8 Minor_Number;
    U8 Revision_Number;
}IPOD_VERSION_NUMBER;

typedef enum
{
    NO_ACTION       = 0,
    OPTIONAL_ACTION = 1,
    NO_ALERT        = 2,
    NO_COMMUNICATION_PROTOCOL = 3
} EAPMatchAction;

/**
 * \struct MFI Message codes
 */
typedef struct
{
    /*! buffer of message code - maximum: 40code(80byte) */
    U16 *msgcodes;
    /*! number of message code */
    U32 msgnum;
} MFI_MSGCODES;

/**
 * \struct iOS App configuration details
 */
typedef struct
{
    /*! iOS App Identifier */
    U8      iOSAppIdentifier;

    /*! iOS App Name */
    U8*     iOSAppName;

    /*! If set iOS App require EA native transport */
    BOOL    EANativeTransport;

    /**
    * \brief It specifies whether device will attempt to find match app.
    *
    * It refers to enum EAPMatchAction
    */
    EAPMatchAction MatchAction;

} IPOD_PLAYER_IOS_APPINFO;

/*!
 * \struct IPOD_PLAYER_ACC_INFO_CONFIGURATION
 * This is a configuration structure of accessory information<br>
 */
typedef struct
{
    /*! Accessory Name - max length is 64 byte (include "NULL" character.) */
    U8 *Name;
    /*! Hardware Version */
    IPOD_VERSION_NUMBER Hardware_version;
    /*! firmware Version */
    IPOD_VERSION_NUMBER Software_version;
    /*! Hardware Version for iAP2 */
    U8 *Hardware_version_iap2;
    /*! firmware Version for iAP2 */
    U8 *Software_version_iap2;
    /*! Manufacturer - max length is 64 byte (include "NULL" character.) */
    U8 *Manufacturer;
    /*! Model Number - max length is 64 byte (include "NULL" character.) */
    U8 *ModelNumber;
    /*! Serial Number - max length is 64 byte (include "NULL" character.) */
    U8 *SerialNumber;
    /*! Vendor ID - max length is 64 byte (include "NULL" character.) Only iAP2 */
    U8 *VendorId;
    /*! Product ID - max length is 64 byte (include "NULL" character.) Only iAP2 */
    U8 *ProductId;
    /*! BCDDevice - max length is 64 byte (include "NULL" character.) Only iAP2 */
    U8 *BCDDevice;
    /*! ProductPlanUUID - max length is 64 byte (include "NULL" character.) Only iAP2 */
    U8 *ProductPlanUUID;
    /*! SupportedIOSInTheCar - Enable carPlay. Only iAP2 */
    U32 SupportedIOSInTheCar;
    /*! Supported iOS Application Count Only iAP2 */
    U32 SupportediOSAppCount;
    /*! iOS configuration details. Only iAP2  */
    IPOD_PLAYER_IOS_APPINFO *iOSAppInfo;
    /*! MFI message codes (sent by accessory) - maximum size is #IPOD_PLAYER_IDEN_MSGCODE_MAX. Only iAP2 */
    MFI_MSGCODES *MsgSentByAcc;
    /*! MFI message codes (receive from device) - maximum size is #IPOD_PLAYER_IDEN_MSGCODE_MAX. Only iAP2 */
    MFI_MSGCODES *MsgRecvFromDevice;
    
} IPOD_PLAYER_ACC_INFO_CONFIGURATION;

/*!
 * \struct IPOD_PLAYER_DEVICE_DETECTION_INFO
 * This is a structure of device detection <br>
 */
typedef struct
{
    /*! See #IPOD_PLAYER_DEVICE_TYPE */
    IPOD_PLAYER_DEVICE_TYPE devType;
    
    /*! See #IPOD_PLAYER_DETECTION_TYPE */
    IPOD_PLAYER_DETECTION_TYPE detectType;
    
    /*! Path of detected device. This puth is used for plugin interface */
    U8 devPath[IPOD_PLAYER_DETECTION_DEVICE_PATH_LEN_MAX];
    
    /*! Path of audio of detected device. This path is specified by "hw:*,*". If this path is set to "\0", iPodPlayer does not do AudioStreaming */
    U8 audioPath[IPOD_PLAYER_DETECTION_AUDIO_PATH_LEN_MAX];
    
    /*! path of end point 0. This puth is used for hostmode interface by gadget driver. (ex."/dev/ffs/ep0") */
    U8 endPointPath[IPOD_PLAYER_STRING_LEN_MAX];
    
    /*! If this pointer is NULL, Bluetooth information is set by IPOD_PLAYER_CFG.xml. See #IPOD_PLAYER_DEV_INFO */
    IPOD_PLAYER_DEV_INFO *devInfo;

    /*! The parameter of accessory information. If this pointer is NULL, it is set by either IPOD_PLAYER_CFG.xml(iAP2) or IPOD_CTRL.cfg(iAP1) */
    IPOD_PLAYER_ACC_INFO_CONFIGURATION  *accInfo;

    /*! The parameter of vehicle information. If this pointer is NULL, it is set by IPOD_PLAYER_CFG.xml. See #IPOD_PLAYER_VEHICLE_INFO */
    IPOD_PLAYER_VEHICLE_INFO *vehicleInfo;
} IPOD_PLAYER_DEVICE_DETECTION_INFO;



/*!
 * \struct IPOD_PLAYER_IOSAPP_INFO
 * This is a structure of iOSApplication information.<br>
 * This structure is used to register the communicable iOS Application in #iPodPlayerInit.
 */
typedef struct
{
    /*! iOSApplication Name. It is NULL terminated UTF-8 strings. also It is restricted by 256 bytes. */
    U8 appName[IPOD_PLAYER_IOSAPP_NAME_LEN_MAX];
    /*! iOSAPplication URL. It is NULL terminated UTF-8 strings. also It is restricted by 256 bytes. */
    U8 appURL[IPOD_PLAYER_IOSAPP_URL_LEN_MAX];
} IPOD_PLAYER_IOSAPP_INFO;

/*!
 * \enum IPOD_PLAYER_AUDIO_SELECT
 * This is an enumeration of audio selection.<br>
 * This enumeration is used in #iPodPlayerSelectAudioOut.
 */
typedef enum
{
    /*! Select Analog audio output */
    IPOD_PLAYER_AUDIO_ANALOG = 0x00,
    /*! Select Digital audio output */
    IPOD_PLAYER_AUDIO_DIGITAL,
    /*! Select Bluetooth A2DP output. Currently this value is not supported */
    IPOD_PLAYER_AUDIO_A2DP
} IPOD_PLAYER_AUDIO_SELECT;

/*!
 * \enum IPOD_PLAYER_REPEAT_STATUS
 * This is an enumeration of repeat status.<br>
 * This enumeration is used in #iPodPlayerSetRepeatStatus or #iPodPlayerGetRepeatStatus.
 */
typedef enum
{
    /*! Repeat Off. If application sets it to repeat status, Apple device will stop the playing by end of track. */
    IPOD_PLAYER_REPEAT_OFF = 0x00,
    /*! Repeat One Track. If application sets it to repeat status, Apple device will repeat the playing by one track. */
    IPOD_PLAYER_REPEAT_ONE,
    /*! Repeat All Tracks. If application set it to repeat status, Apple device will repeat the playing by end of track. */
    IPOD_PLAYER_REPEAT_ALL,
    /*! Repeat status is unknown */
    IPOD_PLAYER_REPEAT_UNKNOWN = 0xFF
} IPOD_PLAYER_REPEAT_STATUS;


/*!
 * \enum IPOD_PLAYER_SHUFFLE_STATUS
 * This is an enumeration of shuffle status.<br>
 */
typedef enum
{
    /*! Shuffle Off. It is used in #iPodPlayerSetShuffleStatus and #iPodPlayerGetShuffleStatus */
    IPOD_PLAYER_SHUFFLE_OFF = 0x00,

    /*! Shuffle with tracks. It is used in #iPodPlayerSetShuffleStatus and #iPodPlayerGetShuffleStatus */
    IPOD_PLAYER_SHUFFLE_TRACKS,

    /*! Shuffle with albums. It is used in #iPodPlayerSetShuffleStatus and #iPodPlayerGetShuffleStatus */
    IPOD_PLAYER_SHUFFLE_ALBUMS,
    
    /*! Shuffle mode is unknown*/
    IPOD_PLAYER_SHUFFLE_STATUS_UNKNOWN = 0xFF

} IPOD_PLAYER_SHUFFLE_STATUS;

/*!
 * \enum IPOD_PLAYER_TRACK_TYPE
 * This is an enumeration of track type.<br>
 */
typedef enum
{
    /*! Track is selected from playback list */
    IPOD_PLAYER_TRACK_TYPE_PLAYBACK = 0x00,
    /*! Track is selected from database list */
    IPOD_PLAYER_TRACK_TYPE_DATABASE,
    /*! Track is selected by unique ID of track */
    IPOD_PLAYER_TRACK_TYPE_UID,
    /*! Track type is unknown */
    IPOD_PLAYER_TRACK_TYPE_UNKNOWN = 0xFF
} IPOD_PLAYER_TRACK_TYPE;


/*!
 * \struct IPOD_PLAYER_INDEX_TIME
 * This is a structure of current track index and current elapsed time.<br>
 */
typedef struct
{
    /*! Index of track */
    U32 index;
    /*! Current elapsed time */
    U32 time;
} IPOD_PLAYER_INDEX_TIME;

/*!
 * \enum IPOD_PLAYER_PLAY_STATUS
 * This is an enumeration of play status.<br>
 */
typedef enum
{
    /*! Current status is play */
    IPOD_PLAYER_PLAY_STATUS_PLAY = 0x00,
    /*! Current status is pause */
    IPOD_PLAYER_PLAY_STATUS_PAUSE,
    /*! Current status is stop */
    IPOD_PLAYER_PLAY_STATUS_STOP,
    /*! Current status is fastforward */
    IPOD_PLAYER_PLAY_STATUS_FF,
    /*! Current status is rewind */
    IPOD_PLAYER_PLAY_STATUS_RW,
    /*! Current status is unknown */
    IPOD_PLAYER_PLAY_STATUS_UNKNOWN
} IPOD_PLAYER_PLAY_STATUS;

/*\{*/
/**
 * \name Playback Status Active Mask Defines
 * \anchor playbackStatusActiveMask
 */
/*! Play status is actived */
#define IPOD_PLAYER_PLAY_ACT_STATUS             0x00000001
/*! Track time is actived */
#define IPOD_PLAYER_PLAY_ACT_TIME               0x00000002
/*! Track index is actived */   
#define IPOD_PLAYER_PLAY_ACT_TRACK_INDEX        0x00000004
/*! Chapter index is actived */   
#define IPOD_PLAYER_PLAY_ACT_CHAPTER_INDEX      0x00000008
/*! Shuffle status is actived */
#define IPOD_PLAYER_PLAY_ACT_SHUFFLE            0x00000010
/*! Repeat status is actived */
#define IPOD_PLAYER_PLAY_ACT_REPEAT             0x00000020
/*! Now Playing app name is actived */
#define IPOD_PLAYER_PLAY_ACT_APP_NAME           0x00000040
/*! Now Playing app bundle ID is actived */
#define IPOD_PLAYER_PLAY_ACT_APP_BUNDLE_ID      0x00000080
/*! Playback queue count is actived */
#define IPOD_PLAYER_PLAY_ACT_QUEUE_COUNT        0x00000100
/*! Playback queue list availability status is actived */
#define IPOD_PLAYER_PLAY_ACT_QUEUE_LIST_AVAIL   0x00000200
/*! Media Library unique identifier is actived */
#define IPOD_PLAYER_PLAY_ACT_MEDIA_LIBRARY_UID  0x00000400
/*! Apple Music Radio station name is actived */
#define IPOD_PLAYER_PLAY_ACT_AMR_STATION_NAME   0x00000800
/*\}*/

/*!
 * \struct IPOD_PLAYER_PLAYBACK_STATUS
 * This is a structure of playback status.<br>
 */
typedef struct
{
    /*! Playback status active event Mask to find active notified status item. See \ref playbackStatusActiveMask */
    U32 playbackStatusActiveMask;
    /*! Enumeration of status. Play, Pause, Stop, FF or RW.*/
    IPOD_PLAYER_PLAY_STATUS status;
    /*! Something change occurred. This value is loop from 0 until 255 */
    U8 playbackCount;
    /*! Structure of track change information.*/
    IPOD_PLAYER_INDEX_TIME track;
    /*! Structure of chapter change information. if track does not have a chapter, this structure must not use. */
    IPOD_PLAYER_INDEX_TIME chapter;
    /*! Structure of shuffle status information. see #IPOD_PLAYER_SHUFFLE_STATUS */
    IPOD_PLAYER_SHUFFLE_STATUS shuffleStatus;
    /*! Structure of repeat status information. see #IPOD_PLAYER_REPEAT_STATUS */
    IPOD_PLAYER_REPEAT_STATUS repeatStatus;
    /*! Now Playing app name infomation. */
    U8 appName[IPOD_PLAYER_APP_NAME_LEN_MAX];
    /*! Now Playing app bundle ID infomation. */
    U8 appBundleID[IPOD_PLAYER_STRING_LEN_MAX];
    /*! Media Library unique identifier. */
    U8 mediaLibraryUID[IPOD_PLAYER_STRING_UID_LEN_MAX];
    /*! Apple Music Radio station name. */
    U8 AmrStationName[IPOD_PLAYER_STRING_LEN_MAX];
    /*! Playback queue count infomation. */
    U32 queueCount;
    /*! Playback queue list availability status infomation. */
    BOOL queueListAvail;
} IPOD_PLAYER_PLAYBACK_STATUS;

/*!
 * \enum IPOD_PLAYER_DEVICE_CONNECT_STATUS
 * This is an enumeration of status of authentication.
 */
typedef enum
{
    /*! Device is disconnected */
    IPOD_PLAYER_DEVICE_CONNECT_STATUS_DISCONNECT = 0x00,
    /*! Device is connected */
    IPOD_PLAYER_DEVICE_CONNECT_STATUS_CONNECT,
    /*! Device is unknown */
    IPOD_PLAYER_DEVICE_CONNECT_STATUS_UNKNOWN = 0xFF
} IPOD_PLAYER_DEVICE_CONNECT_STATUS;

/*!
 * \enum IPOD_PLAYER_AUTHENTICATION_STATUS
 * This is an enumeration of status of authentication.
 */
typedef enum
{
    /*! Authentication Status is fail */
    IPOD_PLAYER_AUTHENTICATION_FAIL = 0x00,
    /*! Authentication Status is running */
    IPOD_PLAYER_AUTHENTICATION_RUNNING,
    /*! Authentication Status is success */
    IPOD_PLAYER_AUTHENTICATION_SUCCESS,
    IPOD_PLAYER_AUTHENTICATION_IAP1_RESTART,
    /*! Authentication Status is unknown */
    IPOD_PLAYER_AUTHENTICATION_UNKNOWN = 0xFF
} IPOD_PLAYER_AUTHENTICATION_STATUS;

/*!
 * \enum IPOD_PLAYER_IAP_PROTOCOL_TYPE
 * This is an enumeration of type of ipod accessory protocols.
 */
typedef enum
{
    /*! ipod accessory protocol is type 1 */
    IPOD_PLAYER_PROTOCOL_TYPE_IAP1 = 0x1,
    /*! ipod accessory protocol is type 2 */
    IPOD_PLAYER_PROTOCOL_TYPE_IAP2 = 0x2,
    /*! ipod accessory protocol is unknown */
    IPOD_PLAYER_PROTOCOL_TYPE_UNKNOWN = 0xFF
} IPOD_PLAYER_IAP_PROTOCOL_TYPE;

/*!
 * \struct IPOD_PLAYER_CONNECTION_STATUS
 * This is a structure of connection status.<br>
 */
typedef struct
{
    /*! Reference to #IPOD_PLAYER_DEVICE_CONNECT_STATUS */
    IPOD_PLAYER_DEVICE_CONNECT_STATUS deviceStatus;
    /*! Reference to #IPOD_PLAYER_AUTHENTICATION_STATUS */
    IPOD_PLAYER_AUTHENTICATION_STATUS authStatus;
    /*! Reference to #IPOD_PLAYER_POWER_STATUS */
    IPOD_PLAYER_POWER_STATUS powerStatus;
    /*! Reference to #IPOD_PLAYER_DEVICE_TYPE */
    IPOD_PLAYER_DEVICE_TYPE deviceType;
    /*! Reference to #IPOD_PLAYER_IAP_PROTOCOL_TYPE */
    IPOD_PLAYER_IAP_PROTOCOL_TYPE iapType;
} IPOD_PLAYER_CONNECTION_STATUS;

/*!
 * \enum IPOD_PLAYER_GPS_TYPE
 * This is an enumeration of GPS type.<br>
 */
typedef enum
{
    /*! GPS type is NMEA. If type is this, data may include GPGGA, GPRMC and so on. */
    IPOD_PLAYER_GPS_TYPE_NMEA = 0x00,
    /*! GPS type is ephemeris */
    IPOD_PLAYER_GPS_TYPE_EPHEMERIS
} IPOD_PLAYER_GPS_TYPE;

/*!
 * \struct IPOD_PLAYER_GPS_TIME
 * This is a structure of GPS time.<br>
 */
typedef struct
{
    /*! GPS week number. It is defined by IS-GPS-200 PIRN-002. */
    U16 week;
    /*! GPS time of week. */
    U32 time;
} IPOD_PLAYER_GPS_TIME;

/*!
 * \struct IPOD_PLAYER_GPS_ANGLE
 * This is a structure of GPS angle.<br>
 */
typedef struct
{
    /*! GPS latitude. Maximum size is 180, Minimum size is -179. */
    S32 latitude;
    /*! GPS longtitude. Maximum size is 180, Minumu size is -179 */
    S32 longtitude;
    /*! GPS radius.*/
    U16 radius;
} IPOD_PLAYER_GPS_ANGLE;

/*!
 * \enum IPOD_PLAYER_HMI_STATUS_TYPE
 * This is an enumeration of status typein HMI control mode.<br>
 */
typedef enum
{
    /*! Current track playing time in milliseconds. */
    IPOD_PLAYER_HMI_STATUS_TYPE_CURRENT_TIME = 0x00,
    /*! Current playing track Index */
    IPOD_PLAYER_HMI_STATUS_TYPE_TRACK_INDEX,
    /*! Current playing chapter index */
    IPOD_PLAYER_HMI_STATUS_TYPE_CHAPTER_INDEX,
    /*! Chapter total number that current playing track has */
    IPOD_PLAYER_HMI_STATUS_TYPE_CHAPTER_COUNT,
    /*! Current playback status. If this type is acquired, Result value is same as #IPOD_PLAYER_PLAY_STATUS. */
    IPOD_PLAYER_HMI_STATUS_TYPE_PLAYBACK_STATUS,
    /*! Current mute status. 0 means mute is off, 1 is mute on.*/
    IPOD_PLAYER_HMI_STATUS_TYPE_MUTE_STATUS,
    /*! Current volume status. This value is between 0 and 255. 0 means minimum volume, 255 means maximum volume. */
    IPOD_PLAYER_HMI_STATUS_TYPE_VOLUME_STATUS,
    /*! Current equalizer setting. It is indicated only equalizer index number. */
    IPOD_PLAYER_HMI_STATUS_TYPE_EQUALIZER_STATUS,
    /*! Current shuffle status. If this type is acquired, Result value is same as #IPOD_PLAYER_SHUFFLE_STATUS. */
    IPOD_PLAYER_HMI_STATUS_TYPE_SHUFFLE_STATUS,
    /*! Current repeat status. If this type is acquired, Result value is same as #IPOD_PLAYER_REPEAT_STATUS */
    IPOD_PLAYER_HMI_STATUS_TYPE_REPEAT_STATUS,
    /*! Current date.  */
    IPOD_PLAYER_HMI_STATUS_TYPE_CURRENT_DATE,
    /*! Current backlight level of Apple device. This value is between 0 and 255. 0 means minumum brightness, 255 meens full intensity. */
    IPOD_PLAYER_HMI_STATUS_TYPE_BACKLIGHT_STATUS,
    /*! Current sound check status. This value is 0 means sound check off, 1 means sound check on.*/
    IPOD_PLAYER_HMI_STATUS_TYPE_SOUND_CHECK,
    /*! Current speed status. If this type is acquired, Result value is same as #IPOD_PLAYER_PLAYING_SPEED. */
    IPOD_PLAYER_HMI_STATUS_TYPE_SPEED_STATUS,
    /*! Current absolue volume status. This value is between 0 and 255. 0 means minimum volume, 255 means maximum volume. */
    IPOD_PLAYER_HMI_STATUS_TYPE_ABSOLUTE_VOLUME,
    /*! Current track capabilities. If this type is set, Return value is same as \ref TrackCapabilityMask */
    IPOD_PLAYER_HMI_STATUS_TYPE_CAPABILITIES,
    /*! HMI status type is unknown */
    IPOD_PLAYER_HMI_STATUS_TYPE_UNKNOWN = 0xFF
} IPOD_PLAYER_HMI_STATUS_TYPE;


/*!
 * \struct IPOD_PLAYER_HMI_STATUS_DATE
 * This is a structure of current date .<br>
 */
typedef struct
{
    /*! Current year */
    U16 year;
    /*! Current month */
    U8 month;
    /*! Current day */
    U8 day;
    /*! Current hour */
    U8 hour;
    /*! Current minute */
    U8 minute;
} IPOD_PLAYER_HMI_STATUS_DATE;
/*\}*/

/*!
 * \enum IPOD_PLAYER_DEVICE_EVENT_TYPE
 * This is an enumeration of device event type.<br>
 */
typedef enum
{
    /*! Event type is song tag */
    IPOD_PLAYER_DEVICE_EVENT_TYPE_TAG = 0x00,
    /*! Event type is camera */
    IPOD_PLAYER_DEVICE_EVENT_TYPE_CAMERA,
    /*! Event type is chaging information */
    IPOD_PLAYER_DEVICE_EVENT_TYPE_CHARGING,
    /*! Event type is Local Device media library database changing */
    IPOD_PLAYER_DEVICE_EVENT_TYPE_DATABASE,
    /*! Event type is iOS Application name */
    IPOD_PLAYER_DEVICE_EVENT_TYPE_IOSAPP,
    /*! Event type is out mode */
    IPOD_PLAYER_DEVICE_EVENT_TYPE_OUT,
    /*! Event type is bluetooth */
    IPOD_PLAYER_DEVICE_EVENT_TYPE_BT,
    /*! Event type is iOS Application full name */
    IPOD_PLAYER_DEVICE_EVENT_TYPE_IOSAPPFULL,
    /*! Event type is assistive touch */
    IPOD_PLAYER_DEVICE_EVENT_TYPE_ASSISTIVE,
    /*! Event type is power updates */
    IPOD_PLAYER_DEVICE_EVENT_TYPE_POWER,
    /*! Event type is sample rate */
    IPOD_PLAYER_DEVICE_EVENT_TYPE_SAMPLE_RATE,
    /*! Event type is store Local Device media library database */
    IPOD_PLAYER_DEVICE_EVENT_TYPE_STORE_DB,
    /*! Event type is update of playback list */
    IPOD_PLAYER_DEVICE_EVENT_TYPE_UPDATE_PBLIST,
    /*! Event type is Apple Music Radio media library database changing */
    IPOD_PLAYER_DEVICE_EVENT_TYPE_DATABASE_AMR,
    /*! Event type is store Apple Music Radio media library database*/
    IPOD_PLAYER_DEVICE_EVENT_TYPE_STORE_DB_AMR,
    /*! Event type is unknown */
    IPOD_PLAYER_DEVICE_EVENT_TYPE_UNKNOWN = 0xFF
} IPOD_PLAYER_DEVICE_EVENT_TYPE;

/*!
 * \enum IPOD_PLAYER_DEVICE_EVENT_TAG_STATUS
 * This is an enumeration of tag status.<br>
 */
typedef enum
{
    /*! Tag operation is completely successful */
    IPOD_PLAYER_DEVICE_EVENT_TAG_STATUS_SUCCESS = 0x00,
    /*! Tag operation is fail */
    IPOD_PLAYER_DEVICE_EVENT_TAG_STATUS_FAIL,
    /*! Tag opearation is unknow  */
    IPOD_PLAYER_DEVICE_EVENT_TAG_STATUS_UNKNOWN = 0xFF
} IPOD_PLAYER_DEVICE_EVENT_TAG_STATUS;

/*!
 * \enum IPOD_PLAYER_DEVICE_EVENT_CAMERA_STATUS
 * This is an enumeration of camera status type.<br>
 */
typedef enum
{
    /*! Currently camera is off */
    IPOD_PLAYER_DEVICE_EVENT_CAMERA_STATUS_OFF = 0x00,
    /*! Currently user uses camera preview */
    IPOD_PLAYER_DEVICE_EVENT_CAMERA_STATUS_PREVIEW,
    /*! Currently user uses camera to record */
    IPOD_PLAYER_DEVICE_EVENT_CAMERA_STATUS_RECORDING,
    /*! Camera event is unknown */
    IPOD_PLAYER_DEVICE_EVENT_CAMERA_STATUS_UNKNOWN = 0xFF
} IPOD_PLAYER_DEVICE_EVENT_CAMERA_STATUS;

/*!
 * \enum IPOD_PLAYER_DEVICE_EVENT_CHARGING_TYPE
 * This is an enumeration of chaging type.<br>
 */
typedef enum
{
    /*! Charging type is currently available charging */
    IPOD_PLAYER_DEVICE_EVENT_CHARGING_TYPE_CUR = 0x00,
    /*! Charging type is unknown */
    IPOD_PLAYER_DEVICE_EVENT_CHARGING_TYPE_UNKNOWN = 0xFF
} IPOD_PLAYER_DEVICE_EVENT_CHARGING_TYPE;


/*!
 * \enum IPOD_PLAYER_DEVICE_EVENT_OUT_STATUS
 * This is an enumeration of out mode status.<br>
 */
typedef enum
{
    /*! Out mode is inactive */
    IPOD_PLAYER_DEVICE_EVENT_OUT_STATUS_INACTIVE = 0x00,
    /*! Out mode is active */
    IPOD_PLAYER_DEVICE_EVENT_OUT_STATUS_ACTIVE,
    /*! Out mode is unknown */
    IPOD_PLAYER_DEVICE_EVENT_OUT_STATUS_UNKNOWN
} IPOD_PLAYER_DEVICE_EVENT_OUT_STATUS;

/*!
 * \enum IPOD_PLAYER_DEVICE_EVENT_ASSISTIVE_STATUS
 * This is an enumeration of assistive status.<br>
 */
typedef enum
{
    /*! Assistive touch functionality is off */
    IPOD_PLAYER_DEVICE_EVENT_ASSISTIVE_STATUS_OFF = 0x00,
    /*! Assistive touch functionality is on */
    IPOD_PLAYER_DEVICE_EVENT_ASSISTIVE_STATUS_ON,
    /*! Assistive touch functionality is unknown */
    IPOD_PLAYER_DEVICE_EVENT_ASSISTIVE_STATUS_UNKNOWN
} IPOD_PLAYER_DEVICE_EVENT_ASSISTIVE_STATUS;
/*!
 * \struct IPOD_PLAYER_DEVICE_EVENT_TAG
 * This is a structure of tag event information.<br>
 * If type of #IPOD_PLAYER_CB_NOTIFY_DEVICE_EVENT is #IPOD_PLAYER_DEVICE_EVENT_TYPE_TAG, this structure shall be used.
 */
typedef struct
{
    /*! Current tag event status */
    IPOD_PLAYER_DEVICE_EVENT_TAG_STATUS status;
} IPOD_PLAYER_DEVICE_EVENT_TAG;

/*!
 * \struct IPOD_PLAYER_DEVICE_EVENT_CAMERA
 * This is a structure of camera event information .<br>
 * If type of #IPOD_PLAYER_CB_NOTIFY_DEVICE_EVENT is #IPOD_PLAYER_DEVICE_EVENT_TYPE_CAMERA, this structure shall be used.
 */
typedef struct
{
    /*! Current camera event status */
    IPOD_PLAYER_DEVICE_EVENT_CAMERA_STATUS status;
} IPOD_PLAYER_DEVICE_EVENT_CAMERA;

/*!
 * \struct IPOD_PLAYER_DEVICE_EVENT_CHARGING
 * This is a structure of chaging information .<br>
 * If type of #IPOD_PLAYER_CB_NOTIFY_DEVICE_EVENT is #IPOD_PLAYER_DEVICE_EVENT_TYPE_CHARGING, this structure shall be used.
 */
typedef struct
{
    /*! Current charging information type */
    IPOD_PLAYER_DEVICE_EVENT_CHARGING_TYPE type;
    /*! Current charging information staus */
    U32 status;
} IPOD_PLAYER_DEVICE_EVENT_CHARGING;

/*!
 * \struct IPOD_PLAYER_DEVICE_EVENT_PLAYING_IOSAPP
 * This is a structure of playing iOS Application information .<br>
 * If type of #IPOD_PLAYER_CB_NOTIFY_DEVICE_EVENT is #IPOD_PLAYER_DEVICE_EVENT_TYPE_IOSAPP, this structure shall be used.
 */
typedef struct
{
    /*! iOS Application name that current playing in Apple device */
    U8 appName[IPOD_PLAYER_APP_NAME_LEN_MAX];
} IPOD_PLAYER_DEVICE_EVENT_PLAYING_IOSAPP;

/*!
 * \struct IPOD_PLAYER_DEVICE_EVENT_OUT
 * This is a structure of out mode  information.<br>
 * If type of #IPOD_PLAYER_CB_NOTIFY_DEVICE_EVENT is #IPOD_PLAYER_DEVICE_EVENT_TYPE_OUT, this structure shall be used.
 */
typedef struct
{
    /*! Currently out mode status */
    IPOD_PLAYER_DEVICE_EVENT_OUT_STATUS status;
} IPOD_PLAYER_DEVICE_EVENT_OUT;

/*!
 * \struct IPOD_PLAYER_DEVICE_EVENT_BT
 * This is a structure of Bluetooth information.<br>
 * If type of #IPOD_PLAYER_CB_NOTIFY_DEVICE_EVENT is #IPOD_PLAYER_DEVICE_EVENT_TYPE_BT, this structure shall be used.
 */
typedef struct
{
    /*! Mac address of currently connected Bluetooth device */
    U8 macAddr[IPOD_PLAYER_BT_MAC_ADDR_LEN_MAX];
    /*! Currently connected profile list mask. Please see \ref profileList */
    /*! If bit is set, indicated profile connection is established. */
    U64 profileList;
} IPOD_PLAYER_DEVICE_EVENT_BT;

/*!
 * \struct IPOD_PLAYER_DEVICE_EVENT_IOSAPP_FULL
 * This is a structure of iOS Application full name information.<br>
 * If type of #IPOD_PLAYER_CB_NOTIFY_DEVICE_EVENT is #IPOD_PLAYER_DEVICE_EVENT_TYPE_IOSAPPFULL, this structure shall be used.
 */
typedef struct
{
    /*! Currently Playing iOS Application full name. Full name is a name which is displayed in Apple device's display*/
    U8 fullName[IPOD_PLAYER_IOS_FULL_NAME_LEN_MAX];
} IPOD_PLAYER_DEVICE_EVENT_IOSAPP_FULL;

/*!
 * \struct IPOD_PLAYER_DEVICE_EVENT_ASSISTIVE
 * This is a structure of assistive touch information.<br>
 * If type of #IPOD_PLAYER_CB_NOTIFY_DEVICE_EVENT is #IPOD_PLAYER_DEVICE_EVENT_TYPE_ASSISTIVE, this structure shall be used.
 */
typedef struct
{
    /*! Currently assistive touch functionality status */
    IPOD_PLAYER_DEVICE_EVENT_ASSISTIVE_STATUS status;
} IPOD_PLAYER_DEVICE_EVENT_ASSISTIVE;

/**
 * \enum IPOD_PLAYER_DEVICE_EVENT_MEDIA_LIB_STATUS
 * This is an enumeration of media library status.<br>
 */
typedef enum
{
    /*! media library item update */
    IPOD_PLAYER_MEDIA_ITEM_UPDATE =     0x0001,
    /*! media library playlist update */
    IPOD_PLAYER_MEDIA_PLAYLIST_UPDATE = 0x0002,
    /*! media library item delete */
    IPOD_PLAYER_MEDIA_ITEM_DELETE =     0x0010,
    /*! media library playlist delete */
    IPOD_PLAYER_MEDIA_PLAYLIST_DELETE = 0x0020,
    /*! media library reset */
    IPOD_PLAYER_MEDIA_RESET =           0x0100,
    /*! media library playlist delete */
    IPOD_PLAYER_MEDIA_UPDATE_PROGRESS = 0x1000
} IPOD_PLAYER_DEVICE_EVENT_MEDIA_LIB_STATUS;

/*!
 * \struct IPOD_PLAYER_MEDIA_LIB_INFO
 * This is a information structure of media library update.<br>
 * If type of #IPOD_PLAYER_CB_NOTIFY_DEVICE_EVENT is #IPOD_PLAYER_DEVICE_EVENT_TYPE_DATABASE, this structure shall be used.
 */
typedef struct
{
    /*! Media library update status */
    IPOD_PLAYER_DEVICE_EVENT_MEDIA_LIB_STATUS status;
    /*! Media library update progress */
    U16 progress;
} IPOD_PLAYER_MEDIA_LIB_INFO;

/*!
 * \union IPOD_PLAYER_DEVICE_EVENT_INFO
 * This is an union of Apple device's event information.<br>
 */
typedef union
{
    /*! Tag event data. Please see #IPOD_PLAYER_DEVICE_EVENT_TAG */
    IPOD_PLAYER_DEVICE_EVENT_TAG                tagEvent;
    /*! Camera event data. Please see #IPOD_PLAYER_DEVICE_EVENT_CAMERA */
    IPOD_PLAYER_DEVICE_EVENT_CAMERA             cameraEvent;
    /*! Chaging event data. Please see #IPOD_PLAYER_DEVICE_EVENT_CHARGING */
    IPOD_PLAYER_DEVICE_EVENT_CHARGING           chargingEvent;
    /*! iOS Application name event data. Please see #IPOD_PLAYER_DEVICE_EVENT_PLAYING_IOSAPP */
    IPOD_PLAYER_DEVICE_EVENT_PLAYING_IOSAPP     playingiOSAppEvent;
    /*! Out mode event data. Please see #IPOD_PLAYER_DEVICE_EVENT_OUT */
    IPOD_PLAYER_DEVICE_EVENT_OUT                outEvent;
    /*! Bluetooth event data. Please see #IPOD_PLAYER_DEVICE_EVENT_BT */
    IPOD_PLAYER_DEVICE_EVENT_BT                 btEvent;
    /*! iOS Application full name event data. Please see #IPOD_PLAYER_DEVICE_EVENT_IOSAPP_FULL */
    IPOD_PLAYER_DEVICE_EVENT_IOSAPP_FULL        iOSAppFullEvent;
    /*! Assistive touch event data. Please see #IPOD_PLAYER_DEVICE_EVENT_ASSISTIVE */
    IPOD_PLAYER_DEVICE_EVENT_ASSISTIVE          assitiveEvent;
    /*! Media library update data(at DATABASE update). Please see #IPOD_PLAYER_MEDIA_LIB_INFO */
    IPOD_PLAYER_MEDIA_LIB_INFO                  info;
    /*! Power updates notification. Please see #IPOD_PLAYER_POWER_NOTIFY */
    IPOD_PLAYER_POWER_NOTIFY                    powerNotify;
    /*! Update Sample rate for device mode audio */
    U32                                         sampleRate;
} IPOD_PLAYER_DEVICE_EVENT_INFO;

/*\}*/


#endif
