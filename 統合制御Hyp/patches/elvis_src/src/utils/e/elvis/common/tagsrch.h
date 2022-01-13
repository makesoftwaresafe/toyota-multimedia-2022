/* The following source file contains modifications to the
 * original elvis source.
 *
 * 2009-04-29 Removed trailing ; character from __SRCVERSION macro for ansi C compliance
 * 2009-04-08 - Added __SRCVERSION macro.
 *
 * Please report issue directly to QNX. */

/* tagsrch.h */

BEGIN_EXTERNC

void tsreset P_((void));
void tsparse P_((char *text));
void tsadjust P_((TAG *tag, _char_ oper));
void tsfile P_((char *filename, long maxlength));

END_EXTERNC

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/7.0.0/trunk/utils/e/elvis/common/tagsrch.h $ $Rev: 680331 $")
#endif
