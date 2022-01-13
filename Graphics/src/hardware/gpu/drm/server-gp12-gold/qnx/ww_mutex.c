#include <linux/qnx.h>
#include <linux/linux.h>

static pthread_mutex_t ww_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t ww_cond = PTHREAD_COND_INITIALIZER;

static int
validae_locklist(struct ww_acquire_ctx *ctx)
{
	struct ww_mutex * pos;
	list_for_each_entry(pos, &ctx->locked_list, list){
		assert (pos && pos->ctx == ctx);
		if (ctx->stamp > pos->pending_stamp){
			return -1;
		}
	}
	return 0;
}

void
ww_acquire_init(struct ww_acquire_ctx *ctx, struct ww_class *ww_class)
{
	ctx->stamp = atomic_long_inc_return(&ww_class->stamp);
	ctx->acquired = 0;
	INIT_LIST_HEAD(&ctx->locked_list);
}

void
ww_acquire_fini(struct ww_acquire_ctx *ctx)
{
	/* assert (list_empty(&ctx->locked_list)); */
	INIT_LIST_HEAD(&ctx->locked_list);
}
void ww_acquire_done(struct ww_acquire_ctx *ctx)
{
}

void
ww_mutex_init(struct ww_mutex *lock,
			  struct ww_class *ww_class)
{
	(void)ww_class;
	mutex_init(&lock->base);
	lock->pending_stamp = -1;
	lock->ctx = NULL;
}

void ww_mutex_destroy(struct ww_mutex *lock)
{
	mutex_destroy(&lock->base);
}

int __must_check
__ww_mutex_lock(struct ww_mutex *lock,
			  struct ww_acquire_ctx *ctx)
{
	int rc = 0;
	while(1) {
		pthread_mutex_lock(&ww_mutex);
		if (lock->ctx && lock->ctx == ctx){
			pthread_mutex_unlock(&ww_mutex);
			return -EALREADY;
		}
		rc = mutex_trylock(&lock->base);
		if (1 == rc){
			INIT_LIST_HEAD(&lock->list);
			list_add_tail(&lock->list, &ctx->locked_list);
			lock->pending_stamp = -1;
			lock->ctx = ctx;

			ctx->acquired++;
			pthread_mutex_unlock(&ww_mutex);
			return 0;
		} else {
			assert (lock->ctx && lock->ctx != ctx && lock->ctx->stamp != ctx->stamp);
			if (!lock->ctx) {
				/* Should not happen in normal conditions */
				BUG_ON(!lock->ctx);
				pthread_mutex_unlock(&ww_mutex);
				return -EINVAL;
			}
			if (ctx->stamp < lock->ctx->stamp){
				lock->pending_stamp = ctx->stamp;
				rc = pthread_cond_broadcast(&ww_cond);
			}
			if (ctx->acquired > 0 && validae_locklist(ctx)){
				pthread_mutex_unlock(&ww_mutex);
				return -EDEADLK;
			}

			rc = pthread_cond_wait(&ww_cond, &ww_mutex);
			pthread_mutex_unlock(&ww_mutex);

			if(rc){
				break;
			}
		}
	}
	return rc;
}

void
__ww_mutex_unlock(struct ww_mutex *lock)
{
	struct ww_acquire_ctx * ctx = lock->ctx;
	pthread_mutex_lock(&ww_mutex);
	assert(ctx);

	mutex_unlock(&lock->base);
	lock->ctx = NULL;
	lock->pending_stamp = -1;
	list_del(&lock->list);
	INIT_LIST_HEAD(&lock->list);

	if (ctx->acquired > 0)
		--ctx->acquired;
	pthread_cond_broadcast(&ww_cond);
	pthread_mutex_unlock(&ww_mutex);
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/qnx/ww_mutex.c $ $Rev: 836322 $")
#endif
