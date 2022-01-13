/* Prints the system dependent name for the current locale's codeset. */

#define _XOPEN_SOURCE 500  /* Needed on AIX 3.2.5 */

#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
#include <langinfo.h>

int main ()
{
  setlocale(LC_ALL, "");
  printf("%s\n", nl_langinfo(CODESET));
  exit(0);
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/7.0.0/trunk/lib/libiconv/dist/libiconv-1.12/libcharset/tools/locale_codeset.c $ $Rev: 680336 $")
#endif
