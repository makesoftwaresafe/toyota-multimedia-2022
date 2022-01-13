/******************************************************************************
 * @file: CAmTriggerQueue.h
 *
 * This file contains the Event Queue. As the name suggest Event Queue will
 * Queue all the triggers which will be forwarded to the Policy engine
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
#ifndef GC_TRIGGERQUEUE_H_
#define GC_TRIGGERQUEUE_H_

#include "IAmControlCommon.h"
#include "CAmTypes.h"

namespace am {

namespace gc {

struct gc_TriggerElement_s
{
    virtual ~gc_TriggerElement_s()
    {
    }
};

struct gc_UnRegisterElementTrigger_s : public gc_TriggerElement_s
{
    std::string elementName;
    am_Error_e unRegisterationStatus;
};

struct gc_DomainRegisterationCompleteTrigger_s : public gc_TriggerElement_s
{
    std::string domainName;
};
struct gc_AllDomainRegisterationCompleteTrigger_s : public gc_TriggerElement_s
{
    am_Error_e status;
};

struct gc_ConnectTrigger_s : public gc_TriggerElement_s
{
    std::string sourceName;
    std::string sinkName;
    std::string className;
};
struct gc_DisconnectTrigger_s : public gc_TriggerElement_s
{
    std::string sourceName;
    std::string sinkName;
    std::string className;
};
struct gc_SinkVolumeChangeTrigger_s : public gc_TriggerElement_s
{
    std::string sinkName;
    am_mainVolume_t volume;
};

struct gc_SourceVolumeChangeTrigger_s : public gc_TriggerElement_s
{
    std::string sourceName;
    am_mainVolume_t volume;
};
struct gc_SinkMuteTrigger_s : public gc_TriggerElement_s
{
    std::string sinkName;
    am_MuteState_e muteState;
};

struct gc_SinkSoundPropertyTrigger_s : public gc_TriggerElement_s
{
    std::string sinkName;
    am_MainSoundProperty_s mainSoundProperty;
};

struct gc_SourceSoundPropertyTrigger_s : public gc_TriggerElement_s
{
    std::string sourceName;
    am_MainSoundProperty_s mainSoundProperty;
};

struct gc_RegisterElementTrigger_s : public gc_TriggerElement_s
{
    std::string elementName;
    am_Error_e RegisterationStatus;
};

struct gc_AvailabilityChangeTrigger_s : public gc_TriggerElement_s
{
    std::string elementName;
    am_Availability_s availability;
};

struct gc_SourceInterruptChangeTrigger_s : public gc_TriggerElement_s
{
    std::string sourceName;
    am_InterruptState_e interrptstate;
};

struct gc_SystemPropertyTrigger_s : public gc_TriggerElement_s
{
    am_SystemProperty_s systemProperty;
};

struct gc_ConnectionStateChangeTrigger_s : public gc_TriggerElement_s
{
    std::string connectionName;
    am_ConnectionState_e connectionState;
    am_Error_e status;
};

struct gc_NotificationConfigurationTrigger_s : public gc_TriggerElement_s
{
    std::string name;
    am_NotificationConfiguration_s notificatonConfiguration;

};

struct gc_NotificationDataTrigger_s : public gc_TriggerElement_s
{
    std::string name;
    am_NotificationPayload_s notificatonPayload;

};

class CAmTriggerQueue
{
public:
        am_Error_e queue(gc_Trigger_e triggerType,gc_TriggerElement_s* triggerData);
        am_Error_e pushTop(gc_Trigger_e triggerType,gc_TriggerElement_s* triggerData);
        gc_TriggerElement_s* dequeue(gc_Trigger_e& triggerType );
        static CAmTriggerQueue* getInstance();
private:
        CAmTriggerQueue();
        std::vector<std::pair<gc_Trigger_e, gc_TriggerElement_s* > > mlistTrigger;
        static CAmTriggerQueue* mpTriggerQueue;
};

} /* namespace gc */

} /* namespace am */



#endif /* GC_TRIGGER_H_ */
