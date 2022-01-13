/*
 * numtoa - return asciized network numbers store in local array space
 */
#include <config.h>

#include <sys/types.h>
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>		/* ntohl */
#endif

#include <stdio.h>

#include "ntp_fp.h"
#include "lib_strbuf.h"
#include "ntp_stdlib.h"

char *
numtoa(
	u_int32 num
	)
{
	register u_int32 netnum;
	register char *buf;

	netnum = ntohl(num);
	LIB_GETBUF(buf);
	snprintf(buf, LIB_BUFLENGTH, "%lu.%lu.%lu.%lu",
		 ((u_long)netnum >> 24) & 0xff,
		 ((u_long)netnum >> 16) & 0xff,
		 ((u_long)netnum >> 8) & 0xff,
		 (u_long)netnum & 0xff);
	return buf;
}


/* Convert a refid & stratum to a string */
const char *
refid_str(
	u_int32	refid,
	int	stratum
	)
{
	char *	text;
	size_t	tlen;

	if (stratum > 1)
		return numtoa(refid);

	LIB_GETBUF(text);
	text[0] = '.';
	memcpy(&text[1], &refid, sizeof(refid));
	text[1 + sizeof(refid)] = '\0';
	tlen = strlen(text);
	text[tlen] = '.';
	text[tlen + 1] = '\0';

	return text;
}


#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/tags/7.0.0/GA/lib/io-pkt/dist/ntp/libntp/numtoa.c $ $Rev: 777476 $")
#endif
