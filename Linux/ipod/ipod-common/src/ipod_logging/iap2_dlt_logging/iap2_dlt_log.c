/*****************************************************************************
-  \file : iap2_dlt_log.c
-  \version : $Id: iap2_dlt_log.c, v Exp $
-  \release : $Name:$
-  Contains source code implementation of DLT log for sending trace messages
-  \component :
-  \author : Sudha.K \RBEI\ECF3 sudha.kuppusamy@in.bosch.com
-  \copyright (c) 2010 - 2013 Advanced Driver Information Technology.
-  This code is developed by Advanced Driver Information Technology.
-  Copyright of Advanced Driver Information Technology, Bosch, and DENSO.
-  All rights reserved.
*****************************************************************************/

#include "iap2_dlt_log.h"

#ifdef IAP2_DLT_ENABLE


DltContext iAP2InterfaceCtxt;
DltContext iAP2SessionCtxt;
DltContext iAP2LinkCtxt;
DltContext iAP2TransportCtxt;
DltContext iAP2USBRoleSwitchCtxt;
DltContext iAP2SmokeTestCtxt;
DltContext iAP2USBPluginCtxt;
DltContext iAP2USBGadgetCtxt;
DltContext iAP2FFSGadgetCtxt;
DltContext iAP2BTPluginCtxt;
DltContext iAP2OverCarPlayPluginCtxt;
DltContext iAP2AuthCtxt;

/*!
* \fn  iAP2RegisterWithDLT(void)
* \par INPUT PARAMETERS
* none
* \par REPLY PARAMETERS
* none
* \par DESCRIPTION
* This function registers iAP2 application and contexts with DLT daemon.
*/
void iAP2RegisterAppWithDLT(const char* ApplnID, const char* ApplnDesc)
{
    /* Register iAP2 application with DLT */
    DLT_REGISTER_APP(ApplnID, ApplnDesc);
}

void iAP2RegisterCtxtWithDLT(void)
{
    /* Register contexts */
    DLT_REGISTER_CONTEXT(iAP2InterfaceCtxt,         IAP2_INTF_DLT_CTXT,                 IAP2_INTF_DLT_CTXT_DESC);
    DLT_REGISTER_CONTEXT(iAP2SessionCtxt,           IAP2_SESSION_DLT_CTXT,              IAP2_SESSION_DLT_CTXT_DESC);
    DLT_REGISTER_CONTEXT(iAP2LinkCtxt,              IAP2_LINK_DLT_CTXT,                 IAP2_LINK_DLT_CTXT_DESC);
    DLT_REGISTER_CONTEXT(iAP2TransportCtxt,         IAP2_TRANSPORT_DLT_CTXT,            IAP2_TRANSPORT_DLT_CTXT_DESC);
    DLT_REGISTER_CONTEXT(iAP2USBRoleSwitchCtxt,     IAP2_USB_ROLE_SWITCH_DLT_CTXT,      IAP2_USB_ROLE_SWITCH_DLT_CTXT_DESC);
    DLT_REGISTER_CONTEXT(iAP2SmokeTestCtxt,         IAP2_TEST_DLT_CTXT,                 IAP2_TEST_DLT_CTXT_DESC);
    DLT_REGISTER_CONTEXT(iAP2USBPluginCtxt,         IAP2_USB_PLUGIN_DLT_CTXT,           IAP2_USB_PLUGIN_DLT_CTXT_DESC);
    DLT_REGISTER_CONTEXT(iAP2USBGadgetCtxt,         IAP2_USB_GADGET_DLT_CTXT,           IAP2_USB_GADGET_DLT_CTXT_DESC);
    DLT_REGISTER_CONTEXT(iAP2FFSGadgetCtxt,         IAP2_FFS_GADGET_DLT_CTXT,           IAP2_FFS_GADGET_DLT_CTXT_DESC);
    DLT_REGISTER_CONTEXT(iAP2BTPluginCtxt,          IAP2_BT_PLUGIN_DLT_CTXT,            IAP2_BT_PLUGIN_DLT_CTXT_DESC);
    DLT_REGISTER_CONTEXT(iAP2OverCarPlayPluginCtxt, IAP2_OVER_CARPLAY_PLUGIN_DLT_CTXT,  IAP2_OVER_CARPLAY_PLUGIN_DLT_CTXT_DESC);
    DLT_REGISTER_CONTEXT(iAP2AuthCtxt,              IAP2_AUTH_DLT_CTXT,                 IAP2_AUTH_DLT_CTXT_DESC);
}

/*!
* \fn  iAP2DeRegisterWithDLT(void)
* \par INPUT PARAMETERS
* none
* \par REPLY PARAMETERS
* none
* \par DESCRIPTION
* This function deregisters iAP2 application and other contexts with DLT Daemon.
*/
void iAP2DeRegisterCtxtWithDLT(void)
{
    /* Deregister contexts */
    DLT_UNREGISTER_CONTEXT(iAP2InterfaceCtxt);
    DLT_UNREGISTER_CONTEXT(iAP2SessionCtxt);
    DLT_UNREGISTER_CONTEXT(iAP2LinkCtxt);
    DLT_UNREGISTER_CONTEXT(iAP2TransportCtxt);
    DLT_UNREGISTER_CONTEXT(iAP2USBRoleSwitchCtxt);
    DLT_UNREGISTER_CONTEXT(iAP2SmokeTestCtxt);
    DLT_UNREGISTER_CONTEXT(iAP2USBPluginCtxt);
    DLT_UNREGISTER_CONTEXT(iAP2USBGadgetCtxt);
    DLT_UNREGISTER_CONTEXT(iAP2FFSGadgetCtxt);
    DLT_UNREGISTER_CONTEXT(iAP2BTPluginCtxt);
    DLT_UNREGISTER_CONTEXT(iAP2OverCarPlayPluginCtxt);
    DLT_UNREGISTER_CONTEXT(iAP2AuthCtxt);

}
void iAP2DeRegisterAppWithDLT(void)
{
    /* Deregister iAP2 Application */
    DLT_UNREGISTER_APP();
}


#endif
