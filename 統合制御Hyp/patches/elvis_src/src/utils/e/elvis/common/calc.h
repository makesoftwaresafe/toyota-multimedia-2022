/* The following source file contains modifications to the
 * original elvis source.
 *
 * 2009-04-29 Removed trailing ; character from __SRCVERSION macro for ansi C compliance
 * 2009-04-08 - Added __SRCVERSION macro.
 *
 * Please report issue directly to QNX. */

/* calc.h */
/* Copyright 1995 by Steve Kirkendall */

/* This is used for storing information about subscripts */
typedef struct
{
	CHAR	*ptr;	/* start of a chunk of text */
	int	len;	/* length of the chunk */
} CHUNK;

typedef enum {CALC_DOLLAR=1, CALC_PAREN=2, CALC_MSG=3, CALC_OUTER=4, CALC_ALL=7} CALCRULE;

BEGIN_EXTERNC
extern ELVBOOL calcnumber P_((CHAR *value));
extern ELVBOOL calctrue P_((CHAR *value));
extern CHAR *calculate P_((CHAR *expr, CHAR **arg, CALCRULE rule));
#ifdef FEATURE_CALC
# ifdef FEATURE_ARRAY
extern _CHAR_ calcsubscript P_((CHAR *array, CHAR *sub, int max, CHUNK *chunks));
# endif
extern ELVBOOL calcbase10 P_((CHAR *value));
extern ELVBOOL calcsel P_((MARK from, MARK to));
extern CHAR *calcelement P_((CHAR *set, CHAR *element));
extern CHAR *calcset P_((CHAR *left, _CHAR_ oper, CHAR *right));
#endif
END_EXTERNC

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/7.0.0/trunk/utils/e/elvis/common/calc.h $ $Rev: 680331 $")
#endif
