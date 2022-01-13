
#include "iap2_test_bt_utility.h"

static inline void iAP2BT_PrintError(DBusError *dbusError, const char* FunctionName)
{
    if(dbusError != NULL)
    {
        if (dbus_error_is_set(dbusError))
        {
            printf("%s(): %s\n", FunctionName, dbusError->message);
        }
    }
}


dbus_bool_t dbus_send_message(DBusConnection *connection, DBusMessage *message)
{
    dbus_bool_t result = FALSE;

    if (dbus_message_get_type(message) == DBUS_MESSAGE_TYPE_METHOD_CALL)
    {
        dbus_message_set_no_reply(message, TRUE);
    }
    else if (dbus_message_get_type(message) == DBUS_MESSAGE_TYPE_SIGNAL)
    {
        /* TODO */
    }
    else
    {
        /* TODO */
    }

    /* Flush pending signal to guarantee message order */
    dbus_connection_flush(connection);

    result = dbus_connection_send(connection, message, NULL);

    dbus_message_unref(message);

    return result;
}

dbus_bool_t dbus_send_error_valist(DBusConnection *connection, DBusMessage *message, const char *name, const char *format, va_list args)
{
    DBusMessage *error;
    char str[1024];

    vsnprintf(str, sizeof(str), format, args);

    error = dbus_message_new_error(message, name, str);
    if (error == NULL)
        return FALSE;

    return dbus_send_message(connection, error);
}

dbus_bool_t dbus_send_error(DBusConnection *connection, DBusMessage *message, const char *name, const char *format, ...)
{
    va_list args;
    dbus_bool_t result;

    va_start(args, format);

    result = dbus_send_error_valist(connection, message, name, format, args);

    va_end(args);

    return result;
}

static dbus_bool_t dbus_send_reply_valist(DBusConnection *connection, DBusMessage *message, int type, va_list args)
{
    DBusMessage *reply;

    reply = dbus_message_new_method_return(message);
    if (reply == NULL)
        return FALSE;

    if (dbus_message_append_args_valist(reply, type, args) == FALSE)
    {
        dbus_message_unref(reply);
        return FALSE;
    }

    return dbus_send_message(connection, reply);
}

dbus_bool_t dbus_send_reply(DBusConnection *connection, DBusMessage *message, int type, ...)
{
    va_list args;
    dbus_bool_t result;

    va_start(args, type);

    result = dbus_send_reply_valist(connection, message, type, args);

    va_end(args);

    return result;
}

S32 iAP2BT_dbusSendMsgWithReply(DBusConnection *dbusConn, DBusMessage *dbusMsg, DBusPendingCall **dbusPendingCall, S32 timeout)
{
    S32 rc = IAP2_OK;

    if( dbus_connection_send_with_reply(dbusConn, dbusMsg, dbusPendingCall, timeout) == FALSE)
    {
        rc = IAP2_CTL_ERROR;
    }
    else if(dbusPendingCall != NULL && *dbusPendingCall == NULL)
    {
        printf("Unable to send message (passing fd blocked?)\n");
        rc = IAP2_CTL_ERROR;
    }
    else
    {
        /* None */
    }

    return rc;
}

S32 iAP2BT_dbusMethodCall(iAP2BTMethodCall BTMethodCall, DBusConnection *dbusConn)
{
    S32 rc = IAP2_OK;
    DBusMessage     *dbusMsg, *dbusReplyMsg;

    dbusMsg = dbus_message_new_method_call((const char*)BLUEZ_SERVICE_NAME,
                                           (const char*)BTMethodCall.ObjectPath,
                                           (const char*)BTMethodCall.InterfaceName,
                                           (const char*)BTMethodCall.MethodName);
    if(dbusMsg == NULL)
    {
        printf("Error in dbus_message_new_method_call()\n");
        rc = IAP2_CTL_ERROR;
    }

    /* Construct your message associated with the Method Call */
    if( (rc == IAP2_OK) && (BTMethodCall.SetupCallback) )
    {
        DBusMessageIter iter;

        dbus_message_iter_init_append(dbusMsg, &iter);
        BTMethodCall.SetupCallback(&iter, BTMethodCall.UserData);
    }

    DBusError dbusError;

    /* Initialize a DBusError structure. */
    dbus_error_init(&dbusError);

    /* Send the message that was formed and block until  DBUS_TIMEOUT_USE_DEFAULT (-1) time period while waiting for a reply */
    dbusReplyMsg = dbus_connection_send_with_reply_and_block(dbusConn, dbusMsg, DBUS_TIMEOUT_USE_DEFAULT, &dbusError);
    if(BTMethodCall.ReplyCallback != NULL)
    {
        BTMethodCall.ReplyCallback(dbusReplyMsg, BTMethodCall.UserData, &dbusError);
    }

    dbus_message_unref(dbusMsg);

    return rc;
}

static inline S32 iAP2BT_AssignNameToConnection(iAP2BTInit *iAP2BT)
{
    S32 rc = IAP2_OK;

    rc = dbus_bus_request_name(iAP2BT->dbusConn,
                               (const char*)iAP2BT->dbusName,
                               DBUS_NAME_FLAG_DO_NOT_QUEUE,
                               &iAP2BT->dbusError);

    if(rc == -1)
    {
        iAP2BT_PrintError(&iAP2BT->dbusError, __FUNCTION__);
    }

    return rc;
}

static S32 iAP2BT_SetupDBusWithGMain(iAP2BTInit *iAP2BT)
{
    S32 rc = IAP2_OK;

    if (iAP2BT->dbusName != NULL)
    {
        rc = iAP2BT_AssignNameToConnection(iAP2BT);
    }

    if(rc == IAP2_OK)
    {
        iAP2BT->gmain_loop = g_main_loop_new(NULL, FALSE);
        dbus_connection_setup_with_g_main(iAP2BT->dbusConn, NULL);
    }

    return rc;
}

S32 iAP2InitializeDBusConnection(iAP2BTInit *iAP2BT)
{
    S32 rc = IAP2_OK;

    iAP2BT->dbusConn = dbus_bus_get(iAP2BT->dbusBustype, &iAP2BT->dbusError);

    if(iAP2BT->dbusConn == NULL)
    {
        rc = IAP2_CTL_ERROR;

        printf("%s:%d %s(): dbusConn is NULL\n",__FILE__, __LINE__, __FUNCTION__);
        iAP2BT_PrintError(&iAP2BT->dbusError, __FUNCTION__);
    }

    if(rc == IAP2_OK)
    {
        printf("Establishing dbusConn successfull \n");
        rc = iAP2BT_SetupDBusWithGMain(iAP2BT);
        printf("iAP2BT_SetupDBus returns rc = %d \n", rc);
    }

    if( (rc != IAP2_OK) && (iAP2BT->dbusConn != NULL) )
    {
        dbus_connection_unref(iAP2BT->dbusConn);
        iAP2BT->dbusConn = NULL;
    }

    return rc;
}
