/*
 * iap2_smoketest_defines.h
 *
 *  Created on: Jul 31, 2014
 *      Author: dgirnus
 */

#ifndef IAP2_SMOKETEST_DEFINES_H_
#define IAP2_SMOKETEST_DEFINES_H_


/* **********************  includes  ********************** */

#include <adit_typedef.h>
#include <sys_time_adit.h>
#include <pthread_adit.h>


/* **********************  defines  ********************** */

#define MAX_DEVICES                                         8
#define MAX_STRING_LEN                                      256

#define TEST_THREAD_ID                                      pthread_t

/* 2 second timeout */
#define TEST_MQ_RECV_TMO                                    5000
#define TEST_MQ_MAX_SIZE                                    1024

#define TEST_MQ_NAME                                        "/TestMq"
#define TEST_MQ_NAME_APP_TSK                                "/TestAppTskMq"

#define TEST_MQ_CMD_STOP_POLL                               "mqStopPoll"

/* necessary for udev enumeration */
#define DEV_DETECT_CFG_STRING_MAX                           256
#define DEV_DETECT_VENDOR_MAX_LEN                           64
#define IPOD_SYSATTR_IDVENDOR                               "idVendor"
#define IPOD_APPLE_IDVENDOR                                 "05ac"
#define IPOD_APPLE_IDPRODUCT_MIN                            "1200"
#define IPOD_USB_MONITOR_DEVTYPE                            "usb"
#define IPOD_USB_FILTER_TYPE                                "usb_device"
#define IPOD_USB_MONITOR_LINK                               "udev"
#define IPOD_USB_ACTION_ADD                                 "add"
#define IPOD_USB_SELECT_RETRY_CNT                           5

/* necessary for device power request */
#define DEV_POWER_REQ_VEN_bmREQ                             (0x40)
#define DEV_POWER_REQ_VEN_REQ                               (0x40)
#define DEV_POWER_REQ_VEN_VAL                               (500)
#define DEV_POWER_REQ_VEN_IX                                (500)

#endif /* IAP2_SMOKETEST_DEFINES_H_ */
