/* strerror -- return a string corresponding to an error number.
   This is a quickie version only intended as compatability glue
   for systems which predate the ANSI C definition of the function;
   the glibc version is recommended for more general use.

   Copyright (C) 1998 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the
   Free Software Foundation; either version 2, or (at your option) any
   later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.  */

#include "config.h"

#ifndef HAVE_STRERROR

# ifndef BOOTSTRAP
#  include <stdio.h>
# endif
# ifdef HAVE_STRING_H
#  include <string.h>
# endif
# include <errno.h>
# undef strerror

extern int sys_nerr;
extern char *sys_errlist[];

char *
strerror(e)
  int e;
{
  static char unknown_string[] =
    "Unknown error code #xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx";

  if (0<=e && e<sys_nerr)
    return sys_errlist[e];
  sprintf(unknown_string+20, "%d", e);
  return unknown_string;
}

#endif /* !HAVE_STRERROR */

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.6.0/trunk/utils/s/sed/lib/strerror.c $ $Rev: 680331 $")
#endif
