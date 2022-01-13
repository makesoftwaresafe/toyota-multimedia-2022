/*******************************************************************************
 *  \copyright (c) 2017 Advanced Driver Information Technology.
 *                   ADIT is a joint venture company of
 *   Robert Bosch GmbH/Robert Bosch Car Multimedia GmbH and DENSO Corporation
 *
 *  \author: Jens Lorenz, jlorenz@de.adit-jv.com 2013-2017
 *           Mattia Guerra, mguerra@de.adit-jv.com 2016-2017
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


#ifndef ROUTINGADAPTERALSADEVICEDETECTOR_H_
#define ROUTINGADAPTERALSADEVICEDETECTOR_H_

#ifdef WITH_DEVICE_DETECTOR

#include <libudev.h>
#include <alsa/asoundlib.h>
#include <string>

#include <CAmSocketHandler.h>
#include <CAmRoutingAdapterALSAdb.h>

namespace am
{

class CAmRoutingAdapterALSASender;

struct CardLabel
{
    std::string id;
    std::string name;
};

class CAmRoutingAdapterALSADeviceDetector
{
public:
    CAmRoutingAdapterALSADeviceDetector(CAmSocketHandler *mpSocketHandler, CAmRoutingAdapterALSASender *mpSender, CAmRoutingAdapterALSAdb &db);
    virtual ~CAmRoutingAdapterALSADeviceDetector();
    int getFD()
    {
        return mFd;
    }

    void receiveData(const pollfd pollfd, const sh_pollHandle_t handle, void* userData);

    TAmShPollFired<CAmRoutingAdapterALSADeviceDetector> receiveCB;

    sh_pollHandle_t mPollHandle;

    void manageDetection();
    void enumerateAndRegister();

private:
    void parseSysPath(const char *path, struct udev_device *dev,const bool isFromMonitor = false);
    void getAlsaInfo(const std::string &cardNumber,
        CardLabel &cardLabelPlayback, CardLabel &cardLabelCapture);
    void deregisterSource(ra_domainInfo_s *domainUSB, const std::string &cardNumber);
    void deregisterSink(ra_domainInfo_s *domainUSB, const std::string &cardNumber);
    void fromSysPathToCardNumber(std::string &sysPathString, std::string &cardNumber);
    void registerSource(ra_domainInfo_s *domainUSB, const std::string &cardNumber, const CardLabel &cardLabel);
    void registerSink(ra_domainInfo_s *domainUSB, const std::string &cardNumber, const CardLabel &cardLabel);
private:
    struct udev *mpUdev;
    struct udev_monitor *mpMon;
    int mFd;
    CAmRoutingAdapterALSASender *mpSender;
    CAmRoutingAdapterALSAdb     &mDatabase;
};

} /* namespace am */

#endif /* WITH_DEVICE_DETECTOR */
#endif /* ROUTINGADAPTERALSADEVICEDETECTOR_H_ */
