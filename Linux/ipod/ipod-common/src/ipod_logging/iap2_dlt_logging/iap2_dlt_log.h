/************************************************************************
 * \file: iap2_dlt_log.h
 *
 * \version: $ $
 *
 * This header file declares functions required for DLT registration and sending log messages to DLT daemon.
 *
 * \component: global definition file
 *
 * \author: Sudha.K /RBEI/ECF3/ sudha.kuppusamy@in.bosch.com
 *
 * \copyright: (c) 2010 - 2013 ADIT Corporation
 *
 ***********************************************************************/

#ifndef IAP2_DLT_LOG_H
#define IAP2_DLT_LOG_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef IAP2_DLT_ENABLE
#include <dlt/dlt.h>
#include "ipod_dlt_log.h"
#endif

#define IAP2_INTF_DLT_CTXT                  "INTF"
#define IAP2_SESSION_DLT_CTXT               "SESS"
#define IAP2_LINK_DLT_CTXT                  "LINK"
#define IAP2_TRANSPORT_DLT_CTXT             "TRNS"
#define IAP2_USB_ROLE_SWITCH_DLT_CTXT       "USBS"
#define IAP2_TEST_DLT_CTXT                  "TEST"
#define IAP2_USB_PLUGIN_DLT_CTXT            "USBP"
#define IAP2_USB_GADGET_DLT_CTXT            "USBG"
#define IAP2_FFS_GADGET_DLT_CTXT            "FFSG"
#define IAP2_BT_PLUGIN_DLT_CTXT             "BTPL"
#define IAP2_OVER_CARPLAY_PLUGIN_DLT_CTXT   "CPLA"
#define IAP2_AUTH_DLT_CTXT                  "AUTN"

#define IAP2_INTF_DLT_CTXT_DESC                 "Interface Layer logging"
#define IAP2_SESSION_DLT_CTXT_DESC              "Session Layer logging"
#define IAP2_LINK_DLT_CTXT_DESC                 "Link Layer logging"
#define IAP2_TRANSPORT_DLT_CTXT_DESC            "Transport Layer logging"
#define IAP2_USB_ROLE_SWITCH_DLT_CTXT_DESC      "USB Role Switch logging"
#define IAP2_TEST_DLT_CTXT_DESC                 "Smoke test logging"
#define IAP2_USB_PLUGIN_DLT_CTXT_DESC           "USB Plugin logging"
#define IAP2_USB_GADGET_DLT_CTXT_DESC           "USB Gadget logging"
#define IAP2_FFS_GADGET_DLT_CTXT_DESC           "FFS Gadget logging"
#define IAP2_BT_PLUGIN_DLT_CTXT_DESC            "BT Plugin logging"
#define IAP2_OVER_CARPLAY_PLUGIN_DLT_CTXT_DESC  "iAP2 Over Carplay Plugin logging"
#define IAP2_AUTH_DLT_CTXT_DESC                 "Auth logging"


#ifdef IAP2_DLT_ENABLE
void iAP2RegisterAppWithDLT(const char * ApplnID, const char * ApplnDesc);
void iAP2RegisterCtxtWithDLT(void);
void iAP2DeRegisterAppWithDLT(void);
void iAP2DeRegisterCtxtWithDLT(void);

#define IAP2REGISTERAPPWITHDLT(ApplnID, ApplnDesc) iAP2RegisterAppWithDLT(ApplnID, ApplnDesc)
#define IAP2REGISTERCTXTWITHDLT() iAP2RegisterCtxtWithDLT()
#define IAP2DEREGISTERAPPWITHDLT() iAP2DeRegisterAppWithDLT()
#define IAP2DEREGISTERCTXTWITHDLT() iAP2DeRegisterCtxtWithDLT();

#define IAP2INTERFACEDLTLOG(LogLevel, logString, ...)           iPodDltLog(&iAP2InterfaceCtxt,          LogLevel, __FILE__, __LINE__, __FUNCTION__, logString, ##__VA_ARGS__)
#define IAP2SESSIONDLTLOG(LogLevel, logString, ...)             iPodDltLog(&iAP2SessionCtxt,            LogLevel, __FILE__, __LINE__, __FUNCTION__, logString, ##__VA_ARGS__)
#define IAP2LINKDLTLOG(LogLevel, logString, ...)                iPodDltLog(&iAP2LinkCtxt,               LogLevel, __FILE__, __LINE__, __FUNCTION__, logString, ##__VA_ARGS__)
#define IAP2TRANSPORTDLTLOG(LogLevel, logString, ...)           iPodDltLog(&iAP2TransportCtxt,          LogLevel, __FILE__, __LINE__, __FUNCTION__, logString, ##__VA_ARGS__)
#define IAP2USBROLESWITCHDLTLOG(LogLevel, logString, ...)       iPodDltLog(&iAP2USBRoleSwitchCtxt,      LogLevel, __FILE__, __LINE__, __FUNCTION__, logString, ##__VA_ARGS__)
#define IAP2TESTDLTLOG(LogLevel, logString, ...)                iPodDltLog(&iAP2SmokeTestCtxt,          LogLevel, __FILE__, __LINE__, __FUNCTION__, logString, ##__VA_ARGS__)
#define IAP2USBPLUGINDLTLOG(LogLevel, logString, ...)           iPodDltLog(&iAP2USBPluginCtxt,          LogLevel, __FILE__, __LINE__, __FUNCTION__, logString, ##__VA_ARGS__)
#define IAP2USBGADGETDLTLOG(LogLevel, logString, ...)           iPodDltLog(&iAP2USBGadgetCtxt,          LogLevel, __FILE__, __LINE__, __FUNCTION__, logString, ##__VA_ARGS__)
#define IAP2FFSGADGETDLTLOG(LogLevel, logString, ...)           iPodDltLog(&iAP2FFSGadgetCtxt,          LogLevel, __FILE__, __LINE__, __FUNCTION__, logString, ##__VA_ARGS__)
#define IAP2BTPLUGINDLTLOG(LogLevel, logString, ...)            iPodDltLog(&iAP2BTPluginCtxt,           LogLevel, __FILE__, __LINE__, __FUNCTION__, logString, ##__VA_ARGS__)
#define IAP2OVERCARPLAYPLUGINDLTLOG(LogLevel, logString, ...)   iPodDltLog(&iAP2OverCarPlayPluginCtxt,  LogLevel, __FILE__, __LINE__, __FUNCTION__, logString, ##__VA_ARGS__)
#define IAP2AUTHDLTLOG(LogLevel, logString, ...)                iPodDltLog(&iAP2AuthCtxt,               LogLevel, __FILE__, __LINE__, __FUNCTION__, logString, ##__VA_ARGS__)
#define IAP2DLTCONVERTANDLOG(context, loglevel, logString, HexArray, ArrayLen) \
iPodDltConvertAndLog(context, loglevel, __FILE__, __LINE__,__FUNCTION__, logString, HexArray, ArrayLen)

/* Declare contexts */
extern DltContext iAP2InterfaceCtxt;
extern DltContext iAP2SessionCtxt;
extern DltContext iAP2LinkCtxt;
extern DltContext iAP2TransportCtxt;
extern DltContext iAP2USBRoleSwitchCtxt;
extern DltContext iAP2SmokeTestCtxt;
extern DltContext iAP2USBPluginCtxt;
extern DltContext iAP2USBGadgetCtxt;
extern DltContext iAP2FFSGadgetCtxt;
extern DltContext iAP2BTPluginCtxt;
extern DltContext iAP2OverCarPlayPluginCtxt;
extern DltContext iAP2AuthCtxt;

#else
#define IAP2REGISTERAPPWITHDLT(ApplnID, ApplnDesc)                              (void)0
#define IAP2REGISTERCTXTWITHDLT()                                               (void)0
#define IAP2DEREGISTERAPPWITHDLT()                                              (void)0
#define IAP2DEREGISTERCTXTWITHDLT()                                             (void)0

#define IAP2INTERFACEDLTLOG(LogLevel, logString, ...)                           (void)0
#define IAP2SESSIONDLTLOG(LogLevel, logString, ...)                             (void)0
#define IAP2LINKDLTLOG(LogLevel, logString, ...)                                (void)0
#define IAP2TRANSPORTDLTLOG(LogLevel, logString, ...)                           (void)0
#define IAP2USBROLESWITCHDLTLOG(LogLevel, logString, ...)                       (void)0
#define IAP2TESTDLTLOG(LogLevel, logString, ...)                                (void)0
#define IAP2USBPLUGINDLTLOG(LogLevel, logString, ...)                           (void)0
#define IAP2USBGADGETDLTLOG(LogLevel, logString, ...)                           (void)0
#define IAP2FFSGADGETDLTLOG(LogLevel, logString, ...)                           (void)0
#define IAP2BTPLUGINDLTLOG(LogLevel, logString, ...)                            (void)0
#define IAP2OVERCARPLAYPLUGINDLTLOG(LogLevel, logString, ...)                   (void)0
#define IAP2AUTHDLTLOG(LogLevel, logString, ...)                                (void)0
#define IAP2DLTCONVERTANDLOG(context, loglevel, logString, HexArray, ArrayLen)  (void)0
#endif

#ifdef __cplusplus
}
#endif

#endif /* IAP2_DLT_LOG_H */
