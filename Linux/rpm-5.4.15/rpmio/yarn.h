/* yarn.h -- generic interface for thread operations
 * Copyright (C) 2008 Mark Adler
 * Version 1.1  26 Oct 2008  Mark Adler
 */
#ifndef	_H_YARN_
#define	_H_YARN_

/*
  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the author be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.

  Mark Adler
  madler@alumni.caltech.edu
 */

/* Basic thread operations

   This interface isolates the local operating system implementation of threads
   from the application in order to facilitate platform independent use of
   threads.  All of the implementation details are deliberately hidden.

   Assuming adequate system resources and proper use, none of these functions
   can fail.  As a result, any errors encountered will cause an exit() to be
   executed.

   These functions allow the simple launching and joining of threads, and the
   locking of objects and synchronization of changes of objects.  The latter is
   implemented with a single lock type that contains an integer value.  The
   value can be ignored for simple exclusive access to an object, or the value
   can be used to signal and wait for changes to an object.

   -- Arguments --

   thread *thread;          identifier for launched thread, used by join
   void probe(void *);      pointer to function "probe", run when thread starts
   void *payload;           single argument passed to the probe function
   yarnLock lock;           a lock with a value -- used for exclusive access to
                            an object and to synchronize threads waiting for
                            changes to an object
   long val;                value to set lock, increment lock, or wait for
   int n;                   number of threads joined

   -- Thread functions --

   thread = yarnLaunch(probe, payload) - launch a thread -- exit via probe() return
   yarnJoin(thread) - join a thread and by joining end it, waiting for the thread
        to exit if it hasn't already -- will free the resources allocated by
        yarnLaunch() (don't try to join the same thread more than once)
   n = yarnJoinAll() - join all threads launched by yarnLaunch() that are not joined
        yet and free the resources allocated by the launches, usually to clean
        up when the thread processing is done -- yarnJoinAll() returns an int with
        the count of the number of threads joined (yarnJoinAll() should only be
        called from the main thread, and should only be called after any calls
        of yarnJoin() have completed)
   yarnDestruct(thread) - terminate the thread in mid-execution and join it
        (depending on the implementation, the termination may not be immediate,
        but may wait for the thread to execute certain thread or file i/o
        operations)

   -- Lock functions --

   lock = yarnNewLock(val) - create a new lock with initial value val (lock is
        created in the released state)
   yarnPossess(lock) - acquire exclusive possession of a lock, waiting if necessary
   yarnTwist(lock, [TO | BY], val) - set lock to or increment lock by val, signal
        all threads waiting on this lock and then release the lock -- must
        possess the lock before calling (twist releases, so don't do a
        yarnRelease() after a yarnTwist() on the same lock)
   yarnWaitFor(lock, [TO_BE | NOT_TO_BE | TO_BE_MORE_THAN | TO_BE_LESS_THAN], val)
        - wait on lock value to be, not to be, be greater than, or be less than
        val -- must possess the lock before calling, will possess the lock on
        return but the lock is released while waiting to permit other threads
        to use yarnTwist() to change the value and signal the change (so make sure
        that the object is in a usable state when waiting)
   yarnRelease(lock) - release a possessed lock (do not try to release a lock that
        the current thread does not possess)
   val = yarnPeekLock(lock) - return the value of the lock (assumes that lock is
        already possessed, no possess or release is done by yarnPeekLock())
   yarnFreeLock(lock) - free the resources allocated by yarnNewLock() (application
        must assure that the lock is released before calling yarnFreeLock())

   -- Memory allocation ---

   yarnMem(better_malloc, better_free) - set the memory allocation and free
        routines for use by the yarn routines where the supplied routines have
        the same interface and operation as malloc() and free(), and may be
        provided in order to supply thread-safe memory allocation routines or
        for any other reason -- by default malloc() and free() will be used

   -- Error control --

   yarnPrefix - a char pointer to a string that will be the prefix for any error
        messages that these routines generate before exiting -- if not changed
        by the application, "yarn" will be used
   yarnAbort - an external function that will be executed when there is an
        internal yarn error, due to out of memory or misuse -- this function
        may exit to abort the application, or if it returns, the yarn error
        handler will exit (set to NULL by default for no action)
 */

#ifdef __cplusplus
extern "C" {
#endif

/*@unchecked@*/ /*@observer@*/
extern const char *yarnPrefix;
/*@-redecl@*/
/*@mayexit@*/
extern void (*yarnAbort)(int)
	/*@globals internalState @*/
	/*@modifies internalState @*/;
/*@=redecl@*/

/*@-globuse@*/
void yarnMem(void *(*)(size_t), void (*)(void *))
	/*@globals internalState @*/
	/*@modifies internalState @*/;
/*@=globuse@*/

typedef struct yarnThread_s * yarnThread;
/*@only@*/
yarnThread yarnLaunchStack(void (*probe)(void *), void *payload,
		/*@null@*/ void * stack, size_t nstack)
	/*@globals fileSystem, internalState @*/
	/*@modifies fileSystem, internalState @*/;
/*@only@*/
yarnThread yarnLaunch(void (*probe)(void *), void *payload)
	/*@globals fileSystem, internalState @*/
	/*@modifies fileSystem, internalState @*/;
/*@only@*/ /*@null@*/
yarnThread yarnJoin(/*@only@*/ yarnThread ally)
	/*@globals fileSystem, internalState @*/
	/*@modifies ally, fileSystem, internalState @*/;
int yarnJoinAll(void)
	/*@globals fileSystem, internalState @*/
	/*@modifies fileSystem, internalState @*/;
void yarnDestruct(/*@only@*/ yarnThread off_course)
	/*@globals fileSystem, internalState @*/
	/*@modifies off_course, fileSystem, internalState @*/;

yarnLock yarnNewLock(long)
	/*@globals fileSystem, internalState @*/
	/*@modifies fileSystem, internalState @*/;
void yarnPossess(yarnLock bolt)
	/*@globals fileSystem, internalState @*/
	/*@modifies bolt, fileSystem, internalState @*/;
void yarnRelease(yarnLock bolt)
	/*@globals fileSystem, internalState @*/
	/*@modifies bolt, fileSystem, internalState @*/;
typedef enum yarnTwistOP_e { TO, BY } yarnTwistOP;
void yarnTwist(yarnLock bolt, yarnTwistOP op, long)
	/*@globals fileSystem, internalState @*/
	/*@modifies bolt, fileSystem, internalState @*/;
typedef enum yarnWaitOP_e {
    TO_BE, /* or */ NOT_TO_BE, /* that is the question */
    TO_BE_MORE_THAN, TO_BE_LESS_THAN } yarnWaitOP;
void yarnWaitFor(yarnLock bolt, yarnWaitOP op, long)
	/*@globals fileSystem, internalState @*/
	/*@modifies bolt, fileSystem, internalState @*/;
long yarnPeekLock(yarnLock bolt)
	/*@*/;
/*@only@*/ /*@null@*/
yarnLock yarnFreeLock(/*@only@*/ yarnLock bolt)
	/*@globals fileSystem, internalState @*/
	/*@modifies bolt, fileSystem, internalState @*/;

#ifdef __cplusplus
}
#endif

#endif	/* _H_YARN_ */
