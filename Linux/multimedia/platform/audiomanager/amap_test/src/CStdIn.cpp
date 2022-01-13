/************************************************************************
 * @file: CStdIn.cpp
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

#include <fstream>
#include "CStdIn.h"
#include "CPlayer.h"
#include "CAmInterface.h"
#include "CAmDltWrapper.h"
#include "audiomanagertypes.h"

using namespace am;
using namespace std;


CStdIn::CStdIn(CAmSocketHandler *socketHandler, IAmInterface *interface, IPlayer *player) :
        recveiveCB(this, &CStdIn::receiveData),
        checkCB(this, &CStdIn::checkData),
        mPollHandle(), mpSocketHandler(socketHandler),
        mpPlayer(player), mpInterface(interface), mSerializer(socketHandler)
{
    short events = POLLIN;
    mpSocketHandler->addFDPoll(STDIN_FILENO, events, NULL, &recveiveCB, &checkCB, NULL, NULL, mPollHandle);
}

CStdIn::~CStdIn()
{
    mpSocketHandler->removeFDPoll(mPollHandle);
}

void CStdIn::receiveData(const pollfd, const sh_pollHandle_t, void*)
{
    cin >> mCommand;
    logWarning("CStdIn::receiveData", mCommand);
}

bool CStdIn::checkData(const sh_pollHandle_t, void*)
{
    if (mCommand.empty())
        return false;

    if (!mCommand.compare("p"))
        mSerializer.asyncCall<IPlayer>(mpPlayer, &IPlayer::pause);
    if (!mCommand.compare("r"))
        mSerializer.asyncCall<IPlayer>(mpPlayer, &IPlayer::play);
    if (!mCommand.compare("s"))
        mSerializer.asyncCall<IPlayer>(mpPlayer, &IPlayer::stop);
    if (!mCommand.compare("n"))
        mSerializer.asyncCall<IPlayer>(mpPlayer, &IPlayer::next);
    if (!mCommand.compare("q"))
        mSerializer.asyncCall<IAmInterface>(mpInterface, &IAmInterface::end);

    return true;
}

