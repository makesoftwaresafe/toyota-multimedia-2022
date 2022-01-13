/*
 * iap_authentication_configuration.c
 *
 *  Created on: Jun 21, 2013
 *      Author: dgirnus
 */

#include <adit_typedef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pfcfg_interface.h>

#include "iap2_dlt_log.h"
#include "authentication.h"
#include "authentication_configuration.h"

LOCAL S32 AuthenticationSetStrParam(AuthenticationConfig_t* AuthConfigValues, U32 i);
LOCAL S32 AuthenticationSetNumParam(AuthenticationConfig_t* AuthConfigValues, U32 i);
LOCAL S32 AuthenticationGetDcStrParam(VP IPOD_AUTH_CFG_FileName, U32 i);
LOCAL S32 AuthenticationGetDcNumParam(VP IPOD_AUTH_CFG_FileName, U32 i);


IPOD_AUTH_Cfg g_AuthCfg[] =
{
    {(U8 *)IPOD_AUTH_DEV_NAME,                    FALSE,{0}, 1, 0, FALSE},     /* 1 */
    {(U8 *)IPOD_AUTH_IOCTL_REG,                   FALSE,{0}, 1, 0, FALSE},     /* 2 */
    {(U8 *)IPOD_AUTH_GPIO_RESET,                  FALSE,{0}, 1, 0, FALSE},     /* 3 */
    {(U8 *)IPOD_AUTH_GPIO_READY,                  FALSE,{0}, 1, 0, FALSE},     /* 4 */
    {(U8 *)IPOD_AUTH_DEV_COM_SHORT_WAIT,          TRUE, {0}, 1, 0, FALSE},     /* 5 */
    {(U8 *)IPOD_AUTH_DEV_COM_WAIT,                TRUE, {0}, 1, 0, FALSE},     /* 6 */
    {(U8 *)IPOD_AUTH_DEV_COM_LONG_WAIT,           TRUE, {0}, 1, 0, FALSE},     /* 7 */
    {(U8 *)IPOD_AUTH_CP_AUTODETECT,               FALSE,{0}, 1, 0, FALSE}      /* 8 */
};

BOOL g_bIsConfigured = FALSE;


/*
 * \fn Authentication_util_bGetCfn(VP IPOD_AUTH_CFG_FileName, B *identifier, S32 *int_value, U32 length)
 *
 * \par INPUT PARAMETERS
 * VP IPOD_AUTH_CFG_FileName .cfg file name to read
 * \par INOUT PARAMETERS
 * B identifier* DEVCONF identifier for the address to read<br>
 * S32 int_value* read integer value (0 in case of error)<br>
 * U32 length length of the number need to copied
 * \par REPLY PARAMETER
 * S32 err -
 * \li \c \b number of defined identifier
 * \li \c \b #IPOD_AUTH_ERR_ABORT
 * \par DESCRIPTION
 * Reads numeric value from the device configuration (DEVCONF).
 *
 */
S32 Authentication_util_bGetCfn(VP IPOD_AUTH_CFG_FileName, S8 *identifier,
                                S32 *int_value, U32 length)
{
    S32 err = IPOD_AUTH_OK;
    PFCFG_ERR retval= PFCFG_get_cfn(IPOD_AUTH_CFG_FileName,
                                    (VP)identifier,
                                    int_value,
                                    length);
    if(retval >= PFCFG_NO_ERROR)
    {
        err = retval;
    }
    else
    {
        err = IPOD_AUTH_ERR_ABORT;
        IAP2AUTHDLTLOG(DLT_LOG_ERROR, "PFCFG_get_cfn returns %d",err);
    }

    /* PRQA: Lint Message 429: Allocated memory is freed in other place*/
    return err;/*lint !e429 */
}


/*
 * \fn  Authentication_util_bGetCfs(VP IPOD_AUTH_CFG_FileName, B  *identifier, char *str_value, U32 size)
 *
 * \par INPUT PARAMETERS
  * VP IPOD_AUTH_CFG_FileName .cfg file name to read
 * U32 size identifier  maximum size of the string
 * \par INOUT PARAMETERS
 * B identifier* - DEVCONF identifier for the address to read<br>
 * UB str_value* - read string value (0 in case of error)
 * \par REPLY PARAMETER
 * \li \c \b number of defined identifier
 * \li \c \b #IPOD_AUTH_ERR_ABORT
 * \par DESCRIPTION
 * Reads string value from the device configuration (DEVCONF).
 *
 */
S32 Authentication_util_bGetCfs(VP IPOD_AUTH_CFG_FileName, S8  *identifier,
                                char *str_value, U32 size)
{
    S32 err = IPOD_AUTH_OK;

    PFCFG_ERR retval = PFCFG_get_cfs(IPOD_AUTH_CFG_FileName,
                                    (VP)identifier,
                                    str_value,
                                    (S32)size);

    if(retval == PFCFG_NO_ERROR)
    {
        err = strlen(str_value);
    }
    else
    {
        err = IPOD_AUTH_ERR_ABORT;
        IAP2AUTHDLTLOG(DLT_LOG_ERROR, "PFCFG_get_cfs returns %d",err);
    }

    return err;
}


/* Get Number of Devconf parameter */
LOCAL S32 AuthenticationGetDcNumParam(VP IPOD_AUTH_CFG_FileName, U32 i)
{
    S32 rc = 1;
    U32 j = 0;

    if(g_AuthCfg[i].multi != 1)
    {
        S32 *num = NULL;

        /* The allocated memory kept alive until the process dies and cleans itself. */
        num = calloc(g_AuthCfg[i].multi, sizeof(S32));
        if (NULL != num)
        {
            /* Multiple Devconf setting */
            rc = Authentication_util_bGetCfn(IPOD_AUTH_CFG_FileName, (VP)g_AuthCfg[i].name, num, g_AuthCfg[i].multi);
            if(rc > 0)
            {
                g_AuthCfg[i].count = (U16)rc;
                /* The allocated memory kept alive until the process dies and cleans itself. */
                g_AuthCfg[i].para.p_val = calloc((U32)rc, sizeof(S32));
                if(g_AuthCfg[i].para.p_val != NULL)
                {
                    for(j = 0; j < (U32)rc; j++)
                    {
                        ((S32 *)(VP)g_AuthCfg[i].para.p_val)[j] = num[j];
                    }
                }
                else
                {
                    rc = IPOD_AUTH_ERR_NOMEM;
                }
            }
            else
            {
                rc = IPOD_AUTH_ERR_ABORT;
            }
            free(num);
        }
        else
        {
           rc = IPOD_AUTH_ERR_NOMEM;
           IAP2AUTHDLTLOG(DLT_LOG_ERROR, "No Memory");
        }
    }
    else
    {
        rc = Authentication_util_bGetCfn(IPOD_AUTH_CFG_FileName, (VP)g_AuthCfg[i].name, &g_AuthCfg[i].para.val, g_AuthCfg[i].multi);
        if(rc <= 0)
        {
          rc = IPOD_AUTH_ERR_ABORT;
          IAP2AUTHDLTLOG(DLT_LOG_ERROR, " Authentication_util_bGetCfn returns %d",rc);
        }
    }

    return rc;
}


/* Get String of Devconf parameter */
LOCAL S32 AuthenticationGetDcStrParam(VP IPOD_AUTH_CFG_FileName, U32 i)
{
    S32 rc = 1;
    U8 str[IPOD_AUTH_CFG_STR_MAX + IPOD_NULL_CHAR_LEN] = {0};

    rc = Authentication_util_bGetCfs(IPOD_AUTH_CFG_FileName, (VP)g_AuthCfg[i].name, (VP)str, IPOD_AUTH_CFG_STR_MAX);
    if(rc > 0)
    {
        if(g_AuthCfg[i].multi > 1)
        {
           /* To be Implemented. Depend on Utility component */
        }
        else
        {
            /* The allocated memory kept alive until the process dies and cleans itself. */
            g_AuthCfg[i].para.p_val = calloc((U32)IPOD_AUTH_CFG_STR_MAX, sizeof(U8));
            if(g_AuthCfg[i].para.p_val != NULL)
            {
                strncpy((VP)g_AuthCfg[i].para.p_val, (VP)str, IPOD_AUTH_CFG_STR_MAX);
                g_AuthCfg[i].para.p_val[IPOD_AUTH_CFG_STR_MAX-1] = '\0';
            }
            else
            {
                rc = IPOD_AUTH_ERR_NOMEM;
                IAP2AUTHDLTLOG(DLT_LOG_ERROR, "No Memory");
            }
        }
    }
    else
    {
        rc = IPOD_AUTH_ERR_ABORT;
        IAP2AUTHDLTLOG(DLT_LOG_ERROR, "Authentication_util_bGetCfs returns %d",rc);
    }

    return rc;
}


/* get integer values from Application */
LOCAL S32 AuthenticationSetNumParam(AuthenticationConfig_t* AuthConfigValues, U32 i)
{
    S32 rc = IPOD_AUTH_OK;

    if(g_AuthCfg[i].multi != 1)
    {
       /* To be Implemented. Depend on Utility component */
    }
    else
    {
        switch(i)
        {
            case IPOD_AUTH_DC_DEV_COM_SHORT_WAIT:
            {
                g_AuthCfg[i].para.val = AuthConfigValues->iPodAuthDevComShortWait;
                g_AuthCfg[i].setValue = TRUE;
                break;
            }
            case IPOD_AUTH_DC_DEV_COM_WAIT:
            {
                g_AuthCfg[i].para.val = AuthConfigValues->iPodAuthDevComWait;
                g_AuthCfg[i].setValue = TRUE;
                break;
            }
            case IPOD_AUTH_DC_DEV_COM_LONG_WAIT:
            {
                g_AuthCfg[i].para.val = AuthConfigValues->iPodAuthDevComLongWait;
                g_AuthCfg[i].setValue = TRUE;
                break;
            }
            default:
            {
                rc = IPOD_AUTH_ERROR;
                IAP2AUTHDLTLOG(DLT_LOG_ERROR, "AuthenticationSetNumParam returns %d",rc);
                break;
            }
        }
    }

    return rc;
}


/* get string values from Application */
LOCAL S32 AuthenticationSetStrParam(AuthenticationConfig_t* AuthConfigValues, U32 i)
{
    S32 rc = IPOD_AUTH_OK;

    if(g_AuthCfg[i].multi > 1)
    {
       /* To be Implemented. Depend on Utility component */
    }
    else
    {
        /* The allocated memory kept alive until the process dies and cleans itself. */
        g_AuthCfg[i].para.p_val = calloc((U32)(IPOD_AUTH_CFG_STR_MAX + IPOD_NULL_CHAR_LEN), sizeof(U8));
        if(g_AuthCfg[i].para.p_val != NULL)
        {
            switch(i)
            {
                case IPOD_AUTH_DC_DEV_NAME:
                {
                    strncpy((VP)g_AuthCfg[i].para.p_val, (VP)AuthConfigValues->iPodAuthDevicename, IPOD_AUTH_CFG_STR_MAX);
                    g_AuthCfg[i].setValue = TRUE;
                    break;
                }
                case IPOD_AUTH_DC_IOCTL:
                {
                    strncpy((VP)g_AuthCfg[i].para.p_val, (VP)AuthConfigValues->iPodAuthIoctlRegAddr, IPOD_AUTH_CFG_STR_MAX);
                    g_AuthCfg[i].setValue = TRUE;
                    break;
                }
                case IPOD_AUTH_DC_GPIO_RESET:
                {
                    strncpy((VP)g_AuthCfg[i].para.p_val, (VP)AuthConfigValues->iPodAuthGPIOReset, IPOD_AUTH_CFG_STR_MAX);
                    g_AuthCfg[i].setValue = TRUE;
                    break;
                }
                case IPOD_AUTH_DC_GPIO_READY:
                {
                    strncpy((VP)g_AuthCfg[i].para.p_val, (VP)AuthConfigValues->iPodAuthGPIOReady, IPOD_AUTH_CFG_STR_MAX);
                    g_AuthCfg[i].setValue = TRUE;
                    break;
                }
                case IPOD_AUTH_DC_CP_AUTODETECT:
                {
                    strncpy((VP)g_AuthCfg[i].para.p_val, (VP)AuthConfigValues->iPodAuthAutoDetect, IPOD_AUTH_CFG_STR_MAX);
                    g_AuthCfg[i].setValue = TRUE;
                    break;
                }
                default:
                {
                    rc = IPOD_AUTH_ERROR;
                    IAP2AUTHDLTLOG(DLT_LOG_ERROR, " returns %d",rc);
                    break;
                }
            }
            if(rc == IPOD_AUTH_OK)
            {
                g_AuthCfg[i].para.p_val[IPOD_AUTH_CFG_STR_MAX] = '\0';
            }
        }
        else
        {
            rc = IPOD_AUTH_ERR_NOMEM;
            IAP2AUTHDLTLOG(DLT_LOG_ERROR, "No memory");
        }
    }

    return rc;
}


/* Use the configuration values from the Application.
 * Save the configuration values into IPOD_AUTH_Cfg g_AuthCfg[]. */
S32 AuthenticationSetDevconfParameter(AuthenticationConfig_t* AuthConfigValues)
{
    S32 rc = IPOD_AUTH_OK;
    U32 i = 0;
    U8 num_cfg = sizeof(g_AuthCfg)/ sizeof(IPOD_AUTH_Cfg);

    /* Check if configuration was already set by previous authentication process. */
    if(TRUE == g_bIsConfigured){
        rc = IPOD_AUTH_OK;
    } else{

        for(i = 0; ((i < num_cfg) && (rc == IPOD_AUTH_OK)); i++)
        {
            if(g_AuthCfg[i].isInt != FALSE)
            {
                /* get integer values */
                rc = AuthenticationSetNumParam(AuthConfigValues, i);
            }
            else
            {
                /* get string values */
                rc = AuthenticationSetStrParam(AuthConfigValues, i);
            }
        }
        if(IPOD_AUTH_OK == rc){
            g_bIsConfigured = TRUE;
        }
    }

    return rc;
}

/* Reads the PFCFG file to get the configuration values.
 * Save the configuration values into IPOD_AUTH_Cfg g_AuthCfg[]. */
S32 AuthenticationGetDevconfParameter()
{
    S32 rc = 1;
    U32 i = 0;
    U8 num_cfg = sizeof(g_AuthCfg)/ sizeof(IPOD_AUTH_Cfg);
    VP IPOD_AUTH_CFG_FileName = NULL;
    if(TRUE == g_bIsConfigured){
        rc = IPOD_AUTH_OK;
    } else{

        for(i = 0; ((i < num_cfg) && (rc > IPOD_AUTH_OK)); i++)
        {
            /* check if configuration was set by application */
            if(g_AuthCfg[i].setValue == FALSE)
            {
                if(IPOD_AUTH_CFG_FileName == NULL)
                {
                    IPOD_AUTH_CFG_FileName = PFCFG_opn_cnf(IPOD_AUTH_CONFIGURATION_FILE, PFCFG_READ);
                }
                if(IPOD_AUTH_CFG_FileName != NULL)
                {
                    if(g_AuthCfg[i].isInt != FALSE)
                    {
                        rc = AuthenticationGetDcNumParam(IPOD_AUTH_CFG_FileName, i);
                    }
                    else
                    {
                        rc = AuthenticationGetDcStrParam(IPOD_AUTH_CFG_FileName, i);
                    }
                }
                else
                {
                    rc = IPOD_AUTH_ERROR;
                    IAP2AUTHDLTLOG(DLT_LOG_ERROR, "IPOD_AUTH_CFG_FileName is NULL");
                }
            }
        }

        if(IPOD_AUTH_CFG_FileName != NULL)
        {
            PFCFG_close_cnf(IPOD_AUTH_CFG_FileName);
        }
        if(rc > 0)
        {
            rc = IPOD_AUTH_OK;
            g_bIsConfigured = TRUE;
        }
    }
    return rc;

}


/*
 * \fn  AuthenticationGetDevInfo()
 *
 * \par INPUT PARAMETERS
 *
 * \par INOUT PARAMETERS
 *
 * \par REPLY PARAMETER
 * \li \c \b IPOD_AUTH_Cfg g_AuthCfg - structure with configuration values
 * \par DESCRIPTION
 * Return pointer to the authentication configuration (DEVCONF).
 *
 */
IPOD_AUTH_Cfg* AuthenticationGetDevInfo()
{
    return g_AuthCfg;
}
