/*
 * aoap_logging.h
 *
 *  Created on: Jul 18, 2013
 *      Author: Thilo Bjoern Fickel
 *
 *  This source code is proprietary of ADIT
 *  Copyright (C) Advanced Driver Information Technology Joint Venture GmbH
 *  All rights reserved
 */

#ifndef AOAP_LOGGING_H_
#define AOAP_LOGGING_H_

#include "aoap_types.h"

namespace AOAP
{
    namespace Logging
    {
        /**
         * @typedef typedef enum LogLevel tLogLevel
         * @enum LogLevel
         * The different log levels for DLT tracing
         */
        typedef enum LogLevel
        {
            eLogDefault = -1,  //!< eLogDefault
            eLogOff = 0x00,    //!< eLogOff
            eLogFatal = 0x02,  //!< eLogFatal
            eLogError = 0x02,  //!< eLogError
            eLogWarn = 0x03,   //!< eLogWarn
            eLogInfo = 0x04,   //!< eLogInfo
            eLogDebug = 0x05,  //!< eLogDebug
            eLogVerbose = 0x06 //!< eLogVerbose
        } tLogLevel;

        /**
         * @brief Get current time as string
         *
         * @param[in,out] pBuffer The buffer where to store the timestamp
         * @param bufferLen The size of the buffer provided
         * @return The pointer to the buffer
         */
        extern "C" { const char* getCurrentTime(char *pBuffer, unsigned int bufferLen); }

        /**
         * @brief Enable/disable DLT logging
         *
         * @param enable Set to true when DLT logging shall be enabled, otherwise set it to false
         */
        extern "C" { void setDltLogging(bool enable); }

        /**
         * @brief Get DLT Logging enabled/disabled
         *
         * @return true when DLT logging is enabled, otherwise false
         */
        extern "C" { bool getDltLogging(void); }

        /**
         * @brief Enable logging to specified destination
         *
         * @param destination the log destination to be enabled
         */
        extern "C" { void enableLogging(t_aoap_logging_destination destination); }

        /**
         * @brief Disable logging to specified destination
         *
         * @param destination the log destination to be disabled
         */
        extern "C" { void disableLogging(t_aoap_logging_destination destination); }

        /**
         * @brief Set the logging level.
         *
         * Note: This will not disable DLT any more.
         *
         * @param logLevel The new log level
         * @param prependTime Prepend a timestamp
         */
        extern "C" { void setLogLevel(int logLevel, bool prependTime); }

        /**
         * @brief Get the logging levels.
         *
         * @return Returns the logging destinations as bitfield.
         */
        extern "C" { unsigned int getLogDestination(void); }

        /**
         * @brief Enable/disable the specified logging destination
         *
         * @param destination The log destination to enabled or disabled
         * @param enable TRUE to enable specified log destination. FALSE to disable log destination.
         *
         */
        extern "C" { void setLogDestination(t_aoap_logging_destination destination, bool enable, const char* filename); }

        /**
         * @brief Get logging level
         *
         * @return The active logging level
         */
        extern "C" { int getLogLevel(void); }

        /**
         * @brief Logging for AOAP.
         *
         * Prints the specified message and adds a line break to the end
         *
         * @param level The log level
         * @param message The message to be printed
         */
        extern "C" { void dbgPrintLine(tLogLevel level, const char *message, ...); }

        /**
         * @brief Logging for AOAP.
         *
         * Prints the specified message (without appending automatically
         * a line break)
         *
         * @param level The log level
         * @param message The message to be printed
         */
        extern "C" { void dbgPrint(tLogLevel level, const char *message, ...); }

        /**
         * @brief Register with DLT
         */
        extern "C" { void registerWithDlt(void); }

        /**
         * @brief Unregister with DLT
         */
        extern "C"{  void unregisterWithDlt(void); }
    }
}

#endif /* AOAP_LOGGING_H_ */
