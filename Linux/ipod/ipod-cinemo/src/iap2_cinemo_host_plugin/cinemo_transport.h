/*****************************************************************************
    Project:        Cinemo Media Engine

    Developed by:   Cinemo GmbH
                    Karlsruhe, Germany

    Copyright (c)   2009-2019 Cinemo GmbH and its licensors.
                    All rights reserved.

    This software is supplied only under the terms of a license agreement,
    nondisclosure agreement or other written agreement with Cinemo GmbH.
    Use, redistribution or other disclosure of any parts of this software
    is prohibited except in accordance with the terms of such written
    agreement with Cinemo GmbH. This software is confidential and proprietary
    information of Cinemo GmbH and its licensors.

    In case of licensing questions please contact: sales@cinemo.com
*****************************************************************************/

#ifndef CINEMO_TRANSPORT_H_INCLUDED
#define CINEMO_TRANSPORT_H_INCLUDED

#define CINEMO_TRANSPORT_LIBRARY_MINIMUM_REQUIRED_PROTOCOL_VERSION 4
#define CINEMO_TRANSPORT_LIBRARY_PROTOCOL_VERSION 9

#define CTLI_NO_ERROR               0
#define CTLI_ERROR_ARGUMENTS        -1
#define CTLI_ERROR_AUDIOPARAMS      -2
#define CTLI_ERROR_CANCEL           -3
#define CTLI_ERROR_DECODE           -4
#define CTLI_ERROR_IO               -5
#define CTLI_ERROR_NOTCONNECTED     -6
#define CTLI_ERROR_NOTENOUGHSPACE   -7
#define CTLI_ERROR_NOTIMPLEMENTED   -8
#define CTLI_ERROR_OPEN             -9
#define CTLI_ERROR_OVERFLOW         -10
#define CTLI_ERROR_RESOURCES        -11
#define CTLI_ERROR_AUDIOSETUP       -12
#define CTLI_ERROR_CONFIGURATION    -13
#define CTLI_ERROR_IDLE             -14
#define CTLI_ERROR_UNAVAILABLE      -15
#define CTLI_ERROR_DRIVER_FAILURE   -16

typedef enum CTLI_PARAM_PROTOCOL_VALUES {
    CTLI_PARAM_PROTOCOL_UNKNOWN = 0,
    CTLI_PARAM_PROTOCOL_USB,
    CTLI_PARAM_PROTOCOL_BLUETOOTH,
    CTLI_PARAM_PROTOCOL_WIFI,
} CTLI_PARAM_PROTOCOL_VALUES;

typedef enum CTLI_PARAM_STATE_VALUES {
    CTLI_PARAM_STATE_UNKNOWN = 0,       /* unknown state */
    CTLI_PARAM_STATE_DEVICEMODE,        /* device is connected in device mode */
    CTLI_PARAM_STATE_HOSTMODE,          /* device is connected in host mode */
    CTLI_PARAM_STATE_DISCONNECTED,      /* device has been disconnected */
} CTLI_PARAM_STATE_VALUES;

// for non-USB transports CTLI_PARAM_STATE_DEVICEMODE should
// be used if the connection is established

typedef enum CTLI_PARAM {
    CTLI_PARAM_PROTOCOL = 0,
    CTLI_PARAM_STATE,
    CTLI_PARAM_DEVICE_VID,
    CTLI_PARAM_DEVICE_PID,
    CTLI_PARAM_DEVICE_MANUFACTURER,
    CTLI_PARAM_DEVICE_PRODUCT,
    CTLI_PARAM_DEVICE_SERIALNO,
    CTLI_PARAM_NCM_IF_NUM,
    CTLI_PARAM_BLUETOOTH_MAC_ADDRESS,
    CTLI_PARAM_BLUETOOTH_ADAPTER_NAME,
    CTLI_PARAM_BLUETOOTH_DEVICE_NAME,
    CTLI_PARAM_BLUETOOTH_AVRCP_VERSION,
    CTLI_PARAM_BLUETOOTH_AVRCP_FEATURESFLAGS,
    CTLI_PARAM_BLUETOOTH_AVRCP_BROWSING_PSM,
    CTLI_PARAM_BLUETOOTH_AVRCP_COVERART_PSM,
    CTLI_PARAM_DEVICE_VERSION
} CTLI_PARAM;

/**
 * union to hold the parameter values for ctli_get_param_fn
 */
/* Changed by ADIT
 * Added typedef to avoid Build Failure
 */
typedef union ctli_params {
    CTLI_PARAM_PROTOCOL_VALUES protocol;    /**< transport protocol type */
    CTLI_PARAM_STATE_VALUES state;          /**< connection state */
    unsigned short device_vid;              /**< device vendor identifier */
    unsigned short device_pid;              /**< device product identifier */
    char device_manufacturer[255];          /**< device manufacturer string */
    char device_product[255];               /**< device product string */
    char device_serialno[255];              /**< device serialnumber string */
    unsigned char ncm_if_num;               /**< NCM interface number */
    char bt_mac_address[6];                 /**< MAC address of the bluetooth adapter */
    char bt_adapter_name[255];              /**< name of the bluetooth adapter */
    char bt_device_name[249];               /**< UTF-8 max 248 bytes */
    unsigned short bt_avrcp_version;        /**< AVRCP version number */
    unsigned short bt_avrcp_fflags;         /**< AVRCP feature flags */
    unsigned short bt_avrcp_browsing_psm;   /**< AVRCP browsing protocol service multiplexer */
    unsigned short bt_avrcp_coverart_psm;   /**< AVRCP coverart protocol service multiplexer */
    unsigned short device_version;          /**< device version number */
}ctli_params;

typedef enum CTLI_AUDIO_CONTAINER {
    CTLI_AUDIO_CONTAINER_UNSPECIFIED = 0,
    CTLI_AUDIO_CONTAINER_RAW,
    CTLI_AUDIO_CONTAINER_LATM,
    CTLI_AUDIO_CONTAINER_AVDTP,
    CTLI_AUDIO_CONTAINER_LATM_NO_SYNC
} CTLI_AUDIO_CONTAINER;

typedef enum CTLI_AUDIO_CODEC {
    CTLI_AUDIO_CODEC_UNSPECIFIED = 0,
    CTLI_AUDIO_CODEC_PCM,
    CTLI_AUDIO_CODEC_MP3,
    CTLI_AUDIO_CODEC_AAC,
    CTLI_AUDIO_CODEC_SBC,
    CTLI_AUDIO_CODEC_APTX,
    CTLI_AUDIO_CODEC_LDAC,
    CTLI_AUDIO_CODEC_APTXHD,
} CTLI_AUDIO_CODEC;

typedef enum CTLI_AUDIO_SAMPLETYPE {
    CTLI_AUDIO_SAMPLETYPE_UNSPECIFIED = 0,
    CTLI_AUDIO_SAMPLETYPE_S16LE,
    CTLI_AUDIO_SAMPLETYPE_S24LE,
} CTLI_AUDIO_SAMPLETYPE;

typedef struct ctli_audio_params {
    CTLI_AUDIO_CONTAINER container;
    CTLI_AUDIO_CODEC codec;
    CTLI_AUDIO_SAMPLETYPE sampletype;
    int samplerate;
    int channels;
    int bits;
} ctli_audio_params;

typedef enum CTLI_DATAPORT_TRANSITION {
    CTLI_DATAPORT_UNKNOWN = 0,
    CTLI_DATAPORT_OFFLINE_TO_ONLINE,
    CTLI_DATAPORT_ONLINE_TO_OFFLINE,
} CTLI_DATAPORT_TRANSITION;

typedef struct ctli_dataport_transit {
    unsigned short id;
    unsigned char state_changed;
    CTLI_DATAPORT_TRANSITION transition;
} ctli_dataport_transit;

typedef enum CTLI_DATAPORT_PARAM {
    CTLI_DATAPORT_PARAM_STATE = 0,
    CTLI_DATAPORT_PARAM_SEND_MTU,
    CTLI_DATAPORT_PARAM_RECV_MTU,
} CTLI_DATAPORT_PARAM;

typedef enum CTLI_DATAPORT_STATE {
    CTLI_DATAPORT_STATE_OFFLINE = 0,
    CTLI_DATAPORT_STATE_ONLINE,
} CTLI_DATAPORT_STATE;

/* Changed by ADIT
 * Added typedef to avoid Build Failure
 */
typedef union ctli_dataport_params {
    CTLI_DATAPORT_STATE state;
    unsigned int send_mtu;
    unsigned int recv_mtu;
}ctli_dataport_params;

typedef struct ctli_hid_report_descriptor {
    unsigned short length;
    const void* pdata;
} ctli_hid_report_descriptor;

typedef struct ctli_create_params {
    const char* szurl;
    const char* szoption;
    const char* szdataports;
    ctli_hid_report_descriptor* phid;
    unsigned int hid_count;
} ctli_create_params;

typedef void* ctli_handle;

typedef int (*ctli_delete_transport_fn)(ctli_handle tphandle);
typedef int (*ctli_receive_fn)(ctli_handle tphandle, void* pdest, unsigned int npdest);
typedef int (*ctli_send_fn)(ctli_handle tphandle, const void* psrc, unsigned int npsrc);
typedef int (*ctli_abort_fn)(ctli_handle tphandle);
typedef int (*ctli_get_param_fn)(ctli_handle tphandle, CTLI_PARAM param_type, ctli_params* params);
typedef int (*ctli_set_param_fn)(ctli_handle tphandle, CTLI_PARAM param_type, const ctli_params* params);
typedef int (*ctli_audio_create_fn)(ctli_handle tphandle);
typedef int (*ctli_audio_delete_fn)(ctli_handle tphandle);
typedef int (*ctli_audio_receive_fn)(ctli_handle tphandle, void* pdest, unsigned int npdest);
typedef int (*ctli_audio_abort_fn)(ctli_handle tphandle);
typedef int (*ctli_audio_get_params_fn)(ctli_handle tphandle, ctli_audio_params* params);
typedef int (*ctli_audio_set_params_fn)(ctli_handle tphandle, const ctli_audio_params* params);
typedef int (*ctli_dataport_select_fn)(ctli_handle tphandle, ctli_dataport_transit* transit, unsigned int num_entries);
typedef int (*ctli_dataport_send_fn)(ctli_handle tphandle, unsigned short id, const void* psrc, unsigned int npsrc);
typedef int (*ctli_dataport_recv_fn)(ctli_handle tphandle, unsigned short id, void* pdest, unsigned int npdest);
typedef int (*ctli_dataport_cancel_fn)(ctli_handle tphandle, unsigned short id);
typedef int (*ctli_dataport_enable_fn)(ctli_handle tphandle, unsigned short id);
typedef int (*ctli_dataport_get_param_fn)(ctli_handle tphandle, unsigned short id, CTLI_DATAPORT_PARAM param_type, ctli_dataport_params* params);
typedef int (*ctli_dataport_set_param_fn)(ctli_handle tphandle, unsigned short id, CTLI_DATAPORT_PARAM param_type, const ctli_dataport_params* params);

typedef struct ctli_context {
    unsigned short protocol_version;
    ctli_handle tphandle;
    ctli_delete_transport_fn delete_transport;
    ctli_receive_fn receive;
    ctli_send_fn send;
    ctli_abort_fn abort;
    ctli_get_param_fn get_param;
    ctli_set_param_fn set_param;
    ctli_audio_create_fn audio_create;
    ctli_audio_delete_fn audio_delete;
    ctli_audio_receive_fn audio_receive;
    ctli_audio_abort_fn audio_abort;
    ctli_audio_get_params_fn audio_get_params;
    ctli_audio_set_params_fn audio_set_params;
    ctli_dataport_select_fn dataport_select;
    ctli_dataport_send_fn dataport_send;
    ctli_dataport_recv_fn dataport_recv;
    ctli_dataport_cancel_fn dataport_cancel;
    ctli_dataport_enable_fn dataport_enable;
    ctli_dataport_get_param_fn dataport_get_param;
    ctli_dataport_set_param_fn dataport_set_param;
} ctli_context;

typedef int (*ctli_create_transport_fn)(const struct ctli_create_params* params, struct ctli_context* ctx);

#endif // CINEMO_TRANSPORT_H_INCLUDED
