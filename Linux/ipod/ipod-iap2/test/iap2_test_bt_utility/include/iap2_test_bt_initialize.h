#ifndef IAP2_TEST_BT_INITIALIZE_H_
#define IAP2_TEST_BT_INITIALIZE_H_


/* **********************  includes  ********************** */

#include "iap2_test_bt_utility.h"

/* **********************  defines   ********************** */


/* **********************  variables ********************** */


/* **********************  functions ********************** */

S32 iAP2BTEstablishConnection(char *BTMacAddress);
S32 iAP2BTRegisterAgentInterface(iAP2BTInit *iAP2BT);
S32 iAP2BTRegisterMessageFilter(iAP2BTInit *iAP2BT);
S32 iAP2BTScanForDevices(iAP2BTInit *iAP2BT);
S32 iAP2InitializeBTConnection(iAP2BTInit *iAP2BT);

#endif /* IAP2_TEST_BT_INITIALIZE_H_ */
