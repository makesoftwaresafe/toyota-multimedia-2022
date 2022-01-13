/*! \file iPodPlayerDeviceDetection.h
 *
 * \version: 1.1
 *
 * \author: Masaaki Adachi
 */

#ifndef IPOD_PALEYR_DEVICE_DETECTION_H
#define IPOD_PALEYR_DEVICE_DETECTION_H

#include "iPodPlayerCB.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \addtogroup DeviceDetectionAPI DeviceDetection
 * This group is API that is used to do the device detection. API of this group returns the result immediately when application calls API.<br>
 * Application can receive the result of iPodPlayer's operation by callback function.<br>
 */
/*\{*/


/*!
 * \fn iPodPlayerSetDeviceDetection(U32 devID, IPOD_PLAYER_DEVICE_DETECTION_INFO *info);
 * This function is used to inform the device detection to iPodPlayerCore .<br>
 * Result of this API can know by receiving the #IPOD_PLAYER_CB_DEVICE_DETECTION_RESULT<br>
 * \param [in] devID Type is U32. This ID is used to specfy the detected Apple device.<br>
 * \param [in] *info Type is #IPOD_PLAYER_DEVICE_DETECTION_INFO pointer. This is the information of device detection.<br>
 * \retval #IPOD_PLAYER_OK Function success.
 * \retval #IPOD_PLAYER_ERROR Function fail.
 * \warning
 */
S32 iPodPlayerSetDeviceDetection(U32 devID, IPOD_PLAYER_DEVICE_DETECTION_INFO *info);


/*\}*/

#ifdef __cplusplus
}
#endif

#endif /* #ifndef IPOD_PALEYR_DEVICE_DETECTION_H */
