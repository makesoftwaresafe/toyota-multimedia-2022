#ifndef _IPOD_PLAYER_CORE_FUNC_H
#define _IPOD_PLAYER_CORE_FUNC_H

void iPodCoreCBAttach(const S32 success, IPOD_CONNECTION_TYPE connection, const U32 iPodID);
void iPodCoreCBDetach(const U32 iPodID);
void iPodCoreCBGetCoverart(IPOD_ALBUM_ARTWORK* artworkData, U32 iPodID);
void iPodCoreCBGetTrackInfo(IPOD_TRACK_INFO_TYPE infoType, const IPOD_TRACK_CAP_INFO_DATA* capInfoData,
                                            const IPOD_TRACK_RELEASE_DATE_DATA* releaseData,
                                            const IPOD_TRACK_ARTWORK_COUNT_DATA* artworkCountData,
                                            U8* stringBuf,
                                            const U32 iPodID);
void iPodCoreCBGetEntries(U32 trackIndex, U8* string, const U32 iPodID);
void iPodCoreCBNotify(IPOD_CHANGED_PLAY_STATUS status, U64 param, const U32 iPodID);
void iPodCoreCBNotifySamplerate(U32 newSample, S32 newSound, S32 newVolume, U32 iPodID);
void iPodCoreCBNotifyPlayingTrackInfo(IPOD_TRACK_INFO_TYPE infoType, const IPOD_TRACK_CAP_INFO_DATA* capInfoData, 
                        const IPOD_TRACK_RELEASE_DATE_DATA* releaseData, const IPOD_TRACK_ARTWORK_COUNT_DATA* artworkCountData, U8* stringBuf, const U32 iPodID);
S32 iPodCoreCBOpenApp(U8 protocolIndex, U16 sessionId, const U32 iPodID);
void iPodCoreCBCloseApp(U16 sessionId, const U32 iPodID);
S32 iPodCoreCBReceiveFromApp(U16 sessionId, U8 *data, U16 length, const U32 iPodID);

S32 iPodCoreiPodCtrlRegisterCB(void);
void iPodCoreiPodCtrlDeleteCB(void);
S32 iPodCoreiPodCtrlNotifyPlaybackStatus(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, const IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents);
S32 iPodCoreiPodCtrlFuncInit(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_CORE_WAIT_LIST *waitData, IPOD_PLAYER_FUNC_HEADER *header, const IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents, U32 *size);
S32 iPodCoreiPodCtrlFuncDeInit(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_CORE_WAIT_LIST *waitData, IPOD_PLAYER_FUNC_HEADER *header, const IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents, U32 *size);
S32 iPodCoreiPodCtrlFuncSelectAudioOut(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_CORE_WAIT_LIST *waitData, IPOD_PLAYER_FUNC_HEADER *header, const IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents, U32 *size);
S32 iPodCoreiPodCtrlFuncStartAuthentication(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_CORE_WAIT_LIST *waitData, IPOD_PLAYER_FUNC_HEADER *header, const IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents, U32 *size);

S32 iPodCoreiPodCtrlStatusChangeToPlay(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, const IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents);
S32 iPodCoreiPodCtrlStatusChangeToPause(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, const IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents);
S32 iPodCoreiPodCtrlStatusChangeToStop(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, const IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents);
S32 iPodCoreiPodCtrlStatusChangeToFastForward(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, const IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents);
S32 iPodCoreiPodCtrlStatusChangeToRewind(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, const IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents);
S32 iPodCoreiPodCtrlPlay(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_CORE_WAIT_LIST *waitData, IPOD_PLAYER_FUNC_HEADER *header, const IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents, U32 *size);
S32 iPodCoreiPodCtrlPause(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_CORE_WAIT_LIST *waitData, IPOD_PLAYER_FUNC_HEADER *header, const IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents, U32 *size);
S32 iPodCoreiPodCtrlStop(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_CORE_WAIT_LIST *waitData, IPOD_PLAYER_FUNC_HEADER *header, const IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents, U32 *size);
S32 iPodCoreiPodCtrlFastForward(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_CORE_WAIT_LIST *waitData, IPOD_PLAYER_FUNC_HEADER *header, const IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents, U32 *size);
S32 iPodCoreiPodCtrlRewind(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_CORE_WAIT_LIST *waitData, IPOD_PLAYER_FUNC_HEADER *header, const IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents, U32 *size);
S32 iPodCoreiPodCtrlNextTrack(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_CORE_WAIT_LIST *waitData, IPOD_PLAYER_FUNC_HEADER *header, const IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents, U32 *size);
S32 iPodCoreiPodCtrlPrevTrack(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_CORE_WAIT_LIST *waitData, IPOD_PLAYER_FUNC_HEADER *header, const IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents, U32 *size);
S32 iPodCoreiPodCtrlNextChapter(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_CORE_WAIT_LIST *waitData, IPOD_PLAYER_FUNC_HEADER *header, const IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents, U32 *size);
S32 iPodCoreiPodCtrlPrevChapter(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_CORE_WAIT_LIST *waitData, IPOD_PLAYER_FUNC_HEADER *header, const IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents, U32 *size);
S32 iPodCoreiPodCtrlGotoTrackPosition(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_CORE_WAIT_LIST *waitData, IPOD_PLAYER_FUNC_HEADER *header, const IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents, U32 *size);
S32 iPodCoreiPodCtrlPlayTrack(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_CORE_WAIT_LIST *waitData, IPOD_PLAYER_FUNC_HEADER *header, const IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents, U32 *size);
S32 iPodCoreiPodCtrlSetAudioMode(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_CORE_WAIT_LIST *waitData, IPOD_PLAYER_FUNC_HEADER *header, const IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents, U32 *size);
S32 iPodCoreiPodCtrlStopAudio(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg);
S32 iPodCoreiPodCtrlAudioInit(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg);
void iPodCoreiPodCtrlAudioDeinit(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg);

S32 iPodCoreiPodCtrlSetMode(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_CORE_WAIT_LIST *waitData, IPOD_PLAYER_FUNC_HEADER *header, const IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents, U32 *size);
S32 iPodCoreiPodCtrlSetRepeat(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_CORE_WAIT_LIST *waitData, IPOD_PLAYER_FUNC_HEADER *header, const IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents, U32 *size);
S32 iPodCoreiPodCtrlSetShuffle(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_CORE_WAIT_LIST *waitData, IPOD_PLAYER_FUNC_HEADER *header, const IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents, U32 *size);
S32 iPodCoreiPodCtrlSetEqualizer(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_CORE_WAIT_LIST *waitData, IPOD_PLAYER_FUNC_HEADER *header, const IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents, U32 *size);
S32 iPodCoreiPodCtrlSetVideoDelay(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_CORE_WAIT_LIST *waitData, IPOD_PLAYER_FUNC_HEADER *header, const IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents, U32 *size);
S32 iPodCoreiPodCtrlSetVideoSetting(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_CORE_WAIT_LIST *waitData, IPOD_PLAYER_FUNC_HEADER *header, const IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents, U32 *size);
S32 iPodCoreiPodCtrlSetDisplayImage(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_CORE_WAIT_LIST *waitData, IPOD_PLAYER_FUNC_HEADER *header, const IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents, U32 *size);
S32 iPodCoreiPodCtrlSetPlaySpeed(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_CORE_WAIT_LIST *waitData, IPOD_PLAYER_FUNC_HEADER *header, const IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents, U32 *size);
S32 iPodCoreiPodCtrlSetTrackInfoNotification(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_CORE_WAIT_LIST *waitData, IPOD_PLAYER_FUNC_HEADER *header, const IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents, U32 *size);
S32 iPodCoreiPodCtrlSetEventNotification(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_CORE_WAIT_LIST *waitData, IPOD_PLAYER_FUNC_HEADER *header, const IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents, U32 *size);
S32 iPodCoreiPodCtrlGetVideoSetting(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_CORE_WAIT_LIST *waitData, IPOD_PLAYER_FUNC_HEADER *header, const IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents, U32 *size);
S32 iPodCoreiPodCtrlGetCoverartInfo(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_CORE_WAIT_LIST *waitData, IPOD_PLAYER_FUNC_HEADER *header, const IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents, U32 *size);
S32 iPodCoreiPodCtrlGetCoverart(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_CORE_WAIT_LIST *waitData, IPOD_PLAYER_FUNC_HEADER *header, const IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents, U32 *size);
S32 iPodCoreiPodCtrlGetPlaybackStatus(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_CORE_WAIT_LIST *waitData, IPOD_PLAYER_FUNC_HEADER *header, const IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents, U32 *size);
S32 iPodCoreiPodCtrlGetTrackInfo(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_CORE_WAIT_LIST *waitData, IPOD_PLAYER_FUNC_HEADER *header, const IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents, U32 *size);
S32 iPodCoreiPodCtrlGetChapterInfo(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_CORE_WAIT_LIST *waitData, IPOD_PLAYER_FUNC_HEADER *header, const IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents, U32 *size);
S32 iPodCoreiPodCtrlGetMode(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_CORE_WAIT_LIST *waitData, IPOD_PLAYER_FUNC_HEADER *header, const IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents, U32 *size);
S32 iPodCoreiPodCtrlGetRepeat(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_CORE_WAIT_LIST *waitData, IPOD_PLAYER_FUNC_HEADER *header, const IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents, U32 *size);
S32 iPodCoreiPodCtrlGetShuffle(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_CORE_WAIT_LIST *waitData, IPOD_PLAYER_FUNC_HEADER *header, const IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents, U32 *size);
S32 iPodCoreiPodCtrlGetPlaySpeed(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_CORE_WAIT_LIST *waitData, IPOD_PLAYER_FUNC_HEADER *header, const IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents, U32 *size);
S32 iPodCoreiPodCtrlGetTrackTotalCount(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_CORE_WAIT_LIST *waitData, IPOD_PLAYER_FUNC_HEADER *header, const IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents, U32 *size);
S32 iPodCoreiPodCtrlGetEqaulizer(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_CORE_WAIT_LIST *waitData, IPOD_PLAYER_FUNC_HEADER *header, const IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents, U32 *size);
S32 iPodCoreiPodCtrlGetEqualizerName(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_CORE_WAIT_LIST *waitData, IPOD_PLAYER_FUNC_HEADER *header, const IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents, U32 *size);
S32 iPodCoreiPodCtrlGetDeviceProperty(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_CORE_WAIT_LIST *waitData, IPOD_PLAYER_FUNC_HEADER *header, const IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents, U32 *size);
S32 iPodCoreiPodCtrlGetDeviceStatus(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_CORE_WAIT_LIST *waitData, IPOD_PLAYER_FUNC_HEADER *header, const IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents, U32 *size);
S32 iPodCoreiPodCtrlGetDBEntries(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_CORE_WAIT_LIST *waitData, IPOD_PLAYER_FUNC_HEADER *header, const IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents, U32 *size);
S32 iPodCoreiPodCtrlGetDBCount(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_CORE_WAIT_LIST *waitData, IPOD_PLAYER_FUNC_HEADER *header, const IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents, U32 *size);
S32 iPodCoreiPodCtrlSelectDBEntry(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_CORE_WAIT_LIST *waitData, IPOD_PLAYER_FUNC_HEADER *header, const IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents, U32 *size);
S32 iPodCoreiPodCtrlCancel(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_CORE_WAIT_LIST *waitData, IPOD_PLAYER_FUNC_HEADER *header, const IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents, U32 *size);
S32 iPodCoreiPodCtrlClearSelection(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_CORE_WAIT_LIST *waitData, IPOD_PLAYER_FUNC_HEADER *header, const IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents, U32 *size);
S32 iPodCoreiPodCtrlSelectAV(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_CORE_WAIT_LIST *waitData, IPOD_PLAYER_FUNC_HEADER *header, const IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents, U32 *size);
S32 iPodCoreiPodCtrlSendToApp(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_CORE_WAIT_LIST *waitData, IPOD_PLAYER_FUNC_HEADER *header, const IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents, U32 *size);
S32 iPodCoreiPodCtrlRequestAppStart(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_CORE_WAIT_LIST *waitData, IPOD_PLAYER_FUNC_HEADER *header, const IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents, U32 *size);
S32 iPodCoreiPodCtrlOpenSongTag(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_CORE_WAIT_LIST *waitData, IPOD_PLAYER_FUNC_HEADER *header, const IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents, U32 *size);
S32 iPodCoreiPodCtrlCloseSongTag(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_CORE_WAIT_LIST *waitData, IPOD_PLAYER_FUNC_HEADER *header, const IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents, U32 *size);
S32 iPodCoreiPodCtrlSongTag(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_CORE_WAIT_LIST *waitData, IPOD_PLAYER_FUNC_HEADER *header, const IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents, U32 *size);
S32 iPodCoreiPodCtrlSetVolume(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_CORE_WAIT_LIST *waitData, IPOD_PLAYER_FUNC_HEADER *header, const IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents, U32 *size);
S32 iPodCoreiPodCtrlGetVolume(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_CORE_WAIT_LIST *waitData, IPOD_PLAYER_FUNC_HEADER *header, const IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents, U32 *size);


S32 iPodCoreiPodCtrlIntGetTrackInfo(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_CORE_WAIT_LIST *waitData, IPOD_PLAYER_FUNC_HEADER *header, const IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents, U32 *size);
S32 iPodCoreiPodCtrlIntGetDeviceProperty(U32 devID, U32 mask, IPOD_PLAYER_DEVICE_PROPERTY *property);
S32 iPodCoreiPodCtrlIntGetCaps(U32 devID, U64 *totalSpace, U32 *maxFileSize, U16 *maxWriteSize);
S32 iPodCoreiPodCtrlIntGetNumEQ(U32 devID, U32 *maxEQ);

S32 iPodCoreiPodCtrlInitConnection(void);
S32 iPodCoreiPodCtrlDisconnect(void);
S32 iPodCoreiPodCtrlIntGetVideoSetting(U32 devID, IPOD_PLAYER_VIDEO_SETTING *setting);
S32 iPodCoreiPodCtrlEndForward(U32 devID);
S32 iPodCoreiPodCtrlIntGetStatus(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_CORE_WAIT_LIST *waitData, IPOD_PLAYER_FUNC_HEADER *header, const IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents, U32 *size);
S32 iPodCoreiPodCtrlStatusCheck(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, const IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents, U32 size);
S32 iPodCoreiPodCtrlNotifyStatus(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_CORE_WAIT_LIST *waitData, IPOD_PLAYER_FUNC_HEADER *header, const IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents, U32 *size);
S32 iPodCoreiPodCtrlRemoteEventNotification(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_CORE_WAIT_LIST *waitData, IPOD_PLAYER_FUNC_HEADER *header, const IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents, U32 *size);

S32 iPodCoreiPodCtrlWaitInit(IPOD_PLAYER_CORE_IPOD_DEVICE_INFO *iPodInfo);
void iPodCoreiPodCtrlWaitDeinit(IPOD_PLAYER_CORE_IPOD_DEVICE_INFO *iPodInfo);
S32 iPodCoreiPodCtrlTimedWait(S32,U32 *prevTime);
S32 iPodCoreiPodCtrlGetDBNumber(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, U32 devID, IPOD_PLAYER_DB_TYPE type, U32 *totalNum);

S32 iPodCoreiPodCtrlGetTimer(S32 fd, U32 *timems);
S32 iPodCoreiPodCtrlSetTimer(S32 fd, U32 timems, U32 interval);
S32 iPodCoreiPodCtrlTimerInit(S32 *fd, U32 *handleNum, S32 *handle);
S32 iPodCoreiPodCtrlTimerDeinit(S32 fd, U32 *handleNum, S32 *handle);
S32 iPodCoreSetHandle(S32 *handle, U32 *handleNum, S32 setHandle);
S32 iPodCoreClearHandle(S32 *handle, U32 *handleNum, S32 clearHandle);

S32 iPodCoreiPodCtrlHMISetSupportedFeature(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_CORE_WAIT_LIST *waitData, IPOD_PLAYER_FUNC_HEADER *header, const IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents, U32 *size);
S32 iPodCoreiPodCtrlHMIGetSupportedFeature(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_CORE_WAIT_LIST *waitData, IPOD_PLAYER_FUNC_HEADER *header, const IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents, U32 *size);
S32 iPodCoreiPodCtrlHMIButtonInput(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_CORE_WAIT_LIST *waitData, IPOD_PLAYER_FUNC_HEADER *header, const IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents, U32 *size);
S32 iPodCoreiPodCtrlHMIRotationInput(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_CORE_WAIT_LIST *waitData, IPOD_PLAYER_FUNC_HEADER *header, const IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents, U32 *size);
S32 iPodCoreiPodCtrlHMISetApplicationStatus(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_CORE_WAIT_LIST *waitData, IPOD_PLAYER_FUNC_HEADER *header, const IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents, U32 *size);

S32 iPodCoreiPodCtrlHMIStatusSend(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_CORE_WAIT_LIST *waitData, IPOD_PLAYER_FUNC_HEADER *header, const IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents, U32 *size);

S32 iPodCoreiPodCtrlHMIButtonStatusSend(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_CORE_WAIT_LIST *waitData, IPOD_PLAYER_FUNC_HEADER *header, const IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents, U32 *size);


#endif

