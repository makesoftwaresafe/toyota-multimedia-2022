#ifndef IAP_TRANSPORT_PROCESS_MESSAGE_H
#define IAP_TRANSPORT_PROCESS_MESSAGE_H

#include "iap_transport_message.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
#define IPOD_WORKER_SIGNATURE 1
#define IPOD_WORKER_IDENTIFY 2
#define IPOD_WORKER_PREFERENCE 3
*/
#define IPOD_STORAGE_LINGO_MIN_SUPPORTED_CMDID 0x83
#define IPOD_STORAGE_LINGO_MAX_SUPPORTED_CMDID 0xFF
#define IPOD_LOCATION_CAPS_MAX_SIZE 18
#define IPOD_LOCATION_CAPS_DATA_SIZE 18
#define IPOD_LOCATION_GPS_SIZE 8
#define IPOD_LOCATION_ASSIST_SIZE 8
#define IPOD_LOCATION_DEVCONTROL_SIZE 8
#define IPOD_LOCATION_SUPPORT_MAJ_VER 1
#define IPOD_LOCATION_SUPPORT_MINOR_VER 0
#define IPOD_LOCATION_SUPPORT_NMEA 1
#define IPOD_LOCATION_DEVDATA_SIZE 4
#define IPOD_LOCATION_DATA_TYPE_MAX_REFRESH 0x03
#define IPOD_LOCATION_DATA_TYPE_RECOM_REFRESH 0x04
#define IPOD_LOCATION_TOTALSIZE_LEN 4
#define IPOD_LOCATION_SYS_CAPS_MAX_LEN 18
#define IPOD_CONVERT_IPOD_ERROR 90
#define IPOD_IDPS_WAIT_TMO  500
#define IPOD_IDPS_RETRY_COUNT 12
#define IPOD_IDPS_ERR_TMOUT 5
#define IPOD_STORAGE_CURRENT_SUPPORT_MAJOR 0x01
#define IPOD_STORAGE_CURRENT_SUPPORT_MINOR 0x02
#define IPOD_GENERAL_CERT_INDEX_LEN 4
#define IPOD_GENERAL_CERT_TOTAL_LEN 6
#define IPOD_SPI_OPEN_RETRY_COUNT 5
#define IPOD_SPI_OPEN_WAIT_TIME 200
#define IPOD_BT_MAC_ADDRESS_LEN 6

void iPodProcessGeneralLingoCommand(IPOD_INSTANCE* iPodHndl, MessageHeaderInfoType MessageHeaderInfo);
void iPodProcessDisplayRemoteLingoCommand(IPOD_INSTANCE* iPodHndl, MessageHeaderInfoType MessageHeaderInfo);
void iPodProcessExtendedLingoCommand(IPOD_INSTANCE* iPodHndl, MessageHeaderInfoType MessageHeaderInfo);
void iPodProcessStorageLingoCommand(IPOD_INSTANCE* iPodHndl, MessageHeaderInfoType MessageHeaderInfo);
void iPodProcessLocationLingoCommand(IPOD_INSTANCE* iPodHndl, MessageHeaderInfoType MessageHeaderInfo);

void iPodProcessAudioLingoCommand(IPOD_INSTANCE* iPodHndl, MessageHeaderInfoType MessageHeaderInfo);

#ifdef __cplusplus
}
#endif

#endif
