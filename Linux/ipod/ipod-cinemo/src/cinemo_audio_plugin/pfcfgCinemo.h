/**
 * \file: pfcfgCinemo.h
 *
 * \version: $Id:$
 *
 * \release: $Name:$
 *
 * cinemo audio parameters extraction from pfcfg file
 *
 * \component: ipod cinemo
 *
 * \author: Vanitha.channaiah@in.bosch.com
 *
 * \copyright (c) 2019 Advanced Driver Information Technology.
 * This code is developed by Advanced Driver Information Technology.
 * Copyright of Advanced Driver Information Technology, Bosch, and DENSO.
 * All rights reserved.
 *
 * \see <related items>
 *
 * \history
 *
 ***********************************************************************/
#ifndef PFCFGCINEMO_H_
#define PFCFGCINEMO_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "AudioCommon.h"
#include "pfcfg_interface.h"

#define CINEMO_AUDIO_CFG_NUM_LENGTH 1
#define CINEMO_AUDIO_CFG_STR_MAX 256
#define CINEMO_AUDIO_DEV_NAME "CINEMO_AUDIO_DEV_NAME"
#define CINEMO_AUDIO_BUFFERSIZE "CINEMO_AUDIO_BUFFERSIZE"
#define CINEMO_AUDIO_PERIOFRAMES "CINEMO_AUDIO_PERIOFRAMES"
#define CINEMO_AUDIO_DUMP "CINEMO_AUDIO_DUMP"
#define CINEMO_AUDIO_SAMPLETYPEBITS "CINEMO_AUDIO_SAMPLETYPEBITS"
#define CINEMO_AUDIO_CHANNEL "CINEMO_AUDIO_CHANNEL"
#define CINEMO_CONFIGURATION_FILE "CINEMO_AUDIO_PARAMS"

typedef struct _CINEMO_AUDIO_Cfg
{
    unsigned char *name;
    unsigned char isInt;
    union
    {
        int val;
        unsigned char* p_val;
    }para;
    unsigned int multi;
    unsigned int count;
} CINEMO_AUDIO_Cfg;

typedef enum
{
    CINEMO_AUDIO_ID_DEV_NAME,
    CINEMO_AUDIO_ID_DUMP,
    CINEMO_AUDIO_ID_BUFFERSIZE,
    CINEMO_AUDIO_ID_PERIOFRAMES,
    CINEMO_AUDIO_ID_SAMPLETYPEBITS,
    CINEMO_AUDIO_ID_CHANNEL,
} CINEMO_AUDIO_TYPE;

/* For pfcfg conf file information */
CINEMO_AUDIO_Cfg *cinemoAudioGetPfcfgParameter(unsigned char *num_cfg);
void cinemoAudioFreePfcfgParameter(CINEMO_AUDIO_Cfg *audioCfg, unsigned char num_cfg);
int cinemoAudioPfcfgGetNumParam(void *filename, int i, CINEMO_AUDIO_Cfg *audioCfg);//local
int cinemoAudioPfcfgGetStrParam(void *filename, int i, CINEMO_AUDIO_Cfg *audioCfg);//local
int cinemoAudio_util_bGetCfn(void *filename, char *identifier, int *int_value);
int cinemoAudio_util_bGetCfs(void *filename, char *identifier, char *str_value, int size);

#ifdef __cplusplus
}
#endif
#endif /* PFCFGCINEMO_H_ */
