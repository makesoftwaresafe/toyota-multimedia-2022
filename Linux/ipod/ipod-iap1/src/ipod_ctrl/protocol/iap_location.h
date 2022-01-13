#ifndef IAP_LOCATION_H
#define IAP_LOCATION_H

#include <adit_typedef.h>
#include "iap_types.h"
#include "ipodcommon.h"
#include "iap_util_func.h"
#include "iap_general.h"

#ifdef __cplusplus
extern "C" {
#endif

#define IPOD_RET_DEV_CAPS_LEN 3
#define IPOD_RET_DEV_DATA_LEN 14
#define IPOD_ASYNC_DEV_DATA 14
#define IPOD_TOTAL_SIZE_LEN 4
#define IPOD_LOCATION_MAX_PAYLOAD 500
#define IPOD_LOCATION_SYSCAPS_MASK 0x04
#define IPOD_LOCATION_LOCCAPS_MASK 0x03
S32 iPodDevAck(U32 iPodID, S32 status, U8 cmdIDOrig, const U16 sectCur, U8 multiFlg);
S32 iPodRetDevCaps(U32 iPodID, IPOD_LOCATION_TYPE locType, U8 *capsData, U8 size);
S32 iPodRetDevControl(U32 iPodID, IPOD_LOCATION_TYPE locType, U64 ctlData);
S32 iPodRetDevData(U32 iPodID, IPOD_LOCATION_TYPE locType, U8 dataType, U8 *locData, U32 locSize);
S32 iPodAsyncDevData(U32 iPodID, IPOD_LOCATION_TYPE locType, U8 dataType, U32 totalSize, U8 *locData);

#ifdef __cplusplus
}
#endif

#endif /* IAP_LOCATION_H */
