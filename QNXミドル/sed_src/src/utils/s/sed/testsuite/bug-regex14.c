/* Tests re_comp and re_exec.
   Copyright (C) 2002 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Isamu Hasegawa <isamu@yamato.ibm.com>, 2002.

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

#define _REGEX_RE_COMP
#include <sys/types.h>
#ifdef HAVE_MCHECK_H
#include <mcheck.h>
#endif
#include <regex.h>
#include <stdio.h>
#include <stdlib.h>

int
main (void)
{
  const char *err;
  size_t i;
  int ret = 0;

#ifdef HAVE_MCHECK_H
  mtrace ();
#endif

  for (i = 0; i < 100; ++i)
    {
      err = re_comp ("a t.st");
      if (err)
	{
	  printf ("re_comp failed: %s\n", err);
	  ret = 1;
	}

      if (! re_exec ("This is a test."))
	{
	  printf ("re_exec failed\n");
	  ret = 1;
	}
    }

  return ret;
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.6.0/trunk/utils/s/sed/testsuite/bug-regex14.c $ $Rev: 680331 $")
#endif
