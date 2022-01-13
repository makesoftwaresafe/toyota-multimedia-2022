#!/bin/sh
# Run this to generate all the initial makefiles, etc.

# Generate required files
test -n "$srcdir" || srcdir=`dirname "$0"`
test -n "$srcdir" || srcdir=.
(
  cd "$srcdir" &&
  touch ChangeLog && # Required by automake.
  (test -d m4 || mkdir m4) && # Required by gtkdocize
  gtkdocize &&
  autoreconf --verbose --force --install
) || exit

# Run configure
test -n "$NOCONFIGURE" || "$srcdir/configure" "$@"
