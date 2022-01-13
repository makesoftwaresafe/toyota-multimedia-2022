#include <adit_typedef.h>
#include "pfcfg_interface.h"
#include "iap2_defines.h"
#include "iap2_test_utility.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "iap2_test_configuration_pfcfg.h"

#include "iap2_dlt_log.h"

/***************************************************************************//**
* Get Number from Devconf parameter
*
* \param[in] IAP2TEST_Cfg_FileName .cfg file name to read
* \param[in] i key identifier
* \param[in] iAP2TestCfg pointer to structure holding the values
*            read from config file
* \return A signed integer value indicating success or failure
*
* \see
* \note
******************************************************************************/
LOCAL S32 iAP2TestGetDevConfNumParam(VP IAP2TEST_Cfg_FileName, U32 i, IAP2TEST_Cfg* iAP2TestCfg);

/***************************************************************************//**
* Get String from Devconf parameter
*
* \param[in] IAP2TEST_Cfg_FileName .cfg file name to read
* \param[in] i key identifier
* \param[in] iAP2TestCfg pointer to structure holding the values
*            read from config file
* \return A signed integer value indicating success or failure
*
* \see
* \note
******************************************************************************/
LOCAL S32 iAP2TestGetDevConfStrParam(VP IAP2TEST_Cfg_FileName, U32 i, IAP2TEST_Cfg* iAP2TestCfg);

/***************************************************************************//**
* Reads numeric value from the device configuration.
*
* \param[in] IAP2TEST_Cfg_FileName .cfg file name to read
* \param[in] *identifier identifier for the address to read
* \param[in] *int_value read integer value (0 in case of error)
* \return A signed integer value indicating success or failure
*
* \see
* \note
******************************************************************************/
LOCAL S32 iAP2Test_util_bGetCfn(VP IAP2TEST_Cfg_FileName, S8  *identifier,
                                   S32 * int_value);

/***************************************************************************//**
* Reads string value from the device configuration
*
* \param[in] IAP2TEST_Cfg_FileName .cfg file name to read
* \param[in] *identifier identifier for the address to read
* \param[in] *str_value read string value (0 in case of error)
* \param[in]  size size of the string needs to copied
* \return A signed integer value indicating success or failure
*
* \see
* \note
******************************************************************************/
LOCAL S32 iAP2Test_util_bGetCfs(VP IAP2TEST_Cfg_FileName, S8  *identifier,
                                   char* str_value,
                                   U32 size);



LOCAL S32 iAP2TestGetDevConfNumParam(VP IAP2TEST_Cfg_FileName, U32 i, IAP2TEST_Cfg* p_iAP2TestCfg)
{
    S32 rc = 1;

    rc = iAP2Test_util_bGetCfn(IAP2TEST_Cfg_FileName,
            (VP) p_iAP2TestCfg[i].name, &p_iAP2TestCfg[i].para.val);
    IAP2TESTDLTLOG(DLT_LOG_DEBUG, "Identifier = %s value = %d",p_iAP2TestCfg[i].name,p_iAP2TestCfg[i].para.val);
    if (rc <= 0)
    {
        rc = IAP2_ERR_ABORT;
    }

    return rc;
}

LOCAL S32 iAP2TestGetDevConfStrParam(VP IAP2TEST_Cfg_FileName, U32 i, IAP2TEST_Cfg* p_iAP2TestCfg)
{
    S32 rc = 1;
    U8 str[IAP2_CFG_STR_MAX] = { 0 };

    rc = iAP2Test_util_bGetCfs(IAP2TEST_Cfg_FileName,
            (VP) p_iAP2TestCfg[i].name, (VP) str, IAP2_CFG_STR_MAX);
    if (rc > 0)
    {
        p_iAP2TestCfg[i].para.p_val = calloc((U32) IAP2_CFG_STR_MAX, sizeof(U8));
        if (p_iAP2TestCfg[i].para.p_val != NULL)
        {
            strncpy((VP) p_iAP2TestCfg[i].para.p_val, (VP) str, IAP2_CFG_STR_MAX);
            p_iAP2TestCfg[i].para.p_val[IAP2_CFG_STR_MAX - 1] = '\0';
            IAP2TESTDLTLOG(DLT_LOG_DEBUG, "Identifier = %s value = %s",p_iAP2TestCfg[i].name,p_iAP2TestCfg[i].para.p_val);
        }
        else
        {
            rc = IAP2_ERR_NO_MEM;
        }
    }
    else
    {
        rc = IAP2_ERR_ABORT;
    }

    return rc;
}

IAP2TEST_Cfg * iAP2TestGetDevconfParameter(U8* num_cfg)
{
    S32 rc = 1;
    U32 i = 0;
    VP IAP2TEST_Cfg_FileName;
    IAP2TEST_Cfg *p_iAP2TestCfg = NULL;
    IAP2TEST_Cfg iAP2TestCfg[] =
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

    *num_cfg = sizeof(iAP2TestCfg)/sizeof(IAP2TEST_Cfg);
    p_iAP2TestCfg = (IAP2TEST_Cfg *)calloc((*num_cfg), sizeof(IAP2TEST_Cfg));
    memcpy(p_iAP2TestCfg, iAP2TestCfg, (*num_cfg)*sizeof(IAP2TEST_Cfg));

    IAP2TEST_Cfg_FileName = PFCFG_opn_cnf(IAP2_AUTH_CONFIGURATION_FILE, PFCFG_READ);
    if (IAP2TEST_Cfg_FileName != NULL)
    {
        for (i = 0; ((i < (*num_cfg)) && (rc > IAP2_OK)); i++)
        {
            if (iAP2TestCfg[i].isInt != FALSE)
            {
                rc = iAP2TestGetDevConfNumParam(IAP2TEST_Cfg_FileName, i, p_iAP2TestCfg);
            }
            else
            {
                rc = iAP2TestGetDevConfStrParam(IAP2TEST_Cfg_FileName, i, p_iAP2TestCfg);
            }
        }
        PFCFG_close_cnf(IAP2TEST_Cfg_FileName);
        if (rc > 0)
        {
            rc = IAP2_OK;
        }
        else
        {
            iAP2TestFreeDevconfParameter(p_iAP2TestCfg, *num_cfg);
            p_iAP2TestCfg = NULL;
        }
    }
    else
    {
        IAP2TESTDLTLOG(DLT_LOG_ERROR, "Failed to open the config file");
        rc = IAP2_ERR_ABORT;
    }
    return p_iAP2TestCfg;

}

void iAP2TestFreeDevconfParameter(IAP2TEST_Cfg *p_iAP2TestCfg, U8 num_cfg)
{
    S32 i = 0;

    for(i = 0; i < num_cfg; i++)
    {
        if (p_iAP2TestCfg[i].isInt == FALSE)
        {
            iap2TestFreePtr((void**)&p_iAP2TestCfg[i].para.p_val);
        }
    }

    iap2TestFreePtr((void**)&p_iAP2TestCfg);
}

LOCAL S32 iAP2Test_util_bGetCfn(VP IAP2TEST_Cfg_FileName, S8 *identifier,
                             S32 *int_value)
{
    S32 err = IAP2_OK;
    PFCFG_ERR retval = PFCFG_get_cfn(IAP2TEST_Cfg_FileName, (VP) identifier,
            int_value, IAP2_CFG_NUM_LENGTH);

    if (retval >= PFCFG_NO_ERROR)
    {
        err = retval;
    }
    else
    {
        IAP2TESTDLTLOG(DLT_LOG_ERROR, "Failed to read the value of %s field, retval = %d",(U8*)identifier,retval);
        err = IAP2_ERR_ABORT;
    }

    return err;
}

LOCAL S32 iAP2Test_util_bGetCfs(VP IAP2TEST_Cfg_FileName, S8  *identifier,
                             char *str_value,
                             U32 size)
{
    S32 err = IAP2_OK;

    PFCFG_ERR retval = PFCFG_get_cfs(IAP2TEST_Cfg_FileName, (VP) identifier,
            str_value, (S32) size);

    if (retval == PFCFG_NO_ERROR)
    {
        err = strlen(str_value);
    }
    else
    {
        IAP2TESTDLTLOG(DLT_LOG_ERROR, "Failed to read the value of %s field, retval = %d",(U8*)identifier,retval);
        err = IAP2_ERR_ABORT;
    }

    return err;
}

