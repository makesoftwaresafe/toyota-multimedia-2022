/************************************************************************
 * @file: AudioAlsaStateMachine.h
 *
 * @version: 0.1
 *
 * @description: This header file contains class definition of StateMachine,
 * Interface class IState [Classes inherited from IState like AlsaStreamStateClosed,
 * AlsaStreamStateStop, AlsaStreamStateFadeIn, AlsaStreamStateRun and
 * AlsaStreamStateFadeOut]. StateMachine class will be called by BackendALSA
 * class for handling the Alsa stream states. IState class will be used by
 * StateMachine class.
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
#ifndef _AUDIOALSASTATEMACHINE_H_
#define _AUDIOALSASTATEMACHINE_H_

#include <map>
#include "AudioTypes.h"
#include "AudioStreaming.h"

#define DEFAULT_WAIT_TIMEOUT    3000

namespace adit
{

namespace utility
{

namespace audio
{

enum AlsaStreamState
{
    ALSA_SSTATE_CLOSED = 0,  // default: devices closed, thread terminated/not yet started
    ALSA_SSTATE_STOP,
    ALSA_SSTATE_STOPPING,
    ALSA_SSTATE_RUN_FADE_IN,
    ALSA_SSTATE_RUNNING,
    ALSA_SSTATE_RUN_FADE_OUT,
    ALSA_SSTATE_ABORTING,
    ALSA_SSTATE_CLOSING
};

inline std::ostream& operator<<(std::ostream& os, const AlsaStreamState state)
{
    const char* string[] = { "CLOSED", "STOP", "STOPPING", "RUN_FADE_IN", "RUNNING", "RUN_FADE_OUT", "ABORTING", "CLOSING" };
    return os << string[static_cast<int>(state)];
}

enum WorkerEvent
{
    WORKER_EVENT_UNKOWN = -1,
    WORKER_EVENT_CLOSE = 0,
    WORKER_EVENT_STOP,
    WORKER_EVENT_STOPPING,
    WORKER_EVENT_RUNNING,
    WORKER_EVENT_FADE_OUT,
    WORKER_EVENT_ABORTING,
    WORKER_EVENT_ABORT
};

inline std::ostream& operator<<(std::ostream& os, const WorkerEvent event)
{
    const char* string[] = { "CLOSE", "STOP", "STOPPING", "RUNNING", "FADE_OUT", "ABORTING", "ABORT" };
    return os << string[static_cast<int>(event)];
}

class BackendAlsaImpl;

class IState
{

public:

    virtual ~IState() {}
    /**
     * This function is called in openStream
     * If the call is success stream state will be changed to STOP
     * @param [IN] data Contains stream info card Name, Stream format, Rate, Channels
     * @return OK          if the stream is opened successfully
     * @return BUSY        if specified stream is already opened/busy
     * @return UNSUPPORTED if trying  to start/stop/abort the stream, without opening
     * @return FAILURE     if specified streaming device is busy/invalid & specified parameters not matching
     */

    virtual AudioError openEvent(struct StreamInfo&)
    {
        return AudioError::INVALID;
    };

    /**
     * This function is called in closeStream
     * If the call is success stream state will be changed to CLOSED
     * @return OK          if the stream is closed successfully
     * @return UNSUPPORTED if trying to close stream, without stopping
     * @return FAILURE     if specified streaming device closing fails
     */
    virtual AudioError closeEvent() { return AudioError::INVALID; };

    /**
     * This function is called in startStream
     * If the call is success, Stream Playback/Capture will start and stream state will be changed to RUNNING
     * @return OK          if the stream is started successfully
     * @return UNSUPPORTED if stream is closed
     * @return FAILURE     if specified streaming device prepare/start fails
     */
    virtual AudioError startEvent() { return AudioError::INVALID; };

    /**
     * This function is called in stopStream
     * If the call is success,Stream Playback/Capture will stop and stream state will be changed to STOP
     * @return OK          if stream is stopped successfully
     * @return UNSUPPORTED if stream is closed
     * Note: In this case, Remaining samples in output queue will be played
     */
    virtual AudioError stopEvent()  { return AudioError::INVALID; };

    /**
     * This function is called in abortStream
     * If the call is success,Stream Playback/Capture will stop and stream state will be changed to STOP
     * @return OK          if stream is aborted successfully
     * @return UNSUPPORTED if stream is closed
     * Note: In this case, Remaining samples in input/output queue will be discarded
     */
    virtual AudioError abortEvent() { return AudioError::INVALID; };

    /**
     * This function will be called to change from current state to new state
     * based on the worker event.
     * @param [IN] WorkerEvent
     */
    virtual void workerEvent(WorkerEvent) { };
};

class StateMachine
{

public:

    StateMachine(BackendAlsaImpl *beAlsaImp, LoggingInterface& loggingHandle);
    virtual ~StateMachine();

    AudioError openEvent(StreamInfo& data);

    AudioError closeEvent();

    AudioError startEvent();

    AudioError stopEvent();

    AudioError abortEvent();

    void workerEvent(WorkerEvent event);

    /**
     * This function will be called to change from current state to new state of stream
     *  @param [IN] newState Stream will change to this state.
     *  @note: This function called from respective state handling classes only
     */
    void changeState(AlsaStreamState newState);

    /**
     * This function will be called to get current stream state
     * @return Current stream state
     */
    AlsaStreamState getState();

    /**
     * This function will be called to wait till state changes from given state to any new state
     * @param [IN] waitState wait till stream state changes from this state to any new state
     */
    void waitTillStateChange(const AlsaStreamState waitState);

    /**
     * This function will be called to wait till stream changes from current state to targetState
     * @param [IN] targetState wait till stream state changes to this state
     * @param [IN] timeInMs    Max wait time [milliseconds] for state change to happen
     * @return OK      if the stream changes successfully from current state to target state within the specified time
     * @return FAILURE if the stream fails to change state within the specified time
     */
    AudioError waitTillTargetState(AlsaStreamState targetState, const uint32_t timeInMs=DEFAULT_WAIT_TIMEOUT);

    /**
     * This function will be called to change state and wait till state changes from given state to any new state
     * @param [IN] newState change to given state and wait till stream state changes from this state to any new state
     * @param [IN] timeInMs Max wait time [milliseconds] for state change to happen
     * @return OK      if the stream changes successfully from given state to any state within the specified time
     * @return FAILURE if the stream fails to change state within the specified time
     */
    AudioError changeStateAndWaitTillStateChange(AlsaStreamState newState, const uint32_t timeInMs=DEFAULT_WAIT_TIMEOUT);

private:

    AlsaStreamState                      mCurrentState;
    std::map<AlsaStreamState, IState*>   mStateTable;
    LoggingInterface                    &mLoggingHandle;

    pthread_cond_t   mCond;
    pthread_mutex_t  mMtx;

    /**
     * This will be used to acquire mutex lock only for stream state change operations
     */
    void sLock();

    /**
     * This will be used to release mutex lock only for stream state change operations
     */
    void sUnLock();
};

class AlsaStreamStateClosed : public IState
{

public:

    AlsaStreamStateClosed(StateMachine& state, BackendAlsaImpl* beAlsaImpl, LoggingInterface& loggingHandle) : mState(state), mBeAlsaImpl(beAlsaImpl), mLoggingHandle(loggingHandle) {};

    AudioError openEvent(StreamInfo& data) override;
    AudioError closeEvent() override;
    void workerEvent(WorkerEvent event);

private:

    StateMachine     &mState;
    BackendAlsaImpl  *mBeAlsaImpl;
    LoggingInterface &mLoggingHandle;
};

class AlsaStreamStateStop : public IState
{

public:

    AlsaStreamStateStop(StateMachine& state, BackendAlsaImpl* beAlsaImpl, LoggingInterface& loggingHandle) : mState(state), mBeAlsaImpl(beAlsaImpl), mLoggingHandle(loggingHandle) {};

    AudioError startEvent() override;
    AudioError closeEvent() override;
    AudioError stopEvent() override;
    AudioError abortEvent() override;

private:

    StateMachine     &mState;
    BackendAlsaImpl  *mBeAlsaImpl;
    LoggingInterface &mLoggingHandle;
};

class AlsaStreamStateFadeIn : public IState
{

public:

    AlsaStreamStateFadeIn(StateMachine& state, BackendAlsaImpl* beAlsaImpl, LoggingInterface& loggingHandle) : mState(state), mBeAlsaImpl(beAlsaImpl), mLoggingHandle(loggingHandle) {};

    AudioError startEvent() override;
    AudioError stopEvent() override;
    AudioError abortEvent() override;
    void workerEvent(WorkerEvent event);

private:

    StateMachine     &mState;
    BackendAlsaImpl  *mBeAlsaImpl;
    LoggingInterface &mLoggingHandle;
};

class AlsaStreamStateRun : public IState
{

public:

    AlsaStreamStateRun(StateMachine& state, BackendAlsaImpl* beAlsaImpl, LoggingInterface& loggingHandle) : mState(state), mBeAlsaImpl(beAlsaImpl), mLoggingHandle(loggingHandle) {};

    AudioError startEvent() override;
    AudioError stopEvent() override;
    AudioError abortEvent() override;
    void workerEvent(WorkerEvent event);

private:

    StateMachine     &mState;
    BackendAlsaImpl  *mBeAlsaImpl;
    LoggingInterface &mLoggingHandle;
};

class AlsaStreamStateFadeOut : public IState
{

public:

    AlsaStreamStateFadeOut(StateMachine& state, BackendAlsaImpl* beAlsaImpl, LoggingInterface& loggingHandle) : mState(state), mBeAlsaImpl(beAlsaImpl), mLoggingHandle(loggingHandle) {};

    AudioError abortEvent() override;
    AudioError stopEvent() override;
    void workerEvent(WorkerEvent event);

private:

    StateMachine     &mState;
    BackendAlsaImpl  *mBeAlsaImpl;
    LoggingInterface &mLoggingHandle;
};

class AlsaStreamStateClosing : public IState
{

public:

    AlsaStreamStateClosing(StateMachine& state, BackendAlsaImpl* beAlsaImpl, LoggingInterface& loggingHandle) : mState(state), mBeAlsaImpl(beAlsaImpl), mLoggingHandle(loggingHandle) {};

    AudioError openEvent(StreamInfo& data) override;
    AudioError closeEvent() override;
    void workerEvent(WorkerEvent event);

private:

    StateMachine     &mState;
    BackendAlsaImpl  *mBeAlsaImpl;
    LoggingInterface &mLoggingHandle;

};

class AlsaStreamStateAborting : public IState
{

public:

    AlsaStreamStateAborting(StateMachine& state, BackendAlsaImpl* beAlsaImpl, LoggingInterface& loggingHandle) : mState(state), mBeAlsaImpl(beAlsaImpl), mLoggingHandle(loggingHandle) {};

    AudioError stopEvent() override;
    AudioError abortEvent() override;
    void workerEvent(WorkerEvent event);

private:

    StateMachine     &mState;
    BackendAlsaImpl  *mBeAlsaImpl;
    LoggingInterface &mLoggingHandle;
};

class AlsaStreamStateStopping : public IState
{

public:

    AlsaStreamStateStopping(StateMachine& state, BackendAlsaImpl* beAlsaImpl, LoggingInterface& loggingHandle) : mState(state), mBeAlsaImpl(beAlsaImpl), mLoggingHandle(loggingHandle) {};

    AudioError stopEvent() override;
    AudioError abortEvent() override;
    void workerEvent(WorkerEvent event);

private:

    StateMachine     &mState;
    BackendAlsaImpl  *mBeAlsaImpl;
    LoggingInterface &mLoggingHandle;
};

} /* namespace audio */

} /* namespace utility */

} /* namespace adit */

#endif /* _AUDIOALSASTATEMACHINE_H_ */
