/*! \file iPodPlayerDebug.h
 *
 * \version: 1.0
 *
 * \author: steranaka
 */

#ifndef IPOD_PLAYER_DEBUG_H
#define IPOD_PLAYER_DEBUG_H

#include <stdio.h>
#include <sys/types.h> 
#include <unistd.h>

#ifdef IPOD_PLAYER_DEBUG
#define IPOD_PLAYER_DEBUG_PRINT(fmt, ...)                        \
  (printf("[PID:%d][%s:%u@%s]:"fmt, getpid(), __FILE__, __LINE__, __func__, ##__VA_ARGS__))
#else
#define IPOD_PLAYER_DEBUG_PRINT(fmt, ...)
#endif
#endif

/* ################################### iPod Player Function Header ########################################## */
