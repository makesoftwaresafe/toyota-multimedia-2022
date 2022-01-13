#ifndef H_LOGIO
#define	H_LOGIO

#include "logio_auto.h"

#define	_LOGIO_PROTO(_SYS_)	\
int logio_##_SYS_##_print \
	__P((DB_ENV *, DBT *, DB_LSN *, db_recops));\
int logio_##_SYS_##_read \
	__P((DB_ENV *, void *, logio_##_SYS_##_args **));\
int logio_##_SYS_##_recover \
	__P((DB_ENV *, DBT *, DB_LSN *, db_recops))

int logio_Creat_log
	__P((DB_ENV *, DB_TXN *, DB_LSN *, uint32_t, const DBT *, mode_t, const DBT *, const DBT *, uint32_t));
_LOGIO_PROTO(Creat);

int logio_Unlink_log
	__P((DB_ENV *, DB_TXN *, DB_LSN *, uint32_t, const DBT *, mode_t, const DBT *, const DBT *, uint32_t));
_LOGIO_PROTO(Unlink);

int logio_Rename_log
	__P((DB_ENV *, DB_TXN *, DB_LSN *, uint32_t, const DBT *, const DBT *, mode_t, const DBT *, const DBT *, uint32_t));
_LOGIO_PROTO(Rename);

int logio_Mkdir_log
	__P((DB_ENV *, DB_TXN *, DB_LSN *, uint32_t, const DBT *, mode_t));
_LOGIO_PROTO(Mkdir);

int logio_Rmdir_log
	__P((DB_ENV *, DB_TXN *, DB_LSN *, uint32_t, const DBT *, mode_t));
_LOGIO_PROTO(Rmdir);

int logio_Lsetfilecon_log
	__P((DB_ENV *, DB_TXN *, DB_LSN *, uint32_t, const DBT *, const DBT *));
_LOGIO_PROTO(Lsetfilecon);

int logio_Chown_log
	__P((DB_ENV *, DB_TXN *, DB_LSN *, uint32_t, const DBT *, uid_t, gid_t));
_LOGIO_PROTO(Chown);

int logio_Lchown_log
	__P((DB_ENV *, DB_TXN *, DB_LSN *, uint32_t, const DBT *, uid_t, gid_t));
_LOGIO_PROTO(Lchown);

int logio_Chmod_log
	__P((DB_ENV *, DB_TXN *, DB_LSN *, uint32_t, const DBT *, mode_t));
_LOGIO_PROTO(Chmod);

int logio_Utime_log
	__P((DB_ENV *, DB_TXN *, DB_LSN *, uint32_t, const DBT *, time_t, time_t));
_LOGIO_PROTO(Utime);

int logio_Symlink_log
	__P((DB_ENV *, DB_TXN *, DB_LSN *, uint32_t, const DBT *, const DBT *));
_LOGIO_PROTO(Symlink);

int logio_Link_log
	__P((DB_ENV *, DB_TXN *, DB_LSN *, uint32_t, const DBT *, const DBT *));
_LOGIO_PROTO(Link);

int logio_Mknod_log
	__P((DB_ENV *, DB_TXN *, DB_LSN *, uint32_t, const DBT *, mode_t, dev_t));
_LOGIO_PROTO(Mknod);

int logio_Mkfifo_log
	__P((DB_ENV *, DB_TXN *, DB_LSN *, uint32_t, const DBT *, mode_t));
_LOGIO_PROTO(Mkfifo);

int logio_Pretrans_log
	__P((DB_ENV *, DB_TXN *, DB_LSN *, uint32_t, const DBT *, const DBT *));
_LOGIO_PROTO(Pretrans);

int logio_Prein_log
	__P((DB_ENV *, DB_TXN *, DB_LSN *, uint32_t, const DBT *, const DBT *));
_LOGIO_PROTO(Prein);

int logio_Postin_log
	__P((DB_ENV *, DB_TXN *, DB_LSN *, uint32_t, const DBT *, const DBT *));
_LOGIO_PROTO(Postin);

int logio_Preun_log
	__P((DB_ENV *, DB_TXN *, DB_LSN *, uint32_t, const DBT *, const DBT *));
_LOGIO_PROTO(Preun);

int logio_Postun_log
	__P((DB_ENV *, DB_TXN *, DB_LSN *, uint32_t, const DBT *, const DBT *));
_LOGIO_PROTO(Postun);

int logio_Posttrans_log
	__P((DB_ENV *, DB_TXN *, DB_LSN *, uint32_t, const DBT *, const DBT *));
_LOGIO_PROTO(Posttrans);

#undef	_LOGIO_PROTO

int logio_dispatch(DB_ENV * dbenv, DBT * dbt, DB_LSN * lsn, db_recops op);

int logio_init_print __P((DB_ENV *, DB_DISTAB *));

#endif /* !H_LOGIO */
