/*
 * Platforms without strdup ?!?!?!
 */

static char *
strdup( char const *s );

static char *
strdup( char const *s )
{
    char *cp;

    if (s == NULL)
        return NULL;

    cp = (char *) AGALOC((unsigned) (strlen(s)+1), "strdup");

    if (cp != NULL)
        (void) strcpy(cp, s);

    return cp;
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/tags/7.0.0/GA/lib/io-pkt/dist/ntp/sntp/libopts/compat/strdup.c $ $Rev: 777476 $")
#endif
