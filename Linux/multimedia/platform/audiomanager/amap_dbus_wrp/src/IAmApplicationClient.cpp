/************************************************************************
 * @file: IAmApplicationClient.cpp
 *
 * @version: 0.9
 *
 * @description: IAmApplicationClient is a common interface class for
 * both receiver and sender command and routing plug-in interface of AM.
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

#include "IAmApplicationClient.h"

#include <algorithm>
#include <assert.h>
#include "CAmDltWrapper.h"
#include "CDBusCommandSender.h"
#include "CDBusRoutingSender.h"
#include "CDBusCommon.h"
#include "IDBusRoutingReceiver.h"
#include "IDBusCommandReceiver.h"

#define throw_assert(EXPRESSION, MESSAGE) \
        if (!(EXPRESSION)) throw std::runtime_error(MESSAGE)

using namespace am;
using namespace std;

DLT_DECLARE_CONTEXT (ApplicationClient)

IAmApplicationClient::IAmApplicationClient(const std::string& node, const std::string &application, DBusBusType type, CAmSocketHandler *socketHandler)
    : IDBusCommandClient(), IDBusRoutingClient(), mThread(0), mpSocketHandler(socketHandler), mIsSocketHandlerInternal(false),
      mpCAmDbusWrapper(NULL), mpCpDbusSenderCore(NULL), mpRpDbusSenderCore(NULL), mpCSerializer(NULL), mpRSerializer(NULL),
      mpIAmCommandReceive(NULL), mpIAmRoutingReceive(NULL)
{
    CAmDltWrapper::instance()->registerContext(ApplicationClient, "ACTT", "ApplicationClient DLT Context");
    try
    {
        if (NULL == mpSocketHandler)
        {
            mpSocketHandler = new CAmSocketHandler();
            mIsSocketHandlerInternal = true;
        }

        mpCSerializer = new V2::CAmSerializer(mpSocketHandler);
        this->IDBusCommandClient::setCAmSerializer(mpCSerializer);
        mpRSerializer = new V2::CAmSerializer(mpSocketHandler);
        this->IDBusRoutingClient::setCAmSerializer(mpRSerializer);

        string prefix(string(ROUTING_PLUGIN_BUSNAME) + string(".") + node + string(".") + application);
        string path(string(ROUTING_DBUS_OBJECT_PATH) + string("/") + node + string("/") + application);
        mpCAmDbusWrapper = new CAmDbusWrapper(mpSocketHandler, type, prefix.c_str(), path.c_str());

        DBusConnection *connection;
        mpCAmDbusWrapper->getDBusConnection(connection);
        if (connection != NULL)
        {
            mpRpDbusSenderCore = new CDBusRoutingSender(this, mpCAmDbusWrapper);
            mpCpDbusSenderCore = new CDBusCommandSender(this, mpCAmDbusWrapper);
            mpIAmRoutingReceive = new IDBusRoutingReceiver(connection, node, application);
            this->setIAmRoutingReceive(mpIAmRoutingReceive);
            mpIAmCommandReceive = new IDBusCommandReceiver(connection);
            this->setIAmCommandReceive(mpIAmCommandReceive);
        }
        else
        {
            log(&ApplicationClient, DLT_LOG_ERROR, "IAmRoutingClient::IAmRoutingClient DBus connection not created");
        }
    }
    catch (const std::ostringstream & error)
    {
        log(&ApplicationClient, DLT_LOG_ERROR, "IAmRoutingClient::IAmRoutingClient Failed to create ", error.str());
        this->~IAmApplicationClient();
    }
}
IAmApplicationClient::~IAmApplicationClient()
{
    CAmDltWrapper::instance()->unregisterContext(ApplicationClient);
    if (mpIAmCommandReceive != NULL)
    {
        delete mpIAmCommandReceive;
    }
    if (mpIAmRoutingReceive != NULL)
    {
        delete mpIAmRoutingReceive;
    }
    if (mpCpDbusSenderCore != NULL)
    {
        delete mpCpDbusSenderCore;
    }
    if (mpRpDbusSenderCore != NULL)
    {
        delete mpRpDbusSenderCore;
    }
    if (mpCAmDbusWrapper != NULL)
    {
        delete mpCAmDbusWrapper;
    }
    if (mpRSerializer != NULL)
    {
        delete mpRSerializer;
    }
    if (mpCSerializer != NULL)
    {
        delete mpCSerializer;
    }
    if (mIsSocketHandlerInternal && (mpSocketHandler != NULL) )
    {
        delete mpSocketHandler;
    }
}

void* IAmApplicationClient::WorkerThread(void)
{
    log(&ApplicationClient, DLT_LOG_INFO, "IAmApplicationClient::WorkerThread running...");
    assert(mpSocketHandler != NULL);
    mpSocketHandler->start_listenting();
    log(&ApplicationClient, DLT_LOG_INFO, "IAmApplicationClient::WorkerThread stopped!");
    return NULL;
}

am_Error_e IAmApplicationClient::stopSocketHandler(void)
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
        log(&ApplicationClient, DLT_LOG_ERROR, "IAmApplicationClient::stopSocketHandler pthread_join failed return=", ret);
    }
    else
    {
        mThread = 0;
    }

    return err;
}

am_Error_e IAmApplicationClient::startSocketHandler(void)
{
    /*
     * if Thread is already created then dont create it again, in other words
     * if startSocketHandler is called twice
     */
    if ((!mpSocketHandler) || (mThread != 0))
    {
        return E_OK;
    }
    am_Error_e err = E_OK;
    int ret = pthread_create(&mThread, NULL, IAmApplicationClient::_WorkerThread, this);
    if (ret)
    {
        err = E_NOT_POSSIBLE;
        log(&ApplicationClient, DLT_LOG_ERROR, "IAmApplicationClient::startSocketHandler pthread_create failed return=",ret);
    }
    return err;
}

am_Error_e IAmApplicationClient::connect(const am_sourceID_t sourceID, const am_sinkID_t sinkID, am_mainConnectionID_t& mainConnectionID)
{
    return IDBusCommandClient::connect(sourceID, sinkID, mainConnectionID);
}

am_Error_e IAmApplicationClient::disconnect(const am_mainConnectionID_t mainConnectionID)
{
    return IDBusCommandClient::disconnect(mainConnectionID);
}

void IAmApplicationClient::ackConnect(const am_Handle_s handle, const am_connectionID_t connectionID, const am_Error_e error)
{
    IDBusRoutingClient::ackConnect(handle, connectionID, error);
}

void IAmApplicationClient::ackDisconnect(const am_Handle_s handle, const am_connectionID_t connectionID, const am_Error_e error)
{
    IDBusRoutingClient::ackDisconnect(handle, connectionID, error);
}

void IAmApplicationClient::ackSetSourceVolumeChange(const am_Handle_s handle, const am_volume_t volume, const am_Error_e error)
{
    IDBusRoutingClient::ackSetSourceVolumeChange(handle, volume, error);
}

void IAmApplicationClient::ackSetSinkVolumeChange(const am_Handle_s handle, const am_volume_t volume, const am_Error_e error)
{
    IDBusRoutingClient::ackSetSinkVolumeChange(handle, volume, error);
}

void IAmApplicationClient::ackSetSourceState(const am_Handle_s handle, const am_Error_e error)
{
    IDBusRoutingClient::ackSetSourceState(handle, error);
}

am_Error_e IAmApplicationClient::registerDomain(const am_Domain_s &domainData, am_domainID_t &domainID)
{
    return IDBusRoutingClient::registerDomain(domainData, domainID);
}

am_Error_e IAmApplicationClient::deregisterDomain(const am_domainID_t domainID)
{
    return IDBusRoutingClient::deregisterDomain(domainID);
}

am_Error_e IAmApplicationClient::registerGateway(const am_Gateway_s &gatewayData, am_gatewayID_t &gatewayID)
{
    return IDBusRoutingClient::registerGateway(gatewayData, gatewayID);
}

am_Error_e IAmApplicationClient::deregisterGateway(const am_gatewayID_t gatewayID)
{
    return IDBusRoutingClient::deregisterGateway(gatewayID);
}

am_Error_e IAmApplicationClient::registerSink(const am_Sink_s &sinkData, am_sinkID_t &sinkID)
{
    return IDBusRoutingClient::registerSink(sinkData, sinkID);
}

am_Error_e IAmApplicationClient::deregisterSink(const am_sinkID_t sinkID)
{
    return IDBusRoutingClient::deregisterSink(sinkID);
}

am_Error_e IAmApplicationClient::registerSource(const am_Source_s &sourceData, am_sourceID_t &sourceID)
{
    return IDBusRoutingClient::registerSource(sourceData, sourceID);
}

am_Error_e IAmApplicationClient::deregisterSource(const am_sourceID_t sourceID)
{
    return IDBusRoutingClient::deregisterSource(sourceID);
}

am_Error_e IAmApplicationClient::updateSink(const am_sinkID_t sinkid, const am_sinkClass_t sinkclassid,
                      const std::vector<am_SoundProperty_s>& listsoundproperties,
                      const std::vector<am_CustomConnectionFormat_t>& listconnectionformats,
                      const std::vector<am_MainSoundProperty_s>& listmainsoundproperties)
{
    return IDBusRoutingClient::updateSink(sinkid, sinkclassid, listsoundproperties, listconnectionformats, listmainsoundproperties);
}

am_Error_e IAmApplicationClient::updateSource(const am_sourceID_t sourceid, const am_sourceClass_t sourceclassid,
                        const std::vector<am_SoundProperty_s>& listsoundproperties,
                        const std::vector<am_CustomConnectionFormat_t>& listconnectionformats,
                        const std::vector<am_MainSoundProperty_s>& listmainsoundproperties)
{
    return IDBusRoutingClient::updateSource(sourceid, sourceclassid, listsoundproperties, listconnectionformats, listmainsoundproperties);
}

void IAmApplicationClient::hookDomainRegistrationComplete(const am_domainID_t domainID)
{
    IDBusRoutingClient::hookDomainRegistrationComplete(domainID);
}

void IAmApplicationClient::hookSinkAvailablityStatusChange(const am_sinkID_t sinkID, const am_Availability_s &availability)
{
    IDBusRoutingClient::hookSinkAvailablityStatusChange(sinkID, availability);
}

void IAmApplicationClient::hookSourceAvailablityStatusChange(const am_sourceID_t sourceID, const am_Availability_s &availability)
{
    IDBusRoutingClient::hookSourceAvailablityStatusChange(sourceID, availability);
}

void IAmApplicationClient::hookSinkNotificationDataChange(const am_sinkID_t sinkID, const am_NotificationPayload_s& payload)
{
    IDBusRoutingClient::hookSinkNotificationDataChange(sinkID, payload);
}

void IAmApplicationClient::hookSourceNotificationDataChange(const am_sourceID_t sourceID, const am_NotificationPayload_s& payload)
{
    IDBusRoutingClient::hookSourceNotificationDataChange(sourceID, payload);
}
