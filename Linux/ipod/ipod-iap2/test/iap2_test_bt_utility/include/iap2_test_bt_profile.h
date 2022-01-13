#ifndef IAP2_TEST_BT_PROFILE_H_
#define IAP2_TEST_BT_PROFILE_H_


/* **********************  includes  ********************** */

#include "iap2_test_bt_utility.h"

/* **********************  defines   ********************** */

#define PROFILE_PATH                                "/org/bluez/iapbt"

#define PROFILE_MANAGER_INTERFACE                   "org.bluez.ProfileManager1"
#define PROFILE1_INTERFACE                          "org.bluez.Profile1"

#define PROFILE_MANAGER_METHOD_REGISTER_PROFILE     "RegisterProfile"
#define PROFILE_MANAGER_METHOD_UNREGISTER_PROFILE   "UnregisterProfile"

#define PROFILE1_METHOD_RELEASE                     "Release"
#define PROFILE1_METHOD_NEW_CONNECTION              "NewConnection"
#define PROFILE1_METHOD_REQUEST_DISCONNECTION       "RequestDisconnection"

#define PROFILE1_MAX_METHODS 3

#define IAP_OVER_BT_UUID "00000000-deca-fade-deca-deafdecacafe"

#define IAP_OVER_BT_RECORD                                              \
        "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>                    \
        <record>                                                        \
                <attribute id=\"0x0001\">                               \
                        <sequence>                                      \
                                <uuid value=\"%s\" />                   \
                        </sequence>                                     \
                </attribute>                                            \
                <attribute id=\"0x0002\">                               \
                        <uint32 value=\"0x00000000\" />                 \
                </attribute>                                            \
                <attribute id=\"0x0004\">                               \
                        <sequence>                                      \
                                <sequence>                              \
                                        <uuid value=\"0x0100\" />       \
                                </sequence>                             \
                                <sequence>                              \
                                        <uuid value=\"0x0003\" />       \
                                        <uint8 value=\"0x%02x\" />      \
                                </sequence>                             \
                        </sequence>                                     \
                </attribute>                                            \
                <attribute id=\"0x0005\">                               \
                        <sequence>                                      \
                                <uuid value=\"0x1002\" />               \
                        </sequence>                                     \
                </attribute>                                            \
                <attribute id=\"0x0006\">                               \
                        <sequence>                                      \
                                <uint16 value=\"0x656e\" />             \
                                <uint16 value=\"0x006a\" />             \
                                <uint16 value=\"0x0100\" />             \
                                <uint16 value=\"0x6672\" />             \
                                <uint16 value=\"0x006a\" />             \
                                <uint16 value=\"0x0110\" />             \
                                <uint16 value=\"0x6465\" />             \
                                <uint16 value=\"0x006a\" />             \
                                <uint16 value=\"0x0120\" />             \
                                <uint16 value=\"0x6a61\" />             \
                                <uint16 value=\"0x006a\" />             \
                                <uint16 value=\"0x0130\" />             \
                        </sequence>                                     \
                </attribute>                                            \
                <attribute id=\"0x0008\">                               \
                        <uint8 value=\"0xff\" />                        \
                </attribute>                                            \
                <attribute id=\"0x0009\">                               \
                        <sequence>                                      \
                                <sequence>                              \
                                        <uuid value=\"0x1101\" />       \
                                        <uint16 value=\"0x0100\" />     \
                                </sequence>                             \
                        </sequence>                                     \
                </attribute>                                            \
                <attribute id=\"0x0100\">                               \
                        <text value=\"Wireless iAP\" />                 \
                </attribute>                                            \
        </record>"

/* **********************  variables ********************** */

typedef enum
{
    PROFILE1_RELEASE,
    PROFILE1_NEW_CONNECTION,
    PROFILE1_REQUEST_DISCONNECTION
}Profile1Method;

typedef struct
{
    const char      *MethodName;
    Profile1Method  Method;
}iAP2BT_Profile1Methods;

/* **********************  functions ********************** */

S32 iAP2BT_RegisterProfile(DBusConnection *dbusConn);

#endif /* IAP2_TEST_BT_PROFILE_H_ */
