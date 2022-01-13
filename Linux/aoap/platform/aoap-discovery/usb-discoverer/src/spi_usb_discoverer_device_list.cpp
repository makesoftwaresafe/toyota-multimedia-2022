
/************************************************************************
 *
 * \file: spi_usb_discoverer_device_list.cpp
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
 * \copyright (c) 2013 Advanced Driver Information Technology.
 * This code is developed by Advanced Driver Information Technology.
 * Copyright of Advanced Driver Information Technology, Bosch, and DENSO.
 * All rights reserved.
 *
 * \see <related items>
 *
 * \history
 *
 ***********************************************************************/


/* *************  includes  ************* */

#include <adit_logging.h>
#include "spi_usb_discoverer_device_list.h"

#include <sys/prctl.h>
#include <sys/eventfd.h>
#include <errno.h>

/* *************  defines  ************* */

LOG_IMPORT_CONTEXT(spi_usb_discoverer)

/*static*/SyncContext UsbDeviceList::mSyncAoapSupportThread;

/*static*/UsbDeviceList* UsbDeviceList::pmUsbDeviceList = NULL;

/*static*/UsbDeviceList* UsbDeviceList::getInstance(void) {
    if (!pmUsbDeviceList) {
        pmUsbDeviceList = new UsbDeviceList();

        /* set initial value to 1 because we are the first one which creates pmUsbDeviceList */
        pmUsbDeviceList->mRefCount = 1;
    }
    return pmUsbDeviceList;
}

/*static*/void UsbDeviceList::deleteInstance(void) {
    if (pmUsbDeviceList) {
        pmUsbDeviceList->putRef();
    }
}

UsbDeviceList::UsbDeviceList(void) {
    if (0 == pthread_mutex_init(&mMutex, NULL)) {
        mUsbDeviceListCreated = true;
    } else {
        LOG_ERROR((spi_usb_discoverer, "UsbDeviceList() pthread_mutex_init(mMutex) failed. errno=%d", errno));
        mUsbDeviceListCreated = false;
    }

    mMonitorEventFd = 0;
    mAoapSupportThreadId = 0;
    mRefCount = 0;
}

UsbDeviceList::~UsbDeviceList(void) {
    getLock();
    /* delete all queued device list items */
    for(int i=0; (true != mUsbDevices.empty()); i++)
    {
        delete (mUsbDevices.front());
        mUsbDevices.pop_front();
    }
    mUsbDevices.clear();

    releaseLock();
    if (true == mUsbDeviceListCreated) {
        pthread_mutex_destroy(&mMutex);
        mUsbDeviceListCreated = false;
    }

    mMonitorEventFd = 0;

    /* Previous pthread_join() is not necessary,
     *  because thread was marked as detached. */
    mAoapSupportThreadId = 0;
}

bool UsbDeviceList::getLock(void) {
    if (true == mUsbDeviceListCreated) {
        /*PRQA: Lint Message 454: This is intention. Mutex will be unlocked in releaseLock() */
        /*lint -save -e454*/
        if (0 == pthread_mutex_lock(&mMutex)) {
            return true;
        }
        /*lint -restore*/
    }
    LOG_ERROR((spi_usb_discoverer, "UsbDeviceList::getLock() Failed to lock mMutex"));
    return false;
}

bool UsbDeviceList::releaseLock(void) {
    if (true == mUsbDeviceListCreated) {
        /*PRQA: Lint Message 455: This is intention. Mutex will be locked in getLock() */
        /*lint -save -e455*/
        if (0 == pthread_mutex_unlock(&mMutex)) {
            return true;
        }
        /*lint -restore*/
    }
    LOG_ERROR((spi_usb_discoverer, "UsbDeviceList::releaseLock() Failed to unlock mMutex"));
    return false;
}

void UsbDeviceList::addDevice(UsbDevice* pDevice, int32_t inEventFd) {
    /* add UsbDevice to internal device list */
    mUsbDevices.push_back(pDevice);

    LOGD_DEBUG((spi_usb_discoverer, "addDevice() UsbDevice=%p sn=%s added to UsbDeviceList(size=%zu)",
            pDevice, pDevice->getSerial().c_str(), mUsbDevices.size()));

    pthread_attr_t attr;
    if (0 == pthread_attr_init(&attr)) {
        /* Mark the thread identified by mAoapSupportThreadId as detached.
         * When a detached thread terminates, its resources are automatically released back to the system.
         * There is no need for pthread_join(). */
        if (0 == pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED)) {

            /* The unique_ptr retains sole ownership of the object through a pointer
             * and destroys that object when the unique_ptr goes out of scope. */
            std::unique_ptr<aoapSupportThreadParam> threadParam(new aoapSupportThreadParam);
            /* caller of addDevice owns aoapSupportThreadParam */

            threadParam->monitorEventFd = inEventFd;
            threadParam->pMe = this;
            threadParam->pUsbDevInfo = pDevice->getUsbDeviceInformation();

            /* get lock so that aoapSupportThread has to wait
             * until we release the ownership to aoapSupportThreadParam */
            if (0 == mSyncAoapSupportThread.lock()) {
                mSyncAoapSupportThread.setCondition(false);
                mSyncAoapSupportThread.setExecution(true);

                /* notify that we want to acquire to access the UsbDeviceList instance
                 * in thread aoapSupportThread which we create */
                getRef();

                /* return a pointer to aoapSupportThreadParam */
                if (0 != pthread_create(&mAoapSupportThreadId, &attr, &aoapSupportThread, threadParam.get())) {
                    LOG_ERROR((spi_usb_discoverer, "addDevice() create aoapSupportThread failed"));
                    /* if aoapSupportThread could not created,
                     * aoapSupportThreadParam will be destroy when addDevice goes out of scope */

                     putRef();
                } else {
                    /* caller of addDevice returns a pointer to aoapSupportThread
                     * and releases the ownership */
                    threadParam.release();
                    int res = -1;
                    int err = mSyncAoapSupportThread.timed_wait(0, &res);
                    if (0 == err) {
                        LOGD_DEBUG((spi_usb_discoverer, "addDevice() aoapSupportThread created %s",
                            (res == 0)?("and synchronized"):("but synchronization failed")));
                    } else {
                        LOG_ERROR((spi_usb_discoverer, "addDevice() Wait for thread synchronization failed err=%d", err));
                        mSyncAoapSupportThread.setExecution(false);
                    }
                }
                mSyncAoapSupportThread.unlock();
            } else {
                LOG_ERROR((spi_usb_discoverer, "addDevice() failed to acquire lock"));
            }
        } else {
            LOG_ERROR((spi_usb_discoverer, "addDevice() set thread aoapSupportThread as detached failed"));
        }
        pthread_attr_destroy(&attr);
    } else {
        LOG_ERROR((spi_usb_discoverer, "addDevice() initialize thread attributes for aoapSupportThread failed"));
    }
}

void UsbDeviceList::removeDevice(UsbDevice* pDevice) {
    uint32_t i = 0;
    std::deque<UsbDevice*>::iterator it;
    if ((pDevice) && (true != mUsbDevices.empty())) {
        for (i=0; i<mUsbDevices.size(); i++)
        {
            for (it = mUsbDevices.begin(); it < mUsbDevices.end(); it++)
            {
                if (pDevice == *it) {
                    mUsbDevices.erase(it);
                    // do not delete here
//                    delete (*it);
                    pDevice = nullptr;
                    return;
                }
            } /* for */
        } /* for */
    } else {
        LOG_ERROR((spi_usb_discoverer, "removeDevice() UsbDeviceList(size=%zu) is empty", mUsbDevices.size()));
    }
}

UsbDevice* UsbDeviceList::findDevice(uint32_t vendorId, uint32_t productId, const std::string& serial) {
    uint32_t i = 0;

    if (true != mUsbDevices.empty()) {
        for (i=0; i<mUsbDevices.size(); i++)
        {
            if ( (mUsbDevices[i])
                 && (mUsbDevices[i]->getVendorId() == vendorId)
                 && (mUsbDevices[i]->getProductId() == productId)
                 && (SPI_USB_DISCOVERY_SUCCESS == std::strcmp(mUsbDevices[i]->getSerial().c_str(), serial.c_str())) ) {
                 /* found device */
                LOGD_DEBUG((spi_usb_discoverer, "findDevice() found UsbDevice=%p", mUsbDevices[i]));
                return mUsbDevices[i];
            }
        }/* for */
    } else {
        LOGD_DEBUG((spi_usb_discoverer, "findDevice() cannot find device, UsbDeviceList is empty"));
    }

    return NULL;
}

UsbDevice* UsbDeviceList::findDeviceById(const char* sysPath) {
    uint32_t i = 0;

    if (true != mUsbDevices.empty()) {
        for (i=0; i<mUsbDevices.size(); i++)
        {
            if ( (mUsbDevices[i]) && (SPI_USB_DISCOVERY_SUCCESS == mUsbDevices[i]->getSysPath().compare(sysPath))) {
                /* found device */
                LOGD_DEBUG((spi_usb_discoverer, "findDeviceById(sysPath) found UsbDevice=%p (devNum=%u) at sysPath=%s",
                        mUsbDevices[i], mUsbDevices[i]->getDevNum(), mUsbDevices[i]->getSysPath().c_str()));
                return mUsbDevices[i];
            } else if (mUsbDevices[i]) {
                /* device mUsbDevices[i] is not the device we are looking for */
            } else {
                LOG_WARN((spi_usb_discoverer, "findDeviceById(sysPath) UsbDevice=NULL"));
            }
        } /* for */
    } else {
        LOGD_DEBUG((spi_usb_discoverer, "findDeviceById(sysPath) cannot find device, UsbDeviceList is empty"));
    }

    return nullptr;
}

UsbDevice* UsbDeviceList::findDeviceById(const uint32_t devNum) {
    uint32_t i = 0;

    if (true != mUsbDevices.empty()) {
        for (i=0; i<mUsbDevices.size(); i++)
        {
            if ( (mUsbDevices[i]) && (mUsbDevices[i]->getDevNum() == devNum) ) {
                /* found device */
                LOGD_DEBUG((spi_usb_discoverer, "findDeviceById(devNum) found UsbDevice=%p (devNum=%u) at sysPath=%s",
                        mUsbDevices[i], mUsbDevices[i]->getDevNum(), mUsbDevices[i]->getSysPath().c_str()));
                return mUsbDevices[i];
            } else if (mUsbDevices[i]) {
                /* device mUsbDevices[i] is not the device we are looking for */
            } else {
                LOG_WARN((spi_usb_discoverer, "findDeviceById(devNum) UsbDevice=NULL"));
            }
        }
    } else {
        LOGD_DEBUG((spi_usb_discoverer, "findDeviceById(devNum) cannot find device, UsbDeviceList is empty"));
    }

    return nullptr;
}

UsbDevice* UsbDeviceList::findDeviceById(const char* sysPath, const uint32_t devNum) {
    uint32_t i = 0;

    if (true != mUsbDevices.empty()) {
        for (i=0; i<mUsbDevices.size(); i++)
        {
            if ( (mUsbDevices[i]) && (SPI_USB_DISCOVERY_SUCCESS == mUsbDevices[i]->getSysPath().compare(sysPath)) ) {
                if (mUsbDevices[i]->getDevNum() == devNum) {
                    /* found device */
                    LOGD_DEBUG((spi_usb_discoverer, "findDeviceById() found UsbDevice=%p (devNum=%u) at sysPath=%s",
                            mUsbDevices[i], mUsbDevices[i]->getDevNum(), mUsbDevices[i]->getSysPath().c_str()));
                    return mUsbDevices[i];
                } else {
                    LOGD_DEBUG((spi_usb_discoverer, "findDeviceById() found UsbDevice=%p at sysPath=%s, but devNum=%u does not match to %u",
                            mUsbDevices[i], mUsbDevices[i]->getSysPath().c_str(), mUsbDevices[i]->getDevNum(), devNum));
                }
            } else if (mUsbDevices[i]) {
                /* device mUsbDevices[i] is not the device we are looking for */
            } else {
                LOG_WARN((spi_usb_discoverer, "findDeviceById() UsbDevice=NULL"));
            }
        }
    } else {
        LOGD_DEBUG((spi_usb_discoverer, "findDeviceById() cannot find device, UsbDeviceList is empty"));
    }

    return nullptr;
}

void* UsbDeviceList::aoapSupportThread(void* p) {

    int32_t res = SPI_USB_DISCOVERY_SUCCESS;
    int32_t err = 0;

    prctl(PR_SET_NAME, "spiAoapSupportThread", 0, 0, 0);

    /* get lock before synchronize with aoapSupportThreadParam */
    if (0 != (err = mSyncAoapSupportThread.lock())) {
        LOG_ERROR((spi_usb_discoverer, "aoapSupportThread() failed to acquire lock err=%d", err));
        res = err;
    }

    /* The object (aoapSupportThreadParam) is destroyed and its memory deallocated when:
     *  - unique_ptr managing the object is destroyed
     *  - or unique_ptr managing the object is assigned another pointer via operator= or reset(). */
    std::unique_ptr<aoapSupportThreadParam> threadParam(std::move(static_cast<aoapSupportThreadParam*>(p)));
    /* now aoapSupportThread owned aoapSupportThreadParam */

    if ((threadParam == nullptr) || (threadParam->pMe == nullptr)
        || (threadParam->pUsbDevInfo == nullptr)) {
        LOG_ERROR((spi_usb_discoverer, "aoapSupportThread() input pointer or pthread_create member are invalid"));
        res = SPI_USB_DISCOVERY_BAD_PARAMETER;
    }

    /* set condition result */
    mSyncAoapSupportThread.set_result(res);
    /* set predicate even in case of an error */
    mSyncAoapSupportThread.setCondition(true);
    /* check if we can continue with execution */
    if (!mSyncAoapSupportThread.getExecution()) {
        LOG_WARN((spi_usb_discoverer, "aoapSupportThread() caller thread indicate to stop execution"));
        res = SPI_USB_DISCOVERY_ERROR;
    }

    /* signal that sync was done and unlock */
    mSyncAoapSupportThread.signal();
    /* unlock mutex only if we were able to lock the mutex */
    if (0 == err) {
        mSyncAoapSupportThread.unlock();
    }

    if (res == SPI_USB_DISCOVERY_SUCCESS) {
        /* lock access to the UsbDeviceList */
        if (threadParam->pMe->getLock()) {
            if (true != threadParam->pMe->mUsbDevices.empty()) {
                /* search by sysPath and devNum to find the correct UsbDevice in device list */
                UsbDevice* pUsbDevice = threadParam->pMe->findDeviceById(threadParam->pUsbDevInfo->sysPath.c_str(), threadParam->pUsbDevInfo->devNum);
                if (pUsbDevice) {
                    /* found device in internal list, assume it is still available */

                    LOG_INFO((spi_usb_discoverer, "aoapSupportThread(mNumDevices=%zu) Found UsbDevice %s (devNum=%u) to check for AOAP",
                            threadParam->pMe->mUsbDevices.size(), to_string(threadParam->pUsbDevInfo).c_str(), threadParam->pUsbDevInfo->devNum));

                    /* check for AOAP support */
                    res =  pUsbDevice->checkAoapSupport();
                    if ((res == SPI_USB_DISCOVERY_SUCCESS) || (res == SPI_USB_DISCOVERY_NO_AOAP_SUPPORT)) {
                        if (true != threadParam->pUsbDevInfo->aoapSupported) {
                            LOGD_DEBUG((spi_usb_discoverer, "aoapSupportThread() UsbDevice %s does not support AOA protocol ",
                                    to_string(threadParam->pUsbDevInfo).c_str()));
                        }
                        /* Ownership is hand over to event list */
                        t_monitorEventListItem* eventItem = new t_monitorEventListItem(EventAttach, threadParam->pUsbDevInfo);

                        /* Add event to event list */
                        if (SPI_USB_DISCOVERY_SUCCESS == (res = EventList::getInstance()->addEventItem(eventItem))) {
                            uint64_t event = EVENTFD_EVENT_CHECK_LIST;

                            /* trigger eventfd */
                            if (threadParam->monitorEventFd >= 0) {
                                if (eventfd_write(threadParam->monitorEventFd, event) != 0) {
                                    LOG_ERROR((spi_usb_discoverer, "aoapSupportThread() eventfd_write(fd=%d) failed",
                                            threadParam->monitorEventFd));
                                } else {
                                    LOG_INFO((spi_usb_discoverer, "aoapSupportThread() check of device %s done",
                                            to_string(threadParam->pUsbDevInfo).c_str()));
                                }
                            } else {
                                LOG_ERROR((spi_usb_discoverer, "aoapSupportThread() mMonitorEventFd=%d is invalid",
                                        threadParam->monitorEventFd));
                            }
                        } else {
                            LOG_ERROR((spi_usb_discoverer, "aoapSupportThread() Add EVENT_CHECK_LIST failed=%d", res));
                        }
                    } else {
                        LOG_ERROR((spi_usb_discoverer, "aoapSupportThread() checkAoapSupport failed=%d", res));
                    }
                } else {
                    LOG_ERROR((spi_usb_discoverer, "aoapSupportThread() Could not find UsbDevice in internal list"));
                }
            } else {
                LOG_ERROR((spi_usb_discoverer, "aoapSupportThread() UsbDeviceList is empty"));
            }
            /* release access to UsbDeviceList */
            threadParam->pMe->releaseLock();
        } else {
            LOG_ERROR((spi_usb_discoverer, "aoapSupportThread() Acquire lock failed"));
        }
    }

    auto pCpyMe = threadParam->pMe;
    /* destroy aoapSupportThreadParam instance */
    threadParam = nullptr;
    /* delete our self if main-thread already dropped
     * and we are the last sub-thread */
    pCpyMe->putRef();

    LOGD_DEBUG((spi_usb_discoverer, "aoapSupportThread() exit"));
    return nullptr;
}

UsbDeviceList::aoapSupportThreadParam::aoapSupportThreadParam()
{
    pMe = nullptr;
    pUsbDevInfo = nullptr;
    monitorEventFd = -1;
}
UsbDeviceList::aoapSupportThreadParam::~aoapSupportThreadParam()
{
}



