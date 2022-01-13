#include "config.h"

#include "ntp.h"
#include "ntp_calendar.h"
#include "ntp_stdlib.h"
#include "rc_cmdlength.h"

#include "unity.h"

#include <string.h>

#include "test-libntp.h"


void
test_EvaluateCommandLength(void){
	size_t length, commandLength;
	const char *command1 = "Random Command";
	const char *command2 = "Random Command\t\t\n\t";
	const char *command3 = "Random\nCommand\t\t\n\t";
	const char *command4 = "Random Command\t\t\n\t1 2 3";
	
	length = strlen(command1);
	commandLength = remoteconfig_cmdlength(command1, command1+length);
	TEST_ASSERT_EQUAL(14, commandLength );
	
	length = strlen(command2);
	commandLength = remoteconfig_cmdlength(command2, command2+length);
	TEST_ASSERT_EQUAL(14, commandLength );

	length = strlen(command3);
	commandLength = remoteconfig_cmdlength(command3, command3+length);
	TEST_ASSERT_EQUAL(6, commandLength );
	
	length = strlen(command4);
	commandLength = remoteconfig_cmdlength(command4, command4+length);
	TEST_ASSERT_EQUAL(16, commandLength );

}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/tags/7.0.0/GA/lib/io-pkt/dist/ntp/tests/ntpd/rc_cmdlength.c $ $Rev: 799952 $")
#endif
