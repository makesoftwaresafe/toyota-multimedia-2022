/* The following source file contains modifications to the
 * original elvis source.
 *
 * 2009-04-29 Removed trailing ; character from __SRCVERSION macro for ansi C compliance
 * 2009-04-08 - Added __SRCVERSION macro.
 *
 * Please report issue directly to QNX. */

/* need.h */
/* Copyright 1995 by Steve Kirkendall */

#ifdef NEED_ABORT
# define abort()	(*"x" = 0)
#endif

#ifdef NEED_ASSERT
# ifdef NDEBUG
#  define assert(x)
# else
#  define assert(x)	if (!(x)) abort(); else (void)(0)
# endif
#else
# include <assert.h>
#endif

#ifdef NEED_STRDUP
BEGIN_EXTERNC
extern char *strdup P_((const char *str));
END_EXTERNC
#endif

#ifdef NEED_MEMMOVE
BEGIN_EXTERNC
extern void *memmove P_((void *, const void *, size_t));
END_EXTERNC
#endif

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/7.0.0/trunk/utils/e/elvis/common/need.h $ $Rev: 680331 $")
#endif
