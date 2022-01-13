/******************************************************************************
 * @file: CAmLogger.h
 *
 * This file contains the declaration of functions used for logging purpose
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

#ifndef GC_LOGGER_H_
#define GC_LOGGER_H_

#include "IAmControlCommon.h"

extern DltContext GenericControllerDlt;

namespace am {
namespace gc {

#define LOG_DEBUG_DEFAULT_VALUE 4

/**
 * logs a given value with error level with the default context
 * @param ...
 */
template <typename ...Args> void CAmLogDebug(Args... args)
{
    log(&GenericControllerDlt, DLT_LOG_DEBUG, args...);
}



/*
 * Helper functions to log
 */
template <typename ...Args> inline void LOG_FN_ENTRY(Args... args)
{
    CAmLogDebug(">> ", args...);
}

template <typename ...Args> inline void LOG_FN_EXIT(Args... args)
{
    CAmLogDebug("<<", args...);
}

template <typename ...Args> inline void LOG_FN_DEBUG(Args... args)
{
    CAmLogDebug(args...);
}

/**
 * logs a given value with warning level with the default context
 * @param ...
 */
template <typename ...Args> inline void LOG_FN_WARN(Args... args)
{
    log(&GenericControllerDlt, DLT_LOG_WARN, args...);
}

/**
 * logs a given value with error level with the default context
 * @param ...
 */
template <typename ...Args> inline void LOG_FN_ERROR(Args... args)
{
    log(&GenericControllerDlt, DLT_LOG_ERROR, args...);
}

/**
 * logs a given value with infolevel with the default context
 * @param ...
 */
template <typename ...Args> inline void LOG_FN_INFO(Args... args)
{
    log(&GenericControllerDlt, DLT_LOG_INFO, args...);
}

inline void LOG_FN_REGISTER_CONTEXT()
{
    CAmDltWrapper::instance()->registerContext(GenericControllerDlt, "AMCO", "Generic Controller Context", DLT_LOG_DEFAULT, DLT_TRACE_STATUS_OFF);
}

template <typename T> inline void LOG_FN_CHANGE_LEVEL(T level)
{
    CAmDltWrapper::instance()->unregisterContext(GenericControllerDlt);
    CAmDltWrapper::instance()->registerContext(GenericControllerDlt, "AMCO", "Generic Controller Context", static_cast<DltLogLevelType>(level), DLT_TRACE_STATUS_OFF);
}

} /* namespace gc */
} /* namespace am */
#endif /* GC_LOGGER_H_ */
