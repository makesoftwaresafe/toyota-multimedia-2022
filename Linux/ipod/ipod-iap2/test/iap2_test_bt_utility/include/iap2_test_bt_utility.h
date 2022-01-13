#ifndef IAP2_TEST_BT_UTILITY_H_
#define IAP2_TEST_BT_UTILITY_H_


/* **********************  includes  ********************** */
#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>
#include "iap2_test_utility.h"

/* **********************  defines   ********************** */

#define BLUEZ_SERVICE_NAME "org.bluez"

/* ObjectManager Interface & Methods */
#define OBJECT_MANAGER_INTERFACE                    "org.freedesktop.DBus.ObjectManager"

#define OBJECT_MANAGER_METHOD_GET_MANAGED_OBJECTS   "GetManagedObjects"

/* Properties Interface & Methods */
#define DBUS_PROPERTIES_INTERFACE                   "org.freedesktop.DBus.Properties"

#define DBUS_PROPERTIES_METHOD_SET                  "Set"
#define DBUS_PROPERTIES_METHOD_GET                  "Get"
#define DBUS_PROPERTIES_METHOD_GET_ALL              "GetAll"

#define APPLE_CPLAY_UUID "2d8d2466-e14d-451c-88bc-7301abea291a"

/* **********************  variables ********************** */

typedef void (* dbusSetupCallback) (DBusMessageIter *iter, void *user_data);
typedef void (* dbusReplyCallback) (DBusMessage *message,  void *user_data, DBusError *dbusError);

typedef struct
{
    char            *Address;
    char            *Name;
    char            *Alias;
    dbus_uint32_t   Class;
    dbus_bool_t     Powered;
    dbus_bool_t     Discoverable;
    dbus_uint32_t   DiscoverableTimeout;
    dbus_bool_t     Pairable;
    dbus_uint32_t   PairableTimeout;
    dbus_bool_t     Discovering;

    char            *PropertyName;
    dbus_bool_t     TypeIsVariant;
    char            **UUID;
    U32             UUID_Count;
}Adapter1;

typedef struct
{
    char            *Address;
    char            *Name;
    char            *Alias;
    dbus_uint32_t   Class;
    char            *Icon;
    dbus_bool_t     Paired;
    dbus_bool_t     Trusted;
    dbus_bool_t     Blocked;
    dbus_bool_t     LegacyPairing;
    dbus_bool_t     Connected;
    char            **UUID;
    U32             UUID_Count;
    char            *ObjectPath;
    BOOL            CarPlaySupported;
    BOOL            Connection;
}Device1;

typedef struct
{
    const U8 *InterfaceName;
    const U8 *ObjectPath;
    const U8 *MethodName;
    dbusSetupCallback SetupCallback;
    dbusReplyCallback ReplyCallback;
    void *UserData;
}iAP2BTMethodCall;

typedef struct
{
    DBusConnection *dbusConn;
    DBusError      dbusError;
    DBusBusType    dbusBustype;
    U8             *dbusName;
    GMainLoop      *gmain_loop;
    Adapter1       AdapterProperties;
    Device1        DeviceProperties[10];
    U32            DeviceProp_count;
    BOOL           CarPlayDeviceFound;
}iAP2BTInit;

/* **********************  functions ********************** */

dbus_bool_t dbus_send_error(DBusConnection *connection, DBusMessage *message, const char *name, const char *format, ...);
dbus_bool_t dbus_send_reply(DBusConnection *connection, DBusMessage *message, int type, ...);
S32 iAP2BT_dbusMethodCall(iAP2BTMethodCall BTMethodCall, DBusConnection *dbusConn);
S32 iAP2BT_dbusSendMsgWithReply(DBusConnection *dbusConn, DBusMessage *dbusMsg, DBusPendingCall **dbusPendingCall, S32 timeout);
S32 iAP2InitializeDBusConnection(iAP2BTInit *iAP2BT);

#endif /* IAP2_TEST_BT_UTILITY_H_ */
