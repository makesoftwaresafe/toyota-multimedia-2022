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

#include "PluginRoutingAdapterALSATest.h"

using namespace testing;
using namespace std;

namespace am
{

PluginRoutingAdapterALSATest::PluginRoutingAdapterALSATest()
{
    /* Here the KVP list is populated like it is done when reading the configuration
     * from the XML file. Testing against:
     * - simpleBool
     * - vectorBool
     * - nonSpecializedDouble
     */

    xmlNodes =
    {
        {"simpleBool", "simpleBool"},
        {"vectorBool", "vectorBool"},
        {"nonSpecializedDouble", "nonSpecializedDouble"}
    };

    xmlValues =
    {
        {xmlNodes["simpleBool"], {"true"} },
        {xmlNodes["vectorBool"], {"true", "true", "false"} },
        {xmlNodes["nonSpecializedDouble"], {"3.1415"} }
    };

    for (auto && it : xmlValues)
    {
        keyValPair.insert(make_pair(it.first, it.second));
    }
}

PluginRoutingAdapterALSATest::~PluginRoutingAdapterALSATest()
{
}

TEST(testKVP, simpleBool)
{
    am::PluginRoutingAdapterALSATest inst;
    bool expectedBool = true;
    ASSERT_EQ(
            expectedBool,
            CAmRoutingAdapterKVPConverter(inst.keyValPair, NULL)
            .kvpQueryValue(inst.xmlNodes["simpleBool"], !expectedBool)
    );
}

TEST(testKVP, vectorBool)
{
    vector<bool> expectedVectorBool = {true, true, false};
    vector<bool> defaultVectorBool = {false};
    /* Every combination of three bool will be tested, only the right one is expected
     * to match
     */
    std::vector<std::vector<bool> > compareThreeElementBool =
    {
        {false, false, false},
        {false, false, true},
        {false, true, false},
        {false, true, true},
        {true, false, false},
        {true, false, true},
        {true, true, false},
        {true, true, true},
    };
    for (auto && it : compareThreeElementBool)
    {
        /* Please note, the instance has to be requested inside the loop
         * so that the map is regenerated every time in its ctor: when a key is
         * found, it is removed by the map, and if the test was run again it
         * would use the defaultVectorBool
         * */
        am::PluginRoutingAdapterALSATest inst;
        if (it == expectedVectorBool)
        {
            ASSERT_EQ(
                it,
                CAmRoutingAdapterKVPConverter(inst.keyValPair, NULL)
                .kvpQueryValue(inst.xmlNodes["vectorBool"], defaultVectorBool)
            );
        }
        else
        {
            ASSERT_NE(
                it,
                CAmRoutingAdapterKVPConverter(inst.keyValPair, NULL)
                .kvpQueryValue(inst.xmlNodes["vectorBool"], defaultVectorBool)
            );
        }
    }
}

TEST(testKVP, nonSpecializedDouble)
{
    double expectedDouble = 3.1415;
    vector<double> comparisonDouble = {3.1415, 1.4142, -std::numeric_limits<double>::max()};
    for (auto && it : comparisonDouble)
    {
        am::PluginRoutingAdapterALSATest inst;
        if (it == expectedDouble)
        {
            ASSERT_EQ(
                it,
                CAmRoutingAdapterKVPConverter(inst.keyValPair, NULL)
                .kvpQueryValue(inst.xmlNodes["nonSpecializedDouble"], 0.0)
            );
        }
        else
        {
            ASSERT_NE(
                it,
                CAmRoutingAdapterKVPConverter(inst.keyValPair, NULL)
                .kvpQueryValue(inst.xmlNodes["nonSpecializedDouble"], 0.0)
            );
        }
    }
}

} /* namespace am */

int main(int argc, char **argv)
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
