/*
 * audio-mock.c
 */

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <iostream>
#include <unistd.h>
#include <vector>
#include "CRaLoader.h"
#include "CRaFuncBlock.h"
#include "CRaMockSender.h"
#include "audiomanagertypes.h"
#include "CAmSocketHandler.h"
#include "IRaMockReceiverShadow.h"
#include "CRaALSASender.h"


#define X86_SOCKET_ADDR  40


using namespace am;
using namespace std;


CAmSocketHandler *gSocketHandler = NULL;

void printHelp(const char* name)
{
    cout << "Usage: " << name << " -c path [-s number] [-h]" << endl;
    cout << "\t -c link to routing plugin" << endl;
    cout << "\t -s socket domain/protocol family [default = " << X86_SOCKET_ADDR << "]" << endl;
    cout << "\t -h this help message" << endl;
}

void outOfMemoryHandler()
{
    logError("No more memory - bye");
    exit(1);
}

static void signalHandler(int sig, siginfo_t *siginfo, void *context)
{
    (void) sig;
    (void) siginfo;
    (void) context;

    if (!gSocketHandler)
    {
        logInfo("Signal handler was called, signal", sig);
    }

    switch (sig)
    {
        case SIGHUP:
        case SIGINT:
        case SIGQUIT:
        case SIGTERM:
            gSocketHandler->stop_listening();
            gSocketHandler->exit_mainloop();
            logInfo("MOCK stopped, signal", sig);
            break;
        default:
            logInfo("Signal handler was called, signal", sig);
            break;
    }
}

int main(int argc, char **argv)
{
    char *path = NULL;
    int domain = X86_SOCKET_ADDR; /* x86: 40, ARM: 41 */

    int option = 0;
    while ((option = getopt(argc, argv, "c:s:")) != -1)
    {
        switch (option)
        {
            case 'c':
                path = optarg;
                break;
            case 's':
                domain = atoi(optarg);
                break;
            default: /* '?' */
                printHelp(argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    if (path == NULL)
    {
        cerr << "Path for routing plug-in library invalid!" << endl;
        printHelp(argv[0]);
        exit(EXIT_FAILURE);
    }

    //now the signal handler:
    struct sigaction signalAction;
    memset(&signalAction, '\0', sizeof(signalAction));
    signalAction.sa_sigaction = &signalHandler;
    signalAction.sa_flags = SA_SIGINFO;
    sigaction(SIGINT, &signalAction, NULL);
    sigaction(SIGQUIT, &signalAction, NULL);
    sigaction(SIGTERM, &signalAction, NULL);
    sigaction(SIGHUP, &signalAction, NULL);

    struct sigaction signalChildAction;
    memset(&signalChildAction, '\0', sizeof(signalChildAction));
    signalChildAction.sa_flags = SA_NOCLDWAIT;
    sigaction(SIGCHLD, &signalChildAction, NULL);

    //register new out of memory handler
    set_new_handler(&outOfMemoryHandler);

    // Initialize the DLT logs with new process and register
    CAmDltWrapper::instance(false)->registerApp("AMOK", "ADR3AudioMock");

    try
    {
        IAmRoutingSend *sender = NULL;
        gSocketHandler = new CAmSocketHandler;

        CRaLoader loader(path);
        loader.getRoutingInterface(sender);

        IRaMockReceiverShadow recv_mock(gSocketHandler);
        CRaMockSender send_mock(gSocketHandler, domain, &recv_mock, {
            { new FBFunc(sender, FUNC_ID_PING, "Ping") },
            { new FBPowerState(sender) },
            { new FBFunc(sender, FUNC_ID_MODE, "MaintainenceMode") },
            { new FBFunc(sender, FUNC_ID_SORC, "Source") },
            { new FBAudioRoute(sender) },
            { new FBVolume(sender) },
            { new FBVolumeOffset(sender) },
            { new FBFunc(sender, FUNC_ID_SETT, "Settings") },
            { new FBMute(sender) },
            { new FBDiagnosisResult(sender) },
            { new FBFunc(sender, FUNC_ID_DISP, "DiagnosisSpeaker") },
            { new FBFunc(sender, FUNC_ID_SCIT, "SoundConfigItem") },
            { new FBSoundConfigVersion(sender) },
            { new FBFunc(sender, FUNC_ID_CSET, "CarSetting") }
        });

        sender->startupInterface(&recv_mock);

        logInfo("MOCK started");

        gSocketHandler->start_listenting();
    }
    catch (exception& exc)
    {
        delete gSocketHandler;
        logError(exc.what());
        cerr << exc.what() << endl;
        exit(EXIT_FAILURE);
    }

    delete gSocketHandler;
    exit(0);
}
