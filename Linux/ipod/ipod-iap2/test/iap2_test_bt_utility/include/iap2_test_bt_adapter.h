#ifndef IAP2_TEST_BT_ADAPTER_H_
#define IAP2_TEST_BT_ADAPTER_H_


/* **********************  includes  ********************** */

#include "iap2_test_bt_utility.h"

/* **********************  defines   ********************** */

/* Adapter1 Interface & Methods */
#define ADAPTER1_INTERFACE                          "org.bluez.Adapter1"

#define ADAPTER1_METHOD_START_DISCOVERY             "StartDiscovery"
#define ADAPTER1_METHOD_STOP_DISCOVERY              "StopDiscovery"
#define ADAPTER1_METHOD_REMOVE_DEVICE               "RemoveDevice"

/* **********************  variables ********************** */


/* **********************  functions ********************** */

S32 iAP2BT_SetBooleanProperties(DBusMessageIter *iter, const char *PropertyName, dbus_bool_t *PropertyValue);
void iAP2BTParseInterfaces(iAP2BTInit *iAP2BT, const char *ObjectPath, DBusMessageIter *InterfaceIter);
S32 iAP2BT_GetManagedObjects(iAP2BTInit *iAP2BT);
S32 iAP2BT_SetAdapterProperties(iAP2BTInit *iAP2BT);
S32 iAP2BT_StartDiscovery(DBusConnection *dbusConn);

#endif /* IAP2_TEST_BT_ADAPTER_H_ */
