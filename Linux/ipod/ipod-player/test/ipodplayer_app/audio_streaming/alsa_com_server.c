/**
 * \file: alsa_com_server.c
 *
 *  IPC server for communicate with ALSA and applicaiton process
 *
 * \component:  IPC server
 *
 * \author: ADIT
 *
 * \copyright: (c) 2003 - 2011 ADIT Corporation
 *
 *
 */

#include <alsa/asoundlib.h>
#include <pthread_adit.h>

#include "ipp_audio_common.h"
#include "alsa_com_server_i.h"

int iPodAudioAlsaGetVolume(IPOD_AUDIO_COM_INFO *alsaInfo, U8 *volume)
{
    int rc = -1;
    long alsaVol = 0;
    long min = 0;
    long max = 0;
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_AUDIO);
    
    if((alsaInfo == NULL) || (volume == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Return value of playback volume control of a mixer simple element. */
    rc = snd_mixer_selem_get_capture_volume(alsaInfo->vol_element, SND_MIXER_SCHN_FRONT_LEFT, &alsaVol);
    if(rc == 0)
    {
        /* Get range for playback volume of a mixer simple element. */
        rc = snd_mixer_selem_get_capture_volume_range(alsaInfo->vol_element, &min, &max);
        if(rc < 0)
        {
            IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc);
        }
    }
    
    if(rc == 0)
    {
        rc = IPOD_PLAYER_OK;
        if((alsaVol < min) || (alsaVol > max))
        {
            rc = IPOD_PLAYER_ERROR;
        }
    }
    else
    {
        rc = IPOD_PLAYER_ERROR;
    }
    
    if ( rc == IPOD_PLAYER_OK )
    {
        /* 0~99 are set as they are, 100 are set as 99. */
        /* The got volume must be in the range, so no need to count. */
        alsaInfo->volume = (U8)alsaVol;
        *volume = (U8)alsaVol;
        IPOD_LOG_INFO_WRITESTR32(IPOD_LOG_TYPE_FUNCDETAIL, IPOD_LOG_PLAYER_AUDIO, "The taken VOLUME", alsaInfo->volume);
    }
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_AUDIO, rc);
    
    return rc;
}

static int iPodAudioAlsaSetVolume(IPOD_AUDIO_COM_INFO *alsaInfo, U8 volume)
{
    int rc = -1;
    long temp = 0;
    long min = 0;
    long max = 0;
    double realVol = 0x0;
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_AUDIO);
    
    if (alsaInfo == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Get range for playback volume of a mixer simple element. */
    rc = snd_mixer_selem_get_capture_volume_range(alsaInfo->vol_element, &min, &max);
    if(rc == 0)
    {
        /* 0~99 are set as they are, 100 are set as 99. */
        /* Note:The volume is typed to U8, so the range is [0~255]. */
        if(volume > max)
        {
            volume = max;
        }
        else if(volume < min)
        {
            volume = min;
        }
        
        realVol = volume * max;
        realVol = realVol / IPOD_ALSA_VOLUME_RANGE;
        temp = (long)realVol;
        
        if((realVol - (double)temp) >= IPOD_ALSA_ACCURACY)
        {
            temp++;
        }
        
        /* Set value of playback volume control for all channels of a mixer simple element. */
        rc = snd_mixer_selem_set_capture_volume_all(alsaInfo->vol_element, temp);
        if(rc == 0)
        {
            rc = IPOD_PLAYER_OK;
        }
        else
        {
            IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc);
            rc = IPOD_PLAYER_ERROR;
        }
    }
    else
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc);
        rc = IPOD_PLAYER_ERROR;
    }
    
    if ( rc == IPOD_PLAYER_OK )
    {
        alsaInfo->volume = (U8)temp;
        IPOD_LOG_INFO_WRITESTR32(IPOD_LOG_TYPE_FUNCDETAIL, IPOD_LOG_PLAYER_AUDIO, "The set VOLUME", alsaInfo->volume);
    }
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_AUDIO, rc);
    
    return rc;
}

static void iPodAudioAlsaDeleteVolume(IPOD_AUDIO_COM_INFO *alsaInfo)
{
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_AUDIO);
    
    if (alsaInfo == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER);
        return;
    }
    
    if (alsaInfo->vol_handle != NULL)
    {
        /* Close a mixer and free all related resources. */
        snd_mixer_close(alsaInfo->vol_handle);
        alsaInfo->vol_handle = NULL;
        alsaInfo->vol_element = NULL;
    }
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_AUDIO, 0);
    
}

static int iPodAudioAlsaCreateVolume(IPOD_AUDIO_COM_INFO *alsaInfo, U8 *devName)
{
    int rc = -1;
    snd_mixer_t *handle = NULL;
    snd_mixer_selem_id_t *sid;
    char card[IPOD_ALSA_INPUTDEV_BUF] = {0};
    const char *selem_name = IPOD_ALSA_MIXER_SIMPLE_IDENTIFIER;
    snd_mixer_elem_t *elem = NULL;
    U8 volume = 0;
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_AUDIO);
    
    if((alsaInfo == NULL) || (devName == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Opens an empty mixer. */
    rc = snd_mixer_open(&handle, 0);
    if(rc == 0)
    {
        /* Attach an HCTL specified with the CTL device name to an opened mixer. */
        snprintf(card, sizeof(card), "%s:%c", IPOD_ALSA_HTCL_NAME, devName[IPOD_ALSA_HW_CARD_NUM_POS]);
        rc = snd_mixer_attach(handle, card);
        if(rc == 0)
        {
            /* Register mixer simple element class. */
            rc = snd_mixer_selem_register(handle, NULL, NULL);
            if(rc < 0)
            {
                snd_mixer_detach(handle, card);
                snd_mixer_close(handle);
                handle = NULL;
                IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc);
            }
        }
        else
        {
            snd_mixer_close(handle);
            handle = NULL;
            IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc);
        }
    }
    else
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc);
    }
    
    if(rc == 0)
    {
        /* Load a mixer elements. */
        rc = snd_mixer_load(handle);
        if(rc == 0)
        {
            /* allocate an invalid snd_mixer_selem_id_t using standard alloca */
            snd_mixer_selem_id_alloca(&sid);
            if (sid != NULL)
            {
                /* Set index part of a mixer simple element identifier. */
                snd_mixer_selem_id_set_index(sid, 0);
                /* Set name part of a mixer simple element identifier. */
                snd_mixer_selem_id_set_name(sid, selem_name);
                /* Find a mixer simple element. */
                elem = snd_mixer_find_selem(handle, sid);
                if ( elem != NULL )
                {
                    /* Get range for playback volume of a mixer simple element. */
                    rc = snd_mixer_selem_set_capture_volume_range(elem, 0, IPOD_ALSA_VOLUME_MAX);
                    if(rc == 0)
                    {
                        alsaInfo->vol_element = elem;
                        rc = IPOD_PLAYER_OK;
                    }
                    else
                    {
                        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc);
                        rc = IPOD_PLAYER_ERROR;
                    }
                }
                else
                {
                    rc = IPOD_PLAYER_ERROR;
                    IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc);
                }
            }
            else
            {
                rc = IPOD_PLAYER_ERROR;
                IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc);
            }
        }
        else
        {
            rc = IPOD_PLAYER_ERROR;
            IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc);
        }
    }
    else
    {
        rc = IPOD_PLAYER_ERROR;
    }
    
    if(rc == IPOD_PLAYER_OK)
    {
        rc = iPodAudioAlsaGetVolume(alsaInfo, &volume);
    }
    
    if (rc == IPOD_PLAYER_OK)
    {
        alsaInfo->vol_handle = handle;
    }
    else
    {
        if (handle != NULL)
        {
            /* Detach the previously attached HCTL */
            snd_mixer_detach(handle, card);
            /* Close a mixer and free all related resources. */
            snd_mixer_close(handle);
        }
    }
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_AUDIO, rc);
    
    return rc;
}


/*
 * iPodAudioAlsaXrunRecovery    : recovery callback in case of error
 * param 1    : Capture device
 * param 2    : Error no.
 */
static int iPodAudioAlsaXrunRecovery(snd_pcm_t *handle, int error)
{
    int rc = -1;
    int i = 0;
    struct timespec req;
    
    memset(&req, 0, sizeof(req));
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_AUDIO);
    
    switch(error)
    {
        /* Buffer Over-run */
        case -EPIPE:
            rc = snd_pcm_prepare(handle);
            break;
        
        /* suspend event occurred */
        case -ESTRPIPE:
            rc = -EAGAIN;
            /* EAGAIN means that the request cannot be processed immediately */
            while ( rc == -EAGAIN )
            {
                /* wait until the suspend flag is clear */
                for(i=0; i<IPOD_ALSA_LOOP_COUNT; i++)
                {
                    req.tv_nsec = IPOD_ALSA_LOOP_TIMEOUT;
                    nanosleep(&req, NULL);
                    rc = snd_pcm_resume(handle);
                    if(rc != -EAGAIN)
                    {
                        break;
                    }
                }
                /* If the suspend flag is not cleared in 5 seconds, break. */
                break;
            }
            if (rc < 0) /* error case */
            {
                rc = snd_pcm_prepare(handle);
            }
            break;

        /*Error PCM descriptor is wrong*/
        case -EBADFD:
            rc = error;
            break;

        default:
            rc = error;
            break;
    }
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCDETAIL, IPOD_LOG_PLAYER_AUDIO, error);
    
    if(rc == 0)
    {
        rc = IPOD_PLAYER_OK;
    }
    else
    {
        rc = IPOD_PLAYER_ERROR;
    }
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_AUDIO, error);
    
    return rc;
}

static int iPodAudioAlsaReadOneChunk(IPOD_AUDIO_COM_INFO *alsaInfo)
{
    int rc = IPOD_PLAYER_ERROR;
    int counter = 0;
    void *ptr = NULL;
    snd_pcm_sframes_t read_size = 0;
    snd_pcm_sframes_t read_size_left = 0;
    struct timespec req;
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_AUDIO);
    
    if (alsaInfo == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    memset(&req, 0, sizeof(req));
    read_size_left = alsaInfo->period_size;
    ptr = alsaInfo->buf[alsaInfo->rIndex];
    
    /* Check if the overrun is occured. */
    if((alsaInfo->rIndex == alsaInfo->wIndex) && (alsaInfo->status == ALSA_READ_OVERRUN) )
    {
        alsaInfo->status = ALSA_READ_OVERRUN;
        rc = IPOD_ALSA_XRUN_ERROR;
    }
    else
    {
        while ((read_size_left > 0) && (counter < IPOD_ALSA_LOOP_COUNT))
        {
            read_size = snd_pcm_readi(alsaInfo->chandle, ptr, read_size_left);
            if (read_size < 0)
            {
                rc = iPodAudioAlsaXrunRecovery(alsaInfo->chandle, read_size);
                if (rc == IPOD_PLAYER_OK)
                {
                    /* Count once again. */
                    counter = 0;
                }
                else
                {
                    /* Have not recover the error. */
                    break;
                }
            }
            else if (read_size == 0)
            {
                /* For prevent the infinity loop, it will break in 5 seconds. */
                req.tv_nsec = IPOD_ALSA_LOOP_TIMEOUT;
                nanosleep(&req, NULL);
                counter++;
            }
            else
            {
                /* Update the read_size_left. */
                ptr = (S8 *)ptr + read_size * IPOD_ALSA_DATA_SIZE * IPOD_ALSA_CHANNEL;
                read_size_left -= read_size;
                /* Count once again. */
                counter = 0;
            }
        }
        
        alsaInfo->rIndex++;
        alsaInfo->rIndex %= IPOD_ALSA_BUF_NUM;
        /* Set the status */
        if(alsaInfo->rIndex == alsaInfo->wIndex)
        {
            alsaInfo->status = ALSA_READ_OVERRUN;
        }
        else
        {
            alsaInfo->status = ALSA_RUNNING;
        }
        
        if(read_size_left == 0)
        {
            rc = IPOD_PLAYER_OK;
        }
        else
        {
            rc = IPOD_PLAYER_ERROR;
        }
    }
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_AUDIO, rc);

    return rc;
}

static int iPodAudioAlsaWriteOneChunk(IPOD_AUDIO_COM_INFO *alsaInfo)
{
    int rc = IPOD_PLAYER_ERROR;
    int counter = 0;
    void *ptr = NULL;
    snd_pcm_sframes_t write_size = 0;
    snd_pcm_sframes_t write_size_left = 0;
    struct timespec req;
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_AUDIO);
    
    if(alsaInfo == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    memset(&req, 0, sizeof(req));
    write_size_left = alsaInfo->period_size;
    ptr = alsaInfo->buf[alsaInfo->wIndex];
    
    /* Check if the overrun is occured. */
    if((alsaInfo->rIndex == alsaInfo->wIndex) && (alsaInfo->status == ALSA_WRITE_OVERRUN))
    {
        alsaInfo->status = ALSA_WRITE_OVERRUN;
        rc = IPOD_ALSA_XRUN_ERROR;
    }
    else
    {
        while ((write_size_left > 0) && (counter < IPOD_ALSA_LOOP_COUNT))
        {
            write_size = snd_pcm_writei(alsaInfo->phandle, ptr, write_size_left);
            if (write_size < 0)
            {
                rc = iPodAudioAlsaXrunRecovery(alsaInfo->phandle, write_size);
                if (rc == IPOD_PLAYER_OK)
                {
                    /* Count once again. */
                    counter = 0;
                }
                else
                {
                    /* Have not recover the error. */
                    break;
                }
            }
            else if (write_size == 0)
            {
                /* For prevent the infinity loop, it will break in 5 seconds. */
                req.tv_nsec = IPOD_ALSA_LOOP_TIMEOUT;
                nanosleep(&req, NULL);
                counter++;
            }
            else
            {
                /* Update the write_size_left. */
                ptr = (S8 *)ptr + write_size * IPOD_ALSA_DATA_SIZE * IPOD_ALSA_CHANNEL;
                write_size_left -= write_size;
                /* Count once again. */
                counter = 0;
            }
        }
        
        alsaInfo->wIndex++;
        alsaInfo->wIndex %= IPOD_ALSA_BUF_NUM;
        
        /* Set the status. */
        if (alsaInfo->rIndex == alsaInfo->wIndex)
        {
            alsaInfo->status = ALSA_WRITE_OVERRUN;
        }
        else
        {
            alsaInfo->status = ALSA_RUNNING;
        }
        
        if(write_size_left == 0)
        {
            rc = IPOD_PLAYER_OK;
        }
        else
        {
            rc = IPOD_PLAYER_ERROR;
        }
    }
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_AUDIO, rc);
    
    return rc;
}

static int iPodAudioAlsaReadData(IPOD_AUDIO_COM_INFO *alsaInfo)
{
    int rc = -1;
    int counter = 0;
    snd_pcm_sframes_t avail = 0;
    struct timespec req;
    
    if (alsaInfo == NULL)
    {
        return IPOD_PLAYER_ERROR;
    }
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_AUDIO);
    memset(&req, 0, sizeof(req));
    /* Return number of frames ready to be read (capture) / written (playback). */
    avail = snd_pcm_avail_update(alsaInfo->chandle);
    if(avail >= 0)
    {
        rc = IPOD_PLAYER_OK;
    }
    else
    {
        /* The overrun occured. */
        rc = iPodAudioAlsaXrunRecovery(alsaInfo->chandle, avail);
        if (rc == IPOD_PLAYER_OK)
        {
            /* Return number of frames ready to be read (capture) / written (playback). */
            avail = snd_pcm_avail_update(alsaInfo->chandle);
        }
    }
    
    if(rc == IPOD_PLAYER_OK)
    {
        while (avail >= alsaInfo->period_size)
        {
            rc = iPodAudioAlsaReadOneChunk(alsaInfo);
            if ( rc == IPOD_PLAYER_OK )
            {
                /* Return number of frames ready to be read (capture) / written (playback). */
                avail = snd_pcm_avail_update(alsaInfo->chandle);
            }
            else if (rc == IPOD_ALSA_XRUN_ERROR)
            {
                break;
            }
            else{
                req.tv_nsec = IPOD_ALSA_LOOP_TIMEOUT;
                nanosleep(&req, NULL);
                counter++;
                if(counter == IPOD_ALSA_LOOP_COUNT)
                {
                    /* If the suspend flag is not cleared in 5 seconds, break. */
                    break;
                }
            }
        }
    }
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_AUDIO, rc);
    
    return rc;
}

static int iPodAudioAlsaWriteData(IPOD_AUDIO_COM_INFO *alsaInfo)
{
    int rc = IPOD_PLAYER_OK;
    int counter = 0;
    snd_pcm_sframes_t delay = 0;
    snd_pcm_sframes_t avail = 0;
    struct timespec req;
    
    if (alsaInfo == NULL)
    {
        return IPOD_PLAYER_ERROR;
    }
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_AUDIO);
    
    memset(&req, 0, sizeof(req));
    /* Obtain delay for a running PCM handle. */
    rc = snd_pcm_delay(alsaInfo->phandle, &delay);
    /* Return number of frames ready to be read (capture) / written (playback). */
    avail = snd_pcm_avail_update(alsaInfo->phandle);
    if ((rc < 0) || (avail < 0))
    {
        rc = iPodAudioAlsaXrunRecovery(alsaInfo->phandle, rc);
        if (rc == IPOD_PLAYER_OK)
        {
            /* Obtain delay for a running PCM handle. */
            rc = snd_pcm_delay(alsaInfo->phandle, &delay);
            if(rc == 0)
            {
                /* Return number of frames ready to be read (capture) / written (playback). */
                avail = snd_pcm_avail_update(alsaInfo->phandle);
                rc = IPOD_PLAYER_OK;
            }
            else
            {
                rc = IPOD_PLAYER_ERROR;
            }
        }
    }
    else
    {
        rc = IPOD_PLAYER_OK;
    }
    
    while ((avail >= alsaInfo->period_size) && (rc == IPOD_PLAYER_OK))
    {
        rc = iPodAudioAlsaWriteOneChunk(alsaInfo);
        if ( rc == IPOD_PLAYER_OK )
        {
            /* Obtain delay for a running PCM handle. */
            rc = snd_pcm_delay(alsaInfo->phandle, &delay);
            if(rc == 0)
            {
                rc = IPOD_PLAYER_OK;
            }
            else
            {
                rc = IPOD_PLAYER_ERROR;
            }
        }
        else if (rc == IPOD_ALSA_XRUN_ERROR)
        {
            break;
        }
        else
        {
            req.tv_nsec = IPOD_ALSA_LOOP_TIMEOUT;
            nanosleep(&req, NULL);
            counter++;
            if(counter == IPOD_ALSA_LOOP_COUNT)
            {
                /* If the suspend flag is not cleared in 5 seconds, break. */
                break;
            }
        }
        if ( rc == IPOD_PLAYER_OK )
        {
            /* Return number of frames ready to be read (capture) / written (playback). */
            avail = snd_pcm_avail_update(alsaInfo->phandle);
        }
    }
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_AUDIO, rc);
    return rc;
}

static int iPodAudioAlsaSetHwParams(IPOD_AUDIO_COM_INFO *alsaInfo, snd_pcm_t *pcm_handle, unsigned int rate)
{
    int rc = -1;
    snd_pcm_hw_params_t *hw_params = NULL;
    snd_pcm_uframes_t buffer_size = 0;
    snd_pcm_uframes_t period_size = 0;
    unsigned int buffer_time = IPOD_ALSA_MAX_BUFFER_TIME;
    unsigned int period_time = IPOD_ALSA_MAX_PERIOD_TIME;
    int bits_per_sample = 0;
    int bits_per_frame = 0;
    int chunk_bytes = 0;
    snd_pcm_format_t fmt = SND_PCM_FORMAT_S16_LE;
    unsigned int channels = 0;
    
    if((alsaInfo == NULL) || (pcm_handle == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }

    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_AUDIO);

    /* allocate an invalid snd_pcm_hw_params_t using standard malloc */
    snd_pcm_hw_params_alloca(&hw_params);
    if ( hw_params != NULL )
    {
        /* Fill params with a full configuration space for a PCM. */
        rc = snd_pcm_hw_params_any(pcm_handle, hw_params);
    }
    if ( rc >= 0 )
    {
        /* Restrict a configuration space to contain only one access type. */
        rc = snd_pcm_hw_params_set_access(pcm_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED);
    }
    if ( rc == 0 )
    {
        /* Restrict a configuration space to contain only real hardware rates. */
        rc = snd_pcm_hw_params_set_rate_resample(pcm_handle, hw_params, 1);
    }
    if ( rc == 0 )
    {
        /* Restrict a configuration space to contain only one format. */
        rc = snd_pcm_hw_params_set_format(pcm_handle, hw_params, fmt);
    }

    if ( rc == 0 )
    {
        /* Restrict a configuration space to contain only one channels count. */
        rc = snd_pcm_hw_params_set_channels(pcm_handle, hw_params, IPOD_ALSA_CHANNEL);
    }

    if ( rc == 0 )
    {
        /* Restrict a configuration space to have rate nearest to a target. */
        rc = snd_pcm_hw_params_set_rate_near(pcm_handle, hw_params, &rate, NULL);
    }
    if ( rc == 0 )
    {
        /* Restrict a configuration space to have buffer time nearest to a target. */
        rc = snd_pcm_hw_params_set_buffer_time_near(pcm_handle, hw_params, &buffer_time, 0);
        period_time = buffer_time / 4;
    }
    if ( rc == 0 )
    {
        /* Restrict a configuration space to have period time nearest to a target. */
        rc = snd_pcm_hw_params_set_period_time_near(pcm_handle, hw_params, &period_time, 0);
    }
    if ( rc == 0 )
    {
        /* Install one PCM hardware configuration chosen from a configuration space and snd_pcm_prepare it. */
        rc = snd_pcm_hw_params(pcm_handle, hw_params);
    }
    if ( rc == 0 )
    {
        /* Extract period size from a configuration space. */
        rc = snd_pcm_hw_params_get_period_size(hw_params, &period_size, NULL);
        if ( rc == 0 )
        {
            alsaInfo->period_size = period_size;
        }
    }
    if ( rc  == 0)
    {
        /* Extract buffer size from a configuration space. */
        rc = snd_pcm_hw_params_get_buffer_size(hw_params, &buffer_size);
    }
    
    if(rc == 0)
    {
        /* Extract format from a configuration space. */
        rc = snd_pcm_hw_params_get_format(hw_params, &fmt);
    }
    
    if( rc >= 0 )
    {
        /* Extract channels from a configuration space. */
        rc = snd_pcm_hw_params_get_channels(hw_params, &channels);
    }
    if ( rc == 0 )
    {
        bits_per_sample = snd_pcm_format_physical_width(fmt);
        bits_per_frame = bits_per_sample * channels;
        chunk_bytes = period_size * bits_per_frame / IPOD_ALSA_BIT_PER_BYTE;
        
        if((alsaInfo->chunk_bytes == 0) ||
           (chunk_bytes < alsaInfo->chunk_bytes))
        {
            alsaInfo->chunk_bytes = chunk_bytes;
        }
        
        rc = IPOD_PLAYER_OK;
    }
    else
    {
        rc = IPOD_PLAYER_ERROR;
    }
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_AUDIO, rc);
    
    return rc;
}

static int iPodAudioAlsaSetSwParams(IPOD_AUDIO_COM_INFO *alsaInfo, snd_pcm_t *pcm_handle)
{
    int rc = -1;
    snd_pcm_sw_params_t *sw_params = NULL;
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_AUDIO);
    
    if((alsaInfo == NULL) || (pcm_handle == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* allocate an invalid snd_pcm_sw_params_t using standard malloc. */
    snd_pcm_sw_params_alloca(&sw_params);
    if ( sw_params != NULL )
    {
        /* Return current software configuration for a PCM. */
        rc = snd_pcm_sw_params_current(pcm_handle, sw_params);
    }
    if ( rc == 0 )
    {
        /* Set avail min inside a software configuration container. */
        rc = snd_pcm_sw_params_set_avail_min(pcm_handle, sw_params, alsaInfo->period_size);
    }
    
    if ( rc == 0 )
    {
        /* Set start threshold inside a software configuration container. */
        rc = snd_pcm_sw_params_set_start_threshold(pcm_handle, sw_params, alsaInfo->period_size);
    }
    
    if ( rc == 0 )
    {
        /* Install PCM software configuration defined by params. */
        rc = snd_pcm_sw_params(pcm_handle, sw_params);
    }
    
    if(rc == 0)
    {
        rc = IPOD_PLAYER_OK;
    }
    else
    {
        rc = IPOD_PLAYER_ERROR;
    }
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_AUDIO, rc);
    
    return rc;
}


void *iPodAudioAlsaPlayTsk(void *arg)
{
    int rc = IPOD_PLAYER_ERROR;
    IPOD_AUDIO_COM_INFO *alsaInfo = NULL;
    struct timespec req;
    
    if(arg == NULL)
    {
        return NULL;
    }
    
    memset(&req, 0, sizeof(req));
    alsaInfo = (IPOD_AUDIO_COM_INFO *)arg;
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_AUDIO);
    while (alsaInfo->playThrState == 1 )
    {
        rc = iPodAudioAlsaReadData(alsaInfo);
        if((rc == IPOD_PLAYER_OK) || (rc == IPOD_ALSA_XRUN_ERROR))
        {
            /* when the XRUN error occured, */
            /* iPodAudioAlsaReadData()/iPodAudioAlsaWriteData() need to run each other to solve the problem. */
            /* Because the loop is not rely on the return value of iPodAudioAlsaReadData()/iPodAudioAlsaWriteData(), */
            /* so there is no need to check the return value of iPodAudioAlsaWriteData(). */
            rc = iPodAudioAlsaWriteData(alsaInfo);
        }
        /* Use the nanosleep() in order to reduce CPU usage. */
        req.tv_nsec = IPOD_ALSA_MAIN_LOOP_SLEEP_TIME;     /* 10000000 */
        nanosleep(&req, NULL);
    }
    
    alsaInfo->playThrState = 0;
    pthread_exit(&rc);
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_AUDIO, rc);
    
    return NULL;
}

static int iPodAudioAlsaStartPlayTask(IPOD_AUDIO_COM_INFO *alsaInfo, pthread_t *threadId)
{
    int rc = -1;
    pthread_attr_t pattr;
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_AUDIO);
    
    if((alsaInfo == NULL) || (threadId == NULL))
    {
        return rc;
    }
    
    /* Initialize the attribution of thread */
    rc = pthread_attr_init(&pattr);
    if(rc == 0)
    {
        /* Joined when thread is removed */
        rc = pthread_attr_setdetachstate(&pattr, PTHREAD_CREATE_JOINABLE);
        if(rc == 0)
        {
            alsaInfo->playThrState = 1;
            rc = pthread_create(threadId, &pattr, iPodAudioAlsaPlayTsk, alsaInfo);
        }
        /* Destroy the attribution of thread */
        (void)pthread_attr_destroy(&pattr);
    }
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_AUDIO, rc);
    
    if(rc == 0)
    {
        rc = IPOD_PLAYER_OK;
    }
    else
    {
        rc = IPOD_PLAYER_ERROR;
    }
    
    return rc;
}


/* Deletes dynamical alsa configuration */
static void iPodAudioAlsaDeleteConfiguration(snd_config_t *conf)
{
    int rc = -1;
    
    if(conf != NULL)
    {
        /* deleate config and all sub-elements */
        rc = snd_config_delete(conf);
        if(rc != 0)
        {
            IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_AUDIO, rc);
        }
        else
        {
            conf = NULL;
        }
    }
}

/* Creates an element if parent node is not null, add it to parent */
static snd_config_t *iPodAudioAlsaCreateChildConfiguration(snd_config_t *parent, snd_config_type_t type, const char *key, const void *value, int ex_flag)
{
    int rc = -1;
    snd_config_t *child = NULL;
    switch(type)
    {
        case SND_CONFIG_TYPE_COMPOUND:
            /* Creates an compound element */ 
            rc = snd_config_make_compound(&child, key, ex_flag);
            break;
        case SND_CONFIG_TYPE_STRING:
            /* Creates an string element */ 
            rc = snd_config_imake_string(&child, key, (const char *)value);
            break;
        case SND_CONFIG_TYPE_INTEGER:
            /* Creates an integer element */ 
            rc = snd_config_imake_integer(&child, key, *(int *)value);
            break;
        default:
            break;
    }
    
    /* Created child successfully and parent is not null */
    if((rc == 0) && (parent !=NULL))
    {
        /* Add to parent */
        rc = snd_config_add(parent, child);
        if(rc != 0)
        {
            /* Free the child */
            iPodAudioAlsaDeleteConfiguration(child);
        }
    }
    
    return child;
}
            
/* Creates dynamical alsa configuration */
static snd_config_t *iPodAudioAlsaCreateInputConfiguration(const char* pcm_name, int card_num, int dev_num)
{
    int rc = -1;
    snd_config_t *top = NULL;
    snd_config_t *main_pcm = NULL;
    snd_config_t *sub_pcm = NULL;
    snd_config_t *base_pcm = NULL;
    snd_config_t *current = NULL;
    snd_config_t *conf = NULL;
    int resolut = IPOD_ALSA_HW_DEVICE_RESOLUTION;
    
    /* Check parameter */
    if(pcm_name == NULL)
    {
        /* Leave the function immediately if paramter invalid */
        IPOD_ERR_WRITE_PTR(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_AUDIO, IPOD_PLAYER_ERROR, pcm_name);
        return NULL;
    }
    
    /* Creates a top level configuration node */
    rc = snd_config_top(&top);
    
    /* Creates a base pcm device */
    if(rc == 0)
    {
        /* Creates main pcm */
        current = iPodAudioAlsaCreateChildConfiguration(top, SND_CONFIG_TYPE_COMPOUND, IPOD_ALSA_SELEM_PCM, NULL, 1);
        if(current != NULL)
        {
            /* Add main pcm name */
            current = iPodAudioAlsaCreateChildConfiguration(current, SND_CONFIG_TYPE_COMPOUND, pcm_name, NULL, 0);
        }
    }
    if(current != NULL)
    {
        /* Add main pcm type */
        main_pcm = current;
        current = iPodAudioAlsaCreateChildConfiguration(current, SND_CONFIG_TYPE_STRING, IPOD_ALSA_SELEM_TYPE, IPOD_ALSA_SELEM_PLUG, 0);
        if(current != NULL)
        {
            /* add main pcm slave */
            current = iPodAudioAlsaCreateChildConfiguration(main_pcm, SND_CONFIG_TYPE_COMPOUND, IPOD_ALSA_SELEM_SLAVE, NULL, 1);
        }
    }
    if(current != NULL)
    {
        /* Add sub pcm */
        current = iPodAudioAlsaCreateChildConfiguration(current, SND_CONFIG_TYPE_COMPOUND, IPOD_ALSA_SELEM_PCM, NULL, 0);
        if(current != NULL)
        {
            sub_pcm = current;
            /* add sub pcm type */
            current = iPodAudioAlsaCreateChildConfiguration(current, SND_CONFIG_TYPE_STRING, IPOD_ALSA_SELEM_TYPE, IPOD_ALSA_SELEM_SVOL, 0);
        }
    }
    if(current != NULL)
    {
        /* Add softvolume slave */
        current = iPodAudioAlsaCreateChildConfiguration(sub_pcm, SND_CONFIG_TYPE_COMPOUND, IPOD_ALSA_SELEM_SLAVE, NULL, 1);
        if(current != NULL)
        {
            /* add base pcm */
            current = iPodAudioAlsaCreateChildConfiguration(current, SND_CONFIG_TYPE_COMPOUND, IPOD_ALSA_SELEM_PCM, NULL, 0);
        }
    }
    if(current != NULL)
    {
        base_pcm = current;
        /* Add base pcm type */
        current = iPodAudioAlsaCreateChildConfiguration(base_pcm, SND_CONFIG_TYPE_STRING, IPOD_ALSA_SELEM_TYPE, IPOD_ALSA_SELEM_HW, 0);
        if(current != NULL)
        {
            /* add base pcm card */
            current = iPodAudioAlsaCreateChildConfiguration(base_pcm, SND_CONFIG_TYPE_INTEGER, IPOD_ALSA_SELEM_CARD, (void *)&card_num, 0);
        }
    }
    if(current != NULL)
    {
        /* Add base pcm device */
        current = iPodAudioAlsaCreateChildConfiguration(base_pcm, SND_CONFIG_TYPE_INTEGER, IPOD_ALSA_SELEM_DEVICE, &dev_num, 0);
        if(current != NULL)
        {
            /* add control */
            current = iPodAudioAlsaCreateChildConfiguration(sub_pcm, SND_CONFIG_TYPE_COMPOUND, IPOD_ALSA_SELEM_CTRL, NULL, 0);
        }
    }
    if(current != NULL)
    {
        /* Add control name */
        current = iPodAudioAlsaCreateChildConfiguration(current, SND_CONFIG_TYPE_STRING, IPOD_ALSA_SELEM_NAME, IPOD_ALSA_MIXER_SIMPLE_IDENTIFIER, 0);
        if(current != NULL)
        {
            /* add resolution */
            current = iPodAudioAlsaCreateChildConfiguration(sub_pcm, SND_CONFIG_TYPE_INTEGER, IPOD_ALSA_SELEM_RESOLUT, &resolut, 0);
        }
    }
    
    if(rc == 0)
    {
        /* save config */
        conf = top;
    }
    else
    {
        if(top != NULL)
        {
            /* Free config */
            rc = snd_config_delete(top);
            if(rc == 0)
            {
                top = NULL;
            }
            else
            {
                IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_AUDIO, rc);
            }
        }
    }
    
    return conf;
}

static int iPodAudioAlsaSetCapturePlaybackParams(IPOD_AUDIO_COM_INFO *alsaInfo, snd_pcm_t **pcm_handle, snd_pcm_stream_t flag, U8 *devName)
{
    int rc = -1;
    int card_num = 0;
    int dev_num = 0;
    snd_config_t *conf = NULL;
    char input_device[IPOD_ALSA_INPUTDEV_BUF] = {0};
    
    if((alsaInfo == NULL) || (devName == NULL))
    {
        return -1;
    }
    
    if(flag == SND_PCM_STREAM_CAPTURE)
    {
        snprintf(input_device, sizeof(input_device), "%s_%c", IPOD_ALSA_INPUT_DEVICE_NAME, devName[IPOD_ALSA_HW_CARD_NUM_POS]);
        /* Gets the number of card and device */
        card_num = atoi((const char*)&devName[IPOD_ALSA_HW_CARD_NUM_POS]);
        dev_num = atoi((const char*)&devName[IPOD_ALSA_HW_DEVICE_NUM_POS]);
        /* Creates input device configuration */
        conf = iPodAudioAlsaCreateInputConfiguration(input_device, card_num, dev_num);
        if(conf != NULL)
        {
            /* Open pcm with dynamical configuration */
            rc = snd_pcm_open_lconf(pcm_handle, input_device, flag, 0, conf);
            
            /* Warning start: This should be deleted after the problem of failing to control volume at first time is solved */
            rc = snd_pcm_close(*pcm_handle);
            rc = snd_pcm_open_lconf(pcm_handle, input_device, flag, 0, conf);
            /* Warning end */
            
            /* Delete local configuration */
            iPodAudioAlsaDeleteConfiguration(conf);
        }
    }
    else
    {
        /* Open Capture or Playback Handle */
        rc = snd_pcm_open(pcm_handle, (const char *)devName, flag, 0);
    }
    if (rc == 0)
    {
        /* Set the hardware parameters of Capture handle */
        rc = iPodAudioAlsaSetHwParams(alsaInfo, *pcm_handle, alsaInfo->prate);
    }
    else
    {
        rc = IPOD_PLAYER_ERROR;
    }
    
    if (rc == IPOD_PLAYER_OK)
    {
        /* Set the software parameters of Capture handle */
        rc = iPodAudioAlsaSetSwParams(alsaInfo, *pcm_handle);
    }
    
    return rc;
}

static int iPodAudioAlsaAllocateBuffer(IPOD_AUDIO_COM_INFO *alsaInfo)
{
    int rc = IPOD_PLAYER_ERROR;
    int i = 0;
    
    if(alsaInfo == NULL)
    {
        return rc;
    }
    
    /* Allocate buffer */
    for (i = 0; i < IPOD_ALSA_BUF_NUM; i++)
    {
        /* The [chunk_bytes] is computed when setting the playback HW. */
        alsaInfo->buf[i] = malloc(alsaInfo->chunk_bytes);
        if (alsaInfo->buf[i] == NULL)
        {
            rc = IPOD_PLAYER_ERROR;
            break;
        }
        rc = IPOD_PLAYER_OK;
    }
    
    return rc;
}

static void iPodAudioAlsaDestroy(IPOD_AUDIO_COM_INFO *alsaInfo)
{
    int i = 0;
    
    if(alsaInfo == NULL)
    {
        return;
    }
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_AUDIO);
    
    /* Close the play task. */
    if (alsaInfo->playThrState == 1)
    {
        alsaInfo->playThrState = 2;
        pthread_join(alsaInfo->threadId, NULL);
    }
    
    /* close PCM handle */
    if (alsaInfo->chandle != NULL)
    {
        snd_pcm_close(alsaInfo->chandle);
        alsaInfo->chandle = NULL;
        alsaInfo->ccallback = NULL;
    }
    
    /* close PCM handle */
    if (alsaInfo->phandle != NULL)
    {
        snd_pcm_close(alsaInfo->phandle);
        alsaInfo->phandle = NULL;
        alsaInfo->pcallback = NULL;
    }
    
    /* Release the buffer */
    for (i = 0; i < IPOD_ALSA_BUF_NUM; i++)
    {
        if (alsaInfo->buf[i] != NULL)
        {
            free(alsaInfo->buf[i]);
            alsaInfo->buf[i] = NULL;
        }
    }
    /* Release the volume device. */
    iPodAudioAlsaDeleteVolume(alsaInfo);
    
    alsaInfo->status = ALSA_IDLE;
    alsaInfo->rIndex = 0;
    alsaInfo->wIndex = 0;
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_AUDIO);
}

static int iPodAudioAlsaResultOfAlsaStart(IPOD_AUDIO_COM_INFO *alsaInfo, int rc, pthread_t *threadId)
{
    if((alsaInfo == NULL) || (threadId == NULL))
    {
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    if(rc == IPOD_PLAYER_OK)
    {
        alsaInfo->status = ALSA_RUNNING;
        /* Start the play task. */
        rc = iPodAudioAlsaStartPlayTask(alsaInfo, threadId);
    }
    
    if ( rc == IPOD_PLAYER_ERROR )
    {
        /* There are some error occured, release the buffer in th iPodAudioAlsaDestroy(). */
        iPodAudioAlsaDestroy(alsaInfo);
    }
    
    return rc;
}

static int iPodAudioAlsaCreate(IPOD_AUDIO_COM_INFO *alsaInfo, U8 *srcName, U8 *sinkName, U32 rate)
{
    snd_pcm_state_t state = (snd_pcm_state_t)0;
    int i = 0;
    int rc = IPOD_PLAYER_ERROR;
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_AUDIO);
    
    if((alsaInfo == NULL) || (srcName == NULL) || (sinkName == NULL))
    {
        return rc;
    }
    
    alsaInfo->crate = rate;
    alsaInfo->prate = IPP_AUDIO_DEFAULT_RATE;
    strncpy((char *)alsaInfo->src_name, (const char *)srcName, sizeof(alsaInfo->src_name) - 1);
    strncpy((char *)alsaInfo->sink_name, (const char *)sinkName, sizeof(alsaInfo->sink_name) - 1);
    
    /* Set capture params */
    rc = iPodAudioAlsaSetCapturePlaybackParams(alsaInfo, &(alsaInfo->chandle), SND_PCM_STREAM_CAPTURE, srcName);
    if (rc == IPOD_PLAYER_OK)
    {
        /* Set playback params */
        rc = iPodAudioAlsaSetCapturePlaybackParams(alsaInfo, &(alsaInfo->phandle), SND_PCM_STREAM_PLAYBACK, sinkName);
    }
    if (rc == IPOD_PLAYER_OK)
    {
        /* Allocate buffer */
        rc = iPodAudioAlsaAllocateBuffer(alsaInfo);
    }
    if (rc == IPOD_PLAYER_OK)
    {
        /* Initialize the volume device. */
        rc = iPodAudioAlsaCreateVolume(alsaInfo, (U8 *)srcName);
    }
    if (rc == IPOD_PLAYER_OK)
    {
        /* Prepare the device */
        rc = snd_pcm_prepare(alsaInfo->chandle);
    }
    else
    {
        rc = -1;
    }
    
    if (rc == 0)
    {
        /* Prepare the device */
        rc = snd_pcm_prepare(alsaInfo->phandle);
    }
    if (rc == 0)
    {
        /* Start the device */
        state = snd_pcm_state(alsaInfo->chandle);
        if ( state == SND_PCM_STATE_PREPARED )
        {
            /* Start a PCM. */
            rc = snd_pcm_start(alsaInfo->chandle);
            if ( rc == 0 )
            {
                /* Wait for a PCM to become ready. */
                snd_pcm_wait(alsaInfo->chandle, IPOD_ALSA_WAIT_MS);
            }
        }
        else
        {
            /* Wait for a PCM to become ready. */
            snd_pcm_wait(alsaInfo->chandle, IPOD_ALSA_WAIT_MS);
        }
    }
    if (rc == 0)
    {
        /* Read first chunks of data */
        for (i = 0; i < IPOD_ALSA_BUF_NUM / 2; i++)
        {
            rc = iPodAudioAlsaReadOneChunk(alsaInfo);
            if (rc == IPOD_PLAYER_OK)
            {
                rc = 0;
            }
            else
            {
                rc = -1;
                break;
            }
        }
    }
    
    if (rc == 0)
    {
        /* Wait for a PCM to become ready. */
        snd_pcm_wait(alsaInfo->phandle, IPOD_ALSA_WAIT_MS);
    }
    if (rc == 0)
    {
        /* Write first chunks of data */
        for (i = 0; i < IPOD_ALSA_BUF_NUM / 4; i++)
        {
            rc = iPodAudioAlsaWriteOneChunk(alsaInfo);
            if (rc == IPOD_PLAYER_OK)
            {
                rc = 0;
            }
            else
            {
                rc = -1;
                break;
            }
        }
    }
    if (rc == 0)
    {
        /* Start the device */
        state = snd_pcm_state(alsaInfo->phandle);
        if ( state == SND_PCM_STATE_PREPARED )
        {
            /* Start a PCM. */
            rc = snd_pcm_start(alsaInfo->phandle);
        }
    }
    
    if(rc == 0)
    {
        rc = IPOD_PLAYER_OK;
    }
    else
    {
        rc = IPOD_PLAYER_ERROR;
    }
    
    rc = iPodAudioAlsaResultOfAlsaStart(alsaInfo, rc, &alsaInfo->threadId);
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_AUDIO, rc);
    
    return rc;
}

S32 iPodAudioAlsaSetSamplerate(IPOD_AUDIO_COM_INFO *alsaInfo, U32 rate)
{
    S32 rc = IPOD_PLAYER_ERROR;
    
    /* Parameter check */
    if (alsaInfo == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Set samplerate */
    if(alsaInfo->crate != rate)
    {
        iPodAudioAlsaDestroy(alsaInfo);
        rc = iPodAudioAlsaCreate(alsaInfo, alsaInfo->src_name, alsaInfo->sink_name, rate);
    }
    
    return rc;
}

S32 iPodAudioAlsaStart(IPOD_AUDIO_COM_INFO **alsaInfo, U8 *srcName, U8 *sinkName, U32 rate)
{
    S32 rc = IPOD_PLAYER_ERROR;
    
    /* Parameter check */
    if((alsaInfo == NULL) || (srcName == NULL) || (sinkName == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, alsaInfo, srcName, sinkName);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Allocate for alsa information */
    *alsaInfo= calloc(1, sizeof(**alsaInfo));
    if(*alsaInfo != NULL)
    {
        /* Create alsa src and sink module */
        rc = iPodAudioAlsaCreate(*alsaInfo, srcName, sinkName, rate);
        if(rc != IPOD_PLAYER_OK)
        {
            free(*alsaInfo);
            *alsaInfo = NULL;
        }
    }
    
    return rc;
}

void iPodAudioAlsaStop(IPOD_AUDIO_COM_INFO *alsaInfo)
{
    /* Parameter check */
    if(alsaInfo == NULL)
    {
        return;
    }
    
    /* Remove alsa src and sink module */
    iPodAudioAlsaDestroy(alsaInfo);
    /* Free allocated memory */
    free(alsaInfo);
    
    return;
}

S32 iPodAudioComInit(IPOD_AUDIO_COM_FUNC_TABLE *table)
{
    /* Parameter check */
    if(table == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, table);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Set function pointer */
    table->start = &iPodAudioAlsaStart;
    table->stop = &iPodAudioAlsaStop;
    table->setVolume = &iPodAudioAlsaSetVolume;
    table->getVolume = &iPodAudioAlsaGetVolume;
    table->setSample = &iPodAudioAlsaSetSamplerate;
    
    return IPOD_PLAYER_OK;
}

void iPodAudioComDeinit(IPOD_AUDIO_COM_FUNC_TABLE *table)
{
    /* Parameter check */
    if(table == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, table);
        return;
    }
    
    /* Remove function pointer */
    table->start = NULL;
    table->stop = NULL;
    table->setVolume = NULL;
    table->getVolume = NULL;
    table->setSample = NULL;
    
    return;
}

