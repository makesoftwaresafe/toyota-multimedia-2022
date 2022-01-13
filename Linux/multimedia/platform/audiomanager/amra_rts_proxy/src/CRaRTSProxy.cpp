/************************************************************************
 * @file: CRaRTSProxy.cpp
 *
 * @description: A proxy class implementation of Routing Adapter. Proxy class
 * does the connect operation between source and sink using ALSA API.
 *
 * @component: platform/audiomanager
 *
 * @author: Jens Lorenz, jlorenz@de.adit-jv.com 2013,2014
 *          Jayanth MC, Jayanth.mc@in.bosch.com 2013,2014
 *          Vanitha C, vanitha.channaiah@in.bosch.com 2017
 *
 * @copyright (c) 2010, 2011 Advanced Driver Information Technology.
 * This code is developed by Advanced Driver Information Technology.
 * Copyright of Advanced Driver Information Technology, Bosch, and DENSO.
 * All rights reserved.
 *
 * @see <related items>
 *
 * @history
 *
 ***********************************************************************/
#include <errno.h>
#include <stdlib.h>
#include <cassert>
#include <string.h>
#include "CRaRTSProxy.h"
#include "CAmDltWrapper.h"

using namespace std;
using namespace am;
using namespace adit::utility;

extern "C" IAmRoutingAdapterALSAProxy* RtsProxyFactory(const ra_Proxy_s& proxy)
{
    return new CRaRTSProxy(proxy);
}

CRaRTSProxy::CRaRTSProxy (const ra_Proxy_s & proxy) :
        IAmRoutingAdapterALSAProxy(proxy), CThread(), mRts(), mCfg()
{
    setThreadSched(mProxy.cpuScheduler.policy, mProxy.cpuScheduler.priority);
}

CRaRTSProxy::~CRaRTSProxy(void)
{
    freeCopyBuffers();
}

int CRaRTSProxy::initThread(void)
{
    int err = rts_create(&mRts, &mCfg);
    if (err < 0)
    {
        logError("CRaRTSProxy::initThread Create RTS streaming failed:", STRERROR(err));
    }
    return err;
}

int CRaRTSProxy::workerThread(void)
{
    int err = rts_read(mRts, (void**)mCopyBuffers);
    if (err < 0)
    {
        err = rts_recover(mRts);
        if (err < 0) {
            if (err == -ECANCELED) {
                logInfo("CRaRTSProxy::workerThread rts read recover cancelled");
            } else {
                logError("CRaRTSProxy::workerThread rts read recover failed:", STRERROR(err));
            }
            return err;
        }
        return 0;/* try rts_read after recover */
    }

    err = rts_write(mRts, (void**)mCopyBuffers);
    if (err < 0)
    {
        err = rts_recover(mRts);
        if (err < 0) {
            if (err == -ECANCELED) {
                logInfo("CRaRTSProxy::workerThread rts write recover cancelled");
            } else {
                logError("CRaRTSProxy::workerThread rts write recover failed:", STRERROR(err));
            }
            return err;
        }
    }
    return 0;
}

void CRaRTSProxy::deinitThread(int errInit)
{
    (void)errInit;
    trts_stat stat;
    rts_statistic(mRts, &stat);
    if (stat.num_xruns != 0)
    {
        logWarning("CRaRTSProxy::deinitThread", stat.num_xruns, "XRUNS detected");
    }

    int err = rts_destroy(mRts);
    if (err < 0)
    {
        logError("CRaRTSProxy::deinitThread Error on RTS destroy:", STRERROR(err));
    }
    else
    {
        logInfo("CRaRTSProxy::deinitThread Stop streaming from", mProxy.pcmSrc, "to", mProxy.pcmSink);
    }
}

string CRaRTSProxy::getThreadName(void) const
{
    string thread_name = "rtsp_" + mProxy.pcmSrc;

    return thread_name;
}

am_Error_e CRaRTSProxy::openStreaming(void)
{
    int formatSize = getFormatSize();
    if (formatSize < 0)
    {
        logError("CRaRTSProxy::openStreaming Invalid format configured");
        return am_Error_e::E_WRONG_FORMAT;
    }
    int frameSize = (static_cast<int>(mProxy.rate * mProxy.msBuffersize)) / MS_PER_SECOND;

    /* configure the devices */
    mAdevs[SRC_DEVICE].period_frames = static_cast<unsigned int>(frameSize);
    mAdevs[SRC_DEVICE].rate = mProxy.rate;
    mAdevs[SRC_DEVICE].format = static_cast<snd_pcm_format_t>(mProxy.format);
    mAdevs[SRC_DEVICE].startup_tout = mProxy.msInitTimeout;

    /* configure the audio streams */
    unsigned int offset(0);
    unsigned int maxDevices(mProxy.duplex ? MAX_DDUPLEX : MAX_DEVICES);

    /* In DUPLEX mode, devices can forward 8 channel stream */
    if ((maxDevices * mProxy.channels) > MAX_STREAMS)
    {
        logError("CRaRTSProxy::openStreaming Channel count exceeded");
        return am_Error_e::E_OUT_OF_RANGE;
    }

    for (unsigned int device = 0; device < maxDevices; device++)
    {
        mAdevs[device] = mAdevs[SRC_DEVICE];
        /* configure capture and playback rotationally ... */
        mAdevs[device].dir = (!(device % 2)) ? SND_PCM_STREAM_CAPTURE : SND_PCM_STREAM_PLAYBACK;
        /* ... and set source name for device[0].dir capture and device[3].dir playback */
        mAdevs[device].pcmname = (!(device % 3)) ? mProxy.pcmSrc.c_str() : mProxy.pcmSink.c_str();

        for (unsigned int channel = 0; channel < mProxy.channels; channel++)
        {
            /* configure the stream information */
            mStreams[mCfg.num_streams].adevidx = device;
            mStreams[mCfg.num_streams].channel = channel;

            /* allocate exchange buffer */
            if (mAdevs[device].dir == SND_PCM_STREAM_CAPTURE)
            {
                mCopyBuffers[offset] = (char*)new char[frameSize * formatSize];
                if (mCopyBuffers[offset] == NULL)
                {
                    logError("CRaRTSProxy::openStreaming Data exchange memory can't be allocated");
                    return am_Error_e::E_NOT_POSSIBLE;
                }
                offset++;
            }
            logInfo("CRaRTSProxy::openStreaming", "pcm_name:", mAdevs[device].pcmname, "device_id:", mStreams[mCfg.num_streams].adevidx, "stream_channel:", mStreams[mCfg.num_streams].channel);
            mCfg.num_streams++;
        }
    }

    mCfg.num_adevs = maxDevices;
    mCfg.adevs = mAdevs;
    mCfg.streams = mStreams;
    mCfg.prefill_ms = mProxy.msPrefill;
    mCfg.features = 0;

    return am_Error_e::E_OK;
}

am_Error_e CRaRTSProxy::startStreaming(void)
{
    string tname = getThreadName();
    int err = CThread::startThread();
    if (err)
    {
        logError("CRaRTSProxy::startStreaming", "startThread failed", strerror(err));
        return am_Error_e::E_UNKNOWN;
    }
    CThread::setThreadName(tname);
    logInfo("CRaRTSProxy::startStreaming", "started thread :", tname);

    return am_Error_e::E_OK;
}

am_Error_e CRaRTSProxy::stopStreaming(void)
{
    int err = CThread::stopThread();

    if (err)
    {
        logError("CRaRTSProxy::stopStreaming", "stopThread failed", strerror(err));
        return am_Error_e::E_UNKNOWN;
    }
    return am_Error_e::E_OK;
}

am_Error_e CRaRTSProxy::closeStreaming(void)
{
    int err = rts_abort(mRts);
    if (err)
    {
        logWarning("CRaRTSProxy::closeStreaming", "rts abort failed", STRERROR(err));
    }
    err = CThread::joinThread();
    if (err)
    {
        if (err == -ECANCELED)
        {
            logInfo("CRaRTSProxy::closeStreaming", "joinThread cancelled");
            return am_Error_e::E_OK;
        } else {
            logError("CRaRTSProxy::closeStreaming", "joinThread failed", STRERROR(err));
            return am_Error_e::E_UNKNOWN;
        }
    }
    freeCopyBuffers();

    return am_Error_e::E_OK;
}

void CRaRTSProxy::freeCopyBuffers(void)
{
    for (unsigned int channel = 0; channel < (mCfg.num_streams / 2); channel++)
    {
        delete [] mCopyBuffers[channel];
        mCopyBuffers[channel] = nullptr;
    }
}

int CRaRTSProxy::getFormatSize(void) const
{
    int err = snd_pcm_format_size(static_cast<snd_pcm_format_t>(mProxy.format), 1);
    if (err < 0)
    {
        logError("CRaRTSProxy::getFormatSize Format", mProxy.format, "is unsupported");
    }

    return err;
}

am_timeSync_t CRaRTSProxy::getDelay(void) const
{
    return static_cast<am_timeSync_t>(mProxy.msPrefill + mProxy.msBuffersize);
}
