/*******************************************************************************
 *  \copyright (c) 2016 Advanced Driver Information Technology.
 *                   ADIT is a joint venture company of
 *   Robert Bosch GmbH/Robert Bosch Car Multimedia GmbH and DENSO Corporation
 *
 *  \author: Jens Lorenz, jlorenz@de.adit-jv.com 2015-2016
 *           Mattia Guerra, mguerra@de.adit-jv.com 2016
 *
 *
 *  \copyright The MIT License (MIT)
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"),
 *  to deal in the Software without restriction, including without limitation
 *  the rights to use, copy, modify, merge, publish, distribute, sublicense,
 *  and/or sell copies of the Software, and to permit persons to whom the
 *  Software is furnished to do so, subject to the following conditions:
 *  The above copyright notice and this permission notice shall be included in
 *  all copies or substantial portions of the Software.
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 *  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 *  DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 *  OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR
 *  THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 *  For further information see http://www.genivi.org/.
 ******************************************************************************/

#ifndef ROUTINGADAPTERKVPCONVERTER_H_
#define ROUTINGADAPTERKVPCONVERTER_H_

#include <map>
#include <vector>
#include <string>
#include <sstream>
#include <algorithm>

#define KVP_MSG(msg)   if (logErrMsg != NULL) { \
    std::ostringstream stream("CKVPConverter: "); \
    stream << (msg); \
    logErrMsg(stream.str()); }

#define KVP_DEFAULT_MSG(msg)   if (logMsg != NULL) { \
    std::ostringstream stream("CKVPConverter: "); \
    stream << "Set \"" << key << "\" to default: " << (msg); \
    logMsg(stream.str()); }

namespace am
{

/**
 * \brief Class for managing many Key-Value Pairs when collecting data from XML
 *
 * This Class offers many localizations of Key-Value Pairs gathering using templates.
 * It maps "First" XML Attributes to "Second" Values and then consumes all of them returning
 * the proper type. It has specializations for bool, vectors of bool, vectors, maps and a standard
 * approach for whatever type.
 *
 */
class CAmRoutingAdapterKVPConverter
{
public:
    /**
     *  KVP list and pair definition
     */
    typedef std::map<std::string, std::vector<std::string> > KVPList;
    typedef KVPList::value_type KVPPair;

    /**
     * KVP trace log function definition
     */
    typedef void (*KVPlog)(std::string);

    CAmRoutingAdapterKVPConverter(KVPList & kvp, KVPlog msg = NULL, KVPlog errMsg = NULL) : mKvp(kvp), logMsg(msg), logErrMsg(errMsg)
    {
    }
    ~CAmRoutingAdapterKVPConverter()
    {
        if ((mKvp.size() != 0) && (logErrMsg != NULL))
        {
            std::string strKeys;
            for (auto & itrKvp: mKvp)
            {
                strKeys.append(itrKvp.first + " ");
            }
            KVP_MSG("Unknown keys remained: " + strKeys);
        }
    }

    void registerLogMsg(KVPlog msg)
    {
        logMsg = msg;
    }

    /**
     * Template functions for finding correct key-value pattern
     */
    template <typename T>
    T kvpQueryValue(const std::string key, const T defType)
    {
        /* Find the correct key value(s) pattern */
        auto itrKVP = mKvp.find(key);
        if (itrKVP != mKvp.end())
        {
            T retVal;
            std::istringstream stream(itrKVP->second[0]);
            stream >> retVal;
            mKvp.erase(itrKVP);
            return retVal;
        }
        KVP_DEFAULT_MSG(defType);
        return defType;
    }

    template <typename T = bool>
    bool kvpQueryValue(const std::string key, const bool defType)
    {
        /* Find the correct key value(s) pattern */
        auto itrKVP = mKvp.find(key);
        if (itrKVP != mKvp.end())
        {
            std::string value = itrKVP->second.at(0);
            if ((value == "0") || (isdigit(value[0]) != 0))
            {
                mKvp.erase(itrKVP);
                return value == "0" ? false : true;
            }
            else if ((value == "false") || (value == "true"))
            {
                mKvp.erase(itrKVP);
                return value == "false" ? false : true;
            }
        }
        KVP_DEFAULT_MSG(defType);
        return defType;
    }

    template <typename T>
    std::vector<T> kvpQueryValue(const std::string key, const std::vector<T> & defType)
    {
        std::vector<T> retVector;

        /* Find the correct key value(s) pattern */
        auto itrKVP = mKvp.find(key);
        if (itrKVP != mKvp.end())
        {
            for (auto itrVal: itrKVP->second)
            {
                T val;
                std::istringstream stream(itrVal);
                stream >> val;
                retVector.push_back(val);
            }
            mKvp.erase(itrKVP);
        }

        if (retVector.size() == 0)
        {
            KVP_DEFAULT_MSG(*defType.begin());
            return defType;
        }
        return retVector;
    }

    template <typename T = bool>
    std::vector<bool> kvpQueryValue(const std::string key, const std::vector<bool> & defType)
    {
        std::vector<bool> retVector;

        /* Find the correct key value(s) pattern */
        auto itrKVP = mKvp.find(key);
        if (itrKVP != mKvp.end())
        {
            for (auto & itrVal: itrKVP->second)
            {
                std::string value(itrVal);
                if ((value == "0") || (isdigit(value[0]) != 0))
                {
                    retVector.push_back(value == "0" ? false : true);
                }
                else if ((value == "false") || (value == "true"))
                {
                    retVector.push_back(value == "false" ? false : true);
                }
            }
            mKvp.erase(itrKVP);
        }

        if (retVector.size() == 0)
        {
            KVP_DEFAULT_MSG(*defType.begin());
            return defType;
        }
        return retVector;
    }

    template <typename T, typename M = std::map<std::string, T> >
    T kvpQueryValue(const std::string key, const T defType, const M & valueMap)
    {
        T retType = defType;
        bool found = false;

        /* Find the correct key value(s) pattern */
        auto itrKVP = mKvp.find(key);
        if (itrKVP != mKvp.end())
        {
            found = translate(itrKVP->second[0], retType, valueMap);
            mKvp.erase(itrKVP);
        }

        if (found == false)
        {
            KVP_DEFAULT_MSG(defType);
        }
        return retType;
    }

    template <typename T, typename M = std::map<std::string, T> >
    std::vector<T> kvpQueryValue(const std::string key, const std::vector<T> & defType, const M & valueMap)
    {
        std::vector<T> retVector;
        size_t errcnt = 0;

        /* Find the correct key value(s) pattern */
        auto itrKVP = mKvp.find(key);
        if (itrKVP != mKvp.end())
        {
            for (auto & itrVal: itrKVP->second)
            {
                T val;
                bool found = translate(itrVal, val, valueMap);
                if (found == true)
                {
                    retVector.push_back(val);
                }
                else
                {
                    errcnt++;
                }
            }
            mKvp.erase(itrKVP);
        }

        if (errcnt > 0)
        {
            KVP_MSG(errcnt + " of \"" + key + "\" are not converted");
        }

        if (retVector.size() == 0)
        {
            KVP_DEFAULT_MSG(*defType.begin());
            return defType;
        }
        return retVector;
    }

private:

    template <typename T, typename M = std::map<std::string, T> >
    bool translate(const std::string value, T & type, const M & valueMap)
    {
        for (auto & itrPair: valueMap)
        {
            if (itrPair.first == value)
            {
                type = itrPair.second;
                return true;
            }
        }
        return false;
    }

private:

    KVPList & mKvp;
    void (*logMsg)(std::string);
    void (*logErrMsg)(std::string);
};

} /* namespace am */

#endif /* ROUTINGADAPTERKVPCONVERTER_H_ */
