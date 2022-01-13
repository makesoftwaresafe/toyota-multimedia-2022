#!/bin/sh

# Build a debug version of the sink and deploy it to target device and current
# sysroot
# assumes the target device is reachable as target.device

dir=$(dirname $0)

# make it work when called from subdir
OUTPUT="$dir/../../../../objects/IMX/RELEASE"
SYSROOT="$dir/../../../../oe-MGC_20130830-mx6q/build/tmp/sysroots/mx6q"

make -C $dir clean
make -C $dir C_FLAGS="-g -O0" -j6 || exit

scp $OUTPUT/usr/lib/gstreamer-0.10/libgst_apx_sink.so \
	target.device:/usr/lib/gstreamer-0.10 || exit
cp -v $OUTPUT/usr/lib/gstreamer-0.10/libgst_apx_sink.so \
	$SYSROOT/usr/lib/gstreamer-0.10 || exit

