#ifndef _QNX_LINUX_ASYNC_H
#define _QNX_LINUX_ASYNC_H 1

#include <linux/types.h>
#include <linux/list.h>

typedef u64 async_cookie_t;
typedef void (*async_func_t) (void *data, async_cookie_t cookie);
struct async_domain {
	struct list_head pending;
	unsigned registered:1;
};

extern async_cookie_t async_schedule(async_func_t func, void *data);
extern void async_synchronize_full(void);
extern void async_synchronize_cookie(async_cookie_t cookie);


#endif

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/include/linux/async.h $ $Rev: 836322 $")
#endif
