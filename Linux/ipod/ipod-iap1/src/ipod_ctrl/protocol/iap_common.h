
#ifndef IPOD_COMMON_H
#define IPOD_COMMON_H

#include <stdio.h>
#include <stdarg.h>
#include <iap_types.h>

#ifdef __cplusplus
extern "C" {
#endif

/*** DEFINE_IFLIB

[LIBRARY HEADER FILE]
ifiap_common.h

[FNUMBER HEADER FILE]
fniap_common.h

[INCLUDE FILE]
"iap_common.h"

[PREFIX]
IPOD
***/

/* [BEGIN SYSCALLS] */

/***************************** Start Callback function *********************************/

IMPORT S32 iPodRegisterCBUSBAttach(IPOD_CB_USB_ATTACH callback);
IMPORT S32 iPodRegisterCBUSBDetach(IPOD_CB_USB_DETACH callback);

IMPORT S32 iPodRegisterCBNotifyStatus(IPOD_CB_NOTIFY callback);
IMPORT S32 iPodRegisterCBNotifyStateChange(IPOD_CB_NOTIFY_STATE_CHANGE callback);
IMPORT S32 iPodRegisterCBGetAccSampleRateCaps(IPOD_CB_GET_ACC_SAMPLE_RATE_CAPS callback);
IMPORT S32 iPodRegisterCBNewiPodTrackInfo(IPOD_CB_NEW_TRACK_INFO callback);
IMPORT S32 iPodRegisterCBTrackArtworkData(IPOD_CB_GET_ARTWORK callback);
IMPORT S32 iPodRegisterCBLocation(IPOD_CB_LOCATION const callback);
IMPORT S32 iPodRegisterCBNotification(IPOD_CB_NOTIFICATION callback);
IMPORT S32 iPodRegisterCBRemoteEventNotification(IPOD_CB_REMOTE_EVENT_NOTIFICATION callback);
IMPORT S32 iPodRegisterCBSetAccStatusNotification(IPOD_CB_SET_ACC_STATUS callback);
IMPORT S32 iPodRegisterCBOpenDataSession(IPOD_CB_OPEN_DATA_SESSION callback);
IMPORT S32 iPodRegisterCBCloseDataSession(IPOD_CB_CLOSE_DATA_SESSION callback);
IMPORT S32 iPodRegisterCBiPodDataTransfer(IPOD_CB_IPOD_DATA_TRANSFER callback);
IMPORT S32 iPodDevDataTransfer(U32 iPodID, U16 sessionId, U8 *data, U16 dataLen);
IMPORT S32 iPodRequestAppLaunch(U32 iPodID, U8 *bundleId, U8 length);
IMPORT S32 iPodGetNowPlayingFocusApp(U32 iPodID, U8 *focusApp, U16 length);
IMPORT S32 iPodCancelCommand(U32 iPodID, IPOD_CANCEL_COMMAND_TYPE command);
IMPORT S32 iPodRequestTransportMaxPacketSize(U32 iPodID, U16 *size);
IMPORT void iPodSetTrackArtworkDataImageSaveParams(U32 iPodID, U8 saveAsBMP, U8* imagePath);
IMPORT S32 iPodRetAccStatusNotification(U32 iPodID, U32 statusMask);
IMPORT S32 iPodAccStatusNotification(U32 iPodID, IPOD_ACC_STATUS_TYPE type, IPOD_ACC_STATUS_PARAM *status, U16 len);

/***************************** End Callback function ***********************************/



/***************************** Start Database function *********************************/

IMPORT S32 iPodGetNumberCategorizedDBRecords(U32 iPodID, IPOD_CATEGORY category);
IMPORT S32 iPodSelectSortDBRecords(U32 iPodID, IPOD_CATEGORY category,
                            U32 index,
                            IPOD_DB_SORT_TYPE sortType);
IMPORT S32 iPodRetrieveCategorizedDBRecords(U32 iPodID, 
                            IPOD_CATEGORY category,
                            U32 start,
                            S32 count,
                            const IPOD_CB_RETRIEVE_CAT_DB_RECORDS callback);
IMPORT S32 iPodResetDBSelection(U32 iPodID);
IMPORT S32 iPodSelectDBRecord(U32 iPodID, IPOD_CATEGORY category, U32 idx);
IMPORT S32 iPodReturnToPreviousDBRecordSelection(U32 iPodID, IPOD_CATEGORY category);
IMPORT S32 iPodPlayCurrentSelection(U32 iPodID, U32 idx);
IMPORT S32 iPodResetDBSelectionHierarchy(U32 iPodID, U8 selection);

IMPORT S32 iPodGetDBiTunesInfo(U32 iPodID, IPOD_ITUNES_METADATA_TYPE metadataType, IPOD_ITUNES_METADATA_INFO *metadataInfo);
IMPORT S32 iPodGetUIDTrackInfo(U32 iPodID, U64 uID, IPOD_TRACK_INFORMATION_TYPE trackType, IPOD_TRACK_INFORMATION_CB callback);
IMPORT S32 iPodGetDBTrackInfo(U32 iPodID, U32 dbIndex, S32 trackCount, IPOD_TRACK_INFORMATION_TYPE trackType, IPOD_TRACK_INFORMATION_CB callback);
IMPORT S32 iPodGetPBTrackInfo(U32 iPodID, U32 pbIndex, S32 trackCount, IPOD_TRACK_INFORMATION_TYPE trackType, IPOD_TRACK_INFORMATION_CB callback);

IMPORT S32 iPodGetBulkUIDTrackInfo(U32 iPodID, U64 uID, IPOD_TRACK_INFORMATION_BITFIELD trackType, IPOD_TRACK_INFORMATION_CB callback);
IMPORT S32 iPodGetBulkDBTrackInfo(U32 iPodID, U32 dbIndex, S32 trackCount, IPOD_TRACK_INFORMATION_BITFIELD trackType, IPOD_TRACK_INFORMATION_CB callback);
IMPORT S32 iPodGetBulkPBTrackInfo(U32 iPodID, U32 dbIndex, S32 trackCount, IPOD_TRACK_INFORMATION_BITFIELD trackType, IPOD_TRACK_INFORMATION_CB callback);

/***************************** End Database function ***********************************/


/***************************** Start Digital Audio Function ****************************/
IMPORT void iPodAccAck(U32 iPodID, IPOD_ACC_ACK_STATUS status);
IMPORT void iPodAccAckDevice(IPOD_ACC_ACK_STATUS status, U32 iPodID);
IMPORT S32 iPodSetVideoDelay(U32 iPodID, U32 delay);

/***************************** End Digital Audio Function ******************************/


/***************************** Start Display Function **********************************/
IMPORT S32 iPodGetArtworkFormats(U32 iPodID, IPOD_ARTWORK_FORMAT* resultBuf, 
                          U16* resultCount);
IMPORT S32 iPodGetMonoDisplayImageLimits(U32 iPodID, U16* width, 
                                  U16* height, 
                                  U8* pixelFormat);
IMPORT S32 iPodGetColorDisplayImageLimits(U32 iPodID, IPOD_DISPLAY_IMAGE_LIMITS* resultBuf, 
                                   U16* resultCount);
IMPORT S32 iPodSetDisplayImage(U32 iPodID, const U8* image, 
                        IPOD_IMAGE_TYPE imageType);
IMPORT S32 iPodSetDisplayImageBMP(U32 iPodID, const U8* bmpImage);
IMPORT S32 iPodGetTrackArtworkData(U32 iPodID, U32 trackIndex,
                            U16 formatId,
                            U32 timeOffset,
                            const IPOD_CB_GET_ARTWORK callback);

IMPORT S32 iPodGetTrackArtworkTimes(U32 iPodID, U32 trackIndex,
                             U16 formatId,
                             U16 artworkIndex,
                             U16 artworkCount,
                             U16* resultCount,
                             U32* buffer);
IMPORT S32 setImage(U32 iPodID, U32 imageWidth, U32 imageHeight, U16 *image, IPOD_IMAGE_TYPE imageType);
IMPORT S32 iPodSetDisplayImageMemory(U32 iPodID, const U8* bmpImage);
IMPORT S32 iPodSetDisplayImageBMPStoredMonochrom(U32 iPodID, const U8* bmpImage);
IMPORT void iPodRGBToRGB565(U8 r,
                     U8 g,
                     U8 b,
                     U16 *rgb565);
IMPORT void iPodReadAndConvertBMPMemory(const U8* file,
                               U16** rgb565Buffer,
                               U32*  width,
                               U32*  height);
                               
IMPORT void iPodReadAndConvertBMPFile(FILE* file,
                               U16** rgb565Buffer,
                               U32*  width,
                               U32*  height);

IMPORT S32 iPodGetTypeOfTrackArtworkData(U32 iPodID, IPOD_TRACK_TYPE type, U64 trackIndex,
                            U16 formatId,
                            U32 timeOffset,
                            const IPOD_CB_GET_ARTWORK callback);

IMPORT S32 iPodGetTypeOfTrackArtworkTimes(U32 iPodID, IPOD_TRACK_TYPE type, 
                                    U32  trackIndex,
                                    U16  formatId,
                                    U16  artworkIndex,
                                    U16  artworkCount,
                                    U16 *resultCount,
                                    U32 *buffer);


/***************************** End Display Function ************************************/



/***************************** Start General Function **********************************/
IMPORT S32  iPodEnterExtendedInterfaceMode(U32 iPodID);
IMPORT S32  iPodEnterSimpleMode(U32 iPodID);
IMPORT S32  iPodGetRemoteUIMode(U32 iPodID);
IMPORT S32  iPodGetModelNum(U32 iPodID, U8* modelString);
IMPORT S32  iPodGetLingoProtocolVersion(U32 iPodID, IPOD_LINGO lingo, U8* majorVer, U8* minorVer);
IMPORT S32  iPodGetSoftwareVersion(U32 iPodID, U8* majorVer, U8* minorVer, U8* revisionVer);
IMPORT S32  iPodGetSerialNumber(U32 iPodID, U8* serialNumber);
IMPORT S32  iPodGetIPodName(U32 iPodID, U8* iPodName);

/*
IMPORT S32 iPodAuthenticateiPod(U32 iPodID);
IMPORT S32 iPodAuthenticationGetDeviceID (U32 *AuthenticationDeviceID);
IMPORT S32 iPodAuthenticationGetFirmwareVersion (U8 *majorVer, U8 *minorVer);
IMPORT S32 iPodAuthenticationGetProtocolVersion (U8 *majorVer, U8 *minorVer);
IMPORT S32 iPodAuthenticationSelftest (U8 *certificate, U8 *private_key, U8 *ram_check, U8 *checksum);
IMPORT void iPodGetAuthenticateCertificate(U16 *certLen, U8 *certData);
IMPORT S32 iPodInitAuthentication(void);
IMPORT S32 iPodDeinitAuthentication(void);
*/

IMPORT S32 iPodGetDevconfParameter(void);

IMPORT S32 iPodGetiPodOptions(U32 iPodID, IPOD_OPTIONS_BIT *optionBits);
IMPORT S32 iPodGetiPodPreferences(U32 iPodID, IPOD_PREFERENCE_CLASS_ID classId, IPOD_PREFERENCE_SETTING_ID *settingId);
IMPORT S32 iPodSetiPodPreferences(U32 iPodID, IPOD_PREFERENCE_CLASS_ID classId, IPOD_PREFERENCE_SETTING_ID settingId, U8 restore);
IMPORT S32 iPodSetEventNotification(U32 iPodID, U64 mask);
IMPORT S32 iPodGetiPodOptionsForLingo(U32 iPodID, IPOD_LINGO lingo, U64 *optionBits);
IMPORT S32 iPodGetEventNotification(U32 iPodID, U64 *eventMask);
IMPORT S32 iPodGetSupportedEventNotification(U32 iPodID, U64 *eventMask);
/***************************** End General Function *************************************/

/***************************** Start Init Function *************************************/
IMPORT S32  iPodInitConnection(void);
IMPORT void iPodDisconnect(void);
IMPORT void iPodSwitchAudioOutput(U32 iPodID, IPOD_SWITCH_AUDIO_OUT switchAudio);
/***************************** End Init Function ***************************************/

/***************************** Start Multiple Support ---*******************************/
IMPORT S32  iPodInitAccessoryConnection(IPOD_ACC_INFO_CONFIGURATION acc_info);
IMPORT S32  iPodInitDeviceConnection(U8* devicename, IPOD_CONNECTION_TYPE connection);
IMPORT void iPodDisconnectDevice(U32 iPodID);
IMPORT S32  iPodSetConfiOSApp(U8 *devicename, IPOD_IOS_APP *iOSInfo, S8 numApps);
IMPORT void iPodRequestIdentify(U32 iPodID);
/***************************** End Multiple Support ************************************/

/***************************** Start Location Function *********************************/
IMPORT S32 iPodRetDevCaps(U32 iPodID, IPOD_LOCATION_TYPE locType, U8 *capsData, U8 size);
IMPORT S32 iPodRetDevControl(U32 iPodID, IPOD_LOCATION_TYPE locType, U64 ctlData);
IMPORT S32 iPodRetDevData(U32 iPodID, IPOD_LOCATION_TYPE locType, U8 dataType, U8 *locData, U32 locSize);
IMPORT S32 iPodAsyncDevData(U32 iPodID, IPOD_LOCATION_TYPE locType, U8 dataType, U32 totalSize, U8 *locData);
/***************************** End Location Function ***********************************/

/***************************** Start Playback Function *********************************/
IMPORT S32 iPodPlayToggle(U32 iPodID);

IMPORT S32 iPodPlayStop(U32 iPodID);

IMPORT S32 iPodPlayNextTrack(U32 iPodID);

IMPORT S32 iPodPlayPrevTrack(U32 iPodID);

IMPORT S32 iPodPlayFastForward(U32 iPodID);

IMPORT S32 iPodPlayFastBackward(U32 iPodID);

IMPORT S32 iPodPlayNormal(U32 iPodID);

IMPORT S32 iPodPlayNextChapter(U32 iPodID);

IMPORT S32 iPodPlayPrevChapter(U32 iPodID);

IMPORT S32 iPodSetShuffle(U32 iPodID, IPOD_SHUFFLE_MODE shuffleMode, BOOL persistent);

IMPORT S32 iPodShuffleOff(U32 iPodID);

IMPORT S32 iPodShuffleOnSongs(U32 iPodID);

IMPORT S32 iPodShuffleOnAlbums(U32 iPodID);

IMPORT S32 iPodGetShuffleMode(U32 iPodID);

IMPORT S32 iPodSetRepeat(U32 iPodID, IPOD_REPEAT_MODE repeatMode, BOOL persistent);

IMPORT S32 iPodRepeatOff(U32 iPodID);

IMPORT S32 iPodRepeatCurrentSong(U32 iPodID);

IMPORT S32 iPodRepeatAllSongs(U32 iPodID);

IMPORT S32 iPodGetRepeatMode(U32 iPodID);

IMPORT S32 iPodGetAudioBookSpeed(U32 iPodID);

IMPORT S32 iPodSetAudioBookSpeed(U32 iPodID, IPOD_AUDIOBOOK_SPEED speed);

IMPORT S32 iPodGetNumPlayingTracks(U32 iPodID);

IMPORT S32 iPodSetCurrentPlayingTrack(U32 iPodID, U32 trackIndex);

IMPORT S32 iPodSetCurrentPlayingTrackChapter(U32 iPodID, U32 chapterIndex);

IMPORT S32 iPodGetCurrentPlayingTrackChapterName(U32 iPodID, U32 chapterIndex, 
                                           U8* chapterName);

IMPORT S32 iPodGetCurrentPlayingTrackIndex(U32 iPodID);

IMPORT S32 iPodGetCurrentPlayingTrackChapterInfo(U32 iPodID, S32* chapterIndex, S32* chapterCount);

IMPORT S32 iPodGetPlayStatus(U32 iPodID, IPOD_PLAYER_STATE* state, U32* length, U32* position);

IMPORT S32 iPodSetPlayStatusChangeNotification(U32 iPodID, IPOD_STATUS_CHANGE_NOTIFICATION mode);

IMPORT S32 iPodExtendedSetPlayStatusChangeNotification(U32 iPodID, IPOD_EXTENDED_STATUS_CHANGE_NOTIFICATION mode);

IMPORT S32 iPodGetIndexedPlayingTrackTitle(U32 iPodID, U32 currIndex, U8* songTitle);

IMPORT S32 iPodGetIndexedPlayingTrackArtistName(U32 iPodID, U32 currIndex, U8* artistName);

IMPORT S32 iPodGetIndexedPlayingTrackAlbumName(U32 iPodID, U32 currIndex, U8* albumName);                                     

IMPORT S32 iPodGetCurrentPlayingTrackChapterPlayStatus(U32 iPodID, U32 chapterIndex, 
                                                U32* chapterLength,
                                                U32* elapsedTime);
IMPORT S32 iPodGetIndexedPlayingTrackInfo(U32 iPodID, IPOD_TRACK_INFO_TYPE info, 
                                    U32 trackIndex, 
                                    U16 chapterIndex, 
                                    const IPOD_CB_PLAYING_TRACK_INFO callback);                                                                                           
IMPORT S32 iPodGetIndexedPlayingTrackGenre(U32 iPodID, U32 trackIndex,
                                    U16 chapterIndex,
                                    U8* artistName);

IMPORT S32 iPodPlayControl(U32 iPodID, IPOD_PLAY_CONTROL ctrl);

IMPORT S32 iPodPrepareUIDList(U32 iPodID, U64* uidList, U32 uidCount);

IMPORT S32 iPodPlayPreparedUIDList(U32 iPodID, U64 trackUID);
/***************************** End Playback Audio Function *****************************/

/***************************** Start Simple Function ***********************************/
IMPORT S32 iPodPlayPause(U32 iPodID);
IMPORT S32 iPodNextTitle(U32 iPodID);
IMPORT S32 iPodPreviousTitle(U32 iPodID);
IMPORT S32 iPodNextAlbum(U32 iPodID);
IMPORT S32 iPodPreviousAlbum(U32 iPodID);
IMPORT S32 iPodVolumeUp(U32 iPodID);
IMPORT S32 iPodVolumeDown(U32 iPodID);
IMPORT S32 iPodOutButtonStatus(U32 iPodID, U8 source, U32 statusBits);
IMPORT S32 iPodRotationInputStatus(U32 iPodID, IPOD_ROTATION_INFO rotation);
IMPORT S32 iPodAccessibilityEvent(U32 iPodID, IPOD_ACC_EVENT_TYPE type, IPOD_ACC_EVENT_DATA data);
/***************************** End Simple Function *************************************/

/***************************** Start Display Remote Function ***********************************/
IMPORT S32 iPodSetTrackPosition(U32 iPodID, U32 trackPosition);

IMPORT S32 iPodSetRemoteEventNotification(U32 iPodID, U32 eventMask);

IMPORT S32 iPodGetCurrentEQProfileIndex(U32 iPodID, U32 *index);
IMPORT S32 iPodSetCurrentEQProfileIndex(U32 iPodID, U32 index, U8 restore);
IMPORT S32 iPodGetNumEQProfiles(U32 iPodID, U32 *profileCount);
IMPORT S32 iPodGetIndexedEQProfileName(U32 iPodID, U32 profileIndex, U8 *name, U32 size);
IMPORT S32 iPodGetDRArtworkFormats(U32 iPodID, IPOD_ARTWORK_FORMAT* resultBuf, U16* resultCount);

IMPORT S32 iPodGetDRTrackArtworkData(U32 iPodID, U32 trackIndex, U16 formatId, U32 timeOffset, const IPOD_CB_GET_ARTWORK callback);
IMPORT S32 iPodGetDRPlayStatus(U32 iPodID, IPOD_PLAYER_STATE* state, U32 *trackIndex, U32* length, U32* position);
IMPORT S32 iPodGetDRTrackArtworkTimes(U32 iPodID, U32 trackIndex, U16  formatId, U16  artworkIndex, U16  artworkCount, U16 *resultCount, U32 *buffer);

/***************************** End Display Remote Function *************************************/

/***************************** Start Storage Function **********************************/
IMPORT S32 iPodGetiPodCaps(U32 iPodID, IPOD_STORAGE_CAPS *storageCaps);
IMPORT S32 iPodGetiPodFreeSpace(U32 iPodID, U64 *freeSpace);
IMPORT S32 iPodOpeniPodFeatureFile(U32 iPodID, IPOD_FEATURE_TYPE featureType, IPOD_FILE_OPTIONS_MASK *bitMask, const U8* fileData, U8 fileSize, U8* fileHandle);
IMPORT S32 iPodWriteiPodFileData(U32 iPodID, U32 offset, U8 handle, const U8* data, U16 length);
IMPORT S32 iPodCloseiPodFile(U32 iPodID, U8 handle);
IMPORT S32 iPodGetiPodOutOptions(U32 iPodID, U8 options, U32 *optionsBits);
IMPORT S32 iPodSetiPodOutOptions(U32 iPodID, U32 optionsBits);
IMPORT S32 iPodDevStateChangeEvent(U32 iPodID, U8 status);
IMPORT S32 iPodDevVideoScreenInfo(U32 iPodID, IPOD_VIDEO_SCREEN_INFO info);
IMPORT S32 iPodGetUIMode(U32 iPodID, U8 *mode);
IMPORT S32 iPodSetUIMode(U32 iPodID, U8 mode);

/***************************** End Storage Function ************************************/

IMPORT U16 iPodGetMaxPayloadSize(U32 iPodID);

#define LOCAL static

/* [END SYSCALLS] */

#ifdef __cplusplus
}
#endif

#endif /* IPOD_COMMON_H */
