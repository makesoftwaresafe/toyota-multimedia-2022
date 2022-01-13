/************************************************************************
 * \file ipodspi.h
 *
 * \version $Id: ipodspi.h ,
 *
 * \release $Name:  $
 *
 * \brief This is the implementation of the header file of the ipodspi component for Linux
 *
 * \component ipod control
 *
 * \author Norbert Fleischmann
 *
 * \copyright (c) 2003 - 2011 ADIT Corporation
 *
 ***********************************************************************/

#ifndef IPODAUTH_H
#define IPODAUTH_H

#ifdef __cplusplus
extern "C" {
#endif

#include <adit_typedef.h>

/*! Device is opened as read only */
#define IPOD_AUTHCOM_FLAGS_RDONLY   0
/*! Device is opened as write only */
#define IPOD_AUTHCOM_FLAGS_WRONLY   1
/*! Device is opened as possible to read/write */
#define IPOD_AUTHCOM_FLAGS_RDWR     2

/*! Device for data communicate is opened */
#define IPOD_AUTHCOM_DATA_OPEN      0x00000000
/*! Device for GPIO is opened */
#define IPOD_AUTHCOM_GPIO_OPEN      0x00000001
/*! ioctl request to set the i2c slave register address */
#define IPOD_AUTHCOM_IOCTL_I2C_SLAVE_ADDR       1

#define IPOD_AUTHCOM_SUCCESS         0
#define IPOD_AUTHCOM_ERROR          -1
#define IPOD_AUTHCOM_NOMEM          -2

/*! Bad parameter */
#define IPOD_AUTHCOM_BAD_PARAMETER  -86
/*! Busy processing other requests */
#define IPOD_AUTHCOM_ERR_TMOUT      -50

#define IPOD_AUTHCOM_WRITE_BIT       (1 << 7)

#define IPOD_AUTHCOM_HIGH_MASK   0xFF00
#define IPOD_AUTHCOM_LOW_MASK    0x00FF

/*!
 * \fn iPodAuthComInit(void)
 * This function is used for initialize the authcom plugin.<br>
 * This may be not used by plugin. In this case, this function must always return success.<br>
 * If this function succeed, it is returned #IPOD_AUTHCOM_SUCCESS. otherwise negative value is returned.
 * \retval #IPOD_AUTHCOM_SUCCESS function success.<br>
 */
int iPodAuthComInit (void);

/*!
 * \fn iPodAuthComDeinit(void)
 * This function is used for de-initialize the authcom plugin.<br>
 * This may be not used by plugin. In this case, this function must always return success.<br>
 * If this function succeed, it is returned #IPOD_AUTHCOM_SUCCESS. otherwise negative value is returned.
 * \retval #IPOD_AUTHCOM_SUCCESS function success.<br>
 */
int iPodAuthComDeinit (void);

/*!
 * \fn iPodAuthComOpen(unsigned char *deviceName, int flags, int mode)
 * This function is used to open the driver with name.<br>
 * This function can open the authentication access driver.<br>
 * If data communicate driver is opened, #IPOD_AUTHCOM_DATA_OPEN bit must be set in mode.<br>
 * If GPIO driver is opened, #IPOD_AUTHCOM_GPIO_OPEN bit must be set in mode.<br>
 * This function returns a value equal to  or greater than 0 if this function succeed.<br>
 * Otherwise this function will returns an negative value<br>
 * \param [in] *deviceName - name of authentication device with the absolute path. It must be NULL terminated strings.<br>
 * \param [in] flags - It must be set #IPOD_AUTHCOM_RDONLY, #IPOD_AUTHCOM_WRONLY or #IPOD_AUTHCOM_RDWR<br>
 * \param [in] mode - It can set the #IPOD_AUTHCOM_DATA_OPEN or #IPOD_AUTHCOM_GPIO_OPEN. If neccessary, pulgin can add<br>
 * \retval fd : file descriptor on success. This value equal to or greater than 0.<br>
 * \retval error : Value is negative. This function is failed.<br>
 */
int iPodAuthComOpen (const char *device_name, int flags, int mode);


/*!
 * \fn iPodAuthComClose(int fd)
 * This function is used to close the driver that is indicated by fd.<br>
 * \param [in] fd - This is file descriptor that is gotten by #iPodAuthComOpen<br>
 * \retval #IPOD_AUTHCOM_SUCCESS function success.<br>
 * \retval error : Value is negative. This function is failed.<br>
 */
int iPodAuthComClose(int fd);

/*!
 * \fn iPodAuthComRead(int fd, unsinged int size, void *buf, int flags)
 * This function is used to read the data.<br>
 * Data can be read until size byte.<br>
 * If this function succeed, this function returns the size which this function could read from device.<br>
 * Otherwise, this function will return negative value.<br>
 * Currently flags parameter does not affect to this function. 
 * \param [in] fd - This is file descriptor that is opened by #iPodAuthComOpen.<br>
 * \param [in] size - Maximum size which can read the data<br>
 * \param [out] *buf - buffer with the data to be read<br>
 * \param [in] flags - If plugin want to do special behavior, this flags may be used.<br>
 * \retval size : The size is that this function could read the data.<br>
 * \retval error : Value is negative. This function is failed.<br>
 */
int iPodAuthComRead(int fd, unsigned int size, void *buf, int flags);


/*!
 * \fn iPodAuthComWrite(int fd, unsigned int size, const void *buf, int flags)
 * This function is used to write the data.<br>
 * Data is written until size byte.<br>
 * If this function succeed, this function returns the size which this function could write to device.<br>
 * Otherwise, this function will return negative value.<br>
 * Currently flags parameter does not affect to this function. 
 * \param [in] fd - This is file descriptor that is opened by #iPodAuthComOpen.<br>
 * \param [in] size - Maximum size which can write the data<br>
 * \param [out] *buf - data buffer <br>
 * \param [in] flags - If plugin want to do special behavior, this flags may be used.<br>
 * \retval size : The size is that this function could write the data.<br>
 * \retval error : Value is negative. This function is failed.<br>
 */
int iPodAuthComWrite(int fd, unsigned int size, const void *buf, int flags);

/*!
 * \fn iPodAuthComIoctl(int fd, int request, char *argp)
 * This function is used for special behavior.<br>
 * Plugin can use freely for some purpose.<br>
 * \param [in] fd - This is file descriptor that is opened by #iPodAuthComOpen.<br>
 * \param [in] request - It may be requested command, requested operation(read/write) or size of next parameter. <br>
 * \param [in] argp - Plugin specific parameter.<br>
 * \retval ret : Plugin can set freely.<br>
 */
int iPodAuthComIoctl(int fd, int request, char *argp);



#ifdef __cplusplus
}
#endif

#endif /* IPODAUTH_H */
