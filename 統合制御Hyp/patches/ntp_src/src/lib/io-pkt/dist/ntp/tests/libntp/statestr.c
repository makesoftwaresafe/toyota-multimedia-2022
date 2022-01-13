#include "config.h"

#include "ntp_stdlib.h"
#include "ntp.h" // needed for MAX_MAC_LEN used in ntp_control.h
#include "ntp_control.h"

#include "unity.h"

void setUp(void);
void test_PeerRestart(void);
void test_SysUnspecified(void);
void test_ClockCodeExists(void);
void test_ClockCodeUnknown(void);


void
setUp(void)
{
	init_lib();

	return;
}


// eventstr()
void
test_PeerRestart(void) {
	TEST_ASSERT_EQUAL_STRING("restart", eventstr(PEVNT_RESTART));
}


void
test_SysUnspecified(void) {
	TEST_ASSERT_EQUAL_STRING("unspecified", eventstr(EVNT_UNSPEC));
}


// ceventstr()
void
test_ClockCodeExists(void) {
	TEST_ASSERT_EQUAL_STRING("clk_unspec", ceventstr(CTL_CLK_OKAY));
}


void
test_ClockCodeUnknown(void) {
	TEST_ASSERT_EQUAL_STRING("clk_-1", ceventstr(-1));
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/tags/7.0.0/GA/lib/io-pkt/dist/ntp/tests/libntp/statestr.c $ $Rev: 799952 $")
#endif
