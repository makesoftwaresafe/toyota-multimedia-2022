
#include "iap2_test_bt_device.h"
#include "iap2_test_bt_adapter.h"

static void iAP2BT_ConnectProfileReplyCallback(DBusMessage *dbusMsg, void *user_data, DBusError *dbusError)
{
    const char *ObjectPath = (char*)user_data;

    if (!dbusMsg)
    {
        printf("ERROR: Failed to Connect Profile to %s\n", ObjectPath);
        if (dbus_error_is_set(dbusError))
        {
            printf("%s\n", dbusError->message);
            dbus_error_free(dbusError);
        }
    }
    else
    {
        printf("Connected to profile successfully %s\n", ObjectPath);
    }
}

static void iAP2BT_ConnectProfileSetupCall(DBusMessageIter *iter, void *user_data)
{
    dbus_bool_t ret;
    char *ProfileUUID = APPLE_CPLAY_UUID;

    (void)user_data;

    ret = dbus_message_iter_append_basic(iter, DBUS_TYPE_STRING, &ProfileUUID);
    if(ret == FALSE)
    {
        printf("ERROR: While appending Adapter Interface Name %s\n", ProfileUUID);
    }
}

static void iAP2BT_TrustDeviceReplyCallback(DBusMessage *dbusMsg, void *user_data, DBusError *dbusError)
{
    const char *ObjectPath = (char*)user_data;

    if (!dbusMsg)
    {
        printf("ERROR: Failed to Set Trusted for %s\n", ObjectPath);
        if (dbus_error_is_set(dbusError))
        {
            printf("%s\n", dbusError->message);
            dbus_error_free(dbusError);
        }
    }
    else
    {
        printf("Set Trusted successful for %s\n", ObjectPath);
    }
}

static void iAP2BT_TrustDeviceSetupCall(DBusMessageIter *iter, void *user_data)
{
    S32 rc = IAP2_OK;
    dbus_bool_t ret;
    char *Device1InterfaceName = IAP2_BT_DEVICE1_INTERFACE;

    (void)user_data;

    ret = dbus_message_iter_append_basic(iter, DBUS_TYPE_STRING, &Device1InterfaceName);
    if(ret == FALSE)
    {
        printf("ERROR: While appending Adapter Interface Name %s\n", Device1InterfaceName);
        rc = IAP2_CTL_ERROR;
    }

    /* Set Trusted - TRUE */
    if(rc == IAP2_OK)
    {
        dbus_bool_t Trusted = TRUE;

        rc = iAP2BT_SetBooleanProperties(iter, "Trusted", &Trusted);
    }
}

static S32 iAP2BT_ConnectDevice(DBusConnection *dbusConn, Device1 DeviceProperties)
{
    S32 rc = IAP2_OK;
    DBusMessage *dbusSendMsg = NULL;

    dbusSendMsg = dbus_message_new_method_call(BLUEZ_SERVICE_NAME, DeviceProperties.ObjectPath, IAP2_BT_DEVICE1_INTERFACE, IAP2_BT_DEVICE1_METHOD_CONNECT);
    if (!dbusSendMsg)
    {
        printf("ERROR: Can't allocate new method call\n");
        rc = IAP2_CTL_ERROR;
    }
    if(rc == IAP2_OK)
    {
        (void)dbus_connection_send_with_reply(dbusConn, dbusSendMsg, NULL, DBUS_TIMEOUT_USE_DEFAULT);
    }

    return rc;
}

static S32 iAP2BT_StartPairing(DBusConnection *dbusConn, Device1 DeviceProperties)
{
    S32 rc = IAP2_OK;
    DBusMessage *dbusSendMsg = NULL;

    dbusSendMsg = dbus_message_new_method_call(BLUEZ_SERVICE_NAME, DeviceProperties.ObjectPath, IAP2_BT_DEVICE1_INTERFACE, IAP2_BT_DEVICE1_METHOD_PAIR);
    if (!dbusSendMsg)
    {
        printf("ERROR: Can't allocate new method call\n");
        rc = IAP2_CTL_ERROR;
    }
    if(rc == IAP2_OK)
    {
        (void)dbus_connection_send_with_reply(dbusConn, dbusSendMsg, NULL, DBUS_TIMEOUT_USE_DEFAULT);
    }

    return rc;
}

static S32 iAP2BT_TrustDevice(DBusConnection *dbusConn, Device1 DeviceProperties)
{
    S32 rc = IAP2_OK;
    iAP2BTMethodCall BTMethodCall = {
            .InterfaceName  = (U8*)DBUS_PROPERTIES_INTERFACE,
            .MethodName     = (U8*)DBUS_PROPERTIES_METHOD_SET,
            .ObjectPath     = (U8*)DeviceProperties.ObjectPath,
            .ReplyCallback  = iAP2BT_TrustDeviceReplyCallback,
            .SetupCallback  = iAP2BT_TrustDeviceSetupCall,
            .UserData       = DeviceProperties.ObjectPath
        };

    rc = iAP2BT_dbusMethodCall(BTMethodCall, dbusConn);

    return rc;
}

S32 iAP2BTInitializePairingSequence(DBusConnection *dbusConn, Device1 DeviceProperties)
{
    S32 rc = IAP2_OK;

    if(DeviceProperties.Trusted != TRUE)
    {
        rc = iAP2BT_TrustDevice(dbusConn, DeviceProperties);
    }
    if(rc == IAP2_OK)
    {
        rc = iAP2BT_StartPairing(dbusConn, DeviceProperties);
    }
    if(rc == IAP2_OK)
    {
        rc = iAP2BT_ConnectDevice(dbusConn, DeviceProperties);
    }

    return rc;
}

S32 iAP2BT_ConnectProfile(DBusConnection *dbusConn, Device1 DeviceProperties)
{
    S32 rc = IAP2_OK;
    iAP2BTMethodCall BTMethodCall = {
            .InterfaceName  = (U8*)IAP2_BT_DEVICE1_INTERFACE,
            .MethodName     = (U8*)IAP2_BT_DEVICE1_METHOD_CONNECT_PROFILE,
            .ObjectPath     = (U8*)DeviceProperties.ObjectPath,
            .ReplyCallback  = iAP2BT_ConnectProfileReplyCallback,
            .SetupCallback  = iAP2BT_ConnectProfileSetupCall,
            .UserData       = DeviceProperties.ObjectPath
        };

    rc = iAP2BT_dbusMethodCall(BTMethodCall, dbusConn);

    return rc;
}
