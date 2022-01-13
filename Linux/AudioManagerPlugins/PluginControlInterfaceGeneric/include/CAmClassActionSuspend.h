/******************************************************************************
 * @file: CAmClassActionSuspend.h
 *
 * This file contains the declaration of user action suspend class
 * (member functions and data members) used to implement the logic of suspending
 * the connected connection at user level
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

#ifndef GC_CLASSACTIONSUSPEND_H_
#define GC_CLASSACTIONSUSPEND_H_

#include "CAmActionContainer.h"

namespace am {
/**
 * @page suspendaction User Action Suspend
 *
 * Name: CAmClassActionSuspend<BR>
 * Responsibility: This action suspends the main connection.The suspending of the main connection
 * means changing the source state to SS_OFF. The route still remains established.<BR>
 *
 * Mandatory parameters:<BR>
 * - classname: The name of the class whose active connection is to be suspended.<BR>
 * Optional parameters:<BR>
 * - timeout: This is the timeout for action execution. If not specified then
 *  DEFAULT_ASYNC_ACTION_TIME is used as timeout.<BR>
 *
 *
 */
namespace gc {
class CAmClassElement;
class CAmMainConnectionElement;

#define DEFAULT_CONNECT_ORDER       (O_NEWEST)

class CAmClassActionSuspend : public CAmActionContainer
{
public:
    /**
     * @brief It is the constructor of suspend connection action at user level.
     * It sets the strings for classification of parameters as expected by this action.
     * Initialize the member variables with default value.
     * @param pRoutingManager: pointer to CAmRoutingManager Class object
     * @return none
     */
    CAmClassActionSuspend(CAmClassElement *pClassElement);
    /**
     * @brief It is the destructor of suspend action at user level.
     * @param none
     * @return none
     */
    virtual ~CAmClassActionSuspend(void);
protected:
    /**
     * @brief This API creates the child action object if connection is found which need
     * to be suspended. It appends the child to base class for execution.
     * @param none
     * @return E_OK on success
     *         E_NOT_POSSIBLE on error
     */
    int _execute(void);
    int _update(const int result);
private:
    IAmActionCommand* _createActionSetLimitState(CAmMainConnectionElement* pMainConnection);
    CAmClassElement *mpClassElement;
    CAmActionParam<std::string > mSourceNameParam;
    CAmActionParam<std::string > mSinkNameParam;
    CAmActionParam<gc_Order_e > mOrderParam;
};

} /* namespace gc */
} /* namespace am */
#endif /* GC_CLASSACTIONSUSPEND_H_ */
