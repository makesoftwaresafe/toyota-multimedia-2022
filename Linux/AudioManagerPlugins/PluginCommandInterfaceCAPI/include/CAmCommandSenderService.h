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

#ifndef CAMCOMMANDSENDERSERVICE_H_
#define CAMCOMMANDSENDERSERVICE_H_


#include "v1/org/genivi/am/commandinterface/CommandControlStubDefault.hpp"
#include "CAmCommandSenderCommon.h"
#include "IAmCommand.h"




namespace am {

/**
 * A concrete stub implementation used by the command sender plug-in.
 */
class CAmCommandSenderService: public v1::org::genivi::am::commandinterface::CommandControlStubDefault {
	IAmCommandReceive* mpIAmCommandReceive;
public:
	CAmCommandSenderService();
	CAmCommandSenderService(IAmCommandReceive *aReceiver);
	virtual ~CAmCommandSenderService();

		void connect(const std::shared_ptr<CommonAPI::ClientId> _client, am_types::am_sourceID_t _sourceID, am_types::am_sinkID_t _sinkID, connectReply_t _reply);

	    void disconnect(const std::shared_ptr<CommonAPI::ClientId> _client, am_types::am_mainConnectionID_t _mainConnectionID, disconnectReply_t _reply);

	    void setVolume(const std::shared_ptr<CommonAPI::ClientId> _client, am_types::am_sinkID_t _sinkID, am_types::am_mainVolume_t _volume, setVolumeReply_t _reply);

	    void volumeStep(const std::shared_ptr<CommonAPI::ClientId> _client, am_types::am_sinkID_t _sinkID, int16_t _step, volumeStepReply_t _reply);

	    void setSinkMuteState(const std::shared_ptr<CommonAPI::ClientId> _client, am_types::am_sinkID_t _sinkID, am_types::am_MuteState_e _muteState, setSinkMuteStateReply_t _reply);

	    void setMainSinkSoundProperty(const std::shared_ptr<CommonAPI::ClientId> _client, am_types::am_sinkID_t _sinkID, am_types::am_MainSoundProperty_s _soundProperty, setMainSinkSoundPropertyReply_t _reply);

	    void setMainSourceSoundProperty(const std::shared_ptr<CommonAPI::ClientId> _client, am_types::am_sourceID_t _sourceID, am_types::am_MainSoundProperty_s _soundProperty, setMainSourceSoundPropertyReply_t _reply);

	    void setSystemProperty(const std::shared_ptr<CommonAPI::ClientId> _client, am_types::am_SystemProperty_s _property, setSystemPropertyReply_t _reply);

	    void getListMainConnections(const std::shared_ptr<CommonAPI::ClientId> _client, getListMainConnectionsReply_t _reply);

	    void getListMainSinks(const std::shared_ptr<CommonAPI::ClientId> _client, getListMainSinksReply_t _reply);

	    void getListMainSources(const std::shared_ptr<CommonAPI::ClientId> _client, getListMainSourcesReply_t _reply);

	    void getListMainSinkSoundProperties(const std::shared_ptr<CommonAPI::ClientId> _client, am_types::am_sinkID_t _sinkID, getListMainSinkSoundPropertiesReply_t _reply);

	    void getListMainSourceSoundProperties(const std::shared_ptr<CommonAPI::ClientId> _client, am_types::am_sourceID_t _sourceID, getListMainSourceSoundPropertiesReply_t _reply);

	    void getListSinkClasses(const std::shared_ptr<CommonAPI::ClientId> _client, getListSinkClassesReply_t _reply);

	    void getListSourceClasses(const std::shared_ptr<CommonAPI::ClientId> _client, getListSourceClassesReply_t _reply);

	    void getListSystemProperties(const std::shared_ptr<CommonAPI::ClientId> _client, getListSystemPropertiesReply_t _reply);

	    void getTimingInformation(const std::shared_ptr<CommonAPI::ClientId> _client, am_types::am_mainConnectionID_t _mainConnectionID, getTimingInformationReply_t _reply);

	    void getListMainSinkNotificationConfigurations(const std::shared_ptr<CommonAPI::ClientId> _client, am_types::am_sinkID_t _sinkID, getListMainSinkNotificationConfigurationsReply_t _reply);

	    void getListMainSourceNotificationConfigurations(const std::shared_ptr<CommonAPI::ClientId> _client, am_types::am_sourceID_t _sourceID, getListMainSourceNotificationConfigurationsReply_t _reply);

	    void setMainSinkNotificationConfiguration(const std::shared_ptr<CommonAPI::ClientId> _client, am_types::am_sinkID_t _sinkID, am_types::am_NotificationConfiguration_s _mainNotificationConfiguration, setMainSinkNotificationConfigurationReply_t _reply);

	    void setMainSourceNotificationConfiguration(const std::shared_ptr<CommonAPI::ClientId> _client, am_types::am_sourceID_t _sourceID, am_types::am_NotificationConfiguration_s _mainNotificationConfiguration, setMainSourceNotificationConfigurationReply_t _reply);

};

} /* namespace am */
#endif /* CAMCOMMANDSENDERSERVICE_H_ */
