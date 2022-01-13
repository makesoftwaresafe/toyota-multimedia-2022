/* Test re_compile_pattern error messages.  */

#include <stdio.h>
#include <string.h>
#include <regex.h>

int
main (void)
{
  struct re_pattern_buffer re;
  const char *s;
  int ret = 0;

  re_set_syntax (RE_SYNTAX_POSIX_EGREP);
  memset (&re, 0, sizeof (re));
  s = re_compile_pattern ("[[.invalid_collating_symbol.]]", 30, &re);
  if (s == NULL || strcmp (s, "Invalid collation character"))
    {
      printf ("re_compile_pattern returned %s\n", s);
      ret = 1;
    }
  s = re_compile_pattern ("[[=invalid_equivalence_class=]]", 31, &re);
  if (s == NULL || strcmp (s, "Invalid collation character"))
    {
      printf ("re_compile_pattern returned %s\n", s);
      ret = 1;
    }
  s = re_compile_pattern ("[[:invalid_character_class:]]", 29, &re);
  if (s == NULL || strcmp (s, "Invalid character class name"))
    {
      printf ("re_compile_pattern returned %s\n", s);
      ret = 1;
    }
  return ret;
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.6.0/trunk/utils/s/sed/testsuite/bug-regex16.c $ $Rev: 680331 $")
#endif
