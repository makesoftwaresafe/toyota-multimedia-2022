
#include "iap2_test_utility.h"

#include "iap2_dlt_log.h"


BOOL g_UtilityQuit = FALSE;
iap2UdevDevice_t g_UtilityDevices[MAX_DEVICES];
U16 g_numDevices = 0;

/* **********************  locals    ********************** */

LOCAL S32 iap2DevPowerRequest(U16 vid, U16 pid);

/* **********************  functions ********************** */

void iap2SetUtilityQuit(BOOL value)
{
    g_UtilityQuit = value;
}
BOOL iap2GetUtilityQuit(void)
{
    return g_UtilityQuit;
}

void iap2TestFreePtr(void** input_ptr)
{
    if(*input_ptr != NULL)
    {
        free(*input_ptr);
        *input_ptr = NULL;
    }
}

/* provide the structure with serial nr., product name, vendor and product ID */
S32 iap2GetUdevDevice(U16 index, iap2UdevDevice_t* udevDevice)
{
    S32 rc = 0;
    S32 len = 0;

    if(g_numDevices > 0){
        if((index > 0) && (index >= g_numDevices)){
            rc = -1;
        } else if(  (NULL == g_UtilityDevices[index].serialNum)
                  ||(NULL == g_UtilityDevices[index].productName)
                  ||(NULL == g_UtilityDevices[index].udevPath)  ){
            rc = -1;
        } else{
            len = strnlen((const char*)g_UtilityDevices[index].serialNum, MAX_STRING_LEN);
            udevDevice->serialNum = (U8*)strndup( (const char*)g_UtilityDevices[index].serialNum, len);
            len = strnlen((const char*)g_UtilityDevices[index].productName, MAX_STRING_LEN);
            udevDevice->productName = (U8*)strndup( (const char*)g_UtilityDevices[index].productName, len);
            len = strnlen((const char*)g_UtilityDevices[index].udevPath, MAX_STRING_LEN);
            udevDevice->udevPath = (U8*)strndup( (const char*) g_UtilityDevices[index].udevPath, len);
            if( (udevDevice->serialNum   == NULL) ||
                 (udevDevice->productName == NULL) ||
                 (udevDevice->udevPath    == NULL) ){
                rc = -1;
                IAP2TESTDLTLOG(DLT_LOG_ERROR,
                               "Failed to set Udev Device structure    \
                               serialNum = %p,                         \
                               productName = %p,                       \
                               udevPath = %p,",
                               udevDevice->serialNum,
                               udevDevice->productName,
                               udevDevice->udevPath);
            }
            if(0 == rc){
                udevDevice->vendorId = g_UtilityDevices[index].vendorId;
                udevDevice->productId = g_UtilityDevices[index].productId;
            }
        }
    } else{
        rc = -1;
    }

    return rc;
}

/* free the udevDevice structure */
void iap2FreeUdevDevice(U16 index, iap2UdevDevice_t* udevDevice)
{
    iap2TestFreePtr( (void**)&udevDevice->serialNum);
    iap2TestFreePtr( (void**)&udevDevice->productName);
    iap2TestFreePtr( (void**)&udevDevice->udevPath);
    udevDevice->productId = 0;
    udevDevice->vendorId = 0;
    iap2TestFreePtr( (void**)&g_UtilityDevices[index].serialNum);
    iap2TestFreePtr( (void**)&g_UtilityDevices[index].productName);
    iap2TestFreePtr( (void**)&g_UtilityDevices[index].udevPath);
    g_UtilityDevices[index].productId = 0;
    g_UtilityDevices[index].vendorId = 0;
}

/* free allocated memory in the global structure */
void iap2FreeDetectDevice(void)
{
    U16 i = 0;
    for(i=0; i<g_numDevices; i++)
    {
        iap2TestFreePtr( (void**)&g_UtilityDevices[i].serialNum);
        iap2TestFreePtr( (void**)&g_UtilityDevices[i].productName);
        iap2TestFreePtr( (void**)&g_UtilityDevices[i].udevPath);
        g_UtilityDevices[i].productId = 0;
        g_UtilityDevices[i].vendorId = 0;
    }
}

U32 iap2CurrTimeMs(void)
{
    U32 timeMs;
    struct timeval tp;
    gettimeofday (&tp, NULL);
    timeMs = iap2CurrTimeValToMs(&tp);

    return timeMs;
}

U32 iap2CurrTimeValToMs(struct timeval* currTime)
{
    return (U32)(currTime->tv_sec * 1000) + (U32)(currTime->tv_usec / 1000);
}

void iap2SleepMs(U32 sleep_ms)
{
    S32 s32ReturnValue;
    struct timespec req;
    struct timespec remain;

    /* Initialize the structure */
    memset(&req, 0, sizeof(req));
    memset(&remain, 0, sizeof(remain));


    req.tv_sec = sleep_ms / 1000;
    req.tv_nsec = (sleep_ms % 1000) * 1000000;

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
    }
}

S32 iap2VerifyThreadId(TEST_THREAD_ID threadID)
{
    if(threadID > 0)
    {
        /* thread ID valid */
        return 0;
    }
    return -1;
}

TEST_THREAD_ID iap2CreateThread(void* thread_addr, char* thread_name, void* exinf)
{
    S32 rc = -1;
    /* -----------  pthread variables  ----------- */
    pthread_t thread;
    TEST_THREAD_ID threadID = 0;
    pthread_attr_t attr;

    memset(&attr, 0, sizeof(pthread_attr_t));

    rc = pthread_attr_init(&attr);
    if(rc != 0)
        printf("attr_init failed with %d \n", rc);
    rc = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    if(rc != 0)
        printf("attr_setdetachstate failed with %d \n", rc);

    if (rc == 0)
    {
        rc = pthread_create(&thread, &attr, (void *(*)(void *))thread_addr, exinf);
        if(rc == 0)
        {
            threadID = (TEST_THREAD_ID)thread;
            /* set thread name */
            rc = pthread_setname_np(thread, (const char*)thread_name);
        }
        else
        {
            rc = -1;
        }
        usleep(100);

        (void)pthread_attr_destroy(&attr);
    }
    else
    {
        rc = -1;
    }

    if(rc != 0)
    {
        threadID = 0;
    }
    return threadID;
}

S32 iap2CreateMq(mqd_t* mq_fd, char* mq_name, int flag)
{
    S32 rc = -1;
    mqd_t fd = -1;
    struct mq_attr attr;

    /* The only flag that can appear in this field is O_NONBLOCK. */
    attr.mq_flags = 0;
    /* Upper limit on the number of messages that may be
     * placed on the queue using mq_send(). */
    attr.mq_maxmsg = 10;
    /* Ipper limit on the size of messages that may be
     * placed on the queue. */
    attr.mq_msgsize = TEST_MQ_MAX_SIZE;
    /* Returns the number of messages currently held in the queue. */
    attr.mq_curmsgs = 0;

    /* create the message queue */
    if((flag & O_CREAT) == O_CREAT)
    {
        /* create server mq */
        fd = mq_open(mq_name, flag, S_IRWXU, &attr);
    }
    else
    {
        /* create client mq */
        fd = mq_open(mq_name, flag);
    }
    if(fd != -1)
    {
        /* mq created */
        *mq_fd = fd;
        rc = 0;
    }
    else
    {
        rc = -1;
    }

    return rc;
}

S32 iap2SendMqRecvAck(mqd_t sndMqFd, S32 rcvMqFd, iap2ComThreadCmds_t mq_cmd, void* param, size_t size)
{
    S32 rc = 0;

    iap2ComThreadMq_t mq_st;
    char sndBuf[TEST_MQ_MAX_SIZE];
    char recvBuf[TEST_MQ_MAX_SIZE];

    mq_st.mq_cmd = mq_cmd;
    if( (param == NULL) && (size == 0))
    {
        /* only a mq command should be send to PollThread */
        mq_st.param = NULL;
        mq_st.param_size = 0;
    } else if(param != NULL){
        mq_st.param = param;
        mq_st.param_size = size;
    } else{
        rc = -1;
    }

    if(rc == 0){
        memcpy(&sndBuf[0], &mq_st, sizeof(mq_st));

        /* send mq command and parameters to PollThread */
        rc = iap2SendMq(sndMqFd, &sndBuf[0], sizeof(mq_st));

        /* receive ACK or Error from PollThread */
        rc = iap2RecvMqTmo(rcvMqFd, &recvBuf[0], TEST_MQ_MAX_SIZE, TEST_MQ_RECV_TMO);
        if(rc > 0){
            if((int)recvBuf[0] == MQ_CMD_ACK){
                rc = 0;
            } else{
                printf(" iap2SendMqRecvAck():  failed with cmd = %d\n", recvBuf[0]);
                rc = -1;
            }
        } else{
            printf(" iap2SendMqRecvAck():  failed with rc = %d\n", rc);
            rc = -1;
        }
    }

    return rc;
}

S32 iap2SendMq(mqd_t mqFD, char* sendBuf, size_t sendBufSize)
{
    S32 rc = -1;

    if(TEST_MQ_MAX_SIZE >= sendBufSize)
    {
        /* send message to pollThread. return value
         * of zero indicates success */
        rc = mq_send(mqFD, sendBuf, sendBufSize, 0);
        if(rc != 0)
        {
            printf(" error:  mq_send[%d] = %d \n", mqFD, rc);
        }
    }
    else
    {
        rc = -1;
    }

    return rc;
}

S32 iap2RecvMq(mqd_t mqFD, char* recvBuf, ssize_t recvBufSize)
{
    S32 rc = -1;
    char Buf[TEST_MQ_MAX_SIZE];
    ssize_t bytes_recv = 0;

    /* On success, mq_receive() return the number
     * of bytes in the received message */
    bytes_recv = mq_receive(mqFD, Buf, TEST_MQ_MAX_SIZE, NULL);
    if( (bytes_recv >= 0) && (bytes_recv < recvBufSize) )
    {
//        Buf[bytes_recv] = '\0';
        memcpy(recvBuf, &Buf[0], bytes_recv);
        rc = bytes_recv;
    }
    else
    {
        printf(" error:  mq_receive[%d] = %zd \n", mqFD, bytes_recv);
        rc = -1;
    }

    return rc;
}

S32 iap2RecvMqTmo(mqd_t mqFD, char* recvBuf, U32 recvBufSize, U32 timeout_ms)
{
    struct timespec ts;
    struct timeval tv;
    U32 prio = 60;
    S32 rc = -1;
    char Buf[TEST_MQ_MAX_SIZE];
    S32 bytes_recv = 0;

    memset(&tv, 0, sizeof(tv));
    memset(&ts, 0, sizeof(ts));

    gettimeofday (&tv, NULL);
    ts.tv_sec = (tv.tv_sec + (timeout_ms / 1000));

    /* On success, mq_timedreceive() return the number
     * of bytes in the received message */
    bytes_recv = mq_timedreceive(mqFD, Buf, TEST_MQ_MAX_SIZE, &prio, &ts);
    if( (bytes_recv >= 0) && ((U32)bytes_recv < recvBufSize) )
    {
        memcpy(recvBuf, &Buf[0], bytes_recv);
        rc = bytes_recv;
    }
    else
    {
        if (errno == ETIMEDOUT){
            printf(" %u ms  error: mq_timedreceive[%d] timed out = %d | errno: %d\n",
                    iap2CurrTimeMs(), mqFD, bytes_recv, errno);
        } else{
            printf(" %u ms  error: mq_timedreceive[%d] = %d | errno: %d \n",
                    iap2CurrTimeMs(), mqFD, bytes_recv, errno);
        }
        rc = -1;
    }

    return rc;
}

S32 iap2AllocateandUpdateData(void* dest_ptr, void* src_ptr, U16* dest_count, U16 src_count, iAP2_Type data_type)
{
    S32 rc = 0;

    switch(data_type)
    {
        case iAP2_int8:
        {
            S8** destptr = (S8**)dest_ptr;
            S8*  srcptr  = (S8*)src_ptr;

            *destptr = (S8*)calloc(src_count,sizeof(S8));
            if(*destptr == NULL)
            {
                rc = -1;
            }
            else
            {
                U16 count;

                for(count = 0; count < src_count; count++)
                {
                    memcpy(destptr[count], &srcptr[count], sizeof(S8));
                    (*dest_count) += 1;
                }
            }

            break;
        }

        case iAP2_uint8:
        {
            U8** destptr = (U8**)dest_ptr;
            U8*  srcptr  = (U8*)src_ptr;

            *destptr = (U8*)calloc(1,sizeof(U8));
            if(*destptr == NULL)
            {
                rc = -1;
            }
            else
            {
                U16 count;

                for(count = 0; count < src_count; count++)
                {
                    memcpy(destptr[count], &srcptr[count], sizeof(U8));
                    (*dest_count) += 1;
                }
            }
            break;
        }

        case iAP2_int16:
        {
            S16** destptr = (S16**)dest_ptr;
            S16*  srcptr  = (S16*)src_ptr;

            *destptr = (S16*)calloc(1,sizeof(S16));
            if(*destptr == NULL)
            {
                rc = -1;
            }
            else
            {
                U16 count;

                for(count = 0; count < src_count; count++)
                {
                    memcpy(destptr[count], &srcptr[count], sizeof(S16));
                    (*dest_count) += 1;
                }
            }
            break;
        }

        case iAP2_uint16:
        {
            U16** destptr = (U16**)dest_ptr;
            U16*  srcptr  = (U16*)src_ptr;

            *destptr = (U16*)calloc(1,sizeof(U16));
            if(*destptr == NULL)
            {
                rc = -1;
            }
            else
            {
                U16 count;

                for(count = 0; count < src_count; count++)
                {
                    memcpy(destptr[count], &srcptr[count], sizeof(U16));
                    (*dest_count) += 1;
                }
            }
            break;
        }

        case iAP2_int32:
        {
            S32** destptr = (S32**)dest_ptr;
            S32*  srcptr  = (S32*)src_ptr;

            *destptr = (S32*)calloc(1,sizeof(S32));
            if(*destptr == NULL)
            {
                rc = -1;
            }
            else
            {
                U16 count;

                for(count = 0; count < src_count; count++)
                {
                    memcpy(destptr[count], &srcptr[count], sizeof(S32));
                    (*dest_count) += 1;
                }
            }
            break;
        }

        case iAP2_uint32:
        {
            U32** destptr = (U32**)dest_ptr;
            U32*  srcptr  = (U32*)src_ptr;

            *destptr = (U32*)calloc(1,sizeof(U32));
            if(*destptr == NULL)
            {
                rc = -1;
            }
            else
            {
                U16 count;

                for(count = 0; count < src_count; count++)
                {
                    memcpy(destptr[count], &srcptr[count], sizeof(U32));
                    (*dest_count) += 1;
                }
            }
            break;
        }

        case iAP2_int64:
        {
            S64** destptr = (S64**)dest_ptr;
            S64*  srcptr  = (S64*)src_ptr;

            *destptr = (S64*)calloc(1,sizeof(S64));
            if(*destptr == NULL)
            {
                rc = -1;
            }
            else
            {
                U16 count;

                for(count = 0; count < src_count; count++)
                {
                    memcpy(destptr[count], &srcptr[count], sizeof(S64));
                    (*dest_count) += 1;
                }
            }
            break;
        }

        case iAP2_uint64:
        {
            U64** destptr = (U64**)dest_ptr;
            U64*  srcptr  = (U64*)src_ptr;

            *destptr = (U64*)calloc(1,sizeof(U64));
            if(*destptr == NULL)
            {
                rc = -1;
            }
            else
            {
                U16 count;

                for(count = 0; count < src_count; count++)
                {
                    memcpy(destptr[count], &srcptr[count], sizeof(U64));
                    (*dest_count) += 1;
                }
            }
            break;
        }

        case iAP2_blob:
        {
            iAP2Blob** dest_blob = (iAP2Blob**)dest_ptr;

            *dest_blob = (iAP2Blob*)calloc(1, sizeof(iAP2Blob));
            if(*dest_blob == NULL)
            {
                rc = -1;
            }
            if(rc == 0)
            {
                (*dest_blob)->iAP2BlobData = (U8*)calloc(src_count, sizeof(U8));
                if((*dest_blob)->iAP2BlobData == NULL)
                {
                    rc = -1;
                }
            }
            if(rc == 0)
            {
                memcpy((*dest_blob)->iAP2BlobData, src_ptr, src_count);
                (*dest_blob)->iAP2BlobLength = src_count;
                *dest_count = 1;
            }
            break;
        }

        case iAP2_utf8:
        {
            U8*** destptr = (U8***)dest_ptr;
            U8**  srcptr  = (U8**)src_ptr;

            *destptr = (U8**)calloc(src_count, sizeof(U8*));
            if(*destptr == NULL)
            {
                rc = -1;
            }
            if(rc == 0)
            {
                U32 StringLength;

                for(*dest_count = 0; ( (*dest_count < src_count) && (rc == 0) ); (*dest_count)++)
                {
                    StringLength = strnlen( (const char*)srcptr[*dest_count], MAX_STRING_LEN) + 1;
                    (*destptr)[*dest_count] = (U8*)calloc(StringLength, sizeof(U8));
                    if((*destptr)[*dest_count] == NULL)
                    {
                        rc = -1;
                    }
                    else
                    {
                        memcpy((*destptr)[*dest_count], srcptr[*dest_count], StringLength);
                        ((*destptr)[*dest_count])[StringLength - 1] = '\0';
                    }
                }
            }
            break;
        }

        case iAP2_bool:
        case iAP2_enum:
        case iAP2_group:
        case iAP2_none:
        default:
        {
            rc = -1;
            break;
        }
    }

    return rc;
}


LOCAL S32 iap2DevPowerRequest(U16 vid, U16 pid)
{
    S32 ret = -1;

    libusb_device_handle *device_handle;
    libusb_context *usb_context = NULL;

    unsigned char *buffer   = NULL;
    uint16_t wLength  = 0;

    /* Initialize libusb */
    ret = libusb_init(&usb_context);
    if((0 > ret) || (usb_context == NULL))
    {
        printf("iap2DevPowerRequest():  libusb_init() failed = %d\n", ret);
        return -1;
    }

    /* Open libusb with vendor id and product id */
    device_handle = libusb_open_device_with_vid_pid(usb_context, vid, pid);
    if(device_handle != NULL)
    {
        /* Send control transfer for power request */
        ret = libusb_control_transfer(device_handle,
                                      DEV_POWER_REQ_VEN_bmREQ,
                                      DEV_POWER_REQ_VEN_REQ,
                                      DEV_POWER_REQ_VEN_VAL,
                                      DEV_POWER_REQ_VEN_IX,
                                      buffer,
                                      wLength,
                                      1000);
        /* Transfer success */
        if(ret == wLength)
        {
            ret = 0;
        }
        else
        {
            printf("iap2DevPowerRequest():  libusb_control_transfer failed = %d\n", ret);
            ret = -1;
        }
        /* Close device */
        libusb_close(device_handle);
    }
    else
    {
        printf("iap2DevPowerRequest():  libusb_open_device_with_vid_pid(%u, %u) failed\n", vid, pid);
        ret = -1;
    }
    libusb_exit(usb_context);

    return ret;
}


S32 iap2CheckForDevice(struct udev *udev)
{
    S32 ret = -1;

    struct udev_enumerate *enumerate;
    struct udev_list_entry *devices, *dev_list_entry;
    struct udev_device *dev;

    const char *idVendor = NULL;
    const char *idProduct = NULL;
    const char *serial = NULL;
    const char* product = NULL;

    S32 len = 0;

    if(udev != NULL)
    {
        /* Create a list of the devices in the 'hidraw' subsystem. */
        enumerate = udev_enumerate_new(udev);
        if(enumerate == NULL)
        {
            IAP2TESTDLTLOG(DLT_LOG_ERROR, "enumerate is NULL");
            exit(1);
        }
        ret = udev_enumerate_add_match_subsystem(enumerate, (const char *)IPOD_USB_MONITOR_DEVTYPE);
        if(ret != 0)
        {
            IAP2TESTDLTLOG(DLT_LOG_ERROR, "enumerate_add_match_subsystem failed: ret = %d", ret);
            exit(1);
        }
        /* search only for Apple devices */
        ret = udev_enumerate_add_match_sysattr(enumerate, IPOD_SYSATTR_IDVENDOR, IPOD_APPLE_IDVENDOR);
        if(ret != 0)
        {
            IAP2TESTDLTLOG(DLT_LOG_ERROR, "enumerate_add_match_sysattr failed: ret = %d", ret);
            exit(1);
        }
        ret = udev_enumerate_scan_devices(enumerate);
        if(ret != 0)
        {
            IAP2TESTDLTLOG(DLT_LOG_ERROR, "enumerate_scan_devices failed: ret = %d", ret);
            exit(1);
        }
        devices = udev_enumerate_get_list_entry(enumerate);
        if(devices == NULL)
        {
            IAP2TESTDLTLOG(DLT_LOG_WARN, "devices is NULL (list_entry)");
            /* no specified Apple device available */
            ret = -1;
        }
        else
        {
            /* For each item enumerated, print out its information.
               udev_list_entry_foreach is a macro which expands to
               a loop. The loop will be executed for each member in
               devices, setting dev_list_entry to a list entry
               which contains the device's path in /sys. */
            udev_list_entry_foreach(dev_list_entry, devices)
            {
                const char *path;

                /* Get the filename of the /sys entry for the device
                   and create a udev_device object (dev) representing it */
                path = udev_list_entry_get_name(dev_list_entry);
                if(path == NULL)
                {
                    /* Get the path of device failed*/
                    IAP2TESTDLTLOG(DLT_LOG_ERROR, "path of device failed");
                }
                else
                {
                    dev = udev_device_new_from_syspath(udev, path);
                    if(dev != NULL)
                    {
                        /* usb_device_get_devnode() returns the path to the device node
                           itself in /dev. */
//                         printf("Device Node Path: %s\n", udev_device_get_devnode(dev));

                        /* From here, we can call get_sysattr_value() for each file
                           in the device's /sys entry. The strings passed into these
                           functions (idProduct, idVendor, serial, etc.) correspond
                           directly to the files in the directory which represents
                           the USB device. Note that USB strings are Unicode, UCS2
                           encoded, but the strings returned from
                           udev_device_get_sysattr_value() are UTF-8 encoded. */
                        idVendor = udev_device_get_sysattr_value(dev,"idVendor");
                        idProduct = udev_device_get_sysattr_value(dev, "idProduct");
                        serial = udev_device_get_sysattr_value(dev, "serial");
                        product = udev_device_get_sysattr_value(dev, "product");

//                        printf("VID: %s  PID: %s  SERIAL: %s \n", idVendor, idProduct, serial);

                        if( (idVendor != NULL) && (idProduct != NULL) && (serial != NULL) && (product != NULL) )
                        {
                            /* Vendor ID equal to Apple Vendor ID and Product ID equal to Apple Product ID */
                            if((strncmp((const char *)idVendor, (const char *)IPOD_APPLE_IDVENDOR, DEV_DETECT_VENDOR_MAX_LEN) == 0) &&
                               (strncmp((const char *)idProduct, (const char *)IPOD_APPLE_IDPRODUCT_MIN, 2) == 0))
                            {
                                printf(" %s found %s with serial-nr: %s\n", __FUNCTION__, product, serial);

                                /* copy udev path */
                                len = (S32)strnlen(path, DEV_DETECT_CFG_STRING_MAX);
                                g_UtilityDevices[g_numDevices].udevPath = (U8*)strndup(path, len);
                                /* copy serial number */
                                len = (S32)strnlen(serial, DEV_DETECT_CFG_STRING_MAX);
                                g_UtilityDevices[g_numDevices].serialNum = (U8*)strndup(serial, len);
                                /* copy product name */
                                len = (S32)strnlen(product, DEV_DETECT_CFG_STRING_MAX);
                                g_UtilityDevices[g_numDevices].productName = (U8*)strndup(product, len);

                                if((g_UtilityDevices[g_numDevices].udevPath == NULL) ||
                                   (g_UtilityDevices[g_numDevices].serialNum == NULL) ||
                                   (g_UtilityDevices[g_numDevices].productName == NULL) )
                                {
                                    ret = -1;
                                    IAP2TESTDLTLOG(DLT_LOG_ERROR,
                                                   "Failed to set device details    \
                                                   udevPath = %p                    \
                                                   serialNum = %p,                  \
                                                   productName = %p,",
                                                   g_UtilityDevices[g_numDevices].udevPath,
                                                   g_UtilityDevices[g_numDevices].serialNum,
                                                   g_UtilityDevices[g_numDevices].productName);
                                }

                                if(ret == 0)
                                {
                                    g_UtilityDevices[g_numDevices].vendorId = strtol((const char*)idVendor, NULL, 16);
                                    g_UtilityDevices[g_numDevices].productId = strtol((const char*)idProduct, NULL, 16);
                                }

                                /* return serial number of the first detected Apple device */
                                if(ret == 0)
                                {
                                    IAP2TESTDLTLOG(DLT_LOG_INFO, "Device details: VID: %s  PID: %s  SERIAL: %s", idVendor, idProduct, serial);
                                }

                                /* send PowerRequest to Apple device */
                                if(0 != iap2DevPowerRequest(g_UtilityDevices[g_numDevices].vendorId,
                                                            g_UtilityDevices[g_numDevices].productId)){
                                    printf("iap2CheckForDevice():  iap2DevPowerRequest failed!\n");
                                }

                                if(ret == 0)
                                {
                                    g_numDevices++;
                                }
                            }
                        }
                        else
                        {
                            IAP2TESTDLTLOG(DLT_LOG_ERROR, "device_get_sysattr_value(idVendor or idProduct or serial) is NULL");
                        }

                        udev_device_unref(dev);
                    }
                    else
                    {
                        IAP2TESTDLTLOG(DLT_LOG_ERROR, "device_new_from_syspath failed");
                    }
                }
            }
        }
        /* Free the enumerator object */
        udev_enumerate_unref(enumerate);
    }

    return ret;
}

/* return the number of detected Apple devices */
S32 iap2DetectDevice(void)
{
    S32 ret = -1;
    int retryCount = 0;

    struct udev *udev;
    struct udev_device *dev;
    struct udev_monitor *monitor;
    S32 udevfd = -1;
    const char *action = NULL;

    U16 i = 0;
    g_numDevices = 0;

    for(i=0; i<MAX_DEVICES; i++)
    {
        if(g_UtilityDevices[i].udevPath != NULL){
            free(g_UtilityDevices[i].udevPath);
        }
        if(g_UtilityDevices[i].serialNum != NULL){
            free(g_UtilityDevices[i].serialNum);
        }
        if(g_UtilityDevices[i].productName != NULL){
            free(g_UtilityDevices[i].productName);
        }
        g_UtilityDevices[i].vendorId = 0;
        g_UtilityDevices[i].productId = 0;
    }

    /* Create the udev object */
    udev = udev_new();
    if (!udev) {
        IAP2TESTDLTLOG(DLT_LOG_ERROR, "Can't create udev");
        exit(1);
    }
    else
    {
        /* check if iPod is already connected */
        ret = iap2CheckForDevice(udev);
        /* no iPod available. wait for connecting iPod */
        if(ret != 0)
        {
            /* Link the monitor to "udev" devices */
            monitor = udev_monitor_new_from_netlink(udev, (const char *)IPOD_USB_MONITOR_LINK);
            if(monitor != NULL)
            {
                /* Filter only for get the "hiddev" devices */
                ret = udev_monitor_filter_add_match_subsystem_devtype(monitor, (const char *)IPOD_USB_MONITOR_DEVTYPE, IPOD_USB_FILTER_TYPE);
                if(ret == 0)
                {
                    /* Start receiving */
                    ret = udev_monitor_enable_receiving(monitor);
                    if(ret == 0)
                    {
                        /* Get the file descriptor for the monitor. This fd is used by select() */
                        udevfd = udev_monitor_get_fd(monitor);
                        ret = -1;
                        IAP2TESTDLTLOG(DLT_LOG_WARN, "Please connect iPod!");
                        while((ret == -1) && (retryCount < IPOD_USB_SELECT_RETRY_CNT)
                              && (iap2GetUtilityQuit() != TRUE))
                        {
                            retryCount++;

                            fd_set fds;
                            struct timeval tv;

                            FD_ZERO(&fds);
                            FD_SET(udevfd, &fds);
                            tv.tv_sec = 5;
                            tv.tv_usec = 0;

                            ret = select(udevfd+1, &fds, NULL, NULL, &tv);
                            /* Check if our file descriptor has received data. */
                            if (ret > 0 && FD_ISSET(udevfd, &fds))
                            {
                                /* Make the call to receive the device.
                                   select() ensured that this will not block. */
                                dev = udev_monitor_receive_device(monitor);
                                if (dev)
                                {
//                                    printf("Got Device\n");
//                                    printf("   Node: %s\n", udev_device_get_devnode(dev));
//                                    printf("   Subsystem: %s\n", udev_device_get_subsystem(dev));
//                                    printf("   Devtype: %s\n", udev_device_get_devtype(dev));
                                    action = udev_device_get_action(dev);
                                    if(strncmp((const char *)action, IPOD_USB_ACTION_ADD, 4) == 0)
                                    {
                                        ret = 0;
                                        IAP2TESTDLTLOG(DLT_LOG_DEBUG, "Action is %s -> ret = %d", action, ret);
                                    }
                                    else
                                    {
                                        ret = -1;
                                        IAP2TESTDLTLOG(DLT_LOG_ERROR, "Action is %s -> ret = %d", action, ret);
                                    }

                                    udev_device_unref(dev);
                                }
                                else
                                {
                                    IAP2TESTDLTLOG(DLT_LOG_ERROR, "Error: No iPod ");
                                }
                            }
                            else if(ret < 0)
                            {
                                IAP2TESTDLTLOG(DLT_LOG_ERROR, "Error: select() failed");
                            }
                            else
                            {
                                /* this occurs if an iPod is already connected */
                                IAP2TESTDLTLOG(DLT_LOG_ERROR, "Error: No iPod found");
                                ret = -1;
                            }
                            IAP2TESTDLTLOG(DLT_LOG_WARN, "retry %d", retryCount);
                        }

                        if(ret == -1)
                        {
                            IAP2TESTDLTLOG(DLT_LOG_ERROR, "Error: max retries %d | ret = %d",  retryCount, ret);
                        }
                        else /* IPOD_OK */
                        {
                            /* get Serial-Nr. VendorID and ProductID of newly connected iPod */
                            ret = iap2CheckForDevice(udev);
                            /* no iPod available. wait for connecting iPod */
                            if(ret != 0)
                            {
                                IAP2TESTDLTLOG(DLT_LOG_ERROR, "Error: enumerate connected iPod failed | ret = %d", ret);
                            }
                        }
                    }
                }
                udev_monitor_unref(monitor);
            }
            else
            {
                IAP2TESTDLTLOG(DLT_LOG_ERROR, "monitor NULL");
                exit(1);
            }
        }
    }

    udev_unref(udev);

    /* if everything is fine,
     * return the number of detected Apple devices */
    if(ret == 0)
    {
        ret = g_numDevices;
    }

    return ret;
}

