/**
 * \file: adit_logging_dlt.h
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

#ifndef ADIT_LOGGING_DLT_H
#define ADIT_LOGGING_DLT_H

/* DLT logging */
#include <stdio.h>
#include <dlt/dlt.h>

#define _ADIT_LOG_FATAL(log_class, log_string, args...) do {  \
        if (dlt_user_is_logLevel_enabled(&dlt_context_##log_class, DLT_LOG_FATAL) == DLT_RETURN_TRUE) { \
            char dltlog[DLT_USER_BUF_MAX_SIZE]; \
            snprintf(dltlog, DLT_USER_BUF_MAX_SIZE, log_string, ## args); \
            DLT_LOG(dlt_context_##log_class, DLT_LOG_FATAL, DLT_STRING(dltlog)); \
        } \
    } while (0)

#define _ADIT_LOG_ERROR(log_class, log_string, args...) do {  \
        if (dlt_user_is_logLevel_enabled(&dlt_context_##log_class, DLT_LOG_ERROR) == DLT_RETURN_TRUE) { \
            char dltlog[DLT_USER_BUF_MAX_SIZE]; \
            snprintf(dltlog, DLT_USER_BUF_MAX_SIZE, log_string, ## args); \
            DLT_LOG(dlt_context_##log_class, DLT_LOG_ERROR, DLT_STRING(dltlog)); \
        } \
    } while (0)

#define _ADIT_LOG_WARN(log_class, log_string, args...) do {  \
        if (dlt_user_is_logLevel_enabled(&dlt_context_##log_class, DLT_LOG_WARN) == DLT_RETURN_TRUE) { \
            char dltlog[DLT_USER_BUF_MAX_SIZE]; \
            snprintf(dltlog, DLT_USER_BUF_MAX_SIZE, log_string, ## args); \
            DLT_LOG(dlt_context_##log_class, DLT_LOG_WARN, DLT_STRING(dltlog)); \
        } \
    } while (0)

#define _ADIT_LOG_INFO(log_class, log_string, args...) do {  \
        if (dlt_user_is_logLevel_enabled(&dlt_context_##log_class, DLT_LOG_INFO) == DLT_RETURN_TRUE) { \
            char dltlog[DLT_USER_BUF_MAX_SIZE]; \
            snprintf(dltlog, DLT_USER_BUF_MAX_SIZE, log_string, ## args); \
            DLT_LOG(dlt_context_##log_class, DLT_LOG_INFO, DLT_STRING(dltlog)); \
        } \
    } while (0)

#define _ADIT_LOGD_DEBUG(log_class, log_string, args...) do {  \
        if (dlt_user_is_logLevel_enabled(&dlt_context_##log_class, DLT_LOG_DEBUG) == DLT_RETURN_TRUE) { \
            char dltlog[DLT_USER_BUF_MAX_SIZE]; \
            snprintf(dltlog, DLT_USER_BUF_MAX_SIZE, log_string, ## args); \
            DLT_LOG(dlt_context_##log_class, DLT_LOG_DEBUG, DLT_STRING(dltlog)); \
        } \
    } while (0)

#define _ADIT_LOGD_VERBOSE(log_class, log_string, args...) do {  \
        if (dlt_user_is_logLevel_enabled(&dlt_context_##log_class, DLT_LOG_VERBOSE) == DLT_RETURN_TRUE) { \
            char dltlog[DLT_USER_BUF_MAX_SIZE]; \
            snprintf(dltlog, DLT_USER_BUF_MAX_SIZE, log_string, ## args); \
            DLT_LOG(dlt_context_##log_class, DLT_LOG_VERBOSE, DLT_STRING(dltlog)); \
        } \
    } while (0)

#define LOG_DECLARE_CONTEXT(context)             DltContext dlt_context_##context;
#define LOG_IMPORT_CONTEXT(context)              extern DltContext dlt_context_##context;
#define LOG_REGISTER_APP(component, description) do { \
        dlt_check_library_version(_DLT_PACKAGE_MAJOR_VERSION, _DLT_PACKAGE_MINOR_VERSION); \
        if (0 > dlt_register_app(component, description)) { \
            fprintf(stderr, "WARNING: register DLT application failed for %s - %s\n", component, \
                    description); \
        } \
    } while (0)
#define LOG_REGISTER_CONTEXT(context, subComponent, description) do { \
        if (0 > dlt_register_context(&(dlt_context_##context), subComponent, description)) { \
            fprintf(stderr, "WARNING: register DLT context failed for %s - %s\n", subComponent, \
                    description); \
        } \
    } while (0)
#define LOG_REGISTER_CONTEXT_LOGLEVEL_TRACESTATUS(context, subComponent, description, logLevel, traceStatus) do { \
        if (0 > dlt_register_context_ll_ts(&(dlt_context_##context), subComponent, description, logLevel, traceStatus)) { \
            fprintf(stderr, "WARNING: register DLT context failed for %s - %s\n", subComponent, \
                    description); \
        } \
    } while (0)
#define LOG_UNREGISTER_CONTEXT(context)          DLT_UNREGISTER_CONTEXT(dlt_context_##context);
#define LOG_UNREGISTER_APP()                     DLT_UNREGISTER_APP();

#endif /* ADIT_LOGGING_DLT_H */
