/*
 * This file emulates the db3/4 structures
 * ...this is supposed to be compatible w/ the _real_ db.h!
 */

#ifndef __DB_EMU_H
#define __DB_EMU_H

struct __db;		typedef struct __db DB;
struct __db_dbt;	typedef struct __db_dbt DBT;
struct __db_env;	typedef struct __db_env DB_ENV;
struct __db_h_stat;     typedef struct __db_h_stat DB_HASH_STAT;
struct __dbc;		typedef struct __dbc DBC;
struct __db_sequence;   typedef struct __db_sequence DB_SEQUENCE;
struct __db_txn;	typedef struct __db_txn DB_TXN;

struct __db {
  void		*app_private;
};

struct __db_dbt {
    void	*data;
    uint32_t	size;

    uint32_t	ulen;
    uint32_t	dlen;
    uint32_t	doff;

    void	*app_data;

#define DB_DBT_APPMALLOC        0x001   /* Callback allocated memory. */
#define DB_DBT_BULK             0x002   /* Internal: Insert if duplicate. */
#define DB_DBT_DUPOK            0x004   /* Internal: Insert if duplicate. */
#define DB_DBT_ISSET            0x008   /* Lower level calls set value. */
#define DB_DBT_MALLOC           0x010   /* Return in malloc'd memory. */
#define DB_DBT_MULTIPLE         0x020   /* References multiple records. */
#define DB_DBT_PARTIAL          0x040   /* Partial put/get. */
#define DB_DBT_REALLOC          0x080   /* Return in realloc'd memory. */
#define DB_DBT_READONLY         0x100   /* Readonly, don't update. */
#define DB_DBT_STREAMING        0x200   /* Internal: DBT is being streamed. */
#define DB_DBT_USERCOPY         0x400   /* Use the user-supplied callback. */
#define DB_DBT_USERMEM          0x800   /* Return in user's memory. */
    uint32_t	flags;
};

struct __db_env {
    void	*app_private;
    int  (*txn_begin) (DB_ENV *, DB_TXN *, DB_TXN **, uint32_t);
    int  (*txn_checkpoint) (DB_ENV *, uint32_t, uint32_t, uint32_t);
};

struct __db_h_stat { /* SHARED */
        uint32_t hash_magic;            /* Magic number. */
        uint32_t hash_version;          /* Version number. */
        uint32_t hash_metaflags;        /* Metadata flags. */
        uint32_t hash_nkeys;            /* Number of unique keys. */
        uint32_t hash_ndata;            /* Number of data items. */
        uint32_t hash_pagecnt;          /* Page count. */
        uint32_t hash_pagesize;         /* Page size. */
        uint32_t hash_ffactor;          /* Fill factor specified at create. */
        uint32_t hash_buckets;          /* Number of hash buckets. */
        uint32_t hash_free;             /* Pages on the free list. */
        uintmax_t hash_bfree;           /* Bytes free on bucket pages. */
        uint32_t hash_bigpages;         /* Number of big key/data pages. */
        uintmax_t hash_big_bfree;       /* Bytes free on big item pages. */
        uint32_t hash_overflows;        /* Number of overflow pages. */
        uintmax_t hash_ovfl_free;       /* Bytes free on ovfl pages. */
        uint32_t hash_dup;              /* Number of dup pages. */
        uintmax_t hash_dup_free;        /* Bytes free on duplicate pages. */
};

struct __dbc {
    DB		*dbp;
};

struct __db_txn {
    int       (*abort) (DB_TXN *);
    int       (*commit) (DB_TXN *, uint32_t);
    int       (*get_name) (DB_TXN *, const char **);
    uint32_t  (*id) (DB_TXN *);
    int       (*set_name) (DB_TXN *, const char *);
};

#define DB_CURRENT		 6
#define DB_KEYLAST		14
#define DB_NEXT			16
#define DB_NEXT_DUP		17
#define DB_SET			26
#define DB_SET_RANGE		27

#define DB_WRITECURSOR		0x00000010

#define DB_BUFFER_SMALL         (-30999)
#define DB_NOTFOUND             (-30988)

#define DB_INIT_TXN                             0x00002000
#define DB_EXCL    0x0004000
#define DB_PRIVATE 0x0200000

#define DB_VERSION_MAJOR 3
#define DB_VERSION_MINOR 0
#define DB_VERSION_PATCH 0

/* --- for rpmdb/dbconfig.c tables: */
typedef enum {
        DB_BTREE=1,
        DB_HASH=2,
        DB_HEAP=6,
        DB_RECNO=3,
        DB_QUEUE=4,
        DB_UNKNOWN=5                    /* Figure it out on open. */
} DBTYPE;
#define DB_CREATE                               0x00000001
#define DB_INIT_LOCK                            0x00000100
#define DB_INIT_LOG                             0x00000200
#define DB_INIT_MPOOL                           0x00000400
#define DB_INIT_TXN                             0x00002000
#define DB_AUTO_COMMIT                          0x00000100
#define DB_THREAD                               0x00000020
#define DB_DUP                                  0x00000010
#define DB_DUPSORT                              0x00000002

#endif
