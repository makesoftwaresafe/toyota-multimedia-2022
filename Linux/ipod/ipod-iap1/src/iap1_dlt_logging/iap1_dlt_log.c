#include "iap1_dlt_log.h"

#ifdef IAP1_DLT_ENABLE

DltContext iAP1CBCtxt;
DltContext iAP1GeneralCtxt;
DltContext iAP1ExtendedCtxt;
DltContext iAP1DigitalCtxt;
DltContext iAP1StorageCtxt;
DltContext iAP1DisplayCtxt;
DltContext iAP1iPodoutCtxt;
DltContext iAP1SimpleCtxt;
DltContext iAP1IdpsfCtxt;
DltContext iAP1LocationCtxt;
DltContext iAP1TransportCtxt;
DltContext iAP1AuthCtxt;
DltContext iAP1InitCtxt;
DltContext iAP1BTPluginCtxt;
DltContext iAP1USBPluginCtxt;
DltContext iAP2AuthCtxt;





/*!
* \fn  iAP1RegisterWithDLT(void)
* \par INPUT PARAMETERS
* none
* \par REPLY PARAMETERS
* none
* \par DESCRIPTION
* This function registers iAP1 application and contexts with DLT daemon.
*/
void iAP1RegisterAppWithDLT(const char* ApplnID, const char* ApplnDesc)
{
    /* Register iAP1 application with DLT */
    DLT_REGISTER_APP(ApplnID, ApplnDesc);
}

void iAP1RegisterCtxtWithDLT(void)
{
    /* Register contexts */
    DLT_REGISTER_CONTEXT(iAP1CBCtxt,            IPOD_IAPCONTROL_CB,             IPOD_IAPCONTROL_CB_DESC);
    DLT_REGISTER_CONTEXT(iAP1GeneralCtxt,       IPOD_IAPCONTROL_GENERAL,        IPOD_IAPCONTROL_GENERAL_DESC);
    DLT_REGISTER_CONTEXT(iAP1ExtendedCtxt,      IPOD_IAPCONTROL_EXTENDED,       IPOD_IAPCONTROL_EXTENDED_DESC);
    DLT_REGISTER_CONTEXT(iAP1DigitalCtxt,       IPOD_IAPCONTROL_DIGITAL,        IPOD_IAPCONTROL_DIGITAL_DESC);
    DLT_REGISTER_CONTEXT(iAP1StorageCtxt,       IPOD_IAPCONTROL_STORAGE,        IPOD_IAPCONTROL_STORAGE_DESC);
    DLT_REGISTER_CONTEXT(iAP1DisplayCtxt,       IPOD_IAPCONTROL_DISPLAY,        IPOD_IAPCONTROL_DISPLAY_DESC);
    DLT_REGISTER_CONTEXT(iAP1iPodoutCtxt,       IPOD_IAPCONTROL_IPODOUT,        IPOD_IAPCONTROL_IPODOUT_DESC);
    DLT_REGISTER_CONTEXT(iAP1SimpleCtxt,        IPOD_IAPCONTROL_SIMPLE,         IPOD_IAPCONTROL_SIMPLE_DESC);
    DLT_REGISTER_CONTEXT(iAP1IdpsfCtxt,         IPOD_IDPS_FEATURE,              IPOD_IDPS_FEATURE_DESC);
    DLT_REGISTER_CONTEXT(iAP1LocationCtxt,      IPOD_IAPCONTROL_LOCATION,       IPOD_IAPCONTROL_LOCATION_DESC);
    DLT_REGISTER_CONTEXT(iAP1TransportCtxt,     IPOD_IAPCONTROL_TRANSPORT,      IPOD_IAPCONTROL_TRANSPORT_DESC);
    DLT_REGISTER_CONTEXT(iAP1AuthCtxt,          IPOD_AUTHENTICATION,            IPOD_AUTHENTICATION_DESC);
    DLT_REGISTER_CONTEXT(iAP1InitCtxt,          IPOD_INITIALIZATION,            IPOD_INITIALIZATION_DESC);
    DLT_REGISTER_CONTEXT(iAP1BTPluginCtxt,      IPOD_BLUETOOTH_PLUGIN,          IPOD_BLUETOOTH_PLUGIN_DESC);
    DLT_REGISTER_CONTEXT(iAP1USBPluginCtxt,     IPOD_USB_PLUGIN,                IPOD_USB_PLUGIN_DESC);
    DLT_REGISTER_CONTEXT(iAP2AuthCtxt,     IAP2_AUTH_DLT_CTXT,       IAP2_AUTH_DLT_CTXT_DESC);
}

/*!
* \fn  iAP1DeRegisterWithDLT(void)
* \par INPUT PARAMETERS
* none
* \par REPLY PARAMETERS
* none
* \par DESCRIPTION
* This function deregisters iAP1 application and other contexts with DLT Daemon.
*/
void iAP1DeRegisterCtxtWithDLT(void)
{
    /* Deregister contexts */

    DLT_UNREGISTER_CONTEXT(iAP1CBCtxt);
    DLT_UNREGISTER_CONTEXT(iAP1GeneralCtxt);
    DLT_UNREGISTER_CONTEXT(iAP1ExtendedCtxt);
    DLT_UNREGISTER_CONTEXT(iAP1DigitalCtxt);
    DLT_UNREGISTER_CONTEXT(iAP1StorageCtxt);
    DLT_UNREGISTER_CONTEXT(iAP1DisplayCtxt);
    DLT_UNREGISTER_CONTEXT(iAP1iPodoutCtxt);
    DLT_UNREGISTER_CONTEXT(iAP1SimpleCtxt);
    DLT_UNREGISTER_CONTEXT(iAP1IdpsfCtxt);
    DLT_UNREGISTER_CONTEXT(iAP1LocationCtxt);
    DLT_UNREGISTER_CONTEXT(iAP1TransportCtxt);
    DLT_UNREGISTER_CONTEXT(iAP1AuthCtxt);
    DLT_UNREGISTER_CONTEXT(iAP1InitCtxt);
    DLT_UNREGISTER_CONTEXT(iAP1BTPluginCtxt);
    DLT_UNREGISTER_CONTEXT(iAP1USBPluginCtxt);
    DLT_UNREGISTER_CONTEXT(iAP2AuthCtxt);
}
void iAP1DeRegisterAppWithDLT(void)
{
    /* Deregister iAP1 Application */
    DLT_UNREGISTER_APP();
}


#endif

