/* The following source file contains modifications to the
 * original elvis source.
 *
 * 2009-04-29 Removed trailing ; character from __SRCVERSION macro for ansi C compliance
 * 2009-04-08 - Added __SRCVERSION macro.
 *
 * Please report issue directly to QNX. */

/* draw2.h */
/* Copyright 1995 by Steve Kirkendall */


#define drawscratch(di)	((di)->mustredraw = True)

BEGIN_EXTERNC
extern DRAWINFO *drawalloc P_((int rows, int columns, MARK top));
extern void drawfree P_((DRAWINFO *di));
extern void drawimage P_((WINDOW win));
extern void drawexpose P_((WINDOW win, int top, int left, int bottom, int right));
extern void drawmsg P_((WINDOW win, MSGIMP imp, CHAR *verbose, int len));
extern void drawex P_((WINDOW win, CHAR *text, int len));
extern void drawfinish P_((WINDOW win));
extern void drawopenedit P_((WINDOW win));
extern void drawopencomplete P_((WINDOW win));
extern void drawextext P_((WINDOW win, CHAR *text, int len));
extern void drawexlist P_((WINDOW win, CHAR *text, int len));
END_EXTERNC

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/7.0.0/trunk/utils/e/elvis/common/draw2.h $ $Rev: 680331 $")
#endif
