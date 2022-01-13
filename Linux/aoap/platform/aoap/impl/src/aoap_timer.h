/*
 * aoap_timer.h
 *
 *  Created on: Aug 20, 2013
 *      Author: Thilo Bjoern Fickel
 *
 *  This source code is proprietary of ADIT
 *  Copyright (C) Advanced Driver Information Technology Joint Venture GmbH
 *  All rights reserved
 */

#ifndef AOAP_TIMER_H_
#define AOAP_TIMER_H_

#include <time.h>
#include <signal.h>

namespace AOAP
{
    namespace Time
    {
        /**
         * @class Timer aoap_timer.h "aoap_timer.h"
         * A timer implementation based on timer_create
         */
        class Timer
        {
        public:

            /**
             * @brief Constructor of Timer
             */
            Timer();

            /**
             * @brief Destructor of Timer
             */
            virtual ~Timer();

            /**
             * @brief Start the timer with the specified time
             *
             * @param seconds The seconds of the timeout
             * @param mseconds The milliseconds of the timeout. Default is 0 ms
             *
             */
            void start(unsigned int seconds, unsigned int mseconds = 0);

            /**
             * @brief Stop the timer
             */
            void stop(void);

            /**
             * @brief The static timeout handler (refer to timer_create)
             */
            static void handler(union sigval sig);

            /**
             * @brief The timeout handler called by the static handler which
             *        must be implemented by derived classes
             */
            virtual void timeoutHandler(void) = 0;

        protected:

            /** The start time of the timer (only for verification of working timer) */
            struct timeval mStartTime;

        private:

            /** true when the timer is running */
            bool mTimerStarted;

            /** true when the timer was created */
            bool mTimerCreated;

            /** The id of the new timer is returned by timer_create()
              * in the buffer pointed by mTimerId. */
            timer_t mTimerId;

            /** The timer event */
            sigevent mTimerEvent;

            /** The timer structure for start and interval values */
            struct itimerspec mITimerSpec;

            /**  global Id of the Timer object. */
            int mTimerObjectId;
        };
    }
}
#endif /* AOAP_TIMER_H_ */
