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


#include <cstring>
#include <stdlib.h> // for getenv()
#include <fstream> //for file operations
#include "CAmSocketRoutingParser.h"
#include "CAmDltWrapper.h"
#include "CAmSocketRoutingSender.h"

DLT_IMPORT_CONTEXT (routingDbus)

using namespace std;
using namespace am;

#ifndef DBUS_ROUTING_DEFAULT_CONF_ROOT
#define DBUS_ROUTING_DEFAULT_CONF_ROOT "/etc/audiomanager/routing"
#endif

#define DBUS_ROUTING_NODE_NAME "amri_dbus"

CAmDbusRoutingParser::CAmDbusRoutingParser(CAmDbusRoutingdb & db) :
        mDataBase(db)
{
}

void CAmDbusRoutingParser::parseDomainData(dr_domain_s & domain, CAmDbusRoutingKVPConverter::KVPList & kvpList)
{
    const std::map<std::string, amri_CommunicationMode> amri_CommunicationMap = {
        {"UNICAST", CM_UNICAST},
        {"MULTICAST", CM_MULTICAST}
    };
    CAmDbusRoutingKVPConverter converter(kvpList, logDebug, logError);
    domain.id = converter.kvpQueryValue("id", 0);
    domain.name = converter.kvpQueryValue("domNam", std::string(""));
    domain.nodeName = converter.kvpQueryValue("nodeNam", std::string(""));
    domain.owner = converter.kvpQueryValue("owner", std::string(""));
    domain.comMod = converter.kvpQueryValue("comMod", CM_UNICAST, amri_CommunicationMap);
}

void CAmDbusRoutingParser::parseSourceData(dr_source_s & element, CAmDbusRoutingKVPConverter::KVPList & kvpList)
{
    CAmDbusRoutingKVPConverter converter(kvpList, logDebug, logError);
    element.id = converter.kvpQueryValue("id", 0);
    element.name = converter.kvpQueryValue("srcNam", std::string(""));
    element.application = converter.kvpQueryValue("appNam", std::string(""));
}

void CAmDbusRoutingParser::parseSinkData(dr_sink_s & element, CAmDbusRoutingKVPConverter::KVPList & kvpList)
{
    CAmDbusRoutingKVPConverter converter(kvpList, logDebug, logError);
    element.id = converter.kvpQueryValue("id", 0);
    element.name = converter.kvpQueryValue("sinkNam", std::string(""));
    element.application = converter.kvpQueryValue("appNam", std::string(""));
}

void CAmDbusRoutingParser::updateDomainInfo(dr_domain_s & domain, CAmDbusRoutingKVPConverter::KVPList & keyValPair)
{
    parseDomainData(domain, keyValPair);
    mpDomainRef = mDataBase.createDomain(domain);
}

void CAmDbusRoutingParser::updatePolicyInfo(string & policiesInfo, CAmDbusRoutingKVPConverter::KVPList & keyValPair)
{
    CAmDbusRoutingKVPConverter converter(keyValPair, logDebug, logError);
    policiesInfo = converter.kvpQueryValue("path", std::string(""));
}

void CAmDbusRoutingParser::updateSourceInfo(const xmlNodePtr devNode, CAmDbusRoutingKVPConverter::KVPList & keyValPair)
{
    (void)devNode;
    dr_source_s source;
    parseSourceData(source, keyValPair);
    mpDomainRef->lSrc.push_back(source);
}

void CAmDbusRoutingParser::updateSinkInfo(const xmlNodePtr devNode, CAmDbusRoutingKVPConverter::KVPList & keyValPair)
{
    (void)devNode;
    dr_sink_s sink;
    parseSinkData(sink, keyValPair);
    mpDomainRef->lSnk.push_back(sink);
}

string CAmDbusRoutingParser::extractToken(string & str, const string & delimiter) const
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

void CAmDbusRoutingParser::removeWhiteSpaces(string & str) const
{
    size_t count = string::npos;
    // remove space and tab from string
    while (string::npos != (count = str.find(' ')) || string::npos != (count = str.find('\t')))
    {
        str.erase(count, 1);
    }
}

std::string CAmDbusRoutingParser::trailWhiteSpaces(std::string & ref) const
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

void CAmDbusRoutingParser::getKeyValPairs(const xmlNodePtr devNode, CAmDbusRoutingKVPConverter::KVPList & keyValPairs) const
{
    vector<string> vValues;
    xmlAttrPtr attr = devNode->properties;

    for (; attr && attr->name && attr->children; attr = attr->next)
    {
        xmlChar * children = xmlNodeListGetString(devNode->doc, attr->children, 1);
        if (!xmlStrlen(children))
        {
            logInfo("CRaALSAParser::getKeyValPairs KeyValue is not defined, for key: ", attr->name);
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

void CAmDbusRoutingParser::parseXML(xmlNodePtr rootNode)
{
    CAmDbusRoutingKVPConverter::KVPList keyValPairDomain;
    xmlNodePtr domNode = NULL;
    xmlNodePtr elemNode = NULL;

    for (domNode = rootNode->children; domNode; domNode = domNode->next)
    {
        if (domNode->type != XML_ELEMENT_NODE)
            continue;

        if (!xmlStrcmp(domNode->name, (const xmlChar *) "tDOMAIN"))
        {
            logDebug("CAmDbusRoutingParser::parseXML", (const char*)domNode->name);

            dr_domain_s domain;
            keyValPairDomain.clear();
            getKeyValPairs(domNode, keyValPairDomain);
            updateDomainInfo(domain, keyValPairDomain);

            /* child elements refers to the device under domain*/
            for (elemNode = domNode->children; elemNode != NULL; elemNode = elemNode->next)
            {
                if (elemNode->type != XML_ELEMENT_NODE)
                    continue;

                CAmDbusRoutingKVPConverter::KVPList keyValPair;
                getKeyValPairs(elemNode, keyValPair);

                if (!xmlStrcmp(elemNode->name, (const xmlChar *) "tSOURCE"))
                {
                    logDebug("CAmDbusRoutingParser::parseXML", (const char*)elemNode->name);
                    updateSourceInfo(elemNode, keyValPair);
                }
                else if (!xmlStrcmp(elemNode->name, (const xmlChar *) "tSINK"))
                {
                    logDebug("CAmDbusRoutingParser::parseXML", (const char*)elemNode->name);
                    updateSinkInfo(elemNode, keyValPair);
                }
                else
                {
                    logInfo("CAmDbusRoutingParser::parseXML:", (const char*) domNode->name, " is not a domain content");
                }
            }
        }
        else if (!xmlStrcmp(domNode->name, (const xmlChar *) "tDBUS_POLICIES"))
        {
            logDebug("CAmDbusRoutingParser::parseXML", (const char*)domNode->name);

            keyValPairDomain.clear();
            getKeyValPairs(domNode, keyValPairDomain);
            updatePolicyInfo(mDbusPoliciesPath, keyValPairDomain);
        }
        else if (xmlStrcmp(domNode->name, (const xmlChar *) "tDBUS_CNFG"))
        {
            logError("CRaALSAParser::parseXML type unknown:", (const char*) domNode->name);
        }
        else
        {
            /* nothing to do */
        }
    }
}

void CAmDbusRoutingParser::readConfig()
{
    const char* dbusConfigFile = DBUS_ROUTING_DEFAULT_CONF_ROOT"/dbus.xml";
    const char* envValue = getenv("DBUS_ROUTING_CONFIGPATH");
    if (envValue)
    {
        dbusConfigFile = envValue;
        logInfo("CAmDbusRoutingParser::readConfig Dbus XML config file path:", envValue);
    }
    else
    {
        logInfo("CAmDbusRoutingParser::readConfig Dbus XML config file path set to default:", dbusConfigFile);
    }

    xmlDocPtr xmlFile = xmlParseFile(dbusConfigFile);
    if (NULL != xmlFile)
    {
        bool amri_dbus_flag = false;

        xmlNodePtr curNode = xmlDocGetRootElement(xmlFile);
        for (; curNode != NULL; curNode = curNode->next)
        {
            if ( (curNode->type == XML_ELEMENT_NODE) &&
                 (!xmlStrcmp(curNode->name, (const xmlChar *)DBUS_ROUTING_NODE_NAME)) )
            {
                parseXML(curNode);
                amri_dbus_flag = true;
            }
        }
        xmlFreeDoc(xmlFile);

        if (!amri_dbus_flag)
        {
            logError("CAmDbusRoutingParser::readConfig", DBUS_ROUTING_NODE_NAME, "is not defined in the xml.");
        }
    }
    else
    {
        logError("CAmDbusRoutingParser::readConfig Failed to open Dbus XML config file");
    }

    /*
     * Here we parse the dbus policies configuration file in order to establish if we'll have
     * a Routing Adapter or not
     */
    xmlDocPtr xmlDBusFile = xmlParseFile(mDbusPoliciesPath.c_str());
    if (!xmlDBusFile)
    {
        logError("CAmDbusRoutingParser::readConfig Failed to open Dbus Policies XML file");
        return;
    }
    xmlNodePtr node = xmlDocGetRootElement(xmlDBusFile);
    node = node->xmlChildrenNode;
    xmlNodePtr innerNode;
    CAmDbusRoutingKVPConverter::KVPList keyValPair;
    for (; node != NULL; node = node->next)
    {
        /*
         * Traversing children of busconfig
         */
        if ( (node->type == XML_ELEMENT_NODE) &&
                        (!xmlStrcmp(node->name, (const xmlChar *)"policy")) )
        {
            innerNode = node->xmlChildrenNode;
            /*
             * Traversing children of policy
             */
            for (; innerNode != NULL; innerNode = innerNode->next)
            {
                if ( (innerNode->type == XML_ELEMENT_NODE) &&
                        (!xmlStrcmp(innerNode->name, (const xmlChar *)"allow")) )
                {
                    keyValPair.clear();
                    getKeyValPairs(innerNode, keyValPair);
                    CAmDbusRoutingKVPConverter converter(keyValPair, logDebug);
                    string interface = converter.kvpQueryValue("own", std::string(""));
                    mDataBase.writeDBusPathToList(interface);
                }
            }
        }
    }
    xmlFreeDoc(xmlDBusFile);
}
