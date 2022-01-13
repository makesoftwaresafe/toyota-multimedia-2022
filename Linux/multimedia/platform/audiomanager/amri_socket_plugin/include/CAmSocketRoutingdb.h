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


#ifndef DBUS_ROUTING_DB_H_
#define DBUS_ROUTING_DB_H_

#include <string>
#include <vector>
#include <map>
#include "CAmSocketRoutingKVPConverter.h"

#include "audiomanagertypes.h"

namespace am
{

enum amri_CommunicationMode
{
    CM_UNICAST = 0,
    CM_MULTICAST,
    CM_MAX
};

struct dr_source_s
{
    am_sourceID_t id;
    std::string name;
    std::string application;
};

struct dr_sink_s
{
    am_sinkID_t id;
    std::string name;
    std::string application;
};

struct dr_domain_s
{
    am_domainID_t id;
    std::string name;
    std::string nodeName;
    std::string owner;
    amri_CommunicationMode comMod;
    bool isThereRA;
    std::vector<dr_source_s> lSrc;
    std::vector<dr_sink_s> lSnk;
};

class CAmDbusRoutingdb
{
public:
    dr_domain_s * createDomain(dr_domain_s &domain)
    {
        mDomains.push_back(domain);
        return &mDomains.back();
    }
    std::vector<dr_domain_s> & getDomains()
    {
        return mDomains;
    }
    void writeDBusPathToList(const std::string & path);
    const std::vector<std::string> & getDBusInterfacesList()
    {
        return mDBusInterfaces;
    }
private:
    std::vector<dr_domain_s> mDomains;
    std::vector<std::string> mDBusInterfaces;
};

} /* namespace am */

#endif /* DBUS_ROUTING_DB_H_ */
