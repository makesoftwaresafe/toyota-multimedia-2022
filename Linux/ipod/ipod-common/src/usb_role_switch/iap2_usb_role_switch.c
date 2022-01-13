/**
* \file: iap2_usb_role_switch.cpp
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

#include "iap2_usb_role_switch.h"
#include "iap2_usb_vendor_request.h"
#include "iap2_usb_power.h"
#include "iap2_usb_otg_switch.h"
#include "iap2_usb_udc.h"
#include "iap2_defines.h"
#include <stdio.h>
#include <unistd.h>
#include <glob.h>

/* **********************  locals    ********************** */

LOCAL S32  iap2SwitchOTG(const char* otgGlob, const char* value);

#ifdef IPOD_ARCH_ARM
LOCAL S32  iap2SwitchVbusAuto(const char* otgPath, const char* value);
#endif /* IPOD_ARCH_ARM */

S32  iap2SwitchToHostMode(iAP2USBRoleSwitchInfo* info, udcParamInfo_t* pUdcParam)
{
    S32 rc = IAP2_OK;
    iAP2USBRoleSwitchStatus usb_status = IAP2_USB_ROLE_SWITCH_OK;
    iAP2USBVendorRequestMonitor monitor;

    usbConnectStateType_t type = NOT_CONNECTED;

    /* create udev monitor to detect disconnect of the iOS device */
    rc = iAP2USBVendorRequestMonitor_Begin(&monitor, info->vendorId, info->productId, info->serialNumber, FALSE);
    if(rc != IAP2_OK){
        IAP2USBROLESWITCHDLTLOG(DLT_LOG_WARN, " iAP2USBVendorRequestMonitor_Begin  = %d",rc);
    } else{
        type = getConnectStateType(&monitor.deviceInfo.sysPath[0], pUdcParam);
        if (type == CON_ERROR){
            IAP2USBROLESWITCHDLTLOG(DLT_LOG_ERROR, "Error in getting the connection type");
            rc = IAP2_CTL_ERROR;
        }
        else
        {
           /* send vendor request to iOS device */
            usb_status = iAP2USBVendorRequest_Send(info->vendorId, info->productId, info->serialNumber, info->mode);
            IAP2USBROLESWITCHDLTLOG(DLT_LOG_INFO, " Send Vendor request to iOS device  = %d", usb_status);

            if(IAP2_USB_ROLE_SWITCH_VENDOR_REQUEST_NOT_SUPPORTED == usb_status){
                rc = IAP2_ERR_USB_ROLE_SWITCH_UNSUP;
            } else if(IAP2_USB_ROLE_SWITCH_VENDOR_REQUEST_FAILED == usb_status){
                rc = IAP2_ERR_USB_ROLE_SWITCH_FAILED;
            } else if(IAP2_USB_ROLE_SWITCH_OK != usb_status){
                rc = IAP2_CTL_ERROR;
            } else{
                rc = IAP2_OK;
            }
        }
    }

    if(rc == IAP2_OK){
        /* use the udev monitor to detect the disconnect of the iOS device */
        rc = iAP2USBVendorRequestMonitor_WaitAndEnd(&monitor);
        if(rc != IAP2_OK){
            IAP2USBROLESWITCHDLTLOG(DLT_LOG_WARN, " iap2VendorRequestMonitor_WaitAndEnd  = %d",rc);
        }
    }
    /* release the udev monitor */
    iAP2USBVendorRequestMonitor_Release(&monitor);

    if(rc == IAP2_OK){
        switch(type)
        {
            case OTG_CONNECTED:
            {
#ifdef IPOD_ARCH_ARM
                if (TRUE != setUdcForce(pUdcParam->pUdcDevice))
                {
                    IAP2USBROLESWITCHDLTLOG(DLT_LOG_ERROR, "iap2SwitchOTG():  set %s to udc_force failed", pUdcParam->pUdcDevice);
                    rc = IAP2_CTL_ERROR;
                    break;
                }
#endif /* #ifdef IPOD_ARCH_ARM */

                /* switch the USB OTG port to 'gadget' */
                rc = iap2SwitchOTG(info->otgGlob, IAP2_USB_OTG_GADGET);
                IAP2USBROLESWITCHDLTLOG(DLT_LOG_INFO, " iap2SwitchOTG(%s)  = %d ", IAP2_USB_OTG_GADGET, rc);

                if(rc == IAP2_OK){
                   /* turn on VBUS power */
                    rc = iAP2USBPower_Switch(info->vbusPower, "on");
                    IAP2USBROLESWITCHDLTLOG(DLT_LOG_INFO, " iap2SwitchVbusPower(on)  = %d ",rc);
                }

                break;
            }
            case UNWIRED_HUB_CONNECTED:
            {
                rc = udcSwitchToHostMode(pUdcParam);
                IAP2USBROLESWITCHDLTLOG(DLT_LOG_INFO, " udcSwitchToHostMode() = %d | portNum:  %s", rc, pUdcParam->pDevicePortNum);
                break;
            }
            default:
            {
                IAP2USBROLESWITCHDLTLOG(DLT_LOG_ERROR, "  Not connected or unknown ConnectStateType  %d", type);
                rc = IAP2_CTL_ERROR;
                break;
            }
        }
    }

    IAP2USBROLESWITCHDLTLOG(DLT_LOG_INFO, " iap2SwitchToHostMode() for %s = %d",info->serialNumber, rc);
    return rc;
}

/***************************************************************************//**
 * S32  iap2SwitchToMultiHostMode(iAP2USBRoleSwitchInfo* info)
 *
 * Performs role switch operation of Apple device.
 * Then enables multi-host session mode for the Molex Hub by sending vendor request command to HFC
 *
 * \param[in-out]  info - Role switch information structure passed by application.
 * Only Port number is filled here and it is an out parameter which should be retained by application to pass the same to TerminateMultiHostMode function
 * Because once apple device switches to Host mode, it is not possible to get this port number. This is required to send disable multi host command to the Molex Hub.
 * Other members of this structure are filled by application
 * NOTE: For Multi-Host mode, info.otgGlob,info.powerGPIO,info.vbusPower are not used so need not be filled
 * \param[in]  pUdcParam - Not relevant to this function. Just interface is retained.
 *
 * \return Returns a signed integer value indicating Zero on success and less than Zero on failure
 *
 ******************************************************************************/

S32  iap2SwitchToMultiHostMode(iAP2USBRoleSwitchInfo* info, udcParamInfo_t* pUdcParam)
{
    S32 rc = IAP2_CTL_ERROR;
    iAP2USBRoleSwitchStatus usb_status = IAP2_USB_ROLE_SWITCH_OK;
    iAP2USBVendorRequestMonitor monitor;
    pUdcParam = pUdcParam;
    libusb_context *usb_context = NULL;
    libusb_device **list;
    S32 i=0,num_devices = IAP2_OK;
    libusb_device_handle *device_handle_hub = NULL, *device_handle_ios = NULL;
    unsigned short wvalue = IAP2_OK;
    struct libusb_device_descriptor device_descriptor;
    libusb_device *device_ios;
    BOOL MatchFound = 0;
    char stringDescription[STR_MAX_LENGTH];

    rc = libusb_init(&usb_context);
    if((rc < IAP2_OK) || (usb_context == NULL))
    {
        IAP2USBROLESWITCHDLTLOG(DLT_LOG_ERROR, "libusb init failed %d(%s)",rc,libusb_error_name(rc));
        return rc;
    }
    num_devices = libusb_get_device_list(usb_context,&list);
    if((num_devices < IAP2_OK))
    {
        IAP2USBROLESWITCHDLTLOG(DLT_LOG_ERROR, " libusb_get_device_list failed %d(%s)",rc,libusb_error_name(rc));
        return rc;
    }

    for(i=0; i<num_devices ;i++)
    {
        rc = libusb_get_device_descriptor(list[i], &device_descriptor);
        if((rc < IAP2_OK))
        {
            IAP2USBROLESWITCHDLTLOG(DLT_LOG_ERROR, " libusb_get_device_descriptor failed %d(%s)",rc,libusb_error_name(rc));
            return rc;
        }
        if((device_descriptor.idVendor == MOLEX_HUB_VID) && ( (device_descriptor.idProduct & MOLEX_HUB_PID) == MOLEX_HUB_PID))
        {
           rc = libusb_open(list[i],&device_handle_hub);
        }
        else if((device_descriptor.idVendor == APPLE_DEV_VID) && (device_descriptor.idProduct == APPLE_DEV_PID)
               && (MatchFound==FALSE))
        {
            rc = libusb_open(list[i],&device_handle_ios);
            if((rc < IAP2_OK))
            {
                IAP2USBROLESWITCHDLTLOG(DLT_LOG_ERROR, " libusb_open failed %d(%s)",rc,libusb_error_name(rc));
                return rc;
            }
            if(NULL != info->serialNumber)
            {
                rc = libusb_get_string_descriptor_ascii( device_handle_ios, device_descriptor.iSerialNumber, stringDescription, 256 );
                if((rc < IAP2_OK))
                {
                    IAP2USBROLESWITCHDLTLOG(DLT_LOG_ERROR, " libusb_get_string_descriptor_ascii failed %d(%s)",rc,libusb_error_name(rc));
                    return rc;
                }
                if(0 == strcmp(stringDescription,info->serialNumber))
                {
                    MatchFound = TRUE;
                    IAP2USBROLESWITCHDLTLOG(DLT_LOG_INFO, " matching device found ",stringDescription, info->serialNumber);
                }
                else
                {
                    libusb_close(device_handle_ios);
                    device_handle_ios = NULL;
                    IAP2USBROLESWITCHDLTLOG(DLT_LOG_INFO, " device found with no match ",stringDescription, info->serialNumber);
                }
            }
         }
     }

    if((rc >= IAP2_OK) && (device_handle_ios != NULL) && (device_handle_hub!=NULL))
    {
        /*Detect the Port Number and frame the command */
        device_ios = libusb_get_device(device_handle_ios);
        info->portnumber = libusb_get_port_number(device_ios);

        if(info->mode == IAP2_USB_ROLE_SWITCH_WITH_EA)
        {
            wvalue = WVALUE_CMD | (info->portnumber) | (SWITCH_ENABLE << 4) | (EA_ENABLE << 7) | ENUM_TIMEOUT << 8 ;
        }
        else
        {
            wvalue = WVALUE_CMD | (info->portnumber) | (SWITCH_ENABLE << 4) | ENUM_TIMEOUT << 8;
        }

        IAP2USBROLESWITCHDLTLOG(DLT_LOG_INFO, " Port number in which apple device is connected = %d and command is %x ",info->portnumber,wvalue);
    }
    else
    {
        IAP2USBROLESWITCHDLTLOG(DLT_LOG_ERROR, "Device handle of iOS device %d Hub %d  rc %d ",device_handle_ios,device_handle_hub, rc);
        return IAP2_CTL_ERROR;
    }

    /* create udev monitor to detect disconnect of the iOS device */
    rc = iAP2USBVendorRequestMonitor_Begin(&monitor, info->vendorId, info->productId, info->serialNumber, FALSE);
    if(rc != IAP2_OK)
    {
        IAP2USBROLESWITCHDLTLOG(DLT_LOG_WARN, " iAP2USBVendorRequestMonitor_Begin  = %d",rc);
    }
    else
    {
        /* send vendor request to iOS device */
        usb_status = iAP2USBVendorRequest_Send(info->vendorId, info->productId, info->serialNumber, info->mode);
        IAP2USBROLESWITCHDLTLOG(DLT_LOG_INFO, " Send Vendor request to iOS device  = %d", usb_status);

        if(IAP2_USB_ROLE_SWITCH_VENDOR_REQUEST_NOT_SUPPORTED == usb_status)
        {
            rc = IAP2_ERR_USB_ROLE_SWITCH_UNSUP;
        }
        else if(IAP2_USB_ROLE_SWITCH_VENDOR_REQUEST_FAILED == usb_status)
        {
            rc = IAP2_ERR_USB_ROLE_SWITCH_FAILED;
        }
        else if(IAP2_USB_ROLE_SWITCH_OK != usb_status)
        {
            rc = IAP2_CTL_ERROR;
        }
        else
        {
            rc = IAP2_OK;
        }
    }

    if(rc == IAP2_OK)
    {
        /* use the udev monitor to detect the disconnect of the iOS device */
        rc = iAP2USBVendorRequestMonitor_WaitAndEnd(&monitor);
        if(rc != IAP2_OK)
        {
            IAP2USBROLESWITCHDLTLOG(DLT_LOG_WARN, " iap2VendorRequestMonitor_WaitAndEnd  = %d",rc);
        }
    }
    /* release the udev monitor */
    iAP2USBVendorRequestMonitor_Release(&monitor);


    /* create udev monitor to detect MultiHostHub */
    rc = iAP2USBVendorRequestMonitor_Begin(&monitor, MOLEX_HUB_VID, MOLEX_MHR_PID, NULL, TRUE);
    if(rc != IAP2_OK)
    {
        IAP2USBROLESWITCHDLTLOG(DLT_LOG_WARN, " iAP2USBMultiHostHubMonitor_Begin  = %d",rc);
    }
    else
    {
        rc = libusb_control_transfer(device_handle_hub,VENDOR_CLASS_REQ,SET_ROLE_SWITCH,wvalue,0x0000,NULL,0x00,LIBUSB_CONTROL_TIMEOUT);
        IAP2USBROLESWITCHDLTLOG(DLT_LOG_INFO, " iap2MultiHostSession action enable %d ", rc);

        /* use the udev monitor to detect MultiHostHub */
        rc = iAP2USBVendorRequestMonitor_WaitAndEnd(&monitor);
        if(rc != IAP2_OK)
        {
            IAP2USBROLESWITCHDLTLOG(DLT_LOG_WARN, " iap2VendorRequestMonitor_WaitAndEnd  = %d",rc);
        }
    }
    /* release the udev monitor */
    iAP2USBVendorRequestMonitor_Release(&monitor);

    libusb_free_device_list(list,1);
    libusb_close(device_handle_hub);
    libusb_close(device_handle_ios);
    libusb_exit(usb_context);

    IAP2USBROLESWITCHDLTLOG(DLT_LOG_INFO, " iap2SwitchToMultiHostMode() for %s = %d",info->serialNumber, rc);
    return rc;
}


/***************************************************************************//**
 * S32  iap2TerminateMultiHostMode(iAP2USBRoleSwitchInfo* info)
 *
 * Disables multi-host session mode for the Molex Hub by sending vendor request command to HFC
 * Note : Vbus reset functionality is not supported by the Hub. So vbus reset is not done.
 * Note : Port number should be the same value which was obtained in function iap2SwitchToMultiHostMode earlier.
 *
 * \param[in]  info - Role switch information structure passed by application
 * \return Returns a signed integer value indicating success or failure
 *
 ******************************************************************************/

S32  iap2TerminateMultiHostMode(iAP2USBRoleSwitchInfo* info)
{
    S32 rc = IAP2_CTL_ERROR;
    libusb_context *usb_context = NULL;
    libusb_device **list;
    S32 num_devices = 0;
    libusb_device_handle *device_handle_hub = NULL;
    unsigned short wvalue = 0;
    struct libusb_device_descriptor device_descriptor;
    iAP2USBVendorRequestMonitor monitor;

    rc = libusb_init(&usb_context);
    if((rc < IAP2_OK) || (usb_context == NULL))
    {
        IAP2USBROLESWITCHDLTLOG(DLT_LOG_ERROR, "libusb init failed %d(%s)",rc,libusb_error_name(rc));
        return rc;
    }
    num_devices = libusb_get_device_list(usb_context,&list);

    for(int i=0;i<num_devices;i++)
    {
        libusb_get_device_descriptor(list[i], &device_descriptor);
        if( (device_descriptor.idVendor == MOLEX_HUB_VID) && ( (device_descriptor.idProduct & MOLEX_HUB_PID) == MOLEX_HUB_PID) )
            rc = libusb_open(list[i],&device_handle_hub);
    }

    if( (rc == IAP2_OK) && (device_handle_hub != NULL) )
    {
        if(info->mode == IAP2_USB_ROLE_SWITCH_WITH_EA)
            wvalue = WVALUE_CMD | (info->portnumber) | (SWITCH_DISABLE << 4) | (EA_ENABLE << 7);
        else
            wvalue = WVALUE_CMD | (info->portnumber) | (SWITCH_DISABLE << 4);

        IAP2USBROLESWITCHDLTLOG(DLT_LOG_INFO, " Port number in which apple device is connected = %d and command is %x ",info->portnumber,wvalue);

        /* create udev monitor to detect removal of MultiHostHub */
        rc = iAP2USBVendorRequestMonitor_Begin(&monitor, MOLEX_HUB_VID, 0x4910, NULL, TRUE);
        if(rc != IAP2_OK)
        {
            IAP2USBROLESWITCHDLTLOG(DLT_LOG_WARN, " iAP2USBMultiHostHubMonitor_Begin  = %d",rc);
        }
        else
        {
            rc = libusb_control_transfer(device_handle_hub,VENDOR_CLASS_REQ,SET_ROLE_SWITCH,wvalue,0x0000,NULL,0x00,0);
            IAP2USBROLESWITCHDLTLOG(DLT_LOG_INFO, "iap2MultiHostSession action disable %d", rc);

            /* use the udev monitor to detect removal MultiHostHub */
            rc = iAP2USBVendorRequestMonitor_WaitAndEnd(&monitor);
            if(rc != IAP2_OK)
            {
                IAP2USBROLESWITCHDLTLOG(DLT_LOG_WARN, " iap2VendorRequestMonitor_WaitAndEnd  = %d",rc);
            }
        }
        /* release the udev monitor */
        iAP2USBVendorRequestMonitor_Release(&monitor);

    }
    else
    {
        IAP2USBROLESWITCHDLTLOG(DLT_LOG_ERROR, "Device handle of Hub is NULL %d",rc);
    }

    libusb_free_device_list(list,1);
    libusb_close(device_handle_hub);
    libusb_exit(usb_context);

    IAP2USBROLESWITCHDLTLOG(DLT_LOG_INFO, "iap2TerminateMultiHostMode() for %s = %d",info->serialNumber, rc);
    return rc;
}


S32  iap2SwitchToDeviceMode(iAP2USBRoleSwitchInfo* info, udcParamInfo_t* pUdcParam)
{
    S32 rc = IAP2_OK;

    switch(pUdcParam->type)
    {
        case OTG_CONNECTED:
        {
            /* switch vbus power off to indicate iOS device to switch back */
            rc = iAP2USBPower_Switch(info->vbusPower, "off");
            IAP2USBROLESWITCHDLTLOG(DLT_LOG_INFO, " iap2SwitchVbusPower(off)  = %d ",rc);

            /* sleep may not necessary */
            sleep(2);

            /* switch the USB OTG port to 'host' */
            rc = iap2SwitchOTG(info->otgGlob, IAP2_USB_OTG_HOST);
            IAP2USBROLESWITCHDLTLOG(DLT_LOG_INFO, " iap2SwitchOTG(%s)  = %d ", IAP2_USB_OTG_HOST, rc);

            /* In case OTG port is host, but vbus was disabled,
             * make sure vbus is switched on to rerun the test. */

            (void)iAP2USBPower_Switch(info->vbusPower, "on");


            break;
        }
        case UNWIRED_HUB_CONNECTED:
        {
            rc = udcSwitchToDeviceMode(pUdcParam);
            IAP2USBROLESWITCHDLTLOG(DLT_LOG_INFO, "udcSwitchToDeviceMode()  = %d | portNum:  %s",
                    rc, pUdcParam->pDevicePortNum);

            /* insert empty string to reset the default udc */
            char empty[1] = "";
            if (TRUE != setUdcForce(&empty[0]))
            {
                IAP2USBROLESWITCHDLTLOG(DLT_LOG_ERROR, "iap2SwitchOTG():  set %s to (default) udc_force failed ", &empty[0]);
                rc = IAP2_CTL_ERROR;
            }
            break;
        }
        default:
        {
            IAP2USBROLESWITCHDLTLOG(DLT_LOG_ERROR, "Not connected or unknown ConnectStateType  %d ",pUdcParam->type);
            rc = IAP2_CTL_ERROR;
            break;
        }
    }

    freeUdcParam(pUdcParam);

    IAP2USBROLESWITCHDLTLOG(DLT_LOG_INFO, "iap2SwitchToDeviceMode() for %s = %d",info->serialNumber, rc);
    return rc;
}

void iap2SwitchOTGInitialize(void)
{
    iap2SwitchOTG(iap2GetSwtichOtgGlobPath(), IAP2_USB_OTG_HOST);
}

/* **********************  local helper functions    ********************** */


LOCAL S32 iap2SwitchOTG(const char* otgGlob, const char* value)
{
    S32 status = IAP2_OK;
    const size_t len = strlen(value) + 1;

#ifdef IPOD_ARCH_ARM
    char otgPath[STR_MAX_LENGTH];

    status = _findOTGPath(otgPath, STR_MAX_LENGTH, otgGlob);

    /* error logged and handled */

    if (status == IAP2_OK)
    {
        /* change VBUS auto behavior dependent on USB OTG mode.
         * host-to-gadget: we have to disable vbus_auto before setting role to gadget. */
        if (0 == strncmp(IAP2_USB_OTG_GADGET, value, len - 1))
        {
            /* disable when switching to gadget. */
            status = iap2SwitchVbusAuto(otgPath, STR_DISABLE);
            IAP2USBROLESWITCHDLTLOG(DLT_LOG_INFO, "iap2SwitchVbusAuto(%s)  = %d ",
                    STR_DISABLE, status);
        }
        /* change USB OTG role */
        if ((status == IAP2_OK)
            && (TRUE != iAP2UsbRoleSwitchCommon_Write(otgPath, "role", value, TRUE)))
        {
            IAP2USBROLESWITCHDLTLOG(DLT_LOG_ERROR, "iap2SwitchOTG():  set USB OTG %s/role to %s failed ", otgPath, value);
            status = IAP2_CTL_ERROR;
        }
        /* change VBUS auto behavior dependent on USB OTG mode.
         * gadget-to-host: we have to enable vbus_auto after setting role to host. */
        if (0 == strncmp(IAP2_USB_OTG_HOST, value, len - 1))
        {
            /* enable when switching to host. */
            status = iap2SwitchVbusAuto(otgPath, STR_ENABLE);
            IAP2USBROLESWITCHDLTLOG(DLT_LOG_INFO, "iap2SwitchVbusAuto(%s)  = %d ", STR_ENABLE, status);
            /* The host controller for the OTG port is not available after switching the role to gadget.
             * Therefore, vbus_auto does not exists until switching back the role to host. */
            /* Do not handle this as an error. */
            status = IAP2_OK;
        }
    }
    else
    {
        IAP2USBROLESWITCHDLTLOG(DLT_LOG_ERROR, "iap2SwitchOTG():  did not find otgPath based on otgGlob: %s ", otgGlob);
    }

#else

    char modeValue[16] = "host";

    if (strncmp(IAP2_USB_OTG_HOST, value, len - 1) == 0)
	{
		strcpy(modeValue, "host");          /* host mode */
	}
	else
	{
		strcpy(modeValue, "peripheral");    /* gadget mode */
	}

	if (!iAP2UsbRoleSwitchCommon_WriteValue(otgGlob, modeValue, 0, TRUE))
	{
		IAP2USBROLESWITCHDLTLOG(DLT_LOG_WARN, "iap2SwitchOTG():  set USB OTG %s/role to %s failed ", otgGlob, modeValue);
		status = IAP2_CTL_ERROR;
	}

#endif /* #ifdef IPOD_ARCH_ARM */

    return status;
}

#ifdef IPOD_ARCH_ARM
LOCAL S32 iap2SwitchVbusAuto(const char* otgPath, const char* value)
{
    S32 ret = IAP2_OK;
    glob_t found;
    char valuePath[STR_MAX_LENGTH];
    char vbusAutoPath[STR_MAX_LENGTH] = "";
    int file = 0;

    // with newly Kernel 3.8 patches and Kernel 3.14,
    // there could be an additional sub-path /usb*/ to the vbus_auto
    // e.g. /sys/devices/soc0/soc.0/21*/2184*/ci*/vbus_aauto
    //      or /sys/devices/soc0/soc.0/21*/2184*/ci*/usb*/vbus_auto

    // otgPath should be:   /sys/devices/soc0/soc.0/21*/2184*/ci*/
    ret = snprintf(valuePath, STR_MAX_LENGTH, "%s/%s/%s", otgPath, STR_USB_DYN_NUM, STR_VBUS_AUTO);
    if (ret >= 0 && ret < STR_MAX_LENGTH)
    {
        /* find VBUS_AUTO path */
        if (0 == (ret = glob(valuePath, 0, NULL, &found)) && found.gl_pathc > 0)
        {
            if (found.gl_pathc > 1)
            {
                IAP2USBROLESWITCHDLTLOG(DLT_LOG_INFO, "iap2SwitchVbusAuto()  more than one VBUS found;    use: %s",
                       found.gl_pathv[0]);
            }
            strncpy(&vbusAutoPath[0], found.gl_pathv[0], strlen(found.gl_pathv[0]) +1);
            IAP2USBROLESWITCHDLTLOG(DLT_LOG_INFO, "iap2SwitchVbusAuto()  found vbus_auto=%s ",vbusAutoPath);
            ret = IAP2_OK;

            globfree(&found);
        } else if (ret == GLOB_NOMATCH){
            IAP2USBROLESWITCHDLTLOG(DLT_LOG_ERROR, "iap2SwitchVbusAuto()  glob does not found %s  ret = %d", valuePath, ret);
            ret = IAP2_CTL_ERROR;
        } else{
            IAP2USBROLESWITCHDLTLOG(DLT_LOG_ERROR, "iap2SwitchVbusAuto()  glob failed with ret = %d", ret);
            ret = IAP2_CTL_ERROR;
        }
        if (IAP2_OK != ret)
        {
            memset(valuePath, 0, STR_MAX_LENGTH);
            ret = snprintf(valuePath, STR_MAX_LENGTH, "%s/%s", otgPath, STR_VBUS_AUTO);
            if (ret >= 0 && ret < STR_MAX_LENGTH)
            {
                /* find VBUS_AUTO path */
                if (0 == (ret = glob(valuePath, 0, NULL, &found)) && found.gl_pathc > 0)
                {
                    if (found.gl_pathc > 1)
                    {
                        IAP2USBROLESWITCHDLTLOG(DLT_LOG_INFO, "iap2SwitchVbusAuto()  more than one VBUS found;     use: %s",
                               found.gl_pathv[0]);
                    }

                    strncpy(&vbusAutoPath[0], found.gl_pathv[0], strlen(found.gl_pathv[0]) +1);
                    IAP2USBROLESWITCHDLTLOG(DLT_LOG_INFO, "iap2SwitchVbusAuto()  found vbus_auto=%s ",vbusAutoPath);
                    ret = IAP2_OK;

                    globfree(&found);
                } else if (ret == GLOB_NOMATCH){
                    IAP2USBROLESWITCHDLTLOG(DLT_LOG_ERROR, "iap2SwitchVbusAuto()  glob does not found %s  ret = %d", valuePath, ret);
                    ret = IAP2_CTL_ERROR;
                } else{
                    IAP2USBROLESWITCHDLTLOG(DLT_LOG_ERROR, "iap2SwitchVbusAuto()  glob failed with ret = %d", ret);
                    ret = IAP2_CTL_ERROR;
                }
            } else {
                IAP2USBROLESWITCHDLTLOG(DLT_LOG_ERROR, "iap2SwitchVbusAuto()  create path to usb*/vbus_auto failed. ");
                ret = IAP2_CTL_ERROR;
            }
        }
    } else {
        IAP2USBROLESWITCHDLTLOG(DLT_LOG_ERROR, "iap2SwitchVbusAuto()  create path to vbus_auto failed. ");
        ret = IAP2_CTL_ERROR;
    }

    if (ret == IAP2_OK)
    {
        file = open(vbusAutoPath, O_WRONLY);
        if (file >= 0)
        {
            close(file);
            if (TRUE != iAP2UsbRoleSwitchCommon_WriteValue(vbusAutoPath, value, 0, TRUE))
            {
                IAP2USBROLESWITCHDLTLOG(DLT_LOG_ERROR, "iap2SwitchVbusAuto()  set %s to %s failed.",
                        vbusAutoPath, value);
                ret = IAP2_CTL_ERROR;
            }
        }
        else
        {
            if (ENOENT == errno)
            {
                IAP2USBROLESWITCHDLTLOG(DLT_LOG_WARN, "iap2SwitchVbusAuto()  open failed. %s does not exist.", vbusAutoPath);
                /* currently, it's ok, because we don't know if the path is available */
                ret = IAP2_OK;
            }
            else
            {
                IAP2USBROLESWITCHDLTLOG(DLT_LOG_ERROR, "iap2SwitchVbusAuto()  open failed: %d %s ", errno, strerror(errno));
                ret = IAP2_CTL_ERROR;
            }
        }
    }
    else
    {
        IAP2USBROLESWITCHDLTLOG(DLT_LOG_ERROR, "iap2SwitchVbusAuto()  create path to vbus_auto failed. ");
        ret = IAP2_CTL_ERROR;
    }

    return ret;
}

#endif /* IPOD_ARCH_ARM */

/* **********************  functions ********************** */

char* iap2GetSwtichOtgGlobPath(void)
{
#ifdef IPOD_ARCH_ARM
    if(iap2IsKernel314()) {
        return IAP2_USB_ROLE_SWITCH_OTG_GLOB_314;
    } else {
        return IAP2_USB_ROLE_SWITCH_OTG_GLOB;
    }
#elif defined IPOD_ARCH_ARM64
    return IAP2_USB_ROLE_SWITCH_OTG_GLOB_RCAR;
#else
    return IAP2_USB_ROLE_SWITCH_OTG_GLOB_EXTCON;
#endif /* #ifdef IPOD_ARCH_ARM */ 
}
S32 iap2FindVbus(iAP2USBRoleSwitchInfo* info)
{
    S32 rc = 0;
    glob_t found;

    /* find VBUS path */
    if (0 == (rc = glob(IAP2_VBUS_POWER, 0, NULL, &found)) && found.gl_pathc > 0)
    {
        if (found.gl_pathc > 1)
        {
            IAP2USBROLESWITCHDLTLOG(DLT_LOG_INFO, "more than one VBUS found;    use: %s", found.gl_pathv[0]);
        }

        info->vbusPower = malloc(strlen(found.gl_pathv[0]) +1);
        strncpy(info->vbusPower, found.gl_pathv[0], strlen(found.gl_pathv[0]) +1);

        globfree(&found);
    } else if (rc == GLOB_NOMATCH){
        IAP2USBROLESWITCHDLTLOG(DLT_LOG_WARN, "glob does not found %s  rc = %d", IAP2_VBUS_POWER, rc);
        rc = IAP2_CTL_ERROR;
    } else{
        IAP2USBROLESWITCHDLTLOG(DLT_LOG_WARN, "glob failed with rc = %d", rc);
        rc = IAP2_CTL_ERROR;
    }

    return rc;
}
