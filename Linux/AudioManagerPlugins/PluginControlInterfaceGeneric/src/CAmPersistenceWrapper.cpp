/******************************************************************************
 * @file: CAmPersistenceWrapper.cpp
 *
 * This file contains the definition of Persistence Wrapper class used to
 * provide interface for creating the single instance for specific persistence
 * implementation for e.g GENIVI persistence or RFS or any other
 * proprietary persistence
 *
 * @component: AudioManager Generic Controller
 *
 * @author: Naohiro Nishiguchi<nnishiguchi@jp.adit-jv.com>
 *          Kapildev Patel, Nilkanth Ahirrao <kpatel@jp.adit-jv.com>
 *
 * @copyright (c) 2015 Advanced Driver Information Technology.
 * This code is developed by Advanced Driver Information Technology.
 * Copyright of Advanced Driver Information Technology, Bosch, and DENSO.
 * All rights reserved.
 *
 *****************************************************************************/
#include "CAmPersistenceWrapper.h"
#include "CAmPersistenceDefault.h"
#include "TAmPluginTemplate.h"
#include "CAmLogger.h"
namespace am {
namespace gc {

IAmPersistence* CAmPersistenceWrapper::mpPersistence = NULL;
TCLAP::ValueArg<std::string> CAmPersistenceWrapper::mPersistenceImplementation(
            "Z", "peristenceLibraryPath", //TODO change this later
            "This switch allows the user to choose his own"\
            "IAmPersistence Implementation.",
            false, "", "string");
void* CAmPersistenceWrapper::mLibHandle = NULL;

CAmPersistenceWrapper::CAmPersistenceWrapper(void)
{
}

void CAmPersistenceWrapper::addCommandLineArgument()
{
    CAmCommandLineSingleton::instance()->add(mPersistenceImplementation);
}

IAmPersistence* CAmPersistenceWrapper::getInstance()
{
    if (mpPersistence != NULL)
    {
        return mpPersistence;
    }
    else
    {
        std::string persistenceLibPath = mPersistenceImplementation.getValue();
        if(persistenceLibPath.size() > 0)
        {
            IAmPersistence *(*createFunc)();
            createFunc = getCreateFunction<IAmPersistence*()>(persistenceLibPath.c_str(), mLibHandle);
            if(createFunc != NULL)
            {
                mpPersistence = createFunc();
            }
            else
            {
                LOG_FN_ERROR(__FILENAME__,__func__,"failed to load the persistence library, default would be used");
            }
        }
        if(mpPersistence == NULL)
        {
            mpPersistence = new CAmPersistenceDefault();
        }
        return mpPersistence;
    } 
}

void CAmPersistenceWrapper::freeInstance()
{
    if(mLibHandle != NULL)
    {
        void (*destroyFunc)(IAmPersistence*);
        destroyFunc = getDestroyFunction<void(IAmPersistence*)>(mPersistenceImplementation.getValue(),mLibHandle);
        if(destroyFunc != NULL)
        {
            destroyFunc(mpPersistence);
        }
        dlclose(mLibHandle);
        mLibHandle = NULL;
    }
    else
    {
        if(mpPersistence != NULL)
        {
            delete mpPersistence;
            mpPersistence = NULL;
        }
    }
}

CAmPersistenceWrapper::~CAmPersistenceWrapper()
{
}

} /* namespace gc */
} /* namespace am */

