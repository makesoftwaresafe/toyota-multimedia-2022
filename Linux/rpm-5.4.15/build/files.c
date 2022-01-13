/** \ingroup rpmbuild
 * \file build/files.c
 *  The post-build, pre-packaging file tree walk to assemble the package
 *  manifest.
 */

#include "system.h"

#define	MYALLPERMS	07777

#if defined(WITH_PCRE) && defined(WITH_PCRE_POSIX)
#include <pcreposix.h>
#else
#include <regex.h>
#endif

#define	_RPMIOB_INTERNAL
#include <rpmiotypes.h>
#include <rpmio_internal.h>	/* XXX fdGetFp */
#include <rpmbf.h>
#include <rpmcb.h>
#define	_RPMSX_INTERNAL		/* XXX permit disabling. */
#include <rpmsx.h>
#include <fts.h>
#include <argv.h>

#include "iosm.h"
#define	_RPMTAG_INTERNAL	/* XXX rpmTags->aTags */
#define	_RPMFI_INTERNAL
#include <rpmbuild.h>

#define	_RPMTE_INTERNAL
#include <rpmte.h>

#include "rpmfc.h"

#include "buildio.h"

#include "legacy.h"	/* XXX dodigest */
#include "debug.h"

/*@access Header @*/
/*@access rpmfi @*/
/*@access rpmte @*/
/*@access FD_t @*/

#define	SKIPWHITE(_x)	{while(*(_x) && (xisspace(*_x) || *(_x) == ',')) (_x)++;}
#define	SKIPNONWHITE(_x){while(*(_x) &&!(xisspace(*_x) || *(_x) == ',')) (_x)++;}

#define MAXDOCDIR 1024

/**
 */
typedef enum specdFlags_e {
    SPECD_DEFFILEMODE	= (1 << 0),
    SPECD_DEFDIRMODE	= (1 << 1),
    SPECD_DEFUID	= (1 << 2),
    SPECD_DEFGID	= (1 << 3),
    SPECD_DEFVERIFY	= (1 << 4),

    SPECD_FILEMODE	= (1 << 8),
    SPECD_DIRMODE	= (1 << 9),
    SPECD_UID		= (1 << 10),
    SPECD_GID		= (1 << 11),
    SPECD_VERIFY	= (1 << 12)
} specdFlags;

/**
 */
typedef struct FileListRec_s {
    struct stat fl_st;
#define	fl_dev	fl_st.st_dev
#define	fl_ino	fl_st.st_ino
#define	fl_mode	fl_st.st_mode
#define	fl_nlink fl_st.st_nlink
#define	fl_uid	fl_st.st_uid
#define	fl_gid	fl_st.st_gid
#define	fl_rdev	fl_st.st_rdev
#define	fl_size	fl_st.st_size
#define	fl_mtime fl_st.st_mtime

/*@only@*/
    const char *diskURL;	/* get file from here       */
/*@only@*/
    const char *fileURL;	/* filename in cpio archive */
/*@observer@*/
    const char *uname;
/*@observer@*/
    const char *gname;
    unsigned	flags;
    specdFlags	specdFlags;	/* which attributes have been explicitly specified. */
    unsigned	verifyFlags;
/*@only@*/
    const char *langs;		/* XXX locales separated with | */
} * FileListRec;

/**
 */
typedef struct AttrRec_s {
/*@null@*/
    const char *ar_fmodestr;
/*@null@*/
    const char *ar_dmodestr;
/*@null@*/
    const char *ar_user;
/*@null@*/
    const char *ar_group;
    mode_t	ar_fmode;
    mode_t	ar_dmode;
} * AttrRec;

/*@-readonlytrans@*/
/*@unchecked@*/ /*@observer@*/
static struct AttrRec_s root_ar = { NULL, NULL, "root", "root", 0, 0 };
/*@=readonlytrans@*/

/**
 * Package file tree walk data.
 */
typedef struct FileList_s {
/*@only@*/
    const char * buildRootURL;
/*@only@*/
    const char * prefix;

    int fileCount;
    int totalFileSize;
    int processingFailed;

    int passedSpecialDoc;
    int isSpecialDoc;

    int noGlob;
    unsigned devtype;
    unsigned devmajor;
    int devminor;
    
    int isDir;
    int inFtw;
    int currentFlags;
    specdFlags currentSpecdFlags;
    unsigned currentVerifyFlags;
    struct AttrRec_s cur_ar;
    struct AttrRec_s def_ar;
    specdFlags defSpecdFlags;
    unsigned defVerifyFlags;
    int nLangs;
/*@only@*/ /*@null@*/
    const char ** currentLangs;

    /* Hard coded limit of MAXDOCDIR docdirs.         */
    /* If you break it you are doing something wrong. */
    const char * docDirs[MAXDOCDIR];
    int docDirCount;
    
/*@only@*/
    FileListRec fileList;
    int fileListRecsAlloced;
    int fileListRecsUsed;
} * FileList;

/**
 */
static void nullAttrRec(/*@out@*/ AttrRec ar)	/*@modifies ar @*/
{
    ar->ar_fmodestr = NULL;
    ar->ar_dmodestr = NULL;
    ar->ar_user = NULL;
    ar->ar_group = NULL;
    ar->ar_fmode = 0;
    ar->ar_dmode = 0;
}

/**
 */
static void freeAttrRec(AttrRec ar)	/*@modifies ar @*/
{
    ar->ar_fmodestr = _free(ar->ar_fmodestr);
    ar->ar_dmodestr = _free(ar->ar_dmodestr);
    ar->ar_user = _free(ar->ar_user);
    ar->ar_group = _free(ar->ar_group);
    /* XXX doesn't free ar (yet) */
    /*@-nullstate@*/
    return;
    /*@=nullstate@*/
}

/**
 */
static void dupAttrRec(const AttrRec oar, /*@in@*/ /*@out@*/ AttrRec nar)
	/*@modifies nar @*/
{
    if (oar == nar)
	return;
    freeAttrRec(nar);
    nar->ar_fmodestr = (oar->ar_fmodestr ? xstrdup(oar->ar_fmodestr) : NULL);
    nar->ar_dmodestr = (oar->ar_dmodestr ? xstrdup(oar->ar_dmodestr) : NULL);
    nar->ar_user = (oar->ar_user ? xstrdup(oar->ar_user) : NULL);
    nar->ar_group = (oar->ar_group ? xstrdup(oar->ar_group) : NULL);
    nar->ar_fmode = oar->ar_fmode;
    nar->ar_dmode = oar->ar_dmode;
}

#if 0
/**
 */
static void dumpAttrRec(const char * msg, AttrRec ar)
	/*@globals fileSystem@*/
	/*@modifies fileSystem @*/
{
    if (msg)
	fprintf(stderr, "%s:\t", msg);
    fprintf(stderr, "(%s, %s, %s, %s)\n",
	ar->ar_fmodestr,
	ar->ar_user,
	ar->ar_group,
	ar->ar_dmodestr);
}
#endif

/**
 * Strip quotes from strtok(3) string.
 * @param s		string
 * @param delim		token delimiters
 */
/*@null@*/
static char *strtokWithQuotes(/*@null@*/ char *s, const char *delim)
	/*@modifies *s @*/
{
    static char *olds = NULL;
    char *token;

    if (s == NULL)
	s = olds;
    if (s == NULL)
	return NULL;

    /* Skip leading delimiters */
    s += strspn(s, delim);
    if (*s == '\0')
	return NULL;

    /* Find the end of the token.  */
    token = s;
    if (*token == '"') {
	token++;
	/* Find next " char */
	s = strchr(token, '"');
    } else {
	s = strpbrk(token, delim);
    }

    /* Terminate it */
    if (s == NULL) {
	/* This token finishes the string */
	olds = strchr(token, '\0');
    } else {
	/* Terminate the token and make olds point past it */
	*s = '\0';
	olds = s+1;
    }

    /*@-retalias -temptrans @*/
    return token;
    /*@=retalias =temptrans @*/
}

/**
 */
static void timeCheck(int tc, Header h)
	/*@globals internalState @*/
	/*@modifies internalState @*/
{
    HE_t he = (HE_t) memset(alloca(sizeof(*he)), 0, sizeof(*he));
    rpmuint32_t currentTime = (rpmuint32_t) time(NULL);
    rpmuint32_t * mtime;
    int xx;
    size_t i;

    he->tag = RPMTAG_FILEMTIMES;
    xx = headerGet(h, he, 0);
    mtime = he->p.ui32p;
    he->tag = RPMTAG_OLDFILENAMES;
    xx = headerGet(h, he, 0);
    
    for (i = 0; i < he->c; i++) {
	xx = currentTime - mtime[i];
	if (xx < 0) xx = -xx;
	if (xx > tc)
	    rpmlog(RPMLOG_WARNING, _("TIMECHECK failure: %s\n"), he->p.argv[i]);
    }
    he->p.ptr = _free(he->p.ptr);
    mtime = _free(mtime);
}

/**
 */
typedef struct VFA {
/*@observer@*/ /*@null@*/ const char * attribute;
    int not;
    int	flag;
} VFA_t;

/**
 */
/*@-exportlocal -exportheadervar@*/
/*@unchecked@*/
static VFA_t verifyAttrs[] = {
    { "md5",	0,	RPMVERIFY_FDIGEST },	/* XXX legacy syntax */
    { "size",	0,	RPMVERIFY_FILESIZE },
    { "link",	0,	RPMVERIFY_LINKTO },
    { "user",	0,	RPMVERIFY_USER },
    { "owner",	0,	RPMVERIFY_USER },
    { "group",	0,	RPMVERIFY_GROUP },
    { "mtime",	0,	RPMVERIFY_MTIME },
    { "mode",	0,	RPMVERIFY_MODE },
    { "rdev",	0,	RPMVERIFY_RDEV },
    { "digest",	0,	RPMVERIFY_FDIGEST },
    { "hmac",	0,	RPMVERIFY_HMAC },
    { NULL, 0,	0 }
};
/*@=exportlocal =exportheadervar@*/

/**
 * Parse %verify and %defverify from file manifest.
 * @param buf		current spec file line
 * @param fl		package file tree walk data
 * @return		RPMRC_OK on success
 */
static rpmRC parseForVerify(char * buf, FileList fl)
	/*@modifies buf, fl->processingFailed,
		fl->currentVerifyFlags, fl->defVerifyFlags,
		fl->currentSpecdFlags, fl->defSpecdFlags @*/
{
    char *p, *pe, *q;
    const char *name;
    unsigned *resultVerify;
    int negated;
    unsigned verifyFlags;
    specdFlags * specdFlags;

    if ((p = strstr(buf, (name = "%verify"))) != NULL) {
	resultVerify = &(fl->currentVerifyFlags);
	specdFlags = &fl->currentSpecdFlags;
    } else if ((p = strstr(buf, (name = "%defverify"))) != NULL) {
	resultVerify = &(fl->defVerifyFlags);
	specdFlags = &fl->defSpecdFlags;
    } else
	return RPMRC_OK;

    for (pe = p; (size_t)(pe-p) < strlen(name); pe++)
	*pe = ' ';

    SKIPSPACE(pe);

    if (*pe != '(') {
	rpmlog(RPMLOG_ERR, _("Missing '(' in %s %s\n"), name, pe);
	fl->processingFailed = 1;
	return RPMRC_FAIL;
    }

    /* Bracket %*verify args */
    *pe++ = ' ';
    for (p = pe; *pe && *pe != ')'; pe++)
	{};

    if (*pe == '\0') {
	rpmlog(RPMLOG_ERR, _("Missing ')' in %s(%s\n"), name, p);
	fl->processingFailed = 1;
	return RPMRC_FAIL;
    }

    /* Localize. Erase parsed string */
    q = alloca((pe-p) + 1);
    strncpy(q, p, pe-p);
    q[pe-p] = '\0';
    while (p <= pe)
	*p++ = ' ';

    negated = 0;
    verifyFlags = RPMVERIFY_NONE;

    for (p = q; *p != '\0'; p = pe) {
	SKIPWHITE(p);
	if (*p == '\0')
	    break;
	pe = p;
	SKIPNONWHITE(pe);
	if (*pe != '\0')
	    *pe++ = '\0';

	{   VFA_t *vfa;
	    for (vfa = verifyAttrs; vfa->attribute != NULL; vfa++) {
		if (strcmp(p, vfa->attribute))
		    /*@innercontinue@*/ continue;
		verifyFlags |= vfa->flag;
		    verifyFlags &= ~RPMVERIFY_FDIGEST;
		/*@innerbreak@*/ break;
	    }
	    if (vfa->attribute)
		continue;
	}

	if (!strcmp(p, "not")) {
	    negated ^= 1;
	} else {
	    rpmlog(RPMLOG_ERR, _("Invalid %s token: %s\n"), name, p);
	    fl->processingFailed = 1;
	    return RPMRC_FAIL;
	}
    }

    *resultVerify = negated ? ~(verifyFlags) : verifyFlags;
    if (negated) {
	/* Make sure "no digest" implies "no hmac" */
	if (!(*resultVerify & RPMVERIFY_FDIGEST))
	    *resultVerify &= ~RPMVERIFY_HMAC;
    } else {
	/* Make sure "hmac" implies "no digest" */
	if (*resultVerify & RPMVERIFY_HMAC)
	    *resultVerify &= ~RPMVERIFY_FDIGEST;
    }
    *specdFlags |= SPECD_VERIFY;

    return RPMRC_OK;
}

#define	isAttrDefault(_ars)	((_ars)[0] == '-' && (_ars)[1] == '\0')

/**
 * Parse %dev from file manifest.
 * @param buf		current spec file line
 * @param fl		package file tree walk data
 * @return		RPMRC_OK on success
 */
static rpmRC parseForDev(char * buf, FileList fl)
	/*@modifies buf, fl->processingFailed,
		fl->noGlob, fl->devtype, fl->devmajor, fl->devminor @*/
{
    const char * name;
    const char * errstr = NULL;
    char *p, *pe, *q;
    rpmRC rc = RPMRC_FAIL;	/* assume error */

    if ((p = strstr(buf, (name = "%dev"))) == NULL)
	return RPMRC_OK;

    for (pe = p; (size_t)(pe-p) < strlen(name); pe++)
	*pe = ' ';
    SKIPSPACE(pe);

    if (*pe != '(') {
	errstr = "'('";
	goto exit;
    }

    /* Bracket %dev args */
    *pe++ = ' ';
    for (p = pe; *pe && *pe != ')'; pe++)
	{};
    if (*pe != ')') {
	errstr = "')'";
	goto exit;
    }

    /* Localize. Erase parsed string */
    q = alloca((pe-p) + 1);
    strncpy(q, p, pe-p);
    q[pe-p] = '\0';
    while (p <= pe)
	*p++ = ' ';

    p = q; SKIPWHITE(p);
    pe = p; SKIPNONWHITE(pe); if (*pe != '\0') *pe++ = '\0';
    if (*p == 'b')
	fl->devtype = 'b';
    else if (*p == 'c')
	fl->devtype = 'c';
    else {
	errstr = "devtype";
	goto exit;
    }

    p = pe; SKIPWHITE(p);
    pe = p; SKIPNONWHITE(pe); if (*pe != '\0') *pe++ = '\0';
    for (pe = p; *pe && xisdigit(*pe); pe++)
	{} ;
    if (*pe == '\0') {
	fl->devmajor = atoi(p);
	/*@-unsignedcompare @*/	/* LCL: ge is ok */
	if (!((int)fl->devmajor >= 0 && (int)fl->devmajor < 256)) {
	    errstr = "devmajor";
	    goto exit;
	}
	/*@=unsignedcompare @*/
	pe++;
    } else {
	errstr = "devmajor";
	goto exit;
    }

    p = pe; SKIPWHITE(p);
    pe = p; SKIPNONWHITE(pe); if (*pe != '\0') *pe++ = '\0';
    for (pe = p; *pe && xisdigit(*pe); pe++)
	{} ;
    if (*pe == '\0') {
	fl->devminor = atoi(p);
	if (!(fl->devminor >= 0 && fl->devminor < 256)) {
	    errstr = "devminor";
	    goto exit;
	}
	pe++;
    } else {
	errstr = "devminor";
	goto exit;
    }

    fl->noGlob = 1;

    rc = 0;

exit:
    if (rc) {
	rpmlog(RPMLOG_ERR, _("Missing %s in %s %s\n"), errstr, name, p);
	fl->processingFailed = 1;
    }
    return rc;
}

/**
 * Parse %attr and %defattr from file manifest.
 * @param buf		current spec file line
 * @param fl		package file tree walk data
 * @return		RPMRC_OK on success
 */
static rpmRC parseForAttr(char * buf, FileList fl)
	/*@modifies buf, fl->processingFailed,
		fl->cur_ar, fl->def_ar,
		fl->currentSpecdFlags, fl->defSpecdFlags @*/
{
    const char *name;
    char *p, *pe, *q;
    int x;
    struct AttrRec_s arbuf;
    AttrRec ar = &arbuf, ret_ar;
    specdFlags * specdFlags;

    if ((p = strstr(buf, (name = "%attr"))) != NULL) {
	ret_ar = &(fl->cur_ar);
	specdFlags = &fl->currentSpecdFlags;
    } else if ((p = strstr(buf, (name = "%defattr"))) != NULL) {
	ret_ar = &(fl->def_ar);
	specdFlags = &fl->defSpecdFlags;
    } else
	return RPMRC_OK;

    for (pe = p; (size_t)(pe-p) < strlen(name); pe++)
	*pe = ' ';

    SKIPSPACE(pe);

    if (*pe != '(') {
	rpmlog(RPMLOG_ERR, _("Missing '(' in %s %s\n"), name, pe);
	fl->processingFailed = 1;
	return RPMRC_FAIL;
    }

    /* Bracket %*attr args */
    *pe++ = ' ';
    for (p = pe; *pe && *pe != ')'; pe++)
	{};

    if (ret_ar == &(fl->def_ar)) {	/* %defattr */
	q = pe;
	q++;
	SKIPSPACE(q);
	if (*q != '\0') {
	    rpmlog(RPMLOG_ERR,
		     _("Non-white space follows %s(): %s\n"), name, q);
	    fl->processingFailed = 1;
	    return RPMRC_FAIL;
	}
    }

    /* Localize. Erase parsed string */
    q = alloca((pe-p) + 1);
    strncpy(q, p, pe-p);
    q[pe-p] = '\0';
    while (p <= pe)
	*p++ = ' ';

    nullAttrRec(ar);

    p = q; SKIPWHITE(p);
    if (*p != '\0') {
	pe = p; SKIPNONWHITE(pe); if (*pe != '\0') *pe++ = '\0';
	ar->ar_fmodestr = p;
	p = pe; SKIPWHITE(p);
    }
    if (*p != '\0') {
	pe = p; SKIPNONWHITE(pe); if (*pe != '\0') *pe++ = '\0';
	ar->ar_user = p;
	p = pe; SKIPWHITE(p);
    }
    if (*p != '\0') {
	pe = p; SKIPNONWHITE(pe); if (*pe != '\0') *pe++ = '\0';
	ar->ar_group = p;
	p = pe; SKIPWHITE(p);
    }
    if (*p != '\0' && ret_ar == &(fl->def_ar)) {	/* %defattr */
	pe = p; SKIPNONWHITE(pe); if (*pe != '\0') *pe++ = '\0';
	ar->ar_dmodestr = p;
	p = pe; SKIPWHITE(p);
    }

    if (!(ar->ar_fmodestr && ar->ar_user && ar->ar_group) || *p != '\0') {
	rpmlog(RPMLOG_ERR, _("Bad syntax: %s(%s)\n"), name, q);
	fl->processingFailed = 1;
	return RPMRC_FAIL;
    }

    /* Do a quick test on the mode argument and adjust for "-" */
    if (ar->ar_fmodestr && !isAttrDefault(ar->ar_fmodestr)) {
	unsigned int ui;
	x = sscanf(ar->ar_fmodestr, "%o", &ui);
	if ((x == 0) || (ar->ar_fmode & ~MYALLPERMS)) {
	    rpmlog(RPMLOG_ERR, _("Bad mode spec: %s(%s)\n"), name, q);
	    fl->processingFailed = 1;
	    return RPMRC_FAIL;
	}
	ar->ar_fmode = ui;
    } else
	ar->ar_fmodestr = NULL;

    if (ar->ar_dmodestr && !isAttrDefault(ar->ar_dmodestr)) {
	unsigned int ui;
	x = sscanf(ar->ar_dmodestr, "%o", &ui);
	if ((x == 0) || (ar->ar_dmode & ~MYALLPERMS)) {
	    rpmlog(RPMLOG_ERR, _("Bad dirmode spec: %s(%s)\n"), name, q);
	    fl->processingFailed = 1;
	    return RPMRC_FAIL;
	}
	ar->ar_dmode = ui;
    } else
	ar->ar_dmodestr = NULL;

    if (!(ar->ar_user && !isAttrDefault(ar->ar_user)))
	ar->ar_user = NULL;

    if (!(ar->ar_group && !isAttrDefault(ar->ar_group)))
	ar->ar_group = NULL;

    dupAttrRec(ar, ret_ar);

    /* XXX fix all this */
    *specdFlags |= SPECD_UID | SPECD_GID | SPECD_FILEMODE | SPECD_DIRMODE;
    
    return RPMRC_OK;
}

/**
 * Parse %config from file manifest.
 * @param buf		current spec file line
 * @param fl		package file tree walk data
 * @return		RPMRC_OK on success
 */
static rpmRC parseForConfig(char * buf, FileList fl)
	/*@modifies buf, fl->processingFailed, fl->currentFlags @*/
{
    char *p, *pe, *q;
    const char *name;

    if ((p = strstr(buf, (name = "%config"))) == NULL)
	return RPMRC_OK;

    fl->currentFlags |= RPMFILE_CONFIG;

    /* Erase "%config" token. */
    for (pe = p; (size_t)(pe-p) < strlen(name); pe++)
	*pe = ' ';
    SKIPSPACE(pe);
    if (*pe != '(')
	return RPMRC_OK;

    /* Bracket %config args */
    *pe++ = ' ';
    for (p = pe; *pe && *pe != ')'; pe++)
	{};

    if (*pe == '\0') {
	rpmlog(RPMLOG_ERR, _("Missing ')' in %s(%s\n"), name, p);
	fl->processingFailed = 1;
	return RPMRC_FAIL;
    }

    /* Localize. Erase parsed string. */
    q = alloca((pe-p) + 1);
    strncpy(q, p, pe-p);
    q[pe-p] = '\0';
    while (p <= pe)
	*p++ = ' ';

    for (p = q; *p != '\0'; p = pe) {
	SKIPWHITE(p);
	if (*p == '\0')
	    break;
	pe = p;
	SKIPNONWHITE(pe);
	if (*pe != '\0')
	    *pe++ = '\0';
	if (!strcmp(p, "missingok")) {
	    fl->currentFlags |= RPMFILE_MISSINGOK;
	} else if (!strcmp(p, "noreplace")) {
	    fl->currentFlags |= RPMFILE_NOREPLACE;
	} else {
	    rpmlog(RPMLOG_ERR, _("Invalid %s token: %s\n"), name, p);
	    fl->processingFailed = 1;
	    return RPMRC_FAIL;
	}
    }

    return RPMRC_OK;
}

/**
 */
static int langCmp(const void * ap, const void * bp)
	/*@*/
{
    return strcmp(*(const char **)ap, *(const char **)bp);
}

/**
 * Parse %lang from file manifest.
 * @param buf		current spec file line
 * @param fl		package file tree walk data
 * @return		RPMRC_OK on success
 */
static rpmRC parseForLang(char * buf, FileList fl)
	/*@modifies buf, fl->processingFailed,
		fl->currentLangs, fl->nLangs @*/
{
    char *p, *pe, *q;
    const char *name;

  while ((p = strstr(buf, (name = "%lang"))) != NULL) {

    for (pe = p; (size_t)(pe-p) < strlen(name); pe++)
	*pe = ' ';
    SKIPSPACE(pe);

    if (*pe != '(') {
	rpmlog(RPMLOG_ERR, _("Missing '(' in %s %s\n"), name, pe);
	fl->processingFailed = 1;
	return RPMRC_FAIL;
    }

    /* Bracket %lang args */
    *pe++ = ' ';
    for (pe = p; *pe && *pe != ')'; pe++)
	{};

    if (*pe == '\0') {
	rpmlog(RPMLOG_ERR, _("Missing ')' in %s(%s\n"), name, p);
	fl->processingFailed = 1;
	return RPMRC_FAIL;
    }

    /* Localize. Erase parsed string. */
    q = alloca((pe-p) + 1);
    strncpy(q, p, pe-p);
    q[pe-p] = '\0';
    while (p <= pe)
	*p++ = ' ';

    /* Parse multiple arguments from %lang */
    for (p = q; *p != '\0'; p = pe) {
	char *newp;
	size_t np;
	int i;

	SKIPWHITE(p);
	pe = p;
	SKIPNONWHITE(pe);

	np = pe - p;
	
	/* Sanity check on locale lengths */
	if (np < 1 || (np == 1 && *p != 'C') || np >= 32) {
	    rpmlog(RPMLOG_ERR,
		_("Unusual locale length: \"%.*s\" in %%lang(%s)\n"),
		(int)np, p, q);
	    fl->processingFailed = 1;
	    return RPMRC_FAIL;
	}

	/* Check for duplicate locales */
	if (fl->currentLangs != NULL)
	for (i = 0; i < fl->nLangs; i++) {
	    if (strncmp(fl->currentLangs[i], p, np))
		/*@innercontinue@*/ continue;
	    rpmlog(RPMLOG_ERR, _("Duplicate locale %.*s in %%lang(%s)\n"),
		(int)np, p, q);
	    fl->processingFailed = 1;
	    return RPMRC_FAIL;
	}

	/* Add new locale */
	fl->currentLangs = xrealloc(fl->currentLangs,
				(fl->nLangs + 1) * sizeof(*fl->currentLangs));
	newp = xmalloc( np+1 );
	strncpy(newp, p, np);
	newp[np] = '\0';
	fl->currentLangs[fl->nLangs++] = newp;
	if (*pe == ',') pe++;	/* skip , if present */
    }
  }

    /* Insure that locales are sorted. */
    if (fl->currentLangs)
	qsort(fl->currentLangs, fl->nLangs, sizeof(*fl->currentLangs), langCmp);

    return RPMRC_OK;
}

/**
 */
static int parseForRegexLang(const char * fileName, /*@out@*/ char ** lang)
	/*@globals rpmGlobalMacroContext, h_errno, internalState @*/
	/*@modifies *lang, rpmGlobalMacroContext, internalState @*/
{
    static int initialized = 0;
    static int hasRegex = 0;
    static regex_t compiledPatt;
    static char buf[BUFSIZ];
    int x;
    regmatch_t matches[2];
    const char *s;

    if (! initialized) {
	const char *patt = rpmExpand("%{?_langpatt}", NULL);
	int rc = 0;
	if (!(patt && *patt != '\0'))
	    rc = 1;
	else if (regcomp(&compiledPatt, patt, REG_EXTENDED))
	    rc = -1;
	patt = _free(patt);
	if (rc)
	    return rc;
	hasRegex = 1;
	initialized = 1;
    }
    
    memset(matches, 0, sizeof(matches));
    if (! hasRegex || regexec(&compiledPatt, fileName, 2, matches, REG_NOTEOL))
	return 1;

    /* Got match */
    s = fileName + matches[1].rm_eo - 1;
    x = (int)matches[1].rm_eo - (int)matches[1].rm_so;
    buf[x] = '\0';
    while (x) {
	buf[--x] = *s--;
    }
    if (lang)
	*lang = buf;
    return 0;
}

/**
 */
/*@-exportlocal -exportheadervar@*/
/*@unchecked@*/
static VFA_t virtualFileAttributes[] = {
	{ "%dir",	0,	0 },	/* XXX why not RPMFILE_DIR? */
	{ "%doc",	0,	RPMFILE_DOC },
	{ "%ghost",	0,	RPMFILE_GHOST },
	{ "%exclude",	0,	RPMFILE_EXCLUDE },
	{ "%readme",	0,	RPMFILE_README },
	{ "%license",	0,	RPMFILE_LICENSE },
	{ "%pubkey",	0,	RPMFILE_PUBKEY },
	{ "%policy",	0,	RPMFILE_POLICY },
	{ "%optional",	0,	RPMFILE_OPTIONAL },
	{ "%remove",	0,	RPMFILE_REMOVE },

#if WHY_NOT
	{ "%icon",	0,	RPMFILE_ICON },
	{ "%spec",	0,	RPMFILE_SPEC },
	{ "%config",	0,	RPMFILE_CONFIG },
	{ "%missingok",	0,	RPMFILE_CONFIG|RPMFILE_MISSINGOK },
	{ "%noreplace",	0,	RPMFILE_CONFIG|RPMFILE_NOREPLACE },
#endif

	{ NULL, 0, 0 }
};
/*@=exportlocal =exportheadervar@*/

/**
 * Parse simple attributes (e.g. %dir) from file manifest.
 * @param spec		spec file control structure
 * @param pkg		package control structure
 * @param buf		current spec file line
 * @param fl		package file tree walk data
 * @retval *fileName	file name
 * @return		RPMRC_OK on success
 */
static rpmRC parseForSimple(/*@unused@*/ Spec spec, Package pkg,
		char * buf, FileList fl, /*@out@*/ const char ** fileName)
	/*@globals rpmGlobalMacroContext, h_errno, internalState @*/
	/*@modifies buf, fl->processingFailed, *fileName,
		fl->currentFlags,
		fl->docDirs, fl->docDirCount, fl->isDir,
		fl->passedSpecialDoc, fl->isSpecialDoc,
		pkg->header, pkg->specialDoc,
		rpmGlobalMacroContext, internalState @*/
{
    char *s, *t;
    int specialDoc = 0;
    char specialDocBuf[BUFSIZ];
    rpmRC res = RPMRC_OK;	/* assume success */

    specialDocBuf[0] = '\0';
    *fileName = NULL;

    t = buf;
    while ((s = strtokWithQuotes(t, " \t\n")) != NULL) {
	t = NULL;
	if (!strcmp(s, "%docdir")) {
	    s = strtokWithQuotes(NULL, " \t\n");
	    if (fl->docDirCount >= MAXDOCDIR) {
		rpmlog(RPMLOG_CRIT, _("Hit limit for %%docdir\n"));
		fl->processingFailed = 1;
		res = RPMRC_FAIL;
		goto exit;
	    }
	
	    if (s != NULL)
		fl->docDirs[fl->docDirCount++] = xstrdup(s);
	    if (s == NULL || strtokWithQuotes(NULL, " \t\n")) {
		rpmlog(RPMLOG_CRIT, _("Only one arg for %%docdir\n"));
		fl->processingFailed = 1;
		res = RPMRC_FAIL;
		goto exit;
	    }
	    break;
	}
#if defined(__LCLINT__)
	assert(s != NULL);
#endif

    /* Set flags for virtual file attributes */
    {	VFA_t *vfa;
	for (vfa = virtualFileAttributes; vfa->attribute != NULL; vfa++) {
	    if (strcmp(s, vfa->attribute))
		/*@innercontinue@*/ continue;
	    if (!vfa->flag) {
		if (!strcmp(s, "%dir"))
		    fl->isDir = 1;	/* XXX why not RPMFILE_DIR? */
	    } else {
		if (vfa->not)
		    fl->currentFlags &= ~vfa->flag;
		else
		    fl->currentFlags |= vfa->flag;
	    }

	    /*@innerbreak@*/ break;
	}
	/* if we got an attribute, continue with next token */
	if (vfa->attribute != NULL)
	    continue;
    }

	if (*fileName) {
	    /* We already got a file -- error */
	    rpmlog(RPMLOG_ERR, _("Two files on one line: %s\n"),
		*fileName);
	    fl->processingFailed = 1;
	    res = RPMRC_FAIL;
	    goto exit;
	}

	if (*s != '/') {
	    if (fl->currentFlags & RPMFILE_DOC) {
		specialDoc = 1;
		strcat(specialDocBuf, " ");
		strcat(specialDocBuf, s);
	    } else
	    if (fl->currentFlags & (RPMFILE_POLICY|RPMFILE_PUBKEY|RPMFILE_ICON))
	    {
		*fileName = s;
	    } else {
		const char * sfn = NULL;
		int urltype = urlPath(s, &sfn);
		switch (urltype) {
		default: /* relative path, not in %doc and not a URL */
		    rpmlog(RPMLOG_ERR,
			_("File must begin with \"/\": %s\n"), s);
		    fl->processingFailed = 1;
		    res = RPMRC_FAIL;
		    goto exit;
		    /*@switchbreak@*/ break;
		case URL_IS_PATH:
		    *fileName = s;
		    /*@switchbreak@*/ break;
		}
	    }
	} else {
	    *fileName = s;
	}
    }

    if (specialDoc) {
	if (*fileName || (fl->currentFlags & ~(RPMFILE_DOC))) {
	    rpmlog(RPMLOG_ERR,
		     _("Can't mix special %%doc with other forms: %s\n"),
		     (*fileName ? *fileName : ""));
	    fl->processingFailed = 1;
	    res = RPMRC_FAIL;
	    goto exit;
	} else {
	/* XXX WATCHOUT: buf is an arg */
	   {	
		/*@only@*/
		static char *_docdir_fmt = NULL;	/* XXX memleak */
		static int oneshot = 0;
		const char *ddir, *fmt, *errstr;
		if (!oneshot) {
		    _docdir_fmt = rpmExpand("%{?_docdir_fmt}", NULL);
		    if (!(_docdir_fmt && *_docdir_fmt))
			_docdir_fmt = _free(_docdir_fmt);
		    oneshot = 1;
		}
		if (_docdir_fmt == NULL)
		    _docdir_fmt = xstrdup("%{NAME}-%{VERSION}");
		fmt = headerSprintf(pkg->header, _docdir_fmt, NULL, rpmHeaderFormats, &errstr);
		if (fmt == NULL) {
		    rpmlog(RPMLOG_ERR, _("illegal _docdir_fmt: %s\n"), errstr);
		    fl->processingFailed = 1;
		    res = RPMRC_FAIL;
		    goto exit;
		} else {
		    ddir = rpmGetPath("%{_docdir}/", fmt, NULL);
		    strcpy(buf, ddir);
		    ddir = _free(ddir);
		    fmt = _free(fmt);
		}
	    }

	/* XXX FIXME: this is easy to do as macro expansion */

	    if (! fl->passedSpecialDoc) {
	    	char *compress_doc;
	    	char *mkdir_p;

		pkg->specialDoc = rpmiobNew(0);
		pkg->specialDoc = rpmiobAppend(pkg->specialDoc, "DOCDIR=\"$RPM_BUILD_ROOT\"", 0);
		pkg->specialDoc = rpmiobAppend(pkg->specialDoc, buf, 1);
		pkg->specialDoc = rpmiobAppend(pkg->specialDoc, "export DOCDIR", 1);
		mkdir_p = rpmExpand("%{?__mkdir_p}%{!?__mkdir_p:mkdir -p}", NULL);
		if (!mkdir_p)
		    mkdir_p = xstrdup("mkdir -p");
		pkg->specialDoc = rpmiobAppend(pkg->specialDoc, mkdir_p, 0);
		mkdir_p = _free(mkdir_p);
		pkg->specialDoc = rpmiobAppend(pkg->specialDoc, " \"$DOCDIR\"", 1);

		compress_doc = rpmExpand("%{__compress_doc}", NULL);
		if (compress_doc && *compress_doc != '%')
	    	    pkg->specialDoc = rpmiobAppend(pkg->specialDoc, compress_doc, 1);
		compress_doc = _free(compress_doc);

		/*@-temptrans@*/
		*fileName = buf;
		/*@=temptrans@*/
		fl->passedSpecialDoc = 1;
		fl->isSpecialDoc = 1;
	    }

	    pkg->specialDoc = rpmiobAppend(pkg->specialDoc, "cp -pr ", 0);
	    pkg->specialDoc = rpmiobAppend(pkg->specialDoc, specialDocBuf, 0);
	    pkg->specialDoc = rpmiobAppend(pkg->specialDoc, " \"$DOCDIR\"", 1);
	}
    }

exit:
    return res;
}

/**
 */
static int compareFileListRecs(const void * ap, const void * bp)	/*@*/
{
    const char *aurl = ((FileListRec)ap)->fileURL;
    const char *a = NULL;
    const char *burl = ((FileListRec)bp)->fileURL;
    const char *b = NULL;
    (void) urlPath(aurl, &a);
    (void) urlPath(burl, &b);
    return strcmp(a, b);
}

/**
 * Test if file is located in a %docdir.
 * @param fl		package file tree walk data
 * @param fileName	file path
 * @return		1 if doc file, 0 if not
 */
static int isDoc(FileList fl, const char * fileName)	/*@*/
{
    int x = fl->docDirCount;
    size_t k, l;

    k = strlen(fileName);
    while (x--) {
	l = strlen(fl->docDirs[x]);
	if (l < k && strncmp(fileName, fl->docDirs[x], l) == 0 && fileName[l] == '/')
	    return 1;
    }
    return 0;
}

/**
 * Verify that file attributes scope over hardlinks correctly.
 * If partial hardlink sets are possible, then add tracking dependency.
 * @param fl		package file tree walk data
 * @return		1 if partial hardlink sets can exist, 0 otherwise.
 */
static int checkHardLinks(FileList fl)
	/*@*/
{
    FileListRec ilp, jlp;
    int i, j;

    for (i = 0;  i < fl->fileListRecsUsed; i++) {
	ilp = fl->fileList + i;
	if (!(S_ISREG(ilp->fl_mode) && ilp->fl_nlink > 1))
	    continue;
	if (ilp->flags & (RPMFILE_EXCLUDE | RPMFILE_GHOST))
	    continue;

	for (j = i + 1; j < fl->fileListRecsUsed; j++) {
	    jlp = fl->fileList + j;
	    if (!S_ISREG(jlp->fl_mode))
		/*@innercontinue@*/ continue;
	    if (ilp->fl_nlink != jlp->fl_nlink)
		/*@innercontinue@*/ continue;
	    if (ilp->fl_ino != jlp->fl_ino)
		/*@innercontinue@*/ continue;
	    if (ilp->fl_dev != jlp->fl_dev)
		/*@innercontinue@*/ continue;
	    if (jlp->flags & (RPMFILE_EXCLUDE | RPMFILE_GHOST))
		/*@innercontinue@*/ continue;
	    return 1;
	}
    }
    return 0;
}

static int dncmp(const void * a, const void * b)
	/*@*/
{
    const char ** aurlp = (const char **)a;
    const char ** burlp = (const char **)b;
    const char * adn;
    const char * bdn;
    (void) urlPath(*aurlp, &adn);
    (void) urlPath(*burlp, &bdn);
    return strcmp(adn, bdn);
}

/**
 * Convert absolute path tag to (dirname,basename,dirindex) tags.
 * @param h             header
 */
static void compressFilelist(Header h)
	/*@globals internalState @*/
	/*@modifies h, internalState @*/
{
    HE_t he = (HE_t) memset(alloca(sizeof(*he)), 0, sizeof(*he));
    const char ** fileNames;
    const char * fn;
    const char ** dirNames;
    const char ** baseNames;
    rpmuint32_t * dirIndexes;
    int count;
    int dirIndex = -1;
    int xx;
    int i;

    /*
     * This assumes the file list is already sorted, and begins with a
     * single '/'. That assumption isn't critical, but it makes things go
     * a bit faster.
     */

    if (headerIsEntry(h, RPMTAG_DIRNAMES)) {
	he->tag = RPMTAG_OLDFILENAMES;
	xx = headerDel(h, he, 0);
	return;		/* Already converted. */
    }

    he->tag = RPMTAG_OLDFILENAMES;
    xx = headerGet(h, he, 0);
    fileNames = he->p.argv;
    count = he->c;
    if (!xx || fileNames == NULL || count <= 0)
	return;		/* no file list */

    dirNames = alloca(sizeof(*dirNames) * count);	/* worst case */
    baseNames = alloca(sizeof(*dirNames) * count);
    dirIndexes = alloca(sizeof(*dirIndexes) * count);

    (void) urlPath(fileNames[0], &fn);
    if (fn[0] != '/') {
	/* HACK. Source RPM, so just do things differently */
	dirIndex = 0;
	dirNames[dirIndex] = "";
	for (i = 0; i < count; i++) {
	    dirIndexes[i] = dirIndex;
	    baseNames[i] = fileNames[i];
	}
	goto exit;
    }

    for (i = 0; i < count; i++) {
	const char ** needle;
	char savechar;
	char * baseName;
	size_t len;

	if (fileNames[i] == NULL)	/* XXX can't happen */
	    continue;
	baseName = strrchr(fileNames[i], '/') + 1;
	len = baseName - fileNames[i];
	needle = dirNames;
	savechar = *baseName;
	*baseName = '\0';
/*@-compdef@*/
	if (dirIndex < 0 ||
	    (needle = bsearch(&fileNames[i], dirNames, dirIndex + 1, sizeof(dirNames[0]), dncmp)) == NULL) {
	    char *s = alloca(len + 1);
	    memcpy(s, fileNames[i], len + 1);
	    s[len] = '\0';
	    dirIndexes[i] = ++dirIndex;
	    dirNames[dirIndex] = s;
	} else
	    dirIndexes[i] = needle - dirNames;
/*@=compdef@*/

	*baseName = savechar;
	baseNames[i] = baseName;
    }

exit:
    if (count > 0) {
	he->tag = RPMTAG_DIRINDEXES;
	he->t = RPM_UINT32_TYPE;
	he->p.ui32p = dirIndexes;
	he->c = count;
	xx = headerPut(h, he, 0);

	he->tag = RPMTAG_BASENAMES;
	he->t = RPM_STRING_ARRAY_TYPE;
	he->p.argv = baseNames;
	he->c = count;
	xx = headerPut(h, he, 0);

	he->tag = RPMTAG_DIRNAMES;
	he->t = RPM_STRING_ARRAY_TYPE;
	he->p.argv = dirNames;
	he->c = dirIndex + 1;
	xx = headerPut(h, he, 0);
    }

    fileNames = _free(fileNames);

    he->tag = RPMTAG_OLDFILENAMES;
    xx = headerDel(h, he, 0);
}

static rpmuint32_t getDigestAlgo(Header h, int isSrc)
	/*@modifies h @*/
{
    HE_t he = (HE_t) memset(alloca(sizeof(*he)), 0, sizeof(*he));
    static rpmuint32_t source_file_dalgo = 0;
    static rpmuint32_t binary_file_dalgo = 0;
    static int oneshot = 0;
    rpmuint32_t dalgo = 0;
    int xx;

    if (!oneshot) {
	source_file_dalgo =
		rpmExpandNumeric("%{?_build_source_file_digest_algo}");
	binary_file_dalgo =
		rpmExpandNumeric("%{?_build_binary_file_digest_algo}");
	oneshot++;
    }

    dalgo = (isSrc ? source_file_dalgo : binary_file_dalgo);
    switch (dalgo) {
    case PGPHASHALGO_SHA1:
    case PGPHASHALGO_MD2:
    case PGPHASHALGO_SHA256:
    case PGPHASHALGO_SHA384:
    case PGPHASHALGO_SHA512:
	(void) rpmlibNeedsFeature(h, "FileDigests", "4.6.0-1");
	he->tag = RPMTAG_FILEDIGESTALGO;
	he->t = RPM_UINT32_TYPE;
	he->p.ui32p = &dalgo;
	he->c = 1;
	xx = headerPut(h, he, 0);
	/*@fallthgrough@*/
    case PGPHASHALGO_RIPEMD160:
    case PGPHASHALGO_TIGER192:
    case PGPHASHALGO_MD4:
    case PGPHASHALGO_RIPEMD128:
    case PGPHASHALGO_CRC32:
    case PGPHASHALGO_ADLER32:
    case PGPHASHALGO_CRC64:
	(void) rpmlibNeedsFeature(h, "FileDigestParameterized", "4.4.6-1");
	    /*@switchbreak@*/ break;
    case PGPHASHALGO_MD5:
    case PGPHASHALGO_HAVAL_5_160:		/* XXX unimplemented */
    default:
	dalgo = PGPHASHALGO_MD5;
	/*@switchbreak@*/ break;
    }

    return dalgo;
}

/**
 * Add file entries to header.
 * @todo Should directories have %doc/%config attributes? (#14531)
 * @todo Remove RPMTAG_OLDFILENAMES, add dirname/basename instead.
 * @param fl		package file tree walk data
 * @retval *fip		file info for package
 * @param h
 * @param isSrc
 */
static void genCpioListAndHeader(/*@partial@*/ FileList fl,
		rpmfi * fip, Header h, int isSrc)
	/*@globals rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
	/*@modifies h, *fip, fl->processingFailed, fl->fileList,
		fl->totalFileSize,
		rpmGlobalMacroContext, fileSystem, internalState @*/
{
    HE_t he = (HE_t) memset(alloca(sizeof(*he)), 0, sizeof(*he));
    const char * apath;
    rpmuint16_t ui16;
    rpmuint32_t ui32;
    int _addDotSlash = !isSrc;
    int apathlen = 0;
    int dpathlen = 0;
    int skipLen = 0;
    rpmsx sx = rpmsxNew("%{?_build_file_context_path}", 0);
    FileListRec flp;
    rpmuint32_t dalgo = getDigestAlgo(h, isSrc);
    char buf[BUFSIZ];
    int i, xx;

memset(buf, 0, sizeof(buf));	/* XXX valgrind on rhel6 beta pickier */

    /* Sort the big list */
    if (fl->fileListRecsUsed > 1)
	qsort(fl->fileList, fl->fileListRecsUsed,
		sizeof(*(fl->fileList)), compareFileListRecs);
    
    /* Generate the header. */
    if (! isSrc) {
	skipLen = 1;
	if (fl->prefix)
	    skipLen += strlen(fl->prefix);
    }

    for (i = 0, flp = fl->fileList; i < fl->fileListRecsUsed; i++, flp++) {
	const char *s;

 	/* Merge duplicate entries. */
	while (i < (fl->fileListRecsUsed - 1) &&
	    !strcmp(flp->fileURL, flp[1].fileURL)) {

	    /* Two entries for the same file found, merge the entries. */
	    /* Note that an %exclude is a duplication of a file reference */

	    /* file flags */
	    flp[1].flags |= flp->flags;	

	    if (!(flp[1].flags & RPMFILE_EXCLUDE))
		rpmlog(RPMLOG_WARNING, _("File listed twice: %s\n"),
			flp->fileURL);
   
	    /* file mode */
	    if (S_ISDIR(flp->fl_mode)) {
		if ((flp[1].specdFlags & (SPECD_DIRMODE | SPECD_DEFDIRMODE)) <
		    (flp->specdFlags & (SPECD_DIRMODE | SPECD_DEFDIRMODE)))
			flp[1].fl_mode = flp->fl_mode;
	    } else {
		if ((flp[1].specdFlags & (SPECD_FILEMODE | SPECD_DEFFILEMODE)) <
		    (flp->specdFlags & (SPECD_FILEMODE | SPECD_DEFFILEMODE)))
			flp[1].fl_mode = flp->fl_mode;
	    }

	    /* uid */
	    if ((flp[1].specdFlags & (SPECD_UID | SPECD_DEFUID)) <
		(flp->specdFlags & (SPECD_UID | SPECD_DEFUID)))
	    {
		flp[1].fl_uid = flp->fl_uid;
		flp[1].uname = flp->uname;
	    }

	    /* gid */
	    if ((flp[1].specdFlags & (SPECD_GID | SPECD_DEFGID)) <
		(flp->specdFlags & (SPECD_GID | SPECD_DEFGID)))
	    {
		flp[1].fl_gid = flp->fl_gid;
		flp[1].gname = flp->gname;
	    }

	    /* verify flags */
	    if ((flp[1].specdFlags & (SPECD_VERIFY | SPECD_DEFVERIFY)) <
		(flp->specdFlags & (SPECD_VERIFY | SPECD_DEFVERIFY)))
		    flp[1].verifyFlags = flp->verifyFlags;

	    /* XXX to-do: language */

	    flp++; i++;
	}

	/* Skip files that were marked with %exclude. */
	if (flp->flags & RPMFILE_EXCLUDE) continue;

	/* Omit '/' and/or URL prefix, leave room for "./" prefix */
	(void) urlPath(flp->fileURL, &apath);
	apathlen += (strlen(apath) - skipLen + (_addDotSlash ? 3 : 1));

	/* Leave room for both dirname and basename NUL's */
	dpathlen += (strlen(flp->diskURL) + 2);

	/*
	 * Make the header, the OLDFILENAMES will get converted to a 
	 * compressed file list write before we write the actual package to
	 * disk.
	 */
	he->tag = RPMTAG_OLDFILENAMES;
	he->t = RPM_STRING_ARRAY_TYPE;
	he->p.argv = &flp->fileURL;
	he->c = 1;
	he->append = 1;
	xx = headerPut(h, he, 0);
	he->append = 0;

/*@-sizeoftype@*/
	ui32 = (rpmuint32_t) flp->fl_size;
	he->tag = RPMTAG_FILESIZES;
	he->t = RPM_UINT32_TYPE;
	he->p.ui32p = &ui32;
	he->c = 1;
	he->append = 1;
	xx = headerPut(h, he, 0);
	he->append = 0;

	he->tag = RPMTAG_FILEUSERNAME;
	he->t = RPM_STRING_ARRAY_TYPE;
	he->p.argv = &flp->uname;
	he->c = 1;
	he->append = 1;
	xx = headerPut(h, he, 0);
	he->append = 0;

	he->tag = RPMTAG_FILEGROUPNAME;
	he->t = RPM_STRING_ARRAY_TYPE;
	he->p.argv = &flp->gname;
	he->c = 1;
	he->append = 1;
	xx = headerPut(h, he, 0);
	he->append = 0;

	ui32 = (rpmuint32_t) flp->fl_mtime;
	he->tag = RPMTAG_FILEMTIMES;
	he->t = RPM_UINT32_TYPE;
	he->p.ui32p = &ui32;
	he->c = 1;
	he->append = 1;
	xx = headerPut(h, he, 0);
	he->append = 0;

	ui16 = (rpmuint16_t)flp->fl_mode;
	he->tag = RPMTAG_FILEMODES;
	he->t = RPM_UINT16_TYPE;
	he->p.ui16p = &ui16;
	he->c = 1;
	he->append = 1;
	xx = headerPut(h, he, 0);
	he->append = 0;

	{   dev_t _dev = flp->fl_rdev;
	    ui16 = _dev;
	}
	he->tag = RPMTAG_FILERDEVS;
	he->t = RPM_UINT16_TYPE;
	he->p.ui16p = &ui16;
	he->c = 1;
	he->append = 1;
	xx = headerPut(h, he, 0);
	he->append = 0;

	/* XXX Hash instead of 64b->32b truncate to prevent aliasing. */
	{   dev_t _dev = flp->fl_dev;
	    ui32 = hashFunctionString(0, &_dev, sizeof(_dev));
	}
	he->tag = RPMTAG_FILEDEVICES;
	he->t = RPM_UINT32_TYPE;
	he->p.ui32p = &ui32;
	he->c = 1;
	he->append = 1;
	xx = headerPut(h, he, 0);
	he->append = 0;

	/* XXX Hash instead of 64b->32b truncate to prevent aliasing. */
	{   ino_t _ino = flp->fl_ino;
	    ui32 = hashFunctionString(0, &_ino, sizeof(_ino));
	}
	he->tag = RPMTAG_FILEINODES;
	he->t = RPM_UINT32_TYPE;
	he->p.ui32p = &ui32;
	he->c = 1;
	he->append = 1;
	xx = headerPut(h, he, 0);
	he->append = 0;

/*@=sizeoftype@*/

	he->tag = RPMTAG_FILELANGS;
	he->t = RPM_STRING_ARRAY_TYPE;
	he->p.argv = &flp->langs;
	he->c = 1;
	he->append = 1;
	xx = headerPut(h, he, 0);
	he->append = 0;

	buf[0] = '\0';
	if (S_ISREG(flp->fl_mode)) {
	    unsigned dflags = 0x01;	/* asAscii */
#define	_mask	(RPMVERIFY_FDIGEST|RPMVERIFY_HMAC)
	    if ((flp->verifyFlags & _mask) == RPMVERIFY_HMAC)
		dflags |= 0x02;		/* doHmac */
#undef	_mask
	    (void) dodigest(dalgo, flp->diskURL, (unsigned char *)buf,
			dflags, NULL);
	}
	s = buf;

	he->tag = RPMTAG_FILEDIGESTS;
	he->t = RPM_STRING_ARRAY_TYPE;
	he->p.argv = &s;
	he->c = 1;
	he->append = 1;
	xx = headerPut(h, he, 0);
	he->append = 0;

if (!(_rpmbuildFlags & 4)) {
	ui32 = dalgo;
	he->tag = RPMTAG_FILEDIGESTALGOS;
	he->t = RPM_UINT32_TYPE;
	he->p.ui32p = &ui32;
	he->c = 1;
	he->append = 1;
	xx = headerPut(h, he, 0);
	he->append = 0;
}
	
	buf[0] = '\0';
	if (S_ISLNK(flp->fl_mode)) {
	    xx = Readlink(flp->diskURL, buf, BUFSIZ);
	    if (xx >= 0)
		buf[xx] = '\0';
	    if (fl->buildRootURL) {
		const char * buildRoot;
		(void) urlPath(fl->buildRootURL, &buildRoot);

		if (buf[0] == '/' && strcmp(buildRoot, "/") &&
		    !strncmp(buf, buildRoot, strlen(buildRoot))) {
		     rpmlog(RPMLOG_ERR,
				_("Symlink points to BuildRoot: %s -> %s\n"),
				flp->fileURL, buf);
		    fl->processingFailed = 1;
		}
	    }
	}
	s = buf;
	he->tag = RPMTAG_FILELINKTOS;
	he->t = RPM_STRING_ARRAY_TYPE;
	he->p.argv = &s;
	he->c = 1;
	he->append = 1;
	xx = headerPut(h, he, 0);
	he->append = 0;

	if (flp->flags & RPMFILE_GHOST) {
	    flp->verifyFlags &= ~(RPMVERIFY_FDIGEST | RPMVERIFY_FILESIZE |
				RPMVERIFY_LINKTO | RPMVERIFY_MTIME);
	}
	ui32 = flp->verifyFlags;
	he->tag = RPMTAG_FILEVERIFYFLAGS;
	he->t = RPM_UINT32_TYPE;
	he->p.ui32p = &ui32;
	he->c = 1;
	he->append = 1;
	xx = headerPut(h, he, 0);
	he->append = 0;
	
	if (!isSrc && isDoc(fl, flp->fileURL))
	    flp->flags |= RPMFILE_DOC;
	/* XXX Should directories have %doc/%config attributes? (#14531) */
	if (S_ISDIR(flp->fl_mode))
	    flp->flags &= ~(RPMFILE_CONFIG|RPMFILE_DOC);

	ui32 = flp->flags;
	he->tag = RPMTAG_FILEFLAGS;
	he->t = RPM_UINT32_TYPE;
	he->p.ui32p = &ui32;
	he->c = 1;
	he->append = 1;
	xx = headerPut(h, he, 0);
	he->append = 0;
	
	/* Add file security context to package. */
	if (sx && sx->fn  && *sx->fn && !(_rpmbuildFlags & 4)) {
	    const char * scon = rpmsxMatch(sx, flp->fileURL, flp->fl_mode);
	    if (scon) {
		he->tag = RPMTAG_FILECONTEXTS;
		he->t = RPM_STRING_ARRAY_TYPE;
		he->p.argv = &scon;
		he->c = 1;
		he->append = 1;
		xx = headerPut(h, he, 0);
		he->append = 0;
	    }
	    scon = _free(scon);		/* XXX freecon(scon) instead()? */
	}
    }

    sx = rpmsxFree(sx);

if (_rpmbuildFlags & 4) {
(void) rpmlibNeedsFeature(h, "PayloadFilesHavePrefix", "4.0-1");
(void) rpmlibNeedsFeature(h, "CompressedFileNames", "3.0.4-1");
}
	
    compressFilelist(h);

  { static int scareMem = 0;
    void * ts = NULL;	/* XXX FIXME drill rpmts ts all the way down here */
    rpmfi fi = rpmfiNew(ts, h, RPMTAG_BASENAMES, scareMem);
    char * a, * d;

    if (fi == NULL) return;		/* XXX can't happen */

/*@-onlytrans@*/
    fi->te = xcalloc(1, sizeof(*((rpmte)fi->te)));
/*@=onlytrans@*/
    ((rpmte)fi->te)->type = TR_ADDED;

    fi->dnl = _free(fi->dnl);
    fi->bnl = _free(fi->bnl);
    if (!scareMem) fi->dil = _free(fi->dil);

    /* XXX Insure at least 1 byte is always allocated. */
    fi->dnl = xmalloc(fi->fc * sizeof(*fi->dnl) + dpathlen + 1);
    d = (char *)(fi->dnl + fi->fc);
    *d = '\0';

    fi->bnl = xmalloc(fi->fc * (sizeof(*fi->bnl) + sizeof(*fi->dil)));
/*@-dependenttrans@*/ /* FIX: artifact of spoofing header tag store */
    fi->dil = (!scareMem)
	? xcalloc(sizeof(*fi->dil), fi->fc)
	: (rpmuint32_t *)(fi->bnl + fi->fc);
/*@=dependenttrans@*/

    /* XXX Insure at least 1 byte is always allocated. */
    fi->apath = xmalloc(fi->fc * sizeof(*fi->apath) + apathlen + 1);
    a = (char *)(fi->apath + fi->fc);
    *a = '\0';

    fi->actions = _free(fi->actions);			/* XXX memory leak */
    fi->actions = xcalloc(sizeof(*fi->actions), fi->fc);
    fi->fmapflags = xcalloc(sizeof(*fi->fmapflags), fi->fc);
    fi->astriplen = 0;
    if (fl->buildRootURL)
	fi->astriplen = strlen(fl->buildRootURL);
    fi->striplen = 0;
    fi->fuser = _free(fi->fuser);
    fi->fgroup = _free(fi->fgroup);

    /* Make the cpio list */
    if (fi->dil != NULL)	/* XXX can't happen */
    for (i = 0, flp = fl->fileList; (unsigned)i < fi->fc; i++, flp++) {
	char * b;

	/* Skip (possible) duplicate file entries, use last entry info. */
	while (((flp - fl->fileList) < (fl->fileListRecsUsed - 1)) &&
		!strcmp(flp->fileURL, flp[1].fileURL))
	    flp++;

	if (flp->flags & RPMFILE_EXCLUDE) {
	    i--;
	    continue;
	}

	{
	    /* this fi uses diskURL (with buildroot), not fileURL */
	    size_t fnlen = strlen(flp->diskURL);
	    if (fnlen > fi->fnlen) {
		/* fnlen-sized buffer must not be allocated yet */
		assert(fi->fn == NULL);
		fi->fnlen = fnlen;
	    }
	}

	/* Create disk directory and base name. */
	fi->dil[i] = i;
/*@-dependenttrans@*/ /* FIX: artifact of spoofing header tag store */
	fi->dnl[fi->dil[i]] = d;
/*@=dependenttrans@*/
	d = stpcpy(d, flp->diskURL);

	/* Make room for the dirName NUL, find start of baseName. */
	for (b = d; b > fi->dnl[fi->dil[i]] && *b != '/'; b--)
	    b[1] = b[0];
	b++;		/* dirname's end in '/' */
	*b++ = '\0';	/* terminate dirname, b points to basename */
	fi->bnl[i] = b;
	d += 2;		/* skip both dirname and basename NUL's */

	/* Create archive path, normally adding "./" */
	/*@-dependenttrans@*/	/* FIX: xstrdup? nah ... */
	fi->apath[i] = a;
 	/*@=dependenttrans@*/
	if (_addDotSlash)
	    a = stpcpy(a, "./");
	(void) urlPath(flp->fileURL, &apath);
	a = stpcpy(a, (apath + skipLen));
	a++;		/* skip apath NUL */

	if (flp->flags & RPMFILE_GHOST) {
	    fi->actions[i] = FA_SKIP;
	    continue;
	}
	fi->actions[i] = FA_COPYOUT;
	fi->fmapflags[i] = IOSM_MAP_PATH |
		IOSM_MAP_TYPE | IOSM_MAP_MODE | IOSM_MAP_UID | IOSM_MAP_GID;
	if (isSrc)
	    fi->fmapflags[i] |= IOSM_FOLLOW_SYMLINKS;

	if (S_ISREG(flp->fl_mode)) {
	    int bingo = 1;
	    /* Hard links need be tallied only once. */
	    if (flp->fl_nlink > 1) {
		FileListRec jlp = flp + 1;
		int j = i + 1;
		for (; (unsigned)j < fi->fc; j++, jlp++) {
		    /* follow outer loop logic */
		    while (((jlp - fl->fileList) < (fl->fileListRecsUsed - 1)) &&
			    !strcmp(jlp->fileURL, jlp[1].fileURL))
			jlp++;
		    if (jlp->flags & RPMFILE_EXCLUDE) {
			j--;
			/*@innercontinue@*/ continue;
		    }
		    if (jlp->flags & RPMFILE_GHOST)
		        /*@innercontinue@*/ continue;
		    if (!S_ISREG(jlp->fl_mode))
			/*@innercontinue@*/ continue;
		    if (flp->fl_nlink != jlp->fl_nlink)
			/*@innercontinue@*/ continue;
		    if (flp->fl_ino != jlp->fl_ino)
			/*@innercontinue@*/ continue;
		    if (flp->fl_dev != jlp->fl_dev)
			/*@innercontinue@*/ continue;
		    bingo = 0;	/* don't tally hardlink yet. */
		    /*@innerbreak@*/ break;
		}
	    }
	    if (bingo)
		fl->totalFileSize += flp->fl_size;
	}
    }

    ui32 = fl->totalFileSize;
    he->tag = RPMTAG_SIZE;
    he->t = RPM_UINT32_TYPE;
    he->p.ui32p = &ui32;
    he->c = 1;
    xx = headerPut(h, he, 0);

    /*@-compdef@*/
    if (fip)
	*fip = fi;
    else
	fi = rpmfiFree(fi);
    /*@=compdef@*/
  }
}

/**
 */
static /*@null@*/ FileListRec freeFileList(/*@only@*/ FileListRec fileList,
			int count)
	/*@*/
{
    while (count--) {
	fileList[count].diskURL = _free(fileList[count].diskURL);
	fileList[count].fileURL = _free(fileList[count].fileURL);
	fileList[count].langs = _free(fileList[count].langs);
    }
    fileList = _free(fileList);
    return NULL;
}

/* forward ref */
static rpmRC recurseDir(FileList fl, const char * diskURL)
	/*@globals rpmGlobalMacroContext, h_errno,
		fileSystem, internalState @*/
	/*@modifies *fl, fl->processingFailed,
		fl->fileList, fl->fileListRecsAlloced, fl->fileListRecsUsed,
		fl->totalFileSize, fl->fileCount, fl->inFtw, fl->isDir,
		rpmGlobalMacroContext,
		fileSystem, internalState @*/;

/**
 * Add a file to the package manifest.
 * @param fl		package file tree walk data
 * @param diskURL	path to file
 * @param statp		file stat (possibly NULL)
 * @return		RPMRC_OK on success
 */
static int addFile(FileList fl, const char * diskURL,
		/*@null@*/ struct stat * statp)
	/*@globals rpmGlobalMacroContext, h_errno,
		fileSystem, internalState @*/
	/*@modifies *statp, *fl, fl->processingFailed,
		fl->fileList, fl->fileListRecsAlloced, fl->fileListRecsUsed,
		fl->totalFileSize, fl->fileCount,
		rpmGlobalMacroContext,
		fileSystem, internalState @*/
{
    const char *fn = xstrdup(diskURL);
    const char *fileURL = fn;
    struct stat statbuf;
    mode_t fileMode;
    uid_t fileUid;
    gid_t fileGid;
    const char *fileUname;
    const char *fileGname;
    char *lang;
    rpmRC rc = RPMRC_OK;
    
    /* Path may have prepended buildRootURL, so locate the original filename. */
    /*
     * XXX There are 3 types of entry into addFile:
     *
     *	From			diskUrl			statp
     *	=====================================================
     *  processBinaryFile	path			NULL
     *  processBinaryFile	glob result path	NULL
     *  recurseDir		path			stat
     *
     */
    {	const char *fileName;
	int urltype = urlPath(fileURL, &fileName);
	switch (urltype) {
	case URL_IS_PATH:
	    fileURL += (fileName - fileURL);
	    if (fl->buildRootURL && strcmp(fl->buildRootURL, "/")) {
		size_t nb = strlen(fl->buildRootURL);
		const char * s = fileURL + nb;
		char * t = (char *) fileURL;
		(void) memmove(t, s, nb);
	    }
	    fileURL = fn;
	    break;
	default:
	    if (fl->buildRootURL && strcmp(fl->buildRootURL, "/"))
		fileURL += strlen(fl->buildRootURL);
	    break;
	}
    }

    /* XXX make sure '/' can be packaged also */
    if (*fileURL == '\0')
	fileURL = "/";

    /* If we are using a prefix, validate the file */
    if (!fl->inFtw && fl->prefix) {
	const char *prefixTest;
	const char *prefixPtr = fl->prefix;

	(void) urlPath(fileURL, &prefixTest);
	while (*prefixPtr && *prefixTest && (*prefixTest == *prefixPtr)) {
	    prefixPtr++;
	    prefixTest++;
	}
	if (*prefixPtr || (*prefixTest && *prefixTest != '/')) {
	    rpmlog(RPMLOG_ERR, _("File doesn't match prefix (%s): %s\n"),
		     fl->prefix, fileURL);
	    fl->processingFailed = 1;
	    rc = RPMRC_FAIL;
	    goto exit;
	}
    }

    if (statp == NULL) {
	statp = &statbuf;
	memset(statp, 0, sizeof(*statp));
	if (fl->devtype) {
	    time_t now = time(NULL);

	    /* XXX hack up a stat structure for a %dev(...) directive. */
	    statp->st_nlink = 1;
	    statp->st_rdev =
		((fl->devmajor & 0xff) << 8) | (fl->devminor & 0xff);
	    statp->st_dev = statp->st_rdev;
	    statp->st_mode = (fl->devtype == 'b' ? S_IFBLK : S_IFCHR);
	    statp->st_mode |= (fl->cur_ar.ar_fmode & 0777);
	    statp->st_atime = now;
	    statp->st_mtime = now;
	    statp->st_ctime = now;
	} else if (Lstat(diskURL, statp)) {
	    if (fl->currentFlags & RPMFILE_OPTIONAL) {
		rpmlog(RPMLOG_WARNING, _("Optional file not found: %s\n"), diskURL);
		rc = RPMRC_OK;
	    } else {
		rpmlog(RPMLOG_ERR, _("File not found: %s\n"), diskURL);
		fl->processingFailed = 1;
		rc = RPMRC_FAIL;
	    }
	    goto exit;
	}
    }

    if ((! fl->isDir) && S_ISDIR(statp->st_mode)) {
/*@-nullstate@*/ /* FIX: fl->buildRootURL may be NULL */
	rc = recurseDir(fl, diskURL);
	goto exit;
/*@=nullstate@*/
    }

    fileMode = statp->st_mode;
    fileUid = statp->st_uid;
    fileGid = statp->st_gid;

    if (S_ISDIR(fileMode) && fl->cur_ar.ar_dmodestr) {
	fileMode &= S_IFMT;
	fileMode |= fl->cur_ar.ar_dmode;
    } else if (fl->cur_ar.ar_fmodestr != NULL) {
	fileMode &= S_IFMT;
	fileMode |= fl->cur_ar.ar_fmode;
    }
    if (fl->cur_ar.ar_user) {
	fileUname = getUnameS(fl->cur_ar.ar_user);
    } else {
	fileUname = getUname(fileUid);
    }
    if (fl->cur_ar.ar_group) {
	fileGname = getGnameS(fl->cur_ar.ar_group);
    } else {
	fileGname = getGname(fileGid);
    }
	
    /* Default user/group to builder's user/group */
    if (fileUname == NULL)
	fileUname = getUname(getuid());
    if (fileGname == NULL)
	fileGname = getGname(getgid());
    
    /* Add to the file list */
    if (fl->fileListRecsUsed == fl->fileListRecsAlloced) {
	fl->fileListRecsAlloced += 128;
	fl->fileList = xrealloc(fl->fileList,
			fl->fileListRecsAlloced * sizeof(*(fl->fileList)));
    }
	    
    {	FileListRec flp = &fl->fileList[fl->fileListRecsUsed];
	int i;

	flp->fl_st = *statp;	/* structure assignment */
	flp->fl_mode = fileMode;
	flp->fl_uid = fileUid;
	flp->fl_gid = fileGid;

	flp->fileURL = xstrdup(fileURL);
	flp->diskURL = xstrdup(diskURL);
	flp->uname = fileUname;
	flp->gname = fileGname;

	if (fl->currentLangs && fl->nLangs > 0) {
	    char * ncl;
	    size_t nl = 0;
	    
	    for (i = 0; i < fl->nLangs; i++)
		nl += strlen(fl->currentLangs[i]) + 1;

	    flp->langs = ncl = xmalloc(nl);
	    for (i = 0; i < fl->nLangs; i++) {
	        const char *ocl;
		if (i)	*ncl++ = '|';
		for (ocl = fl->currentLangs[i]; *ocl != '\0'; ocl++)
			*ncl++ = *ocl;
		*ncl = '\0';
	    }
	} else if (! parseForRegexLang(fileURL, &lang)) {
	    flp->langs = xstrdup(lang);
	} else {
	    flp->langs = xstrdup("");
	}

	flp->flags = fl->currentFlags;
	flp->specdFlags = fl->currentSpecdFlags;
	flp->verifyFlags = fl->currentVerifyFlags;
    }

    fl->fileListRecsUsed++;
    fl->fileCount++;

exit:
/*@i@*/ fn = _free(fn);
    return rc;
}

/**
 * Add directory (and all of its files) to the package manifest.
 * @param fl		package file tree walk data
 * @param diskURL	path to file
 * @return		RPMRC_OK on success
 */
static rpmRC recurseDir(FileList fl, const char * diskURL)
{
    char * ftsSet[2];
    FTS * ftsp;
    FTSENT * fts;
    int myFtsOpts = (FTS_COMFOLLOW | FTS_NOCHDIR | FTS_PHYSICAL);
    rpmRC rc = RPMRC_FAIL;

    fl->inFtw = 1;  /* Flag to indicate file has buildRootURL prefixed */
    fl->isDir = 1;  /* Keep it from following myftw() again         */

    ftsSet[0] = (char *) diskURL;
    ftsSet[1] = NULL;
    ftsp = Fts_open(ftsSet, myFtsOpts, NULL);
    while ((fts = Fts_read(ftsp)) != NULL) {
	switch (fts->fts_info) {
	case FTS_D:		/* preorder directory */
	case FTS_F:		/* regular file */
	case FTS_SL:		/* symbolic link */
	case FTS_SLNONE:	/* symbolic link without target */
	case FTS_DEFAULT:	/* none of the above */
	    rc = addFile(fl, fts->fts_accpath, fts->fts_statp);
	    /*@switchbreak@*/ break;
	case FTS_DOT:		/* dot or dot-dot */
	case FTS_DP:		/* postorder directory */
	    rc = RPMRC_OK;
	    /*@switchbreak@*/ break;
	case FTS_NS:		/* stat(2) failed */
	case FTS_DNR:		/* unreadable directory */
	case FTS_ERR:		/* error; errno is set */
	case FTS_DC:		/* directory that causes cycles */
	case FTS_NSOK:		/* no stat(2) requested */
	case FTS_INIT:		/* initialized only */
	case FTS_W:		/* whiteout object */
	default:
	    rc = RPMRC_FAIL;
	    /*@switchbreak@*/ break;
	}
	if (rc != RPMRC_OK)
	    break;
    }
    (void) Fts_close(ftsp);

    fl->isDir = 0;
    fl->inFtw = 0;

    return rc;
}

/**
 * Add a pubkey/policy/icon to a binary package.
 * @param pkg		package control structure
 * @param fl		package file tree walk data
 * @param fileURL	path to file, relative is builddir, absolute buildroot.
 * @param tag		tag to add
 * @return		RPMRC_OK on success
 */
static rpmRC processMetadataFile(Package pkg, FileList fl, const char * fileURL,
		rpmTag tag)
	/*@globals rpmGlobalMacroContext, h_errno,
		fileSystem, internalState @*/
	/*@modifies pkg->header, *fl, fl->processingFailed,
		fl->fileList, fl->fileListRecsAlloced, fl->fileListRecsUsed,
		fl->totalFileSize, fl->fileCount,
		rpmGlobalMacroContext,
		fileSystem, internalState @*/
{
    HE_t he = (HE_t) memset(alloca(sizeof(*he)), 0, sizeof(*he));
    const char * buildURL = "%{_builddir}/%{?buildsubdir}/";
    const char * fn = NULL;
    const char * apkt = NULL;
    rpmiob iob = NULL;
    rpmuint8_t * pkt = NULL;
    ssize_t pktlen = 0;
    int absolute = 0;
    rpmRC rc = RPMRC_FAIL;
    int xx;

    (void) urlPath(fileURL, &fn);
    if (*fn == '/') {
	fn = rpmGenPath(fl->buildRootURL, NULL, fn);
	absolute = 1;
    } else
	fn = rpmGenPath(buildURL, NULL, fn);

    switch (tag) {
    default:
	rpmlog(RPMLOG_ERR, _("%s: can't load unknown tag (%d).\n"),
		fn, tag);
	goto exit;
	/*@notreached@*/ break;
    case RPMTAG_PUBKEYS:
	if ((xx = pgpReadPkts(fn, &pkt, (size_t *)&pktlen)) <= 0) {
	    rpmlog(RPMLOG_ERR, _("%s: public key read failed.\n"), fn);
	    goto exit;
	}
	if (xx != PGPARMOR_PUBKEY) {
	    rpmlog(RPMLOG_ERR, _("%s: not an armored public key.\n"), fn);
	    goto exit;
	}
	apkt = pgpArmorWrap(PGPARMOR_PUBKEY, pkt, pktlen);
	break;
    case RPMTAG_POLICIES:
	xx = rpmiobSlurp(fn, &iob);
	if (!(xx == 0 && iob != NULL)) {
	    rpmlog(RPMLOG_ERR, _("%s: *.te policy read failed.\n"), fn);
	    goto exit;
	}
	apkt = (const char *) iob->b;	/* XXX unsigned char */
	/* XXX steal the I/O buffer */
	iob->b = (rpmuint8_t *)xcalloc(1, sizeof(*iob->b));
	iob->blen = 0;
	break;
    }

    he->tag = tag;
    he->t = RPM_STRING_ARRAY_TYPE;
    he->p.argv = &apkt;
    he->c = 1;
    he->append = 1;
    xx = headerPut(pkg->header, he, 0);
    he->append = 0;

    rc = RPMRC_OK;
    if (absolute)
	rc = addFile(fl, fn, NULL);

exit:
    apkt = _free(apkt);
    pkt = _free(pkt);
    iob = rpmiobFree(iob);
    fn = _free(fn);
    if (rc != RPMRC_OK)
	fl->processingFailed = 1;
    return rc;
}

/**
 * Add a file to a binary package.
 * @param pkg		package control structure
 * @param fl		package file tree walk data
 * @param fileURL
 * @return		RPMRC_OK on success
 */
static rpmRC processBinaryFile(/*@unused@*/ Package pkg, FileList fl,
		const char * fileURL)
	/*@globals rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
	/*@modifies *fl, fl->processingFailed,
		fl->fileList, fl->fileListRecsAlloced, fl->fileListRecsUsed,
		fl->totalFileSize, fl->fileCount,
		rpmGlobalMacroContext, fileSystem, internalState @*/
{
    int quote = 1;	/* XXX permit quoted glob characters. */
    int doGlob;
    const char *diskURL = NULL;
    rpmRC rc = RPMRC_OK;
    int xx;
    
    doGlob = Glob_pattern_p(fileURL, quote);

    /* Check that file starts with leading "/" */
    {	const char * fileName;
	(void) urlPath(fileURL, &fileName);
	if (*fileName != '/') {
	    rpmlog(RPMLOG_ERR, _("File needs leading \"/\": %s\n"),
			fileName);
	    rc = RPMRC_FAIL;
	    goto exit;
	}
    }
    
    /* Copy file name or glob pattern removing multiple "/" chars. */
    /*
     * Note: rpmGetPath should guarantee a "canonical" path. That means
     * that the following pathologies should be weeded out:
     *		//bin//sh
     *		//usr//bin/
     *		/.././../usr/../bin//./sh
     */
    diskURL = rpmGenPath(fl->buildRootURL, NULL, fileURL);

    if (doGlob) {
	const char ** argv = NULL;
	int argc = 0;
	int i;

	/* XXX for %dev marker in file manifest only */
	if (fl->noGlob) {
	    rpmlog(RPMLOG_ERR, _("Glob not permitted: %s\n"),
			diskURL);
	    rc = RPMRC_FAIL;
	    goto exit;
	}

	xx = rpmGlob(diskURL, &argc, &argv);
	if (xx == 0 && argc >= 1) {
	    for (i = 0; i < argc; i++) {
		rc = addFile(fl, argv[i], NULL);
		argv[i] = _free(argv[i]);
	    }
	    argv = _free(argv);
	} else {
	    if (fl->currentFlags & RPMFILE_OPTIONAL) {
		rpmlog(RPMLOG_WARNING, _("Optional file not found by glob: %s\n"),
			    diskURL);
		rc = RPMRC_OK;
	    } else {
		rpmlog(RPMLOG_ERR, _("File not found by glob: %s\n"),
			    diskURL);
		rc = RPMRC_FAIL;
	    }
	    goto exit;
	}
    } else
	rc = addFile(fl, diskURL, NULL);

exit:
    diskURL = _free(diskURL);
    if (rc != RPMRC_OK)
	fl->processingFailed = 1;
    return rc;
}

/**
 */
static rpmRC processPackageFiles(Spec spec, Package pkg,
			       int installSpecialDoc, int test)
	/*@globals rpmGlobalMacroContext, h_errno,
		fileSystem, internalState@*/
	/*@modifies spec->macros,
		pkg->fi, pkg->fileList, pkg->specialDoc, pkg->header,
		rpmGlobalMacroContext, fileSystem, internalState @*/
{
    HE_t he = (HE_t) memset(alloca(sizeof(*he)), 0, sizeof(*he));
    struct FileList_s fl;
    ARGV_t files = NULL;
    ARGV_t fp;
    const char *fileName;
    char buf[BUFSIZ];
    struct AttrRec_s arbuf;
    AttrRec specialDocAttrRec = &arbuf;
    char *specialDoc = NULL;
    int xx;

    nullAttrRec(specialDocAttrRec);
    pkg->fi = NULL;

    if (pkg->fileFile) {
	char *saveptr = NULL;
	char *filesFiles = xstrdup(pkg->fileFile);
/*@-unrecog@*/
	char *token = strtok_r(filesFiles, ",", &saveptr);
/*@=unrecog@*/
	do {
	    const char *ffn;
	    FILE * f;
	    FD_t fd;

	    /* XXX W2DO? urlPath might be useful here. */
	    if (*token == '/') {
		ffn = rpmGetPath(token, NULL);
	    } else {
		/* XXX FIXME: add %{buildsubdir} */
		ffn = rpmGetPath("%{_builddir}/",
		    (spec->buildSubdir ? spec->buildSubdir : "") ,
		    "/", token, NULL);
	    }

	    fd = Fopen(ffn, "r.fpio");

	    if (fd == NULL || Ferror(fd)) {
		rpmlog(RPMLOG_ERR,
		    _("Could not open %%files file %s: %s\n"),
		    ffn, Fstrerror(fd));
		ffn = _free(ffn);
		filesFiles = _free(filesFiles);
	        return RPMRC_FAIL;
	    }
	    ffn = _free(ffn);

	    /*@+voidabstract@*/ f = fdGetFp(fd); /*@=voidabstract@*/
	    if (f != NULL) {
		while (fgets(buf, (int)sizeof(buf), f)) {
		    handleComments(buf);
		    if (expandMacros(spec, spec->macros, buf, sizeof(buf))) {
			rpmlog(RPMLOG_ERR, _("line: %s\n"), buf);
			filesFiles = _free(filesFiles);
			return RPMRC_FAIL;
	    	    }
	    	    pkg->fileList = rpmiobAppend(pkg->fileList, buf, 0);
		}
	    }
	    (void) Fclose(fd);
	} while((token = strtok_r(NULL, ",", &saveptr)) != NULL);
	filesFiles = _free(filesFiles);
    }
    
    /* Init the file list structure */
    memset(&fl, 0, sizeof(fl));

    fl.buildRootURL = rpmGenPath(spec->rootURL, "%{?buildroot}", NULL);

    he->tag = RPMTAG_DEFAULTPREFIX;
    xx = headerGet(pkg->header, he, 0);
    fl.prefix = he->p.str;

    fl.fileCount = 0;
    fl.totalFileSize = 0;
    fl.processingFailed = 0;

    fl.passedSpecialDoc = 0;
    fl.isSpecialDoc = 0;

    fl.isDir = 0;
    fl.inFtw = 0;
    fl.currentFlags = 0;
    fl.currentVerifyFlags = 0;
    
    fl.noGlob = 0;
    fl.devtype = 0;
    fl.devmajor = 0;
    fl.devminor = 0;

    nullAttrRec(&fl.cur_ar);
    nullAttrRec(&fl.def_ar);
    dupAttrRec(&root_ar, &fl.def_ar);	/* XXX assume %defattr(-,root,root) */

    fl.defVerifyFlags = RPMVERIFY_ALL;
    fl.nLangs = 0;
    fl.currentLangs = NULL;

    fl.currentSpecdFlags = 0;
    fl.defSpecdFlags = 0;

    fl.docDirCount = 0;
#if defined(RPM_VENDOR_OPENPKG) /* no-default-doc-files */
    /* do not declare any files as %doc files by default. */
#else
    fl.docDirs[fl.docDirCount++] = xstrdup("/usr/doc");
    fl.docDirs[fl.docDirCount++] = xstrdup("/usr/man");
    fl.docDirs[fl.docDirCount++] = xstrdup("/usr/info");
    fl.docDirs[fl.docDirCount++] = xstrdup("/usr/X11R6/man");
    fl.docDirs[fl.docDirCount++] = xstrdup("/usr/share/doc");
    fl.docDirs[fl.docDirCount++] = xstrdup("/usr/share/man");
    fl.docDirs[fl.docDirCount++] = xstrdup("/usr/share/info");
    fl.docDirs[fl.docDirCount++] = xstrdup("/usr/src/examples");
    fl.docDirs[fl.docDirCount++] = rpmGetPath("%{_docdir}", NULL);
    fl.docDirs[fl.docDirCount++] = rpmGetPath("%{_mandir}", NULL);
    fl.docDirs[fl.docDirCount++] = rpmGetPath("%{_infodir}", NULL);
    fl.docDirs[fl.docDirCount++] = rpmGetPath("%{_javadocdir}", NULL);
    fl.docDirs[fl.docDirCount++] = rpmGetPath("%{_examplesdir}", NULL);
#endif
    
    fl.fileList = NULL;
    fl.fileListRecsAlloced = 0;
    fl.fileListRecsUsed = 0;

    xx = argvSplit(&files, rpmiobStr(pkg->fileList), "\n");

    for (fp = files; *fp != NULL; fp++) {
	const char * s;
	s = *fp;
	SKIPSPACE(s);
	if (*s == '\0')
	    continue;
	fileName = NULL;
	/*@-nullpass@*/	/* LCL: buf is NULL ?!? */
	strncpy(buf, s, sizeof(buf)-1);
	buf[sizeof(buf)-1] = '\0';
	/*@=nullpass@*/
	
	/* Reset for a new line in %files */
	fl.isDir = 0;
	fl.inFtw = 0;
	fl.currentFlags = 0;
	/* turn explicit flags into %def'd ones (gosh this is hacky...) */
	fl.currentSpecdFlags = ((unsigned)fl.defSpecdFlags) >> 8;
	fl.currentVerifyFlags = fl.defVerifyFlags;
	fl.isSpecialDoc = 0;

	fl.noGlob = 0;
 	fl.devtype = 0;
 	fl.devmajor = 0;
 	fl.devminor = 0;

	/* XXX should reset to %deflang value */
	if (fl.currentLangs) {
	    int i;
	    for (i = 0; i < fl.nLangs; i++)
		/*@-unqualifiedtrans@*/
		fl.currentLangs[i] = _free(fl.currentLangs[i]);
		/*@=unqualifiedtrans@*/
	    fl.currentLangs = _free(fl.currentLangs);
	}
  	fl.nLangs = 0;

	dupAttrRec(&fl.def_ar, &fl.cur_ar);

	/*@-nullpass@*/	/* LCL: buf is NULL ?!? */
	if (parseForVerify(buf, &fl) != RPMRC_OK)
	    continue;
	if (parseForAttr(buf, &fl) != RPMRC_OK)
	    continue;
	if (parseForDev(buf, &fl) != RPMRC_OK)
	    continue;
	if (parseForConfig(buf, &fl) != RPMRC_OK)
	    continue;
	if (parseForLang(buf, &fl) != RPMRC_OK)
	    continue;
	/*@-nullstate@*/	/* FIX: pkg->fileFile might be NULL */
	if (parseForSimple(spec, pkg, buf, &fl, &fileName) != RPMRC_OK)
	/*@=nullstate@*/
	    continue;
	/*@=nullpass@*/
	if (fileName == NULL)
	    continue;

	if (fl.isSpecialDoc) {
	    /* Save this stuff for last */
	    specialDoc = _free(specialDoc);
	    specialDoc = xstrdup(fileName);
	    dupAttrRec(&fl.cur_ar, specialDocAttrRec);
	} else if (fl.currentFlags & RPMFILE_PUBKEY) {
/*@-nullstate@*/	/* FIX: pkg->fileFile might be NULL */
	    (void) processMetadataFile(pkg, &fl, fileName, RPMTAG_PUBKEYS);
/*@=nullstate@*/
	} else if (fl.currentFlags & RPMFILE_POLICY) {
/*@-nullstate@*/	/* FIX: pkg->fileFile might be NULL */
	    (void) processMetadataFile(pkg, &fl, fileName, RPMTAG_POLICIES);
/*@=nullstate@*/
	} else {
/*@-nullstate@*/	/* FIX: pkg->fileFile might be NULL */
	    (void) processBinaryFile(pkg, &fl, fileName);
/*@=nullstate@*/
	}
    }

    /* Now process special doc, if there is one */
    if (specialDoc) {
	if (installSpecialDoc) {
	    int _missing_doc_files_terminate_build =
		    rpmExpandNumeric("%{?_missing_doc_files_terminate_build}");
	    rpmRC rc;

	    rc = doScript(spec, RPMBUILD_STRINGBUF, "%doc", pkg->specialDoc, test);
	    if (rc != RPMRC_OK && _missing_doc_files_terminate_build)
		fl.processingFailed = 1;
	}

	/* Reset for %doc */
	fl.isDir = 0;
	fl.inFtw = 0;
	fl.currentFlags = 0;
	fl.currentVerifyFlags = fl.defVerifyFlags;

	fl.noGlob = 0;
 	fl.devtype = 0;
 	fl.devmajor = 0;
 	fl.devminor = 0;

	/* XXX should reset to %deflang value */
	if (fl.currentLangs) {
	    int i;
	    for (i = 0; i < fl.nLangs; i++)
		/*@-unqualifiedtrans@*/
		fl.currentLangs[i] = _free(fl.currentLangs[i]);
		/*@=unqualifiedtrans@*/
	    fl.currentLangs = _free(fl.currentLangs);
	}
  	fl.nLangs = 0;

	dupAttrRec(specialDocAttrRec, &fl.cur_ar);
	freeAttrRec(specialDocAttrRec);

	/*@-nullstate@*/	/* FIX: pkg->fileFile might be NULL */
	(void) processBinaryFile(pkg, &fl, specialDoc);
	/*@=nullstate@*/

	specialDoc = _free(specialDoc);
    }
    
    files = argvFree(files);

    if (fl.processingFailed)
	goto exit;

    /* Verify that file attributes scope over hardlinks correctly. */
    if (checkHardLinks(&fl))
	(void) rpmlibNeedsFeature(pkg->header,
			"PartialHardlinkSets", "4.0.4-1");

    /* XXX should tags be added if filelist is empty? */
    genCpioListAndHeader(&fl, &pkg->fi, pkg->header, 0);

    if (spec->timeCheck)
	timeCheck(spec->timeCheck, pkg->header);
    
exit:
    fl.buildRootURL = _free(fl.buildRootURL);
    fl.prefix = _free(fl.prefix);

    freeAttrRec(&fl.cur_ar);
    freeAttrRec(&fl.def_ar);

    if (fl.currentLangs) {
	int i;
	for (i = 0; i < fl.nLangs; i++)
	    /*@-unqualifiedtrans@*/
	    fl.currentLangs[i] = _free(fl.currentLangs[i]);
	    /*@=unqualifiedtrans@*/
	fl.currentLangs = _free(fl.currentLangs);
    }

    fl.fileList = freeFileList(fl.fileList, fl.fileListRecsUsed);
    while (fl.docDirCount--)
	fl.docDirs[fl.docDirCount] = _free(fl.docDirs[fl.docDirCount]);
    return (fl.processingFailed ? RPMRC_FAIL : RPMRC_OK);
}

int initSourceHeader(Spec spec, rpmiob *sfp)
{
    HE_t he = (HE_t) memset(alloca(sizeof(*he)), 0, sizeof(*he));
    HeaderIterator hi;
    rpmiob sourceFiles;
    struct Source *srcPtr;
    static rpmTag classTag = 0xffffffff;
    int xx;
    size_t i;

    if (classTag == 0xffffffff)
	classTag = tagValue("Class");

    /* Only specific tags are added to the source package header */
  if (spec->packages && !spec->sourceHdrInit) {
    for (hi = headerInit(spec->packages->header);
	headerNext(hi, he, 0);
	he->p.ptr = _free(he->p.ptr))
    {
	switch (he->tag) {
	case RPMTAG_NAME:
	case RPMTAG_VERSION:
	case RPMTAG_RELEASE:
	case RPMTAG_DISTEPOCH:
	case RPMTAG_EPOCH:
	case RPMTAG_SUMMARY:
	case RPMTAG_DESCRIPTION:
	case RPMTAG_PACKAGER:
	case RPMTAG_DISTRIBUTION:
	case RPMTAG_DISTURL:
	case RPMTAG_VENDOR:
	case RPMTAG_LICENSE:
	case RPMTAG_GROUP:
	case RPMTAG_OS:
	case RPMTAG_ARCH:
	case RPMTAG_CHANGELOGTIME:
	case RPMTAG_CHANGELOGNAME:
	case RPMTAG_CHANGELOGTEXT:
	case RPMTAG_URL:
	case RPMTAG_BUGURL:
	case RPMTAG_ICON:
	case RPMTAG_GIF:
	case RPMTAG_XPM:
	case HEADER_I18NTABLE:
#if defined(RPM_VENDOR_OPENPKG) /* propagate-provides-to-srpms */
	/* make sure the "Provides" headers are available for querying from the .src.rpm files. */
	case RPMTAG_PROVIDENAME:
	case RPMTAG_PROVIDEVERSION:
	case RPMTAG_PROVIDEFLAGS:
#endif
	    if (he->p.ptr)
		xx = headerPut(spec->sourceHeader, he, 0);
	    /*@switchbreak@*/ break;
	default:
	    if (classTag == he->tag && he->p.ptr != NULL)
		xx = headerPut(spec->sourceHeader, he, 0);
	    /*@switchbreak@*/ break;
	}
    }
    hi = headerFini(hi);

    if (spec->BANames && spec->BACount > 0) {
	he->tag = RPMTAG_BUILDARCHS;
	he->t = RPM_STRING_ARRAY_TYPE;
	he->p.argv = spec->BANames;
	he->c = spec->BACount;
	xx = headerPut(spec->sourceHeader, he, 0);
    }

    /* Load arbitrary tags into srpm header. */
    if (spec->foo)
    for (i = 0; i < spec->nfoo; i++) {
	const char * str = spec->foo[i].str;
	rpmTag tag = spec->foo[i].tag;
	rpmiob iob = spec->foo[i].iob;
	char * s;

	if (str == NULL || iob == NULL)
	    continue;

	/* XXX Special case %track interpreter for now. */
	if (!xstrcasecmp(str, "track")) {
	    he->p.str = rpmExpand("%{?__vcheck}", NULL);
	    if (!(he->p.str != NULL && he->p.str[0] != '\0')) {
		he->p.str = _free(he->p.str);
		continue;
	    }
	    he->tag = tagValue("Trackprog");
	    he->t = RPM_STRING_TYPE;
	    he->c = 1;
	    xx = headerPut(spec->sourceHeader, he, 0);
	    he->p.str = _free(he->p.str);
	}

	s = rpmiobStr(iob);
	he->tag = tag;
	he->append = headerIsEntry(spec->sourceHeader, tag);
	if (he->append) {
	    he->t = RPM_STRING_ARRAY_TYPE;
	    he->p.argv = (const char **) &s;
	    he->c = 1;
	} else {
	    he->t = RPM_STRING_TYPE;
	    he->p.str = s;
	    he->c = 1;
	}
	xx = headerPut(spec->sourceHeader, he, 0);
	he->append = 0;
    }
  }

    if (sfp != NULL && *sfp != NULL)
	sourceFiles = *sfp;
    else
	sourceFiles = rpmiobNew(0);

    /* Construct the source/patch tag entries */
    if (spec->specFile != NULL)	/* XXX segfault avoidance */
	sourceFiles = rpmiobAppend(sourceFiles, spec->specFile, 1);
    if (spec->sourceHeader != NULL)
    for (srcPtr = spec->sources; srcPtr != NULL; srcPtr = srcPtr->next) {
      {	const char * sfn;
/*@-nullpass@*/		/* XXX getSourceDir returns NULL with bad flags. */
	sfn = rpmGetPath( ((srcPtr->flags & RPMFILE_GHOST) ? "!" : ""),
#if defined(RPM_VENDOR_OPENPKG) /* splitted-source-directory */
		getSourceDir(srcPtr->flags, srcPtr->source), srcPtr->source, NULL);
#else
		getSourceDir(srcPtr->flags), srcPtr->source, NULL);
#endif
/*@=nullpass@*/
	sourceFiles = rpmiobAppend(sourceFiles, sfn, 1);
	sfn = _free(sfn);
      }

	if (spec->sourceHdrInit)
	    continue;

	if (srcPtr->flags & RPMFILE_SOURCE) {
	    he->tag = RPMTAG_SOURCE;
	    he->t = RPM_STRING_ARRAY_TYPE;
	    he->p.argv = &srcPtr->source;
	    he->c = 1;
	    he->append = 1;
	    xx = headerPut(spec->sourceHeader, he, 0);
	    he->append = 0;
	    if (srcPtr->flags & RPMFILE_GHOST) {
		he->tag = RPMTAG_NOSOURCE;
		he->t = RPM_UINT32_TYPE;
		he->p.ui32p = &srcPtr->num;
		he->c = 1;
		he->append = 1;
		xx = headerPut(spec->sourceHeader, he, 0);
		he->append = 0;
	    }
	}
	if (srcPtr->flags & RPMFILE_PATCH) {
	    he->tag = RPMTAG_PATCH;
	    he->t = RPM_STRING_ARRAY_TYPE;
	    he->p.argv = &srcPtr->source;
	    he->c = 1;
	    he->append = 1;
	    xx = headerPut(spec->sourceHeader, he, 0);
	    he->append = 0;
	    if (srcPtr->flags & RPMFILE_GHOST) {
		he->tag = RPMTAG_NOPATCH;
		he->t = RPM_UINT32_TYPE;
		he->p.ui32p = &srcPtr->num;
		he->c = 1;
		he->append = 1;
		xx = headerPut(spec->sourceHeader, he, 0);
		he->append = 0;
	    }
	}
    }

    if (sfp == NULL)
	sourceFiles = rpmiobFree(sourceFiles);

    spec->sourceHdrInit = 1;

/*@-usereleased@*/
    return 0;
/*@=usereleased@*/
}

int processSourceFiles(Spec spec)
{
    rpmiob sourceFiles, *sfp = &sourceFiles;
    int x, isSpec = 1;
    struct FileList_s fl;
    ARGV_t files = NULL;
    ARGV_t fp;
    int rc;
    /* srcdefattr: needed variables */
    char _srcdefattr_buf[BUFSIZ];
    char * _srcdefattr = rpmExpand("%{?_srcdefattr}", NULL);
    int xx;

    *sfp = rpmiobNew(0);
    x = initSourceHeader(spec, sfp);

    /* srcdefattr: initialize file list structure */
    memset(&fl, 0, sizeof(fl));
    if (_srcdefattr && *_srcdefattr) {
        xx = snprintf(_srcdefattr_buf, sizeof(_srcdefattr_buf), "%%defattr %s", _srcdefattr);
	_srcdefattr_buf[sizeof(_srcdefattr_buf)-1] = '\0';
        xx = parseForAttr(_srcdefattr_buf, &fl);
    }

    /* Construct the SRPM file list. */
    fl.fileList = xcalloc((spec->numSources + 1), sizeof(*fl.fileList));
    rc = fl.processingFailed = 0;
    fl.fileListRecsUsed = 0;
    fl.totalFileSize = 0;
    fl.prefix = NULL;
    fl.buildRootURL = NULL;

    xx = argvSplit(&files, rpmiobStr(*sfp), "\n");

    /* The first source file is the spec file */
    x = 0;
    for (fp = files; *fp != NULL; fp++) {
	const char * diskURL, *diskPath;
	FileListRec flp;

	diskURL = *fp;
	SKIPSPACE(diskURL);
	if (! *diskURL)
	    continue;

	flp = &fl.fileList[x];

	flp->flags = isSpec ? RPMFILE_SPECFILE : 0;
	/* files with leading ! are no source files */
	if (*diskURL == '!') {
	    flp->flags |= RPMFILE_GHOST;
	    diskURL++;
	}

	(void) urlPath(diskURL, &diskPath);

	flp->diskURL = xstrdup(diskURL);
	diskPath = strrchr(diskPath, '/');
	if (diskPath)
	    diskPath++;
	else
	    diskPath = diskURL;

	flp->fileURL = xstrdup(diskPath);
	flp->verifyFlags = RPMVERIFY_ALL;

	if (Stat(diskURL, &flp->fl_st)) {
	    rpmlog(RPMLOG_ERR, _("Bad file: %s: %s\n"),
		diskURL, strerror(errno));
	    rc = fl.processingFailed = 1;
	}

#if defined(RPM_VENDOR_OPENPKG) || defined(RPM_VENDOR_MANDRIVA) /* support-srcdefattr */
	/* srcdefattr: allow to set SRPM file attributes via %{_srcdefattr} macro */
	if (fl.def_ar.ar_fmodestr) {
	    flp->fl_mode &= S_IFMT;
	    flp->fl_mode |= fl.def_ar.ar_fmode;
	}
        flp->uname = fl.def_ar.ar_user  ? getUnameS(fl.def_ar.ar_user)  : getUname(flp->fl_uid);
	flp->gname = fl.def_ar.ar_group ? getGnameS(fl.def_ar.ar_group) : getGname(flp->fl_gid);
#else
	flp->uname = getUname(flp->fl_uid);
	flp->gname = getGname(flp->fl_gid);
#endif
	flp->langs = xstrdup("");
	
	if (! (flp->uname && flp->gname)) {
	    rpmlog(RPMLOG_ERR, _("Bad owner/group: %s\n"), diskURL);
	    rc = fl.processingFailed = 1;
	}

	isSpec = 0;
	x++;
    }
    fl.fileListRecsUsed = x;
    files = argvFree(files);

    if (rc)
	goto exit;

    /* XXX should tags be added if filelist is empty? */
    spec->fi = NULL;
    genCpioListAndHeader(&fl, &spec->fi, spec->sourceHeader, 1);

exit:
    *sfp = rpmiobFree(*sfp);
    fl.fileList = freeFileList(fl.fileList, fl.fileListRecsUsed);
    _srcdefattr = _free(_srcdefattr);
    return rc;
}

/**
 * Check for unpackaged files against what's in the build root.
 * @param spec		spec file control structure
 * @return		-1 if skipped, 0 on OK, 1 on error
 */
static int checkUnpackagedFiles(Spec spec)
	/*@globals rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
	/*@modifies *spec->packages,
		rpmGlobalMacroContext, fileSystem, internalState @*/
{
/*@-readonlytrans@*/
    static const char * av_ckfile[] = { "%{?__check_files}", NULL };
/*@=readonlytrans@*/
    rpmiob iob_stdout = NULL;
    const char * s;
    int rc;
    rpmiob fileList = NULL;
    Package pkg;
    int n = 0;
    
    s = rpmExpand(av_ckfile[0], NULL);
    if (!(s && *s)) {
	rc = -1;
	goto exit;
    }
    rc = 0;

    /* initialize fileList */
    fileList = rpmiobNew(0);
    for (pkg = spec->packages; pkg != NULL; pkg = pkg->next) {
	int i;
#ifndef	DYING	/* XXX rpmfiNew is necessary here. why?!? */
	rpmfi fi = rpmfiNew(NULL, pkg->header, RPMTAG_BASENAMES, 0);
#else
	rpmfi fi = rpmfiLink(pkg->fi, __FUNCTION__);
#endif
	fi = rpmfiInit(fi, 0);
	while ((i = rpmfiNext(fi)) >= 0) {
	    const char *fn = rpmfiFN(fi);
	    fileList = rpmiobAppend(fileList, fn, 1);
	    n++;
	}
	fi = rpmfiFree(fi);
    }

    if (n == 0) {
	/* no packaged files, and buildroot may not exist -
	 * no need to run check */
	rc = -1;
	goto exit;
    }

    rpmlog(RPMLOG_NOTICE, _("Checking for unpackaged file(s): %s\n"), s);

    rc = rpmfcExec(av_ckfile, fileList, &iob_stdout, 0);
    if (rc < 0)
	goto exit;
    
    if (iob_stdout) {
	int _unpackaged_files_terminate_build =
		rpmExpandNumeric("%{?_unpackaged_files_terminate_build}");
	const char * t;

	t = rpmiobStr(iob_stdout);
	if ((*t != '\0') && (*t != '\n')) {
	    rc = (_unpackaged_files_terminate_build) ? 1 : 0;
	    rpmlog((rc ? RPMLOG_ERR : RPMLOG_WARNING),
		_("Installed (but unpackaged) file(s) found:\n%s"), t);
	}
    }
    
exit:
    fileList = rpmiobFree(fileList);
    iob_stdout = rpmiobFree(iob_stdout);
    s = _free(s);
    return rc;
}

/* auxiliary function for checkDuplicateFiles() */
/* XXX need to pass Header because fi->h is NULL */
static int fiIntersect(/*@null@*/ rpmfi fi1, /*@null@*/ rpmfi fi2)
	/*@globals internalState @*/
	/*@modifies fi1, fi2, internalState @*/
{
    int n = 0;
    int i1, i2;
    const char *fn1, *fn2;
    rpmiob dups = NULL;

    if ((fi1 = rpmfiInit(fi1, 0)) != NULL)
    while ((i1 = rpmfiNext(fi1)) >= 0) {
	if (S_ISDIR(rpmfiFMode(fi1)))
	    continue;
	fn1 = rpmfiFN(fi1);
	if ((fi2 = rpmfiInit(fi2, 0)) != NULL)
	while ((i2 = rpmfiNext(fi2)) >= 0) {
	    if (S_ISDIR(rpmfiFMode(fi2)))
		/*@innercontinue@*/ continue;
	    fn2 = rpmfiFN(fi2);
	    if (strcmp(fn1, fn2))
		/*@innercontinue@*/ continue;
	    if (!dups)
		dups = rpmiobNew(0);
	    dups = rpmiobAppend(dups, "\t", 0);
	    dups = rpmiobAppend(dups, fn1, 1);
	    n++;
	}
    }

    if (n > 0) {
	const char *N1, *N2;
	HE_t he = (HE_t) memset(alloca(sizeof(*he)), 0, sizeof(*he));

	he->tag = RPMTAG_NVRA;
	N1 = (headerGet(fi1->h, he, 0) ? he->p.str : NULL);
	he->tag = RPMTAG_NVRA;
	N2 = (headerGet(fi2->h, he, 0) ? he->p.str : NULL);

	rpmlog(RPMLOG_WARNING,
	       _("File(s) packaged into both %s and %s:\n%s"),
	       N1, N2, rpmiobStr(dups));

	N1 = _free(N1);
	N2 = _free(N2);
	dups = rpmiobFree(dups);
    }

    return n;
}

/**
 * Check if the same files are packaged into a few sub-packages.
 * @param spec		spec file control structure
 * @return		number of duplicate files
 */
static int checkDuplicateFiles(Spec spec)
	/*@globals rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
	/*@modifies *spec->packages,
		rpmGlobalMacroContext, fileSystem, internalState @*/
{
    int n = 0;
    Package pkg1, pkg2;

    if (spec->packages)	/* XXX segfault avoidance */
    for (pkg1 = spec->packages; pkg1->next; pkg1 = pkg1->next) {
#ifdef	DYING
	rpmfi fi1 = rpmfiNew(NULL, pkg1->header, RPMTAG_BASENAMES, 0);
#else
	rpmfi fi1 = rpmfiLink(pkg1->fi, __FUNCTION__);
#endif
	if (fi1 == NULL) continue;
	(void) rpmfiSetHeader(fi1, pkg1->header);
	for (pkg2 = pkg1->next; pkg2; pkg2 = pkg2->next) {
#ifdef	DYING
	    rpmfi fi2 = rpmfiNew(NULL, pkg2->header, RPMTAG_BASENAMES, 0);
#else
	    rpmfi fi2 = rpmfiLink(pkg2->fi, __FUNCTION__);
#endif
	    if (fi2 == NULL) continue;
	    (void) rpmfiSetHeader(fi2, pkg2->header);
	    n += fiIntersect(fi1, fi2);
	    (void) rpmfiSetHeader(fi2, NULL);
	    fi2 = rpmfiFree(fi2);
	}
	(void) rpmfiSetHeader(fi1, NULL);
	fi1 = rpmfiFree(fi1);
    }
    return n;
}

/* auxiliary function: check if directory d is packaged */
static inline int packagedDir(Package pkg, const char *d)
	/*@globals rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
	/*@modifies pkg->header,
		rpmGlobalMacroContext, fileSystem, internalState @*/
{
    return rpmbfChk(rpmfiFNBF(pkg->fi), d, strlen(d));
}

/* auxiliary function: find unpackaged subdirectories
 *
 * E.g. consider this %files section:
 *       %dir /A
 *       /A/B/C/D
 * Now directories "/A/B" and "/A/B/C" should also be packaged.
 */
static int pkgUnpackagedSubdirs(Package pkg)
	/*@globals rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
	/*@modifies pkg->header,
		rpmGlobalMacroContext, fileSystem, internalState @*/
{
    int n = 0;
    int i, j;
    char **unpackaged = NULL;
    char *fn;
#ifdef	DYING
    rpmfi fi = rpmfiNew(NULL, pkg->header, RPMTAG_BASENAMES, 0);
#else
    rpmfi fi = rpmfiLink(pkg->fi, __FUNCTION__);
#endif

    if (rpmfiFC(fi) <= 1) {
	fi = rpmfiFree(fi);
	return 0;
    }
    fn = alloca(rpmfiFNMaxLen(fi) + 1);

    fi = rpmfiInit(fi, 0);
    while ((i = rpmfiNext(fi)) >= 0) {
	int found = 0;
	/* make local copy of file name */
	char *p = fn;
	strcpy(fn, rpmfiFN(fi));
	/* find the first path component that is packaged */
	while ((p = strchr(p + 1, '/'))) {
	    *p = '\0';
	    found = packagedDir(pkg, fn);
	    *p = '/';
	    if (found)
		/*@innerbreak@*/ break;
	}
	if (!found)
	    continue;
	/* other path components should be packaged, too */
	if (p != NULL)
	while ((p = strchr(p + 1, '/'))) {
	    *p = '\0';
	    if (packagedDir(pkg, fn)) {
		*p = '/';
		/*@innercontinue@*/ continue;
	    }
	    /* might be already added */
	    found = 0;
	    for (j = 0; j < n; j++)
		if (strcmp(fn, unpackaged[j]) == 0) {
		    found = 1;
		    /*@innerbreak@*/ break;
		}
	    if (found) {
		*p = '/';
		/*@innercontinue@*/ continue;
	    }
	    unpackaged = xrealloc(unpackaged, sizeof(*unpackaged) * (n + 1));
	    unpackaged[n++] = xstrdup(fn);
	    *p = '/';
	}
    }
    fi = rpmfiFree(fi);

    if (n > 0) {
	const char *N;
	HE_t he = (HE_t) memset(alloca(sizeof(*he)), 0, sizeof(*he));
	rpmiob list = rpmiobNew(0);

	he->tag = RPMTAG_NVRA;
	N = (headerGet(pkg->header, he, 0) ? he->p.str : NULL);

	for (i = 0; i < n; i++) {
	    list = rpmiobAppend(list, "\t", 0);
	    list = rpmiobAppend(list, unpackaged[i], 1);
	    unpackaged[i] = _free(unpackaged[i]);
	}
	unpackaged = _free(unpackaged);

	rpmlog(RPMLOG_WARNING,
	       _("Unpackaged subdir(s) in %s:\n%s"),
	       N, rpmiobStr(list));

	N = _free(N);
	list = rpmiobFree(list);
    }	

    return n;
}

/**
 * Check for unpackaged subdirectories.
 * @param spec		spec file control structure
 * @return		number of unpackaged subdirectories
 */
static int checkUnpackagedSubdirs(Spec spec)
	/*@globals rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
	/*@modifies *spec->packages,
		rpmGlobalMacroContext, fileSystem, internalState @*/
{
    int n = 0;
    Package pkg;

    for (pkg = spec->packages; pkg; pkg = pkg->next)
	n += pkgUnpackagedSubdirs(pkg);
    return n;
}

/*@-incondefs@*/
rpmRC processBinaryFiles(Spec spec, int installSpecialDoc, int test)
{
    HE_t he = (HE_t) memset(alloca(sizeof(*he)), 0, sizeof(*he));
    Package pkg;
    rpmRC res = RPMRC_OK;
    
    for (pkg = spec->packages; pkg != NULL; pkg = pkg->next) {
	int rc;

	if (pkg->fileList == NULL)
	    continue;

	(void) headerMacrosLoad(pkg->header);

	he->tag = RPMTAG_NVRA;
	rc = headerGet(pkg->header, he, 0);
	rpmlog(RPMLOG_NOTICE, _("Processing files: %s\n"), he->p.str);
	he->p.ptr = _free(he->p.ptr);
		   
	if ((rc = processPackageFiles(spec, pkg, installSpecialDoc, test))) {
	    res = RPMRC_FAIL;
	    (void) headerMacrosUnload(pkg->header);
	    break;
	}

	/* Finalize package scriptlets before extracting dependencies. */
	if ((rc = processScriptFiles(spec, pkg))) {
	    res = rc;
	    (void) headerMacrosUnload(pkg->header);
	    break;
	}

	if ((rc = rpmfcGenerateDepends(spec, pkg))) {
	    res = RPMRC_FAIL;
	    (void) headerMacrosUnload(pkg->header);
	    break;
	}

	/* XXX this should be earlier for deps to be entirely sorted. */
	providePackageNVR(pkg->header);

	(void) headerMacrosUnload(pkg->header);
    }

    if (res == RPMRC_OK) {
	if (checkUnpackagedFiles(spec) > 0)
	    res = RPMRC_FAIL;
	(void) checkDuplicateFiles(spec);
	(void) checkUnpackagedSubdirs(spec);
    }
    
    return res;
}
/*@=incondefs@*/
