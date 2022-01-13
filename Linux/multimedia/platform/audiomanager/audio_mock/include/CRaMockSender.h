/************************************************************************
 * @file: CRaMockSender.h
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

#ifndef _MOCK_SENDER_H_
#define _MOCK_SENDER_H_

#include <initializer_list>
#include "CAmSocketHandler.h"

namespace am {

typedef std::basic_string<unsigned char> ustring;

class IRaMockReceiverShadow;
class FBFunc;

class IRaMockSender
{
public:
    IRaMockSender() {};
    virtual ~IRaMockSender() {};
    virtual void sendData(const unsigned char* msg, unsigned int length) = 0;
    virtual void acknowledge(const am_Handle_s handle, const void* data, const am_Error_e error) = 0;
};



class CRaMockSender : public IRaMockSender
{
public:
    CRaMockSender(CAmSocketHandler *socketHandler, int domain,
            IRaMockReceiverShadow * rs, std::initializer_list<FBFunc *> i);
    ~CRaMockSender();

    void prepareData(const sh_pollHandle_t handle, void* userData);
    void receiveData(const pollfd pollfd, const sh_pollHandle_t handle, void* userData);
    bool checkData(const sh_pollHandle_t handle, void* userData);
    bool dispatchData(const sh_pollHandle_t handle, void* userData);

    virtual void sendData(const unsigned char* msg, unsigned int length) override;
    virtual void acknowledge(const am_Handle_s handle, const void* data, const am_Error_e error) override;

    TAmShPollPrepare<CRaMockSender> prepareCB;
    TAmShPollFired<CRaMockSender> recveiveCB;
    TAmShPollCheck<CRaMockSender> checkCB;
    TAmShPollDispatch<CRaMockSender> dispatchCB;

private:
    int                   mSocketFd;
    CAmSocketHandler     *mpSocketHandler;
    sh_pollHandle_t       mPollHandle;
    std::vector<FBFunc*>  mList;
    std::vector<ustring>  mMsgList;
};

}

#endif // _MOCK_SENDER_H_
