/******************************************************************************
 * @file: CAmMainConnectionActionSuspend.h
 *
 * This file contains the declaration of main connection action suspend class
 * (member functions and data members) used to implement the logic of suspend
 * at main connection level
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

#ifndef GC_MAINCONNECTIONACTIONSUSPEND_H_
#define GC_MAINCONNECTIONACTIONSUSPEND_H_

#include "CAmActionContainer.h"
#include "CAmMainConnectionElement.h"

namespace am {
namespace gc {

class CAmMainConnectionActionSuspend : public CAmActionContainer
{
public:
    /**
     * @brief It is the constructor of suspend action at main connection level.
     * It sets the strings for classification of parameters as expected by this action.
     * Initialize the member variables with default value.
     * @param pMainConnection: pointer to MainConnection Element
     * @return none
     */
    CAmMainConnectionActionSuspend(CAmMainConnectionElement* pMainConnection);
    /**
     * @brief It is the destructor of suspend action at main connection level.
     * @param none
     * @return none
     */
    virtual ~CAmMainConnectionActionSuspend();

protected:
    /**
     * @brief This API creates the child action object if connection is found which needs to be
     * suspended. It appends the child to base class for execution.
     * @param none
     * @return E_OK on success
     *         E_NOT_POSSIBLE on error
     */
    int _execute(void);
    /**
     * @brief This API updates the connection state of connection.
     * @param result: status of child action execution
     * @return E_OK
     */
    int _update(const int result);
private:
    am_Error_e _createListActionsRouteConnect( std::vector<CAmRouteElement*>& listRouteElements);
    am_Error_e _createActionRouteConnect( CAmRouteElement* routeElement);

    am_Error_e _createListActionsSetSourceState(std::vector<CAmRouteElement*>& listRouteElements,
                                                const am_SourceState_e requestedSourceState);
    am_Error_e _createActionSetSourceState(CAmSourceElement* pSource,
                                           const am_SourceState_e requestedSourceState);
    void _setConnectionStateChangeTrigger(void);
    bool _sharedConnectionCheck(const CAmRouteElement&);
    CAmMainConnectionElement* mpMainConnection;
    CAmActionParam<gc_SetSourceStateDirection_e> mSetSourceStateDirectionParam;
};

} /* namespace gc */
} /* namespace am */
#endif /* GC_MAINCONNECTIONACTIONSUSPEND_H_ */
