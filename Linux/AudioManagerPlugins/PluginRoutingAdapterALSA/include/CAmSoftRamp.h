/*******************************************************************************
 *  \copyright (c) 2016 Advanced Driver Information Technology.
 *                   ADIT is a joint venture company of
 *   Robert Bosch GmbH/Robert Bosch Car Multimedia GmbH and DENSO Corporation
 *
 *  \author: Jens Lorenz, jlorenz@de.adit-jv.com 2015-2016
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

#ifndef _C_AM_SOFTRAMP_H_
#define _C_AM_SOFTRAMP_H_

#include <alsa/asoundlib.h>
#include <stdint.h>
#include <string>
#include <fstream>

namespace am
{

enum ra_RampType_e
{
    RAMP_UP,
    RAMP_DOWN,
};

class CAmSoftRamp
{
public:
    CAmSoftRamp(int numChannels, snd_pcm_format_t format);
    virtual ~CAmSoftRamp();
    void setTotalSamples(uint32_t numSamples);
    void reset();
    void ramp(uint32_t numSamples, uint8_t *data, ra_RampType_e rampType = RAMP_UP);

private:
    int64_t getSample(int numSample, int ChannelNum, uint8_t *data);
    void setSample(int numSample, int ChannelNum, int64_t sample, uint8_t *data);
    int64_t multiply(int64_t sample, uint8_t multiplier);

private:
    uint8_t               mNumChannels;
    snd_pcm_format_t      mFormat;
    uint32_t              mNumSamples;
    uint32_t              mNumSamplesProcessed;
    uint32_t              mMultiplierStep;
    uint32_t              mNumSteps;
    static const uint16_t mSteps[];
};

}

#endif /* _C_AM_SOFTRAMP_H_ */
