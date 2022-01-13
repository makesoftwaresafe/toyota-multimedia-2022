/**
 * file: audiobe_smoketest.cpp
 *
 * Test Application for Audio Backends library
 *
 * author: Andreas Pape / ADIT / SW1 / apape@de.adit-jv.com
 *
 * copyright (c) 2014 Advanced Driver Information Technology.
 * This code is developed by Advanced Driver Information Technology.
 * Copyright of Advanced Driver Information Technology, Bosch, and DENSO.
 * All rights reserved.
 *
 ***********************************************************************/

#include <alsa/asoundlib.h>
#include <sys/signal.h>
#include <iostream>
#include <vector>
#include <string>
#include <memory>

#include "AudioStreaming.h"
#include "AudioFactory.h"
#include "AudioTypes.h"
#include "audiobe_smoketest.h"

#define BACKEND_LIB_NAME "Alsa"
#define FADE_IN_TIME                  8
#define FADE_OUT_TIME                33
#define MAX_FADETEST_COUNT            2
#define MAX_PLAYCAPTURE_LOOP_COUNT    5
#define NO_OF_TESTCASE                8
#define PLAYBACK_FILE_PATH           "/opt/platform/unit_tests/BigBuckBunny.wav"


using namespace std;
using namespace adit::utility;
using namespace adit::utility::audio;

static void ERR(const char *format, ...)
{
    va_list arg;
    va_start(arg, format);
    vfprintf(stderr, format, arg);
    putc('\n', stderr);
    va_end(arg);
}

enum AudioOperatingModes
{
    AUDIO_OPER_MODE_CAPTURE = 0,   /* Capture mode  */
    AUDIO_OPER_MODE_PLAYBACK,      /* Playback mode */
    AUDIO_OPER_MODE_DUPLEX,        /* Duplex modea  */
    AUDIO_OPER_MODE_MAX            /* Max */
};

const char* audioModestring[AUDIO_OPER_MODE_MAX] = { "Capture", "Playback", "Duplex"};

static unsigned int   do_transfer;
static unsigned int   extraframes;
static unsigned int   periodSize;
static unsigned int   passedTestCaseCount;
static unsigned char* audio_out_buffer;
static unsigned int   audio_operating_mode;
static unsigned int   sendAbort;
static unsigned int   sendStop;
static unsigned int   reachedEos;
static string         playbackDevice;
static string         captureDevice;


#define MAX_LINE_L  100
#define MAX_ENTRY_L MAX_LINE_L

/*we use fixed format*/
#define AUDIO_FORMAT_SIZE 2
#define AUDIO_CHANNELS    2

FILE *pcm_file = nullptr;

struct parse_csv
{
    char type[MAX_ENTRY_L];
    char str1[MAX_ENTRY_L];
    char str2[MAX_ENTRY_L];
    char str3[MAX_ENTRY_L];
    char str4[MAX_ENTRY_L];
    char str5[MAX_ENTRY_L];
};

struct cfgadev
{
    string           pcmname;        /* zero terminated string */
    snd_pcm_stream_t dir;       /* SND_PCM_STREAM_PLAYBACK */
    unsigned int     rate;          /* any, e.g. 8000-192000 */
    unsigned int     period_frames; /* e.g. 64/128/256/512 */
    AudioFormat      format;        /* format */
    signed short     startup_tout;  /* tout ms for initial read/write 0=default chosen by AUdio, -1=endless */
}; /* ALSA device */

ApplicationStreaming::ApplicationStreaming(void)
{

}

ApplicationStreaming::~ApplicationStreaming(void)
{

}

void ApplicationStreaming::error(const string& data) const
{
    cerr << data << endl;
}

void ApplicationStreaming::warning(const string& data) const
{
    cout << data << endl;
}

void ApplicationStreaming::info(const string& data) const
{
    cout << data << endl;
}

void ApplicationStreaming::debug(const string& data) const
{
    cout << data << endl;
}

eLogLevel ApplicationStreaming::checkLogLevel() const
{
    return LL_ERROR;
};

AudioState ApplicationStreaming::processing(unsigned char *in, unsigned char **out, uint32_t &frames)
{

    if (sendAbort)
    {
        sendAbort = 0;
        return AudioState::ABORT;
    }

    if (audio_operating_mode == AUDIO_OPER_MODE_CAPTURE) /* Capture mode */
    {
        if (in == nullptr)
        {
            return AudioState::ABORT;
        }
        /* Not doing any process on capture data */
    }
    else if (audio_operating_mode == AUDIO_OPER_MODE_PLAYBACK) /* Playback mode */
    {
        if (feof(pcm_file))
        {
            ERR("Processing: PCM reached EOS Sending Abort from processing Callback ");
            return AudioState::ABORT;
        }
        frames = periodSize + extraframes;
        fread(audio_out_buffer, (frames * AUDIO_FORMAT_SIZE * AUDIO_CHANNELS), 1, pcm_file);
        *out = audio_out_buffer;
    }
    else if (audio_operating_mode == AUDIO_OPER_MODE_DUPLEX)/* Duplex mode */
    {
        if (in == nullptr)
        {
            return AudioState::ABORT;
        }
        *out = in;
    }

    if (sendStop)
    {
        sendStop = 0;
        return AudioState::STOP;
    }

    return AudioState::CONTINUE;
}

void ApplicationStreaming::statistics(const StreamStatistics& status)
{
    (void)status;
}

void ApplicationStreaming::eostreaming(const AudioError error)
{
    reachedEos = 1;
    if (error != AudioError::OK)
        ERR("eostreaming: Reached EOS with erro %d ", error);
}

/* If "isDefFadeTime" is set, Fadetime will be configured with default Fadetime */
static AudioError audioModeConfigure(shared_ptr<Backend> stream, cfgadev *adevs,uint32_t isDefFadeTime)
{
    AudioError err = AudioError::OK;
    if (audio_operating_mode == AUDIO_OPER_MODE_CAPTURE)
    {
        playbackDevice.assign("");
        captureDevice = adevs[0].pcmname;
        if (!isDefFadeTime)
        {
            err = stream->setFadeTime(FadeMode::IN, StreamDirection::IN, FADE_IN_TIME);
            if (err != AudioError::OK)
            {
                ERR("Set fade In time failed with error %d", err);
            }
        }
    }
    else if(audio_operating_mode == AUDIO_OPER_MODE_PLAYBACK)
    {
        captureDevice.assign("");
        playbackDevice = adevs[1].pcmname;
        if (!isDefFadeTime)
        {
            err = stream->setFadeTime(FadeMode::OUT, StreamDirection::OUT, FADE_OUT_TIME);
            if (err != AudioError::OK)
            {
                ERR("Set fade Out time failed with error %d", err);
            }
        }
    }
    else if (audio_operating_mode == AUDIO_OPER_MODE_DUPLEX)
    {
        captureDevice  = adevs[0].pcmname;
        playbackDevice = adevs[1].pcmname;

        if (!isDefFadeTime)
        {
            err = stream->setFadeTime(FadeMode::IN, StreamDirection::IN, FADE_IN_TIME);
            if (err != AudioError::OK)
            {
                ERR("Set fade In time failed with error %d", err);
                return err;
            }

            err = stream->setFadeTime(FadeMode::OUT, StreamDirection::OUT, FADE_OUT_TIME);
            if (err != AudioError::OK)
            {
                ERR("Set fade Out time failed with error %d", err);
            }
        }
    }
    else
    {
        ERR("Invalid Mode");
        return AudioError::FAILURE;
    }
    return err;
}

static struct parse_csv* audio_parse_entry(unsigned int *num_streams, const char *path, const char *type, unsigned int fields)
{
    struct parse_csv *parsed     = nullptr;
    struct parse_csv *new_parsed = nullptr;
    unsigned int num = 0;
    ssize_t len = 0;
    int ret = 0;
    FILE *f = nullptr;
    char *line = nullptr;
    size_t linelen = 0;

    *num_streams = 0;
    f = fopen(path, "r");
    if (!f)
    {
        ERR( "audio_parse_entry: failed opening config file: %s", path);
        return nullptr;
    }

    while ((len = getline(&line, &linelen,f )) > 0)
    {
        if (len > MAX_LINE_L)
        {
            ERR( "audio_parse_entry: line too long %zd", len);
            break;
        }
        new_parsed = static_cast <struct parse_csv *>(realloc(parsed, (num+1) * sizeof(struct parse_csv)));
        if (!new_parsed)
        {
            ERR( "audio_parse_entry: failed allocating mem");
            num = 0;
            break;
        }
        parsed = new_parsed;
        ret = sscanf(line,"%[#]", parsed[num].type);
        if (ret)
        {
            continue;
        }
        ret = sscanf(line,"%[^;];%[^;];%[^;];%[^;];%[^;];%[^;];", parsed[num].type,
                parsed[num].str1, parsed[num].str2, parsed[num].str3, parsed[num].str4,parsed[num].str5);
        if (ret < (int)fields)
        {
            continue;
        }
        if (strcmp(parsed[num].type,type))
        {
            continue;
        }
        num++;
    }
    *num_streams = num;
    free(line);

    return parsed;
}

static int audio_parse_devices(struct cfgadev **adevs, unsigned  int *num_adevs, char* path)
{
    unsigned int i = 0;
    struct parse_csv *parsed = nullptr;
    *adevs = nullptr;
    *num_adevs = 0;

    parsed = audio_parse_entry(num_adevs, path, "ADEV", 6);
    if (parsed == nullptr)
    {
        return -1;
    }

    *adevs = (struct cfgadev*) new cfgadev[*num_adevs];

    for (i = 0; i < *num_adevs; i++)
    {
        if (strcmp(parsed[i].str2, "IN") && strcmp(parsed[i].str2, "OUT"))
        {
            ERR( "invalid direction %s", parsed[i].str2);
            *num_adevs = 0;
            break;
        }
        (*adevs)[i].pcmname.assign(strlen(parsed[i].str1) ? parsed[i].str1 : "");
        (*adevs)[i].dir           = !strcmp(parsed[i].str2, "IN")?SND_PCM_STREAM_CAPTURE:SND_PCM_STREAM_PLAYBACK;
        (*adevs)[i].rate          = atoi(parsed[i].str3);
        (*adevs)[i].period_frames = atoi(parsed[i].str4);
        (*adevs)[i].startup_tout  = atoi(parsed[i].str5);
        (*adevs)[i].format        = AudioFormat::S16_LE;
    }
    if (parsed)
    {
        delete (parsed);
    }
    return 0;
}

static void audio_sighandler(int sig)
{
    switch(sig)
    {
        case SIGABRT:
        case SIGINT:
        case SIGTERM:
            exit(0);
            break;

        case SIGUSR1:
            break;
    }
}

static void usage(void)
{
    fprintf(stdout, "\n");
    fprintf(stdout, "\n");
    fprintf(stdout, "Layout of configuration file:\n");
    fprintf(stdout, "ADEV;<pcm_name>;<direction>;<rate>;<period_size>;<init_timeout>\n");
    fprintf(stdout, "-no spaces in between\n");
    fprintf(stdout, "-direction must be IN or OUT\n");
    fprintf(stdout, "-no ticks ' \" \n");
    fprintf(stdout, "example:\n");
    fprintf(stdout, "\n");
    fprintf(stdout, "ADEV;AdevMicro1In;IN;16000;256;1000\n");
    fprintf(stdout, "ADEV;AdevTxOut;OUT;16000;256;0\n");
    fprintf(stdout, "\n");
    fprintf(stdout, "This defines following:\n");
    fprintf(stdout, "--Input device AdevMicro1In, rate 16000Hz, period size of 256frames, init timeout 1000ms\n");
    fprintf(stdout, "--Output device AdevTxOut, rate 16000Hz, period size of 256frames, default init timeout\n");
    fprintf(stdout, "\n");
    fprintf(stdout, "\n");
}

static void* allocateDataBuffer(unsigned int frames)
{
    void *buff = malloc (AUDIO_CHANNELS * AUDIO_FORMAT_SIZE * frames);
    return buff;
}

void defaults()
{
    if (pcm_file)
    {
        fseek ( pcm_file , 0 , SEEK_SET );
    }
    sendAbort   = 0;
    sendStop    = 0;
    do_transfer = 1;
    reachedEos  = 0;
    extraframes = 0;
}

AudioError TestCase_InavlidStateTransition(shared_ptr<Backend> stream, cfgadev *adevs)
{
    ERR("TestCase : Invalid State Transition ");
    AudioError err = AudioError::OK;

    if (stream == nullptr)
        return AudioError::FAILURE;

    defaults();

    err = audioModeConfigure(stream,adevs,0);
    if (err != AudioError::OK)
    {
        ERR("AUDIO MODE CONFIGURE FAILED, err = %d", err);
        goto error;
    }

    err = stream->openStream(captureDevice, playbackDevice, adevs[0].format, adevs[0].rate, AUDIO_CHANNELS, adevs[0].period_frames);
    if (err != AudioError::OK)
    {
        ERR("OPEN STREAM FAILDED, err = %d", err);
        goto error;
    }

    periodSize = adevs[0].period_frames;

    audio_out_buffer = (unsigned char *)allocateDataBuffer(periodSize);
    if (audio_out_buffer == nullptr)
    {
        ERR("OUTPUT Buffer allocation failed ");
        goto error;
    }

    err = stream->startStream();
    if (err != AudioError::OK)
    {
        ERR("START STREAM FAILED err = %d", err);
        goto error;
    }

    sleep(2);

    err = stream->closeStream();
    if (err != AudioError::INVALID)
    {
        goto error;
    }

    err = stream->openStream(adevs[0].pcmname, adevs[1].pcmname, adevs[0].format, adevs[0].rate, AUDIO_CHANNELS, adevs[0].period_frames);
    if (err != AudioError::INVALID)
    {
        goto error;
    }

    err = stream->stopStream();
    if (err != AudioError::OK)
    {
        ERR("\nSTOP STREAM FAILED err = %d", err);
        goto error;
    }

    err = stream->closeStream();
    if (err != AudioError::OK)
    {
        ERR("\nCLOSE STREAM FAILED err = %d", err);
        goto error;
    }

    passedTestCaseCount++;

    if (audio_out_buffer)
    {
        free(audio_out_buffer);
        audio_out_buffer = nullptr;
    }

    return err;

error:
    ERR("TestCase : Invalid State Transition Failed");
    stream->stopStream();
    stream->closeStream();

    if (audio_out_buffer)
    {
        free(audio_out_buffer);
        audio_out_buffer = nullptr;
    }
    return err;
}

AudioError TestCase_SameStateTransition(shared_ptr<Backend> stream, cfgadev *adevs)
{
    ERR("TestCase : Same State Transition");
    AudioError err = AudioError::OK;

    if (stream == nullptr)
        return AudioError::FAILURE;

    defaults();

    err = audioModeConfigure(stream,adevs,0);
    if (err != AudioError::OK)
    {
        ERR("AUDIO MODE CONFIGURE FAILED, err = %d", err);
        goto error;
    }

    err = stream->openStream(captureDevice, playbackDevice, adevs[0].format, adevs[0].rate, AUDIO_CHANNELS, adevs[0].period_frames);
    if (err != AudioError::OK)
    {
        ERR("OPEN STREAM FAILDED, err = %d", err);
        goto error;
    }

    periodSize = adevs[0].period_frames;

    audio_out_buffer = (unsigned char *)allocateDataBuffer(periodSize);
    if (audio_out_buffer == nullptr)
    {
        ERR("OUTPUT Buffer allocation failed ");
        goto error;
    }

    err = stream->startStream();
    if (err != AudioError::OK)
    {
        ERR("START STREAM FAILED err = %d", err);
        goto error;
    }

    sleep(2);

    err = stream->stopStream();
    if (err != AudioError::OK)
    {
        ERR("STOP STREAM FAILED err = %d", err);
        goto error;
    }

    err = stream->stopStream();
    if (err != AudioError::OK)
    {
        goto error;
    }

    err = stream->abortStream();
    if (err != AudioError::OK)
    {
        goto error;
    }

    err = stream->closeStream();
    if (err != AudioError::OK)
    {
        ERR("CLOSE STREAM FAILED err = %d", err);
        goto error;
    }

    passedTestCaseCount++;

    if (audio_out_buffer)
    {
        free(audio_out_buffer);
        audio_out_buffer = nullptr;
    }

    return err;

error:

    ERR("TestCase : Same State Transition Failed");
    stream->stopStream();
    stream->closeStream();

    if (audio_out_buffer)
    {
        free(audio_out_buffer);
        audio_out_buffer = nullptr;
    }

    return err;
}

AudioError TestCase_AbortStream(shared_ptr<Backend> stream, cfgadev *adevs)
{
    ERR("TestCase : Abort Stream");
    AudioError err = AudioError::OK;

    if (stream == nullptr)
        return AudioError::FAILURE;

    defaults();

    err = audioModeConfigure(stream,adevs,0);
    if (err != AudioError::OK)
    {
        ERR("AUDIO MODE CONFIGURE FAILED, err = %d", err);
        goto error;
    }

    err = stream->openStream(captureDevice, playbackDevice, adevs[0].format, adevs[0].rate, AUDIO_CHANNELS, adevs[0].period_frames);
    if (err != AudioError::OK)
    {
        ERR("OPEN STREAM FAILDED, err = %d", err);
        goto error;
    }

    periodSize = adevs[0].period_frames;

    audio_out_buffer = (unsigned char *)allocateDataBuffer(periodSize);
    if (audio_out_buffer == nullptr)
    {
        ERR("OUTPUT Buffer allocation failed ");
        goto error;
    }

    err = stream->startStream();
    if (err != AudioError::OK)
    {
        ERR("START STREAM FAILED err = %d", err);
        goto error;
    }

    sleep(2);

    err = stream->abortStream();
    if (err != AudioError::OK)
    {
        ERR("ABORT STREAM FAILDED, err = %d", err);
        goto error;
    }

    err = stream->closeStream();
    if (err != AudioError::OK)
    {
        ERR("CLOSE STREAM FAILED err = %d", err);
        goto error;
    }

    passedTestCaseCount++;

    if (audio_out_buffer)
    {
        free(audio_out_buffer);
        audio_out_buffer = nullptr;
    }

    return err;

error:

    ERR("TestCase : Abort Stream Failed");
    stream->stopStream();
    stream->closeStream();

    if (audio_out_buffer)
    {
        free(audio_out_buffer);
        audio_out_buffer = nullptr;
    }

    return err;
}

AudioError TestCase_Fading(shared_ptr<Backend> stream, cfgadev *adevs)
{
    ERR("TestCase : Fading ");
    AudioError err = AudioError::OK;
    int fadeTestCount = 0;

    if (stream == nullptr)
        return AudioError::FAILURE;

    defaults();

    err = audioModeConfigure(stream,adevs,0);
    if (err != AudioError::OK)
    {
        ERR("AUDIO MODE CONFIGURE FAILED, err = %d", err);
        goto error;
    }

    err = stream->openStream(captureDevice, playbackDevice, adevs[0].format, adevs[0].rate, AUDIO_CHANNELS, adevs[0].period_frames);
    if (err != AudioError::OK)
    {
        ERR("OPEN STREAM FAILDED, err = %d", err);
        goto error;
    }

    periodSize = adevs[0].period_frames;

    audio_out_buffer = (unsigned char *)allocateDataBuffer(periodSize);
    if (audio_out_buffer == nullptr)
    {
        ERR("OUTPUT Buffer allocation failed ");
        goto error;
    }

    while (do_transfer && (fadeTestCount < MAX_FADETEST_COUNT))
    {
        err = stream->startStream();
        if (err != AudioError::OK)
        {
            ERR("START STREAM FAILED err = %d", err);
            goto error;
        }

        sleep(2);

        err = stream->stopStream();
        if (err != AudioError::OK)
        {
            ERR("STOP STREAM FAILED err = %d", err);
            goto error;
        }
        sleep(1);
        fadeTestCount++;
    }

    err = stream->closeStream();
    if (err != AudioError::OK)
    {
        ERR("CLOSE STREAM FAILED err = %d", err);
        goto error;
    }

    passedTestCaseCount++;

    if (audio_out_buffer)
    {
        free(audio_out_buffer);
        audio_out_buffer = nullptr;
    }

    return err;

error:

    ERR("TestCase : Fading Failed");
    stream->stopStream();
    stream->closeStream();

    if (audio_out_buffer)
    {
        free(audio_out_buffer);
        audio_out_buffer = nullptr;
    }

    return err;
}

AudioError TestCase_AbortStreamByProcessingCb(shared_ptr<Backend> stream, cfgadev *adevs)
{
    ERR("TestCase : AbortStreamByProcessingCb ");
    AudioError err = AudioError::OK;

    if (stream == nullptr)
        return AudioError::FAILURE;

    defaults();

    err = audioModeConfigure(stream,adevs,0);
    if (err != AudioError::OK)
    {
        ERR("AUDIO MODE CONFIGURE FAILED, err = %d", err);
        goto error;
    }

    err = stream->openStream(captureDevice, playbackDevice, adevs[0].format, adevs[0].rate, AUDIO_CHANNELS, adevs[0].period_frames);
    if (err != AudioError::OK)
    {
        ERR("OPEN STREAM FAILDED, err = %d", err);
        goto error;
    }

    periodSize = adevs[0].period_frames;

    audio_out_buffer = (unsigned char *)allocateDataBuffer(periodSize);
    if (audio_out_buffer == nullptr)
    {
        ERR("OUTPUT Buffer allocation failed ");
        goto error;
    }

    err = stream->startStream();
    if (err != AudioError::OK)
    {
        ERR("START STREAM FAILDED, err = %d", err);
        goto error;
    }

    sleep(1);

    sendAbort = 1;

    sleep(1);

    if (!reachedEos)
    {
        ERR("ABORT stream by Processing callback failed");
        goto error;
    }

    err = stream->closeStream();
    if (err != AudioError::OK)
    {
        ERR("CLOSE STREAM FAILED err = %d", err);
        goto error;
    }

    passedTestCaseCount++;

    if (audio_out_buffer)
    {
        free(audio_out_buffer);
        audio_out_buffer = nullptr;
    }

    return err;

error:

    ERR("TestCase : AbortStreamByProcessingCb Failed");
    stream->stopStream();
    stream->closeStream();

    if (audio_out_buffer)
    {
        free(audio_out_buffer);
        audio_out_buffer = nullptr;
    }

    return err;
}

AudioError TestCase_StopStreamByProcessingCb(shared_ptr<Backend> stream, cfgadev *adevs)
{
    ERR("TestCase : StopStreamByProcessingCb ");
    AudioError err = AudioError::OK;

    if (stream == nullptr)
        return AudioError::FAILURE;

    defaults();

    err = audioModeConfigure(stream,adevs,0);
    if (err != AudioError::OK)
    {
        ERR("AUDIO MODE CONFIGURE FAILED, err = %d", err);
        goto error;
    }

    err = stream->openStream(captureDevice, playbackDevice, adevs[0].format, adevs[0].rate, AUDIO_CHANNELS, adevs[0].period_frames);
    if (err != AudioError::OK)
    {
        ERR("OPEN STREAM FAILDED, err = %d", err);
        goto error;
    }

    periodSize = adevs[0].period_frames;

    audio_out_buffer = (unsigned char *)allocateDataBuffer(periodSize);
    if (audio_out_buffer == nullptr)
    {
        ERR("OUTPUT Buffer allocation failed ");
        goto error;
    }

    err = stream->startStream();
    if (err != AudioError::OK)
    {
        ERR("START STREAM FAILDED, err = %d", err);
        goto error;
    }

    sleep(1);

    sendStop = 1;

    sleep(1);

    if (!reachedEos)
    {
        ERR("STOP stream by Processing callback failed", err);
        goto error;
    }

    err = stream->closeStream();
    if (err != AudioError::OK)
    {
        ERR("CLOSE STREAM FAILED err = %d", err);
        goto error;
    }

    passedTestCaseCount++;

    if (audio_out_buffer)
    {
        free(audio_out_buffer);
        audio_out_buffer = nullptr;
    }

    return err;

error:

    ERR("TestCase : StopStreamByProcessingCb Failed");
    stream->stopStream();
    stream->closeStream();

    if (audio_out_buffer)
    {
        free(audio_out_buffer);
        audio_out_buffer = nullptr;
    }

    return err;
}

AudioError TestCase_AbortDuringStopping(shared_ptr<Backend> stream, cfgadev *adevs)
{
    ERR("TestCase : AbortDuringStopping ");
    AudioError err = AudioError::OK;
    int playCaptureCount = 0;

    if (stream == nullptr)
        return AudioError::FAILURE;

    defaults();

    err = audioModeConfigure(stream,adevs,0);
    if (err != AudioError::OK)
    {
        ERR("AUDIO MODE CONFIGURE FAILED, err = %d", err);
        goto error;
    }

    err = stream->openStream(captureDevice, playbackDevice, adevs[0].format, adevs[0].rate, AUDIO_CHANNELS, adevs[0].period_frames);
    if (err != AudioError::OK)
    {
        ERR("OPEN STREAM FAILDED, err = %d", err);
        goto error;
    }

    periodSize = adevs[0].period_frames;

    audio_out_buffer = (unsigned char *)allocateDataBuffer(periodSize);
    if (audio_out_buffer == nullptr)
    {
        ERR("OUTPUT Buffer allocation failed ");
        goto error;
    }

    err = stream->startStream();
    if (err != AudioError::OK)
    {
        ERR("START STREAM FAILDED, err = %d", err);
        goto error;
    }
    while (do_transfer && (playCaptureCount < MAX_PLAYCAPTURE_LOOP_COUNT ))
    {
        playCaptureCount++;
        usleep(100 * 1000);
    }

    /* Send signal to stop from processing callback */
    sendStop = 1;

    usleep(35 * 1000);

    err = stream->abortStream();

    if (err != AudioError::OK)
    {
        ERR("STOP STREAM FAILED err = %d", err);
        goto error;
    }

    err = stream->closeStream();
    if (err != AudioError::OK)
    {
        ERR("CLOSE STREAM FAILED err = %d", err);
        goto error;
    }

    passedTestCaseCount++;

    if (audio_out_buffer)
    {
        free(audio_out_buffer);
        audio_out_buffer = nullptr;
    }

    return err;

error:

    ERR("TestCase : AbortDuringStopping Failed");
    stream->stopStream();
    stream->closeStream();

    if (audio_out_buffer)
    {
        free(audio_out_buffer);
        audio_out_buffer = nullptr;
    }

    return err;
}

AudioError TestCase_PlayCapture(shared_ptr<Backend> stream, cfgadev *adevs)
{
    ERR("TestCase : PlayCapture ");
    AudioError err = AudioError::OK;
    int playCaptureCount = 0;

    if (stream == nullptr)
        return AudioError::FAILURE;

    defaults();

    err = audioModeConfigure(stream,adevs,0);
    if (err != AudioError::OK)
    {
        ERR("AUDIO MODE CONFIGURE FAILED, err = %d", err);
        goto error;
    }

    err = stream->openStream(captureDevice, playbackDevice, adevs[0].format, adevs[0].rate, AUDIO_CHANNELS, adevs[0].period_frames);
    if (err != AudioError::OK)
    {
        ERR("OPEN STREAM FAILDED, err = %d", err);
        goto error;
    }

    periodSize = adevs[0].period_frames;

    audio_out_buffer = (unsigned char *)allocateDataBuffer(periodSize);
    if (audio_out_buffer == nullptr)
    {
        ERR("OUTPUT Buffer allocation failed ");
        goto error;
    }

    err = stream->startStream();
    if (err != AudioError::OK)
    {
        ERR("START STREAM FAILDED, err = %d", err);
        goto error;
    }
    while (do_transfer && (playCaptureCount < MAX_PLAYCAPTURE_LOOP_COUNT ))
    {
        playCaptureCount++;
        sleep(1);
    }

    err = stream->stopStream();

    if (err != AudioError::OK)
    {
        ERR("STOP STREAM FAILED err = %d", err);
        goto error;
    }

    err = stream->closeStream();
    if (err != AudioError::OK)
    {
        ERR("CLOSE STREAM FAILED err = %d", err);
        goto error;
    }

    passedTestCaseCount++;

    if (audio_out_buffer)
    {
        free(audio_out_buffer);
        audio_out_buffer = nullptr;
    }

    return err;

error:

    ERR("TestCase : PlayCapture Failed");
    stream->stopStream();
    stream->closeStream();

    if (audio_out_buffer)
    {
        free(audio_out_buffer);
        audio_out_buffer = nullptr;
    }

    return err;
}

AudioError TestCase_PlayUnallignedFrames(shared_ptr<Backend> stream, cfgadev *adevs)
{
    ERR("TestCase : PlayUnallignedFrames ");
    AudioError err = AudioError::OK;
    int playCaptureCount = 0;

    if (stream == nullptr)
        return AudioError::FAILURE;

    defaults();

    err = audioModeConfigure(stream,adevs,0);
    if (err != AudioError::OK)
    {
        ERR("OPEN STREAM FAILDED, err = %d", err);
        goto error;
    }

    err = stream->openStream(captureDevice, playbackDevice, adevs[0].format, adevs[0].rate, AUDIO_CHANNELS, adevs[0].period_frames);
    if (err != AudioError::OK)
    {
        ERR("OPEN STREAM FAILDED, err = %d", err);
        goto error;
    }

    periodSize = adevs[0].period_frames;
    extraframes = (periodSize * 5) + (periodSize / 2);

    audio_out_buffer = (unsigned char *)allocateDataBuffer(periodSize + extraframes);
    if (audio_out_buffer == nullptr)
    {
        ERR("OUTPUT Buffer allocation failed ");
        goto error;
    }

    err = stream->startStream();
    if (err != AudioError::OK)
    {
        ERR("START STREAM FAILDED, err = %d", err);
        goto error;
    }
    while (do_transfer && (playCaptureCount < MAX_PLAYCAPTURE_LOOP_COUNT ))
    {
        playCaptureCount++;
        sleep(1);
    }

    err = stream->stopStream();

    if (err != AudioError::OK)
    {
        ERR("STOP STREAM FAILED err = %d", err);
        goto error;
    }

    err = stream->closeStream();
    if (err != AudioError::OK)
    {
        ERR("CLOSE STREAM FAILED err = %d", err);
        goto error;
    }

    passedTestCaseCount++;

    if (audio_out_buffer)
    {
        free(audio_out_buffer);
        audio_out_buffer = nullptr;
    }

    return err;

error:

    ERR("TestCase : PlayUnallignedFrames Failed");
    stream->stopStream();
    stream->closeStream();

    if (audio_out_buffer)
    {
        free(audio_out_buffer);
        audio_out_buffer = nullptr;
    }

    return err;
}

AudioError TestCase_PlayCaptureDefFadeTime(shared_ptr<Backend> stream, cfgadev *adevs)
{
    ERR("TestCase : PlayCaptureDefFadeTime ");
    AudioError err = AudioError::OK;
    int playCaptureCount = 0;

    if (stream == nullptr)
        return AudioError::FAILURE;

    defaults();

    err = audioModeConfigure(stream,adevs,1);
    if (err != AudioError::OK)
    {
        ERR("AUDIO MODE CONFIGURE FAILED, err = %d", err);
        goto error;
    }

    err = stream->openStream(captureDevice, playbackDevice, adevs[0].format, adevs[0].rate, AUDIO_CHANNELS, adevs[0].period_frames);
    if (err != AudioError::OK)
    {
        ERR("OPEN STREAM FAILDED, err = %d", err);
        goto error;
    }

    periodSize = adevs[0].period_frames;

    audio_out_buffer = (unsigned char *)allocateDataBuffer(periodSize);
    if (audio_out_buffer == nullptr)
    {
        ERR("OUTPUT Buffer allocation failed ");
        goto error;
    }

    err = stream->startStream();
    if (err != AudioError::OK)
    {
        ERR("START STREAM FAILDED, err = %d", err);
        goto error;
    }
    while (do_transfer && (playCaptureCount < MAX_PLAYCAPTURE_LOOP_COUNT ))
    {
        playCaptureCount++;
        sleep(1);
    }

    err = stream->stopStream();

    if (err != AudioError::OK)
    {
        ERR("STOP STREAM FAILED err = %d", err);
        goto error;
    }

    err = stream->closeStream();
    if (err != AudioError::OK)
    {
        ERR("CLOSE STREAM FAILED err = %d", err);
        goto error;
    }

    passedTestCaseCount++;

    if (audio_out_buffer)
    {
        free(audio_out_buffer);
        audio_out_buffer = nullptr;
    }

    return err;

error:

    ERR("TestCase : PlayCaptureDefFadeTime Failed");
    stream->stopStream();
    stream->closeStream();

    if (audio_out_buffer)
    {
        free(audio_out_buffer);
        audio_out_buffer = nullptr;
    }

    return err;
}

AudioError TestCase_DeleteStream(cfgadev *adevs)
{
    ERR("TestCase : DeleteStream ");
    ApplicationStreaming streamingHandle;
    string backendLibName;
    shared_ptr<Backend> stream = nullptr;
    AudioError err = AudioError::OK;

    stream = Factory::Instance()->createBackend(backendLibName, streamingHandle);
    if(stream == nullptr)
    {
        ERR("First Backend Object creation Failed");
        return AudioError::FAILURE;
    }

    defaults();

    err = audioModeConfigure(stream,adevs,0);
    if (err != AudioError::OK)
    {
        ERR("AUDIO MODE CONFIGURE FAILED, err = %d", err);
        goto error;
    }

    err = stream->openStream(captureDevice, playbackDevice, adevs[0].format, adevs[0].rate, AUDIO_CHANNELS, adevs[0].period_frames);
    if (err != AudioError::OK)
    {
        ERR("OPEN STREAM FAILDED, err = %d", err);
        goto error;
    }

    periodSize = adevs[0].period_frames;

    audio_out_buffer = (unsigned char *)allocateDataBuffer(periodSize);
    if (audio_out_buffer == nullptr)
    {
        ERR("OUTPUT Buffer allocation failed ");
        goto error;
    }

    err = stream->startStream();
    if (err != AudioError::OK)
    {
        ERR("START STREAM FAILDED, err = %d", err);
        goto error;
    }

    sleep(1);

    return err;

error:

    ERR("TestCase : DeleteStream Failed");
    stream->stopStream();
    stream->closeStream();

    if (audio_out_buffer)
    {
        free(audio_out_buffer);
        audio_out_buffer = nullptr;
    }

    return err;
}

void TestCase_CreateAndDeleteMultipleBackends()
{
    ApplicationStreaming streamingHandle;
    string backendLibName;
    shared_ptr<Backend> stream1 = nullptr;
    shared_ptr<Backend> stream2 = nullptr;
    shared_ptr<Backend> stream3 = nullptr;
    shared_ptr<Backend> stream4 = nullptr;

    backendLibName.assign(BACKEND_LIB_NAME);

    ERR("TestCase : CreateAndDeleteMultipleBackends ");

    stream1 = Factory::Instance()->createBackend(backendLibName, streamingHandle);
    if(stream1 == nullptr)
    {
        ERR("First Backend Object creation Failed");
    }

    stream2 = Factory::Instance()->createBackend(backendLibName, streamingHandle);
    if(stream2 == nullptr)
    {
        ERR("Second Backend Object creation Failed");
    }

    stream3 = Factory::Instance()->createBackend(backendLibName, streamingHandle);
    if(stream3 == nullptr)
    {
        ERR("Third Backend Object creation Failed");
    }

    stream4 = Factory::Instance()->createBackend(backendLibName, streamingHandle);
    if(stream4 == nullptr)
    {
        ERR("Fourth Backend Object creation Failed");
    }
}


int main(int argc, char** argv)
{
    unsigned int ret = 0;
    shared_ptr<Backend> stream = nullptr;
    static struct cfgadev *adevs = nullptr;
    unsigned int num_adevs = 0;
    unsigned int testcases = 0;
    char *path_dev = nullptr;
    string backendLibName;
    ApplicationStreaming streamingHandle;

    struct sigaction new_action;
    sigemptyset (&new_action.sa_mask);
    new_action.sa_flags   = 0;
    new_action.sa_handler = audio_sighandler;

    /*minimum arg: path*/
    if (argc < 2)
    {
        fprintf(stdout,"ERROR: No Configuration file path given");
        usage();
        ret = 1;
        goto exit;
    }

    path_dev = argv[1];
    if (audio_parse_devices(&adevs, &num_adevs, path_dev) < 0)
    {
        ERR("failed parsing devices");
        ret = 1;
        goto exit;
    }

    backendLibName.assign(BACKEND_LIB_NAME);
    stream = Factory::Instance()->createBackend(backendLibName, streamingHandle);
    if(stream == nullptr)
    {
        ERR("Backend Object creation Failed");
        ret = 1;
        goto exit;
    }

    ERR("Selected Backend Name : %s ",stream->getBackendName());

    do_transfer = 1;

    sigaction(SIGUSR1, &new_action, nullptr);
    sigaction(SIGINT,  &new_action, nullptr);
    sigaction(SIGTERM, &new_action, nullptr);

    /*FixMe: Updating card name based on mode ,
     * Also need to fix in argument file parsing
     */
    for (int i = AUDIO_OPER_MODE_CAPTURE; i < AUDIO_OPER_MODE_MAX; i++)
    {
        audio_operating_mode = i;
        AudioError err = AudioError::OK;


        if (audio_operating_mode == AUDIO_OPER_MODE_PLAYBACK)
        {
            pcm_file = fopen(PLAYBACK_FILE_PATH,"rb");
            if (pcm_file == nullptr)
            {
                fprintf(stdout, "ERROR: File open failed ");
                ret = 1;
                goto exit;
            }
        }
        ERR("--------------------------------------------------------");
        ERR("Running smokeTest for %s mode ", audioModestring[i]);
        ERR("--------------------------------------------------------");
        err = TestCase_InavlidStateTransition(stream, adevs);
        if (err != AudioError::OK)
        {
            ret = 1;
            goto exit;
        }

        err = TestCase_SameStateTransition(stream, adevs);
        if (err != AudioError::OK)
        {
            ret = 1;
            goto exit;
        }

        err = TestCase_AbortStream(stream, adevs);
        if (err != AudioError::OK)
        {
            ret = 1;
            goto exit;
        }

        err = TestCase_Fading(stream, adevs);
        if (err != AudioError::OK)
        {
            ret = 1;
            goto exit;
        }

        err = TestCase_AbortStreamByProcessingCb(stream, adevs);
        if (err != AudioError::OK)
        {
            ret = 1;
            goto exit;
        }

        err = TestCase_StopStreamByProcessingCb(stream, adevs);
        if (err != AudioError::OK)
        {
            ret = 1;
            goto exit;
        }

        err = TestCase_PlayCapture(stream, adevs);
        if (err != AudioError::OK)
        {
            ret = 1;
            goto exit;
        }

        err = TestCase_PlayCaptureDefFadeTime(stream, adevs);
        if (err != AudioError::OK)
        {
            ret = 1;
            goto exit;
        }

        if (i == AUDIO_OPER_MODE_PLAYBACK)
        {
            TestCase_PlayUnallignedFrames(stream, adevs);
            if (err != AudioError::OK)
            {
                ret = 1;
                goto exit;
            }
            TestCase_AbortDuringStopping(stream, adevs);
            if (err != AudioError::OK)
            {
                ret = 1;
                goto exit;
            }
            testcases = NO_OF_TESTCASE + 2;
        }
        else
        {
            testcases = NO_OF_TESTCASE;
        }


        ERR("--------------------------------------------------------");
        ERR("SmokeTest passed %d out of %d for %s mode ", passedTestCaseCount, testcases, audioModestring[i]);
        ERR("--------------------------------------------------------");
        ERR("\n");

        passedTestCaseCount = 0;
    }

    audio_operating_mode = AUDIO_OPER_MODE_PLAYBACK;
    TestCase_DeleteStream(adevs);
    TestCase_CreateAndDeleteMultipleBackends();

exit:

    if(pcm_file)
    {
        fclose(pcm_file);
    }

    sigdelset (&new_action.sa_mask, SIGUSR1);
    sigdelset (&new_action.sa_mask, SIGINT);
    sigdelset (&new_action.sa_mask, SIGTERM);

    delete Factory::Instance();

    ERR("Audio Interface SmokeTest Ends !!!");

    /* Exit status 0 is success. Other than 0 are considered as faileur */
    return ret;
}
