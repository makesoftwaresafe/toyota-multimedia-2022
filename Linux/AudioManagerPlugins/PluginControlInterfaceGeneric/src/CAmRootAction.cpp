/******************************************************************************
 * @file: CAmRootAction.cpp
 *
 * This file contains the definition of root action class (member functions
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

#include "CAmRootAction.h"
#include "CAmLogger.h"

namespace am {

namespace gc {

CAmRootAction* CAmRootAction::mRootAction = NULL;
CAmRootAction::CAmRootAction() :
                                CAmActionContainer(std::string("RootAction"))
{
}

CAmRootAction::~CAmRootAction()
{
}

CAmRootAction* CAmRootAction::getInstance(void)
{
    if (mRootAction == NULL)
    {
        mRootAction = new CAmRootAction;
    }
    return mRootAction;
}

int CAmRootAction::_cleanup(void)
{
    /*
     * One of the child action completed call cleanup
     */
    this->setStatus(AS_NOT_STARTED);
    this->setError(0);
    return 0;
}

} /* namespace gc */
} /* namespace am */
