/******************************************************************************
 * @file: CAmClassActionConnect.h
 *
 * This file contains the declaration of class connection action connect class
 * (member functions and data members) used to implement the logic of connect
 * at class level
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

#ifndef GC_CLASSACTIONCONNECT_H_
#define GC_CLASSACTIONCONNECT_H_

#include "CAmActionContainer.h"
#include "CAmTypes.h"

namespace am {
namespace gc {

#define DEFAULT_CONNECT_ORDER       (O_NEWEST)

class CAmClassElement;
class CAmMainConnectionElement;

class CAmClassActionConnect : public CAmActionContainer
{

public:
    CAmClassActionConnect(CAmClassElement *pClassElement);
    virtual ~CAmClassActionConnect();
protected:
    int _execute(void);
    int _update(const int result);
private:
    am_Error_e _getMainConnectionElement();
    IAmActionCommand* _createActionMainConnectionSetVolume(CAmMainConnectionElement* pMainConnection);

    CAmClassElement *mpClassElement;
    CAmMainConnectionElement *mpMainConnectionElement;
    CAmActionParam<std::string > mSourceNameParam;
    CAmActionParam<std::string > mSinkNameParam;
    CAmActionParam<gc_Order_e > mOrderParam;
    CAmActionParam<am_CustomConnectionFormat_t > mConnectionFormatParam;
};
}
}
#endif //GC_CLASSACTIONCONNECT_H_
