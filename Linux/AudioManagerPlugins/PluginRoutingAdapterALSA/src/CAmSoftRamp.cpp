/*******************************************************************************
 *  \copyright (c) 2016 Advanced Driver Information Technology.
 *                   ADIT is a joint venture company of
 *   Robert Bosch GmbH/Robert Bosch Car Multimedia GmbH and DENSO Corporation
 *
 *  \author: Jens Lorenz, jlorenz@de.adit-jv.com 2016
 *           Mattia Guerra, mguerra@de.adit-jv.com 2016
 *
 *
 *  \copyright The MIT License (MIT)
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"),
 *  to deal in the Software without restriction, including without limitation
 *  the rights to use, copy, modify, merge, publish, distribute, sublicense,
 *  and/or sell copies of the Software, and to permit persons to whom the
 *  Software is furnished to do so, subject to the following conditions:
 *  The above copyright notice and this permission notice shall be included in
 *  all copies or substantial portions of the Software.
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 *  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 *  DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 *  OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR
 *  THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 *  For further information see http://www.genivi.org/.
 ******************************************************************************/

#include <CAmSoftRamp.h>
#include "CAmDLTLogging.h"

using namespace am;

#define NUM_BITS         8
#define FLOAT_MULTIPLIER 8000000

/*
 * These are the multiplication factors. each factor multiplies the incoming
 * samples as below
 * output_sample = (input_sample * factor) / (2 ^ NUM_BITS)
 */
const uint16_t CAmSoftRamp::mSteps[] =
{
    0x0000, 0x0001, 0x0002, 0x0003, 0x0004, 0x0005, 0x0006, 0x0007,
    0x0008, 0x0009, 0x000a, 0x000b, 0x000c, 0x000d, 0x000e, 0x000f,
    0x0010, 0x0011, 0x0012, 0x0013, 0x0014, 0x0015, 0x0016, 0x0017,
    0x0018, 0x0019, 0x001a, 0x001b, 0x001c, 0x001d, 0x001e, 0x001f,
    0x0020, 0x0021, 0x0022, 0x0023, 0x0024, 0x0025, 0x0026, 0x0027,
    0x0028, 0x0029, 0x002a, 0x002b, 0x002c, 0x002d, 0x002e, 0x002f,
    0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037,
    0x0038, 0x0039, 0x003a, 0x003b, 0x003c, 0x003d, 0x003e, 0x003f,
    0x0040, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047,
    0x0048, 0x0049, 0x004a, 0x004b, 0x004c, 0x004d, 0x004e, 0x004f,
    0x0050, 0x0051, 0x0052, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057,
    0x0058, 0x0059, 0x005a, 0x005b, 0x005c, 0x005d, 0x005e, 0x005f,
    0x0060, 0x0061, 0x0062, 0x0063, 0x0064, 0x0065, 0x0066, 0x0067,
    0x0068, 0x0069, 0x006a, 0x006b, 0x006c, 0x006d, 0x006e, 0x006f,
    0x0070, 0x0071, 0x0072, 0x0073, 0x0074, 0x0075, 0x0076, 0x0077,
    0x0078, 0x0079, 0x007a, 0x007b, 0x007c, 0x007d, 0x007e, 0x007f,
    0x0080, 0x0081, 0x0082, 0x0083, 0x0084, 0x0085, 0x0086, 0x0087,
    0x0088, 0x0089, 0x008a, 0x008b, 0x008c, 0x008d, 0x008e, 0x008f,
    0x0090, 0x0091, 0x0092, 0x0093, 0x0094, 0x0095, 0x0096, 0x0097,
    0x0098, 0x0099, 0x009a, 0x009b, 0x009c, 0x009d, 0x009e, 0x009f,
    0x00a0, 0x00a1, 0x00a2, 0x00a3, 0x00a4, 0x00a5, 0x00a6, 0x00a7,
    0x00a8, 0x00a9, 0x00aa, 0x00ab, 0x00ac, 0x00ad, 0x00ae, 0x00af,
    0x00b0, 0x00b1, 0x00b2, 0x00b3, 0x00b4, 0x00b5, 0x00b6, 0x00b7,
    0x00b8, 0x00b9, 0x00ba, 0x00bb, 0x00bc, 0x00bd, 0x00be, 0x00bf,
    0x00c0, 0x00c1, 0x00c2, 0x00c3, 0x00c4, 0x00c5, 0x00c6, 0x00c7,
    0x00c8, 0x0cc9, 0x00ca, 0x00cb, 0x00cc, 0x00cd, 0x00ce, 0x00cf,
    0x00d0, 0x00d1, 0x00d2, 0x00d3, 0x00d4, 0x00d5, 0x00d6, 0x00d7,
    0x00d8, 0x00d9, 0x00da, 0x00db, 0x00dc, 0x00dd, 0x00de, 0x00df,
    0x00e0, 0x00e1, 0x00e2, 0x00e3, 0x00e4, 0x00e5, 0x00e6, 0x00e7,
    0x00e8, 0x00e9, 0x00ea, 0x00eb, 0x00ec, 0x00ed, 0x00ee, 0x00ef,
    0x00f0, 0x00f1, 0x00f2, 0x00f3, 0x00f4, 0x00f5, 0x00f6, 0x00f7,
    0x00f8, 0x00f9, 0x00fa, 0x00fb, 0x00fc, 0x00fd, 0x00fe, 0x00ff,
};

CAmSoftRamp::CAmSoftRamp(int numChannels, snd_pcm_format_t format)
    : mNumChannels(numChannels)
    , mFormat(format)
    , mNumSamples(0)
    , mNumSamplesProcessed(0)
    , mMultiplierStep(0)
    , mNumSteps(0)
{
    mNumSteps = sizeof(mSteps) / sizeof(uint16_t);
}

CAmSoftRamp::~CAmSoftRamp()
{
}

void CAmSoftRamp::setTotalSamples(uint32_t numSamples)
{
    logAmRaInfo("CAmSoftRamp::setTotalSamples numSamples=", numSamples);
    mNumSamples     = numSamples;
    mMultiplierStep = mNumSamples / mNumSteps;
    if (mMultiplierStep == 0)
    {
        mMultiplierStep = 1;
    }
}

void CAmSoftRamp::reset()
{
    mNumSamplesProcessed = 0;
}

int64_t CAmSoftRamp::getSample(int numSample, int ChannelNum, uint8_t *data)
{
    int64_t sample = 0;
    switch (mFormat)
    {

    case SND_PCM_FORMAT_S8:
        sample = (int64_t)(((int8_t *)data)[(mNumChannels * numSample) + ChannelNum]);
        break;

    case SND_PCM_FORMAT_U8:
        sample = (int64_t)(data[(mNumChannels * numSample) + ChannelNum]) - 128;
        break;

    case SND_PCM_FORMAT_S16_LE:
    {
        uint8_t *ptr = data + 2 * ((mNumChannels * numSample) + ChannelNum);
        sample = ((int64_t)((int64_t)ptr[1] << 56 | (int64_t)ptr[0] << 48)) >> 48;
        break;
    }

    case SND_PCM_FORMAT_S16_BE:
    {
        uint8_t *ptr = data + 2 * ((mNumChannels * numSample) + ChannelNum);
        sample = ((int64_t)((int64_t)ptr[0] << 56 | (int64_t)ptr[1] << 48)) >> 48;
        break;
    }

    case SND_PCM_FORMAT_S24_LE:
    {
        uint8_t *ptr = data + 3 * ((mNumChannels * numSample) + ChannelNum);
        sample = ((int64_t)((int64_t)ptr[2] << 56 | (int64_t)ptr[1] << 48 | (int64_t)ptr[1] << 40)) >> 40;
        break;
    }

    case SND_PCM_FORMAT_S24_BE:
    {
        uint8_t *ptr = data + 3 * ((mNumChannels * numSample) + ChannelNum);
        sample = ((int64_t)((int64_t)ptr[0] << 56 | (int64_t)ptr[1] << 48 | (int64_t)ptr[2] << 40)) >> 40;
        break;
    }

    case SND_PCM_FORMAT_S32_LE:
    {
        uint8_t *ptr = data + 4 * ((mNumChannels * numSample) + ChannelNum);
        sample = ((int64_t)((int64_t)ptr[3] << 56 | (int64_t)ptr[2] << 48 | (int64_t)ptr[1] << 40
                            | (int64_t)ptr[0] << 32)) >> 32;
        break;
    }
    case SND_PCM_FORMAT_S32_BE:
    {
        uint8_t *ptr = data + 4 * ((mNumChannels * numSample) + ChannelNum);
        sample = ((int64_t)((int64_t)ptr[0] << 56 | (int64_t)ptr[1] << 48 | (int64_t)ptr[2] << 40
                            | (int64_t)ptr[3] << 32)) >> 32;
        break;
    }
    case SND_PCM_FORMAT_FLOAT_LE:
    {
        float *ptr = (float *)(data + 4 * ((mNumChannels * numSample) + ChannelNum));
        sample = (int64_t)((double)*ptr * (double)FLOAT_MULTIPLIER);
        break;
    }
    case SND_PCM_FORMAT_FLOAT_BE:
    {
        union
        {
            float fValue;
            uint8_t iValue[4];
        } float_uint8;

        uint8_t *ptr = data + 4 * ((mNumChannels * numSample) + ChannelNum);
        float_uint8.iValue[3] = ptr[0];
        float_uint8.iValue[2] = ptr[1];
        float_uint8.iValue[1] = ptr[2];
        float_uint8.iValue[0] = ptr[3];
        sample                = (int64_t)((double)(float_uint8.fValue) * (double)FLOAT_MULTIPLIER);
        break;
    }

    default:
        break;
    }

    return sample;
}

void CAmSoftRamp::setSample(int numSample, int ChannelNum, int64_t sample, uint8_t *data)
{
    switch (mFormat)
    {
    case SND_PCM_FORMAT_S8:
        ((int8_t *)data)[(mNumChannels * numSample) + ChannelNum] = (int8_t)sample;
        break;
    case SND_PCM_FORMAT_U8:
        sample                                       += 0x80;
        data[(mNumChannels * numSample) + ChannelNum] = (uint8_t)sample;
        break;

    case SND_PCM_FORMAT_S16_LE:
    {
        uint8_t *ptr = data + 2 * ((mNumChannels * numSample) + ChannelNum);
        ptr[0] = (sample & 0x00FF);
        ptr[1] = (sample & 0xFF00) >> 8;
        break;
    }

    case SND_PCM_FORMAT_S16_BE:
    {
        uint8_t *ptr = data + 2 * ((mNumChannels * numSample) + ChannelNum);
        ptr[0] = (sample & 0xFF00) >> 8;
        ptr[1] = (sample & 0x00FF);
        break;
    }
    case SND_PCM_FORMAT_S24_LE:
    {
        uint8_t *ptr = data + 3 * ((mNumChannels * numSample) + ChannelNum);
        ptr[0] = (sample & 0x0000FF);
        ptr[1] = (sample & 0x00FF00) >> 8;
        ptr[2] = (sample & 0xFF0000) >> 16;
        break;
    }
    case SND_PCM_FORMAT_S24_BE:
    {
        uint8_t *ptr = data + 3 * ((mNumChannels * numSample) + ChannelNum);
        ptr[0] = (sample & 0xFF0000) >> 16;
        ptr[1] = (sample & 0x00FF00) >> 8;
        ptr[2] = (sample & 0x0000FF);
        break;
    }
    case SND_PCM_FORMAT_S32_LE:
    {
        uint8_t *ptr = data + 4 * ((mNumChannels * numSample) + ChannelNum);
        ptr[0] = (sample & 0x000000FF);
        ptr[1] = (sample & 0x0000FF00) >> 8;
        ptr[2] = (sample & 0x00FF0000) >> 16;
        ptr[3] = (sample & 0xFF000000) >> 24;
        break;
    }
    case SND_PCM_FORMAT_S32_BE:
    {
        uint8_t *ptr = data + 4 * ((mNumChannels * numSample) + ChannelNum);
        ptr[0] = (sample & 0xFF000000) >> 24;
        ptr[1] = (sample & 0x00FF0000) >> 16;
        ptr[2] = (sample & 0x0000FF00) >> 8;
        ptr[3] = (sample & 0x000000FF);
        break;
    }
    case SND_PCM_FORMAT_FLOAT_LE:
    {
        float *ptr = (float *)(data + 4 * ((mNumChannels * numSample) + ChannelNum));
        *ptr = (float)((double)sample / (double)FLOAT_MULTIPLIER);
        break;
    }
    case SND_PCM_FORMAT_FLOAT_BE:
    {
        union
        {
            float fValue;
            uint8_t iValue[4];
        } float_uint8;

        float_uint8.fValue = (float)((double)sample / (double)FLOAT_MULTIPLIER);
        uint8_t *ptr = data + 4 * ((mNumChannels * numSample) + ChannelNum);
        ptr[0] = float_uint8.iValue[3];
        ptr[1] = float_uint8.iValue[2];
        ptr[2] = float_uint8.iValue[1];
        ptr[3] = float_uint8.iValue[0];
        break;
    }
    default:
        break;
    }
}

int64_t CAmSoftRamp::multiply(int64_t sample, uint8_t multiplier)
{
    int64_t result = 0;

    for (int i = 0; i < NUM_BITS; i++)
    {
        if (multiplier & (1 << i))
        {
            result += (sample >> (NUM_BITS - i));
        }
    }

    return result;
}

void CAmSoftRamp::ramp(uint32_t numSamples, uint8_t *data, ra_RampType_e rampType)
{
    uint32_t i = 0;
    logAmRaDebug("CAmSoftRamp::ramp", mNumSamplesProcessed);
    if (mNumSamplesProcessed >= mNumSamples)
    {
        logAmRaInfo("CAmSoftRamp::ramp called after ramp completion");
        return;
    }

    for (i = mNumSamplesProcessed; i < mNumSamplesProcessed + numSamples; i++)
    {
        if (i >= mNumSamples)
        {
            break;
        }

        unsigned int currentMultiplier = (rampType == RAMP_UP) ? (i / mMultiplierStep) : ((mNumSteps - 1) - (i / mMultiplierStep));
        if (currentMultiplier > (mNumSteps - 1))
        {
            currentMultiplier = (rampType == RAMP_UP) ? (mNumSteps - 1) : 0;
        }

        for (int j = 0; j < mNumChannels; j++)
        {
            int64_t sample = getSample(i - mNumSamplesProcessed, j, data);
            sample = multiply(sample, mSteps[currentMultiplier]);
            setSample(i - mNumSamplesProcessed, j, sample, data);
        }
    }

    mNumSamplesProcessed = i;
    return;

}
