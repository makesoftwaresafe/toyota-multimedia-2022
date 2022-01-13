
#ifndef IAP2_TSET_CONFIGURATION_PFCGF
#define IAP2_TSET_CONFIGURATION_PFCGF

#include "adit_typedef.h"

#ifdef __cplusplus
extern "C" {
#endif


#define IAP2_AUTH_CONFIGURATION_FILE      "IPOD_AUTH"
#define IAP2_CFG_STR_MAX                  256
#define IAP2_CFG_NUM_LENGTH               1
#define IAP2_ERR_ABORT                    -1

#define IPOD_AUTH_DEV_NAME             "IPOD_AUTH_DEV_NAME"
#define IPOD_AUTH_IOCTL_REG            "IPOD_AUTH_IOCTL_REG"
#define IPOD_AUTH_GPIO_RESET           "IPOD_AUTH_GPIO_RESET"
#define IPOD_AUTH_GPIO_READY           "IPOD_AUTH_GPIO_READY"
#define IPOD_AUTH_DEV_COM_SHORT_WAIT   "IPOD_AUTH_DEV_COM_SHORT_WAIT"
#define IPOD_AUTH_DEV_COM_WAIT         "IPOD_AUTH_DEV_COM_WAIT"
#define IPOD_AUTH_DEV_COM_LONG_WAIT    "IPOD_AUTH_DEV_COM_LONG_WAIT"
#define IPOD_AUTH_CP_AUTODETECT        "IPOD_AUTH_CP_AUTODETECT"


typedef struct _IAP2TEST_Cfg
{
    U8 *name;
    U8 isInt;
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
} IAP2TEST_Cfg;

typedef enum
{
   IAP2_DC_AUTH_DEV_NAME,
   IAP2_DC_AUTH_IOCTL_REG,
   IAP2_DC_AUTH_RESET,
   IAP2_DC_AUTH_READY,
   IAP2_DC_AUTH_SHORT_WAIT,
   IAP2_DC_AUTH_WAIT,
   IAP2_DC_AUTH_LONG_WAIT,
   IAP2_DC_COPRO_AUTODETECT,
}IAP2_DC_TYPE;

/***************************************************************************//**
* Read the configuration values from the PFCFG file and store the settings
* into IAP2TEST_Cfg g_iAP2TestCfg[]
*
* \param[out] *num_cfg Pointer to hold the no. of config entries in the config file.
* \return Pointer to structure holding the values read from config file.
*
* \see
* \note
******************************************************************************/
IAP2TEST_Cfg * iAP2TestGetDevconfParameter(U8* num_cfg);

/***************************************************************************//**
* Free the configuration values stored in IAP2TEST_Cfg g_iAP2TestCfg[]
*
* \param[in] Pointer to structure holding the values read from config file
* \param[in] num_cfg no. of keys in the config file
* \return None
*
* \see
* \note
******************************************************************************/
void iAP2TestFreeDevconfParameter(IAP2TEST_Cfg * p_iAP2TestCfg, U8 num_cfg);

#ifdef __cplusplus
}
#endif

#endif    /* IAP2_TEST_CONFIGURATION_PFCFG */

