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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "beecrypt/c++/lang/Thread.h"
using beecrypt::lang::Thread;

#include <iostream>
using namespace std;

class Worker : public Thread
{
private:
	size_t _count;

public:
	Worker(const char* ident) : Thread(ident)
	{
		_count = 0;
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

	void pop()
	{
		synchronized (this)
		{
			if (_count == 0)
				wait();

			_count--;
		}
	}

	void push()
	{
		synchronized (this)
		{
			_count++;

			notify();
		}
	}
};

int main(int argc, char* argv[])
{
	Worker a("one");
	Worker b("two");

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
}
