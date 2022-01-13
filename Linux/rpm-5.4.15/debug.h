/**
 * To be included after all other includes.
 */
#ifndef	H_DEBUG
#define	H_DEBUG

#ifdef HAVE_ASSERT_H
#undef	assert	/* XXX <beecrypt/api.h> tries to retrofit an assert(x) macro */
#include <assert.h>
#endif

#ifdef  __LCLINT__
#define	ntohl(_x)	(_x)
#define	ntohs(_x)	(_x)
#define	htonl(_x)	(_x)
#define	htons(_x)	(_x)
#endif

#if defined(_OPENMP)
#include <omp.h>
#endif

#ifdef	WITH_DMALLOC
#include <dmalloc.h>
#endif

/* If using GCC, wrap __builtin_expect() (reduces overhead in lookup3.c) */
#if defined(__GNUC__) && (__GNUC__ > 2) && defined(__OPTIMIZE__)
#  define UNLIKELY(value) __builtin_expect((value), 0)
#else
#  define UNLIKELY(value) (value)
#endif

#if defined(WITH_VALGRIND) && defined(HAVE_VALGRIND_VALGRIND_H)

#include <valgrind/valgrind.h>
#include <valgrind/memcheck.h>
#include <valgrind/callgrind.h>
#include <valgrind/helgrind.h>
#include <valgrind/drd.h>

#if !defined(ANNOTATE_BENIGN_RACE_SIZED)
#define	ANNOTATE_BENIGN_RACE_SIZED(_a, _b, _c)
#endif

static inline void * DRD_xmalloc(size_t nb)
{
    void * ptr = xmalloc(nb);
ANNOTATE_BENIGN_RACE_SIZED(ptr, nb, __FUNCTION__);	/* XXX tsan sanity. */
    return ptr;
}

static inline void * DRD_xcalloc(size_t nmemb, size_t size)
{
    size_t nb = nmemb * size;
    void * ptr = DRD_xmalloc(nb);
    memset(ptr, 0, nb);
    return ptr;
}

static inline void * DRD_xrealloc(void * ptr, size_t size)
{
    ptr = xrealloc(ptr, size);
ANNOTATE_BENIGN_RACE_SIZED(ptr, size, __FUNCTION__);	/* XXX tsan sanity. */
    return ptr;
}

static inline char * DRD_xstrdup(const char * s)
{
    size_t nb = strlen(s) + 1;
    char * t = (char *) DRD_xmalloc(nb);
    return strcpy(t, s);
}

#else

/* XXX list most everything just to see the valgrind namespace. */

#define RUNNING_ON_VALGRIND	(0)
#define VALGRIND_DISCARD_TRANSLATIONS(_qzz_addr,_qzz_len)

#define VALGRIND_MALLOCLIKE_BLOCK(addr, sizeB, rzB, is_zeroed)
#define VALGRIND_FREELIKE_BLOCK(addr, rzB)

#define	VALGRIND_CREATE_MEMPOOL(pool, rzB, is_zeroed)
#define	VALGRIND_DESTROY_MEMPOOL(pool)
#define	VALGRIND_MEMPOOL_ALLOC(pool, addr, size)
#define	VALGRIND_MEMPOOL_FREE(pool, addr)
#define	VALGRIND_MEMPOOL_TRIM(pool, addr, size)
#define	VALGRIND_MOVE_MEMPOOL(poolA, poolB)
#define	VALGRIND_MEMPOOL_CHANGE(pool, addrA, addrB, size)
#define	VALGRIND_MEMPOOL_EXISTS(pool)	(0)

#define VALGRIND_MAKE_MEM_NOACCESS(_qzz_addr,_qzz_len)
#define VALGRIND_MAKE_MEM_UNDEFINED(_qzz_addr,_qzz_len)
#define VALGRIND_MAKE_MEM_DEFINED(_qzz_addr,_qzz_len)
#define VALGRIND_MAKE_MEM_DEFINED_IF_ADDRESSABLE(_qzz_addr,_qzz_len)
#define VALGRIND_CREATE_BLOCK(_qzz_addr,_qzz_len, _qzz_desc)
#define VALGRIND_DISCARD(_qzz_blkindex)
#define VALGRIND_CHECK_MEM_IS_ADDRESSABLE(_qzz_addr,_qzz_len)
#define VALGRIND_CHECK_MEM_IS_DEFINED(_qzz_addr,_qzz_len)
#define VALGRIND_CHECK_VALUE_IS_DEFINED(__lvalue)
#define VALGRIND_DO_LEAK_CHECK
#define VALGRIND_DO_QUICK_LEAK_CHECK
#define VALGRIND_COUNT_LEAKS(leaked, dubious, reachable, suppressed)
#define VALGRIND_COUNT_LEAK_BLOCKS(leaked, dubious, reachable, suppressed)
#define VALGRIND_GET_VBITS(zza,zzvbits,zznbytes)
#define VALGRIND_SET_VBITS(zza,zzvbits,zznbytes)

#define	CALLGRIND_DUMP_STATS
#define	CALLGRIND_DUMP_STATS_AT(pos_str)
#define	CALLGRIND_ZERO_STATS
#define	CALLGRIND_TOGGLE_COLLECT
#define	CALLGRIND_START_INSTRUMENTATION
#define	CALLGRIND_STOP_INSTRUMENTATION

#define VALGRIND_HG_MUTEX_INIT_POST(_mutex, _mbRec)
#define VALGRIND_HG_MUTEX_LOCK_PRE(_mutex, _isTryLock)
#define VALGRIND_HG_MUTEX_LOCK_POST(_mutex)
#define VALGRIND_HG_MUTEX_UNLOCK_PRE(_mutex)
#define VALGRIND_HG_MUTEX_UNLOCK_POST(_mutex)
#define VALGRIND_HG_MUTEX_DESTROY_PRE(_mutex)
#define VALGRIND_HG_SEM_INIT_POST(_sem, _value)
#define VALGRIND_HG_SEM_WAIT_POST(_sem)
#define VALGRIND_HG_SEM_POST_PRE(_sem)
#define VALGRIND_HG_SEM_DESTROY_PRE(_sem)
#define VALGRIND_HG_BARRIER_INIT_PRE(_bar, _count, _resizable)
#define VALGRIND_HG_BARRIER_WAIT_PRE(_bar)
#define VALGRIND_HG_BARRIER_RESIZE_PRE(_bar, _newcount)
#define VALGRIND_HG_BARRIER_DESTROY_PRE(_bar)
#define	VALGRIND_HG_CLEAN_MEMORY(_qzz_start, _qzz_len)
#define VALGRIND_HG_CLEAN_MEMORY_HEAPBLOCK(_qzz_blockstart)
#define VALGRIND_HG_DISABLE_CHECKING(_qzz_start, _qzz_len)
#define VALGRIND_HG_ENABLE_CHECKING(_qzz_start, _qzz_len)

#define DRD_GET_VALGRIND_THREADID
#define DRD_GET_DRD_THREADID
#define DRD_IGNORE_VAR(x)
#define DRD_STOP_IGNORING_VAR(x)
#define DRD_TRACE_VAR(x)

#define	ANNOTATE_HAPPENS_BEFORE(_obj)
#define	ANNOTATE_HAPPENS_AFTER(_obj)

#define	ANNOTATE_RWLOCK_CREATE(rwlock)
#define	ANNOTATE_RWLOCK_DESTROY(rwlock)
#define ANNOTATE_RWLOCK_ACQUIRED(rwlock, is_w)
#define ANNOTATE_READERLOCK_ACQUIRED(rwlock)
#define ANNOTATE_WRITERLOCK_ACQUIRED(rwlock)
#define ANNOTATE_RWLOCK_RELEASED(rwlock, is_w)
#define ANNOTATE_READERLOCK_RELEASED(rwlock)
#define ANNOTATE_WRITERLOCK_RELEASED(rwlock)

#define ANNOTATE_BARRIER_INIT(barrier, count, reinitialization_allowed)
#define ANNOTATE_BARRIER_DESTROY(barrier)
#define ANNOTATE_BARRIER_WAIT_BEFORE(barrier)
#define	ANNOTATE_BARRIER_WAIT_AFTER(barrier)

#define	ANNOTATE_BENIGN_RACE(_a, _b)
#define	ANNOTATE_BENIGN_RACE_SIZED(_a, _b, _c)

#define	ANNOTATE_IGNORE_READS_BEGIN()
#define	ANNOTATE_IGNORE_READS_END()
#define	ANNOTATE_IGNORE_WRITES_BEGIN()
#define	ANNOTATE_IGNORE_WRITES_END()
#define	ANNOTATE_IGNORE_READS_AND_WRITES_BEGIN()
#define	ANNOTATE_IGNORE_READS_AND_WRITES_END()

#define	ANNOTATE_NEW_MEMORY(_addr, _size)
#define	ANNOTATE_TRACE_MEMORY(_addr)
#define	ANNOTATE_THREAD_NAME(_name)

#define	DRD_xmalloc(_nb)		xmalloc(_nb)
#define	DRD_xcalloc(_nmemb, _size)	xcalloc(_nmemb, _size)
#define	DRD_xrealloc(_ptr, _size)	xrealloc(_ptr, _size)
#define	DRD_xstrdup(_str)		xstrdup(_str)

#endif

#endif	/* H_DEBUG */
