/************************************************************************
 * \file: iap2_cinemo_test.h
 *
 * \version: $ $
 *
 * This header file declares macros required performing smoketest for Cinemo Application.
 *
 * \component: ipod-cinemo
 *
 * \author: abirami.murugesan@in.bosch.com
 *
/*
 * This software has been developed by Advanced Driver Information Technology.
 * Copyright(c) 2019-2020 Advanced Driver Information Technology GmbH,
 * Advanced Driver Information Technology Corporation, Robert Bosch GmbH,
 * Robert Bosch Car Multimedia GmbH and DENSO Corporation.
 * All rights reserved.
 *
 ***********************************************************************/

#ifndef IAP2_CINEMO_TEST_H
#define IAP2_CINEMO_TEST_H

#include <stdint.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <libudev.h>
#include <sys/mount.h>
#include <adit_dlt.h>
#include <adit_typedef.h>

#include "iap2_defines.h"
#include "iap2_usb_role_switch.h"
#include "iap2_usb_gadget_load_modules.h"
#include "iap2_initialize_usb_gadget.h"
#include "iap2_init.h"
#include "iap2_dlt_log.h"


char c_srate[] = {"44100,48000"};
char p_srate[] = {"44100,48000"};

#define IAP2_ACC_INFO_NAME                                  "AmazingProduct"
#define IAP2_ACC_INFO_MODEL_IDENTIFIER                      "15697"
#define IAP2_ACC_INFO_MANUFACTURER                          "ADIT"
#define IAP2_ACC_INFO_SERIAL_NUM                            "12345678"
#define IAP2_ACC_INFO_FW_VER                                "1"
#define IAP2_ACC_INFO_HW_VER                                "1"
#define IAP2_ACC_INFO_VENDOR_ID                             "44311"
#define IAP2_ACC_INFO_PRODUCT_ID                            "1111"
#define IAP2_ACC_INFO_BCD_DEVICE                            "1"
#define IAP2_ACC_INFO_EP_INIT                               "/dev/ffs/ep0"
#define IAP2_ACC_INFO_PPUUID                                "abcd1234"

#define IPOD_USB_MONITOR_DEVTYPE                            "usb"
#define IPOD_SYSATTR_IDVENDOR                               "idVendor"
#define IPOD_APPLE_VENDORID                                 "05ac"
#define DEV_DETECT_VENDOR_MAX_LEN                            64
#define DEV_DETECT_CFG_STRING_MAX                            256
#define IPOD_APPLE_IDPRODUCT_MIN                            "1200"
#define IPOD_USB_MONITOR_LINK                               "udev"
#define IPOD_USB_FILTER_TYPE                                "usb_device"
#define IPOD_USB_SELECT_RETRY_CNT                            5
#define IPOD_USB_ACTION_ADD                                 "add"
#define IPOD_AUTH_MSEC                                       1000
#define IPOD_AUTH_NSEC                                       1000000
#define FUNCTION_FS_PATH                                    "/dev/ffs"
#define IAP2_CONFIGFS_MOUNT_LOCATION                        "/sys/kernel/config"
#define IPOD_APPLE_IDPRODUCT                                 0x12A8
#define IPOD_APPLE_IDVENDOR                                  0x05AC

#define CINEMO_PLUGIN_INIT_PATH                              "/home/root/sdk/usr/lib/cinemo"
#define CINEMO_LIBRARY_PATH                                  " :/usr/local/lib:/home/root/sdk/usr/lib"

#endif /* IAP2_CINEMO_TEST_H */
