#include <stdlib.h>
#include <stdint.h>

#include "iap_hid_desc.h"
#include "iap1_dlt_log.h"

/*
 *  Gets item field from report descriptor
 */
HID_desc_t hid_report(PHID_DESC_INF pinf)
{
    int         bSize, bType, bTag;
    uint8_t     *end, item;
    int         res = HID_DESC_OK;
    uint8_t     *buffer = NULL;
    uint32_t    size = 0;

    if((pinf == NULL) || (pinf->buffer == NULL))
    {
        IAP1_USBP_LOG(DLT_LOG_ERROR, "hid descriptor error pinf = %p",pinf);
        return HID_DESC_ERROR;
    }
    
    buffer = pinf->buffer;
    size = pinf->size;
    
    for(end = buffer + size ; buffer < end ; ) {
        item = *buffer++;
        bSize = HID_ITEM_SIZE_GET(HID_ITEM_SIZE(item));
        bType = HID_ITEM_TYPE(item);
        bTag  = HID_ITEM_TAG(item);

        /* check for long item */
        if(bTag == HID_ITEM_TAG_LONGITEM) {
            /* long item (not support) */
            res = HID_DESC_ERROR;
            IAP1_USBP_LOG(DLT_LOG_ERROR, "hid descriptor error bTag = %d",bTag);
        } else {
            if( (bType == HID_ITEM_TYPE_MAIN)   || 
                (bType == HID_ITEM_TYPE_GLOBAL) || 
                (bType == HID_ITEM_TYPE_LOCAL))
            {
                res = hid_set_report(bTag, bType, bSize, buffer, pinf);
            } else {
                res = HID_DESC_ERROR;
                IAP1_USBP_LOG(DLT_LOG_ERROR, "hid descriptor error bTag = %d",bTag);
            }
            buffer += bSize; /* next item */
        }
        
        if(res == HID_DESC_ERROR) {
            break;      /* leave function */
        }
    }

    return res;
}

HID_desc_t item_value(size_t bufsize, uint8_t *buffer, uint32_t *val)
{
    HID_desc_t ret = HID_DESC_OK;

    if((buffer == NULL) || (val == NULL))
    {
        IAP1_USBP_LOG(DLT_LOG_ERROR, "hid descriptor error buffer = %p val = %p",buffer,val);
        return HID_DESC_ERROR;
    }
    
    *val = 0;

    switch(bufsize)
    {
        case 0:
            *val = HID_DESC_OK;
            break;
        case 1:
            *val = *buffer;
            break;
        case 2:
            *val = UGETW(buffer);
            break;
        case 4:
            *val = UGETDW(buffer);
            break;
        default:
            ret = HID_DESC_ERROR;
            IAP1_USBP_LOG(DLT_LOG_ERROR, "hid descriptor error bufsize = %zu", bufsize);
            break;
    }
    
    return ret;
}

HID_desc_t hid_set_report(int bTag, int bType, size_t bufsize, uint8_t *buffer, PHID_DESC_INF pinf)
{
    HID_desc_t  res = HID_DESC_OK;
    uint32_t    pix = pinf->act_size;
    uint32_t    ix_max = sizeof(pinf->rep) / sizeof(pinf->rep[0]);
    uint32_t    val;

    if((buffer == NULL) || (pinf == NULL))
    {
        IAP1_USBP_LOG(DLT_LOG_ERROR, "hid descriptor error buffer = %p pinf = %p",buffer,pinf);
        return HID_DESC_ERROR;
    }
    
    res = item_value(bufsize, buffer, &val);    /* get item value */

    if((pix < ix_max) && (res == HID_DESC_OK)) {
        if((bTag == HID_GLOBAL_ITEM_REPORT_ID) && (bType == HID_ITEM_TYPE_GLOBAL)) {
            /* report id */
            pinf->rep[pix].rid = val;

        } else if((bTag == HID_GLOBAL_ITEM_REPORT_COUNT) && (bType == HID_ITEM_TYPE_GLOBAL)) {
            /* report count */
            pinf->rep[pix].len = val;

        } else if((bTag == HID_MAIN_ITEM_INPUT) && (bType == HID_ITEM_TYPE_MAIN)) {
            /* report direction IN */
            pinf->rep[pix].dir = HID_REPORT_IN;
            pix++;
            pinf->act_sz_in++;

        } else if((bTag == HID_MAIN_ITEM_OUTPUT) && (bType == HID_ITEM_TYPE_MAIN)) {
            /* report direction OUT */
            pinf->rep[pix].dir = HID_REPORT_OUT;
            pix++;
            pinf->act_sz_out++;
        }
    } else {
        res = HID_DESC_ERROR;
        IAP1_USBP_LOG(DLT_LOG_ERROR, "hid descriptor error pix = %d",pix);
    }
    pinf->act_size = pix;
    
    return res;
}

