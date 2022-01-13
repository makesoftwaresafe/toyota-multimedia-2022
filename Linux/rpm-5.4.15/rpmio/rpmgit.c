/** \ingroup rpmio
 * \file rpmio/rpmgit.c
 */

#include "system.h"

#include <rpmiotypes.h>
#include <rpmio.h>	/* for *Pool methods */
#include <rpmlog.h>
#include <rpmmacro.h>
#include <rpmurl.h>
#include <ugid.h>
#include <poptIO.h>

#define	_RPMGIT_INTERNAL
#include <rpmgit.h>

#include "debug.h"

/*@unchecked@*/
int _rpmgit_debug;
/*@unchecked@*/
const char * _rpmgit_dir;	/* XXX GIT_DIR */
/*@unchecked@*/
const char * _rpmgit_tree;	/* XXX GIT_WORK_TREE */

/*@unchecked@*/
static int _rpmgit_threads;	/* XXX git_threads_{init,shutdown} counter */

#define	SPEW(_t, _rc, _git)	\
  { if ((_t) || _rpmgit_debug ) \
	fprintf(stderr, "<-- %s(%p) rc %d\n", __FUNCTION__, (_git), \
		(_rc)); \
  }

/*==============================================================*/
#if defined(WITH_LIBGIT2)
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

#define _ENTRY(_v)      { GIT_IDXENTRY_##_v, #_v, }
/*@unchecked@*/ /*@observer@*/
static KEY IDXEflags[] = {
    _ENTRY(UPDATE),
    _ENTRY(REMOVE),
    _ENTRY(UPTODATE),
    _ENTRY(ADDED),
    _ENTRY(HASHED),
    _ENTRY(UNHASHED),
    _ENTRY(WT_REMOVE),
    _ENTRY(CONFLICTED),
    _ENTRY(UNPACKED),
    _ENTRY(NEW_SKIP_WORKTREE),

    _ENTRY(INTENT_TO_ADD),
    _ENTRY(SKIP_WORKTREE),
    _ENTRY(EXTENDED2),
};
#undef	_ENTRY
/*@unchecked@*/
static size_t nIDXEflags = sizeof(IDXEflags) / sizeof(IDXEflags[0]);
/*@observer@*/
static const char * fmtIDXEflags(uint32_t flags)
	/*@*/
{
    static char buf[BUFSIZ];
    char * te = buf;
    te = stpcpy(te, "\n\tflags: ");
    (void) fmtBits(flags, IDXEflags, nIDXEflags, te);
    return buf;
}
#define	_IDXFLAGS(_idxeflags)	fmtIDXEflags(_idxeflags)

#define _ENTRY(_v)      { GIT_REF_##_v, #_v, }
/*@unchecked@*/ /*@observer@*/
static KEY REFflags[] = {
    _ENTRY(OID),
    _ENTRY(SYMBOLIC),
};
#undef	_ENTRY
/*@unchecked@*/
static size_t nREFflags = sizeof(REFflags) / sizeof(REFflags[0]);
/*@observer@*/
static const char * fmtREFflags(uint32_t flags)
	/*@*/
{
    static char buf[BUFSIZ];
    char * te = buf;
    te = stpcpy(te, "\n\tflags: ");
    (void) fmtBits(flags, REFflags, nREFflags, te);
    return buf;
}
#define	_REFFLAGS(_refflags)	fmtREFflags(_refflags)

/*==============================================================*/

static void check(int error, const char *message, const char *extra)
{
    const git_error *lg2err;
    const char *lg2msg = "";
    const char *lg2spacer = "";

    if (!error)
	return;

    if ((lg2err = giterr_last()) != NULL && lg2err->message != NULL) {
	lg2msg = lg2err->message;
	lg2spacer = " - ";
    }

    if (extra)
	fprintf(stderr, "%s '%s' [%d]%s%s\n",
		message, extra, error, lg2spacer, lg2msg);
    else
	fprintf(stderr, "%s [%d]%s%s\n",
		message, error, lg2spacer, lg2msg);

    exit(1);
}

static int Xchkgit(/*@unused@*/ rpmgit git, const char * msg,
                int error, int printit,
                const char * func, const char * fn, unsigned ln)
	/*@*/
{
    int rc = error;

    if (printit && rc) {
	const git_error * e = giterr_last();
	char * message = (e ? e->message : "");
	int klass = (e ? e->klass : -12345);
        rpmlog(RPMLOG_ERR, "%s:%s:%u: %s(%d): %s(%d)\n",
                func, fn, ln, msg, rc, message, klass);
    }

    return rc;
}
#define chkgit(_git, _msg, _error)  \
    Xchkgit(_git, _msg, _error, _rpmgit_debug, __FUNCTION__, __FILE__, __LINE__)

void rpmgitPrintOid(const char * msg, const void * _oidp, void * _fp)
{
    FILE * fp = (_fp ? _fp : stderr);
    const git_oid * oidp = _oidp;
    char * t;
    if (oidp) {
	t = git_oid_allocfmt(oidp);
	git_oid_fmt(t, oidp);
    } else
	t = strdup("NULL");
if (msg) fprintf(fp, "%s:", msg);
fprintf(fp, " %s\n", t);
    t = _free(t);
}

void rpmgitPrintTime(const char * msg, time_t _Ctime, void * _fp)
{
    FILE * fp = (_fp ? _fp : stderr);
    static const char _fmt[] = "%c";
    struct tm tm;
    char _b[BUFSIZ];
    size_t _nb = sizeof(_b);
    size_t nw = strftime(_b, _nb-1, _fmt, localtime_r(&_Ctime, &tm));
    (void)nw;
if (msg) fprintf(fp, "%s:", msg);
fprintf(fp, " %s", _b);
}

void rpmgitPrintSig(const char * msg, const void * ___S, void * _fp)
{
    FILE * fp = (_fp ? _fp : stderr);
    const git_signature * S = ___S;
assert(S != NULL);
if (msg) fprintf(fp, "%s:", msg);
fprintf(fp, " %s <%s>", S->name, S->email);
#ifdef	DYING
rpmgitPrintTime(NULL, (time_t)S->when.time, fp);
#else
fprintf(fp, " %lu", (unsigned long)S->when.time);
fprintf(fp, " %.02d%.02d", (S->when.offset/60), (S->when.offset%60));
#endif
fprintf(fp, "\n");
}

void rpmgitPrintIndex(void * ___I, void * _fp)
{
    FILE * fp = (FILE *) _fp;
    git_index * I = (git_index *) ___I;
    unsigned Icnt;
    unsigned i;

assert(I != NULL);
if (_rpmgit_debug < 0 && fp == NULL) fp = stderr;
if (fp == NULL) return;

    Icnt = git_index_entrycount(I);
    fprintf(fp, "-------- Index(%u)\n", Icnt);
    for (i = 0; i < Icnt; i++) {
	const git_index_entry * E = git_index_get_byindex(I, i);

	fprintf(fp, "=== %s:", E->path);

     rpmgitPrintOid("\n\t  oid", &E->id, fp);

	fprintf(fp,   "\t  dev: %x", (unsigned)E->dev);
	fprintf(fp, "\n\t  ino: %lu", (unsigned long)E->ino);
	fprintf(fp, "\n\t mode: %o", (unsigned)E->mode);

	fprintf(fp, "\n\t user: %s", uidToUname((uid_t)E->uid));
	fprintf(fp, "\n\tgroup: %s", gidToGname((gid_t)E->gid));

	fprintf(fp, "\n\t size: %lu", (unsigned long)E->file_size);

    rpmgitPrintTime("\n\tctime", E->ctime.seconds, fp);

    rpmgitPrintTime("\n\tmtime", E->mtime.seconds, fp);

	fprintf(fp, "%s", _IDXFLAGS(E->flags));

	fprintf(fp, "\n");
    }
}

#ifdef	DYING
static const char * rpmgitOtype(git_otype otype)
	/*@*/
{
    const char * s = "unknown";
    switch(otype) {
    case GIT_OBJ_ANY:		s = "any";	break;
    case GIT_OBJ_BAD:		s = "bad";	break;
    case GIT_OBJ__EXT1:		s = "EXT1";	break;
    case GIT_OBJ_COMMIT:	s = "commit";	break;
    case GIT_OBJ_TREE:		s = "tree";	break;
    case GIT_OBJ_BLOB:		s = "blob";	break;
    case GIT_OBJ_TAG:		s = "tag";	break;
    case GIT_OBJ__EXT2:		s = "EXT2";	break;
    case GIT_OBJ_OFS_DELTA:	s = "delta off";	break;
    case GIT_OBJ_REF_DELTA:	s = "delta oid";	break;
    }
    return s;
}
#endif

void rpmgitPrintTree(void * ___T, void * _fp)
{
    git_tree * T = (git_tree *) ___T;
    FILE * fp = (FILE *) _fp;
    unsigned Tcnt;
    unsigned i;

assert(T != NULL);
if (_rpmgit_debug < 0 && fp == NULL) fp = stderr;
if (fp == NULL) return;

#ifdef	DYING
 rpmgitPrintOid("-------- Toid", git_tree_id(T), fp);
#endif
    Tcnt = git_tree_entrycount(T);
#ifdef	DYING
fprintf(fp,     "         Tcnt: %u\n", Tcnt);
#endif
    for (i = 0; i < Tcnt; i++) {
	const git_tree_entry * E = git_tree_entry_byindex(T, i);
	char * t;
assert(E != NULL);
#ifdef	DYING
fprintf(fp,     "       Eattrs: 0%o\n", git_tree_entry_filemode(E));
fprintf(fp,     "        Ename: %s\n", git_tree_entry_name(E));
 rpmgitPrintOid("         Eoid", git_tree_entry_id(E), fp);
fprintf(fp,     "        Etype: %s\n", rpmgitOtype(git_tree_entry_type(E)));
#else
	t = git_oid_allocfmt(git_tree_entry_id(E));
	fprintf(fp, "%06o %.4s %s\t%s\n",
		git_tree_entry_filemode(E),
		git_object_type2string(git_tree_entry_type(E)),
		t,
		git_tree_entry_name(E));
	t = _free(t);
#endif
    }
}

void rpmgitPrintCommit(rpmgit git, void * ___C, void * _fp)
{
    FILE * fp = (FILE *) _fp;
    git_commit * C = ___C;
    unsigned Pcnt;
    unsigned i;

assert(C != NULL);
if (_rpmgit_debug < 0 && fp == NULL) fp = stderr;
if (fp == NULL) return;

#ifdef	DYING
 rpmgitPrintOid("-------- Coid", git_commit_id(C), fp);
fprintf(fp,     "      Cmsgenc: %s\n", git_commit_message_encoding(C));
fprintf(fp,     "         Cmsg: %s\n", git_commit_message(C));

rpmgitPrintTime("        Ctime", git_commit_time(C), fp);
fprintf(fp, "\n");

fprintf(fp,     "          Ctz: %d\n", git_commit_time_offset(C));
 rpmgitPrintSig("      Cauthor", git_commit_author(C), fp);
 rpmgitPrintSig("      Ccmtter", git_commit_committer(C), fp);
 rpmgitPrintOid("         Toid", git_commit_tree_oid(C), fp);

    Pcnt = git_commit_parentcount(C);
fprintf(fp,     "         Pcnt: %u\n", Pcnt);
    for (i = 0; i < Pcnt; i++) {
	git_commit * E;
	xx = chkgit(git, "git_commit_parent",
			git_commit_parent(&E, C, i));
	const git_oid * Poidp = git_commit_parent_oid(E, i);
 rpmgitPrintOid("         Poid", Poidp, fp);
    }
#else
 rpmgitPrintOid("tree", git_commit_tree_id(C), fp);
    Pcnt = git_commit_parentcount(C);
    for (i = 0; i < Pcnt; i++)
	rpmgitPrintOid("parent", git_commit_parent_id(C, i), fp);
 rpmgitPrintSig("author", git_commit_author(C), fp);
 rpmgitPrintSig("committer", git_commit_committer(C), fp);
fprintf(fp,     "encoding: %s\n", git_commit_message_encoding(C));
fprintf(fp,     "\n%s", git_commit_message(C));
#endif
}

void rpmgitPrintTag(rpmgit git, void * _tag, void * _fp)
{
    FILE * fp = (_fp ? _fp : stderr);
    git_tag * tag = (git_tag *) _tag;

assert(tag != NULL);
#ifdef	NOTYET
if (_rpmgit_debug >= 0) return;
#endif

#ifdef	DYING
 rpmgitPrintOid("--------  TAG", git_tag_id(tag), fp);
fprintf(fp,     "         name: %s\n", git_tag_name(tag));
fprintf(fp,     "         type: %s\n", git_object_type2string(git_tag_type(tag)));
 rpmgitPrintOid("       target", git_tag_target_id(tag), fp);
 rpmgitPrintSig("       tagger", git_tag_tagger(tag), fp);
fprintf(fp,     "\n%s", git_tag_message(tag));
#else
 rpmgitPrintOid("object", git_tag_target_id(tag), fp);
fprintf(fp,     "type: %s\n", git_object_type2string(git_tag_target_type(tag)));
fprintf(fp,     "tag: %s\n", git_tag_name(tag));
/* XXX needs strftime(3) */
 rpmgitPrintSig("tagger", git_tag_tagger(tag), fp);
fprintf(fp,     "\n%s", git_tag_message(tag));
#endif

}

void rpmgitPrintHead(rpmgit git, void * ___H, void * _fp)
{
    FILE * fp = (_fp ? _fp : stderr);
    git_reference * H = (___H ? ___H : git->H);
    git_reference * Hresolved = NULL;
    int xx;

    if (H == NULL) {
	xx = chkgit(git, "git_repository_head",
		git_repository_head((git_reference **)&git->H, git->R));
	H = git->H;
    }
assert(H != NULL);
if (_rpmgit_debug >= 0) return;

    xx = chkgit(git, "git_reference_resolve",
		git_reference_resolve(&Hresolved, H));

 rpmgitPrintOid("------- Hpeel", git_reference_target_peel(H), fp);
 rpmgitPrintOid("      Htarget", git_reference_target(H), fp);
fprintf(fp,     "        Hname: %s\n", git_reference_name(H));
fprintf(fp,     "    Hresolved: %p\n", Hresolved);
fprintf(fp,     "       Howner: %p", git_reference_owner(H));
#ifdef	DYING
fprintf(fp,     "       Hrtype: %d\n", (int)git_reference_type(H));
#else
fprintf(fp,     "%s\n", _REFFLAGS(git_reference_type(H)));
#endif

    if (Hresolved)	/* XXX leak */
	git_reference_free(Hresolved);

}

void rpmgitPrintRepo(rpmgit git, void * ___R, void * _fp)
{
    FILE * fp = (_fp ? _fp : stderr);
    git_repository * R = ___R;
    const char * fn;

if (_rpmgit_debug >= 0) return;

fprintf(fp, "head_detached: %d\n", git_repository_head_detached(R));
fprintf(fp, "  head_unborn: %d\n", git_repository_head_unborn(R));
fprintf(fp, "     is_empty: %d\n", git_repository_is_empty(R));
fprintf(fp, "      is_bare: %d\n", git_repository_is_bare(R));
    fn = git_repository_path(R);
fprintf(fp, "         path: %s\n", fn);
    fn = git_repository_workdir(R);
fprintf(fp, "      workdir: %s\n", fn);
    /* XXX get_repository_config */
    /* XXX get_repository_odb */
    /* XXX get_repository_refdb */
    /* XXX get_repository_index */
    /* XXX get_repository_message */
fprintf(fp, "        state: %d\n", git_repository_state(R));
fprintf(fp, "    namespace: %s\n", git_repository_get_namespace(R));
fprintf(fp, "   is_shallow: %d\n", git_repository_is_empty(R));

}
#endif	/* defined(WITH_LIBGT2) */

/*==============================================================*/
int rpmgitInit(rpmgit git, void * initopts)
{
    int rc = -1;
#if defined(WITH_LIBGIT2)
    git_repository_init_options opts = GIT_REPOSITORY_INIT_OPTIONS_INIT;

    if (git->R) {	/* XXX leak */
	git_repository_free(git->R);
	git->R = NULL;
    }

    if (initopts == NULL) {
	opts.flags = GIT_REPOSITORY_INIT_MKPATH; /* don't use default */
	if (git->is_bare)
		opts.flags |= GIT_REPOSITORY_INIT_BARE;
	initopts = (void *) &opts;
    }
	/* XXX git_repository_init_ext(&git->R, git->fn, &opts); */
	/* XXX git->repodir? */
    rc = chkgit(git, "git_repository_init_ext",
		git_repository_init_ext((git_repository **)&git->R, git->fn,
			(git_repository_init_options *)initopts));
    if (rc)
	goto exit;
if (_rpmgit_debug < 0) rpmgitPrintRepo(git, git->R, git->fp);

exit:
#endif	/* defined(WITH_LIBGT2) */
SPEW(0, rc, git);
    return rc;
}

int rpmgitAddFile(rpmgit git, const char * fn)
{
    int rc = -1;
#if defined(WITH_LIBGIT2)

    /* XXX Strip out workdir prefix if present. */
    {	const char * s = git_repository_workdir(git->R);
	size_t ns = strlen(s);
	if (strlen(fn) > ns && !strncmp(fn, s, ns))
	    fn += ns;
    }

    /* Upsert the file into the index. */
    rc = chkgit(git, "git_index_add_bypath",
		git_index_add_bypath(git->I, fn));
    if (rc)
	goto exit;

    /* Write the index to disk. */
    rc = chkgit(git, "git_index_write",
		git_index_write(git->I));
    if (rc)
	goto exit;

exit:
#endif	/* defined(WITH_LIBGT2) */
SPEW(0, rc, git);
    return rc;
}

int rpmgitCommit(rpmgit git, const char * msg)
{
    int rc = -1;
#if defined(WITH_LIBGIT2)
    static const char _msg[] = "WDJ commit";
    static const char * update_ref = "HEAD";
    static const char * message_encoding = "UTF-8";
    git_commit * old_head = NULL;

    const char * message = (msg ? msg : _msg);
    const char * user_name =
	(git->user_name ? git->user_name : "Jeff Johnson");
    const char * user_email =
	(git->user_email ? git->user_email : "jbj@jbj.org");

    git_oid _oidC;
    git_oid * Coidp = &_oidC;
    git_signature * Cauthor = NULL;
    git_signature * Ccmtter = NULL;

    git_oid _oidT;

    /* Find the root tree oid. */
    rc = chkgit(git, "git_index_write_tree",
		git_index_write_tree(&_oidT, git->I));
    if (rc)
	goto exit;
if (_rpmgit_debug < 0) rpmgitPrintOid("         oidT", &_oidT, NULL);

    /* Find the tree object. */
    rc = chkgit(git, "git_tree_lookup",
		git_tree_lookup((git_tree **)&git->T, git->R, &_oidT));
    if (rc)
	goto exit;

if (_rpmgit_debug < 0) rpmgitPrintTree(git->T, NULL);

    /* Find the current HEAD. */
    rc = chkgit(git, "git_revparse_single",
		git_revparse_single((git_object**)&old_head,
                                 git->R, update_ref));
    if (rc)
	goto exit;

    /* Commit the added file. */
    rc = chkgit(git, "git_signature_now",
		git_signature_now(&Cauthor, user_name, user_email));
    if (rc)
	goto exit;
    rc = chkgit(git, "git_signature_now",
		git_signature_now(&Ccmtter, user_name, user_email));
    if (rc)
	goto exit;


    rc = chkgit(git, "git_commit_create",
		git_commit_create_v(Coidp, git->R, update_ref,
			Cauthor, Ccmtter,
			message_encoding, message,
			git->T,
			1, old_head));
    if (rc)
	goto exit;

if (_rpmgit_debug < 0) rpmgitPrintOid("         oidC", Coidp, NULL);

    /* Find the commit object. */
    rc = chkgit(git, "git_commit_lookup",
		git_commit_lookup((git_commit **)&git->C, git->R, Coidp));

    if (git->T) {	/* XXX leak */
	git_tree_free(git->T);
	git->T = NULL;
    }
    rc = chkgit(git, "git_commit_tree",
		git_commit_tree((git_tree **)&git->T, git->C));

exit:
    if (Cauthor)
	git_signature_free((git_signature *)Cauthor);
    if (Ccmtter)
	git_signature_free((git_signature *)Ccmtter);
#endif	/* defined(WITH_LIBGT2) */
SPEW(0, rc, git);

    return rc;
}

#if defined(WITH_LIBGIT2)
static int rpmgitConfigCB(const git_config_entry * CE, void * _git)
{
    rpmgit git = (rpmgit) _git;
    const char * var_name = CE->name;
    const char * value = CE->value;
    int rc = 0;

    if (!strcmp("core.bare", var_name)) {
	git->core_bare = strcmp(value, "false") ? 1 : 0;
    } else
    if (!strcmp("core.repositoryformatversion", var_name)) {
	git->core_repositoryformatversion = atol(value);
    } else
    if (!strcmp("user.name", var_name)) {
	git->user_name = _free(git->user_name);
	git->user_name = xstrdup(value);
    } else
    if (!strcmp("user.email", var_name)) {
	git->user_email = _free(git->user_email);
	git->user_email = xstrdup(value);
    }
    if (git->fp)
	fprintf(git->fp, "%s: %s\n", var_name, value);

SPEW(0, rc, git);
    return rc;
}
#endif	/* defined(WITH_LIBGT2) */

int rpmgitConfig(rpmgit git)
{
    int rc = -1;

#if defined(WITH_LIBGIT2)
    /* Read/print/save configuration info. */
    rc = chkgit(git, "git_repository_config",
		git_repository_config((git_config **)&git->cfg, git->R));
    if (rc)
	goto exit;

    rc = chkgit(git, "git_config_foreach",
		git_config_foreach(git->cfg, rpmgitConfigCB, git));
    if (rc)
	goto exit;

exit:
#endif	/* defined(WITH_LIBGT2) */
SPEW(0, rc, git);
    return rc;
}

const char * rpmgitOid(rpmgit git, const void * _oid)
{
#if defined(WITH_LIBGIT2)
    git_oid * oid = (git_oid *) _oid;
    git->str[0] = '\0';
    git_oid_tostr(git->str, sizeof(git->str), oid);
    git->str[RPMGIT_OID_HEXSZ] = '\0';
#else
    git->str[0] = '\0';
#endif	/* defined(WITH_LIBGT2) */
    return git->str;
}

int rpmgitClose(rpmgit git)
{
    int rc = 0;

#if defined(WITH_LIBGIT2)
	/* XXX other sanity checks and side effects? */
    if (git->R) {
	git_repository_free(git->R);
	git->R = NULL;
	git->repodir = _free(git->repodir);
    }
#endif	/* defined(WITH_LIBGT2) */

SPEW(0, rc, git);
    return rc;
}

int rpmgitOpen(rpmgit git, const char * repodir)
{
    int rc = 0;

#if defined(WITH_LIBGIT2)
	/* XXX lazy close? */
    if (git->R == NULL) {
	if (repodir) {
	    git->repodir = _free(git->repodir);
	    git->repodir = Realpath(repodir, NULL);
	} else if (git->repodir == NULL) {
	    const char * dn = (git->fn ? git->fn : ".");
	    git->repodir = Realpath(dn, NULL);
	}
	rc = chkgit(git, "git_repository_open_ext",
		git_repository_open_ext((git_repository **)&git->R, git->repodir, 0, NULL));
    }

#endif	/* defined(WITH_LIBGT2) */
SPEW(0, rc, git);
    return rc;
}

/*==============================================================*/

int rpmgitTree(rpmgit git)
{
    int rc = -1;
#if defined(WITH_LIBGIT2)
    const git_tree_entry *entry;
    git_object *objt;
    (void)entry;
    (void)objt;

#endif
SPEW(0, rc, git);
    return rc;
}

int rpmgitWalk(rpmgit git)
{
    int rc = -1;
#if defined(WITH_LIBGIT2)
    FILE * fp = (git->fp ? git->fp : stdout);
    git_revwalk * walk;
    git_oid oid;
    int xx;

    xx = chkgit(git, "git_revwalk_new",
		git_revwalk_new(&walk, git->R));
    git_revwalk_sorting(walk, GIT_SORT_TOPOLOGICAL | GIT_SORT_REVERSE);
    xx = chkgit(git, "git_revwalk_push_head",
		git_revwalk_push_head(walk));
    git->walk = (void *) walk;

    while ((xx = chkgit(git, "git_revwalk_next",
		git_revwalk_next(&oid, walk))) == GIT_OK)
    {
	git_commit * C;
	const git_signature * S;

	xx = chkgit(git, "git_commit_lookup",
		git_commit_lookup(&C, git->R, &oid));
rpmgitPrintOid("Commit", git_commit_id(C), fp);
	S = git_commit_author(C);
fprintf(fp, "Author: %s <%s>", S->name, S->email);
rpmgitPrintTime("\n  Date", (time_t)S->when.time, fp);
fprintf(fp, "\n%s", git_commit_message(C));
fprintf(fp, "\n");
	git_commit_free(C);
    }

    git->walk = NULL;
    git_revwalk_free(walk);
    walk = NULL;
    rc = 0;	/* XXX */
    goto exit;	/* XXX GCC warning */

exit:
#endif
SPEW(0, rc, git);
    return rc;
}

int rpmgitInfo(rpmgit git)
{
    int rc = -1;
#if defined(WITH_LIBGIT2)
    FILE * fp = (git->fp ? git->fp : stdout);
    unsigned ecount;
    unsigned i;
    size_t nt = 128;
    char * t = alloca(nt);
    size_t nb;
    int xx;

    xx = chkgit(git, "git_repository_index",
		git_repository_index((git_index **)&git->I, git->R));
    xx = chkgit(git, "git_index_read",
		git_index_read(git->I, 0));

    ecount = git_index_entrycount(git->I);
    for (i = 0; i < ecount; i++) {
	static const char _fmt[] = "%c";
	const git_index_entry * e = git_index_get_byindex(git->I, i);
	git_oid oid = e->id;
	time_t mtime;
	struct tm tm;

	fprintf(fp, "=== %s:", e->path);

	git_oid_fmt(t, &oid);
	t[GIT_OID_HEXSZ] = '\0';
	fprintf(fp, "\n\t  oid: %s", t);

	fprintf(fp, "\n\t  dev: %x", (unsigned)e->dev);
	fprintf(fp, "\n\t  ino: %lu", (unsigned long)e->ino);
	fprintf(fp, "\n\t mode: %o", (unsigned)e->mode);

	fprintf(fp, "\n\t user: %s", uidToUname((uid_t)e->uid));
	fprintf(fp, "\n\tgroup: %s", gidToGname((gid_t)e->gid));

	fprintf(fp, "\n\t size: %lu", (unsigned long)e->file_size);

	mtime = e->ctime.seconds;
	nb = strftime(t, nt, _fmt, localtime_r(&mtime, &tm));
	fprintf(fp, "\n\tctime: %s", t);

	mtime = e->mtime.seconds;
	nb = strftime(t, nt, _fmt, localtime_r(&mtime, &tm));
	fprintf(fp, "\n\tmtime: %s", t);

	fprintf(fp, "%s", _IDXFLAGS(e->flags));

	fprintf(fp, "\n");
    }

    git_index_free(git->I);
    git->I = NULL;
    rc = 0;
#endif
SPEW(0, rc, git);
    return rc;
}

int rpmgitRead(rpmgit git)
{
    int rc = -1;
#if defined(WITH_LIBGIT2) && defined(NOTYET)
    FILE * fp = (git->fp ? git->fp : stdout);
    git_odb * odb = git_repository_database(git->R);
    git_odb_object * obj;
    git_oid oid;
    unsigned char * data;
    const char * str_type;
    git_otype otype;

    rc = git_odb_read(&obj, odb, &oid);
    data = (char *)git_odb_object_data(obj);
    otype = git_odb_object_type(oid);
    str_type = git_object_type2string(otype);
    fprintf(fp, "object length and type: %d, %s\n",
	(int)git_odb_object_size(obj), str_type);

    git_odb_object_close(obj);

    rc = 0;	/* XXX */
#endif
SPEW(0, rc, git);
    return rc;
}

int rpmgitWrite(rpmgit git)
{
    int rc = -1;
#if defined(WITH_LIBGIT2) && defined(NOTYET)
    FILE * fp = (git->fp ? git->fp : stdout);
    git_odb * odb = git_repository_database(git->R);
    git_oid oid;
    size_t nb = GIT_OID_HEXSZ;
    char * b = alloca(nb+1);

    nb = strncpy(b, nb, "blah blah") - b;
    rc = git_odb_write(&oid, odb, b, nb, GIT_OBJ_BLOB);
    git_oid_format(b, &oid);
    fprintf(fp, "Written Object: %s\n", b);

    rc = 0;	/* XXX */
#endif
SPEW(0, rc, git);
    return rc;
}

/*==============================================================*/

static int rpmgitPopt(rpmgit git, int argc, char *argv[], poptOption opts)
{
    static int _popt_flags = POPT_CONTEXT_POSIXMEHARDER;
    int rc;
int xx;

if (_rpmgit_debug) argvPrint("before", (ARGV_t)argv, NULL);
#if defined(WITH_LIBGIT2)
if (_rpmgit_debug && git->R) rpmgitPrintRepo(git, git->R, git->fp);
#endif	/* defined(WITH_LIBGIT2) */

    git->con = poptFreeContext(git->con);	/* XXX necessary? */
    git->con = poptGetContext(argv[0], argc, (const char **)argv, opts, _popt_flags);

    while ((rc = poptGetNextOpt(git->con)) > 0) {
	const char * arg = poptGetOptArg(git->con);
	arg = _free(arg);
    }
    if (rc < -1) {
        fprintf(stderr, "%s: %s: %s\n", argv[0],
                poptBadOption(git->con, POPT_BADOPTION_NOALIAS),
                poptStrerror(rc));
	git->con = poptFreeContext(git->con);
    }

    git->av = argvFree(git->av);	/* XXX necessary? */
    if (git->con)
	xx = argvAppend(&git->av, (ARGV_t)poptGetArgs(git->con));
    git->ac = argvCount(git->av);
if (_rpmgit_debug) argvPrint(" after", git->av, NULL);
    return rc;
}

/*==============================================================*/
/* parse the tail of the --shared= argument */
#if defined(WITH_LIBGIT2)
static uint32_t parse_shared(const char * shared)
{
    if (!strcmp(shared, "false")
     || !strcmp(shared, "umask"))
	return GIT_REPOSITORY_INIT_SHARED_UMASK;
    else if (!strcmp(shared, "true")
	  || !strcmp(shared, "group"))
	return GIT_REPOSITORY_INIT_SHARED_GROUP;
    else if (!strcmp(shared, "all")
	  || !strcmp(shared, "world")
	  || !strcmp(shared, "everybody"))
	return GIT_REPOSITORY_INIT_SHARED_ALL;
    else if (shared[0] == '0') {
	char *end = NULL;
	long val = strtol(shared + 1, &end, 8);	/* XXX permit other bases */
	if (end == shared + 1 || *end != 0)
	    goto exit;	/* XXX error on trailing crap */
	return (uint32_t)val;
    }
	/* XXX error on unknown keyword */

exit:
    return 0;
}

/*
 * Unlike regular "git init", this example shows how to create an initial
 * empty commit in the repository.  This is the helper function that does
 * that.
 */
static int create_initial_commit(rpmgit git, void * ___R)
{
    FILE * fp = (git->fp ? git->fp : stderr);
    git_oid Toid;
    git_oid Coid;
    int xx;

    /* First use the config to initialize a commit signature for the user */
    xx = chkgit(git, "git_signature_default",
		git_signature_default((git_signature **)&git->S, git->R));
    if (xx < 0) {
	fprintf(fp,	"Unable to create a commit signature.\n"
			"Perhaps 'user.name' and 'user.email' are not set\n");
	goto exit;
    }

    /* Now let's create an empty tree for this commit */
    xx = chkgit(git, "git_repository_index",
		git_repository_index((git_index **)&git->I, git->R));
    if (xx < 0) {
	fprintf(fp, "Could not open repository index\n");
	goto exit;
    }

    /* Outside of this example, you could call git_index_add_bypath()
     * here to put actual files into the index.  For our purposes, we'll
     * leave it empty for now.
     */
    xx = chkgit(git, "git_index_write_tree",
		git_index_write_tree(&Toid, git->I));
    if (xx < 0) {
	fprintf(fp, "Unable to write initial tree from index\n");
	goto exit;
    }

    git_index_free(git->I);
    git->I = NULL;

    xx = chkgit(git, "git_tree_lookup",
		git_tree_lookup((git_tree **)&git->T, git->R, &Toid));
    if (xx < 0) {
	fprintf(fp, "Could not look up initial tree\n");
	goto exit;
    }

    /* Ready to create the initial commit
     *
     * Normally creating a commit would involve looking up the current
     * HEAD commit and making that be the parent of the initial commit,
     * but here this is the first commit so there will be no parent.
     */
    xx = chkgit(git, "git_commit_create_v",
		git_commit_create_v(&Coid, git->R, "HEAD", git->S, git->S,
			NULL, "RPM init commit", git->T, 0));
    if (xx < 0) {
	fprintf(fp, "Could not create the initial commit\n");
	goto exit;
    }

    xx = 0;

exit:
    if (git->I) {
	git_signature_free(git->I);
	git->I = NULL;
    }
    /* Clean up so we don't leak memory */
    if (git->T) {
	git_tree_free(git->T);
	git->T = NULL;
    }
    if (git->S) {
	git_signature_free(git->S);
	git->S = NULL;
    }
    return xx;
}
#endif	/* defined(WITH_LIBGIT2) */

rpmRC rpmgitCmdInit(int argc, char *argv[])
{
    int rc = RPMRC_FAIL;
#if defined(WITH_LIBGIT2)
    const char * init_template = NULL;
    const char * init_shared = NULL;
    const char * init_gitdir = NULL;
    enum {
	_INIT_QUIET	= (1 << 0),
	_INIT_BARE	= (1 << 1),
	_INIT_COMMIT	= (1 << 2),
    };
    int init_flags = 0;
#define	INIT_ISSET(_a)	(init_flags & _INIT_##_a)
    struct poptOption initOpts[] = {
     { "bare", '\0', POPT_BIT_SET,		&init_flags, _INIT_BARE,
	N_("Create a bare repository."), NULL },
     { "template", '\0', POPT_ARG_STRING,	&init_template, 0,
	N_("Specify the <template> directory."), N_("<template>") },
	/* XXX POPT_ARGFLAG_OPTIONAL */
     { "shared", '\0', POPT_ARG_STRING,		&init_shared, 0,
	N_("Specify how the git repository is to be shared."),
	N_("{false|true|umask|group|all|world|everybody|0xxx}") },
     { "separate-git-dir", '\0', POPT_ARG_STRING, &init_gitdir, 0,
	N_("Specify a separate <gitdir> directory."), N_("<gitdir>") },
     { "quiet", 'q', POPT_BIT_SET,		&init_flags, _INIT_QUIET,
	N_("Quiet mode."), NULL },
     { "initial-commit", '\0', POPT_BIT_SET,	&init_flags, _INIT_COMMIT,
	N_("Initial commit.."), NULL },

      POPT_AUTOALIAS
      POPT_AUTOHELP
      POPT_TABLEEND
    };
    rpmgit git = rpmgitNew(argv, 0x80000000, initOpts);
git_repository_init_options opts = GIT_REPOSITORY_INIT_OPTIONS_INIT;
	/* XXX git->repodir? */
const char * dir = git->fn;	/* XXX */
    int xx = -1;
    int i;

if (_rpmgit_debug)
fprintf(stderr, "==> %s(%p[%d]) git %p flags 0x%x\n", __FUNCTION__, argv, argc, git, init_flags);

	/* XXX template? */
	/* XXX parse shared to fmode/dmode. */
	/* XXX quiet? */
	/* XXX separate-git-dir? */
	/* XXX initial-commit? */

    /* Impedance match the options. */
    opts.flags = GIT_REPOSITORY_INIT_MKPATH;
    if (INIT_ISSET(BARE)) {
	git->is_bare = 1;
	opts.flags |= GIT_REPOSITORY_INIT_BARE;
    } else
	git->is_bare = 0;
    if (init_template) {
	opts.flags |= GIT_REPOSITORY_INIT_EXTERNAL_TEMPLATE;
	opts.template_path = init_template;
    }
    if (init_gitdir) {
	/* XXX use git->fn to eliminate dir. xstrdup? */
	opts.workdir_path = dir;
	dir = init_gitdir;
    }
    if (init_shared)
	git->shared_umask = parse_shared(init_shared);
    else
	git->shared_umask = GIT_REPOSITORY_INIT_SHARED_UMASK;

    /* Initialize a git repository. */
    xx = rpmgitInit(git, &opts);
    if (xx)
	goto exit;

	/* XXX elsewhere */
    if (!INIT_ISSET(QUIET)) {
	if (git->is_bare || init_gitdir)
	    dir = git_repository_path(git->R);
	else
	    dir = git_repository_workdir(git->R);
	printf("Initialized empty Git repository in %s\n", dir);
    }
    
	/* XXX elsewhere */
    /* Read/print/save configuration info. */
    xx = rpmgitConfig(git);
    if (xx)
	goto exit;

    /* As an extension to the basic "git init" command, this example
     * gives the option to create an empty initial commit.  This is
     * mostly to demonstrate what it takes to do that, but also some
     * people like to have that empty base commit in their repo.
     */
    if (INIT_ISSET(COMMIT)) {
	xx = create_initial_commit(git, git->R);
	if (xx)
	    goto exit;
	printf("Created empty initial commit\n");
	goto exit;
    }

    /* XXX automagic add and commit (for now) */
    if (git->ac <= 0)
	goto exit;

    /* Create file(s) in _workdir (if any). */
    for (i = 0; i < git->ac; i++) {
	const char * fn = git->av[i];

	/* Add the file to the repository. */
	xx = rpmgitAddFile(git, fn);
	if (xx)
	    goto exit;
    }

    /* Commit added files. */
    if (git->ac > 0) {
	static const char _msg[] = "WDJ commit";
	xx = rpmgitCommit(git, _msg);
    }
    if (xx)
	goto exit;
rpmgitPrintCommit(git, git->C, git->fp);

rpmgitPrintIndex(git->I, git->fp);
rpmgitPrintHead(git, NULL, git->fp);

exit:
    rc = (xx ? RPMRC_FAIL : RPMRC_OK);
SPEW(0, rc, git);
    init_gitdir = _free(init_gitdir);
    init_shared = _free(init_shared);
    init_template = _free(init_template);

    git = rpmgitFree(git);
#endif	/* defined(WITH_LIBGIT2) */
    return rc;
}
#undef	INIT_ISSET

/*==============================================================*/

#if defined(WITH_LIBGIT2)
static int print_matched_cb(const char * fn, const char *matched_pathspec,
		     void * _git)
{
    FILE * fp = stdout;
    rpmgit git = (rpmgit) _git;
    git_status_t status;
    int rc = -1;	/* XXX assume abort */
    int xx;

    (void) matched_pathspec;

    xx = chkgit(git, "git_status_file",
	git_status_file((unsigned int *) &status, git->R, fn));
    if (xx)
	goto exit;

    if (status & GIT_STATUS_WT_MODIFIED || status & GIT_STATUS_WT_NEW) {
	fprintf(fp, "add '%s'\n",  fn);
	rc = 0;
	goto exit;
    }
    rc = 1;

exit:
SPEW(0, rc, git);
    return rc;
}
#endif

#ifdef	REFERENCE
OPTIONS
       <filepattern>...
           Files to add content from. Fileglobs (e.g.  *.c) can be given to
           add all matching files. Also a leading directory name (e.g.  dir to
           add dir/file1 and dir/file2) can be given to add all files in the
           directory, recursively.

       -n, --dry-run
           Don’t actually add the file(s), just show if they exist.

       -v, --verbose
           Be verbose.

       -f, --force
           Allow adding otherwise ignored files.

       -i, --interactive
           Add modified contents in the working tree interactively to the
           index. Optional path arguments may be supplied to limit operation
           to a subset of the working tree. See “Interactive mode” for
           details.

       -p, --patch
           Interactively choose hunks of patch between the index and the work
           tree and add them to the index. This gives the user a chance to
           review the difference before adding modified contents to the index.

           This effectively runs add --interactive, but bypasses the initial
           command menu and directly jumps to the patch subcommand. See
           “Interactive mode” for details.

       -e, --edit
           Open the diff vs. the index in an editor and let the user edit it.
           After the editor was closed, adjust the hunk headers and apply the
           patch to the index.

           NOTE: Obviously, if you change anything else than the first
           character on lines beginning with a space or a minus, the patch
           will no longer apply.

       -u, --update
           Only match <filepattern> against already tracked files in the index
           rather than the working tree. That means that it will never stage
           new files, but that it will stage modified new contents of tracked
           files and that it will remove files from the index if the
           corresponding files in the working tree have been removed.

           If no <filepattern> is given, default to "."; in other words,
           update all tracked files in the current directory and its
           subdirectories.

       -A, --all
           Like -u, but match <filepattern> against files in the working tree
           in addition to the index. That means that it will find new files as
           well as staging modified content and removing files that are no
           longer in the working tree.

       -N, --intent-to-add
           Record only the fact that the path will be added later. An entry
           for the path is placed in the index with no content. This is useful
           for, among other things, showing the unstaged content of such files
           with git diff and committing them with git commit -a.

       --refresh
           Don’t add the file(s), but only refresh their stat() information in
           the index.

       --ignore-errors
           If some files could not be added because of errors indexing them,
           do not abort the operation, but continue adding the others. The
           command shall still exit with non-zero status.

       --
           This option can be used to separate command-line options from the
           list of files, (useful when filenames might be mistaken for
           command-line options).
#endif


rpmRC rpmgitCmdAdd(int argc, char *argv[])
{
    int rc = RPMRC_FAIL;
#if defined(WITH_LIBGIT2)
    enum {
	_ADD_SKIP		= (1 <<  0),
	_ADD_VERBOSE		= (1 <<  1),
	_ADD_FORCE		= (1 <<  2),
	_ADD_INTERACTIVE	= (1 <<  3),
	_ADD_PATCH		= (1 <<  4),
	_ADD_EDIT		= (1 <<  5),
	_ADD_UPDATE		= (1 <<  6),
	_ADD_ALL		= (1 <<  7),
	_ADD_INTENT_TO_ADD	= (1 <<  8),
	_ADD_REFRESH		= (1 <<  9),
	_ADD_IGNORE_ERRORS	= (1 << 10),
    };
    int add_flags = 0;
#define	ADD_ISSET(_a)	(add_flags & _ADD_##_a)
    struct poptOption addOpts[] = {
     { "dry-run", 'n', POPT_BIT_SET,		&add_flags, _ADD_SKIP,
	(""), NULL },
     { "verbose", 'v', POPT_BIT_SET,		&add_flags, _ADD_VERBOSE,
	N_("Verbose mode."), NULL },
     { "force", 'f', POPT_BIT_SET,		&add_flags, _ADD_FORCE,
	(""), NULL },
     { "interactive", 'i', POPT_BIT_SET,	&add_flags, _ADD_INTERACTIVE,
	(""), NULL },
     { "patch", 'p', POPT_BIT_SET,		&add_flags, _ADD_PATCH,
	(""), NULL },
     { "edit", 'e', POPT_BIT_SET,		&add_flags, _ADD_EDIT,
	(""), NULL },
     { "update", 'u', POPT_BIT_SET,		&add_flags, _ADD_UPDATE,
	(""), NULL },
     { "all", 'A', POPT_BIT_SET,		&add_flags, _ADD_ALL,
	(""), NULL },
     { "intent-to-add", 'N', POPT_BIT_SET,	&add_flags, _ADD_INTENT_TO_ADD,
	(""), NULL },
     { "refresh", '\0', POPT_BIT_SET,		&add_flags, _ADD_REFRESH,
	(""), NULL },
     { "ignore-errors", '\0', POPT_BIT_SET,	&add_flags, _ADD_IGNORE_ERRORS,
	(""), NULL },
      POPT_AUTOALIAS
      POPT_AUTOHELP
      POPT_TABLEEND
    };
    rpmgit git = rpmgitNew(argv, 0, addOpts);
    int xx = -1;

    git_index_matched_path_cb matched_cb = NULL;

    git_strarray array = {0};
    array.strings = (char **) git->av;
    array.count = git->ac;

    if (ADD_ISSET(VERBOSE) || ADD_ISSET(SKIP))
	matched_cb = print_matched_cb;

    /* XXX Get the index file for this repository. */
    xx = chkgit(git, "git_repository_index",
		git_repository_index((git_index **)&git->I, git->R));
    if (xx)
	goto exit;

if (_rpmgit_debug < 0) rpmgitPrintIndex(git->I, git->fp);
    if (ADD_ISSET(UPDATE))
	xx = chkgit(git, "git_index_update_all",
                git_index_update_all(git->I, &array, matched_cb, git));
    else
	xx = chkgit(git, "git_index_add_all",
                git_index_add_all(git->I, &array, 0, matched_cb, git));
if (_rpmgit_debug < 0) rpmgitPrintIndex(git->I, git->fp);

    xx = chkgit(git, "git_index_write",
		git_index_write(git->I));

exit:
    rc = (xx ? RPMRC_FAIL : RPMRC_OK);
SPEW(0, rc, git);

    if (git->I)
        git_index_free(git->I);
    git->I = NULL;
    git = rpmgitFree(git);
#endif	/* defined(WITH_LIBGIT2) */
    return rc;
}
#undef	ADD_ISSET

/*==============================================================*/

rpmRC rpmgitCmdCommit(int argc, char *argv[])
{
    int rc = RPMRC_FAIL;
#if defined(WITH_LIBGIT2)
    const char * commit_file = NULL;		/* XXX argv? */
    const char * commit_author = NULL;
    const char * commit_date = NULL;
    const char * commit_msg = NULL;
    const char * commit_template = NULL;
    const char * commit_cleanup = NULL;
    const char * commit_untracked = NULL;
    enum {
	_COMMIT_ALL		= (1 <<  0),
	_COMMIT_RESET_AUTHOR	= (1 <<  1),
	_COMMIT_SHORT		= (1 <<  2),
	_COMMIT_PORCELAIN	= (1 <<  3),
	_COMMIT_ZERO		= (1 <<  4),
	_COMMIT_SIGNOFF		= (1 <<  5),
	_COMMIT_NO_VERIFY	= (1 <<  6),
	_COMMIT_ALLOW_EMPTY	= (1 <<  7),
	_COMMIT_EDIT		= (1 <<  8),
	_COMMIT_AMEND		= (1 <<  9),
	_COMMIT_INCLUDE		= (1 << 10),
	_COMMIT_ONLY		= (1 << 11),
	_COMMIT_VERBOSE		= (1 << 12),
	_COMMIT_QUIET		= (1 << 13),
	_COMMIT_DRY_RUN		= (1 << 14),
	_COMMIT_STATUS		= (1 << 15),
	_COMMIT_NO_STATUS	= (1 << 16),
    };
    int commit_flags = 0;
#define	COMMIT_ISSET(_a)	(commit_flags & _COMMIT_##_a)
    struct poptOption commitOpts[] = {
     { "all", 'a', POPT_BIT_SET,		&commit_flags, _COMMIT_ALL,
	(""), NULL },
	/* XXX -C */
	/* XXX -c */
     { "reset-author", '\0', POPT_BIT_SET,	&commit_flags, _COMMIT_RESET_AUTHOR,
	(""), NULL },
     { "short", '\0', POPT_BIT_SET,		&commit_flags, _COMMIT_SHORT,
	(""), NULL },
     { "porcelain", '\0', POPT_BIT_SET,		&commit_flags, _COMMIT_PORCELAIN,
	(""), NULL },
     { NULL, 'z', POPT_BIT_SET,			&commit_flags, _COMMIT_ZERO,
	(""), NULL },
     { "file", 'F', POPT_ARG_STRING,		&commit_file, 0,
	(""), NULL },
     { "author", '\0', POPT_ARG_STRING,		&commit_author, 0,
	(""), NULL },
     { "date", '\0', POPT_ARG_STRING,		&commit_date, 0,
	(""), NULL },
     { "message", 'm', POPT_ARG_STRING,		&commit_msg, 0,
	(""), NULL },
     { "template", 't', POPT_ARG_STRING,	&commit_template, 0,
	(""), NULL },
     { "signoff", 's', POPT_BIT_SET,		&commit_flags, _COMMIT_SIGNOFF,
	(""), NULL },
     { "no-verify", 'n', POPT_BIT_SET,		&commit_flags, _COMMIT_NO_VERIFY,
	(""), NULL },
     { "allow-empty", '\0', POPT_BIT_SET,	&commit_flags, _COMMIT_ALLOW_EMPTY,
	(""), NULL },
     { "cleanup", '\0', POPT_ARG_STRING,	&commit_cleanup, 0,
	(""), NULL },
     { "edit", 'e', POPT_BIT_SET,		&commit_flags, _COMMIT_EDIT,
	(""), NULL },
     { "amend", '\0', POPT_BIT_SET,		&commit_flags, _COMMIT_AMEND,
	(""), NULL },
     { "include", 'i', POPT_BIT_SET,		&commit_flags, _COMMIT_INCLUDE,
	(""), NULL },
     { "only", 'o', POPT_BIT_SET,		&commit_flags, _COMMIT_ONLY,
	(""), NULL },
     { "untracked", 'u', POPT_ARG_STRING,	&commit_untracked, 0,
	(""), NULL },
     { "verbose", 'v', POPT_BIT_SET,		&commit_flags, _COMMIT_VERBOSE,
	N_("Verbose mode."), NULL },
     { "quiet", 'q', POPT_BIT_SET,		&commit_flags, _COMMIT_QUIET,
	N_("Quiet mode."), NULL },
     { "dry-run", '\n', POPT_BIT_SET,		&commit_flags, _COMMIT_DRY_RUN,
	(""), NULL },

     { "status", '\0', POPT_BIT_SET,		&commit_flags, _COMMIT_STATUS,
	(""), NULL },
     { "no-status", '\0', POPT_BIT_SET,		&commit_flags, _COMMIT_NO_STATUS,
	(""), NULL },

      POPT_AUTOALIAS
      POPT_AUTOHELP
      POPT_TABLEEND
    };
    rpmgit git = rpmgitNew(argv, 0, commitOpts);
    int xx = -1;

    /* XXX Get the index file for this repository. */
    xx = chkgit(git, "git_repository_index",
		git_repository_index((git_index **)&git->I, git->R));
    if (xx)
	goto exit;

    /* Commit changes. */
    if (commit_msg == NULL) commit_msg = xstrdup("WDJ commit");	/* XXX */
    xx = rpmgitCommit(git, commit_msg);
    if (xx)
	goto exit;
rpmgitPrintCommit(git, git->C, git->fp);

rpmgitPrintIndex(git->I, git->fp);
rpmgitPrintHead(git, NULL, git->fp);

exit:
    rc = (xx ? RPMRC_FAIL : RPMRC_OK);
SPEW(0, rc, git);
    commit_file = _free(commit_file);		/* XXX argv? */
    commit_author = _free(commit_author);
    commit_date = _free(commit_date);
    commit_msg = _free(commit_msg);
    commit_template = _free(commit_template);
    commit_cleanup = _free(commit_cleanup);
    commit_untracked = _free(commit_untracked);

    git = rpmgitFree(git);
#endif	/* defined(WITH_LIBGIT2) */
    return rc;
}
#undef	COMMIT_ISSET

/*==============================================================*/

#if defined(WITH_LIBGIT2)
static int diff_output( const git_diff_delta *d, const git_diff_hunk *h,
		const git_diff_line *l, void *p)
{
    FILE *fp = (FILE*)p;
    size_t nw;

    (void)d; (void)h;

    if (fp == NULL)
	fp = stdout;

    if (l->origin == GIT_DIFF_LINE_CONTEXT ||
	l->origin == GIT_DIFF_LINE_ADDITION ||
	l->origin == GIT_DIFF_LINE_DELETION)
	fputc(l->origin, fp);

    nw = fwrite(l->content, 1, l->content_len, fp);

    return 0;
}

static int treeish_to_tree(git_tree ** tree, git_repository * repo,
		const char * treeish)
{
    git_object *obj = NULL;
    int err = 0;

    if ((err = git_revparse_single(&obj, repo, treeish)) < 0)
	return err;

    if ((err = git_object_peel((git_object **)tree, obj, GIT_OBJ_TREE)) < 0)
	return err;

    git_object_free(obj);

    return err;
}

static char *colors[] = {
    "\033[m",			/* reset */
    "\033[1m",			/* bold */
    "\033[31m",			/* red */
    "\033[32m",			/* green */
    "\033[36m"			/* cyan */
};

enum {
    OUTPUT_DIFF = (1 << 0),
    OUTPUT_STAT = (1 << 1),
    OUTPUT_SHORTSTAT = (1 << 2),
    OUTPUT_NUMSTAT = (1 << 3),
    OUTPUT_SUMMARY = (1 << 4)
};

enum {
    CACHE_NORMAL = 0,
    CACHE_ONLY = 1,
    CACHE_NONE = 2
};

/** The 'opts' struct captures all the various parsed command line options. */
struct opts {
    git_diff_options diffopts;
    git_diff_find_options findopts;
    int color;
    int cache;
    int output;
    git_diff_format_t format;
    const char *treeish1;
    const char *treeish2;
    const char *dir;
};

static int color_printer(
		const git_diff_delta *delta,
		const git_diff_hunk *hunk,
		const git_diff_line *line,
		void * data)
{
    int *last_color = data;
    int color = 0;

    if (*last_color >= 0) {
	switch (line->origin) {
	case GIT_DIFF_LINE_ADDITION:	color = 3; break;
	case GIT_DIFF_LINE_DELETION:	color = 2; break;
	case GIT_DIFF_LINE_ADD_EOFNL:	color = 3; break;
	case GIT_DIFF_LINE_DEL_EOFNL:	color = 2; break;
	case GIT_DIFF_LINE_FILE_HDR:	color = 1; break;
	case GIT_DIFF_LINE_HUNK_HDR:	color = 4; break;
	default: break;
	}
	if (color != *last_color) {
	    if (*last_color == 1 || color == 1)
		fputs(colors[0], stdout);
	    fputs(colors[color], stdout);
	    *last_color = color;
	}
    }

    return diff_output(delta, hunk, line, stdout);
}

/** Display diff output with "--stat", "--numstat", or "--shortstat" */
static void diff_print_stats(rpmgit git, git_diff *diff, struct opts *o)
{
    git_diff_stats *stats;
    git_buf b = GIT_BUF_INIT_CONST(NULL, 0);
    git_diff_stats_format_t format = 0;
    int xx;

    xx = chkgit(git, "git_diff_get_stats",
		git_diff_get_stats(&stats, diff));

    if (o->output & OUTPUT_STAT)
		format |= GIT_DIFF_STATS_FULL;
    if (o->output & OUTPUT_SHORTSTAT)
		format |= GIT_DIFF_STATS_SHORT;
    if (o->output & OUTPUT_NUMSTAT)
		format |= GIT_DIFF_STATS_NUMBER;
    if (o->output & OUTPUT_SUMMARY)
		format |= GIT_DIFF_STATS_INCLUDE_SUMMARY;

    xx = chkgit(git, "git_diff_stats_to_buf",
		git_diff_stats_to_buf(&b, stats, format, 80));

    fputs(b.ptr, stdout);

    git_buf_free(&b);
    git_diff_stats_free(stats);
}

#endif	/* defined(WITH_LIBGIT2) */

rpmRC rpmgitCmdDiff(int argc, char *argv[])
{
    int rc = RPMRC_FAIL;
#if defined(WITH_LIBGIT2)
    struct opts o = {
	GIT_DIFF_OPTIONS_INIT, GIT_DIFF_FIND_OPTIONS_INIT,
	-1, 0, OUTPUT_DIFF, GIT_DIFF_FORMAT_PATCH, NULL, NULL, "."
    };
    enum {	/* XXX FIXME */
	_DIFF_ALL		= (1 <<  0),
	_DIFF_ZERO		= (1 <<  1),
    };
    int diff_flags = 0;
#define	DIFF_ISSET(_a)	(o.diffopts.flags & GIT_DIFF_##_a)
    struct poptOption diffOpts[] = {
	/* XXX -u */
     { "patch", 'p', POPT_ARG_VAL,		&o.format, GIT_DIFF_FORMAT_PATCH,
	N_("Generate patch."), NULL },

     { "cached", '\0', POPT_ARG_VAL,		&o.cache, CACHE_ONLY,
	NULL, NULL },
     { "nocache", '\0', POPT_ARG_VAL,		&o.cache, CACHE_NONE,
	NULL, NULL },

	/* XXX --format=diff-index */
	/* XXX --format=raw */
     { "raw", '\0', POPT_ARG_VAL,		&o.format, GIT_DIFF_FORMAT_RAW,
	(""), NULL },
	/* XXX --patch-with-raw */
	/* XXX --patch-with-stat */
     { NULL, 'z', POPT_BIT_SET,			&diff_flags, _DIFF_ZERO,
	(""), NULL },
	/* XXX --format=name */
     { "name-only", '\0', POPT_ARG_VAL,		&o.format, GIT_DIFF_FORMAT_NAME_ONLY,
	N_("Show only names of changed files."), NULL },
	/* XXX --format=name-status */
     { "name-status", '\0', POPT_ARG_VAL,	&o.format, GIT_DIFF_FORMAT_NAME_STATUS,
	N_("Show only names and status of changed files."), NULL },
	/* XXX --submodule */

     { "color", '\0', POPT_ARG_VAL,		&o.color, 0,
	N_("Show colored diff."), NULL },
     { "no-color", '\0', POPT_ARG_VAL,		&o.color, -1,
	N_("Turn off colored diff."), NULL },
	/* XXX --color-words */
	/* XXX --no-renames */
	/* XXX --check */
	/* XXX --full-index */
	/* XXX --binary */
	/* XXX --diff-filter */
	/* XXX -l */
	/* XXX -S */
	/* XXX --pickaxe-all */
	/* XXX --pickaxe-regex */
	/* XXX -O */
     { NULL, 'R', POPT_BIT_SET,			&o.diffopts.flags, GIT_DIFF_REVERSE,
	N_("Swap two inputs."), NULL },
	/* XXX --relative */
     { "text", 'a', POPT_BIT_SET,		&o.diffopts.flags, GIT_DIFF_FORCE_TEXT,
	N_("Treat all files as text."), NULL },
     { "ignore-space-at-eol", '\0',	POPT_BIT_SET, &o.diffopts.flags, GIT_DIFF_IGNORE_WHITESPACE_EOL,
	N_("Ignore changes in whitespace at EOL."), NULL },
     { "ignore-space-change", 'b',	POPT_BIT_SET, &o.diffopts.flags, GIT_DIFF_IGNORE_WHITESPACE_CHANGE,
	N_("Ignore changes in amount of whitespace."), NULL },
     { "ignore-all-space", 'w', POPT_BIT_SET, &o.diffopts.flags, GIT_DIFF_IGNORE_WHITESPACE,
	N_("Ignore whitespace when comparing lines."), NULL },
     { "ignored", '\0', POPT_BIT_SET,	&o.diffopts.flags, GIT_DIFF_INCLUDE_IGNORED,
	NULL, NULL },
     { "untracked", '\0', POPT_BIT_SET,	&o.diffopts.flags, GIT_DIFF_INCLUDE_UNTRACKED,
	NULL, NULL },
     { "patience", '\0', POPT_BIT_SET,		&o.diffopts.flags, GIT_DIFF_PATIENCE,
	N_("Generate a diff using the \"patience diff\" algorithm."), NULL },
     { "minimal", '\0', POPT_BIT_SET,		&o.diffopts.flags, GIT_DIFF_MINIMAL,
	N_("Generate a minimal diff."), NULL },

     { "stat", '\0', POPT_BIT_SET,		&o.output, OUTPUT_STAT,
	NULL, NULL },
     { "numstat", '\0', POPT_BIT_SET,		&o.output, OUTPUT_NUMSTAT,
	NULL, NULL },
     { "shortstat", '\0', POPT_BIT_SET,		&o.output, OUTPUT_SHORTSTAT,
	NULL, NULL },
	/* XXX --dirstat */
	/* XXX --dirstat-by-file */
     { "summary", '\0', POPT_ARG_VAL,		&o.output, OUTPUT_SUMMARY,
	NULL, NULL },

     { "find-renames", 'M', POPT_ARG_SHORT,	&o.findopts.rename_threshold, 0,
	(""), NULL },
     { "find-copies", 'C', POPT_ARG_SHORT,	&o.findopts.copy_threshold, 0,
	(""), NULL },
     { "find-copies-harder", '\0', POPT_BIT_SET, &o.findopts.flags, GIT_DIFF_FIND_COPIES_FROM_UNMODIFIED,
	(""), NULL },
	/* TODO: parse thresholds */
     { "break-rewrites", 'B', POPT_BIT_SET, &o.findopts.flags, GIT_DIFF_FIND_REWRITES,
	(""), NULL },
     { "unified", 'U', POPT_ARG_SHORT,		&o.diffopts.context_lines, 0,
	N_("Generate diffs with <n> lines of context."), N_("<n>") },

     { "inter-hunk-context", '\0', POPT_ARG_SHORT,	&o.diffopts.interhunk_lines, 0,
	N_("Show the context between diff hunks."), N_("<lines>") },
	/* XXX --abbrev */
     { "abbrev", '\0', POPT_ARG_SHORT,	&o.diffopts.interhunk_lines, 0,
	(""), N_("<XXX>") },

	/* XXX --exit-code */
	/* XXX --quiet */
	/* XXX --ext-diff */
	/* XXX --no-ext-diff */
     { "ignore-submodules", '\0', POPT_BIT_SET,	&o.diffopts.flags, GIT_DIFF_IGNORE_SUBMODULES,
	N_("Ignore changes to submodules in the diff generation."), NULL },
     { "src-prefix", '\0', POPT_ARG_STRING,	&o.diffopts.old_prefix, 0,
	N_("Show the given source <prefix> instead of \"a/\"."), N_("<prefix>") },
     { "dst-prefix", '\0', POPT_ARG_STRING,	&o.diffopts.new_prefix, 0,
	N_("Show the given destination prefix instead of \"b/\"."), N_("<prefix>") },
	/* XXX --no-prefix */
	/* XXX --git-dir */

	/* XXX --untracked-dirs? */

      POPT_AUTOALIAS
      POPT_AUTOHELP
      POPT_TABLEEND
    };
    rpmgit git = NULL;		/* XXX rpmgitNew(argv, 0, diffOpts); */
git_diff * diff = NULL;
const char * treeish1 = NULL;
git_tree *t1 = NULL;
const char * treeish2 = NULL;
git_tree *t2 = NULL;
    int xx = -1;

#ifdef	NOTYET
const char * dir = ".";
char path[GIT_PATH_MAX];
const char * fn;
    xx = chkgit(git, "git_repository_discover",
	git_repository_discover(path, sizeof(path), dir, 0, "/"));
    if (xx) {
	fprintf(stderr, "Could not discover repository\n");
	goto exit;
    }
    fn = path;
#endif

    if (o.findopts.rename_threshold)
	o.findopts.flags |= GIT_DIFF_FIND_RENAMES;
    if (o.findopts.copy_threshold)
	o.findopts.flags |= GIT_DIFF_FIND_COPIES;

    git = rpmgitNew(argv, 0, diffOpts);

    treeish1 = (git->ac >= 1 ? git->av[0] : NULL);
    treeish2 = (git->ac >= 2 ? git->av[1] : NULL);

    if (treeish1) {
	xx = chkgit(git, "treeish_to_tree",
		treeish_to_tree(&t1, git->R, treeish1));
	if (xx) {
	    fprintf(stderr, "Looking up first tree\n");
	    goto exit;
	}
    }
    if (treeish2) {
	xx = chkgit(git, "treeish_to_tree",
		treeish_to_tree(&t2, git->R, treeish2));
	if (xx) {
	    fprintf(stderr, "Looking up second tree\n");
	    goto exit;
	}
    }

    /* <sha1> <sha2> */
    /* <sha1> --cached */
    /* <sha1> */
    /* --cached */
    /* nothing */

    if (t1 && t2)
	xx = chkgit(git, "git_diff_tree_to_tree",
		git_diff_tree_to_tree(&diff, git->R, t1, t2, &o.diffopts));
    else if (o.cache != CACHE_NORMAL) {
	if (t1 == NULL)
	    xx = chkgit(git, "treeish_to_tree",
			treeish_to_tree(&t1, git->R, "HEAD"));

	    if (o.cache == CACHE_NONE)
		xx = chkgit(git, "git_diff_tree_to_workdir",
			git_diff_tree_to_workdir(&diff, git->R, t1, &o.diffopts));
	    else
		xx = chkgit(git, "git_diff_tree_to_index",
			git_diff_tree_to_index(&diff, git->R, t1, NULL, &o.diffopts));
    }
    else if (t1)
	xx = chkgit(git, "git_diff_tree_to_index",
		git_diff_tree_to_index(&diff, git->R, t1, NULL, &o.diffopts));
    else if (t1)
	xx = chkgit(git, "git_diff_tree_to_workdir_with_index",
		git_diff_tree_to_workdir_with_index(&diff, git->R, t1, &o.diffopts));
    else
	xx = chkgit(git, "git_diff_index_to_workdir",
		git_diff_index_to_workdir(&diff, git->R, NULL, &o.diffopts));

    /** Apply rename and copy detection if requested. */
    if ((o.findopts.flags & GIT_DIFF_FIND_ALL) != 0)
	xx = chkgit(git, "git_diff_find_similar",
		git_diff_find_similar(diff, &o.findopts));

    /** Generate simple output using libgit2 display helper. */

    if (o.output != OUTPUT_DIFF)
	diff_print_stats(git, diff, &o);

    if (o.output & OUTPUT_DIFF)  {
	if (o.color >= 0)
	    fputs(colors[0], stdout);

	xx = chkgit(git, "git_diff_print",
		git_diff_print(diff, o.format, color_printer, &o.color));

	if (o.color >= 0)
	    fputs(colors[0], stdout);
    }

exit:
    rc = (xx ? RPMRC_FAIL : RPMRC_OK);
SPEW(0, rc, git);

    /** Cleanup before exiting. */
    if (diff)
	git_diff_free(diff);
    if (t1)
	git_tree_free(t1);
    if (t2)
	git_tree_free(t2);

    git = rpmgitFree(git);
#endif	/* defined(WITH_LIBGIT2) */
    return rc;
}
#undef	DIFF_ISSET

/*==============================================================*/

#if defined(WITH_LIBGIT2)
enum {
    FORMAT_DEFAULT	= 0,
    FORMAT_LONG		= 1,
    FORMAT_SHORT	= 2,
    FORMAT_PORCELAIN	= 3,
};

static void show_branch(git_repository *repo, int format)
{
    int error = 0;
    const char *branch = NULL;
    git_reference *head = NULL;

    error = git_repository_head(&head, repo);

    if (error == GIT_EUNBORNBRANCH || error == GIT_ENOTFOUND)
	branch = NULL;
    else if (!error)
	branch = git_reference_shorthand(head);
    else
	check(error, "failed to get current branch", NULL);

    if (format == FORMAT_LONG)
	printf("# On branch %s\n",
		branch ? branch : "Not currently on any branch.");
    else
	printf("## %s\n", branch ? branch : "HEAD (no branch)");

    git_reference_free(head);
}

static void print_long(git_repository *repo, git_status_list *status)
{
    size_t maxi = git_status_list_entrycount(status);
    size_t i;
    const git_status_entry *s;
    int header = 0;
    int changes_in_index = 0;
    int changed_in_workdir = 0;
    int rm_in_workdir = 0;
    const char *old_path;
    const char *new_path;

    (void)repo;

    /* print index changes */

    for (i = 0; i < maxi; ++i) {
	char *istatus = NULL;

	s = git_status_byindex(status, i);

	if (s->status == GIT_STATUS_CURRENT)
	    continue;

	if (s->status & GIT_STATUS_WT_DELETED)
	    rm_in_workdir = 1;

	if (s->status & GIT_STATUS_INDEX_NEW)
	    istatus = "new file: ";
	if (s->status & GIT_STATUS_INDEX_MODIFIED)
	    istatus = "modified: ";
	if (s->status & GIT_STATUS_INDEX_DELETED)
	    istatus = "deleted:  ";
	if (s->status & GIT_STATUS_INDEX_RENAMED)
	    istatus = "renamed:  ";
	if (s->status & GIT_STATUS_INDEX_TYPECHANGE)
	    istatus = "typechange:";

	if (istatus == NULL)
	    continue;

	if (!header) {
	    printf("# Changes to be committed:\n");
	    printf("#   (use \"git reset HEAD <file>...\" to unstage)\n");
	    printf("#\n");
	    header = 1;
	}

	old_path = s->head_to_index->old_file.path;
	new_path = s->head_to_index->new_file.path;

	if (old_path && new_path && strcmp(old_path, new_path))
	    printf("#\t%s  %s -> %s\n", istatus, old_path, new_path);
	else
	    printf("#\t%s  %s\n", istatus, old_path ? old_path : new_path);
    }

    if (header) {
	changes_in_index = 1;
	printf("#\n");
    }

    /* Print workdir changes to tracked files */
    header = 0;
    for (i = 0; i < maxi; ++i) {
	char *wstatus = NULL;

	s = git_status_byindex(status, i);

	/*
         * With `GIT_STATUS_OPT_INCLUDE_UNMODIFIED` (not used in this example)
         * `index_to_workdir` may not be `NULL` even if there are
         * no differences, in which case it will be a `GIT_DELTA_UNMODIFIED`.
         */
	if (s->status == GIT_STATUS_CURRENT || s->index_to_workdir == NULL)
	    continue;

	/* Print out the output since we know the file has some changes */

	if (s->status & GIT_STATUS_WT_MODIFIED)
	    wstatus = "modified: ";
	if (s->status & GIT_STATUS_WT_DELETED)
	    wstatus = "deleted:  ";
	if (s->status & GIT_STATUS_WT_RENAMED)
	    wstatus = "renamed:  ";
	if (s->status & GIT_STATUS_WT_TYPECHANGE)
	    wstatus = "typechange:";

	if (wstatus == NULL)
	    continue;

	if (!header) {
	    printf("# Changes not staged for commit:\n");
	    printf("#   (use \"git add%s <file>...\" to update what will be committed)\n", rm_in_workdir ? "/rm" : "");
	    printf("#   (use \"git checkout -- <file>...\" to discard changes in working directory)\n");
	    printf("#\n");
	    header = 1;
	}

	old_path = s->index_to_workdir->old_file.path;
	new_path = s->index_to_workdir->new_file.path;

	if (old_path && new_path && strcmp(old_path, new_path))
	    printf("#\t%s  %s -> %s\n", wstatus, old_path, new_path);
	else
	    printf("#\t%s  %s\n", wstatus, old_path ? old_path : new_path);
    }

    if (header) {
	changed_in_workdir = 1;
	printf("#\n");
    }

    /* Print untracked files */
    header = 0;
    for (i = 0; i < maxi; ++i) {
	s = git_status_byindex(status, i);

	if (s->status == GIT_STATUS_WT_NEW) {

	    if (!header) {
		printf("# Untracked files:\n");
		printf("#   (use \"git add <file>...\" to include in what will be committed)\n");
		printf("#\n");
		header = 1;
	    }

	    printf("#\t%s\n", s->index_to_workdir->old_file.path);
	}
    }

    /* Print ignored files */
    header = 0;
    for (i = 0; i < maxi; ++i) {
	s = git_status_byindex(status, i);

	if (s->status == GIT_STATUS_IGNORED) {

	    if (!header) {
		printf("# Ignored files:\n");
		printf("#   (use \"git add -f <file>...\" to include in what will be committed)\n");
		printf("#\n");
		header = 1;
	    }

	    printf("#\t%s\n", s->index_to_workdir->old_file.path);
	}
    }

    if (!changes_in_index && changed_in_workdir)
	printf("no changes added to commit (use \"git add\" and/or \"git commit -a\")\n");
}

static void print_short(git_repository *repo, git_status_list *status)
{
    size_t maxi = git_status_list_entrycount(status);
    size_t i;
    const git_status_entry *s;
    char istatus;
    char wstatus;
    const char *extra;
    const char *a;
    const char *b;
    const char *c;

    for (i = 0; i < maxi; ++i) {
	s = git_status_byindex(status, i);

	if (s->status == GIT_STATUS_CURRENT)
	    continue;

	a = b = c = NULL;
	istatus = wstatus = ' ';
	extra = "";

	if (s->status & GIT_STATUS_INDEX_NEW)
	    istatus = 'A';
	if (s->status & GIT_STATUS_INDEX_MODIFIED)
	    istatus = 'M';
	if (s->status & GIT_STATUS_INDEX_DELETED)
	    istatus = 'D';
	if (s->status & GIT_STATUS_INDEX_RENAMED)
	    istatus = 'R';
	if (s->status & GIT_STATUS_INDEX_TYPECHANGE)
	    istatus = 'T';

	if (s->status & GIT_STATUS_WT_NEW) {
	    if (istatus == ' ')
		istatus = '?';
	    wstatus = '?';
	}
	if (s->status & GIT_STATUS_WT_MODIFIED)
	    wstatus = 'M';
	if (s->status & GIT_STATUS_WT_DELETED)
	    wstatus = 'D';
	if (s->status & GIT_STATUS_WT_RENAMED)
	    wstatus = 'R';
	if (s->status & GIT_STATUS_WT_TYPECHANGE)
	    wstatus = 'T';

	if (s->status & GIT_STATUS_IGNORED) {
	    istatus = '!';
	    wstatus = '!';
	}

	if (istatus == '?' && wstatus == '?')
	    continue;

	/*
         * A commit in a tree is how submodules are stored, so
         * let's go take a look at its status.
         */
	if (s->index_to_workdir &&
	    s->index_to_workdir->new_file.mode == GIT_FILEMODE_COMMIT)
	{
	    git_submodule *sm = NULL;
	    unsigned int smstatus = 0;

	    if (!git_submodule_lookup( &sm, repo, s->index_to_workdir->new_file.path)
	     && !git_submodule_status(&smstatus, sm))
	    {
		if (smstatus & GIT_SUBMODULE_STATUS_WD_MODIFIED)
		    extra = " (new commits)";
		else if (smstatus & GIT_SUBMODULE_STATUS_WD_INDEX_MODIFIED)
		    extra = " (modified content)";
		else if (smstatus & GIT_SUBMODULE_STATUS_WD_WD_MODIFIED)
		    extra = " (modified content)";
		else if (smstatus & GIT_SUBMODULE_STATUS_WD_UNTRACKED)
		    extra = " (untracked content)";
	    }
	    git_submodule_free(sm);
	}

	/*
         * Now that we have all the information, format the output.
         */
	if (s->head_to_index) {
	    a = s->head_to_index->old_file.path;
	    b = s->head_to_index->new_file.path;
	}
	if (s->index_to_workdir) {
	    if (!a)
		a = s->index_to_workdir->old_file.path;
	    if (!b)
		b = s->index_to_workdir->old_file.path;
	    c = s->index_to_workdir->new_file.path;
	}

	if (istatus == 'R') {
	    if (wstatus == 'R')
		printf("%c%c %s %s %s%s\n", istatus, wstatus, a, b, c, extra);
	    else
		printf("%c%c %s %s%s\n", istatus, wstatus, a, b, extra);
	} else {
	    if (wstatus == 'R')
		printf("%c%c %s %s%s\n", istatus, wstatus, a, c, extra);
	    else
		printf("%c%c %s%s\n", istatus, wstatus, a, extra);
	}
    }

    for (i = 0; i < maxi; ++i) {
	s = git_status_byindex(status, i);

	if (s->status == GIT_STATUS_WT_NEW)
	    printf("?? %s\n", s->index_to_workdir->old_file.path);
    }
}

static int print_submod(git_submodule *sm, const char *name, void *payload)
{
    int *count = payload;
    (void)name;

    if (*count == 0)
	printf("# Submodules\n");
    (*count)++;

    printf("# - submodule '%s' at %s\n",
	git_submodule_name(sm), git_submodule_path(sm));

    return 0;
}

#endif	/* defined(WITH_LIBGIT2) */

rpmRC rpmgitCmdStatus(int argc, char *argv[])
{
    int rc = RPMRC_FAIL;
#if defined(WITH_LIBGIT2)
    git_status_options opt = { GIT_STATUS_OPTIONS_VERSION, 0, 0, { NULL, 0} };
    const char * status_untracked_files = xstrdup("all");
    const char * status_ignore_submodules = xstrdup("all");
    enum {
	_STATUS_BRANCH		= (1 <<  0),
	_STATUS_ZERO		= (1 <<  1),
	_STATUS_IGNORED		= (1 <<  2),
	_STATUS_SHOWSUBMOD	= (1 <<  3),
    };
    int status_flags = 0;
#define	STATUS_ISSET(_a)	(status_flags & _STATUS_##_a)
    int format = FORMAT_DEFAULT;	/* XXX git->format? */
    int repeat = 0;
    struct poptOption statusOpts[] = {
     { "short", 's', POPT_ARG_VAL,		&format, FORMAT_SHORT,
	N_("Give the output in the short-format."), NULL },
     { "long", '\0', POPT_ARG_VAL,		&format, FORMAT_LONG,
	N_("Give the output in the long-format."), NULL },
     { "porcelain", '\0', POPT_ARG_VAL,		&format, FORMAT_PORCELAIN,
	N_("Give the output in a stable, easy-to-parse format for scripts."), NULL },
     { "branch", 'b', POPT_BIT_SET,		&status_flags, _STATUS_BRANCH,
	N_("."), NULL },
     { NULL, 'z', POPT_BIT_SET,	&status_flags, _STATUS_ZERO,
	N_("."), NULL },
     { "ignored", '\0', POPT_BIT_SET,		&status_flags, _STATUS_IGNORED,
	N_("."), NULL },
	/*XXX -uno */
	/*XXX -unormal */
	/*XXX -uall */
     { "untracked-files", 'u', POPT_ARG_STRING|POPT_ARGFLAG_SHOW_DEFAULT,	&status_untracked_files, 0,
	N_("Show untracked files."), N_("{no|normal|all}") },
     { "ignore-submodules", '\0', POPT_ARG_STRING|POPT_ARGFLAG_SHOW_DEFAULT,	&status_ignore_submodules, 0,
	N_("Ignore sub-modules."), N_("{all}") },
     { "repeat", '\0', POPT_ARG_INT,		&repeat, 0,
	N_("Repeat every <sec> seconds."), N_("<sec>") },
     { "list-submodules", '\0', POPT_BIT_SET,	&status_flags, _STATUS_SHOWSUBMOD,
	N_("."), NULL },
    /* --git-dir */
      POPT_AUTOALIAS
      POPT_AUTOHELP
      POPT_TABLEEND
    };
    rpmgit git = rpmgitNew(argv, 0, statusOpts);
    git_status_list * list = NULL;
    int xx = -1;

    opt.show   =  GIT_STATUS_SHOW_INDEX_AND_WORKDIR;

    opt.flags |=  GIT_STATUS_OPT_INCLUDE_UNTRACKED;
    opt.flags |=  GIT_STATUS_OPT_RENAMES_HEAD_TO_INDEX;
    opt.flags |=  GIT_STATUS_OPT_SORT_CASE_SENSITIVELY;

    if (STATUS_ISSET(IGNORED)) {
	opt.flags |=  GIT_STATUS_OPT_INCLUDE_IGNORED;
    }
    if (!strcmp(status_untracked_files, "no")) {
	opt.flags &= ~GIT_STATUS_OPT_INCLUDE_UNTRACKED;
	opt.flags &= ~GIT_STATUS_OPT_RECURSE_UNTRACKED_DIRS;
    } else
    if (!strcmp(status_untracked_files, "normal")) {
	opt.flags |=  GIT_STATUS_OPT_INCLUDE_UNTRACKED;
	opt.flags &= ~GIT_STATUS_OPT_RECURSE_UNTRACKED_DIRS;
    } else
    if (!strcmp(status_untracked_files, "all")) {
	opt.flags |=  GIT_STATUS_OPT_INCLUDE_UNTRACKED;
	opt.flags |=  GIT_STATUS_OPT_RECURSE_UNTRACKED_DIRS;
    }
    if (!strcmp(status_ignore_submodules, "all")) {
	opt.flags |=  GIT_STATUS_OPT_EXCLUDE_SUBMODULES;
    }

    if (format == FORMAT_DEFAULT)
	format = STATUS_ISSET(ZERO) ? FORMAT_PORCELAIN : FORMAT_LONG;
    if (format == FORMAT_LONG)
	status_flags |= _STATUS_BRANCH;
    if (git->ac > 0) {
	opt.pathspec.strings = (char **) git->av;
	opt.pathspec.count   = git->ac;
    }

show_status:
    if (repeat)
	printf("\033[H\033[2J");

    /*
     * Run status on the repository
     *
     * We use `git_status_list_new()` to generate a list of status
     * information which lets us iterate over it at our
     * convenience and extract the data we want to show out of
     * each entry.
     *
     * You can use `git_status_foreach()` or
     * `git_status_foreach_ext()` if you'd prefer to execute a
     * callback for each entry. The latter gives you more control
     * about what results are presented.
     */

    git->state = 0;
    xx = chkgit(git, "git_status_list_new",
		git_status_list_new(&list, git->R, &opt));

    if (STATUS_ISSET(BRANCH))
	show_branch(git->R, format);

    if (STATUS_ISSET(SHOWSUBMOD)) {
	int submod_count = 0;
	xx = chkgit(git, "git_submodule_foreach",
		git_submodule_foreach(git->R, print_submod, &submod_count));
    }

    if (format == FORMAT_LONG)
	print_long(git->R, list);
    else
	print_short(git->R, list);

    git_status_list_free(list);
    list = NULL;
    if (repeat) {
	sleep(repeat);
	goto show_status;
    }

    goto exit;	/* XXX GCC warning */

exit:
    rc = (xx ? RPMRC_FAIL : RPMRC_OK);
SPEW(0, rc, git);

    if (list) {
	git_status_list_free(list);
	list = NULL;
    }
    status_untracked_files = _free(status_untracked_files);
    status_ignore_submodules = _free(status_ignore_submodules);

    git = rpmgitFree(git);
#endif	/* defined(WITH_LIBGIT2) */
    return rc;
}
#undef	STATUS_ISSET

/*==============================================================*/

#define ARGMINMAX(_min, _max)   (int)(((_min) << 8) | ((_max) & 0xff))

static struct poptOption rpmgitCmds[] = {
/* --- PORCELAIN */
 { "add", '\0',      POPT_ARG_MAINCALL,	rpmgitCmdAdd, ARGMINMAX(1,0),
	N_("Add file contents to the index."), NULL },
#ifdef	NOTYET
 { "bisect", '\0',   POPT_ARG_MAINCALL,	cmd_noop, ARGMINMAX(0,0),
	N_("Find by binary search the change that introduced a bug."), NULL },
 { "branch", '\0',   POPT_ARG_MAINCALL,	cmd_branch, ARGMINMAX(0,0),
	N_("List, create, or delete branches."), NULL },
 { "checkout", '\0', POPT_ARG_MAINCALL,	cmd_noop, ARGMINMAX(0,0),
	N_("Checkout a branch or paths to the working tree."), NULL },
 { "clone", '\0',    POPT_ARG_MAINCALL,	cmd_clone, ARGMINMAX(0,0),
	N_("Clone a repository into a new directory."), NULL },
#endif	/* NOTYET */
 { "commit", '\0',   POPT_ARG_MAINCALL,	rpmgitCmdCommit, ARGMINMAX(0,0),
	N_("Record changes to the repository."), NULL },
 { "diff", '\0',     POPT_ARG_MAINCALL,	rpmgitCmdDiff, ARGMINMAX(0,0),
	N_("Show changes between commits, commit and working tree, etc."), NULL },
#ifdef	NOTYET
 { "fetch", '\0',    POPT_ARG_MAINCALL,	cmd_fetch, ARGMINMAX(0,0),
	N_("Download objects and refs from another repository."), NULL },
 { "grep", '\0',     POPT_ARG_MAINCALL,	cmd_noop, ARGMINMAX(0,0),
	N_("Print lines matching a pattern."), NULL },
#endif	/* NOTYET */
	/* XXX maxargs=1 */
 { "init", '\0',     POPT_ARG_MAINCALL,	rpmgitCmdInit, ARGMINMAX(1,0),
	N_("Create an empty git repository or reinitialize an existing one."), NULL },
#ifdef	NOTYET
 { "log", '\0',      POPT_ARG_MAINCALL,	cmd_log, ARGMINMAX(0,0),
	N_("Show commit logs."), NULL },
 { "merge", '\0',    POPT_ARG_MAINCALL,	cmd_noop, ARGMINMAX(0,0),
	N_("Join two or more development histories together."), NULL },
 { "mv", '\0',       POPT_ARG_MAINCALL,	cmd_noop, ARGMINMAX(0,0),
	N_("Move or rename a file, a directory, or a symlink."), NULL },
 { "pull", '\0',     POPT_ARG_MAINCALL,	cmd_noop, ARGMINMAX(0,0),
	N_("Fetch from and merge with another repository or a local branch."), NULL },
 { "push", '\0',     POPT_ARG_MAINCALL,	cmd_noop, ARGMINMAX(0,0),
	N_("Update remote refs along with associated objects."), NULL },
 { "rebase", '\0',   POPT_ARG_MAINCALL,	cmd_noop, ARGMINMAX(0,0),
	N_("Forward-port local commits to the updated upstream head."), NULL },
 { "reset", '\0',    POPT_ARG_MAINCALL,	cmd_reset, ARGMINMAX(0,0),
	N_("Reset current HEAD to the specified state."), NULL },
 { "rm", '\0',       POPT_ARG_MAINCALL,	cmd_noop, ARGMINMAX(0,0),
	N_("Remove files from the working tree and from the index."), NULL },
 { "show", '\0',     POPT_ARG_MAINCALL,	cmd_noop, ARGMINMAX(0,0),
	N_("Show various types of objects."), NULL },
#endif	/* NOTYET */
 { "status", '\0',   POPT_ARG_MAINCALL,	rpmgitCmdStatus, ARGMINMAX(0,0),
	N_("Show the working tree status."), NULL },
#ifdef	NOTYET
 { "tag", '\0',      POPT_ARG_MAINCALL,	cmd_tag, ARGMINMAX(0,0),
	N_("Create, list, delete or verify a tag object signed with GPG."), NULL },
#endif	/* NOTYET */

/* --- PLUMBING: manipulation */
#ifdef	NOTYET
 { "apply", '\0', POPT_ARG_MAINCALL,	cmd_noop, ARGMINMAX(0,0),
	N_("Apply a patch to files and/or to the index."), NULL },
 { "checkout-index", '\0', POPT_ARG_MAINCALL,	cmd_noop, ARGMINMAX(0,0),
	N_("Copy files from the index to the working tree."), NULL },
 { "commit-tree", '\0', POPT_ARG_MAINCALL,	cmd_noop, ARGMINMAX(0,0),
	N_("Create a new commit object."), NULL },
 { "hash-object", '\0', POPT_ARG_MAINCALL,	cmd_hash_object, ARGMINMAX(0,0),
	N_("Compute object ID and optionally creates a blob from a file."), NULL },
 { "index-pack", '\0', POPT_ARG_MAINCALL,	cmd_index_pack, ARGMINMAX(0,0),
	N_("Build pack index file for an existing packed archive."), NULL },
 { "merge-file", '\0', POPT_ARG_MAINCALL,	cmd_noop, ARGMINMAX(0,0),
	N_("Run a three-way file merge."), NULL },
 { "merge-index", '\0', POPT_ARG_MAINCALL,	cmd_noop, ARGMINMAX(0,0),
	N_("Run a merge for files needing merging."), NULL },
 { "mktag", '\0', POPT_ARG_MAINCALL,	cmd_noop, ARGMINMAX(0,0),
	N_("Creates a tag object."), NULL },
 { "mktree", '\0', POPT_ARG_MAINCALL,	cmd_noop, ARGMINMAX(0,0),
	N_("Build a tree-object from ls-tree formatted text."), NULL },
 { "pack-objects", '\0', POPT_ARG_MAINCALL,	cmd_noop, ARGMINMAX(0,0),
	N_("Create a packed archive of objects."), NULL },
 { "prune-packed", '\0', POPT_ARG_MAINCALL,	cmd_noop, ARGMINMAX(0,0),
	N_("Remove extra objects that are already in pack files."), NULL },
 { "read-tree", '\0', POPT_ARG_MAINCALL,	cmd_noop, ARGMINMAX(0,0),
	N_("iReads tree information into the index."), NULL },
 { "symbolic-ref", '\0', POPT_ARG_MAINCALL,	cmd_noop, ARGMINMAX(0,0),
	N_("iRead and modify symbolic refs."), NULL },
 { "unpack-objects", '\0', POPT_ARG_MAINCALL,	cmd_noop, ARGMINMAX(0,0),
	N_("Unpack objects from a packed archive."), NULL },
 { "update-index", '\0', POPT_ARG_MAINCALL,	cmd_update_index, ARGMINMAX(0,0),
	N_("Register file contents in the working tree to the index."), NULL },
 { "update-ref", '\0', POPT_ARG_MAINCALL,	cmd_noop, ARGMINMAX(0,0),
	N_("Update the object name stored in a ref safely."), NULL },
 { "write-tree", '\0', POPT_ARG_MAINCALL,	cmd_noop, ARGMINMAX(0,0),
	N_("Create a tree object from the current index."), NULL },
#endif	/* NOTYET */

/* --- PLUMBING: interrogation */
#ifdef	NOTYET
 { "archive", '\0', POPT_ARG_MAINCALL,	cmd_noop, ARGMINMAX(0,0),
	N_("Create a tar/zip archive of the files in the named tree object."), NULL },
 { "cat-file", '\0', POPT_ARG_MAINCALL,	cmd_cat_file, ARGMINMAX(0,0),
	N_("Provide content or type and size information for repository objects."), NULL },
 { "diff-files", '\0', POPT_ARG_MAINCALL,	cmd_noop, ARGMINMAX(0,0),
	N_("Compares files in the working tree and the index."), NULL },
 { "diff-index", '\0', POPT_ARG_MAINCALL,	cmd_noop, ARGMINMAX(0,0),
	N_("Compares content and mode of blobs between the index and repository."), NULL },
 { "diff-tree", '\0', POPT_ARG_MAINCALL,	cmd_noop, ARGMINMAX(0,0),
	N_("Compares the content and mode of blobs found via two tree objects."), NULL },
 { "for-each-ref", '\0', POPT_ARG_MAINCALL,	cmd_noop, ARGMINMAX(0,0),
	N_("Output information on each ref."), NULL },
 { "ls-files", '\0', POPT_ARG_MAINCALL,	cmd_noop, ARGMINMAX(0,0),
	N_("Show information about files in the index and the working tree."), NULL },
 { "ls-remote", '\0', POPT_ARG_MAINCALL,	cmd_ls_remote, ARGMINMAX(0,0),
	N_("List references in a remote repository."), NULL },
 { "ls-tree", '\0', POPT_ARG_MAINCALL,	cmd_noop, ARGMINMAX(0,0),
	N_("List the contents of a tree object."), NULL },
 { "merge-base", '\0', POPT_ARG_MAINCALL,	cmd_noop, ARGMINMAX(0,0),
	N_("Find as good common ancestors as possible for a merge."), NULL },
 { "name-rev", '\0', POPT_ARG_MAINCALL,	cmd_noop, ARGMINMAX(0,0),
	N_("Find symbolic names for given revs."), NULL },
 { "pack-redundant", '\0', POPT_ARG_MAINCALL,	cmd_noop, ARGMINMAX(0,0),
	N_("Find redundant pack files."), NULL },
 { "rev-list", '\0', POPT_ARG_MAINCALL,	cmd_noop, ARGMINMAX(0,0),
	N_("Lists commit objects in reverse chronological order."), NULL },
 { "show-index", '\0', POPT_ARG_MAINCALL,	cmd_noop, ARGMINMAX(0,0),
	N_("Show packed archive index."), NULL },
 { "show-ref", '\0', POPT_ARG_MAINCALL,	cmd_noop, ARGMINMAX(0,0),
	N_("List references in a local repository."), NULL },
 { "unpack-file", '\0', POPT_ARG_MAINCALL,	cmd_noop, ARGMINMAX(0,0),
	N_("Creates a temporary file with blob contents."), NULL },
 { "var", '\0', POPT_ARG_MAINCALL,	cmd_noop, ARGMINMAX(0,0),
	N_("Show a git logical variable."), NULL },
 { "verify-pack", '\0', POPT_ARG_MAINCALL,	cmd_noop, ARGMINMAX(0,0),
	N_("Validate packed git archive files."), NULL },
#endif	/* NOTYET */

/* --- WIP */
#ifdef	NOTYET
 { "config", '\0', POPT_ARG_MAINCALL,	cmd_config, ARGMINMAX(0,0),
	N_("Show git configuration."), NULL },
 { "index", '\0', POPT_ARG_MAINCALL,	cmd_index, ARGMINMAX(0,0),
	N_("Show git index."), NULL },
 { "refs", '\0', POPT_ARG_MAINCALL,	cmd_refs, ARGMINMAX(0,0),
	N_("Show git references."), NULL },
 { "rev-parse", '\0', POPT_ARG_MAINCALL,cmd_rev_parse, ARGMINMAX(0,0),
	N_("."), NULL },

 { "index-pack-old", '\0', POPT_ARG_MAINCALL,	cmd_index_pack_old, ARGMINMAX(0,0),
	N_("Index a <PACKFILE>."), N_("<PACKFILE>") },
#endif	/* NOTYET */

  POPT_TABLEEND
};

#ifdef	NOTYET
static rpmRC rpmgitCmdHelp(int argc, /*@unused@*/ char *argv[])
{
    FILE * fp = stdout;
    struct poptOption * c;

    fprintf(fp, "Commands:\n\n");
    for (c = _rpmgitCommandTable; c->longName != NULL; c++) {
        fprintf(fp, "    %s %s\n        %s\n\n",
		c->longName, (c->argDescrip ? c->argDescrip : ""), c->descrip);
    }
    return RPMRC_OK;
}

static rpmRC rpmgitCmdRun(int argc, /*@unused@*/ char *argv[])
{
    struct poptOption * c;
    const char * cmd;
    rpmRC rc = RPMRC_FAIL;

    if (argv == NULL || argv[0] == NULL)	/* XXX segfault avoidance */
	goto exit;
    cmd = argv[0];
    for (c = _rpmgitCommandTable; c->longName != NULL; c++) {
	rpmRC (*func) (int argc, char *argv[]) = NULL;

	if (strcmp(cmd, c->longName))
	    continue;

	func = c->arg;
	rc = (*func) (argc, argv);
	break;
    }

exit:
    return rc;
}
#endif	/* NOTYET */

/*==============================================================*/

/* XXX move to struct rpmgit_s? */
static const char * exec_path;
static const char * html_path;
static const char * man_path;
static const char * info_path;
static int paginate;
static int no_replace_objects;
static int bare;

static struct poptOption rpmgitOpts[] = {
	/* XXX --version */
  { "exec-path", '\0', POPT_ARG_STRING,		&exec_path, 0,
        N_("Set exec path to <DIR>. env(GIT_EXEC_PATH)"), N_("<DIR>") },
  { "html-path", '\0', POPT_ARG_STRING,		&html_path, 0,
        N_("Set html path to <DIR>. env(GIT_HTML_PATH)"), N_("<DIR>") },
  { "man-path", '\0', POPT_ARG_STRING,		&man_path, 0,
        N_("Set man path to <DIR>."), N_("<DIR>") },
  { "info-path", '\0', POPT_ARG_STRING,		&info_path, 0,
        N_("Set info path to <DIR>."), N_("<DIR>") },
	/* XXX --pager */
 { "paginate", 'p', POPT_ARG_VAL,		&paginate, 1,
	N_("Set paginate. env(PAGER)"), NULL },
 { "no-paginate", '\0', POPT_ARG_VAL,		&paginate, 0,
	N_("Set paginate. env(PAGER)"), NULL },
 { "no-replace-objects", '\0', POPT_ARG_VAL,	&no_replace_objects, 1,
	N_("Do not use replacement refs for objects."), NULL },
 { "bare", '\0', POPT_ARG_VAL,	&bare, 1,
	N_("Treat as a bare repository."), NULL },

  { "git-dir", '\0', POPT_ARG_STRING,	&_rpmgit_dir, 0,
        N_("Set git repository dir to <DIR>. env(GIT_DIR)"), N_("<DIR>") },
  { "work-tree", '\0', POPT_ARG_STRING,	&_rpmgit_tree, 0,
        N_("Set git work tree to <DIR>. env(GIT_WORK_TREE)"), N_("<DIR>") },

 { NULL, '\0', POPT_ARG_INCLUDE_TABLE, rpmgitCmds, 0,
	N_("The most commonly used git commands are::"),
	NULL },

#ifdef	NOTYET
 { NULL, '\0', POPT_ARG_INCLUDE_TABLE, rpmioAllPoptTable, 0,
	N_("Common options for all rpmio executables:"),
	NULL },

  POPT_AUTOALIAS
#endif
  POPT_AUTOHELP

  { NULL, (char) -1, POPT_ARG_INCLUDE_TABLE, NULL, 0,
        N_("\
usage: git [--version] [--exec-path[=GIT_EXEC_PATH]] [--html-path]\n\
           [-p|--paginate|--no-pager] [--no-replace-objects]\n\
           [--bare] [--git-dir=GIT_DIR] [--work-tree=GIT_WORK_TREE]\n\
           [--help] COMMAND [ARGS]\n\
\n\
The most commonly used git commands are:\n\
   add        Add file contents to the index\n\
   bisect     Find by binary search the change that introduced a bug\n\
   branch     List, create, or delete branches\n\
   checkout   Checkout a branch or paths to the working tree\n\
   clone      Clone a repository into a new directory\n\
   commit     Record changes to the repository\n\
   diff       Show changes between commits, commit and working tree, etc\n\
   fetch      Download objects and refs from another repository\n\
   grep       Print lines matching a pattern\n\
   init       Create an empty git repository or reinitialize an existing one\n\
   log        Show commit logs\n\
   merge      Join two or more development histories together\n\
   mv         Move or rename a file, a directory, or a symlink\n\
   pull       Fetch from and merge with another repository or a local branch\n\
   push       Update remote refs along with associated objects\n\
   rebase     Forward-port local commits to the updated upstream head\n\
   reset      Reset current HEAD to the specified state\n\
   rm         Remove files from the working tree and from the index\n\
   show       Show various types of objects\n\
   status     Show the working tree status\n\
   tag        Create, list, delete or verify a tag object signed with GPG\n\
\n\
See 'git COMMAND --help' for more information on a specific command.\n\
"), NULL},

  POPT_TABLEEND
};

/*==============================================================*/

static void rpmgitFini(void * _git)
	/*@globals fileSystem @*/
	/*@modifies *_git, fileSystem @*/
{
    rpmgit git = (rpmgit) _git;

#if defined(WITH_LIBGIT2)
    if (git->walk)
	git_revwalk_free(git->walk);
    if (git->odb)
	git_odb_free(git->odb);
    if (git->cfg)
	git_config_free(git->cfg);
    if (git->S)
	git_signature_free(git->S);
    if (git->H)
	git_commit_free(git->H);
    if (git->C)
	git_commit_free(git->C);
    if (git->T)
	git_tree_free(git->T);
    if (git->I)
	git_index_free(git->I);
    if (git->R)
	git_repository_free(git->R);

    /* XXX elsewhere: assumes rpmgitNew()/rpmgitFree() calls pair */
    if (--_rpmgit_threads <= 0) {
	git_threads_shutdown();
	_rpmgit_threads = 0;
    }
#endif
    git->walk = NULL;
    git->odb = NULL;
    git->cfg = NULL;
    git->S = NULL;
    git->H = NULL;
    git->C = NULL;
    git->T = NULL;
    git->I = NULL;
    git->R = NULL;

    git->fp = NULL;
    git->data = NULL;

    git->hide = 0;
    git->sorting = 0;
    git->state = 0;
    git->shared_umask = 0;

    git->rev = 0;
    git->minor = 0;
    git->major = 0;

    git->user_email = _free(git->user_email);
    git->user_name = _free(git->user_name);

    git->core_repositoryformatversion = 0;
    git->core_bare = 0;
    git->is_bare = 0;

    git->workdir = _free(git->workdir);
    git->repodir = _free(git->repodir);

    git->ac = 0;
    git->av = argvFree(git->av);
    git->con = poptFreeContext(git->con);

    git->flags = 0;
    git->fn = _free(git->fn);

}

/*@unchecked@*/ /*@only@*/ /*@null@*/
rpmioPool _rpmgitPool;

/*@unchecked@*/ /*@relnull@*/
rpmgit _rpmgitI;

static rpmgit rpmgitGetPool(/*@null@*/ rpmioPool pool)
	/*@globals _rpmgitPool, fileSystem @*/
	/*@modifies pool, _rpmgitPool, fileSystem @*/
{
    rpmgit git;

    if (_rpmgitPool == NULL) {
	_rpmgitPool = rpmioNewPool("git", sizeof(*git), -1, _rpmgit_debug,
			NULL, NULL, rpmgitFini);
	pool = _rpmgitPool;
    }
    git = (rpmgit) rpmioGetPool(pool, sizeof(*git));
    memset(((char *)git)+sizeof(git->_item), 0, sizeof(*git)-sizeof(git->_item));
    return git;
}

#ifdef	NOTYET	/* XXX rpmgitRun() uses pre-parsed git->av */
/*@unchecked@*/
static const char * _gitI_init = "\
";
#endif	/* NOTYET */

static rpmgit rpmgitI(void)
        /*@globals _rpmgitI @*/
        /*@modifies _rpmgitI @*/
{
    if (_rpmgitI == NULL)
        _rpmgitI = rpmgitNew(NULL, 0, NULL);
    return _rpmgitI;
}

rpmgit rpmgitNew(char ** av, uint32_t flags, void * _opts)
{
    static char * _av[] = { (char *) "rpmgit", NULL };
    int initialize = (!(flags & 0x80000000) || _rpmgitI == NULL);
    rpmgit git = (flags & 0x80000000)
        ? rpmgitI() : rpmgitGetPool(_rpmgitPool);
    int ac;
poptOption opts = (poptOption) _opts;
const char * fn = _rpmgit_dir;
int xx;

if (_rpmgit_debug)
fprintf(stderr, "==> %s(%p, 0x%x) git %p fn %s\n", __FUNCTION__, av, flags, git, fn);

    if (av == NULL) av = _av;
    if (opts == NULL) opts = rpmgitOpts;

    ac = argvCount((ARGV_t)av);
    if (ac > 1)
	xx = rpmgitPopt(git, ac, av, opts);

    git->fn = (fn ? xstrdup(fn) : NULL);
    git->flags = flags;

#if defined(WITH_LIBGIT2)

    /* XXX elsewhere: assumes rpmgitNew() calls pair */
    if (_rpmgit_threads <= 0) {
	git_threads_init();
	_rpmgit_threads = 1;
    }

    if (initialize) {
	struct stat sb;
	int xx;
	git_libgit2_version(&git->major, &git->minor, &git->rev);
#ifdef	DYING
	if (git->fn && git->R == NULL) {
	    git->repodir = xstrdup(git->fn);
	    xx = chkgit(git, "git_repository_open",
		git_repository_open((git_repository **)&git->R, git->repodir));
	}
#else
	if (git->fn && Stat(git->fn, &sb) == 0)
	    xx = rpmgitOpen(git, git->fn);
#if 0
assert(xx == 0 && git->R != NULL && git->repodir != NULL);
#endif
#endif
    }

#ifdef	NOTYET	/* XXX rpmgitRun() uses pre-parsed git->av */
    if (initialize) {
        static const char _rpmgitI_init[] = "%{?_rpmgitI_init}";
        const char * s = rpmExpand(_rpmgitI_init, _gitI_init, NULL);
	ARGV_t sav = NULL;
	const char * arg;
	int i;

	xx = argvSplit(&sav, s, "\n");
	for (i = 0; (arg = sav[i]) != NULL; i++) {
	    if (*arg == '\0')
		continue;
	    (void) rpmgitRun(git, arg, NULL);
	}
	sav = argvFree(sav);
        s = _free(s);
    }
#endif	/* NOTYET */
#endif	/* defined(WITH_LIBGIT2) */

    return rpmgitLink(git);
}

/* XXX str argument isn't used. */
rpmRC rpmgitRun(rpmgit git, const char * str, const char ** resultp)
{
    rpmRC rc = RPMRC_FAIL;
    const char * cmd;
    struct poptOption * c;
    rpmRC (*handler) (int argc, char *argv[]);
    int minargs;
    int maxargs;

if (_rpmgit_debug)
fprintf(stderr, "==> %s(%p,%s,%p)\n", __FUNCTION__, git, str, resultp);

    if (git == NULL) git = rpmgitI();


    if (git->av == NULL || git->av[0] == NULL)	/* XXX segfault avoidance */
	goto exit;
    cmd = git->av[0];
    for (c = rpmgitCmds; c->longName != NULL; c++) {
	if (strcmp(cmd, c->longName))
	    continue;
	handler = c->arg;
	minargs = (c->val >> 8) & 0xff;
	maxargs = (c->val     ) & 0xff;
	break;
    }
    if (c->longName == NULL) {
	fprintf(stderr, "Unknown command '%s'\n", cmd);
	rc = RPMRC_FAIL;
    } else
    if (minargs && git->ac < minargs) {
	fprintf(stderr, "Not enough arguments for \"git %s\"\n", c->longName);
	rc = RPMRC_FAIL;
    } else
    if (maxargs && git->ac > maxargs) {
	fprintf(stderr, "Too many arguments for \"git %s\"\n", c->longName);
	rc = RPMRC_FAIL;
    } else {
	ARGV_t av = git->av;
	int ac = git->ac;

	git->av = NULL;
	git->ac = 0;
        rc = (*handler)(git->ac, (char **)git->av);
	git->av = av;
	git->ac = ac;
    }

exit:
    return rc;
}
