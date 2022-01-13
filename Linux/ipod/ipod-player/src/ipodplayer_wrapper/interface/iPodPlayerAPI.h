/*! \file iPodPlayerAPI.h
 *
 * \version: 1.0
 *
 * \author: mshibata
 */

#ifndef IPOD_PALEYR_API_H
#define IPOD_PALEYR_API_H

#include "iPodPlayerCB.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ################################### iPod Player Function Header ########################################## */
/* IPodPlayerInitFunction */
S32 iPodPlayerInit(U32 connectionMask, IPOD_PLAYER_REGISTER_CB_TABLE *cbTable);
S32 iPodPlayerDeinit(void);
S32 iPodPlayerSelectAudioOut(U32 devID, IPOD_PLAYER_AUDIO_SELECT mode);
S32 iPodPlayerTestReady(void);
S32 iPodPlayerSetiOSAppsInfo(U32 devID, U8 appCount, IPOD_PLAYER_IOSAPP_INFO *appInfo);
S32 iPodPlayerStartAuthentication(U32 devID);

/* IPodPlayerPlayerOperationAPI */
S32 iPodPlayerPlay(U32 devID);
S32 iPodPlayerPlayCurrentSelection(U32 devID);
S32 iPodPlayerPause(U32 devID);
S32 iPodPlayerStop(U32 devID);
S32 iPodPlayerFastForward(U32 devID);
S32 iPodPlayerRewind(U32 devID);
S32 iPodPlayerNextTrack(U32 devID);
S32 iPodPlayerPrevTrack(U32 devID);
S32 iPodPlayerNextChapter(U32 devID);
S32 iPodPlayerPrevChapter(U32 devID);
S32 iPodPlayerGotoTrackPosition(U32 devID, U32 timems);
S32 iPodPlayerPlayTrack(U32 devID, IPOD_PLAYER_TRACK_TYPE type, U64 trackID);
S32 iPodPlayerRelease(U32 devID);

/* iPodPlayerSetInformationAPI */
S32 iPodPlayerSetAudioMode(U32 devID, IPOD_PLAYER_AUDIO_SETTING setting);
S32 iPodPlayerSetMode(U32 devID, IPOD_PLAYER_MODE mode);
S32 iPodPlayerSetRepeatStatus(U32 devID, IPOD_PLAYER_REPEAT_STATUS status);
S32 iPodPlayerSetShuffleStatus(U32 devID, IPOD_PLAYER_SHUFFLE_STATUS status);
S32 iPodPlayerSetEqualizer(U32 devID, U32 eq, U8 restore);
S32 iPodPlayerSetVideoDelay(U32 devID, U32 delayTime);
S32 iPodPlayerSetVideoSetting(U32 devID, IPOD_PLAYER_VIDEO_SETTING *setting, U32 restore);
S32 iPodPlayerSetDisplayImage(U32 devID, U32 imageSize, U8 *image);
S32 iPodPlayerSetPlaySpeed(U32 devID, IPOD_PLAYER_PLAYING_SPEED speed);
S32 iPodPlayerSetTrackInfoNotification(U32 devID, U32 trackInfoMask, U16 formatId);
S32 iPodPlayerSetDeviceEventNotification(U32 devID, U32 deviceEventMask);
S32 iPodPlayerChangeRepeatStatus(U32 devID);
S32 iPodPlayerChangeShuffleStatus(U32 devID);


/* IPodPlayerGetInformationAPI */
S32 iPodPlayerGetVideoSetting(U32 devID, U32 mask);
S32 iPodPlayerGetCoverArtInfo(U32 devID, IPOD_PLAYER_TRACK_TYPE type, U64 trackIndex, U16 formatId);
S32 iPodPlayerGetCoverArt(U32 devID, IPOD_PLAYER_TRACK_TYPE type, U64 trackIndex, U16 formatId, U32 coverartTime);
S32 iPodPlayerGetPlaybackStatus(U32 devID);
S32 iPodPlayerGetTrackInfo(U32 devID, IPOD_PLAYER_TRACK_TYPE type, U64 startID, U32 count, U32 trackInfoMask);
S32 iPodPlayerGetChapterInfo(U32 devID, IPOD_PLAYER_TRACK_TYPE type, U32 trackID, U32 startIndex, U32 count, U32 chapterInfoMask);
S32 iPodPlayerGetMode(U32 devID);
S32 iPodPlayerGetRepeatStatus(U32 devID);
S32 iPodPlayerGetShuffleStatus(U32 devID);
S32 iPodPlayerGetPlaySpeed(U32 devID);
S32 iPodPlayerGetTrackTotalCount(U32 devID, IPOD_PLAYER_TRACK_TYPE type);
S32 iPodPlayerGetMediaItemInformation(U32 devID, U64 trackID);
S32 iPodPlayerGetEqualizer(U32 devID);
S32 iPodPlayerGetEqualizerName(U32 devID, U32 eq);
S32 iPodPlayerGetDeviceProperty(U32 devID, U32 devicePropertyMask);
S32 iPodPlayerGetDeviceStatus(U32 devID, U32 deviceStatusMask);

/* iPodPlayer Database browsing API */
S32 iPodPlayerGetDBEntries(U32 devID, IPOD_PLAYER_DB_TYPE type, U32 start, S32 num);
S32 iPodPlayerGetDBCount(U32 devID, IPOD_PLAYER_DB_TYPE type);
S32 iPodPlayerCancel(U32 devID, IPOD_PLAYER_CANCEL_TYPE type);
S32 iPodPlayerSelectDBEntry(U32 devID, IPOD_PLAYER_DB_TYPE type, S32 entry);
S32 iPodPlayerClearSelection(U32 devID, IPOD_PLAYER_DB_TYPE type);
S32 iPodPlayerSelectAV(U32 devID, U8 avType);

/* HMI Control API*/
S32 iPodPlayerHMISetSupportedFeature(U32 devID, U32 hmiSupportedMask);
S32 iPodPlayerHMIGetSupportedFeature(U32 devID, IPOD_PLAYER_HMI_FEATURE_TYPE type);
S32 iPodPlayerHMIButtonInput(U32 devID, U32 eventMask, IPOD_PLAYER_HMI_BUTTON_SOURCE source);
S32 iPodPlayerHMIRotationInput(U32 devID, IPOD_PLAYER_HMI_ROTATION_INFO *info, U16 move);
S32 iPodPlayerHMIPlaybackInput(U32 devID, IPOD_PLAYER_HMI_PLAYBACK_EVENT event);
S32 iPodPlayerHMISetApplicationStatus(U32 devID, IPOD_PLAYER_HMI_APP_STATUS status);
S32 iPodPlayerHMISetEventNotification(U32 devID, U32 hmiEventNotificationMask);
S32 iPodPlayerHMIGetEventChange(U32 devID);
S32 iPodPlayerHMIGetDeviceStatus(U32 devID, IPOD_PLAYER_HMI_STATUS_TYPE type);

/* Non Player API */
S32 iPodPlayerOpenSongTagFile(U32 devID, U32 tagOptionsMask, U32 optionLen, U8 *optionData);
S32 iPodPlayerCloseSongTagFile(U32 devID, U32 handle);
S32 iPodPlayerSongTag(U32 devID, U32 handle, IPOD_PLAYER_TAG_TYPE type, IPOD_PLAYER_TAG_INFO *info);
S32 iPodPlayerSendToApp(U32 devID, U32 handle, U32 dataSize, U8 *data);
S32 iPodPlayerRequestAppStart(U32 devID, U8 *appName);
S32 iPodPlayerSetPowerSupply(U32 devID, IPOD_PLAYER_CURRENT powermA, U8 chargeButtery);
S32 iPodPlayerCreateIntelligentPL(U32 devID, IPOD_PLAYER_TRACK_TYPE type, U64 trackID);
S32 iPodPlayerTrackSupportsIntelligentPL(U32 devID, IPOD_PLAYER_TRACK_TYPE type, U64 trackID);
S32 iPodPlayerRefreshIntelligentPL(U32 devID, U32 plIndex);
S32 iPodPlayerGetPLProperties(U32 devID, U32 plIndex, U32 plPropertyMask);
S32 iPodPlayerSetGPSData(U32 devID, IPOD_PLAYER_GPS_TYPE type, U32 dataSize, U8 *data);
S32 iPodPlayerSetGPSCurrentSystemTime(U32 devID, IPOD_PLAYER_GPS_TIME *gpsTime);
S32 iPodPlayerSetVolume(U32 devID, U8 volume);
S32 iPodPlayerGetVolume(U32 devID);
S32 iPodPlayerSetLocationInformation(U32 devID, size_t size, U8 *NMEAdata);
S32 iPodPlayerSetVehicleStatus(U32 devID, IPOD_PLAYER_SET_VEHICLE_STATUS *status);

/* iPodPlayer API common function */
S32 iPodPlayerPlayCommon(U32 devID, BOOL playCurSel);

#ifdef __cplusplus
}
#endif
#endif /* IPOD_PALEYR_API_H */
