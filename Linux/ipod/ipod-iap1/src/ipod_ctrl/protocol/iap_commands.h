/* -----------------------------------------------------------------------------
 * An invalid character is automatically inserted by cvs when the file is 
 * commited. We can not do anything about it.
 * -----------------------------------------------------------------------------
 */ 
/*
* \file: iap_commands.h
*
* \version: $Id: iap_commands.h,v 1.36 2012/01/17 14:20:45 kgerhards Exp $
*
* \component: ADIT iPod connector
*
* \author: Lars Lendeckel
*
* \copyright: (c) 2003-2007 ADIT Corporation
*
*
* \history
* $Log: iap_commands.h,v $
* Revision 1.36  2012/01/17 14:20:45  kgerhards
* SFEIIX-79 (SFEIIX-78) Seek-to functionality via IAP needs to be supported
*
* Revision 1.35  2011/12/21 13:21:12  kgerhards
* SFEIIX-74 [b_MAIN] Support new Apple Spec R44 (ATS 2.2)
*
* Revision 1.31.16.1  2011/09/15 11:32:54  serhard
* SWGIIX-1472: put define for extern c in each header file
*
* Revision 1.31  2011/03/15 10:44:28  mshibata
* Marged the linux and t-kernel code.
* Fixed JIRA[SWGII-4742].
* Fixed JIRA[SWGII-4800].
* Fixed JIRA[SWGII-4801].
* Fixed JIRA[SWGII-4803].
* Fixed JIRA[SWGII-4802].
* Fixed JIRA[SWGII-4851].
*
* Revision 1.30  2010/12/21 07:48:23  mshibata
* Fixed the JIRA[SWGII-4227].
* iPod out support.
*
* Revision 1.29  2010/12/20 04:57:38  mshibata
* Fixed for JIRA[SWGII-4227] and JIRA[SWGII-4231].
* also it merged with linux code.
*
* Revision 1.28  2010/12/06 05:30:56  mshibata
* Implemented iPodOut Lingo commands.
* also implemented scrennInfo token in IDPS proess.
*
* Revision 1.27  2010/10/28 07:51:02  mshibata
* Marging the code of Bluetooth support.
*
* Revision 1.26  2010/08/31 02:34:43  mshibata
* Fixed JIRA[SWGII-3516].
*
* Revision 1.25  2010/08/27 05:39:31  mshibata
* Fixed the new feature(iPodAccStatusNotification, iPodSetAccStatusNotification and so on)
*
* Revision 1.24  2010/08/19 08:08:17  mshibata
* Add the new function for communicate iPhone application.
*
* Revision 1.23  2010/08/04 10:28:37  mshibata
* Code improve
*
* Revision 1.22  2010/07/27 06:49:46  mshibata
* Fixed for QAC, Lint
*
* Revision 1.21  2010/06/01 02:50:34  mshibata
* Fix for QAC, Lint.
* Remove deactivation
*
* Revision 1.20  2010/03/09 08:26:37  mshibata
* Code improvement
*
* Revision 1.18  2010/02/08 01:45:57  mshibata
* Fix for QAC, Lint.
* Implement iPodAuthenticateiPod
*
* Revision 1.17  2010/02/04 09:18:48  mshibata
* Fix the bug about identify, iPodGetRepeatMode, SetFIDTokenValue
*
* Revision 1.16  2010/01/26 12:08:36  mshibata
* Fix the iPodGetArtworkData
*
* Revision 1.7  2009/11/13 03:10:53  mshibata
* Fix the JIRA[SWGII-1601], [SWGII-1743]
*
* Revision 1.6  2009/10/13 04:45:35  mshibata
* Fixed for clucible
*
* Revision 1.5  2009/08/24 08:57:16  mshibata
* Fix for QAC,Lint
*
* Revision 1.2  2009/06/29 09:05:19  mshibata
* Add new API
*
* Revision 1.1  2009/06/18 05:54:56  kgerhards
* First commit of Gen2 code.
*
* Revision 1.16  2009/03/23 13:58:33  jlorenz
* SFE-205: Shuffle mode shall persists after disconnecting.
*
* Revision 1.15  2007/12/20 10:48:59  rwillig
* support for usb full speed implemented - SWG-20902
*
* Revision 1.14  2007/12/11 10:27:23  kgerhards
* Added ability to browser Video content on iPod
*
* Revision 1.13  2007/11/07 12:53:26  rwillig
* define IPOD_SEND_ACC_ACK_CMD and define IPOD_GET_ACC_SAMPLE_RATE_CAPS_CMD added
*
* Revision 1.12  2007/11/02 12:34:51  kgerhards
* added check for define IPOD_PERSISTENT_SETTINGS to fix SWG-21007, when define is set, shuffle and repeat settings are persistent, otherwise not
*
* Revision 1.11  2007/11/01 13:19:32  kgerhards
* fixed SWG-21040 by adding handling of iPod request for supported lingoe version
*
* Revision 1.10  2007/10/23 07:52:16  kgerhards
* added two new functions to stop / resume USB audio streaming by request of DN
*
* Revision 1.9  2007/10/17 08:17:46  rwillig
* bugfix for iPod 3rd G
*
* Revision 1.8  2007/10/17 06:14:35  rwillig
* shuffle and repeat, differently for iPod 3rd generation SWG-20662
*
* Revision 1.7  2007/10/15 12:04:57  rwillig
* warnings removed
*
* Revision 1.6  2007/08/22 14:44:48  kgerhards
* changed behaviour of SetRepeat and SetShuffle to be persistent after disconnect (by request of DN)
*
* SWG-20511
*
* Revision 1.5  2007/08/20 15:07:40  kgerhards
* added option for backward compatibility to iPod 3rd generation
*
* Revision 1.4  2007/08/14 14:49:06  kgerhards
* changes and fixes for stresstests
*
* Revision 1.3  2007/06/27 13:01:38  rwillig
* tabs by blanks replaced
*
*
***************************************************************************** */

#ifndef IAP_COMMANDS_H
#define IAP_COMMANDS_H

#ifdef __cplusplus
extern "C" {
#endif

/* general iPod commands ==================================================== */
/* Change allwas disable Digital Audio Lingo and Simple Remoote Lingo Feb/5/2010 */
#define IPOD_IDENTIFY_DEVICE_LINGO_CMD      0x55, 0x0E, 0x00, 0x13, 0x00, 0x00, 0x00, 0x11, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x02, 0x00

#define IPOD_IDENTIFY_GENERAL_ONLY_CMD      0x55, 0x0E, 0x00, 0x13, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00

#define IPOD_GET_LINGO_PROTOCOL_VERSION             0x55, 0x03, 0x00, 0x0F, 0x00
#define IPOD_ENTER_EXTENDED_INTERFACE_MODE_CMD      0x55, 0x02, 0x00, 0x05
#define IPOD_ENTER_SIMPLE_INTERFACE_MODE_CMD        0x55, 0x02, 0x00, 0x06
#define IPOD_GET_REMOTE_UI_MODE_CMD                 0x55, 0x02, 0x00, 0x03
#define IPOD_GET_MODEL_NUMBER_CMD                   0x55, 0x02, 0x00, 0x0D
#define IPOD_GET_SERIAL_NUMBER_CMD                  0x55, 0x02, 0x00, 0x0B
#define IPOD_GET_SOFTWARE_VERSION_CMD               0x55, 0x02, 0x00, 0x09
#define IPOD_GET_NAME_CMD                           0x55, 0x02, 0x00, 0x07
#define IPOD_RET_ACCESSORY_INFO_TYPE0_CMD           0x55, 0x07, 0x00, 0x28, 0x00, 0x00, 0x00, 0x00, 0x00
#define IPOD_RET_ACCESSORY_INFO_TYPE2_CMD           0x55, 0x0A, 0x00, 0x28, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 
#define IPOD_RET_ACCESSORY_INFO_TYPE45_CMD          0x55, 0x06, 0x00, 0x28, 0x00, 0x00, 0x00, 0x00
#define IPOD_RET_ACCESSORY_INFO_TYPE1678_CMD        0x55, 0x00, 0x00, 0x28, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
#define IPOD_RET_ACCESSORY_INFO_TYPE9_CMD           0x55, 0x05, 0x00, 0x28, 0x09, 0x00, 0x00
/* New Feature 04/08/2010 */
#define IPOD_RET_ACCESSORY_INFO_TYPEB_CMD           0x55, 0x07, 0x00, 0x28, 0x0B, 0x00, 0x00, 0x00, 0x00
#define IPOD_GET_AUTH_INFO_CMD          0x55, 0x02, 0x00, 0x1A
#define IPOD_ACK_IPOD_AUTH_INFO_CMD     0x55, 0x03, 0x00, 0x1C, 0x00
#define IPOD_ACK_IPOD_AUTH_SIG_CMD      0x55, 0x03, 0x00, 0x1F, 0x00
#define IPOD_GET_IPOD_OPTIONS_CMD       0x55, 0x02, 0x00, 0x24
#define IPOD_GET_IPOD_PREFERENCES_CMD   0x55, 0x03, 0x00, 0x29, 0x00
#define IPOD_SET_IPOD_PREFERENCES_CMD   0x55, 0x05, 0x00, 0x2B, 0x00, 0x00, 0x00

/* IDPS commands */
#define IPOD_START_IDPS_CMD             0x55, 0x02, 0x00, 0x38
#define IPOD_SET_FID_TOKEN_IDENTIFY     0x10, 0x00, 0x00, 0x05, 0x00, 0x02, 0x04, 0x0A, 0x0C, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x02
#define IPOD_END_IDPS_CMD               0x55, 0x03, 0x00, 0x3B, 0x00, 0x00

/* New Feature 2/1/2010 */
#define IPOD_SET_EVENT_NOTIFICATION         0x55, 0x0A, 0x00, 0x49, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
#define IPOD_GET_IPOD_OPTIONS_FOR_LINGO     0x55, 0x03, 0x00, 0x4B, 0x00
#define IPOD_GET_EVENT_NOTIFICATION         0x55, 0x02, 0x00, 0x4D
#define IPOD_SUPPORTED_EVENT_NOTIFICATION   0x55, 0x02, 0x00, 0x4F

/* New Feature 28/7/2010 */
#define IPOD_SESSION_DEV_ACK                0x55, 0x04, 0x00, 0x41, 0x00, 0x00
#define IPOD_CANCEL_COMMAND                 0x55, 0x07, 0x00, 0x50, 0x00, 0x00, 0x00, 0x00, 0x00
#define IPOD_GET_NOW_PLAYING_FOCUS_APP      0x55, 0x02, 0x00, 0x65
#define IPOD_REQ_TRANSPORT_MAX_PACKET_SIZE  0x55, 0x02, 0x00, 0x11
#define IPOD_RET_ACC_STATUS_NOTIFICATION    0x55, 0x06, 0x00, 0x47, 0x00, 0x00, 0x00, 0x00

/* New Feature 2/12/2010 */
#define IPOD_GETUI_MODE         0x55, 0x02, 0x00, 0x35
#define IPOD_SETUI_MODE         0x55, 0x03, 0x00, 0x37, 0x00
/* simple remote related commands =========================================== */
#define IPOD_PLAY_CMD                               0x55, 0x03, 0x02, 0x00, 0x01
#define IPOD_NEXT_TITLE_CMD                         0x55, 0x03, 0x02, 0x00, 0x08
#define IPOD_PREV_TITLE_CMD                         0x55, 0x03, 0x02, 0x00, 0x10
#define IPOD_NEXT_ALBUM_CMD                         0x55, 0x03, 0x02, 0x00, 0x20
#define IPOD_PREV_ALBUM_CMD                         0x55, 0x03, 0x02, 0x00, 0x40
#define IPOD_VOLUME_UP_CMD                          0x55, 0x03, 0x02, 0x00, 0x02
#define IPOD_VOLUME_DOWN_CMD                        0x55, 0x03, 0x02, 0x00, 0x04
#define IPOD_BUTTON_RELEASE_CMD                     0x55, 0x03, 0x02, 0x00, 0x00
#define IPOD_OUT_BUTTON_STATUS_CMD                  0x55, 0x00, 0x02, 0x0B, 0x00, 0x00, 0x00, 0x00, 0x00
#define IPOD_ROTATION_INPUT_STATUS_CMD              0x55, 0x0F, 0x02, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00

/* display remote related commands =========================================== */
#define IPOD_SET_IPOD_STATE_INFO                0x55, 0x07, 0x03, 0x0E, 0x00, 0x00, 0x00, 0x00, 0x00
#define IPOD_GET_CURRENT_EQ_INDEX               0x55, 0x02, 0x03, 0x01
#define IPOD_SET_CURRENT_EQ_INDEX               0x55, 0x07, 0x03, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00
#define IPOD_GET_NUM_EQ_PROFILES                0x55, 0x02, 0x03, 0x04
#define IPOD_GET_INDEXED_EQ_NAME                0x55, 0x06, 0x03, 0x06, 0x00, 0x00, 0x00, 0x00
#define IPOD_GET_DR_ARTWORK_FORMATS_CMD         0x55, 0x02, 0x03, 0x16
#define IPOD_GET_DR_TRACK_ARTWORK_DATA_CMD      0x55, 0x0C, 0x03, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
#define IPOD_GET_DR_TRACK_ARTWORK_TIMES_CMD     0x55, 0x0C, 0x03, 0x1F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
#define IPOD_GET_DR_PLAY_STATUS                 0x55, 0x02, 0x03, 0x0F, 0x00
#define IPOD_SET_REMOTE_EVENT_NOTIFICATION      0x55, 0x06, 0x03, 0x08, 0x00, 0x00, 0x00, 0x00

/* playback related commands ===================================================== */
#define IPOD_PLAY_TOGGLE_CMD                        0x55, 0x04, 0x04, 0x00, 0x29, 0x01
#define IPOD_PLAY_STOP_CMD                          0x55, 0x04, 0x04, 0x00, 0x29, 0x02
#define IPOD_PLAY_NEXT_TRACK_CMD                    0x55, 0x04, 0x04, 0x00, 0x29, 0x03
#define IPOD_PLAY_PREV_TRACK_CMD                    0x55, 0x04, 0x04, 0x00, 0x29, 0x04
#define IPOD_PLAY_FFWD_CMD                          0x55, 0x04, 0x04, 0x00, 0x29, 0x05
#define IPOD_PLAY_FBWD_CMD                          0x55, 0x04, 0x04, 0x00, 0x29, 0x06
#define IPOD_PLAY_END_FAST_PLAY_CMD                 0x55, 0x04, 0x04, 0x00, 0x29, 0x07
#define IPOD_PLAY_NEXT_CHAPTER_CMD                  0x55, 0x04, 0x04, 0x00, 0x29, 0x08
#define IPOD_PLAY_PREV_CHAPTER_CMD                  0x55, 0x04, 0x04, 0x00, 0x29, 0x09
#define IPOD_PLAY_CONTROL_CMD                       0x55, 0x04, 0x04, 0x00, 0x29, 0x00


#   ifdef IPOD_PERSISTENT_SETTINGS
#       define IPOD_SHUFFLE_OFF_CMD                 0x55, 0x05, 0x04, 0x00, 0x2E, 0x00, 0x00
#       define IPOD_SHUFFLE_ON_SONGS_CMD            0x55, 0x05, 0x04, 0x00, 0x2E, 0x01, 0x00
#       define IPOD_SHUFFLE_ON_ALBUMS_CMD           0x55, 0x05, 0x04, 0x00, 0x2E, 0x02, 0x00
#       define IPOD_REPEAT_OFF_CMD                  0x55, 0x05, 0x04, 0x00, 0x31, 0x00, 0x00
#       define IPOD_REPEAT_CURRENT_SONG_CMD         0x55, 0x05, 0x04, 0x00, 0x31, 0x01, 0x00
#       define IPOD_REPEAT_ALL_SONGS_CMD            0x55, 0x05, 0x04, 0x00, 0x31, 0x02, 0x00
#   else /* IPOD_PERSISTENT_SETTINGS */
#       ifdef IPOD_PERSISTENT_SHUFFLE
#           define IPOD_SHUFFLE_OFF_CMD             0x55, 0x05, 0x04, 0x00, 0x2E, 0x00, 0x00
#           define IPOD_SHUFFLE_ON_SONGS_CMD        0x55, 0x05, 0x04, 0x00, 0x2E, 0x01, 0x00
#           define IPOD_SHUFFLE_ON_ALBUMS_CMD       0x55, 0x05, 0x04, 0x00, 0x2E, 0x02, 0x00
#       else /* IPOD_PERSISTENT_SHUFFLE */
#           define IPOD_SHUFFLE_OFF_CMD             0x55, 0x05, 0x04, 0x00, 0x2E, 0x00, 0x01
#           define IPOD_SHUFFLE_ON_SONGS_CMD        0x55, 0x05, 0x04, 0x00, 0x2E, 0x01, 0x01
#           define IPOD_SHUFFLE_ON_ALBUMS_CMD       0x55, 0x05, 0x04, 0x00, 0x2E, 0x02, 0x01
#       endif /* IPOD_PERSISTENT_SHUFFLE */

#       ifdef IPOD_PERSISTENT_REPEAT
#           define IPOD_REPEAT_OFF_CMD              0x55, 0x05, 0x04, 0x00, 0x31, 0x00, 0x00
#           define IPOD_REPEAT_CURRENT_SONG_CMD     0x55, 0x05, 0x04, 0x00, 0x31, 0x01, 0x00
#           define IPOD_REPEAT_ALL_SONGS_CMD        0x55, 0x05, 0x04, 0x00, 0x31, 0x02, 0x00
#       else  /* IPOD_PERSISTENT_REPEAT */
#           define IPOD_REPEAT_OFF_CMD              0x55, 0x05, 0x04, 0x00, 0x31, 0x00, 0x01
#           define IPOD_REPEAT_CURRENT_SONG_CMD     0x55, 0x05, 0x04, 0x00, 0x31, 0x01, 0x01
#           define IPOD_REPEAT_ALL_SONGS_CMD        0x55, 0x05, 0x04, 0x00, 0x31, 0x02, 0x01
#       endif /* IPOD_PERSISTENT_REPEAT */
#   endif /* IPOD_PERSISTENT_SETTINGS */

#define IPOD_GET_SHUFFLE_MODE_CMD                   0x55, 0x03, 0x04, 0x00, 0x2C
#define IPOD_GET_REPEAT_MODE_CMD                    0x55, 0x03, 0x04, 0x00, 0x2F
#define IPOD_GET_AUDIOBOOK_SPEED_CMD                0x55, 0x03, 0x04, 0x00, 0x09
#define IPOD_SET_AUDIOBOOK_SPEED_CMD                0x55, 0x04, 0x04, 0x00, 0x0B, 0x00
#define IPOD_GET_CUR_PLAY_TRACK_CHAPT_NAME_CMD      0x55, 0x07, 0x04, 0x00, 0x07, 0x00, 0x00, 0x00, 0x00
#define IPOD_GET_CUR_PLAY_TRACK_CHAPT_INFO_CMD      0x55, 0x03, 0x04, 0x00, 0x02
#define IPOD_SET_CUR_PLAYING_TRACK_CHAPTER_CMD      0x55, 0x07, 0x04, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00
#define IPOD_GET_CUR_PLAY_TR_CHAPT_PLAYSTAT_CMD     0x55, 0x07, 0x04, 0x00, 0x05, 0x00, 0x00, 0x00, 0x00
#define IPOD_GET_PLAY_STATUS_CMD                    0x55, 0x03, 0x04, 0x00, 0x1C
#define IPOD_GET_CUR_PLAYING_TRACK_INDEX_CMD        0x55, 0x03, 0x04, 0x00, 0x1E
#define IPOD_GET_INDEXED_PLAYING_TRACK_INFO_CMD     0x55, 0x0A, 0x04, 0x00, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
#define IPOD_GET_TITLE_FROM_PL_CMD                  0x55, 0x07, 0x04, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00
#define IPOD_GET_ARTIST_FROM_PL_CMD                 0x55, 0x07, 0x04, 0x00, 0x22, 0x00, 0x00, 0x00, 0x00
#define IPOD_GET_ALBUMNAME_FROM_PL_CMD              0x55, 0x07, 0x04, 0x00, 0x24, 0x00, 0x00, 0x00, 0x00
#define IPOD_GET_NUM_PLAYING_TRACKS_CMD             0x55, 0x03, 0x04, 0x00, 0x35
#define IPOD_SET_CUR_PLAYING_TRACK_CMD              0x55, 0x07, 0x04, 0x00, 0x37, 0x00, 0x00, 0x00, 0x00
#define IPOD_SET_PLAY_STATUS_CHANGE_NOTIFY_CMD      0x55, 0x04, 0x04, 0x00, 0x26, 0x00

#define IPOD_SEND_ACC_ACK_CMD                       0x55, 0x04, 0x0A, 0x00, 0x00, 0x04
#define IPOD_GET_ACC_SAMPLE_RATE_CAPS_CMD           0x55, 0x02, 0x0A, 0x02, 0xF2

#define IPOD_EXTENDED_SET_PLAYSTATUS_CHANGE_CMD     0x55, 0x07, 0x04, 0x00, 0x26, 0x00, 0x00, 0x00, 0x00
#define IPOD_SET_VIDEO_DELAY_CMD                    0x55, 0x06, 0x0A, 0x05, 0x00, 0x00, 0x00, 0x00
/* database related commands ================================================ */
#define IPOD_SELECT_SORT_DB_RECORDS_CMD             0x55, 0x09, 0x04, 0x00, 0x38, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
#define IPOD_RETR_CAT_DB_RECORDS_CMD                0x55, 0x0C, 0x04, 0x00, 0x1A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
#define IPOD_RESET_DB_SELECTION_CMD                 0x55, 0x03, 0x04, 0x00, 0x16
#define IPOD_SELECT_DB_RECORD_CMD                   0x55, 0x08, 0x04, 0x00, 0x17, 0x00, 0x00, 0x00, 0x00, 0x00
#define IPOD_RET_TO_PREV_DB_SELECTION_CMD           0x55, 0x08, 0x04, 0x00, 0x17, 0x00, 0xFF, 0xFF, 0xFF, 0xFF
#define IPOD_GET_NUM_CAT_DB_RECORDS_CMD             0x55, 0x04, 0x04, 0x00, 0x18, 0x00
#define IPOD_PLAY_CURRENT_SELECTION_CMD             0x55, 0x07, 0x04, 0x00, 0x28, 0x00, 0x00, 0x00, 0x00
#define IPOD_RESET_DB_SELECTION_HIERARCHY           0x55, 0x04, 0x04, 0x00, 0x3B, 0x00

#define IPOD_GET_DB_ITUNES_INFO_CMD                 0x55, 0x04, 0x04, 0x00, 0x3C, 0x00
#define IPOD_UID_CHAPTER_TOTAL_CMD                  0x55, 0x0D, 0x04, 0x00, 0x3E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01
#define IPOD_DB_CHAPTER_TOTAL_CMD                   0x55, 0x0D, 0x04, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01
#define IPOD_PB_CHAPTER_TOTAL_CMD                   0x55, 0x0D, 0x04, 0x00, 0x42, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01
#define IPOD_PREPARE_UID_LIST                       0x55, 0x0F, 0x04, 0x00, 0x4A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
#define IPOD_PLAY_PREPARED_UID_LIST                 0x55, 0x0C, 0x04, 0x00, 0x4B, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00

/* display related commands ================================================= */
#define IPOD_GET_MONO_DISPLAY_LIMITS_CMD            0x55, 0x03, 0x04, 0x00, 0x33
#define IPOD_GET_COLOR_DISPLAY_LIMITS_CMD           0x55, 0x03, 0x04, 0x00, 0x39
#define IPOD_GET_ARTWORK_FORMATS_CMD                0x55, 0x03, 0x04, 0x00, 0x0E
#define IPOD_GET_TRACK_ARTWORK_TIMES_CMD            0x55, 0x0D, 0x04, 0x00, 0x2A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
#define IPOD_GET_TRACK_ARTWORK_DATA_CMD             0x55, 0x0D, 0x04, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
#define IPOD_SET_DISPLAY_IMAGE_DESCR_CMD            0x55, 0x00, 0x00, 0x0E, 0x04, 0x00, 0x32, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
#define IPOD_SET_DISPLAY_IMAGE_DATA_CMD             0x55, 0x00, 0x00, 0x00, 0x04, 0x00, 0x32, 0x00, 0x00

#define IPOD_GET_TYPE_OF_TRACK_ARTWORK_DATA_CMD     0x55, 0x0D, 0x04, 0x00, 0x4E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
#define IPOD_GET_TYPE_OF_TRACK_ARTWORK_TIMES_CMD    0x55, 0x0D, 0x04, 0x00, 0x4C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00

/* storage related commands ================================================= */
#define IPOD_GET_IPOD_CAPS_CMD                          0x55, 0x02, 0x0C, 0x01
#define IPOD_GET_IPOD_FREESPACE_CMD                     0x55, 0x02, 0x0C, 0x10
#define IPOD_OPEN_FEATUREFILE_CMD                       0x55, 0x00, 0x0C, 0x12, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
#define IPOD_CLOSE_IPOD_FILE_CMD                        0x55, 0x03, 0x0C, 0x08, 0x00
#define IPOD_WRITE_IPOD_FILEDATA_CMD                    0x55, 0x00, 0x0C, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
#define IPOD_RET_DEVICE_CAPS_CMD                        0x55, 0x02, 0x0C, 0x82, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00
#define IPOD_DEVICE_ACK_CMD                             0x55, 0x05, 0x0C, 0x80, 0x00, 0x00, 0xFF

/* Location related commands */
#define IPOD_DEV_ACK_NO_SEC_CMD         0x55, 0x04, 0x0E, 0x00, 0x00, 0x00, 0x00
#define IPOD_DEV_ACK_SEC_CMD            0x55, 0x06, 0x0E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
#define IPOD_RET_DEV_CONTROL_CMD        0x55, 0x0B, 0x0E, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00

/* iPodOut related commands */
#define IPOD_GET_IPOD_OUT_OPTIONS_CMD   0x55, 0x03, 0x0D, 0x01, 0x00
#define IPOD_SET_IPOD_OUT_OPTIONS_CMD   0x55, 0x06, 0x0D, 0x03, 0x00, 0x00, 0x00, 0x00
#define IPOD_DEV_STATE_CHANGE_EVENT_CMD 0x55, 0x03, 0x0D, 0x04, 0x00
#define IPOD_DEV_VIDEO_SCREEN_INFO_CMD  0x55, 0x10, 0x0D, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00

/* general lingo command ids ================================================ */
#define IPOD_GENERAL_LINGO_RequestIdentify                              0x00
#define IPOD_GENERAL_LINGO_Identify                                     0x01
#define IPOD_GENERAL_LINGO_ACK                                          0x02
#define IPOD_GENERAL_LINGO_RequestRemoteUIMode                          0x03
#define IPOD_GENERAL_LINGO_ReturnRemoteUIMode                           0x04
#define IPOD_GENERAL_LINGO_EnterRemoteUIMode                            0x05
#define IPOD_GENERAL_LINGO_ExitRemoteUIMode                             0x06
#define IPOD_GENERAL_LINGO_RequestiPodName                              0x07
#define IPOD_GENERAL_LINGO_ReturniPodName                               0x08
#define IPOD_GENERAL_LINGO_RequestiPodSoftwareVersion                   0x09
#define IPOD_GENERAL_LINGO_ReturniPodSoftwareVersion                    0x0A
#define IPOD_GENERAL_LINGO_RequestiPodSerialNum                         0x0B
#define IPOD_GENERAL_LINGO_ReturniPodSerialNum                          0x0C
#define IPOD_GENERAL_LINGO_RequestiPodModelNum                          0x0D
#define IPOD_GENERAL_LINGO_ReturniPodModelNum                           0x0E
#define IPOD_GENERAL_LINGO_RequestLingoProtocolVersion                  0x0F
#define IPOD_GENERAL_LINGO_ReturnLingoProtocolVersion                   0x10

#define IPOD_GENERAL_LINGO_IdentifyDeviceLingoes                        0x13
#define IPOD_GENERAL_LINGO_GetDevAuthenticationInfo                     0x14
#define IPOD_GENERAL_LINGO_RetDevAuthenticationInfo                     0x15
#define IPOD_GENERAL_LINGO_AckDevAuthenticationInfo                     0x16

#define IPOD_GENERAL_LINGO_GetDevAuthenticationSignature                0x17
#define IPOD_GENERAL_LINGO_RetDevAuthenticationSignature                0x18
#define IPOD_GENERAL_LINGO_AckDevAuthenticationStatus                   0x19
#define IPOD_GENERAL_LINGO_GetiPodAuthenticationInfo                    0x1A
#define IPOD_GENERAL_LINGO_RetiPodAuthenticationInfo                    0x1B
#define IPOD_GENERAL_LINGO_AckiPodAuthenticationInfo                    0x1C
#define IPOD_GENERAL_LINGO_GetiPodAuthenticationSignature               0x1D
#define IPOD_GENERAL_LINGO_RetiPodAuthenticationSignature               0x1E
#define IPOD_GENERAL_LINGO_AckiPodAuthenticationStatus                  0x1F

#define IPOD_GENERAL_LINGO_NotifyiPodStateChange                        0x23
#define IPOD_GENERAL_LINGO_GetiPodOptions                               0x24
#define IPOD_GENERAL_LINGO_RetiPodOptions                               0x25

#define IPOD_GENERAL_LINGO_GetAccessoryInfo                             0x27
#define IPOD_GENERAL_LINGO_RetAccessoryInfo                             0x28
#define IPOD_GENERAL_LINGO_GetiPodPreferences                           0x29
#define IPOD_GENERAL_LINGO_RetiPodPreferences                           0x2A
#define IPOD_GENERAL_LINGO_SetiPodPreferences                           0x2B

/* IDPS commands */
#define IPOD_GENERAL_LINGO_StartIDPS                                    0x38
#define IPOD_GENERAL_LINGO_SetFIDTokenValues                            0x39
#define IPOD_GENERAL_LINGO_RetFIDTokenValueAcks                         0x3A
#define IPOD_GENERAL_LINGO_IDPS_Status                                  0x3C
#define IPOD_GENERAL_LINGO_OpenDataSession                              0x3F
#define IPOD_GENERAL_LINGO_CloseDataSession                             0x40
#define IPOD_GENERAL_LINGO_iPodDataTransfer                             0x43

/* New Feature 2/1/2010 */
#define IPOD_GENERAL_LINGO_iPod_Notification                            0x4A
#define IPOD_GENERAL_LINGO_RetiPodOptionsForLingo                       0x4C
#define IPOD_GENERAL_LINGO_RetEventNotification                         0x4E
#define IPOD_GENERAL_LINGO_GetSupportedEventNotification                0x51

/* New Feature 28/7/2010 */
#define IPOD_GENERAL_LINGO_iPodAccStatusNotification                    0x48
#define IPOD_GENERAL_LINGO_iPodDevDataTransfer                          0x42
#define IPOD_GENERAL_LINGO_iPodRequestAppLaunch                         0x64
#define IPOD_GENERAL_LINGO_iPodRetNowPlayingFocusApp                    0x66
#define IPOD_GENERAL_LINGO_iPodRetTransportMaxPacketSize                0x12
#define IPOD_GENERAL_LINGO_iPodSetAccStatusNotification                 0x46

/* New Feature 2/12/2010 */
#define IPOD_GENERAL_LINGO_iPodRetUIMode                                0x36

/* simple remote lingo command ids ========================================== */
#define IPOD_SIMPLE_LINGO_ACKNOWLEDGE                                   0x01
/* display remote lingo command ids ========================================== */
#define IPOD_DISPLAY_LINGO_ACKNOWLEDGE                                  0x00
#define IPOD_DISPLAY_LINGO_RET_CUR_EQ_INDEX                             0x02
#define IPOD_DISPLAY_LINGO_RET_NUM_EQ_PROFILES                          0x05
#define IPOD_DISPLAY_LINGO_RET_INDEXED_EQ_NAME                          0x07
#define IPOD_DISPLAY_LINGO_REMOTE_EVENT_NOTIFICATION                    0x09
#define IPOD_DISPLAY_LINGO_RET_ARTWORK_FORMATS                          0x17
#define IPOD_DISPLAY_LINGO_RETARTWORKDATA                               0x19
#define IPOD_DISPLAY_LINGO_RETARTWORKTIME                               0x20
#define IPOD_DISPLAY_LINGO_RETPLAYSTATUS                                0x10
/* extended lingo command ids =============================================== */
#define IPOD_EXTENDED_LINGO_ACKNOWLEDGE                                 0x0001
#define IPOD_EXTENDED_LINGO_GET_CUR_PLAYING_TRACK_CHAPTER_INFO          0x0002
#define IPOD_EXTENDED_LINGO_RET_CUR_PLAYING_TRACK_CHAPTER_INFO          0x0003
#define IPOD_EXTENDED_LINGO_SET_CUR_PLAYING_TRACK_CHAPTER               0x0004
#define IPOD_EXTENDED_LINGO_GET_CUR_PLAYING_TRACK_CHAPTER_PLAYSTATUS    0x0005
#define IPOD_EXTENDED_LINGO_RET_CUR_PLAYING_TRACK_CHAPTER_PLAYSTATUS    0x0006
#define IPOD_EXTENDED_LINGO_GET_CUR_PLAYING_TRACK_CHAPTER_NAME          0x0007
#define IPOD_EXTENDED_LINGO_RET_CUR_PLAYING_TRACK_CHAPTER_NAME          0x0008

#define IPOD_EXTENDED_LINGO_GET_AUDIOBOOK_SPEED                         0x0009
#define IPOD_EXTENDED_LINGO_RET_AUDIOBOOKSPEED                          0x000A
#define IPOD_EXTENDED_LINGO_SET_AUDIOBOOKSPEED                          0x000B

#define IPOD_EXTENDED_LINGO_GET_INDEXED_PLAYING_TRACKINFO               0x000C
#define IPOD_EXTENDED_LINGO_RET_INDEXED_PLAYING_TRACKINFO               0x000D

#define IPOD_EXTENDED_LINGO_GET_ARTWORKFORMATS                          0x000E
#define IPOD_EXTENDED_LINGO_RET_ARTWORKFORMATS                          0x000F
#define IPOD_EXTENDED_LINGO_GET_TRACKARTWORKDATA                        0x0010
#define IPOD_EXTENDED_LINGO_RET_TRACKARTWORKDATA                        0x0011
#define IPOD_EXTENDED_LINGO_REQ_PROTOCOLVERSION                         0x0012
#define IPOD_EXTENDED_LINGO_RET_PROTOCOLVERSION                         0x0013
#define IPOD_EXTENDED_LINGO_REQ_IPODNAME                                0x0014
#define IPOD_EXTENDED_LINGO_RET_IPODNAME                                0x0015

#define IPOD_EXTENDED_LINGO_RESET_DB_SELECTION                          0x0016
#define IPOD_EXTENDED_LINGO_SELECT_DB_RECORD                            0x0017

#define IPOD_EXTENDED_LINGO_GET_NUM_CATEGORIZED_DB_RECORDS              0x0018
#define IPOD_EXTENDED_LINGO_RET_NUM_CATEGORIZED_DB_RECORDS              0x0019
#define IPOD_EXTENDED_LINGO_RETR_CATEGORIZED_DATABASE_RECORDS           0x001A
#define IPOD_EXTENDED_LINGO_RET_CATEGORIZED_DATABASE_RECORD             0x001B
#define IPOD_EXTENDED_LINGO_GET_PLAYSTATUS                              0x001C
#define IPOD_EXTENDED_LINGO_RET_PLAYSTATUS                              0x001D
#define IPOD_EXTENDED_LINGO_GET_CUR_PLAYING_TRACK_INDEX                 0x001E
#define IPOD_EXTENDED_LINGO_RET_CUR_PLAYINGT_TRACK_INDEX                0x001F
#define IPOD_EXTENDED_LINGO_GET_INDEXED_PLAYING_TRACK_TITLE             0x0020
#define IPOD_EXTENDED_LINGO_RET_INDEXED_PLAYING_TRACK_TITLE             0x0021
#define IPOD_EXTENDED_LINGO_GET_INDEXED_PLAYING_TRACK_ARTISTNAME        0x0022
#define IPOD_EXTENDED_LINGO_RET_INDEXED_PLAYING_TRACK_ARTISTNAME        0x0023
#define IPOD_EXTENDED_LINGO_GET_INDEXED_PLAYING_TRACK_ALBUMNAME         0x0024
#define IPOD_EXTENDED_LINGO_RET_INDEXED_PLAYING_TRACK_ALBUMNAME         0x0025

#define IPOD_EXTENDED_LINGO_SET_PLAYSTATUS_CHANGENOTIFICATION           0x0026
#define IPOD_EXTENDED_LINGO_PLAYSTATUSCHANGENOTIFICATION                0x0027

#define IPOD_EXTENDED_LINGO_PLAY_CURRENT_SELECTION                      0x0028
#define IPOD_EXTENDED_LINGO_PLAYCONTROL                                 0x0029
#define IPOD_EXTENDED_LINGO_GET_TRACK_ARTWORKTIMES                      0x002A
#define IPOD_EXTENDED_LINGO_RET_TRACK_ARTWORKTIMES                      0x002B

#define IPOD_EXTENDED_LINGO_GET_SHUFFLE                                 0x002C
#define IPOD_EXTENDED_LINGO_RET_SHUFFLE                                 0x002D
#define IPOD_EXTENDED_LINGO_SET_SHUFFLE                                 0x002E
#define IPOD_EXTENDED_LINGO_GET_REPEAT                                  0x002F
#define IPOD_EXTENDED_LINGO_RET_REPEAT                                  0x0030
#define IPOD_EXTENDED_LINGO_SET_REPEAT                                  0x0031

#define IPOD_EXTENDED_LINGO_SET_DISPLAYIMAGE                            0x0032
#define IPOD_EXTENDED_LINGO_GET_MONO_DISPLAY_IMAGE_LIMITS               0x0033
#define IPOD_EXTENDED_LINGO_RET_MONO_DISPLAY_IMAGE_LIMITS               0x0034
#define IPOD_EXTENDED_LINGO_GET_NUM_PLAYING_TRACKS                      0x0035
#define IPOD_EXTENDED_LINGO_RET_NUM_PLAYING_TRACKS                      0x0036
#define IPOD_EXTENDED_LINGO_SET_CURRENT_PLAYING_TRACK                   0x0037
#define IPOD_EXTENDED_LINGO_SELECT_SORT_DBRECORD                        0x0038
#define IPOD_EXTENDED_LINGO_GET_COLOR_DISPLAY_IMAGE_LIMITS              0x0039
#define IPOD_EXTENDED_LINGO_RET_COLOR_DISPLAY_IMAGE_LIMITS              0x003A
#define IPOD_EXTENDED_LINGO_RESET_DB_SELECTION_HIERARCHY                0x003B
#define IPOD_EXTENDED_LINGO_GET_DB_ITUNES_INFO                          0x003C
#define IPOD_EXTENDED_LINGO_RET_DB_ITUNES_INFO                          0x003D
#define IPOD_EXTENDED_LINGO_GET_UID_TRACK_INFO                          0x003E
#define IPOD_EXTENDED_LINGO_RET_UID_TRACK_INFO                          0x003F
#define IPOD_EXTENDED_LINGO_GET_DB_TRACK_INFO                           0x0040
#define IPOD_EXTENDED_LINGO_RET_DB_TRACK_INFO                           0x0041
#define IPOD_EXTENDED_LINGO_GET_PB_TRACK_INFO                           0x0042
#define IPOD_EXTENDED_LINGO_RET_PB_TRACK_INFO                           0x0043
#define IPOD_EXTENDED_LINGO_RET_TYPE_OF_TRACKARTWORKDATA                0x004F
#define IPOD_EXTENDED_LINGO_RET_TYPE_OF_TRACK_ARTWORKTIMES              0x004D

/*------------------------- digital audio commands ids ------------------------*/
#define IPOD_IPOD_ACK                                                   0x0001
#define IPOD_AUDIO_LINGO_GetAccSamplerateCaps                           0x0002
#define IPOD_AUDIO_LINGO_NewiPodTrackInfo                               0x0004

/*------------------------- storage commands ids ------------------------------*/

#define IPOD_STORAGE_LINGO_ACKNOWLEDGE                                  0x0000
#define IPOD_STORAGE_LINGO_GET_IPOD_CAPS                                0x0001
#define IPOD_STORAGE_LINGO_RET_IPOD_CAPS                                0x0002
#define IPOD_STORAGE_LINGO_IPOD_RET_IPOD_FILE_HANDLE                    0x0004
#define IPOD_STORAGE_LINGO_WRITE_IPOD_FILE_DATA                         0x0007
#define IPOD_STORAGE_LINGO_CLOSE_IPOD_FILE                              0x0008
#define IPOD_STORAGE_LINGO_GET_IPOD_FREE_SPACE                          0x0010
#define IPOD_STORAGE_LINGO_RET_IPOD_FREE_SPACE                          0x0011
#define IPOD_STORAGE_LINGO_OPEN_IPOD_FEATURE_FILE                       0x0012
#define IPOD_STORAGE_LINGO_DEVICE_ACK                                   0x0080
#define IPOD_STORAGE_LINGO_GET_DEVICE_CAPS                              0x0081
#define IPOD_STORAGE_LINGO_RET_DEVICE_CAPS                              0x0082

/*------------------------ location commands ids ------------------------------*/
#define IPOD_LOCATION_LINGO_GetDevCaps      0x01
#define IPOD_LOCATION_LINGO_RetDevCaps      0x02
#define IPOD_LOCATION_LINGO_GetDevControl   0x03
#define IPOD_LOCATION_LINGO_RetDevControl   0x04
#define IPOD_LOCATION_LINGO_SetDevControl   0x05
#define IPOD_LOCATION_LINGO_GetDevData      0x06
#define IPOD_LOCATION_LINGO_RetDevData      0x07
#define IPOD_LOCATION_LINGO_SetDevData      0x08
#define IPOD_LOCATION_LINGO_AsyncDevData    0x09
#define IPOD_LOCATION_LINGO_iPodAck         0x80

/*------------------------- iPodOut commands ids -------------------------------*/
#define IPOD_IPODOUT_LINGO_iPodAck           0x00
#define IPOD_IPODOUT_LINGO_RetiPodOutOptions 0x02

#ifdef __cplusplus
}
#endif

#endif

