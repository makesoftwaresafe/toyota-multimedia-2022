/************************************************************************
 * @file: IAmRoutingClient.cpp
 *
 * @version: 1.1
 *
 * @description: IAmRoutingClient calls implementation. IAmRoutingClient is a
 * common interface class for both receiver and sender interface of AM.
 * implements AM receiver wrapper.Routs the call to either dlink or Dbus
 * receiver shadow implementation.
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
#include <algorithm>
#include <assert.h>
#include "CAmDltWrapper.h"
#include "IAmRoutingClient.h"
#include "CRpDbusWrpSender.h"
#include "CDBusCommon.h"
#include "IDBusRoutingReceiver.h"
#include "CAmSocketHandlerSingleton.h"

DLT_DECLARE_CONTEXT (RoutingClient)

using namespace am;
using namespace std;

#define throw_assert(EXPRESSION, MESSAGE) \
        if (!(EXPRESSION)) throw std::runtime_error(MESSAGE)

IAmRoutingClient::IAmRoutingClient(const std::string& interfaceName, DBusBusType type, CAmSocketHandler *socketHandler)
    : IDBusRoutingClient(), mThread(0), mpSocketHandler(socketHandler), mIsSocketHandlerInternal(false),
      mpCAmDbusWrapper(NULL), mpRpDbusSenderCore(NULL), mpSerializer(NULL), mpIAmRoutingReceive(NULL),
      mpTimerCallback(this,&IAmRoutingClient::timerCallback),
      mTimerHandle(0)
{
    CAmDltWrapper::instance()->registerContext(RoutingClient, "RTCL", "Routing Client Interface");
    try
    {
        if (NULL == mpSocketHandler)
        {
            mpSocketHandler = CAmSocketHandlerSingleton::getSocketHandler();
            mIsSocketHandlerInternal = true;
        }

        mpSerializer = new V2::CAmSerializer(mpSocketHandler);
        this->setCAmSerializer(mpSerializer);

        string prefix(string(ROUTING_PLUGIN_BUSNAME) + string(".") + interfaceName);
        string path(string(ROUTING_DBUS_OBJECT_PATH) + string("/") + interfaceName);
        mpCAmDbusWrapper = new CAmDbusWrapper(mpSocketHandler, type, prefix.c_str(), path.c_str());

        DBusConnection *connection;
        mpCAmDbusWrapper->getDBusConnection(connection);
        if (connection != NULL)
        {
            mpRpDbusSenderCore = new CRpDbusWrpSender(this, mpCAmDbusWrapper);
            mpIAmRoutingReceive = new IDBusRoutingReceiver(connection, interfaceName);
            this->setIAmRoutingReceive(mpIAmRoutingReceive);
            mpSerializer->asyncCall(this,&IAmRoutingClient::startTimer);
        }
        else
        {
            log(&RoutingClient, DLT_LOG_ERROR, "IAmRoutingClient::IAmRoutingClient DBus connection not created");
        }
    }
    catch (const std::ostringstream & error)
    {
        log(&RoutingClient, DLT_LOG_ERROR, "IAmRoutingClient::IAmRoutingClient Failed to create ", error.str());
        this->~IAmRoutingClient();
    }
}

IAmRoutingClient::~IAmRoutingClient()
{
    CAmDltWrapper::instance()->unregisterContext(RoutingClient);

    if (mpIAmRoutingReceive != NULL)
    {
        delete mpIAmRoutingReceive;
    }
    if (mpRpDbusSenderCore != NULL)
    {
        delete mpRpDbusSenderCore;
    }
    if (mpCAmDbusWrapper != NULL)
    {
        delete mpCAmDbusWrapper;
    }
    if (mpSerializer != NULL)
    {
        delete mpSerializer;
    }
}

void IAmRoutingClient::startTimer(void)
{
    timespec timeoutTime;
    timeoutTime.tv_sec = 0;
    timeoutTime.tv_nsec = 50000000; /* 50 milli second */
    if (mpSocketHandler->addTimer(timeoutTime, &mpTimerCallback, mTimerHandle, NULL) != E_OK)
    {
        log(&RoutingClient, DLT_LOG_ERROR, "IAmRoutingClient::startTimer Timer add failed");
    }
    else
    {
        log(&RoutingClient, DLT_LOG_INFO, "IAmRoutingClient::startTimer Timer added Successfully");
    }
}

am_Error_e IAmRoutingClient::startSocketHandler(void)
{
    /*
     * if Thread is already created then dont create it again, in other words
     * if startSocketHandler is called twice
     */
    if ((!mIsSocketHandlerInternal) || (mThread != 0))
    {
        return E_OK;
    }
    am_Error_e err = E_OK;
    int ret = pthread_create(&mThread, NULL, IAmRoutingClient::_WorkerThread, this);
    if (ret)
    {
        err = E_NOT_POSSIBLE;
        log(&RoutingClient, DLT_LOG_ERROR, "IAmRoutingClient::startSocketHandler pthread_create failed return=", ret);
    }
    return err;
}

am_Error_e IAmRoutingClient::stopSocketHandler(void)
{
    /*
     * mThread can be zero if stopSocketHandler is called before startSocketHandler
     */
    if ((!mIsSocketHandlerInternal) || (mpSocketHandler == NULL) || (mThread == 0))
    {
        return E_OK;
    }
    am_Error_e err = E_OK;
    mpSocketHandler->exit_mainloop();
    int ret = pthread_join(mThread, NULL);
    if (ret)
    {
        err = E_NOT_POSSIBLE;
        log(&RoutingClient, DLT_LOG_ERROR, "IAmRoutingClient::stopSocketHandler pthread_join failed return=", ret);
    }
    else
    {
        mThread = 0;
    }
    return err;
}

void* IAmRoutingClient::WorkerThread(void)
{
    log(&RoutingClient, DLT_LOG_INFO, "IAmRoutingClient::WorkerThread running...");
    assert(mpSocketHandler != NULL);
    CAmSocketHandlerSingleton::startSocketHandler();
    log(&RoutingClient, DLT_LOG_INFO, "IAmRoutingClient::WorkerThread stopped!");
    return NULL;
}

void IAmRoutingClient::timerCallback(sh_timerHandle_t handle, void * userData)
{
    (void)handle;
    (void)userData;
    timespec timeoutTime;
    timeoutTime.tv_sec = 0;
    timeoutTime.tv_nsec = 500000000; /* 500 milli second */
    if( true == IDBusRoutingClient::getRoutingReady())
    {
        am_Handle_s handle;
        handle.handleType = H_UNKNOWN;
        handle.handle = 0;
        this->setRoutingReady(handle.handle);
        mpSocketHandler->removeTimer(mTimerHandle);
    }
    else
    {
        mpSocketHandler->updateTimer(mTimerHandle, timeoutTime);
        mpSocketHandler->restartTimer(mTimerHandle);
    }

    log(&RoutingClient, DLT_LOG_INFO, "IAmRoutingClient::timerCallback exit");
}

void IAmRoutingClient::ackConnect(const am_Handle_s handle, const am_connectionID_t connectionID, const am_Error_e error)
{
    IDBusRoutingClient::ackConnect(handle, connectionID, error);
}

void IAmRoutingClient::ackDisconnect(const am_Handle_s handle, const am_connectionID_t connectionID, const am_Error_e error)
{
    IDBusRoutingClient::ackDisconnect(handle, connectionID, error);
}

void IAmRoutingClient::ackSetSinkVolumeChange(const am_Handle_s handle, const am_volume_t volume, const am_Error_e error)
{
    IDBusRoutingClient::ackSetSinkVolumeChange(handle, volume, error);
}

void IAmRoutingClient::ackSetSourceVolumeChange(const am_Handle_s handle, const am_volume_t volume, const am_Error_e error)
{
    IDBusRoutingClient::ackSetSourceVolumeChange(handle, volume, error);
}

void IAmRoutingClient::ackSetSourceState(const am_Handle_s handle, const am_Error_e error)
{
    IDBusRoutingClient::ackSetSourceState(handle, error);
}

void IAmRoutingClient::ackSetSinkSoundProperties(const am_Handle_s handle, const am_Error_e error)
{
    IDBusRoutingClient::ackSetSinkSoundProperties(handle, error);
}

void IAmRoutingClient::ackSetSinkSoundProperty(const am_Handle_s handle, const am_Error_e error)
{
    IDBusRoutingClient::ackSetSinkSoundProperty(handle, error);
}

void IAmRoutingClient::ackSetSourceSoundProperties(const am_Handle_s handle, const am_Error_e error)
{
    IDBusRoutingClient::ackSetSourceSoundProperties(handle, error);
}

void IAmRoutingClient::ackSetSourceSoundProperty(const am_Handle_s handle, const am_Error_e error)
{
    IDBusRoutingClient::ackSetSourceSoundProperty(handle, error);
}

void IAmRoutingClient::ackCrossFading(const am_Handle_s handle, const am_HotSink_e hotSink, const am_Error_e error)
{
    IDBusRoutingClient::ackCrossFading(handle, hotSink, error);
}

void IAmRoutingClient::ackSourceVolumeTick(const am_Handle_s handle, const am_sourceID_t sourceID, const am_volume_t volume)
{
    IDBusRoutingClient::ackSourceVolumeTick(handle, sourceID, volume);
}

void IAmRoutingClient::ackSinkVolumeTick(const am_Handle_s handle, const am_sinkID_t sinkID, const am_volume_t volume)
{
    IDBusRoutingClient::ackSinkVolumeTick(handle, sinkID, volume);
}

am_Error_e IAmRoutingClient::peekDomain(const std::string &name, am_domainID_t &domainID)
{
    return IDBusRoutingClient::peekDomain(name, domainID);
}

am_Error_e IAmRoutingClient::registerDomain(const am_Domain_s &domainData, am_domainID_t &domainID)
{
    return IDBusRoutingClient::registerDomain(domainData, domainID);
}

am_Error_e IAmRoutingClient::deregisterDomain(const am_domainID_t domainID)
{
    return IDBusRoutingClient::deregisterDomain(domainID);
}

am_Error_e IAmRoutingClient::registerGateway(const am_Gateway_s &gatewayData, am_gatewayID_t &gatewayID)
{
    return IDBusRoutingClient::registerGateway(gatewayData, gatewayID);
}

am_Error_e IAmRoutingClient::deregisterGateway(const am_gatewayID_t gatewayID)
{
    return IDBusRoutingClient::deregisterGateway(gatewayID);
}

am_Error_e IAmRoutingClient::peekSink(const std::string &name, am_sinkID_t &sinkID)
{
    return IDBusRoutingClient::peekSink(name, sinkID);
}

am_Error_e IAmRoutingClient::registerSink(const am_Sink_s &sinkData, am_sinkID_t &sinkID)
{
    return IDBusRoutingClient::registerSink(sinkData, sinkID);
}

am_Error_e IAmRoutingClient::deregisterSink(const am_sinkID_t sinkID)
{
    return IDBusRoutingClient::deregisterSink(sinkID);
}

am_Error_e IAmRoutingClient::peekSource(const std::string &name, am_sourceID_t &sourceID)
{
    return IDBusRoutingClient::peekSource(name, sourceID);
}

am_Error_e IAmRoutingClient::registerSource(const am_Source_s &sourceData, am_sourceID_t &sourceID)
{
    return IDBusRoutingClient::registerSource(sourceData, sourceID);
}

am_Error_e IAmRoutingClient::deregisterSource(const am_sourceID_t sourceID)
{
    return IDBusRoutingClient::deregisterSource(sourceID);
}

am_Error_e IAmRoutingClient::registerCrossfader(const am_Crossfader_s &crossfaderData, am_crossfaderID_t &crossfaderID)
{
    return IDBusRoutingClient::registerCrossfader(crossfaderData, crossfaderID);
}

am_Error_e IAmRoutingClient::deregisterCrossfader(const am_crossfaderID_t crossfaderID)
{
    return IDBusRoutingClient::deregisterCrossfader(crossfaderID);
}

am_Error_e IAmRoutingClient::peekSourceClassID(const std::string &name, am_sourceClass_t &sourceClassID)
{
    return IDBusRoutingClient::peekSourceClassID(name, sourceClassID);
}

am_Error_e IAmRoutingClient::peekSinkClassID(const std::string &name, am_sinkClass_t &sinkClassID)
{
    return IDBusRoutingClient::peekSinkClassID(name, sinkClassID);
}

void IAmRoutingClient::hookInterruptStatusChange(const am_sourceID_t sourceID, const am_InterruptState_e interruptState)
{
    IDBusRoutingClient::hookInterruptStatusChange(sourceID, interruptState);
}

void IAmRoutingClient::hookDomainRegistrationComplete(const am_domainID_t domainID)
{
    IDBusRoutingClient::hookDomainRegistrationComplete(domainID);
}

void IAmRoutingClient::hookSinkAvailablityStatusChange(const am_sinkID_t sinkID, const am_Availability_s &availability)
{
    IDBusRoutingClient::hookSinkAvailablityStatusChange(sinkID, availability);
}

void IAmRoutingClient::hookSourceAvailablityStatusChange(const am_sourceID_t sourceID, const am_Availability_s &availability)
{
    IDBusRoutingClient::hookSourceAvailablityStatusChange(sourceID, availability);
}

void IAmRoutingClient::hookDomainStateChange(const am_domainID_t domainID, const am_DomainState_e domainState)
{
    IDBusRoutingClient::hookDomainStateChange(domainID, domainState);
}

void IAmRoutingClient::hookTimingInformationChanged(const am_connectionID_t connectionID, const am_timeSync_t delay)
{
    IDBusRoutingClient::hookTimingInformationChanged(connectionID, delay);
}

void IAmRoutingClient::sendChangedData(const std::vector<am_EarlyData_s> &earlyData)
{
    IDBusRoutingClient::sendChangedData(earlyData);
}

void IAmRoutingClient::confirmRoutingReady(const uint16_t handle, const am_Error_e error)
{
    IDBusRoutingClient::confirmRoutingReady(handle, error);
}

void IAmRoutingClient::confirmRoutingRundown(const uint16_t handle, const am_Error_e error)
{
    IDBusRoutingClient::confirmRoutingRundown(handle, error);
}

am_Error_e IAmRoutingClient::updateGateway(const am_gatewayID_t gatewayid,
                         const std::vector<am_CustomConnectionFormat_t>& listsourceformats,
                         const std::vector<am_CustomConnectionFormat_t>& listsinkformats,
                         const std::vector<bool>& convertionmatrix)
{
    return IDBusRoutingClient::updateGateway(gatewayid, listsourceformats, listsinkformats, convertionmatrix);
}

am_Error_e IAmRoutingClient::updateSink(const am_sinkID_t sinkid, const am_sinkClass_t sinkclassid,
                      const std::vector<am_SoundProperty_s>& listsoundproperties,
                      const std::vector<am_CustomConnectionFormat_t>& listconnectionformats,
                      const std::vector<am_MainSoundProperty_s>& listmainsoundproperties)
{
    return IDBusRoutingClient::updateSink(sinkid, sinkclassid, listsoundproperties, listconnectionformats, listmainsoundproperties);
}

am_Error_e IAmRoutingClient::updateSource(const am_sourceID_t sourceid, const am_sourceClass_t sourceclassid,
                        const std::vector<am_SoundProperty_s>& listsoundproperties,
                        const std::vector<am_CustomConnectionFormat_t>& listconnectionformats,
                        const std::vector<am_MainSoundProperty_s>& listmainsoundproperties)
{
    return IDBusRoutingClient::updateSource(sourceid, sourceclassid, listsoundproperties, listconnectionformats, listmainsoundproperties);
}

am_Error_e IAmRoutingClient::updateConverter(const am_converterID_t converterID,
                           const std::vector<am_CustomConnectionFormat_t>& listSourceFormats,
                           const std::vector<am_CustomConnectionFormat_t>& listSinkFormats,
                           const std::vector<bool>& convertionMatrix)
{
    return IDBusRoutingClient::updateConverter(converterID, listSourceFormats, listSinkFormats, convertionMatrix);
}

void IAmRoutingClient::ackSetVolumes(const am_Handle_s handle, const std::vector<am_Volumes_s>& listvolumes, const am_Error_e error)
{
    IDBusRoutingClient::ackSetVolumes(handle, listvolumes, error);
}

void IAmRoutingClient::ackSinkNotificationConfiguration(const am_Handle_s handle, const am_Error_e error)
{
    IDBusRoutingClient::ackSinkNotificationConfiguration(handle, error);
}

void IAmRoutingClient::ackSourceNotificationConfiguration(const am_Handle_s handle, const am_Error_e error)
{
    IDBusRoutingClient::ackSourceNotificationConfiguration(handle, error);
}

void IAmRoutingClient::hookSinkNotificationDataChange(const am_sinkID_t sinkid, const am_NotificationPayload_s& payload)
{
    IDBusRoutingClient::hookSinkNotificationDataChange(sinkid, payload);
}

void IAmRoutingClient::hookSourceNotificationDataChange(const am_sourceID_t sourceid, const am_NotificationPayload_s& payload)
{
    IDBusRoutingClient::hookSourceNotificationDataChange(sourceid, payload);
}
