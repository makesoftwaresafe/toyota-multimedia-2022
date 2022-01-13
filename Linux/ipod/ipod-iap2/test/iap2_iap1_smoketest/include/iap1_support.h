/*
 * iap_smoketest.h
 *
 *  Created on: Jul 17, 2013
 *      Author: dgirnus
 */

#ifndef IAP_SMOKETEST_H_
#define IAP_SMOKETEST_H_


/* ----------------------  include headers  ---------------------- */

#include <adit_typedef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


/* ----------------------  defines & variables  ---------------------- */

/* encapsulate extended tests */
#define TEST_BASICS         1
#define TEST_DB_FUNC        0
#define TEST_PLAYBACK_FUNC  0
#define TEST_EXP_DB_FUNC    0
#define TEST_UID_LIST_FUNC  0
#define TEST_ARTWORK_FUNC   0
#define TEST_INIT_ACC_CON   0
#define TEST_CANCEL_DEV_CON 0
#define TEST_PERFORMANCE    0
#define TEST_IOS_APP        0
#define TEST_IPOD_SELF      0
#define SUBTEST_VIDEO_DB    0
#define SUBTEST_AUDIO_DB    0
#define TEST_AUTH_SELFTEST  1

/* playtime duration to test audio streaming
 * and to test playback settings / interactions */
#define TEST_PLAYBACK_TIME_MS       30000

/* connection states */
#define IPOD_DETACHED     -1
#define IPOD_NOT_DETECTED  0
#define IPOD_ATTACHED      1

/* necessary for udev enumeration */
#define DEV_DETECT_CFG_STRING_MAX   256
#define DEV_DETECT_VENDOR_MAX_LEN   64
#define IPOD_SYSATTR_IDVENDOR "idVendor"
#define IPOD_APPLE_IDVENDOR "05ac"
#define IPOD_APPLE_IDPRODUCT_MIN "1200"
#define IPOD_USB_MONITOR_DEVTYPE "usb"
#define IPOD_USB_FILTER_TYPE "usb_device"
#define IPOD_USB_MONITOR_LINK "udev"
#define IPOD_USB_ACTION_ADD "add"
#define IPOD_USB_SELECT_RETRY_CNT 5


#endif /* IAP_SMOKETEST_H_ */
