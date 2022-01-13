/** \ingroup header
 * \file rpmdb/hdrfmt.c
 */

#include "system.h"

/* XXX todo: these should likely be in "system.h" */
#if defined(HAVE_ICONV)
#include <iconv.h>
#if defined(__LCLINT__)
/*@-declundef -exportheader -incondefs @*/
extern /*@only@*/ iconv_t iconv_open(const char *__tocode, const char *__fromcode)
	/*@*/;

extern size_t iconv(iconv_t __cd, /*@null@*/ char ** __inbuf,
		    /*@out@*/ size_t * __inbytesleft,
		    /*@out@*/ char ** __outbuf,
		    /*@out@*/ size_t * __outbytesleft)
	/*@modifies __cd,
		*__inbuf, *__inbytesleft, *__outbuf, *__outbytesleft @*/;

extern int iconv_close(/*@only@*/ iconv_t __cd)
        /*@modifies __cd @*/;
/*@=declundef =exportheader =incondefs @*/
#endif
#endif

#if defined(HAVE_LANGINFO_H)
#include <langinfo.h>
#if defined(__LCLINT__)
/*@-declundef -exportheader -incondefs @*/
extern char *nl_langinfo (nl_item __item)
	/*@*/;
/*@=declundef =exportheader =incondefs @*/
#endif
#endif

#define	_MIRE_INTERNAL
#include "rpmio_internal.h"
#include <rpmbc.h>	/* XXX beecrypt base64 */
#include <rpmcb.h>	/* XXX rpmIsVerbose */
#include <rpmmacro.h>	/* XXX for %_i18ndomains */
#include <rpmuuid.h>
#include <argv.h>
#include <ugid.h>

#define	_RPMTAG_INTERNAL
#include <rpmtag.h>
#define _RPMEVR_INTERNAL
#include <rpmevr.h>	/* XXX RPMSENSE_FOO */
#include <rpmns.h>
#include <rpmdb.h>

#include <rpmtypes.h>	/* XXX rpmfi */
#include "misc.h"	/* XXX rpmMkdirPath */
#include <rpmfi.h>	/* XXX RPMFILE_FOO */

#include "legacy.h"
#include "misc.h"

#include "debug.h"


#ifdef __cplusplus
GENfree(HE_t)
#endif	/* __cplusplus */

/*@unchecked@*/
int _hdrqf_debug;

/*@access pgpDig @*/
/*@access pgpDigParams @*/
/*@access headerSprintfExtension @*/
/*@access headerTagTableEntry @*/
/*@access Header @*/	/* XXX debugging msgs */
/*@access EVR_t @*/
/*@access rpmdb @*/	/* XXX for casts */
/*@access miRE @*/

/**
 * Convert tag data representation.
 * @param he		tag container
 * @param av		parameter list (or NULL)
 * @param fmt		output radix (NULL or "" assumes %d)
 * @return		formatted string
 */
static char * intFormat(HE_t he, /*@unused@*/ /*@null@*/ const char ** av,
		/*@null@*/ const char *fmt)
	/*@*/
{
    rpmuint32_t ix = (he->ix > 0 ? he->ix : 0);
    rpmuint64_t ival = 0;
    const char * istr = NULL;
    char * b;
    size_t nb = 0;
    int xx;

    if (fmt == NULL || *fmt == '\0')
	fmt = "d";

    switch (he->t) {
    default:
	return xstrdup(_("(not a number)"));
	/*@notreached@*/ break;
    case RPM_UINT8_TYPE:
	ival = (rpmuint64_t) he->p.ui8p[ix];
	break;
    case RPM_UINT16_TYPE:
	ival = (rpmuint64_t) he->p.ui16p[ix];
	break;
    case RPM_UINT32_TYPE:
	ival = (rpmuint64_t) he->p.ui32p[ix];
	break;
    case RPM_UINT64_TYPE:
	ival = he->p.ui64p[ix];
	break;
    case RPM_STRING_TYPE:
	istr = he->p.str;
	break;
    case RPM_STRING_ARRAY_TYPE:
	istr = he->p.argv[ix];
	break;
    case RPM_BIN_TYPE:
	{   static char hex[] = "0123456789abcdef";
	    const char * s = he->p.str;
	    rpmTagCount c = he->c;
	    char * t;

	    nb = 2 * c + 1;
	    t = b = alloca(nb+1);
	    while (c-- > 0) {
		unsigned i;
		i = (unsigned) *s++;
		*t++ = hex[ (i >> 4) & 0xf ];
		*t++ = hex[ (i     ) & 0xf ];
	    }
	    *t = '\0';
	}   break;
    }

    if (istr) {		/* string */
	b = (char *)istr;	/* NOCAST */
    } else
    if (nb == 0) {	/* number */
	char myfmt[] = "%llX";
	myfmt[3] = ((fmt != NULL && *fmt != '\0') ? *fmt : 'd');
	nb = 64;
	b = alloca(nb);
/*@-formatconst@*/
	xx = snprintf(b, nb, myfmt, ival);
/*@=formatconst@*/
	b[nb-1] = '\0';
    } else
	b = "";

    return xstrdup(b);
}

/**
 * Return octal formatted data.
 * @param he		tag container
 * @param av		parameter list (or NULL)
 * @return		formatted string
 */
static char * octFormat(HE_t he, /*@null@*/ const char ** av)
	/*@*/
{
    return intFormat(he, av, "o");
}

/**
 * Return hex formatted data.
 * @param he		tag container
 * @param av		parameter list (or NULL)
 * @return		formatted string
 */
static char * hexFormat(HE_t he, /*@null@*/ const char ** av)
	/*@*/
{
    return intFormat(he, av, "x");
}

/**
 * Return decimal formatted data.
 * @param he		tag container
 * @param av		parameter list (or NULL)
 * @return		formatted string
 */
static char * decFormat(HE_t he, /*@null@*/ const char ** av)
	/*@*/
{
    return intFormat(he, av, "d");
}

/**
 * Return strftime formatted data.
 * @param he		tag container
 * @param av		parameter list (or NULL)
 * @param strftimeFormat strftime(3) format
 * @return		formatted string
 */
static char * realDateFormat(HE_t he, /*@unused@*/ /*@null@*/ const char ** av,
		const char * strftimeFormat)
	/*@*/
{
    char * val;

    if (he->t != RPM_UINT64_TYPE) {
	val = xstrdup(_("(not a number)"));
    } else {
	struct tm * tstruct;
	char buf[50];

	/* this is important if sizeof(rpmuint64_t) ! sizeof(time_t) */
	{   time_t dateint = he->p.ui64p[0];
	    tstruct = localtime(&dateint);
	}
	buf[0] = '\0';
	if (tstruct)
	    (void) strftime(buf, sizeof(buf) - 1, strftimeFormat, tstruct);
	buf[sizeof(buf) - 1] = '\0';
	val = xstrdup(buf);
    }

    return val;
}

/**
 * Return date formatted data.
 * @param he		tag container
 * @param av		parameter list (or NULL)
 * @return		formatted string
 */
static char * dateFormat(HE_t he, /*@null@*/ const char ** av)
	/*@*/
{
    return realDateFormat(he, av, _("%c"));
}

/**
 * Return day formatted data.
 * @param he		tag container
 * @param av		parameter list (or NULL)
 * @return		formatted string
 */
static char * dayFormat(HE_t he, /*@null@*/ const char ** av)
	/*@*/
{
    return realDateFormat(he, av, _("%a %b %d %Y"));
}

/**
 * Return shell escape formatted data.
 * @param he		tag container
 * @param av		parameter list (or NULL)
 * @return		formatted string
 */
static char * shescapeFormat(HE_t he, /*@unused@*/ /*@null@*/ const char ** av)
	/*@*/
{
    char * val;
    size_t nb;
    int xx;

    /* XXX one of these integer types is unnecessary. */
    if (he->t == RPM_UINT32_TYPE) {
	nb = 20;
	val = xmalloc(nb);
	xx = snprintf(val, nb, "%u", (unsigned) he->p.ui32p[0]);
	val[nb-1] = '\0';
    } else if (he->t == RPM_UINT64_TYPE) {
	nb = 40;
	val = xmalloc(40);
/*@-duplicatequals@*/
	xx = snprintf(val, nb, "%llu", (unsigned long long)he->p.ui64p[0]);
/*@=duplicatequals@*/
	val[nb-1] = '\0';
    } else if (he->t == RPM_STRING_TYPE) {
	const char * s = he->p.str;
	char * t;
	int c;

	nb = 0;
	for (s = he->p.str; (c = (int)*s) != 0; s++)  {
	    nb++;
	    if (c == (int)'\'')
		nb += 3;
	}
	nb += 3;
	t = val = xmalloc(nb);
	*t++ = '\'';
	for (s = he->p.str; (c = (int)*s) != 0; s++)  {
	    if (c == (int)'\'') {
		*t++ = '\'';
		*t++ = '\\';
		*t++ = '\'';
	    }
	    *t++ = (char) c;
	}
	*t++ = '\'';
	*t = '\0';
    } else
	val = xstrdup(_("invalid type"));

    return val;
}

static struct headerSprintfExtension_s _headerDefaultFormats[] = {
    { HEADER_EXT_FORMAT, "octal",
	{ .fmtFunction = octFormat } },
    { HEADER_EXT_FORMAT, "oct",
	{ .fmtFunction = octFormat } },
    { HEADER_EXT_FORMAT, "hex",
	{ .fmtFunction = hexFormat } },
    { HEADER_EXT_FORMAT, "decimal",
	{ .fmtFunction = decFormat } },
    { HEADER_EXT_FORMAT, "dec",
	{ .fmtFunction = decFormat } },
    { HEADER_EXT_FORMAT, "date",
	{ .fmtFunction = dateFormat } },
    { HEADER_EXT_FORMAT, "day",
	{ .fmtFunction = dayFormat } },
    { HEADER_EXT_FORMAT, "shescape",
	{ .fmtFunction = shescapeFormat } },
    { HEADER_EXT_LAST, NULL, { NULL } }
};

headerSprintfExtension headerDefaultFormats = &_headerDefaultFormats[0];

/*====================================================================*/
typedef const struct spew_s * spew_t;
struct spew_s {
/*@observer@*/
    const char * spew_name;
    const char * spew_init;
    const char * spew_fini;
    size_t (*spew_strlen) (const char * s, int lvl)
	/*@*/;
    char * (*spew_strcpy) (/*@returned@*/ char * t, const char * s, int lvl)
	/*@modifies t @*/;
};

/*====================================================================*/
/**
 * Return length of string represented with xml characters substituted.
 * @param s		string
 * @param lvl		indentation level
 * @return		length of xml string
 */
static size_t xmlstrlen(const char * s, /*@unused@*/ int lvl)
	/*@*/
{
    size_t len = 0;
    int c;

    while ((c = (int) *s++) != (int) '\0') {
	switch (c) {
	case '<':
	case '>':	len += sizeof("&lt;") - 1;	/*@switchbreak@*/ break;
	case '&':	len += sizeof("&amp;") - 1;	/*@switchbreak@*/ break;
	default:	len += 1;			/*@switchbreak@*/ break;
	}
    }
    return len;
}

/**
 * Copy source string to target, substituting for  xml characters.
 * @param t		target xml string
 * @param s		source string
 * @param lvl		indentation level
 * @return		target xml string
 */
static char * xmlstrcpy(/*@returned@*/ char * t, const char * s,
		/*@unused@*/ int lvl)
	/*@modifies t @*/
{
    char * te = t;
    int c;

    while ((c = (int) *s++) != (int) '\0') {
	switch (c) {
	case '<':	te = stpcpy(te, "&lt;");	/*@switchbreak@*/ break;
	case '>':	te = stpcpy(te, "&gt;");	/*@switchbreak@*/ break;
	case '&':	te = stpcpy(te, "&amp;");	/*@switchbreak@*/ break;
	default:	*te++ = (char) c;		/*@switchbreak@*/ break;
	}
    }
    *te = '\0';
    return t;
}

/*@unchecked@*/ /*@observer@*/ 
static const struct spew_s _xml_spew = {
    .spew_name		= "xml",
    .spew_init		= "<rpmHeader>\n",
    .spew_fini		= "</rpmHeader>\n",
    .spew_strlen	= xmlstrlen,
    .spew_strcpy	= xmlstrcpy
};

/*====================================================================*/

/**
 * Return length of string represented with yaml indentation.
 * @param s		string
 * @param lvl		indentation level
 * @return		length of yaml string
 */
static size_t yamlstrlen(const char * s, int lvl)
	/*@*/
{
    size_t len = 0;
    int indent = (lvl > 0);
    int c;

    while ((c = (int) *s++) != (int) '\0')
    {
	if (indent) {
	    len += 2 * lvl;
	    indent = 0;
	}
	if (c == (int) '\n')
	    indent = (lvl > 0);
	len++;
    }
    return len;
}

/**
 * Copy source string to target, indenting for yaml.
 * @param t		target yaml string
 * @param s		source string
 * @param lvl		indentation level
 * @return		target yaml string
 */
static char * yamlstrcpy(/*@out@*/ /*@returned@*/ char * t, const char * s,
		int lvl)
	/*@modifies t @*/
{
    char * te = t;
    int indent = (lvl > 0);
    int c;

    while ((c = (int) *s++) != (int) '\0') {
	if (indent) {
	    int i;
	    for (i = 0; i < lvl; i++) {
		*te++ = ' ';
		*te++ = ' ';
	    }
	    indent = 0;
	}
	if (c == (int) '\n')
	    indent = (lvl > 0);
	*te++ = (char) c;
    }
    *te = '\0';
    return t;
}

/*@unchecked@*/ /*@observer@*/ 
static const struct spew_s _yaml_spew = {
    .spew_name		= "yaml",
    .spew_init		= "- !!omap\n",
    .spew_fini		= "\n",
    .spew_strlen	= yamlstrlen,
    .spew_strcpy	= yamlstrcpy
};

/*====================================================================*/

/**
 * Return length of string represented with single quotes doubled.
 * @param s		string
 * @param lvl		indentation level
 * @return		length of json string
 */
static size_t jsonstrlen(const char * s, /*@unused@*/ int lvl)
	/*@*/
{
    size_t len = 0;
    int c;

    while ((c = (int) *s++) != (int) '\0') {
	switch (c) {
	case '\b':
	case '\t':
	case '\n':
	case '\f':
	case '\r':
	case '"':
	case '\\':	len += 1;			/*@fallthrough@*/
	default:	len += 1;			/*@switchbreak@*/ break;
	/* XXX todo: emit \u1234 here somehow */
	}
    }
    return len;
}

/**
 * Copy source string to target, doubling single quotes.
 * @param t		target json string
 * @param s		source string
 * @param lvl		indentation level
 * @return		target json string
 */
static char * jsonstrcpy(/*@returned@*/ char * t, const char * s,
		/*@unused@*/ int lvl)
	/*@modifies t @*/
{
    char * te = t;
    int c;

    while ((c = (int) *s++) != (int) '\0') {
	switch (c) {
	case '\b':	*te++ = '\\'; *te++ = 'b';	/*@switchbreak@*/ break;
	case '\t':	*te++ = '\\'; *te++ = 't';	/*@switchbreak@*/ break;
	case '\n':	*te++ = '\\'; *te++ = 'n';	/*@switchbreak@*/ break;
	case '\f':	*te++ = '\\'; *te++ = 'f';	/*@switchbreak@*/ break;
	case '\r':	*te++ = '\\'; *te++ = 'r';	/*@switchbreak@*/ break;
	case '"':	*te++ = '\\'; *te++ = '"';	/*@switchbreak@*/ break;
	case '\\':	*te++ = '\\'; *te++ = '\\';	/*@switchbreak@*/ break;
	default:	*te++ = (char) c;		/*@switchbreak@*/ break;
	/* XXX todo: emit \u1234 here somehow */
	}
    }
    *te = '\0';
    return t;
}

/*@unchecked@*/ /*@observer@*/ 
static const struct spew_s _json_spew = {
    .spew_name		= "json",
    /* XXX non-functional atm, /usr/lib/rpm/qf *.mongo template for now. */
    .spew_init		= "db.%{?__mongodb_collection}%{!?__mongodb_collection:packages}.save({\n",
    .spew_fini		= "});\n",
    .spew_strlen	= jsonstrlen,
    .spew_strcpy	= jsonstrcpy
};

/*====================================================================*/

/**
 * Return length of string represented with single quotes doubled.
 * @param s		string
 * @param lvl		indentation level
 * @return		length of sql string
 */
static size_t sqlstrlen(const char * s, /*@unused@*/ int lvl)
	/*@*/
{
    size_t len = 0;
    int c;

    while ((c = (int) *s++) != (int) '\0') {
	switch (c) {
	case '\'':	len += 1;			/*@fallthrough@*/
	default:	len += 1;			/*@switchbreak@*/ break;
	}
    }
    return len;
}

/**
 * Copy source string to target, doubling single quotes.
 * @param t		target sql string
 * @param s		source string
 * @param lvl		indentation level
 * @return		target sql string
 */
static char * sqlstrcpy(/*@returned@*/ char * t, const char * s,
		/*@unused@*/ int lvl)
	/*@modifies t @*/
{
    char * te = t;
    int c;

    while ((c = (int) *s++) != (int) '\0') {
	switch (c) {
	case '\'':	*te++ = (char) c;		/*@fallthrough@*/
	default:	*te++ = (char) c;		/*@switchbreak@*/ break;
	}
    }
    *te = '\0';
    return t;
}

/*@unchecked@*/ /*@observer@*/ 
static const struct spew_s _sql_spew = {
    .spew_name		= "sql",
    .spew_init		= "",
    .spew_fini		= "",
    .spew_strlen	= sqlstrlen,
    .spew_strcpy	= sqlstrcpy
};

/*====================================================================*/

/* XXX FIXME: static for now, refactor from manifest.c later. */
static char * rpmPermsString(int mode)
	/*@*/
{
    char *perms = xstrdup("----------");
   
    if (S_ISREG(mode)) 
	perms[0] = '-';
    else if (S_ISDIR(mode)) 
	perms[0] = 'd';
    else if (S_ISLNK(mode))
	perms[0] = 'l';
    else if (S_ISFIFO(mode)) 
	perms[0] = 'p';
/*@-unrecog@*/
    else if (S_ISSOCK(mode)) 
	perms[0] = 's';
/*@=unrecog@*/
    else if (S_ISCHR(mode))
	perms[0] = 'c';
    else if (S_ISBLK(mode))
	perms[0] = 'b';
    else
	perms[0] = '?';

    if (mode & S_IRUSR) perms[1] = 'r';
    if (mode & S_IWUSR) perms[2] = 'w';
    if (mode & S_IXUSR) perms[3] = 'x';
 
    if (mode & S_IRGRP) perms[4] = 'r';
    if (mode & S_IWGRP) perms[5] = 'w';
    if (mode & S_IXGRP) perms[6] = 'x';

    if (mode & S_IROTH) perms[7] = 'r';
    if (mode & S_IWOTH) perms[8] = 'w';
    if (mode & S_IXOTH) perms[9] = 'x';

    if (mode & S_ISUID)
	perms[3] = ((mode & S_IXUSR) ? 's' : 'S'); 

    if (mode & S_ISGID)
	perms[6] = ((mode & S_IXGRP) ? 's' : 'S'); 

    if (mode & S_ISVTX)
	perms[9] = ((mode & S_IXOTH) ? 't' : 'T');

    return perms;
}

/**
 * Identify type of trigger.
 * @param he		tag container
 * @param av		parameter list (or NULL)
 * @return		formatted string
 */
static /*@only@*/ char * triggertypeFormat(HE_t he, /*@unused@*/ /*@null@*/ const char ** av)
	/*@*/
{
    int ix = (he->ix > 0 ? he->ix : 0);
    char * val;

assert(ix == 0);
    if (he->t != RPM_UINT64_TYPE)
	val = xstrdup(_("(invalid type)"));
    else {
	rpmuint64_t anint = he->p.ui64p[ix];
	if (anint & RPMSENSE_TRIGGERPREIN)
	    val = xstrdup("prein");
	else if (anint & RPMSENSE_TRIGGERIN)
	    val = xstrdup("in");
	else if (anint & RPMSENSE_TRIGGERUN)
	    val = xstrdup("un");
	else if (anint & RPMSENSE_TRIGGERPOSTUN)
	    val = xstrdup("postun");
	else
	    val = xstrdup("");
    }
    return val;
}

/**
 * Format file permissions for display.
 * @param he		tag container
 * @param av		parameter list (or NULL)
 * @return		formatted string
 */
static /*@only@*/ char * permsFormat(HE_t he, /*@unused@*/ /*@null@*/ const char ** av)
	/*@*/
{
    int ix = (he->ix > 0 ? he->ix : 0);
    char * val;

assert(ix == 0);
    if (he->t != RPM_UINT64_TYPE) {
	val = xstrdup(_("(invalid type)"));
    } else {
	rpmuint64_t anint = he->p.ui64p[0];
	val = rpmPermsString((int)anint);
    }

    return val;
}

/**
 * Format file flags for display.
 * @param he		tag container
 * @param av		parameter list (or NULL)
 * @return		formatted string
 */
static /*@only@*/ char * fflagsFormat(HE_t he, /*@unused@*/ /*@null@*/ const char ** av)
	/*@*/
{
    int ix = (he->ix >= 0 ? he->ix : 0);
    char * val;

assert(ix == 0);
    if (he->t != RPM_UINT64_TYPE) {
	val = xstrdup(_("(invalid type)"));
    } else {
	char buf[15];
	rpmuint64_t anint = he->p.ui64p[ix];
	buf[0] = '\0';
	if (anint & RPMFILE_DOC)
	    strcat(buf, "d");
	if (anint & RPMFILE_CONFIG)
	    strcat(buf, "c");
	if (anint & RPMFILE_SPECFILE)
	    strcat(buf, "s");
	if (anint & RPMFILE_MISSINGOK)
	    strcat(buf, "m");
	if (anint & RPMFILE_NOREPLACE)
	    strcat(buf, "n");
	if (anint & RPMFILE_GHOST)
	    strcat(buf, "g");
	if (anint & RPMFILE_LICENSE)
	    strcat(buf, "l");
	if (anint & RPMFILE_README)
	    strcat(buf, "r");
	val = xstrdup(buf);
    }

    return val;
}

/**
 * Wrap a pubkey in ascii armor for display.
 * @todo Permit selectable display formats (i.e. binary).
 * @param he		tag container
 * @param av		parameter list (or NULL)
 * @return		formatted string
 */
static /*@only@*/ char * armorFormat(HE_t he, /*@unused@*/ /*@null@*/ const char ** av)
	/*@*/
{
    int ix = (he->ix > 0 ? he->ix : 0);
    const char * enc;
    const unsigned char * s;
    size_t ns;
    rpmuint8_t atype;
    char * val;

assert(ix == 0);
    switch (he->t) {
    case RPM_BIN_TYPE:
	s = (unsigned char *) he->p.ui8p;
	ns = he->c;
	atype = (rpmuint8_t)PGPARMOR_SIGNATURE;	/* XXX check pkt for signature */
	break;
    case RPM_STRING_TYPE:
    case RPM_STRING_ARRAY_TYPE:
	enc = he->p.str;
	s = NULL;
	ns = 0;
/*@-moduncon@*/
	if (b64decode(enc, (void *)&s, &ns))
	    return xstrdup(_("(not base64)"));
/*@=moduncon@*/
	atype = (rpmuint8_t)PGPARMOR_PUBKEY;	/* XXX check pkt for pubkey */
	break;
#if defined(SUPPORT_I18NSTRING_TYPE)
    case RPM_I18NSTRING_TYPE:
#endif
    case RPM_UINT8_TYPE:
    case RPM_UINT16_TYPE:
    case RPM_UINT32_TYPE:
    case RPM_UINT64_TYPE:
    default:
	return xstrdup(_("(invalid type)"));
	/*@notreached@*/ break;
    }

    val = pgpArmorWrap(atype, s, ns);
    if (atype == (rpmuint8_t)PGPARMOR_PUBKEY)
	s = _free(s);
    return val;
}

/**
 * Encode binary data in base64 for display.
 * @todo Permit selectable display formats (i.e. binary).
 * @param he		tag container
 * @param av		parameter list (or NULL)
 * @return		formatted string
 */
static /*@only@*/ char * base64Format(HE_t he, /*@unused@*/ /*@null@*/ const char ** av)
	/*@*/
{
    int ix = (he->ix > 0 ? he->ix : 0);
    char * val;
    const char * enc;
    char * t;
    int lc;
    size_t ns;
    size_t nt;

assert(ix == 0);
    switch(he->t) {
    default:
	val = xstrdup(_("(invalid type :base64)"));
	goto exit;
	/*@notreached@*/ break;
    case RPM_UINT64_TYPE:
	ns = sizeof(he->p.ui64p[0]);
	break;
    case RPM_STRING_TYPE:
	ns = strlen(he->p.str);
	break;
    case RPM_BIN_TYPE:
	ns = he->c;
	break;
    }

    nt = ((ns + 2) / 3) * 4;

/*@-globs@*/
    /* Add additional bytes necessary for eol string(s). */
    if (b64encode_chars_per_line > 0 && b64encode_eolstr != NULL) {
	lc = (nt + b64encode_chars_per_line - 1) / b64encode_chars_per_line;
	if (((nt + b64encode_chars_per_line - 1) % b64encode_chars_per_line) != 0)
	    ++lc;
	nt += lc * strlen(b64encode_eolstr);
    }
/*@=globs@*/

    val = t = xcalloc(1, nt + 1);
    *t = '\0';

    /* XXX b64encode accesses uninitialized memory. */
    { 	unsigned char * _data = xcalloc(1, ns+1);
assert(he->p.ptr != NULL);
	memcpy(_data, he->p.ptr, ns);
/*@-moduncon@*/
	if ((enc = b64encode(_data, ns)) != NULL) {
	    t = stpcpy(t, enc);
	    enc = _free(enc);
	}
/*@=moduncon@*/
	_data = _free(_data);
    }

exit:
/*@-globstate@*/	/* b64encode_eolstr annotation */
    return val;
/*@=globstate@*/
}

/*====================================================================*/

#if defined(__GLIBC__)	/* XXX todo: find where iconv(3) was implemented. */
/* XXX using "//TRANSLIT" instead assumes known fromcode? */
/*@unchecked@*/
static const char * _iconv_tocode = "UTF-8//IGNORE";
/*@unchecked@*/
static const char * _iconv_fromcode = "UTF-8";
#else
/*@unchecked@*/
static const char * _iconv_tocode = "UTF-8";
/*@unchecked@*/
static const char * _iconv_fromcode = NULL;
#endif

static /*@only@*/ /*@null@*/ char *
strdup_iconv_check (/*@null@*/ const char * buffer,
		/*@null@*/ const char * tocode)
	/*@*/
{
    const char *s = buffer;
    char *t = NULL;
#if defined(HAVE_ICONV)
    const char *fromcode = _iconv_fromcode;
    iconv_t fd;

assert(buffer != NULL);

    if (tocode == NULL)
	tocode = _iconv_tocode;
assert(tocode != NULL);

#ifdef HAVE_LANGINFO_H
    /* XXX the current locale's encoding != package data encodings. */
    if (fromcode == NULL)
        fromcode = nl_langinfo (CODESET);
#endif
assert(fromcode != NULL);

    if ((fd = iconv_open(tocode, fromcode)) != (iconv_t)-1) {
	size_t ileft = strlen(s);
	size_t nt = ileft;
	char * te = t = xmalloc((nt + 1) * sizeof(*t));
	size_t oleft = ileft;
	size_t err = iconv(fd, NULL, NULL, NULL, NULL);
	const char *sprev = NULL;
	int _iconv_errno = 0;
	int done = 0;

	while (done == 0 && _iconv_errno == 0) {
	    err = iconv(fd, (char **)&s, &ileft, &te, &oleft);
	    if (err == (size_t)-1) {
		switch (errno) {
		case E2BIG:
		{   size_t used = (size_t)(te - t);
		    nt *= 2;
		    t = xrealloc(t, (nt + 1) * sizeof(*t));
		    te = t + used;
		    oleft = nt - used;
		}   /*@switchbreak@*/ break;
		case EINVAL:
		    done = 1;
		    /*@fallthrough@*/
		case EILSEQ:
		default:
		    _iconv_errno = errno;
		    /*@switchbreak@*/ break;
		}
	    } else
	    if (sprev == NULL) {
		sprev = s;
		s = NULL;
		ileft = 0;
	    } else
	        done = 1;
	}
	if (iconv_close(fd))
	    _iconv_errno = errno;
	*te = '\0';
	te = xstrdup(t);
	t = _free(t);
	t = te;

if (_iconv_errno)
fprintf(stderr, "warning: %s: from iconv(%s -> %s) for \"%s\" -> \"%s\"\n", strerror(_iconv_errno), fromcode, tocode, buffer, t);

    } else
#endif
	t = xstrdup((s ? s : ""));

    return t;
}

/**
 * Encode string for use in XML CDATA.
 * @param he		tag container
 * @param av		parameter list (or NULL)
 * @return		formatted string
 */
static /*@only@*/ char * cdataFormat(HE_t he, /*@null@*/ const char ** av)
	/*@*/
{
    int ix = (he->ix > 0 ? he->ix : 0);
    char * val;
int lvl = 0;
spew_t spew = &_xml_spew;

assert(ix == 0);
    if (he->t != RPM_STRING_TYPE) {
	val = xstrdup(_("(not a string)"));
    } else {
	const char * s = strdup_iconv_check(he->p.str, (av ? av[0] : NULL));
	size_t nb = spew->spew_strlen(s, lvl);
	char * t = xmalloc(nb + 1);

	val = t;
	t = spew->spew_strcpy(t, s, lvl);	t += strlen(t);
	*t = '\0';
	s = _free(s);
    }

   return val;
}

/**
 * Convert string encoding.
 * @param he		tag container
 * @param av		parameter list (NULL assumes UTF-8)
 * @return		formatted string
 */
static /*@only@*/ char * iconvFormat(HE_t he, /*@unused@*/ /*@null@*/ const char ** av)
	/*@*/
{
    int ix = (he->ix > 0 ? he->ix : 0);
    char * val = NULL;

assert(ix == 0);
    if (he->t == RPM_STRING_TYPE)
	val = strdup_iconv_check(he->p.str, (av ? av[0] : NULL));
    if (val == NULL)
	val = xstrdup(_("(not a string)"));

    return val;
}

/**
 * Wrap tag data in simple header xml markup.
 * @param he		tag container
 * @param av		parameter list (or NULL)
 * @return		formatted string
 */
static /*@only@*/ char * xmlFormat(HE_t he, /*@unused@*/ /*@null@*/ const char ** av)
	/*@*/
{
    int ix = (he->ix > 0 ? he->ix : 0);
    const char * xtag = NULL;
    char * val;
    const char * s = NULL;
    uint64_t anint = 0;
int lvl = 0;
spew_t spew = &_xml_spew;

assert(ix == 0);
assert(he->t == RPM_STRING_TYPE || he->t == RPM_UINT64_TYPE || he->t == RPM_BIN_TYPE);

    switch (he->t) {
    case RPM_STRING_ARRAY_TYPE:	/* XXX currently never happens */
	/*@fallthrough@*/
    case RPM_I18NSTRING_TYPE:	/* XXX currently never happens */
assert(0);
	/*@notreached@*/
    case RPM_STRING_TYPE:
	xtag = "string";
	s = strdup_iconv_check(he->p.str, (av ? av[0] : NULL));
	break;
    case RPM_BIN_TYPE:
/*@-globs -mods@*/	/* Don't bother annotating beecrypt global mods */
    {	int cpl = b64encode_chars_per_line;
	b64encode_chars_per_line = 0;
/*@-formatconst@*/
	s = base64Format(he, NULL);
/*@=formatconst@*/
	b64encode_chars_per_line = cpl;
	xtag = "base64";
    }	break;
/*@=globs =mods@*/
    case RPM_UINT8_TYPE:
	anint = (uint64_t)he->p.ui8p[ix];
	break;
    case RPM_UINT16_TYPE:
	anint = (uint64_t)he->p.ui16p[ix];
	break;
    case RPM_UINT32_TYPE:
	anint = (uint64_t)he->p.ui32p[ix];
	break;
    case RPM_UINT64_TYPE:
	anint = he->p.ui64p[ix];
	break;
    default:
	val = xstrdup(_("(invalid xml type)"));
	goto exit;
	/*@notreached@*/ break;
    }

    if (s == NULL) {
	static int tlen = 64;
	char * t = xmalloc(tlen+1);
	int xx;

	*t = '\0';
	if (anint != 0)		/* XXX empty XML tag sets 0 as default? */
	    xx = snprintf(t, tlen, "%llu", (unsigned long long)anint);
	s = t;
	xtag = "integer";
    }

  {
    size_t nb = spew->spew_strlen(s, lvl);
    char * t, * te;

    if (nb == 0) {
	nb += strlen(xtag) + sizeof("\t</>");
	t = te = alloca(nb);
	*te = '\0';
	te = stpcpy( stpcpy( stpcpy(te, "\t<"), xtag), "/>");
    } else {
	nb += 2 * strlen(xtag) + sizeof("\t<></>");
	t = te = alloca(nb);
	*te = '\0';
	te = stpcpy( stpcpy( stpcpy(te, "\t<"), xtag), ">");
	te = spew->spew_strcpy(te, s, lvl);
	te += strlen(te);
	te = stpcpy( stpcpy( stpcpy(te, "</"), xtag), ">");
    }

    val = xstrdup(t);
  }

    s = _free(s);

exit:
    return val;
}

/**
 * Wrap tag data in simple header yaml markup.
 * @param he		tag container
 * @param av		parameter list (or NULL)
 * @return		formatted string
 */
static /*@only@*/ char * yamlFormat(HE_t he, /*@unused@*/ /*@null@*/ const char ** av)
	/*@*/
{
    int element = he->ix;
    int ix = (he->ix > 0 ? he->ix : 0);
    const char * xtag = NULL;
    char * val;
    const char * s = NULL;
    uint64_t anint = 0;
    int xx = 0;
    int ls = 0;
    int c;
int lvl = 0;
spew_t spew = &_yaml_spew;

assert(ix == 0);
assert(he->t == RPM_STRING_TYPE || he->t == RPM_UINT64_TYPE || he->t == RPM_BIN_TYPE);

    switch (he->t) {
    case RPM_STRING_ARRAY_TYPE:	/* XXX currently never happens */
	/*@fallthrough@*/
    case RPM_I18NSTRING_TYPE:	/* XXX currently never happens */
assert(0);
	/*@notreached@*/
    case RPM_STRING_TYPE:
	s = (he->t == RPM_STRING_ARRAY_TYPE ? he->p.argv[ix] : he->p.str);
	if (strchr("[", s[0]))	/* leading [ */
	    xx = 1;
	if (xx == 0)
	while ((c = (int) *s++) != (int) '\0') {
	    switch (c) {
	    default:
		continue;
	    case '\n':	/* multiline */
		xx = 1;
		if (s[0] == ' ' || s[0] == '\t') /* leading space */
		    ls = 1;
		continue;
	    case '-':	/* leading "- \"" */
	    case ':':	/* embedded ": " or ":" at EOL */
		if (s[0] != ' ' && s[0] != '\0' && s[1] != '"')
		    continue;
		xx = 1;
		/*@switchbreak@*/ break;
	    }
	    /*@loopbreak@*/ break;
	}
	if (xx) {
	    if (ls) { /* leading spaces means we need to specify the indent */
		if (element >= 0) {
		    lvl = 3;
		    xtag = "- |3-\n";
		} else if (he->ix < 0) {
		    lvl = 3;		/* XXX extra indent for array[1] */
		    xtag =   "|3-\n";
		} else {
		    lvl = 2;
		    xtag =   "|2-\n";
		}
	    } else {
		if (element >= 0) {
		    lvl = 3;
		    xtag = "- |-\n";
		} else {
		    lvl = 2;
		    if (he->ix < 0) lvl++; /* XXX extra indent for array[1] */
		    xtag =   "|-\n";
		}
	    }
	} else {
	    xtag = (element >= 0 ? "- " : NULL);
	}

	s = strdup_iconv_check(he->p.str, (av ? av[0] : NULL));
	break;
    case RPM_BIN_TYPE:
/*@-globs -mods@*/	/* Don't bother annotating beecrypt global mods */
    {	int cpl = b64encode_chars_per_line;
	b64encode_chars_per_line = 0;
/*@-formatconst@*/
	s = base64Format(he, NULL);
	element = -element; 	/* XXX skip "    " indent. */
/*@=formatconst@*/
	b64encode_chars_per_line = cpl;
	xtag = "!!binary ";
    }	break;
/*@=globs =mods@*/
    case RPM_UINT8_TYPE:
	anint = (uint64_t)he->p.ui8p[ix];
	break;
    case RPM_UINT16_TYPE:
	anint = (uint64_t)he->p.ui16p[ix];
	break;
    case RPM_UINT32_TYPE:
	anint = (uint64_t)he->p.ui32p[ix];
	break;
    case RPM_UINT64_TYPE:
	anint = he->p.ui64p[ix];
	break;
    default:
	val = xstrdup(_("(invalid yaml type)"));
	goto exit;
	/*@notreached@*/ break;
    }

    if (s == NULL) {
	static int tlen = 64;
	char * t = xmalloc(tlen+1);
/*@-duplicatequals@*/
	xx = snprintf(t, tlen, "%llu", (unsigned long long)anint);
/*@=duplicatequals@*/
	s = t;
	xtag = (element >= 0 ? "- " : NULL);
    }

  {
    size_t nb = spew->spew_strlen(s, lvl);
    char * t, * te;

    if (nb == 0) {
	if (element >= 0)
	    nb += sizeof("    ") - 1;
	nb += sizeof("- ~") - 1;
	nb++;
	t = te = alloca(nb);
	*te = '\0';
	if (element >= 0)
	    te = stpcpy(te, "    ");
	te = stpcpy(te, "- ~");
    } else {
	if (element >= 0)
	    nb += sizeof("    ") - 1;
	if (xtag)
	    nb += strlen(xtag);
	nb++;
	t = te = alloca(nb);
	*te = '\0';
	if (element >= 0)
	    te = stpcpy(te, "    ");
	if (xtag)
	    te = stpcpy(te, xtag);
	te = spew->spew_strcpy(te, s, lvl);
	te += strlen(te);
    }

    val = xstrdup(t);
  }

    s = _free(s);

exit:
    return val;
}

/**
 * Wrap tag data in simple header json markup.
 * @param he		tag container
 * @param av		parameter list (or NULL)
 * @return		formatted string
 */
static /*@only@*/
char * jsonFormat(HE_t he, /*@unused@*/ /*@null@*/ const char ** av)
	/*@*/
{
    int element = he->ix;
    int ix = (he->ix > 0 ? he->ix : 0);
    char * val;
    const char * s = NULL;
    uint64_t anint = 0;
    int xx = 0;
    int c;
int lvl = 0;
spew_t spew = &_json_spew;

assert(ix == 0);
assert(he->t == RPM_STRING_TYPE || he->t == RPM_UINT64_TYPE || he->t == RPM_BIN_TYPE);

    switch (he->t) {
    case RPM_STRING_ARRAY_TYPE:	/* XXX currently never happens */
	/*@fallthrough@*/
    case RPM_I18NSTRING_TYPE:	/* XXX currently never happens */
assert(0);
	/*@notreached@*/
    case RPM_STRING_TYPE:
	s = strdup_iconv_check(he->p.str, (av ? av[0] : NULL));
	break;
    case RPM_BIN_TYPE:
    {	int cpl = b64encode_chars_per_line;
	b64encode_chars_per_line = 0;
	s = base64Format(he, NULL);
	element = -element; 	/* XXX skip "    " indent. */
	b64encode_chars_per_line = cpl;
    }	break;
    case RPM_UINT8_TYPE:
	anint = (uint64_t)he->p.ui8p[ix];
	break;
    case RPM_UINT16_TYPE:
	anint = (uint64_t)he->p.ui16p[ix];
	break;
    case RPM_UINT32_TYPE:
	anint = (uint64_t)he->p.ui32p[ix];
	break;
    case RPM_UINT64_TYPE:
	anint = he->p.ui64p[ix];
	break;
    default:
	val = xstrdup(_("(invalid json type)"));
	goto exit;
	/*@notreached@*/ break;
    }

    if (s == NULL) {
	static int tlen = 64;
	char * t = xmalloc(tlen+1);
	xx = snprintf(t, tlen, "%llu", (unsigned long long)anint);
	s = t;
	c = '\0';
    } else
	c = '"';

  {
    size_t nb = spew->spew_strlen(s, lvl);
    char * t, * te;

    t = te = alloca(nb + sizeof("\"\","));
    *te = '\0';
    if (c != '\0')	*te++ = c;
    if (nb) {
	te = spew->spew_strcpy(te, s, lvl);
	te += strlen(te);
    }
    if (c != '\0') *te++ = c;
    *te++ = ',';
    *te = '\0';

    val = xstrdup(t);
  }

    s = _free(s);

exit:
    return val;
}

/*====================================================================*/

/**
 * Display signature fingerprint and time.
 * @param he		tag container
 * @param av		parameter list (or NULL)
 * @return		formatted string
 */
static /*@only@*/ char * pgpsigFormat(HE_t he, /*@unused@*/ /*@null@*/ const char ** av)
	/*@globals fileSystem, internalState @*/
	/*@modifies fileSystem, internalState @*/
{
    int ix = (he->ix > 0 ? he->ix : 0);
    char * val, * t;

assert(ix == 0);
    if (!(he->t == RPM_BIN_TYPE)) {
	val = xstrdup(_("(not a blob)"));
    } else {
	rpmuint8_t * pkt = he->p.ui8p;
	unsigned int pktlen = 0;
	unsigned int v = (unsigned int) *pkt;
	pgpTag tag = 0;
	unsigned int plen;
	unsigned int hlen = 0;

	if (v & 0x80) {
	    if (v & 0x40) {
		tag = (v & 0x3f);
		plen = pgpLen(pkt+1, &hlen);
	    } else {
		tag = (v >> 2) & 0xf;
		plen = (1 << (v & 0x3));
		hlen = pgpGrab(pkt+1, plen);
	    }
	
	    pktlen = 1 + plen + hlen;
	}

	if (pktlen == 0 || tag != PGPTAG_SIGNATURE) {
	    val = xstrdup(_("(not an OpenPGP signature)"));
	} else {
	    pgpDig dig = pgpDigNew(RPMVSF_DEFAULT, 0);
	    pgpDigParams sigp = pgpGetSignature(dig);
	    size_t nb = 0;
	    const char *tempstr;

	    (void) pgpPrtPkts(pkt, pktlen, dig, 0);

	    val = NULL;
	again:
	    nb += 100;
	    val = t = xrealloc(val, nb + 1);

	    t = stpcpy(t, _pgpPubkeyAlgo2Name(sigp->pubkey_algo));
	    if (t + 5 >= val + nb)
		goto again;

	    *t++ = '/';
	    t = stpcpy(t, _pgpHashAlgo2Name(sigp->hash_algo));
	    if (t + strlen (", ") + 1 >= val + nb)
		goto again;

	    t = stpcpy(t, ", ");

	    /* this is important if sizeof(rpmuint32_t) ! sizeof(time_t) */
	    {	time_t dateint = pgpGrab(sigp->time, sizeof(sigp->time));
		struct tm * tstruct = localtime(&dateint);
		if (tstruct)
 		    (void) strftime(t, (nb - (t - val)), "%c", tstruct);
	    }
	    t += strlen(t);
	    if (t + strlen (", Key ID ") + 1 >= val + nb)
		goto again;
	    t = stpcpy(t, ", Key ID ");
	    tempstr = pgpHexStr(sigp->signid, sizeof(sigp->signid));
	    if (t + strlen (tempstr) > val + nb)
		goto again;
	    t = stpcpy(t, tempstr);

	    dig = pgpDigFree(dig);
	}
    }

    return val;
}

/**
 * Format dependency flags for display.
 * @param he		tag container
 * @param av		parameter list (or NULL)
 * @return		formatted string
 */
static /*@only@*/
char * depflagsFormat(HE_t he, /*@unused@*/ /*@null@*/ const char ** av)
	/*@*/
{
    int ix = (he->ix > 0 ? he->ix : 0);
    char * val;

assert(ix == 0);
    if (he->t != RPM_UINT64_TYPE) {
	val = xstrdup(_("(invalid type)"));
    } else {
	rpmuint64_t anint = he->p.ui64p[ix];
	char *t, *te;

	t = te = alloca(32);
	*te = '\0';

#ifdef	NOTYET	/* XXX appending markers breaks :depflags format. */
	if (anint & RPMSENSE_SCRIPT_PRE)
	    te = stpcpy(te, "(pre)");
	else if (anint & RPMSENSE_SCRIPT_POST)
	    te = stpcpy(te, "(post)");
	else if (anint & RPMSENSE_SCRIPT_PREUN)
	    te = stpcpy(te, "(preun)");
	else if (anint & RPMSENSE_SCRIPT_POSTUN)
	    te = stpcpy(te, "(postun)");
#endif
	if (anint & RPMSENSE_SENSEMASK)
	    *te++ = ' ';
	if (anint & RPMSENSE_LESS)
	    *te++ = '<';
	if (anint & RPMSENSE_GREATER)
	    *te++ = '>';
	if (anint & RPMSENSE_EQUAL)
	    *te++ = '=';
	if (anint & RPMSENSE_SENSEMASK)
	    *te++ = ' ';
	*te = '\0';

	val = xstrdup(t);
    }

    return val;
}

/**
 * Format dependency type for display.
 * @todo There's more sense bits, and the bits are attributes, not exclusive.
 * @param he		tag container
 * @param av		parameter list (or NULL)
 * @return		formatted string
 */
static /*@only@*/
char * deptypeFormat(HE_t he, /*@unused@*/ /*@null@*/ const char ** av)
	/*@*/
{
    int ix = (he->ix > 0 ? he->ix : 0);
    char * val;

assert(ix == 0);
    if (he->t != RPM_UINT64_TYPE) {
	val = xstrdup(_("(invalid type)"));
    } else {
	rpmuint64_t anint = he->p.ui64p[ix];
	char *t, *te;

	t = te = alloca(32);
	*te = '\0';

	if (anint & RPMSENSE_SCRIPT_PRE)
	    te = stpcpy(te, "pre");
	else if (anint & RPMSENSE_SCRIPT_POST)
	    te = stpcpy(te, "post");
	else if (anint & RPMSENSE_SCRIPT_PREUN)
	    te = stpcpy(te, "preun");
	else if (anint & RPMSENSE_SCRIPT_POSTUN)
	    te = stpcpy(te, "postun");
	else if (anint & RPMSENSE_SCRIPT_VERIFY)
	    te = stpcpy(te, "verify");
	else if (anint & RPMSENSE_RPMLIB)
	    te = stpcpy(te, "rpmlib");
	else if (anint & RPMSENSE_INTERP)
	    te = stpcpy(te, "interp");
	else if (anint & (RPMSENSE_FIND_PROVIDES | RPMSENSE_FIND_REQUIRES))
	    te = stpcpy(te, "auto");
	else
	    te = stpcpy(te, "manual");
	*te = '\0';

	val = xstrdup(t);
    }

    return val;
}

#ifdef NOTYET
static const char * bfstring(unsigned int x, const char * xbf)
{
    const char * s = xbf;
    static char digits[] = "0123456789abcdefghijklmnopqrstuvwxyz";
    static char buf[BUFSIZ];
    char * t, * te;
    unsigned radix;
    unsigned c, i, k;

    radix = (s != NULL ? *s++ : 16);

    if (radix <= 1 || radix >= 32)
	radix = 16;

    t = buf;
    switch (radix) {
    case 8:	*t++ = '0';	break;
    case 16:	*t++ = '0';	*t++ = 'x';	break;
    }

    i = 0;
    k = x;
    do { i++; k /= radix; } while (k);

    te = t + i;

    k = x;
    do { --i; t[i] = digits[k % radix]; k /= radix; } while (k);

    t = te;
    i = '<';
    if (s != NULL)
    while ((c = *s++) != '\0') {
	if (c > ' ') continue;

	k = (1 << (c - 1));
	if (!(x & k)) continue;

	if (t == te) *t++ = '=';

	*t++ = i;
	i = ',';
	while (*s > ' ')
	    *t++ = *s++;
    }
    if (t > te)	*t++ = '>';
    *t = '\0';
    return buf;
}
#endif

/**
 * Retrieve install prefixes.
 * @param h		header
 * @retval *he		tag container
 * @return		0 on success
 */
static int instprefixTag(Header h, HE_t he)
	/*@globals internalState @*/
	/*@modifies he, internalState @*/
{
    he->tag = RPMTAG_INSTALLPREFIX;
    if (headerGet(h, he, 0))
	return 0;

    he->tag = RPMTAG_INSTPREFIXES;
    if (headerGet(h, he, 0)) {
	rpmTagData array = { .argv = he->p.argv };
	he->t = RPM_STRING_TYPE;
	he->c = 1;
	he->p.str = xstrdup(array.argv[0]);
	he->freeData = 1;
	array.ptr = _free(array.ptr);
	return 0;
    }
    return 1;
}

/**
 * Convert unix timeval to UUIDv1.
 * @param h		header
 * @retval *he		tag container
 * @param tv		unix timeval
 * @return		0 on success
 */
static int tv2uuidv1(/*@unused@*/ Header h, HE_t he, struct timeval *tv)
	/*@modifies he @*/
{
    rpmuint64_t uuid_time = ((rpmuint64_t)tv->tv_sec * 10000000) +
			(tv->tv_usec * 10) + 0x01B21DD213814000ULL;

    he->t = RPM_BIN_TYPE;
    he->c = 128/8;
    he->p.ptr = xcalloc(1, he->c);
    he->freeData = 1;
    if (rpmuuidMake(1, NULL, NULL, NULL, (unsigned char *)he->p.ui8p)) {
	he->p.ptr = _free(he->p.ptr);
	he->freeData = 0;
	return 1;
    }

    he->p.ui8p[6] &= 0xf0;	/* preserve version, clear time_hi nibble */
    he->p.ui8p[8] &= 0xc0;	/* preserve variant, clear clock */
    he->p.ui8p[9] &= 0x00;

    he->p.ui8p[3] = (rpmuint8_t)(uuid_time >>  0);
    he->p.ui8p[2] = (rpmuint8_t)(uuid_time >>  8);
    he->p.ui8p[1] = (rpmuint8_t)(uuid_time >> 16);
    he->p.ui8p[0] = (rpmuint8_t)(uuid_time >> 24);
    he->p.ui8p[5] = (rpmuint8_t)(uuid_time >> 32);
    he->p.ui8p[4] = (rpmuint8_t)(uuid_time >> 40);
    he->p.ui8p[6] |= (rpmuint8_t)(uuid_time >> 56) & 0x0f;

#ifdef	NOTYET
    /* XXX Jigger up a non-zero (but constant) clock value. Is this needed? */
    he->p.ui8p[8] |= (he->p.ui8p[2] & 0x3f);
    he->p.ui8p[9] |= he->p.ui8p[3]
#endif

    return 0;
}

/**
 * Retrieve time and convert to UUIDv1.
 * @param h		header
 * @retval *he		tag container
 * @return		0 on success
 */
static int tag2uuidv1(Header h, HE_t he)
	/*@globals internalState @*/
	/*@modifies he, internalState @*/
{
    struct timeval tv;

    if (!headerGet(h, he, 0))
	return 1;
    tv.tv_sec = (long) he->p.ui32p[0];
    tv.tv_usec = (long) (he->c > 1 ? he->p.ui32p[1] : 0);
    he->p.ptr = _free(he->p.ptr);
    return tv2uuidv1(h, he, &tv);
}

/**
 * Retrieve install time and convert to UUIDv1.
 * @param h		header
 * @retval *he		tag container
 * @return		0 on success
 */
static int installtime_uuidTag(Header h, HE_t he)
	/*@globals internalState @*/
	/*@modifies he, internalState @*/
{
    he->tag = RPMTAG_INSTALLTIME;
    return tag2uuidv1(h, he);
}

/**
 * Retrieve build time and convert to UUIDv1.
 * @param h		header
 * @retval *he		tag container
 * @return		0 on success
 */
static int buildtime_uuidTag(Header h, HE_t he)
	/*@globals internalState @*/
	/*@modifies he, internalState @*/
{
    he->tag = RPMTAG_BUILDTIME;
    return tag2uuidv1(h, he);
}

/**
 * Retrieve origin time and convert to UUIDv1.
 * @param h		header
 * @retval *he		tag container
 * @return		0 on success
 */
static int origintime_uuidTag(Header h, HE_t he)
	/*@globals internalState @*/
	/*@modifies he, internalState @*/
{
    he->tag = RPMTAG_ORIGINTIME;
    return tag2uuidv1(h, he);
}

/**
 * Retrieve install tid and convert to UUIDv1.
 * @param h		header
 * @retval *he		tag container
 * @return		0 on success
 */
static int installtid_uuidTag(Header h, HE_t he)
	/*@globals internalState @*/
	/*@modifies he, internalState @*/
{
    he->tag = RPMTAG_INSTALLTID;
    return tag2uuidv1(h, he);
}

/**
 * Retrieve remove tid and convert to UUIDv1.
 * @param h		header
 * @retval *he		tag container
 * @return		0 on success
 */
static int removetid_uuidTag(Header h, HE_t he)
	/*@globals internalState @*/
	/*@modifies he, internalState @*/
{
    he->tag = RPMTAG_REMOVETID;
    return tag2uuidv1(h, he);
}

/**
 * Retrieve origin tid and convert to UUIDv1.
 * @param h		header
 * @retval *he		tag container
 * @return		0 on success
 */
static int origintid_uuidTag(Header h, HE_t he)
	/*@globals internalState @*/
	/*@modifies he, internalState @*/
{
    he->tag = RPMTAG_ORIGINTID;
    return tag2uuidv1(h, he);
}

/*@unchecked@*/ /*@observer@*/
static const char uuid_ns[] = "ns:URL";
/*@unchecked@*/ /*@observer@*/
static const char uuid_auth[] = "%{?_uuid_auth}%{!?_uuid_auth:http://rpm5.org}";
/*@unchecked@*/ /*@observer@*/
static const char uuid_path[] = "%{?_uuid_path}%{!?_uuid_path:/package}";
/*@unchecked@*/
static rpmuint32_t uuid_version = 5;

/**
 * Convert tag string to UUID.
 * @param he		tag container
 * @param av		parameter list (or NULL)
 * @param version	UUID version
 * @retval val		UUID string
 * @return		0 on success
 */
static int str2uuid(HE_t he, /*@unused@*/ /*@null@*/ const char ** av,
		rpmuint32_t version, char * val)
	/*@globals rpmGlobalMacroContext, h_errno, internalState @*/
	/*@modifies he, rpmGlobalMacroContext, internalState @*/
{
    const char * ns = NULL;
    const char * tagn = tagName(he->tag);
    const char * s = NULL;
char * t = (val ? val : alloca(40));
    int rc;

    /* XXX Substitute Pkgid & Hdrid strings for aliases. */
    if (!strcmp("Sigmd5", tagn))
	tagn = "Pkgid";
    else if (!strcmp("Sha1header", tagn))
	tagn = "Hdrid";

    switch (version) {
    default:
	version = uuid_version;
	/*@fallthrough@*/
    case 3:
    case 5:
assert(he->t == RPM_STRING_TYPE);
	ns = uuid_ns;
	s = rpmGetPath(uuid_auth, "/", uuid_path, "/", tagn, "/",
			he->p.str, NULL);
	/*@fallthrough@*/
    case 4:
	break;
    }
    he->p.ptr = _free(he->p.ptr);
    he->t = RPM_BIN_TYPE;
    he->c = 128/8;
    he->p.ptr = xcalloc(1, he->c);
    he->freeData = 1;
    rc = rpmuuidMake((int)version, ns, s, t, (unsigned char *)he->p.ui8p);
    if (rc) {
	he->p.ptr = _free(he->p.ptr);
	he->freeData = 0;
    }
    s = _free(s);

    return rc;
}

/**
 * Retrieve tag and convert to UUIDv5.
 * @param h		header
 * @retval *he		tag container
 * @return		0 on success
 */
static int tag2uuidv5(Header h, HE_t he)
	/*@globals rpmGlobalMacroContext, h_errno, internalState @*/
	/*@modifies he, rpmGlobalMacroContext, internalState @*/
{
    if (!headerGet(h, he, 0))
	return 1;
    switch (he->t) {
    default:
assert(0);
	/*@notreached@*/ break;
    case RPM_BIN_TYPE:	{	/* Convert RPMTAG_PKGID from binary => hex. */
	static const char hex[] = "0123456789abcdef";
	char * t;
	char * te;
	rpmuint32_t i;

	t = te = xmalloc (2*he->c + 1);
	for (i = 0; i < he->c; i++) {
	    *te++ = hex[ (int)((he->p.ui8p[i] >> 4) & 0x0f) ];
	    *te++ = hex[ (int)((he->p.ui8p[i]     ) & 0x0f) ];
	}
	*te = '\0';
	he->p.ptr = _free(he->p.ptr);
	he->t = RPM_STRING_TYPE;
	he->p.ptr = t;
	he->c = 1;
	he->freeData = 1;
    }	break;
    case RPM_STRING_TYPE:
	break;
    }
    return str2uuid(he, NULL, 0, NULL);
}

/**
 * Retrieve pkgid and convert to UUIDv5.
 * @param h		header
 * @retval *he		tag container
 * @return		0 on success
 */
static int pkguuidTag(Header h, HE_t he)
	/*@globals rpmGlobalMacroContext, h_errno, internalState @*/
	/*@modifies he, rpmGlobalMacroContext, internalState @*/
{
    he->tag = RPMTAG_PKGID;
    return tag2uuidv5(h, he);
}

/**
 * Retrieve sourcepkgid and convert to UUIDv5.
 * @param h		header
 * @retval *he		tag container
 * @return		0 on success
 */
static int sourcepkguuidTag(Header h, HE_t he)
	/*@globals rpmGlobalMacroContext, h_errno, internalState @*/
	/*@modifies he, rpmGlobalMacroContext, internalState @*/
{
    he->tag = RPMTAG_SOURCEPKGID;
    return tag2uuidv5(h, he);
}

/**
 * Retrieve hdrid and convert to UUIDv5.
 * @param h		header
 * @retval *he		tag container
 * @return		0 on success
 */
static int hdruuidTag(Header h, HE_t he)
	/*@globals rpmGlobalMacroContext, h_errno, internalState @*/
	/*@modifies he, rpmGlobalMacroContext, internalState @*/
{
    he->tag = RPMTAG_HDRID;
    return tag2uuidv5(h, he);
}

/**
 * Retrieve trigger info.
 * @param h		header
 * @retval *he		tag container
 * @return		0 on success
 */
static int triggercondsTag(Header h, HE_t he)
	/*@globals internalState @*/
	/*@modifies he, internalState @*/
{
    HE_t _he = (HE_t) memset(alloca(sizeof(*_he)), 0, sizeof(*_he));
    HE_t Fhe = (HE_t) memset(alloca(sizeof(*Fhe)), 0, sizeof(*Fhe));
    HE_t Ihe = (HE_t) memset(alloca(sizeof(*Ihe)), 0, sizeof(*Ihe));
    HE_t Nhe = (HE_t) memset(alloca(sizeof(*Nhe)), 0, sizeof(*Nhe));
    HE_t Vhe = (HE_t) memset(alloca(sizeof(*Vhe)), 0, sizeof(*Vhe));
    HE_t She = (HE_t) memset(alloca(sizeof(*She)), 0, sizeof(*She));
    rpmuint64_t anint;
    unsigned i, j;
    int rc = 1;		/* assume failure */
    int xx;

    he->freeData = 0;

    Nhe->tag = RPMTAG_TRIGGERNAME;
    xx = headerGet(h, Nhe, 0);
    if (!xx) {		/* no triggers, succeed anyways */
	rc = 0;
	goto exit;
    }

    Ihe->tag = RPMTAG_TRIGGERINDEX;
    xx = headerGet(h, Ihe, 0);
    if (!xx) goto exit;

    Fhe->tag = RPMTAG_TRIGGERFLAGS;
    xx = headerGet(h, Fhe, 0);
    if (!xx) goto exit;

    Vhe->tag = RPMTAG_TRIGGERVERSION;
    xx = headerGet(h, Vhe, 0);
    if (!xx) goto exit;

    She->tag = RPMTAG_TRIGGERSCRIPTS;
    xx = headerGet(h, She, 0);
    if (!xx) goto exit;

    _he->tag = he->tag;
    _he->t = RPM_UINT64_TYPE;
    _he->p.ui64p = &anint;
    _he->c = 1;
    _he->freeData = 0;

    he->t = RPM_STRING_ARRAY_TYPE;
    he->c = She->c;

    he->freeData = 1;
    he->p.argv = xmalloc(sizeof(*he->p.argv) * he->c);
    for (i = 0; i < (unsigned) he->c; i++) {
	char * item, * flagsStr;
	char * chptr;

	chptr = xstrdup("");

	for (j = 0; j < Nhe->c; j++) {
	    if (Ihe->p.ui32p[j] != i)
		/*@innercontinue@*/ continue;

	    item = xmalloc(strlen(Nhe->p.argv[j]) + strlen(Vhe->p.argv[j]) + 20);
/*@-compmempass@*/	/* use separate HE_t, not rpmTagData, containers. */
	    if (Fhe->p.ui32p[j] & RPMSENSE_SENSEMASK) {
		anint = Fhe->p.ui32p[j];
		flagsStr = depflagsFormat(_he, NULL);
		sprintf(item, "%s%s%s", Nhe->p.argv[j], flagsStr, Vhe->p.argv[j]);
		flagsStr = _free(flagsStr);
	    } else
		strcpy(item, Nhe->p.argv[j]);
/*@=compmempass@*/

	    chptr = xrealloc(chptr, strlen(chptr) + strlen(item) + 5);
	    if (*chptr != '\0') strcat(chptr, ", ");
	    strcat(chptr, item);
	    item = _free(item);
	}

	he->p.argv[i] = chptr;
    }
    rc = 0;

exit:
    Ihe->p.ptr = _free(Ihe->p.ptr);
    Fhe->p.ptr = _free(Fhe->p.ptr);
    Nhe->p.ptr = _free(Nhe->p.ptr);
    Vhe->p.ptr = _free(Vhe->p.ptr);
    She->p.ptr = _free(She->p.ptr);

    return rc;
}

/**
 * Retrieve trigger type info.
 * @param h		header
 * @retval *he		tag container
 * @return		0 on success
 */
static int triggertypeTag(Header h, HE_t he)
	/*@globals internalState @*/
	/*@modifies he, internalState @*/
{
    HE_t _he = (HE_t) memset(alloca(sizeof(*_he)), 0, sizeof(*_he));
    rpmTagData indices = { .ptr = NULL };
    rpmTagData flags = { .ptr = NULL };
    rpmTagData s = { .ptr = NULL };
    rpmTagCount numNames;
    rpmTagCount numScripts;
    unsigned i, j;
    int rc = 1;		/* assume failure */
    int xx;

    he->freeData = 0;

/*@-compmempass@*/	/* use separate HE_t, not rpmTagData, containers. */
    _he->tag = RPMTAG_TRIGGERINDEX;
    xx = headerGet(h, _he, 0);
    if (!xx) goto exit;
    indices.ui32p = _he->p.ui32p;
    numNames = _he->c;

    _he->tag = RPMTAG_TRIGGERFLAGS;
    xx = headerGet(h, _he, 0);
    if (!xx) goto exit;
    flags.ui32p = _he->p.ui32p;

    _he->tag = RPMTAG_TRIGGERSCRIPTS;
    xx = headerGet(h, _he, 0);
    if (!xx) goto exit;
    s.argv = _he->p.argv;
    numScripts = _he->c;
/*@=compmempass@*/

    he->t = RPM_STRING_ARRAY_TYPE;
    he->c = numScripts;

    he->freeData = 1;
    he->p.argv = xmalloc(sizeof(*he->p.argv) * he->c);
    for (i = 0; i < (unsigned) he->c; i++) {
	for (j = 0; j < (unsigned) numNames; j++) {
	    if (indices.ui32p[j] != i)
		/*@innercontinue@*/ continue;

	    /* XXX FIXME: there's memory leaks here. */
	    if (flags.ui32p[j] & RPMSENSE_TRIGGERPREIN)
		he->p.argv[i] = xstrdup("prein");
	    else if (flags.ui32p[j] & RPMSENSE_TRIGGERIN)
		he->p.argv[i] = xstrdup("in");
	    else if (flags.ui32p[j] & RPMSENSE_TRIGGERUN)
		he->p.argv[i] = xstrdup("un");
	    else if (flags.ui32p[j] & RPMSENSE_TRIGGERPOSTUN)
		he->p.argv[i] = xstrdup("postun");
	    else
		he->p.argv[i] = xstrdup("");
	    /*@innerbreak@*/ break;
	}
    }
    rc = 0;

exit:
    indices.ptr = _free(indices.ptr);
    flags.ptr = _free(flags.ptr);
    s.ptr = _free(s.ptr);
    return 0;
}

/* I18N look aside diversions */

#if defined(ENABLE_NLS)
/*@-exportlocal -exportheadervar@*/
/*@unchecked@*/
extern int _nl_msg_cat_cntr;	/* XXX GNU gettext voodoo */
/*@=exportlocal =exportheadervar@*/
#endif
/*@observer@*/ /*@unchecked@*/
static const char * language = "LANGUAGE";

/*@observer@*/ /*@unchecked@*/
static const char * _macro_i18ndomains = "%{?_i18ndomains}";

/**
 * Retrieve i18n text.
 * @param h		header
 * @retval *he		tag container
 * @return		0 on success
 */
static int i18nTag(Header h, HE_t he)
	/*@globals rpmGlobalMacroContext, h_errno, internalState @*/
	/*@modifies he, rpmGlobalMacroContext, internalState @*/
{
    char * dstring = rpmExpand(_macro_i18ndomains, NULL);
    int rc = 1;		/* assume failure */

    he->t = RPM_STRING_TYPE;
    he->p.str = NULL;
    he->c = 0;
    he->freeData = 0;

    if (dstring && *dstring) {
	char *domain, *de;
	const char * langval;
	const char * msgkey;
	const char * msgid;

	{   HE_t nhe = (HE_t) memset(alloca(sizeof(*nhe)), 0, sizeof(*nhe));
	    const char * tn;
	    char * mk;
	    size_t nb = sizeof("()");
	    int xx;

	    nhe->tag = RPMTAG_NAME;
	    xx = headerGet(h, nhe, 0);
	    /*
	     * XXX Ick, tagName() is called by headerGet(), and the tagName()
	     * buffer is valid only until next tagName() call.
	     * For now, do the tagName() lookup after headerGet().
	     */
	    tn = tagName(he->tag);
	    if (tn)	nb += strlen(tn);
	    if (nhe->p.str)	nb += strlen(nhe->p.str);
	    mk = alloca(nb);
	    (void) snprintf(mk, nb, "%s(%s)",
			(nhe->p.str ? nhe->p.str : ""), (tn ? tn : ""));
	    mk[nb-1] = '\0';
	    nhe->p.ptr = _free(nhe->p.ptr);
	    msgkey = mk;
	}

	/* change to en_US for msgkey -> msgid resolution */
	langval = getenv(language);
	(void) setenv(language, "en_US", 1);
#if defined(ENABLE_NLS)
/*@i@*/	++_nl_msg_cat_cntr;
#endif

	msgid = NULL;
	for (domain = dstring; domain != NULL; domain = de) {
	    de = strchr(domain, ':');
	    if (de) *de++ = '\0';
/*@-unrecog@*/
	    msgid = dgettext(domain, msgkey);
/*@=unrecog@*/
	    if (msgid != msgkey) break;
	}

	/* restore previous environment for msgid -> msgstr resolution */
	if (langval)
	    (void) setenv(language, langval, 1);
	else
	    unsetenv(language);
#if defined(ENABLE_NLS)
/*@i@*/	++_nl_msg_cat_cntr;
#endif

	if (domain && msgid) {
/*@-unrecog@*/
	    const char * s = dgettext(domain, msgid);
/*@=unrecog@*/
	    if (s) {
		rc = 0;
		he->p.str = xstrdup(s);
		he->c = 1;
		he->freeData = 1;
	    }
	}
    }

/*@-dependenttrans@*/
    dstring = _free(dstring);
/*@=dependenttrans@*/
    if (!rc)
	return rc;

    rc = headerGet(h, he, HEADERGET_NOEXTENSION);
    if (rc) {
	rc = 0;
	he->p.str = xstrtolocale(he->p.str);
	he->freeData = 1;
	return rc;
    }

    he->t = RPM_STRING_TYPE;
    he->p.str = NULL;
    he->c = 0;
    he->freeData = 0;

    return 1;
}

/**
 * Retrieve text and convert to locale.
 */
static int localeTag(Header h, HE_t he)
	/*@globals internalState @*/
	/*@modifies he, internalState @*/
{
    int rc;

    rc = headerGet(h, he, HEADERGET_NOEXTENSION);
    if (!rc || he->p.str == NULL || he->c == 0) {
	he->t = RPM_STRING_TYPE;
	he->freeData = 0;
	return 1;
    }

    switch (he->t) {
    default:
	he->freeData = 0;
	break;
    case RPM_STRING_TYPE:
	he->p.str = xstrtolocale(he->p.str);
	he->freeData = 1;
	break;
    case RPM_STRING_ARRAY_TYPE:
    {	const char ** argv;
	char * te;
	size_t l = 0;
	unsigned i;
	for (i = 0; i < (unsigned) he->c; i++) {
	    he->p.argv[i] = xstrdup(he->p.argv[i]);
	    he->p.argv[i] = xstrtolocale(he->p.argv[i]);
assert(he->p.argv[i] != NULL);
	    l += strlen(he->p.argv[i]) + 1;
	}
	argv = xmalloc(he->c * sizeof(*argv) + l);
	te = (char *)&argv[he->c];
	for (i = 0; i < (unsigned) he->c; i++) {
	    argv[i] = te;
	    te = stpcpy(te, he->p.argv[i]);
	    te++;
	    he->p.argv[i] = _free(he->p.argv[i]);
	}
	he->p.ptr = _free(he->p.ptr);
	he->p.argv = argv;
	he->freeData = 1;
    }	break;
    }

    return 0;
}

/**
 * Retrieve summary text.
 * @param h		header
 * @retval *he		tag container
 * @return		0 on success
 */
static int summaryTag(Header h, HE_t he)
	/*@globals rpmGlobalMacroContext, h_errno, internalState @*/
	/*@modifies he, rpmGlobalMacroContext, internalState @*/
{
    he->tag = RPMTAG_SUMMARY;
    return i18nTag(h, he);
}

/**
 * Retrieve description text.
 * @param h		header
 * @retval *he		tag container
 * @return		0 on success
 */
static int descriptionTag(Header h, HE_t he)
	/*@globals rpmGlobalMacroContext, h_errno, internalState @*/
	/*@modifies he, rpmGlobalMacroContext, internalState @*/
{
    he->tag = RPMTAG_DESCRIPTION;
    return i18nTag(h, he);
}

static int changelognameTag(Header h, HE_t he)
	/*@globals internalState @*/
	/*@modifies he, internalState @*/
{
    he->tag = RPMTAG_CHANGELOGNAME;
    return localeTag(h, he);
}

static int changelogtextTag(Header h, HE_t he)
	/*@globals internalState @*/
	/*@modifies he, internalState @*/
{
    he->tag = RPMTAG_CHANGELOGTEXT;
    return localeTag(h, he);
}

/**
 * Retrieve group text.
 * @param h		header
 * @retval *he		tag container
 * @return		0 on success
 */
static int groupTag(Header h, HE_t he)
	/*@globals rpmGlobalMacroContext, h_errno, internalState @*/
	/*@modifies he, rpmGlobalMacroContext, internalState @*/
{
    he->tag = RPMTAG_GROUP;
    return i18nTag(h, he);
}

/**
 * Retrieve db instance from header.
 * @param h		header
 * @retval *he		tag container
 * @return		0 on success
 */
static int dbinstanceTag(Header h, HE_t he)
	/*@modifies he @*/
{
    he->tag = RPMTAG_DBINSTANCE;
    he->t = RPM_UINT32_TYPE;
    he->p.ui32p = xmalloc(sizeof(*he->p.ui32p));
    he->p.ui32p[0] = headerGetInstance(h);
    he->freeData = 1;
    he->c = 1;
    return 0;
}

/**
 * Retrieve starting byte offset of header.
 * @param h		header
 * @retval *he		tag container
 * @return		0 on success
 */
static int headerstartoffTag(Header h, HE_t he)
	/*@modifies he @*/
{
    he->tag = RPMTAG_HEADERSTARTOFF;
    he->t = RPM_UINT64_TYPE;
    he->p.ui64p = xmalloc(sizeof(*he->p.ui64p));
    he->p.ui64p[0] = headerGetStartOff(h);
    he->freeData = 1;
    he->c = 1;
    return 0;
}

/**
 * Retrieve ending byte offset of header.
 * @param h		header
 * @retval *he		tag container
 * @return		0 on success
 */
static int headerendoffTag(Header h, HE_t he)
	/*@modifies he @*/
{
    he->tag = RPMTAG_HEADERENDOFF;
    he->t = RPM_UINT64_TYPE;
    he->p.ui64p = xmalloc(sizeof(*he->p.ui64p));
    he->p.ui64p[0] = headerGetEndOff(h);
    he->freeData = 1;
    he->c = 1;
    return 0;
}

/**
 * Retrieve package origin from header.
 * @param h		header
 * @retval *he		tag container
 * @return		0 on success
 */
static int pkgoriginTag(Header h, HE_t he)
	/*@globals internalState @*/
	/*@modifies he, internalState @*/
{
    const char * origin;
    int rc = 1;

    he->tag = RPMTAG_PACKAGEORIGIN;
    if (!headerGet(h, he, HEADERGET_NOEXTENSION)
     && (origin = headerGetOrigin(h)) != NULL)
    {
	he->t = RPM_STRING_TYPE;
	he->p.str = xstrdup(origin);
	he->c = 1;
	he->freeData = 1;
	rc = 0;
    }
    return rc;
}

/**
 * Retrieve package baseurl from header.
 * @param h		header
 * @retval *he		tag container
 * @return		0 on success
 */
static int pkgbaseurlTag(Header h, HE_t he)
	/*@globals internalState @*/
	/*@modifies he, internalState @*/
{
    const char * baseurl;
    int rc = 1;

    he->tag = RPMTAG_PACKAGEBASEURL;
    if (!headerGet(h, he, HEADERGET_NOEXTENSION)
     && (baseurl = headerGetBaseURL(h)) != NULL)
    {
	he->t = RPM_STRING_TYPE;
	he->p.str = xstrdup(baseurl);
	he->c = 1;
	he->freeData = 1;
	rc = 0;
    }
    return rc;
}

/**
 * Retrieve package digest from header.
 * @param h		header
 * @retval *he		tag container
 * @return		0 on success
 */
static int pkgdigestTag(Header h, HE_t he)
	/*@modifies he @*/
{
    const char * digest;
    int rc = 1;

    he->tag = RPMTAG_PACKAGEDIGEST;
    if ((digest = headerGetDigest(h)) != NULL)
    {
	he->t = RPM_STRING_TYPE;
	he->p.str = xstrdup(digest);
	he->c = 1;
	he->freeData = 1;
	rc = 0;
    }
    return rc;
}

/**
 * Retrieve *.rpm package st->st_mtime from header.
 * @param h		header
 * @retval *he		tag container
 * @return		0 on success
 */
static int pkgmtimeTag(Header h, HE_t he)
	/*@modifies he @*/
{
    struct stat * st = headerGetStatbuf(h);
    he->tag = RPMTAG_PACKAGETIME;
    he->t = RPM_UINT64_TYPE;
    he->p.ui64p = xmalloc(sizeof(*he->p.ui64p));
/*@-type@*/
    he->p.ui64p[0] = (rpmuint64_t)st->st_mtime;
/*@=type@*/
    he->freeData = 1;
    he->c = 1;
    return 0;
}

/**
 * Retrieve *.rpm package st->st_size from header.
 * @param h		header
 * @retval *he		tag container
 * @return		0 on success
 */
static int pkgsizeTag(Header h, HE_t he)
	/*@modifies he @*/
{
    struct stat * st = headerGetStatbuf(h);
    he->tag = RPMTAG_PACKAGESIZE;
    he->t = RPM_UINT64_TYPE;
    he->p.ui64p = xmalloc(sizeof(*he->p.ui64p));
    he->p.ui64p[0] = (rpmuint64_t)st->st_size;
    he->freeData = 1;
    he->c = 1;
    return 0;
}

/**
 * Return (malloc'd) header name-version-release.arch string.
 * @param h		header
 * @return		name-version-release.arch string
 */
/*@only@*/
static char * hGetNVRA(Header h)
	/*@globals internalState @*/
	/*@modifies h, internalState @*/
{
    const char * N = NULL;
    const char * V = NULL;
    const char * R = NULL;
    const char * A = NULL;
    size_t nb = 0;
    char * NVRA, * t;

    (void) headerNEVRA(h, &N, NULL, &V, &R, &A);
    if (N)	nb += strlen(N);
    if (V)	nb += strlen(V) + 1;
    if (R)	nb += strlen(R) + 1;
#if defined(RPM_VENDOR_OPENPKG) /* no-architecture-expose */
    /* do not expose the architecture as this is too less
       information, as in OpenPKG the "platform" is described by the
       architecture+operating-system combination. But as the whole
       "platform" information is actually overkill, just revert to the
       RPM 4 behaviour and do not expose any such information at all. */
#else
    if (A)	nb += strlen(A) + 1;
#endif
    nb++;
    NVRA = t = xmalloc(nb);
    *t = '\0';
    if (N)	t = stpcpy(t, N);
    if (V)	t = stpcpy( stpcpy(t, "-"), V);
    if (R)	t = stpcpy( stpcpy(t, "-"), R);
#if defined(RPM_VENDOR_OPENPKG) /* no-architecture-expose */
    /* do not expose the architecture as this is too less
       information, as in OpenPKG the "platform" is described by the
       architecture+operating-system combination. But as the whole
       "platform" information is actually overkill, just revert to the
       RPM 4 behaviour and do not expose any such information at all. */
#else
    if (A)	t = stpcpy( stpcpy(t, "."), A);
#endif
    N = _free(N);
    V = _free(V);
    R = _free(R);
    A = _free(A);
    return NVRA;
}

/**
 * Retrieve N-V-R.A compound string from header.
 * @param h		header
 * @retval *he		tag container
 * @return		0 on success
 */
static int nvraTag(Header h, HE_t he)
	/*@globals internalState @*/
	/*@modifies h, he, internalState @*/
{
    he->t = RPM_STRING_TYPE;
    he->p.str = hGetNVRA(h);
    he->c = 1;
    he->freeData = 1;
    return 0;
}

/**
 * Retrieve file names from header.
 *
 * The representation of file names in package headers changed in rpm-4.0.
 * Originally, file names were stored as an array of absolute paths.
 * In rpm-4.0, file names are stored as separate arrays of dirname's and
 * basename's, * with a dirname index to associate the correct dirname
 * with each basename.
 *
 * This function is used to retrieve file names independent of how the
 * file names are represented in the package header.
 * 
 * @param h		header
 * @param tagN		RPMTAG_BASENAMES | PMTAG_ORIGBASENAMES
 * @retval *fnp		array of file names
 * @retval *fcp		number of files
 */
static void rpmfiBuildFNames(Header h, rpmTag tagN,
		/*@null@*/ /*@out@*/ const char *** fnp,
		/*@null@*/ /*@out@*/ rpmTagCount * fcp)
	/*@globals internalState @*/
	/*@modifies *fnp, *fcp, internalState @*/
{
    HE_t he = (HE_t) memset(alloca(sizeof(*he)), 0, sizeof(*he));
    rpmTag dirNameTag = 0;
    rpmTag dirIndexesTag = 0;
    rpmTagData baseNames = { .ptr = NULL };
    rpmTagData dirNames = { .ptr = NULL };
    rpmTagData dirIndexes = { .ptr = NULL };
    rpmTagData fileNames;
    rpmTagCount count;
    size_t size;
    int isSource =
	(headerIsEntry(h, RPMTAG_SOURCERPM) == 0 &&
	 headerIsEntry(h, RPMTAG_ARCH) != 0);
    char * t;
    unsigned i;
    int xx;

    if (tagN == RPMTAG_BASENAMES) {
	dirNameTag = RPMTAG_DIRNAMES;
	dirIndexesTag = RPMTAG_DIRINDEXES;
    } else if (tagN == RPMTAG_ORIGBASENAMES) {
	dirNameTag = RPMTAG_ORIGDIRNAMES;
	dirIndexesTag = RPMTAG_ORIGDIRINDEXES;
    } else {
	if (fnp) *fnp = NULL;
	if (fcp) *fcp = 0;
	return;		/* programmer error */
    }

/*@-compmempass@*/	/* use separate HE_t, not rpmTagData, containers. */
    he->tag = tagN;
    xx = headerGet(h, he, 0);
    /* XXX 3.0.x SRPM's can be used, relative fn's at RPMTAG_OLDFILENAMES. */
    if (xx == 0 && isSource) {
	he->tag = RPMTAG_OLDFILENAMES;
	xx = headerGet(h, he, 0);
	if (xx) {
	    dirNames.argv = xcalloc(3, sizeof(*dirNames.argv));
	    dirNames.argv[0] = (const char *)&dirNames.argv[2];
	    dirIndexes.ui32p  = xcalloc(he->c, sizeof(*dirIndexes.ui32p));
	}
    }
    baseNames.argv = he->p.argv;
    count = he->c;

    if (!xx) {
	if (fnp) *fnp = NULL;
	if (fcp) *fcp = 0;
	return;		/* no file list */
    }

    he->tag = dirNameTag;
    if ((xx = headerGet(h, he, 0)) != 0)
	dirNames.argv = he->p.argv;

    he->tag = dirIndexesTag;
    if ((xx = headerGet(h, he, 0)) != 0)
	dirIndexes.ui32p = he->p.ui32p;
/*@=compmempass@*/

    size = sizeof(*fileNames.argv) * count;
    for (i = 0; i < (unsigned)count; i++) {
	const char * dn = NULL;
	(void) urlPath(dirNames.argv[dirIndexes.ui32p[i]], &dn);
	size += strlen(baseNames.argv[i]) + strlen(dn) + 1;
    }

    fileNames.argv = xmalloc(size);
    t = (char *)&fileNames.argv[count];
    for (i = 0; i < (unsigned)count; i++) {
	const char * dn = NULL;
	(void) urlPath(dirNames.argv[dirIndexes.ui32p[i]], &dn);
	fileNames.argv[i] = t;
	t = stpcpy( stpcpy(t, dn), baseNames.argv[i]);
	*t++ = '\0';
    }
    baseNames.ptr = _free(baseNames.ptr);
    dirNames.ptr = _free(dirNames.ptr);
    dirIndexes.ptr = _free(dirIndexes.ptr);

/*@-onlytrans@*/
    if (fnp)
	*fnp = fileNames.argv;
    else
	fileNames.ptr = _free(fileNames.ptr);
/*@=onlytrans@*/
    if (fcp) *fcp = count;
}

/**
 * Retrieve file paths.
 * @param h		header
 * @retval *he		tag container
 * @param tag		RPMTAG_BASENAMES or RPMTAG_ORIGBASENAMES
 * @return		0 on success
 */
static int _fnTag(Header h, HE_t he, rpmTag tag)
	/*@globals internalState @*/
	/*@modifies he, internalState @*/
{
    he->t = RPM_STRING_ARRAY_TYPE;
    rpmfiBuildFNames(h, tag, &he->p.argv, &he->c);
    he->freeData = 1;
    /* XXX headerGet() rc on RPMTAG_FILEPATHS w empty list. */
    if (he->p.argv && he->p.argv[0] && he->c > 0)
	return 0;
    he->p.ptr = _free(he->p.ptr);
    he->c = 0;
    return 1;
}

static int filenamesTag(Header h, HE_t he)
	/*@globals internalState @*/
	/*@modifies he, internalState @*/
{
    he->tag = tagValue("Filenames");
    return _fnTag(h, he, RPMTAG_BASENAMES);
}

static int filepathsTag(Header h, HE_t he)
	/*@globals internalState @*/
	/*@modifies he, internalState @*/
{
    he->tag = RPMTAG_FILEPATHS;
    return _fnTag(h, he, RPMTAG_BASENAMES);
}

static int origpathsTag(Header h, HE_t he)
	/*@globals internalState @*/
	/*@modifies he, internalState @*/
{
    he->tag = RPMTAG_ORIGPATHS;
    return _fnTag(h, he, RPMTAG_ORIGBASENAMES);
}

/**
 * Return Debian formatted dependencies as string array.
 * @param h		header
 * @retval *he		tag container
 * @param Nhe		dependency name container
 * @param EVRhe		dependency epoch:version-release container
 * @param Fhe		dependency flags container
 * @return		0 on success
 */
static int debevrfmtTag(/*@unused@*/ Header h, HE_t he,
		HE_t Nhe, HE_t EVRhe, HE_t Fhe)
	/*@globals rpmGlobalMacroContext, h_errno, internalState @*/
	/*@modifies he, Nhe, rpmGlobalMacroContext, internalState @*/
{
    char * t, * te;
    size_t nb = 0;
    int rc = 1;

    he->t = RPM_STRING_ARRAY_TYPE;
    he->c = 0;
    he->freeData = 1;
    for (Nhe->ix = 0; Nhe->ix < (int)Nhe->c; Nhe->ix++) {
	nb += sizeof(*he->p.argv);
	nb += strlen(Nhe->p.argv[Nhe->ix]) + 1;
	if (*EVRhe->p.argv[Nhe->ix] != '\0')
	    nb += strlen(EVRhe->p.argv[Nhe->ix]) + (sizeof(" (== )")-1);
	he->c++;
    }
    nb += sizeof(*he->p.argv);

    he->p.argv = xmalloc(nb);
    te = (char *) &he->p.argv[he->c+1];

    he->c = 0;
    for (Nhe->ix = 0; Nhe->ix < (int)Nhe->c; Nhe->ix++) {
	he->p.argv[he->c++] = te;
	if (*EVRhe->p.argv[Nhe->ix] != '\0') {
	    char opstr[4], * op = opstr;
	    if (Fhe->p.ui32p[Nhe->ix] & RPMSENSE_LESS)
		*op++ = '<';
	    if (Fhe->p.ui32p[Nhe->ix] & RPMSENSE_GREATER)
		*op++ = '>';
	    if (Fhe->p.ui32p[Nhe->ix] & RPMSENSE_EQUAL)
		*op++ = '=';
	    *op = '\0';
	    t = rpmExpand(Nhe->p.argv[Nhe->ix],
			" (", opstr, " ", EVRhe->p.argv[Nhe->ix], ")", NULL);
	} else
	    t = rpmExpand(Nhe->p.argv[Nhe->ix], NULL);
	te = stpcpy(te, t);
	te++;
	t = _free(t);
    }
    he->p.argv[he->c] = NULL;
    rc = 0;

    return rc;
}

/**
 * Retrieve and return Debian formatted dependecies for --deb:control.
 * @param h		header
 * @retval *he		tag container
 * @param tagN		dependency tag name
 * @param tagEVR	dependency tag epoch:version-release
 * @param tagF		dependency tag flags
 * @return		0 on success
 */
static int debevrTag(Header h, HE_t he, rpmTag tagN, rpmTag tagEVR, rpmTag tagF)
	/*@globals rpmGlobalMacroContext, h_errno, internalState @*/
	/*@modifies he, rpmGlobalMacroContext, internalState @*/
{
    HE_t Nhe = (HE_t) memset(alloca(sizeof(*Nhe)), 0, sizeof(*Nhe));
    HE_t EVRhe = (HE_t) memset(alloca(sizeof(*EVRhe)), 0, sizeof(*EVRhe));
    HE_t Fhe = (HE_t) memset(alloca(sizeof(*Fhe)), 0, sizeof(*Fhe));
    int rc = 1;
    int xx;

    Nhe->tag = tagN;
    if (!(xx = headerGet(h, Nhe, 0)))
	goto exit;
    EVRhe->tag = tagEVR;
    if (!(xx = headerGet(h, EVRhe, 0)))
	goto exit;
assert(EVRhe->c == Nhe->c);
    Fhe->tag = tagF;
    if (!(xx = headerGet(h, Fhe, 0)))
	goto exit;
assert(Fhe->c == Nhe->c);

    rc = debevrfmtTag(h, he, Nhe, EVRhe, Fhe);

exit:
    Nhe->p.ptr = _free(Nhe->p.ptr);
    EVRhe->p.ptr = _free(EVRhe->p.ptr);
    Fhe->p.ptr = _free(Fhe->p.ptr);
    return rc;
}

/**
 * Retrieve Depends: and Conflicts: for --deb:control.
 * @param h		header
 * @retval *he		tag container
 * @return		0 on success
 */
static int debconflictsTag(Header h, HE_t he)
	/*@globals rpmGlobalMacroContext, h_errno, internalState @*/
	/*@modifies he, rpmGlobalMacroContext, internalState @*/
{
    he->tag = tagValue("Debconflicts");
    return debevrTag(h, he,
	RPMTAG_CONFLICTNAME, RPMTAG_CONFLICTVERSION, RPMTAG_CONFLICTFLAGS);
}

static int debdependsTag(Header h, HE_t he)
	/*@globals rpmGlobalMacroContext, h_errno, internalState @*/
	/*@modifies he, rpmGlobalMacroContext, internalState @*/
{
    he->tag = tagValue("Debdepends");
    return debevrTag(h, he,
	RPMTAG_REQUIRENAME, RPMTAG_REQUIREVERSION, RPMTAG_REQUIREFLAGS);
}

static int debobsoletesTag(Header h, HE_t he)
	/*@globals rpmGlobalMacroContext, h_errno, internalState @*/
	/*@modifies he, rpmGlobalMacroContext, internalState @*/
{
    he->tag = tagValue("Debobsoletes");
    return debevrTag(h, he,
	RPMTAG_OBSOLETENAME, RPMTAG_OBSOLETEVERSION, RPMTAG_OBSOLETEFLAGS);
}

static int debprovidesTag(Header h, HE_t he)
	/*@globals rpmGlobalMacroContext, h_errno, internalState @*/
	/*@modifies he, rpmGlobalMacroContext, internalState @*/
{
    he->tag = tagValue("Debprovides");
    return debevrTag(h, he,
	RPMTAG_PROVIDENAME, RPMTAG_PROVIDEVERSION, RPMTAG_PROVIDEFLAGS);
}

/**
 * Retrieve digest/path pairs for --deb:md5sums.
 * @param h		header
 * @retval *he		tag container
 * @return		0 on success
 */
static int debmd5sumsTag(Header h, HE_t he)
	/*@globals rpmGlobalMacroContext, h_errno, internalState @*/
	/*@modifies he, rpmGlobalMacroContext, internalState @*/
{
    HE_t Nhe = (HE_t) memset(alloca(sizeof(*Nhe)), 0, sizeof(*Nhe));
    HE_t Dhe = (HE_t) memset(alloca(sizeof(*Dhe)), 0, sizeof(*Dhe));
    char * t, * te;
    size_t nb = 0;
    int rc = 1;
    int xx;

    Nhe->tag = RPMTAG_FILEPATHS;
    if (!(xx = headerGet(h, Nhe, 0)))
	goto exit;
    Dhe->tag = RPMTAG_FILEDIGESTS;
    if (!(xx = headerGet(h, Dhe, 0)))
	goto exit;
assert(Dhe->c == Nhe->c);

    he->tag = tagValue("Debmd5sums");
    he->t = RPM_STRING_ARRAY_TYPE;
    he->c = 0;
    he->freeData = 1;
    for (Dhe->ix = 0; Dhe->ix < (int)Dhe->c; Dhe->ix++) {
	if (!(Dhe->p.argv[Dhe->ix] && *Dhe->p.argv[Dhe->ix]))
	    continue;
	nb += sizeof(*he->p.argv);
	nb += strlen(Dhe->p.argv[Dhe->ix]) + sizeof("  ") + strlen(Nhe->p.argv[Dhe->ix]) - 1;
	he->c++;
    }
    nb += sizeof(*he->p.argv);

    he->p.argv = xmalloc(nb);
    te = (char *) &he->p.argv[he->c+1];

    he->c = 0;
    for (Dhe->ix = 0; Dhe->ix < (int)Dhe->c; Dhe->ix++) {
	if (!(Dhe->p.argv[Dhe->ix] && *Dhe->p.argv[Dhe->ix]))
	    continue;
	he->p.argv[he->c++] = te;
	t = rpmExpand(Dhe->p.argv[Dhe->ix], "  ", Nhe->p.argv[Dhe->ix]+1, NULL);
	te = stpcpy(te, t);
	te++;
	t = _free(t);
    }
    he->p.argv[he->c] = NULL;
    rc = 0;

exit:
    Nhe->p.ptr = _free(Nhe->p.ptr);
    Dhe->p.ptr = _free(Dhe->p.ptr);
    return rc;
}

static int filestatTag(Header h, HE_t he)
	/*@globals internalState @*/
	/*@modifies he, internalState @*/
{
    rpmTagData paths = { .ptr = NULL };
    /* _dev */
    rpmTagData _ino = { .ptr = NULL };
    rpmTagData _mode = { .ptr = NULL };
    /* _nlink */
    /* _uid */
    /* _gid */
    rpmTagData _rdev = { .ptr = NULL };
    rpmTagData _size = { .ptr = NULL };
    /* _blksize */
    /* _blocks */
    /* _atime */
    rpmTagData _mtime = { .ptr = NULL };
    /* st_ctime */
    int rc;

    he->tag = RPMTAG_FILEPATHS;
    if ((rc = _fnTag(h, he, RPMTAG_BASENAMES)) != 0 || he->c == 0)
	goto exit;

exit:
    paths.ptr = _free(paths.ptr);
    _ino.ptr = _free(_ino.ptr);
    _mode.ptr = _free(_mode.ptr);
    _rdev.ptr = _free(_rdev.ptr);
    _size.ptr = _free(_size.ptr);
    _mtime.ptr = _free(_mtime.ptr);
    return rc;
}

static int wnlookupTag(Header h, rpmTag tagNVRA, ARGV_t *avp, ARGI_t *hitp,
		HE_t PNhe, /*@null@*/ HE_t PEVRhe, /*@null@*/ HE_t PFhe)
	/*@globals rpmGlobalMacroContext, h_errno,
		fileSystem, internalState @*/
	/*@modifies *avp, *hitp, rpmGlobalMacroContext,
		fileSystem, internalState @*/
{
    HE_t NVRAhe = (HE_t) memset(alloca(sizeof(*NVRAhe)), 0, sizeof(*NVRAhe));
    HE_t RNhe = (HE_t) memset(alloca(sizeof(*RNhe)), 0, sizeof(*RNhe));
    HE_t REVRhe = (HE_t) memset(alloca(sizeof(*REVRhe)), 0, sizeof(*REVRhe));
    HE_t RFhe = (HE_t) memset(alloca(sizeof(*RFhe)), 0, sizeof(*RFhe));
    rpmdb _rpmdb = (rpmdb) headerGetRpmdb(h);
    const char * key = PNhe->p.argv[PNhe->ix];
    size_t keylen = 0;
    rpmmi mi;
    rpmTag tagN = RPMTAG_REQUIRENAME;
    rpmTag tagEVR = RPMTAG_REQUIREVERSION;
    rpmTag tagF = RPMTAG_REQUIREFLAGS;
    rpmuint32_t PFlags;
    rpmuint32_t RFlags;
    EVR_t Pevr;
    Header oh;
    int rc = 0;
    int xx;

    if (tagNVRA == 0)
	tagNVRA = RPMTAG_NVRA;

    PFlags = (PFhe != NULL ? (PFhe->p.ui32p[PNhe->ix] & RPMSENSE_SENSEMASK) : 0);
    Pevr = rpmEVRnew(PFlags, 1);

    if (PEVRhe != NULL)
	xx = rpmEVRparse(PEVRhe->p.argv[PNhe->ix], Pevr);

    RNhe->tag = tagN;
    REVRhe->tag = tagEVR;
    RFhe->tag = tagF;

    mi = rpmmiInit(_rpmdb, tagN, key, keylen);
    if (hitp && *hitp)
	xx = rpmmiPrune(mi, (uint32_t *)argiData(*hitp), argiCount(*hitp), 0);
    while ((oh = rpmmiNext(mi)) != NULL) {
	if (!headerGet(oh, RNhe, 0))
	    goto bottom;
	if (PEVRhe != NULL) {
	    if (!headerGet(oh, REVRhe, 0))
		goto bottom;
assert(REVRhe->c == RNhe->c);
	    if (!headerGet(oh, RFhe, 0))
		goto bottom;
assert(RFhe->c == RNhe->c);
	}

	for (RNhe->ix = 0; RNhe->ix < (int)RNhe->c; RNhe->ix++) {
	    if (strcmp(PNhe->p.argv[PNhe->ix], RNhe->p.argv[RNhe->ix]))
		/*@innercontinue@*/ continue;
	    if (PEVRhe == NULL)
		goto bingo;
	    RFlags = RFhe->p.ui32p[RNhe->ix] & RPMSENSE_SENSEMASK;
	    {	EVR_t Revr = rpmEVRnew(RFlags, 1);
		if (!(PFlags && RFlags))
		    xx = 1;
		else {
		    xx = rpmEVRparse(REVRhe->p.argv[RNhe->ix], Revr);
		    xx = rpmEVRoverlap(Pevr, Revr);
		}
		Revr = rpmEVRfree(Revr);
	    }
	    if (xx)
		goto bingo;
	}
	goto bottom;

bingo:
	NVRAhe->tag = tagNVRA;
	xx = headerGet(oh, NVRAhe, 0);
	if (!(*avp != NULL && argvSearch(*avp, NVRAhe->p.str, NULL) != NULL)) {
	    xx = argvAdd(avp, NVRAhe->p.str);
	    xx = argvSort(*avp, NULL);
	    if (hitp != NULL)
		xx = argiAdd(hitp, -1, rpmmiInstance(mi));
	    rc++;
	}

bottom:
	RNhe->p.ptr = _free(RNhe->p.ptr);
	REVRhe->p.ptr = _free(REVRhe->p.ptr);
	RFhe->p.ptr = _free(RFhe->p.ptr);
	NVRAhe->p.ptr = _free(NVRAhe->p.ptr);
    }
    mi = rpmmiFree(mi);

    Pevr = rpmEVRfree(Pevr);

    return rc;
}

static int whatneedsTag(Header h, HE_t he)
	/*@globals rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
	/*@modifies he, rpmGlobalMacroContext, fileSystem, internalState @*/
{
    HE_t NVRAhe = (HE_t) memset(alloca(sizeof(*NVRAhe)), 0, sizeof(*NVRAhe));
    HE_t PNhe = (HE_t) memset(alloca(sizeof(*PNhe)), 0, sizeof(*PNhe));
    HE_t PEVRhe = (HE_t) memset(alloca(sizeof(*PEVRhe)), 0, sizeof(*PEVRhe));
    HE_t PFhe = (HE_t) memset(alloca(sizeof(*PFhe)), 0, sizeof(*PFhe));
    HE_t FNhe = (HE_t) memset(alloca(sizeof(*FNhe)), 0, sizeof(*FNhe));
    rpmTag tagNVRA = RPMTAG_NVRA;
    ARGV_t pkgs = NULL;
    ARGI_t hits = NULL;
    int rc = 1;

    PNhe->tag = RPMTAG_PROVIDENAME;
    if (!headerGet(h, PNhe, 0))
	goto exit;
    PEVRhe->tag = RPMTAG_PROVIDEVERSION;
    if (!headerGet(h, PEVRhe, 0))
	goto exit;
assert(PEVRhe->c == PNhe->c);
    PFhe->tag = RPMTAG_PROVIDEFLAGS;
    if (!headerGet(h, PFhe, 0))
	goto exit;
assert(PFhe->c == PNhe->c);

    FNhe->tag = RPMTAG_FILEPATHS;
    if (!headerGet(h, FNhe, 0))
	goto exit;

    NVRAhe->tag = tagNVRA;;
    if (!headerGet(h, NVRAhe, 0))
	goto exit;

    (void) argvAdd(&pkgs, NVRAhe->p.str);

    for (PNhe->ix = 0; PNhe->ix < (int)PNhe->c; PNhe->ix++)
	(void) wnlookupTag(h, tagNVRA, &pkgs, &hits, PNhe, PEVRhe, PFhe);
    for (FNhe->ix = 0; FNhe->ix < (int)FNhe->c; FNhe->ix++)
	(void) wnlookupTag(h, tagNVRA, &pkgs, &hits, FNhe, NULL, NULL);

    /* Convert package NVRA array to Header string array. */
    {	size_t nb = 0;
	char * te;
	rpmuint32_t i;

	he->t = RPM_STRING_ARRAY_TYPE;
	he->c = argvCount(pkgs);
	nb = 0;
	for (i = 0; i < he->c; i++) {
	    nb += sizeof(*he->p.argv);
	    nb += strlen(pkgs[i]) + 1;
	}
	nb += sizeof(*he->p.argv);

	he->p.argv = xmalloc(nb);
	te = (char *) &he->p.argv[he->c+1];

	for (i = 0; i < he->c; i++) {
	    he->p.argv[i] = te;
	    te = stpcpy(te, pkgs[i]);
	    te++;
	}
	he->p.argv[he->c] = NULL;
    }

    hits = argiFree(hits);
    pkgs = argvFree(pkgs);
    rc = 0;

exit:
    NVRAhe->p.ptr = _free(NVRAhe->p.ptr);
    PNhe->p.ptr = _free(PNhe->p.ptr);
    PEVRhe->p.ptr = _free(PEVRhe->p.ptr);
    PFhe->p.ptr = _free(PFhe->p.ptr);
    FNhe->p.ptr = _free(FNhe->p.ptr);
    return rc;
}

static int nwlookupTag(Header h, rpmTag tagNVRA, ARGV_t *avp, ARGI_t *hitp,
		HE_t RNhe, /*@null@*/ HE_t REVRhe, /*@null@*/ HE_t RFhe)
	/*@globals rpmGlobalMacroContext, h_errno,
		fileSystem, internalState @*/
	/*@modifies *avp, *hitp, REVRhe, rpmGlobalMacroContext,
		fileSystem, internalState @*/
{
    HE_t NVRAhe = (HE_t) memset(alloca(sizeof(*NVRAhe)), 0, sizeof(*NVRAhe));
    HE_t PNhe = (HE_t) memset(alloca(sizeof(*PNhe)), 0, sizeof(*PNhe));
    HE_t PEVRhe = (HE_t) memset(alloca(sizeof(*PEVRhe)), 0, sizeof(*PEVRhe));
    HE_t PFhe = (HE_t) memset(alloca(sizeof(*PFhe)), 0, sizeof(*PFhe));
    rpmdb _rpmdb = (rpmdb) headerGetRpmdb(h);
    const char * key = RNhe->p.argv[RNhe->ix];
    size_t keylen = 0;
    rpmmi mi;
    rpmTag tagN = (*RNhe->p.argv[RNhe->ix] == '/')
	? RPMTAG_BASENAMES : RPMTAG_PROVIDENAME;
    rpmTag tagEVR = RPMTAG_PROVIDEVERSION;
    rpmTag tagF = RPMTAG_PROVIDEFLAGS;
    rpmuint32_t PFlags;
    rpmuint32_t RFlags;
    EVR_t Revr;
    Header oh;
    int rc = 0;
    int xx;

    if (tagNVRA == 0)
	tagNVRA = RPMTAG_NVRA;

    RFlags = (RFhe != NULL ? (RFhe->p.ui32p[RNhe->ix] & RPMSENSE_SENSEMASK) : 0);
    Revr = rpmEVRnew(RFlags, 1);

    if (REVRhe != NULL)
	xx = rpmEVRparse(REVRhe->p.argv[RNhe->ix], Revr);

    PNhe->tag = tagN;
    PEVRhe->tag = tagEVR;
    PFhe->tag = tagF;

    mi = rpmmiInit(_rpmdb, tagN, key, keylen);
    if (hitp && *hitp)
	xx = rpmmiPrune(mi, (uint32_t *)argiData(*hitp), argiCount(*hitp), 0);
    while ((oh = rpmmiNext(mi)) != NULL) {
	if (!headerGet(oh, PNhe, 0))
	    goto bottom;
	if (REVRhe != NULL) {
	    if (!headerGet(oh, PEVRhe, 0))
		goto bottom;
assert(PEVRhe->c == PNhe->c);
	    if (!headerGet(oh, PFhe, 0))
		goto bottom;
assert(PFhe->c == PNhe->c);
	}

	for (PNhe->ix = 0; PNhe->ix < (int)PNhe->c; PNhe->ix++) {
	    if (strcmp(RNhe->p.argv[RNhe->ix], PNhe->p.argv[PNhe->ix]))
		/*@innercontinue@*/ continue;
	    if (REVRhe == NULL)
		goto bingo;
	    PFlags = PFhe->p.ui32p[PNhe->ix] & RPMSENSE_SENSEMASK;
	    {	EVR_t Pevr = rpmEVRnew(PFlags, 1);
		if (!(PFlags && RFlags))
		    xx = 1;
		else {
		    xx = rpmEVRparse(PEVRhe->p.argv[PNhe->ix], Pevr);
		    xx = rpmEVRoverlap(Revr, Pevr);
		}
		Pevr = rpmEVRfree(Pevr);
	    }
	    if (xx)
		goto bingo;
	}
	goto bottom;

bingo:
	NVRAhe->tag = tagNVRA;
	xx = headerGet(oh, NVRAhe, 0);
	if (!(*avp != NULL && argvSearch(*avp, NVRAhe->p.str, NULL) != NULL)) {
	    xx = argvAdd(avp, NVRAhe->p.str);
	    xx = argvSort(*avp, NULL);
	    if (hitp != NULL)
		xx = argiAdd(hitp, -1, rpmmiInstance(mi));
	    rc++;
	}

bottom:
	PNhe->p.ptr = _free(PNhe->p.ptr);
	PEVRhe->p.ptr = _free(PEVRhe->p.ptr);
	PFhe->p.ptr = _free(PFhe->p.ptr);
	NVRAhe->p.ptr = _free(NVRAhe->p.ptr);
    }
    mi = rpmmiFree(mi);

    Revr = rpmEVRfree(Revr);

    return rc;
}

static int needswhatTag(Header h, HE_t he)
	/*@globals rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
	/*@modifies he, rpmGlobalMacroContext, fileSystem, internalState @*/
{
    HE_t NVRAhe = (HE_t) memset(alloca(sizeof(*NVRAhe)), 0, sizeof(*NVRAhe));
    HE_t RNhe = (HE_t) memset(alloca(sizeof(*RNhe)), 0, sizeof(*RNhe));
    HE_t REVRhe = (HE_t) memset(alloca(sizeof(*REVRhe)), 0, sizeof(*REVRhe));
    HE_t RFhe = (HE_t) memset(alloca(sizeof(*RFhe)), 0, sizeof(*RFhe));
    rpmTag tagNVRA = RPMTAG_NVRA;
    ARGV_t pkgs = NULL;
    ARGI_t hits = NULL;
    int rc = 1;

    RNhe->tag = RPMTAG_REQUIRENAME;
    if (!headerGet(h, RNhe, 0))
	goto exit;
    REVRhe->tag = RPMTAG_REQUIREVERSION;
    if (!headerGet(h, REVRhe, 0))
	goto exit;
assert(REVRhe->c == RNhe->c);
    RFhe->tag = RPMTAG_REQUIREFLAGS;
    if (!headerGet(h, RFhe, 0))
	goto exit;
assert(RFhe->c == RNhe->c);

    NVRAhe->tag = tagNVRA;;
    if (!headerGet(h, NVRAhe, 0))
	goto exit;

    (void) argvAdd(&pkgs, NVRAhe->p.str);

    for (RNhe->ix = 0; RNhe->ix < (int)RNhe->c; RNhe->ix++) {
	if (*RNhe->p.argv[RNhe->ix] == '/' || *REVRhe->p.argv[RNhe->ix] == '\0')
	    (void) nwlookupTag(h, tagNVRA, &pkgs, &hits, RNhe, NULL, NULL);
	else
	    (void) nwlookupTag(h, tagNVRA, &pkgs, &hits, RNhe, REVRhe, RFhe);
    }

    /* Convert package NVRA array to Header string array. */
    {	size_t nb = 0;
	char * te;
	rpmuint32_t i;

	he->t = RPM_STRING_ARRAY_TYPE;
	he->c = argvCount(pkgs);
	nb = 0;
	for (i = 0; i < he->c; i++) {
	    nb += sizeof(*he->p.argv);
	    nb += strlen(pkgs[i]) + 1;
	}
	nb += sizeof(*he->p.argv);

	he->p.argv = xmalloc(nb);
	te = (char *) &he->p.argv[he->c+1];

	for (i = 0; i < he->c; i++) {
	    he->p.argv[i] = te;
	    te = stpcpy(te, pkgs[i]);
	    te++;
	}
	he->p.argv[he->c] = NULL;
    }

    hits = argiFree(hits);
    pkgs = argvFree(pkgs);
    rc = 0;

exit:
    NVRAhe->p.ptr = _free(NVRAhe->p.ptr);
    RNhe->p.ptr = _free(RNhe->p.ptr);
    REVRhe->p.ptr = _free(REVRhe->p.ptr);
    RFhe->p.ptr = _free(RFhe->p.ptr);
    return rc;
}

static int PRCOSkip(rpmTag tag, rpmTagData N, rpmTagData EVR, rpmTagData F,
		uint32_t i)
	/*@*/
{
    int a = -2, b = -2;
    int rc = 0;

assert(N.argv[i] != NULL && *N.argv[i] != '\0');

    if (tag == RPMTAG_REQUIRENAME && i > 0
     && !(a=strcmp(N.argv[i], N.argv[i-1]))
     && !(b=strcmp(EVR.argv[i], EVR.argv[i-1]))
     && (F.ui32p[i] & 0x4e) == ((F.ui32p[i-1] & 0x4e)) )
	rc = 1;
    return rc;
}

static int PRCOxmlTag(Header h, HE_t he, rpmTag EVRtag, rpmTag Ftag)
	/*@globals internalState @*/
	/*@modifies he, internalState @*/
{
    rpmTag tag = he->tag;
    rpmTagData N = { .ptr = NULL };
    rpmTagData EVR = { .ptr = NULL };
    rpmTagData F = { .ptr = NULL };
    size_t nb;
    uint32_t ac;
    uint32_t c;
    uint32_t i;
    char *t;
    int rc = 1;		/* assume failure */
    int xx;
int lvl = 0;
spew_t spew = &_xml_spew;

/*@-compmempass@*/	/* use separate HE_t, not rpmTagData, containers. */
    xx = headerGet(h, he, 0);
    if (xx == 0) goto exit;
    N.argv = he->p.argv;
    c = he->c;

    he->tag = EVRtag;
    xx = headerGet(h, he, 0);
    if (xx == 0) goto exit;
    EVR.argv = he->p.argv;

    he->tag = Ftag;
    xx = headerGet(h, he, 0);
    if (xx == 0) goto exit;
    F.ui32p = he->p.ui32p;

    nb = sizeof(*he->p.argv);
    ac = 0;
    for (i = 0; i < c; i++) {
/*@-nullstate@*/	/* EVR.argv might be NULL */
	if (PRCOSkip(tag, N, EVR, F, i))
	    continue;
/*@=nullstate@*/
	ac++;
	nb += sizeof(*he->p.argv);
	nb += sizeof("<rpm:entry name=\"\"/>");
	if (*N.argv[i] == '/')
	    nb += spew->spew_strlen(N.argv[i], lvl);
	else
	    nb += strlen(N.argv[i]);
	if (EVR.argv != NULL && EVR.argv[i] != NULL && *EVR.argv[i] != '\0') {
	    nb += sizeof(" flags=\"EQ\" epoch=\"0\" ver=\"\"") - 1;
	    nb += strlen(EVR.argv[i]);
	    if (strchr(EVR.argv[i], ':') != NULL)
		nb -= 2;
	    if (strchr(EVR.argv[i], '-') != NULL)
		nb += sizeof(" rel=\"\"") - 2;
	}
#ifdef	NOTNOW
	if (tag == RPMTAG_REQUIRENAME && (F.ui32p[i] & 0x40))
	    nb += sizeof(" pre=\"1\"") - 1;
#endif
    }

    he->t = RPM_STRING_ARRAY_TYPE;
    he->c = ac;
    he->freeData = 1;
    he->p.argv = xmalloc(nb + BUFSIZ);	/* XXX hack: leave slop */
    t = (char *) &he->p.argv[he->c + 1];
    ac = 0;
    for (i = 0; i < c; i++) {
/*@-nullstate@*/	/* EVR.argv might be NULL */
	if (PRCOSkip(tag, N, EVR, F, i))
	    continue;
/*@=nullstate@*/
	he->p.argv[ac++] = t;
	t = stpcpy(t, "<rpm:entry");
	t = stpcpy(t, " name=\"");
	if (*N.argv[i] == '/') {
	    t = spew->spew_strcpy(t, N.argv[i], lvl);	t += strlen(t);
	} else
	    t = stpcpy(t, N.argv[i]);
	t = stpcpy(t, "\"");
/*@-readonlytrans@*/
	if (EVR.argv != NULL && EVR.argv[i] != NULL && *EVR.argv[i] != '\0') {
	    static char *Fstr[] = { "?0","LT","GT","?3","EQ","LE","GE","?7" };
	    rpmuint32_t Fx = ((F.ui32p[i] >> 1) & 0x7);
	    const char *E, *V, *R;
	    char *f, *fe;
	    t = stpcpy( stpcpy( stpcpy(t, " flags=\""), Fstr[Fx]), "\"");
	    f = (char *) EVR.argv[i];
	    for (fe = f; *fe != '\0' && *fe >= '0' && *fe <= '9'; fe++)
		{};
	    if (*fe == ':') { *fe++ = '\0'; E = f; f = fe; } else E = NULL;
	    V = f;
	    for (fe = f; *fe != '\0' && *fe != '-'; fe++)
		{};
	    if (*fe == '-') { *fe++ = '\0'; R = fe; } else R = NULL;
	    t = stpcpy( stpcpy( stpcpy(t, " epoch=\""), (E && *E ? E : "0")), "\"");
	    t = stpcpy( stpcpy( stpcpy(t, " ver=\""), V), "\"");
	    if (R != NULL)
		t = stpcpy( stpcpy( stpcpy(t, " rel=\""), R), "\"");
	}
/*@=readonlytrans@*/
#ifdef	NOTNOW
	if (tag == RPMTAG_REQUIRENAME && (F.ui32p[i] & 0x40))
	    t = stpcpy(t, " pre=\"1\"");
#endif
	t = stpcpy(t, "/>");
	*t++ = '\0';
    }
    he->p.argv[he->c] = NULL;
/*@=compmempass@*/
    rc = 0;

exit:
/*@-kepttrans@*/	/* N.argv may be kept. */
    N.argv = _free(N.argv);
/*@=kepttrans@*/
/*@-usereleased@*/	/* EVR.argv may be dead. */
    EVR.argv = _free(EVR.argv);
/*@=usereleased@*/
    F.ui32p = _free(F.ui32p);
    return rc;
}

static int PxmlTag(Header h, HE_t he)
	/*@globals internalState @*/
	/*@modifies he, internalState @*/
{
    he->tag = RPMTAG_PROVIDENAME;
    return PRCOxmlTag(h, he, RPMTAG_PROVIDEVERSION, RPMTAG_PROVIDEFLAGS);
}

static int RxmlTag(Header h, HE_t he)
	/*@globals internalState @*/
	/*@modifies he, internalState @*/
{
    he->tag = RPMTAG_REQUIRENAME;
    return PRCOxmlTag(h, he, RPMTAG_REQUIREVERSION, RPMTAG_REQUIREFLAGS);
}

static int CxmlTag(Header h, HE_t he)
	/*@globals internalState @*/
	/*@modifies he, internalState @*/
{
    he->tag = RPMTAG_CONFLICTNAME;
    return PRCOxmlTag(h, he, RPMTAG_CONFLICTVERSION, RPMTAG_CONFLICTFLAGS);
}

static int OxmlTag(Header h, HE_t he)
	/*@globals internalState @*/
	/*@modifies he, internalState @*/
{
    he->tag = RPMTAG_OBSOLETENAME;
    return PRCOxmlTag(h, he, RPMTAG_OBSOLETEVERSION, RPMTAG_OBSOLETEFLAGS);
}

/**
 * Encode string for use by SQL/JSON markup.
 * @param he		tag container
 * @param av		parameter list (or NULL)
 * @param spew		spew primitives
 * @param lvl		indentation level
 * @return		formatted string
 */
static /*@only@*/ char * spewescapeFormat(HE_t he, /*@null@*/ const char ** av,
		spew_t spew, int lvl)
	/*@*/
{
    int ix = (he->ix > 0 ? he->ix : 0);
    char * val;

assert(ix == 0);
    if (he->t != RPM_STRING_TYPE) {
	val = xstrdup(_("(not a string)"));
    } else {
	const char * s = strdup_iconv_check(he->p.str, (av ? av[0] : NULL));
	size_t nb = spew->spew_strlen(s, lvl);
	char * t = xmalloc(nb+1);;
	val = t;
	t = spew->spew_strcpy(t, s, lvl);	t += strlen(t);
	*t = '\0';
	s = _free(s);
    }

    return val;
}

#ifndef	DYING	/* XXX is :json gud enuf? there are side effects ... */
static /*@only@*/ char * jsonescapeFormat(HE_t he, /*@null@*/ const char ** av)
	/*@*/
{
    return spewescapeFormat(he, av, &_json_spew, 0);
}
#endif

static /*@only@*/ char * sqlescapeFormat(HE_t he, /*@null@*/ const char ** av)
	/*@*/
{
    return spewescapeFormat(he, av, &_sql_spew, 0);
}

/*@-compmempass -kepttrans -nullstate -usereleased @*/
static int PRCOsqlTag(Header h, HE_t he, rpmTag EVRtag, rpmTag Ftag)
	/*@globals internalState @*/
	/*@modifies he, internalState @*/
{
    static char q = '"';
    rpmTag tag = he->tag;
    rpmTagData N = { .ptr = NULL };
    rpmTagData EVR = { .ptr = NULL };
    rpmTagData F = { .ptr = NULL };
    char instance[64];
    size_t nb;
    uint32_t ac;
    uint32_t c;
    uint32_t i;
    char *te;
    int rc = 1;		/* assume failure */
    int xx;

/*@-compmempass@*/	/* use separate HE_t, not rpmTagData, containers. */
    xx = headerGet(h, he, 0);
    if (xx == 0) goto exit;
    N.argv = he->p.argv;
    c = he->c;

    he->tag = EVRtag;
    xx = headerGet(h, he, 0);
    if (xx == 0) goto exit;
    EVR.argv = he->p.argv;

    he->tag = Ftag;
    xx = headerGet(h, he, 0);
    if (xx == 0) goto exit;
    F.ui32p = he->p.ui32p;

    xx = snprintf(instance, sizeof(instance), "'%u'", (unsigned)headerGetInstance(h));
    nb = 0;
    ac = 0;
    for (i = 0; i < c; i++) {
/*@-nullstate@*/	/* EVR.argv might be NULL */
	if (PRCOSkip(tag, N, EVR, F, i))
	    continue;
/*@=nullstate@*/
	ac++;
	nb += strlen(instance) + sizeof(", '', '', '', '', ''") - 1;
	if (tag == RPMTAG_REQUIRENAME)
	    nb += sizeof(", ''") - 1;
	nb += strlen(N.argv[i]);
	if (EVR.argv != NULL && EVR.argv[i] != NULL && *EVR.argv[i] != '\0') {
	    uint32_t Fx = ((F.ui32p[i] >> 1) & 0x7);
	    EVR_t Revr = rpmEVRnew(Fx, 1);
	    int xx = rpmEVRparse(EVR.argv[i], Revr);
	    const char * E = Revr->F[RPMEVR_E];
	    const char * V = Revr->F[RPMEVR_V];
#ifdef	NOTYET	/* XXX rpmrepo? */
	    const char * T = Revr->F[RPMEVR_T];
#endif
	    const char * R = Revr->F[RPMEVR_R];
#ifdef	NOTYET	/* XXX turning this on breaks rpmrepo */
	    const char * D = Revr->F[RPMEVR_D];
#endif
	    xx = xx;
	    nb += (sizeof(", 'EQ'")-1);
	    nb += (sizeof(", ''")-1) + strlen(E);
	    nb += (sizeof(", ''")-1) + strlen(V);
#ifdef	NOTYET	/* XXX rpmrepo? */
	    nb += (sizeof(", ''")-1) + strlen(T);
#endif
	    nb += (sizeof(", ''")-1) + strlen(R);
#ifdef	NOTYET	/* XXX turning this on breaks rpmrepo */
	    nb += (sizeof(", ''")-1) + strlen(D);
#endif
	    Revr = rpmEVRfree(Revr);
	}
#ifdef	NOTNOW
	if (tag == RPMTAG_REQUIRENAME && (F.ui32p[i] & 0x40))
	    nb += sizeof("1") - 1;
#endif
	nb++;
    }

    nb += (ac + 1) * sizeof(*he->p.argv);

    he->t = RPM_STRING_ARRAY_TYPE;
    he->c = ac;
    he->freeData = 1;
    he->p.argv = xmalloc(nb);
    te = (char *) &he->p.argv[ac + 1];
    *te = '\0';
    ac = 0;
    for (i = 0; i < c; i++) {
/*@-nullstate@*/	/* EVR.argv might be NULL */
	if (PRCOSkip(tag, N, EVR, F, i))
	    continue;
/*@=nullstate@*/
	he->p.argv[ac++] = te;
	te = stpcpy(te, instance);
	*te++ = ',';	*te++ = ' ';
	*te++ = q;	te = stpcpy(te, N.argv[i]);	*te++ = q;
/*@-readonlytrans@*/
	if (EVR.argv != NULL && EVR.argv[i] != NULL && *EVR.argv[i] != '\0') {
	    static const char *Fstr[] = { "?0","LT","GT","?3","EQ","LE","GE","?7" };
	    uint32_t Fx = ((F.ui32p[i] >> 1) & 0x7);
	    EVR_t Revr = rpmEVRnew(Fx, 1);
	    int xx = rpmEVRparse(EVR.argv[i], Revr);
	    const char * E = Revr->F[RPMEVR_E];
	    const char * V = Revr->F[RPMEVR_V];
#ifdef	NOTYET	/* XXX rpmrepo? */
	    const char * T = Revr->F[RPMEVR_T];
#endif
	    const char * R = Revr->F[RPMEVR_R];
#ifdef	NOTYET	/* XXX turning this on breaks rpmrepo */
	    const char * D = Revr->F[RPMEVR_D];
#endif
	    xx = xx;
	    *te++ = ',';	*te++ = ' ';
	    *te++ = q;	te = stpcpy(te, Fstr[Fx]);	*te++ = q;
	    *te++ = ',';	*te++ = ' ';
	    *te++ = q;	te = stpcpy(te, E);	*te++ = q;
	    *te++ = ',';	*te++ = ' ';
	    *te++ = q;	te = stpcpy(te, V);	*te++ = q;
#ifdef	NOTYET	/* XXX rpmrepo? */
	    *te++ = ',';	*te++ = ' ';
	    *te++ = q;	te = stpcpy(te, T);	*te++ = q;
#endif
	    *te++ = ',';	*te++ = ' ';
	    *te++ = q;	te = stpcpy(te, R);	*te++ = q;
#ifdef	NOTYET	/* XXX turning this on breaks rpmrepo */
	    *te++ = ',';	*te++ = ' ';
	    *te++ = q;	te = stpcpy(te, D);	*te++ = q;
#endif
	    Revr = rpmEVRfree(Revr);
	} else {
	    /* XXX FIXME: handle RPMEVR_T and RPMEVR_D? */
	    *te++ = ',';	*te++ = ' ';
	    *te++ = q;		*te++ = q;
	    *te++ = ',';	*te++ = ' ';
	    *te++ = q;		*te++ = q;
	    *te++ = ',';	*te++ = ' ';
	    *te++ = q;		*te++ = q;
	    *te++ = ',';	*te++ = ' ';
	    *te++ = q;		*te++ = q;
	}
/*@=readonlytrans@*/
#ifdef	NOTNOW
	if (tag == RPMTAG_REQUIRENAME)
	    te = stpcpy(stpcpy(stpcpy(te, ", '"),(F.ui32p[i] & 0x40) ? "1" : "0"), "'");
#endif
	*te++ = '\0';
    }
    he->p.argv[ac] = NULL;
/*@=compmempass@*/
    rc = 0;

exit:
/*@-kepttrans@*/	/* N.argv may be kept. */
    N.argv = _free(N.argv);
/*@=kepttrans@*/
/*@-usereleased@*/	/* EVR.argv may be dead. */
    EVR.argv = _free(EVR.argv);
/*@=usereleased@*/
    F.ui32p = _free(F.ui32p);
    return rc;
}

static int PsqlTag(Header h, HE_t he)
	/*@globals internalState @*/
	/*@modifies he, internalState @*/
{
    he->tag = RPMTAG_PROVIDENAME;
    return PRCOsqlTag(h, he, RPMTAG_PROVIDEVERSION, RPMTAG_PROVIDEFLAGS);
}

static int RsqlTag(Header h, HE_t he)
	/*@globals internalState @*/
	/*@modifies he, internalState @*/
{
    he->tag = RPMTAG_REQUIRENAME;
    return PRCOsqlTag(h, he, RPMTAG_REQUIREVERSION, RPMTAG_REQUIREFLAGS);
}

static int CsqlTag(Header h, HE_t he)
	/*@globals internalState @*/
	/*@modifies he, internalState @*/
{
    he->tag = RPMTAG_CONFLICTNAME;
    return PRCOsqlTag(h, he, RPMTAG_CONFLICTVERSION, RPMTAG_CONFLICTFLAGS);
}

static int OsqlTag(Header h, HE_t he)
	/*@globals internalState @*/
	/*@modifies he, internalState @*/
{
    he->tag = RPMTAG_OBSOLETENAME;
    return PRCOsqlTag(h, he, RPMTAG_OBSOLETEVERSION, RPMTAG_OBSOLETEFLAGS);
}

static int PRCOyamlTag(Header h, HE_t he, rpmTag EVRtag, rpmTag Ftag)
	/*@globals internalState @*/
	/*@modifies he, internalState @*/
{
    rpmTag tag = he->tag;
    rpmTagData N = { .ptr = NULL };
    rpmTagData EVR = { .ptr = NULL };
    rpmTagData F = { .ptr = NULL };
    size_t nb;
    rpmuint32_t ac;
    rpmuint32_t c;
    rpmuint32_t i;
    char *t;
    int rc = 1;		/* assume failure */
    int xx;
int indent = 0;
spew_t spew = &_yaml_spew;

/*@-compmempass@*/	/* use separate HE_t, not rpmTagData, containers. */
    xx = headerGet(h, he, 0);
    if (xx == 0) goto exit;
    N.argv = he->p.argv;
    c = he->c;

    he->tag = EVRtag;
    xx = headerGet(h, he, 0);
    if (xx == 0) goto exit;
    EVR.argv = he->p.argv;

    he->tag = Ftag;
    xx = headerGet(h, he, 0);
    if (xx == 0) goto exit;
    F.ui32p = he->p.ui32p;

    nb = sizeof(*he->p.argv);
    ac = 0;
    for (i = 0; i < c; i++) {
/*@-nullstate@*/	/* EVR.argv might be NULL */
	if (PRCOSkip(tag, N, EVR, F, i))
	    continue;
/*@=nullstate@*/
	ac++;
	nb += sizeof(*he->p.argv);
	nb += sizeof("- ");
	if (*N.argv[i] == '/')
	    nb += spew->spew_strlen(N.argv[i], indent);
	else
	    nb += strlen(N.argv[i]);
	if (EVR.argv != NULL && EVR.argv[i] != NULL && *EVR.argv[i] != '\0') {
	    nb += sizeof(" >= ") - 1;
	    nb += strlen(EVR.argv[i]);
	}
    }

    he->t = RPM_STRING_ARRAY_TYPE;
    he->c = ac;
    he->freeData = 1;
    he->p.argv = xmalloc(nb + BUFSIZ);	/* XXX hack: leave slop */
    t = (char *) &he->p.argv[he->c + 1];
    ac = 0;
    for (i = 0; i < c; i++) {
/*@-nullstate@*/	/* EVR.argv might be NULL */
	if (PRCOSkip(tag, N, EVR, F, i))
	    continue;
/*@=nullstate@*/
	he->p.argv[ac++] = t;
	t = stpcpy(t, "- ");
	if (*N.argv[i] == '/') {
	    t = spew->spew_strcpy(t, N.argv[i], indent);	t += strlen(t);
	} else
	    t = stpcpy(t, N.argv[i]);
/*@-readonlytrans@*/
	if (EVR.argv != NULL && EVR.argv[i] != NULL && *EVR.argv[i] != '\0') {
	    static char *Fstr[] = { "?0","<",">","?3","=","<=",">=","?7" };
	    rpmuint32_t Fx = ((F.ui32p[i] >> 1) & 0x7);
	    t = stpcpy( stpcpy( stpcpy(t, " "), Fstr[Fx]), " ");
	    t = stpcpy(t, EVR.argv[i]);
	}
/*@=readonlytrans@*/
	*t++ = '\0';
    }
    he->p.argv[he->c] = NULL;
/*@=compmempass@*/
    rc = 0;

exit:
/*@-kepttrans@*/	/* N.argv may be kept. */
    N.argv = _free(N.argv);
/*@=kepttrans@*/
/*@-usereleased@*/	/* EVR.argv may be dead. */
    EVR.argv = _free(EVR.argv);
/*@=usereleased@*/
    F.ui32p = _free(F.ui32p);
    return rc;
}

static int PyamlTag(Header h, HE_t he)
	/*@globals internalState @*/
	/*@modifies he, internalState @*/
{
    int rc;
    he->tag = RPMTAG_PROVIDENAME;
    rc = PRCOyamlTag(h, he, RPMTAG_PROVIDEVERSION, RPMTAG_PROVIDEFLAGS);
    he->tag = RPMTAG_PROVIDEYAMLENTRY;
    return rc;
}

static int RyamlTag(Header h, HE_t he)
	/*@globals internalState @*/
	/*@modifies he, internalState @*/
{
    int rc;
    he->tag = RPMTAG_REQUIRENAME;
    rc = PRCOyamlTag(h, he, RPMTAG_REQUIREVERSION, RPMTAG_REQUIREFLAGS);
    he->tag = RPMTAG_REQUIREYAMLENTRY;
    return rc;
}

static int CyamlTag(Header h, HE_t he)
	/*@globals internalState @*/
	/*@modifies he, internalState @*/
{
    int rc;
    he->tag = RPMTAG_CONFLICTNAME;
    rc = PRCOyamlTag(h, he, RPMTAG_CONFLICTVERSION, RPMTAG_CONFLICTFLAGS);
    he->tag = RPMTAG_CONFLICTYAMLENTRY;
    return rc;
}

static int OyamlTag(Header h, HE_t he)
	/*@globals internalState @*/
	/*@modifies he, internalState @*/
{
    int rc;
    he->tag = RPMTAG_OBSOLETENAME;
    rc = PRCOyamlTag(h, he, RPMTAG_OBSOLETEVERSION, RPMTAG_OBSOLETEFLAGS);
    he->tag = RPMTAG_OBSOLETEYAMLENTRY;
    return rc;
}

static int FDGSkip(rpmTagData DN, rpmTagData BN, rpmTagData DI, rpmuint32_t i)
	/*@*/
{
    const char * dn = DN.argv[DI.ui32p[i]];
    size_t dnlen = strlen(dn);

assert(dn != NULL);
    if (strstr(dn, "bin/") != NULL)
	return 1;
    if (dnlen >= sizeof("/etc/")-1 && !strncmp(dn, "/etc/", dnlen))
	return 1;
    if (!strcmp(dn, "/usr/lib/") && !strcmp(BN.argv[i], "sendmail"))
	return 1;
    return 2;
}

static int FDGxmlTag(Header h, HE_t he, int lvl)
	/*@globals internalState @*/
	/*@modifies he, internalState @*/
{
    rpmTagData BN = { .ptr = NULL };
    rpmTagData DN = { .ptr = NULL };
    rpmTagData DI = { .ptr = NULL };
    rpmTagData FMODES = { .ptr = NULL };
    rpmTagData FFLAGS = { .ptr = NULL };
    size_t nb;
    rpmuint32_t ac;
    rpmuint32_t c;
    rpmuint32_t i;
    char *t;
    int rc = 1;		/* assume failure */
    int xx;
spew_t spew = &_xml_spew;

/*@-compmempass@*/	/* use separate HE_t, not rpmTagData, containers. */
    he->tag = RPMTAG_BASENAMES;
    xx = headerGet(h, he, 0);
    if (xx == 0) goto exit;
    BN.argv = he->p.argv;
    c = he->c;

    he->tag = RPMTAG_DIRNAMES;
    xx = headerGet(h, he, 0);
    if (xx == 0) goto exit;
    DN.argv = he->p.argv;

    he->tag = RPMTAG_DIRINDEXES;
    xx = headerGet(h, he, 0);
    if (xx == 0) goto exit;
    DI.ui32p = he->p.ui32p;

    he->tag = RPMTAG_FILEMODES;
    xx = headerGet(h, he, 0);
    if (xx == 0) goto exit;
    FMODES.ui16p = he->p.ui16p;

    he->tag = RPMTAG_FILEFLAGS;
    xx = headerGet(h, he, 0);
    if (xx == 0) goto exit;
    FFLAGS.ui32p = he->p.ui32p;

    nb = sizeof(*he->p.argv);
    ac = 0;
    for (i = 0; i < c; i++) {
	if (lvl > 0 && FDGSkip(DN, BN, DI, i) != lvl)
	    continue;
	ac++;
	nb += sizeof(*he->p.argv);
	nb += sizeof("<file></file>");
	nb += spew->spew_strlen(DN.argv[DI.ui32p[i]], lvl);
	nb += spew->spew_strlen(BN.argv[i], lvl);
	if (FFLAGS.ui32p[i] & 0x40)	/* XXX RPMFILE_GHOST */
	    nb += sizeof(" type=\"ghost\"") - 1;
	else if (S_ISDIR(FMODES.ui16p[i])) {
	    nb += sizeof(" type=\"dir\"") - 1;
#ifdef	NOTYET
	    nb += sizeof("/") - 1;
#endif
	}
    }

    he->t = RPM_STRING_ARRAY_TYPE;
    he->c = ac;
    he->freeData = 1;
    he->p.argv = xmalloc(nb);
    t = (char *) &he->p.argv[he->c + 1];
    ac = 0;
    /* FIXME: Files, then dirs, finally ghosts breaks sort order.  */
    for (i = 0; i < c; i++) {
	if (lvl > 0 && FDGSkip(DN, BN, DI, i) != lvl)
	    continue;
	if (FFLAGS.ui32p[i] & 0x40)	/* XXX RPMFILE_GHOST */
	    continue;
	if (S_ISDIR(FMODES.ui16p[i]))
	    continue;
	he->p.argv[ac++] = t;
	t = stpcpy(t, "<file>");
	t = spew->spew_strcpy(t, DN.argv[DI.ui32p[i]], lvl);	t += strlen(t);
	t = spew->spew_strcpy(t, BN.argv[i], lvl);		t += strlen(t);
	t = stpcpy(t, "</file>");
	*t++ = '\0';
    }
    for (i = 0; i < c; i++) {
	if (lvl > 0 && FDGSkip(DN, BN, DI, i) != lvl)
	    continue;
	if (FFLAGS.ui32p[i] & 0x40)	/* XXX RPMFILE_GHOST */
	    continue;
	if (!S_ISDIR(FMODES.ui16p[i]))
	    continue;
	he->p.argv[ac++] = t;
	t = stpcpy(t, "<file type=\"dir\">");
	t = spew->spew_strcpy(t, DN.argv[DI.ui32p[i]], lvl);	t += strlen(t);
	t = spew->spew_strcpy(t, BN.argv[i], lvl);		t += strlen(t);
#ifdef	NOTYET
	/* Append the pesky trailing / to directories. */
	if (t[-1] != '/')
	    t = stpcpy(t, "/");
#endif
	t = stpcpy(t, "</file>");
	*t++ = '\0';
    }
    for (i = 0; i < c; i++) {
	if (lvl > 0 && FDGSkip(DN, BN, DI, i) != lvl)
	    continue;
	if (!(FFLAGS.ui32p[i] & 0x40))	/* XXX RPMFILE_GHOST */
	    continue;
	he->p.argv[ac++] = t;
	t = stpcpy(t, "<file type=\"ghost\">");
	t = spew->spew_strcpy(t, DN.argv[DI.ui32p[i]], lvl);	t += strlen(t);
	t = spew->spew_strcpy(t, BN.argv[i], lvl);		t += strlen(t);
	t = stpcpy(t, "</file>");
	*t++ = '\0';
    }

    he->p.argv[he->c] = NULL;
/*@=compmempass@*/
    rc = 0;

exit:
/*@-kepttrans@*/	/* {BN,DN,DI}.argv may be kept. */
    BN.argv = _free(BN.argv);
/*@-usereleased@*/	/* DN.argv may be dead. */
    DN.argv = _free(DN.argv);
/*@=usereleased@*/
    DI.ui32p = _free(DI.ui32p);
/*@=kepttrans@*/
    FMODES.ui16p = _free(FMODES.ui16p);
/*@-usereleased@*/	/* FFLAGS.argv may be dead. */
    FFLAGS.ui32p = _free(FFLAGS.ui32p);
/*@=usereleased@*/
    return rc;
}

static int F1xmlTag(Header h, HE_t he)
	/*@globals internalState @*/
	/*@modifies he, internalState @*/
{
    he->tag = RPMTAG_BASENAMES;
    return FDGxmlTag(h, he, 1);
}

static int F2xmlTag(Header h, HE_t he)
	/*@globals internalState @*/
	/*@modifies he, internalState @*/
{
    he->tag = RPMTAG_BASENAMES;
    return FDGxmlTag(h, he, 2);
}

static int FDGsqlTag(Header h, HE_t he, int lvl)
	/*@globals internalState @*/
	/*@modifies he, internalState @*/
{
    rpmTagData BN = { .ptr = NULL };
    rpmTagData DN = { .ptr = NULL };
    rpmTagData DI = { .ptr = NULL };
    rpmTagData FMODES = { .ptr = NULL };
    rpmTagData FFLAGS = { .ptr = NULL };
    char instance[64];
    size_t nb;
    rpmuint32_t ac;
    rpmuint32_t c;
    rpmuint32_t i;
    char *t;
    int rc = 1;		/* assume failure */
    int xx;

/*@-compmempass@*/	/* use separate HE_t, not rpmTagData, containers. */
    he->tag = RPMTAG_BASENAMES;
    xx = headerGet(h, he, 0);
    if (xx == 0) goto exit;
    BN.argv = he->p.argv;
    c = he->c;

    he->tag = RPMTAG_DIRNAMES;
    xx = headerGet(h, he, 0);
    if (xx == 0) goto exit;
    DN.argv = he->p.argv;

    he->tag = RPMTAG_DIRINDEXES;
    xx = headerGet(h, he, 0);
    if (xx == 0) goto exit;
    DI.ui32p = he->p.ui32p;

    he->tag = RPMTAG_FILEMODES;
    xx = headerGet(h, he, 0);
    if (xx == 0) goto exit;
    FMODES.ui16p = he->p.ui16p;

    he->tag = RPMTAG_FILEFLAGS;
    xx = headerGet(h, he, 0);
    if (xx == 0) goto exit;
    FFLAGS.ui32p = he->p.ui32p;

    xx = snprintf(instance, sizeof(instance), "'%u'", (unsigned)headerGetInstance(h));
    nb = sizeof(*he->p.argv);
    ac = 0;
    for (i = 0; i < c; i++) {
	if (lvl > 0 && FDGSkip(DN, BN, DI, i) != lvl)
	    continue;
	ac++;
	nb += sizeof(*he->p.argv);
	nb += strlen(instance) + sizeof(", '', ''");
	nb += strlen(DN.argv[DI.ui32p[i]]);
	nb += strlen(BN.argv[i]);
	if (FFLAGS.ui32p[i] & 0x40)	/* XXX RPMFILE_GHOST */
	    nb += sizeof("ghost") - 1;
	else if (S_ISDIR(FMODES.ui16p[i])) {
	    nb += sizeof("dir") - 1;
#ifdef	NOTYET
	    nb += sizeof("/") - 1;
#endif
	} else
	    nb += sizeof("file") - 1;
    }

    he->t = RPM_STRING_ARRAY_TYPE;
    he->c = ac;
    he->freeData = 1;
    he->p.argv = xmalloc(nb);
    t = (char *) &he->p.argv[he->c + 1];
    ac = 0;
    /* FIXME: Files, then dirs, finally ghosts breaks sort order.  */
    for (i = 0; i < c; i++) {
	if (lvl > 0 && FDGSkip(DN, BN, DI, i) != lvl)
	    continue;
	if (FFLAGS.ui32p[i] & 0x40)	/* XXX RPMFILE_GHOST */
	    continue;
	if (S_ISDIR(FMODES.ui16p[i]))
	    continue;
	he->p.argv[ac++] = t;
	t = stpcpy( stpcpy(t, instance), ", '");
	t = strcpy(t, DN.argv[DI.ui32p[i]]);	t += strlen(t);
	t = strcpy(t, BN.argv[i]);		t += strlen(t);
	t = stpcpy(t, "', 'file'");
	*t++ = '\0';
    }
    for (i = 0; i < c; i++) {
	if (lvl > 0 && FDGSkip(DN, BN, DI, i) != lvl)
	    continue;
	if (FFLAGS.ui32p[i] & 0x40)	/* XXX RPMFILE_GHOST */
	    continue;
	if (!S_ISDIR(FMODES.ui16p[i]))
	    continue;
	he->p.argv[ac++] = t;
	t = stpcpy( stpcpy(t, instance), ", '");
	t = strcpy(t, DN.argv[DI.ui32p[i]]);	t += strlen(t);
	t = strcpy(t, BN.argv[i]);		t += strlen(t);
#ifdef	NOTYET
	/* Append the pesky trailing / to directories. */
	if (t[-1] != '/')
	    t = stpcpy(t, "/");
#endif
	t = stpcpy(t, "', 'dir'");
	*t++ = '\0';
    }
    for (i = 0; i < c; i++) {
	if (lvl > 0 && FDGSkip(DN, BN, DI, i) != lvl)
	    continue;
	if (!(FFLAGS.ui32p[i] & 0x40))	/* XXX RPMFILE_GHOST */
	    continue;
	he->p.argv[ac++] = t;
	t = stpcpy( stpcpy(t, instance), ", '");
	t = strcpy(t, DN.argv[DI.ui32p[i]]);	t += strlen(t);
	t = strcpy(t, BN.argv[i]);		t += strlen(t);
	t = stpcpy(t, "', 'ghost'");
	*t++ = '\0';
    }

    he->p.argv[he->c] = NULL;
/*@=compmempass@*/
    rc = 0;

exit:
/*@-kepttrans@*/	/* {BN,DN,DI}.argv may be kept. */
    BN.argv = _free(BN.argv);
/*@-usereleased@*/	/* DN.argv may be dead. */
    DN.argv = _free(DN.argv);
/*@=usereleased@*/
    DI.ui32p = _free(DI.ui32p);
/*@=kepttrans@*/
    FMODES.ui16p = _free(FMODES.ui16p);
/*@-usereleased@*/	/* FFLAGS.argv may be dead. */
    FFLAGS.ui32p = _free(FFLAGS.ui32p);
/*@=usereleased@*/
    return rc;
}

static int F1sqlTag(Header h, HE_t he)
	/*@globals internalState @*/
	/*@modifies he, internalState @*/
{
    he->tag = RPMTAG_BASENAMES;
    return FDGsqlTag(h, he, 1);
}

static int F2sqlTag(Header h, HE_t he)
	/*@globals internalState @*/
	/*@modifies he, internalState @*/
{
    he->tag = RPMTAG_BASENAMES;
    return FDGsqlTag(h, he, 2);
}

static int FDGyamlTag(Header h, HE_t he, int lvl)
	/*@globals internalState @*/
	/*@modifies he, internalState @*/
{
    rpmTagData BN = { .ptr = NULL };
    rpmTagData DN = { .ptr = NULL };
    rpmTagData DI = { .ptr = NULL };
    rpmTagData FMODES = { .ptr = NULL };
    rpmTagData FFLAGS = { .ptr = NULL };
    size_t nb;
    rpmuint32_t ac;
    rpmuint32_t c;
    rpmuint32_t i;
    char *t;
    int rc = 1;		/* assume failure */
    int xx;
int indent = 0;
spew_t spew = &_yaml_spew;

/*@-compmempass@*/	/* use separate HE_t, not rpmTagData, containers. */
    he->tag = RPMTAG_BASENAMES;
    xx = headerGet(h, he, 0);
    if (xx == 0) goto exit;
    BN.argv = he->p.argv;
    c = he->c;

    he->tag = RPMTAG_DIRNAMES;
    xx = headerGet(h, he, 0);
    if (xx == 0) goto exit;
    DN.argv = he->p.argv;

    he->tag = RPMTAG_DIRINDEXES;
    xx = headerGet(h, he, 0);
    if (xx == 0) goto exit;
    DI.ui32p = he->p.ui32p;

    he->tag = RPMTAG_FILEMODES;
    xx = headerGet(h, he, 0);
    if (xx == 0) goto exit;
    FMODES.ui16p = he->p.ui16p;

    he->tag = RPMTAG_FILEFLAGS;
    xx = headerGet(h, he, 0);
    if (xx == 0) goto exit;
    FFLAGS.ui32p = he->p.ui32p;

    nb = sizeof(*he->p.argv);
    ac = 0;
    for (i = 0; i < c; i++) {
	if (lvl > 0 && FDGSkip(DN, BN, DI, i) != lvl)
	    continue;
	ac++;
	nb += sizeof(*he->p.argv);
	nb += sizeof("- ");
	nb += spew->spew_strlen(DN.argv[DI.ui32p[i]], indent);
	nb += spew->spew_strlen(BN.argv[i], indent);
	if (FFLAGS.ui32p[i] & 0x40)	/* XXX RPMFILE_GHOST */
	    nb += sizeof("") - 1;
	else if (S_ISDIR(FMODES.ui16p[i]))
	    nb += sizeof("/") - 1;
    }

    he->t = RPM_STRING_ARRAY_TYPE;
    he->c = ac;
    he->freeData = 1;
    he->p.argv = xmalloc(nb);
    t = (char *) &he->p.argv[he->c + 1];
    ac = 0;
    /* FIXME: Files, then dirs, finally ghosts breaks sort order.  */
    for (i = 0; i < c; i++) {
	if (lvl > 0 && FDGSkip(DN, BN, DI, i) != lvl)
	    continue;
	if (FFLAGS.ui32p[i] & 0x40)	/* XXX RPMFILE_GHOST */
	    continue;
	if (S_ISDIR(FMODES.ui16p[i]))
	    continue;
	he->p.argv[ac++] = t;
	t = stpcpy(t, "- ");
	t = spew->spew_strcpy(t, DN.argv[DI.ui32p[i]], indent); t += strlen(t);
	t = spew->spew_strcpy(t, BN.argv[i], indent);		t += strlen(t);
	t = stpcpy(t, "");
	*t++ = '\0';
    }
    for (i = 0; i < c; i++) {
	if (lvl > 0 && FDGSkip(DN, BN, DI, i) != lvl)
	    continue;
	if (FFLAGS.ui32p[i] & 0x40)	/* XXX RPMFILE_GHOST */
	    continue;
	if (!S_ISDIR(FMODES.ui16p[i]))
	    continue;
	he->p.argv[ac++] = t;
	t = stpcpy(t, "- ");
	t = spew->spew_strcpy(t, DN.argv[DI.ui32p[i]], indent); t += strlen(t);
	t = spew->spew_strcpy(t, BN.argv[i], indent);		t += strlen(t);
	/* Append the pesky trailing / to directories. */
	if (t[-1] != '/')
	    t = stpcpy(t, "/");
	*t++ = '\0';
    }
    for (i = 0; i < c; i++) {
	if (lvl > 0 && FDGSkip(DN, BN, DI, i) != lvl)
	    continue;
	if (!(FFLAGS.ui32p[i] & 0x40))	/* XXX RPMFILE_GHOST */
	    continue;
	he->p.argv[ac++] = t;
	t = stpcpy(t, "- ");
	t = spew->spew_strcpy(t, DN.argv[DI.ui32p[i]], indent); t += strlen(t);
	t = spew->spew_strcpy(t, BN.argv[i], indent);		t += strlen(t);
	*t++ = '\0';
    }

    he->p.argv[he->c] = NULL;
/*@=compmempass@*/
    rc = 0;

exit:
/*@-kepttrans@*/	/* {BN,DN,DI}.argv may be kept. */
    BN.argv = _free(BN.argv);
/*@-usereleased@*/	/* DN.argv may be dead. */
    DN.argv = _free(DN.argv);
/*@=usereleased@*/
    DI.ui32p = _free(DI.ui32p);
/*@=kepttrans@*/
    FMODES.ui16p = _free(FMODES.ui16p);
/*@-usereleased@*/	/* FFLAGS.argv may be dead. */
    FFLAGS.ui32p = _free(FFLAGS.ui32p);
/*@=usereleased@*/
    return rc;
}

static int F1yamlTag(Header h, HE_t he)
	/*@globals internalState @*/
	/*@modifies he, internalState @*/
{
    he->tag = RPMTAG_BASENAMES;
    return FDGyamlTag(h, he, 1);
}

static int F2yamlTag(Header h, HE_t he)
	/*@globals internalState @*/
	/*@modifies he, internalState @*/
{
    he->tag = RPMTAG_BASENAMES;
    return FDGyamlTag(h, he, 2);
}

/**
 * Encode the basename of a string for use in XML CDATA.
 * @param he            tag container
 * @param av		parameter list (or NULL)
 * @return              formatted string
 */
static /*@only@*/ char * bncdataFormat(HE_t he, /*@null@*/ const char ** av)
	/*@*/
{
    char * val;

    if (he->t != RPM_STRING_TYPE) {
	val = xstrdup(_("(not a string)"));
    } else {
	const char * bn;
	const char * s;
	size_t nb;
	char * t;
int lvl = 0;
spew_t spew = &_xml_spew;

assert(he->p.str != NULL);
	/* Get rightmost '/' in string (i.e. basename(3) behavior). */
	if ((bn = strrchr(he->p.str, '/')) != NULL)
	    bn++;
	else
	    bn = he->p.str;

	s = strdup_iconv_check(bn, (av ? av[0] : NULL));
	nb = spew->spew_strlen(s, lvl);
	t = xmalloc(nb + 1);
	val = t;
	t = spew->spew_strcpy(t, s, lvl);	t += strlen(t);
	*t = '\0';
	s = _free(s);
    }

    return val;
}

typedef struct key_s {
/*@observer@*/
	const char *name;		/* key name */
	rpmuint32_t value;
} KEY;

/*@unchecked@*/ /*@observer@*/
static KEY keyDigests[] = {
    { "adler32",	PGPHASHALGO_ADLER32 },
    { "crc32",		PGPHASHALGO_CRC32 },
    { "crc64",		PGPHASHALGO_CRC64 },
    { "haval160",	PGPHASHALGO_HAVAL_5_160 },
    { "jlu32",		PGPHASHALGO_JLU32 },
    { "md2",		PGPHASHALGO_MD2 },
    { "md4",		PGPHASHALGO_MD4 },
    { "md5",		PGPHASHALGO_MD5 },
    { "rmd128",		PGPHASHALGO_RIPEMD128 },
    { "rmd160",		PGPHASHALGO_RIPEMD160 },
    { "rmd256",		PGPHASHALGO_RIPEMD256 },
    { "rmd320",		PGPHASHALGO_RIPEMD320 },
    { "salsa10",	PGPHASHALGO_SALSA10 },
    { "salsa20",	PGPHASHALGO_SALSA20 },
    { "sha1",		PGPHASHALGO_SHA1 },
    { "sha224",		PGPHASHALGO_SHA224 },
    { "sha256",		PGPHASHALGO_SHA256 },
    { "sha384",		PGPHASHALGO_SHA384 },
    { "sha512",		PGPHASHALGO_SHA512 },
    { "tiger192",	PGPHASHALGO_TIGER192 },
};
/*@unchecked@*/
static size_t nkeyDigests = sizeof(keyDigests) / sizeof(keyDigests[0]);

/**
 * Bit field enum for stat(2) keys.
 */
enum keyStat_e {
    STAT_KEYS_NONE	= 0,
    STAT_KEYS_DEV	= (1U <<  0),	/*!< st_dev */
    STAT_KEYS_INO	= (1U <<  1),	/*!< st_ino */
    STAT_KEYS_MODE	= (1U <<  2),	/*!< st_mode */
    STAT_KEYS_NLINK	= (1U <<  3),	/*!< st_nlink */
    STAT_KEYS_UID	= (1U <<  4),	/*!< st_uid */
    STAT_KEYS_GID	= (1U <<  5),	/*!< st_gid */
    STAT_KEYS_RDEV	= (1U <<  6),	/*!< st_rdev */
    STAT_KEYS_SIZE	= (1U <<  7),	/*!< st_size */
    STAT_KEYS_BLKSIZE	= (1U <<  8),	/*!< st_blksize */
    STAT_KEYS_BLOCKS	= (1U <<  9),	/*!< st_blocks */
    STAT_KEYS_ATIME	= (1U << 10),	/*!< st_atime */
    STAT_KEYS_CTIME	= (1U << 11),	/*!< st_ctime */
    STAT_KEYS_MTIME	= (1U << 12),	/*!< st_mtime */
#ifdef	NOTYET
    STAT_KEYS_FLAGS	= (1U << 13),	/*!< st_flags */
#endif
    STAT_KEYS_SLINK	= (1U << 14),	/*!< symlink */
    STAT_KEYS_DIGEST	= (1U << 15),	/*!< digest */
#ifdef	NOTYET
    STAT_KEYS_FCONTEXT	= (1U << 16),	/*!< fcontext */
#endif
    STAT_KEYS_UNAME	= (1U << 17),	/*!< user name */
    STAT_KEYS_GNAME	= (1U << 18),	/*!< group name */
};

/*@unchecked@*/ /*@observer@*/
static KEY keyStat[] = {
    { "adler32",	STAT_KEYS_DIGEST },
    { "atime",		STAT_KEYS_ATIME },
    { "ctime",		STAT_KEYS_CTIME },
    { "blksize",	STAT_KEYS_BLKSIZE },
    { "blocks",		STAT_KEYS_BLOCKS },
    { "crc32",		STAT_KEYS_DIGEST },
    { "crc64",		STAT_KEYS_DIGEST },
    { "dev",		STAT_KEYS_DEV },
#ifdef	NOTYET
    { "digest",		STAT_KEYS_DIGEST },
    { "fcontext",	STAT_KEYS_FCONTEXT },
    { "flags",		STAT_KEYS_FLAGS },
#endif
    { "gid",		STAT_KEYS_GID },
    { "gname",		STAT_KEYS_GNAME },
    { "haval160",	STAT_KEYS_DIGEST },
    { "ino",		STAT_KEYS_INO },
    { "jlu32",		STAT_KEYS_DIGEST },
    { "link",		STAT_KEYS_SLINK },
    { "md2",		STAT_KEYS_DIGEST },
    { "md4",		STAT_KEYS_DIGEST },
    { "md5",		STAT_KEYS_DIGEST },
    { "mode",		STAT_KEYS_MODE },
    { "mtime",		STAT_KEYS_MTIME },
    { "nlink",		STAT_KEYS_NLINK },
    { "rdev",		STAT_KEYS_RDEV },
    { "rmd128",		STAT_KEYS_DIGEST },
    { "rmd160",		STAT_KEYS_DIGEST },
    { "rmd256",		STAT_KEYS_DIGEST },
    { "rmd320",		STAT_KEYS_DIGEST },
    { "salsa10",	STAT_KEYS_DIGEST },
    { "salsa20",	STAT_KEYS_DIGEST },
    { "sha1",		STAT_KEYS_DIGEST },
    { "sha224",		STAT_KEYS_DIGEST },
    { "sha256",		STAT_KEYS_DIGEST },
    { "sha384",		STAT_KEYS_DIGEST },
    { "sha512",		STAT_KEYS_DIGEST },
    { "size",		STAT_KEYS_SIZE },
    { "tiger192",	STAT_KEYS_DIGEST },
    { "uid",		STAT_KEYS_UID },
    { "uname",		STAT_KEYS_UNAME },
};
/*@unchecked@*/
static size_t nkeyStat = sizeof(keyStat) / sizeof(keyStat[0]);

/**
 * Bit field enum for stat(2) keys.
 */
enum keyUuids_e {
    UUID_KEYS_NONE	= (0U <<  0),
    UUID_KEYS_V1	= (1U <<  0),
    UUID_KEYS_V3	= (3U <<  0),
    UUID_KEYS_V4	= (4U <<  0),
    UUID_KEYS_V5	= (5U <<  0),
#ifdef	NOTYET
    UUID_KEYS_STRING	= (0U <<  4),
    UUID_KEYS_SIV	= (1U <<  4),
    UUID_KEYS_BINARY	= (2U <<  4),
    UUID_KEYS_TEXT	= (3U <<  4),
#endif
};

/*@unchecked@*/ /*@observer@*/
static KEY keyUuids[] = {
#ifdef	NOTYET
    { "binary",		UUID_KEYS_BINARY },
    { "siv",		UUID_KEYS_SIV },
    { "string",		UUID_KEYS_STRING },
    { "text",		UUID_KEYS_TEXT },
#endif
    { "v1",		UUID_KEYS_V1 },
    { "v3",		UUID_KEYS_V3 },
    { "v4",		UUID_KEYS_V4 },
    { "v5",		UUID_KEYS_V5 },
};
/*@unchecked@*/
static size_t nkeyUuids = sizeof(keyUuids) / sizeof(keyUuids[0]);

/**
 */
static int
keyCmp(const void * a, const void * b)
	/*@*/
{
    return strcmp(((KEY *)a)->name, ((KEY *)b)->name);
}

/**
 */
static rpmuint32_t
keyValue(KEY * keys, size_t nkeys, /*@null@*/ const char *name)
	/*@*/
{
    rpmuint32_t keyval = 0;

    if (name && * name) {
	KEY needle = { .name = name, .value = 0 };
	KEY *k = (KEY *)bsearch(&needle, keys, nkeys, sizeof(*keys), keyCmp);
	if (k)
	    keyval = k->value;
    }
    return keyval;
}

/**
 * Return digest of tag data.
 * @param he		tag container
 * @param av		parameter list (NULL uses md5)
 * @return		formatted string
 */
static /*@only@*/ char * digestFormat(HE_t he, /*@null@*/ const char ** av)
	/*@*/
{
    int ix = (he->ix > 0 ? he->ix : 0);
    char * val = NULL;
    size_t ns;

assert(ix == 0);
    switch(he->t) {
    default:
	val = xstrdup(_("(invalid type :digest)"));
	goto exit;
	/*@notreached@*/ break;
    case RPM_UINT64_TYPE:
	ns = sizeof(he->p.ui64p[0]);
	break;
    case RPM_STRING_TYPE:
	ns = strlen(he->p.str);
	break;
    case RPM_BIN_TYPE:
	ns = he->c;
	break;
    }

assert(he->p.ptr != NULL);
    {	rpmuint32_t keyval = keyValue(keyDigests, nkeyDigests, (av ? av[0] : NULL));
	rpmuint32_t algo = (keyval ? keyval : PGPHASHALGO_SHA1);
	DIGEST_CTX ctx = rpmDigestInit(algo, 0);
	int xx = rpmDigestUpdate(ctx, he->p.ptr, ns);
	xx = rpmDigestFinal(ctx, &val, NULL, 1);
    }

exit:
    return val;
}

/**
 * Return file info.
 * @param he		tag container
 * @param av		parameter list (NULL uses sha1)
 * @return		formatted string
 */
static /*@only@*/ char * statFormat(HE_t he, /*@null@*/ const char ** av)
	/*@globals rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
	/*@modifies rpmGlobalMacroContext, fileSystem, internalState @*/
{
/*@-nullassign@*/
    /*@unchecked@*/ /*@observer@*/
    static const char *avdefault[] = { "mode", NULL };
/*@=nullassign@*/
    const char * fn = NULL;
    struct stat sb, *st = &sb;
    int ix = (he->ix > 0 ? he->ix : 0);
    char * val = NULL;
    int xx;
    int i;

    memset(st, 0, sizeof(*st));
assert(ix == 0);
    switch(he->t) {
    case RPM_BIN_TYPE:
	/* XXX limit to RPMTAG_PACKAGESTAT ... */
	if (he->tag == RPMTAG_PACKAGESTAT)
	if ((size_t)he->c == sizeof(*st)) {
	    st = (struct stat *)he->p.ptr;
	    break;
	}
	/*@fallthrough @*/
    default:
	val = xstrdup(_("(invalid type :stat)"));
	goto exit;
	/*@notreached@*/ break;
    case RPM_STRING_TYPE:
	fn = he->p.str;
	if (Lstat(fn, st) == 0)
	    break;
/*@-ownedtrans@*/
	val = rpmExpand("(Lstat:", fn, ":", strerror(errno), ")", NULL);
/*@=ownedtrans@*/
	goto exit;
	/*@notreached@*/ break;
    }

    if (!(av && av[0] && *av[0]))
	av = avdefault;
    for (i = 0; av[i] != NULL; i++) {
	char b[BUFSIZ+1];	/* XXX coverity #103583 */
	size_t nb = sizeof(b)-1;
	char * nval;
	rpmuint32_t keyval = keyValue(keyStat, nkeyStat, av[i]);

	nval = NULL;
	b[0] = '\0';
	switch (keyval) {
	default:
	    /*@switchbreak@*/ break;
	case STAT_KEYS_NONE:
	    /*@switchbreak@*/ break;
	case STAT_KEYS_DEV:
	    xx = snprintf(b, nb, "0x%lx", (unsigned long)st->st_dev);
	    /*@switchbreak@*/ break;
	case STAT_KEYS_INO:
	    xx = snprintf(b, nb, "0x%lx", (unsigned long)st->st_ino);
	    /*@switchbreak@*/ break;
	case STAT_KEYS_MODE:
	    xx = snprintf(b, nb, "%06o", (unsigned)st->st_mode);
	    /*@switchbreak@*/ break;
	case STAT_KEYS_NLINK:
	    xx = snprintf(b, nb, "0x%ld", (unsigned long)st->st_nlink);
	    /*@switchbreak@*/ break;
	case STAT_KEYS_UID:
	    xx = snprintf(b, nb, "%ld", (unsigned long)st->st_uid);
	    /*@switchbreak@*/ break;
	case STAT_KEYS_GID:
	    xx = snprintf(b, nb, "%ld", (unsigned long)st->st_gid);
	    /*@switchbreak@*/ break;
	case STAT_KEYS_RDEV:
	    xx = snprintf(b, nb, "0x%lx", (unsigned long)st->st_rdev);
	    /*@switchbreak@*/ break;
	case STAT_KEYS_SIZE:
	    xx = snprintf(b, nb, "%ld", (unsigned long)st->st_size);
	    /*@switchbreak@*/ break;
	case STAT_KEYS_BLKSIZE:
	    xx = snprintf(b, nb, "%ld", (unsigned long)st->st_blksize);
	    /*@switchbreak@*/ break;
	case STAT_KEYS_BLOCKS:
	    xx = snprintf(b, nb, "%ld", (unsigned long)st->st_blocks);
	    /*@switchbreak@*/ break;
	case STAT_KEYS_ATIME:
/*@i@*/	    (void) stpcpy(b, ctime((time_t *)&st->st_atime));
	    /*@switchbreak@*/ break;
	case STAT_KEYS_CTIME:
/*@i@*/	    (void) stpcpy(b, ctime((time_t *)&st->st_ctime));
	    /*@switchbreak@*/ break;
	case STAT_KEYS_MTIME:
/*@i@*/	    (void) stpcpy(b, ctime((time_t *)&st->st_mtime));
	    /*@switchbreak@*/ break;
#ifdef	NOTYET
	case STAT_KEYS_FLAGS:
	    /*@switchbreak@*/ break;
#endif
	case STAT_KEYS_SLINK:
	    if (fn != NULL && S_ISLNK(st->st_mode)) {
		ssize_t size = Readlink(fn, b, nb);
		if (size == -1) {
		    nval = rpmExpand("(Readlink:", fn, ":", strerror(errno), ")", NULL);
		    (void) stpcpy(b, nval);
		    nval = _free(nval);
		} else
		    b[size] = '\0';
	    }
	    /*@switchbreak@*/ break;
	case STAT_KEYS_DIGEST:
	    if (fn != NULL && S_ISREG(st->st_mode)) {
		rpmuint32_t digval = keyValue(keyDigests, nkeyDigests, av[i]);
		rpmuint32_t algo = (digval ? digval : PGPHASHALGO_SHA1);
		FD_t fd = Fopen(fn, "r%{?_rpmgio}");
		if (fd == NULL || Ferror(fd)) {
		    nval = rpmExpand("(Fopen:", fn, ":", Fstrerror(fd), ")", NULL);
		} else {
		    static int asAscii = 1;
		    char buffer[16 * 1024];
		    fdInitDigest(fd, algo, 0);
		    while (Fread(buffer, sizeof(buffer[0]), sizeof(buffer), fd) > 0)
			{};
		    if (Ferror(fd))
			nval = rpmExpand("(Fread:", fn, ":", Fstrerror(fd), ")", NULL);
		    else
			fdFiniDigest(fd, algo, &nval, NULL, asAscii);
	    }
		if (nval) {
		    (void) stpcpy(b, nval);
		    nval = _free(nval);
		}
		if (fd != NULL)
		    xx = Fclose(fd);
	    }
	    /*@switchbreak@*/ break;
	case STAT_KEYS_UNAME:
	{   const char * uname = uidToUname(st->st_uid);
	    if (uname != NULL)
		(void) stpcpy(b, uname);
	    else
		xx = snprintf(b, nb, "%u", (unsigned)st->st_uid);
	}   /*@switchbreak@*/ break;
	case STAT_KEYS_GNAME:
	{   const char * gname = gidToGname(st->st_gid);
	    if (gname != NULL)
		(void) stpcpy(b, gname);
	    else
		xx = snprintf(b, nb, "%u", (unsigned)st->st_gid);
	}   /*@switchbreak@*/ break;
	}
	if (b[0] == '\0')
	    continue;
	b[nb-1] = '\0';

	if (val == NULL)
	    val = xstrdup(b);
	else {
	    nval = rpmExpand(val, " | ", b, NULL);
	    val = _free(val);
	    val = nval;
	}
    }

exit:
    return val;
}

/**
 * Reformat tag string as a UUID.
 * @param he		tag container
 * @param av		parameter list (NULL uses UUIDv5)
 * @return		formatted string
 */
static /*@only@*/ char * uuidFormat(HE_t he, /*@null@*/ const char ** av)
	/*@globals rpmGlobalMacroContext, h_errno, internalState @*/
	/*@modifies rpmGlobalMacroContext, internalState @*/
{
    static const char hex[] = "0123456789abcdef";
    /* XXX use private tag container to avoid memory issues for now. */
    HE_t nhe = (HE_t) memset(alloca(sizeof(*nhe)), 0, sizeof(*nhe));
/*@-nullassign@*/
    /*@unchecked@*/ /*@observer@*/
    static const char *avdefault[] = { "v5", NULL };
/*@=nullassign@*/
    int ix = (he->ix > 0 ? he->ix : 0);
    char * val = NULL;
    struct timeval tv;
    char * t;
    char * te;
    uint32_t i;

assert(ix == 0);
    val = xmalloc((128/4 + 4) + 1);
    *val = '\0';

    nhe->tag = he->tag;
    nhe->t = he->t;
    switch(he->t) {
    default:
	val = _free(val);
	val = xstrdup(_("(invalid type :uuid)"));
	goto exit;
	/*@notreached@*/ break;
    case RPM_UINT64_TYPE:
	/* XXX Limit to tag time stamps with UUIDv1 direct conversion. */
	switch (he->tag) {
	default:
	    val = _free(val);
	    val = xstrdup(_("(invalid tag :uuid)"));
	    goto exit;
	    break;
	case RPMTAG_INSTALLTIME:
	case RPMTAG_BUILDTIME:
	case RPMTAG_ORIGINTIME:
	case RPMTAG_INSTALLTID:
	case RPMTAG_REMOVETID:
	case RPMTAG_ORIGINTID:

	    /* Convert tag time stamp to UUIDv1. */
	    tv.tv_sec = (long) he->p.ui64p[0];
	    tv.tv_usec = (long) (he->c > 1 ? he->p.ui64p[1] : 0);
	    ix = tv2uuidv1(NULL, nhe, &tv);

	    /* Convert UUIDv1 to display string. */
	    te = val;
	    for (i = 0; i < nhe->c; i++) {
		*te++ = hex[ (int)((nhe->p.ui8p[i] >> 4) & 0x0f) ];
		*te++ = hex[ (int)((nhe->p.ui8p[i]     ) & 0x0f) ];
		if (i == 3 || i == 5 || i == 7 || i == 9)
		    *te++ = '-';
	    }
	    *te = '\0';
	    goto exit;	/* XXX immediate exit for UUIDv1 */
	    break;
	}
	break;
    case RPM_BIN_TYPE:
	/* XXX Limit to tag binary digests with djb formatting in UUIDv5. */
	switch (he->tag) {
	default:
	    val = _free(val);
	    val = xstrdup(_("(invalid tag :uuid)"));
	    goto exit;
	    break;
	case RPMTAG_PKGID:
	case RPMTAG_SOURCEPKGID:
	    /* Convert RPMTAG_*PKGID from binary => hex. */
	    t = te = xmalloc(2*he->c + 1);
	    for (i = 0; i < he->c; i++) {
		*te++ = hex[ (int)((he->p.ui8p[i] >> 4) & 0x0f) ];
		*te++ = hex[ (int)((he->p.ui8p[i]     ) & 0x0f) ];
	    }
	    *te = '\0';
	    nhe->t = RPM_STRING_TYPE;
	    nhe->p.ptr = t;
	    nhe->c = 1;
	    break;
	}
	break;
    case RPM_STRING_TYPE:
	nhe->c = 1;
	nhe->p.ptr = xstrdup(he->p.str);
	break;
    }

    if (!(av && av[0] && *av[0]))
	av = avdefault;

    for (i = 0; av[i] != NULL; i++) {
	uint32_t keyval = keyValue(keyUuids, nkeyUuids, av[i]);

	switch (keyval) {
	default:
	    /*@switchbreak@*/ break;
	case UUID_KEYS_V1:
	case UUID_KEYS_V3:
	case UUID_KEYS_V4:
	case UUID_KEYS_V5:
	    ix = str2uuid(nhe, NULL, keyval, val);
	    goto exit;	/* XXX exit after first found. */
	    break;
	}
    }

exit:
    nhe->p.ptr = _free(nhe->p.ptr);
    return val;
}

/**
 * Return arithmetic expressions of input.
 * @param he		tag container
 * @param av		parameter list (NULL uses sha1)
 * @return		formatted string
 */
static /*@only@*/ char * rpnFormat(HE_t he, /*@null@*/ const char ** av)
	/*@*/
{
    int ac = argvCount(av) + 1;
    int64_t * stack = memset(alloca(ac*sizeof(*stack)), 0, (ac*sizeof(*stack)));
    char * end;
    char * val = NULL;
    int ix = 0;
    int i;

    switch(he->t) {
    default:
	val = xstrdup(_("(invalid type :rpn)"));
	goto exit;
	/*@notreached@*/ break;
    case RPM_UINT64_TYPE:
	stack[ix] = he->p.ui64p[0];
	break;
    case RPM_STRING_TYPE:
	end = NULL;
/*@-unrecog@*/	/* Add annotated prototype. */
	stack[ix] = strtoll(he->p.str, &end, 0);
/*@=unrecog@*/
	if (end && *end != '\0') {
	    val = xstrdup(_("(invalid string :rpn)"));
	    goto exit;
	}
	break;
    }

    if (av != NULL)
    for (i = 0; av[i] != NULL; i++) {
	const char * arg = av[i];
	size_t len = strlen(arg);
	int c = (int) *arg;

	if (len == 0) {
	    /* do nothing */
	} else if (len > 1) {
	    if (!(xisdigit(c) || (c == (int)'-' && xisdigit((int) arg[1])))) {
		val = xstrdup(_("(expected number :rpn)"));
		goto exit;
	    }
	    if (++ix == ac) {
		val = xstrdup(_("(stack overflow :rpn)"));
		goto exit;
	    }
	    end = NULL;
	    stack[ix] = strtoll(arg, &end, 0);
	    if (end && *end != '\0') {
		val = xstrdup(_("(invalid number :rpn)"));
		goto exit;
	    }
	} else {
	    if (ix-- < 1) {
		val = xstrdup(_("(stack underflow :rpn)"));
		goto exit;
	    }
	    switch (c) {
	    case '&':	stack[ix] &= stack[ix+1];	/*@switchbreak@*/ break;
	    case '|':	stack[ix] |= stack[ix+1];	/*@switchbreak@*/ break;
	    case '^':	stack[ix] ^= stack[ix+1];	/*@switchbreak@*/ break;
	    case '+':	stack[ix] += stack[ix+1];	/*@switchbreak@*/ break;
	    case '-':	stack[ix] -= stack[ix+1];	/*@switchbreak@*/ break;
	    case '*':	stack[ix] *= stack[ix+1];	/*@switchbreak@*/ break;
	    case '%':	
	    case '/':	
		if (stack[ix+1] == 0) {
		    val = xstrdup(_("(divide by zero :rpn)"));
		    goto exit;
		}
		if (c == (int)'%')
		    stack[ix] %= stack[ix+1];
		else
		    stack[ix] /= stack[ix+1];
		/*@switchbreak@*/ break;
	    }
	}
    }

    {	HE_t nhe = (HE_t) memset(alloca(sizeof(*nhe)), 0, sizeof(*nhe));
	nhe->tag = he->tag;
	nhe->t = RPM_UINT64_TYPE;
	nhe->p.ui64p = (rpmuint64_t *)&stack[ix];
	nhe->c = 1;
	val = intFormat(nhe, NULL, NULL);
    }

exit:
    return val;
}

/**
 * Replace string values.
 * @param he		tag container
 * @param av		parameter list (NULL is an error)
 * @return		formatted string
 */
static /*@only@*/ char * strsubFormat(HE_t he, /*@null@*/ const char ** av)
	/*@globals rpmGlobalMacroContext, h_errno, internalState @*/
	/*@modifies rpmGlobalMacroContext, internalState @*/
{
    char * val = NULL;
    int ac = argvCount(av);
    miRE mires = NULL;
    int nmires = 0;
    int xx;
    int i;

    switch(he->t) {
    default:
	val = xstrdup(_("(invalid type :strsub)"));
	goto exit;
	/*@notreached@*/ break;
    case RPM_STRING_TYPE:
	if (ac < 2 || (ac % 2) != 0) {
	    val = xstrdup(_("(invalid args :strsub)"));
	    goto exit;
	}
	break;
    }
    if (av == NULL)
	goto noop;

    /* Create the mire pattern array. */
    for (i = 0; av[i] != NULL; i += 2)
	xx = mireAppend(RPMMIRE_REGEX, 0, av[i], NULL, &mires, &nmires);

    /* Find-and-replace first pattern that matches. */
    if (mires != NULL) {
	int noffsets = 3;
	int offsets[3];
	const char * s, * se;
	char * t, * te;
	char * nval;
	size_t slen;
	size_t nb;

	for (i = 0; i < nmires; i++) {
	    miRE mire = mires + i;

	    s = he->p.str;
	    slen = strlen(s);
	    if ((xx = mireRegexec(mire, s, slen)) < 0)
		continue;
	    xx = mireSetEOptions(mire, offsets, noffsets);

	    /* Replace the string(s). This is just s/find/replace/g */
	    val = xstrdup("");
	    while (*s != '\0') {
		nb = strlen(s);
		if ((se = strchr(s, '\n')) == NULL)
		    se = s + nb;
		else
		    se++;

		offsets[0] = offsets[1] = -1;
		xx = mireRegexec(mire, s, nb);

		nb = 1;
		/* On match, copy lead-in and match string. */
		if (xx == 0)
		    nb += offsets[0] + strlen(av[2*i+1]);
		/* Copy up to EOL on nomatch or insertion. */
		if (xx != 0 || offsets[1] == offsets[0])
		    nb += (se - (s + offsets[1]));

		te = t = xmalloc(nb);

		/* On match, copy lead-in and match string. */
		if (xx == 0) {
		    te = stpcpy( stpncpy(te, s, offsets[0]), av[2*i+1]);
		    s += offsets[1];
		}
		/* Copy up to EOL on nomatch or insertion. */
		if (xx != 0 || offsets[1] == offsets[0]) {
		    s += offsets[1];
		    te = stpncpy(te, s, (se - s));
		    s = se;
		}
		*te = '\0';

		nval = rpmExpand(val, t, NULL);
		val = _free(val);
		val = nval;
		t = _free(t);
	    }
	}
	mires = mireFreeAll(mires, nmires);
    }

noop:
    if (val == NULL)
	val = xstrdup(he->p.str);
exit:
    return val;
}

static struct headerSprintfExtension_s _headerCompoundFormats[] = {
    { HEADER_EXT_TAG, "RPMTAG_BUILDTIMEUUID",
	{ .tagFunction = buildtime_uuidTag } },
    { HEADER_EXT_TAG, "RPMTAG_CHANGELOGNAME",
	{ .tagFunction = changelognameTag } },
    { HEADER_EXT_TAG, "RPMTAG_CHANGELOGTEXT",
	{ .tagFunction = changelogtextTag } },
    { HEADER_EXT_TAG, "RPMTAG_DESCRIPTION",
	{ .tagFunction = descriptionTag } },
    { HEADER_EXT_TAG, "RPMTAG_GROUP",
	{ .tagFunction = groupTag } },
    { HEADER_EXT_TAG, "RPMTAG_HDRUUID",
	{ .tagFunction = hdruuidTag } },
    { HEADER_EXT_TAG, "RPMTAG_INSTALLPREFIX",
	{ .tagFunction = instprefixTag } },
    { HEADER_EXT_TAG, "RPMTAG_INSTALLTIDUUID",
	{ .tagFunction = installtid_uuidTag } },
    { HEADER_EXT_TAG, "RPMTAG_INSTALLTIMEUUID",
	{ .tagFunction = installtime_uuidTag } },
    { HEADER_EXT_TAG, "RPMTAG_ORIGINTIDUUID",
	{ .tagFunction = origintid_uuidTag } },
    { HEADER_EXT_TAG, "RPMTAG_ORIGINTIMEUUID",
	{ .tagFunction = origintime_uuidTag } },
    { HEADER_EXT_TAG, "RPMTAG_PKGUUID",
	{ .tagFunction = pkguuidTag } },
    { HEADER_EXT_TAG, "RPMTAG_REMOVETIDUUID",
	{ .tagFunction = removetid_uuidTag } },
    { HEADER_EXT_TAG, "RPMTAG_SOURCEPKGUUID",
	{ .tagFunction = sourcepkguuidTag } },
    { HEADER_EXT_TAG, "RPMTAG_SUMMARY",
	{ .tagFunction = summaryTag } },
    { HEADER_EXT_TAG, "RPMTAG_TRIGGERCONDS",
	{ .tagFunction = triggercondsTag } },
    { HEADER_EXT_TAG, "RPMTAG_TRIGGERTYPE",
	{ .tagFunction = triggertypeTag } },
    { HEADER_EXT_TAG, "RPMTAG_DBINSTANCE",
	{ .tagFunction = dbinstanceTag } },
    { HEADER_EXT_TAG, "RPMTAG_HEADERSTARTOFF",
	{ .tagFunction = headerstartoffTag } },
    { HEADER_EXT_TAG, "RPMTAG_HEADERENDOFF",
	{ .tagFunction = headerendoffTag } },
    { HEADER_EXT_TAG, "RPMTAG_PACKAGEBASEURL",
	{ .tagFunction = pkgbaseurlTag } },
    { HEADER_EXT_TAG, "RPMTAG_PACKAGEDIGEST",
	{ .tagFunction = pkgdigestTag } },
    { HEADER_EXT_TAG, "RPMTAG_PACKAGEORIGIN",
	{ .tagFunction = pkgoriginTag } },
    { HEADER_EXT_TAG, "RPMTAG_PACKAGESIZE",
	{ .tagFunction = pkgsizeTag } },
    { HEADER_EXT_TAG, "RPMTAG_PACKAGETIME",
	{ .tagFunction = pkgmtimeTag } },
    { HEADER_EXT_TAG, "RPMTAG_NVRA",
	{ .tagFunction = nvraTag } },
    { HEADER_EXT_TAG, "RPMTAG_FILENAMES",
	{ .tagFunction = filenamesTag } },
    { HEADER_EXT_TAG, "RPMTAG_FILEPATHS",
	{ .tagFunction = filepathsTag } },
    { HEADER_EXT_TAG, "RPMTAG_ORIGPATHS",
	{ .tagFunction = origpathsTag } },
    { HEADER_EXT_TAG, "RPMTAG_FILESTAT",
	{ .tagFunction = filestatTag } },
    { HEADER_EXT_TAG, "RPMTAG_PROVIDEXMLENTRY",
	{ .tagFunction = PxmlTag } },
    { HEADER_EXT_TAG, "RPMTAG_REQUIREXMLENTRY",
	{ .tagFunction = RxmlTag } },
    { HEADER_EXT_TAG, "RPMTAG_CONFLICTXMLENTRY",
	{ .tagFunction = CxmlTag } },
    { HEADER_EXT_TAG, "RPMTAG_OBSOLETEXMLENTRY",
	{ .tagFunction = OxmlTag } },
    { HEADER_EXT_TAG, "RPMTAG_FILESXMLENTRY1",
	{ .tagFunction = F1xmlTag } },
    { HEADER_EXT_TAG, "RPMTAG_FILESXMLENTRY2",
	{ .tagFunction = F2xmlTag } },
    { HEADER_EXT_TAG, "RPMTAG_PROVIDEYAMLENTRY",
	{ .tagFunction = PyamlTag } },
    { HEADER_EXT_TAG, "RPMTAG_REQUIREYAMLENTRY",
	{ .tagFunction = RyamlTag } },
    { HEADER_EXT_TAG, "RPMTAG_CONFLICTYAMLENTRY",
	{ .tagFunction = CyamlTag } },
    { HEADER_EXT_TAG, "RPMTAG_OBSOLETEYAMLENTRY",
	{ .tagFunction = OyamlTag } },
    { HEADER_EXT_TAG, "RPMTAG_FILESYAMLENTRY1",
	{ .tagFunction = F1yamlTag } },
    { HEADER_EXT_TAG, "RPMTAG_FILESYAMLENTRY2",
	{ .tagFunction = F2yamlTag } },
    { HEADER_EXT_TAG, "RPMTAG_PROVIDESQLENTRY",
	{ .tagFunction = PsqlTag } },
    { HEADER_EXT_TAG, "RPMTAG_REQUIRESQLENTRY",
	{ .tagFunction = RsqlTag } },
    { HEADER_EXT_TAG, "RPMTAG_CONFLICTSQLENTRY",
	{ .tagFunction = CsqlTag } },
    { HEADER_EXT_TAG, "RPMTAG_OBSOLETESQLENTRY",
	{ .tagFunction = OsqlTag } },
    { HEADER_EXT_TAG, "RPMTAG_FILESSQLENTRY1",
	{ .tagFunction = F1sqlTag } },
    { HEADER_EXT_TAG, "RPMTAG_FILESSQLENTRY2",
	{ .tagFunction = F2sqlTag } },
    { HEADER_EXT_TAG, "RPMTAG_DEBCONFLICTS",
	{ .tagFunction = debconflictsTag } },
    { HEADER_EXT_TAG, "RPMTAG_DEBDEPENDS",
	{ .tagFunction = debdependsTag } },
    { HEADER_EXT_TAG, "RPMTAG_DEBMD5SUMS",
	{ .tagFunction = debmd5sumsTag } },
    { HEADER_EXT_TAG, "RPMTAG_DEBOBSOLETES",
	{ .tagFunction = debobsoletesTag } },
    { HEADER_EXT_TAG, "RPMTAG_DEBPROVIDES",
	{ .tagFunction = debprovidesTag } },
    { HEADER_EXT_TAG, "RPMTAG_NEEDSWHAT",
	{ .tagFunction = needswhatTag } },
    { HEADER_EXT_TAG, "RPMTAG_WHATNEEDS",
	{ .tagFunction = whatneedsTag } },
    { HEADER_EXT_FORMAT, "armor",
	{ .fmtFunction = armorFormat } },
    { HEADER_EXT_FORMAT, "base64",
	{ .fmtFunction = base64Format } },
    { HEADER_EXT_FORMAT, "bncdata",
	{ .fmtFunction = bncdataFormat } },
    { HEADER_EXT_FORMAT, "cdata",
	{ .fmtFunction = cdataFormat } },
    { HEADER_EXT_FORMAT, "depflags",
	{ .fmtFunction = depflagsFormat } },
    { HEADER_EXT_FORMAT, "deptype",
	{ .fmtFunction = deptypeFormat } },
    { HEADER_EXT_FORMAT, "digest",
	{ .fmtFunction = digestFormat } },
    { HEADER_EXT_FORMAT, "fflags",
	{ .fmtFunction = fflagsFormat } },
    { HEADER_EXT_FORMAT, "iconv",
	{ .fmtFunction = iconvFormat } },
    { HEADER_EXT_FORMAT, "json",
	{ .fmtFunction = jsonFormat } },
#ifndef	DYING	/* XXX is :json gud enuf? there are side effects ... */
    { HEADER_EXT_FORMAT, "jsonescape",
	{ .fmtFunction = jsonescapeFormat } },
#endif
    { HEADER_EXT_FORMAT, "perms",
	{ .fmtFunction = permsFormat } },
    { HEADER_EXT_FORMAT, "permissions",	
	{ .fmtFunction = permsFormat } },
    { HEADER_EXT_FORMAT, "pgpsig",
	{ .fmtFunction = pgpsigFormat } },
    { HEADER_EXT_FORMAT, "rpn",
	{ .fmtFunction = rpnFormat } },
    { HEADER_EXT_FORMAT, "sqlescape",
	{ .fmtFunction = sqlescapeFormat } },
    { HEADER_EXT_FORMAT, "stat",
	{ .fmtFunction = statFormat } },
    { HEADER_EXT_FORMAT, "strsub",
	{ .fmtFunction = strsubFormat } },
    { HEADER_EXT_FORMAT, "triggertype",	
	{ .fmtFunction = triggertypeFormat } },
    { HEADER_EXT_FORMAT, "utf8",
	{ .fmtFunction = iconvFormat } },
    { HEADER_EXT_FORMAT, "uuid",
	{ .fmtFunction = uuidFormat } },
    { HEADER_EXT_FORMAT, "xml",
	{ .fmtFunction = xmlFormat } },
    { HEADER_EXT_FORMAT, "yaml",
	{ .fmtFunction = yamlFormat } },
    { HEADER_EXT_MORE, NULL,		{ (void *) &headerDefaultFormats } }
} ;

headerSprintfExtension headerCompoundFormats = &_headerCompoundFormats[0];

/*====================================================================*/

void rpmDisplayQueryTags(FILE * fp, headerTagTableEntry _rpmTagTable, headerSprintfExtension _rpmHeaderFormats)
{
    const struct headerTagTableEntry_s * t;
    headerSprintfExtension exts;
    headerSprintfExtension ext;
    int extNum;

    if (fp == NULL)
	fp = stdout;
    if (_rpmTagTable == NULL)
	_rpmTagTable = rpmTagTable;

    /* XXX this should use rpmHeaderFormats, but there are linkage problems. */
    if (_rpmHeaderFormats == NULL)
	_rpmHeaderFormats = headerCompoundFormats;

    for (t = _rpmTagTable; t && t->name; t++) {
	/*@observer@*/
	static const char * tagtypes[] = {
		"", "char", "uint8", "uint16", "uint32", "uint64",
		"string", "octets", "argv", "i18nstring",
	};
	rpmuint32_t ttype;

	if (rpmIsVerbose()) {
	    fprintf(fp, "%-20s %6d", t->name + 7, t->val);
	    ttype = t->type & RPM_MASK_TYPE;
	    if (ttype < RPM_MIN_TYPE || ttype > RPM_MAX_TYPE)
		continue;
	    if (t->type & RPM_OPENPGP_RETURN_TYPE)
		fprintf(fp, " openpgp");
	    if (t->type & RPM_X509_RETURN_TYPE)
		fprintf(fp, " x509");
	    if (t->type & RPM_ASN1_RETURN_TYPE)
		fprintf(fp, " asn1");
	    if (t->type & RPM_OPAQUE_RETURN_TYPE)
		fprintf(fp, " opaque");
	    fprintf(fp, " %s", tagtypes[ttype]);
	    if (t->type & RPM_ARRAY_RETURN_TYPE)
		fprintf(fp, " array");
	    if (t->type & RPM_MAPPING_RETURN_TYPE)
		fprintf(fp, " mapping");
	    if (t->type & RPM_PROBE_RETURN_TYPE)
		fprintf(fp, " probe");
	    if (t->type & RPM_TREE_RETURN_TYPE)
		fprintf(fp, " tree");
	} else
	    fprintf(fp, "%s", t->name + 7);
	fprintf(fp, "\n");
    }

    exts = _rpmHeaderFormats;
    for (ext = exts, extNum = 0; ext != NULL && ext->type != HEADER_EXT_LAST;
	ext = (ext->type == HEADER_EXT_MORE ? *ext->u.more : ext+1), extNum++)
    {
	if (ext->name == NULL || ext->type != HEADER_EXT_TAG)
	    continue;

	/* XXX don't print header tags twice. */
	if (tagValue(ext->name) > 0)
	    continue;
	fprintf(fp, "%s\n", ext->name + 7);
    }
}

/*====================================================================*/

#define PARSER_BEGIN 	0
#define PARSER_IN_ARRAY 1
#define PARSER_IN_EXPR  2

/** \ingroup header
 */
typedef /*@abstract@*/ struct sprintfTag_s * sprintfTag;

/** \ingroup header
 */
struct sprintfTag_s {
    HE_s he;
/*@null@*/
    headerTagFormatFunction * fmtfuncs;
/*@null@*/
    headerTagTagFunction ext;   /*!< NULL if tag element is invalid */
    int extNum;
/*@only@*/ /*@relnull@*/
    rpmTag * tagno;
    int justOne;
    int arrayCount;
/*@kept@*/
    char * format;
/*@only@*/ /*@relnull@*/
    ARGV_t av;
/*@only@*/ /*@relnull@*/
    ARGV_t params;
    unsigned pad;
};

/** \ingroup header
 */
typedef /*@abstract@*/ struct sprintfToken_s * sprintfToken;

/** \ingroup header
 */
typedef enum {
        PTOK_NONE       = 0,
        PTOK_TAG        = 1,
        PTOK_ARRAY      = 2,
        PTOK_STRING     = 3,
        PTOK_COND       = 4
} sprintfToken_e;

/** \ingroup header
 */
struct sprintfToken_s {
    sprintfToken_e type;
    union {
	struct sprintfTag_s tag;	/*!< PTOK_TAG */
	struct {
	/*@only@*/
	    sprintfToken format;
	    size_t numTokens;
	} array;			/*!< PTOK_ARRAY */
	struct {
	/*@dependent@*/
	    char * string;
	    size_t len;
	} string;			/*!< PTOK_STRING */
	struct {
	/*@only@*/ /*@null@*/
	    sprintfToken ifFormat;
	    size_t numIfTokens;
	/*@only@*/ /*@null@*/
	    sprintfToken elseFormat;
	    size_t numElseTokens;
	    struct sprintfTag_s tag;
	} cond;				/*!< PTOK_COND */
    } u;
};

/** \ingroup header
 */
typedef /*@abstract@*/ struct headerSprintfArgs_s * headerSprintfArgs;

/** \ingroup header
 */
struct headerSprintfArgs_s {
    Header h;
    char * fmt;
/*@observer@*/ /*@temp@*/
    headerTagTableEntry tags;
/*@observer@*/ /*@temp@*/
    headerSprintfExtension exts;
/*@observer@*/ /*@null@*/
    const char * errmsg;
    HE_t ec;			/*!< Extension data cache. */
    int nec;			/*!< No. of extension cache items. */
    sprintfToken format;
/*@relnull@*/
    HeaderIterator hi;
/*@owned@*/
    char * val;
    size_t vallen;
    size_t alloced;
    size_t numTokens;
    size_t i;
};

/*@access sprintfTag @*/
/*@access sprintfToken @*/
/*@access headerSprintfArgs @*/

/**
 */
static char escapedChar(const char ch)
	/*@*/
{
/*@-modfilesys@*/
if (_hdrqf_debug)
fprintf(stderr, "\t\t\\%c\n", ch);
/*@=modfilesys@*/
    switch (ch) {
    case 'a': 	return '\a';
    case 'b': 	return '\b';
    case 'f': 	return '\f';
    case 'n': 	return '\n';
    case 'r': 	return '\r';
    case 't': 	return '\t';
    case 'v': 	return '\v';
    default:	return ch;
    }
}

/**
 * Clean a tag container, free'ing attached malloc's.
 * @param he		tag container
 */
/*@relnull@*/
static HE_t rpmheClean(/*@returned@*/ /*@null@*/ HE_t he)
	/*@modifies he @*/
{
    if (he) {
	if (he->freeData && he->p.ptr != NULL)
	    he->p.ptr = _free(he->p.ptr);
	memset(he, 0, sizeof(*he));
    }
    return he;
}

/**
 * Destroy headerSprintf format array.
 * @param format	sprintf format array
 * @param num		number of elements
 * @return		NULL always
 */
static /*@null@*/ sprintfToken
freeFormat( /*@only@*/ /*@null@*/ sprintfToken format, size_t num)
	/*@modifies *format @*/
{
    unsigned i;

    if (format == NULL) return NULL;

    for (i = 0; i < (unsigned) num; i++) {
	switch (format[i].type) {
	case PTOK_TAG:
	    (void) rpmheClean(&format[i].u.tag.he);
	    format[i].u.tag.tagno = _free(format[i].u.tag.tagno);
	    format[i].u.tag.av = argvFree(format[i].u.tag.av);
	    format[i].u.tag.params = argvFree(format[i].u.tag.params);
/*@-type@*/
	    format[i].u.tag.fmtfuncs = _free(format[i].u.tag.fmtfuncs);
/*@=type@*/
	    /*@switchbreak@*/ break;
	case PTOK_ARRAY:
	    format[i].u.array.format =
		freeFormat(format[i].u.array.format,
			format[i].u.array.numTokens);
	    /*@switchbreak@*/ break;
	case PTOK_COND:
	    format[i].u.cond.ifFormat =
		freeFormat(format[i].u.cond.ifFormat, 
			format[i].u.cond.numIfTokens);
	    format[i].u.cond.elseFormat =
		freeFormat(format[i].u.cond.elseFormat, 
			format[i].u.cond.numElseTokens);
	    (void) rpmheClean(&format[i].u.cond.tag.he);
	    format[i].u.cond.tag.tagno = _free(format[i].u.cond.tag.tagno);
	    format[i].u.cond.tag.av = argvFree(format[i].u.cond.tag.av);
	    format[i].u.cond.tag.params = argvFree(format[i].u.cond.tag.params);
/*@-type@*/
	    format[i].u.cond.tag.fmtfuncs = _free(format[i].u.cond.tag.fmtfuncs);
/*@=type@*/
	    /*@switchbreak@*/ break;
	case PTOK_NONE:
	case PTOK_STRING:
	default:
	    /*@switchbreak@*/ break;
	}
    }
    format = _free(format);
    return NULL;
}

/**
 * Initialize an hsa iteration.
 * @param hsa		headerSprintf args
 * @return		headerSprintf args
 */
static headerSprintfArgs hsaInit(/*@returned@*/ headerSprintfArgs hsa)
	/*@globals fileSystem @*/
	/*@modifies hsa, fileSystem @*/
{
    sprintfTag tag =
	(hsa->format->type == PTOK_TAG
	    ? &hsa->format->u.tag :
	(hsa->format->type == PTOK_ARRAY
	    ? &hsa->format->u.array.format->u.tag :
	NULL));

    if (hsa != NULL) {
	hsa->i = 0;
	if (tag != NULL && tag->tagno != NULL && tag->tagno[0] == (rpmTag)-2)
	    hsa->hi = headerInit(hsa->h);
    }
/*@-nullret@*/
    return hsa;
/*@=nullret@*/
}

/**
 * Return next hsa iteration item.
 * @param hsa		headerSprintf args
 * @return		next sprintfToken (or NULL)
 */
/*@null@*/
static sprintfToken hsaNext(/*@returned@*/ headerSprintfArgs hsa)
	/*@globals internalState @*/
	/*@modifies hsa, internalState @*/
{
    sprintfToken fmt = NULL;
    sprintfTag tag =
	(hsa->format->type == PTOK_TAG
	    ? &hsa->format->u.tag :
	(hsa->format->type == PTOK_ARRAY
	    ? &hsa->format->u.array.format->u.tag :
	NULL));

    if (hsa != NULL && hsa->i < hsa->numTokens) {
	fmt = hsa->format + hsa->i;
	if (hsa->hi == NULL) {
	    hsa->i++;
	} else {
	    HE_t he = rpmheClean(&tag->he);
	    if (!headerNext(hsa->hi, he, 0))
	    {
		tag->tagno[0] = 0;
		return NULL;
	    }
	    he->avail = 1;
	    tag->tagno[0] = he->tag;
	}
    }

/*@-dependenttrans -onlytrans@*/
    return fmt;
/*@=dependenttrans =onlytrans@*/
}

/**
 * Finish an hsa iteration.
 * @param hsa		headerSprintf args
 * @return		headerSprintf args
 */
static headerSprintfArgs hsaFini(/*@returned@*/ headerSprintfArgs hsa)
	/*@globals fileSystem @*/
	/*@modifies hsa, fileSystem @*/
{
    if (hsa != NULL) {
	hsa->hi = headerFini(hsa->hi);
	hsa->i = 0;
    }
/*@-nullret@*/
    return hsa;
/*@=nullret@*/
}

/**
 * Reserve sufficient buffer space for next output value.
 * @param hsa		headerSprintf args
 * @param need		no. of bytes to reserve
 * @return		pointer to reserved space
 */
/*@dependent@*/ /*@exposed@*/
static char * hsaReserve(headerSprintfArgs hsa, size_t need)
	/*@modifies hsa */
{
    if ((hsa->vallen + need) >= hsa->alloced) {
	if (hsa->alloced <= need)
	    hsa->alloced += need;
	hsa->alloced <<= 1;
	hsa->val = xrealloc(hsa->val, hsa->alloced+1);	
    }
    return hsa->val + hsa->vallen;
}

/**
 * Return tag name from value.
 * @param tbl		tag table
 * @param val		tag value to find
 * @retval *typep	tag type (or NULL)
 * @return		tag name, NULL on not found
 */
/*@observer@*/ /*@null@*/
static const char * myTagName(headerTagTableEntry tbl, rpmuint32_t val,
		/*@null@*/ rpmuint32_t *typep)
	/*@modifies *typep @*/
{
    static char name[128];	/* XXX Ick. */
    const char * s;
    char *t;

    /* XXX Use bsearch on the "normal" rpmTagTable lookup. */
    if (tbl == NULL || tbl == rpmTagTable) {
	s = tagName(val);
	if (s != NULL && typep != NULL)
	    *typep = tagType(val);
	return s;
    }

    for (; tbl->name != NULL; tbl++) {
	if (tbl->val == val)
	    break;
    }
    if ((s = tbl->name) == NULL)
	return NULL;
    s += sizeof("RPMTAG_") - 1;
    t = name;
    *t++ = *s++;
    while (*s != '\0')
	*t++ = (char)xtolower((int)*s++);
    *t = '\0';
    if (typep)
	*typep = tbl->type;
    return name;
}

/**
 * Return tag value from name.
 * @param tbl		tag table
 * @param name		tag name to find
 * @return		tag value, 0 on not found
 */
static rpmuint32_t myTagValue(headerTagTableEntry tbl, const char * name)
	/*@*/
{
    rpmuint32_t val = 0;

    /* XXX Use bsearch on the "normal" rpmTagTable lookup. */
    if (tbl == NULL || tbl == rpmTagTable)
	val = tagValue(name);
    else
    for (; tbl->name != NULL; tbl++) {
	if (xstrcasecmp(tbl->name, name))
	    continue;
	val = tbl->val;
	break;
    }
    return val;
}

/**
 * Search extensions and tags for a name.
 * @param hsa		headerSprintf args
 * @param token		parsed fields
 * @param name		name to find
 * @return		0 on success, 1 on not found
 */
static int findTag(headerSprintfArgs hsa, sprintfToken token, const char * name)
	/*@modifies token @*/
{
    headerSprintfExtension exts = hsa->exts;
    headerSprintfExtension ext;
    sprintfTag stag = (token->type == PTOK_COND
	? &token->u.cond.tag : &token->u.tag);
    int extNum;
    rpmTag tagno = (rpmTag)-1;

    stag->fmtfuncs = NULL;
    stag->ext = NULL;
    stag->extNum = 0;

    if (!strcmp(name, "*")) {
	tagno = (rpmTag)-2;
	goto bingo;
    }

    if (strncmp("RPMTAG_", name, sizeof("RPMTAG_")-1)) {
	char * t = alloca(strlen(name) + sizeof("RPMTAG_"));
	(void) stpcpy( stpcpy(t, "RPMTAG_"), name);
	name = t;
    }

    /* Search extensions for specific tag override. */
    for (ext = exts, extNum = 0; ext != NULL && ext->type != HEADER_EXT_LAST;
	ext = (ext->type == HEADER_EXT_MORE ? *ext->u.more : ext+1), extNum++)
    {
	if (ext->name == NULL || ext->type != HEADER_EXT_TAG)
	    continue;
	if (!xstrcasecmp(ext->name, name)) {
	    stag->ext = ext->u.tagFunction;
	    stag->extNum = extNum;
	    tagno = tagValue(name);
	    goto bingo;
	}
    }

    /* Search tag names. */
    tagno = myTagValue(hsa->tags, name);
    if (tagno != 0)
	goto bingo;

    return 1;

bingo:
    stag->tagno = xcalloc(1, sizeof(*stag->tagno));
    stag->tagno[0] = tagno;
    /* Search extensions for specific format(s). */
    if (stag->av != NULL) {
	int i;
/*@-type@*/
	stag->fmtfuncs = xcalloc(argvCount(stag->av) + 1, sizeof(*stag->fmtfuncs));
/*@=type@*/
	for (i = 0; stag->av[i] != NULL; i++) {
	    for (ext = exts; ext != NULL && ext->type != HEADER_EXT_LAST;
		 ext = (ext->type == HEADER_EXT_MORE ? *ext->u.more : ext+1))
	    {
		if (ext->name == NULL || ext->type != HEADER_EXT_FORMAT)
		    /*@innercontinue@*/ continue;
		if (strcmp(ext->name, stag->av[i]+1))
		    /*@innercontinue@*/ continue;
		stag->fmtfuncs[i] = ext->u.fmtFunction;
		/*@innerbreak@*/ break;
	    }
	}
    }
    return 0;
}

/* forward ref */
/**
 * Parse a headerSprintf expression.
 * @param hsa		headerSprintf args
 * @param token
 * @param str
 * @retval *endPtr
 * @return		0 on success
 */
static int parseExpression(headerSprintfArgs hsa, sprintfToken token,
		char * str, /*@out@*/char ** endPtr)
	/*@modifies hsa, str, token, *endPtr @*/
	/*@requires maxSet(endPtr) >= 0 @*/;

/**
 * Parse a headerSprintf term.
 * @param hsa		headerSprintf args
 * @param str
 * @retval *formatPtr
 * @retval *numTokensPtr
 * @retval *endPtr
 * @param state
 * @return		0 on success
 */
static int parseFormat(headerSprintfArgs hsa, char * str,
		/*@out@*/ sprintfToken * formatPtr,
		/*@out@*/ size_t * numTokensPtr,
		/*@null@*/ /*@out@*/ char ** endPtr, int state)
	/*@modifies hsa, str, *formatPtr, *numTokensPtr, *endPtr @*/
	/*@requires maxSet(formatPtr) >= 0 /\ maxSet(numTokensPtr) >= 0
		/\ maxSet(endPtr) >= 0 @*/
{
/*@observer@*/
static const char *pstates[] = {
"NORMAL", "ARRAY", "EXPR", "WTF?"
};
    char * chptr, * start, * next, * dst;
    sprintfToken format;
    sprintfToken token;
    size_t numTokens;
    unsigned i;
    int done = 0;
    int xx;

/*@-modfilesys@*/
if (_hdrqf_debug)
fprintf(stderr, "-->     parseFormat(%p, \"%.20s...\", %p, %p, %p, %s)\n", hsa, str, formatPtr, numTokensPtr, endPtr, pstates[(state & 0x3)]);
/*@=modfilesys@*/

    /* upper limit on number of individual formats */
    numTokens = 0;
    if (str != NULL)
    for (chptr = str; *chptr != '\0'; chptr++)
	if (*chptr == '%' || *chptr == '[') numTokens++;
    numTokens = numTokens * 2 + 1;

    format = xcalloc(numTokens, sizeof(*format));
    if (endPtr) *endPtr = NULL;

/*@-infloops@*/ /* LCL: can't detect (start, *start) termination */
    dst = start = str;
    numTokens = 0;
    token = NULL;
    if (start != NULL)
    while (*start != '\0') {
	switch (*start) {
	case '%':
	    /* handle %% */
	    if (*(start + 1) == '%') {
		if (token == NULL || token->type != PTOK_STRING) {
		    token = format + numTokens++;
		    token->type = PTOK_STRING;
/*@-temptrans -assignexpose@*/
		    dst = token->u.string.string = start;
/*@=temptrans =assignexpose@*/
		}
		start++;
		*dst++ = *start++;
		/*@switchbreak@*/ break;
	    } 

	    token = format + numTokens++;
	    *dst++ = '\0';
	    start++;

	    if (*start == '|') {
		char * newEnd;

		start++;
		if (parseExpression(hsa, token, start, &newEnd))
		{
		    format = freeFormat(format, numTokens);
		    return 1;
		}
		start = newEnd;
		/*@switchbreak@*/ break;
	    }

/*@-assignexpose@*/
	    token->u.tag.format = start;
/*@=assignexpose@*/
	    token->u.tag.pad = 0;
	    token->u.tag.justOne = 0;
	    token->u.tag.arrayCount = 0;

	    chptr = start;
	    while (*chptr && *chptr != '{' && *chptr != '%') chptr++;
	    if (!*chptr || *chptr == '%') {
		hsa->errmsg = _("missing { after %");
		format = freeFormat(format, numTokens);
		return 1;
	    }

/*@-modfilesys@*/
if (_hdrqf_debug)
fprintf(stderr, "\tchptr *%p = NUL\n", chptr);
/*@=modfilesys@*/
	    *chptr++ = '\0';

	    while (start < chptr) {
		if (xisdigit((int)*start)) {
		    i = strtoul(start, &start, 10);
		    token->u.tag.pad += i;
		    start = chptr;
		    /*@innerbreak@*/ break;
		} else {
		    start++;
		}
	    }

	    if (*start == '=') {
		token->u.tag.justOne = 1;
		start++;
	    } else if (*start == '#') {
		token->u.tag.justOne = 1;
		token->u.tag.arrayCount = 1;
		start++;
	    }

	    next = start;
	    while (*next && *next != '}') next++;
	    if (!*next) {
		hsa->errmsg = _("missing } after %{");
		format = freeFormat(format, numTokens);
		return 1;
	    }
/*@-modfilesys@*/
if (_hdrqf_debug)
fprintf(stderr, "\tnext *%p = NUL\n", next);
/*@=modfilesys@*/
	    *next++ = '\0';

#define	isSEP(_c)	((_c) == ':' || (_c) == '|')
	    chptr = start;
	    while (!(*chptr == '\0' || isSEP(*chptr))) chptr++;
	    /* Split ":bing|bang:boom" --qf pipeline formatters (if any) */
	    while (isSEP(*chptr)) {
		if (chptr[1] == '\0' || isSEP(chptr[1])) {
		    hsa->errmsg = _("empty tag format");
		    format = freeFormat(format, numTokens);
		    return 1;
		}
		/* Parse the formatter parameter list. */
		{   char * te = chptr + 1;
		    char * t = strchr(te, '(');
		    char c;

		    while (!(*te == '\0' || isSEP(*te))) {
#ifdef	NOTYET	/* XXX some means of escaping is needed */
			if (te[0] == '\\' && te[1] != '\0') te++;
#endif
			te++;
		    }
		    c = *te; *te = '\0';
		    /* Parse (a,b,c) parameter list. */
		    if (t != NULL) {
			*t++ = '\0';
			if (te <= t || te[-1] != ')') {
			    *te = c;	/* coverity #1214085 */
			    hsa->errmsg = _("malformed parameter list");
			    format = freeFormat(format, numTokens);
			    return 1;
			}
			te[-1] = '\0';
			xx = argvAdd(&token->u.tag.params, t);
		    } else
			xx = argvAdd(&token->u.tag.params, "");
/*@-modfilesys@*/
if (_hdrqf_debug)
fprintf(stderr, "\tformat \"%s\" params \"%s\"\n", chptr, (t ? t : ""));
/*@=modfilesys@*/
		    xx = argvAdd(&token->u.tag.av, chptr);
		    *te = c;
		    *chptr = '\0';
		    chptr = te;
		}
	    }
#undef	isSEP
	    
	    if (*start == '\0') {
		hsa->errmsg = _("empty tag name");
		format = freeFormat(format, numTokens);
		return 1;
	    }

	    i = 0;
	    token->type = PTOK_TAG;

	    if (findTag(hsa, token, start)) {
		hsa->errmsg = _("unknown tag");
		format = freeFormat(format, numTokens);
		return 1;
	    }

	    dst = start = next;
/*@-modfilesys@*/
if (_hdrqf_debug)
fprintf(stderr, "\tdst = start = next %p\n", dst);
/*@=modfilesys@*/
	    /*@switchbreak@*/ break;

	case '[':
/*@-modfilesys@*/
if (_hdrqf_debug)
fprintf(stderr, "\t%s => %s *%p = NUL\n", pstates[(state & 0x3)], pstates[PARSER_IN_ARRAY], start);
/*@=modfilesys@*/
	    *start++ = '\0';
	    token = format + numTokens++;

	    if (parseFormat(hsa, start,
			    &token->u.array.format,
			    &token->u.array.numTokens,
			    &start, PARSER_IN_ARRAY))
	    {
		format = freeFormat(format, numTokens);
		return 1;
	    }

	    if (!start) {
		hsa->errmsg = _("] expected at end of array");
		format = freeFormat(format, numTokens);
		return 1;
	    }

	    dst = start;
/*@-modfilesys@*/
if (_hdrqf_debug)
fprintf(stderr, "\tdst = start %p\n", dst);
/*@=modfilesys@*/

	    token->type = PTOK_ARRAY;

	    /*@switchbreak@*/ break;

	case ']':
	    if (state != PARSER_IN_ARRAY) {
		hsa->errmsg = _("unexpected ]");
		format = freeFormat(format, numTokens);
		return 1;
	    }
	    *start++ = '\0';
/*@-modfilesys@*/
if (_hdrqf_debug)
fprintf(stderr, "\t<= %s %p[-1] = NUL\n", pstates[(state & 0x3)], start);
/*@=modfilesys@*/
	    if (endPtr) *endPtr = start;
	    done = 1;
	    /*@switchbreak@*/ break;

	case '}':
	    if (state != PARSER_IN_EXPR) {
		hsa->errmsg = _("unexpected }");
		format = freeFormat(format, numTokens);
		return 1;
	    }
	    *start++ = '\0';
/*@-modfilesys@*/
if (_hdrqf_debug)
fprintf(stderr, "\t<= %s %p[-1] = NUL\n", pstates[(state & 0x3)], start);
/*@=modfilesys@*/
	    if (endPtr) *endPtr = start;
	    done = 1;
	    /*@switchbreak@*/ break;

	default:
	    if (token == NULL || token->type != PTOK_STRING) {
		token = format + numTokens++;
		token->type = PTOK_STRING;
/*@-temptrans -assignexpose@*/
		dst = token->u.string.string = start;
/*@=temptrans =assignexpose@*/
	    }

/*@-modfilesys@*/
if (_hdrqf_debug)
fprintf(stderr, "\t*%p = *%p \"%.30s\"\n", dst, start, start);
/*@=modfilesys@*/
	    if (start[0] == '\\' && start[1] != '\0') {
		start++;
		*dst++ = escapedChar(*start);
		*start++ = '\0';
	    } else {
		*dst++ = *start++;
	    }
	    /*@switchbreak@*/ break;
	}
	if (dst < start) *dst = '\0';
	if (done)
	    break;
    }
/*@=infloops@*/

    if (dst != NULL)
        *dst = '\0';

    for (i = 0; i < (unsigned) numTokens; i++) {
	token = format + i;
	switch(token->type) {
	default:
	    /*@switchbreak@*/ break;
	case PTOK_STRING:
	    token->u.string.len = strlen(token->u.string.string);
	    /*@switchbreak@*/ break;
	}
    }

    if (numTokensPtr != NULL)
	*numTokensPtr = numTokens;
    if (formatPtr != NULL)
	*formatPtr = format;

    return 0;
}

static int parseExpression(headerSprintfArgs hsa, sprintfToken token,
		char * str, /*@out@*/ char ** endPtr)
{
    char * chptr;
    char * end;

/*@-modfilesys@*/
if (_hdrqf_debug)
fprintf(stderr, "-->   parseExpression(%p, %p, \"%.20s...\", %p)\n", hsa, token, str, endPtr);
/*@=modfilesys@*/

    hsa->errmsg = NULL;
    chptr = str;
    while (*chptr && *chptr != '?') chptr++;

    if (*chptr != '?') {
	hsa->errmsg = _("? expected in expression");
	return 1;
    }

    *chptr++ = '\0';

    if (*chptr != '{') {
	hsa->errmsg = _("{ expected after ? in expression");
	return 1;
    }

    chptr++;

    if (parseFormat(hsa, chptr, &token->u.cond.ifFormat, 
		    &token->u.cond.numIfTokens, &end, PARSER_IN_EXPR)) 
	return 1;

    /* XXX fix segfault on "rpm -q rpm --qf='%|NAME?{%}:{NAME}|\n'"*/
    if (!(end && *end)) {
	hsa->errmsg = _("} expected in expression");
	token->u.cond.ifFormat =
		freeFormat(token->u.cond.ifFormat, token->u.cond.numIfTokens);
	return 1;
    }

    chptr = end;
    if (*chptr != ':' && *chptr != '|') {
	hsa->errmsg = _(": expected following ? subexpression");
	token->u.cond.ifFormat =
		freeFormat(token->u.cond.ifFormat, token->u.cond.numIfTokens);
	return 1;
    }

    if (*chptr == '|') {
	if (parseFormat(hsa, NULL, &token->u.cond.elseFormat, 
		&token->u.cond.numElseTokens, &end, PARSER_IN_EXPR))
	{
	    token->u.cond.ifFormat =
		freeFormat(token->u.cond.ifFormat, token->u.cond.numIfTokens);
	    return 1;
	}
    } else {
	chptr++;

	if (*chptr != '{') {
	    hsa->errmsg = _("{ expected after : in expression");
	    token->u.cond.ifFormat =
		freeFormat(token->u.cond.ifFormat, token->u.cond.numIfTokens);
	    return 1;
	}

	chptr++;

	if (parseFormat(hsa, chptr, &token->u.cond.elseFormat, 
			&token->u.cond.numElseTokens, &end, PARSER_IN_EXPR)) 
	    return 1;

	/* XXX fix segfault on "rpm -q rpm --qf='%|NAME?{a}:{%}|{NAME}\n'" */
	if (!(end && *end)) {
	    hsa->errmsg = _("} expected in expression");
	    token->u.cond.ifFormat =
		freeFormat(token->u.cond.ifFormat, token->u.cond.numIfTokens);
	    return 1;
	}

	chptr = end;
	if (*chptr != '|') {
	    hsa->errmsg = _("| expected at end of expression");
	    token->u.cond.ifFormat =
		freeFormat(token->u.cond.ifFormat, token->u.cond.numIfTokens);
	    token->u.cond.elseFormat =
		freeFormat(token->u.cond.elseFormat, token->u.cond.numElseTokens);
	    return 1;
	}
    }
	
    chptr++;

    *endPtr = chptr;

    token->type = PTOK_COND;

    (void) findTag(hsa, token, str);

    return 0;
}

/**
 * Call a header extension only once, saving results.
 * @param hsa		headerSprintf args
 * @param fn		function
 * @retval he		tag container
 * @retval ec		extension cache
 * @return		1 on success, 0 on failure
 */
static int getExtension(headerSprintfArgs hsa, headerTagTagFunction fn,
		HE_t he, HE_t ec)
	/*@modifies he, ec @*/
{
    int rc = 0;
    if (!ec->avail) {
	he = rpmheClean(he);
	rc = fn(hsa->h, he);
	*ec = *he;	/* structure copy. */
	if (!rc)
	    ec->avail = 1;
    } else
	*he = *ec;	/* structure copy. */
    he->freeData = 0;
    rc = (rc == 0);	/* XXX invert getExtension return. */
    return rc;
}

/**
 * Format a single item's value.
 * @param hsa		headerSprintf args
 * @param tag		tag
 * @param element	element index
 * @return		end of formatted string (NULL on error)
 */
/*@observer@*/ /*@null@*/
static char * formatValue(headerSprintfArgs hsa, sprintfTag tag,
		size_t element)
	/*@globals rpmGlobalMacroContext, h_errno, internalState @*/
	/*@modifies hsa, tag, rpmGlobalMacroContext, internalState @*/
{
    HE_t vhe = (HE_t) memset(alloca(sizeof(*vhe)), 0, sizeof(*vhe));
    HE_t he = &tag->he;
    char * val = NULL;
    size_t need = 0;
    char * t, * te;
    rpmuint64_t ival = 0;
    rpmTagCount countBuf;
    int xx;

    if (!he->avail) {
	if (tag->ext)
	    xx = getExtension(hsa, tag->ext, he, hsa->ec + tag->extNum);
	else {
	    he->tag = tag->tagno[0];	/* XXX necessary? */
	    xx = headerGet(hsa->h, he, 0);
	}
	if (!xx) {
	    (void) rpmheClean(he);
	    he->t = RPM_STRING_TYPE;	
	    he->p.str = xstrdup("(none)");
	    he->c = 1;
	    he->freeData = 1;
	}
	he->avail = 1;
    }

    if (tag->arrayCount) {
	countBuf = he->c;
	he = rpmheClean(he);
	he->t = RPM_UINT32_TYPE;
	he->p.ui32p = &countBuf;
	he->c = 1;
	he->freeData = 0;
    }

    vhe->tag = he->tag;

    if (he->p.ptr)
    switch (he->t) {
    default:
	val = xstrdup("(unknown type)");
	need = strlen(val) + 1;
	goto exit;
	/*@notreached@*/ break;
#if defined(SUPPORT_I18NSTRING_TYPE)
    case RPM_I18NSTRING_TYPE:
#endif
    case RPM_STRING_ARRAY_TYPE:
	vhe->t = RPM_STRING_TYPE;
	vhe->p.str = he->p.argv[element];
	vhe->c = he->c;
	vhe->ix = (he->t == RPM_STRING_ARRAY_TYPE || he->c > 1 ? 0 : -1);
	break;
#if !defined(SUPPORT_I18NSTRING_TYPE)
    case RPM_I18NSTRING_TYPE:
assert(0);
#endif
    case RPM_STRING_TYPE:
	vhe->p.str = he->p.str;
	vhe->t = RPM_STRING_TYPE;
	vhe->c = 0;
	vhe->ix = -1;
	break;
    case RPM_UINT8_TYPE:
    case RPM_UINT16_TYPE:
    case RPM_UINT32_TYPE:
    case RPM_UINT64_TYPE:
	switch (he->t) {
	default:
assert(0);	/* XXX keep gcc quiet. */
	    /*@innerbreak@*/ break;
	case RPM_UINT8_TYPE:
	    ival = (rpmuint64_t)he->p.ui8p[element];
	    /*@innerbreak@*/ break;
	case RPM_UINT16_TYPE:
	    ival = (rpmuint64_t)he->p.ui16p[element];
	    /*@innerbreak@*/ break;
	case RPM_UINT32_TYPE:
	    ival = (rpmuint64_t)he->p.ui32p[element];
	    /*@innerbreak@*/ break;
	case RPM_UINT64_TYPE:
	    ival = he->p.ui64p[element];
	    /*@innerbreak@*/ break;
	}
	vhe->t = RPM_UINT64_TYPE;
	vhe->p.ui64p = &ival;
	vhe->c = he->c;
	vhe->ix = (he->c > 1 ? 0 : -1);
	if ((tagType(he->tag) & RPM_MASK_RETURN_TYPE) == RPM_ARRAY_RETURN_TYPE)
	    vhe->ix = 0;
	break;

    case RPM_BIN_TYPE:
	vhe->t = RPM_BIN_TYPE;
	vhe->p.ptr = he->p.ptr;
	vhe->c = he->c;
	vhe->ix = -1;
	break;
    }

/*@-compmempass@*/	/* vhe->p.ui64p is stack, not owned */
    if (tag->fmtfuncs) {
	char * nval = NULL;
	int i;
	for (i = 0; tag->av[i] != NULL; i++) {
	    headerTagFormatFunction fmt;
	    ARGV_t av;
	    if ((fmt = tag->fmtfuncs[i]) == NULL)
		continue;
	    /* If !1st formatter, and transformer, not extractor, save val. */
	    if (val != NULL && *tag->av[i] == '|') {
		int ix = vhe->ix;
		vhe = rpmheClean(vhe);
		vhe->tag = he->tag;
		vhe->t = RPM_STRING_TYPE;
		vhe->p.str = xstrdup(val);
		vhe->c = he->c;
		vhe->ix = ix;
		vhe->freeData = 1;
	    }
	    av = NULL;
	    if (tag->params && tag->params[i] && *tag->params[i] != '\0')
		xx = argvSplit(&av, tag->params[i], ",");

	    nval = fmt(vhe, av);

/*@-castfcnptr -modfilesys@*/
if (_hdrqf_debug)
fprintf(stderr, "\t%s(%s) %p(%p,%p) |%s|\n", tag->av[i], (tag->params ? tag->params[i] : NULL), (void *)fmt, (void *)vhe, (void *)(av ? av : NULL), (nval ? nval : "(null)"));
/*@=castfcnptr =modfilesys@*/

	    /* Accumulate (by appending) next formatter's return string. */
	    if (val == NULL)
		val = xstrdup((nval ? nval : ""));
	    else {
		char * oval = val;
		/* XXX using ... | ... as separator is feeble. */
		val = rpmExpand(val, (*val != '\0' ? " | " : ""), nval, NULL);
		oval = _free(oval);
	    }
	    nval = _free(nval);
	    av = argvFree(av);
	}
    }

    if (val == NULL)
	val = intFormat(vhe, NULL, NULL);
/*@=compmempass@*/
assert(val != NULL);
    if (val)
	need = strlen(val) + 1;

exit:
    if (val && need > 0) {
	if (tag->format && *tag->format && tag->pad > 0) {
	    size_t nb;
	    nb = strlen(tag->format) + sizeof("%s");
	    t = alloca(nb);
	    (void) stpcpy( stpcpy( stpcpy(t, "%"), tag->format), "s");
	    nb = tag->pad + strlen(val) + 1;
	    te = xmalloc(nb);
/*@-formatconst@*/
	    (void) snprintf(te, nb, t, val);
/*@=formatconst@*/
	    te[nb-1] = '\0';
	    val = _free(val);
	    val = te;
	    need += tag->pad;
	}
	t = hsaReserve(hsa, need);
	te = stpcpy(t, val);
	hsa->vallen += (te - t);
	val = _free(val);
    }

    return (hsa->val + hsa->vallen);
}

/**
 * Format a single headerSprintf item.
 * @param hsa		headerSprintf args
 * @param token		item to format
 * @param element	element index
 * @return		end of formatted string (NULL on error)
 */
/*@observer@*/ /*@null@*/
static char * singleSprintf(headerSprintfArgs hsa, sprintfToken token,
		size_t element)
	/*@globals rpmGlobalMacroContext, h_errno, internalState @*/
	/*@modifies hsa, token, rpmGlobalMacroContext, internalState @*/
{
    char * t, * te;
    size_t i, j;
    size_t numElements;
    sprintfToken spft;
    sprintfTag tag = NULL;
    HE_t he = NULL;
    size_t condNumFormats;
    size_t need;
    int xx;

    /* we assume the token and header have been validated already! */

    switch (token->type) {
    case PTOK_NONE:
	break;

    case PTOK_STRING:
	need = token->u.string.len;
	if (need == 0) break;
	t = hsaReserve(hsa, need);
	te = stpcpy(t, token->u.string.string);
	hsa->vallen += (te - t);
	break;

    case PTOK_TAG:
	t = hsa->val + hsa->vallen;
/*@-modobserver@*/	/* headerCompoundFormats not modified. */
	te = formatValue(hsa, &token->u.tag,
			(token->u.tag.justOne ? 0 : element));
/*@=modobserver@*/
	if (te == NULL)
	    return NULL;
	break;

    case PTOK_COND:
	if (token->u.cond.tag.ext
	 || headerIsEntry(hsa->h, token->u.cond.tag.tagno[0]))
	{
	    spft = token->u.cond.ifFormat;
	    condNumFormats = token->u.cond.numIfTokens;
	} else {
	    spft = token->u.cond.elseFormat;
	    condNumFormats = token->u.cond.numElseTokens;
	}

	need = condNumFormats * 20;
	if (spft == NULL || need == 0) break;

	t = hsaReserve(hsa, need);
	for (i = 0; i < condNumFormats; i++, spft++) {
/*@-modobserver@*/	/* headerCompoundFormats not modified. */
	    te = singleSprintf(hsa, spft, element);
/*@=modobserver@*/
	    if (te == NULL)
		return NULL;
	}
	break;

    case PTOK_ARRAY:
	numElements = 0;
	spft = token->u.array.format;
	for (i = 0; i < token->u.array.numTokens; i++, spft++)
	{
	    tag = &spft->u.tag;
	    if (spft->type != PTOK_TAG || tag->arrayCount || tag->justOne)
		continue;
	    he = &tag->he;
	    if (!he->avail) {
		he->tag = tag->tagno[0];
		if (tag->ext)
		    xx = getExtension(hsa, tag->ext, he, hsa->ec + tag->extNum);
		else
		    xx = headerGet(hsa->h, he, 0);
		if (!xx) {
		    (void) rpmheClean(he);
		    continue;
		}
		he->avail = 1;
	    }

	    /* Check iteration arrays are same dimension (or scalar). */
	    switch (he->t) {
	    default:
		if (numElements == 0) {
		    numElements = he->c;
		    /*@switchbreak@*/ break;
		}
		if ((size_t)he->c == numElements)
		    /*@switchbreak@*/ break;
		hsa->errmsg =
			_("array iterator used with different sized arrays");
		he = rpmheClean(he);
		return NULL;
		/*@notreached@*/ /*@switchbreak@*/ break;
	    case RPM_BIN_TYPE:
	    case RPM_STRING_TYPE:
		if (numElements == 0)
		    numElements = 1;
		/*@switchbreak@*/ break;
	    }
	}
	spft = token->u.array.format;

	if (numElements == 0) {
#ifdef	DYING	/* XXX lots of pugly "(none)" lines with --conflicts. */
	    need = sizeof("(none)\n") - 1;
	    t = hsaReserve(hsa, need);
	    te = stpcpy(t, "(none)\n");
	    hsa->vallen += (te - t);
#endif
	} else {
	    rpmTagReturnType tagT = 0;
	    const char * tagN = NULL;
	    spew_t spew = NULL;

	    need = numElements * token->u.array.numTokens;
	    if (need == 0) break;

	    tag = &spft->u.tag;

spew = NULL;
	    /* XXX Ick: +1 needed to handle :extractor |transformer marking. */
	    if (spft->type == PTOK_TAG && tag->av != NULL
	     && tag->av[0] != NULL && !strcmp(tag->av[0]+1, "xml"))
		spew = &_xml_spew;
	    if (spft->type == PTOK_TAG && tag->av != NULL
	     && tag->av[0] != NULL && !strcmp(tag->av[0]+1, "yaml"))
		spew = &_yaml_spew;
	    if (spft->type == PTOK_TAG && tag->av != NULL
	     && tag->av[0] != NULL && !strcmp(tag->av[0]+1, "json"))
		spew = &_json_spew;

	    if (spew == &_xml_spew) {
assert(tag->tagno != NULL);
		/* XXX display "Tag_0x01234567" for arbitrary tags. */
		if (tag->tagno[0] & 0x40000000) {
		    tagN = myTagName(hsa->tags, tag->tagno[0], NULL);
		} else
		    tagN = myTagName(hsa->tags, tag->tagno[0], NULL);
		need = sizeof("  <rpmTag name=\"\">\n") + strlen(tagN);
		te = t = hsaReserve(hsa, need);
		te = stpcpy( stpcpy( stpcpy(te, "  <rpmTag name=\""), tagN), "\">\n");
		hsa->vallen += (te - t);
	    }
	    if (spew == &_yaml_spew) {
assert(tag->tagno != NULL);
		/* XXX display "Tag_0x01234567" for arbitrary tags. */
		if (tag->tagno[0] & 0x40000000) {
		    tagN = myTagName(hsa->tags, tag->tagno[0], NULL);
		    tagT = numElements > 1
			?  RPM_ARRAY_RETURN_TYPE : RPM_SCALAR_RETURN_TYPE;
		} else
		    tagN = myTagName(hsa->tags, tag->tagno[0], &tagT);
		need = sizeof("  :     - ") + strlen(tagN);
		te = t = hsaReserve(hsa, need);
		*te++ = ' ';
		*te++ = ' ';
		te = stpcpy(te, tagN);
		*te++ = ':';
		*te++ = (((tagT & RPM_MASK_RETURN_TYPE) == RPM_ARRAY_RETURN_TYPE)
			? '\n' : ' ');
		*te = '\0';
		hsa->vallen += (te - t);
	    }
	    if (spew == &_json_spew) {
assert(tag->tagno != NULL);
		/* XXX display "Tag_0x01234567" for arbitrary tags. */
		if (tag->tagno[0] & 0x40000000) {
		    tagN = myTagName(hsa->tags, tag->tagno[0], NULL);
		    tagT = numElements > 1
			?  RPM_ARRAY_RETURN_TYPE : RPM_SCALAR_RETURN_TYPE;
		} else
		if (tag->tagno[0] == RPMTAG_HDRID) {	/* RPMTAG_SHA1HEADER */
		    tagN = "_id";	/* XXX mongo primary key name */
		} else
		    tagN = myTagName(hsa->tags, tag->tagno[0], &tagT);
		need = sizeof("  : [ ") + strlen(tagN);
		te = t = hsaReserve(hsa, need);
		te = stpcpy( stpcpy( stpcpy(te, "  "), tagN), ": ");
		if ((tagT & RPM_MASK_RETURN_TYPE) == RPM_ARRAY_RETURN_TYPE)
		    te = stpcpy(te, "[ ");
		hsa->vallen += (te - t);
	    }

	    need = numElements * token->u.array.numTokens * 10;
	    t = hsaReserve(hsa, need);
	    for (j = 0; j < numElements; j++) {
		spft = token->u.array.format;
		for (i = 0; i < token->u.array.numTokens; i++, spft++) {
/*@-modobserver@*/	/* headerCompoundFormats not modified. */
		    te = singleSprintf(hsa, spft, j);
/*@=modobserver@*/
		    if (te == NULL)
			return NULL;
		}
	    }

	    if (spew == &_xml_spew) {
		need = sizeof("  </rpmTag>\n") - 1;
		te = t = hsaReserve(hsa, need);
		te = stpcpy(te, "  </rpmTag>\n");
		hsa->vallen += (te - t);
	    }
	    if (spew == &_json_spew) {
		if ((tagT & RPM_MASK_RETURN_TYPE) == RPM_ARRAY_RETURN_TYPE) {
		    need = sizeof(" ],\n") - 1;
		    te = t = hsaReserve(hsa, need);
		    te = stpcpy(te, " ],\n");
		    hsa->vallen += (te - t);
		} else {
		    need = sizeof("\n") - 1;
		    te = t = hsaReserve(hsa, need);
		    te = stpcpy(te, "\n");
		    hsa->vallen += (te - t);
		}
	    }

	}
	break;
    }

    return (hsa->val + hsa->vallen);
}

/**
 * Create an extension cache.
 * @param exts		headerSprintf extensions
 * @retval *necp	no. of elements (or NULL)
 * @return		new extension cache
 */
static /*@only@*/ HE_t
rpmecNew(const headerSprintfExtension exts, /*@null@*/ int * necp)
	/*@modifies *necp @*/
{
    headerSprintfExtension ext;
    HE_t ec;
    int extNum = 0;

    if (exts != NULL)
    for (ext = exts, extNum = 0; ext != NULL && ext->type != HEADER_EXT_LAST;
	ext = (ext->type == HEADER_EXT_MORE ? *ext->u.more : ext+1), extNum++)
    {
	;
    }
    if (necp)
	*necp = extNum;
    ec = (HE_t) xcalloc(extNum+1, sizeof(*ec));	/* XXX +1 unnecessary */
    return ec;
}

/**
 * Destroy an extension cache.
 * @param exts		headerSprintf extensions
 * @param ec		extension cache
 * @return		NULL always
 */
static /*@null@*/ HE_t
rpmecFree(const headerSprintfExtension exts, /*@only@*/ HE_t ec)
	/*@modifies ec @*/
{
    headerSprintfExtension ext;
    int extNum;

    for (ext = exts, extNum = 0; ext != NULL && ext->type != HEADER_EXT_LAST;
	ext = (ext->type == HEADER_EXT_MORE ? *ext->u.more : ext+1), extNum++)
    {
	(void) rpmheClean(&ec[extNum]);
    }

    ec = _free(ec);
    return NULL;
}

char * headerSprintf(Header h, const char * fmt,
		headerTagTableEntry tags,
		headerSprintfExtension exts,
		errmsg_t * errmsg)
{
    headerSprintfArgs hsa = (headerSprintfArgs) memset(alloca(sizeof(*hsa)), 0, sizeof(*hsa));
    sprintfToken nextfmt;
    sprintfTag tag;
    char * t, * te;
    size_t need;
spew_t spew = NULL;

/*@-modfilesys@*/
if (_hdrqf_debug)
fprintf(stderr, "==> headerSprintf(%p, \"%s\", %p, %p, %p)\n", h, fmt, tags, exts, errmsg);
/*@=modfilesys@*/

    /* Set some reasonable defaults */
    if (tags == NULL)
	tags = rpmTagTable;
    /* XXX this loses the extensions in lib/formats.c. */
    if (exts == NULL)
	exts = headerCompoundFormats;
 
/*@-assignexpose -castexpose @*/
    hsa->h = headerLink(h);
/*@=assignexpose =castexpose @*/
    hsa->fmt = xstrdup(fmt);
/*@-assignexpose -dependenttrans@*/
    hsa->exts = exts;
    hsa->tags = tags;
/*@=assignexpose =dependenttrans@*/
    hsa->errmsg = NULL;

    if (parseFormat(hsa, hsa->fmt, &hsa->format, &hsa->numTokens, NULL, PARSER_BEGIN))
	goto exit;

    hsa->nec = 0;
    hsa->ec = rpmecNew(hsa->exts, &hsa->nec);
    hsa->val = xstrdup("");

    tag =
	(hsa->format->type == PTOK_TAG
	    ? &hsa->format->u.tag :
	(hsa->format->type == PTOK_ARRAY
	    ? &hsa->format->u.array.format->u.tag :
	NULL));

spew = NULL;
    /* XXX Ick: +1 needed to handle :extractor |transformer marking. */
    if (tag != NULL && tag->tagno != NULL && tag->tagno[0] == (rpmTag)-2
     && tag->av != NULL && tag->av[0] != NULL && !strcmp(tag->av[0]+1, "xml"))
	spew = &_xml_spew;
    if (tag != NULL && tag->tagno != NULL && tag->tagno[0] == (rpmTag)-2
     && tag->av != NULL && tag->av[0] != NULL && !strcmp(tag->av[0]+1, "yaml"))
	spew = &_yaml_spew;
    if (tag != NULL && tag->tagno != NULL && tag->tagno[0] == (rpmTag)-2
     && tag->av != NULL && tag->av[0] != NULL && !strcmp(tag->av[0]+1, "json"))
	spew = &_json_spew;

    if (spew && spew->spew_init && spew->spew_init[0]) {
	char * spew_init = rpmExpand(spew->spew_init, NULL);
	need = strlen(spew_init);
	t = hsaReserve(hsa, need);
	te = stpcpy(t, spew_init);
	hsa->vallen += (te - t);
	spew_init = _free(spew_init);
    }

    hsa = hsaInit(hsa);
    while ((nextfmt = hsaNext(hsa)) != NULL) {
/*@-globs -mods@*/	/* XXX rpmGlobalMacroContext @*/
	te = singleSprintf(hsa, nextfmt, 0);
/*@=globs =mods @*/
	if (te == NULL) {
	    hsa->val = _free(hsa->val);
	    break;
	}
    }
    hsa = hsaFini(hsa);

    if (spew && spew->spew_fini && spew->spew_fini[0]) {
	char * spew_fini = rpmExpand(spew->spew_fini, NULL);
	need = strlen(spew_fini);
	t = hsaReserve(hsa, need);
	te = stpcpy(t, spew_fini);
	hsa->vallen += (te - t);
	spew_fini = _free(spew_fini);
    }

    if (hsa->val != NULL && hsa->vallen < hsa->alloced)
	hsa->val = (char *) xrealloc(hsa->val, hsa->vallen+1);	

    hsa->ec = rpmecFree(hsa->exts, hsa->ec);
    hsa->nec = 0;
    hsa->format = freeFormat(hsa->format, hsa->numTokens);

exit:
/*@-dependenttrans -observertrans @*/
    if (errmsg)
	*errmsg = hsa->errmsg;
/*@=dependenttrans =observertrans @*/
    (void)headerFree(hsa->h);
    hsa->h = NULL;
    hsa->fmt = _free(hsa->fmt);
/*@-retexpose@*/
    return hsa->val;
/*@=retexpose@*/
}
