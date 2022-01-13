/**
 *  Copyright (c) 2012 BMW
 *
 *  \author Aleksandar Donchev, aleksander.donchev@partner.bmw.de BMW 2013
 *
 *  \copyright
 *  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction,
 *  including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so,
 *  subject to the following conditions:
 *  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 *  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR
 *  THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 *  For further information see http://www.genivi.org/.
 */


#ifndef ROUTINGINTERFACETEST_H_
#define ROUTINGINTERFACETEST_H_

#define UNIT_TEST 1

#include <CommonAPI/CommonAPI.hpp>
#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "CAmSocketHandler.h"
#include "CAmTestCAPIWrapper.h"
#include "../include/CAmRoutingSenderCommon.h"
#include "../include/CAmRoutingSenderCAPI.h"
#include "MockIAmRoutingReceive.h"
#include <v1/org/genivi/am/routinginterface/RoutingControlObserverProxy.hpp>
#include <v1/org/genivi/am/routinginterface/RoutingControlProxy.hpp>

using namespace testing;

namespace am {

class CAmCommandSenderDbusBackdoor;
class IAmCommandSend;
class CAmTestRoutingSenderService;

/** Global test environment which sets the routing interface service and test domain service for all tests.
 * The tests are executed after the connection has been successful established.
 * If the connection is not available for some reason all tests will fail.
 */
class CAmTestsEnvironment : public ::testing::Environment
{
	pthread_t mListenerThread; ///< supporting sync thread
	pthread_t mServicePThread; ///< plugin's thread
	pthread_t mClientPThread; ///< thread for the routing interface proxy and test domain stub

public:
	bool mIsProxyInitilized;
	bool mIsServiceAvailable;
	bool mIsDomainAvailable;
	CAmSocketHandler *mSocketHandlerService;
	CAmSocketHandler *mSocketHandlerClient;
	MockIAmRoutingReceive *mpRoutingReceive;
	CAmRoutingSenderCAPI *mpPlugin;

	std::shared_ptr<am_routing_interface::RoutingControlObserverProxy<> >  mProxy; ///< pointer to routing interface proxy
	std::shared_ptr<CAmTestRoutingSenderService>  mDomainService; ///< pointer to the test domain service

	CAmTestsEnvironment();
    ~CAmTestsEnvironment();
    void SetUp();
    // Override this to define how to tear down the environment.
    void TearDown();
    void onServiceStatusEventProxy(const CommonAPI::AvailabilityStatus& serviceStatus);
    bool isServiceAvailable() { return mIsServiceAvailable; };
};

/**
 * Common tests - types and calls matching
 */
class CAmRoutingInterfaceCAPITests :public ::testing::Test
{

public:
	CAmRoutingInterfaceCAPITests();
	~CAmRoutingInterfaceCAPITests();

	void SetUp();
	void TearDown();

};

}

#endif /* ROUTINGINTERFACETEST_H_ */
