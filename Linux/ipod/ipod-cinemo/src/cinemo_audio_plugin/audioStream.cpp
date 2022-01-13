/**
 * \file: AudioStream.cpp
 *
 * \version: $Id:$
 *
 * \release: $Name:$
 *
 * cinemo audio streaming using backend ALSA API's
 *
 * \component: ipod cinemo
 *
 * \author: Vanitha.channaiah@in.bosch.com
 *
 * \copyright (c) 2019 Advanced Driver Information Technology.
 * This code is developed by Advanced Driver Information Technology.
 * Copyright of Advanced Driver Information Technology, Bosch, and DENSO.
 * All rights reserved.
 *
 * \see <related items>
 *
 * \history
 *
 ***********************************************************************/
#include "AudioStreaming.h"
#include "AudioFactory.h"
#include "AudioTypes.h"
#include "audioStream.h"
#include "iap2_cinemo_host_datacom.h"

using namespace std;
using namespace adit::utility;
using namespace adit::utility::audio;

ApplicationStreaming *streamingHandle = nullptr;

/* In error case, logging is handled
 * error is not returned from cinemo API's
 *
 * TO DO: Handle error
 **/
bool audio_lock(audioHandle *phandle)
{
    int err = -1;
    bool ret = false;
    if (phandle != NULL)
    {
        err = pthread_mutex_lock(&phandle->audioMutex);
        if (err == 0)
        {
            ret = true;
        }
        else
        {
            IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "CINEMO_AUD: pthread_mutex_lock() failed with err %d", err);
        }
    }
    else
    {
        IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "CINEMO_AUD: audioHandle is NULL");
    }
    return ret;
}

void audio_unLock(audioHandle *phandle)
{
    int err = -1;
    if (phandle != NULL)
    {
        err = pthread_mutex_unlock(&phandle->audioMutex);
        if (err != 0)
        {
            IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "CINEMO_AUD: pthread_mutex_unlock() failed with err %d", err);
        }
    }
    else
    {
        IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "CINEMO_AUD: audioHandle is NULL");
    }
}

ApplicationStreaming::ApplicationStreaming(void)
{
    mCinemoHandle = nullptr;
    mStream = nullptr;
}

ApplicationStreaming::~ApplicationStreaming(void)
{

}

void ApplicationStreaming::error(const string& data) const
{
    IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "CINEMO_AUD:%s", data.c_str());
}

void ApplicationStreaming::warning(const string& data) const
{
    IAP2USBPLUGINDLTLOG(DLT_LOG_WARN, "CINEMO_AUD:%s", data.c_str());
}

void ApplicationStreaming::info(const string& data) const
{
    IAP2USBPLUGINDLTLOG(DLT_LOG_INFO, "CINEMO_AUD:%s", data.c_str());
}

void ApplicationStreaming::debug(const string& data) const
{
    IAP2USBPLUGINDLTLOG(DLT_LOG_DEBUG, "CINEMO_AUD:%s", data.c_str());
}

/*
void ApplicationStreaming::verbose(const string& data) const
{
    IAP2USBPLUGINDLTLOG(DLT_LOG_VERBOSE, "CINEMO_AUD:%s", data.c_str());
}
*/

eLogLevel ApplicationStreaming::checkLogLevel() const
{
#ifdef DEBUG
    return LL_DEBUG;
#else
    return LL_ERROR;
#endif
}

void ApplicationStreaming::statistics(const StreamStatistics& status)
{
    (void)status;
}

void ApplicationStreaming::eostreaming(const AudioError error)
{
    const uint64_t eos = 1;
    int ret = 0;
    if (error != AudioError::OK)
    {
        IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "eostreaming: Reached EOS with erro %d", (uint32_t)error);
        updateAudioState(mCinemoHandle, EOS);
        ret = write(mCinemoHandle->eosFd, &eos, sizeof(uint64_t));
        if (ret != sizeof(uint64_t))
        {
            IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "eostreaming event write FAILED\n");
        }
    }
}

void ApplicationStreaming::setApplicationHandle(ctli_handle tphandle)
{
    IPOD_IAP2_CINEMO_HOSTDEV_INFO *devinfo = (IPOD_IAP2_CINEMO_HOSTDEV_INFO *) tphandle;
    if (!mCinemoHandle)
    {
        mCinemoHandle = (audioHandle *)devinfo->audioctx;
    }
    IAP2USBPLUGINDLTLOG(DLT_LOG_DEBUG, "CINEMO_AUD: setApplicationHandle:: handle:%p ", mCinemoHandle);
}

AudioState ApplicationStreaming::processing(unsigned char *in, unsigned char **out, uint32_t &frames)
{
    (void)out;
    const uint64_t proc = 1;
    int err = 0;

    if (in == nullptr)
    {
        IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "CINEMO_AUD: processing capture mStream is NULL : ABORT");
        return AudioState::ABORT;
    }

    if (mCinemoHandle)
    {
        if (mCinemoHandle->aborting == true)
        {
            return AudioState::ABORT;
        }

        if (audio_lock(mCinemoHandle))
        {
            if (mCinemoHandle->audioState != STOPPED)
            {
                const uint32_t BytesPerSample = getBitsPerSample(mCinemoHandle->dev->audioParams.sampletype) / 8;
                uint32_t dataBytes = frames * (BytesPerSample) * mCinemoHandle->dev->audioParams.channels;
#if DEBUG
                printBufferStatus(mCinemoHandle->cbuf);
#endif
                bufferPushData(mCinemoHandle->cbuf, in, dataBytes);
                err = write(mCinemoHandle->processFd, &proc, sizeof(uint64_t));
                if (err != sizeof(uint64_t))
                {
                    IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "CINEMO_AUD: write ret =  %d | errno: %d %s", err, errno, strerror(errno));
                }
            }
            audio_unLock(mCinemoHandle);
        }

    }

    return AudioState::CONTINUE;
}

int audioBackend_create(ctli_handle tphandle)
{
    int ret = 0;
    string backendLibName;
    backendLibName.assign(BACKEND_LIB_NAME);
    streamingHandle = new ApplicationStreaming;
    if (streamingHandle != nullptr)
    {
        streamingHandle->mStream = Factory::Instance()->createBackend(backendLibName, *streamingHandle);
        streamingHandle->setApplicationHandle(tphandle);
        if (streamingHandle->mStream == nullptr)
        {
            IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "CINEMO_AUD:Backend Object creation Failed");
            ret = -1;
        }

        IAP2USBPLUGINDLTLOG(DLT_LOG_DEBUG, "CINEMO_AUD:Selected Backend Name : %s ",streamingHandle->mStream->getBackendName());
    }
    else
    {
        IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "CINEMO_AUD:Streaming handle is null to create Audiobackend");
        ret = -1;
    }
    return ret;
}

int audioBackend_open(deviceParams *dev)
{
    int ret = 0;
    AudioError err = AudioError::OK;
    AudioFormat audioFormat;

    if (dev == NULL)
    {
        return -1;
    }
    switch (dev->audioParams.sampletype)
    {
        case CTLI_AUDIO_SAMPLETYPE_S16LE :
        {
            audioFormat = S16_LE;
            break;
        }
        case CTLI_AUDIO_SAMPLETYPE_S24LE :
        {
            audioFormat = S24_LE;
            break;
        }
        default :
            IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "CINEMO_AUD:Cinemo Audio does not support %d sampletype", dev->audioParams.sampletype);
            ret = -1;
    }
    if (streamingHandle != nullptr)
    {
        streamingHandle->mStream->setInitialTimeout(-1);

        err = streamingHandle->mStream->openStream(dev->captureDevice, dev->playbackDevice,
                            audioFormat,
                            dev->audioParams.samplerate,
                            dev->audioParams.channels,
                            dev->periodFrames);
        if (err != AudioError::OK)
        {
            IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "CINEMO_AUD:OPEN mStream FAILED, err = %d", static_cast<uint32_t>(err));
            ret = -1;
        }

        IAP2USBPLUGINDLTLOG(DLT_LOG_DEBUG, "CINEMO_AUD: audioBackend_open() success with dev:%s \n rate: %d\n bits: %d\n "
                "channels: %d\n periods: %d",
                dev->captureDevice,
                dev->audioParams.samplerate,
                dev->audioParams.bits,
                dev->audioParams.channels,
                dev->periodFrames);
    }
    else
    {
        IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "CINEMO_AUD:Audio stream not opened as streaming handle is null");
        ret = -1;
    }
    return ret;
}

int audioBackend_stream()
{
    int ret = 0;
    AudioError err = AudioError::OK;

    if (streamingHandle != nullptr)
    {
        err = streamingHandle->mStream->startStream();
        if (err != AudioError::OK)
        {
            IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "CINEMO_AUD:audioBackend_stream FAILED");
            err = streamingHandle->mStream->stopStream();
            if (err != AudioError::OK)
            {
                ret = -1;
            }

            err = streamingHandle->mStream->closeStream();
            if (err != AudioError::OK)
            {
                ret = -1;
            }
            ret = -1;
        }

        IAP2USBPLUGINDLTLOG(DLT_LOG_DEBUG, "CINEMO_AUD: audioBackend_stream successfully started the streaming");
    }
    else
    {
        IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "CINEMO_AUD:Audio stream not started as streaming handle is null");
        ret = -1;
    }
    return ret;
}

int audioBackend_abort()
{
    int ret = 0;
    AudioError err = AudioError::OK;

    if (streamingHandle != nullptr)
    {
        err = streamingHandle->mStream->abortStream();
        if (err != AudioError::OK)
        {
            IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "CINEMO_AUD:abortStream FAILED");
            ret = -1;
        }

        err = streamingHandle->mStream->stopStream();
        if (err != AudioError::OK)
        {
            IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "CINEMO_AUD:stopStream FAILED");
            ret = -1;
        }

        IAP2USBPLUGINDLTLOG(DLT_LOG_DEBUG, "CINEMO_AUD: audioBackend_abort successfully stopped the streaming");
    }
    else
    {
        IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "CINEMO_AUD:Audio stream not aborted as streaming handle is null");
        ret = -1;
    }
    return ret;
}

int audioBackend_close()
{
    int ret = 0;
    AudioError err = AudioError::OK;

    if (streamingHandle != nullptr)
    {
        err = streamingHandle->mStream->closeStream();
        if (err != AudioError::OK)
        {
            ret = -1;
        }

        IAP2USBPLUGINDLTLOG(DLT_LOG_DEBUG, "CINEMO_AUD: audioBackend_close successfully closed the streaming device");
    }
    else
    {
        IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "CINEMO_AUD:Audio stream not closed as streaming handle is null");
        ret = -1;
    }
    return ret;
}

void audioBackend_delete()
{
    delete streamingHandle;
    streamingHandle = nullptr;
}

