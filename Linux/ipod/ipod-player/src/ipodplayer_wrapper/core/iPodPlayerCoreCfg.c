#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "adit_typedef.h"
#include "iPodPlayerUtilityLog.h"
#include "iPodPlayerCoreDef.h"
#include "iPodPlayerCoreCfg.h"

/* iPodPlayer Configuration table */
static IPOD_PLAYER_CORE_CFG_INFO g_iPodPlayerCfg[] =
{
    { (U8 *)IPOD_PLAYER_CFG_PLAYER_TMOUT,                   IPOD_UTIL_CFG_INT_VALUE,   1,                             {0} },
    { (U8 *)IPOD_PLAYER_CFG_DEVICE_MAX_NUM,                 IPOD_UTIL_CFG_INT_VALUE,   1,                             {0} },
    { (U8 *)IPOD_PLAYER_CFG_APP_MAX_NUM,                    IPOD_UTIL_CFG_INT_VALUE,   1,                             {0} },
    { (U8 *)IPOD_PLAYER_CFG_DEFAULT_PROTOCOL,               IPOD_UTIL_CFG_INT_VALUE,   1,                             {0} },
    { (U8 *)IPOD_PLAYER_CFG_SUPPORT_IAP1,                   IPOD_UTIL_CFG_INT_VALUE,   1,                             {0} },
    { (U8 *)IPOD_PLAYER_CFG_SELF_AUTHENTICATE,              IPOD_UTIL_CFG_INT_VALUE,   1,                             {0} },
    { (U8 *)IPOD_PLAYER_CFG_MAX_ENTRY_NUM,                  IPOD_UTIL_CFG_INT_VALUE,   1,                             {0} },
    { (U8 *)IPOD_PLAYER_CFG_MAX_MSG_SIZE,                   IPOD_UTIL_CFG_INT_VALUE,   1,                             {0} },
    { (U8 *)IPOD_PLAYER_CFG_MAX_QUEUE_SIZE,                 IPOD_UTIL_CFG_INT_VALUE,   1,                             {0} },
    { (U8 *)IPOD_PLAYER_CFG_AUDIO_OUTPUT_NAME,              IPOD_UTIL_CFG_STR_VALUE,   1,                             {0} },
    { (U8 *)IPOD_PLAYER_CFG_AUDIO_DEFAULT_SAMPLE,           IPOD_UTIL_CFG_INT_VALUE,   1,                             {0} },
    { (U8 *)IPOD_PLAYER_CFG_AUDIO_SERVER_NAME,              IPOD_UTIL_CFG_STR_VALUE,   1,                             {0} },
    { (U8 *)IPOD_PLAYER_CFG_AUDIO_THREAD_SIZE,              IPOD_UTIL_CFG_INT_VALUE,   1,                             {0} },
    { (U8 *)IPOD_PLAYER_CFG_CTRL_READ_RETRIES,              IPOD_UTIL_CFG_INT_VALUE,   1,                             {0} },
    { (U8 *)IPOD_PLAYER_CFG_CTRL_THREAD_SIZE,               IPOD_UTIL_CFG_INT_VALUE,   1,                             {0} },
    { (U8 *)IPOD_PLAYER_CFG_CTRL_READER_PRIO,               IPOD_UTIL_CFG_INT_VALUE,   1,                             {0} },
    { (U8 *)IPOD_PLAYER_CFG_CTRL_WORKER_THREAD_SIZE,        IPOD_UTIL_CFG_INT_VALUE,   1,                             {0} },
    { (U8 *)IPOD_PLAYER_CFG_CTRL_WORKER_PRIO,               IPOD_UTIL_CFG_INT_VALUE,   1,                             {0} },
    { (U8 *)IPOD_PLAYER_CFG_CTRL_WORKER_FLAG_NAME,          IPOD_UTIL_CFG_STR_VALUE,   1,                             {0} },
    { (U8 *)IPOD_PLAYER_CFG_CTRL_READER_FLAG_NAME,          IPOD_UTIL_CFG_STR_VALUE,   1,                             {0} },
    { (U8 *)IPOD_PLAYER_CFG_CTRL_WAIT_TIMEOUT,              IPOD_UTIL_CFG_INT_VALUE,   1,                             {0} },
    { (U8 *)IPOD_PLAYER_CFG_TOKEN_ACCINFO_NAME,             IPOD_UTIL_CFG_STR_VALUE,   1,                             {0} },
    { (U8 *)IPOD_PLAYER_CFG_TOKEN_ACCINFO_FWVER,            IPOD_UTIL_CFG_STR_VALUE,   IPOD_PLAYER_FHWVER_MAX_NUM,    {0} },
    { (U8 *)IPOD_PLAYER_CFG_TOKEN_ACCINFO_HWVER,            IPOD_UTIL_CFG_STR_VALUE,   IPOD_PLAYER_FHWVER_MAX_NUM,    {0} },
    { (U8 *)IPOD_PLAYER_CFG_TOKEN_ACCINFO_MANUFACTURE,      IPOD_UTIL_CFG_STR_VALUE,   1,                             {0} },
    { (U8 *)IPOD_PLAYER_CFG_TOKEN_ACCINFO_MODEL_NUM,        IPOD_UTIL_CFG_STR_VALUE,   1,                             {0} },
    { (U8 *)IPOD_PLAYER_CFG_TOKEN_ACCINFO_SERIAL_NUM,       IPOD_UTIL_CFG_STR_VALUE,   1,                             {0} },
    { (U8 *)IPOD_PLAYER_CFG_DETECTION_SERVER_NAME,          IPOD_UTIL_CFG_STR_VALUE,   1,                             {0} },
    { (U8 *)IPOD_PLAYER_CFG_AUTH_SERVER_NAME,               IPOD_UTIL_CFG_STR_VALUE,   1,                             {0} },
    { (U8 *)IPOD_PLAYER_CFG_DATA_COM_SERVER_NAME,           IPOD_UTIL_CFG_STR_VALUE,   1,                             {0} },
    { (U8 *)IPOD_PLAYER_CFG_SYSTEM_SUPPORT_MASK,            IPOD_UTIL_CFG_INT_VALUE,   1,                             {0} },
    { (U8 *)IPOD_PLAYER_CFG_IOS_APP_ATTR_MASK,              IPOD_UTIL_CFG_INT_VALUE,   1,                             {0} },
    { (U8 *)IPOD_PLAYER_CFG_RF_CERTIFICATIONS,              IPOD_UTIL_CFG_INT_VALUE,   1,                             {0} },
    { (U8 *)IPOD_PLAYER_CFG_DEFAULT_VIDEO_SETTING_MASK,     IPOD_UTIL_CFG_INT_VALUE,   1,                             {0} },
    { (U8 *)IPOD_PLAYER_CFG_GPS_ASSIST_SUPPORT,             IPOD_UTIL_CFG_INT_VALUE,   1,                             {0} },
    { (U8 *)IPOD_PLAYER_CFG_GPS_SATELITE_MAX_REFLESH,       IPOD_UTIL_CFG_INT_VALUE,   1,                             {0} },
    { (U8 *)IPOD_PLAYER_CFG_GPS_SATELITE_RECOMMNED_REFLESH, IPOD_UTIL_CFG_INT_VALUE,   1,                             {0} },
    { (U8 *)IPOD_PLAYER_CFG_CORE_SERVER_NAME,               IPOD_UTIL_CFG_INT_VALUE,   1,                             {0} },
    { (U8 *)IPOD_PLAYER_CFG_TAG_MAJOR,                      IPOD_UTIL_CFG_STR_VALUE,   1,                             {0} },
    { (U8 *)IPOD_PLAYER_CFG_TAG_MINOR,                      IPOD_UTIL_CFG_STR_VALUE,   1,                             {0} },
    { (U8 *)IPOD_PLAYER_CFG_TAG_MANID,                      IPOD_UTIL_CFG_STR_VALUE,   1,                             {0} },
    { (U8 *)IPOD_PLAYER_CFG_TAG_MANNAME,                    IPOD_UTIL_CFG_STR_VALUE,   1,                             {0} },
    { (U8 *)IPOD_PLAYER_CFG_TAG_DEVNAME,                    IPOD_UTIL_CFG_STR_VALUE,   1,                             {0} },
    { (U8 *)IPOD_PLAYER_CFG_TAG_MARKED,                     IPOD_UTIL_CFG_STR_VALUE,   1,                             {0} },
    { (U8 *)IPOD_PLAYER_CFG_IOSAPP_NAME,                    IPOD_UTIL_CFG_STR_VALUE,   IPODCORE_MAX_IOSAPPS_INFO_NUM, {0} },
    { (U8 *)IPOD_PLAYER_CFG_IOSAPP_URL,                     IPOD_UTIL_CFG_STR_VALUE,   IPODCORE_MAX_IOSAPPS_INFO_NUM, {0} },
    { (U8 *)IPOD_PLAYER_CFG_IOSAPP_COUNT,                   IPOD_UTIL_CFG_INT_VALUE,   1,                             {0} },
    { (U8 *)IPOD_PLAYER_CFG_LOCATION_INFO_NMEA,             IPOD_UTIL_CFG_STR_VALUE,   IPODCORE_LOCATION_NMEA_MAX,    {0} },
    { (U8 *)IPOD_PLAYER_CFG_LOCATION_INFO_COUNT,            IPOD_UTIL_CFG_INT_VALUE,   1,                             {0} },
    { (U8 *)IPOD_PLAYER_CFG_VINFO_ENGINE_TYPE,              IPOD_UTIL_CFG_STR_VALUE,   IPODCORE_VINFO_ENGINE_MAX,     {0} },
    { (U8 *)IPOD_PLAYER_CFG_VINFO_ENGINE_COUNT,             IPOD_UTIL_CFG_INT_VALUE,   1,                             {0} },
    { (U8 *)IPOD_PLAYER_CFG_VINFO_DISPLAY_NAME,             IPOD_UTIL_CFG_STR_VALUE,   1,                             {0} },
    { (U8 *)IPOD_PLAYER_CFG_VSTATUS_TYPE,                   IPOD_UTIL_CFG_STR_VALUE,   IPODCORE_VSTATUS_TYPE_MAX,     {0} },
    { (U8 *)IPOD_PLAYER_CFG_VSTATUS_TYPE_COUNT,             IPOD_UTIL_CFG_INT_VALUE,   1,                             {0} },
    { (U8 *)IPOD_PLAYER_CFG_MAX_DATABASE_NUM,               IPOD_UTIL_CFG_INT_VALUE,   1,                             {0} },
    { (U8 *)IPOD_PLAYER_CFG_DATABASE_LOCATION_PREFIX,       IPOD_UTIL_CFG_STR_VALUE,   1,                             {0} },
    { (U8 *)IPOD_PLAYER_CFG_ACC_CONFG_AVAILABLE_CURRENT,    IPOD_UTIL_CFG_INT_VALUE,   1,                             {0} },
    { (U8 *)IPOD_PLAYER_CFG_ACC_CONFG_BATTERY_SHOULD_CHARGE,IPOD_UTIL_CFG_INT_VALUE,   1,                             {0} },
    { (U8 *)IPOD_PLAYER_CFG_ACC_CONFG_MAXIMUM_CURRENT_DRWAN_FROM_ACCESSORY, IPOD_UTIL_CFG_INT_VALUE,   1,             {0} },
    { (U8 *)IPOD_PLAYER_CFG_ACC_CONFIG_BATTERY_WILL_CHARGE, IPOD_UTIL_CFG_INT_VALUE,   1,                             {0} },
    { (U8 *)IPOD_PLAYER_CFG_ACC_CONFIG_POWER_MODE,          IPOD_UTIL_CFG_INT_VALUE,   1,                             {0} },
    { (U8 *)IPOD_PLAYER_CFG_ACC_CONFIG_FILE_XFER_STREAM,    IPOD_UTIL_CFG_INT_VALUE,   1,                             {0} },
    { (U8 *)IPOD_PLAYER_CFG_ACC_CONFIG_EAP_SUPPORT,         IPOD_UTIL_CFG_INT_VALUE,   1,                             {0} },
    { (U8 *)IPOD_PLAYER_CFG_ACC_FILE_XFER_SUPPORT,          IPOD_UTIL_CFG_INT_VALUE,   1,                             {0} },
    { (U8 *)IPOD_PLAYER_CFG_ACC_CONFG_TRANS_USB_OTG_GPIO_POWER_AI,     IPOD_UTIL_CFG_STR_VALUE,   1,                  {0} },
    { (U8 *)IPOD_PLAYER_CFG_ACC_CONFG_TRANS_USB_OTG_GPIO_POWER_SD,     IPOD_UTIL_CFG_STR_VALUE,   1,                  {0} },
    { (U8 *)IPOD_PLAYER_CFG_ACC_INFO_MAXIMUM_CURRENT_DRAWN_FROM_DEVICE, IPOD_UTIL_CFG_STR_VALUE,   1,                 {0} },
    { (U8 *)IPOD_PLAYER_CFG_ACC_INFO_PREFFERD_APP_BUNDLE,   IPOD_UTIL_CFG_STR_VALUE,   1,                             {0} },
    { (U8 *)IPOD_PLAYER_CFG_ACC_INFO_CURRENT_LANGUAGE,      IPOD_UTIL_CFG_STR_VALUE,   1,                             {0} },
    { (U8 *)IPOD_PLAYER_CFG_ACC_INFO_SUPPORTED_LANGUAGE,    IPOD_UTIL_CFG_STR_VALUE,   IPODCORE_MAX_LANG_NUM,         {0} },
    { (U8 *)IPOD_PLAYER_CFG_ACC_INFO_LANGUAGE_COUNT,        IPOD_UTIL_CFG_INT_VALUE,   1,                             {0} },
    { (U8 *)IPOD_PLAYER_CFG_ACC_INFO_SUPPORT_SAMPLE_RATE,   IPOD_UTIL_CFG_INT_VALUE,   IPODCORE_MAX_SAMPLE_NUM,       {0} },
    { (U8 *)IPOD_PLAYER_CFG_ACC_INFO_SAMPLE_RATE_COUNT,     IPOD_UTIL_CFG_INT_VALUE,   1,                             {0} },
    { (U8 *)IPOD_PLAYER_CFG_ACC_INFO_BT_MAC,                IPOD_UTIL_CFG_STR_VALUE,   IPODCORE_BT_MAC_NUM,           {0} },
    { (U8 *)IPOD_PLAYER_CFG_ACC_INFO_BT_MAC_COUNT,          IPOD_UTIL_CFG_INT_VALUE,   1,                             {0} },
    { (U8 *)IPOD_PLAYER_CFG_ACC_INFO_VENDOR_ID,             IPOD_UTIL_CFG_STR_VALUE,   1,                             {0} },
    { (U8 *)IPOD_PLAYER_CFG_ACC_INFO_PRODUCT_ID,            IPOD_UTIL_CFG_STR_VALUE,   1,                             {0} },
    { (U8 *)IPOD_PLAYER_CFG_ACC_INFO_BCD_DEVICE,            IPOD_UTIL_CFG_STR_VALUE,   1,                             {0} },
    { (U8 *)IPOD_PLAYER_CFG_ACC_INFO_PRODUCT_PLAN_UUID,     IPOD_UTIL_CFG_STR_VALUE,   1,                             {0} },
    { (U8 *)IPOD_PLAYER_CFG_POWER_SUPPLY_CURRENT,           IPOD_UTIL_CFG_INT_VALUE,   1,                             {0} },
    { (U8 *)IPOD_PLAYER_CFG_POWER_SUPPLY_CHARGE_BUTTERY,    IPOD_UTIL_CFG_INT_VALUE,   1,                             {0} },
    { (U8 *)IPOD_PLAYER_CFG_ROUTE_GUID_COMP_ID,             IPOD_UTIL_CFG_INT_VALUE,   1,                             {0} },
    { (U8 *)IPOD_PLAYER_CFG_ROUTE_GUID_COMP_ID_COUNT,       IPOD_UTIL_CFG_INT_VALUE,   1,                             {0} },
    { (U8 *)IPOD_PLAYER_CFG_ROUTE_GUID_COMP_NAME,           IPOD_UTIL_CFG_STR_VALUE,   1,                             {0} },
    { (U8 *)IPOD_PLAYER_CFG_ROUTE_GUID_COMP_NAME_COUNT,     IPOD_UTIL_CFG_INT_VALUE,   1,                             {0} },
    { (U8 *)IPOD_PLAYER_CFG_ROUTE_GUID_COMP_MAXCURRENTROADNAMELEN,      IPOD_UTIL_CFG_INT_VALUE,   1,                 {0} },
    { (U8 *)IPOD_PLAYER_CFG_ROUTE_GUID_COMP_MAXCURRENTROADNAMELEN_COUNT,IPOD_UTIL_CFG_INT_VALUE,   1,                 {0} },
    { (U8 *)IPOD_PLAYER_CFG_ROUTE_GUID_COMP_MAXDESTNAMELEN, IPOD_UTIL_CFG_INT_VALUE,   1,                             {0} },
    { (U8 *)IPOD_PLAYER_CFG_ROUTE_GUID_COMP_MAXDESTNAMELEN_COUNT,       IPOD_UTIL_CFG_INT_VALUE,   1,                 {0} },
    { (U8 *)IPOD_PLAYER_CFG_ROUTE_GUID_COMP_MAXAFTERMANEROADNAMELEN,    IPOD_UTIL_CFG_INT_VALUE,   1,                 {0} },
    { (U8 *)IPOD_PLAYER_CFG_ROUTE_GUID_COMP_MAXAFTERMANEROADNAMELEN_COUNT, IPOD_UTIL_CFG_INT_VALUE,1,                 {0} },
    { (U8 *)IPOD_PLAYER_CFG_ROUTE_GUID_COMP_MAXMANEDESCKEN, IPOD_UTIL_CFG_INT_VALUE,   1,                             {0} },
    { (U8 *)IPOD_PLAYER_CFG_ROUTE_GUID_COMP_MAXMANEDESCKEN_COUNT,       IPOD_UTIL_CFG_INT_VALUE,   1,                 {0} },
    { (U8 *)IPOD_PLAYER_CFG_ROUTE_GUID_COMP_MAXGUIDMANECAP, IPOD_UTIL_CFG_INT_VALUE,   1,                             {0} },
    { (U8 *)IPOD_PLAYER_CFG_ROUTE_GUID_COMP_MAXGUIDMANECAP_COUNT,       IPOD_UTIL_CFG_INT_VALUE,   1,                 {0} },
    { (U8 *)IPOD_PLAYER_CFG_ROUTE_GUID_COMP_MAXLANEDESCRIPTLEN,         IPOD_UTIL_CFG_INT_VALUE,   1,                 {0} },
    { (U8 *)IPOD_PLAYER_CFG_ROUTE_GUID_COMP_MAXLANEDESCRIPTLEN_COUNT,   IPOD_UTIL_CFG_INT_VALUE,   1,                 {0} },
    { (U8 *)IPOD_PLAYER_CFG_ROUTE_GUID_COMP_MAXLANESTORAGECAP,          IPOD_UTIL_CFG_INT_VALUE,   1,                 {0} },
    { (U8 *)IPOD_PLAYER_CFG_ROUTE_GUID_COMP_MAXLANESTORAGECAP_COUNT,    IPOD_UTIL_CFG_INT_VALUE,   1,                 {0} },
    { (U8 *)IPOD_PLAYER_CFG_ROUTE_GUID_COMP_COUNT,          IPOD_UTIL_CFG_INT_VALUE,   1,                             {0} },
};


static S32 g_iPodPlayerCoreCfgInitFlg = 0;


static void iPodCoreFreeCfgValue(IPOD_UTIL_CFG_VALUE_TYPE type, U32 count, IPOD_PLAYER_CORE_CFG_VALUE *value);
static S32 iPodCoreAllocCfgValue(IPOD_UTIL_CFG_VALUE_TYPE type, U32 count, IPOD_PLAYER_CORE_CFG_VALUE *value);

/* free memory of cfg values */
static void iPodCoreFreeCfgValue(IPOD_UTIL_CFG_VALUE_TYPE type, U32 count, IPOD_PLAYER_CORE_CFG_VALUE *value)
{
    U32 i = 0;                                          /* for loop       */
    
    
    /* Check the Parameter */
    if(value == NULL)
    {
        /* invalid parameter */
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, value);
        return;
    }
    
     /* check type of value */
     switch(type)
     {
        /* value of integer */
         case IPOD_UTIL_CFG_INT_VALUE:
            if(value->intValue != NULL)
            {
                /* free memory of integer value */
                free(value->intValue);
                value->intValue = NULL;
            }
            break;
         /* value of string */
         case IPOD_UTIL_CFG_STR_VALUE:
            for(i = 0; i < count; i++)
            {
                if(value->strValue[i].strVal != NULL)
                {
                    /* free memory of string value */
                    free(value->strValue[i].strVal);
                    value->strValue[i].strVal = NULL;
                }
            }
            /* free address array */
            free(value->strValue);
            value->strValue = NULL;
            break;
        default:
            break;
    }
    
    return;
}


/* get memory for cfg values */
static S32 iPodCoreAllocCfgValue(IPOD_UTIL_CFG_VALUE_TYPE type, U32 count, IPOD_PLAYER_CORE_CFG_VALUE *value)
{
    U32 i = 0;                                          /* for loop       */
    S32 rc = IPOD_PLAYER_ERROR;                         /* for resault    */
    
    
    /* Check the Parameter */
    if(value == NULL)
    {
        /* invalid parameter */
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, value);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
     /* check type of value */
     switch(type)
     {
        /* value of integer */
         case IPOD_UTIL_CFG_INT_VALUE:
            /* get memory for integer value */
            value->intValue = (S32 *)calloc(count, sizeof(S32));
            if(value->intValue != NULL)
            {
                rc = IPOD_PLAYER_OK;
            }
            else
            {
                /* failed to get memory */
                rc = IPOD_PLAYER_ERR_NOMEM;
                IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc);
            }
            break;
        /* value of string */
         case IPOD_UTIL_CFG_STR_VALUE:
            /* get memory for string value */
            value->strValue = (IPOD_UTIL_CFG_STR *)calloc(count, sizeof(IPOD_UTIL_CFG_STR));
            if(value->strValue != NULL)
            {
                rc = IPOD_PLAYER_OK;
                /* get memory for members of string array */
                for(i = 0; i < count; i++)
                {
                    value->strValue[i].strVal = calloc(IPOD_PLAYER_CFG_STR_MAX_SIZE, sizeof(U8));
                    if(value->strValue[i].strVal != NULL)
                    {
                        /* set init size */
                        value->strValue[i].size = IPOD_PLAYER_CFG_STR_MAX_SIZE;
                    }
                    else
                    {
                        /* free handled memory */
                        iPodCoreFreeCfgValue(type, i, value);
                        rc = IPOD_PLAYER_ERR_NOMEM;
                        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc, i);
                        break;
                    }
                }
            }
            else
            {
                /* failed to get memory for string value */
                rc = IPOD_PLAYER_ERR_NOMEM;
                IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc, i);
            }
            break;
        default:
            rc = IPOD_PLAYER_ERR_INVALID_PARAMETER;
            break;
    }
    
    return rc;
}


/* Finalize the configuration */
void iPodCoreDeInitCfg(void)
{
    U32 i = 0;                                          /* for loop       */
    
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_CORE);
    
    /* Check the Parameter */
    if(g_iPodPlayerCoreCfgInitFlg == 0)
    {
        /* invalid parameter */
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, g_iPodPlayerCoreCfgInitFlg);
        return;
    }
    
    /* free all of cfg value */
    for(i = 0; i < IPOD_PLAYER_CFGNUM_MAX; i++)
    {
        iPodCoreFreeCfgValue(g_iPodPlayerCfg[i].type, g_iPodPlayerCfg[i].count, &g_iPodPlayerCfg[i].value);
    }
    
    g_iPodPlayerCoreCfgInitFlg = 0;
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_CORE);
    
    return;
}

/* Initialize the configuration */
S32 iPodCoreInitCfg(void)
{
    U32 i = 0;                                          /* for loop       */
    U32 j = 0;                                          /* for loop       */
    S32 rc = IPOD_PLAYER_ERROR;                         /* for resault    */
    IPOD_UTIL_CFG_HANDLE handle = NULL;                 /* for cfg handle */
    
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_CORE);
    
    /* check parameter */
    if(g_iPodPlayerCoreCfgInitFlg == 1)
    {
        /* already initailzed */
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc, g_iPodPlayerCoreCfgInitFlg);
        return rc;
    }
    
    /* initialize cfg function */
    handle = iPodUtilInitCf((U8 *)IPOD_PLAYER_CFG_FILE_NAME, (U8 *)IPOD_PLAYER_CFG_ROOT_TAG);
    if(handle != NULL)
    {
        /* get all of cfg value */
        for(i = 0; i < IPOD_PLAYER_CFGNUM_MAX; i++)
        {
            /* get memory for cfg value */
            rc = iPodCoreAllocCfgValue(g_iPodPlayerCfg[i].type, g_iPodPlayerCfg[i].count, &g_iPodPlayerCfg[i].value);
            if(rc != IPOD_PLAYER_OK)
            {
                break;
            }
            /* get one record */
            switch(g_iPodPlayerCfg[i].type)
            {
                /* value of integer */
                case IPOD_UTIL_CFG_INT_VALUE:
                    /* get number from cfg file */
                    rc = iPodUtilGetNumCfn(handle, g_iPodPlayerCfg[i].key, g_iPodPlayerCfg[i].count, g_iPodPlayerCfg[i].value.intValue);
                    break;
                /* value of string */
                case IPOD_UTIL_CFG_STR_VALUE:
                    /* get string from cfg file */
                    rc = iPodUtilGetNumCfs(handle, g_iPodPlayerCfg[i].key, g_iPodPlayerCfg[i].count, g_iPodPlayerCfg[i].value.strValue);
                    break;
                default:
                    rc = IPOD_PLAYER_ERROR;
                    break;
            }
            /* succeed to get value */
            if(rc > 0)
            {
                rc = IPOD_PLAYER_OK;
            }
            else
            {
                /* failed to get value */
                IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc, g_iPodPlayerCfg[i].type, i);
                break;
            }
        }
        /* failed to init cfg */
        if(rc != IPOD_PLAYER_OK)
        {
            /* free all of handled memory */
            for(j = 0; j < i; j++)
            {
                /* free cfg value */
                iPodCoreFreeCfgValue(g_iPodPlayerCfg[j].type, g_iPodPlayerCfg[j].count, &g_iPodPlayerCfg[j].value);
            }
        }
        else
        {
            g_iPodPlayerCoreCfgInitFlg = 1;
        }
        
        /* finalize cfg function */
        iPodUtilDeInitCf(handle);
        
    }
    else
    {
        /* failed to initialize cfg function */
        rc = IPOD_PLAYER_ERROR;
        IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_CORE, rc);
    }
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}

/* get cfg number: this is used for the cfg with one number vaule */
S32 iPodCoreGetCfn(U32 cfgId)
{
    S32 rc = 0;                                             /* for resault    */
    
    /* check parameter */
    if(g_iPodPlayerCoreCfgInitFlg == 0)
    {
        /* already initalized */
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERROR, cfgId);
        return rc;
    }
    
    /* check number of value */
    if(g_iPodPlayerCfg[cfgId].count > 0)
    {
        /* set value to return parameter */
        rc = g_iPodPlayerCfg[cfgId].value.intValue[0];
    }
    
    return rc;
}

/* get cfg number */
S32 iPodCoreGetIndexCfn(U32 cfgId, U32 valIndex)
{
    S32 rc = 0;                                             /* for resault    */
    
    /* check parameter */
    if((g_iPodPlayerCoreCfgInitFlg == 0) || (cfgId >= IPOD_PLAYER_CFGNUM_MAX))
    {
        /* already initalized */
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, cfgId, g_iPodPlayerCoreCfgInitFlg);
        return rc;
    }
    
    /* check number of value */
    if(valIndex < g_iPodPlayerCfg[cfgId].count)
    {
        /* set value to return parameter */
        rc = g_iPodPlayerCfg[cfgId].value.intValue[valIndex];
    }
    
    return rc;
}

/* get cfg number */
S32 iPodCoreGetNumCfn(U32 cfgId, U32 count, S32 *value)
{
    U32 i = 0;                                              /* for loop       */
    S32 rc = IPOD_PLAYER_ERROR;                             /* for resault    */
    
    /* check parameter */
    if((g_iPodPlayerCoreCfgInitFlg == 0) || (value == NULL) || (cfgId >= IPOD_PLAYER_CFGNUM_MAX))
    {
        /* already initalized */
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, cfgId, value, g_iPodPlayerCoreCfgInitFlg);
        return rc;
    }
    
    /* check number of value */
    if(count <= g_iPodPlayerCfg[cfgId].count)
    {
        /* loop for all of value */
        for(i = 0; i < count; i++)
        {
            /* set value to return parameter */
            value[i] = g_iPodPlayerCfg[cfgId].value.intValue[i];
        }
        rc = IPOD_PLAYER_OK;
    }
    else
    {
        /* bad parameter */
        rc = IPOD_PLAYER_ERROR;
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc, count, g_iPodPlayerCfg[cfgId].count);
    }
    
    return rc;
}

/* get cfg string value: this is used for cfg with one string value */
S32 iPodCoreGetCfs(U32 cfgId, U32 size, U8 *value)
{
    S32 rc = IPOD_PLAYER_ERROR;                             /* for resault    */
    
    /* check parameter */
    if((g_iPodPlayerCoreCfgInitFlg == 0) || (value == NULL) || (cfgId >= IPOD_PLAYER_CFGNUM_MAX))
    {
        /* already initalized */
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, cfgId, value, g_iPodPlayerCoreCfgInitFlg);
        return rc;
    }
    
    /* check size of value */
    if((g_iPodPlayerCfg[cfgId].count > 0) && (size >= g_iPodPlayerCfg[cfgId].value.strValue[0].size))
    {
        if(g_iPodPlayerCfg[cfgId].value.strValue[0].size > 0)
        {
            /* copy string value */
            strncpy((char *)value, (const char *)g_iPodPlayerCfg[cfgId].value.strValue[0].strVal, g_iPodPlayerCfg[cfgId].value.strValue[0].size);
        }
        else
        {
            /* add '\0' to string end */
            value[0] = '\0';
        }
        rc = IPOD_PLAYER_OK;
    }
    else
    {
        /* bad parameter */
        rc = IPOD_PLAYER_ERROR;
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc, g_iPodPlayerCfg[cfgId].count, size);
    }
    
    return rc;
}

/* get cfg string value */
S32 iPodCoreGetIndexCfs(U32 cfgId, U32 valIndex, U32 size, U8 *value)
{
    S32 rc = IPOD_PLAYER_ERROR;                             /* for resault    */
    
    /* check parameter */
    if((g_iPodPlayerCoreCfgInitFlg == 0) || (value == NULL) || (cfgId >= IPOD_PLAYER_CFGNUM_MAX))
    {
        /* already initalized */
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, cfgId, value, g_iPodPlayerCoreCfgInitFlg);
        return rc;
    }
    
    /* check size of value */
    if((valIndex < g_iPodPlayerCfg[cfgId].count) && (size >= g_iPodPlayerCfg[cfgId].value.strValue[valIndex].size))
    {
        if(g_iPodPlayerCfg[cfgId].value.strValue[valIndex].size > 0)
        {
            /* copy string value */
            strncpy((char *)value, (const char *)g_iPodPlayerCfg[cfgId].value.strValue[valIndex].strVal, g_iPodPlayerCfg[cfgId].value.strValue[valIndex].size);
        }
        else
        {
            /* add '\0' to string end */
            value[0] = '\0';
        }
        rc = IPOD_PLAYER_OK;
    }
    else
    {
        /* bad parameter */
        rc = IPOD_PLAYER_ERROR;
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc, valIndex, g_iPodPlayerCfg[cfgId].count, size);
    }
    
    return rc;
}

/* get cfg string value */
S32 iPodCoreGetNumCfs(U32 cfgId, U32 count, IPOD_UTIL_CFG_STR *value)
{
    U32 i = 0;                                              /* for loop       */
    S32 rc = IPOD_PLAYER_ERROR;                             /* for resault    */
    
    /* check parameter */
    if((g_iPodPlayerCoreCfgInitFlg == 0) || (value == NULL) || (cfgId >= IPOD_PLAYER_CFGNUM_MAX))
    {
        /* already initalized */
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, cfgId, value, g_iPodPlayerCoreCfgInitFlg);
        return rc;
    }
    
    /* check number of value */
    if(count <= g_iPodPlayerCfg[cfgId].count)
    {
        rc = IPOD_PLAYER_OK;
        /* loop for all of value */
        for(i = 0; i < count; i++)
        {
            if(value->size >= g_iPodPlayerCfg[cfgId].value.strValue[i].size)
            {
                /* set size of string */
                value->size = g_iPodPlayerCfg[cfgId].value.strValue[i].size;
            }
            else
            {
                /* bad parameter */
                rc = IPOD_PLAYER_ERROR;
                break;
            }
            if(g_iPodPlayerCfg[cfgId].value.strValue[i].size > 0)
            {
                /* copy string value */
                strncpy((char *)value->strVal, (const char *)g_iPodPlayerCfg[cfgId].value.strValue[i].strVal, value->size);
            }
            else
            {
                /* add '\0' to string end */
                value->strVal[0] = '\0';
            }
        }
    }
    else
    {
        /* bad parameter */
        rc = IPOD_PLAYER_ERROR;
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc, count, g_iPodPlayerCfg[cfgId].count);
    }
    
    return rc;
}


const IPOD_PLAYER_CORE_CFG_INFO *iPodCoreGetCfgs(void)
{
    return g_iPodPlayerCfg;
}
