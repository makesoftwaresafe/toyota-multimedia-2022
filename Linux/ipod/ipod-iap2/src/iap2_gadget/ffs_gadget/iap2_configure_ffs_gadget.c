/*****************************************************************************
-  \file : iap2_configure_ffs_gadget.c
-  \version : $Id: iap2_configure_ffs_gadget.c, v Exp $
-  \release : $Name:$
-  Contains the source code implementation for Configuring ffs gadget
-  \component :
-  \author : Manavalan Veeramani/RBEI/ manavalan.veeramani@in.bosch.com
-  \copyright (c) 2010 - 2016 Advanced Driver Information Technology.
-  This code is developed by Advanced Driver Information Technology.
-  Copyright of Advanced Driver Information Technology, Bosch, and DENSO.
-  All rights reserved.
*****************************************************************************/

#include <linux/usb/functionfs.h>
#include <pthread_adit.h>
#include <poll.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>

#include "iap2_configure_ffs_gadget.h"

#include "iap2_dlt_log.h"

#define STRING_MAX                          256
#define IPOD_IAP2_USBHOST_MAX_SEND_BYTES    65535
#define IPOD_IAP2_USBHOST_POLL_TIMEOUT      2000

#define STR_INTERFACE_ "iAP Interface"

/******************** Little Endian Handling ********************************/

#define cpu_to_le16(x)  htole16(x)
#define cpu_to_le32(x)  htole32(x)

/******************** Descriptors and Strings *******************************/

typedef struct {
    struct usb_interface_descriptor         intf;
    struct usb_endpoint_descriptor_no_audio sink;
    struct usb_endpoint_descriptor_no_audio source;
} __attribute__( (packed) ) basic_usb_descriptor_set_t;

typedef struct {
    struct usb_interface_descriptor         inactiveIntf;
    struct usb_interface_descriptor         activeIntf;
    struct usb_endpoint_descriptor_no_audio sink;
    struct usb_endpoint_descriptor_no_audio source;
} __attribute__( (packed) ) extended_usb_descriptor_set_t;

/*
 * PRQA: Lint Message 754: parent structure size and address are used in write()
 *       call in  ep0_init() function.
 */
/*lint -esym(754,header,code,str1,str2,lang0)*/

LOCAL inline void iAP2FreePointer(void** input_ptr)
{
    if(*input_ptr != NULL)
    {
        free(*input_ptr);
        *input_ptr = NULL;
    }
}

LOCAL inline void iAP2CloseFd(S32* fd)
{
    if(*fd >= 0)
    {
        close(*fd);
        *fd = -1;
    }
}

void iAP2USBHost_PrintLibUsbgResult(const usbg_error rc, const char* fn_name)
{
    if(rc != USBG_SUCCESS)
    {
        IAP2FFSGADGETDLTLOG(DLT_LOG_ERROR, "%s() returned Error: %s : %s", fn_name, usbg_error_name(rc), usbg_strerror(rc) );
    }
    else
    {
        IAP2FFSGADGETDLTLOG(DLT_LOG_DEBUG, "%s() returned rc = %d", fn_name, rc );
    }
}

LOCAL S32 ep0_consumeEvent(iAP2_ffs_config_t* iAP2_ffs_config, struct usb_functionfs_event *event, BOOL EnuemerationDone)
{
    S32 rc = IAP2_OK;
    char *const ffs_event_names[] = {
        [FUNCTIONFS_BIND]       = "BIND",
        [FUNCTIONFS_UNBIND]     = "UNBIND",
        [FUNCTIONFS_ENABLE]     = "ENABLE",
        [FUNCTIONFS_DISABLE]    = "DISABLE",
        [FUNCTIONFS_SETUP]      = "SETUP",
        [FUNCTIONFS_SUSPEND]    = "SUSPEND",
        [FUNCTIONFS_RESUME]     = "RESUME",
    };

    switch (event->type)
    {
        case FUNCTIONFS_BIND:
        {
            if(EnuemerationDone == FALSE)
            {
                iAP2_ffs_config->g_ffs_status |= G_FFS_BIND;
            }
            break;
        }
        case FUNCTIONFS_UNBIND:
        case FUNCTIONFS_SETUP:
        case FUNCTIONFS_RESUME:
        {
            break;
        }
        case FUNCTIONFS_ENABLE:
        {
            if(EnuemerationDone == FALSE)
            {
                iAP2_ffs_config->g_ffs_status |= G_FFS_ENABLED;
            }
            break;
        }
        case FUNCTIONFS_SUSPEND:
        case FUNCTIONFS_DISABLE:
        {
            rc = IAP2_DEV_NOT_CONNECTED;
            break;
        }
        default:
        {
            IAP2FFSGADGETDLTLOG(DLT_LOG_ERROR, "ERROR: Unknown event type = %d", event->type);
            rc = IAP2_CTL_ERROR;
            break;
        }
    }
    if(rc != IAP2_CTL_ERROR)
    {
        IAP2FFSGADGETDLTLOG(DLT_LOG_INFO, "event %s consumed", ffs_event_names[event->type]);
    }

    return rc;
}

/**
 * This thread waits for the bind and enable event of the gadget ffs.
 *
 * \param IPOD_IAP2_HID_HOSTDEV_INFO *devinfo - device info structure
 */
LOCAL void *gffsReady_thread(void *arg)
{
    iAP2_ffs_config_t* iAP2_ffs_config = (iAP2_ffs_config_t*)arg;
    S32 rc = IAP2_OK;
    S32 data_read = 0;
    U32 buffer_size = 0;
    U8* readBuf = NULL;

    /* indicate that g_ffs is not ready */
    iAP2_ffs_config->g_ffs_status = G_FFS_DEFAULT;

    buffer_size = IPOD_IAP2_USBHOST_MAX_SEND_BYTES;
    /* Allocate buffer for message queue. */
    readBuf = calloc(buffer_size, sizeof(U8));
    if (NULL != readBuf)
    {
        /* Wait for BIND and ENABLE or ENABLE (SWGIII-7978 FunctionFs events are filtered by kernel,
         * an ENABLE event deletes a pending BIND event if not already read by user via ep0)
         * But receive other events like SUSPEND to react if necessary. */
        while ( (iAP2_ffs_config->g_ffs_status != G_FFS_READY)   &&
                (iAP2_ffs_config->g_ffs_status != G_FFS_ENABLED) &&
                (rc == IAP2_OK) )
        {
            struct pollfd fds;

            fds.fd     = iAP2_ffs_config->ep0_fd;
            fds.events = POLLIN;

            /* First, wait for BIND event.
             * Afterwards, wait for ENABLE and use the remained time. */
            rc = poll(&fds, 1, IPOD_IAP2_USBHOST_POLL_TIMEOUT);
            IAP2FFSGADGETDLTLOG(DLT_LOG_DEBUG, "poll() returns: %d", rc);
            if(rc > 0)
            {
                data_read = read(iAP2_ffs_config->ep0_fd, (char*)readBuf, buffer_size);
                if(data_read <= 0)
                {
                    if (errno == EAGAIN)
                    {
                        /* Andreas: if read returns EAGAIN, enter poll / select again */
                        rc = IAP2_OK;
                    }
                    else
                    {
                        rc = IAP2_CTL_ERROR;
                    }
                    IAP2FFSGADGETDLTLOG(DLT_LOG_ERROR, "read() returns rc = %d | errno: %d (%s)| data_read = %d", rc, errno, strerror(errno), data_read);
                }
                else
                {
                    struct usb_functionfs_event *event = (struct usb_functionfs_event *)readBuf;

                    rc = IAP2_OK;
                    IAP2FFSGADGETDLTLOG(DLT_LOG_DEBUG, "data_read = %d, readBuf = %p", data_read, readBuf);
                    while( (data_read >= (S32) sizeof(struct usb_functionfs_event) ) &&
                           (rc == IAP2_OK) )
                    {
                        rc = ep0_consumeEvent(iAP2_ffs_config, event, FALSE);
                        data_read -= sizeof(struct usb_functionfs_event);
                        if (data_read > 0)
                        {
                            /* If there are more events to be read move to next event in readBuf */
                            event += 1;
                        }
                    }
                    if ( (data_read < (S32) sizeof(struct usb_functionfs_event) ) && (data_read != 0) )
                    {
                        IAP2FFSGADGETDLTLOG(DLT_LOG_ERROR, "length is less than the sizeof the event structure len=%d", data_read);
                        data_read = 0;
                    }
                }
            }
            else if(rc == 0)
            {
                IAP2FFSGADGETDLTLOG(DLT_LOG_ERROR, "poll() timedout rc = %d | errno = %d (%s)", rc, errno, strerror(errno));
                rc = IAP2_CTL_ERROR;
            }
            else
            {
                IAP2FFSGADGETDLTLOG(DLT_LOG_ERROR, "poll() rc = %d | errno = %d (%s)", rc, errno, strerror(errno));
                rc = IAP2_CTL_ERROR;
            }
        }
        iAP2FreePointer( (void**)&readBuf);

        /* g_ffs is ready if BIND and ENABLE are received or if ENABLE event is received.
         * (SWGIII-7978 FunctionFs events are filtered by kernel, an ENABLE event deletes
         * a pending BIND event if not already read by user via ep0) */
        if ( (iAP2_ffs_config->g_ffs_status == G_FFS_READY) || (iAP2_ffs_config->g_ffs_status == G_FFS_ENABLED) )
        {
            rc = IAP2_OK;
        }
    }
    else
    {
        rc = IAP2_ERR_NO_MEM;
        IAP2FFSGADGETDLTLOG(DLT_LOG_ERROR, "Allocate readBuf failed.");
    }

    IAP2FFSGADGETDLTLOG(DLT_LOG_DEBUG, "gffsReady_thread() exit\n");
    pthread_exit( (void*)arg);

    return NULL;
}

LOCAL void setExtendedUSBDescriptorSet(extended_usb_descriptor_set_t* extendedDescriptorSet, U16 i, BOOL HiSpeed)
{
    extendedDescriptorSet->inactiveIntf.bLength             = sizeof extendedDescriptorSet->inactiveIntf;
    extendedDescriptorSet->inactiveIntf.bDescriptorType     = USB_DT_INTERFACE;
    extendedDescriptorSet->inactiveIntf.bInterfaceNumber    = i+1; //interface number is 0x01, 0x02, ... (starts at 0x01 because 0x00 is reserved for "iAP2 Interface")
    extendedDescriptorSet->inactiveIntf.bAlternateSetting   = 0;  //default alternative setting is 0
    extendedDescriptorSet->inactiveIntf.bNumEndpoints       = 0;
    extendedDescriptorSet->inactiveIntf.bInterfaceClass     = USB_CLASS_VENDOR_SPEC;
    extendedDescriptorSet->inactiveIntf.bInterfaceSubClass  = 0xf0;
    extendedDescriptorSet->inactiveIntf.bInterfaceProtocol  = 0x01;
    extendedDescriptorSet->inactiveIntf.iInterface          = i+2; //interfaces 2, 3, ... (start with next iInterface, compare basicDescriptorSet)

    extendedDescriptorSet->activeIntf.bLength               = sizeof extendedDescriptorSet->activeIntf;
    extendedDescriptorSet->activeIntf.bDescriptorType       = USB_DT_INTERFACE;
    extendedDescriptorSet->activeIntf.bInterfaceNumber      = i+1; //same number as 'inactiveIntf'
    extendedDescriptorSet->activeIntf.bAlternateSetting     = 1;  //alternative setting must be 1
    extendedDescriptorSet->activeIntf.bNumEndpoints         = 2;
    extendedDescriptorSet->activeIntf.bInterfaceClass       = USB_CLASS_VENDOR_SPEC;
    extendedDescriptorSet->activeIntf.bInterfaceSubClass    = 0xf0;
    extendedDescriptorSet->activeIntf.bInterfaceProtocol    = 0x01;
    extendedDescriptorSet->activeIntf.iInterface            = i+2; //same index as 'inactiveIntf'

    extendedDescriptorSet->sink.bLength                     = sizeof extendedDescriptorSet->sink;
    extendedDescriptorSet->sink.bDescriptorType             = USB_DT_ENDPOINT;
    extendedDescriptorSet->sink.bEndpointAddress            = ( ((i+2)*2)-1) | USB_DIR_IN; //start with address 0x83 and then use every second (0x83, 0x85, 0x87, ...)
    extendedDescriptorSet->sink.bmAttributes                = USB_ENDPOINT_XFER_BULK;
    if(HiSpeed == TRUE)
    {
        extendedDescriptorSet->sink.wMaxPacketSize      = cpu_to_le16(512);
    }
    else
    {
        /* extendedDescriptorSet->sink.wMaxPacketSize = autoconfiguration (kernel) */
    }

    extendedDescriptorSet->source.bLength                   = sizeof extendedDescriptorSet->source;
    extendedDescriptorSet->source.bDescriptorType           = USB_DT_ENDPOINT;
    extendedDescriptorSet->source.bEndpointAddress          = ( (i+2)*2) | USB_DIR_OUT; //start with address 0x04 and then use every second (0x04, 0x06, 0x08, ...)
    extendedDescriptorSet->source.bmAttributes              = USB_ENDPOINT_XFER_BULK;
    if(HiSpeed == TRUE)
    {
        extendedDescriptorSet->source.wMaxPacketSize    = cpu_to_le16(512);
        extendedDescriptorSet->source.bInterval         = 1; /* NAK every 1 uframe */
    }
    else
    {
        /* extendedDescriptorSet->source.wMaxPacketSize = autoconfiguration (kernel) */
    }
}

LOCAL void setBasicUSBDescriptorSet(basic_usb_descriptor_set_t* basicDescriptorSet, BOOL HiSpeed)
{
    basicDescriptorSet->intf.bLength            = sizeof basicDescriptorSet->intf;
    basicDescriptorSet->intf.bDescriptorType    = USB_DT_INTERFACE;
    basicDescriptorSet->intf.bInterfaceNumber   = 0x00; //"iAP2 Interface" is always number 0 (see spec)
    basicDescriptorSet->intf.bAlternateSetting  = 0;
    basicDescriptorSet->intf.bNumEndpoints      = 2;
    basicDescriptorSet->intf.bInterfaceClass    = USB_CLASS_VENDOR_SPEC;
    basicDescriptorSet->intf.bInterfaceSubClass = 0xf0;
    basicDescriptorSet->intf.bInterfaceProtocol = 0x00;
    basicDescriptorSet->intf.iInterface         = 1;

    basicDescriptorSet->sink.bLength            = sizeof basicDescriptorSet->sink;
    basicDescriptorSet->sink.bDescriptorType    = USB_DT_ENDPOINT;
    basicDescriptorSet->sink.bEndpointAddress   = 1 | USB_DIR_IN;
    basicDescriptorSet->sink.bmAttributes       = USB_ENDPOINT_XFER_BULK;
    if(HiSpeed == TRUE)
    {
        basicDescriptorSet->sink.wMaxPacketSize     = cpu_to_le16(512);
    }
    else
    {
        /* basicDescriptorSet->sink.wMaxPacketSize = autoconfiguration (kernel); */
    }

    basicDescriptorSet->source.bLength          = sizeof basicDescriptorSet->source;
    basicDescriptorSet->source.bDescriptorType  = USB_DT_ENDPOINT;
    basicDescriptorSet->source.bEndpointAddress = 2 | USB_DIR_OUT;
    basicDescriptorSet->source.bmAttributes     = USB_ENDPOINT_XFER_BULK;
    if(HiSpeed == TRUE)
    {
        basicDescriptorSet->source.wMaxPacketSize   = cpu_to_le16(512);
        basicDescriptorSet->source.bInterval        = 1;
    }
    else
    {
        /* basicDescriptorSet->source.wMaxPacketSize = autoconfiguration (kernel); */
    }
}

LOCAL S32 iAP2USBHostConfigDescriptorandStrings(iAP2_ffs_config_t* iAP2_ffs_config, BOOL UseConfigFS, usbg_gadget *iAP2_gadget, usbg_udc *iAP2_usbg_udc)
{
    S32 rc = IAP2_OK;
    U16 i = 0;
    U32 offset = 0;
    S32 strLen = 0;
    int count;
    int total_string_length = 0;

    total_string_length = (S32)strnlen(STR_INTERFACE_, (STRING_MAX - 1) ) + 1;
    if(iAP2_ffs_config->nativeTransport == FALSE)
    {
        count = 1;
    }
    else
    {
        count = iAP2_ffs_config->iOSAppCnt + 1;

        /* Identify the total Length of iOS App Names */
        for (i = 0; ( (i < (count-1) ) && (rc == IAP2_OK) ); i++)
        {
            if(iAP2_ffs_config->iOSAppNames[i] != NULL)
            {
                total_string_length += ( (S32)strnlen((const char*)iAP2_ffs_config->iOSAppNames[i], (STRING_MAX - 1) ) + 1);
                IAP2FFSGADGETDLTLOG(DLT_LOG_DEBUG, "iOSAppNames[%d] = %s", i, iAP2_ffs_config->iOSAppNames[i]);
            }
            else
            {
                rc = IAP2_INVALID_INPUT_PARAMETER;
                IAP2FFSGADGETDLTLOG(DLT_LOG_ERROR, "iOSAppCnt = %d, but iOSAppNames[%d] is NULL", iAP2_ffs_config->iOSAppCnt, i);
            }
        }
    }

    /* Definition of USB descriptor:
     * 'count' is the total number of applications which should be supported via EA native
     * transport mode plus one for the default interface "iAP2 Interface" required */
    struct
    {
        struct usb_functionfs_descs_head header;
        struct
        {
            basic_usb_descriptor_set_t basicDescriptorSet;
            extended_usb_descriptor_set_t extendedDescriptorSet[count - 1];
        }__attribute__( (packed) ) fs_descs, hs_descs;
    }__attribute__( (packed) ) descriptors;

    /* Definition of string descriptor */
    struct
    {
        struct usb_functionfs_strings_head header;
        struct
        {
            __le16 code;
            char str1[total_string_length];
        }__attribute__( (packed) ) lang0;
    }__attribute__( (packed) ) strings;

    /* Set USB descriptors */
    memset(&descriptors, 0, sizeof(descriptors) );

    /* - Header */
    descriptors.header.magic  = cpu_to_le32(FUNCTIONFS_DESCRIPTORS_MAGIC);
    descriptors.header.length = cpu_to_le32(sizeof descriptors);

    /* We have always one basic USB descriptor set with 3 descriptors for the default interface
     * "iAP Interface" and additional extended USB descriptor sets for EA native transport with 4
     * descriptors (alt0, alt1 + 2 eps) */
    descriptors.header.fs_count = 3 + ( (count - 1) * 4);
    descriptors.header.hs_count = 3 + ( (count - 1) * 4);

    /* - Basic Interface Set */
    /*   - Full-speed */
    setBasicUSBDescriptorSet(&descriptors.fs_descs.basicDescriptorSet, FALSE);

    /*   - High-speed */
    setBasicUSBDescriptorSet(&descriptors.hs_descs.basicDescriptorSet, TRUE);

    /* - Extended Interface Sets (for apps with support for EA native transport) */
    for (i = 0; i < count - 1; i++)
    {
        /*   - Full speed */
        setExtendedUSBDescriptorSet(&descriptors.fs_descs.extendedDescriptorSet[i], i, FALSE);

        /*   - High-speed (almost the same as full-speed except wMaxPacketSize and bInterval!) */
        setExtendedUSBDescriptorSet(&descriptors.hs_descs.extendedDescriptorSet[i], i, TRUE);
    }

    /* Set strings */
    strings.header.magic        = cpu_to_le32(FUNCTIONFS_STRINGS_MAGIC);
    strings.header.length       = cpu_to_le32(sizeof strings);
    strings.header.str_count    = cpu_to_le32(count);
    strings.header.lang_count   = cpu_to_le32(1);

    strings.lang0.code = cpu_to_le16(0x0409); /* en-us */

    memset(strings.lang0.str1, 0, total_string_length);
    strncpy(strings.lang0.str1, STR_INTERFACE_, strlen(STR_INTERFACE_) );
    offset += strlen(STR_INTERFACE_) + 1;

    if (count > 1)
    {
        /* add iOS App names to strings */
        for (i = 0; i < (count-1); i++)
        {
            strLen = (S32)strnlen((const char*)iAP2_ffs_config->iOSAppNames[i], (STRING_MAX - 1) );
            strncpy(&strings.lang0.str1[offset], (const char*)iAP2_ffs_config->iOSAppNames[i], strLen);
            offset += strLen + 1;
        }
    }
    else
    {
        rc = IAP2_OK;
    }
    if(rc == IAP2_OK)
    {
        rc = write(iAP2_ffs_config->ep0_fd, &descriptors, sizeof(descriptors) );
        if (rc != (S32) sizeof(descriptors) )
        {
            IAP2FFSGADGETDLTLOG(DLT_LOG_ERROR, "Failed to write descriptors rc: %d | errno: %d (%s)", rc, errno, strerror(errno));
            iAP2CloseFd(&iAP2_ffs_config->ep0_fd);
        }
        if (iAP2_ffs_config->ep0_fd >= 0)
        {
            rc = write(iAP2_ffs_config->ep0_fd, &strings, sizeof(strings) );
            if (rc != (S32) sizeof(strings) )
            {
                IAP2FFSGADGETDLTLOG(DLT_LOG_ERROR, "Failed to write strings rc: %d | errno: %d (%s)", rc, errno, strerror(errno));
                iAP2CloseFd(&iAP2_ffs_config->ep0_fd);
            }
            else
            {
                rc = IAP2_OK;
            }
        }
        if( (iAP2_ffs_config->ep0_fd >= 0) && (UseConfigFS == TRUE) )
        {
            rc = usbg_enable_gadget(iAP2_gadget, iAP2_usbg_udc);
            iAP2USBHost_PrintLibUsbgResult((usbg_error)rc, "usbg_enable_gadget");
        }
    }

    return rc;
}

LOCAL S32 ep0_init(iAP2_ffs_config_t* iAP2_ffs_config, BOOL UseConfigFS, usbg_gadget *iAP2_gadget, usbg_udc *iAP2_usbg_udc)
{
    S32 ret = IAP2_OK;
    S32 fd  = -1;
    pthread_t gffs_thread;
    void* status;

    /* Open '/dev/ffs/ep0' and write USB descriptors and strings */
    fd = open( (char*)iAP2_ffs_config->initEndPoint, O_NONBLOCK | O_RDWR | O_CLOEXEC);
    if (fd >= 0)
    {
        iAP2_ffs_config->ep0_fd = fd;
        ret = iAP2USBHostConfigDescriptorandStrings(iAP2_ffs_config, UseConfigFS, iAP2_gadget, iAP2_usbg_udc);
        if (ret == 0)
        {
            ret = pthread_create(&gffs_thread, NULL, &gffsReady_thread, (void*)iAP2_ffs_config);
            (void) pthread_join(gffs_thread, &status);

            /* g_ffs is ready if BIND and ENABLE are received or if ENABLE event is received.
             * (SWGIII-7978 FunctionFs events are filtered by kernel, an ENABLE event deletes
             * a pending BIND event if not already read by user via ep0) */
            if ( (iAP2_ffs_config->g_ffs_status == G_FFS_READY) ||
                 (iAP2_ffs_config->g_ffs_status == G_FFS_ENABLED) )
            {
                ret = IAP2_OK;
            }
            else
            {
                ret = IAP2_DEV_NOT_CONNECTED;
                IAP2FFSGADGETDLTLOG(DLT_LOG_ERROR, "ret: %d | g_ffs_status: %d", ret, iAP2_ffs_config->g_ffs_status);
            }
        }
        else
        {
            IAP2FFSGADGETDLTLOG(DLT_LOG_ERROR, "Failed to create gffsReady_thread ret: %d | errno: %d (%s)", ret, errno, strerror(errno));
        }
        if ( (ret != IAP2_OK) && (iAP2_ffs_config->ep0_fd >= 0) )
        {
            /* as fd & ep0_fd are having the same values
             * and value of ep_fd will be udpated on return of ep0_init()
             */
            iAP2CloseFd(&fd);
        }
    }
    else
    {
        IAP2FFSGADGETDLTLOG(DLT_LOG_ERROR, "Failed to open iAP2_ffs_config->initEndPoint errno: %d (%s)", errno, strerror(errno));
    }
    IAP2FFSGADGETDLTLOG(DLT_LOG_DEBUG, "fd = %d", fd);

    return fd;
}

S32 iAP2ConfigureFFSGadget(iAP2_ffs_config_t* iAP2_ffs_config, BOOL UseConfigFS, usbg_gadget *iAP2_gadget, usbg_udc *iAP2_usbg_udc)
{
    S32 rc = IAP2_OK;

    if(iAP2_ffs_config == NULL)
    {
        IAP2FFSGADGETDLTLOG(DLT_LOG_ERROR, "iAP2_ffs_config is NULL");
        rc = IAP2_INVALID_INPUT_PARAMETER;
    }
    else
    {
        if( (UseConfigFS == TRUE) &&
            ( (iAP2_usbg_udc == NULL) || (iAP2_gadget == NULL) ) )
        {
            IAP2FFSGADGETDLTLOG(DLT_LOG_ERROR, "UseConfigFS is TRUE but, iAP2_usbg_udc = %p (or) iAP2_gadget = %p is NULL", iAP2_usbg_udc, iAP2_gadget);
            rc = IAP2_INVALID_INPUT_PARAMETER;
        }
        if(rc == IAP2_OK)
        {
            if(iAP2_ffs_config->initEndPoint == NULL)
            {
                IAP2FFSGADGETDLTLOG(DLT_LOG_ERROR, "initEndPoint is NULL");
                rc = IAP2_INVALID_INPUT_PARAMETER;
            }
            if( (iAP2_ffs_config->nativeTransport == TRUE) &&
                ( (iAP2_ffs_config->iOSAppIdentifier == NULL) ||
                  (iAP2_ffs_config->iOSAppNames == NULL) ) )
            {
                IAP2FFSGADGETDLTLOG(DLT_LOG_ERROR,
                                    "nativeTransport is TRUE but, iOSAppIdentifier = %p (or) iOSAppNames = %p is NULL",
                                    iAP2_ffs_config->iOSAppIdentifier,
                                    iAP2_ffs_config->iOSAppNames);
                rc = IAP2_INVALID_INPUT_PARAMETER;
            }
        }
        if(rc == IAP2_OK)
        {
            iAP2_ffs_config->ep0_fd = ep0_init(iAP2_ffs_config, UseConfigFS, iAP2_gadget, iAP2_usbg_udc);
            if(iAP2_ffs_config->ep0_fd < 0)
            {
                rc = IAP2_CTL_ERROR;
            }
        }
    }

    return rc;
}
