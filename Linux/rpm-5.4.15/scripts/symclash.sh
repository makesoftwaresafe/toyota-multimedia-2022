#!/bin/sh
files="/usr/sbin/httpd /usr/lib/httpd/modules/*.so /usr/lib/*.so.* /usr/lib/php/modules/*.so"
files="$files `find /usr/lib/perl5 -type f -name \*.so`"
files="$files `ldd $files | sed -n '/ => /{s/.*> //;s/ (.*$//;p}' | sort -u | sed '\\,^/lib/,d'`"
files=`for f in $files; do echo "$f"; done | sed 's/ *//' | sort -u`
files=`for f in $files; do test -h "$f" || echo $f; done`
exec ./symclash.py $files
