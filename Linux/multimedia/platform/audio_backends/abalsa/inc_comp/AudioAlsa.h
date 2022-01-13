/************************************************************************
 * @file: AudioAlsa.h
 *
 * @version: 0.1
 *
 * @description: This header file contains class definition of BackendAlsa.
 * A wrapper class for Alsa State machine and Backend Alsa Implementation.
 * BackendAlsa will call the Backend Alsa Implementation API's for Open/Close,
 * Configuring ,Write/Read to Alsa Device and Alsa State Machine API's for
 * handling Stream states.
 *
 * @authors: Jens Lorenz, jlorenz@de.adit-jv.com 2015
 *           Thouseef Ahamed, tahamed@de.adit-jv.com 2015
 *           Vijay Palaniswamy, vijay.palaniswamy@in.bosch.com 2015
 *
 * @copyright (c) 2015 Advanced Driver Information Technology.
 * Copyright of Advanced Driver Information Technology, Bosch, and DENSO.
 * All rights reserved.
 *
 ***********************************************************************/


#ifndef _AUDIOALSA_H_
#define _AUDIOALSA_H_

#include <string>

#include "AudioAlsaStateMachine.h"
#include "AudioStreaming.h"
#include "AudioBackend.h"
#include "Logging.h"
#include "AudioAlsaImpl.h"
#include "AudioSilenceDetector.h"


namespace adit
{

namespace utility
{

namespace audio
{

class BackendAlsa : public Backend
{

public:

    /**
      * Constructor Function
      * @param[IN] streamingHandle Handle of Streaming class used for Logging and data processing
      */
    BackendAlsa(Streaming& streamingHandle);
    virtual ~BackendAlsa();

private:

    AudioError openStream(const std::string& inCard, const std::string& outCard,
            const AudioFormat format, const uint32_t rate, const uint32_t channels, uint32_t& periodFrames) final;

    AudioError closeStream() final;

    AudioError startStream() final;

    AudioError stopStream() final;

    AudioError abortStream() final;

    AudioError setFadeTime(const enum FadeMode fadeMode, const enum StreamDirection streamDir, const uint32_t fadeTime) final;

    AudioError getFadeTime(const enum FadeMode fadeMode, const enum StreamDirection streamDir, uint32_t& fadeTime) final;

    uint64_t getStreamTimeInFrames() final;

    uint64_t getStreamLatencyInFrames() final;

    const char* getBackendName() final;

    void setThreadSched(int policy, int priority) final;

    void setInitialTimeout(const uint32_t timeout) final;

    uint32_t getInitialTimeout() const final;

    eLogLevel audioErrorToLogLevel(AudioError err);

private:

    StateMachine     *mpStateMachine;
    BackendAlsaImpl  *mpBackendAlsaImpl;
    SilenceDetector  *mpSilenceDetector;
    LoggingInterface &mLoggingHandle;

    pthread_mutex_t  mApiMtx;  /* Mutex to make stream Thread safe */

    /**
     * This function will be called by Audio Interface API's for getting the mutex lock
     * The caller of this function will be under critical section
     * Note: Any function called apiLock should call apiUnLock
     */
    void apiLock();

    /**
      * This function will be called by Audio Interface API's for getting the mutex un lock
      * The caller of this function will be released from critical section
      * Note: without calling apiLock, apiUnLock should not be called
     */
    void apiUnLock();

    /**
     * This function is called for configuring fader
     */
     void silenceDetectConfigure(const StreamInfo& streamInfo);


    /**
     * This function is called for parsing environmental variables and creating object
     */
    void parseEnvAndCreateObject(Streaming& streamingHandle);
};

} /* namespace audio */

} /* namespace utility */

} /* namespace adit */

#endif /* _AUDIOALSA_H_ */
