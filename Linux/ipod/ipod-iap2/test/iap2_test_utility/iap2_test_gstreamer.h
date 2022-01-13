#ifndef IAP2_TEST_GSTREAMER_H_
#define IAP2_TEST_GSTREAMER_H_


/* **********************  includes  ********************** */

#include <adit_typedef.h>
//#include "iap2_smoketest.h"
#define IAP2_GST_AUDIO_STREAM                                1
#if IAP2_GST_AUDIO_STREAM

#include <gst/gst.h>
#include <gst/base/gstbasetransform.h>

#ifndef GSTREAMER_VERSION_10
#include <gst/audio/gstbaseaudiosink.h> //GST_BASE_AUDIO_SINK_SLAVE_RESAMPLE
#endif

/* **********************  defines  ********************** */

typedef enum
{
    IAP2_GSTREAMER_DEINITIALIZED = 0,
    IAP2_GSTREAMER_STOPPED,
    IAP2_GSTREAMER_INITIALIZED,
    IAP2_GSTREAMER_PLAYING,
    IAP2_GSTREAMER_PAUSED
} GStreamer_curr_state;

typedef enum
{
    IAP2_GSTREAMER_STATE_INITIALIZE = 0,
    IAP2_GSTREAMER_STATE_DEINITIALIZE,
    IAP2_GSTREAMER_STATE_PLAYING,
    IAP2_GSTREAMER_STATE_PAUSE,
    IAP2_GSTREAMER_STATE_RESUME,
    IAP2_GSTREAMER_STATE_STOP,
    IAP2_GSTREAMER_SAMPLE_RATE_CHANGE
} GStreamer_state;

typedef struct
{
  GstElement* audiosink;
  GstElement* audiosrc;
  GstElement* capsfilter;
  GstElement* queue;
  GstElement* rate;
  GstElement* pipeline;
  GstElement* resample;
  GstElement* convert;
  GMainLoop *loop;
  GstBus *bus;
} GStreamer_player;


/* **********************  externals  ********************** */

void iap2SetGstState(GStreamer_state GstState);
GStreamer_curr_state iap2GetGstState(void);

void iap2SetGstSampleRate(U8 sampleRate);
U8 iap2GetGstSampleRate(void);
void iap2SetGstSampleRate2(U32 sampleRate);

/* **********************  functions  ********************** */

/* set to TRUE, to leave iap2GstThread */
void iap2SetExitGstThread(BOOL value);
/* gstreamer thread */
void iap2GstThread(void *exinf);

/* run g_main_loop_run() */
S32 iap2GstRun(void *exinf);

#endif  // #if IAP2_GST_AUDIO_STREAM

#endif /* IAP2_TEST_GSTREAMER_H_ */
