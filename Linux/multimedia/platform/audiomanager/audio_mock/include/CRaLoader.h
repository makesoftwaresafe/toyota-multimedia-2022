/************************************************************************
 * @file: CRaLoadRouting.h
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

#ifndef LOADROUTING_H_
#define LOADROUTING_H_

#include "IAmRouting.h"
#include <map>


namespace am
{

class CRaRoutingReceiver;

/**
 * Loads the RoutingSend interface plugin only
 */
class CRaLoader
{
public:
    CRaLoader(const char*);
    virtual ~CRaLoader();
    void getRoutingInterface(IAmRoutingSend*& pRoutingSender) const;

private:
    void* openLibrary(const std::string& path);
    IAmRoutingSend* createInstance(const std::string& path, void* libHandle);

private:
    void               *mpLibraryHandle; //!< plugin handle
    IAmRoutingSend     *mpRoutingInterface; //!< pointer to the routingInterface
};

}

#endif /* LOADROUTING_H_ */
