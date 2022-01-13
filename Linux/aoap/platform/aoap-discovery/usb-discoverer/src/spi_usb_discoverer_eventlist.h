/************************************************************************
 *
 * \file: spi_usb_discoverer_eventlist.h
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
 ***********************************************************************/

#ifndef SPI_USB_DISCOVERER_EVENTLIST_H_
#define SPI_USB_DISCOVERER_EVENTLIST_H_


#include <adit_typedef.h>
#include <sys_time_adit.h>
#include <pthread_adit.h>

#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include <sys/unistd.h>
#include <sys/types.h>

#include <cstring>
#include <deque>

#include "spi_usb_discoverer_utility.h"
#include "spi_usb_discoverer_types.h"

/* *************  defines  ************* */

/* *************  eventFd definition  ************* */

#define EVENTFD_EVENT_CHECK_LIST                1

/**
 * @enum MonitorEvents
 * Definition of events which will be handled by class EventList.
 */
typedef enum MonitorEvents {
    EventStop   = 0,        // use to indicate stop monitoring
    EventAttach,            // use in case udev detects an attach event
    EventDetach,            // use in case udev detects a detach event - not implemented yet.
    EventUnknown
} t_monitorEvents;

/**
 * @struct MonitorEventListItem
 * Definition of the structure which will be stored in the event list of class EventList.
 */
typedef struct MonitorEventListItem {
    /* event which shall be handled */
    t_monitorEvents eventToHandle;

    /* prefer to use class UsbDevice
     * instead structure t_usbDeviceInformation */
    std::shared_ptr<t_usbDeviceInformation> pUsbDevParam;

    MonitorEventListItem(t_monitorEvents inEvent, std::shared_ptr<t_usbDeviceInformation> inUsbDevParam) {
        eventToHandle = inEvent;
        pUsbDevParam = inUsbDevParam;
    }
    ~MonitorEventListItem() {
    }
} t_monitorEventListItem;


/* *************  functions  ************* */

class EventList
{
public:
    /**
     * @brief Get the singleton instance of EventList.
     *
     * If it does not exist, it will be created
     *
     * @return A pointer to the singleton. Cannot be NULL
     */
    static EventList* getInstance(void);

    /**
     * @brief Deletes the singleton instance of EventList if it exits
     */
    static void deleteInstance(void);

    /**
     * @brief Get a lock before modifying this list
     *
     * @return true when it was successful otherwise false
     */
    bool getLock();

    /**
     * @brief Release the lock when modifying this list is done
     *
     * @return true when it was successful otherwise false
     */
    bool releaseLock();

    /**
     * @brief Add the event in the event list mEventItemList
     *
     * @param inEventItem The event which shall be added to the event list
     * @return 0 if success. Otherwise negative value.
     */
    int addEventItem(t_monitorEventListItem* inEventItem);

    /**
     * @brief Returns the first event from the event list mEventItemList
     *
     * @return The pointer to the event. If event list is empty, NULL returned
     */
    t_monitorEventListItem* getEventItem();
private:
    EventList(void);
    virtual ~EventList(void);

    /* the singleton instance */
    static EventList* pmEventList;

    /* event list which shall be checked by usbMonitorThread */
    std::deque<t_monitorEventListItem*> mEventItemList;

    /* Mutex to protect access while adding or removing devices */
    pthread_mutex_t mEventListMutex;

    /* flag to indicate that the mutex was created */
    bool mEventListMutexCreated;
};  // class EventList


#endif /* SPI_USB_DISCOVERER_EVENTLIST_H_ */
