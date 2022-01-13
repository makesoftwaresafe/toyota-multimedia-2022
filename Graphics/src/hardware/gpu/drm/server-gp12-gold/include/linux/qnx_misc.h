#ifndef _QNX_MISC_H
#define _QNX_MISC_H

#include <linux/pci.h>

#ifndef FASYNC
#define FASYNC		00020000	/* fcntl, for BSD compatibility */
#endif

struct sysrq_key_op {
	void (*handler)(int);
	char *help_msg;
	char *action_msg;
	int enable_mask;
};


#define lockdep_assert_held(l)			do { (void)(l); } while (0)

static inline void console_lock(void) {}
static inline void console_unlock(void){}
static inline int console_trylock(void) {
	return 1;
}


#define AGP_NORMAL_MEMORY 0

#define AGP_USER_TYPES (1 << 16)
#define AGP_USER_MEMORY (AGP_USER_TYPES)
#define AGP_USER_CACHED_MEMORY (AGP_USER_TYPES + 1)

#endif

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/include/linux/qnx_misc.h $ $Rev: 853904 $")
#endif
