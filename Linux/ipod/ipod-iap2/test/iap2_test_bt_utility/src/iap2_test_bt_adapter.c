
#include "iap2_test_bt_adapter.h"

#define APPLE_IAP2_BT_UUID "00000000-deca-fade-deca-deafdecacafe"

static S32 iAP2BTExtractProperty(void *Dest, void *Value, const char *PropName, const char *PropName_ToCmp, int ArgType)
{
    S32 rc = IAP2_OK;

    rc = strncmp(PropName, PropName_ToCmp, strlen(PropName_ToCmp) );
    if(rc == IAP2_OK)
    {
        switch(ArgType)
        {
            case DBUS_TYPE_STRING:
            {
                char *val     = (char*)Value;
                char **p_Dest = (char**)Dest;

                printf ("string \"");
                printf ("%s\"\n", val);

                *p_Dest = strndup(Value, strlen(val) );
                if(*p_Dest == NULL)
                {
                    printf("ERROR: Could not allocate memory for %s", PropName_ToCmp);
                }
                break;
            }
            case DBUS_TYPE_INT16:
            {
                dbus_int16_t *val    = (dbus_int16_t*)Value;
                dbus_int16_t *p_Dest = (dbus_int16_t*)Dest;

                printf ("int16 %d\n", *val);
                *p_Dest = *val;
                break;
            }
            case DBUS_TYPE_UINT16:
            {
                dbus_uint16_t *val    = (dbus_uint16_t*)Value;
                dbus_uint16_t *p_Dest = (dbus_uint16_t*)Dest;

                printf ("uint16 %u\n", *val);
                *p_Dest = *val;
                break;
            }
            case DBUS_TYPE_INT32:
            {
                dbus_int32_t *val    = (dbus_int32_t*)Value;
                dbus_int32_t *p_Dest = (dbus_int32_t*)Dest;

                printf ("int32 %d\n", *val);
                *p_Dest = *val;
                break;
            }
            case DBUS_TYPE_UINT32:
            {
                dbus_uint32_t *val    = (dbus_uint32_t*)Value;
                dbus_uint32_t *p_Dest = (dbus_uint32_t*)Dest;

                printf ("uint32 %u\n", *val);
                *p_Dest = *val;
                break;
            }
            case DBUS_TYPE_INT64:
            {
                dbus_int64_t *val    = (dbus_int64_t*)Value;
                dbus_int64_t *p_Dest = (dbus_int64_t*)Dest;

                printf ("int64 %lld\n", *val);
                *p_Dest = *val;
                break;
            }
            case DBUS_TYPE_UINT64:
            {
                dbus_uint64_t *val    = (dbus_uint64_t*)Value;
                dbus_uint64_t *p_Dest = (dbus_uint64_t*)Dest;

                printf ("uint64 %llu\n", *val);
                *p_Dest = *val;
                break;
            }
            case DBUS_TYPE_DOUBLE:
            {
                double *val    = (double*)Value;
                double *p_Dest = (double*)Dest;

                printf ("double %g\n", *val);
                *p_Dest = *val;
                break;
            }
            case DBUS_TYPE_BYTE:
            {
                char *val = (char*)Value;

                printf ("byte %d\n", *val);
                break;
            }
            case DBUS_TYPE_BOOLEAN:
            {
                dbus_bool_t *val    = (dbus_bool_t*)Value;
                dbus_bool_t *p_Dest = (dbus_bool_t*)Dest;

                printf ("boolean %s\n", *val ? "true" : "false");
                *p_Dest = *val;
                break;
            }
            case DBUS_TYPE_VARIANT:
            {
                printf ("variant \n");
                break;
            }
            case DBUS_TYPE_ARRAY:
            {
                printf("array [\n");
                break;
            }
            case DBUS_TYPE_STRUCT:
            {
                printf("struct {\n");
                break;
            }
            default:
            {
                printf("Unknown ArgType %d\n", ArgType);
                rc = IAP2_CTL_ERROR;
                break;
            }
        }
    }

    return rc;
}

static void iAP2BTExtractDeviceProperties(Device1         *DevProp,
                                          int             ArgType,
                                          const char      *PropertyName,
                                          DBusMessageIter *ValueIter)
{
    S32 rc = IAP2_OK;

    switch(ArgType)
    {
        case DBUS_TYPE_STRING:
        {
            char *val;

            dbus_message_iter_get_basic(ValueIter, &val);

            rc = iAP2BTExtractProperty(&DevProp->Address, val, PropertyName, "Address", ArgType);
            if(rc != IAP2_OK)
            {
                rc = iAP2BTExtractProperty(&DevProp->Name, val, PropertyName, "Name", ArgType);
            }
            if(rc != IAP2_OK)
            {
                rc = iAP2BTExtractProperty(&DevProp->Alias, val, PropertyName, "Alias", ArgType);
            }
            if(rc != IAP2_OK)
            {
                rc = iAP2BTExtractProperty(&DevProp->Icon, val, PropertyName, "Icon", ArgType);
            }
            break;
        }
        case DBUS_TYPE_BOOLEAN:
        {
            dbus_bool_t val;

            dbus_message_iter_get_basic(ValueIter, &val);

            rc = iAP2BTExtractProperty(&DevProp->Paired, &val, PropertyName, "Paired", ArgType);
            if(rc != IAP2_OK)
            {
                rc = iAP2BTExtractProperty(&DevProp->Trusted, &val, PropertyName, "Trusted", ArgType);
            }
            if(rc != IAP2_OK)
            {
                rc = iAP2BTExtractProperty(&DevProp->Blocked, &val, PropertyName, "Blocked", ArgType);
            }
            if(rc != IAP2_OK)
            {
                rc = iAP2BTExtractProperty(&DevProp->LegacyPairing, &val, PropertyName, "LegacyPairing", ArgType);
            }
            if(rc != IAP2_OK)
            {
                rc = iAP2BTExtractProperty(&DevProp->Connected, &val, PropertyName, "Connected", ArgType);
            }
            break;
        }
        case DBUS_TYPE_UINT32:
        {
            dbus_uint32_t val;

            dbus_message_iter_get_basic(ValueIter, &val);

            rc = iAP2BTExtractProperty(&DevProp->Class, &val, PropertyName, "Class", ArgType);
            break;
        }
        case DBUS_TYPE_ARRAY:
        {
            DBusMessageIter subiter;

            if(strncmp(PropertyName, "UUIDs", strlen("UUIDs") ) == 0)
            {
                dbus_message_iter_recurse(ValueIter, &subiter);
                do
                {
                    int Type;

                    Type = dbus_message_iter_get_arg_type(&subiter);
                    if(Type == DBUS_TYPE_STRING)
                    {
                        DevProp->UUID_Count += 1;
                    }
                } while(dbus_message_iter_next(&subiter));

                dbus_message_iter_recurse(ValueIter, &subiter);
                DevProp->UUID = calloc(DevProp->UUID_Count, sizeof(char*) );
                DevProp->UUID_Count = 0;
                do
                {
                    int Type;

                    Type = dbus_message_iter_get_arg_type(&subiter);
                    if(Type == DBUS_TYPE_STRING)
                    {
                        char *val;

                        dbus_message_iter_get_basic(&subiter, &val);
                        DevProp->UUID[DevProp->UUID_Count] = strndup(val, strlen(val));

                        if(DevProp->UUID[DevProp->UUID_Count] == NULL)
                        {
                            printf("Error in Allocating memory for UUID's\n");
                            rc = IAP2_ERR_NO_MEM;
                        }
                        else
                        {
                            printf ("string \"");
                            printf ("%s\"\n", val);
                            /* For Reconnection - as of now using BT UUID, because Register profile for CarPlay UUID
                             * not successful, so the Apple device updates its properties (UUID) without CarPlay UUID
                             * after first connection.  So on reconnection CarPlay UUID will not be visible.
                             */
                            //if(strncmp(DevProp->UUID[DevProp->UUID_Count], APPLE_CPLAY_UUID, strlen(APPLE_CPLAY_UUID) ) == 0)
                            if(strncmp(DevProp->UUID[DevProp->UUID_Count], APPLE_IAP2_BT_UUID, strlen(APPLE_IAP2_BT_UUID) ) == 0)
                            {
                                DevProp->CarPlaySupported = TRUE;
                            }
                            DevProp->UUID_Count += 1;
                        }
                    }
                } while( (dbus_message_iter_next(&subiter)) && (rc == IAP2_OK) );

            }
            break;
        }
        default:
        {
            printf("Unknown ArgType %d\n", ArgType);
            break;
        }
    }
}

static void iAP2BTExtractAdapterProperties(Adapter1        *AdapterProp,
                                           int             ArgType,
                                           const char      *PropertyName,
                                           DBusMessageIter *ValueIter)
{
    S32 rc = IAP2_OK;

    switch(ArgType)
    {
        case DBUS_TYPE_STRING:
        {
            char *val;

            dbus_message_iter_get_basic(ValueIter, &val);

            rc = iAP2BTExtractProperty(&AdapterProp->Address, val, PropertyName, "Address", ArgType);
            if(rc != IAP2_OK)
            {
                rc = iAP2BTExtractProperty(&AdapterProp->Name, val, PropertyName, "Name", ArgType);
            }
            if(rc != IAP2_OK)
            {
                rc = iAP2BTExtractProperty(&AdapterProp->Alias, val, PropertyName, "Alias", ArgType);
            }
            break;
        }
        case DBUS_TYPE_BOOLEAN:
        {
            dbus_bool_t val;

            dbus_message_iter_get_basic(ValueIter, &val);

            rc = iAP2BTExtractProperty(&AdapterProp->Powered, &val, PropertyName, "Powered", ArgType);
            if(rc != IAP2_OK)
            {
                rc = iAP2BTExtractProperty(&AdapterProp->Discoverable, &val, PropertyName, "Discoverable", ArgType);
            }
            if(rc != IAP2_OK)
            {
                rc = iAP2BTExtractProperty(&AdapterProp->Pairable, &val, PropertyName, "Pairable", ArgType);
            }
            if(rc != IAP2_OK)
            {
                rc = iAP2BTExtractProperty(&AdapterProp->Discovering, &val, PropertyName, "Discovering", ArgType);
            }
            break;
        }
        case DBUS_TYPE_UINT32:
        {
            dbus_uint32_t val;

            dbus_message_iter_get_basic(ValueIter, &val);

            rc = iAP2BTExtractProperty(&AdapterProp->Class, &val, PropertyName, "Class", ArgType);
            if(rc != IAP2_OK)
            {
                rc = iAP2BTExtractProperty(&AdapterProp->DiscoverableTimeout, &val, PropertyName, "DiscoverableTimeout", ArgType);
            }
            if(rc != IAP2_OK)
            {
                rc = iAP2BTExtractProperty(&AdapterProp->PairableTimeout, &val, PropertyName, "PairableTimeout", ArgType);
            }
            break;
        }
        case DBUS_TYPE_ARRAY:
        {
            DBusMessageIter subiter;

            if(strncmp(PropertyName, "UUIDs", strlen("UUIDs") ) == 0)
            {
                dbus_message_iter_recurse(ValueIter, &subiter);
                do
                {
                    int Type;

                    Type = dbus_message_iter_get_arg_type(&subiter);
                    if(Type == DBUS_TYPE_STRING)
                    {
                        AdapterProp->UUID_Count += 1;
                    }
                } while(dbus_message_iter_next(&subiter));

                dbus_message_iter_recurse(ValueIter, &subiter);
                AdapterProp->UUID = calloc(AdapterProp->UUID_Count, sizeof(char*) );
                AdapterProp->UUID_Count = 0;
                do
                {
                    int Type;

                    Type = dbus_message_iter_get_arg_type(&subiter);
                    if(Type == DBUS_TYPE_STRING)
                    {
                        char *val;

                        dbus_message_iter_get_basic(&subiter, &val);
                        AdapterProp->UUID[AdapterProp->UUID_Count] = strndup(val, strlen(val));

                        if(AdapterProp->UUID[AdapterProp->UUID_Count] == NULL)
                        {
                            printf("Error in Allocating memory for UUID's\n");
                            rc = IAP2_ERR_NO_MEM;
                        }
                        else
                        {
                            printf ("string \"");
                            printf ("%s\"\n", val);
                            AdapterProp->UUID_Count += 1;
                        }
                    }
                } while( (dbus_message_iter_next(&subiter)) && (rc == IAP2_OK) );

            }
            break;
        }
        default:
        {
            printf("Unknown ArgType %d\n", ArgType);
            break;
        }
    }
}

static void iAP2BTParseProperties(iAP2BTInit *iAP2BT, const char *ObjectPath, const char *InterfaceName, DBusMessageIter *PropertyIter)
{
    S32 rc = IAP2_OK;
    DBusMessageIter dbusMsg_DictIter;

    if(dbus_message_iter_get_arg_type(PropertyIter) != DBUS_TYPE_ARRAY)
    {
        rc = IAP2_CTL_ERROR;
        printf("ERROR: Argument is not of type Array \n");
    }

    /* Iterate through the Array */
    if(rc == IAP2_OK)
    {
        /* Iterate through 'n' Dictionary Entry */
        dbus_message_iter_recurse(PropertyIter, &dbusMsg_DictIter);

        /* Interfaces like "org.freedesktop.DBus.Introspectable", "org.bluez.AgentManager1", etc., will be ignored which does contain any properties */
        while(dbus_message_iter_get_arg_type(&dbusMsg_DictIter) == DBUS_TYPE_DICT_ENTRY)
        {
            DBusMessageIter entry;
            const char *PropertyName;

            dbus_message_iter_recurse(&dbusMsg_DictIter, &entry);

            if(dbus_message_iter_get_arg_type(&entry) != DBUS_TYPE_STRING)
            {
                rc = IAP2_CTL_ERROR;
                printf("ERROR: Argument is not of type String \n");
                break;
            }

            /* Identify the Property Name */
            dbus_message_iter_get_basic(&entry, &PropertyName);
            dbus_message_iter_next(&entry);

            if( strncmp(InterfaceName, "org.bluez.Adapter1", strlen("org.bluez.Adapter1") ) == 0 )
            {
                printf("\tPropertyName = %s\t", PropertyName);
                if (dbus_message_iter_get_arg_type(&entry) != DBUS_TYPE_VARIANT)
                {
                    rc = IAP2_CTL_ERROR;
                    printf("ERROR: Argument is not of type Variant \n");
                }

                if(rc == IAP2_OK)
                {
                    DBusMessageIter ValueIter;
                    int ArgType;

                    dbus_message_iter_recurse(&entry, &ValueIter);
                    ArgType = dbus_message_iter_get_arg_type(&ValueIter);

                    /* Extract the Adapter Property Values */
                    iAP2BTExtractAdapterProperties(&iAP2BT->AdapterProperties, ArgType, PropertyName, &ValueIter);
                }
            }
            else if( strncmp(InterfaceName, "org.bluez.Device1", strlen("org.bluez.Device1") ) == 0 )
            {
                printf("\tPropertyName = %s\t", PropertyName);
                if (dbus_message_iter_get_arg_type(&entry) != DBUS_TYPE_VARIANT)
                {
                    rc = IAP2_CTL_ERROR;
                    printf("ERROR: Argument is not of type Variant \n");
                }
                if( (rc == IAP2_OK) &&
                    (iAP2BT->DeviceProp_count < 10) )
                {
                    DBusMessageIter ValueIter;
                    int ArgType;

                    dbus_message_iter_recurse(&entry, &ValueIter);
                    ArgType = dbus_message_iter_get_arg_type(&ValueIter);

                    /* Extract the Device Property Values */
                    iAP2BTExtractDeviceProperties(&iAP2BT->DeviceProperties[iAP2BT->DeviceProp_count], ArgType, PropertyName, &ValueIter);
                }
            }
            else
            {
                printf("\tPropertyName = %s\n", PropertyName);
            }

            dbus_message_iter_next(&dbusMsg_DictIter);
        }
        if( strncmp(InterfaceName, "org.bluez.Device1", strlen("org.bluez.Device1") ) == 0 )
        {
            iAP2BT->DeviceProperties[iAP2BT->DeviceProp_count].ObjectPath = strdup(ObjectPath);
            if(iAP2BT->DeviceProperties[iAP2BT->DeviceProp_count].ObjectPath == NULL)
            {
                printf("Error in Allocating memory for ObjectPath \n");
            }
            iAP2BT->DeviceProp_count += 1;
        }
    }
}


void iAP2BTParseInterfaces(iAP2BTInit *iAP2BT, const char *ObjectPath, DBusMessageIter *InterfaceIter)
{
    S32 rc = IAP2_OK;
    DBusMessageIter dbusMsg_DictIter;

    if(dbus_message_iter_get_arg_type(InterfaceIter) != DBUS_TYPE_ARRAY)
    {
        rc = IAP2_CTL_ERROR;
        printf("ERROR: Argument is not of type Array \n");
    }

    /* Iterate through the Array */
    if(rc == IAP2_OK)
    {
        /* Iterate through 'n' Dictionary Entry */
        dbus_message_iter_recurse(InterfaceIter, &dbusMsg_DictIter);

        while(dbus_message_iter_get_arg_type(&dbusMsg_DictIter) == DBUS_TYPE_DICT_ENTRY)
        {
            DBusMessageIter entry;
            const char *InterfaceName;

            dbus_message_iter_recurse(&dbusMsg_DictIter, &entry);

            if(dbus_message_iter_get_arg_type(&entry) != DBUS_TYPE_STRING)
            {
                rc = IAP2_CTL_ERROR;
                printf("ERROR: Argument is not of type String \n");
                break;
            }

            /* Identify Interface Name */
            dbus_message_iter_get_basic(&entry, &InterfaceName);
            printf("InterfaceName = %s\n", InterfaceName);
            dbus_message_iter_next(&entry);

            /* Parse Interface Properties */
            iAP2BTParseProperties(iAP2BT, ObjectPath, InterfaceName, &entry);

            dbus_message_iter_next(&dbusMsg_DictIter);
        }
    }
}

static S32 iAP2BT_Parse_GetManagedObjects(DBusMessage *dbusMsg, iAP2BTInit *iAP2BT)
{
    S32 rc = IAP2_OK;
    DBusMessageIter dbusMsg_Iter, dbusMsg_DictIter;

    if(dbus_message_iter_init(dbusMsg, &dbusMsg_Iter) == FALSE)
    {
        rc = IAP2_CTL_ERROR;
        printf("ERROR: while performing dbus_message_iter_init() \n");
    }

    if(rc == IAP2_OK)
    {
        /* Iterate through 'n' Dictionary Entry */
        dbus_message_iter_recurse(&dbusMsg_Iter, &dbusMsg_DictIter);

        while(dbus_message_iter_get_arg_type(&dbusMsg_DictIter) == DBUS_TYPE_DICT_ENTRY)
        {
            DBusMessageIter entry;
            const char *ObjectPath;

            dbus_message_iter_recurse(&dbusMsg_DictIter, &entry);

            if(dbus_message_iter_get_arg_type(&entry) != DBUS_TYPE_OBJECT_PATH)
            {
                rc = IAP2_CTL_ERROR;
                printf("ERROR: ObjectPath not present\n");
                break;
            }

            /* Get the ObjectPath */
            dbus_message_iter_get_basic(&entry, &ObjectPath);
            printf("ObjectPath = %s\n", ObjectPath);
            dbus_message_iter_next(&entry);

            /* Parse Interfaces */
            iAP2BTParseInterfaces(iAP2BT, ObjectPath, &entry);

            dbus_message_iter_next(&dbusMsg_DictIter);
        }
    }

    return rc;
}

static void iAP2BT_StartDiscoveryReplyCallback(DBusMessage *dbusMsg, void *user_data, DBusError *dbusError)
{
    (void)user_data;

    if (!dbusMsg)
    {
        printf("ERROR: Failed to StartDiscovery\n");
        if (dbus_error_is_set(dbusError))
        {
            printf("%s\n", dbusError->message);
            dbus_error_free(dbusError);
        }
    }
    else
    {
        printf("StartDiscovery Success\n");
    }
}

static void iAP2BT_SetAdapterPropertiesReplyCallback(DBusMessage *dbusMsg, void *user_data, DBusError *dbusError)
{
    (void)user_data;

    if (!dbusMsg)
    {
        printf("ERROR: Failed to Set Adapter Properties\n");
        if (dbus_error_is_set(dbusError))
        {
            printf("%s\n", dbusError->message);
            dbus_error_free(dbusError);
        }
    }
    else
    {
        printf("Set Adapter Properties Success\n");
    }
}

S32 iAP2BT_SetBooleanProperties(DBusMessageIter *iter, const char *PropertyName, dbus_bool_t *PropertyValue)
{
    S32 rc = IAP2_OK;
    dbus_bool_t ret;
    DBusMessageIter subIter;

    ret = dbus_message_iter_append_basic(iter, DBUS_TYPE_STRING, &PropertyName);
    if(ret == FALSE)
    {
        printf("ERROR: While appending Property name %s\n", PropertyName);
        rc = IAP2_CTL_ERROR;
    }

    if(rc == IAP2_OK)
    {
        ret = dbus_message_iter_open_container(iter, DBUS_TYPE_VARIANT, DBUS_TYPE_BOOLEAN_AS_STRING, &subIter);
        if(ret == FALSE)
        {
            printf("ERROR: In dbus_message_iter_open_container() \n");
            rc = IAP2_CTL_ERROR;
        }
    }

    if(rc == IAP2_OK)
    {
        ret = dbus_message_iter_append_basic(&subIter, DBUS_TYPE_BOOLEAN, PropertyValue);
        if(ret == FALSE)
        {
            printf("ERROR: While trying to append Property value \n");
            rc = IAP2_CTL_ERROR;
        }
    }

    if(rc == IAP2_OK)
    {
        ret = dbus_message_iter_close_container(iter, &subIter);
        if(ret == FALSE)
        {
            printf("ERROR: In dbus_message_iter_close_container() \n");
            rc = IAP2_CTL_ERROR;
        }
    }

    return rc;
}

static void iAP2BT_SetAdapterPropertiesSetupCallback(DBusMessageIter *iter, void *user_data)
{
    S32 rc = IAP2_OK;
    iAP2BTInit *iAP2BT = (iAP2BTInit*)user_data;
    char *AdapterInterfaceName = ADAPTER1_INTERFACE;
    dbus_bool_t ret;

    ret = dbus_message_iter_append_basic(iter, DBUS_TYPE_STRING, &AdapterInterfaceName);
    if(ret == FALSE)
    {
        printf("ERROR: While appending Adapter Interface Name %s\n", AdapterInterfaceName);
        rc = IAP2_CTL_ERROR;
    }
    /* Set Powered - TRUE */
    if(iAP2BT->AdapterProperties.Powered == FALSE)
    {
        dbus_bool_t Powered = TRUE;

        rc = iAP2BT_SetBooleanProperties(iter, "Powered", &Powered);
    }
    /* Set Pairable - TRUE */
    if( (iAP2BT->AdapterProperties.Pairable == FALSE) && (rc == IAP2_OK) )
    {
        dbus_bool_t Pairable = TRUE;

        rc = iAP2BT_SetBooleanProperties(iter, "Pairable", &Pairable);
    }
    /* Set Discoverable - TRUE */
    if( (iAP2BT->AdapterProperties.Discoverable == FALSE) && (rc == IAP2_OK) )
    {
        dbus_bool_t Discoverable = TRUE;

        rc = iAP2BT_SetBooleanProperties(iter, "Discoverable", &Discoverable);
    }
}

static void iAP2BT_GetManagedObjectsCallback(DBusMessage *dbusMsg, void *user_data, DBusError *dbusError)
{
    iAP2BTInit *iAP2BT = (iAP2BTInit*)user_data;

    if (!dbusMsg)
    {
        printf("ERROR: Failed to receive GetManagedObjects\n");
        if (dbus_error_is_set(dbusError))
        {
            printf("%s\n", dbusError->message);
            dbus_error_free(dbusError);
        }
    }
    else
    {
        iAP2BT_Parse_GetManagedObjects(dbusMsg, iAP2BT);
    }
}

S32 iAP2BT_GetManagedObjects(iAP2BTInit *iAP2BT)
{
    S32 rc = IAP2_OK;
    iAP2BTMethodCall BTMethodCall = {
            .InterfaceName  = (U8*)OBJECT_MANAGER_INTERFACE,
            .MethodName     = (U8*)OBJECT_MANAGER_METHOD_GET_MANAGED_OBJECTS,
            .ObjectPath     = (U8*)"/",
            .ReplyCallback  = iAP2BT_GetManagedObjectsCallback,
            .SetupCallback  = NULL,
            .UserData       = iAP2BT
        };

    iAP2BT_dbusMethodCall(BTMethodCall, iAP2BT->dbusConn);

    return rc;
}

S32 iAP2BT_SetAdapterProperties(iAP2BTInit *iAP2BT)
{
    S32 rc = IAP2_OK;
    iAP2BTMethodCall BTMethodCall = {
            .InterfaceName  = (U8*)DBUS_PROPERTIES_INTERFACE,
            .MethodName     = (U8*)DBUS_PROPERTIES_METHOD_SET,
            .ObjectPath     = (U8*)"/org/bluez/hci0",
            .ReplyCallback  = iAP2BT_SetAdapterPropertiesReplyCallback,
            .SetupCallback  = iAP2BT_SetAdapterPropertiesSetupCallback,
            .UserData       = iAP2BT
        };

    iAP2BT_dbusMethodCall(BTMethodCall, iAP2BT->dbusConn);

    return rc;
}

S32 iAP2BT_StartDiscovery(DBusConnection *dbusConn)
{
    S32 rc = IAP2_OK;
    iAP2BTMethodCall BTMethodCall = {
            .InterfaceName  = (U8*)ADAPTER1_INTERFACE,
            .MethodName     = (U8*)ADAPTER1_METHOD_START_DISCOVERY,
            .ObjectPath     = (U8*)"/org/bluez/hci0",
            .ReplyCallback  = iAP2BT_StartDiscoveryReplyCallback,
            .SetupCallback  = NULL,
            .UserData       = NULL
        };

    iAP2BT_dbusMethodCall(BTMethodCall, dbusConn);

    return rc;
}
