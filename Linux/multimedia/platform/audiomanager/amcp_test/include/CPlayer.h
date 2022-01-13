/************************************************************************
 * @file: CPlayer.h
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

#ifndef CPLAYER_H_
#define CPLAYER_H_

#include <string.h>
#include <gst/gst.h>

namespace am {

class CPlayer
{
public:
    CPlayer(int argc, char **argv);
    ~CPlayer();
    void play(std::string file, std::string device);
    bool stop();
    bool wait();

private:

    enum eInitState {
        INIT_UNKNOWN,
        INIT_DONE
    };

    int m_argc;
    char **m_argv;
    eInitState initState;
    pthread_t mThreadID;
    pthread_mutex_t mMtx;
    pthread_cond_t mCond;
    static void* _WorkerThread (void* This)
    {
        return ((CPlayer*)This)->WorkerThread();
    }
    void* WorkerThread(void);

private:
    /* GStreamer */
    static gboolean _BusThread(GstBus *bus,
            GstMessage *msg, gpointer data)
    {
        return ((CPlayer*)data)->BusThread(bus, msg);
    }
    gboolean BusThread(GstBus *bus, GstMessage *msg);

    GstBus         *m_bus;
    GMainLoop      *m_loop;
    GstElement     *m_playbin;
    GstElement     *m_audiosink;
    bool           m_error;
    std::string    m_file;
    std::string    m_device;
};
}

#endif // CPLAYER_H_ //
