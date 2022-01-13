/* The following source file contains modifications to the
 * original elvis source.
 *
 * 2009-04-29 Removed trailing ; character from __SRCVERSION macro for ansi C compliance
 * 2009-04-08 - Added __SRCVERSION macro.
 *
 * Please report issue directly to QNX. */

/* vicmd.h */
/* Copyright 1995 by Steve Kirkendall */


BEGIN_EXTERNC
extern RESULT v_ascii	P_((WINDOW win, VIINFO *vinf));
extern RESULT v_at	P_((WINDOW win, VIINFO *vinf));
extern RESULT v_delchar	P_((WINDOW win, VIINFO *vinf));
extern RESULT v_ex	P_((WINDOW win, VIINFO *vinf));
extern RESULT v_expose	P_((WINDOW win, VIINFO *vinf));
extern RESULT v_input	P_((WINDOW win, VIINFO *vinf));
extern RESULT v_notex	P_((WINDOW win, VIINFO *vinf));
extern RESULT v_notop	P_((WINDOW win, VIINFO *vinf));
extern RESULT v_number	P_((WINDOW win, VIINFO *vinf));
extern RESULT v_paste	P_((WINDOW win, VIINFO *vinf));
extern RESULT v_quit	P_((WINDOW win, VIINFO *vinf));
extern RESULT v_setmark	P_((WINDOW win, VIINFO *vinf));
extern RESULT v_tag	P_((WINDOW win, VIINFO *vinf));
extern RESULT v_undo	P_((WINDOW win, VIINFO *vinf));
extern RESULT v_visible	P_((WINDOW win, VIINFO *vinf));
extern RESULT v_window	P_((WINDOW win, VIINFO *vinf));
END_EXTERNC

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/7.0.0/trunk/utils/e/elvis/common/vicmd.h $ $Rev: 680331 $")
#endif
