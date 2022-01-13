#!/bin/sh -e

JCFDUMP=${1:-jcf-dump}
JAR=${2:-jar}

dump=`mktemp ${TMPDIR:-/tmp}/classdepXXXXXX`
if test -z "$dump"; then
	echo Error creating temporary file.
	exit 1
fi

classdir=`mktemp -d ${TMPDIR:-/tmp}/classdepXXXXXX`
if test -z "$classdir"; then
	echo Error creating temporary directory.
	rm -f $dump
	exit 1
fi

cleanup() {
	rm -f $dump
	rm -fr $classdir
}

trap cleanup EXIT

scandeps()
{
	$JCFDUMP -c -v "$1" > $dump 2> /dev/null
	awk '
	/^This class:/ {
		gsub(",", "", $3);
		gsub("^[0123456789]+=", "", $3);
		print "java(" $3 ")";
	}
	' $dump
}

while read filename ; do
	case "$filename" in
	*.class)
		scandeps "$filename"
		;;
	*.jar) 
		if ! $JAR tf "$filename" | grep -q -e '(^..|^/|^\\)' ; then
                        PUSHDIR=`pwd`
			cd $classdir > /dev/null
			$JAR xf "$filename"
			cd $PUSHDIR > /dev/null
			$JAR tf "$filename" | sed "s|^|$classdir/|g" | $0 "$@"
			rm -fr $classdir/*
		fi
		;;
	*)
		;;
	esac
done
