/************************************************************************
 * @file: IDBusRoutingReceiver.h
 *
 * @version: 1.1
 *
 * @description: A Receiver class shadow definition of Routing Adapter.
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

#ifndef _I_DBUS_ROUTING_RECEIVER_H_
#define _I_DBUS_ROUTING_RECEIVER_H_

#include "CAmSocketWrapper.h"
#include "CAmDltWrapper.h"
#include "IAmRouting.h"
#include "CSocketSender.h"

namespace am
{
/**
 * receives the DBus Callbacks, marhsalls and demarshalls the parameters and calls CommandReceive
 */
class ISocketRoutingReceiver: public IAmRoutingReceive
{
public:
    ISocketRoutingReceiver( CAmSocketWrapper* wrapper );
    virtual ~ISocketRoutingReceiver();
    void ackConnect(const am_Handle_s handle, const am_connectionID_t connectionID, const am_Error_e error);
    void ackDisconnect(const am_Handle_s handle, const am_connectionID_t connectionID, const am_Error_e error);
    void ackSetSinkVolumeChange(const am_Handle_s handle, const am_volume_t volume, const am_Error_e error);
    void ackSetSourceVolumeChange(const am_Handle_s handle, const am_volume_t volume, const am_Error_e error);
    void ackSetSourceState(const am_Handle_s handle, const am_Error_e error);
    void ackSetSinkSoundProperties(const am_Handle_s handle, const am_Error_e error);
    void ackSetSinkSoundProperty(const am_Handle_s handle, const am_Error_e error);
    void ackSetSourceSoundProperties(const am_Handle_s handle, const am_Error_e error);
    void ackSetSourceSoundProperty(const am_Handle_s handle, const am_Error_e error);
    void ackCrossFading(const am_Handle_s handle, const am_HotSink_e hotSink, const am_Error_e error);
    void ackSourceVolumeTick(const am_Handle_s handle, const am_sourceID_t sourceID, const am_volume_t volume);
    void ackSinkVolumeTick(const am_Handle_s handle, const am_sinkID_t sinkID, const am_volume_t volume);
    am_Error_e peekDomain(const std::string &name, am_domainID_t &domainID);
    am_Error_e registerDomain(const am_Domain_s &domainData, am_domainID_t &domainID);
    am_Error_e deregisterDomain(const am_domainID_t domainID);
    am_Error_e registerGateway(const am_Gateway_s &gatewayData, am_gatewayID_t &gatewayID);
    am_Error_e deregisterGateway(const am_gatewayID_t gatewayID);
    am_Error_e peekSink(const std::string &name, am_sinkID_t &sinkID);
    am_Error_e registerSink(const am_Sink_s &sinkData, am_sinkID_t &sinkID);
    am_Error_e deregisterSink(const am_sinkID_t sinkID);
    am_Error_e peekSource(const std::string &name, am_sourceID_t &sourceID);
    am_Error_e registerSource(const am_Source_s &sourceData, am_sourceID_t &sourceID);
    am_Error_e deregisterSource(const am_sourceID_t sourceID);
    am_Error_e registerCrossfader(const am_Crossfader_s &crossfaderData, am_crossfaderID_t &crossfaderID);
    am_Error_e deregisterCrossfader(const am_crossfaderID_t crossfaderID);
    am_Error_e registerConverter(const am_Converter_s& converterData, am_converterID_t& converterID);
    am_Error_e deregisterConverter(am_converterID_t converterID);
    am_Error_e peekSourceClassID(const std::string &name, am_sourceClass_t &sourceClassID);
    am_Error_e peekSinkClassID(const std::string &name, am_sinkClass_t &sinkClassID);
    void hookInterruptStatusChange(const am_sourceID_t sourceID, const am_InterruptState_e interruptState);
    void hookDomainRegistrationComplete(const am_domainID_t domainID);
    void hookSinkAvailablityStatusChange(const am_sinkID_t sinkID, const am_Availability_s &availability);
    void hookSourceAvailablityStatusChange(const am_sourceID_t sourceID, const am_Availability_s &availability);
    void hookDomainStateChange(const am_domainID_t domainID, const am_DomainState_e domainState);
    void hookTimingInformationChanged(const am_connectionID_t connectionID, const am_timeSync_t delay);
    void sendChangedData(const std::vector<am_EarlyData_s> &earlyData);
    am_Error_e getDBusConnectionWrapper(CAmDbusWrapper *&dbusConnectionWrapper) const;
    am_Error_e getSocketHandler(CAmSocketHandler *&socketHandler) const;
    void getInterfaceVersion(std::string &version) const;
    void confirmRoutingReady(const uint16_t handle, const am_Error_e error);
    void confirmRoutingRundown(const uint16_t handle, const am_Error_e error);
    bool getRoutingReady(void);
    am_Error_e updateGateway(const am_gatewayID_t gatewayid,
                             const std::vector<am_CustomConnectionFormat_t>& listsourceformats,
                             const std::vector<am_CustomConnectionFormat_t>& listsinkformats,
                             const std::vector<bool>& convertionmatrix);
    am_Error_e updateSink(const am_sinkID_t sinkid, const am_sinkClass_t sinkclassid,
                          const std::vector<am_SoundProperty_s>& listsoundproperties,
                          const std::vector<am_CustomConnectionFormat_t>& listconnectionformats,
                          const std::vector<am_MainSoundProperty_s>& listmainsoundproperties);
    am_Error_e updateSource(const am_sourceID_t sourceid, const am_sourceClass_t sourceclassid,
                            const std::vector<am_SoundProperty_s>& listsoundproperties,
                            const std::vector<am_CustomConnectionFormat_t>& listconnectionformats,
                            const std::vector<am_MainSoundProperty_s>& listmainsoundproperties);
    am_Error_e updateConverter(const am_converterID_t converterID,
                               const std::vector<am_CustomConnectionFormat_t>& listSourceFormats,
                               const std::vector<am_CustomConnectionFormat_t>& listSinkFormats,
                               const std::vector<bool>& convertionMatrix);
    void ackSetVolumes(const am_Handle_s handle, const std::vector<am_Volumes_s>& listvolumes, const am_Error_e error);
    void ackSinkNotificationConfiguration(const am_Handle_s handle, const am_Error_e error);
    void ackSourceNotificationConfiguration(const am_Handle_s handle, const am_Error_e error);
    void hookSinkNotificationDataChange(const am_sinkID_t sinkid, const am_NotificationPayload_s& payload);
    void hookSourceNotificationDataChange(const am_sourceID_t sourceid, const am_NotificationPayload_s& payload);
    am_Error_e getDomainOfSink(const am_sinkID_t sinkID, am_domainID_t& domainID) const;
    am_Error_e getDomainOfSource(const am_sourceID_t sourceID, am_domainID_t& domainID) const;
    am_Error_e getDomainOfCrossfader(const am_crossfaderID_t crossfader, am_domainID_t& domainID) const;

private:
    std::string mNodename;
    std::string mApplicationName;
	CSocketSender mCSocketSender;
};

} /* namespace am */

#endif /* _I_DBUS_ROUTING_RECEIVER_H_ */
