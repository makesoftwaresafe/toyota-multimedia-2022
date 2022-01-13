/********************************************************************//**
 * \file iap2_initialize_usb_gadget.h
 *
 * \brief to initialize & de-initialize the USB Gadgets using ConfigFS
 * \version $ $
 *
 * This header file function declaration required for initializing and de-initializing the USB Gadgets (using ConfigFS).
 *
 * \component global definition file
 *
 * \author Manavalan Veeramani/ RBEI / manavalan.veeramani@in.bosch.com
 *
 * \copyright (c) 2010 - 2016 ADIT Corporation
 *
 ***********************************************************************/

#ifndef IAP2_INITIALIZE_USB_GADGET_H
#define IAP2_INITIALIZE_USB_GADGET_H

#ifdef __cplusplus
extern "C" {
#endif

#include <usbg/usbg.h>
#include "iap2_configure_ffs_gadget.h"

/**
* \brief USB Gadget Initialization structure
*/
typedef struct
{
    /*! Gadget Name (string) Ex: "iAP_Gadget (or) CarPlayGadget"    */
    U8* iAP2GadgetName;

    /*! FFS Gadget - Instance Name (string) Ex: "ffs1"    */
    U8* iAP2FFS_InstanceName;

    /*! UAC2 Gadget - Instance Name (string) Ex: "uac21"    */
    U8* iAP2UAC2_InstanceName;

    /*! NCM Gadget - Instance Name (string) Ex: "usb1"    */
    U8* iAP2NCM_InstanceName;

    /*! Accessory Name (string) Ex: "AmazingProduct"    */
    U8* iAP2AccessoryName;

    /*! Accessory Model Identifier (string) Ex: "15697" */
    U8* iAP2AccessoryModelIdentifier;

    /*! Accessory Manufacturer (string) Ex: "ADIT"      */
    U8* iAP2AccessoryManufacturer;

    /*! Accessory Serial Number (string) Ex: "12345678" */
    U8* iAP2AccessorySerialNumber;

    /*! Accessory Vendor ID */
    U8* iAP2AccessoryVendorId;

    /*! Accessory Product ID */
    U8* iAP2AccessoryProductId;

    /*! Accessory device-defined revision number */
    U8* iAP2AccessoryBcdDevice;

    /**
     * \brief iAP2ConfigFS_MountLocation
     *
     * Path to which ConfigFS is mounted.
     *
     */
    U8* iAP2ConfigFS_MountLocation;

    /**
     * \brief iAP2UdcDeviceName
     *
     * Name of the UDC Device to which the Apple device is connected.
     *
     */
    U8* iAP2UdcDeviceName;

    /**
     * \brief CarPlayEnabled
     *
     * defines whether CarPlay is Enabled or not.
     * If Carplay is Enabled  - FFS + NCM  Gadgets are created.
     * If Carplay is Disabled - FFS + UAC2 Gadgets are created
     *
     */
    BOOL                CarPlayEnabled;

    /**
     * \brief iAP2_UAC_NCM_gadgets_disable
     *
     * Using this parameter, it's possible to specify
     * neither UAC nor NCM gadgets are needed. Required for BDCL use case
     *     *
     * If TRUE, Neither UAC nor NCM is created
     * If FALSE, Uses CarPlayEnabled to determine the needed gadgets
     *
     */
    BOOL iAP2_UAC_NCM_gadgets_disable;

    /**
     * \brief iAP2_UAC2_Attrs
     *
     * Contains information related to UAC2 gadget that has to be created.
     * This information is required when CarPlayEnabled is set to FALSE.
     *
     */
    usbg_f_uac2_attrs   *iAP2_UAC2_Attrs;

    /**
     * \brief iAP2_gadget
     *
     * Pointer to be filled with pointer to gadget that was created.
     *
     */
    usbg_gadget         *iAP2_gadget;

    /**
     * \brief iAP2_usbg_udc
     *
     * location to which the UDC obtained by name has to be stored.
     *
     */
    usbg_udc            *iAP2_usbg_udc;

    /**
     * \brief iAP2FFSConfig
     *
     * Configurations required to initialize FFS Gadget.
     *
     */
    iAP2_ffs_config_t   iAP2FFSConfig;
}iAP2_usbg_config_t;

/**************************************************************************//**
 * Initialize USB Gadget in USB Host mode using ConfigFS.
 *
 * This API must be called before iAP2InitDeviceStructure(). It initialize USB Gadget
 * in USB Host mode using ConfigFS.
 *
 * \param[in, out] iAP2_usbg_config_t - Structure containing information on how the gadgets has to be configured
 * \return Returns a signed integer value indicating success or failure
 *
 * \see
 * \note
 ******************************************************************************/
S32 iAP2InitializeGadget(iAP2_usbg_config_t* usb_gadget_config);

/**************************************************************************//**
 * De-Initialize USB Gadget in USB Host mode using ConfigFS.
 *
 * It de-initialize's USB Gadget in USB Host mode using ConfigFS. This API will disable &
 * remove all the gadgets that were created using iAP2InitializeGadget().
 *
 * \param[in] iAP2_usbg_config_t - Structure containing information about gadgets that was created.
 * \return Returns a signed integer value indicating success or failure
 *
 * \see
 * \note
 ******************************************************************************/
S32 iAP2DeInitializeGadget();

/**************************************************************************//**
 * Initialize USB FFS Gadget in USB Host mode using ConfigFS.
 *
 * This API must be called after iAP2InitializeGadget(). It initialize USB FFS Gadget
 * in USB Host mode using ConfigFS. This API will initialize the FFS Gadget, writes the USB descriptors
 * and strings to the ep0 fd of FFS gadget, then enables the gadgets created using iAP2InitializeGadget().
 *
 * \param[in, out] iAP2_usbg_config_t - Structure containing information on how the gadgets has to be configured
 * \return Returns a signed integer value indicating success or failure
 *
 * \see
 * \note
 ******************************************************************************/
S32 iAP2InitializeFFSGadget(iAP2_usbg_config_t* usb_gadget_config);

#ifdef __cplusplus
}
#endif

#endif
