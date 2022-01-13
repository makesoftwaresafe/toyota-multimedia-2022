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

#include "audiomanagertypes.h"

#ifndef _C_AM_ROUTING_ADAPTER_ALSA_PROXY_INFO_H_
#define _C_AM_ROUTING_ADAPTER_ALSA_PROXY_INFO_H_

namespace am
{
/**
 *  Scheduling settings
 */
struct ra_cpuSched_s
{
public:
    int32_t policy;
    int32_t priority;
};

/**
 * Struct providing low level details of Audio Resource
 */
struct ra_Proxy_s
{
public:
    std::string pcmSrc;
    std::string pcmSink;
    uint32_t format;
    uint32_t channels;
    uint32_t rate;
    uint8_t duplex;
    uint16_t msBuffersize;
    uint16_t msPrefill;
    uint16_t msInitTimeout;
    uint16_t numSoftRampPeriods;
    ra_cpuSched_s cpuScheduler;
};

/**
 * Struct providing details of Audio Resource in AudioManager acception
 */
struct ra_proxyInfo_s
{
public:
    am_RoutingElement_s route;
    ra_Proxy_s alsa;

    std::string domNam;
    std::string srcNam;
    std::string sinkNam;
    std::string pxyNam;

    std::vector<uint32_t> convertionMatrix;
    std::vector<uint32_t> listChannels;
    std::vector<uint32_t> listRates;
    std::vector<uint32_t> listPcmFormats;

public:
    ra_proxyInfo_s() {};
    ra_proxyInfo_s(const am_sourceID_t srcId, const am_sinkID_t sinkId)
    {
        route.sourceID = srcId;
        route.sinkID = sinkId;
    };
    ra_proxyInfo_s(const std::string & srcName, const std::string & sinkName)
    {
        srcNam = srcName;
        sinkNam = sinkName;
    };
    bool operator()(const ra_proxyInfo_s & elem) const
    {
        if (!srcNam.empty())
        {
            return ( (srcNam == elem.srcNam)
                 && ((sinkNam.empty()) || (sinkNam == elem.sinkNam)));
        }
        return ( (route.sourceID == elem.route.sourceID)
             && ((elem.route.sinkID == 0) || (route.sinkID == elem.route.sinkID)));
    };
};
}/* namespace am */
#endif /* _C_AM_ROUTING_ADAPTER_ALSA_PROXY_INFO_H_ */
