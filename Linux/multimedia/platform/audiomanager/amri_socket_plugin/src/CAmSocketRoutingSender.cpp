/************************************************************************
 * @file: CRaDbusWrpSender.cpp
 *
 * @version: 1.1
 *
 * @description: A CAmSocketRoutingSender class implementation of Routing Adapter.
 * CAmSocketRoutingSender class will run in the context of AM process.
 * This is DBus wrapper class for sender class in AM side. CAmSocketRoutingSender
 * class will call the CRaDbusWrpSender class methods via DBus connection
 * @component: platform/audiomanager
 *
 * @author: Jens Lorenz, jlorenz@de.adit-jv.com 2013,2014
 *          Jayanth MC, Jayanth.mc@in.bosch.com 2013,2014
 *
 * @copyright (c) 2010, 2011 Advanced Driver Information Technology.
 * This code is developed by Advanced Driver Information Technology.
 * Copyright of Advanced Driver Information Technology, Bosch, and DENSO.
 * All rights reserved.
 *
 * @see <related items>
 *
 * @history
 *
 ***********************************************************************/

#include <map>
#include "CAmSocketRoutingSender.h"
#include "CSocketSender.h"
#include "CAmDltWrapper.h"
#include "CAmSocketWrapper.h"
#include "CSocketCommon.h"
#include <string.h>

using namespace am;
using namespace std;

DLT_DECLARE_CONTEXT (routingDbus)

extern "C" IAmRoutingSend* amri_socket_pluginFactory()
{
    CAmDltWrapper::instance()->registerContext(routingDbus, "DRS", "DBus Plugin");
	logDebug("CAmSocketRoutingSender::",__func__," amri_socket_pluginFactory called");

	return (new CAmSocketRoutingSender());
}

extern "C" void destroyamri_socket_plugin(IAmRoutingSend* routingSendInterface)
{
	logDebug("CAmSocketRoutingSender::",__func__," destroyamri_socket_plugin called");

	delete routingSendInterface;
}

CAmSocketRoutingSender::CAmSocketRoutingSender() :
        mpCAmSocketWrapper(NULL),
        mIAmSocketRoutingReceiverShadow(this),
        mpRoutingReceive(NULL),
        mpSocketHandler(NULL),
        mSetRoutingReady(false)
{
	logDebug("CAmSocketRoutingSender::",__func__," RoutingSender constructed");
	mpCAmSocketWrapper	= new CAmSocketWrapper( EN_SOCKET_USED_TYPE_PLUGIN, EN_SOCKET_TYPE_AM);
	mCSocketSender.setSocketWrapper( mpCAmSocketWrapper );
}

CAmSocketRoutingSender::~CAmSocketRoutingSender()
{
	logInfo("CAmSocketRoutingSender::",__func__," RoutingSender destructed");
	CAmDltWrapper::instance()->unregisterContext(routingDbus);

	/* release CAmSocketWrapper class */
	if (mpCAmSocketWrapper != NULL)
	{
		CAmSocketWrapper* mpTmpCAmSocketWrapper = NULL;
		mCSocketSender.setSocketWrapper( mpTmpCAmSocketWrapper );
		delete mpCAmSocketWrapper;
		mpCAmSocketWrapper = NULL;
	}
}

void CAmSocketRoutingSender::establishIsThereRoutingAdapter(const std::vector<std::string> & dbusPaths, std::vector<dr_domain_s> & domains)
{
	(void)dbusPaths;
	(void)domains;
	logDebug("CAmSocketRoutingSender::",__func__," called");
	return;
}

void CAmSocketRoutingSender::writeDomainsToMap(const std::vector<dr_domain_s> & domainArray)
{
	(void)domainArray;
	logDebug("CAmSocketRoutingSender::",__func__," called");
	return;
}

am_domainID_t CAmSocketRoutingSender::queryDomainLookup(const std::string & nodeName)
{
	(void)nodeName;
	logDebug("CAmSocketRoutingSender::",__func__," called");
	return 0;
}

am_Error_e CAmSocketRoutingSender::startupInterface(IAmRoutingReceive* pIAmRoutingReceive)
{
	logInfo("CAmSocketRoutingSender::",__func__," called");

	if( NULL == pIAmRoutingReceive )
	{
		logError("CAmSocketRoutingSender::",__func__," , pIAmRoutingReceive is NULL");
		return E_ABORTED;
	}

	if( NULL == mpCAmSocketWrapper )
	{
		logError("CAmSocketRoutingSender::",__func__," , mpCAmSocketWrapper is NULL");
		return E_ABORTED;
	}

	pIAmRoutingReceive->getSocketHandler(mpSocketHandler);

	if( NULL == mpSocketHandler )
	{
		logError("CAmSocketRoutingSender::",__func__," , mpSocketHandler is NULL");
		return E_ABORTED;
	}

	/* Do requestAddFDpoll */
	bool ret = mpCAmSocketWrapper->requestAddFDpoll( mpSocketHandler );
	if( false == ret )
	{
		logError("CAmSocketRoutingSender::",__func__," , requestAddFDpoll Error!!");
		return E_ABORTED;
	}

	mpRoutingReceive = pIAmRoutingReceive;
	mIAmSocketRoutingReceiverShadow.setRoutingReceiver(pIAmRoutingReceive);

	return E_OK;
}

void CAmSocketRoutingSender::getInterfaceVersion(string & version) const
{
	(void)version;
	logDebug("CAmSocketRoutingSender::",__func__," called");
	version = RoutingVersion;
}

void CAmSocketRoutingSender::setRoutingReady(const uint16_t handle)
{
	(void)handle;
	logDebug("CAmSocketRoutingSender::",__func__," called");
	/* setRoutingReady状態を実行済みに設定する */
	mSetRoutingReady = true;
}

void CAmSocketRoutingSender::setRoutingRundown(const uint16_t handle)
{
	am_Error_e ret = E_UNKNOWN;

	ST_SETROUTINGRUNDOWN data;			//送信データ

	logDebug("CAmSocketRoutingSender::",__func__," called");

	/* 初期化 */
	memset(&data, 0, sizeof(ST_SETROUTINGRUNDOWN));

	/* 送信データ設定 */
	data.handle = handle;

	/* 全ドメインにsetRoutingRundownを送信 */
	mapDomain_t::iterator iter = mMapDomains.begin();
	for( ; iter != mMapDomains.end(); ++iter){
		logDebug("[multidomain]CAmSocketRoutingSender::",__func__," setRoutingRundown send domainID = ", iter->first);
		/* Socketデータ作成 */
		mCSocketSender.make_senddata(D_OPC_SETROUTINGRUNDOWN, (void*)&data, sizeof(data));
		/* Socketデータ送信 */
		ret = mCSocketSender.send_async((iter->second).destaddr);
		if(ret != E_OK){
			logError("CAmSocketRoutingSender::",__func__, " send Error =", ret);
		}
	}

	return;
}

am_Error_e CAmSocketRoutingSender::asyncAbort(const am_Handle_s handle)
{
	am_Error_e ret = E_UNKNOWN;

	ST_ASYNCABORT data;			//送信データ

	logDebug("CAmSocketRoutingSender::",__func__," called");

	/* 初期化 */
	memset(&data, 0, sizeof(ST_ASYNCABORT));

	mapHandles_t::iterator iter = mMapHandles.find(handle.handle);
	if (iter != mMapHandles.end()){
		logDebug("[multidomain]CAmSocketRoutingSender::",__func__," domainID = ", (iter->second).domainID);
		mapDomain_t::iterator iterDomain = mMapDomains.find((iter->second).domainID);
		if(iterDomain == mMapDomains.end()){
			logError("[multidomain]CAmSocketRoutingSender::",__func__," domainID is Unknown, domainID = ", (iter->second).domainID);
			return E_UNKNOWN;
		}

		/* 送信データ設定 */
		data.handle = handle;

		/* Socketデータ作成 */
		mCSocketSender.make_senddata(D_OPC_ASYNCABORT, (void*)&data, sizeof(data));

		/* Socketデータ送信 */
		ret = mCSocketSender.send_async((iterDomain->second).destaddr);
		if(ret == E_OK)
		{
			removeHandle(handle);
		}
	}

	return ret;
}

am_Error_e CAmSocketRoutingSender::asyncConnect( const am_Handle_s handle )
{
	(void)handle;

	logDebug("CAmSocketRoutingSender::",__func__," called");

	//TODO:マルチキャスト設計時に有効化
#if 1	//TODO
	return E_OK;
#else	//TODO
	return asyncConnect(handle,
			mMapHandles.at(handle.handle).mConnection.connectionID,
			mMapHandles.at(handle.handle).mConnection.sourceID,
			mMapHandles.at(handle.handle).mConnection.sinkID,
			mMapHandles.at(handle.handle).mConnection.connectionFormat
	);
#endif	//TODO
}

am_Error_e CAmSocketRoutingSender::asyncConnect(const am_Handle_s handle, const am_connectionID_t connectionID,
                                              const am_sourceID_t sourceID, const am_sinkID_t sinkID,
                                              const am_CustomConnectionFormat_t connectionFormat)
{
	am_Error_e ret = E_UNKNOWN;
	am_domainID_t domainID = 0;

	ST_ASYNCCONNECT data;			//送信データ

	logDebug("CAmSocketRoutingSender::",__func__," called");

	/* 初期化 */
	memset(&data, 0, sizeof(ST_ASYNCCONNECT));

	if( NULL != mpRoutingReceive ){
		ret = mpRoutingReceive->getDomainOfSource(sourceID, domainID);
	}
	else{
		logError("CAmSocketRoutingSender::",__func__," mpRoutingReceive is NULL");
	}

	if (ret == E_OK)
	{
		mapDomain_t::iterator iter = mMapDomains.find(domainID);
		if (iter != mMapDomains.end())
		{
			logInfo("[multidomain]CAmSocketRoutingSender::",__func__," destaddr:", (iter->second).destaddr);

			/* 送信データ設定 */
			data.handle = handle;
			data.connectionID = connectionID;
			data.sourceID = sourceID;
			data.sinkID = sinkID;
			data.connectionFormat = connectionFormat;

			/* Socketデータ作成 */
			mCSocketSender.make_senddata(D_OPC_ASYNCCONNECT, (void*)&data, sizeof(data));

			/* Socketデータ送信 */
			ret = mCSocketSender.send_async((iter->second).destaddr);
			if(ret == E_OK)
			{
				logDebug("[multidomain]CAmSocketRoutingSender::",__func__," connectionID=", connectionID, ", domainID=", (iter->second).domainID);
				mMapConnections.insert(std::make_pair(connectionID, (iter->second)));
				mMapHandles.insert(std::make_pair(handle.handle, iter->second));
			}
		}
		else
		{
			logError("CAmSocketRoutingSender::",__func__," domainID is Unknown, domainID = ", domainID);
			ret = E_UNKNOWN;
		}
	}
	return ret;
}

am_Error_e CAmSocketRoutingSender::asyncDisconnect(const am_Handle_s handle, const am_connectionID_t connectionID)
{
	am_Error_e ret = E_UNKNOWN;

	ST_ASYNCDISCONNECT data;			//送信データ

	logInfo("CAmSocketRoutingSender::",__func__," called");

	/* 初期化 */
	memset(&data, 0, sizeof(ST_ASYNCDISCONNECT));

	mapConnections_t::iterator iter = mMapConnections.find(connectionID);
	if (iter != mMapConnections.end())
	{
		logInfo("[multidomain]CAmSocketRoutingSender::",__func__," destaddr:", (iter->second).destaddr);

		/* 送信データ設定 */
		data.handle = handle;
		data.connectionID = connectionID;

		/* Socketデータ作成 */
		mCSocketSender.make_senddata(D_OPC_ASYNCDISCONNECT, (void*)&data, sizeof(data));

		/* Socketデータ送信 */
		ret = mCSocketSender.send_async((iter->second).destaddr);
		if(ret == E_OK)
		{
			mMapHandles.insert(std::make_pair(handle.handle, iter->second));
		}
	}
	return ret;
}

am_Error_e CAmSocketRoutingSender::asyncSetSinkVolume(const am_Handle_s handle, const am_sinkID_t sinkID,
                                                    const am_volume_t volume, const am_CustomRampType_t ramp, const am_time_t time)
{
	am_Error_e ret = E_UNKNOWN;
	am_domainID_t domainID = 0;

	ST_ASYNCSETSINKVOLUME data;			//送信データ

	logDebug("CAmSocketRoutingSender::",__func__," called");

	/* 初期化 */
	memset(&data, 0, sizeof(ST_ASYNCSETSINKVOLUME));

	if( NULL != mpRoutingReceive ){
		ret = mpRoutingReceive->getDomainOfSink(sinkID, domainID);
	}
	else{
		logError("CAmSocketRoutingSender::",__func__," mpRoutingReceive is NULL");
	}

	if (ret == E_OK){
		mapDomain_t::iterator iter = mMapDomains.find(domainID);
		if (iter != mMapDomains.end())
		{
			/* 送信データ設定 */
			data.handle = handle;
			data.sinkID = sinkID;
			data.volume = volume;
			data.ramp = ramp;
			data.time = time;

			/* Socketデータ作成 */
			mCSocketSender.make_senddata(D_OPC_ASYNCSETSINKVOLUME, (void*)&data, sizeof(data));

			/* Socketデータ送信 */
			ret = mCSocketSender.send_async((iter->second).destaddr);
			if(ret == E_OK){
				mMapHandles.insert(std::make_pair(handle.handle, iter->second));
			}
		}
		else{
			logError("CAmSocketRoutingSender::",__func__," domainID is Unknown, domainID = ", domainID);
			ret = E_UNKNOWN;
		}
	}
	return ret;
}

am_Error_e CAmSocketRoutingSender::asyncSetSourceVolume(const am_Handle_s handle, const am_sourceID_t sourceID,
                                                      const am_volume_t volume, const am_CustomRampType_t ramp,
                                                      const am_time_t time)
{
	am_Error_e ret = E_UNKNOWN;
	am_domainID_t domainID = 0;

	ST_ASYNCSETSOURCEVOLUME data;

	logDebug("CAmSocketRoutingSender::",__func__," called");

	/* 初期化 */
	memset(&data, 0, sizeof(ST_ASYNCSETSOURCEVOLUME));

	if( NULL != mpRoutingReceive ){
		ret = mpRoutingReceive->getDomainOfSource(sourceID, domainID);
	}
	else{
		logError("CAmSocketRoutingSender::",__func__," mpRoutingReceive is NULL");
	}

	if (ret == E_OK)
	{
		mapDomain_t::iterator iter = mMapDomains.find(domainID);
		if (iter != mMapDomains.end())
		{
			/* 送信データ設定 */
			data.handle = handle;
			data.sourceID = sourceID;
			data.volume = volume;
			data.ramp = ramp;
			data.time = time;

			/* Socketデータ作成 */
			mCSocketSender.make_senddata(D_OPC_ASYNCSETSOURCEVOLUME, (void*)&data, sizeof(data));

			/* Socketデータ送信 */
			ret = mCSocketSender.send_async((iter->second).destaddr);
			if(ret == E_OK)
			{
			   mMapHandles.insert(std::make_pair(handle.handle, iter->second));
			}
		}
		else
		{
			ret = E_UNKNOWN;
		}
	}
	return ret;
}

am_Error_e CAmSocketRoutingSender::asyncSetSourceState(const am_Handle_s handle, const am_sourceID_t sourceID,
                                                     const am_SourceState_e state)
{
	am_Error_e ret = E_UNKNOWN;
	am_domainID_t domainID = 0;

	ST_ASYNCSETSOURCESTATE data;		//送信データ

	logDebug("CAmSocketRoutingSender::",__func__," called");

	/* 初期化 */
	memset(&data, 0, sizeof(ST_ASYNCSETSOURCESTATE));

	if( NULL != mpRoutingReceive ){
		ret = mpRoutingReceive->getDomainOfSource(sourceID, domainID);
	}
	else{
		logError("CAmSocketRoutingSender::",__func__," mpRoutingReceive is NULL");
	}

	if (ret == E_OK)
	{
		mapDomain_t::iterator iter = mMapDomains.find(domainID);
		if (iter != mMapDomains.end())
		{
			/* 送信データ設定 */
			data.handle = handle;
			data.sourceID = sourceID;
			data.state = state;

			/* Socketデータ作成 */
			mCSocketSender.make_senddata(D_OPC_ASYNCSETSOURCESTATE, (void*)&data, sizeof(data));

			/* Socketデータ送信 */
			ret = mCSocketSender.send_async((iter->second).destaddr);
			if(ret == E_OK)
			{
				mMapHandles.insert(std::make_pair(handle.handle, iter->second));
			}
		}
		else
		{
			logError("CAmSocketRoutingSender::",__func__," domainID is Unknown, domainID = ", domainID);
			ret = E_UNKNOWN;
		}
	}
    return ret;
}

am_Error_e CAmSocketRoutingSender::asyncSetSinkSoundProperties(const am_Handle_s handle, const am_sinkID_t sinkID,
                                                             const vector<am_SoundProperty_s>& listSoundProperties)
{
	am_Error_e ret = E_UNKNOWN;
	am_domainID_t domainID = 0;
	uint16_t loopCnt = 0;						// カウンタ

	ST_ASYNCSETSINKSOUNDPROPERTIES data;		//送信データ

	logDebug("CAmSocketRoutingSender::",__func__," called");

	/* 初期化 */
	memset(&data, 0, sizeof(ST_ASYNCSETSINKSOUNDPROPERTIES));

	if( NULL != mpRoutingReceive ){
		ret = mpRoutingReceive->getDomainOfSink(sinkID, domainID);
	}
	else{
		logError("CAmSocketRoutingSender::",__func__," mpRoutingReceive is NULL");
	}

	if (ret == E_OK)
	{
		mapDomain_t::iterator iter = mMapDomains.find(domainID);
		if (iter != mMapDomains.end())
		{
			/* 送信データ設定 */
			data.handle = handle;
			data.sinkID = sinkID;

			/* 送信データのリスト設定 */
			if( listSoundProperties.size() > 0 ){
				std::vector<am_SoundProperty_s>::const_iterator sp_iter = listSoundProperties.begin();
				for(loopCnt = 0; sp_iter < listSoundProperties.end(); ++sp_iter, ++loopCnt){
					data.listSoundProperties[loopCnt].type = sp_iter->type;
					data.listSoundProperties[loopCnt].value = sp_iter->value;
					logInfo("CAmSocketRoutingSender::", __func__, " am_SoundProperty_s type=", data.listSoundProperties[loopCnt].type, ", value=", data.listSoundProperties[loopCnt].value );
				}
			}

			/* Socketデータ作成 */
			mCSocketSender.make_senddata(D_OPC_ASYNCSETSINKSOUNDPROPERTIES, (void*)&data, sizeof(data));

			ret = mCSocketSender.send_async((iter->second).destaddr);
			if(ret == E_OK)
			{
				mMapHandles.insert(std::make_pair(handle.handle, iter->second));
			}
		}
		else
		{
			ret = E_UNKNOWN;
		}
	}
	return ret;
}

am_Error_e CAmSocketRoutingSender::asyncSetSinkSoundProperty(const am_Handle_s handle, const am_sinkID_t sinkID,
                                                           const am_SoundProperty_s& soundProperty)
{
	am_Error_e ret = E_UNKNOWN;
	am_domainID_t domainID = 0;

	ST_ASYNCSETSINKSOUNDPROPERTY data;		//送信データ

	logDebug("CAmSocketRoutingSender::",__func__," called");

	/* 初期化 */
	memset(&data, 0, sizeof(ST_ASYNCSETSINKSOUNDPROPERTY));

	if( NULL != mpRoutingReceive ){
		ret = mpRoutingReceive->getDomainOfSink(sinkID, domainID);
	}
	else{
		logError("CAmSocketRoutingSender::",__func__," mpRoutingReceive is NULL");
	}

	if (ret == E_OK)
	{
		mapDomain_t::iterator iter = mMapDomains.find(domainID);
		if (iter != mMapDomains.end())
		{
			/* 送信データ設定 */
			data.handle = handle;
			data.sinkID = sinkID;
			data.soundProperty = soundProperty;

			/* Socketデータ作成 */
			mCSocketSender.make_senddata(D_OPC_ASYNCSETSINKSOUNDPROPERTY, (void*)&data, sizeof(data));

			ret = mCSocketSender.send_async((iter->second).destaddr);
			if(ret == E_OK)
			{
				mMapHandles.insert(std::make_pair(handle.handle, iter->second));
			}
		}
		else
		{
			ret = E_UNKNOWN;
		}
	}

	return ret;
}

am_Error_e CAmSocketRoutingSender::asyncSetSourceSoundProperties(const am_Handle_s handle, const am_sourceID_t sourceID,
                                                               const vector<am_SoundProperty_s>& listSoundProperties)
{
	am_Error_e ret = E_UNKNOWN;
	am_domainID_t domainID = 0;
	uint16_t loopCnt = 0;						// カウンタ

	ST_ASYNCSETSOURCESOUNDPROPERTIES data;		//送信データ

	logDebug("CAmSocketRoutingSender::",__func__," called");

	/* 初期化 */
	memset(&data, 0, sizeof(ST_ASYNCSETSOURCESOUNDPROPERTIES));

	if( NULL != mpRoutingReceive ){
		ret = mpRoutingReceive->getDomainOfSource(sourceID, domainID);
	}
	else{
		logError("CAmSocketRoutingSender::",__func__," mpRoutingReceive is NULL");
	}

	if (ret == E_OK)
	{
		mapDomain_t::iterator iter = mMapDomains.find(domainID);
		if (iter != mMapDomains.end())
		{
			/* 送信データ設定 */
			data.handle = handle;
			data.sourceID = sourceID;

			/* 送信データのリスト設定 */
			if( listSoundProperties.size() > 0 ){
				std::vector<am_SoundProperty_s>::const_iterator sp_iter = listSoundProperties.begin();
				for(loopCnt = 0; sp_iter < listSoundProperties.end(); ++sp_iter, ++loopCnt){
					data.listSoundProperties[loopCnt].type = sp_iter->type;
					data.listSoundProperties[loopCnt].value = sp_iter->value;
					logInfo("ISocketRoutingReceiver::", __func__, " am_SoundProperty_s type=", data.listSoundProperties[loopCnt].type, ", value=", data.listSoundProperties[loopCnt].value );
				}
			}

			/* Socketデータ作成 */
			mCSocketSender.make_senddata(D_OPC_ASYNCSETSOURCESOUNDPROPERTIES, (void*)&data, sizeof(data));

			ret = mCSocketSender.send_async((iter->second).destaddr);
			if(ret == E_OK)	{
				mMapHandles.insert(std::make_pair(handle.handle, iter->second));
			}
		}
		else{
			ret = E_UNKNOWN;
		}
	}

	return ret;
}

am_Error_e CAmSocketRoutingSender::asyncSetSourceSoundProperty(const am_Handle_s handle, const am_sourceID_t sourceID,
                                                             const am_SoundProperty_s& soundProperty)
{
	am_Error_e ret = E_UNKNOWN;
	am_domainID_t domainID = 0;

	ST_ASYNCSETSOURCESOUNDPROPERTY data;		//送信データ

	logDebug("CAmSocketRoutingSender::",__func__," called");

	/* 初期化 */
	memset(&data, 0, sizeof(ST_ASYNCSETSOURCESOUNDPROPERTY));

	if( NULL != mpRoutingReceive ){
		ret = mpRoutingReceive->getDomainOfSource(sourceID, domainID);
	}
	else{
		logError("CAmSocketRoutingSender::",__func__," mpRoutingReceive is NULL");
	}

	if (ret == E_OK)
	{
		mapDomain_t::iterator iter = mMapDomains.find(domainID);
		if (iter != mMapDomains.end())
		{
			/* 送信データ設定 */
			data.handle = handle;
			data.sourceID = sourceID;
			data.soundProperty = soundProperty;

			/* Socketデータ作成 */
			mCSocketSender.make_senddata(D_OPC_ASYNCSETSOURCESOUNDPROPERTY, (void*)&data, sizeof(data));

			ret = mCSocketSender.send_async((iter->second).destaddr);
			if(ret == E_OK)
			{
				mMapHandles.insert(std::make_pair(handle.handle, iter->second));
			}
		}
		else
		{
			ret = E_UNKNOWN;
		}
	}

	return ret;
}

am_Error_e CAmSocketRoutingSender::asyncCrossFade(const am_Handle_s handle, const am_crossfaderID_t crossfaderID,
                                                const am_HotSink_e hotSink, const am_CustomRampType_t rampType,
                                                const am_time_t time)
{
	am_Error_e ret = E_UNKNOWN;

	logDebug("CAmSocketRoutingSender::",__func__," called");
#if 0
// TODO: mapping to target domain (am_Crossfader_s does not have a field for domainID ???).
	mCSocketSender.append(handle.handle);
	mCSocketSender.append(crossfaderID);
	mCSocketSender.append(static_cast<int16_t>(hotSink));
	mCSocketSender.append(static_cast<int16_t>(rampType));
	mCSocketSender.append(time);
	mCSocketSender.send_async();
	return E_OK;
#else
	(void) handle;
	(void) crossfaderID;
	(void) hotSink;
	(void) rampType;
	(void) time;
#endif
	return ret;
}

am_Error_e CAmSocketRoutingSender::setDomainState(const am_domainID_t domainID, const am_DomainState_e domainState)
{
    am_Error_e ret = E_UNKNOWN;

	(void)domainID;
	(void)domainState;

	/* RAAからコール無し */
	logDebug("CAmSocketRoutingSender::",__func__," called");
	return ret;
}

am_Error_e CAmSocketRoutingSender::returnBusName(string& BusName) const
{
	am_Error_e ret = E_UNKNOWN;

	/* Plugin内部メソッド RAAからコール無し */
	logDebug("CAmSocketRoutingSender::",__func__," called");
	BusName = RA_SOCKET_BUSNAME;

	return ret;
}

am_Error_e CAmSocketRoutingSender::asyncSetVolumes(const am_Handle_s handle, const vector<am_Volumes_s>& listVolumes)
{
	am_Error_e ret = E_UNKNOWN;
	am_domainID_t domainID = 0;

	ST_ASYNCSETVOLUMES data;		//送信データ

	logDebug("CAmSocketRoutingSender::",__func__," called");

	/* 初期化 */
	memset(&data, 0, sizeof(ST_ASYNCSETVOLUMES));

	if( NULL != mpRoutingReceive ){
		ret = E_OK;
	}
	else{
		logError("CAmSocketRoutingSender::",__func__," mpRoutingReceive is NULL");
		return ret;
	}

	if (!listVolumes.empty())
	{
		// Assumption: the target for volume list is only one domain
		// finding out the domain of one element is sufficient
		am_Volumes_s volInfo = listVolumes[0];

		if(volInfo.volumeType == VT_SINK)
		{
			ret = mpRoutingReceive->getDomainOfSink(volInfo.volumeID.sink, domainID);
		}
		else if(volInfo.volumeType == VT_SOURCE)
		{
			ret = mpRoutingReceive->getDomainOfSource(volInfo.volumeID.source, domainID);
		}
		else
		{
			ret = E_UNKNOWN;
		}

		if (ret == E_OK)
		{
			mapDomain_t::iterator iter = mMapDomains.find(domainID);
			if (iter != mMapDomains.end())
			{
				/* 送信データ設定 */
				data.handle = handle;
				data.listVolumes[0] = volInfo;

				/* Socketデータ作成 */
				mCSocketSender.make_senddata(D_OPC_ASYNCSETVOLUMES, (void*)&data, sizeof(data));

				ret = mCSocketSender.send_async((iter->second).destaddr);
				if(ret == E_OK)
				{
					mMapHandles.insert(std::make_pair(handle.handle, iter->second));
				}
			}
			else
			{
				ret = E_UNKNOWN;
			}
		}
	}
	return ret;
}

am_Error_e CAmSocketRoutingSender::asyncSetSinkNotificationConfiguration(
        const am_Handle_s handle, const am_sinkID_t sinkID, const am_NotificationConfiguration_s& notificationConfiguration)
{
	am_Error_e ret = E_UNKNOWN;
	am_domainID_t domainID = 0;

	ST_ASYNCSETSINKNOTIFICATIONCONFIGURATION data;		//送信データ

	logDebug("CAmSocketRoutingSender::",__func__," called");

	/* 初期化 */
	memset(&data, 0, sizeof(ST_ASYNCSETSINKNOTIFICATIONCONFIGURATION));

	if( NULL != mpRoutingReceive ){
		ret = mpRoutingReceive->getDomainOfSink(sinkID, domainID);
	}
	else{
		logError("CAmSocketRoutingSender::",__func__," mpRoutingReceive is NULL");
	}

	if (ret == E_OK)
	{
		mapDomain_t::iterator iter = mMapDomains.find(domainID);
		if (iter != mMapDomains.end())
		{
			/* 送信データ設定 */
			data.handle = handle;
			data.sinkID = sinkID;
			data.notifiConf = notificationConfiguration;

			/* Socketデータ作成 */
			mCSocketSender.make_senddata(D_OPC_ASYNCSETSINKNOTIFICATIONCONFIGURATION, (void*)&data, sizeof(data));

			/* Socketデータ送信 */
			ret = mCSocketSender.send_async((iter->second).destaddr);
			if(ret == E_OK)
			{
				mMapHandles.insert(std::make_pair(handle.handle, iter->second));
			}
		}
		else
		{
			logError("CAmSocketRoutingSender::",__func__," domainID is Unknown, domainID = ", domainID);
			ret = E_UNKNOWN;
		}
	}
    return ret;
}

am_Error_e CAmSocketRoutingSender::asyncSetSourceNotificationConfiguration(
        const am_Handle_s handle, const am_sourceID_t sourceID,
        const am_NotificationConfiguration_s& notificationConfiguration)
{
	am_Error_e ret = E_UNKNOWN;
	am_domainID_t domainID = 0;

	ST_ASYNCSETSOURCENOTIFICATIONCONFIGURATION data;		//送信データ

	logDebug("CAmSocketRoutingSender::",__func__," called");
	
	/* 初期化 */
	memset(&data, 0, sizeof(ST_ASYNCSETSOURCENOTIFICATIONCONFIGURATION));

	if( NULL != mpRoutingReceive ){
		ret = mpRoutingReceive->getDomainOfSource(sourceID, domainID);
	}
	else{
		logError("CAmSocketRoutingSender::",__func__," mpRoutingReceive is NULL");
	}

	if (ret == E_OK)
	{
		mapDomain_t::iterator iter = mMapDomains.find(domainID);
		if (iter != mMapDomains.end())
		{
			/* 送信データ設定 */
			data.handle = handle;
			data.sourceID = sourceID;
			data.notifiConf = notificationConfiguration;

			/* Socketデータ作成 */
			mCSocketSender.make_senddata(D_OPC_ASYNCSETSOURCENOTIFICATIONCONFIGURATION, (void*)&data, sizeof(data));

			ret = mCSocketSender.send_async((iter->second).destaddr);
			if(ret == E_OK)
			{
				mMapHandles.insert(std::make_pair(handle.handle, iter->second));
			}
		}
		else
		{
			ret = E_UNKNOWN;
		}
	}

	return ret;
}

/****************************************************************/
/* setRoutingReady実行状態の取得								*/
/* name:		getRoutingReady									*/
/* overview:	setRoutingReadyが実行済み/未実行を取得する		*/
/* param:		なし											*/
/* ret:			true：実行済み									*/
/*				false：未実行									*/
/****************************************************************/
bool CAmSocketRoutingSender::getRoutingReady()
{
	logDebug("CAmSocketRoutingSender::",__func__," called");
	return mSetRoutingReady;
}

am_Error_e CAmSocketRoutingSender::resyncConnectionState(const am_domainID_t domainID, std::vector<am_Connection_s>& listOfExistingConnections)
{
	(void)domainID;
	(void)listOfExistingConnections;

	logDebug("CAmSocketRoutingSender::",__func__," called");
	return E_OK;
}

void CAmSocketRoutingSender::removeHandle( am_Handle_s handle )
{
	logDebug("CAmSocketRoutingSender::",__func__," called");
	mMapHandles.erase(handle.handle);
	return;
}

void CAmSocketRoutingSender::addDomainLookup(const am_domainID_t domainID, socket_comm_s lookupData)
{
	lookupData.domainID = domainID;
	mMapDomains.insert(std::make_pair(domainID, lookupData));
	logDebug("[multidomain]CAmSocketRoutingSender::",__func__," called, KeydomainID=", domainID, ", ValueDomainID=", lookupData.domainID);
	return;
}

void CAmSocketRoutingSender::addSourceLookup(const am_Source_s & source, const std::string & application)
{
	(void)source;
	(void)application;
	logDebug("CAmSocketRoutingSender::",__func__," called");
	return;
}
void CAmSocketRoutingSender::addSinkLookup(const am_Sink_s & sink, const std::string & application)
{
	(void)sink;
	(void)application;
	logDebug("CAmSocketRoutingSender::",__func__," called");
	return;
}

void CAmSocketRoutingSender::removeDomainLookup(const am_domainID_t domainID)
{
	(void)domainID;
	logDebug("CAmSocketRoutingSender::",__func__," called");
	return;
}

void CAmSocketRoutingSender::removeConnectionLookup(const am_connectionID_t connectionID)
{
	(void)connectionID;
	logDebug("CAmSocketRoutingSender::",__func__," called");
	return;
}

template <typename TKey> void  CAmSocketRoutingSender::removeEntriesForValue( void )
{
	logDebug("CAmSocketRoutingSender::",__func__," called");
	return;
}

/****************************************************************/
/* 受信コールバック登録											*/
/* name:		registRoutingReceiver							*/
/* overview:	受信コールバックの登録を実行する				*/
/* param(in):	recieveCb	登録対象コールバック				*/
/* ret:			なし											*/
/****************************************************************/
void CAmSocketRoutingSender::registRoutingReceiver( IRecieveCallBack *recieveCb )
{
	logDebug("CAmSocketRoutingSender::",__func__," called");

	if( NULL != mpCAmSocketWrapper )
	{
		//CAmSocketWrapperクラスのコールバック登録メソッド実行
		mpCAmSocketWrapper->registerCallback( recieveCb );
	}

	return;
}

/****************************************************************************************/
/* Get CSocketSender  class pointer														*/
/* name:		getCSocketSenderPt														*/
/* overview:	Get pointer for CSocketSender held by CAmSocketRoutingSender			*/
/* param:		None																	*/
/* ret:			CSocketSender class pointer												*/
/****************************************************************************************/
CSocketSender* CAmSocketRoutingSender::getCSocketSenderPt( void )
{
	logDebug("[multidomain]CAmSocketRoutingSender::",__func__," called");

	return &mCSocketSender;
}

/****************************************************************************************/
/* Notification receiving forwarding address											*/
/* name:		notificationRecvDest													*/
/* overview:	Notification that forwarding address is received						*/
/* param(in):	Forwarding address														*/
/* ret:			none																	*/
/****************************************************************************************/
void CAmSocketRoutingSender::notificationRecvDest( unsigned short destaddr )
{
	logDebug("[multidomain]CAmSocketRoutingSender::",__func__," called, destaddr = ", destaddr);

	if( NULL != mpCAmSocketWrapper )
	{
		mpCAmSocketWrapper->notificationRecvDest(destaddr);
	}
	return;
}

