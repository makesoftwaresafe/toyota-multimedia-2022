/* Test for memory leaks in regcomp.
   Copyright (C) 2003 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Jakub Jelinek <jakub@redhat.com>, 2003.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, write to the Free
   Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
   02110-1301 USA.  */

#include "config.h"

#ifdef HAVE_MCHECK_H
#include <mcheck.h>
#endif
#include <regex.h>
#include <stdio.h>

int main (void)
{
  regex_t re;
  int i;
  int ret = 0;

#ifdef HAVE_MCHECK_H
  mtrace ();
#endif

  for (i = 0; i < 32; ++i)
    {
      if (regcomp (&re, "X-.+:.+Y=\".*\\.(A|B|C|D|E|F|G|H|I",
		   REG_EXTENDED | REG_ICASE) == 0)
	{
	  puts ("regcomp unexpectedly succeeded");
	  ret = 1;
	}
      else
	regfree (&re);
    }
  return ret;
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.6.0/trunk/utils/s/sed/testsuite/bug-regex21.c $ $Rev: 680331 $")
#endif
