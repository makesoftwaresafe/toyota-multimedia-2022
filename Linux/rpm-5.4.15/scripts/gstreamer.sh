#!/bin/sh
#
# Script to install in:
# /usr/lib/rpm/redhat/find-provides.d
#
# Transform GStreamer auto install info into RPM provides
#
# Author: Bastien Nocera <hadess@hadess.net>
# Based on other provides scripts from RPM
#

[ $# -ge 1 ] || {
    cat > /dev/null
    exit 0
}

filelist=`grep -e '.so$' | sed "s/['\"]/\\\&/g"`
provides=0
gst_inspect=$(which gst-inspect 2>/dev/null)

# --- Alpha does not mark 64bit dependencies•
case `uname -m` in
  alpha*)	mark64="" ;;
  *)		mark64="()(64bit)" ;;
esac

solist=$(echo $filelist | grep "libgst" | \
	xargs file -L 2>/dev/null | grep "ELF.*shared object" | cut -d: -f1 )

getmark()
{
	lib64=`if file -L $1 2>/dev/null | \
		grep "ELF 64-bit" >/dev/null; then echo -n "$mark64"; fi`
}

libdir()
{
	buildlibdir=`dirname $1`
	buildlibdir=`dirname $buildlibdir`
}

while [ "$#" -ne 0 ]; do
    case $1 in
	-P|--provides)
	    provides=1
	    ;;
	--gst-inspect)
	    shift
	    gst_inspect=$1
	    ;;
    esac
    shift
done

if [ -z "$gst_inspect" ]; then
    exit 1
fi

if [ $provides -eq 1 ]; then
    for so in $solist ; do
	getmark $so
	libdir $so
	LD_LIBRARY_PATH=$buildlibdir $gst_inspect --print-plugin-auto-install-info --rpm $so 2> /dev/null | while read line ; do
	    echo -n "$line";
	    echo -n "$lib64"
	    echo
	done
    done
fi
exit 0

