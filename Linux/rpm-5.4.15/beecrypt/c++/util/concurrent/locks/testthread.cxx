/*
 * Copyright (c) 2005 Beeyond Software Holding BV
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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "beecrypt/c++/lang/Thread.h"
using beecrypt::lang::Thread;
#include "beecrypt/c++/util/concurrent/locks/ReentrantLock.h"
using beecrypt::util::concurrent::locks::ReentrantLock;

#include <iostream>
using namespace std;

class Worker : public Thread
{
private:
	size_t _count;

protected:
	ReentrantLock lock;
	Condition* cond;

public:
	Worker(const char* ident) : Thread(ident), _count(0)
	{
		cond = lock.newCondition();
	}

	~Worker()
	{
		delete cond;
	}
	
	virtual void run()
	{
		cout << "Worker[" << getName() << "] starting" << endl;
		try
		{
			while (!isInterrupted())
			{
				pop();
				cout << "Worker[" << getName() << "] popped" << endl;
			}
		}
		catch (InterruptedException)
		{
			cout << "Worker[" << getName() << "] interrupted" << endl;
		}
		cout << "Worker[" << getName() << "] ending" << endl;
	}

	void pop() throw (InterruptedException)
	{
		lock.lock();

		try
		{
			if (_count == 0)
				cond->await();
		
			_count--;

			lock.unlock();
		}
		catch (...)
		{
			lock.unlock();
			throw;
		}
	}

	void push()
	{
		lock.lock();

		_count++;

		cond->signal();

		lock.unlock();
	}
};

int main(int argc, char* argv[])
{
	try
	{
		Worker a("a");
		Worker b("b");

		a.start();
		a.sleep(100);
		b.start();
		b.sleep(100);
		a.push();
		a.push();
		b.push();
		a.push();
		a.yield();
		b.interrupt();
		a.push();
		a.yield();
		a.interrupt();
		a.yield();
		b.join();
		a.join();
	}
	catch (Exception& e)
	{
		std::cout << "Exception:" << e.getMessage() << std::endl;
	}
}
