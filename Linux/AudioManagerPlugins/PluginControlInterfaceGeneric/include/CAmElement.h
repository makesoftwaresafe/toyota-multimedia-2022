/******************************************************************************
 * @file: CAmElement.h
 *
 * This file contains the declaration of element class (member functions and
 * data members) used as base class for source, sink & gateway element classes
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

#ifndef GC_ELEMENT_H_
#define GC_ELEMENT_H_

#include "CAmTypes.h"
#include "IAmControlCommon.h"
#include "CAmLogger.h"
#include <algorithm>
#include <vector>
#include <set>
using namespace std;
namespace am {
namespace gc {

#define DEFAULT_ELEMENT_PRIORITY    (100)
class CAmControlReceive;
class CAmElement;

static am_Error_e unregisterElement(CAmElement* pElement);
static am_Error_e registerElement(CAmElement* pElement);


#define SHARED_COUNT 1

class CAmElement
{
public:
    /**
     * @brief It is the constructor of element class. Initialize the member
     * variables with default value. It will be invoked during creation of any
     * element (gateway/source/sink).
     * @param name: name of the element
     *        pControlReceive: pointer to CAmControlReceive Class object
     * @return none
     */
    CAmElement(const std::string& name, CAmControlReceive* pControlReceive);
    /**
     * @brief It is the destructor of element class.
     * @param none
     * @return none
     */
    virtual ~CAmElement();
    virtual CAmControlReceive* getControlReceive(void);
    /**
     * @brief This API is used to set the type of element
     * @return type of element
     */
    virtual int setType(const gc_Element_e type);
    /**
     * @brief This API is used to get the type of element
     * @return type of element
     */
    virtual gc_Element_e getType(void) const;
    /**
     * @brief This API is used to get the name of element
     * @param none
     * @return element name
     */
    virtual std::string getName(void) const;
    /**
     * @brief This API is used to set the ID of element
     * @param ID: element ID as in DB
     * @return none
     */
    virtual void setID(const uint16_t ID);
    /**
     * @brief This API is used to get the ID of element
     * @param none
     * @return element ID as in DB
     */
    virtual uint16_t getID(void) const;
    /**
     * @brief This API is used to get the priority of element as defined in configuration file.
     * @param[in] priority priority of element
     * @return E_OK
     */
    virtual am_Error_e setPriority(const int32_t priority);
    /**
     * @brief This API is used to get the priority of element as defined in configuration file.
     * @param priority: variable in which priority of element will be returned
     * @return E_OK
     */
    virtual am_Error_e getPriority(int32_t& priority) const;
    virtual am_Error_e setState(int state);
    virtual am_Error_e getState(int& state) const;
    virtual am_Error_e setInterruptState(const am_InterruptState_e interruptState);
    virtual am_Error_e getInterruptState(am_InterruptState_e& interruptState) const;
    /**
     * @brief This API is used to update the availability of element as in database.
     * It is just the dummy implementation and actual implementation will be provided by derived element class.
     * @param availability: variable in which availability will be returned
     * @return E_NOT_POSSIBLE because derived class should implement this interface as valid interface
     */
    virtual am_Error_e setAvailability(const am_Availability_s& availability);
    /**
     * @brief This API is used to get the availability of element as in database.
     * It is just the dummy implementation and actual implementation will be provided by derived element class.
     * @param availability: variable in which availability will be returned
     * @return E_NOT_POSSIBLE because derived class should implement this interface as valid interface
     */
    virtual am_Error_e getAvailability(am_Availability_s& availability) const;
    virtual int update(CAmElement *pNotifierElement,const int result);
    virtual int update(CAmElement *pNotifierElement,am_SoundProperty_s& soundProperty);
    virtual int update(CAmElement *pNotifierElement,const am_mainVolume_t& mainVolume);
    virtual int update(CAmElement *pNotifierElement,const am_ConnectionState_e& state);

    virtual int releaseResources(CAmElement *pNotifierElement);
    am_Error_e attach(CAmElement* pSubject);
    am_Error_e detach(CAmElement* pSubject);
    int getObserverCount(void) const;
    int getObserverCount(gc_Element_e, std::vector<CAmElement*>* = NULL) const;
    int getSubjectCount(void) const;

    template <typename TupdateParam>
    void notify(TupdateParam updateData)
    {
    	vector<CAmElement* >::iterator itListObservers = mListObservers.begin();
    	for(; itListObservers != mListObservers.end(); ++itListObservers)
    	{
    		LOG_FN_DEBUG(__FILENAME__,__func__,"notify update to element :",(*itListObservers)->getName());
    		(*itListObservers)->update(this,updateData);
    	}
    }
    /*Searching of elements*/
    template <typename TelemntFilterType>
    void getListElements(TelemntFilterType elementFilter,
                         std::vector<CAmElement* >& listOfSubjects)const
    {
        vector<CAmElement*>::const_iterator itListSubjects =
                mListSubjects.begin();
        for (; itListSubjects != mListSubjects.end(); ++itListSubjects)
        {
            if (_isFilterMatch(*itListSubjects, elementFilter))
            {
                LOG_FN_DEBUG(__FILENAME__, __func__, "element found :",
                        (*itListSubjects)->getName());
                /*add element to the list*/
                std::vector<CAmElement*>::const_iterator it = std::find(
                        listOfSubjects.begin(), listOfSubjects.end(),
                        (const CAmElement*)(*itListSubjects));
                if(it == listOfSubjects.end())
                {
                    listOfSubjects.push_back(*itListSubjects);
                }
            }
            (*itListSubjects)->getListElements(elementFilter, listOfSubjects);
        }
    }

    friend am_Error_e registerElement(CAmElement* pElement);
    friend am_Error_e unregisterElement(CAmElement* pElement);

    /*
     * volume Managements related methods
     */
    virtual am_Error_e  setVolume(const am_volume_t volume);
    virtual am_volume_t getVolume() const;
    virtual am_Error_e  setMinVolume(const am_volume_t volume);
    virtual am_volume_t getMinVolume() const;
    virtual am_Error_e  setMaxVolume(const am_volume_t volume);
    virtual am_volume_t getMaxVolume() const;
    virtual am_Error_e  setVolumeSupport(const bool volumeSupport);
    virtual bool getVolumeSupport() const;
    void addVolumeElement(CAmElement* pElement);
    virtual void getVolumeChangeElements(am_volume_t requestedVolume,
                      std::map<CAmElement*,gc_volume_s >& listVolumeElements);
    uint16_t getInUse(void) const;
    virtual void setInUse(const bool inUse);

    virtual void setMainVolume(const am_mainVolume_t volume);
    virtual am_mainVolume_t getMainVolume() const;
    virtual am_Error_e setOffsetVolume(const am_volume_t limitVolume);
    virtual am_volume_t getOffsetVolume(void) const;
    virtual am_Error_e setMuteState(const am_MuteState_e muteState);
    virtual am_MuteState_e getMuteState() const;
    virtual void getLimitVolumeElements( am_volume_t limitVolume,
            std::map<CAmElement*,gc_volume_s >& listVolumeElements);
    virtual am_Error_e clearLimitVolume(
                    std::map<CAmElement*,gc_volume_s >& listVolumeElements);
    virtual void getRootVolumeElements(std::map<CAmElement* , am_volume_t>& maprootElements);

protected:
    /**
     * @brief This API is used to set the ID of element
     * @param ID: element ID as in DB
     * @return none
     */
    virtual am_Error_e _register(void)
    {
        return E_OK;
    }
    virtual am_Error_e _unregister(void)
    {
        return E_OK;
    }
    am_Error_e _deregister(CAmElement* pObserver);
    am_Error_e _register(CAmElement* pObserver);
    void  _notifyRemoved();
    void _detachAll();
    bool _isFilterMatch(CAmElement* pAmElement,const gc_Element_e&  elementType)const;
    bool _isFilterMatch(CAmElement* pAmElement,const std::string& elementName)const;
    bool _isFilterMatch(CAmElement* pAmElement,const int& elementPriority)const;
    bool _isFilterMatch(CAmElement* pAmElement,const gc_ElementTypeName_s& elementTypeName)const;
    bool _isFilterMatch(CAmElement* pAmElement,const struct gc_ElementTypeID_s& elementTypeID)const;

private:
    CAmControlReceive* mpControlRecieve;
    std::string mName;
    uint16_t mID;
    gc_Element_e mType;
    int32_t mPriority;
    am_MuteState_e mMuteState;
    int mState;
    am_InterruptState_e mInterruptState;
    am_Availability_s mAvailability;
    /* List of the registered observer*/
    std::vector<CAmElement* > mListObservers;
    /* List of the subjects*/
    std::vector<CAmElement* > mListSubjects;
    /*
     * Volume management related members
     */
    uint16_t mNumInUse;
    am_volume_t mVolume;
    am_volume_t mMinVolume;
    am_volume_t mMaxVolume;
    bool    mVolumeSupported;
    std::vector <CAmElement*> mListChildVolumes;

    am_mainVolume_t mMainVolume;
    am_volume_t mOffsetVolume;
    am_volume_t _getNewVolume(am_volume_t volume,
                              std::map<CAmElement*,gc_volume_s >& listVolumeElements) const;
    am_volume_t _getLimitsSum(std::map<CAmElement*,
                              gc_volume_s >& listVolumeElements) const;

};

static am_Error_e registerElement(CAmElement* pElement)
{
    return pElement->_register();
}

static am_Error_e unregisterElement(CAmElement* pElement)
{
    return pElement->_unregister();
}

template <typename TconstructorParam, typename Telement>
class CAmFactory
{
public:
    CAmFactory()
    {
    }

    virtual ~CAmFactory()
    {
    }

    static am_Error_e destroyElement(void)
    {
        typename std::map<std::string, Telement* >::iterator itMapElements;
        for (itMapElements = mMapElements.begin(); itMapElements != mMapElements.end();
                        ++itMapElements)
        {
            delete itMapElements->second;
        }
        mMapElements.clear();
        return E_OK;
    }

    static am_Error_e destroyElement(const std::string name)
    {
        am_Error_e returnValue = E_OK;
        /*
         * first search if the element with such a name
         * is present in the map
         */
        Telement* pElement = getElement(name);
        if (pElement != NULL)
        {
            if (E_OK == unregisterElement(pElement))
            {
                mMapElements.erase(name);
                delete pElement;
            }
        }
        return returnValue;
    }

    static am_Error_e destroyElement(uint16_t ID)
    {
        am_Error_e returnValue = E_OK;
        /*
         * first search if the element with such a name
         * is present in the map
         */
        Telement* pElement = getElement(ID);
        if (pElement != NULL)
        {

            if (E_OK == unregisterElement(pElement))
            {
                mMapElements.erase(pElement->getName());
                delete pElement;
            }
        }
        return returnValue;
    }

    static Telement* getElement(std::string name)
    {
        Telement* pElement = NULL;
        typename std::map<std::string, Telement* >::iterator itMapElements;
        itMapElements = mMapElements.find(name);
        if (itMapElements != mMapElements.end())
        {
            pElement = itMapElements->second;
        }
        return pElement;
    }

    static Telement* getElement(uint16_t ID)
    {
        Telement* pElement = NULL;
        if (ID != 0)
        {
            typename std::map<std::string, Telement* >::iterator itMapElements;
            for (itMapElements = mMapElements.begin(); itMapElements != mMapElements.end();
                            ++itMapElements)
            {
                if (itMapElements->second->getID() == ID)
                {
                    pElement = itMapElements->second;
                    break;
                }
            }
        }
        return pElement;
    }

    static void getListElements(std::vector<Telement* >& listElements)
    {
        typename std::map<std::string, Telement* >::iterator itMapElements;
        for (itMapElements = mMapElements.begin(); itMapElements != mMapElements.end();
                        ++itMapElements)
        {
            listElements.push_back(itMapElements->second);
        }
    }

    static Telement* createElement(const TconstructorParam& t, CAmControlReceive* pControlReceive)
    {
        int returnValue;
        /*
         * first search if the element with such a name
         * is present in the map
         */
        Telement* pElement = getElement(t.name);
        if (pElement == NULL)
        {
            pElement = new Telement(t, pControlReceive);
            if (pElement != NULL)
            {

                if (E_OK != registerElement(pElement))
                {
                    delete pElement;
                    pElement = NULL;
                }
            }
            if (pElement)
            {
                LOG_FN_DEBUG(__FILENAME__,__func__,t.name);
                mMapElements[t.name] = pElement;
            }
        }
        return pElement;
    }

private:
    static std::map<std::string, Telement* > mMapElements;
};
template <typename TconstructorParam, typename Telement>
std::map<std::string, Telement* > CAmFactory<TconstructorParam, Telement >::mMapElements;

} /* namespace gc */
} /* namespace am */
#endif /* GC_ELEMENT_H_ */
