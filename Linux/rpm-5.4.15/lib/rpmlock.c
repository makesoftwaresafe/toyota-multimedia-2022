#include "system.h"

#include <rpmio.h>
#include <rpmiotypes.h>		/* XXX fnpyKey */
#include <rpmlog.h>
#include <rpmmacro.h>
#include <rpmtypes.h>
#include <rpmtag.h>

#include "rpmts.h"
#include "rpmlock.h"

#include "debug.h"

/*@unchecked@*/ /*@observer@*/
static const char * rpmlock_path_default = "%{?_rpmlock_path}";
/*@unchecked@*/ /*@only@*/ /*null@*/
static const char * rpmlock_path = NULL;

enum {
    RPMLOCK_READ   = 1 << 0,
    RPMLOCK_WRITE  = 1 << 1,
    RPMLOCK_WAIT   = 1 << 2,
};

typedef struct {
    int fd;
    int omode;
} * rpmlock;

#ifdef __cplusplus
GENfree(rpmlock)
#endif	/* __cplusplus */

/*@null@*/
static int rpmlock_new(/*@unused@*/ const char *rootdir, /*@null@*/ rpmlock *lockp)
	/*@globals rpmlock_path, rpmGlobalMacroContext, h_errno,
		fileSystem, internalState @*/
	/*@modifies *lockp, rpmlock_path, rpmGlobalMacroContext, h_errno,
		fileSystem, internalState @*/
{
    static int oneshot = 0;
    rpmlock lock = (rpmlock) xmalloc(sizeof(*lock));
    int rc = -1;

    if (lockp)
	*lockp = NULL;

    /* XXX oneshot to determine path for fcntl lock. */
    /* XXX rpmlock_path is set once, cannot be changed with %{_rpmlock_path}. */
    if (!oneshot) {
	const char * t = NULL;
	char * check = rpmExpand(rpmlock_path_default, NULL);
	if (check[0] != '\0')
	    t = rpmGenPath(rootdir, rpmlock_path_default, NULL);
	check = _free(check);

	if (t == NULL || *t == '\0' || *t == '%')
	    t = _free(t);
	rpmlock_path = t;
	oneshot++;
    }

    if (rpmlock_path != NULL) {
	mode_t oldmask = umask(022);
	lock->fd = open(rpmlock_path, O_RDWR|O_CREAT, 0644);
	(void) umask(oldmask);

	if (lock->fd == -1) {
	    lock->fd = open(rpmlock_path, O_RDONLY);
	    if (lock->fd == -1)
		rc = -1;
	    else
		lock->omode = RPMLOCK_READ;
	} else
	    lock->omode = RPMLOCK_WRITE | RPMLOCK_READ;
	rc = 0;
    }
    if (rc == 0 && lockp != NULL)
	*lockp = lock;
    else
	lock = _free(lock);
/*@-compdef@*/ /*@-nullstate@*/ /*@-globstate@*/
    return rc;
/*@=compdef@*/ /*@=nullstate@*/ /*@=globstate@*/
}

static rpmlock rpmlock_free(/*@only@*/ /*@null@*/ rpmlock lock)
	/*@globals fileSystem, internalState @*/
	/*@modifies lock, fileSystem, internalState @*/
{
    if (lock != NULL) {
	if (lock->fd > 0)
	    (void) close(lock->fd);
	lock = _free(lock);
    }
    return NULL;
}

static int rpmlock_acquire(/*@null@*/ rpmlock lock, int mode)
	/*@*/
{
    int rc = 0;

    if (lock != NULL && (mode & lock->omode)) {
	struct flock info;
	int cmd;
	if (mode & RPMLOCK_WAIT)
	    cmd = F_SETLKW;
	else
	    cmd = F_SETLK;
	if (mode & RPMLOCK_READ)
	    info.l_type = F_RDLCK;
	else
	    info.l_type = F_WRLCK;
	info.l_whence = SEEK_SET;
	info.l_start = 0;
	info.l_len = 0;
	info.l_pid = 0;
	if (fcntl(lock->fd, cmd, &info) != -1)
	    rc = 1;
    }
    return rc;
}

static int rpmlock_release(/*@null@*/ rpmlock lock)
	/*@globals internalState @*/
	/*@modifies internalState @*/
{
    int rc = 0;

    if (lock != NULL) {
	struct flock info;
	info.l_type = F_UNLCK;
	info.l_whence = SEEK_SET;
	info.l_start = 0;
	info.l_len = 0;
	info.l_pid = 0;
	rc = fcntl(lock->fd, F_SETLK, &info);
    }
    return rc;
}

void *rpmtsAcquireLock(rpmts ts)
{
    const char *rootDir = rpmtsRootDir(ts);
    rpmlock lock = NULL;
    int rc;

    if (rootDir == NULL || rpmtsChrootDone(ts))
	rootDir = "/";
    rc = rpmlock_new(rootDir, &lock);
    if (rc) {
	if (rpmlock_path != NULL && strcmp(rpmlock_path, rootDir))
	    rpmlog(RPMLOG_ERR,
		_("can't create transaction lock on %s (%s)\n"),
		rpmlock_path, strerror(errno));
    } else if (lock != NULL) {
	if (!rpmlock_acquire(lock, RPMLOCK_WRITE)) {
	    if (lock->omode & RPMLOCK_WRITE)
		rpmlog(RPMLOG_WARNING,
		   _("waiting for transaction lock on %s\n"), rpmlock_path);
	    if (!rpmlock_acquire(lock, RPMLOCK_WRITE|RPMLOCK_WAIT)) {
		if (rpmlock_path != NULL && strcmp(rpmlock_path, rootDir))
		    rpmlog(RPMLOG_ERR,
			_("can't create transaction lock on %s (%s)\n"),
			rpmlock_path, strerror(errno));
		lock = rpmlock_free(lock);
	    }
	}
    }
/*@-globstate@*/	/* rpmlock_path may be NULL. */
    return lock;
/*@=globstate@*/
}

void * rpmtsFreeLock(void *lock)
{
    (void) rpmlock_release((rpmlock)lock); /* Not really needed here. */
    lock = rpmlock_free((rpmlock)lock);
    return NULL;
}
