/************************************************************************
 * \file: iap2_usb_gadget_load_modules.h
 *
 * \version: $ $
 *
 * This header file declares functions required for usb role switch.
 *
 * \component: global definition file
 *
 * \author : Manavalan Veeramani/RBEI/ manavalan.veeramani@in.bosch.com
 *
 * \copyright: (c) 2010 - 2013 ADIT Corporation
 *
 ***********************************************************************/

#include <adit_typedef.h>
#include <stdio.h>

/* **********************  defines   ********************** */
#define MAX_STRING_LEN              256
#define MAX_LEN                     24

/* Kernel 3.14 necessary kernel modules to load libcomposite and g_ffs */
#define CONFIGFS_MODULE_NAME        "configfs"
#define USB_F_FS_MODULE_NAME        "usb_f_fs"
#define U_ETHER_MODULE_NAME         "u_ether"
#define USB_F_NCM_MODULE_NAME       "usb_f_ncm"
#define USB_F_UAC2                  "usb_f_uac2"

#define LIBCOMPOSITE_MODULE_NAME    "libcomposite"
#define GADGET_FFS_MODULE_NAME      "g_ffs"
#define USB_F_UAC2                  "usb_f_uac2"
#define U_AUDIO                     "u_audio"

#define IAP2_USB_EXPORTED_SYMBOL __attribute__((visibility("default")))

typedef struct
{
    char vendorId[MAX_LEN];
    char productId[MAX_LEN];
    char manufacturer[MAX_LEN];
    char name[MAX_LEN];
    char serial[MAX_LEN];
    char bcdDevice[MAX_LEN];
    char qmult[MAX_LEN];
} iap2LoadGadgetFSModuleParameters_t;

typedef union
{
    iap2LoadGadgetFSModuleParameters_t gadget_fs;
} iap2LoadModuleParameters;

IAP2_USB_EXPORTED_SYMBOL S32 iap2LoadModule(const iap2LoadModuleParameters* iap2ModuleLoadParam, const char* modname, S32 length);
IAP2_USB_EXPORTED_SYMBOL S32 iap2UnloadModule(const char* modname);
