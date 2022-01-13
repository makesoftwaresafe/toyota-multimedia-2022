/*
 * Copyright (c) 2004 X-Way Rights BV
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/*!\file Lock.h
 * \ingroup CXX_UTIL_CONCURRENT_LOCKS_m
 */

#ifndef _INTERFACE_BEE_UTIL_CONCURRENT_LOCKS_LOCK_H
#define _INTERFACE_BEE_UTIL_CONCURRENT_LOCKS_LOCK_H

#ifdef __cplusplus

#include "beecrypt/c++/lang/InterruptedException.h"
using beecrypt::lang::InterruptedException;
#include "beecrypt/c++/util/concurrent/locks/Condition.h"
using beecrypt::util::concurrent::locks::Condition;

namespace beecrypt {
	namespace util {
		namespace concurrent {
			namespace locks {
				/*!\ingroup CXX_UTIL_CONCURRENT_LOCKS_m
				 */
				class BEECRYPTCXXAPI Lock
				{
				public:
					virtual ~Lock() {}

					virtual void lock() = 0;
					virtual void lockInterruptibly() throw (InterruptedException) = 0;
					virtual Condition* newCondition() = 0;
					virtual bool tryLock() = 0;
					virtual void unlock() = 0;
				};
			}
		}
	}
}

#endif

#endif
