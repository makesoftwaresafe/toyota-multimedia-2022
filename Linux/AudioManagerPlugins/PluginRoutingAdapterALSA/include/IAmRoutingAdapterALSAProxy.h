/*******************************************************************************
 *  \copyright (c) 2016 Advanced Driver Information Technology.
 *                   ADIT is a joint venture company of
 *   Robert Bosch GmbH/Robert Bosch Car Multimedia GmbH and DENSO Corporation
 *
 *  \author: Jens Lorenz, jlorenz@de.adit-jv.com 2013-2016
 *           Mattia Guerra, mguerra@de.adit-jv.com 2016
 *           Jayanth MC, Jayanth.mc@in.bosch.com 2013-2014
 *
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


#ifndef ROUTINGADAPTERALSAPROXY_H_
#define ROUTINGADAPTERALSAPROXY_H_

#include "audiomanagertypes.h"
#include "CAmRoutingAdapterALSAProxyInfo.h"

namespace am
{

/**
 * \brief Basic interface shared by proxies exposing elementary streaming operations
 *
 * This allows to implement own streaming solutions in case the existing is not suitable for product
 */
class IAmRoutingAdapterALSAProxy
{
public:
    IAmRoutingAdapterALSAProxy(const ra_Proxy_s & proxy) : mProxy(proxy), libHandle(NULL){}
    virtual ~IAmRoutingAdapterALSAProxy(){}

    virtual am_timeSync_t getDelay() const = 0;
    virtual am_Error_e openStreaming() = 0;
    virtual am_Error_e startStreaming() = 0;
    virtual am_Error_e stopStreaming() = 0;
    virtual am_Error_e closeStreaming() = 0;

    void *getLibHandle()
    {
        return libHandle;
    }
    void setLibHandle(void *ptr)
    {
        libHandle = ptr;
    }
protected:
    const ra_Proxy_s & mProxy;
private:
    void *libHandle;
};

} /* namespace am */

#endif /* ROUTINGADAPTERALSAPROXY_H_ */
