#include "config.h"

#include "ntp_stdlib.h"

#include "unity.h"

void setUp(void);
void test_KnownMode(void);
void test_UnknownMode(void);


void
setUp(void)
{
	init_lib();

	return;
}


void
test_KnownMode(void) {
	const int MODE = 3; // Should be "client"

	TEST_ASSERT_EQUAL_STRING("client", modetoa(MODE));
}

void
test_UnknownMode(void) {
	const int MODE = 100;

	TEST_ASSERT_EQUAL_STRING("mode#100", modetoa(MODE));
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/tags/7.0.0/GA/lib/io-pkt/dist/ntp/tests/libntp/modetoa.c $ $Rev: 799952 $")
#endif
