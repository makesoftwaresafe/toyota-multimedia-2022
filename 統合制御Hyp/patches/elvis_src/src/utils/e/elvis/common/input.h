/* The following source file contains modifications to the
 * original elvis source.
 *
 * 2009-04-29 Removed trailing ; character from __SRCVERSION macro for ansi C compliance
 * 2009-04-08 - Added __SRCVERSION macro.
 *
 * Please report issue directly to QNX. */

/* input.h */
/* Copyright 1995 by Steve Kirkendall */


BEGIN_EXTERNC
extern void inputpush P_((WINDOW win, ELVISSTATE flags, _char_ mode));
extern void inputtoggle P_((WINDOW win, _char_ mode));
extern void inputchange P_((WINDOW win, MARK from, MARK to, ELVBOOL linemd));
extern void inputbeforeenter P_((WINDOW win));
END_EXTERNC

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/7.0.0/trunk/utils/e/elvis/common/input.h $ $Rev: 680331 $")
#endif
