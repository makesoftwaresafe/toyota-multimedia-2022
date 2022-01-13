/************************************************************************
 * @file: CSocketSender.cpp
 *
 * @version: 1.1
 *
 * @description: A CSocketSender class implementation of Routing Adapter.
 * CSocketSender is used to send the data over DBus connection.
 * This class also used to append the data to DBus message.
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
#include <errno.h>
#include "CSocketSender.h"
#include "CAmDltWrapper.h"
#include "CAmSocketWrapper.h"

using namespace std;

namespace am
{

CSocketSender::CSocketSender( void ) : mDatalen(0), mpCAmSocketWrapper(NULL)
{
	logDebug("CSocketSender::CSocketSender CAmSocketWrapper(", mpCAmSocketWrapper, "), CSocketSender(", this, ")");
	/* 送信データバッファをクリア */
	memset(&mMsg, 0, sizeof(mMsg));
	mDatalen = sizeof(ST_SOCKET_MSG);
}

CSocketSender::~CSocketSender()
{
	logDebug("CSocketSender::~CSocketSender called");
}

/********************************************************************************/
/* Socketデータ作成																*/
/* name:		makesenddata													*/
/* overview:	渡された送信元論理ID、OPC、データでメッセージを作成する			*/
/* param(in):	srcaddr		送信元論理ID										*/
/* 				opc		送信データを識別するオペコード(OPC)						*/
/*				data	ソケットデータのデータ部に設定するデータ				*/
/*				size	dataのサイズ											*/
/* ret:			なし															*/
/********************************************************************************/
void CSocketSender::make_senddata(unsigned short opc, void* data, size_t size)
{
	unsigned short	datasize = 0;
	unsigned short	srcaddr = D_AM_DOMAIN_ADDRESS_INVALID;
	logDebug("CSocketSender::make_senddata called, opc = ", opc, ", size = ", size);

	/* 送信データバッファをクリア */
	memset(&mMsg, 0, sizeof(mMsg));

	/* 変数チェック */
	if(mpCAmSocketWrapper == NULL){
		logError("CSocketSender:make_senddata CAmSocketWrapper is not exist");
		return;
	}

	/* 転送元論理アドレス取得 */
	srcaddr = mpCAmSocketWrapper->getSrcAddress();

	datasize = sizeof(mMsg.srcaddr) + sizeof(mMsg.destaddr) + sizeof(mMsg.opc) + sizeof(mMsg.data);
	logDebug("CSocketSender::make_senddata Msg datasize = ", datasize);

	/* 送信データバッファに送信データを保存 */
	mMsg.escm_heada	= AM_SWAP_ENDIAN32(D_AM_DOMAIN_ESCM);
	mMsg.datasize	= (unsigned short)AM_SWAP_ENDIAN16(datasize);
	mMsg.srcaddr	= (unsigned short)AM_SWAP_ENDIAN16(srcaddr);
	mMsg.opc		= (unsigned short)AM_SWAP_ENDIAN16(opc);
	if((data != NULL)&&(size>0)){
		memcpy(&mMsg.data, data, size);
	}

	return;
}

/********************************************************************************/
/* 同期送信																		*/
/* name:		send_sync														*/
/* overview:	作成済みのメッセージを送信し、応答受信待ちをする				*/
/*				応答受信後、応答データを出力用引数に渡す						*/
/* param(out):	replydata	応答データ格納用バッファ							*/
/* 				replysize	応答データ格納用バッファのサイズ					*/
/* ret:			E_OK			正常											*/
/*				E_NOT_POSSIBLE	パラメータ異常で処理続行不能					*/
/* 				E_ABORTED		シグナル割り込みによる中断						*/
/*								send/recvでエラー発生のため中断					*/
/********************************************************************************/
am_Error_e CSocketSender::send_sync(void* replydata, size_t replysize, unsigned short destaddr)
{
	am_Error_e ret;					/* 戻り値 */
	int loopCnt = 0;				/* 戻り値 */
	int retfunc = -1;				/* 外部関数戻り値 */
	bool retfuncbool = false;		/* 外部関数戻り値 */
	int ConnectFd = -1;				/* send/recvに使用するFD */
	int readlen = 0;				/* 読み込みサイズ */
	int socket_errno;				/* エラー番号 */
	ST_SOCKET_MSG rcvmsg;			/* 受信メッセージ格納用 */

	ret = E_NOT_POSSIBLE;

	logDebug("CSocketSender::send_sync call");

	/* 引数チェック */
	if(replydata == NULL){
		logError("CSocketSender:send_sync argument error");
		return ret;
	}

	/* 変数チェック */
	if(mpCAmSocketWrapper == NULL){
		logError("CSocketSender:send_sync CAmSocketWrapper is not exist");
		return ret;
	}

	logInfo("CSocketSender:send_sync CAmSocketWrapper(", mpCAmSocketWrapper, "), CSocketSender(", this, ")");

	/* Connect要求 */
	retfuncbool = mpCAmSocketWrapper->requestConnect(destaddr);
	if(retfuncbool == false){
		logError("CSocketSender:send_sync requestConnect Error");
		return ret;
	}

	/*ConnectFD取得 */
	ConnectFd = mpCAmSocketWrapper->getFileDescripter(EN_FD_TYPE_SOCKET_CONNECT, destaddr);
	if(ConnectFd <= D_AM_SOCKET_FD_INVALID){
		logError("CSocketSender:send_sync ConnectFD is INVALID");
		return ret;
	}

	/* save Forwarding address to sending buffer */
	mMsg.destaddr	= (unsigned short)AM_SWAP_ENDIAN16(destaddr);

	/*メッセージ送信 */
	logInfo("CSocketSender::send_sync send, destaddr=", AM_SWAP_ENDIAN16(mMsg.destaddr), ", fd=", ConnectFd, ", len=", mDatalen, ", replysize=", replysize, ", opc=", AM_SWAP_ENDIAN16(mMsg.opc));
	retfunc = send(ConnectFd, (void*)&mMsg, mDatalen, 0);
	socket_errno = errno;
	if((retfunc < 0) || (retfunc != mDatalen)){
		logError("CSocketSender::send_sync send, errno=", socket_errno, " (", std::string(strerror(socket_errno)), "), len=", retfunc );

		/* ConnectFDClose */
		mpCAmSocketWrapper->socketCloseforSingleFd(EN_FD_TYPE_SOCKET_CONNECT, ConnectFd);
		ret = E_ABORTED;
		return ret;
	}

	/*メッセージ受信 */
	logDebug("CSocketSender::send_sync recv sta, fd=", ConnectFd);
	for( loopCnt=1; loopCnt <= D_AM_RECEIVE_CNT; loopCnt++ ){
		/* 最大5回の受信リトライ */
		retfunc = recv(ConnectFd, &rcvmsg, mDatalen, 0);
		socket_errno = errno;
		logDebug("CSocketSender::send_sync recv retfunc=", retfunc, "mDatalen=", mDatalen, "socket_errno=", socket_errno, " (", std::string(strerror(socket_errno)), ")");
		if(retfunc > 0){
			/* 受信完了 */
			ret = E_OK;
			break;
		}
		else if(retfunc == 0){
			logError("CSocketSender::send_sync recv, disconnect recv_size=", retfunc, "socket_errno=", socket_errno, " (", std::string(strerror(socket_errno)), ")");
			/* ConnectFDClose */
			mpCAmSocketWrapper->socketCloseforSingleFd(EN_FD_TYPE_SOCKET_CONNECT, ConnectFd);
			ret = E_ABORTED;
			break;
		}
		else{
			if(socket_errno == EINTR){
				logInfo("CSocketSender::send_sync recv, EINTR");
				if(loopCnt == D_AM_RECEIVE_CNT){
					logError("CSocketSender::send_sync recv, LoopCount MAX");
					ret = E_ABORTED;
					break;
				}
			}
			else{
				logError("CSocketSender::send_sync recv, systemerr errno=", socket_errno, " (", std::string(strerror(socket_errno)), ")");
				ret = E_ABORTED;
				break;
			}
		}
	}
	logInfo("CSocketSender::send_sync recv end, fd=", ConnectFd, "mMsg.opc=", AM_SWAP_ENDIAN16(mMsg.opc), "rcvmsg.opc=", AM_SWAP_ENDIAN16(rcvmsg.opc));

	if( ret != E_OK ){
		/* ConnectFDClose */
		mpCAmSocketWrapper->socketCloseforSingleFd(EN_FD_TYPE_SOCKET_CONNECT, ConnectFd);
	}
	
	/* 読み込みサイズ確認 */
	readlen = D_AM_MSG_HEADSIZE + replysize;
	if(retfunc >= readlen){
		if(mMsg.opc == rcvmsg.opc){
			/* 受信データを出力引数に格納 */
			memcpy(replydata, &rcvmsg.data, replysize);
			ret = E_OK;
		}
		else{
			logError("CSocketSender::send_sync recv, opc error, mMsg.opc=", AM_SWAP_ENDIAN16(mMsg.opc), "rcvmsg.opc=", AM_SWAP_ENDIAN16(rcvmsg.opc));
			ret = E_ABORTED;
		}
	}
	else{
		logError("CSocketSender::send_sync recv, len error, len=", retfunc);
		ret = E_ABORTED;
	}

	return ret;
}

/****************************************************************************************/
/* Async transmission / response in one													*/
/* name:		send_async																*/
/* overview:	send a created message													*/
/*				send the response message in Async transmission							*/
/* param(out):	destaddr		Forwarding address										*/
/* ret:			E_OK			Normal													*/
/*				E_NOT_POSSIBLE	Impossible to continue processing for parameter fault	*/
/*				E_ABORTED		Interruption due to error occurred in send				*/
/****************************************************************************************/
am_Error_e CSocketSender::send_async(unsigned short destaddr)
{
	am_Error_e ret = E_NOT_POSSIBLE;		/* 戻り値 */
	int retfunc = -1;						/* 外部関数戻り値 */
	int ConnectFd = D_AM_SOCKET_FD_INVALID;	/* sendに使用するFD */
	bool retfuncbool = false;				/* 外部関数戻り値 */
	int socket_errno;						/* error no */

	logDebug("CSocketSender::send_async call");

	if(mpCAmSocketWrapper == NULL){
		logError("CSocketSender:send_async CAmSocketWrapper is not exist");
		return ret;
	}

	/* Connect要求 */
	retfuncbool = mpCAmSocketWrapper->requestConnect(destaddr);
	if(retfuncbool == false){
		logError("CSocketSender:send_async requestConnect Error");
		return ret;
	}

	/*ConnectFD取得 */
	ConnectFd = mpCAmSocketWrapper->getFileDescripter(EN_FD_TYPE_SOCKET_CONNECT, destaddr);
	if(ConnectFd <= D_AM_SOCKET_FD_INVALID){
		logError("CSocketSender:send_async ConnectFD is INVALID");
		return ret;
	}

	/* save Forwarding address to sending buffer */
	mMsg.destaddr	= (unsigned short)AM_SWAP_ENDIAN16(destaddr);

	/*メッセージ送信 */
	logInfo("CSocketSender::send_async send, destaddr=", AM_SWAP_ENDIAN16(mMsg.destaddr), ", fd=", ConnectFd, ", len=", mDatalen, ", opc=", AM_SWAP_ENDIAN16(mMsg.opc));
	retfunc = send(ConnectFd, (void*)&mMsg, mDatalen, 0);
	socket_errno = errno;
	if((retfunc < 0) || (retfunc != mDatalen))
	{
		logError("CSocketSender::send_async send, errno=", socket_errno, " (", std::string(strerror(socket_errno)), "), len=", retfunc );

		/* ConnectFDClose */
		mpCAmSocketWrapper->socketCloseforSingleFd(EN_FD_TYPE_SOCKET_CONNECT, ConnectFd);
		ret = E_ABORTED;
	}
	else{
		ret = E_OK;
	}

	return ret;
}

/****************************************************************************************/
/* response in Sync transmission														*/
/* name:		send_sync_reply															*/
/* overview:	send the response message in Sync transmission							*/
/* param(out):	destaddr		Forwarding address										*/
/* ret:			E_OK			Normal													*/
/*				E_NOT_POSSIBLE	Impossible to continue processing for parameter fault	*/
/*				E_ABORTED		Interruption due to error occurred in send				*/
/****************************************************************************************/
am_Error_e CSocketSender::send_sync_reply(unsigned short destaddr)
{
	am_Error_e ret = E_NOT_POSSIBLE;	/* 戻り値 */
	int retfunc;						/* 外部関数戻り値 */
	int AcceptFd;						/* sendに使用するFD */
	bool retfuncbool = false;			/* 外部関数戻り値 */
	int socket_errno;					/* error no */

	logDebug("CSocketSender::send_sync_reply call");

	if(mpCAmSocketWrapper == NULL){
		logError("CSocketSender:send_sync_reply CAmSocketWrapper is not exist");
		return ret;
	}

	/*AcceptFD取得 */
	AcceptFd = mpCAmSocketWrapper->getCurrentAcceptFD();
	if(AcceptFd <= D_AM_SOCKET_FD_INVALID){
		logError("CSocketSender:send_sync_reply AcceptFD is INVALID");
		return ret;
	}

	/* save Forwarding address to sending buffer */
	mMsg.destaddr	= (unsigned short)AM_SWAP_ENDIAN16(destaddr);

	/*メッセージ送信 */
	logInfo("CSocketSender::send_sync_reply send, destaddr=", AM_SWAP_ENDIAN16(mMsg.destaddr), ", fd=", AcceptFd, ", len=", mDatalen, ", opc=", AM_SWAP_ENDIAN16(mMsg.opc));
	retfunc = send(AcceptFd, (void*)&mMsg, mDatalen, 0);
	socket_errno = errno;
	if((retfunc < 0) || (retfunc != mDatalen))
	{
		logError("CSocketSender::send_sync_reply send, errno=", socket_errno, " (", std::string(strerror(socket_errno)), "), len=", retfunc );

		/* Reconnect Start */
		retfuncbool = mpCAmSocketWrapper->socketReConnect(AcceptFd);
		if( retfuncbool == false ){
			logError("CSocketSender::send_async socketReConnect, retfuncbool=", retfuncbool);
		}
		ret = E_ABORTED;
	}
	else{
		ret = E_OK;
	}

	return ret;
}
/********************************************************************************/
/* ラッパクラスポインタ設定														*/
/* name:		setSocketWrapper												*/
/* overview:	渡されたCAmSocketWrapperクラスインスタンスポインタを保持する	*/
/* param(in):	wrapper	CAmSocketWrapperクラスインスタンスポインタ				*/
/* ret:			なし															*/
/********************************************************************************/
void CSocketSender::setSocketWrapper( CAmSocketWrapper*& wrapper )
{
	logDebug("CSocketSender::setSocketWrapper call, wrapper=", wrapper);
	mpCAmSocketWrapper = wrapper;
}
}/* namespace am*/
