/******************************************************************************
 * @file: CAmSystemElement.h
 *
 * This file contains the declaration of system element class (member functions
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

#ifndef GC_SYSTEMELEMENT_H_
#define GC_SYSTEMELEMENT_H_

#include "IAmControlCommon.h"
#include "CAmElement.h"
#include "CAmTypes.h"
namespace am {
namespace gc {

#define SYSTEM_ID 1
class CAmSystemElement : public CAmElement
{
public:
    CAmSystemElement(const gc_System_s& systemConfiguration, CAmControlReceive* pControlReceive);
    virtual ~CAmSystemElement();
    am_Error_e getSystemProperty(const am_CustomSystemPropertyType_t type, int16_t& value) const;
    am_Error_e setSystemProperty(const am_CustomSystemPropertyType_t type, const int16_t value);
    int16_t getDebugLevel(void) const;
    bool isNonTopologyRouteAllowed(void) const;
    bool isUnknownElementRegistrationSupported(void) const;
    bool isSystemPropertyReadOnly(void) const;
protected:
    am_Error_e _register(void);
    am_Error_e _unregister(void);

private:
    am_Error_e _findSystemProperty(const std::vector<am_SystemProperty_s>& listSystemProperties,
                                    const uint16_t type , int16_t& value) const;
    CAmControlReceive* mpControlReceive;
    gc_System_s mSystem;
};

class CAmSystemFactory : public CAmFactory<gc_System_s, CAmSystemElement >
{
};

} /* namespace gc */
} /* namespace am */

#endif /* GC_SYSTEMELEMENT_H_ */
