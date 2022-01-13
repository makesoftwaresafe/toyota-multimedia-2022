/************************************************************************
 * @file: AudioAlsa.cpp
 *
 * @version: 1.0
 *
 * @description: This source file contains class implementation of BackendAlsa.
 * A wrapper class for Alsa State machine and Backend Alsa Implementation.
 * BackendAlsa will call the Backend Alsa Implementation API's for Open/Close,
 * Configuring ,Write/Read to Alsa Device and Alsa State machine API's for
 * handling Stream states.
 *
 * @authors: Jens Lorenz, jlorenz@de.adit-jv.com 2015
 *           Thouseef Ahamed, tahamed@de.adit-jv.com) 2015
 *           Vijay Palaniswamy, vijay.palaniswamy@in.bosch.com 2015
 *
 * @copyright (c) 2015 Advanced Driver Information Technology.
 * This code is developed by Advanced Driver Information Technology.
 * Copyright of Advanced Driver Information Technology, Bosch, and DENSO.
 * All rights reserved.
 *
 ***********************************************************************/

#include <cassert>
#include <errno.h>
#include <cstdlib>

#include "AudioAlsa.h"

using namespace std;
using namespace adit::utility::audio;

#define _assert(cond) do{int32_t _cond_res = (cond); assert(_cond_res); _cond_res = _cond_res;}while(0)

extern "C" Backend* AlsaBeEntry(Streaming& streamingHandle)
{
    return new BackendAlsa(streamingHandle);
}

BackendAlsa::BackendAlsa(Streaming& streamingHandle) : Backend(), mpStateMachine(nullptr),
                         mpBackendAlsaImpl(nullptr), mpSilenceDetector(nullptr), mLoggingHandle(streamingHandle)
{
    /* Creating object for State Machine and Backend Alsa Implementation */
    parseEnvAndCreateObject(streamingHandle);
    mpStateMachine    = new StateMachine(mpBackendAlsaImpl, streamingHandle);

    /* Sending state machine handle for Backend Alsa Implementation*/
    mpBackendAlsaImpl->setStateMachineHandle(mpStateMachine);

    _assert(pthread_mutex_init(&mApiMtx, nullptr) == 0);

    Logging(mLoggingHandle, LL_INFO) << "BackendAlsa ENTRY adr: " << this << Logging::endl;
}

BackendAlsa::~BackendAlsa()
{
    /* Deleting Backend Alsa Implementation object */
    if (mpBackendAlsaImpl)
    {
        mpBackendAlsaImpl->disablePcmDump();
        delete mpBackendAlsaImpl;
    }

    /* Deleting state machine object */
    if (mpStateMachine)
    {
        delete mpStateMachine;
    }

    /* Deleting Silence detector object */
    if (mpSilenceDetector)
    {
        delete mpSilenceDetector;
    }

    /* Destroying the Mutex */
    pthread_mutex_destroy(&mApiMtx);
}

void BackendAlsa::apiLock()
{
    _assert(pthread_mutex_lock(&mApiMtx) == 0);
}

void BackendAlsa::apiUnLock()
{
    _assert(pthread_mutex_unlock(&mApiMtx) == 0);
}

const char* BackendAlsa::getBackendName()
{
    return mpBackendAlsaImpl->getBackendName();
}

AudioError BackendAlsa::openStream(const string& inCard, const string& outCard,
    const AudioFormat format, const uint32_t rate, const uint32_t channels, uint32_t& periodFrames)
{
    AudioError ret = AudioError::OK;

    apiLock();

    StreamInfo initData(inCard, outCard, format, rate, channels, periodFrames);

    ret = mpStateMachine->openEvent(initData);

    if ((ret == AudioError::OK) && (mpSilenceDetector != nullptr))
    {
        silenceDetectConfigure(initData);
    }
    Logging(mLoggingHandle, audioErrorToLogLevel(ret)) << "BackendAlsa::openStream, ret = " << ret << Logging::endl;

    apiUnLock();

    return ret;
}

AudioError BackendAlsa::closeStream()
{
    AudioError ret = AudioError::OK;

    apiLock();

    ret = mpStateMachine->closeEvent();

    if (mpSilenceDetector)
    {
        mpSilenceDetector->reset();
    }
    Logging(mLoggingHandle, audioErrorToLogLevel(ret)) << "BackendAlsa::closeStream, ret = " << ret << Logging::endl;

    apiUnLock();

    return ret;

}

AudioError BackendAlsa::startStream()
{
    AudioError ret = AudioError::OK;

    apiLock();

    ret = mpStateMachine->startEvent();

    if (mpSilenceDetector)
    {
        mpSilenceDetector->activate();
    }
    Logging(mLoggingHandle, audioErrorToLogLevel(ret)) << "BackendAlsa::startStream, ret = " << ret << Logging::endl;

    apiUnLock();

    return ret;
}

AudioError BackendAlsa::stopStream()
{
    AudioError ret = AudioError::OK;

    apiLock();
    ret = mpStateMachine->stopEvent();
    Logging(mLoggingHandle, audioErrorToLogLevel(ret)) << "BackendAlsa::stopStream, ret = " << ret << Logging::endl;
    apiUnLock();

    return ret;
}

AudioError BackendAlsa::abortStream()
{
    AudioError ret = AudioError::OK;

    apiLock();
    ret = mpStateMachine->abortEvent();
    Logging(mLoggingHandle, audioErrorToLogLevel(ret)) << "BackendAlsa::abortStream, ret = " << ret << Logging::endl;
    apiUnLock();

    return ret;

}

uint64_t BackendAlsa::getStreamTimeInFrames()
{
    return mpBackendAlsaImpl->getStreamTimeInFrames();
}

uint64_t BackendAlsa::getStreamLatencyInFrames()
{
    return mpBackendAlsaImpl->getStreamLatencyInFrames();
}

void BackendAlsa::setThreadSched(int policy, int priority)
{
    Logging(mLoggingHandle, LL_INFO) << "BackendAIsa::setThreadSched policy : " << policy << " ,priority " << priority << Logging::endl;
    mpBackendAlsaImpl->setThreadSched(policy, priority);
}

AudioError BackendAlsa::setFadeTime(const enum FadeMode fadeMode, const enum StreamDirection streamDir, const uint32_t fadeTime)
{
    Logging(mLoggingHandle, LL_INFO) << "BackendAIsa::setFadeTime Fade Mode : " << fadeMode
                                     << " ,Stream Direction : " << streamDir << " ,Fade Time : " << fadeTime << Logging::endl;
    return mpBackendAlsaImpl->setFadeTime(fadeMode, streamDir, fadeTime);
}

void BackendAlsa::setInitialTimeout(const uint32_t timeout)
{
    Logging(mLoggingHandle, LL_INFO) << "BackendAIsa::setInitialTimeout Timeout : " << timeout << Logging::endl;
    mpBackendAlsaImpl->setInitialTimeout(timeout);
}

uint32_t BackendAlsa::getInitialTimeout() const
{
    return mpBackendAlsaImpl->getInitialTimeout();
}

AudioError BackendAlsa::getFadeTime(const enum FadeMode fadeMode, const enum StreamDirection streamDir, uint32_t& fadeTime)
{
    return mpBackendAlsaImpl->getFadeTime(fadeMode, streamDir, fadeTime);
}

adit::utility::eLogLevel BackendAlsa::audioErrorToLogLevel(AudioError err)
{
    eLogLevel level = LL_INFO;
    switch (err)
    {
        case AudioError::UNSUPPORTED:
        case AudioError::INVALID:
            level = LL_WARNING;
            break;
        case AudioError::NOMEM:
        case AudioError::BUSY:
        case AudioError::FAILURE:
            level = LL_ERROR;
            break;
        default:
            break;
    }
    return level;
}

void BackendAlsa::parseEnvAndCreateObject(Streaming& streamingHandle)
{
    /* Reading environmental variable for PCM dump
     * ALSABE_ENABLE_IN_PCM_DUMP=<Path>
     * Eg for enabling pcm dump for capture data:
     *   "ALSABE_ENABLE_IN_PCM_DUMP=/tmp/in.pcm"
     *
     * To enable Silence detector:
     *   ALSABE_ENABLE_SILENCE_DETECTOR=1
     */

    char *pcm_dump_path_in  = getenv("ALSABE_ENABLE_IN_PCM_DUMP");
    char *pcm_dump_path_out = getenv("ALSABE_ENABLE_OUT_PCM_DUMP");
    char *sd_enable         = getenv("ALSABE_ENABLE_SILENCE_DETECTOR");

    if (sd_enable != nullptr)
    {
        if (atoi(sd_enable) == 1)
        {
            mpSilenceDetector = new SilenceDetector(streamingHandle);
        }
    }

    if (mpSilenceDetector)
    {
        mpBackendAlsaImpl = new BackendAlsaImpl(*mpSilenceDetector);
    }
    else
    {
        mpBackendAlsaImpl = new BackendAlsaImpl(streamingHandle);
    }

    if (pcm_dump_path_in != nullptr)
    {
        mpBackendAlsaImpl->enablePcmDump(StreamDirection::IN, pcm_dump_path_in);
    }
    if (pcm_dump_path_out != nullptr)
    {
        mpBackendAlsaImpl->enablePcmDump(StreamDirection::OUT, pcm_dump_path_out);
    }
}

void BackendAlsa::silenceDetectConfigure(const StreamInfo& streamInfo)
{
    SdStreamInfo streaminfo;
    streaminfo.channels = streamInfo.channels;
    streaminfo.format   = streamInfo.format;
    streaminfo.frames   = streamInfo.periodFrames;
    streaminfo.rate     = streamInfo.rate;

    mpSilenceDetector->configure(streaminfo);
}
