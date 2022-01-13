/** \ingroup rpmds
 * \file lib/rpmds.c
 */
#include "system.h"

#if defined(WITH_CPUINFO)
#include <cpuinfo.h>
#endif

#if defined(HAVE_GELF_H) && !defined(__FreeBSD__)
#if LIBELF_H_LFS_CONFLICT
/* Some implementations of libelf.h/gelf.h are incompatible with
 * the Large File API.
 */
# undef _LARGEFILE64_SOURCE
# undef _LARGEFILE_SOURCE
# undef _FILE_OFFSET_BITS
# define _FILE_OFFSET_BITS 32
#endif

#if defined(__LCLINT__)
typedef	long long loff_t;
#endif
#include <gelf.h>
/*
 * On Solaris, gelf.h included libelf.h, which #undef'ed the gettext
 * convenience macro _().  Repair by repeating (from system.h) just
 * the bits that are needed for _() to function.
 */

#if defined(__sun)
#if defined(ENABLE_NLS) && !defined(__LCLINT__)
# define _(Text) gettext (Text)
#else
# define _(Text) Text
#endif /* gettext _() fixup */
#endif
#endif /* HAVE_GELF_H */

#if defined(HAVE_LIBELF) && !defined(HAVE_GELF_GETVERNAUX) && !defined(__FreeBSD__)
/* We have gelf.h and libelf, but we don't have some of the
 * helper functions gelf_getvernaux(), gelf_getverneed(), etc.
 * Provide our own simple versions here.
 */

static GElf_Verdef *gelf_getverdef(Elf_Data *data, int offset,
                   GElf_Verdef *dst)
{
	return (GElf_Verdef *) ((char *) data->d_buf + offset);
}

static GElf_Verdaux *gelf_getverdaux(Elf_Data *data, int offset,
                    GElf_Verdaux *dst)
{
	return (GElf_Verdaux *) ((char *) data->d_buf + offset);
}

static GElf_Verneed *gelf_getverneed(Elf_Data *data, int offset,
                    GElf_Verneed *dst)
{
	return (GElf_Verneed *) ((char *) data->d_buf + offset);
}

static GElf_Vernaux *gelf_getvernaux(Elf_Data *data, int offset,
                    GElf_Vernaux *dst)
{
	return (GElf_Vernaux *) ((char *) data->d_buf + offset);
}

/* Most non-Linux systems won't have SHT_GNU_verdef or SHT_GNU_verneed,
 * but they might have something mostly-equivalent.  Solaris has
 * SHT_SUNW_{verdef,verneed}
 */
#if !defined(SHT_GNU_verdef) && defined(__sun) && defined(SHT_SUNW_verdef)
# define SHT_GNU_verdef SHT_SUNW_verdef
# define SHT_GNU_verneed SHT_SUNW_verneed
#endif

#endif /* HAVE_LIBELF && !HAVE_GELF_GETVERNAUX */

#if !defined(DT_GNU_HASH)
#define	DT_GNU_HASH	0x6ffffef5
#endif

#define	_RPMIOB_INTERNAL
#include <rpmiotypes.h>
#include <rpmio_internal.h>	/* XXX fdGetFILE */
#include <rpmcb.h>		/* XXX fnpyKey */
#include <rpmmacro.h>
#include <argv.h>
#include <set.h>

#include <rpmtypes.h>
#include <rpmtag.h>

#define	_RPMDS_INTERNAL
#define	_RPMEVR_INTERNAL
#define	_RPMPRCO_INTERNAL
#include <rpmds.h>

#include "debug.h"

/*@access rpmns @*/
/*@access EVR_t @*/

#ifdef __cplusplus
GENfree(evrFlags *)
GENfree(rpmuint32_t *)
GENfree(rpmint32_t *)
GENfree(rpmPRCO)
#endif	/* __cplusplus */

#define	_isspace(_c)	\
	((_c) == ' ' || (_c) == '\t' || (_c) == '\r' || (_c) == '\n')

/**
 * Enable noisy range comparison debugging message?
 */
/*@unchecked@*/
static int _noisy_range_comparison_debug_message = 0;

/*@unchecked@*/
int _rpmds_debug = 0;

/*@unchecked@*/
int _rpmds_nopromote = 1;

/*@unchecked@*/
/*@-exportheadervar@*/
int _rpmds_unspecified_epoch_noise = 0;
/*@=exportheadervar@*/

/**
 * Return dependency set type string.
 * @param tagN		dependency set tag
 * @return		dependency set type string
 */
/*@observer@*/
static const char * rpmdsTagName(rpmTag tagN)
	/*@*/
{
    const char * Type;

    /* XXX Preserve existing names in debugging messages. */
    switch (tagN) {
    default:			Type = tagName(tagN);	break;
    case RPMTAG_PROVIDENAME:	Type = "Provides";	break;
    case RPMTAG_REQUIRENAME:	Type = "Requires";	break;
    case RPMTAG_CONFLICTNAME:	Type = "Conflicts";	break;
    case RPMTAG_OBSOLETENAME:	Type = "Obsoletes";	break;
    case RPMTAG_TRIGGERNAME:	Type = "Triggers";	break;
    case RPMTAG_SUGGESTSNAME:	Type = "Suggests";	break;
    case RPMTAG_ENHANCESNAME:	Type = "Enhances";	break;
    case RPMTAG_DIRNAMES:	Type = "Dirs";		break;
    case RPMTAG_BASENAMES:	Type = "Files";		break;
    case RPMTAG_FILELINKTOS:	Type = "Linktos";	break;
    case 0:			Type = "Unknown";	break;
    }
    return Type;
}

const char * rpmdsType(const rpmds ds)
{
    return rpmdsTagName(rpmdsTagN(ds));
}

static void rpmdsFini(void * _ds)
{
    rpmds ds = (rpmds) _ds;

    if (ds->Count > 0) {
	ds->N = _free(ds->N);
	ds->EVR = _free(ds->EVR);
	ds->Flags = _free(ds->Flags);
	(void)headerFree(ds->h);
	ds->h = NULL;
    }

    ds->DNEVR = _free(ds->DNEVR);
    ds->ns.str = _free(ds->ns.str);
    memset(&ds->ns, 0, sizeof(ds->ns));
    ds->A = _free(ds->A);
    ds->Color = _free(ds->Color);
    ds->Refs = _free(ds->Refs);
    ds->Result = _free(ds->Result);
    ds->exclude = (miRE) mireFreeAll(ds->exclude, ds->nexclude);
    ds->include = (miRE) mireFreeAll(ds->include, ds->ninclude);
}

/*@unchecked@*/ /*@only@*/ /*@null@*/
rpmioPool _rpmdsPool;

static rpmds rpmdsGetPool(/*@null@*/ rpmioPool pool)
	/*@globals _rpmdsPool, fileSystem, internalState @*/
	/*@modifies pool, _rpmdsPool, fileSystem, internalState @*/
{
    rpmds ds;

    if (_rpmdsPool == NULL) {
	_rpmdsPool = rpmioNewPool("ds", sizeof(*ds), -1, _rpmds_debug,
			NULL, NULL, rpmdsFini);
	pool = _rpmdsPool;
    }
    ds = (rpmds) rpmioGetPool(pool, sizeof(*ds));
    memset(((char *)ds)+sizeof(ds->_item), 0, sizeof(*ds)-sizeof(ds->_item));
    return ds;
}

static /*@null@*/
const char ** rpmdsDupArgv(/*@null@*/ const char ** argv, int argc)
	/*@*/
{
    const char ** av;
    size_t nb = 0;
    int ac = 0;
    char * t;

    if (argv == NULL)
	return NULL;
    for (ac = 0; ac < argc; ac++) {
assert(argv[ac] != NULL);
	nb += strlen(argv[ac]) + 1;
    }
    nb += (ac + 1) * sizeof(*av);

    av = (const char **) xmalloc(nb);
    t = (char *) (av + ac + 1);
    for (ac = 0; ac < argc; ac++) {
	av[ac] = t;
	t = stpcpy(t, argv[ac]) + 1;
    }
    av[ac] = NULL;
/*@-nullret@*/
    return av;
/*@=nullret@*/
}

rpmds rpmdsNew(Header h, rpmTag tagN, int flags)
{
    int scareMem = (flags & 0x1);
    int delslash = 1;
    HE_t he = (HE_t) memset(alloca(sizeof(*he)), 0, sizeof(*he));
    rpmTag tagEVR, tagF;
    rpmds ds = NULL;
    const char * Type = NULL;
    const char ** N;
    rpmuint32_t Count;
    int xx;

assert(scareMem == 0);		/* XXX always allocate memory */

    if (tagN == RPMTAG_NAME)
	return rpmdsThis(h, tagN, RPMSENSE_EQUAL);

    switch (tagN) {
    default:
	goto exit;
	/*@notreached@*/ break;
    case RPMTAG_PROVIDENAME:
	tagEVR = RPMTAG_PROVIDEVERSION;
	tagF = RPMTAG_PROVIDEFLAGS;
	break;
    case RPMTAG_REQUIRENAME:
	tagEVR = RPMTAG_REQUIREVERSION;
	tagF = RPMTAG_REQUIREFLAGS;
	break;
    case RPMTAG_CONFLICTNAME:
	tagEVR = RPMTAG_CONFLICTVERSION;
	tagF = RPMTAG_CONFLICTFLAGS;
	break;
    case RPMTAG_OBSOLETENAME:
	tagEVR = RPMTAG_OBSOLETEVERSION;
	tagF = RPMTAG_OBSOLETEFLAGS;
	break;
    case RPMTAG_TRIGGERNAME:
	tagEVR = RPMTAG_TRIGGERVERSION;
	tagF = RPMTAG_TRIGGERFLAGS;
	break;
    case RPMTAG_SUGGESTSNAME:
	tagEVR = RPMTAG_SUGGESTSVERSION;
	tagF = RPMTAG_SUGGESTSFLAGS;
	break;
    case RPMTAG_ENHANCESNAME:
	tagEVR = RPMTAG_ENHANCESVERSION;
	tagF = RPMTAG_ENHANCESFLAGS;
	break;
    case RPMTAG_DIRNAMES:
	tagEVR = (rpmTag) 0;
	tagF = (rpmTag) 0;
	delslash = (flags & 0x2) ? 0 : 1;
	break;
    case RPMTAG_BASENAMES:
	tagEVR = RPMTAG_DIRNAMES;
	tagF = RPMTAG_DIRINDEXES;
	break;
    case RPMTAG_FILELINKTOS:
	tagEVR = RPMTAG_DIRNAMES;
	tagF = RPMTAG_DIRINDEXES;
	break;
    }

    if (Type == NULL)
	Type = rpmdsTagName(tagN);

    he->tag = tagN;
    xx = headerGet(h, he, 0);
    N = he->p.argv;
    Count = he->c;
    if (xx && N != NULL && Count > 0) {
	ds = rpmdsGetPool(_rpmdsPool);
	ds->Type = Type;
	ds->h = NULL;
	ds->i = -1;
	ds->DNEVR = NULL;
	ds->tagN = tagN;
	ds->N = N;
	ds->Count = Count;
	ds->nopromote = _rpmds_nopromote;

	if (tagEVR > 0) {
	    he->tag = tagEVR;
	    xx = headerGet(h, he, 0);
	    ds->EVR = he->p.argv;
	}
	if (tagF > 0) {
	    he->tag = tagF;
	    xx = headerGet(h, he, 0);
	    ds->Flags = (evrFlags * ) he->p.ui32p;
	}
	{
	    he->tag = RPMTAG_ARCH;
	    xx = headerGet(h, he, 0);
	    ds->A = he->p.str;
	}
	{
	    he->tag = RPMTAG_BUILDTIME;
	    xx = headerGet(h, he, 0);
	    ds->BT = (he->p.ui32p ? he->p.ui32p[0] : 0);
	    he->p.ptr = _free(he->p.ptr);
	}

	if (tagN == RPMTAG_DIRNAMES) {
	    char * dn;
	    size_t len;
	    unsigned i;
	    /* XXX Dirnames always have trailing '/', trim that here. */
	    if (delslash)
	    for (i = 0; i < Count; i++) {
		(void) urlPath(N[i], (const char **)&dn);
		if (dn > N[i])
		    N[i] = dn;
		dn = (char *)N[i];
		len = strlen(dn);
		/* XXX don't truncate if parent is / */
		if (len > 1 && dn[len-1] == '/')
		    dn[len-1] = '\0';
	    }
	} else
	if (tagN == RPMTAG_BASENAMES) {
	    const char ** av = (const char **) xcalloc(Count+1, sizeof(*av));
	    char * dn;
	    unsigned i;

	    for (i = 0; i < Count; i++) {
		(void) urlPath(ds->EVR[ds->Flags[i]], (const char **)&dn);
		av[i] = rpmGenPath(NULL, dn, N[i]);
	    }
	    av[Count] = NULL;

/*@-unqualifiedtrans@*/
	    N = ds->N = _free(ds->N);
/*@=unqualifiedtrans@*/
	    N = ds->N = rpmdsDupArgv(av, Count);
	    av = argvFree(av);
	    ds->EVR = _free(ds->EVR);
	    ds->Flags = _free(ds->Flags);
	} else
	if (tagN == RPMTAG_FILELINKTOS) {
	    /* XXX Construct the absolute path of the target symlink(s). */
	    const char ** av = (const char **) xcalloc(Count+1, sizeof(*av));
	    unsigned i;

	    for (i = 0; i < Count; i++) {
		if (N[i] == NULL || *N[i] == '\0')
		    av[i] = xstrdup("");
		else if (*N[i] == '/')
		    av[i] = xstrdup(N[i]);
		else if (ds->EVR != NULL && ds->Flags != NULL)
		    av[i] = rpmGenPath(NULL, ds->EVR[ds->Flags[i]], N[i]);
		else
		    av[i] = xstrdup("");
	    }
	    av[Count] = NULL;

/*@-unqualifiedtrans@*/
	    N = ds->N = _free(ds->N);
/*@=unqualifiedtrans@*/
	    N = ds->N = rpmdsDupArgv(av, Count);
	    av = argvFree(av);
	    ds->EVR = _free(ds->EVR);
	    ds->Flags = _free(ds->Flags);
	}

/*@-modfilesys@*/
if (_rpmds_debug < 0)
fprintf(stderr, "*** ds %p\t%s[%d]\n", ds, ds->Type, ds->Count);
/*@=modfilesys@*/

    }

exit:
/*@-compdef -usereleased@*/	/* FIX: ds->Flags may be NULL */
    /*@-nullstate@*/ /* FIX: ds->Flags may be NULL */
    ds = rpmdsLink(ds, (ds ? ds->Type : NULL));
    /*@=nullstate@*/

    return ds;
/*@=compdef =usereleased@*/
}

const char * rpmdsNewN(rpmds ds)
{
    rpmns ns = &ds->ns;
    const char * Name = ds->N[ds->i];
    int xx;

    xx = rpmnsParse(Name, ns);

/*@-compdef -usereleased@*/ /* FIX: correct annotations for ds->ns shadow */
    return ns->N;
/*@=compdef =usereleased@*/
}

char * rpmdsNewDNEVR(const char * dspfx, rpmds ds)
{
    const char * N = rpmdsNewN(ds);
    const char * NS = ds->ns.NS;
    const char * A = ds->ns.A;
    evrFlags dsFlags = (evrFlags) 0;
    char * tbuf, * t;
    size_t nb = 0;

    if (dspfx)	nb += strlen(dspfx) + 1;
    if (ds->ns.str[0] == '!')	nb++;
    if (NS)	nb += strlen(NS) + sizeof("()") - 1;
    if (N)	nb += strlen(N);
    if (A) {
	if (_rpmns_N_at_A && _rpmns_N_at_A[0])
	    nb += sizeof(_rpmns_N_at_A[0]);
	nb += strlen(A);
    }
    /* XXX rpm prior to 3.0.2 did not always supply EVR and Flags. */
    if (ds->Flags != NULL
     && (dsFlags = (evrFlags) (ds->Flags[ds->i] & RPMSENSE_SENSEMASK)))
    {
	if (nb)	nb++;
	if (dsFlags == RPMSENSE_NOTEQUAL)
	    nb += 2;
	else {
	    if (dsFlags & RPMSENSE_LESS)	nb++;
	    if (dsFlags & RPMSENSE_GREATER)	nb++;
	    if (dsFlags & RPMSENSE_EQUAL)	nb++;
	}
    }

    ds->ns.Flags = dsFlags;

    /* XXX rpm prior to 3.0.2 did not always supply EVR and Flags. */
    if (ds->EVR != NULL && ds->EVR[ds->i] && *ds->EVR[ds->i]) {
	if (nb)	nb++;
	nb += strlen(ds->EVR[ds->i]);
    }

    t = tbuf = (char *) xmalloc(nb + 1);
    if (dspfx) {
	t = stpcpy(t, dspfx);
	*t++ = ' ';
    }
    if (ds->ns.str[0] == '!')
	*t++ = '!';
    if (NS)
	t = stpcpy( stpcpy(t, NS), "(");
    if (N)
	t = stpcpy(t, N);
    if (NS)
	t = stpcpy(t, ")");
    if (A) {
	if (_rpmns_N_at_A && _rpmns_N_at_A[0])
	    *t++ = _rpmns_N_at_A[0];
	t = stpcpy(t, A);
    }

    /* XXX rpm prior to 3.0.2 did not always supply EVR and Flags. */
    if (ds->Flags != NULL && (ds->Flags[ds->i] & RPMSENSE_SENSEMASK)) {
	if (t != tbuf)	*t++ = ' ';
	if (dsFlags == RPMSENSE_NOTEQUAL)
	    t = stpcpy(t, "!=");
	else {
	    if (dsFlags & RPMSENSE_LESS)	*t++ = '<';
	    if (dsFlags & RPMSENSE_GREATER)	*t++ = '>';
	    if (dsFlags & RPMSENSE_EQUAL)	*t++ = '=';
	}
    }
    /* XXX rpm prior to 3.0.2 did not always supply EVR and Flags. */
    if (ds->EVR != NULL && ds->EVR[ds->i] && *ds->EVR[ds->i]) {
	if (t != tbuf)	*t++ = ' ';
	t = stpcpy(t, ds->EVR[ds->i]);
    }
    *t = '\0';
    return tbuf;
}

rpmds rpmdsThis(Header h, rpmTag tagN, evrFlags Flags)
{
    HE_t he = (HE_t) memset(alloca(sizeof(*he)), 0, sizeof(*he));
    rpmds ds = NULL;
    const char * Type;
    const char * Name, * V, * R;
#ifdef	RPM_VENDOR_MANDRIVA
    const char * D = NULL;
#endif
    rpmuint32_t E;
    const char ** N, ** EVR;
    char * t;
    size_t nb;
    int xx;

    if (tagN == RPMTAG_NAME)
	tagN = RPMTAG_PROVIDENAME;

    Type = rpmdsTagName(tagN);

    he->tag = RPMTAG_EPOCH;
    xx = headerGet(h, he, 0);
    E = (he->p.ui32p ? he->p.ui32p[0] : 0);
    he->p.ptr = _free(he->p.ptr);

#if defined(RPM_VENDOR_MANDRIVA)    
    he->tag = RPMTAG_DISTEPOCH;
    xx = headerGet(h, he, 0);
    D = (he->p.str ? he->p.str : NULL);
#endif
/*@-mods@*/
    xx = headerNEVRA(h, &Name, NULL, &V, &R, NULL);
/*@=mods@*/
    /* XXX segfault avoidance */
    if (Name == NULL)	Name = xstrdup("N");
    if (V == NULL)	V = xstrdup("V");
    if (R == NULL)	R = xstrdup("R");

    t = (char *) xmalloc(sizeof(*N) + strlen(Name) + 1);
    N = (const char **) t;
    t += sizeof(*N);
    *t = '\0';
    N[0] = t;
    t = stpcpy(t, Name);
    Name = _free(Name);

    nb = sizeof(*EVR) + 20 + strlen(V) + strlen(R) + sizeof("-");
#if defined(RPM_VENDOR_MANDRIVA)
    nb += (D ? strlen(D) + sizeof(":") : 0);
#endif
    t = (char *) xmalloc(nb);
    EVR = (const char **) t;
    t += sizeof(*EVR);
    *t = '\0';
    EVR[0] = t;
    sprintf(t, "%d:", E);
    t += strlen(t);
    t = stpcpy( stpcpy( stpcpy( t, V), "-"), R);
#if defined(RPM_VENDOR_MANDRIVA)
    if (D != NULL) {
	t = stpcpy( stpcpy( t, ":"), D);
	D = _free(D);
    }
#endif
    V = _free(V);
    R = _free(R);

    ds = rpmdsGetPool(_rpmdsPool);
    ds->Type = Type;
    ds->tagN = tagN;
    ds->Count = 1;
    ds->N = N;
    ds->EVR = EVR;
    ds->Flags = (evrFlags *) xmalloc(sizeof(*ds->Flags));	ds->Flags[0] = Flags;

    he->tag = RPMTAG_ARCH;
    xx = headerGet(h, he, 0);
    ds->A = he->p.str;

    he->tag = RPMTAG_BUILDTIME;
    xx = headerGet(h, he, 0);
    ds->BT = (he->p.ui32p ? he->p.ui32p[0] : 0);
    he->p.ptr = _free(he->p.ptr);

    {	char pre[2];
	pre[0] = ds->Type[0];
	pre[1] = '\0';
	/*@-nullstate@*/ /* LCL: ds->Type may be NULL ??? */
	ds->i = 0;	/* XXX rpmdsNewN() needs ds->i = 0, not -1 */
/*@i@*/	ds->DNEVR = rpmdsNewDNEVR(pre, ds);
	/*@=nullstate@*/
    }

    return rpmdsLink(ds, (ds ? ds->Type : NULL));
}

rpmds rpmdsSingle(rpmTag tagN, const char * N, const char * EVR, evrFlags Flags)
{
    rpmds ds = rpmdsGetPool(_rpmdsPool);
    const char * Type = rpmdsTagName(tagN);

    ds->Type = Type;
    ds->tagN = tagN;
    ds->A = NULL;
    {	time_t now = time(NULL);
	ds->BT = (rpmuint32_t)now;
    }
    ds->Count = 1;
    /*@-assignexpose@*/
    ds->N = (const char **) xcalloc(2, sizeof(*ds->N));		ds->N[0] = N;
    ds->EVR = (const char **) xcalloc(2, sizeof(*ds->EVR));	ds->EVR[0] = EVR;
    /*@=assignexpose@*/
    ds->Flags = (evrFlags *) xmalloc(sizeof(*ds->Flags));	ds->Flags[0] = Flags;
    {	char t[2];
	t[0] = ds->Type[0];
	t[1] = '\0';
	ds->i = 0;	/* XXX rpmdsNewN() needs ds->i = 0, not -1 */
/*@i@*/	ds->DNEVR = rpmdsNewDNEVR(t, ds);
    }

    return rpmdsLink(ds, (ds ? ds->Type : NULL));
}

int rpmdsCount(const rpmds ds)
{
    return (ds != NULL ? ds->Count : 0);
}

int rpmdsIx(const rpmds ds)
{
    return (ds != NULL ? ds->i : -1);
}

int rpmdsSetIx(rpmds ds, int ix)
{
    int i = -1;

    if (ds != NULL) {
	i = ds->i;
	ds->i = ix;
    }
    return i;
}

const char * rpmdsDNEVR(const rpmds ds)
{
    const char * DNEVR = NULL;

    if (ds != NULL && ds->i >= 0 && ds->i < (int)ds->Count) {
	if (ds->DNEVR != NULL)
	    DNEVR = ds->DNEVR;
    }
    return DNEVR;
}

const char * rpmdsN(const rpmds ds)
{
    const char * N = NULL;

    if (ds != NULL && ds->i >= 0 && ds->i < (int)ds->Count) {
/*@-globs -mods @*/	/* FIX: correct annotations for ds->ns shadow */
	N = (ds->ns.N ? ds->ns.N : rpmdsNewN(ds));
/*@=globs =mods @*/
    }
    return N;
}

const char * rpmdsEVR(const rpmds ds)
{
    const char * EVR = NULL;

    if (ds != NULL && ds->i >= 0 && ds->i < (int)ds->Count) {
	if (ds->EVR != NULL)
	    EVR = ds->EVR[ds->i];
    }
    return EVR;
}

evrFlags rpmdsFlags(const rpmds ds)
{
    evrFlags Flags = (evrFlags) 0;

    if (ds != NULL && ds->i >= 0 && ds->i < (int)ds->Count) {
	if (ds->Flags != NULL)
	    Flags = ds->Flags[ds->i];
    }
    return Flags;
}

rpmTag rpmdsTagN(const rpmds ds)
{
    rpmTag tagN = (rpmTag) 0;

    if (ds != NULL)
	tagN = ds->tagN;
    return tagN;
}

const char * rpmdsA(const rpmds ds)
{
    const char * A = NULL;

    if (ds != NULL)
	A = ds->A;
    return A;
}

time_t rpmdsBT(const rpmds ds)
{
    time_t BT = 0;
    if (ds != NULL && ds->BT > 0)
	BT = ds->BT;
    return BT;
}

time_t rpmdsSetBT(const rpmds ds, time_t BT)
{
    time_t oBT = 0;
    if (ds != NULL) {
	oBT = (time_t)ds->BT;
	ds->BT = (rpmuint32_t)BT;
    }
    return oBT;
}

nsType rpmdsNSType(const rpmds ds)
{
    nsType NSType = RPMNS_TYPE_UNKNOWN;
    if (ds != NULL)
	NSType = ds->ns.Type;
    return NSType;
}

int rpmdsNoPromote(const rpmds ds)
{
    int nopromote = 0;

    if (ds != NULL)
	nopromote = ds->nopromote;
    return nopromote;
}

int rpmdsSetNoPromote(rpmds ds, int nopromote)
{
    int onopromote = 0;

    if (ds != NULL) {
	onopromote = ds->nopromote;
	ds->nopromote = nopromote;
    }
    return onopromote;
}

void * rpmdsSetEVRparse(rpmds ds,
	int (*EVRparse)(const char *evrstr, EVR_t evr))
{
    void * oEVRparse = NULL;

    if (ds != NULL) {
/*@i@*/	oEVRparse = (void *) ds->EVRparse;
/*@i@*/	ds->EVRparse = EVRparse;
    }
    return oEVRparse;
}

void * rpmdsSetEVRcmp(rpmds ds, int (*EVRcmp)(const char *a, const char *b))
{
    void * oEVRcmp = NULL;

    if (ds != NULL) {
/*@i@*/	oEVRcmp = (void *) ds->EVRcmp;
/*@i@*/	ds->EVRcmp = EVRcmp;
    }
    return oEVRcmp;
}

rpmuint32_t rpmdsColor(const rpmds ds)
{
    rpmuint32_t Color = 0;

    if (ds != NULL && ds->i >= 0 && ds->i < (int)ds->Count) {
	if (ds->Color != NULL)
	    Color = ds->Color[ds->i];
    }
    return Color;
}

rpmuint32_t rpmdsSetColor(const rpmds ds, rpmuint32_t color)
{
    rpmuint32_t ocolor = 0;

    if (ds == NULL)
	return ocolor;

    if (ds->Color == NULL && ds->Count > 0)	/* XXX lazy malloc */
	ds->Color = (rpmuint32_t *) xcalloc(ds->Count, sizeof(*ds->Color));

    if (ds->i >= 0 && ds->i < (int)ds->Count) {
	if (ds->Color != NULL) {
	    ocolor = ds->Color[ds->i];
	    ds->Color[ds->i] = color;
	}
    }
    return ocolor;
}

void * rpmdsExclude(const rpmds ds)
{
    return (ds != NULL ? ds->exclude : NULL);
}

int rpmdsNExclude(const rpmds ds)
{
    return (ds != NULL ? ds->nexclude : 0);
}

void * rpmdsInclude(const rpmds ds)
{
    return (ds != NULL ? ds->include : NULL);
}

int rpmdsNInclude(const rpmds ds)
{
    return (ds != NULL ? ds->ninclude : 0);
}

rpmuint32_t rpmdsRefs(const rpmds ds)
{
    rpmuint32_t Refs = 0;

    if (ds != NULL && ds->i >= 0 && ds->i < (int)ds->Count) {
	if (ds->Refs != NULL)
	    Refs = ds->Refs[ds->i];
    }
    return Refs;
}

rpmuint32_t rpmdsSetRefs(const rpmds ds, rpmuint32_t refs)
{
    rpmuint32_t orefs = 0;

    if (ds == NULL)
	return orefs;

    if (ds->Refs == NULL && ds->Count > 0)	/* XXX lazy malloc */
	ds->Refs = (rpmuint32_t *) xcalloc(ds->Count, sizeof(*ds->Refs));

    if (ds->i >= 0 && ds->i < (int)ds->Count) {
	if (ds->Refs != NULL) {
	    orefs = ds->Refs[ds->i];
	    ds->Refs[ds->i] = refs;
	}
    }
    return orefs;
}

rpmint32_t rpmdsResult(const rpmds ds)
{
    rpmint32_t result = 0;

    if (ds != NULL && ds->i >= 0 && ds->i < (int)ds->Count) {
	if (ds->Result != NULL)
	    result = ds->Result[ds->i];
    }
    return result;
}

rpmint32_t rpmdsSetResult(const rpmds ds, rpmint32_t result)
{
    rpmint32_t oresult = 0;

    if (ds == NULL)
	return oresult;

    if (ds->Result == NULL && ds->Count > 0)	/* XXX lazy malloc */
	ds->Result = (rpmint32_t *) xcalloc(ds->Count, sizeof(*ds->Result));

    if (ds->i >= 0 && ds->i < (int)ds->Count) {
	if (ds->Result != NULL) {
	    oresult = ds->Result[ds->i];
	    ds->Result[ds->i] = result;
	}
    }
    return oresult;
}

void rpmdsNotify(rpmds ds, const char * where, int rc)
{
    if (!(ds != NULL && ds->i >= 0 && ds->i < (int)ds->Count))
	return;
    if (ds->DNEVR == NULL)
	return;

    rpmlog(RPMLOG_DEBUG, "%9s: %-45s %-s %s\n", rpmdsTagName(ds->tagN),
		(!strcmp(ds->DNEVR, "cached") ? ds->DNEVR : ds->DNEVR+2),
		(rc ? _("NO ") : _("YES")),
		(where != NULL ? where : ""));
}

int rpmdsNext(/*@null@*/ rpmds ds)
	/*@modifies ds @*/
{
    int i = -1;

    if (ds != NULL && ++ds->i >= 0) {
	if (ds->i < (int)ds->Count) {
	    char t[2];
	    i = ds->i;
	    ds->DNEVR = _free(ds->DNEVR);
	    ds->ns.str = _free(ds->ns.str);
	    memset(&ds->ns, 0, sizeof(ds->ns));
	    t[0] = ((ds->Type != NULL) ? ds->Type[0] : '\0');
	    t[1] = '\0';
	    /*@-nullstate@*/
	   /*@i@*/ ds->DNEVR = rpmdsNewDNEVR(t, ds);
	    /*@=nullstate@*/

	} else
	    ds->i = -1;

/*@-modfilesys @*/
if (_rpmds_debug  < 0 && i != -1 && ds->DNEVR[2] != '\0')
fprintf(stderr, "*** ds %p\t%s[%d]: %s\n", ds, (ds->Type ? ds->Type : "?Type?"), i, (ds->DNEVR ? ds->DNEVR : "?DNEVR?"));
/*@=modfilesys @*/

    }

    return i;
}

rpmds rpmdsInit(/*@null@*/ rpmds ds)
	/*@modifies ds @*/
{
    if (ds != NULL)
	ds->i = -1;
    /*@-refcounttrans@*/
    return ds;
    /*@=refcounttrans@*/
}

/*@null@*/
static rpmds rpmdsDup(const rpmds ods)
	/*@modifies ods @*/
{
    rpmds ds = rpmdsGetPool(_rpmdsPool);
    size_t nb;

/*@-assignexpose -castexpose @*/
    ds->h = (ods->h != NULL ? headerLink(ods->h) : NULL);
    ds->Type = ods->Type;
/*@=assignexpose =castexpose @*/
    ds->tagN = ods->tagN;
    ds->Count = ods->Count;
    ds->i = ods->i;
    ds->l = ods->l;
    ds->u = ods->u;

    nb = (ds->Count+1) * sizeof(*ds->N);
    ds->N = (const char **) (ds->h != NULL
	? memcpy(xmalloc(nb), ods->N, nb)
	: rpmdsDupArgv(ods->N, ods->Count) );

    /* XXX rpm prior to 3.0.2 did not always supply EVR and Flags. */
assert(ods->EVR != NULL);
assert(ods->Flags != NULL);

    nb = (ds->Count+1) * sizeof(*ds->EVR);
    ds->EVR = (const char **) (ds->h != NULL
	? memcpy(xmalloc(nb), ods->EVR, nb)
	: rpmdsDupArgv(ods->EVR, ods->Count) );

    nb = (ds->Count * sizeof(*ds->Flags));
    ds->Flags = (evrFlags *) (ds->h != NULL
	? ods->Flags
	: memcpy(xmalloc(nb), ods->Flags, nb) );
    ds->nopromote = ods->nopromote;
/*@-assignexpose@*/
    ds->EVRcmp = ods->EVRcmp;;
/*@=assignexpose@*/

/*@-compmempass@*/ /* FIX: ds->Flags is kept, not only */
    return rpmdsLink(ds, (ds ? ds->Type : NULL));
/*@=compmempass@*/
}

int rpmdsFind(rpmds ds, const rpmds ods)
{
    int comparison;

    if (ds == NULL || ods == NULL)
	return -1;

    ds->l = 0;
    ds->u = ds->Count;
    while (ds->l < ds->u) {
	ds->i = (ds->l + ds->u) / 2;

	comparison = strcmp(ods->N[ods->i], ds->N[ds->i]);

	/* XXX rpm prior to 3.0.2 did not always supply EVR and Flags. */
/*@-nullderef@*/
	if (comparison == 0 && ods->EVR && ds->EVR)
	    comparison = strcmp(ods->EVR[ods->i], ds->EVR[ds->i]);
	if (comparison == 0 && ods->Flags && ds->Flags)
	    comparison = (ods->Flags[ods->i] - ds->Flags[ds->i]);
/*@=nullderef@*/

	if (comparison < 0)
	    ds->u = ds->i;
	else if (comparison > 0)
	    ds->l = ds->i + 1;
	else
	    return ds->i;
    }
    return -1;
}

int rpmdsMerge(rpmds * dsp, rpmds ods)
{
    rpmds ds;
    const char ** N;
    const char ** EVR;
    evrFlags * Flags;
    int j;
int save;

    if (dsp == NULL || ods == NULL)
	return -1;

    /* If not initialized yet, dup the 1st entry. */
    if (*dsp == NULL) {
	save = ods->Count;
	ods->Count = 1;
	*dsp = rpmdsDup(ods);
	ods->Count = save;
    }
    ds = *dsp;
    if (ds == NULL)
	return -1;

    /*
     * Add new entries.
     */
save = ods->i;
    ods = rpmdsInit(ods);
    if (ods != NULL)
    while (rpmdsNext(ods) >= 0) {
	/*
	 * If this entry is already present, don't bother.
	 */
	if (rpmdsFind(ds, ods) >= 0)
	    continue;

	/*
	 * Insert new entry.
	 */
	for (j = ds->Count; j > (int)ds->u; j--)
	    ds->N[j] = ds->N[j-1];
	ds->N[ds->u] = ods->N[ods->i];
	N = rpmdsDupArgv(ds->N, ds->Count+1);
	ds->N = _free(ds->N);
	ds->N = N;
	
	/* XXX rpm prior to 3.0.2 did not always supply EVR and Flags. */
/*@-nullderef -nullpass -nullptrarith @*/
assert(ods->EVR != NULL);
assert(ods->Flags != NULL);

	for (j = ds->Count; j > (int)ds->u; j--)
	    ds->EVR[j] = ds->EVR[j-1];
	ds->EVR[ds->u] = ods->EVR[ods->i];
	EVR = rpmdsDupArgv(ds->EVR, ds->Count+1);
	ds->EVR = _free(ds->EVR);
	ds->EVR = EVR;

	Flags = (evrFlags *) xmalloc((ds->Count+1) * sizeof(*Flags));
	if (ds->u > 0)
	    memcpy(Flags, ds->Flags, ds->u * sizeof(*Flags));
	if (ds->u < ds->Count)
	    memcpy(Flags + ds->u + 1, ds->Flags + ds->u, (ds->Count - ds->u) * sizeof(*Flags));
	Flags[ds->u] = ods->Flags[ods->i];
	ds->Flags = _free(ds->Flags);
	ds->Flags = Flags;
/*@=nullderef =nullpass =nullptrarith @*/

	ds->i = -1;
	ds->Count++;

    }
/*@-nullderef@*/
ods->i = save;
/*@=nullderef@*/
    return 0;
}

int rpmdsSearch(rpmds ds, rpmds ods)
{
    int comparison;
    int i, l, u;

    if (ds == NULL || ods == NULL)
	return -1;

    /* Binary search to find the [l,u) subset that contains N */
    i = -1;
    l = 0;
    u = ds->Count;
    while (l < u) {
	i = (l + u) / 2;

	comparison = strcmp(ods->N[ods->i], ds->N[i]);

	if (comparison < 0)
	    u = i;
	else if (comparison > 0)
	    l = i + 1;
	else {
	    /* Set l to 1st member of set that contains N. */
	    if (strcmp(ods->N[ods->i], ds->N[l]))
		l = i;
	    while (l > 0 && !strcmp(ods->N[ods->i], ds->N[l-1]))
		l--;
	    /* Set u to 1st member of set that does not contain N. */
	    if (u >= (int)ds->Count || strcmp(ods->N[ods->i], ds->N[u]))
		u = i;
	    while (++u < (int)ds->Count) {
		if (strcmp(ods->N[ods->i], ds->N[u]))
		    /*@innerbreak@*/ break;
	    }
	    break;
	}
    }

    /* Check each member of [l,u) subset for ranges overlap. */
    i = -1;
    if (l < u) {
	int save = rpmdsSetIx(ds, l-1);
	while ((l = rpmdsNext(ds)) >= 0 && (l < u)) {
	    if ((i = rpmdsCompare(ods, ds)) != 0)
		break;
	}
	/* Return element index that overlaps, or -1. */
	if (i)
	    i = rpmdsIx(ds);
	else {
	    (void) rpmdsSetIx(ds, save);
	    i = -1;
	}
	/* Save the return value. */
	if (ods->Result != NULL)
	    (void) rpmdsSetResult(ods, (i != -1 ? 1 : 0));
    }
    return i;
}

/**
 * Merge a single provides, wrapping N as "NS(N)".
 * @retval *dsp		(loaded) dependency set
 * @param NS		dependency name space
 * @param N		name
 * @param EVR		epoch:version-release
 * @param Flags		comparison/context flags
 */
static void rpmdsNSAdd(/*@out@*/ rpmds *dsp, const char * NS,
		const char *N, const char *EVR, evrFlags Flags)
	/*@modifies *dsp @*/
{
    char *t;
    rpmds ds;
    int xx;

    t = (char *) alloca(strlen(NS)+sizeof("()")+strlen(N));
    *t = '\0';
    (void) stpcpy( stpcpy( stpcpy( stpcpy(t, NS), "("), N), ")");

    ds = rpmdsSingle(RPMTAG_PROVIDENAME, t, EVR, Flags);
    xx = rpmdsMerge(dsp, ds);
    (void)rpmdsFree(ds);
    ds = NULL;
}

#if defined(WITH_CPUINFO)
int rpmdsCpuinfo(rpmds *dsp, const char * fn)
{
    const char * NS = "cpuinfo";
    struct cpuinfo *cip = cpuinfo_new();
    cpuinfo_feature_t feature;
    char tmp[20];
    union _dbswap {
     	rpmuint32_t ui;
     	unsigned char uc[4];
    };
    static union _dbswap orderedbytes = { .ui = 0x11223344 };
    const char * endian = NULL;

    snprintf(tmp, 19, "%d", cpuinfo_get_frequency(cip));
    tmp[19] = '\0';
    rpmdsNSAdd(dsp, NS, "cpu_MHz", tmp, RPMSENSE_PROBE|RPMSENSE_EQUAL);
    snprintf(tmp, 19, "%d", cpuinfo_get_cores(cip));
    rpmdsNSAdd(dsp, NS, "cpu_cores", tmp, RPMSENSE_PROBE|RPMSENSE_EQUAL);
    snprintf(tmp, 19, "%d", cpuinfo_get_threads(cip));
    rpmdsNSAdd(dsp, NS, "cpu_threads", tmp, RPMSENSE_PROBE|RPMSENSE_EQUAL);

    if(orderedbytes.uc[0] == 0x44)
	endian = "little";
    else if(orderedbytes.uc[0] == 0x11)
	endian = "big";
    else if(orderedbytes.uc[0] == 0x22)
	endian = "pdp";
    rpmdsNSAdd(dsp, NS, "endian", endian, RPMSENSE_PROBE|RPMSENSE_EQUAL);

    for (feature = cpuinfo_feature_common; feature != cpuinfo_feature_architecture_max; feature++) {
	if(feature == cpuinfo_feature_common_max)
	    feature = cpuinfo_feature_architecture;
	if (cpuinfo_has_feature(cip, feature)) {
	    const char *name = cpuinfo_string_of_feature(feature);
	    if (name)
		rpmdsNSAdd(dsp, NS, name, "", RPMSENSE_PROBE);
	}
    }
    cpuinfo_destroy(cip);

    return RPMRC_OK;
}    

#else

struct cpuinfo_s {
/*@observer@*/ /*@null@*/
    const char *name;
    int done;
    int flags;
};

/*@unchecked@*/
static struct cpuinfo_s ctags[] = {
    { "processor",	0,  0 },
    { "Processor",	0,  1 },	/* XXX armv5 */
    { "vendor_id",	0,  0 },
    { "cpu_family",	0,  1 },
    { "model",		0,  1 },
    { "model_name",	0,  0 },
    { "stepping",	0,  1 },
    { "cpu_MHz",	0,  1 },
    { "CPU_implementer",0,  1 },	/* XXX armv5 */
    { "CPU_architecture",0,  1 },	/* XXX armv5 */
    { "CPU_variant",	0,  1 },	/* XXX armv5 */
    { "CPU_part",	0,  1 },	/* XXX armv5 */
    { "CPU_revision",	0,  1 },	/* XXX armv5 */
    { "Hardware",	0,  2 },	/* XXX armv5 */
    { "Revision",	0,  1 },	/* XXX armv5 */
    { "Serial",		0,  1 },	/* XXX armv5 */
    { "cache_size",	0,  1 },
    { "physical_id",	0,  0 },
    { "siblings",	0,  0 },
    { "core_id",	0,  0 },
    { "cpu_cores",	0,  0 },
    { "fdiv_bug",	0,  3 },
    { "hlt_bug",	0,  3 },
    { "f00f_bug",	0,  3 },
    { "coma_bug",	0,  3 },
    { "fpu",		0,  0 },	/* XXX use flags attribute instead. */
    { "fpu_exception",	0,  3 },
    { "cpuid_level",	0,  0 },
    { "wp",		0,  3 },
    { "flags",		0,  4 },
    { "Features",	0,  4 },	/* XXX armv5 */
    { "bogomips",	0,  1 },
    { "BogoMIPS",	0,  1 },	/* XXX armv5 */
    { "clflush_size",	0,  1 },
    { NULL,		0, -1 }
};

/**
 * Return dependency format to use for a cpuinfo line.
 * @param name		field name
 * @return		type of format (0 == ignore, -1 == not found)
 */
static int rpmdsCpuinfoCtagFlags(const char * name)
	/*@globals ctags @*/
	/*@modifies ctags @*/
{
    struct cpuinfo_s * ct;
    int flags = -1;

    for (ct = ctags; ct->name != NULL; ct++) {
	if (strcmp(ct->name, name))
	    continue;
	if (ct->done)
	    continue;
	ct->done = 1;		/* XXX insure single occurrence */
	flags = ct->flags;
	break;
    }
    return flags;
}

#define	_PROC_CPUINFO	"/proc/cpuinfo"
/**
 */
/*@unchecked@*/ /*@observer@*/ /*@owned@*/ /*@relnull@*/
const char * _cpuinfo_path = NULL;

int rpmdsCpuinfo(rpmds *dsp, const char * fn)
	/*@globals _cpuinfo_path, ctags @*/
	/*@modifies _cpuinfo_path, ctags @*/
{
    struct cpuinfo_s * ct;
    const char * NS = "cpuinfo";
    rpmiob iob = NULL;
    char * f, * fe, * fend;
    char * g, * ge;
    char * t;
    int rc = -1;
    int xx;

/*@-modobserver@*/
    if (_cpuinfo_path == NULL) {
	_cpuinfo_path = rpmExpand("%{?_rpmds_cpuinfo_path}", NULL);
	/* XXX may need to validate path existence somewhen. */
	if (!(_cpuinfo_path != NULL && *_cpuinfo_path == '/')) {
/*@-observertrans @*/
	    _cpuinfo_path = _free(_cpuinfo_path);
/*@=observertrans @*/
	    _cpuinfo_path = xstrdup(_PROC_CPUINFO);
	}
    }
/*@=modobserver@*/

    if (fn == NULL)
	fn = _cpuinfo_path;

    /* Reset done variables. */
    for (ct = ctags; ct->name != NULL; ct++)
	ct->done = 0;

    xx = rpmiobSlurp(fn, &iob);
    if (!(xx == 0 && iob != NULL))
	goto exit;

    for (f = (char *)iob->b; *f != '\0'; f = fend) {
	/* find EOL */
	fe = f;
	while (*fe != '\0' && !(*fe == '\n' || *fe == '\r'))
	    fe++;
	ge = fe;
	while (*fe != '\0' && (*fe == '\n' || *fe == '\r'))
	    *fe++ = '\0';
	fend = fe;

	/* rtrim on line. */
	while (--ge > f && _isspace(*ge))
	    *ge = '\0';

	/* ltrim on line. */
	while (*f && _isspace(*f))
	    f++;

	/* split on ':' */
	fe = f;
	while (*fe && *fe != ':')
            fe++;
	if (*fe == '\0')
	    continue;
	g = fe + 1;

	/* rtrim on field 1. */
	*fe = '\0';
	while (--fe > f && _isspace(*fe))
	    *fe = '\0';
	if (*f == '\0')
	    continue;

	/* ltrim on field 2. */
	while (*g && _isspace(*g))
            g++;
	if (*g == '\0')
	    continue;

	for (t = f; *t != '\0'; t++) {
	    if (_isspace(*t))
		*t = '_';
	}

	switch (rpmdsCpuinfoCtagFlags(f)) {
	case -1:	/* not found */
	case 0:		/* ignore */
	default:
	    continue;
	    /*@notreached@*/ /*@switchbreak@*/ break;
	case 1:		/* Provides: cpuinfo(f) = g */
	    for (t = g; *t != '\0'; t++) {
		if (_isspace(*t) || *t == '(' || *t == ')')
		    *t = '_';
	    }
	    rpmdsNSAdd(dsp, NS, f, g, (evrFlags)(RPMSENSE_PROBE|RPMSENSE_EQUAL));
	    /*@switchbreak@*/ break;
	case 2:		/* Provides: cpuinfo(g) */
	    for (t = g; *t != '\0'; t++) {
		if (_isspace(*t) || *t == '(' || *t == ')')
		    *t = '_';
	    }
	    rpmdsNSAdd(dsp, NS, g, "", RPMSENSE_PROBE);
	    /*@switchbreak@*/ break;
	case 3:		/* if ("yes") Provides: cpuinfo(f) */
	   if (!strcmp(g, "yes"))
		rpmdsNSAdd(dsp, NS, f, "", RPMSENSE_PROBE);
	    /*@switchbreak@*/ break;
	case 4:		/* Provides: cpuinfo(g[i]) */
	{   char ** av = NULL;
	    int i = 0;
	    rc = poptParseArgvString(g, NULL, (const char ***)&av);
	    if (!rc && av != NULL)
	    while ((t = av[i++]) != NULL)
		rpmdsNSAdd(dsp, NS, t, "", RPMSENSE_PROBE);
	    t = NULL;
	    if (av != NULL)
		free(av);
	}   /*@switchbreak@*/ break;
	}
    }

exit:
    iob = rpmiobFree(iob);
    return rc;
}
#endif

struct rpmlibProvides_s {
/*@observer@*/ /*@relnull@*/
    const char * featureName;
/*@observer@*/ /*@relnull@*/
    const char * featureEVR;
    evrFlags featureFlags;
/*@observer@*/ /*@relnull@*/
    const char * featureDescription;
};

/*@unchecked@*/ /*@observer@*/
static struct rpmlibProvides_s rpmlibProvides[] = {
    { "rpmlib(VersionedDependencies)",	"3.0.3-1",
	(evrFlags)(RPMSENSE_RPMLIB|RPMSENSE_EQUAL),
    N_("PreReq:, Provides:, and Obsoletes: dependencies support versions.") },
    { "rpmlib(CompressedFileNames)",	"3.0.4-1",
	(evrFlags)(RPMSENSE_RPMLIB|RPMSENSE_EQUAL),
    N_("file name(s) stored as (dirName,baseName,dirIndex) tuple, not as path.")},
#if defined(WITH_BZIP2)
    { "rpmlib(PayloadIsBzip2)",		"3.0.5-1",
	(evrFlags)(RPMSENSE_RPMLIB|RPMSENSE_EQUAL),
    N_("package payload can be compressed using bzip2.") },
#endif
    { "rpmlib(PayloadFilesHavePrefix)",	"4.0-1",
	(evrFlags)(RPMSENSE_RPMLIB|RPMSENSE_EQUAL),
    N_("package payload file(s) have \"./\" prefix.") },
    { "rpmlib(ExplicitPackageProvide)",	"4.0-1",
	(evrFlags)(RPMSENSE_RPMLIB|RPMSENSE_EQUAL),
    N_("package name-version-release is not implicitly provided.") },
    { "rpmlib(HeaderLoadSortsTags)",	"4.0.1-1",
	(evrFlags)(RPMSENSE_RPMLIB|RPMSENSE_EQUAL),
    N_("header tags are always sorted after being loaded.") },
    { "rpmlib(ScriptletInterpreterArgs)",    "4.0.3-1",
	(evrFlags)(RPMSENSE_RPMLIB|RPMSENSE_EQUAL),
    N_("the scriptlet interpreter can use arguments from header.") },
    { "rpmlib(PartialHardlinkSets)",	"4.0.4-1",
	(evrFlags)(RPMSENSE_RPMLIB|RPMSENSE_EQUAL),
    N_("a hardlink file set may be installed without being complete.") },
    { "rpmlib(ConcurrentAccess)",	"4.1-1",
	(evrFlags)(RPMSENSE_RPMLIB|RPMSENSE_EQUAL),
    N_("package scriptlets may access the rpm database while installing.") },
#if defined(WITH_LUA)
    { "rpmlib(BuiltinLuaScripts)",	"4.2.2-1",
	(evrFlags)(RPMSENSE_RPMLIB|RPMSENSE_EQUAL),
    N_("internal embedded lua scripts.") },
#endif
#if defined(WITH_AUGEAS)
    { "rpmlib(BuiltinAugeasScripts)",	"5.3-1",
	(evrFlags)(RPMSENSE_RPMLIB|RPMSENSE_EQUAL),
    N_("internal embedded Augeas.") },
#endif
#if defined(WITH_FICL)
    { "rpmlib(BuiltinFiclScripts)",	"5.2-1",
	(evrFlags)(RPMSENSE_RPMLIB|RPMSENSE_EQUAL),
    N_("internal embedded FICL.") },
#endif
#if defined(WITH_JNIEMBED)
    { "rpmlib(BuiltinJVMBeanShell)",	"5.4.15-1",
	(evrFlags)(RPMSENSE_RPMLIB|RPMSENSE_EQUAL),
    N_("internal embedded JVM BeanShell.") },
#endif
#if defined(WITH_GPSEE)
    { "rpmlib(BuiltinJavaScript)",	"5.2-1",
	(evrFlags)(RPMSENSE_RPMLIB|RPMSENSE_EQUAL),
    N_("internal embedded JavaScript.") },
#endif
#if defined(WITH_MRBEMBED)
    { "rpmlib(BuiltinMRubyScripts)",	"5.4.15-1",
	(evrFlags)(RPMSENSE_RPMLIB|RPMSENSE_EQUAL),
    N_("internal embedded Mruby scripts.") },
#endif
#if defined(WITH_PERLEMBED)
    { "rpmlib(BuiltinPerlScripts)",	"5.2-1",
	(evrFlags)(RPMSENSE_RPMLIB|RPMSENSE_EQUAL),
    N_("internal embedded perl scripts.") },
#endif
#if defined(WITH_PYTHONEMBED)
    { "rpmlib(BuiltinPythonScripts)",	"5.2-1",
	(evrFlags)(RPMSENSE_RPMLIB|RPMSENSE_EQUAL),
    N_("internal embedded python scripts.") },
#endif
#if defined(WITH_RUBYEMBED)
    { "rpmlib(BuiltinRubyScripts)",	"5.2-1",
	(evrFlags)(RPMSENSE_RPMLIB|RPMSENSE_EQUAL),
    N_("internal embedded Cruby scripts.") },
#endif
#if defined(WITH_SEMANAGE)
    { "rpmlib(BuiltinSpookScripts)",	"5.3-1",
	(evrFlags)(RPMSENSE_RPMLIB|RPMSENSE_EQUAL),
    N_("internal embedded Spook scripts.") },
#endif
#if defined(WITH_SQLITE)
    { "rpmlib(BuiltinSqlScripts)",	"5.3-1",
	(evrFlags)(RPMSENSE_RPMLIB|RPMSENSE_EQUAL),
    N_("internal embedded sqlite3 scripts.") },
#endif
#if defined(WITH_SQUIRREL)
    { "rpmlib(BuiltinSquirrelScripts)",	"5.2-1",
	(evrFlags)(RPMSENSE_RPMLIB|RPMSENSE_EQUAL),
    N_("internal embedded squirrel scripts.") },
#endif
#if defined(WITH_TCL)
    { "rpmlib(BuiltinTclScripts)",	"5.2-1",
	(evrFlags)(RPMSENSE_RPMLIB|RPMSENSE_EQUAL),
    N_("internal embedded tcl scripts.") },
#endif
    { "rpmlib(HeaderTagTypeInt64)",	"4.4.3-1",
	(evrFlags)(RPMSENSE_RPMLIB|RPMSENSE_EQUAL),
    N_("header tag data can be of type uint64_t.") },
    { "rpmlib(PayloadIsUstar)",		"4.4.4-1",
	(evrFlags)(RPMSENSE_RPMLIB|RPMSENSE_EQUAL),
    N_("package payload can be in ustar tar archive format.") },
#if defined(WITH_XZ)	/* XXX should be 4.4.6, but use SuSE's 4.4.2 instead */
    { "rpmlib(PayloadIsLzma)",		"4.4.2-1",
	(evrFlags)(RPMSENSE_RPMLIB|RPMSENSE_EQUAL),
    N_("package payload can be compressed using lzma.") },
#endif
    { "rpmlib(FileDigestParameterized)",    "4.4.6-1",
	(evrFlags)(RPMSENSE_RPMLIB|RPMSENSE_EQUAL),
    N_("file digests can be other than MD5.") },
    { "rpmlib(FileDigests)",    "4.6.0-1",
	(evrFlags)(RPMSENSE_RPMLIB|RPMSENSE_EQUAL),
    N_("file digests can be other than MD5.") },
#if defined(SUPPORT_AR_PAYLOADS)
    { "rpmlib(PayloadIsAr)",		"5.1-1",
	(evrFlags)(RPMSENSE_RPMLIB|RPMSENSE_EQUAL),
    N_("package payload can be in ar archive format.") },
#endif
#if defined(WITH_XZ)
    { "rpmlib(PayloadIsXz)",		"5.2-1",
	(evrFlags)(RPMSENSE_RPMLIB|RPMSENSE_EQUAL),
    N_("package payload can be compressed using xz.") },
#endif
    /* XXX FIXME: Alt should _NOT_ have added a tracking dependency. */
    { "rpmlib(SetVersions)",		"4.0.4-alt98",
        (evrFlags)(RPMSENSE_RPMLIB|RPMSENSE_EQUAL),
    N_("dependencies support set/subset versions.") },
    { NULL,				NULL, (evrFlags)0,	NULL }
};

/**
 * Load rpmlib provides into a dependency set.
 * @retval *dsp		(loaded) depedency set
 * @param tblp		rpmlib provides table (NULL uses internal table)
 * @return		0 on success
 */
int rpmdsRpmlib(rpmds * dsp, void * tblp)
{
    const struct rpmlibProvides_s * rltblp = (struct rpmlibProvides_s *) tblp;
    const struct rpmlibProvides_s * rlp;
    int xx;

    if (rltblp == NULL)
	rltblp = rpmlibProvides;

    for (rlp = rltblp; rlp->featureName != NULL; rlp++) {
	rpmds ds = rpmdsSingle(RPMTAG_PROVIDENAME, rlp->featureName,
			rlp->featureEVR, rlp->featureFlags);
	xx = rpmdsMerge(dsp, ds);
	(void)rpmdsFree(ds);
	ds = NULL;
    }
    return 0;
}

/**
 * Merge contents of a sysinfo tag file into sysinfo dependencies.
 * @retval *PRCO	provides/requires/conflicts/obsoletes depedency set(s)
 * @param fn		path to file
 * @param tagN		dependency set tag
 * @return		0 on success
 */
static int rpmdsSysinfoFile(rpmPRCO PRCO, const char * fn, rpmTag tagN)
	/*@globals h_errno, fileSystem, internalState @*/
	/*@modifies PRCO, fileSystem, internalState @*/
{
    char buf[BUFSIZ];
    const char *N, *EVR;
    evrFlags Flags;
    rpmds ds;
    char * f, * fe;
    char * g, * ge;
    FD_t fd = NULL;
    FILE * fp;
    int rc = -1;
    int ln;
    int xx;

    /* XXX for now, collect Dirnames/Filelinktos in Providename */
    if (tagN == RPMTAG_DIRNAMES || tagN == RPMTAG_FILELINKTOS)
	tagN = RPMTAG_PROVIDENAME;

assert(fn != NULL);
    fd = Fopen(fn, "r.fpio");
    if (fd == NULL || Ferror(fd))
	goto exit;
    fp = fdGetFILE(fd);

    ln = 0;
    if (fp != NULL)
    while((f = fgets(buf, (int)sizeof(buf), fp)) != NULL) {
	ln++;

	/* insure a terminator. */
	buf[sizeof(buf)-1] = '\0';

	/* ltrim on line. */
	while (*f && _isspace(*f))
	    f++;

	/* XXX skip YAML "- " markup */
	if (f[0] == '-' && _isspace(f[1])) {
	    f += sizeof("- ")-1;
	    while (*f && _isspace(*f))
		f++;
	}

	/* skip empty lines and comments */
	if (*f == '\0' || *f == '#')
	    continue;

	/* rtrim on line. */
	fe = f + strlen(f);
	while (--fe > f && _isspace(*fe))
	    *fe = '\0';

	if (!(xisalnum(f[0]) || strchr("/_%!", f[0]) != NULL)) {
	    fprintf(stderr, _("%s:%d \"%s\" has invalid name. Skipping ...\n"),
		    fn, ln, f);
	    continue;
        }

	/* split on ' '  or comparison operator. */
	fe = f;
	if (*f == '!') fe++;
	while (*fe && !_isspace(*fe) && strchr("!<=>", *fe) == NULL)
            fe++;
	while (*fe && _isspace(*fe))
	    *fe++ = '\0';

	N = f;
	EVR = NULL;
	Flags = (evrFlags) 0;

	/* parse for non-path, versioned dependency. */
	if (*f != '/' && *fe != '\0') {
	    /* parse comparison operator */
	    g = fe;
	    Flags = rpmEVRflags(fe, (const char **)&g);
	    if (Flags == 0) {
		fprintf(stderr, _("%s:%d \"%s\" has no comparison operator. Skipping ...\n"),
			fn, ln, fe);
		continue;
	    }
	    *fe = '\0';

	    /* ltrim on field 2. */
	    while (*g && _isspace(*g))
		g++;
	    if (*g == '\0') {
		/* XXX No EVR comparison value found. */
		fprintf(stderr, _("%s:%d \"%s\" has no EVR string. Skipping ...\n"),
			fn, ln, f);
		continue;
	    }

	    ge = g + 1;
	    while (*ge && !_isspace(*ge))
		ge++;

	    if (*ge != '\0')
		*ge = '\0';	/* XXX can't happen, line rtrim'ed already. */

	    EVR = g;
	}

	if (EVR == NULL)
	    EVR = "";
	Flags = (evrFlags) (Flags | RPMSENSE_PROBE);
	ds = rpmdsSingle(tagN, N, EVR , Flags);
	if (ds) {	/* XXX can't happen */
	    xx = rpmdsMergePRCO(PRCO, ds);
	    (void)rpmdsFree(ds);
	    ds = NULL;
	}
    }
    rc = 0;

exit:
    if (fd != NULL) (void) Fclose(fd);
    return rc;
}

#if defined(RPM_VENDOR_WINDRIVER)
#define	_ETC_RPM_SYSINFO	"%{_etcrpm}/sysinfo"
#else
#define	_ETC_RPM_SYSINFO	SYSCONFIGDIR "/sysinfo"
#endif

/*@unchecked@*/ /*@observer@*/ /*@owned@*/ /*@relnull@*/
const char *_sysinfo_path = NULL;

/*@-nullassign@*/
/*@unchecked@*/ /*@observer@*/ /*@relnull@*/
static const char *_sysinfo_tags[] = {
    "Providename",
    "Requirename",
    "Conflictname",
    "Obsoletename",
    "Dirnames",
    "Filelinktos",
    NULL
};
/*@=nullassign@*/

int rpmdsSysinfo(rpmPRCO PRCO, const char * fn)
	/*@globals _sysinfo_path @*/
	/*@modifies _sysinfo_path @*/
{
    struct stat * st = (struct stat *) memset(alloca(sizeof(*st)), 0, sizeof(*st));
    int rc = -1;
    int xx;

/*@-modobserver@*/
    if (_sysinfo_path == NULL) {
	_sysinfo_path = rpmExpand("%{?_rpmds_sysinfo_path}", NULL);
	/* XXX may need to validate path existence somewhen. */
	if (!(_sysinfo_path != NULL && *_sysinfo_path == '/')) {
/*@-observertrans @*/
	    _sysinfo_path = _free(_sysinfo_path);
/*@=observertrans @*/
	    _sysinfo_path = xstrdup(_ETC_RPM_SYSINFO);
	}
    }
/*@=modobserver@*/

    if (fn == NULL)
	fn = _sysinfo_path;

    if (fn == NULL)
	goto exit;

    xx = Stat(fn, st);
    if (xx < 0)
	goto exit;

    if (S_ISDIR(st->st_mode)) {
	const char *dn = fn;
	const char **av;
	rpmTag tagN;
	rc = 0;		/* assume success */
	for (av = _sysinfo_tags; av && *av; av++) {
	    tagN = tagValue(*av);
	    if ((int)tagN < 0)	/* XXX FIXME: all tags are valid now */
		continue;
	    fn = rpmGetPath(dn, "/", *av, NULL);
	    st = (struct stat *) memset(st, 0, sizeof(*st));
	    xx = Stat(fn, st);
	    if (xx == 0 && S_ISREG(st->st_mode))
		rc = rpmdsSysinfoFile(PRCO, fn, tagN);
	    fn = _free(fn);
	    if (rc)
		break;
	}
    } else
    /* XXX for now, collect Dirnames/Filelinktos in Providename */
    if (S_ISREG(st->st_mode))
	rc = rpmdsSysinfoFile(PRCO, fn, RPMTAG_PROVIDENAME);

exit:
    return rc;
}

typedef enum { SYSCONF, CONFSTR, PATHCONF } _conf_e;
struct _conf_s {
/*@observer@*/ /*@relnull@*/
    const char *name;
    const int call_name;
    const _conf_e call;
};

/*@unchecked@*/ /*@observer@*/
static const struct _conf_s vars[] = {
#ifdef _PC_LINK_MAX
    { "LINK_MAX", _PC_LINK_MAX, PATHCONF },
#endif
#ifdef _PC_LINK_MAX
    { "_POSIX_LINK_MAX", _PC_LINK_MAX, PATHCONF },
#endif
#ifdef _PC_MAX_CANON
    { "MAX_CANON", _PC_MAX_CANON, PATHCONF },
#endif
#ifdef _PC_MAX_CANON
    { "_POSIX_MAX_CANON", _PC_MAX_CANON, PATHCONF },
#endif
#ifdef _PC_MAX_INPUT
    { "MAX_INPUT", _PC_MAX_INPUT, PATHCONF },
#endif
#ifdef _PC_MAX_INPUT
    { "_POSIX_MAX_INPUT", _PC_MAX_INPUT, PATHCONF },
#endif
#ifdef _PC_NAME_MAX
    { "NAME_MAX", _PC_NAME_MAX, PATHCONF },
#endif
#ifdef _PC_NAME_MAX
    { "_POSIX_NAME_MAX", _PC_NAME_MAX, PATHCONF },
#endif
#ifdef _PC_PATH_MAX
    { "PATH_MAX", _PC_PATH_MAX, PATHCONF },
#endif
#ifdef _PC_PATH_MAX
    { "_POSIX_PATH_MAX", _PC_PATH_MAX, PATHCONF },
#endif
#ifdef _PC_PIPE_BUF
    { "PIPE_BUF", _PC_PIPE_BUF, PATHCONF },
#endif
#ifdef _PC_PIPE_BUF
    { "_POSIX_PIPE_BUF", _PC_PIPE_BUF, PATHCONF },
#endif
#ifdef _PC_SOCK_MAXBUF
    { "SOCK_MAXBUF", _PC_SOCK_MAXBUF, PATHCONF },
#endif
#ifdef _PC_ASYNC_IO
    { "_POSIX_ASYNC_IO", _PC_ASYNC_IO, PATHCONF },
#endif
#ifdef _PC_CHOWN_RESTRICTED
    { "_POSIX_CHOWN_RESTRICTED", _PC_CHOWN_RESTRICTED, PATHCONF },
#endif
#ifdef _PC_NO_TRUNC
    { "_POSIX_NO_TRUNC", _PC_NO_TRUNC, PATHCONF },
#endif
#ifdef _PC_PRIO_IO
    { "_POSIX_PRIO_IO", _PC_PRIO_IO, PATHCONF },
#endif
#ifdef _PC_SYNC_IO
    { "_POSIX_SYNC_IO", _PC_SYNC_IO, PATHCONF },
#endif
#ifdef _PC_VDISABLE
    { "_POSIX_VDISABLE", _PC_VDISABLE, PATHCONF },
#endif

#ifdef _SC_ARG_MAX
    { "ARG_MAX", _SC_ARG_MAX, SYSCONF },
#endif
#ifdef _SC_ATEXIT_MAX
    { "ATEXIT_MAX", _SC_ATEXIT_MAX, SYSCONF },
#endif
#ifdef _SC_CHAR_BIT
    { "CHAR_BIT", _SC_CHAR_BIT, SYSCONF },
#endif
#ifdef _SC_CHAR_MAX
    { "CHAR_MAX", _SC_CHAR_MAX, SYSCONF },
#endif
#ifdef _SC_CHAR_MIN
    { "CHAR_MIN", _SC_CHAR_MIN, SYSCONF },
#endif
#ifdef _SC_CHILD_MAX
    { "CHILD_MAX", _SC_CHILD_MAX, SYSCONF },
#endif
#ifdef _SC_CLK_TCK
    { "CLK_TCK", _SC_CLK_TCK, SYSCONF },
#endif
#ifdef _SC_INT_MAX
    { "INT_MAX", _SC_INT_MAX, SYSCONF },
#endif
#ifdef _SC_INT_MIN
    { "INT_MIN", _SC_INT_MIN, SYSCONF },
#endif
#ifdef _SC_UIO_MAXIOV
    { "IOV_MAX", _SC_UIO_MAXIOV, SYSCONF },
#endif
#ifdef _SC_LOGIN_NAME_MAX
    { "LOGNAME_MAX", _SC_LOGIN_NAME_MAX, SYSCONF },
#endif
#ifdef _SC_LONG_BIT
    { "LONG_BIT", _SC_LONG_BIT, SYSCONF },
#endif
#ifdef _SC_MB_LEN_MAX
    { "MB_LEN_MAX", _SC_MB_LEN_MAX, SYSCONF },
#endif
#ifdef _SC_NGROUPS_MAX
    { "NGROUPS_MAX", _SC_NGROUPS_MAX, SYSCONF },
#endif
#ifdef _SC_NL_ARGMAX
    { "NL_ARGMAX", _SC_NL_ARGMAX, SYSCONF },
#endif
#ifdef _SC_NL_LANGMAX
    { "NL_LANGMAX", _SC_NL_LANGMAX, SYSCONF },
#endif
#ifdef _SC_NL_MSGMAX
    { "NL_MSGMAX", _SC_NL_MSGMAX, SYSCONF },
#endif
#ifdef _SC_NL_NMAX
    { "NL_NMAX", _SC_NL_NMAX, SYSCONF },
#endif
#ifdef _SC_NL_SETMAX
    { "NL_SETMAX", _SC_NL_SETMAX, SYSCONF },
#endif
#ifdef _SC_NL_TEXTMAX
    { "NL_TEXTMAX", _SC_NL_TEXTMAX, SYSCONF },
#endif
#ifdef _SC_GETGR_R_SIZE_MAX
    { "NSS_BUFLEN_GROUP", _SC_GETGR_R_SIZE_MAX, SYSCONF },
#endif
#ifdef _SC_GETPW_R_SIZE_MAX
    { "NSS_BUFLEN_PASSWD", _SC_GETPW_R_SIZE_MAX, SYSCONF },
#endif
#ifdef _SC_NZERO
    { "NZERO", _SC_NZERO, SYSCONF },
#endif
#ifdef _SC_OPEN_MAX
    { "OPEN_MAX", _SC_OPEN_MAX, SYSCONF },
#endif
#ifdef _SC_PAGESIZE
    { "PAGESIZE", _SC_PAGESIZE, SYSCONF },
#endif
#ifdef _SC_PAGESIZE
    { "PAGE_SIZE", _SC_PAGESIZE, SYSCONF },
#endif
#ifdef _SC_PASS_MAX
    { "PASS_MAX", _SC_PASS_MAX, SYSCONF },
#endif
#ifdef _SC_THREAD_DESTRUCTOR_ITERATIONS
    { "PTHREAD_DESTRUCTOR_ITERATIONS", _SC_THREAD_DESTRUCTOR_ITERATIONS, SYSCONF },
#endif
#ifdef _SC_THREAD_KEYS_MAX
    { "PTHREAD_KEYS_MAX", _SC_THREAD_KEYS_MAX, SYSCONF },
#endif
#ifdef _SC_THREAD_STACK_MIN
    { "PTHREAD_STACK_MIN", _SC_THREAD_STACK_MIN, SYSCONF },
#endif
#ifdef _SC_THREAD_THREADS_MAX
    { "PTHREAD_THREADS_MAX", _SC_THREAD_THREADS_MAX, SYSCONF },
#endif
#ifdef _SC_SCHAR_MAX
    { "SCHAR_MAX", _SC_SCHAR_MAX, SYSCONF },
#endif
#ifdef _SC_SCHAR_MIN
    { "SCHAR_MIN", _SC_SCHAR_MIN, SYSCONF },
#endif
#ifdef _SC_SHRT_MAX
    { "SHRT_MAX", _SC_SHRT_MAX, SYSCONF },
#endif
#ifdef _SC_SHRT_MIN
    { "SHRT_MIN", _SC_SHRT_MIN, SYSCONF },
#endif
#ifdef _SC_SSIZE_MAX
    { "SSIZE_MAX", _SC_SSIZE_MAX, SYSCONF },
#endif
#ifdef _SC_TTY_NAME_MAX
    { "TTY_NAME_MAX", _SC_TTY_NAME_MAX, SYSCONF },
#endif
#ifdef _SC_TZNAME_MAX
    { "TZNAME_MAX", _SC_TZNAME_MAX, SYSCONF },
#endif
#ifdef _SC_UCHAR_MAX
    { "UCHAR_MAX", _SC_UCHAR_MAX, SYSCONF },
#endif
#ifdef _SC_UINT_MAX
    { "UINT_MAX", _SC_UINT_MAX, SYSCONF },
#endif
#ifdef _SC_UIO_MAXIOV
    { "UIO_MAXIOV", _SC_UIO_MAXIOV, SYSCONF },
#endif
#ifdef _SC_ULONG_MAX
    { "ULONG_MAX", _SC_ULONG_MAX, SYSCONF },
#endif
#ifdef _SC_USHRT_MAX
    { "USHRT_MAX", _SC_USHRT_MAX, SYSCONF },
#endif
#ifdef _SC_WORD_BIT
    { "WORD_BIT", _SC_WORD_BIT, SYSCONF },
#endif
#ifdef _SC_AVPHYS_PAGES
    { "_AVPHYS_PAGES", _SC_AVPHYS_PAGES, SYSCONF },
#endif
#ifdef _SC_NPROCESSORS_CONF
    { "_NPROCESSORS_CONF", _SC_NPROCESSORS_CONF, SYSCONF },
#endif
#ifdef _SC_NPROCESSORS_ONLN
    { "_NPROCESSORS_ONLN", _SC_NPROCESSORS_ONLN, SYSCONF },
#endif
#ifdef _SC_PHYS_PAGES
    { "_PHYS_PAGES", _SC_PHYS_PAGES, SYSCONF },
#endif
#ifdef _SC_ARG_MAX
    { "_POSIX_ARG_MAX", _SC_ARG_MAX, SYSCONF },
#endif
#ifdef _SC_ASYNCHRONOUS_IO
    { "_POSIX_ASYNCHRONOUS_IO", _SC_ASYNCHRONOUS_IO, SYSCONF },
#endif
#ifdef _SC_CHILD_MAX
    { "_POSIX_CHILD_MAX", _SC_CHILD_MAX, SYSCONF },
#endif
#ifdef _SC_FSYNC
    { "_POSIX_FSYNC", _SC_FSYNC, SYSCONF },
#endif
#ifdef _SC_JOB_CONTROL
    { "_POSIX_JOB_CONTROL", _SC_JOB_CONTROL, SYSCONF },
#endif
#ifdef _SC_MAPPED_FILES
    { "_POSIX_MAPPED_FILES", _SC_MAPPED_FILES, SYSCONF },
#endif
#ifdef _SC_MEMLOCK
    { "_POSIX_MEMLOCK", _SC_MEMLOCK, SYSCONF },
#endif
#ifdef _SC_MEMLOCK_RANGE
    { "_POSIX_MEMLOCK_RANGE", _SC_MEMLOCK_RANGE, SYSCONF },
#endif
#ifdef _SC_MEMORY_PROTECTION
    { "_POSIX_MEMORY_PROTECTION", _SC_MEMORY_PROTECTION, SYSCONF },
#endif
#ifdef _SC_MESSAGE_PASSING
    { "_POSIX_MESSAGE_PASSING", _SC_MESSAGE_PASSING, SYSCONF },
#endif
#ifdef _SC_NGROUPS_MAX
    { "_POSIX_NGROUPS_MAX", _SC_NGROUPS_MAX, SYSCONF },
#endif
#ifdef _SC_OPEN_MAX
    { "_POSIX_OPEN_MAX", _SC_OPEN_MAX, SYSCONF },
#endif
#ifdef _SC_PII
    { "_POSIX_PII", _SC_PII, SYSCONF },
#endif
#ifdef _SC_PII_INTERNET
    { "_POSIX_PII_INTERNET", _SC_PII_INTERNET, SYSCONF },
#endif
#ifdef _SC_PII_INTERNET_DGRAM
    { "_POSIX_PII_INTERNET_DGRAM", _SC_PII_INTERNET_DGRAM, SYSCONF },
#endif
#ifdef _SC_PII_INTERNET_STREAM
    { "_POSIX_PII_INTERNET_STREAM", _SC_PII_INTERNET_STREAM, SYSCONF },
#endif
#ifdef _SC_PII_OSI
    { "_POSIX_PII_OSI", _SC_PII_OSI, SYSCONF },
#endif
#ifdef _SC_PII_OSI_CLTS
    { "_POSIX_PII_OSI_CLTS", _SC_PII_OSI_CLTS, SYSCONF },
#endif
#ifdef _SC_PII_OSI_COTS
    { "_POSIX_PII_OSI_COTS", _SC_PII_OSI_COTS, SYSCONF },
#endif
#ifdef _SC_PII_OSI_M
    { "_POSIX_PII_OSI_M", _SC_PII_OSI_M, SYSCONF },
#endif
#ifdef _SC_PII_SOCKET
    { "_POSIX_PII_SOCKET", _SC_PII_SOCKET, SYSCONF },
#endif
#ifdef _SC_PII_XTI
    { "_POSIX_PII_XTI", _SC_PII_XTI, SYSCONF },
#endif
#ifdef _SC_POLL
    { "_POSIX_POLL", _SC_POLL, SYSCONF },
#endif
#ifdef _SC_PRIORITIZED_IO
    { "_POSIX_PRIORITIZED_IO", _SC_PRIORITIZED_IO, SYSCONF },
#endif
#ifdef _SC_PRIORITY_SCHEDULING
    { "_POSIX_PRIORITY_SCHEDULING", _SC_PRIORITY_SCHEDULING, SYSCONF },
#endif
#ifdef _SC_REALTIME_SIGNALS
    { "_POSIX_REALTIME_SIGNALS", _SC_REALTIME_SIGNALS, SYSCONF },
#endif
#ifdef _SC_SAVED_IDS
    { "_POSIX_SAVED_IDS", _SC_SAVED_IDS, SYSCONF },
#endif
#ifdef _SC_SELECT
    { "_POSIX_SELECT", _SC_SELECT, SYSCONF },
#endif
#ifdef _SC_SEMAPHORES
    { "_POSIX_SEMAPHORES", _SC_SEMAPHORES, SYSCONF },
#endif
#ifdef _SC_SHARED_MEMORY_OBJECTS
    { "_POSIX_SHARED_MEMORY_OBJECTS", _SC_SHARED_MEMORY_OBJECTS, SYSCONF },
#endif
#ifdef _SC_SSIZE_MAX
    { "_POSIX_SSIZE_MAX", _SC_SSIZE_MAX, SYSCONF },
#endif
#ifdef _SC_STREAM_MAX
    { "_POSIX_STREAM_MAX", _SC_STREAM_MAX, SYSCONF },
#endif
#ifdef _SC_SYNCHRONIZED_IO
    { "_POSIX_SYNCHRONIZED_IO", _SC_SYNCHRONIZED_IO, SYSCONF },
#endif
#ifdef _SC_THREADS
    { "_POSIX_THREADS", _SC_THREADS, SYSCONF },
#endif
#ifdef _SC_THREAD_ATTR_STACKADDR
    { "_POSIX_THREAD_ATTR_STACKADDR", _SC_THREAD_ATTR_STACKADDR, SYSCONF },
#endif
#ifdef _SC_THREAD_ATTR_STACKSIZE
    { "_POSIX_THREAD_ATTR_STACKSIZE", _SC_THREAD_ATTR_STACKSIZE, SYSCONF },
#endif
#ifdef _SC_THREAD_PRIORITY_SCHEDULING
    { "_POSIX_THREAD_PRIORITY_SCHEDULING", _SC_THREAD_PRIORITY_SCHEDULING, SYSCONF },
#endif
#ifdef _SC_THREAD_PRIO_INHERIT
    { "_POSIX_THREAD_PRIO_INHERIT", _SC_THREAD_PRIO_INHERIT, SYSCONF },
#endif
#ifdef _SC_THREAD_PRIO_PROTECT
    { "_POSIX_THREAD_PRIO_PROTECT", _SC_THREAD_PRIO_PROTECT, SYSCONF },
#endif
#ifdef _SC_THREAD_PROCESS_SHARED
    { "_POSIX_THREAD_PROCESS_SHARED", _SC_THREAD_PROCESS_SHARED, SYSCONF },
#endif
#ifdef _SC_THREAD_SAFE_FUNCTIONS
    { "_POSIX_THREAD_SAFE_FUNCTIONS", _SC_THREAD_SAFE_FUNCTIONS, SYSCONF },
#endif
#ifdef _SC_TIMERS
    { "_POSIX_TIMERS", _SC_TIMERS, SYSCONF },
#endif
#ifdef _SC_TIMER_MAX
    { "TIMER_MAX", _SC_TIMER_MAX, SYSCONF },
#endif
#ifdef _SC_TZNAME_MAX
    { "_POSIX_TZNAME_MAX", _SC_TZNAME_MAX, SYSCONF },
#endif
#ifdef _SC_VERSION
    { "_POSIX_VERSION", _SC_VERSION, SYSCONF },
#endif
#ifdef _SC_T_IOV_MAX
    { "_T_IOV_MAX", _SC_T_IOV_MAX, SYSCONF },
#endif
#ifdef _SC_XOPEN_CRYPT
    { "_XOPEN_CRYPT", _SC_XOPEN_CRYPT, SYSCONF },
#endif
#ifdef _SC_XOPEN_ENH_I18N
    { "_XOPEN_ENH_I18N", _SC_XOPEN_ENH_I18N, SYSCONF },
#endif
#ifdef _SC_XOPEN_LEGACY
    { "_XOPEN_LEGACY", _SC_XOPEN_LEGACY, SYSCONF },
#endif
#ifdef _SC_XOPEN_REALTIME
    { "_XOPEN_REALTIME", _SC_XOPEN_REALTIME, SYSCONF },
#endif
#ifdef _SC_XOPEN_REALTIME_THREADS
    { "_XOPEN_REALTIME_THREADS", _SC_XOPEN_REALTIME_THREADS, SYSCONF },
#endif
#ifdef _SC_XOPEN_SHM
    { "_XOPEN_SHM", _SC_XOPEN_SHM, SYSCONF },
#endif
#ifdef _SC_XOPEN_UNIX
    { "_XOPEN_UNIX", _SC_XOPEN_UNIX, SYSCONF },
#endif
#ifdef _SC_XOPEN_VERSION
    { "_XOPEN_VERSION", _SC_XOPEN_VERSION, SYSCONF },
#endif
#ifdef _SC_XOPEN_XCU_VERSION
    { "_XOPEN_XCU_VERSION", _SC_XOPEN_XCU_VERSION, SYSCONF },
#endif
#ifdef _SC_XOPEN_XPG2
    { "_XOPEN_XPG2", _SC_XOPEN_XPG2, SYSCONF },
#endif
#ifdef _SC_XOPEN_XPG3
    { "_XOPEN_XPG3", _SC_XOPEN_XPG3, SYSCONF },
#endif
#ifdef _SC_XOPEN_XPG4
    { "_XOPEN_XPG4", _SC_XOPEN_XPG4, SYSCONF },
#endif
    /* POSIX.2  */
#ifdef _SC_BC_BASE_MAX
    { "BC_BASE_MAX", _SC_BC_BASE_MAX, SYSCONF },
#endif
#ifdef _SC_BC_DIM_MAX
    { "BC_DIM_MAX", _SC_BC_DIM_MAX, SYSCONF },
#endif
#ifdef _SC_BC_SCALE_MAX
    { "BC_SCALE_MAX", _SC_BC_SCALE_MAX, SYSCONF },
#endif
#ifdef _SC_BC_STRING_MAX
    { "BC_STRING_MAX", _SC_BC_STRING_MAX, SYSCONF },
#endif
#ifdef _SC_CHARCLASS_NAME_MAX
    { "CHARCLASS_NAME_MAX", _SC_CHARCLASS_NAME_MAX, SYSCONF },
#endif
#ifdef _SC_COLL_WEIGHTS_MAX
    { "COLL_WEIGHTS_MAX", _SC_COLL_WEIGHTS_MAX, SYSCONF },
#endif
#ifdef _SC_EQUIV_CLASS_MAX
    { "EQUIV_CLASS_MAX", _SC_EQUIV_CLASS_MAX, SYSCONF },
#endif
#ifdef _SC_EXPR_NEST_MAX
    { "EXPR_NEST_MAX", _SC_EXPR_NEST_MAX, SYSCONF },
#endif
#ifdef _SC_LINE_MAX
    { "LINE_MAX", _SC_LINE_MAX, SYSCONF },
#endif
#ifdef _SC_BC_BASE_MAX
    { "POSIX2_BC_BASE_MAX", _SC_BC_BASE_MAX, SYSCONF },
#endif
#ifdef _SC_BC_DIM_MAX
    { "POSIX2_BC_DIM_MAX", _SC_BC_DIM_MAX, SYSCONF },
#endif
#ifdef _SC_BC_SCALE_MAX
    { "POSIX2_BC_SCALE_MAX", _SC_BC_SCALE_MAX, SYSCONF },
#endif
#ifdef _SC_BC_STRING_MAX
    { "POSIX2_BC_STRING_MAX", _SC_BC_STRING_MAX, SYSCONF },
#endif
#ifdef _SC_2_CHAR_TERM
    { "POSIX2_CHAR_TERM", _SC_2_CHAR_TERM, SYSCONF },
#endif
#ifdef _SC_COLL_WEIGHTS_MAX
    { "POSIX2_COLL_WEIGHTS_MAX", _SC_COLL_WEIGHTS_MAX, SYSCONF },
#endif
#ifdef _SC_2_C_BIND
    { "POSIX2_C_BIND", _SC_2_C_BIND, SYSCONF },
#endif
#ifdef _SC_2_C_DEV
    { "POSIX2_C_DEV", _SC_2_C_DEV, SYSCONF },
#endif
#ifdef _SC_2_C_VERSION
    { "POSIX2_C_VERSION", _SC_2_C_VERSION, SYSCONF },
#endif
#ifdef _SC_EXPR_NEST_MAX
    { "POSIX2_EXPR_NEST_MAX", _SC_EXPR_NEST_MAX, SYSCONF },
#endif
#ifdef _SC_2_FORT_DEV
    { "POSIX2_FORT_DEV", _SC_2_FORT_DEV, SYSCONF },
#endif
#ifdef _SC_2_FORT_RUN
    { "POSIX2_FORT_RUN", _SC_2_FORT_RUN, SYSCONF },
#endif
#ifdef _SC_LINE_MAX
    { "_POSIX2_LINE_MAX", _SC_LINE_MAX, SYSCONF },
#endif
#ifdef _SC_2_LOCALEDEF
    { "POSIX2_LOCALEDEF", _SC_2_LOCALEDEF, SYSCONF },
#endif
#ifdef _SC_RE_DUP_MAX
    { "POSIX2_RE_DUP_MAX", _SC_RE_DUP_MAX, SYSCONF },
#endif
#ifdef _SC_2_SW_DEV
    { "POSIX2_SW_DEV", _SC_2_SW_DEV, SYSCONF },
#endif
#ifdef _SC_2_UPE
    { "POSIX2_UPE", _SC_2_UPE, SYSCONF },
#endif
#ifdef _SC_2_VERSION
    { "POSIX2_VERSION", _SC_2_VERSION, SYSCONF },
#endif
#ifdef _SC_RE_DUP_MAX
    { "RE_DUP_MAX", _SC_RE_DUP_MAX, SYSCONF },
#endif

#ifdef _CS_PATH
    { "PATH", _CS_PATH, CONFSTR },
    { "CS_PATH", _CS_PATH, CONFSTR },
#endif

    /* LFS */
#ifdef _CS_LFS_CFLAGS
    { "LFS_CFLAGS", _CS_LFS_CFLAGS, CONFSTR },
#endif
#ifdef _CS_LFS_LDFLAGS
    { "LFS_LDFLAGS", _CS_LFS_LDFLAGS, CONFSTR },
#endif
#ifdef _CS_LFS_LIBS
    { "LFS_LIBS", _CS_LFS_LIBS, CONFSTR },
#endif
#ifdef _CS_LFS_LINTFLAGS
    { "LFS_LINTFLAGS", _CS_LFS_LINTFLAGS, CONFSTR },
#endif
#ifdef _CS_LFS64_CFLAGS
    { "LFS64_CFLAGS", _CS_LFS64_CFLAGS, CONFSTR },
#endif
#ifdef _CS_LFS64_LDFLAGS
    { "LFS64_LDFLAGS", _CS_LFS64_LDFLAGS, CONFSTR },
#endif
#ifdef _CS_LFS64_LIBS
    { "LFS64_LIBS", _CS_LFS64_LIBS, CONFSTR },
#endif
#ifdef _CS_LFS64_LINTFLAGS
    { "LFS64_LINTFLAGS", _CS_LFS64_LINTFLAGS, CONFSTR },
#endif

    /* Programming environments.  */
#ifdef _SC_XBS5_ILP32_OFF32
    { "_XBS5_ILP32_OFF32", _SC_XBS5_ILP32_OFF32, SYSCONF },
#endif
#ifdef _CS_XBS5_ILP32_OFF32_CFLAGS
    { "XBS5_ILP32_OFF32_CFLAGS", _CS_XBS5_ILP32_OFF32_CFLAGS, CONFSTR },
#endif
#ifdef _CS_XBS5_ILP32_OFF32_LDFLAGS
    { "XBS5_ILP32_OFF32_LDFLAGS", _CS_XBS5_ILP32_OFF32_LDFLAGS, CONFSTR },
#endif
#ifdef _CS_XBS5_ILP32_OFF32_LIBS
    { "XBS5_ILP32_OFF32_LIBS", _CS_XBS5_ILP32_OFF32_LIBS, CONFSTR },
#endif
#ifdef _CS_XBS5_ILP32_OFF32_LINTFLAGS
    { "XBS5_ILP32_OFF32_LINTFLAGS", _CS_XBS5_ILP32_OFF32_LINTFLAGS, CONFSTR },
#endif

#ifdef _SC_XBS5_ILP32_OFFBIG
    { "_XBS5_ILP32_OFFBIG", _SC_XBS5_ILP32_OFFBIG, SYSCONF },
#endif
#ifdef _CS_XBS5_ILP32_OFFBIG_CFLAGS
    { "XBS5_ILP32_OFFBIG_CFLAGS", _CS_XBS5_ILP32_OFFBIG_CFLAGS, CONFSTR },
#endif
#ifdef _CS_XBS5_ILP32_OFFBIG_LDFLAGS
    { "XBS5_ILP32_OFFBIG_LDFLAGS", _CS_XBS5_ILP32_OFFBIG_LDFLAGS, CONFSTR },
#endif
#ifdef _CS_XBS5_ILP32_OFFBIG_LIBS
    { "XBS5_ILP32_OFFBIG_LIBS", _CS_XBS5_ILP32_OFFBIG_LIBS, CONFSTR },
#endif
#ifdef _CS_XBS5_ILP32_OFFBIG_LINTFLAGS
    { "XBS5_ILP32_OFFBIG_LINTFLAGS", _CS_XBS5_ILP32_OFFBIG_LINTFLAGS, CONFSTR },
#endif

#ifdef _SC_XBS5_LP64_OFF64
    { "_XBS5_LP64_OFF64", _SC_XBS5_LP64_OFF64, SYSCONF },
#endif
#ifdef _CS_XBS5_LP64_OFF64_CFLAGS
    { "XBS5_LP64_OFF64_CFLAGS", _CS_XBS5_LP64_OFF64_CFLAGS, CONFSTR },
#endif
#ifdef _CS_XBS5_LP64_OFF64_LDFLAGS
    { "XBS5_LP64_OFF64_LDFLAGS", _CS_XBS5_LP64_OFF64_LDFLAGS, CONFSTR },
#endif
#ifdef _CS_XBS5_LP64_OFF64_LIBS
    { "XBS5_LP64_OFF64_LIBS", _CS_XBS5_LP64_OFF64_LIBS, CONFSTR },
#endif
#ifdef _CS_XBS5_LP64_OFF64_LINTFLAGS
    { "XBS5_LP64_OFF64_LINTFLAGS", _CS_XBS5_LP64_OFF64_LINTFLAGS, CONFSTR },
#endif

#ifdef _SC_XBS5_LPBIG_OFFBIG
    { "_XBS5_LPBIG_OFFBIG", _SC_XBS5_LPBIG_OFFBIG, SYSCONF },
#endif
#ifdef _CS_XBS5_LPBIG_OFFBIG_CFLAGS
    { "XBS5_LPBIG_OFFBIG_CFLAGS", _CS_XBS5_LPBIG_OFFBIG_CFLAGS, CONFSTR },
#endif
#ifdef _CS_XBS5_LPBIG_OFFBIG_LDFLAGS
    { "XBS5_LPBIG_OFFBIG_LDFLAGS", _CS_XBS5_LPBIG_OFFBIG_LDFLAGS, CONFSTR },
#endif
#ifdef _CS_XBS5_LPBIG_OFFBIG_LIBS
    { "XBS5_LPBIG_OFFBIG_LIBS", _CS_XBS5_LPBIG_OFFBIG_LIBS, CONFSTR },
#endif
#ifdef _CS_XBS5_LPBIG_OFFBIG_LINTFLAGS
    { "XBS5_LPBIG_OFFBIG_LINTFLAGS", _CS_XBS5_LPBIG_OFFBIG_LINTFLAGS, CONFSTR },
#endif

#ifdef _SC_V6_ILP32_OFF32
    { "_POSIX_V6_ILP32_OFF32", _SC_V6_ILP32_OFF32, SYSCONF },
#endif
#ifdef _CS_POSIX_V6_ILP32_OFF32_CFLAGS
    { "POSIX_V6_ILP32_OFF32_CFLAGS", _CS_POSIX_V6_ILP32_OFF32_CFLAGS, CONFSTR },
#endif
#ifdef _CS_POSIX_V6_ILP32_OFF32_LDFLAGS
    { "POSIX_V6_ILP32_OFF32_LDFLAGS", _CS_POSIX_V6_ILP32_OFF32_LDFLAGS, CONFSTR },
#endif
#ifdef _CS_POSIX_V6_ILP32_OFF32_LIBS
    { "POSIX_V6_ILP32_OFF32_LIBS", _CS_POSIX_V6_ILP32_OFF32_LIBS, CONFSTR },
#endif
#ifdef _CS_POSIX_V6_ILP32_OFF32_LINTFLAGS
    { "POSIX_V6_ILP32_OFF32_LINTFLAGS", _CS_POSIX_V6_ILP32_OFF32_LINTFLAGS, CONFSTR },
#endif

#ifdef _CS_V6_WIDTH_RESTRICTED_ENVS
    { "_POSIX_V6_WIDTH_RESTRICTED_ENVS", _CS_V6_WIDTH_RESTRICTED_ENVS, CONFSTR },
#endif

#ifdef _SC_V6_ILP32_OFFBIG
    { "_POSIX_V6_ILP32_OFFBIG", _SC_V6_ILP32_OFFBIG, SYSCONF },
#endif
#ifdef _CS_POSIX_V6_ILP32_OFFBIG_CFLAGS
    { "POSIX_V6_ILP32_OFFBIG_CFLAGS", _CS_POSIX_V6_ILP32_OFFBIG_CFLAGS, CONFSTR },
#endif
#ifdef _CS_POSIX_V6_ILP32_OFFBIG_LDFLAGS
    { "POSIX_V6_ILP32_OFFBIG_LDFLAGS", _CS_POSIX_V6_ILP32_OFFBIG_LDFLAGS, CONFSTR },
#endif
#ifdef _CS_POSIX_V6_ILP32_OFFBIG_LIBS
    { "POSIX_V6_ILP32_OFFBIG_LIBS", _CS_POSIX_V6_ILP32_OFFBIG_LIBS, CONFSTR },
#endif
#ifdef _CS_POSIX_V6_ILP32_OFFBIG_LINTFLAGS
    { "POSIX_V6_ILP32_OFFBIG_LINTFLAGS", _CS_POSIX_V6_ILP32_OFFBIG_LINTFLAGS, CONFSTR },
#endif

#ifdef _SC_V6_LP64_OFF64
    { "_POSIX_V6_LP64_OFF64", _SC_V6_LP64_OFF64, SYSCONF },
#endif
#ifdef _CS_POSIX_V6_LP64_OFF64_CFLAGS
    { "POSIX_V6_LP64_OFF64_CFLAGS", _CS_POSIX_V6_LP64_OFF64_CFLAGS, CONFSTR },
#endif
#ifdef _CS_POSIX_V6_LP64_OFF64_LDFLAGS
    { "POSIX_V6_LP64_OFF64_LDFLAGS", _CS_POSIX_V6_LP64_OFF64_LDFLAGS, CONFSTR },
#endif
#ifdef _CS_POSIX_V6_LP64_OFF64_LIBS
    { "POSIX_V6_LP64_OFF64_LIBS", _CS_POSIX_V6_LP64_OFF64_LIBS, CONFSTR },
#endif
#ifdef _CS_POSIX_V6_LP64_OFF64_LINTFLAGS
    { "POSIX_V6_LP64_OFF64_LINTFLAGS", _CS_POSIX_V6_LP64_OFF64_LINTFLAGS, CONFSTR },
#endif

#ifdef _SC_V6_LPBIG_OFFBIG
    { "_POSIX_V6_LPBIG_OFFBIG", _SC_V6_LPBIG_OFFBIG, SYSCONF },
#endif
#ifdef _CS_POSIX_V6_LPBIG_OFFBIG_CFLAGS
    { "POSIX_V6_LPBIG_OFFBIG_CFLAGS", _CS_POSIX_V6_LPBIG_OFFBIG_CFLAGS, CONFSTR },
#endif
#ifdef _CS_POSIX_V6_LPBIG_OFFBIG_LDFLAGS
    { "POSIX_V6_LPBIG_OFFBIG_LDFLAGS", _CS_POSIX_V6_LPBIG_OFFBIG_LDFLAGS, CONFSTR },
#endif
#ifdef _CS_POSIX_V6_LPBIG_OFFBIG_LIBS
    { "POSIX_V6_LPBIG_OFFBIG_LIBS", _CS_POSIX_V6_LPBIG_OFFBIG_LIBS, CONFSTR },
#endif
#ifdef _CS_POSIX_V6_LPBIG_OFFBIG_LINTFLAGS
    { "POSIX_V6_LPBIG_OFFBIG_LINTFLAGS", _CS_POSIX_V6_LPBIG_OFFBIG_LINTFLAGS, CONFSTR },
#endif

#ifdef _SC_ADVISORY_INFO
    { "_POSIX_ADVISORY_INFO", _SC_ADVISORY_INFO, SYSCONF },
#endif
#ifdef _SC_BARRIERS
    { "_POSIX_BARRIERS", _SC_BARRIERS, SYSCONF },
#endif
#ifdef _SC_BASE
    { "_POSIX_BASE", _SC_BASE, SYSCONF },
#endif
#ifdef _SC_C_LANG_SUPPORT
    { "_POSIX_C_LANG_SUPPORT", _SC_C_LANG_SUPPORT, SYSCONF },
#endif
#ifdef _SC_C_LANG_SUPPORT_R
    { "_POSIX_C_LANG_SUPPORT_R", _SC_C_LANG_SUPPORT_R, SYSCONF },
#endif
#ifdef _SC_CLOCK_SELECTION
    { "_POSIX_CLOCK_SELECTION", _SC_CLOCK_SELECTION, SYSCONF },
#endif
#ifdef _SC_CPUTIME
    { "_POSIX_CPUTIME", _SC_CPUTIME, SYSCONF },
#endif
#ifdef _SC_THREAD_CPUTIME
    { "_POSIX_THREAD_CPUTIME", _SC_THREAD_CPUTIME, SYSCONF },
#endif
#ifdef _SC_DEVICE_SPECIFIC
    { "_POSIX_DEVICE_SPECIFIC", _SC_DEVICE_SPECIFIC, SYSCONF },
#endif
#ifdef _SC_DEVICE_SPECIFIC_R
    { "_POSIX_DEVICE_SPECIFIC_R", _SC_DEVICE_SPECIFIC_R, SYSCONF },
#endif
#ifdef _SC_FD_MGMT
    { "_POSIX_FD_MGMT", _SC_FD_MGMT, SYSCONF },
#endif
#ifdef _SC_FIFO
    { "_POSIX_FIFO", _SC_FIFO, SYSCONF },
#endif
#ifdef _SC_PIPE
    { "_POSIX_PIPE", _SC_PIPE, SYSCONF },
#endif
#ifdef _SC_FILE_ATTRIBUTES
    { "_POSIX_FILE_ATTRIBUTES", _SC_FILE_ATTRIBUTES, SYSCONF },
#endif
#ifdef _SC_FILE_LOCKING
    { "_POSIX_FILE_LOCKING", _SC_FILE_LOCKING, SYSCONF },
#endif
#ifdef _SC_FILE_SYSTEM
    { "_POSIX_FILE_SYSTEM", _SC_FILE_SYSTEM, SYSCONF },
#endif
#ifdef _SC_MONOTONIC_CLOCK
    { "_POSIX_MONOTONIC_CLOCK", _SC_MONOTONIC_CLOCK, SYSCONF },
#endif
#ifdef _SC_MULTI_PROCESS
    { "_POSIX_MULTI_PROCESS", _SC_MULTI_PROCESS, SYSCONF },
#endif
#ifdef _SC_SINGLE_PROCESS
    { "_POSIX_SINGLE_PROCESS", _SC_SINGLE_PROCESS, SYSCONF },
#endif
#ifdef _SC_NETWORKING
    { "_POSIX_NETWORKING", _SC_NETWORKING, SYSCONF },
#endif
#ifdef _SC_READER_WRITER_LOCKS
    { "_POSIX_READER_WRITER_LOCKS", _SC_READER_WRITER_LOCKS, SYSCONF },
#endif
#ifdef _SC_SPIN_LOCKS
    { "_POSIX_SPIN_LOCKS", _SC_SPIN_LOCKS, SYSCONF },
#endif
#ifdef _SC_REGEXP
    { "_POSIX_REGEXP", _SC_REGEXP, SYSCONF },
#endif
#ifdef _SC_REGEX_VERSION
    { "_REGEX_VERSION", _SC_REGEX_VERSION, SYSCONF },
#endif
#ifdef _SC_SHELL
    { "_POSIX_SHELL", _SC_SHELL, SYSCONF },
#endif
#ifdef _SC_SIGNALS
    { "_POSIX_SIGNALS", _SC_SIGNALS, SYSCONF },
#endif
#ifdef _SC_SPAWN
    { "_POSIX_SPAWN", _SC_SPAWN, SYSCONF },
#endif
#ifdef _SC_SPORADIC_SERVER
    { "_POSIX_SPORADIC_SERVER", _SC_SPORADIC_SERVER, SYSCONF },
#endif
#ifdef _SC_THREAD_SPORADIC_SERVER
    { "_POSIX_THREAD_SPORADIC_SERVER", _SC_THREAD_SPORADIC_SERVER, SYSCONF },
#endif
#ifdef _SC_SYSTEM_DATABASE
    { "_POSIX_SYSTEM_DATABASE", _SC_SYSTEM_DATABASE, SYSCONF },
#endif
#ifdef _SC_SYSTEM_DATABASE_R
    { "_POSIX_SYSTEM_DATABASE_R", _SC_SYSTEM_DATABASE_R, SYSCONF },
#endif
#ifdef _SC_TIMEOUTS
    { "_POSIX_TIMEOUTS", _SC_TIMEOUTS, SYSCONF },
#endif
#ifdef _SC_TYPED_MEMORY_OBJECTS
    { "_POSIX_TYPED_MEMORY_OBJECTS", _SC_TYPED_MEMORY_OBJECTS, SYSCONF },
#endif
#ifdef _SC_USER_GROUPS
    { "_POSIX_USER_GROUPS", _SC_USER_GROUPS, SYSCONF },
#endif
#ifdef _SC_USER_GROUPS_R
    { "_POSIX_USER_GROUPS_R", _SC_USER_GROUPS_R, SYSCONF },
#endif
#ifdef _SC_2_PBS
    { "POSIX2_PBS", _SC_2_PBS, SYSCONF },
#endif
#ifdef _SC_2_PBS_ACCOUNTING
    { "POSIX2_PBS_ACCOUNTING", _SC_2_PBS_ACCOUNTING, SYSCONF },
#endif
#ifdef _SC_2_PBS_LOCATE
    { "POSIX2_PBS_LOCATE", _SC_2_PBS_LOCATE, SYSCONF },
#endif
#ifdef _SC_2_PBS_TRACK
    { "POSIX2_PBS_TRACK", _SC_2_PBS_TRACK, SYSCONF },
#endif
#ifdef _SC_2_PBS_MESSAGE
    { "POSIX2_PBS_MESSAGE", _SC_2_PBS_MESSAGE, SYSCONF },
#endif
#ifdef _SC_SYMLOOP_MAX
    { "SYMLOOP_MAX", _SC_SYMLOOP_MAX, SYSCONF },
#endif
#ifdef _SC_STREAM_MAX
    { "STREAM_MAX", _SC_STREAM_MAX, SYSCONF },
#endif
#ifdef _SC_AIO_LISTIO_MAX
    { "AIO_LISTIO_MAX", _SC_AIO_LISTIO_MAX, SYSCONF },
#endif
#ifdef _SC_AIO_MAX
    { "AIO_MAX", _SC_AIO_MAX, SYSCONF },
#endif
#ifdef _SC_AIO_PRIO_DELTA_MAX
    { "AIO_PRIO_DELTA_MAX", _SC_AIO_PRIO_DELTA_MAX, SYSCONF },
#endif
#ifdef _SC_DELAYTIMER_MAX
    { "DELAYTIMER_MAX", _SC_DELAYTIMER_MAX, SYSCONF },
#endif
#ifdef _SC_HOST_NAME_MAX
    { "HOST_NAME_MAX", _SC_HOST_NAME_MAX, SYSCONF },
#endif
#ifdef _SC_LOGIN_NAME_MAX
    { "LOGIN_NAME_MAX", _SC_LOGIN_NAME_MAX, SYSCONF },
#endif
#ifdef _SC_MQ_OPEN_MAX
    { "MQ_OPEN_MAX", _SC_MQ_OPEN_MAX, SYSCONF },
#endif
#ifdef _SC_MQ_PRIO_MAX
    { "MQ_PRIO_MAX", _SC_MQ_PRIO_MAX, SYSCONF },
#endif
#ifdef _SC_DEVICE_IO
    { "_POSIX_DEVICE_IO", _SC_DEVICE_IO, SYSCONF },
#endif
#ifdef _SC_TRACE
    { "_POSIX_TRACE", _SC_TRACE, SYSCONF },
#endif
#ifdef _SC_TRACE_EVENT_FILTER
    { "_POSIX_TRACE_EVENT_FILTER", _SC_TRACE_EVENT_FILTER, SYSCONF },
#endif
#ifdef _SC_TRACE_INHERIT
    { "_POSIX_TRACE_INHERIT", _SC_TRACE_INHERIT, SYSCONF },
#endif
#ifdef _SC_TRACE_LOG
    { "_POSIX_TRACE_LOG", _SC_TRACE_LOG, SYSCONF },
#endif
#ifdef _SC_RTSIG_MAX
    { "RTSIG_MAX", _SC_RTSIG_MAX, SYSCONF },
#endif
#ifdef _SC_SEM_NSEMS_MAX
    { "SEM_NSEMS_MAX", _SC_SEM_NSEMS_MAX, SYSCONF },
#endif
#ifdef _SC_SEM_VALUE_MAX
    { "SEM_VALUE_MAX", _SC_SEM_VALUE_MAX, SYSCONF },
#endif
#ifdef _SC_SIGQUEUE_MAX
    { "SIGQUEUE_MAX", _SC_SIGQUEUE_MAX, SYSCONF },
#endif
#ifdef _PC_FILESIZEBITS
    { "FILESIZEBITS", _PC_FILESIZEBITS, PATHCONF },
#endif
#ifdef _PC_ALLOC_SIZE_MIN
    { "POSIX_ALLOC_SIZE_MIN", _PC_ALLOC_SIZE_MIN, PATHCONF },
#endif
#ifdef _PC_REC_INCR_XFER_SIZE
    { "POSIX_REC_INCR_XFER_SIZE", _PC_REC_INCR_XFER_SIZE, PATHCONF },
#endif
#ifdef _PC_REC_MAX_XFER_SIZE
    { "POSIX_REC_MAX_XFER_SIZE", _PC_REC_MAX_XFER_SIZE, PATHCONF },
#endif
#ifdef _PC_REC_MIN_XFER_SIZE
    { "POSIX_REC_MIN_XFER_SIZE", _PC_REC_MIN_XFER_SIZE, PATHCONF },
#endif
#ifdef _PC_REC_XFER_ALIGN
    { "POSIX_REC_XFER_ALIGN", _PC_REC_XFER_ALIGN, PATHCONF },
#endif
#ifdef _PC_SYMLINK_MAX
    { "SYMLINK_MAX", _PC_SYMLINK_MAX, PATHCONF },
#endif
#ifdef _CS_GNU_LIBC_VERSION
    { "GNU_LIBC_VERSION", _CS_GNU_LIBC_VERSION, CONFSTR },
#endif
#ifdef _CS_GNU_LIBPTHREAD_VERSION
    { "GNU_LIBPTHREAD_VERSION", _CS_GNU_LIBPTHREAD_VERSION, CONFSTR },
#endif
#ifdef _PC_2_SYMLINKS
    { "POSIX2_SYMLINKS", _PC_2_SYMLINKS, PATHCONF },
#endif

#ifdef _SC_LEVEL1_ICACHE_SIZE
    { "LEVEL1_ICACHE_SIZE", _SC_LEVEL1_ICACHE_SIZE, SYSCONF },
#endif
#ifdef _SC_LEVEL1_ICACHE_ASSOC
    { "LEVEL1_ICACHE_ASSOC", _SC_LEVEL1_ICACHE_ASSOC, SYSCONF },
#endif
#ifdef _SC_LEVEL1_ICACHE_LINESIZE
    { "LEVEL1_ICACHE_LINESIZE", _SC_LEVEL1_ICACHE_LINESIZE, SYSCONF },
#endif
#ifdef _SC_LEVEL1_DCACHE_SIZE
    { "LEVEL1_DCACHE_SIZE", _SC_LEVEL1_DCACHE_SIZE, SYSCONF },
#endif
#ifdef _SC_LEVEL1_DCACHE_ASSOC
    { "LEVEL1_DCACHE_ASSOC", _SC_LEVEL1_DCACHE_ASSOC, SYSCONF },
#endif
#ifdef _SC_LEVEL1_DCACHE_LINESIZE
    { "LEVEL1_DCACHE_LINESIZE", _SC_LEVEL1_DCACHE_LINESIZE, SYSCONF },
#endif
#ifdef _SC_LEVEL2_CACHE_SIZE
    { "LEVEL2_CACHE_SIZE", _SC_LEVEL2_CACHE_SIZE, SYSCONF },
#endif
#ifdef _SC_LEVEL2_CACHE_ASSOC
    { "LEVEL2_CACHE_ASSOC", _SC_LEVEL2_CACHE_ASSOC, SYSCONF },
#endif
#ifdef _SC_LEVEL2_CACHE_LINESIZE
    { "LEVEL2_CACHE_LINESIZE", _SC_LEVEL2_CACHE_LINESIZE, SYSCONF },
#endif
#ifdef _SC_LEVEL3_CACHE_SIZE
    { "LEVEL3_CACHE_SIZE", _SC_LEVEL3_CACHE_SIZE, SYSCONF },
#endif
#ifdef _SC_LEVEL3_CACHE_ASSOC
    { "LEVEL3_CACHE_ASSOC", _SC_LEVEL3_CACHE_ASSOC, SYSCONF },
#endif
#ifdef _SC_LEVEL3_CACHE_LINESIZE
    { "LEVEL3_CACHE_LINESIZE", _SC_LEVEL3_CACHE_LINESIZE, SYSCONF },
#endif
#ifdef _SC_LEVEL4_CACHE_SIZE
    { "LEVEL4_CACHE_SIZE", _SC_LEVEL4_CACHE_SIZE, SYSCONF },
#endif
#ifdef _SC_LEVEL4_CACHE_ASSOC
    { "LEVEL4_CACHE_ASSOC", _SC_LEVEL4_CACHE_ASSOC, SYSCONF },
#endif

#ifdef _SC_IPV6
    { "IPV6", _SC_IPV6, SYSCONF },
#endif
#ifdef _SC_RAW_SOCKETS
    { "RAW_SOCKETS", _SC_RAW_SOCKETS, SYSCONF },
#endif

    { NULL, 0, SYSCONF }
};

#define	_GETCONF_PATH	"/"
/*@unchecked@*/ /*@observer@*/ /*@owned@*/ /*@relnull@*/
static const char *_getconf_path = NULL;

int
rpmdsGetconf(rpmds * dsp, const char *path)
	/*@globals _getconf_path @*/
	/*@modifies _getconf_path @*/
{
    const struct _conf_s *c;
    size_t clen;
    long int value;
    const char * NS = "getconf";
    const char *N;
    char * EVR;
    char * t;
    evrFlags Flags;

/*@-modobserver@*/
    if (_getconf_path == NULL) {
	_getconf_path = rpmExpand("%{?_rpmds__getconf_path}", NULL);
	/* XXX may need to validate path existence somewhen. */
	if (!(_getconf_path != NULL && *_getconf_path == '/')) {
/*@-observertrans @*/
	    _getconf_path = _free(_getconf_path);
/*@=observertrans @*/
	    _getconf_path = xstrdup(_GETCONF_PATH);
	}
    }
/*@=modobserver@*/

    if (path == NULL)
	path = _getconf_path;

    for (c = vars; c->name != NULL; ++c) {
	N = c->name;
	EVR = NULL;
	switch (c->call) {
	case PATHCONF:
	    value = pathconf(path, c->call_name);
	    if (value != -1) {
		EVR = (char *) xmalloc(32);
		sprintf(EVR, "%ld", value);
	    }
	    /*@switchbreak@*/ break;
	case SYSCONF:
	    value = sysconf(c->call_name);
	    if (value == -1l) {
#if defined(_SC_UINT_MAX) && defined(_SC_ULONG_MAX)
/*@-unrecog@*/
		if (c->call_name == _SC_UINT_MAX
		|| c->call_name == _SC_ULONG_MAX) {
		    EVR = (char *) xmalloc(32);
		    sprintf(EVR, "%lu", value);
		}
/*@=unrecog@*/
#endif
	    } else {
		EVR = (char *) xmalloc(32);
		sprintf(EVR, "%ld", value);
	    }
	    /*@switchbreak@*/ break;
	case CONFSTR:
#ifndef __CYGWIN__
	    clen = confstr(c->call_name, (char *) NULL, 0);
	    EVR = (char *) xmalloc(clen+1);
	    *EVR = '\0';
	    if (confstr (c->call_name, EVR, clen) != clen) {
		fprintf(stderr, "confstr: %s\n", strerror(errno));
		exit (EXIT_FAILURE);
	    }
	    EVR[clen] = '\0';
#endif
	    /*@switchbreak@*/ break;
	}
	if (EVR == NULL)
	    continue;

	for (t = EVR; *t; t++) {
	    if (*t == '\n') *t = ' ';
	}
	if (!strcmp(N, "GNU_LIBC_VERSION")
	 || !strcmp(N, "GNU_LIBPTHREAD_VERSION"))
	{
	    for (t = EVR; *t; t++) {
		if (*t == ' ') *t = '-';
	    }
	}

	if (*EVR == '\0' || strchr(EVR, ' ') != NULL
	 || (EVR[0] == '-' && strchr("0123456789", EVR[1]) == NULL))
	{
	    EVR = _free(EVR);
	    continue;
	}

	Flags = (evrFlags) (RPMSENSE_PROBE|RPMSENSE_EQUAL);
	rpmdsNSAdd(dsp, NS, N, EVR, Flags);
	EVR = _free(EVR);
    }
    return 0;
}

int rpmdsMergePRCO(void * context, rpmds ds)
{
    rpmPRCO PRCO = (rpmPRCO) context;
    int rc = -1;

/*@-modfilesys@*/
if (_rpmds_debug < 0)
fprintf(stderr, "*** rpmdsMergePRCO(%p, %p) %s\n", context, ds, rpmdsTagName(rpmdsTagN(ds)));
/*@=modfilesys@*/
    switch(rpmdsTagN(ds)) {
    default:
	break;
    case RPMTAG_PROVIDENAME:
	rc = rpmdsMerge(PRCO->Pdsp, ds);
	break;
    case RPMTAG_REQUIRENAME:
	rc = rpmdsMerge(PRCO->Rdsp, ds);
	break;
    case RPMTAG_CONFLICTNAME:
	rc = rpmdsMerge(PRCO->Cdsp, ds);
	break;
    case RPMTAG_OBSOLETENAME:
	rc = rpmdsMerge(PRCO->Odsp, ds);
	break;
    case RPMTAG_TRIGGERNAME:
	rc = rpmdsMerge(PRCO->Tdsp, ds);
	break;
    case RPMTAG_DIRNAMES:
	rc = rpmdsMerge(PRCO->Ddsp, ds);
	break;
    case RPMTAG_FILELINKTOS:
	rc = rpmdsMerge(PRCO->Ldsp, ds);
	break;
    }
    return rc;
}

rpmPRCO rpmdsFreePRCO(rpmPRCO PRCO)
{
    if (PRCO) {
	(void)rpmdsFree(PRCO->my);
	PRCO->my = NULL;
	(void)rpmdsFree(PRCO->P);
	PRCO->P = NULL;
	(void)rpmdsFree(PRCO->R);
	PRCO->R = NULL;
	(void)rpmdsFree(PRCO->C);
	PRCO->C = NULL;
	(void)rpmdsFree(PRCO->O);
	PRCO->O = NULL;
	(void)rpmdsFree(PRCO->T);
	PRCO->T = NULL;
	(void)rpmdsFree(PRCO->D);
	PRCO->D = NULL;
	(void)rpmdsFree(PRCO->L);
	PRCO->L = NULL;
	memset(PRCO, 0, sizeof(*PRCO));
	PRCO = _free(PRCO);
    }
    return NULL;
}

rpmPRCO rpmdsNewPRCO(Header h)
{
    rpmPRCO PRCO = (rpmPRCO) xcalloc(1, sizeof(*PRCO));

    if (h != NULL) {
	static int scareMem = 0;
	PRCO->my = rpmdsNew(h, RPMTAG_NAME, scareMem);
	PRCO->P = rpmdsNew(h, RPMTAG_PROVIDENAME, scareMem);
	PRCO->R = rpmdsNew(h, RPMTAG_REQUIRENAME, scareMem);
	PRCO->C = rpmdsNew(h, RPMTAG_CONFLICTNAME, scareMem);
	PRCO->O = rpmdsNew(h, RPMTAG_OBSOLETENAME, scareMem);
	PRCO->T = rpmdsNew(h, RPMTAG_TRIGGERNAME, scareMem);
	PRCO->D = rpmdsNew(h, RPMTAG_DIRNAMES, scareMem);
	PRCO->L = rpmdsNew(h, RPMTAG_FILELINKTOS, scareMem);
    }
    PRCO->Pdsp =  &PRCO->P;
    PRCO->Rdsp =  &PRCO->R;
    PRCO->Cdsp =  &PRCO->C;
    PRCO->Odsp =  &PRCO->O;
    PRCO->Tdsp =  &PRCO->T;
    PRCO->Ddsp =  &PRCO->D;
    PRCO->Ldsp =  &PRCO->L;
    return PRCO;
}

rpmds rpmdsFromPRCO(rpmPRCO PRCO, rpmTag tagN)
{
    /*@-compdef -refcounttrans -retalias -retexpose -usereleased @*/
    if (PRCO != NULL)
    switch (tagN) {
    default:	break;
    case RPMTAG_NAME:		return PRCO->my;	/*@notreached@*/ break;
    case RPMTAG_PROVIDENAME:	return *PRCO->Pdsp;	/*@notreached@*/ break;
    case RPMTAG_REQUIRENAME:	return *PRCO->Rdsp;	/*@notreached@*/ break;
    case RPMTAG_CONFLICTNAME:	return *PRCO->Cdsp;	/*@notreached@*/ break;
    case RPMTAG_OBSOLETENAME:	return *PRCO->Odsp;	/*@notreached@*/ break;
    case RPMTAG_TRIGGERNAME:	return *PRCO->Tdsp;	/*@notreached@*/ break;
    case RPMTAG_DIRNAMES:	return *PRCO->Ddsp;	/*@notreached@*/ break;
    case RPMTAG_FILELINKTOS:	return *PRCO->Ldsp;	/*@notreached@*/ break;
    }
    return NULL;
    /*@=compdef =refcounttrans =retalias =retexpose =usereleased @*/
}

/**
 * Return a soname dependency constructed from an elf string.
 * @retval t		soname dependency
 * @param s		elf string (NULL uses "")
 * @param isElf64	is this an ELF64 symbol?
 */
#if defined(HAVE_GELF_H) && defined(HAVE_LIBELF) && !defined(__FreeBSD__)
static char * sonameDep(/*@returned@*/ char * t, const char * s, int isElf64)
	/*@modifies t @*/
{
    *t = '\0';
#if !defined(__alpha__) && !defined(__sun)
    if (isElf64) {
	if (s[strlen(s)-1] != ')')
	(void) stpcpy( stpcpy(t, s), "()(64bit)");
    else
	    (void) stpcpy( stpcpy(t, s), "(64bit)");
    }else
#endif
	(void) stpcpy(t, s);
    return t;
}
#endif

/*@-moduncon -noeffectuncon @*/
int rpmdsELF(const char * fn, int flags,
		int (*add) (void * context, rpmds ds), void * context)
{
#if defined(HAVE_GELF_H) && defined(HAVE_LIBELF) && !defined(__FreeBSD__)
    Elf * elf;
    Elf_Scn * scn;
    Elf_Data * data;
    GElf_Ehdr ehdr_mem, * ehdr;
    GElf_Shdr shdr_mem, * shdr;
    GElf_Verdef def_mem, * def;
    GElf_Verneed need_mem, * need;
    GElf_Dyn dyn_mem, * dyn;
    unsigned int auxoffset;
    unsigned int offset;
    int fdno;
    int cnt2;
    int cnt;
    char buf[BUFSIZ];
    const char * s;
    int is_executable;
    const char * soname = NULL;
    rpmds ds;
    char * t;
    int xx;
    int isElf64;
    int isDSO;
    int gotSONAME = 0;
    int gotDEBUG = 0;
    int gotHASH = 0;
    int gotGNUHASH = 0;
    int skipP = (flags & RPMELF_FLAG_SKIPPROVIDES);
    int skipR = (flags & RPMELF_FLAG_SKIPREQUIRES);
    static int filter_GLIBC_PRIVATE = 0;
    static int oneshot = 0;

/*@-castfcnptr@*/
if (_rpmds_debug < 0)
fprintf(stderr, "*** rpmdsELF(%s, %d, %p, %p)\n", fn, flags, (void *)add, context);
/*@=castfcnptr@*/
    if (oneshot == 0) {
	oneshot = 1;
	filter_GLIBC_PRIVATE = rpmExpandNumeric("%{?_filter_GLIBC_PRIVATE}");
    }

    /* Extract dependencies only from files with executable bit set. */
    {	struct stat sb, * st = &sb;
	if (stat(fn, st) != 0)
	    return -1;
	is_executable = (int)(st->st_mode & (S_IXUSR|S_IXGRP|S_IXOTH));
    }

    fdno = open(fn, O_RDONLY);
    if (fdno < 0)
	return fdno;

    (void) elf_version(EV_CURRENT);

/*@-evalorder@*/
    elf = NULL;
    if ((elf = elf_begin (fdno, ELF_C_READ, NULL)) == NULL
     || elf_kind(elf) != ELF_K_ELF
     || (ehdr = gelf_getehdr(elf, &ehdr_mem)) == NULL
     || !(ehdr->e_type == ET_DYN || ehdr->e_type == ET_EXEC))
	goto exit;
/*@=evalorder@*/

    isElf64 = ehdr->e_ident[EI_CLASS] == ELFCLASS64;
    isDSO = ehdr->e_type == ET_DYN;

    /*@-uniondef @*/
    scn = NULL;
    while ((scn = elf_nextscn(elf, scn)) != NULL) {
	shdr = gelf_getshdr(scn, &shdr_mem);
	if (shdr == NULL)
	    break;

	soname = _free(soname);
	switch (shdr->sh_type) {
	default:
	    continue;
	    /*@notreached@*/ /*@switchbreak@*/ break;
	case SHT_NOTE:
#if defined(HAVE_GELF_GETNOTE)	/* XXX OpenIndiana & older elfutils haven't. */
	    if (!(shdr->sh_flags & SHF_ALLOC))
		continue;
	    data = NULL;
	    while ((data = elf_getdata(scn, data)) != NULL) {
		GElf_Nhdr nhdr;
		size_t name_offset;
		size_t desc_offset;
		offset = 0;
		while (offset < data->d_size
		 && (offset = gelf_getnote(data, offset,
			&nhdr, &name_offset, &desc_offset)) > 0)
		{
		    const char *name = ((char *)data->d_buf) + name_offset;
		    const char *desc = ((char *)data->d_buf) + desc_offset;
		    if (memchr(name, '\0', nhdr.n_namesz) == NULL)
			/*@innercontinue@*/ continue;
		    switch (nhdr.n_type) {
		    default:	/*@innercontinue@*/ continue;
#if !defined(NT_GNU_BUILD_ID)
#define	NT_GNU_BUILD_ID	3
#endif
		    case NT_GNU_BUILD_ID:
			if (strcmp(name, "GNU") == 0 && nhdr.n_descsz > 0) {  
			    static const char hex[] = "0123456789abcdef";
			    static evrFlags _Flags = (evrFlags)
				(RPMSENSE_EQUAL|RPMSENSE_FIND_PROVIDES);
			    size_t i;
			    buf[0] = '\0';
			    t = buf;
			    for (i = 0; i < nhdr.n_descsz; ++i) {
				*t++ = hex[ (((unsigned)desc[i] >> 4) & 0x0f) ];
				*t++ = hex[ (((unsigned)desc[i]     ) & 0x0f) ];
			    }
			    *t = '\0';
			    /* Add next buildid. */
			    ds = rpmdsSingle(RPMTAG_PROVIDES, "elf(buildid)",
					buf, _Flags);
			    xx = add(context, ds);
			    (void)rpmdsFree(ds);
			    ds = NULL;
			}
			/*@switchbreak@*/ break;
		    }
		}
	    }
#endif	/* defined(HAVE_GELF_GETNOTE) */
	    /*@switchbreak@*/ break;
	case SHT_GNU_verdef:
	    data = NULL;
	    if (!skipP)
	    while ((data = elf_getdata (scn, data)) != NULL) {
		offset = 0;
		for (cnt = (int)shdr->sh_info; --cnt >= 0; ) {
		
		    def = gelf_getverdef (data, offset, &def_mem);
		    if (def == NULL)
			/*@innerbreak@*/ break;
		    auxoffset = (unsigned)(offset + def->vd_aux);
		    for (cnt2 = (int)def->vd_cnt; --cnt2 >= 0; ) {
			GElf_Verdaux aux_mem, * aux;

			aux = gelf_getverdaux (data, auxoffset, &aux_mem);
			if (aux == NULL)
			    /*@innerbreak@*/ break;

			s = elf_strptr(elf, shdr->sh_link, aux->vda_name);
			if (s == NULL)
			    /*@innerbreak@*/ break;

			if (def->vd_flags & VER_FLG_BASE) {
			    soname = _free(soname);
			    soname = xstrdup(s);
			} else
			if (soname != NULL
			 && !(filter_GLIBC_PRIVATE != 0
				&& !strcmp(s, "GLIBC_PRIVATE")))
			{
			    buf[0] = '\0';
			    t = buf;
			    t = stpcpy( stpcpy( stpcpy( stpcpy(t, soname), "("), s), ")");

			    t++;	/* XXX "foo(bar)" already in buf. */

			    /* Add next provide dependency. */
			    ds = rpmdsSingle(RPMTAG_PROVIDES,
					sonameDep(t, buf, isElf64),
					"", RPMSENSE_FIND_PROVIDES);
			    xx = add(context, ds);
			    (void)rpmdsFree(ds);
			    ds = NULL;
			}
			auxoffset += aux->vda_next;
		    }
		    offset += def->vd_next;
		}
	    }
	    /*@switchbreak@*/ break;
	case SHT_GNU_verneed:
	    data = NULL;
	    /* Only from files with executable bit set. */
	    if (!skipR && is_executable)
	    while ((data = elf_getdata (scn, data)) != NULL) {
		offset = 0;
		for (cnt = (int)shdr->sh_info; --cnt >= 0; ) {
		    need = gelf_getverneed (data, offset, &need_mem);
		    if (need == NULL)
			/*@innerbreak@*/ break;

		    s = elf_strptr(elf, shdr->sh_link, need->vn_file);
		    if (s == NULL)
			/*@innerbreak@*/ break;
		    soname = _free(soname);
		    soname = xstrdup(s);
		    auxoffset = (unsigned)(offset + need->vn_aux);
		    for (cnt2 = (int)need->vn_cnt; --cnt2 >= 0; ) {
			GElf_Vernaux aux_mem, * aux;

			aux = gelf_getvernaux (data, auxoffset, &aux_mem);
			if (aux == NULL)
			    /*@innerbreak@*/ break;

			s = elf_strptr(elf, shdr->sh_link, aux->vna_name);
			if (s == NULL)
			    /*@innerbreak@*/ break;

			/* Filter dependencies that contain GLIBC_PRIVATE */
			if (soname != NULL
			 && !(filter_GLIBC_PRIVATE != 0
				&& !strcmp(s, "GLIBC_PRIVATE")))
			{
			    buf[0] = '\0';
			    t = buf;
			    t = stpcpy( stpcpy( stpcpy( stpcpy(t, soname), "("), s), ")");

			    t++;	/* XXX "foo(bar)" already in buf. */

			    /* Add next require dependency. */
			    ds = rpmdsSingle(RPMTAG_REQUIRENAME,
					sonameDep(t, buf, isElf64),
					"", RPMSENSE_FIND_REQUIRES);
			    xx = add(context, ds);
			    (void)rpmdsFree(ds);
			    ds = NULL;
			}
			auxoffset += aux->vna_next;
		    }
		    offset += need->vn_next;
		}
	    }
	    /*@switchbreak@*/ break;
	case SHT_DYNAMIC:
	    data = NULL;
	    while ((data = elf_getdata (scn, data)) != NULL) {
		for (cnt = 0; cnt < (int)(shdr->sh_size / shdr->sh_entsize); ++cnt) {
		    dyn = gelf_getdyn (data, cnt, &dyn_mem);
		    if (dyn == NULL)
			/*@innerbreak@*/ break;
		    s = NULL;
		    switch (dyn->d_tag) {
		    default:
			/*@innercontinue@*/ continue;
			/*@notreached@*/ /*@switchbreak@*/ break;
		    case DT_HASH:
			gotHASH= 1;
			/*@innercontinue@*/ continue;
		    case DT_GNU_HASH:
			gotGNUHASH= 1;
			/*@innercontinue@*/ continue;
                    case DT_DEBUG:    
			gotDEBUG = 1;
			/*@innercontinue@*/ continue;
		    case DT_NEEDED:
			/* Only from files with executable bit set. */
			if (skipR || !is_executable)
			    /*@innercontinue@*/ continue;
			/* Add next require dependency. */
			s = elf_strptr(elf, shdr->sh_link, dyn->d_un.d_val);
assert(s != NULL);
			buf[0] = '\0';
			ds = rpmdsSingle(RPMTAG_REQUIRENAME,
				sonameDep(buf, s, isElf64),
				"", RPMSENSE_FIND_REQUIRES);
			xx = add(context, ds);
			(void)rpmdsFree(ds);
			ds = NULL;
			/*@switchbreak@*/ break;
		    case DT_SONAME:
			gotSONAME = 1;
			if (skipP)
			    /*@innercontinue@*/ continue;
			s = elf_strptr(elf, shdr->sh_link, dyn->d_un.d_val);
assert(s != NULL);
			/* Add next provide dependency. */
			buf[0] = '\0';
			ds = rpmdsSingle(RPMTAG_PROVIDENAME,
				sonameDep(buf, s, isElf64),
				"", RPMSENSE_FIND_PROVIDES);
			xx = add(context, ds);
			(void)rpmdsFree(ds);
			ds = NULL;
			/*@switchbreak@*/ break;
		    }
		}
	    }
	    /*@switchbreak@*/ break;
	}
    }
    /*@=uniondef @*/

    /* For DSOs which use the .gnu_hash section and don't have a .hash
     * section, we need to ensure that we have a new enough glibc. */
    if (gotGNUHASH && !gotHASH) {
	ds = rpmdsSingle(RPMTAG_REQUIRENAME, "rtld(GNU_HASH)", "",
			RPMSENSE_FIND_REQUIRES);
	xx = add(context, ds);
	(void)rpmdsFree(ds);
	ds = NULL;
    }

    /* For DSO's, provide the basename of the file if DT_SONAME not found. */
    if (!skipP && isDSO && !gotDEBUG && !gotSONAME) {
	s = strrchr(fn, '/');
	if (s != NULL)
	    s++;
	else
	    s = fn;
assert(s != NULL);

	/* Add next provide dependency. */
	buf[0] = '\0';
	ds = rpmdsSingle(RPMTAG_PROVIDENAME,
		sonameDep(buf, s, isElf64), "", RPMSENSE_FIND_PROVIDES);
	xx = add(context, ds);
	(void)rpmdsFree(ds);
	ds = NULL;
    }

exit:
    soname = _free(soname);
    if (elf) (void) elf_end(elf);
    if (fdno >= 0)
	xx = close(fdno);
    return 0;
#else
    return -1;
#endif
}
/*@=moduncon =noeffectuncon @*/


#if defined(RPM_VENDOR_MANDRIVA)
/**
 * Return a soname dependency constructed from an elf string, Mandriva-style.
 * @retval t		soname dependency
 * @param s		elf string (NULL uses "")
 * @param isElf64	is this an ELF64 symbol?
 */
#if defined(HAVE_GELF_H) && defined(HAVE_LIBELF) && !defined(__FreeBSD__)
static char * mdvSonameDep(/*@returned@*/ char * t, const char * s, int isElf64, int devel)
	/*@modifies t @*/
{
    char *tmp = t;
    *t = '\0';
    if (devel) {
	tmp = stpcpy(t, "devel(");
    }
#if !defined(__alpha__) && !defined(__sun)
    if (!isElf64) {
	/* XXX: eehhk, would've been nice with consistency, mandriva legacy... :| */
	if (!devel && s[strlen(s)-1] != ')')
	(void) stpcpy( stpcpy(tmp, s), "()(64bit)");
    else {
	    tmp = stpcpy(tmp, s);
	    if (devel)
		tmp = strstr(t, ".so");
	    tmp = stpcpy(tmp, "(64bit)");
        }
    }else
#endif
	tmp = stpcpy(tmp, s);
    if (devel) {
	char *suffix = strstr(t, ".so");
	if (suffix)
	    tmp = suffix;
	tmp = stpcpy(tmp, ")");
    }

    return t;
}
#endif

/** \ingroup rpmds
 * Extract dependencies from a symlink.
 * XXX Prototype added to keep GCC quiet and avoid adding a symbol.
 * @param fn		file name
 * @param flags		1: skip provides 2: skip requires
 * @param *add		add(arg, ds) saves next provide/require symlink dependency.
 * @param context	add() callback context
 * @return		0 on success
 */
int rpmdsSymlink(const char * fn, int flags,
		int (*add) (void * context, rpmds ds), void * context)
	/*@globals rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
	/*@modifies rpmGlobalMacroContext, fileSystem, internalState @*/;
int rpmdsSymlink(const char * fn, int flags,
		int (*add) (void * context, rpmds ds), void * context)
{
#if defined(HAVE_GELF_H) && defined(HAVE_LIBELF) && !defined(__FreeBSD__)
    Elf * elf;
    Elf_Scn * scn;
    Elf_Data * data;
    GElf_Ehdr ehdr_mem, * ehdr;
    GElf_Shdr shdr_mem, * shdr;
    GElf_Dyn dyn_mem, * dyn;
    int fdno;
    int cnt;
    int i;
    char buf[BUFSIZ];
    const char * s;
    int is_executable;
    const char * soname = NULL;
    rpmds ds;
    int xx;
    int isElf64;
    int gotSONAME = 0;
    int skipP = (flags & RPMELF_FLAG_SKIPPROVIDES);
    int skipR = (flags & RPMELF_FLAG_SKIPREQUIRES);
    int lnklen;
    char path[MAXPATHLEN];
    /*
     * We filter out these as they come with glibc, making dependencies on
     * them rather redundant.
     */
    const char *filterRequires[] = {"ld-linux", "ld64-linux" "libBrokenLocale.so",
	"libanl.so", "libc.so", "libcidn.so", "libcrypt.so", "libdl.so", "libm.so",
	"libnsl.so", "libnss_compat.so", "libnss_dns.so", "libnss_files.so",
	"libnss_hesiod.so", "libnss_nis.so", "libnss_nisplus.so", "libpthread.so",
	"libresolv.so", "librt.so", "libutil.so", "libthread_db.so"};
    ARGV_t deps = NULL;

    /* Filename must end with ".so" to be devel(...) dependency. */
    if ((s = strrchr(fn, '.')) && strcmp(s, ".so"))
	return 0;

    if ((lnklen = readlink(fn, path, MAXPATHLEN - 1)) == -1) {
	warn("%s", fn);
	return -1;
    }
    path[lnklen] = '\0';

/*@-castfcnptr@*/
if (_rpmds_debug < 0)
fprintf(stderr, "*** rpmdsELF(%s, %d, %p, %p)\n", fn, flags, (void *)add, context);
/*@=castfcnptr@*/

    /* Extract dependencies only from files with executable bit set. */
    {	struct stat sb, * st = &sb;
	if (lstat(fn, st) != 0)
	    return -1;
	is_executable = (int)(st->st_mode & (S_IXUSR|S_IXGRP|S_IXOTH));
    }

    fdno = open(fn, O_RDONLY);
    if (fdno < 0)
	return fdno;

    (void) elf_version(EV_CURRENT);

/*@-evalorder@*/
    elf = NULL;
    if ((elf = elf_begin (fdno, ELF_C_READ, NULL)) == NULL
     || elf_kind(elf) != ELF_K_ELF
     || (ehdr = gelf_getehdr(elf, &ehdr_mem)) == NULL
     || !(ehdr->e_type == ET_DYN || ehdr->e_type == ET_EXEC))
	goto exit;
/*@=evalorder@*/

    isElf64 = ehdr->e_ident[EI_CLASS] == ELFCLASS64;

    /*@-uniondef @*/
    scn = NULL;
    while ((scn = elf_nextscn(elf, scn)) != NULL) {
	shdr = gelf_getshdr(scn, &shdr_mem);
	if (shdr == NULL)
	    break;

	soname = _free(soname);
	switch (shdr->sh_type) {
	default:
	    continue;
	    /*@notreached@*/ /*@switchbreak@*/ break;
	case SHT_DYNAMIC:
	    data = NULL;
	    while ((data = elf_getdata (scn, data)) != NULL) {
		for (cnt = 0; cnt < (int)(shdr->sh_size / shdr->sh_entsize); ++cnt) {
		    dyn = gelf_getdyn (data, cnt, &dyn_mem);
		    if (dyn == NULL)
			/*@innerbreak@*/ break;
		    s = NULL;
		    switch (dyn->d_tag) {
		    default:
			/*@innercontinue@*/ continue;
			/*@notreached@*/ /*@switchbreak@*/ break;
		    case DT_NEEDED:
			/* Only from files with executable bit set. */
			if (skipR || !is_executable)
			    /*@innercontinue@*/ continue;
			/* Add next require dependency. */
			s = elf_strptr(elf, shdr->sh_link, dyn->d_un.d_val);
assert(s != NULL);
			buf[0] = '\0';

			for (i = 0; i < (int)(sizeof(filterRequires)/sizeof(filterRequires[0])); i++)
			    if (!strncmp(s, filterRequires[i], strlen(filterRequires[i])))
				break;

			if (sizeof(filterRequires)/sizeof(filterRequires[0]) == i)
			    argvAdd(&deps, s);
			/*@switchbreak@*/ break;
		    case DT_SONAME:
			gotSONAME = 1;
			s = elf_strptr(elf, shdr->sh_link, dyn->d_un.d_val);
assert(s != NULL);
			/* Add next provide dependency. */
			buf[0] = '\0';

			if (!skipP) {
			    ds = rpmdsSingle(RPMTAG_PROVIDENAME,
				    mdvSonameDep(buf, s, isElf64, 1),
				    "", RPMSENSE_FIND_PROVIDES);
			    xx = add(context, ds);
			    (void)rpmdsFree(ds);
			    ds = NULL;
			}
			/*@switchbreak@*/ break;
		    }
		}
	    }
	    /*@switchbreak@*/ break;
	}
    }
    /*@=uniondef @*/

exit:
    if (gotSONAME && !skipR)
	for (i = 0, cnt = argvCount(deps); i < cnt; i++) {
	    ds = rpmdsSingle(RPMTAG_REQUIRENAME,
		    mdvSonameDep(buf, deps[i], isElf64, 1),
		    "", RPMSENSE_FIND_REQUIRES);
	    xx = add(context, ds);
	    (void)rpmdsFree(ds);
	    ds = NULL;
	}

    deps = argvFree(deps);
    if (elf) (void) elf_end(elf);
    if (fdno > 0)
	xx = close(fdno);
    return 0;
#else
    return -1;
#endif
}
#endif	/* RPM_VENDOR_MANDRIVA */

#define	_SBIN_LDCONFIG_P	"/sbin/ldconfig -p"
/*@unchecked@*/ /*@observer@*/ /*@owned@*/ /*@relnull@*/
static const char * _ldconfig_cmd = _SBIN_LDCONFIG_P;

#define	_LD_SO_CACHE	"/etc/ld.so.cache"
/*@unchecked@*/ /*@observer@*/ /*@owned@*/ /*@relnull@*/
static const char * _ldconfig_cache = NULL;

int rpmdsLdconfig(rpmPRCO PRCO, const char * fn)
	/*@globals _ldconfig_cmd, _ldconfig_cache @*/
	/*@modifies _ldconfig_cmd, _ldconfig_cache @*/
{
    char buf[BUFSIZ];
    const char *DSOfn;
    const char *N, *EVR;
    evrFlags Flags = (evrFlags) 0;
    rpmds ds;
    char * f, * fe;
    char * g, * ge;
    char * t;
    FILE * fp = NULL;
    int rc = -1;
    int xx;

    if (PRCO == NULL)
	return -1;

/*@-modobserver@*/
    if (_ldconfig_cmd == NULL) {
	_ldconfig_cmd = rpmExpand("%{?_rpmds_ldconfig_cmd}", NULL);
	if (!(_ldconfig_cmd != NULL && *_ldconfig_cmd == '/')) {
/*@-observertrans @*/
	    _ldconfig_cmd = _free(_ldconfig_cmd);
/*@=observertrans @*/
	    _ldconfig_cmd = xstrdup(_SBIN_LDCONFIG_P);
	}
    }

    if (_ldconfig_cache == NULL) {
	_ldconfig_cache = rpmExpand("%{?_rpmds_ldconfig_cache}", NULL);
	/* XXX may need to validate path existence somewhen. */
	if (!(_ldconfig_cache != NULL && *_ldconfig_cache == '/')) {
/*@-observertrans @*/
	    _ldconfig_cache = _free(_ldconfig_cache);
/*@=observertrans @*/
	    _ldconfig_cache = xstrdup(_LD_SO_CACHE);
	}
    }
/*@=modobserver@*/

    if (fn == NULL)
	fn = _ldconfig_cache;

if (_rpmds_debug < 0)
fprintf(stderr, "*** rpmdsLdconfig(%p, %s) P %p R %p C %p O %p T %p D %p L %p\n", PRCO, fn, PRCO->Pdsp, PRCO->Rdsp, PRCO->Cdsp, PRCO->Odsp, PRCO->Tdsp, PRCO->Ddsp, PRCO->Ldsp);

    fp = popen(_ldconfig_cmd, "r");
    if (fp == NULL)
	goto exit;

    while((f = fgets(buf, (int)sizeof(buf), fp)) != NULL) {
	EVR = NULL;
	/* rtrim on line. */
	ge = f + strlen(f);
	while (--ge > f && _isspace(*ge))
	    *ge = '\0';

	/* ltrim on line. */
	while (*f && _isspace(*f))
	    f++;

	/* split on '=>' */
	fe = f;
	while (*fe && !(fe[0] == '=' && fe[1] == '>'))
            fe++;
	if (*fe == '\0')
	    continue;

	/* find the DSO file name. */
	DSOfn = fe + 2;

	/* ltrim on DSO file name. */
	while (*DSOfn && _isspace(*DSOfn))
	    DSOfn++;
	if (*DSOfn == '\0')
	    continue;

	/* rtrim from "=>" */
	if (fe > f && fe[-1] == ' ') fe[-1] = '\0';
	*fe++ = '\0';
	*fe++ = '\0';
	g = fe;

	/* ltrim on field 2. */
	while (*g && _isspace(*g))
            g++;
	if (*g == '\0')
	    continue;

	/* split out flags */
	for (t = f; *t != '\0'; t++) {
	    if (!_isspace(*t))
		/*@innercontinue@*/ continue;
	    *t++ = '\0';
	    /*@innerbreak@*/ break;
	}
	/* XXX "libc4" "ELF" "libc5" "libc6" _("unknown") */
	/* XXX use flags to generate soname color */
	/* ",64bit" ",IA-64" ",x86-64", ",64bit" are color = 2 */
	/* ",N32" for mips64/libn32 */

	/* XXX use flags and LDASSUME_KERNEL to skip sonames? */
	/* "Linux" "Hurd" "Solaris" "FreeBSD" "kNetBSD" N_("Unknown OS") */
	/* ", OS ABI: %s %d.%d.%d" */

	N = f;
	if (EVR == NULL)
	    EVR = "";
	Flags = (evrFlags) (Flags | RPMSENSE_PROBE);
	ds = rpmdsSingle(RPMTAG_PROVIDENAME, N, EVR, Flags);
	xx = rpmdsMerge(PRCO->Pdsp, ds);
	(void)rpmdsFree(ds);
	ds = NULL;

	xx = rpmdsELF(DSOfn, 0, rpmdsMergePRCO, PRCO);
    }
    rc = 0;

exit:
    if (fp != NULL) (void) pclose(fp);
    return rc;
}


#if defined(__sun)
#define	_RLD_SEARCH_PATH	"/lib:/usr/lib"
/*@unchecked@*/ /*@observer@*/ /*@owned@*/ /*@relnull@*/
static const char * _rld_search_path = NULL;

/* search a colon-separated list of directories for shared objects */
int rpmdsRldpath(rpmPRCO PRCO, const char * rldp)
	/*@globals _rld_search_path @*/
	/*@modifies _rld_search_path @*/
{
    char buf[BUFSIZ];
    const char *N, *EVR;
    evrFlags Flags = 0;
    rpmds ds;
    const char * f;
    const char * g;
    int rc = -1;
    int xx;
    glob_t  gl;
    char ** gp;

    if (PRCO == NULL)
	return -1;

/*@-modobserver@*/
    if (_rld_search_path == NULL) {
	_rld_search_path = rpmExpand("%{?_rpmds_rld_search_path}", NULL);
	/* XXX may need to validate path existence somewhen. */
	if (!(_rld_search_path != NULL && *_rld_search_path == '/')) {
/*@-observertrans @*/
	    _rld_search_path = _free(_rld_search_path);
/*@=observertrans @*/
	    _rld_search_path = xstrdup(_RLD_SEARCH_PATH);
	}
    }
/*@=modobserver@*/

    if (rldp == NULL)
	rldp = _rld_search_path;

if (_rpmds_debug > 0)
fprintf(stderr, "*** rpmdsRldpath(%p, %s) P %p R %p C %p O %p\n", PRCO, rldp, PRCO->Pdsp, PRCO->Rdsp, PRCO->Cdsp, PRCO->Odsp);

    f = rldp;
    /* move through the path, splitting on : */
    while (f) {
	EVR = NULL;
	g = strchr(f, ':');
	if (g == NULL) {
	    strcpy(buf, f);
	    /* this is the last element, no more :'s */
	    f = NULL;
	} else {
	    /* copy this chunk to buf */
	    strncpy(buf, f, g - f + 1);
	    buf[g-f] = '\0';

	    /* get ready for next time through */
	    f = g + 1;
	}

	if ( !(strlen(buf) > 0 && buf[0] == '/') )
	    continue;

	/* XXX: danger, buffer len */
	/* XXX: *.so.* should be configurable via a macro */
	strcat(buf, "/*.so.*");

if (_rpmds_debug > 0)
fprintf(stderr, "*** rpmdsRldpath(%p, %s) globbing %s\n", PRCO, rldp, buf);

	xx = Glob(buf, 0, NULL, &gl);
	if (xx)		/* glob error, probably GLOB_NOMATCH */
	    continue;

if (_rpmds_debug > 0)
fprintf(stderr, "*** rpmdsRldpath(%p, %s) glob matched %d files\n", PRCO, rldp, gl.gl_pathc);

	gp = gl.gl_pathv;
	/* examine each match */
	while (gp && *gp) {
	    const char *DSOfn;
	    /* XXX: should probably verify that we matched a file */
	    DSOfn = *gp;
	    gp++;
	    if (EVR == NULL)
		EVR = "";

	    /* N needs to be basename of DSOfn */
	    N = DSOfn + strlen(DSOfn);
	    while (N > DSOfn && *N != '/')
		--N;

	    Flags |= RPMSENSE_PROBE;
	    ds = rpmdsSingle(RPMTAG_PROVIDENAME, N, EVR, Flags);
	    xx = rpmdsMerge(PRCO->Pdsp, ds);
	    (void)rpmdsFree(ds);
	    ds = NULL;

	    xx = rpmdsELF(DSOfn, 0, rpmdsMergePRCO, PRCO);
	}
/*@-immediatetrans@*/
	Globfree(&gl);
/*@=immediatetrans@*/
    }
    rc = 0;

    return rc;
}

#define	_SOLARIS_CRLE	"/usr/sbin/crle"
/*@unchecked@*/ /*@observer@*/ /*@owned@*/ /*@relnull@*/
static const char * _crle_cmd = NULL;

int rpmdsCrle(rpmPRCO PRCO, /*@unused@*/ const char * fn)
	/*@globals _crle_cmd @*/
	/*@modifies _crle_cmd @*/
{
    char buf[BUFSIZ];
    char * f;
    char * g, * ge;
    FILE * fp = NULL;
    int rc = -1;	/* assume failure */
    int xx;
    int found_dlp = 0;

    if (PRCO == NULL)
	return -1;

/*@-modobserver@*/
    if (_crle_cmd == NULL) {
	_crle_cmd = rpmExpand("%{?_rpmds_crle_cmd}", NULL);
	if (!(_crle_cmd != NULL && *_crle_cmd == '/')) {
/*@-observertrans @*/
	    _crle_cmd = _free(_crle_cmd);
/*@=observertrans @*/
	    _crle_cmd = xstrdup(_SOLARIS_CRLE);
	}
    }

    /* XXX: we rely on _crle_cmd including the -64 arg, if ELF64 */
    fp = popen(_crle_cmd, "r");
    if (fp == NULL)
	return rc;

    /* 
     * we want the first line that contains "(ELF):"
     * we cannot search for "Default Library Path (ELF):" because that
     * changes in non-C locales.
     */
    while((f = fgets(buf, sizeof(buf), fp)) != NULL) {
	if (found_dlp)	/* XXX read all data? */
	    continue;

	g = strstr(f, "(ELF):");
	if (g == NULL)
	    continue;

	found_dlp = 1;
	f = g + (sizeof("(ELF):")-1);
	while (_isspace(*f))
	    f++;

	/* rtrim path */
	ge = f + strlen(f);
	while (--ge > f && _isspace(*ge))
	    *ge = '\0';
    }
    xx = pclose(fp);

    /* we have the loader path, let rpmdsRldpath() do the work */
    if (found_dlp)
	rc = rpmdsRldpath(PRCO, f);

    return rc;
}
#endif

int rpmdsUname(rpmds *dsp, const struct utsname * un)
{
/*@observer@*/
    static const char * NS = "uname";
    struct utsname myun;
    int rc = -1;
    int xx;

    if (un == NULL) {
	xx = uname(&myun);
	if (xx != 0)
	    goto exit;
	un = &myun;
    }

/*@-type@*/
    /* XXX values need to be checked for EVR (i.e. no '-' character.) */
    if (un->sysname != NULL)
	rpmdsNSAdd(dsp, NS, "sysname", un->sysname, RPMSENSE_EQUAL);
    if (un->nodename != NULL)
	rpmdsNSAdd(dsp, NS, "nodename", un->nodename, RPMSENSE_EQUAL);
    if (un->release != NULL)
	rpmdsNSAdd(dsp, NS, "release", un->release, RPMSENSE_EQUAL);
#if 0	/* XXX has embedded spaces */
    if (un->version != NULL)
	rpmdsNSAdd(dsp, NS, "version", un->version, RPMSENSE_EQUAL);
#endif
    if (un->machine != NULL)
	rpmdsNSAdd(dsp, NS, "machine", un->machine, RPMSENSE_EQUAL);
#if defined(__linux__)
    if (un->domainname != NULL && strcmp(un->domainname, "(none)"))
	rpmdsNSAdd(dsp, NS, "domainname", un->domainname, RPMSENSE_EQUAL);
#endif
/*@=type@*/
    rc = 0;

exit:
    return rc;
}

#define	_PERL_PROVIDES	"/usr/bin/find /usr/lib/perl5 | /usr/lib/rpm/perl.prov"
/*@unchecked@*/ /*@observer@*/ /*@owned@*/ /*@relnull@*/
static const char * _perldeps_cmd = NULL;

int rpmdsPipe(rpmds * dsp, rpmTag tagN, const char * cmd)
	/*@globals _perldeps_cmd @*/
	/*@modifies _perldeps_cmd @*/
{
    char buf[BUFSIZ];
    const char *N, *EVR;
    evrFlags Flags = (evrFlags) 0;
    rpmds ds;
    char * f, * fe;
    char * g, * ge;
    FILE * fp = NULL;
    const char * fn = "pipe";
    int rc = -1;
    int cmdprinted;
    int ln;
    int xx;

/*@-modobserver@*/
    if (_perldeps_cmd == NULL) {
	_perldeps_cmd = rpmExpand("%{?_rpmds_perldeps_cmd}", NULL);
	/* XXX may need to validate path existence somewhen. */
	if (!(_perldeps_cmd != NULL && *_perldeps_cmd == '/')) {
/*@-observertrans @*/
	    _perldeps_cmd = _free(_perldeps_cmd);
/*@=observertrans @*/
	    _perldeps_cmd = xstrdup(_PERL_PROVIDES);
	}
    }
/*@=modobserver@*/

    if (tagN <= 0)
	tagN = RPMTAG_PROVIDENAME;
    if (cmd == NULL)
	cmd = _perldeps_cmd;

    fp = popen(cmd, "r");
    if (fp == NULL)
	goto exit;

    ln = 0;
    cmdprinted = 0;
    while((f = fgets(buf, (int)sizeof(buf), fp)) != NULL) {
	ln++;

	/* insure a terminator. */
	buf[sizeof(buf)-1] = '\0';

	/* ltrim on line. */
	while (*f && _isspace(*f))
	    f++;

	/* skip empty lines and comments */
	if (*f == '\0' || *f == '#')
	    continue;

	/* rtrim on line. */
	fe = f + strlen(f);
	while (--fe > f && _isspace(*fe))
	    *fe = '\0';

	/* split on ' '  or comparison operator. */
	fe = f;
	if (*f == '!') fe++;
	while (*fe && !_isspace(*fe) && strchr("!<=>", *fe) == NULL)
	    fe++;
	while (*fe && _isspace(*fe))
	    *fe++ = '\0';

	if (!(xisalnum(f[0]) || strchr("/_%!", f[0]) != NULL)) {
	    if (!cmdprinted++)
		fprintf(stderr, _("running \"%s\" pipe command\n"), cmd);
	    fprintf(stderr, _("%s:%d \"%s\" has invalid name. Skipping ...\n"),
			fn, ln, f);
            continue;
	}

	N = f;
	EVR = NULL;
	Flags = (evrFlags) 0;

	/* parse for non-path, versioned dependency. */
	if (*f != '/' && *fe != '\0') {
	    /* parse comparison operator */
	    g = fe;
	    Flags = rpmEVRflags(fe, (const char **)&g);
	    if (Flags == 0) {
		if (!cmdprinted++)
		    fprintf(stderr, _("running \"%s\" pipe command\n"), cmd),
		fprintf(stderr, _("%s:%d \"%s\" has no comparison operator. Skipping ...\n"),
			fn, ln, fe);
		continue;
	    }
	    *fe = '\0';

	    /* ltrim on field 2. */
	    while (*g && _isspace(*g))
		g++;
	    if (*g == '\0') {
		if (!cmdprinted++)
		    fprintf(stderr, _("running \"%s\" pipe command\n"), cmd),
		/* XXX No EVR comparison value found. */
		fprintf(stderr, _("\tline %d: No EVR comparison value found.\n Skipping ..."),
			ln);
		fprintf(stderr, _("%s:%d \"%s\" has no EVR string. Skipping ...\n"),
			fn, ln, f);
		continue;
	    }

	    ge = g + 1;
	    while (*ge && !_isspace(*ge))
		ge++;

	    if (*ge != '\0')
		*ge = '\0';	/* XXX can't happen, line rtrim'ed already. */

	    EVR = g;
	}

	if (EVR == NULL)
	    EVR = "";
	Flags = (evrFlags) (Flags | RPMSENSE_PROBE);
	ds = rpmdsSingle(tagN, N, EVR, Flags);
	xx = rpmdsMerge(dsp, ds);
	(void)rpmdsFree(ds);
	ds = NULL;
    }
    rc = 0;

exit:
    if (fp != NULL) (void) pclose(fp);
    return rc;
}

static int rpmdsNAcmp(rpmds A, rpmds B)
	/*@*/
{
    const char * AN = A->ns.N;
    const char * AA = A->ns.A;
    const char * BN = B->ns.N;
    const char * BA = B->ns.A;
    int rc;

    if (!AA && !BA) {
	rc = strcmp(AN, BN);
    } else if (AA && !BA) {
	rc = strncmp(AN, BN, (AA - AN)) || BN[AA - AN];
	if (!rc)
	    rc = strcmp(AA, B->A);
    } else if (!AA && BA) {
	rc = strncmp(AN, BN, (BA - BN)) || AN[BA - BN];
	if (!rc)
	    rc = strcmp(BA, A->A);
    } else {
	rc = strcmp(AN, BN);
    }
    return rc;
}

/*@unchecked@*/ /*@only@*/ /*@null@*/
const char * evr_tuple_order = NULL;

/**
 * Return precedence permutation string.
 * @return		precedence permutation
 */
/*@observer@*/
static const char * rpmdsEVRorder(void)
	/*@globals evr_tuple_order @*/
	/*@modifies evr_tuple_order @*/
{
    if (evr_tuple_order == NULL) {
/*@-mods@*/
	evr_tuple_order = rpmExpand("%{?evr_tuple_order}", NULL);
/*@=mods@*/
	if (evr_tuple_order == NULL || evr_tuple_order[0] == '\0') {
	    /* XXX coverity #1035879 */
	    evr_tuple_order = _free(evr_tuple_order);
	    evr_tuple_order = xstrdup("EVR");
	}
    }
assert(evr_tuple_order != NULL && evr_tuple_order[0] != '\0');
/*@-freshtrans@*/
    return evr_tuple_order;
/*@=freshtrans@*/
}

int rpmdsCompare(const rpmds A, const rpmds B)
{
    const char *aDepend = (A->DNEVR != NULL ? xstrdup(A->DNEVR+2) : "");
    const char *bDepend = (B->DNEVR != NULL ? xstrdup(B->DNEVR+2) : "");
    EVR_t a = (EVR_t) memset(alloca(sizeof(*a)), 0, sizeof(*a));
    EVR_t b = (EVR_t) memset(alloca(sizeof(*a)), 0, sizeof(*a));
    evrFlags aFlags = A->ns.Flags;
    evrFlags bFlags = B->ns.Flags;
    int (*EVRcmp) (const char *a, const char *b);
    int result = 1;
    const char * s;
    int set_version = 0;
    int sense;
    int xx;

assert((rpmdsFlags(A) & RPMSENSE_SENSEMASK) == A->ns.Flags);
assert((rpmdsFlags(B) & RPMSENSE_SENSEMASK) == B->ns.Flags);
    /* Different names (and/or name.arch's) don't overlap. */
    if (rpmdsNAcmp(A, B)) {
	result = 0;
	goto exit;
    }

    /* XXX rpm prior to 3.0.2 did not always supply EVR and Flags. */
/*@-nullderef@*/
    if (!(A->EVR && A->Flags && B->EVR && B->Flags))
	goto exit;

    /* Same name. If either A or B is an existence test, always overlap. */
    if (!(aFlags && bFlags))
	goto exit;

    /* If either EVR is non-existent or empty, always overlap. */
    if (!(A->EVR[A->i] && *A->EVR[A->i] && B->EVR[B->i] && *B->EVR[B->i]))
	goto exit;

    /* Both AEVR and BEVR exist. */
    xx = (A->EVRparse ? A->EVRparse : rpmEVRparse) (A->EVR[A->i], a);
    xx = (B->EVRparse ? B->EVRparse : rpmEVRparse) (B->EVR[B->i], b);

    /* If EVRcmp is identical, use that, otherwise use default. */
    EVRcmp = (A->EVRcmp && B->EVRcmp && A->EVRcmp == B->EVRcmp)
	? A->EVRcmp : rpmvercmp;

    /* Compare {A,B} [epoch:]version[-release][:distepoch] */
    sense = 0;
    for (s = rpmdsEVRorder(); *s; s++) {
	int ix;
        switch ((int)*s) {
        default:        continue;       /*@notreached@*//*@switchbreak@*/ break;
        case 'E':
	    ix = RPMEVR_E;
	    if (a->F[ix] && *a->F[ix] && b->F[ix] && *b->F[ix]) {
		/* XXX ALT version-set comparison */
		if (!strcmp(a->F[ix], "set") && !strcmp(b->F[ix], "set"))
		    set_version = 1;
		/*@switchbreak@*/ break;
	    }

	    /* XXX Special handling for missing Epoch: tags hysteria */
	    if (a->F[ix] && *a->F[ix] && atol(a->F[ix]) > 0) {
		if (!B->nopromote) {
		    int lvl = (_rpmds_unspecified_epoch_noise
			? RPMLOG_WARNING : RPMLOG_DEBUG);
		    rpmlog(lvl, _("The \"B\" dependency needs an epoch (assuming same epoch as \"A\")\n\tA = \"%s\"\tB = \"%s\"\n"),
			aDepend, bDepend);
		    sense = 0;
		} else
		    sense = 1;
	    } else
	    if (b->F[ix] && *b->F[ix] && atol(b->F[ix]) > 0)
		sense = -1;
	    /*@switchbreak@*/ break;
        case 'V':	ix = RPMEVR_V;	/*@switchbreak@*/break;
        case 'T':	ix = RPMEVR_T;	/*@switchbreak@*/break;
        case 'R':	ix = RPMEVR_R;	/*@switchbreak@*/break;
        case 'D':	ix = RPMEVR_D;	/*@switchbreak@*/break;
        }
#if defined(RPM_VENDOR_MANDRIVA) /* mdvbz#55810 */
	if(ix >= RPMEVR_R && (bFlags & (~RPMSENSE_GREATER & RPMSENSE_EQUAL))
			&& !(ix == RPMEVR_D && (bFlags & RPMSENSE_LESS))
			&& *(b->F[ix]) == '\0')
		break;
	if (a->F[ix] && b->F[ix])
#else
	if (a->F[ix] && *a->F[ix] && b->F[ix] && *b->F[ix])
#endif
	{
	  /* XXX ALT version-set comparison */
	  if (ix == RPMEVR_V && set_version) {
	    set_version = 0;
	    sense = rpmsetCmp(a->F[ix], b->F[ix]);
	    if (sense < -1) {
		if (sense == -3)
		    rpmlog(RPMLOG_WARNING, _("failed to decode %s\n"), a->F[ix]);
		if (sense == -4)
		    rpmlog(RPMLOG_WARNING, _("failed to decode %s\n"), b->F[ix]);
		/* neither is subset of each other */
		sense = 0;
	    }
	  } else
	  if (ix == RPMEVR_T)	/* XXX twiddle-in-version "negative" compare */
            sense = -EVRcmp(a->F[ix], b->F[ix]);
	  else
/*@i@*/	    sense = EVRcmp(a->F[ix], b->F[ix]);
	}
	if (sense)
	    break;
    }

    a->str = _free(a->str);
    b->str = _free(b->str);

    /* Detect overlap of {A,B} range. */
    if (aFlags == RPMSENSE_NOTEQUAL || bFlags == RPMSENSE_NOTEQUAL) {
	result = (sense != 0);
    } else if (sense < 0 && ((aFlags & RPMSENSE_GREATER) || (bFlags & RPMSENSE_LESS))) {
	result = 1;
    } else if (sense > 0 && ((aFlags & RPMSENSE_LESS) || (bFlags & RPMSENSE_GREATER))) {
	result = 1;
    } else if (sense == 0 &&
	(((aFlags & RPMSENSE_EQUAL) && (bFlags & RPMSENSE_EQUAL)) ||
	 ((aFlags & RPMSENSE_LESS) && (bFlags & RPMSENSE_LESS)) ||
	 ((aFlags & RPMSENSE_GREATER) && (bFlags & RPMSENSE_GREATER)))) {
	result = 1;
    } else
	result = 0;
/*@=nullderef@*/

exit:
    if (_noisy_range_comparison_debug_message)
    rpmlog(RPMLOG_DEBUG, D_("  %s    A %s\tB %s\n"),
	(result ? _("YES") : _("NO ")), aDepend, bDepend);
    aDepend = _free(aDepend);
    bDepend = _free(bDepend);
    return result;
}

int rpmdsMatch(const rpmds A, rpmds B)
{
    int result = 0;

    /* If A dependency matches any in B, we're done. */
    if ((B = rpmdsInit(B)) != NULL)
    while (rpmdsNext(B) >= 0)
	if ((result = rpmdsCompare(A, B)))
	    break;
    return result;
}

void rpmdsProblem(rpmps ps, const char * pkgNEVR, const rpmds ds,
	const fnpyKey * suggestedKeys, int adding)
{
    const char * Name =  rpmdsN(ds);
    const char * DNEVR = rpmdsDNEVR(ds);
    const char * EVR = rpmdsEVR(ds);
    rpmProblemType type;
    fnpyKey key;

    if (ps == NULL) return;

    if (Name == NULL) Name = "?N?";
    if (EVR == NULL) EVR = "?EVR?";
    if (DNEVR == NULL) DNEVR = "? ?N? ?OP? ?EVR?";

    rpmlog(RPMLOG_DEBUG, D_("package %s has unsatisfied %s: %s\n"),
	    pkgNEVR, ds->Type, DNEVR+2);

    switch ((unsigned)DNEVR[0]) {
    case 'C':	type = RPMPROB_CONFLICT;	break;
    default:
    case 'R':	type = RPMPROB_REQUIRES;	break;
    }

    key = (suggestedKeys ? suggestedKeys[0] : NULL);
    rpmpsAppend(ps, type, pkgNEVR, key, NULL, NULL, DNEVR, adding);
}

int rpmdsAnyMatchesDep (const Header h, const rpmds req, int nopromote)
{
    int scareMem = 0;
    rpmds provides = NULL;
    evrFlags reqFlags = req->ns.Flags;
    int result = 1;

assert((rpmdsFlags(req) & RPMSENSE_SENSEMASK) == req->ns.Flags);
    /* XXX rpm prior to 3.0.2 did not always supply EVR and Flags. */
    if (req->EVR == NULL || req->Flags == NULL)
	goto exit;

    switch(req->ns.Type) {
    default:
	/* Primary key retrieve satisfes an existence compare. */
	if (!reqFlags || !req->EVR[req->i] || *req->EVR[req->i] == '\0')
	    goto exit;
	/*@fallthrough@*/
    case RPMNS_TYPE_ARCH:
	break;
    }

    /* Get provides information from header */
    provides = rpmdsInit(rpmdsNew(h, RPMTAG_PROVIDENAME, scareMem));
    if (provides == NULL) {
	result = 0;
	goto exit;	/* XXX should never happen */
    }
    if (nopromote)
	(void) rpmdsSetNoPromote(provides, nopromote);

    /*
     * Rpm prior to 3.0.3 did not have versioned provides.
     * If no provides version info is available, match any/all requires
     * with same name.
     */
    if (provides->EVR == NULL)
	goto exit;

    /* If any provide matches the require, we're done. */
    result = 0;
    if (provides != NULL)
    while (rpmdsNext(provides) >= 0)
	if ((result = rpmdsCompare(provides, req)))
	    break;

exit:
    (void)rpmdsFree(provides);
    provides = NULL;

    return result;
}

int rpmdsNVRMatchesDep(const Header h, const rpmds req, int nopromote)
{
    HE_t he = (HE_t) memset(alloca(sizeof(*he)), 0, sizeof(*he));
    const char * pkgN, * V, * R;
#ifdef	RPM_VENDOR_MANDRIVA
    const char * D;
    int gotD = 0;
#endif
    rpmuint32_t E;
    int gotE = 0;
    const char * pkgEVR;
    char * t;
    evrFlags reqFlags = req->ns.Flags;
    evrFlags pkgFlags = RPMSENSE_EQUAL;
    int result = 1;
    rpmds pkg;
    size_t nb;

assert((rpmdsFlags(req) & RPMSENSE_SENSEMASK) == req->ns.Flags);
    /* XXX rpm prior to 3.0.2 did not always supply EVR and Flags. */
    if (req->EVR == NULL || req->Flags == NULL)
	goto exit;

    if (!(reqFlags && req->EVR[req->i] && *req->EVR[req->i]))
	goto exit;

    /* Get package information from header */
/*@-mods@*/
    (void) headerNEVRA(h, &pkgN, NULL, &V, &R, NULL);
/*@=mods@*/
    /* XXX segfault avoidance */
    if (pkgN == NULL)	pkgN = xstrdup("N");
    if (V == NULL)	V = xstrdup("V");
    if (R == NULL)	R = xstrdup("R");
    he->tag = RPMTAG_EPOCH;
    gotE = headerGet(h, he, 0);
    E = (he->p.ui32p ? he->p.ui32p[0] : 0);
    he->p.ptr = _free(he->p.ptr);

#if defined(RPM_VENDOR_MANDRIVA)
    he->tag = RPMTAG_DISTEPOCH;
    gotD = headerGet(h, he, 0);
    D = (he->p.str ? he->p.str : NULL);
#endif

    nb = 21 + 1 + 1;
    if (V) nb += strlen(V);
    if (R) nb += strlen(R);
#if defined(RPM_VENDOR_MANDRIVA)
    if (gotD) nb += strlen(D) + 1;
#endif
    pkgEVR = t = (char *) alloca(nb);
    *t = '\0';
    if (gotE) {
	sprintf(t, "%d:", E);
	t += strlen(t);
    }
    t = stpcpy( stpcpy( stpcpy(t, V) , "-") , R);
#if defined(RPM_VENDOR_MANDRIVA)
    if (gotD) {
	t =  stpcpy( stpcpy( t, ":"), D);
	D = _free(D);
    }
#endif
    V = _free(V);
    R = _free(R);

    if ((pkg = rpmdsSingle(RPMTAG_PROVIDENAME, pkgN, pkgEVR, pkgFlags)) != NULL) {
	if (nopromote)
	    (void) rpmdsSetNoPromote(pkg, nopromote);
	result = rpmdsCompare(pkg, req);
	(void)rpmdsFree(pkg);
	pkg = NULL;
    }
    pkgN = _free(pkgN);

exit:
    return result;
}

int rpmdsNegateRC(const rpmds ds, int rc)
{
    if (ds->ns.str[0] == '!')
	rc = (rc == 0);
    return rc;
}
