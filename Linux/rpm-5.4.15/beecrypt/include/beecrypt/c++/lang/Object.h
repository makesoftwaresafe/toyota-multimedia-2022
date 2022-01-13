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

/*!\file Object.h
 * \ingroup CXX_LANG_m
 *
 * Credit for the technique of emulating POSIX condition variables
 * on Win32 goes to Douglas C. Schmidt and Irfan Pyarali.
 *
 * \see http://www.cs.wustl.edu/~schmidt/win32-cv-1.html
 */

#ifndef _CLASS_BEE_LANG_OBJECT_H
#define _CLASS_BEE_LANG_OBJECT_H

#include "beecrypt/c++/lang/CloneNotSupportedException.h"
using beecrypt::lang::CloneNotSupportedException;
#include "beecrypt/c++/lang/InterruptedException.h"
using beecrypt::lang::InterruptedException;
#include "beecrypt/c++/lang/IllegalMonitorStateException.h"
using beecrypt::lang::IllegalMonitorStateException;
#include "beecrypt/c++/lang/RuntimeException.h"
using beecrypt::lang::RuntimeException;

#ifdef __cplusplus

namespace beecrypt {
	namespace util {
		namespace concurrent {
			namespace locks {
				class BEECRYPTCXXAPI ReentrantLock;
			}
		}
	}
	namespace lang {
		/*!\ingroup CXX_LANG_m
		 */
		class BEECRYPTCXXAPI Object
		{
			friend class Thread;
			friend class beecrypt::util::concurrent::locks::ReentrantLock;

			friend BEECRYPTCXXAPI void collection_attach(Object*) throw ();
			friend BEECRYPTCXXAPI void collection_detach(Object*) throw ();
			friend BEECRYPTCXXAPI void collection_remove(Object*) throw ();
			friend BEECRYPTCXXAPI void collection_rcheck(Object*) throw ();

		protected:
			/*!\brief This class is used to emulate Java's lock/wait/notify
			 *  methods.
			 */
			class Monitor
			{
				friend class Object;

			protected:
				bc_threadid_t         _owner;
				bc_threadid_t         _interruptee;
				bc_mutex_t            _lock;
				volatile unsigned int _lock_count;

				void internal_state_lock();
				void internal_state_unlock();

				Monitor();

			public:
				virtual ~Monitor() {}

				virtual void interrupt(bc_threadid_t) = 0;
				virtual bool interrupted(bc_threadid_t);
				virtual bool isInterrupted(bc_threadid_t);
				virtual bool isLocked();
				virtual void lock() = 0;
				virtual void lockInterruptibly() throw (InterruptedException) = 0;
				virtual bool tryLock() = 0;
				virtual void notify() = 0;
				virtual void notifyAll() = 0;
				virtual void unlock() = 0;
				virtual void wait(jlong timeout) throw (InterruptedException) = 0;

				static Monitor* getInstance(bool fair = false);
			};

		private:
			/*!\brief A Non-fair Monitor
			 */
			class NonfairMonitor : public Monitor
			{
			private:
				#if WIN32
				HANDLE        _lock_sig; // semaphore
				bool          _lock_sig_all;
				HANDLE        _lock_sig_all_done; // event
				unsigned int  _lock_wthreads;
				HANDLE        _notify_sig; // semaphore
				bool          _notify_sig_all;
				HANDLE        _notify_sig_all_done; // event
				unsigned int  _notify_wthreads;
				#else
				bc_cond_t     _lock_sig;
				unsigned int  _lock_wthreads;
				bc_cond_t     _notify_sig;
				unsigned int  _notify_wthreads;
				#endif

			public:
				NonfairMonitor();
				virtual ~NonfairMonitor();

				virtual void interrupt(bc_threadid_t);
				virtual void lock();
				virtual void lockInterruptibly() throw (InterruptedException);
				virtual bool tryLock();
				virtual void unlock();
				virtual void notify();
				virtual void notifyAll();
				virtual void wait(jlong timeout) throw (InterruptedException);
			};

			class FairMonitor : public Monitor
			{
			private:
				struct waiter
				{
					bc_threadid_t owner;
					unsigned int  lock_count;
					bc_cond_t     event;
					waiter*       next;
					waiter*       prev;

					waiter(bc_threadid_t owner, unsigned int lock_count);
					~waiter();
				};

				waiter* _lock_head;
				waiter* _lock_tail;
				waiter* _notify_head;
				waiter* _notify_tail;

			public:
				FairMonitor();
				virtual ~FairMonitor();

				virtual void interrupt(bc_threadid_t);
				virtual void lock();
				virtual void lockInterruptibly() throw (InterruptedException);
				virtual bool tryLock();
				virtual void unlock();
				virtual void notify();
				virtual void notifyAll();
				virtual void wait(jlong timeout) throw (InterruptedException);
			};

		private:
			/*!\brief The object's reference count (for use with Collections).
			 *  When this value is zero, it's safe to delete it.
			 */
			volatile unsigned int _ref_count;

		private:
			/*!\brief The object's monitor.
			 *  Initialized to zero, and only allocated when an Object's
			 *  is first synchronized. Classes which rely heavily on
			 *  synchronization should initialize this member in their
			 *  constructor with the Object::Monitor::getInstance()
			 *  method.
			 * \see Object::Monitor::getInstance(bool fair = false)
			 */
			mutable Monitor* monitor;

		public:
			/*!\brief This class is used to emulate Java's 'synchronized'
			 *        methods.
			 */
			class BEECRYPTCXXAPI Synchronizer
			{
			private:
				const Object* _ref;
				bool _once;

			public:
				Synchronizer(const Object* obj);
				Synchronizer(const Object& obj);
				~Synchronizer();

				bool checkonce();
			};

			/*!\def synchronized
			 * \brief Macro used to emulate Java's synchronization primitive.
			 */
			#define synchronized(obj) for (Object::Synchronizer _s(obj); _s.checkonce(); )

		private:
			static bc_mutex_t _init_lock;
			static bc_mutex_t init();

		private:
			void lock() const;
			void unlock() const;

		protected:
			virtual Object* clone() const throw (CloneNotSupportedException);

		public:
			Object();
			virtual ~Object();

			virtual bool equals(const Object* obj) const throw ();
			virtual jint hashCode() const throw ();
			void notify() const;
			void notifyAll() const;
			virtual String toString() const throw ();
			void wait(jlong millis = 0) const throw (InterruptedException);
		};

		/*!\brief This function is used inside a collection to indicate that
		 *  the object is now attached to it.
		 */
		BEECRYPTCXXAPI
		void collection_attach(Object*) throw ();
		/*!\brief This function is used inside a collection to indicate that
		 *  the object has been detached to it.
		 */
		BEECRYPTCXXAPI
		void collection_detach(Object*) throw ();
		/*!\brief This function is used inside a collection to detach the
		 *  object, and delete it if no longer attached to any other
		 *  collections.
		 */
		BEECRYPTCXXAPI
		void collection_remove(Object*) throw ();
		/*!\brief This function checks if an object needs to be recycled:
		 *  the object is deleted if no longer attached to any collection.
		 */
		BEECRYPTCXXAPI
		void collection_rcheck(Object*) throw ();
	}
}

#endif

#endif
