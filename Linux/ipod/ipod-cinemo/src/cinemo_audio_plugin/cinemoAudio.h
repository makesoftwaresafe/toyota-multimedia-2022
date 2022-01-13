/**
 * \file: cinemoAudio.h
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
#ifndef CINEMOAUDIO_H_
#define CINEMOAUDIO_H_

#include "AudioCommon.h"
#include "cinemo_transport.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum audiostates
{
    CLOSED = 0,
    OPENED,             /* audio device is opened */
    RUNNING,            /* audio device is started streaming */
    STOPPED,            /* audio device has NO DATA i.e pause, stop, BT connection etc */
    NEWAUDIOPARAMS,     /* audio device has changed the audio params */
    ABORTED,            /* audio streaming has aborted */
    EOS,                /* audio streaming xruns, disconnect etc */
} states;

typedef enum uac2states
{
    CEP_DISABLED = 0,           /* uac2 state when capture end point disabled */
    CEP_ENABLED,                /* uac2 state when capture end point enabled */
    RATE_CHANGE_DISABLED,       /* uac2 state during rate change with end point disabled */
    RATE_CHANGE_ENABLED         /* uac2 state during rate change with end point enabled */
} ustates;

typedef struct deviceParam
{
    uint32_t periodFrames;
    uint32_t periodsizeBytes;
    ctli_audio_params audioParams;
    char *captureDevice;
    char *playbackDevice;
} deviceParams;

typedef struct handles
{
    bool aborting;
    bool uac2Monitor;
    int CaptureEndPoint;
    int abortFd;
    int eosFd;
    int uac2EvntFd;
    int processFd;
    uint32_t uac2capturesrate;
    states audioState;
    ustates uac2states;
    deviceParams *dev;
    char *buffer;
    cBufHandle cbuf;
    pthread_t uac2Id;
    pthread_mutex_t audioMutex;
    uint32_t perioSizeBytes;
} audioHandle;

int audioBackend_create(ctli_handle tphandle);
int audioBackend_open(deviceParams *dev);
int audioBackend_stream();
int audioBackend_abort();
void audioBackend_delete();
int audioBackend_close();
int UAC2DeviceMonitorCreateThread(audioHandle *tphandle);
int UAC2DeviceMonitorJoinThread(audioHandle *tphandle);
int Audio_StopClose_Custom(audioHandle *phandle);
void updateAudioState(audioHandle *phandle, states audioState);

/* For Audio processing protection */
bool audio_lock(audioHandle *phandle);
void audio_unLock(audioHandle *phandle);

#ifdef __cplusplus
}
#endif
#endif /* CINEMOAUDIO_H_ */
