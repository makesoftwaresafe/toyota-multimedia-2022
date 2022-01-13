/************************************************************************
 * @file: CAmInterface.cpp
 *
 * @version: 1.0
 *
 * @description: Implements the callback handler triggered by AudioManager
 *               and instantiates the client interface.
 *
 * @component: platform/audiomanager/amcp_test
 *
 * @author: Jens Lorenz, jlorenz@de.adit-jv.com 2015
 *
 * @copyright (c) 2015 Advanced Driver Information Technology.
 * This code is developed by Advanced Driver Information Technology.
 * Copyright of Advanced Driver Information Technology, Bosch, and DENSO.
 * All rights reserved.
 *
 ***********************************************************************/

#include <sys/time.h>
#include "CPlayer.h"
#include "CAmInterface.h"
#include "CAmSocketHandler.h"
#include "CAmDltWrapper.h"

using namespace am;
using namespace std;

#define THROW_ASSERT_NEQ(CALL, COND) \
        if (!((CALL) == (COND))) throw std::runtime_error( \
            std::string(__func__) + ": (" + std::string(#CALL) + " != " + std::to_string(COND) + ")")

CAmInterface::CAmInterface(CAmSocketHandler *socketHandler, IPlayer *player,
        const string &node, const string &application, const am_sourceID_t &sourceID, const am_sinkID_t &sinkID) :
        IAmApplicationClient(node, application),
        mpCbFunc(this, &CAmInterface::cbReconnect), mExitOnRemove(false),
        mDomain(0), mSrc(sourceID), mSink(sinkID), mMainConnection(0), mConnection(0),
        mpSocketHandler(socketHandler), mpPlayerInterface(player), mSerializer(socketHandler)
{
    if (sourceID == 0 || sourceID >= DYNAMIC_ID_BOUNDARY)
    {
        am_Source_s source;
        source.name = application;
        source.sourceID = sourceID;
        source.visible = true;

        IAmApplicationClient::registerSource(source, mSrc);
    }
    if (sinkID == 0 || sinkID >= DYNAMIC_ID_BOUNDARY)
    {
        am_Sink_s sink;
        sink.name = application;
        sink.sinkID = sinkID;
        sink.visible = true;

        IAmApplicationClient::registerSink(sink, mSink);
    }
}

CAmInterface::~CAmInterface()
{
    mpPlayerInterface = NULL;
    mpSocketHandler->removeTimer(mHandle);
}

am_Error_e CAmInterface::connect()
{
    return this->connect(mSrc, mSink);
}
am_Error_e CAmInterface::connect(const am_sourceID_t & src, const am_sinkID_t & sink)
{
    mSrc = src;
    mSink = sink;

    am_Error_e error = IAmApplicationClient::connect(mSrc, mSink, mMainConnection);
    if (error != E_OK) {
        logError("CAmInterface::connect connect from", mSrc, "to", mSink, "failed with", error);
        mpSocketHandler->exit_mainloop();
        return error;
    }

    logInfo("CAmInterface::connect from", mSrc, "to", mSink, error);
    mpSocketHandler->stopTimer(mHandle);
    return error;
}

am_Error_e CAmInterface::disconnect()
{
    am_Error_e error = IAmApplicationClient::disconnect(mMainConnection);
    if (error != E_OK)  {
        logError("CAmInterface::disconnect disconnect from", mSrc, "to", mSink, "failed with", error);
        mpSocketHandler->exit_mainloop();
        return error;
    }

    logInfo("CAmInterface::disconnect" + string(mExitOnRemove ? " FINAL" : ""), error);
    return error;
}

am_Error_e CAmInterface::end()
{
    mExitOnRemove = true;

    if (mMainConnection == 0) {
        logInfo("CAmInterface::end");
        mpSocketHandler->exit_mainloop();
        return E_OK;
    }

    return this->disconnect();
}

void CAmInterface::reconnect(const __time_t seconds)
{
    timespec fire;
    fire.tv_nsec = 50000;
    fire.tv_sec = seconds;
    logInfo("CAmInterface::reconnect from", mSrc, "to", mSink, "called");
    this->disconnect();
    mpSocketHandler->addTimer(fire, &mpCbFunc, mHandle, NULL);
}

void CAmInterface::cbReconnect(sh_timerHandle_t handle, void*)
{
    if (handle != mHandle) {
        logError("CAmInterface::cbReconnect called with unknown handle id");
        return;
    }

    logInfo("CAmInterface::cbReconnect");
    mpSocketHandler->stopTimer(handle);
    this->connect(mSrc, mSink);
}


void CAmInterface::cbRemovedMainConnection(const am_mainConnectionID_t connectionID)
{
    if (connectionID != mMainConnection)
        return;

    mMainConnection = 0;
    logInfo("CAmInterface::cbRemovedMainConnection", connectionID);
    if (mExitOnRemove)
        this->end();
}

am_Error_e CAmInterface::asyncConnect(const am_Handle_s handle, const am_connectionID_t connectionID,
                                const am_sourceID_t sourceID, const am_sinkID_t sinkID,
                                const am_CustomConnectionFormat_t)
{
    am_Error_e error = E_UNKNOWN;

    if (!mpPlayerInterface)
        return E_ABORTED;

    if (sourceID != mSrc) {
        logError("CAmInterface::asyncConnect source", sourceID, error);
        IAmApplicationClient::ackConnect(handle, connectionID, error);
        return E_OK;
    }

    logInfo("CAmInterface::asyncConnect from", sourceID, "to", sinkID);
    mSerializer.syncCall<IPlayer, am_Error_e>(mpPlayerInterface, &IPlayer::initialize, error);
    //error = mpPlayerInterface->initialize();
    if (error != E_OK) {
        IAmApplicationClient::ackConnect(handle, connectionID, error);
        mpSocketHandler->exit_mainloop();
        return E_OK;
    }

    mConnection = connectionID;
    IAmApplicationClient::ackConnect(handle, connectionID, error);
    return E_OK;
}

am_Error_e CAmInterface::asyncDisconnect(const am_Handle_s handle, const am_connectionID_t connectionID)
{
    am_Error_e error = E_UNKNOWN;

    if (!mpPlayerInterface)
        return E_ABORTED;

    if (connectionID != mConnection) {
        logError("CAmInterface::asyncDisconnect connection", connectionID, error);
        IAmApplicationClient::ackDisconnect(handle, connectionID, error);
        return E_OK;
    }

    logInfo("CAmInterface::asyncDisconnect of", connectionID);
    mSerializer.syncCall<IPlayer, am_Error_e>(mpPlayerInterface, &IPlayer::deinitialize, error);
    //error = mpPlayerInterface->deinitialize();
    if (error == E_OK)
        mConnection = 0;
    IAmApplicationClient::ackDisconnect(handle, connectionID, error);
    return E_OK;
}

am_Error_e CAmInterface::asyncSetSourceVolume(const am_Handle_s handle, const am_sourceID_t,
                                        const am_volume_t volume, const am_CustomRampType_t,
                                        const am_time_t)
{
    IAmApplicationClient::ackSetSourceVolumeChange(handle, volume, E_OK);
    return E_OK;
}

am_Error_e CAmInterface::asyncSetSinkVolume(const am_Handle_s handle, const am_sinkID_t,
                                        const am_volume_t volume, const am_CustomRampType_t,
                                        const am_time_t)
{
    IAmApplicationClient::ackSetSinkVolumeChange(handle, volume, E_OK);
    return E_OK;
}

am_Error_e CAmInterface::asyncSetSourceState(const am_Handle_s handle, const am_sourceID_t sourceID,
                                       const am_SourceState_e state)
{
    if (!mpPlayerInterface)
        return E_ABORTED;

    am_Error_e error = E_UNKNOWN;
    if (sourceID != mSrc) {
        logError("CAmInterface::asyncSetSourceState unknown source", sourceID);
        IAmApplicationClient::ackSetSourceState(handle, error);
        return error;
    }

    logInfo("CAmInterface::asyncSetSourceState set state of", sourceID, "to", state);
    switch (state)
    {
        case SS_ON:
            mSerializer.syncCall<IPlayer, am_Error_e>(mpPlayerInterface, &IPlayer::play, error);
            break;
        case SS_PAUSED:
            mSerializer.syncCall<IPlayer, am_Error_e>(mpPlayerInterface, &IPlayer::pause, error);
            break;
        case SS_OFF:
            mSerializer.syncCall<IPlayer, am_Error_e>(mpPlayerInterface, &IPlayer::stop, error);
            break;
        default:
            break;
    }

    IAmApplicationClient::ackSetSourceState(handle, error);
    return E_OK;
}

am_sourceID_t & CAmInterface::getMainSrc()
{
    return mSrc;
}

am_sinkID_t & CAmInterface::getMainSink()
{
    return mSink;
}
