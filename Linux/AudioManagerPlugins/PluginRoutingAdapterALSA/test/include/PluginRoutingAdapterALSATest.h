/*******************************************************************************
 *  \copyright (c) 2016 Advanced Driver Information Technology.
 *                   ADIT is a joint venture company of
 *   Robert Bosch GmbH/Robert Bosch Car Multimedia GmbH and DENSO Corporation
 *
 *  \author: Jens Lorenz, jlorenz@de.adit-jv.com 2016
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

#ifndef PLUGINROUTINGADAPTERALSATEST_H_
#define PLUGINROUTINGADAPTERALSATEST_H_

#include "gtest/gtest.h"
#include "CAmRoutingAdapterKVPConverter.h"
#include <vector>
#include <string>
#include <map>

namespace am
{

class PluginRoutingAdapterALSATest
{
public:
    PluginRoutingAdapterALSATest();
    virtual ~PluginRoutingAdapterALSATest();
    CAmRoutingAdapterKVPConverter::KVPList keyValPair;
    std::map<std::string, std::string> xmlNodes;
    std::map<std::string, std::vector<std::string> > xmlValues;
};

} /* namespace am */
#endif /* PLUGINROUTINGADAPTERALSATEST_H_ */

