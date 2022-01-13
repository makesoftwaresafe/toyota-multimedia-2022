/******************************************************************************
 * @file: CAmXmlConfigParser.cpp
 *
 * This file contains the definition of xml configuration reader class (member functions
 * and data members) used to implement the logic of parsing the configuration file.
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

#include "CAmXmlConfigParser.h"
#include <stdlib.h>
#include <string>
#include <string.h>
#include <vector>
#include <map>
#include <algorithm>
#include <limits.h>
#include <libxml/xmlschemas.h>
#include <libxml/xmlschemastypes.h>
#include "CAmLogger.h"

namespace am {
namespace gc {

using namespace std;

#define PARSE_TAG_MISMATCH  -1
#define PARSE_ERROR         -2
#define PARSE_SUCCESS        0

std::map<std::string, int >* IAmXmlNode::mpMapEnumerations = NULL;
gc_Configuration_s* IAmXmlNode::mpConfiguration = NULL;

xmlNodePtr getNextElementNode(xmlNodePtr pCurrent)
{
    xmlNodePtr pNext = (pCurrent != NULL) ? pCurrent->next : NULL;
    while (pNext != NULL && pNext->type != XML_ELEMENT_NODE)
    {
        pNext = pNext->next;
        if (pNext == NULL)
        {
            break;
        }
    }
    return pNext;
}

class IAmXmlNodeAttribute
{
public:
    virtual ~IAmXmlNodeAttribute()
    {
    }
    virtual int parse(xmlDocPtr pDocument, xmlNodePtr pCurrent)=0;
};

class CAmNodeAttribute : public IAmXmlNodeAttribute
{
public:
    CAmNodeAttribute(std::string attributeName) :
                                    mattributeName(attributeName)
    {
    }
    int parse(xmlDocPtr pDocument, xmlNodePtr pCurrent)
    {
        xmlChar* pKey = xmlGetProp(pCurrent, (xmlChar*)(mattributeName.c_str()));
        if (pKey)
        {
            _copyAttribute(std::string((char*)pKey));
            xmlFree(pKey);
        }
        return 0;
    }
protected:
    virtual void _copyAttribute(std::string pString)
    {
    }
private:
    std::string mattributeName;
    std::string mValue;
};

class CAmStringAttribute : public CAmNodeAttribute
{
public:
    CAmStringAttribute(std::string attributeName, std::string* pstring) :
                                    CAmNodeAttribute(attributeName),
                                    mpValue(pstring)
    {
    }
protected:
    void _copyAttribute(std::string String)
    {
        *mpValue = String;
    }
private:
    std::string* mpValue;
};

class CAmBoolAttribute : public CAmNodeAttribute
{
public:
    CAmBoolAttribute(std::string attributeName, bool* pbool) :
                                    CAmNodeAttribute(attributeName),
                                    mpBool(pbool)
    {
    }
protected:
    void _copyAttribute(std::string String)
    {
        if (String == "TRUE")
        {
            *mpBool = true;
        }
        else
        {
            *mpBool = false;
        }
    }
private:
    bool* mpBool;
};

class CAmint16Attribute : public CAmNodeAttribute
{
public:
    CAmint16Attribute(std::string attributeName, int16_t* pint16) :
                                    CAmNodeAttribute(attributeName),
                                    mpInt16(pint16)
    {
    }
protected:
    void _copyAttribute(std::string String)
    {
        *mpInt16 = (int16_t)atoi(String.c_str());
    }
private:
    int16_t* mpInt16;
};

class CAmSimpleNode : public IAmXmlNode
{
public:
    CAmSimpleNode(std::string tagname) :
                                    mTagName(tagname)
    {
    }
    ~CAmSimpleNode()
    {
        std::vector<IAmXmlNodeAttribute* >::iterator itListAttributes;
        for (itListAttributes = mListAttributes.begin(); itListAttributes != mListAttributes.end();
                        ++itListAttributes)
        {
            delete *itListAttributes;
        }
        mListAttributes.clear();
    }
    int parse(xmlDocPtr pDocument, xmlNodePtr* pCurrent)
    {
        int returnValue = PARSE_TAG_MISMATCH;
        xmlChar* pKey;
        if (xmlStrEqual((*pCurrent)->name, (const xmlChar*)mTagName.c_str()))
        {
            std::vector<IAmXmlNodeAttribute* >::iterator itListAttributes;
            for (itListAttributes = mListAttributes.begin();
                            itListAttributes != mListAttributes.end(); ++itListAttributes)
            {
                (*itListAttributes)->parse(pDocument, *pCurrent);
            }
            pKey = xmlNodeListGetString(pDocument, (*pCurrent)->xmlChildrenNode, 1);
            if (pKey)
            {
                _copyData(std::string((char*)pKey));
                xmlFree(pKey);
                returnValue = PARSE_SUCCESS;
            }
            else
            {
                LOG_FN_ERROR(__FILENAME__,__func__,"There was an error or empty tag was found");
                returnValue = PARSE_ERROR;
            }
        }
        return returnValue;
    }
    void addAttribute(IAmXmlNodeAttribute* pAttribute)
    {
        mListAttributes.push_back(pAttribute);
    }
protected:
    virtual void _copyData(std::string parsedData)
    {
    }
private:
    std::string mTagName;
    std::string mValue;
    std::vector<IAmXmlNodeAttribute* > mListAttributes;
};

class CAmStringNode : public CAmSimpleNode
{
public:
    CAmStringNode(std::string tagname, std::string* pString) :
                                    CAmSimpleNode(tagname),
                                    mpString(pString)
    {
    }
protected:
    void _copyData(std::string parsedData)
    {
        *mpString = parsedData;
    }

private:
    std::string* mpString;
};

class CAmStringIDNode : public CAmSimpleNode
{
public:
    CAmStringIDNode(std::string tagname, uint16_t* pID) :
                                    CAmSimpleNode(tagname),
                                    mpID(pID)
    {
    }
protected:
    void _copyData(std::string parsedData)
    {
        std::map<std::string, int >::iterator itMapEnumerations;
        itMapEnumerations = mpMapEnumerations->find(parsedData);
        if (itMapEnumerations != mpMapEnumerations->end())
        {
            *mpID = (uint16_t)itMapEnumerations->second;
        }
        else
        {
            *mpID = atoi(parsedData.c_str());
        }
    }
private:
    uint16_t * mpID;
};

class CAmint16Node : public CAmSimpleNode
{
public:
    CAmint16Node(std::string tagname, int16_t* pint16) :
                                    CAmSimpleNode(tagname),
                                    mpint16(pint16)
    {
    }
protected:
    void _copyData(std::string parsedData)
    {
        *mpint16 = atoi(parsedData.c_str());
    }

private:
    int16_t* mpint16;
};

class CAmSoundPropertyValueNode : public CAmint16Node
{
public:
    CAmSoundPropertyValueNode(std::string tagName, int16_t* pvalue, int16_t* pminValue,
                              int16_t* pmaxValue) :
                                    CAmint16Node(tagName, pvalue)
    {
        addAttribute(new CAmint16Attribute(SOUND_PROPERTY_TAG_MIN_VALUE, pminValue));
        addAttribute(new CAmint16Attribute(SOUND_PROPERTY_TAG_MAX_VALUE, pmaxValue));
    }
};
class CAmUint16Node : public CAmSimpleNode
{
public:
    CAmUint16Node(std::string tagname, unsigned short* puint16) :
                                    CAmSimpleNode(tagname),
                                    mpuint16(puint16)
    {
    }
protected:
    void _copyData(std::string parsedData)
    {
        *mpuint16 = atoi(parsedData.c_str());
    }

private:
    unsigned short* mpuint16;
};

class CAmUint32Node : public CAmSimpleNode
{
public:
    CAmUint32Node(std::string tagname, unsigned int* puint32) :
                                    CAmSimpleNode(tagname),
                                    mpuint32(puint32)
    {
    }
protected:
    void _copyData(std::string parsedData)
    {
        *mpuint32 = atoi(parsedData.c_str());
    }

private:
    uint32_t* mpuint32;
};

class CAmInt32Node : public CAmSimpleNode
{
public:
    CAmInt32Node(std::string tagname, int32_t* pint32) :
                                    CAmSimpleNode(tagname),
                                    mpint32(pint32)
    {
    }
protected:
    void _copyData(std::string parsedData)
    {
        *mpint32 = atoi(parsedData.c_str());
    }
private:
    int32_t* mpint32;
};

class CAmEnumerationuint16Node : public CAmSimpleNode
{
public:
    CAmEnumerationuint16Node(std::string tagname, unsigned short* puint16) :
                                    CAmSimpleNode(tagname),
                                    mpuint16(puint16)
    {
    }
protected:
    void _copyData(std::string parsedData)
    {
        std::map<std::string, int >::iterator itMapEnumerations;
        itMapEnumerations = mpMapEnumerations->find(parsedData);
        if (itMapEnumerations != mpMapEnumerations->end())
        {
            *mpuint16 = (unsigned short)itMapEnumerations->second;
        }
    }
private:
    unsigned short* mpuint16;
};

class CAmEnumerationintNode : public CAmSimpleNode
{
public:
    CAmEnumerationintNode(std::string tagname, int32_t* pint32) :
                                    CAmSimpleNode(tagname),
                                    mpint32(pint32)
    {
    }
protected:
    void _copyData(std::string parsedData)
    {
        std::map<std::string, int >::iterator itMapEnumerations;
        itMapEnumerations = mpMapEnumerations->find(parsedData);
        if (itMapEnumerations != mpMapEnumerations->end())
        {
            *mpint32 = itMapEnumerations->second;
        }
    }
private:
    int32_t* mpint32;
};

class CAmBoolNode : public CAmSimpleNode
{
public:
    CAmBoolNode(std::string attributeName, bool* pbool) :
                                    CAmSimpleNode(attributeName),
                                    mpBool(pbool)
    {
    }
protected:
    void _copyData(std::string String)
    {
        if (String == "TRUE" || String == "true" || String == "1")
        {
            *mpBool = true;
        }
        else
        {
            *mpBool = false;
        }
    }
private:
    bool* mpBool;
};

template <typename T1, typename T2>
class CAmListNode : public IAmXmlNode
{

public:
    CAmListNode(std::string tagname, std::vector<T1 >* pList) :
                                    mTagName(tagname),
                                    mpList(pList)
    {
    }
    int parse(xmlDocPtr pDocument, xmlNodePtr* pCurrent)
    {
        int returnValue = PARSE_TAG_MISMATCH;
        T1 t1;
        do
        {
            T2* t2 = new T2(mTagName, &t1);
            returnValue = t2->parse(pDocument, pCurrent);
            delete t2;
            if (returnValue == PARSE_SUCCESS)
            {
                mpList->push_back(t1);
                *pCurrent = getNextElementNode(*pCurrent);
                if (*pCurrent == NULL)
                {
                    break;
                }
            }
        } while (returnValue == PARSE_SUCCESS);
        return returnValue;
    }
private:
    std::string mTagName;
    std::vector<T1 >* mpList;
};

class CAmComplexNode : public IAmXmlNode
{
public:
    CAmComplexNode(std::string tagname) :
                                    mTagName(tagname)
    {
    }
    virtual ~CAmComplexNode()
    {
        std::vector<IAmXmlNode* >::iterator itListChilds;
        for (itListChilds = mListChildNodes.begin(); itListChilds != mListChildNodes.end();
                        ++itListChilds)
        {
            delete *itListChilds;
        }
        mListChildNodes.clear();
        std::vector<IAmXmlNodeAttribute* >::iterator itListAttributes;
        for (itListAttributes = mListAttributes.begin(); itListAttributes != mListAttributes.end();
                        ++itListAttributes)
        {
            delete *itListAttributes;
        }
        mListAttributes.clear();
    }
    virtual void expand(void)=0;
    int parse(xmlDocPtr pDocument, xmlNodePtr* pCurrent)
    {
        int returnValue = PARSE_TAG_MISMATCH;
        unsigned int CurrentChild = 0;
        if (xmlStrEqual((*pCurrent)->name, (const xmlChar*)mTagName.c_str()))
        {

            /*
             * First expand the child nodes
             */
            this->expand();

            std::vector<IAmXmlNodeAttribute* >::iterator itListAttributes;
            for (itListAttributes = mListAttributes.begin();
                            itListAttributes != mListAttributes.end(); ++itListAttributes)
            {
                (*itListAttributes)->parse(pDocument, *pCurrent);
            }
            _getDefaultFromTemplate();
            xmlNodePtr pChild = (*pCurrent)->children;
            pChild = getNextElementNode(pChild);
            /*
             * While parse for each chile node
             */
            while (CurrentChild < mListChildNodes.size())
            {
                if (pChild == NULL)
                {
                    break;
                }
                returnValue = mListChildNodes[CurrentChild]->parse(pDocument, &pChild);
                if (returnValue == PARSE_SUCCESS)
                {
                    pChild = getNextElementNode(pChild);
                }
                CurrentChild++;
            }
            returnValue = _copyComplexData();
            if (returnValue != 0)
            {
                returnValue = PARSE_ERROR;
            }
        }
        return returnValue;
    }
    virtual void addChildNode(IAmXmlNode* xmlNode)
    {
        mListChildNodes.push_back(xmlNode);
    }
    virtual void addAttribute(IAmXmlNodeAttribute* pAttribute)
    {
        mListAttributes.push_back(pAttribute);
    }
protected:
    virtual am_Error_e _copyComplexData(void)
    {
        return E_OK;
    }
    virtual void _getDefaultFromTemplate(void)
    {
    }
private:
    std::string mTagName;
    std::vector<IAmXmlNode* > mListChildNodes;
    std::vector<IAmXmlNodeAttribute* > mListAttributes;
};

class CAmAllNode : public IAmXmlNode
{
public:
    CAmAllNode(std::string tagName) :
                                    mTagName(tagName)
    {
    }
    ~CAmAllNode()
    {
        std::map<std::string, IAmXmlNode* >::iterator itMapChildNodes;
        for(itMapChildNodes = mMapChildNodes.begin();itMapChildNodes!=mMapChildNodes.end();++itMapChildNodes)
        {
            if(itMapChildNodes->second !=NULL)
            {
                delete itMapChildNodes->second;
            }
        }
    }
    int parse(xmlDocPtr pDocument, xmlNodePtr* pCurrent)
    {
        int returnValue = PARSE_TAG_MISMATCH;
        unsigned int CurrentChild = 0;
        if (xmlStrEqual((*pCurrent)->name, (const xmlChar*)mTagName.c_str()))
        {
            this->expand();
            xmlNodePtr pChild = (*pCurrent)->children;
            pChild = getNextElementNode(pChild);
            /*
             * While parse for each child node
             */
            while (CurrentChild < mMapChildNodes.size())
            {
                IAmXmlNode* xmlNode;
                if (pChild == NULL)
                {
                    break;
                }
                xmlNode = mMapChildNodes[std::string((char*)pChild->name)];
                returnValue = xmlNode->parse(pDocument, &pChild);
                if (returnValue == PARSE_SUCCESS)
                {
                    pChild = getNextElementNode(pChild);
                }
                CurrentChild++;
            }
            _copyData();
            returnValue = 0;
        }
        return returnValue;
    }
    void addChildNode(std::string tagName, IAmXmlNode* xmlNode)
    {
        mMapChildNodes[tagName] = xmlNode;
    }
    virtual void _copyData(void)
    {
    }
    virtual void expand(void)=0;
private:
    std::string mTagName;
    std::map<std::string, IAmXmlNode* > mMapChildNodes;
};

template <typename T1, typename T2>
class CAmEncapsulatedListNode : public CAmComplexNode
{

public:
    CAmEncapsulatedListNode(std::string tagName, std::string listTagName, std::vector<T1 >* pList,
                            bool inheritance = true) :
                                    CAmComplexNode(tagName),
                                    mListTagName(listTagName),
                                    mpList(pList),
                                    mInhertance(inheritance)
    {
    }
    void expand(void)
    {
        if (mInhertance == true)
        {
            addChildNode(new CAmListNode<T1, T2 >(mListTagName, &mList));
        }
        else
        {
            addChildNode(new CAmListNode<T1, T2 >(mListTagName, mpList));
        }
    }
protected:
    am_Error_e _copyComplexData(void)
    {
        if ((mInhertance == true) && (mList.size() > 0))
        {
            *mpList = mList;
        }
        return E_OK;
    }
private:
    std::string mListTagName;
    std::vector<T1 >* mpList;
    std::vector<T1 > mList;
    bool mInhertance;
};

class CAmClassPropertyNode : public CAmComplexNode
{
public:
    CAmClassPropertyNode(std::string tagname, am_ClassProperty_s* pSoundProperty) :
                                    CAmComplexNode(tagname),
                                    mpClassProperty(pSoundProperty)
    {
        mpClassProperty->classProperty = 0;
        mpClassProperty->value = 0;
    }
    void expand(void)
    {
        addChildNode(new CAmEnumerationuint16Node(SOUND_PROPERTY_TAG_TYPE,
                                                  &(mpClassProperty->classProperty)));
        addChildNode(new CAmint16Node(SOUND_PROPERTY_TAG_VALUE,
                                     &(mpClassProperty->value)));
    }
private:
    am_ClassProperty_s* mpClassProperty;
};
class CAmSoundPropertyNode : public CAmComplexNode
{
public:
    CAmSoundPropertyNode(std::string tagname, gc_SoundProperty_s* pSoundProperty) :
                                    CAmComplexNode(tagname),
                                    mpSoundProperty(pSoundProperty)
    {
        mpSoundProperty->type = 0;
        mpSoundProperty->value = 0;
        mpSoundProperty->minValue = SHRT_MIN;
        mpSoundProperty->maxValue = SHRT_MAX;
    }
    void expand(void)
    {
        addChildNode(new CAmEnumerationuint16Node(SOUND_PROPERTY_TAG_TYPE,
                                                  &(mpSoundProperty->type)));
        addChildNode(new CAmSoundPropertyValueNode(SOUND_PROPERTY_TAG_VALUE,
                                                   &(mpSoundProperty->value),
                                                   &(mpSoundProperty->minValue),
                                                   &(mpSoundProperty->maxValue)));
    }
private:
    gc_SoundProperty_s* mpSoundProperty;
};

class CAmSystemPropertyNode : public CAmComplexNode
{
public:
    CAmSystemPropertyNode(std::string tagname, am_SystemProperty_s* pSystemProperty) :
                                    CAmComplexNode(tagname),
                                    mpSystemProperty(pSystemProperty)
    {
        mpSystemProperty->type = 0;
        mpSystemProperty->value = 0;
    }
    void expand(void)
    {
        addChildNode(new CAmEnumerationuint16Node(SYSTEM_PROPERTY_TAG_TYPE,
                                                  &(mpSystemProperty->type)));
        addChildNode(new CAmint16Node(SYSTEM_PROPERTY_TAG_VALUE, &(mpSystemProperty->value)));
    }
private:
    am_SystemProperty_s* mpSystemProperty;
};

class CAmMainSoundPropertyNode : public CAmComplexNode
{
public:
    CAmMainSoundPropertyNode(std::string tagname, gc_MainSoundProperty_s* pMainSoundProperty) :
                                    CAmComplexNode(tagname),
                                    mpMainSoundProperty(pMainSoundProperty)
    {
        mpMainSoundProperty->type = 0;
        mpMainSoundProperty->value = 0;
        mpMainSoundProperty->minValue = SHRT_MIN;
        mpMainSoundProperty->maxValue = SHRT_MAX;

    }
    void expand(void)
    {
        addChildNode(new CAmEnumerationuint16Node(SOUND_PROPERTY_TAG_TYPE,
                                                  &(mpMainSoundProperty->type)));
        addChildNode(new CAmSoundPropertyValueNode(SOUND_PROPERTY_TAG_VALUE,
                                                   &(mpMainSoundProperty->value),
                                                   &(mpMainSoundProperty->minValue),
                                                   &(mpMainSoundProperty->maxValue)));
    }
private:
    gc_MainSoundProperty_s* mpMainSoundProperty;
};

class CAmNotificationConfigurationNode : public CAmComplexNode
{
public:
    CAmNotificationConfigurationNode(std::string tagname,
                                     am_NotificationConfiguration_s* pNotificationConfiguration) :
                                    CAmComplexNode(tagname),
                                    mpNotificationConfiguration(pNotificationConfiguration)
    {
        mpNotificationConfiguration->type = NT_UNKNOWN;
        mpNotificationConfiguration->status = NS_OFF;
        mpNotificationConfiguration->parameter = 0;
    }
    void expand(void)
    {
        addChildNode(new CAmUint16Node(NOTIFICATION_TAG_TYPE,
                                       &(mpNotificationConfiguration->type)));
        addChildNode(new CAmEnumerationintNode(NOTIFICATION_TAG_STATUS,
                                               (int*)&(mpNotificationConfiguration->status)));
        addChildNode(new CAmint16Node(NOTIFICATION_TAG_PARAM,
                                      &(mpNotificationConfiguration->parameter)));
    }

private:
    am_NotificationConfiguration_s* mpNotificationConfiguration;

};

class CAmAvailabilityNode : public CAmComplexNode
{
public:
    CAmAvailabilityNode(std::string tagname, am_Availability_s* pAvailability) :
                                    CAmComplexNode(tagname),
                                    mpAvailability(pAvailability)
    {
        mpAvailability->availability = A_UNKNOWN;
        mpAvailability->availabilityReason = AR_UNKNOWN;

    }
    void expand(void)
    {
        addChildNode(new CAmEnumerationintNode(AVAILABILITY_TAG_AVAILABLE,
                                               (int*)&(mpAvailability->availability)));
        addChildNode(new CAmUint16Node(AVAILABILITY_TAG_REASON,
                                       &(mpAvailability->availabilityReason)));
    }

private:
    am_Availability_s* mpAvailability;

};

template <typename T1, typename T2, typename T3>
class CAmEncapsulatedMapNode : public IAmXmlNode
{

public:
    CAmEncapsulatedMapNode(std::string tagName, std::string mapTagName, std::map<T1, T2 >* pMap) :
                                    mTagName(tagName),
                                    mListTagName(mapTagName),
                                    mpMap(pMap)
    {
    }
    int parse(xmlDocPtr pDocument, xmlNodePtr* pCurrent)
    {
        int returnValue = PARSE_TAG_MISMATCH;
        T1 t1;
        T2 t2;
        xmlNodePtr pChild;
        if (xmlStrEqual((*pCurrent)->name, (const xmlChar*)mTagName.c_str()))
        {
            pChild = (*pCurrent)->children;
            pChild = getNextElementNode(pChild);
            if (*pCurrent != NULL)
            {
                do
                {

                    T3* t3 = new T3(mListTagName, &t1, &t2);
                    returnValue = t3->parse(pDocument, &pChild);
                    delete t3;
                    if (returnValue == PARSE_SUCCESS)
                    {
                        (*mpMap)[t1] = t2;
                        pChild = getNextElementNode(pChild);
                        if (pChild == NULL)
                        {
                            break;
                        }
                    }
                } while (returnValue == 0);
            }
        }
        return returnValue;
    }
private:
    std::string mTagName;
    std::string mListTagName;
    std::map<T1, T2 >* mpMap;
};

class CAmUserToNormTableNode : public CAmSimpleNode
{
public:
    CAmUserToNormTableNode(std::string tagName, int16_t* puserVolume, float* pNormalised) :
                                    CAmSimpleNode(tagName),
                                    mpuserVolume(puserVolume),
                                    mpnormalizedVolume(pNormalised)
    {
    }
    void _copyData(std::string pString)
    {
        char* pToken = strtok((char*)pString.c_str(), " ,");
        if (pToken != NULL)
        {
            *mpuserVolume = (int16_t)atoi(pToken);
            pToken = strtok(NULL, " ,");
            if (pToken != NULL)
            {
                *mpnormalizedVolume = atof(pToken);
            }
        }
    }
private:
    int16_t* mpuserVolume;
    float* mpnormalizedVolume;
};

class CAmMSPSPNode : public CAmSimpleNode

{
public:
    CAmMSPSPNode(std::string tagName, am_CustomMainSoundPropertyType_t* pMainSoundProperty,
                 am_CustomSoundPropertyType_t* pSoundProperty,
                 gc_MSPMappingDirection_e* pMappingDirection) :
                                    CAmSimpleNode(tagName),
                                    mpMainSoundProperty(pMainSoundProperty),
                                    mpSoundProperty(pSoundProperty),
                                    mpMappingDirection(pMappingDirection)
    {
        *mpMainSoundProperty = 0;
        *mpSoundProperty = 0;
        *mpMappingDirection = MD_BOTH;
        addAttribute(new CAmStringAttribute(MSPSP_MAP_ATTRIBUTE_TYPE, &mMappingDirectionString));
    }
    void _copyData(std::string pString)

    {
        std::map<std::string, int >::iterator itenumerationMap;
        char* pToken = strtok((char*)pString.c_str(), " ,");
        if (pToken != NULL)

        {
            itenumerationMap = mpMapEnumerations->find(pToken);
            if (itenumerationMap != mpMapEnumerations->end())

            {
                *mpMainSoundProperty = itenumerationMap->second;
            }
            pToken = strtok(NULL, " ,");
            if (pToken != NULL)

            {
                itenumerationMap = mpMapEnumerations->find(pToken);
                if (itenumerationMap != mpMapEnumerations->end())

                {
                    *mpSoundProperty = itenumerationMap->second;
                }

            }
        }
        if (mMappingDirectionString != "")

        {
            itenumerationMap = mpMapEnumerations->find(mMappingDirectionString);
            if (itenumerationMap != mpMapEnumerations->end())

            {
                *mpMappingDirection = (gc_MSPMappingDirection_e)itenumerationMap->second;
            }
        }
    }

private:
    am_CustomMainSoundPropertyType_t* mpMainSoundProperty;
    am_CustomSoundPropertyType_t* mpSoundProperty;
    gc_MSPMappingDirection_e* mpMappingDirection;
    std::string mMappingDirectionString;
}
;

class CAmMSPToSPMapListNode : public IAmXmlNode

{
public:
    CAmMSPToSPMapListNode(
                    std::string tagName,
                    std::map<gc_MSPMappingDirection_e, std::map<uint16_t, uint16_t > >* pMSPMap) :
                                    mTagName(tagName),
                                    mpMSPMap(pMSPMap)

    {
        mMappingDirection = MD_BOTH;
    }
    int parse(xmlDocPtr pDocument, xmlNodePtr* pCurrent)

    {
        int returnValue = PARSE_TAG_MISMATCH;
        std::map<gc_MSPMappingDirection_e, std::map<uint16_t, uint16_t > > MSPMap;
        am_CustomMainSoundPropertyType_t mainSoundProperty;
        am_CustomSoundPropertyType_t soundProperty;
        xmlNodePtr pChild;
        if (xmlStrEqual((*pCurrent)->name, (const xmlChar*)mTagName.c_str()))
        {
            pChild = (*pCurrent)->children;
            pChild = getNextElementNode(pChild);
            if (*pCurrent != NULL)
            {
                do
                {

                    CAmMSPSPNode* mapEntryNode = new CAmMSPSPNode(MSPSP_MAP_TAG_TYPE,
                                                                  &mainSoundProperty,
                                                                  &soundProperty,
                                                                  &mMappingDirection);
                    returnValue = mapEntryNode->parse(pDocument, &pChild);
                    delete mapEntryNode;
                    if (returnValue == PARSE_SUCCESS)
                    {
                        if (mMappingDirection == MD_MSP_TO_SP)
                        {
                            MSPMap[MD_MSP_TO_SP].insert(
                                            std::make_pair(mainSoundProperty, soundProperty));
                        }
                        else if (mMappingDirection == MD_SP_TO_MSP)
                        {
                            MSPMap[MD_SP_TO_MSP].insert(
                                            std::make_pair(soundProperty, mainSoundProperty));
                        }
                        else
                        {
                            MSPMap[MD_MSP_TO_SP].insert(
                                            std::make_pair(mainSoundProperty, soundProperty));
                            MSPMap[MD_SP_TO_MSP].insert(
                                            std::make_pair(soundProperty, mainSoundProperty));
                        }
                        pChild = getNextElementNode(pChild);
                        if (pChild == NULL)
                        {
                            break;
                        }
                    }
                } while (returnValue == 0);
            }
            if (MSPMap.size() > 0)
            {
                *mpMSPMap = MSPMap;
            }
        }
        return returnValue;
    }
private:
    std::string mTagName;
    gc_MSPMappingDirection_e mMappingDirection;
    std::map<gc_MSPMappingDirection_e, std::map<uint16_t, uint16_t > >* mpMSPMap;
};

class CAmNormToVolumeTableNode : public CAmSimpleNode
{
public:
    CAmNormToVolumeTableNode(std::string tagName, float* pnormalizedVolume, float* pdecibelVolume) :
                                    CAmSimpleNode(tagName),
                                    mpnormalizedVolume(pnormalizedVolume),
                                    mpdecibelVolume(pdecibelVolume)
    {
    }
    void _copyData(std::string pString)
    {
        char* pToken = strtok((char*)pString.c_str(), " ,");
        if (pToken != NULL)
        {
            *mpnormalizedVolume = atof(pToken);
            pToken = strtok(NULL, " ,");
            if (pToken != NULL)
            {
                *mpdecibelVolume = atof(pToken);
            }
        }
    }
private:
    float* mpnormalizedVolume;
    float* mpdecibelVolume;
};

class CAmConversionMatrixNode : public CAmSimpleNode
{
public:
    CAmConversionMatrixNode(std::string tagName, std::pair<uint16_t, uint16_t >* plistPair) :
                                    CAmSimpleNode(tagName),
                                    pconversionMatrix(plistPair)
    {
    }
    void _copyData(std::string parsedString)
    {
        size_t count = std::string::npos;
        std::string connectionFormat;
        uint16_t sinkFormat;
        uint16_t sourceFormat;
        std::map<std::string, int >::iterator itMapEnumerations;
        // First make sure that we remove any white spaces from the std::string
        while (std::string::npos != (count = parsedString.find(' ')) || std::string::npos
                        != (count = parsedString.find('\t')))
        {
            parsedString.erase(count, 1);
        }
        do
        {
            if (parsedString == "")
            {
                break;;
            }
            count = parsedString.find(",");
            if (std::string::npos == count)
            {
                break;
            }
            connectionFormat = parsedString.substr(0, count);
            itMapEnumerations = mpMapEnumerations->find(connectionFormat);
            if (itMapEnumerations == mpMapEnumerations->end())
            {
                break;
            }
            sinkFormat = (uint16_t)itMapEnumerations->second;
            connectionFormat = parsedString.substr(count + 1, parsedString.length());
            itMapEnumerations = mpMapEnumerations->find(connectionFormat);
            if (itMapEnumerations == mpMapEnumerations->end())
            {
                break;
            }
            sourceFormat = (uint16_t)itMapEnumerations->second;
            *pconversionMatrix = std::make_pair(sinkFormat, sourceFormat);
        } while (0);
    }
private:
    std::pair<uint16_t, uint16_t >* pconversionMatrix;
}
;

class CAmSourceNode : public CAmComplexNode
{
public:
    CAmSourceNode(std::string tagname, gc_Source_s* pSource) :
                                    CAmComplexNode(tagname),
                                    mpSource(pSource),
                                    mBaseName("")
    {
        mpSource->sourceClassID = 0;
        mpSource->sourceID = 0;
        mpSource->priority = DEFAULT_CONFIG_ELEMENT_PRIORITY;
        mpSource->domainID = 0;
        mpSource->volume = 0;
        mpSource->visible = true;
        mpSource->available.availability = A_AVAILABLE;
        mpSource->available.availabilityReason = AR_UNKNOWN;
        mpSource->isVolumeChangeSupported = true;
        mpSource->isPersistencySupported = false;
        mpSource->isVolumePersistencySupported = false;
        mpSource->registrationType = REG_ROUTER;
        mpSource->listConnectionFormats.clear();
        mpSource->listGCMainSoundProperties.clear();
        mpSource->listGCSoundProperties.clear();
        mpSource->listMainNotificationConfigurations.clear();
        mpSource->listNotificationConfigurations.clear();
        mpSource->minVolume = 0;
        mpSource->maxVolume = 0;
        mpSource->mapSourceVolume.clear();
    }
    void expand()
    {
        addChildNode(new CAmStringIDNode(ELEMENT_COMMON_TAG_ID, &mpSource->sourceID));
        addChildNode(new CAmStringNode(ELEMENT_COMMON_TAG_DOMAIN_NAME, &mpSource->domainName));
        addChildNode(new CAmStringNode(ELEMENT_COMMON_TAG_NAME, &mpSource->name));
        addChildNode(new CAmint16Node(ELEMENT_COMMON_TAG_VOLUME, &mpSource->volume));
        addChildNode(new CAmBoolNode(ELEMENT_COMMON_TAG_VISIBLE, &mpSource->visible));
        addChildNode(new CAmStringNode(ELEMENT_COMMON_TAG_CLASS_NAME, &mpSource->className));
        addChildNode(new CAmEnumerationintNode(SOURCE_TAG_SOURCE_STATE,
                                               (int*)&mpSource->sourceState));
        addChildNode(new CAmListNode<uint16_t, CAmEnumerationuint16Node >(
                        XML_ELEMENT_TAG_CONNECTION_FORMAT,&mpSource->listConnectionFormats));
        addChildNode(new CAmEnumerationintNode(SOURCE_TAG_INTERRUPT_STATE,
                                               (int*)&mpSource->interruptState));
        addChildNode(new CAmListNode<am_NotificationConfiguration_s,
                        CAmNotificationConfigurationNode >(
                        XML_ELEMENT_TAG_MAIN_NOTIFICATION_CONFIG,
                        &mpSource->listMainNotificationConfigurations));
        addChildNode(new CAmListNode<am_NotificationConfiguration_s,
                        CAmNotificationConfigurationNode >(
                        XML_ELEMENT_TAG_NOTIFICATION_CONFIG,&mpSource->listNotificationConfigurations));
        addChildNode(new CAmAvailabilityNode(ELEMENT_COMMON_TAG_AVAILABLE, (&mpSource->available)));
        addChildNode(new CAmListNode<gc_SoundProperty_s, CAmSoundPropertyNode >(
                        XML_ELEMENT_TAG_SOUND_PROPERTY,(&mpSource->listGCSoundProperties)));
        addChildNode(new CAmListNode<gc_MainSoundProperty_s, CAmMainSoundPropertyNode >(
                        XML_ELEMENT_TAG_MAIN_SOUND_PROPERTY,(&mpSource->listGCMainSoundProperties)));
        addChildNode(new CAmEnumerationintNode(ELEMENT_COMMON_TAG_REGISTRATION_TYPE,
                                               (int*)&mpSource->registrationType));
        addChildNode(new CAmBoolNode(ELEMENT_COMMON_TAG_IS_VOLUME_SUPPORTED,
                                     &mpSource->isVolumeChangeSupported));
        addChildNode(new CAmBoolNode(ELEMENT_COMMON_TAG_IS_PERSISTENCY_SUPPORTED,
                                             &mpSource->isPersistencySupported));
        addChildNode(new CAmBoolNode(ELEMENT_COMMON_TAG_IS_VOLUME_PERSISTENCY_SUPPORTED,
                                                     &mpSource->isVolumePersistencySupported));
        addChildNode(new CAmInt32Node(COMMON_TAG_PRIORITY, &mpSource->priority));
        addChildNode(new CAmMSPToSPMapListNode(CONFIG_ELEMENT_SET_MSP_SP_MAP,
                                               &(mpSource->mapMSPTOSP)));
        addChildNode(new CAmint16Node(ELEMENT_COMMON_TAG_MIN_VOLUME, &mpSource->minVolume));
        addChildNode(new CAmint16Node(ELEMENT_COMMON_TAG_MAX_VOLUME, &mpSource->maxVolume));
        addChildNode(new CAmEncapsulatedMapNode<float, float, CAmNormToVolumeTableNode >(
                        SOURCE_TAG_VOLUME_MAP, COMMON_TAG_TABLE_ENTRY,
                        &(mpSource->mapSourceVolume)));
        addAttribute(new CAmStringAttribute(ELEMENT_ATTRIBUTE_BASENAME, &mBaseName));
    }
    void _getDefaultFromTemplate()
    {
        std::vector<gc_Source_s >::iterator itListSources;
        for (itListSources = mpConfiguration->listSources.begin();
                        itListSources != mpConfiguration->listSources.end(); itListSources++)
        {
            if (itListSources->name == mBaseName)
            {
                *mpSource = *itListSources;
                break;
            }
        }
    }
private:
    gc_Source_s* mpSource;
    std::string mBaseName;
};

class CAmSinkNode : public CAmComplexNode
{
public:
    CAmSinkNode(std::string tagName, gc_Sink_s* pSink) :
                                    CAmComplexNode(tagName),
                                    mpSink(pSink),
                                    mBaseName("")
    {
        mpSink->sinkID = 0;
        mpSink->sinkClassID = 0;
        mpSink->muteState = MS_UNMUTED;
        mpSink->mainVolume = 0;
        mpSink->priority = DEFAULT_CONFIG_ELEMENT_PRIORITY;
        mpSink->domainID = 0;
        mpSink->volume = 0;
        mpSink->visible = true;
        mpSink->available.availability = A_AVAILABLE;
        mpSink->available.availabilityReason = AR_UNKNOWN;
        mpSink->isVolumeChangeSupported = true;
        mpSink->isPersistencySupported = false;
        mpSink->isVolumePersistencySupported = false;
        mpSink->registrationType = REG_ROUTER;
        mpSink->listConnectionFormats.clear();
        mpSink->listGCMainSoundProperties.clear();
        mpSink->listGCSoundProperties.clear();
        mpSink->listMainNotificationConfigurations.clear();
        mpSink->listNotificationConfigurations.clear();
        mpSink->mapNormalizedVolumeToDecibelVolume.clear();
        mpSink->mapUserVolumeToNormalizedVolume.clear();
    }
    void expand(void)
    {
        addChildNode(new CAmStringIDNode(ELEMENT_COMMON_TAG_ID, &(mpSink->sinkID)));
        addChildNode(new CAmStringNode(ELEMENT_COMMON_TAG_DOMAIN_NAME, &(mpSink->domainName)));
        addChildNode(new CAmStringNode(ELEMENT_COMMON_TAG_NAME, &(mpSink->name)));
        addChildNode(new CAmint16Node(ELEMENT_COMMON_TAG_VOLUME, &(mpSink->volume)));
        addChildNode(new CAmBoolNode(ELEMENT_COMMON_TAG_VISIBLE, &(mpSink->visible)));
        addChildNode(new CAmint16Node(SINK_TAG_MAIN_VOLUME, &(mpSink->mainVolume)));
        addChildNode(new CAmStringNode(ELEMENT_COMMON_TAG_CLASS_NAME, &(mpSink->className)));
        addChildNode(new CAmEnumerationintNode(SINK_TAG_MUTE_STATE, (int*)&(mpSink->muteState)));
        addChildNode(new CAmListNode<uint16_t, CAmEnumerationuint16Node >(
                        XML_ELEMENT_TAG_CONNECTION_FORMAT,&(mpSink->listConnectionFormats)));
        addChildNode(new CAmListNode<am_NotificationConfiguration_s,
                        CAmNotificationConfigurationNode >(
                        XML_ELEMENT_TAG_MAIN_NOTIFICATION_CONFIG,
                        &(mpSink->listMainNotificationConfigurations)));
        addChildNode(new CAmListNode<am_NotificationConfiguration_s,
                        CAmNotificationConfigurationNode >(
                        XML_ELEMENT_TAG_NOTIFICATION_CONFIG,&(mpSink->listNotificationConfigurations)));
        addChildNode(new CAmAvailabilityNode(ELEMENT_COMMON_TAG_AVAILABLE, &(mpSink->available)));
        addChildNode(new CAmListNode<gc_SoundProperty_s, CAmSoundPropertyNode >(
                        XML_ELEMENT_TAG_SOUND_PROPERTY,&(mpSink->listGCSoundProperties)));
        addChildNode(new CAmListNode<gc_MainSoundProperty_s, CAmMainSoundPropertyNode >(
                        XML_ELEMENT_TAG_MAIN_SOUND_PROPERTY,&(mpSink->listGCMainSoundProperties)));
        addChildNode(new CAmEnumerationintNode(ELEMENT_COMMON_TAG_REGISTRATION_TYPE,
                                               (int*)&(mpSink->registrationType)));
        addChildNode(new CAmBoolNode(ELEMENT_COMMON_TAG_IS_VOLUME_SUPPORTED,
                                     &(mpSink->isVolumeChangeSupported)));
        addChildNode(new CAmBoolNode(ELEMENT_COMMON_TAG_IS_PERSISTENCY_SUPPORTED,
                                             &(mpSink->isPersistencySupported)));
        addChildNode(new CAmBoolNode(ELEMENT_COMMON_TAG_IS_VOLUME_PERSISTENCY_SUPPORTED,
                                                     &(mpSink->isVolumePersistencySupported)));
        addChildNode(new CAmInt32Node(COMMON_TAG_PRIORITY, &(mpSink->priority)));
        addChildNode(new CAmEncapsulatedMapNode<int16_t, float, CAmUserToNormTableNode >(
                        COMMON_TAG_VOLUME_USER_TO_NORM, COMMON_TAG_TABLE_ENTRY,
                        &(mpSink->mapUserVolumeToNormalizedVolume)));
        addChildNode(new CAmEncapsulatedMapNode<float, float, CAmNormToVolumeTableNode >(
                        COMMON_TAG_VOLUME_NORM_TO_USER, COMMON_TAG_TABLE_ENTRY,
                        &(mpSink->mapNormalizedVolumeToDecibelVolume)));
        addChildNode(new CAmMSPToSPMapListNode(CONFIG_ELEMENT_SET_MSP_SP_MAP,
                                               &(mpSink->mapMSPTOSP)));
        addAttribute(new CAmStringAttribute(ELEMENT_ATTRIBUTE_BASENAME, &mBaseName));
    }
    void _getDefaultFromTemplate()
    {
        std::vector<gc_Sink_s >::iterator itListSinks;
        for (itListSinks = mpConfiguration->listSinks.begin();
                        itListSinks != mpConfiguration->listSinks.end(); itListSinks++)
        {
            if (itListSinks->name == mBaseName)
            {
                *mpSink = *itListSinks;
                break;
            }
        }
    }
private:
    gc_Sink_s* mpSink;
    std::string mBaseName;
};

class CAmGatewayNode : public CAmComplexNode
{
public:
    CAmGatewayNode(std::string tagName, gc_Gateway_s* pGateway) :
                                    CAmComplexNode(tagName),
                                    mpGateway(pGateway),
                                    mBaseName("")
    {
        mpGateway->gatewayID = 0;
        mpGateway->sinkID = 0;
        mpGateway->sourceID = 0;
        mpGateway->domainSinkID = 0;
        mpGateway->domainSourceID = 0;
        mpGateway->controlDomainID = 0;
        mpGateway->registrationType = REG_ROUTER;
        mpGateway->sinkName = "";
        mpGateway->sourceName = "";
        mpGateway->convertionMatrix.clear();
        mpGateway->listConvertionmatrix.clear();
    }
    void expand()
    {
        addChildNode(new CAmStringIDNode(ELEMENT_COMMON_TAG_ID, &(mpGateway->gatewayID)));
        addChildNode(new CAmStringNode(ELEMENT_COMMON_TAG_NAME, &(mpGateway->name)));
        addChildNode(new CAmListNode<std::pair<uint16_t, uint16_t >, CAmConversionMatrixNode >(
                        GATEWAY_TAG_CONVERSION_MATRIX, &(mpGateway->listConvertionmatrix)));
        addChildNode(new CAmStringNode(GATEWAY_TAG_SINK_NAME, &(mpGateway->sinkName)));
        addChildNode(new CAmStringNode(GATEWAY_TAG_SOURCE_NAME, &(mpGateway->sourceName)));
        addChildNode(new CAmStringNode(GATEWAY_TAG_CONTROL_DOMAIN_NAME,
                                       &(mpGateway->controlDomainName)));
        addChildNode(new CAmEnumerationintNode(ELEMENT_COMMON_TAG_REGISTRATION_TYPE,
                                               (int*)&(mpGateway->registrationType)));
        addAttribute(new CAmStringAttribute(ELEMENT_ATTRIBUTE_BASENAME, &(mBaseName)));
    }
    void _getDefaultFromTemplate()
    {
        std::vector<gc_Gateway_s >::iterator itListGateway;
        for (itListGateway = mpConfiguration->listGateways.begin();
                        itListGateway != mpConfiguration->listGateways.end(); itListGateway++)
        {
            if (itListGateway->name == mBaseName)
            {
                *mpGateway = *itListGateway;
                break;
            }
        }
    }
private:
    gc_Gateway_s* mpGateway;
    std::string mBaseName;
};

class CAmDomainNode : public CAmComplexNode
{
public:
    CAmDomainNode(std::string tagName, gc_Domain_s* pDomain) :
                                    CAmComplexNode(tagName),
                                    mpDomain(pDomain),
                                    mBaseName("")
    {
        mpDomain->domainID = 0;
        mpDomain->busname = "";
        mpDomain->nodename = "";
        mpDomain->complete = false;
        mpDomain->domainID = 0;
        mpDomain->early = false;
        mpDomain->name = "";
        mpDomain->nodename = "";
        mpDomain->registrationType = REG_ROUTER;
        mpDomain->state = DS_CONTROLLED;
    }
    void expand()
    {
        addChildNode(new CAmStringIDNode(ELEMENT_COMMON_TAG_ID, &(mpDomain->domainID)));
        addChildNode(new CAmStringNode(ELEMENT_COMMON_TAG_NAME, &(mpDomain->name)));
        addChildNode(new CAmStringNode(DOMAIN_TAG_BUS_NAME, &(mpDomain->busname)));
        addChildNode(new CAmStringNode(DOMAIN_TAG_NODE_NAME, &(mpDomain->nodename)));
        addChildNode(new CAmBoolNode(DOMAIN_TAG_EARLY, &(mpDomain->early)));
        addChildNode(new CAmBoolNode(DOMAIN_TAG_COMPLETE, &(mpDomain->complete)));
        addChildNode(new CAmEnumerationintNode(DOMAIN_TAG_STATE, (int*)&(mpDomain->state)));
        addChildNode(new CAmEnumerationintNode(ELEMENT_COMMON_TAG_REGISTRATION_TYPE,
                                               (int*)&(mpDomain->registrationType)));
        addAttribute(new CAmStringAttribute(ELEMENT_ATTRIBUTE_BASENAME, &(mBaseName)));
    }
    void _getDefaultFromTemplate()
    {
        std::vector<gc_Domain_s >::iterator itListDomains;
        for (itListDomains = mpConfiguration->listDomains.begin();
                        itListDomains != mpConfiguration->listDomains.end(); itListDomains++)
        {
            if (itListDomains->name == mBaseName)
            {
                *mpDomain = *itListDomains;
                break;
            }
        }
    }
private:
    gc_Domain_s* mpDomain;
    std::string mBaseName;
};

gc_TopologyElement_s::gc_TopologyElement_s( gc_ClassTopologyCodeID_e elementCode,
                                            std::string elementName):
                codeID(elementCode)
{
    switch (codeID)
    {
        case MC_EQUAL_CODE:
            name = TOPOLOGY_SYMBOL_EQUAL;
            break;
        case MC_EXCLUSIVE_CODE:
            name = TOPOLOGY_SYMBOL_CARET;
            break;
        case MC_LBRACKET_CODE:
            name = TOPOLOGY_SYMBOL_LEFT_BRACKET;
            break;
        case MC_RBRACKET_CODE:
            name = TOPOLOGY_SYMBOL_RIGHT_BRACKET;
            break;
        default:
            name = elementName;
            break;
    }
}

gc_TopologyElement_s::gc_TopologyElement_s( gc_ClassTopologyCodeID_e elementCode,
                                            std::string elementName, size_t pos,
                                            size_t length):
        gc_TopologyElement_s(elementCode,elementName.substr(pos, length))
{
}

gc_TopologyElement_s::gc_TopologyElement_s( std::string elementName)
{
    name = elementName;
    if(elementName == "(")
    {
        codeID = MC_LBRACKET_CODE;
    }
    else if(elementName == ")")
    {
        codeID = MC_RBRACKET_CODE;
    }
    else
    {
        codeID = MC_EXCLUSIVE_CODE;
    }
}

class CAmClassNode : public CAmComplexNode
{
public:
    CAmClassNode(std::string tagName, gc_Class_s* pClass) :
                                    CAmComplexNode(tagName),
                                    mpClass(pClass),
                                    mBaseName("")
    {
        mpClass->classID = 0;
        mpClass->name = "";
        mpClass->type = C_PLAYBACK;
        mpClass->priority = 50;
        mpClass->listTopologies.clear();
        mpClass->registrationType = REG_CONTROLLER;
        mpClass->listClassProperties.clear();
        mpClass->isVolumePersistencySupported = false;
        mpClass->defaultVolume = AM_MUTE;
    }
    void expand()
    {
        addChildNode(new CAmStringIDNode(ELEMENT_COMMON_TAG_ID, &(mpClass->classID)));
        addChildNode(new CAmStringNode(CLASS_TAG_CLASS_NAME, &(mpClass->name)));
        addChildNode(new CAmEnumerationintNode(CLASS_TAG_TYPE, (int*)&(mpClass->type)));
        addChildNode(new CAmInt32Node(COMMON_TAG_PRIORITY, &(mpClass->priority)));
        addChildNode(new CAmListNode<std::string, CAmStringNode >(CLASS_TAG_TOPOLOGY_TAG,
                     &(mTopologyStrings)));
        addChildNode(new CAmEnumerationintNode(ELEMENT_COMMON_TAG_REGISTRATION_TYPE,
                                               (int*)&(mpClass->registrationType)));
        addChildNode(new CAmListNode<am_ClassProperty_s, CAmClassPropertyNode >(
                CLASS_TAG_PROPERTIES_TAG,&(mpClass->listClassProperties)));
        addChildNode(new CAmint16Node(CLASS_TAG_DEFAULT_VOLUME, &mpClass->defaultVolume));
        addChildNode(new CAmBoolNode(ELEMENT_COMMON_TAG_IS_VOLUME_PERSISTENCY_SUPPORTED,
                                     &mpClass->isVolumePersistencySupported));
        addAttribute(new CAmStringAttribute(ELEMENT_ATTRIBUTE_BASENAME, &(mBaseName)));
    }
protected:
    void _getDefaultFromTemplate()
    {
        std::vector<gc_Class_s >::iterator itListClasses;
        for (itListClasses = mpConfiguration->listClasses.begin();
                        itListClasses != mpConfiguration->listClasses.end(); itListClasses++)
        {
            if (itListClasses->name == mBaseName)
            {
                *mpClass = *itListClasses;
                break;
            }
        }
    }
    virtual am_Error_e _copyComplexData(void)
    {
        am_Error_e error = E_OK;
        std::vector<std::string >::iterator itListStringTopologies;
        for (itListStringTopologies = mTopologyStrings.begin();
             itListStringTopologies != mTopologyStrings.end();
             itListStringTopologies++)
        {
            std::vector < gc_TopologyElement_s > topology;
            error = _parseTopologyString(*itListStringTopologies, topology);
            if (error == E_OK)
            {
                mpClass->listTopologies.push_back(topology);
            } else
            {
                LOG_FN_ERROR(__FILENAME__,__func__,"Topology parsing failed for",
                        *itListStringTopologies);
            }
        }
        return error;
    }
private:
    am_Error_e _parseElementList(gc_ClassTopologyCodeID_e elementType,
                                 const std::string& elementList,
                                 size_t currentIndex,
                                 const size_t endPosition,
                                 std::vector<gc_TopologyElement_s >& listTopologies)
    {
        size_t caretPosition = elementList.find_first_of(TOPOLOGY_SYMBOL_CARET,
                currentIndex);
        if ((elementList.find_first_of(TOPOLOGY_SYMBOL_LEFT_BRACKET,
                currentIndex) < endPosition)
                || (elementList.find_first_of(TOPOLOGY_SYMBOL_RIGHT_BRACKET,
                        currentIndex) < endPosition))
        {
            return E_NOT_POSSIBLE;
        }
        while (currentIndex < endPosition)
        {
            if (caretPosition < endPosition)
            {
                listTopologies.push_back(
                        gc_TopologyElement_s(elementType, elementList,
                                currentIndex, caretPosition - currentIndex));
                listTopologies.push_back(
                        gc_TopologyElement_s(MC_EXCLUSIVE_CODE));
                currentIndex = caretPosition + 1;
            } else
            {
                listTopologies.push_back(
                        gc_TopologyElement_s(elementType, elementList,
                                currentIndex, endPosition - currentIndex));
                currentIndex = caretPosition;
            }
            caretPosition = elementList.find_first_of(TOPOLOGY_SYMBOL_CARET,
                    currentIndex);
        }
        return E_OK;
    }

    am_Error_e _parseTopologyString(const std::string& topologyString,
            std::vector<gc_TopologyElement_s >& listTopologies)
    {
        size_t currentPosition = 0;
        size_t totalLength = topologyString.length();
        size_t equalsPosition = topologyString.find_first_of(
                TOPOLOGY_SYMBOL_EQUAL, currentPosition);
        am_Error_e result = E_OK;
        if ((std::count(topologyString.begin(), topologyString.end(), '=') != 1)
                || (topologyString.find_first_of(TOPOLOGY_SYMBOL_WHITESPACE)
                        != std::string::npos))
        {
            LOG_FN_ERROR("invalid character found");
            return E_NOT_POSSIBLE;
        }
        if (std::count(topologyString.begin(), topologyString.end(), '(')
                != std::count(topologyString.begin(), topologyString.end(),
                        ')'))
        {
            LOG_FN_ERROR("mismatch in brackets");
            return E_NOT_POSSIBLE;
        }

        if (E_OK != _parseElementList(MC_SINK_ELEMENT, topologyString,
                              currentPosition, equalsPosition, listTopologies))
        {
            return E_NOT_POSSIBLE;
        }
        listTopologies.push_back(gc_TopologyElement_s(MC_EQUAL_CODE));
        currentPosition = equalsPosition + 1;

        while (currentPosition < totalLength)
        {
            size_t caretPosition = topologyString.find_first_of(
                    TOPOLOGY_SYMBOL_CARET, currentPosition);
            size_t leftBracketPosition = topologyString.find_first_of(
                    TOPOLOGY_SYMBOL_LEFT_BRACKET, currentPosition);
            size_t rightBracketPosition = topologyString.find_first_of(
                    TOPOLOGY_SYMBOL_RIGHT_BRACKET, currentPosition);

            if ((caretPosition == std::string::npos) &&
                (leftBracketPosition == std::string::npos) &&
                (rightBracketPosition == std::string::npos))
            {
                //No caret no brackets so only one source case 0
                listTopologies.push_back(
                        gc_TopologyElement_s(MC_SOURCE_ELEMENT, topologyString,
                                currentPosition));
                break;
            } else
            {
                size_t lowestPosition = std::min(caretPosition,
                        std::min(leftBracketPosition, rightBracketPosition));
                gc_ClassTopologyCodeID_e elementType =
                        (topologyString[lowestPosition] == '(') ?
                                MC_GATEWAY_ELEMENT : MC_SOURCE_ELEMENT;
                if (lowestPosition != currentPosition)
                {
                    listTopologies.push_back(
                            gc_TopologyElement_s(elementType, topologyString,
                                    currentPosition,
                                    lowestPosition - currentPosition));
                }
                listTopologies.push_back(
                        gc_TopologyElement_s(
                                topologyString.substr(lowestPosition, 1)));
                currentPosition = lowestPosition + 1;
            }
        }
        return result;
    }

    std::vector<std::string > mTopologyStrings;
    gc_Class_s* mpClass;
    std::string mBaseName;
};

class CAmActionParamNode : public CAmAllNode
{
public:
    CAmActionParamNode(std::string tagName, std::map<std::string, std::string >* pMapParam) :
                                    CAmAllNode(tagName),
                                    mpMapParameters(pMapParam)
    {
        mpMapParameters->clear();
        mRampType = -1;
        mMuteState = -1;
        mOrder = -1;
        mPropertyType = -1;
        mDebugType = -1;
        mNotificationType = -1;
        mNotificationStatus = -1;

    }
    void expand(void)
    {
        addChildNode(ACTION_PARAM_CLASS_NAME,
                     new CAmStringNode(ACTION_PARAM_CLASS_NAME, &mClassName));
        addChildNode(ACTION_PARAM_SOURCE_NAME,
                     new CAmStringNode(ACTION_PARAM_SOURCE_NAME, &mSourceName));
        addChildNode(ACTION_PARAM_SINK_NAME, new CAmStringNode(ACTION_PARAM_SINK_NAME, &mSinkName));
        addChildNode(ACTION_PARAM_GATEWAY_NAME,
                     new CAmStringNode(ACTION_PARAM_GATEWAY_NAME, &mGatewayName));
        addChildNode(ACTION_PARAM_RAMP_TYPE,
                     new CAmEnumerationintNode(ACTION_PARAM_RAMP_TYPE, &mRampType));
        addChildNode(ACTION_PARAM_RAMP_TIME, new CAmStringNode(ACTION_PARAM_RAMP_TIME, &mRampTime));
        addChildNode(ACTION_PARAM_MUTE_STATE,
                     new CAmEnumerationintNode(ACTION_PARAM_MUTE_STATE, &mMuteState));
        addChildNode(ACTION_PARAM_MAIN_VOLUME_STEP,
                     new CAmStringNode(ACTION_PARAM_MAIN_VOLUME_STEP, &mMainVolumeStep));
        addChildNode(ACTION_PARAM_MAIN_VOLUME,
                     new CAmStringNode(ACTION_PARAM_MAIN_VOLUME, &mMainVolume));
        addChildNode(ACTION_PARAM_VOLUME_STEP,
                     new CAmStringNode(ACTION_PARAM_VOLUME_STEP, &mVolumeStep));
        addChildNode(ACTION_PARAM_VOLUME, new CAmStringNode(ACTION_PARAM_VOLUME, &mVolume));
        addChildNode(ACTION_PARAM_ORDER, new CAmEnumerationintNode(ACTION_PARAM_ORDER, &mOrder));
        addChildNode(ACTION_PARAM_PROPERTY_TYPE,
                     new CAmEnumerationintNode(ACTION_PARAM_PROPERTY_TYPE, &mPropertyType));
        addChildNode(ACTION_PARAM_PROPERTY_VALUE,
                     new CAmStringNode(ACTION_PARAM_PROPERTY_VALUE, &mPropertyValue));
        addChildNode(ACTION_PARAM_TIMEOUT, new CAmStringNode(ACTION_PARAM_TIMEOUT, &mTimeOut));
        addChildNode(ACTION_PARAM_PATTERN, new CAmStringNode(ACTION_PARAM_PATTERN, &mPattern));
        addChildNode(ACTION_PARAM_CONNECTION_STATE,
                             new CAmStringNode(ACTION_PARAM_CONNECTION_STATE, &mConnectionState));
        addChildNode(ACTION_PARAM_DEBUG_TYPE,
                     new CAmEnumerationintNode(ACTION_PARAM_DEBUG_TYPE, &mDebugType));
        addChildNode(ACTION_PARAM_DEBUG_VALUE,
                     new CAmStringNode(ACTION_PARAM_DEBUG_VALUE, &mDebugValue));
        addChildNode(ACTION_PARAM_CONNECTION_FORMAT,
                     new CAmStringNode(ACTION_PARAM_CONNECTION_FORMAT, &mConnectionFormat));
        addChildNode(ACTION_PARAM_EXCEPT_SOURCE_NAME,
                     new CAmStringNode(ACTION_PARAM_EXCEPT_SOURCE_NAME, &mExceptSource));
        addChildNode(ACTION_PARAM_EXCEPT_SINK_NAME,
                     new CAmStringNode(ACTION_PARAM_EXCEPT_SINK_NAME, &mExceptSink));
        addChildNode(ACTION_PARAM_EXCEPT_CLASS_NAME,
                     new CAmStringNode(ACTION_PARAM_EXCEPT_CLASS_NAME, &mExceptClass));
        addChildNode(ACTION_PARAM_NOTIFICATION_CONFIGURATION_TYPE, new CAmEnumerationintNode(ACTION_PARAM_NOTIFICATION_CONFIGURATION_TYPE, &mNotificationType));
        addChildNode(ACTION_PARAM_NOTIFICATION_CONFIGURATION_STATUS, new CAmEnumerationintNode(ACTION_PARAM_NOTIFICATION_CONFIGURATION_STATUS, &mNotificationStatus));
        addChildNode(ACTION_PARAM_NOTIFICATION_CONFIGURATION_PARAM,
                             new CAmStringNode(ACTION_PARAM_NOTIFICATION_CONFIGURATION_PARAM, &mNotificationParam));

    }
    void _copyData(void)
    {
        copyStringInMap(ACTION_PARAM_CLASS_NAME, mClassName);
        copyStringInMap(ACTION_PARAM_CONNECTION_STATE, mConnectionState);
        copyStringInMap(ACTION_PARAM_SOURCE_NAME, mSourceName);
        copyStringInMap(ACTION_PARAM_SINK_NAME, mSinkName);
        copyStringInMap(ACTION_PARAM_GATEWAY_NAME, mGatewayName);
        if (mRampType != -1)
        {
            copyStringInMap(ACTION_PARAM_RAMP_TYPE, to_string(mRampType));
        }
        copyStringInMap(ACTION_PARAM_RAMP_TIME, mRampTime);
        if (mMuteState != -1)
        {
            copyStringInMap(ACTION_PARAM_MUTE_STATE, to_string(mMuteState));
        }
        copyStringInMap(ACTION_PARAM_MAIN_VOLUME_STEP, mMainVolumeStep);
        copyStringInMap(ACTION_PARAM_MAIN_VOLUME, mMainVolume);
        copyStringInMap(ACTION_PARAM_VOLUME_STEP, mVolumeStep);
        copyStringInMap(ACTION_PARAM_VOLUME, mVolume);
        if (mOrder != -1)
        {
            copyStringInMap(ACTION_PARAM_ORDER, to_string(mOrder));
        }
        if (mPropertyType != -1)
        {
            copyStringInMap(ACTION_PARAM_PROPERTY_TYPE, to_string(mPropertyType));
        }
        copyStringInMap(ACTION_PARAM_PROPERTY_VALUE, mPropertyValue);
        copyStringInMap(ACTION_PARAM_TIMEOUT, mTimeOut);
        copyStringInMap(ACTION_PARAM_PATTERN, mPattern);
        if (mDebugType != -1)
        {
            copyStringInMap(ACTION_PARAM_DEBUG_TYPE, to_string(mDebugType));
        }
        copyStringInMap(ACTION_PARAM_DEBUG_VALUE, mDebugValue);
        copyStringInMap(ACTION_PARAM_CONNECTION_FORMAT, mConnectionFormat);
        copyStringInMap(ACTION_PARAM_EXCEPT_SOURCE_NAME, mExceptSource);
        copyStringInMap(ACTION_PARAM_EXCEPT_SINK_NAME, mExceptSink);
        copyStringInMap(ACTION_PARAM_EXCEPT_CLASS_NAME, mExceptClass);
        if (mNotificationType != -1)
        {
            copyStringInMap(ACTION_PARAM_NOTIFICATION_CONFIGURATION_TYPE, to_string(mNotificationType));
        }
        if (mNotificationStatus != -1)
        {
            copyStringInMap(ACTION_PARAM_NOTIFICATION_CONFIGURATION_STATUS, to_string(mNotificationStatus));
        }
        copyStringInMap(ACTION_PARAM_NOTIFICATION_CONFIGURATION_PARAM, mNotificationParam);
    }
private:
    void copyStringInMap(std::string keyName, std::string value)
    {
        if (value != "")
            (*mpMapParameters)[keyName] = value;
    }

    std::string mClassName;
    std::string mSourceName;
    std::string mSinkName;
    std::string mGatewayName;
    std::string mRampTime;
    std::string mMainVolumeStep;
    std::string mMainVolume;
    std::string mVolumeStep;
    std::string mVolume;
    std::string mPropertyValue;
    std::string mTimeOut;
    std::string mPattern;
    std::string mConnectionFormat;
    std::string mDebugValue;
    std::string mExceptSource;
    std::string mExceptSink;
    std::string mExceptClass;
    std::string mConnectionState;
    std::string mNotificationParam;

    int mDebugType;
    int mRampType;
    int mMuteState;
    int mOrder;
    int mPropertyType;
    int mNotificationType;
    int mNotificationStatus;
    std::map<std::string, std::string > *mpMapParameters;
}
;
class CAmActionNode : public CAmComplexNode
{
public:
    CAmActionNode(std::string tagName, gc_Action_s* pAction) :
                                    CAmComplexNode(tagName),
                                    mpAction(pAction)
    {
        mpAction->actionName = "";
        mpAction->mapParameters.clear();
    }
    void expand()
    {
        addChildNode(new CAmStringNode(ACTION_SET_TAG_TYPE, &(mpAction->actionName)));
        addChildNode(new CAmActionParamNode(ACTION_SET_TAG_PARAM, &(mpAction->mapParameters)));
    }
private:
    gc_Action_s* mpAction;
}
;

class CAmProcessNode : public CAmComplexNode
{
public:
    CAmProcessNode(std::string tagName, gc_Process_s* pProcess) :
                                    CAmComplexNode(tagName),
                                    mpProcess(pProcess)
    {
        mpProcess->comment = "";
        mpProcess->priority = DEFAULT_CONFIG_PARSED_POLICY_PRIORITY;
        mpProcess->stopEvaluation = false;
        mpProcess->listActions.clear();
        mpProcess->listConditions.clear();
        mValidFunctionNames.push_back(FUNCTION_NAME);
        mValidFunctionNames.push_back(FUNCTION_PRIORITY);
        mValidFunctionNames.push_back(FUNCTION_CONNECTION_STATE);
        mValidFunctionNames.push_back(FUNCTION_VOLUME);
        mValidFunctionNames.push_back(FUNCTION_MAIN_VOLUME);
        mValidFunctionNames.push_back(FUNCTION_SOUND_PROPERTY);
        mValidFunctionNames.push_back(FUNCTION_MAIN_SOUND_PROPERTY_TYPE);
        mValidFunctionNames.push_back(FUNCTION_SYSTEM_PROPERTY_TYPE);
        mValidFunctionNames.push_back(FUNCTION_MAIN_SOUND_PROPERTY_VALUE);
        mValidFunctionNames.push_back(FUNCTION_SYSTEM_PROPERTY_VALUE);
        mValidFunctionNames.push_back(FUNCTION_MUTE_STATE);
        mValidFunctionNames.push_back(FUNCTION_IS_REGISTRATION_COMPLETE);
        mValidFunctionNames.push_back(FUNCTION_AVAILABILITY);
        mValidFunctionNames.push_back(FUNCTION_AVAILABILITY_REASON);
        mValidFunctionNames.push_back(FUNCTION_INTERRUPT_STATE);
        mValidFunctionNames.push_back(FUNCTION_IS_REGISTERED);
        mValidFunctionNames.push_back(FUNCTION_STATE);
        mValidFunctionNames.push_back(FUNCTION_CONNECTION_FORMAT);
        mValidFunctionNames.push_back(FUNCTION_PEEK);
        mValidFunctionNames.push_back(FUNCTION_ERROR);

        mValidCategory.push_back(CATEGORY_SINK);
        mValidCategory.push_back(CATEGORY_SOURCE);
        mValidCategory.push_back(CATEGORY_CLASS);
        mValidCategory.push_back(CATEGORY_CONNECTION);
        mValidCategory.push_back(CATEGORY_DOMAIN);
        mValidCategory.push_back(CATEGORY_DOMAIN_OF_SOURCE);
        mValidCategory.push_back(CATEGORY_DOMAIN_OF_SINK);
        mValidCategory.push_back(CATEGORY_CLASS_OF_SOURCE);
        mValidCategory.push_back(CATEGORY_CLASS_OF_SINK);
        mValidCategory.push_back(CATEGORY_SOURCE_OF_CLASS);
        mValidCategory.push_back(CATEGORY_SINK_OF_CLASS);
        mValidCategory.push_back(CATEGORY_CONNECTION_OF_CLASS);
        mValidCategory.push_back(CATEGORY_CONNECTION_OF_SOURCE);
        mValidCategory.push_back(CATEGORY_CONNECTION_OF_SINK);
        mValidCategory.push_back(CATEGORY_SYSTEM);
        mValidCategory.push_back(CATEGORY_USER);
    }
    void expand(void)
    {
        addChildNode(new CAmStringNode(POLICY_TAG_COMMENT, &(mpProcess->comment)));
        addChildNode(new CAmInt32Node(POLICY_TAG_PRIORITY, &(mpProcess->priority)));
        addChildNode(new CAmListNode<std::string, CAmStringNode >(POLICY_TAG_CONDITION,
                                                                  &(mListConditions)));
        addChildNode(new CAmListNode<gc_Action_s, CAmActionNode >(POLICY_TAG_ACTION,
                                                                  &(mpProcess->listActions)));
        addChildNode(new CAmBoolNode(POLICY_TAG_STOP_EVALUATION, &(mpProcess->stopEvaluation)));
    }
protected:
    virtual am_Error_e _copyComplexData(void)
    {
        std::vector<std::string >::iterator itListStringConditions;
        am_Error_e error = E_OK;
        for (itListStringConditions = mListConditions.begin();
                        itListStringConditions != mListConditions.end(); itListStringConditions++)
        {
            gc_ConditionStruct_s conditionInstance;
            error = _parseCondition((unsigned char*)itListStringConditions->c_str(),
                                    conditionInstance);
            if (error != E_OK)
            {
                LOG_FN_ERROR(__FILENAME__,__func__,"condition parse failure", *itListStringConditions);
                break;
            }
            mpProcess->listConditions.push_back(conditionInstance);
        }
        return error;
    }
private:
    am_Error_e _convertMacroToValueInFunction(gc_FunctionElement_s& functionInstance)
    {
        am_Error_e result;
        /*
         *  get the system property value. It is special case because it support property type
         *  in param1 of condition
         */
        if ((FUNCTION_SYSTEM_PROPERTY_VALUE == functionInstance.functionName) && (false
                        == functionInstance.isValueMacro))
        {
            if (1 != mpMapEnumerations->count(functionInstance.mandatoryParameter))
            {
                LOG_FN_ERROR(__FILENAME__,__func__," system property Not in range:", functionInstance.mandatoryParameter);
                return E_UNKNOWN;
            }
            functionInstance.mandatoryParameter = to_string((*mpMapEnumerations)[functionInstance.mandatoryParameter]);
        }
        if ((false == functionInstance.optionalParameter.empty()) && (FUNCTION_MACRO_SUPPORTED_ALL
                        != functionInstance.optionalParameter)
            && (FUNCTION_MACRO_SUPPORTED_OTHERS != functionInstance.optionalParameter)
            && (FUNCTION_MACRO_SUPPORTED_REQUESTING != functionInstance.optionalParameter))
        {
            // As it is map it can store only one entry for given pKey value
            if (mValidFunctionNames.end() == std::find(mValidFunctionNames.begin(),
                                                       mValidFunctionNames.end(),
                                                       functionInstance.functionName))
            {
                LOG_FN_ERROR(__FILENAME__,__func__," Enum Not in range: function/optional", functionInstance.functionName,
                             functionInstance.optionalParameter);
                return E_UNKNOWN;
            }
            if (1 != mpMapEnumerations->count(functionInstance.optionalParameter))
            {
                LOG_FN_ERROR(__FILENAME__,__func__," Enum Not in range of schema: function/optional",
                             functionInstance.functionName, functionInstance.optionalParameter);
                return E_UNKNOWN;
            }
            functionInstance.optionalParameter = to_string((*mpMapEnumerations)[functionInstance.optionalParameter]);
        }
        return E_OK;
    }
    void _removeLeadingSpace(const unsigned char *inputString, unsigned int &startPosition) const
    {
        while (((inputString[startPosition] == ' ') || (inputString[startPosition] == '\t')) && (startPosition
                        < strlen((char *)inputString)))
        {
            startPosition++;
        }
    }
    bool _isFunctionAvailable(const unsigned char *inputString, unsigned int startPosition) const
    {
        while ((inputString[startPosition] != '(') && (startPosition < strlen((char *)inputString)))
        {
            startPosition++;
        }
        if (startPosition == strlen((char *)inputString))
        {
            return false;
        }
        return true;
    }
// validates whether the input std::string is integer or not.
    bool _isInteger(const unsigned char *inputString, unsigned int startPosition) const
    {
        while ((isdigit(inputString[startPosition]))&&(startPosition < strlen((char *)inputString)))
        {
            startPosition++;
        }
        if (startPosition == strlen((char *)inputString))
        {
            return true;
        }
        return false;
    }
    std::string _findElement(unsigned int &startPosition, const char token,
                             const unsigned char * inputString, const char invalidToken) const
    {
        std::string outputString;

        for (; startPosition < strlen((char *)inputString); startPosition++)
        {
            // break in case token or invalid token found.
            if ((inputString[startPosition] == token) || (inputString[startPosition] == invalidToken))
            {
                break;
            }
            // this check is needed to allow the space in between words
            if (outputString.size() == 0)
            {
                // remove the starting space
                if (' ' != inputString[startPosition])
                {
                    outputString += inputString[startPosition];
                }
            }
            else
            {
                outputString += inputString[startPosition];
            }
        }
        return outputString;
    }

// This API is used to parse and store the token of the LHS/RHS part of the condition
    am_Error_e _parseAttribute(const unsigned char *inputString, unsigned int &startPosition,
                               gc_ConditionStruct_s &conditionInstance, const bool isLHS)
    {
        std::string functionName;
        std::string category;
        std::string mandatoryParameter;
        std::string optionalParameter;
        std::string optionalParameter2;
        bool isValueMacro = false;

        //find function name
        functionName = _findElement(startPosition, '(', inputString, ',');
        //validate function name
        if (mValidFunctionNames.end() == std::find(mValidFunctionNames.begin(),
                                                   mValidFunctionNames.end(), functionName))
        {
            LOG_FN_ERROR(__FILENAME__,__func__,"Function name is not supported");
            return E_UNKNOWN;
        }
        startPosition++;

        //find category
        category = _findElement(startPosition, ',', inputString, ')');
        if(inputString[startPosition] == ')') // end of condition part
        {
            if(true == category.empty())
            {
                category = CATEGORY_USER;
            }
        }
        startPosition++;

        // find the mandatory value. Store the element in macro (REQUESTING/ALL) or std::string with ""
        if (inputString[startPosition - 1] == ')')
        {
            mandatoryParameter = FUNCTION_MACRO_SUPPORTED_REQUESTING;
        }
        else
        {
            mandatoryParameter = _findElement(startPosition, ')', inputString, ',');
        }
        //check the value if it is macro. Only REQUESTING (ALL -- in some case) Macro is allowed
        if ((mandatoryParameter.data())[0] != '"')
        {
            if ((FUNCTION_IS_REGISTERED == functionName) || (FUNCTION_IS_REGISTRATION_COMPLETE
                            == functionName)
                || ((FUNCTION_STATE == functionName) && (CATEGORY_DOMAIN == category)))
            {
                if ((CATEGORY_DOMAIN != category) || ((FUNCTION_MACRO_SUPPORTED_ALL
                                != mandatoryParameter)
                                                      && (FUNCTION_MACRO_SUPPORTED_REQUESTING != mandatoryParameter)))
                {
                    LOG_FN_ERROR(__FILENAME__,__func__,"Mandatory Parameter not valid in condition of policy."
                                 "Might be double quote is missing. Expected is ALL or"
                                 " string inside double quote",
                                 functionName, category, mandatoryParameter);
                    return E_UNKNOWN;
                }
            }
            else if (FUNCTION_MACRO_SUPPORTED_REQUESTING != mandatoryParameter)
            {
                LOG_FN_ERROR(__FILENAME__,__func__,"Mandatory Parameter not valid in condition of policy. "
                             "Might be double quote is missing. Expected is REQUESTING or "
                             "string inside double quote",
                             functionName, category, mandatoryParameter);
                return E_UNKNOWN;
            }
            isValueMacro = true;
        }
        else
        {
            unsigned int temp = 1;
            mandatoryParameter = _findElement(temp, '"', (unsigned char *)mandatoryParameter.data(),
                                              '"');
        }
        //find the optional value
        if (inputString[startPosition] == ',')
        {
            startPosition++;
            optionalParameter = _findElement(startPosition, ')', inputString, ',');
        }
		else
        {
            // this is needed for main notification configuration param/status function because 3 parameter can be left empty.
            //In that case REQUESTING type need to be considered as configuration type
            if((FUNCTION_MAIN_NOTIFICATION_CONFIGURATION_PARAM == functionName)
              || (FUNCTION_MAIN_NOTIFICATION_CONFIGURATION_STATUS == functionName))
            {
                optionalParameter = FUNCTION_MACRO_SUPPORTED_REQUESTING;
            }
        }
        //find the 2nd optional value
        if (inputString[startPosition] == ',')
        {
            startPosition++;
            optionalParameter2 = _findElement(startPosition, ')', inputString, ',');
        }
        //validate category name
        if (mValidCategory.end() == std::find(mValidCategory.begin(), mValidCategory.end(),
                                              category))
        {
            return E_UNKNOWN;
        }
        startPosition++;

        if (true == isLHS)
        {
            conditionInstance.leftObject.functionName = functionName;
            conditionInstance.leftObject.category = category;
            conditionInstance.leftObject.mandatoryParameter = mandatoryParameter;
            conditionInstance.leftObject.optionalParameter = optionalParameter;
            conditionInstance.leftObject.optionalParameter2 = optionalParameter2;
            conditionInstance.leftObject.isValueMacro = isValueMacro;
        }
        else
        {
            conditionInstance.rightObject.functionObject.functionName = functionName;
            conditionInstance.rightObject.functionObject.category = category;
            conditionInstance.rightObject.functionObject.mandatoryParameter = mandatoryParameter;
            conditionInstance.rightObject.functionObject.optionalParameter = optionalParameter;
            conditionInstance.rightObject.functionObject.optionalParameter2 = optionalParameter2;
            conditionInstance.rightObject.functionObject.isValueMacro = isValueMacro;
        }
        return E_OK;
    }

// Tokenize the RHS part of condition
    am_Error_e _findRHSFromCondition(const unsigned char *inputString, unsigned int &startPosition,
                                     gc_ConditionStruct_s &conditionInstance)
    {
        am_Error_e result = E_OK;
        if (inputString[startPosition] == '"')
        {
            startPosition++;
            conditionInstance.rightObject.directValue = _findElement(startPosition, '"',
                                                                     inputString, '"');
            conditionInstance.rightObject.isValue = true;
            conditionInstance.rightObject.isMacro = false;
        }
        else if (true == _isFunctionAvailable(inputString, startPosition))
        {
            conditionInstance.rightObject.isValue = false;
            conditionInstance.rightObject.isMacro = false;
            result = _parseAttribute(inputString, startPosition, conditionInstance, false);
        }
        else if (true == _isInteger(inputString, startPosition))
        {
            conditionInstance.rightObject.directValue = _findElement(startPosition, '"',
                                                                     inputString, ' ');
            conditionInstance.rightObject.isValue = true;
            conditionInstance.rightObject.isMacro = false;
        }
        else
        {
            conditionInstance.rightObject.isValue = false;
            conditionInstance.rightObject.isMacro = true;
            conditionInstance.rightObject.directValue = _findElement(startPosition, '"',
                                                                     inputString, ' ');
        }
        return result;
    }

    am_Error_e _parseCondition(const unsigned char* stringConditions,
                               gc_ConditionStruct_s& conditionInstance)
    {
        am_Error_e result;
        unsigned int position = 0;

        int value;

        _removeLeadingSpace(stringConditions, position);

        // parse the attribute of LHS
        result = _parseAttribute(stringConditions, position, conditionInstance, true);
        if (E_OK != result)
        {
            return E_UNKNOWN;
        }
        _removeLeadingSpace(stringConditions, position);

        //find the operator
        conditionInstance.operation = _findElement(position, ' ', stringConditions, '"');
        position++;
        //remove the space after operator
        _removeLeadingSpace(stringConditions, position);

        //parse the attribute of RHS
        result = _findRHSFromCondition(stringConditions, position, conditionInstance);
        if (E_OK != result)
        {
            return E_UNKNOWN;
        }

        if (1 == mpMapEnumerations->count(conditionInstance.operation))
        {
            value = (*mpMapEnumerations)[conditionInstance.operation];
        }
        else
        {
            LOG_FN_ERROR(__FILENAME__,__func__," Operator Not in range:", conditionInstance.operation);
            return E_UNKNOWN;
        }
        conditionInstance.operation = to_string(value);
        if (E_OK != _convertMacroToValueInFunction(conditionInstance.leftObject))
        {
            return E_UNKNOWN;
        }

        // convert the macro in its corresponding integer value and store it in std::string format for evaluation
        if (true == conditionInstance.rightObject.isMacro)
        {
            if (1 == mpMapEnumerations->count(conditionInstance.rightObject.directValue))
            {
                value = (*mpMapEnumerations)[conditionInstance.rightObject.directValue];
                conditionInstance.rightObject.directValue = to_string(value);
                conditionInstance.rightObject.isMacro = false;
                conditionInstance.rightObject.isValue = true;
            }
            else // function name used in LHS cannot be used with macro on RHS side
            {
                return E_UNKNOWN;
            }
        }
        else // RHS is function so convert the optional parameter to value from enum specified in string format.
        {
            if (E_OK != _convertMacroToValueInFunction(
                            conditionInstance.rightObject.functionObject))
            {
                return E_UNKNOWN;
            }
        }
        return E_OK;
    }
    std::vector<std::string > mListConditions;
    gc_Process_s* mpProcess;
    std::vector<std::string > mValidFunctionNames;
    std::vector<std::string > mValidCategory;
};

class CAmPolicyNode : public CAmComplexNode
{
public:
    CAmPolicyNode(std::string tagName, gc_Policy_s* pPolicy) :
                                    CAmComplexNode(tagName),
                                    mpPolicy(pPolicy)
    {
        mpPolicy->listEvents.clear();
        mpPolicy->listProcesses.clear();
    }
    void expand()
    {
        addChildNode(new CAmListNode<int, CAmEnumerationintNode >(
                                POLICY_TAG_TRIGGER, &(mpPolicy->listEvents)));
        addChildNode(new CAmListNode<gc_Process_s, CAmProcessNode >(POLICY_TAG_LIST_PROCESS,
                                                                    &(mpPolicy->listProcesses)));
    }
private:
    gc_Policy_s* mpPolicy;
};

class CAmConfigurationNode : public CAmComplexNode
{
public:
    CAmConfigurationNode(std::string tagname, gc_Configuration_s* pConfig) :
                                    CAmComplexNode(tagname),
                                    mpConfig(pConfig)
    {
        mpConfig->listSources.clear();
        mpConfig->listSinks.clear();
        mpConfig->listGateways.clear();
        mpConfig->listDomains.clear();
        mpConfig->listClasses.clear();
        mpConfig->listPolicies.clear();
    }
    virtual void expand(void)
    {
        addChildNode(new CAmEncapsulatedListNode<gc_Source_s, CAmSourceNode >(
                        CONFIG_ELEMENT_SET_SOURCE, XML_ELEMENT_TAG_SOURCE, &(mpConfig->listSources),
                        false));
        addChildNode(new CAmEncapsulatedListNode<gc_Sink_s, CAmSinkNode >(CONFIG_ELEMENT_SET_SINK,
                                                                          XML_ELEMENT_TAG_SINK,
                                                                          &(mpConfig->listSinks),
                                                                          false));
        addChildNode(new CAmEncapsulatedListNode<gc_Gateway_s, CAmGatewayNode >(
                        CONFIG_ELEMENT_SET_GATEWAY, XML_ELEMENT_TAG_GATEWAY,
                        &(mpConfig->listGateways), false));
        addChildNode(new CAmEncapsulatedListNode<gc_Domain_s, CAmDomainNode >(
                        CONFIG_ELEMENT_SET_DOMAIN, XML_ELEMENT_TAG_DOMAIN, &(mpConfig->listDomains),
                        false));
        addChildNode(new CAmEncapsulatedListNode<gc_Class_s, CAmClassNode >(
                        CONFIG_ELEMENT_SET_CLASS, XML_ELEMENT_TAG_CLASS, &(mpConfig->listClasses),
                        false));
        addChildNode(new CAmEncapsulatedListNode<gc_Policy_s, CAmPolicyNode >(
                        CONFIG_ELEMENT_SET_POLICY, XML_ELEMENT_TAG_POLICY,
                        &(mpConfig->listPolicies), false));
        addChildNode(new CAmEncapsulatedListNode<am_SystemProperty_s, CAmSystemPropertyNode >(
                        CONFIG_ELEMENT_SET_SYSTEM_PROPERTY, XML_ELEMENT_TAG_SYSTEM_PROPERTY,
                        &(mpConfig->listSystemProperties), false));
    }

private:
    gc_Configuration_s* mpConfig;
};

CAmXmlConfigParser::CAmXmlConfigParser()
{
}
CAmXmlConfigParser::~CAmXmlConfigParser(void)
{
    mMapEnumerations.clear();
}
am_Error_e CAmXmlConfigParser::parse(gc_Configuration_s* pConfiguration)
{
//  check the environment variable to decide the path to be used for user configuration
    char *path = (char *)getenv(CONFIGURATION_FILE_ENV_VARNAME);
    if (NULL == path)
    {
        path = (char*)DEFAULT_USER_CONFIGURATION_PATH;
    }
    LOG_FN_DEBUG(__FILENAME__,__func__,"Parse Start");
// Validate the XML file against the schema.
    if (E_OK != _validateConfiguration(path, DEFAULT_SCHEMA_PATH))
    {
        LOG_FN_ERROR(__FILENAME__,__func__," Not able to validate configuration as per Schema");
        return E_UNKNOWN;
    }
// parse the XML schema
    mMapEnumerations.clear();
    if (E_OK != _parseXSDFile(DEFAULT_SCHEMA_PATH))
    {
        LOG_FN_ERROR(__FILENAME__,__func__," Not able to parse Schema");
        return E_UNKNOWN;
    }
    if (E_OK != _parseXSDFile(DEFAULT_CUSTOM_SCHEMA_PATH))
    {
        LOG_FN_ERROR(__FILENAME__,__func__," Not able to parse Schema");
        return E_UNKNOWN;
    }

    if (E_OK != _parseConfiguration(std::string(path), pConfiguration))
    {
        LOG_FN_ERROR(__FILENAME__,__func__," Not able to parse configuration");
        return E_UNKNOWN;
    }
    LOG_FN_DEBUG(__FILENAME__,__func__,"Parse End");
#ifdef AMCO_DEBUGGING
    printAllEnums();
    printListSources(pConfiguration);
    printListSinks(pConfiguration);
    printListGateways(pConfiguration);
    printListDomains(pConfiguration);
    printListClasses(pConfiguration);
    printListPolicies(pConfiguration);
    printListSystemProperties(pConfiguration);
#endif
    return E_OK;
}
void _forwardschemaerrortodlt(void *pCtx, const char *pMsg, ...)
{
    (void)pCtx;
    char buf[1024];
    std::string ErroMsg;
    va_list args; //va_list();s
//va_start() will initialize args
    va_start(args, pMsg);
    int len = vsnprintf(buf, 1024, pMsg, args);
    va_end(args);
    ErroMsg = buf;
    LOG_FN_ERROR(__FILENAME__,__func__,"Schema Parsing Error, Length=", len, "Message=", ErroMsg);
}

am_Error_e CAmXmlConfigParser::_validateConfiguration(const std::string& XMLFileName,
                                                      const std::string& XSDFileName)
{
    am_Error_e returnValue = E_UNKNOWN;
    xmlDocPtr pDocument;
    xmlSchemaPtr pSchema = NULL;
    xmlSchemaParserCtxtPtr pContext;
    xmlLineNumbersDefault(1);
    pContext = xmlSchemaNewParserCtxt(XSDFileName.c_str());
    if (pContext == NULL)
    {
        LOG_FN_ERROR(__FILENAME__,__func__," schema file not found");
        return returnValue;
    }
#ifndef AMCO_DEBUGGING
    xmlSchemaSetParserErrors(pContext, (xmlSchemaValidityErrorFunc)_forwardschemaerrortodlt,
                             (xmlSchemaValidityWarningFunc)_forwardschemaerrortodlt, NULL);
#else
    xmlSchemaSetParserErrors( pContext,
                    (xmlSchemaValidityErrorFunc) fprintf,
                    (xmlSchemaValidityWarningFunc) fprintf,
                    stderr);
#endif
    pSchema = xmlSchemaParse(pContext);
    if (pSchema != NULL)
    {
        xmlSchemaFreeParserCtxt(pContext);
        pDocument = xmlReadFile(XMLFileName.c_str(), NULL, 0);
        if (pDocument != NULL)
        {
            xmlSchemaValidCtxtPtr pValidContext;
            pValidContext = xmlSchemaNewValidCtxt(pSchema);
#ifndef AMCO_DEBUGGING
            xmlSchemaSetValidErrors(pValidContext,
                                    (xmlSchemaValidityErrorFunc)_forwardschemaerrortodlt,
                                    (xmlSchemaValidityWarningFunc)_forwardschemaerrortodlt, NULL);
#else
            xmlSchemaSetValidErrors(pValidContext,
                            (xmlSchemaValidityErrorFunc) fprintf,
                            (xmlSchemaValidityWarningFunc) fprintf,
                            stderr);
#endif
            if (0 == xmlSchemaValidateDoc(pValidContext, pDocument))
            {
                returnValue = E_OK;
            }
            xmlSchemaFreeValidCtxt(pValidContext);
            xmlFreeDoc(pDocument);
        }
        // free the resource
        xmlSchemaFree(pSchema);
        xmlSchemaCleanupTypes();
        xmlCleanupParser();
        xmlMemoryDump();
    }
    return returnValue;
}

am_Error_e CAmXmlConfigParser::_parseConfiguration(const std::string& XMLFilename,
                                                   gc_Configuration_s* pConfiguration)
{
    xmlDocPtr pDocument;
    xmlNodePtr pCurrent;
    pDocument = xmlParseFile(XMLFilename.c_str());
    if (pDocument == NULL)
    {
        LOG_FN_ERROR(__FILENAME__,__func__," Document not parsed successfully. ");
        return E_NOT_POSSIBLE;
    }
    pCurrent = xmlDocGetRootElement(pDocument);
    if (pCurrent == NULL)
    {
        LOG_FN_ERROR(__FILENAME__,__func__," empty document");
        xmlFreeDoc(pDocument);
        return E_NOT_POSSIBLE;
    }
    CAmConfigurationNode configurationNode(XML_ELEMENT_TAG_CONFIG, pConfiguration);
    configurationNode.mpMapEnumerations = &mMapEnumerations;
    configurationNode.mpConfiguration = pConfiguration;
    configurationNode.parse(pDocument, &pCurrent);
    xmlFreeDoc(pDocument);
    return E_OK;
}

am_Error_e CAmXmlConfigParser::_parseXSDFile(const std::string& XSDFilename)
{
    /**
     * This function should parse the xsd file and make an internal data structure for
     * storing the enumerations as below
     * enumLookup[enumeration element][enum value]
     */
    xmlDocPtr pDocument;
    xmlNodePtr pCurrent;
    pDocument = xmlParseFile(XSDFilename.c_str());
    if (pDocument == NULL)
    {
        LOG_FN_ERROR(__FILENAME__,__func__," Document not parsed successfully. ");
        return E_UNKNOWN;
    }
    pCurrent = xmlDocGetRootElement(pDocument);
    if (pCurrent == NULL)
    {
        LOG_FN_ERROR(__FILENAME__,__func__," empty document");
        xmlFreeDoc(pDocument);
        return E_UNKNOWN;
    }
    if (xmlStrcmp(pCurrent->name, (const xmlChar *)XML_ELEMENT_TAG_SCHEMA))
    {
        LOG_FN_ERROR(__FILENAME__,__func__," Document of the wrong type, root node != schema");
        xmlFreeDoc(pDocument);
        return E_UNKNOWN;
    }
    _parseSimpleType(pDocument, pCurrent->children);
    xmlFreeDoc(pDocument);
    return E_OK;
}

void CAmXmlConfigParser::_parseSimpleType(const xmlDocPtr pDocument, xmlNodePtr pCurrent)
{
    xmlNodePtr pChildNode;
    while (pCurrent != NULL)
    {
        if (xmlStrcmp(pCurrent->name, (const xmlChar *)XML_ELEMENT_TAG_SIMPLE_TYPE) == 0)
        {
            pChildNode = pCurrent->children;
            _parseEnumeration(pDocument, pChildNode);
        }
        pCurrent = pCurrent->next;
    }
}

am_Error_e CAmXmlConfigParser::_parseEnumInitialiser(const xmlDocPtr pDocument,
                                                     xmlNodePtr pCurrent, int& value)
{
    am_Error_e result = E_UNKNOWN;
    xmlChar * pKey;
    while (pCurrent != NULL)
    {
        if (xmlStrcmp(pCurrent->name, (const xmlChar *)XML_ELEMENT_TAG_ANNOTATION) == 0)
        {
            pCurrent = pCurrent->children;
            if (pCurrent != NULL)
            {
                pCurrent = pCurrent->next;
            }
            if (pCurrent != NULL)
            {
                pKey = xmlNodeListGetString(pDocument, pCurrent->xmlChildrenNode, 1);
                value = atoi((char*)pKey);
                if (value != 0)
                {
                    result = E_OK;
                }
                xmlFree(pKey);
            }
        }
        if (pCurrent != NULL)
        {
            pCurrent = pCurrent->next;
        }
    }
    return result;
}

void CAmXmlConfigParser::_parseEnumeration(const xmlDocPtr pDocument, xmlNodePtr pCurrent)
{
    xmlChar * pKey;
    xmlNodePtr pChild;
    int counter = 0;
    int parseValue;
    while (pCurrent != NULL)
    {
        if (xmlStrcmp(pCurrent->name, (const xmlChar *)XML_ELEMENT_TAG_RESTRICTION) == 0)
        {
            pChild = pCurrent->children;
            while (pChild != NULL)
            {
                if (xmlStrcmp(pChild->name, (const xmlChar *)ENUM_TAG_ENUMERATION) == 0)
                {
                    pKey = xmlGetProp(pChild, (const xmlChar*)ENUM_TAG_VALUE);
                    if (E_OK == _parseEnumInitialiser(pDocument, pChild->children, parseValue))
                    {
                        counter = parseValue;
                    }
                    mMapEnumerations[(char*)pKey] = counter++;
                    xmlFree(pKey);
                }
                pChild = pChild->next;
            }
        }
        pCurrent = pCurrent->next;
    }
}

#ifdef AMCO_DEBUGGING
void CAmXmlConfigParser::printAllEnums(void)
{
    std::map<std::string,int>::iterator itMapEnumerations;
    for( itMapEnumerations= mMapEnumerations.begin();itMapEnumerations != mMapEnumerations.end(); ++itMapEnumerations)
    {
        LOG_FN_INFO(__FILENAME__,__func__," Enumeration Name : value",itMapEnumerations->first,itMapEnumerations->second);
    }
}

void CAmXmlConfigParser::printListGateways(gc_Configuration_s* pConfiguration)
{
    std::vector<gc_Gateway_s>::iterator itListGateways;
    LOG_FN_INFO(__FILENAME__,__func__," Gateways Information start *****");
    for(itListGateways = pConfiguration->listGateways.begin();
                    itListGateways!=pConfiguration->listGateways.end();
                    ++itListGateways)
    {
        LOG_FN_INFO(__FILENAME__,__func__," Gateway Information begin");
        LOG_FN_INFO(__FILENAME__,__func__," Gateway Name ",itListGateways->name);
        LOG_FN_INFO(__FILENAME__,__func__," Source Name ",itListGateways->sourceName);
        LOG_FN_INFO(__FILENAME__,__func__," SinkName ",itListGateways->sinkName);
        LOG_FN_INFO(__FILENAME__,__func__," DomainName ",itListGateways->controlDomainName);
        LOG_FN_INFO(__FILENAME__,__func__," Number of Conversion matrix ",itListGateways->listConvertionmatrix.size());
        for(const auto& it: itListGateways->listConvertionmatrix )
        {
        	LOG_FN_INFO(__FILENAME__,__func__," DomainName ",it.first,it.second);
        }
        LOG_FN_INFO(__FILENAME__,__func__," Gateway Information end");
    }
    LOG_FN_INFO(__FILENAME__,__func__," Gateways Information end *****");
}
void CAmXmlConfigParser::printListDomains(gc_Configuration_s* pConfiguration)
{
    std::vector<gc_Domain_s>::iterator itListDomains;
    LOG_FN_INFO(__FILENAME__,__func__," Domains Information start *****");
    for(itListDomains = pConfiguration->listDomains.begin();
                    itListDomains!=pConfiguration->listDomains.end();
                    ++itListDomains)
    {
        LOG_FN_INFO(__FILENAME__,__func__," Domain Information begin");
        LOG_FN_INFO(__FILENAME__,__func__," DomainID ",itListDomains->domainID);
        LOG_FN_INFO(__FILENAME__,__func__," Name ",itListDomains->name);
        LOG_FN_INFO(__FILENAME__,__func__," BusName ",itListDomains->busname);
        LOG_FN_INFO(__FILENAME__,__func__," NodeName ",itListDomains->nodename);
        LOG_FN_INFO(__FILENAME__,__func__," RegistrationType ",itListDomains->registrationType);
        LOG_FN_INFO(__FILENAME__,__func__," Domain Information end");
    }
    LOG_FN_INFO(__FILENAME__,__func__," Domains Information end *****");
}
void CAmXmlConfigParser::printListClasses(gc_Configuration_s* pConfiguration)
{
    std::vector<gc_Class_s>::iterator itListClasses;
    std::map<int16_t,float>::iterator itMapUserVolumeToNormalizedVolume;
    std::map<float,float >::iterator itMapNormalizedVolumeToDecibelVolume;
    std::vector<am_CustomConnectionFormat_t>::iterator itlistConnectionFormats;
    std::vector<gc_MainSoundProperty_s>::iterator itlistMainSoundProperties;
    std::vector<gc_SoundProperty_s>::iterator itlistSoundProperties;
    std::vector<std::string>::iterator itTopologyStr;
    LOG_FN_INFO(__FILENAME__,__func__," Classes Information start *****");
    for(itListClasses = pConfiguration->listClasses.begin();
                    itListClasses!=pConfiguration->listClasses.end();
                    ++itListClasses)
    {
        LOG_FN_INFO(__FILENAME__,__func__," Class Information begin");
        LOG_FN_INFO(__FILENAME__,__func__," Name ",itListClasses->name);
        LOG_FN_INFO(__FILENAME__,__func__," Type ",itListClasses->type);
        LOG_FN_INFO(__FILENAME__,__func__," Priority ",itListClasses->priority);
        LOG_FN_INFO(__FILENAME__,__func__,"size of topology",itListClasses->listTopologies.size());
        for(const auto& it: itListClasses->listTopologies )
        {
        	for(const auto& innerit: it )
        	{
        		LOG_FN_INFO(innerit.name);
        	}
        }

        LOG_FN_INFO(__FILENAME__,__func__," Class Information end");
    }

    LOG_FN_INFO(__FILENAME__,__func__," Classes Information end *****");
}

void CAmXmlConfigParser::printListSinks(gc_Configuration_s* pConfiguration)
{
    std::vector<gc_Sink_s>::iterator itListSInks;
    std::map<int16_t,float>::iterator itMapUserVolumeToNormalizedVolume;
    std::map<float,float >::iterator itMapNormalizedVolumeToDecibelVolume;
    std::vector<am_CustomConnectionFormat_t>::iterator itlistConnectionFormats;
    std::vector<gc_MainSoundProperty_s>::iterator itlistMainSoundProperties;
    std::vector<gc_SoundProperty_s>::iterator itlistSoundProperties;
    std::map<uint16_t,uint16_t>::iterator itMapMSPSP;
    LOG_FN_INFO(__FILENAME__,__func__," Sinks Information start *****");
    for(itListSInks = pConfiguration->listSinks.begin();
                    itListSInks!=pConfiguration->listSinks.end();
                    ++itListSInks)
    {
        LOG_FN_INFO(__FILENAME__,__func__," Sink Information start *****");
        LOG_FN_INFO(__FILENAME__,__func__," Name ",itListSInks->name);
        LOG_FN_INFO(__FILENAME__,__func__," ClassID ",itListSInks->sinkClassID);
        LOG_FN_INFO(__FILENAME__,__func__," SinkID ",itListSInks->sinkID);
        LOG_FN_INFO(__FILENAME__,__func__," Domain ID ",itListSInks->domainID);
        LOG_FN_INFO(__FILENAME__,__func__," Volume ",itListSInks->volume);
        LOG_FN_INFO(__FILENAME__,__func__," Visible ",itListSInks->visible);
        LOG_FN_INFO(__FILENAME__,__func__," availability: reason ",itListSInks->available.availability,":",itListSInks->available.availabilityReason);
        LOG_FN_INFO(__FILENAME__,__func__," Mutestate ",itListSInks->muteState);
        LOG_FN_INFO(__FILENAME__,__func__," mainVolume ",itListSInks->mainVolume);

        LOG_FN_INFO(__FILENAME__,__func__," Domain Name ",itListSInks->domainName);
        LOG_FN_INFO(__FILENAME__,__func__," Class Name ",itListSInks->className);
        LOG_FN_INFO(__FILENAME__,__func__," RegistrationType",itListSInks->registrationType);
        LOG_FN_INFO(__FILENAME__,__func__," isVolumeSupported",itListSInks->isVolumeChangeSupported);
        LOG_FN_INFO(__FILENAME__,__func__," priority",itListSInks->priority);
        /**
         * Print the volume user-> Norm table
         */

        LOG_FN_INFO(__FILENAME__,__func__," Connection Formats Begin--------------------");
        itlistConnectionFormats = itListSInks->listConnectionFormats.begin();
        for(;itlistConnectionFormats != itListSInks->listConnectionFormats.end();++itlistConnectionFormats)
        {
            LOG_FN_INFO(__FILENAME__,__func__," Connection Format ",*itlistConnectionFormats);
        }
        LOG_FN_INFO(__FILENAME__,__func__," Connection Formats Ends--------------------");
        itMapUserVolumeToNormalizedVolume = itListSInks->mapUserVolumeToNormalizedVolume.begin();
        LOG_FN_INFO(__FILENAME__,__func__," User -> Norm Begin---------------------");
        for(;itMapUserVolumeToNormalizedVolume != itListSInks->mapUserVolumeToNormalizedVolume.end();++itMapUserVolumeToNormalizedVolume)
        {
            LOG_FN_INFO(__FILENAME__,__func__," mainVolume:Norm ",itMapUserVolumeToNormalizedVolume->first,":",itMapUserVolumeToNormalizedVolume->second);
        }
        LOG_FN_INFO(__FILENAME__,__func__," User -> Norm Ends------------------------");
        LOG_FN_INFO(__FILENAME__,__func__," Norm 2 Decibel Begin------------------------");
        itMapNormalizedVolumeToDecibelVolume = itListSInks->mapNormalizedVolumeToDecibelVolume.begin();
        for(;itMapNormalizedVolumeToDecibelVolume != itListSInks->mapNormalizedVolumeToDecibelVolume.end();++itMapNormalizedVolumeToDecibelVolume)
        {
            LOG_FN_INFO(__FILENAME__,__func__," Norm:Decibel ",itMapNormalizedVolumeToDecibelVolume->first,":",itMapNormalizedVolumeToDecibelVolume->second);
        }
        LOG_FN_INFO(__FILENAME__,__func__," Norm 2 Decibel end------------------------");
        itlistMainSoundProperties = itListSInks->listGCMainSoundProperties.begin();
        LOG_FN_INFO(__FILENAME__,__func__," MainSoundProperties begin-------------------");
        for(;itlistMainSoundProperties != itListSInks->listGCMainSoundProperties.end();++itlistMainSoundProperties)
        {
            LOG_FN_INFO(__FILENAME__,__func__," Main Sound Property Type:Value:Min:Max",
                            (*itlistMainSoundProperties).type,
                            (*itlistMainSoundProperties).value,
                            (*itlistMainSoundProperties).minValue,
                            (*itlistMainSoundProperties).maxValue);
        }
        LOG_FN_INFO(__FILENAME__,__func__," MainSoundProperties end-------------------");
        itlistSoundProperties = itListSInks->listGCSoundProperties.begin();
        LOG_FN_INFO(__FILENAME__,__func__," Sound Properties begin-------------------");
        for(;itlistSoundProperties != itListSInks->listGCSoundProperties.end();++itlistSoundProperties)
        {
            LOG_FN_INFO(__FILENAME__,__func__," Type:Value:Min:Max ",
                            (*itlistSoundProperties).type,
                            (*itlistSoundProperties).value,
                            (*itlistSoundProperties).minValue,
                            (*itlistSoundProperties).maxValue
            );
        }
        LOG_FN_INFO(__FILENAME__,__func__," Sound Properties end-------------------");
        std::map<am_CustomMainSoundPropertyType_t,am_CustomSoundPropertyType_t>::iterator itMapMSPToSP;
        LOG_FN_INFO(__FILENAME__,__func__," Sound Properties End------------------------");
        itMapMSPSP = itListSInks->mapMSPTOSP[MD_MSP_TO_SP].begin();
        LOG_FN_INFO(__FILENAME__,__func__," MSP:SP Begin------------------------");
        for(;itMapMSPSP != itListSInks->mapMSPTOSP[MD_MSP_TO_SP].end();++itMapMSPSP)
        {
            LOG_FN_INFO(__FILENAME__,__func__," MSP:SP ",itMapMSPSP->first,itMapMSPSP->second);
        }
        LOG_FN_INFO(__FILENAME__,__func__," MSP:SP End------------------------");
        itMapMSPSP = itListSInks->mapMSPTOSP[MD_SP_TO_MSP].begin();
        LOG_FN_INFO(__FILENAME__,__func__," SP:MSP Begin------------------------");
        for(;itMapMSPSP != itListSInks->mapMSPTOSP[MD_SP_TO_MSP].end();++itMapMSPSP)
        {
            LOG_FN_INFO(__FILENAME__,__func__," SP:MSP ",itMapMSPSP->first,itMapMSPSP->second);
        }
        LOG_FN_INFO(__FILENAME__,__func__," MSP:SP End------------------------");
        LOG_FN_INFO(__FILENAME__,__func__," Sink Information end *****");
    }
    LOG_FN_INFO(__FILENAME__,__func__," Sink Information end *****");
}
void CAmXmlConfigParser::printListSources(gc_Configuration_s* pConfiguration)
{
    std::vector<gc_Source_s>::iterator itSources;
    std::map<int16_t,float>::iterator itMapUserVolumeToNormalizedVolume;
    std::map<float,float >::iterator itMapNormalizedVolumeToDecibleVolume;
    std::vector<am_CustomConnectionFormat_t>::iterator itlistConnectionFormats;
    std::vector<gc_MainSoundProperty_s>::iterator itlistMainSoundProperties;
    std::vector<gc_SoundProperty_s>::iterator itlistSoundProperties;
    std::map<uint16_t,uint16_t>::iterator itMapMSPSP;
    LOG_FN_INFO(__FILENAME__,__func__," Source Information start *****");
    for(itSources = pConfiguration->listSources.begin();
                    itSources!=pConfiguration->listSources.end();
                    ++itSources)
    {
        LOG_FN_INFO(__FILENAME__,__func__," Source Information begin");
        LOG_FN_INFO(__FILENAME__,__func__," Name ",itSources->name);
        LOG_FN_INFO(__FILENAME__,__func__," ClassID ",itSources->sourceClassID);
        LOG_FN_INFO(__FILENAME__,__func__," SourceID ",itSources->sourceID);
        LOG_FN_INFO(__FILENAME__,__func__," DomainID ",itSources->domainID);
        LOG_FN_INFO(__FILENAME__,__func__," Volume ",itSources->volume);
        LOG_FN_INFO(__FILENAME__,__func__," Visible ",itSources->visible);
        LOG_FN_INFO(__FILENAME__,__func__," availability: reason ",itSources->available.availability,":",itSources->available.availabilityReason);

        LOG_FN_INFO(__FILENAME__,__func__," Domain Name ",itSources->domainName);
        LOG_FN_INFO(__FILENAME__,__func__," Class Name ",itSources->className);
        LOG_FN_INFO(__FILENAME__,__func__," RegistrationType",itSources->registrationType);
        LOG_FN_INFO(__FILENAME__,__func__," isVolumeSupported",itSources->isVolumeChangeSupported);
        LOG_FN_INFO(__FILENAME__,__func__," priority",itSources->priority);
        LOG_FN_INFO(__FILENAME__,__func__," Connection Formats Begin-------------------");
        itlistConnectionFormats = itSources->listConnectionFormats.begin();
        for(;itlistConnectionFormats != itSources->listConnectionFormats.end();++itlistConnectionFormats)
        {
            LOG_FN_INFO(__FILENAME__,__func__," Connection Format ",*itlistConnectionFormats);
        }
        LOG_FN_INFO(__FILENAME__,__func__," Connection Formats End-------------------");
        itlistMainSoundProperties = itSources->listGCMainSoundProperties.begin();
        LOG_FN_INFO(__FILENAME__,__func__," MainSoundProperties Begin----------------");
        for(;itlistMainSoundProperties != itSources->listGCMainSoundProperties.end();++itlistMainSoundProperties)
        {
            LOG_FN_INFO(__FILENAME__,__func__," Main Sound Property Type:Value:Min:Max",
                            (*itlistMainSoundProperties).type,
                            (*itlistMainSoundProperties).value,
                            (*itlistMainSoundProperties).minValue,
                            (*itlistMainSoundProperties).maxValue);
        }
        LOG_FN_INFO(__FILENAME__,__func__," MainSoundProperties End----------------");
        itlistSoundProperties = itSources->listGCSoundProperties.begin();
        LOG_FN_INFO(__FILENAME__,__func__," Sound Properties Begin-------------------");
        for(;itlistSoundProperties != itSources->listGCSoundProperties.end();++itlistSoundProperties)
        {
            LOG_FN_INFO(__FILENAME__,__func__," Type:Value:Min:Max ",
                            (*itlistSoundProperties).type,
                            (*itlistSoundProperties).value,
                            (*itlistSoundProperties).minValue,
                            (*itlistSoundProperties).maxValue
            );
        }
        LOG_FN_INFO(__FILENAME__,__func__," Sound Properties End-------------------");
        std::map<am_CustomMainSoundPropertyType_t,am_CustomSoundPropertyType_t>::iterator itMapMSPToSP;
        LOG_FN_INFO(__FILENAME__,__func__," MSP:SP Begin ------------------------");
        itMapMSPSP = itSources->mapMSPTOSP[MD_MSP_TO_SP].begin();
        for(;itMapMSPSP != itSources->mapMSPTOSP[MD_MSP_TO_SP].end();++itMapMSPSP)
        {
            LOG_FN_INFO(__FILENAME__,__func__," MSP:SP ",itMapMSPSP->first,itMapMSPSP->second);
        }
        LOG_FN_INFO(__FILENAME__,__func__," MSP:SP End ------------------------");
        itMapMSPSP = itSources->mapMSPTOSP[MD_SP_TO_MSP].begin();
        LOG_FN_INFO(__FILENAME__,__func__," SP:MSP Begin ------------------------");
        for(;itMapMSPSP != itSources->mapMSPTOSP[MD_SP_TO_MSP].end();++itMapMSPSP)
        {
            LOG_FN_INFO(__FILENAME__,__func__," SP:MSP ",itMapMSPSP->first,itMapMSPSP->second);
        }
        LOG_FN_INFO(__FILENAME__,__func__," SP:MSP End ------------------------");
        LOG_FN_INFO(__FILENAME__,__func__," Source Information end");
    }
    LOG_FN_INFO(__FILENAME__,__func__," Source Information end *****");
}
void CAmXmlConfigParser::printListPolicies(gc_Configuration_s* pConfiguration)
{
    const char* str_trigger[] =
    {
        "TRIGGER_UNKNOWN",
        "USER_CONNECTION_REQUEST",
        "USER_DISCONNECTION_REQUEST",
        "USER_SET_SINK_MUTE_STATE",
        "USER_SET_VOLUME",
        "USER_SET_SINK_MAIN_SOUND_PROPERTY",
        "USER_SET_SOURCE_MAIN_SOUND_PROPERTY",
        "USER_SET_SYSTEM_PROPERTY",
        "SYSTEM_SOURCE_AVAILABILITY_CHANGED",
        "SYSTEM_SINK_AVAILABILITY_CHANGED",
        "SYSTEM_INTERRUPT_STATE_CHANGED",
        "SYSTEM_SINK_MUTE_STATE_CHANGED",
        "SYSTEM_SINK_MAIN_SOUND_PROPERTY_CHANGED",
        "SYSTEM_SOURCE_MAIN_SOUND_PROPERTY_CHANGED",
        "SYSTEM_VOLUME_CHANGED",
        "SYSTEM_NOTIFICATION_CONFIGURATION_CHANGED",
        "SYSTEM_REGISTER_DOMAIN",
        "SYSTEM_REGISTER_SOURCE",
        "SYSTEM_REGISTER_SINK",
        "SYSTEM_REGISTER_GATEWAY",
        "SYSTEM_DEREGISTER_DOMAIN",
        "SYSTEM_DEREGISTER_SINK",
        "SYSTEM_DEREGISTER_SOURCE",
        "SYSTEM_DEREGISTER_GATEWAY",
        "SYSTEM_DOMAIN_REGISTRATION_COMPLETE",
        "SYSTEM_CONNECTION_STATE_CHANGE",
        "USER_ALL_TRIGGER",
        "TRIGGER_MAX"
    };

    std::vector<gc_Policy_s>::iterator itListPolicies;
    std::vector<int>::iterator ittrigger;
    std::vector<gc_Process_s>::iterator itprocess;
    std::vector<gc_ConditionStruct_s>::iterator itconditions;
    std::vector<gc_Action_s>::iterator itactions;
    std::map<std::string,std::string>::iterator itMapParams;
    std::multimap<std::string,std::string>::iterator itexception;
    LOG_FN_INFO(__FILENAME__,__func__," Policy Information start *****");

    for(itListPolicies = pConfiguration->listPolicies.begin();
                    itListPolicies!=pConfiguration->listPolicies.end();
                    ++itListPolicies)
    {
        LOG_FN_INFO(__FILENAME__,__func__," Policy Information");
        for(ittrigger = itListPolicies->listEvents.begin();
                        ittrigger != itListPolicies->listEvents.end();
                        ++ittrigger)
        {
            LOG_FN_INFO(__FILENAME__,__func__,"TRIGGER=",str_trigger[*ittrigger]);
        }
        for(itprocess = itListPolicies->listProcesses.begin();
                        itprocess != itListPolicies->listProcesses.end();
                        ++itprocess)
        {
            LOG_FN_INFO(__FILENAME__,__func__,"PROCESS START");
            LOG_FN_INFO(__FILENAME__,__func__,"comment", itprocess->comment);
            LOG_FN_INFO(__FILENAME__,__func__,"stopEvaluation", itprocess->stopEvaluation);
            LOG_FN_INFO(__FILENAME__,__func__,"priority", itprocess->priority);
            for(itconditions = itprocess->listConditions.begin();
                            itconditions != itprocess->listConditions.end();++itconditions)
            {
                LOG_FN_INFO(__FILENAME__,__func__,"-->LHS function name", itconditions->leftObject.functionName);
                LOG_FN_INFO(__FILENAME__,__func__,"LHS category name<--", itconditions->leftObject.category);
                LOG_FN_INFO(__FILENAME__,__func__,"LHS mandatory name<--", itconditions->leftObject.mandatoryParameter);

            }
            for(itactions=itprocess->listActions.begin();
                            itactions!=itprocess->listActions.end();
                            ++itactions)
            {
                LOG_FN_INFO(__FILENAME__,__func__,"action:", itactions->actionName);
                for(itMapParams=itactions->mapParameters.begin();
                                itMapParams!=itactions->mapParameters.end();
                                ++itMapParams)
                {
                    LOG_FN_INFO(__FILENAME__,__func__,itMapParams->first,"=",itMapParams->second);
                }
            }

            LOG_FN_INFO(__FILENAME__,__func__,"PROCESS END");
        }

        LOG_FN_INFO(__FILENAME__,__func__," END Policy Information");
    }
    LOG_FN_INFO(__FILENAME__,__func__," Policy Information end *****");

}

void CAmXmlConfigParser::printListSystemProperties(gc_Configuration_s* pConfiguration)
{
    std::vector<am_SystemProperty_s>::iterator itListSystemProperties;
    for(itListSystemProperties=pConfiguration->listSystemProperties.begin();
                    itListSystemProperties!=pConfiguration->listSystemProperties.end();++itListSystemProperties
    )
    {
        LOG_FN_INFO(__FILENAME__,__func__,"System Property Type:Value",itListSystemProperties->type,itListSystemProperties->value);
    }
}
#endif

}
/* namespace gc */
} /* namespace am */
