/******************************************************************************
 * @file: CAmClassActionDisconnect.h
 *
 * This file contains the declaration of user connection action disconnect class
 * (member functions and data members) used to implement the logic of disconnect
 * at user level
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

#ifndef GC_CLASSACTIONDISCONNECT_H_
#define GC_CLASSACTIONDISCONNECT_H_

#include "CAmActionContainer.h"

namespace am {
/**
 * @page disconnectaction User Action Disconnect
 *
 * Name: CAmClassActionDisconnect<BR>
 * Responsibility: This action disconnects and removes the main connection. The connection can
 * be identified by source sink name pair. Optionally if only class name is passed as a parameter,
 * then the active connection of the class is disconnected if it exists. If only source or sink
 * name is passed then all the main connections involving passed source or sink are disconnected
 * and removed.<BR>
 *
 * Mandatory parameters: Either or any of the below parameter are mandatory<BR>
 *  - sourcename: The name of the main source.<BR>
 *  - sinkname: The name of the main sink.<BR>
 *  - classname: The name of the class<BR>
 * Optional parameters:<BR>
 *  - timeout: This is the timeout for action execution. If not specified then
 *  DEFAULT_ASYNC_ACTION_TIME is used as timeout.
 */

namespace gc {

class CAmClassElement;
class CAmMainConnectionElement;

class CAmClassActionDisconnect : public CAmActionContainer
{
public:
    /**
     * @brief It is the constructor of disconnect action at user level.
     * It sets the strings for classification of parameters as expected by this action.
     * Initialize the member variables with default value.
     * @param pRoutingManager: pointer to CAmRoutingManager Class object
     * @return none
     */
    CAmClassActionDisconnect(CAmClassElement *pClassElement);
    /**
     * @brief It is the destructor of disconnect action at user level.
     * @param none
     * @return none
     */
    virtual ~CAmClassActionDisconnect();
protected:
    /**
     * @brief This API creates the child action object if class is found whose connection needs to be
     * disconnected. It appends the child to base class for execution.
     * @param none
     * @return E_OK on success
     *         E_NOT_POSSIBLE on error
     */
    int _execute(void);
    /**
     * @brief This API dispose the connection if its child action completes successfully.
     * @param None
     * @return E_OK
     */
    int _update(const int result);
private:
    /**
     * @brief This API is used to set the parameters for its child action.
     * @param pMainConnection: pointer to connection which need to be disconnected
     * @return am_Error_e E_OK on success
     */
    am_Error_e _createDisconnectAction(CAmMainConnectionElement* pMainConnection);


    CAmClassElement *mpClassElement;
    std::vector<CAmMainConnectionElement* > mpListMainConnections;
    // Variables in which policy engine will set the parameters.
    CAmActionParam<std::string > mSourceNameParam;
    CAmActionParam<std::string > mSinkNameParam;
    CAmActionParam<std::vector<am_ConnectionState_e > > mListConnectionStatesParam;
    CAmActionParam<std::vector<std::string > > mListSinkExceptionsParam;
    CAmActionParam<std::vector<std::string > > mlistSourceExceptionsParam;
};

} /* namespace gc */
} /* namespace am */
#endif /* GC_CLASSACTIONDISCONNECT_H_ */
