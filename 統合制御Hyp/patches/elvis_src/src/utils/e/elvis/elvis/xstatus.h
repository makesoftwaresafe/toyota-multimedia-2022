/* The following source file contains modifications to the
 * original elvis source.
 *
 * 2009-04-29 Removed trailing ; character from __SRCVERSION macro for ansi C compliance
 * 2009-04-08 - Added __SRCVERSION macro.
 *
 * Please report issue directly to QNX. */

/* xstatus.h */

#define MAXTOOLS	50

typedef struct
{
	Window		win;		/* toolbar subwindow */
	int		x, y;		/* position of status within window */
	unsigned int	w, h;		/* size of the status bar */
	char		info[80];	/* info string */
	long		line, column;	/* cursor position */
	char		learn;		/* learn character */
	char		*mode;		/* e.g., "Command" or "Input" */
	unsigned int	rulerwidth;	/* widest ruler ever seen */
	unsigned int	learnwidth;	/* widest learn ever seen */
	unsigned int	modewidth;	/* widest mode ever seen */
	int		rulerpos;
	int		learnpos;
	int		modepos;
	ELVBOOL		recolored;	/* have colors changed lately? */
} X_STATUSBAR;

void x_st_predict P_((X11WIN *xw, unsigned w, unsigned h));
void x_st_create P_((X11WIN *xw, int x, int y));
void x_st_destroy P_((X11WIN *xw));
void x_st_status P_((X11WIN *xw, CHAR *info, long line, long column, _char_ learn, char *mode));
void x_st_info P_((X11WIN *xw, CHAR *info));
void x_st_event P_((X11WIN *xw, XEvent *event));
void x_st_recolor P_((X11WIN *xw));

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/7.0.0/trunk/utils/e/elvis/elvis/xstatus.h $ $Rev: 680331 $")
#endif
