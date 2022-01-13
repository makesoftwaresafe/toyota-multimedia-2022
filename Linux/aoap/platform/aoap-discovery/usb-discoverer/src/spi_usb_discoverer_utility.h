/************************************************************************
 *
 * \file: spi_usb_discoverer_utility.h
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

#ifndef SPI_USB_DISCOVERER_UTILITY_H_
#define SPI_USB_DISCOVERER_UTILITY_H_

/* *************  includes  ************* */

#include <pthread_adit.h>
#include <adit_typedef.h>
#include <sys_time_adit.h>

#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include <sys/unistd.h>
#include <sys/types.h>

#include <string>
#include <cstring>

#include <sstream>

#include <fcntl.h>

#include <assert.h>

#include <memory> /* use std::unique_ptr & shared_ptr */

#include "spi_usb_discoverer_types.h"


/* *************  defines  ************* */


/* *************  Apple identifiers  ************* */

#define APPLE_VENDOR_ID             0x05AC
#define APPLE_PRODUCT_ID_MIN        0x1200


/* *************  strings definition  ************* */

#define STR_UDEV                    "udev"
#define STR_USB                     "usb"
#define STR_USB_DEVICE              "usb_device"
//#define STR_USB_INTERFACE           "usb_interface"
#define STR_ADD                     "add"
#define STR_REMOVE                  "remove"
#define STR_CHANGE                  "change"

//#define STR_ID_USB_INTERFACES       "ID_USB_INTERFACES"
#define STR_IDVENDOR                "idVendor"
#define STR_IDPRODUCT               "idProduct"
#define STR_SERIAL                  "serial"
#define STR_PRODUCT                 "product"
#define STR_MANUFACTURER            "manufacturer"
#define STR_DEVPATH                 "devpath"
//#define STR_DEVNUM                  "devnum"

#define STR_DEVIVE_CLASS            "bDeviceClass"
#define STR_DEVIVE_SUB_CLASS        "bDeviceSubClass"
#define STR_DEVIVE_PROTOCOL          "bDeviceProtocol"


/* *************  USB definition  ************* */

/*
 * Base Class 09h (Hub)
 * This base class is defined for devices that are USB hubs
 * and conform to the definition in the USB specification.
 */
#define USB_HUB_BASE_CLASS                      9   // 09h
#define USB_HUB_SUB_CLASS                       0   // 00h
#define USB_HUB_PROTOCOL_FS                     0   // 00h
#define USB_HUB_PROTOCOL_HS_TT                  1   // 01h
#define USB_HUB_PROTOCOL_HS_TTS                 2   // 02h

/*
 * VendorId of Unwired Technology (0x2996)
 * which is part of the Unwired Hub.
 */
#define UNWIRED_TECHNOLOGY_VENDOR_ID            "2996"

/*
 * USB Interface class to identify Android Accessory interfaces
 */
#define USB_INTERFACE_CLASS_AOA                 255
#define USB_INTERFACE_SUBCLASS_AOA              255
#define USB_INTERFACE_PROTOCOL_AOA              0

/* LIBUSB_ENDPOINT_IN (0x80) | LIBUSB_REQUEST_TYPE_VENDOR (0x02 << 5) */
#define USB_DISCOVERY_HOST_TO_DEVICE_TYPE       0xc0
/* vendor request to get_protocol */
#define USB_DISCOVERY_GET_AOA_PROTOCOL          51  // 33h

/* defines for thread synchronization */
#define M_SECOND                        1000
#define U_SECOND                        1000000
#define N_SECOND                        1000000000L


/* *************  functions  ************* */

string to_string(std::shared_ptr<t_usbDeviceInformation> inVal);

/**
 * @class SyncContext
 * The class provides helper API's to synchronize threads.
 */
class SyncContext {
public:
    SyncContext(){
        /* initialize the mutex referenced by mConditionMutex */
        if (0 != pthread_mutex_init(&mConditionMutex , NULL)) {
            assert(&mConditionMutex);
        }

        pthread_condattr_t cond_attr;
        pthread_condattr_init(&cond_attr);
        /* set clock attribute of the condition to CLOCK_MONOTONIC */
        pthread_condattr_setclock(&cond_attr, CLOCK_MONOTONIC);
        /* initialize the condition variable referenced by mCondition */
        if (0 != pthread_cond_init(&mConditionVariable, &cond_attr)) {
            assert(&mConditionVariable);
        }
        mResult = -1;
        mConditionDone = false;
        mContinue = true;
    }

    ~SyncContext(){
        /* unblock all thread currently blocked on
        *  the specified condition variable referenced by mConditionVariable
        *  Note:
        *    Call pthread_cond_broadcast() under the protection of
        *    the same mutex that is used with the condition variable being signaled*/
        pthread_cond_broadcast(&mConditionVariable);
        unlock();
        /* destroy the condition object referenced by mConditionVariable */
        pthread_cond_destroy(&mConditionVariable);
        /* destroy the mutex object referenced by mConditionMutex */
        pthread_mutex_destroy(&mConditionMutex);

        mResult = -1;
        mConditionDone = false;
        mContinue = true;
    }

    /**
     * @brief The mutex object referenced by mConditionMutex shall be locked.
     * If the mutex is already locked, the calling thread
     * shall block until the mutex becomes available.
     * */
    /*PRQA: Lint Message 454: This is intention. Mutex will be unlocked in unlock() */
    /*lint -save -e454*/
    int lock(void) {
        /* if successful, pthread_mutex_lock shall return zero;
         * otherwise an error number is returned */
        return pthread_mutex_lock(&mConditionMutex);
    };
    /*lint -restore*/

    /**
     * @brief The mutex object referenced by mConditionMutex shall be released.
     */
    /*PRQA: Lint Message 455: This is intention. Mutex will be locked in lock() */
    /*lint -save -e455*/
    int unlock(void) {
        /* if successful, pthread_mutex_unlock shall return zero;
         * otherwise an error number is returned */
        return pthread_mutex_unlock(&mConditionMutex);
    }
    /*lint -restore*/

    /**
     * @brief Shall unblock at least one thread that are blocked
     * on the specified condition variable referenced by mConditionVariable.
     * Note: Unlocks the mutex referenced by mConditionMutex.
     * Note: Shall be called with mutex locked by the calling thread.
     */
    void signal(void) {
        pthread_cond_signal(&mConditionVariable);
    };

    /**
     * @brief Shall block on the condition variable referenced by mConditionVariable
     * or if the absolute time specified by timeout passes.
     * Note: Unlocks the mutex referenced by mConditionMutex.
     * Note: Shall be called with mutex locked by the calling thread.
     *
     * @param timeout maximum time in millisecond to wait for the condition to get signaled.
     *        value of 0 wait until condition fits.
     * @param outResult request the result of a callback
     *
     * @return 0 if success. Otherwise a negative value.
     * */
    int timed_wait(unsigned int timeout, int* outResult) {
        int ret = 0;
        /* exceed time */
        struct timespec ts;

        /* initialize the exceed time for the wait condition */
        if (timeout > 0) {
            /* get the current absolute system time */
            clock_gettime(CLOCK_MONOTONIC, &ts);
            /* calculate the exceeds time */
            ts.tv_sec += timeout / M_SECOND;
            ts.tv_nsec += (timeout % M_SECOND) * U_SECOND;
            if (ts.tv_nsec >= N_SECOND) {
                ts.tv_sec++;
                ts.tv_nsec -= N_SECOND;
            }
        }

        while (!mConditionDone)
        {
            /* pthread_cond_wait() and pthread_cond_timedwait() atomically release mutex
               and cause the calling thread to block on the condition variable */
            if (timeout > 0) {
                /* wait until condition reached or timeout occur */
                ret = pthread_cond_timedwait(&mConditionVariable, &mConditionMutex, &ts);
            } else {
                /* wait until condition reached */
                ret = pthread_cond_wait(&mConditionVariable, &mConditionMutex);
            }
            /* error handling */
            if (0 != ret) { /* EINTR shall not returned -> zero returned in case we got a signal */
                /* ETIMEDOUT, EPERM or EINVAL occurred */
                if (!mConditionDone) {
                    /* pthread_cond_wait() or pthread_cond_timedwait() return with an error
                     * and predicate was not set */
                    break;  /* come out of while()-loop */
                } else {
                    /* pthread_cond_wait() or pthread_cond_timedwait() return with an error,
                     * but predicate was set */
                    ret = 0;
                }
            }
        } /* while (!mConditionDone) */

        /* return the result of the signal thread, if available */
        if (nullptr != outResult) {
            *outResult = get_result();
        }
        return ret;
    };

    /**
     * @brief Used to store the predicate.
     *
     * @param inValue the value which should be stored
     * */
    void setCondition(const bool inValue) {
        mConditionDone = inValue;
    };
    /**
     * @brief Request the value of the predicate.
     *
     * @return The current predicate.
     * */
    bool getCondition(void) { return mConditionDone; };

    /**
     * @brief Used as indication for the signal thread
     *        to continue or stop execution.
     *
     * @param inValue the value which should be stored
     * */
    void setExecution(const bool inValue) {
        mContinue = inValue;
    };
    /**
     * @brief Request the stored value of the mContinue.
     *
     * @return The current value of private member mContinue.
     * */
    bool getExecution(void) { return mContinue; };

    /**
     * @brief Store an integer value into a private member.
     * Used to store the result of a callback.
     *
     * @param inResult the integer value which should be stored
     * */
    void set_result(int inResult) {
        mResult = inResult;
    };

    /**
     * @brief Request an integer value of the private member.
     * Used to request the result of a callback.
     *
     * @return The value of the stored integer value.
     * At start-up, the private member is set to zero.
     * */
    int get_result(void) const { return mResult; };

private:

    /* the mutex object */
    pthread_mutex_t mConditionMutex;
    /* the condition object */
    pthread_cond_t  mConditionVariable;

    /* mConditionDone shall be set to TRUE even in case of an error */
    bool mConditionDone;
    /* mContinue indicate the signal thread to stop execution */
    bool mContinue;

    /* internal integer variable to store
     * the result of condition */
    int mResult;
};

#endif /* SPI_USB_DISCOVERER_UTILITY_H_ */
