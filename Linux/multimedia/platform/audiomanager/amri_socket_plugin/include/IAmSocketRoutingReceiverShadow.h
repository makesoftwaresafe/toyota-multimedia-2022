/************************************************************************
 * @file: IRaDbusWrpReceiverShadow.h
 *
 * @version: 1.1
 *
 * @description: A Receiver class shadow definition of Routing Adapter.
 * IAmSocketRoutingReceiverShadow class will be running with the AM process.
 * IAmSocketRoutingReceiverShadow class methods will be called via DBus to
 * this methods will intern call the Actual call in AM.
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

#ifndef IAMDBUSROUTINGRECEIVERSHADOW_H_
#define IAMDBUSROUTINGRECEIVERSHADOW_H_

#include <string>
#include <vector>
#include <map>
#include "CAmSocketWrapper.h"
#include "IAmRouting.h"
#include "CSocketSender.h"

namespace am
{
class CAmSocketRoutingSender;
/**
 * receives the DBus Callbacks, marhsalls and demarshalls the parameters and calls CommandReceive
 */
class IAmSocketRoutingReceiverShadow
{
public:
	IAmSocketRoutingReceiverShadow(CAmSocketRoutingSender* mpCAmSocketRoutingSender);
	virtual ~IAmSocketRoutingReceiverShadow();
	void ackConnect(void *msg);
	void ackDisconnect(void *msg);
	void ackSetSinkVolumeChange(void *msg);
	void ackSetSourceVolumeChange(void *msg);
	void ackSetSourceState(void *msg);
	void ackSinkVolumeTick(void *msg);
	void ackSourceVolumeTick(void *msg);
	void ackSetSinkSoundProperty(void *msg);
	void ackSetSourceSoundProperty(void *msg);
	void ackSetSinkSoundProperties(void *msg);
	void ackSetSourceSoundProperties(void *msg);
	void ackCrossFading(void *msg);
	void registerDomain(void *msg);
	void registerSource(void *msg);
	void registerSink(void *msg);
	void registerGateway(void *msg);
	void peekDomain(void *msg);
	void deregisterDomain(void *msg);
	void deregisterGateway(void *msg);
	void peekSink(void *msg);
	void deregisterSink(void *msg);
	void peekSource(void *msg);
	void deregisterSource(void *msg);
	void registerCrossfader(void *msg);
	void deregisterCrossfader(void *msg);
	void peekSourceClassID(void *msg);
	void peekSinkClassID(void *msg);
	void hookInterruptStatusChange(void *msg);
	void hookDomainRegistrationComplete(void *msg);
	void hookSinkAvailablityStatusChange(void *msg);
	void hookSourceAvailablityStatusChange(void *msg);
	void hookDomainStateChange(void *msg);
	void hookTimingInformationChanged(void *msg);
	void sendChangedData(void *msg);
	void confirmRoutingReady(void *msg);
	void confirmRoutingRundown(void *msg);
	void updateGateway(void *msg);
	void updateSink(void *msg);
	void updateSource(void *msg);
	void ackSetVolumes(void *msg);
	void ackSinkNotificationConfiguration(void *msg);
	void ackSourceNotificationConfiguration(void *msg);
	void hookSinkNotificationDataChange(void *msg);
	void hookSourceNotificationDataChange(void *msg);
	void getInterfaceVersion(void *msg);
	void getRoutingReady(void *msg);

	/**
	 * sets the pointer to the CommandReceiveInterface and registers Callback
	 * @param receiver
	 */
	void setRoutingReceiver( IAmRoutingReceive*& receiver );

	void receiveCallback(void* pmsg);

	TRecieveCallBack<IAmSocketRoutingReceiverShadow> mpRecieveCallBack;

private:
	IAmRoutingReceive*		mpIAmRoutingReceive;
	typedef void (IAmSocketRoutingReceiverShadow::*CallBackMethod)(void* message);
	typedef std::map<unsigned short, CallBackMethod>	functionMap_t;
	functionMap_t			mFunctionMap;
	CAmSocketRoutingSender*	mpCAmSocketRoutingSender;
	unsigned short 			mCurrRecvAddr;

	/**
	 * creates the function map needed to combine DBus messages and function adresses
	 * @return the map
	 */
	functionMap_t createMap();
};

} /* namespace am*/

#endif /* IAMDBUSROUTINGRECEIVERSHADOW_H_ */
