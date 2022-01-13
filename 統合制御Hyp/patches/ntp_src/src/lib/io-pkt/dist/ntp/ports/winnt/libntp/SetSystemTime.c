
#include <config.h>

#include "clockstuff.h"
#include "ntp_stdlib.h"
#include "ntp_unixtime.h"

pset_tod_using		set_tod_using = NULL;

int
ntp_set_tod(
	struct timeval *tv,
	void *tzp
	)
{
	SYSTEMTIME st;
	union {
		FILETIME ft;
		ULONGLONG ull;
	} t;

	UNUSED_ARG(tzp);

	t.ull = FILETIME_1970 +
		(ULONGLONG)tv->tv_sec * 10 * 1000 * 1000 +
		(ULONGLONG)tv->tv_usec * 10;

	if (!FileTimeToSystemTime(&t.ft, &st) || !SetSystemTime(&st)) {
		msyslog(LOG_ERR, "SetSystemTime failed: %m");
		return -1;
	}

	return 0;
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/tags/7.0.0/GA/lib/io-pkt/dist/ntp/ports/winnt/libntp/SetSystemTime.c $ $Rev: 777476 $")
#endif
