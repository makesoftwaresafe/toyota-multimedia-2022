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


#include <math.h>
#include <stdio.h>
#include <unistd.h>
#include <algorithm>
#include "audiomanagertypes.h"
#include "CAmRoutingAdapterALSAVolume.h"
#include "CAmRoutingAdapterALSASender.h"
#include "CAmDLTLogging.h"

DLT_DECLARE_CONTEXT (CRaALSAVolume)


using namespace am;
using namespace std;

template <typename T>
T calc_exp(T i, long int &sign, long int &steps, long int logArgument)
{
    return sign * (exp(static_cast<float>(i) / steps * log1p(logArgument)));
}


CAmRoutingAdapterALSAVolume::CAmRoutingAdapterALSAVolume(am_Handle_s handle, am_Volumes_s volumes,
        std::string pcmName, std::string volName, IAmRoutingReceive* routingInterface,
        CAmSocketHandler* socketHandler, IAmRoutingReceiverObserver* observer)
    : CAmRoutingAdapterThread(), mHandle(handle), mVolInfo(volumes), mShadow(routingInterface, socketHandler, observer)
{
    int err = CAmRoutingAdapterALSAMixerCtrl::openMixer(pcmName, volName);
    if (err < 0)
    {
        sendAcknowledge(err);
        logAmRaError(CAmRoutingAdapterALSAMixerCtrl::getStrError());
        throw runtime_error(CAmRoutingAdapterALSAMixerCtrl::getStrError());
    }
    CAmRoutingAdapterALSAMixerCtrl::activateVolumeChangeNotification();
    CAmRoutingAdapterThread::setThreadSched(SCHED_FIFO, 80);
    CAmRoutingAdapterThread::setThreadName("raa_vol_" + volName);
}

CAmRoutingAdapterALSAVolume::~CAmRoutingAdapterALSAVolume()
{
}

int CAmRoutingAdapterALSAVolume::initThread()
{
    /* this call prepares the member values to calculate the requested ramp */
    int err = prepareRampCalc();
    if (err == 0)
    {
        switch (mVolInfo.ramp)
        {
            case RAMP_GENIVI_EXP:
                calcExpRamp();
                break;
            case RAMP_GENIVI_EXP_INV:
                calcExpInvRamp();
                break;
            case RAMP_GENIVI_DIRECT:
            case RAMP_GENIVI_LINEAR:
            case RAMP_GENIVI_NO_PLOP:
            default:
                calcLiniarRamp();
                break;
        }
    }

    mRampItr = mRamp.begin();
    return err;
}

int CAmRoutingAdapterALSAVolume::workerThread()
{
    /* force an ABORT of workerThread() in case ramp is fully executed */
    int err = mRamp.size();

    if (mRampItr != mRamp.end())
    {
        usleep(mRampItr->time);
        err = std::min(0, CAmRoutingAdapterALSAMixerCtrl::setVolume(mRampItr->alsaVolume));
        if (err == 0)
        {
            mRampItr++;
        }
    }

    return err;
}

void CAmRoutingAdapterALSAVolume::deinitThread(int errInit)
{
    sendAcknowledge(errInit);
}

int CAmRoutingAdapterALSAVolume::prepareRampCalc()
{
    long int minVol = AM_MUTE;
    long int maxVol = 0;

    /* try to set the audiomanager range ... */
    int err = CAmRoutingAdapterALSAMixerCtrl::setRange(minVol, maxVol);
    if (err == 0)
    {
        mRange = VOL_RANGE;
        err = CAmRoutingAdapterALSAMixerCtrl::getVolume(mBegVol);
        mEndVol = mVolInfo.volume;
    }
    else
    {
        /* ... if it is not possible than get the range */
        err = CAmRoutingAdapterALSAMixerCtrl::getRange(minVol, maxVol);
        if (err > 0)
        {
            mRange = err;
            err = CAmRoutingAdapterALSAMixerCtrl::getVolume(mBegVol);
            mEndVol = (static_cast<long int>(mVolInfo.volume) * mRange) / VOL_RANGE;
        }
    }

    /* set here the minVol and maxVol in Ramp, so that filtered_push_back can also manage that. */
    mRamp.setMaxVol(maxVol);
    mRamp.setMinVol(minVol);

    if (err == 0)
    {
        mSteps = mVolInfo.time / VOL_QUANT_MS;
        mSleep = VOL_QUANT_MS * 1000;
        if (mVolInfo.ramp == RAMP_GENIVI_DIRECT || 1 >= mSteps)
        {
            mVolInfo.ramp = RAMP_GENIVI_DIRECT;
            mSteps = 1;
            mSleep = 0;
        }
    }
    else
    {
        logAmRaError(CAmRoutingAdapterALSAMixerCtrl::getStrError());
    }

    return err;
}

void CAmRoutingAdapterALSAVolume::calcLiniarRamp()
{
    ra_ramp_s val;
    val.time = mSleep;

    long int slope = (mEndVol - mBegVol) / mSteps;
    for (uint16_t i = 1; i < mSteps; ++i)
    {
        val.alsaVolume = mBegVol + static_cast<long int>(i) * slope;
        mRamp.filtered_push_back(val);
    }
    val.alsaVolume = mEndVol;
    mRamp.filtered_push_back(val);
}

void CAmRoutingAdapterALSAVolume::calcExpRamp()
{
    ra_ramp_s val;
    val.time = mSleep;
    long int sign = (mEndVol > mBegVol) ? 1 : -1;
    for (long int i = 1; i < mSteps ; ++i)
    {
        val.alsaVolume = mBegVol + calc_exp(i, sign, mSteps, sign * mEndVol - sign * mBegVol);
        mRamp.filtered_push_back(val);
    }
    val.alsaVolume = mEndVol;
    mRamp.filtered_push_back(val);
}

void CAmRoutingAdapterALSAVolume::calcExpInvRamp()
{
    ra_ramp_s val;
    val.time = mSleep;
    long int sign = (mEndVol > mBegVol) ? 1 : -1;
    for (long int i = mSteps; i >= 2; --i)
    {
        val.alsaVolume = mEndVol - calc_exp(i, sign, mSteps, sign * mEndVol - sign * mBegVol);
        mRamp.filtered_push_back(val);
    }
    val.alsaVolume = mEndVol;
    mRamp.filtered_push_back(val);
}

void CAmRoutingAdapterALSAVolume::sendAcknowledge(int err)
{
    am_Error_e error = E_OK;

    /* send in case of issue to correct error value */
    if (err < 0)
    {
        error = E_UNKNOWN;
    }
    else if ((err == 0) && (mRampItr != mRamp.end()))
    {
        error = E_ABORTED;
    }
    else
    {
        mCurrentVol = mVolInfo.volume;
    }

    if (mHandle.handleType == H_SETSINKVOLUME)
    {
        mShadow.ackSetSinkVolumeChange(mHandle, mCurrentVol, error);
    }
    else
    {
        mShadow.ackSetSourceVolumeChange(mHandle, mCurrentVol, error);
    }

    mShadow.asyncDeleteVolume(mHandle, this);
}

int CAmRoutingAdapterALSAVolume::cbVolumeChange(const long int & volume)
{
    long int minVol;
    long int maxVol;
    long int range = CAmRoutingAdapterALSAMixerCtrl::getRange(minVol, maxVol);
    if ((range == VOL_RANGE) && (minVol == AM_MUTE) && (maxVol == 0))
    {
        mCurrentVol = volume;
    }
    else
    {
        mCurrentVol = ((volume * VOL_RANGE) / range) + AM_MUTE;
    }

    if (mHandle.handleType == H_SETSINKVOLUME)
    {
        mShadow.ackSinkVolumeTick(mHandle, mVolInfo.volumeID.sink, mCurrentVol);
    }
    else
    {
        mShadow.ackSourceVolumeTick(mHandle, mVolInfo.volumeID.source, mCurrentVol);
    }

    return 0;
}
