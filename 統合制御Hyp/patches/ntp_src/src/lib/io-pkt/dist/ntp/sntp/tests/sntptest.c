
#include "config.h"
#include "ntp_stdlib.h"
#include "sntp-opts.h"
#include "sntptest.h"

void
sntptest(void) {
	optionSaveState(&sntpOptions);
}


void
sntptest_destroy(void) {
	optionRestore(&sntpOptions);
}


void
ActivateOption(const char* option, const char* argument) {

	const int ARGV_SIZE = 4;

	char* opts[ARGV_SIZE];
	
	opts[0] = estrdup("sntpopts");
	opts[1] = estrdup(option);
	opts[2] = estrdup(argument);
	opts[3] = estrdup("127.0.0.1");

	optionProcess(&sntpOptions, ARGV_SIZE, opts);
}


#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/tags/7.0.0/GA/lib/io-pkt/dist/ntp/sntp/tests/sntptest.c $ $Rev: 799952 $")
#endif
