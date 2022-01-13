#include "system.h"
#include <argv.h>

#if defined(WITH_MRBEMBED)
#include <mruby.h>
#include <mruby/array.h>
#include <mruby/compile.h>
#include <mruby/dump.h>
#include <mruby/variable.h>
#endif

#define _RPMMRB_INTERNAL 1
#include "rpmmrb.h"

#include "debug.h"

/*@unchecked@*/
int _rpmmrb_debug = 0;

/*@unchecked@*/ /*@relnull@*/
rpmmrb _rpmmrbI = NULL;

/**
 * Finalizes a MRuby interpreter instance/pool item
 */
static void rpmmrbFini(void *_mrb)
        /*@globals fileSystem @*/
        /*@modifies *_mrb, fileSystem @*/
{
    rpmmrb mrb = (rpmmrb) _mrb;

#if defined(WITH_MRBEMBED)
    mrb_state * I = (mrb_state *) mrb->I;
    mrbc_context * C = (mrbc_context *) mrb->C;
    if (C)
	mrbc_context_free(I, C);
    if (I)
	mrb_close(I); 
#endif
    mrb->I = NULL;
}

/**
* The pool of MRuby interpreter instances
* @see rpmioPool
*/
/*@unchecked@*/ /*@only@*/ /*@null@*/
rpmioPool _rpmmrbPool;

/**
 * Returns the current rpmio pool responsible for MRuby interpreter instances
 *
 * This is a wrapper function that returns the current rpmio pool responsible
 * for embedded MRuby interpreters. It creates and initializes a new pool when
 * there is no current pool.
 *
 * @return The current pool
 * @see rpmioNewPool
 */
static rpmmrb rpmmrbGetPool(/*@null@*/ rpmioPool pool)
        /*@globals _rpmmrbPool, fileSystem @*/
        /*@modifies pool, _rpmmrbPool, fileSystem @*/
{
    rpmmrb mrb;

    if (_rpmmrbPool == NULL) {
        _rpmmrbPool = rpmioNewPool("mrb", sizeof(*mrb), -1, _rpmmrb_debug,
            NULL, NULL, rpmmrbFini);
        pool = _rpmmrbPool;
    } 

    return (rpmmrb) rpmioGetPool(pool, sizeof(*mrb));
}

#if defined(WITH_MRBEMBED)
#ifdef	NOTYET
/** Initializes MRuby's StringIO for storing output in a string. */
/*@unchecked@*/
static const char * rpmmrbInitStringIO = "\
require 'stringio'\n\
$stdout = StringIO.new($result, \"w+\")\n\
";
#endif
#endif	/* WITH_MRBEMBED */

static rpmmrb rpmmrbI(void)
        /*@globals _rpmmrbI @*/
        /*@modifies _rpmmrbI @*/
{
    if (_rpmmrbI == NULL)
        _rpmmrbI = rpmmrbNew(NULL, 0);
    return _rpmmrbI;
}

rpmmrb rpmmrbNew(char **av, uint32_t flags)
{
    static const char *_av[] = { "rpmmrb", NULL };
    int ac;
    
    /* XXX FIXME: recurse like every other embedded interpreter. */
    if (_rpmmrbI)
        return _rpmmrbI;

    rpmmrb mrb = rpmmrbGetPool(_rpmmrbPool);

    if (av == NULL)
        av = (char **) _av;
    ac = argvCount((ARGV_t)av);

#if defined(WITH_MRBEMBED)
    mrb_state * I = mrb_open();
assert(I);
    mrb->I = (void *) I;

    mrb_value ARGV = mrb_ary_new_capa(I, ac);
    int i;
    for (i = 0; i < ac; i++)
	mrb_ary_push(I, ARGV, mrb_str_new_cstr(I, av[i]));
    mrb_define_global_const(I, "ARGV", ARGV);

    mrbc_context * C = mrbc_context_new(I);
assert(C);
    mrb->C = (void *) C;

				/* XXX -v and --version */
				/* XXX --copyright */
    C->dump_result = FALSE;	/* XXX -v and --verbose */
    C->no_exec = FALSE;		/* XXX -c --check */

    /* Set $0 */
    mrb_sym zero_sym = mrb_intern_lit(I, "$0");
#ifdef	NOTYET
    if (args.rfp) {		/* $0 arg is a file or stdin */
	char * cmdline = args.cmdline ? args.cmdline : "-";
	mrbc_filename(I, C, cmdline);
	mrb_gv_set(I, zero_sym, mrb_str_new_cstr(I, cmdline));
    } else			/* $0 arg is -e */
#endif
    {
	mrbc_filename(I, C, "-e");
	mrb_gv_set(I, zero_sym, mrb_str_new_lit(I, "-e"));
    }

#endif	/* WITH_MRBEMBED */

    return rpmmrbLink(mrb);
}

rpmRC rpmmrbRun(rpmmrb mrb, const char *str, const char **resultp)
{
    rpmRC rc = RPMRC_FAIL;

if (_rpmmrb_debug)
fprintf(stderr, "==> %s(%p,%s,%p)\n", __FUNCTION__, mrb, str, resultp);

    if (mrb == NULL)
        mrb = rpmmrbI();

#if defined(WITH_MRBEMBED)
    if (str) {
	mrb_state * I = (mrb_state *) mrb->I;
	mrbc_context * C = (mrbc_context *) mrb->C;
	mrb_value v;

	/* Load program */
#ifdef	NOTYET
	if (args.mrbfile)	/* XXX -b uses binary irep */
	    v = mrb_load_irep_file_cxt(I, args.rfp, C);
	else if (args.rfp)	/* XXX arg is a file */
	    v = mrb_load_file_cxt(I, args.rfp, C);
	else
#endif
	    v = mrb_load_string_cxt(I, str, C);
	fflush(stdout);		/* XXX FIXME */

	if (I->exc) {
	    if (!mrb_undef_p(v))
		mrb_print_error(I);
	    rc = RPMRC_FAIL;
	} else if (C->no_exec) {
	    fprintf(stderr, "Syntax OK\n");
	    rc = RPMRC_OK;
	} else
	    rc = RPMRC_OK;
    }
#endif

    return rc;
}
