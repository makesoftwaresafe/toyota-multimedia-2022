/************************************************************************
 * @file: CRaLoadRouting.cpp
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

#include "CRaLoader.h"
#include <utility>
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dlfcn.h>
#include <libgen.h>
#include <cassert>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include "IAmRouting.h"
#include "CAmDltWrapper.h"

using namespace std;

namespace am
{

#define REQUIRED_RAIF_VERSION_MAJOR 3
#define REQUIRED_RAIF_VERSION_MINOR 0

CRaLoader::CRaLoader(const char* path) :
        mpLibraryHandle(NULL), mpRoutingInterface(NULL)
{
    void* libHandle = openLibrary(string(path));

    IAmRoutingSend* router = createInstance(path, libHandle);
    if (!router)
    {
        logError("RoutingSender::RoutingSender RoutingPlugin initialization failed. Entry Function not callable");
        dlclose(libHandle);
        throw runtime_error("RoutingSender::RoutingSender RoutingPlugin initialization failed. Entry Function not callable!");
    }

	//check libversion
	string version;
	router->getInterfaceVersion(version);
	uint16_t minorVersion, majorVersion;
	istringstream(version.substr(0, 1)) >> majorVersion;
	istringstream(version.substr(2, 1)) >> minorVersion;
	if ( (majorVersion  < REQUIRED_RAIF_VERSION_MAJOR) ||
	    ((majorVersion == REQUIRED_RAIF_VERSION_MAJOR) && (minorVersion > REQUIRED_RAIF_VERSION_MINOR)))
	{
		logInfo("RoutingSender::RoutingSender Version of Interface to old!");
		dlclose(libHandle);
		throw runtime_error("RoutingSender::RoutingSender Version of Interface to old!");
	}

	mpRoutingInterface = router;
	mpLibraryHandle = libHandle;
}

CRaLoader::~CRaLoader()
{
    if (mpLibraryHandle)
    {
        dlclose(mpLibraryHandle);
    }
}

void CRaLoader::getRoutingInterface(IAmRoutingSend*& pRoutingRouting) const
{
    pRoutingRouting = mpRoutingInterface;
}

void* CRaLoader::openLibrary(const std::string& path)
{
    logInfo("CRaLoader::loadLibrary Trying to load library with name: ", path);

    void* soLibrary = dlopen(path.c_str(), RTLD_LAZY |RTLD_DEEPBIND);
    const char* dlopen_error = dlerror();
    if (!soLibrary || dlopen_error)
    {
        logError("CRaLoader::openLibrary", dlopen_error);
        throw runtime_error(string("RoutingSender::openLibrary ") + string(dlopen_error));
        return NULL;
    }
    return soLibrary;
}

IAmRoutingSend* CRaLoader::createInstance(const std::string& path, void* libHandle)
{
    // Create function name
    string entryFunction = path.substr(path.find_last_of("/") + 4);
    entryFunction.resize(entryFunction.length() - 3);
    entryFunction.append("Factory");

    // get entry point from shared lib
    typedef IAmRoutingSend* (*CFunc)(void);
    union {
        void* dlsym;
        CFunc func;
    } ptr_cast;

    ptr_cast.dlsym = dlsym(libHandle, entryFunction.c_str());
    const char* dlsym_error = dlerror();
    if (!ptr_cast.func || dlsym_error)
    {
        logError("CRaLoader::createInstance Unable to load function", entryFunction, "Error:", dlsym_error);
        throw runtime_error(string("CRaLoader::createInstance Unable to load function ") + entryFunction + string(" Error: ") + string(dlsym_error));
        return NULL;
    }

    return ptr_cast.func();
}

}
