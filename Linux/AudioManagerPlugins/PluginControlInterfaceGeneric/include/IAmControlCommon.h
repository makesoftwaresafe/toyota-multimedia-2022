/******************************************************************************
 * @file: IAmControlCommon.h
 *
 * This file implements the backward compatibility for AM daemon 4.2 version
 * interfaces
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

#ifndef GC_ICONTROLCOMMON_H_
#define GC_ICONTROLCOMMON_H_

// if audio manager version is not specified from build environment
// assume AM_VERSION_7_0
#if !defined(AM_VERSION_7_0) && !defined(AM_VERSION_4_2)
#define AM_VERSION_7_0
#endif

#include "audiomanagertypes.h"

#if defined (AM_VERSION_7_0)
#include "CAmSocketHandler.h"
#include "CAmDltWrapper.h"
#include "IAmControl.h"
#elif defined (AM_VERSION_4_2)
#include "shared/CAmDltWrapper.h"
#include "shared/CAmSocketHandler.h"
#include "control/IAmControlSend.h"
#include "control/IAmControlReceive.h"

namespace am
{
// map AM_VERSION_4_2 types to AM_VERSION_7_0
    typedef am_ConnectionFormat_e am_CustomConnectionFormat_t;
    typedef am_RampType_e am_CustomRampType_t;
    typedef am_SoundPropertyType_e am_CustomSoundPropertyType_t;
    typedef am_SystemPropertyType_e am_CustomSystemPropertyType_t;
    typedef am_MainSoundPropertyType_e am_CustomMainSoundPropertyType_t;
    typedef am_AvailabilityReason_e am_CustomAvailabilityReason_t;
    typedef am_NotificationType_e am_CustomNotificationType_t;
// define missing datatypes just make new interfaces happy, however they are unused
    typedef uint16_t am_converterID_t;

    struct am_Converter_s
    {
    public:
        am_converterID_t converterID;
        std::string name;
        am_sinkID_t sinkID;
        am_sourceID_t sourceID;
        am_domainID_t domainID;
        std::vector<am_CustomConnectionFormat_t> listSourceFormats;
        std::vector<am_CustomConnectionFormat_t> listSinkFormats;
        std::vector<bool> listConvertionMatrix;
    };

}
#else
#error "current supported AM_BASE_LINE are 7.0 and 4.2"
#endif
static inline am::am_SoundProperty_s mainSoundPropertyToSoundProperty(
                const am::am_MainSoundProperty_s &mainSoundProperty)
{
    am::am_SoundProperty_s soundProperty;
    soundProperty.type = (am::am_CustomSoundPropertyType_t)mainSoundProperty.type;
    soundProperty.value = mainSoundProperty.value;
    return soundProperty;
}

static inline am::am_MainSoundProperty_s soundPropertyToMainSoundProperty(
                const am::am_SoundProperty_s &soundProperty)
{
    am::am_MainSoundProperty_s mainSoundProperty;
    mainSoundProperty.type = (am::am_CustomMainSoundPropertyType_t)soundProperty.type;
    mainSoundProperty.value = soundProperty.value;
    return mainSoundProperty;
}

#endif /* GC_ICONTROLCOMMON_H_ */
