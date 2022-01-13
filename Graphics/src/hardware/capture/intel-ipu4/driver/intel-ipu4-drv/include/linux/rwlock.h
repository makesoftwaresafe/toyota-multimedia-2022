/*
* Copyright (c) 2017 QNX Software Systems.
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version 2
* of the License, or (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#ifndef _QNX_LINUX_RWLOCK_H
#define _QNX_LINUX_RWLOCK_H

#include <pthread.h>
#include <assert.h>
#include <linux/debug.h>

typedef pthread_rwlock_t arch_rwlock_t;

typedef struct { 
	arch_rwlock_t raw_lock;
}rwlock_t;

#define RW_LOCK_UNLOCKED (rwlock_t) { .raw_lock = PTHREAD_RWLOCK_INITIALIZER, }
#define read_can_lock(rwlock)	TODO
#define write_can_lock(rwlock)  TODO

#define read_trylock(lock)	pthread_rwlock_tryrdlock(&(lock)->raw_lock)
#define write_trylock(lock)	pthread_rwlock_trywrlock(&(lock)->raw_lock)


#define write_lock(lock)	_raw_write_lock(lock)
#define read_lock(lock)		_raw_read_lock(lock)

#define rwlock_init(lock)	_rwlock_init(lock)
#define rwlock_destroy(lock) _rwlock_destroy(lock)

#define read_unlock(lock)  _raw_rw_unlock(lock)
#define write_unlock(lock) _raw_rw_unlock(lock)

static inline int
_rwlock_init(rwlock_t *lock){
	int rc = pthread_rwlock_init(&lock->raw_lock, NULL);
	if(rc){
		qnx_error("pthread_rwlock_init failed [rc=%d] ", rc);
	}
	assert(rc == 0);
	return rc;
}
static inline int
_rwlock_destroy(rwlock_t *lock){
	int rc = pthread_rwlock_destroy(&lock->raw_lock);
	if(rc){
		qnx_error("pthread_rwlock_destroy failed [rc=%d] ", rc);
	}
	assert(rc == 0);
	return rc;
}
static inline int
_raw_read_lock(rwlock_t *lock){
	int rc = pthread_rwlock_rdlock(&lock->raw_lock);
	if(rc){
		qnx_error("pthread_rwlock_rdlock failed [rc=%d]", rc);
	}
	assert(rc == 0);
	return rc;
}
static inline int
_raw_write_lock(rwlock_t *lock){
	int rc = pthread_rwlock_wrlock(&lock->raw_lock);
	if(rc){
		qnx_error("pthread_rwlock_wrlock failed [rc=%d]", rc);
	}
	assert(rc == 0);
	return rc;
}
static inline int
_raw_rw_unlock(rwlock_t *lock){
	int rc = pthread_rwlock_unlock(&(lock)->raw_lock);
	if(rc){
		qnx_error("pthread_rwlock_unlock failed [rc=%d]", rc);
	}
	assert(rc == 0);
	return rc;
}
#endif

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/capture/intel-ipu4/driver/intel-ipu4-drv/include/linux/rwlock.h $ $Rev: 838597 $")
#endif
