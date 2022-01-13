/*-
 * Copyright (c) 1989, 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef lint
static const char copyright[] =
"@(#) Copyright (c) 1989, 1990, 1993\n\
	The Regents of the University of California.  All rights reserved.\n";
#endif /* not lint */

#include "system.h"

#include <fnmatch.h>
#include <signal.h>
#include <stdarg.h>

#if defined(__QNXNTO__)
#define	st_mtimespec	st_mtime
#endif

#include <rpmio_internal.h>	/* XXX fdInitDigest() et al */
#include <fts.h>
#include <ugid.h>
#include <poptIO.h>

#undef	_RPMFI_INTERNAL		/* XXX don't enable *.rpm/ containers yet. */
#if defined(_RPMFI_INTERNAL)
#define	_RPMAV_INTERNAL
#include <rpmdav.h>
#include <rpmtag.h>
#include <rpmfi.h>

#include <rpmlib.h>		/* XXX for rpmts typedef */
#include <rpmts.h>
#endif

#define	RPM_LIST_HEAD(name, type) \
    struct name { struct type *lh_first; }
#define	RPM_LIST_ENTRY(type) \
    struct { struct type *le_next;struct type **le_prev; }
#define RPM_LIST_EMPTY(head) \
    ((head)->lh_first == NULL)
#define RPM_LIST_FIRST(head) \
    ((head)->lh_first)
#define	RPM_LIST_NEXT(elm, field) \
    ((elm)->field.le_next)
#define	RPM_LIST_INIT(head) \
    do { RPM_LIST_FIRST((head)) = NULL; } while (0)
#define	RPM_LIST_INSERT_HEAD(head, elm, field) \
    do { if ((RPM_LIST_NEXT((elm), field) = RPM_LIST_FIRST((head))) != NULL) \
        RPM_LIST_FIRST((head))->field.le_prev = &RPM_LIST_NEXT((elm), field);\
    RPM_LIST_FIRST((head)) = (elm); \
    (elm)->field.le_prev = &RPM_LIST_FIRST((head)); } while (0)
#define RPM_LIST_FOREACH(var, head, field) \
    for ((var) = RPM_LIST_FIRST((head)); (var); (var) = RPM_LIST_NEXT((var), field))

#define	_MTREE_INTERNAL
/*==============================================================*/

#define	_KFB(n)	(1U << (n))
#define	_MFB(n)	(_KFB(n) | 0x40000000)

/**
 * Bit field enum for mtree CLI options.
 */
enum mtreeFlags_e {
    MTREE_FLAGS_NONE		= 0,
    MTREE_FLAGS_QUIET		= _MFB( 0), /*!< -q,--quiet ... */
    MTREE_FLAGS_WARN		= _MFB( 1), /*!< -w,--warn ... */
    MTREE_FLAGS_CREATE		= _MFB( 2), /*!< -c,--create ... */
    MTREE_FLAGS_DIRSONLY	= _MFB( 3), /*!< -d,--dirs ... */
    MTREE_FLAGS_IGNORE		= _MFB( 4), /*!< -e,--ignore ... */
    MTREE_FLAGS_INDENT		= _MFB( 5), /*!< -i,--indent ... */
    MTREE_FLAGS_LOOSE		= _MFB( 6), /*!< -l,--loose ... */
    MTREE_FLAGS_NOCOMMENT	= _MFB( 7), /*!< -n,--nocomment ... */
    MTREE_FLAGS_REMOVE		= _MFB( 8), /*!< -r,--remove ... */
    MTREE_FLAGS_SEEDED		= _MFB( 9), /*!< -s,--seed ... */
    MTREE_FLAGS_TOUCH		= _MFB(10), /*!< -t,--touch ... */
    MTREE_FLAGS_UPDATE		= _MFB(11), /*!< -u,--update ... */
    MTREE_FLAGS_MISMATCHOK	= _MFB(12), /*!< -U,--mismatch ... */
	/* 13-31 unused */
};

/**
 */
typedef struct rpmfts_s * rpmfts;

#if defined(_MTREE_INTERNAL)
/**
 * Bit field enum for mtree keys.
 */
enum mtreeKeys_e {
    MTREE_KEYS_NONE		= 0,
    MTREE_KEYS_CKSUM		= _KFB( 0),	/*!< checksum */
    MTREE_KEYS_DONE		= _KFB( 1),	/*!< directory done */
    MTREE_KEYS_GID		= _KFB( 2),	/*!< gid */
    MTREE_KEYS_GNAME		= _KFB( 3),	/*!< group name */
    MTREE_KEYS_IGN		= _KFB( 4),	/*!< ignore */
    MTREE_KEYS_MAGIC		= _KFB( 5),	/*!< name has magic chars */
    MTREE_KEYS_MODE		= _KFB( 6),	/*!< mode */
    MTREE_KEYS_NLINK		= _KFB( 7),	/*!< number of links */
    MTREE_KEYS_SIZE		= _KFB( 8),	/*!< size */
    MTREE_KEYS_SLINK		= _KFB( 9),	/*!< link count */
    MTREE_KEYS_TIME		= _KFB(10),	/*!< modification time */
    MTREE_KEYS_TYPE		= _KFB(11),	/*!< file type */
    MTREE_KEYS_UID		= _KFB(12),	/*!< uid */
    MTREE_KEYS_UNAME		= _KFB(13),	/*!< user name */
    MTREE_KEYS_VISIT		= _KFB(14),	/*!< file visited */
    MTREE_KEYS_FLAGS		= _KFB(15),	/*!< file flags */
    MTREE_KEYS_NOCHANGE		= _KFB(16),	/*!< do not change owner/mode */
    MTREE_KEYS_OPT		= _KFB(17),	/*!< existence optional */
    MTREE_KEYS_DIGEST		= _KFB(18),	/*!< digest */
	/* 19-31 unused */
};

typedef struct _node {
    struct _node	*parent, *child;	/*!< up, down */
    struct _node	*prev, *next;		/*!< left, right */
    struct stat		sb;			/*!< parsed stat(2) info */
    char		*slink;			/*!< symbolic link reference */
    ARGI_t		algos;			/*!< digest algorithms */
    ARGV_t		digests;		/*!< digest strings */

    uint32_t		cksum;			/*!< check sum */

    enum mtreeKeys_e flags;			/*!< items set */

    uint8_t		type;			/*!< file type */
#define	F_BLOCK	0x001				/*!< block special */
#define	F_CHAR	0x002				/*!< char special */
#define	F_DIR	0x004				/*!< directory */
#define	F_FIFO	0x008				/*!< fifo */
#define	F_FILE	0x010				/*!< regular file */
#define	F_LINK	0x020				/*!< symbolic link */
#define	F_SOCK	0x040				/*!< socket */

    char		name[1];		/* file name (must be last) */
} NODE;

struct rpmfts_s {
    FTS * t;
    FTSENT * p;
    struct stat sb;			/*!< current stat(2) defaults. */
    int sb_is_valid;			/*!< are stat(2) defaults valid? */
    uint32_t crc_total;
    unsigned lineno;
/*@null@*/
    NODE * root;
/*@null@*/
    ARGV_t paths;
    enum mtreeKeys_e keys;
/*@null@*/
    ARGI_t algos;

/*@dependent@*/ /*@null@*/
    FILE * spec1;
/*@dependent@*/ /*@null@*/
    FILE * spec2;

/*@null@*/
    const char * fullpath;
/*@null@*/
    char * path;
    int ftsoptions;

#if defined(HAVE_STRUCT_STAT_ST_FLAGS)
    size_t maxf;
/*@null@*/
    unsigned long * f;
#endif
    size_t maxg;
/*@null@*/
    gid_t * g;
    size_t maxm;
/*@null@*/
    mode_t * m;
    size_t maxu;
/*@null@*/
    uid_t * u;

#if defined(_RPMFI_INTERNAL)
/*@null@*/
    rpmts ts;
/*@null@*/
    rpmfi fi;
#endif
};
#endif	/* _MTREE_INTERNAL */

#undef	_KFB
#undef	_MFB

#ifdef __cplusplus
extern "C" {
#endif

/*@null@*/
static NODE * mtreeSpec(rpmfts fts, /*@null@*/ FILE * fp)
	/*@globals fileSystem, internalState @*/
	/*@modifies fts, fp, fileSystem, internalState @*/;

static int mtreeVSpec(rpmfts fts)
	/*@globals fileSystem, internalState @*/
	/*@modifies fts, fileSystem, internalState @*/;

static int mtreeCWalk(rpmfts fts)
	/*@globals h_errno, fileSystem, internalState @*/
	/*@modifies fts, fileSystem, internalState @*/;

static int mtreeVWalk(rpmfts fts)
	/*@globals h_errno, fileSystem, internalState @*/
	/*@modifies fts, fileSystem, internalState @*/;

#ifdef __cplusplus
}
#endif

/*==============================================================*/

static void mtreeMiss(rpmfts fts, /*@null@*/ NODE * p, char * tail)
	/*@globals h_errno, errno, fileSystem, internalState @*/
	/*@modifies p, tail, errno, fileSystem, internalState @*/;

#include "debug.h"

/*@access DIR @*/
/*@access FD_t @*/
/*@access rpmfi @*/
/*@access rpmts @*/

#define MF_ISSET(_FLAG) ((mtreeFlags & ((MTREE_FLAGS_##_FLAG) & ~0x40000000)) != MTREE_FLAGS_NONE)

#define	KEYDEFAULT \
    (MTREE_KEYS_GID | MTREE_KEYS_MODE | MTREE_KEYS_NLINK | MTREE_KEYS_SIZE | \
     MTREE_KEYS_SLINK | MTREE_KEYS_TIME | MTREE_KEYS_UID)

#define	MISMATCHEXIT	2

#define	MBITS	(S_ISUID|S_ISGID|S_ISTXT|S_IRWXU|S_IRWXG|S_IRWXO)

/*@unchecked@*/
static struct rpmfts_s __rpmfts;
/*@unchecked@*/
static rpmfts _rpmfts = &__rpmfts;

/*@unchecked@*/
static enum mtreeFlags_e mtreeFlags = MTREE_FLAGS_NONE;

/* XXX merge into _rpmfts, use mmiRE instead */
struct exclude {
    RPM_LIST_ENTRY(exclude) link;
    const char *glob;
    int pathname;
};

/*@unchecked@*/
static RPM_LIST_HEAD(, exclude) excludes;

/*@unchecked@*/
static struct rpmop_s dc_totalops;

/*@unchecked@*/
static struct rpmop_s dc_readops;

/*@unchecked@*/
static struct rpmop_s dc_digestops;

/*==============================================================*/

/*@exits@*/
static void
mtree_error(const char *fmt, ...)
#ifdef __GNUC__
__attribute__ ((format (printf, 1, 2)))
#endif
	/*@globals fileSystem @*/
	/*@modifies fileSystem @*/;

void
mtree_error(const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    (void) fflush(NULL);
    (void) fprintf(stderr, "\n%s: ", __progname);
    (void) vfprintf(stderr, fmt, ap);
    va_end (ap);
    (void) fprintf(stderr, "\n");
    if (_rpmfts->lineno)
	(void)fprintf(stderr, _("%s: failed at line %d of the specification\n"),
		__progname, _rpmfts->lineno);
    exit(EXIT_FAILURE);
    /*@notreached@*/
}

typedef struct _key {
/*@observer@*/
	const char *name;		/* key name */
	unsigned val;			/* value */
#define	NEEDVALUE	0xffffffff
	uint32_t flags;
} KEY;

/* NB: the following table must be sorted lexically. */
/*@unchecked@*/ /*@observer@*/
static KEY keylist[] = {
    { "adler32",	MTREE_KEYS_DIGEST,	PGPHASHALGO_ADLER32 },
    { "cksum",		MTREE_KEYS_CKSUM,	NEEDVALUE },
    { "crc32",		MTREE_KEYS_DIGEST,	PGPHASHALGO_CRC32 },
    { "crc64",		MTREE_KEYS_DIGEST,	PGPHASHALGO_CRC64 },
    { "flags",		MTREE_KEYS_FLAGS,	NEEDVALUE },
    { "gid",		MTREE_KEYS_GID,		NEEDVALUE },
    { "gname",		MTREE_KEYS_GNAME,	NEEDVALUE },
    { "haval160digest",	MTREE_KEYS_DIGEST,	PGPHASHALGO_HAVAL_5_160 },
    { "ignore",		MTREE_KEYS_IGN,		0 },
    { "jlu32",		MTREE_KEYS_DIGEST,	PGPHASHALGO_JLU32 },
    { "link",		MTREE_KEYS_SLINK,	NEEDVALUE },
    { "md2digest",	MTREE_KEYS_DIGEST,	PGPHASHALGO_MD2 },
    { "md4digest",	MTREE_KEYS_DIGEST,	PGPHASHALGO_MD4 },
    { "md5digest",	MTREE_KEYS_DIGEST,	PGPHASHALGO_MD5 },
    { "mode",		MTREE_KEYS_MODE,	NEEDVALUE },
    { "nlink",		MTREE_KEYS_NLINK,	NEEDVALUE },
    { "nochange",	MTREE_KEYS_NOCHANGE,	0 },
    { "optional",	MTREE_KEYS_OPT,		0 },
    { "rmd128digest",	MTREE_KEYS_DIGEST,	PGPHASHALGO_RIPEMD128 },
    { "rmd160digest",	MTREE_KEYS_DIGEST,	PGPHASHALGO_RIPEMD160 },
    { "rmd256digest",	MTREE_KEYS_DIGEST,	PGPHASHALGO_RIPEMD256 },
    { "rmd320digest",	MTREE_KEYS_DIGEST,	PGPHASHALGO_RIPEMD320 },
    { "salsa10",	MTREE_KEYS_DIGEST,	PGPHASHALGO_SALSA10 },
    { "salsa20",	MTREE_KEYS_DIGEST,	PGPHASHALGO_SALSA20 },
    { "sha1digest",	MTREE_KEYS_DIGEST,	PGPHASHALGO_SHA1 },
    { "sha224digest",	MTREE_KEYS_DIGEST,	PGPHASHALGO_SHA224 },
    { "sha256digest",	MTREE_KEYS_DIGEST,	PGPHASHALGO_SHA256 },
    { "sha384digest",	MTREE_KEYS_DIGEST,	PGPHASHALGO_SHA384 },
    { "sha512digest",	MTREE_KEYS_DIGEST,	PGPHASHALGO_SHA512 },
    { "size",		MTREE_KEYS_SIZE,	NEEDVALUE },
    { "tiger192digest",	MTREE_KEYS_DIGEST,	PGPHASHALGO_TIGER192 },
    { "time",		MTREE_KEYS_TIME,	NEEDVALUE },
    { "type",		MTREE_KEYS_TYPE,	NEEDVALUE },
    { "uid",		MTREE_KEYS_UID,		NEEDVALUE },
    { "uname",		MTREE_KEYS_UNAME,	NEEDVALUE },
};

static int
keycompare(const void * a, const void * b)
	/*@*/
{
    return strcmp(((KEY *)a)->name, ((KEY *)b)->name);
}

static unsigned
parsekey(char *name, /*@out@*/ uint32_t *needvaluep)
	/*@globals fileSystem @*/
	/*@modifies *needvaluep, fileSystem @*/

{
    KEY *k, tmp;

    if (needvaluep != NULL)
	*needvaluep = 0;
    if (*name == '\0')
	return 0;
    tmp.name = name;
    k = (KEY *)bsearch(&tmp, keylist, sizeof(keylist) / sizeof(keylist[0]),
	    sizeof(keylist[0]), keycompare);
    if (k == NULL)
	mtree_error("unknown keyword %s", name);

    if (needvaluep != NULL)
	*needvaluep = k->flags;
    return k->val;
}

static /*@observer@*/ /*@null@*/ const char *
algo2tagname(uint32_t algo)
	/*@*/
{
    const char * tagname = NULL;

    switch (algo) {
    case PGPHASHALGO_MD5:	tagname = "md5digest";		break;
    case PGPHASHALGO_SHA1:	tagname = "sha1digest";		break;
    case PGPHASHALGO_RIPEMD160:	tagname = "rmd160digest";	break;
    case PGPHASHALGO_MD2:	tagname = "md2digest";		break;
    case PGPHASHALGO_TIGER192:	tagname = "tiger192digest";	break;
    case PGPHASHALGO_HAVAL_5_160: tagname = "haval160digest";	break;
    case PGPHASHALGO_SHA256:	tagname = "sha256digest";	break;
    case PGPHASHALGO_SHA384:	tagname = "sha384digest";	break;
    case PGPHASHALGO_SHA512:	tagname = "sha512digest";	break;
    case PGPHASHALGO_MD4:	tagname = "md4digest";		break;
    case PGPHASHALGO_RIPEMD128:	tagname = "rmd128digest";	break;
    case PGPHASHALGO_CRC32:	tagname = "crc32";		break;
    case PGPHASHALGO_ADLER32:	tagname = "adler32";		break;
    case PGPHASHALGO_CRC64:	tagname = "crc64";		break;
    case PGPHASHALGO_JLU32:	tagname = "jlu32";		break;
    case PGPHASHALGO_SHA224:	tagname = "sha224digest";	break;
    case PGPHASHALGO_RIPEMD256:	tagname = "rmd256digest";	break;
    case PGPHASHALGO_RIPEMD320:	tagname = "rmd320digest";	break;
    case PGPHASHALGO_SALSA10:	tagname = "salsa10";		break;
    case PGPHASHALGO_SALSA20:	tagname = "salsa20";		break;
    default:			tagname = NULL;			break;
    }
    return tagname;
}

#if defined(HAVE_STRUCT_STAT_ST_FLAGS)
static const char *
flags_to_string(u_long fflags)
	/*@*/
{
    char * string = fflagstostr(fflags);
    if (string != NULL && *string == '\0') {
	free(string);
	string = xstrdup("none");
    }
    return string;
}
#endif

/*==============================================================*/

/*@unchecked@*/ /*@observer@*/
static const uint32_t crctab[] = {
    0x0,
    0x04c11db7, 0x09823b6e, 0x0d4326d9, 0x130476dc, 0x17c56b6b,
    0x1a864db2, 0x1e475005, 0x2608edb8, 0x22c9f00f, 0x2f8ad6d6,
    0x2b4bcb61, 0x350c9b64, 0x31cd86d3, 0x3c8ea00a, 0x384fbdbd,
    0x4c11db70, 0x48d0c6c7, 0x4593e01e, 0x4152fda9, 0x5f15adac,
    0x5bd4b01b, 0x569796c2, 0x52568b75, 0x6a1936c8, 0x6ed82b7f,
    0x639b0da6, 0x675a1011, 0x791d4014, 0x7ddc5da3, 0x709f7b7a,
    0x745e66cd, 0x9823b6e0, 0x9ce2ab57, 0x91a18d8e, 0x95609039,
    0x8b27c03c, 0x8fe6dd8b, 0x82a5fb52, 0x8664e6e5, 0xbe2b5b58,
    0xbaea46ef, 0xb7a96036, 0xb3687d81, 0xad2f2d84, 0xa9ee3033,
    0xa4ad16ea, 0xa06c0b5d, 0xd4326d90, 0xd0f37027, 0xddb056fe,
    0xd9714b49, 0xc7361b4c, 0xc3f706fb, 0xceb42022, 0xca753d95,
    0xf23a8028, 0xf6fb9d9f, 0xfbb8bb46, 0xff79a6f1, 0xe13ef6f4,
    0xe5ffeb43, 0xe8bccd9a, 0xec7dd02d, 0x34867077, 0x30476dc0,
    0x3d044b19, 0x39c556ae, 0x278206ab, 0x23431b1c, 0x2e003dc5,
    0x2ac12072, 0x128e9dcf, 0x164f8078, 0x1b0ca6a1, 0x1fcdbb16,
    0x018aeb13, 0x054bf6a4, 0x0808d07d, 0x0cc9cdca, 0x7897ab07,
    0x7c56b6b0, 0x71159069, 0x75d48dde, 0x6b93dddb, 0x6f52c06c,
    0x6211e6b5, 0x66d0fb02, 0x5e9f46bf, 0x5a5e5b08, 0x571d7dd1,
    0x53dc6066, 0x4d9b3063, 0x495a2dd4, 0x44190b0d, 0x40d816ba,
    0xaca5c697, 0xa864db20, 0xa527fdf9, 0xa1e6e04e, 0xbfa1b04b,
    0xbb60adfc, 0xb6238b25, 0xb2e29692, 0x8aad2b2f, 0x8e6c3698,
    0x832f1041, 0x87ee0df6, 0x99a95df3, 0x9d684044, 0x902b669d,
    0x94ea7b2a, 0xe0b41de7, 0xe4750050, 0xe9362689, 0xedf73b3e,
    0xf3b06b3b, 0xf771768c, 0xfa325055, 0xfef34de2, 0xc6bcf05f,
    0xc27dede8, 0xcf3ecb31, 0xcbffd686, 0xd5b88683, 0xd1799b34,
    0xdc3abded, 0xd8fba05a, 0x690ce0ee, 0x6dcdfd59, 0x608edb80,
    0x644fc637, 0x7a089632, 0x7ec98b85, 0x738aad5c, 0x774bb0eb,
    0x4f040d56, 0x4bc510e1, 0x46863638, 0x42472b8f, 0x5c007b8a,
    0x58c1663d, 0x558240e4, 0x51435d53, 0x251d3b9e, 0x21dc2629,
    0x2c9f00f0, 0x285e1d47, 0x36194d42, 0x32d850f5, 0x3f9b762c,
    0x3b5a6b9b, 0x0315d626, 0x07d4cb91, 0x0a97ed48, 0x0e56f0ff,
    0x1011a0fa, 0x14d0bd4d, 0x19939b94, 0x1d528623, 0xf12f560e,
    0xf5ee4bb9, 0xf8ad6d60, 0xfc6c70d7, 0xe22b20d2, 0xe6ea3d65,
    0xeba91bbc, 0xef68060b, 0xd727bbb6, 0xd3e6a601, 0xdea580d8,
    0xda649d6f, 0xc423cd6a, 0xc0e2d0dd, 0xcda1f604, 0xc960ebb3,
    0xbd3e8d7e, 0xb9ff90c9, 0xb4bcb610, 0xb07daba7, 0xae3afba2,
    0xaafbe615, 0xa7b8c0cc, 0xa379dd7b, 0x9b3660c6, 0x9ff77d71,
    0x92b45ba8, 0x9675461f, 0x8832161a, 0x8cf30bad, 0x81b02d74,
    0x857130c3, 0x5d8a9099, 0x594b8d2e, 0x5408abf7, 0x50c9b640,
    0x4e8ee645, 0x4a4ffbf2, 0x470cdd2b, 0x43cdc09c, 0x7b827d21,
    0x7f436096, 0x7200464f, 0x76c15bf8, 0x68860bfd, 0x6c47164a,
    0x61043093, 0x65c52d24, 0x119b4be9, 0x155a565e, 0x18197087,
    0x1cd86d30, 0x029f3d35, 0x065e2082, 0x0b1d065b, 0x0fdc1bec,
    0x3793a651, 0x3352bbe6, 0x3e119d3f, 0x3ad08088, 0x2497d08d,
    0x2056cd3a, 0x2d15ebe3, 0x29d4f654, 0xc5a92679, 0xc1683bce,
    0xcc2b1d17, 0xc8ea00a0, 0xd6ad50a5, 0xd26c4d12, 0xdf2f6bcb,
    0xdbee767c, 0xe3a1cbc1, 0xe760d676, 0xea23f0af, 0xeee2ed18,
    0xf0a5bd1d, 0xf464a0aa, 0xf9278673, 0xfde69bc4, 0x89b8fd09,
    0x8d79e0be, 0x803ac667, 0x84fbdbd0, 0x9abc8bd5, 0x9e7d9662,
    0x933eb0bb, 0x97ffad0c, 0xafb010b1, 0xab710d06, 0xa6322bdf,
    0xa2f33668, 0xbcb4666d, 0xb8757bda, 0xb5365d03, 0xb1f740b4
};

/*
 * Compute a POSIX 1003.2 checksum.  This routine has been broken out so that
 * other programs can use it.  It takes a file descriptor to read from and
 * locations to store the crc and the number of bytes read.  It returns 0 on
 * success and 1 on failure.  Errno is set on failure.
 */
static int
crc(FD_t fd, /*@out@*/ uint32_t * cval, /*@out@*/ uint32_t * clen)
	/*@globals _rpmfts, fileSystem @*/
	/*@modifies fd, *clen, *cval, _rpmfts, fileSystem @*/
{
    uint32_t crc = 0;
    uint32_t len = 0;

#define	COMPUTE(var, ch)	(var) = (var) << 8 ^ crctab[(var) >> 24 ^ (ch)]

    _rpmfts->crc_total ^= 0xffffffff;

    {   uint8_t buf[16 * 1024];
	size_t nr;
	while ((nr = Fread(buf, sizeof(buf[0]), sizeof(buf), fd)) != 0) {
	    uint8_t *p;
	    for (len += nr, p = buf; nr--; ++p) {
		COMPUTE(crc, *p);
		COMPUTE(_rpmfts->crc_total, *p);
	    }
	}
	if (Ferror(fd))
	    return 1;
    }

    *clen = len;

    /* Include the length of the file. */
    for (; len != 0; len >>= 8) {
	COMPUTE(crc, len & 0xff);
	COMPUTE(_rpmfts->crc_total, len & 0xff);
    }

    *cval = (crc ^ 0xffffffff);
    _rpmfts->crc_total ^= 0xffffffff;
    return 0;
}

/*==============================================================*/

/*
 * to select alternate encoding format
 */
#define	VIS_OCTAL	0x01	/* use octal \ddd format */
#define	VIS_CSTYLE	0x02	/* use \[nrft0..] where appropriate */

/*
 * to alter set of characters encoded (default is to encode all
 * non-graphic except space, tab, and newline).
 */
#define	VIS_SP		0x04	/* also encode space */
#define	VIS_TAB		0x08	/* also encode tab */
#define	VIS_NL		0x10	/* also encode newline */
#define	VIS_WHITE	(VIS_SP | VIS_TAB | VIS_NL)
#define	VIS_SAFE	0x20	/* only encode "unsafe" characters */

/*
 * other
 */
#define	VIS_NOSLASH	0x40	/* inhibit printing '\' */

/*
 * unvis return codes
 */
#define	UNVIS_VALID	 1	/* character valid */
#define	UNVIS_VALIDPUSH	 2	/* character valid, push back passed char */
#define	UNVIS_NOCHAR	 3	/* valid sequence, no character produced */
#define	UNVIS_SYNBAD	-1	/* unrecognized escape sequence */
#define	UNVIS_ERROR	-2	/* decoder in unknown state (unrecoverable) */

/*
 * unvis flags
 */
#define	UNVIS_END	1	/* no more characters */

static char *vis(/*@returned@*/ /*@out@*/ char *dst, int c, int flag, int nextc)
	/*@modifies dst @*/;
static int strvis(/*@out@*/ char *dst, const char *src, int flag)
	/*@modifies dst @*/;
#ifdef	NOTUSED
static int strnvis(/*@out@*/ char *dst, const char *src, size_t siz, int flag)
	/*@modifies dst @*/;
static int strvisx(/*@out@*/ char *dst, const char *src, size_t len, int flag)
	/*@modifies dst @*/;
#endif
static int strunvis(/*@out@*/ char *dst, const char *src)
	/*@modifies dst @*/;
static int unvis(/*@out@*/ char *cp, char c, int *astate, int flag)
	/*@modifies cp, astate @*/;

#define	isoctal(c) (((unsigned char)(c)) >= '0' && ((unsigned char)(c)) <= '7')
#define isvisible(c)	\
  (((unsigned)(c) <= (unsigned)UCHAR_MAX && isascii((unsigned char)(c)) && \
	isgraph((unsigned char)(c)))			\
   ||	((flag & VIS_SP) == 0 && (c) == (int)' ')	\
   ||	((flag & VIS_TAB) == 0 && (c) == (int)'\t')	\
   ||	((flag & VIS_NL) == 0 && (c) == (int)'\n')	\
   ||	((flag & VIS_SAFE) \
	 && ((c) == (int)'\b' || (c) == (int)'\007' || (c) == (int)'\r')))

/*
 * vis - visually encode characters
 */
char *
vis(char * dst, int c, int flag, int nextc)
{
    if (isvisible(c)) {
	*dst++ = (char)c;
	if (c == (int)'\\' && (flag & VIS_NOSLASH) == 0)
	    *dst++ = '\\';
	*dst = '\0';
	return dst;
    }

    if (flag & VIS_CSTYLE) {
	switch(c) {
	case '\n':
	    *dst++ = '\\';
	    *dst++ = 'n';
	    goto done;
	case '\r':
	    *dst++ = '\\';
	    *dst++ = 'r';
	    goto done;
	case '\b':
	    *dst++ = '\\';
	    *dst++ = 'b';
	    goto done;
	case '\a':
	    *dst++ = '\\';
	    *dst++ = 'a';
	    goto done;
	case '\v':
	    *dst++ = '\\';
	    *dst++ = 'v';
	    goto done;
	case '\t':
	    *dst++ = '\\';
	    *dst++ = 't';
	    goto done;
	case '\f':
	    *dst++ = '\\';
	    *dst++ = 'f';
	    goto done;
	case ' ':
	    *dst++ = '\\';
	    *dst++ = 's';
	    goto done;
	case '\0':
	    *dst++ = '\\';
	    *dst++ = '0';
	    if (isoctal(nextc)) {
		*dst++ = '0';
		*dst++ = '0';
	    }
	    goto done;
	}
    }
    if (((c & 0177) == (int)' ') || (flag & VIS_OCTAL)) {	
	*dst++ = '\\';
	*dst++ = ((unsigned char)c >> 6 & 07) + '0';
	*dst++ = ((unsigned char)c >> 3 & 07) + '0';
	*dst++ = ((unsigned char)c & 07) + '0';
	goto done;
    }
    if ((flag & VIS_NOSLASH) == 0)
	*dst++ = '\\';
    if (c & 0200) {
	c &= 0177;
	*dst++ = 'M';
    }
    if (iscntrl(c)) {
	*dst++ = '^';
	if (c == 0177)
	    *dst++ = '?';
	else
	    *dst++ = (char)(c + (int)'@');
    } else {
	*dst++ = '-';
	*dst++ = (char)c;
    }

done:
    *dst = '\0';
    return dst;
}

/*
 * strvis, strnvis, strvisx - visually encode characters from src into dst
 *	
 *	Dst must be 4 times the size of src to account for possible
 *	expansion.  The length of dst, not including the trailing NULL,
 *	is returned.
 *
 *	Strnvis will write no more than siz-1 bytes (and will NULL terminate).
 *	The number of bytes needed to fully encode the string is returned.
 *
 *	Strvisx encodes exactly len bytes from src into dst.
 *	This is useful for encoding a block of data.
 */
int
strvis(char * dst, const char * src, int flag)
{
    char c;
    char *start;

    for (start = dst; (c = *src) != '\0';)
	dst = vis(dst, (int)c, flag, (int)*++src);
    *dst = '\0';
    return (dst - start);
}

#ifdef	NOTUSED
int
strnvis(char * dst, const char * src, size_t siz, int flag)
{
    char c;
    char *start, *end;

    for (start = dst, end = start + siz - 1; (c = *src) && dst < end; ) {
	if (isvisible((int)c)) {
	    *dst++ = c;
	    if (c == '\\' && (flag & VIS_NOSLASH) == 0) {
		/* need space for the extra '\\' */
		if (dst < end)
		    *dst++ = '\\';
		else {
		    dst--;
		    break;
		}
	    }
	    src++;
	} else {
	    /* vis(3) requires up to 4 chars */
	    if (dst + 3 < end)
		dst = vis(dst, (int)c, flag, (int)*++src);
	    else
		break;
	}
    }
    *dst = '\0';
    if (dst >= end) {
	char tbuf[5];

	/* adjust return value for truncation */
	while ((c = *src) != '\0')
	    dst += vis(tbuf, (int)c, flag, (int)*++src) - tbuf;
    }
    return (dst - start);
}
#endif	/* NOTUSED */

#ifdef	NOTUSED
int
strvisx(char * dst, const char * src, size_t len, int flag)
{
    char c;
    char *start;

    for (start = dst; len > 1; len--) {
	c = *src;
	dst = vis(dst, (int)c, flag, (int)*++src);
    }
    if (len)
	dst = vis(dst, (int)*src, flag, (int)'\0');
    *dst = '\0';
    return (dst - start);
}
#endif	/* NOTUSED */

/*
 * decode driven by state machine
 */
#define	S_GROUND	0	/* haven't seen escape char */
#define	S_START		1	/* start decoding special sequence */
#define	S_META		2	/* metachar started (M) */
#define	S_META1		3	/* metachar more, regular char (-) */
#define	S_CTRL		4	/* control char started (^) */
#define	S_OCTAL2	5	/* octal digit 2 */
#define	S_OCTAL3	6	/* octal digit 3 */

#if !defined(isoctal)
#define	isoctal(c) (((unsigned char)(c)) >= '0' && ((unsigned char)(c)) <= '7')
#endif

/*
 * unvis - decode characters previously encoded by vis
 */
int
unvis(char *cp, char c, int *astate, int flag)
{

    if (flag & UNVIS_END) {
	if (*astate == S_OCTAL2 || *astate == S_OCTAL3) {
	    *astate = S_GROUND;
	    return (UNVIS_VALID);
	}
	return (*astate == S_GROUND ? UNVIS_NOCHAR : UNVIS_SYNBAD);
    }

    switch (*astate) {

    case S_GROUND:
	*cp = '\0';
	if (c == '\\') {
	    *astate = S_START;
	    return (0);
	}
	*cp = c;
	return (UNVIS_VALID);

    case S_START:
	switch(c) {
	case '\\':
	    *cp = c;
	    *astate = S_GROUND;
	    return (UNVIS_VALID);
	case '0': case '1': case '2': case '3':
	case '4': case '5': case '6': case '7':
	    *cp = (c - '0');
	    *astate = S_OCTAL2;
	    return (0);
	case 'M':
	    *cp = (char) 0200;
	    *astate = S_META;
	    return (0);
	case '^':
	    *astate = S_CTRL;
	    return (0);
	case 'n':
	    *cp = '\n';
	    *astate = S_GROUND;
	    return (UNVIS_VALID);
	case 'r':
	    *cp = '\r';
	    *astate = S_GROUND;
	    return (UNVIS_VALID);
	case 'b':
	    *cp = '\b';
	    *astate = S_GROUND;
	    return (UNVIS_VALID);
	case 'a':
	    *cp = '\007';
	    *astate = S_GROUND;
	    return (UNVIS_VALID);
	case 'v':
	    *cp = '\v';
	    *astate = S_GROUND;
	    return (UNVIS_VALID);
	case 't':
	    *cp = '\t';
	    *astate = S_GROUND;
	    return (UNVIS_VALID);
	case 'f':
	    *cp = '\f';
	    *astate = S_GROUND;
	    return (UNVIS_VALID);
	case 's':
	    *cp = ' ';
	    *astate = S_GROUND;
	    return (UNVIS_VALID);
	case 'E':
	    *cp = '\033';
	    *astate = S_GROUND;
	    return (UNVIS_VALID);
	case '\n':	/* hidden newline */
	    *astate = S_GROUND;
	    return (UNVIS_NOCHAR);
	case '$':	/* hidden marker */
	    *astate = S_GROUND;
	    return (UNVIS_NOCHAR);
	}
	*astate = S_GROUND;
	return (UNVIS_SYNBAD);

    case S_META:
	if (c == '-')
	    *astate = S_META1;
	else if (c == '^')
	    *astate = S_CTRL;
	else {
	    *astate = S_GROUND;
	    return (UNVIS_SYNBAD);
	}
	return (0);

    case S_META1:
	*astate = S_GROUND;
	*cp |= c;
	return (UNVIS_VALID);

    case S_CTRL:
	if (c == '?')
	    *cp |= 0177;
	else
	    *cp |= c & 037;
	*astate = S_GROUND;
	return (UNVIS_VALID);

    case S_OCTAL2:	/* second possible octal digit */
	if (isoctal(c)) {
	    /*
	     * yes - and maybe a third
	     */
	    *cp = (*cp << 3) + (c - '0');
	    *astate = S_OCTAL3;	
	    return (0);
	}
	/*
	 * no - done with current sequence, push back passed char
	 */
	*astate = S_GROUND;
	return (UNVIS_VALIDPUSH);

    case S_OCTAL3:	/* third possible octal digit */
	*astate = S_GROUND;
	if (isoctal(c)) {
	    *cp = (*cp << 3) + (c - '0');
	    return (UNVIS_VALID);
	}
	/*
	 * we were done, push back passed char
	 */
	return (UNVIS_VALIDPUSH);
			
    default:	
	/*
	 * decoder in unknown state - (probably uninitialized)
	 */
	*astate = S_GROUND;
	return (UNVIS_SYNBAD);
    }
}

/*
 * strunvis - decode src into dst
 *
 *	Number of chars decoded into dst is returned, -1 on error.
 *	Dst is null terminated.
 */

int
strunvis(char * dst, const char * src)
{
    char c;
    char *start = dst;
    int state = 0;

    while ((c = *src++) != '\0') {
    again:
	switch (unvis(dst, c, &state, 0)) {
	case UNVIS_VALID:
	    dst++;
	    /*@switchbreak@*/ break;
	case UNVIS_VALIDPUSH:
	    dst++;
	    goto again;
	case 0:
	case UNVIS_NOCHAR:
	    /*@switchbreak@*/ break;
	default:
	    return (-1);
	}
    }
    if (unvis(dst, c, &state, UNVIS_END) == UNVIS_VALID)
	dst++;
    *dst = '\0';
    return (dst - start);
}

/*==============================================================*/

/* XXX *BSD systems already have getmode(3) and setmode(3) */
#if defined(__linux__) || defined(__sun) || defined(__LCLINT__) || defined(__QNXNTO__)
#if !defined(HAVE_GETMODE) || !defined(HAVE_SETMODE)

#define	SET_LEN	6		/* initial # of bitcmd struct to malloc */
#define	SET_LEN_INCR 4		/* # of bitcmd structs to add as needed */

typedef struct bitcmd {
    char	cmd;
    char	cmd2;
    mode_t	bits;
} BITCMD;

#define	CMD2_CLR	0x01
#define	CMD2_SET	0x02
#define	CMD2_GBITS	0x04
#define	CMD2_OBITS	0x08
#define	CMD2_UBITS	0x10

#if !defined(HAVE_GETMODE)
/*
 * Given the old mode and an array of bitcmd structures, apply the operations
 * described in the bitcmd structures to the old mode, and return the new mode.
 * Note that there is no '=' command; a strict assignment is just a '-' (clear
 * bits) followed by a '+' (set bits).
 */
static mode_t
getmode(const void * bbox, mode_t omode)
	/*@*/
{
    const BITCMD *set;
    mode_t clrval, newmode, value;

    set = (const BITCMD *)bbox;
    newmode = omode;
    for (value = 0;; set++)
	switch(set->cmd) {
	/*
	 * When copying the user, group or other bits around, we "know"
	 * where the bits are in the mode so that we can do shifts to
	 * copy them around.  If we don't use shifts, it gets real
	 * grundgy with lots of single bit checks and bit sets.
	 */
	case 'u':
	    value = (newmode & S_IRWXU) >> 6;
	    goto common;

	case 'g':
	    value = (newmode & S_IRWXG) >> 3;
	    goto common;

	case 'o':
	    value = newmode & S_IRWXO;
common:	    if ((set->cmd2 & CMD2_CLR) != '\0') {
		clrval = (set->cmd2 & CMD2_SET) != '\0' ?  S_IRWXO : value;
		if ((set->cmd2 & CMD2_UBITS) != '\0')
		    newmode &= ~((clrval<<6) & set->bits);
		if ((set->cmd2 & CMD2_GBITS) != '\0')
		    newmode &= ~((clrval<<3) & set->bits);
		if ((set->cmd2 & CMD2_OBITS) != '\0')
		    newmode &= ~(clrval & set->bits);
	    }
	    if ((set->cmd2 & CMD2_SET) != '\0') {
		if ((set->cmd2 & CMD2_UBITS) != '\0')
		    newmode |= (value<<6) & set->bits;
		if ((set->cmd2 & CMD2_GBITS) != '\0')
		    newmode |= (value<<3) & set->bits;
		if ((set->cmd2 & CMD2_OBITS) != '\0')
		    newmode |= value & set->bits;
	    }
	    /*@switchbreak@*/ break;

	case '+':
	    newmode |= set->bits;
	    /*@switchbreak@*/ break;

	case '-':
	    newmode &= ~set->bits;
	    /*@switchbreak@*/ break;

	case 'X':
	    if (omode & (S_IFDIR|S_IXUSR|S_IXGRP|S_IXOTH))
			newmode |= set->bits;
	    /*@switchbreak@*/ break;

	case '\0':
	default:
#ifdef SETMODE_DEBUG
	    (void) printf("getmode:%04o -> %04o\n", omode, newmode);
#endif
	    return newmode;
	}
}
#endif	/* !defined(HAVE_GETMODE) */

#if !defined(HAVE_SETMODE)
#ifdef SETMODE_DEBUG
static void
dumpmode(BITCMD *set)
{
    for (; set->cmd; ++set)
	(void) printf("cmd: '%c' bits %04o%s%s%s%s%s%s\n",
	    set->cmd, set->bits, set->cmd2 ? " cmd2:" : "",
	    set->cmd2 & CMD2_CLR ? " CLR" : "",
	    set->cmd2 & CMD2_SET ? " SET" : "",
	    set->cmd2 & CMD2_UBITS ? " UBITS" : "",
	    set->cmd2 & CMD2_GBITS ? " GBITS" : "",
	    set->cmd2 & CMD2_OBITS ? " OBITS" : "");
}
#endif

#define	ADDCMD(a, b, c, d)					\
    if (set >= endset) {					\
	BITCMD *newset;						\
	setlen += SET_LEN_INCR;					\
	newset = realloc(saveset, sizeof(*newset) * setlen);	\
	if (newset == NULL) {					\
		if (saveset != NULL)				\
			free(saveset);				\
		saveset = NULL;					\
		return (NULL);					\
	}							\
	set = newset + (set - saveset);				\
	saveset = newset;					\
	endset = newset + (setlen - 2);				\
    }								\
    set = addcmd(set, (a), (b), (c), (d))

#define	STANDARD_BITS	(S_ISUID|S_ISGID|S_IRWXU|S_IRWXG|S_IRWXO)

static BITCMD *
addcmd(/*@returned@*/ BITCMD *set, int op, int who, int oparg, unsigned mask)
	/*@modifies set @*/
{
    switch (op) {
    case '=':
	set->cmd = '-';
	set->bits = who ? who : (int) STANDARD_BITS;
	set++;

	op = (int)'+';
	/*@fallthrough@*/
    case '+':
    case '-':
    case 'X':
	set->cmd = (char)op;
	set->bits = (who ? (unsigned)who : mask) & oparg;
	break;

    case 'u':
    case 'g':
    case 'o':
	set->cmd = (char)op;
	if (who) {
	    set->cmd2 = (char)( ((who & S_IRUSR) ? CMD2_UBITS : 0) |
				((who & S_IRGRP) ? CMD2_GBITS : 0) |
				((who & S_IROTH) ? CMD2_OBITS : 0));
	    set->bits = (mode_t)~0;
	} else {
	    set->cmd2 =(char)(CMD2_UBITS | CMD2_GBITS | CMD2_OBITS);
	    set->bits = mask;
	}
	
	if (oparg == (int)'+')
	    set->cmd2 |= CMD2_SET;
	else if (oparg == (int)'-')
	    set->cmd2 |= CMD2_CLR;
	else if (oparg == (int)'=')
	    set->cmd2 |= CMD2_SET|CMD2_CLR;
	break;
    }
    return set + 1;
}

/*
 * Given an array of bitcmd structures, compress by compacting consecutive
 * '+', '-' and 'X' commands into at most 3 commands, one of each.  The 'u',
 * 'g' and 'o' commands continue to be separate.  They could probably be
 * compacted, but it's not worth the effort.
 */
static void
compress_mode(/*@out@*/ BITCMD *set)
	/*@modifies set @*/
{
    BITCMD *nset;
    int setbits, clrbits, Xbits, op;

    for (nset = set;;) {
	/* Copy over any 'u', 'g' and 'o' commands. */
	while ((op = (int)nset->cmd) != (int)'+' && op != (int)'-' && op != (int)'X') {
	    *set++ = *nset++;
	    if (!op)
		return;
	}

	for (setbits = clrbits = Xbits = 0;; nset++) {
	    if ((op = (int)nset->cmd) == (int)'-') {
		clrbits |= nset->bits;
		setbits &= ~nset->bits;
		Xbits &= ~nset->bits;
	    } else if (op == (int)'+') {
		setbits |= nset->bits;
		clrbits &= ~nset->bits;
		Xbits &= ~nset->bits;
	    } else if (op == (int)'X')
		Xbits |= nset->bits & ~setbits;
	    else
		/*@innerbreak@*/ break;
	}
	if (clrbits) {
	    set->cmd = '-';
	    set->cmd2 = '\0';
	    set->bits = clrbits;
	    set++;
	}
	if (setbits) {
	    set->cmd = '+';
	    set->cmd2 = '\0';
	    set->bits = setbits;
	    set++;
	}
	if (Xbits) {
	    set->cmd = 'X';
	    set->cmd2 = '\0';
	    set->bits = Xbits;
	    set++;
	}
    }
}

/*@-usereleased@*/
/*@null@*/
static void *
setmode(const char * p)
	/*@globals fileSystem @*/
	/*@modifies fileSystem @*/
{
    int perm, who;
    char op;
    BITCMD *set, *saveset, *endset;
    sigset_t sigset, sigoset;
    mode_t mask;
    int equalopdone = 0;
    int permXbits, setlen;
    long perml;

    if (!*p)
	return (NULL);

    /*
     * Get a copy of the mask for the permissions that are mask relative.
     * Flip the bits, we want what's not set.  Since it's possible that
     * the caller is opening files inside a signal handler, protect them
     * as best we can.
     */
    (void) sigfillset(&sigset);
    (void) sigprocmask(SIG_BLOCK, &sigset, &sigoset);
    (void) umask(mask = umask(0));
    mask = ~mask;
    (void) sigprocmask(SIG_SETMASK, &sigoset, NULL);

    setlen = SET_LEN + 2;
	
    if ((set = malloc((unsigned)(sizeof(*set) * setlen))) == NULL)
	return (NULL);
    saveset = set;
    endset = set + (setlen - 2);

    /*
     * If an absolute number, get it and return; disallow non-octal digits
     * or illegal bits.
     */
    if (isdigit(*p)) {
	perml = strtol(p, NULL, 8);
/*@-unrecog@*/
	if (perml < 0 || (perml & ~(STANDARD_BITS|S_ISTXT)))
/*@=unrecog@*/
	{
	    free(saveset);
	    return (NULL);
	}
	perm = (int)(mode_t)perml;
	while (*++p != '\0')
	    if (*p < '0' || *p > '7') {
		free(saveset);
		return (NULL);
	    }
	ADDCMD((int)'=', (int)(STANDARD_BITS|S_ISTXT), perm, (unsigned)mask);
	return (saveset);
    }

    /*
     * Build list of structures to set/clear/copy bits as described by
     * each clause of the symbolic mode.
     */
    for (;;) {
	/* First, find out which bits might be modified. */
	for (who = 0;; ++p) {
	    switch (*p) {
	    case 'a':
		who |= STANDARD_BITS;
		/*@switchbreak@*/ break;
	    case 'u':
		who |= S_ISUID|S_IRWXU;
		/*@switchbreak@*/ break;
	    case 'g':
		who |= S_ISGID|S_IRWXG;
		/*@switchbreak@*/ break;
	    case 'o':
		who |= S_IRWXO;
		/*@switchbreak@*/ break;
	    default:
		goto getop;
	    }
	}

getop:	if ((op = *p++) != '+' && op != '-' && op != '=') {
	    free(saveset);
	    return (NULL);
	}
	if (op == '=')
	    equalopdone = 0;

	who &= ~S_ISTXT;
	for (perm = 0, permXbits = 0;; ++p) {
	    switch (*p) {
	    case 'r':
		perm |= S_IRUSR|S_IRGRP|S_IROTH;
		/*@switchbreak@*/ break;
	    case 's':
		/*
		 * If specific bits where requested and
		 * only "other" bits ignore set-id.
		 */
		if (who == 0 || (who & ~S_IRWXO))
		    perm |= S_ISUID|S_ISGID;
		/*@switchbreak@*/ break;
	    case 't':
		/*
		 * If specific bits where requested and
		 * only "other" bits ignore sticky.
		 */
		if (who == 0 || (who & ~S_IRWXO)) {
		    who |= S_ISTXT;
		    perm |= S_ISTXT;
		}
		/*@switchbreak@*/ break;
	    case 'w':
		perm |= S_IWUSR|S_IWGRP|S_IWOTH;
		/*@switchbreak@*/ break;
	    case 'X':
		permXbits = (int)(S_IXUSR|S_IXGRP|S_IXOTH);
		/*@switchbreak@*/ break;
	    case 'x':
		perm |= S_IXUSR|S_IXGRP|S_IXOTH;
		/*@switchbreak@*/ break;
	    case 'u':
	    case 'g':
	    case 'o':
		/*
		 * When ever we hit 'u', 'g', or 'o', we have
		 * to flush out any partial mode that we have,
		 * and then do the copying of the mode bits.
		 */
		if (perm) {
		    ADDCMD((int)op, who, perm, (unsigned)mask);
		    perm = 0;
		}
		if (op == '=')
		    equalopdone = 1;
		if (op == '+' && permXbits) {
		    ADDCMD((int)'X', who, permXbits, (unsigned)mask);
		    permXbits = 0;
		}
		ADDCMD((int)*p, who, (int)op, (unsigned)mask);
		/*@switchbreak@*/ break;

	    default:
		/*
		 * Add any permissions that we haven't already
		 * done.
		 */
		if (perm || (op == '=' && !equalopdone)) {
		    if (op == '=')
			equalopdone = 1;
		    ADDCMD((int)op, who, perm, (unsigned)mask);
		    perm = 0;
		}
		if (permXbits) {
		    ADDCMD((int)'X', who, permXbits, (unsigned)mask);
		    permXbits = 0;
		}
		goto apply;
	    }
	}

apply:	if (!*p)
	    break;
	if (*p != ',')
	    goto getop;
	++p;
    }
    set->cmd = '\0';
#ifdef SETMODE_DEBUG
    (void) printf("Before compress_mode()\n");
    dumpmode(saveset);
#endif
    compress_mode(saveset);
#ifdef SETMODE_DEBUG
    (void) printf("After compress_mode()\n");
    dumpmode(saveset);
#endif
    return saveset;
}
/*@=usereleased@*/
#endif	/* !defined(HAVE_SETMODE) */
#endif	/* !defined(HAVE_GETMODE) || !defined(HAVE_SETMODE) */
#endif	/* __linux__ */

/*==============================================================*/

static void
set(char * t, NODE * ip)
	/*@globals fileSystem, internalState @*/
	/*@modifies t, ip, fileSystem, internalState @*/
{
    char *kw;

    for (; (kw = strtok(t, "= \t\n")) != NULL; t = NULL) {
	uint32_t needvalue;
	enum mtreeKeys_e type = parsekey(kw, &needvalue);
	char *val = NULL;
	char *ep;

	if (needvalue && (val = strtok(NULL, " \t\n")) == NULL)
		mtree_error("missing value");
	ip->flags |= type;
	switch(type) {
	case MTREE_KEYS_CKSUM:
	    ip->cksum = strtoul(val, &ep, 10);
	    if (*ep != '\0')
		mtree_error("invalid checksum %s", val);
	    /*@switchbreak@*/ break;
	case MTREE_KEYS_FLAGS:
#if defined(HAVE_STRUCT_STAT_ST_FLAGS)
	    if (!strcmp(val, "none")) {
		ip->sb.st_flags = 0;
		/*@switchbreak@*/ break;
	    }
	    {	unsigned long fset, fclr;
		if (strtofflags(&val, &fset, &fclr))
		    mtree_error("%s", strerror(errno));
		ip->sb.st_flags = fset;
	    }
#endif
	    /*@switchbreak@*/ break;
	case MTREE_KEYS_GID:
	    ip->sb.st_gid = strtoul(val, &ep, 10);
	    if (*ep != '\0')
		mtree_error("invalid gid %s", val);
	    /*@switchbreak@*/ break;
	case MTREE_KEYS_GNAME:
	    if (gnameToGid(val, &ip->sb.st_gid) == -1)
	 	mtree_error("unknown group %s", val);
	    /*@switchbreak@*/ break;
	case MTREE_KEYS_IGN:
	    /* just set flag bit */
	    /*@switchbreak@*/ break;
	case MTREE_KEYS_MODE:
	{   mode_t *m;
	    if ((m = setmode(val)) == NULL)
		mtree_error("invalid file mode %s", val);
	    ip->sb.st_mode = getmode(m, 0);
	    free(m);
	}   /*@switchbreak@*/ break;
	case MTREE_KEYS_NLINK:
	    ip->sb.st_nlink = strtoul(val, &ep, 10);
	    if (*ep != '\0')
		mtree_error("invalid link count %s", val);
	    /*@switchbreak@*/ break;
	case MTREE_KEYS_DIGEST:
	    (void) argiAdd(&ip->algos, -1, (int)needvalue);
	    (void) argvAdd(&ip->digests, val);
	    /*@switchbreak@*/ break;
	case MTREE_KEYS_SIZE:
/*@-unrecog@*/
	    ip->sb.st_size = strtoul(val, &ep, 10);
/*@=unrecog@*/
	    if (*ep != '\0')
		mtree_error("invalid size %s", val);
	    /*@switchbreak@*/ break;
	case MTREE_KEYS_SLINK:
	    ip->slink = xmalloc(strlen(val) + 1);
	    if (strunvis(ip->slink, val) == -1) {
		fprintf(stderr, _("%s: filename (%s) encoded incorrectly\n"),
			__progname, val);
		/* XXX Mac OS X exits here. */
		strcpy(ip->slink, val);
	    }
	    /*@switchbreak@*/ break;
	case MTREE_KEYS_TIME:
#if defined(TIMEVAL_TO_TIMESPEC)
	    ip->sb.st_mtimespec.tv_sec = strtoul(val, &ep, 10);
	    if (*ep != '.')
		mtree_error("invalid time %s", val);
	    val = ep + 1;
	    ip->sb.st_mtimespec.tv_nsec = strtoul(val, &ep, 10);
	    if (*ep != '\0')
		mtree_error("invalid time %s", val);
#else
	    ip->sb.st_mtime = strtoul(val, &ep, 10);
	    if (*ep != '.')
		mtree_error("invalid time %s", val);
	    val = ep + 1;
	    (void) strtoul(val, &ep, 10);
	    if (*ep != '\0')
		mtree_error("invalid time %s", val);
#endif
	    /*@switchbreak@*/ break;
	case MTREE_KEYS_TYPE:
	    switch(*val) {
	    case 'b':
		if (!strcmp(val, "block"))
		    ip->type = F_BLOCK;
		/*@innerbreak@*/ break;
	    case 'c':
		if (!strcmp(val, "char"))
		    ip->type = F_CHAR;
		/*@innerbreak@*/ break;
	    case 'd':
		if (!strcmp(val, "dir"))
		    ip->type = F_DIR;
		/*@innerbreak@*/ break;
	    case 'f':
		if (!strcmp(val, "file"))
		    ip->type = F_FILE;
		if (!strcmp(val, "fifo"))
		    ip->type = F_FIFO;
		/*@innerbreak@*/ break;
	    case 'l':
		if (!strcmp(val, "link"))
		    ip->type = F_LINK;
		/*@innerbreak@*/ break;
	    case 's':
		if (!strcmp(val, "socket"))
		    ip->type = F_SOCK;
		/*@innerbreak@*/ break;
	    default:
		mtree_error("unknown file type %s", val);
	    }
	    /*@switchbreak@*/ break;
	case MTREE_KEYS_UID:
	    ip->sb.st_uid = strtoul(val, &ep, 10);
	    if (*ep != '\0')
		mtree_error("invalid uid %s", val);
	    /*@switchbreak@*/ break;
	case MTREE_KEYS_UNAME:
	    if (unameToUid(val, &ip->sb.st_uid) == -1)
	 	mtree_error("unknown user %s", val);
	    /*@switchbreak@*/ break;
	case MTREE_KEYS_NONE:
	case MTREE_KEYS_DONE:
	case MTREE_KEYS_MAGIC:
	case MTREE_KEYS_VISIT:
	case MTREE_KEYS_NOCHANGE:
	case MTREE_KEYS_OPT:
	    ip->flags &= ~type;		/* XXX clean up "can't happen" cruft? */
	    /*@notreached@*/ /*@switchbreak@*/ break;
	}
    }
}

static void
unset(char * t, NODE * ip)
	/*@globals fileSystem, internalState @*/
	/*@modifies t, ip, fileSystem, internalState @*/
{
    char *p;

    while ((p = strtok(t, "\n\t ")) != NULL)
	ip->flags &= ~parsekey(p, NULL);
}

#define KF_ISSET(_keys, _KEY) ((_keys) & (MTREE_KEYS_##_KEY))

/* XXX todo: only fts->lineo used. lightweight struct {fn,fp,lineno} instead. */
NODE *
mtreeSpec(rpmfts fts, FILE * fp)
{
    NODE *centry = NULL;
    NODE *last = NULL;
    char *p;
    NODE ginfo;
    NODE *root = NULL;
    NODE *forest = NULL;
    int c_cur = 0;
    int c_next = 0;
    char buf[2048];

    if (fp == NULL)
	fp = stdin;

    memset(&ginfo, 0, sizeof(ginfo));
    for (fts->lineno = 1; fgets(buf, (int)sizeof(buf), fp) != NULL;
	    ++fts->lineno, c_cur = c_next, c_next = 0)
    {
	/* Skip empty lines. */
	if (buf[0] == '\n')
	    continue;

	/* Find end of line. */
	if ((p = strchr(buf, '\n')) == NULL)
	    mtree_error("line %d too long", fts->lineno);

	/* See if next line is continuation line. */
	if (p[-1] == '\\') {
	    --p;
	    c_next = 1;
	}

	/* Null-terminate the line. */
	*p = '\0';

	/* Skip leading whitespace. */
	for (p = buf; *p && isspace(*p); ++p);

	/* If nothing but whitespace or comment char, continue. */
	if (*p == '\0' || *p == '#')
	    continue;

#ifdef DEBUG
	(void)fprintf(stderr, "line %3d: {%s}\n", fts->lineno, p);
#endif
	if (c_cur) {
assert(centry);		/* XXX coverity #1035809 */
	    set(p, centry);
	    continue;
	}
			
	/* Grab file name, "$", "set", or "unset". */
	if ((p = strtok(p, "\n\t ")) == NULL)
	    mtree_error("missing field");

	if (p[0] == '/')
	    switch(p[1]) {
	    case 's':
		if (strcmp(p + 1, "set"))
		    /*@switchbreak@*/ break;
		set(NULL, &ginfo);
		continue;
	    case 'u':
		if (strcmp(p + 1, "unset"))
		    /*@switchbreak@*/ break;
		unset(NULL, &ginfo);
		continue;
	    }

#if !defined(_RPMFI_INTERNAL)	/* XXX *.rpm/ specs include '/' in names. */
	if (strchr(p, '/') != NULL)
	    mtree_error("slash character in file name");
#endif

	if (!strcmp(p, "..")) {
	    /* Don't go up, if haven't gone down. */
	    if (root == NULL)
		goto noparent;
	    if (last->type != F_DIR || KF_ISSET(last->flags, DONE)) {
		if (last == root)
		    goto noparent;
		last = last->parent;
	    }
	    last->flags |= MTREE_KEYS_DONE;
	    continue;

noparent:    mtree_error("no parent node");
	}

	/* XXX sizeof(*centry) includes room for final '\0' */
	centry = xcalloc(1, sizeof(*centry) + strlen(p));
	*centry = ginfo;	/* structure assignment */
#define	MAGIC	"?*["
	if (strpbrk(p, MAGIC) != NULL)
	    centry->flags |= MTREE_KEYS_MAGIC;
	if (strunvis(centry->name, p) == -1) {
	    fprintf(stderr, _("%s: filename (%s) encoded incorrectly\n"),
			__progname, p);
	    strcpy(centry->name, p);
	}
	set(NULL, centry);

	if (root == NULL) {
	    last = root = centry;
	    root->parent = root;
	    if (forest == NULL)
		forest = root;
	} else if (centry->name[0] == '.' && centry->name[1] == '\0') {
	    centry->prev = root;
	    last = root = root->next = centry;
	    root->parent = root;
	} else if (last->type == F_DIR && !KF_ISSET(last->flags, DONE)) {
	    centry->parent = last;
	    last = last->child = centry;
	} else {
	    centry->parent = last->parent;
	    centry->prev = last;
	    last = last->next = centry;
	}
    }
    return forest;
}

/*==============================================================*/

/*@observer@*/
static const char *
ftype(unsigned type)
	/*@*/
{
    switch(type) {
    case F_BLOCK: return "block";
    case F_CHAR:  return "char";
    case F_DIR:   return "dir";
    case F_FIFO:  return "fifo";
    case F_FILE:  return "file";
    case F_LINK:  return "link";
    case F_SOCK:  return "socket";
    default:      return "unknown";
    }
    /*@notreached@*/
}

/*@observer@*/
static const char *
inotype(mode_t mode)
	/*@*/
{
    switch(mode & S_IFMT) {
    case S_IFBLK:  return "block";
    case S_IFCHR:  return "char";
    case S_IFDIR:  return "dir";
    case S_IFIFO:  return "fifo";
    case S_IFREG:  return "file";
    case S_IFLNK:  return "link";
/*@-unrecog@*/
    case S_IFSOCK: return "socket";
/*@=unrecog@*/
    default:       return "unknown";
    }
    /*@notreached@*/
}

/*-
 * Copyright (c) 2003 Poul-Henning Kamp
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#define FF(a, b, c, d) \
	(((a)->flags & (c)) && ((b)->flags & (c)) && ((a)->d) != ((b)->d))
#define FS(a, b, c, d) \
	(((a)->flags & (c)) && ((b)->flags & (c)) && strcmp((a)->d,(b)->d))
#define FM(a, b, c, d) \
	(((a)->flags & (c)) && ((b)->flags & (c)) && memcmp(&(a)->d,&(b)->d, sizeof (a)->d))

static void
shownode(NODE *n, enum mtreeKeys_e keys, const char *path)
	/*@globals fileSystem @*/
	/*@modifies fileSystem @*/
{
    printf("%s%s %s", path, n->name, ftype((unsigned)n->type));
    if (KF_ISSET(keys, CKSUM))
	printf(" cksum=%lu", (unsigned long) n->cksum);
    if (KF_ISSET(keys, GID))
	printf(" gid=%lu", (unsigned long) n->sb.st_gid);
    if (KF_ISSET(keys, GNAME)) {
	const char * gname = gidToGname(n->sb.st_gid);
	if (gname != NULL)
	    printf(" gname=%s", gname);
	else
	    printf(" gid=%lu", (unsigned long) n->sb.st_gid);
    }
    if (KF_ISSET(keys, MODE))
	printf(" mode=%o", (unsigned) n->sb.st_mode);
    if (KF_ISSET(keys, NLINK))
	printf(" nlink=%lu", (unsigned long) n->sb.st_nlink);
/*@-duplicatequals@*/
    if (KF_ISSET(keys, SIZE))
	printf(" size=%llu", (unsigned long long)n->sb.st_size);
/*@=duplicatequals@*/
    if (KF_ISSET(keys, UID))
	printf(" uid=%lu", (unsigned long) n->sb.st_uid);
    if (KF_ISSET(keys, UNAME)) {
	const char * uname = uidToUname(n->sb.st_uid);
	if (uname != NULL)
	    printf(" uname=%s", uname);
	else
	    printf(" uid=%lu", (unsigned long) n->sb.st_uid);
    }

    /* Output all the digests. */
    if (KF_ISSET(keys, DIGEST)) {
	int i;

	if (n->algos != NULL)
	for (i = 0; i < (int) n->algos->nvals; i++) {
	    uint32_t algo = n->algos->vals[i];
	    const char * tagname = algo2tagname(algo);
	    if (tagname != NULL)
		printf(" %s=%s", tagname, n->digests[i]);
	}
    }

#if defined(HAVE_STRUCT_STAT_ST_FLAGS)
    if (KF_ISSET(keys, FLAGS)) {
	/* XXX coverity #978437 */
	const char * fflags = flags_to_string(n->sb.st_flags);
	printf(" flags=%s", fflags);
	fflags = _free(fflags);
    }
#endif
    printf("\n");
}

static int
mismatch(NODE *n1, NODE *n2, enum mtreeKeys_e  differ, const char *path)
	/*@globals fileSystem @*/
	/*@modifies fileSystem @*/
{
    enum mtreeKeys_e keys = _rpmfts->keys;

    if (n2 == NULL) {
	shownode(n1, differ, path);
	return 1;
    }
    if (n1 == NULL) {
	printf("\t");
	shownode(n2, differ, path);
	return 1;
    }
    if (!(differ & keys))
	return 0;
    printf("\t\t");
    shownode(n1, differ, path);
    printf("\t\t");
    shownode(n2, differ, path);
    return 1;
}

static int
compare_nodes(NODE *n1, NODE *n2, const char *path)
	/*@globals fileSystem @*/
	/*@modifies n1, n2, fileSystem @*/
{
    enum mtreeKeys_e differs = MTREE_KEYS_NONE;
    int xx;
	
    if (n1 != NULL && n1->type == F_LINK)
	n1->flags &= ~MTREE_KEYS_MODE;
    if (n2 != NULL && n2->type == F_LINK)
	n2->flags &= ~MTREE_KEYS_MODE;
    if (n1 == NULL && n2 != NULL) {
	differs = n2->flags;
	xx = mismatch(n1, n2, differs, path);
	return 1;
    }
    if (n1 != NULL && n2 == NULL) {
	differs = n1->flags;
	xx = mismatch(n1, n2, differs, path);
	return 1;
    }
    if (n1->type != n2->type) {
	differs = MTREE_KEYS_NONE;	/* XXX unneeded */
	xx = mismatch(n1, n2, differs, path);
	return 1;
    }
    if (FF(n1, n2, MTREE_KEYS_CKSUM, cksum))
	differs |= MTREE_KEYS_CKSUM;
    if (FF(n1, n2, MTREE_KEYS_GID, sb.st_gid))
	differs |= MTREE_KEYS_GID;
    if (FF(n1, n2, MTREE_KEYS_GNAME, sb.st_gid))
	differs |= MTREE_KEYS_GNAME;
    if (FF(n1, n2, MTREE_KEYS_MODE, sb.st_mode))
	differs |= MTREE_KEYS_MODE;
    if (FF(n1, n2, MTREE_KEYS_NLINK, sb.st_nlink))
	differs |= MTREE_KEYS_NLINK;
    if (FF(n1, n2, MTREE_KEYS_SIZE, sb.st_size))
	differs |= MTREE_KEYS_SIZE;

    if (FS(n1, n2, MTREE_KEYS_SLINK, slink))
	differs |= MTREE_KEYS_SLINK;

/*@-type@*/
    if (FM(n1, n2, MTREE_KEYS_TIME, sb.st_mtimespec))
	differs |= MTREE_KEYS_TIME;
/*@=type@*/
    if (FF(n1, n2, MTREE_KEYS_UID, sb.st_uid))
	differs |= MTREE_KEYS_UID;
    if (FF(n1, n2, MTREE_KEYS_UNAME, sb.st_uid))
	differs |= MTREE_KEYS_UNAME;

    /* Compare all the digests. */
    if (KF_ISSET(n1->flags, DIGEST) || KF_ISSET(n2->flags, DIGEST)) {
	if ((KF_ISSET(n1->flags, DIGEST) != KF_ISSET(n2->flags, DIGEST))
	 || (n1->algos == NULL || n2->algos == NULL)
	 || (n1->algos->nvals != n2->algos->nvals))
	    differs |= MTREE_KEYS_DIGEST;
	else {
	    int i;

	    for (i = 0; i < (int) n1->algos->nvals; i++) {
		if ((n1->algos->vals[i] == n2->algos->vals[i])
		 && !strcmp(n1->digests[i], n2->digests[i]))
		    continue;
		differs |= MTREE_KEYS_DIGEST;
		break;
	    }
	}
    }

#if defined(HAVE_STRUCT_STAT_ST_FLAGS)
    if (FF(n1, n2, MTREE_KEYS_FLAGS, sb.st_flags))
	differs |= MTREE_KEYS_FLAGS;
#endif

    if (differs) {
	xx = mismatch(n1, n2, differs, path);
	return 1;
    }
    return 0;
}

static int
mtreeSWalk(NODE *t1, NODE *t2, const char *path)
	/*@globals fileSystem @*/
	/*@modifies t1, t2, fileSystem @*/
{
    NODE *c1 = (t1 != NULL ? t1->child : NULL);
    NODE *c2 = (t2 != NULL ? t2->child : NULL);
    int r = 0;

    while (c1 != NULL || c2 != NULL) {
	NODE *n1, *n2;
	char *np;
	int i;

	n1 = (c1 != NULL ? c1->next : NULL);
	n2 = (c2 != NULL ? c2->next : NULL);
	if (c1 != NULL && c2 != NULL) {
	    if (c1->type != F_DIR && c2->type == F_DIR) {
		n2 = c2;
		c2 = NULL;
	    } else if (c1->type == F_DIR && c2->type != F_DIR) {
		n1 = c1;
		c1 = NULL;
	    } else {
		i = strcmp(c1->name, c2->name);
		if (i > 0) {
		    n1 = c1;
		    c1 = NULL;
		} else if (i < 0) {
		    n2 = c2;
		    c2 = NULL;
		}
	    }
	}
/*@-noeffectuncon -unrecog@*/
	if (c1 == NULL && c2->type == F_DIR) {
	    if (asprintf(&np, "%s%s/", path, c2->name)) {
		perror("asprintf");
	    }
	    i = mtreeSWalk(c1, c2, np);
	    free(np);
	    i += compare_nodes(c1, c2, path);
	} else if (c2 == NULL && c1->type == F_DIR) {
	    if (asprintf(&np, "%s%s/", path, c1->name)) {
		perror("asprintf");
	    }
	    i = mtreeSWalk(c1, c2, np);
	    free(np);
	    i += compare_nodes(c1, c2, path);
	} else if (c1 == NULL || c2 == NULL) {
	    i = compare_nodes(c1, c2, path);
	} else if (c1->type == F_DIR && c2->type == F_DIR) {
	    if (asprintf(&np, "%s%s/", path, c1->name)) {
		perror("asprintf");
	    }
	    i = mtreeSWalk(c1, c2, np);
	    free(np);
	    i += compare_nodes(c1, c2, path);
	} else {
	    i = compare_nodes(c1, c2, path);
	}
/*@=noeffectuncon =unrecog@*/
	r += i;
	c1 = n1;
	c2 = n2;
    }
    return r;	
}

int
mtreeVSpec(rpmfts fts)
{
    NODE * root1 = mtreeSpec(fts, fts->spec1);
    NODE * root2 = mtreeSpec(fts, fts->spec2);
    int rval = 0;

    rval = mtreeSWalk(root1, root2, "");
    rval += compare_nodes(root1, root2, "");
    return (rval > 0 ? MISMATCHEXIT : 0);
}

/*==============================================================*/

/*@observer@*/
static const char *
rlink(const char * name)
	/*@globals h_errno, fileSystem, internalState @*/
	/*@modifies fileSystem, internalState @*/

{
    static char lbuf[MAXPATHLEN];
    int len;

    if ((len = Readlink(name, lbuf, sizeof(lbuf)-1)) == -1)
	mtree_error("%s: %s", name, strerror(errno));
    lbuf[len] = '\0';
    return lbuf;
}

#define	SKIPDOTSLASH(_f) ((_f)[0] == '.' && (_f)[1] == '/' ? (_f) + 2 : (_f))

#define	COMPAREINDENTNAMELEN	8
#define	LABEL 			\
    if (!label++) {		\
	(void) printf(_("%s changed\n"), SKIPDOTSLASH(p->fts_path)); \
	tab = "\t"; 		\
    }

/*@observer@*/
static const char * algo2name(uint32_t algo)
	/*@*/
{
    switch (algo) {
    case PGPHASHALGO_MD5:          return "MD5";
    case PGPHASHALGO_SHA1:         return "SHA1";
    case PGPHASHALGO_RIPEMD160:    return "RIPEMD160";
    case PGPHASHALGO_MD2:          return "MD2";
    case PGPHASHALGO_TIGER192:     return "TIGER192";
    case PGPHASHALGO_HAVAL_5_160:  return "HAVAL-5-160";
    case PGPHASHALGO_SHA256:       return "SHA256";
    case PGPHASHALGO_SHA384:       return "SHA384";
    case PGPHASHALGO_SHA512:       return "SHA512";

    case PGPHASHALGO_MD4:	   return "MD4";
    case PGPHASHALGO_RIPEMD128:	   return "RIPEMD128";
    case PGPHASHALGO_CRC32:	   return "CRC32";
    case PGPHASHALGO_ADLER32:	   return "ADLER32";
    case PGPHASHALGO_CRC64:	   return "CRC64";
    case PGPHASHALGO_JLU32:	   return "JLU32";
    case PGPHASHALGO_SHA224:	   return "SHA224";
    case PGPHASHALGO_RIPEMD256:	   return "RIPEMD256";
    case PGPHASHALGO_RIPEMD320:	   return "RIPEMD320";
    case PGPHASHALGO_SALSA10:	   return "SALSA10";
    case PGPHASHALGO_SALSA20:	   return "SALSA20";

    default:                       return "Unknown";
    }
    /*@notreached@*/
}

static int
compare(rpmfts fts, NODE *const s)
	/*@globals errno, h_errno, fileSystem, internalState @*/
	/*@modifies errno, fileSystem, internalState @*/

{
    const char * name = s->name;
    FTSENT *const p = fts->p;
    const char * fts_accpath = p->fts_accpath;
    struct stat *const st = p->fts_statp;
    enum mtreeKeys_e keys = s->flags;
    int label = 0;
    const char *cp;
    const char *tab = "";

    switch(s->type) {
    case F_BLOCK:
	if (!S_ISBLK(st->st_mode))
	    goto typeerr;
	break;
    case F_CHAR:
	if (!S_ISCHR(st->st_mode))
	    goto typeerr;
	break;
    case F_DIR:
	if (!S_ISDIR(st->st_mode))
	    goto typeerr;
	break;
    case F_FIFO:
	if (!S_ISFIFO(st->st_mode))
	    goto typeerr;
	break;
    case F_FILE:
	if (!S_ISREG(st->st_mode))
	    goto typeerr;
	break;
    case F_LINK:
	if (!S_ISLNK(st->st_mode))
	    goto typeerr;
	break;
    case F_SOCK:
/*@-unrecog@*/
	if (!S_ISSOCK(st->st_mode)) {
typeerr:    LABEL;
	    (void) printf(_("\ttype expected %s found %s)\n"),
		    ftype((unsigned)s->type), inotype(st->st_mode));
	}
/*@=unrecog@*/
	break;
    }

    /* Set the uid/gid first, then set the mode. */
    if ((KF_ISSET(keys, UID) || KF_ISSET(keys, UNAME)) && s->sb.st_uid != st->st_uid) {
	LABEL;
	(void) printf(_("%s%s expected %lu found %lu"), tab, "user",
		(unsigned long)s->sb.st_uid, (unsigned long)st->st_uid);
	if (MF_ISSET(UPDATE)) {
	    if (Chown(fts_accpath, s->sb.st_uid, -1))
		(void) printf(_(" not modified: %s)\n"), strerror(errno));
	    else
		(void) printf(_(" modified)\n"));
	} else
	    (void) printf("\n");
	tab = "\t";
    }
    if ((KF_ISSET(keys, GID) || KF_ISSET(keys, GNAME)) && s->sb.st_gid != st->st_gid) {
	LABEL;
	(void) printf(_("%s%s expected %lu found %lu"), tab, "gid",
		(unsigned long)s->sb.st_gid, (unsigned long)st->st_gid);
	if (MF_ISSET(UPDATE)) {
	    if (Chown(fts_accpath, -1, s->sb.st_gid))
		(void) printf(_(" not modified: %s)\n"), strerror(errno));
	    else
		(void) printf(_(" modified)\n"));
	} else
	    (void) printf("\n");
	tab = "\t";
    }
    if (KF_ISSET(keys, MODE) && s->sb.st_mode != (st->st_mode & MBITS)) {
	if (MF_ISSET(LOOSE)) {
	    mode_t tmode = s->sb.st_mode;
	    mode_t mode = (st->st_mode & MBITS);

	    /*
	    * if none of the suid/sgid/etc bits are set,
	    * then if the mode is a subset of the target,
	    * skip.
	    */
	    if (!((tmode & ~(S_IRWXU|S_IRWXG|S_IRWXO))
	     ||    (mode & ~(S_IRWXU|S_IRWXG|S_IRWXO))))
		if ((mode | tmode) == tmode)
		    goto skip;
	}
	LABEL;
	(void) printf(_("%s%s expected %#o found %#o"), tab, "permissions",
		(unsigned)s->sb.st_mode, (unsigned)(st->st_mode & MBITS));
	if (MF_ISSET(UPDATE)) {
	    if (Chmod(fts_accpath, s->sb.st_mode))
		(void) printf(_(" not modified: %s)\n"), strerror(errno));
	    else
		(void) printf(_(" modified)\n"));
	} else
	    (void) printf("\n");
	tab = "\t";
 	skip:
	    ;
    }
    if (KF_ISSET(keys, NLINK) && s->type != F_DIR &&
        s->sb.st_nlink != st->st_nlink)
    {
	LABEL;
	(void) printf(_("%s%s expected %lu found %lu)\n"), tab, "link_count",
		(unsigned long)s->sb.st_nlink, (unsigned long)st->st_nlink);
	tab = "\t";
    }
    if (KF_ISSET(keys, SIZE) && s->sb.st_size != st->st_size) {
	LABEL;
/*@-duplicatequals@*/
	(void) printf(_("%s%s expected %llu found %llu\n"), tab, "size",
		(unsigned long long)s->sb.st_size,
		(unsigned long long)st->st_size);
/*@=duplicatequals@*/
	tab = "\t";
    }
    /*
     * XXX
     * Since utimes(2) only takes a timeval, there's no point in
     * comparing the low bits of the timespec nanosecond field.  This
     * will only result in mismatches that we can never fix.
     *
     * Doesn't display microsecond differences.
     */
    if (KF_ISSET(keys, TIME)) {
	struct timeval tv[2];

/*@-noeffectuncon -unrecog @*/
#if defined(TIMESPEC_TO_TIMEVAL)
	TIMESPEC_TO_TIMEVAL(&tv[0], &s->sb.st_mtimespec);
	TIMESPEC_TO_TIMEVAL(&tv[1], &st->st_mtimespec);
#else
	tv[0].tv_sec = (long)s->sb.st_mtime;
	tv[0].tv_usec = 0L;
	tv[1].tv_sec = (long)st->st_mtime;
	tv[1].tv_usec = 0L;
#endif
/*@=noeffectuncon =unrecog @*/
	if (tv[0].tv_sec != tv[1].tv_sec
#ifdef	NOTYET	/* XXX avoid timespec jitter issues for now. */
	 || tv[0].tv_usec != tv[1].tv_usec
#endif
	) {
	    time_t t1 = (time_t)tv[0].tv_sec;
	    time_t t2 = (time_t)tv[1].tv_sec;
	    LABEL;
	    (void) printf(_("%s%s expected %.24s "), tab, "modification time", ctime(&t1));
	    (void) printf(_("found %.24s"), ctime(&t2));
	    if (MF_ISSET(TOUCH)) {
		tv[1] = tv[0];
		if (Utimes(fts_accpath, tv))
		    (void) printf(_(" not modified: %s)\n"), strerror(errno));
		else
		    (void) printf(_(" modified\n"));
	    } else
		(void) printf("\n");
	    tab = "\t";
	}
    }

    /* Any digests to calculate? */
    if (KF_ISSET(keys, CKSUM) || s->algos != NULL) {
	FD_t fd = Fopen(fts_accpath, "r.ufdio");
	uint32_t vlen, val;
	int i;

	if (fd == NULL || Ferror(fd)) {
	    LABEL;
	    (void) printf("%scksum: %s: %s\n", tab, fts_accpath, Fstrerror(fd));
	    goto cleanup;
	}

	/* Setup all digest calculations.  Reversed order is effete ... */
	if (s->algos != NULL)
	for (i = s->algos->nvals; i-- > 0;)
	    fdInitDigest(fd, s->algos->vals[i], 0);

	/* Compute the cksum and digests. */
	if (KF_ISSET(keys, CKSUM))
	    i = crc(fd, &val, &vlen);
	else {
	    char buffer[16 * 1024];
	    while (Fread(buffer, sizeof(buffer[0]), sizeof(buffer), fd) > 0)
		{};
	    i = (Ferror(fd) ? 1 : 0);
	}
	if (i) {
	    LABEL;
	    (void) printf("%scksum: %s: %s\n", tab, fts_accpath, Fstrerror(fd));
	    goto cleanup;
	}

	/* Verify cksum. */
	if (KF_ISSET(keys, CKSUM)) {
	    if (s->cksum != val) {
		LABEL;
		(void) printf(_("%s%s expected %lu found %lu\n"), tab, "cksum",
				(unsigned long) s->cksum, (unsigned long) val);
		tab = "\t";
	    }
	}

	/* Verify all the digests. */
	if (s->algos != NULL)
	for (i = 0; i < (int) s->algos->nvals; i++) {
	    static int asAscii = 1;
	    uint32_t algo = s->algos->vals[i];
	    const char * digest = NULL;
	    size_t digestlen = 0;

	    fdFiniDigest(fd, algo, &digest, &digestlen, asAscii);
assert(digest != NULL);
	    if (strcmp(digest, s->digests[i])) {
		LABEL;
		printf(_("%s%s expected %s found %s\n"), tab, algo2name(algo),
			s->digests[i], digest);
		tab = "\t";
	    }
	    digest = _free(digest);
	    digestlen = 0;
	}

	/* Accumulate statistics and clean up. */
cleanup:
	if (fd != NULL) {
	    (void) rpmswAdd(&dc_readops, fdstat_op(fd, FDSTAT_READ));
	    (void) rpmswAdd(&dc_digestops, fdstat_op(fd, FDSTAT_DIGEST));
	    (void) Fclose(fd);
	    fd = NULL;
	}
    }

    if (KF_ISSET(keys, SLINK) && strcmp(cp = rlink(name), s->slink)) {
	LABEL;
	(void) printf(_("%s%s expected %s found %s\n"), tab, "link_ref",
		cp, s->slink);
    }
#if defined(HAVE_STRUCT_STAT_ST_FLAGS)
    if (KF_ISSET(keys, FLAGS) && s->sb.st_flags != st->st_flags) {
	char *fflags;

	LABEL;
	fflags = fflagstostr(s->sb.st_flags);
	(void) printf(_("%s%s expected \"%s\""), tab, "flags", fflags);
	fflags = _free(fflags);

	fflags = fflagstostr(st->st_flags);
	(void) printf(_(" found \"%s\""), fflags);
	fflags = _free(fflags);

	if (MF_ISSET(UPDATE)) {
	    if (chflags(fts_accpath, s->sb.st_flags))
		(void) printf(" not modified: %s)\n", strerror(errno));
	    else	
		(void) printf(" modified)\n");
	    }
	} else {
	    (void) printf("\n");
	tab = "\t";
    }
#endif
    return label;
}

/*==============================================================*/

#define	_FTSCALLOC(_p, _n)	\
    if ((_n) > 0) { \
	(_p) = _free(_p); (_p) = xcalloc((_n), sizeof(*(_p))); \
    }

static int
mtreeVisitD(rpmfts fts)
	/*@globals fileSystem, internalState @*/
	/*@modifies fts, fileSystem, internalState @*/
{
    enum mtreeKeys_e keys = fts->keys;
    const FTSENT *const parent = fts->p;
    const FTSENT * p;
    struct stat sb;
    gid_t maxgid = 0;
    uid_t maxuid = 0;
    mode_t maxmode = 0;
#if defined(HAVE_STRUCT_STAT_ST_FLAGS)
    unsigned long maxflags = 0;
#endif

    /* Retrieve all directory members. */
    if ((p = Fts_children(fts->t, 0)) == NULL) {
	if (errno)
	    mtree_error("%s: %s", SKIPDOTSLASH(parent->fts_path),
			strerror(errno));
	return 1;
    }

    sb = fts->sb;		/* structure assignment */
    _FTSCALLOC(fts->g, fts->maxg);
    _FTSCALLOC(fts->m, fts->maxm);
    _FTSCALLOC(fts->u, fts->maxu);
#if defined(HAVE_STRUCT_STAT_ST_FLAGS)
    _FTSCALLOC(fts->f, fts->maxf);
#endif

    /* Find the most common stat(2) settings for the next directory. */
    for (; p != NULL; p = p->fts_link) {
	struct stat *const st = p->fts_statp;

	if (MF_ISSET(DIRSONLY) || !S_ISDIR(st->st_mode))
	    continue;

	if (fts->m != NULL)
	{   mode_t st_mode = st->st_mode & MBITS;
	    if (st_mode < fts->maxm && ++fts->m[st_mode] > maxmode) {
		sb.st_mode = st_mode;
		maxmode = fts->m[st_mode];
	    }
	}
	if (fts->g != NULL)
	if (st->st_gid < fts->maxg && ++fts->g[st->st_gid] > maxgid) {
	    sb.st_gid = st->st_gid;
	    maxgid = fts->g[st->st_gid];
	}
	if (fts->u != NULL)
	if (st->st_uid < fts->maxu && ++fts->u[st->st_uid] > maxuid) {
	    sb.st_uid = st->st_uid;
	    maxuid = fts->u[st->st_uid];
	}
#if defined(HAVE_STRUCT_STAT_ST_FLAGS)
	/*
 	 * XXX
 	 * note that the below will break when file flags
	 * are extended beyond the first 4 bytes of each
	 * half word of the flags
	 */
#define FLAGS2IDX(f) ((f & 0xf) | ((f >> 12) & 0xf0))
	if (fts->f != NULL)
	{   unsigned long st_flags = FLAGS2IDX(st->st_flags);
	    if (st_flags < fts->maxf && ++fts->f[st_flags] > maxflags) {
		/* XXX note st->st_flags saved, not FLAGS2IDX(st->st_flags) */
		sb.st_flags = st->st_flags;
		maxflags = fts->f[st_flags];
	    }
 	}
#endif
    }

    /*
     * If the /set record is the same as the last one we do not need to output
     * a new one.  So first we check to see if anything changed.  Note that we
     * always output a /set record for the first directory.
     */
    if (((KF_ISSET(keys, UNAME) || KF_ISSET(keys, UID)) && (fts->sb.st_uid != sb.st_uid))
     || ((KF_ISSET(keys, GNAME) || KF_ISSET(keys, GID)) && (fts->sb.st_gid != sb.st_gid))
     ||  (KF_ISSET(keys, MODE) && (fts->sb.st_mode != sb.st_mode))
#if defined(HAVE_STRUCT_STAT_ST_FLAGS)
     ||  (KF_ISSET(keys, FLAGS) && (fts->sb.st_flags != sb.st_flags))
#endif
     || fts->sb_is_valid == 0)
    {
	fts->sb_is_valid = 1;
	if (MF_ISSET(DIRSONLY))
	    (void) printf("/set type=dir");
	else
	    (void) printf("/set type=file");
	if (KF_ISSET(keys, UNAME)) {
	    const char * uname = uidToUname(sb.st_uid);
	    if (uname != NULL)
		(void) printf(" uname=%s", uname);
	    else if (MF_ISSET(WARN))
		fprintf(stderr, _("%s: Could not get uname for uid=%lu\n"),
			__progname, (unsigned long) sb.st_uid);
	    else
		mtree_error("could not get uname for uid=%lu",
			(unsigned long)sb.st_uid);
	}
	if (KF_ISSET(keys, UID))
	    (void) printf(" uid=%lu", (unsigned long)sb.st_uid);
	if (KF_ISSET(keys, GNAME)) {
	    const char * gname = gidToGname(sb.st_gid);
	    if (gname != NULL)
		(void) printf(" gname=%s", gname);
	    else if (MF_ISSET(WARN))
		fprintf(stderr, _("%s: Could not get gname for gid=%lu\n"),
			__progname, (unsigned long) sb.st_gid);
	    else
		mtree_error("could not get gname for gid=%lu",
			(unsigned long) sb.st_gid);
	}
	if (KF_ISSET(keys, GID))
	    (void) printf(" gid=%lu", (unsigned long)sb.st_gid);
	if (KF_ISSET(keys, MODE))
	    (void) printf(" mode=%#o", (unsigned)sb.st_mode);
	if (KF_ISSET(keys, NLINK))
	    (void) printf(" nlink=1");
#if defined(HAVE_STRUCT_STAT_ST_FLAGS)
	if (KF_ISSET(keys, FLAGS)) {
	    const char * fflags = flags_to_string(sb.st_flags);
	    (void) printf(" flags=%s", fflags);
	    fflags = _free(fflags);
	}
#endif
	(void) printf("\n");
	fts->sb = sb;		/* structure assignment */
    }
    return (0);
}

#define	CWALKINDENTNAMELEN	15
#define	MAXLINELEN	80


static void
output(int indent, int * offset, const char * fmt, ...)
	/*@globals fileSystem @*/
	/*@modifies *offset, fileSystem @*/
{
    char buf[1024];
    va_list ap;

    va_start(ap, fmt);
    (void) vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    if (*offset + strlen(buf) > MAXLINELEN - 3) {
	(void)printf(" \\\n%*s", CWALKINDENTNAMELEN + indent, "");
	*offset = CWALKINDENTNAMELEN + indent;
    }
    *offset += printf(" %s", buf) + 1;
}

static void
mtreeVisitF(rpmfts fts)
	/*@globals errno, h_errno, fileSystem, internalState @*/
	/*@modifies errno, fileSystem, internalState @*/
{
    enum mtreeKeys_e keys = fts->keys;
    const char * fts_accpath = fts->p->fts_accpath;
    unsigned short fts_info = fts->p->fts_info;
    struct stat *const st = fts->p->fts_statp;
    int indent = (MF_ISSET(INDENT) ? fts->p->fts_level * 4 : 0);
    int offset;

    {	const char * fts_name = fts->p->fts_name;
	size_t fts_namelen = fts->p->fts_namelen;
	char * escname;

	/* XXX fts(3) (and Fts(3)) have fts_name = "" with pesky trailing '/' */
	if (fts->p->fts_level == 0 && fts_namelen == 0) {
	    fts_name = ".";
	    fts_namelen = sizeof(".") - 1;
	}

	escname = xmalloc(fts_namelen * 4  +  1);
	/* XXX TODO: Mac OS X uses VIS_GLOB as well */
	(void) strvis(escname, fts_name, VIS_WHITE | VIS_OCTAL);

	if (MF_ISSET(INDENT) || S_ISDIR(st->st_mode))
	    offset = printf("%*s%s", indent, "", escname);
	else
	    offset = printf("%*s    %s", indent, "", escname);
	escname = _free(escname);
    }

    if (offset > (CWALKINDENTNAMELEN + indent))
	offset = MAXLINELEN;
    else
	offset += printf("%*s", (CWALKINDENTNAMELEN + indent) - offset, "");

    if (!S_ISREG(st->st_mode) && !MF_ISSET(DIRSONLY))
	output(indent, &offset, "type=%s", inotype(st->st_mode));
    if (st->st_uid != fts->sb.st_uid) {
	if (KF_ISSET(keys, UNAME)) {
	    const char * uname = uidToUname(st->st_uid);
	    if (uname != NULL)
		output(indent, &offset, "uname=%s", uname);
	    else if (MF_ISSET(WARN))
		fprintf(stderr, _("%s: Could not get uname for uid=%lu\n"),
			__progname, (unsigned long) st->st_uid);
	    else
		mtree_error("could not get uname for uid=%lu",
			(unsigned long)st->st_uid);
	}
	if (KF_ISSET(keys, UID))
	    output(indent, &offset, "uid=%u", st->st_uid);
    }
    if (st->st_gid != fts->sb.st_gid) {
	if (KF_ISSET(keys, GNAME)) {
	    const char * gname = gidToGname(st->st_gid);
	    if (gname != NULL)
		output(indent, &offset, "gname=%s", gname);
	    else if (MF_ISSET(WARN))
		fprintf(stderr, _("%s: Could not get gname for gid=%lu\n"),
			__progname, (unsigned long) st->st_gid);
	    else
		mtree_error("Could not get gname for gid=%lu",
			(unsigned long) st->st_gid);
	}
	if (KF_ISSET(keys, GID))
	   output(indent, &offset, "gid=%lu", (unsigned long)st->st_gid);
    }
    if (KF_ISSET(keys, MODE) && (st->st_mode & MBITS) != fts->sb.st_mode)
	output(indent, &offset, "mode=%#o", (st->st_mode & MBITS));
    if (KF_ISSET(keys, NLINK) && st->st_nlink != 1)
	output(indent, &offset, "nlink=%lu", (unsigned long)st->st_nlink);
    if (KF_ISSET(keys, SIZE) && S_ISREG(st->st_mode))
	output(indent, &offset, "size=%llu", (unsigned long long)st->st_size);
    if (KF_ISSET(keys, TIME)) {
	struct timeval tv;
#if defined(TIMESPEC_TO_TIMEVAL)
	TIMESPEC_TO_TIMEVAL(&tv, &st->st_mtimespec);
#else
	tv.tv_sec = (long)st->st_mtime;
	tv.tv_usec = 0L;
#endif
	output(indent, &offset, "time=%lu.%lu",
		(unsigned long) tv.tv_sec,
		(unsigned long) tv.tv_usec);
    }

    /* Only files can have digests. */
    if (S_ISREG(st->st_mode)) {

	/* Any digests to calculate? */
	if (KF_ISSET(keys, CKSUM) || fts->algos != NULL) {
	    FD_t fd = Fopen(fts_accpath, "r.ufdio");
	    uint32_t len, val;
	    int i;

	    if (fd == NULL || Ferror(fd)) {
#ifdef	NOTYET	/* XXX can't exit in a library API. */
		(void) fprintf(stderr, _("%s: %s: cksum: %s\n"),
			__progname, fts_accpath, Fstrerror(fd));
		goto cleanup;
#else
		mtree_error("%s: %s", fts_accpath, Fstrerror(fd));
		/*@notreached@*/
#endif
	    }

	    /* Setup all digest calculations.  Reversed order is effete ... */
	    if (fts->algos != NULL)
	    for (i = fts->algos->nvals; i-- > 0;)
		fdInitDigest(fd, fts->algos->vals[i], 0);

	    /* Compute the cksum and digests. */
	    if (KF_ISSET(keys, CKSUM))
		i = crc(fd, &val, &len);
	    else {
		char buffer[16 * 1024];
		while (Fread(buffer, sizeof(buffer[0]), sizeof(buffer), fd) > 0)
		    {};
		i = (Ferror(fd) ? 1 : 0);
	    }
	    if (i) {
#ifdef	NOTYET	/* XXX can't exit in a library API. */
		(void) fprintf(stderr, _("%s: %s: cksum: %s\n"),
			__progname, fts_accpath, Fstrerror(fd));
#else
		mtree_error("%s: %s", fts_accpath, Fstrerror(fd));
		/*@notreached@*/
#endif
		goto cleanup;
	    }

	    /* Output cksum. */
	    if (KF_ISSET(keys, CKSUM)) {
		output(indent, &offset, "cksum=%lu", (unsigned long)val);
	    }

	    /* Output all the digests. */
	    if (fts->algos != NULL)
	    for (i = 0; i < (int) fts->algos->nvals; i++) {
		static int asAscii = 1;
		const char * digest = NULL;
		size_t digestlen = 0;
		uint32_t algo;

		algo = fts->algos->vals[i];
		fdFiniDigest(fd, algo, &digest, &digestlen, asAscii);
#ifdef	NOTYET	/* XXX can't exit in a library API. */
assert(digest != NULL);
#else
		if (digest == NULL)
		    mtree_error("%s: %s", fts_accpath, Fstrerror(fd));
#endif
		{   const char * tagname = algo2tagname(algo);
		    if (tagname != NULL)
			output(indent, &offset, "%s=%s", tagname, digest);
		}
		digest = _free(digest);
		digestlen = 0;
	    }

cleanup:    /* Accumulate statistics and clean up. */
	    if (fd != NULL) {
		(void) rpmswAdd(&dc_readops, fdstat_op(fd, FDSTAT_READ));
		(void) rpmswAdd(&dc_digestops, fdstat_op(fd, FDSTAT_DIGEST));
		(void) Fclose(fd);
		fd = NULL;
	    }
	}
    }

    if (KF_ISSET(keys, SLINK) && (fts_info == FTS_SL || fts_info == FTS_SLNONE))
    {
	const char * name = rlink(fts_accpath);
	char * escname = xmalloc(strlen(name) * 4  +  1);
	(void) strvis(escname, name, VIS_WHITE | VIS_OCTAL);
	output(indent, &offset, "link=%s", escname);
	escname = _free(escname);
    }

    if (KF_ISSET(keys, FLAGS) && !S_ISLNK(st->st_mode)) {
#if defined(HAVE_STRUCT_STAT_ST_FLAGS)
	char * fflags = fflagstostr(st->st_flags);

	if (fflags != NULL && fflags[0] != '\0')
	    output(indent, &offset, "flags=%s", fflags);
	else
	    output(indent, &offset, "flags=none");
	free(fflags);
#else
	output(indent, &offset, "flags=none");
#endif
    }
    (void) putchar('\n');
}

/*==============================================================*/

/*
 * Copyright 2000 Massachusetts Institute of Technology
 *
 * Permission to use, copy, modify, and distribute this software and
 * its documentation for any purpose and without fee is hereby
 * granted, provided that both the above copyright notice and this
 * permission notice appear in all copies, that both the above
 * copyright notice and this permission notice appear in all
 * supporting documentation, and that the name of M.I.T. not be used
 * in advertising or publicity pertaining to distribution of the
 * software without specific, written prior permission.  M.I.T. makes
 * no representations about the suitability of this software for any
 * purpose.  It is provided "as is" without express or implied
 * warranty.
 *
 * THIS SOFTWARE IS PROVIDED BY M.I.T. ``AS IS''.  M.I.T. DISCLAIMS
 * ALL EXPRESS OR IMPLIED WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT
 * SHALL M.I.T. BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * We're assuming that there won't be a whole lot of excludes,
 * so it's OK to use a stupid algorithm.
 */

/*@mayexit@*/
static void
mtreeReadExcludes(const char * fn)
	/*@globals excludes, h_errno, fileSystem, internalState @*/
	/*@modifies excludes, fileSystem, internalState @*/
{
    FD_t fd = Fopen(fn, "r.fpio");
    FILE *fp;
    char buffer[16 * 1024];

    if (fd == NULL || Ferror(fd) || (fp = fdGetFILE(fd)) == NULL) {
	fprintf(stderr, _("%s: open of %s failed: %s\n"), __progname,
		fn, Fstrerror(fd));
	if (fd != NULL) (void) Fclose(fd);
	exit(EXIT_FAILURE);
    }

    while (fgets(buffer, (int)sizeof(buffer), fp) != NULL) {
	struct exclude *e;
	char * line;
	size_t len;
	
	buffer[sizeof(buffer)-1] = '\0';
	for (line = buffer; *line != '\0'; line++)
	    if (strchr(" \t\n\r", line[1]) == NULL) /*@innerbreak@*/ break;
	if (*line == '\0' || *line == '#')
	    continue;
	for (len = strlen(line); len > 0; len--)
	    if (strchr(" \t\n\r", line[len-1]) == NULL) /*@innerbreak@*/ break;
	if (len == 0)
	    continue;

	e = xmalloc(sizeof(*e));
	e->glob = xstrdup(line);
	e->pathname = (strchr(line, '/') != NULL ? 1 : 0);
/*@-immediatetrans@*/
	RPM_LIST_INSERT_HEAD(&excludes, e, link);
/*@=immediatetrans@*/
    }
    if (fd != NULL)
	(void) Fclose(fd);
/*@-compmempass -nullstate @*/
    return;
/*@=compmempass =nullstate @*/
}

static int
mtreeCheckExcludes(const char *fname, const char *path)
	/*@*/
{
    struct exclude *e;

    /* fnmatch(3) has a funny return value convention... */
#define MATCH(g, n) (fnmatch((g), (n), FNM_PATHNAME) == 0)

/*@-predboolptr@*/
    RPM_LIST_FOREACH(e, &excludes, link) {
	if ((e->pathname && MATCH(e->glob, path)) || MATCH(e->glob, fname))
	    return 1;
    }
/*@=predboolptr@*/
    return 0;
}

/*==============================================================*/

static int
dsort(const FTSENT ** a, const FTSENT ** b)
	/*@*/
{
    if (S_ISDIR((*a)->fts_statp->st_mode)) {
	if (!S_ISDIR((*b)->fts_statp->st_mode))
	    return 1;
    } else if (S_ISDIR((*b)->fts_statp->st_mode))
	return -1;
    return strcmp((*a)->fts_name, (*b)->fts_name);
}

#if defined(_RPMFI_INTERNAL)
/**
 * Check file name for a suffix.
 * @param fn		file name
 * @param suffix	suffix
 * @return		1 if file name ends with suffix
 */
static int chkSuffix(const char * fn, const char * suffix)
	/*@*/
{
    size_t flen = strlen(fn);
    size_t slen = strlen(suffix);
    return (flen > slen && !strcmp(fn + flen - slen, suffix));
}

static int _rpmfiStat(const char * path, struct stat * st)
	/*@globals fileSystem @*/
	/*@modifies fileSystem @*/
{
    rpmfts fts = _rpmfts;
    rpmfi fi = _rpmfts->fi;
    size_t len = strlen(fts->paths[0]);
    int rc;

    rc = rpmfiStat(fi, path+len, st);

if (_fts_debug)
fprintf(stderr, "*** _rpmfiStat(%s, %p) fi %p rc %d\n", path+len, st, fi, rc);

    return rc;
}

static int _rpmfiClosedir(/*@only@*/ DIR * dir)
	/*@globals fileSystem @*/
	/*@modifies dir, fileSystem @*/
{
    rpmfi fi = _rpmfts->fi;

if (_fts_debug)
fprintf(stderr, "*** _rpmfiClosedir(%p) fi %p\n", dir, fi);

    return avClosedir(dir);
}

static /*@null@*/ struct dirent * _rpmfiReaddir(DIR * dir)
	/*@globals fileSystem @*/
	/*@modifies fileSystem @*/
{
    rpmfi fi = _rpmfts->fi;
    struct dirent * dp = (struct dirent *) avReaddir(dir);

if (_fts_debug)
fprintf(stderr, "*** _rpmfiReaddir(%p) fi %p %p \"%s\"\n", dir, fi, dp, (dp != NULL ? dp->d_name : ""));

/*@-dependenttrans@*/
    return dp;
/*@=dependenttrans@*/
}

static /*@null@*/
uint8_t * rpmfiParentDirNotWithin(rpmfi fi)
	/*@*/
{
    size_t * dnlens = xmalloc(fi->dc * sizeof(*dnlens));
    uint8_t * noparent = memset(xmalloc(fi->dc), 1, fi->dc);
    int i, j;

    for (i = 0; i < (int)fi->dc; i++)
	dnlens[i] = strlen(fi->dnl[i]);

    /* Mark parent directories that are not contained within same package. */
    for (i = 0; i < (int)fi->fc; i++) {
	size_t dnlen, bnlen;

	if (!S_ISDIR(fi->fmodes[i]))
	    continue;

	dnlen = dnlens[fi->dil[i]];
	bnlen = strlen(fi->bnl[i]);

	for (j = 0; j < (int)fi->dc; j++) {

	    if (!noparent[j] || j == (int)fi->dil[i])
		/*@innercontinue@*/ continue;
	    if (dnlens[j] != (dnlen+bnlen+1))
		/*@innercontinue@*/ continue;
	    if (strncmp(fi->dnl[j], fi->dnl[fi->dil[i]], dnlen))
		/*@innercontinue@*/ continue;
	    if (strncmp(fi->dnl[j]+dnlen, fi->bnl[i], bnlen))
		/*@innercontinue@*/ continue;
	    if (fi->dnl[j][dnlen+bnlen] != '/' || fi->dnl[j][dnlen+bnlen+1] != '\0')
		/*@innercontinue@*/ continue;

	    /* This parent directory is contained within the package. */
	    noparent[j] = (uint8_t)0;
	    /*@innerbreak@*/ break;
	}
    }
    dnlens = _free(dnlens);
    return noparent;
}

static Header rpmftsReadHeader(rpmfts fts, const char * path)
	/*@globals rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
	/*@modifies fts, rpmGlobalMacroContext, fileSystem, internalState @*/
{
    FD_t fd = Fopen(path, "r.ufdio");
    Header h = NULL;

    if (fd != NULL) {
	/* XXX what if path needs expansion? */
	rpmRC rpmrc = rpmReadPackageFile(fts->ts, fd, path, &h);

	(void) Fclose(fd);

	switch (rpmrc) {
	case RPMRC_NOTFOUND:
	    /* XXX Read a package manifest. Restart ftswalk on success. */
	case RPMRC_FAIL:
	default:
	    (void)headerFree(h);
	    h = NULL;
	    break;
	case RPMRC_NOTTRUSTED:
	case RPMRC_NOKEY:
	case RPMRC_OK:
	    break;
	}
    }
    return h;
}

static /*@null@*/ rpmfi rpmftsLoadFileInfo(rpmfts fts, const char * path)
	/*@globals rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
	/*@modifies fts, rpmGlobalMacroContext, fileSystem, internalState @*/
{
    char * fn = xstrdup(path);
    size_t nb = strlen(fn);
    Header h = NULL;

    fn[nb-1] = '\0';		/* XXX trim pesky trailing '/' */
    h = rpmftsReadHeader(fts, fn);
    fn = _free(fn);

    if (h != NULL) {
	fts->fi = rpmfiNew(fts->ts, h, RPMTAG_BASENAMES, 0);
	(void)headerFree(h);
	h = NULL;
    }
    return fts->fi;
}

static /*@null@*/ DIR * _rpmfiOpendir(const char * path)
	/*@globals _rpmfts, rpmGlobalMacroContext, h_errno,
		fileSystem, internalState @*/
	/*@modifies _rpmfts, rpmGlobalMacroContext,
		fileSystem, internalState @*/
{
    rpmfts fts = _rpmfts;
    DIR * dir = NULL;

    if (fts->ts == NULL)
	fts->ts = rpmtsCreate();

    if (fts->fi == NULL) {
	rpmfi fi = rpmftsLoadFileInfo(fts, path);
 	uint8_t * noparent = rpmfiParentDirNotWithin(fi);
	uint16_t * fmodes = xcalloc(rpmfiFC(fi)+1, sizeof(*fmodes));
	const char ** fnames = NULL;
	int ac = 0;
	int i;

	/* Collect top level files/dirs from the package. */
	fi = rpmfiInit(fi, 0);
	while ((i = rpmfiNext(fi)) >= 0) {
	    int xx;
	    if (!S_ISDIR(fi->fmodes[i]) && !noparent[fi->dil[i]])
		continue;
	    xx = argvAdd(&fnames, rpmfiFN(fi));
	    fmodes[ac++] = fi->fmodes[i];
	}

	dir = (DIR *) avOpendir(path, fnames, fmodes);

	fnames = argvFree(fnames);
	fmodes = _free(fmodes);
	noparent = _free(noparent);

    } else {
	const char * dn = path + strlen(fts->paths[0]);

	dir = rpmfiOpendir(fts->fi, dn);
    }

if (_fts_debug)
fprintf(stderr, "*** _rpmfiOpendir(%s) dir %p\n", path, dir);

    return dir;
}

#define ALIGNBYTES      (__alignof__ (long double) - 1)
#define ALIGN(p)        (((unsigned long int) (p) + ALIGNBYTES) & ~ALIGNBYTES)

static FTSENT *
fts_alloc(FTS * sp, const char * name, int namelen)
	/*@*/
{
	register FTSENT *p;
	size_t len;

	/*
	 * The file name is a variable length array and no stat structure is
	 * necessary if the user has set the nostat bit.  Allocate the FTSENT
	 * structure, the file name and the stat structure in one chunk, but
	 * be careful that the stat structure is reasonably aligned.  Since the
	 * fts_name field is declared to be of size 1, the fts_name pointer is
	 * namelen + 2 before the first possible address of the stat structure.
	 */
	len = sizeof(*p) + namelen;
/*@-sizeoftype@*/
	if (!(sp->fts_options & FTS_NOSTAT))
		len += sizeof(*p->fts_statp) + ALIGNBYTES;
/*@=sizeoftype@*/
	p = xmalloc(len);

	/* Copy the name and guarantee NUL termination. */
	memmove(p->fts_name, name, namelen);
	p->fts_name[namelen] = '\0';

/*@-sizeoftype@*/
	if (!(sp->fts_options & FTS_NOSTAT))
		p->fts_statp = (struct stat *)ALIGN(p->fts_name + namelen + 2);
/*@=sizeoftype@*/
	p->fts_namelen = namelen;
	p->fts_path = sp->fts_path;
	p->fts_errno = 0;
	p->fts_flags = 0;
	p->fts_instr = FTS_NOINSTR;
	p->fts_number = 0;
	p->fts_pointer = NULL;
	return (p);
}

static void _rpmfiSetFts(rpmfts fts)
	/*@globals h_errno, fileSystem, internalState @*/
	/*@modifies fts, fileSystem, internalState @*/
{
    char *const * argv = (char *const *) fts->paths;
    FTS * sp = fts->t;
#ifdef	NOTYET
    int (*compar) (const FTSENT **, const FTSENT **) = sp->compar;
#endif
	register FTSENT *p, *root;
	register int nitems;
	FTSENT *parent = NULL;
	FTSENT *tmp = NULL;
	size_t len;

if (_fts_debug)
fprintf(stderr, "*** _rpmfiSetFts(%p)\n", fts);

/*@-type@*/
    sp->fts_opendir = _rpmfiOpendir;
    sp->fts_readdir = _rpmfiReaddir;
    sp->fts_closedir = _rpmfiClosedir;
    sp->fts_stat = _rpmfiStat;
    sp->fts_lstat = _rpmfiStat;
/*@=type@*/

	/* Allocate/initialize root's parent. */
	if (*argv != NULL) {
		parent = fts_alloc(sp, "", 0);
		parent->fts_level = FTS_ROOTPARENTLEVEL;
	}

	/* Allocate/initialize root(s). */
	for (root = NULL, nitems = 0; *argv != NULL; ++argv, ++nitems) {
		len = strlen(*argv);

		p = fts_alloc(sp, *argv, (int)len);
		p->fts_level = FTS_ROOTLEVEL;
		p->fts_parent = parent;
		p->fts_accpath = p->fts_name;
#ifdef	NOTYET
		p->fts_info = fts_stat(sp, p, ISSET(FTS_COMFOLLOW));

		/* Command-line "." and ".." are real directories. */
		if (p->fts_info == FTS_DOT)
			p->fts_info = FTS_D;

#else
		p->fts_name[len-1] = '\0';
		{   struct stat * st = p->fts_statp;
		    int xx = Stat(p->fts_accpath, st);
		    xx = xx;
		    st->st_mode &= ~S_IFMT;
		    st->st_mode |= S_IFDIR;
		    p->fts_dev = st->st_dev;
		    p->fts_ino = st->st_ino;
		    p->fts_nlink = st->st_nlink;
		}
		p->fts_name[len-1] = '/';
		p->fts_info = FTS_D;
#endif

#ifdef	NOTYET
		/*
		 * If comparison routine supplied, traverse in sorted
		 * order; otherwise traverse in the order specified.
		 */
		if (compar) {
			p->fts_link = root;
			root = p;
		} else
#endif
		{
			p->fts_link = NULL;
			if (root == NULL)
				tmp = root = p;
			else {
				if (tmp != NULL)	/* XXX can't happen */
					tmp->fts_link = p;
				tmp = p;
			}
		}
	}
#ifdef	NOTYET
	if (compar && nitems > 1)
		root = fts_sort(sp, root, nitems);
#endif

	/*
	 * Allocate a dummy pointer and make fts_read think that we've just
	 * finished the node before the root(s); set p->fts_info to FTS_INIT
	 * so that everything about the "current" node is ignored.
	 */
    sp->fts_cur = _free(sp->fts_cur);
	sp->fts_cur = fts_alloc(sp, "", 0);
	sp->fts_cur->fts_link = root;
	sp->fts_cur->fts_info = FTS_INIT;

    return;
}
#endif

int
mtreeCWalk(rpmfts fts)
{
#if defined(_RPMFI_INTERNAL)
    int isrpm = chkSuffix(fts->paths[0], ".rpm/");
#else
    int isrpm = 0;
#endif
    const char * empty = NULL;
    char *const * paths = (char *const *) (isrpm ? &empty : fts->paths);
    int ftsoptions = fts->ftsoptions | (isrpm ? (FTS_NOCHDIR|FTS_COMFOLLOW) : 0);
    int rval = 0;

    fts->t = Fts_open(paths, ftsoptions, dsort);
    if (fts->t == NULL)
	mtree_error("Fts_open: %s", strerror(errno));

#if defined(_RPMFI_INTERNAL)
    if (isrpm) {
	fts->keys &= ~MTREE_KEYS_SLINK;		/* XXX no rpmfiReadlink yet. */
	_rpmfiSetFts(fts);
    }
#endif

    while ((fts->p = Fts_read(fts->t)) != NULL) {
	int indent = 0;
	if (MF_ISSET(INDENT))
	    indent = fts->p->fts_level * 4;
	if (mtreeCheckExcludes(fts->p->fts_name, fts->p->fts_path)) {
	    (void) Fts_set(fts->t, fts->p, FTS_SKIP);
	    continue;
	}
	switch(fts->p->fts_info) {
	case FTS_D:
	    if (!MF_ISSET(DIRSONLY))
		(void) printf("\n");
	    if (!MF_ISSET(NOCOMMENT))
		(void) printf("# %s\n", fts->p->fts_path);
	    (void) mtreeVisitD(fts);
	    mtreeVisitF(fts);
	    /*@switchbreak@*/ break;
	case FTS_DP:
	    if (!MF_ISSET(NOCOMMENT) && (fts->p->fts_level > 0))
		(void) printf("%*s# %s\n", indent, "", fts->p->fts_path);
	    (void) printf("%*s..\n", indent, "");
	    if (!MF_ISSET(DIRSONLY))
		(void) printf("\n");
	    /*@switchbreak@*/ break;
	case FTS_DNR:
	case FTS_ERR:
	case FTS_NS:
	    (void) fprintf(stderr, "%s: %s: %s\n", __progname,
			fts->p->fts_path, strerror(fts->p->fts_errno));
	    /*@switchbreak@*/ break;
	default:
	    if (!MF_ISSET(DIRSONLY))
		mtreeVisitF(fts);
	    /*@switchbreak@*/ break;
	}
    }
    (void) Fts_close(fts->t);
    fts->p = NULL;
    fts->t = NULL;
    return rval;
}

/*==============================================================*/

void
mtreeMiss(rpmfts fts, /*@null@*/ NODE * p, char * tail)
{
    int create;
    char *tp;
    const char *type;

    for (; p != NULL; p = p->next) {
	/* XXX Mac OS X doesn't do this. */
	if (KF_ISSET(p->flags, OPT) && !KF_ISSET(p->flags, VISIT))
	    continue;
	if (p->type != F_DIR && (MF_ISSET(DIRSONLY) || KF_ISSET(p->flags, VISIT)))
	    continue;
	(void) strcpy(tail, p->name);
	if (!KF_ISSET(p->flags, VISIT)) {
	    /* Don't print missing message if file exists as a
	       symbolic link and the -q flag is set. */
	    struct stat sb;

	    if (MF_ISSET(QUIET) && Stat(fts->path, &sb) == 0)
		p->flags |= MTREE_KEYS_VISIT;
	    else
		(void) printf(_("missing: %s"), fts->path);
	}
	if (p->type != F_DIR && p->type != F_LINK) {
	    (void) putchar('\n');
	    continue;
	}

	create = 0;
	type = (p->type == F_LINK ? "symlink" : "directory");
	if (!KF_ISSET(p->flags, VISIT) && MF_ISSET(UPDATE)) {
	    if (!(KF_ISSET(p->flags, UID) && KF_ISSET(p->flags, UNAME)))
		(void) printf(_(" (%s not created: user not specified)"), type);
	    else if (!(KF_ISSET(p->flags, GID) || KF_ISSET(p->flags, GNAME)))
		(void) printf(_(" (%s not created: group not specified))"), type);
	    else if (p->type == F_LINK) {
		if (Symlink(p->slink, fts->path))
		    (void) printf(_(" (%s not created: %s)\n"), type,
				strerror(errno));
		else
		    (void) printf(_(" (%s created)\n"), type);
		if (lchown(fts->path, p->sb.st_uid, p->sb.st_gid) == -1) {
		    const char * what;
		    int serr = errno;

		    if (p->sb.st_uid == (uid_t)-1)
			what = "group";
		    else if (lchown(fts->path, (uid_t)-1, p->sb.st_gid) == -1)
			what = "user & group";
		    else {
			what = "user";
			errno = serr;
		    }
		    (void) printf(_("%s: %s not modified: %s\n"),
				fts->path, what, strerror(errno));
		}
		continue;
	    } else if (!KF_ISSET(p->flags, MODE))
		(void) printf(_(" (%s not created: mode not specified)"), type);
	    else if (Mkdir(fts->path, S_IRWXU))
		(void) printf(_(" (%s not created: %s)"),type, strerror(errno));
	    else {
		create = 1;
		(void) printf(_(" (%s created)"), type);
	    }
	}

	if (!KF_ISSET(p->flags, VISIT))
	    (void) putchar('\n');

	for (tp = tail; *tp != '\0'; ++tp);
	*tp = '/';
	mtreeMiss(fts, p->child, tp + 1);
	*tp = '\0';

	if (!create)
	    continue;
	if (Chown(fts->path, p->sb.st_uid, p->sb.st_gid)) {
	    const char * what;
	    int serr = errno;

	    if (p->sb.st_uid == (uid_t)-1)
		what = "group";
	    else if (Chown(fts->path, (uid_t)-1, p->sb.st_gid) == -1)
		what = "user & group";
	    else {
		what = "user";
		errno = serr;
	    }
	    (void) printf(_("%s: %s not modified: %s\n"),
		    fts->path, what, strerror(errno));
	    continue;
	}
	if (Chmod(fts->path, p->sb.st_mode))
	    (void) printf(_("%s: permissions not set: %s\n"),
		    fts->path, strerror(errno));
#if defined(HAVE_STRUCT_STAT_ST_FLAGS)
	if (KF_ISSET(p->flags, FLAGS) && p->sb.st_flags != 0 &&
                    chflags(fts->path, p->sb.st_flags))
                        (void) printf(_("%s: file flags not set: %s\n"),
				fts->path, strerror(errno));
#endif
    }
}

int
mtreeVWalk(rpmfts fts)
{
#if defined(_RPMFI_INTERNAL)
    int isrpm = chkSuffix(fts->paths[0], ".rpm/");
#else
    int isrpm = 0;
#endif
    const char * empty = NULL;
    char *const * paths = (char *const *) (isrpm ? &empty : fts->paths);
    int ftsoptions = fts->ftsoptions | (isrpm ? (FTS_NOCHDIR|FTS_COMFOLLOW) : 0);
    NODE * level = NULL;
    NODE * root = NULL;
    int specdepth = 0;
    int rval = 0;

    fts->t = Fts_open((char *const *)paths, ftsoptions, NULL);
    if (fts->t == NULL)
	mtree_error("Fts_open: %s", strerror(errno));

#if defined(_RPMFI_INTERNAL)
    if (isrpm) {
	fts->keys &= ~MTREE_KEYS_SLINK;		/* XXX no rpmfiReadlink yet. */
	_rpmfiSetFts(fts);
    }
#endif

    while ((fts->p = Fts_read(fts->t)) != NULL) {
	const char * fts_name = fts->p->fts_name;
	size_t fts_namelen = fts->p->fts_namelen;

#if 0
fprintf(stderr, "==> level %d info 0x%x name %p[%d] \"%s\" accpath \"%s\" path \"%s\"\n", fts->p->fts_level, fts->p->fts_info, fts_name, fts_namelen, fts_name, fts->p->fts_accpath, fts->p->fts_path);
#endif
	/* XXX fts(3) (and Fts(3)) have fts_name = "" with pesky trailing '/' */
	if (fts->p->fts_level == 0 && fts_namelen == 0) {
	    fts_name = ".";
	    fts_namelen = sizeof(".") - 1;
	}

	if (mtreeCheckExcludes(fts_name, fts->p->fts_path)) {
	    (void) Fts_set(fts->t, fts->p, FTS_SKIP);
	    continue;
	}
	switch(fts->p->fts_info) {
	case FTS_D:
	case FTS_SL:
	    if (fts->p->fts_level == 0) {
assert(specdepth == 0);
		if (root == NULL)
		    level = root = fts->root;
		else if (root->next != fts->root)
		    level = root = root->next;
assert(level == level->parent);
	    }
	   /*@switchbreak@*/ break;
	case FTS_DP:
	    if (specdepth > fts->p->fts_level) {
		for (level = level->parent; level->prev != NULL; level = level->prev);
		--specdepth;
	    }
	    continue;
	case FTS_DNR:
	case FTS_ERR:
	case FTS_NS:
	    (void) fprintf(stderr, "%s: %s: %s\n", __progname,
			SKIPDOTSLASH(fts->p->fts_path),
			strerror(fts->p->fts_errno));
	    continue;
	default:
	    if (MF_ISSET(DIRSONLY))
		continue;
	}

	if (specdepth == fts->p->fts_level) {
	    NODE *ep;
	    for (ep = level; ep != NULL; ep = ep->next)
		if ((KF_ISSET(ep->flags, MAGIC) &&
/*@-moduncon@*/
		    !fnmatch(ep->name, fts_name, FNM_PATHNAME)) ||
/*@=moduncon@*/
		    !strcmp(ep->name, fts_name))
		{
		    ep->flags |= MTREE_KEYS_VISIT;
		    if (!KF_ISSET(ep->flags, NOCHANGE) &&
			compare(fts, ep))
			    rval = MISMATCHEXIT;
		    if (KF_ISSET(ep->flags, IGN))
			    (void) Fts_set(fts->t, fts->p, FTS_SKIP);
		    else
		    if (ep->child && ep->type == F_DIR && fts->p->fts_info == FTS_D)
		    {
			    level = ep->child;
			    ++specdepth;
		}
		/*@innerbreak@*/ break;
	    }
	    if (ep != NULL)
	 	continue;
	}

	if (!MF_ISSET(IGNORE)) {
	    (void) printf("%s extra", SKIPDOTSLASH(fts->p->fts_path));
	    if (MF_ISSET(REMOVE)) {
		if ((S_ISDIR(fts->p->fts_statp->st_mode)
		    ? Rmdir : Unlink)(fts->p->fts_accpath)) {
			(void) printf(_(", not removed: %s"), strerror(errno));
		} else
			(void) printf(_(", removed"));
	    }
	    (void) putchar('\n');
	}
	(void) Fts_set(fts->t, fts->p, FTS_SKIP);
    }
    (void) Fts_close(fts->t);
    fts->p = NULL;
    fts->t = NULL;
    return rval;
}

/*==============================================================*/

/**
 */
static void mtreeArgCallback(poptContext con,
                /*@unused@*/ enum poptCallbackReason reason,
                const struct poptOption * opt, const char * arg,
                /*@unused@*/ void * data)
        /*@globals _rpmfts, rpmioFtsOpts, h_errno, fileSystem, internalState @*/
        /*@modifies _rpmfts, rpmioFtsOpts, fileSystem, internalState @*/
{
    char * p;

    /* XXX avoid accidental collisions with POPT_BIT_SET for flags */
    if (opt->arg == NULL)
    switch (opt->val) {

    case 'f':
	if (_rpmfts->spec1 == NULL) {
	    if ((_rpmfts->spec1 = fopen(arg, "r")) != NULL)
		mtree_error("%s: %s", arg, strerror(errno));
	} else if (_rpmfts->spec2 == NULL) {
	    if ((_rpmfts->spec2 = fopen(arg, "r")) != NULL)
		mtree_error("%s: %s", arg, strerror(errno));
	} else {
	    /* XXX error message, too many -f options. */
	    poptPrintUsage(con, stderr, 0);
	    exit(EXIT_FAILURE);
	    /*@notreached@*/
	}
	break;
    case 'k':
	/* XXX fts->keys = KEYDEFAULT in main(), clear default now. */
	_rpmfts->keys = MTREE_KEYS_TYPE;
	/*@fallthrough@*/
    case 'K':
/*@-unrecog@*/
	while ((p = strsep((char **)&arg, " \t,")) != NULL) {
	    uint32_t needvalue;
	    enum mtreeKeys_e type = parsekey(p, &needvalue);
	    if (type == 0) {
		/* XXX unknown key error. */
		continue;
	    }
	    _rpmfts->keys |= type;
	    /* XXX dupes can occur */
	    if (KF_ISSET(_rpmfts->keys, DIGEST) && needvalue)
		(void) argiAdd(&_rpmfts->algos, -1, (int)needvalue);
	}
/*@=unrecog@*/
	break;

    /* XXX redundant with --logical. */
    case 'L':
	rpmioFtsOpts &= ~FTS_PHYSICAL;
	rpmioFtsOpts |= FTS_LOGICAL;
	break;
    /* XXX redundant with --physical. */
    case 'P':
	rpmioFtsOpts &= ~FTS_LOGICAL;
	rpmioFtsOpts |= FTS_PHYSICAL;
	break;
    case 'X':
	mtreeReadExcludes(arg);
	break;

    case '?':
    default:
	fprintf(stderr, _("%s: Unknown option -%c\n"), __progname, opt->val);
	poptPrintUsage(con, stderr, 0);
	exit(EXIT_FAILURE);
	/*@notreached@*/ break;
    }
}

/*@unchecked@*/ /*@observer@*/
static struct poptOption optionsTable[] = {
/*@-type@*/ /* FIX: cast? */
 { NULL, '\0', POPT_ARG_CALLBACK | POPT_CBFLAG_INC_DATA | POPT_CBFLAG_CONTINUE,
        mtreeArgCallback, 0, NULL, NULL },
/*@=type@*/

    /* XXX redundant with --logical. */
  { NULL,'L', POPT_ARG_NONE,	NULL, (int)'L',
	N_("Follow symlinks"), NULL },
    /* XXX redundant with --physical. */
  { NULL,'P', POPT_ARG_NONE,	NULL, (int)'P',
	N_("Don't follow symlinks"), NULL },
  { NULL,'X', POPT_ARG_NONE,	NULL, (int)'X',
	N_("Read fnmatch(3) exclude patterns from <file>"), N_("<file>") },

  { "create",'c', POPT_BIT_SET,		&mtreeFlags, MTREE_FLAGS_CREATE,
	N_("Print file tree specification to stdout"), NULL },
  { "dirs",'d', POPT_BIT_SET,		&mtreeFlags, MTREE_FLAGS_DIRSONLY,
	N_("Directories only"), NULL },
  { "ignore",'e', POPT_BIT_SET,		&mtreeFlags, MTREE_FLAGS_IGNORE,
	N_("Don't complain about files not in the specification"), NULL },
  { "file",'f', POPT_ARG_STRING,	NULL, (int)'f',
	N_("Read file tree <spec>"), N_("<spec>") },
  { "indent",'i', POPT_BIT_SET,		&mtreeFlags, MTREE_FLAGS_INDENT,
	N_("Indent sub-directories"), NULL },
  { "add",'K', POPT_ARG_STRING,	NULL, (int)'K',
	N_("Add <key> to specification"), N_("<key>") },
  { "key",'k', POPT_ARG_STRING,	NULL, (int)'k',
	N_("Use \"type\" keywords instead"), N_("<key>") },
  { "loose",'l', POPT_BIT_SET,		&mtreeFlags, MTREE_FLAGS_LOOSE,
	N_("Loose permissions check"), NULL },
  { "nocomment",'n', POPT_BIT_SET,	&mtreeFlags, MTREE_FLAGS_NOCOMMENT,
	N_("Don't include sub-directory comments"), NULL },
  { "path",'p', POPT_ARG_ARGV,	&__rpmfts.paths, 0,
	N_("Use <path> rather than current directory"), N_("<path>") },
  /* XXX --quiet collides w poptIO */
  { "quiet",'q', POPT_BIT_SET,		&mtreeFlags, MTREE_FLAGS_QUIET,
	N_("Quiet mode"), NULL },
  { "remove",'r', POPT_BIT_SET,		&mtreeFlags, MTREE_FLAGS_REMOVE,
	N_("Remove files not in specification"), NULL },
  { "seed",'s', POPT_ARG_INT,		&__rpmfts.crc_total, 0,
	N_("Display crc for file(s) with <seed>"), N_("<seed>") },
  { "touch",'t', POPT_BIT_SET,		&mtreeFlags, MTREE_FLAGS_TOUCH,
	N_("Touch files iff timestamp differs"), NULL },
  { "update",'u', POPT_BIT_SET,		&mtreeFlags, MTREE_FLAGS_UPDATE,
	N_("Update owner/group/permissions to match specification"), NULL },
  { "mismatch",'U', POPT_BIT_SET, &mtreeFlags, (MTREE_FLAGS_UPDATE|MTREE_FLAGS_MISMATCHOK),
	N_("Same as -u, but ignore match status on exit"), NULL },
  { "warn",'w', POPT_BIT_SET,		&mtreeFlags, MTREE_FLAGS_WARN,
	N_("Treat missing uid/gid as warning"), NULL },
    /* XXX duplicated with --xdev. */
  { "xdev",'x', POPT_BIT_SET,  &rpmioFtsOpts, FTS_XDEV,
	N_("Don't cross mount points"), NULL },

 { NULL, '\0', POPT_ARG_INCLUDE_TABLE, rpmioFtsPoptTable, 0,
	N_("Fts(3) traversal options:"), NULL },

 { NULL, '\0', POPT_ARG_INCLUDE_TABLE, rpmioDigestPoptTable, 0,
	N_("Available digests:"), NULL },

 { NULL, '\0', POPT_ARG_INCLUDE_TABLE, rpmioAllPoptTable, 0,
	N_("Common options for all rpmio executables:"),
	NULL },

  POPT_AUTOALIAS
  POPT_AUTOHELP

  { NULL, (char)-1, POPT_ARG_INCLUDE_TABLE, NULL, 0,
        "\
Usage: mtree [-cdeilnqrtUux] [-f spec] [-K key] [-k key] [-p path] [-s seed]\n\
", NULL },

  POPT_TABLEEND
};

#if defined(__linux__)
/*@null@*/ /*@observer@*/
static const char *my_getlogin(void)
	/*@globals fileSystem @*/
	/*@modifies fileSystem @*/
{
    const char *s = getlogin();

    if (s && *s) {
	return (char *) s;
    } else {
	struct passwd *pw = getpwuid(geteuid());
	char *ss = NULL;
/*@-unrecog@*/
	if (pw != NULL && pw->pw_name != NULL && pw->pw_name[0] != '\0') {
	    if (asprintf(&ss, _("(no controlling terminal) %s"), pw->pw_name) < 0) {
		perror("asprintf");
		return NULL;
	    }
	} else {
	    if (asprintf(&ss, _("(no controlling terminal) #%d"), geteuid()) < 0) {
		perror("asprintf");
		return NULL;
	    }
	}
/*@=unrecog@*/
	return ss;
    }
}
#define	__getlogin	my_getlogin
#else
#define	__getlogin	getlogin
#endif

int
main(int argc, char *argv[])
	/*@globals _rpmfts, mtreeFlags, excludes, __assert_program_name,
		rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
	/*@modifies _rpmfts, mtreeFlags, excludes, __assert_program_name,
		rpmGlobalMacroContext, fileSystem, internalState @*/
{
    rpmfts fts = _rpmfts;
    poptContext optCon;
    int rc = 1;		/* assume failure */
    int i;

    __progname = "mtree";

    RPM_LIST_INIT(&excludes);
    fts->keys = KEYDEFAULT;
    fts->maxg = 5000;
    fts->maxu = 5000;
    fts->maxm = (MBITS + 1);
#if defined(HAVE_STRUCT_STAT_ST_FLAGS)
    fts->maxf = 256;
    fts->sb.st_flags = 0xffffffff;
#endif

    /* Process options. */
    optCon = rpmioInit(argc, argv, optionsTable);

    /* XXX ./rpmmtree w no args waits for stdin. poptPrintUsage more better. */
    argv = (char **) poptGetArgs(optCon);
    if (!(argv == NULL || argv[0] == NULL)) {
	poptPrintUsage(optCon, stderr, 0);
	goto exit;
    }

    if (MF_ISSET(LOOSE) && MF_ISSET(UPDATE))
	mtree_error("-l and -u flags are mutually exclusive");

    /*
     * Either FTS_PHYSICAL or FTS_LOGICAL must be set. Don't follow symlinks
     * unless explicitly overridden with FTS_LOGICAL.
     */
    fts->ftsoptions = rpmioFtsOpts;
    switch (fts->ftsoptions & (FTS_LOGICAL|FTS_PHYSICAL)) {
    case (FTS_LOGICAL|FTS_PHYSICAL):
	mtree_error("-L and -P flags are mutually exclusive");
	/*@notreached@*/ break;
    case 0:
	fts->ftsoptions |= FTS_PHYSICAL;
	break;
    }

    if (fts->paths == NULL || fts->paths[0] == NULL) {
	fts->paths = xcalloc(2, sizeof(fts->paths[0]));
	fts->paths[0] = xstrdup(".");
    }

    /* Use absolute paths since Chdir(2) is problematic with remote URI's */
    for (i = 0; fts->paths[i] != NULL; i++) {
	char fullpath[MAXPATHLEN];
	struct stat sb;
	const char * rpath;
	const char * lpath = NULL;
	int ut = urlPath(fts->paths[i], &lpath);
	size_t nb = (size_t)(lpath - fts->paths[i]);
	int isdir = (lpath[strlen(lpath)-1] == '/');
	
	/* Convert to absolute/clean/malloc'd path. */
	if (lpath[0] != '/') {
	    /* XXX GLIBC: realpath(path, NULL) return malloc'd */
	    rpath = Realpath(lpath, NULL);
	    if (rpath == NULL)
		rpath = Realpath(lpath, fullpath);
	    if (rpath == NULL)
		mtree_error("Realpath(%s): %s", lpath, strerror(errno));
	    lpath = rpmGetPath(rpath, NULL);
	    if (rpath != fullpath)	/* XXX GLIBC extension malloc's */
		rpath = _free(rpath);
	} else
	    lpath = rpmGetPath(lpath, NULL);

	/* Reattach the URI to the absolute/clean path. */
	/* XXX todo: rpmGenPath was confused by file:///path/file URI's. */
	switch (ut) {
	case URL_IS_DASH:
	case URL_IS_UNKNOWN:
	    rpath = lpath;
	    lpath = NULL;
	    /*@switchbreak@*/ break;
	default:
	    strncpy(fullpath, fts->paths[i], nb);
	    fullpath[nb] = '\0';
	    rpath = rpmGenPath(fullpath, lpath, NULL);
	    lpath = _free(lpath);
	    /*@switchbreak@*/ break;
	}

	/* Add a trailing '/' on directories. */
	lpath = (isdir || (!Stat(rpath, &sb) && S_ISDIR(sb.st_mode))
		? "/" : NULL);
	fts->paths[i] = _free(fts->paths[i]);
	fts->paths[i] = rpmExpand(rpath, lpath, NULL);
	fts->fullpath = xstrdup(fts->paths[i]);
	rpath = _free(rpath);
    }

    /* XXX prohibits -s 0 invocation */
    if (fts->crc_total)
	mtreeFlags |= MTREE_FLAGS_SEEDED;
    fts->crc_total ^= 0xffffffff;

    (void) rpmswEnter(&dc_totalops, -1);

    if (MF_ISSET(CREATE)) {
	    if (!MF_ISSET(NOCOMMENT)) {
		time_t clock;
		char host[MAXHOSTNAMELEN];

		(void) time(&clock);
		(void) gethostname(host, sizeof(host));
		(void) printf("#\t   user: %s\n", __getlogin());
		(void) printf("#\tmachine: %s\n", host);
		for (i = 0; fts->paths[i] != NULL; i++)
		    (void) printf("#\t   tree: %s\n", fts->paths[i]);
		(void) printf("#\t   date: %s", ctime(&clock));
	    }
	    rc = mtreeCWalk(fts);
	    if (MF_ISSET(SEEDED) && KF_ISSET(fts->keys, CKSUM))
		(void) fprintf(stderr, _("%s: %s checksum: %u\n"), __progname,
			fts->fullpath, (unsigned)fts->crc_total);

    } else {
	if (fts->spec2 != NULL) {
	    rc = mtreeVSpec(fts);
	} else {
/*@-evalorder@*/
	    fts->root = mtreeSpec(fts, fts->spec1);
/*@=evalorder@*/
	    fts->path = xmalloc(MAXPATHLEN);
	    rc = mtreeVWalk(fts);
	    mtreeMiss(fts, fts->root, fts->path);
	    fts->path = _free(fts->path);
	    if (MF_ISSET(SEEDED))
		(void) fprintf(stderr, _("%s: %s checksum: %u\n"), __progname,
			fts->fullpath, (unsigned) fts->crc_total);
	}
    }
    if (MF_ISSET(MISMATCHOK) && (rc == MISMATCHEXIT))
	rc = 0;

    (void) rpmswExit(&dc_totalops, 0);
    if (_rpmsw_stats) {
        rpmswPrint(" total:", &dc_totalops, NULL);
        rpmswPrint("  read:", &dc_readops, NULL);
        rpmswPrint("digest:", &dc_digestops, NULL);
    }

exit:
#if defined(_RPMFI_INTERNAL)
    (void)rpmtsFree(fts->ts); 
    fts->ts = NULL;
    fts->fi = rpmfiFree(fts->fi);
    tagClean(NULL);     /* Free header tag indices. */
#endif
    if (fts->spec1 != NULL && fileno(fts->spec1) > 2) {
	(void) fclose(fts->spec1);
	fts->spec1 = NULL;
    }
    if (fts->spec2 != NULL && fileno(fts->spec2) > 2) {
	(void) fclose(fts->spec2);
	fts->spec2 = NULL;
    }
    fts->paths = argvFree(fts->paths);
#if defined(HAVE_STRUCT_STAT_ST_FLAGS)
    fts->f = _free(fts->f);
#endif
    fts->g = _free(fts->g);
    fts->m = _free(fts->m);
    fts->u = _free(fts->u);
    fts->fullpath = _free(fts->fullpath);
    /* XXX TODO: clean excludes */

    optCon = rpmioFini(optCon);

    return rc;
}
