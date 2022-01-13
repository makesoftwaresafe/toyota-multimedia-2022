#include <linux/qnx.h>
#include <linux/linux.h>


static LIST_HEAD(shrinker_list);
#ifndef __QNXNTO__
static DECLARE_RWSEM(shrinker_rwsem);
#endif /* __QNXNTO__ */

pthread_mutex_t 		shr_mutex;
pthread_cond_t      	cv;
#if 0
static int              quit = 0;
#endif /* 0 */

/*
 * Add a shrinker callback to be called from the vm
 */
int register_shrinker(struct shrinker *shrinker)
{
#if 0 //TODO
	shrinker->nr_deferred = kzalloc(size, GFP_KERNEL);
	if (!shrinker->nr_deferred)
		return -ENOMEM;

	down_write(&shrinker_rwsem);
	list_add_tail(&shrinker->list, &shrinker_list);
	up_write(&shrinker_rwsem);

	atomic_set(&shrinker->nr_in_batch, 0);
	pthread_mutex_lock(&shr_mutex);
	list_add_tail(&shrinker->list, &shrinker_list);
	pthread_mutex_unlock(&shr_mutex);
#else
	atomic_set(&shrinker->nr_in_batch, 0);
	pthread_mutex_lock(&shr_mutex);
	list_add_tail(&shrinker->list, &shrinker_list);
	pthread_mutex_unlock(&shr_mutex);
	return 0;
#endif
}

/*
 * Remove one
 */
void unregister_shrinker(struct shrinker *shrinker)
{
#if 0
	pthread_mutex_lock(&shr_mutex);
	list_del(&shrinker->list);
	pthread_mutex_unlock(&shr_mutex);
#endif
}

static inline int do_shrinker_shrink(struct shrinker *shrinker,
				     struct shrink_control *sc,
				     unsigned long nr_to_scan)
{
#if 0
	sc->nr_to_scan = nr_to_scan;
	return (*shrinker->shrink)(shrinker, sc);
#else
	return 0;
#endif
}

#if 0
static void *sys_shrinker_thread (void *arg)
{
    int status, timedout;
    struct timespec timeout;

    workqueue_struct_t *wq;//FIXME: INTEGRATION
    work_struct_t *we;//FIXME: INTEGRATION

    status = pthread_mutex_lock (&shr_mutex);
    if (status != 0)
        return NULL;

    while (1) {
        timedout = 0;
        clock_gettime (CLOCK_REALTIME, &timeout);
        timeout.tv_sec += 1;

        while (shrinker_list.next == NULL && !quit) {
            status = pthread_cond_timedwait (
                    &cv, &shr_mutex, &timeout);
            if (status == ETIMEDOUT) {
                timedout = 1;
                break;
            } else if (status != 0) {
                pthread_mutex_unlock (&shr_mutex);
                return NULL;
            }
        }

        if (shrinker_list.next != NULL) {
            wq->first = we->next;
            if (wq->last == we)
                wq->last = NULL;
            status = pthread_mutex_unlock (&wq->mutex);
            if (status != 0)
                return NULL;
            we->func (we);
            free (we);
            status = pthread_mutex_lock (&wq->mutex);
            if (status != 0)
                return NULL;
        }

        if (wq->first == NULL && wq->quit) {
            wq->counter--;

            if (wq->counter == 0)
                pthread_cond_broadcast (&wq->cv);
            pthread_mutex_unlock (&wq->mutex);
            return NULL;
        }

        if (wq->first == NULL && timedout) {
            wq->counter--;
            break;
        }
    }

    pthread_mutex_unlock (&wq->mutex);
    return NULL;
}
#endif

void init_shrinker_task(void) {

}

void destroy_shrinker_task(void) {

}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/qnx/shrinker.c $ $Rev: 836322 $")
#endif
