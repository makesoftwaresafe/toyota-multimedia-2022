/************************************************************************
 * @file: ISocketRoutingReceiver.h
 *
 * @version: 1.1
 *
 * @description: A Receiver class shadow implementation of Routing Adapter.
 * Receiver class will make call to AM via DBus connection.
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

#include <string.h>
#include <fstream>
#include <stdexcept>
#include "ISocketRoutingReceiver.h"
#include "CSocketSender.h"
#include "CSocketCommon.h"

using namespace std;
using namespace am;

//For SA fix
//DLT_IMPORT_CONTEXT (RA_DBus_wrapper)

ISocketRoutingReceiver::ISocketRoutingReceiver( CAmSocketWrapper* wrapper )
{
	logDebug("ISocketRoutingReceiver::", __func__," called start, wrapper=", wrapper);
	mCSocketSender.setSocketWrapper(wrapper);
	logDebug("ISocketRoutingReceiver::", __func__," called end");
	return;
}

ISocketRoutingReceiver::~ISocketRoutingReceiver()
{
	logDebug("ISocketRoutingReceiver::", __func__," called");
	return;
}

void ISocketRoutingReceiver::ackConnect(const am_Handle_s handle, const am_connectionID_t connectionID, const am_Error_e error)
{
	am_Error_e ret = E_UNKNOWN;
	ST_ACKCONNECT data;			// 送信データ

	logDebug("ISocketRoutingReceiver::", __func__," called start, handle=", handle, ", connectionID=", connectionID, ", error=", error);

	/* 初期化 */
	memset(&data, 0, sizeof(ST_ACKCONNECT));

	/* 送信データ設定 */
	data.handle = handle;
	data.connectionID = connectionID;
	data.error = error;

	/* Socketデータ作成 */
	mCSocketSender.make_senddata(D_OPC_ACKCONNECT, (void*)&data, sizeof(data));

	/* ClientからPluginへ非同期送信 */
	ret = mCSocketSender.send_async();
	if(ret != E_OK){
		logError("ISocketRoutingReceiver::", __func__, " error, handle=", handle, ", connectionID=", connectionID, ", error=", error);
	}
	logDebug("ISocketRoutingReceiver::", __func__," called end, ret=", ret);

	return;
}

void ISocketRoutingReceiver::ackDisconnect(const am_Handle_s handle, const am_connectionID_t connectionID, const am_Error_e error)
{
	am_Error_e ret = E_UNKNOWN;
	ST_ACKDISCONNECT data;

	logDebug("ISocketRoutingReceiver::", __func__," called start, handle=", handle, ", connectionID=", connectionID, ", error=", error);

	/* 初期化 */
	memset(&data, 0, sizeof(ST_ACKDISCONNECT));

	/* 送信データ設定 */
	data.handle = handle;
	data.connectionID = connectionID;
	data.error = error;

	/* Socketデータ作成 */
	mCSocketSender.make_senddata(D_OPC_ACKDISCONNECT, (void*)&data, sizeof(data));

	/* ClientからPluginへ非同期送信 */
	ret = mCSocketSender.send_async();
	if(ret != E_OK){
		logError("ISocketRoutingReceiver::", __func__, " error, handle=", handle, ", connectionID=", connectionID, ", error=", error);
	}
	logDebug("ISocketRoutingReceiver::", __func__," called end, ret=", ret);

	return;
}

void ISocketRoutingReceiver::ackSetSinkVolumeChange(const am_Handle_s handle, const am_volume_t volume, const am_Error_e error)
{
	am_Error_e ret = E_UNKNOWN;
	ST_ACKSETSINKVOLUMECHANGE data;

	logDebug("ISocketRoutingReceiver::", __func__," called start, handle=", handle, ", volume=", volume, ", error=", error);

	/* 初期化 */
	memset(&data, 0, sizeof(ST_ACKSETSINKVOLUMECHANGE));

	/* 送信データ設定 */
	data.handle = handle;
	data.volume = volume;
	data.error = error;

	/* Socketデータ作成 */
	mCSocketSender.make_senddata(D_OPC_ACKSETSINKVOLUMECHANGE, (void*)&data, sizeof(data));

	/* ClientからPluginへ非同期送信 */
	ret = mCSocketSender.send_async();
	if(ret != E_OK){
		logError("ISocketRoutingReceiver::", __func__, " error, handle=", handle, ", volume=", volume, ", error=", error);
	}
	logDebug("ISocketRoutingReceiver::", __func__," called end, ret=", ret);

	return;
}

void ISocketRoutingReceiver::ackSetSourceVolumeChange(const am_Handle_s handle, const am_volume_t volume, const am_Error_e error)
{
	am_Error_e ret = E_UNKNOWN;
	ST_ACKSETSOURCEVOLUMECHANGE data;

	logDebug("ISocketRoutingReceiver::", __func__," called start, handle=", handle, ", volume=", volume, ", error=", error);

	/* 初期化 */
	memset(&data, 0, sizeof(ST_ACKSETSOURCEVOLUMECHANGE));

	/* 送信データ設定 */
	data.handle = handle;
	data.volume = volume;
	data.error = error;

	/* Socketデータ作成 */
	mCSocketSender.make_senddata(D_OPC_ACKSETSOURCEVOLUMECHANGE, (void*)&data, sizeof(data));

	/* ClientからPluginへ非同期送信 */
	ret = mCSocketSender.send_async();
	if(ret != E_OK){
		logError("ISocketRoutingReceiver::", __func__, " error, handle=", handle, ", volume=", volume, ", error=", error);
	}
	logDebug("ISocketRoutingReceiver::", __func__," called end, ret=", ret);

	return;
}

void ISocketRoutingReceiver::ackSetSourceState(const am_Handle_s handle, const am_Error_e error)
{
	am_Error_e ret = E_UNKNOWN;
	ST_ACKSETSOURCESTATE data;

	logDebug("ISocketRoutingReceiver::", __func__," called start, handle=", handle, ", error=", error);

	/* 初期化 */
	memset(&data, 0, sizeof(ST_ACKSETSOURCESTATE));

	/* 送信データ設定 */
	data.handle = handle;
	data.error = error;

	/* Socketデータ作成 */
	mCSocketSender.make_senddata(D_OPC_ACKSETSOURCESTATE, (void*)&data, sizeof(data));

	/* ClientからPluginへ非同期送信 */
	ret = mCSocketSender.send_async();
	if(ret != E_OK){
		logError(__func__, " error, handle=", handle, ", error=", error);
	}
	logDebug("ISocketRoutingReceiver::", __func__," called end, ret=", ret);

	return;
}

void ISocketRoutingReceiver::ackSetSinkSoundProperties(const am_Handle_s handle, const am_Error_e error)
{
	am_Error_e ret = E_UNKNOWN;
	ST_ACKSETSINKSOUNDPROPERTIES data;

	logDebug("ISocketRoutingReceiver::", __func__," called start, handle=", handle, ", error=", error);

	/* 初期化 */
	memset(&data, 0, sizeof(ST_ACKSETSINKSOUNDPROPERTIES));

	/* 送信データ設定 */
	data.handle = handle;
	data.error = error;

	/* Socketデータ作成 */
	mCSocketSender.make_senddata(D_OPC_ACKSETSINKSOUNDPROPERTIES, (void*)&data, sizeof(data));

	/* ClientからPluginへ非同期送信 */
	ret = mCSocketSender.send_async();
	if(ret != E_OK){
		logError(__func__, " error, handle=", handle, ", error=", error);
	}
	logDebug("ISocketRoutingReceiver::", __func__," called end, ret=", ret);

	return;
}

void ISocketRoutingReceiver::ackSetSinkSoundProperty(const am_Handle_s handle, const am_Error_e error)
{
	am_Error_e ret = E_UNKNOWN;
	ST_ACKSETSINKSOUNDPROPERTY data;

	logDebug("ISocketRoutingReceiver::", __func__," called start, handle=", handle, ", error=", error);

	/* 初期化 */
	memset(&data, 0, sizeof(ST_ACKSETSINKSOUNDPROPERTY));

	/* 送信データ設定 */
	data.handle = handle;
	data.error = error;

	/* Socketデータ作成 */
	mCSocketSender.make_senddata(D_OPC_ACKSETSINKSOUNDPROPERTY, (void*)&data, sizeof(data));

	/* ClientからPluginへ非同期送信 */
	ret = mCSocketSender.send_async();
	if(ret != E_OK){
		logError(__func__, " error, handle=", handle, ", error=", error);
	}
	logDebug("ISocketRoutingReceiver::", __func__," called end, ret=", ret);

	return;
}

void ISocketRoutingReceiver::ackSetSourceSoundProperties(const am_Handle_s handle, const am_Error_e error)
{
	am_Error_e ret = E_UNKNOWN;
	ST_ACKSETSOURCESOUNDPROPERTIES data;

	logDebug("ISocketRoutingReceiver::", __func__," called start, handle=", handle, ", error=", error);

	/* 初期化 */
	memset(&data, 0, sizeof(ST_ACKSETSOURCESOUNDPROPERTIES));

	/* 送信データ設定 */
	data.handle = handle;
	data.error = error;

	/* Socketデータ作成 */
	mCSocketSender.make_senddata(D_OPC_ACKSETSOURCESOUNDPROPERTIES, (void*)&data, sizeof(data));

	/* ClientからPluginへ非同期送信 */
	ret = mCSocketSender.send_async();
	if(ret != E_OK){
		logError(__func__, " error, handle=", handle, ", error=", error);
	}
	logDebug("ISocketRoutingReceiver::", __func__," called end, ret=", ret);

	return;
}

void ISocketRoutingReceiver::ackSetSourceSoundProperty(const am_Handle_s handle, const am_Error_e error)
{
	am_Error_e ret = E_UNKNOWN;
	ST_ACKSETSOURCESOUNDPROPERTY data;

	logDebug("ISocketRoutingReceiver::", __func__," called start, handle=", handle, ", error=", error);

	/* 初期化 */
	memset(&data, 0, sizeof(ST_ACKSETSOURCESOUNDPROPERTY));

	/* 送信データ設定 */
	data.handle = handle;
	data.error = error;

	/* Socketデータ作成 */
	mCSocketSender.make_senddata(D_OPC_ACKSETSOURCESOUNDPROPERTY, (void*)&data, sizeof(data));

	/* ClientからPluginへ非同期送信 */
	ret = mCSocketSender.send_async();
	if(ret != E_OK){
		logError(__func__, " error, handle=", handle, ", error=", error);
	}
	logDebug("ISocketRoutingReceiver::", __func__," called end, ret=", ret);

	return;
}

void ISocketRoutingReceiver::ackCrossFading(const am_Handle_s handle, const am_HotSink_e hotSink, const am_Error_e error)
{
	am_Error_e ret = E_UNKNOWN;
	ST_ACKCROSSFADING data;

	logDebug("ISocketRoutingReceiver::", __func__," called start, handle=", handle, ", error=", error);

	/* 初期化 */
	memset(&data, 0, sizeof(ST_ACKCROSSFADING));

	/* 送信データ設定 */
	data.handle = handle;
	data.hotSink = hotSink;
	data.error = error;

	/* Socketデータ作成 */
	mCSocketSender.make_senddata(D_OPC_ACKCROSSFADING, (void*)&data, sizeof(data));

	/* ClientからPluginへ非同期送信 */
	ret = mCSocketSender.send_async();
	if(ret != E_OK){
		logError(__func__, " error, handle=", handle, ", error=", error);
	}
	logDebug("ISocketRoutingReceiver::", __func__," called end, ret=", ret);

	return;
}

void ISocketRoutingReceiver::ackSourceVolumeTick(const am_Handle_s handle, const am_sourceID_t sourceID, const am_volume_t volume)
{
	(void)handle;
	(void)sourceID;
	(void)volume;

	return;
}

void ISocketRoutingReceiver::ackSinkVolumeTick(const am_Handle_s handle, const am_sinkID_t sinkID, const am_volume_t volume)
{
	(void)handle;
	(void)sinkID;
	(void)volume;

	return;
}

am_Error_e ISocketRoutingReceiver::peekDomain(const string &name, am_domainID_t &domainID)
{
	(void)name;
	(void)domainID;

	return E_OK;
}

am_Error_e ISocketRoutingReceiver::registerDomain(const am_Domain_s &domainData, am_domainID_t &domainID)
{
	am_Error_e ret = E_UNKNOWN;
	ST_REGISTERDOMAIN data;
	std::size_t strlength = 0;

	logDebug("ISocketRoutingReceiver::", __func__," called start");

	/* 初期化 */
	memset(&data, 0, sizeof(ST_REGISTERDOMAIN));

	/* 送信データ設定 */
	data.domainID = domainData.domainID;					// am_domainID_t -> am_domainID_t

	strlength = domainData.name.length();
	domainData.name.copy( data.name, strlength );			// std::string -> cahr[]

	strlength = domainData.busname.length();
	domainData.busname.copy( data.busname, strlength );		// std::string -> cahr[]

	strlength = domainData.nodename.length();
	domainData.nodename.copy( data.nodename, strlength );	// std::string -> cahr[]

	data.early = domainData.early;							// bool -> bool
	data.complete = domainData.complete;					// bool -> bool
	data.state = domainData.state;							// am_DomainState_e -> am_DomainState_e

	/* Socketデータ作成 */
	mCSocketSender.make_senddata(D_OPC_REGISTERDOMAIN, (void*)&data, sizeof(data));

	/* ClientからPluginへ同期送信 */
	ret = mCSocketSender.send_sync((void*)&data, sizeof(data));
	if(ret != E_OK){
		logError("ISocketRoutingReceiver::", __func__, " error, ret=", ret);
	}
	else{
		/* 同期応答 結果判定 */
		if( data.error != E_OK ){
			ret = data.error;
		}
		else{
			domainID = data.domainID;
		}
	}
	logDebug("ISocketRoutingReceiver::", __func__," called end, ret=", ret);

    return ret;
}

am_Error_e ISocketRoutingReceiver::deregisterDomain(const am_domainID_t domainID)
{
	(void)domainID;

	return E_OK;
}

am_Error_e ISocketRoutingReceiver::registerGateway(const am_Gateway_s &gatewayData, am_gatewayID_t &gatewayID)
{
	(void)gatewayData;
	(void)gatewayID;

    return E_OK;
}

am_Error_e ISocketRoutingReceiver::deregisterGateway(const am_gatewayID_t gatewayID)
{
	(void)gatewayID;

	return E_OK;
}

am_Error_e ISocketRoutingReceiver::registerConverter(const am_Converter_s& converterData, am_converterID_t& converterID)
{
	(void)converterData;
	(void)converterID;

	return E_OK;
}

am_Error_e ISocketRoutingReceiver::deregisterConverter(am_converterID_t converterID)
{
	(void)converterID;

	return E_OK;
}
am_Error_e ISocketRoutingReceiver::peekSink(const string &name, am_sinkID_t &sinkID)
{
	(void)name;
	(void)sinkID;

	return E_OK;
}

am_Error_e ISocketRoutingReceiver::registerSink(const am_Sink_s &sinkData, am_sinkID_t &sinkID)
{
	(void)sinkData;
	(void)sinkID;

	return E_OK;
}

am_Error_e ISocketRoutingReceiver::deregisterSink(const am_sinkID_t sinkID)
{
	(void)sinkID;

	return E_OK;
}

am_Error_e ISocketRoutingReceiver::peekSource(const string &name, am_sourceID_t &sourceID)
{
	(void)name;
	(void)sourceID;

	return E_OK;
}

am_Error_e ISocketRoutingReceiver::registerSource(const am_Source_s &sourceData, am_sourceID_t &sourceID)
{
	(void)sourceData;
	(void)sourceID;

	return E_OK;
}

am_Error_e ISocketRoutingReceiver::deregisterSource(const am_sourceID_t sourceID)
{
	(void)sourceID;

	return E_OK;
}

am_Error_e ISocketRoutingReceiver::registerCrossfader(const am_Crossfader_s &crossfaderData, am_crossfaderID_t &crossfaderID)
{
	(void)crossfaderData;
	(void)crossfaderID;

	return E_OK;
}

am_Error_e ISocketRoutingReceiver::deregisterCrossfader(const am_crossfaderID_t crossfaderID)
{
	(void)crossfaderID;

	return E_OK;
}

am_Error_e ISocketRoutingReceiver::peekSourceClassID(const string &name, am_sourceClass_t &sourceClassID)
{
	(void)name;
	(void)sourceClassID;

	return E_OK;
}

am_Error_e ISocketRoutingReceiver::peekSinkClassID(const string &name, am_sinkClass_t &sinkClassID)
{
	(void)name;
	(void)sinkClassID;

	return E_NON_EXISTENT;
}

void ISocketRoutingReceiver::hookInterruptStatusChange(const am_sourceID_t sourceID, const am_InterruptState_e interruptState)
{
	(void)sourceID;
	(void)interruptState;

	return;
}

void ISocketRoutingReceiver::hookDomainRegistrationComplete(const am_domainID_t domainID)
{
	am_Error_e ret = E_UNKNOWN;
	ST_HOOKDOMAINREGISTRATIONCOMPLETE data;		//送信データ

	logDebug("ISocketRoutingReceiver::", __func__," called start, domainID=", domainID);

	/* 初期化 */
	memset(&data, 0, sizeof(ST_HOOKDOMAINREGISTRATIONCOMPLETE));

	/* 送信データ設定 */
	data.domainID = domainID;

	/* Socketデータ作成 */
	mCSocketSender.make_senddata(D_OPC_HOOKDOMAINREGISTRATIONCOMPLETE, (void*)&data, sizeof(data));

	/* ClientからPluginへ非同期送信 */
	ret = mCSocketSender.send_async();
	if(ret != E_OK){
		logError("ISocketRoutingReceiver::", __func__, " error, domainID=", domainID);
	}
	logDebug("ISocketRoutingReceiver::", __func__," called end, ret=", ret);

	return;
}

void ISocketRoutingReceiver::hookSinkAvailablityStatusChange(const am_sinkID_t sinkID, const am_Availability_s &availability)
{
	am_Error_e ret = E_UNKNOWN;
	ST_HOOKSINKAVAILABLITYSTATUSCHANGE data;		//送信データ

	logDebug("ISocketRoutingReceiver::", __func__," called start, sinkID=", sinkID, " availability=", availability.availability, " availabilityReason=", availability.availabilityReason);

	/* 初期化 */
	memset(&data, 0, sizeof(ST_HOOKSINKAVAILABLITYSTATUSCHANGE));

	/* 送信データ設定 */
	data.sinkID = sinkID;
	data.availability.availability = availability.availability;
	data.availability.availabilityReason = availability.availabilityReason;

	/* Socketデータ作成 */
	mCSocketSender.make_senddata(D_OPC_HOOKSINKAVAILABLITYSTATUSCHANGE, (void*)&data, sizeof(data));

	/* ClientからPluginへ非同期送信 */
	ret = mCSocketSender.send_async();
	if(ret != E_OK){
		logError("ISocketRoutingReceiver::", __func__, " error, sinkID=", sinkID);
	}
	logDebug("ISocketRoutingReceiver::", __func__," called end, ret=", ret);

	return;
}

void ISocketRoutingReceiver::hookSourceAvailablityStatusChange(const am_sourceID_t sourceID, const am_Availability_s &availability)
{
	am_Error_e ret = E_UNKNOWN;
	ST_HOOKSOURCEAVAILABLITYSTATUSCHANGE data;		//送信データ

	logDebug("ISocketRoutingReceiver::", __func__," called start, sourceID=", sourceID, " availability=", availability.availability, " availabilityReason=", availability.availabilityReason);

	/* 初期化 */
	memset(&data, 0, sizeof(ST_HOOKSOURCEAVAILABLITYSTATUSCHANGE));

	/* 送信データ設定 */
	data.sourceID = sourceID;
	data.availability.availability = availability.availability;
	data.availability.availabilityReason = availability.availabilityReason;

	/* Socketデータ作成 */
	mCSocketSender.make_senddata(D_OPC_HOOKSOURCEAVAILABLITYSTATUSCHANGE, (void*)&data, sizeof(data));

	/* ClientからPluginへ非同期送信 */
	ret = mCSocketSender.send_async();
	if(ret != E_OK){
		logError("ISocketRoutingReceiver::", __func__, " error, sourceID=", sourceID);
	}
	logDebug("ISocketRoutingReceiver::", __func__," called end, ret=", ret);
	return;
}

void ISocketRoutingReceiver::hookDomainStateChange(const am_domainID_t domainID, const am_DomainState_e domainState)
{
	(void)domainID;
	(void)domainState;

	return;
}

void ISocketRoutingReceiver::hookTimingInformationChanged(const am_connectionID_t connectionID, const am_timeSync_t delay)
{
	(void)connectionID;
	(void)delay;

	return;
}

void ISocketRoutingReceiver::sendChangedData(const vector<am_EarlyData_s> &earlyData)
{
	(void)earlyData;

	return;
}

am_Error_e ISocketRoutingReceiver::getDBusConnectionWrapper(CAmDbusWrapper *&dbusConnectionWrapper) const
{
	(void)dbusConnectionWrapper;

	return E_OK;
}

am_Error_e ISocketRoutingReceiver::getSocketHandler(CAmSocketHandler *&socketHandler) const
{
	(void)socketHandler;

	return E_OK;
}

void ISocketRoutingReceiver::getInterfaceVersion(string &version) const
{
	(void)version;

	return;
}

/****************************************************************/
/* confirmRoutingReady送信										*/
/* name:		confirmRoutingReady								*/
/* overview:	confirmRoutingReadyメッセージを送信する			*/
/* param:		handle:Domainのハンドル							*/
/*				error:処理結果									*/
/* ret:			なし											*/
/****************************************************************/
void ISocketRoutingReceiver::confirmRoutingReady(const uint16_t handle, const am_Error_e error)
{
	am_Error_e ret = E_UNKNOWN;
	ST_CONFIRMROUTINGREADY data;

	logDebug("ISocketRoutingReceiver::", __func__," called start, handle=", handle, ", error=", error);

	/* 送信データバッファの初期化 */
	memset(&data, 0, sizeof(ST_CONFIRMROUTINGREADY));

	/* 処理結果を格納 */
	data.handle = handle;
	data.error = error;

	/* 送信データ作成 */
	mCSocketSender.make_senddata(D_OPC_CONFIRMROUTINGREADY, (void*)&data, sizeof(data));

	/* ClientからPluginへ非同期送信 */
	ret = mCSocketSender.send_async();
	if(ret != E_OK){
		logError("ISocketRoutingReceiver::", __func__, " send_async error, handle=", handle);
	}
	logDebug("ISocketRoutingReceiver::", __func__, " called end, ret=", ret);

	return;
}

void ISocketRoutingReceiver::confirmRoutingRundown(const uint16_t handle, const am_Error_e error)
{
	am_Error_e ret = E_UNKNOWN;
	ST_CONFIRMROUTINGRUNDOWN data;

	logDebug("ISocketRoutingReceiver::", __func__," called start, handle=", handle, ", error=", error);

	/* 送信データバッファの初期化 */
	memset(&data, 0, sizeof(ST_CONFIRMROUTINGRUNDOWN));

	/* 処理結果を格納 */
	data.handle = handle;
	data.error = error;

	/* 送信データ作成 */
	mCSocketSender.make_senddata(D_OPC_CONFIRMROUTINGRUNDOWN, (void*)&data, sizeof(data));

	/* ClientからPluginへ非同期送信 */
	ret = mCSocketSender.send_async();
	if(ret != E_OK){
		logError("ISocketRoutingReceiver::", __func__, " send_async error, handle=", handle);
	}
	logDebug("ISocketRoutingReceiver::", __func__, " called end, ret=", ret);

	return;
}

/****************************************************************/
/* getRoutingReady送信											*/
/* name:		getRoutingReady									*/
/* overview:	getRoutingReadyメッセージを送信する				*/
/* param:		なし											*/
/* ret:			true:setRoutingReady実施済み					*/
/*				false:setRoutingRead未実施						*/
/****************************************************************/
bool ISocketRoutingReceiver::getRoutingReady(void)
{
	bool ret = false;					/* 戻り値 */
	am_Error_e retfunc = E_OK;			/* 外部関数戻り値 */
	bool routingReady = false;			/* setRoutingReady実行状態 */

	logDebug("ISocketRoutingReceiver::", __func__," called, start");

	/* 送信データ作成 */
	mCSocketSender.make_senddata(D_OPC_GETROUTINGREADY);

	/* ClientからPluginへ同期送信 */
	retfunc = mCSocketSender.send_sync((void*)&routingReady, sizeof(routingReady));
	if(retfunc == E_OK){
		ret = routingReady; /* 同期送信応答をsetRoutingReady実行状態に設定 */
	}
	logDebug("ISocketRoutingReceiver::", __func__," called end, ret=", ret);

    return ret;
}

am_Error_e ISocketRoutingReceiver::updateGateway(const am_gatewayID_t gatewayid,
                                                   const vector<am_CustomConnectionFormat_t>& listsourceformats,
                                                   const vector<am_CustomConnectionFormat_t>& listsinkformats,
                                                   const vector<bool>& convertionmatrix)
{
	(void)gatewayid;
	(void)listsourceformats;
	(void)listsinkformats;
	(void)convertionmatrix;

	return E_OK;
}

am_Error_e ISocketRoutingReceiver::updateSink(const am_sinkID_t sinkid, const am_sinkClass_t sinkclassid,
                                                const vector<am_SoundProperty_s>& listsoundproperties,
                                                const vector<am_CustomConnectionFormat_t>& listconnectionformats,
                                                const vector<am_MainSoundProperty_s>& listmainsoundproperties)
{
	am_Error_e ret = E_UNKNOWN;
	uint16_t loopCnt = 0;
	ST_UPDATESINK data;

	logDebug("ISocketRoutingReceiver::", __func__, " called start, sinkid=", sinkid, ", sinkclassid=", sinkclassid );

	/* 初期化 */
	memset(&data, 0, sizeof(ST_UPDATESINK));

	/* 送信データ設定 */
	data.sinkID = sinkid;
	data.sinkclassID = sinkclassid;

	/* 送信データのリスト設定 */
	if( listsoundproperties.size() > 0 )
	{
		logDebug("ISocketRoutingReceiver::", __func__, " listsoundproperties.size() =", listsoundproperties.size() );
		std::vector<am_SoundProperty_s>::const_iterator sp_iter = listsoundproperties.begin();
		for(loopCnt = 0; sp_iter < listsoundproperties.end(); ++sp_iter, ++loopCnt){
			data.listsoundproperties[loopCnt].type = sp_iter->type;
			data.listsoundproperties[loopCnt].value = sp_iter->value;
			logDebug("ISocketRoutingReceiver::", __func__, " am_SoundProperty_s type=", data.listsoundproperties[loopCnt].type, ", value=", data.listsoundproperties[loopCnt].value );
		}
	}
	if( listconnectionformats.size() > 0 )
	{
		std::vector<am_CustomConnectionFormat_t>::const_iterator ccf_iter = listconnectionformats.begin();
		for(loopCnt = 0; ccf_iter < listconnectionformats.end(); ++ccf_iter, ++loopCnt){
			data.listconnectionformats[loopCnt] = *ccf_iter;
			logDebug("ISocketRoutingReceiver::", __func__, " am_CustomConnectionFormat_t");
		}
	}
	if( listmainsoundproperties.size() > 0 )
	{
		std::vector<am_MainSoundProperty_s>::const_iterator msp_iter = listmainsoundproperties.begin();
		for(loopCnt = 0; msp_iter < listmainsoundproperties.end(); ++msp_iter, ++loopCnt){
			data.listmainsoundproperties[loopCnt].type = msp_iter->type;
			data.listmainsoundproperties[loopCnt].value = msp_iter->value;
			logDebug("ISocketRoutingReceiver::", __func__, " am_MainSoundProperty_s type=", data.listmainsoundproperties[loopCnt].type, ", value=", data.listmainsoundproperties[loopCnt].value );
		}
	}

	/* Socketデータ作成 */
	mCSocketSender.make_senddata(D_OPC_UPDATESINK, (void*)&data, sizeof(data));

	/* ClientからPluginへ同期送信 */
	ret = mCSocketSender.send_sync((void*)&data, sizeof(data));
	if(ret != E_OK)
	{
		logError("ISocketRoutingReceiver::", __func__, " send_sync error, ret=", ret);
	}

	/* 同期応答 結果判定 */
	if( data.error != E_OK )
	{
		ret = data.error;
	}
	logDebug("ISocketRoutingReceiver::", __func__, " called end, ret=", ret);

    return ret;
}

am_Error_e ISocketRoutingReceiver::updateSource(const am_sourceID_t sourceid, const am_sourceClass_t sourceclassid,
                                                  const vector<am_SoundProperty_s>& listsoundproperties,
                                                  const vector<am_CustomConnectionFormat_t>& listconnectionformats,
                                                  const vector<am_MainSoundProperty_s>& listmainsoundproperties)
{
	(void)sourceid;
	(void)sourceclassid;
	(void)listsoundproperties;
	(void)listconnectionformats;
	(void)listmainsoundproperties;

	return E_OK;
}

am_Error_e ISocketRoutingReceiver::updateConverter(const am_converterID_t converterID,
		const std::vector<am_CustomConnectionFormat_t>& listSourceFormats,
		const std::vector<am_CustomConnectionFormat_t>& listSinkFormats,
		const std::vector<bool>& convertionMatrix)
{
	(void)converterID;
	(void)listSourceFormats;
	(void)listSinkFormats;
	(void)convertionMatrix;

	return E_OK;
}
void ISocketRoutingReceiver::ackSetVolumes(const am_Handle_s handle, const vector<am_Volumes_s>& listvolumes, const am_Error_e error)
{
	am_Error_e ret = E_UNKNOWN;
	uint16_t loopCnt = 0;
	ST_ACKSETVOLUMES data;

	logDebug("ISocketRoutingReceiver::", __func__, " called start, handle=", handle, ", error=", error);

	/* 初期化 */
	memset(&data, 0, sizeof(ST_ACKSETVOLUMES));

	/* 送信データ設定 */
	data.handle = handle;
	data.error = error;

	/* 送信データのリスト設定 */
	if( listvolumes.size() > 0 )
	{
		std::vector<am_Volumes_s>::const_iterator lv_iter = listvolumes.begin();
		for(loopCnt = 0; lv_iter < listvolumes.end(); ++lv_iter, ++loopCnt){
			data.listvolumes[loopCnt].volumeType = lv_iter->volumeType;
			data.listvolumes[loopCnt].volumeID = lv_iter->volumeID;
			data.listvolumes[loopCnt].volume = lv_iter->volume;
			data.listvolumes[loopCnt].ramp = lv_iter->ramp;
			data.listvolumes[loopCnt].time = lv_iter->time;
		}
	}

	/* Socketデータ作成 */
	mCSocketSender.make_senddata(D_OPC_ACKSETVOLUMES, (void*)&data, sizeof(data));

	/* ClientからPluginへ非同期送信 */
	ret = mCSocketSender.send_async();
	if(ret != E_OK)
	{
		logError("ISocketRoutingReceiver::", __func__, " error, handle=", handle, ", error=", error);
	}
	logDebug("ISocketRoutingReceiver::", __func__, " called end, ret=", ret);

	return;
}

void ISocketRoutingReceiver::ackSinkNotificationConfiguration(const am_Handle_s handle, const am_Error_e error)
{
	am_Error_e ret = E_UNKNOWN;
	ST_ACKSINKNOTIFICATIONCONFIGURATION data;

	logDebug("ISocketRoutingReceiver::", __func__, " called start, handle=", handle, ", error=", error);

	/* 初期化 */
	memset(&data, 0, sizeof(ST_ACKSINKNOTIFICATIONCONFIGURATION));

	/* 送信データ設定 */
	data.handle = handle;
	data.error = error;

	/* Socketデータ作成 */
	mCSocketSender.make_senddata(D_OPC_ACKSINKNOTIFICATIONCONFIGURATION, (void*)&data, sizeof(data));

	/* ClientからPluginへ非同期送信 */
	ret = mCSocketSender.send_async();
	if(ret != E_OK)
	{
		logError("ISocketRoutingReceiver::", __func__, " error, handle=", handle, ", error=", error);
	}
	logDebug("ISocketRoutingReceiver::", __func__, " called end, ret=", ret);

	return;
}

void ISocketRoutingReceiver::ackSourceNotificationConfiguration(const am_Handle_s handle, const am_Error_e error)
{
	am_Error_e ret = E_UNKNOWN;
	ST_ACKSOURCENOTIFICATIONCONFIGURATION data;

	logDebug("ISocketRoutingReceiver::", __func__, " called start, handle=", handle, ", error=", error);

	/* 初期化 */
	memset(&data, 0, sizeof(ST_ACKSOURCENOTIFICATIONCONFIGURATION));

	/* 送信データ設定 */
	data.handle = handle;
	data.error = error;

	/* Socketデータ作成 */
	mCSocketSender.make_senddata(D_OPC_ACKSOURCENOTIFICATIONCONFIGURATION, (void*)&data, sizeof(data));

	/* ClientからPluginへ非同期送信 */
	ret = mCSocketSender.send_async();
	if(ret != E_OK)
	{
		logError("ISocketRoutingReceiver::", __func__, " error, handle=", handle, ", error=", error);
	}
	logDebug("ISocketRoutingReceiver::", __func__, " called end, ret=", ret);

	return;
}

void ISocketRoutingReceiver::hookSinkNotificationDataChange(const am_sinkID_t sinkid, const am_NotificationPayload_s& payload)
{
	(void)sinkid;
	(void)payload;

	return;
}

void ISocketRoutingReceiver::hookSourceNotificationDataChange(const am_sourceID_t sourceid, const am_NotificationPayload_s& payload)
{
	(void)sourceid;
	(void)payload;

	return;
}

/*
 * Functions has to be implemented by routing plugin only
 */
am_Error_e ISocketRoutingReceiver::getDomainOfSink(const am_sinkID_t sinkID, am_domainID_t& domainID) const
{
	(void)sinkID;
	(void)domainID;

	return E_OK;
}

am_Error_e ISocketRoutingReceiver::getDomainOfSource(const am_sourceID_t sourceID, am_domainID_t& domainID) const
{
	(void)sourceID;
	(void)domainID;

	return E_OK;
}

am_Error_e ISocketRoutingReceiver::getDomainOfCrossfader(const am_crossfaderID_t crossfaderID, am_domainID_t& domainID) const
{
	(void)crossfaderID;
	(void)domainID;

	return E_OK;
}
