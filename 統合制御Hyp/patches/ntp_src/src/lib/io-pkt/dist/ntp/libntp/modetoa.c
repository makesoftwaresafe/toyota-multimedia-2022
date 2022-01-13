/*
 * modetoa - return an asciized mode
 */
#include <config.h>
#include <stdio.h>

#include "lib_strbuf.h"
#include "ntp_stdlib.h"

const char *
modetoa(
	size_t mode
	)
{
	char *bp;
	static const char * const modestrings[] = {
		"unspec",
		"sym_active",
		"sym_passive",
		"client",
		"server",
		"broadcast",
		"control",
		"private",
		"bclient",
	};

	if (mode >= COUNTOF(modestrings)) {
		LIB_GETBUF(bp);
		snprintf(bp, LIB_BUFLENGTH, "mode#%zu", mode);
		return bp;
	}

	return modestrings[mode];
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/tags/7.0.0/GA/lib/io-pkt/dist/ntp/libntp/modetoa.c $ $Rev: 777476 $")
#endif
