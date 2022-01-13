#!/bin/sh
phpize
CFLAGS='-L../../lib/.libs -I../../lib' ./configure --with-syck=../../lib/.libs
#./configure --with-syck
make
make install
