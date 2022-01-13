/*
 * ntpdc-layout - print layout of NTP mode 7 request/response packets
 */

#include <config.h>
#include <stdio.h>
#include <stddef.h>

#include "ntpdc.h"
#include "ntp_stdlib.h"

#if defined(IMPL_XNTPD_OLD) && IMPL_XNTPD != 3
#error Unexpected IMPL_XNTPD
#endif

int
main(void)
{
#include "nl.c"

  return (EXIT_SUCCESS);
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/tags/7.0.0/GA/lib/io-pkt/dist/ntp/ntpdc/ntpdc-layout.c $ $Rev: 777476 $")
#endif
