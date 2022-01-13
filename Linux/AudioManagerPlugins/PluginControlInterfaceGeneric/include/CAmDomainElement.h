/******************************************************************************
 * @file: CAmDomainElement.h
 *
 * This file contains the declaration of domain Element class (member functions
 * and data members) used as data container to store the information related to
 * class as maintained by controller.
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

#ifndef GC_DOMAINELEMENT_H_
#define GC_DOMAINELEMENT_H_

#include "CAmElement.h"
#include "CAmTypes.h"

namespace am {
namespace gc {

class CAmDomainElement : public CAmElement
{

public:
    /**
     * @brief It is the constructor of domain element class. Initialize the member
     * variables with default value. It is invoked whenever new class is created in controller.
     * @param name: name of the class element
     *        type: type of the class element
     *        pControlReceive: pointer to control receive class instance
     * @return none
     */
    CAmDomainElement(const gc_Domain_s& domain, CAmControlReceive* pControlReceive);
    /**
     * @brief It is the destructor of class element class.
     * @param none
     * @return none
     */
    virtual ~CAmDomainElement();
    virtual am_Error_e setState(const am_DomainState_e state);
    am_Error_e getState(int& state) const;
protected:

    virtual am_Error_e _register(void);
    virtual am_Error_e _unregister(void);

private:
    CAmControlReceive* mpControlReceive;
    gc_Domain_s mDomain;
};

class CAmDomainFactory : public CAmFactory<gc_Domain_s, CAmDomainElement >
{
};
} /* namespace gc */
} /* namespace am */
#endif /* GC_DOMAINELEMENT_H_ */
