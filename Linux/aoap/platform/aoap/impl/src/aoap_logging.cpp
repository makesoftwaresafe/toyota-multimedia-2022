/*
 * aoap_logging.cpp
 *
 *  Created on: Jul 18, 2013
 *      Author: Thilo Bjoern Fickel
 *
 *  This source code is proprietary of ADIT
 *  Copyright (C) Advanced Driver Information Technology Joint Venture GmbH
 *  All rights reserved
 */

#include "aoap_logging.h"
#include <cstring>
#include <stdio.h>
#include <stdarg.h>
#ifdef AOAP_WITH_DLT_LOGGING
#include <endian.h> //required to before including dlt.h otherwise compile error will be thrown
#include <dlt/dlt.h>
#endif //#ifdef AOAP_WITH_DLT_LOGGING
#include <time.h>
#include <sys/time.h>
#include <string.h>

#define AOAP_DBG_LVL stdout
#define AOAP_MAX_MESSAGE_SIZE 1024 //DLT MAX is 65535, we use less
#define AOAP_TIMESTAMP_MESSAGE_SIZE 64

static int gLogLevel = static_cast<int>(AOAP::Logging::eLogInfo);
static unsigned int gLogging = eLogToDLT; //enable logging of DLT only per default
static bool gPrependTimestamps = true; //default is printing with timestamps
static FILE* gAoapLogFile = NULL;
static char gLogFileLocation[512] = "/tmp/aoapPrint.log";

#ifdef AOAP_WITH_DLT_LOGGING
static DltContext gDltContext;
#endif //#ifdef AOAP_WITH_DLT_LOGGING

const char* AOAP::Logging::getCurrentTime(char *pBuffer, unsigned int bufferLen)
{
    struct timeval tv;
    time_t nowtime;
    struct tm *nowtm;
    char tmbuf[64];

    gettimeofday(&tv, NULL);
    nowtime = tv.tv_sec;
    nowtm = localtime(&nowtime);
    strftime(tmbuf, sizeof tmbuf, "%H:%M:%S", nowtm); //omit date, only print time

    snprintf(pBuffer, bufferLen, "%s.%06d : ", tmbuf, static_cast<int>(tv.tv_usec));

    return pBuffer;
}

void AOAP::Logging::setDltLogging(bool enable)
{
    if (enable)
    {
        enableLogging(eLogToDLT);
    }
    else
    {
        disableLogging(eLogToDLT);
    }
}

bool AOAP::Logging::getDltLogging(void)
{
    return (gLogging && eLogToDLT);
}

void AOAP::Logging::enableLogging(t_aoap_logging_destination destination)
{
    gLogging |= destination;
    dbgPrintLine(eLogInfo, "%s(%d) enabled => 0x%x",
            __FUNCTION__, static_cast<unsigned int>(destination), gLogging);
}

void AOAP::Logging::disableLogging(t_aoap_logging_destination destination)
{
    gLogging &= ~(destination);
    dbgPrintLine(eLogInfo, "%s(%d) disabled => 0x%x",
            __FUNCTION__, static_cast<unsigned int>(destination), gLogging);
}

void AOAP::Logging::setLogLevel(int logLevel, bool prependTime)
{
    gPrependTimestamps = prependTime;
    gLogLevel = logLevel;
    dbgPrintLine(eLogInfo, "%s(%d)", __FUNCTION__, logLevel);
}

unsigned int AOAP::Logging::getLogDestination(void)
{
    return gLogging;
}

void AOAP::Logging::setLogDestination(t_aoap_logging_destination destination, bool enable, const char* filename)
{
    switch (destination)
    {
        case eLogToTTFis:
        {
            dbgPrintLine(eLogError, "%s(type=%d, enable=%s, filename='%s') ERROR: TTFis logging is not supported",
                    __FUNCTION__, destination, enable ? "true" : "false", filename);
            break;
        }
        case eLogToDLT:
        case eLogToStdout:
        case eLogToFile:
        {
            if (enable)
            {
                enableLogging(destination);
            }
            else
            {
                disableLogging(destination);
            }
            break;
        }
        default:
        {
            dbgPrintLine(eLogError, "%s(type=%d, enable=%s, filename='%s') ERROR: logging destination not supported",
                    __FUNCTION__, destination, enable ? "true" : "false", filename);
            break;
        }
    }

    /* Update log filename */
    if (filename && (gLogging & eLogToFile))
    {
        if (sizeof(gLogFileLocation) > strlen(filename))
        {
            strcpy(gLogFileLocation, filename);
            /* Close file if already open */
            if (gAoapLogFile)
            {
                fflush(gAoapLogFile);
                fclose(gAoapLogFile);
                /* file will be open at first log message */
                gAoapLogFile = NULL;
            }
        }
        else
        {
            dbgPrintLine(eLogError, "%s(type=%d, enable=%s, filename='%s') ERROR: Log filename too long",
                    __FUNCTION__, destination, enable ? "true" : "false", filename);
        }
    }
}

int AOAP::Logging::getLogLevel(void)
{
    return gLogLevel;
}

void AOAP::Logging::dbgPrintLine(tLogLevel level, const char *message, ...)
{
    if (gLogging &&
            ((gLogging & eLogToDLT) || (gLogLevel >= static_cast<int>(level))))
    {
        char timestamp[AOAP_TIMESTAMP_MESSAGE_SIZE];
        char parsedMsg[AOAP_MAX_MESSAGE_SIZE];
        memset(parsedMsg, 0, AOAP_MAX_MESSAGE_SIZE);

        /*PRQA: Lint Message 530: va_list args gets initialized via va_start */
        /*lint -save -e530*/
        va_list args;
        va_start (args, message);
        vsnprintf(parsedMsg, AOAP_MAX_MESSAGE_SIZE, message, args);
        va_end (args);
        /*lint -restore*/

        if (gLogging & eLogToDLT)
        {
#ifdef AOAP_WITH_DLT_LOGGING
            /* PRQA: Lint Message 160: It's OK to suppress this warning in C++ code */
            /*lint -save -e160*/
            DLT_LOG(gDltContext, static_cast<DltLogLevelType>(level), DLT_STRING(parsedMsg));
            /*lint -restore*/
#endif //#ifdef AOAP_WITH_DLT_LOGGING
        }

        if ((gLogging & eLogToFile) && (gLogLevel >= static_cast<int>(level)))
        {
            if (!gAoapLogFile)
            {
                gAoapLogFile = fopen(gLogFileLocation, "w");
            }

            if (gAoapLogFile)
            {
                fprintf(gAoapLogFile, "%sAOAP: %s\n",
                        gPrependTimestamps ? getCurrentTime(timestamp, sizeof(timestamp)) : "",
                        parsedMsg); //and append line break
                fflush(gAoapLogFile);
            }
        }

        if ((gLogging & eLogToStdout) && (gLogLevel >= static_cast<int>(level)))
        {
            fprintf(AOAP_DBG_LVL, "%sAOAP: %s\n",
                    gPrependTimestamps ? getCurrentTime(timestamp, sizeof(timestamp)) : "",
                    parsedMsg); //and append line break
        }
    }
}

void AOAP::Logging::dbgPrint(tLogLevel level, const char *message, ...)
{
    if (gLogging &&
            ((gLogging & eLogToDLT) || (gLogLevel >= static_cast<int>(level))))
    {
        char parsedMsg[AOAP_MAX_MESSAGE_SIZE];
        memset(parsedMsg, 0, AOAP_MAX_MESSAGE_SIZE);

        /*PRQA: Lint Message 530: va_list args gets initialized via va_start */
        /*lint -save -e530*/
        va_list args;
        va_start (args, message);
        vsnprintf(parsedMsg, AOAP_MAX_MESSAGE_SIZE, message, args);
        va_end (args);
        /*lint -restore*/

        if (gLogging & eLogToDLT)
        {
#ifdef AOAP_WITH_DLT_LOGGING
            /* PRQA: Lint Message 160: it's OK to suppress this warning in C code */
            /*lint -save -e160*/
            DLT_LOG(gDltContext, static_cast<DltLogLevelType>(level), DLT_STRING(parsedMsg));
            /*lint -restore*/
#endif //#ifdef AOAP_WITH_DLT_LOGGING
        }

        if ((gLogging & eLogToFile) && (gLogLevel >= static_cast<int>(level)))
        {
            if (!gAoapLogFile)
            {
                gAoapLogFile = fopen(gLogFileLocation, "w");
            }

            if (gAoapLogFile)
            {
                fprintf(gAoapLogFile, "%s", parsedMsg);
                fflush(gAoapLogFile);
            }
        }

        if ((gLogging & eLogToStdout) && (gLogLevel >= static_cast<int>(level)))
        {
            fprintf(AOAP_DBG_LVL, "%s", parsedMsg);
        }
    }
}

void AOAP::Logging::registerWithDlt(void)
{
#ifdef AOAP_WITH_DLT_LOGGING
    if (gLogging & eLogToDLT)
    {
        int result = dlt_register_context_ll_ts(&gDltContext, "AOAP", "AOAP", gLogLevel, DLT_TRACE_STATUS_ON);
        if (result < 0)
        {
            disableLogging(eLogToDLT);
            dbgPrintLine(eLogFatal,
                    "registerWithDlt() FATAL ERROR: Failed to register DLT context with code=%d. Is the DLT application registered?",
                    result);
        }

        dbgPrintLine(eLogInfo, "registerWithDlt() register context with result=%d", result);
    }
    else
    {
        dbgPrintLine(eLogInfo, "registerWithDlt() DLT disabled -> nothing to register");
    }
#else
    dbgPrintLine(eLogInfo, "registerWithDlt() DLT is globally disabled");
    disableLogging(eLogToDLT);
#endif //#ifdef AOAP_WITH_DLT_LOGGING
}

void AOAP::Logging::unregisterWithDlt(void)
{
#ifdef AOAP_WITH_DLT_LOGGING
    if (gLogging & eLogToDLT)
    {
        DLT_UNREGISTER_CONTEXT(gDltContext);
        dbgPrintLine(eLogInfo, "unregisterWithDlt()");
    }
    else
    {
        dbgPrintLine(eLogInfo, "unregisterWithDlt() -> nothing to unregister");
    }
#endif //#ifdef AOAP_WITH_DLT_LOGGING
}
