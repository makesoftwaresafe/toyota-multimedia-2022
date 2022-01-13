#!/bin/sh

opts=""
paths=". file:///tmp http://rpm5.org/files/popt ftp://localhost/pub"

for path in $paths
do
    ./tget $opts $path
    ./tget $opts $path/testit.sh

    ./tdir $opts $path

    ./tglob $opts $path/*

    ./tfts $opts $path
done
