/**
* \file: iap2_usb_role_switch_common.c
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

#include "iap2_usb_role_switch_common.h"
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/file.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

BOOL iAP2UsbRoleSwitchCommon_Exists(const char* name)
{
    struct stat fileInfo;
    return (0 <= stat(name, &fileInfo)) ? TRUE : FALSE;
}

BOOL iAP2UsbRoleSwitchCommon_Write(const char* path, const char* subPath, const char* value,
        BOOL checkBeforeWrite)
{
    BOOL status = FALSE;
    char valuePath[IAP2_SYS_MAX_PATH];
    int file = 0;

    int ret = snprintf(valuePath, IAP2_SYS_MAX_PATH, "%s/%s", path, subPath);
    if (ret >= 0 && ret < IAP2_SYS_MAX_PATH)
    {
        file = open(valuePath, checkBeforeWrite ? O_RDWR : O_WRONLY);
        if (file >= 0)
        {
            const size_t len = strlen(value) + 1;

            /* check before writing the same value */
            if (checkBeforeWrite == TRUE)
            {
                char buffer[len + 1]; /* to capture longer entries */
                ret = read(file, buffer, sizeof(buffer));

                if (ret == (int)len)
                {
                    if (0 == strncmp(buffer, value, len - 1)) /* without trailing \0 */
                    {
                        /* no need to write */
                        status = TRUE;
                    }
                }
                else if (ret < 0)
                {
                    IAP2USBROLESWITCHDLTLOG(DLT_LOG_WARN, "read: %d %s", errno, strerror(errno));
                }
            }

            /* write or skip if already the same value */
            if (status == FALSE)
            {
                ret = write(file, value, len);
                if (ret == (int)len)
                {
                    /* successful write */
                    status = TRUE;
                }
                else if (ret < 0)
                {
                    IAP2USBROLESWITCHDLTLOG(DLT_LOG_WARN, "write: %d %s", errno, strerror(errno));
                }
            }

            close(file);
        }
        else
        {
            IAP2USBROLESWITCHDLTLOG(DLT_LOG_WARN, "open: %d %s", errno, strerror(errno));
        }
    }

    return status;
}

IAP2_USB_HIDDEN_SYMBOL BOOL iAP2UsbRoleSwitchCommon_WriteValue(const char* path,
        const char* value, size_t len, BOOL checkBeforeWrite)
{
    BOOL status = FALSE;
    int file = 0;
    int ret = 0;

    file = open(path, checkBeforeWrite ? O_RDWR : O_WRONLY);
    if (file >= 0)
    {
        if(len == 0)
        {
            len = strlen(value) + 1;
        }

        /* check before writing the same value */
        if (checkBeforeWrite == TRUE)
        {
            char buffer[len + 1]; /* to capture longer entries */
            ret = read(file, buffer, sizeof(buffer));

            if (ret == (int)len)
            {
                if (0 == strncmp(buffer, value, len - 1)) /* without trailing \0 */
                {
                    /* no need to write */
                    status = TRUE;
                }
            }
            else if (ret < 0)
            {
                IAP2USBROLESWITCHDLTLOG(DLT_LOG_WARN, "read: %d %s", errno, strerror(errno));
            }
        }

        /* write or skip if already the same value */
        if (status == FALSE)
        {
            ret = write(file, value, len);
            if (ret == (int)len)
            {
                /* successful write */
                status = TRUE;
            }
            else if (ret < 0)
            {
                IAP2USBROLESWITCHDLTLOG(DLT_LOG_WARN, "write: %d %s", errno, strerror(errno));
            }
        }

        close(file);
    }
    else
    {
        IAP2USBROLESWITCHDLTLOG(DLT_LOG_WARN, "open: %d %s", errno, strerror(errno));
    }

    return status;
}
