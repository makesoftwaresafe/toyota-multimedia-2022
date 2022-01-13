/* The following source file contains modifications to the
 * original elvis source.
 *
 * 2009-04-29 Removed trailing ; character from __SRCVERSION macro for ansi C compliance
 * 2009-04-08 - Added __SRCVERSION macro.
 *
 * Please report issue directly to QNX. */

/* tagelvis.h */

extern ELVBOOL	tenewtag;

BEGIN_EXTERNC

#ifdef FEATURE_SHOWTAG
typedef struct tedef_s
{
	MARK	where;		/* where a tag is defined */
	CHAR	*label;		/* tag name, and maybe other info */
} TEDEF;
extern void tebuilddef P_((BUFFER buf));
extern void tefreedef P_((BUFFER buf));
extern CHAR *telabel P_((MARK cursor));
#endif

#ifdef DISPLAY_SYNTAX
spell_t *telibrary P_((char *tagfile, spell_t *dict, ELVBOOL ignorecase, CHAR *prefix));
#endif

#ifdef FEATURE_COMPLETE
CHAR *tagcomplete P_((WINDOW win, MARK m));
#endif

extern void tesametag P_((void));
extern TAG *tetag P_((CHAR *select));
extern BUFFER tebrowse P_((ELVBOOL all, CHAR *select));
extern void tepush P_((WINDOW win, CHAR *label));

END_EXTERNC

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/7.0.0/trunk/utils/e/elvis/common/tagelvis.h $ $Rev: 680331 $")
#endif
