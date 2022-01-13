/* The following source file contains modifications to the
 * original elvis source.
 *
 * 2009-04-29 Removed trailing ; character from __SRCVERSION macro for ansi C compliance
 * 2009-04-08 - Added __SRCVERSION macro.
 *
 * Please report issue directly to QNX. */

/* digraph.c */
/* Copyright 1995 by Steve Kirkendall */

#ifndef NO_DIGRAPH
BEGIN_EXTERNC
extern CHAR digraph P_((_CHAR_ key1, _CHAR_ key2));
extern void digaction P_((WINDOW win, ELVBOOL bang, CHAR *extra));
# ifdef FEATURE_MKEXRC
extern void digsave P_((BUFFER buf));
# endif
END_EXTERNC
#endif

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/7.0.0/trunk/utils/e/elvis/common/digraph.h $ $Rev: 680331 $")
#endif
