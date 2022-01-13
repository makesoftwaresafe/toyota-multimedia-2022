/* -----------------------------------------------------------------------------
 * An invalid character is automatically inserted by cvs when the file is
 * commited. We can not do anything about it.
 * -----------------------------------------------------------------------------
 */
/**************************************************************************
 * \File : iap_util_func.h
 * \version: $Revision: 1.27 $
 *  header file
 *  interfaces to utility functions
 *
 *  \Component: utility
 *  \Author:     Reinhard Willig
 *
 *  \Copyright: (C) 2003-2007 ADIT
 *
 *  \History:
 *  $Log: iap_util_func.h,v $
 *  Revision 1.27  2012/01/05 08:52:24  kgerhards
 *  Fix LINT findings
 *
 *  Revision 1.26  2011/12/21 13:21:12  kgerhards
 *  SFEIIX-74 [b_MAIN] Support new Apple Spec R44 (ATS 2.2)
 *
 *  Revision 1.24.4.1  2011/09/15 11:32:54  serhard
 *  SWGIIX-1472: put define for extern c in each header file
 *
 *  Revision 1.24  2011/05/24 08:45:00  jlorenz
 *  Multiple iPod support [Early Version]
 *
 *  Revision 1.23  2010/09/30 05:55:09  kgerhards
 *  Added Linux Support
 *
 *  Revision 1.22  2010/08/04 10:28:37  mshibata
 *  Code improve
 *
 *  Revision 1.21  2010/07/06 02:44:27  mshibata
 *  Fix the compile error to porting tkernel and posix.
 *
 *  Revision 1.19  2010/06/01 02:50:34  mshibata
 *  Fix for QAC, Lint.
 *  Remove deactivation
 *
 *  Revision 1.18  2010/03/23 08:07:31  mshibata
 *  Re define SYSTEMPROGRAM as SYSPROG
 *  Fix JIRA[SWGII-2667]
 *
 *  Revision 1.16  2010/03/09 08:26:37  mshibata
 *  Code improvement
 *
 *  Revision 1.14  2010/01/26 12:08:36  mshibata
 *  Fix the iPodGetArtworkData
 *
 *  Revision 1.7  2009/09/18 10:35:48  mshibata
 *  Fix the sorce code for testing
 *
 *  Revision 1.6  2009/08/24 08:57:16  mshibata
 *  Fix for QAC,Lint
 *
 *  Revision 1.3  2009/07/07 08:26:04  mshibata
 *  Add new API
 *  Improve the code
 *
 *  Revision 1.2  2009/06/29 09:05:19  mshibata
 *  Add new API
 *
 *  Revision 1.1  2009/06/18 05:54:56  kgerhards
 *  First commit of Gen2 code.
 *
 *  Revision 1.8  2008/10/21 12:28:55  smaleyka
 *  SWG-24015 (SWG-24018) "[b_MAIN] functions "S32 iPodGetIPodName(U8* iPodName)" and "S32 iPodGetSerialNumber(U8* serialNumber)" have an "off-by-one" error"
 *  SWG-23675 (SWG-24031) "[b_MAIN] Playback of iPOD is not possible after coming out of undervoltage"
 *  SWG-23374 (SWG-23547) "[b_MAIN] When reading of the iPod Serial Number fails a retry gets stuck"
 *  SWG-23386 (SWG-23544) "[b_MAIN] iPod nano2g chrash"
 *  SWG-23893 (SWG-24039) "[b_MAIN] Cyclic audio gaps are observed with iPod video 30 GB capacity"
 *  SWG-23995 (SWG-24047) "[b_MAIN] Setting iPod to extended Mode sporadically lasts more than 60 seconds "
 *  SWG-23675 (SWG-24031) "[b_MAIN] Playback of iPOD is not possible after coming out of undervoltage"
 *  SWG-24075 (SWG-24077) "[b_MAIN] Reset after Ipod touch disconnect while "Checking iPod" Popup is visible"
 *
 *  Revision 1.7  2008/10/08 13:37:33  smaleyka
 *  Performance improvement during insufficient devconf reader functions.
 *  Removed memcpy operation for devconf parsing.
 *
 *  Revision 1.6  2007/12/06 12:27:17  rwillig
 *  map ipod error code to adit error codes  SWG-21416
 *
 *  Revision 1.5  2007/10/15 12:08:58  rwillig
 *  warnings removed
 *
 *  Revision 1.4  2007/10/02 12:57:11  rwillig
 *  warnings removed
 *
 *  Revision 1.3  2007/08/07 14:50:26  kgerhards
 *  fix QAC warnings
 *
 *  Revision 1.2  2007/07/24 09:36:22  rwillig
 *  cleanup
 *
 *  Revision 1.1  2007/07/18 07:21:56  rwillig
 *  intial version
 *
 *
 ************************************************************************/
#ifndef __IAP_UTIL_FUNC_H__
#define __IAP_UTIL_FUNC_H__

#include <adit_typedef.h>
#include "iap_transport_message.h"

#ifdef __cplusplus
extern "C" {
#endif

/*=========================================================*/
/*                 GLOBAL DECLARATIONS                     */
/*=========================================================*/
#define IPOD_ACKERR_UNKNOWN_DB_CATEGORY         0x01U
#define IPOD_ACKERR_CMD_FAILED                  0x02U
#define IPOD_ACKERR_OUT_OF_RESOURCES            0x03U
#define IPOD_ACKERR_BAD_PARAM                   0x04U
#define IPOD_ACKERR_UNKNOWN_ID                  0x05U
#define IPOD_ACKERR_ACK_PENDING                 0x06U
#define IPOD_ACKERR_ACC_NOT_AUTHENTICATED       0x07U
#define IPOD_ACKERR_BAD_AUTHENTICATION_VERSION  0x08U
#define IPOD_ACKERR_POWER_MODE_FAILED           0x09U
#define IPOD_ACKERR_CERT_INVALID                0x0AU
#define IPOD_ACKERR_CERT_PERMISSIONS_INVALID    0x0BU
#define IPOD_ACKERR_FILE_IS_IN_USE              0x0CU
#define IPOD_ACKERR_INVALID_FILE_HANDLE         0x0DU
#define IPOD_ACKERR_DIR_NOT_EMPTY               0x0EU
#define IPOD_ACKERR_OPERATION_TMOUT             0x0FU
#define IPOD_ACKERR_UNAVAILABLE_MODE            0x10U
#define IPOD_ACKERR_INVALID_ACC_RESID_VALUE     0x11U
#define IPOD_ACKERR_IPOD_MAX_CONNECT            0x15U
#define IPOD_ACKERR_SESSION_WRITE_FAILURE       0x17U
#define IPOD_ACKERR_IPOD_NOT_CONNECTED          0xF0U

/* mapps ipod error code to adit error codes */

S32 iPod_get_error_code(S32 error);
U8* iPod_get_error_msg(S32 error);
S32 iPod_get_general_error_code(S32 error);
U16 iPod_convert_to_little16(const U8 *buf);
U32 iPod_convert_to_little32(const U8 *buf);
U64 iPod_convert_to_little64(const U8 *buf);
void iPod_convert_to_big16(U8 *msg, U16 data);
void iPod_convert_to_big32(U8 *msg, U32 data);
void iPod_convert_to_big64(U8 *msg, U64 data);
void iPod_convert_macaddr_to_bigendian(U8 *msg, U8 *data);

S32 GetAndSetMsg(IPOD_INSTANCE* iPodHndl, U8 *msg);

#ifdef __cplusplus
}
#endif

#endif /*__IAP_UTIL_FUNC_H__*/
