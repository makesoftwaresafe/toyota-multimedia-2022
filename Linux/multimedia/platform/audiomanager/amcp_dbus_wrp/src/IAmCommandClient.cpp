/************************************************************************
 * @file: IAmCommandClient.cpp
 *
 * @version: 1.1
 *
 * @description: IAmCommandClient is a common interface class for
 * both receiver and sender command plug-in interface of AM.
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

#include <assert.h>
#include "CAmDltWrapper.h"
#include "IAmCommandClient.h"
#include "CCpDbusWrpSender.h"
#include "CDBusCommon.h"
#include "IDBusCommandReceiver.h"
#include "CAmSocketHandlerSingleton.h"

DLT_DECLARE_CONTEXT (CommandClient)


#define throw_assert(EXPRESSION, MESSAGE) \
        if (!(EXPRESSION)) throw std::runtime_error(MESSAGE)

using namespace am;
using namespace std;

IAmCommandClient::IAmCommandClient(DBusBusType dbusWrapperType, CAmSocketHandler *socketHandler)
    : IDBusCommandClient(), mThread(0), mpSocketHandler(socketHandler), mIsSocketHandlerInternal(false),
      mpCAmDbusWrapper(NULL), mpCpDbusSenderCore(NULL), mpSerializer(NULL), mpIAmCommandReceive(NULL)
{
    CAmDltWrapper::instance()->registerContext(CommandClient, "CCIF", "Command Client Interface");
    try
    {
        if (NULL == mpSocketHandler)
        {
            mpSocketHandler = CAmSocketHandlerSingleton::getSocketHandler();
            mIsSocketHandlerInternal = true;
        }

        mpSerializer = new V2::CAmSerializer(mpSocketHandler);
        this->setCAmSerializer(mpSerializer);

        DBusConnection *connection;
        mpCAmDbusWrapper = new CAmDbusWrapper(mpSocketHandler, dbusWrapperType, "", "");
        mpCAmDbusWrapper->getDBusConnection(connection);
        if (connection != NULL)
        {
            mpCpDbusSenderCore = new CCpDbusWrpSender(this, mpCAmDbusWrapper);
            mpIAmCommandReceive = new IDBusCommandReceiver(connection);
            this->setIAmCommandReceive(mpIAmCommandReceive);
        }
        else
        {
            log(&CommandClient, DLT_LOG_ERROR, "CCpDbusWrpSender::CCpDbusWrpSender DBus connection not created");
        }
    }
    catch (const std::ostringstream & error)
    {
        log(&CommandClient, DLT_LOG_ERROR, "CCpDbusWrpSender::CCpDbusWrpSender Failed to create ", error.str());
        this->~IAmCommandClient();
    }
}

IAmCommandClient::~IAmCommandClient()
{
    CAmDltWrapper::instance()->unregisterContext(CommandClient);

    if (mpIAmCommandReceive != NULL)
    {
        delete mpIAmCommandReceive;
    }
    if (mpCpDbusSenderCore != NULL)
    {
        delete mpCpDbusSenderCore;
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

am_Error_e IAmCommandClient::startSocketHandler(void)
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
    int ret = pthread_create(&mThread, NULL, IAmCommandClient::_WorkerThread, this);
    if (ret)
    {
        err = E_NOT_POSSIBLE;
        log(&CommandClient, DLT_LOG_ERROR, "IAmCommandClient::startSocketHandler pthread_create failed return=", ret);
    }
    return err;
}

am_Error_e IAmCommandClient::stopSocketHandler(void)
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
    if (ret != 0)
    {
        err = E_NOT_POSSIBLE;
        log(&CommandClient, DLT_LOG_ERROR, "IAmCommandClient::stopSocketHandler failed return=",ret);
    }
    else
    {
        mThread = 0;
    }

    return err;
}


void* IAmCommandClient::WorkerThread(void)
{
    log(&CommandClient, DLT_LOG_INFO, "IAmCommandClient::WorkerThread running...");
    assert(mpSocketHandler != NULL);
    CAmSocketHandlerSingleton::startSocketHandler();
    log(&CommandClient, DLT_LOG_INFO, "IAmCommandClient::WorkerThread stopped!");
    return NULL;
}

am_Error_e IAmCommandClient::connect(const am_sourceID_t sourceID, const am_sinkID_t sinkID, am_mainConnectionID_t& mainConnectionID)
{
    return IDBusCommandClient::connect(sourceID, sinkID, mainConnectionID);
}

am_Error_e IAmCommandClient::disconnect(const am_mainConnectionID_t mainConnectionID)
{
    return IDBusCommandClient::disconnect(mainConnectionID);
}

am_Error_e IAmCommandClient::setVolume(const am_sinkID_t sinkID, const am_mainVolume_t volume)
{
    return IDBusCommandClient::setVolume(sinkID, volume);
}

am_Error_e IAmCommandClient::volumeStep(const am_sinkID_t sinkID, const int16_t volumeStep)
{
    return IDBusCommandClient::volumeStep(sinkID, volumeStep);
}

am_Error_e IAmCommandClient::setSinkMuteState(const am_sinkID_t sinkID, const am_MuteState_e muteState)
{
    return IDBusCommandClient::setSinkMuteState(sinkID, muteState);
}

am_Error_e IAmCommandClient::setMainSinkSoundProperty(const am_MainSoundProperty_s& soundProperty, const am_sinkID_t sinkID)
{
    return IDBusCommandClient::setMainSinkSoundProperty(soundProperty, sinkID);
}

am_Error_e IAmCommandClient::setMainSourceSoundProperty(const am_MainSoundProperty_s& soundProperty, const am_sourceID_t sourceID)
{
    return IDBusCommandClient::setMainSourceSoundProperty(soundProperty, sourceID);
}

am_Error_e IAmCommandClient::setSystemProperty(const am_SystemProperty_s& property)
{
    return IDBusCommandClient::setSystemProperty(property);
}

am_Error_e IAmCommandClient::getListMainConnections(std::vector<am_MainConnectionType_s>& listConnections)
{
    return IDBusCommandClient::getListMainConnections(listConnections);
}

am_Error_e IAmCommandClient::getListMainSinks(std::vector<am_SinkType_s>& listMainSinks)
{
    return IDBusCommandClient::getListMainSinks(listMainSinks);
}

am_Error_e IAmCommandClient::getListMainSources(std::vector<am_SourceType_s>& listMainSources)
{
    return IDBusCommandClient::getListMainSources(listMainSources);
}

am_Error_e IAmCommandClient::getListMainSinkSoundProperties(const am_sinkID_t sinkID,
                                          std::vector<am_MainSoundProperty_s>& listSoundProperties)
{
    return IDBusCommandClient::getListMainSinkSoundProperties(sinkID, listSoundProperties);
}

am_Error_e IAmCommandClient::getListMainSourceSoundProperties(const am_sourceID_t sourceID,
                                            std::vector<am_MainSoundProperty_s>& listSourceProperties)
{
    return IDBusCommandClient::getListMainSourceSoundProperties(sourceID, listSourceProperties);
}

am_Error_e IAmCommandClient::getListSourceClasses(std::vector<am_SourceClass_s>& listSourceClasses)
{
    return IDBusCommandClient::getListSourceClasses(listSourceClasses);
}

am_Error_e IAmCommandClient::getListSinkClasses(std::vector<am_SinkClass_s>& listSinkClasses)
{
    return IDBusCommandClient::getListSinkClasses(listSinkClasses);
}

am_Error_e IAmCommandClient::getListSystemProperties(std::vector<am_SystemProperty_s>& listSystemProperties)
{
    return IDBusCommandClient::getListSystemProperties(listSystemProperties);
}

am_Error_e IAmCommandClient::getTimingInformation(const am_mainConnectionID_t mainConnectionID, am_timeSync_t& delay)
{
    return IDBusCommandClient::getTimingInformation(mainConnectionID, delay);
}

void IAmCommandClient::confirmCommandReady(const uint16_t handle, const am_Error_e error)
{
    IDBusCommandClient::confirmCommandReady(handle, error);
}

void IAmCommandClient::confirmCommandRundown(const uint16_t handle, const am_Error_e error)
{
    IDBusCommandClient::confirmCommandRundown(handle, error);
}

am_Error_e IAmCommandClient::getListMainSinkNotificationConfigurations(
        const am_sinkID_t sinkID, std::vector<am_NotificationConfiguration_s>& listMainNotificationConfigurations)
{
    return IDBusCommandClient::getListMainSinkNotificationConfigurations(sinkID, listMainNotificationConfigurations);
}

am_Error_e IAmCommandClient::getListMainSourceNotificationConfigurations(
        const am_sourceID_t sourceID,
        std::vector<am_NotificationConfiguration_s>& listMainNotificationConfigurations)
{
    return IDBusCommandClient::getListMainSourceNotificationConfigurations(sourceID, listMainNotificationConfigurations);
}

am_Error_e IAmCommandClient::setMainSinkNotificationConfiguration(const am_sinkID_t sinkID,
                                                const am_NotificationConfiguration_s& mainNotificationConfiguration)
{
    return IDBusCommandClient::setMainSinkNotificationConfiguration(sinkID, mainNotificationConfiguration);
}

am_Error_e IAmCommandClient::setMainSourceNotificationConfiguration(const am_sourceID_t sourceID,
                                                  const am_NotificationConfiguration_s& mainNotificationConfiguration)
{
    return IDBusCommandClient::setMainSourceNotificationConfiguration(sourceID, mainNotificationConfiguration);
}

am_Error_e IAmCommandClient::getVolume(const am_sinkID_t sinkID, am_mainVolume_t& mainVolume)
{
    return IDBusCommandClient::getVolume(sinkID, mainVolume);
}
