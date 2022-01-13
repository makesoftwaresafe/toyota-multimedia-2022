#ifndef _IAP2_OVER_CARPLAY_PLUGIN_H
#define _IAP2_OVER_CARPLAY_PLUGIN_H

#ifdef __cplusplus
extern "C" {
#endif

#include <iap2_datacom.h>
#include <string.h>
#include <unistd.h>
#include <poll.h>

#define IPOD_IAP2_USBHOST_MAX_SEND_BYTES    65535
#define IAP2_OVER_CARPLAY_STRING_MAX        256
#define IAP2_OVER_CARPLAY "/tmp/iAP2_Over_CarPlay"

typedef struct _IAP2_OVER_CARPLAY_DEV_INFO
{
    S32 socket_fd;
    U8  SockPath[IAP2_OVER_CARPLAY_STRING_MAX];  /* Socket Address Path */
}IAP2_OVER_CARPLAY_DEV_INFO;

#ifdef __cplusplus
}
#endif

#endif /* _IAP2_OVER_CARPLAY_PLUGIN_H */

