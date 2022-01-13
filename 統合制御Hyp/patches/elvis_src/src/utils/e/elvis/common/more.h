/* The following source file contains modifications to the
 * original elvis source.
 *
 * 2009-04-29 Removed trailing ; character from __SRCVERSION macro for ansi C compliance
 * 2009-04-08 - Added __SRCVERSION macro.
 *
 * Please report issue directly to QNX. */

/* more.h */
/* Copyright 1995 by Steve Kirkendall */


BEGIN_EXTERNC
extern ELVBOOL morehit;
extern void morepush P_((WINDOW win, _CHAR_ special));
END_EXTERNC

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/7.0.0/trunk/utils/e/elvis/common/more.h $ $Rev: 680331 $")
#endif
