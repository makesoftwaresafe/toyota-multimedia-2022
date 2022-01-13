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

#include "beecrypt/c++/lang/Thread.h"
using beecrypt::lang::Thread;

#include "beecrypt/c++/util/concurrent/locks/ReentrantLock.h"

using namespace beecrypt::util::concurrent::locks;

ReentrantLock::Cond::Cond(Object* lock) : lock(lock)
{
}

void ReentrantLock::Cond::await() throw (InterruptedException)
{
	lock->wait(0);
}

void ReentrantLock::Cond::awaitUninterruptibly()
{
	while (true)
	{
		try
		{
			lock->wait(0);

			return;
		}
		catch (InterruptedException&)
		{
		}
	}
}

void ReentrantLock::Cond::signal()
{
	lock->notify();
}

void ReentrantLock::Cond::signalAll()
{
	lock->notifyAll();
}

ReentrantLock::ReentrantLock(bool fair)
{
	monitor = Monitor::getInstance(fair);
}

void ReentrantLock::lock()
{
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

void ReentrantLock::lockInterruptibly() throw (InterruptedException)
{
	bool interrupted = false;

	Thread* t = Thread::currentThread();

	if (t)
	{
		t->_state = Thread::BLOCKED;
		t->_monitoring = monitor;
	}

	try
	{
		monitor->lockInterruptibly();
	}
	catch (InterruptedException&)
	{
		interrupted = true;
	}

	if (t)
	{
		t->_state = Thread::RUNNABLE;
		t->_monitoring = t->monitor;
	}

	if (interrupted)
		throw InterruptedException();
}

Condition* ReentrantLock::newCondition()
{
	return new Cond(this);
}

bool ReentrantLock::tryLock()
{
	bool result;

	Thread* t = Thread::currentThread();

	if (t)
	{
		t->_state = Thread::BLOCKED;
		t->_monitoring = monitor;
	}

	result = monitor->tryLock();

	if (t)
	{
		t->_state = Thread::RUNNABLE;
		t->_monitoring = t->monitor;
	}

	return result;
}

void ReentrantLock::unlock()
{
	monitor->unlock();
}
