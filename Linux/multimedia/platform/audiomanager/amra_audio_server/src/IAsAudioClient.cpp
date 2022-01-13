/************************************************************************
* @file: IAsAudioClient.cpp
*
* @version: 1.1
*
* IAsAudioClient class combines the interfaces of IAsAudioRouting, IAsAudioProcessing and IasAudioSetup
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
#include "IAsAudioClient.h"
#include <iostream>
#include "Log.h"
DLT_IMPORT_CONTEXT(raContext);

using namespace std;

IAsAudioClient::IAsAudioClient()
{
	LOG_FN_ENTRY(raContext);
	LOG_FN_EXIT(raContext);
}

IAsAudioClient::~IAsAudioClient()
{
	LOG_FN_ENTRY(raContext);
	LOG_FN_EXIT(raContext);
}

