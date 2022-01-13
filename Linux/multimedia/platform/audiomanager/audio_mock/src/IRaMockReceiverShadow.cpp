/************************************************************************
 * @file: IRaDbusWrpReceiverShadow.h
 *
 * @version: 1.1
 *
 * @description: A Receiver class shadow implementation of Routing Adapter.
 * Receiver class will make call to AM via DBus connection.
 * @component: platform/audiomanager
 *
 * @author: Jens Lorenz, jlorenz@de.adit-jv.com 2013,2014
 *          Jayanth MC, Jayanth.mc@in.bosch.com 2013,2014
 *
 * @copyright (c) 2010, 2011 Advanced Driver Information Technology.
 * This code is developed by Advanced Driver Information Technology.
 * Copyright of Advanced Driver Information Technology, Bosch, and DENSO.
 * All rights reserved.
 *
 * @see <related items>
 *
 * @history
 *
 ***********************************************************************/

#include <errno.h>
#include <string.h>
#include <fstream>
#include "CAmDltWrapper.h"
#include "CRaMockSender.h"
#include "IRaMockReceiverShadow.h"

DLT_IMPORT_CONTEXT (AUDIO_MOCK)

using namespace am;


#define str_am_error(err)    tStrError[err]
const std::string tStrError[E_MAX] = {
    "E_OK",
    "E_UNKNOWN",
    "E_OUT_OF_RANGE",
    "E_NOT_USED",
    "E_DATABASE_ERROR",
    "E_ALREADY_EXISTS",
    "E_NO_CHANGE",
    "E_NOT_POSSIBLE",
    "E_NON_EXISTENT",
    "E_ABORTED",
    "E_WRONG_FORMAT"
};


IRaMockReceiverShadow::IRaMockReceiverShadow(CAmSocketHandler *socketHandler) :
        mpSocketHandler(socketHandler)
{
}

IRaMockReceiverShadow::~IRaMockReceiverShadow()
{
}

void IRaMockReceiverShadow::registerAcknowledge(IRaMockSender* receiver)
{
	mpReceiver = receiver;
}

void IRaMockReceiverShadow::ackConnect(const am_Handle_s handle, const am_connectionID_t connectionID,
                                          const am_Error_e error)
{
	if (error == E_OK)
		logInfo("Received::ackConnect");
	else
		logError("Received::ackConnect Error:", str_am_error(error));
	mpReceiver->acknowledge(handle, &connectionID, error);
}

void IRaMockReceiverShadow::ackDisconnect(const am_Handle_s handle, const am_connectionID_t connectionID,
                                             const am_Error_e error)
{
	if (error == E_OK)
		logInfo("Received::ackDisconnect");
	else
		logError("Received::ackDisconnect Error:", str_am_error(error));
	mpReceiver->acknowledge(handle, &connectionID, error);
}

void IRaMockReceiverShadow::ackSetSinkVolumeChange(const am_Handle_s handle, const am_volume_t volume,
                                                      const am_Error_e error)
{
	(void)handle;
	(void)volume;
	(void)error;
}

void IRaMockReceiverShadow::ackSetSourceVolumeChange(const am_Handle_s handle, const am_volume_t volume,
                                                        const am_Error_e error)
{
	(void)volume;
	(void)error;

	if (error == E_OK)
		logInfo("Received::ackSetSourceVolumeChange");
	else
		logError("Received::ackSetSourceVolumeChange Error:", str_am_error(error));
	mpReceiver->acknowledge(handle, &volume, error);
}

void IRaMockReceiverShadow::ackSetSourceState(const am_Handle_s handle, const am_Error_e error)
{
	(void)handle;
	(void)error;
}

void IRaMockReceiverShadow::ackSetSinkSoundProperties(const am_Handle_s handle, const am_Error_e error)
{
	(void)handle;
	(void)error;
}

void IRaMockReceiverShadow::ackSetSinkSoundProperty(const am_Handle_s handle, const am_Error_e error)
{
	(void)handle;
	(void)error;
}

void IRaMockReceiverShadow::ackSetSourceSoundProperties(const am_Handle_s handle, const am_Error_e error)
{
	(void)handle;
	(void)error;
}

void IRaMockReceiverShadow::ackSetSourceSoundProperty(const am_Handle_s handle, const am_Error_e error)
{
	(void)handle;
	(void)error;
}

void IRaMockReceiverShadow::ackCrossFading(const am_Handle_s handle, const am_HotSink_e hotSink,
                                              const am_Error_e error)
{
	(void)handle;
	(void)hotSink;
	(void)error;
}

void IRaMockReceiverShadow::ackSourceVolumeTick(const am_Handle_s handle, const am_sourceID_t sourceID,
                                                   const am_volume_t volume)
{
	(void)sourceID;
	mpReceiver->acknowledge(handle, &volume, E_OK);
}

void IRaMockReceiverShadow::ackSinkVolumeTick(const am_Handle_s handle, const am_sinkID_t sinkID,
                                                 const am_volume_t volume)
{
	(void)handle;
	(void)sinkID;
	(void)volume;
}

am_Error_e IRaMockReceiverShadow::peekDomain(const string &name, am_domainID_t &domainID)
{
	(void)name;
	(void)domainID;
    return E_OK;
}

am_Error_e IRaMockReceiverShadow::registerDomain(const am_Domain_s &domainData, am_domainID_t &domainID)
{
	(void)domainID;
	logInfo("Received::registerDomain", domainData.name);
	return E_OK;
}

am_Error_e IRaMockReceiverShadow::deregisterDomain(const am_domainID_t domainID)
{
	(void)domainID;
    return E_OK;
}

am_Error_e IRaMockReceiverShadow::registerGateway(const am_Gateway_s &gatewayData, am_gatewayID_t &gatewayID)
{
	(void)gatewayData;
	(void)gatewayID;
    return E_OK;
}

am_Error_e IRaMockReceiverShadow::deregisterGateway(const am_gatewayID_t gatewayID)
{
	(void)gatewayID;
	return E_OK;
}

am_Error_e IRaMockReceiverShadow::peekSink(const string &name, am_sinkID_t &sinkID)
{
	(void)name;
	(void)sinkID;
	return E_OK;
}

am_Error_e IRaMockReceiverShadow::registerSink(const am_Sink_s &sinkData, am_sinkID_t &sinkID)
{
	(void)sinkID;
	logInfo("Received::registerSink", sinkData.name, "ID:", sinkID);
	return E_OK;
}

am_Error_e IRaMockReceiverShadow::deregisterSink(const am_sinkID_t sinkID)
{
	(void)sinkID;
	return E_OK;
}

am_Error_e IRaMockReceiverShadow::peekSource(const string &name, am_sourceID_t &sourceID)
{
	(void)name;
	(void)sourceID;
	return E_OK;
}

am_Error_e IRaMockReceiverShadow::registerSource(const am_Source_s &sourceData, am_sourceID_t &sourceID)
{
	(void)sourceID;
	logInfo("Received::registerSource", sourceData.name, "ID:", sourceID);
	return E_OK;
}

am_Error_e IRaMockReceiverShadow::deregisterSource(const am_sourceID_t sourceID)
{
	(void)sourceID;
	return E_OK;
}

am_Error_e IRaMockReceiverShadow::registerCrossfader(const am_Crossfader_s &crossfaderData,
                                                        am_crossfaderID_t &crossfaderID)
{
	(void)crossfaderData;
	(void)crossfaderID;
	return E_OK;
}

am_Error_e IRaMockReceiverShadow::deregisterCrossfader(const am_crossfaderID_t crossfaderID)
{
	(void)crossfaderID;
	return E_OK;
}


am_Error_e IRaMockReceiverShadow::registerConverter(const am_Converter_s& converterData, am_converterID_t& converterID)
{
    (void)converterData;
    (void)converterID;
    return E_OK;
}

am_Error_e IRaMockReceiverShadow::deregisterConverter(const am_converterID_t converterID)
{
    (void)converterID;
    return E_OK;
}

am_Error_e IRaMockReceiverShadow::peekSourceClassID(const string &name, am_sourceClass_t &sourceClassID)
{
	(void)name;
	(void)sourceClassID;
	return E_OK;
}

am_Error_e IRaMockReceiverShadow::peekSinkClassID(const string &name, am_sinkClass_t &sinkClassID)
{
	(void)name;
	(void)sinkClassID;
	return E_OK;
}

void IRaMockReceiverShadow::hookInterruptStatusChange(const am_sourceID_t sourceID,
                                                         const am_InterruptState_e interruptState)
{
	(void)sourceID;
	(void)interruptState;
}

void IRaMockReceiverShadow::hookDomainRegistrationComplete(const am_domainID_t domainID)
{
	(void)domainID;
}

void IRaMockReceiverShadow::hookSinkAvailablityStatusChange(const am_sinkID_t sinkID,
                                                               const am_Availability_s &availability)
{
	(void)sinkID;
	(void)availability;
}

void IRaMockReceiverShadow::hookSourceAvailablityStatusChange(const am_sourceID_t sourceID,
                                                                 const am_Availability_s &availability)
{
	(void)sourceID;
	(void)availability;
}

void IRaMockReceiverShadow::hookDomainStateChange(const am_domainID_t domainID, const am_DomainState_e domainState)
{
	(void)domainID;
	(void)domainState;
}

void IRaMockReceiverShadow::hookTimingInformationChanged(const am_connectionID_t connectionID,
                                                            const am_timeSync_t delay)
{
	(void)connectionID;
	(void)delay;
}

void IRaMockReceiverShadow::sendChangedData(const vector<am_EarlyData_s> &earlyData)
{
	(void)earlyData;
}

am_Error_e IRaMockReceiverShadow::getDBusConnectionWrapper(CAmDbusWrapper*& dbusConnectionWrapper) const
{
	(void)dbusConnectionWrapper;
	return E_NON_EXISTENT;
}

am_Error_e IRaMockReceiverShadow::getSocketHandler(CAmSocketHandler *&socketHandler) const
{
	socketHandler = mpSocketHandler;
	return E_OK;
}

void IRaMockReceiverShadow::getInterfaceVersion(string &version) const
{
	(void)version;
}

void IRaMockReceiverShadow::confirmRoutingReady(const uint16_t handle, const am_Error_e error)
{
	(void)handle;
	(void)error;
	logInfo("Received::confirmRoutingReady");
}

void IRaMockReceiverShadow::confirmRoutingRundown(const uint16_t handle, const am_Error_e error)
{
	(void)handle;
	(void)error;
}

am_Error_e IRaMockReceiverShadow::updateGateway(const am_gatewayID_t gatewayid,
                                                   const vector<am_CustomConnectionFormat_t>& listsourceformats,
                                                   const vector<am_CustomConnectionFormat_t>& listsinkformats,
                                                   const vector<bool>& convertionmatrix)
{
	(void)gatewayid;
	(void)listsourceformats;
	(void)listsinkformats;
	(void)convertionmatrix;
	return E_OK;
}

am_Error_e IRaMockReceiverShadow::updateSink(const am_sinkID_t sinkid, const am_sinkClass_t sinkclassid,
                                                const vector<am_SoundProperty_s>& listsoundproperties,
                                                const vector<am_CustomConnectionFormat_t>& listconnectionformats,
                                                const vector<am_MainSoundProperty_s>& listmainsoundproperties)
{
	(void)sinkid;
	(void)sinkclassid;
	(void)listsoundproperties;
	(void)listconnectionformats;
	(void)listmainsoundproperties;
	return E_OK;
}

am_Error_e IRaMockReceiverShadow::updateSource(const am_sourceID_t sourceid, const am_sourceClass_t sourceclassid,
                                                  const vector<am_SoundProperty_s>& listsoundproperties,
                                                  const vector<am_CustomConnectionFormat_t>& listconnectionformats,
                                                  const vector<am_MainSoundProperty_s>& listmainsoundproperties)
{
	(void)sourceid;
	(void)sourceclassid;
	(void)listsoundproperties;
	(void)listconnectionformats;
	(void)listmainsoundproperties;
	return E_OK;
}

am_Error_e IRaMockReceiverShadow::updateConverter(const am_converterID_t converterID,
                               const std::vector<am_CustomConnectionFormat_t>& listSourceFormats,
                               const std::vector<am_CustomConnectionFormat_t>& listSinkFormats,
                               const std::vector<bool>& convertionMatrix)
{
    (void)converterID;
    (void)listSourceFormats;
    (void)listSinkFormats;
    (void)convertionMatrix;
    return E_OK;
}

void IRaMockReceiverShadow::ackSetVolumes(const am_Handle_s handle, const vector<am_Volumes_s>& listvolumes,
                                             const am_Error_e error)
{
	(void)handle;
	(void)listvolumes;
	(void)error;
}

void IRaMockReceiverShadow::ackSinkNotificationConfiguration(const am_Handle_s handle, const am_Error_e error)
{
	(void)handle;
	(void)error;
}

void IRaMockReceiverShadow::ackSourceNotificationConfiguration(const am_Handle_s handle, const am_Error_e error)
{
	(void)handle;
	(void)error;
}

void IRaMockReceiverShadow::hookSinkNotificationDataChange(const am_sinkID_t sinkid,
                                                              const am_NotificationPayload_s& payload)
{
	(void)sinkid;
	(void)payload;
}

void IRaMockReceiverShadow::hookSourceNotificationDataChange(const am_sourceID_t sourceid,
                                                                const am_NotificationPayload_s& payload)
{
	(void)sourceid;
	(void)payload;
}

am_Error_e IRaMockReceiverShadow::getDomainOfSink(const am_sinkID_t sinkID, am_domainID_t& domainID) const
{
    (void)sinkID;
    (void)domainID;
    return E_NOT_USED;
}

am_Error_e IRaMockReceiverShadow::getDomainOfSource(const am_sourceID_t sourceID, am_domainID_t& domainID) const
{
    (void)sourceID;
    (void)domainID;
    return E_NOT_USED;
}

am_Error_e IRaMockReceiverShadow::getDomainOfCrossfader(const am_crossfaderID_t crossfader, am_domainID_t& domainID) const
{
    (void)crossfader;
    (void)domainID;
    return E_NOT_USED;
};
