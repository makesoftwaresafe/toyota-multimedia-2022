/*! \file iPodPlayerMain.h
 *
 * \version: 1.0
 *
 * \author: mshibata
 */
#ifndef IPOD_PALEYR_THREAD_H
#define IPOD_PALEYR_THREAD_H
#include "iPodPlayerCB.h"
/* ################################### iPod Player Function Header ########################################## */
void* iPodPlayerThredCB(void* args);
S32 iPodPlayerThreadCreate(U8 *name, IPOD_PLAYER_REGISTER_CB_TABLE *cbTable);
S32 iPodPlayerThreadDestroy(void);

#endif /* IPOD_PALEYR_MAIN_H */

