#ifndef _QNX_LINUX_KERNEL_PRINTK__
#define _QNX_LINUX_KERNEL_PRINTK__

#include <linux/kern_levels.h>
#include <linux/debug.h>

#include <stdio.h>
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
#define pr_warn_once(fmt, ...) \
	printk_once(KERN_WARNING pr_fmt(fmt), ##__VA_ARGS__)
#ifndef __QNXNTO__
#define pr_debug(fmt, ...) \
	printk(KERN_INFO pr_fmt(fmt), ##__VA_ARGS__)
#else
#ifdef NDEBUG
#define pr_debug(fmt, ...)
#else
#define pr_debug(fmt, ...) \
	printk(KERN_INFO pr_fmt(fmt), ##__VA_ARGS__)
#endif /* NDEBUG */
#endif /* __QNXNTO__ */

#ifndef DRM_UT_QNX_PRINTF
#define DRM_UT_QNX_PRINTF	0x10000000
#endif /* DRM_UT_QNX_PRINTF */
#ifndef DRM_UT_QNX_SLOGF
#define DRM_UT_QNX_SLOGF	0x20000000
#endif /* DRM_UT_QNX_SLOGF */
#ifndef DRM_UT_QNX_TRACELOG
#define DRM_UT_QNX_TRACELOG	0x40000000
#endif /* DRM_UT_QNX_TRACELOG */
extern unsigned int drm_debug;

__printf(1,2)
void printk(const char *s, ...);
__printf(1,0)
void vprintk(const char *s, va_list arg);

enum {
	DUMP_PREFIX_NONE,
	DUMP_PREFIX_ADDRESS,
	DUMP_PREFIX_OFFSET
};

void print_hex_dump(const char *level, const char *prefix_str, int prefix_type,
					int rowsize, int groupsize,
					const void *buf, size_t len, bool ascii);


static inline __printf(1, 2)
int no_printk(const char *fmt, ...)
{
	return 0;
}

extern /* asmlinkage */ void dump_stack(void) __cold;

#endif

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/include/linux/printk.h $ $Rev: 836935 $")
#endif
