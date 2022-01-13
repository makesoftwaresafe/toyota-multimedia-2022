/**
* \file: iap2_usb_role_switch_smoketest.c
*
* \version: $Id:$
*
* \release: $Name:$
*
* <brief description>.
* <detailed description>
* \component: iAP2 USB Role Switch
*
* \author: J. Harder / ADIT/SW1 / jharder@de.adit-jv.com
*
* \copyright (c) 2013 Advanced Driver Information Technology.
* This code is developed by Advanced Driver Information Technology.
* Copyright of Advanced Driver Information Technology, Bosch, and DENSO.
* All rights reserved.
*
* \see <related items>
*
* \history
*
***********************************************************************/


#include <adit_typedef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/file.h>
/* to use mkdir / rmdir */
#include <sys/stat.h>
/* to use mount / umount */
#include <sys/mount.h>
/* to find directories */
#include <glob.h>
/* to load/unload kernel modules */
#include <libkmod.h>

#include <iap2_usb_gadget_load_modules.h>
#include <iap2_usb_role_switch.h>

/* temporary general logging functions; similar to TR_ loggings */
#define LOG_CTX_USB_ROLE_SWITCH_SMOKE
#define LOG_REGISTER(log_class)
#define LOG_UNREGISTER(log_class)
#define LOG_E(log_class, log_string, ...) printf(log_string, ##__VA_ARGS__);
#define LOG_S(log_class, log_string, ...) printf(log_string, ##__VA_ARGS__);
#define LOG_U1(log_class, log_string, ...) printf(log_string, ##__VA_ARGS__);

/* system nodes used to switch USB OTG port (host|gadget) and to control VBUS power (on|off) */
#ifdef IPOD_ARCH_ARM
#define USB_OTG_GLOB                            "/sys/devices/soc0/soc.0/21*/2184*/*"
#define VBUS_POWER                              "/sys/class/udc/*/device/vbus"
#else
#define USB_OTG_GLOB                            "/sys/bus/platform/devices/intel-mux-drcfg/portmux.0/state"
#define VBUS_POWER                              "/dev/cbc-raw0"
#endif

/* USB OTG roles (host|gadget) */
#define STR_GADGET                              "gadget"
#define STR_HOST                                "host"
/* kernel modules to load/unload for USB Host Mode communication */
#define LIBCOMPOSITE_MODULE_NAME                "libcomposite"
#define GADGET_FFS_MODULE_NAME                  "g_ffs"
/* device nodes of the function filesystem */
#define FUNCTION_FS_NAME                        "ffs"
#define FUNCTION_FS_PATH                        "/dev/ffs"
#define FUNCTION_FS_TYPE                        "functionfs"

/* configuration values to load the g_ffs */
#define ACC_VENDOR_ID                           "44311"
#define ACC_PRODUCT_ID                          "1111"
#define ACC_MANUFACTURER                        "ADIT"
#define ACC_NAME                                "AmazingProduct"
#define ACC_SERIAL_NUM                          "12345678"
#define ACC_BCD_DEVICE                          "1"
#define ACC_QMULT                               "1"

#define IAP2APPSENDSWITCHMSG_STRING_CP(dest, src) \
{ \
    if (strlen((char*)src) >= (sizeof(dest)/sizeof(dest[0]))) { \
        printf ("%s is to large to be copied to dest\n", #src ); \
        status = -1; \
    } else {\
        strcpy ((void*)dest, (void*)src); \
    } \
}

/* local functions */
static void  _show();
static int   _switchOTG(const char* otgGlob, const char* value);
static int   _switchVbusPower(const char* power, const char* value);
static char* _findVbus(void);

#ifdef IPOD_ARCH_ARM
static int   _findOTGPath(char* otgPath, unsigned int len, const char* otgGlob);
static int   _commonWrite(const char* path, const char* subPath, const char* value, BOOL checkBeforeWrite);
static int   _commonWriteValue(const char* path, const char* value);
#else
static BOOL  _accessFs(const char* path, void *value, ssize_t len, BOOL readWrite);
#endif /* IPOD_ARCH_ARM */

int main(int argc, const char** argv)
{
    int status = 0;
    char *vbus = NULL;
    char *mount_fs_param;
    BOOL falseUsage = FALSE;

    /* register with logging */
    LOG_REGISTER(LOG_CTX_USB_ROLE_SWITCH_SMOKE);

    if (argc > 1){

        /* find the vbus power path */
        vbus = _findVbus();
        if (NULL != vbus){
            if (0 == strncmp(argv[1], "gadget", 6)){
                /* --- switch to host mode --- */

                LOG_U1(LOG_CTX_USB_ROLE_SWITCH_SMOKE, "switch OTG to Host Mode\n");
                status = _switchOTG(USB_OTG_GLOB, STR_GADGET);
                LOG_U1(LOG_CTX_USB_ROLE_SWITCH_SMOKE, "=> %d\n", status);

                if (0 == status){
                    /* turn on VBUS power */
                    status = _switchVbusPower(vbus, "on");
                    LOG_U1(LOG_CTX_USB_ROLE_SWITCH_SMOKE, "_switchVbusPower(on)  = %d \n",status);
                }

                if (0 == status){
                    iap2LoadModuleParameters iap2GadgetParams;

                    IAP2APPSENDSWITCHMSG_STRING_CP (iap2GadgetParams.gadget_fs.vendorId,     ACC_VENDOR_ID );
                    IAP2APPSENDSWITCHMSG_STRING_CP (iap2GadgetParams.gadget_fs.productId,    ACC_PRODUCT_ID );
                    IAP2APPSENDSWITCHMSG_STRING_CP (iap2GadgetParams.gadget_fs.manufacturer, ACC_MANUFACTURER );
                    IAP2APPSENDSWITCHMSG_STRING_CP (iap2GadgetParams.gadget_fs.name,         ACC_NAME );
                    IAP2APPSENDSWITCHMSG_STRING_CP (iap2GadgetParams.gadget_fs.serial,       ACC_SERIAL_NUM );
                    IAP2APPSENDSWITCHMSG_STRING_CP (iap2GadgetParams.gadget_fs.bcdDevice,    ACC_BCD_DEVICE);
                    IAP2APPSENDSWITCHMSG_STRING_CP (iap2GadgetParams.gadget_fs.qmult,        ACC_QMULT);

                }

                if (0 == status){
                    status = iap2LoadModule(NULL, CONFIGFS_MODULE_NAME, strlen(CONFIGFS_MODULE_NAME));
                    LOG_U1(LOG_CTX_USB_ROLE_SWITCH_SMOKE,
                                               "iap2LoadModule %s = %d\n", CONFIGFS_MODULE_NAME, status);
                }
                if (0 == status){
                    mount_fs_param = NULL;
                    status = mount("config", "/sys/kernel/config", "configfs", MS_NOEXEC, mount_fs_param);

                    /* after mount we need to set the mode again */
                    status = chmod("/sys/kernel/config", 0750);
                }
                if (0 == status){
                    status = iap2LoadModule(NULL, LIBCOMPOSITE_MODULE_NAME, strlen(LIBCOMPOSITE_MODULE_NAME));
                    LOG_U1(LOG_CTX_USB_ROLE_SWITCH_SMOKE,
                           "iap2LoadModule %s = %d\n", LIBCOMPOSITE_MODULE_NAME, status);
                }
            } else if (0 == strncmp(argv[1], "host", 4)){
                /* --- switch to device mode --- */

                umount("/sys/kernel/config");
                rmdir("/sys/kernel/config");
                umount(FUNCTION_FS_PATH);
                rmdir(FUNCTION_FS_PATH);
                if (iap2IsKernel314() || iap2IsKernel4xx())
                {
                    iap2UnloadModule(USB_F_FS_MODULE_NAME);
                }
                iap2UnloadModule(LIBCOMPOSITE_MODULE_NAME);

                if (iap2IsKernel314() || iap2IsKernel4xx())
                {
                    iap2UnloadModule(CONFIGFS_MODULE_NAME);
                }

                /* turn on VBUS power */
                status = _switchVbusPower(vbus, "off");
                LOG_U1(LOG_CTX_USB_ROLE_SWITCH_SMOKE, "_switchVbusPower(off)  = %d \n",status);

                /* sleep may not necessary */
//                sleep(2);

                LOG_U1(LOG_CTX_USB_ROLE_SWITCH_SMOKE, "switch OTG to Device Mode\n");
                status = _switchOTG(USB_OTG_GLOB, STR_HOST);
                LOG_U1(LOG_CTX_USB_ROLE_SWITCH_SMOKE, "=> %d\n", status);

                /* In case OTG port is host, but vbus was disabled,
                 * make sure vbus is switched on to rerun the test. */
                status = _switchVbusPower(vbus, "on");
                LOG_U1(LOG_CTX_USB_ROLE_SWITCH_SMOKE, "_switchVbusPower(on)  = %d \n",status);

            } else if (0 == strncmp(argv[1], "show", 4)){
                /* --- show current mode info --- */
                _show();

            } else{
                falseUsage = TRUE;
            }
        }
    } else{
        falseUsage = TRUE;
    }

    if (falseUsage == TRUE){
        LOG_S(LOG_CTX_USB_ROLE_SWITCH_SMOKE, "usage: %s (show | host | gadget)\n" \
                "\tshow   \tshow current OTG role info\n" \
                "\thost   \tswitch OTG role to host\n" \
                "\tgadget \tswitch OTG role to gadget\n" \
                , argv[0]);
    }

    if (NULL != vbus){
        free(vbus);
        vbus = NULL;
    }

    /* unregister from logging */
    LOG_UNREGISTER(LOG_CTX_USB_ROLE_SWITCH_SMOKE);

    return 0;
}

static void _show()
{
#ifdef IPOD_ARCH_ARM
    const char* globPath = USB_OTG_GLOB "/gadget";
    glob_t otgPath;
    glob(globPath, 0, NULL, &otgPath);

    LOG_S(LOG_CTX_USB_ROLE_SWITCH_SMOKE, "OTG glob: %s\n", globPath);
    if (otgPath.gl_pathc == 0){
        LOG_E(LOG_CTX_USB_ROLE_SWITCH_SMOKE, "OTG path not found!\n");
    } else{
        LOG_S(LOG_CTX_USB_ROLE_SWITCH_SMOKE, "OTG path: %s\n",
                otgPath.gl_pathv[0]);
        if (otgPath.gl_pathc > 1){
            LOG_S(LOG_CTX_USB_ROLE_SWITCH_SMOKE,
                    "More than one OTG role path found!\n");
        }

        /* modify path from ../gadget to ../role */
        strncpy(otgPath.gl_pathv[0] + strlen(otgPath.gl_pathv[0]) - 6, "role", 5);

        int file = open(otgPath.gl_pathv[0], O_RDONLY);
        if (file >= 0){
            char buffer[256];
            buffer[0] = 0;
            memset(buffer, 0, sizeof(buffer));

            int ret = read(file, buffer, sizeof(buffer));
            if (ret >= 0){
                LOG_S(LOG_CTX_USB_ROLE_SWITCH_SMOKE, "current OTG role: %s\n",
                        buffer);
            }
            close(file);
        }
    }

    globfree(&otgPath);
#else
    int file = open(USB_OTG_GLOB, O_RDONLY);
    if (file >= 0){
        char buffer[256];
        buffer[0] = 0;
        memset(buffer, 0, sizeof(buffer));

        int ret = read(file, buffer, sizeof(buffer));
        if (ret >= 0){
            LOG_S(LOG_CTX_USB_ROLE_SWITCH_SMOKE, "current OTG role: %s\n",
                    buffer);
        }
        close(file);
    }

#endif /* IPOD_ARCH_ARM */

}

static int _switchOTG(const char* otgGlob, const char* value)
{
#ifdef IPOD_ARCH_ARM
    char otgPath[256];

    if (0 != _findOTGPath(otgPath, 256, otgGlob)){
        LOG_E(LOG_CTX_USB_ROLE_SWITCH_SMOKE,
              "_switchOTG():  find USB OTG %s/role failed \n", otgPath);
        return -1;
    }

    if (TRUE != _commonWrite(otgPath, "role", value, TRUE)){
        LOG_E(LOG_CTX_USB_ROLE_SWITCH_SMOKE,
              "_switchOTG():  set USB OTG %s/role to %s failed \n", otgPath, value);
        return -1;
    }
#else
    char host[] = "host";
    char gadget[] = "peripheral";
    char *modeValue = NULL;
    
    if (strcmp(value, STR_HOST) == 0) {
        modeValue = host;   /* host mode */
    } else {
        modeValue = gadget; /* gadget mode */
    }

    if (TRUE != _accessFs(otgGlob, modeValue, strlen(modeValue), TRUE)){
        LOG_E(LOG_CTX_USB_ROLE_SWITCH_SMOKE,
              "_switchOTG():  set USB OTG %s/role to %s failed \n", otgGlob, modeValue);
        return -1;
    }
#endif

    return 0;
}

static int _switchVbusPower(const char* power, const char* value)
{
#ifndef IPOD_ARCH_ARM
    char offData[] = {0x0, 0x1, 0x0, 0x0};
    char onData[]  = {0x0, 0x1, 0x0, 0x1};
    char *modeValue = NULL;
    size_t len = 0;
#endif /* IPOD_ARCH_ARM */

    if(NULL == power)
    {
        return -1;
    }
    
#ifdef IPOD_ARCH_ARM
    if (TRUE != _commonWriteValue(power, value)){
        LOG_E(LOG_CTX_USB_ROLE_SWITCH_SMOKE,
              "_switchVbusPower():  set USB power failed \n");
        return -1;
    }

#else
    if (strcmp(value, "on") == 0){
        modeValue = onData;
        len = sizeof(onData);
    } else {
        modeValue = offData;
        len = sizeof(offData);
    }
    
    if (TRUE != _accessFs(power, modeValue, len, TRUE)){
        LOG_E(LOG_CTX_USB_ROLE_SWITCH_SMOKE,
              "_switchVbusPower():  set USB power failed \n");
        return -1;
    }

#endif /* IPOD_ARCH_ARM */

    return 0;
}

static char* _findVbus(void)
{
    int ret = 0;
    char *vbus = NULL;
    glob_t found;

    /* find VBUS path */
    if (0 == (ret = glob(VBUS_POWER, 0, NULL, &found)) && found.gl_pathc > 0){
        if (found.gl_pathc > 1){
            LOG_U1(LOG_CTX_USB_ROLE_SWITCH_SMOKE, "more than one VBUS found; \n\t\t use: %s\n",
                    found.gl_pathv[0]);
        }

        vbus = malloc(strlen(found.gl_pathv[0]) +1);
        if (NULL != vbus)
            strncpy(vbus, found.gl_pathv[0], strlen(found.gl_pathv[0]) +1);
        else
            LOG_E(LOG_CTX_USB_ROLE_SWITCH_SMOKE, "Allocate memory failed!\n");

    } else if (ret == GLOB_NOMATCH){
        LOG_E(LOG_CTX_USB_ROLE_SWITCH_SMOKE, "glob does not found %s  ret = %d\n",
                VBUS_POWER, ret);
    } else{
        LOG_E(LOG_CTX_USB_ROLE_SWITCH_SMOKE, "glob failed with ret = %d\n", ret);
    }

    globfree(&found);
    return vbus;
}

#ifdef IPOD_ARCH_ARM
static int _findOTGPath(char* otgPath, unsigned int len, const char* otgGlob)
{
    glob_t found;
    int ret;

    strncpy(otgPath, otgGlob, len);
    strncat(otgPath, "/" STR_GADGET, len - strlen(otgPath));

    if (0 == (ret = glob(otgPath, 0, NULL, &found)) && found.gl_pathc > 0){
        strncpy(otgPath, found.gl_pathv[0], len);
        otgPath[strlen(otgPath) - 6] = 0;

        if (found.gl_pathc > 1){
            LOG_U1(LOG_CTX_USB_ROLE_SWITCH_SMOKE,
                   "_findOTGPath():  more than one OTG device found; use: %s \n", otgPath);
        }
        globfree(&found);

    } else if (ret == GLOB_NOMATCH){
        LOG_E(LOG_CTX_USB_ROLE_SWITCH_SMOKE,
              "_findOTGPath():  could not find USB OTG gadget at %s \n", otgPath);
        ret = -1;

    } else{
        LOG_E(LOG_CTX_USB_ROLE_SWITCH_SMOKE,
              "_findOTGPath():  USB OTG switch internal error: glob = %d \n", ret);
        ret = -1;
    }

    return ret;
}

static int _commonWrite(const char* path, const char* subPath, const char* value, BOOL checkBeforeWrite)
{
    BOOL status = 0;
    char valuePath[256];
    int file = 0;

    int ret = snprintf(valuePath, 256, "%s/%s", path, subPath);
    if (ret >= 0 && ret < 256){
        file = open(valuePath, checkBeforeWrite ? O_RDWR : O_WRONLY);
        if (file >= 0){
            const size_t len = strlen(value) + 1;

            /* check before writing the same value */
            if (checkBeforeWrite == TRUE){
                /* to capture longer entries */
                char buffer[len + 1];
                memset(&buffer[0], 0, len + 1);
                ret = read(file, buffer, sizeof(buffer));
                if (ret == (int)len){
                    /* without trailing \0 */
                    if (0 == strncmp(buffer, value, len - 1)){
                        /* no need to write */
                        status = TRUE;
                    }
                    LOG_U1(LOG_CTX_USB_ROLE_SWITCH_SMOKE,
                           "_commonWrite():  read: %s \n", buffer);

                } else if (ret < 0){
                    LOG_E(LOG_CTX_USB_ROLE_SWITCH_SMOKE,
                          "_commonWrite():  read: %d %s \n", errno, strerror(errno));
                }
            }

            /* write or skip if already the same value */
            if (status == FALSE){
                ret = write(file, value, len);
                if (ret == (int)len){
                    /* successful write */
                    status = TRUE;
                } else if (ret < 0){
                    LOG_E(LOG_CTX_USB_ROLE_SWITCH_SMOKE,
                          "_commonWrite():  write: %d %s \n", errno, strerror(errno));
                }
            }
            close(file);

        } else{
            LOG_E(LOG_CTX_USB_ROLE_SWITCH_SMOKE,
                  "_commonWrite():  open: %d %s \n", errno, strerror(errno));
        }
    }

    return status;
}

static int _commonWriteValue(const char* path, const char* value)
{
    BOOL status = FALSE;
    int file = 0;
    int ret = 0;

    file = open(path, O_WRONLY);
    if (file >= 0){
        const size_t len = strlen(value) + 1;

        ret = write(file, value, len);
        if (ret == (int)len){
            /* successful write */
            status = TRUE;
        } else if (ret < 0){
            LOG_E(LOG_CTX_USB_ROLE_SWITCH_SMOKE,
                  "_commonWriteValue():  write: %d %s", errno, strerror(errno));
        }
        close(file);

    } else{
        LOG_E(LOG_CTX_USB_ROLE_SWITCH_SMOKE,
              "_commonWriteValue():  open: %d %s", errno, strerror(errno));
    }

    return status;
}

#else
static BOOL _accessFs(const char* path, void *value, ssize_t len, BOOL readWrite)
{
    BOOL status = FALSE;
    int file = 0;
    ssize_t ret = 0;

    file = open(path, readWrite ? O_RDWR : O_RDONLY);
    if (file >= 0){
        if(readWrite)
        {
            ret = write(file, value, len);
            if (ret == len){
                /* successful write */
                status = TRUE;
            } else if (ret < 0){
                LOG_E(LOG_CTX_USB_ROLE_SWITCH_SMOKE,
                      "_accessFs():  write: %d %s \n", errno, strerror(errno));
            }
        }
        else
        {
            ret = read(file, value, len);
            if (ret == len){
                /* successful write */
                status = TRUE;
            } else if (ret < 0){
                LOG_E(LOG_CTX_USB_ROLE_SWITCH_SMOKE,
                      "_accessFs():  read: %d %s \n", errno, strerror(errno));
            }
        }
        
        close(file);

    } else{
        LOG_E(LOG_CTX_USB_ROLE_SWITCH_SMOKE,
              "_accessFs():  open: %d %s \n", errno, strerror(errno));
    }

    return status;
}
#endif /* IPOD_ARCH_ARM */
