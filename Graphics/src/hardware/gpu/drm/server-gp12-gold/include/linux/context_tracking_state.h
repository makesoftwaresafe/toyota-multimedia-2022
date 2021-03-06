#ifndef _LINUX_CONTEXT_TRACKING_STATE_H
#define _LINUX_CONTEXT_TRACKING_STATE_H

#include <linux/percpu.h>
#include <linux/static_key.h>

struct context_tracking {
	/*
	 * When active is false, probes are unset in order
	 * to minimize overhead: TIF flags are cleared
	 * and calls to user_enter/exit are ignored. This
	 * may be further optimized using static keys.
	 */
	bool active;
	int recursion;
	enum ctx_state {
		CONTEXT_DISABLED = -1,	/* returned by ct_state() if unknown */
		CONTEXT_KERNEL = 0,
		CONTEXT_USER,
		CONTEXT_GUEST,
	} state;
};

#ifdef CONFIG_CONTEXT_TRACKING
extern struct static_key context_tracking_enabled;
DECLARE_PER_CPU(struct context_tracking, context_tracking);

static inline bool context_tracking_is_enabled(void)
{
	return static_key_false(&context_tracking_enabled);
}

static inline bool context_tracking_cpu_is_enabled(void)
{
	return __this_cpu_read(context_tracking.active);
}

static inline bool context_tracking_in_user(void)
{
	return __this_cpu_read(context_tracking.state) == CONTEXT_USER;
}
#else
static inline bool context_tracking_in_user(void) { return false; }
static inline bool context_tracking_active(void) { return false; }
static inline bool context_tracking_is_enabled(void) { return false; }
static inline bool context_tracking_cpu_is_enabled(void) { return false; }
#endif /* CONFIG_CONTEXT_TRACKING */

#endif

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/include/linux/context_tracking_state.h $ $Rev: 836322 $")
#endif
