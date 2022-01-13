/************************************************************************
* @file: RaDBusConfig.h
*
* @version: 1.1
*
* This file contains the D-Bus paramters for the hotplug interface
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
#ifndef __RA_DBUS_CONFIG_H__
#define __RA_DBUS_CONFIG_H__

// routing adapter hosts hotplug receive service
#define DBUS_RA_HOTPLUG_SERVICE_PREFIX "org.adit.kp.routingadapter"
#define DBUS_RA_HOTPLUG_SERVICE_OBJECT_PATH "/org/adit/kp/routingadapter"

#define DBUS_HOTPLUG_RECEIVE_INTERFACE "org.adit.kp.routingadapter.hotplugreceive"
#define DBUS_HOTPLUG_RECEIVE_INTERFACE_OBJECT_PATH "/org/adit/kp/routingadapter/hotplugreceive"

// application hosts hotplug send service
#define DBUS_APP_HOTPLUG_SERVICE_PREFIX "org.adit.kp.application"
#define DBUS_APP_HOTPLUG_SERVICE_OBJECT_PATH "/org/adit/kp/application"

#define DBUS_HOTPLUG_SEND_INTERFACE "org.adit.kp.application.hotplugsend"
#define DBUS_HOTPLUG_SEND_INTERFACE_OBJECT_PATH "/org/adit/kp/application/hotplugsend"

#endif //__RA_DBUS_CONFIG_H__
