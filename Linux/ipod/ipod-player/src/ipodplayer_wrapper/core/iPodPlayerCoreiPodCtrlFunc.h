#ifndef _IPOD_PLAYER_CORE_IPOD_CTRL_FUNC_H
#define _IPOD_PLAYER_CORE_IPOD_CTRL_FUNC_H


S32 iPodCoreFuncInitConnection(void);
void iPodCoreFuncDisconnect(void);
S32 iPodCoreFuncSwitchAudio(U32 devID, U8 mode);
S32 iPodCoreFuncStartAuthentication(U32 devID);
void iPodCoreCBNotifyOldGetTrackInfo(IPOD_TRACK_INFO_TYPE infoType, const IPOD_TRACK_CAP_INFO_DATA* capInfoData, 
                        const IPOD_TRACK_RELEASE_DATE_DATA* releaseData, const IPOD_TRACK_ARTWORK_COUNT_DATA* artworkCountData, U8* stringBuf, const U32 iPodID);
void iPodCoreCBNotifyGetTrackInfo(U64 trackIndex, IPOD_TRACK_INFORMATION_TYPE infoType, IPOD_TRACK_INFORMATION_DATA *infoData, const U32 iPodID);
void iPodCoreCBNotifyOldGetCurrentTrackInfo(IPOD_TRACK_INFO_TYPE infoType, const IPOD_TRACK_CAP_INFO_DATA* capInfoData, 
                        const IPOD_TRACK_RELEASE_DATE_DATA* releaseData, const IPOD_TRACK_ARTWORK_COUNT_DATA* artworkCountData, U8* stringBuf, const U32 iPodID);

/* Playback Function */
S32 iPodCoreFuncPlay(U32 devID);
S32 iPodCoreFuncPause(U32 devID);
S32 iPodCoreFuncStop(U32 devID);
S32 iPodCoreFuncNextTrack(U32 devID);
S32 iPodCoreFuncPrevTrack(U32 devID);
S32 iPodCoreFuncNextChapter(U32 devID);
S32 iPodCoreFuncPrevChapter(U32 devID);
S32 iPodCoreFuncFastForward(U32 devID);
S32 iPodCoreFuncRewind(U32 devID);
S32 iPodCoreFuncGotoTrackPosition(U32 devID, U32 timems);
S32 iPodCoreFuncPlayTrack(U32 devID, IPOD_PLAYER_TRACK_TYPE type, U64 trackIndex);



/* Set Property Function */
S32 iPodCoreFuncSetRepeat(U32 devID, U8 status);
S32 iPodCoreFuncSetShuffle(U32 devID, U8 status);
S32 iPodCoreFuncSetEqualizer(U32 devID, U32 eq, U8 restore);
S32 iPodCoreFuncSetMode(U32 devID, U8 mode);
S32 iPodCoreFuncSetVideoDelay(U32 devID, U32 delayTime);
S32 iPodCoreFuncSetVideoSetting(U32 devID, const IPOD_PLAYER_VIDEO_SETTING *setting, U8 restore);
S32 iPodCoreFuncSetDisplayImage(U32 devID, const U32 imageSize, const U8 *image);
S32 iPodCoreFuncSetPlaySpeed(U32 devID, IPOD_PLAYER_PLAYING_SPEED speed);
S32 iPodCoreFuncSetDeviceEventNotification(U32 devID, U32 bitmask);



/* Get Property Function */
S32 iPodCoreFuncGetVideoSetting(U32 devID, IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_VIDEO_SETTING *setting, U32 mask);
S32 iPodCoreFuncGetCoverartInfo(U32 devID, IPOD_PLAYER_TRACK_TYPE type, U64 trackIndex, U16 formatId, U32 *timeCount, U32 *coverartTime, U32 size);
S32 iPodCoreFuncGetCoverart(U32 devID, const IPOD_PLAYER_TRACK_TYPE type, const U64 trackIndex, const U16 formatId, const U32 coverartTime, const IPOD_CB_GET_ARTWORK callback);
S32 iPodCoreFuncGetTrackInfo(U32 devID, IPOD_PLAYER_TRACK_TYPE type, U64 trackID, U32 *mask, U32 *curMask, U32 featureMask, IPOD_PLAYER_TRACK_INFO *info, void *callback);
S32 iPodCoreFuncGetChapterInfo(U32 devID, IPOD_PLAYER_TRACK_TYPE type, U64 trackID, U32 chapterIndex, U32 mask, U32 *curMask, U32 featureMask, IPOD_PLAYER_CHAPTER_INFO *info, void * callback);
S32 iPodCoreFuncGetMode(U32 devID, U8 *mode);
S32 iPodCoreFuncGetRepeat(U32 devID, U8 *repeat);
S32 iPodCoreFuncGetShuffle(U32 devID, U8 *shuffle);
S32 iPodCoreFuncGetSpeed(U32 devID, U8 *speed);
S32 iPodCoreFuncGetTrackTotalCount(U32 devID, const IPOD_PLAYER_TRACK_TYPE type, U32 *count);
S32 iPodCoreFuncGetEqualizer(U32 devID, U32 *equalizer);
S32 iPodCoreFuncGetEqualizerName(U32 devID, const U8 equalizer, U8 *name);
S32 iPodCoreFuncGetRunningApp(U32 devID, U8 *name, U16 length);
S32 iPodCoreFuncGetDeviceProperty(U32 devID, U32 mask, IPOD_PLAYER_DEVICE_PROPERTY *property);
S32 iPodCoreFuncGetNumEQ(U32 devID, U32 *maxEQ);


/* Database Operation Function */
S32 iPodCoreFuncGetLowerCatList(U32 devID, IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg);
S32 iPodCoreFuncGetDBEntries(U32 devID, IPOD_PLAYER_DB_TYPE type, U32 start, S32 num, const IPOD_CB_RETRIEVE_CAT_DB_RECORDS callback);
S32 iPodCoreFuncGetDBCount(U32 devID, IPOD_PLAYER_DB_TYPE type, U32 *num);
S32 iPodCoreFuncSelectDBEntry(U32 devID, IPOD_PLAYER_DB_TYPE type, S32 entry);
S32 iPodCoreFuncCancel(U32 devID, IPOD_PLAYER_CANCEL_TYPE type);
S32 iPodCoreFuncClearSelection(U32 devID, IPOD_PLAYER_DB_TYPE type, IPOD_PLAYER_DB_TYPE topType, IPOD_PLAYER_DB_TYPE curType);
S32 iPodCoreFuncSelectAV(U32 devID, U8 avType);

S32 iPodCoreFuncSendToApp(U32 devID, U32 handle, U32 dataSize, U8 *appName);
S32 iPodCoreFuncSetiOSAppInfo(U8 *deviceName, IPOD_PLAYER_IOSAPP_INFO *info, U32 count);
S32 iPodCoreFuncRequestAppStart(U32 devID, U8 *appName);
S32 iPodCoreFuncOpenSongTag(U32 devID, IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, U32 optionsBit, U32 optionLen, const U8 *optionData, U32 *tagHandle);
S32 iPodCoreFuncCloseSongTag(U32 devID, IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, U32 tagHandle);
S32 iPodCoreFuncWriteTagging(U32 devID, IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, U32 tagHandle);
S32 iPodCoreFuncSongTag(U32 devID, IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, U32 handle, IPOD_PLAYER_TAG_TYPE type, const IPOD_PLAYER_TAG_INFO *info);

/* Internal Function */
S32 iPodCoreFuncDevInit(U8 *deviceName, IPOD_PLAYER_DEVICE_TYPE devType);
S32 iPodCoreFuncDevDeinit(U32 devID);
S32 iPodCoreFuncAccSample(U32 devID);
S32 iPodCoreFuncEndForward(U32 devID);
S32 iPodCoreFuncGetCaps(U32 devID, U64 *totalSpace, U32 *maxFileSize, U16 *maxWriteSize);
S32 iPodCoreFuncIntGetStatus(U32 devID, U8 *status, U32 *pos);
S32 iPodCoreFuncIntGetVideoSetting(U32 devID, IPOD_PLAYER_VIDEO_SETTING *setting);
S32 iPodCoreFuncIntGetTrackInfo(U32 devID, U32 mask, U32 *trackIndex, IPOD_PLAYER_TRACK_INFO *info);
S32 iPodCoreFuncIntGetCurrentTrackIndex(U32 devID, U32 *trackIndex);
S32 iPodCoreFuncIntGetCurrentChapterIndex(U32 devID, U32 *chapterIndex, U32 *chapterTotal);
S32 iPodCoreFuncIntGetCurrentChapterStatus(U32 devID, U32 chapterIndex, U32 *position);

/* iPodOut Functions */
S32 iPodCoreFuncGetiPodOutOptions(U32 iPodID, U8 types, U32 *optionsBits);
S32 iPodCoreFuncSetiPodOutOptions(U32 iPodID, U32 optionsBits);
S32 iPodCoreFuncDevStateChangeEvent(U32 iPodID, U8 status);
S32 iPodCoreFuncOutButtonStatus(U32 iPodID, U8 source, U32 statusBits);
S32 iPodCoreFuncRotationInputStatus(U32 iPodID, U32 durationMs, IPOD_PLAYER_HMI_ROTATION_INFO rotation, U16 move);


#endif


