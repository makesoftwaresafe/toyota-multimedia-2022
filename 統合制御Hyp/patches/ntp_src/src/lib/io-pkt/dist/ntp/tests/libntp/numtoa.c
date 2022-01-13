#include "config.h"

#include "ntp_stdlib.h"
#include "ntp_fp.h"

#include "unity.h"

void setUp(void);
void test_Address(void);
void test_Netmask(void);


void
setUp(void)
{
	init_lib();

	return;
}


void
test_Address(void) {
	const u_int32 input = htonl(3221225472UL + 512UL + 1UL); // 192.0.2.1

	TEST_ASSERT_EQUAL_STRING("192.0.2.1", numtoa(input));
}

void
test_Netmask(void) {
	// 255.255.255.0
	const u_int32 hostOrder = 255UL*256UL*256UL*256UL + 255UL*256UL*256UL + 255UL*256UL;
	const u_int32 input = htonl(hostOrder);

	TEST_ASSERT_EQUAL_STRING("255.255.255.0", numtoa(input));
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/tags/7.0.0/GA/lib/io-pkt/dist/ntp/tests/libntp/numtoa.c $ $Rev: 799952 $")
#endif
