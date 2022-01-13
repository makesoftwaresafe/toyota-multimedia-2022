/* The following source file contains modifications to the
 * original elvis source.
 *
 * 2009-04-29 Removed trailing ; character from __SRCVERSION macro for ansi C compliance
 * 2009-04-08 - Added __SRCVERSION macro.
 *
 * Please report issue directly to QNX. */

/* xscroll.h */

typedef enum { X_SB_REDRAW, X_SB_NORMAL, X_SB_BLANK, X_SB_STOP } X_SCROLLSTATE;

typedef struct
{
	Window		win;		/* scrollbar subwindow */
	int		x, y;		/* position of scrollbar within app window */
	unsigned	w, h;		/* total size of the scrollbar */
	unsigned	width;		/* width of the page/thumb area */
	unsigned	height;		/* height of page/thumb area */
	unsigned	offset;		/* top of page/thumb area */
	unsigned	top;		/* top of thumb */
	unsigned	bottom;		/* bottom of thumb */
	ELVBOOL		recolored;	/* have colors changed lately? */
	X_SCROLLSTATE	state;		/* scrollbar state */
} X_SCROLLBAR;

void x_sb_predict P_((X11WIN *xw, unsigned w, unsigned h));
void x_sb_create P_((X11WIN *xw, int x, int y));
void x_sb_destroy P_((X11WIN *xw));
void x_sb_setstate P_((X11WIN *xw, X_SCROLLSTATE newstate));
void x_sb_thumb P_((X11WIN *xw, long top, long bottom, long total));
void x_sb_event P_((X11WIN *xw, XEvent *event));
void x_sb_recolor P_((X11WIN *xw));

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/7.0.0/trunk/utils/e/elvis/elvis/xscroll.h $ $Rev: 680331 $")
#endif
