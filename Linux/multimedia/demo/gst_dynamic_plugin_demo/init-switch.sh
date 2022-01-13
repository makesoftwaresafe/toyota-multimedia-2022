
export XDG_RUNTIME_DIR=/tmp
export LM_PLUGIN_PATH=/usr/lib/layermanager
export LD_LIBRARY_PATH=/usr/lib/gstreamer-0.10
# export GST_DEBUG=*:2,*apx*:5,*v4lsink*:5,*tee*:5
export GST_DEBUG=*:2
export GST_DEBUG_DUMP_DOT_DIR=/home/root

#export GST_DEBUG=*:3

./gst_dynamic_plugin /dev/video2 9 /dev/video1
#gdbserver :2345 ./dynamic_plugin_test 9 9 1 t

