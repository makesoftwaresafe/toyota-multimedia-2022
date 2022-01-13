/* This file is modified from the template file logio_template.  */

#include "system.h"

#include <rpmio_internal.h>	/* XXX fdInitDigest */

#include <db.h>

#include "db_int.h"

#include "dbinc/db_swap.h"

#include "logio.h"

#include "debug.h"

#undef	DEBUG_RECOVER
static int _debug = 0;

static const char * opnames[] = {
    "ABORT",
    "APPLY",
    "FIXME",
    "BACKWARD",
    "FORWARD",
    "OPENFILES",
    "POPENFILES",
    "PRINT"
};

static int writeFile(const char * fn, mode_t mode,
		const DBT * content, const DBT * digest, uint32_t dalgo)
{
    uint8_t * b = content->data;
    size_t blen = content->size;
    uint8_t * d = digest->data;
    size_t dlen = digest->size;
    int ret = 0;
    int xx;

if (_debug)
fprintf(stderr, "--> %s(%s,0%o,%p,%p,%u)\n", __FUNCTION__, fn, mode, content, digest, dalgo);
    if (S_ISREG(mode)) {
	FD_t fd = Fopen(fn, "w.fdio");
	size_t nw;

	if (dalgo > 0 && d && dlen > 0)
	    fdInitDigest(fd, dalgo, 0);
	nw = Fwrite(b, 1, blen, fd);

	xx = fsync(Fileno(fd));

	if (dalgo > 0 && d && dlen > 0) {
	    static int asAscii = 0;
	    void * digest = NULL;
	    size_t digestlen = 0;

	    (void) Fflush(fd);
	    fdFiniDigest(fd, dalgo, &digest, &digestlen, asAscii);
	    if (digest == NULL || digestlen != dlen || memcmp(digest, d, dlen))
		ret = -1;
	    digest = _free(digest);
	}
	(void) Fclose(fd);
	(void) Chmod(fn, mode);
	fd = NULL;
	if (nw != blen)
	    ret = -1;
    }
    return ret;
}

/*
 * logio_Creat_recover --
 *	Recovery function for Creat.
 *
 * PUBLIC: int logio_Creat_recover
 * PUBLIC:   __P((dbenv *, DBT *, DB_LSN *, db_recops));
 */
int
logio_Creat_recover(DB_ENV * dbenv, DBT * dbtp, DB_LSN * lsnp, db_recops op)
{
    logio_Creat_args *argp = NULL;
    int ret = EINVAL;

if (_debug)
fprintf(stderr, "--> %s(%p,%p,%p,%s)\n", __FUNCTION__, dbenv, dbtp, lsnp, opnames[op&7]);
#ifdef DEBUG_RECOVER
    (void)logio_Creat_print(dbenv, dbtp, lsnp, op);
#endif
    if ((ret = logio_Creat_read(dbenv, dbtp->data, &argp)) != 0)
	goto exit;

    switch (op) {
    case DB_TXN_ABORT:
    case DB_TXN_BACKWARD_ROLL:
	ret = Unlink(argp->path.data);
	if (ret != 0 && errno != ENOENT)
	    dbenv->err(dbenv, ret, "Creat: DB_TXN_BACKWARD_ROLL");
	else
	    ret = 0;
	break;
    case DB_TXN_APPLY:
    case DB_TXN_FORWARD_ROLL:
	ret = writeFile(argp->path.data, argp->mode, &argp->content,
				&argp->digest, argp->dalgo);
	if (ret != 0)
	    dbenv->err(dbenv, ret, "Creat: DB_TXN_FORWARD_ROLL");
	else
	    ret = 0;
    case DB_TXN_PRINT:
	if (ret != 0)
	    dbenv->err(dbenv, ret, "Creat: DB_TXN_PRINT");
	else
	    ret = 0;
	break;
    default:
	if (ret != 0)
	    dbenv->err(dbenv, ret, "Creat: UNKNOWN");
	else
	    ret = 0;
	break;
    }

    /* Allow for following LSN pointers through a transaction. */
    *lsnp = argp->prev_lsn;

exit:
    if (argp != NULL)
	free(argp);
    return ret;
}

/*
 * logio_Unlink_recover --
 *	Recovery function for Unlink.
 *
 * PUBLIC: int logio_Unlink_recover
 * PUBLIC:   __P((dbenv *, DBT *, DB_LSN *, db_recops));
 */
int
logio_Unlink_recover(DB_ENV * dbenv, DBT * dbtp, DB_LSN * lsnp, db_recops op)
{
    logio_Unlink_args *argp = NULL;
    int ret = EINVAL;

if (_debug)
fprintf(stderr, "--> %s(%p,%p,%p,%s)\n", __FUNCTION__, dbenv, dbtp, lsnp, opnames[op&7]);
#ifdef DEBUG_RECOVER
    (void)logio_Unlink_print(dbenv, dbtp, lsnp, op);
#endif
    if ((ret = logio_Unlink_read(dbenv, dbtp->data, &argp)) != 0)
	goto exit;

    switch (op) {
    case DB_TXN_ABORT:
    case DB_TXN_BACKWARD_ROLL:
	ret = writeFile(argp->path.data, argp->mode, &argp->content,
				&argp->digest, argp->dalgo);
	if (ret != 0)
	    dbenv->err(dbenv, ret, "Unlink: DB_TXN_BACKWARD_ROLL");
	else
	    ret = 0;
	break;
    case DB_TXN_APPLY:
    case DB_TXN_FORWARD_ROLL:
	ret = Unlink(argp->path.data);
	if (ret != 0 && errno != ENOENT)
	    dbenv->err(dbenv, ret, "Unlink: DB_TXN_FORWARD_ROLL");
	else
	    ret = 0;
    case DB_TXN_PRINT:
	if (ret != 0)
	    dbenv->err(dbenv, ret, "Unlink: DB_TXN_PRINT");
	else
	    ret = 0;
	break;
    default:
	if (ret != 0)
	    dbenv->err(dbenv, ret, "Unlink: UNKNOWN");
	else
	    ret = 0;
	break;
    }

    /* Allow for following LSN pointers through a transaction. */
    *lsnp = argp->prev_lsn;

exit:
    if (argp != NULL)
	free(argp);
    return ret;
}

/*
 * logio_Rename_recover --
 *	Recovery function for Rename.
 *
 * PUBLIC: int logio_Rename_recover
 * PUBLIC:   __P((dbenv *, DBT *, DB_LSN *, db_recops));
 */
int
logio_Rename_recover(DB_ENV * dbenv, DBT * dbtp, DB_LSN * lsnp, db_recops op)
{
    logio_Rename_args *argp = NULL;
    int ret = EINVAL;

if (_debug)
fprintf(stderr, "--> %s(%p,%p,%p,%s)\n", __FUNCTION__, dbenv, dbtp, lsnp, opnames[op&7]);
#ifdef DEBUG_RECOVER
    (void)logio_Rename_print(dbenv, dbtp, lsnp, op);
#endif
    if ((ret = logio_Rename_read(dbenv, dbtp->data, &argp)) != 0)
	goto exit;

    switch (op) {
    case DB_TXN_ABORT:
    case DB_TXN_BACKWARD_ROLL:
	ret = writeFile(argp->newpath.data, argp->mode, &argp->content,
				&argp->digest, argp->dalgo);
	ret = Rename(argp->newpath.data, argp->oldpath.data);
	if (ret != 0)
	    dbenv->err(dbenv, ret, "Rename: DB_TXN_BACKWARD_ROLL");
	else
	    ret = 0;
	break;
    case DB_TXN_APPLY:
    case DB_TXN_FORWARD_ROLL:
	ret = Rename(argp->oldpath.data, argp->newpath.data);
	if (ret != 0)
	    dbenv->err(dbenv, ret, "Rename: DB_TXN_FORWARD_ROLL");
	else
	    ret = 0;
    case DB_TXN_PRINT:
	if (ret != 0)
	    dbenv->err(dbenv, ret, "Rename: DB_TXN_PRINT");
	else
	    ret = 0;
	break;
    default:
	if (ret != 0)
	    dbenv->err(dbenv, ret, "Rename: UNKNOWN");
	else
	    ret = 0;
	break;
    }

    /* Allow for following LSN pointers through a transaction. */
    *lsnp = argp->prev_lsn;

exit:
    if (argp != NULL)
	free(argp);
    return ret;
}

int
logio_Mkdir_recover(DB_ENV * dbenv, DBT * dbtp, DB_LSN * lsnp, db_recops op)
{
    logio_Mkdir_args * argp = NULL;
    int ret = EINVAL;

if (_debug)
fprintf(stderr, "--> %s(%p,%p,%p,%s)\n", __FUNCTION__, dbenv, dbtp, lsnp, opnames[op&7]);
#ifdef DEBUG_RECOVER
    logio_Mkdir_print(dbenv, dbtp, lsnp, op);
#endif
    if ((ret = logio_Mkdir_read(dbenv, dbtp->data, &argp)) != 0)
	goto exit;

    switch (op) {
    case DB_TXN_ABORT:
    case DB_TXN_BACKWARD_ROLL:
	/*
	 * If we're aborting, we need to remove the directory if it
	 * exists.  We log the trailing zero in pathnames, so we can
	 * simply pass the data part of the DBT into Rmdir as a string.
	 * (Note that we don't have any alignment guarantees, but for
	 * a char * this doesn't matter.)
	 *
	 * Ignore all errors other than ENOENT;  DB may attempt to undo
	 * or redo operations without knowing whether they have already
	 * been done or undone, so we should never assume in a recovery
	 * function that the task definitely needs doing or undoing.
	 */
	ret = Rmdir(argp->path.data);
	if (ret != 0 && errno != ENOENT)
	    dbenv->err(dbenv, ret, "Mkdir: DB_TXN_BACKWARD_ROLL");
	else
	    ret = 0;
	break;
    case DB_TXN_APPLY:
    case DB_TXN_FORWARD_ROLL:
	/*
	 * The forward direction is just the opposite;  here, we ignore
	 * EEXIST, because the directory may already exist.
	 */
	ret = Mkdir(argp->path.data, argp->mode);
	if (ret != 0 && errno != EEXIST)
	    dbenv->err(dbenv, ret, "Mkdir: DB_TXN_FORWARD_ROLL");
	else
	    ret = 0;
	break;
    case DB_TXN_PRINT:
	if (ret != 0)
	    dbenv->err(dbenv, ret, "Mkdir: DB_TXN_PRINT");
	else
	    ret = 0;
	break;
    default:
	if (ret != 0)
	    dbenv->err(dbenv, ret, "Mkdir: UNKNOWN");
	else
	    ret = 0;
	break;
    }

   /*
    * The recovery function is responsible for returning the LSN of the
    * previous log record in this transaction, so that transaction aborts
    * can follow the chain backwards.
    *
    * (If we'd wanted the LSN of this record earlier, we could have
    * read it from lsnp, as well--but because we weren't working with
    * pages or other objects that store their LSN and base recovery
    * decisions on it, we didn't need to.)
    */
    *lsnp = argp->prev_lsn;

exit:
    if (argp != NULL)
	free(argp);
    return ret;
}

/*
 * logio_Rmdir_recover --
 *	Recovery function for Rmdir.
 *
 * PUBLIC: int logio_Rmdir_recover
 * PUBLIC:   __P((dbenv *, DBT *, DB_LSN *, db_recops));
 */
int
logio_Rmdir_recover(DB_ENV * dbenv, DBT * dbtp, DB_LSN * lsnp, db_recops op)
{
    logio_Rmdir_args *argp = NULL;
    int ret = EINVAL;

if (_debug)
fprintf(stderr, "--> %s(%p,%p,%p,%s)\n", __FUNCTION__, dbenv, dbtp, lsnp, opnames[op&7]);
#ifdef DEBUG_RECOVER
    (void)logio_Rmdir_print(dbenv, dbtp, lsnp, op);
#endif
    if ((ret = logio_Rmdir_read(dbenv, dbtp->data, &argp)) != 0)
	goto exit;

    switch (op) {
    case DB_TXN_ABORT:
    case DB_TXN_BACKWARD_ROLL:
	ret = Mkdir(argp->path.data, argp->mode);
	if (ret != 0 && errno != EEXIST)
	    dbenv->err(dbenv, ret, "Rmdir: DB_TXN_BACKWARD_ROLL");
	else
	    ret = 0;
	break;
    case DB_TXN_APPLY:
    case DB_TXN_FORWARD_ROLL:
	ret = Rmdir(argp->path.data);
	if (ret != 0 && errno != ENOENT)
	    dbenv->err(dbenv, ret, "Rmdir: DB_TXN_FORWARD_ROLL");
	else
	    ret = 0;
	break;
    case DB_TXN_PRINT:
	if (ret != 0)
	    dbenv->err(dbenv, ret, "Rmdir: DB_TXN_PRINT");
	else
	    ret = 0;
	break;
    default:
	if (ret != 0)
	    dbenv->err(dbenv, ret, "Rmdir: UNKNOWN");
	else
	    ret = 0;
	break;
    }

    /* Allow for following LSN pointers through a transaction. */
    *lsnp = argp->prev_lsn;

exit:
    if (argp != NULL)
	free(argp);
    return ret;
}

#ifdef	NOTYET
/*
 * logio_Lsetfilecon_recover --
 *	Recovery function for Lsetfilecon.
 *
 * PUBLIC: int logio_Lsetfilecon_recover
 * PUBLIC:   __P((dbenv *, DBT *, DB_LSN *, db_recops));
 */
int
logio_Lsetfilecon_recover(DB_ENV * dbenv, DBT * dbtp, DB_LSN * lsnp, db_recops op)
{
    logio_Lsetfilecon_args *argp = NULL;
    int ret = EINVAL;

if (_debug)
fprintf(stderr, "--> %s(%p,%p,%p,%s)\n", __FUNCTION__, dbenv, dbtp, lsnp, opnames[op&7]);
#ifdef DEBUG_RECOVER
    (void)logio_Lsetfilecon_print(dbenv, dbtp, lsnp, op);
#endif
    if ((ret = logio_Lsetfilecon_read(dbenv, dbtp->data, &argp)) != 0)
	goto exit;

    switch (op) {
    case DB_TXN_ABORT:
    case DB_TXN_BACKWARD_ROLL:
#ifdef	NOTYET
	ret = UNLsetfilecon(argp->path.data);
#else
	ret = 0;
#endif
	if (ret != 0)
	    dbenv->err(dbenv, ret, "Lsetfilecon: DB_TXN_BACKWARD_ROLL");
	else
	    ret = 0;
	break;
    case DB_TXN_APPLY:
    case DB_TXN_FORWARD_ROLL:
	ret = Lsetfilecon(argp->path.data, argp->context.data);
	if (ret != 0)
	    dbenv->err(dbenv, ret, "Lsetfilecon: DB_TXN_FORWARD_ROLL");
	else
	    ret = 0;
    case DB_TXN_PRINT:
	if (ret != 0)
	    dbenv->err(dbenv, ret, "Lsetfilecon: DB_TXN_PRINT");
	else
	    ret = 0;
	break;
    default:
	if (ret != 0)
	    dbenv->err(dbenv, ret, "Lsetfilecon: UNKNOWN");
	else
	    ret = 0;
	break;
    }

    /* Allow for following LSN pointers through a transaction. */
    *lsnp = argp->prev_lsn;

exit:
    if (argp != NULL)
	free(argp);
    return ret;
}
#endif

/*
 * logio_Chown_recover --
 *	Recovery function for Chown.
 *
 * PUBLIC: int logio_Chown_recover
 * PUBLIC:   __P((dbenv *, DBT *, DB_LSN *, db_recops));
 */
int
logio_Chown_recover(DB_ENV * dbenv, DBT * dbtp, DB_LSN * lsnp, db_recops op)
{
    logio_Chown_args *argp = NULL;
    int ret = EINVAL;

if (_debug)
fprintf(stderr, "--> %s(%p,%p,%p,%s)\n", __FUNCTION__, dbenv, dbtp, lsnp, opnames[op&7]);
#ifdef DEBUG_RECOVER
    (void)logio_Chown_print(dbenv, dbtp, lsnp, op);
#endif
    if ((ret = logio_Chown_read(dbenv, dbtp->data, &argp)) != 0)
	goto exit;

    switch (op) {
    case DB_TXN_ABORT:
    case DB_TXN_BACKWARD_ROLL:
#ifdef	NOTYET
	ret = UNChown(argp->path.data);
#else
	ret = 0;
#endif
	if (ret != 0)
	    dbenv->err(dbenv, ret, "Chown: DB_TXN_BACKWARD_ROLL");
	else
	    ret = 0;
	break;
    case DB_TXN_APPLY:
    case DB_TXN_FORWARD_ROLL:
	ret = Chown(argp->path.data, argp->owner, argp->group);
	if (ret != 0)
	    dbenv->err(dbenv, ret, "Chown: DB_TXN_FORWARD_ROLL");
	else
	    ret = 0;
    case DB_TXN_PRINT:
	if (ret != 0)
	    dbenv->err(dbenv, ret, "Chown: DB_TXN_PRINT");
	else
	    ret = 0;
	break;
    default:
	if (ret != 0)
	    dbenv->err(dbenv, ret, "Chown: UNKNOWN");
	else
	    ret = 0;
	break;
    }

    /* Allow for following LSN pointers through a transaction. */
    *lsnp = argp->prev_lsn;

exit:
    if (argp != NULL)
	free(argp);
    return ret;
}

/*
 * logio_Lchown_recover --
 *	Recovery function for Lchown.
 *
 * PUBLIC: int logio_Lchown_recover
 * PUBLIC:   __P((dbenv *, DBT *, DB_LSN *, db_recops));
 */
int
logio_Lchown_recover(DB_ENV * dbenv, DBT * dbtp, DB_LSN * lsnp, db_recops op)
{
    logio_Lchown_args *argp = NULL;
    int ret = EINVAL;

if (_debug)
fprintf(stderr, "--> %s(%p,%p,%p,%s)\n", __FUNCTION__, dbenv, dbtp, lsnp, opnames[op&7]);
#ifdef DEBUG_RECOVER
    (void)logio_Lchown_print(dbenv, dbtp, lsnp, op);
#endif
    if ((ret = logio_Lchown_read(dbenv, dbtp->data, &argp)) != 0)
	goto exit;

    switch (op) {
    case DB_TXN_ABORT:
    case DB_TXN_BACKWARD_ROLL:
#ifdef	NOTYET
	ret = UNLchown(argp->path.data);
#else
	ret = 0;
#endif
	if (ret != 0)
	    dbenv->err(dbenv, ret, "Lchown: DB_TXN_BACKWARD_ROLL");
	else
	    ret = 0;
	break;
    case DB_TXN_APPLY:
    case DB_TXN_FORWARD_ROLL:
	ret = Lchown(argp->path.data, argp->owner, argp->group);
	if (ret != 0)
	    dbenv->err(dbenv, ret, "Lchown: DB_TXN_FORWARD_ROLL");
	else
	    ret = 0;
    case DB_TXN_PRINT:
	if (ret != 0)
	    dbenv->err(dbenv, ret, "Lchown: DB_TXN_PRINT");
	else
	    ret = 0;
	break;
    default:
	if (ret != 0)
	    dbenv->err(dbenv, ret, "Lchown: UNKNOWN");
	else
	    ret = 0;
	break;
    }

    /* Allow for following LSN pointers through a transaction. */
    *lsnp = argp->prev_lsn;

exit:
    if (argp != NULL)
	free(argp);
    return ret;
}

/*
 * logio_Chmod_recover --
 *	Recovery function for Chmod.
 *
 * PUBLIC: int logio_Chmod_recover
 * PUBLIC:   __P((dbenv *, DBT *, DB_LSN *, db_recops));
 */
int
logio_Chmod_recover(DB_ENV * dbenv, DBT * dbtp, DB_LSN * lsnp, db_recops op)
{
    logio_Chmod_args *argp = NULL;
    int ret = EINVAL;

if (_debug)
fprintf(stderr, "--> %s(%p,%p,%p,%s)\n", __FUNCTION__, dbenv, dbtp, lsnp, opnames[op&7]);
#ifdef DEBUG_RECOVER
    (void)logio_Chmod_print(dbenv, dbtp, lsnp, op);
#endif
    if ((ret = logio_Chmod_read(dbenv, dbtp->data, &argp)) != 0)
	goto exit;

    switch (op) {
    case DB_TXN_ABORT:
    case DB_TXN_BACKWARD_ROLL:
#ifdef	NOTYET
	ret = UNChmod(argp->path.data);
#else
	ret = 0;
#endif
	if (ret != 0)
	    dbenv->err(dbenv, ret, "Chmod: DB_TXN_BACKWARD_ROLL");
	else
	    ret = 0;
	break;
    case DB_TXN_APPLY:
    case DB_TXN_FORWARD_ROLL:
	ret = Chmod(argp->path.data, argp->mode);
	if (ret != 0)
	    dbenv->err(dbenv, ret, "Chmod: DB_TXN_FORWARD_ROLL");
	else
	    ret = 0;
    case DB_TXN_PRINT:
	if (ret != 0)
	    dbenv->err(dbenv, ret, "Chmod: DB_TXN_PRINT");
	else
	    ret = 0;
	break;
    default:
	if (ret != 0)
	    dbenv->err(dbenv, ret, "Chmod: UNKNOWN");
	else
	    ret = 0;
	break;
    }

    /* Allow for following LSN pointers through a transaction. */
    *lsnp = argp->prev_lsn;

exit:
    if (argp != NULL)
	free(argp);
    return ret;
}

/*
 * logio_Utime_recover --
 *	Recovery function for Utime.
 *
 * PUBLIC: int logio_Utime_recover
 * PUBLIC:   __P((dbenv *, DBT *, DB_LSN *, db_recops));
 */
int
logio_Utime_recover(DB_ENV * dbenv, DBT * dbtp, DB_LSN * lsnp, db_recops op)
{
    logio_Utime_args *argp = NULL;
    int ret = EINVAL;

if (_debug)
fprintf(stderr, "--> %s(%p,%p,%p,%s)\n", __FUNCTION__, dbenv, dbtp, lsnp, opnames[op&7]);
#ifdef DEBUG_RECOVER
    (void)logio_Utime_print(dbenv, dbtp, lsnp, op);
#endif
    if ((ret = logio_Utime_read(dbenv, dbtp->data, &argp)) != 0)
	goto exit;

    switch (op) {
    case DB_TXN_ABORT:
    case DB_TXN_BACKWARD_ROLL:
#ifdef	NOTYET
	ret = UNUtime(argp->path.data);
#else
	ret = 0;
#endif
	if (ret != 0)
	    dbenv->err(dbenv, ret, "Utime: DB_TXN_BACKWARD_ROLL");
	else
	    ret = 0;
	break;
    case DB_TXN_APPLY:
    case DB_TXN_FORWARD_ROLL:
	{   struct utimbuf times;
	    times.actime = argp->actime;
	    times.modtime = argp->modtime;
	    ret = Utime(argp->path.data, &times);
	}
	if (ret != 0)
	    dbenv->err(dbenv, ret, "Utime: DB_TXN_FORWARD_ROLL");
	else
	    ret = 0;
    case DB_TXN_PRINT:
	if (ret != 0)
	    dbenv->err(dbenv, ret, "Utime: DB_TXN_PRINT");
	else
	    ret = 0;
	break;
    default:
	if (ret != 0)
	    dbenv->err(dbenv, ret, "Utime: UNKNOWN");
	else
	    ret = 0;
	break;
    }

    /* Allow for following LSN pointers through a transaction. */
    *lsnp = argp->prev_lsn;

exit:
    if (argp != NULL)
	free(argp);
    return ret;
}

/*
 * logio_Symlink_recover --
 *	Recovery function for Symlink.
 *
 * PUBLIC: int logio_Symlink_recover
 * PUBLIC:   __P((dbenv *, DBT *, DB_LSN *, db_recops));
 */
int
logio_Symlink_recover(DB_ENV * dbenv, DBT * dbtp, DB_LSN * lsnp, db_recops op)
{
    logio_Symlink_args *argp = NULL;
    int ret = EINVAL;

if (_debug)
fprintf(stderr, "--> %s(%p,%p,%p,%s)\n", __FUNCTION__, dbenv, dbtp, lsnp, opnames[op&7]);
#ifdef DEBUG_RECOVER
    (void)logio_Symlink_print(dbenv, dbtp, lsnp, op);
#endif
    if ((ret = logio_Symlink_read(dbenv, dbtp->data, &argp)) != 0)
	goto exit;

    switch (op) {
    case DB_TXN_ABORT:
    case DB_TXN_BACKWARD_ROLL:
	ret = Unlink(argp->newpath.data);
	if (ret != 0)
	    dbenv->err(dbenv, ret, "Symlink: DB_TXN_BACKWARD_ROLL");
	else
	    ret = 0;
	break;
    case DB_TXN_APPLY:
    case DB_TXN_FORWARD_ROLL:
	ret = Symlink(argp->oldpath.data, argp->newpath.data);
	if (ret != 0)
	    dbenv->err(dbenv, ret, "Symlink: DB_TXN_FORWARD_ROLL");
	else
	    ret = 0;
    case DB_TXN_PRINT:
	if (ret != 0)
	    dbenv->err(dbenv, ret, "Symlink: DB_TXN_PRINT");
	else
	    ret = 0;
	break;
    default:
	if (ret != 0)
	    dbenv->err(dbenv, ret, "Symlink: UNKNOWN");
	else
	    ret = 0;
	break;
    }

    /* Allow for following LSN pointers through a transaction. */
    *lsnp = argp->prev_lsn;

exit:
    if (argp != NULL)
	free(argp);
    return ret;
}

/*
 * logio_Link_recover --
 *	Recovery function for Link.
 *
 * PUBLIC: int logio_Link_recover
 * PUBLIC:   __P((dbenv *, DBT *, DB_LSN *, db_recops));
 */
int
logio_Link_recover(DB_ENV * dbenv, DBT * dbtp, DB_LSN * lsnp, db_recops op)
{
    logio_Link_args *argp = NULL;
    int ret = EINVAL;

if (_debug)
fprintf(stderr, "--> %s(%p,%p,%p,%s)\n", __FUNCTION__, dbenv, dbtp, lsnp, opnames[op&7]);
#ifdef DEBUG_RECOVER
    (void)logio_Link_print(dbenv, dbtp, lsnp, op);
#endif
    if ((ret = logio_Link_read(dbenv, dbtp->data, &argp)) != 0)
	goto exit;

    switch (op) {
    case DB_TXN_ABORT:
    case DB_TXN_BACKWARD_ROLL:
	ret = Unlink(argp->newpath.data);
	if (ret != 0)
	    dbenv->err(dbenv, ret, "Link: DB_TXN_BACKWARD_ROLL");
	else
	    ret = 0;
	break;
    case DB_TXN_APPLY:
    case DB_TXN_FORWARD_ROLL:
	ret = Link(argp->oldpath.data, argp->newpath.data);
	if (ret != 0)
	    dbenv->err(dbenv, ret, "Link: DB_TXN_FORWARD_ROLL");
	else
	    ret = 0;
    case DB_TXN_PRINT:
	if (ret != 0)
	    dbenv->err(dbenv, ret, "Link: DB_TXN_PRINT");
	else
	    ret = 0;
	break;
    default:
	if (ret != 0)
	    dbenv->err(dbenv, ret, "Link: UNKNOWN");
	else
	    ret = 0;
	break;
    }

    /* Allow for following LSN pointers through a transaction. */
    *lsnp = argp->prev_lsn;

exit:
    if (argp != NULL)
	free(argp);
    return ret;
}

/*
 * logio_Mknod_recover --
 *	Recovery function for Mknod.
 *
 * PUBLIC: int logio_Mknod_recover
 * PUBLIC:   __P((dbenv *, DBT *, DB_LSN *, db_recops));
 */
int
logio_Mknod_recover(DB_ENV * dbenv, DBT * dbtp, DB_LSN * lsnp, db_recops op)
{
    logio_Mknod_args *argp = NULL;
    int ret = EINVAL;

if (_debug)
fprintf(stderr, "--> %s(%p,%p,%p,%s)\n", __FUNCTION__, dbenv, dbtp, lsnp, opnames[op&7]);
#ifdef DEBUG_RECOVER
    (void)logio_Mknod_print(dbenv, dbtp, lsnp, op);
#endif
    if ((ret = logio_Mknod_read(dbenv, dbtp->data, &argp)) != 0)
	goto exit;

    switch (op) {
    case DB_TXN_ABORT:
    case DB_TXN_BACKWARD_ROLL:
	ret = Unlink(argp->path.data);
	if (ret != 0)
	    dbenv->err(dbenv, ret, "Mknod: DB_TXN_BACKWARD_ROLL");
	else
	    ret = 0;
	break;
    case DB_TXN_APPLY:
    case DB_TXN_FORWARD_ROLL:
	ret = Mknod(argp->path.data, argp->mode, argp->dev);
	if (ret != 0)
	    dbenv->err(dbenv, ret, "Mknod: DB_TXN_FORWARD_ROLL");
	else
	    ret = 0;
    case DB_TXN_PRINT:
	if (ret != 0)
	    dbenv->err(dbenv, ret, "Mknod: DB_TXN_PRINT");
	else
	    ret = 0;
	break;
    default:
	if (ret != 0)
	    dbenv->err(dbenv, ret, "Mknod: UNKNOWN");
	else
	    ret = 0;
	break;
    }

    /* Allow for following LSN pointers through a transaction. */
    *lsnp = argp->prev_lsn;

exit:
    if (argp != NULL)
	free(argp);
    return ret;
}

/*
 * logio_Mkfifo_recover --
 *	Recovery function for Mkfifo.
 *
 * PUBLIC: int logio_Mkfifo_recover
 * PUBLIC:   __P((dbenv *, DBT *, DB_LSN *, db_recops));
 */
int
logio_Mkfifo_recover(DB_ENV * dbenv, DBT * dbtp, DB_LSN * lsnp, db_recops op)
{
    logio_Mkfifo_args *argp = NULL;
    int ret = EINVAL;

if (_debug)
fprintf(stderr, "--> %s(%p,%p,%p,%s)\n", __FUNCTION__, dbenv, dbtp, lsnp, opnames[op&7]);
#ifdef DEBUG_RECOVER
    (void)logio_Mkfifo_print(dbenv, dbtp, lsnp, op);
#endif
    if ((ret = logio_Mkfifo_read(dbenv, dbtp->data, &argp)) != 0)
	goto exit;

    switch (op) {
    case DB_TXN_ABORT:
    case DB_TXN_BACKWARD_ROLL:
	ret = Unlink(argp->path.data);
	if (ret != 0)
	    dbenv->err(dbenv, ret, "Mkfifo: DB_TXN_BACKWARD_ROLL");
	else
	    ret = 0;
	break;
    case DB_TXN_APPLY:
    case DB_TXN_FORWARD_ROLL:
	ret = Mkfifo(argp->path.data, argp->mode);
	if (ret != 0)
	    dbenv->err(dbenv, ret, "Mkfifo: DB_TXN_FORWARD_ROLL");
	else
	    ret = 0;
    case DB_TXN_PRINT:
	if (ret != 0)
	    dbenv->err(dbenv, ret, "Mkfifo: DB_TXN_PRINT");
	else
	    ret = 0;
	break;
    default:
	if (ret != 0)
	    dbenv->err(dbenv, ret, "Mkfifo: UNKNOWN");
	else
	    ret = 0;
	break;
    }

    /* Allow for following LSN pointers through a transaction. */
    *lsnp = argp->prev_lsn;

exit:
    if (argp != NULL)
	free(argp);
    return ret;
}

/* ============================================================== */
/*
 * logio_Pretrans_recover --
 *	Recovery function for Pretrans.
 *
 * PUBLIC: int logio_Pretrans_recover
 * PUBLIC:   __P((dbenv *, DBT *, DB_LSN *, db_recops));
 */
int
logio_Pretrans_recover(DB_ENV * dbenv, DBT * dbtp, DB_LSN * lsnp, db_recops op)
{
    logio_Pretrans_args *argp = NULL;
    int ret = EINVAL;

#ifdef DEBUG_RECOVER
    (void)logio_Pretrans_print(dbenv, dbtp, lsnp, op);
#endif
    if ((ret = logio_Pretrans_read(dbenv, dbtp->data, &argp)) != 0)
	goto exit;

    switch (op) {
    case DB_TXN_ABORT:
    case DB_TXN_BACKWARD_ROLL:
#ifdef	NOTYET
	ret = UNPretrans(argp->path.data);
#endif
	if (ret != 0)
	    dbenv->err(dbenv, ret, "Pretrans: DB_TXN_BACKWARD_ROLL");
	else
	    ret = 0;
	break;
    case DB_TXN_APPLY:
    case DB_TXN_FORWARD_ROLL:
#ifdef	NOTYET
	ret = Pretrans(argp->path.data);
#endif
	if (ret != 0)
	    dbenv->err(dbenv, ret, "Pretrans: DB_TXN_FORWARD_ROLL");
	else
	    ret = 0;
    case DB_TXN_PRINT:
	if (ret != 0)
	    dbenv->err(dbenv, ret, "Pretrans: DB_TXN_PRINT");
	else
	    ret = 0;
	break;
    default:
	if (ret != 0)
	    dbenv->err(dbenv, ret, "Pretrans: UNKNOWN");
	else
	    ret = 0;
	break;
    }

    /* Allow for following LSN pointers through a transaction. */
    *lsnp = argp->prev_lsn;

exit:
    if (argp != NULL)
	free(argp);
    return ret;
}

/*
 * logio_Prein_recover --
 *	Recovery function for Prein.
 *
 * PUBLIC: int logio_Prein_recover
 * PUBLIC:   __P((dbenv *, DBT *, DB_LSN *, db_recops));
 */
int
logio_Prein_recover(DB_ENV * dbenv, DBT * dbtp, DB_LSN * lsnp, db_recops op)
{
    logio_Prein_args *argp = NULL;
    int ret = EINVAL;

#ifdef DEBUG_RECOVER
    (void)logio_Prein_print(dbenv, dbtp, lsnp, op);
#endif
    if ((ret = logio_Prein_read(dbenv, dbtp->data, &argp)) != 0)
	goto exit;

    switch (op) {
    case DB_TXN_ABORT:
    case DB_TXN_BACKWARD_ROLL:
#ifdef	NOTYET
	ret = UNPrein(argp->path.data);
#endif
	if (ret != 0)
	    dbenv->err(dbenv, ret, "Prein: DB_TXN_BACKWARD_ROLL");
	else
	    ret = 0;
	break;
    case DB_TXN_APPLY:
    case DB_TXN_FORWARD_ROLL:
#ifdef	NOTYET
	ret = Prein(argp->path.data);
#endif
	if (ret != 0)
	    dbenv->err(dbenv, ret, "Prein: DB_TXN_FORWARD_ROLL");
	else
	    ret = 0;
    case DB_TXN_PRINT:
	if (ret != 0)
	    dbenv->err(dbenv, ret, "Prein: DB_TXN_PRINT");
	else
	    ret = 0;
	break;
    default:
	if (ret != 0)
	    dbenv->err(dbenv, ret, "Prein: UNKNOWN");
	else
	    ret = 0;
	break;
    }

    /* Allow for following LSN pointers through a transaction. */
    *lsnp = argp->prev_lsn;

exit:
    if (argp != NULL)
	free(argp);
    return ret;
}

/*
 * logio_Postin_recover --
 *	Recovery function for Postin.
 *
 * PUBLIC: int logio_Postin_recover
 * PUBLIC:   __P((dbenv *, DBT *, DB_LSN *, db_recops));
 */
int
logio_Postin_recover(DB_ENV * dbenv, DBT * dbtp, DB_LSN * lsnp, db_recops op)
{
    logio_Postin_args *argp = NULL;
    int ret = EINVAL;

#ifdef DEBUG_RECOVER
    (void)logio_Postin_print(dbenv, dbtp, lsnp, op);
#endif
    if ((ret = logio_Postin_read(dbenv, dbtp->data, &argp)) != 0)
	goto exit;

    switch (op) {
    case DB_TXN_ABORT:
    case DB_TXN_BACKWARD_ROLL:
#ifdef	NOTYET
	ret = UNPostin(argp->path.data);
#endif
	if (ret != 0)
	    dbenv->err(dbenv, ret, "Postin: DB_TXN_BACKWARD_ROLL");
	else
	    ret = 0;
	break;
    case DB_TXN_APPLY:
    case DB_TXN_FORWARD_ROLL:
#ifdef	NOTYET
	ret = Postin(argp->path.data);
#endif
	if (ret != 0)
	    dbenv->err(dbenv, ret, "Postin: DB_TXN_FORWARD_ROLL");
	else
	    ret = 0;
    case DB_TXN_PRINT:
	if (ret != 0)
	    dbenv->err(dbenv, ret, "Postin: DB_TXN_PRINT");
	else
	    ret = 0;
	break;
    default:
	if (ret != 0)
	    dbenv->err(dbenv, ret, "Postin: UNKNOWN");
	else
	    ret = 0;
	break;
    }

    /* Allow for following LSN pointers through a transaction. */
    *lsnp = argp->prev_lsn;

exit:
    if (argp != NULL)
	free(argp);
    return ret;
}

/*
 * logio_Preun_recover --
 *	Recovery function for Preun.
 *
 * PUBLIC: int logio_Preun_recover
 * PUBLIC:   __P((dbenv *, DBT *, DB_LSN *, db_recops));
 */
int
logio_Preun_recover(DB_ENV * dbenv, DBT * dbtp, DB_LSN * lsnp, db_recops op)
{
    logio_Preun_args *argp = NULL;
    int ret = EINVAL;

#ifdef DEBUG_RECOVER
    (void)logio_Preun_print(dbenv, dbtp, lsnp, op);
#endif
    if ((ret = logio_Preun_read(dbenv, dbtp->data, &argp)) != 0)
	goto exit;

    switch (op) {
    case DB_TXN_ABORT:
    case DB_TXN_BACKWARD_ROLL:
#ifdef	NOTYET
	ret = UNPreun(argp->path.data);
#endif
	if (ret != 0)
	    dbenv->err(dbenv, ret, "Preun: DB_TXN_BACKWARD_ROLL");
	else
	    ret = 0;
	break;
    case DB_TXN_APPLY:
    case DB_TXN_FORWARD_ROLL:
#ifdef	NOTYET
	ret = Preun(argp->path.data);
#endif
	if (ret != 0)
	    dbenv->err(dbenv, ret, "Preun: DB_TXN_FORWARD_ROLL");
	else
	    ret = 0;
    case DB_TXN_PRINT:
	if (ret != 0)
	    dbenv->err(dbenv, ret, "Preun: DB_TXN_PRINT");
	else
	    ret = 0;
	break;
    default:
	if (ret != 0)
	    dbenv->err(dbenv, ret, "Preun: UNKNOWN");
	else
	    ret = 0;
	break;
    }

    /* Allow for following LSN pointers through a transaction. */
    *lsnp = argp->prev_lsn;

exit:
    if (argp != NULL)
	free(argp);
    return ret;
}

/*
 * logio_Postun_recover --
 *	Recovery function for Postun.
 *
 * PUBLIC: int logio_Postun_recover
 * PUBLIC:   __P((dbenv *, DBT *, DB_LSN *, db_recops));
 */
int
logio_Postun_recover(DB_ENV * dbenv, DBT * dbtp, DB_LSN * lsnp, db_recops op)
{
    logio_Postun_args *argp = NULL;
    int ret = EINVAL;

#ifdef DEBUG_RECOVER
    (void)logio_Postun_print(dbenv, dbtp, lsnp, op);
#endif
    if ((ret = logio_Postun_read(dbenv, dbtp->data, &argp)) != 0)
	goto exit;

    switch (op) {
    case DB_TXN_ABORT:
    case DB_TXN_BACKWARD_ROLL:
#ifdef	NOTYET
	ret = UNPostun(argp->path.data);
#endif
	if (ret != 0)
	    dbenv->err(dbenv, ret, "Postun: DB_TXN_BACKWARD_ROLL");
	else
	    ret = 0;
	break;
    case DB_TXN_APPLY:
    case DB_TXN_FORWARD_ROLL:
#ifdef	NOTYET
	ret = Postun(argp->path.data);
#endif
	if (ret != 0)
	    dbenv->err(dbenv, ret, "Postun: DB_TXN_FORWARD_ROLL");
	else
	    ret = 0;
    case DB_TXN_PRINT:
	if (ret != 0)
	    dbenv->err(dbenv, ret, "Postun: DB_TXN_PRINT");
	else
	    ret = 0;
	break;
    default:
	if (ret != 0)
	    dbenv->err(dbenv, ret, "Postun: UNKNOWN");
	else
	    ret = 0;
	break;
    }

    /* Allow for following LSN pointers through a transaction. */
    *lsnp = argp->prev_lsn;

exit:
    if (argp != NULL)
	free(argp);
    return ret;
}

/*
 * logio_Posttrans_recover --
 *	Recovery function for Posttrans.
 *
 * PUBLIC: int logio_Posttrans_recover
 * PUBLIC:   __P((dbenv *, DBT *, DB_LSN *, db_recops));
 */
int
logio_Posttrans_recover(DB_ENV * dbenv, DBT * dbtp, DB_LSN * lsnp, db_recops op)
{
    logio_Posttrans_args *argp = NULL;
    int ret = EINVAL;

#ifdef DEBUG_RECOVER
    (void)logio_Posttrans_print(dbenv, dbtp, lsnp, op);
#endif
    if ((ret = logio_Posttrans_read(dbenv, dbtp->data, &argp)) != 0)
	goto exit;

    switch (op) {
    case DB_TXN_ABORT:
    case DB_TXN_BACKWARD_ROLL:
#ifdef	NOTYET
	ret = UNPosttrans(argp->path.data);
#endif
	if (ret != 0)
	    dbenv->err(dbenv, ret, "Posttrans: DB_TXN_BACKWARD_ROLL");
	else
	    ret = 0;
	break;
    case DB_TXN_APPLY:
    case DB_TXN_FORWARD_ROLL:
#ifdef	NOTYET
	ret = Posttrans(argp->path.data);
#endif
	if (ret != 0)
	    dbenv->err(dbenv, ret, "Posttrans: DB_TXN_FORWARD_ROLL");
	else
	    ret = 0;
    case DB_TXN_PRINT:
	if (ret != 0)
	    dbenv->err(dbenv, ret, "Posttrans: DB_TXN_PRINT");
	else
	    ret = 0;
	break;
    default:
	if (ret != 0)
	    dbenv->err(dbenv, ret, "Posttrans: UNKNOWN");
	else
	    ret = 0;
	break;
    }

    /* Allow for following LSN pointers through a transaction. */
    *lsnp = argp->prev_lsn;

exit:
    if (argp != NULL)
	free(argp);
    return ret;
}

/* ============================================================== */

int
logio_dispatch(DB_ENV * dbenv, DBT * dbt, DB_LSN * lsn, db_recops op)
{
    uint32_t rectype;

    /* Pull the record type out of the log record. */
    LOGCOPY_32(dbenv->env, &rectype, dbt->data);

    switch (rectype) {
    case DB_logio_Creat:
	return logio_Creat_recover(dbenv, dbt, lsn, op);
    case DB_logio_Unlink:
	return logio_Unlink_recover(dbenv, dbt, lsn, op);
    case DB_logio_Rename:
	return logio_Rename_recover(dbenv, dbt, lsn, op);
    case DB_logio_Mkdir:
	return logio_Mkdir_recover(dbenv, dbt, lsn, op);
    case DB_logio_Rmdir:
	return logio_Rmdir_recover(dbenv, dbt, lsn, op);
#ifdef	NOTYET
    case DB_logio_Lsetfilecon:
	return logio_Lsetfilecon_recover(dbenv, dbt, lsn, op);
#endif
    case DB_logio_Chown:
	return logio_Chown_recover(dbenv, dbt, lsn, op);
    case DB_logio_Lchown:
	return logio_Lchown_recover(dbenv, dbt, lsn, op);
    case DB_logio_Chmod:
	return logio_Chmod_recover(dbenv, dbt, lsn, op);
    case DB_logio_Utime:
	return logio_Utime_recover(dbenv, dbt, lsn, op);
    case DB_logio_Symlink:
	return logio_Symlink_recover(dbenv, dbt, lsn, op);
    case DB_logio_Link:
	return logio_Link_recover(dbenv, dbt, lsn, op);
    case DB_logio_Mknod:
	return logio_Mknod_recover(dbenv, dbt, lsn, op);
    case DB_logio_Mkfifo:
	return logio_Mkfifo_recover(dbenv, dbt, lsn, op);

    case DB_logio_Pretrans:
	return logio_Pretrans_recover(dbenv, dbt, lsn, op);
    case DB_logio_Prein:
	return logio_Prein_recover(dbenv, dbt, lsn, op);
    case DB_logio_Postin:
	return logio_Postin_recover(dbenv, dbt, lsn, op);
    case DB_logio_Preun:
	return logio_Preun_recover(dbenv, dbt, lsn, op);
    case DB_logio_Postun:
	return logio_Postun_recover(dbenv, dbt, lsn, op);
    case DB_logio_Posttrans:
	return logio_Posttrans_recover(dbenv, dbt, lsn, op);

    default:
	/* We've hit an unexpected, allegedly user-defined record type.  */
	dbenv->errx(dbenv, "Unexpected log record type encountered");
	return EINVAL;
    }
}

