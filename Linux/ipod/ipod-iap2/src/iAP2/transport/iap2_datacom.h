/*! 
 * \mainpage iPodDataCom Plugin API Specification
 * \par Description
 * This specification defines the interface between iPodPlayer and Apple device.<br>
 * The plugin preparer can decide the communication device by implementing the defined interface.<br>
 * Initialize function is called by iPodPlayer when iPodPlayer is initialized.<br>
 * Initialize function register the function pointer and iPodPlayer uses the function which was implemented by plugin after calling the initialize function.
 * <br>
 * Current specification allows to use the USB and Bluetooth as communication device.
 * 
 * \par License
 * This plugin is working besides iPodPlayer process. Therefore, the license which plugin has is guaranteed that does not propagate to iPodPlayer.<br>
 * When it is guranteed that does not contain the license that should publish the source code, plugin can work by same process by adding the complie option of IPOD_PLAYER_DATA_COM_NO_PROCESS<br>
 * 
 * \version \b A01:   10/May/2012<br>
 * \li Create the iPodDataComIF<br>
 * \version \b A02:   20/Aug/2012<br>
 * \li Type define changed as ADIT type define <br>
 * \li Added the #iPodDataComAbort function <br>
 * \li Removed the iPodUSBComInit function <br>
 * \li Added the #iPodiAP2USBHostComInit function <br>
 * \li Added the parameter of #iPodBTComInit <br>
 * \li Added the define of error <br>
 * \version \b A03:   18/Feb/2013<br>
 * \li Changed the description of #iPodDataComAbort<br>
 * \li Added the new error of #IPOD_DATACOM_ERR_ABORT<br>
 * \li Changed the description of #iPodDataComRead<br>
 *
 */

#ifndef IAP2_DATACOM_H
#define IAP2_DATACOM_H

#ifdef __cplusplus
extern "C" {
#endif

#include <adit_typedef.h>
#include <stdio.h>
#include <stdlib.h> /* needed to avoid 'implicit declaration of calloc ...' compiler warning */
#include <usbg/usbg.h>

/*! Device is opened as read only */
#define IPOD_DATACOM_FLAGS_RDONLY 1

/*! Device is opened as write only */
#define IPOD_DATACOM_FLAGS_WRONLY 2

/*! Device is opened as possible to read/write */
#define IPOD_DATACOM_FLAGS_RDWR   3

/*! Device is opened in blocking mode */
#define IPOD_DATACOM_MODE_BLOCKING      0
/*! Device is opened in non-blocking mode */
#define IPOD_DATACOM_MODE_NONBLOCKING   1

/*! Completed successfully */
#define IPOD_DATACOM_SUCCESS                        0
/*! Command failed */
#define IPOD_DATACOM_ERROR                          -1
/*! Connected Apple device does not support USB role switch */
#define IPOD_DATACOM_ERR_USB_ROLE_SWITCH_UNSUP      -14
/*! USB role switch process failed */
#define IPOD_DATACOM_ERR_USB_ROLE_SWITCH_FAILED     -15
/*! Insufficient memory */
#define IPOD_DATACOM_ERR_NOMEM                      -33
/*! Request is aborted */
#define IPOD_DATACOM_ERR_ABORT                      -66
/*! unsupported device */
#define IPOD_DATACOM_ERR_UNSUP_DEV                  -84
/*! Bad parameter */
#define IPOD_DATACOM_BAD_PARAMETER                  -86
/*! The Ipod is not connected */
#define IPOD_DATACOM_NOT_CONNECTED                  -90
/*! The Ipod is already connected */
#define IPOD_DATACOM_ALREADY_CONNECTED              -91

#define IPOD_USB_VENDOR_ID              0x05AC
#define IPOD_USB_PRODUCT_ID_MASK        0xFF00
#define IPOD_USB_PRODUCT_ID             0x1200


/*! struct IPOD_IAP2_DATACOM_ALTERNATE_IF_CB
 *
 * (USB Host Mode Plug-in) Callacks to inform iAP2 library in case
 * of start/stop events at the alternate settings /endpoints <br>
 */
typedef S32 (*iAP2StartAlternateIfCB)(void* iap2Device,
                                      U8 iAP2iOSAppIdentifier,
                                      U8 sinkEndpoint,
                                      U8 sourceEndpoint,
                                      void* context);
typedef S32 (*iAP2StopAlternateIfCB)(void* iap2Device,
                                     U8 iAP2iOSAppIdentifier,
                                     U8 sinkEndpoint,
                                     U8 sourceEndpoint,
                                     void* context);

typedef struct
{
    iAP2StartAlternateIfCB  p_iAP2StartAlternateIf_cb;
    iAP2StopAlternateIfCB   p_iAP2StopAlternateIf_cb;
} IPOD_IAP2_DATACOM_ALTERNATE_IF_CB;

/*! struct IPOD_IAP2_DATACOM_IOCTL_CONFIG
 *
 * (USB Host Mode Plug-in) I/O CTL configuration structure<br>
 */
typedef struct
{
    /* Accessory's serial number */
    char* accSerialNumber;
    /* Accessory's vendor ID */
    char* accVendorId;
    /* Accessory's product ID */
    char* accProductId;
    /* The bcdDevice value indicates the device-defined revision number. */
    char* accBcdDevice;
    /* Accessory's manufacture name */
    char* accManufacture;
    /* Accessory's product name */
    char* accProduct;

    /* GPIO port number to switch power on/off of the USB OTG port */
    char* powerGPIO;

    /* Pointer to the iAP2 library device structure */
    void* iap2Device;
    /* Pointer to the iAP2 library callback context */
    void* context;

    /* String which includes all iOS App names.
     * Each iOSAppName must end with '\0' (null termination) */
    char* iOSAppNames;
    /* String length of all iOS app names in param iOSAppNames */
    U32 iOSAppNamesLen;
    /* Array of the iOS App Identifier */
    U8* iOSAppIdentifier;
    /* Number of iOS App names in param iOSAppNames */
    U32 iOSAppCnt;

    /* Flag, indicate iOS in the car support */
    BOOL digitaliPodOut;
    /* Flag, indicate to use EA native transport */
    BOOL nativeTransport;

    /* filename of init endpoint e.g. /dev/ffs/ep0 */
    char* initEndPoint;

    /* Decides Whether the USB Host mode plugin has to use configfs are not */
    BOOL useConfigFS;

    U8* DeviceMAC;
} IPOD_IAP2_DATACOM_IOCTL_CONFIG;

/*! enum IPOD_IAP2_DATACOM_IOCTL_REQ
 *
 * USB Host Mode Plug-in enum to perform I/O CTL requests.<br>
 */
typedef enum
{
    IPOD_IAP2_DATACOM_IOCTL_SET_CONFIG,
    IPOD_IAP2_DATACOM_IOCTL_SET_CB,
    IPOD_IAP2_DATACOM_IOCTL_MAX
} IPOD_IAP2_DATACOM_IOCTL_REQ;


/*! struct IPOD_IAP2_DATACOM_FD
 *
 * File descriptor structure<br>
 */
typedef struct
{
    S32 fd;
    S16 event;
} IPOD_IAP2_DATACOM_FD;

/*! struct IPOD_IAP2_DATACOM_GET_FDS
 *
 * Structure for all available file descriptors<br>
 */
typedef struct
{
    IPOD_IAP2_DATACOM_FD *fds;
    S32 numFDs;
} IPOD_IAP2_DATACOM_GET_FDS;


/*! struct IPOD_IAP2_DATACOM_PROPERTY
 *
 * Structure of opened device's property<br>
 */
typedef struct
{
    /*! Maxmum size that device is possible to read and write the data. */
    U32 maxSize;
} IPOD_IAP2_DATACOM_PROPERTY;


/**
 * \addtogroup DataComIF DataCom Interface
 *
 */

/*\{*/


/***************************************************************************//**
 * This function is used for call the libusb handle event function and for read
 * the data.If this function succeed, this function returns the size which this
 * function could read from device.Otherwise, this function will return with
 * negative value.
 *
 * \attention
 * This function must guarantee that it is thread safe.
 *
 * \param[in] iPodHdl iPod device handle.
 * \param[in] size    Maximum size which could be read into buf .
 * \param[in] buf     data buffer.
 * \param[in] flags   unused.

 * \return size  The size is that this function could read the data.
 * \return error Value is negative.
 * \see
 * \note
 *******************************************************************************/
typedef S32 (*iPodDataComHandleEvent)(void* iPodHdl, U32 size, U8 *buf, S32 flags);


/***************************************************************************//**
 * This function is used for get the libusb file descriptors.
 * If this function succeed, this function returns #IPOD_DATACOM_SUCCESS.
 * Otherwise, this function will return with negative value.
 *
 * \attention
 *  This function must guarantee that it is thread safe.
 *
 * \param[in]  fd     iPodHdl -iPod device handle.
 * \param[out] getFDs Structure of available file descriptor and their events.
 * \param[out] numFDs contains the number of file descriptors in getFDs.
 *
 * \return IPOD_DATACOM_SUCCESS  function success.
 * \return error                 Value is negative.
 * \see
 * \note
 ******************************************************************************/
typedef S32 (*iPodDataComGetFDs)(void* iPodHdl, IPOD_IAP2_DATACOM_FD* getFDs, S32* numFDs);


/***************************************************************************//**
 * This function is used for open the data communication driver by name.
 * This function returns a value equal to or greater than 0 if this function succeed.
 * Otherwise this function will returns an negative value
 * \attention
 * This function must guarantee that it is thread safe.
 *
 * \param[in] iPodHdl     iPod device handle.
 * \param[in] deviceName  name of opened driver. It must be NULL terminated strings.
 * \param[in] flags       It must be set #IPOD_DATACOM_FLAGS_RDONLY,
 *                        #IPOD_DATACOM_FLAGS_WRONLY or #IPOD_DATACOM_FLAGS_RDWR
 * \param[in] mode        It does not have affect currently. If necessary, plug-in can add
 *
 * \return error  Value is negative if failed.
 * \see
 * \note
 ******************************************************************************/
typedef S32 (*iPodDataComOpen)(void* iPodHdl,const U8 *deviceName, S32 flags, S32 mode);



/***************************************************************************//**
 * This function is used for close the opened device that is indicated by fd.
 *
 * \attention
 * <b> This function must guarantee that it is thread safe.</b>
 *
 * \param[in] iPodHdl -iPod device handle.  <br>
 *
 * \return IPOD_DATACOM_SUCCESS function success.<br>
 * \return error : Value is negative. This function is failed.<br>
 * \see
 * \note
 ******************************************************************************/
typedef S32 (*iPodDataComClose)(void* iPodHdl);

/***************************************************************************//**
 * This function is used for write the data.<br>
 * If this function succeed, this function returns the size which this function
 * could write to device.<br>
 * Otherwise, this function will return negative value.<br>
 * Currently flags parameter does not affect to this function.
 * \attention
 * <b> This function must guarantee that it is thread safe.</b>
 *
 * \param[in]  iPodHdl iPod device handle.  <br>
 * \param[in]  size    Maximum size which can write the data<br>
 * \param[out] buf     data buffer <br>
 * \param[in]  flags   If plugin want to do special behavior, this flags may be used.<br>
 *
 * \return size : The size is that this function could write the data.<br>
 * \return error : Value is negative. This function is failed.<br>
 * \see
 * \note
 ******************************************************************************/
typedef S32 (*iPodDataComWrite)(void* iPodHdl, U32 size, const U8 *buf, S32 flags);


/***************************************************************************//**
 * This function is used for read the data.<br>
 * Data can be read until size byte.<br>
 * If this function succeed, this function returns the size which this function
 * could read from device.<br>
 * Otherwise, this function will return negative value.<br>
 * Also this function does not return until data is filled.<br>
 * Currently flags parameter does not affect to this function. 
 * \warning
 * If this function is called when device does not have data, this funcction is
 * stopped until receve the data.<br>
 *
 * \attention
 * <b> This function must guarantee that it is thread safe.</b>
 *
 * \param[in]  iPodHdl iPod device handle.  <br>
 * \param[in]  size    Maximum size which can read the data<br>
 * \param[out] buf     data buffer <br>
 * \param[in]  flags   If plugin want to do special behavior, this flags may be used.<br>
 *
 * \return size  : The size is that this function could read the data.<br>
 * \return error : Value is negative. This function is failed.<br>
 * \see
 * \note
 ******************************************************************************/
typedef S32 (*iPodDataComRead)(void* iPodHdl, U32 size, U8 *buf, S32 flags);


/***************************************************************************//**
 * This function is used for special behovior.<br>
 * Plugin can use freely for some purpose.<br>
 * \attention
 * <b> This function must guarantee that it is thread safe.</b>
 *
 * \param[in] iPodHdl iPod device handle.  <br>
 * \param[in] request It may be requested command, requested operation(read/write)
 *            or size of next parameter. <br>
 * \param[in] argp    Plugin specific parameter.<br>
 *
 * \return ret : Plugin can set freely.<br>
 * \see
 * \note
 ******************************************************************************/
typedef S32 (*iPodDataComIoctl)(void* iPodHdl, S32 request, void *argp);



/***************************************************************************//**
 * This function is used for get the property of opened device.<br>
 * If this function succeed, this function returns #IPOD_DATACOM_SUCCESS.<br>
 * Otherwise, this function will return with negative value.<br>
 * \attention
 * <b> This function must guarantee that it is thread safe.</b>
 *
 * \param[in]  iPodHdl  iPod device handle.  <br>
 * \param[out] property Structure of connected device's property. <br>
 *
 * \return IPOD_DATACOM_SUCCESS : function success.<br>
 * \return error : Value is negative.
 * \see
 * \note
 ******************************************************************************/
typedef S32 (*iPodDataComGetProperty)(void* iPodHdl, IPOD_IAP2_DATACOM_PROPERTY *property);

/***************************************************************************//**
 * This function is used for abort the current running request and 
 * #iPodDataComRead and #iPodDataComWrite cannot use after #iPodDataComAbort completed.<br>
 * If this function suceed, this function returns #IPOD_DATACOM_SUCCESS.<br>
 * Otherwise, this function will return with negative value.<br>
 * also, the request which is aborted by this function will finish with error.<br>
 *
 * \attention
 * <b> This function must guarantee that it is thread safe.</b>
 *
 * \param[in] iPodHdl iPod device handle.  <br>
 *
 * \return #IPOD_DATACOM_SUCCESS : function success.<br>
 * \return error : Value is negative.
 * \see
 * \note
 ******************************************************************************/
typedef S32 (*iPodDataComAbort)(void* iPodHdl);



/*!
 * \struct IPOD_IAP2_DATACOM_FUNC_TABLE
 * This structure is used for set the function of plugin<br>
 */
typedef struct
{
    /*! See #iPodDataComHandleEvent */
    iPodDataComHandleEvent hdlevent;
    /*! See #iPodDataComGetFDs */
    iPodDataComGetFDs getfds;
    /*! See #iPodDataComOpen */
    iPodDataComOpen open;
    /*! See #iPodDataComClose */
    iPodDataComClose close;
    /*! See #iPodDataComAbort */
    iPodDataComAbort abort;
    /*! See #iPodDataComWrite */
    iPodDataComWrite write;
    /*! See #iPodDataComRead */
    iPodDataComRead read;
    /*! See #iPodDataComIoctl */
    iPodDataComIoctl ioctl;
    /*! See #iPodDataComGetProperty */
    iPodDataComGetProperty property;
} IPOD_IAP2_DATACOM_FUNC_TABLE;


/***************************************************************************//**
 * This function is used for initialize the usb host communication plugin.<br>
 * If this function is called, plugin must be set the each function pointer to
 * function table.<br>If this function succeed, it is returned #IPOD_DATACOM_SUCCESS.
 * otherwise negative value is returned.<br>
 *
 * This function is called when iPodPlayer is initialized.<br>
 *
 * \param[out] table  Initialized Function table .Structure of each function pointer
 * \return     iPodHdl: device handle. A void pointer<br>
 * \see
 * \note
 ******************************************************************************/
void* iPodiAP2USBHostComInit(IPOD_IAP2_DATACOM_FUNC_TABLE *table);


/***************************************************************************//**
 * This function is used for de-initialize the usb host communication plugin.<br>
 * If this function succeed, it is returned #IPOD_DATACOM_SUCCESS. otherwise
 * negative value is returned.
 *
 * \param[in] table    Function table.
 * \param[in] iPodHdl  The iPod device handle.
 * \return #IPOD_DATACOM_SUCCESS function success.<br>
 * \see
 * \note
 *******************************************************************************/
S32 iPodiAP2USBHostComDeinit(IPOD_IAP2_DATACOM_FUNC_TABLE *table, void* iPodHdl);


/***************************************************************************//**
 * This function is used for initialize the usb device communication plugin.<br>
 * If this function is called, plugin must be set the each function pointer to
 * function table.If this function succeed, it is returned #IPOD_DATACOM_SUCCESS.
 * otherwise negative value is returned.<br>
 *
 * This function is called when iPodPlayer is initialized.<br>
 *
 * \param[out] table  Structure of each function pointer
 * \return     iPodHdl device handle. A void pointer<br>
 * \see
 * \note
 ******************************************************************************/
void* iPodiAP2USBDeviceComInit(IPOD_IAP2_DATACOM_FUNC_TABLE *table);


/***************************************************************************//**
 * This function is used for de-initialize the usb device communication plugin.<br>
 * If this function succeed, it is returned #IPOD_DATACOM_SUCCESS. otherwise
 * negative value is returned.
 *
 * \param[in] table   Structure of each function pointer
 * \param[in] iPodHdl device handle.
 *
 * \return #IPOD_DATACOM_SUCCESS function success.<br>
 * \see
 * \note
 ******************************************************************************/
S32 iPodiAP2USBDeviceComDeinit(IPOD_IAP2_DATACOM_FUNC_TABLE *table,void* iPodHdl);

/***************************************************************************//**
 * This function is used for initialize the usb multi host mode communication plugin.<br>
 * If this function is called, plugin must be set the each function pointer to
 * function table.If this function succeed, it is returned #IPOD_DATACOM_SUCCESS.
 * otherwise negative value is returned.<br>
 *
 * \param[out] table  Structure of each function pointer
 * \return     iPodHdl device handle. A void pointer<br>
 * \see
 * \note
 ******************************************************************************/

void* iPodiAP2USBMultiHostComInit(IPOD_IAP2_DATACOM_FUNC_TABLE* data_com_function);

/***************************************************************************//**
 * This function is used for de-initialize the usb multi host mode communication plugin.<br>
 * If this function succeed, it is returned #IPOD_DATACOM_SUCCESS. otherwise
 * negative value is returned.
 *
 * \param[in] table   Structure of each function pointer
 * \param[in] iPodHdl the device handle
 *
 * \return IPOD_DATACOM_SUCCESS function success.<br>
 * \see
 * \note
 ******************************************************************************/

S32 iPodiAP2USBMultiHostComDeinit(IPOD_IAP2_DATACOM_FUNC_TABLE* data_com_function,void* iPodHdl);

/***************************************************************************//**
 * This function is used for initialize the bluetooth communication plugin.<br>
 * If this function is called, plugin must be set the each function pointer to
 * function table.If this function succeed, it is returned #IPOD_DATACOM_SUCCESS.
 * otherwise negative value is returned.This function is called when iPodPlayer
 * is initialized.<br>
 *
 * \param[out] table Structure of each function pointer
 * \return iPodHdl   the device handle.<br>
 * \see
 * \note
 ******************************************************************************/
void* iPodiAP2BTComInit(IPOD_IAP2_DATACOM_FUNC_TABLE *table);


/***************************************************************************//**
 * This function is used for de-initialize the bluetooth communication plugin.<br>
 * If this function succeed, it is returned #IPOD_DATACOM_SUCCESS. otherwise
 * negative value is returned.
 *
 * \param[in] table   Structure of each function pointer
 * \param[in] iPodHdl the device handle
 *
 * \return IPOD_DATACOM_SUCCESS function success.<br>
 * \see
 * \note
 ******************************************************************************/
S32 iPodiAP2BTComDeinit(IPOD_IAP2_DATACOM_FUNC_TABLE *table, void* iPodHdl);

/***************************************************************************//**
 * This function is used for initialize the uart communication plugin.<br>
 * If this function is called, plugin must be set the each function pointer to
 * function table.If this function succeed, it is returned #IPOD_DATACOM_SUCCESS.
 * otherwise negative value is returned.This function is called when iPodPlayer
 * is initialized.<br>
 *
 * \param[out] table  Structure of each function pointer
 *
 * \return  iPodHdl  the device handle <br>
 * \see
 * \note
 ******************************************************************************/
// void* iPodiAP2UARTComInit(IPOD_IAP2_DATACOM_FUNC_TABLE* table);


/***************************************************************************//**
 * This function is used for de-initialize the uart communication plugin.<br>
 * If this function succeed, it is returned #IPOD_DATACOM_SUCCESS. otherwise
 * negative value is returned.
 *
 * \param[in] table   Structure of each function pointer
 * \param[in] iPodHdl The device handle
 *
 * \return IPOD_DATACOM_SUCCESS function success.<br>
 * \see
 * \note
 ******************************************************************************/
// S32 iPodiAP2UARTComDenit(IPOD_IAP2_DATACOM_FUNC_TABLE* table, void* iPodHdl);


/***************************************************************************//**
 * This function is used for initialize the iAP2 Over Carplay communication plugin.<br>
 * If this function is called, plugin must set each function pointer to the
 * function table.If this function succeed, it returns #IPOD_DATACOM_SUCCESS.
 * otherwise negative value is returned.This function is called when iAP2 stack
 * is initialized.<br>
 *
 * \param[out] table Structure of each function pointer
 * \return iPodHdl   the device handle.<br>
 * \see
 * \note
 ******************************************************************************/
void* iAP2OverCarPlayComInit(IPOD_IAP2_DATACOM_FUNC_TABLE *data_com_function);


/***************************************************************************//**
 * This function is used for de-initialize the iAP2 Over Carplay communication plugin.<br>
 * If this function succeed, it is returned #IPOD_DATACOM_SUCCESS. otherwise
 * negative value is returned.
 *
 * \param[in] table   Structure of each function pointer
 * \param[in] iPodHdl the device handle
 *
 * \return IPOD_DATACOM_SUCCESS function success.<br>
 * \see
 * \note
 ******************************************************************************/
S32 iAP2OverCarPlayComDeinit(IPOD_IAP2_DATACOM_FUNC_TABLE *data_com_function, void *iPodHdl);


/*\}*/

#ifdef __cplusplus
}
#endif

#endif


