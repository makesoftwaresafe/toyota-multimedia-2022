
#ifndef IAP_TRANSPORT_CONFIGURATION
#define IAP_TRANSPORT_CONFIGURATION

#ifdef __cplusplus
extern "C" {
#endif

S32 iPodGetDevconfParameter(void);
S32 iPodFreeDevconfParameter(void);

#define IPOD_CFG_STR_MAX 256

#define IPOD_CONFIGURATION_FILE "IPOD_CTRL"
#define IPOD_TOKEN_ACC_CAPS_MAX_RANGE 2
#define IPOD_TOKEN_PREFERENCE_MAX_RANGE 16
#define IPOD_TOKEN_PREF_SET_FLG_MAX_RANGE 16
#define IPOD_TOKEN_FW_VER_MAX_RANGE 3
#define IPOD_TOKEN_HW_VER_MAX_RANGE 3
#define IPOD_TOKEN_ASSIST_CAPS_MAX_RANGE 2
#define IPOD_TOKEN_ACCINFO_FLG_MAX_RANGE 13
#define IPOD_DC_INFO_SPACE  " "
#define IPOD_MAX_MULTI_STRING_SUPPORT 16
typedef struct _IPOD_CFG
{
    U8 *name;
    U8 isInt;
    union
    {
        S32 val;
        U8* p_val;
    }para;
    U16 multi;
    U16 count;
} IPOD_Cfg;

typedef enum
{
    IPOD_DC_HID_DEV_INFO            = 0,
    IPOD_DC_IICDEVICE,           /* = 1 */
    IPOD_DC_READ_RETRIES,        /* = 2 */
    IPOD_DC_WAIT_TMO,            /* = 3 */
    IPOD_DC_READER_PRIO,         /* = 4 */
    IPOD_DC_READER_STKSZ,        /* = 5 */
    IPOD_DC_READER_LCID,         /* = 6 */
    IPOD_DC_WORKER_PRIO,         /* = 7 */
    IPOD_DC_WORKER_STKSZ,        /* = 8 */
    IPOD_DC_WORKER_LCID,         /* = 9 */
    IPOD_DC_AUDIOWORKER,         /* = 10 */
    IPOD_DC_AUDIOREADER,         /* = 11 */
    IPOD_DC_SEM_MODESWITCH,      /* = 12 */
    IPOD_DC_SEM_SENDER,          /* = 13 */
    IPOD_DC_SEM_AUTHENTICATION,  /* = 14 */
    IPOD_DC_FLG_WORKER,          /* = 15 */
    IPOD_DC_FLG_READER,          /* = 16 */
    IPOD_DC_MBF_AUDIO,           /* = 17 */
    IPOD_DC_GENERAL,             /* = 18 */
    IPOD_DC_EXTEND,              /* = 19 */
    IPOD_DC_AUDIO,               /* = 20 */
    IPOD_DC_STORAGE,             /* = 21 */
    IPOD_DC_IPODOUT,             /* = 22 */
    IPOD_DC_LOCATION,            /* = 23 */
    IPOD_DC_ACC_CAPS_BIT,        /* = 24 */
    IPOD_DC_PREFERENCE,          /* = 25 */
    IPOD_DC_PREF_SET_FLG,        /* = 26 */
    IPOD_DC_ACCINFO_NAME,        /* = 27 */
    IPOD_DC_FW_VER,              /* = 28 */
    IPOD_DC_HW_VER,              /* = 29 */
    IPOD_DC_MAN,                 /* = 30 */
    IPOD_DC_MODEL,               /* = 31 */
    IPOD_DC_SERIAL,              /* = 32 */
    IPOD_DC_ACC_STATUS,          /* = 33 */
    IPOD_DC_RF_CERTIFICATIONS,   /* = 34 */
    IPOD_DC_ACCINFO_FLG,         /* = 35 */
    IPOD_DC_ASSIST_SUPPORT,      /* = 36 */
    IPOD_DC_ASSIST_CAPS,         /* = 37 */
    IPOD_DC_SATEL_MAX_REFRESH,   /* = 38 */
    IPOD_DC_SATEL_RECOM_REFRESH, /* = 39 */
    IPOD_DC_TOTAL_WIDTH_INCHES,  /* = 40 */
    IPOD_DC_TOTAL_HEIGHT_INCHES, /* = 41 */
    IPOD_DC_TOTAL_WIDTH_PIXELS,  /* = 42 */
    IPOD_DC_TOTAL_HEIGHT_PIXELS, /* = 43 */
    IPOD_DC_WIDTH_PIXELS,        /* = 44 */
    IPOD_DC_HEIGHT_PIXELS,       /* = 45 */
    IPOD_DC_TOTAL_FEATURES_MASK, /* = 46 */
    IPOD_DC_TOTAL_GAMMA_VALUE,   /* = 47 */
    IPOD_DC_SIMPLE,              /* = 48 */
    IPOD_DC_INST_COUNT,          /* = 49 */
    IPOD_DC_DISPLAY_REMOTE,      /* = 50 */
    IPOD_DC_MAX_PAYLOAD_SIZE,    /* = 51 */
    IPOD_DC_MAX_NUM              /* = 52 */
} IPOD_DC_TYPE;

/* get integer value from PFCFG file */
EXPORT S32 iPod_util_bGetCfn(VP IPOD_CFG_FileName, S8  *identifier,
                             S32 * int_value, U32 length);

EXPORT S32 iPod_util_bGetCfs(VP IPOD_CFG_FileName, S8  *identifier,
                             char* str_value,
                             U32 size);

IPOD_Cfg* iPodGetDevInfo(void);

#ifdef __cplusplus
}
#endif

#endif /* IAP_TRANSPORT_CONFIGURATION */
