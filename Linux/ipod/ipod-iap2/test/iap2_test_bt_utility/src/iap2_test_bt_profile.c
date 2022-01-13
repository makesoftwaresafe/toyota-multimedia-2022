
#include "iap2_test_bt_profile.h"

iAP2BT_Profile1Methods Profile1Methods[] = {
    { PROFILE1_METHOD_RELEASE,                  PROFILE1_RELEASE},
    { PROFILE1_METHOD_NEW_CONNECTION,           PROFILE1_NEW_CONNECTION},
    { PROFILE1_METHOD_REQUEST_DISCONNECTION,    PROFILE1_REQUEST_DISCONNECTION}
};

static inline void iAP2BT_HandleProfile1Method(DBusConnection *dbusConn, DBusMessage *dbusMsg, void *data, Profile1Method Method)
{
    (void)data;

    switch(Method)
    {
        case PROFILE1_RELEASE:
        {
            printf("Invoked Profile1 Method: Release");
            break;
        }
        case PROFILE1_NEW_CONNECTION:
        {
            printf("Invoked Profile1 Method: NewConnection");
            break;
        }
        case PROFILE1_REQUEST_DISCONNECTION:
        {
            printf("Invoked Profile1 Method: RequestDisconnection");
            break;
        }
        default:
        {
            dbus_send_error(dbusConn, dbusMsg, "org.bluez.Error.Canceled", NULL);
            break;
        }
    }
}

static inline BOOL iAP2BT_IdentifyInvokedProfile1Method(DBusMessage *dbusMsg, const char *MethodName)
{
    BOOL rc = FALSE;

    if( dbus_message_is_method_call(dbusMsg, PROFILE1_INTERFACE, MethodName) )
    {
        printf("InvokedProfile1Method: %s\n", MethodName);
        rc = TRUE;
    }

    return rc;
}

static DBusHandlerResult iAP2BT_Profile1Message(DBusConnection *dbusConn, DBusMessage *dbusMsg, void *data)
{
    DBusHandlerResult res = DBUS_HANDLER_RESULT_NOT_YET_HANDLED ;
    int i;

    (void)data;

    for(i = 0; i < PROFILE1_MAX_METHODS; i++)
    {
        if( iAP2BT_IdentifyInvokedProfile1Method(dbusMsg, Profile1Methods[i].MethodName) )
            break;
    }

    if(i == PROFILE1_MAX_METHODS)
    {
        dbus_send_error(dbusConn, dbusMsg, "org.bluez.Error.Canceled", NULL);
    }
    else
    {
        iAP2BT_HandleProfile1Method(dbusConn, dbusMsg, data, Profile1Methods[i].Method);
    }

    return res;
}

static void iAP2BTRegisterProfileUtil(DBusMessageIter *iter, const char* UUID)
{
    DBusMessageIter dict, entry, value;
    dbus_uint16_t channel;
    char *record;
    const char *str;

    dbus_message_iter_append_basic(iter, DBUS_TYPE_STRING, &UUID);

    dbus_message_iter_open_container(iter,
                                     DBUS_TYPE_ARRAY,
                                     DBUS_DICT_ENTRY_BEGIN_CHAR_AS_STRING
                                     DBUS_TYPE_STRING_AS_STRING
                                     DBUS_TYPE_VARIANT_AS_STRING
                                     DBUS_DICT_ENTRY_END_CHAR_AS_STRING, &dict);

    dbus_message_iter_open_container(&dict, DBUS_TYPE_DICT_ENTRY, NULL, &entry);
    str = "Role";
    dbus_message_iter_append_basic(&entry, DBUS_TYPE_STRING, &str);

    dbus_message_iter_open_container(&entry, DBUS_TYPE_VARIANT, DBUS_TYPE_STRING_AS_STRING, &value);
    str = "server";
    dbus_message_iter_append_basic(&value, DBUS_TYPE_STRING, &str);

    dbus_message_iter_close_container(&entry, &value);
    dbus_message_iter_close_container(&dict, &entry);

    dbus_message_iter_open_container(&dict, DBUS_TYPE_DICT_ENTRY, NULL, &entry);
    str = "Channel";
    dbus_message_iter_append_basic(&entry, DBUS_TYPE_STRING, &str);

    dbus_message_iter_open_container(&entry, DBUS_TYPE_VARIANT, DBUS_TYPE_UINT16_AS_STRING, &value);
    channel = 23;
    dbus_message_iter_append_basic(&value, DBUS_TYPE_UINT16, &channel);

    dbus_message_iter_close_container(&entry, &value);
    dbus_message_iter_close_container(&dict, &entry);

    dbus_message_iter_open_container(&dict, DBUS_TYPE_DICT_ENTRY, NULL, &entry);
    str = "ServiceRecord";
    dbus_message_iter_append_basic(&entry, DBUS_TYPE_STRING, &str);

    dbus_message_iter_open_container(&entry, DBUS_TYPE_VARIANT, DBUS_TYPE_STRING_AS_STRING, &value);
    record = g_strdup_printf(IAP_OVER_BT_RECORD, IAP_OVER_BT_UUID, channel);
    dbus_message_iter_append_basic(&value, DBUS_TYPE_STRING, &record);
    g_free(record);

    dbus_message_iter_close_container(&entry, &value);
    dbus_message_iter_close_container(&dict, &entry);

    dbus_message_iter_close_container(iter, &dict);
}

static void iAP2BT_RegisterProfileSetupCallback(DBusMessageIter *iter, void *user_data)
{
    const char *path = PROFILE_PATH;

    (void)user_data;

    dbus_message_iter_append_basic(iter, DBUS_TYPE_OBJECT_PATH, &path);
    iAP2BTRegisterProfileUtil(iter, IAP_OVER_BT_UUID);
    //iAP2BTRegisterProfileUtil(iter, "2d8d2466-e14d-451c-88bc-7301abea291a");
}

static void iAP2BT_RegisterProfileReplyCallback(DBusMessage *dbusMsg, void *user_data, DBusError *dbusError)
{
    (void)user_data;

    if (!dbusMsg)
    {
        printf("ERROR: Failed to request default agent\n");
        if (dbus_error_is_set(dbusError))
        {
            printf("%s\n", dbusError->message);
            dbus_error_free(dbusError);
        }
    }
    else
    {
        printf("iAP2-BT Profile Registration successful\n");
    }
}

static const DBusObjectPathVTable Profile1Table = {
    .message_function = iAP2BT_Profile1Message,
};

S32 iAP2BT_RegisterProfile(DBusConnection *dbusConn)
{
    S32 rc = IAP2_OK;
    const char *ProfilePath = PROFILE_PATH;
    iAP2BTMethodCall BTMethodCall = {
            .InterfaceName  = (U8*)PROFILE_MANAGER_INTERFACE,
            .MethodName     = (U8*)PROFILE_MANAGER_METHOD_REGISTER_PROFILE,
            .ObjectPath     = (U8*)"/org/bluez",
            .ReplyCallback  = iAP2BT_RegisterProfileReplyCallback,
            .SetupCallback  = iAP2BT_RegisterProfileSetupCallback,
            .UserData       = NULL
        };

    if(dbus_connection_register_object_path(dbusConn, ProfilePath, &Profile1Table, NULL) == FALSE)
    {
        printf("ERROR: While dbusConn_register_object_path() \n");
        rc = IAP2_CTL_ERROR;
    }
    else
    {
        iAP2BT_dbusMethodCall(BTMethodCall, dbusConn);
    }

    return rc;
}
