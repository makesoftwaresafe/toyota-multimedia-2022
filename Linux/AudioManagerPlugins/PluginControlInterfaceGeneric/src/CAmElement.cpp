/******************************************************************************
 * @file: CAmElement.cpp
 *
 * This file contains the definition of element class (member functions and
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

#include <algorithm>
#include <map>
#include "CAmElement.h"
#include "CAmControlReceive.h"
#include "CAmLogger.h"

namespace am {
namespace gc {

CAmElement::CAmElement(const std::string& name, CAmControlReceive* pControReceive) :
                                mpControlRecieve(pControReceive),
                                mName(name),
                                mID(0),
                                mType(ET_UNKNOWN),
                                mPriority(0),
                                mVolume(0),
                                mMinVolume(0),
                                mMaxVolume(0),
                                mMainVolume(0),
                                mVolumeSupported(false),
                                mMuteState(MS_UNKNOWN),
                                mInterruptState(IS_UNKNOWN),
                                mState(0),
                                mNumInUse(0)
{
    mOffsetVolume = 0;
    mAvailability.availability = A_UNKNOWN;
    mAvailability.availabilityReason = AR_UNKNOWN;
}

CAmElement::~CAmElement()
{
}

CAmControlReceive* CAmElement::getControlReceive(void)
{
    return mpControlRecieve;
}

std::string CAmElement::getName(void) const
{
    return mName;
}

int CAmElement::setType(const gc_Element_e type)
{
    mType = type;
}

gc_Element_e CAmElement::getType(void) const
{
    return mType;
}

void CAmElement::setID(const uint16_t ID)
{
    mID = ID;
}

uint16_t CAmElement::getID(void) const
{
    return mID;
}

am_Error_e CAmElement::setPriority(const int32_t priority)
{
    mPriority = priority;
    return E_OK;
}

am_Error_e CAmElement::getPriority(int32_t & priority) const
{
    priority =  mPriority;
    return E_OK;
}


am_Error_e CAmElement::setInterruptState(const am_InterruptState_e interruptState)
{
    mInterruptState = interruptState;
    return E_OK;
}

am_Error_e CAmElement::getInterruptState(am_InterruptState_e& interruptState) const
{
    interruptState = mInterruptState;
    return E_OK;
}

am_Error_e CAmElement::setState(int state)
{
    mState = state;
    return E_OK;
}

am_Error_e CAmElement::getState(int& state) const
{
    state = mState;
    return E_OK;
}

am_Error_e CAmElement::setAvailability(const am_Availability_s& availability)
{
    mAvailability = availability;
    return E_NON_EXISTENT;
}

am_Error_e CAmElement::getAvailability(am_Availability_s& availability) const
{
    availability = mAvailability;
    return E_NON_EXISTENT;
}

am_Error_e CAmElement::_register(CAmElement* pObserver)
{
	am_Error_e result = E_UNKNOWN;
	std::vector<CAmElement* >::iterator itListObservers;

	if (pObserver != NULL)
	{
		itListObservers = std::find(mListObservers.begin(), mListObservers.end(), pObserver);
		if (itListObservers == mListObservers.end())
		{
		    /*element not found so push it to the list*/
		    mListObservers.push_back(pObserver);
		    result = E_OK;
		}
		else
		{
		    LOG_FN_INFO(__FILENAME__,__func__,"observer is already registered");
		    result = E_ALREADY_EXISTS;
		}
	}
	else
	{
		//Null pointer
		result = E_NOT_POSSIBLE;
	}
	return result;
}

am_Error_e CAmElement::_deregister(CAmElement* pObserver)
{
	am_Error_e result = E_UNKNOWN;
	std::vector<CAmElement* >::iterator itListObservers;

	/* handle the NULL pointer check */
	if (pObserver != NULL)
	{
		itListObservers = std::find(mListObservers.begin(), mListObservers.end(), pObserver);
		if (itListObservers != mListObservers.end())
		{
			mListObservers.erase(itListObservers);
			result = E_OK;
		}
		else
		{
		    result = E_OK;
		}
	}
	else
	{
	    result = E_OK;
	}

	return result;
}

am_Error_e CAmElement::attach(CAmElement* pSubject)
{
	am_Error_e result = E_UNKNOWN;

	if (pSubject != NULL)
	{
		result = pSubject->_register(this);
		if (result == E_OK)
		{
			mListSubjects.push_back(pSubject);
		}
		else if(result == E_ALREADY_EXISTS)
		{
			/*if observer is already registered then check if subject also registered*/
			std::vector<CAmElement* >::iterator itListSubjects;
			itListSubjects = std::find(mListSubjects.begin(), mListSubjects.end(), pSubject);
			if (itListSubjects != mListSubjects.end())
			{
				LOG_FN_INFO(__FILENAME__,__func__,"subject is already registered");
				result = E_ALREADY_EXISTS;

			}
			else
			{
				/*element not found so push it to the list*/
				mListSubjects.push_back(pSubject);
				result = E_OK;
			}
		}
		else
		{
			//error E_NOT_POSSIBLE
		}
	}
	else
	{
		//Null pointer
		result = E_NOT_POSSIBLE;
	}
	return result;
}

am_Error_e CAmElement::detach(CAmElement* pSubject)
{
	am_Error_e result = E_UNKNOWN;
	std::vector<CAmElement* >::iterator itListSubjects;

	if (pSubject != NULL)
	{
		itListSubjects = std::find(mListSubjects.begin(), mListSubjects.end(), pSubject);
		if (itListSubjects != mListSubjects.end())
		{
			mListSubjects.erase(itListSubjects);  /* remove subject from the observer list */
			result = pSubject->_deregister(this); /* remove observer from the subject list */
		}
		else
		{
			//element not found
			result = E_OK;
		}
	}
	else
	{
		//Null pointer
		result = E_NOT_POSSIBLE;
	}
	return result;
}

void CAmElement::_detachAll()
{
	std::vector<CAmElement* >::iterator itListSubjects;
	itListSubjects = mListSubjects.begin();

	while (itListSubjects != mListSubjects.end())
	{
		(*itListSubjects)->_deregister(this);
		itListSubjects = mListSubjects.erase(itListSubjects);
	}
}

int CAmElement::getObserverCount(void) const
{
	return mListObservers.size();
}

int CAmElement::getObserverCount(gc_Element_e type,
        std::vector<CAmElement*>* pListElements) const
{
    int numObservers = 0;
    for (auto it : mListObservers)
    {
        if (it->getType() == type)
        {
            numObservers++;
            if (pListElements != NULL)
            {
                pListElements->push_back(it);
            }

        }
    }
    return numObservers;
}

int CAmElement::getSubjectCount(void) const
{
	return mListSubjects.size();
}

int CAmElement::update(CAmElement *pNotifierElement, const int result)
{
	return 0;

}

int CAmElement::update(CAmElement *pNotifierElement, am_SoundProperty_s& soundProperty)
{
	return 0;
}

int CAmElement::update(CAmElement *pNotifierElement,const am_mainVolume_t& mainVolume)
{
    return 0;
}

int CAmElement::update(CAmElement *pNotifierElement,const am_ConnectionState_e& state)
{
    return 0;
}

void CAmElement::_notifyRemoved()
{
	LOG_FN_ENTRY(__FILENAME__,__func__);
	std::vector<CAmElement* >::iterator itListObservers;
	itListObservers = mListObservers.begin();

	while (!mListObservers.empty())
	{
		itListObservers = mListObservers.begin();
	    LOG_FN_DEBUG(__FILENAME__,__func__,"notify release resources to element :",(*itListObservers)->getName());
		(*itListObservers)->releaseResources(this);
	}
	LOG_FN_EXIT(__FILENAME__,__func__);
}

int CAmElement::releaseResources(CAmElement *pNotifierElement)
{
	return 0;
}

bool CAmElement::_isFilterMatch(CAmElement* pAmElement,const gc_Element_e&  elementType)const
{
	if (pAmElement->getType() == elementType)
		return true;
	else
		return false;
}

bool CAmElement::_isFilterMatch(CAmElement* pAmElement,const std::string& elementName)const
{
	if (pAmElement->getName() == elementName)
		return true;
	else
		return false;
}

bool CAmElement::_isFilterMatch(CAmElement* pAmElement,const int& elementPriority)const
{
	int priority = 0;

	pAmElement->getPriority(priority);
	if (priority == elementPriority)
		return true;
	else
		return false;
}

bool CAmElement::_isFilterMatch(CAmElement* pAmElement,const struct gc_ElementTypeName_s& elementTypeName)const
{
	if ((pAmElement->getType() == elementTypeName.elementType) && (pAmElement->getName() == elementTypeName.elementName))
		return true;
	else
		return false;
}

bool CAmElement::_isFilterMatch(CAmElement* pAmElement,const struct gc_ElementTypeID_s& elementTypeID)const
{
	if ((pAmElement->getType() == elementTypeID.elementType) && (pAmElement->getID() == elementTypeID.elementID))
		return true;
	else
		return false;
}

/*
 * Volume Management stuff
 */

uint16_t CAmElement::getInUse(void) const
{
    return mNumInUse;
}

void CAmElement::setInUse(const bool inUse)
{
    // request is for setting in USE the element
    if (true == inUse)
    {
        mNumInUse++;
    }
    // request is for un-setting in use the element
    else
    {
        if (0 == mNumInUse)
        {
            LOG_FN_ERROR(__FILENAME__,__func__,"  OUT: already no instance of element in use");
            return;
        }
        mNumInUse--;
    }
    std::vector <CAmElement*>::const_iterator itListChild;
    for(itListChild = mListChildVolumes.begin();
       itListChild != mListChildVolumes.end();++itListChild)
    {
        (*itListChild)->setInUse(inUse);
    }
}

am_Error_e CAmElement::setVolume(const am_volume_t volume)
{
    LOG_FN_ENTRY(__FILENAME__,__func__,volume);
    mVolume = volume;
    return E_OK;
}

am_volume_t CAmElement::getVolume(void) const
{
    int volume = mVolume;
    std::vector <CAmElement*>::const_iterator itListChild;
    for(itListChild= mListChildVolumes.begin();
            itListChild != mListChildVolumes.end();++itListChild)
    {
        volume += (*itListChild)->getVolume();
    }
    return volume;
}

am_Error_e CAmElement::setMinVolume(const am_volume_t minVolume)
{
    LOG_FN_ENTRY(__FILENAME__,__func__,getName(),minVolume);
    mMinVolume = minVolume;
    return E_OK;
}

am_volume_t CAmElement::getMinVolume(void) const
{
    return mMinVolume;
}

am_Error_e CAmElement::setMaxVolume(const am_volume_t maxVolume)
{
    mMaxVolume = maxVolume;
    return E_OK;
}

am_volume_t CAmElement::getMaxVolume(void) const
{
    return mMaxVolume;
}

am_Error_e  CAmElement::setVolumeSupport(const bool volumeSupport)
{
    mVolumeSupported = volumeSupport;
    return E_OK;
}

bool CAmElement::getVolumeSupport() const
{
    return mVolumeSupported;
}

void CAmElement::addVolumeElement(CAmElement* pElement)
{
    mListChildVolumes.push_back(pElement);
    mVolumeSupported |= pElement->getVolumeSupport();
}

void CAmElement::getVolumeChangeElements(am_volume_t requestedVolume,
                  std::map<CAmElement*,gc_volume_s >& listVolumeElements)
{
    LOG_FN_ENTRY(__FILENAME__,__func__,this->getName(),"volume=",
            getVolume(),"requested volume = ",requestedVolume);
    requestedVolume = std::min(std::max(requestedVolume, getMinVolume()), getMaxVolume());
    if(mListChildVolumes.empty() == true)
    {
        if((1 >= getInUse()) && (requestedVolume != getVolume()))
        {
            gc_volume_s volume;
            volume.isOffsetSet = false;
            volume.isvolumeSet = true;
            volume.volume = requestedVolume;
            listVolumeElements[this] = volume;

        }
    }
    else
    {
        std::vector <CAmElement* > listChildElements(mListChildVolumes);
        std::vector <CAmElement*>::iterator itListChild = listChildElements.begin();;
        CAmElement* ListEnd = listChildElements.back();;
        if((*itListChild)->getInUse() <= ListEnd->getInUse())
        {
            std::reverse(listChildElements.begin(),listChildElements.end());
        }
        for(itListChild= listChildElements.begin();itListChild != listChildElements.end();++itListChild)
        {
            std::map<CAmElement*,gc_volume_s > listChildVolumeElements;
            (*itListChild)->getVolumeChangeElements(requestedVolume,listChildVolumeElements);
            {
                requestedVolume -= _getNewVolume((*itListChild)->getVolume(),listChildVolumeElements);;
                listVolumeElements.insert(listChildVolumeElements.begin(),listChildVolumeElements.end());
            }
        }
    }
}

void CAmElement::getLimitVolumeElements( am_volume_t limitVolume,
        std::map<CAmElement*,gc_volume_s >& listVolumeElements)
{
    if(true == mListChildVolumes.empty())
    {
        if(1 >= getInUse())
        {
            am_volume_t volume = getVolume();
            am_volume_t maxLimitPossible;
            /*
             * check if volume element is present in the list
             *
             */
            std::map<CAmElement*,gc_volume_s >::iterator itlistVolumeElements;
            itlistVolumeElements = listVolumeElements.find(this);
            if(itlistVolumeElements != listVolumeElements.end())
            {
                if(true == itlistVolumeElements->second.isvolumeSet)
                {
                    volume = itlistVolumeElements->second.volume;
                }
            }
            maxLimitPossible = getMinVolume() - volume;
            if(limitVolume < maxLimitPossible)
            {
                limitVolume = maxLimitPossible;
            }
            if(limitVolume != getOffsetVolume())
            {
                if(itlistVolumeElements != listVolumeElements.end())
                {
                    itlistVolumeElements->second.isOffsetSet = true;
                    itlistVolumeElements->second.offsetVolume = limitVolume;
                }
                else
                {
                    gc_volume_s limits;
                    limits.isvolumeSet = false;
                    limits.volume=0;
                    limits.isOffsetSet = true;
                    limits.offsetVolume = limitVolume;
                    listVolumeElements[this] = limits;
                }
            }
            else
            {
                if (itlistVolumeElements != listVolumeElements.end())
                {
                    itlistVolumeElements->second.isOffsetSet = true;
                    itlistVolumeElements->second.offsetVolume = limitVolume;
                }
            }
        }
        else
        {
            limitVolume -= getOffsetVolume();
        }
    }
    else
    {
        std::vector <CAmElement*>::iterator itrListChild;
        for(itrListChild= mListChildVolumes.begin();
            itrListChild != mListChildVolumes.end();
            ++itrListChild)
        {
            (*itrListChild)->getLimitVolumeElements(limitVolume,listVolumeElements);
            limitVolume -= _getLimitsSum(listVolumeElements);
        }
    }
}

am_Error_e CAmElement::clearLimitVolume(
                std::map<CAmElement*,gc_volume_s >& listVolumeElements)
{
    am_Error_e result = E_OK;
    if(true == mListChildVolumes.empty())
    {
        if((1 >= getInUse()) ||
           ((1 <= getInUse())&&(AM_MUTE == getOffsetVolume()))

          )
        {
            if(getOffsetVolume()!=0)
            {
                std::map<CAmElement*,gc_volume_s >::iterator itlistVolumeElements;
                itlistVolumeElements = listVolumeElements.find(this);
                if(itlistVolumeElements != listVolumeElements.end())
                {
                    itlistVolumeElements->second.isOffsetSet = true;
                    itlistVolumeElements->second.offsetVolume = 0;
                }
                else
                {
                    gc_volume_s limits;
                    limits.isvolumeSet = false;
                    limits.volume=0;
                    limits.isOffsetSet = true;
                    limits.offsetVolume = 0;
                    listVolumeElements[this] = limits;
                }
            }
        }
    }
    else
    {
        std::vector <CAmElement*>::iterator itrListChild;
        for(itrListChild= mListChildVolumes.begin();
            itrListChild != mListChildVolumes.end();
            ++itrListChild)
        {
            (*itrListChild)->clearLimitVolume(listVolumeElements);
        }
    }
    return E_OK;
}

am_volume_t CAmElement::_getLimitsSum( std::map<CAmElement*,
                                   gc_volume_s >& listVolumeElements) const
{
    am_volume_t limitVolume=0;
    std::map <CAmElement*, gc_volume_s>::iterator itListChild;
    for(itListChild= listVolumeElements.begin();
       itListChild != listVolumeElements.end();++itListChild)
    {
        if(true == itListChild->second.isOffsetSet)
        {
            limitVolume += itListChild->second.offsetVolume;
        }
    }
    return limitVolume;
}

int16_t CAmElement::_getNewVolume(am_volume_t volume,
        std::map<CAmElement*,gc_volume_s >& listVolumeElements) const
{
    std::map <CAmElement*, gc_volume_s>::iterator itListChild;
    for(itListChild= listVolumeElements.begin();
       itListChild != listVolumeElements.end();++itListChild)
    {
        volume -= itListChild->first->getVolume();
        volume += itListChild->second.volume;
    }
    return volume;
}

void CAmElement::setMainVolume(const am_mainVolume_t volume)
{
    LOG_FN_ENTRY(__FILENAME__,__func__,volume);
    mMainVolume = volume;
}

am_mainVolume_t CAmElement::getMainVolume() const
{

    return mMainVolume;
}

am_Error_e CAmElement::setOffsetVolume(const am_volume_t limitVolume)
{
    mOffsetVolume = limitVolume;
    return E_OK;
}

am_volume_t CAmElement::getOffsetVolume() const
{
    am_volume_t limitVolume = mOffsetVolume;
    std::vector <CAmElement*>::const_iterator itListChild;
    for(itListChild= mListChildVolumes.begin();
        itListChild != mListChildVolumes.end();++itListChild)
    {
        limitVolume += (*itListChild)->getOffsetVolume();
    }
    return limitVolume;
}

am_Error_e CAmElement::setMuteState(const am_MuteState_e muteState)
{
    mMuteState = muteState;
    return E_OK;
}

am_MuteState_e CAmElement::getMuteState() const
{
    return mMuteState;
}

void CAmElement::getRootVolumeElements(
                        std::map< CAmElement* , am_volume_t>& mapRootElements)
{
    std::vector <CAmElement*>::const_iterator itListChild;
    for(itListChild= mListChildVolumes.begin();
        itListChild != mListChildVolumes.end();++itListChild)
    {
        gc_Element_e type = (*itListChild)->getType();
        if((type == ET_SINK) ||
           (type == ET_SOURCE))
        {
            mapRootElements[*itListChild] = (*itListChild)->getVolume();
        }
        else
        {
            (*itListChild)->getRootVolumeElements(mapRootElements);
        }
    }
}

} /* namespace gc */
} /* namespace am */
