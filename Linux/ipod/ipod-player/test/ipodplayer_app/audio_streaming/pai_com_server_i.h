/**
 * \file: pai_com_server_i.h
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


#ifndef PAI_COM_SERVER_I_H_
#define PAI_COM_SERVER_I_H_

#include "iPodPlayerIPCLib.h"
#include "iPodPlayerDef.h"
#include "iPodPlayerDef_in.h"
#define PAI_PIPE_BUF            256
#define MODULE_LOOPBACK_STRING "module-loopback"
#define MODULE_ALSA_SOURCE_STRING "module-alsa-source"
#define SOURCE_NAME "ipod_play-source_rate_"
#define PAI_DEFAULT_VOLUME 0xFF;
#define PAI_IPC_ERROR_WAIT_TIME 100000000
#define PAI_VOLUME_MAX 100
#define PAI_CHANGE_RANGE 0.5
#define PAI_CONNECT_NAME "ipod_ctrl"

#define RETRY_CNT    (10)
#define RETRY_INTERVAL    (1)
#define PAI_RETRY_WAIT_TIME (10000000)
#define PAI_RETRY_MAX_NUM (300)
#define PAI_LOOPBACK_RETRY_MAX (5)
#define PAI_LOG_PATH "ipodplayeraudio.log"

#define IPOD_PULSE_MAX_DEVICE_NAME_LEN 128
typedef struct
{
    uint32_t module_idx;                /* Temporary for data of module_index_callback */
    int module_eol;                     /* Temporary for end of module_index_callback */
    uint32_t source_idx;                /* Temporary for data of source_index_callback */
    int source_eol;                     /* Temporary for end of source_index_callback */
    const pa_card_info *card_info;      /* Temporary for data of card_index_callback */
    int card_info_eol;                  /* Temporary for end of card_index_callback */
    const pa_source_info *source_info;  /* Temporary for data of source_index_callback */
    int source_info_eol;                /* Temporary for end of source_index_callback */
    const pa_sink_input_info *sink_info; /* Temporary for data of sink_input_info_callback */
    int sink_info_eol;                  /* Temporary for end of sink_input_info_callback */
    int success;                        /* Temporary for result of success_callback */
    int success_eol;                    /* Temporary for end of success_callback */
} PAI_operationResult;

/* PulseAudio control parameter */
struct IPOD_AUDIO_COM_INFO_
{
    pa_threaded_mainloop *pa_mainloop;
    pa_mainloop_api *pa_mainloop_api;
    pa_context *pa_context;
    int run;                            /* FALSE:context stopped, TRUE:context running */
    pa_context_state_t state;           /* context state callbak status */
    unsigned int card_index;            /* alsa-source card index */
    unsigned int module_index;          /* loopback module index */
    unsigned int module_source_index;   /* alsa-source module index */
    unsigned int source_index;          /* source index */
    unsigned int sink_input_index;      /* loopback sink input index */
    unsigned int rate;                  /* current src sample rate */
    U8 volume;
    PAI_operationResult paTemp;
    U8 src_name[IPOD_PULSE_MAX_DEVICE_NAME_LEN];
    U8 sink_name[IPOD_PULSE_MAX_DEVICE_NAME_LEN];
};

#endif /* PAI_COM_SERVER_I_H_ */

