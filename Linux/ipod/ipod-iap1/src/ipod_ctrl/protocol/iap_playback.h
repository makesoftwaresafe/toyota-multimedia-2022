#ifndef IAP_PLAYBACK_H
#define IAP_PLAYBACK_H

#include <adit_typedef.h>

#include "iap_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* byte sizes in iPod response ---------------------------------------------- */
#define IPOD_TRACK_ARTWORK_COUNT_DATA_SIZE 4
#define IPOD_ERROR_CODE_MAX 7
#define IPOD_ERRO_CODE_MIN 0
#define IPOD_HANDLE_RESPONSE_STRING_COUNT 256
#define IPOD_MORE_PACKET 3
#define IPOD_SHUFFLE_REPEAT_BASE_LENGTH 5

S32 iPodPrepareUIDList(U32 iPodID, U64* uidList, U32 uidCount);

S32 iPodPlayPreparedUIDList(U32 iPodID, U64 trackUID);

S32 iPodPlayToggle(U32 iPodID);

S32 iPodPlayStop(U32 iPodID);

S32 iPodPlayNextTrack(U32 iPodID);

S32 iPodPlayPrevTrack(U32 iPodID);

S32 iPodPlayFastForward(U32 iPodID);

S32 iPodPlayFastBackward(U32 iPodID);

S32 iPodPlayNormal(U32 iPodID);

S32 iPodPlayNextChapter(U32 iPodID);

S32 iPodPlayPrevChapter(U32 iPodID);

S32 iPodSetShuffle(U32 iPodID, IPOD_SHUFFLE_MODE shuffleMode, BOOL persistent);

S32 iPodShuffleOff(U32 iPodID);

S32 iPodShuffleOnSongs(U32 iPodID);

S32 iPodShuffleOnAlbums(U32 iPodID);

S32 iPodGetShuffleMode(U32 iPodID);

S32 iPodSetRepeat(U32 iPodID, IPOD_REPEAT_MODE repeatMode, BOOL persistent);

S32 iPodRepeatOff(U32 iPodID);

S32 iPodRepeatCurrentSong(U32 iPodID);

S32 iPodRepeatAllSongs(U32 iPodID);

S32 iPodGetRepeatMode(U32 iPodID);

S32 iPodGetAudioBookSpeed(U32 iPodID);

S32 iPodSetAudioBookSpeed(U32 iPodID, IPOD_AUDIOBOOK_SPEED speed);

S32 iPodGetNumPlayingTracks(U32 iPodID);

S32 iPodSetCurrentPlayingTrack(U32 iPodID, U32 trackIndex);

S32 iPodSetCurrentPlayingTrackChapter(U32 iPodID, U32 chapterIndex);

S32 iPodGetCurrentPlayingTrackChapterName(U32 iPodID, U32 chapterIndex, 
                                           U8* chapterName);

S32 iPodGetCurrentPlayingTrackIndex(U32 iPodID);

S32 iPodGetCurrentPlayingTrackChapterInfo(U32 iPodID, S32* chapterIndex, S32* chapterCount);

S32 iPodGetPlayStatus(U32 iPodID, IPOD_PLAYER_STATE* state, U32* length, U32* position);

S32 iPodSetPlayStatusChangeNotification(U32 iPodID, IPOD_STATUS_CHANGE_NOTIFICATION mode);

S32 iPodExtendedSetPlayStatusChangeNotification(U32 iPodID, IPOD_EXTENDED_STATUS_CHANGE_NOTIFICATION mode);

S32 iPodGetIndexedPlayingTrackTitle(U32 iPodID, U32 currIndex, U8* songTitle);

S32 iPodGetIndexedPlayingTrackArtistName(U32 iPodID, U32 currIndex, U8* artistName);

S32 iPodGetIndexedPlayingTrackAlbumName(U32 iPodID, U32 currIndex, U8* albumName);                                     

S32 iPodGetCurrentPlayingTrackChapterPlayStatus(U32 iPodID, U32 chapterIndex, 
                                                U32* chapterLength,
                                                U32* elapsedTime);
S32 iPodGetIndexedPlayingTrackInfo(U32 iPodID, IPOD_TRACK_INFO_TYPE info, 
                                    U32 trackIndex, 
                                    U16 chapterIndex, 
                                    const IPOD_CB_PLAYING_TRACK_INFO callback);                                                                                           
S32 iPodGetIndexedPlayingTrackGenre(U32 iPodID, U32 trackIndex,
                                    U16 chapterIndex,
                                    U8* artistName);

S32 iPodPlayControl(U32 iPodID, IPOD_PLAY_CONTROL ctrl);

#define IPOD_LAST_PACKET 0x03

#ifdef __cplusplus
}
#endif

#endif /* IAP_PLAYBACK_H */
