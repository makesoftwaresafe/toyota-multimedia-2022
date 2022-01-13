/************************************************************************
 * @file: CPlayer.cpp
 *
 * @version: 1.1
 *
 * @description: This is the playback engine using GST.
 * @component: platform/audiomanager
 *
 * @author: Jens Lorenz, jlorenz@de.adit-jv.com 2013,2014
 *          Jayanth MC, Jayanth.mc@in.bosch.com 2013,2014
 *
 * @copyright (c) 2014 Advanced Driver Information Technology.
 * This code is developed by Advanced Driver Information Technology.
 * Copyright of Advanced Driver Information Technology, Bosch, and DENSO.
 * All rights reserved.
 *
 * @see <related items>
 *
 * @history
 *
 ***********************************************************************/
#include <stdexcept>
#include <sched.h>
#include <errno.h>
#include "CAmDltWrapper.h"
#include "CPlayer.h"


using namespace am;

#define THROW_ASSERT_NEQ(CALL, COND) \
        if (!((CALL) == (COND))) throw std::runtime_error( \
            std::string(__func__) + ": (" + std::string(#CALL) + " != " + std::to_string(COND) + ")")

#define THROW_ASSERT_NULL(CALL) \
        if (!(CALL)) throw std::runtime_error( \
            std::string(__func__) + ": (" + std::string(#CALL) + " == NULL)")

#define GST_PLAY_FLAG_AUDIO         (0x1 << 1)
#define GST_PLAY_FLAG_NATIVE_AUDIO  (0x1 << 5)


CPlayer::CPlayer(int argc, char **argv) :
    m_argc(argc), m_argv(argv),
    initState(INIT_UNKNOWN), mThreadID(), mMtx(), mCond(),
    m_bus(NULL), m_loop(NULL), m_playbin(NULL), m_audiosink(NULL),
    m_error(false)
{
    pthread_attr_t attr;
    struct sched_param param;
    param.sched_priority = 80;

    /* Initialize mutex and condition variable objects */
    THROW_ASSERT_NEQ(pthread_mutex_init(&mMtx, NULL), 0);
    THROW_ASSERT_NEQ(pthread_cond_init(&mCond, NULL), 0);

    /* start thread */
    THROW_ASSERT_NEQ(pthread_attr_init(&attr), 0);
    THROW_ASSERT_NEQ(pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE), 0);
    THROW_ASSERT_NEQ(pthread_attr_setschedpolicy(&attr, SCHED_FIFO), 0);
    THROW_ASSERT_NEQ(pthread_attr_setschedparam(&attr, &param), 0);
    THROW_ASSERT_NEQ(pthread_create(&mThreadID, &attr, &CPlayer::_WorkerThread, this), 0);
    THROW_ASSERT_NEQ(pthread_attr_destroy(&attr), 0);

    /* wait on GST state notification */
    THROW_ASSERT_NEQ(pthread_mutex_lock(&mMtx), 0);
    while (initState == INIT_UNKNOWN)
        THROW_ASSERT_NEQ(pthread_cond_wait(&mCond, &mMtx), 0);
    THROW_ASSERT_NEQ(pthread_mutex_unlock(&mMtx), 0);
}

CPlayer::~CPlayer()
{
    this->stop();
    gst_element_set_state(m_playbin, GST_STATE_NULL);
    gst_object_unref(GST_OBJECT(m_playbin));
    gst_object_unref(GST_OBJECT(m_bus));
}

void* CPlayer::WorkerThread(void)
{
    int flags = GST_PLAY_FLAG_NATIVE_AUDIO | GST_PLAY_FLAG_AUDIO;

    /* init player */
    gst_init(&m_argc, &m_argv);

    THROW_ASSERT_NULL((m_loop = g_main_loop_new(NULL, FALSE)));

    m_playbin = gst_element_factory_make("playbin2", "play");
    if (NULL == m_playbin)
    {
        /*
         * Before throwing the Assert, let's try and open playbin. This deals with
         * gstreamer-0.10 VS gstreamer-1.0 selection on different boards
         */
        THROW_ASSERT_NULL((m_playbin = gst_element_factory_make("playbin", "play")));
    }
    THROW_ASSERT_NULL((m_audiosink = gst_element_factory_make("alsasink", "audio")));

    g_object_set(G_OBJECT(m_playbin), "audio-sink", m_audiosink, NULL);
    g_object_set(G_OBJECT(m_playbin), "flags", flags, NULL);

    THROW_ASSERT_NULL((m_bus = gst_pipeline_get_bus(GST_PIPELINE(m_playbin))));
    gst_bus_add_watch(m_bus, &CPlayer::_BusThread, this);

    /* notify class is successfully created */
    THROW_ASSERT_NEQ(pthread_mutex_lock(&mMtx), 0);
    initState = INIT_DONE;
    THROW_ASSERT_NEQ(pthread_cond_signal(&mCond), 0);
    THROW_ASSERT_NEQ(pthread_mutex_unlock(&mMtx), 0);

    g_main_loop_run(m_loop);

    pthread_exit(NULL);
    return NULL;
}

gboolean CPlayer::BusThread(GstBus *bus, GstMessage *msg)
{
    (void)bus;

    switch (GST_MESSAGE_TYPE(msg))
    {
        case GST_MESSAGE_EOS:
        {
            logInfo("CPlayer::BusThread EOF", m_file);
            g_main_loop_quit(m_loop);
            break;
        }
        case GST_MESSAGE_ERROR:
        {
            gchar *debug;
            GError *error;

            gst_message_parse_error(msg, &error, &debug);
            g_free(debug);

            logError("CPlayer::BusThread GST Error:", error->message, "in file", m_file);
            g_error_free(error);

            m_error = true;
            g_main_loop_quit(m_loop);
            break;
        }
        default:
            break;
    }
    return TRUE;
}

void CPlayer::play(std::string file, std::string device)
{
    m_file = file;
    m_device = device;

    logInfo("CPlayer::play", m_file, "to", m_device);
    g_object_set(G_OBJECT(m_playbin), "uri", m_file.c_str(), NULL);
    g_object_set(G_OBJECT(m_audiosink), "device", m_device.c_str(), NULL);

    gst_element_set_state(m_playbin, GST_STATE_PLAYING);
}

bool CPlayer::stop()
{
    if (mThreadID == 0)
        return false;

    logInfo("CPlayer::stop", m_file);

    g_main_loop_quit(m_loop);
    pthread_join(mThreadID, NULL);
    mThreadID = 0;

    return m_error;
}

bool CPlayer::wait()
{
    if (mThreadID == 0)
        return false;

    logInfo("CPlayer::wait for EOF in", m_file);

    pthread_join(mThreadID, NULL);
    mThreadID = 0;

    return m_error;
}
