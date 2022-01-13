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

#ifndef _QNX_LINUX_KERNEL_PRINTK__
#define _QNX_LINUX_KERNEL_PRINTK__

#include <linux/kern_levels.h>
#include <linux/debug.h>

#include <string.h>
#include <sys/slog.h>
#include <sys/slogcodes.h>
#include <sys/trace.h>

struct va_format {
       const char *fmt;
       va_list *va;
};

#define printk_once printk
#define printk_ratelimited printk

#ifndef pr_fmt
#define pr_fmt(fmt) fmt
#endif

#define pr_emerg(fmt, ...) \
	printk(KERN_EMERG pr_fmt(fmt), ##__VA_ARGS__)
#define pr_alert(fmt, ...) \
	printk(KERN_ALERT pr_fmt(fmt), ##__VA_ARGS__)
#define pr_crit(fmt, ...) \
	printk(KERN_CRIT pr_fmt(fmt), ##__VA_ARGS__)
#define pr_err(fmt, ...) \
	printk(KERN_ERR pr_fmt(fmt), ##__VA_ARGS__)
#define pr_warning(fmt, ...) \
	printk(KERN_WARNING pr_fmt(fmt), ##__VA_ARGS__)
#define pr_warn pr_warning
#define pr_notice(fmt, ...) \
	printk(KERN_NOTICE pr_fmt(fmt), ##__VA_ARGS__)
#define pr_info(fmt, ...) \
	printk(KERN_INFO pr_fmt(fmt), ##__VA_ARGS__)
#define pr_cont(fmt, ...) \
	printk(KERN_CONT fmt, ##__VA_ARGS__)
#define pr_info_once(fmt, ...) \
	printk_once(KERN_INFO pr_fmt(fmt), ##__VA_ARGS__)
#define pr_debug(fmt, ...) \
        { \
        if (0) \
        printk(KERN_INFO pr_fmt(fmt), ##__VA_ARGS__); \
        }

#ifndef QNX_LOG_PRINTF
#define QNX_LOG_PRINTF	0x10000000
#endif /* QNX_LOG_PRINTF */
#ifndef QNX_LOG_SLOGF
#define QNX_LOG_SLOGF	0x20000000
#endif /* QNX_LOG_SLOGF */
#ifndef QNX_LOG_TRACELOG
#define QNX_LOG_TRACELOG	0x40000000
#endif /* QNX_LOG_TRACELOG */
extern unsigned int ipu4_drv_log_debug;

static inline void printk(const char *s, ...)
{
	va_list va;
	va_start(va, s);
	if (ipu4_drv_log_debug & QNX_LOG_PRINTF) {
		vfprintf(stdout, s+2, va); fflush(stdout);
	}
	if (ipu4_drv_log_debug & QNX_LOG_SLOGF) {
		char tempstr[1024];
		int tempstrlen;
		strncpy(tempstr, s+2, sizeof(tempstr)-1);
		tempstrlen=strlen(tempstr);
		if ((tempstrlen>0) && (tempstr[tempstrlen-1] == '\n')) {
			tempstr[tempstrlen-1] = 0x00;
		}
		vslogf(SLOGC_SELF, _SLOG_INFO, tempstr, va);
	}
	if (ipu4_drv_log_debug & QNX_LOG_TRACELOG) {
		trace_vnlogf(_NTO_TRACE_USERFIRST+915, 0, s+2, va);
	}
	va_end(va);
}

static inline void vprintk(const char *s, va_list arg)
{
	if (ipu4_drv_log_debug & QNX_LOG_PRINTF) {
		vfprintf(stdout, s+2, arg); fflush(stdout);
	}
	if (ipu4_drv_log_debug & QNX_LOG_SLOGF) {
		char tempstr[1024];
		int tempstrlen;
		strncpy(tempstr, s+2, sizeof(tempstr)-1);
		tempstrlen=strlen(tempstr);
		if ((tempstrlen>0) && (tempstr[tempstrlen-1] == '\n')) {
			tempstr[tempstrlen-1] = 0x00;
		}
		vslogf(SLOGC_SELF, _SLOG_INFO, tempstr, arg);
	}
	if (ipu4_drv_log_debug & QNX_LOG_TRACELOG) {
		trace_vnlogf(_NTO_TRACE_USERFIRST+915, 0, s+2, arg);
	}
}

enum {
	DUMP_PREFIX_NONE,
	DUMP_PREFIX_ADDRESS,
	DUMP_PREFIX_OFFSET
};

void print_hex_dump(const char *level, const char *prefix_str, int prefix_type,
					int rowsize, int groupsize,
					const void *buf, size_t len, bool ascii);


#endif


#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/capture/intel-ipu4/driver/intel-ipu4-drv/include/linux/printk.h $ $Rev: 838597 $")
#endif
