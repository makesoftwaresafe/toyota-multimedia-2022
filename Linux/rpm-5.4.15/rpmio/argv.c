/** \ingroup rpmio
 * \file rpmio/argv.c
 */

#include "system.h"

#include "rpmio_internal.h"	/* XXX fdGetFILE() */
#include <argv.h>

#include "debug.h"

/*@access FD_t @*/		/* XXX fdGetFILE() */

#ifdef __cplusplus
GENfree(ARGint_t)
GENfree(ARGI_t)
#endif	/* __cplusplus */

void argvPrint(const char * msg, ARGV_t argv, FILE * fp)
{
    ARGV_t av;

    if (fp == NULL) fp = stderr;

    if (msg)
	fprintf(fp, "===================================== %s\n", msg);

    if (argv)
    for (av = argv; *av; av++)
	fprintf(fp, "\t%s\n", *av);

}

ARGI_t argiFree(ARGI_t argi)
{
    if (argi) {
	argi->nvals = 0;
	argi->vals = _free(argi->vals);
    }
    argi = _free(argi);
    return NULL;
}

ARGV_t argvFree(/*@only@*/ /*@null@*/ ARGV_t argv)
{
    ARGV_t av;
    
    if (argv)
    for (av = argv; *av; av++)
	*av = _free(*av);
    argv = _free(argv);
    return NULL;
}

int argiCount(ARGI_t argi)
{
    int nvals = 0;
    if (argi)
	nvals = argi->nvals;
    return nvals;
}

ARGint_t argiData(ARGI_t argi)
{
    ARGint_t vals = NULL;
    if (argi && argi->nvals > 0)
	vals = argi->vals;
    return vals;
}

int argvCount(const ARGV_t argv)
{
    int argc = 0;
    if (argv)
    while (argv[argc] != NULL)
	argc++;
    return argc;
}

ARGV_t argvData(ARGV_t argv)
{
/*@-retalias -temptrans @*/
    return argv;
/*@=retalias =temptrans @*/
}

int argiCmp(ARGint_t * a, ARGint_t * b)
{
    unsigned aint = *(ARGint_t)a;
    unsigned bint = *(ARGint_t)b;
    return ((aint < bint) ? -1 :
	    (aint > bint) ? +1 : 0);
}

int argvCmp(ARGstr_t * a, ARGstr_t * b)
{
    ARGstr_t astr = *(ARGV_t)a;
    ARGstr_t bstr = *(ARGV_t)b;
    return strcmp(astr, bstr);
}

int argvStrcasecmp(ARGstr_t * a, ARGstr_t * b)
{
    ARGstr_t astr = *(ARGV_t)a;
    ARGstr_t bstr = *(ARGV_t)b;
    return xstrcasecmp(astr, bstr);
}

#if defined(RPM_VENDOR_OPENPKG) /* wildcard-matching-arbitrary-tagnames */
int argvFnmatch(ARGstr_t * a, ARGstr_t * b)
{
    ARGstr_t astr = *(ARGV_t)a;
    ARGstr_t bstr = *(ARGV_t)b;
    return (fnmatch(astr, bstr, 0) == 0 ? 0 : 1);
}

int argvFnmatchCasefold(ARGstr_t * a, ARGstr_t * b)
{
    ARGstr_t astr = *(ARGV_t)a;
    ARGstr_t bstr = *(ARGV_t)b;
    return (fnmatch(astr, bstr, FNM_CASEFOLD) == 0 ? 0 : 1);
}
#endif

int argiSort(ARGI_t argi, int (*compar)(ARGint_t *, ARGint_t *))
{
    unsigned nvals = argiCount(argi);
    ARGint_t vals = argiData(argi);
    if (compar == NULL)
	compar = argiCmp;
    if (nvals > 1)
	qsort(vals, nvals, sizeof(*vals),
		(int(*)(const void *, const void *))compar);
    return 0;
}

int argvSort(ARGV_t argv, int (*compar)(ARGstr_t *, ARGstr_t *))
{
    if (compar == NULL)
	compar = argvCmp;
    qsort(argv, argvCount(argv), sizeof(*argv),
		(int(*)(const void *, const void *))compar);
    return 0;
}

ARGV_t argvSearch(ARGV_t argv, ARGstr_t val,
		int (*compar)(ARGstr_t *, ARGstr_t *))
{
    if (argv == NULL)
	return NULL;
    if (compar == NULL)
	compar = argvCmp;
    return (ARGV_t) bsearch(&val, argv, argvCount(argv), sizeof(*argv),
		(int(*)(const void *, const void *))compar);
}

#if defined(RPM_VENDOR_OPENPKG) /* wildcard-matching-arbitrary-tagnames */
ARGV_t argvSearchLinear(ARGV_t argv, ARGstr_t val,
		int (*compar)(ARGstr_t *, ARGstr_t *))
{
    ARGV_t result;
    ARGV_t av;
    if (argv == NULL)
        return NULL;
    if (compar == NULL)
        compar = argvCmp;
    result = NULL;
    for (av = argv; *av != NULL; av++) {
        if (compar(av, &val) == 0) {
            result = av;
            break;
        }
    }
    return result;
}
#endif

int argiAdd(/*@out@*/ ARGI_t * argip, int ix, int val)
{
    ARGI_t argi;

    if (argip == NULL)
	return -1;
    if (*argip == NULL)
	*argip = (ARGI_t) xcalloc(1, sizeof(**argip));
    argi = *argip;
    if (ix < 0)
	ix = argi->nvals;
    if (ix >= (int)argi->nvals) {
	argi->vals = (ARGint_t) xrealloc(argi->vals, (ix + 1) * sizeof(*argi->vals));
	memset(argi->vals + argi->nvals, 0,
		(ix - argi->nvals) * sizeof(*argi->vals));
	argi->nvals = ix + 1;
    }
    argi->vals[ix] = val;
    return 0;
}

int argvAdd(/*@out@*/ ARGV_t * argvp, ARGstr_t val)
{
    ARGV_t argv;
    int argc;

    if (argvp == NULL)
	return -1;
    argc = argvCount(*argvp);
/*@-unqualifiedtrans@*/
    *argvp = (ARGV_t) xrealloc(*argvp, (argc + 1 + 1) * sizeof(**argvp));
/*@=unqualifiedtrans@*/
    argv = *argvp;
    argv[argc++] = xstrdup(val);
    argv[argc  ] = NULL;
    return 0;
}

int argvAppend(/*@out@*/ ARGV_t * argvp, ARGV_t av)
{
    int ac = argvCount(av);

    if (av != NULL && ac > 0) {
	ARGV_t argv = *argvp;
	int argc = argvCount(argv);

	argv = (ARGV_t) xrealloc(argv, (argc + ac + 1) * sizeof(*argv));
	while (*av++)
	    argv[argc++] = xstrdup(av[-1]);
	argv[argc] = NULL;
	*argvp = argv;
    }
    return 0;
}

int argvSplit(ARGV_t * argvp, const char * str, const char * seps)
{
    static char whitespace[] = " \f\n\r\t\v";
    char * dest = (char *) xmalloc(strlen(str) + 1);
    ARGV_t argv;
    int argc = 1;
    const char * s;
    char * t;
    int c;

    if (seps == NULL)
	seps = whitespace;

    for (argc = 1, s = str, t = dest; (c = (int) *s); s++, t++) {
	if (strchr(seps, c) && !(s[0] == ':' && s[1] == '/' && s[2] == '/')) {
	    argc++;
	    c = (int) '\0';
	}
	*t = (char) c;
    }
    *t = '\0';

    argv = (ARGV_t) xmalloc( (argc + 1) * sizeof(*argv));

    for (c = 0, s = dest; s < t; s += strlen(s) + 1) {
	/* XXX Skip repeated seperators (i.e. whitespace). */
	if (seps == whitespace && s[0] == '\0')
	    continue;
	argv[c++] = xstrdup(s);
    }
    argv[c] = NULL;
    if (argvp)
	*argvp = argv;
    else
	argv = argvFree(argv);
    dest = _free(dest);
/*@-nullstate@*/
    return 0;
/*@=nullstate@*/
}

char * argvJoin(ARGV_t argv, char sep)
{
    size_t nb = 0;
    int argc;
    char *t, *te;

    for (argc = 0; argv[argc] != NULL; argc++) {
	if (argc != 0)
	    nb++;
	nb += strlen(argv[argc]);
    }
    nb++;

    te = t = (char *) xmalloc(nb);
    *te = '\0';
    for (argc = 0; argv[argc] != NULL; argc++) {
	if (argc != 0)
	    *te++ = sep;
	te = stpcpy(te, argv[argc]);
    }
    *te = '\0';
    return t;
}

/*@-mustmod@*/
int argvFgets(ARGV_t * argvp, void * _fd)
{
    FD_t fd = (FD_t) _fd;
    FILE * fp = (fd ? (FILE *) fdGetFILE(fd) : stdin);
    ARGV_t av = NULL;
    char buf[BUFSIZ];
    char * b, * be;
    int rc = 0;

    if (fp == NULL)
	return -2;
    while (!rc && (b = fgets(buf, (int)sizeof(buf), fp)) != NULL) {
	buf[sizeof(buf)-1] = '\0';
	be = b + strlen(buf);
	if (be > b) be--;
	while (strchr("\r\n", *be) != NULL)
	    *be-- = '\0';
	rc = argvAdd(&av, b);
    }

    if (!rc)
	rc = ferror(fp);
    if (!rc)
	rc = (feof(fp) ? 0 : 1);
    if (!rc && argvp)
	*argvp = av;
    else
	av = argvFree(av);
    
/*@-nullstate@*/	/* XXX *argvp may be NULL. */
    return rc;
/*@=nullstate@*/
}
/*@=mustmod@*/
