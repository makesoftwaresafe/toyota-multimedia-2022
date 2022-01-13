/******************************************************************************
 * @file: CAmSystemActionDebug.h
 *
 * This file contains the declaration of system action register class (member
 * functions and data members) used to implement the logic of registering the
 * static source/sink/gateway as defined in configuration.
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

#ifndef GC_USERACTIONDEBUG_H_
#define GC_USERACTIONDEBUG_H_

#include "CAmSystemElement.h"
#include "CAmActionCommand.h"
namespace am {
namespace gc {

class CAmSystemActionSetProperty : public CAmActionCommand
{
public:
    /**
     * @brief It is the constructor of register action at system level.
     * It sets the strings for classification of parameters as expected by this action.
     * Initialize the member variables with default value.
     * @param pRoutingManager: pointer to CAmRoutingManager Class object
     * @return none
     */
    CAmSystemActionSetProperty(CAmSystemElement* pSystem, CAmControlReceive* pControlReceive);
    /**
     * @brief It is the destructor of register action at system level.
     * @param none
     * @return none
     */
    virtual ~CAmSystemActionSetProperty(void);
protected:
    /**
     * @brief This API based on parameter set invokes the routing manager API to register the element
     * @param none
     * @return E_OK on success
     *         E_NOT_POSSIBLE on error
     */
    int _execute(void);
private:
    // Set in constructor. It is used to invoke the function to register the element.
    CAmControlReceive* mpControlReceive;
    CAmSystemElement* mpSystem;
    // Variables in which parent action will set the parameters.
    CAmActionParam<uint16_t > mTypeParam;
    CAmActionParam<int16_t > mValueParam;
};

} /* namespace gc */
} /* namespace am */
#endif /* GC_USERACTIONDEBUG_H_ */
