/******************************************************************************
 * @file: CAmMainConnectionActionSetVolume.h
 *
 * This file contains the declaration of main connection action set volume class
 * (member functions and data members) used to implement the logic of setting
 * the volume of the main connection.
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

#ifndef GC_MAINCONNECTIONACTIONSETVOLUME_H_
#define GC_MAINCONNECTIONACTIONSETVOLUME_H_

#include "CAmActionContainer.h"

namespace am {
namespace gc {

class CAmMainConnectionElement;
class CAmElement;
class CAmMainConnectionActionSetVolume : public CAmActionContainer
{
public:
    CAmMainConnectionActionSetVolume(CAmMainConnectionElement* pMainConnection);
    virtual ~CAmMainConnectionActionSetVolume();
protected:
    int _execute(void);
    int _update(int result);
    am_Error_e _getRequestedVolume(am_volume_t& volume);
    am_Error_e _createChildActions(std::map< CAmElement*, gc_volume_s >& mapVolumeElements);

private:
    CAmMainConnectionElement* mpMainConnection;
    std::string mMainConnectionName;
    CAmActionParam<std::string > mSinkNameParam;
    CAmActionParam<std::string > mSourceNameParam;
    CAmActionParam<am_volume_t > mVolumeParam;
    CAmActionParam<am_volume_t > mVolumeStepParam;
    CAmActionParam<am_mainVolume_t > mMainVolumeParam;
    CAmActionParam<am_mainVolume_t > mMainVolumeStepParam;
    CAmActionParam<am_time_t > mRampTimeParam;
    CAmActionParam<am_CustomRampType_t > mRampTypeParam;
    CAmActionParam< bool > mSaveLastVolumeParam;
    CAmActionParam< std::map<uint32_t, gc_LimitVolume_s > >mMapLimitsParam;

};

} /* namespace gc */
} /* namespace am */
#endif /* GC_MAINCONNECTIONACTIONSETLIMITSTATE_H_ */

