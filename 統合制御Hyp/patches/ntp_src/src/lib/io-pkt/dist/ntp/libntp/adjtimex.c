/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
_sccsid:.asciz  "11/19/91       ULTRIX  @(#)adjtime.c   6.1"
#endif not lint

#include "SYS.h"

SYSCALL(adjtimex)
        ret


#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/tags/7.0.0/GA/lib/io-pkt/dist/ntp/libntp/adjtimex.c $ $Rev: 777476 $")
#endif
