/************************************************************************
 * @file: CRaMockSender.cpp
 *
 * @version: 0.1
 *
 * @author: Jens Lorenz, jlorenz@de.adit-jv.com 2015
 *
 * @copyright (c) 2015 Advanced Driver Information Technology.
 * This code is developed by Advanced Driver Information Technology.
 * Copyright of Advanced Driver Information Technology, Bosch, and DENSO.
 * All rights reserved.
 ***********************************************************************/

#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdarg.h>
#include <algorithm>
#include "audiomanagertypes.h"
#include "CAmDltWrapper.h"
#include "CRaFuncBlock.h"
#include "IRaMockReceiverShadow.h"
#include "CRaMockSender.h"

using namespace am;
using namespace std;

#define PORT_OFFSET      0xC700
#define AUDIO_PORT      (0x16 | PORT_OFFSET)


static void ERR(const char *format, ...)
{
    char strError[1024];

    va_list arg;
    va_start(arg, format);
    vsprintf(strError, format, arg);
    va_end(arg);

    cerr << strError << endl;
    logError(strError);
}

CRaMockSender::CRaMockSender(CAmSocketHandler *socketHandler, int domain,
    IRaMockReceiverShadow * rs, initializer_list<FBFunc *> i) :
        prepareCB(this, &CRaMockSender::prepareData), //
        recveiveCB(this, &CRaMockSender::receiveData), //
        checkCB(this, &CRaMockSender::checkData), //
        dispatchCB(this, &CRaMockSender::dispatchData), //
        mSocketFd(-1), mpSocketHandler(socketHandler), //
        mPollHandle(), mList(begin(i), end(i))
{
    int socketFd = -1;
    struct hostent *local = NULL, *remote = NULL;
    struct sockaddr_in local_addr, remote_addr;

    local = gethostbyname("audiomock-local");
    if (local == NULL)
    {
        throw runtime_error(string("CRaMockSender::CRaMockSender gethostbyname(audiomock-local) failed: ") + string(strerror(errno)));
    }
    local_addr.sin_family = AF_INET;
    memcpy((char *) &local_addr.sin_addr.s_addr, (char *) local->h_addr, local->h_length);
    local_addr.sin_port = htons(AUDIO_PORT); /* from inc_ports.h */

    remote = gethostbyname("audiomock");
    if (remote == NULL)
    {
        throw runtime_error(string("CRaMockSender::CRaMockSender gethostbyname(audiomock) failed: ") + string(strerror(errno)));
    }
    remote_addr.sin_family = AF_INET;
    memcpy((char *) &remote_addr.sin_addr.s_addr, (char *) remote->h_addr, remote->h_length);
    remote_addr.sin_port = htons(AUDIO_PORT); /* from inc_ports.h */

    fprintf(stdout, "local  %s:%d\n", inet_ntoa(local_addr.sin_addr), ntohs(local_addr.sin_port));
    fprintf(stdout, "remote %s:%d\n", inet_ntoa(remote_addr.sin_addr), ntohs(remote_addr.sin_port));

    socketFd = socket(domain, SOCK_STREAM, 0);
    if (socketFd == -1)
    {
        throw runtime_error(string("CRaMockSender::CRaMockSender create socket domain address ") + \
                to_string(domain) + string(" failed: ") + string(strerror(errno)));
    }

    if (bind(socketFd, (struct sockaddr *) &local_addr, sizeof(local_addr)) < 0)
    {
        throw runtime_error(string("CRaMockSender::CRaMockSender bind failed: ") + string(strerror(errno)));
    }

    if (connect(socketFd, (struct sockaddr *) &remote_addr,
            sizeof(remote_addr)) < 0)
    {
        throw runtime_error(string("CRaMockSender::CRaMockSender connect failed: ") + string(strerror(errno)));
    }

    short events = POLLIN;
    mSocketFd = socketFd;
    mpSocketHandler->addFDPoll(mSocketFd, events, &prepareCB, &recveiveCB, &checkCB, &dispatchCB, NULL, mPollHandle);
    rs->registerAcknowledge(this);
}

CRaMockSender::~CRaMockSender()
{
    for (auto it = mList.begin(); it != mList.end(); ++it)
    {
        delete *it;
    }
}

void CRaMockSender::prepareData(const sh_pollHandle_t handle, void* userData)
{
    (void) handle;
    (void) userData;

    for (auto it = mList.begin(); it != mList.end(); ++it)
    {
        (*it)->SetSender(this);
    }
}

void CRaMockSender::receiveData(const pollfd pollfd, const sh_pollHandle_t handle, void* userData)
{
    (void) handle;
    (void) userData;

    unsigned char buffer[1024];
    int n = recv(pollfd.fd, &buffer, sizeof(buffer), 0);
    if (n <= 0)
    {
        if (n < 0)
        {
            ERR("CRaMockSender::receiveData failed: %s", strerror(errno));
        }
        else
        {
            ERR("CRaMockSender::receiveData failed: no data\n");
        }
        return;
    }
    mMsgList.push_back(ustring(buffer, n));
}

bool CRaMockSender::checkData(const sh_pollHandle_t handle, void* userData)
{
    (void) handle;
    (void) userData;

    if (mMsgList.empty())
    {
        return false;
    }

    const unsigned char* msg = mMsgList.front().c_str();
    unsigned short fblock = getU16(&msg[offsetof(struct t_msg, clientId)]);
    unsigned char instId = msg[offsetof(struct t_msg, instId)];

    if ((MOST_FBLOCK_ID != fblock) && (MOST_INST_ID != instId))
    {
        ERR("CRaMockSender::checkData Message for FBlock %04x instance %02x received", fblock, instId);
        mMsgList.pop_back();
        return false;
    }
    return true;
}

bool CRaMockSender::dispatchData(const sh_pollHandle_t handle, void* userData)
{
    (void) handle;
    (void) userData;

    const unsigned char* msg = mMsgList.front().c_str();
    unsigned short funcId = getU16(&msg[offsetof(t_msg, funcId)]);
    unsigned char opType = msg[offsetof(t_msg, opType)];
    auto it = find_if(mList.begin(), mList.end(),
            [funcId](FBFunc * const t) { return funcId == t->GetId();} );
    if (it != mList.end())
    {
        switch (opType)
        {
            case OP_TYPE_SET:
                (*it)->OperationSet(msg);
                break;
            case OP_TYPE_GET:
                (*it)->OperationGet(msg);
                break;
            case OP_TYPE_SETGET:
                (*it)->OperationSetGet(msg);
                break;
            default:
                ERR("CRaMockSender::dispatchCB Function %04x with operation 0x%02x not found", funcId, opType);
                break;
        }
    }
    else
    {
        ERR("CRaMockSender::dispatchCB Function %04x with operation 0x%02x not found", funcId, opType);
    }

    mMsgList.pop_back();
    return (!mMsgList.empty());
}

void CRaMockSender::acknowledge(const am_Handle_s handle, const void* data, const am_Error_e error)
{
    unsigned short funcId = handle.handle + 0xC00;
    auto it = find_if(mList.begin(), mList.end(),
            [funcId](FBFunc * const t) { return funcId == t->GetId();} );
    if (it != mList.end())
    {
        (*it)->AcknowledgeCommand(handle.handleType, data, error);
    }
}

void CRaMockSender::sendData(const unsigned char* msg, unsigned int length)
{
    int m = send(mSocketFd, msg, length, 0);
    if (m > 0)
    {
        ostringstream send("CRaMockSender::sendData ");
        for (unsigned int i = 0; i < (offsetof(t_msg, data) + length); i++)
        {
            send << std::hex << msg[i] << " ";
        }
        logDebug(send.str());
    }
    else
    {
        ERR("CRaMockSender::sendData send failed: %s", strerror(errno));
    }
}
