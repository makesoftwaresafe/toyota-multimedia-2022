/******************************************************************************
 * @file: CAmPersistenceWrapper.h
 *
 * This file contains the declaration of Persistence Wrapper class used to
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
#ifndef GC_PERSISTENCE_WRAPPER_H_
#define GC_PERSISTENCE_WRAPPER_H_

#include "IAmPersistence.h"
#include "CAmCommandLineSingleton.h"

namespace am {
namespace gc {
class CAmPersistenceWrapper
{
public:
    static IAmPersistence* getInstance();
    static void freeInstance();
    static void addCommandLineArgument();
    virtual ~CAmPersistenceWrapper();

private:
    CAmPersistenceWrapper(void);            //!< is private because of singleton pattern
    static IAmPersistence* mpPersistence;     //!< pointer to the persistence instance
    static TCLAP::ValueArg<std::string> mPersistenceImplementation;
    static void* mLibHandle;
};

}
}
#endif /* PERSISTENCEWRAPPER_H_ */
