/******************************************************************************
 * @file: CAmPersistenceDefault.cpp
 *
 * This file contains the definition of Persistence default class
 * (member functions and data members) used to implement the logic of
 * reading and writing data for last main connection and last volume
 * from the file system (RFS)
 *
 * @component: AudioManager Generic Controller
 *
 * @author: Naohiro Nishiguchi<nnishiguchi@jp.adit-jv.com>
 *          Kapildev Patel, Nilkanth Ahirrao <kpatel@jp.adit-jv.com>
 *
 * @copyright (c) 2015 Advanced Driver Information Technology.
 * This code is developed by Advanced Driver Information Technology.
 * Copyright of Advanced Driver Information Technology, Bosch, and DENSO.
 * All rights reserved.
 *
 *****************************************************************************/
#include "CAmPersistenceDefault.h"
#include "CAmLogger.h"
#include <string>
#include <sys/stat.h>

namespace am {
namespace gc {

#define GENERIC_CONTROLLER_DEFAULT_PERISTENCE_FILE_PATH "/var/ADIT/persistent/gc_persistence_data.txt"
#define PERISTENCE_FILE_PATH_ENV_VAR_NAME "GENERIC_CONTROLLER_PERISTENCE_FILE_PATH"
#define FILE_ACCESS_MODE 0777

CAmPersistenceDefault::CAmPersistenceDefault():
                                mFileName(GENERIC_CONTROLLER_DEFAULT_PERISTENCE_FILE_PATH)
{
}
CAmPersistenceDefault::~CAmPersistenceDefault()
{
}

std::istream& operator >> ( std::istream& inStream, MapData& Data )
{
    std::string stream, key, value;
    // loop for each (key, value) pair in the file
    while (std::getline( inStream, stream ))
    {
        std::string::size_type begin = stream.find_first_not_of( " \f\t\v" );
        // Skip blank lines
        if (begin == std::string::npos) continue;
        // Extract the key value
        std::string::size_type end = stream.find( '=', begin );
        if (end == std::string::npos) continue;
        key = stream.substr( begin, end - begin );
        // (No leading or trailing whitespace allowed)
        key.erase( key.find_last_not_of( " \f\t\v" ) + 1 );
        // No blank keys allowed
        if (key.empty()) continue;
        // Extract the value (no leading or trailing whitespace allowed)
        begin = stream.find_first_not_of( " \f\n\r\t\v", end +1 );
        end   = stream.find_last_not_of(  " \f\n\r\t\v" )+1;
        if((end != begin) && (end > begin) )
        {
            value = stream.substr( begin, end - begin );
            // Insert the properly extracted (key, value) pair into the map
            Data[ key ] = value;
            LOG_FN_DEBUG(__FILENAME__,__func__,"READ KEY:VALUE",key, value);
        }
    }
    return inStream;
}

std::ostream& operator << ( std::ostream& outStream, const MapData& fileData )
{
    MapData::const_iterator itFileData;
    for (itFileData = fileData.begin(); itFileData != fileData.end(); itFileData++)
    {
        outStream << itFileData->first << " = " << itFileData->second << std::endl;
    }
    return outStream;
}

am_Error_e CAmPersistenceDefault::open(const std::string& appName)
{
    LOG_FN_INFO("CAmPersistenceDefault::open", appName.c_str());
    char* persistencePath =  (char*)getenv(PERISTENCE_FILE_PATH_ENV_VAR_NAME);
    if(persistencePath != NULL)
    {
        mFileName = persistencePath;
    }
    _createSubDirectories();
    mFstream.open(mFileName, std::fstream::in|std::fstream::out | std::fstream::app);
    if (mFstream.is_open())
    {
        mFstream >> mFileData;
        mFstream.close();
        return E_OK;
    }
    else
    {
        LOG_FN_ERROR(__FILENAME__,__func__,"file open failed",persistencePath);
    }
    return E_DATABASE_ERROR;
}

am_Error_e CAmPersistenceDefault::read( const std::string& keyName,
                                        std::string& readData,int dataSize)
{
    MapData::iterator itFileData;
    itFileData = mFileData.find(keyName);
    if (itFileData != mFileData.end())
    {
        readData = itFileData->second;
        return E_OK;
    }
    else
    {
        LOG_FN_INFO(__FILENAME__,__func__,"Read key not found");
    }
    return E_DATABASE_ERROR;
}

am_Error_e CAmPersistenceDefault::write(const std::string& keyName,
                                        const std::string& writeData,
                                        int dataSize)
{
    LOG_FN_DEBUG(__FILENAME__,__func__,"Persistence write keyName: ", keyName,"write data: ", writeData);
    mFileData[keyName] = writeData;
    mFstream.open(mFileName,std::ios::out|std::ios::trunc);
    if(mFstream.is_open())
    {
        mFstream << mFileData;
        mFstream.close();
        return E_OK;
    }
    else
    {
        LOG_FN_INFO(__FILENAME__,__func__,"Persistence Write failed, file not open");
    }
    return E_DATABASE_ERROR;
}

am_Error_e CAmPersistenceDefault::close()
{
    return E_OK;
}

am_Error_e CAmPersistenceDefault::_createDirectory(const std::string& path, mode_t mode)
{
    struct stat st;

    if (stat(path.c_str(), &st) == 0)
        return E_OK;
    if (mkdir(path.c_str(), mode) != 0 && errno != EEXIST)
        return E_NOT_POSSIBLE;
    return E_OK;
}

void CAmPersistenceDefault::_createSubDirectories()
{
    if(mFileName.empty())
        return;
    for(size_t pos = 0; (pos = mFileName.find("/", pos)) != std::string::npos; ++pos)
    {
        std::string dirName = mFileName.substr(0, pos);
        LOG_FN_INFO(__FILENAME__,__func__,"directory name ->",dirName);
        am_Error_e error = _createDirectory(dirName, FILE_ACCESS_MODE);
        if(error == E_NOT_POSSIBLE)
            LOG_FN_WARN(__FILENAME__,__func__, "failed to create directory", dirName);
    }
}

} /* namespace gc */
} /* namespace am */
