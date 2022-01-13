/******************************************************************************
 * @file: CAmRootAction.h
 *
 * This file contains the declaration of root action class (member functions
 * and data members) to which all the actions are attached for execution
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

#ifndef GC_ROOTACTION_H_
#define GC_ROOTACTION_H_

#include "CAmActionContainer.h"

namespace am {
namespace gc {

/**
 * This is the singleton class representing the root node of the action tree. It is required to serialize the
 * multiple requests from the Control Send Interface. All the actions are added to this singleton node.
 */
class CAmRootAction : public CAmActionContainer
{

public:
    /**
     * @brief Static function to get the instance of the root action.
     */
    static CAmRootAction* getInstance(void);

    virtual ~CAmRootAction();
protected:
    /**
     * @brief The clean up for this action is over-ridden by this function.
     *
     * @return 0 on success, non zero otherwise.
     */
    int _cleanup();
private:
    CAmRootAction();

    static CAmRootAction* mRootAction;
};

} /* namespace gc */
} /* namespace am */
#endif /* GC_ROOTACTION_H_ */
