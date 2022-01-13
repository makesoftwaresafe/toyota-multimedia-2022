/* The following source file contains modifications to the
 * original elvis source.
 *
 * 2009-04-29 Removed trailing ; character from __SRCVERSION macro for ansi C compliance
 * 2009-04-08 - Added __SRCVERSION macro.
 *
 * Please report issue directly to QNX. */

/* xclip.h */

void	x_clipevent P_((XEvent *event));
ELVBOOL x_clipopen P_((ELVBOOL forwrite));
int	x_clipwrite P_((CHAR *text, int len));
int	x_clipread P_((CHAR *text, int len));
void	x_clipclose P_((void));

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/7.0.0/trunk/utils/e/elvis/elvis/xclip.h $ $Rev: 680331 $")
#endif
