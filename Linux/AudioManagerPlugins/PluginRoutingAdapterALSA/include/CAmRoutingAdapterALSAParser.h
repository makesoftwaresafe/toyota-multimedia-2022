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


#ifndef ROUTINGADAPTERALSAPARSER_H_
#define ROUTINGADAPTERALSAPARSER_H_

#include <alsa/asoundlib.h>
#include <string>
#include <vector>
#include <map>
#include <libxml/parser.h> // for xmlParsing library calls
#include <libxml/tree.h>
#include "CAmRoutingAdapterKVPConverter.h"
#include "CAmRoutingAdapterALSAdb.h"

namespace am
{

class CAmRoutingAdapterALSAParser
{
public:
    CAmRoutingAdapterALSAParser(CAmRoutingAdapterALSAdb & db, std::string & busname);
    ~CAmRoutingAdapterALSAParser() {};
    void readConfig();

private:

    /*Domain member map*/
    void parseDomainData(ra_domainInfo_s & info, CAmRoutingAdapterKVPConverter::KVPList & kvpList);
    /*source member map*/
    void parseSourceData(ra_sourceInfo_s & info, CAmRoutingAdapterKVPConverter::KVPList & kvpList);
    /*sink member map*/
    void parseSinkData(ra_sinkInfo_s & info, CAmRoutingAdapterKVPConverter::KVPList & kvpList);
    /*Gateway member map*/
    void parseGatewayData(ra_gatewayInfo_s & info, CAmRoutingAdapterKVPConverter::KVPList & kvpList);
    /*Proxy member map*/
    void parseProxyData(ra_proxyInfo_s & info, CAmRoutingAdapterKVPConverter::KVPList & kvpList);
    /*USB member map*/
    void parseUSBData(ra_USBInfo_s & info, CAmRoutingAdapterKVPConverter::KVPList & kvpList);
    /*SoundProperty member map */
    template <typename S> void parseSoundPropertyData(S & info, CAmRoutingAdapterKVPConverter::KVPList & kvpList);
    /*SoundPropertySpecification member map */
    template <typename S> void parseSoundPropertySpecification(S & info, CAmRoutingAdapterKVPConverter::KVPList & kvpList);

    /*Remove space and tab in the string*/
    inline void removeWhiteSpaces(std::string & str) const;
    /*Removes only heading and trailing spaces and tabs, trims*/
    std::string trailWhiteSpaces(std::string & ref) const;
    /*It extract the token from given string and returns it*/
    std::string extractToken(std::string & str, const std::string & delimiter) const;
    /* Returns array of key value pair from the comment string*/
    void getKeyValPairs(const xmlNodePtr devNode, CAmRoutingAdapterKVPConverter::KVPList & keyValPairs) const;

    /* Adds the Domain information into list*/
    void updateDomainInfo(ra_domainInfo_s & domain, CAmRoutingAdapterKVPConverter::KVPList & keyValPair);
    /* Adds the source information into lSourceInfo*/
    void updateSourceInfo(const xmlNodePtr devNode, CAmRoutingAdapterKVPConverter::KVPList & keyValPair);
    /* Adds the Sink information into lSinkInfo*/
    void updateSinkInfo(const xmlNodePtr devNode, CAmRoutingAdapterKVPConverter::KVPList & keyValPair);
    /* Adds the USB source information into lSourceInfo*/
    void updateUSBSourceInfo(const xmlNodePtr devNode, CAmRoutingAdapterKVPConverter::KVPList & keyValPair);
    /* Adds the USB Sink information into lSinkInfo*/
    void updateUSBSinkInfo(const xmlNodePtr devNode, CAmRoutingAdapterKVPConverter::KVPList & keyValPair);
    /* Adds the USB information*/
    void updateUSBInfo(CAmRoutingAdapterKVPConverter::KVPList & keyValPair);
    /* updates child information of a sink or source */
    template <typename S> void parseChildInfo(const xmlNodePtr devNode, S & info);
    /* updates inner child information of a property of sink or source */
    template <typename S> void parseInnerChildInfo(const xmlNodePtr devNode, S & info);
    /* updates gateway information inside lGatewayInfo*/
    void updateGatewayInfo(ra_gatewayInfo_s & gatewayInfo, CAmRoutingAdapterKVPConverter::KVPList & keyValPair);
    /* Adds the Proxy information into lProxyInfo*/
    void updateProxyInfo(CAmRoutingAdapterKVPConverter::KVPList & keyValPair);
    /*Parse the comment line and fill the corresponding structure*/
    void parseXML(xmlNodePtr rootNode);

    /* Reference to Domain Information */
    CAmRoutingAdapterALSAdb &       mDataBase;
    ra_domainInfo_s * mpDomainRef;
    std::string &     mBusname;
    am_CustomSoundPropertyType_t mLastSeenType;
    SoundPropertyVisibility mSoundPropertyVisibility;

};

} /* namespace am */

#endif /* ROUTINGADAPTERALSAPARSER_H_ */
