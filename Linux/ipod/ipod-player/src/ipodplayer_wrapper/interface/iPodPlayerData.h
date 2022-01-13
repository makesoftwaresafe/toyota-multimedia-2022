/*! \file iPodPlayerData.h
 *
 * \version: 1.0
 *
 * \author: steranaka
 */
#ifndef IPOD_PALEYR_DATA_H
#define IPOD_PALEYR_DATA_H
#define IPOD_PLAYER_OPEN_QUEUE_WAIT 0xFF
#define IPOD_PLAYER_OPEN_QUEUE_SERVER_LOCAL 0xFE
#define IPOD_PLAYER_OPEN_QUEUE_CLIENT_LOCAL 0xFD
/* ################################### iPod Player Function Header ########################################## */
void iPodPlayerInitSocketInfo(void);
S32 iPodPlayerSetSocketInfo(S32 socketInfo, U8 mode);
S32 iPodPlayerGetSocketInfo(S32 *socketInfo, U8 mode);

#endif /* IPOD_PALEYR_DATA_H */

