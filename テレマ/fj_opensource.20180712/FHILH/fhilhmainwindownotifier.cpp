/*** COPYRIGHT FUJITSU LIMITED 2017 ***/

#include "fhilhmainwindownotifier.h"
#include <QtNetwork/qhostaddress.h>


FhiLhMainWindowNotifier::FhiLhMainWindowNotifier()
{
	// 初回描画フラグをfalse．
	mIsFirstDrawn = false;
	
	// ブラウザがMainWindowをhideしたタイミングをソケット切断で検出するハンドラを設定．
	QObject::connect(&mTcpSocket,SIGNAL(disconnected()),this,SLOT(onHideWindow()));
}

FhiLhMainWindowNotifier::~FhiLhMainWindowNotifier()
{
	if (mTcpSocket.isOpen())
	{
		mTcpSocket.close();
	}
}

void FhiLhMainWindowNotifier::notifyDrawing()
{
	// ブラウザにソケット接続で初回描画を通知．
    if (mIsFirstDrawn == false)
    {
        mIsFirstDrawn = true;
        mTcpSocket.connectToHost(QHostAddress::LocalHost, 50006);
    }
}

void FhiLhMainWindowNotifier::onHideWindow()
{
	// ブラウザがMainWindowをhideした場合は、
	// 初回描画フラグをfalseにして次回の初回描画に備える．
	mIsFirstDrawn = false;
	mTcpSocket.close();
}

