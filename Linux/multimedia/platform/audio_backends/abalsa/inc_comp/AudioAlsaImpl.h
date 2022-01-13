/************************************************************************
 * @file: AudioAlsaImpl.h
 *
 * @version: 0.1
 *
 * @description: This header file contains class definition of BackendAlsaImpl.
 * A wrapper class for Alsa API's. BackendAlsaImpl API's will be called by
 * Alsa State Machine API's . BackendAlsaImpl will call the Alsa API's for
 * Open/Close, Configuring,Write/Read to Alsa Device.
 *
 * @authors: Jens Lorenz, jlorenz@de.adit-jv.com 2015
 *           Vijay Palaniswamy, vijay.palaniswamy@in.bosch.com 2015
 *
 * @copyright (c) 2015 Advanced Driver Information Technology.
 * Copyright of Advanced Driver Information Technology, Bosch, and DENSO.
 * All rights reserved.
 *
 ***********************************************************************/


#ifndef _AUDIOALSAIMPL_H_
#define _AUDIOALSAIMPL_H_

#include <string>
#include <alsa/asoundlib.h>
#include <vector>

#include "AudioAlsaStateMachine.h"
#include "AudioStreaming.h"
#include "AudioFader.h"
#include "CThread.h"

#define mInDevice    mDevice[ALSA_DIR_CAPTURE]
#define mOutDevice   mDevice[ALSA_DIR_PLAYBACK]


namespace adit
{

namespace utility
{

namespace audio
{

struct StreamInfo
{
    StreamInfo(const std::string &_inCard, const std::string &_outCard,
               const AudioFormat _format, const uint32_t _rate, const uint32_t _channels,
               uint32_t &_periodFrames) :
               inCard(_inCard), outCard(_outCard), format(_format),
               rate(_rate), channels(_channels), periodFrames(_periodFrames)
               {}
    const std::string  &inCard;         /* Card name for inward direction (Capture)                                  */
    const std::string  &outCard;        /* Card name for outward direction (Playback)                                */
    AudioFormat         format;         /* Specify data format of stream is Little or Big endian, Signed or Unsigned */
    uint32_t            rate;           /* The sample rate of the stream                                             */
    uint32_t            channels;       /* The count of channels of stream                                           */
    uint32_t           &periodFrames;   /* Period Size in frames                                                     */
};

class BackendAlsaImpl : public CThread
{

public:

    /**
      * Constructor Function
      * @param[IN] streamingHandle Handle of Streaming class used for Logging and data processing
      */
    BackendAlsaImpl(Streaming& streamingHandle);
    virtual ~BackendAlsaImpl();

    /**
     * This function is called in constructor from Backend Alsa for setting State machine handle
     */
    void setStateMachineHandle(StateMachine* stateMachine);

    /**
     * This function will be called for configuring the FadeIn or FadeOut Time
     * @param [IN] fademode  Specify Fade Mode [FadeIn/FadeOut]
     * @param [IN] streamdir Specify stream direction Playback/Capture
     * @param [IN] fadeTime  Specify the FadeIn/FadeOut time to configure
     * @return OK          if fade time is configured successfully
     * @return UNSUPPORTED if the fader doesn't support the specified fade time [Fade time Range: 0 - 500 milliseconds]
     */
    AudioError setFadeTime(const enum FadeMode fadeMode, const enum StreamDirection streamDir, const uint32_t fadeTime);

    /**
     * This function will be called for getting configured FadeIn or FadeOut Time
     * @param [IN]  fademode  Specify Fade Mode [FadeIn/FadeOut]
     * @param [IN]  streamdir Specify stream direction Playback/Capture
     * @param [OUT] fadeTime  To get the configured FadeIn/FadeOut time
     * @return OK          if configured fade time is returned successfully
     * @return UNSUPPORTED if fader was not configured
     */
    AudioError getFadeTime(const enum FadeMode fadeMode, const enum StreamDirection streamDir, uint32_t& fadeTime);

    /**
     * This function will be called to get stream time in Frames
     * @return Returns Stream time in Frame count
     * @return 0 if stream is not started
     */
    uint64_t getStreamTimeInFrames();

    /**
     * This function will be called to get stream Latency time in Frames
     * @return Returns Stream Latency in Frame count
     * @return 0 if stream not opened  or started
     */
    uint64_t getStreamLatencyInFrames();

    /**
     * This function will be called to set the worker thread priority
     * @param [IN] policy   To specify thread policy for same priority Threads
     * @param [IN] priority Thread Priority value
     *
     * Note: The input value for policy should be 0 to 2
     *       0 means SCHED_OTHER. SCHED_OTHER can only be used at static priority 0
     *       1 means SCHED_FIFO. SCHED_FIFO have a priority value in the range 1 (low) to 99 (high)
     *       2 means SCHED_RR. SCHED_RR have a priority value in the range 1 (low) to 99 (high)
     *
     *       Default priority of worker thread is derived from process.
     *       Thread policy and priority should be configured before start of stream.
     *       Thread policy and priority can be changed after start call but this is not recommended.
     */
    void setThreadSched(int policy, int priority);
    /**
     * This function is called to get selected backend name
     */
    const char* getBackendName();

    void setInitialTimeout(const uint32_t timeout);

    uint32_t getInitialTimeout() const;

    /**
     * This function is called in OpenEvent , To open ALsa Device
     * @param [IN] data Contains stream info card Name, Stream format, Rate, Channels
     * @return OK      if Alsa device is opened successfully
     * @return FAILURE if Alsa device open/configure,card name is empty and fader configuration failed failed
     */
    AudioError doOpenStream(StreamInfo& data);

    /**
     * This function is called in startStream event to prepare ALsa device
     * @return OK      if Alsa device is started successfully
     * @return FAILURE if Alsa Handle is nullptr or Alsa device failed to start
     */
     AudioError startEventHandler();

    /**
     * This function is called in stopStream event for stopping and draining the alsa devices Fader
     * @return OK      if Alsa device is Stopped/drained successfully
     * @return FAILURE if Alsa Handle is nullptr or Alsa device failed to start
     */
    AudioError stopEventHandler();

    /**
     * This function is called in abortStream event to stop Alsa Device
     * @return OK      if ALsa device pcm drop is success
     * @return FAILURE if Alsa Handle is nullptr or pcm drop failed
     */
    AudioError abortEventHandler();

    /**
     * This function is called in closeStream event to join worker thread
     */
     AudioError closeEventHandler();

    /**
     * This function is called to calculate the fading time out
     * @return Return calculated fade time in milli seconds
     */
    uint32_t calcFadeTimeOut();

    /* Utility Functions */
    void enablePcmDump(const enum StreamDirection streamDir, char* dumpPath);
    void disablePcmDump();

protected:

    /**
     * This function will be called on startThread() of thread and can be used to allocated worker specific resources.
     */
    int initThread();

     /**
      * This function will be called after initThread() was successfully started.
      */
    int workerThread();

    /**
     * This function will be called in case joinThread() was called or in case of failure in initThread() or workerThread().
     */
    void deinitThread(int errInit);

private:

    enum AlsaCardDir
    {
        ALSA_DIR_CAPTURE,
        ALSA_DIR_PLAYBACK,
        ALSA_DIR_MAX
    };

    enum class AlsaInfo
    {
        STATUS,
        CONFIG
    };

    /* sound cards + card configuration */
#define PERIODS_DEFAULT  4
    typedef snd_pcm_sframes_t (*AlsaRW)(snd_pcm_t *pcm, void *buffer, snd_pcm_uframes_t size);
    struct AlsaDevice
    {
        AlsaDevice(const snd_pcm_stream_t _dir) :
            name(), hndl(nullptr), dir(_dir), periods(PERIODS_DEFAULT),
            buf(nullptr), delay(0), wait_done(0)
            {};
        bool isConfigured() { if (name.empty() || (hndl == nullptr)) return false; else return true; }
                                                      /* Is AlsaDevice Configure or not */
        std::string              name;                /* ALSA PCM card/device name */
        snd_pcm_t               *hndl;                /* device handle */
        snd_pcm_stream_t         dir;                 /* direction of card/Device */
        unsigned int             periods;             /* No. of periods */
        unsigned char           *buf;                 /* Buffer to hold Capture/Playback data */
        snd_pcm_sframes_t        delay;               /* Delay in frame to capture/Play data */
        uint32_t                 wait_done;           /* initial snd_pcm_wait wait done */
        snd_pcm_access_t         access;              /* PCM access type like interleaved , Non interleaved*/
        AlsaRW                   alsarw;              /* Transfer function - depends on access method */
        snd_pcm_uframes_t        hwBufferSize;        /* Buffer size in frames */
        snd_pcm_uframes_t        startupFrameCount;   /* counter on startup to detect Device active*/
    };

    struct LogAlsa
    {
        LogAlsa(const AlsaDevice &device, const int32_t err) :
            device(device), err(err)
            {};

        AlsaDevice device;
        int32_t    err;
    };

    inline friend std::ostream& operator<<(std::ostream& os, const LogAlsa& msg)
    {
        return os << " err " << snd_strerror(msg.err) << " on card/device " << msg.device.name << (msg.device.dir == SND_PCM_STREAM_PLAYBACK ? " OUT ":" IN ");
    }

    struct StreamTimeInfo
    {
        snd_pcm_uframes_t       countRwFrames;       /* Count of frames  */
        snd_pcm_uframes_t       countRwPeriods;      /* Count of periods */
    };

    std::vector<AlsaDevice>     mDevice;

    AudioFormat                 mFormat;             /* Format of the stream */
    uint32_t                    mRate;               /* Sample rate of the stream */
    uint32_t                    mChannels;           /* Number of channels */
    snd_pcm_uframes_t           mPerSize;            /* Period size: on duplex operation same on in + out device*/
    uint32_t                    mPerTime;            /* Period Time [In milliseconds]: on duplex operation same on in + out device*/
    uint32_t                    mDuplex;
    unsigned char              *mFadeInterBuf;
    uint32_t                    mFadeInterBufFrames;
    uint32_t                    mRemFrames;
    uint32_t                    mInitalTimeOut;

    StreamTimeInfo              mStreamTimeInfo;     /* Stream time information */
    StreamStatistics            mStatistics;         /* Handle to hold stream statistics */

    StateMachine               *mpStateMachine;      /* Handle for State Machine */

    snd_output_t               *mSndOutput;          /* Handle to hold Alsa status  dump */
    snd_pcm_status_t           *mSndPcmstatus;       /* Handle to hold Alsa device status */


    /*fader*/
    adit::utility::audio::Fader mFaderInDirIn;       /* Handle for doing FadeIn for capture stream */
    adit::utility::audio::Fader mFaderOutDirOut;     /* Handle for doing FadeOut for playback stream */

    /* Silence Detector, Streaming & Logging*/
    Streaming                   &mStreamingHandle;   /* Handle for Silence Detector, streaming & Logging Interface */

    uint32_t  mBytesPerSample;     /* < Number of bytes per sample */
    FILE     *mCapturePcmFp;
    FILE     *mPlaybackPcmFp;

    /**
     * This function is called for restarting the fader
     */
    void faderRestart();

    /**
     * This function is called for resetting the fader
     */
    void faderReset();

    /**
     * This function is called for configuring fader
     */
    AudioError faderConfigure();

    /**
     * This function is called for allocating fader intermediate buffer
     */
    AudioError allocFaderIntermediateBuffer(const uint32_t frames);

    /**
     * This function is called for resetting the playback/capture stream time
     */
    void streamTimeReset();

    /**
     * This function is called for updating stream statistics
     */
    void updateStreamStatistics();

    /**
     * This function is called for resetting stream statistics
    */
    void resetStreamStatistics();

    /**
     * This function is called to reset device info like pcm handle, buffers,Device name ...
     * @param[IN] device Alsa device specific data
     */
    void resetDevicesInfo(AlsaDevice& device);

    /*
     * This function will be called at the end of worker thread to hanlde the errors
     */
    void workerThreadExitHandling(WorkerEvent event, AudioError err);

    /* Alsa device operations */

    /**
     * This function will be called for opening the Alsa Device
     * @param[IN] device Alsa device specific data
     * @return OK          if Alsa device is opened successfully
     * @return UNSUPPORTED if Alsa Device name is empty
     * @return FAILURE     if Alsa device open failed
     */
    AudioError openAlsaDevice(AlsaDevice& device);

    /**
     * This function will be called for configuring [Hardware & sofware Params] the Alsa Device
     * @param[IN] device Alsa device specific data
     * @return OK      if Hardware & Software params are configured successfully
     * @return FAILURE if Alsa Handle is nullptr, Parameters specified during open stream is not matching
     */
    AudioError configureAlsaDevice(AlsaDevice& device);

    /**
     * This function will be called for Allocating memory required for Alsa Device
     * @param[IN] device Alsa device specific data
     * @return OK      if memory required by alsa is allocated successfully
     * @return FAILURE if Alsa Handle is nullptr
     * @return NOMEM   if memory allocation failed
     */
    AudioError fixupAlsaDevice(AlsaDevice& device);

    /**
     * This function will be called for freeing memory allocated for Alsa Device
     * @param[IN] device Alsa device specific data
     * @return OK      if all memory allocated during fixupAlsaDevice is freed successfully
     * @return FAILURE if Alsa Handle is nullptr
     *
     */
    AudioError cleanupAlsaDevice(AlsaDevice& device);

    /**
     * This function will be called to start Alsa Device
     * @param[IN] device Alsa device specific data
     * @return OK      if alsa device is started successfully
     * @return FAILURE if Alsa Handle is nullptr or Alsa device failed to start
     *
     */
    AudioError startAlsaDevice(AlsaDevice& device);

    /**
     * This function will be called to stop Alsa Device
     * @param[IN] device Alsa device specific data
     * @return OK      if ALsa device pcm drop is success
     * @return FAILURE if Alsa Handle is nullptr or pcm drop failed
     */
    AudioError stopAlsaDevice(AlsaDevice& device);

    /**
     * This function will be called to drain all PCM data in Alsa Device
     * @param[IN] device Alsa device specific data
     * @return OK      if ALsa device pcm drain is success
     * @return FAILURE if Alsa Handle is nullptr or pcm drain failed
     */
    AudioError drainAlsaDevice(AlsaDevice& device);

    /**
     * This function will be called to close Alsa Device
     * @param[IN] device Alsa device specific data
     * @return OK      if ALsa device close is success
     * @return FAILURE if Alsa Handle is nullptr or pcm close failed
     */
    AudioError closeAlsaDevice(AlsaDevice& device);

    /**
     * This function is called by worker thread to process wait for availability of
     * device, process read/write and detects/recover xruns
     * @param[IN] device      Alsa device specific data
     * @param[IN] frameLength No. of frames to Read/Write to Alsa Device
     * @return 0 or frames read/write on success, Otherwise negative error codes
     * @return -ETIME    if snd_pcm_wait timeout occurred
     * @return -EIO      if input/output error occurs
     * @return -EPIPE    if underrun for playback or ovverrun for caputure
     * @return -ESTRPIPE if alsa driver is suspended
     */
    int32_t readWriteAlsaDevice(AlsaDevice& device, const snd_pcm_uframes_t frameLength);

    /**
     * This function is called to executes read/write on alsa device
     * @param[IN] device      Alsa device specific data
     * @param[IN] frameLength No. of frames to Read/Write to Alsa Device
     * @return OK on success otherwise negative error codes
     * @return -EPIPE    if underrun for playback or ovverrun for caputure
     * @return -ESTRPIPE if alsa driver is suspended
     */
    int readWriteProcessAlsaDevice(AlsaDevice& device, const snd_pcm_uframes_t frameLength, ssize_t offs);

    /**
     * This function called to Wait for availability of alsa device
     * @param[IN] device      Alsa device specific data
     * @return -ENODATA    if snd_pcm_wait timeout occurred
     */
    int pcmWaitAlsaDevice(AlsaDevice& device);

    /**
     * This function is called to prepare Alsa Device
     * @param[IN] device Alsa device specific data
     * @return OK      if ALsa device prepare is success
     * @return FAILURE if Alsa Handle is nullptr or prepare failed
     */
    AudioError prepareAlsaDevice(AlsaDevice& device);

    /**
     * This function is called to configure Alsa read/write access
     * @param[IN] device Alsa device specific data
     * @return OK      if access type is matching
     * @return FAILURE if access type not matching
     */
    AudioError configureAlsaDeviceAcces(AlsaDevice& device);

    /* This function is called to dump current Alsa Device status
     * @param[IN] device Alsa device specific data
     * @param[IN] info Print Alsa configure details or Alsa device status
     */
    void dumpAlsaDeviceInfo(AlsaDevice& device, const AlsaInfo info);

    /**
     * This function is called to allocate memory required for Alsa dumping
     */
    AudioError allocAlsaDumpMemory();

    /**
     * This function is called to clean up Alsa Device
     */
    void cleanup();

    std::string threadName();

    snd_pcm_format_t GetALSAFmt(const AudioFormat format);

    /**
     * This function will called on configuration of Alsa hardware params.
     * @param[IN] device Alsa device specific data
     * @return OK      if Alsa Harware params are configured successfully
     * @return FAILURE if Alsa Handle is nullptr, Parameters specified during open stream is not matching
     */
    AudioError setAlsaHwParams(AlsaDevice& device);

    /**
     * This function will called on configuration of Alsa software params.
     * @param[IN] device Alsa device specific data
     * @return OK      if Alsa Software params are configured successfully
     * @return FAILURE if Alsa Handle is nullptr, Parameters specified during open stream is not matching
     */
    AudioError setAlsaSwParams(AlsaDevice& device);

    /**
     * This function is called for handling AlsaXrun. Since we are doing direct read/write
     * So based on the error return by snd_pcm_wait,
     * snd_pcm_writei/readi, will be handled in this function.
     * @param[IN] device Alsa device specific data
     * @param[IN] err  Alsa error
     */
    int alsaXrun(AlsaDevice& device, int err);

};

} /* namespace audio */

} /* namespace utility */

} /* namespace adit */

#endif /* _AUDIOALSAIMPL_H_ */
