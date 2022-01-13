/**
 * \file: AudioCommon.h
 *
 * \version: $Id:$
 *
 * \release: $Name:$
 *
 * cinemo audio streaming buffer management
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
#ifndef AUDIOCOMMON_H_
#define AUDIOCOMMON_H_

#include <assert.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/time.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <sys/select.h>
#include <sys/eventfd.h>

#include "cinemo_transport.h"
#include "iap2_dlt_log.h"

#ifdef __cplusplus
extern "C" {
#endif

#define STEREO                           2
#define DEFAULT_SAMPLETYPEBITS           16
#define DEFAULT_BUFFER_SIZE_SEC          2
#define DEFAULT_RATE                     44100
#define DEFAULT_CHANNEL                  STEREO        /* uac2 default supports stereo channel */
#define DEFAULT_PERIOD_FRAMES            1024          /* near period size from ALSA on uac2 */
#define MILLI_SECONDS                    1000          /* Seconds representing in Milli seconds */
#define DEFAULT_PERIODTIME               ((DEFAULT_PERIOD_FRAMES * MILLI_SECONDS) / DEFAULT_RATE)
#define DEFAULT_PERIOD_FRAMES_BYTES      (DEFAULT_PERIOD_FRAMES * (DEFAULT_SAMPLETYPEBITS / 8) * DEFAULT_CHANNEL)
/* chosen minimum circular buffer size to have 2sec data*/
#define DEFAULT_BUFFER_SIZE              ((((DEFAULT_PERIOD_FRAMES_BYTES * DEFAULT_BUFFER_SIZE_SEC) / \
                                         DEFAULT_PERIODTIME) * MILLI_SECONDS) + \
                                         (DEFAULT_PERIOD_FRAMES_BYTES - \
                                         ((((DEFAULT_PERIOD_FRAMES_BYTES * DEFAULT_BUFFER_SIZE_SEC) / \
                                         DEFAULT_PERIODTIME) * MILLI_SECONDS) % DEFAULT_PERIOD_FRAMES_BYTES)))

#define BACKEND_LIB_NAME  "Alsa"

typedef struct cBuf
{
    unsigned char *buffer;
    uint32_t writePos;
    uint32_t readPos;
    uint32_t bufMaxLen;
    bool bufFull;
    unsigned char *wrapBuffer;
} cBuf;

typedef cBuf* cBufHandle;

void bufferPushData(cBufHandle cbuf, unsigned char *data, uint32_t size);
uint32_t bufferPopData(cBufHandle cbuf, unsigned char * data, uint32_t size);
cBufHandle bufferInit(char *buffer, uint32_t size);
bool isBufferFull(cBufHandle cbuf);
void audioBufferReset(cBufHandle cbuf);
bool bufferEmpty(cBufHandle cbuf);
void bufferFree(cBufHandle cbuf);
void printBufferStatus(cBufHandle cbuf);
void printDataStatus(cBufHandle cbuf);
uint32_t getBitsPerSample(CTLI_AUDIO_SAMPLETYPE sampletype);

void bufferPushDatanew(cBufHandle cbuf, unsigned char *data, uint32_t size);
uint32_t bufferPopDatanew(cBufHandle cbuf, unsigned char *data, uint32_t size);
uint32_t getBufferAvail(cBufHandle cbuf);
uint32_t getDataAvail(cBufHandle cbuf);

/* For Capturing Audio Dump */
void OpenCaptureDataDump(char* dumpPath);
void CloseCaptureDataDump();

#ifdef __cplusplus
}
#endif
#endif /* AUDIOCOMMON_H_ */
