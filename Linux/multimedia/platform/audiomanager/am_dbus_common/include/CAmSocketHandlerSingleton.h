#ifndef DBUSWRAPPERSINGLETON_H_
#define DBUSWRAPPERSINGLETON_H_

#include "CAmSocketHandler.h"
#include "mutex"

namespace am
{

class CAmSocketHandlerSingleton
{
public:
    static CAmSocketHandler* getSocketHandler();
    static void startSocketHandler();
private:
    CAmSocketHandlerSingleton(){}
    virtual ~CAmSocketHandlerSingleton(){}
    static CAmSocketHandler mpSocketHandlerSingleton;
    static std::once_flag once_flag;
};

}

#endif /* DBUSWRAPPERSINGLETON_H_ */
