/**
 * \file: pai_com_server.c
 *
 *  IPC server for communicate with PulseAudio and applicaiton process
 *
 * \component:  IPC server
 *
 * \author: ADIT
 *
 * \copyright: (c) 2003 - 2011 ADIT Corporation
 *
 *
 */

#include "ipp_audio_common.h"
#include "gst_com_server_i.h"

int gst_com_create_ipodbin(IPOD_AUDIO_COM_INFO *gstInfo, U8 *srcName, U8 *sinkName)
{
    int rc = IPP_AUDIO_OK;
    gboolean bool = FALSE;
    
    if((gstInfo== NULL) || (srcName == NULL))
    {
        return IPP_AUDIO_ERROR;
    }
    
    /* Create Pipeline element */
    if(gstInfo->pipeElement == NULL)
    {
        gstInfo->pipeElement = gst_pipeline_new(GST_PIPELINE_NAME);
    }
    
    if(gstInfo->pipeElement != NULL)
    {
        /* Create source element. It uses the alsasrc plugin */
        gstInfo->srcElement = gst_element_factory_make(GST_SRC_ELEMENT_PLUGIN, GST_SRC_ELEMENT_NAME);
        if(gstInfo->srcElement != NULL)
        {
            /* Set the card number */
            snprintf((char *)gstInfo->card_name, sizeof(gstInfo->card_name), "hw:%c", srcName[3]);
            /* Set the property of alsasrc */
#ifdef IPOD_ARCH_ARM
            g_object_set(G_OBJECT(gstInfo->srcElement), GST_SRC_PROP_DEVICE, gstInfo->card_name, NULL);
#else
            g_object_set(G_OBJECT(gstInfo->srcElement), GST_SRC_PROP_DEVICE, gstInfo->card_name, "latency-time", 128000, "buffer-time" , 512000, NULL);
#endif /* IPOD_ARCH_ARM */
        }
        else
        {
            rc = IPP_AUDIO_ERROR;
        }
    }
    else
    {
        rc = IPP_AUDIO_ERROR;
    }
    
    /* Create volume element */
    if(rc == IPP_AUDIO_OK)
    {
        gstInfo->volumeElement = gst_element_factory_make(GST_VOLUME_ELEMENT_PLUGIN, GST_VOLUME_ELEMENT_NAME);
        if(gstInfo->volumeElement != NULL)
        {
            g_object_get(G_OBJECT(gstInfo->volumeElement), GST_VOLUME_PROP_VOLUME, &gstInfo->volume, NULL);
        }
        else
        {
            rc = IPP_AUDIO_ERROR;
        }
    }
    
    if(rc == IPP_AUDIO_OK)
    {
        /* Create sink element. It uses the autoaudiosink. */
        gstInfo->sinkElement = gst_element_factory_make(GST_SINK_ELEMENT_PLUGIN, GST_SINK_ELEMENT_NAME);
        if(gstInfo->sinkElement != NULL)
        {
            /* Set the property of alsasink (set device name for alsasink) */
#ifdef IPOD_ARCH_ARM
            g_object_set(G_OBJECT(gstInfo->sinkElement), GST_SINK_PROP_DEVICE, (char *)sinkName, NULL);
#else
            g_object_set(G_OBJECT(gstInfo->sinkElement), GST_SINK_PROP_DEVICE, (char *)sinkName, "latency-time", 32000, "buffer-time", 512000, "sync", 0, NULL);
#endif /* IPOD_ARCH_ARM */

            /* Create capsfilter element */
            gstInfo->capsElement = gst_element_factory_make(GST_CAPS_ELEMENET_PLUGIN, GST_CAPS_ELEMENT_NAME);
            if(gstInfo->capsElement != NULL)
            {
                gstInfo->caps = gst_caps_new_simple(GST_CAPS_PROP_SOURCE, GST_CAPS_PROP_KEU_RATE, G_TYPE_INT, gstInfo->rate, NULL);
                if(gstInfo->caps == NULL)
                {
                    rc = IPP_AUDIO_ERROR;
                }
                else
                {
                    g_object_set(G_OBJECT(gstInfo->capsElement), GST_CAPS_RPROP_KEY_CAPS, gstInfo->caps, NULL);
                }
            }
            else
            {
                rc = IPP_AUDIO_ERROR;
            }
        }
        else
        {
            rc = IPP_AUDIO_ERROR;
        }
    }
    
    if(rc == IPP_AUDIO_OK)
    {
        /* Add the three element to pipeline */
        /* PRQA: Lint Message 826: This conversion uses the macro of Gstreamer and this is correct way to use gst_bin_add_many. So this case is correct*/
        gst_bin_add_many(GST_BIN(gstInfo->pipeElement), gstInfo->srcElement, gstInfo->capsElement, gstInfo->volumeElement, gstInfo->sinkElement, NULL); /*lint !e826 */
        gst_caps_unref(gstInfo->caps);
        /* Link the three element. Like a alsasrc -> resample -> autoaudiosink */
        bool = gst_element_link_many(gstInfo->srcElement, gstInfo->capsElement, gstInfo->volumeElement, gstInfo->sinkElement, NULL);
        if(bool == FALSE)
        {
            rc = IPP_AUDIO_ERROR;
        }
    }
    
    return rc;
}

int gst_com_change_state(IPOD_AUDIO_COM_INFO *gstInfo, GstState gstState)
{
    int rc = IPP_AUDIO_OK;
    int count = 0;
    GstState oldState = GST_STATE_VOID_PENDING;
    GstState newState = GST_STATE_VOID_PENDING;
    GstMessage *msg = NULL;
    
    if(gstInfo == NULL)
    {
        return IPP_AUDIO_ERROR;
    }
    
    if(gstInfo->pipeElement == NULL)
    {
        return IPP_AUDIO_ERROR;
    }
    
    do
    {
        /* Set the specified state */
        gst_element_set_state(gstInfo->pipeElement, gstState);
        
        /* Wait the notification of status changing until timeout */
        msg = gst_bus_poll(GST_ELEMENT_BUS(gstInfo->pipeElement), GST_MESSAGE_ERROR | GST_MESSAGE_STATE_CHANGED, GST_STATUS_CHANGE_WAIT_TIME);
        if(msg != NULL)
        {
            switch(GST_MESSAGE_TYPE(msg))
            {
                case GST_MESSAGE_STATE_CHANGED:
                    /* Check the old state and current state */
                    gst_message_parse_state_changed(msg, &oldState, &newState, NULL);
                    /* Current status is not matched to specified state */
                    if(newState != gstState)
                    {
                        /* sleep */
                        usleep(GST_STATUS_CHANGE_SLEEP_TIME);
                    }
                    break;
                    
                case GST_MESSAGE_ERROR:
                    rc = IPP_AUDIO_ERROR;
                    break;
                    
                default:
                    rc = IPP_AUDIO_ERROR;
                    break;
            }
        }
        
        count++;
        
    } while((count < GST_STATUS_CHANGE_RETYR_MAX) && (newState != gstState) && (rc == IPP_AUDIO_OK));

    if(newState == gstState)
    {
        rc = IPP_AUDIO_OK;
    }
    else
    {
        rc = IPP_AUDIO_ERROR;
    }
    
    return rc;
}

/**
 * void GSTStart(void)
 *
 * Start the PulseAudio.
 *
 */
S32 iPodAudioGstStart(IPOD_AUDIO_COM_INFO **gstInfo, U8 *srcName, U8 *sinkName, U32 rate)
{
    int rc = IPP_AUDIO_ERROR;
    
    /* Parameter check */
    if((gstInfo == NULL) || (srcName == NULL) || (sinkName == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, gstInfo, srcName, sinkName);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    *gstInfo = calloc(1, sizeof(**gstInfo));
    if(*gstInfo != NULL)
    {
        (*gstInfo)->rate = rate;
        rc = gst_com_create_ipodbin(*gstInfo, srcName, sinkName);
        if(rc == IPP_AUDIO_OK)
        {
            /* Status changes to Ready */
            rc = gst_com_change_state(*gstInfo, GST_STATE_READY);
            if(rc == IPP_AUDIO_OK)
            {
                /* Status changes to Play */
                rc = gst_com_change_state(*gstInfo, GST_STATE_PLAYING);
            }
        }
    }
    
    return rc;
}

/**
 * void GSTStop(void)
 *
 * Stop the PulseAudio.
 *
 */
void iPodAudioGstStop(IPOD_AUDIO_COM_INFO *gstInfo)
{
    if(gstInfo == NULL)
    {
        return;
    }
    
    if(gstInfo->pipeElement != NULL)
    {
        gst_element_set_state(gstInfo->pipeElement, GST_STATE_NULL);
        if((gstInfo->capsElement != NULL) &&
            (gstInfo->volumeElement != NULL) &&
            (gstInfo->sinkElement != NULL))
        {
            gst_element_unlink_many(gstInfo->srcElement, gstInfo->capsElement, gstInfo->volumeElement, gstInfo->sinkElement, NULL);
            /* PRQA: Lint Message 826: This conversion uses the macro of Gstreamer and this is correct way to use gst_bin_remove_many. So this case is correct*/
            gst_bin_remove_many(GST_BIN(gstInfo->pipeElement), gstInfo->srcElement, gstInfo->capsElement, gstInfo->volumeElement, gstInfo->sinkElement, NULL); /*lint !e826 */
        }
        gst_object_unref(gstInfo->pipeElement);
        gstInfo->pipeElement = NULL;
    }
    
    free(gstInfo);
    
    return;
}


/**
 * void GSTSetSR(void)
 *
 * set sample rate.
 *
 */
S32 iPodAudioGstSetSamplerate(IPOD_AUDIO_COM_INFO *gstInfo, U32 rate)
{
    int rc = IPP_AUDIO_OK;
    
    if(gstInfo == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* change rate */
    if(gstInfo->rate != rate)
    {
        gstInfo->rate = rate; /* update sample rate */
        /* Remove all previous elements */
        rc = gst_com_change_state(gstInfo, GST_STATE_READY);
        if(rc == IPP_AUDIO_OK)
        {
            gst_caps_set_simple(gstInfo->caps, GST_CAPS_PROP_KEU_RATE, G_TYPE_INT, gstInfo->rate, NULL);
            rc = gst_com_change_state(gstInfo, GST_STATE_PLAYING);
        }
    }
    else
    {
        rc = IPP_AUDIO_ERROR;
    }
    
    return rc;
}

S32 iPodAudioGstSetVolume(IPOD_AUDIO_COM_INFO *gstInfo, U8 volume)
{
    int rc = IPP_AUDIO_ERROR;
    
    if(gstInfo == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, gstInfo);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    if(gstInfo->volume != volume)
    {
        gstInfo->volume = (gdouble)volume / GST_VALUE_100;
        
        if(gstInfo->volumeElement != NULL)
        {
            g_object_set(G_OBJECT(gstInfo->volumeElement), GST_VOLUME_PROP_VOLUME, gstInfo->volume, NULL);
            rc = IPP_AUDIO_OK;
        }
        else
        {
            rc = IPP_AUDIO_ERROR;
        }
    }
    else
    {
        rc = IPP_AUDIO_OK;
    }
    
    return rc;
}

S32 iPodAudioGstGetVolume(IPOD_AUDIO_COM_INFO *gstInfo, U8 *volume)
{
    if((gstInfo == NULL) || (volume == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, gstInfo, volume);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    gdouble changeInt = 0.0;
    changeInt = gstInfo->volume * 100;
    *volume = (U8)changeInt;
    
    return IPOD_PLAYER_OK;
}

S32 iPodAudioComInit(IPOD_AUDIO_COM_FUNC_TABLE *table)
{
    if(table == NULL)
    {
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    table->start = &iPodAudioGstStart;
    table->stop = &iPodAudioGstStop;
    table->setVolume = &iPodAudioGstSetVolume;
    table->getVolume = &iPodAudioGstGetVolume;
    table->setSample = &iPodAudioGstSetSamplerate;
    
    gst_init(NULL, NULL);
    
    return IPOD_PLAYER_OK;
}

void iPodAudioComDeinit(IPOD_AUDIO_COM_FUNC_TABLE *table)
{
    if(table == NULL)
    {
        return;
    }
    
    table->start = NULL;
    table->stop = NULL;
    table->setVolume = NULL;
    table->getVolume = NULL;
    
    gst_deinit();
    
    return;
}

