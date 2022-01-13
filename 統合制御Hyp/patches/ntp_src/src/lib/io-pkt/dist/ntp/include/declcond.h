/*
 * declcond.h - declarations conditionalized for ntpd
 *
 * The NTP reference implementation distribution includes two distinct
 * declcond.h files, one in ntpd/ used only by ntpd, and another in
 * include/ used by libntp and utilities.  This relies on the source
 * file's directory being ahead of include/ in the include search.
 *
 * The ntpd variant of declcond.h declares "debug" only #ifdef DEBUG,
 * as the --disable-debugging version of ntpd should not reference
 * "debug".  The libntp and utilities variant always declares debug,
 * as it is used in those codebases even without DEBUG defined.
 */
#ifndef DECLCOND_H
#define DECLCOND_H

/* #ifdef DEBUG */		/* uncommented in ntpd/declcond.h */
extern int debug;
/* #endif */			/* uncommented in ntpd/declcond.h */

#endif	/* DECLCOND_H */

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/tags/7.0.0/GA/lib/io-pkt/dist/ntp/include/declcond.h $ $Rev: 777476 $")
#endif
