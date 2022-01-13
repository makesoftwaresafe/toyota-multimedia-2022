
#include "iap2_test_bt_agent.h"

iAP2BT_Agent1Methods Agent1Methods[] = {
    { AGENT1_METHOD_RELEASE, AGENT1_RELEASE},
    { AGENT1_METHOD_REQUEST_PIN_CODE, AGENT1_REQUEST_PIN_CODE},
    { AGENT1_METHOD_DISPLAY_PIN_CODE, AGENT1_DISPLAY_PIN_CODE},
    { AGENT1_METHOD_REQUEST_PASS_KEY, AGENT1_REQUEST_PASS_KEY},
    { AGENT1_METHOD_DISPLAY_PASS_KEY, AGENT1_DISPLAY_PASS_KEY},
    { AGENT1_METHOD_REQUEST_CONFIRMATION, AGENT1_REQUEST_CONFIRMATION},
    { AGENT1_METHOD_REQUEST_AUTHORIZATION, AGENT1_REQUEST_AUTHORIZATION},
    { AGENT1_METHOD_AUTHORIZE_SERVICE, AGENT1_AUTHORIZE_SERVICE},
    { AGENT1_METHOD_CANCEL, AGENT1_CANCEL}
};

BOOL RequestConfirmation = FALSE;

BOOL iAP2BT_RequestConfirmationStatus()
{
    return RequestConfirmation;
}

static inline void iAP2BT_HandleInvokedMethod(DBusConnection *dbusConn, DBusMessage *dbusMsg, void *data, AgentMethod Method)
{
    switch(Method)
    {
        case AGENT1_RELEASE:
        case AGENT1_DISPLAY_PIN_CODE:
        case AGENT1_DISPLAY_PASS_KEY:
        {
            /* No Action Required */
            break;
        }
        case AGENT1_REQUEST_PIN_CODE:
        {
            dbus_send_reply(dbusConn, dbusMsg, DBUS_TYPE_STRING, &data, DBUS_TYPE_INVALID);
            break;
        }
        case AGENT1_REQUEST_PASS_KEY:
        {
            dbus_uint32_t passkey = 1234;

            dbus_send_reply(dbusConn, dbusMsg, DBUS_TYPE_UINT32, &passkey, DBUS_TYPE_INVALID);
            break;
        }
        case AGENT1_REQUEST_CONFIRMATION:
        case AGENT1_REQUEST_AUTHORIZATION:
        case AGENT1_AUTHORIZE_SERVICE:
        {
            /* Just confirm without user interaction */
            dbus_send_reply(dbusConn, dbusMsg, DBUS_TYPE_INVALID);
            RequestConfirmation = TRUE;
            break;
        }
        case AGENT1_CANCEL:
        default:
        {
            dbus_send_error(dbusConn, dbusMsg, "org.bluez.Error.Canceled", NULL);
            break;
        }
    }
}

static inline BOOL iAP2BT_IdentifyInvokedMethod(DBusMessage *dbusMsg, const char *MethodName)
{
    BOOL rc = FALSE;

    if( dbus_message_is_method_call(dbusMsg, AGENT_INTERFACE, MethodName) )
    {
        printf("InvokedAgent1Method: %s\n", MethodName);
        rc = TRUE;
    }

    return rc;
}

static DBusHandlerResult iAP2BT_AgentMessage(DBusConnection *dbusConn, DBusMessage *dbusMsg, void *data)
{
    DBusHandlerResult res = DBUS_HANDLER_RESULT_NOT_YET_HANDLED ;
    int i;

    for(i = 0; i < AGENT1_MAX_METHODS; i++)
    {
        if( iAP2BT_IdentifyInvokedMethod(dbusMsg, Agent1Methods[i].MethodName) )
            break;
    }

    if(i == AGENT1_MAX_METHODS)
    {
        dbus_send_error(dbusConn, dbusMsg, "org.bluez.Error.Canceled", NULL);
    }
    else
    {
        iAP2BT_HandleInvokedMethod(dbusConn, dbusMsg, data, Agent1Methods[i].Method);
    }

    return res;
}

static const DBusObjectPathVTable AgentTable = {
    .message_function = iAP2BT_AgentMessage,
};

static void iAP2BT_DefaultAgentSetupCallback(DBusMessageIter *iter, void *user_data)
{
    const char *DefaultAgentPath = AGENT_PATH;

    (void)user_data;

    dbus_message_iter_append_basic(iter, DBUS_TYPE_OBJECT_PATH, &DefaultAgentPath);
}

static void iAP2BT_DefaultAgentReplyCallback(DBusMessage *dbusMsg, void *user_data, DBusError *dbusError)
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
        printf("Default agent request successful\n");
    }
}

static void iAP2BT_RegisterAgentSetupCallback(DBusMessageIter *iter, void *user_data)
{
    const char *AgentPath = AGENT_PATH;
    const char *AgentCapabilites = "KeyboardDisplay";

    (void)user_data;

    dbus_message_iter_append_basic(iter, DBUS_TYPE_OBJECT_PATH, &AgentPath);
    dbus_message_iter_append_basic(iter, DBUS_TYPE_STRING,      &AgentCapabilites);
}

static void iAP2BT_RegisterAgentReplyCallback(DBusMessage *dbusMsg, void *user_data, DBusError *dbusError)
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
        printf("Agent Registered\n");
    }
}


S32 iAP2BT_SetDefaultAgent(DBusConnection *dbusConn)
{
    S32 rc = IAP2_OK;
    iAP2BTMethodCall BTMethodCall = {
            .InterfaceName  = (U8*)AGENT_MANAGER_INTERFACE,
            .MethodName     = (U8*)AGENTMANAGER_METHOD_REQUEST_DEFAULT_AGENT,
            .ObjectPath     = (U8*)"/org/bluez",
            .ReplyCallback  = iAP2BT_DefaultAgentReplyCallback,
            .SetupCallback  = iAP2BT_DefaultAgentSetupCallback,
            .UserData       = NULL
        };

    iAP2BT_dbusMethodCall(BTMethodCall, dbusConn);

    return rc;
}

S32 iAP2BT_RegisterAgent(DBusConnection *dbusConn)
{
    S32 rc = IAP2_OK;
    const char *AgentPath = AGENT_PATH;
    iAP2BTMethodCall BTMethodCall = {
            .InterfaceName  = (U8*)AGENT_MANAGER_INTERFACE,
            .MethodName     = (U8*)AGENTMANAGER_METHOD_REGISTER_AGENT,
            .ObjectPath     = (U8*)"/org/bluez",
            .ReplyCallback  = iAP2BT_RegisterAgentReplyCallback,
            .SetupCallback  = iAP2BT_RegisterAgentSetupCallback,
            .UserData       = NULL
        };

    if(dbus_connection_register_object_path(dbusConn, AgentPath, &AgentTable, NULL) == FALSE)
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
