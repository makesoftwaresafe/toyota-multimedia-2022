/* The following source file contains modifications to the
 * original elvis source.
 *
 * 2009-04-29 Removed trailing ; character from __SRCVERSION macro for ansi C compliance
 * 2009-04-08 - Added __SRCVERSION macro.
 *
 * Please report issue directly to QNX. */

/* descr.h */

/* Copyright 2000 by Steve Kirkendall.  Redistributable under the terms of
 * the Perl "Clarified Artistic" license.
 */

#ifdef FEATURE_CACHEDESC
void *descr_recall P_((WINDOW win, char *dfile));
#endif
ELVBOOL descr_open P_((WINDOW win, char *dfile));
CHAR **descr_line P_((void));
void descr_close P_((void *descr));
CHAR *descr_known P_((char *filename, char *dfile));
#ifdef DISPLAY_SYNTAX
ELVBOOL descr_cancall P_((CHAR *filename));
#endif

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/7.0.0/trunk/utils/e/elvis/common/descr.h $ $Rev: 680331 $")
#endif
