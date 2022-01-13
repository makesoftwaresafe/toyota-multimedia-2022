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

#include <string>
#include <vector>
#include <gst/gst.h>
#include "CAmSerializer.h"
#include "IAmApplicationClient.h"
#include "CThread.h"

namespace am {

class IPlayer
{
public:
    virtual ~IPlayer() {};
    virtual am_Error_e initialize() = 0;    /** called on asyncConnect by AudioManager           */
    virtual am_Error_e deinitialize() = 0;  /** called on asyncDisconnect by AudioManager        */
    virtual am_Error_e play() = 0;  /** called on setSourceState(SS_ON) by AudioManager and by user    */
    virtual am_Error_e pause() = 0; /** called on setSourceState(SS_PAUSE) by AudioManager and by user */
    virtual am_Error_e stop() = 0;  /** called on setSourceState(SS_OFF) by AudioManager and by user   */
    virtual void next() = 0;  /** self-called on EOF and by user                                 */
};

struct CPlayInfo
{
    CPlayInfo(std::string f, unsigned int t) : file(f), timeout(t) {};
    std::string  file;
    unsigned int timeout;
};

class IAmInterface;

class CPlayer : public IPlayer, public adit::utility::CThread
{
public:
    CPlayer(int argc, char **argv, CAmSocketHandler *socketHandler);
    ~CPlayer() final;

    void setInterface(IAmInterface *interface);

    void add(const std::string & file, unsigned int timeout = 0);
    void set(const std::string & device);
    am_Error_e initialize() final;
    am_Error_e deinitialize() final;
    am_Error_e play() final;
    am_Error_e pause() final;
    am_Error_e stop() final;
    void next() final;

private:
    void eof();

private:
    int initThread() final;
    int workerThread() final;
    void deinitThread(int errInit) final;

private:
    /* GStreamer */
    static gboolean _BusThread(GstBus *bus, GstMessage *msg, gpointer data) {
        return ((CPlayer*)data)->BusThread(bus, msg);
    }
    gboolean BusThread(GstBus *bus, GstMessage *msg);

    GstBus                            *mpBus;
    GMainLoop                         *mpLoop;
    GstElement                        *mpPlaybin;
    GstElement                        *mpAudioSink;

    bool                               mContinues;
    std::string                        mFile;
    std::string                        mDevice;
    std::vector<CPlayInfo>             mPlaylist;
    std::vector<CPlayInfo>::iterator   mItrPlaylist;

    CAmSocketHandler                  *mpSocketHandler;
    IAmInterface                      *mpAudioManagerInterface;
    CAmSerializer                      mSerializer;
};


}

#endif // CPLAYER_H_ //
