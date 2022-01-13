#include "system.h"
/*@unchecked@*/
extern const char * __progname;

#define	_RPMIOB_INTERNAL
#include <rpmiotypes.h>
#include <rpmio_internal.h>	/* XXX fdGetFILE */
#include <poptIO.h>
#include "debug.h"

static int _rpmdc_debug = 0;

/* XXX older 0install manifest format. */
static int _old_0install = 0;

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
	/* 18-31 unused */
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

/**
 */
static rpmdc dc = &_dc;

static const char hmackey[] = "orboDeJITITejsirpADONivirpUkvarP";

/*==============================================================*/
static uint32_t rpmdcName2Algo(const char * dname)
	/*@*/
{
    struct poptOption * opt = rpmioDigestPoptTable;
    uint32_t dalgo = 0xffffffff;

    /* XXX compatible with 0install legacy derangement. bug imho. */
    if (!strcmp(dname, "sha1new"))
	dname = "sha1";

    for (; (opt->longName || opt->shortName || opt->arg) ; opt++) {
	if ((opt->argInfo & POPT_ARG_MASK) != POPT_ARG_VAL)
	    continue;
	if (opt->longName == NULL)
	    continue;
	if (!(opt->val > 0 && opt->val < 256))
	    continue;
	if (strcmp(opt->longName, dname))
	    continue;
	dalgo = (uint32_t) opt->val;
	break;
    }
    return dalgo;
}

/*@null@*/
static const char * rpmdcAlgo2Name(uint32_t dalgo)
	/*@*/
{
    struct poptOption * opt = rpmioDigestPoptTable;
    const char * dalgoName = NULL;

    for (; (opt->longName || opt->shortName || opt->arg) ; opt++) {
	if ((opt->argInfo & POPT_ARG_MASK) != POPT_ARG_VAL)
	    continue;
	if (opt->longName == NULL)
	    continue;
	if (!(opt->val > 0 && opt->val < 256))
	    continue;
	if ((uint32_t)opt->val != dalgo)
	    continue;
	dalgoName = opt->longName;
	break;
    }
    return dalgoName;
}

/*==============================================================*/

static int rpmdcParseCoreutils(rpmdc dc)
	/*@globals h_errno, fileSystem, internalState @*/
	/*@modifies h_errno, fileSystem, internalState @*/
{
    int rc = -1;	/* assume failure */

    if (dc->manifests != NULL)	/* note rc=0 return with no files to load. */
    while ((dc->fn = *dc->manifests++) != NULL) {
	char buf[BUFSIZ];
	unsigned lineno;
	FILE *fp;

	if (strcmp(dc->fn, "-") == 0) {
	    dc->fd = NULL;
	    fp = stdin;
	} else {
	    /* XXX .fpio is needed because of fgets(3) usage. */
	    dc->fd = Fopen(dc->fn, "r.fpio");
	    if (dc->fd == NULL || Ferror(dc->fd) || (fp = fdGetFILE(dc->fd)) == NULL) {
		fprintf(stderr, _("%s: Failed to open %s: %s\n"),
				__progname, dc->fn, Fstrerror(dc->fd));
		if (dc->fd != NULL) (void) Fclose(dc->fd);
		dc->fd = NULL;
		fp = NULL;
		goto exit;
	    }
	}

	lineno = 0;
	while (fgets(buf, sizeof(buf), fp) != NULL) {
	    const char * dname, * digest, * path;
	    char *se = buf + (int)strlen(buf);
	    int c, xx;

	    lineno++;
	    while (se > buf && xisspace((int)se[-1]))
		se--;
	    *se = '\0';

	    /* Skip blank lines */
	    if (buf[0] == '\0')	/*@innercontinue@*/ continue;
	    /* Skip comment lines */
	    if (buf[0] == '#')	/*@innercontinue@*/ continue;

	    /* Parse "[algo:]digest [* ]path" line. */
	    dname = NULL; path = NULL;
	    for (digest = se = buf; (c = (int)*se) != 0; se++)
	    switch (c) {
	    default:
		/*@switchbreak@*/ break;
	    case ':':
		*se++ = '\0';
		dname = digest;
		digest = se;
		/*@switchbreak@*/ break;
	    case ' ':
		se[0] = '\0';	/* loop will terminate */
		if (se[1] == ' ' || se[1] == '*')
		    se[1] = '\0';
		path = se + 2;
		/*@switchbreak@*/ break;
	    }
	    if (path == NULL) {
		fprintf(stderr, _("%s: %s line %u: No file path found.\n"),
				__progname, dc->fn, lineno);
		rc = 2;
		goto exit;
	    }

	    /* Map name to algorithm number. */
	    if (dname) {
		if ((dc->dalgo = rpmdcName2Algo(dname)) != 0xffffffff)
		    dc->dalgoName = xstrdup(dname);
		if (dc->dalgo == 0xffffffff) {
		    fprintf(stderr, _("%s: %s line %u: Unknown digest name \"%s\"\n"),
				__progname, dc->fn, lineno, dname);
		    rc = 2;
		    goto exit;
		}
	    } else
		dc->dalgo = dc->algo;

	    /* Save {algo, digest, path} for processing. */
	    xx = argiAdd(&dc->algos, -1, dc->dalgo);
	    xx = argvAdd(&dc->digests, digest);
	    xx = argvAdd(&dc->paths, path);
	}

	if (dc->fd != NULL) {
	    (void) Fclose(dc->fd);
	    dc->fd = NULL;
	}
    }
    rc = 0;

exit:
    return rc;
}

/*@null@*/
static const char * rpmdcPrintCoreutils(rpmdc dc, int rc)
{
    const char *msg = (rc ? "FAILED" : "OK");
    char * t, * te;
    size_t nb = 0;

    /* Don't bother formatting if no one cares. */
    if (rc == 0 && F_ISSET(dc, STATUS))
	return NULL;

    /* Calculate size of message. */
    if (dc->dalgoName != NULL)
	nb += strlen(dc->dalgoName) + sizeof(":") - 1;
assert(dc->digest != NULL);
    if (dc->digest != NULL && dc->digestlen > 0)
	nb += dc->digestlen;
    nb += sizeof(" *") - 1;
    if (dc->fn != NULL)
	nb += strlen(dc->fn);
    nb += strlen(msg);
    nb += sizeof("\n");		/* XXX trailing NUL */

    /* Compose the message. */
    te = t = xmalloc(nb);
    *te = '\0';

    if (dc->manifests) {
	if (rc || !F_ISSET(dc, STATUS)) {
	    if (dc->fn)
		te = stpcpy( stpcpy(te, dc->fn), ": ");
	    te = stpcpy(te, msg);
	    *te++ = '\n';
	}
    } else {
	if (dc->dalgoName)
	    te = stpcpy( stpcpy(te, dc->dalgoName), ":");
	te = stpcpy(te, dc->digest);
	*te++ = ' ';
	*te++ = (F_ISSET(dc, BINARY) ? '*' : ' ');
assert(dc->fn);	/* XXX coverity #1035807 */
	te = stpcpy(te, dc->fn);
	*te++ = '\n';
    }
    *te = '\0';

    return t;
}

/*==============================================================*/

static int rpmdcParseZeroInstall(rpmdc dc)
	/*@globals h_errno, fileSystem, internalState @*/
	/*@modifies h_errno, fileSystem, internalState @*/
{
    int rc = 0;	/* assume success */

    if (dc->manifests != NULL)	/* note rc=0 return with no files to load. */
    while ((dc->fn = *dc->manifests++) != NULL) {
	unsigned lineno;
	char * be;
	rpmiob iob = NULL;
	int xx = rpmiobSlurp(dc->fn, &iob);
	const char * digest;
	char * f;
	char * fe;

	if (!(xx == 0 && iob != NULL)) {
	    fprintf(stderr, _("%s: Failed to open %s\n"), __progname, dc->fn);
	    rc = -1;
	    goto bottom;
	}

	be = (char *)(iob->b + iob->blen);
	while (be > (char *)iob->b && (be[-1] == '\n' || be[-1] == '\r')) {
	  be--;
	  *be = '\0';
	}

	/* Parse "algo=digest" from last line. */
	be = strrchr((char *)iob->b, '=');
	if (be == NULL) {
	    fprintf(stderr,
		_("%s: %s: Manifest needs \"algo=digest\" as last line\n"),
		__progname, dc->fn);
	    rc = 2;
	    goto bottom;
	}
	*be = '\0';
	dc->digest = be + 1;
	while (be > (char *)iob->b && !(be[-1] == '\n' || be[-1] == '\r'))
	    be--;
	if (be <= (char *)iob->b) {
	    fprintf(stderr, _("%s: %s: Manifest is empty\n"),
		__progname, dc->fn);
	    rc = 2;
	    goto bottom;
	}

	/* Map name to algorithm number. */
	if ((dc->dalgo = rpmdcName2Algo(be)) == 0xffffffff) {
	    fprintf(stderr, _("%s: %s: Unknown digest algo name \"%s\"\n"),
			__progname, dc->fn, be);
	    rc = 2;
	    goto bottom;
	}
	*be = '\0';

	/* Verify the manifest digest. */
	{   DIGEST_CTX ctx = rpmDigestInit(dc->dalgo, 0);

	    if (F_ISSET(dc, HMAC))
		(void) rpmHmacInit(ctx, hmackey, 0);

	    (void) rpmDigestUpdate(ctx, (char *)iob->b, (be - (char *)iob->b));
	    digest = NULL;
	    (void) rpmDigestFinal(ctx, &digest, NULL, 1);
	    if (strcmp(dc->digest, digest)) {
		fprintf(stderr,
			_("%s: %s: Manifest digest check: Expected(%s) != (%s)\n"),
			__progname, dc->fn, dc->digest, digest);
		rc = 2;
		goto bottom;
	    }
	    digest = _free(digest);
	}

	/* Parse and save manifest items. */
	lineno = 0;
	for (f = (char *)iob->b; *f; f = fe) {
	    static const char hexdigits[] = "0123456789ABCDEFabcdef";
	    const char * _dn = NULL;
	    const char * path;

	    lineno++;
	    fe = f;
	    while (*fe && !(*fe == '\n' || *fe == '\r'))
		fe++;
	    while (*fe && (*fe == '\n' || *fe == '\r'))
		*fe++ = '\0';
	    switch ((int)*f) {
	    case 'D':
		_dn = f + 2;
		continue;
		/*@notreached@*/ break;
	    case 'F':
	    case 'S':
	    case 'X':
		digest = f + 2;
		f += 2;
		while (*f && strchr(hexdigits, *f) != NULL)
		    f++;
		if (*f != ' ') {
		    fprintf(stderr, _("%s: %s line %u: Malformed digest field.\n"),
			__progname, dc->fn, lineno);
		    rc = 2;
		    goto bottom;
		}
		*f++ = '\0';
		while (*f && xisdigit(*f))
		    f++;
		if (*f != ' ') {
		    fprintf(stderr, _("%s: %s line %u: Malformed mtime field.\n"),
			__progname, dc->fn, lineno);
		    rc = 2;
		    goto bottom;
		}
		*f++ = '\0';
		while (*f && xisdigit(*f))
		    f++;
		if (*f != ' ') {
		    fprintf(stderr, _("%s: %s line %u: Malformed size field.\n"),
			__progname, dc->fn, lineno);
		    rc = 2;
		    goto bottom;
		}
		*f++ = '\0';
		if (*f == '\0') {
		    fprintf(stderr, _("%s: %s line %u: No file path.\n"),
			__progname, dc->fn, lineno);
		    rc = 2;
		    goto bottom;
		}

		if (_dn && *_dn == '/')
		    path = rpmExpand(_dn+1, "/", f, NULL);
		else
		    path = xstrdup(f);

		/* Save {algo, digest, path} for processing. */
		xx = argiAdd(&dc->algos, -1, dc->dalgo);
		xx = argvAdd(&dc->digests, digest);
		xx = argvAdd(&dc->paths, path);
		path = _free(path);
		break;
	    }
	}

bottom:
	iob = rpmiobFree(iob);
	if (rc != 0)
	    goto exit;
    }

exit:
    return rc;
}

/*@null@*/
static const char * rpmdcPrintZeroInstall(rpmdc dc, int rc)
{
    char * t, * te;
    size_t nb = 0;
    char _mtime[32];
    char _size[32];
    const struct stat * st = &dc->sb;
    const char * _bn;

    /* Don't bother formatting if no one cares. */
    if (rc == 0 && F_ISSET(dc, STATUS))
	return NULL;

    snprintf(_mtime, sizeof(_mtime), "%llu",
		(unsigned long long) st->st_mtime);
    _mtime[sizeof(_mtime)-1] = '\0';
    snprintf(_size, sizeof(_size), "%llu",
		(unsigned long long) st->st_size);
    _size[sizeof(_size)-1] = '\0';
assert(dc->fn);		/* XXX coverity #1035950 */
    if ((_bn = strrchr(dc->fn, '/')) != NULL)
	_bn++;
    else
	_bn = dc->fn;

    /* Calculate size of message. */
    nb += sizeof("F");
    if (dc->digest != NULL && dc->digestlen > 0)
	nb += 1 + dc->digestlen;
    nb += 1 + strlen(_mtime);
    nb += 1 + strlen(_size);
    nb += 1 + strlen(_bn);
    nb += sizeof("\n");		/* XXX trailing NUL */
    
    /* Compose the message. */
    te = t = xmalloc(nb);
    *te = '\0';

    if (dc->manifests) {
	const char *msg = (rc ? "FAILED" : "OK");
	if (rc || !F_ISSET(dc, STATUS)) {
	    te = stpcpy( stpcpy(te, dc->fn), ": ");
	    te = stpcpy(te, msg);
	    *te++ = '\n';
	}
    } else {
	if (S_ISDIR(st->st_mode)) {
	    *te++ = 'D';
	    if (_old_0install) {
		*te++ = ' ';
		te = stpcpy(te, _mtime);
	    }
	    *te++ = ' ';
	    *te++ = '/';
	    te = stpcpy(te, _bn);
	    *te++ = '\n';
	} else if (S_ISREG(st->st_mode) || S_ISLNK(st->st_mode)) {
	    if (S_ISLNK(st->st_mode))
		*te++ = 'S';
	    else
		*te++ = (st->st_mode & (S_IXUSR|S_IXGRP|S_IXOTH)) ? 'X' : 'F';
	    *te++ = ' ';
assert(dc->digest);	/* XXX coverity #1035808 */
	    te = stpcpy(te, dc->digest);
	    *te++ = ' ';
	    te = stpcpy(te, _mtime);
	    *te++ = ' ';
	    te = stpcpy(te, _size);
	    *te++ = ' ';
	    te = stpcpy(te, _bn);
	    *te++ = '\n';
	}
    }
    *te = '\0';

    return t;
}

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

static int
rpmdcSortLexical(const FTSENT ** a, const FTSENT ** b)
	/*@*/
{
    return strcmp((*a)->fts_name, (*b)->fts_name);
}

static int
rpmdcSortDirsLast(const FTSENT ** a, const FTSENT ** b)
	/*@*/
{
    if (S_ISDIR((*a)->fts_statp->st_mode)) {
	if (!S_ISDIR((*b)->fts_statp->st_mode))
	    return 1;
    } else if (S_ISDIR((*b)->fts_statp->st_mode))
	return -1;
    return strcmp((*a)->fts_name, (*b)->fts_name);
}

static int
rpmdcCWalk(rpmdc dc)
{
    char *const * paths = (char * const *) dc->paths;
    int ftsoptions = dc->ftsoptions;
    int rval = 0;

    dc->t = Fts_open(paths, ftsoptions,
	(F_ISSET(dc, 0INSTALL) && _old_0install ? rpmdcSortLexical : rpmdcSortDirsLast));
    if (dc->t == NULL) {
	fprintf(stderr, "Fts_open: %s", strerror(errno));
	return -1;
    }

    while ((dc->p = Fts_read(dc->t)) != NULL) {
#ifdef	NOTYET
	int indent = 0;
	if (F_ISSET(dc, INDENT))
	    indent = dc->p->fts_level * 4;
	if (rpmdcCheckExcludes(dc->p->fts_name, dc->p->fts_path)) {
	    (void) Fts_set(dc->t, dc->p, FTS_SKIP);
	    continue;
	}
#endif

	dc->fn = dc->p->fts_path;	/* XXX eliminate dc->fn */
	memcpy(&dc->sb, dc->p->fts_statp, sizeof(dc->sb));

	switch(dc->p->fts_info) {
	case FTS_D:
#ifdef	NOTYET
	    if (!F_ISSET(dc, DIRSONLY))
		(void) printf("\n");
	    if (!F_ISSET(dc, NOCOMMENT))
		(void) printf("# %s\n", dc->p->fts_path);
	    (void) rpmdcVisitD(dc);
#endif
	    /* XXX don't visit topdirs for 0install. */
	    if (F_ISSET(dc, 0INSTALL) && dc->p->fts_level > 0)
		rpmdcVisitF(dc);
	    /*@switchbreak@*/ break;
	case FTS_DP:
#ifdef	NOTYET
	    if (!F_ISSET(dc, NOCOMMENT) && (dc->p->fts_level > 0))
		(void) printf("%*s# %s\n", indent, "", dc->p->fts_path);
	    (void) printf("%*s..\n", indent, "");
	    if (!F_ISSET(dc, DIRSONLY))
		(void) printf("\n");
#endif
	    /*@switchbreak@*/ break;
	case FTS_DNR:
	case FTS_ERR:
	case FTS_NS:
	    (void) fprintf(stderr, "%s: %s: %s\n", __progname,
			dc->p->fts_path, strerror(dc->p->fts_errno));
	    /*@switchbreak@*/ break;
	default:
	    if (!F_ISSET(dc, DIRSONLY))
		rpmdcVisitF(dc);
	    /*@switchbreak@*/ break;
	}
    }
    (void) Fts_close(dc->t);
    dc->p = NULL;
    dc->t = NULL;
    return rval;
}

static int rpmdcLoadManifests(rpmdc dc)
	/*@globals h_errno, fileSystem, internalState @*/
	/*@modifies dc, h_errno, fileSystem, internalState @*/
{
    return (dc->manifests != NULL ? (*dc->parse) (dc) : 0);
}

#if !defined(POPT_ARG_ARGV)
static int _poptSaveString(const char ***argvp, unsigned int argInfo, const char * val)
	/*@*/
{
    ARGV_t argv;
    int argc = 0;
    if (argvp == NULL)
	return -1;
    if (*argvp)
    while ((*argvp)[argc] != NULL)
	argc++;
    *argvp = xrealloc(*argvp, (argc + 1 + 1) * sizeof(**argvp));
    if ((argv = *argvp) != NULL) {
	argv[argc++] = xstrdup(val);
	argv[argc  ] = NULL;
    }
    return 0;
}

/**
 */
static void rpmdcArgCallback(poptContext con,
                /*@unused@*/ enum poptCallbackReason reason,
                const struct poptOption * opt, /*@unused@*/ const char * arg,
                /*@unused@*/ void * data)
	/*@globals fileSystem @*/
	/*@modifies fileSystem @*/
{
    /* XXX avoid accidental collisions with POPT_BIT_SET for flags */
    if (opt->arg == NULL)
    switch (opt->val) {
	int xx;
    case 'c':
assert(arg != NULL);
	xx = _poptSaveString(&_dc.manifests, opt->argInfo, arg);
	break;

    default:
	fprintf(stderr, _("%s: Unknown option -%c\n"), __progname, opt->val);
	poptPrintUsage(con, stderr, 0);
/*@-exitarg@*/
	exit(2);
/*@=exitarg@*/
	/*@notreached@*/ break;
    }
}
#endif	/* POPT_ARG_ARGV */

static struct poptOption _optionsTable[] = {
#if !defined(POPT_ARG_ARGV)
/*@-type@*/ /* FIX: cast? */
 { NULL, '\0', POPT_ARG_CALLBACK | POPT_CBFLAG_INC_DATA | POPT_CBFLAG_CONTINUE,
        rpmdcArgCallback, 0, NULL, NULL },
/*@=type@*/
#endif	/* POPT_ARG_ARGV */

  { "0install", '0', POPT_BIT_SET,	&_dc.flags, RPMDC_FLAGS_0INSTALL,
	N_("Print 0install manifest"), NULL },

  { "binary", 'b', POPT_BIT_SET,	&_dc.flags, RPMDC_FLAGS_BINARY,
	N_("Read in binary mode"), NULL },

#if !defined(POPT_ARG_ARGV)
  { "check", 'c', POPT_ARG_STRING,	NULL, 'c',
	N_("Read digests from MANIFEST file and verify (may be used more than once)"),
	N_("MANIFEST") },
#else
  { "check", 'c', POPT_ARG_ARGV,	&_dc.manifests, 0,
	N_("Read digests from MANIFEST file and verify (may be used more than once)"),
	N_("MANIFEST") },
#endif
  { "create",'c', POPT_BIT_SET,         &_dc.flags, RPMDC_FLAGS_CREATE,
        N_("Print file tree specification to stdout"), NULL },
  { "dirs",'d', POPT_BIT_SET,           &_dc.flags, RPMDC_FLAGS_DIRSONLY,
        N_("Directories only"), NULL },

  { "text", 't', POPT_BIT_CLR,		&_dc.flags, RPMDC_FLAGS_BINARY,
	N_("read in text mode (default)"), NULL },

  { "hmac", '\0', POPT_BIT_SET,	&_dc.flags, RPMDC_FLAGS_HMAC,
	N_("generate HMAC's instead"), NULL },

#ifdef	NOTYET		/* XXX todo for popt-1.15 */
  { NULL, -1, POPT_ARG_INCLUDE_TABLE, NULL, 0,
        N_("\
The following two options are useful only when verifying digests:\
"), NULL },
#endif

  { "status", '\0', POPT_BIT_SET,	&_dc.flags, RPMDC_FLAGS_STATUS,
	N_("no output when verifying"), NULL },
  { "warn", 'w', POPT_BIT_SET,		&_dc.flags, RPMDC_FLAGS_WARN,
	N_("warn about improperly formatted checksum lines"), NULL },

 { NULL, '\0', POPT_ARG_INCLUDE_TABLE, rpmioDigestPoptTable, 0,
	N_("Available digests:"), NULL },

 { NULL, '\0', POPT_ARG_INCLUDE_TABLE, rpmioAllPoptTable, 0,
	N_("Common options for all rpmio executables:"), NULL },

  POPT_AUTOALIAS
  POPT_AUTOHELP

  { NULL, -1, POPT_ARG_INCLUDE_TABLE, NULL, 0,
        N_("\
When checking, the input should be a former output of this program.  The\n\
default mode is to print a line with digest, a character indicating type\n\
(`*' for binary, ` ' for text), and name for each FILE.\n"), NULL },

  POPT_TABLEEND
};

static struct poptOption *optionsTable = &_optionsTable[0];

int
main(int argc, char *argv[])
{
    poptContext optCon = rpmioInit(argc, argv, optionsTable);
    ARGV_t av;
    int ac;
    int rc = 0;
    int xx;

    rpmswEnter(&dc->totalops, -1);

    if (F_ISSET(dc, 0INSTALL)) {
	dc->parse = rpmdcParseZeroInstall;
	dc->print = rpmdcPrintZeroInstall;
	if ((int)rpmioDigestHashAlgo < 0)
	    rpmioDigestHashAlgo = PGPHASHALGO_SHA1;
	dc->algo = rpmioDigestHashAlgo;
	dc->oalgo = dc->algo;
	dc->oalgoName = rpmdcAlgo2Name(dc->oalgo);
	if (!strcmp(dc->oalgoName, "sha1"))
	    dc->oalgoName = "sha1new";
    } else {
	dc->parse = rpmdcParseCoreutils;
	dc->print = rpmdcPrintCoreutils;
	if ((int)rpmioDigestHashAlgo < 0)
	    rpmioDigestHashAlgo = PGPHASHALGO_MD5;
	dc->algo = rpmioDigestHashAlgo;
    }

    if (dc->ofn == NULL)
	dc->ofn = "-";
    dc->ftsoptions = rpmioFtsOpts;
    if (!(dc->ftsoptions & (FTS_LOGICAL|FTS_PHYSICAL)))
	dc->ftsoptions |= FTS_PHYSICAL;
    dc->ftsoptions |= FTS_NOCHDIR;

    dc->ofd = Fopen(dc->ofn, "w.ufdio");
    if (F_ISSET(dc, 0INSTALL)) {
	fdInitDigest(dc->ofd, dc->oalgo, 0);
	if (F_ISSET(dc, HMAC))
	    fdInitHmac(dc->ofd, hmackey, 0);
    }

    av = poptGetArgs(optCon);
    ac = argvCount(av);
    if ((ac == 0 && dc->manifests == NULL)
     || (ac >  0 && dc->manifests != NULL))
    {
	poptPrintUsage(optCon, stderr, 0);
	rc = 2;
	goto exit;
    }

    if (dc->manifests != NULL) {
	if ((xx = rpmdcLoadManifests(dc)) != 0)
	    rc = xx;
    } else {
	int i;
	for (i = 0; i < ac; i++)
	    xx = argvAdd(&dc->paths, av[i]);
    }
    if (rc)
	goto exit;

    if (dc->manifests != NULL) {
	dc->ix = 0;
	av = dc->paths;
	if (av != NULL)
	while ((dc->fn = *av++) != NULL) {
	    if ((xx = Lstat(dc->fn, &dc->sb)) != 0
	     || (xx = rpmdcVisitF(dc)) != 0)
		rc = xx;
	    dc->ix++;
	}
    } else {
	if ((xx = rpmdcCWalk(dc)) != 0)
	    rc = xx;
    }

exit:
    if (dc->nfailed)
	fprintf(stderr, "%s: WARNING: %u of %u computed checksums did NOT match\n",
		__progname, (unsigned) dc->nfailed, (unsigned) dc->ncomputed);

    if (dc->ofd) {
	/* Print the output spewage digest for 0install format manifests. */
	if (rc == 0 && F_ISSET(dc, 0INSTALL) && dc->manifests == NULL) {
	    static int asAscii = 1;
	    char *t;
	    fdFiniDigest(dc->ofd, dc->oalgo, &dc->digest, &dc->digestlen, asAscii);
assert(dc->digest != NULL);
	    t = rpmExpand(dc->oalgoName, "=", dc->digest, "\n", NULL);
	    (void) Fwrite(t, strlen(t), sizeof(*t), dc->ofd);
	    t = _free(t);
	    dc->digest = _free(dc->digest);
	}
	(void) Fclose(dc->ofd);
	dc->ofd = NULL;
    }

#ifdef	NOTYET
    dc->manifests = argvFree(dc->manifests);
#endif
    dc->algos = argiFree(dc->algos);
    dc->digests = argvFree(dc->digests);
    dc->paths = argvFree(dc->paths);

    rpmswExit(&dc->totalops, 0);
    if (_rpmsw_stats) {
	rpmswPrint(" total:", &dc->totalops, NULL);
	rpmswPrint("  read:", &dc->readops, NULL);
	rpmswPrint("digest:", &dc->digestops, NULL);
    }

    optCon = rpmioFini(optCon);

    return rc;
}
