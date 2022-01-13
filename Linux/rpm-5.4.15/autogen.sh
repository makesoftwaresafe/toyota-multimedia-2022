#!/bin/sh

#   configure the requirements
AMV="automake (GNU automake) 1.14.1"
ACV="autoconf (GNU Autoconf) 2.69"
LTV="libtoolize (GNU libtool) 2.4.2"
GTT="gettextize (GNU gettext-tools) 0.19.2"
USAGE="
To build RPM from plain CVS sources the following
installed developer tools are mandatory:
    $AMV
    $ACV
    $LTV
    $GTT
"

#   wrapper for running GNU libtool's libtoolize(1)
libtoolize () {
    _libtoolize=`which glibtoolize 2>/dev/null`
    _libtoolize_args="$*"
    case "$_libtoolize" in
        /* ) ;;
        *  ) _libtoolize=`which libtoolize 2>/dev/null`
             case "$_libtoolize" in
                 /* ) ;;
                 *  ) _libtoolize="libtoolize" ;;
             esac
             ;;
    esac
    _libtoolize_version="`$_libtoolize --version | sed -e '1q' | sed -e 's;^[^0-9]*;;'`"
    case "$_libtoolize_version" in
        1.* ) _libtoolize_args=`echo "X$_libtoolize_args" | sed -e 's;^X;;' -e 's;--quiet;;' -e 's;--install;;'` ;;
    esac
    eval $_libtoolize $_libtoolize_args
}

#   requirements sanity check
[ "`automake   --version | head -1`" != "$AMV" ] && echo "$USAGE" # && exit 1
[ "`autoconf   --version | head -1`" != "$ACV" ] && echo "$USAGE" # && exit 1
[ "`libtoolize --version | head -1`" != "$LTV" ] && echo "$USAGE" # && exit 1
[ "`gettextize --version | head -1 | sed -e 's;^.*/\\(gettextize\\);\\1;'`" != "$GTT" ] && echo "$USAGE" # && exit 1

for dir in bash beecrypt file gpsee libtpm libgit2 neon pcre popt rc syck xar xz; do

  [ -d $dir ] || continue
  echo "===> $dir"
  case $dir in
  libgit2)
    ( cd $dir && cmake . && 
	echo "check: test" >> Makefile &&
	echo "distclean: clean" >> Makefile
    )
    ;;
  *)
    ( cd $dir && sh ./autogen.sh --noconfigure "$@" )
    ;;
  esac
  echo "<=== $dir"
done

echo "===> rpm"
rm -rf autom4te.cache || true
echo "---> generate files via GNU libtool (libtoolize)"
libtoolize --quiet --copy --force --install
echo "---> generate files via GNU gettext (autopoint)"
po_dir=./po
LANG=C
ls "$po_dir"/*.po 2>/dev/null |
	sed 's|.*/||; s|\.po$||' > "$po_dir/LINGUAS"
autopoint --force
echo "---> generate files via GNU autoconf (aclocal, autoheader)"
rm -f aclocal.m4
aclocal -I m4
autoheader -I m4
echo "---> generate files via GNU automake (automake)"
automake -Wall -Wno-override -a -c
echo "---> generate files via GNU autoconf (autoconf)"
autoconf -I m4
echo "<=== rpm"

