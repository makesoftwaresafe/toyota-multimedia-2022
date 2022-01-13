/**
 * \file: AudioCommon.c
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
#include "AudioCommon.h"

bool isBufferFull(cBufHandle cbuf)
{
    bool ret = false;
    if (cbuf)
    {
        ret = cbuf->bufFull;
    }

    return ret;
}

void audioBufferReset(cBufHandle cbuf)
{
    if (cbuf)
    {
        cbuf->writePos = 0;
        cbuf->readPos = 0;
        cbuf->bufFull = false;
        memset(cbuf->buffer, 0, cbuf->bufMaxLen);
    }
}

bool bufferEmpty(cBufHandle cbuf)
{
    bool ret = false;
    if (!isBufferFull(cbuf) && (cbuf->writePos == cbuf->readPos))
    {
        ret = true;
    }
    return ret;
}

cBufHandle bufferInit(char *buffer, uint32_t size)
{
    if (!buffer && (size == 0))
    {
        return NULL;
    }
    cBufHandle cbuf = calloc(1, sizeof(cBuf));
    if (cbuf != NULL)
    {
        cbuf->buffer = buffer;
        cbuf->bufMaxLen = size;
        audioBufferReset(cbuf);
    }
    return cbuf;
}

uint32_t getDataAvail(cBufHandle cbuf)
{
    uint32_t size = 0;

    if (cbuf)
    {
        if (cbuf->writePos > cbuf->readPos)
        {
            size = cbuf->writePos - cbuf->readPos;
        }
        else if (cbuf->writePos < cbuf->readPos)
        {
            size = cbuf->bufMaxLen + cbuf->writePos - cbuf->readPos;
        }
        else
        {
            size = cbuf->bufMaxLen;
        }
    }

    return size;
}

uint32_t getBufferAvail(cBufHandle cbuf)
{
    uint32_t size = 0;
    if (cbuf)
    {
        if (bufferEmpty(cbuf))
        {
            size = cbuf->bufMaxLen;
        }
        else
        {
            size = (cbuf->readPos >= cbuf->writePos) ?
                    (cbuf->readPos - cbuf->writePos) :
                    cbuf->bufMaxLen + cbuf->readPos - cbuf->writePos;
        }
    }

    return size;
}

static void enableOverWrite(cBufHandle cbuf, uint32_t size)
{
    if (isBufferFull(cbuf))
    {
        IAP2USBPLUGINDLTLOG(DLT_LOG_DEBUG, "CINEMO_AUD: BUFFER IS FULL NOW: OVERWRITING");
        cbuf->readPos = (cbuf->readPos + size) % cbuf->bufMaxLen;
        cbuf->bufFull = false;
    }
}

static uint32_t isWrapAround(cBufHandle cbuf, uint32_t size, uint32_t pos)
{
    uint32_t initSize = (cbuf->bufMaxLen - pos);
    if (initSize < size)
    {
        return initSize;
    }
    return 0;
}

static void readPosUpdate(cBufHandle cbuf, uint32_t size)
{
    if (cbuf)
    {
        cbuf->bufFull = false;
        cbuf->readPos = (cbuf->readPos + size) % cbuf->bufMaxLen;
    }
}

static void writePosUpdate(cBufHandle cbuf, uint32_t size)
{
    if (cbuf)
    {
        if (cbuf->bufFull)
        {
            enableOverWrite(cbuf, size);
        }
        cbuf->writePos = (cbuf->writePos + size) % cbuf->bufMaxLen;
        cbuf->bufFull = (cbuf->writePos == cbuf->readPos) ? true : false;
    }
}

void copyToAndUpdatePos(cBufHandle cbuf, unsigned char *data, uint32_t size)
{
    if (cbuf && data && size)
    {
        memcpy(&cbuf->buffer[cbuf->writePos], data, size);
        writePosUpdate(cbuf, size);
    }
}

void copyFromAndUpdatePos(cBufHandle cbuf, unsigned char *data, uint32_t size)
{
    if (cbuf && data && size)
    {
        memcpy(data, &cbuf->buffer[cbuf->readPos], size);
        readPosUpdate(cbuf, size);
    }
}

void bufferPushData(cBufHandle cbuf, unsigned char *data, uint32_t dataSize)
{
    if (cbuf && cbuf->buffer)
    {
        uint32_t bufferAvail;
        bufferAvail = getBufferAvail(cbuf);

        /* Buffer availability and overlap */
        if (bufferAvail >= dataSize)
        {
            uint32_t initSize = isWrapAround(cbuf, dataSize, cbuf->writePos);
            if (initSize)
            {
                uint32_t remainSize = dataSize - initSize;
                copyToAndUpdatePos(cbuf, data, initSize);
                copyToAndUpdatePos(cbuf, (data + initSize), remainSize);
            }
            else
            {
                copyToAndUpdatePos(cbuf, data, dataSize);
            }
        }
        else
        {
                /* anyhow dataSize needs to pushed by overwriting */
                uint32_t initSize = isWrapAround(cbuf, bufferAvail, cbuf->writePos);
                if (initSize)
                {
                    uint32_t remainSize = bufferAvail - initSize;
                    copyToAndUpdatePos(cbuf, data, initSize);
                    copyToAndUpdatePos(cbuf, (data + initSize), remainSize);
                }
                else
                {
                    copyToAndUpdatePos(cbuf, data, bufferAvail);
                }

                uint32_t remainData = dataSize - bufferAvail;
                enableOverWrite(cbuf, remainData);

                uint32_t remainSize = isWrapAround(cbuf, remainData, cbuf->writePos);
                uint32_t updatedPos = bufferAvail + remainSize;
                if (remainSize)
                {
                    uint32_t reqSize = remainData - remainSize;
                    copyToAndUpdatePos(cbuf, (data + bufferAvail), remainSize);
                    copyToAndUpdatePos(cbuf, (data + updatedPos), reqSize);
                }
                else
                {
                    copyToAndUpdatePos(cbuf, data + bufferAvail, remainData);
                }
        }
    }
}

uint32_t bufferPopData(cBufHandle cbuf, unsigned char *data, uint32_t reqSize)
{
    uint32_t ret = 0;
    if (cbuf && data && cbuf->buffer)
    {
        if (!bufferEmpty(cbuf))
        {
            /* Data availability and overlap */
            uint32_t dataAvail = getDataAvail(cbuf);
            if (dataAvail <= reqSize)
            {
                uint32_t initSize = isWrapAround(cbuf, dataAvail, cbuf->readPos);
                if (initSize)
                {
                    uint32_t remainSize = dataAvail - initSize;
                    copyFromAndUpdatePos(cbuf, data, initSize);
                    copyFromAndUpdatePos(cbuf, (data + initSize), remainSize);
                    ret = dataAvail;
                }
                else
                {
                    copyFromAndUpdatePos(cbuf, data, dataAvail);
                    ret = dataAvail;
                }
            }
            else
            {
                uint32_t initSize = isWrapAround(cbuf, reqSize, cbuf->readPos);
                if (initSize)
                {
                    uint32_t remainSize = reqSize - initSize;
                    copyFromAndUpdatePos(cbuf, data, initSize);
                    copyFromAndUpdatePos(cbuf, (data + initSize), remainSize);
                    ret = reqSize;
                }
                else
                {
                    copyFromAndUpdatePos(cbuf, data, reqSize);
                    ret = reqSize;
                }
            }
        }
        else
        {
            IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "CINEMO_AUD: BUFFER IS EMPTY NOW: NO DATA");
        }
    }
    return ret;
}

void bufferFree(cBufHandle cbuf)
{
    if (cbuf)
    {
        free(cbuf);
        cbuf = NULL;
    }
    else
    {
        IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR,"CINEMO_AUD: bufferFree() FAILED");
    }
}

void printBufferStatus(cBufHandle cbuf)
{
    IAP2USBPLUGINDLTLOG(DLT_LOG_DEBUG,"BufferFull: %x\n, Bufferempty: %x\n, Bufferavail: %x\n",
            isBufferFull(cbuf), bufferEmpty(cbuf), getBufferAvail(cbuf));
}

void printDataStatus(cBufHandle cbuf)
{
    IAP2USBPLUGINDLTLOG(DLT_LOG_DEBUG,"BufferFull: %x\n, Bufferempty: %x\n, Dataavail: %x\n",
            isBufferFull(cbuf), bufferEmpty(cbuf), getDataAvail(cbuf));
}

uint32_t getBitsPerSample(CTLI_AUDIO_SAMPLETYPE sampletype)
{
    uint32_t bits = 0;
    switch (sampletype)
    {
        case CTLI_AUDIO_SAMPLETYPE_S16LE:
            bits = 16;
            break;
        case CTLI_AUDIO_SAMPLETYPE_S24LE:
            bits = 24;
            break;
        default:
            break;
    }
    return bits;
}
