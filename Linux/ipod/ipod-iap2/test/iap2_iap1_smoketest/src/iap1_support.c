/*
 * iap1_support.c
 *
 *  Created on: Nov 18, 2013
 *      Author: ajaykumar.s
 */
#include "iap1_support.h"
#include "pthread_adit.h"
#include "sys_time_adit.h"
#include "iap_init.h"
#include "iap_general.h"
#include "ipodcommon.h"
#include "iap_common.h"
#include "authentication.h"

#include <libudev.h>

/* max number of supported iPods */
#define NUM_IPOD 8
#define MAX_CONNECT_DEVICE_RETRIES 3

#if TEST_CANCEL_DEV_CON
#define CANCEL_DEV_CON_IPOD_ID  1
#endif

#if TEST_INIT_ACC_CON
/* accessory info structure */
IPOD_ACC_INFO_CONFIGURATION acc_info;
#endif

#ifndef IPOD_HOSTPLUGIN_HIDAPI
U8 g_devicename[NUM_IPOD][32] = {"/dev/usb/hiddev0"};
#else
U8 g_serialNum[NUM_IPOD][DEV_DETECT_CFG_STRING_MAX];
#endif
typedef struct _GET_ARTWORK
{
    U16 artworkCnt;
    U16 artworkFormatId;
} GET_ARTWORK;

U8 g_CBOpenApp = 0;
/* number of detected iPods */
U16 g_detectediPods = 0;
/* structure for each connected iPod */
typedef struct _IPOD_DEV
{
    S32 connected;                              /* attach/detach state */
    S32 iPodID;                                 /* iPod Ctrl ID for the iPod */
    U8  serialNum[DEV_DETECT_CFG_STRING_MAX];   /* the serial number of the iPod */
    U64* arrayUID;                           /* array with several track UID's */
    U16 countUID;                               /* number of received UID's */
    U32 length;                                 /* length of current track */
    U32 position;                               /* play position of current track */
    IPOD_PLAYER_STATE playState;                /* play state of the iPod */
    GET_ARTWORK artworkData[20];                /* available artwork and format ID */
    U16 formatCnt;                              /* number of available formats */
    U8 getArtwork;                              /* indicates the state of the artwork download */
    U8 getInterrupted;                          /* indicates if respective test was interrupted */
    S32 testFailCount;
    U16 CBcount;
    U32 trackIndex;

} IPOD_DEV;
IPOD_DEV iPods[NUM_IPOD];

S32 g_failCnt = 0;

IPOD_TASK_ID g_ttask_id[NUM_IPOD];
pthread_mutex_t callback_trackinfo_mutex = PTHREAD_MUTEX_INITIALIZER;



EXPORT BOOL  g_iap1Device = FALSE;
EXPORT sem_t g_iap1semlock ;


/* ----------------------  device detection functions  ---------------------- */

/* check USB port for connected iPods */
S32 iPodCheckForDevice(struct udev *udev);
S32 iPodCheckForDevice(struct udev *udev)
{
    S32 ret = IPOD_ERROR;

    struct udev_enumerate *enumerate;
    struct udev_list_entry *devices, *dev_list_entry;
    struct udev_device *dev;

    const char *idVendor = NULL;
    const char *idProduct = NULL;
    const char *serial = NULL;

    if(udev != NULL)
    {
        /* Create a list of the devices in the 'hidraw' subsystem. */
        enumerate = udev_enumerate_new(udev);
        if(enumerate == NULL)
        {
            printf("enumerate is NULL \n");
            exit(1);
        }
        ret = udev_enumerate_add_match_subsystem(enumerate, (const char *)IPOD_USB_MONITOR_DEVTYPE);
        if(ret != IPOD_OK)
        {
            printf("enumerate_add_match_subsystem failed %d \n", ret);
            exit(1);
        }
        /* search only for Apple devices */
        ret = udev_enumerate_add_match_sysattr(enumerate, IPOD_SYSATTR_IDVENDOR, IPOD_APPLE_IDVENDOR);
        if(ret != IPOD_OK)
        {
            printf("enumerate_add_match_sysattr failed %d \n", ret);
            exit(1);
        }
        ret = udev_enumerate_scan_devices(enumerate);
        if(ret != IPOD_OK)
        {
            printf("enumerate_scan_devices failed %d \n", ret);
            exit(1);
        }
        devices = udev_enumerate_get_list_entry(enumerate);
        if(devices == NULL)
        {
            printf("devices is NULL (list_entry)\n");
            /* no specified Apple device available */
            ret = IPOD_ERROR;
        }
        else
        {
            ret = IPOD_ERROR;
            /* For each item enumerated, print out its information.
               udev_list_entry_foreach is a macro which expands to
               a loop. The loop will be executed for each member in
               devices, setting dev_list_entry to a list entry
               which contains the device's path in /sys. */
            udev_list_entry_foreach(dev_list_entry, devices)
            {
                const char *path;

                /* Get the filename of the /sys entry for the device
                   and create a udev_device object (dev) representing it */
                path = udev_list_entry_get_name(dev_list_entry);
                if(path == NULL)
                {
                    /* Get the path of device failed*/
                    printf("path of device failed \n");
                }
                else
                {
                    dev = udev_device_new_from_syspath(udev, path);
                    if(dev != NULL)
                    {
                        /* usb_device_get_devnode() returns the path to the device node
                           itself in /dev. */
//                         printf("Device Node Path: %s\n", udev_device_get_devnode(dev));

                        /* From here, we can call get_sysattr_value() for each file
                           in the device's /sys entry. The strings passed into these
                           functions (idProduct, idVendor, serial, etc.) correspond
                           directly to the files in the directory which represents
                           the USB device. Note that USB strings are Unicode, UCS2
                           encoded, but the strings returned from
                           udev_device_get_sysattr_value() are UTF-8 encoded. */
                        idVendor = udev_device_get_sysattr_value(dev,"idVendor");
                        idProduct = udev_device_get_sysattr_value(dev, "idProduct");
                        serial = udev_device_get_sysattr_value(dev, "serial");
//                        printf("VID: %s  PID: %s  SERIAL: %s \n", idVendor, idProduct, serial);

                        if( (idVendor != NULL) && (idProduct != NULL) && (serial != NULL) )
                        {
                            /* Vendor ID equal to Apple Vendor ID and Product ID equal to Apple Product ID */
                            if((strncmp((const char *)idVendor, (const char *)IPOD_APPLE_IDVENDOR, DEV_DETECT_VENDOR_MAX_LEN) == 0) &&
                               (strncmp((const char *)idProduct, (const char *)IPOD_APPLE_IDPRODUCT_MIN, 2) == 0))
                            {
                                ret = IPOD_OK;
                                strncpy((char*)(&g_serialNum[g_detectediPods][0]), serial, DEV_DETECT_CFG_STRING_MAX);
                                printf(" VID: %s  PID: %s  SERIAL: %s \n", idVendor, idProduct, &g_serialNum[g_detectediPods][0]);
                                g_detectediPods++;
                            }
                        }
                        else
                        {
                            printf("device_get_sysattr_value(idVendor or idProduct or serial) is NULL \n");
                        }

                        udev_device_unref(dev);
                    }
                    else
                    {
                        printf("device_new_from_syspath failed \n");
                    }
                }
            }
        }
        /* Free the enumerator object */
        udev_enumerate_unref(enumerate);
    }

    return ret;
}

S32 iPodUdev(void);
S32 iPodUdev(void)
{
    S32 ret = IPOD_ERROR;
    int retryCount = 0;

    struct udev *udev;
    struct udev_device *dev;
    struct udev_monitor *monitor;
    S32 udevfd = -1;
    const char *action = NULL;

    /* Create the udev object */
    udev = udev_new();
    if (!udev) {
        printf("Can't create udev\n");
        exit(1);
    }
    else
    {
        /* set number of detected iPods to 0 */
        g_detectediPods = 0;

        /* check if iPod is already connected */
        ret = iPodCheckForDevice(udev);
        /* no iPod available. wait for connecting iPod */
        if(ret != IPOD_OK)
        {
            /* Link the monitor to "udev" devices */
            monitor = udev_monitor_new_from_netlink(udev, (const char *)IPOD_USB_MONITOR_LINK);
            if(monitor != NULL)
            {
                /* Filter only for get the "hiddev" devices */
                ret = udev_monitor_filter_add_match_subsystem_devtype(monitor, (const char *)IPOD_USB_MONITOR_DEVTYPE, IPOD_USB_FILTER_TYPE);
                if(ret == IPOD_OK)
                {
                    /* Start receiving */
                    ret = udev_monitor_enable_receiving(monitor);
                    if(ret == IPOD_OK)
                    {
                        /* Get the file descriptor for the monitor. This fd is used by select() */
                        udevfd = udev_monitor_get_fd(monitor);
                        ret = IPOD_ERROR;
                        printf("   Please connect iPod!\n");
                        while((ret == IPOD_ERROR) && (retryCount < IPOD_USB_SELECT_RETRY_CNT))
                        {
                            retryCount++;

                            fd_set fds;
                            struct timeval tv;

                            FD_ZERO(&fds);
                            FD_SET(udevfd, &fds);
                            tv.tv_sec = 5;
                            tv.tv_usec = 0;

                            ret = select(udevfd+1, &fds, NULL, NULL, &tv);
                            /* Check if our file descriptor has received data. */
                            if (ret > 0 && FD_ISSET(udevfd, &fds))
                            {
                                /* Make the call to receive the device.
                                   select() ensured that this will not block. */
                                dev = udev_monitor_receive_device(monitor);
                                if (dev)
                                {
//                                    printf("Got Device\n");
//                                    printf("   Node: %s\n", udev_device_get_devnode(dev));
//                                    printf("   Subsystem: %s\n", udev_device_get_subsystem(dev));
//                                    printf("   Devtype: %s\n", udev_device_get_devtype(dev));
                                    action = udev_device_get_action(dev);
                                    if(strncmp((const char *)action, IPOD_USB_ACTION_ADD, 4) == 0)
                                    {
                                        ret = IPOD_OK;
                                        printf("   Action is %s -> ret = %d", action, ret);
                                    }
                                    else
                                    {
                                        ret = IPOD_ERROR;
                                        printf("   Action is %s -> ret = %d", action, ret);
                                    }

                                    udev_device_unref(dev);
                                }
                                else
                                {
                                    printf("   Error: No iPod ");
                                }
                            }
                            else if(ret < 0)
                            {
                                printf("   Error: select() failed");
                            }
                            else
                            {
                                /* this occurs if an iPod is already connected */
                                printf("   Error: No iPod found");
                                ret = IPOD_ERROR;
                            }
                            printf("    retry %d\n", retryCount);
                        }

                        if((ret == IPOD_ERROR) || (retryCount >= 5))
                        {
                            printf("   Error: max retries %d | ret = %d\n", retryCount, ret);
                        }
                        else /* IPOD_OK */
                        {
                            /* get Serial-Nr. VendorID and ProductID of newly connected iPod */
                            ret = iPodCheckForDevice(udev);
                            /* no iPod available. wait for connecting iPod */
                            if(ret != IPOD_OK)
                            {
                                printf("   Error: enumerate connected iPod failed | ret = %d \n", ret);
                            }
                        }
                    }
                }
            }
            else
            {
                printf("\n monitor NULL\n");
                exit(1);
            }
        }
    }

    udev_unref(udev);

    return ret;
}

/* ----------------------  callback functions  ---------------------- */

void ipod_usb_attach(const S32 success, IPOD_CONNECTION_TYPE connection, const U32 id)
{
    printf(" #### ipod_usb_attach:  iPod %s with ID %d  succ = %d \n", &iPods[id].serialNum[0], id, success);
    if(success == IPOD_OK)
    {
        printf("     attach:  iPod %s with ID %d on %d attached! \n", &iPods[id].serialNum[0], id, connection);
        iPods[id].connected = IPOD_ATTACHED;
    }
    else if(success == 1)
    {
        printf("     attach:  iPod %s with ID %d found!\n", &iPods[id].serialNum[0], id);
    }
    else
    {
        printf("     attach:  iPod %s with ID %d has problem %d \n", &iPods[id].serialNum[0], id, success);
        iPods[id].connected = success;
    }
}

void ipod_usb_detach(const U32 id)
{
    printf("     detach:  iPod %s with ID %d detached! \n", &iPods[id].serialNum[0], id);
    iPods[id].connected = IPOD_DETACHED;
}

void track_info_cb (U64 trackIndex, IPOD_TRACK_INFORMATION_TYPE infoType, IPOD_TRACK_INFORMATION_DATA *infoData, const U32 id)
{
    printf("\t Device %d: TrackInfo No. %lu: Type %d ", id, (long unsigned int)trackIndex, infoType);
    iPods[id].CBcount++;
    switch(infoType)
    {
        case TRACK_NAME:
        {
            printf("%s \n", infoData->trackName);
            break;
        }
        case ARTIST_NAME:
        {
            printf("%s \n", infoData->artistName);
            break;
        }
        case ALBUM_NAME:
        {
            printf("%s \n", infoData->albumName);
            break;
        }
        case GENRE_NAME:
        {
            printf("%s \n", infoData->genreName);
            break;
        }
        case YEAR:
        {
            printf("%u \n", infoData->yaer);
            break;
        }
        case UID:
        {
            if(iPods[id].arrayUID != NULL)
            {
                if(iPods[id].countUID >= trackIndex)
                {
                    *(iPods[id].arrayUID+trackIndex) = infoData->trackUID;
                    printf("0x%.16lx \n", (long unsigned int)(iPods[id].arrayUID+trackIndex));
                }
                else
                {
                    printf("0x%.16lx \n", (long unsigned int)infoData->trackUID);
                }
            }
            else
            {
                printf(" Device %d: arrayUID is NULL! \n", id);
            }
            break;
        }
        case CHAPTER_NAMES:
        {
            printf("Chapter -Index: %d  -totalCount %d  -Name: '%s' \n",
                    infoData->chapterNames.chapterIndex, infoData->chapterNames.totalCount, infoData->chapterNames.name);
            break;
        }
        case CHAPTER_TIMES:
        {
            printf("chapterOffset: %d ms \n", infoData->chapterTimes.offset);
            break;
        }
        case CHAPTER_COUNT:
        {
            printf("%d \n", infoData->chapterCount);
            break;
        }
        case LYRIC_STRING:
        {
            printf("lyric -Section: %d  -maxSection: %d  -Lyric: '%s' \n",
                    infoData->lyrics.section, infoData->lyrics.maxSection, infoData->lyrics.lyric);
            break;
        }
        default:
        {
            printf("infoType %d not implemented \n", infoType);
            break;
        }
    }
}

void sampleratechange(U32 newSampleRate,
                      S32 newSoundCheckValue,
                      S32 newTrackVolAdjustment, const U32 id)
{
    printf("\t Device %d: SampleRate: %d, SoundCheck: %d, VolAdjust: %d\n", id, newSampleRate, newSoundCheckValue, newTrackVolAdjustment);
    iPodAccAck(id, IPOD_ACC_ACK_STATUS_SUCCESS);
}

void retrieve_DB_record(U32 trackIndex, U8* string, const U32 id)
{
    printf("\t Device %d: Track No. %d: %s\n", id, trackIndex, string);
}

void CBRemoteEventNotification(IPOD_STATE_INFO_TYPE eventNum, IPOD_REMOTE_EVENT_NOTIFY_STATUS eventData, const U32 id)
{
    switch(eventNum)
    {
        case IPOD_STATE_INFO_TRACK_POSITION_MS:
        {
            printf("\t Device %d:  CBRemoteEventNotify():  track position ms: %d \n", id, eventData.trackPosMS);
            break;
        }
        case IPOD_STATE_INFO_TRACK_INDEX:
        {
            printf("\t Device %d:  CBRemoteEventNotify():  track index: 0x%x \n", id, eventData.trackIndex);
            break;
        }
        case IPOD_STATE_INFO_CHAPTER_INFO:
        {
            printf("\t Device %d:  CBRemoteEventNotify():  chapterCount: %d  chapterIndex: 0x%x  trackIndex: %d \n",
                id, eventData.chapterInfo.chapterCount, eventData.chapterInfo.chapterIndex, eventData.chapterInfo.trackIndex);
            break;
        }
        case IPOD_STATE_INFO_PLAY_STATUS:
        {
            printf("\t Device %d:  CBRemoteEventNotify():  play status: 0x%x \n", id, eventData.playStatus);
            break;
        }
        case IPOD_STATE_INFO_MUTE_VOLUME:
        {
            printf("\t Device %d:  CBRemoteEventNotify():  mute: %d  UI volume: %d \n",
                    id, eventData.muteUiVol.muteState, eventData.muteUiVol.uiVol);
            break;
        }
        case IPOD_STATE_INFO_POWER_BATTERY:
        {
            printf("\t Device %d:  CBRemoteEventNotify():  power state: 0x%x  battery level: %d \n",
                    id, eventData.powerBattery.powerState, eventData.powerBattery.batteryLevel);
            break;
        }
        case IPOD_STATE_INFO_EQUALIZER_STATE:
        {
            printf("\t Device %d:  CBRemoteEventNotify():  equalizer state: 0x%x \n", id, eventData.eqState);
            break;
        }
        case IPOD_STATE_INFO_SHUFFLE:
        {
            printf("\t Device %d:  CBRemoteEventNotify():  shuffle state: 0x%x \n", id, eventData.shuffle);
            break;
        }
        case IPOD_STATE_INFO_REPEAT:
        {
            printf("\t Device %d:  CBRemoteEventNotify():  repeat state: 0x%x \n", id, eventData.repeat);
            break;
        }
        case IPOD_STATE_INFO_DATE_TIME:
        {
            printf("\t Device %d:  CBRemoteEventNotify():  time: %d:%d  date: %d %d %d \n",
                    id, eventData.currDateTime.hour, eventData.currDateTime.min, eventData.currDateTime.day, eventData.currDateTime.month, eventData.currDateTime.year);
            break;
        }
        case IPOD_STATE_INFO_BACKLIGHT:
        {
            printf("\t Device %d:  CBRemoteEventNotify():  backlight: %d \n", id, eventData.backlight);
            break;
        }
        case IPOD_STATE_INFO_HOLD_SWITCH:
        {
            printf("\t Device %d:  CBRemoteEventNotify():  hold switch: %d \n", id, eventData.holdSwitch);
            break;
        }
        case IPOD_STATE_INFO_SOUND_CHECK:
        {
            printf("\t Device %d:  CBRemoteEventNotify():  sound check state: %d \n", id, eventData.soundCheck);
            break;
        }
        case IPOD_STATE_INFO_AUDIOBOOK_SPEED:
        {
            printf("\t Device %d:  CBRemoteEventNotify():  audiobook speed: %d \n", id, eventData.audiobook);
            break;
        }
        case IPOD_STATE_INFO_TRACK_POSITION_S:
        {
            printf("\t Device %d:  CBRemoteEventNotify():  track position sec: %d \n", id, eventData.trackPosSec);
            break;
        }
        case IPOD_STATE_INFO_MUTE_EXTENDED_VOLUME:
        {
            printf("\t Device %d:  CBRemoteEventNotify():  mute: %d  UI volume: %d  absolute volume: %d \n",
                    id, eventData.muteUiAbsoluteVol.muteState, eventData.muteUiAbsoluteVol.uiVol, eventData.muteUiAbsoluteVol.absVol);
            break;
        }
        case IPOD_STATE_INFO_TRACK_CAPABILITIES:
        {
            printf("\t Device %d:  CBRemoteEventNotify():  track capabilities changed \n", id);
            break;
        }
        case IPOD_STATE_INFO_NUM_OF_TRACKS_IN_PLAYLIST:
        {
            printf("\t Device %d:  CBRemoteEventNotify():  number of tracks in playlist: %d \n", id, eventData.playEngineContent);
            break;
        }
        case IPOD_STATE_INFO_ALARM:
        default:
        {
            printf("\t Device %d:  CBRemoteEventNotify():  Unknown / Depreciated RemoteEventNotification 0x%x \n", id, eventNum);
            break;
        }
    }
}

void CBNotification(IPOD_NOTIFY_TYPE type, IPOD_NOTIFY_STATUS status, const U32 id)
{
    U16 commandID = 0;

    switch(type)
    {
    case IPOD_EVT_FLOW_CTRL:
        printf("\t Device %d: Flow Control = %d\n", id, status.waitMs);
        break;
    case IPOD_EVT_RADIO_TAG_STATUS:
        printf("\t Device %d: Radio Tag = %d\n", id, status.notifyStatus);
        break;
    case IPOD_EVT_CAMERA_STATUS:
        printf("\t Device %d: Camera Status = %d\n", id, status.notifyStatus);
        break;
    case IPOD_EVT_CHARGING:
        printf("\t Device %d: Charging current = %d mA\n", id, status.availableCurrent);
        break;
    case IPOD_EVT_DB_CHANGED:
        printf("\t Device %d: Database changed\n", id);
        break;
    case IPOD_EVT_NOW_FOCUS_APP:
        printf("\t Device %d: Now Focus App = %s\n", id, status.focusApp);
        break;
    case IPOD_EVT_SESSION:
        printf("\t Device %d: Session ID = %d\n", id, status.sessionId);
        break;
    case IPOD_EVT_CMD_COMPLETE:
        memcpy(&commandID, &status.commandComplete[1], sizeof(commandID));
        printf("\t Device %d: CMD completed: Status 0x%x Lingo 0x%x, Cmd 0x%x\n",
               id, status.commandComplete[0], status.commandComplete[3], commandID);
        break;
    case IPOD_EVT_IPOD_OUT:
        printf("\t Device %d: iPod Out status = %d\n", id, status.notifyStatus);
        break;
    case IPOD_EVT_BT_STATUS:
        printf("\t Device %d: Bluetooth status received\n", id);
        break;
    case IPOD_EVT_APP_DISPLAY_NAME:
        printf("\t Device %d: App Display Name = %s\n", id, status.focusApp);
        break;
    case IPOD_EVT_ASSIST_TOUCH:
        printf("\t Device %d: Assisted Touch status = %d\n", id, status.notifyStatus);
        break;
    default:
        printf("\t Device %d: Unknown Notification 0x%x\n", id, type);
        break;
    }
}

void CBNotifyStatus(IPOD_CHANGED_PLAY_STATUS status, U64 param, const U32 id)
{
    switch(status)
    {
        case IPOD_STATUS_TRACK_POSITION:
        {
            printf("\t Device %d: TrackPosition = %llu \n", id, param);
            break;
        }
        case IPOD_STATUS_TRACK_CHANGED:
        {
            printf("\t Device %d: TrackChanged = %llu \n", id, param);
            break;
        }
        case IPOD_STATUS_TRACK_UID:
        {
            printf("\t Device %d: TrackUID = 0x%.16lx \n", id, (long unsigned int)param);
            break;
        }
        case IPOD_STATUS_TRACK_CAPABILITIES_CHANGED:
        {
            printf("\t Device %d: TrackCapabilitiesChanged = 0x%.16lx \n", id, (long unsigned int)param);
            break;
        }
        case IPOD_STATUS_PLAYBACK_CONTENT_CHANGED:
        {
            printf("\t Device %d: PlaybackContentChanged = %llu \n", id, param);
            break;
        }
        default:
        {
            printf("\t Device %d: Message = %d - %llu \n", id, status, param);
            break;
        }
    }
}
void TrackInfoCallback (IPOD_TRACK_INFO_TYPE infoType,
            const IPOD_TRACK_CAP_INFO_DATA* capInfoData,
            const IPOD_TRACK_RELEASE_DATE_DATA* releaseData,
            const IPOD_TRACK_ARTWORK_COUNT_DATA* artworkCountData,
            U8* stringBuf,
            const U32 id)
{
    U16 i = 0;

    switch(infoType)
    {
        case IPOD_TRACK_ARTWORK_COUNT:
        {
            printf("\t Device %d: Artwork info callback: \n", id);
            for(i = 0; i < iPods[id].formatCnt; i++)
            {
                printf("\t\t isLast: %d, formatID: %d, artworkCnt: %d \n",
                        artworkCountData->isLast, artworkCountData->id, artworkCountData->count);
                iPods[id].artworkData[i].artworkCnt = artworkCountData->count;
                iPods[id].artworkData[i].artworkFormatId = artworkCountData->id;
                /* get next track artwork data */
                artworkCountData++;
            }
            break;
        }
        case IPOD_TRACK_SONG_LYRICS:
        {
            printf("\t iDevice %d: Lyric info callback '%s' ", id, stringBuf);
            printf("\n");
            break;
        }
        case IPOD_TRACK_RELEASE_DATE:
        {
            printf("\t Device %d: Release date callback: %d", id, releaseData->year);
            printf("\n");
            break;
        }
        case IPOD_TRACK_CAP_INFO:
        {
            printf("\t Device %d: Capability info callback: %d", id, capInfoData->trackLength);
            printf("\n");
            break;
        }
        default:
        {
            printf("\t Device %d: Requested TrackInfo not implemented into smoketest callback", id);
            printf("\n");
            break;
        }
    }
}

#if (TEST_ARTWORK_FUNC || TEST_PERFORMANCE)
void GetArtworkCallback (IPOD_ALBUM_ARTWORK* artworkData, const U32 id)
{
  printf("\t Device %d: ArtworkCallback %d, %d, %d, %d, %d\n", id,
         artworkData->displayPixelFormatCode, artworkData->imageWidth, artworkData->imageHeight,
         artworkData->rowSize, artworkData->pixelDataLength);

  iPods[id].getArtwork = 1;
}
#endif

#if TEST_IOS_APP
S32 CBOpenApp(U8 protocolIndex, U16 sessionId , const U32 iPodID)
{
    S32 rc = IPOD_OK;

    printf("\t Device: %d CBOpenApp protIndex: %d SID: %d\n\n", iPodID, protocolIndex, sessionId);
    g_CBOpenApp = 1;
    return rc;
}

void CBCloseApp(U16 sessionId , const U32 iPodID)
{

    printf("\t Device: %d CBCloseApp SID: %d\n\n", iPodID, sessionId);
    g_CBOpenApp = 0;
}

S32 CBAppDataIn(U16 sessionId, U8 *data, U16 length , const U32 iPodID)
{
    S32 rc = IPOD_OK;

    if(data != NULL)
    {
        printf("\t Device: %d CBAppDataIn SID: %d length %d\n\n", iPodID, sessionId, length);
    }
    else
    {
        printf("\t Device: %d CBAppDataIn pointer data is NULL! \n", iPodID);
    }

    return rc;
}
#endif

/* ----------------------  test functions  ---------------------- */

/* TestPerformance(S32 iPodID);
 *  - Cover Art Download requires a playing track with cover art
 */
#if TEST_PERFORMANCE
/* test to check performance */
S32 TestPerformance(S32 iPodID);
S32 TestPerformance(S32 iPodID)
{
    S32 rc = IPOD_OK;
    S32 trackIndex = -1;
    U8 picName[256];
    U32 retry_count = 0;
    S32 format_id = 0;
    S32 format_width = 0;
    S32 format_height = 0;
    U16 i = 0;
    U16 artworkCount = 0;
    U32 artworkTime = 0;
    IPOD_ARTWORK_FORMAT artworkFormat[16];

    printf("\n PerformanceTest of device %d \n", iPodID);
    printf("\n *****  Part 1: Cover Art Download [Device %d] START  ***** \n", iPodID);

    sprintf((char*)(&picName[0]), "/mnt/perf_image_device%d.bmp", iPodID);
    iPodSetTrackArtworkDataImageSaveParams(iPodID, 1, &picName[0]);

    trackIndex = iPodGetCurrentPlayingTrackIndex(iPodID);
    printf("\n Part 1: Device %d: | TrackIndex = %d\n", iPodID, trackIndex);

    rc = iPodGetArtworkFormats(iPodID, artworkFormat, &artworkCount);
    printf("\n Part 1: Device %d: | artworkCount = %d\n", iPodID, artworkCount);

    iPods[iPodID].formatCnt = artworkCount;

    /* get artwort format ID */
    rc = iPodGetIndexedPlayingTrackInfo(iPodID, IPOD_TRACK_ARTWORK_COUNT,
                                        trackIndex,
                                        0,
                                        TrackInfoCallback);

    /* choose highest resolution from available artwork formats */
    for(i=0; i < artworkCount; i++)
    {
        printf("\t Artwork format: %d, %d, %d, %d\n",
            artworkFormat[i].formatID, artworkFormat[i].pixelFormat, artworkFormat[i].imageWidth, artworkFormat[i].imageHeight);
        if( (format_width < artworkFormat[i].imageWidth) && (format_height < artworkFormat[i].imageHeight) )
        {
            format_width = artworkFormat[i].imageWidth;
            format_height = artworkFormat[i].imageHeight;
            format_id = artworkFormat[i].formatID;
            printf("\t used format = %d  format_width = %d  format_height = %d\n", format_id, format_width, format_height);
        }
    }

    /* find formatId to get artwork count */
    for(i=0; i < iPods[iPodID].formatCnt; i++)
    {
        if(iPods[iPodID].artworkData[i].artworkFormatId == format_id)
        {
            break;
        }
    }

    /* check if there is a artwork available */
    if((iPods[iPodID].artworkData[i].artworkCnt > 0) && ((format_width > 0) && (format_height > 0)) )
    {
        rc = iPodGetTrackArtworkTimes(iPodID, trackIndex, format_id, 0, 1, &artworkCount, &artworkTime);
        printf("\n Part 1: Device %d: iPodGetTrackArtworkTimes rc = %d, trackIdnex = %d, ArtworkTime == %d, count == %d\n",
               iPodID, rc, trackIndex, artworkTime, artworkCount);

        iPods[iPodID].getArtwork = 0;
        rc = iPodGetTrackArtworkData(iPodID, trackIndex, format_id, artworkTime, GetArtworkCallback);
        printf("\n Part 1: Device %d: iPodGetTrackArtworkData rc = %d, trackIndex = %d\n", iPodID, rc, trackIndex);
        if(IPOD_OK == rc)
        {
            while((iPods[iPodID].getArtwork != 1) && (retry_count <= 100))
            {
                iPodOSSleep(50);
                retry_count++;
            }
            if(iPods[iPodID].getArtwork != 1)
            {
                printf("\n Part 1: Device %d: ... download artwork failed! No callback! retry: %d\n", iPodID, retry_count);
            }
            retry_count = 0;
        }
        iPods[iPodID].getArtwork = 0;
    }
    else if(iPods[iPodID].artworkData[i].artworkCnt == 0)
    {
        /* no artwork available */
        printf("\n Part 1: Device %d: ... no artwork available! artwork count: %d \n", iPodID, iPods[iPodID].artworkData[i].artworkCnt);
    }
    else
    {
        /* found no format */
        printf("\n Part 1: Device %d: ... found no format! format_width: %d  format_height: %d \n", iPodID, format_width, format_height);
    }

    printf("\n *****  TestPerformance Part 1 [Device %d] FINISH  ***** \n\n", iPodID);


    return rc;
}
#endif


#if TEST_ARTWORK_FUNC
S32 DownloadCoverArt(S32 iPodID, U8 *pathFileName, U16 formatCount, S32 limitWidth, S32 limitHeight, U32 trackIndex);
S32 DownloadCoverArt(S32 iPodID, U8 *pathFileName, U16 formatCount, S32 limitWidth, S32 limitHeight, U32 trackIndex)
{
    S32 rc = IPOD_OK;
    U32 retry_count = 0;
    U16 i = 0;
    U32 artworkTime = 0;
    U16 artworkCount = 0;
    IPOD_ARTWORK_FORMAT artworkFormat[16];
    S32 format_id = 0;
    S32 format_width = 0;
    S32 format_height = 0;

    /* get format count */
    if(formatCount > 0)
    {
        artworkCount = formatCount;
    }
    else
    {
        /* get supported artworkFormats from Apple device */
        rc = iPodGetArtworkFormats(iPodID, artworkFormat, &artworkCount);
        printf("\n Device %d: iPodGetArtworkFormats rc = %d, count = %d\n", iPodID, rc, artworkCount);
    }
    /* store number of available artworkFormats */
    iPods[iPodID].formatCnt = artworkCount;

    printf("\n Device %d: set image name %s \n", iPodID, pathFileName);
    iPodSetTrackArtworkDataImageSaveParams(iPodID, 1, pathFileName);

    /* find artworkFormat which shall be used */
    for(i=0; i < iPods[iPodID].formatCnt; i++)
    {
        /* check if their are limits */
        if((limitWidth > 0) && (limitHeight > 0))
        {
            if( ((artworkFormat[i].imageWidth <= limitWidth) && (artworkFormat[i].imageHeight <= limitHeight))
                && ((format_width < artworkFormat[i].imageWidth) && (format_height < artworkFormat[i].imageHeight)) )
            {
                format_width = artworkFormat[i].imageWidth;
                format_height = artworkFormat[i].imageHeight;
                format_id = artworkFormat[i].formatID;
                printf("\t used format = %d  format_width = %d  format_height = %d\n", format_id, format_width, format_height);
                break;
            }
        }
        else
        {
            if( (format_width < artworkFormat[i].imageWidth) && (format_height < artworkFormat[i].imageHeight) )
            {
                format_width = artworkFormat[i].imageWidth;
                format_height = artworkFormat[i].imageHeight;
                format_id = artworkFormat[i].formatID;
                printf("\t used format = %d  format_width = %d  format_height = %d\n", format_id, format_width, format_height);
            }
        }
    }

    /* check if there is a artwork available */
    if( (IPOD_OK == rc) && ((iPods[iPodID].artworkData[i].artworkCnt > 0) && ((format_width > 0) && (format_height > 0))) )
    {
        /* get list of artwork time locations for a track */
        rc = iPodGetTrackArtworkTimes(iPodID, trackIndex, format_id, 0, 1, &artworkCount, &artworkTime);
        printf("\n Device %d: iPodGetTrackArtworkTimes rc = %d, trackIdnex = %d, ArtworkTime == %d, count == %d\n",
               iPodID, rc, trackIndex, artworkTime, artworkCount);

        iPods[iPodID].getArtwork = 0;
        rc = iPodGetTrackArtworkData(iPodID, trackIndex, format_id, artworkTime, GetArtworkCallback);
        printf("\n Device %d: iPodGetTrackArtworkData rc = %d, trackIndex = %d\n", iPodID, rc, trackIndex);
        if(IPOD_OK == rc)
        {
            while((iPods[iPodID].getArtwork != 1) && (retry_count <= 200))
            {
                iPodOSSleep(50);
                retry_count++;
            }
            if(iPods[iPodID].getArtwork != 1)
            {
                printf("\n Device %d: ... download artwork failed! No callback! retry: %d\n", iPodID, retry_count);
                rc = IPOD_ERROR;
            }
            retry_count = 0;
        }
        iPods[iPodID].getArtwork = 0;
    }

    return rc;
}
#endif

#if TEST_ARTWORK_FUNC
/* test cover art & artwork functions */
S32 TestArtwork(S32 iPodID);
S32 TestArtwork(S32 iPodID)
{
    S32 rc = IPOD_OK;
    U32 retry_count = 0;
    U32 failCnt = 0;
    S32 format_id = 0;
    S32 format_width = 0;
    S32 format_height = 0;
    U16 i = 0;
    U16 j = 0;
    U16 artworkCount = 0;
    U32 artworkTime = 0;
    IPOD_ARTWORK_FORMAT artworkFormat[16];
    U16 displayLimitsCount = 0;
    IPOD_DISPLAY_IMAGE_LIMITS resultBuf[16];
    S32 trackIndex = -1;
    U8 downloadImageName[256];
    U8 uploadImageName[256];

    printf("\n Device %d: down- and upload artwork\n", iPodID);

    sprintf((char*)(&downloadImageName[0]), "/mnt/device%d_downloadImage.bmp", iPodID);
    iPodSetTrackArtworkDataImageSaveParams(iPodID, 1, &downloadImageName[0]);

    trackIndex = iPodGetCurrentPlayingTrackIndex(iPodID);
    printf("\n Device %d: iPodGetCurrentPlayingTrackIndex rc = %d\n", iPodID, trackIndex);

    rc = iPodGetArtworkFormats(iPodID, artworkFormat, &artworkCount);
    printf("\n Device %d: iPodGetArtworkFormats rc = %d, count = %d\n", iPodID, rc, artworkCount);
    if(IPOD_OK != rc)
        failCnt++;
    iPods[iPodID].formatCnt = artworkCount;

    /* get number of available artworks inside the track */
    rc = iPodGetIndexedPlayingTrackInfo(iPodID, IPOD_TRACK_ARTWORK_COUNT,
                                        trackIndex,
                                        0,
                                        TrackInfoCallback);
    printf("\n Device %d: iPodGetIndexedPlayingTrackInfo rc = %d\n", iPodID, rc);
    if(IPOD_OK != rc)
        failCnt++;

    rc = iPodGetColorDisplayImageLimits(iPodID, resultBuf, &displayLimitsCount);
    printf("\n Device %d: iPodGetColorDisplayImageLimits rc = %d, count = %d\n", iPodID, rc, displayLimitsCount);
    if(IPOD_OK != rc)
        failCnt++;

    for(j=0; j < displayLimitsCount; j++)
    {
        for(i=0; i < artworkCount; i++)
        {
            printf("\t Artwork format: %d, %d, %d, %d\n",
                artworkFormat[i].formatID, artworkFormat[i].pixelFormat, artworkFormat[i].imageWidth, artworkFormat[i].imageHeight);
            if( (format_width < artworkFormat[i].imageWidth) && (format_height < artworkFormat[i].imageHeight) )
            {
                format_width = artworkFormat[i].imageWidth;
                format_height = artworkFormat[i].imageHeight;
                format_id = artworkFormat[i].formatID;
                printf("\t download used format = %d  format_width = %d  format_height = %d\n", format_id, format_width, format_height);
            }
        }
        if(format_id != 0)
            break;
    }
    /* find formatId to get artwork count */
    for(i=0; i < iPods[iPodID].formatCnt; i++)
    {
        if(iPods[iPodID].artworkData[i].artworkFormatId == format_id)
        {
            break;
        }
    }

    /* check if there is a artwork available */
    if((iPods[iPodID].artworkData[i].artworkCnt > 0) && ((format_width > 0) && (format_height > 0)) )
    {
        /* download image from Apple device with max resolution */
        rc = iPodGetTrackArtworkTimes(iPodID, trackIndex, format_id, 0, 1, &artworkCount, &artworkTime);
        printf("\n Device %d: iPodGetTrackArtworkTimes rc = %d, trackIdnex = %d, ArtworkTime == %d, count == %d\n",
               iPodID, rc, trackIndex, artworkTime, artworkCount);
        if(IPOD_OK != rc)
            failCnt++;

        iPods[iPodID].getArtwork = 0;
        rc = iPodGetTrackArtworkData(iPodID, trackIndex, format_id, artworkTime, GetArtworkCallback);
        printf("\n Device %d: iPodGetTrackArtworkData rc = %d, trackIndex = %d\n", iPodID, rc, trackIndex);
        if(IPOD_OK != rc)
            failCnt++;
        else
        {
            while((iPods[iPodID].getArtwork != 1) && (retry_count <= 200))
            {
                iPodOSSleep(50);
                retry_count++;
            }
            if(iPods[iPodID].getArtwork != 1)
            {
                printf("\n Device %d: ... download artwork failed! No callback! retry: %d\n", iPodID, retry_count);
            }
            retry_count = 0;
        }
        iPods[iPodID].getArtwork = 0;

        /* upload image to Apple device */
        displayLimitsCount = 0;
        /* get display limits */
        rc = iPodGetColorDisplayImageLimits(iPodID, resultBuf, &displayLimitsCount);
        printf("\n Device %d: iPodGetColorDisplayImageLimits = %d  count = %d\n", iPodID, rc, displayLimitsCount);
        if(IPOD_OK != rc)
            failCnt++;
        for(i=0; i < displayLimitsCount; i++)
        {
            printf("\t DisplayLimits: %d %d %d\n", resultBuf[i].width, resultBuf[i].height, resultBuf[i].pixelFormat);
        }
        format_width = 0;
        format_height = 0;
        format_id = 0;
        /* download image with  correct format from Apple device himself */
        for(j=0; j < displayLimitsCount; j++)
        {
            for(i=0; i < iPods[iPodID].formatCnt; i++)
            {
                if( ((artworkFormat[i].imageWidth <= resultBuf[j].width) && (artworkFormat[i].imageHeight <= resultBuf[j].height))
                    && ((format_width < artworkFormat[i].imageWidth) && (format_height < artworkFormat[i].imageHeight)) )
                {
                    format_width = artworkFormat[i].imageWidth;
                    format_height = artworkFormat[i].imageHeight;
                    format_id = artworkFormat[i].formatID;
                    printf("\t upload used format = %d  format_width = %d  format_height = %d\n", format_id, format_width, format_height);
                    break;
                }
            }
            if(format_id != 0)
                break;
        }

        sprintf((char*)(&uploadImageName[0]), "/mnt/device%d_uploadImage.bmp", iPodID);
        iPodSetTrackArtworkDataImageSaveParams(iPodID, 1, &uploadImageName[0]);
        /* download image which should be uploaded */
        iPods[iPodID].getArtwork = 0;
        rc = iPodGetTrackArtworkData(iPodID, trackIndex, format_id, artworkTime, GetArtworkCallback);
        printf("\n Device %d: iPodGetTrackArtworkData rc = %d, trackIndex = %d\n", iPodID, rc, trackIndex);
        if(IPOD_OK != rc)
            failCnt++;
        else
        {
            while((iPods[iPodID].getArtwork != 1) && (retry_count <= 200))
            {
                iPodOSSleep(50);
                retry_count++;
            }
            if(iPods[iPodID].getArtwork != 1)
            {
                printf("\n Device %d: ... download artwork failed! No callback! retry: %d\n", iPodID, retry_count);
            }
            retry_count = 0;
        }
        iPods[iPodID].getArtwork = 0;

//        rc = iPodSetDisplayImageBMP(iPodID, &uploadImageName[0]);
//        printf("\n Device %d: iPodSetDisplayImageBMP rc = %d\n", iPodID, rc);
//        if(IPOD_OK != rc)
//            failCnt++;
    }
    else if(iPods[iPodID].artworkData[i].artworkCnt == 0)
    {
        /* no artwork available */
        printf("\n Device %d: ... no artwork available! artwork count: %d \n", iPodID, iPods[iPodID].artworkData[i].artworkCnt);
    }
    else
    {
        /* found no format */
        printf("\n Device %d: ... found no format! format_width: %d  format_height: %d \n", iPodID, format_width, format_height);
    }

    printf("\n *****  TestArtwork [Device %d | fail: %d]  ***** \n", iPodID, failCnt);
    return failCnt;
}
#endif

#if SUBTEST_AUDIO_DB
S32 SubTestDBinfoAudioBrowsing(S32 iPodID);
S32 SubTestDBinfoAudioBrowsing(S32 iPodID)

{
    S32 ret = IPOD_OK;
    U32 failCnt = 0;

    printf(" ***** Audio browsing ***** \n");

    (void)iPodResetDBSelection(iPodID);
    ret = iPodGetNumberCategorizedDBRecords(iPodID, IPOD_CAT_NESTED_PLAYLIST);
    printf(" Device %d: iPodGetNumberCategorizedDBRecords(IPOD_CAT_NESTED_PLAYLIST) rc = %d\n\n", iPodID, ret);
    if(ret > IPOD_OK)
    {
        ret= iPodRetrieveCategorizedDBRecords(iPodID, IPOD_CAT_NESTED_PLAYLIST, 0, ret, retrieve_DB_record);
        printf(" Device %d: iPodRetrieveCategorizedDBRecords(IPOD_CAT_NESTED_PLAYLIST) rc = %d\n\n", iPodID, ret);
    }
    if(IPOD_OK != ret)
        failCnt++;

    (void)iPodResetDBSelection(iPodID);
    ret = iPodGetNumberCategorizedDBRecords(iPodID, IPOD_CAT_TOPLIST);
    printf(" Device %d: iPodGetNumberCategorizedDBRecords(IPOD_CAT_TOPLIST) rc = %d\n\n", iPodID, ret);
    if(ret > IPOD_OK)
    {
        ret= iPodRetrieveCategorizedDBRecords(iPodID, IPOD_CAT_TOPLIST, 0, ret, retrieve_DB_record);
        printf(" Device %d: iPodRetrieveCategorizedDBRecords(IPOD_CAT_TOPLIST) rc = %d\n\n", iPodID, ret);
    }
    if(IPOD_OK != ret)
        failCnt++;

    (void)iPodResetDBSelection(iPodID);
    ret = iPodGetNumberCategorizedDBRecords(iPodID, IPOD_CAT_GENIUS);
    printf(" Device %d: iPodGetNumberCategorizedDBRecords(IPOD_CAT_GENIUS) rc = %d\n\n", iPodID, ret);
    if(ret > IPOD_OK)
    {
        ret= iPodRetrieveCategorizedDBRecords(iPodID, IPOD_CAT_GENIUS, 0, ret, retrieve_DB_record);
        printf(" Device %d: iPodRetrieveCategorizedDBRecords(IPOD_CAT_GENIUS) rc = %d\n\n", iPodID, ret);
    }
    if(IPOD_OK != ret)
        failCnt++;

    printf("\n *****  SubTestDBinfoAudioBrowsing [Device %d | fail: %d]  ***** \n\n", iPodID, failCnt);

    return failCnt;
}
#endif

#if SUBTEST_VIDEO_DB
S32 SubTestDBinfoVideoBrowsing(S32 iPodID);
S32 SubTestDBinfoVideoBrowsing(S32 iPodID)
{
    S32 ret = IPOD_OK;
    U32 failCnt = 0;

    printf("\n ***** Video browsing ***** \n");

    ret = iPodResetDBSelection(iPodID);
    printf(" Device %d: iPodResetDBSelection rc = %d\n", iPodID, ret);

    /* select video hierarchy */
    ret = iPodResetDBSelectionHierarchy(iPodID, 0x02);
    printf(" Device %d: iPodResetDBSelectionHierarchy(video) rc = %d\n", iPodID, ret);
    if(ret == IPOD_OK)
    {
        ret = iPodGetNumberCategorizedDBRecords(iPodID, IPOD_CAT_NESTED_PLAYLIST);
        printf(" Device %d: iPodGetNumberCategorizedDBRecords(IPOD_CAT_NESTED_PLAYLIST) rc = %d\n", iPodID, ret);
        if(ret > IPOD_OK)
        {
            ret= iPodRetrieveCategorizedDBRecords(iPodID, IPOD_CAT_NESTED_PLAYLIST, 0, ret, retrieve_DB_record);
            printf(" Device %d: iPodRetrieveCategorizedDBRecords(IPOD_CAT_NESTED_PLAYLIST) rc = %d\n", iPodID, ret);
        }
        if(IPOD_OK != ret)
            failCnt++;

        ret = iPodGetNumberCategorizedDBRecords(iPodID, IPOD_CAT_GENRE);
        printf(" Device %d: iPodGetNumberCategorizedDBRecords(IPOD_CAT_GENRE) rc = %d\n", iPodID, ret);
        if(ret > IPOD_OK)
        {
            ret= iPodRetrieveCategorizedDBRecords(iPodID, IPOD_CAT_GENRE, 0, ret, retrieve_DB_record);
            printf(" Device %d: iPodRetrieveCategorizedDBRecords rc = %d\n", iPodID, ret);
            if(IPOD_OK != ret)
                failCnt++;

            /* select index 0 (movies) */
            ret = iPodSelectDBRecord(iPodID, IPOD_CAT_GENRE, 0x00);
            printf(" Device %d: iPodSelectDBRecord(IPOD_CAT_GENRE) rc = %d\n", iPodID, ret);
            if(IPOD_OK != ret)
                failCnt++;

            ret = iPodGetNumberCategorizedDBRecords(iPodID, IPOD_CAT_ARTIST);
            printf(" Device %d: iPodGetNumberCategorizedDBRecords(IPOD_CAT_ARTIST) rc = %d\n", iPodID, ret);
            if(ret > IPOD_OK)
            {
                ret= iPodRetrieveCategorizedDBRecords(iPodID, IPOD_CAT_ARTIST, 0, ret, retrieve_DB_record);
                printf(" Device %d: iPodRetrieveCategorizedDBRecords rc = %d\n", iPodID, ret);
                if(IPOD_OK != ret)
                    failCnt++;

                /* select first video */
                ret = iPodSelectDBRecord(iPodID, IPOD_CAT_ARTIST, 0x00);
                printf(" Device %d: iPodSelectDBRecord(IPOD_CAT_ARTIST) rc = %d\n", iPodID, ret);
                if(IPOD_OK != ret)
                    failCnt++;

                ret = iPodGetNumberCategorizedDBRecords(iPodID, IPOD_CAT_ALBUM);
                printf(" Device %d: iPodGetNumberCategorizedDBRecords(IPOD_CAT_ALBUM) rc = %d\n", iPodID, ret);
                if(ret > IPOD_OK)
                {
                    ret = iPodRetrieveCategorizedDBRecords(iPodID, IPOD_CAT_ALBUM, 0, ret, retrieve_DB_record);
                    printf(" Device %d: iPodRetrieveCategorizedDBRecords rc = %d\n", iPodID, ret);
                    if(IPOD_OK != ret)
                        failCnt++;

                    ret = iPodSelectDBRecord(iPodID, IPOD_CAT_ALBUM, 0x00);
                    printf(" Device %d: iPodSelectDBRecord(IPOD_CAT_ALBUM) rc = %d\n", iPodID, ret);
                    if(IPOD_OK != ret)
                        failCnt++;

                    ret = iPodGetNumberCategorizedDBRecords(iPodID, IPOD_CAT_TRACK);
                    printf(" Device %d: iPodGetNumberCategorizedDBRecords(IPOD_CAT_TRACK) rc = %d\n", iPodID, ret);
                    if(ret > IPOD_OK)
                    {
                        ret = iPodRetrieveCategorizedDBRecords(iPodID, IPOD_CAT_TRACK, 0, ret, retrieve_DB_record);
                        printf(" Device %d: iPodRetrieveCategorizedDBRecords rc = %d\n", iPodID, ret);
                        if(IPOD_OK != ret)
                            failCnt++;
                    }
                    else
                    {
                        failCnt++;
                    }
                }
                else
                {
                    ret = iPodGetNumberCategorizedDBRecords(iPodID, IPOD_CAT_TRACK);
                    printf(" Device %d: iPodGetNumberCategorizedDBRecords(IPOD_CAT_TRACK) rc = %d\n", iPodID, ret);
                    if(ret > IPOD_OK)
                    {
                        ret = iPodRetrieveCategorizedDBRecords(iPodID, IPOD_CAT_TRACK, 0, ret, retrieve_DB_record);
                        printf(" Device %d: iPodRetrieveCategorizedDBRecords rc = %d\n", iPodID, ret);
                        if(IPOD_OK != ret)
                            failCnt++;
                    }
                    else
                    {
                        failCnt++;
                    }
                }
            }
            else
            {
                ret = iPodResetDBSelectionHierarchy(iPodID, 0x02);
                printf(" Device %d: iPodResetDBSelectionHierarchy(video) rc = %d\n", iPodID, ret);
                if(IPOD_OK != ret)
                    failCnt++;

                ret = iPodGetNumberCategorizedDBRecords(iPodID, IPOD_CAT_TRACK);
                printf(" Device %d: iPodGetNumberCategorizedDBRecords(IPOD_CAT_TRACK) rc = %d\n", iPodID, ret);
                if(ret > IPOD_OK)
                {
                    ret= iPodRetrieveCategorizedDBRecords(iPodID, IPOD_CAT_TRACK, 0, ret, retrieve_DB_record);
                    printf(" Device %d: iPodRetrieveCategorizedDBRecords rc = %d\n", iPodID, ret);
                    if(IPOD_OK != ret)
                        failCnt++;
                }
                else
                {
                    failCnt++;
                }
            }
        }
        else
        {
            ret = iPodResetDBSelectionHierarchy(iPodID, 0x02);
            printf(" Device %d: iPodResetDBSelectionHierarchy(video) rc = %d\n", iPodID, ret);
            if(IPOD_OK != ret)
                failCnt++;

            ret = iPodGetNumberCategorizedDBRecords(iPodID, IPOD_CAT_TRACK);
            printf(" Device %d: iPodGetNumberCategorizedDBRecords(IPOD_CAT_TRACK) rc = %d\n", iPodID, ret);
            if(ret > IPOD_OK)
            {
                ret= iPodRetrieveCategorizedDBRecords(iPodID, IPOD_CAT_TRACK, 0, ret, retrieve_DB_record);
                printf(" Device %d: iPodRetrieveCategorizedDBRecords rc = %d\n", iPodID, ret);
                if(IPOD_OK != ret)
                    failCnt++;
            }
            else
            {
                failCnt++;
            }
        }
    }

    printf("\n *****  SubTestDBinfoVideoBrowsing [Device %d | fail: %d]  ***** \n\n", iPodID, failCnt);

    return failCnt;
}
#endif

#if TEST_DB_FUNC
/* test basic DB track info functions */
S32 TestDBinfo(S32 iPodID);
S32 TestDBinfo(S32 iPodID)
{
    S32 ret = IPOD_OK;
    U32 failCnt = 0;
    IPOD_TRACK_INFORMATION_TYPE i = 0;
    S32 trackCount = 0;

    ret = iPodResetDBSelection(iPodID);
    printf(" Device %d: iPodResetDBSelection rc = %d\n", iPodID, ret);
    if(IPOD_OK != ret)
        failCnt++;

    /* ***  DBTrackInfo *** */
    trackCount = iPodGetNumberCategorizedDBRecords(iPodID, IPOD_CAT_TRACK);
    printf(" Device %d: iPodGetNumberCategorizedDBRecords(IPOD_CAT_TRACK) rc = %d\n", iPodID, trackCount);
    if(IPOD_OK > ret)
        failCnt++;

    ret= iPodRetrieveCategorizedDBRecords(iPodID, IPOD_CAT_TRACK, 0, 10, retrieve_DB_record);
    printf(" Device %d: iPodRetrieveCategorizedDBRecords rc = %d\n", iPodID, ret);
    if(IPOD_OK != ret)
        failCnt++;

    /* allocate array to store track UIDs */
    iPods[iPodID].arrayUID = calloc(trackCount, sizeof(U64));
    iPods[iPodID].countUID = trackCount;
    if(trackCount > 20)
        trackCount = 20;

    ret = iPodGetDBTrackInfo(iPodID, 0, trackCount, UID, track_info_cb);
    printf(" Device %d: iPodGetDBTrackInfo rc = %d\n", iPodID, ret);
    if(IPOD_OK != ret)
        failCnt++;

    for (i = CAPABILITIES;
            (i <= START_AND_STOP_OFFSET) && (ret == IPOD_OK); i++)
    {
        ret = iPodGetDBTrackInfo(iPodID, 0, 1, i, track_info_cb);
        if(IPOD_OK != ret)
        {
            printf(" Device %d: iPodGetDBTrackInfo of %d rc = %d\n", iPodID, 0, ret);
            failCnt++;
        }
    }
    printf(" Device %d: DBtrackInfoCBcnt = %d MaxCBcnt = %d \n",
            iPodID, iPods[iPodID].CBcount, (U32)START_AND_STOP_OFFSET);

    /* ***  UIDTrackInfo  *** */
    if((iPods[iPodID].CBcount > 0) && (*(iPods[iPodID].arrayUID+8) != 0))
    {
        ret = iPodGetUIDTrackInfo(iPodID, *(iPods[iPodID].arrayUID+8), CHAPTER_COUNT, track_info_cb);
        if(IPOD_OK != ret)
            failCnt++;
        printf(" Device %d: iPodGetUIDTrackInfo rc = %d\n", iPodID, ret);
    }

    /* ***  PBTrackInfo *** */
    trackCount = iPodGetNumPlayingTracks(iPodID);
    printf(" Device %d: iPodGetNumPlayingTracks rc = %d\n", iPodID, trackCount);
    if(IPOD_OK > ret)
        failCnt++;

    iPods[iPodID].CBcount = 0;
    for (i = CAPABILITIES;
            (i <= START_AND_STOP_OFFSET) && (ret == IPOD_OK); i++)
    {
        ret = iPodGetPBTrackInfo(iPodID, 0, 1, i, track_info_cb);
        if(IPOD_OK != ret)
        {
            printf(" Device %d: iPodGetPBTrackInfo of %d rc = %d\n", iPodID, 0, ret);
            failCnt++;
        }
    }
    printf(" Device %d: PBtrackInfoCBcnt = %d MaxCBcnt = %d \n",
            iPodID, iPods[iPodID].CBcount, (U32)START_AND_STOP_OFFSET);


#if SUBTEST_AUDIO_DB
    /* audio browsing */
    ret = SubTestDBinfoAudioBrowsing(iPodID);
    failCnt += ret;
#endif

#if SUBTEST_VIDEO_DB
    /* video browsing */
    ret = SubTestDBinfoVideoBrowsing(iPodID);
    failCnt += ret;
#endif

    /* switch back to audio hierarchy */
    (void)iPodResetDBSelectionHierarchy(iPodID, 0x01);


    printf("\n *****  TestDBinfo [Device %d | fail: %d]  ***** \n\n", iPodID, failCnt);
    return failCnt;
}
#endif

#if TEST_EXP_DB_FUNC
/* test bulk DB track info functions */
S32 TestBulkDBinfo(S32 iPodID, IPOD_TRACK_INFORMATION_BITFIELD bitfield);
S32 TestBulkDBinfo(S32 iPodID, IPOD_TRACK_INFORMATION_BITFIELD bitfield)
{
    S32 rc = IPOD_OK;
    U32 failCnt = 0;

    /* status change notification must be enabled. otherwise we get an ATS error */
    (void)iPodSetPlayStatusChangeNotification(iPodID, IPOD_STATUS_CHANGE_NOTIFICATION_ENABLE);
    /* iPod shall playing for further tests like iPodGetBulkPBTrackInfo() */
    (void)iPodGetPlayStatus(iPodID, &iPods[iPodID].playState, &iPods[iPodID].length, &iPods[iPodID].position);
    if(iPods[iPodID].playState != IPOD_PLAYER_STATE_PLAYING)
    {
        (void)iPodPlayToggle(iPodID);
        iPodOSSleep(3000);
    }

    rc = iPodGetBulkDBTrackInfo(iPodID, 35, 10, bitfield, track_info_cb);
    printf(" Device %d: iPodGetBulkDBTrackInfo rc = %d\n\n", iPodID, rc);
    if(IPOD_OK != rc)
        failCnt++;

    rc = iPodGetBulkPBTrackInfo(iPodID, 0, 1, bitfield, track_info_cb);
    printf(" Device %d: iPodGetBulkPBTrackInfo rc = %d\n\n", iPodID, rc);
    if(IPOD_OK != rc)
        failCnt++;

    rc = iPodGetBulkUIDTrackInfo(iPodID, *(iPods[iPodID].arrayUID+4), bitfield, track_info_cb);
    printf(" Device %d: iPodGetBulkUIDTrackInfo rc = %d\n", iPodID, rc);
    if(IPOD_OK != rc)
        failCnt++;

    printf("\n *****  TestBulkDBinfo [Device %d | fail: %d]  ***** \n\n", iPodID, failCnt);
    return failCnt;
}
#endif

#if TEST_UID_LIST_FUNC
/* test prepare UID list */
S32 TestUIDlist(S32 iPodID, IPOD_TRACK_INFORMATION_BITFIELD bitfield);
S32 TestUIDlist(S32 iPodID, IPOD_TRACK_INFORMATION_BITFIELD bitfield)
{
    S32 rc = IPOD_OK;
    U32 failCnt = 0;

    /*// that's the way how it should work in the future
    U32 arrayIndexUID = iPods[id].countUID - 1;
    rc = iPodPrepareUIDList(iPods[id].arrayUID, iPods[id].countUID);
    rc = iPodPlayPreparedUIDList(iPods[id].arrayUID[arrayIndexUID]);
      // but currently we support only one track in the UID List.
      // therefore ...
    */
    /* set first entry to the UID list */
    rc = iPodPrepareUIDList(iPodID, (iPods[iPodID].arrayUID+8), 1);
    printf(" Device %d: iPodPrepareUIDList rc = %d\n\n", iPodID, rc);
    if(IPOD_OK != rc)
        failCnt++;

    /* start playback of the respective UID track */
    rc = iPodPlayPreparedUIDList(iPodID, *(iPods[iPodID].arrayUID+8));
    printf(" Device %d: iPodPlayPreparedUIDList rc = %d\n\n", iPodID, rc);
    if(IPOD_OK != rc)
        failCnt++;

    if(IPOD_OK == rc)
    {
        rc = iPodGetBulkPBTrackInfo(iPodID, 0, 1, bitfield, track_info_cb);
        printf(" Device %d: Get trackInfo of prepared UID list rc = %d\n", iPodID, rc);
    }

    printf("\n *****  TestUIDlist [Device %d | fail: %d]  ***** \n\n", iPodID, failCnt);
    return failCnt;
}
#endif

#if (TEST_BASICS || TEST_PERFORMANCE)
/* get serial number, name, set & get event notifications */
S32 TestBasic(S32 iPodID);
S32 TestBasic(S32 iPodID)
{
    S32 ret = IPOD_OK;
    U32 failCnt = 0;
    S32 trackCount = 0;
    /* 0x1AAE3C   - enable all bits except reserved bits (specification confirm) */
    /* 0xFFFFFFFF - enable all bits, but leads to an ATS error (Nano 7G, Touch 4G, iPhone 5G */
    U64 notificationMask = (U64)0x1AAE3C;
    U8 iPodName[IPOD_RESPONSE_BUF_SIZE];

    ret = iPodGetSerialNumber(iPodID, iPodName);
    if(ret != 0)
    {
        printf(" Device %d: SerialNumber FAILED %d \n", iPodID, ret);
        failCnt++;
    }
    else
    {
        printf(" Device %d: Serial Number: %d %s \n", iPodID, ret, iPodName);
    }

    ret = iPodGetIPodName(iPodID, iPodName);
    if(ret != IPOD_OK)
    {
        printf(" Device %d: GetName FAILED %d \n", iPodID, ret);
        failCnt++;
    }
    else
    {
        printf(" Device %d: GetName %s \n", iPodID, iPodName);
    }

    ret = iPodSetEventNotification(iPodID, notificationMask);
    if(ret != IPOD_OK)
    {
        printf(" Device %d: iPodSetEventNotification FAILED %d \n", iPodID, ret);
        failCnt++;
    }
    else
    {
        printf(" Device %d: iPodSetEventNotification %x \n", iPodID, (U32)notificationMask);
    }

    notificationMask = 0;
    ret = iPodGetEventNotification(iPodID, &notificationMask);
    if(ret != IPOD_OK)
    {
        printf(" Device %d: iPodGetEventNotification FAILED %d \n", iPodID, ret);
        failCnt++;
    }
    else
    {
        printf(" Device %d: iPodGetEventNotification %x \n", iPodID, (U32)notificationMask);
    }


    U32 remoteEventNotifyMask = 0;
    /* activate shuffle and repeat remote event notification */
    remoteEventNotifyMask = (U32)0x180;
    /* activate all supported remote event notifications */
//    remoteEventNotifyMask = (U32)0x6FFEF;
    ret = iPodSetRemoteEventNotification(iPodID, remoteEventNotifyMask);
    if(ret != IPOD_OK) {
        printf(" Device %d: iPodSetRemoteEventNotification FAILED %d \n", iPodID, ret);
        failCnt++;
    } else{
        printf(" Device %d: iPodSetRemoteEventNotification %x \n", iPodID, (U32)remoteEventNotifyMask);
    }


    /* ***  basic database track info test *** */
    ret = iPodEnterExtendedInterfaceMode(iPodID);
    printf(" Device %d: iPodEnterExtendedInterfaceMode rc = %d\n\n", iPodID, ret);
    if(IPOD_OK != ret)
        failCnt++;

    ret = iPodResetDBSelection(iPodID);
    printf(" Device %d: iPodResetDBSelection rc = %d\n", iPodID, ret);
    if(IPOD_OK != ret)
        failCnt++;

    trackCount = iPodGetNumberCategorizedDBRecords(iPodID, IPOD_CAT_TRACK);
    printf(" Device %d: iPodGetNumberCategorizedDBRecords(IPOD_CAT_TRACK) rc = %d\n", iPodID, trackCount);
    if(IPOD_OK > ret)
        failCnt++;

    ret= iPodRetrieveCategorizedDBRecords(iPodID, IPOD_CAT_TRACK, 0, 10, retrieve_DB_record);
    printf(" Device %d: iPodRetrieveCategorizedDBRecords rc = %d\n", iPodID, ret);
    if(IPOD_OK != ret)
        failCnt++;

    ret = iPodEnterSimpleMode(iPodID);
    printf(" Device %d: iPodEnterSimpleMode rc = %d\n", iPodID, ret);
    if(IPOD_OK != ret)
        failCnt++;


    printf("\n *****  TestBasic [Device %d | fail: %d]  ***** \n\n", iPodID, failCnt);
    return failCnt;
}
#endif

#if TEST_PLAYBACK_FUNC
/* test some playback functions of an iPod */
S32 TestPlayback(S32 iPodID);
S32 TestPlayback(S32 iPodID)
{
    S32 ret = IPOD_OK;
    U32 failCnt = 0;
    S32 shuffleMode = -1;
    S32 repeatMode = -1;

    ret = iPodSetPlayStatusChangeNotification(iPodID, IPOD_STATUS_CHANGE_NOTIFICATION_ENABLE);
    printf(" Device %d: iPodSetStatusNotification rc = %d\n", iPodID, ret);
    if(ret != IPOD_OK)
        failCnt++;

    ret = iPodGetPlayStatus(iPodID, &iPods[iPodID].playState, &iPods[iPodID].length, &iPods[iPodID].position);
    printf(" Device %d: iPodGetPlayStatus(%d) rc = %d\n", iPodID, iPods[iPodID].playState, ret);
    if(ret == IPOD_OK)
    {
        switch(iPods[iPodID].playState)
        {
            case IPOD_PLAYER_STATE_STOPPED:
            {/* playback stopped, start playback */
                ret = iPodPlayControl(iPodID, PLAY);
                printf(" Device %d: iPodPlayControl rc = %d\n", iPodID, ret);
                break;
            }
            case IPOD_PLAYER_STATE_PAUSED:
            {/* playback paused, toggle playback necessary */
                ret = iPodPlayToggle(iPodID);
                printf(" Device %d: iPodPlayToggle rc = %d\n", iPodID, ret);
                break;
            }
            case IPOD_PLAYER_STATE_PLAYING:
            {/* already in play state */
                break;
            }
            default:
            {
                printf(" Device %d: Unexpected state %d \n", iPodID, iPods[iPodID].playState);
                break;
            }
        }
        iPodOSSleep(3000);
    }

    /* check current shuffle mode and repeat mode */
    shuffleMode = iPodGetShuffleMode(iPodID);
    printf(" Device %d:  iPodGetShuffleMode = %x \n", iPodID, shuffleMode);
    repeatMode = iPodGetRepeatMode(iPodID);
    printf(" Device %d:  iPodGetRepeatMode = %x \n", iPodID, repeatMode);

    /* SetTrackPosition will fail if Track does not already play */
    ret = iPodSetTrackPosition(iPodID, 10000);
    printf(" Device %d: iPodSetTrackPosition rc = %d\n", iPodID, ret);
    if(IPOD_OK != ret)
        failCnt++;

    /* let the track play and start some interactions on the Apple device */
    iPodOSSleep(TEST_PLAYBACK_TIME_MS);

    ret = iPodPlayToggle(iPodID);
    printf(" Device %d: iPod PlayToggle rc = %d\n", iPodID, ret);
    if(ret != IPOD_OK)
        failCnt++;

    printf("\n *****  TestPlayback [Device %d | fail: %d]  ***** \n\n", iPodID, failCnt);
    return failCnt;
}
#endif

/* de-initialize & authenticate an iPod */
S32 DisconnectDevice(S32 iPodID);
S32 DisconnectDevice(S32 iPodID)
{
    S32 rc = IPOD_OK;
    U32 retry_count = 0;

    if(iPodID > 0)
    {
        /* de-initialize iPods */
        printf("  iPodDisconnectDevice: %d\n", iPodID);
        iPodDisconnectDevice(iPods[iPodID].iPodID);
        /* wait for detach event */
        retry_count = 0;
        while((iPods[iPodID].connected != IPOD_DETACHED) && (retry_count <= 80))
        {
            iPodOSSleep(50);
            retry_count++;
        }
        /* reset ipod-ctrl iPodID for the specific iPod */
        if(iPods[iPodID].connected != IPOD_DETACHED)
        {
            printf("   disconnectDevice %d failed! retry: %d\n", iPodID, retry_count);
            rc = IPOD_ERROR;
        }
        else
        {
            printf("   disconnectDevice %d success! retry: %d\n", iPodID, retry_count);
            rc = IPOD_OK;
        }
        iPods[iPodID].connected = IPOD_NOT_DETECTED;
    }
    else
    {
        rc = IPOD_ERROR;
    }

    return rc;
}

/* initialize & authenticate an iPod */
S32 ConnectDevice(S32 index);
S32 ConnectDevice(S32 index)
{
    S32 rc = IPOD_OK;
    U32 retry_count = 0;
    S32 iPodID = -1;

    /* initialize iPods */
#ifdef IPOD_HOSTPLUGIN_HIDAPI
    iPodID = iPodInitDeviceConnection(&g_serialNum[index][0], IPOD_USB_HOST_CONNECT);
    printf(" iPodInitDeviceConnection: %d  %s\n", iPodID, &g_serialNum[index][0]);
#else
    iPodID = iPodInitDeviceConnection(g_devicename[index], IPOD_USB_HOST_CONNECT);
    printf(" iPodInitDeviceConnection: %d  %s\n", iPodID, g_devicename[index]);
#endif

    if(iPodID > 0)
    {
        iPods[iPodID].iPodID = iPodID;
        strncpy((char*)(&iPods[iPodID].serialNum[0]), (char*)(&g_serialNum[index][0]), DEV_DETECT_CFG_STRING_MAX);
        iPodOSSleep(100);
        /* wait for attach event */
        retry_count = 0;
        while(iPods[iPodID].connected != IPOD_ATTACHED)
        {
            iPodOSSleep(50);
            retry_count++;
            if(iPods[iPodID].connected < 0)
            {
                printf("   connect %d failed! Error during Authentication! retry: %d\n", iPodID, retry_count);
                break;
            }
#if TEST_CANCEL_DEV_CON /* leave ConnectDevice() to cancel InitDeviceConnection() */
            if((iPodID == CANCEL_DEV_CON_IPOD_ID) && (retry_count == 15) && (iPods[CANCEL_DEV_CON_IPOD_ID].getInterrupted != 1))
            {
                /* indicate that interrupt was done and next retry should finish authentication */
                iPods[CANCEL_DEV_CON_IPOD_ID].getInterrupted = 1;
                break;
            }
#endif
        }
        /* return iPodID if attach callback was received */
        if(iPods[iPodID].connected == IPOD_ATTACHED)
        {
            printf("   connect device %d success! retry: %d\n", iPodID, retry_count);

#if TEST_IPOD_SELF
            if(g_detectediPods == 1)
            {
                rc = iPodAuthenticateiPod(iPodID);
                printf(" Device %d: iPodAuthenticateiPod = %d \n", iPodID, rc);
            }
#endif

        }
        else
        {
            printf("   connect device %s failed! retry: %d | err: %d\n", &iPods[iPodID].serialNum[0], retry_count, iPods[iPodID].connected);
        }
        rc = iPodID;
    }
    else
    {
        rc = IPOD_ERROR;
    }

    return rc;
}

/* thread which executes the test for an iPod */
void TestTsk(void* exinf);
void TestTsk(void* exinf)
{
    S32 ret = IPOD_OK;
    S32 dev = 0;
    S32 retry_count = 0;
    U16 leave = FALSE;

#if TEST_IOS_APP
    IPOD_IOS_APP iOSInfo;
    S8 numApps = 1;
#endif

    U32 index = (U32)*((U32*)exinf);

    while(leave == FALSE)
    {
#if TEST_IOS_APP
        memset(&iOSInfo, 0, sizeof(iOSInfo));
        iOSInfo.protocol = (VP)"com.apple.p1";
        iOSInfo.bundle = (VP)"A0B1C2D3E4";
        iOSInfo.metaData = 0;
        ret = iPodSetConfiOSApp(&g_serialNum[index][0], &iOSInfo, numApps);
        printf(" Device %d: iPodSetConfiOSApp rc = %d\n\n", index, ret);
#endif

        retry_count++;
        printf("\n Init %s\n", &g_serialNum[index][0]);
        dev = ConnectDevice(index);
        if(dev <= IPOD_OK)
        {
            printf(" Init failed %d | try: %d\n\n", dev, retry_count);
            g_failCnt += 1;
            leave = TRUE;
        }
        else
        {
            if(iPods[dev].connected == IPOD_ATTACHED)
            {/* iPod authenticated */
                printf(" Init success %d | try: %d\n\n", dev, retry_count);

#if (TEST_BASICS || TEST_PERFORMANCE)
                ret = TestBasic(dev);
                iPods[dev].testFailCount += ret;
#endif

#if (TEST_PERFORMANCE || TEST_IOS_APP || TEST_DB_FUNC || TEST_EXP_DB_FUNC || TEST_PLAYBACK_FUNC || TEST_UID_LIST_FUNC || TEST_ARTWORK_FUNC)
                ret = iPodEnterExtendedInterfaceMode(dev);
                printf(" Device %d: iPodEnterExtendedInterfaceMode rc = %d\n\n", dev, ret);
#endif

#if TEST_PERFORMANCE
                ret = TestPerformance(dev);
#endif

#if (TEST_DB_FUNC && !TEST_PERFORMANCE)
                ret = TestDBinfo(dev);
                iPods[dev].testFailCount += ret;
#endif

#if (TEST_PLAYBACK_FUNC && !TEST_PERFORMANCE)
                ret = TestPlayback(dev);
                iPods[dev].testFailCount += ret;
#endif

#if (TEST_EXP_DB_FUNC && !TEST_PERFORMANCE)
                IPOD_TRACK_INFORMATION_BITFIELD bitfield = {0};
                bitfield.track_info.ARTIST_NAME = 1;
                bitfield.track_info.TRACK_NAME = 1;
                bitfield.track_info.YEAR = 1;
                bitfield.track_info.ALBUM_NAME = 1;
                bitfield.track_info.GENRE_NAME = 1;
                bitfield.track_info.CHAPTER_COUNT = 1;
                bitfield.track_info.CHAPTER_NAMES = 1;
                bitfield.track_info.CHAPTER_TIMES = 1;
                /* lyrics only returned by iPod if track is currently playing */
                bitfield.track_info.LYRIC_STRING = 1;
                /* set UID active and reset UID array index */
                bitfield.track_info.UID = 1;

                ret = TestBulkDBinfo(dev, bitfield);
                iPods[dev].testFailCount += ret;
#endif

#if (TEST_ARTWORK_FUNC && !TEST_PERFORMANCE)
                ret = TestArtwork(dev);
                iPods[dev].testFailCount += ret;
                iPodOSSleep(3000);
#endif

#if (TEST_UID_LIST_FUNC && !TEST_PERFORMANCE)
                IPOD_EXTENDED_STATUS_CHANGE_NOTIFICATION mode;
                memset(&mode, 0, sizeof(mode));
                mode.basicPlay = 1;
                mode.trackIndex = 1;
                mode.trackTimeMs = 1;
                mode.chapterIndex = 1;
                mode.trackUID = 1;
                mode.capChanges = 1;
                mode.pbChange = 1;

                ret = iPodExtendedSetPlayStatusChangeNotification(dev, mode);
                printf(" Device %d: iPodExtendedSetPlayStatusChangeNotification rc = %d\n\n", dev, ret);
                iPods[dev].testFailCount += ret;

                ret = TestUIDlist(dev, bitfield);
                iPods[dev].testFailCount += ret;
                /* play the new track of the UID list for a while */
                iPodOSSleep(2000);
#endif

#if (TEST_PLAYBACK_FUNC && !TEST_PERFORMANCE)
                /* stop playback */
                (void)iPodGetPlayStatus(dev, &iPods[dev].playState, &iPods[dev].length, &iPods[dev].position);
                if(iPods[dev].playState == IPOD_PLAYER_STATE_PLAYING)
                {
                    (void)iPodPlayToggle(dev);
                }
#endif

#if TEST_IOS_APP
                retry_count = 0;
                printf("Device %d: Please open the App! \n\n", dev);
                while((g_CBOpenApp != 1) && (retry_count < 31))
                {
                    iPodOSSleep(1000);
                    retry_count++;
                }
                if(g_CBOpenApp == 1)
                {
                    retry_count = 0;
                    printf("Device %d: App was opened!\n\t waiting for data transfer ...\n\n", dev);
                    while((g_CBOpenApp != 0) && (retry_count < 121))
                    {
                        iPodOSSleep(1000);
                        retry_count++;
                    }
                    if(g_CBOpenApp == 0)
                        printf("Device %d: App was closed!\n", dev);
                    else
                        printf("Device %d: App was not closed! timeout: %d sec\n", dev, retry_count);
                }
#endif

#if (TEST_PERFORMANCE || TEST_IOS_APP || TEST_DB_FUNC || TEST_EXP_DB_FUNC || TEST_PLAYBACK_FUNC || TEST_UID_LIST_FUNC || TEST_ARTWORK_FUNC)
                ret = iPodEnterSimpleMode(dev);
                printf(" Device %d: iPodEnterSimpleMode rc = %d\n", dev, ret);
#endif

#if TEST_IOS_APP

                ret = iPodSetConfiOSApp(&g_serialNum[index][0], NULL, 1);
                printf(" Device %d: iPodSetConfiOSApp rc = %d\n\n", index, ret);
#endif

//                iPodOSSleep(2000);
                printf(" DeInit\n");
                ret = DisconnectDevice(dev);
                leave = TRUE;

                /* free UID array */
                if(iPods[dev].arrayUID != NULL)
                {
                    free(iPods[dev].arrayUID);
                }
            }
            else
            {/* authentication failed check return value of attach callback (CP busy ?) */
                printf(" Init failed %d | %d | try: %d\n", dev, iPods[dev].connected, retry_count);

                iPods[dev].connected = IPOD_NOT_DETECTED;
                ret = DisconnectDevice(dev);
                if(retry_count < MAX_CONNECT_DEVICE_RETRIES)
                {/* try to authenticate again */
                    /* may an authentication is still in progress, give them time to finish */
                    iPodOSSleep(2000);
                    leave = FALSE;
                }
                else
                {/* max retries reached */
                    leave = TRUE;
                    iPods[dev].testFailCount += 1;
                }
            }
        }
    }

//    printf("exit %d\n", index);
    pthread_exit((void*)exinf);
}

void logLTPresult(S32 failCount);
void logLTPresult(S32 failCount)
{
    if(failCount == 0)
    {
        printf("ipod_ctrl_smoketest            PASS       %d", failCount);
    }
    else
    {
        printf("ipod_ctrl_smoketest            FAIL       %d", failCount);
    }
}


EXPORT void iap1AppThread(void)
{
    S32 failCnt = 0;
    S32 rc = IPOD_OK;
    U32 ipod_count = 0;
    pthread_t thread[NUM_IPOD];
    char tsk_name[8];
    pthread_attr_t attr;
    void *status;

    /* in-case iAP1 Device detected,  wait until iAP2 stack is de-initialized */
    sem_wait(&g_iap1semlock);
    if(g_iap1Device == TRUE)
    {
        printf("\n**********  Switched to iAP1 ***************\n\n");

        /* register iPod Control callbacks */
         iPodRegisterCBNotification(CBNotification);
         iPodRegisterCBUSBAttach(ipod_usb_attach);
         iPodRegisterCBUSBDetach(ipod_usb_detach);
         iPodRegisterCBNotifyStatus(CBNotifyStatus);
         iPodRegisterCBNewiPodTrackInfo(sampleratechange);

         iPodRegisterCBRemoteEventNotification(CBRemoteEventNotification);

#if TEST_IOS_APP
         rc = iPodRegisterCBOpenDataSession(CBOpenApp);
         rc = iPodRegisterCBiPodDataTransfer(CBAppDataIn);
         rc = iPodRegisterCBCloseDataSession(CBCloseApp);
#endif
         /* initialize iPod Control */
         rc = iPodInitConnection();
         printf("iPodInitConnection: %d \n", rc);
         if(rc != IPOD_OK)
         {
             failCnt++;
         }

#if TEST_AUTH_SELFTEST
         U8 certificate[2048];
         U8 private_key[1];
         U8 ram_check[1];
         U8 checksum[1024];

         printf(" AuthenticationSelftest.. \n");
         rc = AuthenticationSelftest(&certificate[0],
                                &private_key[0],
                                &ram_check[0],
                                &checksum[0]);
         printf(" AuthenticationSelftest = %d | certificate: 0x%X | private_key: 0x%X \n", rc, certificate[0], private_key[0]);
#endif



#if TEST_INIT_ACC_CON
         /*Update Accessory Information*/
         memset(&acc_info, 0, sizeof(acc_info));
         acc_info.Name                               = (VP)"GEN3";
         acc_info.Hardware_version.Major_Number      = (U8)4;
         acc_info.Hardware_version.Minor_Number      = (U8)5;
         acc_info.Hardware_version.Revision_Number   = (U8)6;
         acc_info.Software_version.Major_Number      = (U8)1;
         acc_info.Software_version.Minor_Number      = (U8)2;
         acc_info.Software_version.Revision_Number   = (U8)3;
         acc_info.Manufacturer                       = (VP) "BOSCH";
         acc_info.ModelNumber                        = (VP)"15697";
         acc_info.SerialNumber                       = (VP)"189652";

         rc = iPodInitAccessoryConnection(acc_info);
         printf("iPodInitAccessoryConnection: %d\n", rc);
         if(rc != IPOD_OK)
         {
             failCnt++;
         }
#endif

         /* enumerate all connected iPods */
#ifdef IPOD_HOSTPLUGIN_HIDAPI
         rc = iPodUdev();
         printf("iPodUdev = %d\n", rc);
         if(rc != IPOD_OK)
         {
             failCnt++;
         }
#endif

         memset(&attr, 0, (sizeof(pthread_attr_t)));
         memset(iPods, 0 , sizeof(iPods));

         if(rc == IPOD_OK)
         {
             rc = pthread_attr_init(&attr);
             if(rc != IPOD_OK)
                 printf("attr_init failed with %d\n", rc);
             rc = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
             if(rc != IPOD_OK)
                 printf("attr_setdetachstate failed with %d\n", rc);

             /* create for each available iPod an own test thread */
             for(ipod_count = 0; ipod_count < g_detectediPods; ipod_count++)
             {
                 if (rc == 0)
                 {
                     rc = pthread_create(&thread[ipod_count], &attr, (void *(*)(void *))TestTsk, (void*)&ipod_count);
                     if(rc == 0)
                     {
                         g_ttask_id[ipod_count] = (IPOD_TASK_ID)thread[ipod_count];
     //                    printf("create %lu returns %d\n", g_ttask_id[ipod_count], rc);
                         /* set thread name */
                         sprintf(&tsk_name[0], "%s%d", "iTest_", ipod_count);
                         rc = pthread_setname_np(thread[ipod_count], (const char*)tsk_name);
                     }
                     iPodOSSleep(100);
                 }
             }

             (void)pthread_attr_destroy(&attr);
             for(ipod_count = 0; ipod_count < g_detectediPods; ipod_count++)
             {
                 if(g_ttask_id[ipod_count] > 0)
                 {
                     rc = pthread_join(g_ttask_id[ipod_count], &status);
     //                printf("main() completed with %lu returns %d with status %ld\n", g_ttask_id[ipod_count], rc, (long)status);
                 }
             }
         }

         /* de-initialize iPod Control */
         iPodDisconnect();

         /* ltp log */
         for(ipod_count = 1; ipod_count <= g_detectediPods; ipod_count++)
         {
             failCnt += iPods[ipod_count].testFailCount;
             printf("\n Device %d: testFailCount = %d", iPods[ipod_count].iPodID, iPods[ipod_count].testFailCount);
         }
         failCnt += g_failCnt;

         printf("\n\n iPod Smoketest over.\n\n");

         /* ltp log */
         logLTPresult(failCnt);
         printf("\n");

    }

    printf("exit iap1AppThread \n");
    pthread_exit(NULL);
}
