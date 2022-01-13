/**
 * \file: alsa_com_server_i.h
 *
 *  IPC server for communicate with ALSA and applicaiton process
 *
 * \component:  IPC server
 *
 * \author: ADIT
 *
 * \copyright: (c) 2003 - 2011 ADIT Corporation
 *
 */


#ifndef ALSA_COM_SERVER_I_H_
#define ALSA_COM_SERVER_I_H_

#define IPOD_ALSA_BIT_PER_BYTE              8
#define IPOD_ALSA_OUTPUT_RATE               (44100)
#define IPOD_ALSA_CHANNEL                   (2)
#define IPOD_ALSA_DATA_SIZE                 (2)                 /* SND_PCM_FORMAT_S16_LE */
#define IPOD_ALSA_BUF_NUM                   (16)
#define IPOD_ALSA_INPUTDEV_BUF              (32)
#define IPOD_ALSA_INPUT_DEVICE_NAME         "iPodin"
#define IPOD_ALSA_WAIT_MS                   (2000)
#define IPOD_ALSA_HTCL_NAME                 "hw"
#define IPOD_ALSA_HTCL_LEN             (16)
#define IPOD_ALSA_MIXER_SIMPLE_IDENTIFIER   "iPod"
#define IPOD_ALSA_XRUN_ERROR                (-2)
#define IPOD_ALSA_VOLUME_RANGE              (100)
#define IPOD_ALSA_ACCURACY                  (0.01)
#define IPOD_ALSA_MAX_BUFFER_TIME           (100000)
#define IPOD_ALSA_MAX_PERIOD_TIME           (20000)
#define IPOD_ALSA_WAIT_TIME_OUT             (1000)
#define IPOD_ALSA_LOOP_TIMEOUT              (50000000)          /* 50000000ns, 50000us, 50ms */
#define IPOD_ALSA_LOOP_COUNT                (100)               /* 50 * 100 = 5second */
#define IPOD_ALSA_VOLUME_MAX                (99)
#define IPOD_ALSA_MAIN_LOOP_SLEEP_TIME      (100000)          /* nanosecond    max:10000000*/
#define IPOD_ALSA_HW_CARD_NUM_POS           (3)
#define IPOD_ALSA_HW_DEVICE_NUM_POS         (5)
#define IPOD_ALSA_HW_DEVICE_RESOLUTION      (100)

#define IPOD_ALSA_SELEM_PCM                "pcm"
#define IPOD_ALSA_SELEM_SLAVE              "slave"
#define IPOD_ALSA_SELEM_TYPE               "type"
#define IPOD_ALSA_SELEM_CARD               "card"
#define IPOD_ALSA_SELEM_DEVICE             "device"
#define IPOD_ALSA_SELEM_CTRL               "control"
#define IPOD_ALSA_SELEM_NAME               "name"
#define IPOD_ALSA_SELEM_RESOLUT            "resolution"
#define IPOD_ALSA_SELEM_HW                 "hw"
#define IPOD_ALSA_SELEM_PLUG               "plug"
#define IPOD_ALSA_SELEM_SVOL               "softvol"

#define IPOD_ALSA_MAX_DEVICE_NAME_LEN 128

/* status */
typedef enum
{
    ALSA_IDLE = 0,
    ALSA_STOP,
    ALSA_RUNNING,
    ALSA_READ_OVERRUN,
    ALSA_WRITE_OVERRUN
} ALSA_STATUS;

struct IPOD_AUDIO_COM_INFO_
{
    int playThrState;
    pthread_t threadId;
    unsigned char src_name[IPOD_ALSA_MAX_DEVICE_NAME_LEN];
    unsigned char sink_name[IPOD_ALSA_MAX_DEVICE_NAME_LEN];
    ALSA_STATUS status;                 /* IDLE, STOP, RUNNING, READ_OVERRUN, WRITE_OVERRUN */
    void *buf[IPOD_ALSA_BUF_NUM];       /* Buffer */
    snd_pcm_t *chandle;                 /* ALSA capture handle */
    snd_pcm_t *phandle;                 /* ALSA playback handle */
    snd_async_handler_t *ccallback;     /* ALSA capture callback handler */
    snd_async_handler_t *pcallback;     /* ALSA playback callback handler */
    unsigned int crate;                 /* current src sample rate */
    unsigned int prate;                 /* current sink sample rate */
    int rIndex;                         /* Read position of buffer */
    int wIndex;                         /* Write position of buffer */
    snd_mixer_t *vol_handle;            /* Volume handler */
    snd_mixer_elem_t *vol_element;      /* Volume element */
    U8 volume;                          /* Current volume */
    snd_pcm_sframes_t period_size;      /* Period Size */
    int chunk_bytes;                    /* Chunk Bytes */
};

#endif /* PAI_COM_SERVER_I_H_ */

