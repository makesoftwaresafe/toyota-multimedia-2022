/* The following source file contains modifications to the
 * original elvis source.
 *
 * 2009-04-29 Removed trailing ; character from __SRCVERSION macro for ansi C compliance
 * 2009-04-08 - Added __SRCVERSION macro.
 *
 * Please report issue directly to QNX. */

/* xtool.h */

#define MAXTOOLS	50

typedef struct
{
	Window		win;		/* toolbar subwindow */
	int		x, y;		/* position of toolbar within window */
	unsigned int	w, h;		/* overall size of the toolbar */
	ELVBOOL		recolored;	/* have colors been changed lately? */
	struct
	{
		int	x, y;		/* position of a button within toolbar*/
		int	bevel;		/* bevel height of a button */
	}		state[MAXTOOLS];/* info about each button */
} X_TOOLBAR;

CHAR *x_tb_dump P_((char *label));
void x_tb_predict P_((X11WIN *xw, unsigned w, unsigned h));
void x_tb_create P_((X11WIN *xw, int x, int y));
void x_tb_destroy P_((X11WIN *xw));
void x_tb_draw P_((X11WIN *xw, ELVBOOL fromscratch));
void x_tb_event P_((X11WIN *xw, XEvent *event));
ELVBOOL x_tb_config P_((ELVBOOL gap, char *label, _char_ op, char *value));
void x_tb_recolor P_((X11WIN *xw));

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/7.0.0/trunk/utils/e/elvis/elvis/xtool.h $ $Rev: 680331 $")
#endif
