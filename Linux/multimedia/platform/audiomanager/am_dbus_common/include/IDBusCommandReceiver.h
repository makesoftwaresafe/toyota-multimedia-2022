/************************************************************************
 * @file: IDBusCommandReceiver.h
 *
 * @version: 1.1
 *
 * @description: A Receiver class shadow definition of command plug-in.
 * Receiver class will make call to AM via DBus connection.This calls will
 * be invoked by application to make the call.
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

#ifndef _I_DBUS_COMMAND_RECEIVER_H_
#define _I_DBUS_COMMAND_RECEIVER_H_

#include <dbus/dbus.h>
#include "CDBusReceiver.h"
#include "CDBusSender.h"
#include "CAmDbusWrapper.h"
#include "CAmDltWrapper.h"
#include "IAmCommand.h"

namespace am
{
/**
 * receives the DBus Callbacks, marhsalls and demarshalls the parameters and calls CommandReceive
 */
class IDBusCommandReceiver: public IAmCommandReceive
{
public:
    IDBusCommandReceiver(DBusConnection *DbusConnection);
    virtual ~IDBusCommandReceiver();
    void getInterfaceVersion(std::string& version) const;
    am_Error_e connect(const am_sourceID_t sourceID, const am_sinkID_t sinkID, am_mainConnectionID_t& mainConnectionID);
    am_Error_e disconnect(const am_mainConnectionID_t mainConnectionID);
    am_Error_e setVolume(const am_sinkID_t sinkID, const am_mainVolume_t volume);
    am_Error_e volumeStep(const am_sinkID_t sinkID, const int16_t volumeStep);
    am_Error_e setSinkMuteState(const am_sinkID_t sinkID, const am_MuteState_e muteState);
    am_Error_e setMainSinkSoundProperty(const am_MainSoundProperty_s& soundProperty, const am_sinkID_t sinkID);
    am_Error_e setMainSourceSoundProperty(const am_MainSoundProperty_s& soundProperty, const am_sourceID_t sourceID);
    am_Error_e setSystemProperty(const am_SystemProperty_s& property);
    am_Error_e getListMainConnections(std::vector<am_MainConnectionType_s>& listConnections) const;
    am_Error_e getListMainSinks(std::vector<am_SinkType_s>& listMainSinks) const;
    am_Error_e getListMainSources(std::vector<am_SourceType_s>& listMainSources) const;
    am_Error_e getListMainSinkSoundProperties(const am_sinkID_t sinkID,
                                              std::vector<am_MainSoundProperty_s>& listSoundProperties) const;
    am_Error_e getListMainSourceSoundProperties(const am_sourceID_t sourceID,
                                                std::vector<am_MainSoundProperty_s>& listSourceProperties) const;
    am_Error_e getListSourceClasses(std::vector<am_SourceClass_s>& listSourceClasses) const;
    am_Error_e getListSinkClasses(std::vector<am_SinkClass_s>& listSinkClasses) const;
    am_Error_e getListSystemProperties(std::vector<am_SystemProperty_s>& listSystemProperties) const;
    am_Error_e getTimingInformation(const am_mainConnectionID_t mainConnectionID, am_timeSync_t& delay) const;
    am_Error_e getDBusConnectionWrapper(CAmDbusWrapper*& dbusConnectionWrapper) const;
    am_Error_e getSocketHandler(CAmSocketHandler*& socketHandler) const;
    void confirmCommandReady(const uint16_t handle, const am_Error_e error);
    void confirmCommandRundown(const uint16_t handle, const am_Error_e error);
    am_Error_e getListMainSinkNotificationConfigurations(
            const am_sinkID_t sinkID, std::vector<am_NotificationConfiguration_s>& listMainNotificationConfigurations) const;
    am_Error_e getListMainSourceNotificationConfigurations(
            const am_sourceID_t sourceID,
            std::vector<am_NotificationConfiguration_s>& listMainNotificationConfigurations) const;
    am_Error_e setMainSinkNotificationConfiguration(const am_sinkID_t sinkID,
                                                    const am_NotificationConfiguration_s& mainNotificationConfiguration);
    am_Error_e setMainSourceNotificationConfiguration(const am_sourceID_t sourceID,
                                                      const am_NotificationConfiguration_s& mainNotificationConfiguration);
    am_Error_e getVolume(const am_sinkID_t sinkID, am_mainVolume_t& mainVolume) const;

private:
    DBusConnection *mpDBusConnection;
    dbus_comm_s mDBusData;

};

} /* namespace am */

#endif /* _I_DBUS_COMMAND_RECEIVER_H_ */
