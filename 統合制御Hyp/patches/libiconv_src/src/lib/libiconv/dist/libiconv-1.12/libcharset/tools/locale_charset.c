/* Prints the portable name for the current locale's charset. */

#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
#include "localcharset.h"

int main ()
{
  setlocale(LC_ALL, "");
  printf("%s\n", locale_charset());
  exit(0);
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/7.0.0/trunk/lib/libiconv/dist/libiconv-1.12/libcharset/tools/locale_charset.c $ $Rev: 680336 $")
#endif
