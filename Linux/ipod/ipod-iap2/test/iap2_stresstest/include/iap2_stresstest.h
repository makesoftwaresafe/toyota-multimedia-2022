/*
 * iap2_stresstest.h
 */

#ifndef IAP2_STRESSTEST_H
#define IAP2_STRESSTEST_H

/* **********************  includes  ********************** */
#include "iap2_test_utility.h"
#include "iap2_test_defines.h"
#include "iap2_test_config.h"
#include "iap2_test_init.h"

#include <iap2_init.h>

#include <adit_typedef.h>
#include <adit_dlt.h>
#include <libudev.h>

#include <stdio.h>
#include <stdlib.h>
#include <sys/unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>

#include <sys/syscall.h>
#include <sys/utsname.h>
#include <sys/mount.h>
#include <sys_time_adit.h>
#include <pthread_adit.h>
#include <sys/poll.h>
#include <mqueue.h>

#include <gst/gst.h>

/* **********************  defines  ********************** */

#ifndef BUILDENV_KP
#define IAP2_GST_AUDIO_STREAM                                   1
#endif

/* encapsulate extended tests */
#define IAP2_ENABLE_BLUETOOTH_COMPONENT_TEST                    0
#define IAP2_ENABLE_VEHICLE_INFORMATION_COMPONENT_TEST          0
#define IAP2_ENABLE_VEHICLE_STATUS_COMPONENT_TEST               0
#define IAP2_ENABLE_LOCATION_INFORMATION_COMPONENT_TEST         0
#define IAP2_ENABLE_REQUEST_APP_LAUNCH_TEST                     1
#define IAP2_ENABLE_FILE_TRANSFER_CANCEL_PAUSE_CMD              0

#define IAP2_ENABLE_PLAYBACK_QUEUE_LIST_PRINTING                0

/* playtime duration to test audio streaming
 * and to test playback settings / interactions */
#define TEST_PLAYBACK_TIME_MS                                   30000

/* **********************  variables  ********************* */


/* **********************  functions  ********************* */
S32 iap2HdlComThreadPollMqEvent_CB(iAP2Device_t* iap2device, S32 mqFD, S32 mqFdSendAck, BOOL* b_endComThread);
S32 iap2SetInitialParameter(iAP2InitParam_t* iAP2InitParam, iap2UserConfig_t iAP2UserConfig);
void iap2InitStackCallbacks(iAP2StackCallbacks_t* iap2StackCb);
void iap2InitCSCallbacks(iAP2SessionCallbacks_t* iap2CSCallbacks);
void iap2InitEAPSessionCallbacks(iAP2EAPSessionCallbacks_t* iAP2EAPSessionCallbacks);
void iap2InitEANativeTransportCallbacks(iAP2EANativeTransportCallbacks_t* iAP2EANativeTransportCallbacks);
void iap2InitFileTransferCallbacks(iAP2FileTransferCallbacks_t* iAP2FileTransferCallbacks);


#endif /* IAP2_STRESSTEST_H */
