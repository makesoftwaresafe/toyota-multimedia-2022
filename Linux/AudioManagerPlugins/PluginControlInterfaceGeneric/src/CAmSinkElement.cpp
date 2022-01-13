/******************************************************************************
 * @file: CAmSinkElement.cpp
 *
 * This file contains the definition of sink element class (member functions
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

#include "CAmSinkElement.h"
#include "CAmControlReceive.h"
#include "CAmLogger.h"

namespace am {
namespace gc {

CAmSinkElement::CAmSinkElement(const gc_Sink_s& sinkData, CAmControlReceive* pControlReceive) :
                                CAmElement(sinkData.name, pControlReceive),
                                mpControlReceive(pControlReceive),
                                mSink(sinkData),
                                mMapUserToNormalizedVolume(
                                                sinkData.mapUserVolumeToNormalizedVolume),
                                mMapNormalizedToDecibelVolume(
                                                sinkData.mapNormalizedVolumeToDecibelVolume)
{
    setType (ET_SINK);
    setMinVolume(0);
    setMaxVolume(0);
    setVolume(0);
    CAmElement::setMainVolume(0);
    if( true == mSink.isVolumeChangeSupported )
    {
        setVolumeSupport(true);
        setVolume(convertMainVolumeToVolume(mSink.mainVolume));
        CAmElement::setMainVolume(mSink.mainVolume);
        if(false == mSink.mapUserVolumeToNormalizedVolume.empty())
        {
            setMinVolume(mSink.mapNormalizedVolumeToDecibelVolume.begin()->second *10);
            setMaxVolume(mSink.mapNormalizedVolumeToDecibelVolume.rbegin()->second * 10);
        }
    }
    setPriority(mSink.priority);
}

CAmSinkElement::~CAmSinkElement()
{
    _notifyRemoved();
}

am_Error_e CAmSinkElement::getMainSoundPropertyValue(const am_CustomMainSoundPropertyType_t type,
                                                     int16_t& value) const
{
    return mpControlReceive->getMainSinkSoundPropertyValue(getID(), type, value);
}

am_Error_e CAmSinkElement::setMainSoundPropertyValue(const am_CustomMainSoundPropertyType_t type,
                                                     const int16_t value)
{
    am_MainSoundProperty_s property;
    property.type = type;
    property.value = value;
    return mpControlReceive->changeMainSinkSoundPropertyDB(property, getID());
}

template <typename TPropertyType, typename Tlisttype>
am_Error_e CAmSinkElement::_saturateSoundProperty(
                const TPropertyType soundPropertyType,
                const std::vector<Tlisttype >& listGCSoundProperties, int16_t& soundPropertyValue)
{
    am_Error_e result = E_UNKNOWN;
    typename std::vector<Tlisttype >::const_iterator itListSoundProperties;
    // if property list is empty return error
    if (true == listGCSoundProperties.empty())
    {
        LOG_FN_ERROR(__FILENAME__,__func__," List of sound property is empty");
    }
    else
    {
        for (itListSoundProperties = listGCSoundProperties.begin();
                        itListSoundProperties != listGCSoundProperties.end();
                        ++itListSoundProperties)
        {
            if ((*itListSoundProperties).type == soundPropertyType)
            {
                soundPropertyValue = std::min(std::max(soundPropertyValue,
                        (*itListSoundProperties).minValue),(*itListSoundProperties).maxValue);
                LOG_FN_EXIT(__FILENAME__,__func__," Value:", soundPropertyValue);
                result = E_OK;
                break;
            }
        }
    }
    return result;
}

am_Error_e CAmSinkElement::saturateMainSoundPropertyRange(
                const am_CustomMainSoundPropertyType_t mainSoundPropertyType,
                int16_t& soundPropertyValue)
{
    std::vector<gc_MainSoundProperty_s >::iterator itListSoundProperties;
    LOG_FN_ENTRY(__FILENAME__,__func__,"type:value", mainSoundPropertyType, soundPropertyValue);
    return _saturateSoundProperty<am_CustomMainSoundPropertyType_t, gc_MainSoundProperty_s >(
                    mainSoundPropertyType, mSink.listGCMainSoundProperties, soundPropertyValue);
}

am_Error_e CAmSinkElement::saturateSoundPropertyRange(
                const am_CustomSoundPropertyType_t soundPropertyType, int16_t& soundPropertyValue)
{
    std::vector<gc_MainSoundProperty_s >::iterator itListSoundProperties;
    LOG_FN_ENTRY(__FILENAME__,__func__,"type:value", soundPropertyType, soundPropertyValue);
    return _saturateSoundProperty<am_CustomSoundPropertyType_t, gc_SoundProperty_s >(
                    soundPropertyType, mSink.listGCSoundProperties, soundPropertyValue);
}

template <typename TPropertyType, typename Tlisttype>
bool CAmSinkElement::_isSoundPropertyConfigured(
                const TPropertyType soundPropertyType,
                const std::vector<Tlisttype >& listGCSoundProperties)
{
    bool soundPropertyConfigured = false;
    typename std::vector<Tlisttype >::const_iterator itListSoundProperties;
    for (itListSoundProperties = listGCSoundProperties.begin();
                    itListSoundProperties != listGCSoundProperties.end(); ++itListSoundProperties)
    {
        if ((*itListSoundProperties).type == soundPropertyType)
        {
            soundPropertyConfigured = true;
            break;
        }
    }
    return soundPropertyConfigured;
}

am_Error_e CAmSinkElement::getSoundPropertyValue(const am_CustomSoundPropertyType_t type,
                                                 int16_t& value) const
{
    return mpControlReceive->getSinkSoundPropertyValue(getID(), type, value);
}

am_Error_e CAmSinkElement::setAvailability(const am_Availability_s& availability)
{
    return mpControlReceive->changeSinkAvailabilityDB(availability, getID());
}

am_Error_e CAmSinkElement::getAvailability(am_Availability_s& availability) const
{
    am_Sink_s sinkData;
    am_Error_e result;
    result = mpControlReceive->getSinkInfoDB(getID(), sinkData);
    availability = sinkData.available;
    return result;
}

am_Error_e CAmSinkElement::_register(void)
{
    am_sinkID_t sinkID(0);
    am_Error_e result = E_DATABASE_ERROR;
    if (E_OK == mpControlReceive->enterSinkDB(mSink, sinkID))
    {
        setID(sinkID);
        result = E_OK;
    }
    return result;
}

am_Error_e CAmSinkElement::_unregister(void)
{
    am_Error_e result = E_DATABASE_ERROR;
    if (E_OK == mpControlReceive->removeSinkDB(getID()))
    {
        setID(0);
        result = E_OK;
    }
    return result;
}

am_domainID_t CAmSinkElement::getDomainID(void)
{
    return mSink.domainID;
}

am_Error_e CAmSinkElement::getListConnectionFormats(
                std::vector<am_CustomConnectionFormat_t >& listConnectionFormats)
{
    listConnectionFormats = mSink.listConnectionFormats;
    return E_OK;
}

am_Error_e CAmSinkElement::mainSoundPropertyToSoundProperty(
                const am_MainSoundProperty_s &mainSoundProperty, am_SoundProperty_s& soundProperty)
{
    am_Error_e error = E_DATABASE_ERROR;
    if (mSink.mapMSPTOSP[MD_MSP_TO_SP].find(mainSoundProperty.type) != mSink.mapMSPTOSP[MD_MSP_TO_SP].end())
    {
        soundProperty.type = mSink.mapMSPTOSP[MD_MSP_TO_SP][mainSoundProperty.type];
        soundProperty.value = mainSoundProperty.value;
        error = E_OK;
    }
    return error;
}

am_Error_e CAmSinkElement::soundPropertyToMainSoundProperty(
                const am_SoundProperty_s &soundProperty, am_MainSoundProperty_s& mainSoundProperty)
{
    am_Error_e error = E_DATABASE_ERROR;
    if (mSink.mapMSPTOSP[MD_SP_TO_MSP].find(soundProperty.type) != mSink.mapMSPTOSP[MD_SP_TO_MSP].end())
    {
        mainSoundProperty.type = mSink.mapMSPTOSP[MD_SP_TO_MSP][soundProperty.type];
        mainSoundProperty.value = soundProperty.value;
        error = E_OK;
    }
    return error;
}

am_Error_e CAmSinkElement::upadateDB(
                am_sinkClass_t classId, std::vector<am_SoundProperty_s > listSoundProperties,
                std::vector<am_CustomConnectionFormat_t > listConnectionFormats,
                std::vector<am_MainSoundProperty_s > listMainSoundProperties)
{
    am_Error_e result = E_OK;
    am_Sink_s sinkData;
    std::vector < am_SoundProperty_s > listUpdatedSoundProperties;
    std::vector < am_MainSoundProperty_s > listUpdatedMainSoundProperties;
    std::vector < am_CustomConnectionFormat_t > listUpdatedConnectionFormats;

    std::vector<am_SoundProperty_s >::iterator itListSoundProperties;
    std::vector<am_MainSoundProperty_s >::iterator itListMainSoundProperties;
    std::vector<am_CustomConnectionFormat_t >::iterator itListConnectionFormats;

    std::vector<am_SoundProperty_s >::iterator itListUpdatedSoundProperties;
    std::vector<am_MainSoundProperty_s >::iterator itListUpdatedMainSoundProperties;
    std::vector<am_CustomConnectionFormat_t >::iterator itListUpdatedConnectionFormats;

    mpControlReceive->getSinkInfoDB(getID(), sinkData);
    /*
     * Initialize the list with the already present sound properties
     */
    listUpdatedConnectionFormats = sinkData.listConnectionFormats;
    listUpdatedSoundProperties = sinkData.listSoundProperties;
    listUpdatedMainSoundProperties = sinkData.listMainSoundProperties;

    /*
     * sound properties update. The strategy is as follows
     * 1. Get the list of sound properties from the audio manager database
     * 2. for each property present in the list from routing side update in the list returned from
     * AudioManager Daemon.
     * 3. If the sound property is not present in the configuration ignore it
     */
    for (itListSoundProperties = listSoundProperties.begin();
                    itListSoundProperties != listSoundProperties.end(); itListSoundProperties++)
    {
        am_SoundProperty_s soundProperty = *itListSoundProperties;
        am_MainSoundProperty_s mainSoundProperty;
        if (_isSoundPropertyConfigured(soundProperty.type, mSink.listGCSoundProperties))
        {
            for (itListUpdatedSoundProperties = listUpdatedSoundProperties.begin();
                            itListUpdatedSoundProperties != listUpdatedSoundProperties.end();
                            itListUpdatedSoundProperties++)
            {
                if (itListUpdatedSoundProperties->type == soundProperty.type)
                {
                    itListSoundProperties->value = soundProperty.value;
                    break;
                }
            }
            if (itListUpdatedSoundProperties == listUpdatedSoundProperties.end())
            {
                listUpdatedSoundProperties.push_back(soundProperty);
            }
            LOG_FN_INFO(__FILENAME__,__func__,"converting SP TO MSP", soundProperty.type);
            if (E_OK != soundPropertyToMainSoundProperty(soundProperty, mainSoundProperty))
            {
                continue;
            }
            for (itListUpdatedMainSoundProperties = listUpdatedMainSoundProperties.begin();
                            itListUpdatedMainSoundProperties != listUpdatedMainSoundProperties.end();
                            itListUpdatedMainSoundProperties++)
            {
                if (itListUpdatedMainSoundProperties->type == mainSoundProperty.type)
                {
                    itListUpdatedMainSoundProperties->value = mainSoundProperty.value;
                    break;
                }
            }
            if (itListUpdatedMainSoundProperties == listUpdatedMainSoundProperties.end())
            {
                listUpdatedMainSoundProperties.push_back(mainSoundProperty);
            }
        }
    }
    /*
     * main sound properties update. The strategy is as follows
     * 1. Get the main sound properties from the audio manager database
     * 2. Perform the SP to MSP conversion and update the MSP type:values.
     * 3. For each main sound property present in the new list update the MSP value
     * 4. If the new list has a main sound property type which is not present in the
     * configuration list then ignore it.
     */
    for (itListMainSoundProperties = listMainSoundProperties.begin();
                    itListMainSoundProperties != listMainSoundProperties.end();
                    itListMainSoundProperties++)
    {
        am_MainSoundProperty_s mainSoundProperty = *itListMainSoundProperties;
        if (_isSoundPropertyConfigured(mainSoundProperty.type, mSink.listGCMainSoundProperties))
        {
            for (itListUpdatedMainSoundProperties = listUpdatedMainSoundProperties.begin();
                            itListUpdatedMainSoundProperties != listUpdatedMainSoundProperties.end();
                            itListUpdatedMainSoundProperties++)
            {
                if (itListUpdatedMainSoundProperties->type == mainSoundProperty.type)
                {
                    itListUpdatedMainSoundProperties->value = mainSoundProperty.value;
                    break;
                }
            }
            if (itListUpdatedMainSoundProperties == listUpdatedMainSoundProperties.end())
            {
                listUpdatedMainSoundProperties.push_back(mainSoundProperty);
            }
        }
    }
    /*
     * upadte connection format list
     */
    for (itListConnectionFormats = listConnectionFormats.begin();
                    itListConnectionFormats != listConnectionFormats.end();
                    ++itListConnectionFormats)
    {
        for (itListUpdatedConnectionFormats = listUpdatedConnectionFormats.begin();
                        itListUpdatedConnectionFormats != listUpdatedConnectionFormats.end();
                        itListUpdatedConnectionFormats++)
        {
            if (*itListConnectionFormats == *itListUpdatedConnectionFormats)
            {
                break;
            }
        }
        if (itListUpdatedConnectionFormats == listUpdatedConnectionFormats.end())
        {
            listUpdatedConnectionFormats.push_back(*itListConnectionFormats);
        }
    }
    return mpControlReceive->changeSinkDB(getID(), classId, listUpdatedSoundProperties,
                                          listUpdatedConnectionFormats,
                                          listUpdatedMainSoundProperties);
}

bool CAmSinkElement::isPersistencySupported() const
{
    return mSink.isPersistencySupported;
}
bool CAmSinkElement::isVolumePersistencySupported() const
{
    return mSink.isVolumePersistencySupported;
}

am_Error_e CAmSinkElement::setMainNotificationConfiguration(
                const am_NotificationConfiguration_s& mainNotificationConfiguraton)
{
    return mpControlReceive->changeMainSinkNotificationConfigurationDB(getID(),
                                                                       mainNotificationConfiguraton);
}

am_Error_e CAmSinkElement::notificationDataUpdate(const am_NotificationPayload_s& payload)
{
    mpControlReceive->sendMainSinkNotificationPayload(getID(), payload);
    return E_OK;
}

am_Error_e CAmSinkElement::getListMainNotificationConfigurations(
                std::vector<am_NotificationConfiguration_s >& listMainNotificationConfigurations)
{
    am_Sink_s sinkData;
    am_Error_e result;
    //get the source Info from Database
    result = mpControlReceive->getSinkInfoDB(getID(), sinkData);
    listMainNotificationConfigurations = sinkData.listMainNotificationConfigurations;
    return result;
}

am_Error_e CAmSinkElement::getListNotificationConfigurations(
                std::vector<am_NotificationConfiguration_s >& listNotificationConfigurations)
{
    am_Sink_s sinkData;
    am_Error_e result;
    //get the source Info from Database
    result = mpControlReceive->getSinkInfoDB(getID(), sinkData);
    listNotificationConfigurations = sinkData.listNotificationConfigurations;
    return result;
}

am_Error_e CAmSinkElement::getNotificationConfigurations(
                am_CustomNotificationType_t type,
                am_NotificationConfiguration_s& notificationConfiguration)
{
    std::vector < am_NotificationConfiguration_s > listNotificationConfigurations;
    std::vector<am_NotificationConfiguration_s >::iterator itListNotificationConfigurations;
    am_Error_e result = getListNotificationConfigurations(listNotificationConfigurations);
    if (result == E_OK)
    {
        result = E_UNKNOWN;
        for (itListNotificationConfigurations = listNotificationConfigurations.begin();
                        itListNotificationConfigurations != listNotificationConfigurations.end();
                        ++itListNotificationConfigurations)
        {
            if (itListNotificationConfigurations->type == type)
            {
                notificationConfiguration = *itListNotificationConfigurations;
                result = E_OK;
                break;
            }
        }
    }
    return result;
}

am_Error_e CAmSinkElement::getMainNotificationConfigurations(
                am_CustomNotificationType_t type,
                am_NotificationConfiguration_s& mainNotificationConfiguration)
{
    std::vector < am_NotificationConfiguration_s > listMainNotificationConfigurations;
    std::vector<am_NotificationConfiguration_s >::iterator itListMainNotificationConfigurations;
    am_Error_e result = getListMainNotificationConfigurations(listMainNotificationConfigurations);
    if (result == E_OK)
    {
        result = E_UNKNOWN;
        for (itListMainNotificationConfigurations = listMainNotificationConfigurations.begin();
                        itListMainNotificationConfigurations != listMainNotificationConfigurations.end();
                        ++itListMainNotificationConfigurations)
        {
            if (itListMainNotificationConfigurations->type == type)
            {
                mainNotificationConfiguration = *itListMainNotificationConfigurations;
                result = E_OK;
                break;
            }
        }
    }
    return result;
}

am_sinkClass_t CAmSinkElement::getClassID(void) const
{
    am_Sink_s sink;
    sink.sinkClassID = 0;
    mpControlReceive->getSinkInfoDB(getID(), sink);
    return sink.sinkClassID;
}

void CAmSinkElement::setMainVolume(const am_mainVolume_t mainVolume)
{
    am_Error_e result = E_NOT_POSSIBLE;

    am_mainVolume_t oldMainVolume = getMainVolume();
    if (mainVolume != oldMainVolume)
    {
        result = mpControlReceive->changeSinkMainVolumeDB(mainVolume, getID());
        if(result == E_OK)
        {
            CAmElement::setMainVolume(mainVolume);
        }
        else
        {
            LOG_FN_ERROR(__FILENAME__,__func__,"Failed to update sink volume in DB");
        }
    }
}

am_Error_e CAmSinkElement::saturateVolumeRange(am_mainVolume_t &newVolume)
{
    am_mainVolume_t maxVolume = 0;
    am_mainVolume_t minVolume = 0;
    LOG_FN_ENTRY(__FILENAME__,__func__);
    if (true == mMapUserToNormalizedVolume.empty())
    {
        LOG_FN_ERROR(__FILENAME__,__func__," List of Volume range is empty");
        return E_UNKNOWN;
    }
    //Get the min/max volume range for requested sink
    minVolume = (am_volume_t)mMapUserToNormalizedVolume.begin()->first;
    maxVolume = (am_volume_t)(mMapUserToNormalizedVolume.rend())->first;
    //Satuarate the min/max volume
    newVolume = std::min(std::max(newVolume, minVolume), maxVolume);
    LOG_FN_EXIT(__FILENAME__,__func__," Volume:", newVolume);
    return E_OK;
}

am_Error_e CAmSinkElement::setMuteState(const am_MuteState_e muteState)
{
    am_Error_e result = E_NOT_POSSIBLE;
    am_MuteState_e oldMuteState = getMuteState();
    LOG_FN_ENTRY(__FILENAME__,__func__,"OLD MS=", oldMuteState, "MS=", muteState);
    if (oldMuteState != muteState)
    {
        result  = mpControlReceive->changeSinkMuteStateDB(muteState, getID());
        if(result == E_OK)
        {
            CAmElement::setMuteState(muteState);
        }
    }
    return result;
}

template <typename TFirstVolumeType,typename TSecondVolumeType>
TSecondVolumeType CAmSinkElement::_lookupVolumeForward(const TFirstVolumeType volume,
        const std::map<TFirstVolumeType,TSecondVolumeType>& mapVolume)
{
    typename std::map<TFirstVolumeType, TSecondVolumeType >::const_iterator itMapVolume;
    TFirstVolumeType firstValue = mapVolume.begin()->first;
    TSecondVolumeType secondValue = mapVolume.begin()->second;
    if (volume > mapVolume.begin()->first)
    {
        for (itMapVolume = mapVolume.begin();itMapVolume != mapVolume.end();++itMapVolume)
        {
            if (itMapVolume->first > volume)
            {
                if (itMapVolume->first != firstValue)
                {
                    secondValue = ((((float)itMapVolume->second
                            - (float)secondValue)
                            / ((float)itMapVolume->first - (float)firstValue))
                            * ((float)volume - (float)firstValue)
                            + secondValue);
                    return secondValue;
                }
            }
            secondValue = itMapVolume->second;
            firstValue = itMapVolume->first;
            if(firstValue == volume)
            {
                return secondValue;
            }
        }
    }
    return secondValue;
}


template <typename TFirstVolumeType,typename TSecondVolumeType>
float CAmSinkElement::_lookupVolumeReverse(const TSecondVolumeType volume,
                                const std::map<TFirstVolumeType,TSecondVolumeType>& mapVolume)
{
    typename std::map<TFirstVolumeType, TSecondVolumeType >::const_iterator itMapVolume;
    float firstValue = mapVolume.begin()->first;
    TSecondVolumeType secondValue = mapVolume.begin()->second;

    if (volume > mapVolume.begin()->second)
    {
        for (itMapVolume = mapVolume.begin();itMapVolume != mapVolume.end();++itMapVolume)
        {
            if (itMapVolume->second > volume)
            {
                if (itMapVolume->first != firstValue)
                {
                    firstValue = ((((float)itMapVolume->first
                                    - (float)firstValue)
                                    / ((float)itMapVolume->second - (float)secondValue))
                                    * ((float)volume - (float)secondValue)
                                    + firstValue);
                    return firstValue;
                }
            }
            secondValue = itMapVolume->second;
            firstValue = itMapVolume->first;
            if(secondValue == volume)
            {
                return firstValue;
            }
        }
    }
    return firstValue;
}

am_volume_t CAmSinkElement::convertMainVolumeToVolume(const am_mainVolume_t mainVolume)
{
    am_volume_t DBVolume;
    float decibelVolume;
    float normalisedVolume;

    LOG_FN_ENTRY(__FILENAME__,__func__," user volume:", mainVolume);
    if ((true == mMapUserToNormalizedVolume.empty()) || (true
                    == mMapNormalizedToDecibelVolume.empty()))
    {
        LOG_FN_DEBUG(__FILENAME__,__func__,"  empty mapping", DBVolume);
        return mainVolume;
    }
    // First convert from User Volume to Normalized Volume
    normalisedVolume = _lookupVolumeForward<int16_t,float>(mainVolume,mMapUserToNormalizedVolume);
    // Convert Normalized to Decibel
    decibelVolume = _lookupVolumeForward<float,float>(normalisedVolume,mMapNormalizedToDecibelVolume);
    if (decibelVolume > 0)
    {
        DBVolume = (am_volume_t)(((float)decibelVolume * (float)10.0) + 0.5f);
    }
    else
    {
        DBVolume = (am_volume_t)(((float)decibelVolume * (float)10.0) - 0.5f);
    }
    LOG_FN_EXIT(__FILENAME__,__func__," decibel volume:", DBVolume);
    return DBVolume;
}

am_mainVolume_t CAmSinkElement::convertVolumeToMainVolume(const am_volume_t DBVolume)
{
    float mainVolume;
    float decibelVolume = (float)DBVolume / 10.0;
    float normalisedVolume;

    LOG_FN_ENTRY(__FILENAME__,__func__,"-> decibel volume:", DBVolume);
    if ((true == mMapNormalizedToDecibelVolume.empty()) || (true
                    == mMapUserToNormalizedVolume.empty()))
    {
        LOG_FN_DEBUG(__FILENAME__,__func__,"  empty mapping", (am_mainVolume_t)mainVolume);
        return DBVolume;
    }
    // Convert Decibel to normalized
    normalisedVolume = _lookupVolumeReverse<float,float>(decibelVolume,mMapNormalizedToDecibelVolume);
    // Convert Normalized to User volume
    mainVolume = _lookupVolumeReverse<int16_t,float>(normalisedVolume,mMapUserToNormalizedVolume);
    am_mainVolume_t mainVolumeInt;
    if (mainVolume > 0)
    {
        mainVolumeInt = (am_mainVolume_t)((float)mainVolume + (float)0.5);
    }
    else
    {
        mainVolumeInt = (am_mainVolume_t)((float)mainVolume - (float)0.5);
    }
    LOG_FN_EXIT(__FILENAME__,__func__,"  user volume->", (am_mainVolume_t)mainVolumeInt);
    return (am_mainVolume_t)mainVolumeInt;
}

gc_LimitVolume_s CAmSinkElement::getSinkLimit()
{
    return mSinkLimit;
}

void CAmSinkElement::setSinkLimit(gc_LimitVolume_s limit)
{
    mSinkLimit = limit;
}

} /* namespace gc */
} /* namespace am */
