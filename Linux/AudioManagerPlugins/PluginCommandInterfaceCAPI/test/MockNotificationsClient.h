/**
 *  Copyright (c) 2012 BMW
 *
 *  \author Aleksandar Donchev, aleksander.donchev@partner.bmw.de BMW 2013
 *
 *  \copyright
 *  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction,
 *  including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so,
 *  subject to the following conditions:
 *  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 *  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR
 *  THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 *  For further information see http://www.genivi.org/.
 */


#ifndef MOCKNOTIFICATIONSCLIENT_H_
#define MOCKNOTIFICATIONSCLIENT_H_

#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "../include/CAmCommandSenderCommon.h"
#include <v1/org/genivi/am/commandinterface/CommandControlProxy.hpp>


namespace am {
using namespace testing;


class IAmNotificationsClient
{
public:
		IAmNotificationsClient() {}
		virtual ~IAmNotificationsClient() {}
	    virtual void onNewMainConnection(const am_types::am_MainConnectionType_s &) = 0 ;
	    virtual void removedMainConnection(am_types::am_mainConnectionID_t) = 0 ;
	    virtual void onNumberOfSourceClassesChangedEvent() = 0 ;
		virtual void onMainConnectionStateChangedEvent(am_types::am_mainConnectionID_t, am_types::am_ConnectionState_e) = 0 ;
		virtual void onSourceAddedEvent(const am_types::am_SourceType_s &) = 0 ;
		virtual void onSourceRemovedEvent(am_types::am_sourceID_t) = 0  ;
		virtual void onMainSourceSoundPropertyChangedEvent(am_types::am_sourceID_t, const am_types::am_MainSoundProperty_s & ) = 0 ;
		virtual void onSourceAvailabilityChangedEvent(am_types::am_sourceID_t, const am_types::am_Availability_s &) = 0 ;
		virtual void onNumberOfSinkClassesChangedEvent() = 0 ;
		virtual void onSinkAddedEvent(const am_types::am_SinkType_s &)= 0  ;
		virtual void onSinkRemovedEvent(am_types::am_sinkID_t) = 0  ;
		virtual void onMainSinkSoundPropertyChangedEvent(am_types::am_sinkID_t, const am_types::am_MainSoundProperty_s &) = 0  ;
		virtual void onSinkAvailabilityChangedEvent(am_types::am_sinkID_t, const am_types::am_Availability_s &) = 0 ;
		virtual void onVolumeChangedEvent(am_types::am_sinkID_t, am_types::am_mainVolume_t)  = 0 ;
		virtual void onSinkMuteStateChangedEvent(am_types::am_sinkID_t, am_types::am_MuteState_e)  = 0 ;
		virtual void onSystemPropertyChangedEvent(const am_types::am_SystemProperty_s &) = 0 ;
		virtual void onTimingInformationChangedEvent(am_types::am_mainConnectionID_t, am_types::am_timeSync_t)  = 0 ;
		virtual void onSinkUpdatedEvent(am_types::am_sinkID_t, am_types::am_sinkClass_t, const am_types::am_MainSoundProperty_L &)  = 0 ;
		virtual void onSourceUpdatedEvent(am_types::am_sourceID_t, am_types::am_sourceClass_t, const am_types::am_MainSoundProperty_L &)   = 0 ;
		virtual void onSinkNotificationEvent(am_types::am_sinkID_t, const am_types::am_NotificationPayload_s & ) = 0 ;
		virtual void onSourceNotificationEvent(am_types::am_sourceID_t, const am_types::am_NotificationPayload_s &)  = 0 ;
		virtual void onMainSinkNotificationConfigurationChangedEvent(am_types::am_sinkID_t, const am_types::am_NotificationConfiguration_s &)  = 0 ;
		virtual void onMainSourceNotificationConfigurationChangedEvent(am_types::am_sourceID_t, const am_types::am_NotificationConfiguration_s &) = 0 ;
};

class MockNotificationsClient : public IAmNotificationsClient {
public:
	 MOCK_METHOD1(onNewMainConnection,
	      void(const am_types::am_MainConnectionType_s&));
	  MOCK_METHOD1(removedMainConnection,
	      void(am_types::am_mainConnectionID_t));
	  MOCK_METHOD0(onNumberOfSourceClassesChangedEvent,
	      void());
	  MOCK_METHOD2(onMainConnectionStateChangedEvent,
	      void(am_types::am_mainConnectionID_t, am_types::am_ConnectionState_e));
	  MOCK_METHOD1(onSourceAddedEvent,
	      void(const am_types::am_SourceType_s &));
	  MOCK_METHOD1(onSourceRemovedEvent,
	      void(am_types::am_sourceID_t));
	  MOCK_METHOD2(onMainSourceSoundPropertyChangedEvent,
	      void(am_types::am_sourceID_t, const am_types::am_MainSoundProperty_s&));
	  MOCK_METHOD2(onSourceAvailabilityChangedEvent,
	      void(am_types::am_sourceID_t, const am_types::am_Availability_s&));
	  MOCK_METHOD0(onNumberOfSinkClassesChangedEvent,
	      void());
	  MOCK_METHOD1(onSinkAddedEvent,
	      void(const am_types::am_SinkType_s&));
	  MOCK_METHOD1(onSinkRemovedEvent,
	      void(am_types::am_sinkID_t));
	  MOCK_METHOD2(onMainSinkSoundPropertyChangedEvent,
	      void(am_types::am_sinkID_t, const am_types::am_MainSoundProperty_s&));
	  MOCK_METHOD2(onSinkAvailabilityChangedEvent,
	      void(am_types::am_sinkID_t, const am_types::am_Availability_s&));
	  MOCK_METHOD2(onVolumeChangedEvent,
	      void(am_types::am_sinkID_t, am_types::am_mainVolume_t));
	  MOCK_METHOD2(onSinkMuteStateChangedEvent,
	      void(am_types::am_sinkID_t, am_types::am_MuteState_e));
	  MOCK_METHOD1(onSystemPropertyChangedEvent,
	      void(const am_types::am_SystemProperty_s&));
	  MOCK_METHOD2(onTimingInformationChangedEvent,
	      void(am_types::am_mainConnectionID_t, am_types::am_timeSync_t));
	  MOCK_METHOD3(onSinkUpdatedEvent,
	      void(am_types::am_sinkID_t, am_types::am_sinkClass_t, const am_types::am_MainSoundProperty_L&));
	  MOCK_METHOD3(onSourceUpdatedEvent,
	      void(am_types::am_sourceID_t, am_types::am_sourceClass_t, const am_types::am_MainSoundProperty_L&));
	  MOCK_METHOD2(onSinkNotificationEvent,
	      void(am_types::am_sinkID_t, const am_types::am_NotificationPayload_s&));
	  MOCK_METHOD2(onSourceNotificationEvent,
	      void(am_types::am_sourceID_t, const am_types::am_NotificationPayload_s&));
	  MOCK_METHOD2(onMainSinkNotificationConfigurationChangedEvent,
	      void(am_types::am_sinkID_t, const am_types::am_NotificationConfiguration_s&));
	  MOCK_METHOD2(onMainSourceNotificationConfigurationChangedEvent,
	      void(am_types::am_sourceID_t, const am_types::am_NotificationConfiguration_s&));
	};



}  // namespace am
#endif /* MOCKCOMMANDRECEIVENTERFACE_H_ */
