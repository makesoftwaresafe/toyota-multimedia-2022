#ifndef _LINUX_REFCOUNT_H
#define _LINUX_REFCOUNT_H

#include <linux/atomic.h>
#include <linux/mutex.h>
#include <linux/spinlock.h>
#include <linux/kernel.h>

typedef struct refcount_struct {
	atomic_t refs;
} refcount_t;

#define REFCOUNT_INIT(n)	{ .refs = ATOMIC_INIT(n), }

static inline void refcount_set(refcount_t *r, unsigned int n)
{
	atomic_set(&r->refs, n);
}

static inline unsigned int refcount_read(const refcount_t *r)
{
	return atomic_read(&r->refs);
}

extern void refcount_sub(unsigned int i, refcount_t *r);

#if defined(CONFIG_REFCOUNT_FULL)
extern __must_check bool refcount_add_not_zero(unsigned int i, refcount_t *r);
extern void refcount_add(unsigned int i, refcount_t *r);
extern __must_check bool refcount_inc_not_zero(refcount_t *r);
extern void refcount_inc(refcount_t *r);
extern __must_check bool refcount_dec_and_test(refcount_t *r);
extern __must_check bool refcount_sub_and_test(unsigned int i, refcount_t *r);
extern void refcount_dec(refcount_t *r);
#else
static inline __must_check bool refcount_add_not_zero(unsigned int i, refcount_t *r)
{
	return atomic_add_unless(&r->refs, i, 0);
}

static inline void refcount_add(unsigned int i, refcount_t *r)
{
	atomic_add(i, &r->refs);
}

static inline __must_check bool refcount_inc_not_zero(refcount_t *r)
{
	return atomic_add_unless(&r->refs, 1, 0);
}

static inline void refcount_inc(refcount_t *r)
{
	atomic_inc(&r->refs);
}

static inline __must_check bool refcount_dec_and_test(refcount_t *r)
{
	return atomic_dec_and_test(&r->refs);
}

static inline __must_check bool refcount_sub_and_test(unsigned int i, refcount_t *r)
{
	return atomic_sub_and_test(i, &r->refs);
}

static inline void refcount_dec(refcount_t *r)
{
	atomic_dec(&r->refs);
}
#endif /* CONFIG_REFCOUNT_FULL */

extern __must_check bool refcount_dec_if_one(refcount_t *r);
extern __must_check bool refcount_dec_not_one(refcount_t *r);
extern __must_check bool refcount_dec_and_mutex_lock(refcount_t *r, struct mutex *lock);
extern __must_check bool refcount_dec_and_lock(refcount_t *r, spinlock_t *lock);

#endif /* _LINUX_REFCOUNT_H */

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/include/linux/refcount.h $ $Rev: 846821 $")
#endif
