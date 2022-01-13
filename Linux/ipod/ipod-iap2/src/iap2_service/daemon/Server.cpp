/************************************************************************
 * @file: Server.cpp
 *
 * @version: 1.0
 *
 * @description: This module implements main() and signal handlers
 *
 * @component: platform/ipod
 *
 * @author: Dhanasekaran Devarasu, Dhanasekaran.D@in.bosch.com 2017
 *
 * @copyright (c) 2017 Advanced Driver Information Technology.
 * This code is developed by Advanced Driver Information Technology.
 * Copyright of Advanced Driver Information Technology, Bosch, and DENSO.
 * All rights reserved.
 *
 * @see <related items>
 *
 * @history
 *
 ***********************************************************************/

#include <csignal>

#include <adit_logging.h>
#include <iap2_service_init.h>
#include "iap2_dlt_log.h"
#include "Core.h"

namespace adit { namespace iap2service {

void signalHandler(int sig)
{
    if ((sig != SIGINT) && (sig != SIGTERM))
    {
        signal(sig, SIG_DFL);
        kill(getpid(), sig);
    }
    else
    {
        LOG_INFO((iap2, "signalHandler called with id:%d !!!", sig));
        adit::iap2service::Core::instance().stopRunLoop();
    }
}

} }

#define DLT_IAP2_APID "IAPS"

LOG_DECLARE_CONTEXT(iap2)
LOG_DECLARE_CONTEXT(disp)


int main(int argc, const char* argv[])
{
    (void)argc;
    (void)argv;

    // register iAP2_server at logging
    LOG_REGISTER_APP(DLT_IAP2_APID, "iAP2 service daemon");
    LOG_REGISTER_CONTEXT(iap2, "CORE", "Core for iAP2 server application");
    LOG_REGISTER_CONTEXT(disp, "DISP", "Work dispatcher and workitem logs");
    IAP2REGISTERCTXTWITHDLT();

    signal(SIGINT, adit::iap2service::signalHandler);
    signal(SIGTERM, adit::iap2service::signalHandler);

    int32_t rc = adit::iap2service::Core::instance().initialize();

    if(rc == IAP2_OK)
        adit::iap2service::Core::instance().runLoop();

    adit::iap2service::Core::instance().deinitialize();

    IAP2DEREGISTERCTXTWITHDLT();
    LOG_UNREGISTER_CONTEXT(iap2);
    LOG_UNREGISTER_CONTEXT(disp);
    LOG_UNREGISTER_APP();
    return 0;
}
