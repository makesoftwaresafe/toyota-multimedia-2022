/*******************************************************************************
 *  \copyright (c) 2016 Advanced Driver Information Technology.
 *                   ADIT is a joint venture company of
 *   Robert Bosch GmbH/Robert Bosch Car Multimedia GmbH and DENSO Corporation
 *
 *  \author: Jens Lorenz, jlorenz@de.adit-jv.com 2013-2016
 *           Mattia Guerra, mguerra@de.adit-jv.com 2016
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


#include <cassert>
#include <algorithm>
#include "CAmRoutingAdapterALSAdb.h"
#include "CAmDLTLogging.h"

using namespace std;
using namespace am;


CAmRoutingAdapterALSAdb::CAmRoutingAdapterALSAdb(IAmRoutingAdapterDbObserver * observer)
    : mpObserver(observer)
{
}

CAmRoutingAdapterALSAdb::~CAmRoutingAdapterALSAdb()
{
}

ra_domainInfo_s * CAmRoutingAdapterALSAdb::createDomain(ra_domainInfo_s & domain)
{
    mDomains.push_back(domain);
    return &mDomains.back();
}

ra_domainInfo_s * CAmRoutingAdapterALSAdb::findDomain(const am_domainID_t id)
{
    vector<ra_domainInfo_s>::iterator it =
            find_if(mDomains.begin(), mDomains.end(), ra_domainInfo_s(id));
    return ((it != mDomains.end()) ? it.base() : NULL);
}

ra_domainInfo_s * CAmRoutingAdapterALSAdb::findDomain(const string &domNam)
{
    vector<ra_domainInfo_s>::iterator it =
            find_if(mDomains.begin(), mDomains.end(), ra_domainInfo_s(domNam));
    return ((it != mDomains.end()) ? it.base() : NULL);
}

ra_domainInfo_s * CAmRoutingAdapterALSAdb::findDomain(ra_sourceInfo_s & source, ra_sinkInfo_s & sink)
{
    for (ra_domainInfo_s & domain : mDomains)
    {
        /* find source id */
        vector<ra_sourceInfo_s>::iterator itSource =
                find_if(domain.lSourceInfo.begin(), domain.lSourceInfo.end(), source);
        if (itSource == domain.lSourceInfo.end())
        {
            continue; /* in case the sink id is not in domain search in other domain */
        }

        /* find sink id and check for causality */
        vector<ra_sinkInfo_s>::iterator itSink =
                find_if(domain.lSinkInfo.begin(),domain.lSinkInfo.end(), sink);
        if (itSink != domain.lSinkInfo.end())
        {
            source = *itSource;
            sink = *itSink;
            return &domain;
        }
    }
    return NULL;
}

ra_domainInfo_s * CAmRoutingAdapterALSAdb::findDomainByConnection(const am_connectionID_t id)
{
    /* find the route */
    map<am_connectionID_t, ra_route_s>::iterator hit = mMapConnectionIDRoute.find(id);
    if (hit == mMapConnectionIDRoute.end())
    {
        logAmRaError("CRaALSAdb::findDomainByConnection unknown", id);
        return NULL;
    }
    return findDomain(hit->second.domainID);
}

ra_proxyInfo_s * CAmRoutingAdapterALSAdb::findProxyInDomain(
        ra_domainInfo_s * pDomain, const am_sourceID_t sourceID, const am_sinkID_t sinkID)
{
    std::vector<ra_proxyInfo_s>::iterator itr =
            find_if(pDomain->lProxyInfo.begin(), pDomain->lProxyInfo.end(), ra_proxyInfo_s(sourceID, sinkID));
    if (itr != pDomain->lProxyInfo.end())
    {
        return &(*itr);
    }
    // This is a valid behavior. NO error log here!!!
    return NULL;
}

ra_proxyInfo_s * CAmRoutingAdapterALSAdb::findProxyInDomain(
        const am_domainID_t domainID, const std::string & sourceName, const std::string & sinkName)
{
    ra_domainInfo_s * pDomain = findDomain(domainID);
    std::vector<ra_proxyInfo_s>::iterator itr =
            find_if(pDomain->lProxyInfo.begin(), pDomain->lProxyInfo.end(), ra_proxyInfo_s(sourceName, sinkName));
    if (itr != pDomain->lProxyInfo.end())
    {
        return &(*itr);
    }
    // This is a valid behavior. NO error log here!!!
    return NULL;
}


am_connectionID_t CAmRoutingAdapterALSAdb::findConnectionFromSource(const am_domainID_t domainId,
                                                    const am_sourceID_t sourceID)
{
    for (auto && it : mMapConnectionIDRoute)
    {
        if (it.second.domainID == domainId && it.second.sourceID == sourceID)
        {
            return it.first;
        }
    }
    return 0;
}

ra_sinkInfo_s * CAmRoutingAdapterALSAdb::findSink(const am_sinkID_t id)
{
    for (ra_domainInfo_s & domain : mDomains)
    {
        /* find sink id */
        vector<ra_sinkInfo_s>::iterator itr =
                find_if(domain.lSinkInfo.begin(), domain.lSinkInfo.end(), ra_sinkInfo_s(id));
        if (itr != domain.lSinkInfo.end())
        {
            return &(*itr);
        }
    }

    return NULL;
}

ra_sourceInfo_s * CAmRoutingAdapterALSAdb::findSource(const am_sourceID_t id)
{
    for (ra_domainInfo_s & domain : mDomains)
    {
        /* find source id */
        vector<ra_sourceInfo_s>::iterator itr =
                find_if(domain.lSourceInfo.begin(), domain.lSourceInfo.end(), ra_sourceInfo_s(id));
        if (itr != domain.lSourceInfo.end())
        {
            return &(*itr);
        }
    }

    return NULL;
}

void CAmRoutingAdapterALSAdb::cleanup()
{
    for (ra_domainInfo_s & domain : mDomains)
    {
        domain.lSourceInfo.clear();
        domain.lSinkInfo.clear();
        domain.lProxyInfo.clear();
        domain.lGatewayInfo.clear();
    }

    mMapAsyncOperations.clear();
    mMapConnectionIDRoute.clear();
}

void CAmRoutingAdapterALSAdb::registerDomains()
{
    for (ra_domainInfo_s & domain : mDomains)
    {
        mpObserver->registerDomain(domain.domain);

        /* Sources registration */
        for (ra_sourceInfo_s & source : domain.lSourceInfo)
        {
            mpObserver->registerSource(source, domain.domain.domainID);
        }

        /* Sink registration */
        for (ra_sinkInfo_s & sink : domain.lSinkInfo)
        {
            mpObserver->registerSink(sink, domain.domain.domainID);
        }

        /* Update database for proxy */
        updateProxys(domain);

        domain.domain.complete = true;

    }

    /* Gateway registration needs to be done at the end
     * because only then all sources and sinks are registered */
    for (ra_domainInfo_s & domain : mDomains)
    {
        updateGateways(domain);
        for (ra_gatewayInfo_s & gateway : domain.lGatewayInfo)
        {
            mpObserver->registerGateway(gateway, domain.domain.domainID);
        }
    }

    /* Only when also all Gateways have been registered we send the notification
     * that the Domains completed their registration
     */
    for (ra_domainInfo_s & domain : mDomains)
    {
        if (domain.domain.complete == true)
        {
            mpObserver->hookDomainRegistrationComplete(domain.domain.domainID);
        }
    }
}

void CAmRoutingAdapterALSAdb::deregisterDomains()
{
    for (ra_domainInfo_s & domain : mDomains)
    {
        mpObserver->deregisterDomain(domain.domain.domainID);
    }
}

void CAmRoutingAdapterALSAdb::registerVolumeOp(const am_Handle_s handle, class CAmRoutingAdapterALSAVolume* volume)
{
    /* this construct creates a new entry in map or reuses an existing */
    vector<class CAmRoutingAdapterALSAVolume*> & volumes = mMapAsyncOperations[static_cast<uint16_t>(handle.handle)];
    volumes.push_back(volume);
}

vector<class CAmRoutingAdapterALSAVolume*> & CAmRoutingAdapterALSAdb::getVolumeOpList(const am_Handle_s handle)
{
    return mMapAsyncOperations[static_cast<uint16_t>(handle.handle)];
}

void CAmRoutingAdapterALSAdb::getVolumeOpLists(vector<class CAmRoutingAdapterALSAVolume*> & volumes)
{
    for (pair<const uint16_t, vector<class CAmRoutingAdapterALSAVolume*> > & pair : mMapAsyncOperations)
    {
        volumes.insert(volumes.end(), pair.second.begin(), pair.second.end());
    }
}

void CAmRoutingAdapterALSAdb::cleanVolumeOpList(const am_Handle_s handle)
{
    mMapAsyncOperations.erase(static_cast<uint16_t>(handle.handle));
}

void CAmRoutingAdapterALSAdb::registerConnection(const am_connectionID_t connectionId, const am_RoutingElement_s & elements, class IAmRoutingAdapterALSAProxy * proxy)
{
    mMapConnectionIDRoute[connectionId] = ra_route_s(elements, proxy);
}

class IAmRoutingAdapterALSAProxy * CAmRoutingAdapterALSAdb::getProxyOfConnection(const am_connectionID_t connectionId)
{
    try
    {
        ra_route_s route = mMapConnectionIDRoute.at(connectionId);
        return route.proxy;
    }
    catch(const std::out_of_range &e)
    {
        logError("CAmRoutingAdapterALSAdb::getProxyOfConnection - out_of_range", e.what());
    }
    return NULL;
}

void CAmRoutingAdapterALSAdb::getProxyLists(vector<class IAmRoutingAdapterALSAProxy*> & proxies)
{
    for (pair<const uint16_t, ra_route_s> & pair : mMapConnectionIDRoute)
    {
        proxies.push_back(pair.second.proxy);
    }
}

void CAmRoutingAdapterALSAdb::deregisterConnection(const am_connectionID_t connectionId)
{
    mMapConnectionIDRoute.erase(connectionId);
}


uint32_t CAmRoutingAdapterALSAdb::maxValue(vector<uint32_t>::const_iterator itr, vector<uint32_t>::const_iterator end)
{
    uint32_t maxVal = 0;
    for (; itr != end; ++itr)
    {
        maxVal = max(*itr, maxVal);
    }
    return maxVal;
}

void CAmRoutingAdapterALSAdb::updateProxys(ra_domainInfo_s & data)
{
    /* update all known source and sink id' */
    for (ra_proxyInfo_s & proxy : data.lProxyInfo)
    {
        vector<ra_sourceInfo_s>::iterator itSource =
                find_if(data.lSourceInfo.begin(), data.lSourceInfo.end(), ra_sourceInfo_s(proxy.srcNam));

        vector<ra_sinkInfo_s>::iterator itSink =
                find_if(data.lSinkInfo.begin(), data.lSinkInfo.end(), ra_sinkInfo_s(proxy.sinkNam));

        proxy.route.sourceID = itSource->amInfo.sourceID;
        proxy.route.sinkID   = itSink->amInfo.sinkID;

        /* check if the expected matrix size is equal to the configured */
        size_t expectedMatrixSize = itSource->amInfo.listConnectionFormats.size() * itSink->amInfo.listConnectionFormats.size();
        if (expectedMatrixSize != proxy.convertionMatrix.size())
        {
            logAmRaInfo("CRaALSAdb::updateProxys", proxy.srcNam, "to", proxy.sinkNam,
                    ": Matrix vs. Source and Sink formats incompatible. Expected:",
                    expectedMatrixSize, "!=", proxy.convertionMatrix.size());
            continue;
        }
        /* check if the expected matrix size is equal to the configured */
        uint32_t maxVal = maxValue(proxy.convertionMatrix.begin(), proxy.convertionMatrix.end());
        if ((maxVal != proxy.listRates.size()) || (maxVal != proxy.listPcmFormats.size()))
        {
            logAmRaInfo("CRaALSAdb::updateProxys", proxy.srcNam, "to", proxy.sinkNam,
                    ": One or more Matrix elements are out of range.", "Expected:", maxVal, "!= lstRates",
                    proxy.listRates.size(), "and != lstPcmFmts", proxy.listPcmFormats.size());
        }
    }
}

void CAmRoutingAdapterALSAdb::updateGateways(ra_domainInfo_s & data)
{
    /* update all known source, sink and domain id's */
    for (ra_gatewayInfo_s & gateway : data.lGatewayInfo)
    {
        /* update source information */
        vector<ra_domainInfo_s>::iterator itDom =
                find_if(mDomains.begin(), mDomains.end(), ra_domainInfo_s(gateway.srcDomNam));
        if (itDom != mDomains.end())
        {
            gateway.amInfo.domainSourceID = itDom->domain.domainID;

            vector<ra_sourceInfo_s>::iterator itSrc =
                    find_if(itDom->lSourceInfo.begin(), itDom->lSourceInfo.end(), ra_sourceInfo_s(gateway.srcNam));
            if (itSrc != itDom->lSourceInfo.end())
            {
                gateway.amInfo.sourceID = itSrc->amInfo.sourceID;
                if (gateway.amInfo.listSourceFormats.size() == 0)
                {
                    gateway.amInfo.listSourceFormats = itSrc->amInfo.listConnectionFormats;
                }
            }
        }

        /* update sink information */
        itDom = find_if(mDomains.begin(), mDomains.end(), ra_domainInfo_s(gateway.sinkDomNam));
        if (itDom != mDomains.end())
        {
            gateway.amInfo.domainSinkID = itDom->domain.domainID;

            vector<ra_sinkInfo_s>::iterator itSink =
                    find_if(itDom->lSinkInfo.begin(), itDom->lSinkInfo.end(), ra_sinkInfo_s(gateway.sinkNam));
            if (itSink != itDom->lSinkInfo.end())
            {
                gateway.amInfo.sinkID = itSink->amInfo.sinkID;
                if (gateway.amInfo.listSinkFormats.size() == 0)
                {
                    gateway.amInfo.listSinkFormats = itSink->amInfo.listConnectionFormats;
                }
            }
        }
    }
}
