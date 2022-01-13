/* The following source file contains modifications to the
 * original elvis source.
 *
 * 2009-04-29 Removed trailing ; character from __SRCVERSION macro for ansi C compliance
 * 2009-04-08 - Added __SRCVERSION macro.
 *
 * Please report issue directly to QNX. */


/* region.h */

typedef struct region_s
{
	struct region_s *next;
	MARK	from, to;	/* endpoints of region */
	CHAR	*comment;	/* comment */
	char	font;		/* face code */
} region_t;

void regionadd P_((MARK from, MARK to, _char_ font, CHAR *comment));
void regiondel P_((MARK from, MARK to, _char_ font));
region_t *regionfind P_((MARK mark));
void regionundo P_((BUFFER buf, struct umark_s *keep));

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/7.0.0/trunk/utils/e/elvis/common/region.h $ $Rev: 680331 $")
#endif
