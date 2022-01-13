/*
 * iap2_transport_dependent.h
 *
 *  Created on: Jul 17, 2013
 *      Author: dgirnus
 */

#ifndef IAP2_TRANSPORT_DEPENDENT_H_
#define IAP2_TRANSPORT_DEPENDENT_H_

#ifdef __cplusplus
extern "C" {
#endif

#define IPOD_USB_HEADER_SIZE            3
#define IPOD_USB_LINK_CTRL_BYTE_SIZE    1

#define IPOD_LINK_CTRL_ONLY_ONE         0x00
#define IPOD_LINK_CTRL_FRST_FOLLOW      0x02
#define IPOD_LINK_CTRL_MIDDLE           0x03
#define IPOD_LINK_CTRL_LAST             0x01

#define IPOD_POS0   0
#define IPOD_POS1   1
#define IPOD_POS2   2
#define IPOD_POS3   3
#define IPOD_POS4   4
#define IPOD_POS5   5
#define IPOD_POS6   6
#define IPOD_POS7   7

#ifdef __cplusplus
}
#endif

#endif /* IAP2_TRANSPORT_DEPENDENT_H_ */
