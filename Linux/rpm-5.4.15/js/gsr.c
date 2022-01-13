
#include "system.h"

#include <rpmiotypes.h>
#include <rpmio.h>
#include <rpmlog.h>
#include <argv.h>

#define	_RPMJS_INTERNAL
#include <rpmjs.h>

#include <rpmcli.h>

#include "debug.h"

extern const char * __progname;

/*==============================================================*/

#define F_ISSET(_js, _FLAG) ((_js)->flags & RPMJS_FLAGS_##_FLAG)

#define _RPMJS_OPTIONS  \
    (RPMJS_FLAGS_STRICT | RPMJS_FLAGS_RELIMIT | RPMJS_FLAGS_ANONFUNFIX | RPMJS_FLAGS_JIT)

extern struct rpmjs_s _rpmgsr;

static const char * Icode;	/* String with JavaScript program in it */
static const char * Ifn;	/* Filename with JavaScript program in it */
static int _debug;		/* 0 = no debug, bigger = more debug */
static int _loglvl;

/*@unchecked@*/
static const char * _acknack = "\
function ack(cmd, expected) {\n\
  try {\n\
    actual = eval(cmd);\n\
  } catch(e) {\n\
    print(\"NACK:  ack(\"+cmd+\")\tcaught '\"+e+\"'\");\n\
    return;\n\
  }\n\
  if (actual != expected && expected != undefined)\n\
    print(\"NACK:  ack(\"+cmd+\")\tgot '\"+actual+\"' not '\"+expected+\"'\");\n\
  else if (loglvl)\n\
    print(\"       ack(\"+cmd+\")\tgot '\"+actual+\"'\");\n\
}\n\
function nack(cmd, expected) {\n\
  try {\n\
    actual = eval(cmd);\n\
  } catch(e) {\n\
    print(\" ACK: nack(\"+cmd+\")\tcaught '\"+e+\"'\");\n\
    return;\n\
  }\n\
  if (actual == expected)\n\
    print(\" ACK: nack(\"+cmd+\")\tgot '\"+actual+\"' not '\"+expected+\"'\");\n\
  else if (loglvl)\n\
    print(\"      nack(\"+cmd+\")\tgot '\"+actual+\"'\");\n\
}\n\
";

/*==============================================================*/

/** GPSEE uses panic() to panic, expects embedder to provide */
void __attribute__ ((noreturn)) panic(const char *message);
void __attribute__ ((noreturn)) panic(const char *message)
{
    rpmlog(RPMLOG_EMERG, "%s: Fatal Error: %s\n", __progname, message);
    exit(1);
}

static void
rpmjsAppendTest(const char * fn)
{
    char * str;

    if (Icode == NULL) {
	char dstr[32];
	char lstr[32];

	sprintf(dstr, "%d", _debug);
	sprintf(lstr, "%d", _loglvl);
	Icode = rpmExpand(
		"var debug = ", dstr, ";\n"
		"var loglvl = ", lstr, ";\n",
		_acknack, NULL);
    }

    str = rpmExpand(Icode,
		"\ntry { require('system').include('", fn,
		"'); } catch(e) { print(e); print(e.stack); throw e; }", NULL);
    Icode = _free(Icode);
    Icode = str;

}

static void rpmgsrArgCallback(poptContext con,
                /*@unused@*/ enum poptCallbackReason reason,
                const struct poptOption * opt, /*@unused@*/ const char * arg,
                /*@unused@*/ void * data)
	/*@globals fileSystem @*/
	/*@modifies fileSystem @*/
{
    rpmjs js = &_rpmgsr;

    /* XXX avoid accidental collisions with POPT_BIT_SET for flags */
    if (opt->arg == NULL)
    switch (opt->val) {
    case 'F':
	js->flags |= RPMJS_FLAGS_SKIPSHEBANG;
	Ifn = xstrdup(arg);
	break;
    case 'T':
	rpmjsAppendTest(arg);
	break;
    case 'd':
	_debug++;
	_loglvl = 1;		/* XXX -d imlies loglvl = 1 */
	_rpmjs_zeal = 2;	/* XXX -d implies GCzeal = 2 */
	break;
    case 'z':
	_rpmjs_zeal++;
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

static struct poptOption _gsrOptionsTable[] = {
/*@-type@*/ /* FIX: cast? */
 { NULL, '\0', POPT_ARG_CALLBACK | POPT_CBFLAG_INC_DATA | POPT_CBFLAG_CONTINUE,
        rpmgsrArgCallback, 0, NULL, NULL },
/*@=type@*/

  { NULL, 'c', POPT_ARG_STRING,		&Icode, 0,
        N_("Specifies literal JavaScript code to execute"), N_(" code") },
  { NULL, 'f', POPT_ARG_STRING,		&Ifn, 0,
        N_("Specifies the filename containing code to run"), N_(" filename") },
  { NULL, 'F', POPT_ARG_STRING,		NULL, 'F',
        N_("Like -f, but skip shebang if present"), N_(" filename") },
  { "test", 'T', POPT_ARG_STRING,	NULL, 'T',
        N_("Like -f, but prepend ack() when including"), N_(" filename") },

    /* XXX non-functional in GPSEE gsr atm */
  { "noexec", 'n', POPT_BIT_SET,	&_rpmgsr.flags, RPMJS_FLAGS_NOEXEC,
	N_("Engine will load and parse, but not run, the script"), NULL },

  { "debug", 'd', POPT_ARG_NONE|POPT_ARGFLAG_DOC_HIDDEN,	NULL, 'd',
        N_("Increase debugging"), NULL },
  { "zeal", 'z', POPT_ARG_NONE,	NULL, 'z',
        N_("Increase GC Zealousness"), NULL },

  POPT_TABLEEND
};

extern struct poptOption rpmjsIPoptTable[];

static struct poptOption _optionsTable[] = {
/*@-type@*/ /* FIX: cast? */
 { NULL, '\0', POPT_ARG_CALLBACK | POPT_CBFLAG_INC_DATA | POPT_CBFLAG_CONTINUE,
        rpmgsrArgCallback, 0, NULL, NULL },
/*@=type@*/

  { NULL, '\0', POPT_ARG_INCLUDE_TABLE, _gsrOptionsTable, 0,
        N_("Command options:"), NULL },

  { NULL, '\0', POPT_ARG_INCLUDE_TABLE, rpmjsIPoptTable, 0,
        N_("JS interpreter options:"), NULL },

  { NULL, '\0', POPT_ARG_INCLUDE_TABLE, rpmcliAllPoptTable, 0,
	N_("Common options for all rpm executables:"), NULL },

  POPT_AUTOALIAS
  POPT_AUTOHELP

  POPT_TABLEEND
};

static struct poptOption *optionsTable = &_optionsTable[0];

/*==============================================================*/

int main(int argc, char *argv[])
{
    poptContext optCon;
    rpmjs js = &_rpmgsr;
    char *const * Iargv;	/* Becomes arguments array in JS program */
    const char * result = NULL;
    rpmRC rc;
    int ac = 0;
    int ec = 1;		/* assume failure */

    js->flags = _rpmjs_options | _RPMJS_OPTIONS;

#ifdef	NOTYET
    _rpmcli_popt_context_flags = POPT_CONTEXT_POSIXMEHARDER;
#endif
    optCon = rpmcliInit(argc, argv, optionsTable);

    /* Add files from CLI. */
    Iargv = (char *const *)poptGetArgs(optCon);
    ac = argvCount((ARGV_t)Iargv);

    switch (ac) {
    case 0:
	if (Icode || Ifn)
	    break;
	/*@fallthrough@*/
    default:
	poptPrintUsage(optCon, stderr, 0);
	goto exit;
	/*@notreached@*/ break;
    }

    js = rpmjsNew((const char **)Iargv, js->flags);

    /* Run JavaScript specified with -c */
    if (Icode) {
	rc = rpmjsRun(js, Icode, &result);

	if (result != NULL && *result != '\0') {
	    fprintf(stdout, "%s\n", result);
	    fflush(stdout);
	}

	ec = (rc == RPMRC_OK ? 0 : 1);
	goto finish;
    }

    /* Run JavaScript specified with -f */
    if (ac != 0)	/* XXX FIXME */
	js->flags |= RPMJS_FLAGS_SKIPSHEBANG;

    result = NULL;
    switch ((rc = rpmjsRunFile(js, Ifn, &result))) {
    case RPMRC_FAIL:
	rpmlog(RPMLOG_ERR, "%s: Unable to open' script '%s'! (%s)\n",
		      __progname, Ifn, result);
	result = _free(result);
	ec = 1;
	break;
    default:
	ec = -rc;	/* XXX hack tp get I->exitCode into ec */
	break;
    }

finish:
    js = rpmjsFree(js);

exit:
    Icode = _free(Icode);
    Ifn = _free(Ifn);

    optCon = rpmcliFini(optCon);

    return ec;
}
