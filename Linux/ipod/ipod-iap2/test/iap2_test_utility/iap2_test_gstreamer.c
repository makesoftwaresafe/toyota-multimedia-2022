
/* **********************  includes  ********************** */

#include "iap2_test_gstreamer.h"
#include "iap2_test_utility.h"
#include "iap2_test_init.h"
#include <semaphore.h>
#include <string.h>
#include <inttypes.h>
#include "iap2_test_uac2_cur.h"

/* **********************  defines  *********************** */

#if IAP2_GST_AUDIO_STREAM

/* **********************  external  ********************** */



/* **********************  globals  *********************** */
S32 MonitorThreadFunction_stat = 1;

int USBtoINTSampleRate[] = {
    8000,     /**< IAP2_SAMPLERATE_8000HZ  */
    11025,    /**< IAP2_SAMPLERATE_11025HZ */
    12000,    /**< IAP2_SAMPLERATE_12000HZ */
    16000,    /**< IAP2_SAMPLERATE_16000HZ */
    22050,    /**< IAP2_SAMPLERATE_22050HZ */
    24000,    /**< IAP2_SAMPLERATE_24000HZ */
    32000,    /**< IAP2_SAMPLERATE_32000HZ */
    44100,    /**< IAP2_SAMPLERATE_44100HZ */
    48000     /**< IAP2_SAMPLERATE_48000HZ */
};


BOOL g_leaveGstWhile = FALSE;
sem_t g_testGstSemaphore;

/* default sample rate set to 44100Hz */
U8 g_testGstSampleRate = 7;
/* default state of thread iap2GstThread */
GStreamer_state g_testGstState = IAP2_GSTREAMER_STATE_DEINITIALIZE;
GStreamer_curr_state g_testGstThreadState = IAP2_GSTREAMER_DEINITIALIZED;

GStreamer_player g_testGstPlay;

/* **********************  externals  ********************** */


void iap2SetGstState(GStreamer_state GstState)
{
    g_testGstState = GstState;
    sem_post(&g_testGstSemaphore);
}

GStreamer_curr_state iap2GetGstState(void)
{
    return g_testGstThreadState;
}

void iap2SetGstSampleRate(U8 sampleRate)
{
    g_testGstSampleRate = sampleRate;
}

void iap2SetGstSampleRate2(U32 sampleRate)
{
    int i;
    for(i=0;i<9;i++)
    {
        if(sampleRate == (U32)USBtoINTSampleRate[i]) {
            g_testGstSampleRate = i;
            break;
        }
    }
}

U8 iap2GetGstSampleRate(void)
{
    return g_testGstSampleRate;
}


/* **********************  functions  ********************** */

static void readPads(GstElement* pElem)
{
    GstIterator *it = NULL;
    unsigned int done = FALSE;
#ifndef GSTREAMER_VERSION_10
    gpointer itData;
#else
    GValue itData = G_VALUE_INIT;
#endif
    it = gst_element_iterate_src_pads(pElem);
    while (!done)
    {
        switch (gst_iterator_next(it, &itData))
        {
            case GST_ITERATOR_OK:
            {
#ifndef GSTREAMER_VERSION_10
                GstPad* pad = GST_PAD(itData);
#else
                GstPad* pad = GST_PAD(g_value_get_object(&itData));
#endif
                if (pad)
                {
                    GstStructure *str = NULL;
                    GstCaps *caps = NULL;

#ifndef GSTREAMER_VERSION_10
                    caps = gst_pad_get_caps(pad);
#else
                    caps = gst_pad_query_caps(pad, NULL);
#endif
                    str = gst_caps_get_structure(caps, 0);

                    printf("GStreamer: readPads() for '%s' \n   the caps are: '%s'\n",
                            pad->object.name, gst_structure_to_string(str));
                }
                else
                {
                    printf("GStreamer: readPads() ERROR: Failed to cast itdata to GstPad\n");
                }
                break;
            }
            case GST_ITERATOR_RESYNC:
            {
                gst_iterator_resync(it);
                break;
            }
            case GST_ITERATOR_ERROR:
            case GST_ITERATOR_DONE:
            {
                done = TRUE;
                break;
            }
            default:
            {
                break;
            }
        }
    }
    gst_iterator_free(it);
}

static gboolean bus_call(GstBus *bus, GstMessage *msg, gpointer data)
{
    /* To avoid compiler warning */
    (void)bus;
    (void)data;
    switch(GST_MESSAGE_TYPE(msg))
    {
        case GST_MESSAGE_EOS:
        {
            printf("GStreamer: bus_call() End of stream\n");
            g_main_loop_quit(g_testGstPlay.loop);
            break;
        }
        case GST_MESSAGE_STREAM_STATUS:
        {
            GstElement* pOwnerElement = NULL;
            GstStreamStatusType streamStatus;

            gst_message_parse_stream_status(msg, &streamStatus, &pOwnerElement);

            /* 0 => create, 1 => enter */
            printf("GStreamer: bus_call() Stream status event with type=%d for element='%s'\n",
                    streamStatus, pOwnerElement ? pOwnerElement->object.name : "");

            if (pOwnerElement && (0 == strcmp(pOwnerElement->object.name, "src"))
                    && (GST_STREAM_STATUS_TYPE_ENTER == streamStatus))
            {
                readPads(pOwnerElement);
            }
            break;
        }
        case GST_MESSAGE_NEW_CLOCK:
        {
            GstClock* pClock = NULL;
            gst_message_parse_new_clock(msg, &pClock);

#ifndef GSTREAMER_VERSION_10
            printf("GStreamer: bus_call() New clock event for clock id=%p\n", pClock ? pClock->clockid : NULL);
#endif
            printf("GStreamer: bus_call() New clock, structure: %s\n",
                    gst_structure_to_string(gst_message_get_structure(msg)));
            break;
        }
        case GST_MESSAGE_CLOCK_LOST:
        {
            printf("GStreamer: bus_call() ERROR: Clock lost => switch to paused\n");
            gst_element_set_state(GST_ELEMENT(g_testGstPlay.pipeline), GST_STATE_PAUSED);
            break;
        }
        case GST_MESSAGE_ASYNC_DONE:
        {
            printf("GStreamer: bus_call() ASYNC DONE\n");
            break;
        }
        case GST_MESSAGE_ERROR:
        {
            gchar  *debug;
            GError *error;

            gst_message_parse_error(msg, &error, &debug);
            g_free(debug);

            printf("GStreamer: bus_call() Error: %s\n", error->message);
            g_error_free(error);

            g_main_loop_quit(g_testGstPlay.loop);
            break;
        }
        case GST_MESSAGE_BUFFERING :
        {
            gint bufferStatus;
            gint currentBytes = 0;
            gint currentBuffers = 0;
            gint64 currentTime = 0;

            gst_message_parse_buffering(msg, &bufferStatus);

            g_object_get(G_OBJECT(g_testGstPlay.queue), "current-level-time", &currentTime, NULL);
            g_object_get(G_OBJECT(g_testGstPlay.queue), "current-level-bytes", &currentBytes, NULL);
            g_object_get(G_OBJECT(g_testGstPlay.queue), "current-level-buffers", &currentBuffers, NULL);
//            printf("GStreamer: bus_call() buffering %d percent, timeLevel=%lldns, %d Bytes, %d buffers\n",
//                    bufferStatus, currentTime, currentBytes, currentBuffers);

            break;
        }
        case GST_MESSAGE_STATE_CHANGED :
        {
            GstState l_gstOldState;
            GstState l_gstNewState;
            GstState l_gstPendingState;

            gst_message_parse_state_changed(msg, &l_gstOldState, &l_gstNewState, &l_gstPendingState);
            if (l_gstNewState == l_gstPendingState)
            {
                printf("GStreamer: bus_call() New state is same as Pending state (%d)\n", l_gstNewState);
            }
            else
            {
//                printf("\telement %8s state: %7s -> %s\n",
//                           GST_OBJECT_NAME (msg->src),
//                           gst_element_state_get_name (l_gstOldState),
//                           gst_element_state_get_name (l_gstNewState));
            }
            break;
        }
        default:
        {
            printf("GStreamer: bus_call() Other/unknown message =%d\n", GST_MESSAGE_TYPE(msg));
            break;
        }
    }

    return TRUE;
}

/* gstreamer thread */
void iap2GstThread(void *exinf)
{
    int rc = 0;
    guint bus_watch_id = 0;

    GstCaps* caps = NULL;

    /* "hw:iPod,0" */
    char* audiosrc = NULL;
    char* audiosink = "entertainment_main";
    S32 strLen = 0;

    TEST_THREAD_ID GstTskID = 0;
    char threadName[8];
    void* status;

    U8* productName = (U8*)exinf;

    if(iap2InHostMode() == TRUE)
        productName = (U8*)"UAC2Gadget";

    memset(&g_testGstPlay, 0, sizeof(GStreamer_player) );

    strLen = strlen((const char*)productName);
    /* add length for "hw:" and ",0" and null */
    strLen += 6;
    audiosrc = calloc(strLen, sizeof(U8));
    if(audiosrc != NULL){
        if(iap2InHostMode() == TRUE){
            sprintf(audiosrc, "%s%s", "hw:", productName);
            pthread_t snd_thd;

            if(pthread_create( &snd_thd, NULL, MonitorThreadFunction, &MonitorThreadFunction_stat)) {
                printf(" =====================Cannot create uevent thread \n\n");
                g_leaveGstWhile = TRUE;
            }
        }else{
            sprintf(audiosrc, "%s%s%s", "hw:", productName, ",0");
        }
        printf(" iap2GstThread():  AudioSink: %s | AudioSource: %s\n",
                audiosink, audiosrc);
    } else{
        printf(" iap2GstThread():  allocation for audiosrc failed\n");
        g_leaveGstWhile = TRUE;
    }

    /* set default sample rate to 44100Hz */
    g_testGstSampleRate = 7;

    rc = sem_init(&g_testGstSemaphore, 0, 0);
    if(rc != 0){
        printf(" iap2GstThread():  sem_init failed\n");
        g_leaveGstWhile = TRUE;
    }

    while(g_leaveGstWhile == FALSE)
    {
        sem_wait(&g_testGstSemaphore);
        switch(g_testGstState)
        {
            case IAP2_GSTREAMER_STATE_INITIALIZE:
            {
                printf(" iap2GstThread():  Received IAP2_GSTREAMER_STATE_INITIALIZE %d\n",
                        USBtoINTSampleRate[g_testGstSampleRate]);
                /* Initialization */
                gst_init(NULL, NULL);

                /* Create main loop */
                g_testGstPlay.loop = g_main_loop_new(NULL, FALSE);
                if (NULL == g_testGstPlay.loop)
                {
                    printf(" iap2GstThread():  ERROR: Could not create loop\n");
                    exit(EXIT_FAILURE);
                }
                printf(" iap2GstThread():  Main loop created\n");

                /* Create pipeline */
                g_testGstPlay.pipeline = gst_pipeline_new("pipeline");
                if (NULL == g_testGstPlay.pipeline)
                {
                    printf(" iap2GstThread():  ERROR: Could not create pipeline\n");
                    exit(EXIT_FAILURE);
                }
#ifndef GSTREAMER_VERSION_10
                caps = gst_caps_new_simple("audio/x-raw-int",
#else
                caps = gst_caps_new_simple("audio/x-raw",
#endif
                              "rate", G_TYPE_INT, USBtoINTSampleRate[g_testGstSampleRate],
                              "channels", G_TYPE_INT, 2,
                              NULL);

                /* Create audio src */
                g_testGstPlay.audiosrc = gst_element_factory_make("alsasrc", "src");
                if (NULL == g_testGstPlay.audiosrc)
                {
                    printf(" iap2GstThread():  ERROR: Could not create audiosrc\n");
                    exit(EXIT_FAILURE);
                }
                else
                {
                    g_object_set(G_OBJECT(g_testGstPlay.audiosrc), "device", audiosrc, NULL);

                }

                /* Create cap filter to change the sample rate on the fly */
                g_testGstPlay.capsfilter = gst_element_factory_make("capsfilter", "filter");
                if (NULL == g_testGstPlay.capsfilter)
                {
                    printf(" iap2GstThread():  ERROR: Could not create capsfilter\n");
                    exit(EXIT_FAILURE);
                }
                else
                {
                    g_object_set(G_OBJECT(g_testGstPlay.capsfilter), "caps", caps, NULL);
                }

                /* Create queue */
                g_testGstPlay.queue = gst_element_factory_make("queue2", "queue");

                if (NULL == g_testGstPlay.queue)
                {
                    printf(" iap2GstThread():  ERROR: Could not create queue\n");
                    exit(EXIT_FAILURE);
                }
                else
                {
                    gint maxBuffers = 0;
                    gint maxBytes = 0;

                    g_object_set(G_OBJECT(g_testGstPlay.queue), "use-buffering", TRUE, NULL);

                    /* Read and set 'max-size-buffers' and read 'max-size-bytes' (valid for both queues) */
                    g_object_get(G_OBJECT(g_testGstPlay.queue), "max-size-bytes", &maxBytes, NULL);
                    g_object_get(G_OBJECT(g_testGstPlay.queue), "max-size-buffers", &maxBuffers, NULL);
//                    printf("GStreamer: max size buffers is %d (bytes=%d). Change to 1000 buffers\n", maxBuffers, maxBytes);
                    g_object_set(G_OBJECT(g_testGstPlay.queue), "max-size-buffers", 1000, NULL);

                    /* Read new values and print out */
                    g_object_get(G_OBJECT(g_testGstPlay.queue), "max-size-buffers", &maxBuffers, NULL);
                    g_object_get(G_OBJECT(g_testGstPlay.queue), "max-size-bytes", &maxBytes, NULL);
//                    printf("GStreamer: max size buffers is now %d (bytes=%d)\n", maxBuffers, maxBytes);
                }

                /* Create audio sink (USB soundcard) */
                g_testGstPlay.audiosink = gst_element_factory_make("alsasink", "sink");
                if (NULL == g_testGstPlay.audiosink)
                {
                    printf(" iap2GstThread():  ERROR: Could not create audiosink\n");
                    exit(EXIT_FAILURE);
                }
                else
                {
                    gint64 driftTolerance = 0;
                    g_object_set(G_OBJECT(g_testGstPlay.audiosink), "device", audiosink, NULL);
                    g_object_set(G_OBJECT(g_testGstPlay.audiosink), "sync", FALSE , NULL); //this solves the timing issue after some time

                    /* Read values */
                    g_object_get(G_OBJECT(g_testGstPlay.audiosink), "drift-tolerance", &driftTolerance, NULL);
                    printf("GStreamer: drift tolerance of sink is %"PRId64"\n", driftTolerance);
                }

                g_testGstPlay.convert  = gst_element_factory_make ("audioconvert",  "convert");
                g_testGstPlay.resample = gst_element_factory_make ("audioresample", "resample");

//                printf("GStreamer: All elements created\n");
                gst_bin_add_many(GST_BIN(g_testGstPlay.pipeline), g_testGstPlay.audiosrc, g_testGstPlay.capsfilter, g_testGstPlay.queue, g_testGstPlay.convert, g_testGstPlay.resample, g_testGstPlay.audiosink, NULL);

//                gst_element_link_filtered(g_testGstPlay.audiosrc, g_testGstPlay.queue, caps);
                gst_element_link_many(g_testGstPlay.audiosrc, g_testGstPlay.capsfilter, g_testGstPlay.queue, g_testGstPlay.convert, g_testGstPlay.resample, g_testGstPlay.audiosink, NULL);

                /* Adding a message filter */
                g_testGstPlay.bus = gst_pipeline_get_bus(GST_PIPELINE(g_testGstPlay.pipeline));
                if (NULL == g_testGstPlay.bus)
                {
                    printf(" iap2GstThread():  ERROR: Could not create bus\n");
                    exit(EXIT_FAILURE);
                }
                else
                {
                    bus_watch_id = gst_bus_add_watch(g_testGstPlay.bus, bus_call, NULL);
                    gst_object_unref(g_testGstPlay.bus);
                }
                g_testGstThreadState = IAP2_GSTREAMER_INITIALIZED;

                printf(" iap2GstThread():  initialized\n");

                /* Iterate */
                break;
            }
            case IAP2_GSTREAMER_STATE_DEINITIALIZE:
            {
                printf(" iap2GstThread():  Received IAP2_GSTREAMER_STATE_DEINITIALIZE \n");
                g_testGstThreadState = IAP2_GSTREAMER_DEINITIALIZED;
                break;
            }
            case IAP2_GSTREAMER_STATE_PLAYING:
            {
                if(g_testGstThreadState != IAP2_GSTREAMER_PLAYING)
                {
                    printf(" iap2GstThread():  Received IAP2_GSTREAMER_STATE_PLAYING \n");
                    gst_element_set_state(GST_ELEMENT(g_testGstPlay.pipeline), GST_STATE_PLAYING);
                    g_testGstThreadState = IAP2_GSTREAMER_PLAYING;

                    /* set thread name */
                    memset(&threadName[0], 0, (sizeof(threadName)));
                    sprintf(&threadName[0], "%s%d", "iRun", 1);
                    GstTskID = iap2CreateThread(iap2GstRun, &threadName[0], NULL);
                    if(GstTskID == 0)
                    {
                        printf(" iap2GstThread():  Error in creating iap2GstRun Thread \n");
                        g_leaveGstWhile = TRUE;
                    }
                }
                break;
            }
            case IAP2_GSTREAMER_STATE_PAUSE:
            {
                printf(" iap2GstThread():  Received IAP2_GSTREAMER_STATE_PAUSE \n");
                gst_element_set_state(GST_ELEMENT(g_testGstPlay.pipeline), GST_STATE_PAUSED);
                g_testGstThreadState = IAP2_GSTREAMER_PAUSED;
                break;
            }
            case IAP2_GSTREAMER_STATE_RESUME:
            {
                printf(" iap2GstThread():  Received IAP2_GSTREAMER_STATE_RESUME \n");
                gst_element_set_state(GST_ELEMENT(g_testGstPlay.pipeline), GST_STATE_PLAYING);
                g_testGstThreadState = IAP2_GSTREAMER_PLAYING;
                break;
            }
            case IAP2_GSTREAMER_STATE_STOP:
            {
                printf(" iap2GstThread():  Received IAP2_GSTREAMER_STATE_STOP \n");
                MonitorThreadFunction_stat = 0;
                if(g_testGstPlay.pipeline != NULL)
                {
                    printf(" iap2GstThread():  un-referencing all the elements in the pipeline \n");
                    /* Out of main loop, do clean up */
                    if(g_testGstPlay.pipeline != NULL)
                    {
                        gst_element_set_state(g_testGstPlay.pipeline, GST_STATE_NULL);
                        gst_object_unref(GST_OBJECT(g_testGstPlay.pipeline));
                    }
                    g_source_remove(bus_watch_id);

                    if(caps != NULL)
                    {
                        gst_caps_unref(caps);
                        caps = NULL;
                    }
                    if(g_testGstPlay.loop != NULL)
                    {
                        g_main_loop_quit(g_testGstPlay.loop);

                        g_main_loop_unref(g_testGstPlay.loop);
                        g_testGstPlay.loop = NULL;
                    }
                    memset(&g_testGstPlay, 0, sizeof(GStreamer_player) );
                    if(GstTskID > 0)
                        (void)pthread_join(GstTskID, &status);
                }
                g_testGstThreadState = IAP2_GSTREAMER_STOPPED;
                break;
            }
            case IAP2_GSTREAMER_SAMPLE_RATE_CHANGE:
            {
                printf(" iap2GstThread():  Received IAP2_GSTREAMER_SAMPLE_RATE_CHANGE to %d \n",
                        USBtoINTSampleRate[g_testGstSampleRate]);

                if(caps != NULL)
                {
                    gst_caps_unref(caps);
                    caps = NULL;
                 }

#ifndef GSTREAMER_VERSION_10
                caps = gst_caps_new_simple("audio/x-raw-int",
#else
                caps = gst_caps_new_simple("audio/x-raw",
#endif
                        "rate", G_TYPE_INT, USBtoINTSampleRate[g_testGstSampleRate],
                        "channels", G_TYPE_INT, 2,
                        NULL);

                gst_element_set_state(g_testGstPlay.pipeline, GST_STATE_NULL);
                g_object_set(G_OBJECT(g_testGstPlay.capsfilter), "caps", caps, NULL);
                gst_element_set_state(g_testGstPlay.pipeline, GST_STATE_PLAYING);

                g_testGstThreadState = IAP2_GSTREAMER_PLAYING;
                printf(" iap2GstThread():  done =============== Received IAP2_GSTREAMER_SAMPLE_RATE_CHANGE to %d \n",
                        USBtoINTSampleRate[g_testGstSampleRate]);
               break;
            }
            default :
            {
                printf(" iap2GstThread():  Received Unknown g_testGstState \n");
                break;
            }
        }
    }

    if(audiosrc != NULL)
    {
        free(audiosrc);
        audiosrc = NULL;
    }

    sem_destroy(&g_testGstSemaphore);

    pthread_exit((void*)exinf);
}


void iap2SetExitGstThread(BOOL value)
{
    g_leaveGstWhile = value;
}

/* iap2GstRun Thread */
S32 iap2GstRun(void *exinf)
{
    S32 rc = 0;
    U16 retry = 0;
    (void)exinf;

    while(retry < 3)
    {
        if(g_testGstPlay.pipeline != NULL)
        {

            if(g_testGstState != IAP2_GSTREAMER_STATE_PLAYING)
            {
                iap2SetGstState(IAP2_GSTREAMER_STATE_PLAYING);
            }

            g_main_loop_run(g_testGstPlay.loop);

            /* ================ */
            printf("  iap2GstRun():  Returned, stopping playback\n" );

            /* Out of main loop, do clean up */
            iap2SetGstState(IAP2_GSTREAMER_STATE_STOP);

            rc = 0;
            break;
        }
        else
        {
            printf("iap2GstRun():  pipeline is NULL\n" );
            rc = -1;
            iap2SleepMs(1000);
            retry++;
        }
    }

    return rc;
}

#endif  // #if IAP2_GST_AUDIO_STREAM

