#ifndef IAP2_TEST_BT_AGENT_H_
#define IAP2_TEST_BT_AGENT_H_


/* **********************  includes  ********************** */

#include "iap2_test_bt_utility.h"

/* **********************  defines   ********************** */

#define AGENT_PATH                                 "/org/bluez/agent"
#define AGENT_INTERFACE                            "org.bluez.Agent1"
#define AGENT_MANAGER_INTERFACE                    "org.bluez.AgentManager1"

#define AGENTMANAGER_METHOD_REGISTER_AGENT         "RegisterAgent"
#define AGENTMANAGER_METHOD_UNREGISTER_AGENT       "UnregisterAgent"
#define AGENTMANAGER_METHOD_REQUEST_DEFAULT_AGENT  "RequestDefaultAgent"

#define AGENT1_METHOD_RELEASE                       "Release"
#define AGENT1_METHOD_REQUEST_PIN_CODE              "RequestPinCode"
#define AGENT1_METHOD_DISPLAY_PIN_CODE              "DisplayPinCode"
#define AGENT1_METHOD_REQUEST_PASS_KEY              "RequestPassKey"
#define AGENT1_METHOD_DISPLAY_PASS_KEY              "DisplayPassKey"
#define AGENT1_METHOD_REQUEST_CONFIRMATION          "RequestConfirmation"
#define AGENT1_METHOD_REQUEST_AUTHORIZATION         "RequestAuthorization"
#define AGENT1_METHOD_AUTHORIZE_SERVICE             "AuthorizeService"
#define AGENT1_METHOD_CANCEL                        "Cancel"

#define AGENT1_MAX_METHODS 9

/* **********************  variables ********************** */

typedef enum
{
    AGENT1_RELEASE,
    AGENT1_REQUEST_PIN_CODE,
    AGENT1_DISPLAY_PIN_CODE,
    AGENT1_REQUEST_PASS_KEY,
    AGENT1_DISPLAY_PASS_KEY,
    AGENT1_REQUEST_CONFIRMATION,
    AGENT1_REQUEST_AUTHORIZATION,
    AGENT1_AUTHORIZE_SERVICE,
    AGENT1_CANCEL
}AgentMethod;

typedef struct
{
    const char  *MethodName;
    AgentMethod Method;
}iAP2BT_Agent1Methods;

/* **********************  functions ********************** */

BOOL iAP2BT_RequestConfirmationStatus();
S32 iAP2BT_SetDefaultAgent(DBusConnection *dbusConn);
S32 iAP2BT_RegisterAgent(DBusConnection *dbusConn);

#endif /* IAP2_TEST_BT_AGENT_H_ */
