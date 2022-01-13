/**
 * \file: uac2Monitor.c
 *
 * \version: $Id:$
 *
 * \release: $Name:$
 *
 * cinemo audio streaming monitoring sample rate change events
 *
 * \component: ipod cinemo
 *
 * \author: Vanitha.channaiah@in.bosch.com
 *
 * \copyright (c) 2019 Advanced Driver Information Technology.
 * This code is developed by Advanced Driver Information Technology.
 * Copyright of Advanced Driver Information Technology, Bosch, and DENSO.
 * All rights reserved.
 *
 * \see <related items>
 *
 * \history
 *
 ***********************************************************************/
#include "uac2Monitor.h"

int getUac2ElemVal(uac2_ctrl *ctrl, uint32_t idx)
{
    if ((ctrl == NULL) || (idx >= CTRL_UAC2_ELEMENTS))
    {
        IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "CINEMO_AUD: getUac2ElemVal() ctrl is NULL");
        return -1;
    }
    return ctrl->elems_val[idx];
}

int uac2CtlElemInit(uac2_ctrl *ctrl, uint32_t idx, const char * name)
{
    int err;

    if ((ctrl == NULL) || (idx >= CTRL_UAC2_ELEMENTS) || (name == NULL))
    {
        return -1;
    }
    snd_ctl_elem_value_t **elem = &ctrl->elems[idx];
    err = snd_ctl_elem_value_malloc(elem);
    if (err < 0)
    {
        IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "CINEMO_AUD: ERROR: Unable to malloc: %s", snd_strerror(err));
    }
    else
    {
        snd_ctl_elem_value_set_interface(*elem, SND_CTL_ELEM_IFACE_CARD);
        snd_ctl_elem_value_set_name(*elem, name);
    }
    return uac2CtlElemUpdate(ctrl, idx);
}

int uac2CtlElemUpdate(uac2_ctrl *ctrl, uint32_t idx)
{
    int err = 0;
    if (ctrl && idx < CTRL_UAC2_ELEMENTS)
    {
        snd_ctl_elem_value_t **elem = &ctrl->elems[idx];
        err = snd_ctl_elem_read(ctrl->ctl, *elem);
        if (err < 0)
        {
            IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "CINEMO_AUD: Unable to read contents: %s", snd_strerror(err));
        }
        ctrl->elems_val[idx] = snd_ctl_elem_value_get_integer(*elem, 0);
        IAP2USBPLUGINDLTLOG(DLT_LOG_DEBUG, "CINEMO_AUD: element updated Idx %d to val %ld",idx, ctrl->elems_val[idx]);
    }
    return err;
}

int uac2CtrlInit(uac2_ctrl *ctrl)
{
    int err;

    err = snd_ctl_open(&ctrl->ctl, "hw:UAC2Gadget", SND_CTL_READONLY);
    if (err)
    {
        ctrl->ctl = NULL;
        return err;
    }
    err = snd_ctl_subscribe_events(ctrl->ctl, 1);
    if (err)
    {
        snd_ctl_close(ctrl->ctl);
        ctrl->ctl = NULL;
        return err;
    }

    err = uac2CtlElemInit(ctrl, CTRL_UAC2_ELEM_C_RATE, SND_CTL_NAME_CAPTURE"sample rate");
    if (err < 0)
    {
        IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "CINEMO_AUD: ERROR to initialize and read contents of capture SR: %s", snd_strerror(err));
    }
    err = uac2CtlElemInit(ctrl, CTRL_UAC2_ELEM_P_RATE, SND_CTL_NAME_PLAYBACK"sample rate");
    if (err < 0)
    {
        IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "CINEMO_AUD: ERROR to initialize and read contents of playback SR: %s", snd_strerror(err));
    }
    err = uac2CtlElemInit(ctrl, CTRL_UAC2_ELEM_P_ENABLED, SND_CTL_NAME_PLAYBACK"enabled");
    if (err < 0)
    {
        IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "CINEMO_AUD: ERROR to initialize and read contents of playback enabled: %s", snd_strerror(err));
    }
    err = uac2CtlElemInit(ctrl, CTRL_UAC2_ELEM_C_ENABLED, SND_CTL_NAME_CAPTURE"enabled");
    if (err < 0)
    {
        IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "CINEMO_AUD: ERROR to initialize and read contents of capture enabled: %s", snd_strerror(err));
    }

    return err;
}

void uac2ElemFree(uac2_ctrl *ctrl, uint32_t Idx)
{
    if (ctrl && Idx < CTRL_UAC2_ELEMENTS)
    {
        snd_ctl_elem_value_t *elem = ctrl->elems[Idx];
        if (elem != NULL)
        {
            snd_ctl_elem_value_free(elem);
        }
    }
    else
    {
        IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "CINEMO_AUD: snd_ctl_elem_value_free NOT freed");
    }
}

int uac2CtrlDeInit(uac2_ctrl *ctrl)
{
    int err = 0;

    if (NULL == ctrl)
    {
        err = -1;
    }
    else
    {
        uac2ElemFree(ctrl, CTRL_UAC2_ELEM_P_RATE);
        uac2ElemFree(ctrl, CTRL_UAC2_ELEM_C_RATE);
        uac2ElemFree(ctrl, CTRL_UAC2_ELEM_P_ENABLED);
        uac2ElemFree(ctrl, CTRL_UAC2_ELEM_C_ENABLED);

        if (NULL != ctrl->ctl)
        {
            snd_ctl_close(ctrl->ctl);
            ctrl->ctl = NULL;
        }
    }

    return err;
}
void* UAC2DeviceMonitorThread(void *arg)
{
    audioHandle *phandle = (audioHandle *)arg;
    uint32_t uac2CaptureSampleRate = 0;
    uint32_t uac2CaptureEnabled = 0;
    uint32_t uac2PlaybackEnabled = 0;
    uint32_t uac2PlaybackSampleRate = 0;
    uint32_t oldSampleRate = 0;
    uint32_t oldCapEPEnabled = 0;
    const uint64_t ev = 1;
    int err = 0;
    int ret = 0;
    snd_ctl_event_t *uac2CtlEvent = NULL;
    snd_ctl_event_type_t uac2EventType;
    bool uac2GetEvent = false;
    uac2_ctrl ctrl;
    memset( &ctrl, 0, sizeof(uac2_ctrl));

    if (phandle == NULL)
    {
        IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "CINEMO_AUD: UAC2DeviceMonitorThread with NULL phandle");
        return NULL;
    }

    err = snd_ctl_event_malloc(&uac2CtlEvent);
    if (err != 0)
    {
        IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "CINEMO_AUD:  snd_ctl_event_malloc()  FAILED");
        uac2CtlEvent = NULL;
    }
    else if (NULL == uac2CtlEvent)
    {
        IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "CINEMO_AUD:  uac2CtlEvent is NULL");
    }

    if (err == 0)
    {
        err = uac2CtrlInit(&ctrl);
        if (0 != err)
        {
            IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "CINEMO_AUD:  UAC2DeviceMonitorThread:: ERROR: uac2CtrlInit() FAILED");
        }
        else
        {
            oldSampleRate = getUac2ElemVal(&ctrl, CTRL_UAC2_ELEM_C_RATE);
            oldCapEPEnabled = getUac2ElemVal(&ctrl, CTRL_UAC2_ELEM_C_ENABLED);
        }
    }

    if (phandle)
    {
        phandle->uac2capturesrate = oldSampleRate;
        phandle->CaptureEndPoint = oldCapEPEnabled;
        IAP2USBPLUGINDLTLOG(DLT_LOG_DEBUG, "CINEMO_AUD: Initial params updated are Rate: %d CapEP: %d ", oldSampleRate, oldCapEPEnabled);
    }

    while (phandle->uac2Monitor)
    {
        uac2GetEvent = false;

        /* Monitor for rate change */
        err = snd_ctl_wait( ctrl.ctl, 500);
        if (err < 0)
        {
            IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "CINEMO_AUD: snd_ctl_wait() returned error: %d", err);
            break;
        }
        else if (err == 0)
        {
/*            IAP2USBPLUGINDLTLOG(DLT_LOG_VERBOSE, "CINEMO_AUD: snd_ctl_wait() returned timedout: %d", err);*/
        }
        else
        {
            IAP2USBPLUGINDLTLOG(DLT_LOG_DEBUG, "CINEMO_AUD: vMonitorThreadFunction:: snd_ctl_wait() returned events: %d", err);
            err = snd_ctl_read(ctrl.ctl, uac2CtlEvent);
            if (err < 0)
            {
                IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "CINEMO_AUD:  uac2CtlEvent event read FAILED %d", err);
                break;
            }
            else if (err == 0)
            {
                IAP2USBPLUGINDLTLOG(DLT_LOG_DEBUG, "CINEMO_AUD:  uac2CtlEvent events NONE");
            }
            else
            {
                IAP2USBPLUGINDLTLOG(DLT_LOG_DEBUG, "CINEMO_AUD:  uac2CtlEvent got event: %d", err);
                uac2EventType = snd_ctl_event_get_type(uac2CtlEvent);
                if (uac2EventType == SND_CTL_EVENT_ELEM)
                {
                    uac2GetEvent = true;
                }
            }
        }
        if (uac2GetEvent == true)
        {
            uac2CtlElemUpdate(&ctrl, CTRL_UAC2_ELEM_C_RATE);
            uac2CaptureSampleRate = getUac2ElemVal(&ctrl, CTRL_UAC2_ELEM_C_RATE);

            uac2CtlElemUpdate(&ctrl, CTRL_UAC2_ELEM_P_RATE);
            uac2PlaybackSampleRate = getUac2ElemVal(&ctrl, CTRL_UAC2_ELEM_P_RATE);

            uac2CtlElemUpdate(&ctrl, CTRL_UAC2_ELEM_C_ENABLED);
            uac2CaptureEnabled = getUac2ElemVal(&ctrl, CTRL_UAC2_ELEM_C_ENABLED);

            uac2CtlElemUpdate(&ctrl, CTRL_UAC2_ELEM_P_ENABLED);
            uac2PlaybackEnabled = getUac2ElemVal(&ctrl, CTRL_UAC2_ELEM_P_ENABLED);

            IAP2USBPLUGINDLTLOG(DLT_LOG_DEBUG, "CINEMO_AUD:CTRL_UAC2_ELEM_C_ENABLED = %d, old:%d, CTRL_UAC2_ELEM_C_RATE = %d, old:%d,",
                    uac2CaptureEnabled, uac2CaptureEnabled, uac2CaptureSampleRate, oldSampleRate);
            IAP2USBPLUGINDLTLOG(DLT_LOG_DEBUG, "CINEMO_AUD:CTRL_UAC2_ELEM_P_ENABLED = %d, CTRL_UAC2_ELEM_P_RATE = %d",
                    uac2PlaybackEnabled, uac2PlaybackSampleRate);

            if (audio_lock(phandle))
            {
                if (uac2CaptureEnabled == 0 && (oldSampleRate == uac2CaptureSampleRate))
                {
                    phandle->aborting = true;
                    err = audioBackend_abort();
                    if (err)
                    {
                        IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "CINEMO_AUD: audioBackend_abort FAILED %d", err);
                    }
                    phandle->uac2states = CEP_DISABLED;
                    int rc = write(phandle->uac2EvntFd, &ev, sizeof(uint64_t));
                    if (err != sizeof(uint64_t))
                    {
                        IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "CINEMO_AUD: write ret =  %d | errno: %d %s",rc, errno, strerror(errno));
                    }
                } else if (uac2CaptureEnabled == 1 && (oldSampleRate == uac2CaptureSampleRate))
                {
                    IAP2USBPLUGINDLTLOG(DLT_LOG_DEBUG,"uac2 enabled and should start the AIL streaming");
                    phandle->uac2states = CEP_ENABLED;
                    int rc = write(phandle->uac2EvntFd, &ev, sizeof(uint64_t));
                    if (rc != sizeof(uint64_t))
                    {
                        IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "CINEMO_AUD: write ret =  %d | errno: %d %s",rc, errno, strerror(errno));
                    }
                } else if (uac2CaptureEnabled == 0 && (oldSampleRate != uac2CaptureSampleRate))
                {
                    phandle->aborting = true;
                    /* close stream when rate chnages */
                    int err = Audio_StopClose_Custom(phandle);
                    if (err < 0)
                    {
                        IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "CINEMO_AUD: Audio_StopClose_Custom FAILED %d", err);
                    }
                    phandle->uac2states = RATE_CHANGE_DISABLED;
                    phandle->uac2capturesrate = oldSampleRate = uac2CaptureSampleRate;
                    int rc = write(phandle->uac2EvntFd, &ev, sizeof(uint64_t));
                    if (rc != sizeof(uint64_t))
                    {
                        IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "CINEMO_AUD: write ret =  %d | errno: %d %s",rc, errno, strerror(errno));
                    }
                } else if (uac2CaptureEnabled == 1 && (oldSampleRate != uac2CaptureSampleRate))
                {
                    phandle->aborting = true;
                    /* close stream when rate chnages */
                    err = Audio_StopClose_Custom(phandle);
                    if (err < 0)
                    {
                        IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "CINEMO_AUD: Audio_StopClose_Custom FAILED %d", err);
                    }
                    phandle->uac2states = RATE_CHANGE_ENABLED;
                    phandle->uac2capturesrate = oldSampleRate = uac2CaptureSampleRate;
                    int rc = write(phandle->uac2EvntFd, &ev, sizeof(uint64_t));
                    if (rc != sizeof(uint64_t))
                    {
                        IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "CINEMO_AUD: write ret =  %d | errno: %d %s",rc, errno, strerror(errno));
                    }
                }
                audio_unLock(phandle);
            }
        }
    }

    uac2CtrlDeInit(&ctrl);
    if (uac2CtlEvent != NULL)
    {
        snd_ctl_event_free(uac2CtlEvent);
        uac2GetEvent = NULL;
    }
    IAP2USBPLUGINDLTLOG(DLT_LOG_DEBUG, "CINEMO_AUD: UAC2DeviceMonitorThread EXIT");
    pthread_exit(NULL);

}
int UAC2DeviceMonitorCreateThread(audioHandle *tphandle)
{
    int err = -1;

    if (tphandle != NULL)
    {
        audioHandle *phandle = tphandle;
        err = pthread_create(&phandle->uac2Id, NULL, UAC2DeviceMonitorThread, phandle);
        if (err == 0)
        {
            IAP2USBPLUGINDLTLOG(DLT_LOG_DEBUG, "CINEMO_AUD: UAC2DeviceMonitorCreateThread created");
        }
        else
        {
            IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "CINEMO_AUD: UAC2DeviceMonitorCreateThread FAILED");
        }
    }
    else
    {
        IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "CINEMO_AUD:  UAC2DeviceMonitorCreateThread FAILED tphandle NULL");
    }
    return err;
}

int UAC2DeviceMonitorJoinThread(audioHandle *tphandle)
{
    IAP2USBPLUGINDLTLOG(DLT_LOG_DEBUG, "CINEMO_AUD: UAC2DeviceMonitorJoinThread");
    audioHandle *phandle = tphandle;
    int err = -1;

    if (phandle->uac2Id)
    {
        err = pthread_join(phandle->uac2Id, NULL);
        if (err != 0)
        {
            IAP2USBPLUGINDLTLOG(DLT_LOG_DEBUG, "CINEMO_AUD: uac2 pthread_join failed with err:%d | %d %s", err, errno, strerror(errno));
        }
    }
    return err;
}
