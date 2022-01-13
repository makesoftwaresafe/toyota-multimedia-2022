#ifndef IPOD_PLAYER_IPC_LOG_H
#define IPOD_PLAYER_IPC_LOG_H

#include <unistd.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include "dlt/dlt.h"
#include "adit_typedef.h"
#include "adit_dlt.h"

DLT_IMPORT_CONTEXT(g_iPodLogContext)

#define IPOD_LOG_TYPE_LEN   16                                                          /* log type length             */
#define IPOD_LOG_SEC_MAX 1000000                                                        /* maximum second              */
#define IPOD_LOG_MSEC 1000                                                              /* micro second                */
#define IPOD_LOG_MAX_SEC (IPOD_LOG_SEC_MAX * IPOD_LOG_MSEC)                             /* maximum nano second         */
#define IPOD_LOG_NSEC_TO_MSEC 1000000                                                   /* nano to micro second        */
#define IPOD_LOG_DLT_APP_DSP           "iPod Application for Logging"                   /* dlt application description */
#define IPOD_LOG_DLT_CTXT_LEN          4                                                /* dlt context length          */
#define IPOD_LOG_DLT_CTXT_DSP_LEN      50                                               /* dlt context description len */
#define IPOD_LOG_DLT_CMD_MESG_LEN      50                                               /* dlt injection message len   */

#define IPOD_PLAYER_DLT_CONTEXT                         "CORE"
#define IPOD_PLAYER_DLT_CONTEXT_DSP                     "iPod Core Context For Logging"

/*------------------ count dynamic parameters ----------------------------------------*/
#define COUNT_PARMS2(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _, ...) _
#define COUNT_PARMS(...)\
COUNT_PARMS2(__VA_ARGS__,20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0)

/* log block */
typedef enum
{
    IPOD_LOG_CTRL_AUTH       = 0x0001,          /* iPodCtrlAuthentication   */
    IPOD_LOG_CTRL_PRTCL      = 0x0002,          /* iPodCtrlProtocol         */
    IPOD_LOG_CTRL_TRANS      = 0x0004,          /* iPodCtrlTranport         */
    IPOD_LOG_PLAYER_IPCLIB   = 0x0008,          /* iPodPlayerIPCLibrary     */
    IPOD_LOG_PLAYER_CORE     = 0x0010,          /* iPodPlayerCore           */
    IPOD_LOG_PLAYER_IF       = 0x0020,          /* iPodPlayerInterface      */
    IPOD_LOG_PLUGIN_AUTH     = 0x0040,          /* iPodPluginAuthentication */
    IPOD_LOG_PLUGIN_DATA_HID = 0x0080,          /* iPodPluginDataHiddev     */
    IPOD_LOG_PLUGIN_DATA_USB = 0x0100,          /* iPodPluginDataUSB        */
    IPOD_LOG_PLAYER_AUDIO    = 0x0200,          /* iPodPluginAudio          */
    IPOD_LOG_PLAYER_DEVDETECT= 0x0400,          /* iPod device detection    */
    IPOD_LOG_PLAYER_SAPP     = 0x0800,          /* iPod sample app          */
    IPOD_LOG_BLOCK_ALL       = 0xFFFF           /* all of iPod blcok        */
} IPOD_LOG_BLOCK;

/* log param type */
typedef enum
{
    IPOD_LOG_PARAM_BIN       = 0x0001,              /* iPod logging param binary  */
    IPOD_LOG_PARAM_STR,                             /* iPod logging param string  */
    IPOD_LOG_PARAM_STRBIN,                          /* iPod logging param str bin */
    IPOD_LOG_PARAM_16,                              /* iPod logging param 16bit   */
    IPOD_LOG_PARAM_32,                              /* iPod logging param 32bit   */
    IPOD_LOG_PARAM_64,                              /* iPod logging param 64bit   */
    IPOD_LOG_PARAM_STR16,                           /* iPod logging param str 16  */
    IPOD_LOG_PARAM_STR32,                           /* iPod logging param str 16  */
    IPOD_LOG_PARAM_STR64,                           /* iPod logging param str 16  */
    IPOD_LOG_PARAM_PTR                              /* iPod logging param pointer */
}IPOD_LOG_PARAM_TYPE;

/* type of log */
typedef enum
{
    IPOD_LOG_TYPE_SEQUENCE = 0,     /* sequence           */
    IPOD_LOG_TYPE_FUNCSTART,        /* function start     */
    IPOD_LOG_TYPE_FUNCRETURN,       /* function end       */
    IPOD_LOG_TYPE_FUNCREARGS,       /* function end       */
    IPOD_LOG_TYPE_FUNCDETAIL,       /* detail log trace   */
    IPOD_LOG_TYPE_ERROR,            /* error log trace    */
    IPOD_LOG_TYPE_INFORMATION,      /* information        */
    IPOD_LOG_TYPE_UNKNOWN           /* no logging         */
} IPOD_LOG_TYPE;

#define GETTID (pid_t)syscall(SYS_gettid)
#define IPOD_LOGGING_VERBOSE_FORMAT "%d V %s(%d) "
#define IPOD_LOGGING_INFO_FORMAT "%d I %s(%d) "
#define IPOD_LOGGING_WARN_FORMAT "%d W %s(%d) "
#define IPOD_LOGGING_ERR_FORMAT  "%d E %s(%d) "
#define IPOD_LOGGING_FUNCTION_ARGS GETTID, __FUNCTION__, __LINE__

/* iPod log init parameter */
typedef struct
{
    U8  dltCtx[IPOD_LOG_DLT_CTXT_LEN];              /* dlt context    */
    U8  dltCtxDsp[IPOD_LOG_DLT_CTXT_DSP_LEN];       /* dlt context    */
} IPOD_LOG_INT_PARAM;

/*--------------------- iPod log return code  ------------------------------- */
#define IPOD_LOG_OK                      0          /* ipod log ok                */
#define IPOD_LOG_ERROR                  -1          /* ipod log error             */
#define IPOD_LOG_ERR_BADPARAM           -2          /* ipod log invalid parameter */

/*--------------------- iPod log dlt para type  ------------------------------- */
#define IPOD_PARA_STRING_MAX    8
typedef struct
{
    char    *strptr;
    uint8_t *binptr;
    union 
    {
        uint8_t     u8;
        int8_t      s8;
        uint16_t    u16;
        int16_t     s16;
        uint32_t    u32;
        int32_t     s32;
        uint64_t    u64;
        int64_t     s64;
        char        *str;
        void        *ptr;
    }para[IPOD_PARA_STRING_MAX];
} IPOD_PARA_DATA_TYPE;

/*------------------ logging function define ---------------------------------*/

/* !!! the maximum numbers of dynamic parameter is 20.please don't over it. !!! */
#define IPOD_LOG_INFO_WRITESTR(logType, blocID, str) do { \
        if(dlt_user_is_logLevel_enabled(&g_iPodLogContext, iPodLogGetType(logType, NULL)) == DLT_RETURN_TRUE) { \
            iPodLogDltString((IPOD_LOG_TYPE)logType, (IPOD_LOG_BLOCK)blocID, GETTID, (const U8*)__FILE__, (const U8*)__FUNCTION__,  (U32)__LINE__, IPOD_LOG_PARAM_STR, (const U8*)str, 0, NULL); \
        } \
    } while(0)

#define IPOD_LOG_INFO_WRITEBIN(logType, blocID, binSize, binData) do { \
        if(dlt_user_is_logLevel_enabled(&g_iPodLogContext, iPodLogGetType(logType, NULL)) == DLT_RETURN_TRUE) { \
            iPodLogDltString((IPOD_LOG_TYPE)logType, (IPOD_LOG_BLOCK)blocID, GETTID, (const U8*)__FILE__, (const U8*)__FUNCTION__,  (U32)__LINE__, IPOD_LOG_PARAM_BIN, NULL, (U32)binSize, (U8*)binData); \
        } \
    } while(0)

#define IPOD_LOG_INFO_WRITE32(logType, blocID, ...) do { \
        if(dlt_user_is_logLevel_enabled(&g_iPodLogContext, iPodLogGetType(logType, NULL)) == DLT_RETURN_TRUE) { \
            iPodLogDltParam((IPOD_LOG_TYPE)logType, (IPOD_LOG_BLOCK)blocID, GETTID, (const U8*)__FILE__, (const U8*)__FUNCTION__, (U32)__LINE__, 0, IPOD_LOG_PARAM_32, NULL, (U32)COUNT_PARMS(logType, ##__VA_ARGS__), ##__VA_ARGS__); \
        } \
    } while(0)

#define IPOD_LOG_INFO_WRITE64(logType, blocID, ...) do { \
        if(dlt_user_is_logLevel_enabled(&g_iPodLogContext, iPodLogGetType(logType, NULL)) == DLT_RETURN_TRUE) { \
            iPodLogDltParam((IPOD_LOG_TYPE)logType, (IPOD_LOG_BLOCK)blocID, GETTID, (const U8*)__FILE__, (const U8*)__FUNCTION__, (U32)__LINE__, 0, IPOD_LOG_PARAM_64, NULL, (U32)COUNT_PARMS(logType, ##__VA_ARGS__), ##__VA_ARGS__); \
        } \
    } while(0)

#define IPOD_LOG_INFO_WRITESTR32(logType, blocID, str, ...) do { \
        if(dlt_user_is_logLevel_enabled(&g_iPodLogContext, iPodLogGetType(logType, NULL)) == DLT_RETURN_TRUE) { \
            iPodLogDltParam((IPOD_LOG_TYPE)logType, (IPOD_LOG_BLOCK)blocID, GETTID, (const U8*)__FILE__, (const U8*)__FUNCTION__, (U32)__LINE__, 0, IPOD_LOG_PARAM_STR32, (const U8*)str, (U32)COUNT_PARMS(logType, ##__VA_ARGS__), ##__VA_ARGS__); \
        } \
    } while(0)


#define IPOD_LOG_ERR_WRITE32(logType, blocID, errNum, ...) do { \
        if(dlt_user_is_logLevel_enabled(&g_iPodLogContext, iPodLogGetType(logType, NULL)) == DLT_RETURN_TRUE) { \
            iPodLogDltParam((IPOD_LOG_TYPE)logType, (IPOD_LOG_BLOCK)blocID, GETTID, (const U8*)__FILE__, (const U8*)__FUNCTION__,  (U32)__LINE__, (S32)errNum, IPOD_LOG_PARAM_32, NULL, (U32)COUNT_PARMS(logType, ##__VA_ARGS__), ##__VA_ARGS__); \
        } \
    } while(0)

#define IPOD_ERR_WRITE_PTR(logType, blocID, errNum, ...) do { \
        if(dlt_user_is_logLevel_enabled(&g_iPodLogContext, iPodLogGetType(logType, NULL)) == DLT_RETURN_TRUE) { \
            iPodLogDltParam((IPOD_LOG_TYPE)logType, (IPOD_LOG_BLOCK)blocID, GETTID, (const U8*)__FILE__, (const U8*)__FUNCTION__,  (U32)__LINE__, (S32)errNum, IPOD_LOG_PARAM_PTR, NULL, (U32)COUNT_PARMS(logType, ##__VA_ARGS__), ##__VA_ARGS__); \
        } \
    } while(0)

#ifdef IPOD_HAS_DLT
#define IPOD_DLT_VERBOSE(log_string, args...) do {  \
        if(dlt_user_is_logLevel_enabled(&g_iPodLogContext, DLT_LOG_VERBOSE) == DLT_RETURN_TRUE) { \
            char dltlog[DLT_USER_BUF_MAX_SIZE]; \
            snprintf(dltlog, DLT_USER_BUF_MAX_SIZE, IPOD_LOGGING_VERBOSE_FORMAT log_string, IPOD_LOGGING_FUNCTION_ARGS, ## args); \
            iPPDltLog(DLT_LOG_VERBOSE, dltlog); \
        } \
    } while (0)
#define IPOD_DLT_INFO(log_string, args...) do {  \
        if(dlt_user_is_logLevel_enabled(&g_iPodLogContext, DLT_LOG_INFO) == DLT_RETURN_TRUE) { \
            char dltlog[DLT_USER_BUF_MAX_SIZE]; \
            snprintf(dltlog, DLT_USER_BUF_MAX_SIZE, IPOD_LOGGING_INFO_FORMAT log_string, IPOD_LOGGING_FUNCTION_ARGS, ## args); \
            iPPDltLog(DLT_LOG_INFO, dltlog); \
        } \
    } while (0)
#define IPOD_DLT_WARN(log_string, args...) do {  \
        if(dlt_user_is_logLevel_enabled(&g_iPodLogContext, DLT_LOG_WARN) == DLT_RETURN_TRUE) { \
            char dltlog[DLT_USER_BUF_MAX_SIZE]; \
            snprintf(dltlog, DLT_USER_BUF_MAX_SIZE, IPOD_LOGGING_WARN_FORMAT log_string, IPOD_LOGGING_FUNCTION_ARGS, ## args); \
            iPPDltLog(DLT_LOG_WARN, dltlog); \
        } \
    } while (0)
#define IPOD_DLT_ERROR(log_string, args...) do {  \
        if(dlt_user_is_logLevel_enabled(&g_iPodLogContext, DLT_LOG_ERROR) == DLT_RETURN_TRUE) { \
            char dltlog[DLT_USER_BUF_MAX_SIZE]; \
            snprintf(dltlog, DLT_USER_BUF_MAX_SIZE, IPOD_LOGGING_ERR_FORMAT log_string, IPOD_LOGGING_FUNCTION_ARGS, ## args); \
            iPPDltLog(DLT_LOG_ERROR, dltlog); \
        } \
    } while (0)

DltLogLevelType iPodLogGetType(IPOD_LOG_TYPE type, char typeName[]);
void            iPPDltLog(DltLogLevelType levelType, char *dltlog);

#else
#define IPOD_DLT_VERBOSE(log_string, args...)
#define IPOD_DLT_INFO(log_string, args...)
#define IPOD_DLT_WARN(log_string, args...)
#define IPOD_DLT_ERROR(log_string, args...)

#endif // IPOD_HAS_DLT

void iPodLogDltString(IPOD_LOG_TYPE logType, IPOD_LOG_BLOCK blocID, pid_t tid, const U8 *fileName, const U8 *funcName, U32 lineNum, IPOD_LOG_PARAM_TYPE paraType, const U8 *str, U16 binSize, U8 *binData);
void iPodLogDltParam(IPOD_LOG_TYPE type, IPOD_LOG_BLOCK blocID, pid_t tid, const U8 *fileName, const U8 *funcName, U32 lineNum, S32 errcode, IPOD_LOG_PARAM_TYPE paraType, const U8 *str, U32 paraNum, ...);
void iPodLogInitialize(IPOD_LOG_INT_PARAM *initParam);
void iPodLogDeinitialize();

U32 iPodGetTime(void);

#endif /* IPOD_PLAYER_IPC_LOG_H */
