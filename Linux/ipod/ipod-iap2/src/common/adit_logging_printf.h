/**
 * \file: adit_logging_printf.h
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

#ifndef ADIT_LOGGING_PRINTF_H
#define ADIT_LOGGING_PRINTF_H

#include <stdio.h>

#define _ADIT_LOG_FATAL(log_class, log_string, ...) do { fprintf(stderr, "\033[1;31mFATAL %s%s" \
     #log_class ":\033[0m " log_string "\n", ADIT_LOGGING_FILE_STR, ADIT_LOGGING_FUNCTION_STR, \
     ##__VA_ARGS__); } while (0)
#define _ADIT_LOG_ERROR(log_class, log_string, ...) do { fprintf(stderr, "\033[1;31mERROR %s%s" \
     #log_class ":\033[0m " log_string "\n", ADIT_LOGGING_FILE_STR, ADIT_LOGGING_FUNCTION_STR, \
     ##__VA_ARGS__); } while (0)
#define _ADIT_LOG_WARN(log_class, log_string, ...) do { fprintf(stderr, "\033[1;33mWARN %s%s" \
     #log_class ":\033[0m " log_string "\n", ADIT_LOGGING_FILE_STR, ADIT_LOGGING_FUNCTION_STR, \
     ##__VA_ARGS__); } while (0)
#define _ADIT_LOG_INFO(log_class, log_string, ...) do { fprintf(stderr, "\033[1;32mINFO %s%s" \
     #log_class ":\033[0m " log_string "\n", ADIT_LOGGING_FILE_STR, ADIT_LOGGING_FUNCTION_STR, \
     ##__VA_ARGS__); } while (0)

#define _ADIT_LOGD_DEBUG(log_class, log_string, ...) do { fprintf(stdout, "DEBUG %s%s" \
     #log_class ": " log_string "\n", ADIT_LOGGING_FILE_STR, ADIT_LOGGING_FUNCTION_STR, \
     ##__VA_ARGS__); } while (0)
#ifdef ADIT_LOGGING_PRINTF_VERBOSE
#define _ADIT_LOGD_VERBOSE(log_class, log_string, ...) do { fprintf(stdout, "VERBOSE %s%s" \
     #log_class ": " log_string "\n", ADIT_LOGGING_FILE_STR, ADIT_LOGGING_FUNCTION_STR, \
     ##__VA_ARGS__); } while (0)
#else // ADIT_LOGGING_PRINTF_VERBOSE
#define _ADIT_LOGD_VERBOSE(log_class, log_string, ...)
#endif // ADIT_LOGGING_PRINTF_VERBOSE

#define LOG_DECLARE_CONTEXT(context)
#define LOG_IMPORT_CONTEXT(context)
#define LOG_REGISTER_APP(component, description)                    ((void)0)
#define LOG_REGISTER_CONTEXT(context, subComponent, description)    ((void)0)
#define LOG_UNREGISTER_CONTEXT(context)                             ((void)0)
#define LOG_UNREGISTER_APP()                                        ((void)0)

#endif /* ADIT_LOGGING_PRINTF_H */
