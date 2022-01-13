#ifndef IPOD_DLT_LOG_H
#define IPOD_DLT_LOG_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <adit_typedef.h>
#include <dlt/dlt.h>

#define MAX_DLT_LOG_SIZE 2042
#define IPOD_DLT_STR_FORMAT "%s:%d:%s:"
#define IPOD_DLT_HEX_FORMAT "%02x  "
#define IPOD_DLT_HEX_FORMAT_LEN3 3
#define IPOD_DLT_DIGITS_IN_LINE_NUM 10


/**************************************************************************//**
 * This function forms and sends the logmessage to DLT daemon.
 *
 * \param context   Pointer to the registered DLT context
 * \param loglevel  Log level which Specifies the priority/importance of log message
 * \param filename  Name of the file from which this API is called
 * \param linenum   Line number in which this API is called
 * \param function  Function name
 * \param logString Message to be printed in DLT viewer
 *
 * \return None
 * \see
 * \note
 ******************************************************************************/
void iPodDltLog(DltContext* context, DltLogLevelType loglevel, const char* filename, U32 linenum, const char* function, const char* logString, ...) __attribute__ ( (format (printf, 6, 7)));

/**************************************************************************//**
 * This function converts Hexadecimal values into String and sends to DLT.
 *
 * \param context   Pointer to the registered DLT context
 * \param loglevel  Log level which Specifies the priority/importance of log message
 * \param filename  Name of the file from which this API is called
 * \param linenum   Line number in which this API is called
 * \param function  Function name
 * \param logString Message to be printed in DLT viewer
 * \param HexArray  Hexadecimal array to be converted as string
 * \param ArrayLen  Length of HexArray
 *
 * \return None
 * \see
 * \note
 ******************************************************************************/
void iPodDltConvertAndLog(DltContext* context, DltLogLevelType loglevel, const char* filename, S32 linenum,const char* function, const char* logString, U8* HexArray, S32 ArrayLen);

#ifdef __cplusplus
}
#endif

#endif /* IPOD_DLT_LOG_H */
