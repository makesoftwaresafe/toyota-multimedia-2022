/************************************************************************
 * @file: CDBusCommon.cpp
 *
 * @version: 1.1
 *
 * @description: This file include all global variable list
 *
 * @component: platform/audiomanager
 *
 * @author: Jens Lorenz, jlorenz@de.adit-jv.com 2013,2014
 *          Jayanth MC, Jayanth.mc@in.bosch.com 2013,2014
 *
 * @copyright (c) 2010, 2011 Advanced Driver Information Technology.
 * This code is developed by Advanced Driver Information Technology.
 * Copyright of Advanced Driver Information Technology, Bosch, and DENSO.
 * All rights reserved.
 *
 * @see <related items>
 *
 * @history
 *
 ***********************************************************************/

#include <stdlib.h> // for getenv()
#include <fstream> //for file operations
#include <vector>
#include <map>
#include "CAmDltWrapper.h"
#include "CDBusCommon.h"

using namespace am;
using namespace std;

CDBusCommon::CDBusCommon()
{
    /*Do nothing*/
}

CDBusCommon::~CDBusCommon()
{
    /*Do nothing*/
}

void CDBusCommon::removeWhiteSpaces(string & str) const
{
    size_t count = string::npos;
    // remove space and tab from string
    while (string::npos != (count = str.find(' ')) || string::npos != (count = str.find('\t')))
    {
        str.erase(count, 1);
    }
}

void CDBusCommon::getKeyValPairs(xmlNode *devNode, map<string, string> & keyValPairs)
{
    string key, value;

    xmlNode *devMemNode = NULL; // for all device parameters
    for (devMemNode = devNode->children; devMemNode; devMemNode = devMemNode->next)
    {
        if (devMemNode->type == XML_ELEMENT_NODE) // inside device parameter node
        {
            key = (char*) devMemNode->name;
            removeWhiteSpaces(key);
            value = (char *) xmlNodeListGetString(devMemNode->doc, devMemNode->xmlChildrenNode, 1);
            removeWhiteSpaces(value);
            if (value.length())
            {
                keyValPairs.insert(make_pair(key, value));
            }
            else
            {
                logInfo("key Value is not defined, for key: ", key);
            }
        }
    }
}

