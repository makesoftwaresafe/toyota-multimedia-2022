/**
 *  SPDX license identifier: MPL-2.0
 *
 * Copyright (C) 2012, BMW AG
 *
 * This file is part of GENIVI Project AudioManager.
 *
 * Contributions are licensed to the GENIVI Alliance under one or more
 * Contribution License Agreements.
 *
 * \copyright
 * This Source Code Form is subject to the terms of the
 * Mozilla Public License, v. 2.0. If a  copy of the MPL was not distributed with
 * this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *
 * \author Christian Linke, christian.linke@bmw.de BMW 2011,2012
 *
 * \file CAmSocketWrapper.cpp
 *
 */

#include "CAmSocketWrapper.h"
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include "CAmDltWrapper.h"
#include "CAmSocketHandler.h"
#include "CSocketCommon.h"

using namespace am;

/* Setting value for AudioManager Socket */
#define D_AM_SOCKET_ADDRESS_AM "172.16.21.3"
#define D_AM_SOCKET_ADDRESS_RAA "172.16.21.2"
#define D_AM_SOCKET_PORT_AM		(64506)		/* AM Port */
#define D_AM_SOCKET_PORT_RAA	(64505)		/* RAA Port */
#define D_AM_SOCKET_LISTEN_QUE_SIZE	(1)		/* ListenQueSize */

/* ソケット初期化用テーブル(ドメイン設定テーブル）*/
typedef struct _stSocketTbl
{
	SOCKET_TYPE type; 				/* Initialize type */
	unsigned short srcaddr;			/* Forwarding address */
	const char *listenaddr; 		/* string showing IP address specified at listen */
	unsigned short listenport;		/* port specified at listen */
} stSocketTbl;

static const stSocketTbl m_SocketInitTbl[] = {
	{EN_SOCKET_TYPE_RAA, D_AM_DOMAIN_ADDRESS_RAA, D_AM_SOCKET_ADDRESS_RAA, D_AM_SOCKET_PORT_RAA}
};

//コンストラクタ
CAmSocketWrapper::CAmSocketWrapper( SOCKET_USED_TYPE usedtype, SOCKET_TYPE type ) :
				pSocketAcceptCallback(this, &CAmSocketWrapper::socketAcceptCallback),
				pSocketDispatchCallback(this, &CAmSocketWrapper::socketDispatchCallback),
				pSocketFireCallback(this, &CAmSocketWrapper::socketFireCallback),
				pSocketCheckCallback(this, &CAmSocketWrapper::socketCheckCallback),
				mRecvDatalen(0),
				mpSocketHandler(NULL),
				mListenPollHandle(0),
				mListenAddress(NULL),
				mListenPort(0),
				mListenFD(D_AM_SOCKET_FD_INVALID),
				mSrcAddress(D_AM_DOMAIN_ADDRESS_INVALID),
				mSocketUsedType(EN_SOCKET_USED_TYPE_INVALID),
				mReConnectFlag(false),
				mpRecievCallback(NULL),
				mCurrentAcceptFD(D_AM_SOCKET_FD_INVALID)

{
	logInfo("CAmSocketWrapper::", __func__, " called, usedtype = ", usedtype, ", type=", type);

	memset(&mMsg, 0, sizeof(ST_SOCKET_MSG));

	/* Socket通信確立用アドレス・ポートの設定 */
	setSocketSetting( usedtype, type);

	/* SocketType設定 */
	this->mSocketUsedType = usedtype;

	/* Socket接続初期化処理	*/
	bool ret = CAmSocketWrapper::socketInitialize( &mListenFD );
	if( true != ret )
	{
		logError("CAmSocketWrapper::", __func__, " , socketInitialize error ret=", ret);
		this->mListenFD = D_AM_SOCKET_FD_INVALID;
	}
	else
	{
		/* Domain側でのラッパクラス使用の場合	*/
		if( usedtype == EN_SOCKET_USED_TYPE_CLIENT )
		{
			FileDescripterMap_t::iterator itr = mFileDescripterMap.begin();
			if( itr == mFileDescripterMap.end() ){
				logError("CAmSocketWrapper::", __func__, ", map for ConnectFD is not defined.");
				return;
			}

			int tmpConnectFD = D_AM_SOCKET_FD_INVALID;
			/* Connect用SocketFD生成	*/
			ret	= CAmSocketWrapper::socketCreate( &tmpConnectFD );
			if( true != ret )
			{
				logError("CAmSocketWrapper::", __func__, " , socketCreate error ret=", ret);
				int result = close(mListenFD);
				int socket_errno = errno;
				if( result < 0 ){
					logError("CAmSocketWrapper::", __func__, " , listning Close Error! result:", result, ", errno:", socket_errno, "(", std::string(strerror(socket_errno)), ")");
				}
				mListenFD = D_AM_SOCKET_FD_INVALID;
				tmpConnectFD = D_AM_SOCKET_FD_INVALID;
			}
			logInfo("CAmSocketWrapper::", __func__, " connect end, ConnectFD=", tmpConnectFD);

			itr->second.ConnectFD = tmpConnectFD;	/* ConnectFDを保存 */
		}
	}
	logInfo("CAmSocketWrapper::", __func__, " all end, ListenFD=", mListenFD);

	return;
}

//デストラクタ
CAmSocketWrapper::~CAmSocketWrapper()
{
	logInfo("CAmSocketWrapper::", __func__, " calld");
	/* socketFD解放 */
	socketClose();

	return;
}

/****************************************************************************************/
/* Socket受信振り分け関数登録															*/
/* name:		registerCallback														*/
/* overview:	受信データ振り分け用Callbackを登録する									*/
/* param(in):	recieveCb	受信振り分けコールバック関数								*/
/* ret:			なし																	*/
/****************************************************************************************/
void CAmSocketWrapper::registerCallback(IRecieveCallBack *recieveCb)
{
	logInfo("CAmSocketWrapper::", __func__, " called, recieveCb=", recieveCb);
	mpRecievCallback = recieveCb;
	return;
}

/****************************************************************************/
/*!
 * \fn void socketAcceptCallback(const pollfd pollfd, const sh_pollHandle_t handle, void *userData)
 * \brief Accept-Connect処理
 * \param[in]	pollfd		: poll監視しているfd(リスニングソケットのFD)
 * \param[in]	handle		: pollリストの位置を示すhandle
 * \param[in]	userData	: ユーザーデータ(未使用)
 * \retval なし
 * \note
 */
/****************************************************************************/
void am::CAmSocketWrapper::socketAcceptCallback(const pollfd pollfd, const sh_pollHandle_t handle, void *userData)
{
    (void) userData;
	int				result		= 0;						/* return value for close			*/
	am_Error_e 		ret			= E_OK;						/* CAmSocketHandlerのメソッド戻り値 */
    int16_t			event		= 0;
    sh_pollHandle_t addhandle	= 0;
	int				socket_errno = 0;						/* SystemCall errno					*/
	int				tmpAcceptFD = D_AM_SOCKET_FD_INVALID;	/* tempolally storage area for AcceptFD	*/
	FileDescripterMap_t::iterator itr;
	listTempAcceptFD_t::iterator itrtemp;
	bool			bRet = false;

	logInfo("CAmSocketWrapper::", __func__, " called, ListenFD=", pollfd.fd);
	/* 引数、変数のチェック */
	if( pollfd.fd != mListenFD ){
		//何もしない
		logError("CAmSocketWrapper::", __func__, " , ListenFD Error! pollfd.fd:", pollfd.fd, "mListenFD:", mListenFD);
		return;
	}
	if( mpSocketHandler == NULL ){
		logError("CAmSocketWrapper::", __func__, " , mpSocketHandler is nothing!");
		//再接続フラグ初期化
		mReConnectFlag = false;
		return;
	}
	if( pollfd.fd <= D_AM_SOCKET_FD_INVALID ){
		logError("CAmSocketWrapper::", __func__, " , fd is INVALID!!");
		/* listeningFDをPollから削除 */
		mpSocketHandler->removeFDPoll(handle);
		mListenPollHandle = 0;
	}
	if( (pollfd.revents & (POLLHUP | POLLERR)) != 0 ){
		logError("CAmSocketWrapper::", __func__, " , poll revents is Error!");
		//再接続フラグ初期化
		mReConnectFlag = false;
		return;
	}

	/* 接続の確立 */
	tmpAcceptFD = accept( pollfd.fd, NULL, NULL );
	socket_errno = errno;
	if( tmpAcceptFD < 0 ){
		logError("CAmSocketWrapper::", __func__, " , AcceptError! AcceptFD:",tmpAcceptFD, ", errno:", socket_errno, "(", std::string(strerror(socket_errno)), ")");

		/* AcceptSocket Release */
		itr = mFileDescripterMap.begin();
		for( ; itr != mFileDescripterMap.end(); ++itr){
			socketCloseforSingleFd( EN_FD_TYPE_SOCKET_ACCEPT, itr->second.AcceptFD );
		}
		itrtemp = mTempAcceptFD.begin();
		for( ; itrtemp != mTempAcceptFD.end(); ++itrtemp){
			socketCloseforSingleFd( EN_FD_TYPE_SOCKET_ACCEPT, itrtemp->AcceptFD );
		}

		/* リスニングソケットクローズ */
		result = close( pollfd.fd );
		socket_errno = errno;
		if( result < 0 ){
			logError("CAmSocketWrapper::", __func__, " , listning Close Error! result:", result, ", errno:", socket_errno, "(", std::string(strerror(socket_errno)), ")");
		}
		mListenFD = D_AM_SOCKET_FD_INVALID;

		/* listeningFDをPollから削除 */
		ret = mpSocketHandler->removeFDPoll(handle);
		if( ret != E_OK ){
			logError("CAmSocketWrapper::", __func__, " , removeFDPoll! ret:",ret);
		}
		mListenPollHandle = 0;

		/* Socket Connection Initialize	*/
		bRet = socketInitialize( &mListenFD );
		if( bRet != true ){
			logError("CAmSocketWrapper::", __func__," socketInitialize error bRet=", bRet);
			mListenFD = D_AM_SOCKET_FD_INVALID;
		}else{
			/* Connection request waite start */
			bRet = requestAddFDpoll( mpSocketHandler );
			if( bRet != true ){
				logError("CAmSocketWrapper::", __func__," requestAddFDpoll error bRet=", bRet);
				close(mListenFD);
				mListenFD = D_AM_SOCKET_FD_INVALID;
			}
		}

		//再接続フラグ初期化
		mReConnectFlag = false;
		return;
	}

	if( mSocketUsedType == EN_SOCKET_USED_TYPE_CLIENT ){
		/* リスニングソケットクローズ */
		result = close( pollfd.fd );
		socket_errno = errno;
		if( result < 0 ){
			logError("CAmSocketWrapper::", __func__, " , listning Close Error! Error result:",result, ", errno:", socket_errno, "(", std::string(strerror(socket_errno)), ")");

			/* Acceptソケットクローズ */
			result = close( tmpAcceptFD );
			socket_errno = errno;
			if( result < 0 ){
				logError("CAmSocketWrapper::", __func__, " accept Close Error! Error result:",result, ", errno:", socket_errno, "(", std::string(strerror(socket_errno)), ")");
			}
			tmpAcceptFD = D_AM_SOCKET_FD_INVALID;
			mListenFD = D_AM_SOCKET_FD_INVALID;

			/* listeningFDをPollから削除 */
			ret = mpSocketHandler->removeFDPoll(handle);
			if( ret != E_OK ){
				logError("CAmSocketWrapper::", __func__, " , removeFDPoll! ret:",ret);
			}
			mListenPollHandle = 0;
			//再接続フラグ初期化
			mReConnectFlag = false;
			return;
		}
		mListenFD = D_AM_SOCKET_FD_INVALID;

		/* listeningFDをPollから削除 */
		ret = mpSocketHandler->removeFDPoll(handle);
		if( ret != E_OK ){
			logError("CAmSocketWrapper::", __func__, " , removeFDPoll! ret:",ret);
		}
		mListenPollHandle = 0;
	}

	/* acceptFDをPollに追加 */
	event |= POLLIN;
	ret = mpSocketHandler->addFDPoll(tmpAcceptFD, event, NULL, &pSocketFireCallback, &pSocketCheckCallback, &pSocketDispatchCallback, NULL, addhandle);
	if( ret != E_OK ){
		logError("CAmSocketWrapper::", __func__, " , addFDPoll! ret:",ret);
		/* Acceptソケットクローズ */
		result = close( tmpAcceptFD );
		socket_errno = errno;
		if( result < 0 ){
			logError("CAmSocketWrapper::", __func__, " , accept Close result:",result, ", errno:", socket_errno, "(", std::string(strerror(socket_errno)), ")");
		}
		tmpAcceptFD = D_AM_SOCKET_FD_INVALID;
		//再接続フラグ初期化
		mReConnectFlag = false;
		return;
	}

	/* IF socket type is client, AcceptFD is saved FD managiment table. */
	if( mSocketUsedType == EN_SOCKET_USED_TYPE_CLIENT ){
		itr = mFileDescripterMap.begin();
		if( itr != mFileDescripterMap.end() ){
			itr->second.AcceptFD = tmpAcceptFD;
			/* hold pollhandle for acceptFD */
			itr->second.AcceptPollHandle = addhandle;
		}
		else{
			/* delete pollhandle from  poll */
			ret = mpSocketHandler->removeFDPoll(addhandle);
			if( ret != E_OK ){
				logError("CAmSocketWrapper::", __func__, " , removeFDPoll! ret:",ret);
			}

			result = close( tmpAcceptFD );
			socket_errno = errno;
			if( result < 0 ){
				logError("CAmSocketWrapper::", __func__, " , accept Close result:",result, ", errno:", socket_errno, "(", std::string(strerror(socket_errno)), ")");
			}
			tmpAcceptFD = D_AM_SOCKET_FD_INVALID;
		}
	}
	else{
		stAcceptFDTempTbl tmpAccepptFDtbl;
		memset(&tmpAccepptFDtbl, 0, sizeof(stAcceptFDTempTbl));
		tmpAccepptFDtbl.AcceptFD = tmpAcceptFD;
		/* hold pollhandle for acceptFD */
		tmpAccepptFDtbl.AcceptPollHandle = addhandle;
		mTempAcceptFD.push_back(tmpAccepptFDtbl);
	}

	//再接続フラグ初期化
	mReConnectFlag = false;
	logInfo("CAmSocketWrapper::", __func__, " end, AcceptFD=", tmpAcceptFD);

	return;

}

/****************************************************************************************/
/* Socket受信振り分け関数呼び出しCallback												*/
/* name:		socketDispatchCallback													*/
/* overview:	受信データ振り分け用Callbackを呼び出す									*/
/* 				受信データの確認は行わない												*/
/* param(in):	handle		Pollリスト中のpollfdの位置を示すハンドル					*/
/* param(in):	userData	AddFDPol時に指定したUserデータ								*/
/* ret:			true	発火FD削除不要													*/
/* 				false	発火FD削除必要													*/
/****************************************************************************************/
bool am::CAmSocketWrapper::socketDispatchCallback(const sh_pollHandle_t handle, void *userData)
{
    (void) handle;
    (void) userData;

	logDebug("CAmSocketWrapper::", __func__, " called");
	if( mpRecievCallback == NULL ){
		logError("CAmSocketWrapper::", __func__," No CallBackFunction!");
		return (true);
	}

	mpRecievCallback->Call( (void*)&mMsg );

	return (false);
}

/****************************************************************************************/
/* Socket受信チェックCallback															*/
/* name:		socketCheckCallback														*/
/* overview:	受信が成功したか失敗したかをチェックするCallback						*/
/* 				受信データの確認は行わない												*/
/* param(in):	handle		Pollリスト中のpollfdの位置を示すハンドル					*/
/* param(in):	userData	AddFDPol時に指定したUserデータ								*/
/* ret:			true	正常															*/
/* 				false	異常															*/
/****************************************************************************************/
bool am::CAmSocketWrapper::socketCheckCallback(const sh_pollHandle_t handle, void *userData)
{
    (void) handle;
    (void) userData;

	logDebug("CAmSocketWrapper::", __func__, " called");
	if( mRecvDatalen > 0 ){
		return (true);
	}
    return (false);
}

/****************************************************************************************/
/* Socket受信検知Callback																*/
/* name:		socketFireCallback														*/
/* overview:	SocketのAccept状態変化検知してrecvを実施するCallback					*/
/* param(in):	pollfd		AcceptFDを格納してあるPoll構造体							*/
/* param(in):	handle		Pollリスト中のpollfdの位置を示すハンドル					*/
/* param(in):	userData	AddFDPol時に指定したUserデータ								*/
/* ret:			なし															*/
/****************************************************************************************/
void am::CAmSocketWrapper::socketFireCallback(const pollfd pollfd, const sh_pollHandle_t handle, void *userData)
{
    (void) handle;
    (void) userData;
	int socket_errno = 0;	/* SystemCall errno */
	bool ret = false;		/* Reconnect result */

	ssize_t datasize = 0;	/* Recv Size */

	memset((void*)&mMsg, 0, sizeof(mMsg));
	mRecvDatalen = 0;

	//logが出力されすぎるためlogInfoからlogVerboseに変更予定
	//logInfo("CAmSocketWrapper::socketFireCallback called, AcceptFD=", pollfd.fd);
	if( pollfd.revents & ~(POLLIN || POLLOUT) ){
		logError("CAmSocketWrapper::", __func__," poll Error! revents:", pollfd.revents);
		return;
	}

	/* recv */
	datasize = recv( pollfd.fd, (void *)&mMsg, sizeof(mMsg), 0 );
	socket_errno = errno;
	/* 受信エラー（サイズ0の場合も再接続を上位に促すためにエラーを返す） */
	if( datasize == D_AM_MSG_RECV_ERROR ){
		logError("CAmSocketWrapper::", __func__," recv Error datasize:" , datasize, "socket_errno:", socket_errno, "(", std::string(strerror(socket_errno)), ")");
		return;
	}
	else if( datasize == 0 ){
		logError("CAmSocketWrapper::", __func__," No CallBackFunction! datasize:", datasize, "mReConnectFlag", mReConnectFlag);
		ret = socketReConnect(pollfd.fd);
		if( ret == false ){
			logError("CAmSocketWrapper::", __func__," socketReConnect Error ret:", ret);
		}
		return;
	}
	else if( datasize < D_AM_MSG_HEADSIZE ){
		memset(&mMsg, 0, sizeof(mMsg));
		logError("CAmSocketWrapper::", __func__," No CallBackFunction! datasize:" , datasize);
		return;
	}
	mRecvDatalen = datasize;
	mCurrentAcceptFD = pollfd.fd;	/* save receiving FD */

	return;
}

/****************************************************************************************/
/* Connect要求																			*/
/* name:		requestConnect															*/
/* overview:	socket生成(socket)、接続(connect)を実施し、								*/
/* 				生成されたConnectのファイルディスクリプタを保存する						*/
/*				Socket接続済みの場合はtrueを返す										*/
/* param(out):	destaddr:Forwarding address												*/
/* ret:			true	正常															*/
/* 				false	異常															*/
/****************************************************************************************/
bool CAmSocketWrapper::requestConnect( unsigned short destaddr )
{
	int socket_errno;							/* エラー番号	*/
	int socket_close_errno;						/* connedctエラーによるclose()呼び出し時のエラー番号	*/
	bool ret	= false;						/* 内部関数戻り値		*/
	bool ret_in	= false;						/* 内部関数戻り値		*/
	int ret_out	= -1;							/* 外部関数戻り値		*/
	struct sockaddr_in dst_addr;				/* Socketアドレス(相手)	*/
	socklen_t dst_addr_size = sizeof(dst_addr);	/* Socketアドレスサイズ	*/
	int loop_cnt = 0;							/* ループカウンタ */
	int tmpConnectFD = D_AM_SOCKET_FD_INVALID;	/* temporary area for ConnectFD	*/
	const char *tmpConnectAddr = NULL;			/* temporary area for ConnectAddrress */
	unsigned short tmpConnectPort = 0;			/* temporary area for ConnectPort	*/

	logInfo("CAmSocketWrapper::", __func__," called, destaddr:" , destaddr);

	/* check that valiable for connectFD exists */
	FileDescripterMap_t::iterator iter = mFileDescripterMap.find(destaddr);
	if( iter == mFileDescripterMap.end() ){
		logError("CAmSocketWrapper::", __func__, ", map for ConnectFD is not defined.");
		return false;
	}
	tmpConnectFD = iter->second.ConnectFD;

	if( tmpConnectFD == D_AM_SOCKET_FD_INVALID ){
		/* Connect用SocketFD生成	*/
		ret_in	= socketCreate( &tmpConnectFD );
		if( true != ret_in ){
			logError("CAmSocketWrapper::", __func__," ConnectFD Error! FD:" , tmpConnectFD);
			return false;
		}
	}

	tmpConnectAddr = iter->second.ConnectAddress;
	tmpConnectPort = iter->second.ConnectPort;
	if( (tmpConnectAddr != NULL) && (tmpConnectPort > 0) ){
		/* Socketアドレスの初期化 */
		memset(&dst_addr, 0, sizeof(dst_addr));
		dst_addr.sin_port = htons( tmpConnectPort );
		dst_addr.sin_family = AF_INET;
		dst_addr.sin_addr.s_addr = inet_addr( tmpConnectAddr );

		/* 接続 */
		logInfo("CAmSocketWrapper::requestConnect connect, ConnectFD=", tmpConnectFD, " ConnectAddress=", tmpConnectAddr, " ConnectPort=", tmpConnectPort);
		ret_out = connect( tmpConnectFD, (struct sockaddr *)&dst_addr, dst_addr_size );
		socket_errno = errno;
		if( ret_out < 0 ){
			if( socket_errno == EISCONN ){
				/* The socket is already connected. */
				ret = true;
			}
			else if( (mReConnectFlag == true) && (socket_errno == ECONNREFUSED) ){
				logError("CAmSocketWrapper::", __func__," connect loop start:", mReConnectFlag);
				/* 5回connectに失敗した場合、再接続を諦める */
				for(loop_cnt = 0; loop_cnt < 5; loop_cnt++){
					ret_out = connect( tmpConnectFD, (struct sockaddr *)&dst_addr, dst_addr_size );
					socket_errno = errno;
					if( ret_out < 0 ){
						ret = false;
					}
					else{
						logInfo("CAmSocketWrapper::", __func__," ReConnect Success!! ConnectFD:", tmpConnectFD);
						ret = true;
						break;
					}
				}
			}
			else{
				ret_out = close(tmpConnectFD);
				socket_close_errno = errno;
				if( ret_out < 0 ){
					logError("CAmSocketWrapper::", __func__, " , Connect Close result:",ret_out, ", errno:", socket_close_errno, "(", std::string(strerror(socket_close_errno)), ")");
				}
				tmpConnectFD = D_AM_SOCKET_FD_INVALID;
				logError("CAmSocketWrapper::", __func__," connect Error! errno:", socket_close_errno, "(", std::string(strerror(socket_close_errno)), ")");
				ret = false;
			}
		}
		else{
			logInfo("CAmSocketWrapper::", __func__," connect Success!! ConnectFD:", tmpConnectFD);
			ret = true;
		}
	}
	else{
		ret_out = close(tmpConnectFD);
		socket_close_errno = errno;
		if( ret_out < 0 ){
			logError("CAmSocketWrapper::", __func__, " , Connect Close result:",ret_out, ", errno:", socket_close_errno, "(", strerror(socket_close_errno), ")");
		}
		tmpConnectFD = D_AM_SOCKET_FD_INVALID;
		logError("CAmSocketWrapper::", __func__," :tblselect Error! ip_addr:", tmpConnectAddr, ", port_no:", tmpConnectPort, "socket_errno:", socket_close_errno);
		ret = false;
	}

	iter->second.ConnectFD = tmpConnectFD;

	return ret;
}

/****************************************************************************************/
/* リスニングSocket初期処理																*/
/* name:		socketInitialize														*/
/* overview:	socket生成(socket)、アドレス付与(bind)、接続待ち(listen)を実施し、		*/
/* 				生成されたリスニングSocketのファイルディスクリプタを出力する			*/
/* param(out):	pSock	ファイルディスクリプタ											*/
/* ret:			true	正常															*/
/* 				false	異常															*/
/****************************************************************************************/
bool CAmSocketWrapper::socketInitialize( int *pSock )
{
	int					socket_errno;			/* エラー番号									*/
	int					err				= 0;	/* 外部関数戻り値								*/
	struct sockaddr_in	sockaddr;				/* 受信IP and Port								*/
	int					reuse_addr_opt	= 1;	/* ソケットオプション(SO_REUSEADDR)の設定値		*/
	int					tcp_nodelay_opt	= 1;	/* ソケットオプション(TCP_NODELAY)の設定値		*/

	logInfo("CAmSocketWrapper::", __func__," called");
	/* 引数チェック		*/
	if( NULL == pSock )
	{
		logError("CAmSocketWrapper::", __func__," argerr");
		return false;
	}

	if( (mListenAddress == NULL) || (mListenPort <= 0) ){
		logError("CAmSocketWrapper::", __func__," Addres or Port is nothing! address:", mListenAddress, ", port:", mListenPort);
		return false;
	}

	/* socket生成	*/
	*pSock = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );
	socket_errno = errno;
	if( *pSock == D_AM_SOCKET_FD_INVALID ) {
		logError("CAmSocketWrapper::", __func__," socket Error! errno:", socket_errno, "(", std::string(strerror(socket_errno)), ")");
		return false;
	}

	/* ソケットオプションを設定(TIME_WAIT状態でもBind可能とする) */
	err = setsockopt( *pSock, SOL_SOCKET, SO_REUSEADDR, (const char *)&reuse_addr_opt, sizeof(reuse_addr_opt) );
	socket_errno = errno;
	if( err < 0 ) {
		logError("CAmSocketWrapper::", __func__," setsockopt(SO_REUSEADDR) Error! errno:", socket_errno, "(", std::string(strerror(socket_errno)), ")");
	}

	/* ソケットオプションを設定(ネーグルアルゴリズムを無効とする) */
	err = setsockopt( *pSock, IPPROTO_TCP, TCP_NODELAY, (const char *)&tcp_nodelay_opt, sizeof(tcp_nodelay_opt) );
	socket_errno = errno;
	if( err < 0 ) {
		logError("CAmSocketWrapper::", __func__," setsockopt(TCP_NODELAY) Error! errno:", socket_errno, "(", std::string(strerror(socket_errno)), ")");
	}

	/* アドレス付与	*/
	memset( &sockaddr, 0, sizeof(sockaddr) );
	sockaddr.sin_family			= AF_INET;
	sockaddr.sin_addr.s_addr	= inet_addr( mListenAddress );
	sockaddr.sin_port			= htons( mListenPort );

	err = bind( *pSock, (struct sockaddr*) &sockaddr, sizeof(sockaddr) );
	socket_errno = errno;
	if( err < 0 ) {
		logError("CAmSocketWrapper::", __func__," bind Error! errno:", socket_errno, "(", std::string(strerror(socket_errno)), ")");
		return false;
	}

	/* 接続待ち(リスニングソケット化)	*/
	err = listen( *pSock, D_AM_SOCKET_LISTEN_QUE_SIZE );
	socket_errno = errno;
	if( err < 0 ) {
		logError("CAmSocketWrapper::", __func__," listen Error! errno:", socket_errno, "(", std::string(strerror(socket_errno)), ")");
		return false;
	}
	logInfo("CAmSocketWrapper::", __func__," end ListenFD:", *pSock);

	return true;
}

/****************************************************************************************/
/* passive socket生成処理																*/
/* name:		socketCreate															*/
/* overview:	socket生成(socket)を実施、生成されたファイルディスクリプタを出力する	*/
/* param(out):	pSock	ファイルディスクリプタ											*/
/* ret:			true	正常															*/
/* 				false	異常															*/
/****************************************************************************************/
bool CAmSocketWrapper::socketCreate( int *pSock )
{
	int	err = 0;				/* 外部関数戻り値	*/
	int	socket_errno;			/* エラー番号		*/
	int	reuse_addr_opt	= 1;	/* ソケットオプション(SO_REUSEADDR)の設定値		*/
	int	tcp_nodelay_opt	= 1;	/* ソケットオプション(TCP_NODELAY)の設定値		*/

	logInfo("CAmSocketWrapper::", __func__," called");
	/* 引数チェック		*/
	if( NULL == pSock ) {
		logError("CAmSocketWrapper::", __func__," argerr");
		return false;
	}

	/* socket生成	*/
	*pSock = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );
	socket_errno = errno;
	if( *pSock == D_AM_SOCKET_FD_INVALID ){
		logError("CAmSocketWrapper::", __func__," socket Error errno=", socket_errno, "(", std::string(strerror(socket_errno)), ")");
		return false;
	}

	/* ソケットオプションを設定(TIME_WAIT状態でもBind可能とする) */
	err = setsockopt( *pSock, SOL_SOCKET, SO_REUSEADDR, (const char *)&reuse_addr_opt, sizeof(reuse_addr_opt) );
	socket_errno = errno;
	if( err < 0 ) {
		logError("CAmSocketWrapper::", __func__," setsockopt(SO_REUSEADDR) Error errno=", socket_errno, "(", std::string(strerror(socket_errno)), ")");
	}

	/* ソケットオプションを設定(ネーグルアルゴリズムを無効とする) */
	err = setsockopt( *pSock, IPPROTO_TCP, TCP_NODELAY, (const char *)&tcp_nodelay_opt, sizeof(tcp_nodelay_opt) );
	socket_errno = errno;
	if( err < 0 ) {
		logError("CAmSocketWrapper::", __func__," setsockopt(TCP_NODELAY) Error errno=", socket_errno, "(", std::string(strerror(socket_errno)), ")");
	}
	logInfo("CAmSocketWrapper::", __func__," End ConnectFD = ", *pSock);

	return true;
}

/****************************************************************************************/
/* SocketRelease(All)																	*/
/* name:		socketClose																*/
/* overview:	closing created sockeFD													*/
/* ret:			None																	*/
/****************************************************************************************/
void CAmSocketWrapper::socketClose( void )
{
	int socket_errno;		/* SystemCall errno */
	int result = 0;			/* close return value */
	am_Error_e ret = E_OK;	/* external function return value */
	listTempAcceptFD_t::iterator itertemp;
	FileDescripterMap_t::iterator iterMng;

	logInfo("CAmSocketWrapper::", __func__," called");

	if( mpSocketHandler != NULL ){
		/* delete ListenFD from  poll */
		ret = mpSocketHandler->removeFDPoll(mListenPollHandle);
		if( ret != E_OK ){
			logError("CAmSocketWrapper::", __func__," removeFDPoll Error ret:", ret, ", ListenPollHandle:", mListenPollHandle);
		}
		mListenPollHandle = 0;

		/* delete AcceptFD from poll(tying table with the forwarding address) */
		iterMng = mFileDescripterMap.begin();
		for( ; iterMng != mFileDescripterMap.end(); ++iterMng){
			ret = mpSocketHandler->removeFDPoll(iterMng->second.AcceptPollHandle);
			if( ret != E_OK ){
				logError("CAmSocketWrapper::", __func__," removeFDPoll Error ret:", ret, ", AcceptPollHandle:", iterMng->second.AcceptPollHandle);
			}
			iterMng->second.AcceptPollHandle = 0;
		}

		/* delete AcceptFD from poll(temporary storage area) */
		itertemp = mTempAcceptFD.begin();
		for( ; itertemp != mTempAcceptFD.end(); ++itertemp){
			ret = mpSocketHandler->removeFDPoll(itertemp->AcceptPollHandle);
			if( ret != E_OK ){
				logError("CAmSocketWrapper::", __func__," removeFDPoll Error ret:", ret, ", AcceptPollHandle:", itertemp->AcceptPollHandle);
			}
		}
	}

	/* release FD holding tying table with forwarding address */
	iterMng = mFileDescripterMap.begin();
	for( ; iterMng != mFileDescripterMap.end(); ++iterMng){
		/* release ConnectSocket	*/
		if( iterMng->second.ConnectFD != D_AM_SOCKET_FD_INVALID ){
			logInfo("CAmSocketWrapper::", __func__," AcceptSocket close ConnectFD:", iterMng->second.ConnectFD);
			result = close( iterMng->second.ConnectFD );
			socket_errno = errno;
			if( result < 0 ){
				logError("CAmSocketWrapper::", __func__," ConnectSocket Close Error! errno:", socket_errno, "(", std::string(strerror(socket_errno)), ")");
			}
		}
		iterMng->second.ConnectFD = D_AM_SOCKET_FD_INVALID;

		/* release AcceptSocket	*/
		if( iterMng->second.AcceptFD != D_AM_SOCKET_FD_INVALID ){
			logInfo("CAmSocketWrapper::", __func__," AcceptSocket close AcceptFD:", iterMng->second.AcceptFD);
			result = close( iterMng->second.AcceptFD );
			socket_errno = errno;
			if( result < 0 ){
				logError("CAmSocketWrapper::", __func__," AcceptSocket Close Error! errno:", socket_errno, "(", std::string(strerror(socket_errno)), ")");
			}
		}
		iterMng->second.AcceptFD = D_AM_SOCKET_FD_INVALID;
	}

	/* release FD holding temporary storage area */
	itertemp = mTempAcceptFD.begin();
	for( ; itertemp != mTempAcceptFD.end(); ++itertemp){
		/* release AcceptSocket	*/
		if( itertemp->AcceptFD != D_AM_SOCKET_FD_INVALID ){
			logInfo("CAmSocketWrapper::", __func__," AcceptSocket close AcceptFD:", itertemp->AcceptFD);
			result = close( itertemp->AcceptFD );
			socket_errno = errno;
			if( result < 0 ){
				logError("CAmSocketWrapper::", __func__," AcceptSocket Close Error! errno:", socket_errno, "(", std::string(strerror(socket_errno)), ")");
			}
		}
		itertemp->AcceptFD = D_AM_SOCKET_FD_INVALID;
	}
	mTempAcceptFD.clear();

	/* release ListeningSocket	*/
	if( mListenFD != D_AM_SOCKET_FD_INVALID ){
		logInfo("CAmSocketWrapper::", __func__," AcceptSocket close ListenFD:", mListenFD);
		result = close( mListenFD );
		socket_errno = errno;
		if( result < 0 ){
			logError("CAmSocketWrapper::", __func__," ListeningSocket Close Error! errno:", socket_errno, "(", std::string(strerror(socket_errno)), ")");
		}
	}
	mListenFD = D_AM_SOCKET_FD_INVALID;

	logInfo("CAmSocketWrapper::", __func__," End");

	return;
}

/****************************************************************************************/
/* SocketRelease(per File Descripter)													*/
/* name:		socketCloseforSingleFd													*/
/* overview:	closing created sockeFD(per File Descripter)							*/
/* param(in):	type	type of FileDescrepter											*/
/*						>>EN_FD_TYPE_SOCKET_ACCEPT(0)									*/
/*						>>EN_FD_TYPE_SOCKET_CONNECT(1)									*/
/*				fd		value of FileDescrepter											*/
/* ret:			None																	*/
/****************************************************************************************/
void CAmSocketWrapper::socketCloseforSingleFd( FD_TYPE_SOCKET type, int fd )
{
	int socket_errno;		/* SystemCall errno */
	int result = 0;			/* close return value */
	am_Error_e ret = E_OK;	/* external function return value */
	listTempAcceptFD_t::iterator itertemp;
	FileDescripterMap_t::iterator iterMng;

	logInfo("CAmSocketWrapper::", __func__," called, FD Type:", type, ", fd:", fd);

	if( ( (EN_FD_TYPE_SOCKET_ACCEPT  != type)
	    &&(EN_FD_TYPE_SOCKET_CONNECT != type) )
	  ||(D_AM_SOCKET_FD_INVALID == fd) ){
		logError("CAmSocketWrapper::", __func__," param Error type:", type, ", fd:", fd);
	  	return;
	}

	if( (NULL == mpSocketHandler)
	  &&(EN_FD_TYPE_SOCKET_ACCEPT == type) ){
		logError("CAmSocketWrapper::", __func__," SocketHandler is NULL.");
	  	return;
	}

	if( EN_FD_TYPE_SOCKET_ACCEPT == type ){
		iterMng = mFileDescripterMap.begin();
		for( ; iterMng != mFileDescripterMap.end(); ++iterMng){
			if( fd == iterMng->second.AcceptFD ){
				/* delete AcceptFD from poll(tying table with the forwarding address) */
				ret = mpSocketHandler->removeFDPoll(iterMng->second.AcceptPollHandle);
				if( ret != E_OK ){
					logError("CAmSocketWrapper::", __func__," removeFDPoll Error ret:", ret, ", AcceptPollHandle:", iterMng->second.AcceptPollHandle);
				}
				iterMng->second.AcceptPollHandle = 0;
				/* release AcceptSocket	*/
				logInfo("CAmSocketWrapper::", __func__," AcceptSocket close AcceptFD:", fd);
				result = close( fd );
				socket_errno = errno;
				if( result < 0 ){
					logError("CAmSocketWrapper::", __func__," AcceptSocket Close Error! errno:", socket_errno, "(", std::string(strerror(socket_errno)), ")");
				}
				iterMng->second.AcceptFD = D_AM_SOCKET_FD_INVALID;
				return;
			}
		}

		itertemp = mTempAcceptFD.begin();
		for( ; itertemp != mTempAcceptFD.end(); ++itertemp){
			if( fd == itertemp->AcceptFD ){
				/* delete AcceptFD from poll(temporary storage area) */
				ret = mpSocketHandler->removeFDPoll(itertemp->AcceptPollHandle);
				if( ret != E_OK ){
					logError("CAmSocketWrapper::", __func__," removeFDPoll Error ret:", ret, ", AcceptPollHandle:", itertemp->AcceptPollHandle);
				}
				/* release AcceptSocket	*/
				logInfo("CAmSocketWrapper::", __func__," AcceptSocket close AcceptFD:", fd);
				result = close( fd );
				socket_errno = errno;
				if( result < 0 ){
					logError("CAmSocketWrapper::", __func__," AcceptSocket Close Error! errno:", socket_errno, "(", std::string(strerror(socket_errno)), ")");
				}
				mTempAcceptFD.erase(itertemp);
				return;
			}
		}
	}

	if( EN_FD_TYPE_SOCKET_CONNECT == type ){
		/* release FD holding tying table with forwarding address */
		iterMng = mFileDescripterMap.begin();
		for( ; iterMng != mFileDescripterMap.end(); ++iterMng){
			if( fd == iterMng->second.ConnectFD ){
				/* release ConnectSocket	*/
				logInfo("CAmSocketWrapper::", __func__," ConnectSocket close ConnectFD:", fd);
				result = close( fd );
				socket_errno = errno;
				if( result < 0 ){
					logError("CAmSocketWrapper::", __func__," ConnectSocket Close Error! errno:", socket_errno, "(", std::string(strerror(socket_errno)), ")");
				}
				iterMng->second.ConnectFD = D_AM_SOCKET_FD_INVALID;
				return;
			}
		}
	}

	return;
}

/****************************************************************************************/
/* SocketReconnect Process																*/
/* name:		socketReConnect															*/
/* overview:																			*/
/* 																						*/
/* param(in):	AcceptFd	FD getting by accept										*/
/* ret:			true		Normal														*/
/* 				false		Fault														*/
/****************************************************************************************/
bool CAmSocketWrapper::socketReConnect( int AcceptFd )
{
	bool	bRret = false;		/* 戻り値				*/

	logInfo("CAmSocketWrapper::", __func__," called usedType=", mSocketUsedType, "mReConnectFlag", mReConnectFlag);

	/* 既にReConnect実施中の場合は正常終了 */
	if( mReConnectFlag == false ){
		mReConnectFlag = true;
	}
	else{
		bRret = true;
		return bRret;
	}

	/* socketFD解除 */
	socketCloseforSingleFd(EN_FD_TYPE_SOCKET_ACCEPT, AcceptFd);

	/* In case of Plugin */
	if( EN_SOCKET_USED_TYPE_CLIENT != mSocketUsedType ){
		return true;
	}

	/* Socket接続初期化処理	*/
	bRret = socketInitialize( &mListenFD );
	if( bRret != true ){
		logError("CAmSocketWrapper::", __func__," socketInitialize error bRret=", bRret);
		mListenFD = D_AM_SOCKET_FD_INVALID;
		return bRret;
	}

	/* 接続要求待ち開始 */
	bRret = requestAddFDpoll( mpSocketHandler );
	if( bRret != true ){
		logError("CAmSocketWrapper::", __func__," requestAddFDpoll error bRret=", bRret);
		close(mListenFD);
		mListenFD = D_AM_SOCKET_FD_INVALID;
		return bRret;
	}

	return bRret;
}

/****************************************************************************************/
/* passive socket登録依頼																*/
/* name:		requestAddFDpoll														*/
/* overview:	socketHandlerへaddFDpollの実行を依頼する								*/
/* param(in):	socketHandler	CAmSocketHandlerクラスインスタンスポインタ				*/
/* ret:			true	正常															*/
/* 				false	異常															*/
/****************************************************************************************/
bool CAmSocketWrapper::requestAddFDpoll( CAmSocketHandler*& socketHandler )
{
    int16_t			event = 0;	/* poll event		*/
	sh_pollHandle_t	handle;		/* poll handle		*/

	logDebug("CAmSocketWrapper::", __func__," called");
	/* 引数チェック		*/
	if( NULL == socketHandler ){
		logError("CAmSocketWrapper::", __func__," argerr");
		return false;
	}

	/* POLLイベント設定		*/
	event |= POLLIN;

	/* addFDPoll実行		*/
	logInfo("CAmSocketWrapper::", __func__," addFDPoll, ListenFD(", mListenFD, ", event(", event, ")");
	am_Error_e error = socketHandler->addFDPoll( mListenFD, event, NULL, &pSocketAcceptCallback, NULL, NULL, NULL, handle );

	if( error != E_OK || handle == 0 ){
		logError("CAmSocketWrapper::", __func__," addFDPollError error=", error, ", handle=", handle);
		return false;
	}
	/* ListenFDのhandle保持 */
	mListenPollHandle = handle;
	/* socketHandlerの保持 */
	mpSocketHandler = socketHandler;

	return true;
}

/****************************************************************************************/
/* Socket通信用FD生成状態確認															*/
/* name:		checkSocketFD															*/
/* overview:	Socket通信に使用するlisten用FDとconnect用FDが生成されているか確認する	*/
/* param:		なし																	*/
/* ret:			true	FD既生成														*/
/* 				false	FD未生成														*/
/****************************************************************************************/
bool CAmSocketWrapper::checkSocketFD( void )
{
	logDebug("CAmSocketWrapper::", __func__," called");

	if( D_AM_SOCKET_FD_INVALID == mListenFD ){
		/* in case of creating FD for listen	*/
		logError("CAmSocketWrapper::", __func__," Invalid ListenFD=", mListenFD);
		return false;
	}

	FileDescripterMap_t::iterator iter = mFileDescripterMap.begin();
	for( ; iter != mFileDescripterMap.end(); ++iter){
		if( D_AM_SOCKET_FD_INVALID == iter->second.ConnectFD ){
			/* in case of creating FD for connect	*/
			logError("CAmSocketWrapper::", __func__," Invalid ConnectFD=",  iter->second.ConnectFD);
			return false;
		}
	}

	return true;
}

/****************************************************************************************/
/* FileDescripter取得																	*/
/* name:		getFileDescripter														*/
/* overview:	指定されたFDを取得する													*/
/* param:		type:EN_FD_TYPE_SOCKET_ACCEPT:Accept									*/
/*					  EN_FD_TYPE_SOCKET_CONNECT:Connect									*/
/*				destaddr：Forwarding address											*/
/* ret:			Accept済みFD															*/
/****************************************************************************************/
int CAmSocketWrapper::getFileDescripter( FD_TYPE_SOCKET type, unsigned short destaddr )
{
	int tmp_FD = D_AM_SOCKET_FD_INVALID;

	FileDescripterMap_t::iterator iter = mFileDescripterMap.find(destaddr);
	if( iter == mFileDescripterMap.end() ){
		logError("CAmSocketWrapper::getFileDescripter No Dest!!, addr=", destaddr);
		return D_AM_SOCKET_FD_INVALID;
	}

	switch( type )
	{
		case EN_FD_TYPE_SOCKET_ACCEPT:
			tmp_FD = iter->second.AcceptFD;
			break;

		case EN_FD_TYPE_SOCKET_CONNECT:
			tmp_FD = iter->second.ConnectFD;
			break;

		default:
			logError("CAmSocketWrapper::getFileDescripter FD is empty, type=", type, ", addr=", destaddr);
			break;
	}

	logDebug("CAmSocketWrapper::getFileDescripter called, type=", type, ", FD=", tmp_FD);

	return tmp_FD;
}

/****************************************************************************************/
/* Socket通信確立用アドレス・ポート・転送元論理アドレスの設定							*/
/* name:		setSocketSetting														*/
/* overview:	通信確立のためのポートを設定する										*/
/* param:		usedtype：EN_SOCKET_USED_TYPE_PLUGIN:Plugin用として設定					*/
/*					  	  EN_SOCKET_USED_TYPE_CLIENT:Client用として設定					*/
/* ret:			なし																	*/
/****************************************************************************************/
void CAmSocketWrapper::setSocketSetting( SOCKET_USED_TYPE usedtype, SOCKET_TYPE type )
{
	int tblcnt = 0;
	int tblsize = 0;
	stFileDescripterTbl fdtbl;

	logDebug("CAmSocketWrapper::", __func__," called, usedtype=", usedtype, ", type=", type);

	tblsize = sizeof(m_SocketInitTbl)/sizeof(stSocketTbl);
	mFileDescripterMap.clear();

	/* initialize FD managiment table */
	memset(&fdtbl, 0, sizeof(stFileDescripterTbl));
	fdtbl.AcceptFD = D_AM_SOCKET_FD_INVALID;
	fdtbl.ConnectFD = D_AM_SOCKET_FD_INVALID;
	fdtbl.AcceptPollHandle = 0;

	/* Pluginで使用するための設定 */
	if( EN_SOCKET_USED_TYPE_PLUGIN == usedtype ){
		mListenAddress = D_AM_SOCKET_ADDRESS_AM;
		mListenPort = D_AM_SOCKET_PORT_AM;
		/* ドメインに対応したポート番号とIPアドレスを取得					*/
		/* (ドメイン用テーブルのため、設定値をlisten/connect逆に読み替える) */
		for(tblcnt = 0; tblcnt < tblsize; tblcnt++){
			fdtbl.ConnectAddress = m_SocketInitTbl[tblcnt].listenaddr;
			fdtbl.ConnectPort = m_SocketInitTbl[tblcnt].listenport;
			mFileDescripterMap.insert(std::make_pair(m_SocketInitTbl[tblcnt].srcaddr, fdtbl));
		}
		mSrcAddress = D_AM_DOMAIN_ADDRESS_AM; /* AMの転送元論理アドレスを取得 */
	}
	/* Domainで使用するための設定 */
	else if( EN_SOCKET_USED_TYPE_CLIENT == usedtype ){
		fdtbl.ConnectAddress = D_AM_SOCKET_ADDRESS_AM;
		fdtbl.ConnectPort = D_AM_SOCKET_PORT_AM;
		mFileDescripterMap.insert(std::make_pair(D_AM_DOMAIN_ADDRESS_AM, fdtbl));
		/* ドメインに対応したポート番号、IPアドレス、転送元論理アドレスを取得	*/
		for(tblcnt = 0; tblcnt < tblsize; tblcnt++){
			if( m_SocketInitTbl[tblcnt].type == type ){
				mListenAddress = m_SocketInitTbl[tblcnt].listenaddr;
				mListenPort = m_SocketInitTbl[tblcnt].listenport;
				mSrcAddress = m_SocketInitTbl[tblcnt].srcaddr;
				break;
			}
		}
	}
	else
	{
		logError("CAmSocketWrapper::", __func__," EN_SOCKET_USED_TYPE_INVALID");
	}
	logInfo("CAmSocketWrapper::", __func__,", lstnAddr=", mListenAddress, ", lstnPort=", mListenPort,
											", SrcAddr=", mSrcAddress);

	FileDescripterMap_t::iterator iter = mFileDescripterMap.begin();
	for( ; iter != mFileDescripterMap.end(); ++iter){
		logInfo("CAmSocketWrapper::", __func__,", DestAddr =", iter->first,
												", connectAddr=", iter->second.ConnectAddress,
												", connectPort=", iter->second.ConnectPort);
	}
}

/****************************************************************************************/
/* 転送元論理アドレス取得																*/
/* name:		getSrcAddress															*/
/* overview:	転送元論理アドレスを取得する											*/
/* param:		なし																	*/
/* ret:			転送元論理アドレス														*/
/****************************************************************************************/
unsigned short CAmSocketWrapper::getSrcAddress( void )
{
	logDebug("CAmSocketWrapper::", __func__," srcAddr=", mSrcAddress);
	return mSrcAddress;
}

/****************************************************************************************/
/* Get processing AcceptFD																*/
/* name:		getCurrentAcceptFD														*/
/* overview:	Get receiving acceptFD now												*/
/* param:		none																	*/
/* ret:			receiving acceptFD														*/
/****************************************************************************************/
int CAmSocketWrapper::getCurrentAcceptFD( void )
{
	logDebug("CAmSocketWrapper::", __func__," fd=", mCurrentAcceptFD);

	return mCurrentAcceptFD;
}

/****************************************************************************************/
/* Notification receiving forwarding address											*/
/* name:		notificationRecvDest													*/
/* overview:	Notification that forwarding address is received						*/
/* param(in):	forwarding address														*/
/* ret:			none																	*/
/****************************************************************************************/
void CAmSocketWrapper::notificationRecvDest( unsigned short destaddr )
{
	logDebug("[multidomain]CAmSocketWrapper::",__func__," called, destaddr = ", destaddr);

	FileDescripterMap_t::iterator iter = mFileDescripterMap.find( destaddr );
	if( iter == mFileDescripterMap.end() ){
		logError("[multidomain]CAmSocketWrapper::",__func__," destaddr is nothing");
		return;
	}

	/* for moving AcceptFD saved tamporary at accept(same as receiving AcceptFD) to tying table,
	   delete AcceptFD from tsemporary saving table */
	listTempAcceptFD_t::iterator iterlist = mTempAcceptFD.begin();
	for( ; iterlist != mTempAcceptFD.end(); ++iterlist){
		if( iterlist->AcceptFD == mCurrentAcceptFD ){
			/* link forwarding address to receiving AcceptFD */
			iter->second.AcceptFD = iterlist->AcceptFD;
			iter->second.AcceptPollHandle = iterlist->AcceptPollHandle;
			mTempAcceptFD.erase(iterlist);
			break;
		}
	}

	return;
}

