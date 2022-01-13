/****************************************************
 *  ipp_iap2_hidcommon.h
 *  Created on: 2014/01/22 17:32:00
 *  Implementation of the Class ipp_iap2_common
 *  Original author: madachi
 ****************************************************/

#ifndef __ipp_iap2_hidcommon_h__
#define __ipp_iap2_hidcommon_h__

#include <iap2_service_init.h>
#include <adit_dlt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "adit_typedef_linux.h"
#include "iap2_defines.h"
#include "iap2_commands.h"
#include "iap2_parameters.h"
#include "iap2_parameter_free.h"
#include "iPodPlayerDef.h"
#include "iPodPlayerUtilityLog.h"


/* HID report definition */
/*              HID report control code     */
#define IAP2_RESTORE_FLAG           0x10000000    /* Restore saved report ID */
#define IAP2_RESTORE_FLAG_CLEAR     0x20000000    /* Claer saved report ID */
#define IAP2_NOT_SAVE_FLAG          0x40000000    /* Not saved report ID */

#define IAP2_RELEASE_REPORT         0x00000000    /* Release report */
#define IAP2_START_HID_DESC_MASK    0x01000000    /* Start HID */
#define IAP2_START_HID_GROUP1       0x01000001    /* HID Descriptor group 1 */
#define IAP2_START_HID_GROUP2       0x01000002    /* HID Descriptor group 2 */
#define IAP2_START_HID_GROUP3       0x01000003    /* HID Descriptor group 3 */
#define IAP2_STOP_HID               0x02000000    /* Stop HID */

/*              HID descriptor playback 1   */
#define IAP2_PLAY_REPORT            0x00000001    /* playback report */
#define IAP2_PAUSE_REPORT           0x00000002    /* pause report */
#define IAP2_NEXT_TR_REPORT         0x00000004    /* Next track report */
#define IAP2_PREVIOUS_TR_REPORT     0x00000008    /* Previous track report */
#define IAP2_RANDOM_PLAY_REPORT     0x00000010    /* Random play report */
#define IAP2_REPEAT_REPORT          0x00000020    /* Repeat report */

/*              HID descriptor playback 2   */
#define IAP2_TRACKING_NR_REPORT     0x00000001    /* Tracking normal report */
#define IAP2_TRACKING_INC_REPORT    0x00000002    /* Tracking increment report */
#define IAP2_TRACKING_DEC_REPORT    0x00000004    /* Tracking decrement report */
#define IAP2_PLAY_PAUSE_REPORT      0x00000008    /* Play/Pause report */
#define IAP2_VOICE_COMMAND_REPORT   0x00000010    /* Voice command report */
#define IAP2_MUTE_REPORT            0x00000020    /* Mute report */

/*              HID descriptor volume       */
#define IAP2_VOLUP_REPORT           0x00000001    /* volume up */
#define IAP2_VOLDN_REPORT           0x00000002    /* volume down */
#define IAP2_REPORT_TBL_STOP        0xffffffff    /* report tbl stop */


typedef U32 ReportTable_t, *PReportTable_t;

typedef struct _iAP2Descriptor_t
{
    const uint8_t   *table;
    size_t          size;
}iAP2Descriptor_t, *PiAP2Descriptor_t;

S32 ippiAP2SendHIDReports(iAP2Device_t* iap2Device, ReportTable_t reports_tbl[]);
S32 ippiAP2SendAccessoryHIDReport(iAP2Device_t* iap2Device, U8 report, U16 HIDComId);
S32 ippiAP2StartHID(iAP2Device_t *iap2Device, PiAP2Descriptor_t desc_tbl, U16 HIDComId);
S32 ippiAP2StopHID(iAP2Device_t *iap2Device, U16 HIDComponentIdentifier);
S32 ippiAP2StartHID_init(iAP2Device_t *iap2Device);
S32 ippiAP2StopHID_final(iAP2Device_t *iap2Device);

#endif /*__ipp_iap2_hidcommon_h__*/
 
