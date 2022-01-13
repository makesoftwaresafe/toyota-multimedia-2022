
#include "system.h"

#include <rpmiotypes.h>
#include <rpmlog.h>
#include <poptIO.h>

#define	_RPMSM_INTERNAL
#include <rpmsm.h>

#include "debug.h"

#define F_ISSET(_sm, _FLAG) (((_sm)->flags & ((RPMSM_FLAGS_##_FLAG) & ~0x40000000)) != RPMSM_FLAGS_NONE)

/**
 */
static void rpmsmArgCallback(poptContext con,
                /*@unused@*/ enum poptCallbackReason reason,
                const struct poptOption * opt, const char * arg,
                /*@unused@*/ void * data)
        /*@globals _rpmsm, h_errno, fileSystem, internalState @*/
        /*@modifies _rpmsm, fileSystem, internalState @*/
{
    rpmsm sm = _rpmsmI;
    char * t = NULL;
    int xx;

    /* XXX avoid accidental collisions with POPT_BIT_SET for flags */
    if (opt->arg == NULL)
    switch (opt->val) {
    case 'b':
	sm->flags |= RPMSM_FLAGS_BASE;
	sm->flags |= RPMSM_FLAGS_CREATE;
	t = rpmExpand("  ", arg, NULL); *t = opt->val;
	xx = argvAdd(&sm->av, t);
	break;
    case 'i':
	sm->flags |= RPMSM_FLAGS_INSTALL;
	t = rpmExpand("  ", arg, NULL); *t = opt->val;
	xx = argvAdd(&sm->av, t);
	break;
    case 'l':
	sm->flags |= RPMSM_FLAGS_LIST;
	t = rpmExpand("  ", arg, NULL); *t = opt->val;
	xx = argvAdd(&sm->av, t);
	break;
    case 'r':
	sm->flags |= RPMSM_FLAGS_REMOVE;
	t = rpmExpand("  ", arg, NULL); *t = opt->val;
	xx = argvAdd(&sm->av, t);
	break;
    case 'u':
	sm->flags |= RPMSM_FLAGS_UPGRADE;
	t = rpmExpand("  ", arg, NULL); *t = opt->val;
	xx = argvAdd(&sm->av, t);
	break;
    case 'R':
	sm->flags |= RPMSM_FLAGS_RELOAD;
	arg = "";	/* XXX no arg for --reload */
	t = rpmExpand("  ", arg, NULL); *t = opt->val;
	xx = argvAdd(&sm->av, t);
	break;
    case 'n':
	sm->flags &= ~RPMSM_FLAGS_RELOAD;
	break;
    case 's':
	sm->fn = _free(sm->fn);
	sm->fn = xstrdup(arg);
	break;
    case 'B':
	sm->flags |= RPMSM_FLAGS_REBUILD;
	arg = "";	/* XXX no arg for --build */
	t = rpmExpand("  ", arg, NULL); *t = opt->val;
	xx = argvAdd(&sm->av, t);
	break;
    case 'D':
	sm->flags |= RPMSM_FLAGS_NOAUDIT;
	break;

    case 'h':
    case '?':
    default:
	fprintf(stderr, _("%s: Unknown option -%c\n"), __progname, opt->val);
	poptPrintUsage(con, stderr, 0);
	exit(EXIT_FAILURE);
	/*@notreached@*/ break;
    }
    t = _free(t);
}

/*@unchecked@*/ /*@observer@*/
static struct poptOption rpmsmOtherTable[] = {
/*@-type@*/ /* FIX: cast? */
 { NULL, '\0', POPT_ARG_CALLBACK | POPT_CBFLAG_INC_DATA | POPT_CBFLAG_CONTINUE,
        rpmsmArgCallback, 0, NULL, NULL },
/*@=type@*/
  { "store",'s', POPT_ARG_STRING,		NULL, (int)'s',
	N_("Set the STORE to operate on (e.g. \"targeted\")"), N_("STORE") },
  { "noreload",'n', POPT_ARG_NONE,		NULL, (int)'n',
	N_("Do not reload policy after commit"), NULL },
  { "disable_dontaudit",'D', POPT_ARG_NONE,	NULL, (int)'D',
	N_("Remove dontaudits from policy"), NULL },
  POPT_TABLEEND
};

/*@unchecked@*/ /*@observer@*/
static struct poptOption rpmsmOptionsTable[] = {
/*@-type@*/ /* FIX: cast? */
 { NULL, '\0', POPT_ARG_CALLBACK | POPT_CBFLAG_INC_DATA | POPT_CBFLAG_CONTINUE,
        rpmsmArgCallback, 0, NULL, NULL },
/*@=type@*/

  { "list",'l', POPT_ARG_STRING|POPT_ARGFLAG_OPTIONAL,	NULL, (int)'l',
	N_("Display list of installed policy modules"), N_("REGEX") },
  { "install",'i', POPT_ARG_STRING,			NULL, (int)'i',
	N_("Install a new module FILE"), N_("FILE") },
  { "upgrade",'u', POPT_ARG_STRING,			NULL, (int)'u',
	N_("Upgrade an existing module FILE"), N_("FILE") },
  { "base",'b', POPT_ARG_STRING,			NULL, (int)'b',
	N_("Install a new base module FILE"), N_("FILE") },
  { "remove",'r', POPT_ARG_STRING,			NULL, (int)'r',
	N_("Remove an existing MODULE"), N_("MODULE") },
  { "reload",'R', POPT_ARG_NONE,			NULL, (int)'R',
	N_("Reload policy"), NULL },
  { "build",'B', POPT_ARG_NONE,				NULL, (int)'B',
	N_("Build and reload policy"), NULL },

  { NULL, (char)-1, POPT_ARG_INCLUDE_TABLE, rpmsmOtherTable, 0,
	N_("Other options:"), NULL },

 { NULL, '\0', POPT_ARG_INCLUDE_TABLE, rpmioAllPoptTable, 0,
	N_("Common options for all rpmio executables:"),
	NULL },

  POPT_AUTOALIAS
  POPT_AUTOHELP
  POPT_TABLEEND
};

int main(int argc, char *argv[])
{
    rpmsm sm = NULL;
    poptContext optCon = NULL;
    const char ** av = NULL;
    const char * result = NULL;
    int rc = -1;	/* assume failure */

    __progname = "semodule";
    sm = rpmsmNew(NULL, 0);
    if (sm == NULL) {
	fprintf(stderr, "%s:  Could not create handle\n", __progname);
	goto exit;
    }
    _rpmsmI = sm;

    /* Parse CLI options and args. */
    optCon = rpmioInit(argc, argv, rpmsmOptionsTable);
    av = poptGetArgs(optCon);
    if (av && *av) {
	int ac = argvCount(av);
	char lcmd = (av && ac > 0 ? (*av)[0] : 0);
	int i;

	switch (lcmd) {
	default:
	    fprintf(stderr, "unknown additional arguments:\n");
	    for (i = 0; i < ac; i++)
		fprintf(stderr, " %s", av[i]);
	    goto exit;
	    /*@notreached@*/ break;
	case 'i':
	case 'u':
	case 'r':
	    for (i = 0; i < ac; i++) {
		char * t = rpmExpand("  ", av[i], NULL); *t = lcmd;
		(void) argvAdd(&sm->av, t);
		t = _free(t);
	    }
	    break;
	}
    }
    
    if (F_ISSET(sm, REBUILD) || F_ISSET(sm, RELOAD)) {
	if (sm->av && *sm->av) {
	    fprintf(stderr, "build or reload should not be used with other commands\n");
	    goto exit;
	}
    } else {
	if (!(sm->av && *sm->av)) {
	    fprintf(stderr, "At least one mode must be specified.\n");
	    goto exit;
	}
    }

    (void) signal(SIGINT, SIG_IGN);
    (void) signal(SIGQUIT, SIG_IGN);
    (void) signal(SIGTERM, SIG_IGN);

    result = NULL;
    rc = rpmsmRun(sm, (char **) sm->av, &result);
    if (result && *result) {
	const char * eol = (result[strlen(result)-1] != '\n' ? "\n" : "");
	fprintf((rc < 0 ? stderr : stdout), "%s%s", result, eol);
    }

exit:
    if (optCon)
	optCon = rpmioFini(optCon);

    return (rc < 0 ? EXIT_FAILURE : EXIT_SUCCESS);
}
