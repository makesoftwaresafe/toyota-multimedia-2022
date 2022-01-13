/******************************************************************************
 * @file: CAmPersistenceDefault.h
 *
 * This file contains the declaration of Persistence default class
 * (member functions and data members) used to implement the logic of
 * reading and writing data for last main connection and last volume
 * from the file system (RFS)
 *
 * @component: AudioManager Generic Controller
 *
 * @author: Naohiro Nishiguchi<nnishiguchi@jp.adit-jv.com>
 *          Kapildev Patel, Nilkanth Ahirrao  <kpatel@jp.adit-jv.com>
 *
 * @copyright (c) 2015 Advanced Driver Information Technology.
 * This code is developed by Advanced Driver Information Technology.
 * Copyright of Advanced Driver Information Technology, Bosch, and DENSO.
 * All rights reserved.
 *
 *****************************************************************************/
#ifndef GC_PERSISTENCE_DEFAULT_H_
#define GC_PERSISTENCE_DEFAULT_H_

#include "IAmPersistence.h"
#include <string>
#include <fstream>
#include <map>

namespace am {
namespace gc {

typedef std::map<std::string, std::string> MapData;

class CAmPersistenceDefault : public IAmPersistence
{
private:

    std::fstream mFstream;
    MapData mFileData;
    std::string mFileName;
public:
    CAmPersistenceDefault();
    ~CAmPersistenceDefault();
    am_Error_e open(const std::string& appName)final;
    am_Error_e read(const std::string& keyName,std::string& readData,int dataSize=READ_SIZE)final;
    am_Error_e write(const std::string& keyName,const std::string& writeData,int dataSize=WRITE_SIZE)final;
    am_Error_e close()final;
private:
    am_Error_e  _createDirectory(const std::string& path, mode_t mode);
    void _createSubDirectories();
};
} /* namespace gc */
} /* namespace am */
#endif /* GC_PERSISTENCE_DEFAULT_H_ */
