#! /bin/sh

export CFLAGS
export LDFLAGS

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

libtoolize --force --copy
aclocal
automake -a -c
autoconf
autoheader
