/**
 * \file: rn_log.h
 *
 * ADIT SRC core implementation. Logging functions.
 *
 * author: Andreas Pape / ADIT / SW1 / apape@de.adit-jv.com
 *
 * copyright (c) 2013 Advanced Driver Information Technology.
 * This code is developed by Advanced Driver Information Technology.
 * Copyright of Advanced Driver Information Technology, Bosch, and DENSO.
 * All rights reserved.
 *
 ***********************************************************************/
#ifndef __RN_LOG_H__
#define __RN_LOG_H__

extern void (*rate_error)(const char *file, int line, const char *function, int err, const char *fmt, ...);
extern void (*rate_log)(const char *file, int line, const char *function, int err, const char *fmt, ...);

//#define RN_DEBUG

#ifdef RN_DEBUG
#define RN_LOG(...) rate_log(__FILE__, __LINE__, __FUNCTION__, 0, __VA_ARGS__);
#else
#define RN_LOG(...)
#endif

#define RN_ERR(...) rate_error(__FILE__, __LINE__, __FUNCTION__, 0, __VA_ARGS__);
#define RN_ERR_ERRNO(...) rate_error(__FILE__, __LINE__, __FUNCTION__, errno, __VA_ARGS__);

#endif /* __RN_LOG_H__*/

