/************************************************************************
 * @file: IAmRoutingClient.h
 *
 * @version: 1.1
 *
 * @description: IAmRoutingClient is a common interface class
 * for both receiver and sender routing plug-in interface of AM.
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

#ifndef IAMROUTINGCLIENT_H_
#define IAMROUTINGCLIENT_H_

#include "IDBusRoutingClient.h"
#include "CAmSocketHandler.h"
namespace am
{

class IAmRoutingClient : public IDBusRoutingClient
{
public:

    IAmRoutingClient(const std::string& interfaceName, DBusBusType type = DBUS_BUS_SYSTEM, CAmSocketHandler *socketHandler = NULL);

    virtual ~IAmRoutingClient();
    am_Error_e startSocketHandler(void);
    am_Error_e stopSocketHandler(void);

    /*
      * Explicit Receiver methods
      */
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
    void confirmRoutingReady(const uint16_t handle, const am_Error_e error);
    void confirmRoutingRundown(const uint16_t handle, const am_Error_e error);
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

    /*
     * C++11 pure virtual overrider
     */
public:
    void setRoutingReady(const uint16_t handle) override = 0;
    void setRoutingRundown(const uint16_t handle) override = 0;
    am_Error_e asyncAbort(const am_Handle_s handle) override = 0;
    am_Error_e asyncConnect(const am_Handle_s handle, const am_connectionID_t connectionID,
                                    const am_sourceID_t sourceID, const am_sinkID_t sinkID,
                                    const am_CustomConnectionFormat_t connectionFormat) override = 0;
    am_Error_e asyncDisconnect(const am_Handle_s handle, const am_connectionID_t connectionID) override = 0;
    am_Error_e asyncSetSinkVolume(const am_Handle_s handle, const am_sinkID_t sinkID, const am_volume_t volume,
                                          const am_CustomRampType_t ramp, const am_time_t time) override = 0;
    am_Error_e asyncSetSourceVolume(const am_Handle_s handle, const am_sourceID_t sourceID,
                                            const am_volume_t volume, const am_CustomRampType_t ramp,
                                            const am_time_t time) override = 0;
    am_Error_e asyncSetSourceState(const am_Handle_s handle, const am_sourceID_t sourceID,
                                           const am_SourceState_e state) override = 0;
    am_Error_e asyncSetSinkSoundProperties(const am_Handle_s handle, const am_sinkID_t sinkID,
                                                   const std::vector<am_SoundProperty_s>& listSoundProperties) override = 0;
    am_Error_e asyncSetSinkSoundProperty(const am_Handle_s handle, const am_sinkID_t sinkID,
                                                 const am_SoundProperty_s& soundProperty) override = 0;
    am_Error_e asyncSetSourceSoundProperties(const am_Handle_s handle, const am_sourceID_t sourceID,
                                                     const std::vector<am_SoundProperty_s>& listSoundProperties) override = 0;
    am_Error_e asyncSetSourceSoundProperty(const am_Handle_s handle, const am_sourceID_t sourceID,
                                                   const am_SoundProperty_s& soundProperty) override = 0;
    am_Error_e asyncSetVolumes(const am_Handle_s handle, const std::vector<am_Volumes_s>& listVolumes) override = 0;
    am_Error_e asyncSetSinkNotificationConfiguration(
            const am_Handle_s handle, const am_sinkID_t sinkID,
            const am_NotificationConfiguration_s& notificationConfiguration) override = 0;
    am_Error_e asyncSetSourceNotificationConfiguration(
            const am_Handle_s handle, const am_sourceID_t sourceID,
            const am_NotificationConfiguration_s& notificationConfiguration) override = 0;
    am_Error_e asyncCrossFade(const am_Handle_s handle, const am_crossfaderID_t crossfaderID,
                                               const am_HotSink_e hotSink, const am_CustomRampType_t rampType,
                                               const am_time_t time) override = 0;
    am_Error_e setDomainState(const am_domainID_t domainID, const am_DomainState_e domainState) override = 0;

    void timerCallback(sh_timerHandle_t handle, void * userData);
protected:
    static void* _WorkerThread(void* This) {
        return static_cast<IAmRoutingClient*>(This)->WorkerThread();
    };
    void* WorkerThread(void);
private:
    void startTimer(void);
    pthread_t              mThread;
    CAmSocketHandler       *mpSocketHandler;
    bool                   mIsSocketHandlerInternal;
    CAmDbusWrapper         *mpCAmDbusWrapper;
    CDBusRoutingSender     *mpRpDbusSenderCore;
    V2::CAmSerializer      *mpSerializer;
    IAmRoutingReceive      *mpIAmRoutingReceive;
    TAmShTimerCallBack<IAmRoutingClient> mpTimerCallback;
    sh_timerHandle_t        mTimerHandle;
};

}
#endif /* IAMROUTINGCLIENT_H_ */
