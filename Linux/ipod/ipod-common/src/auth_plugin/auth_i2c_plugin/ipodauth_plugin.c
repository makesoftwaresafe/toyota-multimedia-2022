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
 * \author Konrad Gerhards
 *
 * \copyright (c) 2003 - 2011 ADIT Corporation
 *
 ***********************************************************************/



#include <fcntl.h>
#include <stdio.h>
#include "ipodauth.h"
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <errno.h>
#include <linux/i2c-dev.h>

#include <unistd.h>
#include <sys/ioctl.h>

#include "iap2_dlt_log.h"

#define GPIO_BASE         "/sys/class/gpio"
#define GPIO_EXPORT       "export"
#define GPIO_UNEXPORT     "unexport"

#define GPIO_DIR_OUT      "out"

#define GPIO_ATTR_VALUE   "value"
#define GPIO_ATTR_DIR     "direction"


AUT_DEV_INFO gAutInfo;

int iPodAuthComInit (void)
{
    S32 rc = IPOD_AUTHCOM_SUCCESS;

    memset(&gAutInfo, 0, sizeof(gAutInfo));
    /* 0 is a valid value for file descriptor */
    gAutInfo.gpioInfo1.id = IPOD_AUTHCOM_ERROR;
    gAutInfo.gpioInfo2.id = IPOD_AUTHCOM_ERROR;

    return (int)rc;
    
}

int iPodAuthComDeinit (void)
{
    S32 rc = IPOD_AUTHCOM_SUCCESS;

    memset(&gAutInfo, 0, sizeof(gAutInfo));
    /* 0 is a valid value for file descriptor */
    gAutInfo.gpioInfo1.id = IPOD_AUTHCOM_ERROR;
    gAutInfo.gpioInfo2.id = IPOD_AUTHCOM_ERROR;

    return (int)rc;
}

int aut_open_gpio(AUT_GPIO_INFO *gpioInfo, const unsigned char *num, int flags)
{
    int rc = IPOD_AUTHCOM_SUCCESS;
    struct stat fileInfo;
    int fd = -1;

    if(num != NULL)
    {
        /* Indicated ports is not created. */
        if(gpioInfo->flg == 0)
        {
            /* check if gpioXXX exists */

            sprintf(gpioInfo->name, "%s/gpio%s", GPIO_BASE, num);
            if(IPOD_AUTHCOM_SUCCESS != stat(gpioInfo->name, &fileInfo))
            {
                /* Open the /sys/class/gpio/export */
                sprintf(gpioInfo->name, "%s/%s", GPIO_BASE, GPIO_EXPORT);
                fd = open(gpioInfo->name, O_WRONLY);
                if(fd == IPOD_AUTHCOM_ERROR)
                {
                    rc = IPOD_AUTHCOM_ERROR;
                    IAP2AUTHDLTLOG(DLT_LOG_ERROR,"GPIO export - open failed with error %d %s",errno,strerror(errno));
                }

                if(IPOD_AUTHCOM_SUCCESS == rc)
                {
                    /* Write the GPIO port number to export */
                    sprintf(gpioInfo->num, "%s", num);
                    rc = write(fd, gpioInfo->num, strlen(gpioInfo->num) + 1);
                    if(strlen(gpioInfo->num) + 1 == (unsigned int)rc)
                    {
                        rc = IPOD_AUTHCOM_SUCCESS;
                        /* /sys/gpio/class/gpio/gpioXXX is created */
                        gpioInfo->flg = 1;
                        gpioInfo->export_done = 1;
                    }
                    else
                    {
                        rc = IPOD_AUTHCOM_ERROR;
                        IAP2AUTHDLTLOG(DLT_LOG_ERROR,"GPIO export - write failed with error %d %s",errno,strerror(errno));
                    }

                    close(fd);
                }
            }
            if(IPOD_AUTHCOM_SUCCESS != rc)
            {
                IAP2AUTHDLTLOG(DLT_LOG_ERROR,"GPIO export failed");
            }
        }

        if(IPOD_AUTHCOM_SUCCESS == rc)
        {
            /* Open the /sys/class/gpio/gpioXXX/direction */
            sprintf(gpioInfo->name, "%s/gpio%s/%s", GPIO_BASE, num, GPIO_ATTR_DIR);
            fd = open(gpioInfo->name, O_RDONLY);
            if(fd != IPOD_AUTHCOM_ERROR)
            {
                char readBuf[4] = {0};
                /* check if the direction is set to "out" */
                rc = read(fd, &readBuf[0], strlen(GPIO_DIR_OUT));
                close(fd);

                if (0 != strncmp(&readBuf[0], GPIO_DIR_OUT, strlen(GPIO_DIR_OUT +1)))
                {
                    /* direction is not "out", try to set to "out" */
                    fd = open(gpioInfo->name, O_RDWR);
                    if(fd != IPOD_AUTHCOM_ERROR)
                    {
                        /* Write the output setting to opening direction */
                        rc = write(fd, GPIO_DIR_OUT, strlen(GPIO_DIR_OUT) + 1);
                        if(rc == strlen(GPIO_DIR_OUT) + 1)
                        {
                            rc = IPOD_AUTHCOM_SUCCESS;
                        }
                        else
                        {
                            rc = IPOD_AUTHCOM_ERROR;
                            IAP2AUTHDLTLOG(DLT_LOG_ERROR,"GPIO direction set - write failed with error %d %s",errno,strerror(errno));
                        }
                        close(fd);
                    }
                    else
                    {
                        rc = IPOD_AUTHCOM_ERROR;
                        IAP2AUTHDLTLOG(DLT_LOG_ERROR,"GPIO direction set - open failed with error %d %s",errno,strerror(errno));
                    }
                } 
                else
                {
                    /* direction already set to "out" */
                    rc = IPOD_AUTHCOM_SUCCESS;
                }
            }
            else
            {
                rc = IPOD_AUTHCOM_ERROR;
                IAP2AUTHDLTLOG(DLT_LOG_ERROR,"GPIO - open failed with error %d %s",errno,strerror(errno));
            }
            if(IPOD_AUTHCOM_SUCCESS != rc)
            {
                IAP2AUTHDLTLOG(DLT_LOG_ERROR,"GPIO direction set failed");
            }
        }
        /* finally open the gpio port */
        if(IPOD_AUTHCOM_SUCCESS == rc)
        {
            /* Open the /sys/class/gpio/gpioXXX/value */
            sprintf(gpioInfo->name, "%s/gpio%s/%s", GPIO_BASE, num, GPIO_ATTR_VALUE);
            fd = open(gpioInfo->name, flags);
            if(fd == IPOD_AUTHCOM_ERROR)
            {
                rc = IPOD_AUTHCOM_ERROR;
                IAP2AUTHDLTLOG(DLT_LOG_ERROR,"GPIO open failed with error %d %s",errno,strerror(errno));
            }
            else
            {
                rc = flock(fd, (LOCK_EX | LOCK_NB));
                if(rc != 0)
                {
                    /* device already locked */
                    rc = close(fd);
                    rc = IPOD_AUTHCOM_ERROR;
                }
                else
                {
                    gpioInfo->id = fd;
                }
            }
            if( rc != IPOD_AUTHCOM_SUCCESS)
            {
                IAP2AUTHDLTLOG(DLT_LOG_ERROR,"GPIO open failed");
            }
        }
    }
    else
    {
        IAP2AUTHDLTLOG(DLT_LOG_FATAL,"Invalid Parameter");
        rc = IPOD_AUTHCOM_ERROR;
    }

    return rc;
}

int aut_close_gpio(AUT_GPIO_INFO *gpioInfo)
{
    int rc = IPOD_AUTHCOM_SUCCESS;
    int id = IPOD_AUTHCOM_ERROR;

    if(gpioInfo == NULL)
    {
        IAP2AUTHDLTLOG(DLT_LOG_FATAL,"Invalid Parameter");
        return IPOD_AUTHCOM_ERROR;
    }

    /* close /sys/class/gpio/gpioXXX/value */
    if(gpioInfo->id != IPOD_AUTHCOM_ERROR)
    {
        flock(gpioInfo->id, LOCK_UN);
        close(gpioInfo->id);
        gpioInfo->flg = 0;
        gpioInfo->id = -1;
    }

    /* unexport gpioXXX */
    if(gpioInfo->export_done == 1)
    {
        sprintf(gpioInfo->name, "%s/%s", GPIO_BASE, GPIO_UNEXPORT);
        id = open(gpioInfo->name, O_WRONLY);
        if(id != IPOD_AUTHCOM_ERROR)
        {
            /* Write the GPIO port number to unexport */
            rc = write(id, gpioInfo->num, strlen(gpioInfo->num) + 1);
            if(strlen(gpioInfo->num) + 1 == (unsigned int)rc)
            {
                /* /sys/class/gpio/gpioXXX is deleted */
                rc = IPOD_AUTHCOM_SUCCESS;
            }
            else
            {
                rc = IPOD_AUTHCOM_ERROR;
            }
            close(id);
        }
        else
        {
            IAP2AUTHDLTLOG(DLT_LOG_ERROR,"GPIO open failed with error %d %s",errno,strerror(errno));
            rc = IPOD_AUTHCOM_ERROR;
        }
    }

    if( rc != IPOD_AUTHCOM_SUCCESS)
    {
        IAP2AUTHDLTLOG(DLT_LOG_ERROR,"GPIO unexport failed");
    }

    return rc;
}

/** S32 iPodAuthComOpen
 * Open Authentication driver
 */
int iPodAuthComOpen (const char *device_name, int flags, int mode)
{
    S32 rc = IPOD_AUTHCOM_SUCCESS;
    int fd = -1;
    
    if(device_name == NULL)
    {
        IAP2AUTHDLTLOG(DLT_LOG_FATAL,"Invalid Parameter");
        return IPOD_AUTHCOM_BAD_PARAMETER;
    }

    IAP2AUTHDLTLOG(DLT_LOG_INFO,"open : %s, mode: %d", device_name,mode);

    switch(mode)
    {
        case IPOD_AUTHCOM_DATA_OPEN:
            fd = open((const char*)device_name, flags);
            if(IPOD_AUTHCOM_ERROR == fd)
            {
                rc = IPOD_AUTHCOM_ERROR;
                IAP2AUTHDLTLOG(DLT_LOG_ERROR,"failed with rc : %d error %d %s ",rc,errno,strerror(errno));
            }
            else
            {
                rc = flock(fd, (LOCK_EX | LOCK_NB));
                if(rc == 0)
                {
                    gAutInfo.autDevID = fd;
                    rc = gAutInfo.autDevID;
                }
                else
                {
                    /* device already in use and locked */
                    rc = close(fd);
                    rc = IPOD_AUTHCOM_ERROR;
                    IAP2AUTHDLTLOG(DLT_LOG_ERROR,"failed with rc : %d",rc);
                }

            }
            break;
        case IPOD_AUTHCOM_GPIO_OPEN:
            if(gAutInfo.gpioInfo1.id < 0)
            {
                rc = aut_open_gpio(&gAutInfo.gpioInfo1, (unsigned char*)device_name, flags);
                if(rc == IPOD_AUTHCOM_SUCCESS)
                {
                    rc = gAutInfo.gpioInfo1.id;
                }
                else
				{
					IAP2AUTHDLTLOG(DLT_LOG_ERROR,"GPIO configuration failed with rc : %d",rc);
				}
            }
            else if(gAutInfo.gpioInfo2.id < 0)
            {
                rc = aut_open_gpio(&gAutInfo.gpioInfo2, (unsigned char*)device_name, flags);
                if(rc == IPOD_AUTHCOM_SUCCESS)
                {
                    rc = gAutInfo.gpioInfo2.id;
                }
                else
				{
					IAP2AUTHDLTLOG(DLT_LOG_ERROR,"GPIO configuration failed with rc : %d",rc);
				}
            }
            else
            {
                rc = IPOD_AUTHCOM_ERROR;
                IAP2AUTHDLTLOG(DLT_LOG_ERROR,"failed with rc : %d",rc);
            }
            break;

        default:
            rc = IPOD_AUTHCOM_ERROR;
            IAP2AUTHDLTLOG(DLT_LOG_ERROR,"failed with rc : %d",rc);
            break;
    }

    return (int)rc;
}

int iPodAuthComClose(int fd)
{
    S32 rc = IPOD_AUTHCOM_SUCCESS;

    if(fd == gAutInfo.autDevID)
    {
        flock(gAutInfo.autDevID, LOCK_UN);
        close(gAutInfo.autDevID);
        gAutInfo.autDevID = -1;
        rc = IPOD_AUTHCOM_SUCCESS;
    }
    else if(fd == gAutInfo.gpioInfo1.id)
    {
        rc = aut_close_gpio(&gAutInfo.gpioInfo1);
    }
    else if(fd == gAutInfo.gpioInfo2.id)
    {
        rc = aut_close_gpio(&gAutInfo.gpioInfo2);
    }
    else
    {
        rc = IPOD_AUTHCOM_ERROR;
        IAP2AUTHDLTLOG(DLT_LOG_ERROR,"failed");
    }
    
    return (int)rc;
}

int AUTReadDev(int fd, unsigned char *buf, unsigned int length)
{
    int rc = IPOD_AUTHCOM_SUCCESS;

    if(buf == NULL)
    {
        IAP2AUTHDLTLOG(DLT_LOG_FATAL,"Invalid Parameter");
        return IPOD_AUTHCOM_ERROR;
    }
    rc = read(fd, buf, length);
    if(length != (unsigned int)rc)
    {
        rc = IPOD_AUTHCOM_ERROR;
        IAP2AUTHDLTLOG(DLT_LOG_INFO,"read failed %d %s ",errno,strerror(errno));
    }

    return rc;
}

int AUTReadGPIO(int fd, unsigned char *buf, unsigned int length)
{
    int rc = IPOD_AUTHCOM_SUCCESS;

    if(buf == NULL)
    {
        IAP2AUTHDLTLOG(DLT_LOG_FATAL,"Invalid Parameter");
        return IPOD_AUTHCOM_ERROR;
    }

    rc = read(fd, buf, length);

    return rc;
}

int iPodAuthComRead(int fd, unsigned int len, void *buf, int flags)
{
    S32 rc = IPOD_AUTHCOM_ERROR;
    /* LINT */
    flags = flags;

    if(fd < 0)
    {
        IAP2AUTHDLTLOG(DLT_LOG_FATAL,"Invalid Parameter");
        rc = IPOD_AUTHCOM_ERROR;
    }
    else if(gAutInfo.autDevID == fd)
    {
        rc = AUTReadDev(fd, buf, len);

    }
    else
    {
        rc = AUTReadGPIO(fd, buf, len);
    }

    if(IPOD_AUTHCOM_ERROR == rc)
    {
        IAP2AUTHDLTLOG(DLT_LOG_INFO,"failed with rc : %d",rc);
    }

    return (int)rc;
}


int AUTWriteDev(int fd, const unsigned char *buf, unsigned short length)
{
    int rc = IPOD_AUTHCOM_SUCCESS;

    if(buf == NULL)
    {
        IAP2AUTHDLTLOG(DLT_LOG_FATAL,"Invalid Parameter");
        return IPOD_AUTHCOM_ERROR;
    }

    rc = write(fd, buf, length);
    if(length != (unsigned int)rc)
    {
        rc = IPOD_AUTHCOM_ERROR;
        IAP2AUTHDLTLOG(DLT_LOG_INFO,"write failed %d %s",errno,strerror(errno));
    }

    return rc;
}

int AUTWriteGPIO(int fd, const unsigned char *buf, unsigned short length)
{
    int rc = IPOD_AUTHCOM_SUCCESS;
    char num[8] = {0};

    if(buf == NULL)
    {
        IAP2AUTHDLTLOG(DLT_LOG_FATAL,"Invalid Parameter");
        return IPOD_AUTHCOM_ERROR;
    }

    snprintf(num, sizeof(num), "%d", buf[0]);
    rc = write(fd,  num, strnlen(num, sizeof(num)) + 1);
    if((strnlen(num, sizeof(num)) + 1) == (unsigned int)rc)
    {
        rc = length;
    }
    else
    {
        IAP2AUTHDLTLOG(DLT_LOG_ERROR,"write failed");
        rc = IPOD_AUTHCOM_ERROR;
    }

    return rc;
}

int iPodAuthComWrite(int fd, unsigned int size, const void *buf, int flags)
{
    S32 rc = IPOD_AUTHCOM_ERROR;
    /* LINT */
    flags = flags;

    if(fd < 0)
    {
        IAP2AUTHDLTLOG(DLT_LOG_FATAL,"Invalid Parameter");
        rc = IPOD_AUTHCOM_ERROR;
    }
    else if(gAutInfo.autDevID == fd)
    {
        rc = AUTWriteDev(fd, buf, size);
    }
    else
    {
        rc = AUTWriteGPIO(fd, buf, size);
    }

    if(IPOD_AUTHCOM_ERROR == rc)
    {
        IAP2AUTHDLTLOG(DLT_LOG_INFO,"failed with rc : %d",rc);
    }

    return (int)rc;
}

int iPodAuthComIoctl(int fd, int request, char *argp)
{
    S32 rc = IPOD_AUTHCOM_ERROR;
    int regAddr = 0;

    if(fd >= 0)
    {
        if(request == IPOD_AUTHCOM_IOCTL_I2C_SLAVE_ADDR)
        {
            regAddr = atoi((const char*)argp);
            IAP2AUTHDLTLOG(DLT_LOG_INFO,"I2C_SLAVE_ADDR : %d", regAddr);
            rc = ioctl(fd, I2C_SLAVE_FORCE, regAddr);
            if(rc != IPOD_AUTHCOM_SUCCESS)
            {
                rc = IPOD_AUTHCOM_ERROR;
            }
        }
	}
    else
    {
        IAP2AUTHDLTLOG(DLT_LOG_FATAL,"Invalid Parameter");
        rc = IPOD_AUTHCOM_ERROR;
    }

    if(IPOD_AUTHCOM_SUCCESS != rc)
    {
        IAP2AUTHDLTLOG(DLT_LOG_ERROR,"failed with rc : %d",rc);
    }

    return (int)rc;
}

