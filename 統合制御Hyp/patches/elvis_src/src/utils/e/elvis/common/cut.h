/* The following source file contains modifications to the
 * original elvis source.
 *
 * 2009-04-29 Removed trailing ; character from __SRCVERSION macro for ansi C compliance
 * 2009-04-08 - Added __SRCVERSION macro.
 *
 * Please report issue directly to QNX. */

/* cut.h */
/* Copyright 1995 by Steve Kirkendall */


BEGIN_EXTERNC
extern BUFFER cutbuffer P_((_CHAR_ cbname, ELVBOOL create));
extern void cutyank P_((_CHAR_ cbname, MARK from, MARK to, _CHAR_ type, _CHAR_ aideeffect));
extern MARK cutput P_((_CHAR_ cbname, WINDOW win, MARK at, ELVBOOL after, ELVBOOL cretend, ELVBOOL lretend));
extern CHAR *cutmemory P_((_CHAR_ cbname));
END_EXTERNC

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/7.0.0/trunk/utils/e/elvis/common/cut.h $ $Rev: 680331 $")
#endif
