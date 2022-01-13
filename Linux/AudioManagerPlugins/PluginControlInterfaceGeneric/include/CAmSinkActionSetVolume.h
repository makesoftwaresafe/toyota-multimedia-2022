/******************************************************************************
 * @file: CAmSinkActionSetVolume.h
 *
 * This file contains the declaration of user action set volume sink
 * (member functions and data members) used to implement the logic of setting
 * the volume of element at user level
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

#ifndef GC_SINKACTIONSETVOLUME_H_
#define GC_SINKACTIONSETVOLUME_H_

#include "CAmActionCommand.h"

namespace am {

namespace gc {

class CAmSinkElement;
class CAmSinkActionSetVolume : public CAmActionCommand
{
public:
    /**
     * @brief It is the constructor of set volume action at user level.
     * It sets the strings for classification of parameters as expected by this action.
     * Initialize the member variables with default value.
     * @param pRoutingManager: pointer to CAmRoutingManager Class object
     * @return none
     */
    CAmSinkActionSetVolume(CAmSinkElement *pSinkElement);
    /**
     * @brief It is the destructor of set volume action at user level.
     * @param none
     * @return none
     */
    ~CAmSinkActionSetVolume();
protected:
    /**
     * @brief This API creates the child action object if element whose volume need to be
     * set is found. It appends the child to base class for execution.
     * If sink is in mute state then only its main volume is updated and action is completed
     * @param none
     * @return E_OK on success
     *         E_NOT_POSSIBLE on error
     */
    int _execute(void);
    /**
     * @brief This API updates the main volume of element if its child action completes successfully.
     * @param result: status of child action execution
     * @return E_OK
     */
    int _update(const int result);
    int _undo(void);
private:
    int _setRoutingSideVolume(am_volume_t volume,
                              am_volume_t offsetVolume, am_volume_t sinkMute);
    CAmSinkElement *mpSink;

    am_volume_t mOldVolume;
    am_volume_t mOldOffsetVolume;
    am_volume_t mOldSinkMute;
    am_volume_t mRequestedVolume;
    am_volume_t mRequestedLimitVolume;
    am_volume_t mRequestedSinkLimitVolume;
    // Variables in which policy engine will set the parameters.
    CAmActionParam<std::string > mSinkNameParam;
    CAmActionParam<am_mainVolume_t > mMainVolumeParam;
    CAmActionParam<am_mainVolume_t > mMainVolumeStepParam;
    CAmActionParam<am_volume_t > mVolumeParam;
    CAmActionParam<am_mainVolume_t > mVolumeStepParam;
    CAmActionParam<am_volume_t > mLimitVolumeParam;
    CAmActionParam<am_time_t > mRampTimeParam;
    CAmActionParam<am_CustomRampType_t > mRampTypeParam;
};

} /* namespace gc */
} /* namespace am */
#endif /* GC_CLASSCTIONSETSINKVOLUME_H_ */
