/******************************************************************************
 * @file: CAmGatewayElement.h
 *
 * This file contains the declaration of gateway element class (member functions
 * and data members) used as data container to store the information related to
 * gateway as maintained by controller.
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

#ifndef GC_GATEWAYELEMENT_H_
#define GC_GATEWAYELEMENT_H_

#include "CAmControlReceive.h"
#include "CAmElement.h"

namespace am {
namespace gc {

class CAmGatewayElement : public CAmElement
{
public:
    /**
     * @brief It is the constructor of gateway element class. Initialize the member
     * variables with default value.It will be invoked during gateway element registration.
     * @param gatewayConfiguration: details of gateway element as defined in configuration file
     *        pControlReceive: pointer to CAmControlReceive Class object
     * @return none
     */
    CAmGatewayElement(const gc_Gateway_s& gatewayConfiguration, CAmControlReceive* pControlReceive);
    /**
     * @brief It is the destructor of gateway element class.
     * @param none
     * @return none
     */
    virtual ~CAmGatewayElement();
    /**
     * @brief This API is used to get the main sink element pointer of gateway.
     * @param none
     * @return main sink element pointer
     */
    am_sinkID_t getSinkID(void);
    /**
     * @brief This API is used to get the main source element pointer of gateway.
     * @param none
     * @return main source element pointer
     */
    am_sourceID_t getSourceID(void);

    am_domainID_t getSourceDomainID(void);
    am_domainID_t getSinkDomainID(void);

protected:
    am_Error_e _register(void);
    am_Error_e _unregister(void);

private:
    //variable to store configuration of gateway as define in configuration file
    gc_Gateway_s mGateway;
    CAmControlReceive* mpControlReceive;
};

class CAmGatewayFactory : public CAmFactory<gc_Gateway_s, CAmGatewayElement >
{
};

} /* namespace gc */
} /* namespace am */
#endif /* GC_GATEWAYELEMENT_H_ */
