

#include "CAmSocketHandlerSingleton.h"
#include "CAmDltWrapper.h"
namespace am
{

std::once_flag CAmSocketHandlerSingleton::once_flag;
CAmSocketHandler CAmSocketHandlerSingleton::mpSocketHandlerSingleton;

CAmSocketHandler* CAmSocketHandlerSingleton::getSocketHandler()
{
    return &mpSocketHandlerSingleton;
}

void CAmSocketHandlerSingleton::startSocketHandler()
{
    std::call_once(once_flag, [] (){
        mpSocketHandlerSingleton.start_listenting();
    });

}

}
