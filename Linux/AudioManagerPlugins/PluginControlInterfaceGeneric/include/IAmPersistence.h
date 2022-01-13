/******************************************************************************
 * @file: IAmPersistence.h
 *
 * This file contains the declaration of abstract class used to provide
 * the generic interface for persistence specific implementation for e.g
 * GENIVI persistence or RFS or any other proprietary persistence. Framework
 * will use this interface to read / write data to the persistence media,
 * eventhough the persistence specific implementation change the framework
 * side will remain unchanged
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
#ifndef GC_IPERSISTENCE_H_
#define GC_IPERSISTENCE_H_
#include <stdint.h>
#include <sstream>
#include <vector>
#include "audiomanagertypes.h"

namespace am {
namespace gc {

#define READ_SIZE    1024
#define WRITE_SIZE   1024

class IAmPersistence
{
public:
    virtual ~IAmPersistence() {};
    IAmPersistence(){};
    virtual am_Error_e open(const std::string& appName) = 0;
    virtual am_Error_e read(const std::string& keyName,std::string& readData,int dataSize=READ_SIZE) = 0;
    virtual am_Error_e write(const std::string& keyName,const std::string& writeData,int dataSize=WRITE_SIZE)= 0;
    virtual am_Error_e close()=0;
};

} /* namespace gc */
} /* namespace am */
#endif /*GC_IPERSISTENCE_H_*/
