/************************************************************************
 * @file: AudioAlsaImpl.cpp
 *
 * @version: 1.0
 *
 * @description: This source file contains class implementation of BackendAlsaImpl.
 * A wrapper class for Alsa API's. BackendAlsaImpl API's will be called by
 * Alsa State Machine API's . BackendAlsaImpl will call the Alsa API's for
 * Open/Close, Configuring,Write/Read to Alsa Device.
 *
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

#include "AudioAlsaImpl.h"
#include "Logging.h"
#include "AudioHelper.h"
#include "AudioAlsa.h"

using namespace std;
using namespace adit::utility::audio;

#ifndef min
#define min(a,b) ((a)<(b)?(a):(b))
#endif

#define DEFAULT_RUNTIME_WAIT_TOUT        20  /* Runtime snd_pcm_wait time out in Milliseconds*/
#define DEFAULT_INITTIME_WAIT_TOUT     1000  /* Initial snd_pcm_wait time out in Milliseconds */
#define MAX_RETRY_COUNT                   5  /* Max re-try count for Alsa xRun Handling*/
#define NO_OF_PERIODS_ALLOC               1  /* No of periods to be allocated */
#define AUDIO_ACCESS_TYPES                2  /* No of Alsa Access [Read/Write] types used */
#define DEFAULT_FADEOUT_TIME              8  /* Default fade out time */

BackendAlsaImpl::BackendAlsaImpl(Streaming& streamingHandle) : CThread(),
                                 mFaderInDirIn(streamingHandle), mFaderOutDirOut(streamingHandle),
                                 mStreamingHandle(streamingHandle)

{
    mDevice.push_back(AlsaDevice(SND_PCM_STREAM_CAPTURE));
    mDevice.push_back(AlsaDevice(SND_PCM_STREAM_PLAYBACK));

    Logging(mStreamingHandle, LL_INFO) << "BackendAlsaImpl ENTRY adr: " << this << Logging::endl;

    mCapturePcmFp  = nullptr;
    mPlaybackPcmFp = nullptr;
    mFadeInterBuf  = nullptr;
    mSndOutput     = nullptr;
    mSndPcmstatus  = nullptr;
    mInitalTimeOut = DEFAULT_INITTIME_WAIT_TOUT;

    /* Setting Default fade out time, If application is not setting fade out time for playback stream */
    mFaderOutDirOut.setFadeDirectionAndTime(FaderDir::OUT, DEFAULT_FADEOUT_TIME);
}

BackendAlsaImpl::~BackendAlsaImpl()
{
    if (mpStateMachine->getState() != ALSA_SSTATE_CLOSED)
    {
        /*Do Abort stream and close the stream */
        mpStateMachine->abortEvent();
        mpStateMachine->closeEvent();
    }

    if (mCapturePcmFp)
        fclose(mCapturePcmFp);

    if (mPlaybackPcmFp)
        fclose(mPlaybackPcmFp);
}

void BackendAlsaImpl::setStateMachineHandle(StateMachine* stateMachine)
{
    mpStateMachine = stateMachine;
}

void BackendAlsaImpl::enablePcmDump(const enum StreamDirection streamDir, char* dumpPath)
{
    if (mCapturePcmFp == nullptr && streamDir == StreamDirection::IN)
    {
        mCapturePcmFp  = fopen(dumpPath ,"ab+");
        if (mCapturePcmFp == nullptr)
        {
            Logging(mStreamingHandle, LL_ERROR) << "BackendAlsaImpl::enablePcmDump Capture PCM file open failed" << Logging::endl;
        }
    }

    if (mPlaybackPcmFp == nullptr && streamDir == StreamDirection::OUT)
    {
        mPlaybackPcmFp = fopen(dumpPath ,"ab+");
        if (mPlaybackPcmFp == nullptr)
        {
            Logging(mStreamingHandle, LL_ERROR) << "BackendAlsaImpl::enablePcmDump Playback PCM file open failed" << Logging::endl;
        }
    }
}

void BackendAlsaImpl::disablePcmDump()
{
    if (mCapturePcmFp)
        fclose(mCapturePcmFp);
    mCapturePcmFp = nullptr;

    if (mPlaybackPcmFp)
        fclose(mPlaybackPcmFp);
    mPlaybackPcmFp = nullptr;
}

AudioError BackendAlsaImpl::setFadeTime(const enum FadeMode fadeMode, const enum StreamDirection streamDir, const uint32_t fadeTime)
{
    AudioError ret = AudioError::OK;
    if(streamDir == StreamDirection::IN)
    {
        if (fadeMode == FadeMode::IN)
        {
            FaderError err = mFaderInDirIn.setFadeDirectionAndTime(FaderDir::IN, fadeTime);
            if (err != FaderError::OK)
            {
                ret = AudioError::UNSUPPORTED;
            }
        }
        else if (fadeMode == FadeMode::OUT)
        {
            /* Currently FadeOut for capture not supported */
            ret = AudioError::UNSUPPORTED;
        }
    }
    else if(streamDir == StreamDirection::OUT)
    {
        if (fadeMode == FadeMode::IN)
        {
            /* Currently FadeIn for Playback not supported */
            ret = AudioError::UNSUPPORTED;
        }
        else if (fadeMode == FadeMode::OUT)
        {
            FaderError err = mFaderOutDirOut.setFadeDirectionAndTime(FaderDir::OUT, fadeTime);
            if (err != FaderError::OK)
            {
                ret = AudioError::UNSUPPORTED;
            }
        }
    }

    return ret;
}

AudioError BackendAlsaImpl::getFadeTime(const enum FadeMode fadeMode, const enum StreamDirection streamDir, uint32_t& fadeTime)
{
    AudioError ret = AudioError::OK;
    if(streamDir == StreamDirection::IN)
    {
        if (fadeMode == FadeMode::IN)
        {
            FaderError err = mFaderInDirIn.getFadeTime(fadeTime);
            if (err != FaderError::OK)
            {
                ret = AudioError::UNSUPPORTED;
            }
        }
        else if (fadeMode == FadeMode::OUT)
        {
            /* Currently FadeOut for capture not supported */
            ret = AudioError::UNSUPPORTED;
        }
    }
    else if(streamDir == StreamDirection::OUT)
    {
        if (fadeMode == FadeMode::IN)
        {
            /* Currently FadeIn for Playback not supported */
            ret = AudioError::UNSUPPORTED;
        }
        else if (fadeMode == FadeMode::OUT)
        {
            FaderError err = mFaderOutDirOut.getFadeTime(fadeTime);
            if (err != FaderError::OK)
            {
                ret = AudioError::UNSUPPORTED;
            }
        }
    }

    return ret;
}

uint64_t BackendAlsaImpl::getStreamTimeInFrames()
{
    uint64_t elapsedFrames = 0;

    /*
     * Calculation for Frame Per Millisecond and Time Elapsed.
     * 1. bytes per second = (Rate x Number of Channels x Sample)
     *
     * 2. frames Per second = bytes per second / (Number of channels x Sample)
     *                      = (Rate x Number of Channels x Sample) / (Number of Channels x Sample)
     *                      = Rate
     *
     * 3. frames per millisecond = frames per second / 1000
     *                           = Rate / 1000
     *
     * 4. time elapsed ( in frames ) = Period size * Total Periods consumed
     *                               = time elapsed ( in frames ) + If any remaining frames
     *
     */

    elapsedFrames = static_cast<uint64_t>(mPerSize) * mStreamTimeInfo.countRwPeriods;

    if (mStreamTimeInfo.countRwFrames > 0)
    {
        elapsedFrames += static_cast<uint64_t>(mStreamTimeInfo.countRwFrames);
    }

    return elapsedFrames;
}

uint64_t BackendAlsaImpl::getStreamLatencyInFrames()
{
    snd_pcm_sframes_t Latency   = 0;
    uint64_t totalLatencyFrames = 0;
    int ret = 0;

    vector<AlsaDevice>::iterator itr = mDevice.begin();
    for (; itr != mDevice.end(); ++itr)
    {
        Latency = 0;
        if (itr->isConfigured())
        {
            ret = snd_pcm_delay(itr->hndl, &Latency);
            if (ret < 0)
                Latency = 0;
        }
        totalLatencyFrames += Latency;
    }

    return totalLatencyFrames;
}

void BackendAlsaImpl::setThreadSched(int policy, int priority)
{
    CThread::setThreadSched(policy, priority);
}

const char* BackendAlsaImpl::getBackendName()
{
    return "Alsa";
}

void BackendAlsaImpl::setInitialTimeout(const uint32_t timeout)
{
    mInitalTimeOut = timeout;
}

uint32_t BackendAlsaImpl::getInitialTimeout() const
{
    return mInitalTimeOut;
}

AudioError BackendAlsaImpl::doOpenStream(StreamInfo& initData)
{
    AudioError err  = AudioError::OK;
    mFormat         = initData.format;
    mRate           = initData.rate;
    mChannels       = initData.channels;
    mPerSize        = static_cast <snd_pcm_uframes_t>(initData.periodFrames);
    mBytesPerSample = getBytesPerSample(mFormat) * mChannels;

    mInDevice.name  = initData.inCard;
    mOutDevice.name = initData.outCard;

    if (mInDevice.name.empty() && mOutDevice.name.empty())
    {
        Logging(mStreamingHandle, LL_ERROR) << "BackendAlsaImpl::doOpenStream All Card/Device names are empty" << Logging::endl;
        return AudioError::FAILURE;
    }

    mDuplex = (!mInDevice.name.empty() & !mOutDevice.name.empty());

    Logging(mStreamingHandle, LL_DEBUG) << "BackendAlsaImpl::doOpenStream Duplex Mode " << mDuplex << Logging::endl;

    int32_t threadRet = CThread::startThread();
    if (threadRet != 0)
    {
        Logging(mStreamingHandle, LL_ERROR) << "BackendAlsaImpl::doOpenStream start worker thread failed, err " << threadRet << Logging::endl;
        return AudioError::FAILURE;
    }

    string tname = threadName();
    CThread::setThreadName(tname);
    Logging(mStreamingHandle, LL_DEBUG) << "BackendAlsaImpl::doOpenStream started worker thread... " << tname << Logging::endl;

    mPerTime = convertFramesToMs(mRate, mPerSize);

    /* To reset the stream time */
    streamTimeReset();

    /* To reset the stream statistics */
    resetStreamStatistics();

    if (err != AudioError::OK)
    {
        cleanup();
        Logging(mStreamingHandle, LL_ERROR) << "BackendAlsaImpl::doOpenStream failed with err " << err << Logging::endl;
    }
    else
    {
        initData.periodFrames = static_cast <uint32_t>(mPerSize);
    }

    return err;
}

void BackendAlsaImpl::resetDevicesInfo(AlsaDevice& device)
{
    device.periods           = PERIODS_DEFAULT;
    device.hwBufferSize      = 0;
    device.delay             = 0;
    device.alsarw            = nullptr;
    device.startupFrameCount = 0;
    device.wait_done         = 0;
    device.name.assign("");
    mInitalTimeOut           = DEFAULT_INITTIME_WAIT_TOUT;
    mDuplex                  = 0;
    mPerSize                 = 0;
}

int BackendAlsaImpl::initThread()
{
    AudioError err = AudioError::OK;

    err = allocAlsaDumpMemory();
    if (err != AudioError::OK)
    {
        Logging(mStreamingHandle, LL_ERROR) << "BackendAlsaImpl::initThread allocAlsaDumpMemory" << Logging::endl;
        return static_cast<int>(AudioState::ABORT);
    }

    /*configure in fixed order: INPUT, OUTPUT*/
    vector<AlsaDevice>::iterator itr = mDevice.begin();
    for (; itr != mDevice.end(); ++itr)
    {
        err = openAlsaDevice(*itr);
        if (err == AudioError::UNSUPPORTED)
        {
            err = AudioError::OK;
            continue;
        }

        /* If open alsa device failed */
        if (err != AudioError::OK)
        {
            break;
        }

        err = configureAlsaDevice(*itr);
        if (err != AudioError::OK)
        {
            break;
        }

        err = fixupAlsaDevice(*itr);
        if (err != AudioError::OK)
        {
            break;
        }

        /* Dumping Alsa configure details */
        dumpAlsaDeviceInfo(*itr, AlsaInfo::CONFIG);
    }

    if (err == AudioError::OK)
    {
        err = faderConfigure();
    }

    if (err == AudioError::OK)
    {
        mpStateMachine->workerEvent(WORKER_EVENT_STOP);
        return static_cast<int>(AudioState::CONTINUE);
    }

    Logging(mStreamingHandle, LL_ERROR) << "BackendAlsaImpl::initThread device open/configure, err " << err << Logging::endl;
    return static_cast<int>(AudioState::ABORT);
}

int BackendAlsaImpl::workerThread()
{
    int32_t ret            = 0;
    AudioState cret        = AudioState::CONTINUE;
    AudioError audioret    = AudioError::OK;
    WorkerEvent event      = WORKER_EVENT_UNKOWN;
    uint32_t frames        = 0;
    unsigned char* tempBuf = nullptr;
    mpStateMachine->waitTillStateChange(ALSA_SSTATE_STOP);

    if (mpStateMachine->getState() == ALSA_SSTATE_CLOSING)
    {
        return static_cast<int>(AudioState::STOP);
    }

    /* state is running / fading */
    mOutDevice.buf = nullptr;
    frames = mPerSize;
    ret = readWriteAlsaDevice(mInDevice, frames);
    if (ret < 0)
    {
        Logging(mStreamingHandle, LL_ERROR) << "BackendAlsaImpl::workerThread AlsaRead failed with err " << ret << " so aborting stream" << Logging::endl;
        audioret = AudioError::FAILURE;
        mpStateMachine->workerEvent(WORKER_EVENT_ABORTING);
        workerThreadExitHandling(WORKER_EVENT_ABORT, audioret);
        return static_cast<int>(AudioState::CONTINUE);
    }

    if (mCapturePcmFp && mInDevice.buf)
    {
        fwrite(mInDevice.buf, (ret * mBytesPerSample), 1, mCapturePcmFp);
    }

    frames = ret;
    tempBuf = mInDevice.buf;
    if (mpStateMachine->getState() == ALSA_SSTATE_RUN_FADE_IN)
    {
        FaderError fade_ret = mFaderInDirIn.process(mInDevice.buf, frames, mFadeInterBuf);
        if (fade_ret != FaderError::OK)
        {
            Logging(mStreamingHandle, LL_DEBUG) << "BackendAlsaImpl::workerThread FadeIn process completed with, err " << fade_ret << " So changing state to Running" << Logging::endl;
            mpStateMachine->workerEvent(WORKER_EVENT_RUNNING);
        }
        /* FaderError::OK   : Fading in Process
         * FaderError::DONE : Fading Completed
         */

        if ((fade_ret == FaderError::OK) || (fade_ret == FaderError::DONE))
        {
            tempBuf = mFadeInterBuf ;
        }
    }

    cret = mStreamingHandle.processing(tempBuf, &mOutDevice.buf, frames);

    if (cret == AudioState::ABORT)
    {
        Logging(mStreamingHandle, LL_INFO) << "BackendAlsaImpl::workerThread ABORT from process callback" << Logging::endl;
        mpStateMachine->workerEvent(WORKER_EVENT_ABORTING);
        workerThreadExitHandling(WORKER_EVENT_ABORT, audioret);
        return static_cast<int>(AudioState::CONTINUE);
    }

    if (cret == AudioState::STOP)
    {
        Logging(mStreamingHandle, LL_INFO) << "BackendAlsaImpl::workerThread STOP from process callback" << Logging::endl;
        mpStateMachine->workerEvent(WORKER_EVENT_FADE_OUT);
    }

    if (mpStateMachine->getState() == ALSA_SSTATE_RUN_FADE_OUT)
    {
        FaderError fade_ret = FaderError::FAILURE;

        /* Re-alloc fader Intermediate buffer if frames are more than buffer size */
        if (frames > mFadeInterBufFrames)
        {
            uint32_t newFrames  = ((frames / mPerSize) + 1) * mPerSize;
            if (allocFaderIntermediateBuffer(newFrames) == AudioError::OK)
            {
                fade_ret = mFaderOutDirOut.process(mOutDevice.buf, frames, mFadeInterBuf);
            }
            else
            {
                Logging(mStreamingHandle, LL_WARNING) << "BackendAlsaImpl::workerThread Realloc of mFadeInterBuf failed,So not doing FadeOut "  << Logging::endl;
            }
        }
        else
        {
           fade_ret = mFaderOutDirOut.process(mOutDevice.buf, frames, mFadeInterBuf);
        }

        /* FaderError::OK   : Fading in Process
         * FaderError::DONE : Fading Completed
         */
        if ((fade_ret == FaderError::OK) || (fade_ret == FaderError::DONE))
        {
            mOutDevice.buf = mFadeInterBuf;
        }

        if (fade_ret != FaderError::OK)
        {
            Logging(mStreamingHandle, LL_DEBUG) << "BackendAlsaImpl::workerThread FadeOut process completed with, err " << fade_ret << " So changing state to Stop" << Logging::endl;
            mpStateMachine->workerEvent(WORKER_EVENT_STOPPING);
            event = WORKER_EVENT_STOP;
            mRemFrames = frames % mPerSize;
        }
    }

    if (mPlaybackPcmFp && mOutDevice.buf)
    {
        fwrite(mOutDevice.buf, (frames * mBytesPerSample), 1, mPlaybackPcmFp);
    }

    ret = readWriteAlsaDevice(mOutDevice, frames);

    if (ret < 0)
    {
        Logging(mStreamingHandle, LL_ERROR) << "BackendAlsaImpl::workerThread AlsaWrite failed with err " << ret << " so aborting stream" << Logging::endl;
        audioret = AudioError::FAILURE;
        mpStateMachine->workerEvent(WORKER_EVENT_ABORTING);
        event = WORKER_EVENT_ABORT;
    }

    /*To handle the abort event from the application */
    if (mpStateMachine->getState() == ALSA_SSTATE_ABORTING)
    {
        event = WORKER_EVENT_ABORT;
    }

    /* Update statics information only if Xrun occurs */
    if (mStatistics.flag != StreamStatistics::XRUN_UNKNOWN)
    {
        updateStreamStatistics();
    }

    mOutDevice.buf = nullptr;
    workerThreadExitHandling(event, audioret);
    return static_cast<int>(AudioState::CONTINUE);
}

void BackendAlsaImpl::workerThreadExitHandling(WorkerEvent event, AudioError err)
{
    if (event == WORKER_EVENT_ABORT || event == WORKER_EVENT_STOP)
    {
        mpStateMachine->workerEvent(event);
        mStreamingHandle.eostreaming(err);
    }
}

void BackendAlsaImpl::deinitThread(int errInit)
{
    Logging(mStreamingHandle, LL_DEBUG) << "BackendAlsaImpl::deinitThread err " << static_cast<AudioState>(errInit) << Logging::endl;
    disablePcmDump();
    cleanup();
    mpStateMachine->workerEvent(WORKER_EVENT_CLOSE);
}

void BackendAlsaImpl::updateStreamStatistics()
{
    mStatistics.time = getStreamTimeInFrames();
    mStreamingHandle.statistics(mStatistics);
    mStatistics.flag = StreamStatistics::XRUN_UNKNOWN; /* Resetting flag, Flag will be updated again during Xrun */
}

void BackendAlsaImpl::resetStreamStatistics()
{
    mStatistics.time           = 0;
    mStatistics.flag           = StreamStatistics::XRUN_UNKNOWN;
    mStatistics.xruns.playback = 0;
    mStatistics.xruns.capture  = 0;
}
void BackendAlsaImpl::faderRestart()
{
    vector<AlsaDevice>::iterator itr = mDevice.begin();
    for (; itr != mDevice.end(); ++itr)
    {
        if (!itr->isConfigured())
        {
            continue;
        }

        if (itr->dir == SND_PCM_STREAM_CAPTURE)
        {
            mFaderInDirIn.restart();
        }

        if (itr->dir == SND_PCM_STREAM_PLAYBACK)
        {
            mFaderOutDirOut.restart();
        }
    }
}

void BackendAlsaImpl::faderReset()
{
    vector<AlsaDevice>::iterator itr = mDevice.begin();
    for (; itr != mDevice.end(); ++itr)
    {
        if (!itr->isConfigured())
        {
            continue;
        }

        if (itr->dir == SND_PCM_STREAM_CAPTURE)
        {
            mFaderInDirIn.reset();
        }

        if (itr->dir == SND_PCM_STREAM_PLAYBACK)
        {
            mFaderOutDirOut.reset();
        }
    }

    if (mFadeInterBuf)
    {
        delete [] mFadeInterBuf;
        mFadeInterBuf = nullptr;
    }
    mFadeInterBufFrames = 0;
}

AudioError BackendAlsaImpl::faderConfigure()
{
#define DEFAULT_FADE_BUF_PERIOD_COUNT 4
    FaderError fadetRet;

    vector<AlsaDevice>::iterator itr = mDevice.begin();
    for (; itr != mDevice.end(); ++itr)
    {
        if (!itr->isConfigured())
        {
            continue;
        }

        if (itr->dir == SND_PCM_STREAM_CAPTURE)
        {
            fadetRet = mFaderInDirIn.configure(mFormat, mRate, mChannels);
            Logging(mStreamingHandle, LL_DEBUG) << "BackendAlsaImpl::faderConfigure Fade In Configure, err " << fadetRet << Logging::endl;
        }

        if (itr->dir == SND_PCM_STREAM_PLAYBACK)
        {
            fadetRet = mFaderOutDirOut.configure(mFormat, mRate, mChannels);
            Logging(mStreamingHandle, LL_DEBUG) << "BackendAlsaImpl::faderConfigure Fade Out Configure, err " << fadetRet << Logging::endl;
        }
    }

    /* Allocating Intermediate Fader output buffer */
    AudioError err = allocFaderIntermediateBuffer(mPerSize * DEFAULT_FADE_BUF_PERIOD_COUNT);
    if (err != AudioError::OK)
    {
        return AudioError::NOMEM;
    }

    return AudioError::OK;
}

AudioError BackendAlsaImpl::allocFaderIntermediateBuffer(const uint32_t frames)
{
    unsigned char * temp = new unsigned char [frames * mChannels * getBytesPerSample(mFormat)];
    if (temp == nullptr)
    {
        return AudioError::NOMEM;
    }

    if (mFadeInterBuf)
    {
        delete mFadeInterBuf;
        mFadeInterBuf = nullptr;
    }

    mFadeInterBuf = temp;
    mFadeInterBufFrames = frames;

    return AudioError::OK;
}

void BackendAlsaImpl::streamTimeReset()
{
    mStreamTimeInfo.countRwFrames  = 0;
    mStreamTimeInfo.countRwPeriods = 0;
}



/*construct thread name in form <prefix>_<INCard>_<OUTCard>*/
string BackendAlsaImpl::threadName()
{
    string thread_name = "abalsa:";
    string delim       = ":";
    string unused      = "X";

    if (mDuplex)
    {
        /*duplex - due to limited name length we use only part of in and out */
        int32_t avail = PTHREAD_NAME_LEN - thread_name.length() - delim.length();
        if (avail > 0)
        {
            thread_name += mInDevice.name.substr(0, avail/2);
            thread_name += delim;
            thread_name += mOutDevice.name.substr(0, avail/2);
        }
    }
    else
    {
        /*no duplex - try full name*/
        if (mInDevice.isConfigured())
        {
            thread_name += mInDevice.name;
        }
        else
        {
            thread_name += unused;
        }
        thread_name += delim;
        if (mOutDevice.isConfigured())
        {
            thread_name += mOutDevice.name;
        }
        else
        {
            thread_name += unused;
        }
    }

    return thread_name;
}

snd_pcm_format_t BackendAlsaImpl::GetALSAFmt(const AudioFormat format)
{
    snd_pcm_format_t ret = SND_PCM_FORMAT_UNKNOWN;

    std::map<AudioFormat, snd_pcm_format_t> alsaFormat =
    {
            { AudioFormat::UNKNOWN,    SND_PCM_FORMAT_UNKNOWN    },
            { AudioFormat::S8,         SND_PCM_FORMAT_S8         },
            { AudioFormat::U8,         SND_PCM_FORMAT_U8         },
            { AudioFormat::S16_LE,     SND_PCM_FORMAT_S16_LE     },
            { AudioFormat::S16_BE,     SND_PCM_FORMAT_S16_BE     },
            { AudioFormat::U16_LE,     SND_PCM_FORMAT_U16_LE     },
            { AudioFormat::U16_BE,     SND_PCM_FORMAT_U16_BE     },
            { AudioFormat::S24_LE,     SND_PCM_FORMAT_S24_LE     },
            { AudioFormat::S24_BE,     SND_PCM_FORMAT_S24_BE     },
            { AudioFormat::U24_LE,     SND_PCM_FORMAT_U24_LE     },
            { AudioFormat::U24_BE,     SND_PCM_FORMAT_U24_BE     },
            { AudioFormat::S32_LE,     SND_PCM_FORMAT_S32_LE     },
            { AudioFormat::S32_BE,     SND_PCM_FORMAT_S32_BE     },
            { AudioFormat::U32_LE,     SND_PCM_FORMAT_U32_LE     },
            { AudioFormat::U32_BE,     SND_PCM_FORMAT_U32_BE     },
            { AudioFormat::FLOAT32_LE, SND_PCM_FORMAT_FLOAT_LE   },
            { AudioFormat::FLOAT32_BE, SND_PCM_FORMAT_FLOAT_BE   },
            { AudioFormat::FLOAT64_LE, SND_PCM_FORMAT_FLOAT64_LE },
            { AudioFormat::FLOAT64_BE, SND_PCM_FORMAT_FLOAT64_BE },

    };

    ret = alsaFormat.at(format);
    return ret;
}

AudioError BackendAlsaImpl::openAlsaDevice(AlsaDevice& device)
{
    AudioError ret = AudioError::OK;
    if (device.name.empty())
    {
        return AudioError::UNSUPPORTED;
    }

    Logging(mStreamingHandle, LL_DEBUG) << "BackendAlsaImpl::openAlsaDevice for Alsa card/device " << device.name << Logging::endl;

    int err = snd_pcm_open(&device.hndl, device.name.c_str(), device.dir, SND_PCM_NONBLOCK);
    if (err < 0)
    {
        Logging(mStreamingHandle, LL_ERROR) << "open failed" << LogAlsa(device, err) << Logging::endl;
        if (err == -EBUSY)
            ret = AudioError::BUSY;
        else
            ret =  AudioError::FAILURE;
    }

    return ret;
}

AudioError BackendAlsaImpl::cleanupAlsaDevice(AlsaDevice& device)
{
    AudioError ret = AudioError::OK;

    if (mSndPcmstatus)
    {
        snd_pcm_status_free(mSndPcmstatus);
        mSndPcmstatus = nullptr;
    }

    if (mSndOutput)
    {
        snd_output_close(mSndOutput);
        mSndOutput = nullptr;
    }

    if (device.buf)
    {
        if (device.dir == SND_PCM_STREAM_CAPTURE)
        {
            delete [] device.buf;
        }
        device.buf = nullptr;
    }

    return ret;
}

AudioError BackendAlsaImpl::fixupAlsaDevice(AlsaDevice& device)
{
    AudioError ret = AudioError::OK;
    if (!device.isConfigured())
    {
        return AudioError::FAILURE;
    }

    /* Memory allocated only for capture,For playback Application has to allocate */
    if (device.dir == SND_PCM_STREAM_CAPTURE)
    {
        device.buf = new unsigned char[(snd_pcm_frames_to_bytes(device.hndl, mPerSize) * NO_OF_PERIODS_ALLOC)];
        if (device.buf == nullptr)
        {
            return AudioError::NOMEM;
        }
    }
    return ret;
}

AudioError BackendAlsaImpl::configureAlsaDevice(AlsaDevice& device)
{
    AudioError ret = AudioError::OK;
    if (!device.isConfigured())
    {
        return AudioError::FAILURE;
    }

    /* Configure Alsa hardware params*/
    ret = setAlsaHwParams(device);
    if (ret != AudioError::OK)
    {
        return ret;
    }

    /* Configure Alsa software params*/
    ret = setAlsaSwParams(device);

    return ret;
}

AudioError BackendAlsaImpl::startAlsaDevice(AlsaDevice& device)
{
    AudioError ret = AudioError::OK;
    if (!device.isConfigured())
    {
        return AudioError::FAILURE;
    }

    Logging(mStreamingHandle, LL_DEBUG) << "BackendAlsaImpl::startAlsaDevice for Alsa card/device " << device.name << Logging::endl;

    device.wait_done = 0;

    /* snc_pcm_start is required only for capture device */
    if (device.dir == SND_PCM_STREAM_PLAYBACK)
    {
        return ret;
    }

    int err = snd_pcm_start(device.hndl);
    if (err < 0)
    {
        ret = AudioError::FAILURE;
        Logging(mStreamingHandle, LL_ERROR) << "snd_pcm_start" << LogAlsa(device, err) << Logging::endl;
    }

    return ret;
}

AudioError BackendAlsaImpl::stopAlsaDevice(AlsaDevice& device)
{
    AudioError ret = AudioError::OK;
    if (!device.isConfigured())
    {
        return AudioError::FAILURE;
    }
    Logging(mStreamingHandle, LL_DEBUG) << "BackendAlsaImpl::stopAlsaDevice for Alsa card/device " << device.name << Logging::endl;
    int err = snd_pcm_drop(device.hndl);
    if (err < 0)
    {
        ret = AudioError::FAILURE;
        Logging(mStreamingHandle, LL_ERROR) << "snd_pcm_drop" << LogAlsa(device, err) << Logging::endl;
    }

    return ret;
}

AudioError BackendAlsaImpl::drainAlsaDevice(AlsaDevice& device)
{
    AudioError ret = AudioError::OK;
    int err = -1;
    if (!device.isConfigured())
    {
        return AudioError::FAILURE;
    }

    if (mRemFrames)
    {
        err = snd_pcm_format_set_silence(GetALSAFmt(mFormat), mFadeInterBuf, mRemFrames * mChannels);
        if (err == 0)
        {
            device.alsarw(device.hndl, mFadeInterBuf, mRemFrames);
        }
        mRemFrames = 0;
    }

    snd_pcm_nonblock(device.hndl, 0);
    Logging(mStreamingHandle, LL_DEBUG) << "BackendAlsaImpl::drainAlsaDevice for Alsa card/device " << device.name << Logging::endl;
    err = snd_pcm_drain(device.hndl);
    if (err < 0)
    {
        ret = AudioError::FAILURE;
        Logging(mStreamingHandle, LL_ERROR) << "snd_pcm_drain" << LogAlsa(device, err) << Logging::endl;
    }
    snd_pcm_nonblock(device.hndl, 1);

    return ret;
}

AudioError BackendAlsaImpl::prepareAlsaDevice(AlsaDevice& device)
{
    AudioError ret = AudioError::OK;
    if (!device.isConfigured())
    {
        return AudioError::FAILURE;
    }
    Logging(mStreamingHandle, LL_DEBUG) << "BackendAlsaImpl::prepareAlsaDevice for Alsa card/device " << device.name << Logging::endl;
    int err = snd_pcm_prepare(device.hndl);
    if (err < 0)
    {
        ret = AudioError::FAILURE;
        Logging(mStreamingHandle, LL_ERROR) << "snd_pcm_prepare" << LogAlsa(device, err) << Logging::endl;
    }

    return ret;
}

AudioError BackendAlsaImpl::closeAlsaDevice(AlsaDevice& device)
{
    AudioError ret = AudioError::OK;
    if (!device.isConfigured())
    {
        return AudioError::FAILURE;
    }
    Logging(mStreamingHandle, LL_DEBUG) << "BackendAlsaImpl::closeAlsaDevice for Alsa card/device " << device.name << Logging::endl;
    int err = snd_pcm_close(device.hndl);
    if (err < 0)
    {
        ret = AudioError::FAILURE;
        Logging(mStreamingHandle, LL_ERROR) << "snd_pcm_close" << LogAlsa(device, err) << Logging::endl;
    }
    device.hndl = nullptr;

    return ret;
}

int BackendAlsaImpl::pcmWaitAlsaDevice(AlsaDevice& device)
{
    int ret = 0;
    uint32_t timeout = device.wait_done ? (mPerTime + DEFAULT_RUNTIME_WAIT_TOUT) : mInitalTimeOut;
    ret = snd_pcm_wait(device.hndl, timeout);
    if (ret == 0)
    {
        Logging(mStreamingHandle, LL_WARNING) << "snd_pcm_wait "<< (device.wait_done ?"stream ":"initial ") << timeout << LogAlsa(device, ret) << Logging::endl;
        return -ENODATA;
    }
    else if (ret < 0)
    {
        if (ret == -EPIPE)
        {
            Logging(mStreamingHandle, LL_WARNING) << "snd_pcm_wait error (XRUN)" << LogAlsa(device, ret) << Logging::endl;
        }
        else
        {
            Logging(mStreamingHandle, LL_ERROR) << "snd_pcm_wait error " << LogAlsa(device, ret) << Logging::endl;
        }
        return ret;
    }
    return 0;
}

int BackendAlsaImpl::readWriteProcessAlsaDevice(AlsaDevice& device, const snd_pcm_uframes_t frameLength, ssize_t offs)
{
    int ret = 0;

    switch (device.access)
    {
        case SND_PCM_ACCESS_MMAP_INTERLEAVED:
        case SND_PCM_ACCESS_RW_INTERLEAVED:
        {
            ret = device.alsarw(device.hndl, &device.buf[offs], frameLength);
        }
        break;

        default:
            Logging(mStreamingHandle, LL_ERROR) << "BackendAlsaImpl::readWriteAlsaDevice failed with Invalid access " << device.access << Logging::endl;
            return -EINVAL;
    }

    if (ret >= 0)
    {
        if (!device.wait_done)
        {
            device. startupFrameCount += ret;
            if (device.dir == SND_PCM_STREAM_CAPTURE)
            {
                device.wait_done = 1;
            }
            else
            {
                if (device. startupFrameCount > device.hwBufferSize)
                {
                    device.wait_done = 1;
                }
            }
        }

        /* Updating Stream time info */
        if (mDuplex)
        {
            if (SND_PCM_STREAM_PLAYBACK == device.dir)
            {
                mStreamTimeInfo.countRwFrames += ret;
            }
        }
        else
        {
            mStreamTimeInfo.countRwFrames += ret;
        }

        if (mStreamTimeInfo.countRwFrames >= mPerSize)
        {
            mStreamTimeInfo.countRwPeriods += (mStreamTimeInfo.countRwFrames / mPerSize);
            mStreamTimeInfo.countRwFrames   = mStreamTimeInfo.countRwFrames % mPerSize;
        }
    }

    return ret;
}

int32_t BackendAlsaImpl::readWriteAlsaDevice(AlsaDevice& device, const snd_pcm_uframes_t frameLength)
{
#define EGAIN_WAIT_TIME    2000 /* wait time[micro seconds] for EGAIN error */
    int ret               = 0;
    uint32_t retry_cnt    = 0;
    int32_t remain        = frameLength;
    int32_t rwsize        = 0;
    uint32_t eagainFlag   = 0;
    int32_t rwframecount  = 0;
    ssize_t offs          = 0;

    if (!device.isConfigured())
    {
        return 0;
    }

    while ((remain > 0) && (retry_cnt < MAX_RETRY_COUNT))
    {
        rwsize = ((uint32_t)remain < mPerSize) ? remain : mPerSize;

        ret = pcmWaitAlsaDevice(device);

        /* If user sends Abort Stream Event.Return Immediately*/
        if (mpStateMachine->getState() == ALSA_SSTATE_ABORTING)
        {
            return 0;
        }

        /* Handling pcm wait Timeout */
        if (ret == -ENODATA)
        {
            retry_cnt++;
            continue;
        }

        ret = readWriteProcessAlsaDevice(device, rwsize, offs);

        if (ret > rwsize)
            return -EINVAL;

        if (ret == -EAGAIN)
        {
            /* Do short sleep, only to avoid 100% CPU load*/
            if (eagainFlag != 0)
                usleep(EGAIN_WAIT_TIME);
            eagainFlag = 1;

            retry_cnt++;
            continue;
        }

        if (ret >= 0)
        {
            /* If read/write is success update the offset and reset the flags */
            remain       -= ret;
            offs         += snd_pcm_frames_to_bytes(device.hndl, ret);
            rwframecount += ret;

            retry_cnt     = 0;
            eagainFlag    = 0;
        }
        else
        {
            dumpAlsaDeviceInfo(device, AlsaInfo::STATUS);
            ret = alsaXrun(device, ret);
            if (ret < 0)
            {
                /* Except EPIPE,ESTRPIPE,EAGAIN & ENODATA
                 * All other error codes are considered as FATAL and Streaming will be aborted
                 */
                break;
            }
        }
    }

    return ((ret < 0 ) ? ret : rwframecount);
}

AudioError BackendAlsaImpl::startEventHandler()
{
    AudioError ret = AudioError::OK;
    vector<AlsaDevice>::iterator itr = mDevice.begin();

    Logging(mStreamingHandle, LL_INFO) << "BackendAlsaImpl::startEventHandler" << Logging::endl;
    for (; itr != mDevice.end(); ++itr)
    {
        if (!itr->isConfigured())
        {
            continue;
        }

        itr->wait_done         = 0;
        itr->startupFrameCount = 0;

        ret = prepareAlsaDevice(*itr);
        if (ret != AudioError::OK)
            break;

        ret = startAlsaDevice(*itr);
        if (ret != AudioError::OK)
            break;
    }
    mRemFrames = 0;
    faderRestart();
    return ret;
}

AudioError BackendAlsaImpl::stopEventHandler()
{
    Logging(mStreamingHandle, LL_INFO) << "BackendAlsaImpl::stopEventHandler" << Logging::endl;
    vector<AlsaDevice>::iterator itr = mDevice.begin();
    for (; itr != mDevice.end(); ++itr)
    {
        if (!itr->isConfigured())
            continue;

        itr->wait_done         = 0;
        itr->startupFrameCount = 0;

        if (itr->dir == SND_PCM_STREAM_PLAYBACK)
            drainAlsaDevice(*itr);

        if (itr->dir == SND_PCM_STREAM_CAPTURE)
            stopAlsaDevice(*itr);
    }

    updateStreamStatistics();
    return AudioError::OK;
}

AudioError BackendAlsaImpl::abortEventHandler()
{
    AudioError ret = AudioError::OK;
    updateStreamStatistics();
    vector<AlsaDevice>::iterator itr = mDevice.begin();
    Logging(mStreamingHandle, LL_INFO) << "BackendAlsaImpl::abortEventHandler" << Logging::endl;
    for (; itr != mDevice.end(); ++itr)
    {
        if (!itr->isConfigured())
            continue;

        ret = stopAlsaDevice(*itr);
    }
    return ret;
}

AudioError BackendAlsaImpl::closeEventHandler()
{
    AudioState ret = static_cast<AudioState>(CThread::joinThread());
    if (ret != AudioState::ABORT)
    {
        return AudioError::OK;
    }

    Logging(mStreamingHandle, LL_ERROR) << "BackendAlsaImpl::closeEventHandler failed with err " << ret << Logging::endl;
    return AudioError::FAILURE;
}

uint32_t BackendAlsaImpl::calcFadeTimeOut()
{
#define DEFAULT_STATECHANGE_WAIT_TIME 100
    uint32_t fadeOutTime;
    getFadeTime(FadeMode::OUT, StreamDirection::OUT, fadeOutTime);

    /* Considering time taken for capturing in case of duplexmode.So doubling the time */
    fadeOutTime = mDuplex ? (fadeOutTime << 1) : fadeOutTime;

    return (fadeOutTime + (mDuplex ? (mPerTime << 1) : mPerTime) + DEFAULT_STATECHANGE_WAIT_TIME);
}

void BackendAlsaImpl::cleanup()
{
    vector<AlsaDevice>::iterator itr = mDevice.begin();

    Logging(mStreamingHandle, LL_DEBUG) << "BackendAlsaImpl::cleanup" << Logging::endl;
    faderReset();
    for (; itr != mDevice.end(); ++itr)
    {
        if (!itr->isConfigured())
            continue;

        closeAlsaDevice(*itr);
        cleanupAlsaDevice(*itr);
        resetDevicesInfo(*itr);
    }
}

AudioError BackendAlsaImpl::setAlsaHwParams(AlsaDevice& device)
{
    int err = 0;
    snd_pcm_format_t format = SND_PCM_FORMAT_UNKNOWN;
    unsigned int rrate = 0;
    snd_pcm_hw_params_t *hwParams = nullptr;
    const snd_pcm_access_t pcmAccess[AUDIO_ACCESS_TYPES] =
                { SND_PCM_ACCESS_MMAP_INTERLEAVED, SND_PCM_ACCESS_RW_INTERLEAVED };

    snd_pcm_hw_params_alloca(&hwParams);

    format = GetALSAFmt(mFormat);

    err = snd_pcm_hw_params_any(device.hndl, hwParams);
    if (err < 0)
    {
        Logging(mStreamingHandle, LL_ERROR) << "Broken configuration" << LogAlsa(device, err) << Logging::endl;
        return AudioError::FAILURE;
    }

    for (uint32_t i = 0; i < AUDIO_ACCESS_TYPES; i++)
    {
        err = snd_pcm_hw_params_set_access(device.hndl, hwParams, pcmAccess[i]);
        if (err == 0)
        {
            device.access = pcmAccess[i];
            break;
        }
    }
    if (err < 0)
    {
        Logging(mStreamingHandle, LL_ERROR) << "Access type " << device.access << " not available" << LogAlsa(device, err) << Logging::endl;
        return AudioError::UNSUPPORTED;
    }

    err = snd_pcm_hw_params_set_format(device.hndl, hwParams, format);
    if (err < 0)
    {
        Logging(mStreamingHandle, LL_ERROR) << "Sample format " << format << " not available" << LogAlsa(device, err) << Logging::endl;
        return AudioError::UNSUPPORTED;
    }

    err = snd_pcm_hw_params_set_channels(device.hndl, hwParams, mChannels);
    if (err < 0)
    {
        Logging(mStreamingHandle, LL_ERROR) << "Channels count " <<  mChannels << " not available" << LogAlsa(device, err) << Logging::endl;
        return AudioError::UNSUPPORTED;
    }

    rrate = mRate;
    err = snd_pcm_hw_params_set_rate_near(device.hndl, hwParams, &rrate, 0);
    if (err < 0)
    {
        Logging(mStreamingHandle, LL_ERROR) << "Rate " << rrate << "Hz not available" << LogAlsa(device, err) << Logging::endl;
        return AudioError::FAILURE;
    }
    if (rrate != mRate)
    {
        Logging(mStreamingHandle, LL_ERROR) << "BackendAlsaImpl::setAlsaHwParams Rate doesn't match (requested " << mRate << "Hz, got " <<  rrate << "Hz" << LogAlsa(device, err) << Logging::endl;
        return AudioError::UNSUPPORTED;
    }

    if (mDuplex && (device.dir == SND_PCM_STREAM_PLAYBACK))
    {
        /*capture device has been setup before-> allow only same period size*/
        err = snd_pcm_hw_params_set_period_size(device.hndl, hwParams, mPerSize, -1);
        if (err < 0)
        {
            /* Trying with Sub unit direction as "0" */
            err = snd_pcm_hw_params_set_period_size(device.hndl, hwParams, mPerSize, 0);
        }
        if (err < 0)
        {
            /* Trying with Sub unit direction as "1" */
            err = snd_pcm_hw_params_set_period_size(device.hndl, hwParams, mPerSize, 1);
        }
    }
    else
    {
        err = snd_pcm_hw_params_set_period_size_near(device.hndl, hwParams, &mPerSize, nullptr);
    }

    if (err < 0)
    {
        Logging(mStreamingHandle, LL_ERROR) << "Unable to set period size " << mPerSize << LogAlsa(device, err) << Logging::endl;
        return AudioError::UNSUPPORTED;
    }

    err = snd_pcm_hw_params_set_periods_near(device.hndl, hwParams, &device.periods, nullptr);
    if (err < 0)
    {
        Logging(mStreamingHandle, LL_ERROR) << "Unable to set periods " << device.periods << LogAlsa(device, err) << Logging::endl;
        return AudioError::UNSUPPORTED;
    }

    err = snd_pcm_hw_params(device.hndl, hwParams);
    if (err < 0)
    {
        Logging(mStreamingHandle, LL_ERROR) << "Unable to set Alsa hw params" << LogAlsa(device, err) << Logging::endl;
        return AudioError::FAILURE;
    }

    snd_pcm_hw_params_get_buffer_size(hwParams, &device.hwBufferSize);

    return configureAlsaDeviceAcces(device);
}

AudioError BackendAlsaImpl::configureAlsaDeviceAcces(AlsaDevice& device)
{
    AudioError err = AudioError::OK;
    switch (device.access)
    {
        case SND_PCM_ACCESS_MMAP_INTERLEAVED:
            /*on interleaved access we can directly pass user buffers*/
            if (device.dir == SND_PCM_STREAM_PLAYBACK)
                device.alsarw = (AlsaRW) snd_pcm_mmap_writei;
            else
                device.alsarw = (AlsaRW) snd_pcm_mmap_readi;
            break;

        case SND_PCM_ACCESS_RW_INTERLEAVED:
            /*on interleaved access we can directly pass user buffers*/
            if (device.dir == SND_PCM_STREAM_PLAYBACK)
                device.alsarw = (AlsaRW) snd_pcm_writei;
            else
                device.alsarw = (AlsaRW) snd_pcm_readi;
            break;
        default:
            Logging(mStreamingHandle, LL_ERROR) << "BackendAlsaImpl::configureAlsaDeviceAcces Invalid access " << device.access << Logging::endl;
            err = AudioError::FAILURE;
            break;
    }
    Logging(mStreamingHandle, LL_DEBUG) << "BackendAlsaImpl::configureAlsaDeviceAcces  access " << device.access << Logging::endl;
    return err;
}

void BackendAlsaImpl::dumpAlsaDeviceInfo(AlsaDevice& device, const AlsaInfo info)
{
    int err = -1;

    if (!device.isConfigured())
    {
        return;
    }

    if (info == AlsaInfo::CONFIG)
    {
        err = snd_pcm_dump(device.hndl, mSndOutput);
    }
    else if (info == AlsaInfo::STATUS)
    {
        Logging(mStreamingHandle, LL_DEBUG) << "Status of device [" << device.name << "]: " <<
                        snd_pcm_state_name((snd_pcm_state_t)snd_pcm_state(device.hndl)) << Logging::endl;
        snd_pcm_status(device.hndl, mSndPcmstatus);
        err = snd_pcm_status_dump(mSndPcmstatus, mSndOutput);
    }

    if (err < 0)
    {
        Logging(mStreamingHandle, LL_WARNING) << "Alsa Status Dump Failed" << device.access << Logging::endl;
    }
    else
    {
        char *s = NULL;
        snd_output_buffer_string(mSndOutput, &s);
        Logging(mStreamingHandle, LL_DEBUG) << s << Logging::endl;
    }

    snd_output_flush(mSndOutput);
}

AudioError BackendAlsaImpl::allocAlsaDumpMemory()
{
    int err;
    err = snd_pcm_status_malloc(&mSndPcmstatus);
    if (err < 0)
    {
        return AudioError::NOMEM;
    }

    err = snd_output_buffer_open(&mSndOutput);
    if (err < 0)
    {
        if (mSndPcmstatus)
            snd_pcm_status_free(mSndPcmstatus);
        mSndPcmstatus = nullptr;
        return AudioError::NOMEM;
    }
    return AudioError::OK;
}

AudioError BackendAlsaImpl::setAlsaSwParams(AlsaDevice& device)
{
    int err = -1;
    snd_pcm_uframes_t val = 0;
    snd_pcm_sw_params_t *swParams = nullptr;
    snd_pcm_sw_params_alloca(&swParams);

    err = snd_pcm_sw_params_current(device.hndl, swParams);
    if (err < 0)
    {
        Logging(mStreamingHandle, LL_ERROR) << "Unable to determine current Alsa sw params" << LogAlsa(device, err) << Logging::endl;
        return AudioError::FAILURE;
    }

    if (SND_PCM_STREAM_CAPTURE == device.dir)
    {
        val = 1;
    }
    else
    {
        val = min(mPerSize * device.periods, mPerSize * 2);
    }

    err = snd_pcm_sw_params_set_start_threshold(device.hndl, swParams, val);
    if (err < 0)
    {
        Logging(mStreamingHandle, LL_ERROR) << "Unable to set start threshold: " << val << LogAlsa(device, err) << Logging::endl;
        return AudioError::UNSUPPORTED;
    }

    err = snd_pcm_sw_params_set_avail_min(device.hndl, swParams, mPerSize);
    if (err < 0)
    {
        Logging(mStreamingHandle, LL_ERROR) << "Unable to set avail min " << mPerSize << LogAlsa(device, err) << Logging::endl;
        return AudioError::UNSUPPORTED;
    }

    err = snd_pcm_sw_params(device.hndl, swParams);
    if (err < 0)
    {
        Logging(mStreamingHandle, LL_ERROR) << "Unable to set Alsa sw params" << LogAlsa(device, err) << Logging::endl;
        return AudioError::FAILURE;
    }

    return AudioError::OK;
}

/* I/O error handler */
int BackendAlsaImpl::alsaXrun(AlsaDevice& device, int err)
{
#define SUSPEND_FLAG_WAIT_TIME    100000       /* wait time [micro seconds] to resume from suspend flag */
    Logging(mStreamingHandle, LL_WARNING) << "BackendAlsaImpl::alsaXrun with err " << err << Logging::endl;

    device.wait_done         = 0;
    device.startupFrameCount = 0;

    if (err == -EPIPE)
    {
        /* x-run */
        /* Updating stream statistics */
        if (device.dir == SND_PCM_STREAM_PLAYBACK)
        {
            mStatistics.flag = (mStatistics.flag == StreamStatistics::XRUN_CAPTURE) ? StreamStatistics::XRUN_CAPNPLAY : StreamStatistics::XRUN_PLAYBACK;
            mStatistics.xruns.playback++;
        }
        else
        {
            mStatistics.flag = (mStatistics.flag == StreamStatistics::XRUN_PLAYBACK) ? StreamStatistics::XRUN_CAPNPLAY : StreamStatistics::XRUN_CAPTURE;
            mStatistics.xruns.capture++;
        }

        err = snd_pcm_prepare (device.hndl);
    }
    else if (err == -ESTRPIPE)
    {
        /* wait till suspend flag is resumed */
        while ((err = snd_pcm_resume (device.hndl)) == -EAGAIN)
            usleep (SUSPEND_FLAG_WAIT_TIME);

        if (err < 0)
        {
            err = snd_pcm_prepare (device.hndl);
        }
    }

    if (err == 0)
    {
        if (device.dir == SND_PCM_STREAM_CAPTURE)
        {
            err = snd_pcm_start (device.hndl);
        }
    }

    if (err < 0)
    {
        Logging(mStreamingHandle, LL_ERROR) << "Not able to recover " << LogAlsa(device, err) << Logging::endl;
    }
    return err;
}
