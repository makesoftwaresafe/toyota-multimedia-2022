/******************************************************************************
 * @file: CAmSourceActionSetState.h
 *
 * This file contains the declaration of router action set source state class
 * (member functions and data members) used to implement the logic of setting
 * the source state at router level
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

#ifndef GC_SOURCEACTIONSETSTATE_H_
#define GC_SOURCEACTIONSETSTATE_H_

#include "CAmActionCommand.h"

namespace am {
namespace gc {
class CAmSourceElement;
class CAmSourceActionSetState : public CAmActionCommand
{
public:
    /**
     * @brief It is the constructor of set source state action at router level.
     * It sets the strings for classification of parameters as expected by this action.
     * Initialize the member variables with default value.
     * @param pSourcelement: pointer to CAmSourceElement Class object
     * @return none
     */
    CAmSourceActionSetState(CAmSourceElement* pSourceElement);
    /**
     * @brief It is the destructor of set source state action at router level.
     * @param none
     * @return none
     */
    virtual ~CAmSourceActionSetState();
protected:
    /**
     * @brief This API invokes the control receive class function to send request to AM. It
     * register the observer to receive the acknowledgment of action completion.
     * @param none
     * @return E_WAIT_FOR_CHILD_COMPLETION on waiting for async action to be completed
     *         E_NOT_POSSIBLE on error
     *         E_OK on success
     */
    int _execute(void);
    /**
     * @brief This API unregister the observer which was set in _execute function.
     * @param result: status of child action execution
     * @return E_OK
     */
    int _update(const int result);
    /**
     * @brief In case of failure this API performs the undo operation if parent has requested for undo.
     * @param none
     * @return E_WAIT_FOR_CHILD_COMPLETION on waiting for async action to be completed
     *         E_NOT_POSSIBLE on error
     *         E_OK on success
     */
    int _undo(void);
    void _timeout(void);
private:
    // pointer to source object whose state need to be changed
    CAmSourceElement* mpSourceElement;
    // new state of source expected at the end of this action
    am_SourceState_e mState;
    // old state in which source was before completion of this action
    am_SourceState_e mOldState;
    CAmActionParam<am_SourceState_e > mSourceStateParam;

};

} /* namespace gc */
} /* namespace am */
#endif /* GC_SOURCEACTIONSETSTATE_H_ */
