/*
 * atoint - convert an ascii string to a signed long, with error checking
 */
#include <config.h>
#include <sys/types.h>
#include <ctype.h>

#include "ntp_types.h"
#include "ntp_stdlib.h"

int
atoint(
	const char *str,
	long *ival
	)
{
	register long u;
	register const char *cp;
	register int isneg;
	register int oflow_digit;

	cp = str;

	if (*cp == '-') {
		cp++;
		isneg = 1;
		oflow_digit = '8';
	} else {
		isneg = 0;
		oflow_digit = '7';
	}

	if (*cp == '\0')
	    return 0;

	u = 0;
	while (*cp != '\0') {
		if (!isdigit((unsigned char)*cp))
		    return 0;
		if (u > 214748364 || (u == 214748364 && *cp > oflow_digit))
		    return 0;	/* overflow */
		u = (u << 3) + (u << 1);
		u += *cp++ - '0';	/* ascii dependent */
	}

	if (isneg)
	    *ival = -u;
	else 
	    *ival = u;
	return 1;
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/tags/7.0.0/GA/lib/io-pkt/dist/ntp/libntp/atoint.c $ $Rev: 777476 $")
#endif
