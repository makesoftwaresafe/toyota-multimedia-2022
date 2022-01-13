#include <linux/notifier.h>

int register_oom_notifier(struct notifier_block *nb)
{
	/*TODO */
	return 0;
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/qnx/oom_kill.c $ $Rev: 836322 $")
#endif
