/*
 * aoap_usbmonitor_subject.cpp
 *
 *  Created on: Jul 19, 2013
 *      Author: Thilo Bjoern Fickel
 *
 *  This source code is proprietary of ADIT
 *  Copyright (C) Advanced Driver Information Technology Joint Venture GmbH
 *  All rights reserved
 */

#include "aoap_usbmonitor_subject.h"
#include "aoap_usbobserver.h"
#include "aoap_logging.h"

using namespace AOAP::Logging;

UsbMonitorSubject::~UsbMonitorSubject()
{
    try
    {
        mObservers.clear();
    }
    catch(...)
    {
        dbgPrintLine(eLogFatal, "Deleting UsbMonitorSubject FATAL ERROR: Exception thrown in clear()");
    }
}

void UsbMonitorSubject::registerObserver(UsbObserver *pObserver)
{
    for (unsigned int i = 0; i < mObservers.size(); i++)
    {
        if (mObservers[i] == pObserver)
        {
            dbgPrintLine(eLogDebug, "registerObserver() Observer already present -> return");
            return;
        }
    }

    mObservers.push_back(pObserver);
}

void UsbMonitorSubject::unregisterObserver(UsbObserver *pObserver)
{
    std::vector<UsbObserver*>::iterator iterator = mObservers.begin();
    while (iterator != mObservers.end())
    {
        if (*iterator)
        {
            if (*iterator == pObserver)
            {
                dbgPrintLine(eLogDebug, "unregisterObserver() Erasing observer %p",
                        pObserver);
                mObservers.erase(iterator);
                return;
            }
        }
        ++iterator;
    }
}

void UsbMonitorSubject::notify(bool attach, unsigned int vendorId,
        unsigned int productId, const std::string& serial, unsigned int devNum)
{
    for (unsigned int i = 0; i < mObservers.size(); i++)
    {
        if (mObservers[i])
        {
            mObservers[i]->update(attach, vendorId, productId, serial, devNum);
        }
    }
}

