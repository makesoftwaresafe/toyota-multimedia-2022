#include "system.h"
const char *__progname;

#include <rpmio.h>
#include <rpmiotypes.h>
#include <rpmcb.h>
#include <argv.h>
#include <rpmtypes.h>
#include <rpmtag.h>

#include <rpmds.h>
#define	_RPMFC_INTERNAL		/* XXX for debugging */
#include <rpmfc.h>

#include <rpmcli.h>

#include "debug.h"

/*@unchecked@*/
char *progname;

#define	RPMDEP_RPMFC		1
#define	RPMDEP_RPMDSCPUINFO	2
#define	RPMDEP_RPMDSRPMLIB	3
#define	RPMDEP_RPMDSSYSINFO	4
#define	RPMDEP_RPMDSGETCONF	5
#define	RPMDEP_RPMDSELF		6
#define	RPMDEP_RPMDSLDCONFIG	7
#define	RPMDEP_RPMDSUNAME	8
#define	RPMDEP_RPMDSPIPE	9

#define	RPMDEP_RPMDSPERL	10
#define	RPMDEP_RPMDSPYTHON	11
#define	RPMDEP_RPMDSLIBTOOL	12
#define	RPMDEP_RPMDSPKGCONFIG	13

#define	RPMDEP_RPMDSPUBKEY	14
#define	RPMDEP_RPMDSARCH	15
#define	RPMDEP_RPMDSFILE	16
#define	RPMDEP_RPMDSSONAME	17
#define	RPMDEP_RPMDSPACKAGE	18

#define	RPMDEP_RPMDSJAVA	20
#define	RPMDEP_RPMDSRUBY	21
#define	RPMDEP_RPMDSPHP		22

#define	RPMDEP_RPMDSDPKGRPM	32
#define	RPMDEP_RPMDSRPMDPKG	33

/*@unchecked@*/
static int rpmdeps_mode = RPMDEP_RPMFC;

/*@unchecked@*/
static int print_provides = 1;

/*@unchecked@*/
static int print_requires = 1;

/*@unchecked@*/
static int print_closure = 0;

#define _PERL_PROVIDES  "/usr/bin/find /usr/lib/perl5 | /usr/lib/rpm/perl.prov"
/*@unchecked@*/ /*@observer@*/
static const char * _perl_provides = _PERL_PROVIDES;

#define _PERL_REQUIRES  "rpm -qa --fileclass | grep 'perl script' | sed -e 's/\t.*$//' | /usr/lib/rpm/perl.req"
/*@unchecked@*/ /*@observer@*/
static const char * _perl_requires = _PERL_REQUIRES;

#define _JAVA_PROVIDES  "rpm -qal | egrep '\\.(jar|class)$' | /usr/lib/rpm/javadeps.sh -P"
/*@unchecked@*/ /*@observer@*/
static const char * _java_provides = _JAVA_PROVIDES;

#define _JAVA_REQUIRES  "rpm -qal | egrep '\\.(jar|class)$' | /usr/lib/rpm/javadeps.sh -R"
/*@unchecked@*/ /*@observer@*/
static const char * _java_requires = _JAVA_REQUIRES;

#define _LIBTOOL_PROVIDES  "/usr/bin/find /usr/lib -name '*.la' | /usr/lib/rpm/libtooldeps.sh -P /"
/*@unchecked@*/ /*@observer@*/
static const char * _libtool_provides = _LIBTOOL_PROVIDES;

#define _LIBTOOL_REQUIRES  "rpm -qal | grep '\\.la$' | /usr/lib/rpm/libtooldeps.sh -R /"
/*@unchecked@*/ /*@observer@*/
static const char * _libtool_requires = _LIBTOOL_REQUIRES;

#define _PKGCONFIG_PROVIDES  "/usr/bin/find /usr/lib -name '*.pc' | /usr/lib/rpm/pkgconfigdeps.sh -P"
static const char * _pkgconfig_provides = _PKGCONFIG_PROVIDES;

#define _PKGCONFIG_REQUIRES  "rpm -qal | grep '\\.pc$' | /usr/lib/rpm/pkgconfigdeps.sh -R"
static const char * _pkgconfig_requires = _PKGCONFIG_REQUIRES;

#define _DPKG_PROVIDES  "egrep '^(Package|Status|Version|Provides):' /var/lib/dpkg/status | sed -e '\n\
/^Package: / {\n\
	N\n\
	/not-installed/d\n\
	N\n\
	s|^Package: \\([^\\n]*\\)\\n[^\\n]*\\nVersion: \\(.*\\)$|\\1 = \\2|\n\
}\n\
/^Provides: / {\n\
	s|^Provides: ||\n\
	s|, |\\n|g\n\
}' | sed -f /usr/lib/rpm/dpkg2fc.sed | sort -u | tee /tmp/dpkg"
static const char * _dpkg_provides = _DPKG_PROVIDES;

#define	_DPKG_REQUIRES "egrep '^(Package|Status|Pre-Depends|Depends):' /var/lib/dpkg/status | sed -e '\n\
/^Package: / {\n\
	N\n\
	/not-installed/d\n\
	s|^Package: [^\\n]*\\n.*$||\n\
}\n\
/^Depends: / {\n\
	s|^Depends: ||\n\
	s|(\\([^)]*\\))|\\1|g\n\
	s|>>|>|\n\
	s|<<|<|\n\
	s|, |\\n|g\n\
}\n\
/^Pre-Depends: / {\n\
	s|^Pre-Depends: ||\n\
	s|(\\([^)]*\\))|\\1|g\n\
	s|>>|>|\n\
	s|<<|<|\n\
	s|, |\\n|g\n\
}' | sed -f /usr/lib/rpm/dpkg2fc.sed | sed -e 's/ |.*$//' | sort -u | tee /tmp/dpkg"
static const char * _dpkg_requires = _DPKG_REQUIRES;

#define _RPMDB_PACKAGE_PROVIDES  "rpm -qa --qf '%{name} = %|epoch?{%{epoch}:}|%{version}-%{release}\n' | sort -u"
static const char * _rpmdb_package_provides = _RPMDB_PACKAGE_PROVIDES;

#define _RPMDB_PACKAGE_REQUIRES  "rpm -qa --requires | sort -u | sed -e '/^\\//d' -e '/.*\\.so.*/d' -e '/^%/d' -e '/^.*(.*)/d'"
static const char * _rpmdb_package_requires = _RPMDB_PACKAGE_REQUIRES;

#define _RPMDB_SONAME_REQUIRES  "rpm -qa --requires | grep -v '^/' | grep '.*\\.so.*' | sort -u"
static const char * _rpmdb_soname_requires = _RPMDB_SONAME_REQUIRES;

#define _RPMDB_FILE_REQUIRES  "rpm -qa --requires | grep '^/' | sort -u"
static const char * _rpmdb_file_requires = _RPMDB_FILE_REQUIRES;

static int rpmdepPrint(const char * msg, rpmds ds, FILE * fp)
{
    if (fp == NULL) fp = stderr;

    if (msg)
	fprintf(fp, "===================================== %s\n", msg);

    ds = rpmdsInit(ds);
    while (rpmdsNext(ds) >= 0) {
	if (_rpmfc_debug || rpmIsDebug())
	    fprintf(fp, "%6d\t", rpmdsIx(ds));
	if (_rpmfc_debug || rpmIsVerbose())
	    fprintf(fp, "%s: ", rpmdsType(ds));
	fprintf(fp, "%s\n", rpmdsDNEVR(ds)+2);
    }
    return 0;
}

static struct poptOption optionsTable[] = {

 { NULL, '\0', POPT_ARG_INCLUDE_TABLE, rpmcliAllPoptTable, 0,
	N_("Common options:"),
	NULL }, 

 { "cpuinfo",	0, POPT_ARG_VAL, &rpmdeps_mode, RPMDEP_RPMDSCPUINFO,
	N_("print cpuinfo(...) dependency set"), NULL },
 { "rpmlib",	0, POPT_ARG_VAL, &rpmdeps_mode, RPMDEP_RPMDSRPMLIB,
	N_("print rpmlib(...) dependency set"), NULL },
 { "sysinfo",	0, POPT_ARG_VAL, &rpmdeps_mode, RPMDEP_RPMDSSYSINFO,
	N_("print /etc/rpm/sysinfo dependency set"), NULL },
 { "getconf",	0, POPT_ARG_VAL, &rpmdeps_mode, RPMDEP_RPMDSGETCONF,
	N_("print getconf(...) dependency set"), NULL },
 { "elf",	0, POPT_ARG_VAL, &rpmdeps_mode, RPMDEP_RPMDSELF,
	N_("print soname(...) dependencies for elf files"), NULL },
 { "ldconfig",	0, POPT_ARG_VAL, &rpmdeps_mode, RPMDEP_RPMDSLDCONFIG,
	N_("print soname(...) dependencies from /etc/ld.so.cache"), NULL },
 { "uname",	0, POPT_ARG_VAL, &rpmdeps_mode, RPMDEP_RPMDSUNAME,
	N_("print uname(...) dependency set"), NULL },
 { "pipe",	0, POPT_ARG_VAL|POPT_ARGFLAG_DOC_HIDDEN, &rpmdeps_mode, RPMDEP_RPMDSPIPE,
	N_("print dependency set from a command pipe"), NULL },

 { "perl",	0, POPT_ARG_VAL, &rpmdeps_mode, RPMDEP_RPMDSPERL,
	N_("print perl(...) dependency set"), NULL },
 { "python",	0, POPT_ARG_VAL|POPT_ARGFLAG_DOC_HIDDEN, &rpmdeps_mode, RPMDEP_RPMDSPYTHON,
	N_("print python(...) dependency set"), NULL },
 { "libtool",	0, POPT_ARG_VAL, &rpmdeps_mode, RPMDEP_RPMDSLIBTOOL,
	N_("print libtool(...) dependency set"), NULL },
 { "pkgconfig",	0, POPT_ARG_VAL, &rpmdeps_mode, RPMDEP_RPMDSPKGCONFIG,
	N_("print pkgconfig(...) dependency set"), NULL },

 { "pubkey",	0, POPT_ARG_VAL|POPT_ARGFLAG_DOC_HIDDEN, &rpmdeps_mode, RPMDEP_RPMDSPUBKEY,
	N_("print pubkey(...) dependency set"), NULL },
 { "arch",	0, POPT_ARG_VAL|POPT_ARGFLAG_DOC_HIDDEN, &rpmdeps_mode, RPMDEP_RPMDSARCH,
	N_("print arch(...) dependency set"), NULL },
 { "file",	0, POPT_ARG_VAL, &rpmdeps_mode, RPMDEP_RPMDSFILE,
	N_("print file(...) dependency set"), NULL },
 { "soname",	0, POPT_ARG_VAL, &rpmdeps_mode, RPMDEP_RPMDSSONAME,
	N_("print soname(...) dependency set"), NULL },
 { "package",	0, POPT_ARG_VAL|POPT_ARGFLAG_DOC_HIDDEN, &rpmdeps_mode, RPMDEP_RPMDSPACKAGE,
	N_("print package(...) dependency set"), NULL },

 { "java",	0, POPT_ARG_VAL, &rpmdeps_mode, RPMDEP_RPMDSJAVA,
	N_("print java(...) dependency set"), NULL },
 { "php",	0, POPT_ARG_VAL|POPT_ARGFLAG_DOC_HIDDEN, &rpmdeps_mode, RPMDEP_RPMDSPHP,
	N_("print php(...) dependency set"), NULL },
 { "ruby",	0, POPT_ARG_VAL|POPT_ARGFLAG_DOC_HIDDEN, &rpmdeps_mode, RPMDEP_RPMDSRUBY,
	N_("print ruby(...) dependency set"), NULL },
 { "dpkgrpm",	0, POPT_ARG_VAL|POPT_ARGFLAG_DOC_HIDDEN, &rpmdeps_mode, RPMDEP_RPMDSDPKGRPM,
	N_("print /var/lib/dpkg Provides: dependency set"), NULL },
 { "rpmdpkg",	0, POPT_ARG_VAL|POPT_ARGFLAG_DOC_HIDDEN, &rpmdeps_mode, RPMDEP_RPMDSRPMDPKG,
	N_("print /var/lib/dpkg Requires: dependency set"), NULL },

 { "provides", 'P', POPT_ARG_VAL, &print_provides, -1,
	N_("print Provides: dependency set"), NULL },
 { "requires", 'R', POPT_ARG_VAL, &print_requires, -1,
	N_("print Requires: dependency set"), NULL },
 { "closure", 0, POPT_ARG_VAL|POPT_ARGFLAG_DOC_HIDDEN, &print_closure, -1,
	N_("check Requires: against Provides: for dependency closure"), NULL },

   POPT_AUTOALIAS
   POPT_AUTOHELP
   POPT_TABLEEND
};


int
main(int argc, char *argv[])
{
static const char rpmdeps_string[] = "rpmdeps";
    poptContext optCon;
    ARGV_t av = NULL;
    rpmfc fc = NULL;
    rpmds P = NULL;
    rpmds R = NULL;
    rpmPRCO PRCO = rpmdsNewPRCO(NULL);
const char * closure_name = "for";
    FILE * fp = NULL;
    int flags = 0;
    int ac = 0;
    int ec = 1;
    int xx;
    int i;
char buf[BUFSIZ];

/*@-modobserver@*/
    if ((progname = strrchr(argv[0], '/')) != NULL)
	progname++;
    else
	progname = argv[0];
/*@=modobserver@*/

    optCon = rpmcliInit(argc, argv, optionsTable);
    if (optCon == NULL)
	goto exit;

    av = poptGetArgs(optCon);
    ac = argvCount(av);

    if (rpmdeps_mode == RPMDEP_RPMFC && ac == 0) {
	av = NULL;
	xx = argvFgets(&av, NULL);
	ac = argvCount(av);
    }

    /* Make sure file names are sorted. */
    xx = argvSort(av, NULL);

    switch (rpmdeps_mode) {
    case RPMDEP_RPMFC:
	if (ac <= 0)		/* XXX avoid rpmfcApply() segfault */
	    break;

	/* Build file class dictionary. */
	fc = rpmfcNew();
	xx = rpmfcClassify(fc, av, NULL);

	/* Build file/package dependency dictionary. */
	xx = rpmfcApply(fc);

if (_rpmfc_debug) {
sprintf(buf, "final: files %d cdict[%d] %d%% ddictx[%d]", (int)fc->nfiles, argvCount(fc->cdict), (int)((100 * fc->fknown)/fc->nfiles), argiCount(fc->ddictx));
rpmfcPrint(buf, fc, NULL);
}
	if (print_provides > 0)	print_provides = 0;
	if (print_requires > 0)	print_requires = 0;
	P = fc->provides;	fc->provides = NULL;
	R = fc->requires;	fc->requires = NULL;
	fp = stdout;
	break;
    case RPMDEP_RPMDSCPUINFO:
	closure_name = "cpuinfo(...)";
	xx = rpmdsCpuinfo(&P, NULL);
	break;
    case RPMDEP_RPMDSRPMLIB:
	closure_name = "rpmlib(...)";
	xx = rpmdsRpmlib(&P, NULL);
	break;
    case RPMDEP_RPMDSSYSINFO:
	xx = rpmdsSysinfo(PRCO, NULL);
	P = rpmdsLink(rpmdsFromPRCO(PRCO, RPMTAG_PROVIDENAME), rpmdeps_string);
	R = rpmdsLink(rpmdsFromPRCO(PRCO, RPMTAG_REQUIRENAME), rpmdeps_string);
	break;
    case RPMDEP_RPMDSGETCONF:
	closure_name = "getconf(...)";
	xx = rpmdsGetconf(&P, NULL);
	break;
    case RPMDEP_RPMDSELF:
	closure_name = "soname(...)";
	for (i = 0; i < ac; i++)
	    xx = rpmdsELF(av[i], flags, rpmdsMergePRCO, PRCO);
	P = rpmdsLink(rpmdsFromPRCO(PRCO, RPMTAG_PROVIDENAME), rpmdeps_string);
	R = rpmdsLink(rpmdsFromPRCO(PRCO, RPMTAG_REQUIRENAME), rpmdeps_string);
	break;
    case RPMDEP_RPMDSLDCONFIG:
	closure_name = "soname(...)";
	xx = rpmdsLdconfig(PRCO, NULL);
	P = rpmdsLink(rpmdsFromPRCO(PRCO, RPMTAG_PROVIDENAME), rpmdeps_string);
	R = rpmdsLink(rpmdsFromPRCO(PRCO, RPMTAG_REQUIRENAME), rpmdeps_string);
	break;
    case RPMDEP_RPMDSUNAME:
	closure_name = "uname(...)";
	xx = rpmdsUname(&P, NULL);
	break;

    case RPMDEP_RPMDSPIPE:
	break;
    case RPMDEP_RPMDSPERL:
	closure_name = "perl(...)";
if (rpmIsVerbose())
fprintf(stderr, "\n*** Gathering %s Provides: using\n\t%s\n", closure_name, _perl_provides);
	xx = rpmdsPipe(&P, RPMTAG_PROVIDENAME, _perl_provides);
if (print_closure || rpmIsVerbose()) {
fprintf(stderr, "\n*** Gathering %s Requires: using\n\t%s\n", closure_name, _perl_requires);
	xx = rpmdsPipe(&R, RPMTAG_REQUIRENAME, _perl_requires);
	print_closure = 1;
}
	break;
    case RPMDEP_RPMDSPYTHON:
	break;
    case RPMDEP_RPMDSLIBTOOL:
	closure_name = "libtool(...)";
if (rpmIsVerbose())
fprintf(stderr, "\n*** Gathering %s Provides: using\n\t%s\n", closure_name, _libtool_provides);
	xx = rpmdsPipe(&P, RPMTAG_PROVIDENAME, _libtool_provides);
if (print_closure || rpmIsVerbose()) {
fprintf(stderr, "\n*** Gathering %s Requires: using\n\t%s\n", closure_name, _libtool_requires);
	xx = rpmdsPipe(&R, RPMTAG_REQUIRENAME, _libtool_requires);
	print_closure = 1;
}
	break;
    case RPMDEP_RPMDSPKGCONFIG:
	closure_name = "pkgconfig(...)";
if (rpmIsVerbose())
fprintf(stderr, "\n*** Gathering %s Provides: using\n\t%s\n", closure_name, _pkgconfig_provides);
	xx = rpmdsPipe(&P, RPMTAG_PROVIDENAME, _pkgconfig_provides);
if (print_closure || rpmIsVerbose()) {
fprintf(stderr, "\n*** Gathering %s Requires: using\n\t%s\n", closure_name, _pkgconfig_requires);
	xx = rpmdsPipe(&R, RPMTAG_REQUIRENAME, _pkgconfig_requires);
	print_closure = 1;
}
	break;

    case RPMDEP_RPMDSPUBKEY:
	break;
    case RPMDEP_RPMDSARCH:
	break;
    case RPMDEP_RPMDSFILE:
if (rpmIsVerbose())
fprintf(stderr, "\n*** Gathering rpmdb file Requires: using\n\t%s\n", _rpmdb_file_requires);
	xx = rpmdsPipe(&R, RPMTAG_REQUIRENAME, _rpmdb_file_requires);
	break;
    case RPMDEP_RPMDSSONAME:
	closure_name = "soname(...)";
	xx = rpmdsLdconfig(PRCO, NULL);
	P = rpmdsLink(rpmdsFromPRCO(PRCO, RPMTAG_PROVIDENAME), rpmdeps_string);
if (print_closure || rpmIsVerbose()) {
fprintf(stderr, "\n*** Gathering rpmdb soname Requires: using\n\t%s\n", _rpmdb_soname_requires);
	xx = rpmdsPipe(&R, RPMTAG_REQUIRENAME, _rpmdb_soname_requires);
	print_closure = 1;
}
	break;
    case RPMDEP_RPMDSPACKAGE:
	closure_name = "package(...)";
if (rpmIsVerbose())
fprintf(stderr, "\n*** Gathering rpmdb package Provides: using\n\t%s\n", _rpmdb_package_provides);
	xx = rpmdsPipe(&P, RPMTAG_PROVIDENAME, _rpmdb_package_provides);
if (print_closure || rpmIsVerbose()) {
fprintf(stderr, "\n*** Gathering rpmdb package Requires: using\n\t%s\n", _rpmdb_package_requires);
	xx = rpmdsPipe(&R, RPMTAG_REQUIRENAME, _rpmdb_package_requires);
	print_closure = 1;
}
	break;

    case RPMDEP_RPMDSJAVA:
	closure_name = "java(...)";
if (rpmIsVerbose())
fprintf(stderr, "\n*** Gathering %s Provides: using\n\t%s\n", closure_name, _java_provides);
	xx = rpmdsPipe(&P, RPMTAG_PROVIDENAME, _java_provides);
if (print_closure || rpmIsVerbose()) {
fprintf(stderr, "\n*** Gathering %s Requires: using\n\t%s\n", closure_name, _java_requires);
	xx = rpmdsPipe(&R, RPMTAG_REQUIRENAME, _java_requires);
	print_closure = 1;
}
	break;
    case RPMDEP_RPMDSRUBY:
	break;
    case RPMDEP_RPMDSPHP:
	break;
    case RPMDEP_RPMDSDPKGRPM:
	closure_name = "dpkgrpm(...)";
if (rpmIsVerbose())
fprintf(stderr, "\n*** Gathering dpkg Provides: using\n\t%s\n", _dpkg_provides);
	xx = rpmdsPipe(&P, RPMTAG_PROVIDENAME, _dpkg_provides);
if (print_closure || rpmIsVerbose()) {
fprintf(stderr, "\n*** Gathering rpmdb package Requires: using\n\t%s\n", _rpmdb_package_requires);
	xx = rpmdsPipe(&R, RPMTAG_REQUIRENAME, _rpmdb_package_requires);
	print_closure = 1;
}
	break;
    case RPMDEP_RPMDSRPMDPKG:
	closure_name = "rpmdpkg(...)";
if (print_closure || rpmIsVerbose()) {
fprintf(stderr, "\n*** Gathering rpmdb package Provides: using\n\t%s\n", _rpmdb_package_provides);
	xx = rpmdsPipe(&P, RPMTAG_PROVIDENAME, _rpmdb_package_provides);
	print_closure = 1;
}
if (rpmIsVerbose())
fprintf(stderr, "\n*** Gathering dpkg Requires: using\n\t%s\n", _dpkg_requires);
	xx = rpmdsPipe(&R, RPMTAG_REQUIRENAME, _dpkg_requires);
	break;
    }

    if (print_provides && P != NULL)
	xx = rpmdepPrint(NULL, P, fp);
    if (print_requires && R != NULL)
	xx = rpmdepPrint(NULL, R, fp);
    if (print_closure) {
if (rpmIsVerbose())
fprintf(stderr, "\n*** Checking %s Requires(%d): against Provides(%d): closure:\n", closure_name, rpmdsCount(R), rpmdsCount(P));
	xx = rpmdsPrintClosure(P, R, fp);
    }

    fc = rpmfcFree(fc);
    (void)rpmdsFree(P);
    P = NULL;
    (void)rpmdsFree(R);
    R = NULL;
    PRCO = rpmdsFreePRCO(PRCO);

    ec = 0;

exit:
    optCon = rpmcliFini(optCon);
    return ec;
}
