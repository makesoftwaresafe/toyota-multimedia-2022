/* The following source file contains modifications to the
 * original elvis source.
 *
 * 2009-04-29 Removed trailing ; character from __SRCVERSION macro for ansi C compliance
 * 2009-04-08 - Added __SRCVERSION macro.
 *
 * Please report issue directly to QNX. */

/* safe.h */
/* Copyright 1995 by Steve Kirkendall */


#ifndef DEBUG_ALLOC

BEGIN_EXTERNC
extern void *safealloc P_((int qty, size_t size));
extern void safefree P_((void *ptr));
extern char *safedup P_((char *str));
END_EXTERNC
# define safekept	safealloc
#define safekdup	safedup
# define safeterm()
# define safeinspect()

#else

# define safealloc(qty, size)	_safealloc(__FILE__, __LINE__, ElvFalse, qty, size)
# define safekept(qty, size)	_safealloc(__FILE__, __LINE__, ElvTrue, qty, size)
# define safefree(ptr)		_safefree(__FILE__, __LINE__, ptr)
# define safedup(str)		_safedup(__FILE__, __LINE__, ElvFalse, str)
# define safekdup(str)		_safedup(__FILE__, __LINE__, ElvTrue, str)
BEGIN_EXTERNC
extern void *_safealloc P_((char *file, int line, ELVBOOL kept, int qty, size_t size));
extern void _safefree P_((char *file, int line, void *ptr));
extern char *_safedup P_((char *file, int line, ELVBOOL kept, char *str));
extern void safeterm P_((void));
extern void safeinspect P_((void));
END_EXTERNC

#endif

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/7.0.0/trunk/utils/e/elvis/common/safe.h $ $Rev: 680331 $")
#endif
