/*******************************************************************************
 *  \copyright (c) 2016 Advanced Driver Information Technology.
 *                   ADIT is a joint venture company of
 *   Robert Bosch GmbH/Robert Bosch Car Multimedia GmbH and DENSO Corporation
 *
 *  \author: Jens Lorenz, jlorenz@de.adit-jv.com 2015-2016
 *           Mattia Guerra, mguerra@de.adit-jv.com 2016
 *
 *
 *  \copyright The MIT License (MIT)
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"),
 *  to deal in the Software without restriction, including without limitation
 *  the rights to use, copy, modify, merge, publish, distribute, sublicense,
 *  and/or sell copies of the Software, and to permit persons to whom the
 *  Software is furnished to do so, subject to the following conditions:
 *  The above copyright notice and this permission notice shall be included in
 *  all copies or substantial portions of the Software.
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 *  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 *  DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 *  OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR
 *  THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 *  For further information see http://www.genivi.org/.
 ******************************************************************************/


#include "IAmRoutingReceiverShadow.h"
#include <cassert>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <string.h>
#include <netdb.h>
#include <fcntl.h>
#include <sys/un.h>
#include <errno.h>
#include <string>
#include "CAmDltWrapper.h"
#include "CAmSerializer.h"

namespace am {

IAmRoutingReceiverShadow::IAmRoutingReceiverShadow(IAmRoutingReceive* iReceiveInterface,
    CAmSocketHandler* iSocketHandler, IAmRoutingReceiverObserver* iObserver) :
        mpSocketHandler(iSocketHandler),
        mpRoutingReceiveInterface(iReceiveInterface),
        mpObserver(iObserver),
        mSerializer(iSocketHandler)
{
    assert(mpRoutingReceiveInterface!=NULL);
    assert(mpSocketHandler!=NULL);
}

IAmRoutingReceiverShadow::~IAmRoutingReceiverShadow()
{
}

void IAmRoutingReceiverShadow::ackConnect(const am_Handle_s handle, const am_connectionID_t connectionID, const am_Error_e error)
{
    mSerializer.asyncCall<IAmRoutingReceive,const am_Handle_s,const am_connectionID_t,const am_Error_e>(mpRoutingReceiveInterface,&IAmRoutingReceive::ackConnect,handle,connectionID,error);
}

void IAmRoutingReceiverShadow::ackDisconnect(const am_Handle_s handle, const am_connectionID_t connectionID, const am_Error_e error)
{
    mSerializer.asyncCall<IAmRoutingReceive,const am_Handle_s,const am_connectionID_t,const am_Error_e>(mpRoutingReceiveInterface,&IAmRoutingReceive::ackDisconnect,handle,connectionID,error);
}

void IAmRoutingReceiverShadow::ackSetSinkVolumeChange(const am_Handle_s handle, const am_volume_t volume, const am_Error_e error)
{
    mSerializer.asyncCall<IAmRoutingReceive,const am_Handle_s,const am_volume_t,const am_Error_e>(mpRoutingReceiveInterface,&IAmRoutingReceive::ackSetSinkVolumeChange,handle,volume,error);
}

void IAmRoutingReceiverShadow::ackSetSourceVolumeChange(const am_Handle_s handle, const am_volume_t volume, const am_Error_e error)
{
    mSerializer.asyncCall<IAmRoutingReceive,const am_Handle_s,const am_volume_t,const am_Error_e>(mpRoutingReceiveInterface,&IAmRoutingReceive::ackSetSourceVolumeChange,handle,volume,error);
}

void IAmRoutingReceiverShadow::ackSetSourceState(const am_Handle_s handle, const am_Error_e error)
{
    mSerializer.asyncCall<IAmRoutingReceive,const am_Handle_s,const am_Error_e>(mpRoutingReceiveInterface,&IAmRoutingReceive::ackSetSourceState,handle,error);
}

void IAmRoutingReceiverShadow::ackSetSinkSoundProperty(const am_Handle_s handle, const am_Error_e error)
{
    mSerializer.asyncCall<IAmRoutingReceive,const am_Handle_s,const am_Error_e>(mpRoutingReceiveInterface,&IAmRoutingReceive::ackSetSinkSoundProperty,handle,error);
}

void IAmRoutingReceiverShadow::ackSetSourceSoundProperty(const am_Handle_s handle, const am_Error_e error)
{
    mSerializer.asyncCall<IAmRoutingReceive,const am_Handle_s,const am_Error_e>(mpRoutingReceiveInterface,&IAmRoutingReceive::ackSetSourceSoundProperty,handle,error);
}

void IAmRoutingReceiverShadow::ackCrossFading(const am_Handle_s handle, const am_HotSink_e hotSink, const am_Error_e error)
{
    mSerializer.asyncCall<IAmRoutingReceive,const am_Handle_s,const am_HotSink_e,const am_Error_e>(mpRoutingReceiveInterface,&IAmRoutingReceive::ackCrossFading,handle,hotSink,error);
}

void IAmRoutingReceiverShadow::ackSourceVolumeTick(const am_Handle_s handle, const am_sourceID_t sourceID, const am_volume_t volume)
{
    mSerializer.asyncCall<IAmRoutingReceive,const am_Handle_s,const am_sourceID_t,const am_volume_t>(mpRoutingReceiveInterface,&IAmRoutingReceive::ackSourceVolumeTick,handle,sourceID,volume);
}

void IAmRoutingReceiverShadow::ackSinkVolumeTick(const am_Handle_s handle, const am_sinkID_t sinkID, const am_volume_t volume)
{
    mSerializer.asyncCall<IAmRoutingReceive,const am_Handle_s,const am_sinkID_t,const am_volume_t>(mpRoutingReceiveInterface,&IAmRoutingReceive::ackSinkVolumeTick,handle,sinkID,volume);
}

void IAmRoutingReceiverShadow::hookInterruptStatusChange(const am_sourceID_t sourceID, const am_InterruptState_e interruptState)
{
    mpRoutingReceiveInterface->hookInterruptStatusChange(sourceID, interruptState);
}

void IAmRoutingReceiverShadow::hookSinkAvailablityStatusChange(const am_sinkID_t sinkID, const am_Availability_s & availability)
{
    mpRoutingReceiveInterface->hookSinkAvailablityStatusChange(sinkID, availability);
}

void IAmRoutingReceiverShadow::hookSourceAvailablityStatusChange(const am_sourceID_t sourceID, const am_Availability_s & availability)
{
    mpRoutingReceiveInterface->hookSourceAvailablityStatusChange(sourceID, availability);
}

void IAmRoutingReceiverShadow::hookDomainStateChange(const am_domainID_t domainID, const am_DomainState_e domainState)
{
    mpRoutingReceiveInterface->hookDomainStateChange(domainID, domainState);
}

void IAmRoutingReceiverShadow::hookDomainRegistrationComplete(const am_domainID_t domain)
{
    mpRoutingReceiveInterface->hookDomainRegistrationComplete(domain);
}

void IAmRoutingReceiverShadow::hookTimingInformationChanged(const am_connectionID_t connectionID, const am_timeSync_t delay)
{
    mpRoutingReceiveInterface->hookTimingInformationChanged(connectionID, delay);
}

am_Error_e IAmRoutingReceiverShadow::registerDomain(const am_Domain_s & domainData, am_domainID_t & domainID)
{
    return mpRoutingReceiveInterface->registerDomain(domainData, domainID);
}

am_Error_e IAmRoutingReceiverShadow::deregisterDomain(const am_domainID_t domainID)
{
    return mpRoutingReceiveInterface->deregisterDomain(domainID);
}

am_Error_e IAmRoutingReceiverShadow::registerGateway(const am_Gateway_s & gatewayData, am_gatewayID_t & gatewayID)
{
    return mpRoutingReceiveInterface->registerGateway(gatewayData, gatewayID);
}

am_Error_e IAmRoutingReceiverShadow::deregisterGateway(const am_gatewayID_t gatewayID)
{
    return mpRoutingReceiveInterface->deregisterGateway(gatewayID);
}

am_Error_e IAmRoutingReceiverShadow::registerSink(const am_Sink_s & sinkData, am_sinkID_t & sinkID)
{
    return mpRoutingReceiveInterface->registerSink(sinkData, sinkID);
}

am_Error_e IAmRoutingReceiverShadow::deregisterSink(const am_sinkID_t sinkID)
{
    return mpRoutingReceiveInterface->deregisterSink(sinkID);
}

am_Error_e IAmRoutingReceiverShadow::registerSource(const am_Source_s & sourceData, am_sourceID_t & sourceID)
{
    return mpRoutingReceiveInterface->registerSource(sourceData, sourceID);
}

am_Error_e IAmRoutingReceiverShadow::deregisterSource(const am_sourceID_t sourceID)
{
    return mpRoutingReceiveInterface->deregisterSource(sourceID);
}

am_Error_e IAmRoutingReceiverShadow::registerCrossfader(const am_Crossfader_s & crossfaderData, am_crossfaderID_t & crossfaderID)
{
    return mpRoutingReceiveInterface->registerCrossfader(crossfaderData, crossfaderID);
}

void IAmRoutingReceiverShadow::confirmRoutingReady(uint16_t starupHandle, am_Error_e error)
{
    mpRoutingReceiveInterface->confirmRoutingReady(starupHandle, error);
}

void IAmRoutingReceiverShadow::confirmRoutingRundown(uint16_t rundownHandle, am_Error_e error)
{
    mpRoutingReceiveInterface->confirmRoutingRundown(rundownHandle, error);
}

void IAmRoutingReceiverShadow::ackSinkNotificationConfiguration(const am_Handle_s handle, const am_Error_e error)
{
    mSerializer.asyncCall<IAmRoutingReceive,const am_Handle_s,const am_Error_e>(mpRoutingReceiveInterface,&IAmRoutingReceive::ackSinkNotificationConfiguration,handle,error);
}

void IAmRoutingReceiverShadow::ackSourceNotificationConfiguration(const am_Handle_s handle, const am_Error_e error)
{
    mSerializer.asyncCall<IAmRoutingReceive,const am_Handle_s,const am_Error_e>(mpRoutingReceiveInterface,&IAmRoutingReceive::ackSourceNotificationConfiguration,handle,error);
}

void IAmRoutingReceiverShadow::hookSinkNotificationDataChange(const am_sinkID_t sinkID, const am_NotificationPayload_s& payload)
{
    mpRoutingReceiveInterface->hookSinkNotificationDataChange(sinkID, payload);
}

void IAmRoutingReceiverShadow::hookSourceNotificationDataChange(const am_sourceID_t sourceID, const am_NotificationPayload_s& payload)
{
    mpRoutingReceiveInterface->hookSourceNotificationDataChange(sourceID, payload);
}

void IAmRoutingReceiverShadow::asyncDeleteVolume(const am_Handle_s handle, const class CAmRoutingAdapterALSAVolume* volume)
{
    assert(mpObserver!=NULL);
    mSerializer.asyncCall<IAmRoutingReceiverObserver,const am_Handle_s,const class CAmRoutingAdapterALSAVolume*>(mpObserver,&IAmRoutingReceiverObserver::asyncDeleteVolume,handle,volume);
}

}
