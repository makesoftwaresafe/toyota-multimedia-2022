/** \ingroup rpmio
 * \file rpmio/rpmaug.c
 */

#include "system.h"

#if defined(WITH_AUGEAS) && defined(HAVE_AUGEAS_H)
#ifdef __cplusplus
extern "C" {
#endif
#include "augeas.h"
#ifdef __cplusplus
}
#endif
#endif

#define _RPMIOB_INTERNAL
#include <rpmiotypes.h>
#include <rpmio.h>	/* for *Pool methods */
#include <rpmlog.h>
#include <argv.h>

#define	_RPMAUG_INTERNAL
#include <rpmaug.h>

#include "debug.h"

#ifdef __cplusplus
GENfree(rpmioP)
#endif	/* __cplusplus */

/*@unchecked@*/
int _rpmaug_debug = 0;

/*@unchecked@*/ /*@relnull@*/
rpmaug _rpmaugI = NULL;

/*@-mustmod@*/	/* XXX splint on crack */
static void rpmaugFini(void * _aug)
	/*@globals fileSystem @*/
	/*@modifies *_aug, fileSystem @*/
{
    rpmaug aug = (rpmaug) _aug;

#if defined(WITH_AUGEAS)
    augeas * I = (augeas *) aug->I;
    (void) aug_close(I);
#endif
    aug->I = NULL;
    (void)rpmiobFree(aug->iob);
    aug->iob = NULL;
    aug->root = _free(aug->root);
    aug->loadpath = _free(aug->loadpath);
}
/*@=mustmod@*/

/*@unchecked@*/ /*@only@*/ /*@null@*/
rpmioPool _rpmaugPool = NULL;

static rpmaug rpmaugGetPool(/*@null@*/ rpmioPool pool)
	/*@globals _rpmaugPool, fileSystem @*/
	/*@modifies pool, _rpmaugPool, fileSystem @*/
{
    rpmaug aug;

    if (_rpmaugPool == NULL) {
	_rpmaugPool = rpmioNewPool("aug", sizeof(*aug), -1, _rpmaug_debug,
			NULL, NULL, rpmaugFini);
	pool = _rpmaugPool;
    }
    return (rpmaug) rpmioGetPool(pool, sizeof(*aug));
}

/*@unchecked@*/
static const char _root[] = "/";
const char * _rpmaugRoot = _root;
/*@unchecked@*/
static const char _loadpath[] = "";	/* XXX /usr/lib/rpm/lib ? */
const char * _rpmaugLoadpath = _loadpath;
unsigned int _rpmaugFlags = 0;		/* AUG_NONE */

#if defined(REFERENCE)
__attribute__((noreturn))
static void usage(void)
{
    fprintf(stderr, "Usage: %s [OPTIONS] [COMMAND]\n", progname);
    fprintf(stderr, "Load the Augeas tree and modify it. If no COMMAND is given, run interactively\n");
    fprintf(stderr, "Run '%s help' to get a list of possible commands.\n",
            progname);
    fprintf(stderr, "\nOptions:\n\n");
    fprintf(stderr, "  -c, --typecheck    typecheck lenses\n");
    fprintf(stderr, "  -b, --backup       preserve originals of modified files with\n"
                    "                     extension '.augsave'\n");
    fprintf(stderr, "  -n, --new          save changes in files with extension '.augnew',\n"
                    "                     leave original unchanged\n");
    fprintf(stderr, "  -r, --root ROOT    use ROOT as the root of the filesystem\n");
    fprintf(stderr, "  -I, --include DIR  search DIR for modules; can be given mutiple times\n");
    fprintf(stderr, "  --nostdinc         do not search the builtin default directories for modules\n");
    fprintf(stderr, "  --noload           do not load any files into the tree on startup\n");
    fprintf(stderr, "  --noautoload       do not autoload modules from the search path\n");

    exit(EXIT_FAILURE);
}
#endif

/*@unchecked@*/ /*@null@*/
const char ** _rpmaugLoadargv;

struct poptOption rpmaugPoptTable[] = {
#if defined(WITH_AUGEAS)
    /* XXX POPT_ARGFLAG_TOGGLE? */
    { "typecheck",'c', POPT_BIT_SET,		&_rpmaugFlags, AUG_TYPE_CHECK,
	N_("Type check lenses"), NULL },
    { "backup", 'b',POPT_BIT_SET,		&_rpmaugFlags, AUG_SAVE_BACKUP,
	N_("Backup modified files with suffix '.augsave'"), NULL },
    { "new", 'n', POPT_BIT_SET,			&_rpmaugFlags, AUG_SAVE_NEWFILE,
	N_("Save modified files with suffix '.augnew'"), NULL },
    { "root", 'r', POPT_ARG_STRING,		&_rpmaugRoot, 0,
	N_("Use ROOT as the root of the filesystem"), N_("ROOT") },
    { "include", 'I', POPT_ARG_ARGV,		&_rpmaugLoadargv, 0,
	N_("Search DIR for modules (may be used more than once)"), N_("DIR") },
    { "nostdinc", '\0', POPT_BIT_SET,		&_rpmaugFlags, AUG_NO_STDINC,
	N_("Do not search default modules path"), NULL },
    { "noload", '\0', POPT_BIT_SET,		&_rpmaugFlags, AUG_NO_LOAD,
	N_("Do not load files into tree on startup"), NULL },
    { "noautoload", '\0', POPT_BIT_SET,	 &_rpmaugFlags, AUG_NO_MODL_AUTOLOAD,
	N_("Do not autoload modules from the search path"), NULL },
#endif
    POPT_TABLEEND
};

rpmaug rpmaugNew(const char * root, const char * loadpath, unsigned int flags)
{
    rpmaug aug = rpmaugGetPool(_rpmaugPool);

    if (root == NULL || *root == '\0')
	root = _rpmaugRoot;
    if (root == NULL || *root == '\0')
	root = "/";
    if (loadpath == NULL)
	loadpath = _rpmaugLoadpath;
    aug->root = xstrdup(root);
    aug->loadpath = xstrdup(loadpath);
    aug->flags = flags;
#if defined(WITH_AUGEAS)
    aug->I = (void *) aug_init(aug->root, aug->loadpath, aug->flags);
assert(aug->I != NULL);
#endif
    aug->iob = rpmiobNew(0);

    return rpmaugLink(aug);
}

#ifdef	WITH_AUGEAS
static rpmaug rpmaugI(void)
        /*@globals _rpmaugI @*/
        /*@modifies _rpmaugI @*/
{
    if (_rpmaugI == NULL)
        _rpmaugI = rpmaugNew(_rpmaugRoot, _rpmaugLoadpath, _rpmaugFlags);
    return _rpmaugI;
}
#endif

int rpmaugDefvar(rpmaug aug, const char * name, const char * expr)
{
    int rc = -1;
#ifdef	WITH_AUGEAS
    augeas * I;
    if (aug == NULL) aug = rpmaugI();
    I = (augeas *) aug->I;
    rc = aug_defvar(I, name, expr);
#endif
if (_rpmaug_debug < 0)
fprintf(stderr, "<-- %s(%p,\"%s\",\"%s\") rc %d\n", __FUNCTION__, aug, name, expr, rc);
    return rc;
}

int rpmaugDefnode(rpmaug aug, const char * name, const char * expr,
		const char * value, int * created)
{
    int rc = -1;
#ifdef	WITH_AUGEAS
    augeas * I;
    if (aug == NULL) aug = rpmaugI();
    I = (augeas *) aug->I;
    rc = aug_defnode(I, name, expr, value, created);
#endif
if (_rpmaug_debug < 0)
fprintf(stderr, "<-- %s(%p,\"%s\",\"%s\",\"%s\",%p) rc %d *created %d\n", __FUNCTION__, aug, name, expr, value, created, rc, (created ? *created : 0));
    return rc;
}

int rpmaugGet(rpmaug aug, const char * path, const char ** value)
{
    int rc = -1;
#ifdef	WITH_AUGEAS
    augeas * I;
    if (aug == NULL) aug = rpmaugI();
    I = (augeas *) aug->I;
    rc = aug_get(I, path, value);
#endif
if (_rpmaug_debug < 0)
fprintf(stderr, "<-- %s(%p,\"%s\",%p) rc %d *value \"%s\"\n", __FUNCTION__, aug, path, value, rc, (value ? *value : NULL));
    return rc;
}

int rpmaugSet(rpmaug aug, const char * path, const char * value)
{
    int rc = -1;
#ifdef	WITH_AUGEAS
    augeas * I;
    if (aug == NULL) aug = rpmaugI();
    I = (augeas *) aug->I;
    rc = aug_set(I, path, value);
#endif
if (_rpmaug_debug < 0)
fprintf(stderr, "<-- %s(%p,\"%s\",\"%s\") rc %d\n", __FUNCTION__, aug, path, value, rc);
    return rc;
}

int rpmaugInsert(rpmaug aug, const char * path, const char * label, int before)
{
    int rc = -1;
#ifdef	WITH_AUGEAS
    augeas * I;
    if (aug == NULL) aug = rpmaugI();
    I = (augeas *) aug->I;
    rc = aug_insert(I, path, label, before);
#endif
if (_rpmaug_debug < 0)
fprintf(stderr, "<-- %s(%p,\"%s\",\"%s\",%d) rc %d\n", __FUNCTION__, aug, path, label, before, rc);
    return rc;
}

int rpmaugRm(rpmaug aug, const char * path)
{
    int rc = -1;
#ifdef	WITH_AUGEAS
    augeas * I;
    if (aug == NULL) aug = rpmaugI();
    I = (augeas *) aug->I;
    rc = aug_rm(I, path);
#endif
if (_rpmaug_debug < 0)
fprintf(stderr, "<-- %s(%p,\"%s\") rc %d\n", __FUNCTION__, aug, path, rc);
    return rc;
}

int rpmaugMv(rpmaug aug, const char * src, const char * dst)
{
    int rc = -1;
#ifdef	WITH_AUGEAS
    augeas * I;
    if (aug == NULL) aug = rpmaugI();
    I = (augeas *) aug->I;
    rc = aug_mv(I, src, dst);
#endif
if (_rpmaug_debug < 0)
fprintf(stderr, "<-- %s(%p,\"%s\",\"%s\") rc %d\n", __FUNCTION__, aug, src, dst, rc);
    return rc;
}

int rpmaugMatch(rpmaug aug, const char * path, char *** matches)
{
    int rc = -1;
#ifdef	WITH_AUGEAS
    augeas * I;
    if (aug == NULL) aug = rpmaugI();
    I = (augeas *) aug->I;
    rc = aug_match(I, path, matches);
#endif
if (_rpmaug_debug < 0)
fprintf(stderr, "<-- %s(%p,\"%s\",%p) rc %d *matches %p\n", __FUNCTION__, aug, path, matches, rc, (matches ? *matches : NULL));
    return rc;
}

int rpmaugSave(rpmaug aug)
{
    int rc = -1;
#ifdef	WITH_AUGEAS
    augeas * I;
    if (aug == NULL) aug = rpmaugI();
    I = (augeas *) aug->I;
    rc = aug_save(I);
#endif
if (_rpmaug_debug < 0)
fprintf(stderr, "<-- %s(%p) rc %d\n", __FUNCTION__, aug, rc);
    return rc;
}

int rpmaugLoad(rpmaug aug)
{
    int rc = -1;
#ifdef	WITH_AUGEAS
    augeas * I;
    if (aug == NULL) aug = rpmaugI();
    I = (augeas *) aug->I;
    rc = aug_load(I);
#endif
if (_rpmaug_debug < 0)
fprintf(stderr, "<-- %s(%p) rc %d\n", __FUNCTION__, aug, rc);
    return rc;
}

int rpmaugPrint(rpmaug aug, FILE *out, const char * path)
{
    int rc = -1;
#ifdef	WITH_AUGEAS
    augeas * I;
    if (aug == NULL) aug = rpmaugI();
    I = (augeas *) aug->I;
    if (out == NULL) out = stderr;
    rc = aug_print(I, out, path);
    (void) fflush(out);
#endif
if (_rpmaug_debug < 0)
fprintf(stderr, "<-- %s(%p, %p, \"%s\") rc %d\n", __FUNCTION__, aug, out, path, rc);
    return rc;
}

void rpmaugFprintf(rpmaug aug, const char *fmt, ...)
{
#if defined(WITH_AUGEAS)
    size_t nb = 1024;
    char * b = (char *) xmalloc(nb);
    va_list va;

    va_start(va, fmt);
    while(1) {
	int nw = vsnprintf(b, nb, fmt, va);
	if (nw > -1 && (size_t)nw < nb)
	    break;
	if (nw > -1)		/* glibc 2.1 (and later) */
	    nb = nw+1;
	else			/* glibc 2.0 */
	    nb *= 2;
	b = (char *) xrealloc(b, nb);
    }
    va_end(va);

    if (aug == NULL) aug = rpmaugI();
    (void) rpmiobAppend(aug->iob, b, 0);
    b = _free(b);
#endif
}

/* ===== internal.h */

#if !defined(SEP)
#define	SEP	'/'
#endif

#define DATADIR "/usr/share"

/* Define: AUGEAS_LENS_DIR
 * The default location for lens definitions */
#define AUGEAS_LENS_DIR DATADIR "/augeas/lenses"

/* The directory where we install lenses distribute with Augeas */
#define AUGEAS_LENS_DIST_DIR DATADIR "/augeas/lenses/dist"

/* Define: PATH_SEP_CHAR
 * Character separating paths in a list of paths
 */
#define PATH_SEP_CHAR ':'

/* ===== */

static char *cleanstr(char *path, const char sep)
{
    if (path && *path) {
	char *e = path + strlen(path) - 1;
	while (e >= path && (*e == sep || xisspace(*e)))
	    *e-- = '\0';
    }
    return path;
}

static char *cleanpath(char *path)
{
    return cleanstr(path, SEP);		/* XXX strip pesky trailing slashes */
}

static void err_check(void)
{
#ifdef	WITH_AUGEAS
    rpmaug aug = rpmaugI();
    augeas * I = (augeas *) aug->I;

    if (aug_error(I) != AUG_NOERROR) {
	const char *minor = aug_error_minor_message(I);
	const char *details = aug_error_details(I);

	fprintf(stderr, "error: %s\n", aug_error_message(I));
	if (minor != NULL)
	    fprintf(stderr, "error: %s\n", minor);
	if (details != NULL)
	    fprintf(stderr, "error: %s\n", details);
    }
#endif
}

static char *ls_pattern(const char *path)
{
    char *q;
    int r;

    if (path[strlen(path)-1] == SEP)
        r = asprintf(&q, "%s*", path);
    else
        r = asprintf(&q, "%s/*", path);
    if (r == -1)
        return NULL;
    return q;
}

static int child_count(const char *path)
{
    char *q = ls_pattern(path);
    int cnt;

    if (q == NULL)
        return 0;
    cnt = rpmaugMatch(NULL, q, NULL);
    err_check();
    free(q);
    return cnt;
}

static int cmd_quit(int ac, char *av[])
{
    return -1;	/* XXX return immediately on quit and exit commands */
}

static int cmd_ls(int ac, char *av[])
{
    char *path = cleanpath(av[0]);
    char **paths;
    int cnt;
    int i;

    path = ls_pattern(path);
    if (path == NULL)
        return -1;
    cnt = rpmaugMatch(NULL, path, &paths);
    err_check();
    for (i = 0; i < cnt; i++) {
        const char *basnam = strrchr(paths[i], SEP);
        int dir = child_count(paths[i]);
        const char *val;
        rpmaugGet(NULL, paths[i], &val);
	err_check();
        basnam = (basnam == NULL) ? paths[i] : basnam + 1;
        if (val == NULL)
            val = "(none)";
        rpmaugFprintf(NULL, "%s%s= %s\n", basnam, dir ? "/ " : " ", val);
	paths[i] = _free(paths[i]);
    }
    if (cnt > 0)
	paths = _free(paths);
    path = _free(path);
    return 0;
}

static int cmd_match(int ac, char *av[])
{
    const char *pattern = cleanpath(av[0]);
    int filter = (av[1] != NULL) && (strlen(av[1]) > 0);
    int result = 0;
    char **matches = NULL;
    int cnt;
    int i;

    cnt = rpmaugMatch(NULL, pattern, &matches);
    err_check();
    if (cnt < 0) {
        rpmaugFprintf(NULL, "  (error matching %s)\n", pattern);
        result = -1;
        goto done;
    }
    if (cnt == 0) {
        rpmaugFprintf(NULL, "  (no matches)\n");
        goto done;
    }

    for (i = 0; i < cnt; i++) {
        const char *val;
        rpmaugGet(NULL, matches[i], &val);
	err_check();
        if (val == NULL)
            val = "(none)";
        if (filter) {
            if (!strcmp(av[1], val))
                rpmaugFprintf(NULL, "%s\n", matches[i]);
        } else {
            rpmaugFprintf(NULL, "%s = %s\n", matches[i], val);
        }
    }
 done:
    for (i = 0; i < cnt; i++)
	matches[i] = _free(matches[i]);
    matches = _free(matches);
    return result;
}

static int cmd_rm(int ac, char *av[])
{
    const char *path = cleanpath(av[0]);
    int cnt;

    rpmaugFprintf(NULL, "rm : %s", path);
    cnt = rpmaugRm(NULL, path);
    err_check();
    rpmaugFprintf(NULL, " %d\n", cnt);
    return 0;
}

static int cmd_mv(int ac, char *av[])
{
    const char *src = cleanpath(av[0]);
    const char *dst = cleanpath(av[1]);
    int r = rpmaugMv(NULL, src, dst);

    err_check();
    if (r == -1)
	rpmaugFprintf(NULL, "Failed\n");
    return r;
}

static int cmd_set(int ac, char *av[])
{
    const char *path = cleanpath(av[0]);
    const char *val = av[1];
    int r = rpmaugSet(NULL, path, val);

    err_check();
    if (r == -1)
	rpmaugFprintf(NULL, "Failed\n");
    return r;
}

static int cmd_defvar(int ac, char *av[])
{
    const char *name = av[0];
    const char *path = (av[1] && *av[1] ? cleanpath(av[1]) : NULL);
    int r = rpmaugDefvar(NULL, name, path);

    err_check();
    if (r == -1)
	rpmaugFprintf(NULL, "Failed\n");
    return r;
}

static int cmd_defnode(int ac, char *av[])
{
    const char *name = av[0];
    const char *path = cleanpath(av[1]);
    const char *value = av[2];
    int r;

    /* Our simple minded line parser treats non-existant and empty values
     * the same. We choose to take the empty string to mean NULL */
    if (value != NULL && value[0] == '\0')
        value = NULL;

    r = rpmaugDefnode(NULL, name, path, value, NULL);
    err_check();
    if (r == -1)
	rpmaugFprintf(NULL, "Failed\n");
    return r;
}

static int cmd_clear(int ac, char *av[])
{
    const char *path = cleanpath(av[0]);
    int r = rpmaugSet(NULL, path, NULL);

    err_check();
    if (r == -1)
	rpmaugFprintf(NULL, "Failed\n");
    return r;
}

static int cmd_get(int ac, char *av[])
{
    const char *path = cleanpath(av[0]);
    const char *val;

    rpmaugFprintf(NULL, "%s", path);
    if (rpmaugGet(NULL, path, &val) != 1)
        rpmaugFprintf(NULL, " (o)\n");
    else if (val == NULL)
        rpmaugFprintf(NULL, " (none)\n");
    else
        rpmaugFprintf(NULL, " = %s\n", val);
    err_check();
    return 0;
}

static int cmd_print(int ac, char *av[])
{
    int r = rpmaugPrint(NULL, stdout, cleanpath(av[0]));
    err_check();
    return r;
}

static int cmd_save(int ac, /*@unused@*/ char *av[])
{
    int r = rpmaugSave(NULL);

    if (r == -1) {
	rpmaugFprintf(NULL, "Saving failed\n");
	err_check();
    } else {
        r = rpmaugMatch(NULL, "/augeas/events/saved", NULL);
        if (r > 0)
            rpmaugFprintf(NULL, "Saved %d file(s)\n", r);
        else if (r < 0)
            rpmaugFprintf(NULL, "Error during match: %d\n", r);
    }
    return r;
}

static int cmd_load(int ac, /*@unused@*/ char *av[])
{
    int r = rpmaugLoad(NULL);
    if (r == -1) {
	rpmaugFprintf(NULL, "Loading failed\n");
	err_check();
    } else {
        r = rpmaugMatch(NULL, "/augeas/events/saved", NULL);
        if (r > 0)
            rpmaugFprintf(NULL, "Saved %d file(s)\n", r);
        else if (r < 0)
            rpmaugFprintf(NULL, "Error during match: %d\n", r);
    }
    return r;
}

static int cmd_ins(int ac, char *av[])
{
    const char *label = av[0];
    const char *where = av[1];
    const char *path = cleanpath(av[2]);
    int before;
    int r = -1;		/* assume failure */

    if (!strcmp(where, "after"))
        before = 0;
    else if (!strcmp(where, "before"))
        before = 1;
    else {
        rpmaugFprintf(NULL, "The <WHERE> argument must be either 'before' or 'after'.");
	goto exit;
    }

    r = rpmaugInsert(NULL, path, label, before);
    err_check();

exit:
    return r;
}

static int cmd_help(int ac, /*@unused@*/ char *av[])
{
    const struct poptOption * c;

    rpmaugFprintf(NULL, "Commands:\n\n");
    for (c = _rpmaugCommandTable; c->longName != NULL; c++) {
        rpmaugFprintf(NULL, "    %s %s\n        %s\n\n",
		c->longName, (c->argDescrip ? c->argDescrip : ""), c->descrip);
    }
    rpmaugFprintf(NULL, "\nEnvironment:\n\n");
    rpmaugFprintf(NULL, "    AUGEAS_ROOT\n        the file system root, defaults to '/'\n\n");
    rpmaugFprintf(NULL, "    AUGEAS_LENS_LIB\n        colon separated list of directories with lenses,\n\
        defaults to " AUGEAS_LENS_DIR "\n\n");
    return 0;
}

#define	ARGMINMAX(_min, _max)	(int)(((_min) << 8) | ((_max) & 0xff))

const struct poptOption _rpmaugCommandTable[] = {
    { "ls", '\0', POPT_ARG_MAINCALL,		(void *)cmd_ls, ARGMINMAX(1, 1),
	N_("List the direct children of PATH"), N_("<PATH>")
    },
    { "match", '\0', POPT_ARG_MAINCALL,		(void *)cmd_match, ARGMINMAX(1, 2),
   N_("Find all paths that match the path expression PATH. If VALUE is given,\n"
      "        only the matching paths whose value equals VALUE are printed"),
	N_("<PATH> [<VALUE>]")
    },
    { "rm", '\0', POPT_ARG_MAINCALL,		(void *)cmd_rm, ARGMINMAX(1, 1),
	N_("Delete PATH and all its children from the tree"), N_("<PATH>")
    },
    { "mv", '\0', POPT_ARG_MAINCALL,		(void *)cmd_mv, ARGMINMAX(2, 2),
   N_("Move node SRC to DST. SRC must match exactly one node in the tree.\n"
      "        DST must either match exactly one node in the tree, or may not\n"
      "        exist yet. If DST exists already, it and all its descendants are\n"
      "        deleted. If DST does not exist yet, it and all its missing \n"
      "        ancestors are created."),
	N_("<SRC> <DST>")
    },
    { "set", '\0', POPT_ARG_MAINCALL,		(void *)cmd_set, ARGMINMAX(1, 2),
   N_("Associate VALUE with PATH. If PATH is not in the tree yet,\n"
      "        it and all its ancestors will be created. These new tree entries\n"
      "        will appear last amongst their siblings"),
	N_("<PATH> <VALUE>")
    },
    { "clear", '\0', POPT_ARG_MAINCALL,		(void *)cmd_clear, ARGMINMAX(1, 1),
   N_("Set the value for PATH to NULL. If PATH is not in the tree yet,\n"
      "        it and all its ancestors will be created. These new tree entries\n"
      "        will appear last amongst their siblings"),
	N_("<PATH>")
    },
    { "get", '\0', POPT_ARG_MAINCALL,		(void *)cmd_get, ARGMINMAX(1, 1),
	N_("Print the value associated with PATH"), N_("<PATH>")
    },
    { "print", '\0', POPT_ARG_MAINCALL,		(void *)cmd_print, ARGMINMAX(0, 1),
   N_("Print entries in the tree. If PATH is given, printing starts there,\n"
      "        otherwise the whole tree is printed"),
	N_("[<PATH>]")
    },
    { "ins", '\0', POPT_ARG_MAINCALL,		(void *)cmd_ins, ARGMINMAX(3, 3),
   N_("Insert a new node with label LABEL right before or after PATH into\n"
     "        the tree. WHERE must be either 'before' or 'after'."),
	N_("<LABEL> <WHERE> <PATH>")
    },
    { "save", '\0', POPT_ARG_MAINCALL,		(void *)cmd_save, ARGMINMAX(0, 0),
   N_("Save all pending changes to disk. For now, files are not overwritten.\n"
      "        Instead, new files with extension .augnew are created"),
	NULL
    },
    { "load", '\0', POPT_ARG_MAINCALL,		(void *)cmd_load, ARGMINMAX(0, 0),
	N_("Load files accordig to the transforms in /augeas/load."), NULL
    },
    { "defvar", '\0', POPT_ARG_MAINCALL,	(void *)cmd_defvar, ARGMINMAX(1, 2),
   N_("Define the variable NAME to the result of evalutating EXPR. The\n"
      "        variable can be used in path expressions as $NAME. Note that EXPR\n"
      "        is evaluated when the variable is defined, not when it is used."),
	N_("<NAME> <EXPR>")
    },
    { "defnode", '\0', POPT_ARG_MAINCALL,	(void *)cmd_defnode, ARGMINMAX(2, 3),
   N_("Define the variable NAME to the result of evalutating EXPR, which\n"
      "        must be a nodeset. If no node matching EXPR exists yet, one\n"
      "        is created and NAME will refer to it. If VALUE is given, this\n"
      "        is the same as 'set EXPR VALUE'; if VALUE is not given, the\n"
      "        node is created as if with 'clear EXPR' would and NAME refers\n"
      "        to that node."),
	N_("<NAME> <EXPR> [<VALUE>]")
    },
    { "exit", '\0', POPT_ARG_MAINCALL,		(void *)cmd_quit, ARGMINMAX(0, 0),
	N_("Exit the program"), NULL
    },
    { "quit", '\0', POPT_ARG_MAINCALL,		(void *)cmd_quit, ARGMINMAX(0, 0),
	N_("Exit the program"), NULL
    },
    { "help", '\0', POPT_ARG_MAINCALL,		(void *)cmd_help, ARGMINMAX(0, 0),
	N_("Print this help text"), NULL
    },
    { NULL, '\0', 0, NULL, ARGMINMAX(0, 0), NULL, NULL }
};

rpmRC rpmaugRun(rpmaug aug, const char * str, const char ** resultp)
{
    rpmioP P = NULL;
    rpmRC rc = RPMRC_OK;	/* assume success */
    int xx;

#ifdef	WITH_AUGEAS
    if (aug == NULL) aug = rpmaugI();
#else
    if (aug == NULL) return rc;		/* XXX FIXME */
#endif

    if (resultp)
	*resultp = NULL;

    while (rpmioParse(&P, str) != RPMRC_NOTFOUND) {	/* XXX exit on EOS */
	const struct poptOption * c;
	int (*handler) (int ac, char * av[]);
	int minargs;
	int maxargs;

	str = NULL;

	if (!(P->av && P->ac > 0 && P->av[0] != NULL && strlen(P->av[0]) > 0))
	    continue;

	for (c = _rpmaugCommandTable; c->longName; c++) {
	    if (strcmp(P->av[0], c->longName))
		continue;
	    handler = (int (*)(int, char**)) c->arg;
	    minargs = (c->val >> 8) & 0xff;
	    maxargs = (c->val     ) & 0xff;
	    break;
	}
	if (c->longName == NULL) {
	    rpmaugFprintf(NULL, "Unknown command '%s'\n", P->av[0]);
	    rc = RPMRC_FAIL;
	} else
	if ((P->ac - 1) < minargs) {
	    rpmaugFprintf(NULL, "Not enough arguments for %s\n", c->longName);
	    rc = RPMRC_FAIL;
	} else
	if ((P->ac - 1) > maxargs) {
	    rpmaugFprintf(NULL, "Too many arguments for %s\n", c->longName);
	    rc = RPMRC_FAIL;
	} else
	if ((xx = (*handler)(P->ac-1, (char **)P->av+1)) < 0) {
	    /* XXX return immediately on quit and exit commands */
	    if (!strcmp(c->longName, "quit") || !strcmp(c->longName, "exit"))
		rc = RPMRC_NOTFOUND;
	    else {
		rpmaugFprintf(NULL, "Failed(%d): %s\n", xx, P->av[0]);
		rc = RPMRC_FAIL;
	    }
	}

	if (rc != RPMRC_OK)
	    break;
    }
    if (aug != NULL) {
	rpmiob iob = aug->iob;
	if (resultp && iob->blen > 0) /* XXX return result iff bytes appended */
	    *resultp = rpmiobStr(iob);
	iob->blen = 0;			/* XXX reset the print buffer */
    }
    if (P != NULL) {
	P->str = _free(P->str);
	P->av = argvFree(P->av);
	P = _free(P);
    }
    return rc;
}
