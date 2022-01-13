#ifndef IAP2_TRANSPORT_H
#define IAP2_TRANSPORT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "iap2_init.h"


/**************************************************************************//**
 * This function is used for open the transport connection and the driver.
 * If Successfully opened the the transport->iAP2TransportHdl is updated with
 * the device handle
 *
 * \param iap2Transport  transport connection details for a specific device
 *
 * \return IAP2_OK  Completed successfully
 * \return AP2_CTL_ERROR  Open transport connection failed
 * \return IAP2_BAD_PARAMETER  invalid parameter or NULL pointer
 * \return IAP2_ERR_USB_ROLE_SWITCH_UNSUP  Apple device does not support USB
 *                                         role switch
 * \return IAP2_ERR_USB_ROLE_SWITCH_FAILED  USB role switch failed
 * \see
 * \note
 ****************************************************************************/
S32 iAP2DeviceOpen(iAP2Transport_t* transport);


/**************************************************************************//**
 * This function is used to close the transport connection and the used driver.
 *
 * \param iap2Transport  transport connection details for a specific device
 *
 * \return IAP2_OK  Completed successfully
 * \return IAP2_CTL_ERROR  close transport connection failed
 * \return IAP2_BAD_PARAMETER  invalid parameter or NULL pointer
 * \see
 * \note
 ****************************************************************************/
S32 iAP2DeviceClose(iAP2Transport_t* transport);

/**************************************************************************//**
 * This function is used to write  data to Apple device.
 *
 * \param iap2Transport  transport connection details for a specific device
 * \param writeBuff  data buffer
 * \param writeLen   Maximum size which shall written
 *
 * \return IAP2_OK  Write data completed successfully
 * \return IAP2_CTL_ERROR  Write data failed
 * \return IAP2_BAD_PARAMETER  invalid parameter or NULL pointer
 * \return IAP2_ERR_NO_MEM  could not get enough memory or buffer to small
 * \see
 * \note
 ****************************************************************************/
S32 iAP2DeviceWrite(iAP2Transport_t* transport, U8* writeBuff, U16 writeLen);


/**************************************************************************//**
 * This function is used for write the data to usb in device mode (Apple device in Device mode)
 *
 * \param iap2Transport - transport connection details for a
 *                                  specific device
 * \param writeBuff  data buffer
 * \param writeLen   Maximum size which shall written
 *
 * \return IAP2_OK  Write data completed successfully
 * \return IAP2_CTL_ERROR  Write data failed
 * \return IAP2_BAD_PARAMETER  invalid parameter or NULL pointer
 * \return IAP2_ERR_NO_MEM  could not get enough memory or buffer to small
 * \see
 * \note
 ****************************************************************************/
S32 iAP2UsbDeviceWrite(iAP2Transport_t* transport, U8* writeBuff, U16 writeLen);


/**************************************************************************//**
 * This function is used for write the data to usb in host mode (Apple device is host)
 *
 * \param iap2Transport  transport connection details for a specific device
 * \param writeBuff      data buffer
 * \param writeLen       Maximum size which shall written
 *
 * \return IAP2_OK         Write data completed successfully
 * \return IAP2_CTL_ERROR  Write data failed
 * \return IAP2_BAD_PARAMETER  invalid parameter or NULL pointer
 * \return IAP2_ERR_NO_MEM     could not get enough memory or buffer to small
 * \see
 * \note
 ****************************************************************************/
S32 iAP2UsbHostWrite(iAP2Transport_t* transport, U8* writeBuff, U16 writeLen);

/**************************************************************************//**
 * This function is used for configure (currently, only for USB Host Mode Plug-in)
 * the transport connection and the driver.
 *
 * \param iap2Transport  transport connection details for a specific device
 *
 * \return IAP2_OK  Completed successfully
 * \return IAP2_CTL_ERROR  I/O ctl transport connection failed
 * \retutn IAP2_BAD_PARAMETER  invalid parameter or NULL pointer
 * \see
 * \note
 ****************************************************************************/
S32 iAP2UsbMultiHostWrite(iAP2Transport_t* transport, U8* writeBuff, U16 writeLen);

/**************************************************************************//**
 * This function is used for configure (currently, only for USB Multi Host Mode Plug-in)
 * the transport connection and the driver.
 *
 * \param iap2Transport  transport connection details for a specific device
 *
 * \return IAP2_OK  Completed successfully
 * \return IAP2_CTL_ERROR  I/O ctl transport connection failed
 * \retutn IAP2_BAD_PARAMETER  invalid parameter or NULL pointer
 * \see
 * \note
 ****************************************************************************/

S32 iAP2DeviceIoCtl(iAP2Transport_t* iap2Transport);

/**************************************************************************//**
 * This function is used for initialize the transport communication plug-in.
 *
 * \param iap2Device  the device structure
 * \param iap2InitParam - Initial parameter structure
 *
 * \return IAP2_OK  Completed successfully
 * \return IAP2_CTL_ERROR  function failed
 * \see
 * \note
 ****************************************************************************/
S32 iAP2TransportInit(iAP2Device_t* iap2Device, iAP2InitParam_t* iap2InitParam);


/**************************************************************************//**
 * This function is used for de-initialize the transport communication plug-in.
 *
 * \param iap2Transport transport connection details for a specific device
 *
 * \return IAP2_OK         Completed successfully
 * \return IAP2_CTL_ERROR  function failed
 * \see
 * \note
 ****************************************************************************/
S32 iAP2TransportDeInit(iAP2Transport_t* iAP2TransportDetails);

/**************************************************************************//**
 * This function is used for freeing the memory.
 *
 * \param input_ptr   the double pointer which needs to be freed
 *
 * \return None
 * \see
 * \note
 ****************************************************************************/
void iAP2FreePointer(void** input_ptr);

/**************************************************************************//**
 * This function is used for allocating the memory & copy the contents from src to dest
 *
 * \param dest_ptr  double pointer to destination location
 * \param src_ptr   pointer to the source location
 *
 * \return IAP2_OK          Completed successfully
 * \return IAP2_ERR_NO_MEM  failed to allocate the requested memory
 * \see
 * \note
 ****************************************************************************/
S32 iAP2AllocateSPtr(U8** dest_ptr, U8* src_ptr);

#ifdef __cplusplus
}
#endif

#endif
