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


#ifndef ROUTINGADAPTERALSAPROXYDEFAULT_H_
#define ROUTINGADAPTERALSAPROXYDEFAULT_H_

#include "IAmRoutingAdapterALSAProxy.h"
#include "CAmRoutingAdapterThread.h"
#include <alsa/asoundlib.h>
#include "CAmSoftRamp.h"

namespace am
{

class CAmRoutingAdapterALSAProxyDefault : public IAmRoutingAdapterALSAProxy, public CAmRoutingAdapterThread
{
public:
    CAmRoutingAdapterALSAProxyDefault(const ra_Proxy_s & proxy);
    virtual ~CAmRoutingAdapterALSAProxyDefault();

    /* IAmRoutingAdapterALSAProxy */
    am_timeSync_t getDelay() const override;
    am_Error_e openStreaming() override;
    am_Error_e startStreaming() override;
    am_Error_e stopStreaming() override;
    am_Error_e closeStreaming() override;



private:
    struct ap_data_t
    {
    public:
        const char *name;
        snd_pcm_t *hndl;
        snd_pcm_hw_params_t *hwPar;
        snd_pcm_sw_params_t *swPar;
    public:
        ap_data_t()
        {
            name = NULL;
            hndl = NULL;
            hwPar = NULL;
            swPar = NULL;
        }
    };

    /**
     * \brief Sets Hardware Parameters through ALSA APIs
     *
     * This method sets Format, Channels and Rate provided by the ra_proxyInfo_s
     *
     * \param[in] struct with pointers to ALSA name, sw and hw parameters
     * \param[out] ALSA APIs' errors
     */
    int setHwParams(ap_data_t & data);
    /**
     * \brief Sets Buffer Size through ALSA APIs
     *
     * Sets the Ring Buffer Size checking for suitable values from Hardware side.
     * If a fitting configuration is found, Ring Buffer Size is set to 3 times the
     * period size associated to the specified msBuffersize
     *
     * \param[in] struct with pointers to ALSA name, sw and hw parameters
     * \param[in] bool to establish if the function was already invoked before
     * \param[out] ALSA APIs' errors
     */
    int setHwBuffSize(ap_data_t & data, bool firstTry = true);
    /**
     * \brief Sets Software Parameters through ALSA APIs
     *
     * This method sets the minimum available frame to one and threshold to two
     * to avoid big latencies while forwarding.
     *
     * \param[in] struct with pointers to ALSA name, sw and hw parameters
     * \param[out] ALSA APIs' errors
     */
    int setSwParams(ap_data_t & data);

    /* CAmRoutingAdapterThread */
    int initThread() override;
    int workerThread() override;
    void deinitThread(int errInit) override;

    /**
     * Reads a buffer from device using ALSA APIs
     * \param[in] struct with pointers to ALSA name, sw and hw parameters
     * \param[in] pointer to buffer
     * \param[in] size of buffer (in ALSA Frames Quantity)
     * \param[out] ALSA APIs' errors
     */
    int readFromDevice(ap_data_t &device, void *buffer, int sizeOfBuffer);
    /**
     * Writes a buffer to device using ALSA APIs
     * \param[in] struct with pointers to ALSA name, sw and hw parameters
     * \param[in] pointer to buffer
     * \param[in] size of buffer (in ALSA Frames Quantity)
     * \param[out] ALSA APIs' errors
     */
    int writeToDevice(ap_data_t &device, void *buffer, int sizeOfBuffer);
    /**
     * Sequence for recovering:
     * 1) Drop/Stop the device
     * 2) Create Prefill injecting silence for syncing aim on next writing operation
     * 3) Prepare the device
     * 4) Start the device manually if it's Capture Device
     * \param[in] struct with pointers to ALSA name, sw and hw parameters
     * \param[out] ALSA APIs' errors
     */
    int prepareWithPrefill(ap_data_t &device);
    /**
     * Creates PreFill Buffer, that is a 0 filled buffer injected to ALSA Ring Buffer.
     * The size is calculated based on msPrefill specified through XML attribute.
     * Value should be set compatible with the Ring Buffer Size set.
     *
     * \param[out] 0 when successful, -ENOMEM otherwise
     */
    int createPrefill();
    /**
     * Deallocates resources for PreFill Buffer
     */
    void destroyPrefill();

    void deinitPlaybackDevice(void);
    void deinitCaptureDevice(void);
    template <typename F>
    void throwAssert( std::string functionName, uint32_t lineNumber, F func, int expectedReturn);

    struct
    {
        char *mPreFill;
        size_t prefillByteSize;
        snd_pcm_uframes_t mPerSize;
    }ra_Prefill_s;

    char              *mpCopyBuffer;
    snd_pcm_uframes_t mPerSize;
    snd_pcm_uframes_t mBufSize;
    ap_data_t         mPb;
    ap_data_t         mCap;
    ra_RampType_e     mRampType;
    uint32_t          mRampCnt;
    CAmSoftRamp       mSoftRamp;
    bool              mCaptureClosed;
    bool              mInitDone;
    pthread_mutex_t   mMtx;
    pthread_cond_t    mCond;

};

} /* namespace am */
#endif /* ROUTINGADAPTERALSAPROXYDEFAULT_H_ */
