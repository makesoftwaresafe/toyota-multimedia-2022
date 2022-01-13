
#include <errno.h>
#include <stdint.h>

#include "iPodDataCom.h"
#include "iPodDataCom_i.h"
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
#include "pthread_adit.h"
#include <libusb-1.0/libusb.h>
#include "hidapi.h"
#include "iap_hid_desc.h"
#include "iap1_dlt_log.h"

#define FAILED              -1
#define APPLE_VID           0x05ac
#define APPLE_PID_MSK       0x1200

/* #define DEBUG_PRINTF */

#define COMP             "ipod_ctrl: "

#ifdef DEBUG_PRINTF
#define DUMP(c, a, l) mem_dsp(c ,a ,l)
#else
#define DUMP(c, a, l)
#endif 
S32 iPodHIDReadReportID(VP devInfo, IPOD_REPORT_DATA **report_data, U32 *data_counter);
S32 iPodUSBGetReportId(IPOD_HID_DEVINFO* devinfo, U32 msgLen, U32 *reportId, U32 *reportLen);

static int   g_log_fd = IPOD_DATACOM_ERROR;
static sem_t g_log_lock = {};
static sem_t g_ctrl_dev_lock = {};

hid_device *Hid_open(wchar_t *serial, S16 *vid, S16 *pid);

static void dumpf(int fd, const char* fmt, ...)
{
    /*PRQA: Lint Message 530: This is intention. va_list will be initialized by va_start (see stdarg.h). */
    /*lint -save -e530*/
    va_list args;
    char    buf[256];
    S32     len      = 0;
    S32     ret      = 0;

    if (FAILED != fd)
    {
        va_start(args, fmt);
        len = vsnprintf(buf, sizeof(buf), fmt, args);
        va_end(args);
        /*lint -restore*/

        if (len > 0)
        {
            if (((unsigned int)len) > sizeof(buf))
            {
                len = sizeof(buf);
            }
            ret = write(fd, buf, (U32)len);
            if(ret < 0)
            {
                IAP1_USBP_LOG(DLT_LOG_ERROR, "Failed to write, Error:%d(%s)",errno, strerror(errno));
            }
        }
    }
}

#if 0
static void dumph(int fd, void *data_, int len)
{
    char    *data    = data_;
    char     hex[]   = "0123456789ABCDEF";
    char     digit[] =  {' ', ' ', ' '};

    if (FAILED != fd)
    {
        while (0 < len)
        {
            digit[0] = hex[0xf & ((*data) >> 4)];
            digit[1] = hex[0xf & (*data)];
            write(fd, digit, sizeof(digit));
            data++;
            len--;
        }
    }
}
#endif /* if 0 */


/*!
 * \func iPodUSBGetReportId
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
S32 iPodUSBGetReportId(IPOD_HID_DEVINFO* devinfo, U32 msgLen, U32 *reportId, U32 *reportLen)
{
    U32  zaehler             = 0;
    U32 maxLen              = 0;
    U32 maxRepId            = 0;
    S32 rc                  = IPOD_DATACOM_SUCCESS;
    IPOD_REPORT_DATA* report_data = NULL;

    if((devinfo != NULL) && (reportId != NULL) && (reportLen != NULL) &&
       (devinfo->reportData != NULL))
    {
        report_data = devinfo->reportData;
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
        if((devinfo != NULL) && (devinfo->reportData == NULL))
        {
            rc = IPOD_DATACOM_NOT_CONNECTED;
            IAP1_USBP_LOG(DLT_LOG_ERROR, "iPod Datacom Not Connected devinfo = %p devinfo->reportData = %p",devinfo,devinfo->reportData);
        }
        else
        {
            rc = IPOD_DATACOM_BAD_PARAMETER;
            IAP1_USBP_LOG(DLT_LOG_ERROR, "devinfo = %p reportId = %p reportLen = %p",devinfo,reportId,reportLen);
        }
    }

    return rc;
}

S32 iPodUSBGetReportLenMax(IPOD_HID_DEVINFO* devinfo, U32 *reportLen)
{
    S32 rc = IPOD_DATACOM_SUCCESS;
    
    
    if((devinfo == NULL) || (reportLen == NULL))
    {
        IAP1_USBP_LOG(DLT_LOG_ERROR, "iPod Datacom Bad Parameter devinfo = %p reportLen = %p ",devinfo,reportLen);
        return IPOD_DATACOM_BAD_PARAMETER;
    }
    
    if(devinfo->maxDataLen == 0)
    {
        rc = IPOD_DATACOM_ERROR;
        IAP1_USBP_LOG(DLT_LOG_ERROR, "iPod Datacom Error - maxDataLen is Zero");
    }
    else
    {
        *reportLen = devinfo->maxDataLen;
    }
    
    return rc;
}

S32 iPodUSBGetReportId_internal(IPOD_HID_DEVINFO* devinfo)
{
    S32 rc = IPOD_DATACOM_ERROR;
    U32 count = 0;
    IPOD_REPORT_DATA* report_data = NULL;
    U32 maxLen = 0;
    
    if ((devinfo == NULL) || (devinfo->reportData != NULL))
    {
        IAP1_USBP_LOG(DLT_LOG_ERROR, "iPod Datacom error devinfo = %p",devinfo);
        return IPOD_DATACOM_ERROR;
    }
    
    rc = iPodHIDReadReportID(devinfo, &devinfo->reportData, &devinfo->dataCounter);
    if(rc == IPOD_DATACOM_SUCCESS)
    {
        if(devinfo->reportData != NULL)
        {
            report_data = devinfo->reportData;
            maxLen = 0;
            for(count = 0; count < devinfo->dataCounter; count++)
            {
                if(maxLen < report_data[count].rep_size)
                {
                    maxLen = report_data[count].rep_size;
                }
            }
            
            /* Writable length for user is more than 1 byte because 1 byte is used for reportID */
            if(maxLen > 1)
            {
                devinfo->outBuf = calloc(maxLen, sizeof(U8));
                if(devinfo->outBuf != NULL)
                {
                    /* One byte is report Id byte. This byte shoud be removed from maximum length */
                    devinfo->maxDataLen = maxLen - 1;
                }
                else
                {
                    devinfo->maxDataLen = 0;
                    rc = IPOD_DATACOM_ERR_NOMEM;
                    IAP1_USBP_LOG(DLT_LOG_ERROR, "No Memory devinfo->outBuf is NULL");

                }
            }
            else
            {
                devinfo->maxDataLen = 0;
                rc = IPOD_DATACOM_ERROR;
                IAP1_USBP_LOG(DLT_LOG_ERROR, "iPod Datacom Error - maxLen is not greater than 1");
            }
        }
        else
        {
            devinfo->dataCounter = 0;
            devinfo->maxDataLen = 0;
            rc = IPOD_DATACOM_ERROR;
            IAP1_USBP_LOG(DLT_LOG_ERROR, "iPod Datacom Error - devinfo->reportData is NULL");
        }
    }
    else
    {
        IAP1_USBP_LOG(DLT_LOG_ERROR, "iPodHIDReadReportID() returns rc = %d", rc);
        devinfo->dataCounter = 0;
        devinfo->maxDataLen = 0;
    }
    
    return rc;
}

void iPodUSBComSleep(U32 sleep_ms)
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

S32 iPodHIDGetINReportLength(IPOD_HID_DEVINFO* devinfo, U32 reportId, U32 *reportLen)
{
    U8              zaehler   = 0;
    S32             rc        = IPOD_DATACOM_SUCCESS;
    
    if((devinfo != NULL) && (devinfo->inReportData != NULL) && (reportLen != NULL))
    {
        *reportLen              = 0;
        
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
        rc = IPOD_DATACOM_BAD_PARAMETER;
        IAP1_USBP_LOG(DLT_LOG_ERROR, "iPod Datacom Bad Parameter devinfo = %p reportLen = %p",devinfo,reportLen);
    }

    return rc;
}

/*
*   usb connection open
*/
hid_device *Hid_open(wchar_t *serial, S16 *vid, S16 *pid)
{
    struct hid_device_info  *devs, *cur_dev;
    hid_device *handle = NULL;

    if((serial == NULL) || (vid == NULL) || (pid == NULL))
    {
        return NULL;
    }
    IAP1_USBP_LOG(DLT_LOG_INFO, "open occur !");
    *vid = APPLE_VID;
    *pid = 0;

    /* device search */
    cur_dev = devs = hid_enumerate(0x0, 0x0);
    while(cur_dev){
        if(cur_dev->vendor_id == APPLE_VID){
            if((cur_dev->product_id & APPLE_PID_MSK) == APPLE_PID_MSK){
                if((cur_dev->serial_number != NULL) && (serial != NULL)){
                    if (wcscmp(cur_dev->serial_number, serial) == 0) {
                        *pid = cur_dev->product_id;
                        break;
                    }
                }
            }
        }
        cur_dev = cur_dev->next;
    }
    hid_free_enumeration(devs);

    if(*pid != 0){
        /* Open the device using the VID, PID, */
        /* and optionally the Serial number.   */
        handle = hid_open(*vid, *pid, serial);
    }
    IAP1_USBP_LOG(DLT_LOG_INFO, "open finish (vid:0x%04x pid:0x%04x) handle:%p \n", *vid, *pid, handle);

    return handle;
}

S32 iPodHIDOpen(IPOD_HID_DEVINFO* pDevInfo, const U8* drivername, S32 mode)
{
    S32                 res       = IPOD_DATACOM_SUCCESS;
    hid_device          *dd = NULL;
    S16                 vid, pid;
    wchar_t dest[IPOD_COM_DEVICE_NAME_MAX] = {0};

    if((pDevInfo == NULL) || (drivername == NULL))
    {
        IAP1_USBP_LOG(DLT_LOG_ERROR, "iPod Datacom Bad Parameter pDevInfo = %p drivername = %p",pDevInfo,drivername);
        return IPOD_DATACOM_BAD_PARAMETER;
    }
    
    /*
     * build socket message
     */
    mbstowcs(dest, (const char *)drivername, (sizeof(dest) / sizeof(dest[0])));
    (void)sem_wait(&g_ctrl_dev_lock);
    dd = Hid_open((wchar_t *)dest, &vid, &pid);  /* hidapi open */
    if (NULL != dd)
    {                     /* open ok ? */
        pDevInfo->dd = dd;              /* handle */
        pDevInfo->vid = vid;            /* vendor id */
        pDevInfo->pid = pid;            /* product id */

        res = hid_set_nonblocking(dd, mode);
    }
    else
    {
        pDevInfo->dd = NULL;    /* handle NG */
        res = IPOD_DATACOM_NOT_CONNECTED;
        IAP1_USBP_LOG(DLT_LOG_ERROR, "iPod Datacom Not Connected");
    }

    (void)sem_post(&g_ctrl_dev_lock);
    return res;
}

S32 iPodHIDClose(IPOD_HID_DEVINFO* pDevInfo)
{
    if(pDevInfo == NULL)
    {
        IAP1_USBP_LOG(DLT_LOG_ERROR, "pDevInfo is NULL");
        return IPOD_DATACOM_BAD_PARAMETER;
    }
    

   (void)sem_wait(&g_ctrl_dev_lock);

    if (NULL != pDevInfo->dd)
    {
        hid_close(pDevInfo->dd);        /* hidapi close */
        pDevInfo->dd = NULL;

    }
    
    if(pDevInfo->outBuf != NULL)
    {
        free(pDevInfo->outBuf);
        pDevInfo->outBuf = NULL;
    }

    (void)sem_post(&g_ctrl_dev_lock);
    IAP1_USBP_LOG(DLT_LOG_INFO, "close finish");

    return IPOD_DATACOM_SUCCESS;
}

S32 HidReadReport(hid_device *dd, U8 *buff, U32 len)
{
    S32 res;
    int rc = 0;
    
    if ((NULL != dd) && (NULL != buff))
    {
        res = hid_read(dd, buff, len);
        if(res == HID_API_NOT_CON)
        {
            res = IPOD_DATACOM_NOT_CONNECTED;
            IAP1_USBP_LOG(DLT_LOG_ERROR, "iPod Datacom Not Connected");
        }
        else if(0 > res)
        {
            res = IPOD_DATACOM_ERROR;
            IAP1_USBP_LOG(DLT_LOG_WARN, "iPod Datacom Error hid_read() returns res = %d",res);
            dumpf(g_log_fd, COMP "rx : 0x%08x\n", rc);
            
        } else {
            DUMP(">>>>>>>>>>>>>>>>>> read report data", buff, res);
            
        }
    }
    else
    {
        pthread_testcancel();
        res = IPOD_DATACOM_ERROR;
        IAP1_USBP_LOG(DLT_LOG_ERROR, "iPod Datacom Error dd = %p buff = %p",dd,buff);
    }
    IAP1_USBP_LOG(DLT_LOG_DEBUG, "read result (size = %d)", res);

    return res;
}

S32 iPodHIDWaitEvent(IPOD_HID_DEVINFO *devInfo, U32 buffer_size, U8* msgBuffer)
{
    IPOD_HID_DEVINFO *pDevInfo  = NULL;
    U8               *pBuf      = NULL;
    S32               res       = IPOD_DATACOM_SUCCESS;
    U32               reportId = 0;
    U32               reportLen   = 0;
    S32               trans_len   = 0;
    S32               cRecvBuffer = 0;
    U32               offset = 0;
    
    if((devInfo == NULL) || (msgBuffer == NULL) ||
       (devInfo->dd == NULL))
    {
        IAP1_USBP_LOG(DLT_LOG_ERROR, "devInfo = %p msgBuffer = %p",devInfo,msgBuffer);
        return IPOD_DATACOM_BAD_PARAMETER;
    }
    
    pDevInfo = devInfo;
    pBuf = pDevInfo->Evnt.Buff;
        
    /* for compiler warning */
    buffer_size = buffer_size;
    
    /* first read */
    trans_len = res = HidReadReport(pDevInfo->dd, pBuf, HID_BUFF_MAX);
    if(IPOD_DATACOM_ERROR != res)
    {
        reportId = pBuf[0];
        trans_len--;
        res = iPodHIDGetINReportLength(pDevInfo, reportId, &reportLen);
        IAP1_USBP_LOG(DLT_LOG_DEBUG, "read report(first) ok (report len = %d trans len = %d)", reportLen, trans_len);
        if(res == IPOD_DATACOM_SUCCESS) {
            if(offset + trans_len <= buffer_size)
            {
                memcpy(&msgBuffer[offset], &pBuf[1], trans_len);    /* first message */
                cRecvBuffer = reportLen - trans_len;
                offset += trans_len;
                
                while((cRecvBuffer > 0) && (res != IPOD_DATACOM_ERROR))
                {
                    trans_len = res = HidReadReport(pDevInfo->dd, pBuf, HID_BUFF_MAX);
                    if(IPOD_DATACOM_ERROR != res)
                    {
                        if(offset + trans_len <= buffer_size)
                        {
                            memcpy(&msgBuffer[offset], pBuf, trans_len);   /* long message */
                            offset += trans_len;
                            cRecvBuffer -= trans_len;
                            IAP1_USBP_LOG(DLT_LOG_DEBUG, "read report ok (report len = %d trans len = %d)\n", reportLen, trans_len);
                        }
                        else
                        {
                            memcpy(&msgBuffer[offset], pBuf, buffer_size - offset);
                            offset += (buffer_size - offset);
                            cRecvBuffer -= trans_len;
                        }
                    } else {
                        IAP1_USBP_LOG(DLT_LOG_ERROR, "read report error  !!!! (report len = %d trans len = %d)\n", reportLen, trans_len);
                    }
                }
            }
            else
            {
                res = IPOD_DATACOM_ERROR;
                IAP1_USBP_LOG(DLT_LOG_ERROR, "iPod Datacom error offset + trans_len is greater than buffer_size");
            }
        } else {
            res = IPOD_DATACOM_ERROR;
            IAP1_USBP_LOG(DLT_LOG_ERROR, "read report(first) error !!!! (report len = %d trans len = %d)\n", reportLen, trans_len);
        }
    }

    if (IPOD_DATACOM_ERROR != res)
    {
        res = offset;
    }
    else
    {
        pthread_testcancel();
    }
    
    return res;
}

S32 iPodHIDReadReportID(VP                        devInfo,
                        IPOD_REPORT_DATA        **report_data,
                        U32                      *data_counter)
{
    PHID_DESC_INF       inf = NULL;
    IPOD_HID_DEVINFO    *pDevInfo       = NULL;

    U32                     i         = 0;
    S32                     res       = IPOD_DATACOM_SUCCESS;

    if((devInfo == NULL) || (report_data == NULL) || (data_counter == NULL))
    {
        IAP1_USBP_LOG(DLT_LOG_ERROR, "iPod Datacom error devInfo = %p report_data = %p data_counter = %p",devInfo,report_data,data_counter);
        return IPOD_DATACOM_BAD_PARAMETER;
    }

    pDevInfo = (IPOD_HID_DEVINFO *)devInfo;

    (void)sem_wait(&g_ctrl_dev_lock);
    if (NULL != pDevInfo->dd)
    {
        IAP1_USBP_LOG(DLT_LOG_DEBUG, "read report descriptor occur ! (handle:%p)\n", pDevInfo->dd);
        
        /* gets report informaton  */
        inf = HidGetReportInformation(pDevInfo->dd);
        if(inf != NULL)
        {
            /* allocate memory for in report */
            if (NULL == pDevInfo->inReportData)
            {
                if(inf->act_sz_in > 0)
                {
                    pDevInfo->inReportData = calloc( inf->act_sz_in, sizeof(IPOD_REPORT_DATA));
                    pDevInfo->inDataCounter = inf->act_sz_in;
                }
                else
                {
                    pDevInfo->inDataCounter = 0;
                }
            }
            
            /* allocate data for out report */
            if (NULL == *report_data)
            {
                if(inf->act_sz_out > 0)
                {
                    *report_data = calloc( inf->act_sz_out, sizeof(IPOD_REPORT_DATA));
                    (*data_counter) = inf->act_sz_out;
                }
                else
                {
                    (*data_counter) = 0;
                }
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
                if(pDevInfo->inReportData != NULL)
                {
                    free(pDevInfo->inReportData);
                    pDevInfo->inReportData = NULL;
                    pDevInfo->inDataCounter = 0;
                }
                
                if(*report_data != NULL)
                {
                    free(*report_data);
                    *report_data = NULL;
                    (*data_counter) = 0;
                }
                
                res = IPOD_DATACOM_ERR_NOMEM;
                IAP1_USBP_LOG(DLT_LOG_ERROR, "No Memory *report_data = %p pDevInfo->inReportData = %p",*report_data,pDevInfo->inReportData);
            }
        }
        else
        {
            res = IPOD_DATACOM_ERROR;
            IAP1_USBP_LOG(DLT_LOG_ERROR, "iPod Datacom error - inf is NULL");
        }
    }
    else
    {
        res = IPOD_DATACOM_ERROR;
        IAP1_USBP_LOG(DLT_LOG_ERROR, "iPod Datacom error - pDevInfo->dd is NULL");
    }

    (void)sem_post(&g_ctrl_dev_lock);
    IAP1_USBP_LOG(DLT_LOG_DEBUG, "read report descriptor result (%d)", res);

    return res;
}

S32 iPodHIDWriteReport(IPOD_HID_DEVINFO *devinfo, U32 len, const U8 *buff)
{
    S32 res;
    U32 reportId = 0;
    U32 reportLen = 0;
    
    DUMP("<<<<<<<<<<<<<<<<<< write report data", buff, len);
    
    if((devinfo == NULL) || (buff == NULL))
    {
        IAP1_USBP_LOG(DLT_LOG_ERROR, "Bad Parameter devinfo = %p buff = %p",devinfo,buff);
        return IPOD_DATACOM_BAD_PARAMETER;
    }
    
    (void)sem_wait(&g_ctrl_dev_lock);
    
    if((devinfo->dd != NULL) && (devinfo->outBuf != NULL))
    {
        IAP1_USBP_LOG(DLT_LOG_DEBUG, "write report occur ! (handle:%p)\n", devinfo->dd);
        
        memset(devinfo->outBuf, 0, devinfo->maxDataLen + 1);
        res = iPodUSBGetReportId(devinfo, len, &reportId, &reportLen);
        IAP1_USBP_LOG(DLT_LOG_VERBOSE, "iPodUSBGetReportId() returns : res = %d",res);
        if(res == IPOD_DATACOM_SUCCESS)
        {
            if(devinfo->maxDataLen >= len)
            {
                devinfo->outBuf[0] = (U8)reportId;
                memcpy(&devinfo->outBuf[1], buff, len);
                res = hid_write(devinfo->dd, devinfo->outBuf, reportLen);
                if(res == HID_API_NOT_CON)
                {
                    res = IPOD_DATACOM_NOT_CONNECTED;
                    IAP1_USBP_LOG(DLT_LOG_ERROR, "iPod Datacom Not Connected");
                }
                else if(0 > res)
                {
                    res = IPOD_DATACOM_ERROR;
                    IAP1_USBP_LOG(DLT_LOG_ERROR, "iPod Datacom error hid_write() returns res = %d",res);
                    dumpf(g_log_fd, COMP "tx : 0x%08x\n", res);
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
                IAP1_USBP_LOG(DLT_LOG_ERROR, "maxDataLen is less than len");
            }
        }
    }
    else
    {
        res = IPOD_DATACOM_ERROR;
        IAP1_USBP_LOG(DLT_LOG_ERROR, "iPod Datacom error devinfo->dd = %p devinfo->outBuf = %p",devinfo->dd,devinfo->outBuf);
    }
    
    (void)sem_post(&g_ctrl_dev_lock);
    IAP1_USBP_LOG(DLT_LOG_DEBUG, "write result (size = %d)\n", res);

    return res;
}

S32 iPodHIDInit(IPOD_HID_DEVINFO *devInfo)
{
    S32               res     = IPOD_DATACOM_SUCCESS;

    if (devInfo != NULL)
    {
        memset(devInfo, 0, sizeof(*devInfo));
    }
    else
    {
        res = IPOD_DATACOM_ERR_NOMEM;
        IAP1_USBP_LOG(DLT_LOG_ERROR, "No Memory devInfo is NULL");
    }

    return res;
}

void iPodHIDDeInit(IPOD_HID_DEVINFO *devinfo)
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

        memset(devinfo, 0, sizeof(*devinfo));
    }
}

S32 iPodHIDTrnspInit(U32 num_devices)
{
    S32                  res      = IPOD_DATACOM_SUCCESS;

    if (num_devices > 0)
    {
        g_log_fd = open("/tmp/ipod.log", O_WRONLY | O_TRUNC,
                        S_IRUSR | S_IWUSR);

        (void)sem_init(&g_log_lock, 0, 1);
        (void)sem_init(&g_ctrl_dev_lock, 0, 1);

        (void)sem_wait(&g_ctrl_dev_lock);

        res = hid_init();
        if(res == HID_API_OK){
            res = IPOD_DATACOM_SUCCESS;
        }else{
            res = IPOD_DATACOM_ERROR;
            IAP1_USBP_LOG(DLT_LOG_ERROR, "hid_init() returns res = %d",res);
        }

        (void)sem_post(&g_ctrl_dev_lock);
    }

    return res;
}

void iPodHIDTrnspDeInit(void)
{
    int value = -1;
    /* check if semaphore was initialized */
    sem_getvalue(&g_ctrl_dev_lock, &value);
    if(value > 0)
    {
        (void)sem_wait(&g_ctrl_dev_lock);

        hid_exit();

        (void)sem_post(&g_ctrl_dev_lock);
    }
}

/* TODO: ToPo: logging */

