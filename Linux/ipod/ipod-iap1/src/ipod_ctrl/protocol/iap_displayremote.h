#ifndef IAP_DISPLAYREMOTE_H
#define IAP_DISPLAYREMOTE_H

#include <adit_typedef.h>

#include "iap_types.h"

#ifdef __cplusplus
extern "C" {
#endif
#define IPOD_ARTWORK_FORMAT_SIZE            7

S32 iPodSetRemoteEventNotification(U32 iPodID, U32 eventMask);
S32 iPodSetTrackPosition(U32 iPodID, U32 trackPosition);

S32 iPodGetCurrentEQProfileIndex(U32 iPodID, U32 *index);
S32 iPodSetCurrentEQProfileIndex(U32 iPodID, U32 index, U8 restore);
S32 iPodGetNumEQProfiles(U32 iPodID, U32 *profileCount);
S32 iPodGetIndexedEQProfileName(U32 iPodID, U32 profileIndex, U8 *name, U32 size);
S32 iPodGetDRArtworkFormats(U32 iPodID, IPOD_ARTWORK_FORMAT* resultBuf, U16* resultCount);

S32 iPodGetDRTrackArtworkData(U32 iPodID, U32 trackIndex, U16 formatId, U32 timeOffset, const IPOD_CB_GET_ARTWORK callback);
S32 iPodGetDRPlayStatus(U32 iPodID, IPOD_PLAYER_STATE* state, U32 *trackIndex, U32* length, U32* position);
S32 iPodGetDRTrackArtworkTimes(U32 iPodID, U32 trackIndex, U16  formatId, U16  artworkIndex, U16  artworkCount, U16 *resultCount, U32 *buffer);

#ifdef __cplusplus
}
#endif

#endif /* IAP_SIMPLEREMOTE_H */
