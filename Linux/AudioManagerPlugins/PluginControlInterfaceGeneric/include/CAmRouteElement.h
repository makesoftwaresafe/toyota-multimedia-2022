/******************************************************************************
 * @file: CAmRouteElement.h
 *
 * This file contains the declaration of route element class (member functions
 * and data members) used as data container to store the information related to
 * route of connection as maintained by controller.
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

#ifndef GC_ROUTEELEMENT_H_
#define GC_ROUTEELEMENT_H_

#include "CAmElement.h"
#include "CAmTypes.h"
namespace am {
namespace gc {

class CAmControlReceive;
class CAmSourceElement;
class CAmSinkElement;

class CAmRouteElement : public CAmElement
{
public:
    /**
     * @brief It is the constructor of route element class. Initialize the member
     * variables with default value.It will be invoked during connection request processing
     * @param routingElement: instance of routingElement structure containing source, sink, domain Id and connection format
     *        pControlReceive: control receive class pointer
     * @return none
     */
    CAmRouteElement(const gc_RoutingElement_s& routingElement, CAmControlReceive* pControlReceive);
    /**
     * @brief It is the destructor of route element class.
     * @param none
     * @return none
     */
    virtual ~CAmRouteElement();
    /**
     * @brief It is the used to get the ID of sink of this route.
     * @return ID of sink that belongs to this route
     */
    am_sinkID_t getSinkID(void) const;
    /**
     * @brief It is the used to get the ID of source of this route.
     * @return ID of source that belongs to this route
     */
    am_sourceID_t getSourceID(void) const;
    /**
     * @brief It is the used to get source element involved in this route
     * @return source element
     */
    CAmSourceElement* getSource(void) const;
    /**
     * @brief It is the used to get sink element involved in this route
     * @return sink element
     */
    CAmSinkElement* getSink(void) const;
    /**
     * @brief It is the used to get the connection format
     * @param none
     * @return format of connection
     */
    am_CustomConnectionFormat_t getConnectionFormat(void) const;
    am_Error_e getPriority(int32_t& priority) const;
    int releaseResources(CAmElement *pNotifierElement);
private:
    //store the source/sink/domain ID and connection format
    gc_RoutingElement_s mRoutingElement;

    CAmSinkElement*     mpSink;
    CAmSourceElement*   mpSource;
};

class CAmRouteFactory : public CAmFactory<gc_RoutingElement_s, CAmRouteElement>
{

};

}/* namespace gc */
}/* namespace am */
#endif /* GC_ROUTEELEMENT_H_ */
