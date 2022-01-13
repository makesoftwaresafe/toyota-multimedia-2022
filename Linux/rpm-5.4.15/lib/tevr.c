#include "system.h"

#include <rpmio.h>
#include <poptIO.h>
#include <argv.h>
#include <rpmhash.h>

#include <rpmtag.h>
#define	_RPMEVR_INTERNAL
#include <rpmevr.h>

#include "debug.h"

const char *__progname;
#define	progname	__progname

static struct stats_e {
    unsigned total;
    unsigned files;
    unsigned Kdigest;
    unsigned Odigest;
    unsigned Emiss;
    unsigned Rmiss;
    size_t strnb;
    size_t dictnb;
    size_t uuidnb;
} s;

static int nofiles = 0;
static int noKdigests = 0;
static int noOdigests = 0;

typedef	struct rpmdict_s * rpmdict;
struct rpmdict_s {
    ARGV_t av;
    size_t ac;
    hashTable ht;
};

/*@null@*/
static rpmdict rpmdictFree(/*@only@*/ rpmdict dict)
{
    if (dict != NULL) {
	dict->ht = (dict->ht ? htFree(dict->ht) : NULL);
	dict->av = argvFree(dict->av);
	dict = _free(dict);
    }
    return NULL;
}

static void rpmdictAdd(rpmdict dict, const char * key)
{
    rpmuint64_t * val = NULL;
    void ** data = (void **)&val;
    if (htGetEntry(dict->ht, key, &data, NULL, NULL)) {
	(void) argvAdd(&dict->av, key);
	val = (rpmuint64_t *) xcalloc(1, sizeof(*val));
	htAddEntry(dict->ht, dict->av[dict->ac++], val);
    } else
	val = (rpmuint64_t *)data[0];
    val[0]++;
}

/*@only@*/
static rpmdict rpmdictCreate(void)
{
    static const char key[] = "";
    rpmdict dict = (rpmdict) xcalloc(1, sizeof(*dict));
    int nbuckets = 4093;
    size_t keySize = 0;
    int freeData = 1;
    /* XXX hashEqualityString uses strcmp, perhaps rpmEVRcmp instead? */
    dict->ht = htCreate(nbuckets, keySize, freeData, NULL, NULL);
    rpmdictAdd(dict, key);
    return dict;
}

static int rpmdictCmp(const void * a, const void * b)
{
    const char * astr = *(ARGV_t)a;
    const char * bstr = *(ARGV_t)b;
    return rpmEVRcmp(astr, bstr);
}

static int isKdigest(const char * s)
{
    static char hex[] = "0123456789abcdefABCDEF";
    size_t len = strlen(s);
    int rc = 0;
    int c;

    switch (len) {
    default:
	break;
    case 32:
    case 40:
	while ((c = *s++) != 0)
	    if (strchr(hex, c) == NULL)
		break;
	rc = (c == 0 ? 1 : 0);
	break;
    }
    return rc;
}

static int isOdigest(const char * s)
{
    static char hex[] = "0123456789abcdef";
    size_t len = strlen(s);
    int hascolon = 0;
    int hasdash = 0;
    int rc = 0;
    int c;

    switch (len) {
    default:
	break;
    case 27:
	while ((c = *s++) != 0) {
	    if (c == (int)':') hascolon++;
	    else if (c == (int)'-') hasdash++;
	    else if (strchr(hex, c) == NULL)
		break;
	}
	rc = (c == 0 && hascolon == 1 && hasdash == 1 ? 1 : 0);
	break;
    }
    return rc;
}

static struct poptOption optionsTable[] = {

 { NULL, '\0', POPT_ARG_INCLUDE_TABLE, rpmioAllPoptTable, 0,
	N_("Common options for all rpmio executables:"),
	NULL },

  POPT_AUTOHELP
  POPT_TABLEEND
};

int
main(int argc, char *const argv[])
{
    poptContext optCon = rpmioInit(argc, argv, optionsTable);
    ARGV_t av;
    int ac;
    rpmdict dict;
    EVR_t evr = (EVR_t) xcalloc(1, sizeof(*evr));
    const char * arg;
    int rc = 0;
    int xx;
    int i;

    if ((progname = strrchr(argv[0], '/')) != NULL)
	progname++;
    else
	progname = argv[0];

    av = NULL;
    (void) argvAppend(&av, poptGetArgs(optCon));
    ac = argvCount(av);

    if (ac == 0 || !strcmp(*av, "-")) {
	av = NULL;
	xx = argvFgets(&av, NULL);
	ac = argvCount(av);
    }
    
    dict = rpmdictCreate();

    if (av != NULL)
    for (i = 0; (arg = av[i]) != NULL; i++) {
	if (*arg == '\0')	/* Skip cruft */
	    continue;
	s.total++;

	if (nofiles && *arg == '/') {		/* Skip file paths. */
	    s.files++;
	    continue;
	}
	if (noKdigests && isKdigest(arg)) {	/* Skip kernel MD5/SHA1. */
	    s.Kdigest++;
	    continue;
	}
	if (noOdigests && isOdigest(arg)) {	/* Skip OCAML EVR strings. */
	    s.Odigest++;
	    continue;
	}

	/* Split E:V-R into components. */
	xx = rpmEVRparse(arg, evr);
	if (evr->F[RPMEVR_E] == NULL) {
	    evr->F[RPMEVR_E] = "0";
	    s.Emiss++;
	}
	if (evr->F[RPMEVR_R] == NULL) {
	    evr->F[RPMEVR_R] = "";
	    s.Rmiss++;
	}
	rpmdictAdd(dict, evr->F[RPMEVR_E]);
	rpmdictAdd(dict, evr->F[RPMEVR_V]);
	rpmdictAdd(dict, evr->F[RPMEVR_R]);

if (__debug)
fprintf(stderr, "%5d: %s => %s:%s-%s\n", s.total, arg, evr->F[RPMEVR_E], evr->F[RPMEVR_V], evr->F[RPMEVR_R]);

	evr->str = _free(evr->str);
    }

    (void) argvSort(dict->av,(int (*)(const char **, const char **))rpmdictCmp);

    /* Compute size of string & uuid store. */
    if (av != NULL)
    for (i = 0; av[i] != NULL; i++) {
	s.strnb += sizeof(*av) + strlen(av[i]) + 1;
	s.uuidnb += 64/8;
    }
    s.strnb += sizeof(*av) + 1;

    /* Compute size of dictionary store. */
    for (i = 0; dict->av[i] != NULL; i++) {
	s.dictnb += sizeof(*dict->av) + strlen(dict->av[i]) + 1;
    }
    s.dictnb += sizeof(*dict->av) + 1;

fprintf(stderr, "total:%u files:%u Kdigest:%u Odigest:%u Emiss:%u Rmiss:%u dictlen:%u strnb:%u dictnb:%u uuidnb:%u\n",
s.total, s.files, s.Kdigest, s.Odigest, s.Emiss, s.Rmiss, argvCount(dict->av), (unsigned)s.strnb, (unsigned)s.dictnb, (unsigned)s.uuidnb);

if (__debug)
    argvPrint("E:V-R dictionary", dict->av, NULL);

    evr = _free(evr);
    dict = rpmdictFree(dict);

    av = argvFree(av);
    optCon = rpmioFini(optCon);

    return rc;
}
