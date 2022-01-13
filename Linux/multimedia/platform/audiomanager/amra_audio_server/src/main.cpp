/************************************************************************
* @file: main.cpp
*
* @version: 1.1
*
* This file implements the main function of the Routing Adapter KP.
* 
* @component: platform/audiomanager
*
* @author: Nrusingh Dash <ndash@jp.adit-jv.com>
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
#include <sys/stat.h>
#include <unistd.h>
#include <string>
#include <cstdlib>
#include "CRaAMRoutingClient.h"
#include "CRaASClient.h"
#include "CRaConfigManager.h"
#include "CRaRequestManager.h"
#include "dlt/dlt.h"

#include "CAmSocketHandler.h"
#include "CRaHotplugReceiver.h"
#include "CRaHotplugSender.h"
#include "Log.h"

DLT_DECLARE_CONTEXT(raContext);

int main(int argc, char *argv[])
{
	(void) argc;
	(void) argv;
	int ret = -1;
	struct stat info;
	DLT_REGISTER_APP("KPRA","KP Routing Adapter");

	// path of the configuration file is set in environment variable
	// if not set take default path
	// this feature is required for routing adapter tests
	char *path = (char *) getenv("KPRA_CONFIG_PATH");
	if (path == NULL)
	{
		path = (char *) "/etc/routingadapter/KPAudioDomain.xml";
	}

	std::string config_file_name(path);
	ret = stat(config_file_name.c_str(), &info);

	if (0 != ret)
	{
		cout << config_file_name << "is not available" << endl;
		exit(-1);
	}

	CRaConfigManager configManager(config_file_name.c_str());
	DLT_REGISTER_CONTEXT_LL_TS(raContext,"RACT", "KP Routin Adaptor Context", configManager.GetLogThreshold(), DLT_TRACE_STATUS_OFF);
	
    CAmSocketHandler iMainLoopHandler;
	CContextManager contexManager;
	CRaASClient asClient(&configManager, NULL, NULL, &contexManager);
	CRaRequestManager requestManager(&asClient, NULL, &configManager, &contexManager);
	CRaAMRoutingClient amClient(&configManager, &requestManager, &contexManager, configManager.GetInterface(), &iMainLoopHandler );


	requestManager.SetAmClient(&amClient);
	asClient.SetRequestManager(&requestManager);
	asClient.SetAMClient(&amClient);

	// start hot plug interface
	CRaHotplugReceiver	 iHotplugReceiver(&amClient, &configManager, &requestManager, &contexManager);
	CRaHotplugSender iHotplugSender;
	requestManager.SetHotplugSender(&iHotplugSender);
	asClient.SetHotplugSender(&iHotplugSender);
	iHotplugSender.init(&iMainLoopHandler, &iHotplugReceiver);

	LOG_INFO(raContext, DLT_STRING("Initalization Complete"));

	iMainLoopHandler.start_listenting();
	
	DLT_UNREGISTER_CONTEXT(raContext);
	DLT_UNREGISTER_APP();
	return 0;
}
