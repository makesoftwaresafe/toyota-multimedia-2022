/*@-mustmod@*/
/*@-paramuse@*/
/*@-globuse@*/
/*@-moduncon@*/
/*@-noeffectuncon@*/
/*@-compdef@*/
/*@-compmempass@*/
/*@-modfilesystem@*/
/*@-evalorderuncon@*/

/*
 * sqlite.c
 * sqlite interface for rpmdb
 *
 * Author: Mark Hatle <mhatle@mvista.com> or <fray@kernel.crashing.org>
 * Copyright (c) 2004 MontaVista Software, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * or GNU Library General Public License, at your option,
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * and GNU Library Public License along with this program;
 * if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 */

#include "system.h"

#include <sqlite3.h>

#define	_RPMSQL_INTERNAL
#include <rpmsql.h>

#include <rpmlog.h>
#include <rpmmacro.h>
#include <rpmurl.h>     /* XXX urlPath proto */

#include <rpmtag.h>
#define	_RPMDB_INTERNAL
#include <rpmdb.h>

#include "debug.h"

/* XXX retrofit the *BSD typedef for the deprived. */
#if defined(__QNXNTO__)
typedef rpmuint32_t       u_int32_t;
#endif

#if defined(__LCLINT__)
#define	UINT32_T	u_int32_t
#else
#define	UINT32_T	uint32_t
#endif

/*@access rpmdb @*/
/*@access dbiIndex @*/

/*@unchecked@*/
int _sqldb_debug = 0;

#define SQLDBDEBUG(_dbi, _list)	\
    if (((_dbi) && (_dbi)->dbi_debug) || (_sqldb_debug)) fprintf _list

/* =================================================================== */
/*@-redef@*/
typedef struct key_s {
    uint32_t	v;
/*@observer@*/
    const char *n;
} KEY;
/*@=redef@*/

/*@observer@*/
static const char * tblName(uint32_t v, KEY * tbl, size_t ntbl)
	/*@*/
{
    const char * n = NULL;
    static char buf[32];
    size_t i;

    for (i = 0; i < ntbl; i++) {
	if (v != tbl[i].v)
	    continue;
	n = tbl[i].n;
	break;
    }
    if (n == NULL) {
	(void) snprintf(buf, sizeof(buf), "0x%x", (unsigned)v);
	n = buf;
    }
    return n;
}

static const char * fmtBits(uint32_t flags, KEY tbl[], size_t ntbl, char *t)
	/*@modifies t @*/
{
    char pre = '<';
    char * te = t;
    int i;

    sprintf(t, "0x%x", (unsigned)flags);
    te = t;
    te += strlen(te);
    for (i = 0; i < 32; i++) {
	uint32_t mask = (1 << i);
	const char * name;

	if (!(flags & mask))
	    continue;

	name = tblName(mask, tbl, ntbl);
	*te++ = pre;
	pre = ',';
	te = stpcpy(te, name);
    }
    if (pre == ',') *te++ = '>';
    *te = '\0';
    return t;
}
#define _DBT_ENTRY(_v)      { DB_DBT_##_v, #_v, }
/*@unchecked@*/ /*@observer@*/
static KEY DBTflags[] = {
    _DBT_ENTRY(MALLOC),
    _DBT_ENTRY(REALLOC),
    _DBT_ENTRY(USERMEM),
    _DBT_ENTRY(PARTIAL),
    _DBT_ENTRY(APPMALLOC),
    _DBT_ENTRY(MULTIPLE),
#if defined(DB_DBT_READONLY)	/* XXX db-5.2.28 */
    _DBT_ENTRY(READONLY),
#endif
};
#undef	_DBT_ENTRY
/*@unchecked@*/
static size_t nDBTflags = sizeof(DBTflags) / sizeof(DBTflags[0]);
/*@observer@*/
static char * fmtDBT(const DBT * K, char * te)
	/*@modifies te @*/
{
    static size_t keymax = 35;
    int unprintable;
    uint32_t i;

    sprintf(te, "%p[%u]\t", K->data, (unsigned)K->size);
    te += strlen(te);
    (void) fmtBits(K->flags, DBTflags, nDBTflags, te);
    te += strlen(te);
    if (K->data && K->size > 0) {
 	uint8_t * _u;
	size_t _nu;

	/* Grab the key data/size. */
	if (K->flags & DB_DBT_MULTIPLE) {
	    DBT * _K = K->data;
	    _u = _K->data;
	    _nu = _K->size;
	} else {
	    _u = K->data;
	    _nu = K->size;
	}
	/* Verify if data is a string. */
	unprintable = 0;
	for (i = 0; i < _nu; i++)
	    unprintable |= !xisprint(_u[i]);

	/* Display the data. */
	if (!unprintable) {
	    size_t nb = (_nu < keymax ? _nu : keymax);
	    char * ellipsis = (_nu < keymax ? "" : "...");
	    sprintf(te, "\t\"%.*s%s\"", (int)nb, (char *)_u, ellipsis);
	} else {
	    switch (_nu) {
	    default: break;
	    case 4:	sprintf(te, "\t0x%08x", (unsigned)*(uint32_t *)_u); break;
	    }
	}

	te += strlen(te);
	*te = '\0';
    }
    return te;
}
/*@observer@*/
static const char * fmtKDR(const DBT * K, const DBT * P, const DBT * D, const DBT * R)
	/*@*/
{
    static char buf[BUFSIZ];
    char * te = buf;

    if (K) {
	te = stpcpy(te, "\n\t  key: ");
	te = fmtDBT(K, te);
    }
    if (P) {
	te = stpcpy(te, "\n\t pkey: ");
	te = fmtDBT(P, te);
    }
    if (D) {
	te = stpcpy(te, "\n\t data: ");
	te = fmtDBT(D, te);
    }
    if (R) {
	te = stpcpy(te, "\n\t  res: ");
	te = fmtDBT(R, te);
    }
    *te = '\0';
    
    return buf;
}
#define	_KEYDATA(_K, _P, _D, _R)	fmtKDR(_K, _P, _D, _R)

/* =================================================================== */
static int Xcvtdberr(/*@unused@*/ dbiIndex dbi, const char * msg,
		int error, int printit,
		const char * func, const char * fn, unsigned ln)
	/*@globals fileSystem @*/
	/*@modifies fileSystem @*/
{
    int rc = error;

    if (printit && rc)
    switch (rc) {
    case SQLITE_DONE:
	break;		/* Filter out valid returns. */
    default:
      {	rpmsql sql = (rpmsql) dbi->dbi_db;
	sqlite3 * sqlI = (sqlite3 *) sql->I;
	const char * errmsg = dbi != NULL
		? sqlite3_errmsg(sqlI)
		: "";
	rpmlog(RPMLOG_ERR, "%s:%s:%u: %s(%d): %s\n",
		func, fn, ln, msg, rc, errmsg);
      }	break;
    }

    return rc;
}
#define	cvtdberr(_dbi, _msg, _error)	\
    Xcvtdberr(_dbi, _msg, _error, _sqldb_debug, __FUNCTION__, __FILE__, __LINE__)

/* =================================================================== */
struct _sql_dbcursor_s;	typedef struct _sql_dbcursor_s *SCP_t;
struct _sql_dbcursor_s {
    struct rpmioItem_s _item;   /*!< usage mutex and pool identifier. */
	/* XXX FIXME: chain back to the sqlite3 * handle. */

/*@only@*/ /*@relnull@*/
    char * cmd;			/* SQL command string */
/*@only@*/ /*@relnull@*/
    sqlite3_stmt *pStmt;	/* SQL byte code */
    char * pzErrmsg;		/* SQL error msg */

  /* Table -- result of query */
/*@only@*/ /*@relnull@*/
    char ** av;			/* item ptrs */
/*@only@*/ /*@relnull@*/
    size_t * avlen;		/* item sizes */
    int nalloc;
    int ac;			/* no. of items */
    int rx;			/* Which row are we on? 1, 2, 3 ... */
    int nr;			/* no. of rows */
    int nc;			/* no. of columns */

    int all;			/* sequential iteration cursor */
/*@relnull@*/
    DBT ** keys;		/* array of package keys */
    int nkeys;

    int count;

/*@null@*/
    void * lkey;		/* Last key returned */
/*@null@*/
    void * ldata;		/* Last data returned */

    int used;
#if defined(__LCLINT__)
/*@refs@*/
    int nrefs;			/*!< (unused) keep splint happy */
#endif
};

/*==============================================================*/
int _scp_debug = 0;

#define SCPDEBUG(_dbi, _list)   if (_scp_debug) fprintf _list

/**
 * Unreference a SCP wrapper instance.
 * @param scp		SCP wrapper
 * @return		NULL on last dereference
 */
/*@unused@*/ /*@null@*/
SCP_t scpUnlink (/*@killref@*/ /*@only@*/ /*@null@*/ SCP_t scp)
	/*@modifies scp @*/;
#define	scpUnlink(_scp)	\
    ((SCP_t)rpmioUnlinkPoolItem((rpmioItem)(_scp), __FUNCTION__, __FILE__, __LINE__))

/**
 * Reference a SCP wrapper instance.
 * @param scp		SCP wrapper
 * @return		new SCP wrapper reference
 */
/*@unused@*/ /*@newref@*/ /*@null@*/
SCP_t scpLink (/*@null@*/ SCP_t scp)
	/*@modifies scp @*/;
#define	scpLink(_scp)	\
    ((SCP_t)rpmioLinkPoolItem((rpmioItem)(_scp), __FUNCTION__, __FILE__, __LINE__))

/**
 * Destroy a SCP wrapper.
 * @param scp		SCP wrapper
 * @return		NULL on last dereference
 */
/*@null@*/
SCP_t scpFree(/*@killref@*/ /*@null@*/SCP_t scp)
	/*@globals fileSystem @*/
	/*@modifies scp, fileSystem @*/;
#define	scpFree(_scp)	\
    ((SCP_t)rpmioFreePoolItem((rpmioItem)(_scp), __FUNCTION__, __FILE__, __LINE__))

static void dbg_scp(void *ptr)
	/*@globals stderr, fileSystem @*/
	/*@modifies stderr, fileSystem @*/
{
    SCP_t scp = ptr;

if (_scp_debug)
fprintf(stderr, "\tscp %p [%d:%d] av %p avlen %p nr [%d:%d] nc %d all %d\n", scp, scp->ac, scp->nalloc, scp->av, scp->avlen, scp->rx, scp->nr, scp->nc, scp->all);

}

static void dbg_keyval(const char * msg, dbiIndex dbi, /*@null@*/ DBC * dbcursor,
		DBT * key, DBT * data, unsigned int flags)
	/*@globals stderr, fileSystem @*/
	/*@modifies stderr, fileSystem @*/
{

if (!_scp_debug) return;

    fprintf(stderr, "%s on %s (%p,%p,%p,0x%x)", msg, dbi->dbi_subfile, dbcursor, key, data, flags);

    /* XXX FIXME: ptr alignment is fubar here. */
    if (key != NULL && key->data != NULL) {
	fprintf(stderr, "  key 0x%x[%d]", *(unsigned int *)key->data, key->size);
	if (dbi->dbi_rpmtag == RPMTAG_NAME)
	    fprintf(stderr, " \"%s\"", (const char *)key->data);
    }
    if (data != NULL && data->data != NULL)
	fprintf(stderr, " data 0x%x[%d]", *(unsigned int *)data->data, data->size);

    fprintf(stderr, "\n");
    if (dbcursor != NULL)
	dbg_scp(dbcursor);
}

/*@only@*/
static SCP_t scpResetKeys(/*@only@*/ SCP_t scp)
	/*@modifies scp @*/
{
    int ix;

#if 0
SCPDEBUG(NULL, (stderr, "--> %s(%p)\n", __FUNCTION__, scp));
dbg_scp(scp);
#endif

    for ( ix =0 ; ix < scp->nkeys ; ix++ ) {
	scp->keys[ix]->data = _free(scp->keys[ix]->data);
/*@-unqualifiedtrans@*/
	scp->keys[ix] = _free(scp->keys[ix]);
/*@=unqualifiedtrans@*/
    }
    scp->keys = _free(scp->keys);
    scp->nkeys = 0;

    return scp;
}

/*@only@*/
static SCP_t scpResetAv(/*@only@*/ SCP_t scp)
	/*@modifies scp @*/
{
    int xx;

#if 0
SCPDEBUG(NULL, (stderr, "--> %s(%p)\n", __FUNCTION__, scp));
dbg_scp(scp);
#endif

    if (scp->av != NULL) {
	if (scp->nalloc <= 0) {
	    /* Clean up SCP_t used by sqlite3_get_table(). */
	    sqlite3_free_table(scp->av);
	    scp->av = NULL;
	    scp->nalloc = 0;
	} else {
	    /* Clean up SCP_t used by sql_step(). */
	    for (xx = 0; xx < scp->ac; xx++)
		scp->av[xx] = _free(scp->av[xx]);
	    if (scp->av != NULL)
		memset(scp->av, 0, scp->nalloc * sizeof(*scp->av));
	    if (scp->avlen != NULL)
		memset(scp->avlen, 0, scp->nalloc * sizeof(*scp->avlen));
	    scp->av = _free(scp->av);
	    scp->avlen = _free(scp->avlen);
	    scp->nalloc = 0;
	}
    } else
	scp->nalloc = 0;
    scp->ac = 0;
    scp->nr = 0;
    scp->nc = 0;

    return scp;
}

/*@only@*/
static SCP_t scpReset(/*@only@*/ SCP_t scp)
	/*@modifies scp @*/
{
    int xx;

#if 0
SCPDEBUG(NULL, (stderr, "--> %s(%p)\n", __FUNCTION__, scp));
dbg_scp(scp);
#endif

#ifndef	DYING
    if (scp->cmd) {
	sqlite3_free(scp->cmd);
	scp->cmd = NULL;
    }
#else
    scp->cmd = _free(scp->cmd);
#endif

    if (scp->pStmt) {
	xx = cvtdberr(NULL, "sqlite3_reset",
		sqlite3_reset(scp->pStmt));
	xx = cvtdberr(NULL, "sqlite3_finalize",
		sqlite3_finalize(scp->pStmt));
	scp->pStmt = NULL;
    }

    scp = scpResetAv(scp);

    scp->rx = 0;
    return scp;
}

static void scpFini(void * _scp)
	/*@globals fileSystem @*/
	/*@modifies *_scp, fileSystem @*/
{
    SCP_t scp = (SCP_t) _scp;

SCPDEBUG(NULL, (stderr, "==> %s(%p)\n", __FUNCTION__, scp));
    scp = scpReset(scp);
    scp = scpResetKeys(scp);
    scp->av = _free(scp->av);
    scp->avlen = _free(scp->avlen);
}

/*@unchecked@*/ /*@only@*/ /*@null@*/
rpmioPool _scpPool = NULL;

static SCP_t scpGetPool(/*@null@*/ rpmioPool pool)
	/*@globals _rpmscpPool, fileSystem @*/
	/*@modifies pool, _scpPool, fileSystem @*/
{
    SCP_t scp;

    if (_scpPool == NULL) {
	_scpPool = rpmioNewPool("scp", sizeof(*scp), -1, _scp_debug,
			NULL, NULL, scpFini);
	pool = _scpPool;
    }
    scp = (SCP_t) rpmioGetPool(pool, sizeof(*scp));
    memset(((char *)scp)+sizeof(scp->_item), 0, sizeof(*scp)-sizeof(scp->_item));
    return scp;
}

static SCP_t scpNew(/*@unsed@*/ void * dbp)
{
    SCP_t scp = scpGetPool(_scpPool);

#ifdef	 NOTYET	/* XXX FIXME: chain back to the sqlite3 * handle. */
/*@-temptrans@*/
    scp->dbp = dbp;
/*@=temptrans@*/
#endif

    scp->used = 0;
    scp->lkey = NULL;
    scp->ldata = NULL;

    return scpLink(scp);
}

/* ============================================================== */
static int sql_step(dbiIndex dbi, SCP_t scp)
	/*@modifies dbi, scp @*/
{
    int swapped = dbiByteSwapped(dbi);
    const char * cname;
    const char * vtype;
    size_t nb;
    int loop;
    int need;
    int rc;
    int i;

    scp->nc = sqlite3_column_count(scp->pStmt);	/* XXX cvtdberr? */

    if (scp->nr == 0 && scp->av != NULL)
	need = 2 * scp->nc;
    else
	need = scp->nc;

    /* XXX scp->nc = need = scp->nalloc = 0 case forces + 1 here */
    if (!scp->ac && !need && !scp->nalloc)
	need++;

    if (scp->ac + need >= scp->nalloc) {
	/* XXX +4 is bogus, was +1 */
	scp->nalloc = 2 * scp->nalloc + need + 4;
	scp->av = xrealloc(scp->av, scp->nalloc * sizeof(*scp->av));
	scp->avlen = xrealloc(scp->avlen, scp->nalloc * sizeof(*scp->avlen));
    }

    if (scp->av != NULL && scp->nr == 0) {
	for (i = 0; i < scp->nc; i++) {
	    scp->av[scp->ac] = xstrdup(sqlite3_column_name(scp->pStmt, i));
	    if (scp->avlen) scp->avlen[scp->ac] = strlen(scp->av[scp->ac]) + 1;
	    scp->ac++;
assert(scp->ac <= scp->nalloc);
	}
    }

    loop = 1;
    while (loop) {
	rc = cvtdberr(dbi, "sqlite3_step",
		sqlite3_step(scp->pStmt));
	switch (rc) {
	case SQLITE_DONE:
SQLDBDEBUG(dbi, (stderr, "%s(%p,%p): DONE [%d:%d] av %p avlen %p\n", __FUNCTION__, dbi, scp, scp->ac, scp->nalloc, scp->av, scp->avlen));
	    loop = 0;
	    /*@switchbreak@*/ break;
	case SQLITE_ROW:
	    if (scp->av != NULL)
	    for (i = 0; i < scp->nc; i++) {
		/* Expand the row array for new elements */
    		if (scp->ac + need >= scp->nalloc) {
		    /* XXX +4 is bogus, was +1 */
		    scp->nalloc = 2 * scp->nalloc + need + 4;
		    scp->av = xrealloc(scp->av, scp->nalloc * sizeof(*scp->av));
		    scp->avlen = xrealloc(scp->avlen, scp->nalloc * sizeof(*scp->avlen));
	        }
assert(scp->av != NULL);
assert(scp->avlen != NULL);

		cname = sqlite3_column_name(scp->pStmt, i);
		vtype = sqlite3_column_decltype(scp->pStmt, i);
		nb = 0;

		if (!strcmp(vtype, "blob")) {
		    const void * v = sqlite3_column_blob(scp->pStmt, i);
		    nb = sqlite3_column_bytes(scp->pStmt, i);
SQLDBDEBUG(dbi, (stderr, "\t%d %s %s %p[%d]\n", i, cname, vtype, v, (int)nb));
		    if (nb > 0) {
			void * t = (void *) xmalloc(nb);
			scp->av[scp->ac] = (char *) memcpy(t, v, nb);
			scp->avlen[scp->ac] = nb;
			scp->ac++;
		    }
		} else
		if (!strcmp(vtype, "double")) {
		    double v = sqlite3_column_double(scp->pStmt, i);
		    nb = sizeof(v);
SQLDBDEBUG(dbi, (stderr, "\t%d %s %s %g\n", i, cname, vtype, v));
		    if (nb > 0) {
			scp->av[scp->ac] = (char *) memcpy(xmalloc(nb), &v, nb);
			scp->avlen[scp->ac] = nb;
assert(swapped == 0); /* Byte swap?! */
			scp->ac++;
		    }
		} else
		if (!strcmp(vtype, "int")) {
		    rpmint32_t v = sqlite3_column_int(scp->pStmt, i);
		    nb = sizeof(v);
SQLDBDEBUG(dbi, (stderr, "\t%d %s %s %d\n", i, cname, vtype, (int) v));
		    if (nb > 0) {
			scp->av[scp->ac] = (char *) memcpy(xmalloc(nb), &v, nb);
			scp->avlen[scp->ac] = nb;
			scp->ac++;
		    }
		} else
		if (!strcmp(vtype, "int64")) {
		    int64_t v = sqlite3_column_int64(scp->pStmt, i);
		    nb = sizeof(v);
SQLDBDEBUG(dbi, (stderr, "\t%d %s %s %ld\n", i, cname, vtype, (long)v));
		    if (nb > 0) {
			scp->av[scp->ac] = (char *) memcpy(xmalloc(nb), &v, nb);
			scp->avlen[scp->ac] = nb;
assert(swapped == 0); /* Byte swap?! */
			scp->ac++;
		    }
		} else
		if (!strcmp(vtype, "text")) {
		    const char * v = (const char *)sqlite3_column_text(scp->pStmt, i);
		    nb = strlen(v) + 1;
SQLDBDEBUG(dbi, (stderr, "\t%d %s %s \"%s\"\n", i, cname, vtype, v));
		    if (nb > 0) {
			scp->av[scp->ac] = (char *) memcpy(xmalloc(nb), v, nb);
			scp->avlen[scp->ac] = nb;
			scp->ac++;
		    }
		}
assert(scp->ac <= scp->nalloc);
	    }
	    scp->nr++;
	    /*@switchbreak@*/ break;
	case SQLITE_BUSY:
	    fprintf(stderr, "sqlite3_step: BUSY %d\n", rc);
	    /*@switchbreak@*/ break;
	case SQLITE_ERROR:
	{   rpmsql sql = (rpmsql) dbi->dbi_db;
	    sqlite3 * sqlI = (sqlite3 *) sql->I;
	    fprintf(stderr, "sqlite3_step: ERROR %d -- %s\n", rc, scp->cmd);
	    fprintf(stderr, "              %s (%d)\n",
			sqlite3_errmsg(sqlI), sqlite3_errcode(sqlI));
	    fprintf(stderr, "              cwd '%s'\n", getcwd(NULL,0));
	    loop = 0;
	}   /*@switchbreak@*/ break;
	case SQLITE_MISUSE:
	    fprintf(stderr, "sqlite3_step: MISUSE %d\n", rc);
	    loop = 0;
	    /*@switchbreak@*/ break;
	default:
	    fprintf(stderr, "sqlite3_step: rc %d\n", rc);
	    loop = 0;
	    /*@switchbreak@*/ break;
	}
    }

    if (rc == SQLITE_DONE)
	rc = SQLITE_OK;

    return rc;
}

static int sql_bind_key(dbiIndex dbi, SCP_t scp, int pos, DBT * key)
	/*@modifies dbi, scp @*/
{
    int rc = 0;

assert(key->data != NULL);
    switch (dbi->dbi_rpmtag) {
    case RPMDBI_PACKAGES:
	{   unsigned int hnum;
assert(key->size == sizeof(rpmuint32_t));
	    memcpy(&hnum, key->data, sizeof(hnum));

	    rc = cvtdberr(dbi, "sqlite3_bind_int",
			sqlite3_bind_int(scp->pStmt, pos, hnum));
	} break;
    default:
	switch (tagType(dbi->dbi_rpmtag) & RPM_MASK_TYPE) {
	default:
assert(0);	/* borken */
	case RPM_BIN_TYPE:
	    rc = cvtdberr(dbi, "sqlite3_bind_blob",
			sqlite3_bind_blob(scp->pStmt, pos,
				key->data, key->size, SQLITE_STATIC));
	    /*@innerbreak@*/ break;
	case RPM_UINT8_TYPE:
	{   rpmuint8_t i;
assert(key->size == sizeof(rpmuint8_t));
	    memcpy(&i, key->data, sizeof(i));
	    rc = cvtdberr(dbi, "sqlite3_bind_int",
	    		sqlite3_bind_int(scp->pStmt, pos, (int) i));
	} /*@innerbreak@*/ break;
	case RPM_UINT16_TYPE:
	{   rpmuint16_t i;
assert(key->size == sizeof(rpmuint16_t));
	    memcpy(&i, key->data, sizeof(i));
	    rc = cvtdberr(dbi, "sqlite3_bind_int",
			sqlite3_bind_int(scp->pStmt, pos, (int) i));
	} /*@innerbreak@*/ break;
	case RPM_UINT32_TYPE:
	{   rpmuint32_t i;
assert(key->size == sizeof(rpmuint32_t));
	    memcpy(&i, key->data, sizeof(i));

	    rc = cvtdberr(dbi, "sqlite3_bind_int",
			sqlite3_bind_int(scp->pStmt, pos, i));
	}   /*@innerbreak@*/ break;
	case RPM_UINT64_TYPE:
	{   rpmuint64_t i;
assert(key->size == sizeof(rpmuint64_t));
	    memcpy(&i, key->data, sizeof(i));

	    rc = cvtdberr(dbi, "sqlite3_bind_int64",
			sqlite3_bind_int64(scp->pStmt, pos, i));
	}   /*@innerbreak@*/ break;
	/*@innerbreak@*/ break;
	case RPM_STRING_TYPE:
	case RPM_I18NSTRING_TYPE:
	case RPM_STRING_ARRAY_TYPE:
	    rc = cvtdberr(dbi, "sqlite3_bind_text",
			sqlite3_bind_text(scp->pStmt, pos,
				key->data, key->size, SQLITE_STATIC));
	    /*@innerbreak@*/ break;
	}
    }

    return rc;
}

static int sql_bind_data(/*@unused@*/ dbiIndex dbi, SCP_t scp,
		int pos, DBT * data)
	/*@modifies scp @*/
{
    int rc;

assert(data->data != NULL);
    rc = cvtdberr(dbi, "sqlite3_bind_blob",
		sqlite3_bind_blob(scp->pStmt, pos,
			data->data, data->size, SQLITE_STATIC));

    return rc;
}

/* =================================================================== */
static int sql_exec(dbiIndex dbi, const char * cmd,
		int (*callback)(void*,int,char**,char**),
		void * context)
{
    rpmsql sql = (rpmsql) dbi->dbi_db;
    sqlite3 * sqlI = (sqlite3 *) sql->I;
    char * errmsg = NULL;
    int rc = cvtdberr(dbi, "sqlite3_exec",
		sqlite3_exec(sqlI, cmd, callback, context, &errmsg));

SQLDBDEBUG(dbi, (stderr, "%s\n<-- %s(%p,%p(%p)) rc %d %s\n", cmd, __FUNCTION__, dbi, callback, context, rc, (errmsg ? errmsg : "")));
    errmsg = _free(errmsg);
    return rc;
}

static int sql_busy_handler(void * _dbi, int time)
	/*@*/
{
    dbiIndex dbi = (dbiIndex) _dbi;
    int rc = 1;	/* assume retry */

#ifdef	DYING
    rpmlog(RPMLOG_WARNING, _("Unable to get lock on db %s, retrying... (%d)\n"),
		dbi->dbi_file, time);
#endif

    /* XXX FIXME: backoff timer with drop dead ceiling. */
    (void) sleep(1);

SQLDBDEBUG(dbi, (stderr, "<-- %s(%p,%d) rc %d\n", __FUNCTION__, _dbi, time, rc));
    return rc;
}

/* =================================================================== */

/* XXX FIXME: all the "new.key" fields in trigger need headerGet() getter */
static const char _Packages_sql_init[] = "\
  CREATE TABLE IF NOT EXISTS 'Packages' (\n\
    key		INTEGER UNIQUE PRIMARY KEY NOT NULL,\n\
    value	BLOB NOT NULL\n\
\n\
  );\n\
  CREATE TRIGGER IF NOT EXISTS insert_Packages AFTER INSERT ON 'Packages'\n\
    BEGIN\n\
      INSERT INTO 'Nvra' (key,value)		VALUES (\n\
	new.key, new.rowid );\n\
      INSERT INTO 'Packagecolor' (key,value)	VALUES (\n\
	new.key, new.rowid );\n\
      INSERT INTO 'Pubkeys' (key,value)		VALUES (\n\
	new.key, new.rowid );\n\
      INSERT INTO 'Sha1header' (key,value)	VALUES (\n\
	new.key, new.rowid );\n\
      INSERT INTO 'Installtid' (key,value)	VALUES (\n\
	new.key, new.rowid );\n\
      INSERT INTO 'Providename' (key,value)	VALUES (\n\
	new.key, new.rowid );\n\
      INSERT INTO 'Group' (key,value)		VALUES (\n\
	new.key, new.rowid );\n\
      INSERT INTO 'Release' (key,value)		VALUES (\n\
	new.key, new.rowid );\n\
      INSERT INTO 'Version' (key,value)		VALUES (\n\
	new.key, new.rowid );\n\
      INSERT INTO 'Name' (key,value)		VALUES (\n\
	new.key, new.rowid );\n\
    END;\n\
  CREATE TRIGGER IF NOT EXISTS delete_Packages BEFORE DELETE ON 'Packages'\n\
    BEGIN\n\
      DELETE FROM 'Nvra'	WHERE value = old.rowid;\n\
      DELETE FROM 'Packagecolor' WHERE value = old.rowid;\n\
      DELETE FROM 'Pubkeys'	WHERE value = old.rowid;\n\
      DELETE FROM 'Sha1header'	WHERE value = old.rowid;\n\
      DELETE FROM 'Installtid'	WHERE value = old.rowid;\n\
      DELETE FROM 'Providename'	WHERE value = old.rowid;\n\
      DELETE FROM 'Group'	WHERE value = old.rowid;\n\
      DELETE FROM 'Release'	WHERE value = old.rowid;\n\
      DELETE FROM 'Version'	WHERE value = old.rowid;\n\
      DELETE FROM 'Name'	WHERE value = old.rowid;\n\
    END;\n\
\n\
  CREATE TABLE IF NOT EXISTS 'Seqno' (\n\
    key		INTEGER,\n\
    value	INTEGER\n\
  );\n\
\n\
  CREATE TABLE IF NOT EXISTS 'Nvra' (\n\
    key		TEXT NOT NULL,\n\
    value	INTEGER REFERENCES 'Packages'\n\
  );\n\
\n\
  CREATE TABLE IF NOT EXISTS 'Packagecolor' (\n\
    key		INTEGER NOT NULL,\n\
    value	INTEGER REFERENCES 'Packages'\n\
  );\n\
\n\
  CREATE TABLE IF NOT EXISTS 'Pubkeys' (\n\
    key		BLOB NOT NULL,\n\
    value	INTEGER REFERENCES 'Packages'\n\
  );\n\
\n\
  CREATE TABLE IF NOT EXISTS 'Sha1header' (\n\
    key		TEXT NOT NULL,\n\
    value	INTEGER REFERENCES 'Packages'\n\
  );\n\
\n\
  CREATE TABLE IF NOT EXISTS 'Installtid' (\n\
    key		INTEGER NOT NULL,\n\
    value	INTEGER REFERENCES 'Packages'\n\
  );\n\
\n\
  CREATE TABLE IF NOT EXISTS 'Providename' (\n\
    key		TEXT NOT NULL,\n\
    value	INTEGER REFERENCES 'Packages'\n\
  );\n\
\n\
  CREATE TABLE IF NOT EXISTS 'Group' (\n\
    key		TEXT NOT NULL,\n\
    value	INTEGER REFERENCES 'Packages'\n\
  );\n\
\n\
  CREATE TABLE IF NOT EXISTS 'Release' (\n\
    key		TEXT NOT NULL,\n\
    value	INTEGER REFERENCES 'Packages'\n\
  );\n\
\n\
  CREATE TABLE IF NOT EXISTS 'Version' (\n\
    key		TEXT NOT NULL,\n\
    value	INTEGER REFERENCES 'Packages'\n\
  );\n\
\n\
  CREATE TABLE IF NOT EXISTS 'Name' (\n\
    key		TEXT NOT NULL,\n\
    value	INTEGER REFERENCES 'Packages'\n\
  );\n\
";

static const char * tagTypes[] = {
    "",
    "INTEGER NOT NULL",	/* RPM_UINT8_TYPE */
    "INTEGER NOT NULL",	/* RPM_UINT8_TYPE */
    "INTEGER NOT NULL",	/* RPM_UINT16_TYPE */
    "INTEGER NOT NULL",	/* RPM_UINT32_TYPE */
    "INTEGER NOT NULL",	/* RPM_UINT64_TYPE */
    "TEXT NOT NULL",	/* RPM_STRING_TYPE */
    "BLOB NOT NULL",	/* RPM_BIN_TYPE */
    "TEXT NOT NULL",	/* RPM_STRING_ARRAY_TYPE */
    "TEXT NOT NULL",	/* RPM_I18NSTRING_TYPE */
};
static size_t ntagTypes = sizeof(tagTypes) / sizeof(tagTypes[0]);

static int sql_initDB_cb(void * _dbi, int argc, char ** argv, char ** cols)
{
    dbiIndex dbi = (dbiIndex) _dbi;
    int rc = -1;
    if (dbi && argc == 1) {
	char * end = NULL;
	dbi->dbi_table_exists = strtoll(argv[0], &end, 10);
	if (end && *end == '\0') rc = 0;
    }
SQLDBDEBUG(dbi, (stderr, "<-- %s(%p,%p[%d],%p) rc %d table_exists %llu\n", __FUNCTION__, _dbi, argv, argc, cols, rc, (unsigned long long) (dbi ? dbi->dbi_table_exists : 0)));
    return rc;
}

/**
 * Verify the DB is setup.. if not initialize it
 *
 * Create the table.. create the db_info
 */
static int sql_initDB(dbiIndex dbi)
	/*@globals rpmGlobalMacroContext, h_errno, internalState @*/
	/*@modifies internalState @*/
{
    char cmd[BUFSIZ];
    int rc = 0;

#ifdef REFERENCE
PRAGMA auto_vacuum = 0 | NONE | 1 | FULL | 2 | INCREMENTAL;
PRAGMA automatic_index = boolean;
PRAGMA cache_size = pages | -kibibytes;
PRAGMA case_sensitive_like = boolean;
PRAGMA checkpoint_fullfsync = boolean;
PRAGMA collation_list;
PRAGMA compile_options;
PRAGMA count_changes = boolean;
PRAGMA database_list;
PRAGMA default_cache_size = Number-of-pages;
PRAGMA empty_result_callbacks = boolean;
PRAGMA encoding = "UTF-8" | "UTF-16" | "UTF-16le" | "UTF-16be"; 
PRAGMA foreign_key_list(table-name);
PRAGMA foreign_keys = boolean;
PRAGMA freelist_count;
PRAGMA full_column_names = boolean;
PRAGMA fullfsync = boolean;
PRAGMA ignore_check_constraints = boolean;
PRAGMA incremental_vacuum(N);
PRAGMA index_info(index-name);
PRAGMA index_list(table-name);
PRAGMA integrity_check(integer)
PRAGMA database.journal_mode = DELETE | TRUNCATE | PERSIST | MEMORY | WAL | OFF;
PRAGMA journal_size_limit = N;
PRAGMA legacy_file_format = boolean;
PRAGMA locking_mode = NORMAL | EXCLUSIVE;
PRAGMA max_page_count = N;
PRAGMA page_count;
PRAGMA page_size = bytes;
PRAGMA parser_trace = boolean;
PRAGMA quick_check(integer);
PRAGMA read_uncommitted = boolean;
PRAGMA recursive_triggers = boolean;
PRAGMA reverse_unordered_selects = boolean;
PRAGMA schema_version = integer;
PRAGMA user_version = integer;
PRAGMA database.secure_delete = boolean;
PRAGMA short_column_names = boolean; (deprecated)
PRAGMA shrink_memory;
PRAGMA synchronous = 0 | OFF | 1 | NORMAL | 2 | FULL;
PRAGMA table_info(table-name);
PRAGMA temp_store = 0 | DEFAULT | 1 | FILE | 2 | MEMORY;
PRAGMA temp_store_directory = 'directory-name';
PRAGMA vdbe_listing = boolean;
PRAGMA vdbe_trace = boolean;
PRAGMA wal_autocheckpoint=N;
PRAGMA database.wal_checkpoint(PASSIVE | FULL | RESTART);
PRAGMA writable_schema = boolean;
#endif

    if (dbi->dbi_tmpdir) {
	const char *root;
	const char *tmpdir;
	root = (dbi->dbi_root ? dbi->dbi_root : dbi->dbi_rpmdb->db_root);
	if ((root[0] == '/' && root[1] == '\0') || dbi->dbi_rpmdb->db_chrootDone)
	    root = NULL;
	tmpdir = rpmGenPath(root, dbi->dbi_tmpdir, NULL);
	if (rpmioMkpath(tmpdir, 0755, getuid(), getgid()) == 0) {
	    sprintf(cmd, "  PRAGMA temp_store_directory = '%s';", tmpdir);
	    rc = sql_exec(dbi, cmd, NULL, NULL);
	}
	tmpdir = _free(tmpdir);
    }

    if (dbi->dbi_eflags & DB_EXCL) {
	sprintf(cmd, "  PRAGMA locking_mode = EXCLUSIVE;");
	rc = sql_exec(dbi, cmd, NULL, NULL);
    }

    if (dbi->dbi_no_fsync) {
	static const char _cmd[] = "  PRAGMA synchronous = OFF;";
	rc = sql_exec(dbi, _cmd, NULL, NULL);
    }

#ifdef	DYING
    if (dbi->dbi_pagesize > 0) {
	sprintf(cmd, "  PRAGMA cache_size = %d;", dbi->dbi_cachesize);
	rc = sql_exec(dbi, cmd, NULL, NULL);
    }
    if (dbi->dbi_cachesize > 0) {
	sprintf(cmd, "  PRAGMA page_size = %d;", dbi->dbi_pagesize);
	rc = sql_exec(dbi, cmd, NULL, NULL);
    }
#endif

    /* Determine if table has already been initialized. */
    if (!dbi->dbi_table_exists) {
	sprintf(cmd, "\
  SELECT count(type) FROM 'sqlite_master' WHERE type='table' and name='%s';\n\
", dbi->dbi_subfile);
	rc = sql_exec(dbi, cmd, sql_initDB_cb, dbi);
	if (rc)
	    goto exit;
    }

    /* Create the table schema (if not yet done).  */
    if (!dbi->dbi_table_exists) {
	const char * valtype = "INTEGER REFERENCES Packages";
	const char * keytype;
	int kt;

	switch (dbi->dbi_rpmtag) {
	case RPMDBI_PACKAGES:
	    rc = sql_exec(dbi, _Packages_sql_init, NULL, NULL);
	    /*@ fallthrough @*/
	case RPMTAG_PUBKEYS:
	case RPMDBI_SEQNO:
	    keytype = NULL;
	    break;
	default:
	    kt = (tagType(dbi->dbi_rpmtag) & RPM_MASK_TYPE);
	    keytype = tagTypes[(kt > 0 && kt < (int)ntagTypes ? kt : 0)];
	    break;
	}

	if (keytype) {
	    /* XXX no need for IF NOT EXISTS */
	    /* XXX add per-table triggers? */
	    sprintf(cmd, "\
  CREATE %sTABLE IF NOT EXISTS '%s' (key %s, value %s);\
",
			dbi->dbi_temporary ? "TEMPORARY " : "",
			dbi->dbi_subfile, keytype, valtype);
	    rc = sql_exec(dbi, cmd, NULL, NULL);
	    if (rc)
		goto exit;
	}
    }

exit:
SQLDBDEBUG(dbi, (stderr, "<-- %s(%p) rc %d\n", __FUNCTION__, dbi, rc));
    return rc;
}

/**
 * Close database cursor.
 * @param dbi           index database handle
 * @param dbcursor      database cursor
 * @param flags         (unused)
 * @return              0 on success
 */
static int sql_cclose (dbiIndex dbi, /*@only@*/ DBC * dbcursor,
		unsigned int flags)
	/*@globals fileSystem, internalState @*/
	/*@modifies dbi, *dbcursor, fileSystem, internalState @*/
{
    SCP_t scp = (SCP_t) dbcursor;
    int rc = 0;

SQLDBDEBUG(dbi, (stderr, "==> sql_cclose(%p)\n", scp));

    if (scp->lkey)
	scp->lkey = _free(scp->lkey);

    if (scp->ldata)
	scp->ldata = _free(scp->ldata);

    scp = scpFree(scp);

SQLDBDEBUG(dbi, (stderr, "<-- %s(%p,%p,0x%x) rc %d\n", __FUNCTION__, dbi, dbcursor, flags, rc));
    return rc;
}

/**
 * Close index database, and destroy database handle.
 * @param dbi           index database handle
 * @param flags         (unused)
 * @return              0 on success
 */
static int sql_close(/*@only@*/ dbiIndex dbi, unsigned int flags)
	/*@globals fileSystem, internalState @*/
	/*@modifies dbi, fileSystem, internalState @*/
{
    rpmsql sql = (rpmsql) dbi->dbi_db;
    int rc = 0;

    if (sql) {

#if defined(MAYBE) /* XXX should SQLite and BDB have different semantics? */
	if (dbi->dbi_temporary && !(dbi->dbi_eflags & DB_PRIVATE)) {
	    const char * dbhome = NULL;
	    urltype ut = urlPath(dbi->dbi_home, &dbhome);
	    const char * dbfname = rpmGenPath(dbhome, dbi->dbi_file, NULL);
	    int xx = (dbfname ? Unlink(dbfname) : 0);
	    (void)ut; (void)xx;	/* XXX tell gcc to be quiet. */
	    dbfname = _free(dbfname);
	}
#endif

	/* XXX different than Berkeley DB: dbi->dbi_db is allocated. */
	sql = rpmsqlFree(sql);
	dbi->dbi_db = sql = NULL;

	rpmlog(RPMLOG_DEBUG, D_("closed   table          %s\n"),
		dbi->dbi_subfile);

    }

    (void) db3Free(dbi);

SQLDBDEBUG(dbi, (stderr, "<-- %s(%p,0x%x) rc %d\n", __FUNCTION__, dbi, flags, rc));
    return rc;
}

/**
 * Return handle for an index database.
 * @param rpmdb         rpm database
 * @param rpmtag        rpm tag
 * @retval *dbip	index database handle
 * @return              0 on success
 */
static int sql_open(rpmdb rpmdb, rpmTag rpmtag, /*@out@*/ dbiIndex * dbip)
	/*@globals rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
	/*@modifies *dbip, rpmGlobalMacroContext, fileSystem, internalState @*/
{
/*@-nestedextern -shadow @*/
    extern struct _dbiVec sqlitevec;
/*@=nestedextern -shadow @*/

    const char * urlfn = NULL;
    const char * dbhome = NULL;
    const char * dbfname = NULL;
    dbiIndex dbi = NULL;
    rpmsql sql = NULL;
    int rc = -1;	/* assume failure */
    int xx;

/* XXX dbi = NULL here */
SQLDBDEBUG(dbi, (stderr, "==> %s(%p,%s(%u),%p)\n", __FUNCTION__, rpmdb, tagName(rpmtag), rpmtag, dbip));

    if (dbip)
	*dbip = NULL;

    /* Parse db configuration parameters. */
    if ((dbi = db3New(rpmdb, rpmtag)) == NULL)
	goto exit;

   /* Get the prefix/root component and directory path */
    dbi->dbi_root = xstrdup(rpmdb->db_root);
    dbi->dbi_home = xstrdup(rpmdb->db_home);
    {	const char * s = tagName(dbi->dbi_rpmtag);
	dbi->dbi_file = xstrdup(s);
	dbi->dbi_subfile = xstrdup(s);
    }
    dbi->dbi_mode = O_RDWR;

    /*
     * Either the root or directory components may be a URL. Concatenate,
     * convert the URL to a path, and add the name of the file.
     */
    urlfn = rpmGenPath(NULL, dbi->dbi_home, NULL);
    (void) urlPath(urlfn, &dbhome);

    /* Create the %{sqldb} directory if it doesn't exist. */
    (void) rpmioMkpath(dbhome, 0755, getuid(), getgid());

    if (dbi->dbi_eflags & DB_PRIVATE)
	dbfname = xstrdup(":memory:");
    else {
#ifdef	DYING	/* XXX all tables in a single database file. */
	dbfname = rpmGenPath(dbhome, dbi->dbi_file, NULL);
#else
	dbfname = rpmGenPath(dbhome, "sqldb", NULL);
#endif
    }

    rpmlog(RPMLOG_DEBUG, D_("opening  table          %s (%s) mode=0x%x\n"),
		dbfname, dbi->dbi_subfile, dbi->dbi_mode);

    /* Open the Database */
    /* XXX use single handle for all tables in one database file? */
    {	const char * _av[] = { __FUNCTION__, NULL, NULL };
	int _flags = RPMSQL_FLAGS_CREATE;
	_av[1] = dbfname;
	sql = rpmsqlNew((char **) _av, _flags);
	if (sql == NULL || sql->I == NULL)
	    goto exit;
	rc = 0;
    }

    {	sqlite3 * sqlI = (sqlite3 *) sql->I;
	xx = cvtdberr(dbi, "sqlite3_busy_handler",
		sqlite3_busy_handler(sqlI, &sql_busy_handler, dbi));
    }

    dbi->dbi_db = (void *) sql;

    /* Initialize table */
    xx = sql_initDB(dbi);	/* XXX wire up rc == 0 on success */

exit:
    if (rc == 0 && dbi->dbi_db != NULL && dbip != NULL) {
	dbi->dbi_vec = &sqlitevec;
	*dbip = dbi;
	/* XXX BDB secondary -> primary db3Associate() */
	/* XXX BDB Seqno seqid_init() */
    } else {
	if (dbi) {
	    (void) sql_close(dbi, 0);
	}
	dbi = NULL;
	if (dbip) *dbip = dbi;
    }

    urlfn = _free(urlfn);
    dbfname = _free(dbfname);

SQLDBDEBUG(dbi, (stderr, "<== %s(%p,%s(%u),%p) rc %d dbi %p\n", __FUNCTION__, rpmdb, tagName(rpmtag), rpmtag, dbip, rc, (dbip ? *dbip : NULL)));
    return rc;
}

/**
 * Flush pending operations to disk.
 * @param dbi           index database handle
 * @param flags         (unused)
 * @return              0 on success
 */
static int sql_sync (dbiIndex dbi, /*@unused@*/ unsigned int flags)
	/*@globals fileSystem, internalState @*/
	/*@modifies fileSystem, internalState @*/
{
    int rc = 0;

    /* XXX FIXME: implement. */

SQLDBDEBUG(dbi, (stderr, "<-- %s(%p,0x%x) rc %d\n", __FUNCTION__, dbi, flags, rc));
    return rc;
}

/** \ingroup dbi
 * Return whether key exists in a database.
 * @param dbi		index database handle
 * @param key		retrieve key value/length/flags
 * @param flags		usually 0
 * @return		0 if key exists, DB_NOTFOUND if not, else error
 */
static int sql_exists(dbiIndex dbi, DBT * key, unsigned int flags)
	/*@globals fileSystem @*/
	/*@modifies fileSystem @*/
{
    int rc = EINVAL;
SQLDBDEBUG(dbi, (stderr, "<-- %s(%p,%p,0x%x) rc %d %s\n", __FUNCTION__, dbi, key, flags, rc, _KEYDATA(key, NULL, NULL, NULL)));
    return rc;
}

#ifdef REFERENCE
UPDATE SQLITE_SEQUENCE SET seq = 0 WHERE name = 'MyTable';
#endif
static const char seqno_inc_cmd[] = "\
  BEGIN EXCLUSIVE TRANSACTION;\n\
  REPLACE INTO Seqno VALUES (0,\n\
      COALESCE((SELECT value FROM Seqno WHERE key == 0), 0) + 1);\n\
  SELECT value FROM Seqno WHERE key == 0;\n\
  COMMIT TRANSACTION;\n\
";

static int sql_seqno_cb(void * _dbi, int argc, char ** argv, char ** cols)
{
    dbiIndex dbi = (dbiIndex) _dbi;
    int rc = -1;
    if (dbi && argc == 1) {
	char * end = NULL;
	dbi->dbi_seqno = strtoll(argv[0], &end, 10);
	if (end && *end) rc = 0;
	if (end && *end == '\0') rc = 0;
    }
SQLDBDEBUG(dbi, (stderr, "<-- %s(%p,%p[%d],%p) rc %d seqno %llu\n", __FUNCTION__, _dbi, argv, argc, cols, rc, (unsigned long long) (dbi ? dbi->dbi_seqno : 0)));
    return rc;
}

/** \ingroup dbi
 * Return next sequence number.
 * @param dbi		index database handle (with attached sequence)
 * @retval *seqnop	IN: delta (0 does seqno++) OUT: returned 64bit seqno
 * @param flags		usually 0
 * @return		0 on success
 */
static int sql_seqno(dbiIndex dbi, int64_t * seqnop, unsigned int flags)
{
    int rc = EINVAL;
    if (dbi && seqnop) {
	/* XXX DB_SEQNO has min:max increment/decrement and name */
	rc = sql_exec(dbi, seqno_inc_cmd, sql_seqno_cb, dbi);
	if (!rc) *seqnop = dbi->dbi_seqno;
    }
SQLDBDEBUG(dbi, (stderr, "<-- %s(%p,%p,0x%x) rc %d seqno %llu\n", __FUNCTION__, dbi, seqnop, flags, rc, (unsigned long long) (seqnop ? *seqnop : 0xdeadbeef)));
    return rc;
}

/**
 * Open database cursor.
 * @param dbi		index database handle
 * @param txnid		database transaction handle
 * @retval dbcp		address of new database cursor
 * @param flags		DB_WRITECURSOR or 0
 * @return		0 on success
 */
static int sql_copen (dbiIndex dbi,
		/*@unused@*/ /*@null@*/ DB_TXN * txnid,
		/*@out@*/ DBC ** dbcp, unsigned int flags)
	/*@globals fileSystem, internalState @*/
	/*@modifies dbi, *txnid, *dbcp, fileSystem, internalState @*/
{
    rpmsql sql = (rpmsql) dbi->dbi_db;
    SCP_t scp = scpNew(sql);
    DBC * dbcursor = (DBC *)scp;
    int rc = 0;

SQLDBDEBUG(dbi, (stderr, "==> %s(%s) tag %d type %d scp %p\n", __FUNCTION__, tagName(dbi->dbi_rpmtag), dbi->dbi_rpmtag, (tagType(dbi->dbi_rpmtag) & RPM_MASK_TYPE), scp));

    if (dbcp)
	*dbcp = dbcursor;
    else
	(void) sql_cclose(dbi, dbcursor, 0);

SQLDBDEBUG(dbi, (stderr, "<== %s(%p,%p,%p,0x%x) rc %d subfile %s\n", __FUNCTION__, dbi, txnid, dbcp, flags, rc, dbi->dbi_subfile));
    return rc;
}

/**
 * Delete (key,data) pair(s) using db->del or dbcursor->c_del.
 * @param dbi           index database handle
 * @param dbcursor      database cursor (NULL will use db->del)
 * @param key           delete key value/length/flags
 * @param data          delete data value/length/flags
 * @param flags         (unused)
 * @return              0 on success
 */
static int sql_cdel (dbiIndex dbi, /*@null@*/ DBC * dbcursor, DBT * key,
		DBT * data, unsigned int flags)
	/*@globals fileSystem, internalState @*/
	/*@modifies dbi, *dbcursor, fileSystem, internalState @*/
{
    rpmsql sql = (rpmsql) dbi->dbi_db;
    sqlite3 * sqlI = (sqlite3 *) sql->I;
    SCP_t scp = scpLink(dbcursor);	/* XXX scpNew() instead? */
    int rc = 0;

dbg_keyval(__FUNCTION__, dbi, dbcursor, key, data, flags);

assert(scp->cmd == NULL);	/* XXX memleak prevention */
    scp->cmd = sqlite3_mprintf("DELETE FROM '%q' WHERE key=? AND value=?;",
	dbi->dbi_subfile);

    rc = cvtdberr(dbi, "sqlite3_prepare",
		sqlite3_prepare(sqlI, scp->cmd, (int)strlen(scp->cmd),
		&scp->pStmt, (const char **) &scp->pzErrmsg));
    if (rc) rpmlog(RPMLOG_WARNING, "cdel(%s) prepare %s (%d)\n", dbi->dbi_subfile, sqlite3_errmsg(sqlI), rc);
    rc = cvtdberr(dbi, "sql_bind_key",
		sql_bind_key(dbi, scp, 1, key));
    if (rc) rpmlog(RPMLOG_WARNING, "cdel(%s) bind key %s (%d)\n", dbi->dbi_subfile, sqlite3_errmsg(sqlI), rc);
    rc = cvtdberr(dbi, "sql_bind_data",
		sql_bind_data(dbi, scp, 2, data));
    if (rc) rpmlog(RPMLOG_WARNING, "cdel(%s) bind data %s (%d)\n", dbi->dbi_subfile, sqlite3_errmsg(sqlI), rc);

    rc = cvtdberr(dbi, "sql_step",
		sql_step(dbi, scp));
    if (rc) rpmlog(RPMLOG_WARNING, "cdel(%s) sql_step rc %d\n", dbi->dbi_subfile, rc);

    scp = scpFree(scp);

SQLDBDEBUG(dbi, (stderr, "<-- %s(%p,%p,%p,%p,0x%x) rc %d subfile %s %s\n", __FUNCTION__, dbi, dbcursor, key, data, flags, rc, dbi->dbi_subfile, _KEYDATA(key, NULL, data, NULL)));
    return rc;
}

/**
 * Retrieve (key,data) pair using db->get or dbcursor->c_get.
 * @param dbi           index database handle
 * @param dbcursor      database cursor (NULL will use db->get)
 * @param key           retrieve key value/length/flags
 * @param data          retrieve data value/length/flags
 * @param flags         (unused)
 * @return              0 on success
 */
static int sql_cget (dbiIndex dbi, /*@null@*/ DBC * dbcursor, DBT * key,
		DBT * data, unsigned int flags)
	/*@globals fileSystem, internalState @*/
	/*@modifies dbi, dbcursor, *key, *data, fileSystem, internalState @*/
{
    rpmsql sql = (rpmsql) dbi->dbi_db;
    sqlite3 * sqlI = (sqlite3 *) sql->I;
    SCP_t scp = (SCP_t)dbcursor;
    int rc = 0;
    int ix;

assert(dbcursor != NULL);
dbg_keyval(__FUNCTION__, dbi, dbcursor, key, data, flags);

    /* First determine if this is a new scan or existing scan */

SQLDBDEBUG(dbi, (stderr, "\tcget(%s) scp %p rc %d flags %d av %p\n",
		dbi->dbi_subfile, scp, rc, flags, scp->av));
    if (flags == DB_SET || scp->used == 0) {
	scp->used = 1; /* Signal this scp as now in use... */
	scp = scpReset(scp);	/* Free av and avlen, reset counters.*/

/* XXX: Should we also reset the key table here?  Can you re-use a cursor? */

	/* If we're scanning everything, load the iterator key table */
	if (key->size == 0) {
	    scp->all = 1;

/*
 * The only condition not dealt with is if there are multiple identical keys.  This can lead
 * to later iteration confusion.  (It may return the same value for the multiple keys.)
 */

assert(scp->cmd == NULL);	/* XXX memleak prevention */
	    switch (dbi->dbi_rpmtag) {
	    case RPMDBI_PACKAGES:
	        scp->cmd = sqlite3_mprintf("SELECT key FROM '%q' ORDER BY key;",
		    dbi->dbi_subfile);
	        break;
	    default:
	        scp->cmd = sqlite3_mprintf("SELECT key FROM '%q';",
		    dbi->dbi_subfile);
	        break;
	    }
	    rc = cvtdberr(dbi, "sqlite3_prepare",
			sqlite3_prepare(sqlI, scp->cmd,
				(int)strlen(scp->cmd), &scp->pStmt,
				(const char **) &scp->pzErrmsg));
	    if (rc) rpmlog(RPMLOG_WARNING, "cget(%s) sequential prepare %s (%d)\n", dbi->dbi_subfile, sqlite3_errmsg(sqlI), rc);

	    rc = sql_step(dbi, scp);
	    if (rc) rpmlog(RPMLOG_WARNING, "cget(%s) sequential sql_step rc %d\n", dbi->dbi_subfile, rc);

	    scp = scpResetKeys(scp);
	    scp->nkeys = scp->nr;
	    scp->keys = (DBT **) xcalloc(scp->nkeys, sizeof(*scp->keys));
	    for (ix = 0; ix < scp->nkeys; ix++) {
		scp->keys[ix] = (DBT *) xmalloc(sizeof(*scp->keys[0]));
		scp->keys[ix]->size = (UINT32_T) scp->avlen[ix+1];
		scp->keys[ix]->data = (void *) xmalloc(scp->keys[ix]->size);
		memcpy(scp->keys[ix]->data, scp->av[ix+1], scp->avlen[ix+1]);
	    }
	} else {
	    /* We're only scanning ONE element */
	    scp = scpResetKeys(scp);
	    scp->nkeys = 1;
	    scp->keys = (DBT **) xcalloc(scp->nkeys, sizeof(*scp->keys));
	    scp->keys[0] = (DBT *) xmalloc(sizeof(*scp->keys[0]));
	    scp->keys[0]->size = key->size;
	    scp->keys[0]->data = (void *) xmalloc(scp->keys[0]->size);
	    memcpy(scp->keys[0]->data, key->data, key->size);
	}

	scp = scpReset(scp);

	/* Prepare SQL statement to retrieve the value for the current key */
assert(scp->cmd == NULL);	/* XXX memleak prevention */
	scp->cmd = sqlite3_mprintf("SELECT value FROM '%q' WHERE key=?;", dbi->dbi_subfile);
	rc = cvtdberr(dbi, "sqlite3_prepare",
		sqlite3_prepare(sqlI, scp->cmd, (int)strlen(scp->cmd),
			&scp->pStmt, (const char **) &scp->pzErrmsg));

	if (rc) rpmlog(RPMLOG_WARNING, "cget(%s) prepare %s (%d)\n", dbi->dbi_subfile, sqlite3_errmsg(sqlI), rc);
    }

    scp = scpResetAv(scp);

    /* Now continue with a normal retrive based on key */
    if ((scp->rx + 1) > scp->nkeys )
	rc = DB_NOTFOUND; /* At the end of the list */

    if (rc != 0)
	goto exit;

    /* Bind key to prepared statement */
    rc = cvtdberr(dbi, "sql_bind_key",
		sql_bind_key(dbi, scp, 1, scp->keys[scp->rx]));
    if (rc) rpmlog(RPMLOG_WARNING, "cget(%s)  key bind %s (%d)\n", dbi->dbi_subfile, sqlite3_errmsg(sqlI), rc);

    rc = cvtdberr(dbi, "sql_step",
		sql_step(dbi, scp));
    if (rc) rpmlog(RPMLOG_WARNING, "cget(%s) sql_step rc %d\n", dbi->dbi_subfile, rc);

    rc = cvtdberr(dbi, "sqlite3_reset",
		sqlite3_reset(scp->pStmt));
    if (rc) rpmlog(RPMLOG_WARNING, "reset %d\n", rc);

/* 1 key should return 0 or 1 row/value */
assert(scp->nr < 2);

    if (scp->nr == 0 && scp->all == 0)
	rc = DB_NOTFOUND; /* No data for that key found! */

    if (rc != 0)
	goto exit;

    /* If we're looking at the whole db, return the key */
    if (scp->all) {

assert(scp->nr == 1);	/* XXX Ensure no duplicate keys */

	if (scp->lkey)
	    scp->lkey = _free(scp->lkey);

	key->size = scp->keys[scp->rx]->size;
	key->data = (void *) xmalloc(key->size);
	if (! (key->flags & DB_DBT_MALLOC))
	    scp->lkey = key->data;

	(void) memcpy(key->data, scp->keys[scp->rx]->data, key->size);
    }

    /* Construct and return the data element (element 0 is "value", 1 is _THE_ value)*/
    switch (dbi->dbi_rpmtag) {
    default:
	if (scp->ldata)
	    scp->ldata = _free(scp->ldata);

	data->size = (UINT32_T) scp->avlen[1];
	data->data = (void *) xmalloc(data->size);
	if (! (data->flags & DB_DBT_MALLOC) )
	    scp->ldata = data->data;

	(void) memcpy(data->data, scp->av[1], data->size);
    }

    scp->rx++;

    /* XXX FIXME: ptr alignment is fubar here. */
SQLDBDEBUG(dbi, (stderr, "\tcget(%s) found  key 0x%x (%d)\n", dbi->dbi_subfile,
		key->data == NULL ? 0 : *(unsigned int *)key->data, key->size));
SQLDBDEBUG(dbi, (stderr, "\tcget(%s) found data 0x%x (%d)\n", dbi->dbi_subfile,
		key->data == NULL ? 0 : *(unsigned int *)data->data, data->size));

exit:
    if (rc == DB_NOTFOUND) {
SQLDBDEBUG(dbi, (stderr, "\tcget(%s) not found\n", dbi->dbi_subfile));
    }

SQLDBDEBUG(dbi, (stderr, "<-- %s(%p,%p,%p,%p,0x%x) rc %d subfile %s %s\n", __FUNCTION__, dbi, dbcursor, key, data, flags, rc, dbi->dbi_subfile, _KEYDATA(key, NULL, data, NULL)));
    return rc;
}

/**
 * Store (key,data) pair using db->put or dbcursor->c_put.
 * @param dbi           index database handle
 * @param dbcursor      database cursor (NULL will use db->put)
 * @param key           store key value/length/flags
 * @param data          store data value/length/flags
 * @param flags         (unused)
 * @return              0 on success
 */
static int sql_cput (dbiIndex dbi, /*@null@*/ DBC * dbcursor, DBT * key,
			DBT * data, unsigned int flags)
	/*@globals fileSystem, internalState @*/
	/*@modifies dbi, *dbcursor, fileSystem, internalState @*/
{
    rpmsql sql = (rpmsql) dbi->dbi_db;
    sqlite3 * sqlI = (sqlite3 *) sql->I;
    SCP_t scp = scpLink(dbcursor);	/* XXX scpNew() instead? */
    int rc = 0;

dbg_keyval("sql_cput", dbi, dbcursor, key, data, flags);

assert(scp->cmd == NULL);	/* XXX memleak prevention */
    switch (dbi->dbi_rpmtag) {
    default:
	/* XXX sqlite3_prepare() persistence */
	scp->cmd = sqlite3_mprintf("INSERT OR REPLACE INTO '%q' VALUES(?, ?);",
		dbi->dbi_subfile);
	rc = cvtdberr(dbi, "sqlite3_prepare",
		sqlite3_prepare(sqlI, scp->cmd, (int)strlen(scp->cmd),
			&scp->pStmt, (const char **) &scp->pzErrmsg));
	rc = cvtdberr(dbi, "sql_bind_key",
		sql_bind_key(dbi, scp, 1, key));
	rc = cvtdberr(dbi, "sql_bind_data",
		sql_bind_data(dbi, scp, 2, data));

	rc = sql_step(dbi, scp);
	if (rc) rpmlog(RPMLOG_WARNING, "cput(%s) sql_step rc %d\n", dbi->dbi_subfile, rc);

	break;
    }

    scp = scpFree(scp);

SQLDBDEBUG(dbi, (stderr, "<-- %s(%p,%p,%p,%p,0x%x) rc %d subfile %s %s\n", __FUNCTION__, dbi, dbcursor, key, data, flags, rc, dbi->dbi_subfile, _KEYDATA(key, NULL, data, NULL)));
    return rc;
}

/**
 * Is database byte swapped?
 * @param dbi           index database handle
 * @return              0 no
 */
static int sql_byteswapped (dbiIndex dbi)
	/*@globals fileSystem, internalState @*/
	/*@modifies fileSystem, internalState @*/
{
    int rc = 0;		/* XXX FIXME: assume native always */
#if defined(NOISY)
SQLDBDEBUG(dbi, (stderr, "<-- %s(%p) rc %d subfile %s\n", __FUNCTION__, dbi, rc, dbi->dbi_subfile));
#endif
    return rc;
}

/**
 * Associate secondary database with primary.
 * @param dbi           index database handle
 * @param dbisecondary  secondary index database handle
 * @param callback      create secondary key from primary (NULL if DB_RDONLY)
 * @param flags         DB_CREATE or 0
 * @return              0 on success
 */
static int sql_associate (/*@unused@*/ dbiIndex dbi,
		/*@unused@*/ dbiIndex dbisecondary,
    /*@unused@*/int (*callback) (DB *, const DBT *, const DBT *, DBT *),
		/*@unused@*/ unsigned int flags)
	/*@*/
{
    int rc = EINVAL;
SQLDBDEBUG(dbi, (stderr, "<-- %s(%p,%p,%p,0x%x) rc %d subfile %s\n", __FUNCTION__, dbi, dbisecondary, callback, flags, rc, dbi->dbi_subfile));
    return rc;
}

/**
 * Associate secondary database with primary.
 * @param dbi           index database handle
 * @param dbisecondary  secondary index database handle
 * @param callback      create secondary key from primary (NULL if DB_RDONLY)
 * @param flags         DB_CREATE or 0
 * @return              0 on success
 */
static int sql_associate_foreign (/*@unused@*/ dbiIndex dbi,
		/*@unused@*/ dbiIndex dbisecondary,
    /*@unused@*/int (*callback) (DB *, const DBT *, DBT *, const DBT *, int *),
		/*@unused@*/ unsigned int flags)
	/*@*/
{
    int rc = EINVAL;
SQLDBDEBUG(dbi, (stderr, "<-- %s(%p,%p,%p,0x%x) rc %d subfile %s\n", __FUNCTION__, dbi, dbisecondary, callback, flags, rc, dbi->dbi_subfile));
    return rc;
}

/**
 * Return join cursor for list of cursors.
 * @param dbi           index database handle
 * @param curslist      NULL terminated list of database cursors
 * @retval dbcp         address of join database cursor
 * @param flags         DB_JOIN_NOSORT or 0
 * @return              0 on success
 */
static int sql_join (/*@unused@*/ dbiIndex dbi,
		/*@unused@*/ DBC ** curslist,
		/*@unused@*/ /*@out@*/ DBC ** dbcp,
		/*@unused@*/ unsigned int flags)
	/*@globals fileSystem @*/
	/*@modifies dbi, *dbcp, fileSystem @*/
{
    int rc = EINVAL;
SQLDBDEBUG(dbi, (stderr, "<-- %s(%p,%p,%p,0x%x) rc %d\n", __FUNCTION__, dbi, curslist, dbcp, flags, rc));
    return rc;
}

/**
 * Duplicate a database cursor.
 * @param dbi           index database handle
 * @param dbcursor      database cursor
 * @retval dbcp         address of new database cursor
 * @param flags         DB_POSITION for same position, 0 for uninitialized
 * @return              0 on success
 */
static int sql_cdup (/*@unused@*/ dbiIndex dbi,
		/*@unused@*/ DBC * dbcursor,
		/*@unused@*/ /*@out@*/ DBC ** dbcp,
		/*@unused@*/ unsigned int flags)
	/*@globals fileSystem @*/
	/*@modifies dbi, *dbcp, fileSystem @*/
{
    int rc = EINVAL;
SQLDBDEBUG(dbi, (stderr, "<-- %s(%p,%p,%p,0x%x) rc %d\n", __FUNCTION__, dbi, dbcursor, dbcp, flags, rc));
    return rc;
}

/**
 * Retrieve (key,data) pair using dbcursor->c_pget.
 * @param dbi           index database handle
 * @param dbcursor      database cursor
 * @param key           secondary retrieve key value/length/flags
 * @param pkey          primary retrieve key value/length/flags
 * @param data          primary retrieve data value/length/flags
 * @param flags         DB_NEXT, DB_SET, or 0
 * @return              0 on success
 */
static int sql_cpget (/*@unused@*/ dbiIndex dbi,
		/*@unused@*/ /*@null@*/ DBC * dbcursor,
		/*@unused@*/ DBT * key,
		/*@unused@*/ DBT * pkey,
		/*@unused@*/ DBT * data,
		/*@unused@*/ unsigned int flags)
	/*@globals fileSystem @*/
	/*@modifies *dbcursor, *key, *pkey, *data, fileSystem @*/
{
    int rc = EINVAL;
SQLDBDEBUG(dbi, (stderr, "<-- %s(%p,%p,%p,%p,%p,0x%x) rc %d %s\n", __FUNCTION__, dbi, dbcursor, key, pkey, data, flags, rc, _KEYDATA(key, pkey, data, NULL)));
    return rc;
}

/**
 * Retrieve count of (possible) duplicate items using dbcursor->c_count.
 * @param dbi           index database handle
 * @param dbcursor      database cursor
 * @param countp        address of count
 * @param flags         (unused)
 * @return              0 on success
 */
static int sql_ccount (/*@unused@*/ dbiIndex dbi,
		/*@unused@*/ DBC * dbcursor,
		/*@unused@*/ /*@out@*/ unsigned int * countp,
		/*@unused@*/ unsigned int flags)
	/*@globals fileSystem @*/
	/*@modifies *dbcursor, fileSystem @*/
{
    int rc = EINVAL;
SQLDBDEBUG(dbi, (stderr, "<-- %s(%p,%p,%p,0x%x) rc %d\n", __FUNCTION__, dbi, dbcursor, countp, flags, rc));
    return rc;
}

static int sql_stat_cb(void * _dbi, int argc, char ** argv, char ** cols)
{
    dbiIndex dbi = (dbiIndex) _dbi;
    int rc = -1;
    if (dbi && argc == 1) {
	char * end = NULL;
	dbi->dbi_table_nkeys = strtoll(argv[0], &end, 10);
	if (end && *end == '\0') rc = 0;
    }
SQLDBDEBUG(dbi, (stderr, "<-- %s(%p,%p[%d],%p) rc %d table_nkeys %llu\n", __FUNCTION__, _dbi, argv, argc, cols, rc, (unsigned long long) (dbi ? dbi->dbi_table_nkeys : 0)));
    return rc;
}

/** \ingroup dbi
 * Save statistics in database handle.
 * @param dbi           index database handle
 * @param flags         retrieve statistics that don't require traversal?
 * @return              0 on success
 */
static int sql_stat (dbiIndex dbi, /*@unused@*/ unsigned int flags)
	/*@globals fileSystem, internalState @*/
	/*@modifies dbi, fileSystem, internalState @*/
{
    char * cmd;
    int rc = 0;

    dbi->dbi_table_nkeys = -1;

    cmd = sqlite3_mprintf("  SELECT COUNT('key') FROM '%q';", dbi->dbi_subfile);
    rc = sql_exec(dbi, cmd, sql_stat_cb, dbi);
    cmd = _free(cmd);

    if (dbi->dbi_table_nkeys < 0)
	dbi->dbi_table_nkeys = 4096;  /* XXX hacky */

    dbi->dbi_stats = _free(dbi->dbi_stats);
    {	DB_HASH_STAT * _stats = (DB_HASH_STAT *) xcalloc(1, sizeof(*_stats));
	_stats->hash_nkeys = dbi->dbi_table_nkeys;
	dbi->dbi_stats = (void *) _stats;
    }

SQLDBDEBUG(dbi, (stderr, "<-- %s(%p,0x%x) rc %d subfile %s\n", __FUNCTION__, dbi, flags, rc, dbi->dbi_subfile));
    return rc;
}

/* Major, minor, patch version of DB.. we're not using db.. so set to 0 */
/* open, close, sync, associate, asociate_foreign, join */
/* cursor_open, cursor_close, cursor_dup, cursor_delete, cursor_get, */
/* cursor_pget?, cursor_put, cursor_count */
/* db_bytewapped, stat */
/*@observer@*/ /*@unchecked@*/
struct _dbiVec sqlitevec = {
    "Sqlite " SQLITE_VERSION,
    ((SQLITE_VERSION_NUMBER / (1000 * 1000)) % 1000),
    ((SQLITE_VERSION_NUMBER / (       1000)) % 1000),
    ((SQLITE_VERSION_NUMBER                ) % 1000),
    sql_open,
    sql_close,
    sql_sync,
    sql_associate,
    sql_associate_foreign,
    sql_join,
    sql_exists,
    sql_seqno,
    sql_copen,
    sql_cclose,
    sql_cdup,
    sql_cdel,
    sql_cget,
    sql_cpget,
    sql_cput,
    sql_ccount,
    sql_byteswapped,
    sql_stat
};

/*@=evalorderuncon@*/
/*@=modfilesystem@*/
/*@=compmempass@*/
/*@=compdef@*/
/*@=moduncon@*/
/*@=noeffectuncon@*/
/*@=globuse@*/
/*@=paramuse@*/
/*@=mustmod@*/
