#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "ntp_syslog.h"
#include "ntp_stdlib.h"
#include "ntp_unixtime.h"
#include "clockstuff.h"

int
gettimeofday(
	struct timeval *	tv,
	void *			ignored
	)
{
	struct timespec ts;

	UNUSED_ARG(ignored);

	getclock(TIMEOFDAY, &ts);

	tv->tv_sec = ts.tv_sec;
	tv->tv_usec = ts.tv_nsec / 10;

	return 0;
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/tags/7.0.0/GA/lib/io-pkt/dist/ntp/ports/winnt/libntp/util_clockstuff.c $ $Rev: 777476 $")
#endif
