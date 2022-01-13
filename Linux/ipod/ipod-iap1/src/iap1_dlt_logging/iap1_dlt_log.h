#ifndef IAP1_DLT_LOG_H
#define IAP1_DLT_LOG_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef IAP1_DLT_ENABLE
#include <dlt/dlt.h>
#include "ipod_dlt_log.h"
#endif



#define IPOD_IAPCONTROL_CB "IPCB"
#define IPOD_IAPCONTROL_GENERAL "GENL"
#define IPOD_IAPCONTROL_EXTENDED "EXTL"
#define IPOD_IAPCONTROL_DIGITAL "DIGL"
#define IPOD_IAPCONTROL_STORAGE "STOL"
#define IPOD_IAPCONTROL_DISPLAY "DISP"
#define IPOD_IAPCONTROL_IPODOUT "IOUT"
#define IPOD_IAPCONTROL_SIMPLE "SIMP"
#define IPOD_IDPS_FEATURE "IDPS"
#define IPOD_IAPCONTROL_LOCATION  "LOCL"
#define IPOD_IAPCONTROL_TRANSPORT "TRNL"
#define IPOD_AUTHENTICATION "AUTH"
#define IPOD_INITIALIZATION "INIT"
#define IPOD_BLUETOOTH_PLUGIN "BTPN"
#define IPOD_USB_PLUGIN "USPN"
#define IAP2_AUTH_DLT_CTXT "AUTN"

#define IPOD_IAPCONTROL_CB_DESC "iPod Control CB logging"
#define IPOD_IAPCONTROL_GENERAL_DESC "General Lingo Logging"
#define IPOD_IAPCONTROL_EXTENDED_DESC "EXtended Interface Lingo Logging"
#define IPOD_IAPCONTROL_DIGITAL_DESC "Digital Audio Lingo Logging"
#define IPOD_IAPCONTROL_STORAGE_DESC "Storage Lingo Logging"
#define IPOD_IAPCONTROL_DISPLAY_DESC "Display Remote Lingo Logging"
#define IPOD_IAPCONTROL_IPODOUT_DESC "iPod Out Lingo Logging"
#define IPOD_IAPCONTROL_SIMPLE_DESC "Simple Remote Lingo Logging"
#define IPOD_IDPS_FEATURE_DESC "IDPS Feature Logging"
#define IPOD_IAPCONTROL_LOCATION_DESC  "Location Lingo Logging"
#define IPOD_IAPCONTROL_TRANSPORT_DESC "Transport Logging"
#define IPOD_AUTHENTICATION_DESC "Authentication Logging"
#define IPOD_INITIALIZATION_DESC "Initialization Logging"
#define IPOD_BLUETOOTH_PLUGIN_DESC "Bluetooth Plugin logging"
#define IPOD_USB_PLUGIN_DESC "USB Plugin logging"
#define IAP2_AUTH_DLT_CTXT_DESC "Auth logging"


#ifdef IAP1_DLT_ENABLE
void iAP1RegisterAppWithDLT(const char * ApplnID, const char * ApplnDesc);
void iAP1RegisterCtxtWithDLT(void);
void iAP1DeRegisterAppWithDLT(void);
void iAP1DeRegisterCtxtWithDLT(void);

#define IAP1REGISTERAPPWITHDLT(ApplnID, ApplnDesc) iAP1RegisterAppWithDLT(ApplnID, ApplnDesc)
#define IAP1REGISTERCTXTWITHDLT() iAP1RegisterCtxtWithDLT()
#define IAP1DEREGISTERAPPWITHDLT() iAP1DeRegisterAppWithDLT()
#define IAP1DEREGISTERCTXTWITHDLT() iAP1DeRegisterCtxtWithDLT();

#define IAP1_CB_LOG(LogLevel, logString, ...)              iPodDltLog(&iAP1CBCtxt,             LogLevel, __FILE__, __LINE__, __FUNCTION__, logString, ##__VA_ARGS__)
#define IAP1_GENERAL_LOG(LogLevel, logString, ...)         iPodDltLog(&iAP1GeneralCtxt,        LogLevel, __FILE__, __LINE__, __FUNCTION__, logString, ##__VA_ARGS__)
#define IAP1_EXTENDED_LOG(LogLevel, logString, ...)        iPodDltLog(&iAP1ExtendedCtxt,       LogLevel, __FILE__, __LINE__, __FUNCTION__, logString, ##__VA_ARGS__)
#define IAP1_DIGITAL_LOG(LogLevel, logString, ...)         iPodDltLog(&iAP1DigitalCtxt,        LogLevel, __FILE__, __LINE__, __FUNCTION__, logString, ##__VA_ARGS__)
#define IAP1_STORAGE_LOG(LogLevel, logString, ...)         iPodDltLog(&iAP1StorageCtxt,        LogLevel, __FILE__, __LINE__, __FUNCTION__, logString, ##__VA_ARGS__)
#define IAP1_DISPLAY_LOG(LogLevel, logString, ...)         iPodDltLog(&iAP1DisplayCtxt,        LogLevel, __FILE__, __LINE__, __FUNCTION__, logString, ##__VA_ARGS__)
#define IAP1_IPODOUT_LOG(LogLevel, logString, ...)         iPodDltLog(&iAP1iPodoutCtxt,        LogLevel, __FILE__, __LINE__, __FUNCTION__, logString, ##__VA_ARGS__)
#define IAP1_SIMPLE_LOG(LogLevel, logString, ...)          iPodDltLog(&iAP1SimpleCtxt,         LogLevel, __FILE__, __LINE__, __FUNCTION__, logString, ##__VA_ARGS__)
#define IAP1_IDPSF_LOG(LogLevel, logString, ...)           iPodDltLog(&iAP1IdpsfCtxt,          LogLevel, __FILE__, __LINE__, __FUNCTION__, logString, ##__VA_ARGS__)
#define IAP1_LOCATION_LOG(LogLevel, logString, ...)        iPodDltLog(&iAP1LocationCtxt,       LogLevel, __FILE__, __LINE__, __FUNCTION__, logString, ##__VA_ARGS__)
#define IAP1_TRANSPORT_LOG(LogLevel, logString, ...)       iPodDltLog(&iAP1TransportCtxt,      LogLevel, __FILE__, __LINE__, __FUNCTION__, logString, ##__VA_ARGS__)
#define IAP1_AUTH_LOG(LogLevel, logString, ...)            iPodDltLog(&iAP1AuthCtxt,           LogLevel, __FILE__, __LINE__, __FUNCTION__, logString, ##__VA_ARGS__)
#define IAP1_INIT_LOG(LogLevel, logString, ...)            iPodDltLog(&iAP1InitCtxt,           LogLevel, __FILE__, __LINE__, __FUNCTION__, logString, ##__VA_ARGS__)
#define IAP1_BTP_LOG(LogLevel, logString, ...)             iPodDltLog(&iAP1BTPluginCtxt,       LogLevel, __FILE__, __LINE__, __FUNCTION__, logString, ##__VA_ARGS__)
#define IAP1_USBP_LOG(LogLevel, logString, ...)            iPodDltLog(&iAP1USBPluginCtxt,      LogLevel, __FILE__, __LINE__, __FUNCTION__, logString, ##__VA_ARGS__)
#define IAP2AUTHDLTLOG(LogLevel, logString, ...)            iPodDltLog(&iAP2AuthCtxt,      LogLevel, __FILE__, __LINE__, __FUNCTION__, logString, ##__VA_ARGS__)

/* Declare contexts */
extern DltContext iAP1CBCtxt;
extern DltContext iAP1GeneralCtxt;
extern DltContext iAP1ExtendedCtxt;
extern DltContext iAP1DigitalCtxt;
extern DltContext iAP1StorageCtxt;
extern DltContext iAP1DisplayCtxt;
extern DltContext iAP1iPodoutCtxt;
extern DltContext iAP1SimpleCtxt;
extern DltContext iAP1IdpsfCtxt;
extern DltContext iAP1LocationCtxt;
extern DltContext iAP1TransportCtxt;
extern DltContext iAP1AuthCtxt;
extern DltContext iAP1InitCtxt;
extern DltContext iAP1BTPluginCtxt;
extern DltContext iAP1USBPluginCtxt;
extern DltContext iAP2AuthCtxt;

#else
#define IAP1REGISTERAPPWITHDLT(ApplnID, ApplnDesc)                              (void)0
#define IAP1REGISTERCTXTWITHDLT()                                               (void)0
#define IAP1DEREGISTERAPPWITHDLT()                                              (void)0
#define IAP1DEREGISTERCTXTWITHDLT()                                             (void)0

#define IAP1_CB_LOG(LogLevel, logString, ...)                                  (void)0
#define IAP1_GENERAL_LOG(LogLevel, logString, ...)                             (void)0
#define IAP1_EXTENDED_LOG(LogLevel, logString, ...)                            (void)0
#define IAP1_DIGITAL_LOG(LogLevel, logString, ...)                             (void)0
#define IAP1_STORAGE_LOG(LogLevel, logString, ...)                             (void)0
#define IAP1_DISPLAY_LOG(LogLevel, logString, ...)                             (void)0
#define IAP1_IPODOUT_LOG(LogLevel, logString, ...)                             (void)0
#define IAP1_SIMPLE_LOG(LogLevel, logString, ...)                              (void)0
#define IAP1_IDPSF_LOG(LogLevel, logString, ...)                               (void)0
#define IAP1_LOCATION_LOG(LogLevel, logString, ...)                            (void)0
#define IAP1_TRANSPORT_LOG(LogLevel, logString, ...)                           (void)0
#define IAP1_AUTH_LOG(LogLevel, logString, ...)                                (void)0
#define IAP1_INIT_LOG(LogLevel, logString, ...)                                (void)0
#define IAP1_BTP_LOG(LogLevel, logString, ...)                                 (void)0
#define IAP1_USBP_LOG(LogLevel, logString, ...)                                (void)0
#define IAP2AUTHDLTLOG(LogLevel, logString, ...)                                (void)0
#endif

#ifdef __cplusplus
}
#endif

#endif /* IAP1_DLT_LOG_H */

