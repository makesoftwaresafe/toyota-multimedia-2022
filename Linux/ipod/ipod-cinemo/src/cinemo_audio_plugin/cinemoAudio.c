/**
 * \file: cinemoAudio.c
 *
 * \version: $Id:$
 *
 * \release: $Name:$
 *
 * cinemo audio streaming API's Implementation
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

#include "iap2_cinemo_host_datacom.h"
#include "cinemoAudio.h"
#include "pfcfgCinemo.h"

static FILE *capturefp = NULL;
void OpenCaptureDataDump(char* dumpPath)
{
    if (capturefp == NULL && dumpPath != NULL)
    {
        capturefp  = fopen(dumpPath ,"ab+");
        if (capturefp == NULL)
        {
            IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "capture file opened failed");
        }
    }
    else
    {
        IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "capture file opened failed because capture fp is %p and dumpPath is %p",capturefp, dumpPath);
    }
}

void CloseCaptureDataDump()
{
    if (capturefp)
    {
        fclose(capturefp);
    }
    capturefp = NULL;
}

static inline int getMaxFd(int x, int y)
{
    return ((x > y) ? x : y);
}

static int Audio_CreateEventFds(audioHandle *phandle)
{
    if (phandle == NULL)
    {
        return -1;
    }
    phandle->abortFd = eventfd(0, 0);
    if (phandle->abortFd == -1)
    {
        IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "CINEMO_AUD: abortFd: eventfd creation failed");
        return -1;
    }

    phandle->eosFd = eventfd(0, 0);
    if (phandle->eosFd == -1)
    {
        IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "CINEMO_AUD: eosFd: eventfd creation failed");
        return -1;
    }

    phandle->uac2EvntFd = eventfd(0, 0);
    if (phandle->uac2EvntFd == -1)
    {
        IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "CINEMO_AUD: uac2EvntFd: eventfd creation failed");
        return -1;
    }

    phandle->processFd = eventfd(0, 0);
    if (phandle->processFd == -1)
    {
        IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "CINEMO_AUD: processFd: eventfd creation failed");
        return -1;
    }
    return 0;
}

/**
 * This function will be called If Cinemo needs to start streaming
 * It will be called before any audio related functions are called.
 * @param[IN] tphandle       Transporthandle.
 * @return CTLI_NO_ERROR     If Audio Backend and Alsa device is opened successfully
 * @return CTLI_ERROR_OPEN   If Audio Backend and Alsa device is not opened
 */
int Audio_Create_Custom(ctli_handle tphandle)
{
    int err = CTLI_NO_ERROR;
    capturefp = NULL;
    audioHandle *phandle = NULL;
    char *cap_dump = NULL;
    unsigned int bufMaxLen = 0;
    unsigned char num_cfg = 0;

    if (tphandle == NULL)
    {
        IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "CINEMO_AUD: Audio_Create_Custom returns CTLI_ERROR_ARGUMENTS");
        return CTLI_ERROR_ARGUMENTS;
    }
    IPOD_IAP2_CINEMO_HOSTDEV_INFO *devinfo = (IPOD_IAP2_CINEMO_HOSTDEV_INFO *) tphandle;

    phandle = (audioHandle *) calloc(1, sizeof(audioHandle));
    if (phandle == NULL)
    {
        IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "CINEMO_AUD: Audio_Create_Custom: malloc phandle failed");
        return CTLI_ERROR_RESOURCES;
    }

    err = pthread_mutex_init(&phandle->audioMutex, NULL);
    if (err < 0)
    {
        IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "CINEMO_AUD: pthread_mutex_init failed : %d: %s", err, strerror(errno));
        return CTLI_ERROR_RESOURCES;
    }

    /* create event fd's */
    err = Audio_CreateEventFds(phandle);
    if (err < 0)
    {
        IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "CINEMO_AUD: Audio_CreateEventFds failed");
        return CTLI_ERROR_RESOURCES;
    }

    phandle->dev = (deviceParams*) calloc(1, sizeof(deviceParams));
    if (phandle->dev == NULL)
    {
        IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "CINEMO_AUD: Audio_Create_Custom: malloc phandle->dev failed");
        return CTLI_ERROR_RESOURCES;
    }

    CINEMO_AUDIO_Cfg *cfgInfo = (CINEMO_AUDIO_Cfg *)cinemoAudioGetPfcfgParameter(&num_cfg);
    if (cfgInfo[CINEMO_AUDIO_ID_DEV_NAME].para.p_val != NULL)
    {
        phandle->dev->captureDevice = cfgInfo[CINEMO_AUDIO_ID_DEV_NAME].para.p_val;
    }
    else
    {
        phandle->dev->captureDevice = "hw:UAC2Gadget,0";
    }

    if (cfgInfo[CINEMO_AUDIO_ID_DUMP].para.p_val != NULL)
    {
        cap_dump = cfgInfo[CINEMO_AUDIO_ID_DUMP].para.p_val;
    }

    if (cfgInfo[CINEMO_AUDIO_ID_PERIOFRAMES].para.val)
    {
        phandle->dev->periodFrames = cfgInfo[CINEMO_AUDIO_ID_PERIOFRAMES].para.val;
    }
    else
    {
        phandle->dev->periodFrames = DEFAULT_PERIOD_FRAMES;
    }

    if (cfgInfo[CINEMO_AUDIO_ID_SAMPLETYPEBITS].para.val)
    {
        phandle->dev->audioParams.bits = cfgInfo[CINEMO_AUDIO_ID_SAMPLETYPEBITS].para.val;
    }
    else
    {
        phandle->dev->audioParams.bits = DEFAULT_SAMPLETYPEBITS;
    }

    if (cfgInfo[CINEMO_AUDIO_ID_CHANNEL].para.val)
    {
        phandle->dev->audioParams.channels = cfgInfo[CINEMO_AUDIO_ID_CHANNEL].para.val;
    }
    else
    {
        phandle->dev->audioParams.channels = DEFAULT_CHANNEL;
    }

    if (cfgInfo[CINEMO_AUDIO_ID_BUFFERSIZE].para.val)
    {
        bufMaxLen = cfgInfo[CINEMO_AUDIO_ID_BUFFERSIZE].para.val;
    }
    else
    {
        bufMaxLen = DEFAULT_BUFFER_SIZE;
    }

    IAP2USBPLUGINDLTLOG(DLT_LOG_DEBUG, "CINEMO_AUD: dev: %s bufsize:%d ps:%d sampletype:%d ch:%d",
            phandle->dev->captureDevice , bufMaxLen,
            phandle->dev->periodFrames, phandle->dev->audioParams.bits, phandle->dev->audioParams.channels);

    phandle->buffer = (char *) malloc(bufMaxLen * sizeof(char));
    if (phandle->buffer == NULL)
    {
        IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "CINEMO_AUD: Audio_Create_Custom : malloc phandle->buffer failed");
        return CTLI_ERROR_RESOURCES;
    }

    phandle->cbuf = bufferInit(phandle->buffer, bufMaxLen);
    if (phandle->cbuf == NULL)
    {
        IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "CINEMO_AUD: Audio_Create_Custom : buffer_init failed");
        return CTLI_ERROR_RESOURCES;
    }

    phandle->uac2Monitor = true;
    err = UAC2DeviceMonitorCreateThread(phandle);
    if (err < 0)
    {
        return CTLI_ERROR_RESOURCES;
    }

    usleep(100000);/* 100ms wait to get sample rate info from uac2 */

    if (devinfo)
    {
        devinfo->audioctx = (audioHandle *) phandle;
    }

    phandle->audioState = CLOSED;
    phandle->dev->playbackDevice = "";
    phandle->dev->audioParams.codec = CTLI_AUDIO_CODEC_PCM;
    phandle->dev->audioParams.container = CTLI_AUDIO_CONTAINER_RAW;

    if (phandle->dev->audioParams.bits == 16)
    {
        phandle->dev->audioParams.sampletype = CTLI_AUDIO_SAMPLETYPE_S16LE;
    }
    else if (phandle->dev->audioParams.bits == 24)
    {
        phandle->dev->audioParams.sampletype = CTLI_AUDIO_SAMPLETYPE_S24LE;
    }
    else
    {
        phandle->dev->audioParams.sampletype = CTLI_AUDIO_SAMPLETYPE_UNSPECIFIED;
    }

    err = audioBackend_create(tphandle);
    if (err)
    {
        IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "CINEMO_AUD: Audio Backend Object is NULL");
        return CTLI_ERROR_OPEN;
    }

    if (cap_dump)
    {
        IAP2USBPLUGINDLTLOG(DLT_LOG_DEBUG, "CINEMO_AUD: Audio dump available at: %s", cap_dump);
        OpenCaptureDataDump(cap_dump);
    }

    IAP2USBPLUGINDLTLOG(DLT_LOG_DEBUG, "CINEMO_AUD: Audio_Create_Custom() success with audio handle: %p", devinfo->audioctx);
    return err;
}

/**
 * audio backend stop and close
 * This function is used when capture point is disabled
 * and when there is a rate change
 */
int Audio_StopClose_Custom(audioHandle *phandle)// do this when stop and needs to opened again
{

    if (!phandle->dev)
    {
        return -1;
    }

    int err = audioBackend_abort();
    if (err)
    {
        IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "CINEMO_AUD: audioBackend_abort failed");
        return -1;
    }

    err = audioBackend_close();
    if (err)
    {
        IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "CINEMO_AUD: audioBackend_close failed");
        return -1;
    }
    audioBufferReset(phandle->cbuf);

    IAP2USBPLUGINDLTLOG(DLT_LOG_DEBUG, "CINEMO_AUD: Audio_StopClose_Custom success");
    return 0;
}

void updateAudioState(audioHandle *phandle, states audioState)
{
    if (audio_lock(phandle))
    {
        phandle->audioState = audioState;
        audio_unLock(phandle);
    }
}

static int handle_newparams(audioHandle *phandle)
{
    int err = CTLI_NO_ERROR;

    if (audio_lock(phandle))
    {
        if (phandle->audioState == ABORTED)
        {
            IAP2USBPLUGINDLTLOG(DLT_LOG_DEBUG, "CINEMO_AUD: Audio_Receive_Custom Already aborted");
            err = CTLI_ERROR_CANCEL;
        }
        else if ((phandle->audioState == CLOSED) && (phandle->dev->audioParams.samplerate != phandle->uac2capturesrate))
        {
            err = CTLI_ERROR_AUDIOPARAMS;
            phandle->audioState = NEWAUDIOPARAMS;
        }
        else if (phandle->audioState == NEWAUDIOPARAMS)
        {
            int err = audioBackend_open(phandle->dev);
            if (err)
            {
                IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "CINEMO_AUD: audioBackend_open FAILED");
                err = CTLI_ERROR_OPEN;
            }
            else
            {
                phandle->audioState = OPENED;
            }

            if (phandle->uac2states == CEP_ENABLED && phandle->audioState == OPENED)
            {
                phandle->aborting = false;
                IAP2USBPLUGINDLTLOG(DLT_LOG_DEBUG, "CINEMO_AUD:started stream handle_newparams");
                err = audioBackend_stream();
                if (err)
                {
                    IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "CINEMO_AUD: start stream FAILED and stopped");
                    err = CTLI_ERROR_CANCEL;
                }
                phandle->audioState = RUNNING;
            }
        }

        audio_unLock(phandle);
    }

    return err;
}

static int handle_uac2states(audioHandle *phandle)
{
    int err = CTLI_NO_ERROR;

    if (phandle)
    {
        switch (phandle->uac2states)
        {
            case CEP_ENABLED:
            {
                if (phandle->dev->audioParams.samplerate != phandle->uac2capturesrate)
                {
                    IAP2USBPLUGINDLTLOG(DLT_LOG_DEBUG, "CINEMO_AUD: cp:1 RC=1");
                    phandle->audioState = NEWAUDIOPARAMS;
                    err = CTLI_ERROR_AUDIOPARAMS;
                }
                else
                {
                    /* start streaming when end point enabled */
                    phandle->aborting = false;
                    err = audioBackend_stream();
                    if (err)
                    {
                        IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "CINEMO_AUD: start stream FAILED and stopped");
                        err = CTLI_ERROR_CANCEL;
                    }
                    phandle->audioState = RUNNING;
                }
                break;
            }
            case CEP_DISABLED:
            {
                phandle->audioState = STOPPED;
                if (phandle->dev->audioParams.samplerate != phandle->uac2capturesrate)
                {
                    IAP2USBPLUGINDLTLOG(DLT_LOG_DEBUG, "CINEMO_AUD: cp:0 RC=1");
                    phandle->audioState = NEWAUDIOPARAMS;
                    err = CTLI_ERROR_AUDIOPARAMS;
                }
                break;
            }
            case RATE_CHANGE_DISABLED:
            case RATE_CHANGE_ENABLED:
            {
                phandle->audioState = NEWAUDIOPARAMS;
                err = CTLI_ERROR_AUDIOPARAMS;
                break;
            }
            default:
            {
                IAP2USBPLUGINDLTLOG(DLT_LOG_DEBUG, "CINEMO_AUD: state of uac2: %d", phandle->uac2states);
                break;
            }
        }
    }
    else
    {
        err = CTLI_ERROR_ARGUMENTS;
    }
    return err;
}

static unsigned int dataTransfer(audioHandle *phandle, void* pdest, uint32_t npdest)
{
    uint32_t transferred = 0;
    if (phandle == NULL || pdest == NULL || npdest < 1)
    {
        return CTLI_ERROR_ARGUMENTS;
    }
#if DEBUG
    printDataStatus(phandle->cbuf);
#endif
    transferred = bufferPopData(phandle->cbuf, (unsigned char *)pdest, npdest);
    if (transferred)
    {
        if (capturefp)
        {
            fwrite(pdest, transferred, 1, capturefp);
        }
    }

    return transferred;
}


/**
 * This function will be called if Cinemo needs to
 * receive audio data from the device
 * @param[IN]  tphandle                  Transport handle.
 * @param[IN]  npdest                    Size of audio receive buffer in Bytes
 * @param[OUT] pdest                     Pointer to audio receive buffer.
 * @return transferred                   Number of bytes of audio data transferred
 * @return CTLI_ERROR_ARGUMENTS          If arguments are NULL or npdest < 1
 * @return CTLI_ERROR_IO                 If system related error
 * @return CTLI_ERROR_NOTCONNECTED       If Device is not connected
 * @return CTLI_ERROR_CANCEL             If Aborted
 * @return CTLI_ERROR_AUDIOPARAMS        If new audio params have arrived
 * Error Not handled: CTLI_ERROR_IDLE    If Audio is suspended Temporarily
 */
int Audio_Receive_Custom(ctli_handle tphandle, void* pdest, uint32_t npdest)
{
    IPOD_IAP2_CINEMO_HOSTDEV_INFO *hdl = (IPOD_IAP2_CINEMO_HOSTDEV_INFO *)tphandle;
    audioHandle *phandle = (audioHandle *) hdl->audioctx;

    int err = CTLI_NO_ERROR;
    uint32_t transferred = 0;
    fd_set rFds;
    int maxFd = 0;
    int ret = 0;
    uint64_t eftd_ctr = 0;

    if (phandle == NULL || pdest == NULL || npdest < 1)
    {
        IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "CINEMO_AUD: Audio_Receive_Custom FAILED with CTLI_ERROR_ARGUMENTS");
        return CTLI_ERROR_ARGUMENTS;
    }

    err = handle_newparams(phandle);
    if (err == CTLI_NO_ERROR)
    {
        while ((err == CTLI_NO_ERROR) && (phandle->audioState != ABORTED))
        {
            FD_ZERO(&rFds);
            FD_SET(phandle->abortFd, &rFds);
            FD_SET(phandle->eosFd, &rFds);
            FD_SET(phandle->uac2EvntFd, &rFds);
            FD_SET(phandle->processFd, &rFds);
            maxFd = getMaxFd(phandle->abortFd, phandle->eosFd);
            maxFd = getMaxFd(maxFd, phandle->uac2EvntFd);
            maxFd = getMaxFd(maxFd, phandle->processFd) + 1;

            int ret = select(maxFd, &rFds, NULL, NULL, NULL);
            if (ret < 0)
            {
                IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "CINEMO_AUD: select() returns error : %d", ret);
                err = CTLI_ERROR_IO;
            }
            else if (ret == 0)
            {
                /* should be reached in case we use select with timeout */
            }
            else
            {
                if (audio_lock(phandle))
                {
                    if ((err == CTLI_NO_ERROR) && (FD_ISSET(phandle->eosFd, &rFds) && phandle->audioState == EOS))
                    {
                        IAP2USBPLUGINDLTLOG(DLT_LOG_DEBUG, "CINEMO_AUD: EOS FD");
                        err = read(phandle->eosFd, &eftd_ctr, sizeof(eftd_ctr));
                        if (err != sizeof(eftd_ctr))
                        {
                            IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "CINEMO_AUD: read ret =  %d | errno: %d %s", err, errno, strerror(errno));
                            err = CTLI_ERROR_IO;
                        }
                        else
                        {
                            err = CTLI_ERROR_NOTCONNECTED;
                        }
                    }

                    if ((err == CTLI_NO_ERROR) && (FD_ISSET(phandle->abortFd, &rFds) && phandle->audioState == ABORTED))
                    {
                        IAP2USBPLUGINDLTLOG(DLT_LOG_DEBUG, "CINEMO_AUD: ABORT FD");
                        err = read(phandle->abortFd, &eftd_ctr, sizeof(eftd_ctr));
                        err = CTLI_ERROR_CANCEL;
                    }

                    if ((err == CTLI_NO_ERROR) && (FD_ISSET(phandle->uac2EvntFd, &rFds)))
                    {
                        IAP2USBPLUGINDLTLOG(DLT_LOG_DEBUG, "CINEMO_AUD: Uac2EVENT FD");
                        err = read(phandle->uac2EvntFd, &eftd_ctr, sizeof(eftd_ctr));
                        if (err != sizeof(eftd_ctr))
                        {
                            IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "CINEMO_AUD: read ret =  %d | errno: %d %s", err, errno, strerror(errno));
                            err = CTLI_ERROR_IO;
                        }
                        else
                        {
                            IAP2USBPLUGINDLTLOG(DLT_LOG_DEBUG, "CINEMO_AUD: handle_uac2states: uac:%d audio:%d", phandle->uac2states, phandle->audioState);
                            err = handle_uac2states(phandle);
                            if (err < 0)
                            {
                                IAP2USBPLUGINDLTLOG(DLT_LOG_DEBUG, "CINEMO_AUD: handle_uac2states() returned with err: %d", err);
                            }
                        }
                    }

                    if ((err == CTLI_NO_ERROR) && (FD_ISSET(phandle->processFd, &rFds)) && (phandle->audioState == RUNNING))
                    {
                        err = read(phandle->processFd, &eftd_ctr, sizeof(eftd_ctr));
                        if (err != sizeof(eftd_ctr))
                        {
                            IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "CINEMO_AUD: read ret =  %d | errno: %d %s",err, errno, strerror(errno));
                            err = CTLI_ERROR_IO;
                        }
                        else
                        {
                            err = dataTransfer(phandle, pdest, npdest);
                            if (err > 0)
                            {
/*                                IAP2USBPLUGINDLTLOG(DLT_LOG_VERBOSE, "CINEMO_AUD: Audio_Receive_Custom :transfered %d bytes", err);*/
                            }
                        }
                    }
                    audio_unLock(phandle);
                }
            }
        }
    }
    else
    {
        IAP2USBPLUGINDLTLOG(DLT_LOG_DEBUG, "CINEMO_AUD:handle_newparams() returns err %d", err);
    }

    if (0 >= err)
    {
        IAP2USBPLUGINDLTLOG(DLT_LOG_DEBUG, "CINEMO_AUD: Audio_Receive_Custom EXIT with err: %d", err);
    }

    return err;
}

/**
 * This function is called if Cinemo needs to
 * unblock any audio-related function calls
 * @param[IN] tphandle       Transport handle.
 * @return CTLI_NO_ERROR     The return value of this function is ignored
 */
int Audio_Abort_Custom(ctli_handle tphandle)
{

    IAP2USBPLUGINDLTLOG(DLT_LOG_DEBUG, "CINEMO_AUD: Audio_Abort_Custom()");

    IPOD_IAP2_CINEMO_HOSTDEV_INFO *hdl = (IPOD_IAP2_CINEMO_HOSTDEV_INFO *)tphandle;
    audioHandle *phandle = (audioHandle *) hdl->audioctx;
    const uint64_t abort = 1;

    if (!phandle)
    {
        return CTLI_ERROR_ARGUMENTS;
    }
    phandle->aborting = true;
    int err = audioBackend_abort();
    if (err)
    {
       IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "CINEMO_AUD: audioBackend_abort FAILED");
    }
    else
    {
        /* Unblock the ctli_audio_receive_fn function */
        updateAudioState(phandle, ABORTED);
        err = write(phandle->abortFd, &abort, sizeof(uint64_t));
        if (err != sizeof(uint64_t))
        {
            IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "CINEMO_AUD: write ret =  %d | errno: %d %s", err, errno, strerror(errno));
        }
    }
    IAP2USBPLUGINDLTLOG(DLT_LOG_DEBUG, "CINEMO_AUD: Audio_Abort_Custom() returned");

    return CTLI_NO_ERROR;
}

/**
 * This function will be called if Cinemo needs
 * to receive the audio parameters from the transport layer.
 * @param[IN] tphandle       Transport handle.
 * @param[OUT] params        Pointer to audio parameters
 * @return CTLI_NO_ERROR     No error
 * Applicable for USB Host Mode
 */
int Audio_Get_Params_Custom( ctli_handle tphandle, ctli_audio_params* params )
{

    IPOD_IAP2_CINEMO_HOSTDEV_INFO *hdl = (IPOD_IAP2_CINEMO_HOSTDEV_INFO *)tphandle;
    audioHandle *phandle = (audioHandle *) hdl->audioctx;

    if (!phandle || !params)
    {
        return CTLI_ERROR_ARGUMENTS;
    }

    /* Provide the audio parameters to Cinemo */
    params->container = phandle->dev->audioParams.container;
    params->codec = phandle->dev->audioParams.codec;
    params->sampletype = phandle->dev->audioParams.sampletype;
    params->samplerate = phandle->dev->audioParams.samplerate = phandle->uac2capturesrate;
    params->channels = phandle->dev->audioParams.channels;
    params->bits = phandle->dev->audioParams.bits;

    IAP2USBPLUGINDLTLOG(DLT_LOG_DEBUG, "CINEMO_AUD: Audio_Get_Params_Custom() returned with rate:%d", params->samplerate);

    return CTLI_NO_ERROR;
}

/**
 * This function will be called if Cinemo
 * needs to set audio parameters to the transport layer.
 * @param[IN] tphandle       Transport handle.
 * @param[OUT] params        Pointer to audio parameters
 * @return CTLI_NO_ERROR     No error
 * Only Applicable for USB Device Mode
 */
int Audio_Set_Params_Custom( ctli_handle tphandle, const ctli_audio_params* params )
{

    IAP2USBPLUGINDLTLOG(DLT_LOG_DEBUG, "CINEMO_AUD: Audio_Set_Params_Custom()");
    IPOD_IAP2_CINEMO_HOSTDEV_INFO *hdl = (IPOD_IAP2_CINEMO_HOSTDEV_INFO *)tphandle;
    audioHandle *phandle = (audioHandle *) hdl->audioctx;

    if (!phandle || !params)
    {
        return CTLI_ERROR_ARGUMENTS;
    }

    phandle->dev->audioParams.container = params->container;
    phandle->dev->audioParams.codec = params->codec;
    phandle->dev->audioParams.sampletype = params->sampletype;
    phandle->dev->audioParams.samplerate = params->samplerate;
    phandle->dev->audioParams.channels = params->channels;
    phandle->dev->audioParams.bits = params->bits;

    IAP2USBPLUGINDLTLOG(DLT_LOG_DEBUG, "CINEMO_AUD: Audio_Set_Params_Custom() return");

    return CTLI_NO_ERROR;
}

/**
 * This function will be called If Cinemo needs to end audio streaming
 * @param[IN] tphandle       Transporthandle.
 * @return CTLI_NO_ERROR     The return value of this function is ignored in cinemo
 */
int Audio_Delete_Custom( ctli_handle tphandle )
{

    IAP2USBPLUGINDLTLOG(DLT_LOG_DEBUG, "CINEMO_AUD: Audio_Delete_Custom()");

    int err = CTLI_NO_ERROR;
    IPOD_IAP2_CINEMO_HOSTDEV_INFO *hdl = (IPOD_IAP2_CINEMO_HOSTDEV_INFO *)tphandle;
    audioHandle *phandle = (audioHandle *) hdl->audioctx;
    if (!phandle)
    {
        return CTLI_ERROR_ARGUMENTS;
    }

    /* Delete can be called without Abort */
    if (phandle->audioState != ABORTED)
    {
        err = audioBackend_abort();
        if (err)
        {
           IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "CINEMO_AUD: audioBackend_abort FAILED");
        }
    }

    err = audioBackend_close();
    if (err)
    {
       IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "CINEMO_AUD: audioBackend_abort FAILED");
    }

    err = close(phandle->abortFd);
    if (err < 0)
    {
        IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "close() abortFd failed with err:%d | %d %s", err, errno, strerror(errno));
    }
    err = close(phandle->eosFd);
    if (err < 0)
    {
        IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "close() eosFd failed with err:%d | %d %s", err, errno, strerror(errno));
    }
    err = close(phandle->uac2EvntFd);
    if (err < 0)
    {
        IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "close() uac2EvntFd failed with err:%d | %d %s", err, errno, strerror(errno));
    }
    err = close(phandle->processFd);
    if (err < 0)
    {
        IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "close() processFd failed with err:%d | %d %s", err, errno, strerror(errno));
    }

    /* Deleting circular buffer */
    if (phandle->buffer)
    {
        free(phandle->buffer);
        phandle->buffer = NULL;
    }

    bufferFree(phandle->cbuf);

    /* Deleting device parameter handle */
    if (phandle->dev)
    {
        free(phandle->dev);
        phandle->dev = NULL;
    }

    /* close capture file handle */
    CloseCaptureDataDump();

    /* Destroying the Mutex */
    err = pthread_mutex_destroy(&phandle->audioMutex);
    if (err < 0)
    {
        IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "pthread_mutex_destroy() failed with err:%d | %d %s", err, errno, strerror(errno));
    }

    phandle->uac2Monitor = false;
    err = UAC2DeviceMonitorJoinThread(phandle);
    if (err < 0)
    {
        IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "UAC2DeviceMonitorJoinThread() failed with err %d", err);
    }

    /* Deleting audio transport handle */
    if (phandle)
    {
        free(phandle);
        phandle = NULL;
    }
    audioBackend_delete();
    IAP2USBPLUGINDLTLOG(DLT_LOG_DEBUG, "CINEMO_AUD: Audio_Delete_Custom() return");

    return CTLI_NO_ERROR;
}
