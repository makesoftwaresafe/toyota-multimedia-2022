/**
 * \file: pfcfgCinemo.c
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
 *          Rahul.babu@in.bosch.com
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

#include "pfcfgCinemo.h"

CINEMO_AUDIO_Cfg *cinemoAudioGetPfcfgParameter(unsigned char *num_cfg)
{
    int err = -1;
    void *filename;
    CINEMO_AUDIO_Cfg *pAudioCfg = NULL;
    CINEMO_AUDIO_Cfg audioCfg[] =
    {
            {(unsigned char *)CINEMO_AUDIO_DEV_NAME,                     FALSE,{0}, 1, 0},
            {(unsigned char *)CINEMO_AUDIO_DUMP,                         FALSE,{0}, 1, 0},
            {(unsigned char *)CINEMO_AUDIO_BUFFERSIZE,                   TRUE,{0},  1, 0},
            {(unsigned char *)CINEMO_AUDIO_PERIOFRAMES,                  TRUE,{0},  1, 0},
            {(unsigned char *)CINEMO_AUDIO_SAMPLETYPEBITS,               TRUE,{0},  1, 0},
            {(unsigned char *)CINEMO_AUDIO_CHANNEL,                      TRUE,{0},  1, 0}
    };
    *num_cfg = sizeof(audioCfg) / sizeof(CINEMO_AUDIO_Cfg);
    pAudioCfg = (CINEMO_AUDIO_Cfg *)calloc((*num_cfg), sizeof(CINEMO_AUDIO_Cfg));

    if (pAudioCfg)
    {
        memcpy(pAudioCfg, audioCfg, *num_cfg * sizeof(CINEMO_AUDIO_Cfg));
    }

    filename = PFCFG_opn_cnf(CINEMO_CONFIGURATION_FILE, PFCFG_READ);
    if (filename != NULL)
    {
        for (int i = 0; (i < *num_cfg); i++)
        {
            if (audioCfg[i].isInt != FALSE)
            {
                err = cinemoAudioPfcfgGetNumParam(filename, i, pAudioCfg);
            }
            else
            {
                err = cinemoAudioPfcfgGetStrParam(filename, i, pAudioCfg);
            }
        }
        PFCFG_close_cnf(filename);
        if (err > 0)
        {
            err = CTLI_NO_ERROR;
        }
        else
        {
            cinemoAudioFreePfcfgParameter(pAudioCfg, *num_cfg);
        }
    }
    else
    {
        fprintf(stderr, "Cinemo pfcfg FileName is NULL\n");
    }
    return pAudioCfg;
}

void cinemoAudioFreePfcfgParameter(CINEMO_AUDIO_Cfg *audioCfg, unsigned char num_cfg)
{
    int i = 0;

    for (i = 0; i < num_cfg; i++)
    {
        if (audioCfg[i].isInt == FALSE)
        {
            free(audioCfg[i].para.p_val);
            audioCfg[i].para.p_val = NULL;
        }
    }

    free(audioCfg);
    audioCfg = NULL;
}

/* Get numeric value of Devconf parameter */
int cinemoAudioPfcfgGetNumParam(void *filename, int i, CINEMO_AUDIO_Cfg *pAudioCfg)
{
    int rc = 1;
    rc = cinemoAudio_util_bGetCfn(filename, (void *)pAudioCfg[i].name, &pAudioCfg[i].para.val);
    if (rc <= 0)
    {
        rc = -1;
    }
    return rc;
}

/* Get String of Devconf parameter */
int cinemoAudioPfcfgGetStrParam(void *filename, int i, CINEMO_AUDIO_Cfg *pAudioCfg)
{
    int err = 0;
    unsigned char str[CINEMO_AUDIO_CFG_STR_MAX] = { 0 };

    err = cinemoAudio_util_bGetCfs(filename, (void *)pAudioCfg[i].name, (void *)str, CINEMO_AUDIO_CFG_STR_MAX);
    if (err > 0)
    {
        pAudioCfg[i].para.p_val = calloc((unsigned int) CINEMO_AUDIO_CFG_STR_MAX, sizeof(unsigned char));
        if (pAudioCfg[i].para.p_val != NULL)
        {
            strncpy((void *) pAudioCfg[i].para.p_val, (void *) str, CINEMO_AUDIO_CFG_STR_MAX);
            pAudioCfg[i].para.p_val[CINEMO_AUDIO_CFG_STR_MAX - 1] = '\0';
        }
        else
        {
            err = -1;
        }
    }
    else
    {
        err = -1;
    }
    return err;
}

/**
 * Reads numeric value from the device configuration (DEVCONF).
 */
int cinemoAudio_util_bGetCfn(void *filename, char *identifier, int *int_value)
{
    int err = 0;
    PFCFG_ERR retval= PFCFG_get_cfn(filename,
                                    (void *)identifier,
                                    int_value,
                                    CINEMO_AUDIO_CFG_NUM_LENGTH);
    if (retval >= PFCFG_NO_ERROR)
    {
        err = retval;
    }
    else
    {
        err = -1;
        IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "PFCFG_get_cfn returns retval = %d",retval);
    }

    return err;
}

/**
 * Reads string value from the device configuration (DEVCONF).
 */
int cinemoAudio_util_bGetCfs(void *filename, char *identifier,
                             char *str_value,
                             int size)
{
    int err = 0;

    PFCFG_ERR retval = PFCFG_get_cfs (filename, (void *)identifier, str_value, (int)size);

    if (retval == PFCFG_NO_ERROR)
    {
        err = strlen(str_value);
    }
    else
    {
        err = -1;
        IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "PFCFG_get_cfn returns retval = %d",retval);
    }

    return err;
}
