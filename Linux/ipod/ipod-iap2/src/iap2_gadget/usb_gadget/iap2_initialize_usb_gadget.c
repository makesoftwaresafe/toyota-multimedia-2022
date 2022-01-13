/*****************************************************************************
-  \file : iap2_initialize_usb_gadget.c
-  \version : $Id: iap2_initialize_usb_gadget.c, v Exp $
-  \release : $Name:$
-  Contains the source code implementation for initializing and de-initializing usb gadgets
-  \component :
-  \author : Manavalan Veeramani/RBEI/ manavalan.veeramani@in.bosch.com
-  \copyright (c) 2010 - 2016 Advanced Driver Information Technology.
-  This code is developed by Advanced Driver Information Technology.
-  Copyright of Advanced Driver Information Technology, Bosch, and DENSO.
-  All rights reserved.
*****************************************************************************/

#include "iap2_initialize_usb_gadget.h"

#include "iap2_dlt_log.h"
#include "iap2_configure_ffs_gadget.h"

/**
 * Maximum string length to use.
 */
#define STRING_MAX                      256

usbg_gadget *g_iAP2_gadget = NULL;
S32         g_ep0_fd = -1;

S32 iAP2InitializeFFSGadget(iAP2_usbg_config_t *usb_gadget_config)
{
    S32 rc = IAP2_INVALID_INPUT_PARAMETER;

    if(usb_gadget_config != NULL)
    {
        rc = iAP2ConfigureFFSGadget(&usb_gadget_config->iAP2FFSConfig,
                                    TRUE,
                                    usb_gadget_config->iAP2_gadget,
                                    usb_gadget_config->iAP2_usbg_udc);
        IAP2USBGADGETDLTLOG(DLT_LOG_DEBUG, "iAP2ConfigureFFSGadget() rc = %d", rc);
        if(rc == IAP2_OK)
        {
            g_ep0_fd      = usb_gadget_config->iAP2FFSConfig.ep0_fd;
        }
    }

    return rc;
}

S32 iAP2DeInitializeGadget()
{
    S32 rc = IAP2_BAD_PARAMETER;

    if(g_iAP2_gadget != NULL)
    {
        usbg_udc *udc;

        /* Check if gadget is enabled */
        udc = usbg_get_gadget_udc(g_iAP2_gadget);
        if (udc) /* If gadget is enable we have to disable it first */
        {
            rc = usbg_disable_gadget(g_iAP2_gadget);
            iAP2USBHost_PrintLibUsbgResult((usbg_error)rc, "usbg_disable_gadget");
        }

        /* Remove gadget with USBG_RM_RECURSE flag to remove
         * also its configurations, functions and strings */
        rc = usbg_rm_gadget(g_iAP2_gadget, USBG_RM_RECURSE);
        iAP2USBHost_PrintLibUsbgResult((usbg_error)rc, "usbg_rm_gadget");

        g_iAP2_gadget = NULL;
    }
    else
    {
        IAP2USBGADGETDLTLOG(DLT_LOG_ERROR, "g_iAP2_gadget is NULL");
    }

    if(g_ep0_fd >= 0)
    {
        close(g_ep0_fd);
        g_ep0_fd = -1;
    }
    else
    {
        IAP2USBGADGETDLTLOG(DLT_LOG_ERROR, "g_ep0_fd = %d", g_ep0_fd);
    }

    IAP2USBGADGETDLTLOG(DLT_LOG_DEBUG, "rc = %d", rc);

    return rc;
}

LOCAL S32 iAP2USBHostModeConfigFSInit(iAP2_usbg_config_t       *usb_gadget_config,
                                      const usbg_gadget_attrs  *iAP2_gadget_attr,
                                      const usbg_gadget_strs   *iAP2_gadget_strs)
{
    S32 rc = IAP2_OK;

    usbg_config         *iAP2_usbg_config = NULL;
    usbg_function       *iAP2_f_ffs = NULL;
    usbg_function       *iAP2_f_ncm = NULL;

    usbg_config_strs    iAP2_config_strs;
    usbg_function_attrs iAP2_usbg_fn_attrs;
    usbg_state          *iAP2_usbg_state = NULL;
    usbg_function       *iAP2_f_uac2 = NULL;

    memset(&iAP2_config_strs,   0, sizeof(usbg_config_strs) );
    memset(&iAP2_usbg_fn_attrs, 0, sizeof(usbg_function_attrs) );

    rc = usbg_init((const char*)usb_gadget_config->iAP2ConfigFS_MountLocation, &iAP2_usbg_state);
    iAP2USBHost_PrintLibUsbgResult((usbg_error)rc, "usbg_init");

    if(rc == USBG_SUCCESS)
    {
        rc = usbg_create_gadget(iAP2_usbg_state,
                                (const char*)usb_gadget_config->iAP2GadgetName,
                                iAP2_gadget_attr,
                                iAP2_gadget_strs,
                                &usb_gadget_config->iAP2_gadget);
        iAP2USBHost_PrintLibUsbgResult((usbg_error)rc, "usbg_create_gadget");
    }

    if(rc == USBG_SUCCESS)
    {
        rc = usbg_create_function(usb_gadget_config->iAP2_gadget,
                                  F_FFS,                                                 /* Provide This as an configurable option */
                                  (const char*)usb_gadget_config->iAP2FFS_InstanceName,  /* When NULL is passed as instance name, dev_name take from f_attrs
                                                                                            is used as instance name for this function */
                                  NULL,
                                  &iAP2_f_ffs);
        iAP2USBHost_PrintLibUsbgResult((usbg_error)rc, "(FFS)usbg_create_function");
        g_iAP2_gadget = usb_gadget_config->iAP2_gadget;
    }

    if(rc == USBG_SUCCESS)
    {
        if(usb_gadget_config->iAP2_UAC_NCM_gadgets_disable == TRUE)
        {
            if(usb_gadget_config->CarPlayEnabled == TRUE)
            {
                IAP2USBGADGETDLTLOG(DLT_LOG_ERROR, "Conflicting configuration to disable NCM and enable CarPlay");
            }
            IAP2USBGADGETDLTLOG(DLT_LOG_DEBUG, "Neither UAC nor NCM is enabled");
        }
        else if(usb_gadget_config->CarPlayEnabled == FALSE)
        {
            /* Initialize UAC2 Attributes */

            iAP2_usbg_fn_attrs.header.attrs_type = USBG_F_ATTRS_UAC2;

            memcpy(&iAP2_usbg_fn_attrs.attrs.uac2, usb_gadget_config->iAP2_UAC2_Attrs, sizeof(usbg_f_uac2_attrs) );

            rc = usbg_create_function(usb_gadget_config->iAP2_gadget,
                                      F_UAC2,                                                /* Provide This as an configurable option */
                                      (const char*)usb_gadget_config->iAP2UAC2_InstanceName, /* When NULL is passed as instance name, dev_name take from f_attrs
                                                                                                is used as instance name for this function */
                                      &iAP2_usbg_fn_attrs,
                                      &iAP2_f_uac2);
            iAP2USBHost_PrintLibUsbgResult((usbg_error)rc, "(UAC2)usbg_create_function");
        }
        else
        {
            rc = usbg_create_function(usb_gadget_config->iAP2_gadget,
                                      F_NCM,                                                 /* Provide This as an configurable option */
                                      (const char*)usb_gadget_config->iAP2NCM_InstanceName,  /* When NULL is passed as instance name, dev_name take from f_attrs
                                                                                                is used as instance name for this function */
                                      NULL,
                                      &iAP2_f_ncm);
            iAP2USBHost_PrintLibUsbgResult((usbg_error)rc, "(NCM)usbg_create_function");
            if(iAP2_f_ncm)
            {
                iAP2USBHost_PrintLibUsbgResult((usbg_error)rc, "(NCM) set qmult to 1");
                usbg_set_net_qmult( iAP2_f_ncm, 1);
            }
        }
    }

    if(rc == USBG_SUCCESS)
    {
        rc = usbg_create_config(usb_gadget_config->iAP2_gadget,
                                1,
                                "Configuration-1",
                                NULL,
                                &iAP2_config_strs,
                                &iAP2_usbg_config);
        iAP2USBHost_PrintLibUsbgResult((usbg_error)rc, "usbg_create_config");
    }

    if(rc == USBG_SUCCESS)
    {
        U8 ffsinstance[STRING_MAX] = {"ffs"};

        if( (strlen((const char*)ffsinstance) + strlen((const char*)usb_gadget_config->iAP2FFS_InstanceName) + 1) < STRING_MAX )
        {
            (void)strncat((char*)ffsinstance, (const char*)usb_gadget_config->iAP2FFS_InstanceName, (STRING_MAX - 1) );
        }
        else
        {
            rc = IAP2_INVALID_INPUT_PARAMETER;
            IAP2USBGADGETDLTLOG(DLT_LOG_ERROR, "FFS - Instance name exceeds the max character i.e., 252");
        }
        if(rc == IAP2_OK)
        {
            rc = usbg_add_config_function(iAP2_usbg_config, (const char*)ffsinstance, iAP2_f_ffs);
            iAP2USBHost_PrintLibUsbgResult((usbg_error)rc, "(FFS)usbg_add_config_function");
        }
    }

    if(rc == USBG_SUCCESS)
    {
        if(usb_gadget_config->iAP2_UAC_NCM_gadgets_disable == TRUE)
        {
            IAP2USBGADGETDLTLOG(DLT_LOG_DEBUG, "Neither UAC nor NCM is enabled");
        }
        else if(usb_gadget_config->CarPlayEnabled == FALSE)
        {
            U8 uac2instance[STRING_MAX] = {"uac2"};

            if( (strlen((const char*)uac2instance) + strlen((const char*)usb_gadget_config->iAP2UAC2_InstanceName) + 1) < STRING_MAX )
            {
                (void)strncat((char*)uac2instance, (const char*)usb_gadget_config->iAP2UAC2_InstanceName, (STRING_MAX - 1) );
            }
            else
            {
                rc = IAP2_INVALID_INPUT_PARAMETER;
                IAP2USBGADGETDLTLOG(DLT_LOG_ERROR, "UAC2 - Instance name exceeds the max character i.e., 252");
            }
            if(rc == USBG_SUCCESS)
            {
                rc = usbg_add_config_function(iAP2_usbg_config, (const char*)uac2instance, iAP2_f_uac2);
                iAP2USBHost_PrintLibUsbgResult((usbg_error)rc, "(UAC2)usbg_add_config_function");
            }
        }
        else
        {
            U8 ncminstance[STRING_MAX] = {"ncm"};

            if( (strlen((const char*)ncminstance) + strlen((const char*)usb_gadget_config->iAP2NCM_InstanceName) + 1) < STRING_MAX )
            {
                (void)strncat((char*)ncminstance, (const char*)usb_gadget_config->iAP2NCM_InstanceName, (STRING_MAX - 1) );
            }
            else
            {
                rc = IAP2_INVALID_INPUT_PARAMETER;
                IAP2USBGADGETDLTLOG(DLT_LOG_ERROR, "NCM - Instance name exceeds the max character i.e., 252");
            }
            if(rc == USBG_SUCCESS)
            {
                rc = usbg_add_config_function(iAP2_usbg_config, (const char*)ncminstance, iAP2_f_ncm);
                iAP2USBHost_PrintLibUsbgResult((usbg_error)rc, "(NCM)usbg_add_config_function");
            }
        }
    }
    if(rc == USBG_SUCCESS)
    {
        usb_gadget_config->iAP2_usbg_udc = usbg_get_udc(iAP2_usbg_state, (const char*)usb_gadget_config->iAP2UdcDeviceName);

        if(usb_gadget_config->iAP2_usbg_udc == NULL)
        {
            rc = IAP2_INVALID_INPUT_PARAMETER;
            IAP2USBGADGETDLTLOG(DLT_LOG_DEBUG, "iAP2_usbg_udc is NULL, UdcDeviceName = %s", usb_gadget_config->iAP2UdcDeviceName);
        }
    }

    return rc;
}

LOCAL S32 iAP2CopyString(const char *dest_str, U8 *source_str)
{
    S32 rc = IAP2_OK;

    if( (dest_str != NULL) && (source_str != NULL) )
    {
        U32 Length;

        Length = strnlen((const char*)source_str, STRING_MAX) + 1;
        strncpy((char *)dest_str, (const char*)source_str, Length);
    }

    return rc;
}

S32 iAP2InitializeGadget(iAP2_usbg_config_t *usb_gadget_config)
{
    S32 rc = IAP2_OK;

    if(NULL != usb_gadget_config)
    {
        static usbg_gadget_strs  iAP2_GadgStrs;
        static usbg_gadget_attrs iAP2_GadgAttr;

        if( (usb_gadget_config->iAP2FFS_InstanceName         == NULL) ||
            (usb_gadget_config->iAP2AccessoryName            == NULL) ||
            (usb_gadget_config->iAP2AccessoryModelIdentifier == NULL) ||
            (usb_gadget_config->iAP2AccessoryManufacturer    == NULL) ||
            (usb_gadget_config->iAP2AccessorySerialNumber    == NULL) ||
            (usb_gadget_config->iAP2AccessoryVendorId        == NULL) ||
            (usb_gadget_config->iAP2AccessoryProductId       == NULL) ||
            (usb_gadget_config->iAP2AccessoryBcdDevice       == NULL) ||
            (usb_gadget_config->iAP2ConfigFS_MountLocation   == NULL) ||
            (usb_gadget_config->iAP2UdcDeviceName            == NULL) )
        {
            rc = IAP2_INVALID_INPUT_PARAMETER;
            IAP2USBGADGETDLTLOG(DLT_LOG_ERROR,
                                "Invalid input parameter            \
                                iAP2FFS_InstanceName = %p,          \
                                iAP2AccessoryName = %p,             \
                                iAP2AccessoryModelIdentifier = %p,  \
                                iAP2AccessoryManufacturer = %p,     \
                                iAP2AccessorySerialNumber = %p,     \
                                iAP2AccessoryVendorId = %p,         \
                                iAP2AccessoryProductId = %p,        \
                                iAP2AccessoryBcdDevice = %p,        \
                                iAP2ConfigFS_MountLocation = %p,    \
                                iAP2UdcDeviceName = %p, ",
                                usb_gadget_config->iAP2FFS_InstanceName,
                                usb_gadget_config->iAP2AccessoryName,
                                usb_gadget_config->iAP2AccessoryModelIdentifier,
                                usb_gadget_config->iAP2AccessoryManufacturer,
                                usb_gadget_config->iAP2AccessorySerialNumber,
                                usb_gadget_config->iAP2AccessoryVendorId,
                                usb_gadget_config->iAP2AccessoryProductId,
                                usb_gadget_config->iAP2AccessoryBcdDevice,
                                usb_gadget_config->iAP2ConfigFS_MountLocation,
                                usb_gadget_config->iAP2UdcDeviceName);
        }

        if((usb_gadget_config->iAP2_UAC_NCM_gadgets_disable == FALSE) && (usb_gadget_config->CarPlayEnabled == FALSE) &&
            ( (usb_gadget_config->iAP2_UAC2_Attrs == NULL) ||
              (usb_gadget_config->iAP2UAC2_InstanceName == NULL) ) )
        {
            rc = IAP2_INVALID_INPUT_PARAMETER;
            IAP2USBGADGETDLTLOG(DLT_LOG_ERROR,
                                "CarPlay Not Enabled but either UAC2 Attributes = %p is iAP2UAC2_InstanceName = %p NULL",
                                usb_gadget_config->iAP2_UAC2_Attrs,
                                usb_gadget_config->iAP2UAC2_InstanceName);
        }

        if((usb_gadget_config->iAP2_UAC_NCM_gadgets_disable == FALSE) && (usb_gadget_config->CarPlayEnabled == TRUE) &&
            (usb_gadget_config->iAP2NCM_InstanceName == NULL) )
        {
            rc = IAP2_INVALID_INPUT_PARAMETER;
            IAP2USBGADGETDLTLOG(DLT_LOG_ERROR, "CarPlay Enabled but NCM Instance name is NULL");
        }
        if(rc == IAP2_OK)
        {
            memset(&iAP2_GadgStrs, 0, sizeof(usbg_gadget_strs) );
            memset(&iAP2_GadgAttr, 0, sizeof(usbg_gadget_attrs) );

            /***************  CONFIGURE / INITIALIZE USB Gadget Attributes *****************/
            /* bcdUSB           - USB Specification Number which device complies to */
            /* bDeviceClass     - Class Code */
            /* bDeviceSubClass  - Subclass Code (Assigned by USB org) */
            /* bDeviceProtocol  - Protocol Code (Assigned by USB org) */
            /* bMaxPacketSize   - Maximum Packet Size for Zero Endpoint */
            /* Provide bcdUSB */
            iAP2_GadgAttr.bcdUSB    = 0x0200;
            /* Copy VendorID */
            iAP2_GadgAttr.idVendor  = atoi((const char*)usb_gadget_config->iAP2AccessoryVendorId);
            /* Copy ProductID */
            iAP2_GadgAttr.idProduct = atoi((const char*)usb_gadget_config->iAP2AccessoryProductId);
            /* Copy bcdDevice */
            iAP2_GadgAttr.bcdDevice = atoi((const char*)usb_gadget_config->iAP2AccessoryBcdDevice);

            /*******************************************************************************/

        /*****************  CONFIGURE / INITIALIZE USB Gadget Strings *******************/
            /* Copy Serial Number */
            rc = iAP2CopyString(iAP2_GadgStrs.str_ser, usb_gadget_config->iAP2AccessorySerialNumber);
        }
        if(rc == IAP2_OK)
        {
            /* Copy Manufacturer Name */
            rc = iAP2CopyString(iAP2_GadgStrs.str_mnf, usb_gadget_config->iAP2AccessoryManufacturer);
        }
        if(rc == IAP2_OK)
        {
            /* Copy Product Name */
            rc = iAP2CopyString(iAP2_GadgStrs.str_prd, usb_gadget_config->iAP2AccessoryName);
        }
        /*******************************************************************************/

        if(rc == IAP2_OK)
        {
            rc = iAP2USBHostModeConfigFSInit(usb_gadget_config, &iAP2_GadgAttr, &iAP2_GadgStrs);
        }
    }
    else
    {
        IAP2USBGADGETDLTLOG(DLT_LOG_ERROR, "usb_gadget_config is NULL");
        rc = IAP2_INVALID_INPUT_PARAMETER;
    }

    return rc;
}
