/******************************************************************************
 * @file: CAmTriggerQueue.cpp
 *
 * This file contains the declaration of abstract class used to provide
 * the interface for policy engine side to provide API to framework to pass the
 * trigger and get actions related to trigger from policy engine
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



#include "CAmTriggerQueue.h"


namespace am {

namespace gc {

CAmTriggerQueue* CAmTriggerQueue::mpTriggerQueue = NULL;
CAmTriggerQueue::CAmTriggerQueue()
{
    mlistTrigger.clear();
}


am_Error_e CAmTriggerQueue::queue(gc_Trigger_e triggerType,gc_TriggerElement_s* triggerData)
{
    mlistTrigger.push_back(std::make_pair(triggerType,triggerData));
    return E_OK;
}
am_Error_e CAmTriggerQueue::pushTop(gc_Trigger_e triggerType,gc_TriggerElement_s* triggerData)
{
    mlistTrigger.insert(mlistTrigger.begin(),std::make_pair(triggerType,triggerData));
    return E_OK;
}

gc_TriggerElement_s* CAmTriggerQueue::dequeue(gc_Trigger_e& triggerType )
{
    gc_TriggerElement_s* pTriggerElement = NULL;
    std::pair<gc_Trigger_e,gc_TriggerElement_s*> triggerpair;
    if(!mlistTrigger.empty())
    {
        triggerpair = mlistTrigger.front();
        pTriggerElement = triggerpair.second;
        triggerType = triggerpair.first;
        mlistTrigger.erase(mlistTrigger.begin());
    }
    return pTriggerElement;
}

CAmTriggerQueue* CAmTriggerQueue::getInstance()
{
    if(mpTriggerQueue == NULL)
    {
        mpTriggerQueue = new CAmTriggerQueue();
    }
    return mpTriggerQueue;
}


} /* namespace gc */

} /* namespace am */
