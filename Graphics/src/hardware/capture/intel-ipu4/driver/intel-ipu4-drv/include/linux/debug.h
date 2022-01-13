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

#ifndef _QNX_DEBUG_H
#define _QNX_DEBUG_H

#include <sys/neutrino.h>
#include <sys/trace.h>
#include <sys/slogcodes.h>
#include <errno.h>

#define in_dbg_master() (0)

#define _SLOG_MAJOR _SLOGC_GRAPHICS
#define _SLOG_MINOR 915

#define SLOGC_IGPU _SLOG_SETCODE(_SLOG_MAJOR, _SLOG_MINOR)
#define SLOGC_SELF SLOGC_IGPU

/* trace routine */
#define qnx_trace_log(format,...)						   \
	trace_logf(_NTO_TRACE_USERFIRST+915, " ipu4 [%s:%d]" format, \
			   __func__, __LINE__, ##__VA_ARGS__)

#define qnx_warning(format,...)								\
	do {													\
		slogf(SLOGC_SELF, _SLOG_WARNING, "[%s:%d]" format,	\
			  __func__, __LINE__, ##__VA_ARGS__);			\
	} while(0)

#define qnx_error(format, ...)											\
	do {																\
		slogf(SLOGC_SELF, _SLOG_ERROR, "[errno:%d @ %s:%d]" format,		\
			  errno, __func__, __LINE__, ##__VA_ARGS__);				\
	}while(0)

#endif

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/capture/intel-ipu4/driver/intel-ipu4-drv/include/linux/debug.h $ $Rev: 838597 $")
#endif
