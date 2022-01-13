/****************************************************
 *  ipp_iap2_ctrlcfglist.c                           
 *  Created on: 2017/05/24 10:00:00                  
 *  Implementation of the Class ipp_iap2_ctrlcfglist 
 *  Original author: mando                           
 ****************************************************/
#include "pthread_adit.h"
#include "ipp_iap2_ctrlcfglist.h"
#include "ipp_iap2_common.h"
#include "ipp_iap2_observer.h"
#include <search.h>

struct element {
    struct element *forward;
    struct element *backward;
    IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg;
};

struct element *g_headElement = NULL;
struct element *g_tailElement = NULL;

static pthread_mutex_t g_ctrlCfgListMutex = PTHREAD_MUTEX_INITIALIZER;

void ippiAP2PrintCtrlCfgInfo()
{
    struct element *e = NULL;

    IPOD_DLT_INFO("***********\n");
    IPOD_DLT_INFO("g_headElement:%p\n", g_headElement);
    IPOD_DLT_INFO("g_tailElement:%p\n", g_tailElement);
    if(g_headElement != NULL)
    {
        e = g_headElement;
        do
        {
            IPOD_DLT_INFO("devPath:%s\n", e->iPodCtrlCfg->threadInfo->nameInfo.deviceName);
            e = e->forward;
        } while ((e != NULL) && (e != g_headElement));
    }
    IPOD_DLT_INFO("***********\n");
}

static struct element *ippiAP2SerchCtrlCfg(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg)
{
    struct element *e = NULL;

    if(g_headElement == NULL)
    {
        return NULL;
    }

    e = g_headElement;
    do
    {
        if(e->iPodCtrlCfg == iPodCtrlCfg)
        {
            return e;
        }
        e = e->forward;
    } while ((e != NULL) && (e != g_headElement));
    
    return NULL;
}

S32 ippiAP2InitCtrlCfgList()
{
    S32 rc = IPOD_PLAYER_OK;
    S32 rcm = 0;

    rcm = pthread_mutex_init(&g_ctrlCfgListMutex, NULL);
    if(rcm != 0)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rcm);
        return IPOD_PLAYER_ERROR;
    }

    g_headElement = NULL;
    g_tailElement = NULL;

    return rc;
}

S32 ippiAP2EnqueueToCtrlCfgList(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg)
{
    S32 rc = IPOD_PLAYER_OK;
    S32 rcm = 0;
    struct element *e = NULL;

    /* Parameter check */
    if(iPodCtrlCfg == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCtrlCfg);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }

    rcm = pthread_mutex_lock(&g_ctrlCfgListMutex);
    if(rcm != 0)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rcm);
        pthread_mutex_unlock(&g_ctrlCfgListMutex);

        return IPOD_PLAYER_ERROR;
    }
    e = ippiAP2SerchCtrlCfg(iPodCtrlCfg);
    if(e != NULL)
    {
        rcm = pthread_mutex_unlock(&g_ctrlCfgListMutex);
        if(rcm != 0)
        {
            IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rcm);
        }
        return IPOD_PLAYER_ERROR;
    }

    e = g_tailElement;
    g_tailElement = calloc(1, sizeof(struct element));
    if(g_tailElement == NULL)
    {
        g_tailElement = e;
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_NOMEM);
        rcm = pthread_mutex_unlock(&g_ctrlCfgListMutex);
        if(rcm != 0)
        {
            IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rcm);
        }
        return IPOD_PLAYER_ERR_NOMEM;
    }
    if(g_headElement == NULL)
    {
        g_headElement = g_tailElement;
    }
    g_tailElement->iPodCtrlCfg = iPodCtrlCfg;
    insque(g_tailElement, e);
    rcm = pthread_mutex_unlock(&g_ctrlCfgListMutex);
    if(rcm != 0)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rcm);
        return IPOD_PLAYER_ERROR;
    }

    return rc;
}

S32 ippiAP2DequeueFromCtrlCfgList(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg)
{
    S32 rc = IPOD_PLAYER_OK;
    S32 rcm = 0;
    struct element *e = NULL;

    /* Parameter check */
    if(iPodCtrlCfg == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCtrlCfg);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }

    rcm = pthread_mutex_lock(&g_ctrlCfgListMutex);
    if(rcm != 0)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rcm);
        pthread_mutex_unlock(&g_ctrlCfgListMutex);

        return IPOD_PLAYER_ERROR;
    }
    e = ippiAP2SerchCtrlCfg(iPodCtrlCfg);
    if(e == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERROR);
        rcm = pthread_mutex_unlock(&g_ctrlCfgListMutex);
        if(rcm != 0)
        {
            IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rcm);
        }
        return IPOD_PLAYER_ERROR;
    }
    if(e == g_headElement)
    {
        g_headElement = e->forward;
    }
    if(e == g_tailElement)
    {
        g_tailElement = e->backward;
    }
    remque(e);
    free(e);
    rcm = pthread_mutex_unlock(&g_ctrlCfgListMutex);
    if(rcm != 0)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rcm);
        return IPOD_PLAYER_ERROR;
    }

    return rc;
}

static S32 ippiAP2CompareSerialNum(U8 *srcNum, U8 *distNum)
{
    U32 i = 0;

    for(i = 0;i < IPOD_PLAYER_SERIAL_LEN_MAX;i++)
    {
        if(distNum[i] != srcNum[i])
        {
            return IPOD_PLAYER_ERROR;
        }
        if(srcNum[i] == '\0')
        {
            break;
        }
    }
    return IPOD_PLAYER_OK;
}

IPOD_PLAYER_CORE_IPODCTRL_CFG *ippiAP2SerchBySerialNumFromCtrlCfgList(U8 *serialNum)
{
    S32 rc = IPOD_PLAYER_OK;
    S32 rcm = 0;
    struct element *e = NULL;
    U8 *distNum = NULL;
    IPOD_PLAYER_CORE_IPODCTRL_CFG *rp = NULL;

    if(serialNum == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, serialNum);
        return NULL;
    }

    rcm = pthread_mutex_lock(&g_ctrlCfgListMutex);
    if(rcm != 0)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rcm);
        pthread_mutex_unlock(&g_ctrlCfgListMutex);
        return NULL;
    }
    if(g_headElement == NULL)
    {
        //IPOD_DLT_WARN("[DBG]g_headElement is NULL.");
        rp = NULL;
    }
    else
    {
        e = g_headElement;
        do
        {
            distNum = &(e->iPodCtrlCfg->threadInfo->nameInfo.deviceName[0]);
            rc = ippiAP2CompareSerialNum(serialNum, distNum); 
            if(rc == IPOD_PLAYER_OK)
            {
                rp = e->iPodCtrlCfg;
                break;
            }
            e = e->forward;
        } while ((e != NULL) && (e != g_headElement));
    }
    rcm = pthread_mutex_unlock(&g_ctrlCfgListMutex);
    if(rcm != 0)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rcm);
        rp = NULL;
    }

    return rp;
}
