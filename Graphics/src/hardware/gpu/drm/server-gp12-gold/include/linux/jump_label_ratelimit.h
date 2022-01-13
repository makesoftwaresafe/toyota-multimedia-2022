#ifndef _LINUX_JUMP_LABEL_RATELIMIT_H
#define _LINUX_JUMP_LABEL_RATELIMIT_H

#include <linux/jump_label.h>
#include <linux/workqueue.h>

#if defined(CC_HAVE_ASM_GOTO) && defined(CONFIG_JUMP_LABEL)
struct static_key_deferred {
	struct static_key key;
	unsigned long timeout;
	struct delayed_work work;
};
#endif

#ifdef HAVE_JUMP_LABEL
extern void static_key_slow_dec_deferred(struct static_key_deferred *key);
extern void
jump_label_rate_limit(struct static_key_deferred *key, unsigned long rl);

#else	/* !HAVE_JUMP_LABEL */
struct static_key_deferred {
	struct static_key  key;
};
static inline void static_key_slow_dec_deferred(struct static_key_deferred *key)
{
	STATIC_KEY_CHECK_USE();
	static_key_slow_dec(&key->key);
}
static inline void
jump_label_rate_limit(struct static_key_deferred *key,
		unsigned long rl)
{
	STATIC_KEY_CHECK_USE();
}
#endif	/* HAVE_JUMP_LABEL */
#endif	/* _LINUX_JUMP_LABEL_RATELIMIT_H */

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/include/linux/jump_label_ratelimit.h $ $Rev: 836322 $")
#endif
