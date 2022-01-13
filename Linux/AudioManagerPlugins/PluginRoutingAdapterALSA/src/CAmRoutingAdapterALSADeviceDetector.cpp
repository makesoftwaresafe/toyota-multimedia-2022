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

#ifdef WITH_DEVICE_DETECTOR

#include <limits>

#include "CAmRoutingAdapterALSADeviceDetector.h"
#include "CAmRoutingAdapterALSAdb.h"
#include "CAmRoutingAdapterALSASender.h"

#include "CAmDLTLogging.h"

using namespace std;
using namespace am;

CAmRoutingAdapterALSADeviceDetector::CAmRoutingAdapterALSADeviceDetector(CAmSocketHandler *mpSocketHandler, CAmRoutingAdapterALSASender *mpSender, CAmRoutingAdapterALSAdb &db) :
        receiveCB(this, &CAmRoutingAdapterALSADeviceDetector::receiveData), //
        mPollHandle(),
        mFd(-1),
        mpSender(mpSender),
        mDatabase(db)
{
    if (!mDatabase.getUSBInfo()->domNam.empty())
    {
        mpUdev = udev_new();
        if (!mpUdev)
            throw bad_alloc();
        mpMon = udev_monitor_new_from_netlink(mpUdev, "udev");
        if (!mpMon)
            throw bad_alloc();
        if (udev_monitor_filter_add_match_subsystem_devtype(mpMon, "sound", NULL))
            throw runtime_error("Failed to attach filter to monitor");
        if (udev_monitor_enable_receiving(mpMon))
            throw runtime_error("Failed to enable receiving");
        mFd = udev_monitor_get_fd(mpMon);

        short events = POLLIN;
        mpSocketHandler->addFDPoll(getFD(), events, NULL, &receiveCB, NULL, NULL, NULL, mPollHandle);
    }
}

void CAmRoutingAdapterALSADeviceDetector::enumerateAndRegister()
{
    struct udev_enumerate *enumerate;
    enumerate = udev_enumerate_new(mpUdev);
    udev_enumerate_add_match_subsystem(enumerate, "sound");
    udev_enumerate_scan_devices(enumerate);
    struct udev_list_entry *devices;
    struct udev_list_entry *dev_list_entry;
    devices = udev_enumerate_get_list_entry(enumerate);
    udev_list_entry_foreach(dev_list_entry, devices)
    {
        const char *path;
        path = udev_list_entry_get_name(dev_list_entry);
        std::string sysPath = path;
        if (sysPath.find("usb") != std::string::npos)
        {
            struct udev_device *dev;
            dev = udev_device_new_from_syspath(mpUdev, path);
            parseSysPath(path, dev);
        }
    }
    udev_enumerate_unref(enumerate);
}

CAmRoutingAdapterALSADeviceDetector::~CAmRoutingAdapterALSADeviceDetector()
{

}

void CAmRoutingAdapterALSADeviceDetector::fromSysPathToCardNumber(string &sysPathString, string &cardNumber)
{
    sysPathString = sysPathString.substr(sysPathString.find_last_of("/") + 1);
    if (sysPathString.substr(0, 8).compare("controlC") == 0)
    {
        cardNumber = sysPathString.substr(8);
    }
}

void CAmRoutingAdapterALSADeviceDetector::registerSink(
    ra_domainInfo_s *domainUSB, const string &cardNumber, const CardLabel &cardLabel)
{
    if (cardLabel.id.empty())
    {
        return;
    }

    /*
     * Save Card# VS resource name in mapping
     */
    for (auto && it : mDatabase.getUSBInfo()->lSinkInfo)
    {
        mDatabase.getUSBInfo()->mapCardToSinkName[cardNumber].push_back(cardLabel.name + " [" + cardLabel.id + "][" + it.sinkClsNam + "]");

        ra_sinkInfo_s sinkInfo;

        sinkInfo.amInfo.domainID = domainUSB->domain.domainID;
        sinkInfo.domNam = domainUSB->domain.name;
        sinkInfo.amInfo.sinkID = it.amInfo.sinkID;
        sinkInfo.amInfo.name = mDatabase.getUSBInfo()->mapCardToSinkName[cardNumber].at(&it - &mDatabase.getUSBInfo()->lSinkInfo[0]);
        sinkInfo.amInfo.visible = it.amInfo.visible;
        sinkInfo.amInfo.muteState = it.amInfo.muteState;
        sinkInfo.devTyp = it.devTyp;
        sinkInfo.sinkClsNam = it.sinkClsNam;

        mpSender->peekSinkClassID(sinkInfo.sinkClsNam, sinkInfo.amInfo.sinkClassID);

        mpSender->registerSink(sinkInfo, sinkInfo.amInfo.domainID);

        /*
         * Add the entry to our db
         */
        domainUSB->lSinkInfo.push_back(sinkInfo);
    }
}

void CAmRoutingAdapterALSADeviceDetector::registerSource(
    ra_domainInfo_s *domainUSB, const string &cardNumber, const CardLabel &cardLabel)
{
    if (cardLabel.id.empty())
    {
        return;
    }

    /*
     * Save Card# VS resource name in mapping
     */
    for (auto && it : mDatabase.getUSBInfo()->lSourceInfo)
    {
        mDatabase.getUSBInfo()->mapCardToSourceName[cardNumber].push_back(cardLabel.name + " [" + cardLabel.id + "][" +  it.srcClsNam + "]");

        ra_sourceInfo_s sourceInfo;

        sourceInfo.amInfo.domainID = domainUSB->domain.domainID;
        sourceInfo.domNam = domainUSB->domain.name;
        sourceInfo.amInfo.sourceID = it.amInfo.sourceID;
        sourceInfo.amInfo.name = mDatabase.getUSBInfo()->mapCardToSourceName[cardNumber].at(&it - &mDatabase.getUSBInfo()->lSourceInfo[0]);
        sourceInfo.amInfo.visible = it.amInfo.visible;
        sourceInfo.amInfo.sourceState = it.amInfo.sourceState;
        sourceInfo.devTyp = it.devTyp;
        sourceInfo.srcClsNam = it.srcClsNam;

        mpSender->peekSourceClassID(sourceInfo.srcClsNam, sourceInfo.amInfo.sourceClassID);

        mpSender->registerSource(sourceInfo, sourceInfo.amInfo.domainID);

        /*
         * Add the entry to our db
         */
        domainUSB->lSourceInfo.push_back(sourceInfo);
    }
}

void CAmRoutingAdapterALSADeviceDetector::getAlsaInfo(const string &cardNumber, CardLabel & cardLabelPlayback, CardLabel & cardLabelCapture)
{
    int err;
    snd_ctl_t *handle = NULL;
    snd_ctl_card_info_t *info;
    snd_pcm_info_t *pcminfo;
    snd_ctl_card_info_alloca(&info);
    snd_pcm_info_alloca(&pcminfo);
    string alsaHwPath = "hw:" + cardNumber;
    if ((err = snd_ctl_open(&handle, alsaHwPath.c_str(), 0)) < 0)
    {
        logAmRaError("CAmRoutingAdapterALSADeviceDetector::getAlsaInfo - snd_ctl_open failed while opening", alsaHwPath, "with error code", err);
        /*
         * Quit here, otherwise the invalid handle will cause crashes ahead
         */
        return;
    }
    if ((err = snd_ctl_card_info(handle, info)) < 0)
    {
        logAmRaError("CAmRoutingAdapterALSADeviceDetector::getAlsaInfo - snd_ctl_card_info failed with error code", err);
    }
    else
    {
        int dev = -1;
        if (snd_ctl_pcm_next_device(handle, &dev)<0)
        {
            logAmRaError("CAmRoutingAdapterALSADeviceDetector::getAlsaInfo - snd_ctl_pcm_next_device failed with error code", err);
        }
        if (dev >= 0)
        {
            snd_pcm_info_set_device(pcminfo, dev);
            snd_pcm_info_set_subdevice(pcminfo, 0);
            /*
             * Playback direction
             */
            snd_pcm_info_set_stream(pcminfo, SND_PCM_STREAM_PLAYBACK);
            if ((err = snd_ctl_pcm_info(handle, pcminfo)) >= 0)
            {
                cardLabelPlayback.id = snd_ctl_card_info_get_id(info);
                cardLabelPlayback.name = snd_ctl_card_info_get_name(info);
            }
            /*
             * Capture direction
             */
            snd_pcm_info_set_stream(pcminfo, SND_PCM_STREAM_CAPTURE);
            if ((err = snd_ctl_pcm_info(handle, pcminfo)) >= 0)
            {
                cardLabelCapture.id = snd_ctl_card_info_get_id(info);
                cardLabelCapture.name = snd_ctl_card_info_get_name(info);
            }
        }
    }
    if (handle)
    {
        snd_ctl_close(handle);
    }
}

void CAmRoutingAdapterALSADeviceDetector::deregisterSource(ra_domainInfo_s *domainUSB, const string &cardNumber)
{
    for (auto && it : mDatabase.getUSBInfo()->lSourceInfo)
    {
        ra_sourceInfo_s *source = mDatabase.findElement<ra_sourceInfo_s>(domainUSB->domain.domainID, mDatabase.getUSBInfo()->mapCardToSourceName[cardNumber].at(&it - &mDatabase.getUSBInfo()->lSourceInfo[0]));
        if (source)
        {
            mpSender->deregisterSource(source->amInfo.sourceID);
            domainUSB->lSourceInfo.erase(static_cast<vector<ra_sourceInfo_s>::iterator>(source));
        }
    }
    mDatabase.getUSBInfo()->mapCardToSourceName.erase(cardNumber);
}

void CAmRoutingAdapterALSADeviceDetector::deregisterSink(ra_domainInfo_s *domainUSB, const string &cardNumber)
{
    for (auto && it : mDatabase.getUSBInfo()->lSinkInfo)
    {
        ra_sinkInfo_s *sink = mDatabase.findElement<ra_sinkInfo_s>(domainUSB->domain.domainID, mDatabase.getUSBInfo()->mapCardToSinkName[cardNumber].at(&it - &mDatabase.getUSBInfo()->lSinkInfo[0]));
        if (sink)
        {
            mpSender->deregisterSink(sink->amInfo.sinkID);
            domainUSB->lSinkInfo.erase(static_cast<vector<ra_sinkInfo_s>::iterator>(sink));
        }
    }
    mDatabase.getUSBInfo()->mapCardToSinkName.erase(cardNumber);
}

void CAmRoutingAdapterALSADeviceDetector::parseSysPath(const char *path, struct udev_device *dev, const bool isFromMonitor)
{
    /*
     * Retrieve the USB Domain
     */
    ra_domainInfo_s *domainUSB = mDatabase.findDomain(mDatabase.getUSBInfo()->domNam);
    if (domainUSB == NULL)
    {
        return;
    }
    string sysPathString;
    string cardNumber;
    string actionType;
    if (!path)
    {
        return;
    }
    sysPathString = path;
    /*
     * Check if ending Syspath is controlC# and get the card number
     */
    fromSysPathToCardNumber(sysPathString, cardNumber);
    if (!cardNumber.empty())
    {
        const char *action = udev_device_get_action(dev);
        if (action || !isFromMonitor)
        {
            string actionType = isFromMonitor ? action : "add";
            if (actionType.compare("add") == 0)
            {
                /*
                 * Access the control info via ALSA APIs
                 */
                CardLabel cardLabelPlayback;
                CardLabel cardLabelCapture;
                getAlsaInfo(cardNumber, cardLabelPlayback, cardLabelCapture);
                /*
                 * Registering resources here
                 */
                registerSource(domainUSB, cardNumber, cardLabelCapture);
                registerSink(domainUSB, cardNumber, cardLabelPlayback);
            }
            else if (actionType.compare("remove") == 0)
            {
                /*
                 * Deregistering the resource here
                 */
                deregisterSource(domainUSB, cardNumber);
                deregisterSink(domainUSB, cardNumber);
            }
            else
            {
                logAmRaError("CAmRoutingAdapterALSADeviceDetector::parseSysPath - Unexpected Action from udev");
            }
        }
    }
    udev_device_unref(dev);
}

void CAmRoutingAdapterALSADeviceDetector::manageDetection()
{
    struct udev_device *dev = udev_monitor_receive_device(mpMon);
    if (!dev)
    {
        logAmRaInfo("CAmRoutingAdapterALSADeviceDetector::manageDetection - Device is NULL");
        return;
    }
    const char *path;
    path = udev_device_get_syspath(dev);
    parseSysPath(path, dev, true);
}

void CAmRoutingAdapterALSADeviceDetector::receiveData(const pollfd pollfd, const sh_pollHandle_t handle, void* userData)
{
    (void)pollfd;
    (void)handle;
    (void)userData;
    manageDetection();
}

#endif /* WITH_DEVICE_DETECTOR */

