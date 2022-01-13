/****************************************************
 *  ipp_iap2_hidcommon.c
 *  Created on: 2014/01/22 15:42:00
 *  Implementation of handling HID
 *  Original author: madachi
 ****************************************************/

#include "ipp_iap2_hidcommon.h"
#include "iPodPlayerCoreDef.h"
#include "ipp_iap2_common.h"


/* construction data for HID descriptor */
const uint8_t g_ippHIDDescriptorGroup1[] =
{
                                    0x05, 0x0C, /* usage page   */
                                    0x09, 0x01, /* usage        */
                                    0xA1, 0x01, /* collection   */
                                                /* Consumer     */
                                    0x15, 0x00, /* logical min  */
                                    0x25, 0x01, /* logical max  */
                                    0x75, 0x01, /* report size  */
                                    0x95, 0x06, /* report count */
                                    0x09, 0xB0, /* usage: Play  */
                                    0x09, 0xB1, /* usage: Pause */
                                    0x09, 0xB5, /* usage: Scan Next Track       */
                                    0x09, 0xB6, /* usage: scan previous track   */
                                    0x09, 0xB9, /* usage: Random Play  */
                                    0x09, 0xBC, /* usage: Repeat       */
                                    0x81, 0x02, /* Input        */
                                    0x75, 0x02, /* report size  */
                                    0x95, 0x01, /* report count */
                                    0x81, 0x03, /* Input        */
                                    0xC0        /* end collection       */
};

const uint8_t g_ippHIDDescriptorGroup2[] =
{
                                    0x05, 0x0C, /* usage page   */
                                    0x09, 0x01, /* usage        */
                                    0xA1, 0x01, /* collection   */
                                                /* Consumer     */
                                    0x15, 0x00, /* logical min  */
                                    0x25, 0x01, /* logical max  */
                                    0x75, 0x01, /* report size  */
                                    0x95, 0x06, /* report count */
                                    0x09, 0xBE, /* usage: Tracking normal       */
                                    0x09, 0xCA, /* usage: Tracking increment    */
                                    0x09, 0xCB, /* usage: Tracking decrement    */
                                    0x09, 0xCD, /* usage: Play/Pause    */
                                    0x09, 0xCF, /* usage: Voice command         */
                                    0x09, 0xE2, /* usage: Mute          */
                                    0x81, 0x02, /* Input        */
                                    0x75, 0x02, /* report size  */
                                    0x95, 0x01, /* report count */
                                    0x81, 0x03, /* Input        */
                                    0xC0        /* end collection       */
};

const uint8_t g_ippHIDDescriptorGroup3[] =
{
                                    0x05, 0x0C, /* usage page   */
                                    0x09, 0x01, /* usage        */
                                    0xA1, 0x01, /* collection   */
                                                /* Consumer     */
                                    0x15, 0x00, /* logical min  */
                                    0x25, 0x01, /* logical max  */
                                    0x75, 0x01, /* report size  */
                                    0x95, 0x02, /* report count */
                                    0x09, 0xE9, /* usage: volume up     */
                                    0x09, 0xEA, /* usage: volume down   */
                                    0x81, 0x02, /* Input        */
                                    0x75, 0x02, /* report size  */
                                    0x95, 0x01, /* report count */
                                    0x81, 0x03, /* Input        */
                                    0xC0        /* end collection       */
};


/*******************************************
    Accessory HID Report control.
********************************************/
S32 ippiAP2SendAccessoryHIDReport(iAP2Device_t* iap2Device, U8 report, U16 HIDComId)
{
    S32 rc = IAP2_OK;
    iAP2AccessoryHIDReportParameter HIDReportPara;

    /* parameter check */
    if(iap2Device == NULL)
    {
        rc = IAP2_BAD_PARAMETER;
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER);

    }
    else
    {
        memset(&HIDReportPara, 0, sizeof(HIDReportPara));
        rc = ippiAP2AllocateandUpdateData(&HIDReportPara.iAP2HIDComponentIdentifier,
                                       &HIDComId,
                                       &HIDReportPara.iAP2HIDComponentIdentifier_count,
                                       1, iAP2_uint16);

        if(rc == IAP2_OK)
        {
            rc = ippiAP2AllocateandUpdateData(&HIDReportPara.iAP2HIDReport,
                                           &report,
                                           &HIDReportPara.iAP2HIDReport_count,
                                           sizeof(report), iAP2_blob);
        }

        if(rc == IAP2_OK)
        {
            /* Accessory HID Report API */
            rc = iAP2AccessoryHIDReport(iap2Device, &HIDReportPara);
        }

        /* Free API parameter buffer */
        iAP2FreeiAP2AccessoryHIDReportParameter(&HIDReportPara);
    }

    /* Conver to error code of iPodPlayer from error code of iAP2 library */
    rc = ippiAP2RetConvertToiPP(rc);

    return rc;
}

/*******************************************
    Send report IDs.
********************************************/
S32 ippiAP2SendHIDReports(iAP2Device_t* iap2Device, ReportTable_t reports_tbl[])
{
    S32 lp = 0;
    S32 rc = IPOD_PLAYER_OK;
    iAP2Descriptor_t hid_desc;
    U8  exec_report = 0;
    static  U8 save_report = 0;

    /* Checks parameter */
    if(iap2Device != NULL)
    {
        memset(&hid_desc, 0, sizeof(hid_desc));
        /* Checks the report ID array */
        for(lp = 0; reports_tbl[lp] != IAP2_REPORT_TBL_STOP; lp++)
        {
            /* clear saved report ID  */
            if(reports_tbl[lp] & IAP2_RESTORE_FLAG_CLEAR)
            {
                save_report = 0;
                reports_tbl[lp] ^= IAP2_RESTORE_FLAG_CLEAR;     /* clear flug */
            }

            if(reports_tbl[lp] & IAP2_START_HID_DESC_MASK)
            {
            /* Start HID */
                /* Checks HID descriptor table */
                if(reports_tbl[lp] == IAP2_START_HID_GROUP1)
                {
                    hid_desc.table = g_ippHIDDescriptorGroup1;
                    hid_desc.size = sizeof(g_ippHIDDescriptorGroup1);
                }
                else if(reports_tbl[lp] == IAP2_START_HID_GROUP2)
                {
                    hid_desc.table = g_ippHIDDescriptorGroup2;
                    hid_desc.size = sizeof(g_ippHIDDescriptorGroup2);
                }
                else if(reports_tbl[lp] == IAP2_START_HID_GROUP3)
                {
                    hid_desc.table = g_ippHIDDescriptorGroup3;
                    hid_desc.size = sizeof(g_ippHIDDescriptorGroup3);
                }
                else
                {
                    rc = IAP2_BAD_PARAMETER;
                    IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc, reports_tbl[lp]);
                }
                if(rc == IPOD_PLAYER_OK)
                {
                    rc = ippiAP2StartHID(iap2Device, &hid_desc, 0);
                    if(rc != IPOD_PLAYER_OK)
                    {
                        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc, reports_tbl[lp]);
                        break;
                    }
                }
            }
            else if(reports_tbl[lp] == IAP2_STOP_HID)
            {
            /* Stop HID */
                rc = ippiAP2StopHID(iap2Device, 0);
                if(rc != IPOD_PLAYER_OK)
                {
                    IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc);
                }
            }
            else
            {
            /* SendAccessoryHIDReport */
                if(rc == IPOD_PLAYER_OK)
                {
                    exec_report = (U8)reports_tbl[lp];
                    if((reports_tbl[lp] & IAP2_RESTORE_FLAG))
                    {
                        exec_report |= save_report; /* Restore saved report ID */
                    }
                    else if(!(reports_tbl[lp] & IAP2_NOT_SAVE_FLAG))
                    {
                        save_report = exec_report;  /* Save report ID */
                    }
                    
                    rc = ippiAP2SendAccessoryHIDReport(iap2Device, exec_report, 0);
                    if(rc != IPOD_PLAYER_OK)
                    {
                        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc, reports_tbl[lp]);
                        break;
                    }
                }
            }
        }
    }
    else
    {
        rc = IAP2_BAD_PARAMETER;
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER);
    }

    return rc;
}

/*******************************************
    Start HID control.
********************************************/
S32 ippiAP2StartHID(iAP2Device_t *iap2Device, PiAP2Descriptor_t desc_tbl, U16 HIDComId)
{
    S32 rc = IAP2_OK;
    iAP2StartHIDParameter StartHIDPara;
    U16 VendorId = IAP2_VENDOR_ID;      /* Vindor id */
    U16 ProductId = IAP2_PRODUCT_ID;    /* Product id */

    /* parameter check */
    if((iap2Device == NULL) || (desc_tbl == NULL))
    {
        rc = IAP2_BAD_PARAMETER;
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER);

    }
    else
    {
        memset(&StartHIDPara, 0, sizeof(StartHIDPara));
        rc = ippiAP2AllocateandUpdateData(&StartHIDPara.iAP2HIDComponentIdentifier,
                                       &HIDComId,
                                       &StartHIDPara.iAP2HIDComponentIdentifier_count,
                                       1, iAP2_uint16);
        if(rc == IAP2_OK)
        {
            rc = ippiAP2AllocateandUpdateData(&StartHIDPara.iAP2VendorIdentifier,
                                           &VendorId,
                                           &StartHIDPara.iAP2VendorIdentifier_count,
                                           1, iAP2_uint16);
        }
        if(rc == IAP2_OK)
        {
            rc = ippiAP2AllocateandUpdateData(&StartHIDPara.iAP2ProductIdentifier,
                                           &ProductId,
                                           &StartHIDPara.iAP2ProductIdentifier_count,
                                           1, iAP2_uint16);
        }
        if(rc == IAP2_OK)
        {
            rc = ippiAP2AllocateandUpdateData(&StartHIDPara.iAP2HIDReportDescriptor,
                                           (void *)desc_tbl->table,
                                           &StartHIDPara.iAP2HIDReportDescriptor_count,
                                           desc_tbl->size, iAP2_blob);
        }
        if(rc == IAP2_OK)
        {
            /* Start HID  */
            rc = iAP2StartHID(iap2Device, &StartHIDPara);
        }

        /* Free API parameter buffer */
        iAP2FreeiAP2StartHIDParameter(&StartHIDPara);
    }

    /* Wait for iPod device (unit of 100ms) */
    rc = ippiAP2Wait100ms(1);
    if(rc == IPOD_PLAYER_OK)
    {
        /* Conver to error code of iPodPlayer from error code of iAP2 library */
        rc = ippiAP2RetConvertToiPP(rc);
    }
    else
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc);
    }

    return rc;
}

/*******************************************
    Start HID control.initialize
********************************************/
S32 ippiAP2StartHID_init(iAP2Device_t *iap2Device)
{
    ReportTable_t   reports_tbl[] = {   IAP2_START_HID_GROUP1 | IAP2_RESTORE_FLAG_CLEAR,
                                        IAP2_REPORT_TBL_STOP};

    /* start HID */
    return ippiAP2SendHIDReports(iap2Device, reports_tbl);
}

/*******************************************
    Stop HID control
********************************************/
S32 ippiAP2StopHID(iAP2Device_t *iap2Device, U16 HIDComId)
{
    S32 rc = IAP2_OK;
    iAP2StopHIDParameter StopHIDPara;

    /* parameter check */
    if(iap2Device == NULL)
    {
        rc = IAP2_BAD_PARAMETER;
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER);

    }
    else
    {
        memset(&StopHIDPara, 0, sizeof(StopHIDPara));
        rc = ippiAP2AllocateandUpdateData(&StopHIDPara.iAP2HIDComponentIdentifier,
                                       &HIDComId,
                                       &StopHIDPara.iAP2HIDComponentIdentifier_count,
                                       1, iAP2_uint16);
        if(rc == IAP2_OK)
        {
            /* Stop HID */
            rc = iAP2StopHID(iap2Device, &StopHIDPara);
        }

        /* Free API parameter buffer */
        iAP2FreeiAP2StopHIDParameter(&StopHIDPara);
    }

    /* Conver to error code of iPodPlayer from error code of iAP2 library */
    rc = ippiAP2RetConvertToiPP(rc);

    return rc;
}

/*******************************************
    Start HID control.finalize
********************************************/
S32 ippiAP2StopHID_final(iAP2Device_t *iap2Device)
{
    ReportTable_t   reports_tbl[] = {   IAP2_STOP_HID,
                                        IAP2_REPORT_TBL_STOP};

    /* stop HID */
    return ippiAP2SendHIDReports(iap2Device, reports_tbl);
}

