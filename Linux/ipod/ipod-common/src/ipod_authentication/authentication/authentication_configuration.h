
#ifndef IAP_AUTHENTICATION_CONFIGURATION
#define IAP_AUTHENTICATION_CONFIGURATION

#include "adit_typedef.h"

#ifdef __cplusplus
extern "C" {
#endif


#define IPOD_AUTH_CFG_STR_MAX 256
#define IPOD_NULL_CHAR_LEN    1
/* name of the pfcfg configuration file */
#define IPOD_AUTH_CONFIGURATION_FILE        "IPOD_AUTH"
/* name of the configuration values */
#define IPOD_AUTH_DEV_NAME                  "IPOD_AUTH_DEV_NAME"
#define IPOD_AUTH_GPIO_RESET                "IPOD_AUTH_GPIO_RESET"
#define IPOD_AUTH_GPIO_READY                "IPOD_AUTH_GPIO_READY"
#define IPOD_AUTH_DEV_COM_SHORT_WAIT        "IPOD_AUTH_DEV_COM_SHORT_WAIT"
#define IPOD_AUTH_DEV_COM_WAIT              "IPOD_AUTH_DEV_COM_WAIT"
#define IPOD_AUTH_DEV_COM_LONG_WAIT         "IPOD_AUTH_DEV_COM_LONG_WAIT"
#define IPOD_AUTH_IOCTL_REG                 "IPOD_AUTH_IOCTL_REG"
#define IPOD_AUTH_CP_AUTODETECT             "IPOD_AUTH_CP_AUTODETECT"


typedef struct _IPOD_AUTH_CFG
{
    /* name of pfcfg file */
    U8 *name;
    /* value is of type integer */
    U8 isInt;
    /* union to store configuration values */
    union
    {
        S32 val;
        U8* p_val;
    }para;
    U16 multi;
    /* amount of this configuration value */
    U16 count;
    /* configuration value set by application */
    BOOL setValue;
} IPOD_AUTH_Cfg;

/* enum values correlate to the position in the pfcfg file */
typedef enum
{
    IPOD_AUTH_DC_DEV_NAME,                  /* = 1 */
    IPOD_AUTH_DC_IOCTL,                     /* = 2 */
    IPOD_AUTH_DC_GPIO_RESET,                /* = 3 */
    IPOD_AUTH_DC_GPIO_READY,                /* = 4 */
    IPOD_AUTH_DC_DEV_COM_SHORT_WAIT,        /* = 5 */
    IPOD_AUTH_DC_DEV_COM_WAIT,              /* = 6 */
    IPOD_AUTH_DC_DEV_COM_LONG_WAIT,         /* = 7 */
    IPOD_AUTH_DC_CP_AUTODETECT,             /* = 8 */
    IPOD_AUTH_DC_MAX_NUM                    /* = 9 */

} IPOD_DC_TYPE;

/* enum values correlate values read from Device Version Register(0x00)*/
typedef enum
{
    CP_NOT_SET = 0x00,               /* = 0x00 Not Set*/
    CP_2_0_B   = 0x03,               /* = 0x03 */
    CP_2_0_C   = 0x05,               /* = 0x05 */
    CP_3_0     = 0x07                /* = 0x07 */
} IPODCoProVer_t;

/* get integer value from PFCFG file */
S32 Authentication_util_bGetCfn(VP IPOD_AUTH_CFG_FileName, S8 *identifier,
                                S32 *int_value, U32 length);

S32 Authentication_util_bGetCfs(VP IPOD_AUTH_CFG_FileName, S8 *identifier,
                                char *str_value,
                                U32 size);


/*
 * \fn  S32 AuthenticationSetDevconfParameter(AuthenticationConfig_t* AuthConfigValues)
 *
 * \par INPUT PARAMETERS
  * AuthenticationConfig_t* AuthConfigValues - structure with configuration values from the Application.
 * \par RETURN PARAMETER
 * \li \c \b #IPOD_AUTH_OK - If configuration could be set or was already set.
 * \li \c \b #IPOD_AUTH_ERROR - In case of an error.
 * \par DESCRIPTION
 * Use the configuration values from the Application and store the settings into IPOD_AUTH_Cfg g_AuthCfg[].
 * Note:
 * Free
 *
 * The semaphore object is kept alive until the process dies and cleans
 */
S32 AuthenticationSetDevconfParameter(AuthenticationConfig_t* AuthConfigValues);

/*
 * \fn  S32 AuthenticationGetDevconfParameter(void)
 *
 * \par INPUT PARAMETERS
  *
 * \par RETURN PARAMETER
 * \li \c \b #IPOD_AUTH_OK - If configuration could be set or was already set.
 * \li \c \b #IPOD_AUTH_ERROR - In case of an error.
 * \par DESCRIPTION
 * Use the configuration values from the PFCFG file and store the settings into IPOD_AUTH_Cfg g_AuthCfg[].
 */
S32 AuthenticationGetDevconfParameter(void);

/*
 * \fn  IPOD_AUTH_Cfg* AuthenticationGetDevInfo(void)
 *
 * \par INPUT PARAMETERS
  *
 * \par RETURN PARAMETER
 * \li \c \b #IPOD_AUTH_Cfg* - Return the structure g_AuthCfg[] of type IPOD_AUTH_Cfg.
 * \par DESCRIPTION
 * Return a pointer to authentication configuration IPOD_AUTH_Cfg g_AuthCfg[].
 * Note:
 *
 */
IPOD_AUTH_Cfg* AuthenticationGetDevInfo(void);


#ifdef __cplusplus
}
#endif

#endif /* IAP_AUTHENTICATION_CONFIGURATION */
