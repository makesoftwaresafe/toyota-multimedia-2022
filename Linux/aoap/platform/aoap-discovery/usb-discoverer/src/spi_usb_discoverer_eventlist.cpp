/************************************************************************
 *
 * \file: spi_usb_discoverer_eventlist.cpp
 *
 * \version: $Id:$
 *
 * \release: $Name:$
 *
 * <brief description>.
 * <detailed description>
 * \component: SPI Discovery
 *
 * \author: D. Girnus / ADIT/SW2 / dgirnus@de.adit-jv.com
 *
 * \copyright (c) 2015 Advanced Driver Information Technology.
 * This code is developed by Advanced Driver Information Technology.
 * Copyright of Advanced Driver Information Technology, Bosch, and DENSO.
 * All rights reserved.
 *
 * \see <related items>
 *
 * \history
 *
 ***********************************************************************/

#include <adit_logging.h>
#include "spi_usb_discoverer_eventlist.h"

#include <errno.h>
#include <sys/prctl.h>
#include <sys/eventfd.h>

/* *************  defines  ************* */

LOG_IMPORT_CONTEXT(spi_usb_discoverer)

/* *************  functions  ************* */

/*static*/EventList* EventList::pmEventList = NULL;


/*static*/EventList* EventList::getInstance(void) {
    if (!pmEventList) {
        pmEventList = new EventList();
    }
    return pmEventList;
}

/*static*/void EventList::deleteInstance(void) {
    if (pmEventList) {
        delete (pmEventList);
        pmEventList = nullptr;
    }
}

EventList::EventList(void) {
    if (0 == pthread_mutex_init(&mEventListMutex, NULL)) {
        mEventListMutexCreated = true;
    } else {
        LOG_ERROR((spi_usb_discoverer, "EventList()  pthread_mutex_init(&mEventListMutex failed. errno=%d", errno));
        mEventListMutexCreated = false;
    }
}

EventList::~EventList(void) {
    getLock();
    /* delete all queued event list items */
    for(int i=0; (true != mEventItemList.empty()); i++)
    {
        delete (mEventItemList.front());
        mEventItemList.pop_front();
    }
    mEventItemList.clear();

    releaseLock();
    if (true == mEventListMutexCreated) {
        pthread_mutex_destroy(&mEventListMutex);
        mEventListMutexCreated = false;
    }
}

bool EventList::getLock(void) {
    if (true == mEventListMutexCreated) {
        /*PRQA: Lint Message 454: This is intention. Mutex will be unlocked in releaseLock() */
        /*lint -save -e454*/
        if (0 == pthread_mutex_lock(&mEventListMutex)) {
            return true;
        }
        /*lint -restore*/
    }
    LOG_ERROR((spi_usb_discoverer, "EventList::getLock() Failed to lock mEventListMutex"));
    return false;
}

bool EventList::releaseLock(void) {
    if (true == mEventListMutexCreated) {
        /*PRQA: Lint Message 455: This is intention. Mutex will be locked in getLock() */
        /*lint -save -e455*/
        if (0 == pthread_mutex_unlock(&mEventListMutex)) {
            return true;
        }
        /*lint -restore*/
    }
    LOG_ERROR((spi_usb_discoverer, "EventList::releaseLock() Failed to unlock mEventListMutex"));
    return false;
}

int EventList::addEventItem(t_monitorEventListItem* inEventItem) {
    int res = SPI_USB_DISCOVERY_SUCCESS;

    /* lock first, so that inEventItem cannot be modified anyway */
    if (getLock()) {
        if (inEventItem) {
            /* check event and add EventStop in front of the mEventItemList */
            switch(inEventItem->eventToHandle)
            {
                case EventStop:
                {
                    /* add EventStop to the front of the event list */
                    mEventItemList.push_front(inEventItem);
                    break;
                }
                case EventAttach:
                {
                    /* add event to event list */
                    mEventItemList.push_back(inEventItem);
                    break;
                }
                default:
                {
                    LOG_ERROR((spi_usb_discoverer, "addEventItem() Unknown event=%d", inEventItem->eventToHandle));
                    res = SPI_USB_DISCOVERY_ERROR;
                    break;
                }
            } // switch
        } else {
            LOG_ERROR((spi_usb_discoverer, "addEventItem() Invalid parameter inEventItem=%p", inEventItem));
            res = SPI_USB_DISCOVERY_BAD_PARAMETER;
        }

        releaseLock();
    } else {
        LOG_ERROR((spi_usb_discoverer, "addEventItem() Acquire lock mEventListMutex failed"));
    }
    return res;
}

t_monitorEventListItem* EventList::getEventItem() {
    t_monitorEventListItem* retValue = nullptr;

    if (getLock()) {
        /* check if the event list is empty */
        if (true != mEventItemList.empty()) {
            LOGD_DEBUG((spi_usb_discoverer, "getEventItem() events in list = %zu", mEventItemList.size()));

            /* get the first event in the event list */
            retValue = mEventItemList.front();

            /* remove the event from the event list */
            mEventItemList.pop_front();
        } else {
            /* no events stored. return null */
            LOGD_DEBUG((spi_usb_discoverer, "getEventItem() mUsbDeviceListEvents is empty"));
        }

        releaseLock();
    } else {
        LOG_ERROR((spi_usb_discoverer, "getEventItem() Acquire lock mEventListMutex failed"));
    }
    return retValue;
}

