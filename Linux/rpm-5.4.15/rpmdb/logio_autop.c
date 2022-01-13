/* Do not edit: automatically built by logio.awk. */

#include "system.h"

#include "db.h"

#include <rpmio.h>
#include "logio.h"
#include "debug.h"

/*
 * PUBLIC: int logio_Creat_print __P((DB_ENV *, DBT *, DB_LSN *, db_recops));
 */
int
logio_Creat_print(DB_ENV * dbenv, DBT * dbtp, DB_LSN * lsnp, db_recops notused2)
{
	logio_Creat_args *argp;
	int logio_Creat_read __P((DB_ENV *, void *, logio_Creat_args **));
	uint32_t i;
	int ch;
	int ret;

	notused2 = DB_TXN_PRINT;

	if ((ret = logio_Creat_read(dbenv, dbtp->data, &argp)) != 0)
		return (ret);
	(void)printf(
    "[%lu][%lu]logio_Creat%s: rec: %lu txnp %lx prevlsn [%lu][%lu]\n",
	    (u_long)lsnp->file, (u_long)lsnp->offset,
	    (argp->type & DB_debug_FLAG) ? "_debug" : "",
	    (u_long)argp->type,
	    (u_long)argp->txnp->txnid,
	    (u_long)argp->prev_lsn.file, (u_long)argp->prev_lsn.offset);
	(void)printf("\tpath: ");
	for (i = 0; i < argp->path.size; i++) {
		ch = ((uint8_t *)argp->path.data)[i];
		printf(isprint(ch) || ch == 0x0a ? "%c" : "%#x ", ch);
	}
	(void)printf("\n");
	(void)printf("\tmode: %o\n", argp->mode);
	(void)printf("\tcontent: ");
	for (i = 0; i < argp->content.size; i++) {
		ch = ((uint8_t *)argp->content.data)[i];
		printf(isprint(ch) || ch == 0x0a ? "%c" : "%#x ", ch);
	}
	(void)printf("\n");
	(void)printf("\tdigest: ");
	for (i = 0; i < argp->digest.size; i++) {
		ch = ((uint8_t *)argp->digest.data)[i];
		printf(isprint(ch) || ch == 0x0a ? "%c" : "%#x ", ch);
	}
	(void)printf("\n");
	(void)printf("\tdalgo: %lu\n", (u_long)argp->dalgo);
	(void)printf("\n");
	free(argp);
	return (0);
}

/*
 * PUBLIC: int logio_Unlink_print __P((DB_ENV *, DBT *, DB_LSN *, db_recops));
 */
int
logio_Unlink_print(DB_ENV * dbenv, DBT * dbtp, DB_LSN * lsnp, db_recops notused2)
{
	logio_Unlink_args *argp;
	int logio_Unlink_read __P((DB_ENV *, void *, logio_Unlink_args **));
	uint32_t i;
	int ch;
	int ret;

	notused2 = DB_TXN_PRINT;

	if ((ret = logio_Unlink_read(dbenv, dbtp->data, &argp)) != 0)
		return (ret);
	(void)printf(
    "[%lu][%lu]logio_Unlink%s: rec: %lu txnp %lx prevlsn [%lu][%lu]\n",
	    (u_long)lsnp->file, (u_long)lsnp->offset,
	    (argp->type & DB_debug_FLAG) ? "_debug" : "",
	    (u_long)argp->type,
	    (u_long)argp->txnp->txnid,
	    (u_long)argp->prev_lsn.file, (u_long)argp->prev_lsn.offset);
	(void)printf("\tpath: ");
	for (i = 0; i < argp->path.size; i++) {
		ch = ((uint8_t *)argp->path.data)[i];
		printf(isprint(ch) || ch == 0x0a ? "%c" : "%#x ", ch);
	}
	(void)printf("\n");
	(void)printf("\tmode: %o\n", argp->mode);
	(void)printf("\tcontent: ");
	for (i = 0; i < argp->content.size; i++) {
		ch = ((uint8_t *)argp->content.data)[i];
		printf(isprint(ch) || ch == 0x0a ? "%c" : "%#x ", ch);
	}
	(void)printf("\n");
	(void)printf("\tdigest: ");
	for (i = 0; i < argp->digest.size; i++) {
		ch = ((uint8_t *)argp->digest.data)[i];
		printf(isprint(ch) || ch == 0x0a ? "%c" : "%#x ", ch);
	}
	(void)printf("\n");
	(void)printf("\tdalgo: %lu\n", (u_long)argp->dalgo);
	(void)printf("\n");
	free(argp);
	return (0);
}

/*
 * PUBLIC: int logio_Rename_print __P((DB_ENV *, DBT *, DB_LSN *, db_recops));
 */
int
logio_Rename_print(DB_ENV * dbenv, DBT * dbtp, DB_LSN * lsnp, db_recops notused2)
{
	logio_Rename_args *argp;
	int logio_Rename_read __P((DB_ENV *, void *, logio_Rename_args **));
	uint32_t i;
	int ch;
	int ret;

	notused2 = DB_TXN_PRINT;

	if ((ret = logio_Rename_read(dbenv, dbtp->data, &argp)) != 0)
		return (ret);
	(void)printf(
    "[%lu][%lu]logio_Rename%s: rec: %lu txnp %lx prevlsn [%lu][%lu]\n",
	    (u_long)lsnp->file, (u_long)lsnp->offset,
	    (argp->type & DB_debug_FLAG) ? "_debug" : "",
	    (u_long)argp->type,
	    (u_long)argp->txnp->txnid,
	    (u_long)argp->prev_lsn.file, (u_long)argp->prev_lsn.offset);
	(void)printf("\toldpath: ");
	for (i = 0; i < argp->oldpath.size; i++) {
		ch = ((uint8_t *)argp->oldpath.data)[i];
		printf(isprint(ch) || ch == 0x0a ? "%c" : "%#x ", ch);
	}
	(void)printf("\n");
	(void)printf("\tnewpath: ");
	for (i = 0; i < argp->newpath.size; i++) {
		ch = ((uint8_t *)argp->newpath.data)[i];
		printf(isprint(ch) || ch == 0x0a ? "%c" : "%#x ", ch);
	}
	(void)printf("\n");
	(void)printf("\tmode: %o\n", argp->mode);
	(void)printf("\tcontent: ");
	for (i = 0; i < argp->content.size; i++) {
		ch = ((uint8_t *)argp->content.data)[i];
		printf(isprint(ch) || ch == 0x0a ? "%c" : "%#x ", ch);
	}
	(void)printf("\n");
	(void)printf("\tdigest: ");
	for (i = 0; i < argp->digest.size; i++) {
		ch = ((uint8_t *)argp->digest.data)[i];
		printf(isprint(ch) || ch == 0x0a ? "%c" : "%#x ", ch);
	}
	(void)printf("\n");
	(void)printf("\tdalgo: %lu\n", (u_long)argp->dalgo);
	(void)printf("\n");
	free(argp);
	return (0);
}

/*
 * PUBLIC: int logio_Mkdir_print __P((DB_ENV *, DBT *, DB_LSN *, db_recops));
 */
int
logio_Mkdir_print(DB_ENV * dbenv, DBT * dbtp, DB_LSN * lsnp, db_recops notused2)
{
	logio_Mkdir_args *argp;
	int logio_Mkdir_read __P((DB_ENV *, void *, logio_Mkdir_args **));
	uint32_t i;
	int ch;
	int ret;

	notused2 = DB_TXN_PRINT;

	if ((ret = logio_Mkdir_read(dbenv, dbtp->data, &argp)) != 0)
		return (ret);
	(void)printf(
    "[%lu][%lu]logio_Mkdir%s: rec: %lu txnp %lx prevlsn [%lu][%lu]\n",
	    (u_long)lsnp->file, (u_long)lsnp->offset,
	    (argp->type & DB_debug_FLAG) ? "_debug" : "",
	    (u_long)argp->type,
	    (u_long)argp->txnp->txnid,
	    (u_long)argp->prev_lsn.file, (u_long)argp->prev_lsn.offset);
	(void)printf("\tpath: ");
	for (i = 0; i < argp->path.size; i++) {
		ch = ((uint8_t *)argp->path.data)[i];
		printf(isprint(ch) || ch == 0x0a ? "%c" : "%#x ", ch);
	}
	(void)printf("\n");
	(void)printf("\tmode: %o\n", argp->mode);
	(void)printf("\n");
	free(argp);
	return (0);
}

/*
 * PUBLIC: int logio_Rmdir_print __P((DB_ENV *, DBT *, DB_LSN *, db_recops));
 */
int
logio_Rmdir_print(DB_ENV * dbenv, DBT * dbtp, DB_LSN * lsnp, db_recops notused2)
{
	logio_Rmdir_args *argp;
	int logio_Rmdir_read __P((DB_ENV *, void *, logio_Rmdir_args **));
	uint32_t i;
	int ch;
	int ret;

	notused2 = DB_TXN_PRINT;

	if ((ret = logio_Rmdir_read(dbenv, dbtp->data, &argp)) != 0)
		return (ret);
	(void)printf(
    "[%lu][%lu]logio_Rmdir%s: rec: %lu txnp %lx prevlsn [%lu][%lu]\n",
	    (u_long)lsnp->file, (u_long)lsnp->offset,
	    (argp->type & DB_debug_FLAG) ? "_debug" : "",
	    (u_long)argp->type,
	    (u_long)argp->txnp->txnid,
	    (u_long)argp->prev_lsn.file, (u_long)argp->prev_lsn.offset);
	(void)printf("\tpath: ");
	for (i = 0; i < argp->path.size; i++) {
		ch = ((uint8_t *)argp->path.data)[i];
		printf(isprint(ch) || ch == 0x0a ? "%c" : "%#x ", ch);
	}
	(void)printf("\n");
	(void)printf("\tmode: %o\n", argp->mode);
	(void)printf("\n");
	free(argp);
	return (0);
}

/*
 * PUBLIC: int logio_Lsetfilecon_print __P((DB_ENV *, DBT *,
 * PUBLIC:     DB_LSN *, db_recops));
 */
int
logio_Lsetfilecon_print(DB_ENV * dbenv, DBT * dbtp, DB_LSN * lsnp, db_recops notused2)
{
	logio_Lsetfilecon_args *argp;
	int logio_Lsetfilecon_read __P((DB_ENV *, void *, logio_Lsetfilecon_args **));
	uint32_t i;
	int ch;
	int ret;

	notused2 = DB_TXN_PRINT;

	if ((ret = logio_Lsetfilecon_read(dbenv, dbtp->data, &argp)) != 0)
		return (ret);
	(void)printf(
    "[%lu][%lu]logio_Lsetfilecon%s: rec: %lu txnp %lx prevlsn [%lu][%lu]\n",
	    (u_long)lsnp->file, (u_long)lsnp->offset,
	    (argp->type & DB_debug_FLAG) ? "_debug" : "",
	    (u_long)argp->type,
	    (u_long)argp->txnp->txnid,
	    (u_long)argp->prev_lsn.file, (u_long)argp->prev_lsn.offset);
	(void)printf("\tpath: ");
	for (i = 0; i < argp->path.size; i++) {
		ch = ((uint8_t *)argp->path.data)[i];
		printf(isprint(ch) || ch == 0x0a ? "%c" : "%#x ", ch);
	}
	(void)printf("\n");
	(void)printf("\tcontext: ");
	for (i = 0; i < argp->context.size; i++) {
		ch = ((uint8_t *)argp->context.data)[i];
		printf(isprint(ch) || ch == 0x0a ? "%c" : "%#x ", ch);
	}
	(void)printf("\n");
	(void)printf("\n");
	free(argp);
	return (0);
}

/*
 * PUBLIC: int logio_Chown_print __P((DB_ENV *, DBT *, DB_LSN *, db_recops));
 */
int
logio_Chown_print(DB_ENV * dbenv, DBT * dbtp, DB_LSN * lsnp, db_recops notused2)
{
	logio_Chown_args *argp;
	int logio_Chown_read __P((DB_ENV *, void *, logio_Chown_args **));
	uint32_t i;
	int ch;
	int ret;

	notused2 = DB_TXN_PRINT;

	if ((ret = logio_Chown_read(dbenv, dbtp->data, &argp)) != 0)
		return (ret);
	(void)printf(
    "[%lu][%lu]logio_Chown%s: rec: %lu txnp %lx prevlsn [%lu][%lu]\n",
	    (u_long)lsnp->file, (u_long)lsnp->offset,
	    (argp->type & DB_debug_FLAG) ? "_debug" : "",
	    (u_long)argp->type,
	    (u_long)argp->txnp->txnid,
	    (u_long)argp->prev_lsn.file, (u_long)argp->prev_lsn.offset);
	(void)printf("\tpath: ");
	for (i = 0; i < argp->path.size; i++) {
		ch = ((uint8_t *)argp->path.data)[i];
		printf(isprint(ch) || ch == 0x0a ? "%c" : "%#x ", ch);
	}
	(void)printf("\n");
	(void)printf("\towner: %lu\n", (u_long)argp->owner);
	(void)printf("\tgroup: %lu\n", (u_long)argp->group);
	(void)printf("\n");
	free(argp);
	return (0);
}

/*
 * PUBLIC: int logio_Lchown_print __P((DB_ENV *, DBT *, DB_LSN *, db_recops));
 */
int
logio_Lchown_print(DB_ENV * dbenv, DBT * dbtp, DB_LSN * lsnp, db_recops notused2)
{
	logio_Lchown_args *argp;
	int logio_Lchown_read __P((DB_ENV *, void *, logio_Lchown_args **));
	uint32_t i;
	int ch;
	int ret;

	notused2 = DB_TXN_PRINT;

	if ((ret = logio_Lchown_read(dbenv, dbtp->data, &argp)) != 0)
		return (ret);
	(void)printf(
    "[%lu][%lu]logio_Lchown%s: rec: %lu txnp %lx prevlsn [%lu][%lu]\n",
	    (u_long)lsnp->file, (u_long)lsnp->offset,
	    (argp->type & DB_debug_FLAG) ? "_debug" : "",
	    (u_long)argp->type,
	    (u_long)argp->txnp->txnid,
	    (u_long)argp->prev_lsn.file, (u_long)argp->prev_lsn.offset);
	(void)printf("\tpath: ");
	for (i = 0; i < argp->path.size; i++) {
		ch = ((uint8_t *)argp->path.data)[i];
		printf(isprint(ch) || ch == 0x0a ? "%c" : "%#x ", ch);
	}
	(void)printf("\n");
	(void)printf("\towner: %lu\n", (u_long)argp->owner);
	(void)printf("\tgroup: %lu\n", (u_long)argp->group);
	(void)printf("\n");
	free(argp);
	return (0);
}

/*
 * PUBLIC: int logio_Chmod_print __P((DB_ENV *, DBT *, DB_LSN *, db_recops));
 */
int
logio_Chmod_print(DB_ENV * dbenv, DBT * dbtp, DB_LSN * lsnp, db_recops notused2)
{
	logio_Chmod_args *argp;
	int logio_Chmod_read __P((DB_ENV *, void *, logio_Chmod_args **));
	uint32_t i;
	int ch;
	int ret;

	notused2 = DB_TXN_PRINT;

	if ((ret = logio_Chmod_read(dbenv, dbtp->data, &argp)) != 0)
		return (ret);
	(void)printf(
    "[%lu][%lu]logio_Chmod%s: rec: %lu txnp %lx prevlsn [%lu][%lu]\n",
	    (u_long)lsnp->file, (u_long)lsnp->offset,
	    (argp->type & DB_debug_FLAG) ? "_debug" : "",
	    (u_long)argp->type,
	    (u_long)argp->txnp->txnid,
	    (u_long)argp->prev_lsn.file, (u_long)argp->prev_lsn.offset);
	(void)printf("\tpath: ");
	for (i = 0; i < argp->path.size; i++) {
		ch = ((uint8_t *)argp->path.data)[i];
		printf(isprint(ch) || ch == 0x0a ? "%c" : "%#x ", ch);
	}
	(void)printf("\n");
	(void)printf("\tmode: %o\n", argp->mode);
	(void)printf("\n");
	free(argp);
	return (0);
}

/*
 * PUBLIC: int logio_Utime_print __P((DB_ENV *, DBT *, DB_LSN *, db_recops));
 */
int
logio_Utime_print(DB_ENV * dbenv, DBT * dbtp, DB_LSN * lsnp, db_recops notused2)
{
	logio_Utime_args *argp;
	int logio_Utime_read __P((DB_ENV *, void *, logio_Utime_args **));
	struct tm *lt;
	time_t timeval;
	char time_buf[24+1+1];
	uint32_t i;
	int ch;
	int ret;

	notused2 = DB_TXN_PRINT;

	if ((ret = logio_Utime_read(dbenv, dbtp->data, &argp)) != 0)
		return (ret);
	(void)printf(
    "[%lu][%lu]logio_Utime%s: rec: %lu txnp %lx prevlsn [%lu][%lu]\n",
	    (u_long)lsnp->file, (u_long)lsnp->offset,
	    (argp->type & DB_debug_FLAG) ? "_debug" : "",
	    (u_long)argp->type,
	    (u_long)argp->txnp->txnid,
	    (u_long)argp->prev_lsn.file, (u_long)argp->prev_lsn.offset);
	(void)printf("\tpath: ");
	for (i = 0; i < argp->path.size; i++) {
		ch = ((uint8_t *)argp->path.data)[i];
		printf(isprint(ch) || ch == 0x0a ? "%c" : "%#x ", ch);
	}
	(void)printf("\n");
	timeval = (time_t)argp->actime;
	lt = localtime(&timeval);
	(void)printf(
	    "\tactime: %lu (%.24s, 20%02lu%02lu%02lu%02lu%02lu.%02lu)\n",
	    (long)argp->actime, (char *)ctime_r(&timeval, time_buf),
	    (u_long)lt->tm_year - 100, (u_long)lt->tm_mon+1,
	    (u_long)lt->tm_mday, (u_long)lt->tm_hour,
	    (u_long)lt->tm_min, (u_long)lt->tm_sec);
	timeval = (time_t)argp->modtime;
	lt = localtime(&timeval);
	(void)printf(
	    "\tmodtime: %lu (%.24s, 20%02lu%02lu%02lu%02lu%02lu.%02lu)\n",
	    (long)argp->modtime, (char *)ctime_r(&timeval, time_buf),
	    (u_long)lt->tm_year - 100, (u_long)lt->tm_mon+1,
	    (u_long)lt->tm_mday, (u_long)lt->tm_hour,
	    (u_long)lt->tm_min, (u_long)lt->tm_sec);
	(void)printf("\n");
	free(argp);
	return (0);
}

/*
 * PUBLIC: int logio_Symlink_print __P((DB_ENV *, DBT *, DB_LSN *, db_recops));
 */
int
logio_Symlink_print(DB_ENV * dbenv, DBT * dbtp, DB_LSN * lsnp, db_recops notused2)
{
	logio_Symlink_args *argp;
	int logio_Symlink_read __P((DB_ENV *, void *, logio_Symlink_args **));
	uint32_t i;
	int ch;
	int ret;

	notused2 = DB_TXN_PRINT;

	if ((ret = logio_Symlink_read(dbenv, dbtp->data, &argp)) != 0)
		return (ret);
	(void)printf(
    "[%lu][%lu]logio_Symlink%s: rec: %lu txnp %lx prevlsn [%lu][%lu]\n",
	    (u_long)lsnp->file, (u_long)lsnp->offset,
	    (argp->type & DB_debug_FLAG) ? "_debug" : "",
	    (u_long)argp->type,
	    (u_long)argp->txnp->txnid,
	    (u_long)argp->prev_lsn.file, (u_long)argp->prev_lsn.offset);
	(void)printf("\toldpath: ");
	for (i = 0; i < argp->oldpath.size; i++) {
		ch = ((uint8_t *)argp->oldpath.data)[i];
		printf(isprint(ch) || ch == 0x0a ? "%c" : "%#x ", ch);
	}
	(void)printf("\n");
	(void)printf("\tnewpath: ");
	for (i = 0; i < argp->newpath.size; i++) {
		ch = ((uint8_t *)argp->newpath.data)[i];
		printf(isprint(ch) || ch == 0x0a ? "%c" : "%#x ", ch);
	}
	(void)printf("\n");
	(void)printf("\n");
	free(argp);
	return (0);
}

/*
 * PUBLIC: int logio_Link_print __P((DB_ENV *, DBT *, DB_LSN *, db_recops));
 */
int
logio_Link_print(DB_ENV * dbenv, DBT * dbtp, DB_LSN * lsnp, db_recops notused2)
{
	logio_Link_args *argp;
	int logio_Link_read __P((DB_ENV *, void *, logio_Link_args **));
	uint32_t i;
	int ch;
	int ret;

	notused2 = DB_TXN_PRINT;

	if ((ret = logio_Link_read(dbenv, dbtp->data, &argp)) != 0)
		return (ret);
	(void)printf(
    "[%lu][%lu]logio_Link%s: rec: %lu txnp %lx prevlsn [%lu][%lu]\n",
	    (u_long)lsnp->file, (u_long)lsnp->offset,
	    (argp->type & DB_debug_FLAG) ? "_debug" : "",
	    (u_long)argp->type,
	    (u_long)argp->txnp->txnid,
	    (u_long)argp->prev_lsn.file, (u_long)argp->prev_lsn.offset);
	(void)printf("\toldpath: ");
	for (i = 0; i < argp->oldpath.size; i++) {
		ch = ((uint8_t *)argp->oldpath.data)[i];
		printf(isprint(ch) || ch == 0x0a ? "%c" : "%#x ", ch);
	}
	(void)printf("\n");
	(void)printf("\tnewpath: ");
	for (i = 0; i < argp->newpath.size; i++) {
		ch = ((uint8_t *)argp->newpath.data)[i];
		printf(isprint(ch) || ch == 0x0a ? "%c" : "%#x ", ch);
	}
	(void)printf("\n");
	(void)printf("\n");
	free(argp);
	return (0);
}

/*
 * PUBLIC: int logio_Mknod_print __P((DB_ENV *, DBT *, DB_LSN *, db_recops));
 */
int
logio_Mknod_print(DB_ENV * dbenv, DBT * dbtp, DB_LSN * lsnp, db_recops notused2)
{
	logio_Mknod_args *argp;
	int logio_Mknod_read __P((DB_ENV *, void *, logio_Mknod_args **));
	uint32_t i;
	int ch;
	int ret;

	notused2 = DB_TXN_PRINT;

	if ((ret = logio_Mknod_read(dbenv, dbtp->data, &argp)) != 0)
		return (ret);
	(void)printf(
    "[%lu][%lu]logio_Mknod%s: rec: %lu txnp %lx prevlsn [%lu][%lu]\n",
	    (u_long)lsnp->file, (u_long)lsnp->offset,
	    (argp->type & DB_debug_FLAG) ? "_debug" : "",
	    (u_long)argp->type,
	    (u_long)argp->txnp->txnid,
	    (u_long)argp->prev_lsn.file, (u_long)argp->prev_lsn.offset);
	(void)printf("\tpath: ");
	for (i = 0; i < argp->path.size; i++) {
		ch = ((uint8_t *)argp->path.data)[i];
		printf(isprint(ch) || ch == 0x0a ? "%c" : "%#x ", ch);
	}
	(void)printf("\n");
	(void)printf("\tmode: %o\n", argp->mode);
	(void)printf("\tdev: 0x%lx\n", (u_long)argp->dev);
	(void)printf("\n");
	free(argp);
	return (0);
}

/*
 * PUBLIC: int logio_Mkfifo_print __P((DB_ENV *, DBT *, DB_LSN *, db_recops));
 */
int
logio_Mkfifo_print(DB_ENV * dbenv, DBT * dbtp, DB_LSN * lsnp, db_recops notused2)
{
	logio_Mkfifo_args *argp;
	int logio_Mkfifo_read __P((DB_ENV *, void *, logio_Mkfifo_args **));
	uint32_t i;
	int ch;
	int ret;

	notused2 = DB_TXN_PRINT;

	if ((ret = logio_Mkfifo_read(dbenv, dbtp->data, &argp)) != 0)
		return (ret);
	(void)printf(
    "[%lu][%lu]logio_Mkfifo%s: rec: %lu txnp %lx prevlsn [%lu][%lu]\n",
	    (u_long)lsnp->file, (u_long)lsnp->offset,
	    (argp->type & DB_debug_FLAG) ? "_debug" : "",
	    (u_long)argp->type,
	    (u_long)argp->txnp->txnid,
	    (u_long)argp->prev_lsn.file, (u_long)argp->prev_lsn.offset);
	(void)printf("\tpath: ");
	for (i = 0; i < argp->path.size; i++) {
		ch = ((uint8_t *)argp->path.data)[i];
		printf(isprint(ch) || ch == 0x0a ? "%c" : "%#x ", ch);
	}
	(void)printf("\n");
	(void)printf("\tmode: %o\n", argp->mode);
	(void)printf("\n");
	free(argp);
	return (0);
}

/*
 * PUBLIC: int logio_Pretrans_print __P((DB_ENV *, DBT *, DB_LSN *,
 * PUBLIC:     db_recops));
 */
int
logio_Pretrans_print(DB_ENV * dbenv, DBT * dbtp, DB_LSN * lsnp, db_recops notused2)
{
	logio_Pretrans_args *argp;
	int logio_Pretrans_read __P((DB_ENV *, void *, logio_Pretrans_args **));
	uint32_t i;
	int ch;
	int ret;

	notused2 = DB_TXN_PRINT;

	if ((ret = logio_Pretrans_read(dbenv, dbtp->data, &argp)) != 0)
		return (ret);
	(void)printf(
    "[%lu][%lu]logio_Pretrans%s: rec: %lu txnp %lx prevlsn [%lu][%lu]\n",
	    (u_long)lsnp->file, (u_long)lsnp->offset,
	    (argp->type & DB_debug_FLAG) ? "_debug" : "",
	    (u_long)argp->type,
	    (u_long)argp->txnp->txnid,
	    (u_long)argp->prev_lsn.file, (u_long)argp->prev_lsn.offset);
	(void)printf("\tcmd: ");
	for (i = 0; i < argp->cmd.size; i++) {
		ch = ((uint8_t *)argp->cmd.data)[i];
		printf(isprint(ch) || ch == 0x0a ? "%c" : "%#x ", ch);
	}
	(void)printf("\n");
	(void)printf("\tscript: ");
	for (i = 0; i < argp->script.size; i++) {
		ch = ((uint8_t *)argp->script.data)[i];
		printf(isprint(ch) || ch == 0x0a ? "%c" : "%#x ", ch);
	}
	(void)printf("\n");
	(void)printf("\n");
	free(argp);
	return (0);
}

/*
 * PUBLIC: int logio_Prein_print __P((DB_ENV *, DBT *, DB_LSN *, db_recops));
 */
int
logio_Prein_print(DB_ENV * dbenv, DBT * dbtp, DB_LSN * lsnp, db_recops notused2)
{
	logio_Prein_args *argp;
	int logio_Prein_read __P((DB_ENV *, void *, logio_Prein_args **));
	uint32_t i;
	int ch;
	int ret;

	notused2 = DB_TXN_PRINT;

	if ((ret = logio_Prein_read(dbenv, dbtp->data, &argp)) != 0)
		return (ret);
	(void)printf(
    "[%lu][%lu]logio_Prein%s: rec: %lu txnp %lx prevlsn [%lu][%lu]\n",
	    (u_long)lsnp->file, (u_long)lsnp->offset,
	    (argp->type & DB_debug_FLAG) ? "_debug" : "",
	    (u_long)argp->type,
	    (u_long)argp->txnp->txnid,
	    (u_long)argp->prev_lsn.file, (u_long)argp->prev_lsn.offset);
	(void)printf("\tcmd: ");
	for (i = 0; i < argp->cmd.size; i++) {
		ch = ((uint8_t *)argp->cmd.data)[i];
		printf(isprint(ch) || ch == 0x0a ? "%c" : "%#x ", ch);
	}
	(void)printf("\n");
	(void)printf("\tscript: ");
	for (i = 0; i < argp->script.size; i++) {
		ch = ((uint8_t *)argp->script.data)[i];
		printf(isprint(ch) || ch == 0x0a ? "%c" : "%#x ", ch);
	}
	(void)printf("\n");
	(void)printf("\n");
	free(argp);
	return (0);
}

/*
 * PUBLIC: int logio_Postin_print __P((DB_ENV *, DBT *, DB_LSN *, db_recops));
 */
int
logio_Postin_print(DB_ENV * dbenv, DBT * dbtp, DB_LSN * lsnp, db_recops notused2)
{
	logio_Postin_args *argp;
	int logio_Postin_read __P((DB_ENV *, void *, logio_Postin_args **));
	uint32_t i;
	int ch;
	int ret;

	notused2 = DB_TXN_PRINT;

	if ((ret = logio_Postin_read(dbenv, dbtp->data, &argp)) != 0)
		return (ret);
	(void)printf(
    "[%lu][%lu]logio_Postin%s: rec: %lu txnp %lx prevlsn [%lu][%lu]\n",
	    (u_long)lsnp->file, (u_long)lsnp->offset,
	    (argp->type & DB_debug_FLAG) ? "_debug" : "",
	    (u_long)argp->type,
	    (u_long)argp->txnp->txnid,
	    (u_long)argp->prev_lsn.file, (u_long)argp->prev_lsn.offset);
	(void)printf("\tcmd: ");
	for (i = 0; i < argp->cmd.size; i++) {
		ch = ((uint8_t *)argp->cmd.data)[i];
		printf(isprint(ch) || ch == 0x0a ? "%c" : "%#x ", ch);
	}
	(void)printf("\n");
	(void)printf("\tscript: ");
	for (i = 0; i < argp->script.size; i++) {
		ch = ((uint8_t *)argp->script.data)[i];
		printf(isprint(ch) || ch == 0x0a ? "%c" : "%#x ", ch);
	}
	(void)printf("\n");
	(void)printf("\n");
	free(argp);
	return (0);
}

/*
 * PUBLIC: int logio_Preun_print __P((DB_ENV *, DBT *, DB_LSN *, db_recops));
 */
int
logio_Preun_print(DB_ENV * dbenv, DBT * dbtp, DB_LSN * lsnp, db_recops notused2)
{
	logio_Preun_args *argp;
	int logio_Preun_read __P((DB_ENV *, void *, logio_Preun_args **));
	uint32_t i;
	int ch;
	int ret;

	notused2 = DB_TXN_PRINT;

	if ((ret = logio_Preun_read(dbenv, dbtp->data, &argp)) != 0)
		return (ret);
	(void)printf(
    "[%lu][%lu]logio_Preun%s: rec: %lu txnp %lx prevlsn [%lu][%lu]\n",
	    (u_long)lsnp->file, (u_long)lsnp->offset,
	    (argp->type & DB_debug_FLAG) ? "_debug" : "",
	    (u_long)argp->type,
	    (u_long)argp->txnp->txnid,
	    (u_long)argp->prev_lsn.file, (u_long)argp->prev_lsn.offset);
	(void)printf("\tcmd: ");
	for (i = 0; i < argp->cmd.size; i++) {
		ch = ((uint8_t *)argp->cmd.data)[i];
		printf(isprint(ch) || ch == 0x0a ? "%c" : "%#x ", ch);
	}
	(void)printf("\n");
	(void)printf("\tscript: ");
	for (i = 0; i < argp->script.size; i++) {
		ch = ((uint8_t *)argp->script.data)[i];
		printf(isprint(ch) || ch == 0x0a ? "%c" : "%#x ", ch);
	}
	(void)printf("\n");
	(void)printf("\n");
	free(argp);
	return (0);
}

/*
 * PUBLIC: int logio_Postun_print __P((DB_ENV *, DBT *, DB_LSN *, db_recops));
 */
int
logio_Postun_print(DB_ENV * dbenv, DBT * dbtp, DB_LSN * lsnp, db_recops notused2)
{
	logio_Postun_args *argp;
	int logio_Postun_read __P((DB_ENV *, void *, logio_Postun_args **));
	uint32_t i;
	int ch;
	int ret;

	notused2 = DB_TXN_PRINT;

	if ((ret = logio_Postun_read(dbenv, dbtp->data, &argp)) != 0)
		return (ret);
	(void)printf(
    "[%lu][%lu]logio_Postun%s: rec: %lu txnp %lx prevlsn [%lu][%lu]\n",
	    (u_long)lsnp->file, (u_long)lsnp->offset,
	    (argp->type & DB_debug_FLAG) ? "_debug" : "",
	    (u_long)argp->type,
	    (u_long)argp->txnp->txnid,
	    (u_long)argp->prev_lsn.file, (u_long)argp->prev_lsn.offset);
	(void)printf("\tcmd: ");
	for (i = 0; i < argp->cmd.size; i++) {
		ch = ((uint8_t *)argp->cmd.data)[i];
		printf(isprint(ch) || ch == 0x0a ? "%c" : "%#x ", ch);
	}
	(void)printf("\n");
	(void)printf("\tscript: ");
	for (i = 0; i < argp->script.size; i++) {
		ch = ((uint8_t *)argp->script.data)[i];
		printf(isprint(ch) || ch == 0x0a ? "%c" : "%#x ", ch);
	}
	(void)printf("\n");
	(void)printf("\n");
	free(argp);
	return (0);
}

/*
 * PUBLIC: int logio_Posttrans_print __P((DB_ENV *, DBT *, DB_LSN *,
 * PUBLIC:     db_recops));
 */
int
logio_Posttrans_print(DB_ENV * dbenv, DBT * dbtp, DB_LSN * lsnp, db_recops notused2)
{
	logio_Posttrans_args *argp;
	int logio_Posttrans_read __P((DB_ENV *, void *, logio_Posttrans_args **));
	uint32_t i;
	int ch;
	int ret;

	notused2 = DB_TXN_PRINT;

	if ((ret = logio_Posttrans_read(dbenv, dbtp->data, &argp)) != 0)
		return (ret);
	(void)printf(
    "[%lu][%lu]logio_Posttrans%s: rec: %lu txnp %lx prevlsn [%lu][%lu]\n",
	    (u_long)lsnp->file, (u_long)lsnp->offset,
	    (argp->type & DB_debug_FLAG) ? "_debug" : "",
	    (u_long)argp->type,
	    (u_long)argp->txnp->txnid,
	    (u_long)argp->prev_lsn.file, (u_long)argp->prev_lsn.offset);
	(void)printf("\tcmd: ");
	for (i = 0; i < argp->cmd.size; i++) {
		ch = ((uint8_t *)argp->cmd.data)[i];
		printf(isprint(ch) || ch == 0x0a ? "%c" : "%#x ", ch);
	}
	(void)printf("\n");
	(void)printf("\tscript: ");
	for (i = 0; i < argp->script.size; i++) {
		ch = ((uint8_t *)argp->script.data)[i];
		printf(isprint(ch) || ch == 0x0a ? "%c" : "%#x ", ch);
	}
	(void)printf("\n");
	(void)printf("\n");
	free(argp);
	return (0);
}

/*
 * PUBLIC: int logio_init_print __P((DB_ENV *, DB_DISTAB *));
 */
int
logio_init_print(DB_ENV * dbenv, DB_DISTAB * dtabp)
{
	#define __db_add_recovery_rpmdb __db_add_recovery
	int __db_add_recovery_rpmdb __P((DB_ENV *, DB_DISTAB *,
	    int (*)(DB_ENV *, DBT *, DB_LSN *, db_recops), uint32_t));
	int ret;

	if ((ret = __db_add_recovery_rpmdb(dbenv, dtabp,
	    logio_Creat_print, DB_logio_Creat)) != 0)
		return (ret);
	if ((ret = __db_add_recovery_rpmdb(dbenv, dtabp,
	    logio_Unlink_print, DB_logio_Unlink)) != 0)
		return (ret);
	if ((ret = __db_add_recovery_rpmdb(dbenv, dtabp,
	    logio_Rename_print, DB_logio_Rename)) != 0)
		return (ret);
	if ((ret = __db_add_recovery_rpmdb(dbenv, dtabp,
	    logio_Mkdir_print, DB_logio_Mkdir)) != 0)
		return (ret);
	if ((ret = __db_add_recovery_rpmdb(dbenv, dtabp,
	    logio_Rmdir_print, DB_logio_Rmdir)) != 0)
		return (ret);
	if ((ret = __db_add_recovery_rpmdb(dbenv, dtabp,
	    logio_Lsetfilecon_print, DB_logio_Lsetfilecon)) != 0)
		return (ret);
	if ((ret = __db_add_recovery_rpmdb(dbenv, dtabp,
	    logio_Chown_print, DB_logio_Chown)) != 0)
		return (ret);
	if ((ret = __db_add_recovery_rpmdb(dbenv, dtabp,
	    logio_Lchown_print, DB_logio_Lchown)) != 0)
		return (ret);
	if ((ret = __db_add_recovery_rpmdb(dbenv, dtabp,
	    logio_Chmod_print, DB_logio_Chmod)) != 0)
		return (ret);
	if ((ret = __db_add_recovery_rpmdb(dbenv, dtabp,
	    logio_Utime_print, DB_logio_Utime)) != 0)
		return (ret);
	if ((ret = __db_add_recovery_rpmdb(dbenv, dtabp,
	    logio_Symlink_print, DB_logio_Symlink)) != 0)
		return (ret);
	if ((ret = __db_add_recovery_rpmdb(dbenv, dtabp,
	    logio_Link_print, DB_logio_Link)) != 0)
		return (ret);
	if ((ret = __db_add_recovery_rpmdb(dbenv, dtabp,
	    logio_Mknod_print, DB_logio_Mknod)) != 0)
		return (ret);
	if ((ret = __db_add_recovery_rpmdb(dbenv, dtabp,
	    logio_Mkfifo_print, DB_logio_Mkfifo)) != 0)
		return (ret);
	if ((ret = __db_add_recovery_rpmdb(dbenv, dtabp,
	    logio_Pretrans_print, DB_logio_Pretrans)) != 0)
		return (ret);
	if ((ret = __db_add_recovery_rpmdb(dbenv, dtabp,
	    logio_Prein_print, DB_logio_Prein)) != 0)
		return (ret);
	if ((ret = __db_add_recovery_rpmdb(dbenv, dtabp,
	    logio_Postin_print, DB_logio_Postin)) != 0)
		return (ret);
	if ((ret = __db_add_recovery_rpmdb(dbenv, dtabp,
	    logio_Preun_print, DB_logio_Preun)) != 0)
		return (ret);
	if ((ret = __db_add_recovery_rpmdb(dbenv, dtabp,
	    logio_Postun_print, DB_logio_Postun)) != 0)
		return (ret);
	if ((ret = __db_add_recovery_rpmdb(dbenv, dtabp,
	    logio_Posttrans_print, DB_logio_Posttrans)) != 0)
		return (ret);
	return (0);
}
