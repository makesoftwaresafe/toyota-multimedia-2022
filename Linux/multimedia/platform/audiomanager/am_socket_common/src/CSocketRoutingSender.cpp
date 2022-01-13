/************************************************************************
 * @file: CSocketRoutingSender.cpp
 *
 * @version: 1.1
 *
 * @description: A CSocketRoutingSender class implementation of Routing Adapter.
 * A wrapper class for sender class. CSocketRoutingSender class will call the
 * sender class API which has the actual sender API definition.
 * @component: platform/audiomanager
 *
 * @author: Jens Lorenz, jlorenz@de.adit-jv.com 2016
 *          Mattia Guerra, mguerra@de.adit-jv.com 2016
 *
 * @copyright (c) 2016 Advanced Driver Information Technology.
 * This code is developed by Advanced Driver Information Technology.
 * Copyright of Advanced Driver Information Technology, Bosch, and DENSO.
 * All rights reserved.
 *
 * @see <related items>
 *
 * @history
 *
 ***********************************************************************/

#include <fstream> //for file operations
#include <stdexcept> // for runtime_error
#include <vector>
#include "audiomanagertypes.h"
#include "CAmDltWrapper.h"
#include "ISocketRoutingClient.h"
#include "CSocketRoutingSender.h"
#include <assert.h>
#include "CAmSocketWrapper.h"
#include <string.h>

using namespace std;
using namespace am;

DLT_DECLARE_CONTEXT (routingDbus)

CSocketRoutingSender::CSocketRoutingSender(ISocketRoutingClient* const client, CAmSocketWrapper*& wrapper) :
	mpRecieveCallBack(this, &CSocketRoutingSender::receiveCallback),
	mpCAmSocketWrapper(wrapper),
	mpISocketRoutingClient(client),
	mFunctionMap(createMap())
{
	CAmDltWrapper::instance()->registerContext(routingDbus, "DRS", "DBus Plugin");
	try
	{
		logDebug("CSocketRoutingSender::",__func__," constructed");
		/* CSocketSenderクラスへのラッパクラスインスタンスポインタ設定		*/
		mCSocketSender.setSocketWrapper( wrapper );
	}
	catch (...)
	{
		logError("CSocketRoutingSender::",__func__," Failed to create object");
		this->~CSocketRoutingSender();
	}
}

CSocketRoutingSender::~CSocketRoutingSender()
{
	logInfo("CSocketRoutingSender::",__func__," Called");
    CAmDltWrapper::instance()->unregisterContext(routingDbus);
}

void CSocketRoutingSender::setRoutingReady(void *msg)
{
	(void)msg;

	logInfo("CSocketRoutingSender::",__func__," gets called");

	/* IAmRoutingClientからドメインのメソッドを呼び出すため、ここでの処理は不要 */
	return;
}

void CSocketRoutingSender::setRoutingRundown(void *msg)
{
	ST_SETROUTINGRUNDOWN	data;		// 受信データ

	logInfo("CSocketRoutingSender::",__func__," gets called");

	/* 初期化 */
	memset(&data, 0, sizeof(ST_SETROUTINGRUNDOWN));

	/* 受信データ取得 */
	memcpy(&data, msg, sizeof(ST_SETROUTINGRUNDOWN));

	/* Socketデータ作成 */
	mCSocketSender.make_senddata(D_OPC_SETROUTINGRUNDOWN, (void*)&data, sizeof(data));

	/* ClientからPluginへ非同期応答送信 */
	mCSocketSender.send_async();

	/* 処理実行 */
	mpISocketRoutingClient->setRoutingRundown(data.handle);

	return;
}

void CSocketRoutingSender::asyncAbort(void *msg)
{
	am_Error_e		returnCode;	// 内部メソッド戻り値
	ST_ASYNCABORT	data;		// 受信データ

	logInfo("CSocketRoutingSender::",__func__," gets called");

	/* 初期化 */
	memset(&data, 0, sizeof(ST_ASYNCABORT));

	/* 受信データ取得 */
	memcpy(&data, msg, sizeof(ST_ASYNCABORT));

	/* 処理実行 */
	returnCode = mpISocketRoutingClient->asyncAbort(data.handle);

	/* 処理結果設定 */
	data.error = returnCode;

	/* Socketデータ作成 */
	mCSocketSender.make_senddata(D_OPC_ASYNCABORT, (void*)&data, sizeof(data));

	/* 処理結果をClientからPluginへ非同期応答送信 */
	mCSocketSender.send_async();

	return;
}

void CSocketRoutingSender::asyncConnect(void *msg)
{
	am_Error_e					returnCode;			// 内部メソッド戻り値
	ST_ASYNCCONNECT				data;				// 受信データ

	logInfo("CSocketRoutingSender::",__func__," gets called");

	/* 初期化 */
	memset(&data, 0, sizeof(ST_ASYNCCONNECT));

	/* 受信データ取得 */
	memcpy(&data, msg, sizeof(ST_ASYNCCONNECT));

	/* 処理実行 */
	returnCode = mpISocketRoutingClient->asyncConnect(data.handle, data.connectionID, data.sourceID, data.sinkID, data.connectionFormat);

	/* 処理結果設定 */
	data.error = returnCode;

	/* Socketデータ作成 */
	mCSocketSender.make_senddata(D_OPC_ASYNCCONNECT, (void*)&data, sizeof(data));

	/* 処理結果をClientからPluginへ非同期応答送信 */
	mCSocketSender.send_async();

	return;
}

void CSocketRoutingSender::asyncDisconnect(void *msg)
{
	am_Error_e					returnCode;			// 内部メソッド戻り値
	ST_ASYNCDISCONNECT			data;				// 受信データ

	logInfo("CSocketRoutingSender::",__func__," gets called");

	/* 初期化 */
	memset(&data, 0, sizeof(ST_ASYNCDISCONNECT));

	/* 受信データ取得 */
	memcpy(&data, msg, sizeof(ST_ASYNCDISCONNECT));

	/* 処理実行 */
	returnCode = mpISocketRoutingClient->asyncDisconnect(data.handle, data.connectionID);

	/* 処理結果設定 */
	data.error = returnCode;

	/* Socketデータ作成 */
	mCSocketSender.make_senddata(D_OPC_ASYNCDISCONNECT, (void*)&data, sizeof(data));

	/* 処理結果をClientからPluginへ非同期応答送信 */
	mCSocketSender.send_async();

	return;
}

void CSocketRoutingSender::asyncSetSinkVolume(void *msg)
{
	am_Error_e					returnCode;			// 内部メソッド戻り値
	ST_ASYNCSETSINKVOLUME		data;				// 受信データ

	logInfo("CSocketRoutingSender::",__func__," gets called");

	/* 初期化 */
	memset(&data, 0, sizeof(ST_ASYNCSETSINKVOLUME));

	/* 受信データ取得 */
	memcpy(&data, msg, sizeof(ST_ASYNCSETSINKVOLUME));

	/* 処理実行 */
	returnCode = mpISocketRoutingClient->asyncSetSinkVolume(data.handle, data.sinkID, data.volume, data.ramp, data.time);

	/* 処理結果設定 */
	data.error = returnCode;

	/* Socketデータ作成 */
	mCSocketSender.make_senddata(D_OPC_ASYNCSETSINKVOLUME, (void*)&data, sizeof(data));

	/* 処理結果をClientからPluginへ非同期応答送信 */
	mCSocketSender.send_async();

	return;
}

void CSocketRoutingSender::asyncSetSourceVolume(void *msg)
{
	am_Error_e					returnCode;			// 内部メソッド戻り値
	ST_ASYNCSETSOURCEVOLUME		data;				// 受信データ

	logInfo("CSocketRoutingSender::",__func__," gets called");

	/* 初期化 */
	memset(&data, 0, sizeof(ST_ASYNCSETSOURCEVOLUME));

	/* 受信データ取得 */
	memcpy(&data, msg, sizeof(ST_ASYNCSETSOURCEVOLUME));

	/* 処理実行 */
	returnCode = mpISocketRoutingClient->asyncSetSourceVolume(data.handle, data.sourceID, data.volume, data.ramp, data.time);

	/* 処理結果設定 */
	data.error = returnCode;

	/* Socketデータ作成 */
	mCSocketSender.make_senddata(D_OPC_ASYNCSETSOURCEVOLUME, (void*)&data, sizeof(data));

	/* 処理結果をClientからPluginへ非同期応答送信 */
	mCSocketSender.send_async();

	return;
}

void CSocketRoutingSender::asyncSetSourceState(void *msg)
{
	am_Error_e					returnCode;			// 内部メソッド戻り値
	ST_ASYNCSETSOURCESTATE		data;				// 受信データ

	logInfo("CSocketRoutingSender::",__func__," gets called");

	/* 初期化 */
	memset(&data, 0, sizeof(ST_ASYNCSETSOURCESTATE));

	/* 受信データ取得 */
	memcpy(&data, msg, sizeof(ST_ASYNCSETSOURCESTATE));

	/* 処理実行 */
	returnCode = mpISocketRoutingClient->asyncSetSourceState(data.handle, data.sourceID, data.state);

	/* 処理結果設定 */
	data.error = returnCode;

	/* Socketデータ作成 */
	mCSocketSender.make_senddata(D_OPC_ASYNCSETSOURCESTATE, (void*)&data, sizeof(data));

	/* 処理結果をClientからPluginへ非同期応答送信 */
	mCSocketSender.send_async();

	return;
}

void CSocketRoutingSender::asyncSetSinkSoundProperties(void *msg)
{
	vector<am_SoundProperty_s>		listSoundProperties;		// SoundProperty

	uint16_t						loopCnt = 0;				// カウンタ
	am_Error_e						returnCode = E_UNKNOWN;		// 処理結果
	ST_ASYNCSETSINKSOUNDPROPERTIES	data;				// 受信データ

	logInfo("CSocketRoutingSender::",__func__," gets called");

	/* 引数チェック */
	if(msg == NULL){
		logError("CSocketRoutingSender::",__func__," arg is NULL");
		return;
	}

	/* 初期化 */
	memset(&data, 0, sizeof(ST_ASYNCSETSINKSOUNDPROPERTIES));
	listSoundProperties.clear();

	/* 受信データ取得 */
	memcpy(&data, msg, sizeof(ST_ASYNCSETSINKSOUNDPROPERTIES));

	for( loopCnt=0; loopCnt < D_AM_SOUNDPROPERTIES_LISTNUM; loopCnt++ ){
		listSoundProperties.push_back( data.listSoundProperties[loopCnt] );
	}

	returnCode = mpISocketRoutingClient->asyncSetSinkSoundProperties(data.handle, data.sinkID, listSoundProperties);
	if(returnCode != E_OK){
		logError("CSocketRoutingSender::",__func__, " Error =", returnCode);
	}

	/* 応答用 実行結果設定 */
	data.error = returnCode;

	/* 応答データ作成 */
	mCSocketSender.make_senddata(D_OPC_ASYNCSETSINKSOUNDPROPERTIES, (void*)&data, sizeof(data));

	/* 処理結果をClientからPluginへ非同期応答送信 */
	mCSocketSender.send_async();

	return;
}
void CSocketRoutingSender::asyncSetSinkSoundProperty(void *msg)
{
	am_Error_e						returnCode;			// 内部メソッド戻り値
	ST_ASYNCSETSINKSOUNDPROPERTY	data;				// 受信データ

	logInfo("CSocketRoutingSender::",__func__," gets called");

	/* 初期化 */
	memset(&data, 0, sizeof(ST_ASYNCSETSINKSOUNDPROPERTY));

	/* 受信データ取得 */
	memcpy(&data, msg, sizeof(ST_ASYNCSETSINKSOUNDPROPERTY));

	/* 処理実行 */
	returnCode = mpISocketRoutingClient->asyncSetSinkSoundProperty(data.handle, data.sinkID, data.soundProperty);

	/* 処理結果設定 */
	data.error = returnCode;

	/* Socketデータ作成 */
	mCSocketSender.make_senddata(D_OPC_ASYNCSETSINKSOUNDPROPERTY, (void*)&data, sizeof(data));

	/* 処理結果をClientからPluginへ非同期応答送信 */
	mCSocketSender.send_async();

	return;
}

void CSocketRoutingSender::asyncSetSourceSoundProperties(void *msg)
{
	vector<am_SoundProperty_s>		listSoundProperties;		// SoundProperty

	uint16_t						loopCnt = 0;				// カウンタ
	am_Error_e						returnCode = E_UNKNOWN;		// 処理結果
	ST_ASYNCSETSOURCESOUNDPROPERTIES	data;			// 受信データ

	logInfo("CSocketRoutingSender::",__func__," gets called");

	/* 引数チェック */
	if(msg == NULL){
		logError("CSocketRoutingSender::",__func__," arg is NULL");
		return;
	}

	/* 初期化 */
	memset(&data, 0, sizeof(ST_ASYNCSETSOURCESOUNDPROPERTIES));
	listSoundProperties.clear();

	/* 受信データ取得 */
	memcpy(&data, msg, sizeof(ST_ASYNCSETSOURCESOUNDPROPERTIES));

	for( loopCnt=0; loopCnt < D_AM_SOUNDPROPERTIES_LISTNUM; loopCnt++ ){
		listSoundProperties.push_back( data.listSoundProperties[loopCnt] );
	}

	returnCode = mpISocketRoutingClient->asyncSetSourceSoundProperties(data.handle, data.sourceID, listSoundProperties);
	if(returnCode != E_OK){
		logError("CSocketRoutingSender::",__func__, " Error =", returnCode);
	}

	/* 応答用 実行結果設定 */
	data.error = returnCode;

	/* 応答データ作成 */
	mCSocketSender.make_senddata(D_OPC_ASYNCSETSOURCESOUNDPROPERTIES, (void*)&data, sizeof(data));

	/* 処理結果をClientからPluginへ非同期応答送信 */
	mCSocketSender.send_async();

	return;
}

void CSocketRoutingSender::asyncSetSourceSoundProperty(void *msg)
{
	am_Error_e						returnCode = E_UNKNOWN;			// 内部メソッド戻り値
	ST_ASYNCSETSOURCESOUNDPROPERTY	data;				// 受信データ

	logInfo("CSocketRoutingSender::",__func__," gets called");

	/* 初期化 */
	memset(&data, 0, sizeof(ST_ASYNCSETSOURCESOUNDPROPERTY));

	/* 受信データ取得 */
	memcpy(&data, msg, sizeof(ST_ASYNCSETSOURCESOUNDPROPERTY));

	/* 処理実行 */
	returnCode = mpISocketRoutingClient->asyncSetSourceSoundProperty(data.handle, data.sourceID, data.soundProperty);

	/* 処理結果設定 */
	data.error = returnCode;

	/* Socketデータ作成 */
	mCSocketSender.make_senddata(D_OPC_ASYNCSETSOURCESOUNDPROPERTY, (void*)&data, sizeof(data));

	/* 処理結果をClientからPluginへ非同期応答送信 */
	mCSocketSender.send_async();

	return;
}

void CSocketRoutingSender::asyncSetVolumes(void *msg)
{
	vector<am_Volumes_s> listVolumes;

	uint16_t			loopCnt = 0;				// カウンタ
	am_Error_e			returnCode = E_UNKNOWN;			// 内部メソッド戻り値
	ST_ASYNCSETVOLUMES	data;				// 受信データ

	logInfo("CSocketRoutingSender::",__func__," gets called");

	/* 初期化 */
	memset(&data, 0, sizeof(ST_ASYNCSETVOLUMES));

	/* 受信データ取得 */
	memcpy(&data, msg, sizeof(ST_ASYNCSETVOLUMES));

	for( loopCnt=0; loopCnt < D_AM_VOLUMES_LISTNUM; loopCnt++ ){
		listVolumes.push_back( data.listVolumes[loopCnt] );
	}

	/* 処理実行 */
	returnCode = mpISocketRoutingClient->asyncSetVolumes(data.handle, listVolumes);

	/* 処理結果設定 */
	data.error = returnCode;

	/* Socketデータ作成 */
	mCSocketSender.make_senddata(D_OPC_ASYNCSETVOLUMES, (void*)&data, sizeof(data));

	/* 処理結果をClientからPluginへ非同期応答送信 */
	mCSocketSender.send_async();

	return;
}

void CSocketRoutingSender::asyncSetSinkNotificationConfiguration(void *msg)
{
	am_Error_e										returnCode = E_UNKNOWN;			// 内部メソッド戻り値
	ST_ASYNCSETSINKNOTIFICATIONCONFIGURATION		data;				// 受信データ

	logInfo("CSocketRoutingSender::",__func__," gets called");

	/* 初期化 */
	memset(&data, 0, sizeof(ST_ASYNCSETSINKNOTIFICATIONCONFIGURATION));

	/* 受信データ取得 */
	memcpy(&data, msg, sizeof(ST_ASYNCSETSINKNOTIFICATIONCONFIGURATION));

	/* 処理実行 */
	returnCode = mpISocketRoutingClient->asyncSetSinkNotificationConfiguration(data.handle, data.sinkID, data.notifiConf);

	/* 処理結果設定 */
	data.error = returnCode;

	/* Socketデータ作成 */
	mCSocketSender.make_senddata(D_OPC_ASYNCSETSINKNOTIFICATIONCONFIGURATION, (void*)&data, sizeof(data));

	/* 処理結果をClientからPluginへ非同期応答送信 */
	mCSocketSender.send_async();

	return;
}

void CSocketRoutingSender::asyncSetSourceNotificationConfiguration(void *msg)
{
	am_Error_e										returnCode = E_UNKNOWN;			// 内部メソッド戻り値
	ST_ASYNCSETSOURCENOTIFICATIONCONFIGURATION		data;				// 受信データ

	logInfo("CSocketRoutingSender::",__func__," gets called");

	/* 初期化 */
	memset(&data, 0, sizeof(ST_ASYNCSETSOURCENOTIFICATIONCONFIGURATION));

	/* 受信データ取得 */
	memcpy(&data, msg, sizeof(ST_ASYNCSETSOURCENOTIFICATIONCONFIGURATION));

	/* 処理実行 */
	returnCode = mpISocketRoutingClient->asyncSetSourceNotificationConfiguration(data.handle, data.sourceID, data.notifiConf);

	/* 処理結果設定 */
	data.error = returnCode;

	/* Socketデータ作成 */
	mCSocketSender.make_senddata(D_OPC_ASYNCSETSOURCENOTIFICATIONCONFIGURATION, (void*)&data, sizeof(data));

	/* 処理結果をClientからPluginへ非同期応答送信 */
	mCSocketSender.send_async();

	return;
}

void CSocketRoutingSender::asyncCrossFade(void *msg)
{
	am_Error_e			returnCode = E_UNKNOWN;			// 内部メソッド戻り値
	ST_ASYNCCROSSFADE	data;				// 受信データ

	logInfo("CSocketRoutingSender::",__func__," gets called");
	
	/* 初期化 */
	memset(&data, 0, sizeof(ST_ASYNCCROSSFADE));

	/* 受信データ取得 */
	memcpy(&data, msg, sizeof(ST_ASYNCCROSSFADE));
	
	/* 処理実行 */
	returnCode = mpISocketRoutingClient->asyncCrossFade(data.handle, data.crossfaderID, data.hotSink, data.rampType, data.amTime);

	/* 処理結果設定 */
	data.error = returnCode;

	/* Socketデータ作成 */
	mCSocketSender.make_senddata(D_OPC_ASYNCCROSSFADE, (void*)&data, sizeof(data));
	
	/* 処理結果をClientからPluginへ非同期応答送信 */
	mCSocketSender.send_async();

	return;
}

void CSocketRoutingSender::setDomainState(void *msg)
{
	(void)msg;

	logInfo("CSocketRoutingSender::",__func__," gets called");

	return;
}

/****************************************************************/
/* 受信メッセージ振り分け										*/
/* name:		receiveCallback									*/
/* overview:	受信したメッセージを対応する処理に振り分ける	*/
/* param:		pmsg : 受信メッセージ（ヘッダ部/データ部)		*/
/* ret:			なし											*/
/****************************************************************/
void CSocketRoutingSender::receiveCallback(void* pmsg)
{
	ST_SOCKET_MSG *message;
	unsigned short opc = 0;

	logDebug("CSocketRoutingSender::",__func__," called");

	/* 引数のチェック */
	if(pmsg == NULL){
		logError("CSocketRoutingSender::",__func__," No msg");
		return;
	}
	
	/* 初期化 */
	message = (ST_SOCKET_MSG*)pmsg;

	functionMap_t::iterator iter = mFunctionMap.begin();
	opc = (unsigned short)AM_SWAP_ENDIAN16(message->opc); /* opc取得 */
	/* opcに対応する処理の検索 */
    iter = mFunctionMap.find(opc);
    if (iter == mFunctionMap.end())
	{
		/* 処理なし */
		logDebug("CSocketRoutingSender::",__func__," opc process is nothing opc=", opc);
	}
	else{
		/* opcに対応する処理へ振り分け */
        CallBackMethod cb = iter->second;
        (this->*cb)((void*)&(message->data));
    }

	return;
}

CSocketRoutingSender::functionMap_t CSocketRoutingSender::createMap()
{
    functionMap_t m;
    m[D_OPC_SETROUTINGREADY] = &CSocketRoutingSender::setRoutingReady;
    m[D_OPC_SETROUTINGRUNDOWN] = &CSocketRoutingSender::setRoutingRundown;
    m[D_OPC_ASYNCABORT] = &CSocketRoutingSender::asyncAbort;
    m[D_OPC_ASYNCCONNECT] = &CSocketRoutingSender::asyncConnect;
    m[D_OPC_ASYNCDISCONNECT] = &CSocketRoutingSender::asyncDisconnect;
    m[D_OPC_ASYNCSETSINKVOLUME] = &CSocketRoutingSender::asyncSetSinkVolume;
    m[D_OPC_ASYNCSETSOURCEVOLUME] = &CSocketRoutingSender::asyncSetSourceVolume;
    m[D_OPC_ASYNCSETSOURCESTATE] = &CSocketRoutingSender::asyncSetSourceState;
    m[D_OPC_ASYNCSETSINKSOUNDPROPERTIES] = &CSocketRoutingSender::asyncSetSinkSoundProperties;
    m[D_OPC_ASYNCSETSINKSOUNDPROPERTY] = &CSocketRoutingSender::asyncSetSinkSoundProperty;
    m[D_OPC_ASYNCSETSOURCESOUNDPROPERTIES] = &CSocketRoutingSender::asyncSetSourceSoundProperties;
    m[D_OPC_ASYNCSETSOURCESOUNDPROPERTY] = &CSocketRoutingSender::asyncSetSourceSoundProperty;
    m[D_OPC_ASYNCSETVOLUMES] = &CSocketRoutingSender::asyncSetVolumes;
    m[D_OPC_ASYNCSETSINKNOTIFICATIONCONFIGURATION] = &CSocketRoutingSender::asyncSetSinkNotificationConfiguration;
    m[D_OPC_ASYNCSETSOURCENOTIFICATIONCONFIGURATION] = &CSocketRoutingSender::asyncSetSourceNotificationConfiguration;
    return (m);
}

