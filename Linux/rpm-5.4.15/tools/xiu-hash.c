#include "system.h"
/*@unchecked@*/
extern const char * __progname;

#define	_RPMIOB_INTERNAL
#include <rpmiotypes.h>
#include <rpmio_internal.h>	/* XXX fdGetFILE */
#include <poptIO.h>

#include "debug.h"

static int _rpmdc_debug = 0;

#define _KFB(n) (1U << (n))
#define _DFB(n) (_KFB(n) | 0x40000000)

#define F_ISSET(_dc, _FLAG) ((_dc)->flags & ((RPMDC_FLAGS_##_FLAG) & ~0x40000000))

/**
 * Bit field enum for rpmdigest CLI options.
 */
enum dcFlags_e {
    RPMDC_FLAGS_NONE		= 0,
	/* 0 reserved */
    RPMDC_FLAGS_WARN		= _DFB( 1),	/*!< -w,--warn ... */
    RPMDC_FLAGS_CREATE		= _DFB( 2),	/*!< -c,--create ... */
    RPMDC_FLAGS_DIRSONLY	= _DFB( 3),	/*!< -d,--dirs ... */
	/* 4-13 reserved */
    RPMDC_FLAGS_BINARY		= _DFB(14),	/*!< -b,--binary ... */
    RPMDC_FLAGS_STATUS		= _DFB(15),	/*!<    --status ... */
    RPMDC_FLAGS_0INSTALL	= _DFB(16),	/*!< -0,--0install ... */
    RPMDC_FLAGS_HMAC		= _DFB(17),	/*!<    --hmac ... */

    RPMDC_FLAGS_FLAT		= _DFB(18),	/*!<    --flat */
    RPMDC_FLAGS_BASE32		= _DFB(19),	/*!<    --base32 */
    RPMDC_FLAGS_TRUNCATE	= _DFB(20),	/*!<    --truncate */
    RPMDC_FLAGS_TOBASE16	= _DFB(21),	/*!<    --to-base16 */
    RPMDC_FLAGS_TOBASE32	= _DFB(22),	/*!<    --to-base32 */
	/* 23-31 unused */
};

/**
 */
typedef struct rpmdc_s * rpmdc;

/**
 */
struct rpmdc_s {
    int ftsoptions;		/*!< global Fts(3) traversal options. */
    FTS * t;			/*!< global Fts(3) traversal data. */
    FTSENT * p;			/*!< current node Fts(3) traversal data. */
    struct stat sb;		/*!< current node stat(2) data. */

    enum dcFlags_e flags;	/*!< rpmdc control bits. */
    uint32_t algo;		/*!< default digest algorithm. */
    uint32_t dalgo;		/*!< digest algorithm. */
/*@observer@*/ /*@null@*/
    const char * dalgoName;	/*!< digest algorithm name. */
    const char * digest;
    size_t digestlen;
    const char * fn;
    FD_t fd;
    int (*parse) (rpmdc dc);
    const char * (*print) (rpmdc dc, int rc);
    const char * ofn;		/*!< output file name */
    FD_t ofd;			/*!< output file handle */
    uint32_t oalgo;		/*!< output digest algorithm. */
    const char * oalgoName;	/*!< output digest algorithm name. */
    ARGV_t manifests;		/*!< array of file manifests to verify. */
    ARGI_t algos;		/*!< array of file digest algorithms. */
    ARGV_t digests;		/*!< array of file digests. */
    ARGV_t paths;		/*!< array of file paths. */
    unsigned char buf[BUFSIZ];
    ssize_t nb;
    int ix;

    size_t ncomputed;		/*!< no. of digests computed. */
    size_t nchecked;		/*!< no. of digests checked. */
    size_t nmatched;		/*!< no. of digests matched. */
    size_t nfailed;		/*!< no. of digests failed. */
    struct rpmop_s totalops;
    struct rpmop_s readops;
    struct rpmop_s digestops;
};

/**
 */
static struct rpmdc_s _dc = {
	.ftsoptions = FTS_PHYSICAL,
	.flags = RPMDC_FLAGS_CREATE
};

static const char hmackey[] = "orboDeJITITejsirpADONivirpUkvarP";

static const char * dalgoName;

/*==============================================================*/

static int rpmdcPrintFile(rpmdc dc)
{
    static int asAscii = 1;
    int rc = 0;

if (_rpmdc_debug)
fprintf(stderr, "\t%s(%p) fd %p fn %s\n", __FUNCTION__, dc, dc->fd, dc->fn);

assert(dc->fd != NULL);
    fdFiniDigest(dc->fd, dc->dalgo, &dc->digest, &dc->digestlen, asAscii);
assert(dc->digest != NULL);
    dc->ncomputed++;

    if (dc->manifests) {
	dc->nchecked++;
	if ((rc = strcmp(dc->digest, dc->digests[dc->ix])) != 0)
	    dc->nfailed++;
	else
	    dc->nmatched++;
    }

    {	const char * t = (*dc->print) (dc, rc);
	if (dc->ofd && t && *t) {
	    size_t nb = strlen(t);
	    nb = Fwrite(t, nb, sizeof(*t), dc->ofd);
	    (void) Fflush(dc->ofd);
	}
	t = _free(t);
    }

    dc->digest = _free(dc->digest);
    dc->digestlen = 0;
    return rc;
}

static int rpmdcFiniFile(rpmdc dc)
{
    uint32_t dalgo = (dc->manifests ? dc->algos->vals[dc->ix] : dc->algo);
    int rc = 0;
    int xx;

    /* Only regular files have dc->fd != NULL here. Skip all other paths. */
    if (dc->fd == NULL)
	return rc;

if (_rpmdc_debug)
fprintf(stderr, "\t%s(%p) fn %s\n", __FUNCTION__, dc, dc->fn);
    switch (dalgo) {
    default:
	dc->dalgo = dalgo;
	dc->dalgoName = NULL;
	xx = rpmdcPrintFile(dc);
	if (xx) rc = xx;
	break;
    case 256:		/* --all digests requested. */
      {	struct poptOption * opt = rpmioDigestPoptTable;
	for (; (opt->longName || opt->shortName || opt->arg) ; opt++) {
	    if ((opt->argInfo & POPT_ARG_MASK) != POPT_ARG_VAL)
		continue;
	    if (opt->arg != (void *)&rpmioDigestHashAlgo)
		continue;
	    dc->dalgo = opt->val;
	    if (!(dc->dalgo > 0 && dc->dalgo < 256))
		continue;
	    dc->dalgoName = opt->longName;
	    xx = rpmdcPrintFile(dc);
	    if (xx) rc = xx;
	}
      }	break;
    }

    (void) rpmswAdd(&dc->readops, fdstat_op(dc->fd, FDSTAT_READ));
    (void) rpmswAdd(&dc->digestops, fdstat_op(dc->fd, FDSTAT_DIGEST));
    Fclose(dc->fd);
    dc->fd = NULL;

    return rc;
}

static int rpmdcCalcFile(rpmdc dc)
{
    int rc = 0;

if (_rpmdc_debug)
fprintf(stderr, "\t%s(%p) fn %s\n", __FUNCTION__, dc, dc->fn);
    /* Skip (unopened) non-files. */
    if (dc->fd != NULL)
    do {
	dc->nb = Fread(dc->buf, sizeof(dc->buf[0]), sizeof(dc->buf), dc->fd);
	if (Ferror(dc->fd)) {
	    rc = 2;
	    break;
	}
    } while (dc->nb > 0);

    return rc;
}

static int rpmdcInitFile(rpmdc dc)
{
    int rc = 0;

if (_rpmdc_debug)
fprintf(stderr, "\t%s(%p) fn %s\n", __FUNCTION__, dc, dc->fn);
    /* Skip non-files. */
    if (!S_ISREG(dc->sb.st_mode)) {
	/* XXX not found return code? */
	goto exit;
    }

    dc->fd = Fopen(dc->fn, "r.ufdio");
    if (dc->fd == NULL || Ferror(dc->fd)) {
	fprintf(stderr, _("open of %s failed: %s\n"), dc->fn, Fstrerror(dc->fd));
	if (dc->fd != NULL) Fclose(dc->fd);
	dc->fd = NULL;
	rc = 2;
	goto exit;
    }

    switch (dc->algo) {
    default:
	/* XXX TODO: instantiate verify digests for all identical paths. */
	dc->dalgo = dc->algo;
	fdInitDigest(dc->fd, dc->dalgo, 0);
	if (F_ISSET(dc, HMAC))
	    fdInitHmac(dc->fd, hmackey, 0);
	break;
    case 256:		/* --all digests requested. */
      {	struct poptOption * opt = rpmioDigestPoptTable;
	for (; (opt->longName || opt->shortName || opt->arg) ; opt++) {
	    if ((opt->argInfo & POPT_ARG_MASK) != POPT_ARG_VAL)
		continue;
	    if (opt->longName == NULL)
		continue;
	    if (!(opt->val > 0 && opt->val < 256))
		continue;
	    dc->dalgo = opt->val;
	    dc->dalgoName = opt->longName;
	    fdInitDigest(dc->fd, dc->dalgo, 0);
	    if (F_ISSET(dc, HMAC))
		fdInitHmac(dc->fd, hmackey, 0);
	}
      }	break;
    }

exit:
    return rc;
}

static int
rpmdcVisitF(rpmdc dc)
	/*@modifies dc @*/
{
    int rc = 0;
    int xx;

if (_rpmdc_debug)
fprintf(stderr, "*** %s(%p) fn %s\n", __FUNCTION__, dc, dc->fn);
    if ((xx = rpmdcInitFile(dc)) != 0)
	rc = xx;
    else {
	if ((xx = rpmdcCalcFile(dc)) != 0)
	    rc = xx;
	if ((xx = rpmdcFiniFile(dc)) != 0)
	    rc = xx;
    }
    return rc;
}

/*@null@*/
static const char * rpmdcPrintNix(rpmdc dc, int rc)
{
    char * t, * te;
    size_t nb = 0;

    /* Don't bother formatting if no one cares. */
    if (rc == 0 && F_ISSET(dc, STATUS))
	return NULL;

    /* Calculate size of message. */
assert(dc->digest != NULL);
    if (dc->digest != NULL && dc->digestlen > 0)
	nb += dc->digestlen;
    nb += sizeof("\n");		/* XXX trailing NUL */

    /* Compose the message. */
    te = t = xmalloc(nb);
    *te = '\0';

    te = stpcpy(te, dc->digest);
    *te++ = '\n';
    *te = '\0';

    return t;
}
/*==============================================================*/

static void nixArgCallback(poptContext con,
                /*@unused@*/ enum poptCallbackReason reason,
                const struct poptOption * opt, const char * arg,
                /*@unused@*/ void * data)
	/*@*/
{
    /* XXX avoid accidental collisions with POPT_BIT_SET for flags */
    if (opt->arg == NULL)
    switch (opt->val) {
    default:
	fprintf(stderr, _("%s: Unknown option -%c\n"), __progname, opt->val);
	poptPrintUsage(con, stderr, 0);
	/*@-exitarg@*/ exit(2); /*@=exitarg@*/
	/*@notreached@*/ break;
	break;
    }
}

static struct poptOption optionsTable[] = {
/*@-type@*/ /* FIX: cast? */
 { NULL, '\0', POPT_ARG_CALLBACK | POPT_CBFLAG_INC_DATA | POPT_CBFLAG_CONTINUE,
	nixArgCallback, 0, NULL, NULL },
/*@=type@*/

 { "flat", '\0', POPT_BIT_SET,		&_dc.flags, RPMDC_FLAGS_FLAT,
	N_("compute hash of regular file contents, not metadata"), NULL },
 { "base32", '\0', POPT_BIT_SET,	&_dc.flags, RPMDC_FLAGS_BASE32,
	N_("print hash in base-32 instead of hexadecimal"), NULL },
 { "truncate", '\0', POPT_BIT_SET,	&_dc.flags, RPMDC_FLAGS_TRUNCATE,
	N_("truncate the hash to 160 bits"), NULL },
 { "to-base16", '\0', POPT_BIT_SET,	&_dc.flags, RPMDC_FLAGS_TOBASE16,
	N_("convert output in base16"), NULL },
 { "to-base32", '\0', POPT_BIT_SET,	&_dc.flags, RPMDC_FLAGS_TOBASE32,
	N_("convert output in base32"), NULL },

 { "type", '\0', POPT_ARG_STRING,	&dalgoName, 0,
	N_("use hash algorithm HASH (\"md5\" (default), \"sha1\", \"sha256\")"),
	N_("HASH") },

#ifdef	NOTYET
 { NULL, '\0', POPT_ARG_INCLUDE_TABLE, rpmioDigestPoptTable, 0,
	N_("Available digests:"), NULL },

 { NULL, '\0', POPT_ARG_INCLUDE_TABLE, rpmioAllPoptTable, 0,
	N_("Common options for all rpmio executables:"), NULL },
#endif

  { NULL, (char)-1, POPT_ARG_INCLUDE_TABLE, NULL, 0,
	N_("\
Usage: nix-hash [OPTIONS...] [FILES...]\n\
\n\
`nix-hash' computes and prints cryptographic hashes for the specified\n\
files.\n\
\n\
  --flat: compute hash of regular file contents, not metadata\n\
  --base32: print hash in base-32 instead of hexadecimal\n\
  --type HASH: use hash algorithm HASH (\"md5\" (default), \"sha1\", \"sha256\")\n\
  --truncate: truncate the hash to 160 bits\n\
"), NULL },

  POPT_AUTOHELP
  POPT_TABLEEND
};

int
main(int argc, char *argv[])
{
    rpmdc dc = &_dc;
    poptContext optCon = rpmioInit(argc, argv, optionsTable);
    ARGV_t av = poptGetArgs(optCon);
#ifdef	UNUSED
    int ac = argvCount(av);
#endif
    int ec = 0;		/* assume success */
    int xx;

    if (dalgoName == NULL)
	dc->algo = PGPHASHALGO_MD5;
    else if (!strcasecmp(dalgoName, "md5"))
	dc->algo = PGPHASHALGO_MD5;
    else if (!strcasecmp(dalgoName, "sha1"))
	dc->algo = PGPHASHALGO_SHA1;
    else if (!strcasecmp(dalgoName, "sha256"))
	dc->algo = PGPHASHALGO_SHA256;
    else {
fprintf(stderr, _("Unknown hash algorithm(%s)\n"), dalgoName);
	goto exit;
    }

    dc->print = rpmdcPrintNix;
    dc->ofn = "-";
    dc->ofd = Fopen(dc->ofn, "w.ufdio");

    dc->ix = 0;
    if (av != NULL)
    while ((dc->fn = *av++) != NULL) {
	if ((xx = Lstat(dc->fn, &dc->sb)) != 0
	 || (xx = rpmdcVisitF(dc)) != 0)
	    ec = xx;
	dc->ix++;
    }

exit:
    if (dc->ofd)
	xx = Fclose(dc->ofd);

    optCon = rpmioFini(optCon);

    return ec;
}
