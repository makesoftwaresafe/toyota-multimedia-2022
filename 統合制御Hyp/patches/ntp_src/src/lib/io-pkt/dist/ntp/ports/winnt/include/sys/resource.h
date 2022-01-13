/*
 * ports/winnt/include/sys/resource.h
 *
 * routines declared in Unix systems' sys/resource.h
 */

#define	PRIO_PROCESS	0
#define	NTP_PRIO	(-12)

int setpriority(int, int, int);		/* winnt\libntp\setpriority.c */

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/tags/7.0.0/GA/lib/io-pkt/dist/ntp/ports/winnt/include/sys/resource.h $ $Rev: 777476 $")
#endif
