/*
 * ymd2yd - compute the date in the year from y/m/d
 *
 * A thin wrapper around a more general calendar function.
 */

#include <config.h>
#include "ntp_stdlib.h"
#include "ntp_calendar.h"

int
ymd2yd(
	int y,
	int m,
	int d)
{
	/*
	 * convert y/m/d to elapsed calendar units, convert that to
	 * elapsed days since the start of the given year and convert
	 * back to unity-based day in year.
	 *
	 * This does no further error checking, since the underlying
	 * function is assumed to work out how to handle the data.
	 */
	return ntpcal_edate_to_yeardays(y-1, m-1, d-1) + 1;
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/tags/7.0.0/GA/lib/io-pkt/dist/ntp/libntp/ymd2yd.c $ $Rev: 777476 $")
#endif
