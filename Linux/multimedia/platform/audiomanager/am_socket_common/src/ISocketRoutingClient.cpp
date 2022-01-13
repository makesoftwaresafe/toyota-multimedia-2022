/************************************************************************
 * @file: IDBusRoutingClient.h
 *
 * @version: 1.1
 *
 * @description: IAmRoutingClient is a common interface class
 * for both receiver and sender routing plug-in interface of AM.
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
#include <algorithm>
#include <assert.h>
#include "CAmDltWrapper.h"
#include "ISocketRoutingClient.h"
#include "CSocketRoutingSender.h"
#include "ISocketRoutingReceiver.h"
#include "CSocketCommon.h"

DLT_DECLARE_CONTEXT (RoutingClientCore)

using namespace am;
using namespace std;

ISocketRoutingClient::ISocketRoutingClient(std::string dep1, std::string dep2, const std::string& interfaceName)
		:mpIAmRoutingReceive(NULL), mpSerializer(NULL)
{
	(void)dep1;
	(void)dep2;
	(void)interfaceName;
	logInfo(string(__func__) +
			string(" is depreciated. Please use instead IAmRoutingClient::IAmRoutingClient(const std::string&, DBusBusType, CAmSocketHandler*)"));

	// Important for backward compatibility! Only the interface name is required for construction, e.g.:
	// "audioserver" of "org.adit.audiomanager.routing.audioserver" will be taken!
	this->constructor();
}

ISocketRoutingClient::ISocketRoutingClient()
		:mpIAmRoutingReceive(NULL), mpSerializer(NULL)
{
	logDebug(string(__func__) + string(" Called"));
	this->constructor();
}

void ISocketRoutingClient::constructor()
{
    CAmDltWrapper::instance()->registerContext(RoutingClientCore, "CCIF", "Command Client Interface");
}

ISocketRoutingClient::~ISocketRoutingClient()
{
	logDebug(string(__func__) + string(" Called"));
}

void ISocketRoutingClient::ackConnect(const am_Handle_s handle, const am_connectionID_t connectionID,
                                  const am_Error_e error)
{
	logDebug("ISocketRoutingClient::", __func__," called start");

    assert(mpIAmRoutingReceive != NULL);

	mpIAmRoutingReceive->ackConnect(handle, connectionID, error);

	logDebug("ISocketRoutingClient::", __func__," called end");
}

void ISocketRoutingClient::ackDisconnect(const am_Handle_s handle, const am_connectionID_t connectionID,
                                     const am_Error_e error)
{
	logDebug("ISocketRoutingClient::", __func__," called start");

    assert(mpIAmRoutingReceive != NULL);

	mpIAmRoutingReceive->ackDisconnect(handle, connectionID, error);

	logDebug("ISocketRoutingClient::", __func__," called end");
}

void ISocketRoutingClient::ackSetSinkVolumeChange(const am_Handle_s handle, const am_volume_t volume,
                                              const am_Error_e error)
{
	logDebug("ISocketRoutingClient::", __func__," called start");

    assert(mpIAmRoutingReceive != NULL);

	mpIAmRoutingReceive->ackSetSinkVolumeChange(handle, volume, error);

	logDebug("ISocketRoutingClient::", __func__," called end");
}

void ISocketRoutingClient::ackSetSourceVolumeChange(const am_Handle_s handle, const am_volume_t volume,
                                                const am_Error_e error)
{
	logDebug("ISocketRoutingClient::", __func__," called start");

    assert(mpIAmRoutingReceive != NULL);

	mpIAmRoutingReceive->ackSetSinkVolumeChange(handle, volume, error);

	logDebug("ISocketRoutingClient::", __func__," called end");
}

void ISocketRoutingClient::ackSetSourceState(const am_Handle_s handle, const am_Error_e error)
{
	logDebug("ISocketRoutingClient::", __func__," called start");

    assert(mpIAmRoutingReceive != NULL);

	mpIAmRoutingReceive->ackSetSourceState(handle, error);

	logDebug("ISocketRoutingClient::", __func__," called end");
}

void ISocketRoutingClient::ackSetSinkSoundProperties(const am_Handle_s handle, const am_Error_e error)
{
	logDebug("ISocketRoutingClient::", __func__," called start");

    assert(mpIAmRoutingReceive != NULL);

	mpIAmRoutingReceive->ackSetSinkSoundProperties(handle, error);

	logDebug("ISocketRoutingClient::", __func__," called end");
}

void ISocketRoutingClient::ackSetSinkSoundProperty(const am_Handle_s handle, const am_Error_e error)
{
	logDebug("ISocketRoutingClient::", __func__," called start");

    assert(mpIAmRoutingReceive != NULL);

	mpIAmRoutingReceive->ackSetSinkSoundProperty(handle, error);

	logDebug("ISocketRoutingClient::", __func__," called end");
}

void ISocketRoutingClient::ackSetSourceSoundProperties(const am_Handle_s handle, const am_Error_e error)
{
	logDebug("ISocketRoutingClient::", __func__," called start");

    assert(mpIAmRoutingReceive != NULL);

	mpIAmRoutingReceive->ackSetSourceSoundProperties(handle, error);

	logDebug("ISocketRoutingClient::", __func__," called end");
}

void ISocketRoutingClient::ackSetSourceSoundProperty(const am_Handle_s handle, const am_Error_e error)
{
	logDebug("ISocketRoutingClient::", __func__," called start");

    assert(mpIAmRoutingReceive != NULL);

	mpIAmRoutingReceive->ackSetSourceSoundProperty(handle, error);

	logDebug("ISocketRoutingClient::", __func__," called end");
}

void ISocketRoutingClient::ackCrossFading(const am_Handle_s handle, const am_HotSink_e hotSink, const am_Error_e error)
{
	logDebug("ISocketRoutingClient::", __func__," called start");

    assert(mpIAmRoutingReceive != NULL);

	mpIAmRoutingReceive->ackCrossFading(handle, hotSink, error);

	logDebug("ISocketRoutingClient::", __func__," called end");
}

void ISocketRoutingClient::ackSourceVolumeTick(const am_Handle_s handle, const am_sourceID_t sourceID,
                                           const am_volume_t volume)
{
	logDebug("ISocketRoutingClient::", __func__," called start");

    assert(mpIAmRoutingReceive != NULL);

	mpIAmRoutingReceive->ackSourceVolumeTick(handle, sourceID, volume);

	logDebug("ISocketRoutingClient::", __func__," called end");
}

void ISocketRoutingClient::ackSinkVolumeTick(const am_Handle_s handle, const am_sinkID_t sinkID, const am_volume_t volume)
{
	logDebug("ISocketRoutingClient::", __func__," called start");

    assert(mpIAmRoutingReceive != NULL);

	mpIAmRoutingReceive->ackSinkVolumeTick(handle, sinkID, volume);

	logDebug("ISocketRoutingClient::", __func__," called end");
}

am_Error_e ISocketRoutingClient::peekDomain(const string& name, am_domainID_t& domainID)
{
	logDebug("ISocketRoutingClient::", __func__," called start");

    assert(mpIAmRoutingReceive != NULL);
    am_Error_e err = E_OK;

	err = mpIAmRoutingReceive->peekDomain(name, domainID);
	if( err != E_OK){
		logError(__func__, " error = ", err);
	}
	logDebug("ISocketRoutingClient::", __func__," called end");

	return err;
}

am_Error_e ISocketRoutingClient::registerDomain(const am_Domain_s& domainData, am_domainID_t& domainID)
{
	logDebug("ISocketRoutingClient::", __func__," called start");

    assert(mpIAmRoutingReceive != NULL);
    am_Error_e err = E_OK;

	err = mpIAmRoutingReceive->registerDomain(domainData, domainID);
	if( err != E_OK){
		logError(__func__, " error = ", err);
	}
	logDebug("ISocketRoutingClient::", __func__," called end");

	return err;
}

am_Error_e ISocketRoutingClient::deregisterDomain(const am_domainID_t domainID)
{
	logDebug("ISocketRoutingClient::", __func__," called start");

    assert(mpIAmRoutingReceive != NULL);
    am_Error_e err = E_OK;

	err = mpIAmRoutingReceive->deregisterDomain(domainID);
	if( err != E_OK){
		logError(__func__, " error = ", err);
	}
	logDebug("ISocketRoutingClient::", __func__," called end");

	return err;
}

am_Error_e ISocketRoutingClient::registerGateway(const am_Gateway_s& gatewayData, am_gatewayID_t& gatewayID)
{
	logDebug("ISocketRoutingClient::", __func__," called start");

    assert(mpIAmRoutingReceive != NULL);
    am_Error_e err = E_OK;

	err = mpIAmRoutingReceive->registerGateway(gatewayData, gatewayID);
	if( err != E_OK){
		logError(__func__, " error = ", err);
	}
	logDebug("ISocketRoutingClient::", __func__," called end");

	return err;
}

am_Error_e ISocketRoutingClient::deregisterGateway(const am_gatewayID_t gatewayID)
{
	logDebug("ISocketRoutingClient::", __func__," called start");

    assert(mpIAmRoutingReceive != NULL);
    am_Error_e err = E_OK;

	err = mpIAmRoutingReceive->deregisterGateway(gatewayID);
	if( err != E_OK){
		logError(__func__, " error = ", err);
	}
	logDebug("ISocketRoutingClient::", __func__," called end");

	return err;
}

am_Error_e ISocketRoutingClient::peekSink(const string& name, am_sinkID_t& sinkID)
{
	logDebug("ISocketRoutingClient::", __func__," called start");

    assert(mpIAmRoutingReceive != NULL);
    am_Error_e err = E_OK;

	err = mpIAmRoutingReceive->peekSink(name, sinkID);
	if( err != E_OK){
		logError(__func__, " error = ", err);
	}
	logDebug("ISocketRoutingClient::", __func__," called end");

	return err;
}

am_Error_e ISocketRoutingClient::registerSink(const am_Sink_s& sinkData, am_sinkID_t& sinkID)
{
	logDebug("ISocketRoutingClient::", __func__," called start");

    assert(mpIAmRoutingReceive != NULL);
    am_Error_e err = E_OK;

	err = mpIAmRoutingReceive->registerSink(sinkData, sinkID);
	if( err != E_OK){
		logError(__func__, " error = ", err);
	}
	logDebug("ISocketRoutingClient::", __func__," called end");

	return err;
}

am_Error_e ISocketRoutingClient::deregisterSink(const am_sinkID_t sinkID)
{
	logDebug("ISocketRoutingClient::", __func__," called start");

    assert(mpIAmRoutingReceive != NULL);
    am_Error_e err = E_OK;

	err = mpIAmRoutingReceive->deregisterSink(sinkID);
	if( err != E_OK){
		logError(__func__, " error = ", err);
	}
	logDebug("ISocketRoutingClient::", __func__," called end");

	return err;
}

am_Error_e ISocketRoutingClient::peekSource(const string& name, am_sourceID_t& sourceID)
{
	logDebug("ISocketRoutingClient::", __func__," called start");

    assert(mpIAmRoutingReceive != NULL);
    am_Error_e err = E_OK;

	err = mpIAmRoutingReceive->peekSource(name, sourceID);
	if( err != E_OK){
		logError(__func__, " error = ", err);
	}
	logDebug("ISocketRoutingClient::", __func__," called end");

	return err;
}

am_Error_e ISocketRoutingClient::registerSource(const am_Source_s& sourceData, am_sourceID_t& sourceID)
{
	logDebug("ISocketRoutingClient::", __func__," called start");

    assert(mpIAmRoutingReceive != NULL);
    am_Error_e err = E_OK;

	err = mpIAmRoutingReceive->registerSource(sourceData, sourceID);
	if( err != E_OK){
		logError(__func__, " error = ", err);
	}
	logDebug("ISocketRoutingClient::", __func__," called end");

	return err;
}

am_Error_e ISocketRoutingClient::deregisterSource(const am_sourceID_t sourceID)
{
	logDebug("ISocketRoutingClient::", __func__," called start");

    assert(mpIAmRoutingReceive != NULL);
    am_Error_e err = E_OK;

	err = mpIAmRoutingReceive->deregisterSource(sourceID);
	if( err != E_OK){
		logError(__func__, " error = ", err);
	}
	logDebug("ISocketRoutingClient::", __func__," called end");

	return err;
}

am_Error_e ISocketRoutingClient::registerCrossfader(const am_Crossfader_s& crossfaderData, am_crossfaderID_t& crossfaderID)
{
	logDebug("ISocketRoutingClient::", __func__," called start");

    assert(mpIAmRoutingReceive != NULL);
    am_Error_e err = E_OK;

	err = mpIAmRoutingReceive->registerCrossfader(crossfaderData, crossfaderID);
	if( err != E_OK){
		logError(__func__, " error = ", err);
	}
	logDebug("ISocketRoutingClient::", __func__," called end");

	return err;
}

am_Error_e ISocketRoutingClient::deregisterCrossfader(const am_crossfaderID_t crossfaderID)
{
	logDebug("ISocketRoutingClient::", __func__," called start");

    assert(mpIAmRoutingReceive != NULL);
    am_Error_e err = E_OK;

	err = mpIAmRoutingReceive->deregisterCrossfader(crossfaderID);
	if( err != E_OK){
		logError(__func__, " error = ", err);
	}
	logDebug("ISocketRoutingClient::", __func__," called end");

	return err;
}

am_Error_e ISocketRoutingClient::peekSourceClassID(const string& name, am_sourceClass_t& sourceClassID)
{
	logDebug("ISocketRoutingClient::", __func__," called start");

    assert(mpIAmRoutingReceive != NULL);
    am_Error_e err = E_OK;

	err = mpIAmRoutingReceive->peekSourceClassID(name, sourceClassID);
	if( err != E_OK){
		logError(__func__, " error = ", err);
	}
	logDebug("ISocketRoutingClient::", __func__," called end");

	return err;

}

am_Error_e ISocketRoutingClient::peekSinkClassID(const string& name, am_sinkClass_t& sinkClassID)
{
	logDebug("ISocketRoutingClient::", __func__," called start");

    assert(mpIAmRoutingReceive != NULL);
    am_Error_e err = E_OK;

	err = mpIAmRoutingReceive->peekSinkClassID(name, sinkClassID);
	if( err != E_OK){
		logError(__func__, " error = ", err);
	}
	logDebug("ISocketRoutingClient::", __func__," called end");

	return err;

}

void ISocketRoutingClient::hookInterruptStatusChange(const am_sourceID_t sourceID, const am_InterruptState_e interruptState)
{
	logDebug("ISocketRoutingClient::", __func__," called start");

    assert(mpIAmRoutingReceive != NULL);

	mpIAmRoutingReceive->hookInterruptStatusChange(sourceID, interruptState);

	logDebug("ISocketRoutingClient::", __func__," called end");
}

void ISocketRoutingClient::hookDomainRegistrationComplete(const am_domainID_t domainID)
{
	logDebug("ISocketRoutingClient::", __func__," called start");

    assert(mpIAmRoutingReceive != NULL);

	mpIAmRoutingReceive->hookDomainRegistrationComplete(domainID);

	logDebug("ISocketRoutingClient::", __func__," called end");
}

void ISocketRoutingClient::hookSinkAvailablityStatusChange(const am_sinkID_t sinkID, const am_Availability_s& availability)
{
	logDebug("ISocketRoutingClient::", __func__," called start");

    assert(mpIAmRoutingReceive != NULL);

	mpIAmRoutingReceive->hookSinkAvailablityStatusChange(sinkID, availability);

	logDebug("ISocketRoutingClient::", __func__," called end");
}

void ISocketRoutingClient::hookSourceAvailablityStatusChange(const am_sourceID_t sourceID,
                                                         const am_Availability_s& availability)
{
	logDebug("ISocketRoutingClient::", __func__," called start");

    assert(mpIAmRoutingReceive != NULL);

	mpIAmRoutingReceive->hookSourceAvailablityStatusChange(sourceID, availability);

	logDebug("ISocketRoutingClient::", __func__," called end");
}

void ISocketRoutingClient::hookDomainStateChange(const am_domainID_t domainID, const am_DomainState_e domainState)
{
	logDebug("ISocketRoutingClient::", __func__," called start");

    assert(mpIAmRoutingReceive != NULL);

	mpIAmRoutingReceive->hookDomainStateChange(domainID, domainState);

	logDebug("ISocketRoutingClient::", __func__," called end");
}

void ISocketRoutingClient::hookTimingInformationChanged(const am_connectionID_t connectionID, const am_timeSync_t delay)
{
	logDebug("ISocketRoutingClient::", __func__," called start");

    assert(mpIAmRoutingReceive != NULL);

	mpIAmRoutingReceive->hookTimingInformationChanged(connectionID, delay);

	logDebug("ISocketRoutingClient::", __func__," called end");
}

void ISocketRoutingClient::sendChangedData(const vector<am_EarlyData_s>& earlyData)
{
	logDebug("ISocketRoutingClient::", __func__," called start");

    assert(mpIAmRoutingReceive != NULL);

	mpIAmRoutingReceive->sendChangedData(earlyData);

	logDebug("ISocketRoutingClient::", __func__," called end");
}

void ISocketRoutingClient::confirmRoutingReady(const uint16_t handle, const am_Error_e error)
{
	logDebug(__func__, " called, handle=", handle, ", error=", error);
    assert(mpIAmRoutingReceive != NULL);

    mpIAmRoutingReceive->confirmRoutingReady(handle, error);

	logDebug("ISocketRoutingClient::", __func__," called end");
}

bool ISocketRoutingClient::getRoutingReady(void)
{
	logDebug("ISocketRoutingClient::", __func__," called");
    assert(mpIAmRoutingReceive != NULL);
    return ((ISocketRoutingReceiver*)mpIAmRoutingReceive)->getRoutingReady();
}

void ISocketRoutingClient::confirmRoutingRundown(const uint16_t handle, const am_Error_e error)
{
	(void)handle;
	(void)error;

	logDebug("ISocketRoutingClient::", __func__," called start");

    assert(mpIAmRoutingReceive != NULL);

	logDebug("ISocketRoutingClient::", __func__," called end");
}

am_Error_e ISocketRoutingClient::updateGateway(const am_gatewayID_t gatewayID,
                                           const vector<am_CustomConnectionFormat_t>& listSourceFormats,
                                           const vector<am_CustomConnectionFormat_t>& listSinkFormats,
                                           const vector<bool>& convertionMatrix)
{
	(void)gatewayID;
	(void)listSourceFormats;
	(void)listSinkFormats;
	(void)convertionMatrix;

	logDebug("ISocketRoutingClient::", __func__," called start.");
    assert(mpIAmRoutingReceive != NULL);
    am_Error_e err = E_OK;
	logDebug("ISocketRoutingClient::", __func__," called end");
	return err;
}

am_Error_e ISocketRoutingClient::updateSink(const am_sinkID_t sinkID, const am_sinkClass_t sinkClassID,
                                        const vector<am_SoundProperty_s>& listSoundProperties,
                                        const vector<am_CustomConnectionFormat_t>& listConnectionFormats,
                                        const vector<am_MainSoundProperty_s>& listMainSoundProperties)
{
	logDebug("ISocketRoutingClient::", __func__," called start");

    assert(mpIAmRoutingReceive != NULL);
    am_Error_e err = E_OK;

	err = mpIAmRoutingReceive->updateSink(sinkID, sinkClassID, listSoundProperties, listConnectionFormats, listMainSoundProperties);
	if( err != E_OK)	{
		logError(__func__, " error = ", err);
	}
	logDebug("ISocketRoutingClient::", __func__," called end");

	return err;
}

am_Error_e ISocketRoutingClient::updateSource(const am_sourceID_t sourceID, const am_sourceClass_t sourceClassID,
                                          const vector<am_SoundProperty_s>& listSoundProperties,
                                          const vector<am_CustomConnectionFormat_t>& listConnectionFormats,
                                          const vector<am_MainSoundProperty_s>& listMainSoundProperties)
{
	(void)sourceID;
	(void)sourceClassID;
	(void)listSoundProperties;
	(void)listConnectionFormats;
	(void)listMainSoundProperties;

	logDebug("ISocketRoutingClient::", __func__," called start.");

    assert(mpIAmRoutingReceive != NULL);
    am_Error_e err = E_OK;
	logDebug("ISocketRoutingClient::", __func__," called end");
	return err;

}

am_Error_e ISocketRoutingClient::updateConverter(const am_converterID_t converterID,
                                             const vector<am_CustomConnectionFormat_t>& listSourceFormats,
                                             const vector<am_CustomConnectionFormat_t>& listSinkFormats,
                                             const vector<bool>& convertionMatrix)
{
	(void)converterID;
	(void)listSourceFormats;
	(void)listSinkFormats;
	(void)convertionMatrix;

	logDebug("ISocketRoutingClient::", __func__," called start.");

    assert(mpIAmRoutingReceive != NULL);
    am_Error_e err = E_OK;
	logDebug("ISocketRoutingClient::", __func__," called end");
	return err;
}

void ISocketRoutingClient::ackSetVolumes(const am_Handle_s handle, const vector<am_Volumes_s>& listvolumes,
                                     const am_Error_e error)
{
	(void)handle;
	(void)listvolumes;
	(void)error;

	logDebug("ISocketRoutingClient::", __func__," called start");

    assert(mpIAmRoutingReceive != NULL);
	logDebug("ISocketRoutingClient::", __func__," called end");
}

void ISocketRoutingClient::ackSinkNotificationConfiguration(const am_Handle_s handle, const am_Error_e error)
{
	logDebug("ISocketRoutingClient::", __func__," called start");

    assert(mpIAmRoutingReceive != NULL);

	mpIAmRoutingReceive->ackSinkNotificationConfiguration(handle, error);

	logDebug("ISocketRoutingClient::", __func__," called end");
}

void ISocketRoutingClient::ackSourceNotificationConfiguration(const am_Handle_s handle, const am_Error_e error)
{
	(void)handle;
	(void)error;

	logDebug("ISocketRoutingClient::", __func__," called start");

    assert(mpIAmRoutingReceive != NULL);
	logDebug("ISocketRoutingClient::", __func__," called end");
}

void ISocketRoutingClient::hookSinkNotificationDataChange(const am_sinkID_t sinkID, const am_NotificationPayload_s& payload)
{
	(void)sinkID;
	(void)payload;

	logDebug("ISocketRoutingClient::", __func__," called start");

    assert(mpIAmRoutingReceive != NULL);
	logDebug("ISocketRoutingClient::", __func__," called end");
}

void ISocketRoutingClient::hookSourceNotificationDataChange(const am_sourceID_t sourceID,
                                                        const am_NotificationPayload_s& payload)
{
	(void)sourceID;
	(void)payload;

	logDebug("ISocketRoutingClient::", __func__," called start");
    assert(mpIAmRoutingReceive != NULL);
	logDebug("ISocketRoutingClient::", __func__," called end");
}

/*
 * Stubs implementations against the race condition on creation phase, see the
 * header for more details
 */
void ISocketRoutingClient::setRoutingReady(const uint16_t)
{

}

void ISocketRoutingClient::setRoutingRundown(const uint16_t)
{

}

am_Error_e ISocketRoutingClient::asyncAbort(const am_Handle_s)
{
    return E_OK;
}

am_Error_e ISocketRoutingClient::asyncConnect(const am_Handle_s, const am_connectionID_t,
                                const am_sourceID_t, const am_sinkID_t,
                                const am_CustomConnectionFormat_t)
{
    return E_OK;
}

am_Error_e ISocketRoutingClient::asyncDisconnect(const am_Handle_s, const am_connectionID_t)
{
    return E_OK;
}

am_Error_e ISocketRoutingClient::asyncSetSinkVolume(const am_Handle_s, const am_sinkID_t, const am_volume_t,
                                      const am_CustomRampType_t, const am_time_t)
{
    return E_OK;
}

am_Error_e ISocketRoutingClient::asyncSetSourceVolume(const am_Handle_s, const am_sourceID_t,
                                        const am_volume_t, const am_CustomRampType_t,
                                        const am_time_t)
{
    return E_OK;
}

am_Error_e ISocketRoutingClient::asyncSetSourceState(const am_Handle_s, const am_sourceID_t,
                                       const am_SourceState_e)
{
    return E_OK;
}

am_Error_e ISocketRoutingClient::asyncSetSinkSoundProperties(const am_Handle_s, const am_sinkID_t,
                                               const std::vector<am_SoundProperty_s>&)
{
    return E_OK;
}

am_Error_e ISocketRoutingClient::asyncSetSinkSoundProperty(const am_Handle_s, const am_sinkID_t,
                                             const am_SoundProperty_s&)
{
    return E_OK;
}

am_Error_e ISocketRoutingClient::asyncSetSourceSoundProperties(const am_Handle_s, const am_sourceID_t,
                                                 const std::vector<am_SoundProperty_s>&)
{
    return E_OK;
}

am_Error_e ISocketRoutingClient::asyncSetSourceSoundProperty(const am_Handle_s, const am_sourceID_t,
                                               const am_SoundProperty_s&)
{
    return E_OK;
}

am_Error_e ISocketRoutingClient::asyncSetVolumes(const am_Handle_s, const std::vector<am_Volumes_s>&)
{
    return E_OK;
}

am_Error_e ISocketRoutingClient::asyncSetSinkNotificationConfiguration(
        const am_Handle_s, const am_sinkID_t,
        const am_NotificationConfiguration_s&)
{
    return E_OK;
}

am_Error_e ISocketRoutingClient::asyncSetSourceNotificationConfiguration(
        const am_Handle_s, const am_sourceID_t,
        const am_NotificationConfiguration_s&)
{
    return E_OK;
}

am_Error_e ISocketRoutingClient::asyncCrossFade(const am_Handle_s, const am_crossfaderID_t,
                                           const am_HotSink_e, const am_CustomRampType_t,
                                           const am_time_t)
{
    return E_OK;
}

am_Error_e ISocketRoutingClient::setDomainState(const am_domainID_t, const am_DomainState_e)
{
    return E_OK;
}

