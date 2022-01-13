#ifndef IAP_DIGITALAUDIO_H
#define IAP_DIGITALAUDIO_H

#include <adit_typedef.h>

#include "iap_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define IPOD_USB_SAMPLERATE_32K         {0x00, 0x00, 0x7D, 0x00}
#define IPOD_USB_SAMPLERATE_44K         {0x00, 0x00, 0xAC, 0x44}
#define IPOD_USB_SAMPLERATE_48K         {0x00, 0x00, 0xBB, 0x80}

void iPodAccAck(U32 iPodID, IPOD_ACC_ACK_STATUS status);
void iPodAccAckDevice(IPOD_ACC_ACK_STATUS status, U32 iPodID);
void iPodUSBRetAccSampleCaps(U32 iPodID);
S32 iPodSetVideoDelay(U32 iPodID, U32 delay);

#ifdef __cplusplus
}
#endif

#endif /* IAP_DIGITALAUDIO_H */
