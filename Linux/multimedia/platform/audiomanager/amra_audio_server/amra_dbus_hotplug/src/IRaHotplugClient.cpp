/************************************************************************
* @file: CAppHotplugReceiver.h
*
* @version: 1.1
*
* CAppHoptplugReceiver class implements the interface IRaHotplugReceive
* for communication from Application (Hotplug) to Routing Adapter using over D-Bus
* 
* @component: platform/audiomanager
*
* @author: Nrusingh Dash <ndash@jp.adit-jv.com>
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
#include "CAppHotplugReceiver.h"
#include "IRaHotplugClient.h"

static void *EventLoopThreadHandler(void *args)
{
	am::CAmSocketHandler *socketHandler = static_cast<am::CAmSocketHandler *> (args);
	socketHandler->start_listenting();
	pthread_exit(NULL);
	return NULL;
}

IRaHotplugClient::IRaHotplugClient(am::CAmSocketHandler* socketHandler) :
	mpHotplugReceiver(NULL),
	mpSocketHandler(socketHandler),
	mpEventLoopThread(NULL),
	mpSerializer(NULL)
{
    int ret = 0;
    /*
    * if socketHandler is NULL
    * then it is responsibility of IRaHotplugClient to instantiate it.
    * else just use it
    */
    if (NULL == mpSocketHandler)
    {
        mpSocketHandler = new am::CAmSocketHandler();
        if (NULL == mpSocketHandler)
        {
            std::cout << "<<error>> : " << __PRETTY_FUNCTION__ << "event loop handler could not be instantiated" << std::endl;
        }
    }

    if (NULL == mpSocketHandler)
    {
        throw std::runtime_error("socket handler is NULL !!!");
    }

	mpHotplugReceiver = new CAppHoptplugReceiver();

    if (NULL == mpHotplugReceiver)
    {
        // release memory for socket handler if it is internally allocated
        if (NULL == socketHandler)
        {
            delete mpSocketHandler;
            mpSocketHandler = NULL;
        }

        throw std::runtime_error("failed to allocate CAppHoptplugReceiver !!!");
    }

	/*
	* set the event loop handler and IRaHotplugSend pointers in hot plug receiver
	*/
	mpHotplugReceiver->init(mpSocketHandler, this);

	mpSerializer = new am::CAmSerializer(mpSocketHandler);
    if (NULL == mpSerializer)
    {
        delete mpHotplugReceiver;
        mpHotplugReceiver = NULL;

        // check if socket handler is internally allocated
        if (NULL == socketHandler)
        {
            delete mpSocketHandler;
            mpSocketHandler = NULL;
        }

        throw std::runtime_error("failed to allocate CAmSerializer !!!");
    }

	/*
	* if the event loop handler is instantiated by IRaHotplugClient
	* then the event loop handling thread should also be internal
	*/
	if (NULL == socketHandler)
	{
        mpEventLoopThread = new pthread_t();

        if (NULL == mpEventLoopThread)
        {
            delete mpSerializer;
            mpSerializer = NULL;

            delete mpHotplugReceiver;
            mpHotplugReceiver = NULL;

            // check if socket handler is internally allocated
            if (NULL == socketHandler)
            {
                delete mpSocketHandler;
                mpSocketHandler = NULL;
            }

            throw std::runtime_error("failed to allocate pthread_t !!!");
        }

		ret = pthread_create(mpEventLoopThread, NULL, EventLoopThreadHandler, mpSocketHandler);
		if (0 != ret)
		{
            delete mpEventLoopThread;
            mpEventLoopThread = NULL;

            delete mpSerializer;
            mpSerializer = NULL;

            delete mpHotplugReceiver;
            mpHotplugReceiver = NULL;

            // check if socket handler is internally allocated
            if (NULL == socketHandler)
            {
                delete mpSocketHandler;
                mpSocketHandler = NULL;
            }

            throw std::runtime_error("failed to start event loop thread !!!");
		}
	}
}

IRaHotplugClient::~IRaHotplugClient()
{
	if (NULL != mpEventLoopThread)
	{
		mpSocketHandler->exit_mainloop();
		pthread_join(*mpEventLoopThread, NULL);
	}

	if (NULL != mpSerializer)
	{
		delete mpSerializer;
	}

	if (NULL != mpHotplugReceiver)
	{
		delete mpHotplugReceiver;
	}

    if (NULL != mpEventLoopThread)
    {
        if (NULL != mpSocketHandler)
        {
            delete mpSocketHandler;
        }

        delete mpEventLoopThread;
    }

}

void IRaHotplugClient::confirmHotplugReady(const uint16_t handle, const am::am_Error_e error)
{
	mpSerializer->asyncCall<IRaHotplugReceive, const uint16_t, const am::am_Error_e>(mpHotplugReceiver, &IRaHotplugReceive::confirmHotplugReady,  handle, error);
}

void IRaHotplugClient::asyncRegisterSink(const am::am_Sink_s& sinkData)
{
	mpSerializer->asyncCall<IRaHotplugReceive, const am::am_Sink_s>(mpHotplugReceiver, &IRaHotplugReceive::asyncRegisterSink, sinkData);
}

void IRaHotplugClient::asyncDeregisterSink(const am::am_sinkID_t sinkID)
{
	mpSerializer->asyncCall<IRaHotplugReceive, const am::am_sinkID_t>(mpHotplugReceiver, &IRaHotplugReceive::asyncDeregisterSink, sinkID);
}

void IRaHotplugClient::asyncRegisterSource(const am::am_Source_s& sourceData)
{
	mpSerializer->asyncCall<IRaHotplugReceive, const am::am_Source_s>(mpHotplugReceiver, &IRaHotplugReceive::asyncRegisterSource, sourceData);
}

void IRaHotplugClient::asyncDeregisterSource(const am::am_sourceID_t sourceID)
{
	mpSerializer->asyncCall<IRaHotplugReceive, const am::am_sourceID_t>(mpHotplugReceiver, &IRaHotplugReceive::asyncDeregisterSource, sourceID);
}

void IRaHotplugClient::ackSetSinkVolumeChange(const uint16_t handle, const am::am_volume_t volume, const am::am_Error_e error)
{
	mpSerializer->asyncCall<IRaHotplugReceive, const uint16_t, const am::am_volume_t, const am::am_Error_e>(mpHotplugReceiver, &IRaHotplugReceive::ackSetSinkVolumeChange, handle, volume, error);
}

void IRaHotplugClient::ackSetSourceVolumeChange(const uint16_t handle, const am::am_volume_t volume, const am::am_Error_e error)
{
	mpSerializer->asyncCall<IRaHotplugReceive, const uint16_t, const am::am_volume_t, const am::am_Error_e>(mpHotplugReceiver, &IRaHotplugReceive::ackSetSourceVolumeChange, handle, volume, error);
}

void IRaHotplugClient::ackSetSourceState(const uint16_t handle, const am::am_Error_e error)
{
	mpSerializer->asyncCall<IRaHotplugReceive, const uint16_t, const am::am_Error_e>(mpHotplugReceiver, &IRaHotplugReceive::ackSetSourceState, handle, error);
}

void IRaHotplugClient::ackSetSinkSoundProperties(const uint16_t handle, const am::am_Error_e error)
{
	mpSerializer->asyncCall<IRaHotplugReceive, const uint16_t, const am::am_Error_e>(mpHotplugReceiver, &IRaHotplugReceive::ackSetSinkSoundProperties, handle, error);
}

void IRaHotplugClient::ackSetSinkSoundProperty(const uint16_t handle, const am::am_Error_e error)
{
	mpSerializer->asyncCall<IRaHotplugReceive, const uint16_t, const am::am_Error_e>(mpHotplugReceiver, &IRaHotplugReceive::ackSetSinkSoundProperty, handle, error);
}

void IRaHotplugClient::ackSetSourceSoundProperties(const uint16_t handle, const am::am_Error_e error)
{
	mpSerializer->asyncCall<IRaHotplugReceive, const uint16_t, const am::am_Error_e>(mpHotplugReceiver, &IRaHotplugReceive::ackSetSourceSoundProperties, handle, error);
}

void IRaHotplugClient::ackSetSourceSoundProperty(const uint16_t handle, const am::am_Error_e error)
{
	mpSerializer->asyncCall<IRaHotplugReceive, const uint16_t, const am::am_Error_e>(mpHotplugReceiver, &IRaHotplugReceive::ackSetSourceSoundProperty, handle, error);
}

void IRaHotplugClient::hookInterruptStatusChange(const am::am_sourceID_t sourceID, const am::am_InterruptState_e interruptState)
{
	mpSerializer->asyncCall<IRaHotplugReceive, const am::am_sourceID_t, const am::am_InterruptState_e>(mpHotplugReceiver, &IRaHotplugReceive::hookInterruptStatusChange, sourceID, interruptState);
}

void IRaHotplugClient::hookSinkAvailablityStatusChange(const am::am_sinkID_t sinkID, const am::am_Availability_s& availability)
{
	mpSerializer->asyncCall<IRaHotplugReceive, const am::am_sinkID_t, const am::am_Availability_s>(mpHotplugReceiver, &IRaHotplugReceive::hookSinkAvailablityStatusChange, sinkID, availability);
}

void IRaHotplugClient::hookSourceAvailablityStatusChange(const am::am_sourceID_t sourceID, const am::am_Availability_s& availability)
{
	mpSerializer->asyncCall<IRaHotplugReceive, const am::am_sourceID_t, const am::am_Availability_s>(mpHotplugReceiver, &IRaHotplugReceive::hookSourceAvailablityStatusChange, sourceID, availability);
}

void IRaHotplugClient::hookSinkNotificationDataChange(const am::am_sinkID_t sinkID, const am::am_NotificationPayload_s& payload)
{
	mpSerializer->asyncCall<IRaHotplugReceive, const am::am_sinkID_t, const am::am_NotificationPayload_s>(mpHotplugReceiver, &IRaHotplugReceive::hookSinkNotificationDataChange, sinkID, payload);
}

void IRaHotplugClient::hookSourceNotificationDataChange(const am::am_sourceID_t sourceID, const am::am_NotificationPayload_s& payload)
{

	mpSerializer->asyncCall<IRaHotplugReceive, const am::am_sourceID_t, const am::am_NotificationPayload_s>(mpHotplugReceiver, &IRaHotplugReceive::hookSourceNotificationDataChange, sourceID, payload);
}

void IRaHotplugClient::asyncUpdateSource(const am::am_sourceID_t sourceID, const am::am_sourceClass_t sourceClassID, const std::vector<am::am_SoundProperty_s>& listSoundProperties, const std::vector<am::am_CustomConnectionFormat_t>& listConnectionFormats, const std::vector<am::am_MainSoundProperty_s>& listMainSoundProperties)
{
    mpSerializer->asyncCall<IRaHotplugReceive, const am::am_sourceID_t, const am::am_sourceClass_t, const std::vector<am::am_SoundProperty_s>, const std::vector<am::am_CustomConnectionFormat_t>, const std::vector<am::am_MainSoundProperty_s> >(mpHotplugReceiver, &IRaHotplugReceive::asyncUpdateSource, sourceID, sourceClassID, listSoundProperties, listConnectionFormats, listMainSoundProperties);
}

void IRaHotplugClient::asyncUpdateSink(const am::am_sinkID_t sinkID, const am::am_sinkClass_t sinkClassID, const std::vector<am::am_SoundProperty_s>& listSoundProperties, const std::vector<am::am_CustomConnectionFormat_t>& listConnectionFormats, const std::vector<am::am_MainSoundProperty_s>& listMainSoundProperties)
{
    mpSerializer->asyncCall<IRaHotplugReceive, const am::am_sinkID_t, const am::am_sinkClass_t, const std::vector<am::am_SoundProperty_s>, const std::vector<am::am_CustomConnectionFormat_t>, const std::vector<am::am_MainSoundProperty_s> >(mpHotplugReceiver, &IRaHotplugReceive::asyncUpdateSink, sinkID, sinkClassID, listSoundProperties, listConnectionFormats, listMainSoundProperties);
}

void IRaHotplugClient::getInterfaceVersion(std::string& version) const
{
	version = RA_HOTPLUG_CLIENT_INTERFACE_VERSION;
}

am::am_Error_e IRaHotplugClient::init(am::CAmSocketHandler *socketHandler, IRaHotplugReceive* hotplugreceiveinterface)
{
	// no special init is required
	// however IRaHotplugSend interface requires this to be implemented
	(void) socketHandler;
	(void) hotplugreceiveinterface;
	return am::E_OK;
}
