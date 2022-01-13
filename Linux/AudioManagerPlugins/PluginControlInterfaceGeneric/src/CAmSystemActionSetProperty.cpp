/******************************************************************************
 * @file: CAmSystemActionDebug.cpp
 *
 * This file contains the definition of system action register class (member
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

#include "CAmSystemActionSetProperty.h"
#include "CAmLogger.h"

namespace am {
namespace gc {

CAmSystemActionSetProperty::CAmSystemActionSetProperty(CAmSystemElement* pSystem,
                                           CAmControlReceive* pControlReceive) :
                                CAmActionCommand(std::string("CAmSystemSetProperty")),
                                mpSystem(pSystem),
                                mpControlReceive(pControlReceive)

{
    _registerParam(ACTION_PARAM_DEBUG_TYPE, &mTypeParam);
    _registerParam(ACTION_PARAM_DEBUG_VALUE, &mValueParam);
}

CAmSystemActionSetProperty::~CAmSystemActionSetProperty(void)
{
}

int CAmSystemActionSetProperty::_execute(void)
{
    uint16_t PropertyType;
    int16_t PropertyValue;
    int error = E_OK;
    if ((false == mTypeParam.getParam(PropertyType)) || \
        (false == mValueParam.getParam(PropertyValue)))
    {
        LOG_FN_EXIT(__FILENAME__,__func__,"Parameters not set");
        return E_NOT_POSSIBLE;
    }
    else
    {
        error = mpSystem->setSystemProperty(PropertyType, PropertyValue);
    }
    return error;
}

} /* namespace gc */
} /* namespace am */
