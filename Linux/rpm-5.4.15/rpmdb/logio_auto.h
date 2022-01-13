/* Do not edit: automatically built by logio.awk. */

#ifndef	logio_AUTO_H
#define	logio_AUTO_H
#define	DB_logio_Creat	10000
typedef struct _logio_Creat_args {
	uint32_t type;
	DB_TXN *txnp;
	DB_LSN prev_lsn;
	DBT	path;
	mode_t	mode;
	DBT	content;
	DBT	digest;
	uint32_t	dalgo;
} logio_Creat_args;

#define	DB_logio_Unlink	10001
typedef struct _logio_Unlink_args {
	uint32_t type;
	DB_TXN *txnp;
	DB_LSN prev_lsn;
	DBT	path;
	mode_t	mode;
	DBT	content;
	DBT	digest;
	uint32_t	dalgo;
} logio_Unlink_args;

#define	DB_logio_Rename	10002
typedef struct _logio_Rename_args {
	uint32_t type;
	DB_TXN *txnp;
	DB_LSN prev_lsn;
	DBT	oldpath;
	DBT	newpath;
	mode_t	mode;
	DBT	content;
	DBT	digest;
	uint32_t	dalgo;
} logio_Rename_args;

#define	DB_logio_Mkdir	10003
typedef struct _logio_Mkdir_args {
	uint32_t type;
	DB_TXN *txnp;
	DB_LSN prev_lsn;
	DBT	path;
	mode_t	mode;
} logio_Mkdir_args;

#define	DB_logio_Rmdir	10004
typedef struct _logio_Rmdir_args {
	uint32_t type;
	DB_TXN *txnp;
	DB_LSN prev_lsn;
	DBT	path;
	mode_t	mode;
} logio_Rmdir_args;

#define	DB_logio_Lsetfilecon	10005
typedef struct _logio_Lsetfilecon_args {
	uint32_t type;
	DB_TXN *txnp;
	DB_LSN prev_lsn;
	DBT	path;
	DBT	context;
} logio_Lsetfilecon_args;

#define	DB_logio_Chown	10006
typedef struct _logio_Chown_args {
	uint32_t type;
	DB_TXN *txnp;
	DB_LSN prev_lsn;
	DBT	path;
	uid_t	owner;
	gid_t	group;
} logio_Chown_args;

#define	DB_logio_Lchown	10007
typedef struct _logio_Lchown_args {
	uint32_t type;
	DB_TXN *txnp;
	DB_LSN prev_lsn;
	DBT	path;
	uid_t	owner;
	gid_t	group;
} logio_Lchown_args;

#define	DB_logio_Chmod	10008
typedef struct _logio_Chmod_args {
	uint32_t type;
	DB_TXN *txnp;
	DB_LSN prev_lsn;
	DBT	path;
	mode_t	mode;
} logio_Chmod_args;

#define	DB_logio_Utime	10009
typedef struct _logio_Utime_args {
	uint32_t type;
	DB_TXN *txnp;
	DB_LSN prev_lsn;
	DBT	path;
	time_t	actime;
	time_t	modtime;
} logio_Utime_args;

#define	DB_logio_Symlink	10010
typedef struct _logio_Symlink_args {
	uint32_t type;
	DB_TXN *txnp;
	DB_LSN prev_lsn;
	DBT	oldpath;
	DBT	newpath;
} logio_Symlink_args;

#define	DB_logio_Link	10011
typedef struct _logio_Link_args {
	uint32_t type;
	DB_TXN *txnp;
	DB_LSN prev_lsn;
	DBT	oldpath;
	DBT	newpath;
} logio_Link_args;

#define	DB_logio_Mknod	10012
typedef struct _logio_Mknod_args {
	uint32_t type;
	DB_TXN *txnp;
	DB_LSN prev_lsn;
	DBT	path;
	mode_t	mode;
	dev_t	dev;
} logio_Mknod_args;

#define	DB_logio_Mkfifo	10013
typedef struct _logio_Mkfifo_args {
	uint32_t type;
	DB_TXN *txnp;
	DB_LSN prev_lsn;
	DBT	path;
	mode_t	mode;
} logio_Mkfifo_args;

#define	DB_logio_Pretrans	10030
typedef struct _logio_Pretrans_args {
	uint32_t type;
	DB_TXN *txnp;
	DB_LSN prev_lsn;
	DBT	cmd;
	DBT	script;
} logio_Pretrans_args;

#define	DB_logio_Prein	10031
typedef struct _logio_Prein_args {
	uint32_t type;
	DB_TXN *txnp;
	DB_LSN prev_lsn;
	DBT	cmd;
	DBT	script;
} logio_Prein_args;

#define	DB_logio_Postin	10032
typedef struct _logio_Postin_args {
	uint32_t type;
	DB_TXN *txnp;
	DB_LSN prev_lsn;
	DBT	cmd;
	DBT	script;
} logio_Postin_args;

#define	DB_logio_Preun	10033
typedef struct _logio_Preun_args {
	uint32_t type;
	DB_TXN *txnp;
	DB_LSN prev_lsn;
	DBT	cmd;
	DBT	script;
} logio_Preun_args;

#define	DB_logio_Postun	10034
typedef struct _logio_Postun_args {
	uint32_t type;
	DB_TXN *txnp;
	DB_LSN prev_lsn;
	DBT	cmd;
	DBT	script;
} logio_Postun_args;

#define	DB_logio_Posttrans	10035
typedef struct _logio_Posttrans_args {
	uint32_t type;
	DB_TXN *txnp;
	DB_LSN prev_lsn;
	DBT	cmd;
	DBT	script;
} logio_Posttrans_args;

#endif
