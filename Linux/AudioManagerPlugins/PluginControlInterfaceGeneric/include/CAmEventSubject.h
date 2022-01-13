/******************************************************************************
 * @file: CAmEventSubject.h
 *
 * This file contains the declaration of event subject class (member functions
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

#ifndef GC_EVENTSUBJECT_H_
#define GC_EVENTSUBJECT_H_

#include <vector>
#include "IAmEventObserver.h"

namespace am {
namespace gc {

/**
 * This class implements the event subject. The subject notifies the registered observer about any
 * change in state.
 */
class CAmEventSubject
{

public:

    CAmEventSubject();
    ~CAmEventSubject();
    /**
     * @brief This function unregistred the previously registered observer.
     * @param pObserver The pointer to the observer.
     *
     * @return 0 on success and non zero on error.
     */
    int unregisterObserver(IAmEventObserver* pObserver);
    /**
     * @brief This function registers a new observer.
     *
     * @param pObserver This fnuction registers a new observer.
     *
     * @return 0 on success and non zero on error.
     */
    int registerObserver(IAmEventObserver* pObserver);
protected:
    /**
     * @brief On any state change, this function notifies all the registered observer.
     *
     * @param result The result
     * @return 0 on success and non zero on error.
     */
    int notify(const int result);
private:
    // List of the registred observer
    std::vector<IAmEventObserver* > mListObservers;

};

} /* namespace gc */
} /* namespace am */
#endif /* GC_EVENTSUBJECT_H_ */
