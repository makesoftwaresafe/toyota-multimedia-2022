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

/*!\file Thread.h
 * \ingroup CXX_LANG_m
 */

#ifndef _CLASS_BEE_LANG_THREAD_H
#define _CLASS_BEE_LANG_THREAD_H

#ifdef __cplusplus

#include "beecrypt/c++/lang/String.h"
using beecrypt::lang::String;
#include "beecrypt/c++/lang/Runnable.h"
using beecrypt::lang::Runnable;
#include "beecrypt/c++/lang/IllegalThreadStateException.h"
using beecrypt::lang::IllegalThreadStateException;

#include <map>
using std::map;

namespace beecrypt {
	namespace lang {
		/*!\ingroup CXX_LANG_m
		 */
		class BEECRYPTCXXAPI Thread : public Object, public virtual Runnable
		{
			friend class Object;
			friend class beecrypt::util::concurrent::locks::ReentrantLock;

			friend void MonitorEnter(const Object*);
			friend void MonitorExit(const Object*) throw (IllegalMonitorStateException);

		public:
			/*!\ingroup CXX_LANG_m
			 */
			enum BEECRYPTCXXAPI State
			{
				NEW,
				RUNNABLE,
				BLOCKED,
				WAITING,
				TIMED_WAITING,
				TERMINATED
			};

		private:
			Runnable* _target;
			String    _name;

			bc_thread_t   _thr;
			bc_threadid_t _tid;
			size_t        _stacksize;
			bool          _daemon;
			bool          _interrupted;

			State         _state;
			Monitor*      _monitoring;
			Monitor*      _joiner;

			#if WIN32
			static DWORD WINAPI start_routine(void*);
			#else
			static void* start_routine(void*);
			#endif

			void terminate();

		private:
			typedef map<bc_threadid_t,Thread*> thread_map;
			typedef map<bc_threadid_t,Thread*>::iterator thread_map_iterator;

			static thread_map _tmap;
			static bc_mutex_t _tmap_lock;
			static bc_mutex_t init();

			static Thread _main;

			static Thread* find(bc_threadid_t);

		public:
			static void sleep(jlong millis) throw (InterruptedException);
			static void yield();

			static Thread* currentThread();

		private:
			Thread(const String&, bc_threadid_t);

		public:
			Thread();
			Thread(const String& name);
			Thread(Runnable& target);
			Thread(Runnable& target, const String& name);
			Thread(Runnable& target, const String& name, size_t stacksize);
			virtual ~Thread();

			virtual void run();
			virtual String toString() const throw ();

			const String& getName() const throw ();
			Thread::State getState() const throw ();
			void interrupt();
			bool interrupted();
			bool isAlive() const throw ();
			bool isDaemon() const throw ();
			bool isInterrupted() const throw ();
			void join() throw (InterruptedException);
			void setDaemon(bool on) throw (IllegalThreadStateException);
			void start() throw (IllegalThreadStateException);
		};
	}
}

#endif

#endif
