/************************************************************************
 * @file: IAmApplicationClient.h
 *
 * @version: 0.9
 *
 * @description: IAmApplicationClient is a common interface class
 * for both receiver and sender command and routing plug-in interface of AM.
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

#ifndef _I_AM_APPLICATION_CLIENT_H_
#define _I_AM_APPLICATION_CLIENT_H_

#include "IDBusCommandClient.h"
#include "IDBusRoutingClient.h"

namespace am
{

class IAmApplicationClient : public IDBusCommandClient, public IDBusRoutingClient
{
public:
    IAmApplicationClient(const std::string& node, const std::string &application, DBusBusType type = DBUS_BUS_SYSTEM, CAmSocketHandler *socketHandler = NULL);
    virtual ~IAmApplicationClient();
public:

    am_Error_e startSocketHandler(void);
    am_Error_e stopSocketHandler(void);
    /*
     * Command side Callbacks
     * C++11 pure virtual overrider
     */
    void cbRemovedMainConnection(const am_mainConnectionID_t connectionID) = 0;

    /*
     * Command side Receiver
     */
    am_Error_e connect(const am_sourceID_t sourceID, const am_sinkID_t sinkID, am_mainConnectionID_t& mainConnectionID);
    am_Error_e disconnect(const am_mainConnectionID_t mainConnectionID);

    /*
     * Routing side Callbacks
     * C++11 pure virtual overrider
     */
    am_Error_e asyncConnect(const am_Handle_s handle, const am_connectionID_t connectionID,
                                    const am_sourceID_t sourceID, const am_sinkID_t sinkID,
                                    const am_CustomConnectionFormat_t connectionFormat) override = 0;
    am_Error_e asyncDisconnect(const am_Handle_s handle, const am_connectionID_t connectionID) override = 0;
    am_Error_e asyncSetSourceVolume(const am_Handle_s handle, const am_sourceID_t sourceID,
                                            const am_volume_t volume, const am_CustomRampType_t ramp,
                                            const am_time_t time) override = 0;
    am_Error_e asyncSetSinkVolume(const am_Handle_s handle, const am_sinkID_t sinkID,
                                            const am_volume_t volume, const am_CustomRampType_t ramp,
                                            const am_time_t time) override = 0;
    am_Error_e asyncSetSourceState(const am_Handle_s handle, const am_sourceID_t sourceID,
                                           const am_SourceState_e state) override = 0;
    /*
     * Routing side Receiver
     */
    void ackConnect(const am_Handle_s handle, const am_connectionID_t connectionID, const am_Error_e error);
    void ackDisconnect(const am_Handle_s handle, const am_connectionID_t connectionID, const am_Error_e error);
    void ackSetSourceVolumeChange(const am_Handle_s handle, const am_volume_t volume, const am_Error_e error);
    void ackSetSinkVolumeChange(const am_Handle_s handle, const am_volume_t volume, const am_Error_e error);
    void ackSetSourceState(const am_Handle_s handle, const am_Error_e error);

    void hookSinkAvailablityStatusChange(const am_sinkID_t sinkID, const am_Availability_s& availability);
    void hookSourceAvailablityStatusChange(const am_sourceID_t sourceID, const am_Availability_s& availability);
    void hookDomainRegistrationComplete(const am_domainID_t domainID);
    void hookSinkNotificationDataChange(const am_sinkID_t sinkID, const am_NotificationPayload_s& payload);
    void hookSourceNotificationDataChange(const am_sourceID_t sourceID, const am_NotificationPayload_s& payload);


    am_Error_e registerDomain(const am_Domain_s& domainData, am_domainID_t& domainID);
    am_Error_e deregisterDomain(const am_domainID_t domainID);
    am_Error_e registerGateway(const am_Gateway_s& gatewayData, am_gatewayID_t& gatewayID);
    am_Error_e deregisterGateway(const am_gatewayID_t gatewayID);
    am_Error_e registerSink(const am_Sink_s& sinkData, am_sinkID_t& sinkID);
    am_Error_e deregisterSink(const am_sinkID_t sinkID);
    am_Error_e registerSource(const am_Source_s& sourceData, am_sourceID_t& sourceID);
    am_Error_e deregisterSource(const am_sourceID_t sourceID);
    am_Error_e updateSink(const am_sinkID_t sinkID, const am_sinkClass_t sinkClassID,
                          const std::vector<am_SoundProperty_s>& listSoundProperties,
                          const std::vector<am_CustomConnectionFormat_t>& listConnectionFormats,
                          const std::vector<am_MainSoundProperty_s>& listMainSoundProperties);
    am_Error_e updateSource(const am_sourceID_t sourceID, const am_sourceClass_t sourceClassID,
                            const std::vector<am_SoundProperty_s>& listSoundProperties,
                            const std::vector<am_CustomConnectionFormat_t>& listConnectionFormats,
                            const std::vector<am_MainSoundProperty_s>& listMainSoundProperties);

protected:
    static void* _WorkerThread(void* This) {
        return static_cast<IAmApplicationClient*>(This)->WorkerThread();
    };
    void* WorkerThread(void);
private:
    pthread_t              mThread;
    CAmSocketHandler       *mpSocketHandler;
    bool                   mIsSocketHandlerInternal;
    CAmDbusWrapper         *mpCAmDbusWrapper;
    CDBusCommandSender     *mpCpDbusSenderCore;
    CDBusRoutingSender     *mpRpDbusSenderCore;
    V2::CAmSerializer      *mpCSerializer;
    V2::CAmSerializer      *mpRSerializer;
    IAmCommandReceive      *mpIAmCommandReceive;
    IAmRoutingReceive      *mpIAmRoutingReceive;
};

}

#endif /* _I_AM_APPLICATION_CLIENT_H_ */
