/**
 * \file: audio_com_common.h
 *
 *  IPC server for communicate with PulseAudio and applicaiton process
 *
 * \component:  IPC server
 *
 * \author: ADIT
 *
 * \copyright: (c) 2003 - 2011 ADIT Corporation
 *
 */


#ifndef AUDIO_COM_COMMON_H_
#define AUDIO_COM_COMMON_H_

#include "iPodPlayerUtilityLog.h"
#include "iPodPlayerDef.h"
#include "iPodPlayerDef_in.h"

#define IPP_AUDIO_SOCK_PATH "AudioSock"
#define IPP_AUDIO_INFO_IDENTIFY   "pai_com"
#define IPP_AUDIO_INFO_IDENTIFY_MAX 256
#define IPOD_AUDIO_CFG_FILE_NAME                       "IPOD_PLAYER_CFG"
#define IPOD_AUDIO_CFG_ROOT_TAG                        "/ipodplayercfg"
#define IPOD_AUDIO_CFG_DEVICE_MAX_NUM                  "IPOD_DEVICE_MAX_NUM"

#define IPP_AUDIO_SOCK_CLIENT_CONNECT_NUM 1
#define IPP_AUDIO_MAX_BUF_SIZE        2048

#define IPP_AUDIO_OK                  0
#define IPP_AUDIO_ERROR               -1
#define IPP_AUDIO_ERR_NOMEM           -10
#define IPP_AUDIO_ERR_INTERNAL        -11
#define IPP_AUDIO_ERR_BIND            -12
#define IPP_AUDIO_ERR_LISTEN          -13
#define IPP_AUDIO_ERR_ACCEPT          -14
#define IPP_AUDIO_ERR_THREAD          -15
#define IPP_AUDIO_ERR_SOCKET          -16
#define IPP_AUDIO_ERR_CFG             -20
#define IPP_AUDIO_ERR_SETSR           -21
#define IPP_AUDIO_ERR_PA_TMO          -30
#define IPP_AUDIO_ERR_PA_STOPPED      -31
#define IPP_AUDIO_ERR_INVALID         -32
#define IPP_AUDIO_ERR_IPC             -32

#define IPP_AUDIO_DEFAULT_RATE        (44100)
#define IPP_AUDIO_PROTOCOL_BASE 3
#define IPP_AUDIO_PROTOCOL_RETURN 5
#define IPP_AUDIO_MAX_DEVICE_NAME 128

#define IPOD_AUDIO_DLT_CONTEXT                         "AUD"
#define IPOD_AUDIO_DLT_CONTEXT_DSP                     "iPod Audio Context For Logging"
#define IPOD_AUDIO_END_EVENT               1            /* end process event      */
#define IPOD_AUDIO_SEND_RETRY_MAX_NUM      10           /* send event retry times */
#define IPOD_AUDIO_SEND_RETRY_WAIT_TIME    500000       /* retry waite 50 msec    */

#define IPOD_AUDIO_WAIT_SEND 10000

typedef struct IPOD_AUDIO_COM_INFO_ IPOD_AUDIO_COM_INFO;

typedef int  (*iPodAudioComStart)(IPOD_AUDIO_COM_INFO **alsaInfo, U8 *srcName, U8 *sinkName, U32 rate);
typedef void (*iPodAudioComStop)(IPOD_AUDIO_COM_INFO *alsaInfo);
typedef int  (*iPodAudioComSetVolume)(IPOD_AUDIO_COM_INFO *alsaInfo, U8 volume);
typedef int  (*iPodAudioComGetVolume)(IPOD_AUDIO_COM_INFO *alsaInfo, U8 *volume);
typedef int  (*iPodAudioComSetSamplerate)(IPOD_AUDIO_COM_INFO *alsaInfo, U32 rate);

typedef struct
{
    iPodAudioComStart start;
    iPodAudioComStop stop;
    iPodAudioComSetVolume setVolume;
    iPodAudioComGetVolume getVolume;
    iPodAudioComSetSamplerate setSample;
} IPOD_AUDIO_COM_FUNC_TABLE;

/* Audio control parameter */
typedef struct
{
    S32 id;
    S32 audioSckClient;
    IPOD_PLAYER_PARAM_PAI_CFG_TABLE cfgTable;
    IPOD_AUDIO_COM_INFO *audioInfo;
} IPOD_AUDIO_PLAYINFO;

typedef struct
{
    S32 audioSckWait;
    S32 audioSckServer;
    IPOD_PLAYER_MESSAGE_DATA_CONTENTS *pSckIn;  /* input buffer */
    U32 devMax;
    IPOD_AUDIO_PLAYINFO *pinfo;
    IPOD_AUDIO_COM_FUNC_TABLE table;
} IPOD_AUDIO_INFO;

S32 iPodAudioComInit(IPOD_AUDIO_COM_FUNC_TABLE *table);
void iPodAudioComDeinit(IPOD_AUDIO_COM_FUNC_TABLE *table);

#endif /* PAI_COM_SERVER_H_ */

