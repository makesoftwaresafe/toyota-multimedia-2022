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

#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <string>
#include <vector>
#include <set>
#include <sys/time.h>

#include "CAmRoutingInterfaceCAPITests.h"
#include "TAmPluginTemplate.h"
#include "CAmDltWrapper.h"
#include "../include/CAmRoutingSenderCAPI.h"
#include "../include/CAmRoutingSenderCommon.h"
#include "CAmTestRoutingSenderService.h"
#include "IAmRoutingSenderBackdoor.h"

using namespace am;
using namespace testing;

static CAmTestsEnvironment* env;

/**
 * Main thread synchronization
 */
pthread_cond_t      cond  		= PTHREAD_COND_INITIALIZER;
pthread_mutex_t     mutex		= PTHREAD_MUTEX_INITIALIZER;

/**
 * Common-API connection thread synchronization
 */
pthread_cond_t      condPxy 	= PTHREAD_COND_INITIALIZER;
pthread_mutex_t     mutexPxy    = PTHREAD_MUTEX_INITIALIZER;

/**
 * Proxy initialization thread synchronization
 */
pthread_cond_t      condSer 	= PTHREAD_COND_INITIALIZER;
pthread_mutex_t     mutexSer    = PTHREAD_MUTEX_INITIALIZER;


#define TEST_ID_1 DYNAMIC_ID_BOUNDARY+2
#define TEST_ID_2 DYNAMIC_ID_BOUNDARY+3

/**
 * Initialize common-api and am sinks
 */
void initSink(am_types::am_Sink_s & newSink, am_Sink_s & newAmSink, const am_types::am_domainID_t & domainID, const am_types::am_sinkID_t & sinkID = 0)
{
	am_types::am_MuteState_e muteState = am_types::am_MuteState_e::MS_UNKNOWN;
    am_types::am_Availability_s available(am_types::am_Availability_e::A_AVAILABLE, AR_UNKNOWN);
	am_types::am_SoundProperty_L listSoundProperties;
	listSoundProperties.push_back(am_types::am_SoundProperty_s(SP_GENIVI_MID, 100));
	listSoundProperties.push_back(am_types::am_SoundProperty_s(SP_GENIVI_BASS, 101));
	listSoundProperties.push_back(am_types::am_SoundProperty_s(SP_UNKNOWN, 102));
	am_types::am_ConnectionFormat_L listConnectionFormats;
	listConnectionFormats.push_back(CF_UNKNOWN);
	am_types::am_MainSoundProperty_L listMainSoundProperties;
	listMainSoundProperties.push_back(am_types::am_MainSoundProperty_s(MSP_UNKNOWN, 100));
	am_types::am_NotificationConfiguration_L listMainNotificationConfigurations;
	listMainNotificationConfigurations.push_back(am_types::am_NotificationConfiguration_s(NT_UNKNOWN, am_types::am_NotificationStatus_e::NS_PERIODIC, 100));
	am_types::am_NotificationConfiguration_L listNotificationConfigurations;
	listNotificationConfigurations.push_back(am_types::am_NotificationConfiguration_s(NT_UNKNOWN, am_types::am_NotificationStatus_e::NS_PERIODIC, 100));

	am_types::am_Sink_s sink(sinkID, "name", domainID, 104, 50, true, available, muteState, 50,
							listSoundProperties, listConnectionFormats, listMainSoundProperties, listMainNotificationConfigurations, listNotificationConfigurations);
	newSink = sink;
	convert_am_types(sink, newAmSink);
}

/**
 * Initialize common-api and am sources
 */
void initSource(am_types::am_Source_s & newSource, am_Source_s & newAmSource, const am_types::am_domainID_t & domainID, const am_types::am_sourceID_t & sourceID = 0)
{
	am_types::am_SourceState_e srcState = am_types::am_SourceState_e::SS_OFF;
    am_types::am_Availability_s available(am_types::am_Availability_e::A_AVAILABLE, AR_UNKNOWN);
	am_types::am_SoundProperty_L listSoundProperties;
	listSoundProperties.push_back(am_types::am_SoundProperty_s(SP_GENIVI_MID, 100));
	listSoundProperties.push_back(am_types::am_SoundProperty_s(SP_GENIVI_BASS, 101));
	listSoundProperties.push_back(am_types::am_SoundProperty_s(SP_UNKNOWN, 102));
	am_types::am_ConnectionFormat_L listConnectionFormats;
	listConnectionFormats.push_back(CF_UNKNOWN);
	am_types::am_MainSoundProperty_L listMainSoundProperties;
	listMainSoundProperties.push_back(am_types::am_MainSoundProperty_s(MSP_UNKNOWN, 100));
	am_types::am_NotificationConfiguration_L listMainNotificationConfigurations;
	listMainNotificationConfigurations.push_back(am_types::am_NotificationConfiguration_s(NT_UNKNOWN, am_types::am_NotificationStatus_e::NS_PERIODIC, 100));
	am_types::am_NotificationConfiguration_L listNotificationConfigurations;
	listNotificationConfigurations.push_back(am_types::am_NotificationConfiguration_s(NT_UNKNOWN, am_types::am_NotificationStatus_e::NS_PERIODIC, 100));

	am_types::am_Source_s source(sourceID, domainID, "name", 104, srcState, 50, true, available, am_types::am_InterruptState_e::IS_UNKNOWN,
								listSoundProperties, listConnectionFormats, listMainSoundProperties, listMainNotificationConfigurations, listNotificationConfigurations);
	newSource = source;
	convert_am_types(source, newAmSource);
}

/**
 * Initialize common-api and am crossfaders
 */
void initCrossfader(am_types::am_Crossfader_s & newCrossfader, am_Crossfader_s & newAmCrossfader, const am_types::am_crossfaderID_t & crossfaderID = 0)
{
	am_types::am_Crossfader_s crossfader(crossfaderID, "name", TEST_ID_1, TEST_ID_2, TEST_ID_1, am_types::am_HotSink_e::HS_UNKNOWN);
	convert_am_types(crossfader, newAmCrossfader);
	newCrossfader = crossfader;
}

/**
 * Client thread where the proxy and the routing test domain service run
 */
void* run_client(void*)
{
	CAmSocketHandler socketHandler;
	CAmTestCAPIWrapper wrapper(&socketHandler, "AudioManager-client");
	env->mSocketHandlerClient = &socketHandler;

	env->mProxy = wrapper.buildProxy<am_routing_interface::RoutingControlObserverProxy>(CAmRoutingSenderCAPI::DEFAULT_DOMAIN, CAmRoutingSenderCAPI::ROUTING_INSTANCE);
	assert(env->mProxy);

	env->mProxy->getProxyStatusEvent().subscribe(std::bind(&CAmTestsEnvironment::onServiceStatusEventProxy,env,std::placeholders::_1));

	env->mDomainService = std::make_shared<CAmTestRoutingSenderService>(&wrapper, env->mProxy);

	if( false == wrapper.registerService(env->mDomainService, CAmRoutingSenderCAPI::DEFAULT_DOMAIN, CAPI_SENDER_INSTANCE) )
	{
		printf("\n Can't register service -> %s \n", CAPI_SENDER_INSTANCE);
		logInfo("Can't register service -> ", CAPI_SENDER_INSTANCE);
	}

	pthread_mutex_lock(&mutexSer);
	env->mIsProxyInitilized = true;
	pthread_mutex_unlock(&mutexSer);
	pthread_cond_signal(&condSer);

	socketHandler.start_listenting();

//Cleanup
    env->mProxy.reset();
    env->mSocketHandlerClient = NULL;

    return (NULL);
}

/**
 * Service thread for the routing plugin
 */
void* run_service(void*)
{
	CAmSocketHandler socketHandler;
	CAmTestCAPIWrapper wrapper(&socketHandler, "AudioManager");
	CAmRoutingSenderCAPI *pPlugin = CAmRoutingSenderCAPI::newRoutingSenderCAPI(&wrapper);
	env->mpPlugin = pPlugin;
	env->mSocketHandlerService = &socketHandler;
	MockIAmRoutingReceive mock;
	env->mpRoutingReceive = &mock;
    if(pPlugin->startupInterface(env->mpRoutingReceive)!=E_OK)
	{
		logError("CommandSendInterface can't start!");
	}
    else
    {
    	EXPECT_CALL(*env->mpRoutingReceive, confirmRoutingReady(10,E_OK)).Times(1);
    	pPlugin->setRoutingReady(10);
    	socketHandler.start_listenting();
    	EXPECT_CALL(*env->mpRoutingReceive, confirmRoutingRundown(10,E_OK)).Times(1);
    	pPlugin->setRoutingRundown(10);
    	pPlugin->tearDownInterface(env->mpRoutingReceive);
    }
 //Cleanup
	env->mpRoutingReceive = NULL;
    env->mpPlugin = NULL;
    env->mSocketHandlerService = NULL;

    return (NULL);
}

/**
 * Helper thread that listens for signals from another threads in order to synchronize them.
 * This thread starts actually the unit tests.
 */
void* run_listener(void*)
{
    pthread_mutex_lock(&mutexSer);
    while (env->mIsProxyInitilized==false)
    {
    	std::cout << "\n\r Intialize proxy..\n\r" ;
    	pthread_cond_wait(&condSer, &mutexSer);
    }
    pthread_mutex_unlock(&mutexSer);

    time_t start = time(0);
    time_t now = start;
    pthread_mutex_lock(&mutexPxy);
    while ( env->isServiceAvailable()==false && now-start <= 15 )
    {
    	std::cout << " Waiting for proxy..\n\r" ;
        struct timespec ts = { 0, 0 };
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec += 5;
        pthread_cond_timedwait(&condPxy, &mutexPxy, &ts);
        now = time(0);
    }
    pthread_mutex_unlock(&mutexPxy);

    pthread_cond_signal(&cond);

	return NULL;
}


CAmRoutingInterfaceCAPITests::CAmRoutingInterfaceCAPITests()
{

}

CAmRoutingInterfaceCAPITests::~CAmRoutingInterfaceCAPITests()
{

}

void CAmRoutingInterfaceCAPITests::SetUp()
{
//	sleep(1);
}

void CAmRoutingInterfaceCAPITests::TearDown()
{
	EXPECT_TRUE(Mock::VerifyAndClearExpectations(env->mpRoutingReceive));
}


int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    ::testing::AddGlobalTestEnvironment(new CAmTestsEnvironment);
	pthread_cond_wait(&cond, &mutex);
	logInfo("Run all tests");
	std::cout << std::endl << "Run all tests" << std::endl;
    return RUN_ALL_TESTS();
}

CAmTestsEnvironment::CAmTestsEnvironment() :
				mIsProxyInitilized(false),
				mIsServiceAvailable(false),
				mIsDomainAvailable(false),
				mListenerThread(0),
				mServicePThread(0),
				mClientPThread(0),
				mSocketHandlerService(NULL),
				mSocketHandlerClient(NULL),
				mpRoutingReceive(NULL),
				mpPlugin(NULL),
				mProxy(NULL),
				mDomainService(NULL)
{
    env=this;
     CAmDltWrapper::instanctiateOnce("capiTest", "capiTest");
	pthread_create(&mListenerThread, NULL, run_listener, NULL);
    pthread_create(&mServicePThread, NULL, run_service, NULL);
    pthread_create(&mClientPThread, NULL, run_client, NULL);
}

CAmTestsEnvironment::~CAmTestsEnvironment()
{

}

void CAmTestsEnvironment::SetUp()
{
//	pthread_cond_wait(&cond, &mutex);
//	logInfo("CAmTestsEnvironment::SetUp GO received");
//	std::cout << std::endl << "CAmTestsEnvironment::SetUp GO received" << std::endl;
}

void CAmTestsEnvironment::TearDown()
{
	if(mSocketHandlerClient)
		mSocketHandlerClient->exit_mainloop();
    pthread_join(mClientPThread, NULL);

    if(mSocketHandlerService)
    	mSocketHandlerService->exit_mainloop();
    pthread_join(mServicePThread, NULL);
    sleep(1);
}

void CAmTestsEnvironment::onServiceStatusEventProxy(const CommonAPI::AvailabilityStatus& serviceStatus)
{
    std::stringstream  avail;
    avail  << "(" << static_cast<int>(serviceStatus) << ")";

    logInfo("CAmTestsEnvironment::onServiceStatusEventProxy -> Service Status changed to ", avail.str());
    std::cout << std::endl << "CAmTestsEnvironment::onServiceStatusEventProxy -> Service Status changed to " << avail.str() << std::endl;
    pthread_mutex_lock(&mutexPxy);
    mIsServiceAvailable=(serviceStatus==CommonAPI::AvailabilityStatus::AVAILABLE);
    pthread_mutex_unlock(&mutexPxy);
   	pthread_cond_signal(&condPxy);
}

TEST_F(CAmRoutingInterfaceCAPITests, ClientStartupTest)
{
	ASSERT_TRUE(env->isServiceAvailable());

}

MATCHER_P(IsHandleEqual, value, "") {
	am_Handle_s & lh = arg;
	return lh.handle == value.handle && lh.handleType == value.handleType;
}

MATCHER_P(IsDomainDataEqualTo, value, "") {
	auto lh = arg;
	return lh.domainID == value.domainID &&
			lh.name == value.name &&
			lh.nodename == value.nodename &&
			lh.early == value.early &&
			lh.complete == value.complete &&
			lh.state == value.state;
}

ACTION(actionRegister){
	arg1=TEST_ID_1;
}

ACTION(actionPeekDomain){
	arg1=TEST_ID_1;
}

ACTION(actionPeekDomain2){
	arg1=TEST_ID_2;
}

ACTION(actionRegister2){
	arg1=TEST_ID_2;
}

ACTION(actionRegisterSource){
	arg1=TEST_ID_1;
}

MATCHER_P(IsSourceDataEqualTo, value, "") {
	auto lh = arg;

	bool bMainSoundProperties = lh.listMainSoundProperties.size() == value.listMainSoundProperties.size();
	for(int i=0; i<lh.listMainSoundProperties.size(); i++)
	{
		bMainSoundProperties &= (lh.listMainSoundProperties.at(i).type==value.listMainSoundProperties.at(i).type &&
								 lh.listMainSoundProperties.at(i).value==value.listMainSoundProperties.at(i).value);
	}

	bool bSoundProperties = lh.listSoundProperties.size() == value.listSoundProperties.size();
	for(int i=0; i<lh.listSoundProperties.size(); i++)
	{
		bSoundProperties &= (lh.listSoundProperties.at(i).type==value.listSoundProperties.at(i).type &&
							  lh.listSoundProperties.at(i).value==value.listSoundProperties.at(i).value);
	}

	bool bMainNotificationConfigurations = lh.listMainNotificationConfigurations.size() == value.listMainNotificationConfigurations.size();
	for(int i=0; i<lh.listMainNotificationConfigurations.size(); i++)
	{
        bMainNotificationConfigurations &= (lh.listMainNotificationConfigurations.at(i).type==value.listMainNotificationConfigurations.at(i).type &&
        									lh.listMainNotificationConfigurations.at(i).status==value.listMainNotificationConfigurations.at(i).status &&
        									lh.listMainNotificationConfigurations.at(i).parameter==value.listMainNotificationConfigurations.at(i).parameter);
	}

	bool bNotificationConfigurations = lh.listNotificationConfigurations.size() == value.listNotificationConfigurations.size();
	for(int i=0; i<lh.listNotificationConfigurations.size(); i++)
	{
		bNotificationConfigurations &= (lh.listNotificationConfigurations.at(i).type==value.listNotificationConfigurations.at(i).type &&
        									lh.listNotificationConfigurations.at(i).status==value.listNotificationConfigurations.at(i).status &&
        									lh.listNotificationConfigurations.at(i).parameter==value.listNotificationConfigurations.at(i).parameter);
	}
	return bMainSoundProperties &&
			bSoundProperties &&
			bNotificationConfigurations &&
			bMainNotificationConfigurations &&
			lh.sourceID == value.sourceID &&
			lh.name == value.name &&
			lh.domainID == value.domainID &&
			lh.sourceClassID == value.sourceClassID &&
			lh.volume == value.volume &&
			lh.visible == value.visible &&
			lh.available.availability == value.available.availability &&
			lh.available.availabilityReason == value.available.availabilityReason &&
			lh.sourceState == value.sourceState &&
			lh.interruptState == value.interruptState &&
			lh.listConnectionFormats == value.listConnectionFormats;
}

MATCHER_P(IsSinkDataEqualTo, value, "") {
	auto lh = arg;

	bool bMainSoundProperties = lh.listMainSoundProperties.size() == value.listMainSoundProperties.size();
	for(int i=0; i<lh.listMainSoundProperties.size(); i++)
	{
		bMainSoundProperties &= (lh.listMainSoundProperties.at(i).type==value.listMainSoundProperties.at(i).type &&
								 lh.listMainSoundProperties.at(i).value==value.listMainSoundProperties.at(i).value);
	}

	bool bSoundProperties = lh.listSoundProperties.size() == value.listSoundProperties.size();
	for(int i=0; i<lh.listSoundProperties.size(); i++)
	{
		bSoundProperties &= (lh.listSoundProperties.at(i).type==value.listSoundProperties.at(i).type &&
							  lh.listSoundProperties.at(i).value==value.listSoundProperties.at(i).value);
	}

	bool bMainNotificationConfigurations = lh.listMainNotificationConfigurations.size() == value.listMainNotificationConfigurations.size();
	for(int i=0; i<lh.listMainNotificationConfigurations.size(); i++)
	{
        bMainNotificationConfigurations &= (lh.listMainNotificationConfigurations.at(i).type==value.listMainNotificationConfigurations.at(i).type &&
        									lh.listMainNotificationConfigurations.at(i).status==value.listMainNotificationConfigurations.at(i).status &&
        									lh.listMainNotificationConfigurations.at(i).parameter==value.listMainNotificationConfigurations.at(i).parameter);
	}

	bool bNotificationConfigurations = lh.listNotificationConfigurations.size() == value.listNotificationConfigurations.size();
	for(int i=0; i<lh.listNotificationConfigurations.size(); i++)
	{
		bNotificationConfigurations &= (lh.listNotificationConfigurations.at(i).type==value.listNotificationConfigurations.at(i).type &&
        									lh.listNotificationConfigurations.at(i).status==value.listNotificationConfigurations.at(i).status &&
        									lh.listNotificationConfigurations.at(i).parameter==value.listNotificationConfigurations.at(i).parameter);
	}

	return bMainSoundProperties &&
			bSoundProperties &&
			bNotificationConfigurations &&
			bMainNotificationConfigurations &&
			lh.sinkID == value.sinkID &&
			lh.name == value.name &&
			lh.domainID == value.domainID &&
			lh.sinkClassID == value.sinkClassID &&
			lh.volume == value.volume &&
			lh.visible == value.visible &&
			lh.available.availability == value.available.availability &&
			lh.available.availabilityReason == value.available.availabilityReason &&
			lh.muteState == value.muteState &&
			lh.mainVolume == value.mainVolume &&
			lh.listConnectionFormats == value.listConnectionFormats;
}

ACTION(actionRegisterCrossfader){
	arg1=TEST_ID_1;
}

MATCHER_P(IsCrossfaderDataEqualTo, value, "") {
	auto lh = arg;
	return lh.crossfaderID == value.crossfaderID &&
			lh.name == value.name &&
			lh.sinkID_A == value.sinkID_A &&
			lh.sinkID_B == value.sinkID_B &&
			lh.sinkID_B == value.sinkID_B &&
			lh.sourceID == value.sourceID &&
			lh.hotSink == value.hotSink;
}

MATCHER_P2(IsSinkVolumeArrayEqualTo, value, isSink,  "") {

	std::vector<am_Volumes_s> lh = arg;
	bool bResult = lh.size() == value.size();
	for(int i=0; i<lh.size(); i++)
	{
		bResult &= (((isSink && lh.at(i).volumeID.sink==value.at(i).volumeID.sink) || (!isSink && lh.at(i).volumeID.source==value.at(i).volumeID.source)) &&
					lh.at(i).volume==value.at(i).volume &&
					lh.at(i).ramp==value.at(i).ramp &&
					lh.at(i).time==value.at(i).time);
	}
	return bResult;
}

MATCHER_P(IsHandleStructEqualTo, value, "") {
	am_Handle_s lh = arg;
	return lh.handle==value.handle &&
			lh.handleType==value.handleType ;
}

TEST_F(CAmRoutingInterfaceCAPITests, registerService)
{
	ASSERT_TRUE(env->isServiceAvailable());
	if(env->isServiceAvailable())
	{
		sleep(1);
		am::am_Domain_s amDomainData;
		convert_am_types(env->mDomainService->getDomainData(), amDomainData);

		EXPECT_CALL(*env->mpRoutingReceive, registerDomain(IsDomainDataEqualTo(amDomainData), _)).WillOnce(DoAll(actionRegister(), Return(E_OK)));

		bool result = env->mDomainService->registerDomain();
		int tries = 5;
		do{sleep(2);}while(!env->mDomainService->getIsReady() && tries-->0);
		ASSERT_TRUE( result );
		ASSERT_EQ( env->mDomainService->getDomainData().getDomainID(), TEST_ID_1 );
		IAmRoutingSenderBackdoor backdoor(env->mpPlugin);
		ASSERT_TRUE( backdoor.containsDomainWithID( TEST_ID_1 ) );
	}
}


TEST_F(CAmRoutingInterfaceCAPITests, ackConnect)
{
	ASSERT_TRUE(env->isServiceAvailable());
	if(env->isServiceAvailable())
	{
		am_types::am_connectionID_t connectionID = TEST_ID_1;
		am_Error_e error = E_OK;
		am_Handle_s handle = {H_CONNECT, 20};
		am_types::am_Handle_s retHandle(am_types::am_Handle_e::H_CONNECT, 20);
		convert_am_types(handle,retHandle);
		EXPECT_CALL(*env->mpRoutingReceive, ackConnect(IsHandleEqual(handle), connectionID, error)).Times(1);
		env->mDomainService->fireAckConnectSelective(retHandle, connectionID, am_types::am_Error_e::E_OK);
		usleep(1000000);
	}

}

TEST_F(CAmRoutingInterfaceCAPITests, ackDisconnect)
{
	ASSERT_TRUE(env->isServiceAvailable());
	if(env->isServiceAvailable())
	{
		am_types::am_connectionID_t connectionID = TEST_ID_1;
		am_Error_e error = E_OK;
		am_Handle_s handle = {H_DISCONNECT, 20};
		am_types::am_Handle_s retHandle;
		convert_am_types(handle,retHandle);
		EXPECT_CALL(*env->mpRoutingReceive, ackDisconnect(IsHandleEqual(handle), connectionID, error)).Times(1);
		env->mDomainService->fireAckDisconnectSelective(retHandle, connectionID, am_types::am_Error_e::E_OK);
		usleep(1000000);
	}

}

TEST_F(CAmRoutingInterfaceCAPITests, ackSetSinkVolumeChange)
{
	ASSERT_TRUE(env->isServiceAvailable());
	if(env->isServiceAvailable())
	{
		am_Error_e error = E_OK;
		am_volume_t volume = 50;
		am_Handle_s handle = {H_SETSINKVOLUME, 20};
		am_types::am_Handle_s retHandle;
		convert_am_types(handle,retHandle);
		EXPECT_CALL(*env->mpRoutingReceive, ackSetSinkVolumeChange(IsHandleEqual(handle), volume, error)).Times(1);
		env->mDomainService->fireAckSetSinkVolumeChangeSelective(retHandle, volume, am_types::am_Error_e::E_OK);
		usleep(1000000);
	}
}

TEST_F(CAmRoutingInterfaceCAPITests, ackSetSourceVolumeChange)
{
	ASSERT_TRUE(env->isServiceAvailable());
	if(env->isServiceAvailable())
	{
		am_Error_e error = E_OK;
		am_volume_t volume = 50;
		am_Handle_s handle = {H_SETSOURCEVOLUME, 20};
		am_types::am_Handle_s retHandle;
		convert_am_types(handle,retHandle);
		EXPECT_CALL(*env->mpRoutingReceive, ackSetSourceVolumeChange(IsHandleEqual(handle), volume, error)).Times(1);
		env->mDomainService->fireAckSetSourceVolumeChangeSelective(retHandle, volume, am_types::am_Error_e::E_OK);
		usleep(1000000);
	}
}

TEST_F(CAmRoutingInterfaceCAPITests, ackSetSourceState)
{
	ASSERT_TRUE(env->isServiceAvailable());
	if(env->isServiceAvailable())
	{
		am_Error_e error = E_OK;
		am_volume_t volume = 50;
		am_Handle_s handle = {H_SETSOURCESTATE, 20};
		am_types::am_Handle_s retHandle;
		convert_am_types(handle,retHandle);
		EXPECT_CALL(*env->mpRoutingReceive, ackSetSourceState(IsHandleEqual(handle), error)).Times(1);
		env->mDomainService->fireAckSetSourceStateSelective(retHandle, am_types::am_Error_e::E_OK);
		usleep(1000000);
	}
}

TEST_F(CAmRoutingInterfaceCAPITests, ackSetSinkSoundProperties)
{
	ASSERT_TRUE(env->isServiceAvailable());
	if(env->isServiceAvailable())
	{
		am_Error_e error = E_OK;
		am_volume_t volume = 50;
		am_Handle_s handle = {H_SETSINKSOUNDPROPERTIES, 20};
		am_types::am_Handle_s retHandle;
		convert_am_types(handle,retHandle);
		EXPECT_CALL(*env->mpRoutingReceive, ackSetSinkSoundProperties(IsHandleEqual(handle), error)).Times(1);
		env->mDomainService->fireAckSetSinkSoundPropertiesSelective(retHandle, am_types::am_Error_e::E_OK);
		usleep(1000000);
	}
}

TEST_F(CAmRoutingInterfaceCAPITests, ackSetSinkSoundProperty)
{
	ASSERT_TRUE(env->isServiceAvailable());
	if(env->isServiceAvailable())
	{
		am_Error_e error = E_OK;
		am_volume_t volume = 50;
		am_Handle_s handle = {H_SETSINKSOUNDPROPERTY, 20};
		am_types::am_Handle_s retHandle;
		convert_am_types(handle,retHandle);
		EXPECT_CALL(*env->mpRoutingReceive, ackSetSinkSoundProperty(IsHandleEqual(handle), error)).Times(1);
		env->mDomainService->fireAckSetSinkSoundPropertySelective(retHandle, am_types::am_Error_e::E_OK);
		usleep(1000000);
	}
}

TEST_F(CAmRoutingInterfaceCAPITests, ackSetSourceSoundProperties)
{
	ASSERT_TRUE(env->isServiceAvailable());
	if(env->isServiceAvailable())
	{
		am_Error_e error = E_OK;
		am_volume_t volume = 50;
		am_Handle_s handle = {H_SETSOURCESOUNDPROPERTIES, 20};
		am_types::am_Handle_s retHandle;
		convert_am_types(handle,retHandle);
		EXPECT_CALL(*env->mpRoutingReceive, ackSetSourceSoundProperties(IsHandleEqual(handle), error)).Times(1);
		env->mDomainService->fireAckSetSourceSoundPropertiesSelective(retHandle, am_types::am_Error_e::E_OK);
		usleep(1000000);
	}
}

TEST_F(CAmRoutingInterfaceCAPITests, ackSetSourceSoundProperty)
{
	ASSERT_TRUE(env->isServiceAvailable());
	if(env->isServiceAvailable())
	{
		am_Error_e error = E_OK;
		am_volume_t volume = 50;
		am_Handle_s handle = {H_SETSOURCESOUNDPROPERTY, 20};
		am_types::am_Handle_s retHandle;
		convert_am_types(handle,retHandle);
		EXPECT_CALL(*env->mpRoutingReceive, ackSetSourceSoundProperty(IsHandleEqual(handle), error)).Times(1);
		env->mDomainService->fireAckSetSourceSoundPropertySelective(retHandle, am_types::am_Error_e::E_OK);
		usleep(1000000);
	}
}

TEST_F(CAmRoutingInterfaceCAPITests, ackCrossFading)
{
	ASSERT_TRUE(env->isServiceAvailable());
	if(env->isServiceAvailable())
	{
		am_Error_e error = E_OK;
		am_HotSink_e hotSink = HS_UNKNOWN;
		am_Handle_s handle = {H_CROSSFADE, 20};
		am_types::am_Handle_s retHandle;
		convert_am_types(handle,retHandle);
		EXPECT_CALL(*env->mpRoutingReceive, ackCrossFading(IsHandleEqual(handle), hotSink, error)).Times(1);
		env->mDomainService->fireAckCrossFadingSelective(retHandle, (am_types::am_HotSink_e::Literal)hotSink, am_types::am_Error_e::E_OK);
		usleep(1000000);
	}
}

TEST_F(CAmRoutingInterfaceCAPITests, ackSourceVolumeTick)
{
	ASSERT_TRUE(env->isServiceAvailable());
	if(env->isServiceAvailable())
	{
		am_types::am_sourceID_t sourceID = TEST_ID_1;
		am_Handle_s handle = {H_SETSOURCEVOLUME, 20};
		am_types::am_volume_t volume (20);
		am_types::am_Handle_s retHandle;
		convert_am_types(handle,retHandle);
		EXPECT_CALL(*env->mpRoutingReceive, ackSourceVolumeTick(IsHandleEqual(handle), sourceID, volume)).Times(1);
		env->mDomainService->fireAckSourceVolumeTickSelective(retHandle, sourceID, volume);
		usleep(1000000);
	}
}

TEST_F(CAmRoutingInterfaceCAPITests, ackSinkVolumeTick)
{
	ASSERT_TRUE(env->isServiceAvailable());
	if(env->isServiceAvailable())
	{
		am_sinkID_t sID = TEST_ID_1;
		am_Handle_s handle = {H_SETSINKVOLUME, 20};
		am_types::am_Handle_s retHandle;
		convert_am_types(handle,retHandle);
		am_volume_t volume (20);
		EXPECT_CALL(*env->mpRoutingReceive, ackSinkVolumeTick(IsHandleEqual(handle), sID, volume)).Times(1);
		env->mDomainService->fireAckSinkVolumeTickSelective(retHandle, sID, volume);
		usleep(1000000);
	}
}

TEST_F(CAmRoutingInterfaceCAPITests, ackSetVolumes)
{
	ASSERT_TRUE(env->isServiceAvailable());
	if(env->isServiceAvailable())
	{
		uint16_t error = (uint16_t)E_OK;
		uint16_t testID = 10;
		am_types::am_Volumes_L listVolumes;
		am_types::am_SinkSourceID_t dt(103);
		listVolumes.push_back(am_types::am_Volumes_s(am_types::am_VolumeType_e::VT_SINK,
												   dt,
												   50,
												   static_cast<am_types::am_RampType_pe>(RAMP_GENIVI_DIRECT),
												   50));

		std::vector<am_Volumes_s> am_listVolumes;
		am_DataType_u dt1;
		dt1.sink = 103;
		am_listVolumes.push_back(am_Volumes_s{VT_SINK,
												dt1,
												50,
												RAMP_GENIVI_DIRECT,
												50});

		am_Handle_s handle_s;
		handle_s.handle = 10;
		handle_s.handleType = H_SETVOLUMES;
		am_types::am_Handle_s handle;
		convert_am_types(handle_s,handle);
		am_types::am_Error_e genError(am_types::am_Error_e::E_OK);
		am_types::am_Handle_s retHandle;
		convert_am_types(handle_s,retHandle);
		EXPECT_CALL(*env->mpRoutingReceive, ackSetVolumes(IsHandleStructEqualTo(handle_s), IsSinkVolumeArrayEqualTo(am_listVolumes, true), E_OK)).Times(1);
		env->mDomainService->fireAckSetVolumesSelective(retHandle, listVolumes, genError);
		usleep(1000000);
	}
}

TEST_F(CAmRoutingInterfaceCAPITests, ackSinkNotificationConfiguration)
{
	ASSERT_TRUE(env->isServiceAvailable());
	if(env->isServiceAvailable())
	{
		am_types::am_Error_e error = am_types::am_Error_e::E_OK;
		uint16_t testID = TEST_ID_1;
		am_Handle_s handle_s;
		handle_s.handle = testID;
		handle_s.handleType = H_CONNECT;
		am_types::am_Handle_s retHandle;
		convert_am_types(handle_s,retHandle);
		am_types::am_Error_e genError(am_types::am_Error_e::E_OK);
		EXPECT_CALL(*env->mpRoutingReceive, ackSinkNotificationConfiguration(IsHandleStructEqualTo(handle_s), (am_Error_e)((int)error))).Times(1);
		env->mDomainService->fireAckSinkNotificationConfigurationSelective(retHandle, genError);
		usleep(1000000);
	}
}

TEST_F(CAmRoutingInterfaceCAPITests, ackSourceNotificationConfiguration)
{
	ASSERT_TRUE(env->isServiceAvailable());
	if(env->isServiceAvailable())
	{
		am_types::am_Error_e error = am_types::am_Error_e::E_OK;
		uint16_t testID = TEST_ID_1;
		am_Handle_s handle_s;
		handle_s.handle = testID;
		handle_s.handleType = H_CONNECT;
		am_types::am_Handle_s retHandle;
		convert_am_types(handle_s,retHandle);
		am_types::am_Error_e genError(am_types::am_Error_e::E_OK);
		EXPECT_CALL(*env->mpRoutingReceive, ackSourceNotificationConfiguration(IsHandleStructEqualTo(handle_s), (am_Error_e)((int)error))).Times(1);
		env->mDomainService->fireAckSourceNotificationConfigurationSelective(retHandle, genError);
		usleep(1000000);
	}
}

TEST_F(CAmRoutingInterfaceCAPITests, peekDomain)
{
	ASSERT_TRUE(env->isServiceAvailable());
	if(env->isServiceAvailable())
	{
		CommonAPI::CallStatus callStatus = CommonAPI::CallStatus::NOT_AVAILABLE;
		am_types::am_Error_e error = am_types::am_Error_e::E_UNKNOWN;
		std::string  name("domain name");
		am_types::am_domainID_t domainID = 0;
		EXPECT_CALL(*env->mpRoutingReceive, peekDomain(name, _)).WillOnce(DoAll(actionPeekDomain2(), Return(E_ALREADY_EXISTS)));
		env->mProxy->peekDomain(name, callStatus, domainID, error);
		ASSERT_EQ( domainID, TEST_ID_2 );
		ASSERT_EQ((int)error, am_types::am_Error_e::E_ALREADY_EXISTS);
	}
}

TEST_F(CAmRoutingInterfaceCAPITests, registerDomain)
{
	ASSERT_TRUE(env->isServiceAvailable());
	if(env->isServiceAvailable())
	{
		CommonAPI::CallStatus callStatus = CommonAPI::CallStatus::NOT_AVAILABLE;
		am_types::am_Error_e error = am_types::am_Error_e::E_UNKNOWN;
		am_types::am_domainID_t domainID = 0;

		std::string name("domain name");
        std::string busname("busname");
        std::string nodename("nodename");

		am_types::am_Domain_s domainData(0, name, busname, nodename, false, false, am_types::am_DomainState_e::DS_CONTROLLED);
		am::am_Domain_s amDomainData;
		convert_am_types(domainData, amDomainData);

//If the result is E_OK, then the routing service will try to establish a connection with the domain via common-api.
//For now we won't test common-api connection with the domain therefore E_ALREADY_EXISTS is returned.
		EXPECT_CALL(*env->mpRoutingReceive, registerDomain(IsDomainDataEqualTo(amDomainData), _)).WillOnce(DoAll(actionRegister2(), Return(E_ALREADY_EXISTS)));

//example: 	[local:org.genivi.audiomanager.testdomaininterface:org.genivi.audiomanager]
		env->mProxy->registerDomain(domainData,
									"org.genivi.audiomanager", //(last part)
									"org.genivi.audiomanager.testdomaininterface", //(middle part)
									callStatus,
									domainID,
									error);
		ASSERT_EQ( domainID, TEST_ID_2 );
		ASSERT_EQ((int)error, am_types::am_Error_e::am_Error_e::E_ALREADY_EXISTS );
		ASSERT_EQ( callStatus, CommonAPI::CallStatus::SUCCESS );
	}
}

TEST_F(CAmRoutingInterfaceCAPITests, deregisterDomain)
{
	ASSERT_TRUE(env->isServiceAvailable());
	if(env->isServiceAvailable())
	{
		CommonAPI::CallStatus callStatus = CommonAPI::CallStatus::NOT_AVAILABLE;
		am_types::am_Error_e error = am_types::am_Error_e::E_UNKNOWN;
		am_types::am_domainID_t domainID = TEST_ID_2;

		EXPECT_CALL(*env->mpRoutingReceive, deregisterDomain(domainID)).WillOnce(Return(E_NON_EXISTENT));
		env->mProxy->deregisterDomain(domainID, callStatus, error);
		ASSERT_EQ((int)error, am_types::am_Error_e::E_NON_EXISTENT);
		ASSERT_EQ( callStatus, CommonAPI::CallStatus::SUCCESS );
	}
}

ACTION(actionRegisterGateway){
	arg1=TEST_ID_1;
}

MATCHER_P(IsGatewayDataEqualTo, value, "") {
	auto lh = arg;
	return lh.gatewayID == value.gatewayID &&
			lh.name == value.name &&
			lh.sinkID == value.sinkID &&
			lh.sourceID == value.sourceID &&
			lh.domainSinkID == value.domainSinkID &&
			lh.controlDomainID == value.controlDomainID &&
			lh.listSourceFormats == value.listSourceFormats &&
			lh.listSinkFormats == value.listSinkFormats &&
			lh.convertionMatrix == value.convertionMatrix;
}

TEST_F(CAmRoutingInterfaceCAPITests, registerGateway)
{
	ASSERT_TRUE(env->isServiceAvailable());
	if(env->isServiceAvailable())
	{
		CommonAPI::CallStatus callStatus = CommonAPI::CallStatus::NOT_AVAILABLE;
		am_types::am_Error_e error = am_types::am_Error_e::E_UNKNOWN;

		am_types::am_ConnectionFormat_L listSourceFormats;
		listSourceFormats.push_back(static_cast<am_types::am_ConnectionFormat_pe>(CF_GENIVI_ANALOG));
		listSourceFormats.push_back(static_cast<am_types::am_ConnectionFormat_pe>(CF_GENIVI_AUTO));
		am_types::am_ConnectionFormat_L listSinkFormats;
		listSinkFormats.push_back(static_cast<am_types::am_ConnectionFormat_pe>(CF_GENIVI_AUTO));
		listSinkFormats.push_back(static_cast<am_types::am_ConnectionFormat_pe>(CF_GENIVI_ANALOG));
		am_types::am_Convertion_L convertionMatrix;
		convertionMatrix.push_back(1);
		convertionMatrix.push_back(0);

		am_types::am_gatewayID_t gatewayID = 0;
		am_types::am_Gateway_s gateway(gatewayID, "name", 103, 104, 105, 106, 107, listSourceFormats, listSinkFormats, convertionMatrix);
		am_Gateway_s amGateway;
		convert_am_types(gateway, amGateway);

		EXPECT_CALL(*env->mpRoutingReceive, registerGateway(IsGatewayDataEqualTo(amGateway), _)).WillOnce(DoAll(actionRegisterGateway(), Return(E_OK)));
		env->mProxy->registerGateway(gateway, callStatus, gatewayID, error);
		ASSERT_EQ( gatewayID, TEST_ID_1 );
		ASSERT_EQ((int)error, am_types::am_Error_e::E_OK);
		ASSERT_EQ( callStatus, CommonAPI::CallStatus::SUCCESS );
	}

}

TEST_F(CAmRoutingInterfaceCAPITests, deregisterGateway)
{
	ASSERT_TRUE(env->isServiceAvailable());
	if(env->isServiceAvailable())
	{
		CommonAPI::CallStatus callStatus = CommonAPI::CallStatus::NOT_AVAILABLE;
		am_types::am_Error_e error = am_types::am_Error_e::E_UNKNOWN;
		am_types::am_gatewayID_t gatewayID = TEST_ID_1;

		EXPECT_CALL(*env->mpRoutingReceive, deregisterGateway(gatewayID)).WillOnce(Return(E_OK));
		env->mProxy->deregisterGateway(gatewayID, callStatus, error);
		ASSERT_EQ((int)error, am_types::am_Error_e::E_OK);
		ASSERT_EQ( callStatus, CommonAPI::CallStatus::SUCCESS );
	}

}

MATCHER_P(IsConverterDataEqualTo, value, "") {
	auto lh = arg;
	return lh.converterID == value.converterID &&
			lh.name == value.name &&
			lh.sinkID == value.sinkID &&
			lh.sourceID == value.sourceID &&
			lh.domainID == value.domainID &&
			lh.listSourceFormats == value.listSourceFormats &&
			lh.listSinkFormats == value.listSinkFormats &&
			lh.convertionMatrix == value.convertionMatrix;
}

TEST_F(CAmRoutingInterfaceCAPITests, registerConverter)
{
	ASSERT_TRUE(env->isServiceAvailable());
	if(env->isServiceAvailable())
	{
		CommonAPI::CallStatus callStatus = CommonAPI::CallStatus::NOT_AVAILABLE;
		am_types::am_Error_e error = am_types::am_Error_e::E_UNKNOWN;

		am_types::am_ConnectionFormat_L listSourceFormats;
		listSourceFormats.push_back(static_cast<am_types::am_ConnectionFormat_pe>(CF_GENIVI_ANALOG));
		listSourceFormats.push_back(static_cast<am_types::am_ConnectionFormat_pe>(CF_GENIVI_AUTO));
		am_types::am_ConnectionFormat_L listSinkFormats;
		listSinkFormats.push_back(static_cast<am_types::am_ConnectionFormat_pe>(CF_GENIVI_AUTO));
		listSinkFormats.push_back(static_cast<am_types::am_ConnectionFormat_pe>(CF_GENIVI_ANALOG));
		am_types::am_Convertion_L convertionMatrix;
		convertionMatrix.push_back(1);
		convertionMatrix.push_back(0);

		am_types::am_converterID_t converterID = 0;
		am_types::am_Converter_s gateway(converterID, "name", 103, 104, 105, listSourceFormats, listSinkFormats, convertionMatrix);
		am_Converter_s amGateway;
		convert_am_types(gateway, amGateway);

		EXPECT_CALL(*env->mpRoutingReceive, registerConverter(IsConverterDataEqualTo(amGateway), _)).WillOnce(DoAll(actionRegisterGateway(), Return(E_OK)));
		env->mProxy->registerConverter(gateway, callStatus, converterID, error);
		ASSERT_EQ( converterID, TEST_ID_1 );
		ASSERT_EQ((int)error, am_types::am_Error_e::E_OK);
		ASSERT_EQ( callStatus, CommonAPI::CallStatus::SUCCESS );
	}

}

TEST_F(CAmRoutingInterfaceCAPITests, deregisterConverter)
{
	ASSERT_TRUE(env->isServiceAvailable());
	if(env->isServiceAvailable())
	{
		CommonAPI::CallStatus callStatus = CommonAPI::CallStatus::NOT_AVAILABLE;
		am_types::am_Error_e error = am_types::am_Error_e::E_UNKNOWN;
		am_types::am_converterID_t converterID = TEST_ID_1;

		EXPECT_CALL(*env->mpRoutingReceive, deregisterConverter(converterID)).WillOnce(Return(E_OK));
		env->mProxy->deregisterConverter(converterID, callStatus, error);
		ASSERT_EQ((int)error, am_types::am_Error_e::E_OK);
		ASSERT_EQ( callStatus, CommonAPI::CallStatus::SUCCESS );
	}

}

ACTION(actionPeek){
	logInfo("peek", arg0);
	arg1=TEST_ID_1;
}

ACTION(actionPeekUnexpected){
	logError("unexpected peek", arg0);
	std::cout<<"unexpected peek " << arg0.c_str()<<std::endl;
	arg1=TEST_ID_1;
}

TEST_F(CAmRoutingInterfaceCAPITests, peekSink)
{
	ASSERT_TRUE(env->isServiceAvailable());
	if(env->isServiceAvailable())
	{
		CommonAPI::CallStatus callStatus = CommonAPI::CallStatus::NOT_AVAILABLE;
		am_types::am_Error_e error = am_types::am_Error_e::E_UNKNOWN;
		std::string  name("name");
		am_types::am_sinkID_t sinkID = 0;
		EXPECT_CALL(*env->mpRoutingReceive, peekSink(_, _)).WillOnce(DoAll(actionPeek(), Return(E_OK)));
		env->mProxy->peekSink(name, callStatus, sinkID, error);
		ASSERT_EQ( sinkID, TEST_ID_1 );
		ASSERT_EQ((int)error, am_types::am_Error_e::E_OK);
		ASSERT_EQ( callStatus, CommonAPI::CallStatus::SUCCESS );
	}

}


TEST_F(CAmRoutingInterfaceCAPITests, registerSink)
{
	ASSERT_TRUE(env->isServiceAvailable());
	if(env->isServiceAvailable())
	{
		CommonAPI::CallStatus callStatus = CommonAPI::CallStatus::NOT_AVAILABLE;
		am_types::am_Error_e error = am_types::am_Error_e::E_UNKNOWN;

		am_types::am_Sink_s sink;
		am_Sink_s amSink;
		am_types::am_sinkID_t sinkID = 0;
		initSink(sink, amSink, TEST_ID_2, sinkID);

		EXPECT_CALL(*env->mpRoutingReceive, registerSink(IsSinkDataEqualTo(amSink), _)).WillOnce(DoAll(actionRegister(), Return(E_OK)));
		env->mProxy->registerSink(sink, callStatus, sinkID, error);
		ASSERT_EQ( sinkID, TEST_ID_1 );
		ASSERT_EQ((int)error, am_types::am_Error_e::E_OK);
		ASSERT_EQ( callStatus, CommonAPI::CallStatus::SUCCESS );
	}

}

TEST_F(CAmRoutingInterfaceCAPITests, deregisterSink)
{
	ASSERT_TRUE(env->isServiceAvailable());
	if(env->isServiceAvailable())
	{
		CommonAPI::CallStatus callStatus = CommonAPI::CallStatus::NOT_AVAILABLE;
		am_types::am_Error_e error = am_types::am_Error_e::E_UNKNOWN;
		am_types::am_sinkID_t sinkID = TEST_ID_1;

		EXPECT_CALL(*env->mpRoutingReceive, deregisterSink(sinkID)).WillOnce(Return(E_OK));
		env->mProxy->deregisterSink(sinkID, callStatus, error);
		ASSERT_EQ((int)error, am_types::am_Error_e::E_OK);
		ASSERT_EQ( callStatus, CommonAPI::CallStatus::SUCCESS );
	}

}

TEST_F(CAmRoutingInterfaceCAPITests, peekSource)
{
	ASSERT_TRUE(env->isServiceAvailable());
	if(env->isServiceAvailable())
	{
		CommonAPI::CallStatus callStatus = CommonAPI::CallStatus::NOT_AVAILABLE;
		am_types::am_Error_e error = am_types::am_Error_e::E_UNKNOWN;
		std::string  name("name");
		am_types::am_sourceID_t sinkID = 0;
		EXPECT_CALL(*env->mpRoutingReceive, peekSource(_, _)).WillOnce(DoAll(actionPeek(), Return(E_OK)));
		env->mProxy->peekSource(name, callStatus, sinkID, error);
		ASSERT_EQ( sinkID, TEST_ID_1 );
		ASSERT_EQ((int)error, am_types::am_Error_e::E_OK);
		ASSERT_EQ( callStatus, CommonAPI::CallStatus::SUCCESS );
	}

}

TEST_F(CAmRoutingInterfaceCAPITests, registerSource)
{
	ASSERT_TRUE(env->isServiceAvailable());
	if(env->isServiceAvailable())
	{
		CommonAPI::CallStatus callStatus = CommonAPI::CallStatus::NOT_AVAILABLE;
		am_types::am_Error_e error = am_types::am_Error_e::E_UNKNOWN;

		am_types::am_Source_s source;
		am_Source_s amSource;
		am_types::am_sourceID_t sourceID = 0;
		initSource(source, amSource, TEST_ID_2, sourceID);

		EXPECT_CALL(*env->mpRoutingReceive, registerSource(IsSourceDataEqualTo(amSource), _)).WillOnce(DoAll(actionRegister(), Return(E_OK)));
		env->mProxy->registerSource(source, callStatus, sourceID, error);
		ASSERT_EQ( sourceID, TEST_ID_1 );
		ASSERT_EQ((int)error, am_types::am_Error_e::E_OK);
		ASSERT_EQ( callStatus, CommonAPI::CallStatus::SUCCESS );
	}

}

TEST_F(CAmRoutingInterfaceCAPITests, deregisterSource)
{
	ASSERT_TRUE(env->isServiceAvailable());
	if(env->isServiceAvailable())
	{
		CommonAPI::CallStatus callStatus = CommonAPI::CallStatus::NOT_AVAILABLE;
		am_types::am_Error_e error = am_types::am_Error_e::E_UNKNOWN;
		am_types::am_sourceID_t sinkID = TEST_ID_1;

		EXPECT_CALL(*env->mpRoutingReceive, deregisterSource(sinkID)).WillOnce(Return(E_OK));
		env->mProxy->deregisterSource(sinkID, callStatus, error);
		ASSERT_EQ((int)error, am_types::am_Error_e::E_OK);
		ASSERT_EQ( callStatus, CommonAPI::CallStatus::SUCCESS );
	}

}

TEST_F(CAmRoutingInterfaceCAPITests, registerCrossfader)
{
	ASSERT_TRUE(env->isServiceAvailable());
	if(env->isServiceAvailable())
	{
		CommonAPI::CallStatus callStatus = CommonAPI::CallStatus::NOT_AVAILABLE;
		am_types::am_Error_e error = am_types::am_Error_e::E_UNKNOWN;

		am_types::am_crossfaderID_t crossfaderID = 0;
		am_types::am_Crossfader_s crossfader;
		am_Crossfader_s amCrossfader;
		initCrossfader(crossfader, amCrossfader, crossfaderID);

		EXPECT_CALL(*env->mpRoutingReceive, registerCrossfader(IsCrossfaderDataEqualTo(amCrossfader), _)).WillOnce(DoAll(actionRegister(), Return(E_OK)));
		env->mProxy->registerCrossfader(crossfader, callStatus, crossfaderID, error);
		ASSERT_EQ( crossfaderID, TEST_ID_1 );
		ASSERT_EQ((int)error, am_types::am_Error_e::E_OK);
		ASSERT_EQ( callStatus, CommonAPI::CallStatus::SUCCESS );
	}

}

TEST_F(CAmRoutingInterfaceCAPITests, deregisterCrossfader)
{
	ASSERT_TRUE(env->isServiceAvailable());
	if(env->isServiceAvailable())
	{
		CommonAPI::CallStatus callStatus = CommonAPI::CallStatus::NOT_AVAILABLE;
		am_types::am_Error_e error = am_types::am_Error_e::E_UNKNOWN;
		am_types::am_crossfaderID_t crossfaderID = TEST_ID_1;

		EXPECT_CALL(*env->mpRoutingReceive, deregisterCrossfader(crossfaderID)).WillOnce(Return(E_OK));
		env->mProxy->deregisterCrossfader(crossfaderID, callStatus, error);
		ASSERT_EQ((int)error, am_types::am_Error_e::E_OK);
		ASSERT_EQ( callStatus, CommonAPI::CallStatus::SUCCESS );
	}

}

TEST_F(CAmRoutingInterfaceCAPITests, peekSourceClassID)
{
	ASSERT_TRUE(env->isServiceAvailable());
	if(env->isServiceAvailable())
	{
		CommonAPI::CallStatus callStatus = CommonAPI::CallStatus::NOT_AVAILABLE;
		am_types::am_Error_e error = am_types::am_Error_e::E_UNKNOWN;
		std::string  name("name");
		am_types::am_sourceClass_t sinkID = 0;
		ON_CALL(*env->mpRoutingReceive, peekSourceClassID(_, _)).WillByDefault(DoAll(actionPeekUnexpected(), Return(E_OK)));
		EXPECT_CALL(*env->mpRoutingReceive, peekSourceClassID(name, _)).WillOnce(DoAll(actionPeek(), Return(E_OK)));
		env->mProxy->peekSourceClassID(name, callStatus, sinkID, error);
		ASSERT_EQ( sinkID, TEST_ID_1 );
		ASSERT_EQ((int)error, am_types::am_Error_e::E_OK);
		ASSERT_EQ( callStatus, CommonAPI::CallStatus::SUCCESS );
	}

}

TEST_F(CAmRoutingInterfaceCAPITests, peekSinkClassID)
{
	ASSERT_TRUE(env->isServiceAvailable());
	if(env->isServiceAvailable())
	{
		CommonAPI::CallStatus callStatus = CommonAPI::CallStatus::NOT_AVAILABLE;
		am_types::am_Error_e error = am_types::am_Error_e::E_UNKNOWN;
		std::string  name("name");
		am_types::am_sinkClass_t sinkID = 0;
		ON_CALL(*env->mpRoutingReceive, peekSinkClassID(_, _)).WillByDefault(DoAll(actionPeekUnexpected(), Return(E_OK)));
		EXPECT_CALL(*env->mpRoutingReceive, peekSinkClassID(name, _)).WillOnce(DoAll(actionPeek(), Return(E_OK)));
		env->mProxy->peekSinkClassID(name, callStatus, sinkID, error);
		ASSERT_EQ( sinkID, TEST_ID_1 );
		ASSERT_EQ((int)error, am_types::am_Error_e::E_OK);
		ASSERT_EQ( callStatus, CommonAPI::CallStatus::SUCCESS );
	}
}

TEST_F(CAmRoutingInterfaceCAPITests, hookInterruptStatusChange)
{
	ASSERT_TRUE(env->isServiceAvailable());
	if(env->isServiceAvailable())
	{
		CommonAPI::CallStatus callStatus = CommonAPI::CallStatus::NOT_AVAILABLE;
		am_Error_e error = E_OK;
		am_types::am_sourceID_t sourceID = TEST_ID_1;
		am_InterruptState_e interruptState = am_InterruptState_e::IS_UNKNOWN;
		am_types::am_InterruptState_e CAPIInterruptState=static_cast<am_types::am_InterruptState_e::Literal>(interruptState);
		EXPECT_CALL(*env->mpRoutingReceive, hookInterruptStatusChange(sourceID, interruptState)).Times(1);
		env->mProxy->hookInterruptStatusChange(sourceID, CAPIInterruptState, callStatus);
		ASSERT_EQ( callStatus, CommonAPI::CallStatus::SUCCESS );
	}

}

TEST_F(CAmRoutingInterfaceCAPITests, hookDomainRegistrationComplete)
{
	ASSERT_TRUE(env->isServiceAvailable());
	if(env->isServiceAvailable())
	{
		CommonAPI::CallStatus callStatus = CommonAPI::CallStatus::NOT_AVAILABLE;
		am_Error_e error = E_OK;
		am_types::am_domainID_t testID = TEST_ID_1;
		EXPECT_CALL(*env->mpRoutingReceive, hookDomainRegistrationComplete(testID)).Times(1);
		env->mProxy->hookDomainRegistrationComplete(testID, callStatus);
		ASSERT_EQ( callStatus, CommonAPI::CallStatus::SUCCESS );
	}

}

MATCHER_P(IsAvailabilityEqualTo, value, "") {
	am_Availability_s lh = arg;
	return lh.availability == value.availability &&
			lh.availabilityReason == value.availabilityReason;
}

TEST_F(CAmRoutingInterfaceCAPITests, hookSinkAvailablityStatusChange)
{
	ASSERT_TRUE(env->isServiceAvailable());
	if(env->isServiceAvailable())
	{
		CommonAPI::CallStatus callStatus = CommonAPI::CallStatus::NOT_AVAILABLE;
		am_Error_e error = E_OK;
		am_types::am_sinkID_t testID = TEST_ID_1;

		am_types::am_Availability_s available(am_types::am_Availability_e::A_UNKNOWN, AR_UNKNOWN);
		am_Availability_s amAvailable;
		convert_am_types(available, amAvailable);

		EXPECT_CALL(*env->mpRoutingReceive, hookSinkAvailablityStatusChange(testID, IsAvailabilityEqualTo(amAvailable))).Times(1);
		env->mProxy->hookSinkAvailablityStatusChange(testID, available, callStatus);
		ASSERT_EQ( callStatus, CommonAPI::CallStatus::SUCCESS );
	}

}

TEST_F(CAmRoutingInterfaceCAPITests, hookSourceAvailablityStatusChange)
{
	ASSERT_TRUE(env->isServiceAvailable());
	if(env->isServiceAvailable())
	{
		CommonAPI::CallStatus callStatus = CommonAPI::CallStatus::NOT_AVAILABLE;
		am_Error_e error = E_OK;
		am_types::am_sourceID_t testID = TEST_ID_1;

		am_types::am_Availability_s available(am_types::am_Availability_e::A_UNKNOWN, AR_UNKNOWN);
		am_Availability_s amAvailable;
		convert_am_types(available, amAvailable);

		EXPECT_CALL(*env->mpRoutingReceive, hookSourceAvailablityStatusChange(testID, IsAvailabilityEqualTo(amAvailable))).Times(1);
		env->mProxy->hookSourceAvailablityStatusChange(testID, available, callStatus);
		ASSERT_EQ( callStatus, CommonAPI::CallStatus::SUCCESS );
	}

}

TEST_F(CAmRoutingInterfaceCAPITests, hookDomainStateChange)
{
	ASSERT_TRUE(env->isServiceAvailable());
	if(env->isServiceAvailable())
	{
		CommonAPI::CallStatus callStatus = CommonAPI::CallStatus::NOT_AVAILABLE;
		am_Error_e error = E_OK;
		am_types::am_domainID_t testID = TEST_ID_1;
		am_types::am_DomainState_e domainState(am_types::am_DomainState_e::DS_CONTROLLED);
		EXPECT_CALL(*env->mpRoutingReceive, hookDomainStateChange(testID, am_DomainState_e::DS_CONTROLLED)).Times(1);
		env->mProxy->hookDomainStateChange(testID, domainState, callStatus);
		ASSERT_EQ( callStatus, CommonAPI::CallStatus::SUCCESS );
	}

}

TEST_F(CAmRoutingInterfaceCAPITests, hookTimingInformationChanged)
{
	ASSERT_TRUE(env->isServiceAvailable());
	if(env->isServiceAvailable())
	{
		CommonAPI::CallStatus callStatus = CommonAPI::CallStatus::NOT_AVAILABLE;
		am_Error_e error = E_OK;
		am_types::am_connectionID_t testID = TEST_ID_1;
		int16_t delay = 10;
		am_types::am_DomainState_e domainState = am_types::am_DomainState_e::DS_CONTROLLED;
		EXPECT_CALL(*env->mpRoutingReceive, hookTimingInformationChanged(testID, delay)).Times(1);
		env->mProxy->hookTimingInformationChanged(testID, delay, callStatus);
		ASSERT_EQ( callStatus, CommonAPI::CallStatus::SUCCESS );
	}

}

MATCHER_P(IsEarlyDataEqualTo, value, "") {

	std::vector<am_EarlyData_s> lh = arg;
	bool result = lh.size() == value.size();
	for(int i=0; result && i<lh.size(); i++)
	{
		am_EarlyData_s ed_lh = lh.at(i);
		am_types::am_EarlyData_s ed_rh = value.at(i);
		if(ed_lh.type != (int)(ed_rh.getType()))
		{
			result = false;
			break;
		}
		else
		{
			if(ed_lh.type==ED_SINK_VOLUME)
			{
				result &= ed_lh.data.volume == ed_rh.getData().get<am_types::am_volume_t>();
				result &= ed_lh.sinksource.sink == ed_rh.getSinksource();
			}
			else if(ed_lh.type==ED_SINK_PROPERTY)
			{
				am_types::am_SoundProperty_s soundproperty=ed_rh.getData().get<am_types::am_SoundProperty_s>();
				result &= ed_lh.data.soundProperty.type == soundproperty.getType();
				result &= ed_lh.data.soundProperty.value == soundproperty.getValue();
				result &= ed_lh.sinksource.sink == ed_rh.getSinksource();
			}
			else if(ed_lh.type==ED_SOURCE_VOLUME)
			{
				result &= ed_lh.data.volume == ed_rh.getData().get<am_types::am_volume_t>();
				result &= ed_lh.sinksource.source == ed_rh.getSinksource();
			}
			else if(ed_lh.type==ED_SOURCE_PROPERTY)
			{
				am_types::am_SoundProperty_s soundproperty=ed_rh.getData().get<am_types::am_SoundProperty_s>();
				result &= ed_lh.data.soundProperty.type == soundproperty.getType();
				result &= ed_lh.data.soundProperty.value == soundproperty.getValue();
				result &= ed_lh.sinksource.source == ed_rh.getSinksource();
			}
			else
			{
				result = false;
				break;
			}
		}
	}

	return result;
}

TEST_F(CAmRoutingInterfaceCAPITests, sendChangedData)
{
	ASSERT_TRUE(env->isServiceAvailable());
	if(env->isServiceAvailable())
	{
		CommonAPI::CallStatus callStatus = CommonAPI::CallStatus::NOT_AVAILABLE;
		am_Error_e error = E_OK;
		am_types::am_connectionID_t testID = TEST_ID_1;
		am_types::am_EarlyData_L earlyData;

		am_types::am_SinkSourceID_t dt11(103);
		am_types::am_EarlyData_u ed11(static_cast<am_types::am_volume_t>(50));
		earlyData.push_back(am_types::am_EarlyData_s(ed11,
											dt11,
											am_types::am_EarlyDataType_e::ED_SINK_VOLUME));
		am_types::am_SinkSourceID_t dt12(104);
		earlyData.push_back(am_types::am_EarlyData_s(ed11,
											dt12,
											am_types::am_EarlyDataType_e::ED_SINK_VOLUME));

		am_types::am_SinkSourceID_t dt13(105);
		am_types::am_EarlyData_u ed12(am_types::am_SoundProperty_s(SP_UNKNOWN, 50));
		earlyData.push_back(am_types::am_EarlyData_s(ed12,
											dt13,
											am_types::am_EarlyDataType_e::ED_SINK_PROPERTY));
		am_types::am_SinkSourceID_t dt14(106);
		earlyData.push_back(am_types::am_EarlyData_s(ed12,
											dt14,
											am_types::am_EarlyDataType_e::ED_SINK_PROPERTY));

		EXPECT_CALL(*env->mpRoutingReceive, sendChangedData(IsEarlyDataEqualTo(earlyData))).Times(1);

		env->mProxy->sendChangedData(earlyData, callStatus);
	}

}

TEST_F(CAmRoutingInterfaceCAPITests, updateGateway)
{
	ASSERT_TRUE(env->isServiceAvailable());
	if(env->isServiceAvailable())
	{
		CommonAPI::CallStatus callStatus = CommonAPI::CallStatus::NOT_AVAILABLE;
		am_Error_e error = E_OK;
		am_types::am_gatewayID_t testID = TEST_ID_1;

		am_types::am_ConnectionFormat_L listSourceFormats;
		listSourceFormats.push_back(static_cast<am_types::am_ConnectionFormat_pe>(CF_GENIVI_ANALOG));
		listSourceFormats.push_back(static_cast<am_types::am_ConnectionFormat_pe>(CF_GENIVI_AUTO));

		am_types::am_ConnectionFormat_L listSinkFormats;
		listSinkFormats.push_back(static_cast<am_types::am_ConnectionFormat_pe>(CF_GENIVI_AUTO));
		listSinkFormats.push_back(static_cast<am_types::am_ConnectionFormat_pe>(CF_GENIVI_ANALOG));

		am_types::am_Convertion_L convertionMatrix;
		convertionMatrix.push_back(1);
		convertionMatrix.push_back(0);

		std::vector<am_CustomConnectionFormat_t> am_listSourceFormats;
		am_listSourceFormats.push_back(CF_GENIVI_ANALOG);
		am_listSourceFormats.push_back(CF_GENIVI_AUTO);

		std::vector<am_CustomConnectionFormat_t> am_listSinkFormats;
		am_listSinkFormats.push_back(CF_GENIVI_AUTO);
		am_listSinkFormats.push_back(CF_GENIVI_ANALOG);

		std::vector<bool> am_convertionMatrix;
		am_convertionMatrix.push_back(1);
		am_convertionMatrix.push_back(0);
		EXPECT_CALL(*env->mpRoutingReceive, updateGateway(testID, am_listSourceFormats, am_listSinkFormats, am_convertionMatrix)).WillOnce(Return(E_OK));

		am_types::am_Error_e CAPIError;
		env->mProxy->updateGateway(testID, listSourceFormats, listSinkFormats, convertionMatrix, callStatus,CAPIError);
		ASSERT_EQ( callStatus, CommonAPI::CallStatus::SUCCESS );
	}

}

TEST_F(CAmRoutingInterfaceCAPITests, updateConverter)
{
	ASSERT_TRUE(env->isServiceAvailable());
	if(env->isServiceAvailable())
	{
		CommonAPI::CallStatus callStatus = CommonAPI::CallStatus::NOT_AVAILABLE;
		am_Error_e error = E_OK;
		am_types::am_converterID_t testID = TEST_ID_1;

		am_types::am_ConnectionFormat_L listSourceFormats;
		listSourceFormats.push_back(static_cast<am_types::am_ConnectionFormat_pe>(CF_GENIVI_ANALOG));
		listSourceFormats.push_back(static_cast<am_types::am_ConnectionFormat_pe>(CF_GENIVI_AUTO));

		am_types::am_ConnectionFormat_L listSinkFormats;
		listSinkFormats.push_back(static_cast<am_types::am_ConnectionFormat_pe>(CF_GENIVI_AUTO));
		listSinkFormats.push_back(static_cast<am_types::am_ConnectionFormat_pe>(CF_GENIVI_ANALOG));

		am_types::am_Convertion_L convertionMatrix;
		convertionMatrix.push_back(1);
		convertionMatrix.push_back(0);

		std::vector<am_CustomConnectionFormat_t> am_listSourceFormats;
		am_listSourceFormats.push_back(CF_GENIVI_ANALOG);
		am_listSourceFormats.push_back(CF_GENIVI_AUTO);

		std::vector<am_CustomConnectionFormat_t> am_listSinkFormats;
		am_listSinkFormats.push_back(CF_GENIVI_AUTO);
		am_listSinkFormats.push_back(CF_GENIVI_ANALOG);

		std::vector<bool> am_convertionMatrix;
		am_convertionMatrix.push_back(1);
		am_convertionMatrix.push_back(0);
		EXPECT_CALL(*env->mpRoutingReceive, updateConverter(testID, am_listSourceFormats, am_listSinkFormats, am_convertionMatrix)).WillOnce(Return(E_OK));

		am_types::am_Error_e CAPIError;
		env->mProxy->updateConverter(testID, listSourceFormats, listSinkFormats, convertionMatrix, callStatus,CAPIError);
		ASSERT_EQ( callStatus, CommonAPI::CallStatus::SUCCESS );
	}

}

MATCHER_P(IsSoundPropertiesArrayEqualTo, value, "") {
	auto lh = arg;

	bool bSoundProperties = lh.size() == value.size();
	for(int i=0; i<lh.size(); i++)
	{
		bSoundProperties &= (lh.at(i).type==value.at(i).type &&
								 lh.at(i).value==value.at(i).value);
	}
	return bSoundProperties;
}

void prepareArrays(	am_types::am_SoundProperty_L & listSoundProperties,
						am_types::am_ConnectionFormat_L & listSinkFormats,
						am_types::am_MainSoundProperty_L & listMainSoundProperties,
						std::vector<am_CustomConnectionFormat_t> & am_listSinkFormats,
						std::vector<am_SoundProperty_s> & am_listSoundProperties,
						std::vector<am_MainSoundProperty_s> & am_listMainSoundProperties)
{
	listSoundProperties.push_back(am_types::am_SoundProperty_s(SP_UNKNOWN, 50));
	listSoundProperties.push_back(am_types::am_SoundProperty_s(SP_UNKNOWN, 51));

	listSinkFormats.push_back(static_cast<am_types::am_ConnectionFormat_pe>(CF_GENIVI_AUTO));
	listSinkFormats.push_back(static_cast<am_types::am_ConnectionFormat_pe>(CF_GENIVI_ANALOG));

	listMainSoundProperties.push_back(am_types::am_MainSoundProperty_s(MSP_UNKNOWN, 50));
	listMainSoundProperties.push_back(am_types::am_MainSoundProperty_s(MSP_UNKNOWN, 51));

	am_listSinkFormats.push_back(CF_GENIVI_AUTO);
	am_listSinkFormats.push_back(CF_GENIVI_ANALOG);

	am_listSoundProperties.push_back(am_SoundProperty_s{SP_UNKNOWN, 50});
	am_listSoundProperties.push_back(am_SoundProperty_s{SP_UNKNOWN, 51});

	am_listMainSoundProperties.push_back(am_MainSoundProperty_s{MSP_UNKNOWN, 50});
	am_listMainSoundProperties.push_back(am_MainSoundProperty_s{MSP_UNKNOWN, 51});
}

TEST_F(CAmRoutingInterfaceCAPITests, updateSink)
{
	ASSERT_TRUE(env->isServiceAvailable());
	if(env->isServiceAvailable())
	{
		CommonAPI::CallStatus callStatus = CommonAPI::CallStatus::NOT_AVAILABLE;
		am_Error_e error = E_OK;
		am_types::am_sinkID_t testID = TEST_ID_1;

		am_types::am_SoundProperty_L listSoundProperties;
		am_types::am_ConnectionFormat_L listSinkFormats;
		am_types::am_MainSoundProperty_L listMainSoundProperties;
		std::vector<am_CustomConnectionFormat_t> am_listSinkFormats;
		std::vector<am_SoundProperty_s> am_listSoundProperties;
		std::vector<am_MainSoundProperty_s> am_listMainSoundProperties;
		prepareArrays(listSoundProperties, listSinkFormats, listMainSoundProperties, am_listSinkFormats, am_listSoundProperties, am_listMainSoundProperties);

		EXPECT_CALL(*env->mpRoutingReceive, updateSink(testID, TEST_ID_2, IsSoundPropertiesArrayEqualTo(am_listSoundProperties), am_listSinkFormats, IsSoundPropertiesArrayEqualTo(am_listMainSoundProperties))).WillOnce(Return(E_OK));
		am_types::am_Error_e CAPIError;
		env->mProxy->updateSink(testID, TEST_ID_2, listSoundProperties, listSinkFormats, listMainSoundProperties, callStatus,CAPIError);
		ASSERT_EQ( callStatus, CommonAPI::CallStatus::SUCCESS );
	}

}

TEST_F(CAmRoutingInterfaceCAPITests, updateSource)
{
	ASSERT_TRUE(env->isServiceAvailable());
	if(env->isServiceAvailable())
	{
		CommonAPI::CallStatus callStatus = CommonAPI::CallStatus::NOT_AVAILABLE;
		am_Error_e error = E_OK;
		am_types::am_sourceID_t testID = TEST_ID_1;

		am_types::am_SoundProperty_L listSoundProperties;
		am_types::am_ConnectionFormat_L listSinkFormats;
		am_types::am_MainSoundProperty_L listMainSoundProperties;
		std::vector<am_CustomConnectionFormat_t> am_listSinkFormats;
		std::vector<am_SoundProperty_s> am_listSoundProperties;
		std::vector<am_MainSoundProperty_s> am_listMainSoundProperties;
		prepareArrays(listSoundProperties, listSinkFormats, listMainSoundProperties, am_listSinkFormats, am_listSoundProperties, am_listMainSoundProperties);

		EXPECT_CALL(*env->mpRoutingReceive, updateSource(testID, TEST_ID_2, IsSoundPropertiesArrayEqualTo(am_listSoundProperties), am_listSinkFormats, IsSoundPropertiesArrayEqualTo(am_listMainSoundProperties))).WillOnce(Return(E_OK));
		am_types::am_Error_e CAPIError;
		env->mProxy->updateSource(testID, TEST_ID_2, listSoundProperties, listSinkFormats, listMainSoundProperties, callStatus,CAPIError);
		ASSERT_EQ( callStatus, CommonAPI::CallStatus::SUCCESS );
	}

}



MATCHER_P(IsPayloadEqualTo, value,  "") {

	auto lh = arg;
	return lh.type == value.type && lh.value == value.value;
}


TEST_F(CAmRoutingInterfaceCAPITests, hookSinkNotificationDataChange)
{
	ASSERT_TRUE(env->isServiceAvailable());
	if(env->isServiceAvailable())
	{
		CommonAPI::CallStatus callStatus = CommonAPI::CallStatus::NOT_AVAILABLE;
		am_Error_e error = E_OK;
		am_types::am_sinkID_t testID = TEST_ID_1;
		am_types::am_NotificationPayload_s payload(NT_UNKNOWN, 50);

		am_NotificationPayload_s am_payload = am_NotificationPayload_s{NT_UNKNOWN, 50};

		EXPECT_CALL(*env->mpRoutingReceive, hookSinkNotificationDataChange(testID, IsPayloadEqualTo(am_payload))).Times(1);
		env->mProxy->hookSinkNotificationDataChange(testID, payload, callStatus);
		ASSERT_EQ( callStatus, CommonAPI::CallStatus::SUCCESS );
	}

}

TEST_F(CAmRoutingInterfaceCAPITests, hookSourceNotificationDataChange)
{
	ASSERT_TRUE(env->isServiceAvailable());
	if(env->isServiceAvailable())
	{
		CommonAPI::CallStatus callStatus = CommonAPI::CallStatus::NOT_AVAILABLE;
		am_Error_e error = E_OK;
		am_types::am_sourceID_t testID = TEST_ID_1;
		am_types::am_NotificationPayload_s payload(NT_UNKNOWN, 50);

		am_NotificationPayload_s am_payload = am_NotificationPayload_s{NT_UNKNOWN, 50};

		EXPECT_CALL(*env->mpRoutingReceive, hookSourceNotificationDataChange(testID, IsPayloadEqualTo(am_payload))).Times(1);
		env->mProxy->hookSourceNotificationDataChange(testID, payload, callStatus);
		ASSERT_EQ( callStatus, CommonAPI::CallStatus::SUCCESS );
	}

}



// The following methods must be executed in the given order.
// It is important that the TestDomain_register* methods are executed first because they make domain, sink, source and crossfader registrations.
// The last couple of methods are TestDomain_deregister* methods which check whether the 'deregistration' functionality works properly.

TEST_F(CAmRoutingInterfaceCAPITests, TestDomain_registerSource)
{
	ASSERT_TRUE(env->isServiceAvailable());
	if(env->isServiceAvailable())
	{
		const am_types::am_domainID_t domainID = TEST_ID_1;
		IAmRoutingSenderBackdoor backdoor(env->mpPlugin);
		ASSERT_TRUE( backdoor.containsDomainWithID( (const am_domainID_t)domainID) );

		CommonAPI::CallStatus callStatus = CommonAPI::CallStatus::NOT_AVAILABLE;
		am_types::am_Error_e error = am_types::am_Error_e::E_UNKNOWN;

		am_types::am_Source_s source;
		am_Source_s amSource;
		am_types::am_sourceID_t sinkID = 0;
		initSource(source, amSource, domainID, sinkID);

		EXPECT_CALL(*env->mpRoutingReceive, registerSource(IsSourceDataEqualTo(amSource), _)).WillOnce(DoAll(actionRegister(), Return(E_OK)));
		env->mProxy->registerSource(source, callStatus, sinkID, error);
		usleep(50000);
		ASSERT_EQ( sinkID, TEST_ID_1 );
		ASSERT_EQ((int)error, am_types::am_Error_e::E_OK);
		ASSERT_EQ( callStatus, CommonAPI::CallStatus::SUCCESS );
		ASSERT_TRUE( backdoor.containsSourceWithID( (const am_sourceID_t)TEST_ID_1) );

		initSource(source, amSource, domainID, sinkID);
		EXPECT_CALL(*env->mpRoutingReceive, registerSource(IsSourceDataEqualTo(amSource), _)).WillOnce(DoAll(actionRegister2(), Return(E_OK)));
		env->mProxy->registerSource(source, callStatus, sinkID, error);
		usleep(50000);
		ASSERT_EQ( callStatus, CommonAPI::CallStatus::SUCCESS );
		ASSERT_EQ((int)error, am_types::am_Error_e::E_OK);
		ASSERT_EQ( sinkID, TEST_ID_2 );
		ASSERT_TRUE( backdoor.containsSourceWithID( (const am_sourceID_t)TEST_ID_2) );
		ASSERT_TRUE( backdoor.sourcesCount()==2 );
	}
}

TEST_F(CAmRoutingInterfaceCAPITests, TestDomain_registerSink)
{
	ASSERT_TRUE(env->isServiceAvailable());
	if(env->isServiceAvailable())
	{
		const am_types::am_domainID_t domainID = TEST_ID_1;
		IAmRoutingSenderBackdoor backdoor(env->mpPlugin);
		ASSERT_TRUE( backdoor.containsDomainWithID( (const am_domainID_t)domainID) );

		CommonAPI::CallStatus callStatus = CommonAPI::CallStatus::NOT_AVAILABLE;
		am_types::am_Error_e error = am_types::am_Error_e::E_UNKNOWN;

		am_types::am_Sink_s sink;
		am_Sink_s amSink;
		am_types::am_sinkID_t sinkID = 0;
		initSink(sink, amSink, domainID, sinkID);

		EXPECT_CALL(*env->mpRoutingReceive, registerSink(IsSinkDataEqualTo(amSink), _)).WillOnce(DoAll(actionRegister(), Return(E_OK)));
		env->mProxy->registerSink(sink, callStatus, sinkID, error);

		usleep(50000);
		ASSERT_EQ( callStatus, CommonAPI::CallStatus::SUCCESS );
		ASSERT_EQ((int)error, am_types::am_Error_e::E_OK);
		ASSERT_EQ( sinkID, TEST_ID_1 );
		ASSERT_TRUE( backdoor.containsSinkWithID( (const am_sourceID_t)TEST_ID_1) );

		initSink(sink, amSink, TEST_ID_1, 0);
		EXPECT_CALL(*env->mpRoutingReceive, registerSink(IsSinkDataEqualTo(amSink), _)).WillOnce(DoAll(actionRegister2(), Return(E_OK)));
		env->mProxy->registerSink(sink, callStatus, sinkID, error);

		usleep(50000);
		ASSERT_EQ( callStatus, CommonAPI::CallStatus::SUCCESS );
		ASSERT_EQ((int)error, am_types::am_Error_e::E_OK);
		ASSERT_EQ( sinkID, TEST_ID_2 );
		ASSERT_TRUE( backdoor.containsSinkWithID( (const am_sourceID_t)TEST_ID_2) );
		ASSERT_TRUE( backdoor.sinksCount()==2 );
	}
}

TEST_F(CAmRoutingInterfaceCAPITests, TestDomain_setDomainState)
{
//todo: Decide how to test
}

TEST_F(CAmRoutingInterfaceCAPITests, TestDomain_asyncSetSourceState)
{
	ASSERT_TRUE(env->isServiceAvailable());
	if(env->isServiceAvailable())
	{
		const am_types::am_domainID_t domainID = TEST_ID_1;
		const am_types::am_sourceID_t sID = TEST_ID_1;
		IAmRoutingSenderBackdoor backdoor(env->mpPlugin);
		ASSERT_TRUE( backdoor.containsDomainWithID( (const am_domainID_t)domainID) );
		ASSERT_TRUE( backdoor.containsSourceWithID( (const am_sourceID_t)sID) );

		am_volume_t volume = 50;
		am_Handle_s handle = {H_SETSOURCESTATE, 20};
		am_SourceState_e state = am_SourceState_e::SS_OFF;
		EXPECT_CALL(*env->mpRoutingReceive, ackSetSourceState(IsHandleEqual(handle), E_OK)).Times(1);
		EXPECT_CALL(*env->mpRoutingReceive, getDomainOfSource(TEST_ID_1, _)).WillOnce(Return(E_OK));
		am_Error_e error = env->mpPlugin->asyncSetSourceState(handle, sID, state);
		usleep(1000000);
		ASSERT_EQ( error, E_OK );
		ASSERT_FALSE( backdoor.containsHandle( handle.handle) );
	}
}

TEST_F(CAmRoutingInterfaceCAPITests, TestDomain_asyncSetSourceVolume)
{
	ASSERT_TRUE(env->isServiceAvailable());
	if(env->isServiceAvailable())
	{
		const am_types::am_domainID_t domainID = TEST_ID_1;
		const am_types::am_sourceID_t sID = TEST_ID_1;
		IAmRoutingSenderBackdoor backdoor(env->mpPlugin);
		ASSERT_TRUE( backdoor.containsDomainWithID( (const am_domainID_t)domainID) );
		ASSERT_TRUE( backdoor.containsSourceWithID( (const am_sourceID_t)sID) );

		am_volume_t volume = 50;
		am_time_t time = 10;
		am_CustomRampType_t ramp = RAMP_GENIVI_DIRECT;
		am_Handle_s handle = {H_SETSOURCEVOLUME, 20};
		am_SourceState_e state = am_SourceState_e::SS_OFF;
		EXPECT_CALL(*env->mpRoutingReceive, ackSetSourceVolumeChange(IsHandleEqual(handle), volume, E_OK)).Times(1);
		EXPECT_CALL(*env->mpRoutingReceive, getDomainOfSource(TEST_ID_1, _)).WillOnce(Return(E_OK));
		am_Error_e error = env->mpPlugin->asyncSetSourceVolume(handle, sID, volume, ramp, time);
		usleep(50000);
		ASSERT_EQ( error, E_OK );
		ASSERT_FALSE( backdoor.containsHandle( handle.handle) );
	}
}

TEST_F(CAmRoutingInterfaceCAPITests, TestDomain_registerCrossfader)
{
	ASSERT_TRUE(env->isServiceAvailable());
	if(env->isServiceAvailable())
	{
		const am_types::am_domainID_t domainID = TEST_ID_1;
		IAmRoutingSenderBackdoor backdoor(env->mpPlugin);
		ASSERT_TRUE( backdoor.containsDomainWithID( (const am_domainID_t)domainID) );

		CommonAPI::CallStatus callStatus = CommonAPI::CallStatus::NOT_AVAILABLE;
		am_types::am_Error_e error = am_types::am_Error_e::E_UNKNOWN;

		am_types::am_crossfaderID_t crossfaderID = 0;
		am_types::am_Crossfader_s crossfaderData;
		am_Crossfader_s amCrossfaderData;
		initCrossfader(crossfaderData, amCrossfaderData, crossfaderID);

		EXPECT_CALL(*env->mpRoutingReceive, registerCrossfader(IsCrossfaderDataEqualTo(amCrossfaderData), _)).WillOnce(DoAll(actionRegister(), Return(E_OK)));
		env->mProxy->registerCrossfader(crossfaderData, callStatus, crossfaderID, error);

		usleep(50000);
		ASSERT_EQ( callStatus, CommonAPI::CallStatus::SUCCESS );
		ASSERT_EQ((int)error, am_types::am_Error_e::E_OK);
		ASSERT_EQ( crossfaderID, TEST_ID_1 );
		ASSERT_TRUE( backdoor.containsCrossfader( TEST_ID_1) );
	}
}

TEST_F(CAmRoutingInterfaceCAPITests, TestDomain_asyncSetSinkVolume)
{
	ASSERT_TRUE(env->isServiceAvailable());
	if(env->isServiceAvailable())
	{
		const am_types::am_domainID_t domainID = TEST_ID_1;
		const am_types::am_sinkID_t sID = TEST_ID_1;
		IAmRoutingSenderBackdoor backdoor(env->mpPlugin);
		ASSERT_TRUE( backdoor.containsDomainWithID( (const am_domainID_t)domainID) );
		ASSERT_TRUE( backdoor.containsSourceWithID( (const am_sourceID_t)sID) );

		am_volume_t volume = 50;
		am_time_t time = 10;
		am_CustomRampType_t ramp = RAMP_GENIVI_DIRECT;
		am_Handle_s handle = {H_SETSINKVOLUME, 20};
		am_SourceState_e state = am_SourceState_e::SS_OFF;
		EXPECT_CALL(*env->mpRoutingReceive, ackSetSinkVolumeChange(IsHandleEqual(handle), volume, E_OK)).Times(1);
		EXPECT_CALL(*env->mpRoutingReceive, getDomainOfSink(TEST_ID_1, _)).WillOnce(Return(E_OK));
		am_Error_e error = env->mpPlugin->asyncSetSinkVolume(handle, sID, volume, ramp, time);
		usleep(50000);
		ASSERT_EQ( error, E_OK );
		ASSERT_FALSE( backdoor.containsHandle( handle.handle) );
	}
}

TEST_F(CAmRoutingInterfaceCAPITests, TestDomain_asyncConnect)
{
	ASSERT_TRUE(env->isServiceAvailable());
	if(env->isServiceAvailable())
	{
		IAmRoutingSenderBackdoor backdoor(env->mpPlugin);
		ASSERT_TRUE( backdoor.containsDomainWithID( TEST_ID_1 ));
		ASSERT_TRUE( backdoor.containsSinkWithID( TEST_ID_1 ));
		ASSERT_TRUE( backdoor.containsSourceWithID( TEST_ID_1 ));

		am_types::am_connectionID_t connectionID = TEST_ID_1;
		am_Handle_s handle = {H_CONNECT, 20};
		am_CustomConnectionFormat_t cf = CF_GENIVI_STEREO;
		EXPECT_CALL(*env->mpRoutingReceive, ackConnect(IsHandleEqual(handle), connectionID, E_OK)).Times(1);
		EXPECT_CALL(*env->mpRoutingReceive, getDomainOfSource(TEST_ID_1, _)).WillOnce(Return(E_OK));
		EXPECT_CALL(*env->mpRoutingReceive, getDomainOfSink(TEST_ID_1, _)).WillOnce(Return(E_OK));
		am_Error_e error = env->mpPlugin->asyncConnect(handle, connectionID, TEST_ID_1, TEST_ID_1, cf);
		usleep(50000);
		ASSERT_EQ( error, E_OK );
		ASSERT_FALSE( backdoor.containsHandle( handle.handle ) );
		ASSERT_EQ( backdoor.connectionsCount( ) , 1 );
	}
}

TEST_F(CAmRoutingInterfaceCAPITests, TestDomain_asyncDisconnect)
{
	ASSERT_TRUE(env->isServiceAvailable());
	if(env->isServiceAvailable())
	{
		IAmRoutingSenderBackdoor backdoor(env->mpPlugin);
		ASSERT_TRUE( backdoor.containsDomainWithID( TEST_ID_1 ));
		ASSERT_TRUE( backdoor.containsConnection( TEST_ID_1 ));

		am_types::am_connectionID_t connectionID = TEST_ID_1;
		am_Handle_s handle = {H_DISCONNECT, 20};
		am_CustomConnectionFormat_t cf = CF_GENIVI_STEREO;
		EXPECT_CALL(*env->mpRoutingReceive, ackDisconnect(IsHandleEqual(handle), connectionID, E_OK)).Times(1);
		am_Error_e error = env->mpPlugin->asyncDisconnect(handle, connectionID);
		usleep(50000);
		ASSERT_EQ( error, E_OK );
		ASSERT_FALSE( backdoor.containsHandle( handle.handle ) );
		ASSERT_FALSE( backdoor.containsConnection( TEST_ID_1 ) );
	}
}

TEST_F(CAmRoutingInterfaceCAPITests, TestDomain_asyncAbort)
{
	ASSERT_TRUE(env->isServiceAvailable());
	if(env->isServiceAvailable())
	{
		const am_types::am_domainID_t domainID = TEST_ID_1;
		const am_types::am_sourceID_t sID = TEST_ID_1;
		IAmRoutingSenderBackdoor backdoor(env->mpPlugin);
		ASSERT_TRUE( backdoor.containsDomainWithID( (const am_domainID_t)domainID) );
		ASSERT_TRUE( backdoor.containsSourceWithID( (const am_sourceID_t)sID) );

		am_volume_t volume = 50;
		am_Handle_s handle = {H_SETSOURCESTATE, 200};
		am_types::am_Handle_s retHandle;
		retHandle.setHandle(200);
		retHandle.setHandleType(am_types::am_Handle_e(am_types::am_Handle_e::H_SETSOURCESTATE));
		am_SourceState_e state = am_SourceState_e::SS_OFF;

		//we set an abort handle in order to test the return status
		env->mDomainService->setAbortHandle(retHandle);
		EXPECT_CALL(*env->mpRoutingReceive, getDomainOfSource(TEST_ID_1, _)).WillOnce(Return(E_OK));
		EXPECT_CALL(*env->mpRoutingReceive, ackSetSourceState(IsHandleEqual(handle), E_ABORTED)).Times(1);
		am_Error_e error = env->mpPlugin->asyncSetSourceState(handle, sID, state);
		usleep(50000);
		ASSERT_EQ( error, E_OK );
		ASSERT_FALSE( backdoor.containsHandle( handle.handle) );
	}
}

MATCHER_P(IsListMainSoundPropertiesEqualTo, value, "") {
	auto lh = arg;

	bool bMainSoundProperties = lh.size() == value.size();
	for(int i=0; i<lh.size(); i++)
	{
		bMainSoundProperties &= (lh.at(i).type==value.at(i).type &&
								 lh.at(i).value==value.at(i).value);
	}
	return bMainSoundProperties;
}

TEST_F(CAmRoutingInterfaceCAPITests, TestDomain_asyncSetSinkSoundProperties)
{
	ASSERT_TRUE(env->isServiceAvailable());
	if(env->isServiceAvailable())
	{
		const am_types::am_domainID_t domainID = TEST_ID_1;
		const am_types::am_sinkID_t sID = TEST_ID_1;
		IAmRoutingSenderBackdoor backdoor(env->mpPlugin);
		ASSERT_TRUE( backdoor.containsDomainWithID( (const am_domainID_t)domainID) );
		ASSERT_TRUE( backdoor.containsSourceWithID( (const am_sourceID_t)sID) );

		am_Handle_s handle = {H_SETSINKSOUNDPROPERTIES, 200};
		am_SourceState_e state = am_SourceState_e::SS_OFF;

		std::vector<am_SoundProperty_s> listSoundProperties;
		listSoundProperties.push_back(am_SoundProperty_s{SP_GENIVI_MID, 100});
		listSoundProperties.push_back(am_SoundProperty_s{SP_UNKNOWN, 101});
		listSoundProperties.push_back(am_SoundProperty_s{SP_GENIVI_TREBLE, 100});
		EXPECT_CALL(*env->mpRoutingReceive, getDomainOfSink(TEST_ID_1, _)).WillOnce(Return(E_OK));
		EXPECT_CALL(*env->mpRoutingReceive, ackSetSinkSoundProperties(IsHandleEqual(handle), E_OK)).Times(1);
		am_Error_e error = env->mpPlugin->asyncSetSinkSoundProperties(handle, sID, listSoundProperties);
		usleep(50000);
		ASSERT_EQ( error, E_OK );
		ASSERT_FALSE( backdoor.containsHandle( handle.handle) );
	}
}

TEST_F(CAmRoutingInterfaceCAPITests, TestDomain_asyncSetSinkSoundProperty)
{
	ASSERT_TRUE(env->isServiceAvailable());
	if(env->isServiceAvailable())
	{
		const am_types::am_domainID_t domainID = TEST_ID_1;
		const am_types::am_sinkID_t sID = TEST_ID_1;
		IAmRoutingSenderBackdoor backdoor(env->mpPlugin);
		ASSERT_TRUE( backdoor.containsDomainWithID( (const am_domainID_t)domainID) );
		ASSERT_TRUE( backdoor.containsSourceWithID( (const am_sourceID_t)sID) );

		am_Handle_s handle = {H_SETSINKSOUNDPROPERTY, 200};
		am_SourceState_e state = am_SourceState_e::SS_OFF;
		EXPECT_CALL(*env->mpRoutingReceive, getDomainOfSink(TEST_ID_1, _)).WillOnce(Return(E_OK));
		EXPECT_CALL(*env->mpRoutingReceive, ackSetSinkSoundProperty(IsHandleEqual(handle), E_OK)).Times(1);
		am_Error_e error = env->mpPlugin->asyncSetSinkSoundProperty(handle, sID, am_SoundProperty_s{SP_GENIVI_MID, 100});
		usleep(50000);
		ASSERT_EQ( error, E_OK );
		ASSERT_FALSE( backdoor.containsHandle( handle.handle) );
	}
}

TEST_F(CAmRoutingInterfaceCAPITests, TestDomain_asyncSetSourceSoundProperties)
{
	ASSERT_TRUE(env->isServiceAvailable());
	if(env->isServiceAvailable())
	{
		const am_types::am_domainID_t domainID = TEST_ID_1;
		const am_types::am_sourceID_t sID = TEST_ID_1;
		IAmRoutingSenderBackdoor backdoor(env->mpPlugin);
		ASSERT_TRUE( backdoor.containsDomainWithID( (const am_domainID_t)domainID) );
		ASSERT_TRUE( backdoor.containsSourceWithID( (const am_sourceID_t)sID) );

		am_Handle_s handle = {H_SETSOURCESOUNDPROPERTIES, 200};
		am_SourceState_e state = am_SourceState_e::SS_OFF;

		std::vector<am_SoundProperty_s> listSoundProperties;
		listSoundProperties.push_back(am_SoundProperty_s{SP_GENIVI_MID, 100});
		listSoundProperties.push_back(am_SoundProperty_s{SP_UNKNOWN, 101});
		listSoundProperties.push_back(am_SoundProperty_s{SP_GENIVI_TREBLE, 100});
		EXPECT_CALL(*env->mpRoutingReceive, getDomainOfSource(TEST_ID_1, _)).WillOnce(Return(E_OK));
		EXPECT_CALL(*env->mpRoutingReceive, ackSetSourceSoundProperties(IsHandleEqual(handle), E_OK)).Times(1);
		am_Error_e error = env->mpPlugin->asyncSetSourceSoundProperties(handle, sID, listSoundProperties);
		usleep(50000);
		ASSERT_EQ( error, E_OK );
		ASSERT_FALSE( backdoor.containsHandle( handle.handle) );
	}
}

TEST_F(CAmRoutingInterfaceCAPITests, TestDomain_asyncSetSourceSoundProperty)
{
	ASSERT_TRUE(env->isServiceAvailable());
	if(env->isServiceAvailable())
	{
		const am_types::am_domainID_t domainID = TEST_ID_1;
		const am_types::am_sourceID_t sID = TEST_ID_1;
		IAmRoutingSenderBackdoor backdoor(env->mpPlugin);
		ASSERT_TRUE( backdoor.containsDomainWithID( (const am_domainID_t)domainID) );
		ASSERT_TRUE( backdoor.containsSourceWithID( (const am_sourceID_t)sID) );

		am_Handle_s handle = {H_SETSOURCESOUNDPROPERTY, 200};
		am_SourceState_e state = am_SourceState_e::SS_OFF;
		EXPECT_CALL(*env->mpRoutingReceive, getDomainOfSource(TEST_ID_1, _)).WillOnce(Return(E_OK));
		EXPECT_CALL(*env->mpRoutingReceive, ackSetSourceSoundProperty(IsHandleEqual(handle), E_OK)).Times(1);
		am_Error_e error = env->mpPlugin->asyncSetSourceSoundProperty(handle, sID, am_SoundProperty_s{SP_GENIVI_MID, 100});
		usleep(50000);
		ASSERT_EQ( error, E_OK );
		ASSERT_FALSE( backdoor.containsHandle( handle.handle) );
	}
}

TEST_F(CAmRoutingInterfaceCAPITests, TestDomain_asyncCrossFade)
{
	ASSERT_TRUE(env->isServiceAvailable());
	if(env->isServiceAvailable())
	{
		const am_types::am_crossfaderID_t sID = TEST_ID_1;
		IAmRoutingSenderBackdoor backdoor(env->mpPlugin);
		ASSERT_TRUE( backdoor.containsDomainWithID( TEST_ID_1 ));
		ASSERT_TRUE( backdoor.containsSourceWithID( TEST_ID_1 ));
		ASSERT_TRUE( backdoor.containsCrossfader( TEST_ID_1 ));

		am_Handle_s handle = {H_CROSSFADE, 200};
		am_CustomRampType_t state = RAMP_GENIVI_DIRECT;
		am_HotSink_e hotSink = HS_UNKNOWN;
		EXPECT_CALL(*env->mpRoutingReceive, ackCrossFading(IsHandleEqual(handle), hotSink, E_OK)).Times(1);
		am_Error_e error = env->mpPlugin->asyncCrossFade(handle, sID, hotSink, state, 50);
		usleep(50000);
		ASSERT_EQ( error, E_OK );
		ASSERT_FALSE( backdoor.containsHandle( handle.handle) );
	}
}

TEST_F(CAmRoutingInterfaceCAPITests, TestDomain_asyncSetSinkNotificationConfiguration)
{
	ASSERT_TRUE(env->isServiceAvailable());
	if(env->isServiceAvailable())
	{
		IAmRoutingSenderBackdoor backdoor(env->mpPlugin);
		ASSERT_TRUE( backdoor.containsDomainWithID( TEST_ID_1 ));
		ASSERT_TRUE( backdoor.containsSinkWithID( TEST_ID_1 ));

		am_Handle_s handle = {H_CONNECT, 200};
		am_NotificationConfiguration_s nc = am_NotificationConfiguration_s{NT_UNKNOWN, am_NotificationStatus_e::NS_PERIODIC};
		EXPECT_CALL(*env->mpRoutingReceive, getDomainOfSink(TEST_ID_1, _)).WillOnce(Return(E_OK));
		EXPECT_CALL(*env->mpRoutingReceive, ackSinkNotificationConfiguration(IsHandleStructEqualTo(handle), E_OK)).Times(1);
		am_Error_e error = env->mpPlugin->asyncSetSinkNotificationConfiguration(handle, TEST_ID_1, nc);
		usleep(50000);
		ASSERT_EQ( error, E_OK );
		ASSERT_FALSE( backdoor.containsHandle( handle.handle) );
	}
}

TEST_F(CAmRoutingInterfaceCAPITests, TestDomain_asyncSetSourceNotificationConfiguration)
{
	ASSERT_TRUE(env->isServiceAvailable());
	if(env->isServiceAvailable())
	{
		IAmRoutingSenderBackdoor backdoor(env->mpPlugin);
		ASSERT_TRUE( backdoor.containsDomainWithID( TEST_ID_1 ));
		ASSERT_TRUE( backdoor.containsSinkWithID( TEST_ID_1 ));

		am_Handle_s handle = {H_CONNECT, 200};
		am_NotificationConfiguration_s nc = am_NotificationConfiguration_s{NT_UNKNOWN, am_NotificationStatus_e::NS_PERIODIC};
		EXPECT_CALL(*env->mpRoutingReceive, getDomainOfSource(TEST_ID_1, _)).WillOnce(Return(E_OK));
		EXPECT_CALL(*env->mpRoutingReceive, ackSourceNotificationConfiguration(IsHandleStructEqualTo(handle), E_OK)).Times(1);
		am_Error_e error = env->mpPlugin->asyncSetSourceNotificationConfiguration(handle, TEST_ID_1, nc);
		usleep(50000);
		ASSERT_EQ( error, E_OK );
		ASSERT_FALSE( backdoor.containsHandle( handle.handle) );
	}
}

TEST_F(CAmRoutingInterfaceCAPITests, TestDomain_asyncSetVolumes)
{
	ASSERT_TRUE(env->isServiceAvailable());
	if(env->isServiceAvailable())
	{
		IAmRoutingSenderBackdoor backdoor(env->mpPlugin);
		ASSERT_TRUE( backdoor.containsDomainWithID( TEST_ID_1) );
		ASSERT_TRUE( backdoor.containsSourceWithID( TEST_ID_1) );

		am_types::am_Volumes_L listVolumes;
		am_types::am_SinkSourceID_t dt(TEST_ID_1);
		listVolumes.push_back(am_types::am_Volumes_s(am_types::am_VolumeType_e::VT_SOURCE,
												   dt,
												   50,
												   static_cast<am_types::am_RampType_pe>(RAMP_GENIVI_DIRECT),
												   50));

		std::vector<am_Volumes_s> am_listVolumes;
		am_DataType_u dt1;
		dt1.source = TEST_ID_2;
		am_listVolumes.push_back(am_Volumes_s{VT_SOURCE,
												dt1,
												50,
												RAMP_GENIVI_DIRECT,
												50});

		am_Handle_s handle = {H_SETVOLUMES, 200};
		EXPECT_CALL(*env->mpRoutingReceive, ackSetVolumes(IsHandleStructEqualTo(handle), IsSinkVolumeArrayEqualTo(am_listVolumes, false), E_OK)).Times(1);
		am_Error_e error = env->mpPlugin->asyncSetVolumes(handle, am_listVolumes);

		usleep(50000);
		ASSERT_EQ( error, E_OK );
		ASSERT_FALSE( backdoor.containsHandle( handle.handle) );

	}
}

TEST_F(CAmRoutingInterfaceCAPITests, TestDomain_deregisterSink)
{
	ASSERT_TRUE(env->isServiceAvailable());
	if(env->isServiceAvailable())
	{
		CommonAPI::CallStatus callStatus = CommonAPI::CallStatus::NOT_AVAILABLE;
		am_types::am_Error_e error = am_types::am_Error_e::E_UNKNOWN;
		am_types::am_sinkID_t sinkID = TEST_ID_2;

		IAmRoutingSenderBackdoor backdoor(env->mpPlugin);
		ASSERT_TRUE( backdoor.containsSinkWithID( sinkID ));

		EXPECT_CALL(*env->mpRoutingReceive, deregisterSink(sinkID)).WillOnce(Return(E_OK));
		env->mProxy->deregisterSink(sinkID, callStatus, error);
		usleep(50000);
		ASSERT_EQ((int)error, am_types::am_Error_e::E_OK);
		ASSERT_EQ( callStatus, CommonAPI::CallStatus::SUCCESS );
		ASSERT_FALSE( backdoor.containsSinkWithID( sinkID ));
	}

}

TEST_F(CAmRoutingInterfaceCAPITests, TestDomain_deregisterSource)
{
	ASSERT_TRUE(env->isServiceAvailable());
	if(env->isServiceAvailable())
	{
		CommonAPI::CallStatus callStatus = CommonAPI::CallStatus::NOT_AVAILABLE;
		am_types::am_Error_e error = am_types::am_Error_e::E_UNKNOWN;
		am_types::am_sourceID_t sID = TEST_ID_2;

		IAmRoutingSenderBackdoor backdoor(env->mpPlugin);
		ASSERT_TRUE( backdoor.containsSourceWithID( sID ));

		EXPECT_CALL(*env->mpRoutingReceive, deregisterSource(sID)).WillOnce(Return(E_OK));
		env->mProxy->deregisterSource(sID, callStatus, error);
		usleep(50000);
		ASSERT_EQ((int)error, am_types::am_Error_e::E_OK);
		ASSERT_EQ( callStatus, CommonAPI::CallStatus::SUCCESS );
		ASSERT_FALSE( backdoor.containsSourceWithID( sID ));
	}

}

TEST_F(CAmRoutingInterfaceCAPITests, TestDomain_deregisterCrossfader)
{
	ASSERT_TRUE(env->isServiceAvailable());
	if(env->isServiceAvailable())
	{
		CommonAPI::CallStatus callStatus = CommonAPI::CallStatus::NOT_AVAILABLE;
		am_types::am_Error_e error = am_types::am_Error_e::E_UNKNOWN;
		am_types::am_crossfaderID_t sID = TEST_ID_1;

		IAmRoutingSenderBackdoor backdoor(env->mpPlugin);
		ASSERT_TRUE( backdoor.containsCrossfader( sID ));

		EXPECT_CALL(*env->mpRoutingReceive, deregisterCrossfader(sID)).WillOnce(Return(E_OK));
		env->mProxy->deregisterCrossfader(sID, callStatus, error);
		usleep(50000);
		ASSERT_EQ((int)error, am_types::am_Error_e::E_OK);
		ASSERT_EQ( callStatus, CommonAPI::CallStatus::SUCCESS );
		ASSERT_FALSE( backdoor.containsCrossfader( sID ));
	}

}

TEST_F(CAmRoutingInterfaceCAPITests, deregisterService)
{
	ASSERT_TRUE(env->isServiceAvailable());
	if(env->isServiceAvailable())
	{
		EXPECT_CALL(*env->mpRoutingReceive, deregisterDomain(env->mDomainService->getDomainData().getDomainID())).WillOnce(Return(E_OK));
		bool result = env->mDomainService->deregisterDomain();
		usleep(50000);
		ASSERT_TRUE( result );
		IAmRoutingSenderBackdoor backdoor(env->mpPlugin);
		ASSERT_FALSE( backdoor.containsDomainWithID( TEST_ID_1 ));
		ASSERT_EQ( backdoor.domainsCount(), 0 );
		ASSERT_EQ( backdoor.sourcesCount(), 0 );
		ASSERT_EQ( backdoor.sinksCount(), 0 );
		ASSERT_EQ( backdoor.crossfadersCount(), 0 );
		ASSERT_EQ( backdoor.connectionsCount(), 0 );
	}
}


