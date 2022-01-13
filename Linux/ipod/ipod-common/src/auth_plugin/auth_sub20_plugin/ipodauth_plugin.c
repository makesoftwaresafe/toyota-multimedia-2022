/************************************************************************
 * \file ipodspi.c
 *
 * \version $Id: ipodspi_lx.c,v 1.1 2011/07/26 08:01:13 serhard Exp $
 *
 * \release $Name:  $
 *
 * \brief This is the implementation of the ipodspi component for Linux
 *
 * \component ipod control
 *
 * \author Norbert Fleischmann
 *
 * \copyright (c) 2003 - 2011 ADIT Corporation
 *
 ***********************************************************************/



#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include "ipodauth.h"
#include <stdlib.h>
#include <string.h>
#include <sys/un.h>
#include "iap2_dlt_log.h"

#include <libsub.h>

sub_handle g_sub_handle = NULL;
int g_slave_address = 0x11;
int g_gpio_mask = 0x1000;

int iPodAuthComInit (void)
{
    S32 rc = IPOD_AUTHCOM_SUCCESS;

    g_sub_handle = sub_open( 0 );

    return (int)rc;

}

int iPodAuthComDeinit (void)
{
    if(g_sub_handle != NULL)
    {
        sub_close(g_sub_handle);
        g_sub_handle = NULL;
    }

    return IPOD_AUTHCOM_SUCCESS;
}

/** S32 iPodAuthComOpen
 * Open Authentication driver
 */

int iPodAuthComOpen (const char *device_name, int flags, int mode)
{
    S32 rc = IPOD_AUTHCOM_SUCCESS;
    int config = 0;
    int freq = 50000;

    device_name = device_name;
    flags = flags;

    switch(mode)
    {
    case IPOD_AUTHCOM_DATA_OPEN:
        if( !g_sub_handle )
        {
            rc = IPOD_AUTHCOM_ERROR;
        }
        if(rc == IPOD_AUTHCOM_SUCCESS)
        {
            rc = sub_gpio_config(g_sub_handle, g_gpio_mask, &config, g_gpio_mask);
        }
        if(rc == IPOD_AUTHCOM_SUCCESS)
        {
            rc = sub_gpio_write(g_sub_handle, g_gpio_mask, &config, g_gpio_mask);
        }
        if(rc == IPOD_AUTHCOM_SUCCESS)
        {
            rc = sub_i2c_freq(g_sub_handle, &freq);
        }
        if(rc == IPOD_AUTHCOM_SUCCESS)
        {
            rc = 1;
        }

        break;

    case IPOD_AUTHCOM_GPIO_OPEN:
        if(flags == IPOD_AUTHCOM_FLAGS_WRONLY)
        {
            rc = 2;
        }
        else
        {
            rc = 3;
        }

        break;

    default:
        rc = IPOD_AUTHCOM_ERROR;
        break;
    }

    if(rc < IPOD_AUTHCOM_SUCCESS)
    {
        IAP2AUTHDLTLOG(DLT_LOG_WARN,"failed with rc : %d",rc);
    }

    return (int)rc;
}

int iPodAuthComClose(int fd)
{
    S32 rc = IPOD_AUTHCOM_SUCCESS;

    fd = fd;

    return (int)rc;
}

int iPodAuthComRead(int fd, unsigned int size, void *buf, int flags)
{
    S32 rc = IPOD_AUTHCOM_ERROR;
    flags = flags;

    if (fd == 1)
    {
        rc = sub_i2c_read(g_sub_handle, g_slave_address, 0, 0, (char*) buf, (int) size );
        if (rc == 0)
        {
            rc = size;
        }
        else
        {
            rc = -1;
        }
    }

    if(IPOD_AUTHCOM_SUCCESS != rc)
    {
        IAP2AUTHDLTLOG(DLT_LOG_WARN,"failed with rc : %d",rc);
    }

    return (int)rc;
}

int iPodAuthComWrite(int fd, unsigned int size, const void *buf, int flags)
{
    S32 rc = IPOD_AUTHCOM_ERROR;
    flags = flags;

    if (fd == 1)
    {
        rc = sub_i2c_write(g_sub_handle, g_slave_address, 0, 0, (char*) buf, (int) size );
    }
    else if (fd == 2)
    {
        char gpio_value = ((char*)buf)[0];
        int config = 0;

        if (gpio_value == 0)
        {
            rc = sub_gpio_write(g_sub_handle, 0, &config, g_gpio_mask);
        }
        else
        {
            rc = sub_gpio_write(g_sub_handle, g_gpio_mask, &config, g_gpio_mask);
        }
    }
    if (rc == 0)
    {
        rc = size;
    }
    else
    {
        rc = -1;
    }

    if(IPOD_AUTHCOM_SUCCESS != rc)
    {
        IAP2AUTHDLTLOG(DLT_LOG_WARN,"failed with rc : %d",rc);
    }

    return (int)rc;
}

int iPodAuthComIoctl(int fd, int request, char *argp)
{
    S32 rc = IPOD_AUTHCOM_SUCCESS;

    fd = fd;
    request = request;
    argp = argp;

    return (int)rc;
}

