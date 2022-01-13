#ifndef _IPODCORE_CFG_H_
#define _IPODCORE_CFG_H_

#include "adit_typedef.h"
#include "iPodPlayerUtilityConfiguration.h"


/*--------------------- Configuration defines ----------------------------- */

#define IPOD_PLAYER_CFG_ITEM_MAX                        32
#define IPOD_PLAYER_CFG_STR_MAX_SIZE                    100

#define IPOD_PLAYER_CFG_FILE_NAME                       "IPOD_PLAYER_CFG"
#define IPOD_PLAYER_CFG_ROOT_TAG                        "/ipodplayercfg"

#define IPOD_PLAYER_CFG_PLAYER_TMOUT                     "IPOD_PLAYER_TMOUT"
#define IPOD_PLAYER_CFG_DEVICE_MAX_NUM                   "IPOD_DEVICE_MAX_NUM"
#define IPOD_PLAYER_CFG_APP_MAX_NUM                      "IPOD_APP_MAX_NUM"
#define IPOD_PLAYER_CFG_DEFAULT_PROTOCOL                 "IPOD_DEFAULT_PROTOCOL"
#define IPOD_PLAYER_CFG_SUPPORT_IAP1                     "IPOD_SUPPORT_IAP1"
#define IPOD_PLAYER_CFG_SELF_AUTHENTICATE                "IPOD_SELF_AUTHENTICATE"
#define IPOD_PLAYER_CFG_MAX_ENTRY_NUM                    "IPOD_MAX_ENTRY_NUM"
#define IPOD_PLAYER_CFG_MAX_MSG_SIZE                     "IPOD_MAX_MSG_SIZE"
#define IPOD_PLAYER_CFG_MAX_QUEUE_SIZE                   "IPOD_MAX_QUEUE_SIZE"
#define IPOD_PLAYER_CFG_AUDIO_OUTPUT_NAME                "IPOD_AUDIO_OUTPUT_NAME"
#define IPOD_PLAYER_CFG_AUDIO_DEFAULT_SAMPLE             "IPOD_AUDIO_DEFAULT_SAMPLE"
#define IPOD_PLAYER_CFG_AUDIO_SERVER_NAME                "IPOD_AUDIO_SERVER_NAME"
#define IPOD_PLAYER_CFG_AUDIO_THREAD_SIZE                "IPOD_AUDIO_THREAD_SIZE"
#define IPOD_PLAYER_CFG_CTRL_READ_RETRIES                "IPOD_CTRL_READ_RETRIES"
#define IPOD_PLAYER_CFG_CTRL_THREAD_SIZE                 "IPOD_CTRL_THREAD_SIZE"
#define IPOD_PLAYER_CFG_CTRL_READER_PRIO                 "IPOD_CTRL_READER_PRIO"
#define IPOD_PLAYER_CFG_CTRL_WORKER_THREAD_SIZE          "IPOD_CTRL_WORKER_THREAD_SIZE"
#define IPOD_PLAYER_CFG_CTRL_WORKER_PRIO                 "IPOD_CTRL_WORKER_PRIO"
#define IPOD_PLAYER_CFG_CTRL_WORKER_FLAG_NAME            "IPOD_CTRL_WORKER_FLAG_NAME"
#define IPOD_PLAYER_CFG_CTRL_READER_FLAG_NAME            "IPOD_CTRL_READER_FLAG_NAME"
#define IPOD_PLAYER_CFG_CTRL_WAIT_TIMEOUT                "IPOD_CTRL_WAIT_TIMEOUT"
#define IPOD_PLAYER_CFG_TOKEN_ACCINFO_NAME               "IPOD_TOKEN_ACCINFO_NAME"
#define IPOD_PLAYER_CFG_TOKEN_ACCINFO_FWVER              "IPOD_TOKEN_ACCINFO_FWVER"
#define IPOD_PLAYER_CFG_TOKEN_ACCINFO_HWVER              "IPOD_TOKEN_ACCINFO_HWVER"
#define IPOD_PLAYER_CFG_TOKEN_ACCINFO_MANUFACTURE        "IPOD_TOKEN_ACCINFO_MANUFACTURE"
#define IPOD_PLAYER_CFG_TOKEN_ACCINFO_MODEL_NUM          "IPOD_TOKEN_ACCINFO_MODEL_NUM"
#define IPOD_PLAYER_CFG_TOKEN_ACCINFO_SERIAL_NUM         "IPOD_TOKEN_ACCINFO_SERIAL_NUM"
#define IPOD_PLAYER_CFG_DETECTION_SERVER_NAME            "IPOD_DETECTION_SERVER_NAME"
#define IPOD_PLAYER_CFG_AUTH_SERVER_NAME                 "IPOD_AUTH_SERVER_NAME"
#define IPOD_PLAYER_CFG_DATA_COM_SERVER_NAME             "IPOD_DATA_COM_SERVER_NAME"
#define IPOD_PLAYER_CFG_SYSTEM_SUPPORT_MASK              "IPOD_SYSTEM_SUPPORT_MASK"
#define IPOD_PLAYER_CFG_IOS_APP_ATTR_MASK                "IPOD_IOS_APP_ATTR_MASK"
#define IPOD_PLAYER_CFG_RF_CERTIFICATIONS                "IPOD_RF_CERTIFICATIONS"
#define IPOD_PLAYER_CFG_DEFAULT_VIDEO_SETTING_MASK       "IPOD_DEFAULT_VIDEO_SETTING_MASK"
#define IPOD_PLAYER_CFG_GPS_ASSIST_SUPPORT               "IPOD_GPS_ASSIST_SUPPORT"
#define IPOD_PLAYER_CFG_GPS_SATELITE_MAX_REFLESH         "IPOD_GPS_SATELITE_MAX_REFLESH"
#define IPOD_PLAYER_CFG_GPS_SATELITE_RECOMMNED_REFLESH   "IPOD_GPS_SATELITE_RECOMMNED_REFLESH"
#define IPOD_PLAYER_CFG_CORE_SERVER_NAME                 "IPOD_CORE_SERVER_NAME"
#define IPOD_PLAYER_CFG_TAG_MAJOR                        "IPOD_TAG_MAJOR"
#define IPOD_PLAYER_CFG_TAG_MINOR                        "IPOD_TAG_MINOR"
#define IPOD_PLAYER_CFG_TAG_MANID                        "IPOD_TAG_MANID"
#define IPOD_PLAYER_CFG_TAG_MANNAME                      "IPOD_TAG_MANNAME"
#define IPOD_PLAYER_CFG_TAG_DEVNAME                      "IPOD_TAG_DEVNAME"
#define IPOD_PLAYER_CFG_TAG_MARKED                       "IPOD_TAG_MARKED"
#define IPOD_PLAYER_CFG_IOSAPP_NAME                      "IPOD_IOSAPP_NAME"
#define IPOD_PLAYER_CFG_IOSAPP_URL                       "IPOD_IOSAPP_URL"
#define IPOD_PLAYER_CFG_IOSAPP_COUNT                     "IPOD_IOSAPP_COUNT"

#define IPOD_PLAYER_CFG_LOCATION_INFO_NMEA               "IPOD_LOCINFO_NMEA_TYPE"
#define IPOD_PLAYER_CFG_LOCATION_INFO_COUNT              "IPOD_LOCINFO_NMEA_TYPE_COUNT"
#define IPOD_PLAYER_CFG_VINFO_ENGINE_TYPE                "IPOD_VINFO_ENGINE_TYPE"
#define IPOD_PLAYER_CFG_VINFO_ENGINE_COUNT               "IPOD_VINFO_ENGINE_COUNT"
#define IPOD_PLAYER_CFG_VINFO_DISPLAY_NAME               "IPOD_VINFO_DISPLAY_NAME"
#define IPOD_PLAYER_CFG_VSTATUS_TYPE                     "IPOD_VSTATUS_TYPE"
#define IPOD_PLAYER_CFG_VSTATUS_TYPE_COUNT               "IPOD_VSTATUS_TYPE_COUNT"

#define IPOD_PLAYER_CFG_MAX_DATABASE_NUM                       "IPOD_IAP2_MAX_DATABASE_NUM"
#define IPOD_PLAYER_CFG_DATABASE_LOCATION_PREFIX               "IPOD_IAP2_DATABASE_LOCATION_PREFIX"
#define IPOD_PLAYER_CFG_ACC_CONFG_AVAILABLE_CURRENT            "IPOD_IAP2_ACC_CONFG_AVAILABLE_CURRENT"
#define IPOD_PLAYER_CFG_ACC_CONFG_BATTERY_SHOULD_CHARGE        "IPOD_IAP2_ACC_CONFG_BATTERY_SHOULD_CHARGE"
#define IPOD_PLAYER_CFG_ACC_CONFG_MAXIMUM_CURRENT_DRWAN_FROM_ACCESSORY    "IPOD_IAP2_ACC_CONFG_MAXIMUM_CURRENT_DRWAN_FROM_ACCESSORY"
#define IPOD_PLAYER_CFG_ACC_CONFIG_BATTERY_WILL_CHARGE         "IPOD_IAP2_ACC_CONFIG_BATTERY_WILL_CHARGE"
#define IPOD_PLAYER_CFG_ACC_CONFIG_POWER_MODE                  "IPOD_IAP2_ACC_CONFIG_POWER_MODE"
#define IPOD_PLAYER_CFG_ACC_CONFIG_FILE_XFER_STREAM            "IPOD_IAP2_ACC_CONFIG_FILE_XFER_STREAM"
#define IPOD_PLAYER_CFG_ACC_CONFIG_EAP_SUPPORT                 "IPOD_IAP2_ACC_CONFIG_EAP_SUPPORT"
#define IPOD_PLAYER_CFG_ACC_FILE_XFER_SUPPORT                  "IPOD_IAP2_ACC_FILE_XFER_SUPPORT"
#define IPOD_PLAYER_CFG_ACC_CONFG_TRANS_USB_OTG_GPIO_POWER_AI  "IPOD_IAP2_ACC_CONFG_TRANS_USB_OTG_GPIO_POWER_AI"
#define IPOD_PLAYER_CFG_ACC_CONFG_TRANS_USB_OTG_GPIO_POWER_SD  "IPOD_IAP2_ACC_CONFG_TRANS_USB_OTG_GPIO_POWER_SD"
#define IPOD_PLAYER_CFG_ACC_INFO_MAXIMUM_CURRENT_DRAWN_FROM_DEVICE    "IPOD_IAP2_ACC_INFO_MAXIMUM_CURRENT_DRAWN_FROM_DEVICE"
#define IPOD_PLAYER_CFG_ACC_INFO_PREFFERD_APP_BUNDLE           "IPOD_IAP2_ACC_INFO_PREFFERD_APP_BUNDLE"
#define IPOD_PLAYER_CFG_ACC_INFO_CURRENT_LANGUAGE              "IPOD_IAP2_ACC_INFO_CURRENT_LANGUAGE"
#define IPOD_PLAYER_CFG_ACC_INFO_SUPPORTED_LANGUAGE            "IPOD_IAP2_ACC_INFO_SUPPORTED_LANGUAGE"
#define IPOD_PLAYER_CFG_ACC_INFO_LANGUAGE_COUNT                "IPOD_IAP2_ACC_INFO_LANGUAGE_COUNT"

#define IPOD_PLAYER_CFG_ACC_INFO_SUPPORT_SAMPLE_RATE           "IPOD_IAP2_ACC_INFO_SUPPORT_SAMPLE_RATE"
#define IPOD_PLAYER_CFG_ACC_INFO_SAMPLE_RATE_COUNT             "IPOD_IAP2_ACC_INFO_SAMPLE_RATE_COUNT"

#define IPOD_PLAYER_CFG_ACC_INFO_BT_MAC                        "IPOD_IAP2_ACC_INFO_BT_MAC"
#define IPOD_PLAYER_CFG_ACC_INFO_BT_MAC_COUNT                  "IPOD_IAP2_ACC_INFO_BT_MAC_COUNT"
#define IPOD_PLAYER_CFG_ACC_INFO_VENDOR_ID                     "IPOD_IAP2_ACC_INFO_VENDOR_ID"
#define IPOD_PLAYER_CFG_ACC_INFO_PRODUCT_ID                    "IPOD_IAP2_ACC_INFO_PRODUCT_ID"
#define IPOD_PLAYER_CFG_ACC_INFO_BCD_DEVICE                    "IPOD_IAP2_ACC_INFO_BCD_DEVICE"
#define IPOD_PLAYER_CFG_ACC_INFO_PRODUCT_PLAN_UUID             "IPOD_IAP2_ACC_INFO_PRODUCT_PLAN_UUID"

#define IPOD_PLAYER_CFG_POWER_SUPPLY_CURRENT                   "IPOD_IAP2_POWER_SUPPLY_CURRENT"
#define IPOD_PLAYER_CFG_POWER_SUPPLY_CHARGE_BUTTERY            "IPOD_IAP2_POWER_SUPPLY_CHARGE_BUTTERY"

#define IPOD_PLAYER_CFG_ROUTE_GUID_COMP_ID                     "IPOD_IAP2_ROUTE_GUID_COMP_ID"
#define IPOD_PLAYER_CFG_ROUTE_GUID_COMP_ID_COUNT               "IPOD_IAP2_ROUTE_GUID_COMP_ID_COUNT"
#define IPOD_PLAYER_CFG_ROUTE_GUID_COMP_NAME                   "IPOD_IAP2_ROUTE_GUID_COMP_NAME"
#define IPOD_PLAYER_CFG_ROUTE_GUID_COMP_NAME_COUNT             "IPOD_IAP2_ROUTE_GUID_COMP_NAME_COUNT"
#define IPOD_PLAYER_CFG_ROUTE_GUID_COMP_MAXCURRENTROADNAMELEN  "IPOD_IAP2_ROUTE_GUID_COMP_MAX_CURRENT_ROADNAME_LEN"
#define IPOD_PLAYER_CFG_ROUTE_GUID_COMP_MAXCURRENTROADNAMELEN_COUNT   "IPOD_IAP2_ROUTE_GUID_COMP_MAX_CURRENT_ROADNAME_LEN_COUNT"
#define IPOD_PLAYER_CFG_ROUTE_GUID_COMP_MAXDESTNAMELEN         "IPOD_IAP2_ROUTE_GUID_COMP_MAX_DESTINATION_NAME_LEN"
#define IPOD_PLAYER_CFG_ROUTE_GUID_COMP_MAXDESTNAMELEN_COUNT   "IPOD_IAP2_ROUTE_GUID_COMP_MAX_DESTINATION_NAME_LEN_COUNT"
#define IPOD_PLAYER_CFG_ROUTE_GUID_COMP_MAXAFTERMANEROADNAMELEN "IPOD_IAP2_ROUTE_GUID_COMP_MAX_AFTER_MANEUVER_ROAD_NAME_LEN"
#define IPOD_PLAYER_CFG_ROUTE_GUID_COMP_MAXAFTERMANEROADNAMELEN_COUNT  "IPOD_IAP2_ROUTE_GUID_COMP_MAX_AFTER_MANEUVER_ROAD_NAME_LEN_COUNT"
#define IPOD_PLAYER_CFG_ROUTE_GUID_COMP_MAXMANEDESCKEN         "IPOD_IAP2_ROUTE_GUID_COMP_MAX_MANEUVER_DESCRIPTION_LEN"
#define IPOD_PLAYER_CFG_ROUTE_GUID_COMP_MAXMANEDESCKEN_COUNT   "IPOD_IAP2_ROUTE_GUID_COMP_MAX_MANEUVER_DESCRIPTION_LEN_COUNT"
#define IPOD_PLAYER_CFG_ROUTE_GUID_COMP_MAXGUIDMANECAP         "IPOD_IAP2_ROUTE_GUID_COMP_GUIDANCE_MANEUVER_CAPACITY"
#define IPOD_PLAYER_CFG_ROUTE_GUID_COMP_MAXGUIDMANECAP_COUNT   "IPOD_IAP2_ROUTE_GUID_COMP_GUIDANCE_MANEUVER_CAPACITY_COUNT"
#define IPOD_PLAYER_CFG_ROUTE_GUID_COMP_MAXLANEDESCRIPTLEN         "IPOD_IAP2_ROUTE_GUID_COMP_MAX_LANE_DESCRIPTION_LEN"
#define IPOD_PLAYER_CFG_ROUTE_GUID_COMP_MAXLANEDESCRIPTLEN_COUNT   "IPOD_IAP2_ROUTE_GUID_COMP_MAX_LANE_DESCRIPTION_LEN_COUNT"
#define IPOD_PLAYER_CFG_ROUTE_GUID_COMP_MAXLANESTORAGECAP          "IPOD_IAP2_ROUTE_GUID_COMP_MAX_LANE_STORAGE_CAPACITY"
#define IPOD_PLAYER_CFG_ROUTE_GUID_COMP_MAXLANESTORAGECAP_COUNT    "IPOD_IAP2_ROUTE_GUID_COMP_MAX_LANE_STORAGE_CAPACITY_COUNT"
#define IPOD_PLAYER_CFG_ROUTE_GUID_COMP_COUNT                  "IPOD_IAP2_ROUTE_GUID_COMP_COUNT"

/* player cfg item enum */
typedef enum
{
    IPOD_PLAYER_CFGNUM_PLAYER_TMOUT = 0,
    IPOD_PLAYER_CFGNUM_DEVICE_MAX_NUM,
    IPOD_PLAYER_CFGNUM_APP_MAX_NUM,
    IPOD_PLAYER_CFGNUM_DEFAULT_PROTOCOL,
    IPOD_PLAYER_CFGNUM_SUPPORT_IAP1,
    IPOD_PLAYER_CFGNUM_SELF_AUTHENTICATE,
    IPOD_PLAYER_CFGNUM_MAX_ENTRY_NUM,
    IPOD_PLAYER_CFGNUM_MAX_MSG_SIZE,
    IPOD_PLAYER_CFGNUM_MAX_QUEUE_SIZE,
    IPOD_PLAYER_CFGNUM_AUDIO_OUTPUT_NAME,
    IPOD_PLAYER_CFGNUM_AUDIO_DEFAULT_SAMPLE,
    IPOD_PLAYER_CFGNUM_AUDIO_SERVER_NAME,
    IPOD_PLAYER_CFGNUM_AUDIO_THREAD_SIZE,
    IPOD_PLAYER_CFGNUM_CTRL_READ_RETRIES,
    IPOD_PLAYER_CFGNUM_CTRL_THREAD_SIZE,
    IPOD_PLAYER_CFGNUM_CTRL_READER_PRIO,
    IPOD_PLAYER_CFGNUM_CTRL_WORKER_THREAD_SIZE,
    IPOD_PLAYER_CFGNUM_CTRL_WORKER_PRIO,
    IPOD_PLAYER_CFGNUM_CTRL_WORKER_FLAG_NAME,
    IPOD_PLAYER_CFGNUM_CTRL_READER_FLAG_NAME,
    IPOD_PLAYER_CFGNUM_CTRL_WAIT_TIMEOUT,
    IPOD_PLAYER_CFGNUM_TOKEN_ACCINFO_NAME,
    IPOD_PLAYER_CFGNUM_TOKEN_ACCINFO_FWVER,
    IPOD_PLAYER_CFGNUM_TOKEN_ACCINFO_HWVER,
    IPOD_PLAYER_CFGNUM_TOKEN_ACCINFO_MANUFACTURE,
    IPOD_PLAYER_CFGNUM_TOKEN_ACCINFO_MODEL_NUM,
    IPOD_PLAYER_CFGNUM_TOKEN_ACCINFO_SERIAL_NUM,
    IPOD_PLAYER_CFGNUM_DETECTION_SERVER_NAME,
    IPOD_PLAYER_CFGNUM_AUTH_SERVER_NAME,
    IPOD_PLAYER_CFGNUM_DATA_COM_SERVER_NAME,
    IPOD_PLAYER_CFGNUM_SYSTEM_SUPPORT_MASK,
    IPOD_PLAYER_CFGNUM_IOS_APP_ATTR_MASK,
    IPOD_PLAYER_CFGNUM_RF_CERTIFICATIONS,
    IPOD_PLAYER_CFGNUM_DEFAULT_VIDEO_SETTING_MASK,
    IPOD_PLAYER_CFGNUM_GPS_ASSIST_SUPPORT,
    IPOD_PLAYER_CFGNUM_GPS_SATELITE_MAX_REFLESH,
    IPOD_PLAYER_CFGNUM_GPS_SATELITE_RECOMMNED_REFLESH,
    IPOD_PLAYER_CFGNUM_CORE_SERVER_NAME,
    IPOD_PLAYER_CFGNUM_TAG_MAJOR,
    IPOD_PLAYER_CFGNUM_TAG_MINOR,
    IPOD_PLAYER_CFGNUM_TAG_MANID,
    IPOD_PLAYER_CFGNUM_TAG_MANNAME,
    IPOD_PLAYER_CFGNUM_TAG_DEVNAME,
    IPOD_PLAYER_CFGNUM_TAG_MARKED,
    IPOD_PLAYER_CFGNUM_IOSAPP_NAME,
    IPOD_PLAYER_CFGNUM_IOSAPP_URL,
    IPOD_PLAYER_CFGNUM_IOSAPP_COUNT,
    IPOD_PLAYER_CFGNUM_LOCATION_INFO_NMEA,
    IPOD_PLAYER_CFGNUM_LOCATION_INFO_COUNT,
    IPOD_PLAYER_CFGNUM_VINFO_ENGINE_TYPE,
    IPOD_PLAYER_CFGNUM_VINFO_ENGINE_COUNT,
    IPOD_PLAYER_CFGNUM_VINFO_DISPLAY_NAME,
    IPOD_PLAYER_CFGNUM_VSTATUS_TYPE,
    IPOD_PLAYER_CFGNUM_VSTATUS_TYPE_COUNT,
    IPOD_PLAYER_CFGNUM_MAX_DATABASE_NUM,
    IPOD_PLAYER_CFGNUM_DATABASE_LOCATION_PREFIX,
    IPOD_PLAYER_CFGNUM_ACC_CONFG_AVAILABLE_CURRENT,
    IPOD_PLAYER_CFGNUM_ACC_CONFG_BATTERY_SHOULD_CHARGE,
    IPOD_PLAYER_CFGNUM_MAXIMUM_CURRENT_DRWAN_FROM_ACCESSORY,
    IPOD_PLAYER_CFGNUM_ACC_CONFIG_BATTERY_WILL_CHARGE,
    IPOD_PLAYER_CFGNUM_ACC_CONFIG_POWER_MODE,
    IPOD_PLAYER_CFGNUM_ACC_CONFIG_FILE_XFER_STREAM,
    IPOD_PLAYER_CFGNUM_ACC_CONFIG_EAP_SUPPORT,
    IPOD_PLAYER_CFGNUM_ACC_FILE_XFER_SUPPORT,
    IPOD_PLAYER_CFGNUM_ACC_CONFG_TRANS_USB_OTG_GPIO_POWER_AI,
    IPOD_PLAYER_CFGNUM_ACC_CONFG_TRANS_USB_OTG_GPIO_POWER_SD,
    IPOD_PLAYER_CFGNUM_ACC_INFO_MAXIMUM_CURRENT_DRAWN_FROM_DEVICE,
    IPOD_PLAYER_CFGNUM_ACC_INFO_PREFFERD_APP_BUNDLE,
    IPOD_PLAYER_CFGNUM_ACC_INFO_CURRENT_LANGUAGE,
    IPOD_PLAYER_CFGNUM_ACC_INFO_SUPPORTED_LANGUAGE,
    IPOD_PLAYER_CFGNUM_ACC_INFO_LANGUAGE_COUNT,
    IPOD_PLAYER_CFGNUM_ACC_INFO_SUPPORT_SAMPLE_RATE,
    IPOD_PLAYER_CFGNUM_ACC_INFO_SAMPLE_RATE_COUNT,
    IPOD_PLAYER_CFGNUM_ACC_INFO_BT_MAC,
    IPOD_PLAYER_CFGNUM_ACC_INFO_BT_MAC_COUNT,
    IPOD_PLAYER_CFGNUM_ACC_INFO_VENDOR_ID,
    IPOD_PLAYER_CFGNUM_ACC_INFO_PRODUCT_ID,
    IPOD_PLAYER_CFGNUM_ACC_INFO_BCD_DEVICE,
    IPOD_PLAYER_CFGNUM_ACC_INFO_PRODUCT_PLAN_UUID,
    IPOD_PLAYER_CFGNUM_POWER_SUPPLY_CURRENT,
    IPOD_PLAYER_CFGNUM_POWER_SUPPLY_CHARGE_BUTTERY,
    IPOD_PLAYER_CFGNUM_ROUTE_GUID_COMP_ID,
    IPOD_PLAYER_CFGNUM_ROUTE_GUID_COMP_ID_COUNT,
    IPOD_PLAYER_CFGNUM_ROUTE_GUID_COMP_NAME,
    IPOD_PLAYER_CFGNUM_ROUTE_GUID_COMP_NAME_COUNT,
    IPOD_PLAYER_CFGNUM_ROUTE_GUID_COMP_MAX_CURRENTROADNAME_LEN,
    IPOD_PLAYER_CFGNUM_ROUTE_GUID_COMP_MAX_CURRENTROADNAME_COUNT,
    IPOD_PLAYER_CFGNUM_ROUTE_GUID_COMP_MAX_DESTROADNAME_LEN,
    IPOD_PLAYER_CFGNUM_ROUTE_GUID_COMP_MAX_DESTROADNAME_LEN_COUNT,
    IPOD_PLAYER_CFGNUM_ROUTE_GUID_COMP_MAX_AFTERMANEROADNAME_LEN,
    IPOD_PLAYER_CFGNUM_ROUTE_GUID_COMP_MAX_AFTERMANEROADNAME_LEN_COUNT,
    IPOD_PLAYER_CFGNUM_ROUTE_GUID_COMP_MAX_MANEDESCRIPT_LEN,
    IPOD_PLAYER_CFGNUM_ROUTE_GUID_COMP_MAX_MANEDESCRIPT_LEN_COUNT,
    IPOD_PLAYER_CFGNUM_ROUTE_GUID_COMP_MAX_GUIDMANESTORCAP,
    IPOD_PLAYER_CFGNUM_ROUTE_GUID_COMP_MAX_GUIDMANESTORCAP_COUNT,
    IPOD_PLAYER_CFGNUM_ROUTE_GUID_COMP_MAX_LANEDESCRIPT_LEN,
    IPOD_PLAYER_CFGNUM_ROUTE_GUID_COMP_MAX_LANEDESCRIPT_LEN_COUNT,
    IPOD_PLAYER_CFGNUM_ROUTE_GUID_COMP_MAX_LANESTORCAP,
    IPOD_PLAYER_CFGNUM_ROUTE_GUID_COMP_MAX_LANESTORCAP_COUNT,
    IPOD_PLAYER_CFGNUM_ROUTE_GUID_COMP_COUNT,
    IPOD_PLAYER_CFGNUM_MAX
} IPOD_PLAYER_CFGNUM;


typedef union
{
    S32 *intValue;
    IPOD_UTIL_CFG_STR *strValue;
} IPOD_PLAYER_CORE_CFG_VALUE;

typedef struct
{
    U8 *key;
    IPOD_UTIL_CFG_VALUE_TYPE type;
    U32 count;
    IPOD_PLAYER_CORE_CFG_VALUE value;
} IPOD_PLAYER_CORE_CFG_INFO;



S32 iPodCoreInitCfg(void);
void iPodCoreDeInitCfg(void);
S32 iPodCoreGetCfn(U32 cfgId);
S32 iPodCoreGetIndexCfn(U32 cfgId, U32 index);
S32 iPodCoreGetNumCfn(U32 cfgId, U32 count, S32 *value);
S32 iPodCoreGetCfs(U32 cfgId, U32 size, U8 *value);
S32 iPodCoreGetIndexCfs(U32 cfgId, U32 index, U32 size, U8 *value);
S32 iPodCoreGetNumCfs(U32 cfgId, U32 count, IPOD_UTIL_CFG_STR *value);
const IPOD_PLAYER_CORE_CFG_INFO *iPodCoreGetCfgs(void);

#endif /* _IPODCORE_CFG_H_ */