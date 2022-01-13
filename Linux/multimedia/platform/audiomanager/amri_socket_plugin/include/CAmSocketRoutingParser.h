/*******************************************************************************
 *  \copyright (c) 2016 Advanced Driver Information Technology.
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


#ifndef DBUS_ROUTING_PARSER_H_
#define DBUS_ROUTING_PARSER_H_

#include <string>
#include <vector>
#include <libxml/parser.h> // for xmlParsing library calls
#include <libxml/tree.h>
#include "CAmSocketRoutingKVPConverter.h"
#include "CAmSocketRoutingdb.h"

#include "audiomanagertypes.h"
#include <iostream>

namespace am
{

class CAmDbusRoutingParser
{
public:
    CAmDbusRoutingParser(CAmDbusRoutingdb & db);
    ~CAmDbusRoutingParser() {};
    void readConfig();

private:

    /*Domain member map*/
    void parseDomainData(dr_domain_s & info, CAmDbusRoutingKVPConverter::KVPList & kvpList);
    /*source member map*/
    void parseSourceData(dr_source_s & info, CAmDbusRoutingKVPConverter::KVPList & kvpList);
    /*sink member map*/
    void parseSinkData(dr_sink_s & info, CAmDbusRoutingKVPConverter::KVPList & kvpList);

    /* Adds the Domain information into list*/
    void updateDomainInfo(dr_domain_s & domain, CAmDbusRoutingKVPConverter::KVPList & keyValPair);
    /* Adds the DBus Policies information into list*/
    void updatePolicyInfo(std::string & policiesPath, CAmDbusRoutingKVPConverter::KVPList & keyValPair);
    /* Adds the source information into lSourceInfo*/
    void updateSourceInfo(const xmlNodePtr devNode, CAmDbusRoutingKVPConverter::KVPList & keyValPair);
    /* Adds the Sink information into lSinkInfo*/
    void updateSinkInfo(const xmlNodePtr devNode, CAmDbusRoutingKVPConverter::KVPList & keyValPair);
    /*Parse the comment line and fill the corresponding structure*/
    void parseXML(xmlNodePtr rootNode);

    /*Remove space and tab in the string*/
    inline void removeWhiteSpaces(std::string & str) const;
    /*Removes only heading and trailing spaces and tabs, trims*/
    std::string trailWhiteSpaces(std::string & ref) const;
    /*It extract the token from given string and returns it*/
    std::string extractToken(std::string & str, const std::string & delimiter) const;
    /* Returns array of key value pair from the comment string*/
    void getKeyValPairs(const xmlNodePtr devNode, CAmDbusRoutingKVPConverter::KVPList & keyValPairs) const;

    CAmDbusRoutingdb &mDataBase;
    dr_domain_s *mpDomainRef;
    std::string mDbusPoliciesPath;
};

} /* namespace am */

#endif /* DBUS_ROUTING_PARSER_H_ */
