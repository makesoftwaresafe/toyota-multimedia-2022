/*****************************************************************************
-  \file : usb-host_iap2_plugin.c
-  \version : $Id: iap2_cinemo_usb_host_plugin.c, v Exp $
-  \release : $Name:$
-  Contains the source code implementation for USB Host mode communication
-  i.e., the Accessory acts as gadget and the iOS device acts as host.
-  \component :
-  \author : Sundhar Asokan / RBEI / sundhar.asokan@in.bosch.com
-  \copyright (c) 2010 - 2013 Advanced Driver Information Technology.
-  This code is developed by Advanced Driver Information Technology.
-  Copyright of Advanced Driver Information Technology, Bosch, and DENSO.
-  All rights reserved.
*****************************************************************************/

#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <sys/mount.h>
#include <linux/usb/functionfs.h>
#include <pthread_adit.h>
#include <fcntl.h>           /* For O_* constants */
#include <sys/stat.h>    /* For mode constants */
#include <sys/ioctl.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/eventfd.h>

#include "iap2_dlt_log.h"
#include "iap2_cinemo_host_datacom.h"

#define  MAXFD(x, y)  ((x)>(y)?(x):(y))
#define MAX_PATH 256
#define IAP_PATH "ADITIAP:"
#define AUD_PATH ":ADITAUD:"

int ADITctli_dataport_select_fn(ctli_handle tphandle, ctli_dataport_transit* transit, unsigned int num_entries)
{
    tphandle    =   tphandle;
    transit     =   transit;
    num_entries =   num_entries;
    IAP2USBPLUGINDLTLOG(DLT_LOG_WARN,"ctli_dataport_select_fn not implemented ");
    return CTLI_ERROR_NOTIMPLEMENTED;
}
int ADITctli_dataport_send_fn(ctli_handle tphandle, unsigned short id, const void* psrc, unsigned int npsrc)
{
    tphandle    =   tphandle;
    id          =   id;
    psrc        =   psrc;
    npsrc       =   npsrc;
    IAP2USBPLUGINDLTLOG(DLT_LOG_WARN,"ctli_dataport_send_fn not implemented ");
    return CTLI_ERROR_NOTIMPLEMENTED;
}
int ADITctli_dataport_recv_fn(ctli_handle tphandle, unsigned short id, void* pdest, unsigned int npdest)
{
    tphandle    =   tphandle;
    id          =   id;
    pdest       =   pdest;
    npdest      =   npdest;
    IAP2USBPLUGINDLTLOG(DLT_LOG_WARN,"ctli_dataport_recv_fn not implemented ");
    return CTLI_ERROR_NOTIMPLEMENTED;
}
int ADITctli_dataport_cancel_fn(ctli_handle tphandle, unsigned short id)
{
    tphandle    =   tphandle;
    id          =   id;
    IAP2USBPLUGINDLTLOG(DLT_LOG_WARN,"ctli_dataport_cancel_fn not implemented ");
    return CTLI_ERROR_NOTIMPLEMENTED;
}
int ADITctli_dataport_enable_fn(ctli_handle tphandle, unsigned short id)
{
    tphandle    =   tphandle;
    id          =   id;
    IAP2USBPLUGINDLTLOG(DLT_LOG_WARN,"ctli_dataport_enable_fn not implemented ");
    return CTLI_ERROR_NOTIMPLEMENTED;
}
int ADITctli_dataport_get_param_fn(ctli_handle tphandle, unsigned short id, CTLI_DATAPORT_PARAM param_type, ctli_dataport_params* params)
{
    tphandle    =   tphandle;
    id          =   id;
    param_type  =   param_type;
    params      =   params;
    IAP2USBPLUGINDLTLOG(DLT_LOG_WARN,"ctli_dataport_get_param_fn not implemented ");
    return CTLI_ERROR_NOTIMPLEMENTED;
}
int ADITctli_dataport_set_param_fn(ctli_handle tphandle, unsigned short id, CTLI_DATAPORT_PARAM param_type, const ctli_dataport_params* params)
{
    tphandle    =   tphandle;
    id          =   id;
    param_type  =   param_type;
    params      =   params;
    IAP2USBPLUGINDLTLOG(DLT_LOG_WARN,"ctli_dataport_set_param_fn not implemented ");
    return CTLI_ERROR_NOTIMPLEMENTED;
}

int ADITctli_get_param_fn(ctli_handle tphandle, CTLI_PARAM param_type, ctli_params* params)
{
    tphandle    =   tphandle;
    if(NULL == params)
    {
        IAP2USBPLUGINDLTLOG(DLT_LOG_FATAL,"ctli_get_param_fn  invalid arguments");
        return CTLI_ERROR_ARGUMENTS;
    }
    switch(param_type)
    {
        case CTLI_PARAM_PROTOCOL:
        {
            params->protocol = CTLI_PARAM_PROTOCOL_USB;
            IAP2USBPLUGINDLTLOG(DLT_LOG_INFO,"params->protocol = CTLI_PARAM_PROTOCOL_USB  %d",param_type);
            break;
        }

        case CTLI_PARAM_STATE:
        {
            params->state = CTLI_PARAM_STATE_HOSTMODE;
            IAP2USBPLUGINDLTLOG(DLT_LOG_INFO,"params->state = CTLI_PARAM_STATE_HOSTMODE %d",param_type);
            break;
        }
        default :
            IAP2USBPLUGINDLTLOG(DLT_LOG_WARN,"invalid param_type %d",param_type);
            break;
    }
    return CTLI_NO_ERROR;
}
int ADITctli_set_param_fn(ctli_handle tphandle, CTLI_PARAM param_type, const ctli_params* params)
{
    tphandle    =   tphandle;
    param_type  =   param_type;
    params      =   params;
    IAP2USBPLUGINDLTLOG(DLT_LOG_INFO,"ctli_set_param_fn not applicable for current transport param_type %d ",param_type);
    return CTLI_ERROR_NOTIMPLEMENTED;
}

int iAP2USBHostCinemoAbort(ctli_handle tphandle)
{
    S32 rc = CTLI_NO_ERROR;
    int64_t data = 1;
    IPOD_IAP2_CINEMO_HOSTDEV_INFO* devinfo  =  (IPOD_IAP2_CINEMO_HOSTDEV_INFO*)tphandle;

    if(NULL == devinfo)
    {
        IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR,"iAP2USBHostCinemoAbort  invalid arguments");
        return CTLI_ERROR_ARGUMENTS;
    }
    IAP2USBPLUGINDLTLOG(DLT_LOG_INFO,"signaling to abort ");
    rc = write(devinfo->abort_fd, &data, sizeof(int64_t));
    if(rc < 0)
    {
        IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR,"Write to Abortfd failed rc = %d",rc);
    }
    return rc;
}

int iAP2USBHostCinemoReceive(ctli_handle tphandle, void* pdest, unsigned int npdest)
{
    IPOD_IAP2_CINEMO_HOSTDEV_INFO* devinfo  =  (IPOD_IAP2_CINEMO_HOSTDEV_INFO*)tphandle;
    S32 rc = CTLI_NO_ERROR;
    fd_set read_fds;
    S32 maxfd=0;
    FD_ZERO(&read_fds);
    FD_SET(devinfo->read_fd , &read_fds);
    FD_SET(devinfo->abort_fd , &read_fds);
    maxfd = MAXFD(devinfo->abort_fd, devinfo->read_fd);

    /*TBD: If driver has aborting mechanism, this can to be used*/
    rc = select(maxfd + 1, &read_fds, NULL, NULL, NULL );
    if (rc < 0)
    {
        IAP2USBPLUGINDLTLOG(DLT_LOG_FATAL,"select()returns error %d",rc);
        rc = CTLI_ERROR_IO;
    }
    else
    {
        if(FD_ISSET(devinfo->abort_fd, &read_fds))
        {
            IAP2USBPLUGINDLTLOG(DLT_LOG_INFO,"Abort fd triggered(on read)");
            /* Do not read the data since the signal is needed to abort subsequent calls of
             * iAP2USBHostCinemoReceive and iAP2USBHostCinemoSend */
            return CTLI_ERROR_CANCEL;
        }
        if(FD_ISSET(devinfo->read_fd, &read_fds))
        {
            rc = read(devinfo->read_fd, (char*)pdest, npdest);
            if (rc < 0)
            {
                rc = CTLI_ERROR_IO;
                IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR,"read ret =  %d | errno: %d %s",rc, errno, strerror(errno));
                if(errno == EAGAIN)
                {
                    /* Andreas: if read returns EAGAIN, resource temporarily unavailable try again */
                    rc = 0;
                }
            }
            else
            {
                IAP2USBPLUGINDLTLOG(DLT_LOG_DEBUG,"read %d bytes",rc);
                /*For initial read/write, DLT logs are not coming
                * so kept printf for debugging purpose*/
                printf("iAP2 Read %d  bytes\n",rc);
            }
        }
    }
    return rc;
}

int iAP2USBHostCinemoSend(ctli_handle tphandle, const void* psrc, unsigned int npsrc)
{
    IPOD_IAP2_CINEMO_HOSTDEV_INFO* devinfo  =  (IPOD_IAP2_CINEMO_HOSTDEV_INFO*)tphandle;
    int rc = CTLI_NO_ERROR;
    fd_set write_fds,read_fds;
    int maxfd=0;
    FD_ZERO(&read_fds);
    FD_ZERO(&write_fds);
    FD_SET(devinfo->write_fd , &write_fds);
    FD_SET(devinfo->abort_fd , &read_fds);
    maxfd = MAXFD(devinfo->abort_fd, devinfo->write_fd);

    /*TBD: If driver has aborting mechanism, this can to be used*/
    rc = select(maxfd + 1 , &read_fds, &write_fds, NULL, NULL );
    if (rc < 0)
    {
        IAP2USBPLUGINDLTLOG(DLT_LOG_INFO,"select()returns error %d ",rc);
        rc = CTLI_ERROR_IO;
    }
    else
    {
        if(FD_ISSET(devinfo->abort_fd, &read_fds))
        {
            IAP2USBPLUGINDLTLOG(DLT_LOG_INFO,"Abort fd triggered(on write)");
            /* Do not read the data since the signal is needed to abort subsequent calls of
             * iAP2USBHostCinemoReceive and iAP2USBHostCinemoSend */
            return CTLI_ERROR_CANCEL;
        }
        if(FD_ISSET(devinfo->write_fd, &write_fds))
        {
            rc = (S32) write(devinfo->write_fd, (void*) psrc, (size_t) npsrc);
            if (rc < 0)
            {
                IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR,"write ret =  %d | errno: %d %s",rc, errno, strerror(errno));
                S32 err = errno;
               /* Retaining the below error handling as in existing host plugin*/
               /* write() return EAGAIN if the iOS device is (temporary) not available. */
               if (err == EAGAIN)
               {
                   IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR,"write rc =  %d | errno: %d %s", rc, errno, strerror(errno));
                   /* we assume that the iOS device is no longer connected. */
                   rc = CTLI_ERROR_NOTCONNECTED;
               }
               else if (err == ETIME)
               {   /* SWGIII-9274 */
                   /* if write_fd was open with flag O_NONBLOCK and return with ETIME,
                   * and the patch in the g_ffs which writes with a timeout,
                   * we assume that we could not write to the device */
                   IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR,"write=%d, errno=ETIME", rc);
                   /* return IPOD_DATACOM_ERR_ABORT because write request was aborted by timer in g_ffs */
                   rc = CTLI_ERROR_CANCEL;
                }
                else
                {
                   /* another unexpected error occurred. */
                   IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR,"unexpected error occurred, errno=%d", err);
                   rc = CTLI_ERROR_IO;
                }
            }
            else
            {
                IAP2USBPLUGINDLTLOG(DLT_LOG_DEBUG,"Written %d  bytes",rc);
                /*For initial read/write, DLT logs are not coming
                * so kept printf for debugging purpose*/
                printf("iAP2 Written %d  bytes\n",rc);
            }
        }
    }
    return rc;
}

int iAP2USBHostCinemoDelete(ctli_handle tphandle)
{
    IAP2USBPLUGINDLTLOG(DLT_LOG_INFO,"Transport deleting ");
    S32 rc = CTLI_NO_ERROR;
    IPOD_IAP2_CINEMO_HOSTDEV_INFO* devinfo  =  (IPOD_IAP2_CINEMO_HOSTDEV_INFO*)tphandle;

    rc = close(devinfo->read_fd);
    IAP2USBPLUGINDLTLOG(DLT_LOG_INFO,"close read fd returns %d",rc);
    rc = close(devinfo->write_fd);
    IAP2USBPLUGINDLTLOG(DLT_LOG_INFO,"close write fd returns %d",rc);
    rc = close(devinfo->abort_fd);
    IAP2USBPLUGINDLTLOG(DLT_LOG_INFO,"close abort fd returns %d",rc);
    if( devinfo->AudioSource != NULL)
    {
        free(devinfo->AudioSource);
    }
    free(devinfo);
    IAP2USBPLUGINDLTLOG(DLT_LOG_INFO,"Transport deleted");
    DLT_UNREGISTER_CONTEXT(iAP2USBPluginCtxt);
    /* APP deregistration will be done in Application. Below is for test purpose only*/
    /* IAP2DEREGISTERAPPWITHDLT();*/
    return rc;
}

int iAP2USBHostCinemoCreate(const struct ctli_create_params* params, struct ctli_context* ctx)
{
    S32 rc = CTLI_NO_ERROR;
    U8 iappath[MAX_PATH];
    U8 ep1path[MAX_PATH];
    U8 ep2path[MAX_PATH];


    IAP2USBPLUGINDLTLOG(DLT_LOG_INFO, "ADITIAP iAP2USBHostCinemoCreate");
    DLT_REGISTER_CONTEXT_LL_TS(iAP2USBPluginCtxt, "CUSB", "CINEMO USB plugin logging", DLT_LOG_DEBUG, DLT_TRACE_STATUS_OFF);

    if((ctx != NULL) && (params != NULL))
    {
        /* Initialize the handle  */
        IPOD_IAP2_CINEMO_HOSTDEV_INFO* devinfo  =  calloc(1,sizeof(IPOD_IAP2_CINEMO_HOSTDEV_INFO));
        if (devinfo != NULL)
        {
            /*validate URL  Eg. ./SampleIAP -d -u iap://ADITIAP:/dev/ffs/:ADITAUD:hw:UAC2Gadget,0 */
            IAP2USBPLUGINDLTLOG(DLT_LOG_INFO,"The input URL is %s",params->szurl);
            U8 *aditiap = strstr(params->szurl,IAP_PATH);
            U8 *aditaud = strstr(params->szurl,AUD_PATH);
            U8 *src;
            S32 iap_length = 0;
            S32 aud_length = 0;

            if(aditiap != NULL)
            {
                src = aditiap+strlen(IAP_PATH);
                iap_length = aditaud-aditiap-strlen(IAP_PATH);
                if(iap_length>MAX_PATH)
                {
                    rc = CTLI_ERROR_ARGUMENTS;
                    IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR,"Path length exceeds Max_Path");
                }
                else
                {
                    strncpy(iappath,src,iap_length);
                    iappath[iap_length]='\0';
                    sprintf(ep1path,"%sep1",iappath);
                    sprintf(ep2path,"%sep2",iappath);
                }
            }
            else
            {
                IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR,"No IAP path given");
                return CTLI_ERROR_CONFIGURATION;
            }
            if((aditaud != NULL) && (rc == 0))
            {
                src = aditaud+strlen(AUD_PATH);

                aud_length = strnlen(src,MAX_PATH);
                if(aud_length<MAX_PATH)
                {
                    devinfo->AudioSource = calloc(aud_length , sizeof(U8) );
                    if(devinfo->AudioSource == NULL)
                    {
                        rc = CTLI_ERROR_ARGUMENTS;
                    }
                    else
                    {
                        strcpy(devinfo->AudioSource,src);
                    }
                    IAP2USBPLUGINDLTLOG(DLT_LOG_INFO,"devinfo->AudioSource : %s",devinfo->AudioSource);
                }
                else
                {
                    rc = CTLI_ERROR_ARGUMENTS;
                    IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR,"Audio length exceeds Max_Path");
                }
            }
            else
            {
                IAP2USBPLUGINDLTLOG(DLT_LOG_WARN,"Audio path not given so only iAP use case");
                devinfo->AudioSource = NULL;
            }

            devinfo->abort_fd = eventfd(0, 0);
            if(devinfo->abort_fd == -1)
            {
                IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR,"eventfd creation failed");
                return CTLI_ERROR_RESOURCES;
            }
            IAP2USBPLUGINDLTLOG(DLT_LOG_INFO,"opening write fd %s",ep1path);
            devinfo->write_fd = open( (const char*)ep1path,O_NONBLOCK | O_RDWR | O_CLOEXEC);
            if(devinfo->write_fd  < 0)
            {
                IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR,"opening write fd failed rc = %d ",devinfo->write_fd);
                return CTLI_ERROR_OPEN;
            }
            IAP2USBPLUGINDLTLOG(DLT_LOG_INFO,"opening read fd %s ",ep2path);
            devinfo->read_fd  = open( (const char*)ep2path,O_NONBLOCK | O_RDWR | O_CLOEXEC);
            if(devinfo->read_fd  < 0)
            {
                IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR,"opening read fd failed rc = %d ",devinfo->read_fd);
                return CTLI_ERROR_OPEN;
            }
            ctx->tphandle           = devinfo;
            ctx->protocol_version   = CINEMO_TRANSPORT_LIBRARY_PROTOCOL_VERSION;
            ctx->delete_transport   = iAP2USBHostCinemoDelete;
            ctx->send               = iAP2USBHostCinemoSend;
            ctx->receive            = iAP2USBHostCinemoReceive;
            ctx->abort              = iAP2USBHostCinemoAbort;
            ctx->get_param          = ADITctli_get_param_fn;
            ctx->set_param          = ADITctli_set_param_fn;
            ctx->audio_create       = Audio_Create_Custom;
            ctx->audio_delete       = Audio_Delete_Custom;
            ctx->audio_receive      = Audio_Receive_Custom;
            ctx->audio_abort        = Audio_Abort_Custom;
            ctx->audio_get_params   = Audio_Get_Params_Custom;
            ctx->audio_set_params   = Audio_Set_Params_Custom;
            ctx->dataport_select    = ADITctli_dataport_select_fn;
            ctx->dataport_send      = ADITctli_dataport_send_fn;
            ctx->dataport_recv      = ADITctli_dataport_recv_fn;
            ctx->dataport_cancel    = ADITctli_dataport_cancel_fn;
            ctx->dataport_enable    = ADITctli_dataport_enable_fn;
            ctx->dataport_get_param = ADITctli_dataport_get_param_fn;
            ctx->dataport_set_param = ADITctli_dataport_set_param_fn;
        }
        else
        {
            IAP2USBPLUGINDLTLOG(DLT_LOG_FATAL,"devinfo is NULL NO Memory");
            rc = CTLI_ERROR_RESOURCES;
        }
    }
    else
    {
        IAP2USBPLUGINDLTLOG(DLT_LOG_FATAL,"Invalid input parameters");
        rc = CTLI_ERROR_ARGUMENTS;
    }
    return rc;
}
