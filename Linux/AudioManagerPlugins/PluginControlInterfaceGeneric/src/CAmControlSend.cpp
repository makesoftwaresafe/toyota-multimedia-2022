/******************************************************************************
 * @file: CAmControlSend.cpp
 *
 * This file contains the definition of control send class (member functions
 * and data members) used to implement the interface of AM daemon to allow the
 * hook from application and routing adaptor to reach the controller
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

#include "CAmControlSend.h"
#include "CAmRootAction.h"
#include "CAmLogger.h"
#include "CAmSystemElement.h"
#include "CAmDomainElement.h"
#include "CAmSinkElement.h"
#include "CAmGatewayElement.h"
#include "CAmClassElement.h"
#include "CAmMainConnectionElement.h"
#include "CAmTimerEvent.h"
#include <cassert>
#include "CAmSourceElement.h"
#include "IAmPolicySend.h"
#include "CAmPolicyReceive.h"
#include "limits.h"
#include "CAmPersistenceWrapper.h"

DltContext GenericControllerDlt;
namespace am {
namespace gc {
#define APPLICATION_NAME "genericController"
#define STATIC_ID_CONFIGURED(ConfiguredID) \
    ((ConfiguredID > 0) && (ConfiguredID < DYNAMIC_ID_BOUNDARY))
extern "C" IAmPolicySend* createPolicySendInterface();

extern "C" IAmControlSend* PluginControlInterfaceGenericFactory()
{
    return (new CAmControlSend());
}

/*previously the name of the function was not proper so changed as per the AM protocol
extern "C" void destroyControlPluginInterface(IAmControlSend* controlSendInterface)*/
extern "C" void destroyPluginControlInterfaceGeneric(IAmControlSend* controlSendInterface)
{
    delete controlSendInterface;
}

CAmControlSend::CAmControlSend() :
                                mpControlReceive(NULL),
                                mpPolicySend(NULL),
                                mdebugEnable( std::string("D"),
                                              std::string("debug"),
                   std::string("Start the controller in debug/development\
                             mode. In debug/development mode controller\
                             would supports additional features like read\
                             write system property etc."),false),
                             mpdomainRegiTimerCallback(this, &CAmControlSend::_domainRegistrationTimeout),
                             mDomainRegistrationTimerHandle(0)
{
    CAmCommandLineSingleton::instance()->add(mdebugEnable);
    CAmPersistenceWrapper::addCommandLineArgument();
}

CAmControlSend::~CAmControlSend()
{
    LOG_FN_ENTRY(__FILENAME__,__func__,"Destructor");
    am_Error_e result = E_OK;
    result  = _storeVolumetoPersistency();
    if(E_OK != result)
    {
        LOG_FN_WARN(__FILENAME__,__func__,"storing class level volume to persistence failed",result);
    }
    result  = _storeMainConnectiontoPersistency();
    if(E_OK != result)
    {
    	LOG_FN_WARN(__FILENAME__,__func__,"storing last main connections to persistence failed",result);
    }
    result  = _storeMainConnectionVolumetoPersistency();
    if(E_OK != result)
    {
        LOG_FN_WARN(__FILENAME__,__func__,"storing main connection level volume to persistence failed",result);
    }
    _freeMemory();
    IAmPersistence* pPersistence = CAmPersistenceWrapper::getInstance();
    result = pPersistence->close();
    if(E_OK != result)
    {
        LOG_FN_ERROR(__FILENAME__,__func__,"Persistence de-initialization failed",result);
    }
    //release the persistence library instance
    CAmPersistenceWrapper::freeInstance();
}

void CAmControlSend::getInterfaceVersion(std::string& version) const
{
    version = ControlVersion;
}

am_Error_e CAmControlSend::startupController(IAmControlReceive* controlReceiveInterface)
{
    CAmSocketHandler* pSocketHandler;
    am_Error_e result;

    assert(controlReceiveInterface);
    // Register context with DLT for logging purpose.
    LOG_FN_REGISTER_CONTEXT();

    LOG_FN_ENTRY(__FILENAME__,__func__,"A new context registered with DLT");

    // get socket handler context
    result = controlReceiveInterface->getSocketHandler(pSocketHandler);
    if ((NULL == pSocketHandler) || (E_OK != result))
    {
        LOG_FN_ERROR(__FILENAME__,__func__," Failed to get socket handler from AM", result);
        return (E_NOT_POSSIBLE);
    }

    // create timer object
    CAmTimerEvent* mpTimerEvent = CAmTimerEvent::getInstance();
    if (NULL == mpTimerEvent)
    {
        LOG_FN_ERROR(__FILENAME__,__func__," Not able to create Timer object");
        return (E_NOT_POSSIBLE);
    }
    mpTimerEvent->setSocketHandle(pSocketHandler, this);

    /**
     * Startup sequence should be as follows
     * 1. Instantiate Control Receive
     * 2. Instantiate Policy Send
     * 3. Instantiate Policy receive
     */
    mpControlReceive = new gc::CAmControlReceive(controlReceiveInterface);
    if (NULL == mpControlReceive)
    {
        _freeMemory();
        return (E_NOT_POSSIBLE);
    }

    mpPolicySend = createPolicySendInterface();
    if (NULL == mpPolicySend)
    {
        _freeMemory();
        LOG_FN_ERROR(__FILENAME__,__func__," Not able to create Policy Engine Receive object");
        return (E_NOT_POSSIBLE);
    }

    IAmPolicyReceive* pPolicyReceive = new CAmPolicyReceive(mpControlReceive, mpPolicySend);
    if (NULL == pPolicyReceive)
    {
        _freeMemory();
        LOG_FN_ERROR(__FILENAME__,__func__," Not able to create Policy Engine Receive object");
        return (E_NOT_POSSIBLE);
    }

    result = mpPolicySend->startupInterface(pPolicyReceive);
    if (E_OK != result)
    {
        _freeMemory();
        LOG_FN_ERROR(__FILENAME__,__func__," failed to start policy interface");
        return (E_NOT_POSSIBLE);
    }
    /*
     * register system properties and the list classes
     */
    gc_System_s systemElement;
    systemElement.name = SYSTEM_ELEMENT_NAME;
    systemElement.readOnly = !(mdebugEnable.getValue());
    LOG_FN_INFO(__FILENAME__,__func__,"READ_ONLY",systemElement.readOnly);
    if (E_OK == mpPolicySend->getListSystemProperties(systemElement.listSystemProperties))
    {
        CAmSystemElement* pSystemElement = CAmSystemFactory::createElement(systemElement,
                                                                 mpControlReceive);
        if (pSystemElement == NULL)
        {
            LOG_FN_ERROR(__FILENAME__,__func__,"system Element creation failed");
            return (E_NOT_POSSIBLE);
        }
    }
    std::vector<gc_Class_s> listClasses;
    std::vector<gc_Class_s>::iterator itListClasses;
    CAmClassElement* pClassElement;
    if (E_OK == mpPolicySend->getListClasses(listClasses))
    {
        for (itListClasses = listClasses.begin(); itListClasses != listClasses.end();
                        itListClasses++)
        {
            // create new class object for each class
            pClassElement = CAmClassFactory::createElement((*itListClasses),
                                                           mpControlReceive);
            if (NULL == pClassElement)
            {
                LOG_FN_ERROR(__FILENAME__,__func__," not able to create new class object for class=",
                             itListClasses->name);
                return (E_NOT_POSSIBLE);
            }
        }
    }
    /**
     * Check if user has configured the class with name default
     */
    pClassElement = CAmClassFactory::getElement(DEFAULT_CLASS_NAME);
    if( NULL == pClassElement )
    {
        gc_Class_s defaultClass;
        defaultClass.name = DEFAULT_CLASS_NAME;
        defaultClass.type = C_PLAYBACK;
        defaultClass.priority = DEFAULT_ELEMENT_PRIORITY;
        // create new class object for each class
        pClassElement = CAmClassFactory::createElement(defaultClass,
                                                       mpControlReceive);
        if (NULL == pClassElement)
        {
            LOG_FN_ERROR(__FILENAME__,__func__," not able to create default new");
        }
    }
    IAmPersistence* pPersistence = CAmPersistenceWrapper::getInstance();
    result = pPersistence->open(APPLICATION_NAME);
    if ( E_OK != result )
    {
        LOG_FN_ERROR(" Persistency initialization failed",result);
    }
    result = _restoreVolumefromPersistency();
    if ( E_OK != result )
    {
        LOG_FN_INFO(__FILENAME__,__func__," restore volume from persistence failed",result);
    }
    result = _restoreMainConnectionVolumefromPersistency();
    if ( E_OK != result )
    {
        LOG_FN_INFO(__FILENAME__,__func__," restore main connection volume from persistence failed",result);
    }
    iterateActions();
    return E_OK;
}

void CAmControlSend::setControllerReady()
{
    assert(NULL != mpControlReceive);

    mpControlReceive->setCommandReady();
    mpControlReceive->setRoutingReady();
    LOG_FN_INFO(__FILENAME__,__func__," Finished");
}

void CAmControlSend::setControllerRundown(const int16_t amSignal)
{
    assert(NULL != mpControlReceive);
    if (2 == amSignal)
    {
        mpControlReceive->confirmControllerRundown(E_UNKNOWN);
    }
    mpControlReceive->confirmControllerRundown(E_OK);

    LOG_FN_INFO(__FILENAME__,__func__,"  Finished");
}

am_Error_e CAmControlSend::hookUserConnectionRequest(const am_sourceID_t sourceID,
                                                     const am_sinkID_t sinkID,
                                                     am_mainConnectionID_t& mainConnectionID)
{
    CAmSinkElement* pSinkElement;
    CAmSourceElement* pSourceElement;
    CAmClassElement* pClassElement;
    am_Error_e result;
    std::vector < am_Route_s > listRoutes;
    LOG_FN_ENTRY(__FILENAME__,__func__,sourceID, sinkID);
    pSourceElement = CAmSourceFactory::getElement(sourceID);
    pSinkElement = CAmSinkFactory::getElement(sinkID);
    if ( (NULL == pSinkElement) || (NULL == pSourceElement))
    {
        return E_NON_EXISTENT;
    }
    mainConnectionID = 0;
    pClassElement = CAmClassFactory::getElement(pSourceElement->getName(), pSinkElement->getName());
    if (NULL == pClassElement)
    {
        LOG_FN_ERROR(__FILENAME__,__func__,"  got failed.");
        return (E_NOT_POSSIBLE);
    }
    result = mpControlReceive->getRoute(false, pSourceElement->getID(), pSinkElement->getID(),
                                        listRoutes);
    if (E_OK != result)
    {
        LOG_FN_ERROR(__FILENAME__,__func__," getRoute returned error:", result);
        return result;
    }
    if (true == listRoutes.empty())
    {
        LOG_FN_ERROR(__FILENAME__,__func__,"  couldn't getRoute. The list is empty");
        return (E_NOT_POSSIBLE);
    }
    result = pClassElement->createMainConnection(pSourceElement->getName(), pSinkElement->getName(),
                                                 mainConnectionID);
    if ((result != E_OK) && (E_ALREADY_EXISTS != result))
    {
        LOG_FN_ERROR(__FILENAME__,__func__,"Failed to create connection");
        return result;
    }
    result = E_OK;
    gc_ConnectTrigger_s* pconnectTrigger = new gc_ConnectTrigger_s;
    if (NULL == pconnectTrigger)
    {
        return E_NOT_POSSIBLE;
    }
    pconnectTrigger->className = pClassElement->getName();
    pconnectTrigger->sinkName = pSinkElement->getName();
    pconnectTrigger->sourceName = pSourceElement->getName();
    CAmTriggerQueue::getInstance()->queue(USER_CONNECTION_REQUEST,pconnectTrigger);
    iterateActions();
    return result;
}

am_Error_e CAmControlSend::hookUserDisconnectionRequest(const am_mainConnectionID_t connectionID)
{
    CAmMainConnectionElement* pMainConnection;
    CAmClassElement* pClassElement;

    pMainConnection = CAmMainConnectionFactory::getElement(connectionID);
    if (NULL == pMainConnection)
    {
        LOG_FN_ERROR(__FILENAME__,__func__,"  no such connection:", connectionID);
        return E_NOT_POSSIBLE;
    }
    LOG_FN_INFO(__FILENAME__,__func__,pMainConnection->getName(), pMainConnection->getID());
    pClassElement = CAmClassFactory::getElementByConnection(pMainConnection->getName());
    if (NULL == pClassElement)
    {
        LOG_FN_ERROR(__FILENAME__,__func__,"  no such class:", connectionID);
        return E_NOT_POSSIBLE;
    }
    // Store the trigger in a Queue
    gc_DisconnectTrigger_s* triggerData = new gc_DisconnectTrigger_s;
    if (NULL == triggerData)
    {
        LOG_FN_ERROR(__FILENAME__,__func__,"  bad memory state:", connectionID);
        return E_NOT_POSSIBLE;
    }
    triggerData->className = pClassElement->getName();
    triggerData->sourceName = pMainConnection->getMainSourceName();
    triggerData->sinkName = pMainConnection->getMainSinkName();
    CAmTriggerQueue::getInstance()->queue(USER_DISCONNECTION_REQUEST, triggerData);
    iterateActions();
    return E_OK;
}

am_Error_e CAmControlSend::hookUserSetMainSinkSoundProperty(
                const am_sinkID_t sinkID, const am_MainSoundProperty_s& soundProperty)
{
    CAmSinkElement* pElement;
    am_Error_e result;
    am_MainSoundProperty_s localMainSoundProperty;

    LOG_FN_ENTRY(__FILENAME__,__func__,sinkID);
    localMainSoundProperty = soundProperty;

    // check if the sinkID is valid
    pElement = CAmSinkFactory::getElement(sinkID);
    if (NULL == pElement)
    {
        LOG_FN_ERROR(__FILENAME__,__func__,"  Not able to get sink element");
        return (E_OUT_OF_RANGE);
    }

    //Check the SP Type and Range
    // Check the SP range for requested sink from configuration and return with
    // same value if soundProperty is within the range else return with saturated min/max value.
    result = pElement->saturateMainSoundPropertyRange(localMainSoundProperty.type,
                                                      localMainSoundProperty.value);

    if (E_OK != result)
    {
        LOG_FN_ERROR(__FILENAME__,__func__,"  Not able to saturate property. Type->value", localMainSoundProperty.type,
                     localMainSoundProperty.value);
        return result;
    }
    // Store the trigger in a Queue
    gc_SinkSoundPropertyTrigger_s* triggerData = new gc_SinkSoundPropertyTrigger_s;
    if (NULL == triggerData)
    {
        LOG_FN_ERROR(__FILENAME__,__func__,"  bad memory state:");
        return E_NOT_POSSIBLE;
    }
    triggerData->sinkName = pElement->getName();
    triggerData->mainSoundProperty.type = localMainSoundProperty.type;
    triggerData->mainSoundProperty.value = localMainSoundProperty.value;
    CAmTriggerQueue::getInstance()->queue(USER_SET_SINK_MAIN_SOUND_PROPERTY, triggerData);
    LOG_FN_EXIT(__FILENAME__,__func__);
    iterateActions();
    return (E_OK);
}

am_Error_e CAmControlSend::hookUserSetMainSourceSoundProperty(
                const am_sourceID_t sourceID, const am_MainSoundProperty_s& soundProperty)
{
    CAmSourceElement* pElement;
    am_Error_e result;
    am_MainSoundProperty_s localMainSoundProperty;

    LOG_FN_ENTRY(__FILENAME__,__func__,sourceID);
    localMainSoundProperty = soundProperty;

    // check if the sourceID is valid
    pElement = CAmSourceFactory::getElement(sourceID);
    if (NULL == pElement)
    {
        LOG_FN_ERROR(__FILENAME__,__func__,"  Not able to get source element");
        return (E_OUT_OF_RANGE);
    }

    //Check the SP Type and Range
    // Check the SP range for requested source from configuration and return with
    // same value if soundProperty is within the range else return with saturated min/max value.
    result = pElement->saturateMainSoundPropertyRange(localMainSoundProperty.type,
                                                      localMainSoundProperty.value);

    if (E_OK != result)
    {
        LOG_FN_ERROR(__FILENAME__,__func__,"  Not able to saturate property. Type->value", localMainSoundProperty.type,
                     localMainSoundProperty.value);
        return result;
    }

    // Store the trigger in a Queue
    gc_SourceSoundPropertyTrigger_s* triggerData = new gc_SourceSoundPropertyTrigger_s;
    if (NULL == triggerData)
    {
        LOG_FN_ERROR(__FILENAME__,__func__,"  bad memory state:");
        return E_NOT_POSSIBLE;
    }
    triggerData->sourceName = pElement->getName();
    triggerData->mainSoundProperty.type = localMainSoundProperty.type;
    triggerData->mainSoundProperty.value = localMainSoundProperty.value;
    CAmTriggerQueue::getInstance()->queue(USER_SET_SOURCE_MAIN_SOUND_PROPERTY, triggerData);
    iterateActions();
    LOG_FN_EXIT(__FILENAME__,__func__);
    return (E_OK);
}

am_Error_e CAmControlSend::hookUserSetSystemProperty(const am_SystemProperty_s& property)
{
    LOG_FN_ENTRY(__FILENAME__,__func__,"type=",property.type,"value=",property.value);
    am_Error_e result;

    CAmSystemElement* pSystem = CAmSystemFactory::getElement(SYSTEM_ELEMENT_NAME);
    if(true == pSystem->isSystemPropertyReadOnly())
    {
        LOG_FN_EXIT(__FILENAME__,__func__,"System Property read only ");
        return  E_NOT_POSSIBLE;
    }
    // Store the trigger in a Queue
    gc_SystemPropertyTrigger_s* triggerData = new gc_SystemPropertyTrigger_s;
    if (NULL == triggerData)
    {
        LOG_FN_EXIT(__FILENAME__,__func__,"  bad memory state:");
        return E_NOT_POSSIBLE;
    }
    triggerData->systemProperty = property;
    CAmTriggerQueue::getInstance()->queue(USER_SET_SYSTEM_PROPERTY, triggerData);
    iterateActions();
    LOG_FN_EXIT(__FILENAME__,__func__);
    return (E_OK);

}

am_Error_e CAmControlSend::hookUserVolumeChange(const am_sinkID_t sinkID,
                                                const am_mainVolume_t mainVolume)
{
    LOG_FN_ENTRY(__FILENAME__,__func__,sinkID, mainVolume);
    am_Error_e result(E_NOT_POSSIBLE);

    CAmSinkElement* pElement = CAmSinkFactory::getElement(sinkID);
    if (NULL == pElement)
    {
        LOG_FN_ERROR(__FILENAME__,__func__,"  parameter error");
        return E_OUT_OF_RANGE;
    }

    // Store the trigger in a Queue
    gc_SinkVolumeChangeTrigger_s* triggerData = new gc_SinkVolumeChangeTrigger_s;
    if (NULL == triggerData)
    {
        LOG_FN_ERROR(__FILENAME__,__func__,"  bad memory state:");
        return E_NOT_POSSIBLE;
    }

    triggerData->sinkName = pElement->getName();
    triggerData->volume = mainVolume;
    CAmTriggerQueue::getInstance()->queue(USER_SET_VOLUME, triggerData);
    iterateActions();
    LOG_FN_EXIT(__FILENAME__,__func__);
    return E_OK;
}

am_Error_e CAmControlSend::hookUserVolumeStep(const am_sinkID_t sinkID, const int16_t increment)
{
    LOG_FN_ENTRY(__FILENAME__,__func__,sinkID, increment);

    CAmSinkElement* pElement = CAmSinkFactory::getElement(sinkID);
    if (NULL == pElement)
    {
        LOG_FN_ERROR(__FILENAME__,__func__,"  parameter error");
        return (E_OUT_OF_RANGE);
    }

    //get the user volume and add increment point in it.
    am_mainVolume_t mainVolume;
    mainVolume = pElement->getMainVolume();
    mainVolume+=increment;
    // Store the trigger in a Queue
    gc_SinkVolumeChangeTrigger_s* triggerData = new gc_SinkVolumeChangeTrigger_s;
    if (NULL == triggerData)
    {
        LOG_FN_ERROR(__FILENAME__,__func__,"  bad memory state:");
        return E_NOT_POSSIBLE;
    }
    triggerData->sinkName = pElement->getName();
    triggerData->volume = mainVolume;
    CAmTriggerQueue::getInstance()->queue(USER_SET_VOLUME, triggerData);
    iterateActions();
    LOG_FN_EXIT(__FILENAME__,__func__);
    return (E_OK);
}

am_Error_e CAmControlSend::hookUserSetSinkMuteState(const am_sinkID_t sinkID,
                                                    const am_MuteState_e muteState)
{
    CAmElement* pElement;

    LOG_FN_ENTRY(__FILENAME__,__func__,sinkID);
    // check parameter
    if ((MS_MUTED != muteState) && (MS_UNMUTED != muteState))
    {
        LOG_FN_ERROR(__FILENAME__,__func__,"  parameter error");
        return (E_OUT_OF_RANGE);
    }

    // check if the sinkID is valid
    pElement = CAmSinkFactory::getElement(sinkID);
    if (NULL == pElement)
    {
        LOG_FN_ERROR(__FILENAME__,__func__,"  Not able to get sink element");
        return (E_OUT_OF_RANGE);
    }
    // Store the trigger in a Queue
    gc_SinkMuteTrigger_s* triggerData = new gc_SinkMuteTrigger_s;
    if (NULL == triggerData)
    {
        LOG_FN_ERROR(__FILENAME__,__func__,"  bad memory state:");
        return E_NOT_POSSIBLE;
    }
    triggerData->sinkName = pElement->getName();
    triggerData->muteState = muteState;
    //mute state will be taken care at policy send side.
    CAmTriggerQueue::getInstance()->queue(USER_SET_SINK_MUTE_STATE, triggerData);
    iterateActions();
    LOG_FN_EXIT(__FILENAME__,__func__);
    return (E_OK);
}

am_Error_e CAmControlSend::hookSystemRegisterDomain(const am_Domain_s& domainData,
                                                    am_domainID_t& domainID)
{
    LOG_FN_ENTRY(__FILENAME__,__func__,domainData.name);
    am_Error_e result(E_NOT_POSSIBLE);
    std::vector < gc_Domain_s > listDomainInfo;
    std::vector < std::string > listNames {domainData.name};
    result = mpPolicySend->getListElements(listNames, listDomainInfo);
    if (E_OK == result)
    {
        gc_Domain_s domainInfo = listDomainInfo.front();
        domainInfo.domainID = listDomainInfo.front().domainID;
        domainInfo.busname = domainData.busname;
        domainInfo.nodename = domainData.nodename;
        domainInfo.early = domainData.early;
        domainInfo.complete = domainData.complete;
        domainInfo.state = domainData.state;
        CAmElement* pElement = CAmDomainFactory::createElement(domainInfo, mpControlReceive);
        if (NULL != pElement )
        {
            domainID = pElement->getID();
            gc_RegisterElementTrigger_s* pregisterdomainTrigger = new gc_RegisterElementTrigger_s;
            if (NULL == pregisterdomainTrigger)
            {
                return E_NOT_POSSIBLE;
            }
            pregisterdomainTrigger->elementName = domainData.name;
            pregisterdomainTrigger->RegisterationStatus = result;
            CAmTriggerQueue::getInstance()->queue(SYSTEM_REGISTER_DOMAIN, pregisterdomainTrigger);
            LOG_FN_INFO(__FILENAME__,__func__,"  registered domain Name->result =", domainData.name, result);
            result = E_OK;
        }
    }
    iterateActions();
    LOG_FN_EXIT(__FILENAME__,__func__);
    return result;
}

am_Error_e CAmControlSend::hookSystemDeregisterDomain(const am_domainID_t domainID)
{
    am_Error_e result;
    std::vector < am_sinkID_t > listSinkIDs;
    std::vector<am_sinkID_t >::iterator itListSinkIDs;
    std::vector < am_sourceID_t > listSourceIDs;
    std::vector<am_sourceID_t >::iterator itListSourceIDs;
    std::vector < am_gatewayID_t > listGatewaysIDs;
    std::vector<am_gatewayID_t >::iterator itListGatewayIDs;
    am_Domain_s DomainInfo;
    LOG_FN_ENTRY(__FILENAME__,__func__,domainID);

    result = mpControlReceive->getDomainInfoByID(domainID, DomainInfo);
    if (E_OK == result)
    {
        //Get the list of gatewayID belongs to specific domain and also unregister them from the AM DB.
        result = mpControlReceive->getListGatewaysOfDomain(domainID, listGatewaysIDs);
        if ((E_OK == result) && (false == listGatewaysIDs.empty()))
        {
            for (itListGatewayIDs = listGatewaysIDs.begin();
                            itListGatewayIDs != listGatewaysIDs.end(); itListGatewayIDs++)
            {
                CAmGatewayFactory::destroyElement(*itListGatewayIDs);
            }
        }
        //Get the list of sinkID belongs to specific domain and also unregister them from the AM DB.
        result = mpControlReceive->getListSinksOfDomain(domainID, listSinkIDs);
        if ((E_OK == result) && (false == listSinkIDs.empty()))
        {
            for (itListSinkIDs = listSinkIDs.begin(); itListSinkIDs != listSinkIDs.end();
                            itListSinkIDs++)
            {
                CAmSinkFactory::destroyElement(*itListSinkIDs);
            }
        }
        //Get the list of sourceID belongs to specific domain and also unregister them from the AM DB.
        result = mpControlReceive->getListSourcesOfDomain(domainID, listSourceIDs);
        if ((E_OK == result) && (false == listSourceIDs.empty()))
        {
            for (itListSourceIDs = listSourceIDs.begin(); itListSourceIDs != listSourceIDs.end();
                            itListSourceIDs++)
            {
                CAmSourceFactory::destroyElement(*itListSourceIDs);
            }
        }

        result = CAmDomainFactory::destroyElement(domainID);
        if (E_OK == result)
        {
            gc_UnRegisterElementTrigger_s* unRegisterTrigger = new gc_UnRegisterElementTrigger_s;
            if (NULL == unRegisterTrigger)
            {
                LOG_FN_ERROR(__FILENAME__,__func__,"  bad memory state:");
                return E_NOT_POSSIBLE;
            }
            unRegisterTrigger->elementName = DomainInfo.name;
            unRegisterTrigger->unRegisterationStatus = result;
            CAmTriggerQueue::getInstance()->queue(SYSTEM_DEREGISTER_DOMAIN, unRegisterTrigger);
        }
    }
    LOG_FN_EXIT(__FILENAME__,__func__," Error:", result);
    iterateActions();
    return result;

}

void CAmControlSend::hookSystemDomainRegistrationComplete(const am_domainID_t domainID)
{
    LOG_FN_INFO(__FILENAME__,__func__);
    am_Domain_s domainInfo;
    am_Error_e result;
    LOG_FN_ENTRY(__FILENAME__,__func__,domainID);
    if ((NULL == mpControlReceive))
    {
        LOG_FN_ERROR(__FILENAME__,__func__,"Database handler error ");
        return;
    }
    result = mpControlReceive->getDomainInfoByID(domainID, domainInfo);
    if (E_OK == result)
    {
        domainInfo.complete = true;
        domainInfo.domainID = 0;
        mpControlReceive->enterDomainDB(domainInfo, domainInfo.domainID);
        gc_DomainRegisterationCompleteTrigger_s* registrationCompleteTrigger = new gc_DomainRegisterationCompleteTrigger_s;
        if (NULL == registrationCompleteTrigger)
        {
            LOG_FN_ERROR(__FILENAME__,__func__,"  bad memory state:");
            return;
        }
        registrationCompleteTrigger->domainName = domainInfo.name;
        CAmTriggerQueue::getInstance()->queue(SYSTEM_DOMAIN_REGISTRATION_COMPLETE,
                                              registrationCompleteTrigger);
    }
    else
    {
        LOG_FN_ERROR(__FILENAME__,__func__,"domain not found :",domainID);
        return;
    }
    /*check if all domains are registered*/
    if(true == _checkAllDomainRegistered())
    {
        /* remove domain registration complete timer */
        if (mDomainRegistrationTimerHandle)
        {
            CAmTimerEvent* pTimer = CAmTimerEvent::getInstance();
            pTimer->removeTimer(mDomainRegistrationTimerHandle);
        }
        result = _restoreConnectionsFromPersistency();
        gc_AllDomainRegisterationCompleteTrigger_s* pdomainRegistrationTrigger = new gc_AllDomainRegisterationCompleteTrigger_s;
        if (NULL == pdomainRegistrationTrigger)
        {
            LOG_FN_ERROR(__FILENAME__,__func__,"  bad memory state:");
            return;
        }
        if (E_OK == result)
        {
            pdomainRegistrationTrigger->status = E_OK;
        }
        else
        {
            pdomainRegistrationTrigger->status = E_DATABASE_ERROR;
        }
        CAmTriggerQueue::getInstance()->queue(SYSTEM_ALL_DOMAIN_REGISTRATION_COMPLETE,pdomainRegistrationTrigger);
    }
    else
    {
        LOG_FN_INFO(__FILENAME__,__func__,"All domain not registered yet");
    }
    iterateActions();
    LOG_FN_EXIT(__FILENAME__,__func__);
    return;
}

am_Error_e CAmControlSend::hookSystemRegisterSink(const am_Sink_s& sinkData, am_sinkID_t& sinkID)
{
    am_Error_e result = E_NOT_POSSIBLE;
    CAmElement* pElement;
    std::vector < gc_Sink_s > listSinks;
    std::vector < std::string > listNames;
    gc_Sink_s sinkInfo;
    CAmClassElement* pClassElement = NULL;
    listNames.push_back(sinkData.name);
    LOG_FN_ENTRY(__FILENAME__,__func__," sink=", sinkData.name);
    // First check with the policy engine if this sink is allowed
    if (E_OK == mpPolicySend->getListElements(listNames, listSinks))
    {
        sinkInfo = listSinks.front();
        pClassElement = CAmClassFactory::getElementBySinkClassID(
                sinkData.sinkClassID);
        if ( NULL == pClassElement)
        {
            pClassElement = CAmClassFactory::getElement(sinkInfo.className);
        }
    }
    else
    {
        CAmSystemElement* pSystemElement = CAmSystemFactory::getElement(SYSTEM_ELEMENT_NAME);
        if ( (NULL != pSystemElement) &&
             (true ==  pSystemElement->isUnknownElementRegistrationSupported()))
        {
            pClassElement = CAmClassFactory::getElementBySinkClassID(
                    sinkData.sinkClassID);
            if ( NULL == pClassElement)
            {
                pClassElement = CAmClassFactory::getElement(DEFAULT_CLASS_NAME);
            }
        }
    }
    if (NULL == pClassElement)
    {
        LOG_FN_ERROR(__FILENAME__,__func__,"unknown sink found");
    }
    else
    {
        sinkInfo.sinkClassID = pClassElement->getSinkClassID();
        sinkInfo.domainID = sinkData.domainID;
        sinkInfo.name = sinkData.name;
        sinkInfo.volume = sinkData.volume;
        sinkInfo.visible = sinkData.visible;
        sinkInfo.available = sinkData.available;
        sinkInfo.listSoundProperties = sinkData.listSoundProperties;
        sinkInfo.listConnectionFormats = sinkData.listConnectionFormats;
        sinkInfo.listMainSoundProperties = sinkData.listMainSoundProperties;
        /*
         * over write the configuration data with the dynamic data
         */
        std::vector <CAmClassElement*>::iterator itListClassElements;
        std::vector <CAmClassElement*> listClassElements;
        CAmClassFactory::getListElements(listClassElements);
        itListClassElements = listClassElements.begin();
        for(;itListClassElements!=listClassElements.end();++itListClassElements)
        {
            am_mainVolume_t mainVolume = sinkInfo.mainVolume;
            if((true == (*itListClassElements)->isPerSinkClassVolumeEnabled()) &&
               (E_OK == (*itListClassElements)->getLastVolume(sinkInfo.name, mainVolume)))
                {
                    gc_MainSoundProperty_s mainSoundProperty;
                    mainSoundProperty.type = MSP_SINK_PER_CLASS_VOLUME_TYPE((*itListClassElements)->getID());
                    mainSoundProperty.value = mainVolume;
                    mainSoundProperty.minValue = SHRT_MIN;
                    mainSoundProperty.maxValue = SHRT_MAX;
                    sinkInfo.listGCMainSoundProperties.push_back(mainSoundProperty);
                    sinkInfo.listMainSoundProperties.push_back(mainSoundProperty);
                }
        }

        sinkInfo.listMainNotificationConfigurations =
                sinkData.listMainNotificationConfigurations;
        sinkInfo.listNotificationConfigurations =
                sinkData.listNotificationConfigurations;
        if (!(!STATIC_ID_CONFIGURED(sinkData.sinkID) &&
             STATIC_ID_CONFIGURED(sinkInfo.sinkID)))
        {
            sinkInfo.sinkID = sinkData.sinkID;
        }
        sinkInfo.muteState = sinkData.muteState;
        sinkInfo.mainVolume = sinkData.mainVolume;
        // This sink might have been created during the gateway registration.Please check locally.
        pElement = CAmSinkFactory::createElement(sinkInfo, mpControlReceive);
        if (NULL != pElement)
        {
            sinkID = pElement->getID();
            result = pClassElement->attach(pElement);
            if(result != E_OK)
            {
                /*its an error need to decide */
                LOG_FN_ERROR("element attach failed, result is:",result);
            }
        }
        else
        {
            LOG_FN_INFO(__FILENAME__,__func__,"Not able to create sink element", sinkData.name);
            result = E_NOT_POSSIBLE;
        }
        gc_RegisterElementTrigger_s* registerTrigger =
                new gc_RegisterElementTrigger_s;
        if (NULL == registerTrigger)
        {
            LOG_FN_ERROR(__FILENAME__,__func__,"bad memory state:");
            return E_NOT_POSSIBLE;
        }
        registerTrigger->elementName = sinkData.name;
        registerTrigger->RegisterationStatus = result;
        CAmTriggerQueue::getInstance()->queue(SYSTEM_REGISTER_SINK,
                registerTrigger);
        LOG_FN_INFO(__FILENAME__,__func__,"registered sink name:result=", sinkData.name, result);
    }
    LOG_FN_EXIT(__FILENAME__,__func__);
    iterateActions();
    return result;
}

am_Error_e CAmControlSend::hookSystemDeregisterSink(const am_sinkID_t sinkID)
{
    am_Error_e result;
    std::string name;
    CAmElement* pElement;

    LOG_FN_ENTRY(__FILENAME__,__func__,sinkID);
    pElement = CAmSinkFactory::getElement(sinkID);
    if ( NULL == pElement )
    {
        LOG_FN_ERROR(__FILENAME__,__func__,"Invalid Sink Element ");
        return E_NOT_POSSIBLE;
    }
    name = pElement->getName();
    result = CAmSinkFactory::destroyElement(sinkID);
    // remove the sink from DB
    if (E_OK == result)
    {
        gc_UnRegisterElementTrigger_s* unRegisterTrigger = new gc_UnRegisterElementTrigger_s;
        if (NULL == unRegisterTrigger)
        {
            LOG_FN_ERROR(__FILENAME__,__func__,"  bad memory state:");
            return E_NOT_POSSIBLE;
        }
        unRegisterTrigger->elementName = name;
        unRegisterTrigger->unRegisterationStatus = E_OK;
        CAmTriggerQueue::getInstance()->queue(SYSTEM_DEREGISTER_SINK, unRegisterTrigger);
        LOG_FN_INFO(__FILENAME__,__func__,"  unregistered sink =", sinkID);
    }
    else
    {
        LOG_FN_ERROR(__FILENAME__,__func__," Error while removing sink from DB. sinkId:Error", sinkID, result);
    }
    LOG_FN_EXIT(__FILENAME__,__func__);
    iterateActions();
    return result;
}

am_Error_e CAmControlSend::hookSystemRegisterSource(const am_Source_s& sourceData,
                                                    am_sourceID_t& sourceID)
{
    am_Error_e result = E_NOT_POSSIBLE;
    CAmElement* pElement;
    gc_Source_s sourceInfo;
    std::vector < gc_Source_s > listSources;
    std::vector < std::string > listNames;
    CAmClassElement* pClassElement = NULL;

    LOG_FN_ENTRY(__FILENAME__,__func__," source=", sourceData.name);
    listNames.push_back(sourceData.name);

    // First check with the policy engine if this sink is allowed
    if (E_OK == mpPolicySend->getListElements(listNames, listSources))
    {
        /*
         * Already known sink
         * 1. If valid classId use it else
         * 2. use from configuration
         */
        sourceInfo = listSources.front();
        pClassElement = CAmClassFactory::getElementBySourceClassID(
                sourceData.sourceClassID);
        if (pClassElement == NULL)
        {
            pClassElement = CAmClassFactory::getElement(sourceInfo.className);
        }
    }
    else
    {
        CAmSystemElement* pSystemElement = CAmSystemFactory::getElement(SYSTEM_ELEMENT_NAME);
        if( (NULL != pSystemElement) &&
            (true == pSystemElement->isUnknownElementRegistrationSupported()))
        {
            /*
             * element not present in the configuration
             */
            pClassElement = CAmClassFactory::getElementBySourceClassID(
                    sourceData.sourceClassID);
            if (NULL == pClassElement)
            {
                pClassElement = CAmClassFactory::getElement(DEFAULT_CLASS_NAME);
            }
        }
    }
    if (NULL == pClassElement)
    {
        LOG_FN_ERROR(__FILENAME__,__func__,"Could not determine source class ID");
    }
    else
    {
        sourceInfo.sourceClassID = pClassElement->getSinkClassID();
        sourceInfo.domainID = sourceData.domainID;
        sourceInfo.name = sourceData.name;
        sourceInfo.volume = sourceData.volume;
        sourceInfo.visible = sourceData.visible;
        sourceInfo.available = sourceData.available;
        sourceInfo.listSoundProperties = sourceData.listSoundProperties;
        sourceInfo.listConnectionFormats = sourceData.listConnectionFormats;
        sourceInfo.listMainSoundProperties = sourceData.listMainSoundProperties;
        sourceInfo.listMainNotificationConfigurations =
                sourceData.listMainNotificationConfigurations;
        sourceInfo.listNotificationConfigurations =
                sourceData.listNotificationConfigurations;
        /*
         * In case the router registers with id = 0 , but the controller
         * configuration has the static configuration then use controller
         * configuration else overwrite controller's ID
         */
        if (!(!(STATIC_ID_CONFIGURED(sourceData.sourceID)) &&
             STATIC_ID_CONFIGURED(sourceInfo.sourceID)))
        {
            sourceInfo.sourceID = sourceData.sourceID;
        }
        sourceInfo.sourceState = sourceData.sourceState;
        sourceInfo.interruptState = sourceData.interruptState;

        // This sink might have been created during the gateway registration.Please check locally.
        pElement = CAmSourceFactory::createElement(sourceInfo, mpControlReceive);
        if (NULL != pElement)
        {
            sourceID = pElement->getID();
            result = pClassElement->attach(pElement);
            if(result != E_OK)
            {
                /*its an error need to decide */
                LOG_FN_ERROR("element attach failed, result is:",result);
            }
        } else
        {
            LOG_FN_INFO(__FILENAME__,__func__,"Not able to create source element", sourceData.name);
            result = E_NOT_POSSIBLE;
        }
        gc_RegisterElementTrigger_s* registerTrigger =
                new gc_RegisterElementTrigger_s;
        if (NULL == registerTrigger)
        {
            LOG_FN_ERROR(__FILENAME__,__func__,"bad memory state:");
            return E_NOT_POSSIBLE;
        }
        registerTrigger->elementName = sourceData.name;
        registerTrigger->RegisterationStatus = result;
        CAmTriggerQueue::getInstance()->queue(SYSTEM_REGISTER_SOURCE,
                registerTrigger);
        LOG_FN_INFO(__FILENAME__,__func__,"registered source name:result=", sourceData.name, result);
    }
    LOG_FN_EXIT(__FILENAME__,__func__);
    iterateActions();
    return result;
}

am_Error_e CAmControlSend::hookSystemDeregisterSource(const am_sourceID_t sourceID)
{
    am_Error_e result;
    std::string name;
    CAmElement* pElement;

    LOG_FN_ENTRY(__FILENAME__,__func__,sourceID);
    pElement = CAmSourceFactory::getElement(sourceID);
    if (NULL == pElement)
    {
        LOG_FN_ERROR(__FILENAME__,__func__,"Invalid Source Element ");
        return E_NOT_POSSIBLE;
    }
    name = pElement->getName();
    result = CAmSourceFactory::destroyElement(sourceID);
    // remove the sink from DB
    if (E_OK == result)
    {
        gc_UnRegisterElementTrigger_s* unRegisterTrigger = new gc_UnRegisterElementTrigger_s;
        if (NULL == unRegisterTrigger)
        {
            LOG_FN_ERROR(__FILENAME__,__func__,"  bad memory state:");
            return E_NOT_POSSIBLE;
        }
        unRegisterTrigger->elementName = name;
        unRegisterTrigger->unRegisterationStatus = E_OK;
        CAmTriggerQueue::getInstance()->queue(SYSTEM_DEREGISTER_SOURCE, unRegisterTrigger);
        LOG_FN_INFO(__FILENAME__,__func__,"  unregistered source =", sourceID);
    }
    else
    {
        LOG_FN_ERROR(__FILENAME__,__func__," Error while removing sink from DB. sourceId:Error", sourceID, result);
    }
    LOG_FN_EXIT(__FILENAME__,__func__);
    iterateActions();
    return result;
}

am_Error_e CAmControlSend::hookSystemRegisterGateway(const am_Gateway_s& gatewayData,
                                                     am_gatewayID_t& gatewayID)
{
    am_Error_e result = E_NOT_POSSIBLE;
    CAmElement* pElement;
    gc_Gateway_s gatewayInfo;
    std::vector < gc_Gateway_s > listGateways;
    std::vector < std::string > listNames;
    LOG_FN_ENTRY(__FILENAME__,__func__," gateway=", gatewayData.name);
    listNames.push_back(gatewayData.name);
    if (E_OK == mpPolicySend->getListElements(listNames, listGateways))
    {
        gatewayInfo = listGateways.front();
        gatewayInfo.gatewayID = gatewayData.gatewayID;
        gatewayInfo.name = gatewayData.name;
        gatewayInfo.sinkID = gatewayData.sinkID;
        gatewayInfo.sourceID = gatewayData.sourceID;
        gatewayInfo.domainSinkID = gatewayData.domainSinkID;
        gatewayInfo.domainSourceID = gatewayData.domainSourceID;
        gatewayInfo.controlDomainID = gatewayData.controlDomainID;
        gatewayInfo.listSourceFormats = gatewayData.listSourceFormats;
        gatewayInfo.listSinkFormats = gatewayData.listSinkFormats;
        gatewayInfo.convertionMatrix = gatewayData.convertionMatrix;
        // This sink might have been created during the gateway registration.Please check locally.
        pElement = CAmGatewayFactory::createElement(gatewayInfo, mpControlReceive);
        if (NULL != pElement)
        {
            gatewayID = pElement->getID();
            result = E_OK;
        }
        else
        {
            LOG_FN_INFO(__FILENAME__,__func__," Not able to create source element", gatewayData.name);
            result = E_NOT_POSSIBLE;
        }
        gc_RegisterElementTrigger_s* registerTrigger = new gc_RegisterElementTrigger_s;
        if (NULL == registerTrigger)
        {
            LOG_FN_ERROR(__FILENAME__,__func__,"  bad memory state:");
            return E_NOT_POSSIBLE;
        }
        registerTrigger->elementName = gatewayData.name;
        registerTrigger->RegisterationStatus = result;
        CAmTriggerQueue::getInstance()->queue(SYSTEM_REGISTER_GATEWAY, registerTrigger);
        LOG_FN_INFO(__FILENAME__,__func__,"  registered gateway name:result=", gatewayData.name, result);
    }
    LOG_FN_EXIT(__FILENAME__,__func__);
    iterateActions();
    return result;
}

am_Error_e CAmControlSend::hookSystemDeregisterGateway(const am_gatewayID_t gatewayID)
{
    am_Error_e result;
    std::string name;
    CAmElement* pElement;

    LOG_FN_ENTRY(__FILENAME__,__func__,gatewayID);
    pElement = CAmGatewayFactory::getElement(gatewayID);
    if (NULL == pElement)
    {
        LOG_FN_ERROR(__FILENAME__,__func__,"Invalid Gateway Element ");
        return E_NOT_POSSIBLE;
    }
    name = pElement->getName();
    result = CAmGatewayFactory::destroyElement(gatewayID);
    if (E_OK == result)
    {
        gc_UnRegisterElementTrigger_s* unRegisterTrigger = new gc_UnRegisterElementTrigger_s;
        if (NULL == unRegisterTrigger)
        {
            LOG_FN_ERROR(__FILENAME__,__func__,"  bad memory state:");
            return E_NOT_POSSIBLE;
        }
        unRegisterTrigger->elementName = name;
        unRegisterTrigger->unRegisterationStatus = E_OK;
        CAmTriggerQueue::getInstance()->queue(SYSTEM_DEREGISTER_GATEWAY, unRegisterTrigger);
        LOG_FN_INFO(__FILENAME__,__func__,"  unregistered gatewayID =", gatewayID);
    }
    else
    {
        LOG_FN_ERROR(__FILENAME__,__func__," Error while removing sink from DB. gatewayID Error", gatewayID, result);
    }
    LOG_FN_EXIT(__FILENAME__,__func__);
    iterateActions();
    return result;
}

am_Error_e CAmControlSend::hookSystemRegisterCrossfader(const am_Crossfader_s& crossFaderData,
                                                        am_crossfaderID_t& crossFaderID)
{
    LOG_FN_ENTRY(__FILENAME__,__func__);
    am_Error_e err = mpControlReceive->enterCrossfaderDB(crossFaderData, crossFaderID);
    iterateActions();
    return err;
}

am_Error_e CAmControlSend::hookSystemDeregisterCrossfader(const am_crossfaderID_t crossFaderID)
{
    am_Error_e err = mpControlReceive->removeCrossfaderDB(crossFaderID);
    iterateActions();
    return err;
}

void CAmControlSend::hookSystemSinkVolumeTick(const am_Handle_s handle, const am_sinkID_t sinkID,
                                              const am_volume_t volume)
{
    (void)handle;
    (void)sinkID;
    (void)volume;
    iterateActions();
}

void CAmControlSend::hookSystemSourceVolumeTick(const am_Handle_s handle,
                                                const am_sourceID_t sourceID,
                                                const am_volume_t volume)
{
    (void)handle;
    (void)sourceID;
    (void)volume;
    iterateActions();
}

void CAmControlSend::hookSystemInterruptStateChange(const am_sourceID_t sourceID,
                                                    const am_InterruptState_e interruptState)
{
    LOG_FN_ENTRY(__FILENAME__,__func__,sourceID);
    // get the source element by its ID
    CAmSourceElement* pElement = CAmSourceFactory::getElement(sourceID);
    if (NULL == pElement)
    {
        LOG_FN_ERROR(__FILENAME__,__func__,"  invalid sourceID=", sourceID);
        return;
    }
    pElement->setInterruptState(interruptState);
    // Store the trigger in a Queue
    gc_SourceInterruptChangeTrigger_s* triggerData = new gc_SourceInterruptChangeTrigger_s;
    if (NULL == triggerData)
    {
        LOG_FN_ERROR(__FILENAME__,__func__,"  bad memory state:");
        return;
    }
    triggerData->sourceName = pElement->getName();
    triggerData->interrptstate = interruptState;
    CAmTriggerQueue::getInstance()->queue(SYSTEM_INTERRUPT_STATE_CHANGED, triggerData);
    iterateActions();
    LOG_FN_EXIT(__FILENAME__,__func__);
}

void CAmControlSend::hookSystemSinkAvailablityStateChange(const am_sinkID_t sinkID,
                                                          const am_Availability_s& availability)
{
    LOG_FN_ENTRY(__FILENAME__,__func__,sinkID);
    mpControlReceive->changeSinkAvailabilityDB(availability, sinkID);
    // check if the sourceID is valid
    CAmElement* pElement = CAmSinkFactory::getElement(sinkID);
    if (NULL == pElement)
    {
        LOG_FN_ERROR(__FILENAME__,__func__,"   parameter error");
        return;
    }

    // Store the trigger in a Queue
    gc_AvailabilityChangeTrigger_s* triggerData = new gc_AvailabilityChangeTrigger_s;
    if (NULL == triggerData)
    {
        LOG_FN_ERROR(__FILENAME__,__func__,"  bad memory state:");
        return;
    }

    triggerData->elementName = pElement->getName();
    triggerData->availability.availability = availability.availability;
    triggerData->availability.availabilityReason = availability.availabilityReason;
    CAmTriggerQueue::getInstance()->queue(SYSTEM_SINK_AVAILABILITY_CHANGED, triggerData);
    iterateActions();
    LOG_FN_EXIT(__FILENAME__,__func__);
}

void CAmControlSend::hookSystemSourceAvailablityStateChange(
                const am_sourceID_t sourceID, const am_Availability_s& availabilityInstance)
{
    CAmElement* pElement;

    LOG_FN_ENTRY(__FILENAME__,__func__,sourceID);
    mpControlReceive->changeSourceAvailabilityDB(availabilityInstance, sourceID);
    // check if the sourceID is valid
    pElement = CAmSourceFactory::getElement(sourceID);
    if (NULL == pElement)
    {
        LOG_FN_ERROR(__FILENAME__,__func__,"   parameter error");
        return;
    }

    // Store the trigger in a Queue
    gc_AvailabilityChangeTrigger_s* triggerData = new gc_AvailabilityChangeTrigger_s;
    if (NULL == triggerData)
    {
        LOG_FN_ERROR(__FILENAME__,__func__,"  bad memory state:");
        return;
    }
    triggerData->elementName = pElement->getName();
    triggerData->availability.availability = availabilityInstance.availability;
    triggerData->availability.availabilityReason = availabilityInstance.availabilityReason;
    CAmTriggerQueue::getInstance()->queue(SYSTEM_SOURCE_AVAILABILITY_CHANGED, triggerData);
    iterateActions();
    LOG_FN_EXIT(__FILENAME__,__func__);
}

void CAmControlSend::hookSystemDomainStateChange(const am_domainID_t domainID,
                                                 const am_DomainState_e state)
{
    assert(NULL != mpControlReceive);
    LOG_FN_ENTRY(__FILENAME__,__func__);
    //update the domain state in DB
    mpControlReceive->changDomainStateDB(state, domainID);
}

void CAmControlSend::hookSystemReceiveEarlyData(const std::vector<am_EarlyData_s >& listData)
{
    (void)listData;
    iterateActions();
}

void CAmControlSend::hookSystemSpeedChange(const am_speed_t speed)
{
    (void)speed;
    iterateActions();
}

void CAmControlSend::hookSystemTimingInformationChanged(
                const am_mainConnectionID_t mainConnectionID, const am_timeSync_t syncTime)
{
    (void)mainConnectionID;
    (void)syncTime;
    iterateActions();
}

void CAmControlSend::cbAckConnect(const am_Handle_s handle, const am_Error_e errorID)
{
    LOG_FN_INFO(__FILENAME__,__func__,"  IN handle.Type=", (int)handle.handleType, " handle.handle=", (int)handle.handle,
                " errorID=", errorID);
    assert(NULL != mpControlReceive);

    mpControlReceive->notifyAsyncResult(handle, errorID);
    iterateActions();
}

void CAmControlSend::cbAckDisconnect(const am_Handle_s handle, const am_Error_e errorID)
{
    LOG_FN_INFO(__FILENAME__,__func__,"  IN  handle.Type=", (int)handle.handleType, " handle.handle=", (int)handle.handle,
                " errorID=", errorID);
    assert(NULL != mpControlReceive);

    mpControlReceive->notifyAsyncResult(handle, errorID);
    iterateActions();
}

void CAmControlSend::cbAckCrossFade(const am_Handle_s handle, const am_HotSink_e hotSink,
                                    const am_Error_e errorID)
{
    LOG_FN_INFO(__FILENAME__,__func__," IN  handle.Type/handle=", (int)handle.handleType, (int)handle.handle,
                " hostsink=", hotSink, " errorID=", errorID);
    assert(NULL != mpControlReceive);

    mpControlReceive->notifyAsyncResult(handle, errorID);
    iterateActions();
}

void CAmControlSend::cbAckSetSinkVolumeChange(const am_Handle_s handle, const am_volume_t volume,
                                              const am_Error_e errorID)
{
    LOG_FN_INFO(__FILENAME__,__func__," IN  handle.Type/handle=", (int)handle.handleType, (int)handle.handle, " volume=",
                volume, " errorID", errorID);
    assert(NULL != mpControlReceive);

    mpControlReceive->notifyAsyncResult(handle, errorID);
    iterateActions();
}

void CAmControlSend::cbAckSetSourceVolumeChange(const am_Handle_s handle, const am_volume_t volume,
                                                const am_Error_e errorID)
{

    LOG_FN_INFO(__FILENAME__,__func__," IN  handle.Type/handle=", (int)handle.handleType, (int)handle.handle, " volume=",
                volume, " errorID", errorID);
    assert(NULL != mpControlReceive);

    mpControlReceive->notifyAsyncResult(handle, errorID);
    iterateActions();
}

void CAmControlSend::cbAckSetSourceState(const am_Handle_s handle, const am_Error_e errorID)
{

    LOG_FN_INFO(__FILENAME__,__func__,"  IN  handle.Type=", (int)handle.handleType, " handle.Type=", (int)handle.handle,
                " errorID=", errorID);
    assert(NULL != mpControlReceive);

    mpControlReceive->notifyAsyncResult(handle, errorID);
    iterateActions();
}

void CAmControlSend::cbAckSetSourceSoundProperties(const am_Handle_s handle,
                                                   const am_Error_e errorID)
{

    LOG_FN_INFO(__FILENAME__,__func__,"  IN  handle.Type=", (int)handle.handleType, " handle.Type=", (int)handle.handle,
                " errorID=", errorID);
    assert(NULL != mpControlReceive);

    mpControlReceive->notifyAsyncResult(handle, errorID);
    iterateActions();
}

void CAmControlSend::cbAckSetSourceSoundProperty(const am_Handle_s handle, const am_Error_e errorID)
{
    LOG_FN_INFO(__FILENAME__,__func__,"  IN  handle.Type=", (int)handle.handleType, " handle.Type=", (int)handle.handle,
                " errorID=", errorID);
    assert(NULL != mpControlReceive);

    mpControlReceive->notifyAsyncResult(handle, errorID);
    iterateActions();
}

void CAmControlSend::cbAckSetSinkSoundProperties(const am_Handle_s handle, const am_Error_e errorID)
{

    LOG_FN_INFO(__FILENAME__,__func__,"  IN  handle.Type=", (int)handle.handleType, " handle.Type=", (int)handle.handle,
                " errorID=", errorID);
    assert(NULL != mpControlReceive);

    mpControlReceive->notifyAsyncResult(handle, errorID);
    iterateActions();
}

void CAmControlSend::cbAckSetSinkSoundProperty(const am_Handle_s handle, const am_Error_e errorID)
{

    LOG_FN_INFO(__FILENAME__,__func__,"  IN  handle.Type=", (int)handle.handleType, " handle.Type=", (int)handle.handle,
                " errorID=", errorID);
    assert(NULL != mpControlReceive);

    mpControlReceive->notifyAsyncResult(handle, errorID);
    iterateActions();
}

am_Error_e CAmControlSend::getConnectionFormatChoice(
                const am_sourceID_t sourceID, const am_sinkID_t sinkID,
                const am_Route_s routeInstance,
                const std::vector<am_CustomConnectionFormat_t > listPossibleConnectionFormats,
                std::vector<am_CustomConnectionFormat_t >& listPrioConnectionFormats)
{

    (void)sourceID;
    (void)sinkID;
    (void)routeInstance;
    (void)listPossibleConnectionFormats;
    (void)listPrioConnectionFormats;
    listPrioConnectionFormats = listPossibleConnectionFormats;
    return E_OK;
}

void CAmControlSend::confirmCommandReady(const am_Error_e error)
{
    (void)error;
    // have to be implemented if requirements come
}

void CAmControlSend::confirmRoutingReady(const am_Error_e error)
{
    std::vector<am_SystemProperty_s> listSystemProperties;
    std::vector<am_SystemProperty_s>::iterator itListSystemProperties;
    int16_t domainRegistrationTimeout = 0;

    if(E_OK != error)
    {
        return;
    }
    if(!mDomainRegistrationTimerHandle)
    {
        if (E_OK == mpControlReceive->getListSystemProperties(listSystemProperties))
        {
            if (listSystemProperties.size() > 0)
            {
                for (itListSystemProperties = listSystemProperties.begin();
                                itListSystemProperties != listSystemProperties.end();
                                itListSystemProperties++)
                {
                    if (itListSystemProperties->type == SYP_REGISTRATION_DOMAIN_TIMEOUT)
                    {
                        domainRegistrationTimeout = itListSystemProperties->value;
                        CAmTimerEvent* pTimer = CAmTimerEvent::getInstance();
                        pTimer->setTimer(&mpdomainRegiTimerCallback, this,
                                         domainRegistrationTimeout, mDomainRegistrationTimerHandle);
                    }
                }
            }
        }
        else
        {
            LOG_FN_ERROR(__FILENAME__,__func__,"invalid getListSystemProperties");
        }
    }
    return;
}

void CAmControlSend::_domainRegistrationTimeout(void* data)
{
    CAmControlSend* pSelf = (CAmControlSend*)data;
    am_Error_e result= E_UNKNOWN;
    if (pSelf == this)
    {
        result = _restoreConnectionsFromPersistency();
        gc_AllDomainRegisterationCompleteTrigger_s* pdomainRegistrationTrigger = new gc_AllDomainRegisterationCompleteTrigger_s;
        if (NULL == pdomainRegistrationTrigger)
        {
            return;
        }
        if (E_DATABASE_ERROR == result)
        {
            pdomainRegistrationTrigger->status = E_DATABASE_ERROR;
        }
        else
        {
            pdomainRegistrationTrigger->status = E_NOT_POSSIBLE;
        }
        CAmTriggerQueue::getInstance()->queue(SYSTEM_ALL_DOMAIN_REGISTRATION_COMPLETE,pdomainRegistrationTrigger);
        iterateActions();
    }
    else
    {
        LOG_FN_ERROR(__FILENAME__,__func__,"invalid arguments to timer function");
    }
    return;
}

bool CAmControlSend::_checkAllDomainRegistered(void)
{
    std::vector<gc_Domain_s> listDomains;
    std::vector<gc_Domain_s>::iterator itListDomains;
    CAmClassElement* pClassElement;
    bool result = false;
    if (E_OK == mpPolicySend->getListDomains(listDomains))
    {
        for (itListDomains = listDomains.begin(); itListDomains != listDomains.end();
                        itListDomains++)
        {
            result = _isDomainRegistrationComplete((*itListDomains).name);
            if (false == result)
            {
                LOG_FN_ERROR(__FILENAME__,__func__,"domain registration not complete:", (*itListDomains).name);
                break;
            }
        }
    }
    return result;
}

bool CAmControlSend::_isDomainRegistrationComplete(const std::string& domainName)
{
    std::vector < am_Domain_s > listDomains;
    std::vector<am_Domain_s >::iterator itListDomains;
    if (E_OK == mpControlReceive->getListElements(listDomains))
    {
        for (itListDomains = listDomains.begin(); itListDomains != listDomains.end();
                        itListDomains++)
        {
            if ((*itListDomains).name == domainName)
            {
                return (*itListDomains).complete;
            }
        }
    }
    return false;
}

am_Error_e CAmControlSend::_restoreConnectionsFromPersistency(void)
{
    am_Error_e result= E_UNKNOWN;
    std::vector<string> listClasses;
    std::vector<string>::iterator itlistClasses;
    std::string mainConnectionString;
    std::vector<gc_LastMainConnections_s> listlastMainConnections;
    std::vector<gc_LastMainConnections_s>::iterator itlistlastMainConnections;
    std::string keylastMainConnection = "lastMainConnection";
    std::string delimiter = "{";

    IAmPersistence* pPersistence = CAmPersistenceWrapper::getInstance();
    result = pPersistence->read(keylastMainConnection,mainConnectionString);
    if((E_OK == result) && (!mainConnectionString.empty()))
    {
        if(E_OK != _parseString(delimiter,mainConnectionString,listClasses))
        {
            LOG_FN_INFO(__FILENAME__,__func__,"wrong data format in the persistence database");
            return (E_NOT_POSSIBLE);
        }
        for (itlistClasses = listClasses.begin(); itlistClasses != listClasses.end(); itlistClasses++)
        {
            std::vector<string> listMainConnections;
            std::vector<string>::iterator itlistMainConnections;
            gc_LastMainConnections_s lastMainConnections;
            delimiter = ",";
            if(E_OK !=_parseString(delimiter,*itlistClasses,listMainConnections))
            {
                continue;
            }
            for (itlistMainConnections = listMainConnections.begin();
                            itlistMainConnections != listMainConnections.end(); itlistMainConnections++)
            {
                std::vector<string> listsourcesink;
                std::vector<string>::iterator itlistsourcesink;
                lastMainConnections.className = *itlistMainConnections;
                itlistMainConnections++;
                if(itlistMainConnections == listMainConnections.end())
                {
                    LOG_FN_INFO(__FILENAME__,__func__,"wrong class information for main connections in the persistence database");
                    break;
                }
                delimiter = ";";
                std::vector<string> mainConnections;
                if(E_OK != _parseString(delimiter,*itlistMainConnections,listsourcesink))
                {
                    continue;
                }
                for (itlistsourcesink = listsourcesink.begin();
                                itlistsourcesink != listsourcesink.end(); itlistsourcesink++)
                {
                    /*in case the special character '}' is present in the string then avoid inserting it in to the main connection list*/
                    std::size_t found = (*itlistsourcesink).find_first_of("}");
                    if (found == std::string::npos)
                    {
                        lastMainConnections.listMainConnections.insert(*itlistsourcesink);
                    }
                }
                listlastMainConnections.push_back(lastMainConnections);
            }
        }
    }
    else
    {
        LOG_FN_INFO(__FILENAME__,__func__,"persistence database error");
        return (E_DATABASE_ERROR);
    }
    /* make the connection based on the persistence data */
    for (itlistlastMainConnections = listlastMainConnections.begin();
                    itlistlastMainConnections != listlastMainConnections.end(); itlistlastMainConnections++)
    {
        LOG_FN_DEBUG(__FILENAME__,__func__,"display class name::",itlistlastMainConnections->className);
        std::set<string> listsourcesink = itlistlastMainConnections->listMainConnections;
        std::set<string>::iterator itlistsourcesink;
        for (itlistsourcesink = listsourcesink.begin(); itlistsourcesink != listsourcesink.end(); itlistsourcesink++)
        {
            LOG_FN_DEBUG(__FILENAME__,__func__,"display sourcesink name::",*itlistsourcesink);
            _createLastMainConnection(*itlistsourcesink);
        }
    }
    return result;
}

am_Error_e CAmControlSend::_parseString(const string& delimiter, string& inputString,vector<string>& listOutput)
{
    if(!inputString.empty())
    {
        size_t pos = 0;
        string token;
        bool delimiterFound = false;
        while ((pos = inputString.find(delimiter)) != string::npos)
        {
            delimiterFound = true;
            token = inputString.substr(0, pos);
            inputString.erase(0, pos + delimiter.length());
            if(!token.empty())
            {
                listOutput.push_back(token);
            }
        }
        if(!inputString.empty() && delimiterFound)
        {
            listOutput.push_back(inputString);
			return E_OK;
        }
        else
        {
            return E_NOT_POSSIBLE;
        }
    }
    else
    {
        LOG_FN_INFO(__FILENAME__,__func__,"input string is empty");
        return E_NOT_POSSIBLE;
    }
}

am_Error_e CAmControlSend::_createLastMainConnection(string mainConnection)
{
    if(!mainConnection.empty())
    {
        am_mainConnectionID_t mainConnectionID = 0;
        CAmSinkElement* pSinkElement;
        CAmSourceElement* pSourceElement;
        CAmClassElement* pClassElement;
        am_Error_e result = E_UNKNOWN;
        std::vector < am_Route_s > listRoutes;
        string mainSourceName;
        string mainSinkName;
        string delimiter = ":";
        vector<string> listSourceSink;

        if (E_OK != _parseString(delimiter,mainConnection,listSourceSink))
        {
        	LOG_FN_INFO(__FILENAME__,__func__,"wrong data format for source and sink element in the persistence database");
        	return (E_NOT_POSSIBLE);
        }
        std::vector<string>::iterator itlistSourceSink = listSourceSink.begin();

        if(itlistSourceSink == listSourceSink.end())
        {
            return (E_NOT_POSSIBLE);
        }
        mainSourceName = *itlistSourceSink;
        itlistSourceSink++;
        if(itlistSourceSink == listSourceSink.end())
        {
            return (E_NOT_POSSIBLE);
        }
        mainSinkName = *itlistSourceSink;
        pSourceElement = CAmSourceFactory::getElement(mainSourceName);
        pSinkElement = CAmSinkFactory::getElement(mainSinkName);
        if (pSinkElement == NULL || pSourceElement == NULL)
        {
            LOG_FN_INFO(__FILENAME__,__func__,"source or sink element does not exist.");
            return E_NON_EXISTENT;
        }
        pClassElement = CAmClassFactory::getElement(pSourceElement->getName(), pSinkElement->getName());
        if (NULL == pClassElement)
        {
            LOG_FN_ERROR(__FILENAME__,__func__,"  got failed.");
            return (E_NOT_POSSIBLE);
        }
        result = mpControlReceive->getRoute(false, pSourceElement->getID(), pSinkElement->getID(),
                                            listRoutes);
        if (E_OK != result)
        {
            LOG_FN_ERROR(__FILENAME__,__func__," getRoute returned error:", result);
            return result;
        }
        if (true == listRoutes.empty())
        {
            LOG_FN_ERROR(__FILENAME__,__func__,"  couldn't getRoute. The list is empty");
            return (E_NOT_POSSIBLE);
        }
        result = pClassElement->createMainConnection(pSourceElement->getName(), pSinkElement->getName(),
                                                     mainConnectionID);
        if ((result != E_OK) && (result != E_ALREADY_EXISTS))
        {
            LOG_FN_ERROR(__FILENAME__,__func__,"Failed to create connection");
            return result;
        }
        return result;
    }
    else
    {
        return E_NOT_POSSIBLE;
    }
}

void CAmControlSend::confirmCommandRundown(const am_Error_e error)
{
    (void)error;
    // have to be implemented if requirements come
}

void CAmControlSend::confirmRoutingRundown(const am_Error_e error)
{
    (void)error;
    // have to be implemented if requirements come
}

am_Error_e CAmControlSend::hookSystemUpdateSink(
                const am_sinkID_t sinkID, const am_sinkClass_t sinkClassID,
                const std::vector<am_SoundProperty_s >& listSoundProperties,
                const std::vector<am_CustomConnectionFormat_t >& listConnectionFormats,
                const std::vector<am_MainSoundProperty_s >& listMainSoundProperties)
{
    am_Error_e result(E_NOT_POSSIBLE);
    assert(NULL != mpControlReceive);
    CAmSinkElement* pSink = CAmSinkFactory::getElement(sinkID);
    if (NULL != pSink)
    {
        /*
         * Regarding the classId the logic is as below
         * 1. If the class Id from routing side is valid change the class ID
         * 2. If class ID is invalid and retain the old class ID
         */
        CAmClassElement* pClass = CAmClassFactory::getElement(sinkClassID);
        if(pClass == NULL)
        {
            result = pSink->upadateDB( pSink->getClassID(),
                                       listSoundProperties,
                                       listConnectionFormats,
                                       listMainSoundProperties);
        }
        else
        {
            result = pSink->upadateDB( sinkClassID,
                                       listSoundProperties,
                                       listConnectionFormats,
                                       listMainSoundProperties);
        }
    }
    return result;
}

am_Error_e CAmControlSend::hookSystemUpdateSource(
                const am_sourceID_t sourceID, const am_sourceClass_t sourceClassID,
                const std::vector<am_SoundProperty_s >& listSoundProperties,
                const std::vector<am_CustomConnectionFormat_t >& listConnectionFormats,
                const std::vector<am_MainSoundProperty_s >& listMainSoundProperties)
{
    am_Error_e result(E_NOT_POSSIBLE);
    assert(NULL != mpControlReceive);
    CAmSourceElement* pSource = CAmSourceFactory::getElement(sourceID);
    if (NULL != pSource)
    {
        /*
         * Regarding the classId the logic is as below
         * 1. If the class Id from routing side is valid change the class ID
         * 2. If class ID is invalid and retain the old class ID
         */
        CAmClassElement* pClass = CAmClassFactory::getElement(sourceClassID);
        if(pClass == NULL)
        {
            result = pSource->upadateDB( pSource->getClassID(),
                                         listSoundProperties,
                                         listConnectionFormats,
                                         listMainSoundProperties);
        }
        else
        {
            result = pSource->upadateDB( sourceClassID,
                                         listSoundProperties,
                                         listConnectionFormats,
                                         listMainSoundProperties);
        }
    }
    return result;
}

am_Error_e CAmControlSend::hookSystemUpdateGateway(
                const am_gatewayID_t gatewayID,
                const std::vector<am_CustomConnectionFormat_t >& listSourceConnectionFormats,
                const std::vector<am_CustomConnectionFormat_t >& listSinkConnectionFormats,
                const std::vector<bool >& listConvertionMatrix)
{
    assert(NULL != mpControlReceive);
    //TODO Gateway element should implement updateDB() as well as source, sink
    return mpControlReceive->changeGatewayDB(gatewayID, listSourceConnectionFormats,
                                             listSinkConnectionFormats, listConvertionMatrix);
}

void CAmControlSend::cbAckSetVolumes(const am_Handle_s handle,
                                     const std::vector<am_Volumes_s >& listVolumes,
                                     const am_Error_e error)
{
    (void)handle;
    (void)listVolumes;
    (void)error;
}

void CAmControlSend::cbAckSetSinkNotificationConfiguration(const am_Handle_s handle,
                                                           const am_Error_e error)
{
    LOG_FN_INFO(__FILENAME__,__func__,"  IN  handle.Type/Handle=", (int)handle.handleType, " handle.handle=", (int)handle.handle,
                " errorID=", error);
    assert(NULL != mpControlReceive);
    mpControlReceive->notifyAsyncResult(handle, error);
    iterateActions();
    LOG_FN_EXIT(__FILENAME__,__func__);
    return;
}

void CAmControlSend::cbAckSetSourceNotificationConfiguration(const am_Handle_s handle,
                                                             const am_Error_e error)
{
    LOG_FN_INFO(__FILENAME__,__func__,"  IN  handle.Type/Handle=", (int)handle.handleType, " handle.handle=", (int)handle.handle,
                " errorID=", error);
    assert(NULL != mpControlReceive);

    mpControlReceive->notifyAsyncResult(handle, error);
    iterateActions();
    LOG_FN_EXIT(__FILENAME__,__func__);
    return;
}
void CAmControlSend::hookSinkNotificationDataChanged(const am_sinkID_t sinkID,
                                                     const am_NotificationPayload_s& payload)
{
    CAmSinkElement* pElement;
    LOG_FN_ENTRY(__FILENAME__,__func__,sinkID);

    // check if the sinkID is valid
    pElement = CAmSinkFactory::getElement(sinkID);
    if (NULL == pElement)
    {
        LOG_FN_ERROR(__FILENAME__,__func__,"  Not able to get sink element");
        return;
    }
    pElement->notificationDataUpdate(payload);
    // Store the trigger in a Queue
    gc_NotificationDataTrigger_s* triggerData = new gc_NotificationDataTrigger_s;
    if (NULL == triggerData)
    {
        LOG_FN_ERROR(__FILENAME__,__func__,"  bad memory state:");
        return;
    }
    triggerData->name = pElement->getName();
    triggerData->notificatonPayload = payload;
    CAmTriggerQueue::getInstance()->queue(SYSTEM_SINK_NOTIFICATION_DATA_CHANGED, triggerData);
    iterateActions();
    LOG_FN_EXIT(__FILENAME__,__func__);
    return;
}

void CAmControlSend::hookSourceNotificationDataChanged(const am_sourceID_t sourceID,
                                                       const am_NotificationPayload_s& payload)
{
    CAmSourceElement* pElement;
    LOG_FN_ENTRY(__FILENAME__,__func__,sourceID);

    // check if the sourceID is valid
    pElement = CAmSourceFactory::getElement(sourceID);
    if (NULL == pElement)
    {
        LOG_FN_ERROR(__FILENAME__,__func__,"  Not able to get source element");
        return;
    }
    pElement->notificationDataUpdate(payload);
    // Store the trigger in a Queue
    gc_NotificationDataTrigger_s* triggerData = new gc_NotificationDataTrigger_s;
    if (NULL == triggerData)
    {
        LOG_FN_ERROR(__FILENAME__,__func__,"  bad memory state:");
        return;
    }
    triggerData->name = pElement->getName();
    triggerData->notificatonPayload = payload;
    CAmTriggerQueue::getInstance()->queue(SYSTEM_SOURCE_NOTIFICATION_DATA_CHANGED, triggerData);
    iterateActions();
    LOG_FN_EXIT(__FILENAME__,__func__);
    return;

}

am_Error_e CAmControlSend::hookUserSetMainSinkNotificationConfiguration(
                const am_sinkID_t sinkID,
                const am_NotificationConfiguration_s& notificationConfiguration)
{
    CAmSinkElement* pElement;
    LOG_FN_ENTRY(__FILENAME__,__func__,sinkID,notificationConfiguration.parameter,notificationConfiguration.type,notificationConfiguration.status);

    // check if the sinkID is valid
    pElement = CAmSinkFactory::getElement(sinkID);
    if (NULL == pElement)
    {
        LOG_FN_ERROR(__FILENAME__,__func__,"  Not able to get sink element");
        return (E_OUT_OF_RANGE);
    }

    // Store the trigger in a Queue
    gc_NotificationConfigurationTrigger_s* triggerData = new gc_NotificationConfigurationTrigger_s;
    if (NULL == triggerData)
    {
        LOG_FN_ERROR(__FILENAME__,__func__,"  bad memory state:");
        return E_NOT_POSSIBLE;
    }
    triggerData->name = pElement->getName();
    triggerData->notificatonConfiguration = notificationConfiguration;
    CAmTriggerQueue::getInstance()->queue(USER_SET_SINK_MAIN_NOTIFICATION_CONFIGURATION, triggerData);
    iterateActions();
    LOG_FN_EXIT(__FILENAME__,__func__);
    return E_OK;
}

am_Error_e CAmControlSend::hookUserSetMainSourceNotificationConfiguration(
                const am_sourceID_t sourceID,
                const am_NotificationConfiguration_s& notificationConfiguration)
{
    CAmSourceElement* pElement;
    LOG_FN_ENTRY(__FILENAME__,__func__,sourceID);

    // check if the sourceID is valid
    pElement = CAmSourceFactory::getElement(sourceID);
    if (NULL == pElement)
    {
        LOG_FN_ERROR(__FILENAME__,__func__,"  Not able to get source element");
        return (E_OUT_OF_RANGE);
    }

    // Store the trigger in a Queue
    gc_NotificationConfigurationTrigger_s* triggerData = new gc_NotificationConfigurationTrigger_s;
    if (NULL == triggerData)
    {
        LOG_FN_ERROR(__FILENAME__,__func__,"  bad memory state:");
        return E_NOT_POSSIBLE;
    }
    triggerData->name = pElement->getName();
    triggerData->notificatonConfiguration = notificationConfiguration;
    CAmTriggerQueue::getInstance()->queue(USER_SET_SOURCE_MAIN_NOTIFICATION_CONFIGURATION, triggerData);
    iterateActions();
    LOG_FN_EXIT(__FILENAME__,__func__);
    return E_OK;
}
#ifdef NSM_IFACE_PRESENT
void CAmControlSend::hookSystemNodeStateChanged(const NsmNodeState_e nodeStateId)
{
    (void)nodeStateId;
    //here you can process informations about the nodestate
}

void CAmControlSend::hookSystemNodeApplicationModeChanged(
                const NsmApplicationMode_e applicationModeId)
{
    (void)applicationModeId;
}

void CAmControlSend::hookSystemSessionStateChanged(const std::string& sessionName,
                                                   const NsmSeat_e seatId,
                                                   const NsmSessionState_e sessionStateId)
{
    (void)sessionName;
    (void)seatId;
    (void)sessionStateId;
}

NsmErrorStatus_e CAmControlSend::hookSystemLifecycleRequest(const uint32_t request,
                                                            const uint32_t requestId)
{
    (void)request;
    (void)requestId;
    return (NsmErrorStatus_Error);
}
#endif

void CAmControlSend::hookSystemSingleTimingInformationChanged(const am_connectionID_t connectionID,
                                                              const am_timeSync_t time)
{
    (void)connectionID;
    (void)time;
}

am_Error_e CAmControlSend::hookSystemRegisterConverter(const am_Converter_s& converterData,
                                                       am_converterID_t& converterID)
{
    (void)converterData;
    (void)converterID;
    return E_OK;
}

am_Error_e CAmControlSend::hookSystemDeregisterConverter(const am_converterID_t converterID)
{
    (void)converterID;
    return E_OK;
}

am_Error_e CAmControlSend::hookSystemUpdateConverter(
                const am_converterID_t converterID,
                const std::vector<am_CustomConnectionFormat_t >& listSourceConnectionFormats,
                const std::vector<am_CustomConnectionFormat_t >& listSinkConnectionFormats,
                const std::vector<bool >& listConvertionMatrix)
{
    (void)converterID;
    (void)listConvertionMatrix;
    (void)listSinkConnectionFormats;
    (void)listSourceConnectionFormats;
    return E_OK;
}

//
// private functions
//
void CAmControlSend::_freeMemory(void)
{
    CAmGatewayFactory::destroyElement();
    CAmSourceFactory::destroyElement();
    CAmSinkFactory::destroyElement();
    CAmClassFactory::destroyElement();
    if (NULL != mpPolicySend)
    {
        delete mpPolicySend;
    }
    if (NULL != mpControlReceive)
    {
        delete mpControlReceive;
    }
}

void CAmControlSend::iterateActions(void)
{
    CAmRootAction* pRootAction = CAmRootAction::getInstance();
    gc_Trigger_e triggerType;
    gc_TriggerElement_s* triggerData = NULL;
    if (NULL == pRootAction)
    {
        LOG_FN_ERROR(__FILENAME__,__func__," Not able to get Root instance");
        return;
    }
    do
    {
        pRootAction->execute();
        if ((AS_ERROR_STOPPED == pRootAction->getStatus()) ||
            (AS_UNDOING == pRootAction->getStatus()))
        {
            pRootAction->undo();
        }
        pRootAction->cleanup();
        if (AS_NOT_STARTED == pRootAction->getStatus())
        {
            if (true == pRootAction->isEmpty())
            {
                std::pair<gc_Trigger_e, gc_TriggerElement_s* > triggerPair;
                triggerData = CAmTriggerQueue::getInstance()->dequeue(triggerType);
                if (NULL != triggerData)
                {
                    _forwardTriggertoPolicyEngine(triggerType, triggerData);
                    delete triggerData;
                }
                else
                {
                    break;
                }
            }
            else
            {
                // No more actions present in the loop
                LOG_FN_DEBUG(__FILENAME__,__func__,"trigger data is null");
                break;
            }
        }
    } while (pRootAction->getStatus() == AS_NOT_STARTED);
}

am_Error_e CAmControlSend::_forwardTriggertoPolicyEngine(gc_Trigger_e triggerType,
                                                         gc_TriggerElement_s* triggerData)
{
    am_Error_e result = E_OK;
    if (NULL == mpPolicySend)
    {
        return E_UNKNOWN;
    }
    switch (triggerType)
    {
    case SYSTEM_REGISTER_SINK:
    {
        gc_RegisterElementTrigger_s* pRegisterElementTrigger_t = (gc_RegisterElementTrigger_s*)triggerData;
        result = mpPolicySend->hookRegisterSink(pRegisterElementTrigger_t->elementName,
                                                pRegisterElementTrigger_t->RegisterationStatus);
    }
        break;
    case SYSTEM_REGISTER_SOURCE:
    {
        gc_RegisterElementTrigger_s* pRegisterElementTrigger_t = (gc_RegisterElementTrigger_s*)triggerData;
        result = mpPolicySend->hookRegisterSource(pRegisterElementTrigger_t->elementName,
                                                  pRegisterElementTrigger_t->RegisterationStatus);
    }
        break;
    case SYSTEM_SINK_AVAILABILITY_CHANGED:
    {
        gc_AvailabilityChangeTrigger_s* pAvailabilityChangeTrigger_t = (gc_AvailabilityChangeTrigger_s*)triggerData;
        result = mpPolicySend->hookSinkAvailabilityChange(
                        pAvailabilityChangeTrigger_t->elementName,
                        pAvailabilityChangeTrigger_t->availability);
    }
        break;
    case SYSTEM_SOURCE_AVAILABILITY_CHANGED:
    {
        gc_AvailabilityChangeTrigger_s* pSourceAvailabilityChangeTrigger_t = (gc_AvailabilityChangeTrigger_s*)triggerData;
        result = mpPolicySend->hookSourceAvailabilityChange(
                        pSourceAvailabilityChangeTrigger_t->elementName,
                        pSourceAvailabilityChangeTrigger_t->availability);
    }
        break;
    case SYSTEM_DEREGISTER_SINK:
    {
        gc_UnRegisterElementTrigger_s* pUnRegisterElementTrigger_t = (gc_UnRegisterElementTrigger_s*)triggerData;
        result = mpPolicySend->hookDeregisterSink(
                        pUnRegisterElementTrigger_t->elementName,
                        pUnRegisterElementTrigger_t->unRegisterationStatus);
    }
        break;
    case SYSTEM_DEREGISTER_SOURCE:
    {
        gc_UnRegisterElementTrigger_s* pUnRegisterElementTrigger_t = (gc_UnRegisterElementTrigger_s*)triggerData;
        result = mpPolicySend->hookDeregisterSource(
                        pUnRegisterElementTrigger_t->elementName,
                        pUnRegisterElementTrigger_t->unRegisterationStatus);
    }
        break;
    case USER_CONNECTION_REQUEST:
    {
        gc_ConnectTrigger_s *pConnectTrigger_t = (gc_ConnectTrigger_s*)triggerData;
        result = mpPolicySend->hookConnectionRequest(pConnectTrigger_t->className,
                                                     pConnectTrigger_t->sourceName,
                                                     pConnectTrigger_t->sinkName);
    }
        break;
    case USER_DISCONNECTION_REQUEST:
    {
        gc_DisconnectTrigger_s* pDisconnectTrigger_t = (gc_DisconnectTrigger_s*)triggerData;
        result = mpPolicySend->hookDisconnectionRequest(pDisconnectTrigger_t->className,
                                                        pDisconnectTrigger_t->sourceName,
                                                        pDisconnectTrigger_t->sinkName);
    }
        break;
    case USER_SET_SINK_MUTE_STATE:
    {
        gc_SinkMuteTrigger_s* pSinkMuteTrigger_t = (gc_SinkMuteTrigger_s*)triggerData;
        result = mpPolicySend->hookSetSinkMuteState(pSinkMuteTrigger_t->sinkName,
                                                    pSinkMuteTrigger_t->muteState);
    }
        break;
    case USER_SET_VOLUME:
    {
        gc_SinkVolumeChangeTrigger_s* pSinkVolumeChangeTrigger_t = (gc_SinkVolumeChangeTrigger_s*)triggerData;
        result = mpPolicySend->hookVolumeChange(pSinkVolumeChangeTrigger_t->sinkName,
                                                pSinkVolumeChangeTrigger_t->volume);
    }
        break;
    case USER_SET_SINK_MAIN_SOUND_PROPERTY:
    {
        gc_SinkSoundPropertyTrigger_s* pSinkSoundPropertyTrigger_t = (gc_SinkSoundPropertyTrigger_s*)triggerData;
        result = mpPolicySend->hookSetMainSinkSoundProperty(
                        pSinkSoundPropertyTrigger_t->sinkName,
                        pSinkSoundPropertyTrigger_t->mainSoundProperty);
    }
        break;
    case USER_SET_SOURCE_MAIN_SOUND_PROPERTY:
    {
        gc_SourceSoundPropertyTrigger_s* pSourceSoundPropertyTrigger_t = (gc_SourceSoundPropertyTrigger_s*)triggerData;
        result = mpPolicySend->hookSetMainSourceSoundProperty(
                        pSourceSoundPropertyTrigger_t->sourceName,
                        pSourceSoundPropertyTrigger_t->mainSoundProperty);
    }
        break;
    case SYSTEM_INTERRUPT_STATE_CHANGED:
    {
        gc_SourceInterruptChangeTrigger_s* pSourceInterruptChangeTrigger_t = (gc_SourceInterruptChangeTrigger_s*)triggerData;
        result = mpPolicySend->hookSourceInterruptStateChange(
                        pSourceInterruptChangeTrigger_t->sourceName,
                        pSourceInterruptChangeTrigger_t->interrptstate);
    }
        break;
    case SYSTEM_REGISTER_DOMAIN:
    {
        gc_RegisterElementTrigger_s* pRegisterElementTrigger_t = (gc_RegisterElementTrigger_s*)triggerData;
        result = mpPolicySend->hookRegisterDomain(pRegisterElementTrigger_t->elementName,
                                                  pRegisterElementTrigger_t->RegisterationStatus);
    }
        break;
    case SYSTEM_DEREGISTER_DOMAIN:
    {
        gc_UnRegisterElementTrigger_s* pUnRegisterElementTrigger_t = (gc_UnRegisterElementTrigger_s*)triggerData;
        result = mpPolicySend->hookDeregisterDomain(
                        pUnRegisterElementTrigger_t->elementName,
                        pUnRegisterElementTrigger_t->unRegisterationStatus);
    }
        break;
    case SYSTEM_REGISTER_GATEWAY:
    {
        gc_RegisterElementTrigger_s* pRegisterElementTrigger_t = (gc_RegisterElementTrigger_s*)triggerData;
        result = mpPolicySend->hookRegisterGateway(pRegisterElementTrigger_t->elementName,
                                                   pRegisterElementTrigger_t->RegisterationStatus);
    }
        break;
    case SYSTEM_DEREGISTER_GATEWAY:
    {
        gc_UnRegisterElementTrigger_s* pUnRegisterElementTrigger_t = (gc_UnRegisterElementTrigger_s*)triggerData;
        result = mpPolicySend->hookDeregisterGateway(
                        pUnRegisterElementTrigger_t->elementName,
                        pUnRegisterElementTrigger_t->unRegisterationStatus);
    }
        break;
    case SYSTEM_DOMAIN_REGISTRATION_COMPLETE:
    {
        gc_DomainRegisterationCompleteTrigger_s* pRegisterElementTrigger_t = (gc_DomainRegisterationCompleteTrigger_s*)triggerData;
        result = mpPolicySend->hookDomainRegistrationComplete(
                        pRegisterElementTrigger_t->domainName);
    }
    break;
    case SYSTEM_ALL_DOMAIN_REGISTRATION_COMPLETE:
    {
        gc_AllDomainRegisterationCompleteTrigger_s* pRegisterElementTrigger_t = (gc_AllDomainRegisterationCompleteTrigger_s*)triggerData;
            result = mpPolicySend->hookAllDomainRegistrationComplete(
                            pRegisterElementTrigger_t->status);
    }
    break;
    case USER_SET_SYSTEM_PROPERTY:
    {
        gc_SystemPropertyTrigger_s* pSystemPropertyTrigger_t = (gc_SystemPropertyTrigger_s*)triggerData;
        result = mpPolicySend->hookSetSystemProperty(pSystemPropertyTrigger_t->systemProperty);
    }
    break;
    case SYSTEM_CONNECTION_STATE_CHANGE:
    {
        gc_ConnectionStateChangeTrigger_s* pConnectionStateTrigger_t = (gc_ConnectionStateChangeTrigger_s*)triggerData;
        result = mpPolicySend->hookConnectionStateChange(pConnectionStateTrigger_t->connectionName,
                                                         pConnectionStateTrigger_t->connectionState,
                                                         pConnectionStateTrigger_t->status);
    }
        break;
    case USER_SET_SINK_MAIN_NOTIFICATION_CONFIGURATION:
    {
        gc_NotificationConfigurationTrigger_s* pNotificationConfiguration = (gc_NotificationConfigurationTrigger_s*)triggerData;
        result = mpPolicySend->hookSetMainSinkNotificationConfiguration(
                        pNotificationConfiguration->name,
                        pNotificationConfiguration->notificatonConfiguration);
    }
        break;
    case USER_SET_SOURCE_MAIN_NOTIFICATION_CONFIGURATION:
    {
        gc_NotificationConfigurationTrigger_s* pNotificationConfiguration = (gc_NotificationConfigurationTrigger_s*)triggerData;
        result = mpPolicySend->hookSetMainSourceNotificationConfiguration(
                        pNotificationConfiguration->name,
                        pNotificationConfiguration->notificatonConfiguration);
    }
        break;
    case SYSTEM_SINK_NOTIFICATION_DATA_CHANGED:
    {
        gc_NotificationDataTrigger_s* pNotificationData = (gc_NotificationDataTrigger_s*)triggerData;
        result = mpPolicySend->hookSinkNotificationDataChanged(
                        pNotificationData->name, pNotificationData->notificatonPayload);
    }
        break;
    case SYSTEM_SOURCE_NOTIFICATION_DATA_CHANGED:
    {
        gc_NotificationDataTrigger_s* pNotificationData = (gc_NotificationDataTrigger_s*)triggerData;
        result = mpPolicySend->hookSourceNotificationDataChanged(
                        pNotificationData->name, pNotificationData->notificatonPayload);
    }
        break;
    default:
        break;
    }
    return result;
}

am_Error_e CAmControlSend::_storeMainConnectiontoPersistency()
{
    /*write main connection to the persistency*/
    std::vector<gc_Class_s> listClasses;
    std::vector<gc_Class_s>::iterator itListClasses;
    CAmClassElement* pClassElement = NULL;
    am_Error_e result= E_OK;
    string mainConnectionString;
    std::string keylastMainConnection = "lastMainConnection";

    if (E_OK == mpPolicySend->getListClasses(listClasses))
    {
        for (itListClasses = listClasses.begin(); itListClasses != listClasses.end(); itListClasses++)
        {
            pClassElement = CAmClassFactory::getElement(itListClasses->name);
            if (NULL == pClassElement)
            {
                LOG_FN_ERROR(__FILENAME__,__func__,"getElement got failed.");
                return (E_NOT_POSSIBLE);
            }
            mainConnectionString += pClassElement->getMainConnectionString();
        }
    }
    /* if there is no connection then main connection string can be empty also
     * even though it is empty, it needs to be updated in the persistence area */
    LOG_FN_DEBUG(__FILENAME__,__func__,"mainConnectionString is :",mainConnectionString);
    //sample string ::
    //{BASE,source1:sink1;source2:sink2;source3:sink3;}{PHONE,source4:sink4;source5:sink5;source6:sink6;}
    IAmPersistence* pPersistence = CAmPersistenceWrapper::getInstance();
    result = pPersistence->write(keylastMainConnection,mainConnectionString,mainConnectionString.size());
    if(E_OK == result)
    {
        LOG_FN_INFO("persistence write successful for last main connections");
    }
    else
    {
        LOG_FN_WARN("write to persistence failed for last main connections",result);
    }
    return result;
}

am_Error_e CAmControlSend::_storeVolumetoPersistency()
{
    /*write class level volume to the persistency*/
    std::vector<gc_Class_s> listClasses;
    std::vector<gc_Class_s>::iterator itListClasses;
    CAmClassElement* pClassElement = NULL;
    am_Error_e result= E_OK;
    string volumeString;
    std::string keylastClassVolume = "lastClassVolume";

    if (E_OK == mpPolicySend->getListClasses(listClasses))
    {
        for (itListClasses = listClasses.begin(); itListClasses != listClasses.end(); itListClasses++)
        {
            pClassElement = CAmClassFactory::getElement(itListClasses->name);
            if (NULL == pClassElement)
            {
                LOG_FN_ERROR(__FILENAME__,__func__,"getElement got failed.");
                return (E_NOT_POSSIBLE);
            }
            if (!pClassElement->isVolumePersistencySupported())
            {
                LOG_FN_INFO(__FILENAME__,__func__,pClassElement->getName(),
                                "class doesn't support volume persistence so ignoring its volume");
                continue;
            }
            volumeString+= pClassElement->getVolumeString();
        }
    }
    /* if there is no volume set to the sink then volumeString string can be empty
     * even though it is empty, it needs to be updated in the persistence area */
    LOG_FN_DEBUG(__FILENAME__,__func__,"volumeString is :",volumeString);
    //sample string :: {BASE,[sink1:10][sink2:20][*:30]}{PHONE,[sink3:40][sink4:50][*:60]}
    IAmPersistence* pPersistence = CAmPersistenceWrapper::getInstance();
    result = pPersistence->write(keylastClassVolume,volumeString,volumeString.size());
    if(E_OK == result)
    {
        LOG_FN_INFO("persistence write successful at class level volume");
    }
    else
    {
        LOG_FN_WARN("write to persistence failed at class level volume",result);
    }
    return result;
}

am_Error_e CAmControlSend::_restoreVolumefromPersistency()
{
    /*read class level volume from the persistency*/
    std::vector<gc_Class_s> listClasses;
    std::vector<gc_Class_s>::iterator itListClasses;
    CAmClassElement* pClassElement = NULL;
    am_Error_e result= E_UNKNOWN;
    string volumeString;
    std::string keylastClassVolume = "lastClassVolume";

    IAmPersistence* pPersistence = CAmPersistenceWrapper::getInstance();
    result = pPersistence->read(keylastClassVolume,volumeString);
    LOG_FN_DEBUG(__FILENAME__,__func__,"from persistence",volumeString);
    if(E_OK == result)
    {
        std::vector<gc_LastClassVolume_s> listLastClassVolume;
        std::vector<gc_LastClassVolume_s>::iterator itlistLastClassVolume;
        std::vector<string> listClassVolumes;
        std::vector<string>::iterator itlistClassVolumes;
        gc_LastClassVolume_s classVolume;
        string delimiter = "{";
        if(E_OK != _parseString(delimiter,volumeString,listClassVolumes))
        {
            LOG_FN_INFO(__FILENAME__,__func__,"wrong data format in the persistence database");
            return (E_NOT_POSSIBLE);
        }
        for (itlistClassVolumes = listClassVolumes.begin();
                        itlistClassVolumes != listClassVolumes.end(); itlistClassVolumes++)
        {
            std::vector<string> listClass;
            std::vector<string>::iterator itlistClass;
            delimiter = ",";
            if(E_OK != _parseString(delimiter,*itlistClassVolumes,listClass))
            {
                continue;
            }
            for (itlistClass = listClass.begin(); itlistClass != listClass.end(); itlistClass++)
            {
                gc_LastClassVolume_s lastClassVolume;
                lastClassVolume.className = *itlistClass;
                std::vector<string> listSinkInfo;
                std::vector<string>::iterator itListSinkInfo;
                gc_SinkVolume_s sinkInfo;
                delimiter = "[";
                itlistClass++;
                if (itlistClass == listClass.end())
                {
                    LOG_FN_INFO(__FILENAME__,__func__,"wrong class level data format in the persistence database");
                    break;
                }
                if(E_OK != _parseString(delimiter,*itlistClass,listSinkInfo))
                {
                    continue;
                }
                for (itListSinkInfo = listSinkInfo.begin(); itListSinkInfo != listSinkInfo.end(); itListSinkInfo++)
                {
                    std::vector<string> listSinkVolume;
                    std::vector<string>::iterator itListSinkVolume;
                    delimiter = ":";
                    if(E_OK != _parseString(delimiter,*itListSinkInfo,listSinkVolume))
                    {
                        continue;
                    }
                    gc_SinkVolume_s sinkVolume;
                    for(itListSinkVolume = listSinkVolume.begin();
                                    itListSinkVolume != listSinkVolume.end(); itListSinkVolume++)
                    {
                        sinkVolume.sinkName = *itListSinkVolume;
                        itListSinkVolume++;
                        if(itListSinkVolume == listSinkVolume.end())
                        {
                            sinkVolume.sinkName = "";
                            LOG_FN_INFO(__FILENAME__,__func__,"wrong sink level data format in the persistence database");
                            break;
                        }
                        string tmp = *itListSinkVolume;
                        sinkVolume.mainVolume = atoi(tmp.c_str());
                    }
                    if(!sinkVolume.sinkName.empty())
                    {
                        lastClassVolume.listSinkVolume.push_back(sinkVolume);
                    }
                }
                listLastClassVolume.push_back(lastClassVolume);
            }
        }
        for (itlistLastClassVolume = listLastClassVolume.begin();
                itlistLastClassVolume != listLastClassVolume.end(); itlistLastClassVolume++)
        {
            LOG_FN_DEBUG("class name::",itlistLastClassVolume->className);
            pClassElement = CAmClassFactory::getElement(itlistLastClassVolume->className);
            if (NULL == pClassElement)
            {
                LOG_FN_ERROR(__FILENAME__,__func__,"getElement got failed.");
                return (E_NOT_POSSIBLE);
            }
            pClassElement->restoreVolume(*itlistLastClassVolume);
            std::vector<gc_SinkVolume_s>::iterator itlistSinkVolume;
            for (itlistSinkVolume = itlistLastClassVolume->listSinkVolume.begin();
                    itlistSinkVolume != itlistLastClassVolume->listSinkVolume.end(); itlistSinkVolume++)
            {
                LOG_FN_DEBUG("sink name::",itlistSinkVolume->sinkName);
                LOG_FN_DEBUG("sink volume::",itlistSinkVolume->mainVolume);
            }
        }
    }
    else
    {
        LOG_FN_INFO(__FILENAME__,__func__,"read from persistence failed",result);
    }
    return result;
}

am_Error_e CAmControlSend::_storeMainConnectionVolumetoPersistency()
{
    /*write main connection level volume to the persistency*/
    std::vector<gc_Class_s> listClasses;
    std::vector<gc_Class_s>::iterator itListClasses;
    CAmClassElement* pClassElement = NULL;
    string volumeString;
    if (E_OK == mpPolicySend->getListClasses(listClasses))
    {
        for (itListClasses = listClasses.begin(); itListClasses != listClasses.end(); itListClasses++)
        {
            pClassElement = CAmClassFactory::getElement(itListClasses->name);
            if (NULL == pClassElement)
            {
                LOG_FN_ERROR(__FILENAME__,__func__,"getElement got failed.");
                return (E_NOT_POSSIBLE);
            }
            LOG_FN_INFO(__FILENAME__,__func__,"[mainconvol]class name",pClassElement->getName());
            volumeString+= pClassElement->getLastMainConnectionsVolumeString();
        }
    }
    else
    {
        LOG_FN_WARN("Class list empty");
        return E_NOT_POSSIBLE;
    }
    LOG_FN_DEBUG(__FILENAME__,__func__,"main connection volumeString is :",volumeString);
    /*sample string :: {BASE,[source1:sink1=10][source2:sink2=20]}{INT,[source3:sink3=30][source4:sink4=40]}*/
    IAmPersistence* pPersistence = CAmPersistenceWrapper::getInstance();
    am_Error_e result = pPersistence->write(std::string("lastMainConnectionVolume"),
                                            volumeString,volumeString.size());
    if(E_OK == result)
    {
        LOG_FN_INFO("persistence write successful at main connection level volume");
    }
    else
    {
        LOG_FN_WARN("write to persistence failed at main connection level volume",result);
    }
    return result;
}

am_Error_e CAmControlSend::_restoreMainConnectionVolumefromPersistency()
{
    am_Error_e result= E_OK;
    std::string mainConnectionLevelVolumeString;
    std::vector<gc_LastMainConnectionsVolume_s> listLastMainConnectionVolume;
    std::vector<gc_LastMainConnectionsVolume_s>::iterator itlistLastMainConnectionVolume;
    std::vector<string> listClasses;
    std::vector<string>::iterator itListClasses;

    IAmPersistence* pPersistence = CAmPersistenceWrapper::getInstance();
    result = pPersistence->read(std::string("lastMainConnectionVolume"),
                                mainConnectionLevelVolumeString);
    LOG_FN_DEBUG(__FILENAME__,__func__,"main connection volume from persistence",mainConnectionLevelVolumeString);
    /*{BASE,[source1:sink1=20][source2:sink2=30]}{PHONE,[source3:sink3=40][source4:sink4=50]}*/
    if(E_OK == result)
    {
        string delimiter = "{";

        if(E_OK != _parseString(delimiter,mainConnectionLevelVolumeString,listClasses))
        {
            LOG_FN_INFO(__FILENAME__,__func__,"wrong data format in the persistence database");
            return (E_NOT_POSSIBLE);
        }
        for (itListClasses = listClasses.begin(); itListClasses != listClasses.end(); itListClasses++)
        {
            gc_LastMainConnectionsVolume_s mMainConnectionLastVolume;
            std::vector<string> listMainConnection;
            std::vector<string>::iterator itlistMainConnection;

            delimiter = ",";
            if(E_OK != _parseString(delimiter,*itListClasses,listMainConnection))
            {
                continue;
            }
            for (itlistMainConnection = listMainConnection.begin(); itlistMainConnection != listMainConnection.end();
                            itlistMainConnection++)
            {
                std::vector<string> listMainConnectionVol;
                std::vector<string>::iterator itlistMainConnectionVol;

                mMainConnectionLastVolume.className = *itlistMainConnection;
                delimiter = "[";
                itlistMainConnection++;
                if(itlistMainConnection == listMainConnection.end())
                {
                    LOG_FN_INFO(__FILENAME__,__func__,"wrong class information for the main connection level volume "
                                    "in the persistence database");
                    break;
                }
                if(E_OK != _parseString(delimiter,*itlistMainConnection,listMainConnectionVol))
                {
                    continue;
                }
                for (itlistMainConnectionVol = listMainConnectionVol.begin(); itlistMainConnectionVol != listMainConnectionVol.end();
                                itlistMainConnectionVol++)
                {
                    std::vector<string> listMainVolume;
                    std::vector<string>::iterator itListMainVolume;
                    delimiter = "=";
                    if(E_OK != _parseString(delimiter,*itlistMainConnectionVol,listMainVolume))
                    {
                        continue;
                    }
                    gc_LastMainConVolInfo_s lastMainConVolInfo;
                    for(itListMainVolume = listMainVolume.begin(); itListMainVolume != listMainVolume.end();
                                    itListMainVolume++)
                    {
                        lastMainConVolInfo.mainConnectionName = *itListMainVolume;
                        itListMainVolume++;
                        if(itListMainVolume == listMainVolume.end())
                        {
                            lastMainConVolInfo.mainConnectionName = "";
                            LOG_FN_INFO(__FILENAME__,__func__,"wrong main connection information for the main connection level "
                                            "volume in the persistence database");
                            break;
                        }
                        string tmp = *itListMainVolume;
                        lastMainConVolInfo.mainVolume = atoi(tmp.c_str());
                    }
                    if(!lastMainConVolInfo.mainConnectionName.empty())
                    {
                        mMainConnectionLastVolume.listLastMainConVolInfo.push_back(lastMainConVolInfo);
                    }
                }
                listLastMainConnectionVolume.push_back(mMainConnectionLastVolume);
            }
        }
        /*get the class element from the class name and assign the last main connection volume to its member variable
         * and use this information during the main connection establishment
         */
        for (itlistLastMainConnectionVolume = listLastMainConnectionVolume.begin();
                        itlistLastMainConnectionVolume != listLastMainConnectionVolume.end(); itlistLastMainConnectionVolume++)
        {
            std::vector<gc_LastMainConVolInfo_s>::iterator itlistLastMainConVolInfo;
            CAmClassElement* pClassElement = NULL;

            LOG_FN_DEBUG(__FILENAME__,__func__,"display class name:",itlistLastMainConnectionVolume->className);
            pClassElement = CAmClassFactory::getElement(itlistLastMainConnectionVolume->className);
            if (NULL == pClassElement)
            {
                LOG_FN_ERROR(__FILENAME__,__func__,"getElement got failed for class element:",
                             itlistLastMainConnectionVolume->className,"checking next class element");
                continue;
            }
            pClassElement->restoreLastMainConnectionsVolume(*itlistLastMainConnectionVolume);
            for (itlistLastMainConVolInfo = itlistLastMainConnectionVolume->listLastMainConVolInfo.begin();
                            itlistLastMainConVolInfo != itlistLastMainConnectionVolume->listLastMainConVolInfo.end(); itlistLastMainConVolInfo++)
            {
                LOG_FN_DEBUG(__FILENAME__,__func__,"display main connection name :",itlistLastMainConVolInfo->mainConnectionName);
                LOG_FN_DEBUG(__FILENAME__,__func__,"display main volume :",itlistLastMainConVolInfo->mainVolume);
            }
        }
    }
    return result;
}

} /* namespace gc */
} /* namespace am */
