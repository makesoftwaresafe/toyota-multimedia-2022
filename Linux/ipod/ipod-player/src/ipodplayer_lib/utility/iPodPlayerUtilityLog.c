#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <dirent.h>
#include <adit_typedef.h>
#include <errno.h>
#include <stdlib.h>
#include <fcntl.h>
#include <math.h>
#include <inttypes.h>

#include "pthread_adit.h"
#include "iPodPlayerUtilityLog.h"

DLT_DECLARE_CONTEXT(g_iPodLogContext)               /* DLT context ID           */
DltContextData g_iPodLogContextData;                /* DLT context data         */

/* iPod logging initialize */
void iPodLogInitialize(IPOD_LOG_INT_PARAM *initParam)
{
    /* paramer check */
    if(initParam == NULL)
    {
        return;
    }
    
    /* initialize dlt function */
    DLT_REGISTER_APP(DLT_IPOD_PLAYER_APID, IPOD_LOG_DLT_APP_DSP);
    DLT_REGISTER_CONTEXT(g_iPodLogContext, (char *)initParam->dltCtx, (char *)initParam->dltCtxDsp);
    
    return;
}

/* iPod logging Deinitialize */
void iPodLogDeinitialize()
{
    /* dlt mode */
    DLT_UNREGISTER_CONTEXT(g_iPodLogContext);
    DLT_UNREGISTER_APP();
    
    return;
}

/* get system time */
U32 iPodGetTime(void)
{
    struct timespec real_time;                  /* for get tiem      */
    S32    rc = 0;                              /* for resault       */
    U32    ipod_time = 0;                       /* for return tiem   */
    
    /* Initialize the structure */
    memset(&real_time, 0, sizeof(real_time));
    /* get monotonic time */
    rc = clock_gettime(CLOCK_MONOTONIC, &real_time);
    if(rc == 0)
    {
        /* calculate time for ms */
        ipod_time = ((real_time.tv_sec % IPOD_LOG_SEC_MAX) * IPOD_LOG_MSEC)  + (real_time.tv_nsec / IPOD_LOG_NSEC_TO_MSEC);
    }
    
    return ipod_time;
}

DltLogLevelType iPodLogGetType(IPOD_LOG_TYPE type, char typeName[])
{
    unsigned int    i = 0;
    DltLogLevelType loglevel = DLT_LOG_DEFAULT;
    typedef struct logTypeTable_type
    {
        IPOD_LOG_TYPE   type;
        char            name[IPOD_LOG_TYPE_LEN];
        DltLogLevelType level;
    } log_type_table;
    log_type_table logTypeTbl[] = 
    {
        {   IPOD_LOG_TYPE_SEQUENCE,   "Seq  ", DLT_LOG_DEBUG},     /* sequence           */
        {   IPOD_LOG_TYPE_FUNCSTART,  "FSta ", DLT_LOG_DEBUG},     /* function start     */
        {   IPOD_LOG_TYPE_FUNCRETURN, "FRet ", DLT_LOG_DEBUG},     /* function end       */
        {   IPOD_LOG_TYPE_FUNCREARGS, "Args ", DLT_LOG_VERBOSE},   /* function argument  */
        {   IPOD_LOG_TYPE_FUNCDETAIL, "Det  ", DLT_LOG_VERBOSE},   /* detail log trace   */
        {   IPOD_LOG_TYPE_ERROR,      "Err  ", DLT_LOG_ERROR},     /* error log trace    */
        {   IPOD_LOG_TYPE_INFORMATION,"Info ", DLT_LOG_INFO},      /* Information        */
        {   IPOD_LOG_TYPE_UNKNOWN,    "No   ", DLT_LOG_DEFAULT}    /* no type            */
    };

    for(i = 0; IPOD_LOG_TYPE_UNKNOWN >= i; i++)
    {
        if((type == logTypeTbl[i].type) || (i == IPOD_LOG_TYPE_UNKNOWN))
        {
            if(typeName != NULL)
            {
                strcpy(typeName, logTypeTbl[i].name);
            }

            loglevel = logTypeTbl[i].level;

            break;
        }
    }

    return loglevel;
}

void iPodLogAddString(char *buff, IPOD_PARA_DATA_TYPE *data, int size, IPOD_LOG_PARAM_TYPE type)
{
    int     i = 0;
    char    *p = buff;
    
    if((buff == NULL) || (data == NULL))
    {
        return;
    }

    if((type == IPOD_LOG_PARAM_STRBIN) || (type == IPOD_LOG_PARAM_STR) || (type == IPOD_LOG_PARAM_STR16) ||
        type == IPOD_LOG_PARAM_STR32 || (type == IPOD_LOG_PARAM_STR64))
    {
        sprintf(p, " %s", data->strptr);
        p = buff + strlen(buff);   
    }

    for(i = 0; (i < size) && (i < IPOD_PARA_STRING_MAX); i++)
    {
        switch(type)
        {
            case IPOD_LOG_PARAM_BIN:
            case IPOD_LOG_PARAM_STRBIN:
                sprintf(p, " 0x%02" PRIx8, (unsigned int)data->binptr[i]);
                break;
            case IPOD_LOG_PARAM_16:
            case IPOD_LOG_PARAM_STR16:
                sprintf(p, " 0x%04" PRIx16, data->para[i].u16);
                break;
            case IPOD_LOG_PARAM_32:
            case IPOD_LOG_PARAM_STR32:
                sprintf(p, " 0x%08" PRIx32, data->para[i].u32);
                break;
            case IPOD_LOG_PARAM_64:
            case IPOD_LOG_PARAM_STR64:
                sprintf(p, " 0x%016" PRIx64, data->para[i].u64);
                break;
            case IPOD_LOG_PARAM_PTR:
                sprintf(p, " %p", data->para[i].ptr);
                break;
            default:
                break;
        }
        p = buff + strlen(buff);   
    }
}

void iPodLogDltString(IPOD_LOG_TYPE type, IPOD_LOG_BLOCK blocID, pid_t tid, const U8 *fileName, const U8 *funcName, U32 lineNum, IPOD_LOG_PARAM_TYPE paraType, const U8 *str, U16 binSize, U8 *binData)
{
#ifdef IPOD_HAS_DLT
    char dltlog[DLT_USER_BUF_MAX_SIZE] = {0};
    char typeString[IPOD_LOG_TYPE_LEN];
    IPOD_PARA_DATA_TYPE data = {0};
    DltLogLevelType logLevel;
    
    blocID = blocID;

    /* check initialize */
    if((fileName == NULL) || (funcName == NULL))
    {
        return;
    }
    
    logLevel = iPodLogGetType(type, typeString);
    snprintf(dltlog, DLT_USER_BUF_MAX_SIZE, "%d %s %s %d", tid, typeString, funcName, lineNum);
        
    /* insert parameter */
    data.strptr = (char *)str;
    data.binptr = binData;
    iPodLogAddString(dltlog + strlen(dltlog), &data, binSize, paraType);

    DLT_LOG(g_iPodLogContext, logLevel, DLT_STRING(dltlog));
#else
    (void)type;
    (void)blocID;
    (void)fileName;
    (void)funcName;
    (void)lineNum;
    (void)paraType;
    (void)str;
    (void)binSize;
    (void)binData;

#endif // IPOD_HAS_DLT

    return;
}

void iPodLogDltParam(IPOD_LOG_TYPE type, IPOD_LOG_BLOCK blocID, pid_t tid, const U8 *fileName, const U8 *funcName, U32 lineNum, S32 errCode, IPOD_LOG_PARAM_TYPE paraType, const U8 *str, U32 paraNum, ...)
{
#ifdef IPOD_HAS_DLT
    va_list args;                               /* for dynamic parameter */
    char dltlog[DLT_USER_BUF_MAX_SIZE] = {0};
    U32 i;
    char typeString[IPOD_LOG_TYPE_LEN];
    DltLogLevelType logLevel;
    IPOD_PARA_DATA_TYPE data = {0};
    
    blocID = blocID;

    /* check initialize */
    if((fileName == NULL) || (funcName == NULL))
    {
        return;
    }
    
    logLevel = iPodLogGetType(type, typeString);
    if(logLevel != DLT_LOG_ERROR)
    {
        snprintf(dltlog, DLT_USER_BUF_MAX_SIZE, "%d %s %s(%d)", tid, typeString, funcName, lineNum);
    }
    else
    {
        snprintf(dltlog, DLT_USER_BUF_MAX_SIZE, "%d %s %s(%d) err=0x%02x", tid, typeString, funcName, lineNum, errCode);
    }
        
    /* insert parameter */
    data.strptr = (char *)str;

    /* initialize args */
    va_start(args, paraNum);

    /* setting of arg */
    for(i = 0; (i < paraNum) && (i < IPOD_PARA_STRING_MAX); i++)
    {
        switch(paraType)
        {
            /* parameter is U16 */
            case IPOD_LOG_PARAM_16:
            case IPOD_LOG_PARAM_STR16:
                data.para[i].u16 = (uint16_t)va_arg(args, uint32_t);
                break;

            /* parameter is U32 */
            case IPOD_LOG_PARAM_32:
            case IPOD_LOG_PARAM_STR32:
                data.para[i].u32 = (uint32_t)va_arg(args, uint32_t);
                break;

            /* parameter is U64 */
            case IPOD_LOG_PARAM_64:
            case IPOD_LOG_PARAM_STR64:
                data.para[i].u64 = (uint64_t)va_arg(args, uint64_t);
                break;

            default:
                break;
        }
    }
    /* finish args */
    va_end(args);

    iPodLogAddString(dltlog + strlen(dltlog), &data, paraNum, paraType);

    DLT_LOG(g_iPodLogContext, logLevel, DLT_STRING(dltlog));
#else
    (void)type;
    (void)blocID;
    (void)fileName;
    (void)funcName;
    (void)lineNum;
    (void)errCode;
    (void)paraType;
    (void)str;
    (void)paraNum;

#endif // IPOD_HAS_DLT

    return;
}

void iPPDltLog(DltLogLevelType type, char *dltlog)
{
    DLT_LOG(g_iPodLogContext, type, DLT_STRING(dltlog));
}
