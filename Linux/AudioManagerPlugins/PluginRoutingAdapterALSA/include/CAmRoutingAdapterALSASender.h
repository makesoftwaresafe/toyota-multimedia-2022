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


#ifndef ROUTINGADAPTERALSASENDER_H_
#define ROUTINGADAPTERALSASENDER_H_

#include <memory>
#include "IAmRouting.h"
#include "IAmRoutingReceiverShadow.h"
#include "CAmRoutingAdapterALSAProxyDefault.h"
#include "CAmRoutingAdapterALSAVolume.h"
#include "CAmRoutingAdapterALSAdb.h"

#include "CAmCommandLineSingleton.h"

#include "CAmRoutingAdapterALSADeviceDetector.h"

#define RA_ALSA_BUSNAME  "ALSARoutingPlugin"

namespace am
{

class CAmRoutingAdapterALSASender : public IAmRoutingSend,
    public IAmRoutingReceiverObserver, public IAmRoutingAdapterDbObserver
{
public:

    CAmRoutingAdapterALSASender();
    virtual ~CAmRoutingAdapterALSASender();

    /* IAmRoutingSend*/
    am_Error_e startupInterface(IAmRoutingReceive* receiveInterface) override;
    am_Error_e returnBusName(std::string& busname) const override;
    void getInterfaceVersion(std::string& version) const override;

    void setRoutingReady(const uint16_t handle) override;
    void setRoutingRundown(const uint16_t handle) override;

    am_Error_e setDomainState(const am_domainID_t domainID, const am_DomainState_e domainState) override;

    am_Error_e asyncAbort(const am_Handle_s handle) override;

    am_Error_e asyncConnect(const am_Handle_s handle, const am_connectionID_t connectionID,
                            const am_sourceID_t sourceID, const am_sinkID_t sinkID,
                            const am_CustomConnectionFormat_t connectionFormat) override;

    am_Error_e asyncDisconnect(const am_Handle_s handle, const am_connectionID_t connectionID) override;

    am_Error_e asyncSetSinkVolume(const am_Handle_s handle, const am_sinkID_t sinkID, const am_volume_t volume,
                                  const am_CustomRampType_t ramp, const am_time_t time) override;

    am_Error_e asyncSetSourceVolume(const am_Handle_s handle, const am_sourceID_t sourceID, const am_volume_t volume,
                                    const am_CustomRampType_t ramp, const am_time_t time) override;

    am_Error_e asyncSetSourceState(const am_Handle_s handle, const am_sourceID_t sourceID,
                                   const am_SourceState_e state) override;

    am_Error_e asyncSetSinkSoundProperty(const am_Handle_s handle, const am_sinkID_t sinkID,
                                         const am_SoundProperty_s& soundProperty) override;

    am_Error_e asyncSetSinkSoundProperties(const am_Handle_s handle, const am_sinkID_t sinkID,
                                           const std::vector<am_SoundProperty_s>& listSoundProperties) override;

    am_Error_e asyncSetSourceSoundProperty(const am_Handle_s handle, const am_sourceID_t sourceID,
                                           const am_SoundProperty_s& soundProperty) override;

    am_Error_e asyncSetSourceSoundProperties(const am_Handle_s handle, const am_sourceID_t sourceID,
                                             const std::vector<am_SoundProperty_s>& listSoundProperties) override;

    am_Error_e asyncCrossFade(const am_Handle_s handle, const am_crossfaderID_t crossfaderID,
                              const am_HotSink_e hotSink, const am_CustomRampType_t rampType, const am_time_t time) override;

    am_Error_e asyncSetVolumes(const am_Handle_s handle, const std::vector<am_Volumes_s>& volumes) override;

    am_Error_e asyncSetSinkNotificationConfiguration(const am_Handle_s handle, const am_sinkID_t sinkID,
                                                     const am_NotificationConfiguration_s& notificationConfiguration) override;

    am_Error_e asyncSetSourceNotificationConfiguration(const am_Handle_s handle, const am_sourceID_t sourceID,
                                                       const am_NotificationConfiguration_s& notificationConfiguration) override;

    am_Error_e resyncConnectionState(const am_domainID_t domainID, std::vector<am_Connection_s>& listOfExistingConnections) override;

    /* IAmRoutingReceiverObserver*/
    void asyncDeleteVolume(const am_Handle_s handle, const CAmRoutingAdapterALSAVolume* reference) override;

    /* IAmRoutingAdapterDbObserver */
    void registerDomain(am_Domain_s & domain) override;
    void registerSource(ra_sourceInfo_s & info, am_domainID_t domainID) override;
    void registerSink(ra_sinkInfo_s & info, am_domainID_t domainID) override;
    void registerGateway(ra_gatewayInfo_s & info, am_domainID_t domainID) override;
    void hookDomainRegistrationComplete(am_domainID_t domainID) override;

    void deregisterDomain(const am_domainID_t &domainID) override;
    void deregisterSource(const am_sourceID_t &sourceID) override;
    void deregisterSink(const am_sinkID_t &sinkID) override;
    void deregisterGateway(const am_gatewayID_t &gatewayID) override;

    /*
     * Helper for USB
     */
    void peekSourceClassID(const std::string& name, am_sourceID_t& sourceID);
    void peekSinkClassID(const std::string& name, am_sinkID_t& sinkID);

private:
    static bool isNumber(const std::string& value);

private:
    IAmRoutingReceiverShadow  *mpShadow;
    IAmRoutingReceive         *mpReceiveInterface;
    CAmSocketHandler          *mpSocketHandler;

    CAmRoutingAdapterALSAdb   mDataBase;
    std::string               mBusname;

#ifdef WITH_DEVICE_DETECTOR
    std::shared_ptr<CAmRoutingAdapterALSADeviceDetector> mpDeviceDetector;
#endif /* WITH_DEVICE_DETECTOR */

    TCLAP::ValueArg<std::string> mCommandLineArg;

};

} /* namespace am*/

#endif /* ROUTINGADAPTERALSASENDER_H_ */
