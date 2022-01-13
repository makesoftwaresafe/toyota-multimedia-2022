/** \ingroup rpmdb
 * \file rpmdb/rpmlio.c
 */

#include "system.h"

#include <rpmiotypes.h>
#include <rpmtypes.h>
#include <argv.h>

#include <rpmtag.h>
#define _RPMDB_INTERNAL
#include <rpmdb.h>
#include <rpmlio.h>

#include "debug.h"

/*@unchecked@*/
int _rpmlio_debug = 0;

#if defined(SUPPORT_FILE_ACID)
/*@unchecked@*/
static int _enable_syscall_logging = 0;
/*@unchecked@*/
static int _enable_scriptlet_logging = 0;
#endif	/* SUPPORT_FILE_ACID */

int rpmlioCreat(rpmdb rpmdb, const char * fn, mode_t mode,
		const uint8_t * b, size_t blen,
		const uint8_t * d, size_t dlen, uint32_t dalgo)
{
    int rc = 0;
#if defined(SUPPORT_FILE_ACID)
    extern int logio_Creat_log
        __P((DB_ENV *, DB_TXN *, DB_LSN *, uint32_t, const DBT *, mode_t, const DBT *, const DBT *, uint32_t));
    DB_ENV * dbenv = (rpmdb ? rpmdb->db_dbenv : NULL);
    DB_TXN * _txn = (rpmdb ? rpmdb->db_txn : NULL);
    DB_LSN _lsn = {0,0};
    DBT FNdbt = {0};
    DBT Bdbt = {0};
    DBT Ddbt = {0};
    if (!(dbenv && _txn && _enable_syscall_logging)) return 0;
    FNdbt.data = (void *)fn;
    FNdbt.size = strlen(fn) + 1;	/* trailing NUL too */
    Bdbt.data = (void *)b;
    Bdbt.size = blen;
    Ddbt.data = (void *)d;
    Ddbt.size = dlen;
    rc = logio_Creat_log(dbenv, _txn, &_lsn, DB_FLUSH, &FNdbt, mode, &Bdbt, &Ddbt, dalgo);
if (_rpmlio_debug)
fprintf(stderr, "<== %s(%s, 0%o, %p[%u], %p[%u], %u) rc %d\n", __FUNCTION__, fn, mode, b, (unsigned)blen, d, (unsigned)dlen, (unsigned)dalgo, rc);
#endif	/* SUPPORT_FILE_ACID */
    return rc;
}

int rpmlioUnlink(rpmdb rpmdb, const char * fn, mode_t mode,
		const uint8_t * b, size_t blen,
		const uint8_t * d, size_t dlen, uint32_t dalgo)
{
    int rc = 0;
#if defined(SUPPORT_FILE_ACID)
    extern int logio_Unlink_log
        __P((DB_ENV *, DB_TXN *, DB_LSN *, uint32_t, const DBT *, mode_t, const DBT *, const DBT *, uint32_t));
    DB_ENV * dbenv = (rpmdb ? rpmdb->db_dbenv : NULL);
    DB_TXN * _txn = (rpmdb ? rpmdb->db_txn : NULL);
    DB_LSN _lsn = {0,0};
    DBT FNdbt = {0};
    DBT Bdbt = {0};
    DBT Ddbt = {0};
    if (!(dbenv && _txn && _enable_syscall_logging)) return 0;
    FNdbt.data = (void *)fn;
    FNdbt.size = strlen(fn) + 1;	/* trailing NUL too */
    Bdbt.data = (void *)b;
    Bdbt.size = blen;
    Ddbt.data = (void *)d;
    Ddbt.size = dlen;
    rc = logio_Unlink_log(dbenv, _txn, &_lsn, DB_FLUSH, &FNdbt, mode, &Bdbt, &Ddbt, dalgo);
if (_rpmlio_debug)
fprintf(stderr, "<== %s(%s, 0%o, %p[%u], %p[%u], %u) rc %d\n", __FUNCTION__, fn, mode, b, (unsigned)blen, d, (unsigned)dlen, (unsigned)dalgo, rc);
#endif	/* SUPPORT_FILE_ACID */
    return rc;
}

int rpmlioRename(rpmdb rpmdb, const char * oldname, const char * newname,
		mode_t mode,
		const uint8_t * b, size_t blen,
		const uint8_t * d, size_t dlen, uint32_t dalgo)
{
    int rc = 0;
#if defined(SUPPORT_FILE_ACID)
    extern int logio_Rename_log
        __P((DB_ENV *, DB_TXN *, DB_LSN *, uint32_t, const DBT *, const DBT *, mode_t, const DBT *, const DBT *, uint32_t));
    DB_ENV * dbenv = (rpmdb ? rpmdb->db_dbenv : NULL);
    DB_TXN * _txn = (rpmdb ? rpmdb->db_txn : NULL);
    DB_LSN _lsn = {0,0};
    DBT ONdbt = {0};
    DBT NNdbt = {0};
    DBT Bdbt = {0};
    DBT Ddbt = {0};
    if (!(dbenv && _txn && _enable_syscall_logging)) return 0;
    ONdbt.data = (void *)oldname;
    ONdbt.size = strlen(oldname) + 1;	/* trailing NUL too */
    NNdbt.data = (void *)newname;
    NNdbt.size = strlen(newname) + 1;	/* trailing NUL too */
    Bdbt.data = (void *)b;
    Bdbt.size = blen;
    Ddbt.data = (void *)d;
    Ddbt.size = dlen;
    rc = logio_Rename_log(dbenv, _txn, &_lsn, DB_FLUSH, &ONdbt, &NNdbt, mode, &Bdbt, &Ddbt, dalgo);
if (_rpmlio_debug)
fprintf(stderr, "<== %s(%s, %s, 0%o, %p[%u], %p[%u], %u) rc %d\n", __FUNCTION__, oldname, newname, mode, b, (unsigned)blen, d, (unsigned)dlen, (unsigned)dalgo, rc);
#endif	/* SUPPORT_FILE_ACID */
    return rc;
}

int rpmlioMkdir(rpmdb rpmdb, const char * dn, mode_t mode)
{
    int rc = 0;
# if defined(SUPPORT_FILE_ACID)
    extern int logio_Mkdir_log
        __P((DB_ENV *, DB_TXN *, DB_LSN *, uint32_t, const DBT *, mode_t));
    DB_ENV * dbenv = (rpmdb ? rpmdb->db_dbenv : NULL);
    DB_TXN * _txn = (rpmdb ? rpmdb->db_txn : NULL);
    DB_LSN _lsn = {0,0};
    DBT DNdbt = {0};
    if (!(dbenv && _txn && _enable_syscall_logging)) return 0;
    DNdbt.data = (void *)dn;
    DNdbt.size = strlen(dn) + 1;	/* trailing NUL too */
    rc = logio_Mkdir_log(dbenv, _txn, &_lsn, DB_FLUSH, &DNdbt, mode);
if (_rpmlio_debug)
fprintf(stderr, "<== %s(%s, 0%o) rc %d\n", __FUNCTION__, dn, mode, rc);
#endif	/* SUPPORT_FILE_ACID */
    return rc;
}

int rpmlioRmdir(rpmdb rpmdb, const char * dn, mode_t mode)
{
    int rc = 0;
# if defined(SUPPORT_FILE_ACID)
    extern int logio_Rmdir_log
        __P((DB_ENV *, DB_TXN *, DB_LSN *, uint32_t, const DBT *, mode_t));
    DB_ENV * dbenv = (rpmdb ? rpmdb->db_dbenv : NULL);
    DB_TXN * _txn = (rpmdb ? rpmdb->db_txn : NULL);
    DB_LSN _lsn = {0,0};
    DBT DNdbt = {0};
    if (!(dbenv && _txn && _enable_syscall_logging)) return 0;
    DNdbt.data = (void *)dn;
    DNdbt.size = strlen(dn) + 1;	/* trailing NUL too */
    rc = logio_Rmdir_log(dbenv, _txn, &_lsn, DB_FLUSH, &DNdbt, mode);
if (_rpmlio_debug)
fprintf(stderr, "<== %s(%s, 0%o) rc %d\n", __FUNCTION__, dn, mode, rc);
#endif	/* SUPPORT_FILE_ACID */
    return rc;
}

int rpmlioLsetfilecon(rpmdb rpmdb, const char * fn, const char * context)
{
    int rc = 0;
# if defined(SUPPORT_FILE_ACID)
    extern int logio_Lsetfilecon_log
        __P((DB_ENV *, DB_TXN *, DB_LSN *, uint32_t, const DBT *, const DBT *));
    DB_ENV * dbenv = (rpmdb ? rpmdb->db_dbenv : NULL);
    DB_TXN * _txn = (rpmdb ? rpmdb->db_txn : NULL);
    DB_LSN _lsn = {0,0};
    DBT FNdbt = {0};
    DBT CONTEXTdbt = {0};
    if (!(dbenv && _txn && _enable_syscall_logging)) return 0;
    FNdbt.data = (void *)fn;
    FNdbt.size = strlen(fn) + 1;	/* trailing NUL too */
    if (context == NULL) context = "";	/* XXX prevent segfaults */
/*@-observertrans@*/
    CONTEXTdbt.data = (void *)context;
/*@=observertrans@*/
    CONTEXTdbt.size = strlen(context) + 1;	/* trailing NUL too */
    rc = logio_Lsetfilecon_log(dbenv, _txn, &_lsn, DB_FLUSH, &FNdbt, &CONTEXTdbt);
if (_rpmlio_debug)
fprintf(stderr, "<== %s(%s, \"%s\") rc %d\n", __FUNCTION__, fn, context, rc);
#endif	/* SUPPORT_FILE_ACID */
    return rc;
}

int rpmlioChown(rpmdb rpmdb, const char * fn, uid_t uid, gid_t gid)
{
    int rc = 0;
# if defined(SUPPORT_FILE_ACID)
    extern int logio_Chown_log
        __P((DB_ENV *, DB_TXN *, DB_LSN *, uint32_t, const DBT *, uid_t, gid_t));
    DB_ENV * dbenv = (rpmdb ? rpmdb->db_dbenv : NULL);
    DB_TXN * _txn = (rpmdb ? rpmdb->db_txn : NULL);
    DB_LSN _lsn = {0,0};
    DBT FNdbt = {0};
    if (!(dbenv && _txn && _enable_syscall_logging)) return 0;
    FNdbt.data = (void *)fn;
    FNdbt.size = strlen(fn) + 1;	/* trailing NUL too */
    rc = logio_Chown_log(dbenv, _txn, &_lsn, DB_FLUSH, &FNdbt, uid, gid);
if (_rpmlio_debug)
fprintf(stderr, "<== %s(%s, %u, %u) rc %d\n", __FUNCTION__, fn, (unsigned)uid, (unsigned)gid, rc);
#endif	/* SUPPORT_FILE_ACID */
    return rc;
}

int rpmlioLchown(rpmdb rpmdb, const char * fn, uid_t uid, gid_t gid)
{
    int rc = 0;
# if defined(SUPPORT_FILE_ACID)
    extern int logio_Lchown_log
        __P((DB_ENV *, DB_TXN *, DB_LSN *, uint32_t, const DBT *, uid_t, gid_t));
    DB_ENV * dbenv = (rpmdb ? rpmdb->db_dbenv : NULL);
    DB_TXN * _txn = (rpmdb ? rpmdb->db_txn : NULL);
    DB_LSN _lsn = {0,0};
    DBT FNdbt = {0};
    if (!(dbenv && _txn && _enable_syscall_logging)) return 0;
    FNdbt.data = (void *)fn;
    FNdbt.size = strlen(fn) + 1;	/* trailing NUL too */
    rc = logio_Lchown_log(dbenv, _txn, &_lsn, DB_FLUSH, &FNdbt, uid, gid);
if (_rpmlio_debug)
fprintf(stderr, "<== %s(%s, %u, %u) rc %d\n", __FUNCTION__, fn, (unsigned)uid, (unsigned)gid, rc);
#endif	/* SUPPORT_FILE_ACID */
    return rc;
}

int rpmlioChmod(rpmdb rpmdb, const char * fn, mode_t mode)
{
    int rc = 0;
# if defined(SUPPORT_FILE_ACID)
    extern int logio_Chmod_log
        __P((DB_ENV *, DB_TXN *, DB_LSN *, uint32_t, const DBT *, mode_t));
    DB_ENV * dbenv = (rpmdb ? rpmdb->db_dbenv : NULL);
    DB_TXN * _txn = (rpmdb ? rpmdb->db_txn : NULL);
    DB_LSN _lsn = {0,0};
    DBT FNdbt = {0};
    if (!(dbenv && _txn && _enable_syscall_logging)) return 0;
    FNdbt.data = (void *)fn;
    FNdbt.size = strlen(fn) + 1;	/* trailing NUL too */
    rc = logio_Chmod_log(dbenv, _txn, &_lsn, DB_FLUSH, &FNdbt, mode);
if (_rpmlio_debug)
fprintf(stderr, "<== %s(%s, 0%o) rc %d\n", __FUNCTION__, fn, mode, rc);
#endif	/* SUPPORT_FILE_ACID */
    return rc;
}

int rpmlioUtime(rpmdb rpmdb, const char * fn, time_t actime, time_t modtime)
{
    int rc = 0;
# if defined(SUPPORT_FILE_ACID)
    extern int logio_Utime_log
        __P((DB_ENV *, DB_TXN *, DB_LSN *, uint32_t, const DBT *, time_t, time_t));
    DB_ENV * dbenv = (rpmdb ? rpmdb->db_dbenv : NULL);
    DB_TXN * _txn = (rpmdb ? rpmdb->db_txn : NULL);
    DB_LSN _lsn = {0,0};
    DBT FNdbt = {0};
    if (!(dbenv && _txn && _enable_syscall_logging)) return 0;
    FNdbt.data = (void *)fn;
    FNdbt.size = strlen(fn) + 1;	/* trailing NUL too */
    rc = logio_Utime_log(dbenv, _txn, &_lsn, DB_FLUSH, &FNdbt, actime, modtime);
if (_rpmlio_debug)
fprintf(stderr, "<== %s(%s, 0x%x, 0x%x) rc %d\n", __FUNCTION__, fn, (unsigned)actime, (unsigned)modtime, rc);
#endif	/* SUPPORT_FILE_ACID */
    return rc;
}

int rpmlioSymlink(rpmdb rpmdb, const char * ln, const char * fn)
{
    int rc = 0;
# if defined(SUPPORT_FILE_ACID)
    extern int logio_Symlink_log
        __P((DB_ENV *, DB_TXN *, DB_LSN *, uint32_t, const DBT *, const DBT *));
    DB_ENV * dbenv = (rpmdb ? rpmdb->db_dbenv : NULL);
    DB_TXN * _txn = (rpmdb ? rpmdb->db_txn : NULL);
    DB_LSN _lsn = {0,0};
    DBT LNdbt = {0};
    DBT FNdbt = {0};
    if (!(dbenv && _txn && _enable_syscall_logging)) return 0;
    LNdbt.data = (void *)ln;
    LNdbt.size = strlen(ln) + 1;	/* trailing NUL too */
    FNdbt.data = (void *)fn;
    FNdbt.size = strlen(fn) + 1;	/* trailing NUL too */
    rc = logio_Symlink_log(dbenv, _txn, &_lsn, DB_FLUSH, &LNdbt, &FNdbt);
if (_rpmlio_debug)
fprintf(stderr, "<== %s(%s, %s) rc %d\n", __FUNCTION__, ln, fn, rc);
#endif	/* SUPPORT_FILE_ACID */
    return rc;
}

int rpmlioLink(rpmdb rpmdb, const char * ln, const char * fn)
{
    int rc = 0;
# if defined(SUPPORT_FILE_ACID)
    extern int logio_Link_log
        __P((DB_ENV *, DB_TXN *, DB_LSN *, uint32_t, const DBT *, const DBT *));
    DB_ENV * dbenv = (rpmdb ? rpmdb->db_dbenv : NULL);
    DB_TXN * _txn = (rpmdb ? rpmdb->db_txn : NULL);
    DB_LSN _lsn = {0,0};
    DBT LNdbt = {0};
    DBT FNdbt = {0};
    if (!(dbenv && _txn && _enable_syscall_logging)) return 0;
    LNdbt.data = (void *)ln;
    LNdbt.size = strlen(ln) + 1;	/* trailing NUL too */
    FNdbt.data = (void *)fn;
    FNdbt.size = strlen(fn) + 1;	/* trailing NUL too */
    rc = logio_Link_log(dbenv, _txn, &_lsn, DB_FLUSH, &LNdbt, &FNdbt);
if (_rpmlio_debug)
fprintf(stderr, "<== %s(%s, %s) rc %d\n", __FUNCTION__, ln, fn, rc);
#endif	/* SUPPORT_FILE_ACID */
    return rc;
}

int rpmlioMknod(rpmdb rpmdb, const char * fn, mode_t mode, dev_t dev)
{
    int rc = 0;
# if defined(SUPPORT_FILE_ACID)
    extern int logio_Mknod_log
        __P((DB_ENV *, DB_TXN *, DB_LSN *, uint32_t, const DBT *, mode_t, dev_t));
    DB_ENV * dbenv = (rpmdb ? rpmdb->db_dbenv : NULL);
    DB_TXN * _txn = (rpmdb ? rpmdb->db_txn : NULL);
    DB_LSN _lsn = {0,0};
    DBT FNdbt = {0};
    if (!(dbenv && _txn && _enable_syscall_logging)) return 0;
    FNdbt.data = (void *)fn;
    FNdbt.size = strlen(fn) + 1;	/* trailing NUL too */
    rc = logio_Mknod_log(dbenv, _txn, &_lsn, DB_FLUSH, &FNdbt, mode, dev);
if (_rpmlio_debug)
fprintf(stderr, "<== %s(%s, 0%o, 0x%x) rc %d\n", __FUNCTION__, fn, mode, (unsigned)dev, rc);
#endif	/* SUPPORT_FILE_ACID */
    return rc;
}

int rpmlioMkfifo(rpmdb rpmdb, const char * fn, mode_t mode)
{
    int rc = 0;
# if defined(SUPPORT_FILE_ACID)
    extern int logio_Mkfifo_log
        __P((DB_ENV *, DB_TXN *, DB_LSN *, uint32_t, const DBT *, mode_t));
    DB_ENV * dbenv = (rpmdb ? rpmdb->db_dbenv : NULL);
    DB_TXN * _txn = (rpmdb ? rpmdb->db_txn : NULL);
    DB_LSN _lsn = {0,0};
    DBT FNdbt = {0};
    if (!(dbenv && _txn && _enable_syscall_logging)) return 0;
    FNdbt.data = (void *)fn;
    FNdbt.size = strlen(fn) + 1;	/* trailing NUL too */
    rc = logio_Mkfifo_log(dbenv, _txn, &_lsn, DB_FLUSH, &FNdbt, mode);
if (_rpmlio_debug)
fprintf(stderr, "<== %s(%s, 0%o) rc %d\n", __FUNCTION__, fn, mode, rc);
#endif	/* SUPPORT_FILE_ACID */
    return rc;
}

int rpmlioPrein(rpmdb rpmdb, const char ** av, const char * body)
{
    int rc = 0;
# if defined(SUPPORT_FILE_ACID)
    const char * cmd = argvJoin(av, ' ');
    extern int logio_Prein_log
        __P((DB_ENV *, DB_TXN *, DB_LSN *, uint32_t, const DBT *, const DBT *));
    DB_ENV * dbenv = (rpmdb ? rpmdb->db_dbenv : NULL);
    DB_TXN * _txn = (rpmdb ? rpmdb->db_txn : NULL);
    DB_LSN _lsn = {0,0};
    DBT AVdbt = {0};
    DBT BODYdbt = {0};
    if (!(dbenv && _txn && _enable_scriptlet_logging)) return 0;
    AVdbt.data = (void *)cmd;
    AVdbt.size = strlen(cmd) + 1;	/* trailing NUL too */
    BODYdbt.data = (void *)body;
    BODYdbt.size = strlen(body) + 1;	/* trailing NUL too */
    rc = logio_Prein_log(dbenv, _txn, &_lsn, DB_FLUSH, &AVdbt, &BODYdbt);
if (_rpmlio_debug)
fprintf(stderr, "<== %s(%p,%p) rc %d\n", __FUNCTION__, av, body, rc);
    cmd = _free(cmd);
#endif	/* SUPPORT_FILE_ACID */
    return rc;
}

int rpmlioPostin(rpmdb rpmdb, const char ** av, const char * body)
{
    int rc = 0;
# if defined(SUPPORT_FILE_ACID)
    const char * cmd = argvJoin(av, ' ');
    extern int logio_Postin_log
        __P((DB_ENV *, DB_TXN *, DB_LSN *, uint32_t, const DBT *, const DBT *));
    DB_ENV * dbenv = rpmdb->db_dbenv;
    DB_TXN * _txn = rpmdb->db_txn;
    DB_LSN _lsn = {0,0};
    DBT AVdbt = {0};
    DBT BODYdbt = {0};
    if (!(dbenv && _txn && _enable_scriptlet_logging)) return 0;
    AVdbt.data = (void *)cmd;
    AVdbt.size = strlen(cmd) + 1;	/* trailing NUL too */
    BODYdbt.data = (void *)body;
    BODYdbt.size = strlen(body) + 1;	/* trailing NUL too */
    rc = logio_Postin_log(dbenv, _txn, &_lsn, DB_FLUSH, &AVdbt, &BODYdbt);
if (_rpmlio_debug)
fprintf(stderr, "<== %s(%p,%p) rc %d\n", __FUNCTION__, av, body, rc);
    cmd = _free(cmd);
#endif	/* SUPPORT_FILE_ACID */
    return rc;
}

int rpmlioPreun(rpmdb rpmdb, const char ** av, const char * body)
{
    int rc = 0;
# if defined(SUPPORT_FILE_ACID)
    const char * cmd = argvJoin(av, ' ');
    extern int logio_Preun_log
        __P((DB_ENV *, DB_TXN *, DB_LSN *, uint32_t, const DBT *, const DBT *));
    DB_ENV * dbenv = (rpmdb ? rpmdb->db_dbenv : NULL);
    DB_TXN * _txn = (rpmdb ? rpmdb->db_txn : NULL);
    DB_LSN _lsn = {0,0};
    DBT AVdbt = {0};
    DBT BODYdbt = {0};
    if (!(dbenv && _txn && _enable_scriptlet_logging)) return 0;
    AVdbt.data = (void *)cmd;
    AVdbt.size = strlen(cmd) + 1;	/* trailing NUL too */
    BODYdbt.data = (void *)body;
    BODYdbt.size = strlen(body) + 1;	/* trailing NUL too */
    rc = logio_Preun_log(dbenv, _txn, &_lsn, DB_FLUSH, &AVdbt, &BODYdbt);
if (_rpmlio_debug)
fprintf(stderr, "<== %s(%p,%p) rc %d\n", __FUNCTION__, av, body, rc);
    cmd = _free(cmd);
#endif	/* SUPPORT_FILE_ACID */
    return rc;
}

int rpmlioPostun(rpmdb rpmdb, const char ** av, const char * body)
{
    int rc = 0;
# if defined(SUPPORT_FILE_ACID)
    const char * cmd = argvJoin(av, ' ');
    extern int logio_Postun_log
        __P((DB_ENV *, DB_TXN *, DB_LSN *, uint32_t, const DBT *, const DBT *));
    DB_ENV * dbenv = (rpmdb ? rpmdb->db_dbenv : NULL);
    DB_TXN * _txn = (rpmdb ? rpmdb->db_txn : NULL);
    DB_LSN _lsn = {0,0};
    DBT AVdbt = {0};
    DBT BODYdbt = {0};
    if (!(dbenv && _txn && _enable_scriptlet_logging)) return 0;
    AVdbt.data = (void *)cmd;
    AVdbt.size = strlen(cmd) + 1;	/* trailing NUL too */
    BODYdbt.data = (void *)body;
    BODYdbt.size = strlen(body) + 1;	/* trailing NUL too */
    rc = logio_Postun_log(dbenv, _txn, &_lsn, DB_FLUSH, &AVdbt, &BODYdbt);
if (_rpmlio_debug)
fprintf(stderr, "<== %s(%p,%p) rc %d\n", __FUNCTION__, av, body, rc);
    cmd = _free(cmd);
#endif	/* SUPPORT_FILE_ACID */
    return rc;
}
