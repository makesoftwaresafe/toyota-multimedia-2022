/*
 * iap2_smoketest.h
 */

#ifndef IAP2_SMOKETEST_H
#define IAP2_SMOKETEST_H

/* **********************  includes  ********************** */
#include <iap2_init.h>
#include "iap2_test_config.h"
#include <iap2_test_utility.h>
#include <iap2_test_init.h>
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

#include <sys_time_adit.h>
#include <pthread_adit.h>
#include <sys/poll.h>
#include <mqueue.h>

#ifndef BUILDENV_KP
#include <gst/gst.h>

#endif

/* **********************  defines  ********************** */

#define IAP2_ENABLE_BLUETOOTH_COMPONENT_TEST                    1
#define IAP2_ENABLE_VEHICLE_INFORMATION_COMPONENT_TEST          1
#define IAP2_ENABLE_VEHICLE_STATUS_COMPONENT_TEST               1
#define IAP2_ENABLE_LOCATION_INFORMATION_COMPONENT_TEST         1
#define IAP2_ENABLE_REQUEST_APP_LAUNCH_TEST                     1
#define IAP2_ENABLE_FILE_TRANSFER_CANCEL_PAUSE_CMD              0

/* playtime duration to test audio streaming
 * and to test playback settings / interactions */
#define TEST_PLAYBACK_TIME_MS                   30000

/* **********************  variables  ********************* */

/* **********************  functions  ********************* */
void iap2InitStackCallbacks(iAP2StackCallbacks_t* iap2StackCb);
void iap2InitCSCallbacks(iAP2SessionCallbacks_t* iap2CSCallbacks,BOOL iAP2EAPSupported);
void iap2InitFileTransferCallbacks(iAP2FileTransferCallbacks_t* iAP2FileTransferCallbacks);
S32 iap2SetInitialParameter(iAP2InitParam_t* iAP2InitParam, iap2UserConfig_t iAP2UserConfig);

#endif /* IAP2_SMOKETEST_H */
