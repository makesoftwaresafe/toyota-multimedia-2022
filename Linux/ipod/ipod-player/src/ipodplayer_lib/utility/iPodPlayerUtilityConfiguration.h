#ifndef IPOD_PLAYER_UTILITY_H
#define IPOD_PLAYER_UTILITY_H

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <libxml2/libxml/tree.h>
#include <libxml2/libxml/parser.h>
#include <libxml2/libxml/xpath.h>
#include "adit_typedef.h"

/* error code for util */
#define IPOD_UTIL_OK                         0                              /* util ok              */
#define IPOD_UTIL_ERROR                      -1000                          /* util error           */
#define IPOD_UTIL_ERR_INVALID_PARAMETER      -1001                          /* util invalid error   */


#define IPOD_UTIL_CFG_STR_MAX_SIZE           512                            /* max size of string   */
#define IPOD_UTIL_CFG_PATH_MAX_LEN           1023                           /* max path length      */

#define IPOD_UTIL_CFG_FILE_DIR_NAME          "/etc/pfcfg/"                  /* cfg file directory   */
#define IPOD_UTIL_CFG_FILE_EXTENSION         ".xml"                         /* cfg file extension   */
#define IPOD_UTIL_CFG_ITEM_TAG               "/cfgitem"                     /* cfg set node name    */
#define IPOD_UTIL_CFG_INT_TAG                "integer"                      /* int node attribute   */
#define IPOD_UTIL_CFG_STR_TAG                "string"                       /* str node attribute   */


/* type of util cfg value */
typedef enum
{
    IPOD_UTIL_CFG_INT_VALUE = 0,
    IPOD_UTIL_CFG_STR_VALUE
} IPOD_UTIL_CFG_VALUE_TYPE;

/* union of key value */
typedef struct
{
    U32 size;                                                               /* for size of string    */
    U8  *strVal;                                                            /* for string value      */
} IPOD_UTIL_CFG_STR;

typedef struct IPOD_UTIL_CFG_INFO *IPOD_UTIL_CFG_HANDLE;

/* util cfg interface */
IPOD_UTIL_CFG_HANDLE iPodUtilInitCf(const U8 *xmlFile, const U8* rootTag);
S32 iPodUtilGetNumCfn(IPOD_UTIL_CFG_HANDLE cfgHandle, const U8 *key, U32 maxNum, S32 *value);
S32 iPodUtilGetNumCfs(IPOD_UTIL_CFG_HANDLE cfgHandle, const U8 *key, U32 maxNum, IPOD_UTIL_CFG_STR *value);
void iPodUtilDeInitCf(IPOD_UTIL_CFG_HANDLE cfgHandle);

#endif /* IPOD_PLAYER_UTILITY_H */
