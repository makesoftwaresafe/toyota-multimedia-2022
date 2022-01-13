#ifndef IAP_DATABASE_H
#define IAP_DATABASE_H

#include <adit_typedef.h>

#include "iap_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================== */
/* defines                                                                    */
/* ========================================================================== */
#define IPOD_GET_ALL_CAT_ENTRIES -1
#define IPOD_GET_ALL 0xFFFFFFFF
#define IPOD_TRACK_INFO_BASE_LENGTH 12 
#define IPOD_UID_INFO_POS 8
#define IPOD_DB_PB_INFO_POS 4

#define BITMASK_MAX_LENGTH 3
/* ========================================================================== */
/* function prototypes                                                        */
/* ========================================================================== */
S32 iPodGetNumberCategorizedDBRecords(U32 iPodID, IPOD_CATEGORY category);
S32 iPodSelectSortDBRecords(U32 iPodID, IPOD_CATEGORY category,
                            U32 index,
                            IPOD_DB_SORT_TYPE sortType);
S32 iPodRetrieveCategorizedDBRecords(
                            U32 iPodID, 
                            IPOD_CATEGORY category,
                            U32 start,
                            S32 count,
                            const IPOD_CB_RETRIEVE_CAT_DB_RECORDS callback);
S32 iPodResetDBSelection(U32 iPodID);
S32 iPodSelectDBRecord(U32 iPodID, IPOD_CATEGORY category, U32 idx);
S32 iPodReturnToPreviousDBRecordSelection(U32 iPodID, IPOD_CATEGORY category);
S32 iPodPlayCurrentSelection(U32 iPodID, U32 idx);
S32 iPodResetDBSelectionHierarchy(U32 iPodID, U8 selection);

S32 iPodGetDBiTunesInfo(U32 iPodID, IPOD_ITUNES_METADATA_TYPE metadataType, IPOD_ITUNES_METADATA_INFO *metadataInfo);
S32 iPodGetUIDTrackInfo(U32 iPodID, U64 uID, IPOD_TRACK_INFORMATION_TYPE trackType, IPOD_TRACK_INFORMATION_CB callback);
S32 iPodGetDBTrackInfo(U32 iPodID, U32 dbIndex, S32 trackCount, IPOD_TRACK_INFORMATION_TYPE trackType, IPOD_TRACK_INFORMATION_CB callback);
S32 iPodGetPBTrackInfo(U32 iPodID, U32 pbIndex, S32 trackCount, IPOD_TRACK_INFORMATION_TYPE trackType, IPOD_TRACK_INFORMATION_CB callback);

S32 iPodGetBulkUIDTrackInfo(U32 iPodID, U64 uID, IPOD_TRACK_INFORMATION_BITFIELD trackType, IPOD_TRACK_INFORMATION_CB callback);
S32 iPodGetBulkDBTrackInfo(U32 iPodID, U32 dbIndex, S32 trackCount, IPOD_TRACK_INFORMATION_BITFIELD trackType, IPOD_TRACK_INFORMATION_CB callback);
S32 iPodGetBulkPBTrackInfo(U32 iPodID, U32 pbIndex, S32 trackCount, IPOD_TRACK_INFORMATION_BITFIELD trackType, IPOD_TRACK_INFORMATION_CB callback);

#ifdef __cplusplus
}
#endif

#endif /* IAP_DATABASE_H */
