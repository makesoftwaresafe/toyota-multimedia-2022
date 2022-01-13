#!/bin/sh
JCFDUMP=jcf-dump
JAR=fastjar
pkglist=`mktemp ${TMPDIR:-/tmp}/check-closure-pkglist-XXXXXX`
if test -z "$pkglist" ; then
	echo Error creating temporary file!
	exit 1
fi
tmpfile=`mktemp ${TMPDIR:-/tmp}/check-closure-provides-XXXXXX`
if test -z "$tmpfile" ; then
	echo Error creating temporary file!
	rm -f "$pkglist"
	exit 1
fi
trap "rm -f $pkglist $tmpfile" EXIT
rpm -qa --qf '%{name}\n' | sort -u > $pkglist
> $tmpfile
cat "$pkglist" | while read package ; do
	rpm -ql "$package" | ./java.prov $JCFDUMP $JAR | sort -u | sed "s,^,${package}:,g" >> $tmpfile
done
echo ""
cat "$pkglist" | while read package ; do
	rpm -ql "$package" | ./java.req $JCFDUMP $JAR | sort -u | while read req; do
		if ! grep -q ":${req}\$" $tmpfile ; then
			echo Missing: "$package" needs "$req".
		fi
	done
done
