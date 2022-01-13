#include <config.h>

#include <ntp_assert.h>
#include "ntp_malloc.h"
#include <string.h>

#ifndef HAVE_STRDUP

char *strdup(const char *s);

char *
strdup(
	const char *s
	)
{
	size_t	octets;
	char *	cp;

	REQUIRE(s);
	octets = strlen(s) + 1;
	if ((cp = malloc(octets)) == NULL)
		return NULL;
	memcpy(cp, s, octets);

	return cp;
}
#else
int strdup_c_nonempty_compilation_unit;
#endif

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/tags/7.0.0/GA/lib/io-pkt/dist/ntp/libntp/strdup.c $ $Rev: 799952 $")
#endif
