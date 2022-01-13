#ifndef SNTPTEST_H
#define SNTPTEST_H

#include "ntp_stdlib.h"
#include "sntp-opts.h"

void sntptest(void);
void sntptest_destroy(void);
void ActivateOption(const char* option, const char* argument);

#endif // SNTPTEST_H

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/tags/7.0.0/GA/lib/io-pkt/dist/ntp/sntp/tests/sntptest.h $ $Rev: 799952 $")
#endif
