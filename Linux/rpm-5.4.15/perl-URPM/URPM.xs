/* Copyright (c) 2002, 2003, 2004, 2005 MandrakeSoft SA
 * Copyright (c) 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012 Mandriva SA
 *
 * All rights reserved.
 * This program is free software; you can redistribute it and/or
 * modify it under the same terms as Perl itself.
 *
 * $Id: URPM.xs 274527 2012-06-09 14:11:55Z peroyvind $
 * 
 */
#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"

#include <sys/utsname.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
#include <libintl.h>
#include <glob.h>
#include <stdbool.h>
#include <magic.h>

#undef Fflush
#undef Mkdir
#undef Stat
#undef Fstat

#define _RPMGI_INTERNAL
#define _RPMEVR_INTERNAL
#define _RPMPS_INTERNAL
#define _RPMDB_INTERNAL
#define _RPMTAG_INTERNAL
#define WITH_DB

#define	_RPMVSF_NODIGESTS	\
  ( RPMVSF_NOSHA1HEADER |	\
    RPMVSF_NOMD5HEADER |	\
    RPMVSF_NOSHA1 |		\
    RPMVSF_NOMD5 )

#define	_RPMVSF_NOSIGNATURES	\
  ( RPMVSF_NODSAHEADER |	\
    RPMVSF_NORSAHEADER |	\
    RPMVSF_NODSA |		\
    RPMVSF_NORSA )

#define	_RPMVSF_NOHEADER	\
  ( RPMVSF_NOSHA1HEADER |	\
    RPMVSF_NOMD5HEADER |	\
    RPMVSF_NODSAHEADER |	\
    RPMVSF_NORSAHEADER )

#define	_RPMVSF_NOPAYLOAD	\
  ( RPMVSF_NOSHA1 |		\
    RPMVSF_NOMD5 |		\
    RPMVSF_NODSA |		\
    RPMVSF_NORSA )

// rpmgi.h includes fts.h (incompatible with _FILE_OFFSET_BITS=64)
// but only so it can add FTS* and FTSENT* members to structures.
// Given we never dereference those structures, this is ugly but
// safe:
#define _FTS_H 1
typedef void FTS;
typedef void FTSENT;

#include <rpmio.h>
#include <rpmtag.h>
#include <rpmdb.h>
#include <pkgio.h>
#include <rpmcb.h>
#include <rpmte.h>
#include <rpmps.h>
#include <rpmbuild.h>
#include <rpmgi.h>
#include <rpmlog.h>
#include <rpmconstant.h>

struct s_Package {
  char *info;
  int  filesize;
  char *requires;
  char *suggests;
  char *obsoletes;
  char *conflicts;
  char *provides;
  char *rflags;
  char *summary;
  unsigned flag;
  Header h;
};

struct s_Transaction {
  rpmts ts;
  int count;
};

struct s_TransactionData {
  SV* callback_open;
  SV* callback_close;
  SV* callback_trans;
  SV* callback_uninst;
  SV* callback_inst;
  long min_delta;
  SV *data; /* chain with another data user provided */
};

typedef struct s_Transaction* URPM__DB;
typedef struct s_Transaction* URPM__Transaction;
typedef struct s_Package* URPM__Package;

#define FLAG_ID               0x001fffffU
#define FLAG_RATE             0x00e00000U
#define FLAG_BASE             0x01000000U
#define FLAG_SKIP             0x02000000U
#define FLAG_DISABLE_OBSOLETE 0x04000000U
#define FLAG_INSTALLED        0x08000000U
#define FLAG_REQUESTED        0x10000000U
#define FLAG_REQUIRED         0x20000000U
#define FLAG_UPGRADE          0x40000000U
#define FLAG_NO_HEADER_FREE   0x80000000U

#define FLAG_ID_MAX           0x001ffffe
#define FLAG_ID_INVALID       0x001fffff

#define FLAG_RATE_POS         21
#define FLAG_RATE_MAX         5
#define FLAG_RATE_INVALID     0


#define FILTER_MODE_ALL_FILES     0
#define FILTER_MODE_CONF_FILES    2

#if BYTE_ORDER == LITTLE_ENDIAN
#define bswap32(x) htobe32(x)
#elif __BYTE_ORDER == BIG_ENDIAN
#define bswap32(x) htole32(x)
#endif

static ssize_t
write_nocheck(int fd, const void *buf, size_t count) {
  return write(fd, buf, count);
}

static inline void
unused_variable(__attribute__((unused)) const void *p) {
}

static int rpmError_callback_data;

static int
rpmError_callback() {
  write_nocheck(rpmError_callback_data, rpmlogMessage(), strlen(rpmlogMessage()));
  return RPMLOG_DEFAULT;
}

static bool rpm_codeset_is_utf8 = false;

static struct s_backup {
    char *ptr;
    char chr;
} char_backups[32];

static int BI = 0;

static void
backup_char(char *c) {
      char_backups[BI].chr = *c,
      *(char_backups[BI++].ptr = &(*c)) = 0; /* mark end of string to enable searching backwards */
}

static void
restore_chars() {
    for(; BI > 0; char_backups[BI].ptr = NULL)
	BI--, *char_backups[BI].ptr = char_backups[BI].chr;
}

static SV*
newSVpv_utf8(const char *s, STRLEN len)
{
  SV *sv = newSVpv(s, len);
  SvUTF8_on(sv);
  return sv;
}

/* XXX: RPMTAG_NVRA doesn't have disttag & distepoch */
#if 0
static const char *
get_nvra(const Header header) {
  HE_t val = (HE_t)memset(alloca(sizeof(*val)), 0, sizeof(*val));

  val->tag = RPMTAG_NVRA;
  if(headerGet(header, val, 0))
    return val->p.str;
  return "";
}

#else

/* Since the NVRA format won't change, we'll store it in a global variable so
 * that we only have to expand the macro once.
 */
static const char *nvra_fmt = NULL;

static const char *
get_nvra_fmt() {
  if(!nvra_fmt) {
    char *qfmt = rpmExpand("%{?___NVRA:%___NVRA}%{?!___NVRA:/%_build_name_fmt}", NULL);
    /* On older rpm versions '%___NVRA' isn't defined, so then we'll have to create
     * it from the '%_build_name_fmt'
     */
    if(qfmt[0] == '/') {
      char *tmp;
      const char macroName[] = "___NVRA";
      if(strcasecmp(tmp = qfmt+strlen(qfmt)-4, ".rpm") == 0)   
	*tmp = '\0';
      tmp = qfmt;
      /* As %{ARCH} will be incorrect with source rpms, we replace it with a
       * conditional expression so that we get '.src.rpm' for source rpms.
       * This we'll do in a uhm.. "creative" way replacing '%{ARCH}' with '%{XXXX}',
       * which is a macro we'll define for the conditional expression,
       * when expanded it will return the format macro with the conditional
       * expression.
       */
      while((size_t)(tmp = strcasestr(tmp, "%{ARCH}")+2) != 2)while(*tmp != '}')
	*tmp++ = 'X';

      rpmDefineMacro(NULL, "XXXX %%|ARCH?{%%|SOURCERPM?{%%{ARCH}}:{src}|}:{}|", RMIL_DEFAULT);
      tmp = rpmExpand((tmp = strrchr(qfmt, '/')) ? tmp+1 : qfmt, NULL);

      qfmt = realloc(qfmt, strlen(tmp) + sizeof(macroName)+1);
      sprintf(qfmt, "%s %s", macroName, tmp);
      rpmDefineMacro(NULL, qfmt, RMIL_DEFAULT);
      sprintf(qfmt, "%s", qfmt+sizeof(macroName));
      _free(tmp);
    }
    nvra_fmt = qfmt;
  }
  return nvra_fmt;
}

static const char *
get_nvra(const Header h) {
  const char *qfmt = get_nvra_fmt();
  const char *NVRA = headerSprintf(h, qfmt, NULL, NULL, NULL);
  return NVRA;
}

#endif

static int
do_rpmEVRcompare(const char *a, const char *b) {
  int compare;

  EVR_t lEVR = rpmEVRnew(RPMSENSE_EQUAL, 0),
        rEVR = rpmEVRnew(RPMSENSE_EQUAL, 0);
  rpmEVRparse(a, lEVR);
  rpmEVRparse(b, rEVR);
  compare = rpmEVRcompare(lEVR, rEVR);
  lEVR = rpmEVRfree(lEVR),
  rEVR = rpmEVRfree(rEVR);
  return compare;
}

static rpmTag
rpmtag_from_string(const char *tag)
{
  static rpmconst tag_c = NULL,
		  qv_c = NULL;
  static const char tag_context[] = "rpmtag",
		    qv_context[] = "rpmqvsources";

  if(tag_c == NULL) {
    tag_c = rpmconstNew();
    if(!rpmconstInitToContext(tag_c, tag_context))
      croak("unknown context [%s]", tag_context);
  }
  if(rpmconstFindName(tag_c, tag, 0))
    return rpmconstValue(tag_c);
  if(qv_c == NULL) {
    qv_c = rpmconstNew();
    if(!rpmconstInitToContext(qv_c, qv_context))
      croak("unknown context [%s]", qv_context);
  }
  if(rpmconstFindName(qv_c, tag, 0))
    return rpmconstValue(qv_c);
  croak("unknown tag [%s]", tag);
}

static const char *
get_name(const Header header, rpmTag tag) {
  HE_t val = (HE_t)memset(alloca(sizeof(*val)), 0, sizeof(*val));

  val->tag = tag;
  if(headerGet(header, val, 0)) {
    if (val->t == RPM_STRING_TYPE)
      return val->p.str;
    else if(val->t == RPM_STRING_ARRAY_TYPE || val->t == RPM_I18NSTRING_TYPE)
      return val->p.argv[val->ix];
  }
  return NULL;
}

static int
get_int(const Header header, rpmTag tag) {
  HE_t val = (HE_t)memset(alloca(sizeof(*val)), 0, sizeof(*val));
  int ret = 0;

  val->tag = tag;
  if(headerGet(header, val, 0)) {
    ret = (val->t == RPM_UINT32_TYPE) ? val->p.ui32p[val->ix >= 0 ? val->ix : 0] : 0;
    val->p.ui32p = _free(val->p.ui32p);
  }
  return ret;
}

#define push_utf8_name(pkg, tag) { \
    const char *str = get_name(pkg->h, tag); \
    mXPUSHs((str && *str) ? newSVpv_utf8(str, 0) : newSVpvs("")); \
    _free(str);\
}

#define push_name(pkg, tag) {\
  const char *str = get_name(pkg->h, tag); \
  mXPUSHs((str && *str) ? newSVpv(str, 0) : newSVpvs("")); \
  _free(str); \
}

#define push_utf8_name_only(str, len) mXPUSHs((str && *str) ? newSVpv_utf8(str, len) : newSVpvs(""))
#define push_name_only(str, len) mXPUSHs((str && *str) ? newSVpv(str, len) : newSVpvs(""))
/* This function might modify strings that needs to be restored after use
 * with restore_chars()
 */
static void
get_fullname_parts(const URPM__Package pkg, char **name, int *epoch, char **version, char **release, char **disttag, char **distepoch, char **arch, char **eos) {
  char *_version = NULL, *_release = NULL, *_disttag = NULL, *_distepoch = NULL, *_arch = NULL, *_eos = NULL, *tmp = NULL;

  /* XXX: Could probably be written in a more generic way, only thing we
   * really want to do is to check for arch field, which will be missing in
   * the case of gpg-pubkey at least..
   */
  int pubkey;

  if ((_eos = strchr(pkg->info, '@')) != NULL) {
    if (epoch != NULL) *epoch = isdigit(_eos[1]) ? atoi(_eos+1) : 0;
    if (name != NULL || version != NULL || release != NULL || disttag != NULL || distepoch != NULL || arch != NULL) {
      backup_char(_eos++);
      if (eos != NULL) *eos = _eos;
      if ((pubkey = !strncmp(pkg->info, "gpg-pubkey", 10)) || (_arch = strrchr(pkg->info, '.')) != NULL) {
	if (!pubkey)
	  backup_char(_arch++);
	if (arch != NULL) *arch = pubkey ? "" : _arch;
	if (distepoch != NULL || disttag != NULL || release != NULL || version != NULL || name != NULL) {
	  _disttag = _eos;
	  for (int i = 0; i < 3 && _disttag; i++, _disttag)
	    _disttag = strchr(++_disttag, '@');
	  if (_disttag && (_distepoch = strchr(++_disttag, '@')))
	      backup_char(_distepoch++);
	  /* currently not very useful as there's no additional fields, but we'll do this check
	   * so that adding any potential fields in the future shouldn't break anything
	   */
	  if (_distepoch != NULL && (tmp = strchr(_distepoch, '@')))
	    backup_char(tmp);
	  /* eliminate disttag from fullname so the parsing won't get messed up */
	  if (_disttag != NULL && *_disttag && (tmp = strrchr(pkg->info, '-')) && !strncmp(tmp+1, _disttag, strlen(_disttag)))
		backup_char(tmp);
	  if (distepoch != NULL) *distepoch = _distepoch ? _distepoch : "";
	  if (disttag != NULL || release != NULL || version != NULL || name != NULL) {
	    if (disttag != NULL) *disttag = _disttag ? _disttag : "";
	    if ((release != NULL || version != NULL || name != NULL) && (_release = strrchr(pkg->info, '-')) != NULL) {
	      backup_char(_release++);
	      if (release != NULL) *release = _release;			  
	      if ((version != NULL || name != NULL) && (_version = strrchr(pkg->info, '-')) != NULL) {
		backup_char(_version++);
		if (version != NULL) *version = _version;
		if (name != NULL) *name = pkg->info;
	      }
	    }
	  }
	}
      }
    }
  }
}

static size_t
get_filesize(const Header h) {
  return get_int(h, RPMTAG_SIGSIZE) + 440; /* 440 is the rpm header size (?) empirical, but works */
}

static int
print_list_entry(char *buff, int sz, const char *name, rpmsenseFlags flags, const char *evr) {
  int len = strlen(name);
  char *p = buff;

  if (flags & RPMSENSE_RPMLIB) return -1;
  memcpy(p, name, len); p += len;

  /* XXX: RPMSENSE_PREREQ obsolete, remove? */
  if (flags & (RPMSENSE_PREREQ|RPMSENSE_SCRIPT_PREUN|RPMSENSE_SCRIPT_PRE|RPMSENSE_SCRIPT_POSTUN|RPMSENSE_SCRIPT_POST)) {
    if (p - buff + 3 >= sz) return -1;
    memcpy(p, "[*]", 4); p += 3;
  }
  if (evr != NULL) {
    len = strlen(evr);
    if (len > 0) {
      if (p - buff + 6 + len >= sz) return -1;
      static const char *Fstr[] = { "?0","<",">","?3","==","<=",">=","?7" };
      uint32_t Fx = ((flags >> 1) & 0x7);
      *p++ = '[';
      p = stpcpy( stpcpy( stpcpy(p, Fstr[Fx]), " "), evr);
      *p++ = ']';
    }
  }
  *p = 0; /* make sure to mark null char, Is it really necessary ? */

  return p - buff;
}

static int
ranges_overlap(rpmsenseFlags aflags, char *sa, rpmsenseFlags bflags, char *sb) {
  if (!aflags || !bflags)
    return 1; /* really faster to test it there instead of later */
  else {
    char *eosa = strchr(sa, ']');
    char *eosb = strchr(sb, ']');
    EVR_t lEVR = rpmEVRnew(aflags, 0),
          rEVR = rpmEVRnew(bflags, 0);
    int result;

    if(eosa) backup_char(eosa);
    if(eosb) backup_char(eosb);
    rpmEVRparse(sa, lEVR);
    rpmEVRparse(sb, rEVR);
    /* TODO: upstream bug? should rpmEVRparse really reset Flags? */
    lEVR->Flags = aflags;
    rEVR->Flags = bflags;
    result = rpmEVRoverlap(lEVR, rEVR);
    rpmEVRfree(lEVR);
    rpmEVRfree(rEVR);
    restore_chars();

    return result;
  }
}

static int has_old_suggests;
rpmsenseFlags is_old_suggests(rpmsenseFlags flags) { 
  rpmsenseFlags is = flags & RPMSENSE_MISSINGOK;
  if (is) has_old_suggests = is;
  return is;
}
rpmsenseFlags is_not_old_suggests(rpmsenseFlags flags) {
  return !is_old_suggests(flags);
}

typedef int (*callback_list_str)(char *s, int slen, const char *name, const rpmsenseFlags flags, const char *evr, void *param);

static int
callback_list_str_xpush(char *s, int slen, const char *name, rpmsenseFlags flags, const char *evr, __attribute__((unused)) void *param) {
  dSP;
  if (s)
    push_name_only(s, slen);
  else {
    char buff[BUFSIZ];
    /* to silence warnings about never being NULL */
    char *buf = buff;
    int len = print_list_entry(buf, sizeof(buff)-1, name, flags, evr);
    if (len >= 0)
      push_name_only(buf, len);
  }
  PUTBACK;
  /* returning zero indicates to continue processing */
  return 0;
}
static int
callback_list_str_xpush_requires(char *s, int slen, const char *name, const rpmsenseFlags flags, const char *evr, __attribute__((unused)) void *param) {
  dSP;
  if (s)
    push_name_only(s, slen);
  else if (is_not_old_suggests(flags)) {
    char buff[BUFSIZ];
    char *buf = buff;
    int len = print_list_entry(buf, sizeof(buff)-1, name, flags, evr);
    if (len >= 0)
      push_name_only(buf, len);
  }
  PUTBACK;
  /* returning zero indicates to continue processing */
  return 0;
}
static int
callback_list_str_xpush_old_suggests(char *s, int slen, const char *name, rpmsenseFlags flags, const char *evr, __attribute__((unused)) void *param) {
  dSP;
  if (s)
    push_name_only(s, slen);
  else if (is_old_suggests(flags)) {
    char buff[BUFSIZ];
    char *buf = buff;
    int len = print_list_entry(buff, sizeof(buff)-1, name, flags, evr);
    if (len >= 0)
      push_name_only(buf, len);
  }
  PUTBACK;
  /* returning zero indicates to continue processing */
  return 0;
}

struct cb_overlap_s {
  char *name;
  rpmsenseFlags flags;
  char *evr;
  int direction; /* indicate to compare the above at left or right to the iteration element */
};

static int
callback_list_str_overlap(char *s, int slen, const char *name, rpmsenseFlags flags, const char *evr, void *param) {
  struct cb_overlap_s *os = (struct cb_overlap_s *)param;
  int result = 0;
  char *eos = NULL;
  char *eon = NULL;
  char eosc = '\0';
  char eonc = '\0';

  /* we need to extract name, flags and evr from a full sense information, store result in local copy */
  if (s) {
    if (slen) { eos = s + slen; eosc = *eos; *eos = 0; }
    name = s;
    while (*s && *s != ' ' && *s != '[' && *s != '<' && *s != '>' && *s != '=') ++s;
    if (*s) {
      eon = s;
      while (*s) {
	if (*s == ' ' || *s == '[' || *s == '*' || *s == ']');
	else if (*s == '<') flags |= RPMSENSE_LESS;
	else if (*s == '>') flags |= RPMSENSE_GREATER;
	else if (*s == '=') flags |= RPMSENSE_EQUAL;
	else break;
	++s;
      }
      evr = s;
    } else
      evr = "";
  }

  /* mark end of name */
  if (eon) { eonc = *eon; *eon = 0; }
  /* names should be equal, else it will not overlap */
  if (!strcmp(name, os->name)) {
    /* perform overlap according to direction needed, negative for left */
    if (os->direction < 0)
      result = ranges_overlap(os->flags, os->evr, flags, (char *) evr);
    else
      result = ranges_overlap(flags, (char *) evr, os->flags, os->evr);
  }

  /* fprintf(stderr, "cb_list_str_overlap result=%d, os->direction=%d, os->name=%s, os->evr=%s, name=%s, evr=%s\n",
     result, os->direction, os->name, os->evr, name, evr); */

  /* restore s if needed */
  if (eon) *eon = eonc;
  if (eos) *eos = eosc;

  return result;
}

static int
return_list_str(char *s, const Header header, rpmTag tag_name, rpmTag tag_flags, rpmTag tag_version, callback_list_str f, void *param) {
  int count = 0;

  if (s != NULL) {
    char *ps = strchr(s, '@');
    if (tag_flags && tag_version) {
      while(ps != NULL) {
	++count;
	if (f(s, ps-s, NULL, 0, NULL, param)) return -count;
	s = ps + 1; ps = strchr(s, '@');
      }
      ++count;
      if (f(s, 0, NULL, 0, NULL, param)) return -count;
    } else {
      char *eos;
      while(ps != NULL) {
	*ps = 0; eos = strchr(s, '['); if (!eos) eos = strchr(s, ' ');
	++count;
	if (f(s, eos ? eos-s : ps-s, NULL, 0, NULL, param)) { *ps = '@'; return -count; }
	*ps = '@'; /* restore in memory modified char */
	s = ps + 1; ps = strchr(s, '@');
      }
      eos = strchr(s, '['); if (!eos) eos = strchr(s, ' ');
      ++count;
      if (f(s, eos ? eos-s : 0, NULL, 0, NULL, param)) return -count;
    }
  } else if (header) {
    HE_t he = memset(alloca(sizeof(*he)), 0, sizeof(*he));

    he->tag = tag_name;
    if (headerGet(header, he, 0)) {
      const char **list = he->p.argv;
      rpmsenseFlags *flags = NULL;
      const char **list_evr = NULL;
      int c = he->c;

      if (tag_flags) {
        he->tag = tag_flags;
        if (headerGet(header, he, 0))
	  flags = (rpmsenseFlags*)he->p.ui32p;
      }
      if (tag_version) {
        he->tag = tag_version;
        if (headerGet(header, he, 0))
	  list_evr = he->p.argv;
      }
      for (he->ix = 0; he->ix < c; he->ix++) {
	++count;
	if (f(NULL, 0, list[he->ix], flags ? flags[he->ix] : 0, 
	      list_evr ? list_evr[he->ix] : NULL,
	      param)) {
	  list = _free(list);
	  if (tag_flags) flags = _free(flags);
	  if (tag_version) list_evr = _free(list_evr);
	  return (int)-count;
	}
      }
      list = _free(list);
      if (tag_flags) flags = _free(flags);
      if (tag_version) list_evr = _free(list_evr);
    }
  }
  return count;
}

static int
xpush_simple_list_str(const Header header, rpmTag tag_name) {
  dSP;
  if (header) {
    HE_t he = memset(alloca(sizeof(*he)), 0, sizeof(*he));

    he->tag = tag_name;
    if (!headerGet(header, he, 0)) return 0;

    for (he->ix = 0; he->ix < (int)he->c; he->ix++)
      push_name_only(he->p.argv[he->ix], 0);
    he->p.ptr = _free(he->p.ptr);
    PUTBACK;
    return he->c;
  } else return 0;
}

static void
return_list_uint32_t(const Header header, rpmTag tag_name) {
  dSP;
  if (header) {
    HE_t he = memset(alloca(sizeof(*he)), 0, sizeof(*he));

    he->tag = tag_name;
    if (headerGet(header, he, 0)) {
      for (he->ix = 0; he->ix < (int)he->c; he->ix++)
	mXPUSHs(newSViv(he->p.ui32p[he->ix]));
      he->p.ptr = _free(he->p.ptr);
    }
  }
  PUTBACK;
}

static void
return_list_uint_16(const Header header, rpmTag tag_name) {
  dSP;
  if (header) {
    HE_t he = memset(alloca(sizeof(*he)), 0, sizeof(*he));

    he->tag = tag_name;
    if (headerGet(header, he, 0)) {
      for(he->ix = 0; he->ix < (int)he->c; he->ix++)
	XPUSHs(newSViv(he->p.ui16p[he->ix]));
      he->p.ptr = _free(he->p.ptr);
    }
  }
  PUTBACK;
}

static void
return_list_tag_modifier(const Header header, const char *tag_name) {
  dSP;
  HE_t he = memset(alloca(sizeof(*he)), 0, sizeof(*he));
  rpmTag tag = isdigit(*tag_name) ? (rpmTag)atoi(tag_name) : rpmtag_from_string(tag_name);

  he->tag = tag;
  if (!headerGet(header, he, 0)) return;

  for (he->ix = 0; he->ix < (int)he->c; he->ix++) {
    char buff[15];
    char *s = buff;
    char *buf = buff;
    rpmTagType tags = he->p.ui32p[he->ix];
    switch (tag) {
    case RPMTAG_FILEFLAGS:
      if (tags & RPMFILE_CONFIG)    *s++ = 'c';
      if (tags & RPMFILE_DOC)       *s++ = 'd';
      if (tags & RPMFILE_GHOST)     *s++ = 'g';
      if (tags & RPMFILE_LICENSE)   *s++ = 'l';
      if (tags & RPMFILE_MISSINGOK) *s++ = 'm';
      if (tags & RPMFILE_NOREPLACE) *s++ = 'n';
      if (tags & RPMFILE_SPECFILE)  *s++ = 'S';
      if (tags & RPMFILE_README)    *s++ = 'R';
      if (tags & RPMFILE_EXCLUDE)   *s++ = 'e';
      if (tags & RPMFILE_ICON)      *s++ = 'i';
      if (tags & RPMFILE_UNPATCHED) *s++ = 'u';
      if (tags & RPMFILE_PUBKEY)    *s++ = 'p';
    break;
    default:
      he->p.ptr = _free(he->p.ptr);
      return;  
    }
    *s = '\0';
    push_name_only(buf, strlen(buff));
  }
  he->p.ptr = _free(he->p.ptr);
  PUTBACK;
}

static void
return_list_tag(const URPM__Package pkg, const char *tag_name) {
  dSP;
  rpmTag tag = isdigit(*tag_name) ? (rpmTag)atoi(tag_name) : rpmtag_from_string(tag_name);

  if (pkg->h != NULL) {
    HE_t he = memset(alloca(sizeof(*he)), 0, sizeof(*he));

    he->tag = tag;
    if (!strcasecmp(tag_name, "nvra")) {
      const char *nvra = get_nvra(pkg->h);
      push_name_only(nvra, 0);
      _free(nvra);
    } else if (headerGet(pkg->h, he, 0)) {
      if (tag == RPMTAG_ARCH)
	push_name_only((headerIsEntry(pkg->h, RPMTAG_SOURCERPM) ? he->p.str : "src"), 0);
      else
	switch (he->t) {
	  case RPM_UINT8_TYPE:
	  case RPM_UINT16_TYPE:
	  case RPM_UINT32_TYPE:
	    for (he->ix=0; he->ix < (int)he->c; he->ix++)
	      mXPUSHs(newSViv(he->p.ui32p[he->ix]));
	    break;
	  case RPM_STRING_TYPE:
	    push_name_only(he->p.str, 0);
	    break;
	  case RPM_BIN_TYPE:
	    break;
	  case RPM_STRING_ARRAY_TYPE:
	    for (he->ix = 0; he->ix < (int)he->c; he->ix++)
	      push_name_only(he->p.argv[he->ix], 0);
	    break;
	  case RPM_I18NSTRING_TYPE:
	    break;
	  case RPM_UINT64_TYPE:
	    break;
	}
      he->p.ptr = _free(he->p.ptr);
    }
  } else {
    char *name;
    int epoch;
    char *version;
    char *release;
    char *disttag;
    char *distepoch;
    char *arch;

    switch (tag) {
      case RPMTAG_NAME:
	get_fullname_parts(pkg, &name, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
	if(!strlen(name))
	  croak("invalid fullname");
	push_name_only(name, 0);
	break;
      case RPMTAG_EPOCH:
	get_fullname_parts(pkg, NULL, &epoch, NULL, NULL, NULL, NULL, NULL, NULL);
	mXPUSHs(newSViv(epoch));
      case RPMTAG_VERSION:
	get_fullname_parts(pkg, NULL, NULL, &version, NULL, NULL, NULL, NULL, NULL);
	if(!strlen(version))
	  croak("invalid fullname");
	push_name_only(version, 0);
	break;
      case RPMTAG_RELEASE:
	get_fullname_parts(pkg, NULL, NULL, NULL, &release, NULL, NULL, NULL, NULL);
	if(!strlen(release))
	  croak("invalid fullname");
	push_name_only(release, 0);
	break;
      case RPMTAG_DISTTAG:
	get_fullname_parts(pkg, NULL, NULL, NULL, NULL, NULL, &disttag, NULL, NULL);
	push_name_only(disttag, 0);
	break;
      case RPMTAG_DISTEPOCH:
	get_fullname_parts(pkg, NULL, NULL, NULL, NULL, NULL, NULL, &distepoch, NULL);
	push_name_only(distepoch, 0);
	break;
      case RPMTAG_ARCH:
	get_fullname_parts(pkg, NULL, NULL, NULL, NULL, &arch, NULL, NULL, NULL);
	push_name_only(arch, 0);
	break;
      case RPMTAG_SUMMARY:
	push_name_only(pkg->summary, 0);
	break;
      /* fix to match %{___NVRA} later... */
      case RPMTAG_NVRA:
	{
	  const char *eon = strchr(pkg->info, '@');
	  push_name_only(pkg->info, eon ? eon-pkg->info : 0);
	}
	break;
      default:
	croak("unexpected tag %s", tag_name);
	break;
    }
    restore_chars();
  }
  PUTBACK;
}


static void
return_files(const Header header, int filter_mode) {
  dSP;
  if (header) {
    const char *s;
    STRLEN len;
    const char **list = NULL;
    HE_t he = memset(alloca(sizeof(*he)), 0, sizeof(*he));
    rpmsenseFlags *flags = NULL;

    if (filter_mode) {
      he->tag = RPMTAG_FILEFLAGS;
      if(headerGet(header, he, 0))
	flags = (rpmsenseFlags*)he->p.ui32p;
    }

    he->tag = RPMTAG_FILEPATHS;
    if (!headerGet(header, he, 0)) return;
    list = he->p.argv;

    for(he->ix = 0; he->ix < (int)he->c; he->ix++) {
      s = list[he->ix];
      len = strlen(list[he->ix]);

      if (filter_mode && (filter_mode & FILTER_MODE_CONF_FILES) &&
	  flags && (flags[he->ix] & RPMFILE_CONFIG) == 0)
	continue;

      push_name_only(s, len);
    }
    flags = _free(flags);
    list = _free(list);
  }
  PUTBACK;
}

static void
return_problems(rpmps ps, int translate_message, int raw_message) {
  dSP;
  if (ps && rpmpsNumProblems(ps) > 0) {
    rpmpsi iterator = rpmpsInitIterator(ps);
    while (rpmpsNextIterator(iterator) >= 0) {
      rpmProblem p = rpmpsGetProblem(iterator->ps, iterator->ix);

      if (translate_message) {
	/* translate error using rpm localization */
	const char *buf = rpmProblemString(p);
	SV *sv = newSVpv(buf, 0);
	if (rpm_codeset_is_utf8) SvUTF8_on(sv);
	mXPUSHs(sv);
	_free(buf);
      }
      if (raw_message) {
	const char *pkgNEVR = rpmProblemGetPkgNEVR(p) ? rpmProblemGetPkgNEVR(p) : "";
	const char *altNEVR = rpmProblemGetAltNEVR(p) ? rpmProblemGetAltNEVR(p) : "";
	const char *s = rpmProblemGetStr(p) ? rpmProblemGetStr(p) : "";
	SV *sv;

	switch (rpmProblemGetType(p)) {
	case RPMPROB_BADARCH:
	  sv = newSVpvf("badarch@%s", pkgNEVR); break;
	case RPMPROB_BADOS:
	  sv = newSVpvf("bados@%s", pkgNEVR); break;
	case RPMPROB_PKG_INSTALLED:
	  sv = newSVpvf("installed@%s", pkgNEVR); break;
	case RPMPROB_BADRELOCATE:
	  sv = newSVpvf("badrelocate@%s@%s", pkgNEVR, s); break;
	case RPMPROB_NEW_FILE_CONFLICT:
	case RPMPROB_FILE_CONFLICT:
	  sv = newSVpvf("conflicts@%s@%s@%s", pkgNEVR, altNEVR, s); break;
	case RPMPROB_OLDPACKAGE:
	  sv = newSVpvf("installed@%s@%s", pkgNEVR, altNEVR); break;
	case RPMPROB_DISKSPACE:
	  sv = newSVpvf("diskspace@%s@%s@%lld", pkgNEVR, s, (long long)rpmProblemGetDiskNeed(p)); break;
	case RPMPROB_DISKNODES:
	  sv = newSVpvf("disknodes@%s@%s@%lld", pkgNEVR, s, (long long)rpmProblemGetDiskNeed(p)); break;
	case RPMPROB_REQUIRES:
	  sv = newSVpvf("requires@%s@%s", pkgNEVR, altNEVR+2); break;
	case RPMPROB_CONFLICT:
	  sv = newSVpvf("conflicts@%s@%s", pkgNEVR, altNEVR+2); break;
	default:
	  sv = newSVpvf("unknown@%s", pkgNEVR); break;
	}
	mXPUSHs(sv);
      }
    }
    rpmpsFreeIterator(iterator);
  }
  PUTBACK;
}

static char *
pack_list(const Header header, rpmTag tag_name, rpmTag tag_flags, rpmTag tag_version, rpmsenseFlags (*check_flag)(rpmsenseFlags)) {
  char buff[8*BUFSIZ];
  char *p = buff;
  HE_t he = memset(alloca(sizeof(*he)), 0, sizeof(*he));

  he->tag = tag_name;
  if (headerGet(header, he, 0)) {
    const char **list = he->p.argv;
    rpmsenseFlags *flags = NULL;
    const char **list_evr = NULL;
    int count = he->c;
   
    if (tag_flags) {
      he->tag = tag_flags;
      if(headerGet(header, he, 0))
	flags = (rpmsenseFlags*)he->p.ui32p;
    }
    if (tag_version) {
      he->tag = tag_version;
      if(headerGet(header, he, 0))
	list_evr = he->p.argv;
    }
    for(he->ix = 0; he->ix < count; he->ix++) {
      if (check_flag && (flags == NULL || !check_flag(flags[he->ix]))) continue;
      int len = print_list_entry(p, sizeof(buff)-(p-buff)-1, list[he->ix], flags ? flags[he->ix] : 0, list_evr ? list_evr[he->ix] : NULL);
      if (len < 0) continue;
      p += len;
      *p++ = '@';
    }
    if (p > buff) p[-1] = 0;

    flags = _free(flags);
    list = _free(list);
    list_evr = _free(list_evr);
  }

  return p > buff ? memcpy(malloc(p-buff), buff, p-buff) : NULL;
}

/* This function might modify strings that needs to be reverted after use
 * with restore_chars()
 */
static const char *
get_evr(const URPM__Package pkg) {
  const char *evr = NULL;
  if(pkg->info && !pkg->h) {
    if (!pkg->provides) {
      /* for src.rpms there's no @provides@ field added to the synthesis, so
       * we'll create one by request here for EVR. */
      char *name = NULL;
      int epoch = 0;
      char *version = NULL;
      char *release = NULL;
      get_fullname_parts(pkg, &name, &epoch, &version, &release, NULL, NULL, NULL, NULL);
      int sz = asprintf(&pkg->provides, "%s[== %d:%s-%s]", name, epoch, version, release);
      restore_chars();
      if (sz < 0)
	return "";
    }
    char *name = NULL;
    char *tmp = NULL, *tmp2 = NULL, *tmp3 = NULL;
    get_fullname_parts(pkg, &name, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    /*
     * TODO: this function is way too awkward and complex now, need to change
     *       pattern & separator
     */
    if(name) {
      size_t namelen = strlen(name);
      char needle[namelen+3];
      snprintf(needle, namelen+3, "@%s[", name);
      restore_chars();
      tmp = pkg->provides;
      if(!strncmp(pkg->provides, needle+1, namelen+1)) {
	evr = pkg->provides;
      }
      while(tmp && (tmp = strstr(tmp, needle))) {
	if(evr && (tmp3 = strchr(evr, '@')))
	  backup_char(tmp3);
	if((tmp2 = strchr(++tmp, '@')))
	  *tmp2 = '\0';
	if(evr == NULL || strlen(tmp) > strlen(evr))
	  evr = tmp;
	if(tmp2)
	  *tmp2 = '@';
      }
      if(!evr)
	croak("unable to locate package name (%s) in @provides@%s", needle, pkg->provides);
      evr = strchr(evr, ' ');

      if(evr)
	tmp = strchr(++evr, ']');
      if(tmp)
	backup_char(tmp);
    }
  } else if(pkg->h) {
    rpmds ds = rpmdsThis(pkg->h, RPMTAG_PROVIDEVERSION, 0);
    const char *needle = rpmdsEVR(ds);
    if(needle[0] == '0' && needle[1] == ':')
      needle += 2;
    size_t len = strlen(needle);
    if (!headerIsEntry(pkg->h, RPMTAG_SOURCERPM))
      evr = needle;
    else {
      if (pkg->provides == NULL)
	pkg->provides = pack_list(pkg->h, RPMTAG_PROVIDENAME, RPMTAG_PROVIDEFLAGS, RPMTAG_PROVIDEVERSION, NULL);

      evr = strstr(pkg->provides, needle);
    }
    if(evr && strlen(evr) != len)
      backup_char((char*)&evr[len]);
    ds = rpmdsFree(ds);
  }
  return evr;
}

static void
pack_header(const URPM__Package pkg) {
  if (pkg->h) {
    if (pkg->info == NULL) {
      char buff[1024];
      char *p = buff;
      const char *group = get_name(pkg->h, RPMTAG_GROUP);
      const char *nvra = get_nvra(pkg->h);
      const char *disttag = get_name(pkg->h, RPMTAG_DISTTAG);
      const char *distepoch = get_name(pkg->h, RPMTAG_DISTEPOCH);

      p += snprintf(buff, sizeof(buff), "%s@%d@%d@%s", nvra,
		    get_int(pkg->h, RPMTAG_EPOCH), get_int(pkg->h, RPMTAG_SIZE), 
		    group);
      if (disttag || distepoch) {
	p = stpcpy(p, "@");
	if (disttag) {
	  p = stpcpy(p, disttag);
	  _free(disttag);
	}
	p = stpcpy(p, "@");
	if (distepoch) {
	  p = stpcpy(p, distepoch);
	  _free(distepoch);
	}
      }
      *++p = '\0';
      pkg->info = memcpy(malloc(p-buff), buff, p-buff);
      _free(group);
      _free(nvra);
    }
    if (pkg->filesize == 0) pkg->filesize = get_filesize(pkg->h);
    if (pkg->requires == NULL && pkg->suggests == NULL)
      has_old_suggests = 0;
    pkg->requires = pack_list(pkg->h, RPMTAG_REQUIRENAME, RPMTAG_REQUIREFLAGS, RPMTAG_REQUIREVERSION, is_not_old_suggests);
    if (has_old_suggests)
      pkg->suggests = pack_list(pkg->h, RPMTAG_REQUIRENAME, RPMTAG_REQUIREFLAGS, RPMTAG_REQUIREVERSION, is_old_suggests);
    else
        pkg->suggests = pack_list(pkg->h, RPMTAG_SUGGESTSNAME, RPMTAG_SUGGESTSFLAGS, RPMTAG_SUGGESTSVERSION, NULL);
    if (pkg->obsoletes == NULL)
      pkg->obsoletes = pack_list(pkg->h, RPMTAG_OBSOLETENAME, RPMTAG_OBSOLETEFLAGS, RPMTAG_OBSOLETEVERSION, NULL);
    if (pkg->conflicts == NULL)
      pkg->conflicts = pack_list(pkg->h, RPMTAG_CONFLICTNAME, RPMTAG_CONFLICTFLAGS, RPMTAG_CONFLICTVERSION, NULL);
    if (pkg->provides == NULL)
      pkg->provides = pack_list(pkg->h, RPMTAG_PROVIDENAME, RPMTAG_PROVIDEFLAGS, RPMTAG_PROVIDEVERSION, NULL);
    if (pkg->summary == NULL) {
      const char *summary = get_name(pkg->h, RPMTAG_SUMMARY);
      pkg->summary = summary ? (char*)summary : strdup("");
    }

    if (!(pkg->flag & FLAG_NO_HEADER_FREE)) pkg->h =headerFree(pkg->h);
    pkg->h = NULL;
  }
}

static void
update_hash_entry(HV *hash, const char *name, STRLEN len, int force, IV use_sense, const URPM__Package pkg) {
  SV** isv;

  if (!len) len = strlen(name);
  if ((isv = hv_fetch(hash, name, len, force))) {
    /* check if an entry has been found or created, it should so be updated */
    if (!SvROK(*isv) || SvTYPE(SvRV(*isv)) != SVt_PVHV) {
      SV* choice_set = (SV*)newHV();
      if (choice_set) {
	SvREFCNT_dec(*isv); /* drop the old as we are changing it */
	if (!(*isv = newRV_noinc(choice_set))) {
	  SvREFCNT_dec(choice_set);
	  *isv = &PL_sv_undef;
	}
      }
    }
    if (isv && *isv != &PL_sv_undef) {
      char id[8];
      STRLEN id_len = snprintf(id, sizeof(id), "%d", pkg->flag & FLAG_ID);
      SV **sense = hv_fetch((HV*)SvRV(*isv), id, id_len, 1);
      if (sense && use_sense) sv_setiv(*sense, use_sense);
    }
  }
}

static void
update_provide_entry(const char *name, STRLEN len, int force, IV use_sense, const URPM__Package pkg, HV *provides) {
  update_hash_entry(provides, name, len, force, use_sense, pkg);
}

static void
update_provides(const URPM__Package pkg, HV *provides) {
  if (pkg->h) {
    int len;
    HE_t he = memset(alloca(sizeof(*he)), 0, sizeof(*he));

    /* examine requires for files which need to be marked in provides */
    he->tag = RPMTAG_REQUIRENAME;
    if (headerGet(pkg->h, he, 0)) {
      for (he->ix = 0; he->ix < (int)he->c; he->ix++) {
	len = strlen(he->p.argv[he->ix]);
	if (he->p.argv[he->ix][0] == '/') (void)hv_fetch(provides, he->p.argv[he->ix], len, 1);
      }
      he->p.ptr = _free(he->p.ptr);
    }

    /* update all provides */
    he->tag = RPMTAG_PROVIDENAME;
    if (headerGet(pkg->h, he, 0)) {
      const char **list = he->p.argv;
      rpmsenseFlags *flags = NULL;
      int count = he->c;

      he->tag = RPMTAG_PROVIDEFLAGS;
      if (headerGet(pkg->h, he, 0))
	flags = (rpmsenseFlags*)he->p.ui32p;
      for (he->ix = 0; he->ix < count; he->ix++) {
	len = strlen(list[he->ix]);
	update_provide_entry(list[he->ix], len, 1, flags && (flags[he->ix] & (RPMSENSE_PREREQ|RPMSENSE_SCRIPT_PREUN|RPMSENSE_SCRIPT_PRE|RPMSENSE_SCRIPT_POSTUN|RPMSENSE_SCRIPT_POST|RPMSENSE_LESS|RPMSENSE_EQUAL|RPMSENSE_GREATER)),
	    pkg, provides);
      }
      flags = _free(flags);
      list = _free(list);
    }
  } else {
    char *ps, *s, *es;

    if ((s = pkg->requires) != NULL && *s != 0) {
      ps = strchr(s, '@');
      while(ps != NULL) {
	if (s[0] == '/') {
	  *ps = 0; es = strchr(s, '['); if (!es) es = strchr(s, ' '); *ps = '@';
	  (void)hv_fetch(provides, s, es != NULL ? es-s : ps-s, 1);
	}
	s = ps + 1; ps = strchr(s, '@');
      }
      if (s[0] == '/') {
      es = strchr(s, '['); if (!es) es = strchr(s, ' ');
	(void)hv_fetch(provides, s, es != NULL ? (U32)(es-s) : strlen(s), 1);
      }
    }

    if ((s = pkg->provides) != NULL && *s != 0) {
      char *es;

      ps = strchr(s, '@');
      while(ps != NULL) {
	*ps = 0; es = strchr(s, '['); if (!es) es = strchr(s, ' '); *ps = '@';
	update_provide_entry(s, es != NULL ? es-s : ps-s, 1, es != NULL, pkg, provides);
	s = ps + 1; ps = strchr(s, '@');
      }
      es = strchr(s, '['); if (!es) es = strchr(s, ' ');
      update_provide_entry(s, es != NULL ? es-s : 0, 1, es != NULL, pkg, provides);
    }
  }
}

static void
update_obsoletes(const URPM__Package pkg, HV *obsoletes) {
  if (pkg->h) {
    HE_t he = memset(alloca(sizeof(*he)), 0, sizeof(*he));

    /* update all provides */
    he->tag = RPMTAG_OBSOLETENAME;
    if (headerGet(pkg->h, he, 0)) {
      const char **list = he->p.argv;
      for (he->ix = 0; he->ix < (int)he->c; he->ix++)
	update_hash_entry(obsoletes, list[he->ix], 0, 1, 0, pkg);
    }
  } else {
    char *ps, *s;

    if ((s = pkg->obsoletes) != NULL && *s != 0) {
      char *es;

      ps = strchr(s, '@');
      while(ps != NULL) {
	*ps = 0; es = strchr(s, '['); if (!es) es = strchr(s, ' '); *ps = '@';
	update_hash_entry(obsoletes, s, es != NULL ? es-s : ps-s, 1, 0, pkg);
	s = ps + 1; ps = strchr(s, '@');
      }
      es = strchr(s, '['); if (!es) es = strchr(s, ' ');
      update_hash_entry(obsoletes, s, es != NULL ? es-s : 0, 1, 0, pkg);
    }
  }
}

static void
update_provides_files(const URPM__Package pkg, HV *provides) {
  if (pkg->h) {
    STRLEN len;
    HE_t he = memset(alloca(sizeof(*he)), 0, sizeof(*he));

    he->tag = RPMTAG_FILEPATHS;
    if(headerGet(pkg->h, he, 0)) {
      for (he->ix = 0; he->ix < (int)he->c; he->ix++) {
	len = strlen(he->p.argv[he->ix]);
	update_provide_entry(he->p.argv[he->ix], len, 0, 0, pkg, provides);
      }

      he->p.ptr= _free(he->p.ptr);
    }
  }
}

static int
open_archive(char *filename, pid_t *pid, int *empty_archive) {
  int fd;
  struct {
    char header[4];
    char toc_d_count[4];
    char toc_l_count[4];
    char toc_f_count[4];
    char toc_str_size[4];
    char uncompress[40];
    char trailer[4];
  } buf;

  fd = open(filename, O_RDONLY);
  if (fd >= 0) {
    int pos = lseek(fd, -(int)sizeof(buf), SEEK_END);
    if (read(fd, &buf, sizeof(buf)) != sizeof(buf) || strncmp(buf.header, "cz[0", 4) || strncmp(buf.trailer, "0]cz", 4))
      /* this is not an archive, open it without magic, but first rewind at begin of file */
      lseek(fd, 0, SEEK_SET);
    else if (pos == 0) {
      *empty_archive = 1;
      if (fd >= 0) close(fd);
      fd = -1;
    } else {
      /* this is an archive, create a pipe and fork for reading with uncompress defined inside */
      int fdno[2];

      if (!pipe(fdno)) {
	if ((*pid = fork()) != 0) {
	  fd_set readfds;
	  struct timeval timeout;

	  FD_ZERO(&readfds);
	  FD_SET(fdno[0], &readfds);
	  timeout.tv_sec = 1;
	  timeout.tv_usec = 0;
	  select(fdno[0]+1, &readfds, NULL, NULL, &timeout);

	  close(fd);
	  fd = fdno[0];
	  close(fdno[1]);
	} else {
	  char *unpacker[22]; /* enough for 40 bytes in uncompress to never overbuf */
	  char *p = buf.uncompress;
	  int ip = 0;
	  char *ld_loader = getenv("LD_LOADER");

	  if (ld_loader && *ld_loader)
	    unpacker[ip++] = ld_loader;

	  buf.trailer[0] = 0; /* make sure end-of-string is right */
	  while (*p) {
	    if (*p == ' ' || *p == '\t') *p++ = 0;
	    else {
	      unpacker[ip++] = p;
	      while (*p && *p != ' ' && *p != '\t') ++p;
	    }
	  }
	  unpacker[ip] = NULL; /* needed for execlp */

	  lseek(fd, 0, SEEK_SET);
	  dup2(fd, STDIN_FILENO); close(fd);
	  dup2(fdno[1], STDOUT_FILENO); close(fdno[1]);

	  /* get rid of "decompression OK, trailing garbage ignored" */
          if ((fd = open("/dev/null", O_WRONLY)) >= 0) {
	    dup2(fd, STDERR_FILENO); close(fd);
          }

	  execvp(unpacker[0], unpacker);
	  exit(1);
	}
      } else {
	close(fd);
	fd = -1;
      }
    }
  }
  return fd;
}

static int
call_package_callback(SV *urpm, SV *sv_pkg, SV *callback) {
  if (sv_pkg != NULL && callback != NULL) {
    int count;

    /* now, a callback will be called for sure */
    dSP;
    PUSHMARK(SP);
    XPUSHs(urpm);
    XPUSHs(sv_pkg);
    PUTBACK;
    count = call_sv(callback, G_SCALAR);
    SPAGAIN;
    if (count == 1 && !POPi) {
      /* package should not be added in depslist, so we free it */
      SvREFCNT_dec(sv_pkg);
      sv_pkg = NULL;
    }
    PUTBACK;
  }

  return sv_pkg != NULL;
}

static int
parse_line(AV *depslist, HV *provides, HV *obsoletes, URPM__Package pkg, char *buff, SV *urpm, SV *callback) {
  SV *sv_pkg;
  URPM__Package _pkg;
  char *tag, *data;
  int data_len;

  if (buff[0] == 0)
    return 1;
  else if ((tag = buff)[0] == '@' && (data = strchr(tag+1, '@')) != NULL) {
    *tag++ = *data++ = 0;
    data_len = 1+strlen(data);
    if (!strcmp(tag, "info")) {
      pkg->info = memcpy(malloc(data_len), data, data_len);
      pkg->flag &= ~FLAG_ID;
      pkg->flag |= 1 + av_len(depslist);
      sv_pkg = sv_setref_pv(newSVpvs(""), "URPM::Package",
			    _pkg = memcpy(malloc(sizeof(struct s_Package)), pkg, sizeof(struct s_Package)));
      if (call_package_callback(urpm, sv_pkg, callback)) {
	if (provides) update_provides(_pkg, provides);
	if (obsoletes) update_obsoletes(_pkg, obsoletes);
	av_push(depslist, sv_pkg);
      }
      memset(pkg, 0, sizeof(struct s_Package));
    } else if (!strcmp(tag, "filesize"))
      pkg->filesize = atoi(data);
    else if (!strcmp(tag, "requires"))
      free(pkg->requires), pkg->requires = memcpy(malloc(data_len), data, data_len);
    else if (!strcmp(tag, "suggests"))
      free(pkg->suggests), pkg->suggests = memcpy(malloc(data_len), data, data_len);
    else if (!strcmp(tag, "obsoletes"))
      free(pkg->obsoletes), pkg->obsoletes = memcpy(malloc(data_len), data, data_len);
    else if (!strcmp(tag, "conflicts"))
      free(pkg->conflicts), pkg->conflicts = memcpy(malloc(data_len), data, data_len);
    else if (!strcmp(tag, "provides"))
      free(pkg->provides), pkg->provides = memcpy(malloc(data_len), data, data_len);
    else if (!strcmp(tag, "summary"))
      free(pkg->summary), pkg->summary = memcpy(malloc(data_len), data, data_len);
    return 1;
  } else
    fprintf(stderr, "bad line <%s>\n", buff);

  return 0;
}

#if 0
static void pack_rpm_header(Header *h) {
  Header packed = headerNew();

  HeaderIterator hi = headerInitIterator(*h);
  struct rpmtd_s td;
  while (headerNext(hi, &td)) {
      // fprintf(stderr, "adding %s %d\n", tagname(tag), c);
      headerPut(packed, &td, HEADERPUT_DEFAULT);
      rpmtdFreeData(&td);
  }

  headerFreeIterator(hi);
  *h = headerFree(*h);

  *h = packed;
}

static void drop_tags(Header *h) {
  headerDel(*h, RPMTAG_FILEUSERNAME); /* user ownership is correct */
  headerDel(*h, RPMTAG_FILEGROUPNAME); /* group ownership is correct */
  headerDel(*h, RPMTAG_FILEMTIMES); /* correct time without it */
  headerDel(*h, RPMTAG_FILEINODES); /* hardlinks work without it */
  headerDel(*h, RPMTAG_FILEDEVICES); /* it is the same number for every file */
  headerDel(*h, RPMTAG_FILESIZES); /* ? */
  headerDel(*h, RPMTAG_FILERDEVS); /* it seems unused. always empty */
  headerDel(*h, RPMTAG_FILEVERIFYFLAGS); /* only used for -V */
  headerDel(*h, RPMTAG_FILEDIGESTALGOS); /* only used for -V */
  headerDel(*h, RPMTAG_FILEDIGESTS); /* only used for -V */ /* alias: RPMTAG_FILEMD5S */ 
  /* keep RPMTAG_FILEFLAGS for %config (rpmnew) to work */
  /* keep RPMTAG_FILELANGS for %lang (_install_langs) to work */
  /* keep RPMTAG_FILELINKTOS for checking conflicts between symlinks */
  /* keep RPMTAG_FILEMODES otherwise it segfaults with excludepath */

  /* keep RPMTAG_POSTIN RPMTAG_POSTUN RPMTAG_PREIN RPMTAG_PREUN */
  /* keep RPMTAG_TRIGGERSCRIPTS RPMTAG_TRIGGERVERSION RPMTAG_TRIGGERFLAGS RPMTAG_TRIGGERNAME */
  /* small enough, and only in some packages. not needed per se */

  headerDel(*h, RPMTAG_ICON);
  headerDel(*h, RPMTAG_GIF);
  headerDel(*h, RPMTAG_EXCLUDE);
  headerDel(*h, RPMTAG_EXCLUSIVE);
  headerDel(*h, RPMTAG_COOKIE);
  headerDel(*h, RPMTAG_VERIFYSCRIPT);

  /* always the same for our packages */
  headerDel(*h, RPMTAG_VENDOR);
  headerDel(*h, RPMTAG_DISTRIBUTION);

  /* keep RPMTAG_SIGSIZE, useful to tell the size of the rpm file (+440) */

  headerDel(*h, RPMTAG_DSAHEADER);
  headerDel(*h, RPMTAG_SHA1HEADER);
  headerDel(*h, RPMTAG_SIGMD5);
  headerDel(*h, RPMTAG_SIGGPG);

  pack_rpm_header(h);
}
#endif

static int
update_header(char *filename, URPM__Package pkg, __attribute__((unused)) int keep_all_tags, int vsflags) {
  int d = open(filename, O_RDONLY);

  if (d >= 0) {
    unsigned char sig[4];

    if (read(d, &sig, sizeof(sig)) == sizeof(sig)) {
      lseek(d, 0, SEEK_SET);
      if (sig[0] == 0xed && sig[1] == 0xab && sig[2] == 0xee && sig[3] == 0xdb) {
	FD_t fd = fdDup(d);
	Header header;
	rpmts ts;

	close(d); d = -1;
	ts = rpmtsCreate();
	rpmtsSetVSFlags(ts, _RPMVSF_NOSIGNATURES | vsflags);
	if (fd != NULL && rpmReadPackageFile(ts, fd, filename, &header) == 0 && header) {
	  Fclose(fd);

	  if (pkg->h && !(pkg->flag & FLAG_NO_HEADER_FREE)) pkg->h = headerFree(pkg->h);
	  pkg->h = header;
	  pkg->flag &= ~FLAG_NO_HEADER_FREE;

	  /*if (!keep_all_tags) drop_tags(&pkg->h);*/
	  (void)rpmtsFree(ts);
	  return 1;
	}
	(void)rpmtsFree(ts);
      } else if (sig[0] == 0x8e && sig[1] == 0xad && sig[2] == 0xe8 && sig[3] == 0x01) {
	FD_t fd = fdDup(d);

	close(d); d = -1;
	if (fd != NULL) {
	  if (pkg->h && !(pkg->flag & FLAG_NO_HEADER_FREE)) pkg->h = headerFree(pkg->h);
	  const char item[] = "Header";
	  const char * msg = NULL;
	  rpmRC rc = rpmpkgRead(item, fd, &pkg->h, &msg);

	  switch (rc) {
	  default:
	    rpmlog(RPMLOG_ERR, "%s: %s: %s\n", "rpmpkgRead", item, msg);
	  case RPMRC_NOTFOUND:
	    pkg->h = NULL;
	  case RPMRC_OK:
	    break;
	  }
	  msg = (const char*)_free(msg);

	  pkg->flag &= ~FLAG_NO_HEADER_FREE;
	  Fclose(fd);
	  return 1;
	}
      }
    }
    if (d >= 0) close(d);
  }
  return 0;
}

static int
read_config_files(int force) {
  static int already = 0;
  int rc = 0;

  if (!already || force) {
    rc = rpmReadConfigFiles(NULL, NULL);
    already = (rc == 0); /* set config as load only if it succeed */
  }
  return rc;
}

static void
ts_nosignature(rpmts ts) {
  rpmtsSetVSFlags(ts, _RPMVSF_NODIGESTS | _RPMVSF_NOSIGNATURES);
}

static void *
rpmRunTransactions_callback(__attribute__((unused)) const void *h,
					 const rpmCallbackType what, 
					 const rpmuint64_t amount, 
					 const rpmuint64_t total,
					 fnpyKey pkgKey,
					 rpmCallbackData data) {
  static struct timeval tprev;
  static struct timeval tcurr;
  static FD_t fd = NULL;
  long delta;
  int i;
  struct s_TransactionData *td = data;
  SV *callback = NULL;
  char *callback_type = NULL;
  char *callback_subtype = NULL;
  
  rpmdbCheckTerminate(0);

  if (!td)
    return NULL;

  switch (what) {
    case RPMCALLBACK_INST_OPEN_FILE:
      callback = td->callback_open;
      callback_type = "open";
      break;
    case RPMCALLBACK_INST_CLOSE_FILE:
      callback = td->callback_close;
      callback_type = "close";
      break;
    case RPMCALLBACK_TRANS_START:
    case RPMCALLBACK_TRANS_PROGRESS:
    case RPMCALLBACK_TRANS_STOP:
      callback = td->callback_trans;
      callback_type = "trans";
      break;
    case RPMCALLBACK_UNINST_START:
    case RPMCALLBACK_UNINST_PROGRESS:
    case RPMCALLBACK_UNINST_STOP:
      callback = td->callback_uninst;
      callback_type = "uninst";
      break;
    case RPMCALLBACK_INST_START:
    case RPMCALLBACK_INST_PROGRESS:
      callback = td->callback_inst;
      callback_type = "inst";
      break;
    default:
      break;
  }

  if (callback != NULL) {
    switch (what) {
      case RPMCALLBACK_TRANS_START:
      case RPMCALLBACK_UNINST_START:
      case RPMCALLBACK_INST_START:
	callback_subtype = "start";
	gettimeofday(&tprev, NULL);
	break;
      case RPMCALLBACK_TRANS_PROGRESS:
      case RPMCALLBACK_UNINST_PROGRESS:
      case RPMCALLBACK_INST_PROGRESS:
	callback_subtype = "progress";
	gettimeofday(&tcurr, NULL);
	delta = 1000000 * (tcurr.tv_sec - tprev.tv_sec) + (tcurr.tv_usec - tprev.tv_usec);
	if (delta < td->min_delta && amount < total - 1)
	  callback = NULL; /* avoid calling too often a given callback */
	else
	  tprev = tcurr;
	break;
      case RPMCALLBACK_TRANS_STOP:
      case RPMCALLBACK_UNINST_STOP:
	callback_subtype = "stop";
	break;
      default:
	break;
    }

    if (callback != NULL) {
      /* now, a callback will be called for sure */
      dSP;
      ENTER;
      SAVETMPS;
      PUSHMARK(SP);
      XPUSHs(td->data);
      push_name_only(callback_type, 0);
      XPUSHs(pkgKey != NULL ? sv_2mortal(newSViv((long)pkgKey - 1)) : &PL_sv_undef);
      if (callback_subtype != NULL) {
	push_name_only(callback_subtype, 0);
	mXPUSHs(newSViv(amount));
	mXPUSHs(newSViv(total));
      }
      PUTBACK;
      i = call_sv(callback, callback == td->callback_open ? G_SCALAR : G_DISCARD);
      SPAGAIN;
      if (callback == td->callback_open) {
	if (i != 1) croak("callback_open should return a file handle");
	i = POPi;
	fd = fdDup(i);
	if (fd) {
	  fd = fdLink(fd, "persist perl-URPM");
	  (void) Fcntl(fd, F_SETFD, (void *)1); /* necessary to avoid forked/execed process to lock removable */
	}
	PUTBACK;
      } else if (callback == td->callback_close) {
	fd = fdFree(fd, "persist perl-URPM");
	if (fd) {
	  Fclose(fd);
	  fd = NULL;
	}
      }
      FREETMPS;
      LEAVE;
    }
  }
  return callback == td->callback_open ? fd : NULL;
}

static int
bdb_log_archive(DB_ENV *dbenv, char ***list, uint32_t flags) {
  int ret;
  if ((ret = dbenv->log_archive(dbenv, list, flags)) != 0)
    dbenv->err(dbenv, ret, "DB_ENV->log_archive");
  return ret;
}

static int
bdb_log_lsn_reset(DB_ENV *dbenv) {
  int ret = 0;
  char **list = NULL;
  /* Reset log sequence numbers to allow for moving to new environment */
  if(!(ret = dbenv->log_archive(dbenv, &list, DB_ARCH_DATA|DB_ARCH_ABS))) {
    char **p = list;
    for(; *p; p++)
      if(!ret)
	ret = dbenv->lsn_reset(dbenv, *p, 0);
    _free(list);
  }
  return ret;
}

/* This should be mostly working..
 * TODO:
 * Cleanup
 * Better error checking
 */

static int
rpmdb_convert(const char *prefix, int dbtype, int swap, int rebuild) {
  rpmts tsCur = NULL;
  int xx, i;
  const char *dbpath = NULL;
  const char *__dbi_txn = NULL;
  const char *_dbi_tags = NULL;
  const char *_dbi_config = NULL;
  const char *_dbi_config_Packages = NULL;
  const char *fn = NULL, *fn2 = NULL;
  const char *tmppath = NULL;
  glob_t gl = { .gl_pathc = 0, .gl_pathv = NULL, .gl_offs = 0 };
  struct stat st;

  unsetenv("TMPDIR");
  rpmReadConfigFiles(NULL, NULL);

  dbpath = rpmExpand("%{?_dbpath}", NULL);
  __dbi_txn = rpmExpand("%{__dbi_txn}", NULL);
  _dbi_tags = rpmExpand("%{_dbi_tags}", NULL);
  _dbi_config = rpmExpand("%{_dbi_config}", NULL);
  _dbi_config_Packages = rpmExpand("%{_dbi_config_Packages}", NULL);
  addMacro(NULL, "__dbi_txn", NULL, "create nofsync mpool txn thread thread_count=64", -1);

  /* (ugly) clear any existing locks */
  fn = rpmGetPath(prefix && prefix[0] ? prefix : "", dbpath, "/", "__db.*", NULL);
  xx = Glob(fn, 0, NULL, &gl);
  for (i = 0; i < (int)gl.gl_pathc; i++)
    xx = Unlink(gl.gl_pathv[i]);
  fn = _free(fn);
  Globfree(&gl);

  tsCur = rpmtsCreate();
  rpmtsSetRootDir(tsCur, prefix && prefix[0] ? prefix : NULL);

  /* To try make upgrades smooth, we've tried to prevent the new configuration
   * with possibly incompatible configuration from being dropped in during the
   * upgrade. Now that the rpm upgrade has finished we'll make sure to switch
   * to this new configuration before performing the conversion.
   */
  fn2 = rpmGetPath(prefix && prefix[0] ? prefix : "", "%{_dbpath}", "/DB_CONFIG.rpmnew", NULL);
  if (!Stat(fn2, &st) && st.st_size) {
    fn = rpmGetPath(prefix && prefix[0] ? prefix : "", "%{_dbpath}", "/DB_CONFIG", NULL);
    if (!Stat(fn, &st)) {
      /* if empty configuration, we'll just remove it */
      if (!st.st_size)
	Unlink(fn);
      else {
	/* if non-empty configuration exists, we'll rename it */
	fn2 = rpmGetPath(prefix && prefix[0] ? prefix : "", "%{_dbpath}", "/DB_CONFIG.rpmsave", NULL);
	Rename(fn, fn2);
	fn2 = _free(fn2);
      }
    }
    Rename(fn2, fn);
    fn = _free(fn);
  }
  fn2 = _free(fn2);

  if(!rpmtsOpenDB(tsCur, O_RDONLY)) {
    if(dbtype == 1) {
      addMacro(NULL, "_dbi_tags", NULL, "Packages:Name:Basenames:Group:Requirename:Providename:Conflictname:Triggername:Dirnames:Requireversion:Provideversion:Installtid:Sigmd5:Sha1header:Filedigests:Depends:Pubkeys", -1);
      addMacro(NULL, "_dbi_config", NULL, "%{_dbi_htconfig}", -1);
      addMacro(NULL, "_dbi_config_Packages", NULL, "%{_dbi_htconfig} lockdbfd", -1);
    }

    rpmts tsNew = rpmtsCreate();
    rpmdb rdbNew = NULL;
    DB_ENV *dbenvNew = NULL;
    struct stat sb;

    fn = rpmGetPath("%{_dbpath}", NULL);
    tmppath = tempnam(fn, "rpmdb_convert.XXXXXX");
    fn = _free(fn);
    addMacro(NULL, "_dbpath", NULL, tmppath, -1);
    rpmtsSetRootDir(tsNew, prefix && prefix[0] ? prefix : NULL);
    if(!rpmtsOpenDB(tsNew, O_RDWR)) {
      DBC *dbcpCur = NULL, *dbcpNew = NULL;
      rdbNew = rpmtsGetRdb(tsNew);
      dbenvNew = rdbNew->db_dbenv;
      dbiIndex dbiCur = dbiOpen(rpmtsGetRdb(tsCur), RPMDBI_PACKAGES, 0);
      dbiIndex dbiNew = dbiOpen(rdbNew, RPMDBI_PACKAGES, 0);
      DB_TXN *txnidNew = dbiTxnid(dbiNew);

      if(!(xx = dbiCopen(dbiCur, NULL, NULL, 0)) && !(xx = dbiCopen(dbiNew, txnidNew, &dbcpNew, DB_WRITECURSOR))) {
	DB * _dbN = (DB *) dbiNew->dbi_db;
	DB * _dbO = (DB *) dbiCur->dbi_db;
	DBT key, data;
	DB_TXN *txnidCur = dbiTxnid(dbiCur);
	uint32_t nkeys = 0;

	memset(&key, 0, sizeof(key));
	memset(&data, 0, sizeof(data));

	/* Acquire a cursor for the database. */
	if ((xx = _dbO->cursor(_dbO, NULL, &dbcpCur, 0)) != 0)
	  _dbO->err(_dbO, xx, "DB->cursor");

	if(!(xx = _dbO->stat(_dbO, txnidCur, &dbiCur->dbi_stats, 0))) {

	  switch(_dbO->type) {
	    case DB_BTREE:
	    case DB_RECNO: {
		DB_BTREE_STAT *db_stat = dbiCur->dbi_stats;
		nkeys = db_stat->bt_nkeys;
	      }
	      break;
	    case DB_HASH: {
		DB_HASH_STAT *db_stat = dbiCur->dbi_stats;
		nkeys = db_stat->hash_nkeys;
	      }
	      break;
	    case DB_QUEUE: {
		DB_QUEUE_STAT *db_stat = dbiCur->dbi_stats;
		nkeys = db_stat->qs_nkeys;
	      }
	      break;
	    case DB_UNKNOWN:
	    default:
	      xx = -1;
	      break;
	  }


	  if(!xx) {
	    uint32_t i = 0;
	    int doswap = -1;
	    float pct = 0;
	    uint8_t tmp;
	    /* 
	     * Older rpm places number of keys as first entry of hash database,
	     * so any package placed at beginning of it will be "missing" from
	     * rpmdb...
	     */
	    if (dbtype == 1){
	      key.data = &i;
	      data.data = &nkeys;
	      key.size = data.size = sizeof(uint32_t);
	      xx = _dbN->put(_dbN, NULL, &key, &data, 0);
	    }
	    while ((xx = dbcpCur->c_get(dbcpCur, &key, &data, DB_NEXT)) == 0) {
	      tmp = pct;
	      pct = (100*(float)++i/nkeys) + 0.5;
	      /* TODO: callbacks for status output? */
	      if(tmp < (int)(pct+0.5)) {
		fprintf(stderr, "\rconverting %s%s/Packages: %u/%u %d%%", prefix && prefix[0] ? prefix : "", tmppath, i, nkeys, (int)pct);
	      }
	      fflush(stdout);
	      if(i == 1 && !*(uint32_t*)key.data)
		    continue;

	      if(__builtin_expect(doswap, 1) < 0) {
		if((htole32(*(uint32_t*)key.data) > 10000000 && swap < 0) ||
		    (htole32(*(uint32_t*)key.data) < 10000000 && swap > 0))
		  doswap = 1;
		else
		  doswap = 0;
	      }
	      if(__builtin_expect(doswap, 1)) {
		if(swap)
		  *(uint32_t*)key.data = bswap32(*(uint32_t*)key.data);
	      }
	      xx = _dbN->put(_dbN, NULL, &key, &data, 0);

	    }
	    fprintf(stderr, "\n");
	    if(!(xx = dbiCclose(dbiNew, dbcpNew, 0)) && !(xx = dbiCclose(dbiCur, dbcpCur, 0)) &&
		rebuild) {
	      xx = rpmtsCloseDB(tsCur);

	      rpmVSFlags vsflags = rpmExpandNumeric("%{_vsflags_rebuilddb}");
	      if (rpmcliQueryFlags & VERIFY_DIGEST)
		vsflags |= _RPMVSF_NODIGESTS;
	      if (rpmcliQueryFlags & VERIFY_SIGNATURE)
		vsflags |= _RPMVSF_NOSIGNATURES;
	      rpmtsSetVSFlags(tsNew, vsflags);

	      {
		size_t dbix;
		fprintf(stderr, "rebuilding rpmdb:\n");
		fflush(stdout);
		for (dbix = 0; dbix < rdbNew->db_ndbi; dbix++) {
		  tagStore_t dbiTags = &rdbNew->db_tags[dbix];

		  /* Remove configured secondary indices. */
		  switch (dbiTags->tag) {
		    case RPMDBI_AVAILABLE:
		    case RPMDBI_ADDED:
		    case RPMDBI_REMOVED:
		    case RPMDBI_DEPCACHE:
		    case RPMDBI_BTREE:
		    case RPMDBI_HASH:
		    case RPMDBI_QUEUE:
		    case RPMDBI_RECNO:
		      fprintf(stderr, "skipping %s:\t%d%%\n", (dbiTags->str != NULL ? dbiTags->str : tagName(dbiTags->tag)),
			  (int)(100*((float)dbix/rdbNew->db_ndbi)));
		    case RPMDBI_PACKAGES:
		    case RPMDBI_SEQNO:
		      continue;
		      break;
		    default:
		      fn = rpmGetPath(rdbNew->db_root, rdbNew->db_home, "/",
			  (dbiTags->str != NULL ? dbiTags->str : tagName(dbiTags->tag)),
			  NULL);
		      fprintf(stderr, "%s:\t", fn);
		      if (!Stat(fn, &sb))
			xx = Unlink(fn);
		      fn = _free(fn);
		      break;
		  }
		  /* TODO: signal handler? */

		  /* Open (and re-create) each index. */
		  (void) dbiOpen(rdbNew, dbiTags->tag, rdbNew->db_flags);
		  fprintf(stderr, "%d%%\n", (int)(100*((float)dbix/rdbNew->db_ndbi)));
		  fflush(stdout);
		}

		/* Unreference header used by associated secondary index callbacks. */
		(void) headerFree(rdbNew->db_h);
		rdbNew->db_h = NULL;

		/* Reset the Seqno counter to the maximum primary key */
		rpmlog(RPMLOG_DEBUG, "rpmdb: max. primary key %u\n",
		    (unsigned)rdbNew->db_maxkey);
		fn = rpmGetPath(rdbNew->db_root, rdbNew->db_home, "/Seqno", NULL);
		if (!Stat(fn, &sb))
		  xx = Unlink(fn);
		fprintf(stderr, "%s:\t", fn);
		(void) dbiOpen(rdbNew, RPMDBI_SEQNO, rdbNew->db_flags);
		fprintf(stderr, "100%%\n");

		fn = _free(fn);

		/* Remove no longer required transaction logs */
		if(!(xx = bdb_log_archive(dbenvNew, NULL, DB_ARCH_REMOVE)))
		  xx = bdb_log_lsn_reset(dbenvNew);
		xx = rpmtsCloseDB(tsNew);
	      }
	    }
	  }
	}
      }
      if(!xx) {
	const char *dest = NULL;
	size_t dbix;
	
	if(!rpmtsOpenDB(tsNew, O_RDONLY)) {
	  rdbNew = rpmtsGetRdb(tsNew);
	  for (dbix = 0; dbix < rdbNew->db_ndbi; dbix++) {
	    tagStore_t dbiTags = &rdbNew->db_tags[dbix];
	    fn = rpmGetPath(rdbNew->db_root, rdbNew->db_home, "/", dbiTags->str, NULL);
	    dest = rpmGetPath(rdbNew->db_root, dbpath, "/", dbiTags->str, NULL);
	    if(!Stat(dest, &sb))
	      xx = Unlink(dest);
	    if(!Stat(fn, &sb)) {
	      xx = Rename(fn, dest);
	    }
	    fn = _free(fn);
	    dest = _free(dest);
	  }
	  dest = rpmGetPath(rdbNew->db_root, NULL);
	  xx = rpmtsCloseDB(tsNew);

	  /* (ugly) cleanup */
	  fn = rpmGetPath(dest, tmppath, "/", "*", NULL);
	  xx = Glob(fn, 0, NULL, &gl);
	  for (i = 0; i < (int)gl.gl_pathc; i++)
	    xx = Unlink(gl.gl_pathv[i]);
	  fn = _free(fn);
	  Globfree(&gl);

	  fn = rpmGetPath(dest, dbpath, "/log/", "*", NULL);
	  xx = Glob(fn, 0, NULL, &gl);
	  for (i = 0; i < (int)gl.gl_pathc; i++)
	    xx = Unlink(gl.gl_pathv[i]);
	  fn = _free(fn);
	  Globfree(&gl);

	  fn = rpmGetPath(dest, tmppath, "/log/", "*", NULL);
	  xx = Glob(fn, 0, NULL, &gl);
	  for (i = 0; i < (int)gl.gl_pathc; i++)
	    xx = Unlink(gl.gl_pathv[i]);
	  fn = _free(fn);
	  Globfree(&gl);

	  fn = rpmGetPath(dest, dbpath, "/tmp/", "*", NULL);
	  xx = Glob(fn, 0, NULL, &gl);
	  for (i = 0; i < (int)gl.gl_pathc; i++)
	    xx = Unlink(gl.gl_pathv[i]);
	  fn = _free(fn);
	  Globfree(&gl);

	  /* remove indices no longer used */
	  fn = rpmGetPath(dest, dbpath, "Provideversion", NULL);
	  if(!Stat(fn, &sb))
	    xx = Unlink(fn);
	  fn = _free(fn);
 	  fn = rpmGetPath(dest, dbpath, "Requireversion", NULL);
	  if(!Stat(fn, &sb))
	    xx = Unlink(fn);
	  fn = _free(fn);

	  /* clear locks */
	  fn = rpmGetPath(prefix && prefix[0] ? prefix : "", dbpath, "/", "__db.*", NULL);
	  xx = Glob(fn, 0, NULL, &gl);
	  for (i = 0; i < (int)gl.gl_pathc; i++)
	    xx = Unlink(gl.gl_pathv[i]);
	  fn = _free(fn);
	  Globfree(&gl);

	  fn = rpmGetPath(dest, tmppath, "/log", NULL);
	  xx = Rmdir(fn);
	  fn = _free(fn);
	  fn = rpmGetPath(dest, tmppath, NULL);
	  xx = Rmdir(fn);
	  fn = _free(fn);

	  _free(dest);
	}
      }
    }
    tsNew = rpmtsFree(tsNew);
  }
  tsCur = rpmtsFree(tsCur);
  addMacro(NULL, "_dbpath", NULL, dbpath, -1);
  addMacro(NULL, "__dbi_txn", NULL, __dbi_txn, -1);
  addMacro(NULL, "_dbi_tags", NULL, _dbi_tags, -1);
  addMacro(NULL, "_dbi_config", NULL, _dbi_config, -1);
  addMacro(NULL, "_dbi_config_Packages", NULL, _dbi_config_Packages, -1);

  _free(dbpath);
  _free(__dbi_txn);
  _free(_dbi_tags);
  _free(_dbi_config);
  _free(_dbi_config_Packages);
  _free(tmppath);
  return xx;
}

static bool detectXZ(const char *path) {
    struct stat sb;
    bool ret = false;

    if (Stat(path, &sb) >= 0 && sb.st_size > 8) {
      unsigned char buf[8];
      FD_t fd;

      if ((fd = Fopen(path, "r")) != NULL
       && Fread(buf, 1, sizeof(buf), fd) == sizeof(buf)
       && (buf[0] == 0xFD && buf[1] == 0x37 && buf[2] == 0x7A &&
	   buf[3] == 0x58 && buf[4] == 0x5A))
	ret = true;
      if (fd) (void) Fclose(fd);
    }

    return ret;
}

static FD_t xOpen(const char *path) {
    FD_t fd = NULL;
    const char *message, *tmp;
    magic_t cookie;
      enum {
	FD_ASCII,
	FD_BZIP2,
	FD_GZIP,
	FD_LZMA,
	FD_XZ,
	FD_FAIL
      } type = FD_FAIL;
    if ((cookie = magic_open(MAGIC_NONE)) && !magic_load(cookie, NULL)) {
	message = magic_file(cookie, path);
	if(message == NULL)
	    type = FD_FAIL;
	else if(strstr(message, "ASCII"))
	    type = FD_ASCII;
	else if(strstr(message, "bzip2 compressed"))
	    type = FD_BZIP2;
	else if(strstr(message, "gzip compressed"))
	    type = FD_GZIP;
	else if(strstr(message, "xz compressed"))
	    type = FD_XZ;
	else if(strstr(message, "LZMA compressed"))
	    type = FD_LZMA;
	magic_close(cookie);
    }
    if(type == FD_FAIL) {
      if (detectXZ(path))
	type = FD_XZ;
      else if ((tmp = strrchr(path, '.'))) {
	if(!strcmp(tmp, ".bz2"))
	  type = FD_BZIP2;
	if(!strcmp(tmp, ".cz") || !strcmp(tmp, ".gz"))
	  type = FD_GZIP;
	else if(!strcmp(tmp, ".xz"))
	  type = FD_XZ;
	else if(!strcmp(tmp, ".lzma"))
	  type = FD_LZMA;
      }
    }

    switch(type) {
	case FD_ASCII:
	    fd = Fopen(path, "r.fdio");
	    break;
	case FD_BZIP2:
	    fd = Fopen(path, "r.bzdio");
	    break;
	case FD_GZIP:
	    fd = Fopen(path, "r.gzdio");
	    break;
	case FD_LZMA:
	case FD_XZ:
	    fd = Fopen(path, "r.xzdio");
	    break;
	default:
	    break;
    }

    return fd;
}

static void
urpm_perl_atexit(void)
{
  (void) rpmcliFini(NULL);
}

MODULE = URPM            PACKAGE = URPM::Package       PREFIX = Pkg_

void
Pkg_DESTROY(pkg)
  URPM::Package pkg
  CODE:
  free(pkg->info);
  free(pkg->requires);
  free(pkg->suggests);
  free(pkg->obsoletes);
  free(pkg->conflicts);
  free(pkg->provides);
  free(pkg->rflags);
  free(pkg->summary);
  if (pkg->h && !(pkg->flag & FLAG_NO_HEADER_FREE)) pkg->h = headerFree(pkg->h);
  free(pkg);

void
Pkg_name(pkg)
  URPM::Package pkg
  INIT:
  char *name;
  PPCODE:
  if (pkg->info) {
    get_fullname_parts(pkg, &name, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    push_name_only(name, 0);
    restore_chars();
  } else if (pkg->h)
    push_name(pkg, RPMTAG_NAME);

void
Pkg_version(pkg)
  URPM::Package pkg
  INIT:
  char *version;
  PPCODE:
  if (pkg->info) {
    get_fullname_parts(pkg, NULL, NULL, &version, NULL, NULL, NULL, NULL, NULL);
    push_name_only(version, 0);
    restore_chars();
  } else if (pkg->h)
    push_name(pkg, RPMTAG_VERSION);

void
Pkg_release(pkg)
  URPM::Package pkg
  INIT:
  char *release;
  PPCODE:
  if (pkg->info) {
    get_fullname_parts(pkg, NULL, NULL, NULL, &release, NULL, NULL, NULL, NULL);
    push_name_only(release, 0);
    restore_chars();
  } else if (pkg->h)
    push_name(pkg, RPMTAG_RELEASE);

void
Pkg_disttag(pkg)
  URPM::Package pkg
  INIT:
  char *disttag;
  PPCODE:
  if (pkg->info) {
    get_fullname_parts(pkg, NULL, NULL, NULL, NULL, &disttag, NULL, NULL, NULL);
    push_name_only(disttag, 0);
    restore_chars();
  } else if (pkg->h)
    push_name(pkg, RPMTAG_DISTTAG);

void
Pkg_distepoch(pkg)
  URPM::Package pkg
  INIT:
  char *distepoch;
  PPCODE:
  if (pkg->info) {
    get_fullname_parts(pkg, NULL, NULL, NULL, NULL, NULL, &distepoch, NULL, NULL);
    push_name_only(distepoch, 0);
    restore_chars();
  } else if (pkg->h)
    push_name(pkg, RPMTAG_DISTEPOCH);

void
Pkg_arch(pkg)
  URPM::Package pkg
  INIT:
  char *arch;
  PPCODE:
  if (pkg->info) {
    get_fullname_parts(pkg, NULL, NULL, NULL, NULL, NULL, NULL, &arch, NULL);
    push_name_only(arch, 0);
    restore_chars();
  } else if (pkg->h) {
    if (headerIsEntry(pkg->h, RPMTAG_ARCH)) {
      if (headerIsEntry(pkg->h, RPMTAG_SOURCERPM)) {
	push_name(pkg, RPMTAG_ARCH);
      } else
	mXPUSHs(newSVpvs("src"));
    }
    else
      /* gpg-pubkey packages has no arch tag */
      mXPUSHs(newSVpvs(""));
  }

int
Pkg_is_arch_compat__XS(pkg)
  URPM::Package pkg
  INIT:
  const char * platform;
  CODE:
  read_config_files(0);
  if (pkg->info) {
    char *arch;

    get_fullname_parts(pkg, NULL, NULL, NULL, NULL, NULL, NULL, &arch, NULL);
    if (!strcmp(arch, "src"))
      RETVAL = 1;
    else {
      platform = rpmExpand(arch, "-%{_target_vendor}-%{_target_os}%{?_gnu}", NULL);
      RETVAL = rpmPlatformScore(platform, NULL, 0);
      _free(platform);
    }
    restore_chars();
  } else if (pkg->h) {
    if (headerIsEntry(pkg->h, RPMTAG_SOURCERPM)) {
      const char *arch = get_name(pkg->h, RPMTAG_ARCH);
      platform = rpmExpand(arch ? arch : "", "-%{_target_vendor}-%{_target_os}%{?_gnu}", NULL);
      RETVAL = rpmPlatformScore(platform, NULL, 0);
      _free(arch);
      _free(platform);
    } else
      RETVAL = 1;
  } else
    RETVAL = 0;
  OUTPUT:
  RETVAL

int
Pkg_is_platform_compat(pkg)
  URPM::Package pkg
  INIT:
  const char * platform = NULL;
  HE_t val = memset(alloca(sizeof(*val)), 0, sizeof(*val));

  CODE:
  read_config_files(0);
  RETVAL = 0;
  if (pkg->h && headerIsEntry(pkg->h, RPMTAG_PLATFORM)) {
    val->tag = RPMTAG_PLATFORM;
    if(headerGet(pkg->h, val, 0)) {
      platform = val->p.str;
      RETVAL = rpmPlatformScore(platform, NULL, 0);
      platform = _free(platform);
    }
  } else if (pkg->info) {
    char *arch;
    char *eos;

    get_fullname_parts(pkg, NULL, NULL, NULL, NULL, NULL, NULL, &arch, &eos);
    platform = rpmExpand(arch, "-%{_target_vendor}-", eos, "%{?_gnu}", NULL);
    RETVAL = rpmPlatformScore(platform, NULL, 0);
    restore_chars();
    _free(platform);
  }
  
  OUTPUT:
  RETVAL

void
Pkg_summary(pkg)
  URPM::Package pkg
  PPCODE:
  if (pkg->summary)
    push_utf8_name_only(pkg->summary, 0);
  else if (pkg->h)
   push_utf8_name(pkg, RPMTAG_SUMMARY);

void
Pkg_description(pkg)
  URPM::Package pkg
  PPCODE:
  if (pkg->h)
   push_utf8_name(pkg, RPMTAG_DESCRIPTION);

void
Pkg_sourcerpm(pkg)
  URPM::Package pkg
  PPCODE:
  if (pkg->h)
   push_name(pkg, RPMTAG_SOURCERPM);

void
Pkg_packager(pkg)
  URPM::Package pkg
  PPCODE:
  if (pkg->h)
   push_utf8_name(pkg, RPMTAG_PACKAGER);

void
Pkg_buildhost(pkg)
  URPM::Package pkg
  PPCODE:
  if (pkg->h)
   push_name(pkg, RPMTAG_BUILDHOST);

int
Pkg_buildtime(pkg)
  URPM::Package pkg
  CODE:
  if (pkg->h)
    RETVAL = get_int(pkg->h, RPMTAG_BUILDTIME);
  else
    RETVAL = 0;
  OUTPUT:
  RETVAL

int
Pkg_installtid(pkg)
  URPM::Package pkg
  CODE:
  if (pkg->h)
    RETVAL = get_int(pkg->h, RPMTAG_INSTALLTID);
  else
    RETVAL = 0;
  OUTPUT:
  RETVAL

void
Pkg_url(pkg)
  URPM::Package pkg
  PPCODE:
  if (pkg->h)
   push_name(pkg, RPMTAG_URL);

void
Pkg_license(pkg)
  URPM::Package pkg
  PPCODE:
  if (pkg->h)
   push_name(pkg, RPMTAG_LICENSE);

void
Pkg_distribution(pkg)
  URPM::Package pkg
  PPCODE:
  if (pkg->h)
   push_name(pkg, RPMTAG_DISTRIBUTION);

void
Pkg_vendor(pkg)
  URPM::Package pkg
  PPCODE:
  if (pkg->h)
   push_name(pkg, RPMTAG_VENDOR);

void
Pkg_os(pkg)
  URPM::Package pkg
  PPCODE:
  if (pkg->h)
   push_name(pkg, RPMTAG_OS);

void
Pkg_payload_format(pkg)
  URPM::Package pkg
  PPCODE:
  if (pkg->h)
   push_name(pkg, RPMTAG_PAYLOADFORMAT);

void
Pkg_evr(pkg)
  URPM::Package pkg
  PREINIT:
  const char *evr;
  PPCODE:
  evr = get_evr(pkg);
  push_name_only(evr, 0);
  restore_chars();

void
Pkg_fullname(pkg)
  URPM::Package pkg
  PREINIT:
  I32 gimme = GIMME_V;
  PPCODE:
  if (gimme == G_ARRAY) {
    char *name = NULL;
    char *version = NULL;
    char *release = NULL;
    char *disttag = NULL;
    char *distepoch = NULL;
    char *arch = NULL;
    char *eos = NULL;
    int items = 6;

    if (pkg->info)
      get_fullname_parts(pkg, &name, NULL, &version, &release, &disttag, &distepoch, &arch, &eos);
    else if (pkg->h) {
      name = (char*)get_name(pkg->h, RPMTAG_NAME);
      version = (char*)get_name(pkg->h, RPMTAG_VERSION);
      release = (char*)get_name(pkg->h, RPMTAG_RELEASE);
      disttag = (char*)get_name(pkg->h, RPMTAG_DISTTAG);
      distepoch = (char*)get_name(pkg->h, RPMTAG_DISTEPOCH);
      arch = (char*)get_name(pkg->h, RPMTAG_ARCH);
    }
    EXTEND(SP, items);
    PUSHs(sv_2mortal(name ? newSVpv(name, 0) : newSVpvs("")));
    PUSHs(sv_2mortal(version ? newSVpv(version, 0) : newSVpvs("")));
    PUSHs(sv_2mortal(release ? newSVpv(release, 0) : newSVpvs("")));
    PUSHs(sv_2mortal(disttag ? newSVpv(disttag, 0) : newSVpvs("")));
    PUSHs(sv_2mortal(distepoch ? newSVpv(distepoch, 0) : newSVpvs("")));
    PUSHs(sv_2mortal(arch ? newSVpv(arch, 0) : newSVpvs("")));
    if (pkg->info)
      restore_chars();
    else {
      _free(name);
      _free(version);
      _free(release);
      _free(disttag);
      _free(distepoch);
      _free(arch);
    }
  } else if (gimme == G_SCALAR) {
    if (pkg->info) {
      char *eos;
      if ((eos = strchr(pkg->info, '@')) != NULL)
	push_name_only(pkg->info, eos-pkg->info);
    } else if (pkg->h) {
      const char *nvra = get_nvra(pkg->h);
      push_name_only(nvra, 0);
      _free(nvra);
    }
  }

int
Pkg_epoch(pkg)
  URPM::Package pkg
  PREINIT:
  int epoch;
  CODE:
  if (pkg->info)
    get_fullname_parts(pkg, NULL, &epoch, NULL, NULL, NULL, NULL, NULL, NULL);
  else
    epoch = get_int(pkg->h, RPMTAG_EPOCH);
  RETVAL = epoch;
  OUTPUT:
  RETVAL

int
Pkg_compare_pkg(lpkg, rpkg)
  URPM::Package lpkg
  URPM::Package rpkg
  PREINIT:
  int compare = 0;
  char *levr = NULL;
  char *larch = NULL;
  char *revr = NULL;
  char *rarch = NULL;
  char *tmp = NULL;
  CODE:
  if (lpkg == rpkg) RETVAL = 0;
  else {
    tmp = (char*)get_evr(lpkg);
    levr = alloca(strlen(tmp)+1);
    stpcpy(levr, tmp);

    revr = (char*)get_evr(rpkg);
    if(revr == NULL) {
      restore_chars();
      croak("undefined package");
    }
    compare = do_rpmEVRcompare(levr, revr);
    restore_chars();
    if (!compare) {
      int lscore, rscore;
      char *platform = NULL;
      if (lpkg->info)
	get_fullname_parts(lpkg, NULL, NULL, NULL, NULL, NULL, NULL, &larch, NULL);
      else
	larch = (char*)get_name(lpkg->h, RPMTAG_ARCH);

      if (rpkg->info)
	get_fullname_parts(rpkg, NULL, NULL, NULL, NULL, NULL, NULL, &rarch, NULL);
      else
	rarch = (char*)get_name(rpkg->h, RPMTAG_ARCH);

      read_config_files(0);

      platform = rpmExpand(larch, "-%{_target_vendor}-%{_target_os}%{?_gnu}", NULL);
      lscore = rpmPlatformScore(platform, NULL, 0);
      platform = _free(platform);

      platform = rpmExpand(rarch, "-%{_target_vendor}-%{_target_os}%{?_gnu}", NULL);
      rscore = rpmPlatformScore(platform, NULL, 0);
      platform = _free(platform);

      if (lscore == 0) {
	if (rscore == 0)
#if 0
	  /* Nanar: TODO check this 
	   * hu ?? what is the goal of strcmp, some of arch are equivalent */
	  compare = 0
#endif
	    compare = (larch && rarch ? strcmp(larch, rarch) : 0);
	else
	  compare = -1;
      } else {
	if (rscore == 0)
	  compare = 1;
	else
	  compare = rscore - lscore; /* score are lower for better */
      }
    }
    if (!lpkg->info) larch = _free(larch);
    if (!rpkg->info) rarch = _free(rarch);
    restore_chars();
    RETVAL = compare;
  }
  OUTPUT:
  RETVAL

int
Pkg_compare(pkg, evr)
  URPM::Package pkg
  char *evr
  PREINIT:
  int compare = 0;
  EVR_t lEVR = rpmEVRnew(RPMSENSE_EQUAL, 0),
        rEVR = rpmEVRnew(RPMSENSE_EQUAL, 0);
  CODE:
  if (!compare) {
    int i;

    /* This will remove fields from _evr (from the right) that evr is missing
     * so that ie. if only version is given as an argument, it won't compare
     * release etc.
     */
    rpmEVRparse(get_evr(pkg), lEVR);
    restore_chars();
    rpmEVRparse(evr, rEVR);
    for(i = RPMEVR_V; i <= RPMEVR_D; i++)
      if(!*(rEVR->F[i]))
	lEVR->F[i] = "";

    compare = rpmEVRcompare(lEVR, rEVR);
    rpmEVRfree(lEVR);
    rpmEVRfree(rEVR);
  }
  RETVAL = compare;
  OUTPUT:
  RETVAL

int
Pkg_size(pkg)
  URPM::Package pkg
  CODE:
  if (pkg->info) {
    char *s;

    if ((s = strchr(pkg->info, '@')) != NULL && (s = strchr(s+1, '@')) != NULL)
      RETVAL = atoi(s+1);
    else
      RETVAL = 0;
  } else if (pkg->h)
    RETVAL = get_int(pkg->h, RPMTAG_SIZE);
    else
    RETVAL = 0;
  OUTPUT:
  RETVAL

int
Pkg_filesize(pkg)
  URPM::Package pkg
  CODE:
  if (pkg->filesize)
    RETVAL = pkg->filesize;
  else if (pkg->h)
    RETVAL = get_filesize(pkg->h);
  else RETVAL = 0;
  OUTPUT:
  RETVAL

void
Pkg_group(pkg)
  URPM::Package pkg
  PPCODE:
  if (pkg->info) {
    char *s;

    if ((s = strchr(pkg->info, '@')) != NULL && (s = strchr(s+1, '@')) != NULL && (s = strchr(s+1, '@')) != NULL) {
      char *eos = strchr(s+1, '@');
      push_utf8_name_only(s+1, eos != NULL ? eos-s-1 : 0);
    }
  } else if (pkg->h)
    push_utf8_name(pkg, RPMTAG_GROUP);

void
Pkg_filename(pkg)
  URPM::Package pkg
  PPCODE:
  if (pkg->info) {
    char *eon;
    size_t len;

    len = strlen(pkg->info);

    if (len > 5 && !strcmp(&pkg->info[len-4], ".rpm") && (eon = strrchr(pkg->info, '@')) != NULL)
      push_name_only(++eon, 0);
    else if((eon = strchr(pkg->info, '@')) != NULL && (len = eon - pkg->info) > 0) {
      char filename[len + sizeof(".rpm")];
      char *buf = filename;
      memset(filename, 0, len+sizeof("rpm"));
      strncat(filename, pkg->info, len);
      stpcpy(&filename[len], ".rpm");
      push_name_only(buf, 0);
    }
  } else if (pkg->h) {
    const char *nvra = get_nvra(pkg->h);
    mXPUSHs(newSVpvf("%s.rpm", nvra));
    _free(nvra);
  }

void
Pkg_id(pkg)
  URPM::Package pkg
  PPCODE:
  if ((pkg->flag & FLAG_ID) <= FLAG_ID_MAX)
    mXPUSHs(newSViv(pkg->flag & FLAG_ID));

void
Pkg_set_id(pkg, id=-1)
  URPM::Package pkg
  int id
  PPCODE:
  if ((pkg->flag & FLAG_ID) <= FLAG_ID_MAX)
    mXPUSHs(newSViv(pkg->flag & FLAG_ID));
  pkg->flag &= ~FLAG_ID;
  pkg->flag |= id >= 0 && id <= FLAG_ID_MAX ? id : FLAG_ID_INVALID;

void
Pkg_requires(pkg)
  URPM::Package pkg
  PPCODE:
  PUTBACK;
  return_list_str(pkg->requires, pkg->h, RPMTAG_REQUIRENAME, RPMTAG_REQUIREFLAGS, RPMTAG_REQUIREVERSION,
		  callback_list_str_xpush_requires, NULL);
  SPAGAIN;

void
Pkg_requires_nosense(pkg)
  URPM::Package pkg
  PPCODE:
  PUTBACK;
  return_list_str(pkg->requires, pkg->h, RPMTAG_REQUIRENAME, RPMTAG_REQUIREFLAGS, 0, 
		  callback_list_str_xpush_requires, NULL);
  SPAGAIN;

void
Pkg_suggests(pkg)
  URPM::Package pkg
  PPCODE:
  PUTBACK;
  int count = return_list_str(pkg->suggests, pkg->h, RPMTAG_SUGGESTSNAME, RPMTAG_SUGGESTSFLAGS, RPMTAG_SUGGESTSVERSION, callback_list_str_xpush, NULL);
  if (count == 0)
    return_list_str(pkg->suggests, pkg->h, RPMTAG_REQUIRENAME, RPMTAG_REQUIREFLAGS, 0,
		    callback_list_str_xpush_old_suggests, NULL);
  SPAGAIN;

void
Pkg_obsoletes(pkg)
  URPM::Package pkg
  PPCODE:
  PUTBACK;
  return_list_str(pkg->obsoletes, pkg->h, RPMTAG_OBSOLETENAME, RPMTAG_OBSOLETEFLAGS, RPMTAG_OBSOLETEVERSION,
		  callback_list_str_xpush, NULL);
  SPAGAIN;

void
Pkg_obsoletes_nosense(pkg)
  URPM::Package pkg
  PPCODE:
  PUTBACK;
  return_list_str(pkg->obsoletes, pkg->h, RPMTAG_OBSOLETENAME, 0, 0, callback_list_str_xpush, NULL);
  SPAGAIN;

int
Pkg_obsoletes_overlap(pkg, s, direction=-1)
  URPM::Package pkg
  char *s
  int direction
  PREINIT:
  struct cb_overlap_s os;
  char *eon = NULL;
  char eonc = '\0';
  CODE:
  os.name = s;
  os.flags = 0;
  while (*s && *s != ' ' && *s != '[' && *s != '<' && *s != '>' && *s != '=') ++s;
  if (*s) {
    eon = s;
    while (*s) {
      if (*s == ' ' || *s == '[' || *s == '*' || *s == ']');
      else if (*s == '<') os.flags |= RPMSENSE_LESS;
      else if (*s == '>') os.flags |= RPMSENSE_GREATER;
      else if (*s == '=') os.flags |= RPMSENSE_EQUAL;
      else break;
      ++s;
    }
    os.evr = s;
  } else
    os.evr = "";
  os.direction = direction;
  /* mark end of name */
  if (eon) { eonc = *eon; *eon = 0; }
  /* return_list_str returns a negative value is the callback has returned non-zero */
  RETVAL = return_list_str(pkg->obsoletes, pkg->h, RPMTAG_OBSOLETENAME, RPMTAG_OBSOLETEFLAGS, RPMTAG_OBSOLETEVERSION,
			   callback_list_str_overlap, &os) < 0;
  /* restore end of name */
  if (eon) *eon = eonc;
  OUTPUT:
  RETVAL

void
Pkg_conflicts(pkg)
  URPM::Package pkg
  PPCODE:
  PUTBACK;
  return_list_str(pkg->conflicts, pkg->h, RPMTAG_CONFLICTNAME, RPMTAG_CONFLICTFLAGS, RPMTAG_CONFLICTVERSION,
		  callback_list_str_xpush, NULL);
  SPAGAIN;

void
Pkg_conflicts_nosense(pkg)
  URPM::Package pkg
  PPCODE:
  PUTBACK;
  return_list_str(pkg->conflicts, pkg->h, RPMTAG_CONFLICTNAME, 0, 0, callback_list_str_xpush, NULL);
  SPAGAIN;

void
Pkg_provides(pkg)
  URPM::Package pkg
  PPCODE:
  PUTBACK;
  return_list_str(pkg->provides, pkg->h, RPMTAG_PROVIDENAME, RPMTAG_PROVIDEFLAGS, RPMTAG_PROVIDEVERSION,
		  callback_list_str_xpush, NULL);
  SPAGAIN;

void
Pkg_provides_nosense(pkg)
  URPM::Package pkg
  PPCODE:
  PUTBACK;
  return_list_str(pkg->provides, pkg->h, RPMTAG_PROVIDENAME, 0, 0, callback_list_str_xpush, NULL);
  SPAGAIN;

int
Pkg_provides_overlap(pkg, s, direction=1)
  URPM::Package pkg
  char *s
  int direction
  PREINIT:
  struct cb_overlap_s os;
  char *eon = NULL;
  char eonc = '\0';
  CODE:
  os.name = s;
  os.flags = 0;
  while (*s && *s != ' ' && *s != '[' && *s != '<' && *s != '>' && *s != '=') ++s;
  if (*s) {
    eon = s;
    while (*s) {
      if (*s == ' ' || *s == '[' || *s == '*' || *s == ']');
      else if (*s == '<') os.flags |= RPMSENSE_LESS;
      else if (*s == '>') os.flags |= RPMSENSE_GREATER;
      else if (*s == '=') os.flags |= RPMSENSE_EQUAL;
      else break;
      ++s;
    }
    os.evr = s;
  } else
    os.evr = "";
  os.direction = direction;
  /* mark end of name */
  if (eon) { eonc = *eon; *eon = 0; }
  /* return_list_str returns a negative value is the callback has returned non-zero */
  RETVAL = return_list_str(pkg->provides, pkg->h, RPMTAG_PROVIDENAME, RPMTAG_PROVIDEFLAGS, RPMTAG_PROVIDEVERSION,
			   callback_list_str_overlap, &os) < 0;
  /* restore end of name */
  if (eon) *eon = eonc;
  OUTPUT:
  RETVAL

void
Pkg_buildarchs(pkg)
  URPM::Package pkg
  PPCODE:
  PUTBACK;
  xpush_simple_list_str(pkg->h, RPMTAG_BUILDARCHS);
  SPAGAIN;
  
void
Pkg_excludearchs(pkg)
  URPM::Package pkg
  PPCODE:
  PUTBACK;
  xpush_simple_list_str(pkg->h, RPMTAG_EXCLUDEARCH);
  SPAGAIN;
  
void
Pkg_exclusivearchs(pkg)
  URPM::Package pkg
  PPCODE:
  PUTBACK;
  xpush_simple_list_str(pkg->h, RPMTAG_EXCLUSIVEARCH);
  SPAGAIN;
  
void
Pkg_dirnames(pkg)
  URPM::Package pkg
  PPCODE:
  PUTBACK;
  xpush_simple_list_str(pkg->h, RPMTAG_DIRNAMES);
  SPAGAIN;

void
Pkg_filelinktos(pkg)
  URPM::Package pkg
  PPCODE:
  PUTBACK;
  xpush_simple_list_str(pkg->h, RPMTAG_FILELINKTOS);
  SPAGAIN;

void
Pkg_files(pkg)
  URPM::Package pkg
  PPCODE:
  PUTBACK;
  return_files(pkg->h, 0);
  SPAGAIN;

void
Pkg_files_digest(pkg)
  URPM::Package pkg
  PPCODE:
  PUTBACK;
  xpush_simple_list_str(pkg->h, RPMTAG_FILEDIGESTS);
  SPAGAIN;

void
Pkg_files_md5sum(pkg)
  PPCODE:
  croak("files_md5sum() is dead. use files_digest() instead");

void
Pkg_files_owner(pkg)
  URPM::Package pkg
  PPCODE:
  PUTBACK;
  xpush_simple_list_str(pkg->h, RPMTAG_FILEUSERNAME);
  SPAGAIN;

void
Pkg_files_group(pkg)
  URPM::Package pkg
  PPCODE:
  PUTBACK;
  xpush_simple_list_str(pkg->h, RPMTAG_FILEGROUPNAME);
  SPAGAIN;

void
Pkg_files_mtime(pkg)
  URPM::Package pkg
  PPCODE:
  PUTBACK;
  return_list_uint32_t(pkg->h, RPMTAG_FILEMTIMES);
  SPAGAIN;

void
Pkg_files_size(pkg)
  URPM::Package pkg
  PPCODE:
  PUTBACK;
  return_list_uint32_t(pkg->h, RPMTAG_FILESIZES);
  SPAGAIN;

void
Pkg_files_uid(pkg)
  URPM::Package pkg
  PPCODE:
  PUTBACK;
  return_list_uint32_t(pkg->h, RPMTAG_FILEUIDS);
  SPAGAIN;

void
Pkg_files_gid(pkg)
  URPM::Package pkg
  PPCODE:
  PUTBACK;
  return_list_uint32_t(pkg->h, RPMTAG_FILEGIDS);
  SPAGAIN;

void
Pkg_files_mode(pkg)
  URPM::Package pkg
  PPCODE:
  PUTBACK;
  return_list_uint_16(pkg->h, RPMTAG_FILEMODES);
  SPAGAIN;

void
Pkg_files_flags(pkg)
  URPM::Package pkg
  PPCODE:
  PUTBACK;
  return_list_uint32_t(pkg->h, RPMTAG_FILEFLAGS);
  SPAGAIN;
  
void
Pkg_conf_files(pkg)
  URPM::Package pkg
  PPCODE:
  PUTBACK;
  return_files(pkg->h, FILTER_MODE_CONF_FILES);
  SPAGAIN;

void
Pkg_changelog_time(pkg)
  URPM::Package pkg
  PPCODE:
  PUTBACK;
  return_list_uint32_t(pkg->h, RPMTAG_CHANGELOGTIME);
  SPAGAIN;

void
Pkg_changelog_name(pkg)
  URPM::Package pkg
  PPCODE:
  PUTBACK;
  xpush_simple_list_str(pkg->h, RPMTAG_CHANGELOGNAME);
  SPAGAIN;

void
Pkg_changelog_text(pkg)
  URPM::Package pkg
  PPCODE:
  PUTBACK;
  xpush_simple_list_str(pkg->h, RPMTAG_CHANGELOGTEXT);
  SPAGAIN;

void
Pkg_queryformat(pkg, fmt)
  URPM::Package pkg
  char *fmt
  PREINIT:
  char *s;
  PPCODE:
  if (pkg->h) {
    s = headerSprintf(pkg->h, fmt, NULL, NULL, NULL);
    if (s)
      push_utf8_name_only(s, 0);
    _free(s);
  }
  
void
Pkg_get_tag(pkg, tagname)
  URPM::Package pkg
  char *tagname
  PPCODE:
  PUTBACK;
  return_list_tag(pkg, tagname);
  SPAGAIN;

void
Pkg_get_tag_modifiers(pkg, tagname)
  URPM::Package pkg
  char *tagname
  PPCODE:
  PUTBACK;
  return_list_tag_modifier(pkg->h, tagname);
  SPAGAIN;
  
void
Pkg_pack_header(pkg)
  URPM::Package pkg
  CODE:
  pack_header(pkg);

int
Pkg_update_header(pkg, filename, ...)
  URPM::Package pkg
  char *filename
  PREINIT:
  int packing = 0;
  int keep_all_tags = 0;
  CODE:
  /* compability mode with older interface of parse_hdlist */
  if (items == 3)
    packing = SvIV(ST(2));
  else if (items > 3) {
    int i;
    for (i = 2; i < items-1; i+=2) {
      STRLEN len;
      char *s = SvPV(ST(i), len);

      if (len == 7 && !memcmp(s, "packing", 7))
	packing = SvTRUE(ST(i + 1));
      else if (len == 13 && !memcmp(s, "keep_all_tags", 13))
	keep_all_tags = SvTRUE(ST(i+1));
    }
  }
  RETVAL = update_header(filename, pkg, !packing && keep_all_tags, RPMVSF_DEFAULT);
  if (RETVAL && packing) pack_header(pkg);
  OUTPUT:
  RETVAL

void
Pkg_free_header(pkg)
  URPM::Package pkg
  CODE:
  if (pkg->h && !(pkg->flag & FLAG_NO_HEADER_FREE)) pkg->h = headerFree(pkg->h);
  pkg->h = NULL;

void
Pkg_build_info(pkg, fileno, provides_files=NULL)
  URPM::Package pkg
  int fileno
  char *provides_files
  CODE:
  if (pkg->info) {
    char buff[8*BUFSIZ];
    size_t size;

    /* info line should be the last to be written */
    if (pkg->provides && *pkg->provides) {
      size = snprintf(buff, sizeof(buff), "@provides@%s\n", pkg->provides);
      if (size < sizeof(buff)) {
	if (provides_files && *provides_files) {
	  --size;
	  size += snprintf(buff+size, sizeof(buff)-size, "@%s\n", provides_files);
	}
	write_nocheck(fileno, buff, size);
      }
    }
    if (pkg->conflicts && *pkg->conflicts) {
      size = snprintf(buff, sizeof(buff), "@conflicts@%s\n", pkg->conflicts);
      if (size < sizeof(buff)) write_nocheck(fileno, buff, size);
    }
    if (pkg->obsoletes && *pkg->obsoletes) {
      size = snprintf(buff, sizeof(buff), "@obsoletes@%s\n", pkg->obsoletes);
      if (size < sizeof(buff)) write_nocheck(fileno, buff, size);
    }
    if (pkg->requires && *pkg->requires) {
      size = snprintf(buff, sizeof(buff), "@requires@%s\n", pkg->requires);
      if (size < sizeof(buff)) write_nocheck(fileno, buff, size);
    }
    if (pkg->suggests && *pkg->suggests) {
      size = snprintf(buff, sizeof(buff), "@suggests@%s\n", pkg->suggests);
      if (size < sizeof(buff)) write_nocheck(fileno, buff, size);
    }
    if (pkg->summary && *pkg->summary) {
      size = snprintf(buff, sizeof(buff), "@summary@%s\n", pkg->summary);
      if (size < sizeof(buff)) write_nocheck(fileno, buff, size);
    }
    if (pkg->filesize) {
      size = snprintf(buff, sizeof(buff), "@filesize@%d\n", pkg->filesize);
      if (size < sizeof(buff)) write_nocheck(fileno, buff, size);
    }
    size = snprintf(buff, sizeof(buff), "@info@%s\n", pkg->info);
    write_nocheck(fileno, buff, size);
  } else croak("no info available for package %s",
	  pkg->h ? get_name(pkg->h, RPMTAG_NAME) : "-");

void
Pkg_build_header(pkg, fileno)
  URPM::Package pkg
  int fileno
  CODE:
  if (pkg->h) {
    FD_t fd;

    if ((fd = fdDup(fileno)) != NULL) {
      const char item[] = "Header";
      const char * msg = NULL;
      rpmRC rc = rpmpkgWrite(item, fd, pkg->h, &msg);
      if (rc != RPMRC_OK) {
	rpmlog(RPMLOG_ERR, "%s: %s: %s\n", "rpmkpgWrite", item, msg);
	rc = RPMRC_FAIL;
      }
      msg = (const char*)_free(msg);
      Fclose(fd);
    } else croak("unable to get rpmio handle on fileno %d", fileno);
  } else croak("no header available for package");

int
Pkg_flag(pkg, name)
  URPM::Package pkg
  char *name
  PREINIT:
  unsigned mask;
  CODE:
  if (!strcmp(name, "skip")) mask = FLAG_SKIP;
  else if (!strcmp(name, "disable_obsolete")) mask = FLAG_DISABLE_OBSOLETE;
  else if (!strcmp(name, "installed")) mask = FLAG_INSTALLED;
  else if (!strcmp(name, "requested")) mask = FLAG_REQUESTED;
  else if (!strcmp(name, "required")) mask = FLAG_REQUIRED;
  else if (!strcmp(name, "upgrade")) mask = FLAG_UPGRADE;
  else croak("unknown flag: %s", name);
  RETVAL = pkg->flag & mask;
  OUTPUT:
  RETVAL

int
Pkg_set_flag(pkg, name, value=1)
  URPM::Package pkg
  char *name
  int value
  PREINIT:
  unsigned mask;
  CODE:
  if (!strcmp(name, "skip")) mask = FLAG_SKIP;
  else if (!strcmp(name, "disable_obsolete")) mask = FLAG_DISABLE_OBSOLETE;
  else if (!strcmp(name, "installed")) mask = FLAG_INSTALLED;
  else if (!strcmp(name, "requested")) mask = FLAG_REQUESTED;
  else if (!strcmp(name, "required")) mask = FLAG_REQUIRED;
  else if (!strcmp(name, "upgrade")) mask = FLAG_UPGRADE;
  else croak("unknown flag: %s", name);
  RETVAL = pkg->flag & mask;
  if (value) pkg->flag |= mask;
  else       pkg->flag &= ~mask;
  OUTPUT:
  RETVAL

int
Pkg_flag_skip(pkg)
  URPM::Package pkg
  CODE:
  RETVAL = pkg->flag & FLAG_SKIP;
  OUTPUT:
  RETVAL

int
Pkg_set_flag_skip(pkg, value=1)
  URPM::Package pkg
  int value
  CODE:
  RETVAL = pkg->flag & FLAG_SKIP;
  if (value) pkg->flag |= FLAG_SKIP;
  else       pkg->flag &= ~FLAG_SKIP;
  OUTPUT:
  RETVAL

int
Pkg_flag_base(pkg)
  URPM::Package pkg
  CODE:
  RETVAL = pkg->flag & FLAG_BASE;
  OUTPUT:
  RETVAL

int
Pkg_set_flag_base(pkg, value=1)
  URPM::Package pkg
  int value
  CODE:
  RETVAL = pkg->flag & FLAG_BASE;
  if (value) pkg->flag |= FLAG_BASE;
  else       pkg->flag &= ~FLAG_BASE;
  OUTPUT:
  RETVAL

int
Pkg_flag_disable_obsolete(pkg)
  URPM::Package pkg
  CODE:
  RETVAL = pkg->flag & FLAG_DISABLE_OBSOLETE;
  OUTPUT:
  RETVAL

int
Pkg_set_flag_disable_obsolete(pkg, value=1)
  URPM::Package pkg
  int value
  CODE:
  RETVAL = pkg->flag & FLAG_DISABLE_OBSOLETE;
  if (value) pkg->flag |= FLAG_DISABLE_OBSOLETE;
  else       pkg->flag &= ~FLAG_DISABLE_OBSOLETE;
  OUTPUT:
  RETVAL

int
Pkg_flag_installed(pkg)
  URPM::Package pkg
  CODE:
  RETVAL = pkg->flag & FLAG_INSTALLED;
  OUTPUT:
  RETVAL

int
Pkg_set_flag_installed(pkg, value=1)
  URPM::Package pkg
  int value
  CODE:
  RETVAL = pkg->flag & FLAG_INSTALLED;
  if (value) pkg->flag |= FLAG_INSTALLED;
  else       pkg->flag &= ~FLAG_INSTALLED;
  OUTPUT:
  RETVAL

int
Pkg_flag_requested(pkg)
  URPM::Package pkg
  CODE:
  RETVAL = pkg->flag & FLAG_REQUESTED;
  OUTPUT:
  RETVAL

int
Pkg_set_flag_requested(pkg, value=1)
  URPM::Package pkg
  int value
  CODE:
  RETVAL = pkg->flag & FLAG_REQUESTED;
  if (value) pkg->flag |= FLAG_REQUESTED;
  else       pkg->flag &= ~FLAG_REQUESTED;
  OUTPUT:
  RETVAL

int
Pkg_flag_required(pkg)
  URPM::Package pkg
  CODE:
  RETVAL = pkg->flag & FLAG_REQUIRED;
  OUTPUT:
  RETVAL

int
Pkg_set_flag_required(pkg, value=1)
  URPM::Package pkg
  int value
  CODE:
  RETVAL = pkg->flag & FLAG_REQUIRED;
  if (value) pkg->flag |= FLAG_REQUIRED;
  else       pkg->flag &= ~FLAG_REQUIRED;
  OUTPUT:
  RETVAL

int
Pkg_flag_upgrade(pkg)
  URPM::Package pkg
  CODE:
  RETVAL = pkg->flag & FLAG_UPGRADE;
  OUTPUT:
  RETVAL

int
Pkg_set_flag_upgrade(pkg, value=1)
  URPM::Package pkg
  int value
  CODE:
  RETVAL = pkg->flag & FLAG_UPGRADE;
  if (value) pkg->flag |= FLAG_UPGRADE;
  else       pkg->flag &= ~FLAG_UPGRADE;
  OUTPUT:
  RETVAL

int
Pkg_flag_selected(pkg)
  URPM::Package pkg
  CODE:
  RETVAL = pkg->flag & FLAG_UPGRADE ? pkg->flag & (FLAG_BASE | FLAG_REQUIRED) : 0;
  OUTPUT:
  RETVAL

int
Pkg_flag_available(pkg)
  URPM::Package pkg
  CODE:
  RETVAL = (pkg->flag & FLAG_INSTALLED && !(pkg->flag & FLAG_UPGRADE)) ||
           (pkg->flag & FLAG_UPGRADE ? pkg->flag & (FLAG_BASE | FLAG_REQUIRED) : 0);
  OUTPUT:
  RETVAL

int
Pkg_rate(pkg)
  URPM::Package pkg
  CODE:
  RETVAL = (pkg->flag & FLAG_RATE) >> FLAG_RATE_POS;
  OUTPUT:
  RETVAL

int
Pkg_set_rate(pkg, rate)
  URPM::Package pkg
  int rate
  CODE:
  RETVAL = (pkg->flag & FLAG_RATE) >> FLAG_RATE_POS;
  pkg->flag &= ~FLAG_RATE;
  pkg->flag |= (rate >= 0 && rate <= FLAG_RATE_MAX ? rate : FLAG_RATE_INVALID) << FLAG_RATE_POS;
  OUTPUT:
  RETVAL

void
Pkg_rflags(pkg)
  URPM::Package pkg
  PREINIT:
  I32 gimme = GIMME_V;
  PPCODE:
  if (gimme == G_ARRAY && pkg->rflags != NULL) {
    char *s = pkg->rflags;
    char *eos;
    while ((eos = strchr(s, '\t')) != NULL) {
      push_name_only(s, eos-s);
      s = ++eos;
    }
    push_name_only(s, 0);
  }

void
Pkg_set_rflags(pkg, ...)
  URPM::Package pkg
  PREINIT:
  I32 gimme = GIMME_V;
  char *new_rflags;
  STRLEN total_len;
  int i;
  PPCODE:
  total_len = 0;
  for (i = 1; i < items; ++i)
    total_len += SvCUR(ST(i)) + 1;

  new_rflags = malloc(total_len);
  total_len = 0;
  for (i = 1; i < items; ++i) {
    STRLEN len;
    char *s = SvPV(ST(i), len);
    memcpy(new_rflags + total_len, s, len);
    new_rflags[total_len + len] = '\t';
    total_len += len + 1;
  }
  new_rflags[total_len - 1] = 0; /* but mark end-of-string correctly */

  if (gimme == G_ARRAY && pkg->rflags != NULL) {
    char *s = pkg->rflags;
    char *eos;
    while ((eos = strchr(s, '\t')) != NULL) {
      push_name_only(s, eos-s);
      s = eos + 1;
    }
    push_name_only(s, 0);
  }

  free(pkg->rflags);
  pkg->rflags = new_rflags;


MODULE = URPM            PACKAGE = URPM::DB            PREFIX = Db_

URPM::DB
Db_open(prefix=NULL, write_perm=0, log_auto_remove=1)
  char *prefix
  int write_perm
  int log_auto_remove
  PREINIT:
  URPM__DB db;
  CODE:
  read_config_files(0);
  db = malloc(sizeof(struct s_Transaction));
  db->count = 1;
  db->ts = rpmtsCreate();
  if(prefix && prefix[0] && prefix[0] != '/') {
    char relpath[PATH_MAX];
    size_t len;

    if(getcwd(relpath, sizeof(relpath)) == NULL)
      croak("%s", strerror(errno));
    len = strlen(relpath);
    snprintf(&(relpath[len]), sizeof(relpath)-len, "/%s", prefix);
    rpmtsSetRootDir(db->ts, relpath);
  } else
    rpmtsSetRootDir(db->ts, prefix && prefix[0] ? prefix : NULL);
  if (rpmtsOpenDB(db->ts, write_perm ? O_RDWR | O_CREAT : O_RDONLY) == 0) {
    if(write_perm) {
      rpmdb rdb = rpmtsGetRdb(db->ts);
      DB_ENV *dbenv = rdb->db_dbenv;

      if (dbenv == NULL) {
	(void)rpmtsFree(db->ts);
	croak("unable to open rpmdb in read/write mode, write permissions missing?");
      }

      /* TODO: allow for user configuration? */
      if(log_auto_remove)
	dbenv->log_set_config(dbenv, DB_LOG_AUTO_REMOVE, 1);
    }
    RETVAL = db;
  } else {
    RETVAL = NULL;
    (void)rpmtsFree(db->ts);
    free(db);
  }
  OUTPUT:
  RETVAL

void
Db_info(prefix=NULL)
  char *prefix
  PREINIT:
  int xx, empty = 1;
  const char *dbpath = NULL;
  struct stat sb;
  PPCODE:
  read_config_files(0);
  dbpath = rpmGetPath(prefix ? prefix : "", "%{?_dbpath}", "/", "Packages", NULL);
  if (Stat(dbpath, &sb) >= 0) {
    DB_ENV *dbenv = NULL;
    DB *bdb = NULL;
    DBC *dbcp = NULL;
    DBT key, data;

    if ((xx = db_env_create(&dbenv, 0)) != 0) {
      fprintf(stderr, "db_env_create: %s\n", db_strerror(xx));
      return;
    }
    if ((xx = dbenv->open(dbenv, dbpath,
	    DB_JOINENV | DB_USE_ENVIRON, 0)) != 0 &&
	(xx = dbenv->open(dbenv, dbpath,
			  DB_CREATE | DB_INIT_MPOOL | DB_PRIVATE | DB_USE_ENVIRON, 0)) != 0)
      dbenv->err(dbenv, xx, "DB_ENV->open");
    else {
      if ((xx = db_create(&bdb, dbenv, 0)) != 0)
	dbenv->err(dbenv, xx, "db_create");
      else {
	if ((xx = bdb->open(bdb, NULL, dbpath, NULL, DB_UNKNOWN, DB_RDONLY, 0)))
	  dbenv->err(dbenv, xx, "DB->open", bdb);
	else {

	  switch(bdb->type) {
	    case DB_BTREE:
	      mXPUSHs(newSVpvs("btree"));
	      break;
	    case DB_RECNO:
	      mXPUSHs(newSVpvs("recno"));
	      break;
	    case DB_HASH:
	      mXPUSHs(newSVpvs("hash"));
	      break;
	    case DB_QUEUE:
	      mXPUSHs(newSVpvs("queue"));
	      break;
	    case DB_UNKNOWN:
	    default:
	      XPUSHs(&PL_sv_undef);
	      break;
	  }
	  /* Acquire a cursor for the database. */
	  if ((xx = bdb->cursor(bdb, NULL, &dbcp, 0)) != 0) {
	    bdb->err(bdb, xx, "DB->cursor");
	    empty = 2;
	  }
	  if (empty != 2) {
	    /* Re-initialize the key/data pair. */
	    memset(&key, 0, sizeof(key));
	    memset(&data, 0, sizeof(data));

	    while ((xx = dbcp->c_get(dbcp, &key, &data, DB_NEXT)) == 0) {
	      if (!*(uint32_t*)key.data)
		continue;
	      if (htole32(*(uint32_t*)key.data) > 10000000)
		mXPUSHs(newSVpvs("bigendian"));
	      else
		mXPUSHs(newSVpvs("littleendian"));
	      empty = 0;
	      break;
	    }
	  }
	}
	if ((xx = bdb->close(bdb, DB_NOSYNC)) != 0)
	  dbenv->err(dbenv, xx, "DB_ENV->close");
      }
      if ((xx = dbenv->close(dbenv, 0)) != 0)
	fprintf(stderr, "dbenv->close: %s\n", db_strerror(xx));
    }

    if (empty)
      XPUSHs(&PL_sv_undef);
  }
  _free(dbpath);

int
Db_convert(prefix=NULL, dbtype=NULL, swap=0, rebuild=0)
  char *prefix
  char *dbtype
  int swap  
  int rebuild
  PREINIT:
  int type = 0;
  CODE:
  if(dbtype && !strcmp(dbtype, "hash"))
    type = 1;
  else if(dbtype && !strcmp(dbtype, "btree"))
    type = 0;
  else if(dbtype)
    croak("Unsupported database type: %s\n", dbtype);
  RETVAL = rpmdb_convert(prefix, type, swap, rebuild) == 0;
  OUTPUT:
  RETVAL

int
Db_rebuild(prefix=NULL)
  char *prefix
  PREINIT:
  rpmts ts;
  rpmVSFlags vsflags;
  CODE:
  read_config_files(0);
  ts = rpmtsCreate();
  vsflags = rpmExpandNumeric("%{_vsflags_rebuilddb}");
  if (rpmcliQueryFlags & VERIFY_DIGEST)
    vsflags |= _RPMVSF_NODIGESTS;
  if (rpmcliQueryFlags & VERIFY_SIGNATURE)
    vsflags |= _RPMVSF_NOSIGNATURES;

  rpmtsSetVSFlags(ts, vsflags);
  rpmtsSetRootDir(ts, prefix);
  RETVAL = rpmtsRebuildDB(ts) == 0;
  (void)rpmtsFree(ts);
  OUTPUT:
  RETVAL

int
Db_verify(prefix=NULL)
  char *prefix
  PREINIT:
  rpmts ts;
  CODE:
  ts = rpmtsCreate();
  rpmtsSetRootDir(ts, prefix);
  RETVAL = rpmtsVerifyDB(ts) == 0;
  ts = rpmtsFree(ts);
  OUTPUT:
  RETVAL

void
Db_DESTROY(db)
  URPM::DB db
  CODE:
  (void)rpmtsFree(db->ts);
  if (!--db->count) free(db);

void
Db_archive(db, remove=0, data=0, log=0, abs=1)
  URPM::DB db
  int remove
  int data
  int log
  int abs
  PREINIT:
  char **list = NULL;
  uint32_t flags = 0;
  int ret;
  DB_ENV *dbenv;
  PPCODE:
  dbenv  = rpmtsGetRdb(db->ts)->db_dbenv;

  if(remove)
    flags |= DB_ARCH_REMOVE;
  if(data)
    flags |= DB_ARCH_DATA;
  if(log)
    flags |= DB_ARCH_LOG;
  if(abs)
    flags |= DB_ARCH_ABS;
  if (!(ret = bdb_log_archive(dbenv, &list, flags))) {
    if(list) {
      char **p;
      for(p = list; *p != NULL; p++)
	push_name_only(*p, 0);
      free(list);
    }
  } else
    croak("%s", db_strerror(ret));

int
Db_traverse(db,callback)
  URPM::DB db
  SV *callback
  PREINIT:
  Header header;
  rpmmi mi;
  int count = 0;
  CODE:
  db->ts = rpmtsLink(db->ts, "URPM::DB::traverse");
  ts_nosignature(db->ts);
  mi = rpmtsInitIterator(db->ts, RPMDBI_PACKAGES, NULL, 0);
  while ((header = rpmmiNext(mi))) {
    if (SvROK(callback)) {
      dSP;
      URPM__Package pkg = calloc(1, sizeof(struct s_Package));

      pkg->flag = FLAG_ID_INVALID | FLAG_NO_HEADER_FREE;
      pkg->h = header;

      PUSHMARK(SP);
      mXPUSHs(sv_setref_pv(newSVpvs(""), "URPM::Package", pkg));
      PUTBACK;

      call_sv(callback, G_DISCARD | G_SCALAR);

      SPAGAIN;
      pkg->h = NULL; /* avoid using it anymore, in case it has been copied inside callback */
    }
    ++count;
  }
  mi = rpmmiFree(mi);
  (void)rpmtsFree(db->ts);
  RETVAL = count;
  OUTPUT:
  RETVAL

int
Db_traverse_tag(db,tag,names,callback)
  URPM::DB db
  char *tag
  SV *names
  SV *callback
  PREINIT:
  Header header;
  rpmmi mi;
  int count = 0;
  CODE:
  if (SvROK(names) && SvTYPE(SvRV(names)) == SVt_PVAV) {
    AV* names_av = (AV*)SvRV(names);
    int len = av_len(names_av);
    int i;
    rpmTag rpmtag;

    rpmtag = rpmtag_from_string(tag);

    for (i = 0; i <= len; ++i) {
      STRLEN str_len;
      SV **isv = av_fetch(names_av, i, 0);
      char *name = SvPV(*isv, str_len);
      db->ts = rpmtsLink(db->ts, "URPM::DB::traverse_tag");
      ts_nosignature(db->ts);
      mi = rpmtsInitIterator(db->ts, rpmtag, name, str_len);
      while ((header = rpmmiNext(mi))) {
	if (SvROK(callback)) {
	  dSP;
	  URPM__Package pkg = calloc(1, sizeof(struct s_Package));

	  pkg->flag = FLAG_ID_INVALID | FLAG_NO_HEADER_FREE;
	  pkg->h = header;

	  PUSHMARK(SP);
	  mXPUSHs(sv_setref_pv(newSVpvs(""), "URPM::Package", pkg));
	  PUTBACK;

	  call_sv(callback, G_DISCARD | G_SCALAR);

	  SPAGAIN;
	  pkg->h = NULL; /* avoid using it anymore, in case it has been copied inside callback */
	}
	++count;
      }
      (void)rpmmiFree(mi);
      (void)rpmtsFree(db->ts);
    } 
  } else croak("bad arguments list");
  RETVAL = count;
  OUTPUT:
  RETVAL

int
Db_traverse_tag_find(db,tag,name,callback)
  URPM::DB db
  char *tag
  char *name
  SV *callback
  PREINIT:
  Header header;
  rpmmi mi;
  CODE:
  rpmTag rpmtag = rpmtag_from_string(tag);
  int found = 0;

  db->ts = rpmtsLink(db->ts, "URPM::DB::traverse_tag");
  ts_nosignature(db->ts);
  mi = rpmtsInitIterator(db->ts, rpmtag, name, 0);
  while ((header = rpmmiNext(mi))) {
      dSP;
      URPM__Package pkg = calloc(1, sizeof(struct s_Package));

      assert(pkg != NULL);
      pkg->flag = FLAG_ID_INVALID | FLAG_NO_HEADER_FREE;
      pkg->h = header;

      PUSHMARK(SP);
      mXPUSHs(sv_setref_pv(newSVpvs(""), "URPM::Package", pkg));
      PUTBACK;

      int count = call_sv(callback, G_SCALAR);

      SPAGAIN;
      pkg->h = NULL; /* avoid using it anymore, in case it has been copied inside callback */

      if (count == 1 && POPi) {
	found = 1;
	break;
      }
  }
  (void)rpmmiFree(mi);
  (void)rpmtsFree(db->ts);
  RETVAL = found;
  OUTPUT:
  RETVAL

URPM::Transaction
Db_create_transaction(db, prefix=NULL)
  URPM::DB db
  char *prefix
  CODE:
  /* this is *REALLY* dangerous to create a new transaction while another is open,
     so use the db transaction instead. */
  db->ts = rpmtsLink(db->ts, "URPM::DB::create_transaction");
  ++db->count;
  RETVAL = db;
  OUTPUT:
  RETVAL


MODULE = URPM            PACKAGE = URPM::Transaction   PREFIX = Trans_

void
Trans_DESTROY(trans)
  URPM::Transaction trans
  CODE:
  (void)rpmtsFree(trans->ts);
  if (!--trans->count) free(trans);

void
Trans_set_script_fd(trans, fdno)
  URPM::Transaction trans
  int fdno
  CODE:
  rpmtsSetScriptFd(trans->ts, fdDup(fdno));

int
Trans_add(trans, pkg, ...)
  URPM::Transaction trans
  URPM::Package pkg
  CODE:
  if ((pkg->flag & FLAG_ID) <= FLAG_ID_MAX && pkg->h != NULL) {
    int update = 0;
    int rc;
    rpmRelocation relocations = NULL;
    /* compability mode with older interface of add */
    if (items == 3)
      update = SvIV(ST(2));
    else if (items > 3) {
      int i;
      for (i = 2; i < items-1; i+=2) {
	STRLEN len;
	char *s = SvPV(ST(i), len);

	if (len == 6 && !memcmp(s, "update", 6))
	  update = SvIV(ST(i+1));
	else if (len == 11 && !memcmp(s, "excludepath", 11)) {
	  if (SvROK(ST(i+1)) && SvTYPE(SvRV(ST(i+1))) == SVt_PVAV) {
	    AV *excludepath = (AV*)SvRV(ST(i+1));
	    I32 j = 1 + av_len(excludepath);
	    int relno = 0;
	    while (--j >= 0) {
	      SV **e = av_fetch(excludepath, j, 0);
	      if (e != NULL && *e != NULL)
		rpmfiAddRelocation(&relocations, &relno, SvPV_nolen(*e), NULL);
	    }
	  }
	}
      }
    }
    rc = rpmtsAddInstallElement(trans->ts, pkg->h, (fnpyKey)(1+(long)(pkg->flag & FLAG_ID)), update, relocations);
    if(rc) {
      rpmps ps = rpmtsProblems(trans->ts);
      PUTBACK;
      return_problems(ps, 1, 0);
      SPAGAIN;
    }

    /* free allocated memory, check rpm is copying it just above, at least in 4.0.4 */
    rpmfiFreeRelocations(relocations);
    RETVAL = rc == 0;
  } else RETVAL = 0;
  OUTPUT:
  RETVAL

int
Trans_remove(trans, name, tagname = NULL)
  URPM::Transaction trans
  char *name
  char *tagname
  PREINIT:
  Header h;
  rpmmi mi;
  int count = 0;
  rpmTag tag = RPMTAG_NVRA;
  CODE:
  if (tagname)
    tag = rpmtag_from_string(tagname);
  mi = rpmtsInitIterator(trans->ts, tag, name, 0);
  while ((h = rpmmiNext(mi))) {
    unsigned int recOffset = rpmmiInstance(mi);
    if (recOffset != 0) {
      rpmtsAddEraseElement(trans->ts, h, recOffset);
      ++count;
    }
  }
  mi = rpmmiFree(mi);
  RETVAL=count;
  OUTPUT:
  RETVAL

int
Trans_traverse(trans, callback)
  URPM::Transaction trans
  SV *callback
  PREINIT:
  rpmmi mi;
  Header h;
  int c = 0;
  CODE:
  mi = rpmtsInitIterator(trans->ts, RPMDBI_PACKAGES, NULL, 0);
  while ((h = rpmmiNext(mi))) {
    if (SvROK(callback)) {
      dSP;
      URPM__Package pkg = calloc(1, sizeof(struct s_Package));
      pkg->flag = FLAG_ID_INVALID | FLAG_NO_HEADER_FREE;
      pkg->h = h;
      PUSHMARK(SP);
      mXPUSHs(sv_setref_pv(newSVpvs(""), "URPM::Package", pkg));
      PUTBACK;
      call_sv(callback, G_DISCARD | G_SCALAR);
      SPAGAIN;
      pkg->h = NULL; /* avoid using it anymore, in case it has been copied inside callback */
    }
    ++c;
  }
  mi = rpmmiFree(mi);
  RETVAL = c;
  OUTPUT:
  RETVAL

void
Trans_check(trans, ...)
  URPM::Transaction trans
  PREINIT:
  I32 gimme = GIMME_V;
  int translate_message = 1, raw_message = 0;
  int i, r;
  PPCODE:
  for (i = 1; i < items-1; i+=2) {
    STRLEN len;
    char *s = SvPV(ST(i), len);

    if (len == 17 && !memcmp(s, "translate_message", 17))
      translate_message = SvIV(ST(i+1));
    else if (len == 11 && !memcmp(s, "raw_message", 11))
      raw_message = 1;
  }
  r = rpmtsCheck(trans->ts);
  rpmps ps = rpmtsProblems(trans->ts);
  if (rpmpsNumProblems(ps) > 0) {
    if (gimme == G_SCALAR)
      mXPUSHs(newSViv(0));
    else if (gimme == G_ARRAY) {
      /* now translation is handled by rpmlib, but only for version 4.2 and above */
      PUTBACK;
      return_problems(ps, translate_message, raw_message || !translate_message);
      SPAGAIN;
    }
  } else if (gimme == G_SCALAR)
    mXPUSHs(newSViv(1));
  if(r == 1)
    mXPUSHs(newSVpvs("error while checking dependencies"));

  ps = rpmpsFree(ps);

void
Trans_order(trans)
  URPM::Transaction trans
  PREINIT:
  I32 gimme = GIMME_V;
  PPCODE:
  if (rpmtsOrder(trans->ts) == 0) {
    if (gimme == G_SCALAR)
      mXPUSHs(newSViv(1));
  } else {
    if (gimme == G_SCALAR)
      mXPUSHs(newSViv(0));
    else if (gimme == G_ARRAY)
      mXPUSHs(newSVpvs("error while ordering dependencies"));
  }

int
Trans_NElements(trans)
  URPM::Transaction trans
  CODE:
  RETVAL = rpmtsNElements(trans->ts);
  OUTPUT:
  RETVAL

char *
Trans_Element_name(trans, index)
  URPM::Transaction trans
  int index
  CODE:
  rpmte te = rpmtsElement(trans->ts, index);
  RETVAL = te ? (char *) rpmteN(te) : NULL;
  OUTPUT:
  RETVAL

char *
Trans_Element_epoch(trans, index)
  URPM::Transaction trans
  int index
  CODE:
  rpmte te = rpmtsElement(trans->ts, index);
  RETVAL = te ? (char *) rpmteE(te) : NULL;
  OUTPUT:
  RETVAL

char *
Trans_Element_version(trans, index)
  URPM::Transaction trans
  int index
  CODE:
  rpmte te = rpmtsElement(trans->ts, index);
  RETVAL = te ? (char *) rpmteV(te) : NULL;
  OUTPUT:
  RETVAL

char *
Trans_Element_release(trans, index)
  URPM::Transaction trans
  int index
  CODE:
  rpmte te = rpmtsElement(trans->ts, index);
  RETVAL = te ? (char *) rpmteR(te) : NULL;
  OUTPUT:
  RETVAL

char *
Trans_Element_distepoch(trans, index)
  URPM::Transaction trans
  int index
  CODE:
  rpmte te = rpmtsElement(trans->ts, index);
  RETVAL = te ? (char *) rpmteD(te) : NULL;
  OUTPUT:
  RETVAL


char *
Trans_Element_fullname(trans, index)
  URPM::Transaction trans
  int index
  CODE:
  rpmte te = rpmtsElement(trans->ts, index);
  RETVAL = te ? (char *) rpmteNEVRA(te) : NULL;
  OUTPUT:
  RETVAL

void
Trans_run(trans, data, ...)
  URPM::Transaction trans
  SV *data
  PREINIT:
  struct s_TransactionData td = { NULL, NULL, NULL, NULL, NULL, 100000, data };
  rpmtransFlags transFlags = RPMTRANS_FLAG_NONE;
  int probFilter = 0;
  int translate_message = 0, raw_message = 0;
  int i;
  PPCODE:
  for (i = 2 ; i < items - 1 ; i += 2) {
    STRLEN len;
    char *s = SvPV(ST(i), len);

    if (len == 4 && !memcmp(s, "test", 4)) {
      if (SvIV(ST(i+1))) transFlags |= RPMTRANS_FLAG_TEST;
    } else if (len == 11 && !memcmp(s, "excludedocs", 11)) {
      if (SvIV(ST(i+1))) transFlags |= RPMTRANS_FLAG_NODOCS;
    } else if (len == 5) {
      if (!memcmp(s, "force", 5)) {
	if (SvIV(ST(i+1))) probFilter |= (RPMPROB_FILTER_REPLACEPKG |
					  RPMPROB_FILTER_REPLACEOLDFILES |
					  RPMPROB_FILTER_REPLACENEWFILES |
					  RPMPROB_FILTER_OLDPACKAGE);
      } else if (!memcmp(s, "delta", 5))
	td.min_delta = SvIV(ST(i+1));
    } else if (len == 6 && !memcmp(s, "nosize", 6)) {
      if (SvIV(ST(i+1))) probFilter |= (RPMPROB_FILTER_DISKSPACE|RPMPROB_FILTER_DISKNODES);
    } else if (len == 9 && !memcmp(s, "noscripts", 9)) {
      if (SvIV(ST(i+1))) transFlags |= (RPMTRANS_FLAG_NOSCRIPTS |
				        RPMTRANS_FLAG_NOPRE |
				        RPMTRANS_FLAG_NOPREUN |
				        RPMTRANS_FLAG_NOPOST |
				        RPMTRANS_FLAG_NOPOSTUN );

    } else if (len == 10 && !memcmp(s, "notriggers", 10)) {
      if (SvIV(ST(i+1))) transFlags |= (RPMTRANS_FLAG_NOTRIGGERS|_noTransTriggers);
    } else if (len == 10 && !memcmp(s, "nofdigests", 10)) {
      if (SvIV(ST(i+1))) transFlags |= RPMTRANS_FLAG_NOFDIGESTS;
    } else if (len == 10 && !memcmp(s, "oldpackage", 10)) {
      if (SvIV(ST(i+1))) probFilter |= RPMPROB_FILTER_OLDPACKAGE;
    } else if (len == 11 && !memcmp(s, "replacepkgs", 11)) {
      if (SvIV(ST(i+1))) probFilter |= RPMPROB_FILTER_REPLACEPKG;
    } else if (len == 11 && !memcmp(s, "raw_message", 11)) {
      raw_message = 1;
    } else if (len == 12 && !memcmp(s, "replacefiles", 12)) {
      if (SvIV(ST(i+1))) probFilter |= RPMPROB_FILTER_REPLACEOLDFILES | RPMPROB_FILTER_REPLACENEWFILES;
    } else if (len == 9 && !memcmp(s, "repackage", 9)) {
      if (SvIV(ST(i+1))) transFlags |= RPMTRANS_FLAG_REPACKAGE;
    } else if (len == 6 && !memcmp(s, "justdb", 6)) {
      if (SvIV(ST(i+1))) transFlags |= RPMTRANS_FLAG_JUSTDB;
    } else if (len == 10 && !memcmp(s, "ignorearch", 10)) {
      if (SvIV(ST(i+1))) probFilter |= RPMPROB_FILTER_IGNOREARCH;
    } else if (len == 17 && !memcmp(s, "translate_message", 17))
      translate_message = 1;
    else if (len >= 9 && !memcmp(s, "callback_", 9)) {
      if (len == 9+4 && !memcmp(s+9, "open", 4)) {
	if (SvROK(ST(i+1))) td.callback_open = ST(i+1);
      } else if (len == 9+5 && !memcmp(s+9, "close", 5)) {
	if (SvROK(ST(i+1))) td.callback_close = ST(i+1);
      } else if (len == 9+5 && !memcmp(s+9, "trans", 5)) {
	if (SvROK(ST(i+1))) td.callback_trans = ST(i+1);
      } else if (len == 9+6 && !memcmp(s+9, "uninst", 6)) {
	if (SvROK(ST(i+1))) td.callback_uninst = ST(i+1);
      } else if (len == 9+4 && !memcmp(s+9, "inst", 4)) {
	if (SvROK(ST(i+1))) td.callback_inst = ST(i+1);
      }
    }
  }
  /* check macros */
  {
    char *repa = rpmExpand("%{_repackage_all_erasures}", NULL);
    if (repa && *repa && *repa != '0')
      transFlags |= RPMTRANS_FLAG_REPACKAGE;
    if (repa) free(repa);
  }
  rpmtsSetFlags(trans->ts, transFlags);
  trans->ts = rpmtsLink(trans->ts, "URPM::Transaction::run");
  rpmtsSetNotifyCallback(trans->ts, rpmRunTransactions_callback, &td);
  if (rpmtsRun(trans->ts, NULL, probFilter) > 0) {
    rpmps ps = rpmtsProblems(trans->ts);
    PUTBACK;
    return_problems(ps, translate_message, raw_message || !translate_message);
    SPAGAIN;
    ps = rpmpsFree(ps);
  }
  rpmtsEmpty(trans->ts);
  (void)rpmtsFree(trans->ts);

MODULE = URPM            PACKAGE = URPM                PREFIX = Urpm_

BOOT:
(void) read_config_files(0);
Perl_call_atexit(PERL_GET_CONTEXT, (ATEXIT_t)urpm_perl_atexit,0);
rpmdbCheckSignals();

void
Urpm_bind_rpm_textdomain_codeset()
  CODE:
  rpm_codeset_is_utf8 = 1;
  bind_textdomain_codeset("rpm", "UTF-8");

int
Urpm_read_config_files()
  CODE:
  RETVAL = (read_config_files(1) == 0); /* force re-read of configuration files */
  OUTPUT:
  RETVAL

void
Urpm_list_rpm_tag(urpm=Nullsv)
   SV *urpm
   CODE:
   croak("list_rpm_tag() has been removed from perl-URPM. please report if you need it back");

int
rpmvercmp(one, two)
    char *one
    char *two        

int
rpmEVRcmp(one, two)
    char *one
    char *two        

int
rpmEVRcompare(one, two)
    char *one
    char *two        
  PREINIT:
  int compare;
  CODE:
  compare = do_rpmEVRcompare(one, two);
  RETVAL = compare;
  OUTPUT:
  RETVAL

int
rpmtag_from_string(tag)
    const char *tag

int
Urpm_ranges_overlap(a, b)
  char *a
  char *b
  PREINIT:
  char *sa = a, *sb = b;
  int aflags = 0, bflags = 0;
  CODE:
  while (*sa && *sa != ' ' && *sa != '[' && *sa != '<' && *sa != '>' && *sa != '=' && *sa == *sb) {
    ++sa;
    ++sb;
  }
  if ((*sa && *sa != ' ' && *sa != '[' && *sa != '<' && *sa != '>' && *sa != '=') ||
      (*sb && *sb != ' ' && *sb != '[' && *sb != '<' && *sb != '>' && *sb != '=')) {
    /* the strings are sure to be different */
    RETVAL = 0;
  } else {
    while (*sa) {
      if (*sa == ' ' || *sa == '[' || *sa == '*' || *sa == ']');
      else if (*sa == '<') aflags |= RPMSENSE_LESS;
      else if (*sa == '>') aflags |= RPMSENSE_GREATER;
      else if (*sa == '=') aflags |= RPMSENSE_EQUAL;
      else break;
      ++sa;
    }
    while (*sb) {
      if (*sb == ' ' || *sb == '[' || *sb == '*' || *sb == ']');
      else if (*sb == '<') bflags |= RPMSENSE_LESS;
      else if (*sb == '>') bflags |= RPMSENSE_GREATER;
      else if (*sb == '=') bflags |= RPMSENSE_EQUAL;
      else break;
      ++sb;
    }
    RETVAL = ranges_overlap(aflags, sa, bflags, sb);
  }
  OUTPUT:
  RETVAL

void
Urpm_parse_synthesis__XS(urpm, filename, ...)
  SV *urpm
  char *filename
  PPCODE:
  if (SvROK(urpm) && SvTYPE(SvRV(urpm)) == SVt_PVHV) {
    SV **fdepslist = hv_fetch((HV*)SvRV(urpm), "depslist", 8, 0);
    AV *depslist = fdepslist && SvROK(*fdepslist) && SvTYPE(SvRV(*fdepslist)) == SVt_PVAV ? (AV*)SvRV(*fdepslist) : NULL;
    SV **fprovides = hv_fetch((HV*)SvRV(urpm), "provides", 8, 0);
    HV *provides = fprovides && SvROK(*fprovides) && SvTYPE(SvRV(*fprovides)) == SVt_PVHV ? (HV*)SvRV(*fprovides) : NULL;
    SV **fobsoletes = hv_fetch((HV*)SvRV(urpm), "obsoletes", 9, 0);
    HV *obsoletes = fobsoletes && SvROK(*fobsoletes) && SvTYPE(SvRV(*fobsoletes)) == SVt_PVHV ? (HV*)SvRV(*fobsoletes) : NULL;

    if (depslist != NULL) {
      char buff[8*BUFSIZ];
      char *p, *eol;
      int buff_len;
      struct s_Package pkg;
      FD_t fd;
      int start_id = 1 + av_len(depslist);
      SV *callback = NULL;

      if (items > 2) {
	int i;
	for (i = 2; i < items-1; i+=2) {
	  STRLEN len;
	  char *s = SvPV(ST(i), len);

	  if (len == 8 && !memcmp(s, "callback", 8) && SvROK(ST(i+1)))
	    callback = ST(i+1);
	}
      }

      PUTBACK;
      if ((fd = xOpen(filename))) {
	memset(&pkg, 0, sizeof(struct s_Package));
	buff[sizeof(buff)-1] = 0;
	p = buff;
	int ok = 1;
	while ((buff_len = Fread(p, 1, sizeof(buff)-1-(p-buff), fd)) >= 0 && 
	       (buff_len += p-buff)) {
	  if (buff_len) {
	    buff[buff_len] = 0;
	    p = buff;
	    if ((eol = strchr(p, '\n')) != NULL) {
	      do {
		*eol++ = 0;
		if (!parse_line(depslist, provides, obsoletes, &pkg, p, urpm, callback)) { ok = 0; break; }
		p = eol;
	      } while ((eol = strchr(p, '\n')) != NULL);
	    } else {
	      /* a line larger than sizeof(buff) has been encountered, bad file problably */
	      fprintf(stderr, "invalid line <%s>\n%s\n", p, buff);
	      ok = 0;
	      break;
	    }
	    /* move the remaining non-complete-line at beginning */
	    memmove(buff, p, buff_len-(p-buff));
	    /* point to the end of the non-complete-line */
	    p = &buff[buff_len-(p-buff)];
	  } else {
	    if (!parse_line(depslist, provides, obsoletes, &pkg, p, urpm, callback)) ok = 0;
	    break;
	  }
	}
	if (Fclose(fd)) ok = 0;
	SPAGAIN;
	if (ok) {
	  mXPUSHs(newSViv(start_id));
	  mXPUSHs(newSViv(av_len(depslist)));
	}
      } else {
	  SV **nofatal = hv_fetch((HV*)SvRV(urpm), "nofatal", 7, 0);
	  if (!errno) errno = EINVAL; /* zlib error */
	  if (!nofatal || !SvIV(*nofatal))
	      croak(errno == ENOENT
		      ? "unable to read synthesis file %s"
		      : "unable to uncompress synthesis file %s", filename);
      }
    } else croak("first argument should contain a depslist ARRAY reference");
  } else croak("first argument should be a reference to a HASH");

void
Urpm_parse_hdlist__XS(urpm, filename, ...)
  SV *urpm
  char *filename
  PPCODE:
  if (SvROK(urpm) && SvTYPE(SvRV(urpm)) == SVt_PVHV) {
    SV **fdepslist = hv_fetch((HV*)SvRV(urpm), "depslist", 8, 0);
    AV *depslist = fdepslist && SvROK(*fdepslist) && SvTYPE(SvRV(*fdepslist)) == SVt_PVAV ? (AV*)SvRV(*fdepslist) : NULL;
    SV **fprovides = hv_fetch((HV*)SvRV(urpm), "provides", 8, 0);
    HV *provides = fprovides && SvROK(*fprovides) && SvTYPE(SvRV(*fprovides)) == SVt_PVHV ? (HV*)SvRV(*fprovides) : NULL;
    SV **fobsoletes = hv_fetch((HV*)SvRV(urpm), "obsoletes", 9, 0);
    HV *obsoletes = fobsoletes && SvROK(*fobsoletes) && SvTYPE(SvRV(*fobsoletes)) == SVt_PVHV ? (HV*)SvRV(*fobsoletes) : NULL;

    if (depslist != NULL) {
      pid_t pid = 0;
      int d;
      int empty_archive = 0;
      FD_t fd;

      d = open_archive(filename, &pid, &empty_archive);
      if (d >= 0) {
        fd = fdDup(d);
        close(d);
      } else
        fd = NULL;

      if (empty_archive) {
	mXPUSHs(newSViv(1 + av_len(depslist)));
	mXPUSHs(newSViv(av_len(depslist)));
      } else if (fd) {
	rpmts ts = NULL;
	rpmgi gi = NULL;
	rpmRC rc = RPMRC_NOTFOUND;
	int start_id = 1 + av_len(depslist);
	int packing = 0;
	SV *callback = NULL;

	/* compability mode with older interface of parse_hdlist */
	if (items == 3)
	  packing = SvTRUE(ST(2));
	else if (items > 3) {
	  int i;
	  for (i = 2; i < items-1; i+=2) {
	    STRLEN len;
	    char *s = SvPV(ST(i), len);

	    if (len == 7 && !memcmp(s, "packing", 7))
	      packing = SvTRUE(ST(i+1));
	    else if (len == 8 && !memcmp(s, "callback", 8) && SvROK(ST(i+1)))
	      callback = ST(i+1);
	  }
	}

	PUTBACK;
	rc = RPMRC_NOTFOUND;
	ts = rpmtsCreate();
	rpmtsSetRootDir(ts, NULL);
	gi = rpmgiNew(ts, RPMDBI_HDLIST, NULL, 0);

	rpmtsSetVSFlags(ts, _RPMVSF_NOSIGNATURES | RPMVSF_NOHDRCHK | _RPMVSF_NOPAYLOAD | _RPMVSF_NOHEADER);
	gi->active = 1;
	gi->fd = fd;
	while ((rc = rpmgiNext(gi)) == RPMRC_OK) {
	  struct s_Package pkg, *_pkg;
	  SV *sv_pkg;

	  memset(&pkg, 0, sizeof(struct s_Package));
	  pkg.flag = 1 + av_len(depslist);
	  pkg.h = rpmgiHeader(gi);
	  /* prevent rpmgiNext() from freeing header */
	  gi->h = NULL;
	  sv_pkg = sv_setref_pv(newSVpvs(""), "URPM::Package",
	      (_pkg = memcpy(malloc(sizeof(struct s_Package)), &pkg, sizeof(struct s_Package))));
	  if (call_package_callback(urpm, sv_pkg, callback)) {
	    if (provides) {
	      update_provides(_pkg, provides);
	      update_provides_files(_pkg, provides);
	    }
	    if (obsoletes) update_obsoletes(_pkg, obsoletes);
	    if (packing) pack_header(_pkg);
	    av_push(depslist, sv_pkg);
	  }
	}
	gi = rpmgiFree(gi);
	ts = rpmtsFree(ts);

	int ok = 1;

	if (pid) {
	  kill(pid, SIGTERM);
	  int status;
	  int rc = waitpid(pid, &status, 0);
	  ok = rc != -1 && WEXITSTATUS(status) != 1; /* in our standard case, gzip will exit with status code 2, meaning "decompression OK, trailing garbage ignored" */
	  pid = 0;
	} else if (!empty_archive)
	  ok = av_len(depslist) >= start_id;
	SPAGAIN;
	if (ok) {
	  mXPUSHs(newSViv(start_id));
	  mXPUSHs(newSViv(av_len(depslist)));
	}
      } else {
	SV **nofatal = hv_fetch((HV*)SvRV(urpm), "nofatal", 7, 0);
	if (!nofatal || !SvIV(*nofatal))
	  croak("cannot open hdlist file %s", filename);
      }
    } else croak("first argument should contain a depslist ARRAY reference");
  } else croak("first argument should be a reference to a HASH");

void
Urpm_parse_rpm(urpm, filename, ...)
  SV *urpm
  char *filename
  PPCODE:
  if (SvROK(urpm) && SvTYPE(SvRV(urpm)) == SVt_PVHV) {
    SV **fdepslist = hv_fetch((HV*)SvRV(urpm), "depslist", 8, 0);
    AV *depslist = fdepslist && SvROK(*fdepslist) && SvTYPE(SvRV(*fdepslist)) == SVt_PVAV ? (AV*)SvRV(*fdepslist) : NULL;
    SV **fprovides = hv_fetch((HV*)SvRV(urpm), "provides", 8, 0);
    HV *provides = fprovides && SvROK(*fprovides) && SvTYPE(SvRV(*fprovides)) == SVt_PVHV ? (HV*)SvRV(*fprovides) : NULL;
    SV **fobsoletes = hv_fetch((HV*)SvRV(urpm), "obsoletes", 8, 0);
     HV *obsoletes = fobsoletes && SvROK(*fobsoletes) && SvTYPE(SvRV(*fobsoletes)) == SVt_PVHV ? (HV*)SvRV(*fobsoletes) : NULL;

    if (depslist != NULL) {
      struct s_Package pkg, *_pkg;
      SV *sv_pkg;
      int packing = 0;
      int keep_all_tags = 0;
      SV *callback = NULL;
      rpmVSFlags vsflags = RPMVSF_DEFAULT;

      /* compability mode with older interface of parse_hdlist */
      if (items == 3)
	packing = SvTRUE(ST(2));
      else if (items > 3) {
	int i;
	for (i = 2; i < items-1; i+=2) {
	  STRLEN len;
	  char *s = SvPV(ST(i), len);

	  if (len == 7 && !memcmp(s, "packing", 7))
	    packing = SvTRUE(ST(i + 1));
	  else if (len == 13 && !memcmp(s, "keep_all_tags", 13))
	    keep_all_tags = SvTRUE(ST(i+1));
	  else if (len == 8 && !memcmp(s, "callback", 8) && SvROK(ST(i+1)))
	    callback = ST(i+1);
	  else if SvIV(ST(i+1)) {
	    if (len == 5) {
	      if (!memcmp(s, "nopgp", 5))
		vsflags |= (RPMVSF_NOSHA1 | RPMVSF_NOSHA1HEADER);
	      else if (!memcmp(s, "nogpg", 5))
		vsflags |= (RPMVSF_NOSHA1 | RPMVSF_NOSHA1HEADER);
	      else if (!memcmp(s, "nomd5", 5))
		vsflags |= (RPMVSF_NOMD5 |  RPMVSF_NOMD5HEADER);
	      else if (!memcmp(s, "norsa", 5))
		vsflags |= (RPMVSF_NORSA | RPMVSF_NORSAHEADER);
	      else if (!memcmp(s, "nodsa", 5))
		vsflags |= (RPMVSF_NODSA | RPMVSF_NODSAHEADER);
	    } else if (len == 9) {
	      if (!memcmp(s, "nodigests", 9))
		vsflags |= _RPMVSF_NODIGESTS;
	      else if (!memcmp(s, "nopayload", 9))
		vsflags |= _RPMVSF_NOPAYLOAD;
	    }
	  }
	}
      }
      PUTBACK;
      memset(&pkg, 0, sizeof(struct s_Package));
      pkg.flag = 1 + av_len(depslist);
      _pkg = memcpy(malloc(sizeof(struct s_Package)), &pkg, sizeof(struct s_Package));

      if (update_header(filename, _pkg, keep_all_tags, vsflags)) {
	sv_pkg = sv_setref_pv(newSVpvs(""), "URPM::Package", _pkg);
	if (call_package_callback(urpm, sv_pkg, callback)) {
	  if (provides) {
	    update_provides(_pkg, provides);
	    update_provides_files(_pkg, provides);
	  }
	  if (obsoletes) update_obsoletes(_pkg, obsoletes);
	  if (packing) pack_header(_pkg);
	  av_push(depslist, sv_pkg);
	}
	SPAGAIN;
	/* only one element read */
	mXPUSHs(newSViv(av_len(depslist)));
	mXPUSHs(newSViv(av_len(depslist)));
      } else free(_pkg);
    } else croak("first argument should contain a depslist ARRAY reference");
  } else croak("first argument should be a reference to a HASH");

int
Urpm_verify_rpm(filename, ...)
  char *filename
  PREINIT:
  FD_t fd;
  int i, oldlogmask;
  rpmts ts = NULL;
  struct rpmQVKArguments_s qva;
  CODE:
  /* Don't display error messages */
  oldlogmask = rpmlogSetMask(RPMLOG_UPTO(RPMLOG_PRI(4)));
  memset(&qva, 0, sizeof(struct rpmQVKArguments_s));
  qva.qva_source = RPMQV_RPM;
  qva.qva_flags = VERIFY_ALL;
  for (i = 1 ; i < items - 1 ; i += 2) {
    STRLEN len;
    char *s = SvPV(ST(i), len);
    if (SvIV(ST(i+1))) {
      if (len == 9 && !strncmp(s, "nodigests", 9))
	qva.qva_flags &= ~VERIFY_DIGEST;
      else if (len == 10 && !strncmp(s, "nofdigests", 10))
	qva.qva_flags &= ~VERIFY_FDIGEST;
      else if (len == 12 && !strncmp(s, "nosignatures", 12))
	qva.qva_flags &= ~VERIFY_SIGNATURE;
    }
  }
  fd = Fopen(filename, "r");
  if (fd == NULL)
    RETVAL = 0;
  else {
    read_config_files(0);
    ts = rpmtsCreate();
    rpmtsSetRootDir(ts, NULL);
    (void)rpmtsOpenDB(ts, O_RDONLY);
    if (rpmVerifySignatures(&qva, ts, fd, filename))
      RETVAL = 0;
    else
      RETVAL = 1;
    Fclose(fd);
    (void)rpmtsFree(ts);
  }
  rpmlogSetMask(oldlogmask);

  OUTPUT:
  RETVAL


char *
Urpm_get_gpg_fingerprint(filename)
    char * filename
    PREINIT:
    uint8_t fingerprint[sizeof(pgpKeyID_t)];
    char fingerprint_str[sizeof(pgpKeyID_t) * 2 + 1];
    const uint8_t *pkt = NULL;
    size_t pktlen = 0;
    int rc;

    CODE:
    memset (fingerprint, 0, sizeof (fingerprint));
    if ((rc = pgpReadPkts(filename, (uint8_t ** ) &pkt, &pktlen)) <= 0)
	pktlen = 0;
    else if (rc != PGPARMOR_PUBKEY)
	pktlen = 0;
    else {
	unsigned int i;
        pgpPubkeyFingerprint (pkt, pktlen, fingerprint);
   	for (i = 0; i < sizeof (pgpKeyID_t); i++)
	    sprintf(&fingerprint_str[i*2], "%02x", fingerprint[i]);
    }
    _free(pkt);
    RETVAL = fingerprint_str;
    OUTPUT:
    RETVAL


char *
Urpm_verify_signature(filename, prefix=NULL)
  char *filename
  char *prefix
  PREINIT:
  rpmts ts = NULL;
  char result[1024];
  rpmRC rc;
  FD_t fd;
  Header h;
  CODE:
  fd = Fopen(filename, "r");
  if (fd == NULL)
    RETVAL = "NOT OK (could not read file)";
  else {
    read_config_files(0);
    ts = rpmtsCreate();
    rpmtsSetRootDir(ts, prefix);
    (void)rpmtsOpenDB(ts, O_RDONLY);
    rpmtsSetVSFlags(ts, RPMVSF_DEFAULT);
    rc = rpmReadPackageFile(ts, fd, filename, &h);
    Fclose(fd);
    *result = '\0';
    switch(rc) {
      case RPMRC_OK:
	if (h) {
	  char *fmtsig = headerSprintf(
	      h,
	      "%|DSAHEADER?{%{DSAHEADER:pgpsig}}:{%|RSAHEADER?{%{RSAHEADER:pgpsig}}:"
	      "{%|SIGGPG?{%{SIGGPG:pgpsig}}:{%|SIGPGP?{%{SIGPGP:pgpsig}}:{(none)}|}|}|}|",
	      NULL,
	      NULL,
	      NULL);
	  snprintf(result, sizeof(result), "OK (%s)", fmtsig);
	  free(fmtsig);
	} else snprintf(result, sizeof(result), "NOT OK (bad rpm): %s", rpmlogMessage());
	break;
      case RPMRC_NOTFOUND:
      case RPMRC_NOSIG:
	snprintf(result, sizeof(result), "NOT OK (signature not found): %s", rpmlogMessage());
	break;
      case RPMRC_FAIL:
	snprintf(result, sizeof(result), "NOT OK (fail): %s", rpmlogMessage());
	break;
      case RPMRC_NOTTRUSTED:
	snprintf(result, sizeof(result), "NOT OK (key not trusted): %s", rpmlogMessage());
	break;
      case RPMRC_NOKEY:
	snprintf(result, sizeof(result), "NOT OK (no key): %s", rpmlogMessage());
	break;
    }
    RETVAL = result;
    if (h) h = headerFree(h);
    (void)rpmtsFree(ts);
  }

  OUTPUT:
  RETVAL

    
int
Urpm_import_pubkey_file(db, filename)
    URPM::DB db
    char * filename
    PREINIT:
    const uint8_t *pkt = NULL;
    size_t pktlen = 0;
    int rc;
    CODE:

    rpmts ts = rpmtsLink(db->ts, "URPM::import_pubkey_file");
    rpmtsClean(ts);
    
    if ((rc = pgpReadPkts(filename, (uint8_t ** ) &pkt, &pktlen)) <= 0)
        RETVAL = 0;
    else if (rc != PGPARMOR_PUBKEY)
        RETVAL = 0;
    else if (rpmcliImportPubkey(ts, pkt, pktlen) != RPMRC_OK)
        RETVAL = 0;
    else
        RETVAL = 1;
    pkt = _free(pkt);
    (void)rpmtsFree(ts);
    OUTPUT:
    RETVAL

int
Urpm_import_pubkey(...)
  CODE:
  unused_variable(&items);
  croak("import_pubkey() is dead. use import_pubkey_file() instead");
  RETVAL = 1;
  OUTPUT:
  RETVAL

int
Urpm_archscore(arch)
  const char * arch
  PREINIT:
  char * platform = NULL;
  CODE:
  read_config_files(0);
  platform = rpmExpand(arch, "-%{_target_vendor}-%{_target_os}%{?_gnu}", NULL);
  RETVAL=rpmPlatformScore(platform, NULL, 0);
  _free(platform);
  OUTPUT:
  RETVAL

int
Urpm_osscore(os)
  const char * os
  PREINIT:
  char * platform = NULL;
  CODE:
  read_config_files(0);
  platform = rpmExpand("%{_target_cpu}-%{_target_vendor}-", os, "%{?_gnu}", NULL);
  RETVAL=rpmPlatformScore(platform, NULL, 0);
  _free(platform);
  OUTPUT:
  RETVAL

int
Urpm_platformscore(platform)
  const char * platform
  CODE:
  read_config_files(0);
  RETVAL=rpmPlatformScore(platform, NULL, 0);
  OUTPUT:
  RETVAL

void
Urpm_stream2header(fp)
    FILE *fp
  PREINIT:
    FD_t fd;
    URPM__Package pkg;
  PPCODE:
    if ((fd = fdDup(fileno(fp)))) {
	const char item[] = "Header";
	const char * msg = NULL;
	rpmRC rc;

	pkg = (URPM__Package)calloc(1, sizeof(struct s_Package));
	rc = rpmpkgRead(item, fd, &pkg->h, &msg);

	switch (rc) {
	  default:
	    rpmlog(RPMLOG_ERR, "%s: %s: %s\n", "rpmpkgRead", item, msg);
	  case RPMRC_NOTFOUND:
	    pkg->h = NULL;
	  case RPMRC_OK:
	    break;
	}
	msg = (const char*)_free(msg);
        if (pkg->h) {
            SV *sv_pkg;
            EXTEND(SP, 1);
            sv_pkg = sv_newmortal();
            sv_setref_pv(sv_pkg, "URPM::Package", (void*)pkg);
            PUSHs(sv_pkg);
        }
        Fclose(fd);
    }

void
Urpm_spec2srcheader(specfile)
  char *specfile
  PREINIT:
    rpmts ts = rpmtsCreate();
    URPM__Package pkg;
    Spec spec = NULL;
  PPCODE:
/* ensure the config is in memory with all macro */
  read_config_files(0);
/* Do not verify architecture */
#define SPEC_ANYARCH 1
/* Do not verify whether sources exist */
#define SPEC_FORCE 1
  if (!parseSpec(ts, specfile, "/", 0, NULL, NULL, SPEC_ANYARCH, SPEC_FORCE, 0)) {
    SV *sv_pkg;
    HE_t he = (HE_t)memset(alloca(sizeof(*he)), 0, sizeof(*he));

    spec = rpmtsSetSpec(ts, NULL);
    initSourceHeader(spec, NULL);
    pkg = (URPM__Package)calloc(1, sizeof(struct s_Package));

    he->tag = RPMTAG_SOURCERPM;
    he->p.str = "";
    he->c = 1;
    headerPut(spec->sourceHeader, he, 0);

    {
      he->tag = RPMTAG_ARCH;
      he->t = RPM_STRING_TYPE;
      he->p.str = "src";
      he->c = 1,
      /* parseSpec() sets RPMTAG_ARCH to %{_target_cpu} whereas we really a header similar to .src.rpm header */
      headerMod(spec->sourceHeader, he, 0);
    }

    pkg->h = headerLink(spec->sourceHeader);
    sv_pkg = sv_newmortal();
    sv_setref_pv(sv_pkg, "URPM::Package", (void*)pkg);
    XPUSHs(sv_pkg);
    spec = freeSpec(spec);
  } else {
    XPUSHs(&PL_sv_undef);
    /* apparently rpmlib sets errno this when given a bad spec. */
    if (errno == EBADF)
      errno = 0;
  }
  ts = rpmtsFree(ts);

void
expand(name)
    char * name
    PPCODE:
    const char * value = rpmExpand(name, NULL);
    push_name_only(value, 0);
    _free(value);

void
add_macro_noexpand(macro)
    char * macro
    CODE:
    rpmDefineMacro(NULL, macro, RMIL_DEFAULT);

void
del_macro(name)
    char * name
    CODE:
    delMacro(NULL, name);

void
loadmacrosfile(filename)
    char * filename
    PPCODE:
    rpmInitMacros(NULL, filename);

void
resetmacros()
    PPCODE:
    rpmFreeMacros(NULL);

void
setVerbosity(level)
    int level
    PPCODE:
    rpmSetVerbosity(level);

void
setInternalVariable(type, value)
    char * type
    int value
    CODE:
    if (!strcmp(type, "_rpmbf_debug"))
      _rpmbf_debug = value;
    else if (!strcmp(type, "_rpmdb_debug"))
      _rpmdb_debug = value;
    else if (!strcmp(type, "_rpmfi_debug"))
      _rpmfi_debug = value;
    else if (!strcmp(type, "_rpmio_debug"))
      _rpmio_debug = value;
    else if (!strcmp(type, "_rpmps_debug"))
      _rpmps_debug = value;
    else if (!strcmp(type, "_rpmgi_debug"))
      _rpmgi_debug = value;
    else if (!strcmp(type, "_rpmte_debug"))
      _rpmte_debug = value;
    else if (!strcmp(type, "_rpmevr_debug"))
      _rpmevr_debug = value;
    else if (!strcmp(type, "_rpmds_debug"))
      _rpmds_debug = value;
    else if (!strcmp(type, "_rpmmi_debug"))
      _rpmmi_debug = value;
    else if (!strcmp(type, "_rpmns_debug"))
      _rpmns_debug = value;
    else if (!strcmp(type, "_rpmts_debug"))
      _rpmts_debug = value;
    else if (!strcmp(type, "_fps_debug"))
      _fps_debug = value;
    else if (!strcmp(type, "_mire_debug"))
      _mire_debug = value;
    else
      croak("unknown variable: %s", type);
    
const char *
rpmErrorString()
  CODE:
  RETVAL = rpmlogMessage();
  OUTPUT:
  RETVAL 

void
rpmErrorWriteTo(fd)
  int fd
  CODE:
  rpmError_callback_data = fd;
  rpmlogSetCallback(rpmError_callback, NULL);

  /* vim:set ts=8 sts=2 sw=2: */
