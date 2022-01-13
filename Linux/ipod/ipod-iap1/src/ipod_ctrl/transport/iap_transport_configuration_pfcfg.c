#include <adit_typedef.h>
#include "iap_devconf.h"
#include "iap_transport_message.h"
//#include "iap_transport_dependent.h"
#include "ipodcommon.h"
#include "iap_transport_configuration.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "pfcfg_interface.h"
#include "iap1_dlt_log.h"




#if defined(IPOD_USB_AUDIO) || defined(IPOD_USB_AUDIO_NO_STREAMING)
#include "iap_transport_audiostreaming.h"
#endif  /* IPOD_USB_AUDIO */

LOCAL S32 iPodGetDcStrParam(VP IPOD_CFG_FileName, U32 i);
LOCAL S32 iPodGetDcNumParam(VP IPOD_CFG_FileName, U32 i);

IPOD_Cfg g_iPodCfg[] =
{
    {(U8 *)IPOD_USB_HID_DEVICE,                   FALSE, {0}, 1, 0},                                       /* 0 */
    {(U8 *)IICDEVICE,                             FALSE, {0}, 1, 0},                                       /* 1 */
    {(U8 *)IPOD_USB_READ_RETRIES,                 TRUE, {0}, 1, 0},                                        /* 2 */
    {(U8 *)IPOD_USB_WAIT_TIMEOUT,                 TRUE, {0}, 1, 0},                                        /* 3 */
    {(U8 *)IPOD_USB_READER_TSK_PRIO,              TRUE, {0}, 1, 0},                                        /* 4 */
    {(U8 *)IPOD_USB_READER_TSK_STACKSIZE,         TRUE, {0}, 1, 0},                                        /* 5 */
    {(U8 *)IPOD_USB_READER_TSK_LCID,              TRUE, {0}, 1, 0},                                        /* 6 */
    {(U8 *)IPOD_WORKER_TSK_PRIO,                  TRUE, {0}, 1, 0},                                        /* 7 */
    {(U8 *)IPOD_WORKER_TSK_STACKSIZE,             TRUE, {0}, 1, 0},                                        /* 8 */
    {(U8 *)IPOD_WORKER_TSK_LCID,                  TRUE, {0}, 1, 0},                                        /* 9 */
    {(U8 *)TSNAME_AUDIOWORKER,                    FALSE, {0}, 1, 0},                                       /* 10 */
    {(U8 *)TSNAME_AUDIOREADER,                    FALSE, {0}, 1, 0},                                       /* 11 */
    {(U8 *)TSNAME_SEM_MODESWITCH,                 FALSE, {0}, 1, 0},                                       /* 12 */
    {(U8 *)TSNAME_SEM_SENDER,                     FALSE, {0}, 1, 0},                                       /* 13 */
    {(U8 *)TSNAME_SEM_AUTHENTICATION,             FALSE, {0}, 1, 0},                                       /* 14 */
    {(U8 *)TSNAME_FLG_WORKER,                     FALSE, {0}, 1, 0},                                       /* 15 */
    {(U8 *)TSNAME_FLG_READER,                     FALSE, {0}, 1, 0},                                       /* 16 */
    {(U8 *)TSNAME_MBF_AUDIO,                      FALSE, {0}, 1, 0},                                       /* 17 */
    {(U8 *)IPOD_TOKEN_IDENTIFY_GENERAL,           TRUE, {0}, 1, 0},                                        /* 18 */
    {(U8 *)IPOD_TOKEN_IDENTIFY_EXTEND,            TRUE, {0}, 1, 0},                                        /* 19 */
    {(U8 *)IPOD_TOKEN_IDENTIFY_AUDIO,             TRUE, {0}, 1, 0},                                        /* 20 */
    {(U8 *)IPOD_TOKEN_IDENTIFY_STORAGE,           TRUE, {0}, 1, 0},                                        /* 21 */
    {(U8 *)IPOD_TOKEN_IDENTIFY_IPODOUT,           TRUE, {0}, 1, 0},                                        /* 22 */
    {(U8 *)IPOD_TOKEN_IDENTIFY_LOCATION,          TRUE, {0}, 1, 0},                                        /* 23 */
    {(U8 *)IPOD_TOKEN_ACCCAPS,                    TRUE, {0}, IPOD_TOKEN_ACC_CAPS_MAX_RANGE, 0},            /* 24 */
    {(U8 *)IPOD_TOKEN_PREFERENCE,                 TRUE, {0}, IPOD_TOKEN_PREFERENCE_MAX_RANGE, 0},          /* 25 */
    {(U8 *)IPOD_TOKEN_PREF_SET_FLG,               TRUE, {0}, IPOD_TOKEN_PREF_SET_FLG_MAX_RANGE, 0},        /* 26 */
    {(U8 *)IPOD_TOKEN_ACCINFO_NAME,               FALSE, {0}, 1, 0},                                       /* 27 */
    {(U8 *)IPOD_TOKEN_ACCINFO_FW_VER,             TRUE, {0}, IPOD_TOKEN_FW_VER_MAX_RANGE, 0},              /* 28 */
    {(U8 *)IPOD_TOKEN_ACCINFO_HW_VER,             TRUE, {0}, IPOD_TOKEN_HW_VER_MAX_RANGE, 0},              /* 29 */
    {(U8 *)IPOD_TOKEN_ACCINFO_MANUFACTURE,        FALSE, {0}, 1, 0},                                       /* 30 */
    {(U8 *)IPOD_TOKEN_ACCINFO_MODEL_NUM,          FALSE, {0}, 1, 0},                                       /* 31 */
    {(U8 *)IPOD_TOKEN_ACCINFO_SERIAL_NUM,         FALSE, {0}, 1, 0},                                       /* 32 */
    {(U8 *)IPOD_TOKEN_ACC_STATUS,                 TRUE, {0}, 1, 0},                                        /* 33*/
    {(U8 *)IPOD_TOKEN_RF_CERTIFICATIONS,          TRUE, {0}, 1, 0},                                        /* 34 */
    {(U8 *)IPOD_TOKEN_ACCINFO_FLG,                TRUE, {0}, IPOD_TOKEN_ACCINFO_FLG_MAX_RANGE, 0},         /* 35 */
    {(U8 *)IPOD_LOCATION_ASSIST_SUPPORT,          TRUE, {0}, 1, 0},                                        /* 36 */
    {(U8 *)IPOD_LOCATION_ASSIST_CAPS,             TRUE, {0}, IPOD_TOKEN_ASSIST_CAPS_MAX_RANGE, 0},         /* 37 */
    {(U8 *)IPOD_LOCATION_SATEL_POS_MAX_REFRESH,   TRUE, {0}, 1, 0},                                        /* 38 */
    {(U8 *)IPOD_LOCATION_SATEL_POS_RECOM_REFRESH, TRUE, {0}, 1, 0},                                        /* 39 */
    {(U8 *)IPOD_OUT_TOTAL_WIDTH_INCHES,           TRUE, {0}, 1, 0},                                        /* 40 */
    {(U8 *)IPOD_OUT_TOTAL_HEIGHT_INCHES,          TRUE, {0}, 1, 0},                                        /* 41 */
    {(U8 *)IPOD_OUT_TOTAL_WIDTH_PIXELS,           TRUE, {0}, 1, 0},                                        /* 42 */
    {(U8 *)IPOD_OUT_TOTAL_HEIGHT_PIXELS,          TRUE, {0}, 1, 0},                                        /* 43 */
    {(U8 *)IPOD_OUT_WIDTH_PIXELS,                 TRUE, {0}, 1, 0},                                        /* 44 */
    {(U8 *)IPOD_OUT_HEIGHT_PIXELS,                TRUE, {0}, 1, 0},                                        /* 45 */
    {(U8 *)IPOD_OUT_FEATURES_MASK,                TRUE, {0}, 1, 0},                                        /* 46 */
    {(U8 *)IPOD_OUT_GAMMA_VALUE,                  TRUE, {0}, 1, 0},                                        /* 47 */
    {(U8 *)IPOD_TOKEN_IDENTIFY_SIMPLE,            TRUE, {0}, 1, 0},                                        /* 48 */
    {(U8 *)IPOD_INST_COUNT,                       TRUE, {0}, 1, 0},                                        /* 49 */
    {(U8 *)IPOD_TOKEN_IDENTIFY_DISPLAY_REMOTE,    TRUE, {0}, 1, 0},                                        /* 50 */
    {(U8 *)IPOD_MAX_PAYLOAD_SIZE,                 TRUE, {0}, 1, 0}                                         /* 51 */
 };

/* Get Number of Devconf parameter */
LOCAL S32 iPodGetDcNumParam(VP IPOD_CFG_FileName, U32 i)
{
    S32 rc = 1;
    U32 j = 0;

    if(g_iPodCfg[i].multi != 1)
    {
        S32 *num = NULL;

        num = calloc(g_iPodCfg[i].multi, sizeof(S32));
        if (NULL != num)
        {
            /* Multiple Devconf setting */
            rc = iPod_util_bGetCfn(IPOD_CFG_FileName, (VP)g_iPodCfg[i].name, num, g_iPodCfg[i].multi);
            if(rc > 0)
            {
                g_iPodCfg[i].count = (U16)rc;
                g_iPodCfg[i].para.p_val = calloc((U32)rc, sizeof(S32));
                if(g_iPodCfg[i].para.p_val != NULL)
                {
                    for(j = 0; j < (U32)rc; j++)
                    {
                        ((S32 *)(VP)g_iPodCfg[i].para.p_val)[j] = num[j];
                       }
                }
                else
                {
                    rc = IPOD_ERR_NOMEM;
                    IAP1_TRANSPORT_LOG(DLT_LOG_ERROR, "No Memory: Pointer to read data from is NULL ");
                }
            }
            else
            {
                rc = IPOD_ERR_ABORT;
                IAP1_TRANSPORT_LOG(DLT_LOG_ERROR, "iPod_util_bGetCfn() returns rc = %d",rc);
            }
            free(num);
        }
        else
        {
           rc = IPOD_ERR_NOMEM;
           IAP1_TRANSPORT_LOG(DLT_LOG_ERROR, "No Memory num is NULL");
        }
    }
    else
    {
        rc = iPod_util_bGetCfn(IPOD_CFG_FileName, (VP)g_iPodCfg[i].name, &g_iPodCfg[i].para.val, g_iPodCfg[i].multi);
        if(rc <= 0)
        {
            if( (i == (U32)IPOD_DC_MAX_PAYLOAD_SIZE) && (rc != IPOD_OK) )
            {
                g_iPodCfg[i].para.val = IPOD_RESPONSE_BUF_SIZE;
                rc = IPOD_OK;
                IAP1_TRANSPORT_LOG(DLT_LOG_DEBUG, "IPOD_MAX_PAYLOAD_SIZE not set in config file. Defaulting to %d", IPOD_RESPONSE_BUF_SIZE);
            }
            else
            {
                rc = IPOD_ERR_ABORT;
                IAP1_TRANSPORT_LOG(DLT_LOG_ERROR, " iPod_util_bGetCfn() returns rc = %d - Aborting, i = %d",rc, i);
            }
        }
    }

    return rc;
}

/* Get String of Devconf parameter */
LOCAL S32 iPodGetDcStrParam(VP IPOD_CFG_FileName, U32 i)
{
    S32 rc = 1;
    U8 str[IPOD_CFG_STR_MAX] = {0};

    rc = iPod_util_bGetCfs(IPOD_CFG_FileName, (VP)g_iPodCfg[i].name, (VP)str, IPOD_CFG_STR_MAX);
    if(rc > 0)
    {
        if(g_iPodCfg[i].multi > 1)
        {
           //To be Implemented. Depend on Utility component
        }
        else
        {
            g_iPodCfg[i].para.p_val = calloc((U32)IPOD_CFG_STR_MAX, sizeof(U8));
            if(g_iPodCfg[i].para.p_val != NULL)
            {
                strncpy((VP)g_iPodCfg[i].para.p_val, (VP)str, IPOD_CFG_STR_MAX);
                g_iPodCfg[i].para.p_val[IPOD_CFG_STR_MAX-1] = '\0';
            }
            else
            {
                rc = IPOD_ERR_NOMEM;
                IAP1_TRANSPORT_LOG(DLT_LOG_ERROR, "No Memory: Pointer to read data from is NULL");
            }
        }
    }
    else
    {
        rc = IPOD_ERR_ABORT;
        IAP1_TRANSPORT_LOG(DLT_LOG_ERROR, "iPod_util_bGetCfs() returns rc = %d - Aborting",rc);
    }

    return rc;
}

S32 iPodGetDevconfParameter()
{
    S32 rc = 1;
    U32 i = 0;
    U8 num_cfg = sizeof(g_iPodCfg)/ sizeof(IPOD_Cfg);
    VP IPOD_CFG_FileName;

    IPOD_CFG_FileName = PFCFG_opn_cnf(IPOD_CONFIGURATION_FILE, PFCFG_READ);
    if(IPOD_CFG_FileName != NULL)
    {
        for(i = 0; ((i < num_cfg) && (rc > IPOD_OK)); i++)
        {
            if(g_iPodCfg[i].isInt != FALSE)
            {
                rc = iPodGetDcNumParam(IPOD_CFG_FileName, i);
            }
            else
            {
                rc = iPodGetDcStrParam(IPOD_CFG_FileName, i);
            }
        }
        PFCFG_close_cnf(IPOD_CFG_FileName);
        if(rc > 0)
        {
            rc = IPOD_OK;
        }
    }
    else
    {
        rc = IPOD_ERROR;
        IAP1_TRANSPORT_LOG(DLT_LOG_ERROR, "IPOD_CFG_FileName is NULL");
    }
    return rc;

}

S32 iPodFreeDevconfParameter()
{
    S32 rc = 0;
    U32 i = 0;
    U32 j = 0;
    U8 num_cfg = sizeof(g_iPodCfg)/ sizeof(IPOD_Cfg);

    for(i = 0; i < num_cfg; i++)
    {
        if(((g_iPodCfg[i].isInt != FALSE) && (g_iPodCfg[i].multi != 1)) || (g_iPodCfg[i].isInt == FALSE))
        {
            if((g_iPodCfg[i].isInt == FALSE) && (g_iPodCfg[i].count > 1))
            {
                if((U8 **)(VP)g_iPodCfg[i].para.p_val != NULL)
                {
                    for(j = 0; j < g_iPodCfg[i].count; j++)
                    {
                        if(((U8 **)(VP)g_iPodCfg[i].para.p_val)[j] != NULL)
                        {
                            free(((U8 **)(VP)g_iPodCfg[i].para.p_val)[j]);
                            ((U8 **)(VP)g_iPodCfg[i].para.p_val)[j] = NULL;
                        }
                    }
                    free((U8 **)(VP)g_iPodCfg[i].para.p_val);
                    g_iPodCfg[i].para.p_val = NULL;
                }
            }
            else
            {
                if(g_iPodCfg[i].para.p_val != NULL)
                {
                    free(g_iPodCfg[i].para.p_val);
                    g_iPodCfg[i].para.p_val = NULL;
                }
            }
               if (((g_iPodCfg[i].isInt != FALSE) && (g_iPodCfg[i].multi != 1)))
               {
                    free((U8 **)(VP)g_iPodCfg[i].para.p_val);
                    g_iPodCfg[i].para.p_val = NULL;
               }
        }
           else
           {
                g_iPodCfg[i].para.p_val = NULL;
           }
    }

    return rc;
}

/*************************************************************************
 * \fn iPod_util_bGetCfn(VP IPOD_CFG_FileName, B *identifier, S32 *int_value, U32 length)
 *
 * \par INPUT PARAMETERS
 * VP IPOD_CFG_FileName .cfg file name to read
 * \par INOUT PARAMETERS
 * B identifier* DEVCONF identifier for the address to read<br>
 * S32 int_value* read integer value (0 in case of error)<br>
 * U32 length length of the number need to copied
 * \par REPLY PARAMETER
 * S32 err -
 * \li \c \b number of defined identifier
 * \li \c \b #IPOD_ERR_NOEXS
 * \par DESCRIPTION
 * Reads numeric value from the device configuration (DEVCONF).
 *
 *************************************************************************/
EXPORT S32 iPod_util_bGetCfn(VP IPOD_CFG_FileName, S8 *identifier,
                             S32 *int_value, U32 length)
{
    S32 err = IPOD_OK;
    PFCFG_ERR retval= PFCFG_get_cfn(IPOD_CFG_FileName,
                                    (VP)identifier,
                                    int_value,
                                    length);
    if(retval >= PFCFG_NO_ERROR)
    {
        err = retval;
    }
    else
    {
        err = IPOD_ERR_ABORT;
        IAP1_TRANSPORT_LOG(DLT_LOG_ERROR, "PFCFG_get_cfn returns retval = %d",retval);
    }

    /* PRQA: Lint Message 429: Allocated memory is freed in other place*/
    return err;/*lint !e429 */
}

/*************************************************************************
 * \fn  iPod_util_bGetCfs(VP IPOD_CFG_FileName, B  *identifier, char *str_value, U32 size)
 *
 * \par INPUT PARAMETERS
  * VP IPOD_CFG_FileName .cfg file name to read
 * U32 size identifier  maximum size of the string
 * \par INOUT PARAMETERS
 * B identifier* - DEVCONF identifier for the address to read<br>
 * UB str_value* - read string value (0 in case of error)
 * \par REPLY PARAMETER
 * \li \c \b number of defined identifier
 * \li \c \b #IPOD_ERR_NOEXS
 * \par DESCRIPTION
 * Reads string value from the device configuration (DEVCONF).
 *
 *************************************************************************/
EXPORT S32 iPod_util_bGetCfs(VP IPOD_CFG_FileName, S8  *identifier,
                             char *str_value,
                             U32 size)
{
    S32 err = IPOD_OK;

       PFCFG_ERR retval = PFCFG_get_cfs (IPOD_CFG_FileName,
                                                                (VP)identifier,
                                                                str_value,
                                                                (S32)size);

    if(retval == PFCFG_NO_ERROR)
       {
            err = strlen(str_value);
       }
       else
    {
        err = IPOD_ERR_ABORT;
        IAP1_TRANSPORT_LOG(DLT_LOG_ERROR, "PFCFG_get_cfs returns retval = %d",retval);
    }

    return err;
}

IPOD_Cfg* iPodGetDevInfo()
{
    return g_iPodCfg;
}
