/*
 * uglydate - convert a time stamp to something barely readable
 *	      The string returned is 37 characters long.
 */
#include <config.h>
#include <stdio.h>

#include "ntp_fp.h"
#include "ntp_unixtime.h"
#include "lib_strbuf.h"
#include "ntp_stdlib.h"


char *
uglydate(
	l_fp *ts
	)
{
	char *bp;
	char *timep;
	struct tm *tm;
	time_t sec;
	long msec;
	int year;

	timep = ulfptoa(ts, 6);		/* returns max 17 characters */
	LIB_GETBUF(bp);
	sec = ts->l_ui - JAN_1970;
	msec = ts->l_uf / 4294967;	/* fract / (2**32/1000) */
	tm = gmtime(&sec);
	if (ts->l_ui == 0) {
		/*
		 * Probably not a real good thing to do.  Oh, well.
		 */
		year = 0;
		tm->tm_yday = 0;
		tm->tm_hour = 0;
		tm->tm_min = 0;
		tm->tm_sec = 0;
	} else {
		year = tm->tm_year;
		while (year >= 100)
		    year -= 100;
	}
	snprintf(bp, LIB_BUFLENGTH,
		 "%17s %02d:%03d:%02d:%02d:%02d.%03ld", timep, year,
		 tm->tm_yday, tm->tm_hour, tm->tm_min, tm->tm_sec,
		 msec);

	return bp;
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/tags/7.0.0/GA/lib/io-pkt/dist/ntp/libntp/uglydate.c $ $Rev: 777476 $")
#endif
