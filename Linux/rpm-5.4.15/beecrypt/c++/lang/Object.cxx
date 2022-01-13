/*
 * Copyright (c) 2004, 2005 Beeyond Software Holding BV
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

#define BEECRYPT_CXX_DLL_EXPORT

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#if HAVE_ERRNO_H
# include <errno.h>
#endif

#include <string.h>

#include "beecrypt/timestamp.h"

#include "beecrypt/c++/lang/Object.h"
#include "beecrypt/c++/lang/Integer.h"
#include "beecrypt/c++/lang/Thread.h"
#include "beecrypt/c++/lang/Error.h"
#include "beecrypt/c++/lang/CloneNotSupportedException.h"
#include "beecrypt/c++/lang/IllegalMonitorStateException.h"
#include "beecrypt/c++/lang/InterruptedException.h"
#include "beecrypt/c++/lang/StringBuilder.h"
using namespace beecrypt::lang;

#if HAVE_PTHREAD_H
# include "beecrypt/c++/posix.h"
#endif

#include <typeinfo>

Object::Monitor::Monitor() : _owner(0), _interruptee(0)
{
	#if WIN32
	if (!(_lock = CreateMutex(0, FALSE, 0)))
		throw Error("CreateMutex failed");
	#elif HAVE_SYNCH_H
	if (mutex_init(&_lock, USYNC_THREAD, 0))
		throw Error("mutex_init failed");
	#elif HAVE_PTHREAD_H
	posixErrorDetector(pthread_mutex_init(&_lock, 0), "pthread_mutex_init failed ");
	#else
	# error
	#endif
	_lock_count = 0;
}

void Object::Monitor::internal_state_lock()
{
	#if WIN32
	if (WaitForSingleObject(_lock, INFINITE) != WAIT_OBJECT_0)
		throw RuntimeException("WaitForSingleObject failed");
	#elif HAVE_SYNCH_H
	if (mutex_lock(&_lock))
		throw RuntimeException("mutex_lock failed");
	#elif HAVE_PTHREAD_H
	posixErrorDetector(pthread_mutex_lock(&_lock),
		"pthread_mutex_lock failed in Object::Monitor::internal_state_lock()");
	#else
	# error
	#endif
}

void Object::Monitor::internal_state_unlock()
{
	#if WIN32
	if (!ReleaseMutex(_lock))
		throw RuntimeException("ReleaseMutex failed");
	#elif HAVE_SYNCH_H
	if (mutex_unlock(&_lock))
		throw RuntimeException("mutex_unlock failed");
	#elif HAVE_PTHREAD_H
	posixErrorDetector(pthread_mutex_unlock(&_lock),
		"pthread_mutex_unlock failed in Object::Monitor::internal_state_unlock()");
	#else
	# error
	#endif
}
    
bool Object::Monitor::interrupted(bc_threadid_t target)
{
	bool result;
	internal_state_lock();
	result = (_interruptee == target);
	if (result)
		_interruptee = false;
	internal_state_unlock();
	return result;
}

bool Object::Monitor::isInterrupted(bc_threadid_t target)
{
	bool result;
	internal_state_lock();
	result = (_interruptee == target);
	internal_state_unlock();
	return result;
}

bool Object::Monitor::isLocked()
{
	bool result;
	internal_state_lock();
	result = (_lock_count != 0);
	internal_state_unlock();
	return result;
}

Object::Monitor* Object::Monitor::getInstance(bool fair)
{
	if (fair)
		return new FairMonitor();
	else
		return new NonfairMonitor();
}

Object::NonfairMonitor::NonfairMonitor()
{
	#if WIN32
	if (!(_lock_sig = CreateSemaphore(NULL, 0, 0x7fffffff, NULL)))
		throw Error("CreateSemaphore failed");
	_lock_sig_all = false;
	if (!(_lock_sig_all_done = CreateEvent(NULL, FALSE, FALSE, NULL)))
		throw Error("CreateEvent failed");
	if (!(_notify_sig = CreateSemaphore(NULL, 0, 0x7fffffff, NULL)))
		throw Error("CreateSemaphore failed");
	_notify_sig_all = false;
	if (!(_notify_sig_all_done = CreateEvent(NULL, FALSE, FALSE, NULL)))
		throw Error("CreateEvent failed");
	#elif HAVE_SYNCH_H
	if (cond_init(&_lock_sig, USYNC_THREAD, 0))
		throw Error("cond_init failed");
	if (cond_init(&_notify_sig, USYNC_THREAD, 0))
		throw Error("cond_init failed");
	#elif HAVE_PTHREAD_H
	posixErrorDetector(pthread_cond_init(&_lock_sig, 0), "pthread_cond_init failed");
	posixErrorDetector(pthread_cond_init(&_notify_sig, 0), "pthread_cond_init failed");
	#else
	# error
	#endif

	_lock_wthreads = 0;
	_notify_wthreads = 0;
}

Object::NonfairMonitor::~NonfairMonitor()
{
	#if WIN32
	if (!CloseHandle(_lock))
		throw Error("CloseHandle failed");
	if (!CloseHandle(_lock_sig))
		throw Error("CloseHandle failed");
	if (!CloseHandle(_lock_sig_all_done))
		throw Error("CloseHandle failed");
	if (!CloseHandle(_notify_sig))
		throw Error("CloseHandle failed");
	if (!CloseHandle(_notify_sig_all_done))
		throw Error("CloseHandle failed");
	#elif HAVE_SYNCH_H
	if (mutex_destroy(&_lock))
		throw Error("mutex_destroy failed");
	if (cond_destroy(&_lock_sig))
		throw Error("cond_destroy failed");
	if (cond_destroy(&_notify_sig))
		throw Error("cond_destroy failed");
	#elif HAVE_PTHREAD_H
	posixErrorDetector(pthread_mutex_lock(&_lock), "pthread_mutex_lock failed in Object::NonfairMonitor::~NonfairMonitor()");
	posixErrorDetector(pthread_mutex_unlock(&_lock), "pthread_mutex_unlock failed");
	posixErrorDetector(pthread_mutex_destroy(&_lock), "pthread_mutex_destroy failed");
	posixErrorDetector(pthread_cond_destroy(&_lock_sig), "pthread_cond_destroy failed");
	posixErrorDetector(pthread_cond_destroy(&_notify_sig), "pthread_cond_destroy failed");
	#else
	# error
	#endif
}

void Object::NonfairMonitor::interrupt(bc_threadid_t target)
{
	internal_state_lock();

	_interruptee = target;

	if (_notify_wthreads)
	{
		#if WIN32
		_notify_sig_all = true;
		if (!ReleaseSemaphore(_notify_sig, _notify_wthreads, 0))
			throw Error("ReleaseSemaphore failed");
		#elif HAVE_SYNCH_H
		if (cond_broadcast(&_notify_sig))
			throw Error("cond_broadcast failed");
		#elif HAVE_PTHREAD_H
		posixErrorDetector(pthread_cond_broadcast(&_notify_sig), "pthread_cond_broadcast failed");
		#else
		# error
		#endif
	}

	internal_state_unlock();
}

void Object::NonfairMonitor::lock()
{
	#if WIN32
	bc_threadid_t self = GetCurrentThreadId();
	#elif HAVE_SYNCH_H
	bc_threadid_t self = thr_self();
	#elif HAVE_PTHREAD_H
	bc_threadid_t self = pthread_self();
	#else
	# error
	#endif

	internal_state_lock();

	if (_lock_count)
	{
		if (_owner == self)
		{
			if (++_lock_count == 0)
				throw Error("maximum lock count exceeded");

			internal_state_unlock();
			return;
		}
		else
		{
			if (++_lock_wthreads == 0)
				throw Error("maximum waiting threads exceeded");

			while (_lock_count)
			{
				#if WIN32
				switch (SignalObjectAndWait(_lock, _lock_sig, INFINITE, FALSE))
				{
				case WAIT_OBJECT_0:
					break;
				default:
					throw Error("SignalObjectAndWait failed");
				}
				if (WaitForSingleObject(_lock, INFINITE) != WAIT_OBJECT_0)
					throw Error("WaitForSingleObject failed");
				#elif HAVE_SYNCH_H
				switch (cond_wait(&_lock_sig, &_lock))
				{
				case EINTR:
				case 0:
					break;
				default:
					throw Error("cond_wait failed");
				}
				#elif HAVE_PTHREAD_H
				int err;
				switch ((err = pthread_cond_wait(&_lock_sig, &_lock)))
				{
				case EINTR:
				case 0:
					break;
				default:
					posixErrorDetector(err, "pthread_cond_wait failed");
				}
				#else
				# error
				#endif
			}
			_lock_wthreads--;
			#if WIN32
			if (_lock_sig_all && (_lock_wthreads == 0))
			{
				if (!ReleaseMutex(_lock))
					throw Error("ReleaseMutex failed");
				switch (SignalObjectAndWait(_lock_sig_all_done, _lock, INFINITE, FALSE))
				{
				case WAIT_OBJECT_0:
					break;
				default:
					throw Error("SetEvent failed");
				}
			}
			#endif
		}
	}

	_owner = self;
	_lock_count = 1;

	internal_state_unlock();
}

void Object::NonfairMonitor::lockInterruptibly() throw (InterruptedException)
{
	bool interrupted = false;

	#if WIN32
	bc_threadid_t self = GetCurrentThreadId();
	#elif HAVE_SYNCH_H
	bc_threadid_t self = thr_self();
	#elif HAVE_PTHREAD_H
	bc_threadid_t self = pthread_self();
	#else
	# error
	#endif

	internal_state_lock();

	if (_interruptee == self)
	{
		interrupted = true;
		_interruptee = 0;
	}

	if (!interrupted)
	{
		if (_lock_count)
		{
			if (_owner == self)
			{
				if (++_lock_count == 0)
					throw Error("maximum lock count exceeded");

				internal_state_unlock();
				return;
			}
			else
			{
				if (++_lock_wthreads == 0)
					throw Error("maximum waiting threads exceeded");

				while (_lock_count)
				{
					#if WIN32
					switch (SignalObjectAndWait(_lock, _lock_sig, INFINITE, FALSE))
					{
					case WAIT_OBJECT_0:
						break;
					default:
						throw Error("SignalObjectAndWait failed");
					}
					if (WaitForSingleObject(_lock, INFINITE) != WAIT_OBJECT_0)
						throw Error("WaitForSingleObject failed");
					#elif HAVE_SYNCH_H
					switch (cond_wait(&_lock_sig, &_lock))
					{
					case EINTR:
						interrupted = true;
					case 0:
						break;
					default:
						throw Error("cond_wait failed");
					}
					#elif HAVE_PTHREAD_H
					int err;
					switch ((err = pthread_cond_wait(&_lock_sig, &_lock)))
					{
					case EINTR:
						interrupted = true;
					case 0:
						break;
					default:
						posixErrorDetector(err, "pthread_cond_wait failed");
					}
					#else
					# error
					#endif
				}
				_lock_wthreads--;
				#if WIN32
				if (_lock_sig_all && (_lock_wthreads == 0))
				{
					if (!ReleaseMutex(_lock))
						throw Error("ReleaseMutex failed");
					switch (SignalObjectAndWait(_lock_sig_all_done, _lock, INFINITE, FALSE))
					{
					case WAIT_OBJECT_0:
						break;
					default:
						throw Error("SetEvent failed");
					}
				}
				#endif
			}
		}
		if (!interrupted)
		{
			_owner = self;
			_lock_count = 1;
		}
	}

	internal_state_unlock();

	if (interrupted)
		throw InterruptedException();
}

bool Object::NonfairMonitor::tryLock()
{
	bool result = false;

	#if WIN32
	bc_threadid_t self = GetCurrentThreadId();
	#elif HAVE_SYNCH_H
	bc_threadid_t self = thr_self();
	#elif HAVE_PTHREAD_H
	bc_threadid_t self = pthread_self();
	#else
	# error
	#endif

	internal_state_lock();

	if (_lock_count)
	{
		result = (_owner == self);
		if (result)
			if (++_lock_count == 0)
				throw Error("maximum lock count exceeded");
	}
	else
	{
		_lock_count = 1;
		_owner = self;
	}

	internal_state_unlock();

	return result;
}

void Object::NonfairMonitor::unlock()
{
	bool owned;

	#if WIN32
	bc_threadid_t self = GetCurrentThreadId();
	#elif HAVE_SYNCH_H
	bc_threadid_t self = thr_self();
	#elif HAVE_PTHREAD_H
	bc_threadid_t self = pthread_self();
	#else
	# error
	#endif

	internal_state_lock();

	if ((owned = (_owner == self)))
	{
		if (!--_lock_count)
		{
			if (_lock_wthreads)
			{
				#if WIN32
				if (!ReleaseSemaphore(_lock_sig, 1, 0))
					throw Error("ReleaseSemaphore failed");
				#elif HAVE_SYNCH_H
				if (cond_signal(&_lock_sig))
					throw Error("cond_signal failed");
				#elif HAVE_PTHREAD_H
				posixErrorDetector(pthread_cond_signal(&_lock_sig), "pthread_cond_signal failed");
				#else
				# error
				#endif
			}
			_owner = 0;
		}
	}

	internal_state_unlock();

	if (!owned)
		throw IllegalMonitorStateException();
}

void Object::NonfairMonitor::notify()
{
	bool owned;

	#if WIN32
	bc_threadid_t self = GetCurrentThreadId();
	#elif HAVE_SYNCH_H
	bc_threadid_t self = thr_self();
	#elif HAVE_PTHREAD_H
	bc_threadid_t self = pthread_self();
	#else
	# error
	#endif

	internal_state_lock();

	owned = (_owner == self);
	if (owned)
	{
		if (_notify_wthreads)
		{
			#if WIN32
			if (!ReleaseSemaphore(_notify_sig, 1, 0))
				throw Error("ReleaseSemaphore failed");
			#elif HAVE_SYNCH_H
			if (cond_signal(&_notify_sig))
				throw Error("cond_signal failed");
			#elif HAVE_PTHREAD_H
			posixErrorDetector(pthread_cond_signal(&_notify_sig), "pthread_cond_signal failed");
			#else
			# error
			#endif
		}
	}

	internal_state_unlock();

	if (!owned)
		throw IllegalMonitorStateException();
}

void Object::NonfairMonitor::notifyAll()
{
	bool owned;

	#if WIN32
	bc_threadid_t self = GetCurrentThreadId();
	#elif HAVE_SYNCH_H
	bc_threadid_t self = thr_self();
	#elif HAVE_PTHREAD_H
	bc_threadid_t self = pthread_self();
	#else
	# error
	#endif

	internal_state_lock();

	owned = (_owner == self);
	if (owned)
	{
		if (_notify_wthreads)
		{
			#if WIN32
			_notify_sig_all = true;
			if (!ReleaseSemaphore(_notify_sig, _notify_wthreads, 0))
				throw Error("ReleaseSemaphore failed");
			#elif HAVE_SYNCH_H
			if (cond_broadcast(&_notify_sig))
				throw Error("cond_broadcast failed");
			#elif HAVE_PTHREAD_H
			posixErrorDetector(pthread_cond_broadcast(&_notify_sig), "pthread_cond_broadcast failed");
			#else
			# error
			#endif
		}
	}

	internal_state_unlock();

	if (!owned)
		throw IllegalMonitorStateException();
}

void Object::NonfairMonitor::wait(jlong timeout) throw (InterruptedException)
{
	bool owned, interrupted = false;

	#if WIN32
	bc_threadid_t self = GetCurrentThreadId();
	#elif HAVE_SYNCH_H
	bc_threadid_t self = thr_self();
	#elif HAVE_PTHREAD_H
	bc_threadid_t self = pthread_self();
	#else
	# error
	#endif

	internal_state_lock();

	if (_interruptee == self)
	{
		interrupted = true;
		_interruptee = 0;
	}

	if (!interrupted && (owned = (_owner == self)))
	{
		#if WIN32
		bool last_waiter;
		#endif
		unsigned int save_lock_count = _lock_count;

		_owner = 0;
		_lock_count = 0;

		if (++_notify_wthreads == 0)
			throw Error("maximum waiting threads exceeded");

		#if WIN32
		if (!ReleaseSemaphore(_lock_sig, 1, 0))
			throw Error("ReleaseSemaphore failed");
		switch (SignalObjectAndWait(_lock, _notify_sig, timeout ? timeout : INFINITE, TRUE))
		{
		case WAIT_IO_COMPLETION:
			interrupted = true;
		case WAIT_OBJECT_0:
		case WAIT_TIMEOUT:
			break;
		default:
			throw Error("SignalObjectAndWait failed");
		}
		if (WaitForSingleObject(_lock, INFINITE) != WAIT_OBJECT_0)
			throw Error("WaitForSingleObject failed");
		#else
		register int rc;
		# if HAVE_SYNCH_H
		if (cond_signal(&_lock_sig))
			throw Error("cond_signal failed");
		if (timeout)
		{
			timestruc to;

			to.tv_sec = timeout / 1000;
			to.tv_nsec = (timeout % 1000) * 1000000L;

			rc = cond_reltimedwait(&_notify_sig, &_lock, &to);
		}
		else
			rc = cond_wait(&_notify_sig, &_lock);
		# elif HAVE_PTHREAD_H
		posixErrorDetector(pthread_cond_signal(&_lock_sig), "pthread_cond_signal failed");
		if (timeout)
		{
			struct timespec to;

			timeout += timestamp();

			to.tv_sec = timeout / 1000;
			to.tv_nsec = (timeout % 1000) * 1000000L;

			rc = pthread_cond_timedwait(&_notify_sig, &_lock, &to);
		}
		else
			rc = pthread_cond_wait(&_notify_sig, &_lock);
		# else
		#  error
		# endif

		switch (rc)
		{
		case EINTR:
			interrupted = true;
		case ETIMEDOUT:
			break;
		case 0:
			if (_interruptee == self)
			{
				interrupted = true;
				_interruptee = 0;
			}
			break;
		default:
			# if HAVE_SYNCH_H
			if (timeout)
				throw Error("cond_timedwait failed");
			else
				throw Error("cond_wait failed");
			# elif HAVE_PTHREAD_H
			if (timeout)
				throw Error("pthread_cond_timedwait failed");
			else
				throw Error("pthread_cond_wait failed");
			# else
			#  error
			# endif
		}
		#endif

		if (_lock_count)
		{
			// owner cannot be self; somebody else obtained the lock; wait until it's out turn
			if (++_lock_wthreads == 0)
				throw Error("maximum waiting threads exceeded");

			do
			{
				#if WIN32
				switch (SignalObjectAndWait(_lock, _lock_sig, INFINITE, TRUE))
				{
				case WAIT_IO_COMPLETION:
					interrupted = true;
				case WAIT_OBJECT_0:
					break;
				default:
					throw Error("SignalObjectAndWait failed");
				}
				if (WaitForSingleObject(_lock, INFINITE) != WAIT_OBJECT_0)
					throw Error("WaitForSingleObject failed");
				#elif HAVE_SYNCH_H
				switch (cond_wait(&_lock_sig, &_lock))
				{
				case EINTR:
					interrupted = true;
				case 0:
					break;
				default:
					throw Error("cond_wait failed");
				}
				#elif HAVE_PTHREAD_H
				switch (pthread_cond_wait(&_lock_sig, &_lock))
				{
				case EINTR:
					interrupted = true;
				case 0:
					break;
				default:
					throw Error("pthread_cond_wait failed");
				}
				#else
				# error
				#endif
			} while (_lock_count);
			_lock_wthreads--;
		}
		_notify_wthreads--;
		_owner = self;
		_lock_count = save_lock_count;
		#if WIN32
		if (_notify_sig_all && (_notify_wthreads == 0))
			if (!SetEvent(_notify_sig_all_done))
				throw Error("SetEvent failed");
		#endif
	}

	internal_state_unlock();

	if (interrupted)
		throw InterruptedException();
}

Object::FairMonitor::waiter::waiter(bc_threadid_t owner, unsigned int lock_count) : owner(owner), lock_count(lock_count)
{
	#if WIN32
	if (!(event = CreateEvent(NULL, FALSE, FALSE, NULL)))
		throw Error("CreateEvent failed");
	#elif HAVE_THREAD_H
	if (cond_init(&event, USYNC_THREAD, 0))
		throw Error("cond_init failed");
	#elif HAVE_PTHREAD_H
	if (pthread_cond_init(&event, 0))
		throw Error("pthread_cond_init failed");
	#else
	# error
	#endif
	next = 0;
	prev = 0;
}

Object::FairMonitor::waiter::~waiter()
{
	#if WIN32
	if (!CloseHandle(event))
		throw Error("CloseHandle failed");
	#elif HAVE_THREAD_H
	if (cond_destroy(&event))
		throw Error("cond_destroy failed");
	#elif HAVE_PTHREAD_H
	if (pthread_cond_destroy(&event))
		throw Error("pthread_cond_destroy failed");
	#else
	# error
	#endif
}

Object::FairMonitor::FairMonitor()
{
	_lock_count = 0;
	_lock_head = _lock_tail = 0;
	_notify_head = _notify_tail = 0;
}

Object::FairMonitor::~FairMonitor()
{
	internal_state_lock();

	/*!\todo should a warning be given when there are still has pending locks?
	 */
	while (_lock_head)
	{
		waiter* tmp = _lock_head;
		_lock_head = _lock_head->next;
		delete tmp;
	}
	/*!\todo should a warning be given when there are still has pending waits?
	 */
	while (_notify_head)
	{
		waiter* tmp = _notify_head;
		_notify_head = _notify_head->next;
		delete tmp;
	}

	internal_state_unlock();

	#if WIN32
	if (!CloseHandle(_lock))
		throw Error("CloseHandle failed");
	#elif HAVE_SYNCH_H
	if (mutex_destroy(&_lock))
		throw Error("mutex_destroy failed");
	#elif HAVE_PTHREAD_H
	posixErrorDetector(pthread_mutex_destroy(&_lock), "pthread_mutex_destroy failed");
	#else
	# error
	#endif
}

void Object::FairMonitor::interrupt(bc_threadid_t target)
{
	internal_state_lock();

	_interruptee = target;

	if (_notify_head)
	{
		waiter* tmp = _notify_head;

		do
		{
			if (tmp->owner == target)
			{
				if (_notify_head == tmp)
				{
					if (!(_notify_head = tmp->next))
						_notify_tail = 0;
				}
				else
				{
					if ((tmp->prev->next = tmp->next))
						tmp->next->prev = tmp->prev;
					else
						_notify_tail = 0;
				}
				tmp->next = 0;

				#if WIN32
				if (!SetEvent(tmp->event))
					throw Error("SetEvent failed");
				#elif HAVE_SYNCH_H
				if (cond_signal(&tmp->event))
					throw Error("cond_signal failed");
				#elif HAVE_PTHREAD_H
				if (pthread_cond_signal(&tmp->event))
					throw Error("pthread_cond_signal failed");
				#else
				# error
				#endif
				break;
			}
			tmp = tmp->next;
		} while (tmp);
	}

	internal_state_unlock();
}

void Object::FairMonitor::lock()
{
	#if WIN32
	bc_threadid_t self = GetCurrentThreadId();
	#elif HAVE_SYNCH_H
	bc_threadid_t self = thr_self();
	#elif HAVE_PTHREAD_H
	bc_threadid_t self = pthread_self();
	#else
	# error
	#endif

	internal_state_lock();

	if (_lock_count)
	{
		if (_owner == self)
		{
			if (++_lock_count == 0)
				throw Error("maximum lock count exceeded");

			internal_state_unlock();
			return;
		}
		else
		{
			waiter* me = new waiter(self, 1);

			if (_lock_head)
			{
				me->prev = _lock_tail;
				_lock_tail = _lock_tail->next = me;
			}
			else
				_lock_head = _lock_tail = me;

			while (_owner != self)
			{
				#if WIN32
				switch (SignalObjectAndWait(_lock, me->event, INFINITE, FALSE))
				{
				case WAIT_OBJECT_0:
					break;
				default:
					throw Error("SignalObjectAndWait failed");
				}
				if (WaitForSingleObject(_lock, INFINITE) != WAIT_OBJECT_0)
					throw Error("WaitForSingleObject failed");
				#elif HAVE_SYNCH_H
				switch (cond_wait(&me->event, &_lock))
				{
				case EINTR:
				case 0:
					break;
				default:
					throw Error("cond_wait failed");
				}
				#elif HAVE_PTHREAD_H
				switch (pthread_cond_wait(&me->event, &_lock))
				{
				case EINTR:
				case 0:
					break;
				default:
					throw Error("pthread_cond_wait failed");
				}
				#else
				# error
				#endif
			}

			delete me;
		}
	}
	else
	{
		_owner = self;
		_lock_count = 1;
	}

	internal_state_unlock();
}

void Object::FairMonitor::lockInterruptibly() throw (InterruptedException)
{
	bool interrupted = false;

	#if WIN32
	bc_threadid_t self = GetCurrentThreadId();
	#elif HAVE_SYNCH_H
	bc_threadid_t self = thr_self();
	#elif HAVE_PTHREAD_H
	bc_threadid_t self = pthread_self();
	#else
	# error
	#endif

	internal_state_lock();

	if (_interruptee == self)
	{
		interrupted = true;
		_interruptee = 0;
	}

	if (!interrupted)
	{
		if (_lock_count)
		{
			if (_owner == self)
			{
				if (++_lock_count == 0)
					throw Error("maximum lock count exceeded");

				internal_state_unlock();
				return;
			}
			else
			{
				waiter* me = new waiter(self, 1);

				if (_lock_head)
				{
					me->prev = _lock_tail;
					_lock_tail = _lock_tail->next = me;
				}
				else
					_lock_head = _lock_tail = me;

				while (_owner != self)
				{
					#if WIN32
					switch (SignalObjectAndWait(_lock, me->event, INFINITE, FALSE))
					{
					case WAIT_OBJECT_0:
						break;
					default:
						throw Error("SignalObjectAndWait failed");
					}
					if (WaitForSingleObject(_lock, INFINITE) != WAIT_OBJECT_0)
						throw Error("WaitForSingleObject failed");
					#elif HAVE_SYNCH_H
					switch (cond_wait(&me->event, &_lock))
					{
					case EINTR:
						interrupted = true;
					case 0:
						break;
					default:
						throw Error("cond_wait failed");
					}
					#elif HAVE_PTHREAD_H
					switch (pthread_cond_wait(&me->event, &_lock))
					{
					case EINTR:
						interrupted = true;
					case 0:
						break;
					default:
						throw Error("pthread_cond_wait failed");
					}
					#else
					# error
					#endif
				}

				delete me;
			}
		}

		if (!interrupted)
		{
			_owner = self;
			_lock_count = 1;
		}
	}

	internal_state_unlock();

	if (interrupted)
		throw InterruptedException();
}

bool Object::FairMonitor::tryLock()
{
	bool result = false;

	#if WIN32
	bc_threadid_t self = GetCurrentThreadId();
	#elif HAVE_SYNCH_H
	bc_threadid_t self = thr_self();
	#elif HAVE_PTHREAD_H
	bc_threadid_t self = pthread_self();
	#else
	# error
	#endif

	internal_state_lock();

	if (_lock_count)
	{
		result = (_owner == self);
		if (result)
			if (++_lock_count == 0)
				throw Error("maximum lock count exceeded");
	}
	else
	{
		_lock_count = 1;
		_owner = self;
	}

	internal_state_unlock();

	return result;
}

void Object::FairMonitor::unlock()
{
	bool owned;

	#if WIN32
	bc_threadid_t self = GetCurrentThreadId();
	#elif HAVE_SYNCH_H
	bc_threadid_t self = thr_self();
	#elif HAVE_PTHREAD_H
	bc_threadid_t self = pthread_self();
	#else
	# error
	#endif

	internal_state_lock();

	owned = (_owner == self);
	if (owned)
	{
		if (!--_lock_count)
		{
			if (_lock_head)
			{
				_owner = _lock_head->owner;
				_lock_count = _lock_head->lock_count;

				#if WIN32
				if (!SetEvent(_lock_head->event))
					throw Error("SetEvent failed");
				#elif HAVE_SYNCH_H
				if (cond_signal(&_lock_head->event))
					throw Error("cond_signal failed");
				#elif HAVE_PTHREAD_H
				if (pthread_cond_signal(&_lock_head->event))
					throw Error("pthread_cond_signal failed");
				#else
				# error
				#endif

				if ((_lock_head = _lock_head->next))
					_lock_head->prev = 0;
				else
					_lock_tail = 0;
			}
			else
			{
				// no more waiters for the lock
				_owner = 0;
			}
		}
	}

	internal_state_unlock();

	if (!owned)
		throw IllegalMonitorStateException();
}

void Object::FairMonitor::notify()
{
	bool owned;

	#if WIN32
	bc_threadid_t self = GetCurrentThreadId();
	#elif HAVE_SYNCH_H
	bc_threadid_t self = thr_self();
	#elif HAVE_PTHREAD_H
	bc_threadid_t self = pthread_self();
	#else
	# error
	#endif

	internal_state_lock();

	owned = (_owner == self);
	if (owned)
	{
		if (_notify_head)
		{
			// take head from notify list, signal it, and move it from notify list to the tail of the lock list
			waiter* target = _notify_head;

			#if WIN32
			if (!SetEvent(&target->event))
				throw Error("SetEvent failed");
			#elif HAVE_SYNCH_H
			if (cond_signal(&target->event))
				throw Error("cond_signal failed");
			#elif HAVE_PTHREAD_H
			if (pthread_cond_signal(&target->event))
				throw Error("pthread_cond_signal failed");
			#else
			# error
			#endif

			if ((_notify_head = _notify_head->next))
				_notify_head->prev = 0;
			else
				_notify_tail = 0;

			if (_lock_head)
			{
				target->prev = _lock_tail;
				_lock_tail = _lock_tail->next = target;
			}
			else
				_lock_head = _lock_tail = target;
		}
	}

	internal_state_unlock();

	if (!owned)
		throw IllegalMonitorStateException();
}

void Object::FairMonitor::notifyAll()
{
	bool owned;

	#if WIN32
	bc_threadid_t self = GetCurrentThreadId();
	#elif HAVE_SYNCH_H
	bc_threadid_t self = thr_self();
	#elif HAVE_PTHREAD_H
	bc_threadid_t self = pthread_self();
	#else
	# error
	#endif

	internal_state_lock();

	owned = (_owner == self);
	if (owned)
	{
		if (_notify_head)
		{
			while (_notify_head)
			{
				waiter* target = _notify_head;

				#if WIN32
				if (!SetEvent(target->event))
					throw Error("SetEvent failed");
				#elif HAVE_SYNCH_H
				if (cond_signal(&target->event))
					throw Error("cond_signal failed");
				#elif HAVE_PTHREAD_H
				if (pthread_cond_signal(&target->event))
					throw Error("pthread_cond_signal failed");
				#else
				# error
				#endif

				if ((_notify_head = _notify_head->next))
					_notify_head->prev = 0;
				else
					_notify_tail = 0;

				if (_lock_head)
				{
					target->prev = _lock_tail;
					_lock_tail = _lock_tail->next = target;
				}
				else
					_lock_head = _lock_tail = target;
			}
		}
	}

	internal_state_unlock();

	if (!owned)
		throw IllegalMonitorStateException();
}

void Object::FairMonitor::wait(jlong timeout) throw (InterruptedException)
{
	bool owned, interrupted = false;

	#if WIN32
	bc_threadid_t self = GetCurrentThreadId();
	#elif HAVE_SYNCH_H
	bc_threadid_t self = thr_self();
	#elif HAVE_PTHREAD_H
	bc_threadid_t self = pthread_self();
	#else
	# error
	#endif

	internal_state_lock();

	if (_interruptee == self)
	{
		interrupted = true;
		_interruptee = 0;
	}

	if (!interrupted && (owned = (_owner == self)))
	{
		waiter* me = new waiter(_owner, _lock_count);

		// append to notify queue
		if (_notify_head)
		{
			me->prev = _notify_tail;
			_notify_tail = _notify_tail->next = me;
		}
		else
		{
			_notify_head = _notify_tail = me;
		}

		if (_lock_head)
		{
			// transfer ownership to the head of the lock list
			_owner = _lock_head->owner;
			_lock_count = _lock_head->lock_count;

			#if WIN32
			if (!SetEvent(_lock_head->event))
				throw Error("SetEvent failed");
			#elif HAVE_SYNCH_H
			if (cond_signal(&_lock_head->event))
				throw Error("cond_signal failed");
			#elif HAVE_PTHREAD_H
			if (pthread_cond_signal(&_lock_head->event))
				throw Error("pthread_cond_signal failed");
			#else
			# error
			#endif

			if ((_lock_head = _lock_head->next))
				_lock_head->prev = 0;
			else
				_lock_tail = 0;
		}
		else
		{
			// nobody is waiting for the lock
			_owner = 0;
			_lock_count = 0;
		}

		#if WIN32
		#else
		register int rc;
		# if HAVE_SYNCH_H
		if (timeout)
		{
			timestruc to;

			to.tv_sec = timeout / 1000;
			to.tv_nsec = (timeout % 1000) * 1000000L;

			rc = cond_reltimedwait(&me->event, &_lock, &to);
		}
		else
			rc = cond_wait(&me->event, &_lock);
		# elif HAVE_PTHREAD_H
		if (timeout)
		{
			struct timespec to;

			timeout += timestamp();

			to.tv_sec = timeout / 1000;
			to.tv_nsec = (timeout % 1000) * 1000000L;

			rc = pthread_cond_timedwait(&me->event, &_lock, &to);
		}
		else
			rc = pthread_cond_wait(&me->event, &_lock);
		# else
		#  error
		# endif

		switch (rc)
		{
		case EINTR:
			interrupted = true;
			// run on into next case
		case ETIMEDOUT:
			// event was not received, and definitely not interrupted: remove me from the notify list
			if (_notify_head == me)
			{
				if (!(_notify_head = me->next))
					_notify_tail = 0;
			}
			else
			{
				if ((me->prev->next = me->next))
					me->next->prev = me->prev;
				else
					_notify_tail = 0;
			}
			me->next = 0;
			break;
		case 0:
			if (_interruptee == self)
			{
				interrupted = true;
				_interruptee = 0;
			}
			break;
		default:
			# if HAVE_SYNCH_H
			if (timeout)
				throw Error("cond_reltimedwait failed");
			else
				throw Error("cond_wait failed");
			# elif HAVE_PTHREAD_H
			if (timeout)
				throw Error("pthread_cond_timedwait failed");
			else
				throw Error("pthread_cond_wait failed");
			# else
			#  error
			# endif
		}
		#endif

		// after receiving the signal, we simply take the ownership
		_owner = me->owner;
		_lock_count = me->lock_count;

		delete me;
	}

	internal_state_unlock();

	if (interrupted)
		throw InterruptedException();
}

Object::Synchronizer::Synchronizer(const Object* obj) : _ref(obj), _once(true)
{
	_ref->lock();
}

Object::Synchronizer::Synchronizer(const Object& obj) : _ref(&obj), _once(true)
{
	_ref->lock();
}

Object::Synchronizer::~Synchronizer()
{
	_ref->unlock();
}

bool Object::Synchronizer::checkonce()
{
	if (_once)
	{
		_once = false;
		return true;
	}
	return false;
}

bc_mutex_t Object::_init_lock = Object::init();

bc_mutex_t Object::init()
{
	bc_mutex_t tmp;

	#if WIN32
	if (!(tmp = CreateMutex(NULL, FALSE, NULL)))
		throw Error("CreateMutex failed");
	#elif HAVE_SYNCH_H
	if (mutex_init(&tmp, USYNC_THREAD, 0))
		throw Error("mutex_init failed");
	#elif HAVE_PTHREAD_H
	posixErrorDetector(pthread_mutex_init(&tmp, 0), "pthread_mutex_init failed");
	#else
	# error
	#endif

	return tmp;
}

Object::Object() : _ref_count(0), monitor(0)
{
}

Object::~Object()
{
	delete monitor;
	monitor = 0;
}

Object* Object::clone() const throw (CloneNotSupportedException)
{
	throw CloneNotSupportedException();
}

bool Object::equals(const Object* cmp) const throw ()
{
	return this == cmp;
}

jint Object::hashCode() const throw ()
{
	if (sizeof(this) == sizeof(jint))
	{
		return (jint) (unsigned long) this;
	}
	else
	{
		return (jint) ((unsigned long) this ^ ((unsigned long) this) >> 32);
	}
}

void Object::lock() const
{
	while (!monitor)
	{
		// lazy initialization of the notifier
		#if WIN32
		if (WaitForSingleObject(_init_lock, INFINITE) != WAIT_OBJECT_0)
			throw Error("WaitForSingleObject failed");
		#elif HAVE_SYNCH_H
		if (mutex_lock(&_init_lock))
			throw Error("mutex_lock failed");
		#elif HAVE_PTHREAD_H
		posixErrorDetector(pthread_mutex_lock(&_init_lock),
			"pthread_mutex_lock failed in Object::lock()");
		#else
		# error
		#endif

		if (!monitor)
		{
			monitor = Monitor::getInstance();
		}

		#if WIN32
		if (!ReleaseMutex(_init_lock))
			throw Error("ReleaseMutex failed");
		#elif HAVE_SYNCH_H
		if (mutex_unlock(&_init_lock))
			throw Error("mutex_unlock failed");
		#elif HAVE_PTHREAD_H
		posixErrorDetector(pthread_mutex_unlock(&_init_lock), "pthread_mutex_unlock failed");
		#else
		# error
		#endif
	}

	Thread* t = Thread::currentThread();

	if (t)
	{
		t->_state = Thread::BLOCKED;
		t->_monitoring = monitor;
	}

	monitor->lock();

	if (t)
	{
		t->_state = Thread::RUNNABLE;
		t->_monitoring = t->monitor;
	}
}

void Object::unlock() const
{
	if (!monitor)
		throw IllegalMonitorStateException("Object has no monitor");

	monitor->unlock();
}

void Object::notify() const
{
	if (!monitor)
		throw IllegalMonitorStateException("Object has no monitor");

	monitor->notify();
}

void Object::notifyAll() const
{
	if (!monitor)
		throw IllegalMonitorStateException("Object has no monitor");

	monitor->notifyAll();
}

void Object::wait(jlong timeout) const throw (InterruptedException)
{
	if (timeout < 0)
	{
		throw IllegalArgumentException("timeout value must be >= 0");
	}
	else
	{
		bool interrupted = false;

		if (!monitor)
			throw IllegalMonitorStateException("Object has no monitor");

		Thread* t = Thread::find(monitor->_owner); // alternatively: Thread::currentThread()

		if (t)
		{
			t->monitor->lock();
			if (t->_interrupted)
			{
				t->_interrupted = false;
				t->monitor->unlock();
				throw InterruptedException();
			}
			else
			{
				t->_state = (timeout == 0) ? Thread::WAITING : Thread::TIMED_WAITING;
				t->_monitoring = monitor;
				t->monitor->unlock();
			}
		}

		try
		{
			monitor->wait(timeout);
		}
		catch (InterruptedException)
		{
			interrupted = true;
		}

		if (t)
		{
			t->monitor->lock();
			t->_state = Thread::RUNNABLE;
			t->_monitoring = t->monitor;
			t->_interrupted = false;
			t->monitor->unlock();
		}

		if (interrupted)
			throw InterruptedException();
	}
}

String Object::toString() const throw ()
{
	return StringBuilder(typeid(*this).name()).append('@').append(Integer::toHexString(hashCode())).toString();
}


void beecrypt::lang::collection_attach(Object* obj) throw ()
{
	assert(obj != 0);
	obj->_ref_count++;
}

void beecrypt::lang::collection_detach(Object* obj) throw ()
{
	assert(obj != 0);
	obj->_ref_count--;
}

void beecrypt::lang::collection_remove(Object* obj) throw ()
{
	assert(obj != 0);
	if (--obj->_ref_count == 0)
		delete obj;
}

void beecrypt::lang::collection_rcheck(Object* obj) throw ()
{
	assert(obj != 0);
	if (obj->_ref_count == 0)
		delete obj;
}
