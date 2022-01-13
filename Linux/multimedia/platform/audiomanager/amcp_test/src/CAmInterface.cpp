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
#include "CAmInterface.h"
#include "CAmDltWrapper.h"

using namespace am;
using namespace std;

#define THROW_ASSERT_NEQ(CALL, COND) \
        if (!((CALL) == (COND))) throw std::runtime_error( \
            std::string(__func__) + ": (" + std::string(#CALL) + " != " + std::to_string(COND) + ")")

CAmInterface::CAmInterface(void) :
		IAmCommandClient(), mConnectionID(0), mConnectionState(CS_UNKNOWN)
{
    /* Initialize mutex and condition variable objects */
	THROW_ASSERT_NEQ(pthread_mutex_init(&mMtx, NULL), 0);
	THROW_ASSERT_NEQ(pthread_cond_init(&mCond, NULL), 0);
}

CAmInterface::~CAmInterface()
{
}

void CAmInterface::setCommandReady(const uint16_t handle)
{
    (void) handle;
}

void CAmInterface::setCommandRundown(const uint16_t handle)
{
    (void) handle;
}

void CAmInterface::cbNewMainConnection(const am_MainConnectionType_s& mainConnectionType)
{
    logInfo("CAmInterface::cbNewMainConnection", mainConnectionType.mainConnectionID, "State:", mainConnectionType.connectionState);
}

void CAmInterface::cbRemovedMainConnection(const am_mainConnectionID_t mainConnection)
{
    (void) mainConnection;
}

void CAmInterface::cbNewSink(const am_SinkType_s& sink)
{
    (void) sink;
}

void CAmInterface::cbRemovedSink(const am_sinkID_t sinkID)
{
    (void) sinkID;
}

void CAmInterface::cbNewSource(const am_SourceType_s& source)
{
    (void) source;
}

void CAmInterface::cbRemovedSource(const am_sourceID_t sourceID)
{
    (void) sourceID;
}

void CAmInterface::cbNumberOfSinkClassesChanged()
{

}

void CAmInterface::cbNumberOfSourceClassesChanged()
{

}

am_Error_e CAmInterface::waitForStateChange(const am_mainConnectionID_t connectionID,
                                const am_ConnectionState_e connectionState)
{
    timespec timeToWait;
    int err = 0;

    THROW_ASSERT_NEQ(pthread_mutex_lock(&mMtx), 0);

    /* wait until status and id are equal */
    while ( (err == 0) &&
        ((connectionID != mConnectionID) || (connectionState != mConnectionState)) )
    {
        logInfo("waitForStateChange",connectionID, connectionState, "compare to:", mConnectionID, mConnectionState);
        clock_gettime(CLOCK_REALTIME, &timeToWait);
        timeToWait.tv_sec += 10;
        err = pthread_cond_timedwait(&mCond, &mMtx, &timeToWait);
    }

    /* cleanup */
    mConnectionID = 0;
    mConnectionState = CS_UNKNOWN;

    THROW_ASSERT_NEQ(pthread_mutex_unlock(&mMtx), 0);

    if (err != 0)
        return (err == ETIMEDOUT) ? E_ABORTED : E_UNKNOWN;

    return E_OK;
}


void CAmInterface::cbMainConnectionStateChanged(const am_mainConnectionID_t connectionID,
                                           const am_ConnectionState_e connectionState)
{
	logInfo("CAmInterface::cbMainConnectionStateChanged", connectionID, connectionState);
	THROW_ASSERT_NEQ(pthread_mutex_lock(&mMtx), 0);
    mConnectionID = connectionID;
    mConnectionState = connectionState;
    THROW_ASSERT_NEQ(pthread_cond_signal(&mCond), 0);
    THROW_ASSERT_NEQ(pthread_mutex_unlock(&mMtx), 0);
}

void CAmInterface::cbMainSinkSoundPropertyChanged(const am_sinkID_t sinkID, const am_MainSoundProperty_s& soundProperty)
{
    (void) sinkID;
    (void) soundProperty;
}

void CAmInterface::cbMainSourceSoundPropertyChanged(const am_sourceID_t sourceID, const am_MainSoundProperty_s& soundProperty)
{
    (void) sourceID;
    (void) soundProperty;
}

void CAmInterface::cbSinkAvailabilityChanged(const am_sinkID_t sinkID, const am_Availability_s& availability)
{
    (void) sinkID;
    (void) availability;
}

void CAmInterface::cbSourceAvailabilityChanged(const am_sourceID_t sourceID, const am_Availability_s& availability)
{
    (void) sourceID;
    (void) availability;
}

void CAmInterface::cbVolumeChanged(const am_sinkID_t sinkID, const am_mainVolume_t volume)
{
    (void) sinkID;
    (void) volume;
}

void CAmInterface::cbSinkMuteStateChanged(const am_sinkID_t sinkID, const am_MuteState_e muteState)
{
    (void) sinkID;
    (void) muteState;
}

void CAmInterface::cbSystemPropertyChanged(const am_SystemProperty_s& systemProperty)
{
    (void) systemProperty;
}

void CAmInterface::cbTimingInformationChanged(const am_mainConnectionID_t mainConnectionID, const am_timeSync_t time)
{
    (void) mainConnectionID;
    (void) time;
}

void CAmInterface::cbSinkUpdated(const am_sinkID_t sinkID, const am_sinkClass_t sinkClassID,
                            const vector<am_MainSoundProperty_s>& listMainSoundProperties)
{
    (void) sinkID;
    (void) sinkClassID;
    (void) listMainSoundProperties;
}

void CAmInterface::cbSourceUpdated(const am_sourceID_t sourceID, const am_sourceClass_t sourceClassID,
                              const vector<am_MainSoundProperty_s>& listMainSoundProperties)
{
    (void) sourceID;
    (void) sourceClassID;
    (void) listMainSoundProperties;
}

void CAmInterface::cbSinkNotification(const am_sinkID_t sinkID, const am_NotificationPayload_s& notification)
{
    (void) sinkID;
    (void) notification;
}

void CAmInterface::cbSourceNotification(const am_sourceID_t sourceID, const am_NotificationPayload_s& notification)
{
    (void) sourceID;
    (void) notification;
}

void CAmInterface::cbMainSinkNotificationConfigurationChanged(const am_sinkID_t sinkID,
                                                         const am_NotificationConfiguration_s& mainNotificationConfiguration)
{
    (void) sinkID;
    (void) mainNotificationConfiguration;
}

void CAmInterface::cbMainSourceNotificationConfigurationChanged(
        const am_sourceID_t sourceID, const am_NotificationConfiguration_s& mainNotificationConfiguration)
{
    (void) sourceID;
    (void) mainNotificationConfiguration;
}
