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
 * ipu4_log.c
 *
 * This file contains implementation of the logging functionality.
 */

#include <errno.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdio.h>

#include "ipu4_log.h"

// Static variables
#ifdef LIBSLOG2
static slog2_buffer_set_config_t buffer_config;
static slog2_buffer_t buffer_handle[EVENT_LOG_BUFFER_NUM_BUFFERS];
static const char * buffer_set_name = "ipu4";
static const char * buffer_name_errors = "errors_warnings";
static const char * buffer_name_info = "info";
static const char * buffer_name_debug = "debug";
#else
#if !defined(_SLOGC_CAMERA)
    #define _SLOGC_CAMERA 35
#endif
#endif

/**
 * Logging initialization routine.
 *
 * This initializes the logging module before first use.
 * @return EOK on success, error code on failure.
 */
int ipu4_log_init(void)
{
#ifdef LIBSLOG2
    // Initialize the buffer configuration
    // 4 kB buffer to store our errors/warnings
    // 12 kB buffer to store our notice/info
    // 16 kB buffer to store our debug information
    buffer_config.buffer_set_name = (char *) buffer_set_name;
    buffer_config.num_buffers = 3;
    buffer_config.verbosity_level = SLOG2_DEBUG1;
    buffer_config.buffer_config[EVENT_LOG_BUFFER_ERRORS].buffer_name = (char *) buffer_name_errors;
    buffer_config.buffer_config[EVENT_LOG_BUFFER_ERRORS].num_pages = 1;
    buffer_config.buffer_config[EVENT_LOG_BUFFER_INFO].buffer_name = (char *) buffer_name_info;
    buffer_config.buffer_config[EVENT_LOG_BUFFER_INFO].num_pages = 3;
    buffer_config.buffer_config[EVENT_LOG_BUFFER_DEBUG].buffer_name = (char *) buffer_name_debug;
    buffer_config.buffer_config[EVENT_LOG_BUFFER_DEBUG].num_pages = 4;

    // Register the Buffer Set
    if (slog2_register(&buffer_config, buffer_handle, 0) != EOK) {
        fprintf(stderr, "IPU4: error registering slogger2 buffer!\n");
        return EIO;
    }
#endif
    return EOK;
}

/**
 * Logging routine.
 *
 * This is the function called by our macros to add a log entry.
 * @param[IN] severity At what log severity to log this entry
 * @param[IN] buffer Which of the logging buffer to use for this entry
 * @param[IN] format Format string for the log entry; followed by any parameters
 * @return EOK on success, error code on failure.
 */
#ifdef LIBSLOG2
void ipu4_log(int severity, log_buffers_t buffer, const char *format, ...)
{
    va_list             arg;
    slog2_buffer_t      slog2_buffer;
    // preserve errno
    int                 ext_errno = errno;

    if (buffer >= EVENT_LOG_BUFFER_NUM_BUFFERS) {
        return;
    }
    slog2_buffer = buffer_handle[buffer];
    if (slog2_buffer == NULL) {
        return;
    }

    // add entry to slog2
    va_start(arg, format);
    vslog2f(slog2_buffer, 0, severity, format, arg);
    va_end(arg);

    errno = ext_errno;
}
#else
void ipu4_log(int severity, log_buffers_t buffer, const char *format, ...)
{
    va_list             arg;
    // preserve errno
    int                 ext_errno = errno;

    // buffer is ignore for slog

    // add entry to slog
    va_start(arg, format);
    vslogf(_SLOGC_CAMERA, severity, format, arg);
    va_end(arg);

    errno = ext_errno;
}
#endif


#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/capture/intel-ipu4/resmgr/ipu4/ipu4_log.c $ $Rev: 838597 $")
#endif
