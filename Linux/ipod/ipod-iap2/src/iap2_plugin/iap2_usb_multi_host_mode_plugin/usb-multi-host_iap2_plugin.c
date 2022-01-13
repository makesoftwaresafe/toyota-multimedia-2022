
#include "iap2_dlt_log.h"
#include "iap2_datacom.h"
#include "iap2_multi_host_datacom.h"


void iap2_multihost_mSleep(U32 sleep_ms)
{
    S32 rc = IPOD_DATACOM_SUCCESS;
    struct timespec req;
    struct timespec remain;

    /* Initialize the structure */
    memset(&req, 0, sizeof(req));
    memset(&remain, 0, sizeof(remain));

    req.tv_sec = sleep_ms / IAP2_MULTIHOST_MSEC;
    req.tv_nsec = (sleep_ms % IAP2_MULTIHOST_MSEC) * IAP2_MULTIHOST_NSEC;

    while(1)
    {
        rc = nanosleep(&req, &remain);

        if (rc == 0)
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


LOCAL inline void iAP2FreePointer(void** input_ptr)
{
    if(*input_ptr != NULL)
    {
        free(*input_ptr);
        *input_ptr = NULL;
    }
}


static void iap2_multihost_init_interrupt_transfer(void *devInfo)
{
    S32 rc = IPOD_DATACOM_SUCCESS ;
    IPOD_IAP2_MULTI_HOST_DEV_INFO* devinfo = (IPOD_IAP2_MULTI_HOST_DEV_INFO*)devInfo;

    devinfo->transfer_interrupt = libusb_alloc_transfer(0);
    if(devinfo->transfer_interrupt == NULL)
        IAP2USBPLUGINDLTLOG(DLT_LOG_DEBUG, "transfer_interrupt is NULL ");

    devinfo->transfer_interrupt->flags |= LIBUSB_TRANSFER_FREE_BUFFER;

    devinfo->buffer = malloc(INTERRUPT_DATA);
    if(devinfo->buffer == NULL)
        IAP2USBPLUGINDLTLOG(DLT_LOG_DEBUG, " Buffer is NULL ");


    libusb_fill_interrupt_transfer(devinfo->transfer_interrupt,
                                               devinfo->device_handle_interrupt,
                                               devinfo->link_endpt_info->input_endpoint,
                                               (unsigned char *)devinfo->buffer,
                                               INTERRUPT_DATA,
                                               read_interrupt_callback,
                                               devinfo,
                                               0/*timeout*/);

    rc = libusb_submit_transfer(devinfo->transfer_interrupt);
    if(rc != 0)
        IAP2USBPLUGINDLTLOG(DLT_LOG_DEBUG, "libusb_submit_transfer failed ");

    IAP2USBPLUGINDLTLOG(DLT_LOG_DEBUG, "Interrupt transfer submitted for first time ");

}


S32 iap2_multihost_submit_interrupt_tranfer(void *devInfo)
{
    S32 rc = IPOD_DATACOM_SUCCESS ;
    IPOD_IAP2_MULTI_HOST_DEV_INFO* devinfo = (IPOD_IAP2_MULTI_HOST_DEV_INFO*)devInfo;

    if( (devinfo != NULL) && (devinfo->transfer_interrupt != NULL))
    {
        rc = libusb_submit_transfer(devinfo->transfer_interrupt);

        if(LIBUSB_SUCCESS != rc)
        {
            IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "submit transfer %p failed %d(%s)\n", devinfo->transfer_interrupt, rc, libusb_error_name(rc));
            if ( (rc == LIBUSB_ERROR_NO_DEVICE) || (rc == LIBUSB_TRANSFER_ERROR) )
            {
                rc = IPOD_DATACOM_NOT_CONNECTED ;
            }

            /* free libusb transfer */
            libusb_free_transfer(devinfo->transfer_interrupt);
            devinfo->transfer_interrupt = NULL;
            rc = IPOD_DATACOM_ERROR ;
        }
    }
    else
    {
        rc = IPOD_DATACOM_BAD_PARAMETER;
        IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "devinfo buffer is NULL\n");
    }

    return rc;
}


static void read_interrupt_callback(struct libusb_transfer *transfer)
{
    S32 rc = IPOD_DATACOM_SUCCESS ;
    IPOD_IAP2_MULTI_HOST_DEV_INFO* devInfo = NULL;

    IAP2USBPLUGINDLTLOG(DLT_LOG_DEBUG, "Read callback for interrupt transfer is called");

    if((transfer == NULL) || (transfer->user_data == NULL))
    {
        IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "transfer is NULL\n");
        return;
    }

    devInfo = transfer->user_data;

    if(devInfo->transfer_interrupt != transfer)
        IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "dev->transfer %p != transfer %p\n",devInfo->transfer, transfer);


    if (transfer->status == LIBUSB_TRANSFER_COMPLETED)
    {
        IAP2USBPLUGINDLTLOG(DLT_LOG_VERBOSE, " Interrupt Read transfer is Success %d (%s), read %d bytes\n",transfer->status,libusb_error_name(transfer->status),transfer->actual_length);
        IAP2DLTCONVERTANDLOG(&iAP2USBPluginCtxt,DLT_LOG_VERBOSE, "msgBuffer:  %s\n",devInfo->buffer,transfer->actual_length);
        rc = iap2_multihost_submit_interrupt_tranfer(devInfo);
        if (rc != IPOD_DATACOM_SUCCESS)
            IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "Interrupt libusb transfer submit failed %d(%s) \n",rc,libusb_error_name(rc));
    }
    else if (transfer->status == LIBUSB_TRANSFER_TIMED_OUT)
    {
        IAP2USBPLUGINDLTLOG(DLT_LOG_DEBUG, "Interrupt transfer returned with time out %d (%s)\n",transfer->status,libusb_error_name(transfer->status));
        rc = iap2_multihost_submit_interrupt_tranfer(devInfo);
        if (rc != IPOD_DATACOM_SUCCESS)
            IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "Interrupt libusb transfer submit failed %d(%s) \n",rc,libusb_error_name(rc));
    }
    else
    {
        IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "Interrupt libusb transfer failed with status %d(%s) \n",transfer->status,libusb_error_name(transfer->status));
    }
}


void iAP2USBMultiHostFreeIoCtlConfig(IPOD_IAP2_MULTI_HOST_DEV_INFO *devinfo)
{
    S32 rc = IPOD_DATACOM_SUCCESS;

    if(devinfo->ioctl_config.nativeTransport == TRUE)
    {
        if (NULL != devinfo->device_handle_interrupt)
        {
            /* release the interface */
            rc = libusb_release_interface(devinfo->device_handle_interrupt, INTERFACE_LINK);
            if (LIBUSB_SUCCESS != rc)
            {
                IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "libusb_release_interface returns %d(%s)\n", rc, libusb_error_name(rc));
            }

            /* Close the handle */
            libusb_close(devinfo->device_handle_interrupt);
            devinfo->device_handle_interrupt = NULL;
        }

        if ((devinfo != NULL)&& (devinfo->usb_context_interrupt != NULL))
        {
            libusb_exit(devinfo->usb_context_interrupt);
            devinfo->usb_context_interrupt = NULL;

            if(devinfo->buffer != NULL)
                iAP2FreePointer((void**)&devinfo->buffer);

            if(devinfo->link_endpt_info != NULL)
                iAP2FreePointer((void**)&devinfo->link_endpt_info);

            if(devinfo->alternate_interface_ea != NULL)
                iAP2FreePointer((void**)&devinfo->alternate_interface_ea);

            if(devinfo->pollFDs_interrupt != NULL)
                iAP2FreePointer((void**)&devinfo->pollFDs_interrupt);
        }

        memset(&devinfo->EAcallbacks, 0, sizeof(IPOD_IAP2_DATACOM_ALTERNATE_IF_CB) );
    }

    devinfo->ioctl_config.nativeTransport   = FALSE;
    devinfo->ioctl_config.iap2Device        = NULL;
    devinfo->ioctl_config.context           = NULL;

    IAP2USBPLUGINDLTLOG(DLT_LOG_DEBUG, "iAP2USBMultiHostFreeIoCtlConfig returns");

}


S32 iAP2USBMultiHostInitIoCtlConfig(IPOD_IAP2_MULTI_HOST_DEV_INFO *devinfo, IPOD_IAP2_DATACOM_IOCTL_CONFIG* config)
{
    S32 rc = IPOD_DATACOM_ERROR, rc_tmp = IPOD_DATACOM_ERROR, retry_count = 1;

    if( (NULL != devinfo) && (NULL != config) )
    {
        devinfo->ioctl_config.nativeTransport = config->nativeTransport;
        devinfo->ioctl_config.iap2Device      = config->iap2Device;
        devinfo->ioctl_config.context         = config->context;

        if(devinfo->ioctl_config.nativeTransport == TRUE)
        {

            libusb_device **list;
            libusb_device *device;
            struct libusb_device_descriptor device_descriptor;
            struct libusb_config_descriptor *config_descriptor = NULL;
            U32 i = 0;

            rc_tmp = libusb_init(&(devinfo->usb_context_interrupt));
            if(devinfo->usb_context_interrupt == NULL)
                return IPOD_DATACOM_ERROR ;

            if(config->iOSAppCnt == 1 && rc_tmp == IPOD_DATACOM_SUCCESS)
            {
                devinfo->alternate_interface_ea = calloc(1, sizeof(alternate_interface_ea) );
                if(NULL == devinfo->alternate_interface_ea)
                {
                    IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "devinfo->alternate_interface_ea is NULL");
                    return IPOD_DATACOM_ERR_NOMEM;
                }
                else
                {
                    for (i = 0; i < config->iOSAppCnt; i++)
                    {
                    /* Alternate Interface Number and endpoints are initialized to 0xFF to indicate the application that phone is connected via Molex Hub */
                        devinfo->alternate_interface_ea->interfaceNumber  = ALT_INTERFACE_EAN;
                        devinfo->alternate_interface_ea->inEndpoint       = ALT_INTERFACE_EAN;
                        devinfo->alternate_interface_ea->outEndpoint      = ALT_INTERFACE_EAN;
                        devinfo->alternate_interface_ea->iOSAppIdentifier = config->iOSAppIdentifier[0];
                        IAP2USBPLUGINDLTLOG(DLT_LOG_DEBUG,"Interface Number %d InputEndpoint %x OutputEndpoint %x AppIdentifier %d\n",
                               devinfo->alternate_interface_ea->interfaceNumber,devinfo->alternate_interface_ea->inEndpoint,devinfo->alternate_interface_ea->outEndpoint,
                                   devinfo->alternate_interface_ea->iOSAppIdentifier);
                    }
                }

                devinfo->link_endpt_info = calloc (1,sizeof(iap2_multi_host_endpt_cfg));
                if (devinfo->link_endpt_info == NULL)
                {
                    IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR,"Allocating resource error %d\n",rc);
                    return IPOD_DATACOM_ERR_NOMEM;
                }
                else
                {
                    while((rc == IPOD_DATACOM_ERROR)&&(retry_count <= RETRY_TRIAL))
                    {
                        IAP2USBPLUGINDLTLOG(DLT_LOG_VERBOSE," Enumeration retry trial for %d th time ", retry_count);
                        S32 m = 0;

                        libusb_get_device_list(devinfo->usb_context_interrupt,&list);

                        while ((device = list[m++]) != NULL)
                        {
                            rc_tmp = libusb_get_device_descriptor(device, &device_descriptor);
                            if (rc_tmp != IPOD_DATACOM_SUCCESS)
                            {
                                IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR,"device_descriptor get failed %d(%s) \n",rc_tmp,libusb_error_name(rc_tmp));
                                continue;
                            }
                            rc_tmp = libusb_get_active_config_descriptor(device, &config_descriptor);
                            if (rc_tmp != IPOD_DATACOM_SUCCESS)
                            {
                                IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR,"libusb_get_active_config_descriptor failed %d(%s)",rc_tmp,libusb_error_name(rc_tmp));
                                rc_tmp = libusb_get_config_descriptor(device, 0, &config_descriptor);
                                if (rc_tmp != IPOD_DATACOM_SUCCESS)
                                {
                                    IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR,"libusb_get_active_config_descriptor failed %d(%s)",rc_tmp,libusb_error_name(rc_tmp));
                                }
                            }
                            if (rc_tmp == IPOD_DATACOM_SUCCESS)
                            {
                                for (S32 j = 0; j < config_descriptor->bNumInterfaces; j++)
                                {
                                    const struct libusb_interface *intf = &config_descriptor->interface[j];
                                    for (S32 k = 0; k < intf->num_altsetting; k++)
                                    {
                                        const struct libusb_interface_descriptor *intf_desc;
                                        intf_desc = &intf->altsetting[k];
                                        if ((device_descriptor.idVendor== 0x0424 && device_descriptor.idProduct == 0x4910)&&(intf_desc->bInterfaceNumber == INTERFACE_LINK))
                                        {
                                            IAP2USBPLUGINDLTLOG(DLT_LOG_DEBUG," Vendor id %x Product id %x Interface Number %d \n ",
                                                device_descriptor.idVendor,device_descriptor.idProduct,intf_desc->bInterfaceNumber);
                                            rc = libusb_open(device, &(devinfo->device_handle_interrupt));
                                            IAP2USBPLUGINDLTLOG(DLT_LOG_DEBUG,"libusb_open %d(%s)",rc,libusb_error_name(rc));

                                            if( (rc == IPOD_DATACOM_SUCCESS) && (devinfo->device_handle_interrupt != NULL) )
                                            {
                                                if (libusb_kernel_driver_active(devinfo->device_handle_interrupt, intf_desc->bInterfaceNumber) == KERNEL_DRV_ACTIVE)
                                                {
                                                    rc = libusb_detach_kernel_driver(devinfo->device_handle_interrupt, intf_desc->bInterfaceNumber);
                                                    if (rc != IPOD_DATACOM_SUCCESS) /* returns 0 on success */
                                                    {
                                                        libusb_close(devinfo->device_handle_interrupt);
                                                        devinfo->device_handle_interrupt = NULL;
                                                        IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR," libusb_detach_kernel_driver failed \n with err %d(%s)",rc,libusb_error_name(rc));
                                                        break;
                                                    }
                                                }
                                                rc = libusb_claim_interface(devinfo->device_handle_interrupt, intf_desc->bInterfaceNumber);
                                                if (rc != IPOD_DATACOM_SUCCESS) /* returns 0 on success */
                                                {
                                                    IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR," libusb_claim_interface failed with err %d(%s)\n",rc,libusb_error_name(rc));
                                                    libusb_close(devinfo->device_handle_interrupt);
                                                    devinfo->device_handle_interrupt = NULL;
                                                }
                                                else
                                                {
                                                    for (S32 l = 0; l < intf_desc->bNumEndpoints ; l++)
                                                    {
                                                        const struct libusb_endpoint_descriptor *ep = &intf_desc->endpoint[l];
                                                        S32 is_bulk = (ep->bmAttributes & LIBUSB_TRANSFER_TYPE_MASK) == LIBUSB_TRANSFER_TYPE_INTERRUPT;
                                                        S32 is_output = (ep->bEndpointAddress & LIBUSB_ENDPOINT_DIR_MASK) == LIBUSB_ENDPOINT_OUT;
                                                        S32 is_input = (ep->bEndpointAddress & LIBUSB_ENDPOINT_DIR_MASK) == LIBUSB_ENDPOINT_IN;
                                                        if (is_bulk && is_input)
                                                        {
                                                            /* Use this endpoint for INPUT */
                                                            devinfo->link_endpt_info->input_endpoint = ep->bEndpointAddress;
                                                            devinfo->link_endpt_info->input_ep_max_packet_size = ep->wMaxPacketSize;
                                                        }
                                                        if (is_bulk && is_output)
                                                        {
                                                            /* Use this endpoint for OUTPUT */
                                                            devinfo->link_endpt_info->output_endpoint = ep->bEndpointAddress;
                                                        }
                                                    }
                                                    IAP2USBPLUGINDLTLOG(DLT_LOG_DEBUG," Input endpoint : %d ,output endpoint %d input_ep_max_packet_size %d ",
                                                            devinfo->link_endpt_info->input_endpoint,devinfo->link_endpt_info->output_endpoint,devinfo->link_endpt_info->input_ep_max_packet_size);
                                                    iap2_multihost_init_interrupt_transfer(devinfo);
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                            libusb_free_config_descriptor(config_descriptor);
                        }
                        libusb_free_device_list(list, 1);
                        if(rc == IPOD_DATACOM_ERROR)
                        {
                            retry_count++;
                            iap2_multihost_mSleep(100);
                        }

                    }
                }
            }
        }
        else
        {
            IAP2USBPLUGINDLTLOG(DLT_LOG_DEBUG, "Native EA App Support not requested");
            return IPOD_DATACOM_SUCCESS;
        }
    }
    else
    {
        IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "devinfo or config is NULL, devinfo = %p, config = %p", devinfo, config);
        rc = IPOD_DATACOM_BAD_PARAMETER;
    }
    IAP2USBPLUGINDLTLOG(DLT_LOG_DEBUG, "rc = %d", rc);

    return rc;
}


void iAP2USBMultiHostHandle_EAP(void* devInfo)
{
    IPOD_IAP2_MULTI_HOST_DEV_INFO* devinfo = (IPOD_IAP2_MULTI_HOST_DEV_INFO*)devInfo;

    if((devinfo->buffer[LINK_STATUS]== LINK_UP) && (devinfo->buffer[INTERFACE_STATUS] != 0x00))
    {
        if (devinfo->EAcallbacks.p_iAP2StartAlternateIf_cb != NULL)
        {
            devinfo->EAcallbacks.p_iAP2StartAlternateIf_cb(devinfo->ioctl_config.iap2Device,
                    devinfo->alternate_interface_ea->iOSAppIdentifier, devinfo->alternate_interface_ea->inEndpoint,
                    devinfo->alternate_interface_ea->outEndpoint, devinfo->ioctl_config.context);
            IAP2USBPLUGINDLTLOG(DLT_LOG_DEBUG, "p_iAP2StartAlternateIf_cb is called! ");
        }
        else
        {
            IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "p_iAP2StartAlternateIf_cb is NULL! ");
        }
    }
    else if((devinfo->buffer[LINK_STATUS]== LINK_DOWN) && (devinfo->buffer[INTERFACE_STATUS] != 0x00))
    {
        if (devinfo->EAcallbacks.p_iAP2StopAlternateIf_cb != NULL)
        {
            devinfo->EAcallbacks.p_iAP2StopAlternateIf_cb(devinfo->ioctl_config.iap2Device,
                    devinfo->alternate_interface_ea->iOSAppIdentifier, devinfo->alternate_interface_ea->inEndpoint,
                    devinfo->alternate_interface_ea->outEndpoint, devinfo->ioctl_config.context);
            IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "p_iAP2StopAlternateIf_cb is called! ");
        }
        else
        {
            IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "p_iAP2StopAlternateIf_cb is NULL! ");
        }
    }
    else
    {
        IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "Unknown Interrupt triggered! ");
    }
}


static void iap2_multihost_init_transfer(void *devInfo)
{
    IPOD_IAP2_MULTI_HOST_DEV_INFO* devinfo = (IPOD_IAP2_MULTI_HOST_DEV_INFO*)devInfo;
    devinfo->blk_tfr_data = calloc (1,sizeof(iap2_multi_host_bulk_trsfr));

    if (devinfo->blk_tfr_data == NULL)
    {
        IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "can't allocate buffer resource \n");
        return;
    }
    else
    {
        devinfo->blk_tfr_data->length = IPOD_IAP2_USB_MULTI_HOST_MAX_SEND_BYTES;
        devinfo->blk_tfr_data->rcv_buff = malloc(devinfo->blk_tfr_data->length);
        if(devinfo->blk_tfr_data->rcv_buff != NULL)
        {

            devinfo->transfer = libusb_alloc_transfer(0);

            if (devinfo->transfer == NULL)
            {
                IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "can't allocate libusb transfer \n");
                free(devinfo->blk_tfr_data->rcv_buff);
                return;
            }
            else
            {
                /* libusb shall free the transfer buffer at libusb_free_transfer */
                devinfo->transfer->flags |= LIBUSB_TRANSFER_FREE_BUFFER;

                libusb_fill_bulk_transfer(devinfo->transfer,
                                           devinfo->device_handle,
                                           devinfo->endpt_info->input_endpoint,
                                           devinfo->blk_tfr_data->rcv_buff,
                                           devinfo->blk_tfr_data->length,
                                           iap2_multihost_read_callback,
                                           devinfo,
                                           0/*timeout*/);

                /* Make the first submission here. Further submissions are made
                 from inside iap2_multihost_read_callback() */

                libusb_submit_transfer(devinfo->transfer);
                IAP2USBPLUGINDLTLOG(DLT_LOG_DEBUG, "First libusb submit success ! \n");
            }
        }
        else
        {
            IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "can't allocate buffer resource \n");
        }
    }
}


static void iap2_multihost_read_callback(struct libusb_transfer *transfer)
{
    S32 rc = IPOD_DATACOM_SUCCESS ;
    IPOD_IAP2_MULTI_HOST_DEV_INFO* devInfo = NULL;
    uint64_t event = 100;

    if((transfer == NULL) || (transfer->user_data == NULL))
    {
        IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "transfer is NULL\n");
        return;
    }

    devInfo = transfer->user_data;

    if(devInfo->transfer != transfer)
        IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "dev->transfer %p != transfer %p\n",devInfo->transfer, transfer);


    if (eventfd_write(devInfo->event_fd, event) != 0)
        IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "eventfd_write to %d failed", devInfo->event_fd);
    else
        IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "eventfd_write to %d success", devInfo->event_fd);
}


static void iap2_multihost_libusb_add_pollfd_cb(S32 fd, short events, void *user_context)
{
    libusb_context *context = user_context;

    context = context;
    fd = fd;
    events = events;

    IAP2USBPLUGINDLTLOG(DLT_LOG_DEBUG, "multihost_libusb_add_pollfd_cb triggered to add libusb pollfd (handle: %d, events: %d) \n", fd, events);
}


static void iap2_multihost_libusb_remove_pollfd_cb(S32 fd, void *user_context)
{
    (void)user_context;
    fd = fd;

    IAP2USBPLUGINDLTLOG(DLT_LOG_DEBUG, "multihost_libusb_remove_pollfd_cb triggered to remove libusb pollfd (handle: %d)  \n", fd);
}


S32 iap2_multihost_submit_bulk_tranfer(void *devInfo)
{
    S32 rc = IPOD_DATACOM_SUCCESS ;
    IPOD_IAP2_MULTI_HOST_DEV_INFO* devinfo = (IPOD_IAP2_MULTI_HOST_DEV_INFO*)devInfo;

    if( (devinfo != NULL) && (devinfo->transfer != NULL))
    {
        rc = libusb_submit_transfer(devinfo->transfer);

        if(LIBUSB_SUCCESS != rc)
        {
            IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "submit transfer %p failed %d(%s)\n", devinfo->transfer, rc, libusb_error_name(rc));
            if ( (rc == LIBUSB_ERROR_NO_DEVICE) || (rc == LIBUSB_TRANSFER_ERROR) )
            {
                rc = IPOD_DATACOM_NOT_CONNECTED ;
            }

            /* free libusb transfer */
            libusb_free_transfer(devinfo->transfer);
            devinfo->transfer = NULL;
            rc = IPOD_DATACOM_ERROR ;
        }
    }
    else
    {
        rc = IPOD_DATACOM_BAD_PARAMETER;
        IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "devinfo buffer is NULL\n");
    }

    return rc;
}


S32 iPodiAP2MultiHostLibUsbHandleEvent(libusb_context *usb_context, S32 pollFD)
{
    S32 rc = IPOD_DATACOM_SUCCESS;
    struct timeval tv;

    if( usb_context != NULL )
    {
        IAP2USBPLUGINDLTLOG(DLT_LOG_VERBOSE, "libusb fd %d trigerred",pollFD);

        /* libusb description:
        * "When you detect activity on libusb's file descriptors,
        *  you call libusb_handle_events_timeout() in non-blocking mode."
        *
        * libusb_handle_events_timeout() API description:
        * "If a zero timeval is passed, this function will handle any already-pending events
        *  and then immediately return in non-blocking style."
        * */
        tv.tv_sec = 0;
        tv.tv_usec = 0;
        rc = libusb_handle_events_timeout(usb_context, &tv);
        if (rc != IPOD_DATACOM_SUCCESS)
        {
            /* There was an error. */
            IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "Can't handle libusb events libusb_handle_events_timeout() returns %d(%s) \n", rc,libusb_error_name(rc));
        }
        else
        {
            IAP2USBPLUGINDLTLOG(DLT_LOG_DEBUG, "handled libusb events libusb_handle_events_timeout() returns %d(%s) \n", rc,libusb_error_name(rc));
        }
    }
    else
    {
        IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "usb context is NULL");
        rc = IPOD_DATACOM_BAD_PARAMETER;
    }

    return rc;
}


S32 iap2_multihost_libusb_get_pollfds(void *devInfo, iap2_multi_host_poll_fd *pollFDs, S32 *fd_count)
{
    S32 rc = IPOD_DATACOM_ERROR;
    S32 i = 0, k = 0;
    IPOD_IAP2_MULTI_HOST_DEV_INFO* devinfo = (IPOD_IAP2_MULTI_HOST_DEV_INFO*)devInfo;

    if(devinfo->usb_context == NULL )
    {
        IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "usb_context is NULL");
        return IPOD_DATACOM_BAD_PARAMETER;
    }

    if((devinfo->ioctl_config.nativeTransport == TRUE) && (devinfo->usb_context_interrupt == NULL))
    {
        IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "usb_context_interrupt is NULL");
        return IPOD_DATACOM_BAD_PARAMETER;
    }

    const struct libusb_pollfd **usb_poll_fds = libusb_get_pollfds(devinfo->usb_context);
    const struct libusb_pollfd **usb_poll_fds_interrupt = libusb_get_pollfds(devinfo->usb_context_interrupt);

    if(pollFDs == NULL)
    {
        for(i = 0; usb_poll_fds[i] != NULL; i++)
        {
            (devinfo->nfds_iap)++;
        }
        if(devinfo->ioctl_config.nativeTransport == TRUE)
        {
            for(i = 0; usb_poll_fds_interrupt[i] != NULL; i++)
            {
                (devinfo->nfds_interrupt)++;
            }
        }

        *fd_count = (devinfo->nfds_iap)+(devinfo->nfds_interrupt);

        if(devinfo->event_fd > 0)
        {
            *fd_count = *fd_count + 1;
        }

        rc = IPOD_DATACOM_SUCCESS;
        IAP2USBPLUGINDLTLOG(DLT_LOG_DEBUG, "Total number of fds = %d, iap fds count %d,interrupt interface fds count %d",
                        *fd_count,devinfo->nfds_iap,devinfo->nfds_interrupt);

    }
    else
    {
        IAP2USBPLUGINDLTLOG(DLT_LOG_DEBUG, "When memory is allocated ");
        devinfo->pollFDs_interrupt = calloc(1,(devinfo->nfds_interrupt)*sizeof(iap2_multi_host_poll_fd));
        if((*fd_count) > 0)
        {
            IAP2USBPLUGINDLTLOG(DLT_LOG_DEBUG, "fd_count is greater than zero ");
            for(i = 0; i < (devinfo->nfds_iap); i++)
            {
                (pollFDs)[k].fd = usb_poll_fds[i]->fd;
                (pollFDs)[k].events = usb_poll_fds[i]->events;
                IAP2USBPLUGINDLTLOG(DLT_LOG_DEBUG, "fd at %dth place is = %d and event is %d for iAP interface",
                                k, (pollFDs)[k].fd,(pollFDs)[k].events);
                k++;
            }
            for(i = 0; i < (devinfo->nfds_interrupt); i++)
            {
                (pollFDs)[k].fd = (devinfo->pollFDs_interrupt)[i].fd = usb_poll_fds_interrupt[i]->fd;
                (pollFDs)[k].events = (devinfo->pollFDs_interrupt)[i].events = usb_poll_fds_interrupt[i]->events;
                IAP2USBPLUGINDLTLOG(DLT_LOG_DEBUG, "fd at %dth place is = %d and event is %d for Interrupt interface", k, (pollFDs)[k].fd,(pollFDs)[k].events);
                IAP2USBPLUGINDLTLOG(DLT_LOG_DEBUG, "fd at %dth place is = %d and event is %d for Interrupt interface local variable",
                                k, (devinfo->pollFDs_interrupt)[i].fd,(devinfo->pollFDs_interrupt)[i].events);
                k++;
            }
            (pollFDs)[k].fd = devinfo->event_fd;
            (pollFDs)[k].events = 0x001;

            IAP2USBPLUGINDLTLOG(DLT_LOG_DEBUG, "fd at %dth place is = %d and event is %d for eventfd ",
                            k, (pollFDs)[k].fd,(pollFDs)[k].events);

            rc = IPOD_DATACOM_SUCCESS;
        }
        else
        {
            IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "rc = %d", rc);
        }
    }

    free(usb_poll_fds);
    free(usb_poll_fds_interrupt);

    IAP2USBPLUGINDLTLOG(DLT_LOG_DEBUG, "rc = %d", rc);

    return rc;
}


S32 iPodiAP2USBMultiHostOpenPlugin(void* devInfo,const U8* device_name, S32 flags, S32 mode)
{
    S32 rc = IPOD_DATACOM_ERROR, rc_tmp = IPOD_DATACOM_ERROR;
    S32 interface_num = 0, retry_count = 1;

    /* for compiler warnings */
    flags = flags;
    mode = mode;

    libusb_device **list;
    libusb_device *device;
    struct libusb_device_descriptor device_descriptor;
    struct libusb_config_descriptor *config_descriptor = NULL;

    IPOD_IAP2_MULTI_HOST_DEV_INFO* devinfo = (IPOD_IAP2_MULTI_HOST_DEV_INFO*)devInfo;

    devinfo->event_fd = eventfd(0, EFD_NONBLOCK);

    /* Initialize the device instance */
    if ((NULL != devinfo)&&(NULL != device_name))
    {
        if (devinfo->name[0] == '\0')
        {
            memset(devinfo->name, 0, sizeof(devinfo->name));
            strncpy((VP)devinfo->name, (VP)device_name, sizeof(devinfo->name) - 1);
        }
        else
        {
            if (strncmp((VP)devinfo->name, (VP)device_name, sizeof(devinfo->name)) == 0)
            {
                return IPOD_DATACOM_ALREADY_CONNECTED;
            }
        }
        devinfo->endpt_info = calloc (1,sizeof(iap2_multi_host_endpt_cfg));
        if (devinfo->endpt_info == NULL)
        {
            IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR,"Allocating resource error %d\n",rc);
            return IPOD_DATACOM_ERR_NOMEM;
        }

        while((rc == IPOD_DATACOM_ERROR)&&(retry_count <= RETRY_TRIAL))
        {
            IAP2USBPLUGINDLTLOG(DLT_LOG_VERBOSE," Enumeration retry trial for %d th time ", retry_count);
            S32 i = 0;
            libusb_get_device_list(devinfo->usb_context,&list);

            while ((device = list[i++]) != NULL)
            {
                rc_tmp = libusb_get_device_descriptor(device, &device_descriptor);
                if (rc_tmp != IPOD_DATACOM_SUCCESS)
                {
                    IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR,"device_descriptor get failed %d(%s) \n",rc_tmp,libusb_error_name(rc_tmp));
                    continue;
                }
                rc_tmp = libusb_get_active_config_descriptor(device, &config_descriptor);
                if (rc_tmp != IPOD_DATACOM_SUCCESS)
                {
                    IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR,"libusb_get_active_config_descriptor failed %d(%s)",rc_tmp,libusb_error_name(rc_tmp));
                    rc_tmp = libusb_get_config_descriptor(device, 0, &config_descriptor);
                    if (rc_tmp != IPOD_DATACOM_SUCCESS)
                    {
                        IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR,"libusb_get_active_config_descriptor failed %d(%s)",rc_tmp,libusb_error_name(rc_tmp));
                    }
                }
                if (rc_tmp == IPOD_DATACOM_SUCCESS)
                {
                    for (S32 j = 0; j < config_descriptor->bNumInterfaces; j++)
                    {
                        const struct libusb_interface *intf = &config_descriptor->interface[j];
                        for (S32 k = 0; k < intf->num_altsetting; k++)
                        {
                            const struct libusb_interface_descriptor *intf_desc;
                            intf_desc = &intf->altsetting[k];
                            if ((device_descriptor.idVendor== 0x0424 && device_descriptor.idProduct == 0x4910)&&(intf_desc->bInterfaceNumber == INTERFACE_IAP))
                            {
                                interface_num = intf_desc->bInterfaceNumber;
                                IAP2USBPLUGINDLTLOG(DLT_LOG_DEBUG," Vendor id %x Product id %x Interface Number %d \n ",
                                    device_descriptor.idVendor,device_descriptor.idProduct,interface_num);

                                int count = 0;
                                do
                                {
                                    rc = libusb_open(device, &(devinfo->device_handle));
                                    if(rc < 0)
                                    {
                                        IAP2USBPLUGINDLTLOG(DLT_LOG_WARN,"libusb_open error %d(%s) retry count = %d",rc,libusb_error_name(rc),count);
                                        count++;
                                        iap2_multihost_mSleep(50);
                                    }
                                    else
                                    {
                                        IAP2USBPLUGINDLTLOG(DLT_LOG_WARN,"libusb_open success %d(%s) retry count = %d",rc,libusb_error_name(rc),count);
                                    }
                                }while((count < 20) && (rc < 0)); // This retrial is provided to take care of USB access permisssion issue.

                                if((rc == IPOD_DATACOM_SUCCESS) && (devinfo->device_handle != NULL))
                                {
                                    if (libusb_kernel_driver_active(devinfo->device_handle, intf_desc->bInterfaceNumber) == KERNEL_DRV_ACTIVE)
                                    {
                                        rc = libusb_detach_kernel_driver(devinfo->device_handle, intf_desc->bInterfaceNumber);
                                        if (rc != IPOD_DATACOM_SUCCESS) /* returns 0 on success */
                                        {
                                            libusb_close(devinfo->device_handle);
                                            devinfo->device_handle = NULL;
                                            IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR," libusb_detach_kernel_driver failed \n with err %d(%s)",rc,libusb_error_name(rc));
                                            break;
                                        }
                                    }
                                    rc = libusb_claim_interface(devinfo->device_handle, intf_desc->bInterfaceNumber);
                                    if (rc != IPOD_DATACOM_SUCCESS) /* returns 0 on success */
                                    {
                                        IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR," libusb_claim_interface failed with err %d(%s)\n",rc,libusb_error_name(rc));
                                        libusb_close(devinfo->device_handle);
                                        devinfo->device_handle = NULL;
                                    }
                                    else
                                    {
                                        for (S32 l = 0; l < intf_desc->bNumEndpoints ; l++)
                                        {
                                            const struct libusb_endpoint_descriptor *ep = &intf_desc->endpoint[l];
                                            S32 is_bulk = (ep->bmAttributes & LIBUSB_TRANSFER_TYPE_MASK) == LIBUSB_TRANSFER_TYPE_BULK;
                                            S32 is_output = (ep->bEndpointAddress & LIBUSB_ENDPOINT_DIR_MASK) == LIBUSB_ENDPOINT_OUT;
                                            S32 is_input = (ep->bEndpointAddress & LIBUSB_ENDPOINT_DIR_MASK) == LIBUSB_ENDPOINT_IN;
                                            if (is_bulk && is_input)
                                            {
                                                /* Use this endpoint for INPUT */
                                                devinfo ->endpt_info->input_endpoint = ep->bEndpointAddress;
                                                devinfo ->endpt_info->input_ep_max_packet_size = ep->wMaxPacketSize;
                                            }
                                            if (is_bulk && is_output)
                                            {
                                                /* Use this endpoint for OUTPUT */
                                                devinfo ->endpt_info->output_endpoint = ep->bEndpointAddress;
                                            }
                                        }
                                        IAP2USBPLUGINDLTLOG(DLT_LOG_DEBUG," Input endpoint : %d ,output endpoint %d input_ep_max_packet_size %d ",
                                                devinfo ->endpt_info->input_endpoint,devinfo ->endpt_info->output_endpoint,devinfo->endpt_info->input_ep_max_packet_size);
                                        libusb_set_pollfd_notifiers(devinfo->usb_context, iap2_multihost_libusb_add_pollfd_cb, iap2_multihost_libusb_remove_pollfd_cb, devinfo->usb_context);
                                        iap2_multihost_init_transfer(devinfo);
                                    }
                                }
                                else
                                {
                                    IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR," Device handle is NULL or non-zero return value %d ", rc);
                                    return IPOD_DATACOM_BAD_PARAMETER;
                                }
                            }
                        }
                    }
                }
                libusb_free_config_descriptor(config_descriptor);
            }
            libusb_free_device_list(list, 1);
            if(rc == IPOD_DATACOM_ERROR)
            {
                retry_count++;
                iap2_multihost_mSleep(100);
            } // This retrial is introduced since MHR is accessible from user space only after some time.

        }
    }
    else
    {
        IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "devinfo or device name is NULL");
        rc = IPOD_DATACOM_BAD_PARAMETER;
    }
    IAP2USBPLUGINDLTLOG(DLT_LOG_DEBUG, "rc = %d", rc);

    return rc;
}


S32 iPodiAP2USBMultiHostLibUSBGetFDs(void* devinfo,IPOD_IAP2_DATACOM_FD* getFDs, S32* fd_count)
{
    S32 rc = IPOD_DATACOM_ERROR;

    if (devinfo != NULL)
    {
        rc = iap2_multihost_libusb_get_pollfds(devinfo, (iap2_multi_host_poll_fd*)getFDs, fd_count);
        if( (fd_count != NULL) && (getFDs != NULL) && (rc == IPOD_DATACOM_SUCCESS) )
        {
            S32 i;

            for(i = 0; i < *fd_count; i++)
            {
                IAP2USBPLUGINDLTLOG(DLT_LOG_INFO, "getFDs[%d].fd = %d, getFDs[%d].event = %d", i, getFDs[i].fd, i, getFDs[i].event);
            }
        }
    }
    else
    {
        IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "devinfo is NULL");
        rc = IPOD_DATACOM_BAD_PARAMETER;
    }

    IAP2USBPLUGINDLTLOG(DLT_LOG_DEBUG, "rc = %d", rc);

    return rc;
}


S32 iPodiAP2USBMultiHostIoCtl(void* devInfo, S32 request, void* argp)
{
    S32 rc = IPOD_DATACOM_SUCCESS;
    IPOD_IAP2_MULTI_HOST_DEV_INFO* devinfo = (IPOD_IAP2_MULTI_HOST_DEV_INFO*)devInfo;

    if( (devinfo != NULL) && (argp != NULL) )
    {
        switch(request)
        {
            case IPOD_IAP2_DATACOM_IOCTL_SET_CONFIG:
            {
                rc = iAP2USBMultiHostInitIoCtlConfig(devinfo, (IPOD_IAP2_DATACOM_IOCTL_CONFIG*)argp);
                if(IPOD_DATACOM_SUCCESS != rc)
                {
                    IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "iAP2USBMultiHostInitIoCtlConfig() failed  rc = %d", rc);
                    iAP2USBMultiHostFreeIoCtlConfig(devinfo);
                }

                break;
            }

            case IPOD_IAP2_DATACOM_IOCTL_SET_CB:
            {
                memcpy( &(devinfo->EAcallbacks), (IPOD_IAP2_DATACOM_ALTERNATE_IF_CB*)argp, sizeof(IPOD_IAP2_DATACOM_ALTERNATE_IF_CB) );
                break;
            }

            default:
            {
                IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "request = %d", request);
                rc = IPOD_DATACOM_ERROR;
                break;
            }
        }
    }
    else
    {
        IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "devinfo = %p, argp = %p", devinfo, argp);
        rc = IPOD_DATACOM_BAD_PARAMETER;
    }
    return rc;
}


S32 iPodiAP2USBMultiHostWrite(void *devinfo, U32 msgLenTotal, const U8 *iPod_msg, S32 flags)
{
    S32 rc = IPOD_DATACOM_SUCCESS;
    S32 actual_length = 0;
    const U32 timeoutMs = 5000; //To make it non-blocking timeout of 5000ms introduced as done in f_fs driver(PR#202)
    IPOD_IAP2_MULTI_HOST_DEV_INFO* devInfo = (IPOD_IAP2_MULTI_HOST_DEV_INFO*)devinfo;
    /* for compiler warnings */
    flags = flags;

    if ((NULL != devInfo)&&(NULL != iPod_msg)&&(msgLenTotal > 0)&&(devInfo->endpt_info->output_endpoint > 0)&&(devInfo->device_handle != NULL))
    {
        rc = libusb_bulk_transfer(devInfo->device_handle,devInfo->endpt_info->output_endpoint,(unsigned char*) iPod_msg,msgLenTotal,&actual_length, timeoutMs);

        if (rc == IPOD_DATACOM_SUCCESS)
        {
            IAP2USBPLUGINDLTLOG(DLT_LOG_VERBOSE, "libusb_bulk_transfer() success returns %d(%s)", rc, libusb_error_name(rc));
            rc = actual_length;
        }
        else if(rc == LIBUSB_ERROR_TIMEOUT)
        {
            IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "libusb_bulk_transfer() returns %d(%s)", rc, libusb_error_name(rc));
            rc = IPOD_DATACOM_ERR_ABORT;
        }
        else if (rc < IPOD_DATACOM_SUCCESS)
        {
            IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "libusb_bulk_transfer() returns %d(%s)", rc, libusb_error_name(rc));
            rc = IPOD_DATACOM_NOT_CONNECTED;
        }
        else
        {
            IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "libusb_bulk_transfer() returns with error %d(%s)", rc, libusb_error_name(rc));
            rc = IPOD_DATACOM_ERROR ;
        }
    }
    else  /* iPodHndl == NULL or buf == NULL or sendInfo == NULL or invalid endpoint */
    {
        IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "devinfo or iPod_msg is NULL, Bad parameter error");
        rc = IPOD_DATACOM_BAD_PARAMETER;
    }

    return rc;
}


S32 iPodiAP2USBMultiHostLibUSBHandleEvent(void* devInfo, U32 buffer_size, U8 *msgBuffer, S32 pollFD)
{
    S32 rc = IPOD_DATACOM_ERROR;
    IPOD_IAP2_MULTI_HOST_DEV_INFO* devinfo = (IPOD_IAP2_MULTI_HOST_DEV_INFO*)devInfo;
    /* for compiler warnings */
    buffer_size = buffer_size;
    int interrupt_transfer=0;
    uint64_t event;

    IAP2USBPLUGINDLTLOG(DLT_LOG_DEBUG, "Poll fd triggered %d", pollFD);

    for(int i=0;i<(devinfo->nfds_interrupt);i++)
    {
        if(pollFD == (devinfo->pollFDs_interrupt)[i].fd)
        {
            interrupt_transfer=1;
            IAP2USBPLUGINDLTLOG(DLT_LOG_DEBUG, "Interrupt Transfer %d", interrupt_transfer);
        }
    }

    if((devinfo != NULL) && (msgBuffer != NULL) && (interrupt_transfer != 1))
    {
        if(devinfo->event_fd == pollFD) /*USB data received by the callback*/
        {
            if (eventfd_read(devinfo->event_fd, &event) != 0)
            {
                IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "eventfd_read from %d failed\n", devinfo->event_fd);
            }

            if( devinfo->transfer->status == LIBUSB_TRANSFER_COMPLETED)
            {
                IAP2USBPLUGINDLTLOG(DLT_LOG_VERBOSE, " Bulk Read transfer is Success %d (%s), read %d bytes\n",
                        devinfo->transfer->status,
                        libusb_error_name(devinfo->transfer->status),
                        devinfo->transfer->actual_length);
                memcpy((void*)msgBuffer,(const void*)devinfo->blk_tfr_data->rcv_buff, devinfo->transfer->actual_length);

                /*submit read request to continue reading*/
                rc = iap2_multihost_submit_bulk_tranfer(devInfo);
                if (rc != IPOD_DATACOM_SUCCESS)
                    IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "Bulk libusb transfer submit failed %d(%s) \n",rc, libusb_error_name(rc));

                /*bytes read from the USB for iAP2HandleEvent*/
                rc = devinfo->transfer->actual_length;
            }
            else if (devinfo->transfer->status == LIBUSB_TRANSFER_TIMED_OUT)
            {
                IAP2USBPLUGINDLTLOG(DLT_LOG_VERBOSE, "Bulk transfer returned with time out %d (%s)\n",
                        devinfo->transfer->status,libusb_error_name(devinfo->transfer->status));
                rc = iap2_multihost_submit_bulk_tranfer(devInfo);
                if (rc != IPOD_DATACOM_SUCCESS)
                    IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "Bulk libusb transfer submit failed %d(%s) \n",rc,libusb_error_name(rc));

                rc = IPOD_DATACOM_ERROR;
            }
            else
            {
                rc = IPOD_DATACOM_NOT_CONNECTED ;
                IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "Bulk libusb transfer failed with status %d(%s) \n",
                        devinfo->transfer->status,libusb_error_name(devinfo->transfer->status));
            }
        }
        else /*USB internal event*/
        {
            rc = iPodiAP2MultiHostLibUsbHandleEvent(devinfo->usb_context, pollFD);
        }
    }
    else if ((devinfo != NULL) && (devinfo->usb_context_interrupt != NULL) && (interrupt_transfer == 1))
    {
        interrupt_transfer = 0;

        rc = iPodiAP2MultiHostLibUsbHandleEvent(devinfo->usb_context_interrupt, pollFD);
        iAP2USBMultiHostHandle_EAP(devinfo);

    }
    else
    {
        IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "devinfo = %p, msgBuffer = %p", devinfo, msgBuffer);
        rc = IPOD_DATACOM_BAD_PARAMETER;
    }

    return rc;
}


S32 iPodiAP2USBMultiHostClosePlugin(void *devInfo)
{
    S32 rc = IPOD_DATACOM_ERROR;
    IPOD_IAP2_MULTI_HOST_DEV_INFO* devinfo = (IPOD_IAP2_MULTI_HOST_DEV_INFO*)devInfo;

    close(devinfo->event_fd);
    devinfo->event_fd = -1;

    if ((devinfo != NULL)&& (devinfo->usb_context != NULL))
    {
        libusb_exit(devinfo->usb_context);
        devinfo->usb_context = NULL;

        if(devinfo->blk_tfr_data != NULL)
        {
            if(devinfo->blk_tfr_data->rcv_buff != NULL)
            {
                iAP2FreePointer( (void**)&devinfo->blk_tfr_data->rcv_buff);
            }
            iAP2FreePointer( (void**)&devinfo->blk_tfr_data);
        }

        if(devinfo->endpt_info != NULL)
        {
            iAP2FreePointer( (void**)&devinfo->endpt_info);
        }

        rc = IPOD_DATACOM_SUCCESS;
    }

    iAP2USBMultiHostFreeIoCtlConfig(devinfo);

    IAP2USBPLUGINDLTLOG(DLT_LOG_DEBUG, "rc = %d", rc);

    return rc;
}


S32 iPodiAP2USBMultiHostCloseMsgHandling(void *devInfo)
{
    S32 rc = IPOD_DATACOM_SUCCESS;
    IPOD_IAP2_MULTI_HOST_DEV_INFO* devinfo = (IPOD_IAP2_MULTI_HOST_DEV_INFO*)devInfo;

    if ((devinfo != NULL) )
    {
        IAP2USBPLUGINDLTLOG(DLT_LOG_DEBUG, "close occur ! (%p)", devinfo);

        if (NULL != devinfo->device_handle)
        {
            /* release the interface */
            rc = libusb_release_interface(devinfo->device_handle, INTERFACE_IAP);
            if (LIBUSB_SUCCESS != rc)
            {
                IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "libusb_release_interface returns %d(%s)\n", rc, libusb_error_name(rc));
            }

            /* Close the handle */
            libusb_close(devinfo->device_handle);
            devinfo->device_handle = NULL;
        }
    }
    else
    {
        IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "devinfo or usb context is NULL");
        rc = IPOD_DATACOM_BAD_PARAMETER;
    }
    IAP2USBPLUGINDLTLOG(DLT_LOG_DEBUG, "rc = %d", rc);

    return rc;
}


S32 iPodiAP2USBMultiHostGetProperty(void* iPodHdl, IPOD_IAP2_DATACOM_PROPERTY *property)
{
    S32 rc = IPOD_DATACOM_ERROR;
    IPOD_IAP2_MULTI_HOST_DEV_INFO *devinfo = (IPOD_IAP2_MULTI_HOST_DEV_INFO*)iPodHdl;

    if( (devinfo != NULL) && (property != NULL) )
    {
        property->maxSize = IPOD_IAP2_USB_MULTI_HOST_MAX_SEND_BYTES;
        rc = IPOD_DATACOM_SUCCESS;
    }
    else
    {
        IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "devinfo = %p, property = %p", devinfo, property);
    }

    return rc;
}


S32 iPodiAP2USBMultiHostReceiveMessage(void* iPodHdl, U32 buffer_size, U8 *msgBuffer, S32 flags)
{
    S32 rc = IPOD_DATACOM_SUCCESS;

    /* for compiler warnings */
    iPodHdl     = iPodHdl;
    buffer_size = buffer_size;
    msgBuffer   = msgBuffer;
    flags       = flags;

    return rc;
}


void* iPodiAP2USBMultiHostComInit(IPOD_IAP2_DATACOM_FUNC_TABLE* data_com_function)
{

    S32 rc = IPOD_DATACOM_SUCCESS;
    IPOD_IAP2_MULTI_HOST_DEV_INFO* devinfo = NULL;

    if (data_com_function != NULL)
    {
        data_com_function->hdlevent = &iPodiAP2USBMultiHostLibUSBHandleEvent;
        data_com_function->getfds = &iPodiAP2USBMultiHostLibUSBGetFDs;
        data_com_function->open = &iPodiAP2USBMultiHostOpenPlugin;
        data_com_function->close = &iPodiAP2USBMultiHostClosePlugin;
        data_com_function->abort = &iPodiAP2USBMultiHostCloseMsgHandling;
        data_com_function->write = &iPodiAP2USBMultiHostWrite;
        data_com_function->read = &iPodiAP2USBMultiHostReceiveMessage;
        data_com_function->ioctl = &iPodiAP2USBMultiHostIoCtl;
        data_com_function->property = &iPodiAP2USBMultiHostGetProperty;
    }
    else
    {
        IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "data_com_function is NULL \n");
        rc = IPOD_DATACOM_BAD_PARAMETER;
    }

    if(rc == IPOD_DATACOM_SUCCESS)
    {
        devinfo = calloc(1,sizeof(IPOD_IAP2_MULTI_HOST_DEV_INFO));

        if (devinfo != NULL)
        {
            IAP2USBPLUGINDLTLOG(DLT_LOG_DEBUG, "devinfo = %p", devinfo);
            devinfo->usb_context = devinfo->usb_context_interrupt = NULL;

            rc = libusb_init(&(devinfo->usb_context));
            if(devinfo->usb_context == NULL)
                rc = IPOD_DATACOM_ERROR ;

            devinfo->nfds_iap = devinfo->nfds_interrupt = 0;
        }
        else
        {
            rc = IPOD_DATACOM_ERR_NOMEM;
        }
    }

    IAP2USBPLUGINDLTLOG(DLT_LOG_DEBUG, "rc %d \n",rc);
    return devinfo;
}


S32 iPodiAP2USBMultiHostComDeinit(IPOD_IAP2_DATACOM_FUNC_TABLE* data_com_function,void* iPodHdl)
{
    S32 rc = IPOD_DATACOM_SUCCESS;
    IPOD_IAP2_MULTI_HOST_DEV_INFO* devinfo = (IPOD_IAP2_MULTI_HOST_DEV_INFO*)iPodHdl;

    if((data_com_function != NULL) && (devinfo != NULL) )
    {
        data_com_function->hdlevent = NULL;
        data_com_function->getfds = NULL;
        data_com_function->open = NULL;
        data_com_function->close = NULL;
        data_com_function->abort = NULL;
        data_com_function->write = NULL;
        data_com_function->read = NULL;
        data_com_function->ioctl = NULL;
        data_com_function->property = NULL;


        memset(devinfo, 0, sizeof(*devinfo));
        devinfo = NULL;

        IAP2USBPLUGINDLTLOG(DLT_LOG_DEBUG, "rc = %d", rc);
    }
    else
    {
        IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "devinfo is not NULL or data_com_function is NULL");
        rc = IPOD_DATACOM_BAD_PARAMETER;
    }

    return rc;
}
