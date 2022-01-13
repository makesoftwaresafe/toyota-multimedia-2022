/** \ingroup rpmio
 * \file rpmio/strtolocale.c
 */

#include "system.h"
#ifdef HAVE_LANGINFO_H
#include <langinfo.h>
#endif
#ifdef HAVE_ICONV
#include <iconv.h>
#endif
#include <rpmiotypes.h>
#include "debug.h"

const char * xstrtolocale(const char *str)
{
#ifdef HAVE_ICONV
    /*@only@*/
    static char *locale_encoding = NULL;
    static int locale_encoding_is_utf8 = 0;
    iconv_t cd;
    size_t src_size, dest_size;
    char *result, *dest;
    const char *src;

    if (locale_encoding == NULL) {
#ifdef HAVE_LANGINFO_H
	const char *encoding = nl_langinfo(CODESET);
#else
	const char *encoding = "char";
#endif
	locale_encoding = (char *) xmalloc(strlen(encoding) + 11);
	sprintf(locale_encoding, "%s//TRANSLIT", encoding);
	locale_encoding_is_utf8 = strcasecmp(encoding, "UTF-8") == 0;
    }

    if (!str || !*str || locale_encoding_is_utf8)
	return str;

    cd = iconv_open(locale_encoding, "UTF-8");
    if (cd == (iconv_t)-1)
	return str;

    src_size = strlen(str);
    dest_size = src_size + 1;
    result = (char *) xmalloc(dest_size);
    src = str;
    dest = result;
    for(;;) {
	size_t status = iconv(cd, (char **)&src, &src_size, &dest, &dest_size);
	if (status == (size_t)-1) {
	    size_t dest_offset;
	    if (errno != E2BIG) {
		free(result);
		(void) iconv_close(cd);
		return str;
	    }
	    dest_offset = dest - result;
	    dest_size += 16;
	    result = (char *) xrealloc(result, dest_offset + dest_size);
	    dest = result + dest_offset;
	} else if (src_size == 0) {
	    if (src == NULL) break;
	    src = NULL;
	}
    }
    (void) iconv_close(cd);
    free((void *)str);
    if (dest_size == 0) {
	size_t dest_offset = dest - result;
	result = (char *) xrealloc(result, dest_offset + 1);
	dest = result + dest_offset;
    }
    *dest = '\0';
    return result;
#else
    return str;
#endif
}
