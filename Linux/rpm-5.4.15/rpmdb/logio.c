
#include "system.h"

#include <rpmio.h>
#include <poptIO.h>

#include "db.h"
#include "db_int.h"
#include "dbinc/db_swap.h"

#include "logio.h"

#include "debug.h"

static int _debug = 0;

static const char *home = "HOME";
static const char * dir = "DIR";
static const char * file = "FILE";
static mode_t _mode = 0755;

static int
verify(const char * fn, mode_t mode)
{
    struct stat sb;
    int rc;
    sb.st_mode = 0;
    rc = Lstat(fn, &sb)
	? errno
	: (sb.st_mode == mode ? 0 : -1);
if (_debug)
fprintf(stderr, "<-- %s(%s,0%o) st_mode 0%o rc %d\n", __FUNCTION__, fn, mode, sb.st_mode, rc);
    return rc;
}

static int
Creat(const char * fn, mode_t mode, const char * s)
{
    size_t ns = strlen(s) + 1;
    FD_t fd = Fopen(fn, "w");
    int rc = 0;
    (void) Fwrite(s, 1, ns, fd);
    (void) Fclose(fd);
    (void) Chmod(fn, mode & 0777);
    return rc;
}

/*
 * Sample application dispatch function to handle user-specified log record
 * types.
 */
static int
apprec_dispatch(DB_ENV * dbenv, DBT * dbt, DB_LSN * lsn, db_recops op)
{
    uint32_t rectype;

    /* Pull the record type out of the log record. */
    LOGCOPY_32(dbenv->env, &rectype, dbt->data);

    switch (rectype) {
    case DB_logio_Creat:
	return logio_Creat_recover(dbenv, dbt, lsn, op);
    case DB_logio_Mkdir:
	return logio_Mkdir_recover(dbenv, dbt, lsn, op);
    case DB_logio_Rename:
	return logio_Rename_recover(dbenv, dbt, lsn, op);
    case DB_logio_Rmdir:
	return logio_Rmdir_recover(dbenv, dbt, lsn, op);
    case DB_logio_Unlink:
	return logio_Unlink_recover(dbenv, dbt, lsn, op);
    default:
	/* We've hit an unexpected, allegedly user-defined record type.  */
	dbenv->errx(dbenv, "Unexpected log record type encountered");
	return EINVAL;
    }
}

static int
open_env(const char * home, FILE * fp, DB_ENV ** dbenvp)
{
    DB_ENV *dbenv;
    uint32_t eflags;
    int ret;

    if (fp == NULL) fp = stderr;

    /* Create an environment initialized for error reporting. */
    if ((ret = db_env_create(&dbenv, 0)) != 0) {
	fprintf(fp, "%s: db_env_create: %s\n", __progname, db_strerror(ret));
	return ret;
    }
    dbenv->set_errfile(dbenv, fp);
    dbenv->set_errpfx(dbenv, __progname);

    /* Set up our custom recovery dispatch function. */
    if ((ret = dbenv->set_app_dispatch(dbenv, apprec_dispatch)) != 0) {
	dbenv->err(dbenv, ret, "DB_ENV->set_app_dispatch");
	return ret;
    }

   /* Open the environment with full transactional support, running recovery. */
    eflags = DB_CREATE | DB_RECOVER |
	DB_INIT_LOCK | DB_INIT_LOG | DB_INIT_MPOOL | DB_INIT_TXN;
    if ((ret = dbenv->open(dbenv, home, eflags, 0)) != 0) {
	dbenv->err(dbenv, ret, "DB_ENV->open: %s", home);
	dbenv->close(dbenv, 0);
	return ret;
    }

    *dbenvp = dbenv;
    return 0;
}

static int
testMkdir(DB_ENV * dbenv, const char * DN, mode_t mode)
{
    const char * msg;
    DB_TXN *txn;
    DB_LSN lsn;
    DBT DNdbt = {0};
    int ret;

    msg = "INIT: Mkdir transaction logged and directory created";
    if ((ret = dbenv->txn_begin(dbenv, NULL, &txn, 0)) != 0) {
	dbenv->err(dbenv, ret, "txn_begin");
	goto exit;
    }
    memset(&lsn, 0, sizeof(lsn));
    DNdbt.data = (void *)DN;
    DNdbt.size = strlen(DN) + 1;	/* trailing NUL too */
    if ((ret = logio_Mkdir_log(dbenv, txn, &lsn, DB_FLUSH, &DNdbt, mode)) != 0) {
	dbenv->err(dbenv, ret, "Mkdir_log");
	goto exit;
    }
    if (Mkdir(DN, mode) != 0) {
	dbenv->err(dbenv, errno, "Mkdir");
	goto exit;
    }
    fprintf(stderr, "%s: %s\n", msg, (verify(DN, mode) == 0 ? "ok" : "FAIL"));

    msg = "TEST: directory is removed by transaction abort";
    if ((ret = txn->abort(txn)) != 0) {
	dbenv->err(dbenv, ret, "TXN->abort");
	goto exit;
    }
    fprintf(stderr, "%s: %s\n", msg, (verify(DN, mode) == ENOENT ? "ok" : "FAIL"));

    msg = "INIT: Mkdir transaction logged and directory created";
    if ((ret = dbenv->txn_begin(dbenv, NULL, &txn, 0)) != 0) {
	dbenv->err(dbenv, ret, "DB_ENV->txn_begin");
	goto exit;
    }
    memset(&lsn, 0, sizeof(lsn));
    DNdbt.data = (void *)DN;
    DNdbt.size = strlen(DN) + 1;
    if ((ret = logio_Mkdir_log(dbenv, txn, &lsn, 0, &DNdbt, mode)) != 0) {
	dbenv->err(dbenv, ret, "Mkdir_log");
	goto exit;
    }
    if (Mkdir(DN, mode) != 0) {
	dbenv->err(dbenv, errno, "Mkdir");
	goto exit;
    }
    fprintf(stderr, "%s: %s\n", msg, (verify(DN, mode) == 0 ? "ok" : "FAIL"));

    /* Commit the transaction and verify that the creation persists. */
    msg = "TEST: creation persists after transaction commit";
    if ((ret = txn->commit(txn, 0)) != 0) {
	dbenv->err(dbenv, ret, "TXN->commit");
	goto exit;
    }
    fprintf(stderr, "%s: %s\n", msg, (verify(DN, mode) == 0 ? "ok" : "FAIL"));

    msg = "INIT: Close dbenv and remove directory";
    if ((ret = dbenv->close(dbenv, 0)) != 0) {
	fprintf(stderr, "DB_ENV->close: %s\n", db_strerror(ret));
	goto exit;
    }
    if (Rmdir(DN) != 0) {
	fprintf(stderr, "%s: Rmdir failed with error %s", __progname,
	    strerror(errno));
    }
    fprintf(stderr, "%s: %s\n", msg, (verify(DN, mode) == ENOENT ? "ok" : "FAIL"));

    msg = "TEST: directory is re-created opening dbenv with DB_RECOVER";
    if ((ret = open_env(home, stderr, &dbenv)) != 0)
	goto exit;
    fprintf(stderr, "%s: %s\n", msg, (verify(DN, mode) == 0 ? "ok" : "FAIL"));

exit:
    return ret;
}

static int
testRmdir(DB_ENV * dbenv, const char * DN, mode_t mode)
{
    const char * msg;
    DB_TXN *txn;
    DB_LSN lsn;
    DBT DNdbt = {0};
    int ret;

    msg = "INIT: Rmdir transaction logged and directory removed";
    if ((ret = dbenv->txn_begin(dbenv, NULL, &txn, 0)) != 0) {
	dbenv->err(dbenv, ret, "txn_begin");
	goto exit;
    }
    memset(&lsn, 0, sizeof(lsn));
    DNdbt.data = (void *)DN;
    DNdbt.size = strlen(DN) + 1;	/* trailing NUL too */
    if ((ret = logio_Rmdir_log(dbenv, txn, &lsn, DB_FLUSH, &DNdbt, mode)) != 0) {
	dbenv->err(dbenv, ret, "Rmdir_log");
	goto exit;
    }
    if (Rmdir(DN) != 0) {
	dbenv->err(dbenv, errno, "Rmdir");
	goto exit;
    }
    fprintf(stderr, "%s: %s\n", msg, (verify(DN, mode) == ENOENT ? "ok" : "FAIL"));

    msg = "TEST: directory is re-created by transaction abort";
    if ((ret = txn->abort(txn)) != 0) {
	dbenv->err(dbenv, ret, "TXN->abort");
	goto exit;
    }
    fprintf(stderr, "%s: %s\n", msg, (verify(DN, mode) == 0 ? "ok" : "FAIL"));

    /* Repeat, with a commit this time. */
    msg = "INIT: Rmdir transaction logged and directory removed";
    if ((ret = dbenv->txn_begin(dbenv, NULL, &txn, 0)) != 0) {
	dbenv->err(dbenv, ret, "DB_ENV->txn_begin");
	goto exit;
    }
    memset(&lsn, 0, sizeof(lsn));
    DNdbt.data = (void *)DN;
    DNdbt.size = strlen(DN) + 1;
    if ((ret = logio_Rmdir_log(dbenv, txn, &lsn, 0, &DNdbt, mode)) != 0) {
	dbenv->err(dbenv, ret, "Rmdir_log");
	goto exit;
    }
    if (Rmdir(DN) != 0) {
	dbenv->err(dbenv, errno, "Rmdir");
	goto exit;
    }
    fprintf(stderr, "%s: %s\n", msg, (verify(DN, mode) == ENOENT ? "ok" : "FAIL"));

    msg = "TEST: removal persists after transaction commit";
    if ((ret = txn->commit(txn, 0)) != 0) {
	dbenv->err(dbenv, ret, "TXN->commit");
	goto exit;
    }
    fprintf(stderr, "%s: %s\n", msg, (verify(DN, mode) == ENOENT ? "ok" : "FAIL"));

    msg = "INIT: Close dbenv and create directory";
    if ((ret = dbenv->close(dbenv, 0)) != 0) {
	fprintf(stderr, "DB_ENV->close: %s\n", db_strerror(ret));
	goto exit;
    }
    if (Mkdir(DN, mode) != 0) {
	fprintf(stderr, "%s: Mkdir failed with error %s", __progname,
	    strerror(errno));
    }
    fprintf(stderr, "%s: %s\n", msg, (verify(DN, mode) == 0 ? "ok" : "FAIL"));

    msg = "TEST: directory is removed opening dbenv with DB_RECOVER";
    if ((ret = open_env(home, stderr, &dbenv)) != 0)
	goto exit;
    fprintf(stderr, "%s: %s\n", msg, (verify(DN, mode) == ENOENT ? "ok" : "FAIL"));

exit:
    return ret;
}

static int
testCreat(DB_ENV * dbenv, const char * FN, mode_t mode)
{
    const char * msg;
    DB_TXN *txn;
    DB_LSN lsn;
    DBT FNdbt = {0};
    DBT Bdbt = {0};
    DBT Ddbt = {0};
    uint32_t dalgo = 0;
    int ret;

    msg = "INIT: Creat transaction logged and file created";
    if ((ret = dbenv->txn_begin(dbenv, NULL, &txn, 0)) != 0) {
	dbenv->err(dbenv, ret, "txn_begin");
	goto exit;
    }
    FNdbt.data = (void *)FN;
    FNdbt.size = strlen(FN) + 1;	/* trailing NUL too */
    Bdbt.data = (void *)FN;
    Bdbt.size = strlen(FN) + 1;
    memset(&lsn, 0, sizeof(lsn));
    if ((ret = logio_Creat_log(dbenv, txn, &lsn, DB_FLUSH, &FNdbt, mode, &Bdbt, &Ddbt, dalgo)) != 0) {
	dbenv->err(dbenv, ret, "Creat_log");
	goto exit;
    }
    if (Creat(FN, mode, FN) != 0) {
	dbenv->err(dbenv, errno, "Creat");
	goto exit;
    }
    fprintf(stderr, "%s: %s\n", msg, (verify(FN, mode) == 0 ? "ok" : "FAIL"));

    msg = "TEST: file is removed by transaction abort";
    if ((ret = txn->abort(txn)) != 0) {
	dbenv->err(dbenv, ret, "TXN->abort");
	goto exit;
    }
    fprintf(stderr, "%s: %s\n", msg, (verify(FN, mode) == ENOENT ? "ok" : "FAIL"));

    /* Repeat, with a commit this time. */
    msg = "INIT: Creat transaction logged and file created";
    if ((ret = dbenv->txn_begin(dbenv, NULL, &txn, 0)) != 0) {
	dbenv->err(dbenv, ret, "DB_ENV->txn_begin");
	goto exit;
    }
    memset(&lsn, 0, sizeof(lsn));
    FNdbt.data = (void *)FN;
    FNdbt.size = strlen(FN) + 1;
    Bdbt.data = (void *)FN;
    Bdbt.size = strlen(FN) + 1;
    if ((ret = logio_Creat_log(dbenv, txn, &lsn, 0, &FNdbt, mode, &Bdbt, &Ddbt, dalgo)) != 0) {
	dbenv->err(dbenv, ret, "Creat_log");
	goto exit;
    }
    if (Creat(FN, mode, FN) != 0) {
	dbenv->err(dbenv, errno, "Creat");
	goto exit;
    }
    fprintf(stderr, "%s: %s\n", msg, (verify(FN, mode) == 0 ? "ok" : "FAIL"));

    /* Commit the transaction and verify that the creation persists. */
    msg = "TEST: creation persists after transaction commit";
    if ((ret = txn->commit(txn, 0)) != 0) {
	dbenv->err(dbenv, ret, "TXN->commit");
	goto exit;
    }
    fprintf(stderr, "%s: %s\n", msg, (verify(FN, mode) == 0 ? "ok" : "FAIL"));

    msg = "INIT: Close dbenv and remove file";
    if ((ret = dbenv->close(dbenv, 0)) != 0) {
	fprintf(stderr, "DB_ENV->close: %s\n", db_strerror(ret));
	goto exit;
    }
    if (Unlink(FN) != 0) {
	fprintf(stderr, "%s: Unlink failed with error %s", __progname,
	    strerror(errno));
    }
    fprintf(stderr, "%s: %s\n", msg, (verify(FN, mode) == ENOENT ? "ok" : "FAIL"));

    msg = "TEST: file is re-created opening dbenv with DB_RECOVER";
    if ((ret = open_env(home, stderr, &dbenv)) != 0)
	goto exit;
    fprintf(stderr, "%s: %s\n", msg, (verify(FN, mode) == 0 ? "ok" : "FAIL"));

exit:
    return ret;
}

static int
testUnlink(DB_ENV * dbenv, const char * FN, mode_t mode)
{
    const char * msg;
    DB_TXN *txn;
    DB_LSN lsn;
    DBT FNdbt = {0};
    DBT Bdbt = {0};
    DBT Ddbt = {0};
    uint32_t dalgo = 0;
    int ret;

    msg = "INIT: Unlink transaction logged and file removed";
    if ((ret = dbenv->txn_begin(dbenv, NULL, &txn, 0)) != 0) {
	dbenv->err(dbenv, ret, "txn_begin");
	goto exit;
    }
    FNdbt.data = (void *)FN;
    FNdbt.size = strlen(FN) + 1;	/* trailing NUL too */
    Bdbt.data = (void *)FN;
    Bdbt.size = strlen(FN) + 1;
    memset(&lsn, 0, sizeof(lsn));
    if ((ret = logio_Unlink_log(dbenv, txn, &lsn, DB_FLUSH, &FNdbt, mode, &Bdbt, &Ddbt, dalgo)) != 0) {
	dbenv->err(dbenv, ret, "Unlink_log");
	goto exit;
    }
    if (Unlink(FN) != 0) {
	dbenv->err(dbenv, errno, "Unlink");
	goto exit;
    }
    fprintf(stderr, "%s: %s\n", msg, (verify(FN, mode) == ENOENT ? "ok" : "FAIL"));

    msg = "TEST: file is re-created by transaction abort";
    if ((ret = txn->abort(txn)) != 0) {
	dbenv->err(dbenv, ret, "TXN->abort");
	goto exit;
    }
    fprintf(stderr, "%s: %s\n", msg, (verify(FN, mode) == 0 ? "ok" : "FAIL"));

    /* Repeat, with a commit this time. */
    msg = "INIT: Unlink transaction logged and file removed";
    if ((ret = dbenv->txn_begin(dbenv, NULL, &txn, 0)) != 0) {
	dbenv->err(dbenv, ret, "DB_ENV->txn_begin");
	goto exit;
    }
    memset(&lsn, 0, sizeof(lsn));
    FNdbt.data = (void *)FN;
    FNdbt.size = strlen(FN) + 1;
    Bdbt.data = (void *)FN;
    Bdbt.size = strlen(FN) + 1;
    if ((ret = logio_Unlink_log(dbenv, txn, &lsn, 0, &FNdbt, mode, &Bdbt, &Ddbt, dalgo)) != 0) {
	dbenv->err(dbenv, ret, "Unlink_log");
	goto exit;
    }
    if (Unlink(FN) != 0) {
	dbenv->err(dbenv, errno, "Unlink");
	goto exit;
    }
    fprintf(stderr, "%s: %s\n", msg, (verify(FN, mode) == ENOENT ? "ok" : "FAIL"));

    msg = "TEST: removal persists after transaction commit";
    if ((ret = txn->commit(txn, 0)) != 0) {
	dbenv->err(dbenv, ret, "TXN->commit");
	goto exit;
    }
    fprintf(stderr, "%s: %s\n", msg, (verify(FN, mode) == ENOENT ? "ok" : "FAIL"));

    msg = "INIT: Close dbenv and create file";
    if ((ret = dbenv->close(dbenv, 0)) != 0) {
	fprintf(stderr, "DB_ENV->close: %s\n", db_strerror(ret));
	goto exit;
    }
    if (Creat(FN, mode, FN) != 0) {
	fprintf(stderr, "%s: Creat failed with error %s", __progname,
	    strerror(errno));
    }
    fprintf(stderr, "%s: %s\n", msg, (verify(FN, mode) == 0 ? "ok" : "FAIL"));

    msg = "TEST: file is removed opening dbenv with DB_RECOVER";
    if ((ret = open_env(home, stderr, &dbenv)) != 0)
	goto exit;
    fprintf(stderr, "%s: %s\n", msg, (verify(FN, mode) == ENOENT ? "ok" : "FAIL"));

exit:
    return ret;
}

static struct poptOption _optionsTable[] = {

  { "home", 'h', POPT_ARG_STRING|POPT_ARGFLAG_SHOW_DEFAULT,	&home, 0,
	N_("Specify DIR for database environment"), N_("DIR") },

 { "debug", 'd', POPT_ARG_VAL,	&_debug, -1,		NULL, NULL },

 { NULL, '\0', POPT_ARG_INCLUDE_TABLE, rpmioAllPoptTable, 0,
	N_("Common options for all rpmio executables:"), NULL },

  POPT_AUTOALIAS
  POPT_AUTOHELP
  POPT_TABLEEND
};

static struct poptOption *optionsTable = &_optionsTable[0];

int
main(int argc, char *argv[])
{
    poptContext optCon = rpmioInit(argc, argv, optionsTable);
    const char * DN = rpmGetPath(home, "/", dir, NULL);
    const char * FN = rpmGetPath(home, "/", file, NULL);
    DB_ENV *dbenv;
    int ret;
    int ec = EXIT_FAILURE;	/* assume failure */

    if (verify(home, S_IFDIR|_mode) == ENOENT)
	(void) Mkdir(home, _mode);

    if ((ret = open_env(home, NULL, &dbenv)) != 0)
	goto exit;

   /*
    * Remember, always log actions before you execute them!
    * Since this log record is describing a file system operation and
    * we have no control over when file system operations go to disk,
    * we need to flush the log record immediately to ensure that the
    * log record is on disk before the operation it describes.  The
    * flush would not be necessary were we doing an operation into the
    * BDB mpool and using LSNs that mpool knew about.
    */

    if ((ret = testMkdir(dbenv, DN, (S_IFDIR|_mode))) != 0)
	goto exit;
    if ((ret = testRmdir(dbenv, DN, (S_IFDIR|_mode))) != 0)
	goto exit;

    if ((ret = testCreat(dbenv, FN, (S_IFREG|_mode))) != 0)
	goto exit;
    if ((ret = testUnlink(dbenv, FN, (S_IFREG|_mode))) != 0)
	goto exit;

    if ((ret = dbenv->close(dbenv, 0)) != 0) {
	fprintf(stderr, "DB_ENV->close: %s\n", db_strerror(ret));
	goto exit;
    }

    ec = EXIT_SUCCESS;

exit:
    FN = _free(FN);
    DN = _free(DN);
    optCon = rpmioFini(optCon);
    return ec;
}
