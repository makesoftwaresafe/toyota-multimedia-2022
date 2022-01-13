#ifndef IAP2_TEST_CONFIG_H_
#define IAP2_TEST_CONFIG_H_


/* **********************  includes  ********************** */

#include <iap2_init.h>

/* **********************  defines  ********************** */


#define IAP2_IOS_APP_NAME_DEMO                              "com.apple.p1"
#define IAP2_IOS_APP_NAME_DEMO_2                            "com.apple.p2"
#define IAP2_IOS_APP_NAME_MYSPIN                            "com.bosch.m"
#define IAP2_IOS_BUNDLE_ID                                  "com.yourcompany.EADemo"
//#define IAP2_IOS_BUNDLE_ID                                  "com.bosch.myspin"

#define IAP2_ACC_INFO_NAME                                  "AmazingProduct"
#define IAP2_ACC_INFO_MODEL_IDENTIFIER                      "15697"
#define IAP2_ACC_INFO_MANUFACTURER                          "ADIT"
#define IAP2_ACC_INFO_SERIAL_NUM                            "12345678"
#define IAP2_ACC_INFO_FW_VER                                "1"
#define IAP2_ACC_INFO_HW_VER                                "1"
#define IAP2_ACC_INFO_VENDOR_ID                             "44311"
#define IAP2_ACC_INFO_PRODUCT_ID                            "1111"
#define IAP2_ACC_INFO_BCD_DEVICE                            "1"
#define IAP2_ACC_INFO_EP_INIT                               "/dev/ffs/ep0"
#define IAP2_ACC_INFO_PPUUID                                "abcd1234"

#define IAP2_CURRENT_DRAW_FOR_DEVICE                        2100

#define IAP2_HID_COMP_IDENTIFIER                            0x0000

#define IAP2_BT_MAC_ADDRESS1                                0x0B5D00671100
#define IAP2_BT_MAC_ADDRESS2                                0x75db3f56e8b8
#define IAP2_BT_MAC_ADDRESS_CNT                             2
#define IAP2_BT_MY_CAR                                      "My Car"

#define IAP2_USE_CONFIGFS
#define IAP2_CONFIGURE_CONTROL_SESSION_VERSION_MANUALLY

/* **********************  variables  ********************** */



typedef struct
{
    /*! If set  iAP2iOSintheCar feature*/
    BOOL    iap2iOSintheCar;

    /*! If set to TRUE, EA native transport is required. */
    BOOL    iap2EANativeTransport;

    /*! If set, accessory will enable the configurations in link layer and control session. */
    BOOL    iap2EAPSupported;

    /*! GPIO power pin. Takes a string. */
    U8*     iap2UsbOtgGPIOPower;

    /*! Accessory transport type */
    iAP2TransportType_t      iAP2TransportType;

    /*! Accessory authentication  type */
    iAP2AuthenticationType_t iAP2AuthenticationType;

    /* number of supported iOS Apps */
    U32 SupportediOSAppCount;

    /* user to be used for app thread */
    uid_t app_thread_user;
    gid_t app_thread_prim_group;
    int app_thread_groups_cnt;
    gid_t *app_thread_groups;

    /* group to be used when mounting ffs */
    gid_t ffs_group;
    char* UdcDeviceName;
    U64 BTAdapterMAC;
} iap2UserConfig_t;


/* **********************  functions  ********************** */
S32 iap2ConfigureAccessory(iAP2AccessoryConfig_t* accConfig, iap2UserConfig_t iap2UserConfig);
void iap2ResetInitialParameter(iAP2InitParam_t* iAP2InitParam);
void iap2FreeAccInfo(iAP2AccessoryInfo_t *accInfo);
S32 iap2InitAccInfo(iAP2AccessoryInfo_t *accInfo,iap2UserConfig_t iap2UserConfig);
S32 iap2ConfigAcc(iAP2InitParam_t* iAP2InitParam,iap2UserConfig_t iap2UserConfig);

#endif /* IAP2_TEST_CONFIG_H_ */
