#include "ipod_dlt_log.h"

void iPodDltLog(DltContext* context, DltLogLevelType loglevel, const char* filename, U32 linenum,const char* function, const char* logString, ...)
{
    char dltmsg[MAX_DLT_LOG_SIZE] = "";
    S32 len = 0;
    /*PRQA: Lint Message 530: This is intention. va_list will be initialized by va_start (see stdarg.h). */
    /*lint -save -e530*/
    va_list LogArgs;
    
    if((context != NULL) && (filename != NULL) && (logString != NULL))
    {
        len = snprintf(dltmsg, MAX_DLT_LOG_SIZE, IPOD_DLT_STR_FORMAT, filename, linenum, function);
        if(len > 0)
        {
            va_start (LogArgs, logString);
            vsnprintf(dltmsg+len, MAX_DLT_LOG_SIZE-len, logString, LogArgs);
            va_end (LogArgs);
            /*lint -restore*/

            DLT_LOG(*context, loglevel, DLT_STRING(dltmsg));
        }
    }

}


void iPodDltConvertAndLog(DltContext* context, DltLogLevelType loglevel, const char* filename, S32 linenum,const char* function, const char* logString, U8* HexArray, S32 ArrayLen)
{
    U8 dltmesg[MAX_DLT_LOG_SIZE] = "";
    S32 i = 0;

    if((context != NULL) && (filename != NULL) && (logString != NULL)&&(function != NULL))
    {
        for(i = 0; (i < ArrayLen) &&(i < (MAX_DLT_LOG_SIZE/IPOD_DLT_HEX_FORMAT_LEN3)); i++)
        {
            sprintf((char*)dltmesg+(i*IPOD_DLT_HEX_FORMAT_LEN3), IPOD_DLT_HEX_FORMAT, (unsigned int)HexArray[i]);
        }

        iPodDltLog(context, loglevel, filename, linenum, function, logString, dltmesg);
    }
}
