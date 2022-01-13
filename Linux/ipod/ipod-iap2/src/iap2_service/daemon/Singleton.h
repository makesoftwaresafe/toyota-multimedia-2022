/**
* \file: Singleton.h
*
* \version: $Id:$
*
* \release: $Name:$
*
* <brief description>.
* <detailed description>
* \component: Utility
*
* \author: J. Harder / ADIT/SW1 / jharder@de.adit-jv.com
*
* \copyright (c) 2014 Advanced Driver Information Technology.
* This code is developed by Advanced Driver Information Technology.
* Copyright of Advanced Driver Information Technology, Bosch, and DENSO.
* All rights reserved.
*
* \see <related items>
*
* \history
*
***********************************************************************/

#ifndef SINGLETON_H_
#define SINGLETON_H_

namespace adit { namespace iap2service {

template<class Type> class Singleton
{
private:
    Singleton(const Singleton& inObj) { }
    Singleton& operator = (const Singleton& inObj) { return *this; }

public:
    static Type& instance()
    {
#if !defined(__GNUC__) || (__GNUC__ <= 3)
#error // not supported, TODO add proper locking
#endif
        static Type _instance; // this is guaranteed to be thread-safe
        return _instance;
    }

protected:
    Singleton() {};
    virtual ~Singleton() {};
};

} } //namespace adit { namespace iap2service {

#endif /* SINGLETON_H_ */
