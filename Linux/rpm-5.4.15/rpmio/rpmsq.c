/** \ingroup rpmio
 * \file rpmio/rpmsq.c
 */

#include "system.h"

#if defined(__LCLINT__)
#define	_BITS_SIGTHREAD_H	/* XXX avoid __sigset_t heartburn. */

/*@-incondefs -protoparammatch@*/
/*@-exportheader@*/
/*@constant int SA_SIGINFO@*/
extern int sighold(int sig)
	/*@globals errno, systemState @*/;
extern int sigignore(int sig)
	/*@globals errno, systemState @*/;
extern int sigpause(int sig)
	/*@globals errno, systemState @*/;
extern int sigrelse(int sig)
	/*@globals errno, systemState @*/;
extern void (*sigset(int sig, void (*disp)(int)))(int)
	/*@globals errno, systemState @*/;

struct qelem;
extern	void __insque(struct qelem * __elem, struct qelem * __prev)
	/*@modifies  __elem, __prev @*/;
extern	void __remque(struct qelem * __elem)
	/*@modifies  __elem @*/;

extern pthread_t pthread_self(void)
	/*@*/;
extern int pthread_equal(pthread_t t1, pthread_t t2)
	/*@*/;

extern int pthread_create(/*@out@*/ pthread_t *restrict thread,
		const pthread_attr_t *restrict attr,
		void *(*start_routine)(void*), void *restrict arg)
	/*@modifies *thread @*/;
extern int pthread_join(pthread_t thread, /*@out@*/ void **value_ptr)
	/*@modifies *value_ptr @*/;

extern int pthread_setcancelstate(int state, /*@out@*/ int *oldstate)
	/*@globals internalState @*/
	/*@modifies *oldstate, internalState @*/;
extern int pthread_setcanceltype(int type, /*@out@*/ int *oldtype)
	/*@globals internalState @*/
	/*@modifies *oldtype, internalState @*/;
extern void pthread_testcancel(void)
	/*@globals internalState @*/
	/*@modifies internalState @*/;
extern void pthread_cleanup_pop(int execute)
	/*@globals internalState @*/
	/*@modifies internalState @*/;
extern void pthread_cleanup_push(void (*routine)(void*), void *arg)
	/*@globals internalState @*/
	/*@modifies internalState @*/;
extern void _pthread_cleanup_pop(/*@out@*/ struct _pthread_cleanup_buffer *__buffer, int execute)
	/*@globals internalState @*/
	/*@modifies internalState @*/;
extern void _pthread_cleanup_push(/*@out@*/ struct _pthread_cleanup_buffer *__buffer, void (*routine)(void*), /*@out@*/ void *arg)
	/*@globals internalState @*/
	/*@modifies internalState @*/;

extern int pthread_mutexattr_destroy(pthread_mutexattr_t *attr)
	/*@globals errno, internalState @*/
	/*@modifies *attr, errno, internalState @*/;
extern int pthread_mutexattr_init(/*@out@*/ pthread_mutexattr_t *attr)
	/*@globals errno, internalState @*/
	/*@modifies *attr, errno, internalState @*/;

int pthread_mutexattr_gettype(const pthread_mutexattr_t *restrict attr,
		/*@out@*/ int *restrict type)
	/*@modifies *type @*/;
int pthread_mutexattr_settype(pthread_mutexattr_t *attr, int type)
	/*@globals errno, internalState @*/
	/*@modifies *attr, errno, internalState @*/;

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
		const pthread_condattr_t *restrict attr)
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

/*@=exportheader@*/
/*@=incondefs =protoparammatch@*/
#endif

#include <signal.h>
#if !defined(__QNX__)
#  include <sys/signal.h>
#endif
#include <sys/wait.h>
#include <search.h>

/* portability fallback for sighold(3) */
#if !defined(HAVE_SIGHOLD) && defined(HAVE_SIGPROCMASK) && defined(HAVE_SIGADDSET)
static int __RPM_sighold(int sig)
{
    sigset_t set;
    if (sigprocmask(SIG_SETMASK, NULL, &set) < 0)
        return -1;
    if (sigaddset(&set, sig) < 0)
        return -1;
    return sigprocmask(SIG_SETMASK, &set, NULL);
}
#define sighold(sig) __RPM_sighold(sig)
#endif

/* portability fallback for sigrelse(3) */
#if !defined(HAVE_SIGRELSE) && defined(HAVE_SIGPROCMASK) && defined(HAVE_SIGDELSET)
static int __RPM_sigrelse(int sig)
{
    sigset_t set;
    if (sigprocmask(SIG_SETMASK, NULL, &set) < 0)
        return -1;
    if (sigdelset(&set, sig) < 0)
        return -1;
    return sigprocmask(SIG_SETMASK, &set, NULL);
}
#define sigrelse(sig) __RPM_sigrelse(sig)
#endif

/* portability fallback for sigpause(3) */
#if !defined(HAVE_SIGPAUSE) && defined(HAVE_SIGEMPTYSET) && defined(HAVE_SIGADDSET) && defined(HAVE_SIGSUSPEND)
static int __RPM_sigpause(int sig)
{
    sigset_t set;
    if (sigemptyset(&set) < 0)
        return -1;
    if (sigaddset(&set, sig) < 0)
        return -1;
    return sigsuspend(&set);
}
#define sigpause(sig) __RPM_sigpause(sig)
#endif

/* portability fallback for insque(3)/remque(3) */
#if defined(__CYGWIN__) || defined(__MINGW32__) || defined(__QNX__)
struct qelem {
  struct qelem *q_forw;
  struct qelem *q_back;
};

static void __insque(struct qelem * elem, struct qelem * pred)
{
  elem -> q_forw = pred -> q_forw;
  pred -> q_forw -> q_back = elem;
  elem -> q_back = pred;
  pred -> q_forw = elem;
}
#define	insque(_e, _p)	__insque((_e), (_p))

static	void __remque(struct qelem * elem)
{
  elem -> q_forw -> q_back = elem -> q_back;
  elem -> q_back -> q_forw = elem -> q_forw;
}
#define	remque(_e)	__remque(_e)
#endif

#if defined(WITH_PTHREADS)

# if !defined(__QNX__)
/* XXX suggested in bugzilla #159024 */
#  if PTHREAD_MUTEX_DEFAULT != PTHREAD_MUTEX_NORMAL
#    error RPM expects PTHREAD_MUTEX_DEFAULT == PTHREAD_MUTEX_NORMAL
#  endif
# endif

#ifndef PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP
/*@unchecked@*/
static pthread_mutex_t rpmsigTbl_lock = PTHREAD_MUTEX_INITIALIZER;
#else
/*@unchecked@*/
/*@-type@*/
static pthread_mutex_t rpmsigTbl_lock = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;
/*@=type@*/
#endif

/*@-macromatchname@*/
#define	DO_LOCK()	/*@-retvalint@*/pthread_mutex_lock(&rpmsigTbl_lock)/*@=retvalint@*/;
#define	DO_UNLOCK()	pthread_mutex_unlock(&rpmsigTbl_lock);
#define	INIT_LOCK()	\
    {	pthread_mutexattr_t attr; \
	(void) pthread_mutexattr_init(&attr); \
	(void) pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE); \
	(void) pthread_mutex_init (&rpmsigTbl_lock, &attr); \
	(void) pthread_mutexattr_destroy(&attr); \
	rpmsigTbl_sigchld->active = 0; \
    }
#define	ADD_REF(__tbl)	(__tbl)->active++
#define	SUB_REF(__tbl)	--(__tbl)->active
#define	CLEANUP_HANDLER(__handler, __arg, __oldtypeptr) \
    (void) pthread_setcanceltype (PTHREAD_CANCEL_ASYNCHRONOUS, (__oldtypeptr));\
	pthread_cleanup_push((__handler), (__arg));
#define	CLEANUP_RESET(__execute, __oldtype) \
    pthread_cleanup_pop(__execute); \
    (void) pthread_setcanceltype ((__oldtype), &(__oldtype));

#define	SAME_THREAD(_a, _b)	pthread_equal(((pthread_t)_a), ((pthread_t)_b))
/*@-macromatchname@*/

#define ME() __tid2vp(pthread_self())
/*@shared@*/
static void *__tid2vp(pthread_t tid)
	/*@*/
{
    union { pthread_t tid; /*@shared@*/ void *vp; } u;
    u.tid = tid;
    return u.vp;
}

#else

/*@-macromatchname@*/
#define	DO_LOCK() (0)
#define	DO_UNLOCK() (0)
#define	INIT_LOCK()
#define	ADD_REF(__tbl)	/*@-noeffect@*/ (0) /*@=noeffect@*/
#define	SUB_REF(__tbl)	/*@-noeffect@*/ (0) /*@=noeffect@*/
#define	CLEANUP_HANDLER(__handler, __arg, __oldtypeptr)
#define	CLEANUP_RESET(__execute, __oldtype)

#define	SAME_THREAD(_a, _b)	(42)
/*@=macromatchname@*/

#define ME() __pid2vp(getpid())
/*@shared@*/
static void *__pid2vp(pid_t pid)
	/*@*/
{
    union { pid_t pid; /*@shared@*/ void *vp; } u;
    u.pid = pid;
    return u.vp;
}

#endif	/* WITH_PTHREADS */

#define	_RPMSQ_INTERNAL
#include <rpmsq.h>

#include "debug.h"

#define	_RPMSQ_DEBUG	0
/*@unchecked@*/
int _rpmsq_debug = _RPMSQ_DEBUG;

/* XXX __OpenBSD__ insque(3) needs rock->q_forw initialized. */
/*@unchecked@*/
/*@-fullinitblock @*/
static struct rpmsqElem rpmsqRock = { &rpmsqRock, NULL };
/*@=fullinitblock @*/

/*@-compmempass@*/
/*@unchecked@*/
rpmsq rpmsqQueue = &rpmsqRock;
/*@=compmempass@*/

int rpmsqInsert(void * elem, void * prev)
{
    rpmsq sq = (rpmsq) elem;
    int ret = -1;

    if (sq != NULL) {
#ifdef _RPMSQ_DEBUG
if (_rpmsq_debug)
fprintf(stderr, "    Insert(%p): %p\n", ME(), sq);
#endif
	ret = sighold(SIGCHLD);
	if (ret == 0) {
	    sq->child = 0;
	    sq->reaped = 0;
	    sq->status = 0;
	   /* ==> Set to 1 to catch SIGCHLD, set to 0 to use waitpid(2).  */
	    sq->reaper = 1;
	    sq->pipes[0] = sq->pipes[1] = -1;

	    sq->id = ME();
/*@-noeffect@*/
	    insque(elem, (prev != NULL ? prev : rpmsqQueue));
/*@=noeffect@*/
	    ret = sigrelse(SIGCHLD);
	}
    }
    return ret;
}

int rpmsqRemove(void * elem)
{
    rpmsq sq = (rpmsq) elem;
    int ret = -1;

    if (elem != NULL) {

#ifdef _RPMSQ_DEBUG
if (_rpmsq_debug)
fprintf(stderr, "    Remove(%p): %p\n", ME(), sq);
#endif
	ret = sighold (SIGCHLD);
	if (ret == 0) {
/*@-noeffect@*/
	    remque(elem);
/*@=noeffect@*/
	    sq->id = NULL;
	    if (sq->pipes[1] > 0)	ret = close(sq->pipes[1]);
	    if (sq->pipes[0] > 0)	ret = close(sq->pipes[0]);
	    sq->pipes[0] = sq->pipes[1] = -1;
#ifdef	NOTYET	/* rpmpsmWait debugging message needs */
	    sq->status = 0;
	    sq->reaped = 0;
	    sq->child = 0;
#endif
	    ret = sigrelse(SIGCHLD);
	}
    }
    return ret;
}

/*@unchecked@*/
sigset_t rpmsqCaught;

/*@unchecked@*/
/*@-fullinitblock@*/
static struct rpmsig_s {
    int signum;
    void (*handler) (int signum, void * info, void * context);
    int active;
    struct sigaction oact;
} rpmsigTbl[] = {
    { SIGINT,	rpmsqAction },
#define	rpmsigTbl_sigint	(&rpmsigTbl[0])
    { SIGQUIT,	rpmsqAction },
#define	rpmsigTbl_sigquit	(&rpmsigTbl[1])
    { SIGCHLD,	rpmsqAction },
#define	rpmsigTbl_sigchld	(&rpmsigTbl[2])
    { SIGHUP,	rpmsqAction },
#define	rpmsigTbl_sighup	(&rpmsigTbl[3])		/* XXX unused */
    { SIGTERM,	rpmsqAction },
#define	rpmsigTbl_sigterm	(&rpmsigTbl[4])		/* XXX unused */
    { SIGPIPE,	rpmsqAction },
#define	rpmsigTbl_sigpipe	(&rpmsigTbl[5])		/* XXX unused */

#ifdef	NOTYET		/* XXX todo++ */
#if defined(SIGXCPU)
    { SIGXCPU,	rpmsqAction },
#define	rpmsigTbl_sigxcpu	(&rpmsigTbl[6])		/* XXX unused */
#endif
#if defined(SIGXFSZ)
    { SIGXFSZ,	rpmsqAction },
#define	rpmsigTbl_sigxfsz	(&rpmsigTbl[7])		/* XXX unused */
#endif
#endif

    { -1,	NULL },
};
/*@=fullinitblock@*/

void rpmsqAction(int signum, /*@unused@*/ void * info,
		/*@unused@*/ void * context)
{
    int save = errno;
    rpmsig tbl;

    for (tbl = rpmsigTbl; tbl->signum >= 0; tbl++) {
	if (tbl->signum != signum)
	    continue;

	(void) sigaddset(&rpmsqCaught, signum);

	switch (signum) {
	case SIGCHLD:
	    while (1) {
		rpmsq sq;
		int status = 0;
		pid_t reaped = waitpid(0, &status, WNOHANG);

		/* XXX errno set to ECHILD/EINVAL/EINTR. */
		if (reaped <= 0)
		    /*@innerbreak@*/ break;

		/* XXX insque(3)/remque(3) are dequeue, not ring. */
		for (sq = rpmsqQueue->q_forw;
		     sq != NULL && sq != rpmsqQueue;
		     sq = sq->q_forw)
		{
		    int ret;

		    if (sq->child != reaped)
			/*@innercontinue@*/ continue;
		    sq->reaped = reaped;
		    sq->status = status;

		    ret = close(sq->pipes[1]);	sq->pipes[1] = -1;

		    /*@innerbreak@*/ break;
		}
	    }
	    /*@switchbreak@*/ break;
	default:
	    /*@switchbreak@*/ break;
	}
	break;
    }
    errno = save;
}

int rpmsqEnable(int signum, /*@null@*/ rpmsqAction_t handler)
	/*@globals rpmsigTbl @*/
	/*@modifies rpmsigTbl @*/
{
    int tblsignum = (signum >= 0 ? signum : -signum);
    struct sigaction sa;
    rpmsig tbl;
    int ret = (signum >= 0 ? 1 : 0);
    int xx;

    xx = DO_LOCK ();
    if (rpmsqQueue->id == NULL)
	rpmsqQueue->id = ME();
    for (tbl = rpmsigTbl; tbl->signum >= 0; tbl++) {
	if (tblsignum != tbl->signum)
	    continue;

	if (signum >= 0) {			/* Enable. */
	    if (ADD_REF(tbl) <= 0) {
		(void) sigdelset(&rpmsqCaught, tbl->signum);

		/* XXX Don't set a signal handler if already SIG_IGN */
		(void) sigaction(tbl->signum, NULL, &tbl->oact);
		if (tbl->oact.sa_handler == SIG_IGN)
		    continue;

		(void) sigemptyset (&sa.sa_mask);
		sa.sa_flags = SA_SIGINFO;
#if defined(__LCLINT__)	/* XXX glibc has union to track handler prototype. */
		sa.sa_handler = (handler != NULL ? handler : tbl->handler);
#else
		sa.sa_sigaction = (void (*)(int, siginfo_t*, void*)) (handler != NULL ? handler : tbl->handler);
#endif
		if (sigaction(tbl->signum, &sa, &tbl->oact) < 0) {
		    xx = SUB_REF(tbl);
		    break;
		}
		tbl->active = 1;		/* XXX just in case */
		if (handler != NULL)
		    tbl->handler = handler;
	    }
	} else {				/* Disable. */
	    if (SUB_REF(tbl) <= 0) {
		if (sigaction(tbl->signum, &tbl->oact, NULL) < 0)
		    break;
		tbl->active = 0;		/* XXX just in case */
		tbl->handler = (handler != NULL ? handler : rpmsqAction);
	    }
	}
	ret = tbl->active;
	break;
    }
    xx = DO_UNLOCK ();
    return ret;
}

pid_t rpmsqFork(rpmsq sq)
{
    pid_t pid;
    int xx;

    if (sq->reaper) {
	xx = rpmsqInsert(sq, NULL);
#ifdef _RPMSQ_DEBUG
if (_rpmsq_debug)
fprintf(stderr, "    Enable(%p): %p\n", ME(), sq);
#endif
	xx = rpmsqEnable(SIGCHLD, NULL);
    }

    xx = pipe(sq->pipes);

    xx = sighold(SIGCHLD);

    pid = fork();
    if (pid < (pid_t) 0) {		/* fork failed.  */
	sq->child = (pid_t)-1;
	xx = close(sq->pipes[0]);
	xx = close(sq->pipes[1]);
	sq->pipes[0] = sq->pipes[1] = -1;
	goto out;
    } else if (pid == (pid_t) 0) {	/* Child. */
	int yy;

	/* Block to permit parent time to wait. */
	xx = close(sq->pipes[1]);
	if (sq->reaper)
	    xx = (int)read(sq->pipes[0], &yy, sizeof(yy));
	xx = close(sq->pipes[0]);
	sq->pipes[0] = sq->pipes[1] = -1;

#ifdef _RPMSQ_DEBUG
if (_rpmsq_debug)
fprintf(stderr, "     Child(%p): %p child %d\n", ME(), sq, (int)getpid());
#endif

    } else {				/* Parent. */

	sq->child = pid;

#ifdef _RPMSQ_DEBUG
if (_rpmsq_debug)
fprintf(stderr, "    Parent(%p): %p child %d\n", ME(), sq, (int)sq->child);
#endif

    }

out:
    xx = sigrelse(SIGCHLD);
    return sq->child;
}

/**
 * Wait for child process to be reaped, and unregister SIGCHLD handler.
 * @todo Rewrite to use waitpid on helper thread.
 * @param sq		scriptlet queue element
 * @return		0 on success
 */
static int rpmsqWaitUnregister(rpmsq sq)
	/*@globals fileSystem, internalState @*/
	/*@modifies sq, fileSystem, internalState @*/
{
    int nothreads = 0;
    int ret = 0;
    int xx;

assert(sq->reaper);
    /* Protect sq->reaped from handler changes. */
    ret = sighold(SIGCHLD);

    /* Start the child, linux often runs child before parent. */
    if (sq->pipes[0] >= 0)
	xx = close(sq->pipes[0]);
    if (sq->pipes[1] >= 0)
	xx = close(sq->pipes[1]);

    /* Re-initialize the pipe to receive SIGCHLD receipt confirmation. */
    xx = pipe(sq->pipes);

    /* Put a stopwatch on the time spent waiting to measure performance gain. */
    (void) rpmswEnter(&sq->op, -1);

    /* Wait for handler to receive SIGCHLD. */
    /*@-infloops@*/
    while (ret == 0 && sq->reaped != sq->child) {
	if (nothreads)
	    /* Note that sigpause re-enables SIGCHLD. */
	    ret = sigpause(SIGCHLD);
	else {
	    xx = sigrelse(SIGCHLD);
	    
	    /* Signal handler does close(sq->pipes[1]) triggering 0b EOF read */
	    if (read(sq->pipes[0], &xx, sizeof(xx)) == 0) {
		xx = close(sq->pipes[0]);	sq->pipes[0] = -1;
		ret = 1;
	    }

	    xx = sighold(SIGCHLD);
	}
    }
    /*@=infloops@*/

    /* Accumulate stopwatch time spent waiting, potential performance gain. */
    sq->ms_scriptlets += rpmswExit(&sq->op, -1)/1000;

    xx = sigrelse(SIGCHLD);

#ifdef _RPMSQ_DEBUG
if (_rpmsq_debug)
fprintf(stderr, "      Wake(%p): %p child %d reaper %d ret %d\n", ME(), sq, (int)sq->child, sq->reaper, ret);
#endif

    /* Remove processed SIGCHLD item from queue. */
    xx = rpmsqRemove(sq);

    /* Disable SIGCHLD handler on refcount == 0. */
    xx = rpmsqEnable(-SIGCHLD, NULL);
#ifdef _RPMSQ_DEBUG
if (_rpmsq_debug)
fprintf(stderr, "   Disable(%p): %p\n", ME(), sq);
#endif

    return ret;
}

pid_t rpmsqWait(rpmsq sq)
{

#ifdef _RPMSQ_DEBUG
if (_rpmsq_debug)
fprintf(stderr, "      Wait(%p): %p child %d reaper %d\n", ME(), sq, (int)sq->child, sq->reaper);
#endif

    if (sq->reaper) {
	(void) rpmsqWaitUnregister(sq);
    } else {
	pid_t reaped;
	int status;
	do {
	    reaped = waitpid(sq->child, &status, 0);
	} while (reaped >= 0 && reaped != sq->child);
	sq->reaped = reaped;
	sq->status = status;
#ifdef _RPMSQ_DEBUG
if (_rpmsq_debug)
fprintf(stderr, "   Waitpid(%p): %p child %d reaped %d\n", ME(), sq, (int)sq->child, (int)sq->reaped);
#endif
    }

#ifdef _RPMSQ_DEBUG
if (_rpmsq_debug)
fprintf(stderr, "      Fini(%p): %p child %d status 0x%x\n", ME(), sq, (int)sq->child, sq->status);
#endif

    return sq->reaped;
}

void * rpmsqThread(void * (*start) (void * arg), void * arg)
{
#if defined(WITH_PTHREADS)
    pthread_t pth;
    int ret;

    ret = pthread_create(&pth, NULL, start, arg);
    return (ret == 0 ? (void *)pth : NULL);
#else
    (void) start;
    (void) arg; 
    return NULL;
#endif
}

int rpmsqJoin(void * thread)
{
#if defined(WITH_PTHREADS)
    pthread_t pth = (pthread_t) thread;
    if (thread == NULL)
	return EINVAL;
    return pthread_join(pth, NULL);
#else
    (void) thread;
    return EINVAL;
#endif
}

int rpmsqThreadEqual(void * thread)
{
#if defined(WITH_PTHREADS)
    pthread_t t1 = (pthread_t) thread;
    pthread_t t2 = pthread_self();
    return pthread_equal(t1, t2);
#else
    (void) thread;
    return 0;
#endif
}

/**
 * SIGCHLD cancellation handler.
 */
#if defined(WITH_PTHREADS)
static void
sigchld_cancel (void *arg)
	/*@globals rpmsigTbl, fileSystem, internalState @*/
	/*@modifies rpmsigTbl, fileSystem, internalState @*/
{
    pid_t child = *(pid_t *) arg;
    pid_t result;
    int xx;

    xx = kill(child, SIGKILL);

    do {
	result = waitpid(child, NULL, 0);
    } while (result == (pid_t)-1 && errno == EINTR);

    xx = DO_LOCK ();
    if (SUB_REF (rpmsigTbl_sigchld) == 0) {
	xx = rpmsqEnable(-SIGQUIT, NULL);
	xx = rpmsqEnable(-SIGINT, NULL);
    }
    xx = DO_UNLOCK ();
}
#endif

/**
 * Execute a command, returning its status.
 */
int
rpmsqExecve (const char ** argv)
	/*@globals rpmsigTbl @*/
	/*@modifies rpmsigTbl @*/
{
#if defined(WITH_PTHREADS)
    int oldtype;
#endif
    int status = -1;
    pid_t pid = 0;
    pid_t result;
    sigset_t newMask, oldMask;
    rpmsq sq = (rpmsq) memset(alloca(sizeof(*sq)), 0, sizeof(*sq));
    int xx;

#ifndef PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP
	INIT_LOCK ();
#endif

    xx = DO_LOCK ();
    if (ADD_REF (rpmsigTbl_sigchld) == 0) {
	if (rpmsqEnable(SIGINT, NULL) < 0) {
	    xx = SUB_REF (rpmsigTbl_sigchld);
	    goto out;
	}
	if (rpmsqEnable(SIGQUIT, NULL) < 0) {
	    xx = SUB_REF (rpmsigTbl_sigchld);
	    goto out_restore_sigint;
	}
    }
    xx = DO_UNLOCK ();

    (void) sigemptyset (&newMask);
    (void) sigaddset (&newMask, SIGCHLD);
    if (sigprocmask (SIG_BLOCK, &newMask, &oldMask) < 0) {
	xx = DO_LOCK ();
	if (SUB_REF (rpmsigTbl_sigchld) == 0)
	    goto out_restore_sigquit_and_sigint;
	goto out;
    }

/*@-sysunrecog@*/
    CLEANUP_HANDLER(sigchld_cancel, &pid, &oldtype);
/*@=sysunrecog@*/

    pid = fork ();
    if (pid < (pid_t) 0) {		/* fork failed.  */
	goto out;
    } else if (pid == (pid_t) 0) {	/* Child. */

	/* Restore the signals.  */
	(void) sigaction (SIGINT, &rpmsigTbl_sigint->oact, NULL);
	(void) sigaction (SIGQUIT, &rpmsigTbl_sigquit->oact, NULL);
	(void) sigprocmask (SIG_SETMASK, &oldMask, NULL);

	/* Reset rpmsigTbl lock and refcnt. */
	INIT_LOCK ();

	(void) execve (argv[0], (char *const *) argv, environ);
	_exit (127);
    } else {				/* Parent. */
	do {
	    result = waitpid(pid, &status, 0);
	} while (result == (pid_t)-1 && errno == EINTR);
	if (result != pid)
	    status = -1;
    }

    CLEANUP_RESET(0, oldtype);

    xx = DO_LOCK ();
    if ((SUB_REF (rpmsigTbl_sigchld) == 0 &&
        (rpmsqEnable(-SIGINT, NULL) < 0 || rpmsqEnable (-SIGQUIT, NULL) < 0))
      || sigprocmask (SIG_SETMASK, &oldMask, NULL) != 0)
    {
	status = -1;
    }
    goto out;

out_restore_sigquit_and_sigint:
    xx = rpmsqEnable(-SIGQUIT, NULL);
out_restore_sigint:
    xx = rpmsqEnable(-SIGINT, NULL);
out:
    xx = DO_UNLOCK ();
    return status;
}
