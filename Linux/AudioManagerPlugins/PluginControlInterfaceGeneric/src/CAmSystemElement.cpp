/******************************************************************************
 * @file: CAmSystemElement.cpp
 *
 * This file contains the definition of system element class (member functions
 * and data members) used as data container to store the information related to
 * system as maintained by controller.
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

#include "CAmSystemElement.h"
#include "CAmControlReceive.h"
#include "CAmLogger.h"

namespace am {
namespace gc {

CAmSystemElement::CAmSystemElement(const gc_System_s& systemData,
                                   CAmControlReceive* pControlReceive) :
                                mSystem(systemData),
                                mpControlReceive(pControlReceive),
                                CAmElement(systemData.name, pControlReceive)
{
    setType (ET_SYSTEM);
    setID (SYSTEM_ID);

    am_SystemProperty_s systemProperty;

    systemProperty.type = SYP_GLOBAL_LOG_THRESHOLD;
    systemProperty.value = LOG_DEBUG_DEFAULT_VALUE;
    int16_t value;
    if(E_OK != _findSystemProperty(systemData.listSystemProperties,SYP_GLOBAL_LOG_THRESHOLD,value))
    {
        mSystem.listSystemProperties.push_back(systemProperty);
    }

    systemProperty.type = SYP_REGISTRATION_ALLOW_UNKNOWN_ELEMENT;
    systemProperty.value = 0;
    if(E_OK != _findSystemProperty(systemData.listSystemProperties,SYP_REGISTRATION_ALLOW_UNKNOWN_ELEMENT,value))
    {
        mSystem.listSystemProperties.push_back(systemProperty);
    }

    systemProperty.type = SYP_CONNECTION_ALLOW_ONLY_TOPOLOGY_ROUTES;
    systemProperty.value = 1;
    if(E_OK != _findSystemProperty(systemData.listSystemProperties,SYP_CONNECTION_ALLOW_ONLY_TOPOLOGY_ROUTES,value))
    {
        mSystem.listSystemProperties.push_back(systemProperty);

    }

    systemProperty.type = SYP_REGISTRATION_DOMAIN_TIMEOUT;
    systemProperty.value = 10000;
    if (E_OK != _findSystemProperty(systemData.listSystemProperties,SYP_REGISTRATION_DOMAIN_TIMEOUT,value))
    {
        mSystem.listSystemProperties.push_back(systemProperty);
    }

}

CAmSystemElement::~CAmSystemElement()
{
}

am_Error_e CAmSystemElement::_findSystemProperty(const std::vector<am_SystemProperty_s>& listSystemProperties,
                                const uint16_t type , int16_t& value) const
{
    std::vector<am_SystemProperty_s >::const_iterator itListSystemProperties;
    for (itListSystemProperties = listSystemProperties.begin();
                    itListSystemProperties != listSystemProperties.end();
                    ++itListSystemProperties)
    {
        if (itListSystemProperties->type == type)
        {
            value = itListSystemProperties->value;
            return E_OK;
        }
    }
    return E_NOT_POSSIBLE;
}

am_Error_e CAmSystemElement::_register(void)
{
    if (mSystem.listSystemProperties.size() > 0)
    {
        mpControlReceive->enterSystemPropertiesListDB(mSystem.listSystemProperties);
    }
    int16_t logThresoldLevel = LOG_DEBUG_DEFAULT_VALUE;
    if(E_OK != _findSystemProperty(mSystem.listSystemProperties,SYP_GLOBAL_LOG_THRESHOLD,logThresoldLevel))
    {
        LOG_FN_INFO(__FILENAME__,__func__,"Setting default threshold value, failed to get from configuration");
    }
    LOG_FN_CHANGE_LEVEL(logThresoldLevel);
    return E_OK;
}

am_Error_e CAmSystemElement::_unregister(void)
{
    return E_OK;
}

am_Error_e CAmSystemElement::getSystemProperty(const am_CustomSystemPropertyType_t type,
                                               int16_t& value) const
{
    am_Error_e result = E_NOT_POSSIBLE;
    std::vector < am_SystemProperty_s > listSystemProperties;
    std::vector<am_SystemProperty_s >::iterator itListSystemProperties;
    result = mpControlReceive->getListSystemProperties(listSystemProperties);
    if (E_OK == result)
    {
        result = _findSystemProperty(listSystemProperties,type,value);
    }
    return result;
}

am_Error_e CAmSystemElement::setSystemProperty(const am_CustomSystemPropertyType_t type,
                                               const int16_t value)
{
    am_Error_e result = E_NOT_POSSIBLE;
    am_SystemProperty_s systemProperty;
    systemProperty.type = type;
    systemProperty.value = value;
    result = mpControlReceive->changeSystemPropertyDB(systemProperty);
    if(SYP_GLOBAL_LOG_THRESHOLD == systemProperty.type)
    {
        LOG_FN_CHANGE_LEVEL(systemProperty.value);
    }
    return result;
}

int16_t CAmSystemElement::getDebugLevel(void) const
{
    int16_t value = -1;
    getSystemProperty(SYP_GLOBAL_LOG_THRESHOLD, value);
    return value;
}

bool CAmSystemElement::isUnknownElementRegistrationSupported(void) const
{
    int16_t value = -1;
    getSystemProperty(SYP_REGISTRATION_ALLOW_UNKNOWN_ELEMENT, value);
    return ((value==0)?false:true);
}

bool CAmSystemElement::isSystemPropertyReadOnly() const
{
    return mSystem.readOnly;
}

bool CAmSystemElement::isNonTopologyRouteAllowed(void) const
{
    int16_t value;
    getSystemProperty(SYP_CONNECTION_ALLOW_ONLY_TOPOLOGY_ROUTES, value);
    return ((value==0)?true:false);
}

} /* namespace gc */
} /* namespace am */
