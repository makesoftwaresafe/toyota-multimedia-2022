/******************************************************************************
 * @file: CAmRouteActionDisconnect.h
 *
 * This file contains the declaration of route element action disconnect class (member
 * functions and data members) used to implement the logic of disconnection at
 * router level
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

#ifndef GC_ROUTEACTIONDISCONNECT_H_
#define GC_ROUTEACTIONDISCONNECT_H_

#include "CAmActionCommand.h"

namespace am {
namespace gc {
class CAmRouteElement;
class CAmRouteActionDisconnect : public CAmActionCommand
{
public:
    /**
     * @brief It is the constructor of disconnect action at router level.
     * It sets the strings for classification of parameters as expected by this action.
     * Initialize the member variables with default value.
     * @param pRouteElement The pointer to route element
     * @return none
     */
    CAmRouteActionDisconnect(CAmRouteElement* pRouteElement);
    /**
     * @brief It is the destructor of disconnect action at router level.
     * @param none
     * @return none
     */
    virtual ~CAmRouteActionDisconnect();
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
     * @brief This API updates the connection state of router level connection.
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
    //pointer to route class object containing source and sink information between which disconnect need to be done.
    CAmRouteElement* mpRouteElement;

};

}/* namespace gc */
}/* namespace am */
#endif /* GC_ROUTEACTIONDISCONNECT_H_ */
