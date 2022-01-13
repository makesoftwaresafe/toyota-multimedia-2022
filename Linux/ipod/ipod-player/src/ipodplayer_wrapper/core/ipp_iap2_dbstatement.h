#ifndef IPP_IAP2_DBSTATEMENT_H
#define IPP_IAP2_DBSTATEMENT_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>

#include "adit_typedef_linux.h"
#include "iap2_defines.h"
#include "iPodPlayerCoreDef.h"

U32 ippiAP2GetMediaType(IPOD_PLAYER_IAP2_DB_CATLIST *catList);
BOOL ippiAP2DBCheckCategory(IPOD_PLAYER_IAP2_DB_CATLIST *catList, char *checkCategory);

void ippiAP2GenerateStatement(U32 length, U8 *statement, iAP2PlaybackAttributes *item);
U8 *ippiAP2DBGenerateGetPlaybackStatusStatement(void);
U8 *ippiAP2DBGenerateGetMediaLibraryInformationStatement(sqlite3* handle, const U8 *key);

U8* ippiAP2DBGenerateGetCountStatement(const U8 *table);
U8 *ippiAP2DBGenerateGetListStatement(IPOD_PLAYER_DB_TYPE type, IPOD_PLAYER_IAP2_DB_CATLIST *catList, U32 start, S32 count);
U8 *ippiAP2DBGenerateCountStatement(const U8 *table, IPOD_PLAYER_DB_TYPE type, IPOD_PLAYER_IAP2_DB_CATLIST *catList);
U8 *ippiAP2DBGenerateGetCategoryIDStatement(IPOD_PLAYER_DB_TYPE type, U32 catIndex, IPOD_PLAYER_IAP2_DB_CATLIST *catList);
U8 *ippiAP2DBGenerateSetCategoryIDStatement(IPOD_PLAYER_DB_TYPE type, U32 catIndex, U64 id);
U8 *ippiAP2DBGenerateClearSelectingCategory(IPOD_PLAYER_DB_TYPE type);
U8 *ippiAP2DBGenerateGetMediaItemIDFromMediaItemStatement(IPOD_PLAYER_DB_TYPE type, IPOD_PLAYER_IAP2_DB_CATLIST *catList);
U8 *ippiAP2DBGenerateGetMediaItemIDFromNowPlayingStatement(IPOD_PLAYER_DB_TYPE type);
U8 *ippiAP2DBGenerateSetSampleStatement(U32 rate);
U8 *ippiAP2DBGenerateSetAssistiveStatement(U32 assistiveID, IPOD_PLAYER_DEVICE_EVENT_ASSISTIVE_STATUS assistiveStatus);
U8 *ippiAP2DBGenerateGetAssistiveStatusStatement(U32 assistiveID);
U8 *ippiAP2DBGenerateGetTrackIDListFromNowPlayingStatement(U64 trackIndex, U32 count);
U8 *ippiAP2DBGenerateGetTrackInfoListStatement(IPOD_PLAYER_IAP2_DB_HANDLE *dbHandle, U64 trackIndex, U32 count, U32 mediaType);
U8 *ippiAP2DBGenerateSetBluetoothStatusStatement(IAP2_BLUETOOTH_INFO *btInfo);
U8 *ippiAP2DBGenerateGetBluetoothStatusStatement(U16 btID);
U8 *ippiAP2DBGenerateSetDeviceNameStatement(const U8 *key, const U8 *name);
U8 *ippiAP2DBGenerateGetTrackInfoTrackIDStatement(U64 *mediaId);
U8 *ippiAP2DBGenerateGetMediaTypeStatement(U64 *mediaId);

S32 ippiAP2CreateStatement(IPOD_PLAYER_DB_TYPE type, U8 **statement, U8 **orderstate, U16 length, 
                                                        U32 *mediaType, IPOD_PLAYER_IAP2_DB_CATLIST *catList);
BOOL ippiAP2DBCheckCategory(IPOD_PLAYER_IAP2_DB_CATLIST *catList, char *checkCategory);

sqlite3_stmt *ippiAP2DBGenerateSetMediaLibraryInformationStatement(sqlite3* handle, const U8 *key, iAP2MediaLibraryInformationSubParameter *media);
sqlite3_stmt *ippiAP2DBGenerateSetRevisionStatement(sqlite3* handle, const U8 *key, const U8 *revision, U32 progress);
sqlite3_stmt *ippiAP2DBGenerateSetNowPlayingItemID(sqlite3* handle);
sqlite3_stmt *ippiAP2DBGenerateDeleteMediaItemStatement(sqlite3 *handle);
sqlite3_stmt *ippiAP2DBGenerateSetPlaylistStatement(sqlite3* handle);
sqlite3_stmt *ippiAP2DBGenerateSetIsHidingStatement(sqlite3* handle, const U8 *key, U8 isHiding);
sqlite3_stmt *ippiAP2DBGenerateDeletePlaylistStatement(sqlite3 *handle);
sqlite3_stmt *ippiAP2DBGenerateSetPlaylistTracksStatement(sqlite3* handle);
sqlite3_stmt *ippiAP2DBGenerateDeletePlaylistTracksStatement(sqlite3 *handle);


#endif /* IPP_IAP2_DBSTATEMENT_H */
