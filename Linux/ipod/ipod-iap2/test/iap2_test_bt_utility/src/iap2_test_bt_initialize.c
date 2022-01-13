
#include "iap2_test_bt_initialize.h"
#include "iap2_test_bt_agent.h"
#include "iap2_test_bt_adapter.h"
#include "iap2_test_bt_profile.h"
#include "iap2_test_bt_device.h"

#include <sys/socket.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/rfcomm.h>
#include <errno.h>

#define APPLE_CPLAY_UUID "2d8d2466-e14d-451c-88bc-7301abea291a"

S32 iAP2BTEstablishConnection(char *BTMacAddress)
{
    S32 iAP2BTSocket;
    S32 rc = IAP2_OK;

    iAP2BTSocket = socket(AF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM);
    if(iAP2BTSocket == -1)
    {
        printf("Error in Creating the Socket, errno = %d (%s)\n", errno, strerror(errno));
        rc = IPOD_DATACOM_ERROR;
    }
    else
    {
        struct sockaddr_rc addr_new = { 0 };

        printf("Allocated Socket Successfully\n");

        // set the connection parameters (who to connect to)
        addr_new.rc_family  = AF_BLUETOOTH;
        addr_new.rc_channel = (uint8_t) 1;
        str2ba(BTMacAddress, &addr_new.rc_bdaddr);

        // connect to server
        rc = connect(iAP2BTSocket, (struct sockaddr *)&addr_new, sizeof(addr_new));
        if(rc == -1)
        {
            printf("Error in creating connection, errno = %d (%s)\n", errno, strerror(errno));
        }
        else
        {
            printf("connected through BT Successfully\n");
            close(iAP2BTSocket);
        }
    }

    return rc;
}

static void iAP2BTCheckForCarPlaySupport(iAP2BTInit *iAP2BT, U32 DeviceCount)
{
    U32 i;

    for(i = 0; i < iAP2BT->DeviceProperties[DeviceCount].UUID_Count; i++)
    {
        if(iAP2BT->DeviceProperties[DeviceCount].UUID[i] != NULL)
        {
            if(strncmp(iAP2BT->DeviceProperties[DeviceCount].UUID[i], APPLE_CPLAY_UUID, strlen(APPLE_CPLAY_UUID) ) == 0)
            {
                printf("Device with Apple CarPlay support found\n");
                iAP2BTInitializePairingSequence(iAP2BT->dbusConn, iAP2BT->DeviceProperties[DeviceCount]);
                iAP2BT->DeviceProperties[DeviceCount].CarPlaySupported = TRUE;
                iAP2BT->CarPlayDeviceFound = TRUE;

                if(iAP2BT->DeviceProperties[DeviceCount].Connection == FALSE)
                {
                    char SEM_NAME[]= "iAP2BT";
                    sem_t *iAP2OverBT;

                    //create & initialize semaphore
                    iAP2OverBT = sem_open(SEM_NAME, O_CREAT, 0644, 0);
                    if(iAP2OverBT == SEM_FAILED)
                    {
                        printf("unable to create semaphore\n");
                        sem_unlink(SEM_NAME);
                    }
                    else
                    {
                        sem_post(iAP2OverBT);
                        printf("Posted Semaphore\n");
                    }
                }

                break;
            }
            else
            {
                iAP2BT->DeviceProperties[DeviceCount].CarPlaySupported = FALSE;
            }
        }
    }
}

static void iAP2BTHandleInterfaceAdded(iAP2BTInit *iAP2BT, DBusMessage *dbusMsg)
{
    S32 rc = IAP2_OK;
    DBusMessageIter dbusMsg_Iter;
    const char *ObjectPath;

    if(dbus_message_iter_init(dbusMsg, &dbusMsg_Iter) == FALSE)
    {
        printf("dbusMsgFilter ERROR: while performing dbus_message_iter_init() \n");
        rc = IAP2_CTL_ERROR;
    }
    if(rc == IAP2_OK)
    {
        if(dbus_message_iter_get_arg_type(&dbusMsg_Iter) != DBUS_TYPE_OBJECT_PATH)
        {
            printf("dbusMsgFilter ERROR: ObjectPath not present\n");
            rc = IAP2_CTL_ERROR;
        }
    }
    if(rc == IAP2_OK)
    {
        /* Get the ObjectPath */
        dbus_message_iter_get_basic(&dbusMsg_Iter, &ObjectPath);
        printf("ObjectPath = %s\n", ObjectPath);
        dbus_message_iter_next(&dbusMsg_Iter);
        iAP2BTParseInterfaces(iAP2BT, ObjectPath, &dbusMsg_Iter);

        /* Check whether the device supports CarPlay */
        iAP2BTCheckForCarPlaySupport(iAP2BT, (iAP2BT->DeviceProp_count - 1) );
    }
}

static DBusHandlerResult dbusMsgFilter(DBusConnection *dbusConn, DBusMessage *dbusMsg, void *UserData)
{
    DBusHandlerResult rc = DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    const char *MsgType, *Sender, *Path, *Interface, *Member;
    int Msg_Type;
    char *dbusMsgTypes[DBUS_NUM_MESSAGE_TYPES] = {
        "DBUS_MESSAGE_TYPE_INVALID",
        "DBUS_MESSAGE_TYPE_METHOD_CALL",
        "DBUS_MESSAGE_TYPE_METHOD_RETURN",
        "DBUS_MESSAGE_TYPE_ERROR",
        "DBUS_MESSAGE_TYPE_SIGNAL",
    };
    iAP2BTInit *iAP2BT = (iAP2BTInit*)UserData;

    (void)dbusConn;

    Msg_Type = dbus_message_get_type(dbusMsg);

    MsgType     = dbusMsgTypes[Msg_Type];
    Sender      = dbus_message_get_sender(dbusMsg);
    Path        = dbus_message_get_path(dbusMsg);
    Interface   = dbus_message_get_interface(dbusMsg);
    Member      = dbus_message_get_member(dbusMsg);

    printf("\n\ndbusMsgFilter: \n\t");
    printf("MsgType   = %s \n\t", MsgType);
    printf("Sender    = %s \n\t", Sender);
    printf("Path      = %s \n\t", Path);
    printf("Interface = %s \n\t", Interface);
    printf("Member    = %s \n\n", Member);

    if( ( Msg_Type == DBUS_MESSAGE_TYPE_SIGNAL ) &&
        ( strncmp(Interface, "org.freedesktop.DBus.ObjectManager", strlen("org.freedesktop.DBus.ObjectManager") ) == 0 ) &&
        ( strncmp(Member, "InterfacesAdded", strlen("InterfacesAdded") ) == 0 ) )
    {
        iAP2BTHandleInterfaceAdded(iAP2BT, dbusMsg);
    }

    return rc;
}

S32 iAP2BTRegisterMessageFilter(iAP2BTInit *iAP2BT)
{
    S32 rc = IAP2_OK;
    DBusError dbusError;

    dbus_error_init(&dbusError);

    /* Register for Signal from ObjectManager Interface */
    dbus_bus_add_match(iAP2BT->dbusConn, "type='signal',interface='org.freedesktop.DBus.ObjectManager'", &dbusError);
    if (dbus_error_is_set(&dbusError))
    {
        printf("ERROR: While adding match for signal from ObjectManager Interface %s\n", dbusError.message);
        dbus_error_free(&dbusError);
        rc = IAP2_CTL_ERROR;
    }

    if(rc == IAP2_OK)
    {
        /* Register for Signal from Properties Interface */
        dbus_bus_add_match(iAP2BT->dbusConn, "type='signal',interface='org.freedesktop.DBus.Properties'", &dbusError);
        if (dbus_error_is_set(&dbusError))
        {
            printf("ERROR: While adding match for signal from Properties Interface %s\n", dbusError.message);
            dbus_error_free(&dbusError);
            rc = IAP2_CTL_ERROR;
        }
    }
    if(rc == IAP2_OK)
    {
        dbus_bool_t ret;

        ret = dbus_connection_add_filter(iAP2BT->dbusConn, dbusMsgFilter, iAP2BT, NULL);
        if(ret == FALSE)
        {
            printf("Error: in dbus_connection_add_filter()\n");
            rc = IAP2_CTL_ERROR;
        }
    }

    return rc;
}

S32 iAP2BTRegisterAgentInterface(iAP2BTInit *iAP2BT)
{
    S32 rc = IAP2_OK;

    rc = iAP2BT_RegisterAgent(iAP2BT->dbusConn);
    printf("iAP2BT_RegisterAgent returns rc = %d \n", rc);
    if(rc == IAP2_OK)
    {
        rc = iAP2BT_SetDefaultAgent(iAP2BT->dbusConn);
        printf("iAP2BT_SetDefaultAgent returns rc = %d \n", rc);
    }

    return rc;
}

S32 iAP2BTScanForDevices(iAP2BTInit *iAP2BT)
{
    S32 rc = IAP2_OK;

    /* Set Adapter Properties - power on, pairable on, discoverable on, etc., */
    rc = iAP2BT_SetAdapterProperties(iAP2BT);
    printf("iAP2BT_SetAdapterProperties returns rc = %d \n", rc);
    if(rc == IAP2_OK)
    {
        rc = iAP2BT_StartDiscovery(iAP2BT->dbusConn);
    }

    return rc;
}

S32 iAP2InitializeBTConnection(iAP2BTInit *iAP2BT)
{
    S32 rc = IAP2_OK;

    dbus_error_init(&iAP2BT->dbusError);

    iAP2BT->dbusBustype = DBUS_BUS_SYSTEM;

    rc = iAP2InitializeDBusConnection(iAP2BT);
    if(rc == IAP2_OK)
    {
        rc = iAP2BTRegisterMessageFilter(iAP2BT);
        printf("iAP2BTRegisterMessageFilter returns rc = %d \n", rc);
    }
    if(rc == IAP2_OK)
    {
        rc = iAP2BT_RegisterProfile(iAP2BT->dbusConn);
        printf("iAP2BT_RegisterProfile returns rc = %d \n", rc);
    }
    if(rc == IAP2_OK)
    {
        rc = iAP2BTRegisterAgentInterface(iAP2BT);
        printf("iAP2BTRegisterAgentInterface returns rc = %d \n", rc);
    }
    if(rc == IAP2_OK)
    {
        rc = iAP2BT_GetManagedObjects(iAP2BT);
        printf("iAP2BT_GetManagedObjects returns rc = %d \n", rc);
    }

    return rc;
}
