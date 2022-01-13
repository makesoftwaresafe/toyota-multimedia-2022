/*******************************************************************************
 *  \copyright (c) 2016 Advanced Driver Information Technology.
 *                   ADIT is a joint venture company of
 *   Robert Bosch GmbH/Robert Bosch Car Multimedia GmbH and DENSO Corporation
 *
 *  \author: Jens Lorenz, jlorenz@de.adit-jv.com 2013-2016
 *           Mattia Guerra, mguerra@de.adit-jv.com 2016
 *           Jayanth MC, Jayanth.mc@in.bosch.com 2013,2014
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


#ifndef ROUTINGADAPTERALSADB_H_
#define ROUTINGADAPTERALSADB_H_

#include <string>
#include <vector>
#include <map>
#include <tuple>
#include <sstream>
#include <algorithm>
#include "audiomanagertypes.h"
#include "CAmRoutingAdapterALSAProxyInfo.h"

namespace am
{

/**
 *  This represents the ALSA
 */
enum am_cardState_e
{
    DPS_VIRTUAL = 0,    //* Source or Sink is an application
    DPS_REAL = 1,        //* Source or Sink represents a real ALSA sound card
    DPS_MAX
};

/**
 *  Interpreter maps of audio manager
 */
const std::map<std::string, am_cardState_e> am_dpsMap = {
    { "0", DPS_VIRTUAL },
    { "1", DPS_REAL},
};

const std::map<std::string, am_DomainState_e> am_dsMap = {
    { "1", DS_CONTROLLED },
    { "2", DS_INDEPENDENT_STARTUP },
    { "3", DS_INDEPENDENT_RUNDOWN }
};

const std::map<std::string, am_SourceState_e> am_ssMap = {
    { "1", SS_ON },
    { "2", SS_OFF},
    { "3", SS_PAUSED},
};

const std::map<std::string, am_Availability_e> am_aMap = {
    { "1", A_AVAILABLE },
    { "2", A_UNAVAILABLE},
};

const std::map<std::string, am_InterruptState_e> am_isMap = {
    { "1", IS_OFF },
    { "2", IS_INTERRUPTED},
};

const std::map<std::string, am_MuteState_e> am_msMap = {
    { "1", MS_MUTED },
    { "2", MS_UNMUTED},
};

enum SoundPropertyVisibility
{
    SPV_NONE = 0, /* reads as standard, that is, only listSoundProperty will be filled */
    SPV_MAIN, /* only listMainSoundProperty will be filled */
    SPV_BOTH /* Both list will be filled */
};

const std::map<std::string, SoundPropertyVisibility> am_spvMap = {
    { "0", SPV_NONE },
    { "1", SPV_MAIN },
    { "2", SPV_BOTH },
};

/**
 *  Source's single element for mixer configuration
 */
struct am_PropertySpecification_s
{
public:
    std::string mixerName;
    std::string value;

public:
    am_PropertySpecification_s() {};
    am_PropertySpecification_s(const std::string& str, const std::string val) : mixerName(str), value(val) {};
};

/**
 * am_SoundPropertyMapping_s inherits from am_SoundProperty_s and defines
 * comparison operator for being used by std::map
 */
struct am_SoundPropertyMapping_s:am_SoundProperty_s
{
    bool operator<(const am_SoundPropertyMapping_s& x) const
    {
        return std::tie(type, value) < std::tie(x.type, x.value);
    }
};

/**
 * This operator allows to easily populate the std::map second. Basically it consumes
 * specification given in the form mixerName_1, value_1; mixerName_2, value_2; ... ; mixerName_N, value_N;
 * Every pair of elements separated by ',' is managed by this overloaded operator when parsing
 */
inline std::istringstream& operator>>(std::istringstream &is, am::am_PropertySpecification_s &ref)
{
    std::string str = is.str();
    size_t hit = str.find(",");
    if (hit != std::string::npos)
    {
        ref.mixerName = str.substr(0, hit);
        const std::string whitespace = " \t";
        auto begin = ref.mixerName.find_first_not_of(whitespace);
        if (begin != std::string::npos)
        {
            const auto end = ref.mixerName.find_last_not_of(whitespace);
            ref.mixerName = ref.mixerName.substr(begin, end - begin + 1);
        }

        ref.value = str.substr(hit + 1, str.size()).c_str();
        begin = ref.value.find_first_not_of(whitespace);
        if (begin != std::string::npos)
        {
            const auto end = ref.value.find_last_not_of(whitespace);
            ref.value = ref.value.substr(begin, end - begin + 1);
        }
    }
    return is;
}

/**
 * This operator gets string even with blank spaces within
 */
inline std::istringstream& operator>>(std::istringstream &is, std::string &destString)
{
    destString = is.str();
    return is;
}

/**
 * This operator allows to print out a pair of type am_PropertySpecification_s
 */
inline std::ostream& operator<<(std::ostream &os, const am::am_PropertySpecification_s &ref)
{
    return os << ref.mixerName << "=" << ref.value;
}

/**
 *  Source specific info
 */
struct ra_sourceInfo_s
{
public:
    am_Source_s amInfo;
    am_cardState_e devTyp;
    std::string srcClsNam;
    std::string domNam;
    std::string pcmNam;
    std::string volNam;

    /**
     * std::map for resource of type Source associating (type, value) of tPROPERTYSPEC XML Child Node against mixer_i, value_i collection
     */
    std::map <am_SoundPropertyMapping_s, std::vector<am_PropertySpecification_s> > mapSoundPropertiesToMixer;

public:
    ra_sourceInfo_s() {};
    ra_sourceInfo_s(const am_sourceID_t id)
    {
        amInfo.sourceID = id;
    };
    ra_sourceInfo_s(const std::string name)
    {
        amInfo.name = name;
    };
    bool operator()(const ra_sourceInfo_s & elem) const
    {
        if (amInfo.name.length() != 0)
        {
            return (amInfo.name == elem.amInfo.name);
        }
        return (amInfo.sourceID == elem.amInfo.sourceID);
    };
};

/**
 *  Sink specific info
 */
struct ra_sinkInfo_s
{
public:
    am_Sink_s amInfo;
    am_cardState_e devTyp;
    std::string sinkClsNam;
    std::string domNam;
    std::string pcmNam;
    std::string volNam;

    /**
     * std::map for resource of type Sink associating (type, value) of tPROPERTYSPEC XML Child Node against mixer_i, value_i collection
     */
    std::map <am_SoundPropertyMapping_s, std::vector<am_PropertySpecification_s> > mapSoundPropertiesToMixer;
public:
    ra_sinkInfo_s() {};
    ra_sinkInfo_s(const am_sinkID_t id)
    {
        amInfo.sinkID = id;
    };
    ra_sinkInfo_s(const std::string name)
    {
        amInfo.name = name;
    };
    bool operator()(const ra_sinkInfo_s & elem) const
    {
        if (amInfo.name.length() != 0)
        {
            return (amInfo.name == elem.amInfo.name);
        }
        return (amInfo.sinkID == elem.amInfo.sinkID);
    };
};

/**
 *  Gateway specific info
 */
struct ra_gatewayInfo_s
{
public:
    am_Gateway_s amInfo;
    std::string ctrlDomNam;
    std::string srcDomNam;
    std::string sinkDomNam;
    std::string srcNam;
    std::string sinkNam;
    bool used_for_conn;

public:
    ra_gatewayInfo_s() {};
    ra_gatewayInfo_s(const ra_sourceInfo_s & source)
    {
        ctrlDomNam = source.domNam;
        srcDomNam  = source.domNam;
        srcNam     = source.amInfo.name;
    };
    ra_gatewayInfo_s(const ra_sinkInfo_s & sink)
    {
        ctrlDomNam = sink.domNam;
        sinkDomNam = sink.domNam;
        sinkNam    = sink.amInfo.name;
    };
};

/**
 * Struct providing details on USB management in Audio Manager acception
 */
struct ra_USBInfo_s
{
    std::string domNam;
    std::map<std::string, std::vector<std::string> > mapCardToSourceName;
    std::map<std::string, std::vector<std::string> > mapCardToSinkName;
    std::vector<ra_sourceInfo_s> lSourceInfo;
    std::vector<ra_sinkInfo_s> lSinkInfo;
};

/**
 * Struct providing details on Domain in Audio Manager acception
 */
struct ra_domainInfo_s
{
public:
    am_Domain_s domain;
    std::vector<ra_sourceInfo_s> lSourceInfo;
    std::vector<ra_sinkInfo_s> lSinkInfo;
    std::vector<ra_gatewayInfo_s> lGatewayInfo;
    std::vector<ra_proxyInfo_s> lProxyInfo;
    std::string pxyNam;

public:
    ra_domainInfo_s() {};
    ra_domainInfo_s(const am_domainID_t id)
    {
        domain.domainID = id;
    };
    ra_domainInfo_s(const std::string name)
    {
        domain.name = name;
    };
    bool operator()(const ra_domainInfo_s & elem) const
    {
        if (domain.name.length() != 0)
        {
            return (domain.name == elem.domain.name);
        }
        return (domain.domainID == elem.domain.domainID);
    };
};


/*
 * Complete system Information in Audio domain view
 */
class IAmRoutingAdapterDbObserver
{
public:
    virtual ~IAmRoutingAdapterDbObserver() {};
    /**
     * Registers Domain to Audio Manager
     */
    virtual void registerDomain(am_Domain_s & domain) = 0;
    /**
     * Registers Source to Audio Manager
     */
    virtual void registerSource(ra_sourceInfo_s & info, am_domainID_t domainID) = 0;
    /**
     * Registers Sink to Audio Manager
     */
    virtual void registerSink(ra_sinkInfo_s & info, am_domainID_t domainID) = 0;
    /**
     * Registers Gateway to Audio Manager
     */
    virtual void registerGateway(ra_gatewayInfo_s & info, am_domainID_t domainID) = 0;
    /**
     * Sends to AudioManager when a Domain Registration is finished
     */
    virtual void hookDomainRegistrationComplete(am_domainID_t domainID) = 0;

    /**
     * Deregisters Domain from Audio Manager
     */
    virtual void deregisterDomain(const am_domainID_t &domainID) = 0;
    /**
     * Deregisters Source from Audio Manager
     */
    virtual void deregisterSource(const am_sourceID_t &sourceID) = 0;
    /**
     * Deregisters Sink from Audio Manager
     */
    virtual void deregisterSink(const am_sinkID_t &sinkID) = 0;
    /**
     * Deregisters Gateway from Audio Manager
     */
    virtual void deregisterGateway(const am_gatewayID_t &gatewayID) = 0;
};

class CAmRoutingAdapterALSAdb
{
public:
    CAmRoutingAdapterALSAdb(IAmRoutingAdapterDbObserver * observer);
    ~CAmRoutingAdapterALSAdb();

    ra_domainInfo_s * createDomain(ra_domainInfo_s & domain);
    ra_domainInfo_s * findDomain(const am_domainID_t id);
    ra_domainInfo_s * findDomain(const std::string &domNam);
    ra_domainInfo_s * findDomain(ra_sourceInfo_s & source, ra_sinkInfo_s & sink);
    ra_domainInfo_s * findDomainByConnection(const am_connectionID_t id);
    ra_proxyInfo_s  * findProxyInDomain(ra_domainInfo_s *pDomain,
                                        const am_sourceID_t sourceID, const am_sinkID_t sinkID = 0);
    ra_proxyInfo_s  * findProxyInDomain(const am_domainID_t domainID,
                                        const std::string & sourceName, const std::string & sinkName = "");
    am_connectionID_t findConnectionFromSource(const am_domainID_t domainId,
                                                    const am_sourceID_t sourceID);
    ra_sinkInfo_s   * findSink(const am_sinkID_t id);
    ra_sourceInfo_s * findSource(const am_sourceID_t id);

    void setUSBInfo(ra_USBInfo_s &usbInfo)
    {
        mUSB = usbInfo;
    }

    ra_USBInfo_s *getUSBInfo()
    {
        return &mUSB;
    }

    void getElementList(ra_domainInfo_s *domain, std::vector<ra_sourceInfo_s> **mpList)
    {
        *mpList = &domain->lSourceInfo;
    }

    void getElementList(ra_domainInfo_s *domain, std::vector<ra_sinkInfo_s> **mpList)
    {
        *mpList = &domain->lSinkInfo;
    }

    void getElementList(ra_domainInfo_s *domain, std::vector<ra_gatewayInfo_s> **mpList)
    {
        *mpList = &domain->lGatewayInfo;
    }

    template <typename T>
    T* findElement(const am_domainID_t id, const std::string name)
    {
        ra_domainInfo_s *domain = findDomain(id);
        if (domain == NULL)
        {
            return NULL;
        }

        std::vector<T> *mpList = NULL;
        getElementList(domain, &mpList);
        auto it = std::find_if(mpList->begin(), mpList->end(),
            [=] (const T &m) -> bool {return (m.amInfo.name.compare(name) == 0) ? true : false;}
        );
        return (it == mpList->end()) ? NULL : &(*it);
    }

    void registerDomains();
    void deregisterDomains();
    void cleanup();

    void registerVolumeOp(const am_Handle_s handle, class CAmRoutingAdapterALSAVolume* volume);
    std::vector<class CAmRoutingAdapterALSAVolume*> & getVolumeOpList(const am_Handle_s handle);
    void getVolumeOpLists(std::vector<class CAmRoutingAdapterALSAVolume*> & volumes);
    void cleanVolumeOpList(const am_Handle_s handle);

    void registerConnection(const am_connectionID_t connectionId,
                            const am_RoutingElement_s & elements, class IAmRoutingAdapterALSAProxy * proxy);
    class IAmRoutingAdapterALSAProxy * getProxyOfConnection(const am_connectionID_t connectionId);
    void getProxyLists(std::vector<class IAmRoutingAdapterALSAProxy*> & proxies);
    void deregisterConnection(const am_connectionID_t connectionId);

    void updateProxys(ra_domainInfo_s & data);
    void updateGateways(ra_domainInfo_s & data);

private:
    uint32_t maxValue(std::vector<uint32_t>::const_iterator itr, std::vector<uint32_t>::const_iterator end);

private:
    struct ra_route_s: am_RoutingElement_s
    {
    public:
        class IAmRoutingAdapterALSAProxy * proxy;
    public:
        ra_route_s() : proxy(NULL) {};
        ra_route_s(const am_RoutingElement_s & e, class IAmRoutingAdapterALSAProxy * p)
            : am_RoutingElement_s(e), proxy(p) {};
    };
    std::map<am_connectionID_t, ra_route_s>                 mMapConnectionIDRoute;
    std::map<uint16_t, std::vector<class CAmRoutingAdapterALSAVolume*> >  mMapAsyncOperations;
    std::vector<ra_domainInfo_s>                            mDomains;
    IAmRoutingAdapterDbObserver                                           *mpObserver;
    ra_USBInfo_s                                            mUSB;
};

}/* namespace am */

#endif /* ROUTINGADAPTERALSADB_H_ */
