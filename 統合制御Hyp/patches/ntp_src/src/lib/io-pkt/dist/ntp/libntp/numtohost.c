/*
 * numtohost - convert network number to host name.
 */
#include <config.h>

#include <sys/types.h>
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>		/* ntohl */
#endif

#include "ntp_fp.h"
#include "ntp_stdlib.h"
#include "lib_strbuf.h"

#define	LOOPBACKNET	0x7f000000
#define	LOOPBACKHOST	0x7f000001
#define	LOOPBACKNETMASK	0xff000000

char *
numtohost(
	u_int32 netnum
	)
{
	char *bp;
	struct hostent *hp;

	/*
	 * This is really gross, but saves lots of hanging looking for
	 * hostnames for the radio clocks.  Don't bother looking up
	 * addresses on the loopback network except for the loopback
	 * host itself.
	 */
	if ((((ntohl(netnum) & LOOPBACKNETMASK) == LOOPBACKNET)
	     && (ntohl(netnum) != LOOPBACKHOST))
	    || ((hp = gethostbyaddr((char *)&netnum, sizeof netnum, AF_INET))
		== 0))
	    return numtoa(netnum);
	
	LIB_GETBUF(bp);
	strlcpy(bp, hp->h_name, LIB_BUFLENGTH);

	return bp;
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/tags/7.0.0/GA/lib/io-pkt/dist/ntp/libntp/numtohost.c $ $Rev: 777476 $")
#endif
