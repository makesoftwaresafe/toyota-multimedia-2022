/************************************************************************
 * @file: IDBusRoutingClient.h
 *
 * @version: 1.1
 *
 * @description: IDBusRoutingClient is a common interface class
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

#ifndef _I_DBUS_ROUTING_CLIENT_H_
#define _I_DBUS_ROUTING_CLIENT_H_

#include <pthread.h>
#include "CAmSerializer.h"
#include "CAmSocketWrapper.h"

#include "IAmRouting.h"
namespace am
{

class CSocketRoutingSender;

class ISocketRoutingClient
{
public:

	ISocketRoutingClient();

	ISocketRoutingClient(std::string dep1, std::string dep2, const std::string& interfaceName); //* Depreciated
	void constructor(); //* Depreciated

    virtual ~ISocketRoutingClient();

    /* Receiver methods. Protected ones are shadowed to ApplicationClient */
protected:
    void ackConnect(const am_Handle_s handle, const am_connectionID_t connectionID, const am_Error_e error);
    void ackDisconnect(const am_Handle_s handle, const am_connectionID_t connectionID, const am_Error_e error);
    void ackSetSourceVolumeChange(const am_Handle_s handle, const am_volume_t volume, const am_Error_e error);
    void ackSetSourceState(const am_Handle_s handle, const am_Error_e error);
    am_Error_e registerDomain(const am_Domain_s& domainData, am_domainID_t& domainID);
    am_Error_e deregisterDomain(const am_domainID_t domainID);
    am_Error_e registerGateway(const am_Gateway_s& gatewayData, am_gatewayID_t& gatewayID);
    am_Error_e deregisterGateway(const am_gatewayID_t gatewayID);
    am_Error_e registerSink(const am_Sink_s& sinkData, am_sinkID_t& sinkID);
    am_Error_e deregisterSink(const am_sinkID_t sinkID);
    am_Error_e registerSource(const am_Source_s& sourceData, am_sourceID_t& sourceID);
    am_Error_e deregisterSource(const am_sourceID_t sourceID);
    void hookDomainRegistrationComplete(const am_domainID_t domainID);
    am_Error_e updateSink(const am_sinkID_t sinkID, const am_sinkClass_t sinkClassID,
                          const std::vector<am_SoundProperty_s>& listSoundProperties,
                          const std::vector<am_CustomConnectionFormat_t>& listConnectionFormats,
                          const std::vector<am_MainSoundProperty_s>& listMainSoundProperties);
    am_Error_e updateSource(const am_sourceID_t sourceID, const am_sourceClass_t sourceClassID,
                            const std::vector<am_SoundProperty_s>& listSoundProperties,
                            const std::vector<am_CustomConnectionFormat_t>& listConnectionFormats,
                            const std::vector<am_MainSoundProperty_s>& listMainSoundProperties);
    void hookSinkAvailablityStatusChange(const am_sinkID_t sinkID, const am_Availability_s& availability);
    void hookSourceAvailablityStatusChange(const am_sourceID_t sourceID, const am_Availability_s& availability);
    void hookSinkNotificationDataChange(const am_sinkID_t sinkID, const am_NotificationPayload_s& payload);
    void hookSourceNotificationDataChange(const am_sourceID_t sourceID, const am_NotificationPayload_s& payload);

    /*
     * Will not be used in the ApplicationClient
     */
    void ackSetSinkVolumeChange(const am_Handle_s handle, const am_volume_t volume, const am_Error_e error);
    void ackSetSinkSoundProperties(const am_Handle_s handle, const am_Error_e error);
    void ackSetSinkSoundProperty(const am_Handle_s handle, const am_Error_e error);
    void ackSetSourceSoundProperties(const am_Handle_s handle, const am_Error_e error);
    void ackSetSourceSoundProperty(const am_Handle_s handle, const am_Error_e error);
    void ackCrossFading(const am_Handle_s handle, const am_HotSink_e hotSink, const am_Error_e error);
    void ackSourceVolumeTick(const am_Handle_s handle, const am_sourceID_t sourceID, const am_volume_t volume);
    void ackSinkVolumeTick(const am_Handle_s handle, const am_sinkID_t sinkID, const am_volume_t volume);
    am_Error_e peekDomain(const std::string& name, am_domainID_t& domainID);
    am_Error_e peekSink(const std::string& name, am_sinkID_t& sinkID);
    am_Error_e peekSource(const std::string& name, am_sourceID_t& sourceID);
    am_Error_e registerCrossfader(const am_Crossfader_s& crossfaderData, am_crossfaderID_t& crossfaderID);
    am_Error_e deregisterCrossfader(const am_crossfaderID_t crossfaderID);
    am_Error_e peekSourceClassID(const std::string& name, am_sourceClass_t& sourceClassID);
    am_Error_e peekSinkClassID(const std::string& name, am_sinkClass_t& sinkClassID);
    void hookInterruptStatusChange(const am_sourceID_t sourceID, const am_InterruptState_e interruptState);
    void hookDomainStateChange(const am_domainID_t domainID, const am_DomainState_e domainState);
    void hookTimingInformationChanged(const am_connectionID_t connectionID, const am_timeSync_t delay);
    void sendChangedData(const std::vector<am_EarlyData_s>& earlyData);
    void confirmRoutingReady(const uint16_t handle, const am_Error_e error);
    void confirmRoutingRundown(const uint16_t handle, const am_Error_e error);
    bool getRoutingReady(void);
    am_Error_e updateGateway(const am_gatewayID_t gatewayID,
                             const std::vector<am_CustomConnectionFormat_t>& listSourceFormats,
                             const std::vector<am_CustomConnectionFormat_t>& listSinkFormats,
                             const std::vector<bool>& convertionMatrix);
    am_Error_e updateConverter(const am_converterID_t converterID,
                               const std::vector<am_CustomConnectionFormat_t>& listSourceFormats,
                               const std::vector<am_CustomConnectionFormat_t>& listSinkFormats,
                               const std::vector<bool>& convertionMatrix);
    void ackSetVolumes(const am_Handle_s handle, const std::vector<am_Volumes_s>& listvolumes, const am_Error_e error);
    void ackSinkNotificationConfiguration(const am_Handle_s handle, const am_Error_e error);
    void ackSourceNotificationConfiguration(const am_Handle_s handle, const am_Error_e error);

    /* Sender callbacks */

    /*
     * To prevent the race condition on creation phase (the problem is that
     * IAmCommandClient is launching the workerThread listening on IPC during
     * ctor, and there, we are not assured that virtual function table is complete;
     * so, it can happen that a message arrives to IPC before the vtable is
     * finalized, leading to a crash) all the following functions will be declared
     * virtual and not pure virtual, providing a stub implementation here. But
     * then, we'll use C++11 pure virtual overrider in the upper layer interfaces
     * (IAm*Client) to force the final user to implement such methods.
     */
public:
    virtual void setRoutingReady(const uint16_t handle);
    virtual void setRoutingRundown(const uint16_t handle);
    virtual am_Error_e asyncAbort(const am_Handle_s handle);
    virtual am_Error_e asyncConnect(const am_Handle_s handle, const am_connectionID_t connectionID,
                                    const am_sourceID_t sourceID, const am_sinkID_t sinkID,
                                    const am_CustomConnectionFormat_t connectionFormat);
    virtual am_Error_e asyncDisconnect(const am_Handle_s handle, const am_connectionID_t connectionID);
    virtual am_Error_e asyncSetSinkVolume(const am_Handle_s handle, const am_sinkID_t sinkID, const am_volume_t volume,
                                          const am_CustomRampType_t ramp, const am_time_t time);
    virtual am_Error_e asyncSetSourceVolume(const am_Handle_s handle, const am_sourceID_t sourceID,
                                            const am_volume_t volume, const am_CustomRampType_t ramp,
                                            const am_time_t time);
    virtual am_Error_e asyncSetSourceState(const am_Handle_s handle, const am_sourceID_t sourceID,
                                           const am_SourceState_e state);
    virtual am_Error_e asyncSetSinkSoundProperties(const am_Handle_s handle, const am_sinkID_t sinkID,
                                                   const std::vector<am_SoundProperty_s>& listSoundProperties);
    virtual am_Error_e asyncSetSinkSoundProperty(const am_Handle_s handle, const am_sinkID_t sinkID,
                                                 const am_SoundProperty_s& soundProperty);
    virtual am_Error_e asyncSetSourceSoundProperties(const am_Handle_s handle, const am_sourceID_t sourceID,
                                                     const std::vector<am_SoundProperty_s>& listSoundProperties);
    virtual am_Error_e asyncSetSourceSoundProperty(const am_Handle_s handle, const am_sourceID_t sourceID,
                                                   const am_SoundProperty_s& soundProperty);
    virtual am_Error_e asyncSetVolumes(const am_Handle_s handle, const std::vector<am_Volumes_s>& listVolumes);
    virtual am_Error_e asyncSetSinkNotificationConfiguration(
            const am_Handle_s handle, const am_sinkID_t sinkID,
            const am_NotificationConfiguration_s& notificationConfiguration);
    virtual am_Error_e asyncSetSourceNotificationConfiguration(
            const am_Handle_s handle, const am_sourceID_t sourceID,
            const am_NotificationConfiguration_s& notificationConfiguration);
    virtual am_Error_e asyncCrossFade(const am_Handle_s handle, const am_crossfaderID_t crossfaderID,
                                               const am_HotSink_e hotSink, const am_CustomRampType_t rampType,
                                               const am_time_t time);
    virtual am_Error_e setDomainState(const am_domainID_t domainID, const am_DomainState_e domainState);

public:
    void setIAmRoutingReceive(IAmRoutingReceive *routingReceive)
    {
        mpIAmRoutingReceive = routingReceive;
    }
    IAmRoutingReceive *getIAmRoutingReceive()
    {
        return mpIAmRoutingReceive;
    }

#ifdef QNXBuild
    void setCAmSerializer(CAmSerializer *serializer)
    {
        mpSerializer = serializer;
    }
    CAmSerializer *getCAmSerializer()
    {
        return mpSerializer;
    }
#else
    void setCAmSerializer(V2::CAmSerializer *serializer)
    {
        mpSerializer = serializer;
    }
    V2::CAmSerializer *getCAmSerializer()
    {
        return mpSerializer;
    }
#endif

private:
    IAmRoutingReceive *mpIAmRoutingReceive;
#ifdef QNXBuild
    CAmSerializer     *mpSerializer;
#else
    V2::CAmSerializer *mpSerializer;
#endif
};

}
#endif /* _I_DBUS_ROUTING_CLIENT_H_ */
