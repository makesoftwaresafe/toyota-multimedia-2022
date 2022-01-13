/************************************************************************
 * @file: main.cpp
 *
 * @version: 1.1
 *
 * @description: This file is to start the media player process.
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

#include "CAmDltWrapper.h"
#include "CAmInterface.h"
#include "CPlayer.h"


using namespace am;
using namespace std;

#define throw_assert(EXPRESSION, MESSAGE) \
        if (!(EXPRESSION)) throw std::runtime_error(MESSAGE)

#define MEDIAPLAYER     1
#define INTERRUPT       2
#define AMPLIFIER       1

#define MAX_INTERRUPTS  4
struct _tInterrupts{
    string  file;
    unsigned int timeout;
} gInterrupt[MAX_INTERRUPTS] = {
    { "please_turn_right.wav"           ,  3 },
    { "turn_right.wav"                  ,  3 },
    { "follow_the_road.wav"             , 10 },
    { "you_reached_your_destination.wav",  5 }
};


DLT_DECLARE_CONTEXT (CP_TestMain)
string strPath = "/opt/platform/unit_tests/";


void mainProgram(int argc, char **argv)
{
    am_Error_e error = E_OK;

    CAmInterface* pSendIf = new CAmInterface();
    pSendIf->startSocketHandler();
    CPlayer PlayerSource(argc, argv);
    am_mainConnectionID_t mediaplayerRoute = 0;
    am_mainConnectionID_t interruptRoute = 0;

    error = pSendIf->connect(MEDIAPLAYER, AMPLIFIER, mediaplayerRoute);
    throw_assert(error == E_OK, "MP route connect request failed");
    logInfo("Connection ID for media player source = ", mediaplayerRoute);
    PlayerSource.play("file://" + strPath + "BigBuckBunny.wav", "vdev_app_mediaplayer");
    error = pSendIf->waitForStateChange(mediaplayerRoute, CS_CONNECTED);
    throw_assert(error == E_OK, "MP route connection failed");

    sleep(5);

    for (int i = 0; i < MAX_INTERRUPTS; i++)
    {
        CPlayer InterruptSource(argc, argv);

        error = pSendIf->connect(INTERRUPT, AMPLIFIER, interruptRoute);
        throw_assert(error == E_OK, "Interrupt route connect request failed");
        logInfo("Connection ID for interrupt source = ", interruptRoute);
        error = pSendIf->waitForStateChange(interruptRoute, CS_CONNECTED);
        throw_assert(error == E_OK, "Interrupt route connection failed");

        InterruptSource.play("file://" + strPath + gInterrupt[i].file, "vdev_app_interrupt");
        InterruptSource.wait();

        error = pSendIf->disconnect(interruptRoute);
        throw_assert(error == E_OK, "Interrupt route disconnect failed");

        sleep(gInterrupt[i].timeout);
    }

    error = pSendIf->disconnect(mediaplayerRoute);
    throw_assert(error == E_OK, "MP route disconnect failed");
    error = pSendIf->waitForStateChange(mediaplayerRoute, CS_DISCONNECTED);
    throw_assert(error == E_OK, "MP route disconnection failed");

    PlayerSource.stop();
}

int main(int argc, char **argv)
{
    am_Error_e error = E_OK;

    // Initialize the DLT logs with new process and register
    CAmDltWrapper::instanctiateOnce("CMP", "Media Player");
    CAmDltWrapper::instance()->registerContext(CP_TestMain, "CP_Main", "CP_MainContext");

    logInfo("GENIVI AM - MediaPlayer test starts ...");

    try
    {
        mainProgram(argc, argv);
    }
    catch (exception& exc)
    {
        stringstream ss;
        ss << "GENIVI AM Test - Exception thrown:" << exc.what();
        logError(ss.str());
        cout << ss.str() << endl;
        CAmDltWrapper::instance()->unregisterContext(CP_TestMain);
        CAmDltWrapper* inst(CAmDltWrapper::instance());
        inst->deinit();
        return EXIT_FAILURE;
    }

    logInfo("GENIVI AM Test - Ended Successfully");

    CAmDltWrapper::instance()->unregisterContext(CP_TestMain);
    CAmDltWrapper* inst(CAmDltWrapper::instance());
    inst->deinit();

    return error;
}
