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

/*!\file ReentrantLock.h
 * \ingroup CXX_UTIL_CONCURRENT_LOCKS_m
 */

#ifndef _CLASS_BEE_UTIL_CONCURRENT_LOCKS_REENTRANTLOCK_H
#define _CLASS_BEE_UTIL_CONCURRENT_LOCKS_REENTRANTLOCK_H

#ifdef __cplusplus

#include "beecrypt/c++/lang/Object.h"
using beecrypt::lang::Object;
#include "beecrypt/c++/util/concurrent/locks/Condition.h"
using beecrypt::util::concurrent::locks::Condition;
#include "beecrypt/c++/util/concurrent/locks/Lock.h"
using beecrypt::util::concurrent::locks::Lock;

namespace beecrypt {
	namespace util {
		namespace concurrent {
			namespace locks {
				/*!\ingroup CXX_UTIL_CONCURRENT_LOCKS_m
				 */
				class BEECRYPTCXXAPI ReentrantLock : public Object, public virtual Lock
				{
				private:
					class BEECRYPTCXXAPI Cond : public Object, public virtual Condition
					{
					private:
						Object* lock;

					public:
						Cond(Object*);
						virtual ~Cond() {}

						virtual void await() throw (InterruptedException);
						virtual void awaitUninterruptibly();
						virtual void signal();
						virtual void signalAll();
					};

					bool _fair;

				public:
					ReentrantLock(bool fair = false);
					virtual ~ReentrantLock() {}

					virtual void lock();
					virtual void lockInterruptibly() throw (InterruptedException);
					virtual Condition* newCondition();
					virtual bool tryLock();
					virtual void unlock();
				};
			}
		}
	}
}

#endif

#endif
