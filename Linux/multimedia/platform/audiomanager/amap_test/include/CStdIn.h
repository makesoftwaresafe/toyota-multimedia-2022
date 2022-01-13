/************************************************************************
 * @file: CStdIn.h
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

#ifndef _STD_INPUT_H_
#define _STD_INPUT_H_

#include "CAmSerializer.h"
#include "CAmSocketHandler.h"

namespace am {

class IPlayer;
class IAmInterface;

class CStdIn
{
public:
    CStdIn(CAmSocketHandler *socketHandler, IAmInterface *interface, IPlayer *player);
    ~CStdIn();

    void receiveData(const pollfd pollfd, const sh_pollHandle_t handle, void* userData);
    bool checkData(const sh_pollHandle_t handle, void* userData);

    TAmShPollFired<CStdIn> recveiveCB;
    TAmShPollCheck<CStdIn> checkCB;

private:
    sh_pollHandle_t       mPollHandle;
    std::string           mCommand;

    CAmSocketHandler     *mpSocketHandler;
    IPlayer              *mpPlayer;
    IAmInterface         *mpInterface;
    CAmSerializer         mSerializer;
};

}

#endif // _STD_INPUT_H_
