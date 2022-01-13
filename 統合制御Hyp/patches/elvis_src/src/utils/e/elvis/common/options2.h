/* The following source file contains modifications to the
 * original elvis source.
 *
 * 2009-04-29 Removed trailing ; character from __SRCVERSION macro for ansi C compliance
 * 2009-04-08 - Added __SRCVERSION macro.
 *
 * Please report issue directly to QNX. */

/* options2.h */

#ifdef FEATURE_MKEXRC
BEGIN_EXTERNC
extern void optsave P_((BUFFER custom));
END_EXTERNC
#endif

#ifdef FEATURE_COMPLETE
BEGIN_EXTERNC
extern CHAR *optcomplete P_((WINDOW win, MARK m));
END_EXTERNC
#endif

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/7.0.0/trunk/utils/e/elvis/common/options2.h $ $Rev: 680331 $")
#endif
