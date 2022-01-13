#ifndef IAP_IPODOUT_H
#define IAP_IPODOUT_H

#include <adit_typedef.h>
#include "iap_types.h"

#ifdef __cplusplus
extern "C" {
#endif

S32 iPodGetiPodOutOptions(U32 iPodID, U8 options, U32 *optionsBits);
S32 iPodSetiPodOutOptions(U32 iPodID, U32 optionsBits);
S32 iPodDevStateChangeEvent(U32 iPodID, U8 status);
S32 iPodDevVideoScreenInfo(U32 iPodID, IPOD_VIDEO_SCREEN_INFO info);

#ifdef __cplusplus
}
#endif

#endif /* IAP_IPODOUT_H */
