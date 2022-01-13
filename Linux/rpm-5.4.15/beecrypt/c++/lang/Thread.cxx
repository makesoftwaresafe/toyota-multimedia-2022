/*
 * Copyright (c) 2004 Beeyond Software Holding BV
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

#if !WIN32 && HAVE_TIME_H
# include <time.h>
#endif

#if HAVE_ERRNO_H
# include <errno.h>
#endif

#include "beecrypt/c++/lang/Thread.h"
#include "beecrypt/c++/lang/String.h"
#include "beecrypt/c++/lang/StringBuilder.h"
#include "beecrypt/c++/lang/Error.h"
#include "beecrypt/c++/lang/RuntimeException.h"

using namespace beecrypt::lang;

#ifdef HAVE_PTHREAD_H
# include "beecrypt/c++/posix.h"
#endif

Thread::thread_map Thread::_tmap;

bc_mutex_t Thread::_tmap_lock = Thread::init();

/* TODO THE MAIN THREAD CANNOT BE JOINED! */
#if WIN32
Thread Thread::_main("main", GetCurrentThreadId());
#elif HAVE_SYNCH_H
Thread Thread::_main("main", thr_self());
#elif HAVE_PTHREAD_H
Thread Thread::_main("main", pthread_self());
#else
# error
#endif

#if WIN32
DWORD Thread::start_routine(void* args)
#else
void* Thread::start_routine(void* args)
#endif
{
	register Thread* t = (Thread*) args;

	#if WIN32
	if (WaitForSingleObject(_tmap_lock, INFINITE) != WAIT_OBJECT_0)
		throw RuntimeException("WaitForSingleObject failed");
	#elif HAVE_SYNCH_H
	if (mutex_lock(&_tmap_lock))
		throw RuntimeException("mutex_lock failed");
	#elif HAVE_PTHREAD_H
	posixErrorDetector(pthread_mutex_lock(&_tmap_lock), "pthread_mutex_lock failed in Thread::start_routine()");
	#else
	# error
	#endif

	_tmap[t->_tid] = t;

	#if WIN32
	if (!ReleaseMutex(_tmap_lock))
		throw RuntimeException("ReleaseMutex failed");
	#elif HAVE_SYNCH_H
	if (mutex_unlock(&_tmap_lock))
		throw RuntimeException("mutex_unlock failed");
	#elif HAVE_PTHREAD_H
	posixErrorDetector(pthread_mutex_unlock(&_tmap_lock), "pthread_mutex_unlock failed");
	#else
	# error
	#endif

	t->_target->run();

	if (!t->_daemon)
	{
		t->monitor->lock();
		t->_state = Thread::TERMINATED;
		t->_joiner->lock();
		t->_joiner->notifyAll();
		t->_joiner->unlock();
		t->monitor->unlock();
	}

	return 0;
}

bc_mutex_t Thread::init()
{
	bc_mutex_t lock;

	#if WIN32
	if (!(lock = CreateMutex(NULL, FALSE, NULL)))
		throw RuntimeException("CreateMutex failed");
	#elif HAVE_SYNCH_H
	if (mutex_init(&lock, USYNC_THREAD, 0))
		throw RuntimeException("mutex_init failed");
	#elif HAVE_PTHREAD_H
	posixErrorDetector(pthread_mutex_init(&lock, 0), "pthread_mutex_init failed");
	#else
	# error
	#endif

	_tmap[_main._tid] = &_main;

	return lock;
}

Thread* Thread::find(bc_threadid_t id)
{
	Thread* result = 0;

	#if WIN32
	if (WaitForSingleObject(_tmap_lock, INFINITE) != WAIT_OBJECT_0)
		throw RuntimeException("WaitForSingleObject failed");
	#elif HAVE_SYNCH_H
	if (mutex_lock(&_tmap_lock))
		throw RuntimeException("mutex_lock failed");
	#elif HAVE_PTHREAD_H
	posixErrorDetector(pthread_mutex_lock(&_tmap_lock), "pthread_mutex_lock failed in Thread::find(bc_threadid_t)");
	#else
	# error
	#endif

	thread_map_iterator it = _tmap.find(id);
	if (it != _tmap.end())
		result = it->second;

	#if WIN32
	if (!ReleaseMutex(_tmap_lock))
		throw RuntimeException("ReleaseMutex failed");
	#elif HAVE_SYNCH_H
	if (mutex_unlock(&_tmap_lock))
		throw RuntimeException("mutex_unlock failed");
	#elif HAVE_PTHREAD_H
	posixErrorDetector(pthread_mutex_unlock(&_tmap_lock), "pthread_mutex_unlock failed");
	#else
	# error
	#endif

	return result;
}

Thread* Thread::currentThread()
{
	#if WIN32
	return find(GetCurrentThreadId());
	#elif HAVE_SYNCH_H
	return find(thr_self());
	#elif HAVE_PTHREAD_H
	return find(pthread_self());
	#else
	# error
	#endif
}

void Thread::sleep(jlong millis) throw (InterruptedException)
{
	Object sleeper;

	synchronized (sleeper)
	{
		sleeper.wait(millis);
	}
}

void Thread::yield()
{
	#if WIN32
	SwitchToThread();
	#elif HAVE_THREAD_H
	thr_yield();
	#elif HAVE_SCHED_H
	sched_yield();
	#endif
}

Thread::Thread(const String& name, bc_threadid_t id) : _name(name)
{
	_target = this;
	_stacksize = 0;
	_daemon = true;
	_interrupted = false;
	_state = Thread::RUNNABLE;
	_monitoring = monitor = Monitor::getInstance();
	_joiner = 0;
}

Thread::Thread()
{
	_target = this;
	_stacksize = 0;
	_daemon = false;
	_interrupted = false;
	_state = Thread::NEW;
	_monitoring = monitor = Monitor::getInstance();
	_joiner = Monitor::getInstance();
}

Thread::Thread(const String& name) : _name(name)
{
	_target = this;
	_stacksize = 0;
	_daemon = false;
	_interrupted = false;
	_state = Thread::NEW;
	_monitoring = monitor = Monitor::getInstance();
	_joiner = Monitor::getInstance();
}

Thread::Thread(Runnable& target) : _target(&target)
{
	_stacksize = 0;
	_daemon = false;
	_interrupted = false;
	_state = Thread::NEW;
	_monitoring = monitor = Monitor::getInstance();
	_joiner = Monitor::getInstance();
}

Thread::Thread(Runnable& target, const String& name) : _target(&target), _name(name)
{
	_stacksize = 0;
	_daemon = false;
	_interrupted = false;
	_state = Thread::NEW;
	_monitoring = monitor = Monitor::getInstance();
	_joiner = Monitor::getInstance();
}

Thread::Thread(Runnable& target, const String& name, size_t stacksize) : _target(&target), _name(name)
{
	_stacksize = stacksize;
	_daemon = false;
	_interrupted = false;
	_state = Thread::NEW;
	_monitoring = monitor = Monitor::getInstance();
	_joiner = Monitor::getInstance();
}

Thread::~Thread()
{
	/* The main Thread can't have a joiner */
	if (_joiner)
	{
		if (!_daemon)
		{
			if (_state != Thread::NEW)
			{
				join();

				#if WIN32
				if (WaitForSingleObject(_tid, INFINITE) != WAIT_OBJECT_0)
					throw RuntimeException("WaitForSingleObject failed");
				if (!CloseHandle(_tid))
					throw RuntimeException("CloseHandle failed");
				#elif HAVE_THREAD_H
				if (thr_join(_tid, 0, 0))
					throw RuntimeException("thr_join failed");
				#elif HAVE_PTHREAD_H
				posixErrorDetector(pthread_join(_tid, 0), "pthread_join failed");
				#endif
			}

			#if WIN32
			if (WaitForSingleObject(_tmap_lock, INFINITE) != WAIT_OBJECT_0)
				throw RuntimeException("WaitForSingleObject failed");
			#elif HAVE_SYNCH_H
			if (mutex_lock(&_tmap_lock))
				throw RuntimeException("mutex_lock failed");
			#elif HAVE_PTHREAD_H
			posixErrorDetector(pthread_mutex_lock(&_tmap_lock), "pthread_mutex_lock failed in Thread::~Thread()");
			#else
			# error
			#endif

			_tmap.erase(_tid);

			#if WIN32
			if (!ReleaseMutex(_tmap_lock))
				throw RuntimeException("ReleaseMutex failed");
			#elif HAVE_SYNCH_H
			if (mutex_unlock(&_tmap_lock))
				throw RuntimeException("mutex_unlock failed");
			#elif HAVE_PTHREAD_H
			posixErrorDetector(pthread_mutex_unlock(&_tmap_lock), "pthread_mutex_unlock failed");
			#else
			# error
			#endif
		}

		delete _joiner;
		_joiner = 0;
	}
}

void Thread::start() throw (IllegalThreadStateException)
{
	monitor->lock();
	if (_state != Thread::NEW)
	{
		monitor->unlock();
		throw IllegalThreadStateException("Thread was already started");
	}

	#if WIN32
	if (!(_thr = CreateThread(NULL, _stacksize, (LPTHREAD_START_ROUTINE) start_routine, this, 0, &_tid)))
		throw RuntimeException("CreateThread failed");
	#else
	# if HAVE_THREAD_H
	if (thr_create(0, _stacksize, start_routine, this, _daemon ? THR_DAEMON : 0, &_tid))
		throw RuntimeException("thr_create failed");
	# elif HAVE_PTHREAD_H
	if (_stacksize || _daemon)
	{
		pthread_attr_t attr;

		posixErrorDetector(pthread_attr_init(&attr), "pthread_attr_init failed");

		if (_stacksize)
			posixErrorDetector(pthread_attr_setstacksize(&attr, _stacksize),
				"pthread_attr_setstacksize failed");

		if (_daemon)
			posixErrorDetector(pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED),
				"pthread_attr_setdetachstate failed");

		posixErrorDetector(pthread_create(&_tid, &attr, start_routine, this),
			"pthread_create failed");

		posixErrorDetector(pthread_attr_destroy(&attr),
			"pthread_attr_destroy failed");
	}
	else
	{
		posixErrorDetector(pthread_create(&_tid, 0, start_routine, this),
			"pthread_create failed");
	}
	# else
	#  error
	# endif
	_thr = _tid;
	#endif

	_state = Thread::RUNNABLE;

	monitor->unlock();
}

const String& Thread::getName() const throw ()
{
	return _name;
}

Thread::State Thread::getState() const throw ()
{
	return _state;
}

void Thread::interrupt()
{
	monitor->lock();

	if (_state == Thread::WAITING || _state == Thread::TIMED_WAITING)
		_monitoring->interrupt(_tid);
	else
		_interrupted = true;

	monitor->unlock();
}

bool Thread::interrupted()
{
	if (_monitoring->interrupted(_tid) || _interrupted)
	{
		_interrupted = false;
		return true;
	}
	return false;
}

bool Thread::isAlive() const throw ()
{
	switch (_state)
	{
	case Thread::NEW:
	case Thread::TERMINATED:
		return false;
	default:
		return true;
	}
}

bool Thread::isDaemon() const throw ()
{
	return _daemon;
}

bool Thread::isInterrupted() const throw ()
{
	return _interrupted || _monitoring->isInterrupted(_tid);
}

void Thread::join() throw (InterruptedException)
{
	if (_joiner == 0)
	{
		throw Error("You cannot join the main thread");
	}

	monitor->lock();
	if (_state != Thread::NEW)
	{
		while (_state != Thread::TERMINATED)
		{
			monitor->unlock();
			_joiner->lock();
			_joiner->wait(0);
			_joiner->unlock();
			monitor->lock();
		}
	}
	monitor->unlock();
}

void Thread::run()
{
}

void Thread::setDaemon(bool on) throw (IllegalThreadStateException)
{
	if (_state != NEW)
		throw IllegalThreadStateException("Thread was already started");

	_daemon = on;
}

String Thread::toString() const throw ()
{
	return StringBuilder("Thread[").append(getName()).append("]").toString();
}
