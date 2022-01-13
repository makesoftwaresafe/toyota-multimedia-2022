#!/bin/sh

# Kernel module dependency extractor.
#
# Author(s):	Danny Tholen <obiwan@mandriva.org>
#	     	Olivier Blin <blino@mandriva.org>
#		Per Øyvind Karlsen <peroyvind@mandriva.org>
# 

provides=0
requires=0
modinfo=/sbin/modinfo

while [ "$#" -ne 0 ]; do
    case $1 in
	-P|--provides)
	    provides=1
	    ;;
	-R|--requires)
	    requires=1
	    ;;
	--modinfo)
	    shift
	    modinfo=$1
	    ;;
    esac
    shift
done

if [ $requires -eq 1 ]; then
    echo "--requires not implemented!" 1>&2
    exit 1
fi

if [ $provides -eq 1 ]; then
    provideslist=`sed "s/['\"]/\\\&/g"`
    modulelist=$(echo "$provideslist" | egrep '^.*(/lib/modules/|/var/lib/dkms/).*\.ko(\.gz|\.xz)?$')
    echo $modulelist | xargs -r $modinfo | \
	perl -lne '
    $name = $1 if m!^filename:\s*(?:.*/)?([^/]+)\.k?o!;
    $ver = $1 if /^version:\s*[a-zA-Z]{0,6}\-?(\d+[\.\:\-\[\]]?\d*[\.\:\-\[\]]?\d*[\.\:\-\[\]]?\d*[\.\:\-\[\]]?\d*-?[a-zA-Z]{0,6}\d?).*/;
    $ver =~ s/(\:|-)/_/;
    if (/^vermagic:/) {
	print "kmod\($name\)" . ($ver ? " = $ver" : "") if $name;
	undef $name; undef $ver;
    }
    '
    dkmslist=$(echo "$provideslist" | egrep '(/var/lib/dkms-binary/[^/]+/[^/]+|/usr/src)/[^/]+/dkms.conf$')
    [ -n "$dkmslist" ] && for d in $dkmslist; do
	VERSION=`sed -rne 's/^PACKAGE_VERSION="?([^"]+)"?$/\1/;T;p' $d`
	[ -z "$VERSION" ] && continue
	PACKAGE_NAME=`sed -rne 's/^PACKAGE_NAME="?([^"]+)"?$/\1/;T;p' $d`
	MODULES=`sed -rne 's/^DEST_MODULE_NAME\[[0-9]+\]="?([^"]+)"?$/\1/;T;p' $d`
	[ -z "$MODULES" ] && MODULES=`sed -rne 's/^BUILT_MODULE_NAME\[[0-9]+\]="?([^"]+)"?$/\1/;T;p' $d`
	# default on PACKAGE_NAME if no BUILT_MODULE_NAME is specified
	[ -z "$MODULES" ] && MODULES=$PACKAGE_NAME
	echo "$MODULES" | sed -re "s/\\\$PACKAGE_NAME/$PACKAGE_NAME/" | while read m; do
	    echo "kmod($m) = $VERSION"
	done
    done
fi
