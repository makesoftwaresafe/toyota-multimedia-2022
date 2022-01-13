#include <linux/qnx.h>
#include <linux/linux.h>

#include "drmP.h"

#if defined(CONFIG_TRACE_QNX_MUTEX)
#define __MUTEX_DEBUG_COUNT 65536

struct __mutex_debug_info {
	void* address;
	const char* s1;
	int s2;
	const char* s3;
};

static int __mutex_total_count = 0;

struct __mutex_debug_info mdi[__MUTEX_DEBUG_COUNT];

void __mutex_debug_dump()
{
	int it;
	int mmax = __mutex_total_count;

	for (it = 0; it < mmax; it++) {
		if (mdi[it].address) {
			fprintf(stderr, "mutex[%d]: address=%p, location=%s():%d %s\n", it, mdi[it].address, mdi[it].s1, mdi[it].s2, mdi[it].s3);
		}
	}
}
#endif /* CONFIG_TRACE_QNX_MUTEX */

#if !defined(CONFIG_TRACE_QNX_MUTEX)
void mutex_init(struct mutex* m)
#else
void __mutex_init(struct mutex* m, const char* s1, int s2, const char* s3)
#endif /* !CONFIG_TRACE_QNX_MUTEX */
{
	int rc;

	rc = pthread_mutex_init(&m->mux, NULL);
	if (rc == EOK) {
		atomic_long_set(&m->owner, 0);
		atomic_long_set(&m->flag, 0);
		m->destroyed = 0;
	}
	BUG_ON(rc);

#if defined(CONFIG_TRACE_QNX_MUTEX)
	if (__mutex_total_count > (__MUTEX_DEBUG_COUNT - 1)) {
		fprintf(stderr, "mutex debug array overflow!\n");
		return;
	}
	mdi[__mutex_total_count].address = m;
	mdi[__mutex_total_count].s1 = s1;
	mdi[__mutex_total_count].s2 = s2;
	mdi[__mutex_total_count].s3 = s3;
	__mutex_total_count++;
#endif /* CONFIG_TRACE_QNX_MUTEX */
}

#if !defined(CONFIG_TRACE_QNX_MUTEX)
void _spin_lock_init(spinlock_t* spin)
#else
void _spin_lock_init(spinlock_t* spin, const char* s1, int s2, const char* s3)
#endif /* !CONFIG_TRACE_QNX_MUTEX */
{
	int rc;

	rc = pthread_spin_init(&spin->spinlock, PTHREAD_PROCESS_PRIVATE);
	BUG_ON(rc);
	spin->options = 0;

#if defined(CONFIG_TRACE_QNX_MUTEX)
	if (__mutex_total_count > (__MUTEX_DEBUG_COUNT - 1)) {
		fprintf(stderr, "mutex debug array overflow!\n");
		return;
	}
	mdi[__mutex_total_count].address = spin;
	mdi[__mutex_total_count].s1 = s1;
	mdi[__mutex_total_count].s2 = s2;
	mdi[__mutex_total_count].s3 = s3;
	__mutex_total_count++;
#endif /* CONFIG_TRACE_QNX_MUTEX */
}

void mutex_destroy(struct mutex* m)
{
	int rc;

	rc = pthread_mutex_destroy(&m->mux);
	if (rc == EOK) {
		m->destroyed = 1;
	}
	BUG_ON(rc);

#if defined(CONFIG_TRACE_QNX_MUTEX)
	{
		int it;
		int mmax = __mutex_total_count;
		int total_destroyed = 0;

		for (it = 0; it < mmax; it++) {
			if (mdi[it].address == m) {
				mdi[it].address = NULL;
				mdi[it].s1 = NULL;
				mdi[it].s2 = 0;
				mdi[it].s3 = NULL;
				total_destroyed++;
			}
		}

		if (total_destroyed > 1) {
			fprintf(stderr, "same mutex was created %d times!\n", total_destroyed);
			abort();
			return;
		}
		if (total_destroyed == 0) {
			fprintf(stderr, "attempt to destroy non-existing mutex!\n");
			abort();
			return;
		}
	}
#endif /* CONFIG_TRACE_QNX_MUTEX */
}

void __spin_lock_destroy(spinlock_t* spin)
{
	int rc;

	rc = pthread_spin_destroy(&spin->spinlock);
	BUG_ON(rc);

#if defined(CONFIG_TRACE_QNX_MUTEX)
	{
		int it;
		int mmax = __mutex_total_count;
		int total_destroyed = 0;

		for (it = 0; it < mmax; it++) {
			if (mdi[it].address == spin) {
				mdi[it].address = NULL;
				mdi[it].s1 = NULL;
				mdi[it].s2 = 0;
				mdi[it].s3 = NULL;
				total_destroyed++;
			}
		}

		if (total_destroyed > 1) {
			fprintf(stderr, "same spinlock was created %d times!\n", total_destroyed);
			abort();
			return;
		}
		if (total_destroyed == 0) {
			fprintf(stderr, "attempt to destroy non-existing spinlock!\n");
			abort();
			return;
		}
	}
#endif /* CONFIG_TRACE_QNX_MUTEX */
}

int mutex_trylock(struct mutex* m)
{
	BUG_ON(m->destroyed);

	if (atomic_long_cmpxchg(&m->owner, 0, (long)(uintptr_t)current) == 0) {
		int rc = pthread_mutex_trylock(&m->mux);
		if (rc == EOK) {
			return 1;
		}
	}
	return 0;
}

int mutex_lock(struct mutex* m)
{
	int rc = 0;

	BUG_ON(m->destroyed);

	if (atomic_long_cmpxchg(&m->owner, 0, (long)(uintptr_t)current) == 0) {
		rc = pthread_mutex_lock(&m->mux);
		if (rc != EOK) {
			BUG_ON(rc);
		}
	} else {
		rc = pthread_mutex_lock(&m->mux);
		if (rc == EOK) {
			atomic_long_set((atomic_long_t*)&m->owner, (long)(uintptr_t)current);
		} else {
			BUG_ON(rc);
		}
	}

	return rc;
}

void mutex_unlock(struct mutex* m)
{
	BUG_ON(m->destroyed);

	atomic_long_set(&m->owner, 0);
	pthread_mutex_unlock(&m->mux);
}

int mutex_is_locked(struct mutex* m)
{
	struct _sync* sync;

	BUG_ON(m->destroyed);

	if (m == NULL) {
		return false;
	}

	sync = (struct _sync*)&m->mux;

	return READ_ONCE(sync->__owner) ? 1 : 0 ;
}

bool mutex_is_locked_by(struct mutex* m,
		struct task_struct *task)
{
	struct _sync* sync;

	BUG_ON(m->destroyed);

	if (m == NULL) {
		return false;
	}

	sync = (struct _sync*)&m->mux;

	if (READ_ONCE(sync->__owner) && task == mutex_get_owner(m)) {
		return true;
	}
	return false;
}

/**
 * mutex_trylock_recursive - trylock variant that allows recursive locking
 * @lock: mutex to be locked
 *
 * This function should not be used, _ever_. It is purely for hysterical GEM
 * raisins, and once those are gone this will be removed.
 *
 * Returns:
 *  MUTEX_TRYLOCK_FAILED    - trylock failed,
 *  MUTEX_TRYLOCK_SUCCESS   - lock acquired,
 *  MUTEX_TRYLOCK_RECURSIVE - we already owned the lock.
 */
enum mutex_trylock_recursive_enum mutex_trylock_recursive(struct mutex *lock)
{
	if (unlikely(atomic_long_read(&lock->owner) == (uintptr_t)current))
		return MUTEX_TRYLOCK_RECURSIVE;

	return mutex_trylock(lock);
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/qnx/mutex.c $ $Rev: 864420 $")
#endif
