/******************************************************************************
 * @file: IAmEventObserver.h
 *
 * This file contains the declaration of event observer class (member functions
 * and data members)
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

#ifndef GC_IEVENTOBSERVER_H_
#define GC_IEVENTOBSERVER_H_

namespace am {
namespace gc {

class IAmEventObserver
{
public:
    IAmEventObserver()
    {
    }
    virtual ~IAmEventObserver()
    {
    }
    virtual int update(const int result)=0;
};

} /* namespace gc */
} /* namespace am */
#endif /* GC_IEVENTOBSERVER_H_ */
