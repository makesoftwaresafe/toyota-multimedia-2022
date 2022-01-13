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
#include "IDBusRoutingClient.h"
#include "CDBusRoutingSender.h"
#include "CDBusCommon.h"
#include "IDBusRoutingReceiver.h"

DLT_DECLARE_CONTEXT (RoutingClientCore)

using namespace am;
using namespace std;

IDBusRoutingClient::IDBusRoutingClient(std::string dep1, std::string dep2, const std::string& interfaceName)
{
	(void)dep1;
	(void)dep2;
	(void)interfaceName;
	logError(string(__func__) +
			string(" is depreciated. Please use instead IAmRoutingClient::IAmRoutingClient(const std::string&, DBusBusType, CAmSocketHandler*)"));

	// Important for backward compatibility! Only the interface name is required for construction, e.g.:
	// "audioserver" of "org.adit.audiomanager.routing.audioserver" will be taken!
	this->constructor();
}

IDBusRoutingClient::IDBusRoutingClient()
{
	this->constructor();
}

void IDBusRoutingClient::constructor()
{
    CAmDltWrapper::instance()->registerContext(RoutingClientCore, "CCIF", "Command Client Interface");
}

IDBusRoutingClient::~IDBusRoutingClient()
{

}

void IDBusRoutingClient::ackConnect(const am_Handle_s handle, const am_connectionID_t connectionID,
                                  const am_Error_e error)
{
    assert(mpIAmRoutingReceive != NULL);
    assert(mpSerializer != NULL);
    mpSerializer->asyncCall(mpIAmRoutingReceive,&IAmRoutingReceive::ackConnect,handle,connectionID,error);
}

void IDBusRoutingClient::ackDisconnect(const am_Handle_s handle, const am_connectionID_t connectionID,
                                     const am_Error_e error)
{
    assert(mpIAmRoutingReceive != NULL);
    assert(mpSerializer != NULL);
    mpSerializer->asyncCall(mpIAmRoutingReceive,&IAmRoutingReceive::ackDisconnect,handle,connectionID,error);
}

void IDBusRoutingClient::ackSetSinkVolumeChange(const am_Handle_s handle, const am_volume_t volume,
                                              const am_Error_e error)
{
    assert(mpIAmRoutingReceive != NULL);
    assert(mpSerializer != NULL);
    mpSerializer->asyncCall(mpIAmRoutingReceive,&IAmRoutingReceive::ackSetSinkVolumeChange,handle,volume,error);
}

void IDBusRoutingClient::ackSetSourceVolumeChange(const am_Handle_s handle, const am_volume_t volume,
                                                const am_Error_e error)
{
    assert(mpIAmRoutingReceive != NULL);
    assert(mpSerializer != NULL);
    mpSerializer->asyncCall(mpIAmRoutingReceive,&IAmRoutingReceive::ackSetSourceVolumeChange,handle,volume,error);
}

void IDBusRoutingClient::ackSetSourceState(const am_Handle_s handle, const am_Error_e error)
{
    assert(mpIAmRoutingReceive != NULL);
    assert(mpSerializer != NULL);
    mpSerializer->asyncCall(mpIAmRoutingReceive,&IAmRoutingReceive::ackSetSourceState,handle,error);
}

void IDBusRoutingClient::ackSetSinkSoundProperties(const am_Handle_s handle, const am_Error_e error)
{
    assert(mpIAmRoutingReceive != NULL);
    assert(mpSerializer != NULL);
    mpSerializer->asyncCall(mpIAmRoutingReceive,&IAmRoutingReceive::ackSetSinkSoundProperties,handle,error);
}

void IDBusRoutingClient::ackSetSinkSoundProperty(const am_Handle_s handle, const am_Error_e error)
{
    assert(mpIAmRoutingReceive != NULL);
    assert(mpSerializer != NULL);
    mpSerializer->asyncCall(mpIAmRoutingReceive,&IAmRoutingReceive::ackSetSinkSoundProperty,handle,error);
}

void IDBusRoutingClient::ackSetSourceSoundProperties(const am_Handle_s handle, const am_Error_e error)
{
    assert(mpIAmRoutingReceive != NULL);
    assert(mpSerializer != NULL);
    mpSerializer->asyncCall(mpIAmRoutingReceive,&IAmRoutingReceive::ackSetSourceSoundProperties,handle,error);
}

void IDBusRoutingClient::ackSetSourceSoundProperty(const am_Handle_s handle, const am_Error_e error)
{
    assert(mpIAmRoutingReceive != NULL);
    assert(mpSerializer != NULL);
    mpSerializer->asyncCall(mpIAmRoutingReceive,&IAmRoutingReceive::ackSetSourceSoundProperty,handle,error);
}

void IDBusRoutingClient::ackCrossFading(const am_Handle_s handle, const am_HotSink_e hotSink, const am_Error_e error)
{
    assert(mpIAmRoutingReceive != NULL);
    assert(mpSerializer != NULL);
    mpSerializer->asyncCall(mpIAmRoutingReceive,&IAmRoutingReceive::ackCrossFading,handle, hotSink,error);
}

void IDBusRoutingClient::ackSourceVolumeTick(const am_Handle_s handle, const am_sourceID_t sourceID,
                                           const am_volume_t volume)
{
    assert(mpIAmRoutingReceive != NULL);
    assert(mpSerializer != NULL);
    mpSerializer->asyncCall(mpIAmRoutingReceive,&IAmRoutingReceive::ackSourceVolumeTick,handle, sourceID,volume);
}

void IDBusRoutingClient::ackSinkVolumeTick(const am_Handle_s handle, const am_sinkID_t sinkID, const am_volume_t volume)
{
    assert(mpIAmRoutingReceive != NULL);
    assert(mpSerializer != NULL);
    mpSerializer->asyncCall(mpIAmRoutingReceive,&IAmRoutingReceive::ackSinkVolumeTick,handle, sinkID,volume);
}

am_Error_e IDBusRoutingClient::peekDomain(const string& name, am_domainID_t& domainID)
{
    assert(mpIAmRoutingReceive != NULL);
    assert(mpSerializer != NULL);
    am_Error_e err;
    std::string _name = name;
    mpSerializer->syncCall(mpIAmRoutingReceive,&IAmRoutingReceive::peekDomain,err,_name,domainID);
    return err;
}

am_Error_e IDBusRoutingClient::registerDomain(const am_Domain_s& domainData, am_domainID_t& domainID)
{
    assert(mpIAmRoutingReceive != NULL);
    assert(mpSerializer != NULL);
    am_Domain_s _domainData = domainData;
    am_Error_e err;
    mpSerializer->syncCall(mpIAmRoutingReceive,&IAmRoutingReceive::registerDomain,err,_domainData,domainID);
    return err;
}

am_Error_e IDBusRoutingClient::deregisterDomain(const am_domainID_t domainID)
{
    assert(mpIAmRoutingReceive != NULL);
    assert(mpSerializer != NULL);
    am_Error_e err;
    am_domainID_t _domainID = domainID;
    mpSerializer->syncCall(mpIAmRoutingReceive,&IAmRoutingReceive::deregisterDomain,err,_domainID);
    return err;
}

am_Error_e IDBusRoutingClient::registerGateway(const am_Gateway_s& gatewayData, am_gatewayID_t& gatewayID)
{
    assert(mpIAmRoutingReceive != NULL);
    assert(mpSerializer != NULL);
    am_Error_e err;
    am_Gateway_s _gatewayData = gatewayData;
    mpSerializer->syncCall(mpIAmRoutingReceive,&IAmRoutingReceive::registerGateway,err,_gatewayData,gatewayID);
    return err;
}

am_Error_e IDBusRoutingClient::deregisterGateway(const am_gatewayID_t gatewayID)
{
    assert(mpIAmRoutingReceive != NULL);
    assert(mpSerializer != NULL);
    am_Error_e err;
    am_gatewayID_t _gatewayID = gatewayID;
    mpSerializer->syncCall(mpIAmRoutingReceive,&IAmRoutingReceive::deregisterGateway,err,_gatewayID);
    return err;
}

am_Error_e IDBusRoutingClient::peekSink(const string& name, am_sinkID_t& sinkID)
{
    assert(mpIAmRoutingReceive != NULL);
    assert(mpSerializer != NULL);
    am_Error_e err;
    string _name = name;
    mpSerializer->syncCall(mpIAmRoutingReceive,&IAmRoutingReceive::peekSink,err,_name,sinkID);
    return err;
}

am_Error_e IDBusRoutingClient::registerSink(const am_Sink_s& sinkData, am_sinkID_t& sinkID)
{
    assert(mpIAmRoutingReceive != NULL);
    assert(mpSerializer != NULL);
    am_Error_e err;
    am_Sink_s _sinkData = sinkData;
    mpSerializer->syncCall(mpIAmRoutingReceive,&IAmRoutingReceive::registerSink,err,_sinkData,sinkID);
    return err;
}

am_Error_e IDBusRoutingClient::deregisterSink(const am_sinkID_t sinkID)
{
    assert(mpIAmRoutingReceive != NULL);
    assert(mpSerializer != NULL);
    am_Error_e err;
    am_sinkID_t _sinkID = sinkID;
    mpSerializer->syncCall(mpIAmRoutingReceive,&IAmRoutingReceive::deregisterSink,err,_sinkID);
    return err;
}

am_Error_e IDBusRoutingClient::peekSource(const string& name, am_sourceID_t& sourceID)
{
    assert(mpIAmRoutingReceive != NULL);
    assert(mpSerializer != NULL);
    am_Error_e err;
    string _name = name;
    mpSerializer->syncCall(mpIAmRoutingReceive,&IAmRoutingReceive::peekSource,err,_name,sourceID);
    return err;
}

am_Error_e IDBusRoutingClient::registerSource(const am_Source_s& sourceData, am_sourceID_t& sourceID)
{
    assert(mpIAmRoutingReceive != NULL);
    assert(mpSerializer != NULL);
    am_Error_e err;
    am_Source_s _sourceData = sourceData;
    mpSerializer->syncCall(mpIAmRoutingReceive,&IAmRoutingReceive::registerSource,err,_sourceData,sourceID);
    return err;
}

am_Error_e IDBusRoutingClient::deregisterSource(const am_sourceID_t sourceID)
{
    assert(mpIAmRoutingReceive != NULL);
    assert(mpSerializer != NULL);
    am_Error_e err;
    am_sourceID_t _sourceID = sourceID;
    mpSerializer->syncCall(mpIAmRoutingReceive,&IAmRoutingReceive::deregisterSource,err,_sourceID);
    return err;
}

am_Error_e IDBusRoutingClient::registerCrossfader(const am_Crossfader_s& crossfaderData, am_crossfaderID_t& crossfaderID)
{
    assert(mpIAmRoutingReceive != NULL);
    assert(mpSerializer != NULL);
    am_Error_e err;
    am_Crossfader_s _crossfaderData = crossfaderData;
    mpSerializer->syncCall(mpIAmRoutingReceive,&IAmRoutingReceive::registerCrossfader,err,_crossfaderData,crossfaderID);
    return err;
}

am_Error_e IDBusRoutingClient::deregisterCrossfader(const am_crossfaderID_t crossfaderID)
{
    assert(mpIAmRoutingReceive != NULL);
    assert(mpSerializer != NULL);
    am_Error_e err;
    am_crossfaderID_t _crossfaderID = crossfaderID;
    mpSerializer->syncCall(mpIAmRoutingReceive,&IAmRoutingReceive::deregisterCrossfader,err,_crossfaderID);
    return err;

}

am_Error_e IDBusRoutingClient::peekSourceClassID(const string& name, am_sourceClass_t& sourceClassID)
{
    assert(mpIAmRoutingReceive != NULL);
    assert(mpSerializer != NULL);
    am_Error_e err;
    string _name = name;
    mpSerializer->syncCall(mpIAmRoutingReceive,&IAmRoutingReceive::peekSourceClassID,err,_name,sourceClassID);
    return err;

}

am_Error_e IDBusRoutingClient::peekSinkClassID(const string& name, am_sinkClass_t& sinkClassID)
{
    assert(mpIAmRoutingReceive != NULL);
    assert(mpSerializer != NULL);
    am_Error_e err;
    string _name = name;
    mpSerializer->syncCall(mpIAmRoutingReceive,&IAmRoutingReceive::peekSinkClassID,err,_name,sinkClassID);
    return err;

}

void IDBusRoutingClient::hookInterruptStatusChange(const am_sourceID_t sourceID, const am_InterruptState_e interruptState)
{
    assert(mpIAmRoutingReceive != NULL);
    assert(mpSerializer != NULL);
    mpSerializer->asyncCall(mpIAmRoutingReceive,&IAmRoutingReceive::hookInterruptStatusChange,sourceID, interruptState);
}

void IDBusRoutingClient::hookDomainRegistrationComplete(const am_domainID_t domainID)
{
    assert(mpIAmRoutingReceive != NULL);
    assert(mpSerializer != NULL);
    mpSerializer->asyncCall(mpIAmRoutingReceive,&IAmRoutingReceive::hookDomainRegistrationComplete,domainID);
}

void IDBusRoutingClient::hookSinkAvailablityStatusChange(const am_sinkID_t sinkID, const am_Availability_s& availability)
{
    assert(mpIAmRoutingReceive != NULL);
    assert(mpSerializer != NULL);
    mpSerializer->asyncCall(mpIAmRoutingReceive,&IAmRoutingReceive::hookSinkAvailablityStatusChange,sinkID,availability);
}

void IDBusRoutingClient::hookSourceAvailablityStatusChange(const am_sourceID_t sourceID,
                                                         const am_Availability_s& availability)
{
    assert(mpIAmRoutingReceive != NULL);
    assert(mpSerializer != NULL);
    mpSerializer->asyncCall(mpIAmRoutingReceive,&IAmRoutingReceive::hookSourceAvailablityStatusChange,sourceID,availability);
}

void IDBusRoutingClient::hookDomainStateChange(const am_domainID_t domainID, const am_DomainState_e domainState)
{
    assert(mpIAmRoutingReceive != NULL);
    assert(mpSerializer != NULL);
    mpSerializer->asyncCall(mpIAmRoutingReceive,&IAmRoutingReceive::hookDomainStateChange,domainID,domainState);
}

void IDBusRoutingClient::hookTimingInformationChanged(const am_connectionID_t connectionID, const am_timeSync_t delay)
{
    assert(mpIAmRoutingReceive != NULL);
    assert(mpSerializer != NULL);
    mpSerializer->asyncCall(mpIAmRoutingReceive,&IAmRoutingReceive::hookTimingInformationChanged,connectionID,delay);
}

void IDBusRoutingClient::sendChangedData(const vector<am_EarlyData_s>& earlyData)
{ 
    assert(mpIAmRoutingReceive != NULL);
    assert(mpSerializer != NULL);
    mpSerializer->asyncCall(mpIAmRoutingReceive,&IAmRoutingReceive::sendChangedData,earlyData);
}

void IDBusRoutingClient::confirmRoutingReady(const uint16_t handle, const am_Error_e error)
{
    assert(mpIAmRoutingReceive != NULL);
    assert(mpSerializer != NULL);
    mpSerializer->asyncCall(mpIAmRoutingReceive,&IAmRoutingReceive::confirmRoutingReady,handle, error);
}

bool IDBusRoutingClient::getRoutingReady(void)
{
    assert(mpIAmRoutingReceive != NULL);
    assert(mpSerializer != NULL);
    return ((IDBusRoutingReceiver*)mpIAmRoutingReceive)->getRoutingReady();
}

void IDBusRoutingClient::confirmRoutingRundown(const uint16_t handle, const am_Error_e error)
{
    assert(mpIAmRoutingReceive != NULL);
    assert(mpSerializer != NULL);
    mpSerializer->asyncCall(mpIAmRoutingReceive,&IAmRoutingReceive::confirmRoutingRundown,handle, error);
}

am_Error_e IDBusRoutingClient::updateGateway(const am_gatewayID_t gatewayID,
                                           const vector<am_CustomConnectionFormat_t>& listSourceFormats,
                                           const vector<am_CustomConnectionFormat_t>& listSinkFormats,
                                           const vector<bool>& convertionMatrix)
{
    assert(mpIAmRoutingReceive != NULL);
    assert(mpSerializer != NULL);
    am_Error_e err;
    am_gatewayID_t _gatewayID = gatewayID;
    vector<am_CustomConnectionFormat_t> _listSourceFormats = listSourceFormats;
    vector<am_CustomConnectionFormat_t> _listSinkFormats = listSinkFormats;
    vector<bool> _convertionMatrix = convertionMatrix;
    mpSerializer->syncCall(mpIAmRoutingReceive,&IAmRoutingReceive::updateGateway,err,_gatewayID,_listSourceFormats,_listSinkFormats,_convertionMatrix);
    return err;
}

am_Error_e IDBusRoutingClient::updateSink(const am_sinkID_t sinkID, const am_sinkClass_t sinkClassID,
                                        const vector<am_SoundProperty_s>& listSoundProperties,
                                        const vector<am_CustomConnectionFormat_t>& listConnectionFormats,
                                        const vector<am_MainSoundProperty_s>& listMainSoundProperties)
{
    assert(mpIAmRoutingReceive != NULL);
    assert(mpSerializer != NULL);
    am_Error_e err;
    am_sinkID_t _sinkID = sinkID;
    am_sinkClass_t _sinkClassID = sinkClassID;
    vector<am_SoundProperty_s> _listSoundProperties = listSoundProperties;
    vector<am_CustomConnectionFormat_t> _listConnectionFormats = listConnectionFormats;
    vector<am_MainSoundProperty_s> _listMainSoundProperties = listMainSoundProperties;

    this->mpSerializer->syncCall(mpIAmRoutingReceive,&am::IAmRoutingReceive::updateSink,err,_sinkID,_sinkClassID,_listSoundProperties,_listConnectionFormats,_listMainSoundProperties);
    return err;
}

am_Error_e IDBusRoutingClient::updateSource(const am_sourceID_t sourceID, const am_sourceClass_t sourceClassID,
                                          const vector<am_SoundProperty_s>& listSoundProperties,
                                          const vector<am_CustomConnectionFormat_t>& listConnectionFormats,
                                          const vector<am_MainSoundProperty_s>& listMainSoundProperties)
{
    assert(mpIAmRoutingReceive != NULL);
    assert(mpSerializer != NULL);
    am_Error_e err;
    am_sourceID_t _sourceID = sourceID;
    am_sourceClass_t _sourceClassID = sourceClassID;
    vector<am_SoundProperty_s> _listSoundProperties = listSoundProperties;
    vector<am_CustomConnectionFormat_t> _listConnectionFormats = listConnectionFormats;
    vector<am_MainSoundProperty_s> _listMainSoundProperties = listMainSoundProperties;
    this->mpSerializer->syncCall(mpIAmRoutingReceive,&am::IAmRoutingReceive::updateSource,err,_sourceID,_sourceClassID,_listSoundProperties,_listConnectionFormats,_listMainSoundProperties);
    return err;

}

am_Error_e IDBusRoutingClient::updateConverter(const am_converterID_t converterID,
                                             const vector<am_CustomConnectionFormat_t>& listSourceFormats,
                                             const vector<am_CustomConnectionFormat_t>& listSinkFormats,
                                             const vector<bool>& convertionMatrix)
{
    assert(mpIAmRoutingReceive != NULL);
    assert(mpSerializer != NULL);
    am_Error_e err;
    am_converterID_t _converterID = converterID;
    vector<am_CustomConnectionFormat_t> _listSourceFormats = listSourceFormats;
    vector<am_CustomConnectionFormat_t> _listSinkFormats = listSinkFormats;
    vector<bool> _convertionMatrix = convertionMatrix;
    this->mpSerializer->syncCall(mpIAmRoutingReceive,&am::IAmRoutingReceive::updateConverter,err,_converterID,_listSourceFormats,_listSinkFormats,_convertionMatrix);
	return err;
}

void IDBusRoutingClient::ackSetVolumes(const am_Handle_s handle, const vector<am_Volumes_s>& listvolumes,
                                     const am_Error_e error)
{
    assert(mpIAmRoutingReceive != NULL);
    assert(mpSerializer != NULL);
    mpSerializer->asyncCall(mpIAmRoutingReceive,&IAmRoutingReceive::ackSetVolumes,handle,listvolumes, error);
}

void IDBusRoutingClient::ackSinkNotificationConfiguration(const am_Handle_s handle, const am_Error_e error)
{
    assert(mpIAmRoutingReceive != NULL);
    assert(mpSerializer != NULL);
    mpSerializer->asyncCall(mpIAmRoutingReceive,&IAmRoutingReceive::ackSinkNotificationConfiguration,handle, error);
}

void IDBusRoutingClient::ackSourceNotificationConfiguration(const am_Handle_s handle, const am_Error_e error)
{
    assert(mpIAmRoutingReceive != NULL);
    assert(mpSerializer != NULL);
    mpSerializer->asyncCall(mpIAmRoutingReceive,&IAmRoutingReceive::ackSourceNotificationConfiguration,handle, error);
}

void IDBusRoutingClient::hookSinkNotificationDataChange(const am_sinkID_t sinkID, const am_NotificationPayload_s& payload)
{
    assert(mpIAmRoutingReceive != NULL);
    assert(mpSerializer != NULL);
    mpSerializer->asyncCall(mpIAmRoutingReceive,&IAmRoutingReceive::hookSinkNotificationDataChange,sinkID, payload);
}

void IDBusRoutingClient::hookSourceNotificationDataChange(const am_sourceID_t sourceID,
                                                        const am_NotificationPayload_s& payload)
{
    assert(mpIAmRoutingReceive != NULL);
    assert(mpSerializer != NULL);
    mpSerializer->asyncCall(mpIAmRoutingReceive,&IAmRoutingReceive::hookSourceNotificationDataChange,sourceID, payload);
}
