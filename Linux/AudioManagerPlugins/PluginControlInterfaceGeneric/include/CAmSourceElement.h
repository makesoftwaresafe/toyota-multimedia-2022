/******************************************************************************
 * @file: CAmSourceElement.h
 *
 * This file contains the declaration of source element class (member functions
 * and data members) used as data container to store the information related to
 * source as maintained by controller.
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

#ifndef GC_SOURCEELEMENT_H_
#define GC_SOURCEELEMENT_H_

#include "IAmControlCommon.h"
#include "CAmTypes.h"
#include "CAmElement.h"

namespace am {
namespace gc {
class CAmControlReceive;
class CAmSourceElement : public CAmElement
{
public:
    /**
     * @brief It is the constructor of source element class. Initialize the member
     * variables with default value.It will be invoked during gateway registration in
     * case source of gateway is not registered previously.
     * @param name: name of the source element
     *        pControlReceive: pointer to CAmControlReceive Class object
     * @return none
     */
    CAmSourceElement(const gc_Source_s& sourceConfiguration, CAmControlReceive* pControlReceive);
    /**
     * @brief It is the destructor of source element class.
     * @param none
     * @return none
     */
    virtual ~CAmSourceElement();
    /**
     * @brief This API is used to get the main sound property value of source element as in database.
     * @param type: type of property whose value is required
     *        value: variable in which value will be returned
     * @return E_OK on success
     *         E_UNKNOWN on error
     */
    am_Error_e getMainSoundPropertyValue(const am_CustomMainSoundPropertyType_t type,
                                         int16_t& value) const;
    /**
     * @brief This API is used to set the main sound property value of source element in database.
     * @param type: type of property whose value is required
     *        value: value of the property
     * @return E_OK on success
     *         errors as return by AM
     */
    am_Error_e setMainSoundPropertyValue(const am_CustomMainSoundPropertyType_t type,
                                         const int16_t value);
    /**
     * @brief This API is used to set the main notification configuration of sink element in database.
     * @param mainNotificationConfiguraton: Notification configuration
     * @return E_OK on success
     *         errors as return by AM
     */
    am_Error_e setMainNotificationConfiguration(
                    const am_NotificationConfiguration_s& mainNotificationConfiguraton);

    /**
     * @brief This API is used to update the notification data
     * @param payload: Notification Payload
     * @return E_OK on success
     *         errors as return by AM
     */
    am_Error_e notificationDataUpdate(const am_NotificationPayload_s& payload);

    /**
     * @brief This API is used to get list of Notification Configuration
     * @param listMainNotificationConfigurations: list of Main Notification Configuration
     * @return E_OK on success
     *         errors as return by AM
     */
    am_Error_e getListMainNotificationConfigurations(
                    std::vector<am_NotificationConfiguration_s >& listMainNotificationConfigurations);

    /**
     * @brief This API is used to get Notification Configuration
     * @param notificationConfiguration: Notification configuration
     * @return E_OK on success
     *         errors as return by AM
     */
    am_Error_e getNotificationConfigurations(
                    am_CustomNotificationType_t type,
                    am_NotificationConfiguration_s& notificationConfiguration);

    /**
     * @brief This API is used to get Main Notification Configuration
     * @param mainNotificationConfiguration: Main Notification configuration
     * @return E_OK on success
     *         errors as return by AM
     */
    am_Error_e getMainNotificationConfigurations(
                    am_CustomNotificationType_t type,
                    am_NotificationConfiguration_s& mainNotificationConfiguration);

    /**
     * @brief This API is used to get list of Main Notification Configuration
     * @param listNotificationConfigurations: list of Notification Configuration
     * @return E_OK on success
     *         errors as return by AM
     */
    am_Error_e getListNotificationConfigurations(
                    std::vector<am_NotificationConfiguration_s >& listNotificationConfigurations);

    /**
     * @brief This API is used to get the sound property value of source element as in database.
     * @param soundPropertyType: type of property whose value is required
     *        soundPropertyValue: variable in which value will be returned
     * @return E_OK on success
     *         E_UNKNOWN on error
     */
    am_Error_e getSoundPropertyValue(const am_CustomSoundPropertyType_t type, int16_t& value) const;
    /**
     * @brief This API is used to update the availability of source element.
     *        When this API is called, the datebase is udpated as well as notification
     * @param availability: variable in which availability will be returned
     */
    am_Error_e setAvailability(const am_Availability_s& availability);
    /**
     * @brief This API is used to get the availability of source element as in database.
     * @param availability: variable in which availability will be returned
     */
    am_Error_e getAvailability(am_Availability_s& availability) const;
    /**
     * @brief This API is used to get the source state value of source element as in database.
     * @param state: variable in which source state of element will be returned
     * @return
     */
    am_Error_e getState(int& state) const;

    am_SoundProperty_s mainSoundPropertyToSoundProperty(
                    const am::am_MainSoundProperty_s &mainSoundProperty);

    am_Error_e mainSoundPropertyToSoundProperty(const am_MainSoundProperty_s &mainSoundProperty,
                                                am_SoundProperty_s& soundProperty);
    am_Error_e soundPropertyToMainSoundProperty(const am_SoundProperty_s &soundProperty,
                                                am_MainSoundProperty_s& mainSoundProperty);
    /**
     * @brief This API is used to saturate the main sound property value as per the range specified in configuration file
     * @param soundPropertyType: type of property whose value need to be saturated
     *        soundPropertyValue: variable in which value will be returned
     * @return E_OK on success
     *         E_UNKNOWN on error
     */
    am_Error_e saturateMainSoundPropertyRange(
                    const am_CustomMainSoundPropertyType_t soundPropertyType,
                    int16_t& soundPropertyValue);
    am_Error_e saturateSoundPropertyRange(const am_CustomSoundPropertyType_t soundPropertyType,
                                          int16_t& soundPropertyValue);

    am_domainID_t getDomainID(void);
    am_Error_e getListConnectionFormats(
                    std::vector<am_CustomConnectionFormat_t >& listConnectionFormats);
    am_Error_e upadateDB(am_sourceClass_t classId,
                         std::vector<am_SoundProperty_s > listSoundProperties,
                         std::vector<am_CustomConnectionFormat_t > listConnectionFormats,
                         std::vector<am_MainSoundProperty_s > listMainSoundProperties);

    bool isPersistencySupported() const;
    bool isVolumePersistencySupported() const;

    am_sourceClass_t getClassID(void) const;

    am_volume_t getRoutingSideVolume(am_volume_t internalVolume);
    /*
     * Volume management methods
     */
protected:
    am_Error_e _register(void);
    am_Error_e _unregister(void);
private:
    template <typename TPropertyType, typename Tlisttype>
    am_Error_e _saturateSoundProperty(const TPropertyType soundPropertyType,
                                      const std::vector<Tlisttype >& listGCSoundProperties,
                                      int16_t& soundPropertyValue);
    template <typename TPropertyType, typename Tlisttype>
    bool _isSoundPropertyConfigured(const TPropertyType soundPropertyType,
                                    const std::vector<Tlisttype >& listGCSoundProperties);
    gc_Source_s mSource;
    CAmControlReceive* mpControlReceive;
};

class CAmSourceFactory : public CAmFactory<gc_Source_s, CAmSourceElement >
{
};

} /* namespace gc */
} /* namespace am */
#endif /* GC_SOURCEELEMENT_H_ */
