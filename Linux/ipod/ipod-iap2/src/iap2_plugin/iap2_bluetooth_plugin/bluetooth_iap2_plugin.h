#ifndef _IAP2_BLUETOOTH_PLUGIN_H
#define _IAP2_BLUETOOTH_PLUGIN_H

#ifdef __cplusplus
extern "C" {
#endif

#include "iap2_datacom.h"

#define IPOD_IAP2_BT_MAC_STRING_MAX 18

typedef struct _IAP2_BT_INFO
{
    U8* iAP2DeviceMAC;
    S32 iAP2BTSocket;
} IAP2_BT_INFO;

#endif /* _IAP2_BLUETOOTH_PLUGIN_H */

