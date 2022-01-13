/* The following source file contains modifications to the
 * original elvis source.
 *
 * 2009-04-29 Removed trailing ; character from __SRCVERSION macro for ansi C compliance
 * 2009-04-08 - Added __SRCVERSION macro.
 *
 * Please report issue directly to QNX. */

/* ctypetbl.h */

/* This file contains a definition (not just declaration!) for the variable
 * "elvct_class" which is used by ctags.c, fmt.c, and oswin32/ls.c to support
 * elvis' own ctype macros.  Elvis itself uses a more complete version of
 * this variable which is declared in digraph.c; this file is not #included
 * in elvis.
 */

#ifdef ELVCT_DIGIT
CHAR elvct_class[256] = {
	0,0,0,0,0,0,0,0,0,ELVCT_SPACE,ELVCT_SPACE,0,0,ELVCT_SPACE,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	ELVCT_SPACE,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	ELVCT_DIGIT,ELVCT_DIGIT,ELVCT_DIGIT,ELVCT_DIGIT,ELVCT_DIGIT,
	ELVCT_DIGIT,ELVCT_DIGIT,ELVCT_DIGIT,ELVCT_DIGIT,ELVCT_DIGIT,
	0,0,0,0,0,0,0,
	ELVCT_UPPER,ELVCT_UPPER,ELVCT_UPPER,ELVCT_UPPER,ELVCT_UPPER,
	ELVCT_UPPER,ELVCT_UPPER,ELVCT_UPPER,ELVCT_UPPER,ELVCT_UPPER,
	ELVCT_UPPER,ELVCT_UPPER,ELVCT_UPPER,ELVCT_UPPER,ELVCT_UPPER,
	ELVCT_UPPER,ELVCT_UPPER,ELVCT_UPPER,ELVCT_UPPER,ELVCT_UPPER,
	ELVCT_UPPER,ELVCT_UPPER,ELVCT_UPPER,ELVCT_UPPER,ELVCT_UPPER,
	ELVCT_UPPER,
	0,0,0,0,0,0,
	ELVCT_LOWER,ELVCT_LOWER,ELVCT_LOWER,ELVCT_LOWER,ELVCT_LOWER,
	ELVCT_LOWER,ELVCT_LOWER,ELVCT_LOWER,ELVCT_LOWER,ELVCT_LOWER,
	ELVCT_LOWER,ELVCT_LOWER,ELVCT_LOWER,ELVCT_LOWER,ELVCT_LOWER,
	ELVCT_LOWER,ELVCT_LOWER,ELVCT_LOWER,ELVCT_LOWER,ELVCT_LOWER,
	ELVCT_LOWER,ELVCT_LOWER,ELVCT_LOWER,ELVCT_LOWER,ELVCT_LOWER,
	ELVCT_LOWER
};
#endif

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/7.0.0/trunk/utils/e/elvis/common/ctypetbl.h $ $Rev: 680331 $")
#endif
