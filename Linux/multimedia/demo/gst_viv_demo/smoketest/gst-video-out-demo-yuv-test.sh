#!/bin/bash

echo "smoke test for GST VIVANTE DEMO - YUV movies"

################################################################################
# prepare

which gst_viv_testapp  >/dev/null
if [ "$?" != "0" ] ; then
	echo "please install adit-gst-viv-demo_git-*.ipk"; exit 1
fi

/opt/platform/switch-graphics-backend.sh framebuffer

################################################################################

rm -f ~/.gstreamer-0.10/registry*

export LD_LIBRARY_PATH=/usr/lib/gstreamer-0.10

################################################################################

# MOVIES="bigbuckbunny_uyvy_1280_800.yuv  uyvy_1280_800.yuv bigbuckbunny_uyvy_320_240.yuv"
# MOVIES="bigbuckbunny_uyvy_1280_800.yuv"
MOVIES="bigbuckbunny_uyvy_320_240.yuv"
# MOVIES="uyvy_1280_800.yuv"

MOVIES_TAR="bigbuckbunny_uyvy_320_240.tar.bz2"
	if [ -f /var/volatile/$MOVIES_TAR ]; then	
		echo "Extracting YUV file.."
		tar xvf /var/volatile/$MOVIES_TAR --directory /var/volatile
	fi

# W=1024
# H=768

# W=1280
# H=800

W=320
H=240

# gst_viv_testapp <filename> <width> <height>

for YUV_MOVIE in $MOVIES
do

	if [ -f /var/volatile/$YUV_MOVIE ]; then	
		echo "gst viv YUV playing $YUV_MOVIE"
		gst_viv_testapp /var/volatile/$YUV_MOVIE $W $H
		if [ "$?" != "0" ]; then
			echo "failed!"
		fi

		echo "======================================================"
	else

		echo "movie /var/volatile/$YUV_MOVIE not found!"
		echo ""
		echo "copy movie file by executing on host:"
		echo "scp ~/DFSADIT/ADIT_Components/Software/VDEC/Test/bigbuckbunny_uyvy_320_240.tar.bz2 root@192.168.2.31:/var/volatile"
		echo ""

	fi

done


