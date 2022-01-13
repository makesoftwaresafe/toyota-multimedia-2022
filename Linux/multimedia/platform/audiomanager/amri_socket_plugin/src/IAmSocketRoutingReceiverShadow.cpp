/************************************************************************
 * @file: IRaDbusWrpReceiverShadow.cpp
 *
 * @version: 1.1
 *
 * @description: A Receiver class shadow implementation of Routing Adapter.
 * IAmSocketRoutingReceiverShadow class will be running with the AM process.
 * IAmSocketRoutingReceiverShadow class methods will be called via DBus to
 * this methods will intern call the Actual call in AM.
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

#include <fstream> // for ifstream
#include <stdexcept> // for runtime_error
#include <string.h>
#include "IAmSocketRoutingReceiverShadow.h"
#include "CAmDltWrapper.h"
#include "CAmSocketRoutingSender.h"
#include "CSocketCommon.h"

DLT_IMPORT_CONTEXT (routingDbus)

using namespace std;
using namespace am;

IAmSocketRoutingReceiverShadow::IAmSocketRoutingReceiverShadow(CAmSocketRoutingSender* pCAmSocketRoutingSender)
        : mpRecieveCallBack(this, &IAmSocketRoutingReceiverShadow::receiveCallback),
          mpIAmRoutingReceive(NULL),
          mFunctionMap(createMap()),
          mpCAmSocketRoutingSender(pCAmSocketRoutingSender),
          mCurrRecvAddr(0xFFFF)
{
    logDebug("IAmSocketRoutingReceiverShadow::IAmSocketRoutingReceiverShadow called");
}

IAmSocketRoutingReceiverShadow::~IAmSocketRoutingReceiverShadow()
{
    logDebug("IAmSocketRoutingReceiverShadow::~IAmSocketRoutingReceiverShadow called");
}

void IAmSocketRoutingReceiverShadow::registerDomain( void *msg )
{
	am_Error_e ret = E_UNKNOWN;
	am_Domain_s domainData;
	ST_REGISTERDOMAIN data;		//受信データ
	CSocketSender *pCSocketSender = NULL;

	logDebug("[multidomain]IAmSocketRoutingReceiverShadow::",__func__," called start, RecvAddr=", mCurrRecvAddr);

	/* 引数チェック */
	if(msg == NULL){
		logError("IAmSocketRoutingReceiverShadow::",__func__," arg is NULL");
		return;
	}

	if(mpCAmSocketRoutingSender == NULL){
		logError("[multidomain]IAmSocketRoutingReceiverShadow::",__func__," CAmSocketRoutingSender is NULL");
		return;
	}

	/* CSocketSenderポインタをCAmSocketRoutingSenderから取得 */
	pCSocketSender = mpCAmSocketRoutingSender->getCSocketSenderPt();

	if(pCSocketSender == NULL){
		logError("IAmSocketRoutingReceiverShadow::",__func__," pCSocketSender is NULL ");
		return;
	}

	/* 初期化 */
	memset(&data, 0, sizeof(ST_REGISTERDOMAIN));

	/* 受信データ取得 */
	memcpy(&data, msg, sizeof(ST_REGISTERDOMAIN));

	/* 受信データ設定 */
	domainData.domainID = data.domainID;			// am_domainID_t -> am_domainID_t
	domainData.name = data.name;					// cahr[] -> std::string
	domainData.busname = RA_SOCKET_BUSNAME;			// 固定値
	domainData.nodename = data.nodename;			// cahr[] -> std::string
	domainData.early = data.early;					// bool -> bool
	domainData.complete = data.complete;			// bool -> bool
	domainData.state = data.state;					// am_DomainState_e -> am_DomainState_e

	socket_comm_s lookupData(ROUTING_SOCKET_NAMESPACE, domainData.nodename);
	lookupData.destaddr = mCurrRecvAddr;

	ret = mpIAmRoutingReceive->registerDomain(domainData, domainData.domainID);
	if(ret != E_OK){
		logError("IAmSocketRoutingReceiverShadow::",__func__, " Error =", ret);
	}
	else{
		/* 応答用 domainID設定 */
		data.domainID = domainData.domainID;
	}

	/* 応答用 実行結果設定 */
	data.error = ret;

	pCSocketSender->make_senddata(D_OPC_REGISTERDOMAIN, (void*)&data, sizeof(data));

	/* PluginからClientへ同期応答 */
	ret = pCSocketSender->send_sync_reply( mCurrRecvAddr );
	if(ret != E_OK){
		logError("IAmSocketRoutingReceiverShadow::",__func__, " send Error =", ret);
	}

	if( data.error != E_OK ){
		logError("IAmSocketRoutingReceiverShadow::",__func__, " data.error =", data.error);
	}
	else{
		/* CAmSocketRoutingSenderへDomain登録 */
		mpCAmSocketRoutingSender->addDomainLookup(domainData.domainID, lookupData);
	}

	logDebug("[multidomain]IAmSocketRoutingReceiverShadow::",__func__," called end, domainID=", domainData.domainID);

	return;
}

void IAmSocketRoutingReceiverShadow::registerSource( void *msg )
{
	(void)msg;
	return;
}

void IAmSocketRoutingReceiverShadow::registerSink( void *msg )
{
	(void)msg;
	return;
}

void IAmSocketRoutingReceiverShadow::registerGateway( void *msg )
{
	(void)msg;
	return;
}

void IAmSocketRoutingReceiverShadow::hookDomainRegistrationComplete( void *msg )
{
	am_Error_e ret = E_UNKNOWN;
	ST_HOOKDOMAINREGISTRATIONCOMPLETE data;		//受信データ
	CSocketSender *pCSocketSender = NULL;

	logDebug("IAmSocketRoutingReceiverShadow::",__func__," called start, RecvAddr", mCurrRecvAddr);

	/* 引数チェック */
	if(msg == NULL){
		logError("IAmSocketRoutingReceiverShadow::",__func__," arg is NULL");
		return;
	}

	if(mpCAmSocketRoutingSender == NULL){
		logError("[multidomain]IAmSocketRoutingReceiverShadow::",__func__," CAmSocketRoutingSender is NULL");
		return;
	}

	/* CSocketSenderポインタをCAmSocketRoutingSenderから取得 */
	pCSocketSender = mpCAmSocketRoutingSender->getCSocketSenderPt();

	if(pCSocketSender == NULL){
		logError("IAmSocketRoutingReceiverShadow::",__func__," pCSocketSender is NULL");
		return;
	}

	/* 初期化 */
	memset(&data, 0, sizeof(ST_HOOKDOMAINREGISTRATIONCOMPLETE));

	/* 受信データ取得 */
	memcpy(&data, msg, sizeof(ST_HOOKDOMAINREGISTRATIONCOMPLETE));

	mpIAmRoutingReceive->hookDomainRegistrationComplete(data.domainID);

	pCSocketSender->make_senddata(D_OPC_HOOKDOMAINREGISTRATIONCOMPLETE, (void*)&data, sizeof(data));

	/* PluginからClientへ非同期応答 */
	ret = pCSocketSender->send_async( mCurrRecvAddr );
	if(ret != E_OK){
		logError("IAmSocketRoutingReceiverShadow::",__func__, " send Error =", ret);
	}
	logDebug("IAmSocketRoutingReceiverShadow::",__func__," called end");

	return;
}

void IAmSocketRoutingReceiverShadow::ackConnect( void *msg )
{
	am_Error_e ret = E_UNKNOWN;
	ST_ACKCONNECT data;		//受信データ
	CSocketSender *pCSocketSender = NULL;

	logDebug("IAmSocketRoutingReceiverShadow::",__func__," called start");

	/* 引数チェック */
	if(msg == NULL){
		logError("IAmSocketRoutingReceiverShadow::",__func__," arg is NULL");
		return;
	}

	if(mpCAmSocketRoutingSender == NULL){
		logError("[multidomain]IAmSocketRoutingReceiverShadow::",__func__," CAmSocketRoutingSender is NULL");
		return;
	}

	/* CSocketSenderポインタをCAmSocketRoutingSenderから取得 */
	pCSocketSender = mpCAmSocketRoutingSender->getCSocketSenderPt();

	if(pCSocketSender == NULL){
		logError("[multidomain]IAmSocketRoutingReceiverShadow::",__func__," pCSocketSender is NULL");
		return;
	}

	/* 初期化 */
	memset(&data, 0, sizeof(ST_ACKCONNECT));

	/* 受信データ取得 */
	memcpy(&data, msg, sizeof(ST_ACKCONNECT));

	mpIAmRoutingReceive->ackConnect(data.handle, data.connectionID, data.error);
	mpCAmSocketRoutingSender->removeHandle(data.handle);

	pCSocketSender->make_senddata(D_OPC_ACKCONNECT, (void*)&data, sizeof(data));

	/* PluginからClientへ非同期応答 */
	ret = pCSocketSender->send_async( mCurrRecvAddr );
	if(ret != E_OK){
		logError("IAmSocketRoutingReceiverShadow::",__func__, " send Error =", ret);
	}
	logDebug("IAmSocketRoutingReceiverShadow::",__func__," called end");

	return;
}

void IAmSocketRoutingReceiverShadow::ackDisconnect( void *msg )
{
	am_Error_e ret = E_UNKNOWN;
	ST_ACKDISCONNECT data;		//受信データ
	CSocketSender *pCSocketSender = NULL;

	logDebug("IAmSocketRoutingReceiverShadow::",__func__," called start, RecvAddr=", mCurrRecvAddr);

	/* 引数チェック */
	if(msg == NULL){
		logError("IAmSocketRoutingReceiverShadow::",__func__," arg is NULL");
		return;
	}

	if(mpCAmSocketRoutingSender == NULL){
		logError("[multidomain]IAmSocketRoutingReceiverShadow::",__func__," CAmSocketRoutingSender is NULL");
		return;
	}

	/* CSocketSenderポインタをCAmSocketRoutingSenderから取得 */
	pCSocketSender = mpCAmSocketRoutingSender->getCSocketSenderPt();

	if(pCSocketSender == NULL){
		logError("IAmSocketRoutingReceiverShadow::",__func__," pCSocketSender is NULL");
		return;
	}

	/* 初期化 */
	memset(&data, 0, sizeof(ST_ACKDISCONNECT));

	/* 受信データ取得 */
	memcpy(&data, msg, sizeof(ST_ACKDISCONNECT));

	mpIAmRoutingReceive->ackDisconnect(data.handle, data.connectionID, data.error);
	mpCAmSocketRoutingSender->removeHandle(data.handle);

	if(data.error == E_OK){
		/*
		 * The map entry mapping a connection ID to the domain socket information should be removed
		 * only if the connection is successfully disconnected. If the lookup entry is removed
		 * without this check any further disconnect request from the Audiomanager controller
		 * would not be sent to the routing side.
		 */
		mpCAmSocketRoutingSender->removeConnectionLookup(data.connectionID);
	}

	pCSocketSender->make_senddata(D_OPC_ACKDISCONNECT, (void*)&data, sizeof(data));

	/* PluginからClientへ非同期応答 */
	ret = pCSocketSender->send_async( mCurrRecvAddr );
	if(ret != E_OK){
		logError("IAmSocketRoutingReceiverShadow::",__func__, " send Error =", ret);
	}
	logDebug("IAmSocketRoutingReceiverShadow::",__func__," called end");

	return;
}

void IAmSocketRoutingReceiverShadow::ackSetSinkVolumeChange( void *msg )
{
	am_Error_e ret = E_UNKNOWN;
	ST_ACKSETSINKVOLUMECHANGE data;		//受信データ
	CSocketSender *pCSocketSender = NULL;

	logDebug("IAmSocketRoutingReceiverShadow::",__func__," called start, RecvAddr=", mCurrRecvAddr);

	/* 引数チェック */
	if(msg == NULL){
		logError("IAmSocketRoutingReceiverShadow::",__func__," arg is NULL");
		return;
	}

	if(mpCAmSocketRoutingSender == NULL){
		logError("[multidomain]IAmSocketRoutingReceiverShadow::",__func__," CAmSocketRoutingSender is NULL");
		return;
	}

	/* CSocketSenderポインタをCAmSocketRoutingSenderから取得 */
	pCSocketSender = mpCAmSocketRoutingSender->getCSocketSenderPt();

	if(pCSocketSender == NULL){
		logError("IAmSocketRoutingReceiverShadow::",__func__," pCSocketSender is NULL");
		return;
	}

	/* 初期化 */
	memset(&data, 0, sizeof(ST_ACKSETSINKVOLUMECHANGE));

	/* 受信データ取得 */
	memcpy(&data, msg, sizeof(ST_ACKSETSINKVOLUMECHANGE));

	/* 受信データ設定 */
	mpIAmRoutingReceive->ackSetSinkVolumeChange(data.handle, data.volume, data.error);
	mpCAmSocketRoutingSender->removeHandle(data.handle);

	pCSocketSender->make_senddata(D_OPC_ACKSETSINKVOLUMECHANGE, (void*)&data, sizeof(data));

	/* PluginからClientへ非同期応答 */
	ret = pCSocketSender->send_async( mCurrRecvAddr );
	if(ret != E_OK){
		logError("IAmSocketRoutingReceiverShadow::",__func__, " send Error =", ret);
	}
	logDebug("IAmSocketRoutingReceiverShadow::",__func__," called end");

	return;
}

void IAmSocketRoutingReceiverShadow::ackSetSourceState( void *msg )
{
	am_Error_e ret = E_UNKNOWN;
	ST_ACKSETSOURCESTATE data;		//受信データ
	CSocketSender *pCSocketSender = NULL;

	logDebug("IAmSocketRoutingReceiverShadow::",__func__," called start, RecvAddr=", mCurrRecvAddr);

	/* 引数チェック */
	if(msg == NULL){
		logError("IAmSocketRoutingReceiverShadow::",__func__," arg is NULL");
		return;
	}

	if(mpCAmSocketRoutingSender == NULL){
		logError("[multidomain]IAmSocketRoutingReceiverShadow::",__func__," CAmSocketRoutingSender is NULL");
		return;
	}

	/* CSocketSenderポインタをCAmSocketRoutingSenderから取得 */
	pCSocketSender = mpCAmSocketRoutingSender->getCSocketSenderPt();

	if(pCSocketSender == NULL){
		logError("IAmSocketRoutingReceiverShadow::",__func__," pCSocketSender is NULL");
		return;
	}

	/* 初期化 */
	memset(&data, 0, sizeof(ST_ACKSETSOURCESTATE));

	/* 受信データ取得 */
	memcpy(&data, msg, sizeof(ST_ACKSETSOURCESTATE));

	/* 受信データ設定 */
	mpIAmRoutingReceive->ackSetSourceState(data.handle, data.error);
	mpCAmSocketRoutingSender->removeHandle(data.handle);

	pCSocketSender->make_senddata(D_OPC_ACKSETSOURCESTATE, (void*)&data, sizeof(data));

	/* PluginからClientへ非同期応答 */
	ret = pCSocketSender->send_async( mCurrRecvAddr );
	if(ret != E_OK){
		logError("IAmSocketRoutingReceiverShadow::",__func__, " send Error =", ret);
	}
	logDebug("IAmSocketRoutingReceiverShadow::",__func__," called end");

	return;
}

void IAmSocketRoutingReceiverShadow::ackSinkVolumeTick( void *msg )
{
	(void)msg;
	return;
}

void IAmSocketRoutingReceiverShadow::ackSourceVolumeTick( void *msg )
{
	(void)msg;
	return;
}

void IAmSocketRoutingReceiverShadow::ackSetSourceVolumeChange( void *msg )
{
	am_Error_e ret = E_UNKNOWN;
	ST_ACKSETSOURCEVOLUMECHANGE data;		//受信データ
	CSocketSender *pCSocketSender = NULL;

	logDebug("IAmSocketRoutingReceiverShadow::",__func__," called start, RecvAddr=", mCurrRecvAddr);

	/* 引数チェック */
	if(msg == NULL){
		logError("IAmSocketRoutingReceiverShadow::",__func__," arg is NULL");
		return;
	}

	if(mpCAmSocketRoutingSender == NULL){
		logError("[multidomain]IAmSocketRoutingReceiverShadow::",__func__," CAmSocketRoutingSender is NULL");
		return;
	}

	/* CSocketSenderポインタをCAmSocketRoutingSenderから取得 */
	pCSocketSender = mpCAmSocketRoutingSender->getCSocketSenderPt();

	if(pCSocketSender == NULL){
		logError("IAmSocketRoutingReceiverShadow::",__func__," pCSocketSender is NULL");
		return;
	}

	/* 初期化 */
	memset(&data, 0, sizeof(ST_ACKSETSOURCEVOLUMECHANGE));

	/* 受信データ取得 */
	memcpy(&data, msg, sizeof(ST_ACKSETSOURCEVOLUMECHANGE));

	/* 受信データ設定 */
	mpIAmRoutingReceive->ackSetSourceVolumeChange(data.handle, data.volume, data.error);
	mpCAmSocketRoutingSender->removeHandle(data.handle);

	pCSocketSender->make_senddata(D_OPC_ACKSETSOURCEVOLUMECHANGE, (void*)&data, sizeof(data));

	/* PluginからClientへ非同期応答 */
	ret = pCSocketSender->send_async( mCurrRecvAddr );
	if(ret != E_OK){
		logError("IAmSocketRoutingReceiverShadow::",__func__, " send Error =", ret);
	}
	logDebug("IAmSocketRoutingReceiverShadow::",__func__," called end");

	return;
}

void IAmSocketRoutingReceiverShadow::ackSetSinkSoundProperties( void *msg )
{
	am_Error_e ret = E_UNKNOWN;
	ST_ACKSETSINKSOUNDPROPERTIES data;		//受信データ
	CSocketSender *pCSocketSender = NULL;

	logDebug("IAmSocketRoutingReceiverShadow::",__func__," called start, RecvAddr=", mCurrRecvAddr);

	/* 引数チェック */
	if(msg == NULL){
		logError("IAmSocketRoutingReceiverShadow::",__func__," arg is NULL");
		return;
	}

	if(mpCAmSocketRoutingSender == NULL){
		logError("[multidomain]IAmSocketRoutingReceiverShadow::",__func__," CAmSocketRoutingSender is NULL");
		return;
	}

	/* CSocketSenderポインタをCAmSocketRoutingSenderから取得 */
	pCSocketSender = mpCAmSocketRoutingSender->getCSocketSenderPt();

	if(pCSocketSender == NULL){
		logError("IAmSocketRoutingReceiverShadow::",__func__," pCSocketSender is NULL");
		return;
	}

	/* 初期化 */
	memset(&data, 0, sizeof(ST_ACKSETSINKSOUNDPROPERTIES));

	/* 受信データ取得 */
	memcpy(&data, msg, sizeof(ST_ACKSETSINKSOUNDPROPERTIES));

	/* 受信データ設定 */
	mpIAmRoutingReceive->ackSetSinkSoundProperties(data.handle, data.error);
	mpCAmSocketRoutingSender->removeHandle(data.handle);

	pCSocketSender->make_senddata(D_OPC_ACKSETSINKSOUNDPROPERTIES, (void*)&data, sizeof(data));

	/* PluginからClientへ非同期応答 */
	ret = pCSocketSender->send_async( mCurrRecvAddr );
	if(ret != E_OK){
		logError("IAmSocketRoutingReceiverShadow::",__func__, " send Error =", ret);
	}
	logDebug("IAmSocketRoutingReceiverShadow::",__func__," called end");

	return;
}

void IAmSocketRoutingReceiverShadow::ackSetSourceSoundProperties( void *msg )
{
	am_Error_e ret = E_UNKNOWN;
	ST_ACKSETSOURCESOUNDPROPERTIES data;		//受信データ
	CSocketSender *pCSocketSender = NULL;

	logDebug("IAmSocketRoutingReceiverShadow::",__func__," called start, RecvAddr=", mCurrRecvAddr);

	/* 引数チェック */
	if(msg == NULL){
		logError("IAmSocketRoutingReceiverShadow::",__func__," arg is NULL");
		return;
	}

	if(mpCAmSocketRoutingSender == NULL){
		logError("[multidomain]IAmSocketRoutingReceiverShadow::",__func__," CAmSocketRoutingSender is NULL");
		return;
	}

	/* CSocketSenderポインタをCAmSocketRoutingSenderから取得 */
	pCSocketSender = mpCAmSocketRoutingSender->getCSocketSenderPt();

	if(pCSocketSender == NULL){
		logError("IAmSocketRoutingReceiverShadow::",__func__," pCSocketSender is NULL");
		return;
	}

	/* 初期化 */
	memset(&data, 0, sizeof(ST_ACKSETSOURCESOUNDPROPERTIES));

	/* 受信データ取得 */
	memcpy(&data, msg, sizeof(ST_ACKSETSOURCESOUNDPROPERTIES));

	/* 受信データ設定 */
	mpIAmRoutingReceive->ackSetSourceSoundProperties(data.handle, data.error);
	mpCAmSocketRoutingSender->removeHandle(data.handle);

	pCSocketSender->make_senddata(D_OPC_ACKSETSOURCESOUNDPROPERTIES, (void*)&data, sizeof(data));

	/* PluginからClientへ非同期応答 */
	ret = pCSocketSender->send_async( mCurrRecvAddr );
	if(ret != E_OK){
		logError("IAmSocketRoutingReceiverShadow::",__func__, " send Error =", ret);
	}
	logDebug("IAmSocketRoutingReceiverShadow::",__func__," called end");

	return;
}

void IAmSocketRoutingReceiverShadow::ackSetSinkSoundProperty( void *msg )
{
	am_Error_e ret = E_UNKNOWN;
	ST_ACKSETSINKSOUNDPROPERTY data;		//受信データ
	CSocketSender *pCSocketSender = NULL;

	logDebug("IAmSocketRoutingReceiverShadow::",__func__," called start, RecvAddr=", mCurrRecvAddr);

	/* 引数チェック */
	if(msg == NULL){
		logError("IAmSocketRoutingReceiverShadow::",__func__," arg is NULL");
		return;
	}

	if(mpCAmSocketRoutingSender == NULL){
		logError("[multidomain]IAmSocketRoutingReceiverShadow::",__func__," CAmSocketRoutingSender is NULL");
		return;
	}

	/* CSocketSenderポインタをCAmSocketRoutingSenderから取得 */
	pCSocketSender = mpCAmSocketRoutingSender->getCSocketSenderPt();

	if(pCSocketSender == NULL){
		logError("IAmSocketRoutingReceiverShadow::",__func__," pCSocketSender is NULL");
		return;
	}

	/* 初期化 */
	memset(&data, 0, sizeof(ST_ACKSETSINKSOUNDPROPERTY));

	/* 受信データ取得 */
	memcpy(&data, msg, sizeof(ST_ACKSETSINKSOUNDPROPERTY));

	/* 受信データ設定 */
	mpIAmRoutingReceive->ackSetSinkSoundProperty(data.handle, data.error);
	mpCAmSocketRoutingSender->removeHandle(data.handle);

	pCSocketSender->make_senddata(D_OPC_ACKSETSINKSOUNDPROPERTY, (void*)&data, sizeof(data));

	/* PluginからClientへ非同期応答 */
	ret = pCSocketSender->send_async( mCurrRecvAddr );
	if(ret != E_OK){
		logError("IAmSocketRoutingReceiverShadow::",__func__, " send Error =", ret);
	}
	logDebug("IAmSocketRoutingReceiverShadow::",__func__," called end");

	return;
}

void IAmSocketRoutingReceiverShadow::ackSetSourceSoundProperty( void *msg )
{
	am_Error_e ret = E_UNKNOWN;
	ST_ACKSETSOURCESOUNDPROPERTY data;		//受信データ
	CSocketSender *pCSocketSender = NULL;

	logDebug("IAmSocketRoutingReceiverShadow::",__func__," called start, RecvAddr=", mCurrRecvAddr);

	/* 引数チェック */
	if(msg == NULL){
		logError("IAmSocketRoutingReceiverShadow::",__func__," arg is NULL");
		return;
	}

	if(mpCAmSocketRoutingSender == NULL){
		logError("[multidomain]IAmSocketRoutingReceiverShadow::",__func__," CAmSocketRoutingSender is NULL");
		return;
	}

	/* CSocketSenderポインタをCAmSocketRoutingSenderから取得 */
	pCSocketSender = mpCAmSocketRoutingSender->getCSocketSenderPt();

	if(pCSocketSender == NULL){
		logError("IAmSocketRoutingReceiverShadow::",__func__," pCSocketSender is NULL");
		return;
	}

	/* 初期化 */
	memset(&data, 0, sizeof(ST_ACKSETSOURCESOUNDPROPERTY));

	/* 受信データ取得 */
	memcpy(&data, msg, sizeof(ST_ACKSETSOURCESOUNDPROPERTY));

	/* 受信データ設定 */
	mpIAmRoutingReceive->ackSetSourceSoundProperty(data.handle, data.error);
	mpCAmSocketRoutingSender->removeHandle(data.handle);

	pCSocketSender->make_senddata(D_OPC_ACKSETSOURCESOUNDPROPERTY, (void*)&data, sizeof(data));

	/* PluginからClientへ非同期応答 */
	ret = pCSocketSender->send_async( mCurrRecvAddr );
	if(ret != E_OK){
		logError("IAmSocketRoutingReceiverShadow::",__func__, " send Error =", ret);
	}
	logDebug("IAmSocketRoutingReceiverShadow::",__func__," called end");

	return;
}

void IAmSocketRoutingReceiverShadow::ackCrossFading( void *msg )
{
	am_Error_e ret = E_UNKNOWN;
	ST_ACKCROSSFADING data;		//受信データ
	CSocketSender *pCSocketSender = NULL;

	logDebug("IAmSocketRoutingReceiverShadow::",__func__," called start, RecvAddr=", mCurrRecvAddr);

	/* 引数チェック */
	if(msg == NULL){
		logError("IAmSocketRoutingReceiverShadow::",__func__," arg is NULL");
		return;
	}

	if(mpCAmSocketRoutingSender == NULL){
		logError("[multidomain]IAmSocketRoutingReceiverShadow::",__func__," CAmSocketRoutingSender is NULL");
		return;
	}

	/* CSocketSenderポインタをCAmSocketRoutingSenderから取得 */
	pCSocketSender = mpCAmSocketRoutingSender->getCSocketSenderPt();

	if(pCSocketSender == NULL){
		logError("IAmSocketRoutingReceiverShadow::",__func__," pCSocketSender is NULL");
		return;
	}

	/* 初期化 */
	memset(&data, 0, sizeof(ST_ACKCROSSFADING));

	/* 受信データ取得 */
	memcpy(&data, msg, sizeof(ST_ACKCROSSFADING));

	/* 受信データ設定 */
	mpIAmRoutingReceive->ackCrossFading(data.handle, data.hotSink, data.error);
	mpCAmSocketRoutingSender->removeHandle(data.handle);

	pCSocketSender->make_senddata(D_OPC_ACKCROSSFADING, (void*)&data, sizeof(data));

	/* PluginからClientへ非同期応答 */
	ret = pCSocketSender->send_async( mCurrRecvAddr );
	if(ret != E_OK){
		logError("IAmSocketRoutingReceiverShadow::",__func__, " send Error =", ret);
	}
	logDebug("IAmSocketRoutingReceiverShadow::",__func__," called end");

	return;
}

void IAmSocketRoutingReceiverShadow::peekDomain( void *msg )
{
	(void)msg;
	return;
}

void IAmSocketRoutingReceiverShadow::deregisterDomain( void *msg )
{
	(void)msg;
	return;
}

void IAmSocketRoutingReceiverShadow::deregisterGateway( void *msg )
{
	(void)msg;
	return;
}

void IAmSocketRoutingReceiverShadow::peekSink( void *msg )
{
	(void)msg;
	return;
}

void IAmSocketRoutingReceiverShadow::deregisterSink( void *msg )
{
	(void)msg;
	return;
}

void IAmSocketRoutingReceiverShadow::peekSource( void *msg )
{
	(void)msg;
	return;
}

void IAmSocketRoutingReceiverShadow::deregisterSource( void *msg )
{
	(void)msg;
	return;
}

void IAmSocketRoutingReceiverShadow::registerCrossfader( void *msg )
{
	(void)msg;
	return;
}

void IAmSocketRoutingReceiverShadow::deregisterCrossfader( void *msg )
{
	(void)msg;
	return;
}

void IAmSocketRoutingReceiverShadow::peekSourceClassID( void *msg )
{
	(void)msg;
	return;
}

void IAmSocketRoutingReceiverShadow::peekSinkClassID( void *msg )
{
	(void)msg;
	return;
}

void IAmSocketRoutingReceiverShadow::hookInterruptStatusChange( void *msg )
{
	(void)msg;
	return;
}

void IAmSocketRoutingReceiverShadow::hookSinkAvailablityStatusChange( void *msg )
{
	am_Error_e ret = E_UNKNOWN;
	ST_HOOKSINKAVAILABLITYSTATUSCHANGE data;		//受信データ
	CSocketSender *pCSocketSender = NULL;

	logDebug("IAmSocketRoutingReceiverShadow::",__func__," called start, RecvAddr=", mCurrRecvAddr);

	/* 引数チェック */
	if(msg == NULL){
		logError("IAmSocketRoutingReceiverShadow::",__func__," arg is NULL");
		return;
	}

	if(mpCAmSocketRoutingSender == NULL){
		logError("[multidomain]IAmSocketRoutingReceiverShadow::",__func__," CAmSocketRoutingSender is NULL");
		return;
	}

	/* CSocketSenderポインタをCAmSocketRoutingSenderから取得 */
	pCSocketSender = mpCAmSocketRoutingSender->getCSocketSenderPt();

	if(pCSocketSender == NULL){
		logError("IAmSocketRoutingReceiverShadow::",__func__," pCSocketSender is NULL");
		return;
	}

	/* 初期化 */
	memset(&data, 0, sizeof(ST_HOOKSINKAVAILABLITYSTATUSCHANGE));

	/* 受信データ取得 */
	memcpy(&data, msg, sizeof(ST_HOOKSINKAVAILABLITYSTATUSCHANGE));

	/* 受信データ設定 */
	mpIAmRoutingReceive->hookSinkAvailablityStatusChange(data.sinkID, data.availability);

	pCSocketSender->make_senddata(D_OPC_HOOKSINKAVAILABLITYSTATUSCHANGE, (void*)&data, sizeof(data));

	/* PluginからClientへ非同期応答 */
	ret = pCSocketSender->send_async( mCurrRecvAddr );
	if(ret != E_OK){
		logError("IAmSocketRoutingReceiverShadow::",__func__, " send Error =", ret);
	}
	logDebug("IAmSocketRoutingReceiverShadow::",__func__," called end");

	return;
}

void IAmSocketRoutingReceiverShadow::hookSourceAvailablityStatusChange( void *msg )
{
	am_Error_e ret = E_UNKNOWN;
	ST_HOOKSOURCEAVAILABLITYSTATUSCHANGE data;		//受信データ
	CSocketSender *pCSocketSender = NULL;

	logDebug("IAmSocketRoutingReceiverShadow::",__func__," called start");

	/* 引数チェック */
	if(msg == NULL){
		logError("IAmSocketRoutingReceiverShadow::",__func__," arg is NULL");
		return;
	}

	if(mpCAmSocketRoutingSender == NULL){
		logError("[multidomain]IAmSocketRoutingReceiverShadow::",__func__," CAmSocketRoutingSender is NULL");
		return;
	}

	/* CSocketSenderポインタをCAmSocketRoutingSenderから取得 */
	pCSocketSender = mpCAmSocketRoutingSender->getCSocketSenderPt();

	if(pCSocketSender == NULL){
		logError("IAmSocketRoutingReceiverShadow::",__func__," pCSocketSender is NULL");
		return;
	}

	/* 初期化 */
	memset(&data, 0, sizeof(ST_HOOKSOURCEAVAILABLITYSTATUSCHANGE));

	/* 受信データ取得 */
	memcpy(&data, msg, sizeof(ST_HOOKSOURCEAVAILABLITYSTATUSCHANGE));

	/* 受信データ設定 */
	mpIAmRoutingReceive->hookSourceAvailablityStatusChange(data.sourceID, data.availability);

	pCSocketSender->make_senddata(D_OPC_HOOKSOURCEAVAILABLITYSTATUSCHANGE, (void*)&data, sizeof(data));

	/* PluginからClientへ非同期応答 */
	ret = pCSocketSender->send_async( mCurrRecvAddr );
	if(ret != E_OK){
		logError("IAmSocketRoutingReceiverShadow::",__func__, " send Error =", ret);
	}
	logDebug("IAmSocketRoutingReceiverShadow::",__func__," called end");

	return;
}

void IAmSocketRoutingReceiverShadow::hookDomainStateChange( void *msg )
{
	(void)msg;
	return;
}

void IAmSocketRoutingReceiverShadow::hookTimingInformationChanged( void *msg )
{
	(void)msg;
	return;
}

void IAmSocketRoutingReceiverShadow::sendChangedData( void *msg )
{
	(void)msg;
	return;
}

/****************************************************************/
/* confirmRoutingReady実行										*/
/* name:		confirmRoutingReady								*/
/* overview:	受信したconfirmRoutingReadyの処理を実行する		*/
/* param:		msg : 受信メッセージのデータ部					*/
/* ret:			なし											*/
/****************************************************************/
void IAmSocketRoutingReceiverShadow::confirmRoutingReady( void *msg )
{
	ST_CONFIRMROUTINGREADY		data;				// 受信データ
	CSocketSender *pCSocketSender = NULL;

	logDebug("IAmSocketRoutingReceiverShadow::",__func__," called start, RecvAddr=", mCurrRecvAddr);

	/* 引数チェック */
	if(msg == NULL){
		logError("IAmSocketRoutingReceiverShadow::",__func__," arg is NULL");
		return;
	}

	/* 変数チェック */
	if(mpIAmRoutingReceive == NULL){
		logError("IAmSocketRoutingReceiverShadow::",__func__," mpIAmRoutingReceive is NULL");
		return;
	}

	if(mpCAmSocketRoutingSender == NULL){
		logError("[multidomain]IAmSocketRoutingReceiverShadow::",__func__," CAmSocketRoutingSender is NULL");
		return;
	}

	/* get CSocketSender pointer from CAmSocketRoutingSender */
	pCSocketSender = mpCAmSocketRoutingSender->getCSocketSenderPt();

	if(pCSocketSender == NULL){
		logError("IAmSocketRoutingReceiverShadow::",__func__," pCSocketSender is NULL");
		return;
	}

	/* 受信データバッファの初期化 */
	memset(&data, 0, sizeof(ST_CONFIRMROUTINGREADY));

	/* 受信データ取得 */
	memcpy(&data, msg, sizeof(ST_CONFIRMROUTINGREADY));

	/* RoutingPluginからControlPluginへ処理を移すため、ControlPluginのメソッドを呼び出す */
	mpIAmRoutingReceive->confirmRoutingReady(data.handle, data.error);

	/* 応答データ作成 */
	pCSocketSender->make_senddata(D_OPC_CONFIRMROUTINGREADY);

	/* PluginからClientへ非同期応答 */
	pCSocketSender->send_async( mCurrRecvAddr );

	logDebug("IAmSocketRoutingReceiverShadow::",__func__," called end");

	return;
}

void IAmSocketRoutingReceiverShadow::confirmRoutingRundown( void *msg )
{
	ST_CONFIRMROUTINGRUNDOWN		data;				// 受信データ
	CSocketSender *pCSocketSender = NULL;

	logDebug("IAmSocketRoutingReceiverShadow::",__func__," called start, RecvAddr=", mCurrRecvAddr);

	/* 引数チェック */
	if(msg == NULL){
		logError("IAmSocketRoutingReceiverShadow::",__func__," arg is NULL");
		return;
	}

	if(mpCAmSocketRoutingSender == NULL){
		logError("[multidomain]IAmSocketRoutingReceiverShadow::",__func__," CAmSocketRoutingSender is NULL");
		return;
	}

	/* CSocketSenderポインタをCAmSocketRoutingSenderから取得 */
	pCSocketSender = mpCAmSocketRoutingSender->getCSocketSenderPt();

	if(pCSocketSender == NULL){
		logError("IAmSocketRoutingReceiverShadow::",__func__," pCSocketSender is NULL");
		return;
	}

	/* 受信データバッファの初期化 */
	memset(&data, 0, sizeof(ST_CONFIRMROUTINGRUNDOWN));

	/* 受信データ取得 */
	memcpy(&data, msg, sizeof(ST_CONFIRMROUTINGRUNDOWN));

	/* RoutingPluginからControlPluginへ処理を移すため、ControlPluginのメソッドを呼び出す */
	mpIAmRoutingReceive->confirmRoutingRundown(data.handle, data.error);

	/* 応答データ作成 */
	pCSocketSender->make_senddata(D_OPC_CONFIRMROUTINGRUNDOWN);

	/* PluginからClientへ非同期応答 */
	pCSocketSender->send_async( mCurrRecvAddr );

	logDebug("IAmSocketRoutingReceiverShadow::",__func__," called end");

	return;
}

void IAmSocketRoutingReceiverShadow::updateGateway( void *msg )
{
	(void)msg;
	return;
}

void IAmSocketRoutingReceiverShadow::updateSink( void *msg )
{
	am_sinkID_t							sinkID;						// sinkID
	am_sinkClass_t						sinkclassID;				// sinkclassID
	vector<am_SoundProperty_s>			listSoundProperties;		// SoundProperty
	vector<am_CustomConnectionFormat_t>	listConnectionFormats;		// CustomConnectionFormat
	vector<am_MainSoundProperty_s>		listMainSoundProperties;	// MainSoundProperty

	uint16_t					loopCnt = 0;						// カウンタ
	am_Error_e					returnCode = E_UNKNOWN;				// 処理結果
	am_Error_e					ret = E_UNKNOWN;					// 内部処理結果
	ST_UPDATESINK				data;								// 受信データ
	CSocketSender *pCSocketSender = NULL;

	logDebug("IAmSocketRoutingReceiverShadow::",__func__," called start, RecvAddr=", mCurrRecvAddr);

	/* 引数チェック */
	if(msg == NULL){
		logError("IAmSocketRoutingReceiverShadow::",__func__," arg is NULL");
		return;
	}

	if(mpCAmSocketRoutingSender == NULL){
		logError("[multidomain]IAmSocketRoutingReceiverShadow::",__func__," CAmSocketRoutingSender is NULL");
		return;
	}

	/* CSocketSenderポインタをCAmSocketRoutingSenderから取得 */
	pCSocketSender = mpCAmSocketRoutingSender->getCSocketSenderPt();

	if(pCSocketSender == NULL){
		logError("IAmSocketRoutingReceiverShadow::",__func__," pCSocketSender is NULL");
		return;
	}

	/* 初期化 */
	memset(&data, 0, sizeof(ST_UPDATESINK));
	listSoundProperties.clear();
	listConnectionFormats.clear();
	listMainSoundProperties.clear();

	/* 受信データ取得 */
	memcpy(&data, msg, sizeof(ST_UPDATESINK));

	sinkID = data.sinkID;
	sinkclassID = data.sinkclassID;

	for( loopCnt=0; loopCnt < D_AM_UPDATESINK_LISTNUM; loopCnt++ ){
		listSoundProperties.push_back( data.listsoundproperties[loopCnt] );
		listConnectionFormats.push_back( data.listconnectionformats[loopCnt] );
		listMainSoundProperties.push_back( data.listmainsoundproperties[loopCnt] );
	}

	returnCode = mpIAmRoutingReceive->updateSink(sinkID, sinkclassID, listSoundProperties, listConnectionFormats, listMainSoundProperties);
	if(returnCode != E_OK){
		logError("IAmSocketRoutingReceiverShadow::",__func__, " Error =", returnCode);
	}

	/* 応答用 実行結果設定 */
	data.error = returnCode;

	/* 応答データ作成 */
	pCSocketSender->make_senddata(D_OPC_UPDATESINK, (void*)&data, sizeof(data));

	/* PluginからClientへ同期応答 */
	ret = pCSocketSender->send_sync_reply( mCurrRecvAddr );
	if(ret != E_OK){
		logError(__func__, " send Error =", ret);
	}
	
	logDebug("IAmSocketRoutingReceiverShadow::",__func__," called end");

	return;
}

void IAmSocketRoutingReceiverShadow::updateSource( void *msg )
{
	(void)msg;
	return;
}

void IAmSocketRoutingReceiverShadow::ackSetVolumes( void *msg )
{
	vector<am_Volumes_s>		listvolumes;						// listvolumes

	uint16_t					loopCnt = 0;						// カウンタ
	am_Error_e					ret = E_UNKNOWN;					// 内部処理結果
	ST_ACKSETVOLUMES			data;								// 受信データ
	CSocketSender *pCSocketSender = NULL;

	logDebug("IAmSocketRoutingReceiverShadow::",__func__," called start, RecvAddr=", mCurrRecvAddr);

	/* 引数チェック */
	if(msg == NULL){
		logError("IAmSocketRoutingReceiverShadow::",__func__," arg is NULL");
		return;
	}

	/* 初期化 */
	memset(&data, 0, sizeof(ST_ACKSETVOLUMES));
	listvolumes.clear();

	if(mpCAmSocketRoutingSender == NULL){
		logError("[multidomain]IAmSocketRoutingReceiverShadow::",__func__," CAmSocketRoutingSender is NULL");
		return;
	}

	/* CSocketSenderポインタをCAmSocketRoutingSenderから取得 */
	pCSocketSender = mpCAmSocketRoutingSender->getCSocketSenderPt();

	if(pCSocketSender == NULL){
		logError("IAmSocketRoutingReceiverShadow::",__func__," pCSocketSender is NULL");
		return;
	}

	/* 受信データ取得 */
	memcpy(&data, msg, sizeof(ST_ACKSETVOLUMES));

	for( loopCnt=0; loopCnt < D_AM_VOLUMES_LISTNUM; loopCnt++ ){
		listvolumes.push_back( data.listvolumes[loopCnt] );
	}

	mpIAmRoutingReceive->ackSetVolumes(data.handle, listvolumes, data.error);

	/* 応答データ作成 */
	pCSocketSender->make_senddata(D_OPC_ACKSETVOLUMES, (void*)&data, sizeof(data));

	/* PluginからClientへ非同期応答 */
	ret = pCSocketSender->send_async( mCurrRecvAddr );
	if(ret != E_OK){
		logError("IAmSocketRoutingReceiverShadow::",__func__, " send Error =", ret);
	}
	logDebug("IAmSocketRoutingReceiverShadow::",__func__," called end");

	return;
}

void IAmSocketRoutingReceiverShadow::ackSinkNotificationConfiguration( void *msg )
{
	am_Error_e				ret = E_UNKNOWN;		// 内部処理結果
	ST_ACKSINKNOTIFICATIONCONFIGURATION		data;		// 受信データ
	CSocketSender *pCSocketSender = NULL;

	logDebug("IAmSocketRoutingReceiverShadow::",__func__," called start, RecvAddr=", mCurrRecvAddr);

	/* 引数チェック */
	if(msg == NULL){
		logError("IAmSocketRoutingReceiverShadow::",__func__," arg is NULL");
		return;
	}

	if(mpCAmSocketRoutingSender == NULL){
		logError("[multidomain]IAmSocketRoutingReceiverShadow::",__func__," CAmSocketRoutingSender is NULL");
		return;
	}

	/* CSocketSenderポインタをCAmSocketRoutingSenderから取得 */
	pCSocketSender = mpCAmSocketRoutingSender->getCSocketSenderPt();

	if(pCSocketSender == NULL){
		logError("IAmSocketRoutingReceiverShadow::",__func__," pCSocketSender is NULL");
		return;
	}

	/* 初期化 */
	memset(&data, 0, sizeof(ST_ACKSINKNOTIFICATIONCONFIGURATION));

	/* 受信データ取得 */
	memcpy(&data, msg, sizeof(ST_ACKSINKNOTIFICATIONCONFIGURATION));

	mpIAmRoutingReceive->ackSinkNotificationConfiguration(data.handle, data.error);

	/* 応答データ作成 */
	pCSocketSender->make_senddata(D_OPC_ACKSINKNOTIFICATIONCONFIGURATION, (void*)&data, sizeof(data));

	/* PluginからClientへ非同期応答 */
	ret = pCSocketSender->send_async( mCurrRecvAddr );
	if(ret != E_OK){
		logError("IAmSocketRoutingReceiverShadow::",__func__, " send Error =", ret);
	}
	logDebug("IAmSocketRoutingReceiverShadow::",__func__," called end");

	return;
}

void IAmSocketRoutingReceiverShadow::ackSourceNotificationConfiguration( void *msg )
{
	am_Error_e			ret = E_UNKNOWN;		// 内部処理結果
	ST_ACKSOURCENOTIFICATIONCONFIGURATION		data;		// 受信データ
	CSocketSender *pCSocketSender = NULL;

	logDebug("IAmSocketRoutingReceiverShadow::",__func__," called start, RecvAddr=", mCurrRecvAddr);

	/* 引数チェック */
	if(msg == NULL){
		logError("IAmSocketRoutingReceiverShadow::",__func__," arg is NULL");
		return;
	}

	if(mpCAmSocketRoutingSender == NULL){
		logError("[multidomain]IAmSocketRoutingReceiverShadow::",__func__," CAmSocketRoutingSender is NULL");
		return;
	}

	/* CSocketSenderポインタをCAmSocketRoutingSenderから取得 */
	pCSocketSender = mpCAmSocketRoutingSender->getCSocketSenderPt();

	if(pCSocketSender == NULL){
		logError("IAmSocketRoutingReceiverShadow::",__func__," pCSocketSender is NULL");
		return;
	}

	/* 初期化 */
	memset(&data, 0, sizeof(ST_ACKSOURCENOTIFICATIONCONFIGURATION));

	/* 受信データ取得 */
	memcpy(&data, msg, sizeof(ST_ACKSOURCENOTIFICATIONCONFIGURATION));

	mpIAmRoutingReceive->ackSourceNotificationConfiguration(data.handle, data.error);

	/* 応答データ作成 */
	pCSocketSender->make_senddata(D_OPC_ACKSOURCENOTIFICATIONCONFIGURATION, (void*)&data, sizeof(data));

	/* PluginからClientへ非同期応答 */
	ret = pCSocketSender->send_async( mCurrRecvAddr );
	if(ret != E_OK){
		logError("IAmSocketRoutingReceiverShadow::",__func__, " send Error =", ret);
	}
	logDebug("IAmSocketRoutingReceiverShadow::",__func__," called end");

	return;
}

void IAmSocketRoutingReceiverShadow::hookSinkNotificationDataChange( void *msg )
{
	(void)msg;
	return;
}

void IAmSocketRoutingReceiverShadow::hookSourceNotificationDataChange( void *msg )
{
	(void)msg;
	return;
}

void IAmSocketRoutingReceiverShadow::getInterfaceVersion( void *msg )
{
	(void)msg;
	return;
}

/****************************************************************/
/* getRoutingReady実行											*/
/* name:		getRoutingReady									*/
/* overview:	受信したgetRoutingReadyの処理を実行する			*/
/* param:		msg : 受信メッセージのデータ部					*/
/* ret:			なし											*/
/****************************************************************/
void IAmSocketRoutingReceiverShadow::getRoutingReady( void *msg )
{
	am_Error_e	ret = E_UNKNOWN;		// 内部処理結果
	ST_GETROUTINGREADY data;
    bool routingReady = false;
	CSocketSender *pCSocketSender = NULL;

	logInfo("IAmSocketRoutingReceiverShadow::",__func__," called start");

	/* 引数チェック */
	if(msg == NULL){
		logError("IAmSocketRoutingReceiverShadow::",__func__," arg is NULL");
		return;
	}

	if(mpCAmSocketRoutingSender == NULL){
		logError("[multidomain]IAmSocketRoutingReceiverShadow::",__func__," CAmSocketRoutingSender is NULL");
		return;
	}

	/* CSocketSenderポインタをCAmSocketRoutingSenderから取得 */
	pCSocketSender = mpCAmSocketRoutingSender->getCSocketSenderPt();

	if(pCSocketSender == NULL){
		logError("IAmSocketRoutingReceiverShadow::",__func__," pCSocketSender is NULL");
		return;
	}

	/* 内部変数初期化 */
	memset(&data, 0, sizeof(ST_GETROUTINGREADY));

	routingReady = mpCAmSocketRoutingSender->getRoutingReady();

	/* 処理結果を設定 */
	data.routingReady = routingReady;

	/* 応答データ作成 */
	pCSocketSender->make_senddata(D_OPC_GETROUTINGREADY, (void*)&data, sizeof(data));

	/* PluginからClientへ同期応答 */
    ret = pCSocketSender->send_sync_reply( mCurrRecvAddr );
	if(ret != E_OK){
		logError("IAmSocketRoutingReceiverShadow::",__func__, " send Error =", ret);
	}
	logDebug("IAmSocketRoutingReceiverShadow::",__func__," called end");

	return;
}

/****************************************************************/
/* 受信メッセージ振り分け										*/
/* name:		receiveCallback									*/
/* overview:	受信したメッセージを対応する処理に振り分ける	*/
/* param:		pmsg : 受信メッセージ（ヘッダ部/データ部)		*/
/* ret:			なし											*/
/****************************************************************/
void IAmSocketRoutingReceiverShadow::receiveCallback(void* pmsg)
{
	ST_SOCKET_MSG *message;
	unsigned short opc = 0;

	logDebug("IAmSocketRoutingReceiverShadow::",__func__," called start, RecvAddr=", mCurrRecvAddr);

	/* 引数のチェック */
	if(pmsg == NULL){
		logError("IAmSocketRoutingReceiverShadow::",__func__," No msg");
		return;
	}

	if(mpCAmSocketRoutingSender == NULL){
		logError("[multidomain]IAmSocketRoutingReceiverShadow::",__func__," CAmSocketRoutingSender is NULL");
		return;
	}

	/* 初期化 */
	message = (ST_SOCKET_MSG*)pmsg;

	functionMap_t::iterator iter = mFunctionMap.begin();
	opc = (unsigned short)AM_SWAP_ENDIAN16(message->opc); /* opc取得 */
	/* opcに対応する処理の検索 */
    iter = mFunctionMap.find(opc);
    if (iter == mFunctionMap.end()){
		/* no processing */
    }
	else{
		/* save source address as receiving address */
		mCurrRecvAddr = (unsigned short)AM_SWAP_ENDIAN16(message->srcaddr);
		/* notification receiving forwarding address */
		mpCAmSocketRoutingSender->notificationRecvDest(mCurrRecvAddr);
		/* opcに対応する処理へ振り分け */
    	CallBackMethod cb = iter->second;
        (this->*cb)((void*)&(message->data));
    }
	logDebug("IAmSocketRoutingReceiverShadow::",__func__," called end");

    return;
}

/****************************************************************************************************/
/* IAmSocketRoutingReceiverShadowの初期化を行う														*/
/* name:		setRoutingReceiver																	*/
/* overview:	受信コールバックの登録依頼及びクラスポインタのセットを行う							*/
/* param:		receiver：IAmRoutingReceiveクラス(CAmRoutingReceiverクラスの継承元）のポインタ		*/
/* ret:			なし																				*/
/****************************************************************************************************/
void IAmSocketRoutingReceiverShadow::setRoutingReceiver( IAmRoutingReceive*& receiver )
{
	logDebug("IAmSocketRoutingReceiverShadow::",__func__," called start, RecvAddr=", mCurrRecvAddr);

	/* 引数チェック */
	if( receiver == NULL ){
	    logError("IAmSocketRoutingReceiverShadow::",__func__," arg is NULL reciever:", receiver);
		return;
	}

	/* 引数をメンバ変数へ保存 */
	mpIAmRoutingReceive = receiver;

	if( NULL != mpCAmSocketRoutingSender ){
		//CAmSocketRoutingSenderクラスへ受信コールバック登録依頼
		mpCAmSocketRoutingSender->registRoutingReceiver( &mpRecieveCallBack );
	}
	else{
		logError("IAmSocketRoutingReceiverShadow::",__func__," mpCAmSocketRoutingSender is NULL");
	}
	logDebug("IAmSocketRoutingReceiverShadow::",__func__," called end");

	return;
}

IAmSocketRoutingReceiverShadow::functionMap_t IAmSocketRoutingReceiverShadow::createMap()
{
    functionMap_t m;
	m[D_OPC_ACKCONNECT] = &IAmSocketRoutingReceiverShadow::ackConnect;
    m[D_OPC_ACKDISCONNECT] = &IAmSocketRoutingReceiverShadow::ackDisconnect;
    m[D_OPC_ACKSETSINKVOLUMECHANGE] = &IAmSocketRoutingReceiverShadow::ackSetSinkVolumeChange;
    m[D_OPC_ACKSETSOURCEVOLUMECHANGE] = &IAmSocketRoutingReceiverShadow::ackSetSourceVolumeChange;
    m[D_OPC_ACKSETSOURCESTATE] = &IAmSocketRoutingReceiverShadow::ackSetSourceState;
    m[D_OPC_ACKSINKVOLUMETICK] = &IAmSocketRoutingReceiverShadow::ackSinkVolumeTick;
    m[D_OPC_ACKSOURCEVOLUMETICK] = &IAmSocketRoutingReceiverShadow::ackSourceVolumeTick;
    m[D_OPC_ACKSETSINKSOUNDPROPERTY] = &IAmSocketRoutingReceiverShadow::ackSetSinkSoundProperty;
    m[D_OPC_ACKSETSOURCESOUNDPROPERTY] = &IAmSocketRoutingReceiverShadow::ackSetSourceSoundProperty;
    m[D_OPC_ACKSETSINKSOUNDPROPERTIES] = &IAmSocketRoutingReceiverShadow::ackSetSinkSoundProperties;
    m[D_OPC_ACKSETSOURCESOUNDPROPERTIES] = &IAmSocketRoutingReceiverShadow::ackSetSourceSoundProperties;
    m[D_OPC_ACKCROSSFADING] = &IAmSocketRoutingReceiverShadow::ackCrossFading;
    m[D_OPC_REGISTERDOMAIN] = &IAmSocketRoutingReceiverShadow::registerDomain;
    m[D_OPC_REGISTERSOURCE] = &IAmSocketRoutingReceiverShadow::registerSource;
    m[D_OPC_REGISTERSINK] = &IAmSocketRoutingReceiverShadow::registerSink;
    m[D_OPC_REGISTERGATEWAY] = &IAmSocketRoutingReceiverShadow::registerGateway;
    m[D_OPC_PEEKDOMAIN] = &IAmSocketRoutingReceiverShadow::peekDomain;
    m[D_OPC_DEREGISTERDOMAIN] = &IAmSocketRoutingReceiverShadow::deregisterDomain;
    m[D_OPC_DEREGISTERGATEWAY] = &IAmSocketRoutingReceiverShadow::deregisterGateway;
    m[D_OPC_PEEKSINK] = &IAmSocketRoutingReceiverShadow::peekSink;
    m[D_OPC_DEREGISTERSINK] = &IAmSocketRoutingReceiverShadow::deregisterSink;
    m[D_OPC_PEEKSOURCE] = &IAmSocketRoutingReceiverShadow::peekSource;
    m[D_OPC_DEREGISTERSOURCE] = &IAmSocketRoutingReceiverShadow::deregisterSource;
    m[D_OPC_REGISTERCROSSFADER] = &IAmSocketRoutingReceiverShadow::registerCrossfader;
    m[D_OPC_DEREGISTERCROSSFADER] = &IAmSocketRoutingReceiverShadow::deregisterCrossfader;
    m[D_OPC_PEEKSOURCECLASSID] = &IAmSocketRoutingReceiverShadow::peekSourceClassID;
    m[D_OPC_PEEKSINKCLASSID] = &IAmSocketRoutingReceiverShadow::peekSinkClassID;
    m[D_OPC_HOOKINTERRUPTSTATUSCHANGE] = &IAmSocketRoutingReceiverShadow::hookInterruptStatusChange;
    m[D_OPC_HOOKDOMAINREGISTRATIONCOMPLETE] = &IAmSocketRoutingReceiverShadow::hookDomainRegistrationComplete;
    m[D_OPC_HOOKSINKAVAILABLITYSTATUSCHANGE] = &IAmSocketRoutingReceiverShadow::hookSinkAvailablityStatusChange;
    m[D_OPC_HOOKSOURCEAVAILABLITYSTATUSCHANGE] = &IAmSocketRoutingReceiverShadow::hookSourceAvailablityStatusChange;
    m[D_OPC_HOOKDOMAINSTATECHANGE] = &IAmSocketRoutingReceiverShadow::hookDomainStateChange;
    m[D_OPC_HOOKTIMINGINFORMATIONCHANGED] = &IAmSocketRoutingReceiverShadow::hookTimingInformationChanged;
    m[D_OPC_SENDCHANGEDDATA] = &IAmSocketRoutingReceiverShadow::sendChangedData;
    m[D_OPC_CONFIRMROUTINGREADY] = &IAmSocketRoutingReceiverShadow::confirmRoutingReady;
    m[D_OPC_CONFIRMROUTINGRUNDOWN] = &IAmSocketRoutingReceiverShadow::confirmRoutingRundown;
    m[D_OPC_UPDATEGATEWAY] = &IAmSocketRoutingReceiverShadow::updateGateway;
    m[D_OPC_UPDATESINK] = &IAmSocketRoutingReceiverShadow::updateSink;
    m[D_OPC_UPDATESOURCE] = &IAmSocketRoutingReceiverShadow::updateSource;
    m[D_OPC_ACKSETVOLUMES] = &IAmSocketRoutingReceiverShadow::ackSetVolumes;
    m[D_OPC_ACKSINKNOTIFICATIONCONFIGURATION] = &IAmSocketRoutingReceiverShadow::ackSinkNotificationConfiguration;
    m[D_OPC_ACKSOURCENOTIFICATIONCONFIGURATION] = &IAmSocketRoutingReceiverShadow::ackSourceNotificationConfiguration;
    m[D_OPC_HOOKSINKNOTIFICATIONDATACHANGE] = &IAmSocketRoutingReceiverShadow::hookSinkNotificationDataChange;
    m[D_OPC_HOOKSOURCENOTIFICATIONDATACHANGE] = &IAmSocketRoutingReceiverShadow::hookSourceNotificationDataChange;
    m[D_OPC_GETINTERFACEVERSION] = &IAmSocketRoutingReceiverShadow::getInterfaceVersion;
    m[D_OPC_GETROUTINGREADY] = &IAmSocketRoutingReceiverShadow::getRoutingReady;
    return (m);
}
