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

#include <stdlib.h>
#include <stdexcept>
#include <errno.h>
#include <string.h>
#include "CPlayer.h"
#include "CAmInterface.h"
#include "CAmDltWrapper.h"
#include "CAmSocketHandler.h"


using namespace am;
using namespace std;

#define ADD_URI(_file) ("file://" + _file)

#define THROW_ASSERT_NEQ(CALL, COND) \
        if (!((CALL) == (COND))) throw runtime_error( \
            string(__func__) + ": (" + string(#CALL) + " != " + to_string(COND) + ")")

#define RE_IF_NULL(CALL) \
        if (!(CALL)) return -1;

#define GST_PLAY_FLAG_AUDIO         (0x1 << 1)
#define GST_PLAY_FLAG_NATIVE_AUDIO  (0x1 << 5)



CPlayer::CPlayer(int argc, char **argv, CAmSocketHandler *socketHandler) :
    mpBus(NULL), mpLoop(NULL), mpPlaybin(NULL), mpAudioSink(NULL),
    mContinues(true), mFile(""), mDevice(""), mItrPlaylist(mPlaylist.end()),
    mpSocketHandler(socketHandler), mpAudioManagerInterface(NULL), mSerializer(socketHandler)
{
    gst_init(&argc, &argv);
}

CPlayer::~CPlayer()
{
    this->stop();
    this->deinitialize();
}

void CPlayer::setInterface(IAmInterface *interface)
{
    mpAudioManagerInterface = interface;
}


void CPlayer::add(const string & file, unsigned int timeout)
{
    logInfo("CPlayer::add", file, timeout);
    mContinues &= (timeout == 0);
    mPlaylist.push_back(CPlayInfo(file, timeout));
    mItrPlaylist = mPlaylist.begin();
}

void CPlayer::set(const string & device)
{
    mDevice = device;
}

am_Error_e CPlayer::initialize()
{
    if (mPlaylist.empty()) {
        logError("CPlayer::initialize not possible");
        return E_DATABASE_ERROR;
    }

    if (mItrPlaylist == mPlaylist.end()) {
        logError("CPlayer::initialize end of playlist reached");
        return E_NO_CHANGE;
    }

    int err = CThread::startThread();
    if (err < 0) {
        logError("CPlayer::initialize startThread returns", (const char*)strerror(err));
        return E_UNKNOWN;
    }

    logInfo("CPlayer::initialize");
    return E_OK;
}

am_Error_e CPlayer::deinitialize()
{
    if (!mpLoop)
        return E_OK;

    g_main_loop_quit(mpLoop);
    int err = CThread::joinThread();
    if (err < 0) {
        logError("CPlayer::deinitialize joinThread returns", (const char*)strerror(err));
        return E_UNKNOWN;
    }

    logInfo("CPlayer::deinitialize");
    return E_OK;
}

am_Error_e CPlayer::play()
{
    if (!mpPlaybin)
        return E_UNKNOWN;

    logInfo("CPlayer::play", mFile);
    gst_element_set_state(mpPlaybin, GST_STATE_PLAYING);

    return E_OK;
}

am_Error_e CPlayer::pause()
{
    if (!mpPlaybin)
        return E_UNKNOWN;

    logInfo("CPlayer::pause", mFile);
    gst_element_set_state(mpPlaybin, GST_STATE_PAUSED);

    return E_OK;
}

am_Error_e CPlayer::stop()
{
    if (!mpPlaybin)
        return E_UNKNOWN;

    logInfo("CPlayer::stop", mFile);
    gst_element_set_state(mpPlaybin, GST_STATE_NULL);

    return E_OK;
}

void CPlayer::next()
{
    if (!mpPlaybin && mContinues)
        return;

    if (!mpPlaybin && !mFile.empty()) {
        mpAudioManagerInterface->connect();
        return;
    }

    if (++mItrPlaylist == mPlaylist.end()) {
        mpAudioManagerInterface->end();
        return;
    }

    mFile = ADD_URI(mItrPlaylist->file);
    logInfo("CPlayer::next", mFile);
    gst_element_set_state(mpPlaybin, GST_STATE_NULL);
    g_object_set(G_OBJECT(mpPlaybin), "uri", mFile.c_str(), NULL);
    gst_element_set_state(mpPlaybin, GST_STATE_PLAYING);
}

void CPlayer::eof()
{
    if (++mItrPlaylist == mPlaylist.end()) {
        mpAudioManagerInterface->end();
        return;
    }

    mFile = ADD_URI(mItrPlaylist->file);
    mpAudioManagerInterface->reconnect((mItrPlaylist-1)->timeout);
}

int CPlayer::initThread()
{
#define RETRY 3
    const gchar* playbin[RETRY] = { "playbin3", "playbin2", "playbin" };
    int flags = GST_PLAY_FLAG_NATIVE_AUDIO | GST_PLAY_FLAG_AUDIO;

    RE_IF_NULL(mpLoop = g_main_loop_new(NULL, FALSE));

    /** This loop is important to support GST 0.10 as well as 1.0 */
    for (int i = 0; i < RETRY && mpPlaybin == NULL; i++)
        mpPlaybin = gst_element_factory_make(playbin[i], "play");

    RE_IF_NULL(mpPlaybin);
    RE_IF_NULL(mpAudioSink = gst_element_factory_make("alsasink", "audio"));
    g_object_set(G_OBJECT(mpPlaybin), "audio-sink", mpAudioSink, NULL);
    g_object_set(G_OBJECT(mpPlaybin), "flags", flags, NULL);

    RE_IF_NULL(mpBus = gst_pipeline_get_bus(GST_PIPELINE(mpPlaybin)));
    gst_bus_add_watch(mpBus, &CPlayer::_BusThread, this);

    mFile = ADD_URI(mItrPlaylist->file);
    g_object_set(G_OBJECT(mpPlaybin), "uri", mFile.c_str(), NULL);
    g_object_set(G_OBJECT(mpAudioSink), "device", mDevice.c_str(), NULL);

    return 0;
}

int CPlayer::workerThread()
{
    g_main_loop_run(mpLoop);
    return 1;
}

void CPlayer::deinitThread(int)
{
    gst_object_unref(GST_OBJECT(mpBus));
    mpBus = NULL;
    gst_object_unref(GST_OBJECT(mpPlaybin));
    mpPlaybin = NULL;
    mpAudioSink = NULL;
    g_main_loop_unref(mpLoop);
    mpLoop = NULL;
}

gboolean CPlayer::BusThread(GstBus*, GstMessage *msg)
{
    switch (GST_MESSAGE_TYPE(msg))
    {
        case GST_MESSAGE_EOS:
        {
            logInfo("CPlayer::BusThread EOF", mFile);
            mSerializer.asyncCall<CPlayer>(this, &CPlayer::eof);
            break;
        }
        case GST_MESSAGE_ERROR:
        {
            gchar *debug;
            GError *error;

            gst_message_parse_error(msg, &error, &debug);
            g_free(debug);

            logError("CPlayer::BusThread GST Error:", static_cast<const char*>(error->message), "in file", mFile);
            g_error_free(error);

            mSerializer.asyncCall<IAmInterface, am_Error_e>(mpAudioManagerInterface, &IAmInterface::disconnect);
            mpSocketHandler->exit_mainloop();
            break;
        }
        default:
            break;
    }
    return TRUE;
}
