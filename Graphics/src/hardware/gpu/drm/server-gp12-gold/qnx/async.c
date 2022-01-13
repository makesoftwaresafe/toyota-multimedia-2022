#include <linux/qnx.h>
#include <linux/linux.h>

async_cookie_t async_schedule(async_func_t func, void *data)
{
	async_cookie_t foo = 0;
	func(data, foo);
	return foo;
}

void async_synchronize_full(void)
{
}

void async_synchronize_cookie(async_cookie_t cookie)
{
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/qnx/async.c $ $Rev: 836322 $")
#endif
