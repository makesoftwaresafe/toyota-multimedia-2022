/******************************************************************************
 * @file: CAmClassActionSetVolume.h
 *
 * This file contains the declaration of user action set volume class
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

#ifndef GC_CLASSACTIONSETVOLUME_H_
#define GC_CLASSACTIONSETVOLUME_H_

#include "CAmActionContainer.h"

namespace am {
/**
 * @page setvolumeaction User Action Set Volume
 *
 * Name: CAmClassActionSetVolume<BR>
 * Responsibility: This action sets the source or sink volume.<BR>
 *
 * Mandatory parameters:<BR>
 *  - sourcename: The name of the source.<BR>
 *  - sinkname: The name of the sink.<BR>
 *  - targetvolume: The main volume to be set.<BR>
 * Optional parameters:<BR>
 *  - ramptime: The ramptime for volume change.<BR>
 *  - ramptype: The ramptype for the volume change.<BR>
 *  - timeout: This is the timeout for action execution. If not specified then
 *  DEFAULT_ASYNC_ACTION_TIME is used as timeout.<BR>
 *
 */

namespace gc {

class CAmElement;
class CAmClassElement;
class CAmMainConnectionElement;

class CAmClassActionSetVolume : public CAmActionContainer
{
public:
    CAmClassActionSetVolume(CAmClassElement *pClassElement);
    ~CAmClassActionSetVolume();
protected:

    int _execute(void);
    int _update(int result);
private:
    CAmMainConnectionElement* _getTargetConnection(void);
    am_Error_e _createChildActions(CAmMainConnectionElement* pMainConnection);
    am_Error_e _createSourceSinkActions(void);
    bool _SinkMuteAction();
    CAmClassElement *mpClassElement;
    bool mDirectSinkAction;
    std::string mTargetMainConnectionName;
    // Variables in which policy engine will set the parameters.
    CAmActionParam<am_time_t > mRampTimeParam;
    CAmActionParam<am_CustomRampType_t > mRampTypeParam;
    CAmActionParam<am_volume_t > mVolumeParam;
    CAmActionParam<am_volume_t > mVolumeStepParam;
    CAmActionParam<am_mainVolume_t > mMainVolumeParam;
    CAmActionParam<am_mainVolume_t > mMainVolumeStepParam;
    CAmActionParam<gc_LimitState_e > mOpRequestedParam;
    CAmActionParam<uint32_t > mPatternParam;
    CAmActionParam<std::string > mSinkNameParam;
    CAmActionParam<std::string > mSourceNameParam;
    CAmActionParam<std::string > mConnectionNameParam;
    CAmActionParam<std::string > mClassNameParam;
};

} /* namespace gc */
} /* namespace am */
#endif /* GC_CLASSACTIONASETVOLUME_H_ */
