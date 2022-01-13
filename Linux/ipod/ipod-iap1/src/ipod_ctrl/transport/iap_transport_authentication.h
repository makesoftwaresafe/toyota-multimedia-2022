#ifndef IAP_TRANSPORT_AUTHENTICATION_H
#define IAP_TRANSPORT_AUTHENTICATION_H

#include "authentication.h"
#include "iap_transport_message.h"

#ifdef __cplusplus
    extern "C" {
#endif

S32  iPodCreateWorkerTask(void);
void iPodDeleteWorkerTask(void);
void iPodWorkerIdentify(IPOD_INSTANCE* iPodHndl, BOOL isRequestidentify);
void iPodWorkerSecondIdentify(void);
S32  iPodWorkerWaitForEvent(S32 timeout);
void iPodWorkerSetForEvent(IPOD_INSTANCE* iPodHndl, U32 flg);
void iPodWorkerStopAuthentication(IPOD_INSTANCE* iPodHndl);
void iPodWorkerResume(void);
U8 iPodGetiPodDetected(void);
void iPodWorkerExeCB(IPOD_INSTANCE* iPodHndl, S32 err);

#ifdef __cplusplus
}
#endif

#endif /* IAP_TRANSPORT_AUTHENTICATION_H */
