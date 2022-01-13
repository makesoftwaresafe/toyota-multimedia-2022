/******************************************************************************
 * @file: CAmClassActionInterrupt.h
 *
 * This file contains the declaration of user action interrupt class (member functions
 * and data members) used to implement the logic of pushing (disconnect) the connection
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

#ifndef GC_CLASSACTIONINTERRUPT_H_
#define GC_CLASSACTIONINTERRUPT_H_

#include "CAmActionContainer.h"

namespace am {
/**
 * @page interruptaction User Action Interrupt
 *
 * Name: CAmClassActionInterrupt<BR>
 * Responsibility: This action pushes a main connection. The meaning of a pushing a connection
 * is connection is disconnected but not removed from the database. The controller remembers the
 * connection and later at some point of time in future a connect action execution will re-establish
 * the connection again. If class name is provided as a parameter then the active main connection of that
 * class would be pushed if exists. If source and sink name pair is provided then the main
 * connection involving source sink pair is pushed in the queue. If the connection exists then
 * the main connection is disconnected, but if the connection does not exists the the main
 * connection is added in the audiomanager database in the disconnected state.<BR>
 *
 * Mandatory parameters: Either or any of the below parameter are mandatory<BR>
 *  - classname: The name of the class<BR>
 *  - sourcename: The name of the source. This parameter should be provided with the sinkname<BR>
 *  - sinkname: The name of the sink. This parameter should be provided with the sourcename<BR>
 * Optional parameters:<BR>
 *  - timeout: This is the timeout for action execution. If not specified then
 *  DEFAULT_ASYNC_ACTION_TIME is used as timeout.
 *
 */
namespace gc {

class CAmClassElement;
class CAmMainConnectionElement;

class CAmClassActionInterrupt : public CAmActionContainer
{
public:
    /**
     * @brief It is the constructor of push connection action at user level.
     * It sets the strings for classification of parameters as expected by this action.
     * Initialize the member variables with default value.
     * @param pRoutingManager: pointer to CAmRoutingManager Class object
     * @return none
     */
    CAmClassActionInterrupt(CAmClassElement *pClassElement);
    /**
     * @brief It is the destructor of push action at user level.
     * @param none
     * @return none
     */
    virtual ~CAmClassActionInterrupt();
protected:
    /**
     * @brief This API creates the child action object if connection is found which need
     * to be pushed (disconnected). It appends the child to base class for execution.
     * @param none
     * @return E_OK on success
     *         E_NOT_POSSIBLE on error
     */
    int _execute(void);
private:
    IAmActionCommand* _createActionSetLimitState(CAmMainConnectionElement* pMainConnection);
    CAmClassElement *mpClassElement;
    // Variables in which policy engine will set the parameters.
    CAmActionParam<std::string > mSourceNameParam;
    CAmActionParam<std::string > mSinkNameParam;
};

} /* namespace gc */
} /* namespace am */
#endif /* GC_CLASSACTIONINTERRUPT_H_ */
