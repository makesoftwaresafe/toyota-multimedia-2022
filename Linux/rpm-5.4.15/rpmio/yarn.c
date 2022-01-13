/* yarn.c -- generic thread operations implemented using pthread functions
 * Copyright (C) 2008 Mark Adler
 * Version 1.1  26 Oct 2008  Mark Adler
 * For conditions of distribution and use, see copyright notice in yarn.h
 */

/* Basic thread operations implemented using the POSIX pthread library.  All
   pthread references are isolated within this module to allow alternate
   implementations with other thread libraries.  See yarn.h for the description
   of these operations. */

/* Version history:
   1.0    19 Oct 2008  First version
   1.1    26 Oct 2008  No need to set the stack size -- remove
                       Add yarn_abort() function for clean-up on error exit
 */

#include "system.h"

#include <rpmiotypes.h>

/* for thread portability */
#if defined(WITH_PTHREADS)
#if !defined(_POSIX_PTHREAD_SEMANTICS)
#define _POSIX_PTHREAD_SEMANTICS
#endif
#if !defined(_REENTRANT)
#define _REENTRANT
#endif
#else	/* WITH_PTHREADS */
#undef	_POSIX_PTHREAD_SEMANTICS
#undef	_REENTRANT
#endif	/* WITH_PTHREADS */

/* external libraries and entities referenced */
#include <stdio.h>      /* fprintf(), stderr */
#include <stdlib.h>     /* exit(), malloc(), free(), NULL */

#if defined(WITH_PTHREADS)

/*@-incondefs@*/
#include <pthread.h>    /* pthread_t, pthread_create(), pthread_join(), */
    /* pthread_attr_t, pthread_attr_init(), pthread_attr_destroy(),
       PTHREAD_CREATE_JOINABLE, pthread_attr_setdetachstate(),
       pthread_self(), pthread_equal(),
       pthread_mutex_t, PTHREAD_MUTEX_INITIALIZER, pthread_mutex_init(),
       pthread_mutex_lock(), pthread_mutex_unlock(), pthread_mutex_destroy(),
       pthread_cond_t, PTHREAD_COND_INITIALIZER, pthread_cond_init(),
       pthread_cond_broadcast(), pthread_cond_wait(), pthread_cond_destroy() */
/*@=incondefs@*/

#else	/* WITH_PTHREADS */

#define	pthread_t	int
#define	pthread_self()	0
#define	pthread_equal(_t1, _t2)	((_t1) == (_t2))
#define	pthread_create(__newthread, __attr, __start_routine, arg)	(EINVAL)
#define	pthread_join(__thread, __value_ptr)				(EINVAL)
#define	pthread_cancel(__th)						(EINVAL)
#define	pthread_cleanup_pop(__execute)
#define	pthread_cleanup_push(__routine, __arg)

#define	pthread_attr_t	int
#define	pthread_attr_init(__attr)				(EINVAL)
#define	pthread_attr_destroy(__attr)				(EINVAL)
#define	pthread_attr_setdetachstate(__attr, __detachstate)	(EINVAL)

#define	pthread_mutex_t	int
#define	PTHREAD_MUTEX_INITIALIZER	0
#define	PTHREAD_CREATE_JOINABLE		0
#define	pthread_mutex_destroy(__mutex)		(0)	/* FreeLock */
#define	pthread_mutex_init(__mutex, __attr)	(0)	/* NewLock */
#define	pthread_mutex_lock(__mutex)		(0)	/* Possess */
#define	pthread_mutex_unlock(__mutex)		(0)	/* Twist/Release */

#define	pthread_cond_t	int
#define	PTHREAD_COND_INITIALIZER	0
#define	pthread_cond_destroy(__cond)		(0)	/* FreeLock */
#define	pthread_cond_init(__cond, __attr)	(0)	/* NewLock */
#define	pthread_cond_wait(__cond, __mutex)	(EINVAL)/* WaitFor */
#define	pthread_cond_broadcast(__cond)		(0)	/* Twist */

#endif	/* WITH_PTHREADS */

#include <errno.h>      /* ENOMEM, EAGAIN, EINVAL */

#if defined(__LCLINT__)
/*@-incondefs -protoparammatch -redecl @*/
/*@-exportheader@*/
#ifdef	NOTYET
extern int __sigsetjmp (struct __jmp_buf_tag __env[1], int __savemask) __THROW
	/*@*/;
extern void (*__cancel_routine) (void *)
	/*@*/;
extern void __pthread_register_cancel (__pthread_unwind_buf_t *__buf)
	/*@*/;
extern void __pthread_unwind_next (__pthread_unwind_buf_t *__buf)
	/*@*/;
extern void __pthread_unregister_cancel (__pthread_unwind_buf_t *__buf)
	/*@*/;
#endif

extern pthread_t pthread_self(void)
	/*@*/;
extern int pthread_equal(pthread_t t1, pthread_t t2)
	/*@*/;

extern int pthread_attr_init (pthread_attr_t *__attr)
		__THROW __nonnull ((1))
	/*@*/;
extern int pthread_attr_destroy (pthread_attr_t *__attr)
		__THROW __nonnull ((1))
	/*@*/;

extern int pthread_attr_setdetachstate (pthread_attr_t *__attr,
					int __detachstate)
		__THROW __nonnull ((1))
	/*@*/;

extern int pthread_create(/*@out@*/ pthread_t *__restrict __newthread,
		/*@null@*/ __const pthread_attr_t *__restrict __attr,
		void *(*__start_routine)(void*),
		void *__restrict arg) __THROW __nonnull ((1, 3))
	/*@modifies *__newthread @*/;
extern int pthread_join(pthread_t thread, /*@null@*/ /*@out@*/ void **value_ptr)
	/*@modifies *value_ptr @*/;

extern int pthread_mutex_destroy(pthread_mutex_t *mutex)
	/*@modifies *mutex @*/;
extern int pthread_mutex_init(/*@out@*/ pthread_mutex_t *restrict mutex,
		/*@null@*/ const pthread_mutexattr_t *restrict attr)
	/*@globals errno, internalState @*/
	/*@modifies *mutex, errno, internalState @*/;

extern int pthread_mutex_lock(pthread_mutex_t *mutex)
	/*@globals errno @*/
	/*@modifies *mutex, errno @*/;
extern int pthread_mutex_trylock(pthread_mutex_t *mutex)
	/*@globals errno @*/
	/*@modifies *mutex, errno @*/;
extern int pthread_mutex_unlock(pthread_mutex_t *mutex)
	/*@globals errno @*/
	/*@modifies *mutex, errno @*/;

extern int pthread_cond_destroy(pthread_cond_t *cond)
	/*@modifies *cond @*/;
extern int pthread_cond_init(/*@out@*/ pthread_cond_t *restrict cond,
		/*@null@*/ const pthread_condattr_t *restrict attr)
	/*@globals errno, internalState @*/
	/*@modifies *cond, errno, internalState @*/;

extern int pthread_cond_timedwait(pthread_cond_t *restrict cond,
		pthread_mutex_t *restrict mutex,
		const struct timespec *restrict abstime)
	/*@modifies *cond, *mutex @*/;
extern int pthread_cond_wait(pthread_cond_t *restrict cond,
		pthread_mutex_t *restrict mutex)
	/*@modifies *cond, *mutex @*/;
extern int pthread_cond_broadcast(pthread_cond_t *cond)
	/*@globals errno, internalState @*/
	/*@modifies *cond, errno, internalState @*/;
extern int pthread_cond_signal(pthread_cond_t *cond)
	/*@globals errno, internalState @*/
	/*@modifies *cond, errno, internalState @*/;

extern int pthread_cancel (pthread_t __th)
	/*@*/;

/*@=exportheader@*/
/*@=incondefs =protoparammatch =redecl @*/
#endif	/* __LCLINT__ */

/* interface definition */
#include "yarn.h"

#include "debug.h"

/* error handling external globals, resettable by application */
/*@unchecked@*/ /*@observer@*/
const char *yarnPrefix = "yarn";
/*@-nullassign -redecl @*/
void (*yarnAbort)(int) = NULL;
/*@=nullassign =redecl @*/


/* immediately exit -- use for errors that shouldn't ever happen */
/*@exits@*/
static void _fail(int err, const char * fn, unsigned ln)
	/*@globals fileSystem, internalState @*/
	/*@modifies fileSystem, internalState @*/
{
    fprintf(stderr, "%s: pthread error: %s(%d) -- aborting at %s:%u\n", yarnPrefix,
            strerror(err), err, fn, ln);
    if (yarnAbort != NULL)
        yarnAbort(err);
assert(0);
    exit(err == ENOMEM || err == EAGAIN ? err : EINVAL);
}
#define	fail(_err)	_fail((_err), __FILE__, __LINE__)

/* memory handling routines provided by user -- if none are provided, malloc()
   and free() are used, which are therefore assumed to be thread-safe */
typedef void *(*malloc_t)(size_t);
typedef void (*free_t)(void *);
#if defined(__LCLINT__)
static void *(*my_malloc_f)(size_t nb)
	/*@*/
	= malloc;
static void (*my_free)(/*@only@*/ void * p)
	/*@modifies p @*/
	= free;
#else
static malloc_t my_malloc_f = malloc;
static free_t my_free = free;
#endif

/* use user-supplied allocation routines instead of malloc() and free() */
/*@-mustmod @*/
void yarnMem(malloc_t lease, free_t vacate)
	/*@globals my_malloc_f, my_free @*/
	/*@modifies my_malloc_f, my_free @*/
{
    my_malloc_f = lease;
    my_free = vacate;
}
/*@=mustmod @*/

/* memory allocation that cannot fail (from the point of view of the caller) */
static void *my_malloc(size_t size)
	/*@globals fileSystem, internalState @*/
	/*@modifies fileSystem, internalState @*/
{
    void *block;

    if ((block = my_malloc_f(size)) == NULL)
        fail(ENOMEM);
    return block;
}

/* -- lock functions -- */

struct yarnLock_s {
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    long value;
};

yarnLock yarnNewLock(long initial)
{
    int ret;
    yarnLock bolt;

    bolt = (yarnLock) my_malloc(sizeof(*bolt));
    if ((ret = pthread_mutex_init(&(bolt->mutex), NULL)) ||
        (ret = pthread_cond_init(&(bolt->cond), NULL)))
        fail(ret);
    bolt->value = initial;
    return bolt;
}

/*@-mustmod@*/
void yarnPossess(yarnLock bolt)
{
    int ret;

    if ((ret = pthread_mutex_lock(&(bolt->mutex))) != 0)
        fail(ret);
}

void yarnRelease(yarnLock bolt)
{
    int ret;

    if ((ret = pthread_mutex_unlock(&(bolt->mutex))) != 0)
        fail(ret);
}
/*@=mustmod@*/

void yarnTwist(yarnLock bolt, yarnTwistOP op, long val)
{
    int ret;

    if (op == TO)
        bolt->value = val;
    else if (op == BY)
        bolt->value += val;
    if ((ret = pthread_cond_broadcast(&(bolt->cond))) ||
        (ret = pthread_mutex_unlock(&(bolt->mutex))))
        fail(ret);
}

#define until(a) while(!(a))

/*@-mustmod@*/
void yarnWaitFor(yarnLock bolt, yarnWaitOP op, long val)
{
    int ret;

/*@-infloops -whileblock@*/	/* XXX splint can't see non-annotated effects */
    switch (op) {
    case TO_BE:
        until (bolt->value == val)
            if ((ret = pthread_cond_wait(&(bolt->cond), &(bolt->mutex))) != 0)
                fail(ret);
        break;
    case NOT_TO_BE:
        until (bolt->value != val)
            if ((ret = pthread_cond_wait(&(bolt->cond), &(bolt->mutex))) != 0)
                fail(ret);
        break;
    case TO_BE_MORE_THAN:
        until (bolt->value > val)
            if ((ret = pthread_cond_wait(&(bolt->cond), &(bolt->mutex))) != 0)
                fail(ret);
        break;
    case TO_BE_LESS_THAN:
        until (bolt->value < val)
            if ((ret = pthread_cond_wait(&(bolt->cond), &(bolt->mutex))) != 0)
                fail(ret);
    }
/*@=infloops =whileblock@*/
}
/*@=mustmod@*/

long yarnPeekLock(yarnLock bolt)
{
    return bolt->value;
}

yarnLock yarnFreeLock(yarnLock bolt)
{
    int ret;
    if ((ret = pthread_cond_destroy(&(bolt->cond))) ||
        (ret = pthread_mutex_destroy(&(bolt->mutex))))
        fail(ret);
    my_free(bolt);
    return NULL;
}

/* -- thread functions (uses lock functions above) -- */

struct yarnThread_s {
    pthread_t id;
    int done;                   /* true if this thread has exited */
/*@dependent@*/ /*@null@*/
    yarnThread next;            /* for list of all launched threads */
};

/* list of threads launched but not joined, count of threads exited but not
   joined (incremented by yarnIgnition() just before exiting) */
/*@unchecked@*/
static struct yarnLock_s threads_lock = {
    PTHREAD_MUTEX_INITIALIZER,
    PTHREAD_COND_INITIALIZER,
    0                           /* number of threads exited but not joined */
};
/*@unchecked@*/ /*@owned@*/ /*@null@*/
static yarnThread threads = NULL;       /* list of extant threads */

/* structure in which to pass the probe and its payload to yarnIgnition() */
struct capsule {
    void (*probe)(void *)
	/*@*/;
    void * payload;
};

/* mark the calling thread as done and alert yarnJoinAll() */
#if defined(WITH_PTHREADS)
static void yarnReenter(/*@unused@*/ void * dummy)
	/*@globals threads, threads_lock, fileSystem, internalState @*/
	/*@modifies threads, threads_lock, fileSystem, internalState @*/
{
    yarnThread match;
    yarnThread *prior;
    pthread_t me;

    /* find this thread in the threads list by matching the thread id */
    me = pthread_self();
    yarnPossess(&(threads_lock));
    prior = &(threads);
    while ((match = *prior) != NULL) {
        if (pthread_equal(match->id, me))
            break;
        prior = &(match->next);
    }
    if (match == NULL)
        fail(EINVAL);

    /* mark this thread as done and move it to the head of the list */
    match->done = 1;
    if (threads != match) {
        *prior = match->next;
        match->next = threads;
        threads = match;
    }

    /* update the count of threads to be joined and alert yarnJoinAll() */
    yarnTwist(&(threads_lock), BY, 1);
}
#endif

/* all threads go through this routine so that just before the thread exits,
   it marks itself as done in the threads list and alerts yarnJoinAll() so that
   the thread resources can be released -- use cleanup stack so that the
   marking occurs even if the thread is cancelled */
/*@null@*/
#if defined(WITH_PTHREADS)
static void * yarnIgnition(/*@only@*/ void * arg)
	/*@*/
{
    struct capsule *capsule = (struct capsule *)arg;

    /* run yarnReenter() before leaving */
/*@-moduncon -noeffectuncon -sysunrecog @*/
    pthread_cleanup_push(yarnReenter, NULL);
/*@=moduncon =noeffectuncon =sysunrecog @*/

    /* execute the requested function with argument */
/*@-noeffectuncon@*/
    capsule->probe(capsule->payload);
/*@=noeffectuncon@*/
    my_free(capsule);

    /* mark this thread as done and let yarnJoinAll() know */
/*@-moduncon -noeffect -noeffectuncon @*/
    pthread_cleanup_pop(1);
/*@=moduncon =noeffect =noeffectuncon @*/

    /* exit thread */
    return NULL;
}
#endif

/* not all POSIX implementations create threads as joinable by default, so that
   is made explicit here */
yarnThread yarnLaunchStack(void (*probe)(void *), void * payload,
		void * stack, size_t nstack)
	/*@globals threads @*/
	/*@modifies threads @*/
{
    int ret;
    yarnThread th;
    struct capsule * capsule;
#if defined(WITH_PTHREADS)
    pthread_attr_t attr;
#endif

    /* construct the requested call and argument for the yarnIgnition() routine
       (allocated instead of automatic so that we're sure this will still be
       there when yarnIgnition() actually starts up -- yarnIgnition() will free this
       allocation) */
    capsule = (struct capsule *) my_malloc(sizeof(*capsule));
    capsule->probe = probe;
/*@-mustfreeonly -temptrans @*/
    capsule->payload = payload;
/*@=mustfreeonly =temptrans @*/

    /* assure this thread is in the list before yarnJoinAll() or yarnIgnition() looks
       for it */
    yarnPossess(&(threads_lock));

    /* create the thread and call yarnIgnition() from that thread */
    th = (yarnThread) my_malloc(sizeof(*th));
    if ((ret = pthread_attr_init(&attr))
     || (ret = (stack ? pthread_attr_setstack(&attr, stack, nstack) : 0))
     || (ret = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE))
     || (ret = pthread_create(&(th->id), &attr, yarnIgnition, capsule))
     || (ret = pthread_attr_destroy(&attr)))
        fail(ret);

    /* put the thread in the threads list for yarnJoinAll() */
    th->done = 0;
    th->next = threads;
    threads = th;
    yarnRelease(&(threads_lock));
/*@-dependenttrans -globstate -mustfreefresh -retalias @*/ /* XXX what frees capsule?!? */
    return th;
/*@=dependenttrans =globstate =mustfreefresh =retalias @*/
}

yarnThread yarnLaunch(void (*probe)(void *), void * payload)
	/*@globals threads @*/
	/*@modifies threads @*/
{
    return yarnLaunchStack(probe, payload, NULL, 0);
}

yarnThread yarnJoin(yarnThread ally)
	/*@globals threads, threads_lock @*/
	/*@modifies threads, threads_lock @*/
{
    yarnThread match;
    yarnThread *prior;
    int ret;

    /* wait for thread to exit and return its resources */
    if ((ret = pthread_join(ally->id, NULL)) != 0)
        fail(ret);

    /* find the thread in the threads list */
    yarnPossess(&(threads_lock));
    prior = &(threads);
    while ((match = *prior) != NULL) {
        if (match == ally)
            break;
        prior = &(match->next);
    }
    if (match == NULL)
        fail(EINVAL);

    /* remove thread from list and update exited count, free thread */
    if (match->done)
        threads_lock.value--;
    *prior = match->next;
    yarnRelease(&(threads_lock));
    my_free(ally);
    return NULL;
}

/* This implementation of yarnJoinAll() only attempts to join threads that have
   announced that they have exited (see yarnIgnition()).  When there are many
   threads, this is faster than waiting for some random thread to exit while a
   bunch of other threads have already exited. */
int yarnJoinAll(void)
	/*@globals threads, threads_lock @*/
	/*@modifies threads, threads_lock @*/
{
    yarnThread match;
    yarnThread *prior;
    int ret;
    int count;

    /* grab the threads list and initialize the joined count */
    count = 0;
    yarnPossess(&(threads_lock));

    /* do until threads list is empty */
    while (threads != NULL) {
        /* wait until at least one thread has reentered */
        yarnWaitFor(&(threads_lock), NOT_TO_BE, 0);

        /* find the first thread marked done (should be at or near the top) */
        prior = &(threads);
        while ((match = *prior) != NULL) {
            if (match->done)
                /*@innerbreak@*/ break;
            prior = &(match->next);
        }
        if (match == NULL)
            fail(EINVAL);

        /* join the thread (will be almost immediate), remove from the threads
           list, update the reenter count, and free the thread */
        if ((ret = pthread_join(match->id, NULL)) != 0)
            fail(ret);
        threads_lock.value--;
        *prior = match->next;
        my_free(match);
        count++;
    }

    /* let go of the threads list and return the number of threads joined */
    yarnRelease(&(threads_lock));
/*@-globstate@*/
    return count;
/*@=globstate@*/
}

/* cancel and join the thread -- the thread will cancel when it gets to a file
   operation, a sleep or pause, or a condition wait */
void yarnDestruct(yarnThread off_course)
{
    int ret;

    if ((ret = pthread_cancel(off_course->id)) != 0)
        fail(ret);
    (void) yarnJoin(off_course);
}
