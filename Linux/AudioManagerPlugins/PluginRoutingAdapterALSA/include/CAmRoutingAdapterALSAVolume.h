/*******************************************************************************
 *  \copyright (c) 2016 Advanced Driver Information Technology.
 *                   ADIT is a joint venture company of
 *   Robert Bosch GmbH/Robert Bosch Car Multimedia GmbH and DENSO Corporation
 *
 *  \author: Jens Lorenz, jlorenz@de.adit-jv.com 2013-2016
 *           Mattia Guerra, mguerra@de.adit-jv.com 2016
 *           Jayanth MC, Jayanth.mc@in.bosch.com 2013-2014
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


#ifndef ROUTINGADAPTERALSA_VOLUME_H_
#define ROUTINGADAPTERALSA_VOLUME_H_

#include "IAmRoutingReceiverShadow.h"
#include "CAmRoutingAdapterThread.h"
#include "CAmRoutingAdapterALSAMixerCtrl.h"
#include <climits>

#define VOL_RANGE 3000
#define VOL_QUANT_MS 20
#define MAX_RAMP_TIME (VOL_RANGE * VOL_QUANT_MS)

namespace am
{

struct ra_ramp_s
{
    long int alsaVolume;
    __useconds_t time;
};

class CAmRoutingAdapterAlsaRamp
{
public:
    CAmRoutingAdapterAlsaRamp() : mMinVol(std::numeric_limits<long int>::max()),
        mMaxVol(std::numeric_limits<long int>::min())
    {
    }
    ~CAmRoutingAdapterAlsaRamp() {}

    inline void setMinVol(long int &value)
    {
        mMinVol = value;
    }

    inline void setMaxVol(long int &value)
    {
        mMaxVol = value;
    }

    typedef std::vector<ra_ramp_s>::iterator ramp_iterator;

    inline ramp_iterator begin()
    {
        return mValues.begin();
    };

    inline ramp_iterator end()
    {
        return mValues.end();
    };

    inline int size()
    {
        return mValues.size();
    };

    void filtered_push_back(ra_ramp_s & val)
    {
        size_t size = mValues.size();
        val.alsaVolume = std::min(val.alsaVolume, mMaxVol);
        val.alsaVolume = std::max(val.alsaVolume, mMinVol);
        if (size == 0 || val.alsaVolume != mValues.at(size - 1).alsaVolume)
            mValues.push_back(val);
        else
            mValues.at(size - 1).time += val.time;
    };

private:
    std::vector<ra_ramp_s> mValues;
    long int mMinVol;
    long int mMaxVol;
};


class CAmRoutingAdapterALSAVolume : private CAmRoutingAdapterALSAMixerCtrl, private CAmRoutingAdapterThread
{
public:
    CAmRoutingAdapterALSAVolume(am_Handle_s handle, am_Volumes_s volumes,
            std::string pcmName, std::string volName, IAmRoutingReceive* routingInterface,
            CAmSocketHandler* socketHandler, IAmRoutingReceiverObserver* observer);
    ~CAmRoutingAdapterALSAVolume();

    void startFading() {
        CAmRoutingAdapterThread::startThread();
    };

private:
    /* CAmRoutingAdapterThread */
    int initThread() override;
    int workerThread() override;
    void deinitThread(int errInit) override;

    /* CAmRoutingAdapterALSAMixerCtrl */
    int cbVolumeChange(const long int & volume) override;

private:
    int prepareRampCalc();
    void calcLiniarRamp();
    void calcExpRamp();
    void calcExpInvRamp();
    void sendAcknowledge(int err);

    am_Handle_s mHandle;                // AM request handle
    am_Volumes_s mVolInfo;              // Volume request
    am_volume_t mCurrentVol;            // Current volume
    IAmRoutingReceiverShadow mShadow;   // Sender class to inform thread exit

    /* ALSA stuff */
    long int mRange;                    // Volume Range of ALSA mixer
    long int mBegVol;                   // Begin ALSA Volume
    long int mEndVol;                   // End ALSA Volume
    long int mSteps;                    // Amount of Volume steps
    __useconds_t mSleep;                // Calculated Sleep Time

    CAmRoutingAdapterAlsaRamp mRamp;
    CAmRoutingAdapterAlsaRamp::ramp_iterator mRampItr;
};

} /* namespace am */

#endif /* ROUTINGADAPTERALSA_VOLUME_H_ */
