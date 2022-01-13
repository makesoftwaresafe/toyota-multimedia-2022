/*
* Copyright (c) 2017 QNX Software Systems.
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version 2
* of the License, or (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

/*
 * ipu4_log.h
 *
 * This module permits logging of error and debug information.
 */

#ifndef _IPU4_LOG_H_INCLUDED
#define _IPU4_LOG_H_INCLUDED

#ifdef LIBSLOG2
#include <sys/slog2.h>

#define IPU4_LOG_CRITICAL                   SLOG2_CRITICAL
#define IPU4_LOG_ERROR                      SLOG2_ERROR
#define IPU4_LOG_WARNING                    SLOG2_WARNING
#define IPU4_LOG_NOTICE                     SLOG2_NOTICE
#define IPU4_LOG_INFO                       SLOG2_INFO
#define IPU4_LOG_DEBUG1                     SLOG2_DEBUG1
#define IPU4_LOG_DEBUG2                     SLOG2_DEBUG2

#else
#include <sys/slogcodes.h>

#define IPU4_LOG_CRITICAL                   _SLOG_CRITICAL
#define IPU4_LOG_ERROR                      _SLOG_ERROR
#define IPU4_LOG_WARNING                    _SLOG_WARNING
#define IPU4_LOG_NOTICE                     _SLOG_NOTICE
#define IPU4_LOG_INFO                       _SLOG_INFO
#define IPU4_LOG_DEBUG1                     _SLOG_DEBUG1
#define IPU4_LOG_DEBUG2                     _SLOG_DEBUG2

#endif

#ifndef __GNUC__
#define __attribute__(x) /*NOTHING*/
#endif

/**
 * @brief Logging buffer types
 * @details
 * Really a private definition to be used by macros below
 */
typedef enum {
    EVENT_LOG_BUFFER_ERRORS = 0,
    EVENT_LOG_BUFFER_INFO,
    EVENT_LOG_BUFFER_DEBUG,
    EVENT_LOG_BUFFER_NUM_BUFFERS
} log_buffers_t;

/**
 * @brief Macros to add a log at a given severity level
 * @details
 * Call the appropriate macro to add a log entry at a given severity level; the
 * log will only be visible if this severity level is below or equal our
 * verbosity level.  Macros add in function name and line number to each log entry.
 *
 * @param format printf-style format string
 * @param args variable argument list
 */
#define LOG_CRITICAL(format, args...) \
    ipu4_log(IPU4_LOG_CRITICAL, EVENT_LOG_BUFFER_ERRORS, "%s(%d): " format, __PRETTY_FUNCTION__, __LINE__, ##args);

#define LOG_ERROR(format, args...) \
    ipu4_log(IPU4_LOG_ERROR, EVENT_LOG_BUFFER_ERRORS, "%s(%d): " format, __PRETTY_FUNCTION__, __LINE__, ##args);

#define LOG_WARNING(format, args...) \
    ipu4_log(IPU4_LOG_WARNING, EVENT_LOG_BUFFER_ERRORS, "%s(%d): " format, __PRETTY_FUNCTION__, __LINE__, ##args);

#define LOG_NOTICE(format, args...) \
    ipu4_log(IPU4_LOG_NOTICE, EVENT_LOG_BUFFER_INFO, "%s(%d): " format, __PRETTY_FUNCTION__, __LINE__, ##args);

#define LOG_INFO(format, args...) \
    ipu4_log(IPU4_LOG_INFO, EVENT_LOG_BUFFER_INFO, "%s(%d): " format, __PRETTY_FUNCTION__, __LINE__, ##args);

#define LOG_DEBUG1(format, args...) \
    ipu4_log(IPU4_LOG_DEBUG1, EVENT_LOG_BUFFER_DEBUG, "%s(%d): " format, __PRETTY_FUNCTION__, __LINE__, ##args);

#define LOG_DEBUG2(format, args...) \
    ipu4_log(IPU4_LOG_DEBUG2, EVENT_LOG_BUFFER_DEBUG, "%s(%d): " format, __PRETTY_FUNCTION__, __LINE__, ##args);

/**
 * @brief Add a log entry at a given severity level
 * @details
 * Macros call this function to add a log entry; do not call directly, use the
 * macros instead.  Entry for function and line number will be added to format string.
 *
 * @param severity 0-7 defined severity levels
 * @param buffer to use for the logging
 * @format printf-style format string
 */
void ipu4_log(int severity, log_buffers_t buffer, const char *format, ...) __attribute__ ((format(printf, 3, 4)));

/**
 * @brief Initialize our logging system before first use
 *
 * @return @c errno
 */
int ipu4_log_init(void);

#endif // _IPU4_LOG_H_INCLUDED


#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/capture/intel-ipu4/resmgr/ipu4/ipu4_log.h $ $Rev: 838597 $")
#endif
