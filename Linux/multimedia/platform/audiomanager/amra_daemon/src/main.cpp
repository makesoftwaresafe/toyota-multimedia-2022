/************************************************************************
 * @file: main.cpp
 *
 * @version: 0.1
 *
 * @author: Jens Lorenz, jlorenz@de.adit-jv.com 2013,2014
 *
 * @copyright (c) 2015 Advanced Driver Information Technology.
 * This code is developed by Advanced Driver Information Technology.
 * Copyright of Advanced Driver Information Technology, Bosch, and DENSO.
 * All rights reserved.
 ***********************************************************************/

#include <sys/resource.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cstdlib>
#include <cstdlib>
#include <cassert>
#include <fcntl.h>
#include <csignal>
#include <cstring>
#include <cstdio>
#include <new>

#include "IAmRouting.h"
#include "IAmRoutingClient.h"
#include "CAmDltWrapper.h"
#include "CAmSocketHandler.h"
#include "CAmCommandLineSingleton.h"

#ifdef WITH_SYSTEMD_WATCHDOG
	#include "CAmWatchdog.h"
#endif


using namespace am;

//commandline options of the daemon itself
TCLAP::ValueArg<std::string> clientPlugin("c","clientPlugin","use clientPlugin full path with .so ending",false," ","string");
TCLAP::ValueArg<std::string> routingPlugin("r","routingPlugin","use routingPlugin full path with .so ending",false," ","string");
TCLAP::SwitchArg enableNoDLTDebug ("V","logDlt","print DLT logs to stdout",false);
TCLAP::SwitchArg currentSettings("i","currentSettings","print current settings and exit",false);
TCLAP::SwitchArg daemonizeRA("d","daemonize","daemonize RoutingAdapter. Better use systemd...",false);

int fd0, fd1, fd2;

/**
 * the out of memory handler
 */
void OutOfMemoryHandler()
{
    logError("No more memory - bye");
    exit(1);
}

/**
 * daemonizes the Routing Adapter
 */
void daemonize()
{
    umask(0);
    std::string dir = "/";

    rlimit rl;
    if (getrlimit(RLIMIT_NOFILE, &rl) < 0)
    {
        logError("can't get file limit ");
    }

    pid_t pid;
    if ((pid = fork()) < 0)
    {
        logError("cannot fork!");
    }
    else if (pid != 0)
    {
        exit(0);
    }

    setsid();

    if (!dir.empty() && chdir(dir.c_str()) < 0)
    {
        logError("couldn't chdir to the new directory");
    }

    if (rl.rlim_max == RLIM_INFINITY)
    {
        rl.rlim_max = 1024;
    }

    for (unsigned int i = 0; i < rl.rlim_max; i++)
    {
        close(i);
    }

    fd0 = open("/dev/null", O_RDONLY);
    fd1 = open("/dev/null", O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR);
    fd2 = open("/dev/null", O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR);

    if (fd0 != STDIN_FILENO || fd1 != STDOUT_FILENO || fd2 != STDERR_FILENO)
    {
        logError("new standard file descriptors were not opened");
    }
}



void printCmdInformation()
{
	printf("\n\n\nCurrent settings:\n\n");
	printf("\tAudioRoutingAdatper Version:\t\t%s\n", ARADVERSION);
#ifndef WITH_DLT
	printf("\tDlt Command Line Output: \t\t%s\n", enableNoDLTDebug.getValue()?"enabled":"not enabled");
#endif
	exit(0);
}

/**
 * the signal handler
 * @param sig
 * @param siginfo
 * @param context
 */
static void signalHandler(int sig, siginfo_t *siginfo, void *context)
{
    (void) sig;
    (void) siginfo;
    (void) context;
    logInfo("signal handler was called, signal",sig);

    switch (sig)
    {
        case SIGINT:
            break;

        /* huch- we are getting killed. Better take the fast but risky way: */
        case SIGQUIT:
            break;

        /* more friendly here assuming systemd wants to stop us, so we can use the mainloop */
        case SIGTERM:
            break;

        /* looks friendly, too, so lets take the long run */
        case SIGHUP:
            break;
        default:
            break;
    }
}

void mainProgram(int argc, char *argv[])
{

	//initialize the commandline parser, and add all neccessary commands
    try
    {
    	TCLAP::CmdLine* cmd(CAmCommandLineSingleton::instanciateOnce("Enjoy the AudioRoutingAdapterDaemon!",' ',ARADVERSION,true));
    	cmd->add(clientPlugin);
    	cmd->add(routingPlugin);
    	cmd->add(currentSettings);
    	cmd->add(daemonizeRA);
#ifndef WITH_DLT
    	cmd->add(enableNoDLTDebug);
#endif
    }
    catch (TCLAP::ArgException &e)  // catch any exceptions
    { std::cerr << "error: " << e.error() << " for arg " << e.argId() << std::endl; }

    //hen and egg. We need to parse a part of the commandline options to get the paths of the controller and the plugins.
    //So we do some little parsing first and the real parsing later so that the plugins can profit from that.
    CAmCommandLineSingleton::instance()->preparse(argc,argv);
	if (daemonizeRA.getValue())
	{
		daemonize();
	}

    CAmDltWrapper::instance(enableNoDLTDebug.getValue())->registerApp("AudioManagerDeamon", "AudioManagerDeamon");

    //Instantiate all classes. Keep in same order !
    CAmSocketHandler iSocketHandler;

    //in this place, the plugins can get the gloval commandlineparser via CAmCommandLineSingleton::instance() and add their options to the commandline
    //this must be done in the constructor.
    //later when the plugins are started, the commandline is already parsed and the objects defined before can be used to get the necessary information

    IAmRoutingClient iRoutingSender(listRoutingPluginDirs);
    CAmCommandSender iCommandSender(listCommandPluginDirs);

    try
    {
    	//parse the commandline options
    	CAmCommandLineSingleton::instance()->reset();
    	CAmCommandLineSingleton::instance()->parse(argc,argv);
    	if (currentSettings.getValue())
    	{
    		printCmdInformation();
    	}
    }
    catch (TCLAP::ArgException &e)  // catch any exceptions
    { std::cerr << "error: " << e.error() << " for arg " << e.argId() << std::endl; }

    logInfo("The RoutingAdapter is started");
    logInfo("The version of the RoutingAdapter", ARADVERSION);

#ifdef WITH_SYSTEMD_WATCHDOG
    CAmWatchdog iWatchdog(&iSocketHandler);
#endif /*WITH_SYSTEMD_WATCHDOG*/

    //startup all the Plugins
    //at this point, commandline arguments can be parsed
    iControlSender.startupController(&iControlReceiver);
    iRoutingSender.startupInterfaces(&iRoutingReceiver);

#ifdef WITH_SYSTEMD_WATCHDOG
    iWatchdog.startWatchdog();
#endif /*WITH_SYSTEMD_WATCHDOG*/

    //start the mainloop here....
    iSocketHandler.start_listenting();
}

int main(int argc, char *argv[])
{
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
    std::set_new_handler(&OutOfMemoryHandler);

    try
    {
        //we do this to catch all exceptions and have a graceful ending just in case
        mainProgram(argc,argv);
    }

    catch (std::exception& exc)
    {
        logError("The RoutingAdapter ended by throwing the exception", exc.what());
        std::cerr<<"The RoutingAdapter ended by throwing an exception "<<exc.what()<<std::endl;
        exit(EXIT_FAILURE);
    }

    close(fd0);
    close(fd1);
    close(fd2);

    //deinit the DLT
    CAmDltWrapper* inst(getWrapper());
    inst->deinit();

    exit(0);

}

