/****************************************************
 *  ipp_iap2_common.c
 *  Created on: 2014/01/22 15:42:00
 *  Implementation of handling HID
 *  Original author: madachi
 ****************************************************/

#include "iPodPlayerCoreDef.h"
#include "ipp_iap2_common.h"

/* construction data for HID descriptor */
const IPOD_CORE_RETCODE_MAPPING_TBL g_ippiAP2_retcode_mapping_tbl[] = 
{
/* Table list for return code option */
    { IAP2_OK,                              IPOD_PLAYER_OK                                  },
    { IAP2_ERR_NO_MEM,                      IPOD_PLAYER_ERR_NOMEM                           },
    { IAP2_BAD_PARAMETER,                   IPOD_PLAYER_ERR_INVALID_PARAMETER               },
    { IAP2_INVALID_INPUT_PARAMETER,         IPOD_PLAYER_ERR_INVALID_PARAMETER               },
    { IAP2_INVALID_PARAMETER_LENGTH,        IPOD_PLAYER_ERR_INVALID_PARAMETER               },
    { IAP2_INVALID_PARAMETER_COUNT,         IPOD_PLAYER_ERR_INVALID_PARAMETER               },
    { IAP2_UNKNOWN_PARAMETER_ID,            IPOD_PLAYER_ERR_INVALID_PARAMETER               },
    { IAP2_UNKNOWN_SUB_PARAMETER_ID,        IPOD_PLAYER_ERR_INVALID_PARAMETER               },
    { IAP2_ERROR_INVALID_MESSAGE,           IPOD_PLAYER_ERR_INVALID_MESSAGE                 },
    { IAP2_UNKNOWN_MSG_ID,                  IPOD_PLAYER_ERR_INVALID_MSG_ID                  },
    { IAP2_INVALID_EAP_SESSION_IDENTIFIER,  IPOD_PLAYER_ERR_EAP_SESSION_ID                  },
    { IAP2_INVALID_EAP_IDENTIFIER,          IPOD_PLAYER_ERR_EAP_INVALID_ID                  },
    { IAP2_ERR_USB_ROLE_SWITCH_UNSUP,       IPOD_PLAYER_ERR_IPOD_ROLE_SW_UNSUP              },
    { IAP2_ERR_USB_ROLE_SWITCH_FAILED,      IPOD_PLAYER_ERR_IPOD_ROLE_SW_FAILED             },
    { IAP2_FILE_XFER_SETUP_NOT_RECVD,       IPOD_PLAYER_ERR_FILE_XFER_SETUP_NOT_RECVD       },
    { IAP2_FILE_XFER_SETUP_ALREADY_RECVD,   IPOD_PLAYER_ERR_FILE_XFER_SETUP_ALREADY_RECVD   },
    { IAP2_FILE_XFER_INVALID_ID,            IPOD_PLAYER_ERR_FILE_INVALID_ID                 },
    { IAP2_FILE_XFER_MAX_XFER_REACHED,      IPOD_PLAYER_ERR_FILE_XFER_MAX_XFER_REACHED      },
    { IAP2_FILE_XFER_ID_NOT_PRESENT,        IPOD_PLAYER_ERR_FILE_XFER_ID_NOT_PRESENT        },
    { IAP2_INVALID_START_OF_BYTES,          IPOD_PLAYER_ERR_INVALID_PARAMETER               },
    { IAP2_CTL_ERROR,                       IPOD_PLAYER_ERR_IPOD_CTRL_ERROR                 },
    { IAP2_DEV_NOT_CONNECTED,               IPOD_PLAYER_ERR_NOT_CONNECT                     }
};


#ifdef DUMP_CHECK
/*******************************************
    memory dump
********************************************/
void ipp_dump(U8 *pSrc, int byte)
{
    int i = 0;

    printf("address         : +0 +1 +2 +3 +4 +5 +6 +7 +8 +9 +a +b +c +d +e +f\n");
    for(i = 0; i < byte; i++)
    {
        if((i % 16) == 0) printf("%016lx: ", (unsigned long)&(pSrc[i]));
        printf("%02x ",pSrc[i]);
        if((i % 16) == 15) printf("\n");
    }
    printf("\n");
}
#endif

/*******************************************
    Check null pointer for API Parameter
********************************************/
BOOL ippiAP2CheckNullParameter(void *para_c, void *para_p)
{
    BOOL                                rc = FALSE;
    IPOD_PLAYER_CORE_IPODCTRL_CFG       *iCfg = (IPOD_PLAYER_CORE_IPODCTRL_CFG *)para_c;
    IPOD_PLAYER_CORE_IAP2_EXECUTE_PARAM *param = (IPOD_PLAYER_CORE_IAP2_EXECUTE_PARAM *)para_p;

    /* check configuration structure and excution parameter */
    if((iCfg != NULL) && (param != NULL))
    {
        /* check detail of parameter (Wait list, API parameter contents) */
        if((param->waitData != NULL) && (param->contents != NULL))
        {
            rc = TRUE;
        }
        else
        {
            /* parameter error */
            IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE,
                            IPOD_PLAYER_ERR_INVALID_PARAMETER, param->waitData, param->contents);
        }
    }
    else
    {
        /* parameter error */
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE,
                                IPOD_PLAYER_ERR_INVALID_PARAMETER, iCfg, param);
    }
    
    return rc;
}

/*******************************************
    Check ipod Connection ready
********************************************/
BOOL ippiAP2CheckConnectionReady(IPOD_PLAYER_CONNECTION_STATUS *dc)
{
    BOOL rc = FALSE;
    
    if(dc == NULL)
    {
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    if(dc->deviceStatus == IPOD_PLAYER_DEVICE_CONNECT_STATUS_CONNECT)
    {
        if(dc->authStatus == IPOD_PLAYER_AUTHENTICATION_SUCCESS)
        {
            rc = TRUE;
        }
    }

    return rc;
}

/*******************************************************************
    Allocte the interface of iAP2 library (blob type)
********************************************************************/
S32 ippiAP2AllocateandUpdateBlob(   void        *dstp,      /* destination pointer */
                                    void        *srcp,      /* source pointer */
                                    U16         *dstc,      /* destination count pointer */
                                    U16         srcc)       /* source count */
{
    S32 rc = IAP2_ERR_NO_MEM;
    iAP2Blob** dest_blob = (iAP2Blob**)dstp;

    *dest_blob = (iAP2Blob*)calloc(1, sizeof(iAP2Blob));
    if(*dest_blob != NULL)
    {
        (*dest_blob)->iAP2BlobData = (U8*)calloc(srcc, sizeof(U8));
        if((*dest_blob)->iAP2BlobData != NULL)
        {
            memcpy((*dest_blob)->iAP2BlobData, srcp, srcc);
            (*dest_blob)->iAP2BlobLength = srcc;
            *dstc = 1;
            rc = IAP2_OK;
        }
        else
        {
            /* allocation error */
            free(*dest_blob);
            IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_NOMEM);
        }
    }
    else
    {
        /* allocation error */
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_NOMEM);
    }
    
    /* log output */
    MEM_DUMP((U8 *)((*dest_blob)->iAP2BlobData), (*dest_blob)->iAP2BlobLength);
    
    return rc;
}

/*******************************************************************
    Allocte the interface of iAP2 library (UTF8 type)
********************************************************************/
S32 ippiAP2AllocateandUpdateUtf8(   void        *dstp,      /* destination pointer */
                                    void        *srcp,      /* source pointer */
                                    U16         *dstc,      /* destination count pointer */
                                    U16         srcc)       /* source count */
{
    S32     rc = IAP2_ERR_NO_MEM;
    int     ix = 0, rm_ix = 0;
    U8**    pDes = NULL;
    U8**    pSrc = (U8 **)srcp;
    U32     nStrlen = 0;

    pDes = (U8 **)calloc(srcc, sizeof(U8*));
    if(pDes != NULL)
    {
        for(ix = 0; ix < srcc; ix++, pSrc++)
        {
            nStrlen = strnlen((const char*)*pSrc, IPP_STRING_MAX) + IPP_CHAR_NULL_LEN;
            pDes[ix] = (U8*)calloc(nStrlen, sizeof(U8));
            if(pDes[ix] != NULL)
            {
                memcpy(pDes[ix], *pSrc, nStrlen);
                *(pDes[ix] + nStrlen - IPP_CHAR_NULL_LEN) = '\0';
            }
            else
            {
                /* allocation error */
                for(rm_ix = 0; rm_ix < ix; rm_ix++)
                {
                    free(pDes[rm_ix]);
                }
                IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_NOMEM);
                break;
            }
        }
        *dstc = ix;
        *((U8 ***)dstp) = pDes;
        rc = IAP2_OK;
    }
    else
    {
        /* allocation error */
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_NOMEM);
    }

    return rc;
}

/*******************************************************************
    Allocte the interface of iAP2 library
********************************************************************/
S32 ippiAP2AllocateandUpdateData(   void        *dstp,      /* destination pointer */
                                    void        *srcp,      /* source pointer */
                                    U16         *dstc,      /* destination count pointer */
                                    U16         srcc,       /* source count */
                                    iAP2_Type   iAP2Type)   /* kind of copyed type */
{
    S32     rc = IAP2_OK;
    U32     ix = 0;
    size_t  tsize = 0;
    U8      **dstpc = (U8 **)dstp;
    /* size table for iAP2 library type */
    const   iAP2Type_t  ttable[] =
    {
    /*        Type enum       Byte size */
            { iAP2_int8,        1},
            { iAP2_uint8,       1},
            { iAP2_int16,       2},
            { iAP2_uint16,      2},
            { iAP2_int32,       4},
            { iAP2_uint32,      4},
            { iAP2_int64,       8},
            { iAP2_uint64,      8},
            { iAP2_blob,        IPP_LIBTYPE_VAR},
            { iAP2_utf8,        IPP_LIBTYPE_VAR},
            { iAP2_none,        IPP_LIBTYPE_STOP}
    };

    /* Parameter check */
    if((dstp == NULL) || (srcp == NULL) || (dstc == NULL))
    {

        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, 
                            IPOD_PLAYER_ERR_INVALID_PARAMETER, dstp, srcp, dstc);
        return IAP2_BAD_PARAMETER;  /* leave function */
    }

    /* Search for iAP2 library type */
    while(ttable[ix].size != IPP_LIBTYPE_STOP)
    {
        if(ttable[ix].type == iAP2Type)
        {
            tsize = ttable[ix].size;
            break;
        }
        ix++;
    }
    if(tsize == 0)
    {
        /* Type is not exist. */
        rc = IAP2_BAD_PARAMETER;
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iAP2Type);
    }
    else if(tsize == IPP_LIBTYPE_VAR)
    {
        if(iAP2Type == iAP2_blob)
        {
            /* Blob type processing */
            rc = ippiAP2AllocateandUpdateBlob(dstp, srcp, dstc, srcc);
        }
        else if(iAP2Type == iAP2_utf8)
        {
            /* UTF8 type processing */
            rc = ippiAP2AllocateandUpdateUtf8(dstp, srcp, dstc, srcc);
        }
        else
        {
            /* Type is not exist. */
            rc = IAP2_BAD_PARAMETER;
            IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, 
                                IPOD_PLAYER_ERR_INVALID_PARAMETER, tsize, iAP2Type);
        }
    }
    else
    {
        /* Normal data copy */
        *dstpc = (U8 *)calloc(srcc, tsize);         /* Allocate parameter buffer */
        if(*dstpc != NULL)
        {                         /* Is allcation OK ? */
            memcpy(*dstpc, srcp, srcc * tsize);     /* Copy source data to destination data buffer */
            *dstc = srcc;                           /* Copy source count to destination count buffer */
        }
        else
        {
            rc = IAP2_ERR_NO_MEM;                   /* allocation error */
            IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_NOMEM);
        }
        /* log output */
        MEM_DUMP((U8 *)*dstpc, *dstc * tsize);
    }
    

    return rc;
}


/***************************************************
    Convert iAP2 library error to iPodplayer error
****************************************************/
S32 ippiAP2RetConvertToiPP(S32 retCode)
{
    S32 rc = retCode;
    U32 tblsize = 0;
    U32 ix = 0;

    rc = IPOD_PLAYER_ERR_IPOD_CTRL_ERROR;
    tblsize = sizeof(g_ippiAP2_retcode_mapping_tbl) / sizeof(g_ippiAP2_retcode_mapping_tbl[0]);

    /* table search */
    for(ix = 0; ix < tblsize; ix++)
    {
        /* category is type of player */
        if(retCode == g_ippiAP2_retcode_mapping_tbl[ix].ctrl_ret)
        {
            /* change it to type of ipod  */
            rc = g_ippiAP2_retcode_mapping_tbl[ix].player_ret;
            break;
        }
    }

    return rc;
}

/*************************************************
    Unit of 100ms wait
**************************************************/
S32 ippiAP2Wait100ms(int timec)
{
    S32             rc = IPOD_PLAYER_OK;
    struct timespec waitTime;
    
    waitTime.tv_sec = 0;
    waitTime.tv_nsec = IPP_IAP2_WAIT_100MS * timec;

    rc = nanosleep(&waitTime, 0);
    if(rc != 0)
    {
        rc = IPOD_PLAYER_ERR_TIMER;
    }

    return rc;

}

S32 ippiAP2FileXferInit(IPOD_PLAYER_CORE_IAP2_FILE_XFER_LIST **list,
                        IPOD_PLAYER_CORE_IAP2_FILE_XFER_TYPE type,
                        U64 targetID,
                        U8 fileID)
{
    S32 rc = IPOD_PLAYER_ERROR;
    IPOD_PLAYER_CORE_IAP2_FILE_XFER_LIST *fileList = NULL;
    IPOD_PLAYER_CORE_IAP2_FILE_XFER_LIST *curList = NULL;

    fileList = *list;

    /* Check if entry already exists */
    for(; fileList != NULL; fileList = fileList->next)
    {
        if(fileList->info.fileID == fileID)
        {
            /* Found list */
            break;
        }
    }

    if(fileList == NULL)
    {
        if(type == IPOD_PLAYER_XFER_TYPE_UNKNOWN)
        {
            IPOD_DLT_WARN("New entry with type unknown. :*list=%p, type=%d, targetID=%llu, fileID=%u", *list, type, targetID, fileID);
        }
        else
        {
            IPOD_DLT_INFO("New :*list=%p, type=%d, targetID=%llu, fileID=%u", *list, type, targetID, fileID);
        }

        fileList = calloc(1, sizeof(*fileList));
        if(fileList == NULL)
        {
            IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_NOMEM, fileList);
            return IPOD_PLAYER_ERR_NOMEM;
        }

        fileList->info.fileID = fileID;
        fileList->info.type = type;
        fileList->info.targetID = targetID;
        fileList->info.status = IPOD_PLAYER_XFER_STATUS_NONE;

        /* List is top(root) list */
        if(*list == NULL)
        {
            fileList->next = NULL;
            fileList->prev = NULL;
            *list = fileList;
        }
        /* List is not top list */
        else
        {
            /* Move the list until tail */
            for(curList = *list; curList->next != NULL; curList = curList->next)
            {
                /* Nothing do */
            }

            /* Connect list to tail list */
            fileList->prev = curList;
            fileList->next = NULL;
            curList->next = fileList;
        }

        rc = IPOD_PLAYER_OK;
    }
    else
    {
        fileList->info.fileID = fileID;
        if(type != IPOD_PLAYER_XFER_TYPE_UNKNOWN)
        {
            IPOD_DLT_WARN("Already entry with type is not unknown. :*list=%p, type=%d, targetID=%llu, fileID=%u, status=%d", *list, type, targetID, fileID, fileList->info.status);

            fileList->info.type = type;
            fileList->info.targetID = targetID;
        }
        else
        {
            IPOD_DLT_VERBOSE("Already :*list=%p, type=%d, targetID=%llu, fileID=%u", *list, type, targetID, fileID);
        }
        rc = IPOD_PLAYER_OK;
    }

    return rc;
}

S32 ippiAP2FileXferDeinit(IPOD_PLAYER_CORE_IAP2_FILE_XFER_LIST **list,
                          U8 fileID)
{
    S32 rc = IPOD_PLAYER_ERROR;
    IPOD_PLAYER_CORE_IAP2_FILE_XFER_LIST *fileList = NULL;
    IPOD_PLAYER_CORE_IAP2_FILE_XFER_LIST *prevList = NULL;
    IPOD_PLAYER_CORE_IAP2_FILE_XFER_LIST *nextList = NULL;
    
    /* Parameter check */
    if((list == NULL) || (*list == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, list);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }

    IPOD_DLT_INFO("*list=%p, fileID=%u", *list, fileID);
    
    fileList = *list;
    
    /* Move list until tail list */
    for(; fileList!= NULL; fileList = fileList->next)
    {
        /* Check if there is file id in lists */
        if(fileList->info.fileID == fileID)
        {
            /* Found list */
            break;
        }
    }
    
    if(fileList != NULL)
    {
        prevList = fileList->prev;
        nextList = fileList->next;
        
        /* Free the allocated buffer */
        if(fileList->info.buf != NULL)
        {
            free(fileList->info.buf);
            fileList->info.buf = NULL;
        }
        
        /* Current list is not top or tail list */
        if((prevList != NULL) && (nextList != NULL))
        {
            /* Previous list of current list is connected to next list of current list */
            prevList->next = nextList;
            nextList->prev = prevList;
        }
        /* Current list is top list and next list exists */
        else if((prevList == NULL) && (nextList != NULL))
        {
            /* Top list is next list  */
            *list = nextList;
            /* Previos list is nothing */
            (*list)->prev = NULL;
        }
        /* Current list is tail and previous list exists */
        else if((prevList != NULL) && (nextList == NULL))
        {
            prevList->next = NULL;
        }
        /* Any other lists does not exist */
        else
        {
            /* Remove all list */
            *list = NULL;
        }
        
        free(fileList);
        rc = IPOD_PLAYER_OK;
    }
    
    return rc;
}

S32 ippiAP2FileXferDeinitAll(IPOD_PLAYER_CORE_IAP2_FILE_XFER_LIST **list)
{
    S32 rc = IPOD_PLAYER_ERROR;
    IPOD_PLAYER_CORE_IAP2_FILE_XFER_LIST *fileList = NULL;
    IPOD_PLAYER_CORE_IAP2_FILE_XFER_LIST *prevList = NULL;
    
    /* Parameter check */
    if((list == NULL) || (*list == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, list);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    IPOD_DLT_INFO("*list=%p", *list);

    fileList = *list;
    
    do
    {
        /*  */
        prevList = fileList->prev;
        if(fileList->info.buf != NULL)
        {
            free(fileList->info.buf);
            fileList->info.buf = NULL;
        }
        free(fileList);
        fileList = prevList;
    } while(fileList != NULL);
    
    rc = IPOD_PLAYER_OK;
    
    return rc;
}

IPOD_PLAYER_CORE_IAP2_FILE_XFER_INFO *ippiAP2FileXferGetInfo(IPOD_PLAYER_CORE_IAP2_FILE_XFER_LIST *list,
                                                             U8 fileID)
{
    IPOD_PLAYER_CORE_IAP2_FILE_XFER_INFO *info = NULL;
    IPOD_PLAYER_CORE_IAP2_FILE_XFER_LIST *fileList = NULL;
    
    fileList = list;

    //IPOD_DLT_INFO("[DBG]list=%p, fileID=%d", list, fileID);
    
    /* Move list until tail list */
    for(; fileList != NULL; fileList = fileList->next)
    {
        //IPOD_DLT_INFO("[DBG]fileList->info.fileID=%d, fileID=%d", fileList->info.fileID, fileID);
        /* Check if there is file id in the list */
        if(fileList->info.fileID == fileID)
        {
            info = &fileList->info;
            break;
        }
    }
    
    return info;
}

