/**
 * \file: gst_com_server_i.h
 *
 *  IPC server for communicate with PulseAudio and applicaiton process
 *
 * \component:  IPC server
 *
 * \author: ADIT
 *
 * \copyright: (c) 2003 - 2011 ADIT Corporation
 *
 */


#ifndef GST_COM_SERVER_I_H_
#define GST_COM_SERVER_I_H_
#include <gst/gst.h>
#include "iPodPlayerIPCLib.h"
#include "iPodPlayerDef.h"
#include "iPodPlayerDef_in.h"
#define GST_PIPE_BUF                256
#define GST_PIPELINE_NAME           "iPodGSTPipe"
#define GST_SRC_ELEMENT_PLUGIN      "alsasrc"
#define GST_SRC_ELEMENT_NAME        "alsa-source"
#define GST_SRC_PROP_DEVICE         "device"

#ifdef GST_AUTO_AUDIO_SINK_ENABLE
#define GST_SINK_ELEMENT_PLUGIN     "autoaudiosink"
#define GST_SINK_ELEMENT_NAME       "autoaudio-sink"
#else
#define GST_SINK_ELEMENT_PLUGIN     "alsasink"
#define GST_SINK_ELEMENT_NAME       "alsa-sink"
#endif /* GST_AUTO_AUDIO_SINK_ENABLE */

#define GST_SINK_PROP_DEVICE        "device"

#define GST_VOLUME_ELEMENT_PLUGIN   "volume"
#define GST_VOLUME_ELEMENT_NAME     "ipod_volume"
#define GST_VOLUME_PROP_VOLUME      "volume"
#define GST_CAPS_ELEMENET_PLUGIN    "capsfilter"
#define GST_CAPS_ELEMENT_NAME       "caps-filter"

#ifndef GSTREAMER_VERSION_10
#define GST_CAPS_PROP_SOURCE        "audio/x-raw-int"
#else
#define GST_CAPS_PROP_SOURCE        "audio/x-raw"
#endif

#define GST_CAPS_PROP_KEU_RATE      "rate"
#define GST_CAPS_RPROP_KEY_CAPS     "caps"


#define GST_STATUS_CHANGE_WAIT_TIME 500000000
#define GST_STATUS_CHANGE_SLEEP_TIME 200000
#define GST_STATUS_CHANGE_RETYR_MAX 20
#define GST_VALUE_100 100

/* PulseAudio control parameter */
struct IPOD_AUDIO_COM_INFO_
{
    GstElement *pipeElement;
    GstElement *srcElement;
    GstElement *sinkElement;
    GstElement *volumeElement;
    GstElement *capsElement;
    GstState gstState;
    GstCaps *caps;
    int run;    /* FALSE:context stopped, TRUE:context running */
    int ready;  /* context state callbak status
                   0:initial, 1:ready, 2:terminated */
    unsigned int card_index;            /* alsa-source card index */
    unsigned int module_index;          /* loopback module index */
    unsigned int module_source_index;   /* alsa-source module index */
    unsigned int source_index;          /* source index */
    unsigned int sink_input_index;      /* loopback sink input index */
    unsigned int rate;                  /* current src sample rate */
    unsigned char card_name[256];
    gdouble volume;
};


#endif /* GST_COM_SERVER_I_H_ */

