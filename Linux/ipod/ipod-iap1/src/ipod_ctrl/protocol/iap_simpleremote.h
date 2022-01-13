#ifndef IAP_SIMPLEREMOTE_H
#define IAP_SIMPLEREMOTE_H

#include <adit_typedef.h>

#include "iap_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define IPOD_OUT_STATUS_BASE_LEN 4
#define IPOD_ACC_EVENT_BASE_LEN 3

S32 iPodPlayPause(U32 iPodID);
S32 iPodNextTitle(U32 iPodID);
S32 iPodPreviousTitle(U32 iPodID);
S32 iPodNextAlbum(U32 iPodID);
S32 iPodPreviousAlbum(U32 iPodID);
S32 iPodVolumeUp(U32 iPodID);
S32 iPodVolumeDown(U32 iPodID);
S32 iPodOutButtonStatus(U32 iPodID, U8 source, U32 statusBits);
S32 iPodRotationInputStatus(U32 iPodID, IPOD_ROTATION_INFO rotation);
S32 iPodAccessibilityEvent(U32 iPodID, IPOD_ACC_EVENT_TYPE type, IPOD_ACC_EVENT_DATA data);
LOCAL S32 iPodButtonRelease(U32 iPodID);

#ifdef __cplusplus
}
#endif

#endif /* IAP_SIMPLEREMOTE_H */
