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


#include <cstring>
#include <stdlib.h> // for getenv()
#include <fstream> //for file operations
#include "CAmRoutingAdapterALSAParser.h"
#include "CAmDLTLogging.h"

DLT_DECLARE_CONTEXT (ALSARoutingAdapterParser)

using namespace std;
using namespace am;

#ifndef ROUTING_ADAPTER_ALSA_DEFAULT_CONF_ROOT
#define ROUTING_ADAPTER_ALSA_DEFAULT_CONF_ROOT "/etc/audiomanager/routing"
#endif

#define ROUTING_ADAPTER_ALSA_NODE_NAME "amra_alsa"

/*
 * INIT_TOUT is the default number of milliseconds assigned to attribute msInitTimeout when parsing Proxy elements.
 */
#define INIT_TOUT 1000

CAmRoutingAdapterALSAParser::CAmRoutingAdapterALSAParser(CAmRoutingAdapterALSAdb & db, string & busname) :
        mDataBase(db), mpDomainRef(), mBusname(busname)
{
}

void CAmRoutingAdapterALSAParser::parseDomainData(ra_domainInfo_s & domainInfo, CAmRoutingAdapterKVPConverter::KVPList & kvpList)
{
    CAmRoutingAdapterKVPConverter converter(kvpList, logAmRaDebug, logAmRaError);
    domainInfo.domain.name = converter.kvpQueryValue("domNam", static_cast<string>(""));
    domainInfo.domain.domainID = converter.kvpQueryValue("domID", 0);
    domainInfo.domain.busname = converter.kvpQueryValue("busNam", mBusname);
    domainInfo.domain.nodename = converter.kvpQueryValue("nodNam", static_cast<string>(""));
    domainInfo.domain.early = converter.kvpQueryValue("early", false);
    domainInfo.domain.state = converter.kvpQueryValue("state", DS_CONTROLLED, am_dsMap);
    domainInfo.pxyNam = converter.kvpQueryValue("pxyNam", static_cast<string>(""));
}

void CAmRoutingAdapterALSAParser::parseSourceData(ra_sourceInfo_s & info, CAmRoutingAdapterKVPConverter::KVPList & kvpList)
{
    string domName(mpDomainRef ? mpDomainRef->domain.name : "");
    vector<am_CustomConnectionFormat_t> DEF_VEC_CONFMT = { CF_GENIVI_STEREO };

    CAmRoutingAdapterKVPConverter converter(kvpList, logAmRaDebug, logAmRaError);
    info.domNam = converter.kvpQueryValue("domNam", domName);
    info.devTyp = converter.kvpQueryValue("devTyp", DPS_REAL, am_dpsMap);
    info.srcClsNam = converter.kvpQueryValue("srcClsNam", static_cast<string>(""));
    info.pcmNam = converter.kvpQueryValue("pcmNam", static_cast<string>("default"));
    info.volNam = converter.kvpQueryValue("volNam", static_cast<string>(""));

    am_Source_s & src = info.amInfo;
    src.sourceID = converter.kvpQueryValue("srcID", 0);
    src.domainID = converter.kvpQueryValue("domID", 0);
    src.name = converter.kvpQueryValue("srcNam", static_cast<string>(""));
    src.volume = converter.kvpQueryValue("volume", AM_MUTE);
    src.visible = converter.kvpQueryValue("visible", true);
    src.sourceClassID = converter.kvpQueryValue("srcClsID", 1);
    src.sourceState = converter.kvpQueryValue("srcStat", src.visible ? SS_OFF : SS_UNKNNOWN, am_ssMap);
    src.available.availability = converter.kvpQueryValue("availability", A_AVAILABLE, am_aMap);
    src.available.availabilityReason = converter.kvpQueryValue("availabilityReason", AR_UNKNOWN);
    src.interruptState = converter.kvpQueryValue("interruptState", IS_UNKNOWN, am_isMap);
    src.listConnectionFormats = converter.kvpQueryValue("lstConFrmt", DEF_VEC_CONFMT);
}

void CAmRoutingAdapterALSAParser::parseSinkData(ra_sinkInfo_s & info, CAmRoutingAdapterKVPConverter::KVPList & kvpList)
{
    string domName(mpDomainRef ? mpDomainRef->domain.name : "");
    vector<am_CustomConnectionFormat_t> DEF_VEC_CONFMT = { CF_GENIVI_STEREO };

    CAmRoutingAdapterKVPConverter converter(kvpList, logAmRaDebug, logAmRaError);
    info.domNam = converter.kvpQueryValue("domNam", domName);
    info.devTyp = converter.kvpQueryValue("devTyp", DPS_REAL, am_dpsMap);
    info.sinkClsNam = converter.kvpQueryValue("sinkClsNam", static_cast<string>(""));
    info.pcmNam = converter.kvpQueryValue("pcmNam", static_cast<string>("default"));
    info.volNam = converter.kvpQueryValue("volNam", static_cast<string>(""));

    am_Sink_s & sink = info.amInfo;
    sink.sinkID = converter.kvpQueryValue("sinkID", 0);
    sink.domainID = converter.kvpQueryValue("domID", 0);
    sink.name = converter.kvpQueryValue("sinkNam", static_cast<string>(""));
    sink.sinkClassID = converter.kvpQueryValue("sinkClsID", 1);
    sink.volume = converter.kvpQueryValue("volume", AM_MUTE);
    sink.visible = converter.kvpQueryValue("visible", true);
    sink.available.availability = converter.kvpQueryValue("availability", A_AVAILABLE, am_aMap);
    sink.available.availabilityReason = converter.kvpQueryValue("availabilityReason", AR_UNKNOWN);
    sink.muteState  = converter.kvpQueryValue("muteState", MS_MUTED, am_msMap);
    sink.mainVolume = converter.kvpQueryValue("mainVolume", AM_MUTE);
    sink.listConnectionFormats = converter.kvpQueryValue("lstConFrmt", DEF_VEC_CONFMT);
}

void CAmRoutingAdapterALSAParser::parseGatewayData(ra_gatewayInfo_s & info, CAmRoutingAdapterKVPConverter::KVPList & kvpList)
{
    vector<am_CustomConnectionFormat_t> DEF_VEC_CONFMT = { CF_GENIVI_STEREO };
    vector<bool> DEF_VEC_MATRIX = { false };

    CAmRoutingAdapterKVPConverter converter(kvpList, logAmRaDebug, logAmRaError);
    info.ctrlDomNam = converter.kvpQueryValue("ctrlDomNam", info.ctrlDomNam);
    info.srcNam = converter.kvpQueryValue("srcNam", info.srcNam);
    info.sinkNam = converter.kvpQueryValue("sinkNam", info.sinkNam);
    info.srcDomNam = converter.kvpQueryValue("srcDomNam", info.srcDomNam);
    info.sinkDomNam = converter.kvpQueryValue("sinkDomNam", info.sinkDomNam);

    am_Gateway_s & gw = info.amInfo;
    gw.name = converter.kvpQueryValue("gatewayNam", static_cast<string>(""));
    gw.gatewayID = converter.kvpQueryValue("gatewayID", 0);
    gw.listSourceFormats = converter.kvpQueryValue("lstSrcFrmt", DEF_VEC_CONFMT);
    gw.listSinkFormats = converter.kvpQueryValue("lstSinkFrmt", DEF_VEC_CONFMT);
    gw.convertionMatrix = converter.kvpQueryValue("conversionMatrix", DEF_VEC_MATRIX);
}

void CAmRoutingAdapterALSAParser::parseProxyData(ra_proxyInfo_s & info, CAmRoutingAdapterKVPConverter::KVPList & kvpList)
{
    vector<uint32_t> DEF_VEC_MATRIX = { 1 };
    vector<uint32_t> DEF_VEC_CHANNELS = { 2 };
    vector<uint32_t> DEF_VEC_FORMATS = { SND_PCM_FORMAT_S16_LE };
    vector<uint32_t> DEF_VEC_RATES = { 48000 };

    CAmRoutingAdapterKVPConverter converter(kvpList, logAmRaDebug, logAmRaError);
    info.domNam = converter.kvpQueryValue("domNam", info.domNam);
    info.srcNam = converter.kvpQueryValue("srcNam", static_cast<string>(""));
    info.sinkNam = converter.kvpQueryValue("sinkNam", static_cast<string>(""));
    info.pxyNam = converter.kvpQueryValue("pxyNam", mpDomainRef->pxyNam);
    info.convertionMatrix = converter.kvpQueryValue("dftConv", DEF_VEC_MATRIX);
    info.listChannels = converter.kvpQueryValue("lstChannels", DEF_VEC_CHANNELS);
    info.listPcmFormats = converter.kvpQueryValue("lstPcmFmts", DEF_VEC_FORMATS);
    info.listRates = converter.kvpQueryValue("lstRates", DEF_VEC_RATES);

    ra_Proxy_s & alsa = info.alsa;
    alsa.pcmSrc = converter.kvpQueryValue("pcmSrc", static_cast<string>(""));
    alsa.pcmSink = converter.kvpQueryValue("pcmSink", static_cast<string>(""));
    alsa.duplex = converter.kvpQueryValue("duplex", false);
    alsa.msBuffersize = converter.kvpQueryValue("msBuffersize", 0);
    alsa.msPrefill = converter.kvpQueryValue("msPrefill", alsa.msBuffersize);
    alsa.msInitTimeout = converter.kvpQueryValue("msInitTimeout", INIT_TOUT);
    alsa.numSoftRampPeriods = converter.kvpQueryValue("softRampPeriod", 0);
    alsa.cpuScheduler.policy = converter.kvpQueryValue("CPUSchedulingPolicy", SCHED_OTHER);
    alsa.cpuScheduler.priority = converter.kvpQueryValue("CPUSchedulingPriority", 0);
}

void CAmRoutingAdapterALSAParser::parseUSBData(ra_USBInfo_s & info, CAmRoutingAdapterKVPConverter::KVPList & kvpList)
{
    CAmRoutingAdapterKVPConverter converter(kvpList, logAmRaDebug, logAmRaError);
    info.domNam = converter.kvpQueryValue("domNam", info.domNam);
}

template <typename S>
void CAmRoutingAdapterALSAParser::parseSoundPropertyData(S & info, CAmRoutingAdapterKVPConverter::KVPList & kvpList)
{
    CAmRoutingAdapterKVPConverter converter(kvpList, logAmRaDebug, logAmRaError);
    mLastSeenType = converter.kvpQueryValue("type", info.amInfo.listSoundProperties.size());
    mSoundPropertyVisibility = converter.kvpQueryValue("visibility", SPV_NONE, am_spvMap);
}

template <typename S>
void CAmRoutingAdapterALSAParser::parseSoundPropertySpecification(S & info, CAmRoutingAdapterKVPConverter::KVPList & kvpList)
{
    CAmRoutingAdapterKVPConverter converter(kvpList, logAmRaDebug, logAmRaError);


    am_SoundPropertyMapping_s property;
    property.type = mLastSeenType;
    property.value = converter.kvpQueryValue("value", 0);
    if (mSoundPropertyVisibility != SoundPropertyVisibility::SPV_MAIN)
    {
        info.amInfo.listSoundProperties.push_back(property);

        /* Default value provided to kvpQueryValue */
        vector<am_PropertySpecification_s> mixersAndValuesDefault;
        mixersAndValuesDefault.push_back(am_PropertySpecification_s("", ""));

        /* Populating the map with  am_SoundPropertyMapping_s as first,
         * second is an array of coupled mixer-value. Specifying a proper
         * >> istringstream operator to fill such struct, see CAmRoutingAdapterALSAdb.h*/
        info.mapSoundPropertiesToMixer[property] = converter.kvpQueryValue("namsVals", mixersAndValuesDefault);
    }
    /*
     * Prepare the listMainSoundProperty if it was specified
     */
    if (mSoundPropertyVisibility != SoundPropertyVisibility::SPV_NONE)
    {
        am_MainSoundProperty_s mainProperty;
        mainProperty.type = property.type;
        mainProperty.value = property.value;
        info.amInfo.listMainSoundProperties.push_back(mainProperty);
    }
}

string CAmRoutingAdapterALSAParser::extractToken(string & str, const string & delimiter) const
{
    string token("");
    size_t hit = str.find(delimiter);
    if (hit != string::npos)
    {
        token = str.substr(0, hit);
        str.erase(0, hit + delimiter.length());
    }
    else
    {
        token.swap(str);
    }
    return token;
}

void CAmRoutingAdapterALSAParser::removeWhiteSpaces(string & str) const
{
    size_t count = string::npos;
    // remove space and tab from string
    while (string::npos != (count = str.find(' ')) || string::npos != (count = str.find('\t')))
    {
        str.erase(count, 1);
    }
}

std::string CAmRoutingAdapterALSAParser::trailWhiteSpaces(std::string & ref) const
{
    const std::string whitespace = " \t";
    const auto begin = ref.find_first_not_of(whitespace);
    if (begin == std::string::npos)
    {
        return ""; // no content
    }
    const auto end = ref.find_last_not_of(whitespace);
    std::string result = ref.substr(begin, end - begin + 1);
    return result;
}

void CAmRoutingAdapterALSAParser::getKeyValPairs(const xmlNodePtr devNode, CAmRoutingAdapterKVPConverter::KVPList & keyValPairs) const
{
    vector<string> vValues;
    xmlAttrPtr attr = devNode->properties;

    for (; attr && attr->name && attr->children; attr = attr->next)
    {
        xmlChar * children = xmlNodeListGetString(devNode->doc, attr->children, 1);
        if (!xmlStrlen(children))
        {
            logAmRaInfo("CRaALSAParser::getKeyValPairs KeyValue is not defined, for key: ", attr->name);
        }
        else
        {
            /* extracts now the values "val3.1;val3.2;val3.3" */
            string values = (const char *)children;
            while (values.length())
            {
                string token(extractToken(values, ";"));
                vValues.push_back(trailWhiteSpaces(token));
            }
            keyValPairs.insert(make_pair((const char*)attr->name, vValues));
            vValues.clear();
        }

        /* free the children buffer */
        xmlFree(children);
    }
}

void CAmRoutingAdapterALSAParser::updateDomainInfo(ra_domainInfo_s & domainInfo, CAmRoutingAdapterKVPConverter::KVPList & keyValPair)
{
    parseDomainData(domainInfo, keyValPair);
    mpDomainRef = mDataBase.createDomain(domainInfo);
}

void CAmRoutingAdapterALSAParser::updateSourceInfo(const xmlNodePtr devNode, CAmRoutingAdapterKVPConverter::KVPList & keyValPair)
{
    ra_sourceInfo_s sourceInfo;
    parseSourceData(sourceInfo, keyValPair);
    parseChildInfo(devNode, sourceInfo);
    mpDomainRef->lSourceInfo.push_back(sourceInfo);
}

void CAmRoutingAdapterALSAParser::updateSinkInfo(const xmlNodePtr devNode, CAmRoutingAdapterKVPConverter::KVPList & keyValPair)
{
    ra_sinkInfo_s sinkInfo;
    parseSinkData(sinkInfo, keyValPair);
    parseChildInfo(devNode, sinkInfo);
    mpDomainRef->lSinkInfo.push_back(sinkInfo);
}

void CAmRoutingAdapterALSAParser::updateUSBSourceInfo(const xmlNodePtr devNode, CAmRoutingAdapterKVPConverter::KVPList & keyValPair)
{
    ra_sourceInfo_s sourceInfo;
    parseSourceData(sourceInfo, keyValPair);
    mDataBase.getUSBInfo()->lSourceInfo.push_back(sourceInfo);
}

void CAmRoutingAdapterALSAParser::updateUSBSinkInfo(const xmlNodePtr devNode, CAmRoutingAdapterKVPConverter::KVPList & keyValPair)
{
    ra_sinkInfo_s sinkInfo;
    parseSinkData(sinkInfo, keyValPair);
    mDataBase.getUSBInfo()->lSinkInfo.push_back(sinkInfo);
}

template <typename S>
void CAmRoutingAdapterALSAParser::parseChildInfo(const xmlNodePtr devNode, S & info)
{
    bool gatewayFound = false;

    for (xmlNodePtr childNode = devNode->children; childNode != NULL; childNode = childNode->next)
    {
        if (childNode->type != XML_ELEMENT_NODE)
            continue;

        if (!xmlStrcmp(childNode->name, (const xmlChar *) "tGATEWAY"))
        {
            logAmRaDebug("CRaALSAParser::parseChildInfo", (const char*) childNode->name);

            if (gatewayFound)
            {
                logAmRaInfo("CRaALSAParser::parseChildInfo:", (const char*) devNode->name, "contains more than one tGATEWAY element");
                continue;
            }

            ra_gatewayInfo_s gatewayInfo(info);

            CAmRoutingAdapterKVPConverter::KVPList keyValPair;
            getKeyValPairs(childNode, keyValPair);
            updateGatewayInfo(gatewayInfo, keyValPair);

            /* only one gateway per sink allowed */
            gatewayFound = true;
        }
        else if (!xmlStrcmp(childNode->name, (const xmlChar *) "tPROPERTY"))
        {
            logAmRaDebug("CRaALSAParser::parseChildInfo", (const char*) childNode->name);

            CAmRoutingAdapterKVPConverter::KVPList keyValPair;
            getKeyValPairs(childNode, keyValPair);
            parseSoundPropertyData(info, keyValPair);
            parseInnerChildInfo(childNode, info);
        }
        else
        {
            logAmRaError("CRaALSAParser::parseChildInfo type unknown:", (const char*) childNode->name);
        }
    }
}

template <typename S>
void CAmRoutingAdapterALSAParser::parseInnerChildInfo(const xmlNodePtr devNode, S & info)
{
    for (xmlNodePtr childNode = devNode->children; childNode != NULL; childNode = childNode->next)
    {
        if (childNode->type != XML_ELEMENT_NODE)
            continue;

        if (!xmlStrcmp(childNode->name, (const xmlChar *) "tPROPSPEC"))
        {
            logAmRaDebug("CRaALSAParser::parseInnerChildInfo", (const char*) childNode->name);

            CAmRoutingAdapterKVPConverter::KVPList keyValPair;
            getKeyValPairs(childNode, keyValPair);
            parseSoundPropertySpecification(info, keyValPair);
        }
        else
        {
            logAmRaError("CRaALSAParser::parseInnerChildInfo type unknown:", (const char*) childNode->name);
        }
    }
}

void CAmRoutingAdapterALSAParser::updateGatewayInfo(ra_gatewayInfo_s & gatewayInfo, CAmRoutingAdapterKVPConverter::KVPList & keyValPair)
{
    parseGatewayData(gatewayInfo, keyValPair);
    mpDomainRef->lGatewayInfo.push_back(gatewayInfo);
}

void CAmRoutingAdapterALSAParser::updateProxyInfo(CAmRoutingAdapterKVPConverter::KVPList & keyValPair)
{
    ra_proxyInfo_s proxyInfo;
    proxyInfo.domNam = mpDomainRef->domain.name;
    parseProxyData(proxyInfo, keyValPair);
    mpDomainRef->lProxyInfo.push_back(proxyInfo);
}

void CAmRoutingAdapterALSAParser::updateUSBInfo(CAmRoutingAdapterKVPConverter::KVPList & keyValPair)
{
    ra_USBInfo_s usbInfo;
    parseUSBData(usbInfo, keyValPair);
    mDataBase.setUSBInfo(usbInfo);
}

void CAmRoutingAdapterALSAParser::parseXML(xmlNodePtr rootNode)
{
    CAmRoutingAdapterKVPConverter::KVPList keyValPairDomain;
    xmlNodePtr domNode = NULL;
    xmlNodePtr elemNode = NULL;

    for (domNode = rootNode->children; domNode; domNode = domNode->next)
    {
        if (domNode->type != XML_ELEMENT_NODE)
            continue;

        if (!xmlStrcmp(domNode->name, (const xmlChar *) "tDOMAIN"))
        {
            logAmRaDebug("CRaALSAParser::parseXML", (const char*)domNode->name);

            ra_domainInfo_s domain;
            keyValPairDomain.clear();
            getKeyValPairs(domNode, keyValPairDomain);
            updateDomainInfo(domain, keyValPairDomain);

            /* child elements refers to the device under domain*/
            for (elemNode = domNode->children; elemNode != NULL; elemNode = elemNode->next)
            {
                if (elemNode->type != XML_ELEMENT_NODE)
                    continue;

                ra_gatewayInfo_s gatewayInfo;
                CAmRoutingAdapterKVPConverter::KVPList keyValPair;

                getKeyValPairs(elemNode, keyValPair);
                if (!xmlStrcmp(elemNode->name, (const xmlChar *) "tSOURCE"))
                {
                    logAmRaDebug("CRaALSAParser::parseXML", (const char*)elemNode->name);
                    updateSourceInfo(elemNode, keyValPair);
                }
                else if (!xmlStrcmp(elemNode->name, (const xmlChar *) "tSINK"))
                {
                    logAmRaDebug("CRaALSAParser::parseXML", (const char*)elemNode->name);
                    updateSinkInfo(elemNode, keyValPair);
                }
                else if (!xmlStrcmp(elemNode->name, (const xmlChar *) "tGATEWAY"))
                {
                    logAmRaDebug("CRaALSAParser::parseXML", (const char*)elemNode->name);
                    gatewayInfo.ctrlDomNam = domain.domain.name;
                    updateGatewayInfo(gatewayInfo, keyValPair);
                }
                else if (!xmlStrcmp(elemNode->name, (const xmlChar *) "tPROXY"))
                {
                    logAmRaDebug("CRaALSAParser::parseXML", (const char*)elemNode->name);
                    updateProxyInfo(keyValPair);
                }
                else
                {
                    logAmRaInfo("CRaALSAParser::parseXML:", (const char*) domNode->name, " is not a domain content");
                }
            }
        }
        else if (!xmlStrcmp(domNode->name, (const xmlChar *) "tUSB"))
        {
            logAmRaDebug("CRaALSAParser::parseXML", (const char*)domNode->name);

            keyValPairDomain.clear();
            getKeyValPairs(domNode, keyValPairDomain);
            updateUSBInfo(keyValPairDomain);
            /* child elements refers to the device under domain*/
            for (elemNode = domNode->children; elemNode != NULL; elemNode = elemNode->next)
            {
                if (elemNode->type != XML_ELEMENT_NODE)
                    continue;

                CAmRoutingAdapterKVPConverter::KVPList keyValPair;
                getKeyValPairs(elemNode, keyValPair);
                if (!xmlStrcmp(elemNode->name, (const xmlChar *) "tSOURCE"))
                {
                    logAmRaDebug("CRaALSAParser::parseXML", (const char*)elemNode->name);
                    updateUSBSourceInfo(elemNode, keyValPair);
                }
                else if (!xmlStrcmp(elemNode->name, (const xmlChar *) "tSINK"))
                {
                    logAmRaDebug("CRaALSAParser::parseXML", (const char*)elemNode->name);
                    updateUSBSinkInfo(elemNode, keyValPair);
                }
                else
                {
                    logAmRaInfo("CRaALSAParser::parseXML:", (const char*) domNode->name, " is not a USB domain content");
                }
            }
        }
        else if (xmlStrcmp(domNode->name, (const xmlChar *) "tDBUS_CNFG"))
        {
            logAmRaError("CRaALSAParser::parseXML type unknown:", (const char*) domNode->name);
        }
        else
        {
            /* nothing to do */
        }
    }
}

void CAmRoutingAdapterALSAParser::readConfig()
{
    const char* alsaDevConfigFile = ROUTING_ADAPTER_ALSA_DEFAULT_CONF_ROOT"/alsa.xml";
    const char* envValue = getenv("AMRA_ALSA_CONFIGPATH");
    if (envValue)
    {
        alsaDevConfigFile = envValue;
        logAmRaInfo("CRaALSAParser::readConfig ALSA XML config file path:", envValue);
    }
    else
    {
        logAmRaInfo("CRaALSAParser::readConfig ALSA XML config file path set to default:", alsaDevConfigFile);
    }

    xmlDocPtr xmlFile = xmlParseFile(alsaDevConfigFile);
    if (NULL != xmlFile)
    {
        bool amra_alsa_flag = false;

        xmlNodePtr curNode = xmlDocGetRootElement(xmlFile);
        for (; curNode != NULL; curNode = curNode->next)
        {
            if ( (curNode->type == XML_ELEMENT_NODE) &&
                 (!xmlStrcmp(curNode->name, (const xmlChar *)ROUTING_ADAPTER_ALSA_NODE_NAME)) )
            {
                parseXML(curNode);
                amra_alsa_flag = true;
            }
        }
        xmlFreeDoc(xmlFile);

        if (!amra_alsa_flag)
        {
            logAmRaError("CRaALSAParser::readConfig", ROUTING_ADAPTER_ALSA_NODE_NAME, "is not defined in the xml.");
        }
    }
    else
    {
        logAmRaError("CRaALSAParser::readConfig Failed to open ALSA XML config file");
    }
}
