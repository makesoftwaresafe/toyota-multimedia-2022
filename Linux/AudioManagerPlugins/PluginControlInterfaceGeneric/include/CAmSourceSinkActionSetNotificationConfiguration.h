/******************************************************************************
 * @file: CAmSourceSinkActionSetNotificationConfiguration.h
 *
 * This file contains the declaration of router action set notification Configuration
 * (member functions and data members) used to implement the logic of setting
 * the notification configuration in router.
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

#ifndef GC_SOURCESINKACTIONSETNOTIFICATIONCONFIGURATION_H_
#define GC_SOURCESINKACTIONSETNOTIFICATIONCONFIGURATION_H_

#include "CAmLogger.h"
#include "CAmActionCommand.h"
#include "CAmControlReceive.h"

namespace am {
namespace gc {

template <typename Telement>
class CAmSourceSinkActionSetNotificationConfiguration : public CAmActionCommand
{
public:
    CAmSourceSinkActionSetNotificationConfiguration(Telement* pElement) :
                                    CAmActionCommand(
                                                    std::string("CAmActionSetNotificationConfiguration")),
                                    mpElement(pElement)
    {
        this->_registerParam(ACTION_PARAM_NOTIFICATION_CONFIGURATION_TYPE, &mNotificationType);
        this->_registerParam(ACTION_PARAM_NOTIFICATION_CONFIGURATION_STATUS, &mNotificationStatus);
        this->_registerParam(ACTION_PARAM_NOTIFICATION_CONFIGURATION_PARAM, &mNotificationparam);
    }

    virtual ~CAmSourceSinkActionSetNotificationConfiguration()
    {
    }

protected:
    int _execute(void)
    {
        LOG_FN_ENTRY(__FILENAME__,__func__);
        if ((NULL == mpElement) || (false
                        == mNotificationType.getParam(mNotificationConfiguration.type))
            || (false == mNotificationStatus.getParam(mNotificationConfiguration.status))
            || (false == mNotificationparam.getParam(mNotificationConfiguration.parameter)))
        {
            LOG_FN_ERROR(__FILENAME__,__func__,"Parameters not set");
            return E_NOT_POSSIBLE;
        }
        mpElement->getMainNotificationConfigurations(mNotificationConfiguration.type,
                                                    mOldNotificationConfiguration);
        return _setRoutingSideNotificationConfiguration(mNotificationConfiguration);
    }

    int _undo(void)
    {
        return _setRoutingSideNotificationConfiguration(mOldNotificationConfiguration);
    }

    int _update(const int result)
    {
        if (AS_UNDO_COMPLETE == this->getStatus())
        {
            // Result need not be checked as, even if undo is failed nothing can be done.
            mpElement->setMainNotificationConfiguration(mOldNotificationConfiguration);
        }
        else
        {
            if ((E_OK == result) && (this->getStatus() == AS_COMPLETED))
            {
                mpElement->setMainNotificationConfiguration(mNotificationConfiguration);
            }
        }
        //unregister the observer
        mpElement->getControlReceive()->unregisterObserver(this);
        return E_OK;
    }

    void _timeout(void)
    {
        CAmControlReceive* pControlReceive = mpElement->getControlReceive();
        pControlReceive->abortAction();
    }

private:
    int _setRoutingSideNotificationConfiguration(
                    am_NotificationConfiguration_s& notificationConfiguration)
    {
        int result(E_NOT_POSSIBLE);
        CAmControlReceive* pControlReceive = mpElement->getControlReceive();
        if (mpElement->getType() == ET_SOURCE)
        {
            result = pControlReceive->setSourceNotificationConfiguration(mpElement->getID(),
                                                                       mNotificationConfiguration);
        }
        else
        {
            result = pControlReceive->setSinkNotificationConfiguration(
                            mpElement->getID(), mNotificationConfiguration);
        }
        if (result == E_OK)
        {
            pControlReceive->registerObserver(this);
            result = E_WAIT_FOR_CHILD_COMPLETION;
        }
        else
        {
            this->setError(result);
        }
        return result;
    }

    Telement* mpElement;
    am_NotificationConfiguration_s mNotificationConfiguration;
    am_NotificationConfiguration_s mOldNotificationConfiguration;
    CAmActionParam<am_CustomNotificationType_t > mNotificationType;
    CAmActionParam<am_NotificationStatus_e > mNotificationStatus;
    CAmActionParam<int16_t > mNotificationparam;
}
;

} /* namespace gc */
} /* namespace am */
#endif /* GC_SOURCESINKACTIONSETNOTIFICATIONCONFIGURATION_H_ */
