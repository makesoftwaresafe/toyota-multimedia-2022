#ifndef IAP_STORAGE_H
#define IAP_STORAGE_H

#include <adit_typedef.h>
#include "iap_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define IPOD_OPEN_BASE_LENGTH 3
#define IPOD_WRITE_BASE_LENGTH 7
#define IPOD_WRITE_SIZE_LONG 2
#define IPOD_FILE_OPTION_LENGTH 4
#define IPOD_MAX_FILE_SIZE 128
#define IPOD_NOT_APPLICABLE 0xFF
#define IPOD_DEFAULT_MAJOR 1 /* Default major version of iPodRetDeviceCaps */
#define IPOD_DEFAULT_MINOR 2 /* Default minor version of iPodRetDeviceCaps */
#define IPOD_POS24 24
#define IPOD_POS25 25 
S32 iPodGetiPodCaps(U32 iPodID, IPOD_STORAGE_CAPS *storageCaps);
S32 iPodGetiPodFreeSpace(U32 iPodID, U64 *freeSpace);
S32 iPodOpeniPodFeatureFile(U32 iPodID, IPOD_FEATURE_TYPE featureType, IPOD_FILE_OPTIONS_MASK *bitMask, const U8* fileData, U8 fileSize, U8* fileHandle);
S32 iPodWriteiPodFileData(U32 iPodID, U32 offset, U8 handle, const U8* data, U16 length);
S32 iPodCloseiPodFile(U32 iPodID, U8 handle);
S32 iPodDeviceACK(U32 iPodID, U8 command, U8 status);
S32 iPodRetDeviceCaps(U32 iPodID, U8 major_ver, U8 minor_ver);

#ifdef __cplusplus
}
#endif

#endif
