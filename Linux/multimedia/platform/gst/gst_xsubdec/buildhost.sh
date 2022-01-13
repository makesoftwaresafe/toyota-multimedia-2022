#!/bin/sh

mkdir -p host

gcc -shared src/*.c -o host/libgstxsubdec.so -g -O0 -Wall -fpic -DHOST_COMPILE $(pkg-config --cflags --libs gstreamer-0.10 gstreamer-video-0.10)

D=$HOME/.gstreamer-0.10/plugins
mkdir -p ${D}
cp -v host/libgstxsubdec.so ${D}

