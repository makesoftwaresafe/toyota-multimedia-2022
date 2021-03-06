/* The following source file contains modifications to the
 * original elvis source.
 *
 * 2009-04-29 Removed trailing ; character from __SRCVERSION macro for ansi C compliance
 * 2009-04-08 - Added __SRCVERSION macro.
 *
 * Please report issue directly to QNX. */

/* move.h */
/* Copyright 1995 by Steve Kirkendall */


BEGIN_EXTERNC
extern RESULT	m_left P_((WINDOW win, VIINFO *vinf));
extern RESULT	m_right P_((WINDOW win, VIINFO *vinf));
extern RESULT	m_updown P_((WINDOW win, VIINFO *vinf));
extern RESULT	m_front P_((WINDOW win, VIINFO *vinf));
extern RESULT	m_absolute P_((WINDOW win, VIINFO *vinf));
extern RESULT	m_mark P_((WINDOW win, VIINFO *vinf));
extern RESULT	m_bigword P_((WINDOW win, VIINFO *vinf));
extern RESULT	m_word P_((WINDOW win, VIINFO *vinf));
extern RESULT	m_scroll P_((WINDOW win, VIINFO *vinf));
extern RESULT	m_column P_((WINDOW win, VIINFO *vinf));
extern RESULT	m_csearch P_((WINDOW win, VIINFO *vinf));
extern RESULT	m_tag P_((WINDOW win, VIINFO *vinf));
extern RESULT	m_bsection P_((WINDOW win, VIINFO *vinf));
extern RESULT	m_fsection P_((WINDOW win, VIINFO *vinf));
extern RESULT	m_scrnrel P_((WINDOW win, VIINFO *vinf));
extern RESULT	m_z P_((WINDOW win, VIINFO *vinf));
extern RESULT	m_fsentence P_((WINDOW win, VIINFO *vinf));
extern RESULT	m_bsentence P_((WINDOW win, VIINFO *vinf));
extern RESULT	m_spell P_((WINDOW win, VIINFO *vinf));
extern RESULT	m_endspell P_((WINDOW win, VIINFO *vinf));
extern RESULT	m_g P_((WINDOW win, VIINFO *vinf));
extern RESULT	m_gupdown P_((WINDOW win, VIINFO *vinf));

/* in search.c */
extern RESULT	m_search P_((WINDOW win, VIINFO *vinf));
extern char	searchfont;
extern regexp	*searchre;
extern ELVBOOL	searchforward;
extern ELVBOOL	searchhasdelta;
extern long	searchdelta;
extern void	searchhighlight P_((WINDOW win, int linesshown, long end));
END_EXTERNC

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/7.0.0/trunk/utils/e/elvis/common/move.h $ $Rev: 680331 $")
#endif
