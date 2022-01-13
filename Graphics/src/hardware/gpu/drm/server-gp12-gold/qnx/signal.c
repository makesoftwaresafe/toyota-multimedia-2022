#include <linux/types.h>
#include <linux/compiler.h>
#include <signal.h>
#include <linux/debug.h>
#include <linux/sched.h>

int send_sig(int sig, struct task_struct *p, int priv)
{
	(void)priv;
	kill(p->pid, sig);

	return 0;
}

void block_all_signals(int (*notifier)(void *priv), void *priv, sigset_t *mask)
{
	(void)notifier;
	(void)priv;
	(void)mask;
	BUG();
}

void unblock_all_signals(void)
{
	BUG();
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/qnx/signal.c $ $Rev: 836935 $")
#endif
