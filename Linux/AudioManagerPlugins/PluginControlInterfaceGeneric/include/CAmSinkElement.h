/******************************************************************************
 * @file: CAmSinkElement.h
 *
 * This file contains the declaration of sink element class (member functions
 * and data members) used as data container to store the information related to
 * sink as maintained by controller.
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

#ifndef GC_SINKELEMENT_H_
#define GC_SINKELEMENT_H_

#include "IAmControlCommon.h"
#include "CAmElement.h"
#include "CAmTypes.h"
namespace am {
namespace gc {

class CAmSinkElement : public CAmElement
{
public:
    /**
     * @brief It is the constructor of sink element class. Initialize the member
     * variables with default value.It will be invoked during sink element registration.
     * @param name: name of the sink element
     *        pControlReceive: pointer to CAmControlReceive Class object
     *        sinkConfiguration: details of sink element as defined in configuration file
     * @return none
     */
    CAmSinkElement(const gc_Sink_s& sinkConfiguration, CAmControlReceive* pControlReceive);
    /**
     * @brief It is the destructor of sink element class.
     * @param none
     * @return none
     */
    virtual ~CAmSinkElement();
    /**
     * @brief This API is used to get the main sound property value of sink element as in database.
     * @param soundPropertyType: type of property whose value is required
     *        soundPropertyValue: variable in which value will be returned
     * @return E_OK on success
     *         E_UNKNOWN on error
     */
    am_Error_e getMainSoundPropertyValue(const am_CustomMainSoundPropertyType_t type,
                                         int16_t& value) const;
    /**
     * @brief This API is used to set the main sound property value of sink element in database.
     * @param soundPropertyType: type of property whose value is required
     *        soundPropertyValue: value of the property
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
     * @brief This API is used to get list of Main Notification Configuration
     * @param listNotificationConfigurations: list of Notification Configuration
     * @return E_OK on success
     *         errors as return by AM
     */
    am_Error_e getListNotificationConfigurations(
                    std::vector<am_NotificationConfiguration_s >& listNotificationConfigurations);

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
     * @brief This API is used to get the sound property value of sink element as in database.
     * @param soundPropertyType: type of property whose value is required
     *        soundPropertyValue: variable in which value will be returned
     * @return E_OK on success
     *         E_UNKNOWN on error
     */
    am_Error_e getSoundPropertyValue(const am_CustomSoundPropertyType_t type, int16_t& value) const;
    /**
     * @brief This API is used to update the availability of sink element
     * @param availability: variable in which availability will be returned
     * @return none
     */
    am_Error_e setAvailability(const am_Availability_s& availability);
    /**
     * @brief This API is used to get the availability of sink element as in database.
     * @param availability: variable in which availability will be returned
     * @return none
     */
    am_Error_e getAvailability(am_Availability_s& availability) const;
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
    am_Error_e upadateDB(am_sinkClass_t classId,
                         std::vector<am_SoundProperty_s > listSoundProperties,
                         std::vector<am_CustomConnectionFormat_t > listConnectionFormats,
                         std::vector<am_MainSoundProperty_s > listMainSoundProperties);
    am_Error_e mainSoundPropertyToSoundProperty(const am_MainSoundProperty_s &mainSoundProperty,
                                                am_SoundProperty_s& soundProperty);
    am_Error_e soundPropertyToMainSoundProperty(const am_SoundProperty_s &soundProperty,
                                                am_MainSoundProperty_s& mainSoundProperty);
    am_domainID_t getDomainID();
    am_Error_e getListConnectionFormats(
                    std::vector<am_CustomConnectionFormat_t >& listConnectionFormats);

    bool isPersistencySupported() const;
    bool isVolumePersistencySupported() const;
    am_sinkClass_t getClassID(void) const;
    /*
     * volume related methods
     */
    /**
     * @brief This API is used to set the main volume of sink element in database.
     * @param mainVolume: main volume of element
     * @return none
     */
    void setMainVolume(const am_mainVolume_t mainVolume);
    /**
     * @brief This API is used to saturate the main volume value as per the range specified in configuration file
     * @param newVolume: variable in which value will be returned
     * @return E_OK on success
     *         E_UNKNOWN on error
     */
    am_Error_e saturateVolumeRange(am_mainVolume_t &newVolume);
    /**
     * @brief This API is used to set the mute state of sink element as in database.
     * @param muteState: mute state of element to be set
     * @return none
     */
    am_Error_e setMuteState(const am_MuteState_e muteState);
    /**
     * @brief This API is used to convert the main volume to volume as per range given in configuration file
     * @param mainVolume: main volume of element
     * @return volume of element
     */
    am_volume_t convertMainVolumeToVolume(const am_mainVolume_t mainVolume);
    /**
     * @brief This API is used to convert the volume to main volume as per range given in configuration file
     * @param decibelVolume: volume of element
     * @return main volume of element
     */
    am_mainVolume_t convertVolumeToMainVolume(const am_volume_t decibelVolume);
    gc_LimitVolume_s getSinkLimit();
    void setSinkLimit(gc_LimitVolume_s);
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
    template <typename TFirstVolumeType,typename TSecondVolumeType>
    TSecondVolumeType _lookupVolumeForward(const TFirstVolumeType volume,
                                    const std::map<TFirstVolumeType,TSecondVolumeType>& mapVolume);
    template <typename TFirstVolumeType,typename TSecondVolumeType>
    float _lookupVolumeReverse(const TSecondVolumeType volume,
                               const std::map<TFirstVolumeType,TSecondVolumeType>& mapVolume);

    gc_Sink_s mSink;
    gc_LimitVolume_s mSinkLimit;
    CAmControlReceive* mpControlReceive;
    std::map<int16_t, float > mMapUserToNormalizedVolume;
    std::map<float, float > mMapNormalizedToDecibelVolume;
};

class CAmSinkFactory : public CAmFactory<gc_Sink_s, CAmSinkElement >
{

};

} /* namespace gc */
} /* namespace am */
#endif /* GC_SINKELEMENT_H_ */
