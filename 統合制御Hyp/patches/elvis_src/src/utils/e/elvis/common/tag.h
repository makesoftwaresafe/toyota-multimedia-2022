/* The following source file contains modifications to the
 * original elvis source.
 *
 * 2009-04-29 Removed trailing ; character from __SRCVERSION macro for ansi C compliance
 * 2009-04-08 - Added __SRCVERSION macro.
 *
 * Please report issue directly to QNX. */

/* tag.h */

/* maximum number of attributes */
#define MAXATTR	10

/* The indicies of the standard fields */
#define TAGNAME	attr[0]
#define TAGFILE attr[1]
#define TAGADDR	attr[2]

/* values of a single tag */
typedef struct tag_s
{
	struct tag_s	*next;	/* next tag in sorted order, or NULL */
	struct tag_s	*bighop;/* some other tag, or NULL */
	long	match;		/* likelyhood that this is desired tag */
	char	*attr[MAXATTR];	/* other attribute values; see tagattrname[] for names */
} TAG;

#if 0
/* Stuff normally defined in elvis.h, but elvis.h might not be included */
# ifndef QTY
#  include <stdio.h>
typedef enum {ElvFalse, ElvTrue} ELVBOOL;
#  define P_(args)	args
# endif
#endif

extern char *tagattrname[MAXATTR];
extern TAG *taglist;
extern ELVBOOL tagforward;

BEGIN_EXTERNC

extern void tagnamereset P_((void));
extern TAG *tagdup P_((TAG *tag));
extern ELVBOOL tagattr P_((TAG *tag, char *name, char *value));
extern TAG *tagfree P_((TAG *tag));
extern void tagdelete P_((ELVBOOL all));
extern void tagadd P_((TAG *tag));
extern TAG *tagparse P_((char *line));

END_EXTERNC

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/7.0.0/trunk/utils/e/elvis/common/tag.h $ $Rev: 680331 $")
#endif
