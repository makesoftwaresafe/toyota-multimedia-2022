#include <linux/qnx.h>
#include <linux/linux.h>

/*
 * lock for reading
 */
void down_read(struct rw_semaphore *sem)
{
#if 0
	might_sleep();
	rwsem_acquire_read(&sem->dep_map, 0, 0, _RET_IP_);

	LOCK_CONTENDED(sem, __down_read_trylock, __down_read);
#endif
}

EXPORT_SYMBOL(down_read);

/*
 * trylock for reading -- returns 1 if successful, 0 if contention
 */
int down_read_trylock(struct rw_semaphore *sem)
{
#if 0
	int ret = __down_read_trylock(sem);

	if (ret == 1)
		rwsem_acquire_read(&sem->dep_map, 0, 1, _RET_IP_);
	return ret;
#else
	return 0;
#endif
}

EXPORT_SYMBOL(down_read_trylock);

/*
 * lock for writing
 */
void down_write(struct rw_semaphore *sem)
{
#if 0
	might_sleep();
	rwsem_acquire(&sem->dep_map, 0, 0, _RET_IP_);

	LOCK_CONTENDED(sem, __down_write_trylock, __down_write);
#else
#endif
}

EXPORT_SYMBOL(down_write);

int down_write_killable(struct rw_semaphore *sem)
{
	return 0;
}

/*
 * trylock for writing -- returns 1 if successful, 0 if contention
 */
int down_write_trylock(struct rw_semaphore *sem)
{
#if 0
	int ret = __down_write_trylock(sem);

	if (ret == 1)
		rwsem_acquire(&sem->dep_map, 0, 1, _RET_IP_);
#else
	int ret = 0;
#endif
	return ret;
}

EXPORT_SYMBOL(down_write_trylock);

/*
 * release a read lock
 */
void up_read(struct rw_semaphore *sem)
{
#if 0 
	rwsem_release(&sem->dep_map, 1, _RET_IP_);

	__up_read(sem);
#endif
}

EXPORT_SYMBOL(up_read);

/*
 * release a write lock
 */
void up_write(struct rw_semaphore *sem)
{
#if 0
	rwsem_release(&sem->dep_map, 1, _RET_IP_);

	__up_write(sem);
#endif
}

EXPORT_SYMBOL(up_write);

/*
 * downgrade write lock to read lock
 */
void downgrade_write(struct rw_semaphore *sem)
{
#if 0
	/*
	 * lockdep: a downgraded write will live on as a write
	 * dependency.
	 */
	__downgrade_write(sem);
#endif
}

EXPORT_SYMBOL(downgrade_write);

//#ifdef CONFIG_DEBUG_LOCK_ALLOC

void down_read_nested(struct rw_semaphore *sem, int subclass)
{
#if 0
	might_sleep();
	rwsem_acquire_read(&sem->dep_map, subclass, 0, _RET_IP_);

	LOCK_CONTENDED(sem, __down_read_trylock, __down_read);
#endif
}

EXPORT_SYMBOL(down_read_nested);

void _down_write_nest_lock(struct rw_semaphore *sem, struct lockdep_map *nest)
{
#if 0
	might_sleep();
	rwsem_acquire_nest(&sem->dep_map, 0, 0, nest, _RET_IP_);

	LOCK_CONTENDED(sem, __down_write_trylock, __down_write);
#endif
}

EXPORT_SYMBOL(_down_write_nest_lock);

void down_read_non_owner(struct rw_semaphore *sem)
{
#if 0
	might_sleep();

	__down_read(sem);
#endif
}

EXPORT_SYMBOL(down_read_non_owner);

void down_write_nested(struct rw_semaphore *sem, int subclass)
{
#if 0
	might_sleep();
	rwsem_acquire(&sem->dep_map, subclass, 0, _RET_IP_);

	LOCK_CONTENDED(sem, __down_write_trylock, __down_write);
#endif
}

EXPORT_SYMBOL(down_write_nested);

void up_read_non_owner(struct rw_semaphore *sem)
{
#if 0
	__up_read(sem);
#endif
}

EXPORT_SYMBOL(up_read_non_owner);

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/qnx/rwsem.c $ $Rev: 836322 $")
#endif
