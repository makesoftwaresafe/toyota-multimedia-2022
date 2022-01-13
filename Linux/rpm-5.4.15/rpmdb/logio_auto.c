/* Do not edit: automatically built by logio.awk. */

#include "system.h"

#include "db.h"
#include "db_int.h"
#include "dbinc/db_swap.h"

#include <rpmio.h>
#include "logio.h"
#include "debug.h"

/*
 * PUBLIC: int logio_Creat_read __P((DB_ENV *, void *, logio_Creat_args **));
 */
int
logio_Creat_read(DB_ENV * dbenv, void * recbuf, logio_Creat_args ** argpp)
{
	logio_Creat_args *argp;
	uint32_t uinttmp;
	uint8_t *bp;
	ENV *env;

	env = dbenv->env;

	if ((argp = malloc(sizeof(logio_Creat_args) + sizeof(DB_TXN))) == NULL)
		return (ENOMEM);
	bp = recbuf;
	argp->txnp = (DB_TXN *)&argp[1];
	memset(argp->txnp, 0, sizeof(DB_TXN));

	LOGCOPY_32(env, &argp->type, bp);
	bp += sizeof(argp->type);

	LOGCOPY_32(env, &argp->txnp->txnid, bp);
	bp += sizeof(argp->txnp->txnid);

	LOGCOPY_TOLSN(env, &argp->prev_lsn, bp);
	bp += sizeof(DB_LSN);

	memset(&argp->path, 0, sizeof(argp->path));
	LOGCOPY_32(env,&argp->path.size, bp);
	bp += sizeof(uint32_t);
	argp->path.data = bp;
	bp += argp->path.size;

	LOGCOPY_32(env, &uinttmp, bp);
	argp->mode = (mode_t)uinttmp;
	bp += sizeof(uinttmp);

	memset(&argp->content, 0, sizeof(argp->content));
	LOGCOPY_32(env,&argp->content.size, bp);
	bp += sizeof(uint32_t);
	argp->content.data = bp;
	bp += argp->content.size;

	memset(&argp->digest, 0, sizeof(argp->digest));
	LOGCOPY_32(env,&argp->digest.size, bp);
	bp += sizeof(uint32_t);
	argp->digest.data = bp;
	bp += argp->digest.size;

	LOGCOPY_32(env, &argp->dalgo, bp);
	bp += sizeof(argp->dalgo);

	*argpp = argp;
	return (0);
}

/*
 * PUBLIC: int logio_Creat_log __P((DB_ENV *, DB_TXN *, DB_LSN *,
 * PUBLIC:     uint32_t, const DBT *, mode_t, const DBT *, const DBT *,
 * PUBLIC:     uint32_t));
 */
int
logio_Creat_log(DB_ENV *dbenv, DB_TXN *txnp, DB_LSN *ret_lsnp, uint32_t flags,
		const DBT *path, mode_t mode, const DBT *content, const DBT *digest,
		uint32_t dalgo)
{
	DBT logrec;
	DB_LSN *lsnp, null_lsn, *rlsnp;
	ENV *env;
	uint32_t zero, uinttmp, rectype, txn_num;
	unsigned int npad;
	uint8_t *bp;
	int ret;

	env = dbenv->env;
	rlsnp = ret_lsnp;
	rectype = DB_logio_Creat;
	npad = 0;
	ret = 0;

	if (txnp == NULL) {
		txn_num = 0;
		lsnp = &null_lsn;
		null_lsn.file = null_lsn.offset = 0;
	} else {
		/*
		 * We need to assign begin_lsn while holding region mutex.
		 * That assignment is done inside the DbEnv->log_put call,
		 * so pass in the appropriate memory location to be filled
		 * in by the log_put code.
		 */
		DB_SET_TXN_LSNP(txnp, &rlsnp, &lsnp);
		txn_num = txnp->txnid;
	}

	logrec.size = sizeof(rectype) + sizeof(txn_num) + sizeof(DB_LSN)
	    + sizeof(uint32_t) + (path == NULL ? 0 : path->size)
	    + sizeof(uint32_t)
	    + sizeof(uint32_t) + (content == NULL ? 0 : content->size)
	    + sizeof(uint32_t) + (digest == NULL ? 0 : digest->size)
	    + sizeof(uint32_t);
	if ((logrec.data = malloc(logrec.size)) == NULL)
		return (ENOMEM);
	bp = logrec.data;

	if (npad > 0)
		memset((uint8_t *)logrec.data + logrec.size - npad, 0, npad);

	bp = logrec.data;

	LOGCOPY_32(env, bp, &rectype);
	bp += sizeof(rectype);

	LOGCOPY_32(env, bp, &txn_num);
	bp += sizeof(txn_num);

	LOGCOPY_FROMLSN(env, bp, lsnp);
	bp += sizeof(DB_LSN);

	if (path == NULL) {
		zero = 0;
		LOGCOPY_32(env, bp, &zero);
		bp += sizeof(uint32_t);
	} else {
		LOGCOPY_32(env, bp, &path->size);
		bp += sizeof(path->size);
		memcpy(bp, path->data, path->size);
		bp += path->size;
	}

	uinttmp = (uint32_t)mode;
	LOGCOPY_32(env,bp, &uinttmp);
	bp += sizeof(uinttmp);

	if (content == NULL) {
		zero = 0;
		LOGCOPY_32(env, bp, &zero);
		bp += sizeof(uint32_t);
	} else {
		LOGCOPY_32(env, bp, &content->size);
		bp += sizeof(content->size);
		memcpy(bp, content->data, content->size);
		bp += content->size;
	}

	if (digest == NULL) {
		zero = 0;
		LOGCOPY_32(env, bp, &zero);
		bp += sizeof(uint32_t);
	} else {
		LOGCOPY_32(env, bp, &digest->size);
		bp += sizeof(digest->size);
		memcpy(bp, digest->data, digest->size);
		bp += digest->size;
	}

	LOGCOPY_32(env, bp, &dalgo);
	bp += sizeof(dalgo);

	if ((ret = dbenv->log_put(dbenv, rlsnp, (DBT *)&logrec,
	    flags | DB_LOG_NOCOPY)) == 0 && txnp != NULL) {
		*lsnp = *rlsnp;
		if (rlsnp != ret_lsnp)
			 *ret_lsnp = *rlsnp;
	}
#ifdef LOG_DIAGNOSTIC
	if (ret != 0)
		(void)logio_Creat_print(dbenv,
		    (DBT *)&logrec, ret_lsnp, DB_TXN_PRINT);
#endif

	free(logrec.data);
	return (ret);
}

/*
 * PUBLIC: int logio_Unlink_read __P((DB_ENV *, void *, logio_Unlink_args **));
 */
int
logio_Unlink_read(DB_ENV * dbenv, void * recbuf, logio_Unlink_args ** argpp)
{
	logio_Unlink_args *argp;
	uint32_t uinttmp;
	uint8_t *bp;
	ENV *env;

	env = dbenv->env;

	if ((argp = malloc(sizeof(logio_Unlink_args) + sizeof(DB_TXN))) == NULL)
		return (ENOMEM);
	bp = recbuf;
	argp->txnp = (DB_TXN *)&argp[1];
	memset(argp->txnp, 0, sizeof(DB_TXN));

	LOGCOPY_32(env, &argp->type, bp);
	bp += sizeof(argp->type);

	LOGCOPY_32(env, &argp->txnp->txnid, bp);
	bp += sizeof(argp->txnp->txnid);

	LOGCOPY_TOLSN(env, &argp->prev_lsn, bp);
	bp += sizeof(DB_LSN);

	memset(&argp->path, 0, sizeof(argp->path));
	LOGCOPY_32(env,&argp->path.size, bp);
	bp += sizeof(uint32_t);
	argp->path.data = bp;
	bp += argp->path.size;

	LOGCOPY_32(env, &uinttmp, bp);
	argp->mode = (mode_t)uinttmp;
	bp += sizeof(uinttmp);

	memset(&argp->content, 0, sizeof(argp->content));
	LOGCOPY_32(env,&argp->content.size, bp);
	bp += sizeof(uint32_t);
	argp->content.data = bp;
	bp += argp->content.size;

	memset(&argp->digest, 0, sizeof(argp->digest));
	LOGCOPY_32(env,&argp->digest.size, bp);
	bp += sizeof(uint32_t);
	argp->digest.data = bp;
	bp += argp->digest.size;

	LOGCOPY_32(env, &argp->dalgo, bp);
	bp += sizeof(argp->dalgo);

	*argpp = argp;
	return (0);
}

/*
 * PUBLIC: int logio_Unlink_log __P((DB_ENV *, DB_TXN *, DB_LSN *,
 * PUBLIC:     uint32_t, const DBT *, mode_t, const DBT *, const DBT *,
 * PUBLIC:     uint32_t));
 */
int
logio_Unlink_log(DB_ENV *dbenv, DB_TXN *txnp, DB_LSN *ret_lsnp, uint32_t flags,
		const DBT *path, mode_t mode, const DBT *content, const DBT *digest,
		uint32_t dalgo)
{
	DBT logrec;
	DB_LSN *lsnp, null_lsn, *rlsnp;
	ENV *env;
	uint32_t zero, uinttmp, rectype, txn_num;
	unsigned int npad;
	uint8_t *bp;
	int ret;

	env = dbenv->env;
	rlsnp = ret_lsnp;
	rectype = DB_logio_Unlink;
	npad = 0;
	ret = 0;

	if (txnp == NULL) {
		txn_num = 0;
		lsnp = &null_lsn;
		null_lsn.file = null_lsn.offset = 0;
	} else {
		/*
		 * We need to assign begin_lsn while holding region mutex.
		 * That assignment is done inside the DbEnv->log_put call,
		 * so pass in the appropriate memory location to be filled
		 * in by the log_put code.
		 */
		DB_SET_TXN_LSNP(txnp, &rlsnp, &lsnp);
		txn_num = txnp->txnid;
	}

	logrec.size = sizeof(rectype) + sizeof(txn_num) + sizeof(DB_LSN)
	    + sizeof(uint32_t) + (path == NULL ? 0 : path->size)
	    + sizeof(uint32_t)
	    + sizeof(uint32_t) + (content == NULL ? 0 : content->size)
	    + sizeof(uint32_t) + (digest == NULL ? 0 : digest->size)
	    + sizeof(uint32_t);
	if ((logrec.data = malloc(logrec.size)) == NULL)
		return (ENOMEM);
	bp = logrec.data;

	if (npad > 0)
		memset((uint8_t *)logrec.data + logrec.size - npad, 0, npad);

	bp = logrec.data;

	LOGCOPY_32(env, bp, &rectype);
	bp += sizeof(rectype);

	LOGCOPY_32(env, bp, &txn_num);
	bp += sizeof(txn_num);

	LOGCOPY_FROMLSN(env, bp, lsnp);
	bp += sizeof(DB_LSN);

	if (path == NULL) {
		zero = 0;
		LOGCOPY_32(env, bp, &zero);
		bp += sizeof(uint32_t);
	} else {
		LOGCOPY_32(env, bp, &path->size);
		bp += sizeof(path->size);
		memcpy(bp, path->data, path->size);
		bp += path->size;
	}

	uinttmp = (uint32_t)mode;
	LOGCOPY_32(env,bp, &uinttmp);
	bp += sizeof(uinttmp);

	if (content == NULL) {
		zero = 0;
		LOGCOPY_32(env, bp, &zero);
		bp += sizeof(uint32_t);
	} else {
		LOGCOPY_32(env, bp, &content->size);
		bp += sizeof(content->size);
		memcpy(bp, content->data, content->size);
		bp += content->size;
	}

	if (digest == NULL) {
		zero = 0;
		LOGCOPY_32(env, bp, &zero);
		bp += sizeof(uint32_t);
	} else {
		LOGCOPY_32(env, bp, &digest->size);
		bp += sizeof(digest->size);
		memcpy(bp, digest->data, digest->size);
		bp += digest->size;
	}

	LOGCOPY_32(env, bp, &dalgo);
	bp += sizeof(dalgo);

	if ((ret = dbenv->log_put(dbenv, rlsnp, (DBT *)&logrec,
	    flags | DB_LOG_NOCOPY)) == 0 && txnp != NULL) {
		*lsnp = *rlsnp;
		if (rlsnp != ret_lsnp)
			 *ret_lsnp = *rlsnp;
	}
#ifdef LOG_DIAGNOSTIC
	if (ret != 0)
		(void)logio_Unlink_print(dbenv,
		    (DBT *)&logrec, ret_lsnp, DB_TXN_PRINT);
#endif

	free(logrec.data);
	return (ret);
}

/*
 * PUBLIC: int logio_Rename_read __P((DB_ENV *, void *, logio_Rename_args **));
 */
int
logio_Rename_read(DB_ENV * dbenv, void * recbuf, logio_Rename_args ** argpp)
{
	logio_Rename_args *argp;
	uint32_t uinttmp;
	uint8_t *bp;
	ENV *env;

	env = dbenv->env;

	if ((argp = malloc(sizeof(logio_Rename_args) + sizeof(DB_TXN))) == NULL)
		return (ENOMEM);
	bp = recbuf;
	argp->txnp = (DB_TXN *)&argp[1];
	memset(argp->txnp, 0, sizeof(DB_TXN));

	LOGCOPY_32(env, &argp->type, bp);
	bp += sizeof(argp->type);

	LOGCOPY_32(env, &argp->txnp->txnid, bp);
	bp += sizeof(argp->txnp->txnid);

	LOGCOPY_TOLSN(env, &argp->prev_lsn, bp);
	bp += sizeof(DB_LSN);

	memset(&argp->oldpath, 0, sizeof(argp->oldpath));
	LOGCOPY_32(env,&argp->oldpath.size, bp);
	bp += sizeof(uint32_t);
	argp->oldpath.data = bp;
	bp += argp->oldpath.size;

	memset(&argp->newpath, 0, sizeof(argp->newpath));
	LOGCOPY_32(env,&argp->newpath.size, bp);
	bp += sizeof(uint32_t);
	argp->newpath.data = bp;
	bp += argp->newpath.size;

	LOGCOPY_32(env, &uinttmp, bp);
	argp->mode = (mode_t)uinttmp;
	bp += sizeof(uinttmp);

	memset(&argp->content, 0, sizeof(argp->content));
	LOGCOPY_32(env,&argp->content.size, bp);
	bp += sizeof(uint32_t);
	argp->content.data = bp;
	bp += argp->content.size;

	memset(&argp->digest, 0, sizeof(argp->digest));
	LOGCOPY_32(env,&argp->digest.size, bp);
	bp += sizeof(uint32_t);
	argp->digest.data = bp;
	bp += argp->digest.size;

	LOGCOPY_32(env, &argp->dalgo, bp);
	bp += sizeof(argp->dalgo);

	*argpp = argp;
	return (0);
}

/*
 * PUBLIC: int logio_Rename_log __P((DB_ENV *, DB_TXN *, DB_LSN *,
 * PUBLIC:     uint32_t, const DBT *, const DBT *, mode_t, const DBT *,
 * PUBLIC:     const DBT *, uint32_t));
 */
int
logio_Rename_log(DB_ENV *dbenv, DB_TXN *txnp, DB_LSN *ret_lsnp, uint32_t flags,
		const DBT *oldpath, const DBT *newpath, mode_t mode, const DBT *content,
		const DBT *digest, uint32_t dalgo)
{
	DBT logrec;
	DB_LSN *lsnp, null_lsn, *rlsnp;
	ENV *env;
	uint32_t zero, uinttmp, rectype, txn_num;
	unsigned int npad;
	uint8_t *bp;
	int ret;

	env = dbenv->env;
	rlsnp = ret_lsnp;
	rectype = DB_logio_Rename;
	npad = 0;
	ret = 0;

	if (txnp == NULL) {
		txn_num = 0;
		lsnp = &null_lsn;
		null_lsn.file = null_lsn.offset = 0;
	} else {
		/*
		 * We need to assign begin_lsn while holding region mutex.
		 * That assignment is done inside the DbEnv->log_put call,
		 * so pass in the appropriate memory location to be filled
		 * in by the log_put code.
		 */
		DB_SET_TXN_LSNP(txnp, &rlsnp, &lsnp);
		txn_num = txnp->txnid;
	}

	logrec.size = sizeof(rectype) + sizeof(txn_num) + sizeof(DB_LSN)
	    + sizeof(uint32_t) + (oldpath == NULL ? 0 : oldpath->size)
	    + sizeof(uint32_t) + (newpath == NULL ? 0 : newpath->size)
	    + sizeof(uint32_t)
	    + sizeof(uint32_t) + (content == NULL ? 0 : content->size)
	    + sizeof(uint32_t) + (digest == NULL ? 0 : digest->size)
	    + sizeof(uint32_t);
	if ((logrec.data = malloc(logrec.size)) == NULL)
		return (ENOMEM);
	bp = logrec.data;

	if (npad > 0)
		memset((uint8_t *)logrec.data + logrec.size - npad, 0, npad);

	bp = logrec.data;

	LOGCOPY_32(env, bp, &rectype);
	bp += sizeof(rectype);

	LOGCOPY_32(env, bp, &txn_num);
	bp += sizeof(txn_num);

	LOGCOPY_FROMLSN(env, bp, lsnp);
	bp += sizeof(DB_LSN);

	if (oldpath == NULL) {
		zero = 0;
		LOGCOPY_32(env, bp, &zero);
		bp += sizeof(uint32_t);
	} else {
		LOGCOPY_32(env, bp, &oldpath->size);
		bp += sizeof(oldpath->size);
		memcpy(bp, oldpath->data, oldpath->size);
		bp += oldpath->size;
	}

	if (newpath == NULL) {
		zero = 0;
		LOGCOPY_32(env, bp, &zero);
		bp += sizeof(uint32_t);
	} else {
		LOGCOPY_32(env, bp, &newpath->size);
		bp += sizeof(newpath->size);
		memcpy(bp, newpath->data, newpath->size);
		bp += newpath->size;
	}

	uinttmp = (uint32_t)mode;
	LOGCOPY_32(env,bp, &uinttmp);
	bp += sizeof(uinttmp);

	if (content == NULL) {
		zero = 0;
		LOGCOPY_32(env, bp, &zero);
		bp += sizeof(uint32_t);
	} else {
		LOGCOPY_32(env, bp, &content->size);
		bp += sizeof(content->size);
		memcpy(bp, content->data, content->size);
		bp += content->size;
	}

	if (digest == NULL) {
		zero = 0;
		LOGCOPY_32(env, bp, &zero);
		bp += sizeof(uint32_t);
	} else {
		LOGCOPY_32(env, bp, &digest->size);
		bp += sizeof(digest->size);
		memcpy(bp, digest->data, digest->size);
		bp += digest->size;
	}

	LOGCOPY_32(env, bp, &dalgo);
	bp += sizeof(dalgo);

	if ((ret = dbenv->log_put(dbenv, rlsnp, (DBT *)&logrec,
	    flags | DB_LOG_NOCOPY)) == 0 && txnp != NULL) {
		*lsnp = *rlsnp;
		if (rlsnp != ret_lsnp)
			 *ret_lsnp = *rlsnp;
	}
#ifdef LOG_DIAGNOSTIC
	if (ret != 0)
		(void)logio_Rename_print(dbenv,
		    (DBT *)&logrec, ret_lsnp, DB_TXN_PRINT);
#endif

	free(logrec.data);
	return (ret);
}

/*
 * PUBLIC: int logio_Mkdir_read __P((DB_ENV *, void *, logio_Mkdir_args **));
 */
int
logio_Mkdir_read(DB_ENV * dbenv, void * recbuf, logio_Mkdir_args ** argpp)
{
	logio_Mkdir_args *argp;
	uint32_t uinttmp;
	uint8_t *bp;
	ENV *env;

	env = dbenv->env;

	if ((argp = malloc(sizeof(logio_Mkdir_args) + sizeof(DB_TXN))) == NULL)
		return (ENOMEM);
	bp = recbuf;
	argp->txnp = (DB_TXN *)&argp[1];
	memset(argp->txnp, 0, sizeof(DB_TXN));

	LOGCOPY_32(env, &argp->type, bp);
	bp += sizeof(argp->type);

	LOGCOPY_32(env, &argp->txnp->txnid, bp);
	bp += sizeof(argp->txnp->txnid);

	LOGCOPY_TOLSN(env, &argp->prev_lsn, bp);
	bp += sizeof(DB_LSN);

	memset(&argp->path, 0, sizeof(argp->path));
	LOGCOPY_32(env,&argp->path.size, bp);
	bp += sizeof(uint32_t);
	argp->path.data = bp;
	bp += argp->path.size;

	LOGCOPY_32(env, &uinttmp, bp);
	argp->mode = (mode_t)uinttmp;
	bp += sizeof(uinttmp);

	*argpp = argp;
	return (0);
}

/*
 * PUBLIC: int logio_Mkdir_log __P((DB_ENV *, DB_TXN *, DB_LSN *,
 * PUBLIC:     uint32_t, const DBT *, mode_t));
 */
int
logio_Mkdir_log(DB_ENV *dbenv, DB_TXN *txnp, DB_LSN *ret_lsnp, uint32_t flags,
		const DBT *path, mode_t mode)
{
	DBT logrec;
	DB_LSN *lsnp, null_lsn, *rlsnp;
	ENV *env;
	uint32_t zero, uinttmp, rectype, txn_num;
	unsigned int npad;
	uint8_t *bp;
	int ret;

	env = dbenv->env;
	rlsnp = ret_lsnp;
	rectype = DB_logio_Mkdir;
	npad = 0;
	ret = 0;

	if (txnp == NULL) {
		txn_num = 0;
		lsnp = &null_lsn;
		null_lsn.file = null_lsn.offset = 0;
	} else {
		/*
		 * We need to assign begin_lsn while holding region mutex.
		 * That assignment is done inside the DbEnv->log_put call,
		 * so pass in the appropriate memory location to be filled
		 * in by the log_put code.
		 */
		DB_SET_TXN_LSNP(txnp, &rlsnp, &lsnp);
		txn_num = txnp->txnid;
	}

	logrec.size = sizeof(rectype) + sizeof(txn_num) + sizeof(DB_LSN)
	    + sizeof(uint32_t) + (path == NULL ? 0 : path->size)
	    + sizeof(uint32_t);
	if ((logrec.data = malloc(logrec.size)) == NULL)
		return (ENOMEM);
	bp = logrec.data;

	if (npad > 0)
		memset((uint8_t *)logrec.data + logrec.size - npad, 0, npad);

	bp = logrec.data;

	LOGCOPY_32(env, bp, &rectype);
	bp += sizeof(rectype);

	LOGCOPY_32(env, bp, &txn_num);
	bp += sizeof(txn_num);

	LOGCOPY_FROMLSN(env, bp, lsnp);
	bp += sizeof(DB_LSN);

	if (path == NULL) {
		zero = 0;
		LOGCOPY_32(env, bp, &zero);
		bp += sizeof(uint32_t);
	} else {
		LOGCOPY_32(env, bp, &path->size);
		bp += sizeof(path->size);
		memcpy(bp, path->data, path->size);
		bp += path->size;
	}

	uinttmp = (uint32_t)mode;
	LOGCOPY_32(env,bp, &uinttmp);
	bp += sizeof(uinttmp);

	if ((ret = dbenv->log_put(dbenv, rlsnp, (DBT *)&logrec,
	    flags | DB_LOG_NOCOPY)) == 0 && txnp != NULL) {
		*lsnp = *rlsnp;
		if (rlsnp != ret_lsnp)
			 *ret_lsnp = *rlsnp;
	}
#ifdef LOG_DIAGNOSTIC
	if (ret != 0)
		(void)logio_Mkdir_print(dbenv,
		    (DBT *)&logrec, ret_lsnp, DB_TXN_PRINT);
#endif

	free(logrec.data);
	return (ret);
}

/*
 * PUBLIC: int logio_Rmdir_read __P((DB_ENV *, void *, logio_Rmdir_args **));
 */
int
logio_Rmdir_read(DB_ENV * dbenv, void * recbuf, logio_Rmdir_args ** argpp)
{
	logio_Rmdir_args *argp;
	uint32_t uinttmp;
	uint8_t *bp;
	ENV *env;

	env = dbenv->env;

	if ((argp = malloc(sizeof(logio_Rmdir_args) + sizeof(DB_TXN))) == NULL)
		return (ENOMEM);
	bp = recbuf;
	argp->txnp = (DB_TXN *)&argp[1];
	memset(argp->txnp, 0, sizeof(DB_TXN));

	LOGCOPY_32(env, &argp->type, bp);
	bp += sizeof(argp->type);

	LOGCOPY_32(env, &argp->txnp->txnid, bp);
	bp += sizeof(argp->txnp->txnid);

	LOGCOPY_TOLSN(env, &argp->prev_lsn, bp);
	bp += sizeof(DB_LSN);

	memset(&argp->path, 0, sizeof(argp->path));
	LOGCOPY_32(env,&argp->path.size, bp);
	bp += sizeof(uint32_t);
	argp->path.data = bp;
	bp += argp->path.size;

	LOGCOPY_32(env, &uinttmp, bp);
	argp->mode = (mode_t)uinttmp;
	bp += sizeof(uinttmp);

	*argpp = argp;
	return (0);
}

/*
 * PUBLIC: int logio_Rmdir_log __P((DB_ENV *, DB_TXN *, DB_LSN *,
 * PUBLIC:     uint32_t, const DBT *, mode_t));
 */
int
logio_Rmdir_log(DB_ENV *dbenv, DB_TXN *txnp, DB_LSN *ret_lsnp, uint32_t flags,
		const DBT *path, mode_t mode)
{
	DBT logrec;
	DB_LSN *lsnp, null_lsn, *rlsnp;
	ENV *env;
	uint32_t zero, uinttmp, rectype, txn_num;
	unsigned int npad;
	uint8_t *bp;
	int ret;

	env = dbenv->env;
	rlsnp = ret_lsnp;
	rectype = DB_logio_Rmdir;
	npad = 0;
	ret = 0;

	if (txnp == NULL) {
		txn_num = 0;
		lsnp = &null_lsn;
		null_lsn.file = null_lsn.offset = 0;
	} else {
		/*
		 * We need to assign begin_lsn while holding region mutex.
		 * That assignment is done inside the DbEnv->log_put call,
		 * so pass in the appropriate memory location to be filled
		 * in by the log_put code.
		 */
		DB_SET_TXN_LSNP(txnp, &rlsnp, &lsnp);
		txn_num = txnp->txnid;
	}

	logrec.size = sizeof(rectype) + sizeof(txn_num) + sizeof(DB_LSN)
	    + sizeof(uint32_t) + (path == NULL ? 0 : path->size)
	    + sizeof(uint32_t);
	if ((logrec.data = malloc(logrec.size)) == NULL)
		return (ENOMEM);
	bp = logrec.data;

	if (npad > 0)
		memset((uint8_t *)logrec.data + logrec.size - npad, 0, npad);

	bp = logrec.data;

	LOGCOPY_32(env, bp, &rectype);
	bp += sizeof(rectype);

	LOGCOPY_32(env, bp, &txn_num);
	bp += sizeof(txn_num);

	LOGCOPY_FROMLSN(env, bp, lsnp);
	bp += sizeof(DB_LSN);

	if (path == NULL) {
		zero = 0;
		LOGCOPY_32(env, bp, &zero);
		bp += sizeof(uint32_t);
	} else {
		LOGCOPY_32(env, bp, &path->size);
		bp += sizeof(path->size);
		memcpy(bp, path->data, path->size);
		bp += path->size;
	}

	uinttmp = (uint32_t)mode;
	LOGCOPY_32(env,bp, &uinttmp);
	bp += sizeof(uinttmp);

	if ((ret = dbenv->log_put(dbenv, rlsnp, (DBT *)&logrec,
	    flags | DB_LOG_NOCOPY)) == 0 && txnp != NULL) {
		*lsnp = *rlsnp;
		if (rlsnp != ret_lsnp)
			 *ret_lsnp = *rlsnp;
	}
#ifdef LOG_DIAGNOSTIC
	if (ret != 0)
		(void)logio_Rmdir_print(dbenv,
		    (DBT *)&logrec, ret_lsnp, DB_TXN_PRINT);
#endif

	free(logrec.data);
	return (ret);
}

/*
 * PUBLIC: int logio_Lsetfilecon_read __P((DB_ENV *, void *,
 * PUBLIC:     logio_Lsetfilecon_args **));
 */
int
logio_Lsetfilecon_read(DB_ENV * dbenv, void * recbuf, logio_Lsetfilecon_args ** argpp)
{
	logio_Lsetfilecon_args *argp;
	uint8_t *bp;
	ENV *env;

	env = dbenv->env;

	if ((argp = malloc(sizeof(logio_Lsetfilecon_args) + sizeof(DB_TXN))) == NULL)
		return (ENOMEM);
	bp = recbuf;
	argp->txnp = (DB_TXN *)&argp[1];
	memset(argp->txnp, 0, sizeof(DB_TXN));

	LOGCOPY_32(env, &argp->type, bp);
	bp += sizeof(argp->type);

	LOGCOPY_32(env, &argp->txnp->txnid, bp);
	bp += sizeof(argp->txnp->txnid);

	LOGCOPY_TOLSN(env, &argp->prev_lsn, bp);
	bp += sizeof(DB_LSN);

	memset(&argp->path, 0, sizeof(argp->path));
	LOGCOPY_32(env,&argp->path.size, bp);
	bp += sizeof(uint32_t);
	argp->path.data = bp;
	bp += argp->path.size;

	memset(&argp->context, 0, sizeof(argp->context));
	LOGCOPY_32(env,&argp->context.size, bp);
	bp += sizeof(uint32_t);
	argp->context.data = bp;
	bp += argp->context.size;

	*argpp = argp;
	return (0);
}

/*
 * PUBLIC: int logio_Lsetfilecon_log __P((DB_ENV *, DB_TXN *,
 * PUBLIC:     DB_LSN *, uint32_t, const DBT *, const DBT *));
 */
int
logio_Lsetfilecon_log(DB_ENV *dbenv, DB_TXN *txnp, DB_LSN *ret_lsnp, uint32_t flags,
		const DBT *path, const DBT *context)
{
	DBT logrec;
	DB_LSN *lsnp, null_lsn, *rlsnp;
	ENV *env;
	uint32_t zero, rectype, txn_num;
	unsigned int npad;
	uint8_t *bp;
	int ret;

	env = dbenv->env;
	rlsnp = ret_lsnp;
	rectype = DB_logio_Lsetfilecon;
	npad = 0;
	ret = 0;

	if (txnp == NULL) {
		txn_num = 0;
		lsnp = &null_lsn;
		null_lsn.file = null_lsn.offset = 0;
	} else {
		/*
		 * We need to assign begin_lsn while holding region mutex.
		 * That assignment is done inside the DbEnv->log_put call,
		 * so pass in the appropriate memory location to be filled
		 * in by the log_put code.
		 */
		DB_SET_TXN_LSNP(txnp, &rlsnp, &lsnp);
		txn_num = txnp->txnid;
	}

	logrec.size = sizeof(rectype) + sizeof(txn_num) + sizeof(DB_LSN)
	    + sizeof(uint32_t) + (path == NULL ? 0 : path->size)
	    + sizeof(uint32_t) + (context == NULL ? 0 : context->size);
	if ((logrec.data = malloc(logrec.size)) == NULL)
		return (ENOMEM);
	bp = logrec.data;

	if (npad > 0)
		memset((uint8_t *)logrec.data + logrec.size - npad, 0, npad);

	bp = logrec.data;

	LOGCOPY_32(env, bp, &rectype);
	bp += sizeof(rectype);

	LOGCOPY_32(env, bp, &txn_num);
	bp += sizeof(txn_num);

	LOGCOPY_FROMLSN(env, bp, lsnp);
	bp += sizeof(DB_LSN);

	if (path == NULL) {
		zero = 0;
		LOGCOPY_32(env, bp, &zero);
		bp += sizeof(uint32_t);
	} else {
		LOGCOPY_32(env, bp, &path->size);
		bp += sizeof(path->size);
		memcpy(bp, path->data, path->size);
		bp += path->size;
	}

	if (context == NULL) {
		zero = 0;
		LOGCOPY_32(env, bp, &zero);
		bp += sizeof(uint32_t);
	} else {
		LOGCOPY_32(env, bp, &context->size);
		bp += sizeof(context->size);
		memcpy(bp, context->data, context->size);
		bp += context->size;
	}

	if ((ret = dbenv->log_put(dbenv, rlsnp, (DBT *)&logrec,
	    flags | DB_LOG_NOCOPY)) == 0 && txnp != NULL) {
		*lsnp = *rlsnp;
		if (rlsnp != ret_lsnp)
			 *ret_lsnp = *rlsnp;
	}
#ifdef LOG_DIAGNOSTIC
	if (ret != 0)
		(void)logio_Lsetfilecon_print(dbenv,
		    (DBT *)&logrec, ret_lsnp, DB_TXN_PRINT);
#endif

	free(logrec.data);
	return (ret);
}

/*
 * PUBLIC: int logio_Chown_read __P((DB_ENV *, void *, logio_Chown_args **));
 */
int
logio_Chown_read(DB_ENV * dbenv, void * recbuf, logio_Chown_args ** argpp)
{
	logio_Chown_args *argp;
	uint32_t uinttmp;
	uint8_t *bp;
	ENV *env;

	env = dbenv->env;

	if ((argp = malloc(sizeof(logio_Chown_args) + sizeof(DB_TXN))) == NULL)
		return (ENOMEM);
	bp = recbuf;
	argp->txnp = (DB_TXN *)&argp[1];
	memset(argp->txnp, 0, sizeof(DB_TXN));

	LOGCOPY_32(env, &argp->type, bp);
	bp += sizeof(argp->type);

	LOGCOPY_32(env, &argp->txnp->txnid, bp);
	bp += sizeof(argp->txnp->txnid);

	LOGCOPY_TOLSN(env, &argp->prev_lsn, bp);
	bp += sizeof(DB_LSN);

	memset(&argp->path, 0, sizeof(argp->path));
	LOGCOPY_32(env,&argp->path.size, bp);
	bp += sizeof(uint32_t);
	argp->path.data = bp;
	bp += argp->path.size;

	LOGCOPY_32(env, &uinttmp, bp);
	argp->owner = (uid_t)uinttmp;
	bp += sizeof(uinttmp);

	LOGCOPY_32(env, &uinttmp, bp);
	argp->group = (gid_t)uinttmp;
	bp += sizeof(uinttmp);

	*argpp = argp;
	return (0);
}

/*
 * PUBLIC: int logio_Chown_log __P((DB_ENV *, DB_TXN *, DB_LSN *,
 * PUBLIC:     uint32_t, const DBT *, uid_t, gid_t));
 */
int
logio_Chown_log(DB_ENV *dbenv, DB_TXN *txnp, DB_LSN *ret_lsnp, uint32_t flags,
		const DBT *path, uid_t owner, gid_t group)
{
	DBT logrec;
	DB_LSN *lsnp, null_lsn, *rlsnp;
	ENV *env;
	uint32_t zero, uinttmp, rectype, txn_num;
	unsigned int npad;
	uint8_t *bp;
	int ret;

	env = dbenv->env;
	rlsnp = ret_lsnp;
	rectype = DB_logio_Chown;
	npad = 0;
	ret = 0;

	if (txnp == NULL) {
		txn_num = 0;
		lsnp = &null_lsn;
		null_lsn.file = null_lsn.offset = 0;
	} else {
		/*
		 * We need to assign begin_lsn while holding region mutex.
		 * That assignment is done inside the DbEnv->log_put call,
		 * so pass in the appropriate memory location to be filled
		 * in by the log_put code.
		 */
		DB_SET_TXN_LSNP(txnp, &rlsnp, &lsnp);
		txn_num = txnp->txnid;
	}

	logrec.size = sizeof(rectype) + sizeof(txn_num) + sizeof(DB_LSN)
	    + sizeof(uint32_t) + (path == NULL ? 0 : path->size)
	    + sizeof(uint32_t)
	    + sizeof(uint32_t);
	if ((logrec.data = malloc(logrec.size)) == NULL)
		return (ENOMEM);
	bp = logrec.data;

	if (npad > 0)
		memset((uint8_t *)logrec.data + logrec.size - npad, 0, npad);

	bp = logrec.data;

	LOGCOPY_32(env, bp, &rectype);
	bp += sizeof(rectype);

	LOGCOPY_32(env, bp, &txn_num);
	bp += sizeof(txn_num);

	LOGCOPY_FROMLSN(env, bp, lsnp);
	bp += sizeof(DB_LSN);

	if (path == NULL) {
		zero = 0;
		LOGCOPY_32(env, bp, &zero);
		bp += sizeof(uint32_t);
	} else {
		LOGCOPY_32(env, bp, &path->size);
		bp += sizeof(path->size);
		memcpy(bp, path->data, path->size);
		bp += path->size;
	}

	uinttmp = (uint32_t)owner;
	LOGCOPY_32(env,bp, &uinttmp);
	bp += sizeof(uinttmp);

	uinttmp = (uint32_t)group;
	LOGCOPY_32(env,bp, &uinttmp);
	bp += sizeof(uinttmp);

	if ((ret = dbenv->log_put(dbenv, rlsnp, (DBT *)&logrec,
	    flags | DB_LOG_NOCOPY)) == 0 && txnp != NULL) {
		*lsnp = *rlsnp;
		if (rlsnp != ret_lsnp)
			 *ret_lsnp = *rlsnp;
	}
#ifdef LOG_DIAGNOSTIC
	if (ret != 0)
		(void)logio_Chown_print(dbenv,
		    (DBT *)&logrec, ret_lsnp, DB_TXN_PRINT);
#endif

	free(logrec.data);
	return (ret);
}

/*
 * PUBLIC: int logio_Lchown_read __P((DB_ENV *, void *, logio_Lchown_args **));
 */
int
logio_Lchown_read(DB_ENV * dbenv, void * recbuf, logio_Lchown_args ** argpp)
{
	logio_Lchown_args *argp;
	uint32_t uinttmp;
	uint8_t *bp;
	ENV *env;

	env = dbenv->env;

	if ((argp = malloc(sizeof(logio_Lchown_args) + sizeof(DB_TXN))) == NULL)
		return (ENOMEM);
	bp = recbuf;
	argp->txnp = (DB_TXN *)&argp[1];
	memset(argp->txnp, 0, sizeof(DB_TXN));

	LOGCOPY_32(env, &argp->type, bp);
	bp += sizeof(argp->type);

	LOGCOPY_32(env, &argp->txnp->txnid, bp);
	bp += sizeof(argp->txnp->txnid);

	LOGCOPY_TOLSN(env, &argp->prev_lsn, bp);
	bp += sizeof(DB_LSN);

	memset(&argp->path, 0, sizeof(argp->path));
	LOGCOPY_32(env,&argp->path.size, bp);
	bp += sizeof(uint32_t);
	argp->path.data = bp;
	bp += argp->path.size;

	LOGCOPY_32(env, &uinttmp, bp);
	argp->owner = (uid_t)uinttmp;
	bp += sizeof(uinttmp);

	LOGCOPY_32(env, &uinttmp, bp);
	argp->group = (gid_t)uinttmp;
	bp += sizeof(uinttmp);

	*argpp = argp;
	return (0);
}

/*
 * PUBLIC: int logio_Lchown_log __P((DB_ENV *, DB_TXN *, DB_LSN *,
 * PUBLIC:     uint32_t, const DBT *, uid_t, gid_t));
 */
int
logio_Lchown_log(DB_ENV *dbenv, DB_TXN *txnp, DB_LSN *ret_lsnp, uint32_t flags,
		const DBT *path, uid_t owner, gid_t group)
{
	DBT logrec;
	DB_LSN *lsnp, null_lsn, *rlsnp;
	ENV *env;
	uint32_t zero, uinttmp, rectype, txn_num;
	unsigned int npad;
	uint8_t *bp;
	int ret;

	env = dbenv->env;
	rlsnp = ret_lsnp;
	rectype = DB_logio_Lchown;
	npad = 0;
	ret = 0;

	if (txnp == NULL) {
		txn_num = 0;
		lsnp = &null_lsn;
		null_lsn.file = null_lsn.offset = 0;
	} else {
		/*
		 * We need to assign begin_lsn while holding region mutex.
		 * That assignment is done inside the DbEnv->log_put call,
		 * so pass in the appropriate memory location to be filled
		 * in by the log_put code.
		 */
		DB_SET_TXN_LSNP(txnp, &rlsnp, &lsnp);
		txn_num = txnp->txnid;
	}

	logrec.size = sizeof(rectype) + sizeof(txn_num) + sizeof(DB_LSN)
	    + sizeof(uint32_t) + (path == NULL ? 0 : path->size)
	    + sizeof(uint32_t)
	    + sizeof(uint32_t);
	if ((logrec.data = malloc(logrec.size)) == NULL)
		return (ENOMEM);
	bp = logrec.data;

	if (npad > 0)
		memset((uint8_t *)logrec.data + logrec.size - npad, 0, npad);

	bp = logrec.data;

	LOGCOPY_32(env, bp, &rectype);
	bp += sizeof(rectype);

	LOGCOPY_32(env, bp, &txn_num);
	bp += sizeof(txn_num);

	LOGCOPY_FROMLSN(env, bp, lsnp);
	bp += sizeof(DB_LSN);

	if (path == NULL) {
		zero = 0;
		LOGCOPY_32(env, bp, &zero);
		bp += sizeof(uint32_t);
	} else {
		LOGCOPY_32(env, bp, &path->size);
		bp += sizeof(path->size);
		memcpy(bp, path->data, path->size);
		bp += path->size;
	}

	uinttmp = (uint32_t)owner;
	LOGCOPY_32(env,bp, &uinttmp);
	bp += sizeof(uinttmp);

	uinttmp = (uint32_t)group;
	LOGCOPY_32(env,bp, &uinttmp);
	bp += sizeof(uinttmp);

	if ((ret = dbenv->log_put(dbenv, rlsnp, (DBT *)&logrec,
	    flags | DB_LOG_NOCOPY)) == 0 && txnp != NULL) {
		*lsnp = *rlsnp;
		if (rlsnp != ret_lsnp)
			 *ret_lsnp = *rlsnp;
	}
#ifdef LOG_DIAGNOSTIC
	if (ret != 0)
		(void)logio_Lchown_print(dbenv,
		    (DBT *)&logrec, ret_lsnp, DB_TXN_PRINT);
#endif

	free(logrec.data);
	return (ret);
}

/*
 * PUBLIC: int logio_Chmod_read __P((DB_ENV *, void *, logio_Chmod_args **));
 */
int
logio_Chmod_read(DB_ENV * dbenv, void * recbuf, logio_Chmod_args ** argpp)
{
	logio_Chmod_args *argp;
	uint32_t uinttmp;
	uint8_t *bp;
	ENV *env;

	env = dbenv->env;

	if ((argp = malloc(sizeof(logio_Chmod_args) + sizeof(DB_TXN))) == NULL)
		return (ENOMEM);
	bp = recbuf;
	argp->txnp = (DB_TXN *)&argp[1];
	memset(argp->txnp, 0, sizeof(DB_TXN));

	LOGCOPY_32(env, &argp->type, bp);
	bp += sizeof(argp->type);

	LOGCOPY_32(env, &argp->txnp->txnid, bp);
	bp += sizeof(argp->txnp->txnid);

	LOGCOPY_TOLSN(env, &argp->prev_lsn, bp);
	bp += sizeof(DB_LSN);

	memset(&argp->path, 0, sizeof(argp->path));
	LOGCOPY_32(env,&argp->path.size, bp);
	bp += sizeof(uint32_t);
	argp->path.data = bp;
	bp += argp->path.size;

	LOGCOPY_32(env, &uinttmp, bp);
	argp->mode = (mode_t)uinttmp;
	bp += sizeof(uinttmp);

	*argpp = argp;
	return (0);
}

/*
 * PUBLIC: int logio_Chmod_log __P((DB_ENV *, DB_TXN *, DB_LSN *,
 * PUBLIC:     uint32_t, const DBT *, mode_t));
 */
int
logio_Chmod_log(DB_ENV *dbenv, DB_TXN *txnp, DB_LSN *ret_lsnp, uint32_t flags,
		const DBT *path, mode_t mode)
{
	DBT logrec;
	DB_LSN *lsnp, null_lsn, *rlsnp;
	ENV *env;
	uint32_t zero, uinttmp, rectype, txn_num;
	unsigned int npad;
	uint8_t *bp;
	int ret;

	env = dbenv->env;
	rlsnp = ret_lsnp;
	rectype = DB_logio_Chmod;
	npad = 0;
	ret = 0;

	if (txnp == NULL) {
		txn_num = 0;
		lsnp = &null_lsn;
		null_lsn.file = null_lsn.offset = 0;
	} else {
		/*
		 * We need to assign begin_lsn while holding region mutex.
		 * That assignment is done inside the DbEnv->log_put call,
		 * so pass in the appropriate memory location to be filled
		 * in by the log_put code.
		 */
		DB_SET_TXN_LSNP(txnp, &rlsnp, &lsnp);
		txn_num = txnp->txnid;
	}

	logrec.size = sizeof(rectype) + sizeof(txn_num) + sizeof(DB_LSN)
	    + sizeof(uint32_t) + (path == NULL ? 0 : path->size)
	    + sizeof(uint32_t);
	if ((logrec.data = malloc(logrec.size)) == NULL)
		return (ENOMEM);
	bp = logrec.data;

	if (npad > 0)
		memset((uint8_t *)logrec.data + logrec.size - npad, 0, npad);

	bp = logrec.data;

	LOGCOPY_32(env, bp, &rectype);
	bp += sizeof(rectype);

	LOGCOPY_32(env, bp, &txn_num);
	bp += sizeof(txn_num);

	LOGCOPY_FROMLSN(env, bp, lsnp);
	bp += sizeof(DB_LSN);

	if (path == NULL) {
		zero = 0;
		LOGCOPY_32(env, bp, &zero);
		bp += sizeof(uint32_t);
	} else {
		LOGCOPY_32(env, bp, &path->size);
		bp += sizeof(path->size);
		memcpy(bp, path->data, path->size);
		bp += path->size;
	}

	uinttmp = (uint32_t)mode;
	LOGCOPY_32(env,bp, &uinttmp);
	bp += sizeof(uinttmp);

	if ((ret = dbenv->log_put(dbenv, rlsnp, (DBT *)&logrec,
	    flags | DB_LOG_NOCOPY)) == 0 && txnp != NULL) {
		*lsnp = *rlsnp;
		if (rlsnp != ret_lsnp)
			 *ret_lsnp = *rlsnp;
	}
#ifdef LOG_DIAGNOSTIC
	if (ret != 0)
		(void)logio_Chmod_print(dbenv,
		    (DBT *)&logrec, ret_lsnp, DB_TXN_PRINT);
#endif

	free(logrec.data);
	return (ret);
}

/*
 * PUBLIC: int logio_Utime_read __P((DB_ENV *, void *, logio_Utime_args **));
 */
int
logio_Utime_read(DB_ENV * dbenv, void * recbuf, logio_Utime_args ** argpp)
{
	logio_Utime_args *argp;
	uint32_t uinttmp;
	uint8_t *bp;
	ENV *env;

	env = dbenv->env;

	if ((argp = malloc(sizeof(logio_Utime_args) + sizeof(DB_TXN))) == NULL)
		return (ENOMEM);
	bp = recbuf;
	argp->txnp = (DB_TXN *)&argp[1];
	memset(argp->txnp, 0, sizeof(DB_TXN));

	LOGCOPY_32(env, &argp->type, bp);
	bp += sizeof(argp->type);

	LOGCOPY_32(env, &argp->txnp->txnid, bp);
	bp += sizeof(argp->txnp->txnid);

	LOGCOPY_TOLSN(env, &argp->prev_lsn, bp);
	bp += sizeof(DB_LSN);

	memset(&argp->path, 0, sizeof(argp->path));
	LOGCOPY_32(env,&argp->path.size, bp);
	bp += sizeof(uint32_t);
	argp->path.data = bp;
	bp += argp->path.size;

	LOGCOPY_32(env, &uinttmp, bp);
	argp->actime = (time_t)uinttmp;
	bp += sizeof(uinttmp);

	LOGCOPY_32(env, &uinttmp, bp);
	argp->modtime = (time_t)uinttmp;
	bp += sizeof(uinttmp);

	*argpp = argp;
	return (0);
}

/*
 * PUBLIC: int logio_Utime_log __P((DB_ENV *, DB_TXN *, DB_LSN *,
 * PUBLIC:     uint32_t, const DBT *, time_t, time_t));
 */
int
logio_Utime_log(DB_ENV *dbenv, DB_TXN *txnp, DB_LSN *ret_lsnp, uint32_t flags,
		const DBT *path, time_t actime, time_t modtime)
{
	DBT logrec;
	DB_LSN *lsnp, null_lsn, *rlsnp;
	ENV *env;
	uint32_t zero, uinttmp, rectype, txn_num;
	unsigned int npad;
	uint8_t *bp;
	int ret;

	env = dbenv->env;
	rlsnp = ret_lsnp;
	rectype = DB_logio_Utime;
	npad = 0;
	ret = 0;

	if (txnp == NULL) {
		txn_num = 0;
		lsnp = &null_lsn;
		null_lsn.file = null_lsn.offset = 0;
	} else {
		/*
		 * We need to assign begin_lsn while holding region mutex.
		 * That assignment is done inside the DbEnv->log_put call,
		 * so pass in the appropriate memory location to be filled
		 * in by the log_put code.
		 */
		DB_SET_TXN_LSNP(txnp, &rlsnp, &lsnp);
		txn_num = txnp->txnid;
	}

	logrec.size = sizeof(rectype) + sizeof(txn_num) + sizeof(DB_LSN)
	    + sizeof(uint32_t) + (path == NULL ? 0 : path->size)
	    + sizeof(uint32_t)
	    + sizeof(uint32_t);
	if ((logrec.data = malloc(logrec.size)) == NULL)
		return (ENOMEM);
	bp = logrec.data;

	if (npad > 0)
		memset((uint8_t *)logrec.data + logrec.size - npad, 0, npad);

	bp = logrec.data;

	LOGCOPY_32(env, bp, &rectype);
	bp += sizeof(rectype);

	LOGCOPY_32(env, bp, &txn_num);
	bp += sizeof(txn_num);

	LOGCOPY_FROMLSN(env, bp, lsnp);
	bp += sizeof(DB_LSN);

	if (path == NULL) {
		zero = 0;
		LOGCOPY_32(env, bp, &zero);
		bp += sizeof(uint32_t);
	} else {
		LOGCOPY_32(env, bp, &path->size);
		bp += sizeof(path->size);
		memcpy(bp, path->data, path->size);
		bp += path->size;
	}

	uinttmp = (uint32_t)actime;
	LOGCOPY_32(env,bp, &uinttmp);
	bp += sizeof(uinttmp);

	uinttmp = (uint32_t)modtime;
	LOGCOPY_32(env,bp, &uinttmp);
	bp += sizeof(uinttmp);

	if ((ret = dbenv->log_put(dbenv, rlsnp, (DBT *)&logrec,
	    flags | DB_LOG_NOCOPY)) == 0 && txnp != NULL) {
		*lsnp = *rlsnp;
		if (rlsnp != ret_lsnp)
			 *ret_lsnp = *rlsnp;
	}
#ifdef LOG_DIAGNOSTIC
	if (ret != 0)
		(void)logio_Utime_print(dbenv,
		    (DBT *)&logrec, ret_lsnp, DB_TXN_PRINT);
#endif

	free(logrec.data);
	return (ret);
}

/*
 * PUBLIC: int logio_Symlink_read __P((DB_ENV *, void *,
 * PUBLIC:     logio_Symlink_args **));
 */
int
logio_Symlink_read(DB_ENV * dbenv, void * recbuf, logio_Symlink_args ** argpp)
{
	logio_Symlink_args *argp;
	uint8_t *bp;
	ENV *env;

	env = dbenv->env;

	if ((argp = malloc(sizeof(logio_Symlink_args) + sizeof(DB_TXN))) == NULL)
		return (ENOMEM);
	bp = recbuf;
	argp->txnp = (DB_TXN *)&argp[1];
	memset(argp->txnp, 0, sizeof(DB_TXN));

	LOGCOPY_32(env, &argp->type, bp);
	bp += sizeof(argp->type);

	LOGCOPY_32(env, &argp->txnp->txnid, bp);
	bp += sizeof(argp->txnp->txnid);

	LOGCOPY_TOLSN(env, &argp->prev_lsn, bp);
	bp += sizeof(DB_LSN);

	memset(&argp->oldpath, 0, sizeof(argp->oldpath));
	LOGCOPY_32(env,&argp->oldpath.size, bp);
	bp += sizeof(uint32_t);
	argp->oldpath.data = bp;
	bp += argp->oldpath.size;

	memset(&argp->newpath, 0, sizeof(argp->newpath));
	LOGCOPY_32(env,&argp->newpath.size, bp);
	bp += sizeof(uint32_t);
	argp->newpath.data = bp;
	bp += argp->newpath.size;

	*argpp = argp;
	return (0);
}

/*
 * PUBLIC: int logio_Symlink_log __P((DB_ENV *, DB_TXN *, DB_LSN *,
 * PUBLIC:     uint32_t, const DBT *, const DBT *));
 */
int
logio_Symlink_log(DB_ENV *dbenv, DB_TXN *txnp, DB_LSN *ret_lsnp, uint32_t flags,
		const DBT *oldpath, const DBT *newpath)
{
	DBT logrec;
	DB_LSN *lsnp, null_lsn, *rlsnp;
	ENV *env;
	uint32_t zero, rectype, txn_num;
	unsigned int npad;
	uint8_t *bp;
	int ret;

	env = dbenv->env;
	rlsnp = ret_lsnp;
	rectype = DB_logio_Symlink;
	npad = 0;
	ret = 0;

	if (txnp == NULL) {
		txn_num = 0;
		lsnp = &null_lsn;
		null_lsn.file = null_lsn.offset = 0;
	} else {
		/*
		 * We need to assign begin_lsn while holding region mutex.
		 * That assignment is done inside the DbEnv->log_put call,
		 * so pass in the appropriate memory location to be filled
		 * in by the log_put code.
		 */
		DB_SET_TXN_LSNP(txnp, &rlsnp, &lsnp);
		txn_num = txnp->txnid;
	}

	logrec.size = sizeof(rectype) + sizeof(txn_num) + sizeof(DB_LSN)
	    + sizeof(uint32_t) + (oldpath == NULL ? 0 : oldpath->size)
	    + sizeof(uint32_t) + (newpath == NULL ? 0 : newpath->size);
	if ((logrec.data = malloc(logrec.size)) == NULL)
		return (ENOMEM);
	bp = logrec.data;

	if (npad > 0)
		memset((uint8_t *)logrec.data + logrec.size - npad, 0, npad);

	bp = logrec.data;

	LOGCOPY_32(env, bp, &rectype);
	bp += sizeof(rectype);

	LOGCOPY_32(env, bp, &txn_num);
	bp += sizeof(txn_num);

	LOGCOPY_FROMLSN(env, bp, lsnp);
	bp += sizeof(DB_LSN);

	if (oldpath == NULL) {
		zero = 0;
		LOGCOPY_32(env, bp, &zero);
		bp += sizeof(uint32_t);
	} else {
		LOGCOPY_32(env, bp, &oldpath->size);
		bp += sizeof(oldpath->size);
		memcpy(bp, oldpath->data, oldpath->size);
		bp += oldpath->size;
	}

	if (newpath == NULL) {
		zero = 0;
		LOGCOPY_32(env, bp, &zero);
		bp += sizeof(uint32_t);
	} else {
		LOGCOPY_32(env, bp, &newpath->size);
		bp += sizeof(newpath->size);
		memcpy(bp, newpath->data, newpath->size);
		bp += newpath->size;
	}

	if ((ret = dbenv->log_put(dbenv, rlsnp, (DBT *)&logrec,
	    flags | DB_LOG_NOCOPY)) == 0 && txnp != NULL) {
		*lsnp = *rlsnp;
		if (rlsnp != ret_lsnp)
			 *ret_lsnp = *rlsnp;
	}
#ifdef LOG_DIAGNOSTIC
	if (ret != 0)
		(void)logio_Symlink_print(dbenv,
		    (DBT *)&logrec, ret_lsnp, DB_TXN_PRINT);
#endif

	free(logrec.data);
	return (ret);
}

/*
 * PUBLIC: int logio_Link_read __P((DB_ENV *, void *, logio_Link_args **));
 */
int
logio_Link_read(DB_ENV * dbenv, void * recbuf, logio_Link_args ** argpp)
{
	logio_Link_args *argp;
	uint8_t *bp;
	ENV *env;

	env = dbenv->env;

	if ((argp = malloc(sizeof(logio_Link_args) + sizeof(DB_TXN))) == NULL)
		return (ENOMEM);
	bp = recbuf;
	argp->txnp = (DB_TXN *)&argp[1];
	memset(argp->txnp, 0, sizeof(DB_TXN));

	LOGCOPY_32(env, &argp->type, bp);
	bp += sizeof(argp->type);

	LOGCOPY_32(env, &argp->txnp->txnid, bp);
	bp += sizeof(argp->txnp->txnid);

	LOGCOPY_TOLSN(env, &argp->prev_lsn, bp);
	bp += sizeof(DB_LSN);

	memset(&argp->oldpath, 0, sizeof(argp->oldpath));
	LOGCOPY_32(env,&argp->oldpath.size, bp);
	bp += sizeof(uint32_t);
	argp->oldpath.data = bp;
	bp += argp->oldpath.size;

	memset(&argp->newpath, 0, sizeof(argp->newpath));
	LOGCOPY_32(env,&argp->newpath.size, bp);
	bp += sizeof(uint32_t);
	argp->newpath.data = bp;
	bp += argp->newpath.size;

	*argpp = argp;
	return (0);
}

/*
 * PUBLIC: int logio_Link_log __P((DB_ENV *, DB_TXN *, DB_LSN *,
 * PUBLIC:     uint32_t, const DBT *, const DBT *));
 */
int
logio_Link_log(DB_ENV *dbenv, DB_TXN *txnp, DB_LSN *ret_lsnp, uint32_t flags,
		const DBT *oldpath, const DBT *newpath)
{
	DBT logrec;
	DB_LSN *lsnp, null_lsn, *rlsnp;
	ENV *env;
	uint32_t zero, rectype, txn_num;
	unsigned int npad;
	uint8_t *bp;
	int ret;

	env = dbenv->env;
	rlsnp = ret_lsnp;
	rectype = DB_logio_Link;
	npad = 0;
	ret = 0;

	if (txnp == NULL) {
		txn_num = 0;
		lsnp = &null_lsn;
		null_lsn.file = null_lsn.offset = 0;
	} else {
		/*
		 * We need to assign begin_lsn while holding region mutex.
		 * That assignment is done inside the DbEnv->log_put call,
		 * so pass in the appropriate memory location to be filled
		 * in by the log_put code.
		 */
		DB_SET_TXN_LSNP(txnp, &rlsnp, &lsnp);
		txn_num = txnp->txnid;
	}

	logrec.size = sizeof(rectype) + sizeof(txn_num) + sizeof(DB_LSN)
	    + sizeof(uint32_t) + (oldpath == NULL ? 0 : oldpath->size)
	    + sizeof(uint32_t) + (newpath == NULL ? 0 : newpath->size);
	if ((logrec.data = malloc(logrec.size)) == NULL)
		return (ENOMEM);
	bp = logrec.data;

	if (npad > 0)
		memset((uint8_t *)logrec.data + logrec.size - npad, 0, npad);

	bp = logrec.data;

	LOGCOPY_32(env, bp, &rectype);
	bp += sizeof(rectype);

	LOGCOPY_32(env, bp, &txn_num);
	bp += sizeof(txn_num);

	LOGCOPY_FROMLSN(env, bp, lsnp);
	bp += sizeof(DB_LSN);

	if (oldpath == NULL) {
		zero = 0;
		LOGCOPY_32(env, bp, &zero);
		bp += sizeof(uint32_t);
	} else {
		LOGCOPY_32(env, bp, &oldpath->size);
		bp += sizeof(oldpath->size);
		memcpy(bp, oldpath->data, oldpath->size);
		bp += oldpath->size;
	}

	if (newpath == NULL) {
		zero = 0;
		LOGCOPY_32(env, bp, &zero);
		bp += sizeof(uint32_t);
	} else {
		LOGCOPY_32(env, bp, &newpath->size);
		bp += sizeof(newpath->size);
		memcpy(bp, newpath->data, newpath->size);
		bp += newpath->size;
	}

	if ((ret = dbenv->log_put(dbenv, rlsnp, (DBT *)&logrec,
	    flags | DB_LOG_NOCOPY)) == 0 && txnp != NULL) {
		*lsnp = *rlsnp;
		if (rlsnp != ret_lsnp)
			 *ret_lsnp = *rlsnp;
	}
#ifdef LOG_DIAGNOSTIC
	if (ret != 0)
		(void)logio_Link_print(dbenv,
		    (DBT *)&logrec, ret_lsnp, DB_TXN_PRINT);
#endif

	free(logrec.data);
	return (ret);
}

/*
 * PUBLIC: int logio_Mknod_read __P((DB_ENV *, void *, logio_Mknod_args **));
 */
int
logio_Mknod_read(DB_ENV * dbenv, void * recbuf, logio_Mknod_args ** argpp)
{
	logio_Mknod_args *argp;
	uint32_t uinttmp;
	uint8_t *bp;
	ENV *env;

	env = dbenv->env;

	if ((argp = malloc(sizeof(logio_Mknod_args) + sizeof(DB_TXN))) == NULL)
		return (ENOMEM);
	bp = recbuf;
	argp->txnp = (DB_TXN *)&argp[1];
	memset(argp->txnp, 0, sizeof(DB_TXN));

	LOGCOPY_32(env, &argp->type, bp);
	bp += sizeof(argp->type);

	LOGCOPY_32(env, &argp->txnp->txnid, bp);
	bp += sizeof(argp->txnp->txnid);

	LOGCOPY_TOLSN(env, &argp->prev_lsn, bp);
	bp += sizeof(DB_LSN);

	memset(&argp->path, 0, sizeof(argp->path));
	LOGCOPY_32(env,&argp->path.size, bp);
	bp += sizeof(uint32_t);
	argp->path.data = bp;
	bp += argp->path.size;

	LOGCOPY_32(env, &uinttmp, bp);
	argp->mode = (mode_t)uinttmp;
	bp += sizeof(uinttmp);

	LOGCOPY_32(env, &uinttmp, bp);
	argp->dev = (dev_t)uinttmp;
	bp += sizeof(uinttmp);

	*argpp = argp;
	return (0);
}

/*
 * PUBLIC: int logio_Mknod_log __P((DB_ENV *, DB_TXN *, DB_LSN *,
 * PUBLIC:     uint32_t, const DBT *, mode_t, dev_t));
 */
int
logio_Mknod_log(DB_ENV *dbenv, DB_TXN *txnp, DB_LSN *ret_lsnp, uint32_t flags,
		const DBT *path, mode_t mode, dev_t dev)
{
	DBT logrec;
	DB_LSN *lsnp, null_lsn, *rlsnp;
	ENV *env;
	uint32_t zero, uinttmp, rectype, txn_num;
	unsigned int npad;
	uint8_t *bp;
	int ret;

	env = dbenv->env;
	rlsnp = ret_lsnp;
	rectype = DB_logio_Mknod;
	npad = 0;
	ret = 0;

	if (txnp == NULL) {
		txn_num = 0;
		lsnp = &null_lsn;
		null_lsn.file = null_lsn.offset = 0;
	} else {
		/*
		 * We need to assign begin_lsn while holding region mutex.
		 * That assignment is done inside the DbEnv->log_put call,
		 * so pass in the appropriate memory location to be filled
		 * in by the log_put code.
		 */
		DB_SET_TXN_LSNP(txnp, &rlsnp, &lsnp);
		txn_num = txnp->txnid;
	}

	logrec.size = sizeof(rectype) + sizeof(txn_num) + sizeof(DB_LSN)
	    + sizeof(uint32_t) + (path == NULL ? 0 : path->size)
	    + sizeof(uint32_t)
	    + sizeof(uint32_t);
	if ((logrec.data = malloc(logrec.size)) == NULL)
		return (ENOMEM);
	bp = logrec.data;

	if (npad > 0)
		memset((uint8_t *)logrec.data + logrec.size - npad, 0, npad);

	bp = logrec.data;

	LOGCOPY_32(env, bp, &rectype);
	bp += sizeof(rectype);

	LOGCOPY_32(env, bp, &txn_num);
	bp += sizeof(txn_num);

	LOGCOPY_FROMLSN(env, bp, lsnp);
	bp += sizeof(DB_LSN);

	if (path == NULL) {
		zero = 0;
		LOGCOPY_32(env, bp, &zero);
		bp += sizeof(uint32_t);
	} else {
		LOGCOPY_32(env, bp, &path->size);
		bp += sizeof(path->size);
		memcpy(bp, path->data, path->size);
		bp += path->size;
	}

	uinttmp = (uint32_t)mode;
	LOGCOPY_32(env,bp, &uinttmp);
	bp += sizeof(uinttmp);

	uinttmp = (uint32_t)dev;
	LOGCOPY_32(env,bp, &uinttmp);
	bp += sizeof(uinttmp);

	if ((ret = dbenv->log_put(dbenv, rlsnp, (DBT *)&logrec,
	    flags | DB_LOG_NOCOPY)) == 0 && txnp != NULL) {
		*lsnp = *rlsnp;
		if (rlsnp != ret_lsnp)
			 *ret_lsnp = *rlsnp;
	}
#ifdef LOG_DIAGNOSTIC
	if (ret != 0)
		(void)logio_Mknod_print(dbenv,
		    (DBT *)&logrec, ret_lsnp, DB_TXN_PRINT);
#endif

	free(logrec.data);
	return (ret);
}

/*
 * PUBLIC: int logio_Mkfifo_read __P((DB_ENV *, void *, logio_Mkfifo_args **));
 */
int
logio_Mkfifo_read(DB_ENV * dbenv, void * recbuf, logio_Mkfifo_args ** argpp)
{
	logio_Mkfifo_args *argp;
	uint32_t uinttmp;
	uint8_t *bp;
	ENV *env;

	env = dbenv->env;

	if ((argp = malloc(sizeof(logio_Mkfifo_args) + sizeof(DB_TXN))) == NULL)
		return (ENOMEM);
	bp = recbuf;
	argp->txnp = (DB_TXN *)&argp[1];
	memset(argp->txnp, 0, sizeof(DB_TXN));

	LOGCOPY_32(env, &argp->type, bp);
	bp += sizeof(argp->type);

	LOGCOPY_32(env, &argp->txnp->txnid, bp);
	bp += sizeof(argp->txnp->txnid);

	LOGCOPY_TOLSN(env, &argp->prev_lsn, bp);
	bp += sizeof(DB_LSN);

	memset(&argp->path, 0, sizeof(argp->path));
	LOGCOPY_32(env,&argp->path.size, bp);
	bp += sizeof(uint32_t);
	argp->path.data = bp;
	bp += argp->path.size;

	LOGCOPY_32(env, &uinttmp, bp);
	argp->mode = (mode_t)uinttmp;
	bp += sizeof(uinttmp);

	*argpp = argp;
	return (0);
}

/*
 * PUBLIC: int logio_Mkfifo_log __P((DB_ENV *, DB_TXN *, DB_LSN *,
 * PUBLIC:     uint32_t, const DBT *, mode_t));
 */
int
logio_Mkfifo_log(DB_ENV *dbenv, DB_TXN *txnp, DB_LSN *ret_lsnp, uint32_t flags,
		const DBT *path, mode_t mode)
{
	DBT logrec;
	DB_LSN *lsnp, null_lsn, *rlsnp;
	ENV *env;
	uint32_t zero, uinttmp, rectype, txn_num;
	unsigned int npad;
	uint8_t *bp;
	int ret;

	env = dbenv->env;
	rlsnp = ret_lsnp;
	rectype = DB_logio_Mkfifo;
	npad = 0;
	ret = 0;

	if (txnp == NULL) {
		txn_num = 0;
		lsnp = &null_lsn;
		null_lsn.file = null_lsn.offset = 0;
	} else {
		/*
		 * We need to assign begin_lsn while holding region mutex.
		 * That assignment is done inside the DbEnv->log_put call,
		 * so pass in the appropriate memory location to be filled
		 * in by the log_put code.
		 */
		DB_SET_TXN_LSNP(txnp, &rlsnp, &lsnp);
		txn_num = txnp->txnid;
	}

	logrec.size = sizeof(rectype) + sizeof(txn_num) + sizeof(DB_LSN)
	    + sizeof(uint32_t) + (path == NULL ? 0 : path->size)
	    + sizeof(uint32_t);
	if ((logrec.data = malloc(logrec.size)) == NULL)
		return (ENOMEM);
	bp = logrec.data;

	if (npad > 0)
		memset((uint8_t *)logrec.data + logrec.size - npad, 0, npad);

	bp = logrec.data;

	LOGCOPY_32(env, bp, &rectype);
	bp += sizeof(rectype);

	LOGCOPY_32(env, bp, &txn_num);
	bp += sizeof(txn_num);

	LOGCOPY_FROMLSN(env, bp, lsnp);
	bp += sizeof(DB_LSN);

	if (path == NULL) {
		zero = 0;
		LOGCOPY_32(env, bp, &zero);
		bp += sizeof(uint32_t);
	} else {
		LOGCOPY_32(env, bp, &path->size);
		bp += sizeof(path->size);
		memcpy(bp, path->data, path->size);
		bp += path->size;
	}

	uinttmp = (uint32_t)mode;
	LOGCOPY_32(env,bp, &uinttmp);
	bp += sizeof(uinttmp);

	if ((ret = dbenv->log_put(dbenv, rlsnp, (DBT *)&logrec,
	    flags | DB_LOG_NOCOPY)) == 0 && txnp != NULL) {
		*lsnp = *rlsnp;
		if (rlsnp != ret_lsnp)
			 *ret_lsnp = *rlsnp;
	}
#ifdef LOG_DIAGNOSTIC
	if (ret != 0)
		(void)logio_Mkfifo_print(dbenv,
		    (DBT *)&logrec, ret_lsnp, DB_TXN_PRINT);
#endif

	free(logrec.data);
	return (ret);
}

/*
 * PUBLIC: int logio_Pretrans_read __P((DB_ENV *, void *,
 * PUBLIC:     logio_Pretrans_args **));
 */
int
logio_Pretrans_read(DB_ENV * dbenv, void * recbuf, logio_Pretrans_args ** argpp)
{
	logio_Pretrans_args *argp;
	uint8_t *bp;
	ENV *env;

	env = dbenv->env;

	if ((argp = malloc(sizeof(logio_Pretrans_args) + sizeof(DB_TXN))) == NULL)
		return (ENOMEM);
	bp = recbuf;
	argp->txnp = (DB_TXN *)&argp[1];
	memset(argp->txnp, 0, sizeof(DB_TXN));

	LOGCOPY_32(env, &argp->type, bp);
	bp += sizeof(argp->type);

	LOGCOPY_32(env, &argp->txnp->txnid, bp);
	bp += sizeof(argp->txnp->txnid);

	LOGCOPY_TOLSN(env, &argp->prev_lsn, bp);
	bp += sizeof(DB_LSN);

	memset(&argp->cmd, 0, sizeof(argp->cmd));
	LOGCOPY_32(env,&argp->cmd.size, bp);
	bp += sizeof(uint32_t);
	argp->cmd.data = bp;
	bp += argp->cmd.size;

	memset(&argp->script, 0, sizeof(argp->script));
	LOGCOPY_32(env,&argp->script.size, bp);
	bp += sizeof(uint32_t);
	argp->script.data = bp;
	bp += argp->script.size;

	*argpp = argp;
	return (0);
}

/*
 * PUBLIC: int logio_Pretrans_log __P((DB_ENV *, DB_TXN *, DB_LSN *,
 * PUBLIC:     uint32_t, const DBT *, const DBT *));
 */
int
logio_Pretrans_log(DB_ENV *dbenv, DB_TXN *txnp, DB_LSN *ret_lsnp, uint32_t flags,
		const DBT *cmd, const DBT *script)
{
	DBT logrec;
	DB_LSN *lsnp, null_lsn, *rlsnp;
	ENV *env;
	uint32_t zero, rectype, txn_num;
	unsigned int npad;
	uint8_t *bp;
	int ret;

	env = dbenv->env;
	rlsnp = ret_lsnp;
	rectype = DB_logio_Pretrans;
	npad = 0;
	ret = 0;

	if (txnp == NULL) {
		txn_num = 0;
		lsnp = &null_lsn;
		null_lsn.file = null_lsn.offset = 0;
	} else {
		/*
		 * We need to assign begin_lsn while holding region mutex.
		 * That assignment is done inside the DbEnv->log_put call,
		 * so pass in the appropriate memory location to be filled
		 * in by the log_put code.
		 */
		DB_SET_TXN_LSNP(txnp, &rlsnp, &lsnp);
		txn_num = txnp->txnid;
	}

	logrec.size = sizeof(rectype) + sizeof(txn_num) + sizeof(DB_LSN)
	    + sizeof(uint32_t) + (cmd == NULL ? 0 : cmd->size)
	    + sizeof(uint32_t) + (script == NULL ? 0 : script->size);
	if ((logrec.data = malloc(logrec.size)) == NULL)
		return (ENOMEM);
	bp = logrec.data;

	if (npad > 0)
		memset((uint8_t *)logrec.data + logrec.size - npad, 0, npad);

	bp = logrec.data;

	LOGCOPY_32(env, bp, &rectype);
	bp += sizeof(rectype);

	LOGCOPY_32(env, bp, &txn_num);
	bp += sizeof(txn_num);

	LOGCOPY_FROMLSN(env, bp, lsnp);
	bp += sizeof(DB_LSN);

	if (cmd == NULL) {
		zero = 0;
		LOGCOPY_32(env, bp, &zero);
		bp += sizeof(uint32_t);
	} else {
		LOGCOPY_32(env, bp, &cmd->size);
		bp += sizeof(cmd->size);
		memcpy(bp, cmd->data, cmd->size);
		bp += cmd->size;
	}

	if (script == NULL) {
		zero = 0;
		LOGCOPY_32(env, bp, &zero);
		bp += sizeof(uint32_t);
	} else {
		LOGCOPY_32(env, bp, &script->size);
		bp += sizeof(script->size);
		memcpy(bp, script->data, script->size);
		bp += script->size;
	}

	if ((ret = dbenv->log_put(dbenv, rlsnp, (DBT *)&logrec,
	    flags | DB_LOG_NOCOPY)) == 0 && txnp != NULL) {
		*lsnp = *rlsnp;
		if (rlsnp != ret_lsnp)
			 *ret_lsnp = *rlsnp;
	}
#ifdef LOG_DIAGNOSTIC
	if (ret != 0)
		(void)logio_Pretrans_print(dbenv,
		    (DBT *)&logrec, ret_lsnp, DB_TXN_PRINT);
#endif

	free(logrec.data);
	return (ret);
}

/*
 * PUBLIC: int logio_Prein_read __P((DB_ENV *, void *, logio_Prein_args **));
 */
int
logio_Prein_read(DB_ENV * dbenv, void * recbuf, logio_Prein_args ** argpp)
{
	logio_Prein_args *argp;
	uint8_t *bp;
	ENV *env;

	env = dbenv->env;

	if ((argp = malloc(sizeof(logio_Prein_args) + sizeof(DB_TXN))) == NULL)
		return (ENOMEM);
	bp = recbuf;
	argp->txnp = (DB_TXN *)&argp[1];
	memset(argp->txnp, 0, sizeof(DB_TXN));

	LOGCOPY_32(env, &argp->type, bp);
	bp += sizeof(argp->type);

	LOGCOPY_32(env, &argp->txnp->txnid, bp);
	bp += sizeof(argp->txnp->txnid);

	LOGCOPY_TOLSN(env, &argp->prev_lsn, bp);
	bp += sizeof(DB_LSN);

	memset(&argp->cmd, 0, sizeof(argp->cmd));
	LOGCOPY_32(env,&argp->cmd.size, bp);
	bp += sizeof(uint32_t);
	argp->cmd.data = bp;
	bp += argp->cmd.size;

	memset(&argp->script, 0, sizeof(argp->script));
	LOGCOPY_32(env,&argp->script.size, bp);
	bp += sizeof(uint32_t);
	argp->script.data = bp;
	bp += argp->script.size;

	*argpp = argp;
	return (0);
}

/*
 * PUBLIC: int logio_Prein_log __P((DB_ENV *, DB_TXN *, DB_LSN *,
 * PUBLIC:     uint32_t, const DBT *, const DBT *));
 */
int
logio_Prein_log(DB_ENV *dbenv, DB_TXN *txnp, DB_LSN *ret_lsnp, uint32_t flags,
		const DBT *cmd, const DBT *script)
{
	DBT logrec;
	DB_LSN *lsnp, null_lsn, *rlsnp;
	ENV *env;
	uint32_t zero, rectype, txn_num;
	unsigned int npad;
	uint8_t *bp;
	int ret;

	env = dbenv->env;
	rlsnp = ret_lsnp;
	rectype = DB_logio_Prein;
	npad = 0;
	ret = 0;

	if (txnp == NULL) {
		txn_num = 0;
		lsnp = &null_lsn;
		null_lsn.file = null_lsn.offset = 0;
	} else {
		/*
		 * We need to assign begin_lsn while holding region mutex.
		 * That assignment is done inside the DbEnv->log_put call,
		 * so pass in the appropriate memory location to be filled
		 * in by the log_put code.
		 */
		DB_SET_TXN_LSNP(txnp, &rlsnp, &lsnp);
		txn_num = txnp->txnid;
	}

	logrec.size = sizeof(rectype) + sizeof(txn_num) + sizeof(DB_LSN)
	    + sizeof(uint32_t) + (cmd == NULL ? 0 : cmd->size)
	    + sizeof(uint32_t) + (script == NULL ? 0 : script->size);
	if ((logrec.data = malloc(logrec.size)) == NULL)
		return (ENOMEM);
	bp = logrec.data;

	if (npad > 0)
		memset((uint8_t *)logrec.data + logrec.size - npad, 0, npad);

	bp = logrec.data;

	LOGCOPY_32(env, bp, &rectype);
	bp += sizeof(rectype);

	LOGCOPY_32(env, bp, &txn_num);
	bp += sizeof(txn_num);

	LOGCOPY_FROMLSN(env, bp, lsnp);
	bp += sizeof(DB_LSN);

	if (cmd == NULL) {
		zero = 0;
		LOGCOPY_32(env, bp, &zero);
		bp += sizeof(uint32_t);
	} else {
		LOGCOPY_32(env, bp, &cmd->size);
		bp += sizeof(cmd->size);
		memcpy(bp, cmd->data, cmd->size);
		bp += cmd->size;
	}

	if (script == NULL) {
		zero = 0;
		LOGCOPY_32(env, bp, &zero);
		bp += sizeof(uint32_t);
	} else {
		LOGCOPY_32(env, bp, &script->size);
		bp += sizeof(script->size);
		memcpy(bp, script->data, script->size);
		bp += script->size;
	}

	if ((ret = dbenv->log_put(dbenv, rlsnp, (DBT *)&logrec,
	    flags | DB_LOG_NOCOPY)) == 0 && txnp != NULL) {
		*lsnp = *rlsnp;
		if (rlsnp != ret_lsnp)
			 *ret_lsnp = *rlsnp;
	}
#ifdef LOG_DIAGNOSTIC
	if (ret != 0)
		(void)logio_Prein_print(dbenv,
		    (DBT *)&logrec, ret_lsnp, DB_TXN_PRINT);
#endif

	free(logrec.data);
	return (ret);
}

/*
 * PUBLIC: int logio_Postin_read __P((DB_ENV *, void *, logio_Postin_args **));
 */
int
logio_Postin_read(DB_ENV * dbenv, void * recbuf, logio_Postin_args ** argpp)
{
	logio_Postin_args *argp;
	uint8_t *bp;
	ENV *env;

	env = dbenv->env;

	if ((argp = malloc(sizeof(logio_Postin_args) + sizeof(DB_TXN))) == NULL)
		return (ENOMEM);
	bp = recbuf;
	argp->txnp = (DB_TXN *)&argp[1];
	memset(argp->txnp, 0, sizeof(DB_TXN));

	LOGCOPY_32(env, &argp->type, bp);
	bp += sizeof(argp->type);

	LOGCOPY_32(env, &argp->txnp->txnid, bp);
	bp += sizeof(argp->txnp->txnid);

	LOGCOPY_TOLSN(env, &argp->prev_lsn, bp);
	bp += sizeof(DB_LSN);

	memset(&argp->cmd, 0, sizeof(argp->cmd));
	LOGCOPY_32(env,&argp->cmd.size, bp);
	bp += sizeof(uint32_t);
	argp->cmd.data = bp;
	bp += argp->cmd.size;

	memset(&argp->script, 0, sizeof(argp->script));
	LOGCOPY_32(env,&argp->script.size, bp);
	bp += sizeof(uint32_t);
	argp->script.data = bp;
	bp += argp->script.size;

	*argpp = argp;
	return (0);
}

/*
 * PUBLIC: int logio_Postin_log __P((DB_ENV *, DB_TXN *, DB_LSN *,
 * PUBLIC:     uint32_t, const DBT *, const DBT *));
 */
int
logio_Postin_log(DB_ENV *dbenv, DB_TXN *txnp, DB_LSN *ret_lsnp, uint32_t flags,
		const DBT *cmd, const DBT *script)
{
	DBT logrec;
	DB_LSN *lsnp, null_lsn, *rlsnp;
	ENV *env;
	uint32_t zero, rectype, txn_num;
	unsigned int npad;
	uint8_t *bp;
	int ret;

	env = dbenv->env;
	rlsnp = ret_lsnp;
	rectype = DB_logio_Postin;
	npad = 0;
	ret = 0;

	if (txnp == NULL) {
		txn_num = 0;
		lsnp = &null_lsn;
		null_lsn.file = null_lsn.offset = 0;
	} else {
		/*
		 * We need to assign begin_lsn while holding region mutex.
		 * That assignment is done inside the DbEnv->log_put call,
		 * so pass in the appropriate memory location to be filled
		 * in by the log_put code.
		 */
		DB_SET_TXN_LSNP(txnp, &rlsnp, &lsnp);
		txn_num = txnp->txnid;
	}

	logrec.size = sizeof(rectype) + sizeof(txn_num) + sizeof(DB_LSN)
	    + sizeof(uint32_t) + (cmd == NULL ? 0 : cmd->size)
	    + sizeof(uint32_t) + (script == NULL ? 0 : script->size);
	if ((logrec.data = malloc(logrec.size)) == NULL)
		return (ENOMEM);
	bp = logrec.data;

	if (npad > 0)
		memset((uint8_t *)logrec.data + logrec.size - npad, 0, npad);

	bp = logrec.data;

	LOGCOPY_32(env, bp, &rectype);
	bp += sizeof(rectype);

	LOGCOPY_32(env, bp, &txn_num);
	bp += sizeof(txn_num);

	LOGCOPY_FROMLSN(env, bp, lsnp);
	bp += sizeof(DB_LSN);

	if (cmd == NULL) {
		zero = 0;
		LOGCOPY_32(env, bp, &zero);
		bp += sizeof(uint32_t);
	} else {
		LOGCOPY_32(env, bp, &cmd->size);
		bp += sizeof(cmd->size);
		memcpy(bp, cmd->data, cmd->size);
		bp += cmd->size;
	}

	if (script == NULL) {
		zero = 0;
		LOGCOPY_32(env, bp, &zero);
		bp += sizeof(uint32_t);
	} else {
		LOGCOPY_32(env, bp, &script->size);
		bp += sizeof(script->size);
		memcpy(bp, script->data, script->size);
		bp += script->size;
	}

	if ((ret = dbenv->log_put(dbenv, rlsnp, (DBT *)&logrec,
	    flags | DB_LOG_NOCOPY)) == 0 && txnp != NULL) {
		*lsnp = *rlsnp;
		if (rlsnp != ret_lsnp)
			 *ret_lsnp = *rlsnp;
	}
#ifdef LOG_DIAGNOSTIC
	if (ret != 0)
		(void)logio_Postin_print(dbenv,
		    (DBT *)&logrec, ret_lsnp, DB_TXN_PRINT);
#endif

	free(logrec.data);
	return (ret);
}

/*
 * PUBLIC: int logio_Preun_read __P((DB_ENV *, void *, logio_Preun_args **));
 */
int
logio_Preun_read(DB_ENV * dbenv, void * recbuf, logio_Preun_args ** argpp)
{
	logio_Preun_args *argp;
	uint8_t *bp;
	ENV *env;

	env = dbenv->env;

	if ((argp = malloc(sizeof(logio_Preun_args) + sizeof(DB_TXN))) == NULL)
		return (ENOMEM);
	bp = recbuf;
	argp->txnp = (DB_TXN *)&argp[1];
	memset(argp->txnp, 0, sizeof(DB_TXN));

	LOGCOPY_32(env, &argp->type, bp);
	bp += sizeof(argp->type);

	LOGCOPY_32(env, &argp->txnp->txnid, bp);
	bp += sizeof(argp->txnp->txnid);

	LOGCOPY_TOLSN(env, &argp->prev_lsn, bp);
	bp += sizeof(DB_LSN);

	memset(&argp->cmd, 0, sizeof(argp->cmd));
	LOGCOPY_32(env,&argp->cmd.size, bp);
	bp += sizeof(uint32_t);
	argp->cmd.data = bp;
	bp += argp->cmd.size;

	memset(&argp->script, 0, sizeof(argp->script));
	LOGCOPY_32(env,&argp->script.size, bp);
	bp += sizeof(uint32_t);
	argp->script.data = bp;
	bp += argp->script.size;

	*argpp = argp;
	return (0);
}

/*
 * PUBLIC: int logio_Preun_log __P((DB_ENV *, DB_TXN *, DB_LSN *,
 * PUBLIC:     uint32_t, const DBT *, const DBT *));
 */
int
logio_Preun_log(DB_ENV *dbenv, DB_TXN *txnp, DB_LSN *ret_lsnp, uint32_t flags,
		const DBT *cmd, const DBT *script)
{
	DBT logrec;
	DB_LSN *lsnp, null_lsn, *rlsnp;
	ENV *env;
	uint32_t zero, rectype, txn_num;
	unsigned int npad;
	uint8_t *bp;
	int ret;

	env = dbenv->env;
	rlsnp = ret_lsnp;
	rectype = DB_logio_Preun;
	npad = 0;
	ret = 0;

	if (txnp == NULL) {
		txn_num = 0;
		lsnp = &null_lsn;
		null_lsn.file = null_lsn.offset = 0;
	} else {
		/*
		 * We need to assign begin_lsn while holding region mutex.
		 * That assignment is done inside the DbEnv->log_put call,
		 * so pass in the appropriate memory location to be filled
		 * in by the log_put code.
		 */
		DB_SET_TXN_LSNP(txnp, &rlsnp, &lsnp);
		txn_num = txnp->txnid;
	}

	logrec.size = sizeof(rectype) + sizeof(txn_num) + sizeof(DB_LSN)
	    + sizeof(uint32_t) + (cmd == NULL ? 0 : cmd->size)
	    + sizeof(uint32_t) + (script == NULL ? 0 : script->size);
	if ((logrec.data = malloc(logrec.size)) == NULL)
		return (ENOMEM);
	bp = logrec.data;

	if (npad > 0)
		memset((uint8_t *)logrec.data + logrec.size - npad, 0, npad);

	bp = logrec.data;

	LOGCOPY_32(env, bp, &rectype);
	bp += sizeof(rectype);

	LOGCOPY_32(env, bp, &txn_num);
	bp += sizeof(txn_num);

	LOGCOPY_FROMLSN(env, bp, lsnp);
	bp += sizeof(DB_LSN);

	if (cmd == NULL) {
		zero = 0;
		LOGCOPY_32(env, bp, &zero);
		bp += sizeof(uint32_t);
	} else {
		LOGCOPY_32(env, bp, &cmd->size);
		bp += sizeof(cmd->size);
		memcpy(bp, cmd->data, cmd->size);
		bp += cmd->size;
	}

	if (script == NULL) {
		zero = 0;
		LOGCOPY_32(env, bp, &zero);
		bp += sizeof(uint32_t);
	} else {
		LOGCOPY_32(env, bp, &script->size);
		bp += sizeof(script->size);
		memcpy(bp, script->data, script->size);
		bp += script->size;
	}

	if ((ret = dbenv->log_put(dbenv, rlsnp, (DBT *)&logrec,
	    flags | DB_LOG_NOCOPY)) == 0 && txnp != NULL) {
		*lsnp = *rlsnp;
		if (rlsnp != ret_lsnp)
			 *ret_lsnp = *rlsnp;
	}
#ifdef LOG_DIAGNOSTIC
	if (ret != 0)
		(void)logio_Preun_print(dbenv,
		    (DBT *)&logrec, ret_lsnp, DB_TXN_PRINT);
#endif

	free(logrec.data);
	return (ret);
}

/*
 * PUBLIC: int logio_Postun_read __P((DB_ENV *, void *, logio_Postun_args **));
 */
int
logio_Postun_read(DB_ENV * dbenv, void * recbuf, logio_Postun_args ** argpp)
{
	logio_Postun_args *argp;
	uint8_t *bp;
	ENV *env;

	env = dbenv->env;

	if ((argp = malloc(sizeof(logio_Postun_args) + sizeof(DB_TXN))) == NULL)
		return (ENOMEM);
	bp = recbuf;
	argp->txnp = (DB_TXN *)&argp[1];
	memset(argp->txnp, 0, sizeof(DB_TXN));

	LOGCOPY_32(env, &argp->type, bp);
	bp += sizeof(argp->type);

	LOGCOPY_32(env, &argp->txnp->txnid, bp);
	bp += sizeof(argp->txnp->txnid);

	LOGCOPY_TOLSN(env, &argp->prev_lsn, bp);
	bp += sizeof(DB_LSN);

	memset(&argp->cmd, 0, sizeof(argp->cmd));
	LOGCOPY_32(env,&argp->cmd.size, bp);
	bp += sizeof(uint32_t);
	argp->cmd.data = bp;
	bp += argp->cmd.size;

	memset(&argp->script, 0, sizeof(argp->script));
	LOGCOPY_32(env,&argp->script.size, bp);
	bp += sizeof(uint32_t);
	argp->script.data = bp;
	bp += argp->script.size;

	*argpp = argp;
	return (0);
}

/*
 * PUBLIC: int logio_Postun_log __P((DB_ENV *, DB_TXN *, DB_LSN *,
 * PUBLIC:     uint32_t, const DBT *, const DBT *));
 */
int
logio_Postun_log(DB_ENV *dbenv, DB_TXN *txnp, DB_LSN *ret_lsnp, uint32_t flags,
		const DBT *cmd, const DBT *script)
{
	DBT logrec;
	DB_LSN *lsnp, null_lsn, *rlsnp;
	ENV *env;
	uint32_t zero, rectype, txn_num;
	unsigned int npad;
	uint8_t *bp;
	int ret;

	env = dbenv->env;
	rlsnp = ret_lsnp;
	rectype = DB_logio_Postun;
	npad = 0;
	ret = 0;

	if (txnp == NULL) {
		txn_num = 0;
		lsnp = &null_lsn;
		null_lsn.file = null_lsn.offset = 0;
	} else {
		/*
		 * We need to assign begin_lsn while holding region mutex.
		 * That assignment is done inside the DbEnv->log_put call,
		 * so pass in the appropriate memory location to be filled
		 * in by the log_put code.
		 */
		DB_SET_TXN_LSNP(txnp, &rlsnp, &lsnp);
		txn_num = txnp->txnid;
	}

	logrec.size = sizeof(rectype) + sizeof(txn_num) + sizeof(DB_LSN)
	    + sizeof(uint32_t) + (cmd == NULL ? 0 : cmd->size)
	    + sizeof(uint32_t) + (script == NULL ? 0 : script->size);
	if ((logrec.data = malloc(logrec.size)) == NULL)
		return (ENOMEM);
	bp = logrec.data;

	if (npad > 0)
		memset((uint8_t *)logrec.data + logrec.size - npad, 0, npad);

	bp = logrec.data;

	LOGCOPY_32(env, bp, &rectype);
	bp += sizeof(rectype);

	LOGCOPY_32(env, bp, &txn_num);
	bp += sizeof(txn_num);

	LOGCOPY_FROMLSN(env, bp, lsnp);
	bp += sizeof(DB_LSN);

	if (cmd == NULL) {
		zero = 0;
		LOGCOPY_32(env, bp, &zero);
		bp += sizeof(uint32_t);
	} else {
		LOGCOPY_32(env, bp, &cmd->size);
		bp += sizeof(cmd->size);
		memcpy(bp, cmd->data, cmd->size);
		bp += cmd->size;
	}

	if (script == NULL) {
		zero = 0;
		LOGCOPY_32(env, bp, &zero);
		bp += sizeof(uint32_t);
	} else {
		LOGCOPY_32(env, bp, &script->size);
		bp += sizeof(script->size);
		memcpy(bp, script->data, script->size);
		bp += script->size;
	}

	if ((ret = dbenv->log_put(dbenv, rlsnp, (DBT *)&logrec,
	    flags | DB_LOG_NOCOPY)) == 0 && txnp != NULL) {
		*lsnp = *rlsnp;
		if (rlsnp != ret_lsnp)
			 *ret_lsnp = *rlsnp;
	}
#ifdef LOG_DIAGNOSTIC
	if (ret != 0)
		(void)logio_Postun_print(dbenv,
		    (DBT *)&logrec, ret_lsnp, DB_TXN_PRINT);
#endif

	free(logrec.data);
	return (ret);
}

/*
 * PUBLIC: int logio_Posttrans_read __P((DB_ENV *, void *,
 * PUBLIC:     logio_Posttrans_args **));
 */
int
logio_Posttrans_read(DB_ENV * dbenv, void * recbuf, logio_Posttrans_args ** argpp)
{
	logio_Posttrans_args *argp;
	uint8_t *bp;
	ENV *env;

	env = dbenv->env;

	if ((argp = malloc(sizeof(logio_Posttrans_args) + sizeof(DB_TXN))) == NULL)
		return (ENOMEM);
	bp = recbuf;
	argp->txnp = (DB_TXN *)&argp[1];
	memset(argp->txnp, 0, sizeof(DB_TXN));

	LOGCOPY_32(env, &argp->type, bp);
	bp += sizeof(argp->type);

	LOGCOPY_32(env, &argp->txnp->txnid, bp);
	bp += sizeof(argp->txnp->txnid);

	LOGCOPY_TOLSN(env, &argp->prev_lsn, bp);
	bp += sizeof(DB_LSN);

	memset(&argp->cmd, 0, sizeof(argp->cmd));
	LOGCOPY_32(env,&argp->cmd.size, bp);
	bp += sizeof(uint32_t);
	argp->cmd.data = bp;
	bp += argp->cmd.size;

	memset(&argp->script, 0, sizeof(argp->script));
	LOGCOPY_32(env,&argp->script.size, bp);
	bp += sizeof(uint32_t);
	argp->script.data = bp;
	bp += argp->script.size;

	*argpp = argp;
	return (0);
}

/*
 * PUBLIC: int logio_Posttrans_log __P((DB_ENV *, DB_TXN *, DB_LSN *,
 * PUBLIC:     uint32_t, const DBT *, const DBT *));
 */
int
logio_Posttrans_log(DB_ENV *dbenv, DB_TXN *txnp, DB_LSN *ret_lsnp, uint32_t flags,
		const DBT *cmd, const DBT *script)
{
	DBT logrec;
	DB_LSN *lsnp, null_lsn, *rlsnp;
	ENV *env;
	uint32_t zero, rectype, txn_num;
	unsigned int npad;
	uint8_t *bp;
	int ret;

	env = dbenv->env;
	rlsnp = ret_lsnp;
	rectype = DB_logio_Posttrans;
	npad = 0;
	ret = 0;

	if (txnp == NULL) {
		txn_num = 0;
		lsnp = &null_lsn;
		null_lsn.file = null_lsn.offset = 0;
	} else {
		/*
		 * We need to assign begin_lsn while holding region mutex.
		 * That assignment is done inside the DbEnv->log_put call,
		 * so pass in the appropriate memory location to be filled
		 * in by the log_put code.
		 */
		DB_SET_TXN_LSNP(txnp, &rlsnp, &lsnp);
		txn_num = txnp->txnid;
	}

	logrec.size = sizeof(rectype) + sizeof(txn_num) + sizeof(DB_LSN)
	    + sizeof(uint32_t) + (cmd == NULL ? 0 : cmd->size)
	    + sizeof(uint32_t) + (script == NULL ? 0 : script->size);
	if ((logrec.data = malloc(logrec.size)) == NULL)
		return (ENOMEM);
	bp = logrec.data;

	if (npad > 0)
		memset((uint8_t *)logrec.data + logrec.size - npad, 0, npad);

	bp = logrec.data;

	LOGCOPY_32(env, bp, &rectype);
	bp += sizeof(rectype);

	LOGCOPY_32(env, bp, &txn_num);
	bp += sizeof(txn_num);

	LOGCOPY_FROMLSN(env, bp, lsnp);
	bp += sizeof(DB_LSN);

	if (cmd == NULL) {
		zero = 0;
		LOGCOPY_32(env, bp, &zero);
		bp += sizeof(uint32_t);
	} else {
		LOGCOPY_32(env, bp, &cmd->size);
		bp += sizeof(cmd->size);
		memcpy(bp, cmd->data, cmd->size);
		bp += cmd->size;
	}

	if (script == NULL) {
		zero = 0;
		LOGCOPY_32(env, bp, &zero);
		bp += sizeof(uint32_t);
	} else {
		LOGCOPY_32(env, bp, &script->size);
		bp += sizeof(script->size);
		memcpy(bp, script->data, script->size);
		bp += script->size;
	}

	if ((ret = dbenv->log_put(dbenv, rlsnp, (DBT *)&logrec,
	    flags | DB_LOG_NOCOPY)) == 0 && txnp != NULL) {
		*lsnp = *rlsnp;
		if (rlsnp != ret_lsnp)
			 *ret_lsnp = *rlsnp;
	}
#ifdef LOG_DIAGNOSTIC
	if (ret != 0)
		(void)logio_Posttrans_print(dbenv,
		    (DBT *)&logrec, ret_lsnp, DB_TXN_PRINT);
#endif

	free(logrec.data);
	return (ret);
}

