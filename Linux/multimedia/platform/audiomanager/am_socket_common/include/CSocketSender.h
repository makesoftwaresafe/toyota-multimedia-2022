/************************************************************************
 * @file: CDBusSender.h
 *
 * @version: 1.1
 *
 * @description: A CDBusSender class definition of Routing Adapter.
 * CDBusSender is used to send the data over DBus connection.
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

#ifndef CDBUSSENDER_H_
#define CDBUSSENDER_H_

#include <iostream>
#include <vector>
#include <string>
#include "CAmSocketWrapper.h"
#include "CSocketCommon.h"

namespace am
{

#define D_AM_RECEIVE_CNT		( 5 )				/* 再受信リトライ回数 */

class CSocketSender
{
public:
    CSocketSender( void );
    virtual ~CSocketSender();
    // set's the Socket wrapper pointer
	void setSocketWrapper( CAmSocketWrapper*& wrapper );

	// To make socket data
	void make_senddata(unsigned short opc, void* data = NULL, size_t size = 0);

	// To make synchronous(blocking) method call.
    am_Error_e send_sync(void* replydata, size_t replysize, unsigned short destaddr = D_AM_DOMAIN_ADDRESS_AM);
    // To make asynchronous(non-blocking) call.
    am_Error_e send_async(unsigned short destaddr = D_AM_DOMAIN_ADDRESS_AM);
    // Send's the reply back to Socket blocking call.
	am_Error_e send_sync_reply(unsigned short destaddr = D_AM_DOMAIN_ADDRESS_AM);

private:
	ST_SOCKET_MSG  mMsg;
	int mDatalen;
	CAmSocketWrapper* mpCAmSocketWrapper;
};

} /* namespace am */

#endif /* CDBUSSENDER_H_ */
