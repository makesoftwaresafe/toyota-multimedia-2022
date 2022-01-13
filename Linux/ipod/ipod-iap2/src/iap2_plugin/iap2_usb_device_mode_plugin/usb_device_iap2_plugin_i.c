
#include <errno.h>
#include <stdint.h>

#include "iap2_datacom.h"
#include "iap2_datacom_i.h"
#include <time.h>
#include <stdarg.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <asm/types.h>
#include <fcntl.h>
#include <semaphore.h>
#include <sys/socket.h>
#include <pthread_adit.h>
#include <libusb-1.0/libusb.h>

#include "iap2_dlt_log.h"
#include "iap2_hidapi.h"
#include "iap2_hid_desc.h"


#define APPLE_VID           0x05ac
#define APPLE_PID_MSK       0x1200

/* #define DEBUG_PRINTF */


#ifdef DEBUG_PRINTF
#define DUMP(c, a, l) mem_dsp(c ,a ,l)
#else
#define DUMP(c, a, l)
#endif /* DEBUG_PRINTF */

S32 iPodiAP2HIDReadReportID(VP devInfo, IPOD_IAP2_REPORT_DATA **report_data, U32 *data_counter);
S32 iPodiAP2USBGetReportId(IPOD_IAP2_HID_DEV_INFO* devinfo, U32 msgLen, U32 *reportId, U32 *reportLen);

iap2_hid_device *iAP2_Hid_open(IPOD_IAP2_HID_DEV_INFO* devInfo, wchar_t *serial, S16 *vid, S16 *pid);

#ifdef DEBUG_PRINTF
void mem_dsp(char *entity ,uint8_t *ptr ,size_t len)
{
    static const char   xd[]    = "0123456789ABCDEF";
    static char         buf[]   = " XX";
    size_t              pos = 0;
    uint8_t             b;

    IAP2USBPLUGINDLTLOG(DLT_LOG_DEBUG, " %s ", entity);
    IAP2USBPLUGINDLTLOG(DLT_LOG_DEBUG, " addr: +0 +1 +2 +3 +4 +5 +6 +7 +8 +9 +a +b +c +d +e +f");
    IAP2USBPLUGINDLTLOG(DLT_LOG_DEBUG, " %04x:", pos);
    for (pos = 1; len > 0; len--, ptr++, pos++)
    {
        b = *ptr;
        buf[1] = xd[b >> 4];
        buf[2] = xd[b & 0xF];

        if(pos % 16 == 0)
        {
            IAP2USBPLUGINDLTLOG(DLT_LOG_DEBUG, "%s", buf);
            if(len != 1) IAP2USBPLUGINDLTLOG(DLT_LOG_DEBUG, " %04x:", pos);
        }
        else
        {
            IAP2USBPLUGINDLTLOG(DLT_LOG_DEBUG, "%s", buf);
        }
    }
    if(pos % 16 != 1) IAP2USBPLUGINDLTLOG(DLT_LOG_DEBUG, "");
}
#endif /* DEBUG_PRINTF */

/*!
 * \func iPodiAP2USBGetReportId
 * \par INPUT PARAMETERS
 * \li \c \b U32  msgLen
 * \par OUTPUT PARAMETERS
 * \li \c \b U32 *reportId
 * \li \c \b U32 *reportLen
 * \par REPLY PARAMETERS
 *  none
 * \par DESCRIPTION
 * This function gets the report id.
 * This function isn't visible outside
 */
S32 iPodiAP2USBGetReportId(IPOD_IAP2_HID_DEV_INFO* devinfo, U32 msgLen, U32 *reportId, U32 *reportLen)
{
    U32  zaehler             = 0;
    U32 maxLen              = 0;
    U32 maxRepId            = 0;
    S32 rc                  = IPOD_DATACOM_SUCCESS;
    IPOD_IAP2_REPORT_DATA* report_data = devinfo->reportData;

    if((report_data != NULL) && (reportId != NULL) && (reportLen != NULL))
    {
        *reportLen              = 0;
        *reportId               = 0;  /* default value */

        for (zaehler = 0; zaehler < devinfo->dataCounter; zaehler++)
        {
            if (report_data[zaehler].rep_size >= (msgLen + IPOD_MESSAGE_START_LENGTH))
            {
                *reportLen = report_data[zaehler].rep_size;
                *reportId  = report_data[zaehler].rep_id;
                break;
            }

            if (maxLen < report_data[zaehler].rep_size)
            {
                maxLen    =  report_data[zaehler].rep_size;
                maxRepId  =  report_data[zaehler].rep_id;
            }
        }

        if ((*reportId == 0) || (msgLen == 0))
        {
            *reportLen = maxLen;
            *reportId  = maxRepId;
        }
    }
    else
    {
        if(report_data == NULL)
        {
            IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "report_data is NULL");
            rc = IPOD_DATACOM_NOT_CONNECTED;
        }
        else
        {
            IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "reportId = %p, reportLen = %p", reportId, reportLen);
            rc = IPOD_DATACOM_BAD_PARAMETER;
        }
    }

    return rc;
}

S32 iPodiAP2USBGetReportLenMax(IPOD_IAP2_HID_DEV_INFO* devinfo, U32 *reportLen)
{
    S32 rc = IPOD_DATACOM_SUCCESS;


    if((devinfo == NULL) || (reportLen == NULL))
    {
        IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "devinfo = %p, reportLen = %p", devinfo, reportLen);
        return IPOD_DATACOM_BAD_PARAMETER;
    }

    if(devinfo->maxDataLen == 0)
    {
        IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "devinfo->maxDataLen is 0");
        rc = IPOD_DATACOM_ERROR;
    }
    else
    {
        *reportLen = devinfo->maxDataLen;
    }

    return rc;
}

S32 iPodiAP2USBGetReportId_internal(IPOD_IAP2_HID_DEV_INFO* devinfo)
{
    S32 rc = IPOD_DATACOM_SUCCESS;
    U32 count = 0;
    IPOD_IAP2_REPORT_DATA* report_data = NULL;
    U32 maxLen = 0;

    if (devinfo->reportData != NULL)
    {
        IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "devinfo->reportData is NULL");
        rc = IPOD_DATACOM_ERROR;
    }
    if (rc == IPOD_DATACOM_SUCCESS)
    {
        rc = iPodiAP2HIDReadReportID(devinfo, &devinfo->reportData, &devinfo->dataCounter);
    }

    if(rc == IPOD_DATACOM_SUCCESS)
    {
        if(devinfo->reportData != NULL)
        {
            report_data = devinfo->reportData;
            for(count = 0; count < devinfo->dataCounter; count++)
            {
                if(maxLen < report_data[count].rep_size)
                {
                    maxLen = report_data[count].rep_size;
                }
            }

            devinfo->outBuf = calloc(maxLen, sizeof(U8));
            if(devinfo->outBuf != NULL)
            {
                /* One byte is report Id byte. This byte shoud be removed from maximum length */
                devinfo->maxDataLen = maxLen - 1;
            }
            else
            {
                IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "devinfo->outBuf is NULL");
                rc = IPOD_DATACOM_ERR_NOMEM;
            }
        }
        else
        {
            IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "devinfo->reportData is NULL");
            rc = IPOD_DATACOM_ERROR;
        }
    }
    else
    {
        IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "rc = %d", rc);
    }

    return rc;
}

void iPodiAP2USBComSleep(U32 sleep_ms)
{
    S32 s32ReturnValue;
    struct timespec req;
    struct timespec remain;

    /* Initialize the structure */
    memset(&req, 0, sizeof(req));
    memset(&remain, 0, sizeof(remain));


    req.tv_sec = sleep_ms / IPOD_COM_MSEC;
    req.tv_nsec = (sleep_ms % IPOD_COM_MSEC) * IPOD_COM_NSEC;

    while(1)
    {
        s32ReturnValue = nanosleep(&req, &remain);

        if (s32ReturnValue == 0)
        {
            break;
        }
        else
        {
            if (errno == EINTR)
            {
                req.tv_sec = remain.tv_sec ;
                req.tv_nsec = remain.tv_nsec;
            }
            else
            {
                break;
            }
        }
    }// end while

}

S32 iPodiAP2HIDGetINReportLength(IPOD_IAP2_HID_DEV_INFO* devinfo, U32 reportId, U32 *reportLen)
{
    U8              zaehler   = 0;
    S32             rc        = IPOD_DATACOM_SUCCESS;

    if((devinfo->inReportData != NULL) && (reportLen != NULL))
    {
        *reportLen = 0;

        for (zaehler = 0; zaehler < devinfo->inDataCounter; zaehler++)
        {
            if (devinfo->inReportData[zaehler].rep_id == reportId)
            {
                *reportLen = devinfo->inReportData[zaehler].rep_size;
                break;
            }
        }
    }
    else
    {
        IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "devinfo->inReportData = %p, reportLen = %p", devinfo->inReportData, reportLen);
        rc = IPOD_DATACOM_BAD_PARAMETER;
    }

    return rc;
}

/*
 *   usb connection open
 */
iap2_hid_device *iAP2_Hid_open(IPOD_IAP2_HID_DEV_INFO* devinfo, wchar_t *serial, S16 *vid, S16 *pid)
{
    struct iap2_hid_device_info  *devs, *cur_dev;
    iap2_hid_device *handle = NULL;

    *vid = APPLE_VID;
    *pid = 0;

    /* device search */
    if (NULL != devinfo)
    {
        cur_dev = devs = iap2_hid_enumerate(devinfo->usb_context, 0x0, 0x0);
        while (cur_dev)
        {
            if (cur_dev->vendor_id == APPLE_VID)
            {
                if ((cur_dev->product_id & APPLE_PID_MSK) == APPLE_PID_MSK)
                {
                    if ((cur_dev->serial_number != NULL) && (serial != NULL))
                    {
                        if (wcscmp(cur_dev->serial_number, serial) == 0)
                        {
                            *pid = cur_dev->product_id;
                            break;
                        }
                    }
                }
            }
            cur_dev = cur_dev->next;
        }
        iap2_hid_free_enumeration(devs);

        if (*pid != 0)
        {
            /* Open the device using the VID, PID, */
            /* and optionally the Serial number.   */
            handle = iap2_hid_open(devinfo->usb_context, *vid, *pid, serial);
        }

        IAP2USBPLUGINDLTLOG(DLT_LOG_DEBUG, "open finish (vid:0x%04x pid:0x%04x) handle:%p ", *vid, *pid, handle);
    }

    return handle;
}

S32 iAP2HidReadReport(iap2_hid_device *dd, U8 *buff, U32 len)
{
    S32 res;

    if (NULL != dd)
    {
        res = iap2_hid_read(dd, buff, len);
        if (HID_API_NOT_CON == res)
        {
            IAP2USBPLUGINDLTLOG(DLT_LOG_DEBUG, "res = %d", res);
            res = IPOD_DATACOM_NOT_CONNECTED;
        }
        else if (0 > res)
        {
            IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "res = %d", res);
            res = IPOD_DATACOM_ERROR;
        }
        else
        {
            DUMP(">>>>>>>>>>>>>>>>>> read report data", buff, res);
        }
    }
    else
    {
        IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "dd is NULL");
        res = IPOD_DATACOM_ERROR;
    }

    return res;
}

S32 iPodiAP2HIDLibUsbHandleEvent(IPOD_IAP2_HID_DEV_INFO *devInfo, U32 buffer_size, U8* msgBuffer, S32 pollFD)
{

    IPOD_IAP2_HID_DEV_INFO *pDevInfo  = devInfo;
    U8               *pBuf      = pDevInfo->Evnt.Buff;
    S32               res       = IPOD_DATACOM_SUCCESS;
    U32               reportId = 0;
    U32               reportLen   = 0;
    S32               trans_len   = 0;
    S32               cRecvBuffer = 0;
    U32               offset = 0;

    if (NULL != pDevInfo->dd)
    {
        /* handle libusb event to receive transfer */
        res = iap2_hid_libusb_handle_events(pDevInfo->usb_context, pDevInfo->dd, pollFD);
        /* immediately read of transfer data */
        if (res == IPOD_DATACOM_SUCCESS)
        {
            /* first read */
            res = iAP2HidReadReport(pDevInfo->dd, pBuf, HID_BUFF_MAX);
            /* data available */
            if (res > IPOD_DATACOM_SUCCESS)
            {
                /* substract report ID and hid control byte */
                trans_len = res - 2;

                reportId = pBuf[0];
                res = iPodiAP2HIDGetINReportLength(pDevInfo, reportId, &reportLen);
                /* substract hid control byte */
                reportLen--;

                if (offset + trans_len <= buffer_size)
                {
                    /* first message */
                    memcpy(&msgBuffer[offset], &pBuf[2], trans_len);
                    cRecvBuffer = reportLen - trans_len;
                    offset += trans_len;
                }
                else
                {
                    IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "offset = %d, trans_len = %d buffer_size = %d",
                                        offset, trans_len, buffer_size);
                    res = IPOD_DATACOM_ERROR;
                }
            }
            else if (res == IPOD_DATACOM_SUCCESS)
            {
                /* no data available. No error case */
                res = IPOD_DATACOM_SUCCESS;
            }
            else
            {
                /* error occurred */
                IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, " iAP2HidReadReport()  res = %d ", res);
            }
        }
        else if (res == HID_API_NOT_CON)
        {
            res = IPOD_DATACOM_NOT_CONNECTED;
            IAP2USBPLUGINDLTLOG(DLT_LOG_DEBUG, "res = %d", res);
        }
        else
        {
            IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "iap2_hid_libusb_handle_events error !!!! res = %d ", res);
        }
    }
    else
    {
        IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, " pDevInfo->dd = %p", pDevInfo->dd);
        res = IPOD_DATACOM_BAD_PARAMETER;
    }

    if (res == IPOD_DATACOM_SUCCESS)
    {
        /* read further messages */
        while ((cRecvBuffer > 0) && (res >= IPOD_DATACOM_SUCCESS))
        {
            res = iAP2HidReadReport(pDevInfo->dd, pBuf, HID_BUFF_MAX);
            if (res > IPOD_DATACOM_SUCCESS)
            {
                trans_len = res;
                if (offset + trans_len <= buffer_size)
                {
                    memcpy(&msgBuffer[offset], pBuf, trans_len); /* long message */
                    offset += trans_len;
                    cRecvBuffer -= trans_len;
                }
                else
                {
                    memcpy(&msgBuffer[offset], pBuf, buffer_size - offset);
                    offset += (buffer_size - offset);
                    cRecvBuffer -= trans_len;
                }
            }
            else if (res == IPOD_DATACOM_SUCCESS)
            {
                /* no data available. No error case */
                res = IPOD_DATACOM_SUCCESS;
            }
            else
            {
                IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "read report error  !!!! (res = %d report len = %d trans len = %d)",
                                    res, reportLen, trans_len);
            }
        }
    }

    if (res >= IPOD_DATACOM_SUCCESS)
    {
        res = offset;
    }

    return res;
}

S32 iPodiAP2HIDLibUSBGetFDs(IPOD_IAP2_HID_DEV_INFO* pDevInfo, iap2_hid_poll_fd *get_pollFDs, S32* fd_count)
{
    S32 rc = IPOD_DATACOM_SUCCESS;

    if(pDevInfo->dd != NULL)
    {
        rc = iap2_hid_libusb_get_pollfds(pDevInfo->dd, pDevInfo->usb_context, get_pollFDs, fd_count);
        if(rc < IPOD_DATACOM_SUCCESS)
        {
            IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "rc = %d", rc);
            rc = IPOD_DATACOM_ERROR;
        }
        else
        {
            rc = IPOD_DATACOM_SUCCESS;
        }
    }
    else
    {
        IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "pDevInfo->dd is NULL");
        rc = IPOD_DATACOM_ERROR;
    }

    return rc;
}

S32 iPodiAP2HIDOpen(IPOD_IAP2_HID_DEV_INFO* pDevInfo, const U8* drivername, S32 mode)
{
    S32                 res = IPOD_DATACOM_SUCCESS;
    iap2_hid_device     *dd = NULL;
    S16                 vid, pid;
    wchar_t dest[IPOD_COM_DEVICE_NAME_MAX] = {0};

    if(drivername == NULL)
    {
        IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "drivername is NULL");
        return IPOD_DATACOM_BAD_PARAMETER;
    }

    /*
     * build socket message
     */
    mbstowcs(dest, (const char *)drivername, (sizeof(dest) / sizeof(dest[0])));

    dd = iAP2_Hid_open(pDevInfo, (wchar_t *)dest, &vid, &pid);  /* hidapi open */
    if (NULL != dd)
    {                     /* open ok ? */
        pDevInfo->dd = dd;              /* handle */
        pDevInfo->vid = vid;            /* vendor id */
        pDevInfo->pid = pid;            /* product id */

        res = iap2_hid_set_nonblocking(dd, mode);
    }
    else
    {
        IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "dd is NULL");
        pDevInfo->dd = NULL;    /* handle NG */
        res = IPOD_DATACOM_NOT_CONNECTED;
    }
    IAP2USBPLUGINDLTLOG(DLT_LOG_DEBUG, "res = %d", res);

    return res;
}

S32 iPodiAP2HIDClose(IPOD_IAP2_HID_DEV_INFO* pDevInfo)
{
    if (NULL != pDevInfo->dd)
    {
        IAP2USBPLUGINDLTLOG(DLT_LOG_DEBUG, "close occur ! (%p)", pDevInfo->dd);

        /* hidapi close */
        iap2_hid_close(pDevInfo->usb_context, pDevInfo->dd);
        pDevInfo->dd = NULL;
    }

    if(pDevInfo->outBuf != NULL)
    {
        free(pDevInfo->outBuf);
        pDevInfo->outBuf = NULL;
    }
    IAP2USBPLUGINDLTLOG(DLT_LOG_DEBUG, "close finish");

    return IPOD_DATACOM_SUCCESS;
}

S32 iPodiAP2HIDWaitEvent(IPOD_IAP2_HID_DEV_INFO *devInfo, U32 buffer_size, U8* msgBuffer)
{
    IPOD_IAP2_HID_DEV_INFO *pDevInfo  = devInfo;
    U8               *pBuf      = pDevInfo->Evnt.Buff;
    S32               res       = IPOD_DATACOM_SUCCESS;
    U32               reportId = 0;
    U32               reportLen   = 0;
    S32               trans_len   = 0;
    S32               cRecvBuffer = 0;
    U32               offset = 0;

    /* for compiler warning */
    buffer_size = buffer_size;

    /* first read */
    trans_len = res = iAP2HidReadReport(pDevInfo->dd, pBuf, HID_BUFF_MAX);
    if (res >= IPOD_DATACOM_SUCCESS)
    {
        reportId = pBuf[0];
        trans_len--;
        res = iPodiAP2HIDGetINReportLength(pDevInfo, reportId, &reportLen);
        if (res == IPOD_DATACOM_SUCCESS)
        {
            if (offset + trans_len <= buffer_size)
            {
                memcpy(&msgBuffer[offset], &pBuf[1], trans_len); /* first message */
                cRecvBuffer = reportLen - trans_len;
                offset += trans_len;

                while ((cRecvBuffer > 0) && (res >= IPOD_DATACOM_SUCCESS))
                {
                    trans_len = res = iAP2HidReadReport(pDevInfo->dd, pBuf, HID_BUFF_MAX);
                    if (res >= IPOD_DATACOM_SUCCESS)
                    {
                        if (offset + trans_len <= buffer_size)
                        {
                            memcpy(&msgBuffer[offset], pBuf, trans_len); /* long message */
                            offset += trans_len;
                            cRecvBuffer -= trans_len;
                        }
                        else
                        {
                            memcpy(&msgBuffer[offset], pBuf, buffer_size - offset);
                            offset += (buffer_size - offset);
                            cRecvBuffer -= trans_len;
                        }
                    }
                    else
                    {
                        IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "read report error  !!!! (report len = %d trans len = %d)", reportLen, trans_len);
                    }
                }
            }
            else
            {
                IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "offset = %d, trans_len = %d buffer_size = %d", offset, trans_len, buffer_size);
                res = IPOD_DATACOM_ERROR;
            }
        }
        else
        {
            res = IPOD_DATACOM_ERROR;
            IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "read report(first) error !!!! (report len = %d trans len = %d)", reportLen, trans_len);
        }
    }

    if (res >= IPOD_DATACOM_SUCCESS)
    {
        res = offset;
    }

    return res;
}

S32 iPodiAP2HIDReadReportID(VP                      devInfo,
                            IPOD_IAP2_REPORT_DATA   **report_data,
                            U32                     *data_counter)
{
    P_IAP2_HID_DESC_INF       inf;
    IPOD_IAP2_HID_DEV_INFO    *pDevInfo       = devInfo;

    U32                     i         = 0;
    S32                     res       = IPOD_DATACOM_SUCCESS;

    if (NULL != pDevInfo->dd)
    {
        /* gets report informaton  */
        inf = iAP2HidGetReportInformation(pDevInfo->dd);
        if (!inf)
        {
            IAP2USBPLUGINDLTLOG(DLT_LOG_DEBUG, "iAP2HidGetReportInformation() inf = %p", inf);
            res = IPOD_DATACOM_ERR_NOMEM;
        }
        else
        {
            /* allocate memory for in report */
            if (NULL == pDevInfo->inReportData)
            {
                pDevInfo->inReportData = calloc( inf->act_sz_in, sizeof(IPOD_IAP2_REPORT_DATA));
                pDevInfo->inDataCounter = inf->act_sz_in;
            }

            /* allocate data for out report */
            if (NULL == *report_data)
            {
                *report_data = calloc( inf->act_sz_out, sizeof(IPOD_IAP2_REPORT_DATA));
                (*data_counter) = inf->act_sz_out;
            }
            if ((NULL != *report_data) && (NULL != pDevInfo->inReportData))
            {
                for (i = 0; i < pDevInfo->inDataCounter; i++)
                {
                    pDevInfo->inReportData[i].rep_size = inf->rep[i].len;
                    pDevInfo->inReportData[i].rep_id = inf->rep[i].rid;
                }
                for (i = 0; i < (*data_counter); i++)
                {
                    (*report_data)[i].rep_size = inf->rep[i + inf->act_sz_in].len;
                    (*report_data)[i].rep_id = inf->rep[i + inf->act_sz_in].rid;
                }
            }
            else
            {
                IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "*report_data = %p, pDevInfo->inReportData = %p", *report_data, pDevInfo->inReportData);
                res = IPOD_DATACOM_ERR_NOMEM;
            }
        }
    }
    else
    {
        IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "pDevInfo->dd is NULL");
        res = IPOD_DATACOM_ERROR;
    }

    return res;
}

S32 iPodiAP2HIDWriteReport(IPOD_IAP2_HID_DEV_INFO *devinfo, U32 len, const U8 *buff)
{
    S32 res;
    U32 reportId = 0;
    U32 reportLen = 0;

    DUMP("<<<<<<<<<<<<<<<<<< write report data", buff, len);

    if((devinfo == NULL) || (buff == NULL))
    {
        IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "devinfo = %p, buff = %p", devinfo, buff);
        return IPOD_DATACOM_BAD_PARAMETER;
    }


    if(devinfo->dd != NULL)
    {
        memset(devinfo->outBuf, 0, devinfo->maxDataLen + 1);
        res = iPodiAP2USBGetReportId(devinfo, len, &reportId, &reportLen);
        if(res == IPOD_DATACOM_SUCCESS)
        {
            if(devinfo->maxDataLen >= len)
            {
                devinfo->outBuf[0] = (U8)reportId;
                memcpy(&devinfo->outBuf[1], buff, len);
                res = iap2_hid_write(devinfo->dd, devinfo->outBuf, reportLen);
                if(res == HID_API_NOT_CON)
                {
                    res = IPOD_DATACOM_NOT_CONNECTED;
                    IAP2USBPLUGINDLTLOG(DLT_LOG_DEBUG, "res = %d", res);
                }
                else if(0 > res)
                {
                    res = IPOD_DATACOM_ERROR;
                    IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "res = %d", res);
                }
                else if((U32)res == reportLen)
                {
                    /* complete message was sent, return message length */
                    res = len;
                }
                else
                {
                    res = res;
                }
            }
            else
            {
                res = IPOD_DATACOM_BAD_PARAMETER;
                IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "res = %d", res);
            }
        }
    }
    else
    {
        IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "devinfo->dd is NULL");
        res = IPOD_DATACOM_ERROR;
    }

    return res;
}

S32 iPodiAP2HIDInit(IPOD_IAP2_HID_DEV_INFO *devInfo)
{
    S32 res = IPOD_DATACOM_SUCCESS;

    if (devInfo != NULL)
    {
        memset(devInfo, 0, sizeof(*devInfo));
    }
    else
    {
        res = IPOD_DATACOM_ERR_NOMEM;
        IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "res = %d", res);
    }

    return res;
}

void iPodiAP2HIDDeInit(IPOD_IAP2_HID_DEV_INFO *devinfo)
{
    if(devinfo != NULL)
    {
        if(devinfo->inReportData != NULL)
        {
            free(devinfo->inReportData);
            devinfo->inReportData = NULL;
        }
        devinfo->inDataCounter = 0;
        if(devinfo->reportData != NULL)
        {
            free(devinfo->reportData);
            devinfo->reportData = NULL;
        }
        devinfo->dataCounter = 0;
        devinfo->dd = NULL;
    }
}

S32 iPodiAP2HIDTrnspInit(IPOD_IAP2_HID_DEV_INFO* devinfo)
{
    S32                  res      = IPOD_DATACOM_SUCCESS;

    devinfo->usb_context = iap2_hid_init();

    if (NULL != devinfo->usb_context)
    {
        res = HID_API_OK;
    }
    else
    {
        res = HID_API_NG;
    }
    if (res == HID_API_OK)
    {
        res = IPOD_DATACOM_SUCCESS;
    }
    else
    {
        res = IPOD_DATACOM_ERROR;
    }
    IAP2USBPLUGINDLTLOG(DLT_LOG_DEBUG, "res = %d", res);

    return res;
}

S32 iPodiAP2HIDTrnspDeInit(IPOD_IAP2_HID_DEV_INFO* devinfo)
{
    return iap2_hid_exit(devinfo->usb_context);
}

