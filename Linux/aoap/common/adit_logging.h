/**
 * \file: adit_logging.h
 *
 * \version: $Id:$
 *
 * \release: $Name:$
 *
 * <brief description>.
 * <detailed description>
 * \component: ADIT logging macros
 *
 * \author: J. Harder / ADIT/SW1 / jharder@de.adit-jv.com
 *
 * \copyright (c) 2013 Advanced Driver Information Technology.
 * This code is developed by Advanced Driver Information Technology.
 * Copyright of Advanced Driver Information Technology, Bosch, and DENSO.
 * All rights reserved.
 *
 * \see <related items>
 *
 * \history
 *
 ***********************************************************************/

#ifndef ADIT_LOGGING_H
#define ADIT_LOGGING_H

/* check if multiple mechanisms were defined */
#if defined(ADIT_LOGGING_DLT) && defined(ADIT_LOGGING_PRINTF) || \
    defined(ADIT_LOGGING_DLT) && defined(ADIT_LOGGING_OFF) || \
    defined(ADIT_LOGGING_PRINTF) && defined(ADIT_LOGGING_OFF)
#   warning "define only one ADIT_LOGGING_xxx (DLT|PRINTF|OFF)! Default to PRINTF"
#   ifdef ADIT_LOGGING_DLT
#       undef ADIT_LOGGING_DLT
#   endif
#   ifdef ADIT_LOGGING_OFF
#       undef ADIT_LOGGING_OFF
#   endif
#   ifndef ADIT_LOGGING_PRINTF
#       define ADIT_LOGGING_PRINTF
#   endif
#endif

/* check if no mechanism was defined */
#if !defined(ADIT_LOGGING_DLT) && !defined(ADIT_LOGGING_PRINTF) && !defined(ADIT_LOGGING_OFF)
#   warning "ADIT_LOGGING_xxx not defined (DLT|PRINTF|OFF)! Default to PRINTF"
#   define ADIT_LOGGING_PRINTF
#endif

/* define this to include function name in log string */
#ifdef ADIT_LOGGING_PRINT_FUNCTION
#define ADIT_LOGGING_FUNCTION_STR "@" __FUNCTION__
#else
#define ADIT_LOGGING_FUNCTION_STR ""
#endif

/* define this to include file name in log string */
#ifdef ADIT_LOGGING_PRINT_FILE
#define ADIT_LOGGING_FILE_STR __FILE__ ""
#else
#define ADIT_LOGGING_FILE_STR ""
#endif

/* system levels, usually activated */
#define LOG_FATAL(args)         _ADIT_LOG_FATAL       args
#define LOG_ERROR(args)         _ADIT_LOG_ERROR       args
#define LOG_WARN(args)          _ADIT_LOG_WARN        args
#define LOG_INFO(args)          _ADIT_LOG_INFO        args

/* debug levels, usually deactivated */
#define LOGD_DEBUG(args)        _ADIT_LOGD_DEBUG      args
#define LOGD_VERBOSE(args)      _ADIT_LOGD_VERBOSE    args

/* log rate variants */
#define LOG_WARN_RATE(deltaSeconds, args)       _ADIT_LOG_RATE_BEGIN(deltaSeconds) \
                                                _ADIT_LOG_WARN       args ; \
                                                _ADIT_LOG_RATE_END
#define LOG_INFO_RATE(deltaSeconds, args)       _ADIT_LOG_RATE_BEGIN(deltaSeconds) \
                                                _ADIT_LOG_INFO       args ; \
                                                _ADIT_LOG_RATE_END
#define LOGD_DEBUG_RATE(deltaSeconds, args)     _ADIT_LOG_RATE_BEGIN(deltaSeconds) \
                                                _ADIT_LOGD_DEBUG     args ; \
                                                _ADIT_LOG_RATE_END
#define LOGD_VERBOSE_RATE(deltaSeconds, args)   _ADIT_LOG_RATE_BEGIN(deltaSeconds) \
                                                _ADIT_LOGD_VERBOSE   args ; \
                                                _ADIT_LOG_RATE_END

// TODO think about thread-safety
#define _ADIT_LOG_RATE_BEGIN(deltaSeconds) \
    do { \
        static struct timespec adit_log_rate_last = { 0, 0 }; \
        static uint32_t adit_log_rate_counter = 0; \
        struct timespec adit_log_rate_now; \
        clock_gettime(CLOCK_MONOTONIC, &adit_log_rate_now); \
        adit_log_rate_counter++; \
        if (adit_log_rate_now.tv_sec > adit_log_rate_last.tv_sec + deltaSeconds) \
        {
#define _ADIT_LOG_RATE_END \
            adit_log_rate_last = adit_log_rate_now; \
            adit_log_rate_counter = 0; \
        } \
    } while (0)
#define LOG_RATE_COUNTER adit_log_rate_counter

/* DLT logging */
#ifdef ADIT_LOGGING_DLT

#include "adit_logging_dlt.h"

/* printf logging; use only for development! */
#elif defined(ADIT_LOGGING_PRINTF)

#include "adit_logging_printf.h"

/* no logging */
#else

#define _ADIT_LOG_FATAL(log_class, log_string, ...)
#define _ADIT_LOG_ERROR(log_class, log_string, ...)
#define _ADIT_LOG_WARN(log_class, log_string, ...)
#define _ADIT_LOG_INFO(log_class, log_string, ...)
#define _ADIT_LOGD_DEBUG(log_class, log_string, ...)
#define _ADIT_LOGD_VERBOSE(log_class, log_string, ...)

#define LOG_DECLARE_CONTEXT(context)
#define LOG_IMPORT_CONTEXT(context)
#define LOG_REGISTER_APP(component, description)
#define LOG_REGISTER_CONTEXT(context, subComponent, description)
#define LOG_REGISTER_CONTEXT_LOGLEVEL_TRACESTATUS(context, subComponent, description, logLevel, traceStatus)
#define LOG_UNREGISTER_CONTEXT(context)
#define LOG_UNREGISTER_APP()

#endif

#endif /* ADIT_LOGGING */
