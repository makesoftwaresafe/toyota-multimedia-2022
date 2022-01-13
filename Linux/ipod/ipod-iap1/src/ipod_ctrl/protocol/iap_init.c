/* -----------------------------------------------------------------------------
 * An invalid character is automatically inserted by cvs when the file is
 * commited. We can not do anything about it.
 * -----------------------------------------------------------------------------
 */
/**
* \file: iap_init.c
*
*
***************************************************************************** */

#include <adit_typedef.h>
#include "iap_init.h"
#include "ipodcommon.h"
#include "iap_common.h"
#include "iap_commands.h"
#include "iap_general.h"
#include "iap_callback.h"
#include "iap_transport_authentication.h"
#include "iap_transport_process_message.h"
#include "iap_util_func.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "iap_transport_os.h"
#include "ipodauth.h"
#include "iap1_dlt_log.h"
LOCAL void iPodCleariOSInfo(IPOD_IOS_INSTANCE *iPodiOSInst);
LOCAL S32 iPodSetiOSInfo(IPOD_IOS_INSTANCE *iPodiOSInst, IPOD_IOS_APP *iOSInfo, S8 numApps);
LOCAL S8 iPodSearchiOSInstance(IPOD_IOS_INSTANCE *iPodiOSInst, U8 *devicename, U8 iPodNum);

/**
 * \addtogroup InitCommands
 */
/*\{*/

/*!
 * \fn iPodInitConnection(void)
 * \par INPUT PARAMETERS
 * \par REPLY PARAMETERS
 * S32 ReturnCode - 
 * \li \c \b #IPOD_OK Completed successfully
 * \li \c \b #IPOD_ALREADY_CONNECTED iPod is already connected
 * \li \c \b #IPOD_ERR_NOMEM Insufficient memory
 * \li \c \b #IPOD_ERR_NOEXS Wait state released
 * \li \c \b #IPOD_ERROR Command failed
 * \li \c \b #IPOD_NOT_CONNECTED iPod is not connected
 * \par DESCRIPTION
 * This function initializes the connection to the iPod.
 */
S32 iPodInitConnection(void)
{
            S32             rc = IPOD_ERROR;
    const   IPOD_Cfg       *dcInfo = iPodGetDevInfo();
    IMPORT  IPOD_SEM_ID     g_semSenderId;
    IMPORT  IPOD_INSTANCE  *gp_iPodInst;

    if ((NULL != dcInfo) && (NULL == gp_iPodInst))
    {
        rc = iPodGetDevconfParameter();
        IAP1_INIT_LOG(DLT_LOG_DEBUG, "iPodGetDevconfParameter() returns : rc = %d", rc);

        if(rc == IPOD_OK)
        {
            rc = iPodTrnspInitPlugins();
        }

        if (IPOD_OK == rc)
        {
            rc = iPodCreateWorkerTask();
            if (IPOD_OK == rc)
            {
                gp_iPodInst = (IPOD_INSTANCE*)malloc(sizeof(IPOD_INSTANCE) * (U32)dcInfo[IPOD_DC_INST_COUNT].para.val);
                if (NULL != gp_iPodInst)
                {
                    memset(gp_iPodInst, 0, sizeof(IPOD_INSTANCE) * (U32)dcInfo[IPOD_DC_INST_COUNT].para.val);

                    if (g_semSenderId == 0)
                    {
                        rc = iPodOSCreateSemaphore(dcInfo[IPOD_DC_SEM_SENDER].para.p_val, &g_semSenderId);
                    }

                    if (IPOD_OK != rc)
                    {
                        free(gp_iPodInst);
                        gp_iPodInst = NULL;
                    }
                }
                else
                {
                    rc = IPOD_ERR_NOMEM;
                    IAP1_INIT_LOG(DLT_LOG_ERROR, "No Memory iPod Instance is NULL");
                }

                if (IPOD_OK != rc)
                {
                    iPodDeleteWorkerTask();
                    (void)iPodTrnspDeinitPlugins();
                }
            } /* worker task created */
            if (IPOD_OK != rc)
            {
                IAP1_INIT_LOG(DLT_LOG_ERROR, "Failed in Creating worker task");
                iPodFreeDevconfParameter();
            }
        } /* if iPodGetDevconfParameter */
    } /* (NULL != dcInfo) && (NULL == gp_iPodInst) */
    else
    {
        IAP1_INIT_LOG(DLT_LOG_ERROR, "dcInfo = %p gp_iPodInst = %p",dcInfo,gp_iPodInst);
    }
    return rc;


}
/*!
 * \fn iPodInitAccessoryConnection(IPOD_ACC_INFO_CONFIGURATION acc_info)
 * \par INPUT PARAMETERS
 * acc_info - Accessory Info from MC application
 * \par REPLY PARAMETERS
 * S32 ReturnCode -
 * 
 * \par DESCRIPTION
 * This function receives the accessory information from mediaplayer.
 */

S32 iPodInitAccessoryConnection(IPOD_ACC_INFO_CONFIGURATION acc_info)
{
    S32 rc = IPOD_ERROR;
    IPOD_Cfg *dcInfo = (IPOD_Cfg *)iPodGetDevInfo();

    if(dcInfo != NULL)
    {
        if ((dcInfo[IPOD_DC_ACCINFO_NAME].para.p_val != NULL) &&
            (dcInfo[IPOD_DC_HW_VER].para.p_val != NULL) &&
            (dcInfo[IPOD_DC_FW_VER].para.p_val != NULL) &&
            (dcInfo[IPOD_DC_MAN].para.p_val != NULL) &&
            (dcInfo[IPOD_DC_MODEL].para.p_val != NULL) &&
            (dcInfo[IPOD_DC_SERIAL].para.p_val != NULL)
           )
        {
            rc = IPOD_OK;
        }
        if ((acc_info.Name != NULL) && (rc == IPOD_OK))
        {
            (void)strncpy((VP)dcInfo[IPOD_DC_ACCINFO_NAME].para.p_val, (const char*)acc_info.Name, IPOD_CFG_STR_MAX);
            dcInfo[IPOD_DC_ACCINFO_NAME].para.p_val[IPOD_CFG_STR_MAX-1] = '\0';
        }
        if ((rc == IPOD_OK) &&
            ((acc_info.Hardware_version.Major_Number != 0) ||
            (acc_info.Hardware_version.Minor_Number!= 0) ||
            (acc_info.Hardware_version.Revision_Number!= 0))
           )
        {
                  ((S32 *)(VP)dcInfo[IPOD_DC_HW_VER].para.p_val)[0] = (S32)acc_info.Hardware_version.Major_Number;
            ((S32 *)(VP)dcInfo[IPOD_DC_HW_VER].para.p_val)[1] = (S32)acc_info.Hardware_version.Minor_Number;
                  ((S32 *)(VP)dcInfo[IPOD_DC_HW_VER].para.p_val)[2] = (S32)acc_info.Hardware_version.Revision_Number;
        }
        if ((rc == IPOD_OK) &&
            ((acc_info.Software_version.Major_Number != 0) ||
            (acc_info.Software_version.Minor_Number!= 0) ||
            (acc_info.Software_version.Revision_Number!= 0))
           )
        {
                  ((S32 *)(VP)dcInfo[IPOD_DC_FW_VER].para.p_val)[0] = (S32)acc_info.Software_version.Major_Number;
            ((S32 *)(VP)dcInfo[IPOD_DC_FW_VER].para.p_val)[1] = (S32)acc_info.Software_version.Minor_Number;
            ((S32 *)(VP)dcInfo[IPOD_DC_FW_VER].para.p_val)[2] = (S32)acc_info.Software_version.Revision_Number;
        }
        if((acc_info.Manufacturer != NULL) && rc == IPOD_OK)
        {
            (void)strncpy((VP)dcInfo[IPOD_DC_MAN].para.p_val, (const char*)acc_info.Manufacturer, IPOD_CFG_STR_MAX);
            dcInfo[IPOD_DC_MAN].para.p_val[IPOD_CFG_STR_MAX-1] = '\0';
        }
        if ((acc_info.ModelNumber != NULL) && (rc == IPOD_OK))
        {
            (void)strncpy((VP)dcInfo[IPOD_DC_MODEL].para.p_val, (const char*)acc_info.ModelNumber, IPOD_CFG_STR_MAX);
            dcInfo[IPOD_DC_MODEL].para.p_val[IPOD_CFG_STR_MAX-1] = '\0';
        }
        if ((acc_info.SerialNumber != NULL) && (rc == IPOD_OK))
        {
            (void)strncpy((VP)dcInfo[IPOD_DC_SERIAL].para.p_val, (const char*)acc_info.SerialNumber, IPOD_CFG_STR_MAX);
            dcInfo[IPOD_DC_SERIAL].para.p_val[IPOD_CFG_STR_MAX-1] = '\0';
        }
    }
    else
    {
        rc = IPOD_ERR_PAR;
        IAP1_INIT_LOG(DLT_LOG_ERROR, "dcInfo is NULL");
    }

    return rc;
}

/*!
 * \fn iPodInitDeviceConnection(U8* devicename, IPOD_CONNECTION_TYPE connection)
 * \par INPUT PARAMETERS
 * devicename - name of the device
 * IPOD_CONNECTION_TYPE connection - connection medium, where the iPod is connected
 * (e.g. IPOD_USB_HOST_CONNECT for USB or IPOD_BT_CONNECT for Bluetooth)
 * \par REPLY PARAMETERS
 * S32 ID of the Apple device or
 * S32 ReturnCode - 
 * \li \c \b #IPOD_OK Completed successfully
 * \li \c \b #IPOD_ALREADY_CONNECTED iPod is already connected
 * \li \c \b #IPOD_ERR_NOMEM Insufficient memory
 * \li \c \b #IPOD_ERR_NOEXS Wait state released
 * \li \c \b #IPOD_ERROR Command failed
 * \li \c \b #IPOD_NOT_CONNECTED iPod is not connected
 * \par DESCRIPTION
 * This function initializes the connection to the iPod.
 * Name of the device could be the serial number of the Apple device
 * or the name of the device in Linux where it is connected e.g. /dev/usb/hiddev0
 */
S32 iPodInitDeviceConnection(U8* devicename, IPOD_CONNECTION_TYPE connection)
{
    S32 rc = IPOD_ERROR;

    if(devicename != NULL)
    {
        rc = iPodTransportInitConnection(devicename, connection);
        IAP1_INIT_LOG(DLT_LOG_DEBUG, "iPodTransportInitConnection() returns : rc = %d",rc);
    }
    else
    {
        rc = IPOD_BAD_PARAMETER;
        IAP1_INIT_LOG(DLT_LOG_ERROR, "devicename is NULL");
    }
    
    return rc;
}

/*!
 * \fn iPodDisconnectDevice(U32 iPodID)
 * \par INPUT PARAMETERS
* U32 iPodID - ID of the Apple device<br>
*  \par REPLY PARAMETERS
 * S32 ReturnCode -
 * \li \c \b #IPOD_OK Completed successfully
 * \li \c \b #IPOD_ALREADY_CONNECTED iPod is already connected
 * \li \c \b #IPOD_ERR_NOMEM Insufficient memory
 * \li \c \b #IPOD_ERR_NOEXS Wait state released
 * \li \c \b #IPOD_ERROR Command failed
 * \li \c \b #IPOD_NOT_CONNECTED iPod is not connected
 * \par DESCRIPTION
 * This function deinitializes the connection to the iPod indicated by iPodID.
 */
void iPodDisconnectDevice(U32 iPodID)
{
    iPodTransportDisconnect(iPodID);
}



/*!
 * \fn iPodDisconnect(void)
 * \par INPUT PARAMETERS
 * \par REPLY PARAMETERS
 * \par DESCRIPTION
 * This function disconnects the iPod.
 */
void iPodDisconnect(void)
{
    S32 ret = IPOD_OK;
    S32 i = 0;
    const IPOD_Cfg* dcInfo = iPodGetDevInfo();
    IMPORT IPOD_SEM_ID g_semSenderId;
    IMPORT IPOD_INSTANCE* gp_iPodInst;

    for (i = 1; i <= dcInfo[IPOD_DC_INST_COUNT].para.val; i++)
    {
        iPodDisconnectDevice(i);
    }
    iPodDeleteWorkerTask();

    (void)iPodWaitForSenderSemaphore();
    (void)iPodTrnspDeinitPlugins();

    if(gp_iPodInst != NULL)
    {
        free(gp_iPodInst);
        gp_iPodInst = NULL;
    }

    (void)iPodSigSenderSemaphore();

    ret = iPodOSDeleteSemaphore(g_semSenderId, dcInfo[IPOD_DC_SEM_SENDER].para.p_val);
    if (ret == IPOD_OK)
    {
        g_semSenderId = 0;
    }
    
    iPodFreeDevconfParameter();
}

/*\}*/


/*!
 * \fn iPodSwitchAudioOutput(U32 iPodID, IPOD_SWITCH_AUDIO_OUT switchAudio)
 * \par INPUT PARAMETERS
 * U32 iPodID - ID of the Apple device<br>
 * \par REPLY PARAMETERS
 * S32 ReturnCode -
    if identify success, callback attach ok.
 * \par DESCRIPTION
 * This function use swtich analog/digital audio.<br>
 * when this function use, please change the status to stop or pause.<br>
 * If switchAudio sets IPOD_SWITCH_LINE_OUT and reconnect iPod, iPod is recognized as analog.<br>
 * If switchAudio sets IPOD_SWITCH_DIGITAL and reconnect iPod, iPod is recognized as digital.<br>
 * If switchAUdio sets IPOD_SWITCH_RESET and reconnect iPod, iPod is recognized as default setting.<br>
 */
void iPodSwitchAudioOutput(U32 iPodID, IPOD_SWITCH_AUDIO_OUT switchAudio)
{
    IPOD_INSTANCE* iPodHndl = iPodGetHandle(iPodID);

    if(iPodHndl != NULL)
    {
        iPodHndl->audioSwitch = switchAudio;
        if(iPodHndl->isAPIReady != FALSE)
        {
            iPodHndl->isAPIReady = FALSE;
            iPodWorkerSetForEvent(iPodHndl, IPOD_WORKER_REIDENTIFY);
        }
    }
}


/*!
 * \fn iPodRequestIdentify(U32 iPodID)
 * \par INPUT PARAMETERS
 * \par REPLY PARAMETERS
 * \par DESCRIPTION
 * This function is used to identy the iPod again.<br>
 * This function does nothing during authentication process.<br>
 * \note when this function is called, selected device change to identified device.
 */
void iPodRequestIdentify(U32 iPodID)
{
    IPOD_INSTANCE* iPodHndl = NULL;
    
    /* have to get handle this way to avoid race condition with MC application*/
    iPodHndl = iPodGetHandle(iPodID);
    if(iPodHndl != NULL)
    {
        iPodReidentify(iPodHndl);
    }
}

void iPodReidentify(IPOD_INSTANCE* iPodHndl)
{
        iPodHndl->isAPIReady = FALSE;
        iPodSetStartIDPS(iPodHndl, FALSE);
        iPodWorkerIdentify(iPodHndl,TRUE);
}

LOCAL void iPodCleariOSInfo(IPOD_IOS_INSTANCE *iPodiOSInst)
{
    U8 count = 0;
    
    if(iPodiOSInst != NULL)
    {
        /* Check whether application info memory is allocated or not. */
        if(iPodiOSInst->appInfo != NULL)
        {
            for(count = 0; count < iPodiOSInst->numApps; count++)
            {
                /* Check whether Bundle SeedID memory is allocated or not */
                if(iPodiOSInst->appInfo[count].protocol != NULL)
                {
                    free(iPodiOSInst->appInfo[count].protocol);
                    iPodiOSInst->appInfo[count].protocol = NULL;
                }
                
                /* Check whether Bundle SeedID memory is allocated or not */
                if(iPodiOSInst->appInfo[count].bundle != NULL)
                {
                    free(iPodiOSInst->appInfo[count].bundle);
                    iPodiOSInst->appInfo[count].bundle = NULL;
                }

                /* Metadata value sets to 0. */
                iPodiOSInst->appInfo[count].metaData = 0;
            }
            
            iPodiOSInst->numApps = 0;
            free(iPodiOSInst->appInfo);
            iPodiOSInst->appInfo = NULL;
        }
    }
}


LOCAL S32 iPodSetiOSInfo(IPOD_IOS_INSTANCE *iPodiOSInst, IPOD_IOS_APP *iOSInfo, S8 numApps)
{
    S32 rc = IPOD_OK;
    U8 count = 0;
    
    /* Parameter check */
    if((iPodiOSInst == NULL) || (iOSInfo == NULL) || (numApps < 0))
    {
        rc = IPOD_BAD_PARAMETER;
        IAP1_INIT_LOG(DLT_LOG_ERROR, "Bad Parameter - iPodiOSInst = %p, iOSInfo = %p numApps = %d",iPodiOSInst,iOSInfo,numApps);
    }
    else
    {
        /* Set the application number */
        iPodiOSInst->numApps = (U8)numApps;

        if(iPodiOSInst->appInfo == NULL)
        {
            /* Allocated the memory to set the application info */
            iPodiOSInst->appInfo = calloc(sizeof(IPOD_IOS_APP), (U32)(U8)numApps);
            if(iPodiOSInst->appInfo == NULL)
            {
                rc = IPOD_ERR_NOMEM;
                IAP1_INIT_LOG(DLT_LOG_ERROR, "No Memory appInfo is NULL");
            }
        }
        else
        {
            rc = IPOD_BAD_PARAMETER;
            IAP1_INIT_LOG(DLT_LOG_ERROR, "appInfo is NULL");
        }
        
        /* Set the application info */
        for(count = 0; (count < numApps) && (rc == IPOD_OK); count++)
        {
            if((iOSInfo[count].protocol == NULL) || (iOSInfo[count].bundle == NULL))
            {
                rc = IPOD_ERROR;
                IAP1_INIT_LOG(DLT_LOG_ERROR, "iOSInfo[count].protocol = %p, iOSInfo[count].bundle = %p",iOSInfo[count].protocol,iOSInfo[count].bundle);
            }
            else
            {
                if(iPodiOSInst->appInfo[count].protocol == NULL)
                {
                    /* Allocated the memory to set the Protocol String. */
                    iPodiOSInst->appInfo[count].protocol = (U8 *)calloc(strlen((VP)iOSInfo[count].protocol) + 1, sizeof(U8));
                }
                else
                {
                    rc = IPOD_ERROR;
                    IAP1_INIT_LOG(DLT_LOG_ERROR, "protocol string of appInfo is not NULL");
                }
                
                if((iPodiOSInst->appInfo[count].bundle == NULL) && (rc == IPOD_OK))
                {
                    /* Allocated the memory to set the Bundle SeedID. */
                    iPodiOSInst->appInfo[count].bundle = (U8 *)calloc(strlen((VP)iOSInfo[count].bundle) + 1, sizeof(U8));
                }
                else
                {
                    rc = IPOD_ERROR;
                    IAP1_INIT_LOG(DLT_LOG_ERROR, "iPodiOSInst->appInfo[count].bundle = %p",iPodiOSInst->appInfo[count].bundle);
                }

                /* Check the memory allocation. */
                if((iPodiOSInst->appInfo[count].protocol != NULL) &&
                   (iPodiOSInst->appInfo[count].bundle != NULL) &&
                   (rc == IPOD_OK))
                {
                    /* Copy the Protocol String. */
                    memcpy((VP)iPodiOSInst->appInfo[count].protocol, iOSInfo[count].protocol, strlen((VP)iOSInfo[count].protocol) + 1);

                    /* Copy the Bundle SeedID. */
                    memcpy((VP)iPodiOSInst->appInfo[count].bundle, iOSInfo[count].bundle, strlen((VP)iOSInfo[count].bundle) + 1);

                    /* Set the metadata status. */
                    iPodiOSInst->appInfo[count].metaData = iOSInfo[count].metaData;
                }
                else
                {
                    rc = IPOD_ERR_NOMEM;
                    IAP1_INIT_LOG(DLT_LOG_ERROR, "No Memory - Protocol string = %p and Bundle Id = %p",iPodiOSInst->appInfo[count].protocol,iPodiOSInst->appInfo[count].bundle);
                }
            }
        }
        
        /* Set iOS info is failed. */
        if(rc != IPOD_OK)
        {
            IAP1_INIT_LOG(DLT_LOG_ERROR, "Set iOS info failed");
            /* Clear the allocated memory. */
            iPodCleariOSInfo(iPodiOSInst);
        }
    }
    
    return rc;
}

LOCAL S8 iPodSearchiOSInstance(IPOD_IOS_INSTANCE *iPodiOSInst, U8 *devicename, U8 iPodNum)
{
    S8 deviceID = -1;
    U8 count = 0;
    
    if((iPodiOSInst != NULL) && (devicename != NULL))
    {
        /* Serch whether there is iPodiOS Instance. */
        for(count = 0; count < iPodNum; count++)
        {
            /* Instance does not exist. */
            if(iPodiOSInst[count].name[0] == '\0')
            {
                /* deviceID is not set */
                if(deviceID < 0)
                {
                    /* Most young number is set */
                    deviceID = (S8)count;
                }
            }
            /* Instance exists */
            else
            {
                /* Same device name was found. */
                if(strncmp((VP)iPodiOSInst[count].name, (VP)devicename, sizeof(iPodiOSInst[count].name)) == 0)
                {
                    deviceID = (S8)count;
                }
            }
        }
    }
    
    return deviceID;
}

/*!
 * \fn iPodSetConfiOSApp(U8 *devicename, IPOD_IOS_APP *iOSInfo, S8 numApps)
 * \par INPUT PARAMETERS
 * U8 *devicename - This is a name of device(iPod) setting.
 * #IPOD_IOS_APP *iOSInfo - This is structure of iOS information
 * S8 numApps - This is number that set iOS information.
 * \par REPLY PARAMETERS
 * S32 ReturnCode -
 * \par DESCRIPTION
 * This function is used to set the iOS information.<br>
 * If iOS information is set by this function, iPod is set the iOS information in authentication process.<br>
 * If parameter of iOSInfo is set to NULL, this fucntion clears the iOS information that device name is having.
 * If parameter of numApps is set to negative value, this function clears the ALL iOS information.<br>
 * This function only sets the iOS information, not do the identify.
 * \note if return code is IPOD_ERROR, previous setting may be cleared.<br>
 * \note This function must be called before the iPodInitDeviceConnection for a particular device. 
 * If the device has already been initialized, this function will fail.
 */
S32 iPodSetConfiOSApp(U8 *devicename, IPOD_IOS_APP *iOSInfo, S8 numApps)
{
    S32 rc = IPOD_OK;
    U8 count = 0;
    IMPORT IPOD_IOS_INSTANCE *gp_iPodiOSInst;
    static U8 iPodNum = 0;
    S16 deviceID = -1;
    IPOD_Cfg *dcInfo = iPodGetDevInfo();

    /* Confirm the parameter. */
    if((devicename != NULL) && (numApps >= 0))
    {
        /* Get Instance number when this function is called for the first time. */
        if(iPodNum == 0)
        {
            /* It gets only once */
            if(dcInfo != NULL)
            {
                iPodNum = dcInfo[IPOD_DC_INST_COUNT].para.val;
            }
            else
            {
                rc = IPOD_ERR_NOEXS;
                IAP1_INIT_LOG(DLT_LOG_ERROR, "dcInfo is NULL");
            }
        }
        
        if(rc == IPOD_OK)
        {
            /* Add the iOS configuration */
            if((iOSInfo != NULL)  && (numApps > 0))
            {
                /* This function is called for the first time. */
                if(gp_iPodiOSInst == NULL)
                {
                    /* Allocate the memory to save the iOS information for each device(iPod). */
                    gp_iPodiOSInst = (IPOD_IOS_INSTANCE *)calloc(iPodNum, sizeof(IPOD_IOS_INSTANCE));
                    if(gp_iPodiOSInst == NULL)
                    {
                        rc = IPOD_ERR_NOMEM;
                        IAP1_INIT_LOG(DLT_LOG_ERROR, "No Memory - iPodiOSInstance is NULL");
                    }
                }

                if(rc == IPOD_OK)
                {
                    rc = iPodCheckConnected(devicename);
                    IAP1_INIT_LOG(DLT_LOG_DEBUG, "iPodCheckConnected() returns : rc = %d",rc);
                    if (IPOD_OK == rc)
                    {
                        deviceID = iPodSearchiOSInstance(gp_iPodiOSInst, devicename, iPodNum);
                        IAP1_INIT_LOG(DLT_LOG_DEBUG, "iPodSearchiOSInstance() returns : deviceID = %d",deviceID);
                        if(deviceID >= 0)
                        {
                            /* Device name did not exist. */
                            if(gp_iPodiOSInst[deviceID].name[0] == '\0')
                            {
                                /* Copy the device name. */
                                strncpy((char*)gp_iPodiOSInst[deviceID].name, (char*)devicename, sizeof(gp_iPodiOSInst[deviceID].name));
                                gp_iPodiOSInst[deviceID].name[sizeof(gp_iPodiOSInst[deviceID].name) - 1] = '\0';
                            }
                            /* Device  exists */
                            else
                            {
                                /* Clear the previous application info. */
                                iPodCleariOSInfo(&gp_iPodiOSInst[deviceID]);
                            }

                            /* Set the application info. */
                            rc = iPodSetiOSInfo(&gp_iPodiOSInst[deviceID], iOSInfo, numApps);
                        }
                        else
                        {
                            rc = IPOD_MAX_CONNECT;
                            IAP1_INIT_LOG(DLT_LOG_ERROR, "Maximum Number of connections already reached");

                        }
                    }
                }

            }
            /* Clear the iOS configuration */
            else
            {
                if ((iOSInfo == NULL) && (gp_iPodiOSInst != NULL))
                {
                    /* Serch the devicename instance. */
                    for(count = 0; count < iPodNum; count++)
                    {
                        /* Compare the devicename and instance name. */
                        if (strncmp((VP)gp_iPodiOSInst[count].name, (VP)devicename, sizeof(gp_iPodiOSInst[count].name)) == 0)
                        {
                            /* Clear the name to 0. */
                            memset(gp_iPodiOSInst[count].name, 0, sizeof(gp_iPodiOSInst[count].name));
                            
                            /* Clear the iOS Info. */
                            iPodCleariOSInfo(&gp_iPodiOSInst[count]); 
                        }
                    }
                }
            }
        }
    }
    /* Clear all instance. */
    else if(numApps < 0)
    {
        if(gp_iPodiOSInst != NULL)
        {
            /* Clear all iOS info. */
            for(count = 0; count < iPodNum; count++)
            {
                    /* Clear the iOS Info */
                    iPodCleariOSInfo(&gp_iPodiOSInst[count]);
            }

            /* Free iOSInstance */
            free(gp_iPodiOSInst);
            gp_iPodiOSInst = NULL;
        }
        /* Clear iPod number */
        iPodNum = 0;
    }
    else
    {
        rc = IPOD_BAD_PARAMETER;
        IAP1_INIT_LOG(DLT_LOG_ERROR, "Bad Parameter devicename = %p, numApps = %d",devicename,numApps);
    }
    
    return rc;
}
