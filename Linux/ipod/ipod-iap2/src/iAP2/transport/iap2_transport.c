#include <stdio.h>

#include <adit_typedef.h>
#include "iap2_transport.h"

#include "iap2_dlt_log.h"
#include "iap2_transport_dependent.h"
#include "iap2_parameters.h"
#include "iAP2LinkRunLoop.h"
#include "iAP2Link.h"
#include "iap2_init_private.h"

#include "iap2_datacom.h"


LOCAL S32 iAP2InitTransportConfig(iAP2Device_t* device, iAP2InitParam_t* iap2InitParam);
LOCAL S32 iAP2FreeTransportConfig(iAP2Transport_t *iAP2Transport);

void iAP2FreePointer(void** input_ptr)
{
    if(*input_ptr != NULL)
    {
        free(*input_ptr);
        *input_ptr = NULL;
    }
}

S32 iAP2AllocateSPtr(U8** dest_ptr, U8* src_ptr)
{
    S32 rc = IAP2_OK;
    U32 StringLength;

    StringLength = strnlen( (const char*)src_ptr, STRING_MAX) ;
    *dest_ptr = (U8*)calloc( (StringLength + IAP2_NULL_CHAR_LEN), sizeof(U8));
    if(*dest_ptr == NULL)
    {
        rc = IAP2_ERR_NO_MEM;
    }
    else
    {
        memcpy(*dest_ptr, src_ptr, StringLength);
        (*dest_ptr)[StringLength] = '\0';
    }

    return rc;
}

S32 iAP2StartEANativeTransport_CB(void* device, U8 iAP2iOSAppIdentifier, U8 sinkEndpoint, U8 sourceEndpoint, void* context)
{
    S32 rc = IAP2_OK;
    iAP2Device_st* iap2Device = NULL;

    iap2Device    = (iAP2Device_st*)device;
    if((NULL != iap2Device)
       && (NULL != iap2Device->p_iAP2EANativeTransportCallbacks.p_iAP2StartEANativeTransport_cb))
    {
        iap2Device->p_iAP2EANativeTransportCallbacks.p_iAP2StartEANativeTransport_cb(device,
                                                                                     iAP2iOSAppIdentifier,
                                                                                     sinkEndpoint,
                                                                                     sourceEndpoint,
                                                                                     context);
    }
    else
    {
        IAP2TRANSPORTDLTLOG(DLT_LOG_ERROR,"p_iAP2StartEANativeTransport_cb is NULL DevID:%p",iap2Device);
        rc = IAP2_CTL_ERROR;
    }

    return rc;
}

S32 iAP2StopEANativeTransport_CB(void* device, U8 iAP2iOSAppIdentifier, U8 sinkEndpoint, U8 sourceEndpoint, void* context)
{
    S32 rc = IAP2_OK;
    iAP2Device_st* iap2Device = NULL;

    iap2Device    = (iAP2Device_st*)device;
    if((NULL != iap2Device)
       && (NULL != iap2Device->p_iAP2EANativeTransportCallbacks.p_iAP2StopEANativeTransport_cb))
    {
        iap2Device->p_iAP2EANativeTransportCallbacks.p_iAP2StopEANativeTransport_cb(device,
                                                                                    iAP2iOSAppIdentifier,
                                                                                    sinkEndpoint,
                                                                                    sourceEndpoint,
                                                                                    context);
    }
    else
    {
        IAP2TRANSPORTDLTLOG(DLT_LOG_ERROR,"p_iAP2StopEANativeTransport_cb is NULL DevID:%p",iap2Device);
        rc = IAP2_CTL_ERROR;
    }

    return rc;
}


LOCAL S32 iAP2FreeTransportConfig(iAP2Transport_t *iAP2Transport)
{
    S32 rc = IAP2_OK;

    if(NULL == iAP2Transport)
    {
        return IAP2_BAD_PARAMETER;
    }
    else
    {
        if(NULL != iAP2Transport->iAP2DeviceIdentifier)
        {
            iAP2FreePointer( (void**)&iAP2Transport->iAP2DeviceIdentifier);
        }
        if(NULL != iAP2Transport->iAP2IoCtlConfig.accManufacture)
        {
            iAP2FreePointer( (void**)&iAP2Transport->iAP2IoCtlConfig.accManufacture);
        }
        if(NULL != iAP2Transport->iAP2IoCtlConfig.accProduct)
        {
            iAP2FreePointer( (void**)&iAP2Transport->iAP2IoCtlConfig.accProduct);
        }
        if(NULL != iAP2Transport->iAP2IoCtlConfig.accSerialNumber)
        {
            iAP2FreePointer( (void**)&iAP2Transport->iAP2IoCtlConfig.accSerialNumber);
        }
        if(NULL != iAP2Transport->iAP2IoCtlConfig.accVendorId)
        {
            iAP2FreePointer( (void**)&iAP2Transport->iAP2IoCtlConfig.accVendorId);
        }
        if(NULL != iAP2Transport->iAP2IoCtlConfig.accProductId)
        {
            iAP2FreePointer( (void**)&iAP2Transport->iAP2IoCtlConfig.accProductId);
        }
        if(NULL != iAP2Transport->iAP2IoCtlConfig.powerGPIO)
        {
            iAP2FreePointer( (void**)&iAP2Transport->iAP2IoCtlConfig.powerGPIO);
        }
        if(NULL != iAP2Transport->iAP2IoCtlConfig.iOSAppNames)
        {
            iAP2FreePointer( (void**)&iAP2Transport->iAP2IoCtlConfig.iOSAppNames);
        }
        if(NULL != iAP2Transport->iAP2IoCtlConfig.iOSAppIdentifier)
        {
            iAP2FreePointer( (void**)&iAP2Transport->iAP2IoCtlConfig.iOSAppIdentifier);
        }
        if(NULL != iAP2Transport->iAP2IoCtlConfig.accBcdDevice)
        {
            iAP2FreePointer( (void**)&iAP2Transport->iAP2IoCtlConfig.accBcdDevice);
        }
        if(NULL != iAP2Transport->iAP2IoCtlConfig.initEndPoint)
        {
            iAP2FreePointer( (void**)&iAP2Transport->iAP2IoCtlConfig.initEndPoint);
        }

        iAP2Transport->iAP2IoCtlConfig.digitaliPodOut = FALSE;
        iAP2Transport->iAP2IoCtlConfig.nativeTransport = FALSE;
        iAP2Transport->iAP2IoCtlConfig.iOSAppCnt = 0;
        iAP2Transport->iAP2IoCtlConfig.iOSAppNamesLen = 0;
        iAP2Transport->iAP2IoCtlConfig.useConfigFS = FALSE;

        iAP2Transport->iAP2IoCtlConfig.iap2Device = NULL;
        iAP2Transport->iAP2IoCtlConfig.context = NULL;

        memset(&iAP2Transport->iAP2EAnativetransport_cb, 0, sizeof(IPOD_IAP2_DATACOM_ALTERNATE_IF_CB));
    }

    return rc;
}

LOCAL S32 iAP2InitTransportConfig(iAP2Device_t* device, iAP2InitParam_t* iap2InitParam)
{
    S32 rc = IAP2_OK;
    S32 len = 0;
    U16 i = 0;
    U32 offset = 0;
    iAP2Device_st* iap2Device = NULL;
    iAP2Transport_t* iAP2Transport = NULL;

    iap2Device    = (iAP2Device_st*)device;
    iAP2Transport = &(iap2Device->iAP2Transport);

    if(NULL == iap2InitParam)
    {
        return IAP2_BAD_PARAMETER;
    }
    else if( (NULL == iap2InitParam->p_iAP2AccessoryConfig) || (NULL == iap2InitParam->p_iAP2AccessoryInfo) )
    {
        return IAP2_BAD_PARAMETER;
    }
    else if(((iap2InitParam->p_iAP2AccessoryConfig->iAP2TransportType == iAP2USBHOSTMODE) ||(iap2InitParam->p_iAP2AccessoryConfig->iAP2TransportType == iAP2MULTIHOSTMODE))&&
             ((NULL == iap2InitParam->p_iAP2AccessoryInfo->iAP2AccessoryManufacturer)
              ||(NULL == iap2InitParam->p_iAP2AccessoryInfo->iAP2AccessoryName)
              ||(NULL == iap2InitParam->p_iAP2AccessoryInfo->iAP2AccessorySerialNumber)
              ||(NULL == iap2InitParam->p_iAP2AccessoryInfo->iAP2AccessoryVendorId)
              ||(NULL == iap2InitParam->p_iAP2AccessoryInfo->iAP2AccessoryProductId)
              ||(NULL == iap2InitParam->p_iAP2AccessoryInfo->iAP2AccessoryBcdDevice)
              ||(NULL == iap2InitParam->p_iAP2AccessoryInfo->iAP2InitEndPoint)
              ||(NULL == iap2InitParam->p_iAP2AccessoryConfig->iAP2UsbOtgGPIOPower)) )
    {
        return IAP2_BAD_PARAMETER;
    }
    else
    {
        iAP2Transport->iAP2IoCtlConfig.iap2Device = device;
        iAP2Transport->iAP2IoCtlConfig.context = iap2Device->iAP2ContextCallback;

        iAP2Transport->iAP2TransportType = iap2InitParam->p_iAP2AccessoryConfig->iAP2TransportType;
        iAP2Transport->iAP2IoCtlConfig.digitaliPodOut = iap2InitParam->p_iAP2AccessoryConfig->iAP2iOSintheCar;
        iAP2Transport->iAP2IoCtlConfig.useConfigFS = iap2InitParam->p_iAP2AccessoryConfig->useConfigFS;

        /* get serial number of the connected Apple device */
        rc = iAP2AllocateSPtr((U8**)&iAP2Transport->iAP2DeviceIdentifier, iap2InitParam->iAP2DeviceId);

        if(((iAP2Transport->iAP2TransportType == iAP2USBHOSTMODE)||(iAP2Transport->iAP2TransportType == iAP2MULTIHOSTMODE)) && (rc == IAP2_OK))
        {
            /* get manufacture name of the accessory */
            rc = iAP2AllocateSPtr((U8**)&iAP2Transport->iAP2IoCtlConfig.accManufacture, iap2InitParam->p_iAP2AccessoryInfo->iAP2AccessoryManufacturer);

            if(rc == IAP2_OK)
            {
                /* get product name of the accessory */
                rc = iAP2AllocateSPtr((U8**)&iAP2Transport->iAP2IoCtlConfig.accProduct, iap2InitParam->p_iAP2AccessoryInfo->iAP2AccessoryName);
            }

            if(rc == IAP2_OK)
            {
                /* get serial number of the accessory */
                rc = iAP2AllocateSPtr((U8**)&iAP2Transport->iAP2IoCtlConfig.accSerialNumber, iap2InitParam->p_iAP2AccessoryInfo->iAP2AccessorySerialNumber);
            }

            if(rc == IAP2_OK)
            {
                /* get Vendor ID of the accessory */
                rc = iAP2AllocateSPtr((U8**)&iAP2Transport->iAP2IoCtlConfig.accVendorId, iap2InitParam->p_iAP2AccessoryInfo->iAP2AccessoryVendorId);
            }

            if(rc == IAP2_OK)
            {
                 /* get Product ID of the accessory */
                rc = iAP2AllocateSPtr((U8**)&iAP2Transport->iAP2IoCtlConfig.accProductId, iap2InitParam->p_iAP2AccessoryInfo->iAP2AccessoryProductId);
            }

            if(rc == IAP2_OK)
            {
                /* get the bcdDevice value which indicates the device-defined revision number */
                rc = iAP2AllocateSPtr((U8**)&iAP2Transport->iAP2IoCtlConfig.accBcdDevice, iap2InitParam->p_iAP2AccessoryInfo->iAP2AccessoryBcdDevice);
            }

            if(rc == IAP2_OK)
            {
                /* get filename to the init endpoint*/
                rc = iAP2AllocateSPtr((U8**)&iAP2Transport->iAP2IoCtlConfig.initEndPoint, iap2InitParam->p_iAP2AccessoryInfo->iAP2InitEndPoint);
            }

            if(rc == IAP2_OK)
            {
                /* get GPIO port number to switch power on/off at USB OTG port */
                rc = iAP2AllocateSPtr((U8**)&iAP2Transport->iAP2IoCtlConfig.powerGPIO, iap2InitParam->p_iAP2AccessoryConfig->iAP2UsbOtgGPIOPower);
            }

            if(rc == IAP2_OK)
            {
                /* get the number of iOS Apps */
                iAP2Transport->iAP2IoCtlConfig.iOSAppCnt = 0;
                if(iap2InitParam->p_iAP2AccessoryInfo->iAP2SupportediOSAppCount > 0)
                {
                    if(iap2InitParam->p_iAP2AccessoryInfo->iAP2iOSAppInfo == NULL)
                    {
                        rc = IAP2_BAD_PARAMETER;
                    }
                    /* get total length of all iOS app names */
                    len = 0;
                    for(i=0; ( (i<iap2InitParam->p_iAP2AccessoryInfo->iAP2SupportediOSAppCount) && (rc == IAP2_OK) ); i++)
                    {
                        /* search for iOS Apps which should use native transport */
                        if(TRUE == iap2InitParam->p_iAP2AccessoryInfo->iAP2iOSAppInfo[i].iAP2EANativeTransport)
                        {
                            if(iap2InitParam->p_iAP2AccessoryInfo->iAP2iOSAppInfo[i].iAP2iOSAppName != NULL)
                            {
                                len += (S32)strnlen((const char*)iap2InitParam->p_iAP2AccessoryInfo->iAP2iOSAppInfo[i].iAP2iOSAppName,STRING_MAX) +1;
                            }
                            else
                            {
                                IAP2TRANSPORTDLTLOG(DLT_LOG_ERROR,"Name of %d iOSApp is null ",i);
                                return IAP2_BAD_PARAMETER;
                            }
                        }
                    }
                }
                if(len > 0)
                {
                    iAP2Transport->iAP2IoCtlConfig.iOSAppNamesLen = len;
                    /* allocate one buffer for all strings */
                    iAP2Transport->iAP2IoCtlConfig.iOSAppNames = (char*)calloc(1, iAP2Transport->iAP2IoCtlConfig.iOSAppNamesLen);
                    if(NULL == iAP2Transport->iAP2IoCtlConfig.iOSAppNames)
                    {
                        return IAP2_ERR_NO_MEM;
                    }
                    else
                    {
                        memset(iAP2Transport->iAP2IoCtlConfig.iOSAppNames , 0, iAP2Transport->iAP2IoCtlConfig.iOSAppNamesLen);
                        for(i=0; i<iap2InitParam->p_iAP2AccessoryInfo->iAP2SupportediOSAppCount; i++)
                        {
                            if(TRUE == iap2InitParam->p_iAP2AccessoryInfo->iAP2iOSAppInfo[i].iAP2EANativeTransport)
                            {
                                len = (S32)strnlen((const char*)iap2InitParam->p_iAP2AccessoryInfo->iAP2iOSAppInfo[i].iAP2iOSAppName,STRING_MAX);
                                strncpy(&iAP2Transport->iAP2IoCtlConfig.iOSAppNames[offset],
                                        (const char*)iap2InitParam->p_iAP2AccessoryInfo->iAP2iOSAppInfo[i].iAP2iOSAppName,
                                        len+1);
                                offset += len+1;
                                iAP2Transport->iAP2IoCtlConfig.iOSAppCnt++;
                                iAP2Transport->iAP2IoCtlConfig.nativeTransport = TRUE;
                            }
                        }
                    }
                    if(iAP2Transport->iAP2IoCtlConfig.iOSAppCnt > 0)
                    {
                        iAP2Transport->iAP2IoCtlConfig.iOSAppIdentifier = (U8*)calloc(iAP2Transport->iAP2IoCtlConfig.iOSAppCnt, sizeof(U8));
                        if(NULL == iAP2Transport->iAP2IoCtlConfig.iOSAppIdentifier)
                        {
                            return IAP2_ERR_NO_MEM;
                        }
                        else
                        {
                            memset(iAP2Transport->iAP2IoCtlConfig.iOSAppIdentifier, 0, iAP2Transport->iAP2IoCtlConfig.iOSAppCnt);
                            for(i=0; i<iAP2Transport->iAP2IoCtlConfig.iOSAppCnt; i++)
                            {
                                iAP2Transport->iAP2IoCtlConfig.iOSAppIdentifier[i] = iap2InitParam->p_iAP2AccessoryInfo->iAP2iOSAppInfo[i].iAP2iOSAppIdentifier;
                            }
                        }
                    }
                }
                else
                {
                    IAP2TRANSPORTDLTLOG(DLT_LOG_WARN,"iAP2USBHOSTMODE set, but no iOS App notified for Native Transport DevID:%p",device);
                }
            }

            if(rc == IAP2_OK)
            {
                /* assign EA native transport callbacks */
                iAP2Transport->iAP2EAnativetransport_cb.p_iAP2StartAlternateIf_cb = &iAP2StartEANativeTransport_CB;
                iAP2Transport->iAP2EAnativetransport_cb.p_iAP2StopAlternateIf_cb  = &iAP2StopEANativeTransport_CB;
            }
        }
    }

    return rc;
}


S32 iAP2TransportInit(iAP2Device_t* device, iAP2InitParam_t* iap2InitParam)
{
    S32 rc = IAP2_CTL_ERROR;
    iAP2Device_st* iap2Device = NULL;
    iAP2Transport_t* iAP2Transport = NULL;

    iap2Device    = (iAP2Device_st*)device;
    iAP2Transport = &(iap2Device->iAP2Transport);

    rc = iAP2InitTransportConfig(device ,iap2InitParam);
    if(rc == IAP2_OK)
    {
        switch(iAP2Transport->iAP2TransportType)
        {
            case iAP2USBHOSTMODE:
            {
                iAP2Transport->iAP2TransportHdl = iPodiAP2USBHostComInit(&(iAP2Transport->iAP2DataComFunction));
                if(NULL == iAP2Transport->iAP2TransportHdl)
                {
                    rc = IAP2_BAD_PARAMETER;
                }
                break;
            }
            case iAP2MULTIHOSTMODE:
            {
                iAP2Transport->iAP2TransportHdl = iPodiAP2USBMultiHostComInit(&(iAP2Transport->iAP2DataComFunction));
                if(NULL == iAP2Transport->iAP2TransportHdl)
                {
                    rc = IAP2_BAD_PARAMETER;
                }
                    break;
            }
            case iAP2USBDEVICEMODE:
            {
                iAP2Transport->iAP2TransportHdl = iPodiAP2USBDeviceComInit(&(iAP2Transport->iAP2DataComFunction));
                if(NULL == iAP2Transport->iAP2TransportHdl)
                {
                    rc = IAP2_BAD_PARAMETER;
                }

                break;
            }
            case iAP2UART:
            {
                /** Disabled as plug-in not available

                iAP2Transport->iAP2TransportHdl = iPodiAP2UARTComInit(&(iAP2Transport->iAP2DataComFunction));
                if(NULL != iAP2Transport->iAP2TransportHdl)
                {
                    rc = IAP2_OK;
                }
                */
                break;

            }
            case iAP2BLUETOOTH:
            {
                iAP2Transport->iAP2TransportHdl = iPodiAP2BTComInit(&(iAP2Transport->iAP2DataComFunction));
                if(NULL == iAP2Transport->iAP2TransportHdl)
                {
                    rc = IAP2_BAD_PARAMETER;
                }

                break;
            }
            case iAP2OVERCARPLAY:
            {
                iAP2Transport->iAP2TransportHdl = iAP2OverCarPlayComInit(&(iAP2Transport->iAP2DataComFunction));
                if(NULL == iAP2Transport->iAP2TransportHdl)
                {
                    rc = IAP2_BAD_PARAMETER;
                    IAP2TRANSPORTDLTLOG(DLT_LOG_ERROR,"iAP2OverCarPlayComInit() failed rc:%d DevID:%p",rc, iap2Device);
                }

                break;
            }
            default:
            {
                /* Plugins are not available for this transport */
                IAP2TRANSPORTDLTLOG(DLT_LOG_ERROR,"Unknown transport type:%d DevID:%p",
                                    iAP2Transport->iAP2TransportType, iap2Device);
                rc = IAP2_BAD_PARAMETER;

                break ;
            }
        }
    }

    if(rc != IAP2_OK)
    {
        IAP2TRANSPORTDLTLOG(DLT_LOG_ERROR,"FAILED. rc:%d DevID:%p",rc, iap2Device);
    }
    return rc;
}


S32 iAP2TransportDeInit(iAP2Transport_t *iAP2Transport)
{
    S32 rc = IAP2_CTL_ERROR;

    /* De-Initialize transport Plugins */
    switch(iAP2Transport->iAP2TransportType)
    {
        case iAP2USBHOSTMODE:
        {
            rc = iPodiAP2USBHostComDeinit(&(iAP2Transport->iAP2DataComFunction),iAP2Transport->iAP2TransportHdl);
            break;
        }
        case iAP2USBDEVICEMODE:
        {
            rc = iPodiAP2USBDeviceComDeinit(&(iAP2Transport->iAP2DataComFunction),iAP2Transport->iAP2TransportHdl);

            break;
        }
        case iAP2MULTIHOSTMODE:
        {
            rc = iPodiAP2USBMultiHostComDeinit(&(iAP2Transport->iAP2DataComFunction),iAP2Transport->iAP2TransportHdl);
            break;
        }
        case iAP2UART:
        {
            /** Disabled as plug-in not available
            rc=iPodiAP2UARTComDeinit(&(iAP2Transport->iAP2DataComFunction),iAP2Transport->iAP2TransportHdl);
            */

            break;
        }
        case iAP2BLUETOOTH:
        {
            rc = iPodiAP2BTComDeinit(&(iAP2Transport->iAP2DataComFunction),iAP2Transport->iAP2TransportHdl);

            break;
        }
        case iAP2OVERCARPLAY:
        {
            rc = iAP2OverCarPlayComDeinit(&(iAP2Transport->iAP2DataComFunction),iAP2Transport->iAP2TransportHdl);

            break;
        }
        default:
        {
            /* Plugins are not available for this transport */
            IAP2TRANSPORTDLTLOG(DLT_LOG_ERROR,"Unknown transport type %d",iAP2Transport->iAP2TransportType);
            rc = IAP2_BAD_PARAMETER;
            break;
        }
    }
    if(IAP2_OK != rc)
    {
        IAP2TRANSPORTDLTLOG(DLT_LOG_ERROR,"DeInit transport plug-in failed. transport type: %d. rc:%d",
                            iAP2Transport->iAP2TransportType, rc);
    }

    rc = iAP2FreeTransportConfig(iAP2Transport);
    if(IAP2_OK != rc)
    {
        IAP2TRANSPORTDLTLOG(DLT_LOG_ERROR,"iAP2FreeTransportConfig failed. rc:%d",rc);
    }
    return rc;
}

S32 iAP2OverCarPlayWrite(iAP2Transport_t* transport, U8* writeBuff, U16 writeLen)
{
    S32 res                   = IAP2_BAD_PARAMETER;

    if (NULL == transport || NULL == writeBuff)
    {
        IAP2TRANSPORTDLTLOG(DLT_LOG_ERROR,"Invalid input parameter");
        return IAP2_BAD_PARAMETER;
    }

    if(transport->iAP2DataComFunction.write == NULL)
    {
        IAP2TRANSPORTDLTLOG(DLT_LOG_ERROR, "Invalid write() function" );
        return IAP2_BAD_PARAMETER;
    }

    res = transport->iAP2DataComFunction.write(transport->iAP2TransportHdl,
                                               writeLen,
                                               writeBuff,
                                               0);
    if(res == IPOD_DATACOM_NOT_CONNECTED)
    {
        IAP2TRANSPORTDLTLOG(DLT_LOG_INFO,"iAP2DataComFunction.write return IPOD_DATACOM_NOT_CONNECTED");
        res = IAP2_DEV_NOT_CONNECTED;
    }
    else if (res == IPOD_DATACOM_ERR_ABORT)
    {
        IAP2TRANSPORTDLTLOG(DLT_LOG_INFO,"iAP2DataComFunction.write return IPOD_DATACOM_ERR_ABORT");
        res = IAP2_COMM_ERROR_SEND;
    }
    else if(0 > res)
    {
        IAP2TRANSPORTDLTLOG(DLT_LOG_INFO,"iAP2DataComFunction.write failed %d", res);
        res = IAP2_CTL_ERROR;
    }
    else if((U32)res == writeLen)
    {
        /* write all data completed */
        res = IAP2_OK;
    }
    else
    {
        /* could not write all data */
        IAP2TRANSPORTDLTLOG(DLT_LOG_ERROR,"iAP2DataComFunction.write could not write all data %d", res);
        res = IAP2_CTL_ERROR;
    }

    return res;
}

S32 iAP2UsbHostWrite(iAP2Transport_t* transport, U8* writeBuff, U16 writeLen)
{
    S32 res                   = IAP2_BAD_PARAMETER;

    if (NULL == transport || NULL == writeBuff)
    {
        IAP2TRANSPORTDLTLOG(DLT_LOG_ERROR,"Invalid input parameter");
        return IAP2_BAD_PARAMETER;
    }

    if(transport->iAP2DataComFunction.write == NULL)
    {
        IAP2TRANSPORTDLTLOG(DLT_LOG_ERROR, "Invalid write() function" );
        return IAP2_BAD_PARAMETER;
    }

    res = transport->iAP2DataComFunction.write(transport->iAP2TransportHdl,
                                               writeLen,
                                               writeBuff,
                                               0);
    if(res == IPOD_DATACOM_NOT_CONNECTED)
    {
        IAP2TRANSPORTDLTLOG(DLT_LOG_INFO,"iAP2DataComFunction.write return IPOD_DATACOM_NOT_CONNECTED");
        res = IAP2_DEV_NOT_CONNECTED;
    }
    else if (res == IPOD_DATACOM_ERR_ABORT)
    {
        IAP2TRANSPORTDLTLOG(DLT_LOG_INFO,"iAP2DataComFunction.write return IPOD_DATACOM_ERR_ABORT");
        res = IAP2_COMM_ERROR_SEND;
    }
    else if(0 > res)
    {
        IAP2TRANSPORTDLTLOG(DLT_LOG_INFO,"iAP2DataComFunction.write failed %d", res);
        res = IAP2_CTL_ERROR;
    }
    else if((U32)res == writeLen)
    {
        /* write all data completed */
        res = IAP2_OK;
    }
    else
    {
        /* could not write all data */
        IAP2TRANSPORTDLTLOG(DLT_LOG_ERROR,"iAP2DataComFunction.write could not write all data %d", res);
        res = IAP2_CTL_ERROR;
    }

    return res;
}

S32 iAP2UsbDeviceWrite(iAP2Transport_t* transport, U8* writeBuff, U16 writeLen)
{
    U32 i                     = 0;
    S32 res                   = IAP2_BAD_PARAMETER;
    U32 maxLen                = 0;
    U32 dataLen               = 0;
    U32 totalLen              = 0;
    U32 numFullReports        = 0;
    U32 hasPartlyFilledReport = TRUE;
    U8* msg                  = NULL;

    IPOD_IAP2_DATACOM_PROPERTY usbProperty = {0};

    dataLen         = writeLen;
    totalLen        = writeLen;

    if (NULL == transport || NULL == writeBuff)
    {
        IAP2TRANSPORTDLTLOG(DLT_LOG_ERROR,"Invalid input");
        return IAP2_BAD_PARAMETER;
    }

    if((transport->iAP2DataComFunction.write == NULL)
            || (transport->iAP2DataComFunction.property == NULL))
    {
        IAP2TRANSPORTDLTLOG(DLT_LOG_ERROR,"Invalid functions:write or property");
        return IAP2_BAD_PARAMETER;
    }

    res = transport->iAP2DataComFunction.property(transport->iAP2TransportHdl,
                                                  &usbProperty);
    if (IAP2_OK == res)
    {
        maxLen = usbProperty.maxSize;
        /* maxLen - IPOD_USB_LINK_CTRL_BYTE_SIZE because one
           byte of the report must be used for the Link Control
           Byte */
        if(maxLen > IPOD_USB_LINK_CTRL_BYTE_SIZE)
        {
            numFullReports = ((U16)totalLen / (maxLen - IPOD_USB_LINK_CTRL_BYTE_SIZE));

            if ((totalLen % (maxLen - IPOD_USB_LINK_CTRL_BYTE_SIZE)) == 0)
            {
                hasPartlyFilledReport = FALSE;
            }

            msg = (U8 *)calloc(maxLen + IPOD_USB_HEADER_SIZE, sizeof(U8));
            if (msg != NULL)
            {
                /*
                 * This loop is counted by i and maximum size is
                 * numFullReports(U16). Therefore, This loop never occur
                 * infinite loop.
                 */
                for (i = 0; (((i < numFullReports )
                             && (hasPartlyFilledReport == FALSE))
                             || ((i <= numFullReports) && (hasPartlyFilledReport != FALSE)))
                             && (res == IAP2_OK); i++)
                {
                    memset(msg, 0, maxLen + IPOD_USB_HEADER_SIZE);

                    /* check special cases */
                    if ((i == 0) && (numFullReports >= 1))
                    {
                        /* link control byte ??? */
                        msg[IPOD_POS0] = IPOD_LINK_CTRL_FRST_FOLLOW;
                    }
                    else if ((i >= 1) && (numFullReports > i))
                    {
                        /* link control byte ???    */
                        msg[IPOD_POS0] = IPOD_LINK_CTRL_MIDDLE;
                    }
                    else if ((i >= 1) && (i == numFullReports))
                    {
                        /* link control byte ???    */
                        msg[IPOD_POS0] = IPOD_LINK_CTRL_LAST;
                    }
                    else
                    {
                        msg[IPOD_POS0]  =  IPOD_LINK_CTRL_ONLY_ONE;
                    }

                    if ((i == (numFullReports - 1))
                            && (hasPartlyFilledReport == FALSE))
                    {
                        msg[IPOD_POS0] = IPOD_LINK_CTRL_LAST;
                    }

                    if((i == 0)
                            && (numFullReports == 1)
                            && (hasPartlyFilledReport == FALSE))
                    {
                        msg[IPOD_POS0] = IPOD_LINK_CTRL_ONLY_ONE;
                    }

                    /* If data length is bigger than maxLen,
                     * it should be set maxLen,
                     * otherwise should be set data length
                     */
                    if(dataLen > (maxLen - IPOD_USB_LINK_CTRL_BYTE_SIZE))
                    {
                        writeLen = maxLen;
                        dataLen  = dataLen - (maxLen - IPOD_USB_LINK_CTRL_BYTE_SIZE);
                    }
                    else
                    {
                        writeLen = dataLen + IPOD_USB_LINK_CTRL_BYTE_SIZE;
                    }

                    if(maxLen >= writeLen)
                    {
                        /*
                         * TODO: why memcpy instead of pointer to
                         * iPod_msg[ xxx ] ?
                         */
                        memcpy(&msg[IPOD_POS1],
                               &writeBuff[i * (maxLen - IPOD_USB_LINK_CTRL_BYTE_SIZE)],
                               (writeLen - IPOD_USB_LINK_CTRL_BYTE_SIZE));
                        res = transport->iAP2DataComFunction.write(transport->iAP2TransportHdl,
                                                                   writeLen,
                                                                   msg,
                                                                   0);
                        if(res == IPOD_DATACOM_NOT_CONNECTED)
                        {
                            IAP2TRANSPORTDLTLOG(DLT_LOG_INFO,"iAP2DataComFunction.write return IPOD_DATACOM_NOT_CONNECTED");
                            res = IAP2_DEV_NOT_CONNECTED;
                        }
                        else if(res == IPOD_DATACOM_ERR_ABORT)
                        {
                            IAP2TRANSPORTDLTLOG(DLT_LOG_INFO,"iAP2DataComFunction.write return IPOD_DATACOM_ERR_ABORT");
                            res = IAP2_COMM_ERROR_SEND;
                        }
                        else if(0 > res)
                        {
                            IAP2TRANSPORTDLTLOG(DLT_LOG_INFO,"iAP2DataComFunction.write failed %d", res);
                            res = IAP2_CTL_ERROR;
                        }
                        else if((U32)res == writeLen)
                        {
                            /* write all data completed */
                            res = IAP2_OK;
                        }
                        else
                        {
                            /* could not write all data */
                            IAP2TRANSPORTDLTLOG(DLT_LOG_ERROR,"iAP2DataComFunction.write could not write all data %d", res);
                            res = IAP2_CTL_ERROR;
                        }
                    }
                    else
                    {
                        IAP2TRANSPORTDLTLOG(DLT_LOG_ERROR,"IPOD_DATACOM_ERR_NOMEM");
                        res = IAP2_ERR_NO_MEM;
                    }
                }
                free(msg);
            }
            else
            {
                IAP2TRANSPORTDLTLOG(DLT_LOG_ERROR,"IPOD_DATACOM_ERR_NOMEM");
                res = IAP2_ERR_NO_MEM;
            }
        }
        else
        {
            IAP2TRANSPORTDLTLOG(DLT_LOG_ERROR,"IPOD_DATACOM_ERR_NOMEM");
            res = IAP2_CTL_ERROR;
        }
    }
    else
    {
        IAP2TRANSPORTDLTLOG(DLT_LOG_ERROR,"IPOD_DATACOM_ERR_NOMEM");
        res = IAP2_ERR_NO_MEM;
    }

    return res;
}

S32 iAP2UsbMultiHostWrite(iAP2Transport_t* transport, U8* writeBuff, U16 writeLen)
{
    S32 res                   = IAP2_BAD_PARAMETER;

    if (NULL == transport || NULL == writeBuff)
    {
        IAP2TRANSPORTDLTLOG(DLT_LOG_ERROR,"Invalid input parameter");
        return IAP2_BAD_PARAMETER;
    }

    if(transport->iAP2DataComFunction.write == NULL)
    {
        IAP2TRANSPORTDLTLOG(DLT_LOG_ERROR, "Invalid write() function" );
        return IAP2_BAD_PARAMETER;
    }

    res = transport->iAP2DataComFunction.write(transport->iAP2TransportHdl,
                                               writeLen,
                                               writeBuff,
                                               0);
    if(res == IPOD_DATACOM_NOT_CONNECTED)
    {
        IAP2TRANSPORTDLTLOG(DLT_LOG_INFO,"iAP2DataComFunction.write return IPOD_DATACOM_NOT_CONNECTED");
        res = IAP2_DEV_NOT_CONNECTED;
    }
    else if (res == IPOD_DATACOM_ERR_ABORT)
    {
        IAP2TRANSPORTDLTLOG(DLT_LOG_INFO,"iAP2DataComFunction.write return IPOD_DATACOM_ERR_ABORT");
        res = IAP2_COMM_ERROR_SEND;
    }
    else if(0 > res)
    {
        IAP2TRANSPORTDLTLOG(DLT_LOG_INFO,"iAP2DataComFunction.write failed %d", res);
        res = IAP2_CTL_ERROR;
    }
    else if((U32)res == writeLen)
    {
        /* write all data completed */
        res = IAP2_OK;
        IAP2TRANSPORTDLTLOG(DLT_LOG_INFO,"iAP2DataComFunction.write success %d written %d bytes", res,writeLen);

    }
    else
    {
        /* could not write all data */
        IAP2TRANSPORTDLTLOG(DLT_LOG_ERROR,"iAP2DataComFunction.write could not write all data %d", res);
        res = IAP2_CTL_ERROR;
    }

    return res;
}

S32 iAP2DeviceWrite(iAP2Transport_t* transport, U8* writeBuff, U16 writeLen)
{
    S32 rc = IAP2_CTL_ERROR;

    switch(transport->iAP2TransportType)
    {
        case iAP2USBHOSTMODE:
        {
            rc = iAP2UsbHostWrite(transport, writeBuff, writeLen);
            break;
        }
        case iAP2USBDEVICEMODE:
        {
            rc = iAP2UsbDeviceWrite(transport, writeBuff, writeLen);
            break;
        }
        case iAP2MULTIHOSTMODE:
        {
            rc = iAP2UsbMultiHostWrite(transport, writeBuff, writeLen);
            break;
        }
        case iAP2UART:
        {
            /*
             * TODO: rc = iAP2SerialWrite(iAP2Transport_t* transport,
             *                            U8*              writeBuff,
             *                            U16              writeLen);
             */

            break;
        }
        case iAP2BLUETOOTH:
        {
            rc = iAP2UsbHostWrite(transport, writeBuff, writeLen);
            break;
        }
        case iAP2OVERCARPLAY:
        {
            rc = iAP2OverCarPlayWrite(transport, writeBuff, writeLen);
            break;
        }
        default:
        {
            /* oops: got into wrong transport */
            break;
        }
    }

    return rc;

}


/*!
 * \fn S32 iAP2DeviceClose(iAP2Transport_t* iap2Transport)
 * \par INPUT PARAMETERS
 * iAP2Transport_t* iap2Transport - transport connection details for a
 * specific device
  * \par REPLY PARAMETERS
 * S32 ReturnCode -
 * \li \c \b #IAP2_OK  Completed successfully
 * \li \c \b #IAP2_CTL_ERROR  close transport connection failed
 * \li \c \b #IAP2_BAD_PARAMETER  invalid parameter or NULL pointer
 * \par DESCRIPTION
 * This function is used to close the transport connection and the used driver.
 */
S32 iAP2DeviceClose(iAP2Transport_t* transport)
{
    S32 rc = IAP2_CTL_ERROR;

    if(NULL != transport)
    {
        rc = transport->iAP2DataComFunction.abort(transport->iAP2TransportHdl);
        rc = transport->iAP2DataComFunction.close(transport->iAP2TransportHdl);

        if(rc == IAP2_OK)
        {
            IAP2TRANSPORTDLTLOG(DLT_LOG_INFO,"Device Close (iAP2TransportType  %d) Success ! ", transport->iAP2TransportType);
        }
        else
        {
            IAP2TRANSPORTDLTLOG(DLT_LOG_ERROR,"Device Close (iAP2TransportType  %d) failed. rc:%d", transport->iAP2TransportType, rc);
        }
    }
    else
    {
        rc = IAP2_BAD_PARAMETER;
    }

    return rc;
}

/*!
 * \fn S32 iAP2DeviceOpen(iAP2Transport_t* iap2Transport)
 * \par INPUT PARAMETERS
 * iAP2Transport_t* iap2Transport - transport connection details for a
 * specific device
 * \par OUTPUT PARAMETERS
 * S32 iAP2TransportFid - file descriptor.This value equal to or greater than 0.
  * \par REPLY PARAMETERS
 * S32 ReturnCode -
 * \li \c \b #IAP2_OK  Completed successfully
 * \li \c \b #IAP2_CTL_ERROR  Open transport connection failed
 * \li \c \b #IAP2_BAD_PARAMETER  invalid parameter or NULL pointer
 * \li \c \b #IAP2_ERR_USB_ROLE_SWITCH_UNSUP  Apple device does not support USB
 *            role switch
 * \li \c \b #IAP2_ERR_USB_ROLE_SWITCH_FAILED  USB role switch failed
 * \par DESCRIPTION
 * This function is used for open the transport connection and the driver.
 */
S32 iAP2DeviceOpen(iAP2Transport_t* iap2Transport)
{

    S32 rc = IAP2_CTL_ERROR;

    if(NULL != iap2Transport)
    {
        /*
         * Do we need a switch here for transport type ??
         * Open the attached device
         *
         * Device name(serial number) should be provided by app .it should be a
         * part of device or transport structure.
         *
         * Assume iAP2DataComFunction is already initialized depending on the
         * transport type
         */
        rc = iap2Transport->iAP2DataComFunction.open(iap2Transport->iAP2TransportHdl, /*iAP2TransportHdl is an out parameter*/
                                                      (const U8*)iap2Transport->iAP2DeviceIdentifier,
                                                      IPOD_DATACOM_FLAGS_RDWR,
                                                      1 );
        if(rc == IAP2_OK)
        {
            IAP2TRANSPORTDLTLOG(DLT_LOG_INFO,"Device Open (iAP2TransportType  %d) Success ! ", iap2Transport->iAP2TransportType);
        }
        else
        {
            if(IPOD_DATACOM_ERR_USB_ROLE_SWITCH_UNSUP  == rc)
            {
                IAP2TRANSPORTDLTLOG(DLT_LOG_ERROR,"Apple device does not support USB Host mode");
            }
            else if(IPOD_DATACOM_ERR_USB_ROLE_SWITCH_FAILED  == rc)
            {
                IAP2TRANSPORTDLTLOG(DLT_LOG_ERROR,"Switch to USB Host mode failed");
            }
            else
            {
                IAP2TRANSPORTDLTLOG(DLT_LOG_ERROR,"Device open (iAP2TransportType  %d) failed. rc:%d", iap2Transport->iAP2TransportType, rc);
                rc = IAP2_CTL_ERROR;
            }
        }
    }
    else
    {
        rc = IAP2_BAD_PARAMETER;
    }

    return rc;
}


/*!
 * \fn S32 iAP2DeviceIoCtl(iAP2Transport_t* iap2Transport)
 * \par INPUT PARAMETERS
 * iAP2Transport_t* iap2Transport - transport connection details for a specific device
  * \par REPLY PARAMETERS
 * S32 ReturnCode -
 * \li \c \b #IAP2_OK  Completed successfully
 * \li \c \b #IAP2_CTL_ERROR  I/O ctl transport connection failed
 * \li \c \b #IAP2_BAD_PARAMETER  invalid parameter or NULL pointer
 * \par DESCRIPTION
 * This function is used for configure (currently, only for USB Host Mode Plug-in)
 * the transport connection and the driver.
 */
 S32 iAP2DeviceIoCtl(iAP2Transport_t* iap2Transport)
 {
     S32 rc = IAP2_OK;

     if(NULL != iap2Transport)
     {
         switch(iap2Transport->iAP2TransportType)
         {
             case iAP2USBHOSTMODE:
             {
                 rc = iap2Transport->iAP2DataComFunction.ioctl(iap2Transport->iAP2TransportHdl,
                                                               IPOD_IAP2_DATACOM_IOCTL_SET_CONFIG,
                                                               &(iap2Transport->iAP2IoCtlConfig));
                 if((rc == IAP2_OK) && (iap2Transport->iAP2IoCtlConfig.nativeTransport == TRUE))
                 {
                     rc = iap2Transport->iAP2DataComFunction.ioctl(iap2Transport->iAP2TransportHdl,
                                                               IPOD_IAP2_DATACOM_IOCTL_SET_CB,
                                                               &(iap2Transport->iAP2EAnativetransport_cb));
                 }
                 break;
             }
             case iAP2USBDEVICEMODE:
             {
                 /*
                  * TODO: Currently not used at USB Device Mode Plug-in.
                  */
                 break;
             }
             case iAP2MULTIHOSTMODE:
             {
                 rc = iap2Transport->iAP2DataComFunction.ioctl(iap2Transport->iAP2TransportHdl,
                                                               IPOD_IAP2_DATACOM_IOCTL_SET_CONFIG,
                                                               &(iap2Transport->iAP2IoCtlConfig));
                 IAP2TRANSPORTDLTLOG(DLT_LOG_DEBUG,"Device ioctl set config (iAP2TransportType  %d) returned. rc:%d", iap2Transport->iAP2TransportType, rc);
                 if((rc == IAP2_OK) && (iap2Transport->iAP2IoCtlConfig.nativeTransport == TRUE))
                 {
                     rc = iap2Transport->iAP2DataComFunction.ioctl(iap2Transport->iAP2TransportHdl,
                                                               IPOD_IAP2_DATACOM_IOCTL_SET_CB,
                                                               &(iap2Transport->iAP2EAnativetransport_cb));
                 }
                 break;
             }
             case iAP2UART:
             {
                 /*
                  * TODO: Currently not used at UART Plug-in.
                  */

                 break;
             }
             case iAP2BLUETOOTH:
             {
                 /*
                  * TODO: Currently not used at BT Plug-in.
                  */
                 break;
             }
             case iAP2OVERCARPLAY:
             {
                 /*
                  * TODO: Currently not used for iAP2 Over Carplay Plug-in.
                  */
                 break;
             }
             default:
             {
                 rc = IAP2_CTL_ERROR;
                 break;
             }
         }
     }
     else
     {
         rc = IAP2_BAD_PARAMETER;
     }

     return rc;
 }
