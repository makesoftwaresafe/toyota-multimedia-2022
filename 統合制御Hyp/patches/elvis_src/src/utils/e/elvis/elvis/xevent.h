/* The following source file contains modifications to the
 * original elvis source.
 *
 * 2009-04-29 Removed trailing ; character from __SRCVERSION macro for ansi C compliance
 * 2009-04-08 - Added __SRCVERSION macro.
 *
 * Please report issue directly to QNX. */

/* xevent.h */

extern ELVBOOL x_repeating;
extern ELVBOOL x_didcmd;

void x_ev_repeat P_((XEvent *event, long timeout));
XEvent *x_ev_wait P_((void));
void x_ev_process P_((XEvent *event));
ELVBOOL x_ev_poll P_((ELVBOOL reset));

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/7.0.0/trunk/utils/e/elvis/elvis/xevent.h $ $Rev: 680331 $")
#endif
