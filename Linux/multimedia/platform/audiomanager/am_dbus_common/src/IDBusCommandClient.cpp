/************************************************************************
 * @file: IDBusCommandClient.cpp
 *
 * @version: 1.1
 *
 * @description: IAmCommandClient is a common interface class for
 * both receiver and sender command plug-in interface of AM.
 * @component: platform/audiomanager
 *
 * @author: Jens Lorenz, jlorenz@de.adit-jv.com 2016
 *          Mattia Guerra, mguerra@de.adit-jv.com 2016
 *
 * @copyright (c) 2016 Advanced Driver Information Technology.
 * This code is developed by Advanced Driver Information Technology.
 * Copyright of Advanced Driver Information Technology, Bosch, and DENSO.
 * All rights reserved.
 *
 * @see <related items>
 *
 * @history
 *
 ***********************************************************************/

#include <assert.h>
#include "CAmDltWrapper.h"
#include "IDBusCommandClient.h"
#include "CDBusCommandSender.h"
#include "CDBusCommon.h"
#include "IDBusCommandReceiver.h"

DLT_DECLARE_CONTEXT (CP_CommandClientCore)


#define throw_assert(EXPRESSION, MESSAGE) \
        if (!(EXPRESSION)) throw std::runtime_error(MESSAGE)

using namespace am;
using namespace std;

IDBusCommandClient::IDBusCommandClient()
{
    CAmDltWrapper::instance()->registerContext(CP_CommandClientCore, "CCIF", "Command Client Interface");
    int ret = pthread_mutex_init(&mMutex, NULL);
    throw_assert(ret == 0, "Mutex");
}

IDBusCommandClient::~IDBusCommandClient()
{

}

am_Error_e IDBusCommandClient::connect(const am_sourceID_t sourceID, const am_sinkID_t sinkID, am_mainConnectionID_t& mainConnectionID)
{
    assert(mpIAmCommandReceive != NULL);
    assert(mpSerializer != NULL);
    am_Error_e error = E_UNKNOWN;
    am_sourceID_t _sourceID = sourceID;
    am_sinkID_t _sinkID = sinkID;
    pthread_mutex_lock(&mMutex);
    mpSerializer->syncCall(mpIAmCommandReceive, &IAmCommandReceive::connect, error, _sourceID, _sinkID, mainConnectionID);
    pthread_mutex_unlock(&mMutex);
    return error;
}

am_Error_e IDBusCommandClient::disconnect(const am_mainConnectionID_t mainConnectionID)
{
    assert(mpIAmCommandReceive != NULL);
    assert(mpSerializer != NULL);
    am_Error_e error = E_UNKNOWN;
    am_mainConnectionID_t _mainConnectionID = mainConnectionID;
    pthread_mutex_lock(&mMutex);
    mpSerializer->syncCall(mpIAmCommandReceive, &IAmCommandReceive::disconnect, error, _mainConnectionID);
    pthread_mutex_unlock(&mMutex);
    return error;
}

am_Error_e IDBusCommandClient::setVolume(const am_sinkID_t sinkID, const am_mainVolume_t volume)
{
    assert(mpIAmCommandReceive != NULL);
    assert(mpSerializer != NULL);
    am_Error_e error = E_UNKNOWN;
    am_sinkID_t _sinkID = sinkID;
    am_mainVolume_t _volume = volume;
    pthread_mutex_lock(&mMutex);
    mpSerializer->syncCall(mpIAmCommandReceive, &IAmCommandReceive::setVolume, error, _sinkID, _volume);
    pthread_mutex_unlock(&mMutex);
    return error;
}

am_Error_e IDBusCommandClient::volumeStep(const am_sinkID_t sinkID, const int16_t volumeStep)
{
    assert(mpIAmCommandReceive != NULL);
    assert(mpSerializer != NULL);
    am_Error_e error = E_UNKNOWN;
    am_sinkID_t _sinkID = sinkID;
    int16_t _stepVolume = volumeStep;
    pthread_mutex_lock(&mMutex);
    mpSerializer->syncCall(mpIAmCommandReceive, &IAmCommandReceive::volumeStep, error, _sinkID, _stepVolume);
    pthread_mutex_unlock(&mMutex);
    return error;
}

am_Error_e IDBusCommandClient::setSinkMuteState(const am_sinkID_t sinkID, const am_MuteState_e muteState)
{
    assert(mpIAmCommandReceive != NULL);
    assert(mpSerializer != NULL);
    am_Error_e error = E_UNKNOWN;
    am_sinkID_t _sinkID = sinkID;
    am_MuteState_e _muteState = muteState;
    pthread_mutex_lock(&mMutex);
    mpSerializer->syncCall(mpIAmCommandReceive, &IAmCommandReceive::setSinkMuteState, error, _sinkID, _muteState);
    pthread_mutex_unlock(&mMutex);
    return error;
}

am_Error_e IDBusCommandClient::setMainSinkSoundProperty(const am_MainSoundProperty_s& soundProperty, const am_sinkID_t sinkID)
{
    assert(mpIAmCommandReceive != NULL);
    assert(mpSerializer != NULL);
    am_Error_e error = E_UNKNOWN;
    am_MainSoundProperty_s _soundProperty = soundProperty;
    am_sinkID_t _sinkID = sinkID;
    pthread_mutex_lock(&mMutex);
    mpSerializer->syncCall(mpIAmCommandReceive, &IAmCommandReceive::setMainSinkSoundProperty, error, _soundProperty, _sinkID);
    pthread_mutex_unlock(&mMutex);
    return error;
}

am_Error_e IDBusCommandClient::setMainSourceSoundProperty(const am_MainSoundProperty_s& soundProperty, const am_sourceID_t sourceID)
{
    assert(mpIAmCommandReceive != NULL);
    assert(mpSerializer != NULL);
    am_Error_e error = E_UNKNOWN;
    am_MainSoundProperty_s _soundProperty = soundProperty;
    am_sourceID_t _sourceID = sourceID;
    pthread_mutex_lock(&mMutex);
    mpSerializer->syncCall(mpIAmCommandReceive, &IAmCommandReceive::setMainSourceSoundProperty, error, _soundProperty, _sourceID);
    pthread_mutex_unlock(&mMutex);
    return error;
}

am_Error_e IDBusCommandClient::setSystemProperty(const am_SystemProperty_s& property)
{
    assert(mpIAmCommandReceive != NULL);
    assert(mpSerializer != NULL);
    am_Error_e error = E_UNKNOWN;
    am_SystemProperty_s _property = property;
    pthread_mutex_lock(&mMutex);
    mpSerializer->syncCall(mpIAmCommandReceive, &IAmCommandReceive::setSystemProperty, error, _property);
    pthread_mutex_unlock(&mMutex);
    return error;
}

am_Error_e IDBusCommandClient::getListMainConnections(vector<am_MainConnectionType_s>& listConnections)
{
    assert(mpIAmCommandReceive != NULL);
    assert(mpSerializer != NULL);
    am_Error_e error = E_UNKNOWN;
    pthread_mutex_lock(&mMutex);
    mpSerializer->syncCall(mpIAmCommandReceive, &IAmCommandReceive::getListMainConnections, error, listConnections);
    pthread_mutex_unlock(&mMutex);
    return error;
}

am_Error_e IDBusCommandClient::getListMainSinks(vector<am_SinkType_s>& listMainSinks)
{
    assert(mpIAmCommandReceive != NULL);
    assert(mpSerializer != NULL);
    am_Error_e error = E_UNKNOWN;
    pthread_mutex_lock(&mMutex);
    mpSerializer->syncCall(mpIAmCommandReceive, &IAmCommandReceive::getListMainSinks, error, listMainSinks);
    pthread_mutex_unlock(&mMutex);
    return error;
}

am_Error_e IDBusCommandClient::getListMainSources(vector<am_SourceType_s>& listMainSources)
{
    assert(mpIAmCommandReceive != NULL);
    assert(mpSerializer != NULL);
    am_Error_e error = E_UNKNOWN;
    pthread_mutex_lock(&mMutex);
    mpSerializer->syncCall(mpIAmCommandReceive, &IAmCommandReceive::getListMainSources, error, listMainSources);
    pthread_mutex_unlock(&mMutex);
    return error;
}

am_Error_e IDBusCommandClient::getListMainSinkSoundProperties(const am_sinkID_t sinkID,
                                          vector<am_MainSoundProperty_s>& listSoundProperties)
{
    assert(mpIAmCommandReceive != NULL);
    assert(mpSerializer != NULL);
    am_Error_e error = E_UNKNOWN;
    am_sinkID_t _sinkID = sinkID;
    pthread_mutex_lock(&mMutex);
    mpSerializer->syncCall(mpIAmCommandReceive, &IAmCommandReceive::getListMainSinkSoundProperties, error, _sinkID, listSoundProperties);
    pthread_mutex_unlock(&mMutex);
    return error;
}

am_Error_e IDBusCommandClient::getListMainSourceSoundProperties(const am_sourceID_t sourceID,
                                            vector<am_MainSoundProperty_s>& listSourceProperties)
{
    assert(mpIAmCommandReceive != NULL);
    assert(mpSerializer != NULL);
    am_Error_e error = E_UNKNOWN;
    am_sourceID_t _sourceID = sourceID;
    pthread_mutex_lock(&mMutex);
    mpSerializer->syncCall(mpIAmCommandReceive, &IAmCommandReceive::getListMainSourceSoundProperties, error, _sourceID, listSourceProperties);
    pthread_mutex_unlock(&mMutex);
    return error;
}

am_Error_e IDBusCommandClient::getListSourceClasses(vector<am_SourceClass_s>& listSourceClasses)
{
    assert(mpIAmCommandReceive != NULL);
    assert(mpSerializer != NULL);
    am_Error_e error = E_UNKNOWN;
    pthread_mutex_lock(&mMutex);
    mpSerializer->syncCall(mpIAmCommandReceive, &IAmCommandReceive::getListSourceClasses, error, listSourceClasses);
    pthread_mutex_unlock(&mMutex);
    return error;
}

am_Error_e IDBusCommandClient::getListSinkClasses(vector<am_SinkClass_s>& listSinkClasses)
{
    assert(mpIAmCommandReceive != NULL);
    assert(mpSerializer != NULL);
    am_Error_e error = E_UNKNOWN;
    pthread_mutex_lock(&mMutex);
    mpSerializer->syncCall(mpIAmCommandReceive, &IAmCommandReceive::getListSinkClasses, error, listSinkClasses);
    pthread_mutex_unlock(&mMutex);
    return error;
}

am_Error_e IDBusCommandClient::getListSystemProperties(vector<am_SystemProperty_s>& listSystemProperties)
{
    assert(mpIAmCommandReceive != NULL);
    assert(mpSerializer != NULL);
    am_Error_e error = E_UNKNOWN;
    pthread_mutex_lock(&mMutex);
    mpSerializer->syncCall(mpIAmCommandReceive, &IAmCommandReceive::getListSystemProperties, error, listSystemProperties);
    pthread_mutex_unlock(&mMutex);
    return error;
}

am_Error_e IDBusCommandClient::getTimingInformation(const am_mainConnectionID_t mainConnectionID, am_timeSync_t& delay)
{
    assert(mpIAmCommandReceive != NULL);
    assert(mpSerializer != NULL);
    am_Error_e error = E_UNKNOWN;
    am_mainConnectionID_t _mainConnectionID = mainConnectionID;
    pthread_mutex_lock(&mMutex);
    mpSerializer->syncCall(mpIAmCommandReceive, &IAmCommandReceive::getTimingInformation, error, _mainConnectionID, delay);
    pthread_mutex_unlock(&mMutex);
    return error;
}

void IDBusCommandClient::confirmCommandReady(const uint16_t handle, const am_Error_e error)
{
    assert(mpIAmCommandReceive != NULL);
    pthread_mutex_lock(&mMutex);
    mpSerializer->asyncCall(mpIAmCommandReceive, &IAmCommandReceive::confirmCommandReady, handle, error);
    pthread_mutex_unlock(&mMutex);
}

void IDBusCommandClient::confirmCommandRundown(const uint16_t handle, const am_Error_e error)
{
    assert(mpIAmCommandReceive != NULL);
    pthread_mutex_lock(&mMutex);
    mpSerializer->asyncCall(mpIAmCommandReceive, &IAmCommandReceive::confirmCommandRundown, handle, error);
    pthread_mutex_unlock(&mMutex);
}

am_Error_e IDBusCommandClient::getListMainSinkNotificationConfigurations(
        const am_sinkID_t sinkID, vector<am_NotificationConfiguration_s>& listMainNotificationConfigurations)
{
    assert(mpIAmCommandReceive != NULL);
    assert(mpSerializer != NULL);
    am_Error_e error = E_UNKNOWN;
    am_sinkID_t _sinkID = sinkID;
    pthread_mutex_lock(&mMutex);
    mpSerializer->syncCall(mpIAmCommandReceive, &IAmCommandReceive::getListMainSinkNotificationConfigurations, error, _sinkID, listMainNotificationConfigurations);
    pthread_mutex_unlock(&mMutex);
    return error;
}

am_Error_e IDBusCommandClient::getListMainSourceNotificationConfigurations(
        const am_sourceID_t sourceID, vector<am_NotificationConfiguration_s>& listMainNotificationConfigurations)
{
    assert(mpIAmCommandReceive != NULL);
    assert(mpSerializer != NULL);
    am_Error_e error = E_UNKNOWN;
    am_sourceID_t _sourceID = sourceID;
    pthread_mutex_lock(&mMutex);
    mpSerializer->syncCall(mpIAmCommandReceive, &IAmCommandReceive::getListMainSourceNotificationConfigurations, error, _sourceID, listMainNotificationConfigurations);
    pthread_mutex_unlock(&mMutex);
    return error;
}

am_Error_e IDBusCommandClient::setMainSinkNotificationConfiguration(const am_sinkID_t sinkID,
                                                const am_NotificationConfiguration_s& mainNotificationConfiguration)
{
    assert(mpIAmCommandReceive != NULL);
    assert(mpSerializer != NULL);
    am_Error_e error = E_UNKNOWN;
    am_sinkID_t _sinkID = sinkID;
    am_NotificationConfiguration_s _mainNotificationConfiguration = mainNotificationConfiguration;
    pthread_mutex_lock(&mMutex);
    mpSerializer->syncCall(mpIAmCommandReceive, &IAmCommandReceive::setMainSinkNotificationConfiguration, error, _sinkID, _mainNotificationConfiguration);
    pthread_mutex_unlock(&mMutex);
    return error;
}

am_Error_e IDBusCommandClient::setMainSourceNotificationConfiguration(const am_sourceID_t sourceID,
                                                  const am_NotificationConfiguration_s& mainNotificationConfiguration)
{
    assert(mpIAmCommandReceive != NULL);
    assert(mpSerializer != NULL);
    am_Error_e error = E_UNKNOWN;
    am_sourceID_t _sourceID = sourceID;
    am_NotificationConfiguration_s _mainNotificationConfiguration = mainNotificationConfiguration;
    pthread_mutex_lock(&mMutex);
    mpSerializer->syncCall(mpIAmCommandReceive, &IAmCommandReceive::setMainSourceNotificationConfiguration, error, _sourceID, _mainNotificationConfiguration);
    pthread_mutex_unlock(&mMutex);
    return error;
}

am_Error_e IDBusCommandClient::getVolume(const am_sinkID_t sinkID, am_mainVolume_t& mainVolume)
{
    assert(mpIAmCommandReceive != NULL);
    assert(mpSerializer != NULL);
    am_Error_e error = E_UNKNOWN;
    am_sinkID_t _sinkID = sinkID;
    pthread_mutex_lock(&mMutex);
    mpSerializer->syncCall(mpIAmCommandReceive, &IAmCommandReceive::getVolume, error, _sinkID, mainVolume);
    pthread_mutex_unlock(&mMutex);
    return error;
}
