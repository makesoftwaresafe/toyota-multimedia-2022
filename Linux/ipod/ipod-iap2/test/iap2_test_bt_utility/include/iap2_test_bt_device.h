#ifndef IAP2_TEST_BT_DEVICE_H_
#define IAP2_TEST_BT_DEVICE_H_


/* **********************  includes  ********************** */

#include "iap2_test_bt_utility.h"

/* **********************  defines   ********************** */

#define IAP2_BT_DEVICE1_INTERFACE                   "org.bluez.Device1"

#define IAP2_BT_DEVICE1_METHOD_CONNECT              "Connect"
#define IAP2_BT_DEVICE1_METHOD_DISCONNECT           "Disconnect"
#define IAP2_BT_DEVICE1_METHOD_CONNECT_PROFILE      "ConnectProfile"
#define IAP2_BT_DEVICE1_METHOD_DISCONNECT_PROFILE   "DisconnectProfile"
#define IAP2_BT_DEVICE1_METHOD_PAIR                 "Pair"
#define IAP2_BT_DEVICE1_METHOD_CANCEL_PAIRING       "CancelPairing"

/* **********************  variables ********************** */


/* **********************  functions ********************** */

S32 iAP2BT_ConnectProfile(DBusConnection *dbusConn, Device1 DeviceProperties);
S32 iAP2BTInitializePairingSequence(DBusConnection *dbusConn, Device1 DeviceProperties);

#endif /* IAP2_TEST_BT_DEVICE_H_ */
