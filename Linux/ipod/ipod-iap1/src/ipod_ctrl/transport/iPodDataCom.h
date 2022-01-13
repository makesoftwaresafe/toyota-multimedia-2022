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
 * \li Added the #iPodUSBHostComInit function <br>
 * \li Added the parameter of #iPodBTComInit <br>
 * \li Added the define of error <br>
 * \version \b A03:   18/Feb/2013<br>
 * \li Changed the description of #iPodDataComAbort<br>
 * \li Added the new error of #IPOD_DATACOM_ERR_ABORT<br>
 * \li Changed the description of #iPodDataComRead<br>
 *
 */

#ifndef IPOD_DATACOM_H
#define IPOD_DATACOM_H

#ifdef __cplusplus
extern "C" {
#endif

#include <adit_typedef.h>
#include <stdio.h>
#include <stdlib.h> /* needed to avoid 'implicit declaration of calloc ...' compiler warning */

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
#define IPOD_DATACOM_SUCCESS            0
/*! Command failed */
#define IPOD_DATACOM_ERROR             -1
/*! Insufficient memory */
#define IPOD_DATACOM_ERR_NOMEM         -33
/*! Request is aborted */
#define IPOD_DATACOM_ERR_ABORT         -66
/*! unsupported device */
#define IPOD_DATACOM_ERR_UNSUP_DEV     -84
/*! Bad parameter */
#define IPOD_DATACOM_BAD_PARAMETER     -86
/*! The Ipod is not connected */
#define IPOD_DATACOM_NOT_CONNECTED     -90
/*! The Ipod is already connected */
#define IPOD_DATACOM_ALREADY_CONNECTED -91

#define IPOD_USB_VENDOR_ID              0x05AC
#define IPOD_USB_PRODUCT_ID_MASK        0xFF00
#define IPOD_USB_PRODUCT_ID             0x1200

/*! struct IPOD_DATACOM_PROPERTY
 *
 * Structure of opened device's property<br>
 */
typedef struct
{
    /*! Maxmum size that device is possible to read and write the data. */
    U32 maxSize;
} IPOD_DATACOM_PROPERTY;
/**
 * \addtogroup DataComIF DataCom Interface
 *
 */

/*\{*/

/*!
 * \fn typedef S32 (*iPodDataComOpen)(const U8 *deviceName, S32 flags, S32 mode)
 * \param [in] *deviceName - name of opened driver. It must be NULL terminated strings.<br>
 * \param [in] flags - It must be set #IPOD_DATACOM_FLAGS_RDONLY, #IPOD_DATACOM_FLAGS_WRONLY or #IPOD_DATACOM_FLAGS_RDWR<br>
 * \param [in] mode - It does not have affect currently. If neccessary, pugin can add<br>
 * \retval fd : file descriptor. This value equal to or greater than 0.<br>
 * \retval error : Value is negative. This function is failed.<br>
 * \par DESCRIPTION
 * This function is used for open the data communication driver by name.
 * This function returns a value equal to or greater than 0 if this function succeed.
 * Otherwise this function will returns an negative value
 * \attention
 * <b> This function must guarantee that it is thread safe.</b>
 */
typedef S32 (*iPodDataComOpen)(const U8 *deviceName, S32 flags, S32 mode);



/*!
 * \fn typedef S32 (*iPodDataComClose)(S32 fd)
 * \param [in] fd - This is file descriptor that is gotten by #iPodDataComOpen<br>
 * \retval #IPOD_DATACOM_SUCCESS function success.<br>
 * \retval error : Value is negative. This function is failed.<br>
 * \par DESCRIPTION
 * This function is used for close the opened device that is indicated by fd.
 * \attention
 * <b> This function must guarantee that it is thread safe.</b>
 */
typedef S32 (*iPodDataComClose)(S32 fd);

/*!
 * \fn typedef S32 (*iPodDataComWrite)(S32 fd, U32 size, const U8 *buf, S32 flags)
 * Data is written until size byte.<br>
 * \param [in] fd - This is file descreptor that is opened by #iPodDataComOpen.<br>
 * \param [in] size - Maximum size which can write the data<br>
 * \param [out] *buf - data buffer <br>
 * \param [in] flags - If plugin want to do special behavior, this flags may be used.<br>
 * \retval size : The size is that this function could write the data.<br>
 * \retval error : Value is negative. This function is failed.<br>
 * \par DESCRIPTION
 * This function is used for write the data.<br>
 * If this function succeed, this function returns the size which this function could write to device.<br>
 * Otherwise, this function will return negative value.<br>
 * Currently flags parameter does not affect to this function.
 * \attention
 * <b> This function must guarantee that it is thread safe.</b>
 */
typedef S32 (*iPodDataComWrite)(S32 fd, U32 size, const U8 *buf, S32 flags);


/*!
 * \fn typedef int (*iPodDataComRead)(S32 fd, U32 size, U8 *buf, S32 flags)
 * \param [in] fd - This is file descreptor that is opened by #iPodDataComOpen.<br>
 * \param [in] size - Maximum size which can read the data<br>
 * \param [out] *buf - data buffer <br>
 * \param [in] flags - If plugin want to do special behavior, this flags may be used.<br>
 * \retval size : The size is that this function could read the data.<br>
 * \retval error : Value is negative. This function is failed.<br>
 * \par DESCRIPTION
 * This function is used for read the data.<br>
 * Data can be read until size byte.<br>
 * If this function succeed, this function returns the size which this function could read from device.<br>
 * Otherwise, this function will return negative value.<br>
 * Also this function does not return until data is filled.<br>
 * Currently flags parameter does not affect to this function. 
 * \warning
 * If this function is called when device does not have data, this funcction is stopped until receve the data.<br>
 * \attention
 * <b> This function must guarantee that it is thread safe.</b>
 */
typedef S32 (*iPodDataComRead)(S32 fd, U32 size, U8 *buf, S32 flags);


/*!
 * \fn typedef S32 (*iPodDataComIoctl)(S32 fd, S32 request, U8 *argp)
 * \param [in] fd - This is file descreptor that is opened by #iPodDataComOpen.<br>
 * \param [in] request - It may be requested command, requested operation(read/write) or size of next parameter. <br>
 * \param [in] argp - Plugin specific parameter.<br>
 * \retval ret : Plugin can set freely.<br>
 * \par DESCRIPTION
 * This function is used for special behovior.<br>
 * Plugin can use freely for some purpose.<br>
 * \attention
 * <b> This function must guarantee that it is thread safe.</b>
 */
typedef S32 (*iPodDataComIoctl)(S32 fd, S32 request, U8 *argp);



/*!
 * \fn typedef S32 (*iPodDataComGetProperty)(S32 fd, IPOD_DATACOM_PROPERTY *property)
 * \param [in] fd - This is file descreptor that is opened by #iPodDataComOpen.<br>
 * \param [out] property - Structure of connected device's property. <br>
 * \retval #IPOD_DATACOM_SUCCESS : function success.<br>
 * \retval error : Value is negative.
 * \par DESCRIPTION
 * This function is used for get the property of opened device.<br>
 * If this function succeed, this function returns #IPOD_DATACOM_SUCCESS.<br>
 * Otherwise, this function will return with negative value.<br>
 * \attention
 * <b> This function must guarantee that it is thread safe.</b>
 */
typedef S32 (*iPodDataComGetProperty)(S32 fd, IPOD_DATACOM_PROPERTY *property);

/*!
 * \fn typedef S32 (*iPodDataComAbort)(S32 fd);
 * \param [in] fd - This is file descreptor that is opened by #iPodDataComOpen.<br>
 * \retval #IPOD_DATACOM_SUCCESS : function success.<br>
 * \retval error : Value is negative.
 * \par DESCRIPTION
 * This function is used for abort the current running request and 
 * #iPodDataComRead and #iPodDataComWrite cannot use after #iPodDataComAbort completed.<br>
 * If this function suceed, this function returns #IPOD_DATACOM_SUCCESS.<br>
 * Otherwise, this function will return with negative value.<br>
 * also, the request which is aborted by this function will finish with error.<br>
 * \attention
 * <b> This function must guarantee that it is thread safe.</b>
 */
typedef S32 (*iPodDataComAbort)(S32 fd);



/*!
 * \struct IPOD_DATACOM_FUNC_TABLE
 * This structure is used for set the function of plugin<br>
 */
typedef struct
{
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
} IPOD_DATACOM_FUNC_TABLE;

#ifdef IPOD_USB_HOST_PLUGIN

/*!
 * \fn S32 iPodUSBHostComInit(IPOD_DATACOM_FUNC_TABLE *table, U32 deviceNum)
 * This function is used for initialize the usb host communication plugin.<br>
 * If this function is called, plugin must be set the each function pointer to function table.<br>
 * If this function succeed, it is returned #IPOD_DATACOM_SUCCESS. otherwise negative value is returned.<br>
 * This function is called when iPodPlayer is initialized.<br>
 * \param [out] *table - Structure of each function pointer
 * \param [in] deviceNum - Maximum number of device
 * \retval #IPOD_DATACOM_SUCCESS function success.<br>
 */
S32 iPodUSBHostComInit(IPOD_DATACOM_FUNC_TABLE *table, U32 deviceNum);


/*!
 * \fn iPodUSBHostComDeinit(IPOD_DATACOM_FUNC_TABLE *table)
 * This function is used for de-initialize the usb host communication plugin.<br>
 * If this function succeed, it is returned #IPOD_DATACOM_SUCCESS. otherwise negative value is returned.
 * \param [out] *table - Structure of each function pointer
 * \retval #IPOD_DATACOM_SUCCESS function success.<br>
 */
S32 iPodUSBHostComDeinit(IPOD_DATACOM_FUNC_TABLE *table);

#endif /* IPOD_USB_HOST_PLUGIN */

#ifdef IPOD_BT_PLUGIN


/*!
 * \fn iPodBTComInit(IPOD_DATACOM_FUNC_TABLE *table, U32 deviceNum)
 * This function is used for initialize the bluetooth communication plugin.<br>
 * If this function is called, plugin must be set the each function pointer to function table.
 * If this function succeed, it is returned #IPOD_DATACOM_SUCCESS. otherwise negative value is returned.
 * This function is called when iPodPlayer is initialized.<br>
 * \param [out] *table - Structure of each function pointer
 * \param [in] deviceNum - Maximum number of device
 * \retval #IPOD_DATACOM_SUCCESS function success.<br>
 */
S32 iPodBTComInit(IPOD_DATACOM_FUNC_TABLE *table, U32 deviceNum);


/*!
 * \fn iPodBTComDeinit(IPOD_DATACOM_FUNC_TABLE *table)
 * This function is used for de-initialize the bluetooth communication plugin.<br>
 * If this function succeed, it is returned #IPOD_DATACOM_SUCCESS. otherwise negative value is returned.
 * \param [out] *table - Structure of each function pointer
 * \retval #IPOD_DATACOM_SUCCESS function success.<br>
 */
S32 iPodBTComDeinit(IPOD_DATACOM_FUNC_TABLE *table);

#endif /* IPOD_BT_PLUGIN */

#ifdef IPOD_UART_PLUGIN
/*!
 * \fn iPodUARTComInit(IPOD_DATACOM_FUNC_TABLE *table, U32 deviceNum)
 * This function is used for initialize the uart communication plugin.<br>
 * If this function is called, plugin must be set the each function pointer to function table.
 * If this function succeed, it is returned #IPOD_DATACOM_SUCCESS. otherwise negative value is returned.
 * This function is called when iPodPlayer is initialized.<br>
 * \param [out] *table - Structure of each function pointer
 * \param [in] deviceNum - Maximum number of device
 * \retval #IPOD_DATACOM_SUCCESS function success.<br>
 */
S32 iPodUARTComInit(IPOD_DATACOM_FUNC_TABLE* table, U32 num_devices);


/*!
 * \fn iPodUARTComDenit(IPOD_DATACOM_FUNC_TABLE *table)
 * This function is used for de-initialize the uart communication plugin.<br>
 * If this function succeed, it is returned #IPOD_DATACOM_SUCCESS. otherwise negative value is returned.
 * \param [out] *table - Structure of each function pointer
 * \retval #IPOD_DATACOM_SUCCESS function success.<br>
 */
S32 iPodUARTComDenit(IPOD_DATACOM_FUNC_TABLE* table);
#endif /* IPOD_UART_PLUGIN */

/*\}*/

#endif


