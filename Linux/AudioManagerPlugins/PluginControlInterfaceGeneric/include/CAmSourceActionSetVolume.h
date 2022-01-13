/******************************************************************************
 * @file: CAmSourceActionSetVolume.h
 *
 * This file contains the declaration of router action set volume source
 * (member functions and data members) used to implement the logic of setting
 * the volume of element at router level
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

#ifndef GC_SOURCEACTIONSETVOLUME_H_
#define GC_SOURCEACTIONSETVOLUME_H_

#include "CAmActionCommand.h"

namespace am {
namespace gc {
class CAmSourceElement;
class CAmSourceActionSetVolume : public CAmActionCommand
{
public:
    /**
     * @brief It is the constructor of set volume action at router level.
     * It sets the strings for classification of parameters as expected by this action.
     * Initialize the member variables with default value.
     * @param pElement: pointer to Element
     * @return none
     */
    CAmSourceActionSetVolume(CAmSourceElement* pElement);
    /**
     * @brief It is the destructor of set volume action at router level.
     * @param none
     * @return none
     */
    virtual ~CAmSourceActionSetVolume();
protected:
    /**
     * @brief This API invokes the control receive class function to send request to AM. It
     * register the observer to receive the acknowledgment of action completion.
     * @param none
     * @return E_WAIT_FOR_CHILD_COMPLETION on waiting for async action to be completed
     *         E_NOT_POSSIBLE on error
     *         E_OK on success
     */
    int _execute(void);
    /**
     * @brief This API unregister the observer set in _execute function.
     * @param result: status of child action execution
     * @return E_OK
     */
    int _update(const int result);
    /**
     * @brief In case of failure this API performs the undo operation if parent has requested for undo.
     * @param none
     * @return E_WAIT_FOR_CHILD_COMPLETION on waiting for async action to be completed
     *         E_NOT_POSSIBLE on error
     *         E_OK on success
     */
    int _undo(void);
private:
    int _setRoutingSideVolume(am_volume_t volume, am_volume_t limitVolume);
    // element whose volume need to be changed
    CAmSourceElement* mpSource;

    am_volume_t mOldVolume;
    am_volume_t mOldLimitVolume;
    am_volume_t mRequestedVolume;
    am_volume_t mRequestedLimitVolume;

    CAmActionParam<am_volume_t > mVolumeParam;
    CAmActionParam<am_volume_t > mVolumeStepParam;
    CAmActionParam<am_volume_t > mLimitVolumeParam;
    CAmActionParam<am_time_t > mRampTimeParam;
    CAmActionParam<am_CustomRampType_t > mRampTypeParam;
};

} /* namespace gc */
} /* namespace am */
#endif /* GC_SOURCEACTIONSETVOLUME_H_ */
