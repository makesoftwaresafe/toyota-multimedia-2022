/* The following source file contains modifications to the
 * original elvis source.
 *
 * 2009-04-29 Removed trailing ; character from __SRCVERSION macro for ansi C compliance
 * 2009-04-08 - Added __SRCVERSION macro.
 *
 * Please report issue directly to QNX. */

/* version.h */
/* Copyright 1996-2001 by Steve Kirkendall */

/* Some other places where the version number resides:
 *	Makefile.in, search for "VERSION="
 *	README.html, search for the actual version number
 */

#define VERSION	"2.2.0"
#define COPY1 "Copyright (c) 1995-2003 by Steve Kirkendall"
#if 1
# define COPY2 "Permission is granted to redistribute the source or binaries under the terms of"
# define COPY3 "of the Perl `Clarified Artistic License', as described in the doc/license.html"
# define COPY4 "file.  In particular, unmodified versions can be freely redistributed."
# define COPY5 "Elvis is offered with no warranty, to the extent permitted by law."
#else
# define COPY2 "This version of elvis is intended to be used only by its developers"
#endif

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/7.0.0/trunk/utils/e/elvis/common/version.h $ $Rev: 680331 $")
#endif
