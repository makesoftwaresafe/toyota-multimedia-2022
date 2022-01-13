/******************************************************************************
 * @file: CAmEventSubject.cpp
 *
 * This file contains the definition of event subject class (member functions
 * and data members) used to register/unregister the observer and invoking
 * observer update function
 *
 * @component: AudioManager Generic Controller
 *
 * @author: Toshiaki Isogai <tisogai@jp.adit-jv.com>
 *          Kapildev Patel  <kpatel@jp.adit-jv.com>
 *          Prashant Jain   <pjain@jp.adit-jv.com>
 *
 * @copyright (c) 2015 Advanced Driver Information Technology.
 * This code is developed by Advanced Driver Information Technology.
 * Copyright of Advanced Driver Information Technology, Bosch, and DENSO.
 * All rights reserved.
 *
 *****************************************************************************/

#include "CAmEventSubject.h"
#include "CAmLogger.h"

namespace am {
namespace gc {

CAmEventSubject::CAmEventSubject()
{
}

CAmEventSubject::~CAmEventSubject()
{
}

int CAmEventSubject::registerObserver(IAmEventObserver* pObserver)
{
    if (pObserver != NULL)
    {
        mListObservers.push_back(pObserver);
    }
    return 0;
}

int CAmEventSubject::unregisterObserver(IAmEventObserver* pObserver)
{
    std::vector<IAmEventObserver* >::iterator itListObservers;
    itListObservers = mListObservers.begin();
    for (; itListObservers != mListObservers.end(); ++itListObservers)
    {
        if ((*itListObservers) == pObserver)
        {
            mListObservers.erase(itListObservers);
            break;
        }
    }
    return 0;
}

int CAmEventSubject::notify(const int result)
{
    std::vector<IAmEventObserver* >::iterator itListObservers;
    itListObservers = mListObservers.begin();
    if (itListObservers != mListObservers.end())
    {
        (*itListObservers)->update(result);
    }

    return 0;
}

} /* namespace gc */
} /* namespace am */
