/*******************************************************************************
 *  \copyright (c) 2016 Advanced Driver Information Technology.
 *                   ADIT is a joint venture company of
 *   Robert Bosch GmbH/Robert Bosch Car Multimedia GmbH and DENSO Corporation
 *
 *  \author: Jens Lorenz, jlorenz@de.adit-jv.com 2016
 *           Mattia Guerra, mguerra@de.adit-jv.com 2016
 *
 *  \copyright The MIT License (MIT)
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"),
 *  to deal in the Software without restriction, including without limitation
 *  the rights to use, copy, modify, merge, publish, distribute, sublicense,
 *  and/or sell copies of the Software, and to permit persons to whom the
 *  Software is furnished to do so, subject to the following conditions:
 *  The above copyright notice and this permission notice shall be included in
 *  all copies or substantial portions of the Software.
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 *  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 *  DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 *  OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR
 *  THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 *  For further information see http://www.genivi.org/.
 ******************************************************************************/

#ifndef CAMDLTLOGGING_H_
#define CAMDLTLOGGING_H_

#include "CAmDltWrapper.h"

#define CONTEXT "AMRA"

namespace am
{

/**
 * \brief DLT logging on AMRA Context
 *
 * This class is a Singleton which allows Plugin Routing Adapter ALSA to DLT log towards its own Context
 */
class CAmDLTLogging
{
public:
    /**
     * Allows to retrieve the Singleton
     */
    static CAmDLTLogging *Instance();
    /**
     * dtor
     */
    virtual ~CAmDLTLogging();
    /**
     * returns a pointer to the DltContext. Such Context is then used when implementing
     * the variadic template logging functions used throughout the plugin
     */
    DltContext *getContextPointer();

private:
    /**
     * private ctor being a singleton
     */
    CAmDLTLogging();

private:
    DltContext mContext;
    static CAmDLTLogging *mCAmDLTLogging;

};

/**
 * logs given values with debuglevel with the plugin context
 * @param value
 * @param ...
 */
template<typename T, typename... TArgs>
void logAmRaDebug(T value, TArgs... args)
{
    CAmDltWrapper* inst(CAmDltWrapper::instance());
    if (!inst->init(DLT_LOG_DEBUG, CAmDLTLogging::Instance()->getContextPointer()))
        return;
    inst->append(value);
    inst->append(args...);
    inst->send();
}

/**
 * logs given values with infolevel with the plugin context
 * @param value
 * @param ...
 */
template<typename T, typename... TArgs>
void logAmRaInfo(T value, TArgs... args)
{
    CAmDltWrapper* inst(CAmDltWrapper::instance());
    if (!inst->init(DLT_LOG_INFO, CAmDLTLogging::Instance()->getContextPointer()))
        return;
    inst->append(value);
    inst->append(args...);
    inst->send();
}

/**
 * logs given values with errorlevel with the plugin context
 * @param value
 * @param ...
 */
template<typename T, typename... TArgs>
void logAmRaError(T value, TArgs... args)
{
    CAmDltWrapper* inst(CAmDltWrapper::instance());

    if (!inst->init(DLT_LOG_ERROR, CAmDLTLogging::Instance()->getContextPointer()))
        return;
    inst->append(value);
    inst->append(args...);
    inst->send();
}

} /* namespace am */
#endif /* CAMDLTLOGGING_H_ */
