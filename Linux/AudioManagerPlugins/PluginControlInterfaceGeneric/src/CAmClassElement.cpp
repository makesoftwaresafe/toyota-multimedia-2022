/******************************************************************************
 * @file: CAmClassElement.cpp
 *
 * This file contains the definition of class element (member functions
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

#include <algorithm>
#include <sstream>
#include "CAmClassElement.h"
#include "CAmControlReceive.h"
#include "CAmMainConnectionElement.h"
#include "CAmGatewayElement.h"
#include "CAmLogger.h"
#include "CAmSinkElement.h"
#include "CAmSourceElement.h"
#include "CAmSystemElement.h"
#include "CAmRootAction.h"
#include "CAmMainConnectionActionDisconnect.h"
#include "CAmClassActionDisconnect.h"
#include "CAmPersistenceWrapper.h"


namespace am {
namespace gc {
#define APPLICATION_NAME "genericController"
#define CLASS_LEVEL_SINK "*"

CAmClassElement::CAmClassElement(const gc_Class_s& gcClass, CAmControlReceive* pControlReceive) :
                                                CAmElement(gcClass.name, pControlReceive),
                                                mpControlReceive(pControlReceive),
                                                mClass(gcClass),
                                                mSourceClassID(0),
                                                mSinkClassID(0)
{
    mClassLastVolume.className = gcClass.name;
    mLastMainConnections.className = gcClass.name;
    mLastMainConnectionsVolume.className = gcClass.name;
}

CAmClassElement::~CAmClassElement()
{
}

bool CAmClassElement::isSourceSinkPairBelongtoClass(const std::string& sinkName,
                                                    const std::string& sourceName) const
{
    bool isSourceBelongingToClass = isElementBelongtoClass(sourceName,ET_SOURCE);
    bool isSinkBelongingToClass = isElementBelongtoClass(sinkName,ET_SINK);
    if (((true == isSourceBelongingToClass) && (true == isSinkBelongingToClass)) ||
                    ((true  == isSourceBelongingToClass)  && (mClass.type == C_PLAYBACK)) ||
                    ((true == isSinkBelongingToClass) && (mClass.type == C_CAPTURE)))
    {
        return true;
    }
    return false;

}

void CAmClassElement::disposeConnection(const CAmMainConnectionElement* pMainConnection)
{
    std::vector<CAmMainConnectionElement*>::iterator itListMainConnections;
    std::set<std::string>::iterator itListLastMainConnections;

    if (NULL != pMainConnection)
    {
        std::set<std::string>::iterator itListLastMainConnections;

        /*find the main connection in the mLastMainConnections list and if exsist them remove it */
        itListLastMainConnections = std::find(mLastMainConnections.listMainConnections.begin(),
                                              mLastMainConnections.listMainConnections.end(),pMainConnection->getName());
        if (itListLastMainConnections != mLastMainConnections.listMainConnections.end())
        {
            LOG_FN_INFO(__FILENAME__,__func__,"Erasing last main connection :",pMainConnection->getName());
            mLastMainConnections.listMainConnections.erase(itListLastMainConnections);
        }
        /*find the mainconnection in the last main connection volume list and if exsist them remove it */
       /* std::vector<gc_LastMainConVolInfo_s>::iterator itlistLastMainConVolInfo;

        for (itlistLastMainConVolInfo = mLastMainConnectionsVolume.listLastMainConVolInfo.begin();
                        itlistLastMainConVolInfo != mLastMainConnectionsVolume.listLastMainConVolInfo.end();++itlistLastMainConVolInfo)
        {
            if (itlistLastMainConVolInfo->mainConnectionName == pMainConnection->getName())
            {
                break;
            }
        }
        if (itlistLastMainConVolInfo != mLastMainConnectionsVolume.listLastMainConVolInfo.end())
        {
            LOG_FN_INFO(__FILENAME__,__func__,"Erasing volume information for main connection:",pMainConnection->getName());
            mLastMainConnectionsVolume.listLastMainConVolInfo.erase(itlistLastMainConVolInfo);
        }*/
        //remove link between class element and main connection element
        detach((CAmElement*)pMainConnection);
        CAmMainConnectionFactory::destroyElement(pMainConnection->getID());
    }
    for (itListMainConnections = mListMainConnections.begin();
                    itListMainConnections != mListMainConnections.end(); ++itListMainConnections)
    {
        if (*itListMainConnections == pMainConnection)
        {
            break;
        }
    }
    if (itListMainConnections != mListMainConnections.end())
    {
        mListMainConnections.erase(itListMainConnections);
    }
    mListOldMainConnections = mListMainConnections;
}

CAmMainConnectionElement* CAmClassElement::getMainConnection(
                const std::string& sourceName, const std::string& sinkName,
                std::vector<am_ConnectionState_e >& listConnectionStates, const gc_Order_e order)
{
    std::vector<CAmMainConnectionElement* > listMainConnections;
    std::vector<CAmMainConnectionElement* >::iterator itListMainConnections;
    std::vector<CAmMainConnectionElement* >::reverse_iterator itListReverseMainConnections;
    CAmMainConnectionElement* pMainConnection = NULL;
    CAmConnectionListFilter filterObject;
    filterObject.setSourceName(sourceName);
    filterObject.setSinkName(sinkName);
    filterObject.setListConnectionStates(listConnectionStates);
    getListMainConnections(listMainConnections,filterObject);
    int32_t tempPriority = 0;
    int32_t tempPriority1;
    switch (order)
    {
    case O_HIGH_PRIORITY:
        for (itListMainConnections = listMainConnections.begin();
                        itListMainConnections != listMainConnections.end(); itListMainConnections++)
        {
            (*itListMainConnections)->getPriority(tempPriority1);
            // store the first connection pointer found in queue which is in given state
            if (NULL == pMainConnection)
            {
                LOG_FN_DEBUG(__FILENAME__,__func__,"  connection found with ID:", (*itListMainConnections)->getID());
                pMainConnection = *itListMainConnections;
                pMainConnection->getPriority(tempPriority);
            }
            else if (tempPriority > tempPriority1) // update connection pointer if higher priority connection is found with given state.  Lower number means higher priority.
            {
                pMainConnection = *itListMainConnections;
                pMainConnection->getPriority(tempPriority);
                LOG_FN_DEBUG(__FILENAME__,__func__,"  higher priority connection found with ID:Priority:",
                             (*itListMainConnections)->getID(), tempPriority);
            }
        }
        break;
    case O_LOW_PRIORITY:
        for (itListMainConnections = listMainConnections.begin();
                        itListMainConnections != listMainConnections.end(); itListMainConnections++)
        {

            (*itListMainConnections)->getPriority(tempPriority1);
            // store the first connection pointer found in queue which is in given state
            if (NULL == pMainConnection)
            {
                LOG_FN_DEBUG(__FILENAME__,__func__,"  connection found with ID:", (*itListMainConnections)->getID());
                pMainConnection = *itListMainConnections;
                pMainConnection->getPriority(tempPriority);
            }
            else if (tempPriority < tempPriority1) // update connection pointer if higher priority connection is found with given state. Higher number means lower priority
            {
                LOG_FN_DEBUG(__FILENAME__,__func__,"  lower priority connection found with ID:",
                             (*itListMainConnections)->getID());
                pMainConnection = *itListMainConnections;
                pMainConnection->getPriority(tempPriority);
            }
        }
        break;
    case O_NEWEST:
        itListReverseMainConnections = listMainConnections.rbegin();
        if (itListReverseMainConnections != listMainConnections.rend())
        {
            pMainConnection = *itListReverseMainConnections;
        }
        break;
    case O_OLDEST:
        LOG_FN_DEBUG(__FILENAME__,__func__,"Number of connections =", mListMainConnections.size());
        itListMainConnections = listMainConnections.begin();
        if (itListMainConnections != listMainConnections.end())
        {
            pMainConnection = *itListMainConnections;
        }
        break;
    default:
        break;
    }
    LOG_FN_EXIT(__FILENAME__,__func__);
    return pMainConnection;
}

void CAmClassElement::getListMainConnections(std::vector<CAmMainConnectionElement* >& listMainConnections,
                                             const CAmConnectionListFilter& fobject)
{
    listMainConnections = std::for_each(mListMainConnections.begin(), mListMainConnections.end(),
                                        fobject).getListMainConnection();
}

am_Error_e CAmClassElement::createMainConnection(const std::string& sourceName,
                                                 const std::string& sinkName,
                                                 am_mainConnectionID_t& mainConnectionID)
{
    CAmMainConnectionElement* pMainConnection = NULL;
    gc_Route_s route;
    am_Error_e result = E_NOT_POSSIBLE;
    CAmSourceElement* pSourceElement;
    CAmSinkElement* pSinkElement;
    std::vector < am_Route_s > listRoutes;

    pSourceElement = CAmSourceFactory::getElement(sourceName);
    pSinkElement = CAmSinkFactory::getElement(sinkName);
    if ((NULL == pSourceElement) || (NULL == pSinkElement))
    {
        LOG_FN_ERROR(__FILENAME__,__func__,"Source or sink doesn't exist");
        return E_NOT_POSSIBLE;
    }
    pMainConnection = CAmMainConnectionFactory::getElement(sourceName + ":" + sinkName);
    if (NULL != pMainConnection)
    {
        result = E_ALREADY_EXISTS;
    }
    else
    {
        /*
         * 1. get the route from daemon
         * 2. get the route from topology
         * 3. validate if the route from topology exist in the list retrieved from daemon
         * 4. if the route is present then use that route for connection creation.
         */
        result = mpControlReceive->getRoute(false, pSourceElement->getID(), pSinkElement->getID(),
                                            listRoutes);
        if ((E_OK != result) || (true == listRoutes.empty()))
        {
            LOG_FN_ERROR(__FILENAME__,__func__,"getting route list from daemon failed");
        }
        else
        {
            CAmSystemElement* pSystemElement;
            pSystemElement = CAmSystemFactory::getElement(SYSTEM_ELEMENT_NAME);
            result = _getPreferredRoute(sourceName, sinkName, route);
            if ((result != E_OK) && (pSystemElement->isNonTopologyRouteAllowed()==true))
            {
                LOG_FN_INFO(__FILENAME__,__func__,getName(), "failed to get route from topology");
                /*
                 * Preferred route not found from topology could be unknown source or sink !! select
                 * the first route from daemon.
                 */
                route.sinkID = listRoutes.begin()->sinkID;
                route.sourceID = listRoutes.begin()->sourceID;
                route.route = listRoutes.begin()->route;
                route.name = sourceName + ":" + sinkName;
                result = E_OK;
            }
            else
            {
                result = _validateRouteFromTopology(listRoutes, route);
            }
        }
        if (result == E_OK)
        {
            // create main connection object
            pMainConnection = CAmMainConnectionFactory::createElement(route, mpControlReceive);
        }
        if(pMainConnection != NULL)
        {
            result = pMainConnection->attach(pSourceElement);
            if(result != E_OK)
            {
                /*its an error need to decide */
                LOG_FN_DEBUG("element attach failed, result is:",result);
            }
            result = pMainConnection->attach(pSinkElement);
            if(result != E_OK)
            {
                /*its an error need to decide */
                LOG_FN_DEBUG("element attach failed, result is:",result);

            }
            result = attach(pMainConnection);
            if(result != E_OK)
            {
                /*its an error need to decide */
                LOG_FN_DEBUG("element attach failed, result is:",result);

            }
        }
    }
    if (NULL != pMainConnection)
    {
        // set mainConnectionID to return parameter
        mainConnectionID = pMainConnection->getID();
        pushMainConnectionInQueue(pMainConnection);
        result = E_OK;
    }
    return result;
}

am_Error_e CAmClassElement::_validateRouteFromTopology(std::vector<am_Route_s >& listRoutes,
                                                       gc_Route_s& topologyRoute) const
{
    am_Error_e result = E_NOT_POSSIBLE;
    std::vector<am_Route_s >::iterator itListRoutes;
    std::vector<am_RoutingElement_s >::iterator itdaemonRoute;
    std::vector<am_RoutingElement_s >::reverse_iterator ittopologyRoute;
    LOG_FN_ENTRY(__FILENAME__,__func__,listRoutes.size());
    for (itListRoutes = listRoutes.begin(); itListRoutes != listRoutes.end(); ++itListRoutes)
    {
        if (itListRoutes->route.size() != topologyRoute.route.size())
        {
            LOG_FN_DEBUG(__FILENAME__,__func__,"routes size don't match",itListRoutes->route.size(),"=",topologyRoute.route.size());
            continue;
        }
        for (itdaemonRoute = itListRoutes->route.begin(), ittopologyRoute = topologyRoute.route.rbegin();
                        itdaemonRoute != itListRoutes->route.end();
                        ++itdaemonRoute, ++ittopologyRoute)
        {
            LOG_FN_INFO(__FILENAME__,__func__,"checking next element");
            if ((itdaemonRoute->sinkID != ittopologyRoute->sinkID) || (itdaemonRoute->sourceID
                            != ittopologyRoute->sourceID))
            {
                break;
            }
        }
        if (itdaemonRoute == itListRoutes->route.end())
        {
            topologyRoute.sinkID = itListRoutes->sinkID;
            topologyRoute.sourceID = itListRoutes->sourceID;
            topologyRoute.route = itListRoutes->route;
            result = E_OK;
            break;
        }
    }
    return result;
}
am_Error_e CAmClassElement::setLimitState(const gc_LimitState_e limitState,
                                          const gc_LimitVolume_s& limitVolume,
                                          const uint32_t pattern)
{
    /*
     * Add the volume limit in the map, in case if the same pattern is already present
     * overwrite the limit.
     */
    if (LS_LIMITED == limitState)
    {
        mMapLimitVolumes[pattern] = limitVolume;
        LOG_FN_DEBUG(__FILENAME__,__func__,"ADDED", limitVolume.limitType, limitVolume.limitVolume);
    }
    else if (LS_UNLIMITED == limitState)
    {
        /*
         * The pattern is a bit mask to clear the volume limit, clear the entries
         * which match the pattern
         */
        std::map<uint32_t, gc_LimitVolume_s >::iterator itMapLimitVolumes(mMapLimitVolumes.begin());
        for (; itMapLimitVolumes != mMapLimitVolumes.end();)
        {
            if (0 == (itMapLimitVolumes->first & (~pattern)))
            {
                itMapLimitVolumes = mMapLimitVolumes.erase(itMapLimitVolumes);
                LOG_FN_DEBUG(__FILENAME__,__func__,"ERASED");
            }
            else
            {
                ++itMapLimitVolumes;
            }
        }
    }
    return E_OK;
}

am_Error_e CAmClassElement::getListClassLimits(std::map<uint32_t, gc_LimitVolume_s >& mapLimits)
{
    mapLimits =  mMapLimitVolumes;
    return E_OK;
}

void CAmClassElement::getListMainConnections(
                std::vector<CAmMainConnectionElement* >& listConnections)
{
    listConnections = mListMainConnections;
}


am_Error_e CAmClassElement::_checkElementPresentInTopology(const std::vector<gc_TopologyElement_s>& topology,
                                                           const std::string& elementName,
                                                           gc_ClassTopologyCodeID_e type) const
{
    std::vector<gc_TopologyElement_s >::const_iterator itListTopologyElements;
    for (itListTopologyElements = topology.begin();
                    itListTopologyElements != topology.end(); ++itListTopologyElements)
    {
        if ((type == itListTopologyElements->codeID) &&
            (elementName == itListTopologyElements->name))
        {
            return E_OK;
        }
    }
    return E_UNKNOWN;
}

am_Error_e CAmClassElement::_getRouteFromTopology(const std::vector<gc_TopologyElement_s>& topology,
                                                  const std::string& mainSourceName,
                                                  const std::string& mainSinkName,
                                                  gc_Route_s& route) const
{
    CAmSinkElement *pMainSinkElement, *pSinkElement;
    CAmSourceElement *pMainSourceElement, *pSourceElement;
    am_RoutingElement_s routingElement;
    CAmGatewayElement* pGatewayElement;
    std::vector<gc_TopologyElement_s >::const_iterator itListTopologyElements;
    int gatewayFoundCounter = 0;
    bool sourceFound = false;

    LOG_FN_ENTRY(__FILENAME__,__func__,mainSourceName, mainSinkName);
    if (0 == isSourceSinkPairBelongtoClass(mainSinkName, mainSourceName))
    {
        return E_UNKNOWN;
    }

    pMainSinkElement = CAmSinkFactory::getElement(mainSinkName);
    pMainSourceElement = CAmSourceFactory::getElement(mainSourceName);
    if ((NULL == pMainSourceElement) || (NULL == pMainSinkElement))
    {
        LOG_FN_ERROR(__FILENAME__,__func__," Not able to get source and sink");
        return E_UNKNOWN;
    }

    /*
     * Verify if the sink name is present in the topology
     */
    if (E_OK != _checkElementPresentInTopology(topology,mainSinkName))
    {
        return E_NOT_POSSIBLE;
    }

    route.sinkID = pMainSinkElement->getID();
    route.sourceID = pMainSourceElement->getID();
    route.name = pMainSourceElement->getName() + ":" + pMainSinkElement->getName();
    // This field would be overwritten during topology validation.
    routingElement.connectionFormat = 0;
    routingElement.domainID = pMainSinkElement->getDomainID();
    routingElement.sinkID = pMainSinkElement->getID();
    //search for gateway and source element
    for (itListTopologyElements = topology.begin();
                    itListTopologyElements != topology.end();
                    itListTopologyElements++)
    {
        // if topology element is source
        if (MC_SOURCE_ELEMENT == itListTopologyElements->codeID)
        {
            // get the source element by name
            pSourceElement = CAmSourceFactory::getElement(itListTopologyElements->name);
            if (NULL == pSourceElement)
            {
                continue;
            }
            //check if source is as per request
            if (mainSourceName == pSourceElement->getName())
            {
                routingElement.sourceID = pMainSourceElement->getID();
                route.route.push_back(routingElement);
                sourceFound = true;
                break;
            }

        }
        //if topology element is gateway
        else if (MC_GATEWAY_ELEMENT == itListTopologyElements->codeID)
        {
            // get the gateway element by name
            pGatewayElement = CAmGatewayFactory::getElement(itListTopologyElements->name);
            if (NULL == pGatewayElement)
            {
                /*
                 * Gateway does not exist. source is not found. so this gateway not
                 * required in this connection
                 *  delete the source ID, sink ID and gateway element from vector
                 */
                for (int i = 0; i < gatewayFoundCounter; i++)
                {
                    //remove sink ID
                    routingElement = route.route.back();
                    route.route.pop_back();
                }
                gatewayFoundCounter++;
                int loopCount = 0;
                while (loopCount < gatewayFoundCounter)
                {
                    itListTopologyElements++;
                    if (itListTopologyElements == topology.end())
                    {
                        break;
                    }
                    while (MC_RBRACKET_CODE != itListTopologyElements->codeID)
                    {
                        if (MC_GATEWAY_ELEMENT == itListTopologyElements->codeID)
                        {
                            gatewayFoundCounter++;
                        }
                        itListTopologyElements++;
                        if (itListTopologyElements == topology.end())
                        {
                            break;
                        }
                    }

                    loopCount++;
                }
                gatewayFoundCounter = 0;
                continue;
            }
            // increment the gatewayfound counter
            gatewayFoundCounter++;
            //get the source and sink ID of gateway
            routingElement.sourceID = pGatewayElement->getSourceID();
            route.route.push_back(routingElement);
            routingElement.sinkID = pGatewayElement->getSinkID();
            routingElement.domainID = pGatewayElement->getSinkDomainID();
        }
        // if topology element is )
        else if (MC_RBRACKET_CODE == itListTopologyElements->codeID)
        {
            // source is not found. so this gateway not required in this connection
            // delete the source ID, sink ID and gateway element from vector
            for (int loopCount = 0; loopCount < gatewayFoundCounter; loopCount++)
            {
                routingElement = route.route.back();
                route.route.pop_back();
            }
            gatewayFoundCounter = 0;
        }
    }
    // if source is found then form the pair of source and sink to be connected
    return (sourceFound == true) ? E_OK : E_UNKNOWN;
}

am_Error_e CAmClassElement::_getPreferredRoute(const std::string& mainSourceName,
                                               const std::string& mainSinkName, gc_Route_s& route) const
{
    am_Error_e result = E_NOT_POSSIBLE;
    std::vector<std::vector< gc_TopologyElement_s> >::const_iterator itListTopologies;
    for(itListTopologies = mClass.listTopologies.begin();
                    itListTopologies != mClass.listTopologies.end();
                    ++itListTopologies)
    {
        result = _getRouteFromTopology((*itListTopologies),
                                       mainSourceName,
                                       mainSinkName,
                                       route);
        if (E_OK == result)
        {
            break;
        }
    }
    return result;
}

void CAmClassElement::updateMainConnectionQueue(void)
{
    std::vector<CAmMainConnectionElement* >::iterator itListMainConnections;
    CAmMainConnectionElement* pMainConnection;
    int state;
    /*
     * If there is connected or suspended connection change its order to
     * the top of queue
     */
    for (itListMainConnections = mListMainConnections.begin();
                    itListMainConnections != mListMainConnections.end(); ++itListMainConnections)
    {
        (*itListMainConnections)->getState(state);
        if ((CS_CONNECTED == state) || (CS_SUSPENDED == state))
        {
            break;
        }
    }
    if (itListMainConnections != mListMainConnections.end())
    {
        pMainConnection = *itListMainConnections;
        mListMainConnections.erase(itListMainConnections);
        mListMainConnections.push_back(pMainConnection);
    }
}

void CAmClassElement::pushMainConnectionInQueue(CAmMainConnectionElement* pMainConnection)
{
    std::vector<CAmMainConnectionElement* >::iterator itListMainConnection;
    itListMainConnection = std::find(mListMainConnections.begin(), mListMainConnections.end(),
                                     pMainConnection);
    if (itListMainConnection != mListMainConnections.end())
    {
        mListMainConnections.erase(itListMainConnection);
    }
    mListMainConnections.push_back(pMainConnection);
    updateMainConnectionQueue();
}

am_Error_e CAmClassElement::_register(void)
{
    am_SourceClass_s sourceClassInstance;
    am_SinkClass_s sinkClassInstance;

    if(mClass.classID < DYNAMIC_ID_BOUNDARY)
    {
        sourceClassInstance.sourceClassID = mClass.classID;
        sinkClassInstance.sinkClassID = mClass.classID;
    }
    else
    {
        sourceClassInstance.sourceClassID = 0;
        sinkClassInstance.sinkClassID = 0;
    }
    sourceClassInstance.name = mClass.name;
    sourceClassInstance.listClassProperties = mClass.listClassProperties;

    // store source class in DB
    if (E_OK == mpControlReceive->enterSourceClassDB(sourceClassInstance.sourceClassID,
                                                     sourceClassInstance))
    {
        if (C_PLAYBACK == mClass.type)
        {
            setID(sourceClassInstance.sourceClassID);
        }
    }
    else
    {
        LOG_FN_ERROR(__FILENAME__,__func__," Error while registering source Class");
        return E_DATABASE_ERROR;
    }

    sinkClassInstance.name = mClass.name;
    sinkClassInstance.listClassProperties = mClass.listClassProperties;
    // store sink class in DB
    if (E_OK == mpControlReceive->enterSinkClassDB(sinkClassInstance,
                                                   sinkClassInstance.sinkClassID))
    {
        if (C_CAPTURE == mClass.type)
        {
            setID(sinkClassInstance.sinkClassID);
            LOG_FN_ERROR(__FILENAME__,__func__," Error while registering sink Class");
        }
    }
    else
    {
        LOG_FN_ERROR(__FILENAME__,__func__," Error while registering source Class");
        return E_DATABASE_ERROR;
    }
    mSinkClassID = sinkClassInstance.sinkClassID;
    mSourceClassID  = sourceClassInstance.sourceClassID;
    return E_OK;
}

am_Error_e CAmClassElement::_unregister(void)
{
    if (C_PLAYBACK == mClass.type)
    {
        mpControlReceive->removeSourceClassDB(getID());
    }
    else if (C_CAPTURE == mClass.type)
    {
        mpControlReceive->removeSinkClassDB(getID());
    }
    return E_OK;
}


CAmClassElement* CAmClassFactory::getElement(const std::string sourceName,
                                             const std::string sinkName)
{
    CAmClassElement* pClassElement = NULL;
    std::vector<CAmClassElement* > listElements;
    std::vector<CAmClassElement* >::iterator itListElements;
    getListElements(listElements);
    for (itListElements = listElements.begin(); itListElements != listElements.end();
                    ++itListElements)
    {
        if ((*itListElements)->isSourceSinkPairBelongtoClass(sinkName, sourceName))
        {
            pClassElement = *itListElements;
        }
    }
    return pClassElement;
}

void CAmClassFactory::getElementsBySource(const std::string sourceName,
                                          std::vector<CAmClassElement* >& listClasses)
{
    std::vector<CAmClassElement* > listElements;
    std::vector<CAmClassElement* >::iterator itListElements;
    getListElements(listElements);
    for (itListElements = listElements.begin(); itListElements != listElements.end();
                    ++itListElements)
    {
        if ((*itListElements)->isElementBelongtoClass(sourceName,ET_SOURCE))
        {
            listClasses.push_back(*itListElements);
        }
    }
}

void CAmClassFactory::getElementsBySink(const std::string sinkName,
                                        std::vector<CAmClassElement* >& listClasses)
{
    std::vector<CAmClassElement* > listElements;
    std::vector<CAmClassElement* >::iterator itListElements;
    getListElements(listElements);
    for (itListElements = listElements.begin(); itListElements != listElements.end();
                    ++itListElements)
    {
        if ((*itListElements)->isElementBelongtoClass(sinkName,ET_SINK))
        {
            listClasses.push_back(*itListElements);
        }
    }
}

am_sourceID_t CAmClassElement::getSourceClassID(void) const
{
    return mSourceClassID;
}

am_sinkID_t CAmClassElement::getSinkClassID(void) const
{
    return mSinkClassID;
}

CAmClassElement* CAmClassFactory::getElementBySourceClassID(const am_sourceClass_t sourceClassID)
{
    std::vector<CAmClassElement* > listElements;
    std::vector<CAmClassElement* >::iterator itListElements;
    getListElements(listElements);
    for (itListElements = listElements.begin(); itListElements != listElements.end();
                    ++itListElements)
    {
        if ((*itListElements)->getSourceClassID() == sourceClassID)
        {
            return (*itListElements);
        }
    }
    return NULL;
}

CAmClassElement* CAmClassFactory::getElementBySinkClassID(const am_sinkClass_t sinkClassID)
{
    std::vector<CAmClassElement* > listElements;
    std::vector<CAmClassElement* >::iterator itListElements;
    getListElements(listElements);
    for (itListElements = listElements.begin(); itListElements != listElements.end();
                    ++itListElements)
    {
        if ((*itListElements)->getSinkClassID() == sinkClassID)
        {
            return (*itListElements);
        }
    }
    return NULL;
}

CAmClassElement* CAmClassFactory::getElementByConnection(const std::string& connectionName)
{
    CAmClassElement* pClassElement = NULL;
    std::vector<CAmClassElement* > listElements;
    std::vector<CAmClassElement* >::iterator itListElements;
    bool found = false;
    getListElements(listElements);
    for (itListElements = listElements.begin(); itListElements != listElements.end();
                    ++itListElements)
    {
        std::vector<CAmMainConnectionElement* >::iterator itListMainConnections;
        std::vector<CAmMainConnectionElement* > listMainConnections;
        (*itListElements)->getListMainConnections(listMainConnections);
        for (itListMainConnections = listMainConnections.begin();
                        itListMainConnections != listMainConnections.end(); ++itListMainConnections)
        {
            if ((*itListMainConnections)->getName() == connectionName)
            {
                found = true;
                break;
            }
        }
        if (true == found)
        {
            break;
        }
    }
    if (itListElements != listElements.end())
    {
        pClassElement = *itListElements;
    }
    return pClassElement;
}

am_MuteState_e CAmClassElement::getMuteState() const
{
    am_MuteState_e muteState = MS_UNMUTED;
    std::map<uint32_t, gc_LimitVolume_s >::const_iterator itMapLimitVolumes;
    for (itMapLimitVolumes = mMapLimitVolumes.begin(); itMapLimitVolumes != mMapLimitVolumes.end();
                    ++itMapLimitVolumes)
    {
        if ((LT_ABSOLUTE == itMapLimitVolumes->second.limitType) &&
                        (AM_MUTE == itMapLimitVolumes->second.limitVolume))
        {
            muteState = MS_MUTED;
            break;
        }
    }
    return muteState;
}

CAmConnectionListFilter::CAmConnectionListFilter() :
                                                mSourceName(""),
                                                mSinkName("")
{
    mListConnectionStates.clear();
    mListExceptSinks.clear();
    mListExceptSources.clear();
}
void CAmConnectionListFilter::setSourceName(std::string sourceName)
{
    mSourceName = sourceName;
}

void CAmConnectionListFilter::setSinkName(std::string sinkName)
{
    mSinkName = sinkName;
}

void CAmConnectionListFilter::setListConnectionStates(
                std::vector<am_ConnectionState_e >& listConnectionStates)
{
    mListConnectionStates = listConnectionStates;
}

void CAmConnectionListFilter::setListExceptSourceNames(std::vector<std::string >& listExceptSources)
{
    mListExceptSources = listExceptSources;
}

void CAmConnectionListFilter::setListExceptSinkNames(std::vector<std::string >& listExceptSinks)
{
    mListExceptSinks = listExceptSinks;
}

std::vector<CAmMainConnectionElement* >& CAmConnectionListFilter::getListMainConnection()
{
    return mListMainConnections;
}

void CAmConnectionListFilter::operator()(CAmMainConnectionElement* pMainConnection)
{
    if (NULL == pMainConnection)
    {
        return;
    }
    int state;
    pMainConnection->getState(state);
    std::string sourceName = pMainConnection->getMainSourceName();
    std::string sinkName = pMainConnection->getMainSinkName();
    if (_stringMatchFilter(mSourceName, sourceName) && _stringMatchFilter(mSinkName, sinkName)
                    && _connetionStateFilter((am_ConnectionState_e)state)
                    && _exceptionNamesFilter(mListExceptSources, sourceName)
                    && _exceptionNamesFilter(mListExceptSinks, sinkName))
    {
        mListMainConnections.push_back(pMainConnection);
    }

}

bool CAmConnectionListFilter::_stringMatchFilter(std::string filterName, std::string inputName)
{
    bool result = false;
    if ("" == filterName)
    {
        result = true;
    }
    else
    {
        if (filterName == inputName)
        {
            result = true;
        }
    }
    return result;
}
bool CAmConnectionListFilter::_connetionStateFilter(am_ConnectionState_e connectionState)
{
    bool result = false;
    if (std::find(mListConnectionStates.begin(), mListConnectionStates.end(), connectionState) != mListConnectionStates.end())
    {
        result = true;
    }
    return result;
}
bool CAmConnectionListFilter::_exceptionNamesFilter(std::vector<std::string >& listString,
                                                    std::string input)
{
    bool result = true;
    if (std::find(listString.begin(), listString.end(), input) != listString.end())
    {
        result = false;
    }
    return result;
}

gc_Class_e CAmClassElement::getClassType() const
{
    return mClass.type;
}

bool CAmClassElement::isPerSinkClassVolumeEnabled(void) const
{
    std::vector<am_ClassProperty_s>::const_iterator itMapClassProperty;
    for( itMapClassProperty = mClass.listClassProperties.begin();
                    itMapClassProperty != mClass.listClassProperties.end();
                    itMapClassProperty++
    )
    {
        if((*itMapClassProperty).classProperty == CP_PER_SINK_CLASS_VOLUME_SUPPORT)
        {
            return ((*itMapClassProperty).value == 0) ? false : true;
        }
    }
    return false;
}

int CAmClassElement::releaseResources(CAmElement *pNotifierElement)
{
    if(pNotifierElement != NULL)
    {
        LOG_FN_DEBUG(__FILENAME__,__func__,"notifing element id is :",pNotifierElement->getName());
        detach(pNotifierElement);
    }
    else
    {
        LOG_FN_DEBUG(__FILENAME__,__func__,"notifing element is invalid");
    }

}

int CAmClassElement::update(CAmElement *pNotifierElement,const am_ConnectionState_e& state)
{
    int result= E_UNKNOWN;
    std::vector<CAmMainConnectionElement* >::iterator itListMainConnections;
    CAmSourceElement* pSourceElement = NULL;
    CAmSinkElement* pSinkElement = NULL;
    std::string className;
    std::string mainConnectionList;

    if(NULL != pNotifierElement)
    {
        if(state == CS_CONNECTED)
        {
            if(mListMainConnections != mListOldMainConnections )
            {
                /*update the main connection in the local structure */
                for (itListMainConnections = mListMainConnections.begin();
                                itListMainConnections != mListMainConnections.end(); itListMainConnections++)
                {
                    pSourceElement = (*itListMainConnections)->getMainSource();
                    pSinkElement =  (*itListMainConnections)->getMainSink();
                    if(pSourceElement->isPersistencySupported() || pSinkElement->isPersistencySupported())
                    {
                        mLastMainConnections.listMainConnections.insert((*itListMainConnections)->getName());
                        LOG_FN_INFO(__FILENAME__,__func__,"inserting main connection",(*itListMainConnections)->getName());
                    }
                    else
                    {
                        LOG_FN_INFO(__FILENAME__,__func__,"persistence not supported for main source:",
                                    pSourceElement->getName(),"or main sink",pSinkElement->getName());
                    }
                }
                mListOldMainConnections = mListMainConnections;
            }
            else
            {
                LOG_FN_INFO(__FILENAME__,__func__,"old connection list and new connections list are same");
            }
        }
    }
    else
    {
        result = E_NOT_POSSIBLE;
        LOG_FN_ERROR(__FILENAME__,__func__,"notifing element is invalid");
    }
    LOG_FN_EXIT(__FILENAME__,__func__);
    return result;
}

int CAmClassElement::update(CAmElement *pNotifierElement,gc_Element_Status_e& elementStatus)
{
    LOG_FN_ENTRY(__FILENAME__,__func__);
    if(pNotifierElement != NULL)
    {
        if(elementStatus == MAINCONNECTION_DISCONNECT)
        {
            LOG_FN_DEBUG(__FILENAME__,__func__,"class element disconnection the mainconnection, its name is :",pNotifierElement->getName());
            /*create the class action disconnect*/
            IAmActionCommand* pAction(NULL);
            CAmRootAction* pRootAction = CAmRootAction::getInstance();
            pAction = new CAmClassActionDisconnect((CAmClassElement*)this);
            if (NULL == pAction)
            {
                LOG_FN_ERROR(__FILENAME__,__func__,"  bad memory state");
                return E_NOT_POSSIBLE;
            }
            /*attach the dynamic action pointer to root*/
            pRootAction->append(pAction);
        }
    }
    else
    {
        LOG_FN_DEBUG(__FILENAME__,__func__,"notifing element is invalid");
    }
    LOG_FN_EXIT(__FILENAME__,__func__);
}

bool CAmClassElement::isElementBelongtoClass(const std::string& elementName,gc_Element_e elementType)const
{
    std::vector<CAmElement* > listOfSubjects;
    std::vector<CAmElement* >::iterator itListSubjects;
    gc_ElementTypeName_s element;

    element.elementName = elementName;
    element.elementType = elementType;
    getListElements(element,listOfSubjects);
    for (itListSubjects = listOfSubjects.begin(); itListSubjects != listOfSubjects.end();
                    ++itListSubjects)
    {
        return true;
    }
    /*
     * Verify if both source and sink belong to topology
     */
    gc_ClassTopologyCodeID_e topologyType = (elementType == ET_SINK)? MC_SINK_ELEMENT : MC_SOURCE_ELEMENT;
    std::vector<std::vector< gc_TopologyElement_s> >::const_iterator itListTopologies;
    for(itListTopologies = mClass.listTopologies.begin();
                    itListTopologies != mClass.listTopologies.end();
                    ++itListTopologies)
    {
        if( E_OK == _checkElementPresentInTopology((*itListTopologies),elementName,topologyType))
        {
            return true;
        }
        else
        {
            LOG_FN_DEBUG(__FILENAME__,__func__,"Element :",elementName, "not found for class :",
                       getName());
        }
    }
    return false;
}

bool CAmClassElement::isVolumePersistencySupported() const
{
    return mClass.isVolumePersistencySupported;
}

am_Error_e CAmClassElement::restoreVolume(gc_LastClassVolume_s& lastClassVolume)
{
    am_Error_e result = E_UNKNOWN;
    mClassLastVolume = lastClassVolume;
    return result;
}

void CAmClassElement::restoreLastMainConnectionsVolume(gc_LastMainConnectionsVolume_s& lastMainConnectionsVolume)
{
    mLastMainConnectionsVolume = lastMainConnectionsVolume;
}

std::string CAmClassElement::getVolumeString()
{
    string volumeString;
    std::vector<gc_SinkVolume_s>::iterator itlistSinks;

    if((!mClassLastVolume.className.empty()) &&
                    (!mClassLastVolume.listSinkVolume.empty())
    )
    {
        volumeString = "{"+ mClassLastVolume.className +",";
        //sample format of volume string :: {BASE,[sink1:10][sink2:20][*:30]}
        for (itlistSinks = mClassLastVolume.listSinkVolume.begin();
                        itlistSinks != mClassLastVolume.listSinkVolume.end(); itlistSinks++)
        {
            LOG_FN_DEBUG(__FILENAME__,__func__,"sink name is:",itlistSinks->sinkName);
            stringstream mainVolumeString;
            mainVolumeString << itlistSinks->mainVolume;
            volumeString += "["+itlistSinks->sinkName+":"+ mainVolumeString.str() +"]";
        }
        volumeString += "}";
    }
    return volumeString;
}

std::string CAmClassElement::getLastMainConnectionsVolumeString()
{
    string mainConnectionsVolumeString;
    std::vector<gc_SinkVolume_s>::iterator itlistSinks;
    std::vector<gc_LastMainConVolInfo_s>::iterator itlistLastMainConVolInfo;

    if((!mLastMainConnectionsVolume.className.empty()) &&
                    (!mLastMainConnectionsVolume.listLastMainConVolInfo.empty()))
    {
         mainConnectionsVolumeString = "{"+ mLastMainConnectionsVolume.className +",";
        //sample format of last main connection volume string :: {BASE,[source1:sink1=30][source2:sink2=10]}
        for (itlistLastMainConVolInfo = mLastMainConnectionsVolume.listLastMainConVolInfo.begin();
                        itlistLastMainConVolInfo != mLastMainConnectionsVolume.listLastMainConVolInfo.end(); itlistLastMainConVolInfo++)
        {
            stringstream mainVolumeString;
            mainVolumeString << itlistLastMainConVolInfo->mainVolume;
            mainConnectionsVolumeString += "[" + itlistLastMainConVolInfo->mainConnectionName + "=" + mainVolumeString.str() + "]";
        }
        mainConnectionsVolumeString += "}";
    }
    return mainConnectionsVolumeString;
}

std::string CAmClassElement::getMainConnectionString()
{
    string mainConnectionString;
    std::set<std::string>::iterator itlistMainConnections;

    if(!mLastMainConnections.className.empty() && !mLastMainConnections.listMainConnections.empty())
    {
        //sample format of volume string {BASE,source1:sink1;source2:sink2;source3:sink3;}
        mainConnectionString = "{"+ mLastMainConnections.className +",";
        for (itlistMainConnections = mLastMainConnections.listMainConnections.begin();
                        itlistMainConnections != mLastMainConnections.listMainConnections.end(); itlistMainConnections++)
        {
            LOG_FN_DEBUG(__FILENAME__,__func__,"main connection name is:",*itlistMainConnections);
            mainConnectionString +=  *itlistMainConnections +";";
        }
        mainConnectionString += "}";
    }
    return mainConnectionString;
}

am_Error_e CAmClassElement::getLastVolume(const std::string& sinkName,
                                          am_mainVolume_t& mainVolume)
{
    if((isVolumePersistencySupported() == false) &&
       (isPerSinkClassVolumeEnabled()== false))
    {
        return E_OK;
    }

    std::string strCompare(isPerSinkClassVolumeEnabled() ? sinkName : CLASS_LEVEL_SINK);
    std::vector<gc_SinkVolume_s>::iterator itlistSink;
    for (itlistSink = mClassLastVolume.listSinkVolume.begin();
         itlistSink != mClassLastVolume.listSinkVolume.end(); itlistSink++)
    {
        if (itlistSink->sinkName == strCompare)
        {
            LOG_FN_DEBUG(__FILENAME__,__func__,"class last main volume is:", mainVolume, sinkName);
            mainVolume = itlistSink->mainVolume;
            return E_OK;
        }
    }
    if (mClass.defaultVolume != AM_MUTE)
    {
        mainVolume = mClass.defaultVolume;
        LOG_FN_DEBUG(__FILENAME__,__func__,"class default main volume is:", mainVolume);
    }
    return E_OK;
}

am_Error_e CAmClassElement::getLastVolume(CAmMainConnectionElement* pMainConnection,am_mainVolume_t& mainVolume)
{
    am_Error_e result = E_UNKNOWN;

    if(pMainConnection != NULL)
    {
        /* 1. check if last main connection volume is present, if present then return the same
         * 2. if last main connection volume not present then check if last class level volume
         * is present, if present then return the same
         * 3. if last class volume not present then return the default class volume
         */
        std::vector<gc_LastMainConVolInfo_s>::iterator itlistLastMainConVolInfo;
        for ( itlistLastMainConVolInfo = mLastMainConnectionsVolume.listLastMainConVolInfo.begin();
                        itlistLastMainConVolInfo != mLastMainConnectionsVolume.listLastMainConVolInfo.end();
                        itlistLastMainConVolInfo++)
        {
            if(itlistLastMainConVolInfo->mainConnectionName == pMainConnection->getName())
            {
                mainVolume = itlistLastMainConVolInfo->mainVolume;
                LOG_FN_DEBUG(__FILENAME__,__func__,"last main connection",pMainConnection->getName(),"main volume is:", mainVolume);
                result = E_OK;
                return result;
            }
        }
        CAmSinkElement* pSinkElement = pMainConnection->getMainSink();
        if(pSinkElement!= NULL)
        {
            result = getLastVolume(pSinkElement->getName(),mainVolume);
            return result;
        }
    }
    return E_NOT_POSSIBLE;
}

am_Error_e CAmClassElement::setLastVolume(CAmMainConnectionElement* pMainConnection,const std::string& sinkName,
                             am_mainVolume_t mainVolume)
{
    am_Error_e result = E_UNKNOWN;
    if(pMainConnection != NULL)
    {
        result = _setLastMainConnectionVolume(pMainConnection);
        /*check if class level volume to be stored */
        CAmSourceElement* pSourceElement = pMainConnection->getMainSource();
        CAmSinkElement* pSinkElement = pMainConnection->getMainSink();
        if ((NULL != pSinkElement) && (NULL != pSourceElement))
        {
            result = _setLastClassVolume(pSinkElement->getName(),pMainConnection->getMainVolume());
        }
    }
    if(!sinkName.empty())
    {
        result = _setLastClassVolume(sinkName,mainVolume);
    }
    return result;
}

am_Error_e CAmClassElement::_setLastClassVolume(const std::string& sinkName,am_mainVolume_t mainVolume)
{
    std::string localSinkName = sinkName;
    std::vector<gc_SinkVolume_s>::iterator itlistSink;
    am_Error_e result = E_UNKNOWN;

    if((isVolumePersistencySupported() == false) &&
       (isPerSinkClassVolumeEnabled()==false))
    {
        return E_OK;
    }
    LOG_FN_DEBUG(__FILENAME__,__func__,getName(),sinkName,mainVolume);
    if(false == isPerSinkClassVolumeEnabled())
    {
        localSinkName = "*";
    }
    for ( itlistSink = mClassLastVolume.listSinkVolume.begin();
                    itlistSink != mClassLastVolume.listSinkVolume.end(); itlistSink++)
    {
        //if sink is already exist then update only its main volume in the list
        if(itlistSink->sinkName == localSinkName)
        {
            itlistSink->mainVolume = mainVolume;
            result = E_OK;
            break;
        }
    }
    //if sink is not found in the list then consider it as new sink and insert in to the list
    if(itlistSink == mClassLastVolume.listSinkVolume.end())
    {
        gc_SinkVolume_s sinkVolume;
        sinkVolume.sinkName = localSinkName;
        sinkVolume.mainVolume = mainVolume;
        mClassLastVolume.listSinkVolume.push_back(sinkVolume);
        result = E_OK;
    }
    if(true == isPerSinkClassVolumeEnabled())
    {
        CAmSinkElement* pSink = CAmSinkFactory::getElement(localSinkName);
        if(NULL != pSink)
        {
            am_MainSoundProperty_s mainSoundProperty;
            mainSoundProperty.type = MSP_SINK_PER_CLASS_VOLUME_TYPE(getID());
            mainSoundProperty.value = mainVolume;
            LOG_FN_DEBUG("type:value",mainSoundProperty.type,mainSoundProperty.value);
            int16_t oldValue;
            pSink->getMainSoundPropertyValue(mainSoundProperty.type,oldValue);
            if(oldValue!= mainSoundProperty.value)
            {
                result = mpControlReceive->changeMainSinkSoundPropertyDB(
                                mainSoundProperty,pSink->getID());
            }
        }
    }
    return result;
}

am_Error_e CAmClassElement::_setLastMainConnectionVolume(CAmMainConnectionElement* pMainConnection)
{
    if(pMainConnection != NULL)
    {
        std::vector<gc_LastMainConVolInfo_s>::iterator itlistLastMainConVolInfo;
        CAmSourceElement* pSourceElement = pMainConnection->getMainSource();
        CAmSinkElement* pSinkElement = pMainConnection->getMainSink();
        if ((NULL == pSinkElement) || (NULL == pSourceElement))
        {
            return E_NOT_POSSIBLE;
        }
        /*set last main connection volume if main source or main sink supports
         *volume persistence
         */
        if(pSourceElement->isVolumePersistencySupported() || pSinkElement->isVolumePersistencySupported())
        {
            for ( itlistLastMainConVolInfo = mLastMainConnectionsVolume.listLastMainConVolInfo.begin();
                            itlistLastMainConVolInfo != mLastMainConnectionsVolume.listLastMainConVolInfo.end(); itlistLastMainConVolInfo++)
            {
                //if main connection  is already exist then update only its main volume in the list
                if(itlistLastMainConVolInfo->mainConnectionName == pMainConnection->getName())
                {
                    LOG_FN_INFO(__FILENAME__,__func__,"main connection is exsist and its volume is :",pMainConnection->getMainVolume());
                    itlistLastMainConVolInfo->mainVolume = pMainConnection->getMainVolume();
                    return E_OK;
                }
            }
            //if main connection is not found in the list then consider it as new main connection and insert in to the list
            if(itlistLastMainConVolInfo == mLastMainConnectionsVolume.listLastMainConVolInfo.end())
            {
                LOG_FN_INFO(__FILENAME__,__func__,"new main connection : ",pMainConnection->getName(),"its volume :",pMainConnection->getMainVolume());
                gc_LastMainConVolInfo_s lastMainConVolInfo;
                lastMainConVolInfo.mainConnectionName = pMainConnection->getName();
                lastMainConVolInfo.mainVolume = pMainConnection->getMainVolume();
                mLastMainConnectionsVolume.listLastMainConVolInfo.push_back(lastMainConVolInfo);
                return E_OK;
            }
        }
        else
        {
            LOG_FN_INFO(__FILENAME__,__func__,"volume persistence not supported for main source:",
                        pSourceElement->getName(),"or main sink",pSinkElement->getName());
            return E_OK;
        }
    }
    else
    {
        LOG_FN_INFO(__FILENAME__,__func__,"invalid main connection");
        return E_NOT_POSSIBLE;
    }
}

} /* namespace gc */
} /* namespace am */
