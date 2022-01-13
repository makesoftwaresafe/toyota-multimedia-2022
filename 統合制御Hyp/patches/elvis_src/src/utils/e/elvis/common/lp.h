/* The following source file contains modifications to the
 * original elvis source.
 *
 * 2009-04-29 Removed trailing ; character from __SRCVERSION macro for ansi C compliance
 * 2009-04-08 - Added __SRCVERSION macro.
 *
 * Please report issue directly to QNX. */

/* lp.h */
/* Copyright 1995 by Steve Kirkendall */

#ifdef FEATURE_LPR

typedef struct
{
    char *name;				/* printer type, e.g. "epson" */
    int  minorno;			/* value to pass to `before' */
    ELVBOOL spooled;			/* uses lpout spooler */
    void (*before) P_((int minorno, void (*draw)(_CHAR_ ch)));/* called before print job */
    void (*fontch) P_((_char_ font, _CHAR_ ch)); /* output a single char */
    void (*page) P_((int linesleft));	/* called at end of each page */
    void (*after) P_((int linesleft));	/* called at end of print job */
} LPTYPE;

BEGIN_EXTERNC
extern unsigned char *lpfg P_((_char_ fontcode));
extern char *lpoptfield P_((char *field, char *dflt));
extern RESULT lp P_((WINDOW win, MARK top, MARK bottom, ELVBOOL force));
END_EXTERNC
extern LPTYPE lpepson, lppana, lpibm, lphp, lpdumb, lpansi, lphtml;
extern LPTYPE lpcr, lpbs;
extern LPTYPE lpps, lpps2;
#ifdef GUI_WIN32
extern LPTYPE lpwindows;
#endif

#endif /* FEATURE_LPR */

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/7.0.0/trunk/utils/e/elvis/common/lp.h $ $Rev: 680331 $")
#endif
