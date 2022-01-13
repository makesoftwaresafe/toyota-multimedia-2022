#include "system.h"

#include <argv.h>

#undef	_	/* XXX everyone gotta be different */
#define _RPMPERL_INTERNAL
#include "rpmperl.h"
#include <rpmmacro.h>

#if defined(WITH_PERLEMBED)
#include <EXTERN.h>
#include <perl.h>
#endif

#undef	UNLIKELY	/* XXX everyone gotta be different */
#include "debug.h"

/*@unchecked@*/
int _rpmperl_debug = 0;

/*@unchecked@*/ /*@relnull@*/
rpmperl _rpmperlI = NULL;

#define	my_perl	((PerlInterpreter *)perl->I)

static void rpmperlFini(void * _perl)
        /*@globals fileSystem @*/
        /*@modifies *_perl, fileSystem @*/
{
    rpmperl perl = (rpmperl) _perl;

#if defined(WITH_PERLEMBED)
    PERL_SET_CONTEXT(my_perl);
    PL_perl_destruct_level = 1;
    perl_destruct(my_perl);
    perl_free(my_perl);
    if (perl == _rpmperlI)	/* XXX necessary on HP-UX? */
	PERL_SYS_TERM();
#endif
    perl->I = NULL;
}

/*@unchecked@*/ /*@only@*/ /*@null@*/
rpmioPool _rpmperlPool;

static rpmperl rpmperlGetPool(/*@null@*/ rpmioPool pool)
        /*@globals _rpmperlPool, fileSystem @*/
        /*@modifies pool, _rpmperlPool, fileSystem @*/
{
    rpmperl perl;

    if (_rpmperlPool == NULL) {
        _rpmperlPool = rpmioNewPool("perl", sizeof(*perl), -1, _rpmperl_debug,
                        NULL, NULL, rpmperlFini);
        pool = _rpmperlPool;
    }
    return (rpmperl) rpmioGetPool(pool, sizeof(*perl));
}

#if defined(WITH_PERLEMBED)
EXTERN_C void xs_init (PerlInterpreter * _my_perl PERL_UNUSED_DECL);

EXTERN_C void boot_DynaLoader (PerlInterpreter* _my_perl, CV* cv);

EXTERN_C void
xs_init(PerlInterpreter* _my_perl PERL_UNUSED_DECL)
{
	char * file = (char *) __FILE__;
	dXSUB_SYS;

	/* DynaLoader is a special case */
	Perl_newXS(_my_perl, "DynaLoader::boot_DynaLoader", boot_DynaLoader, file);
}

/*@unchecked@*/
static const char * _rpmperlI_init = "\
use strict;\n\
use IO::String;\n\
our $io = IO::String->new;\n\
select $io;\n\
";
#endif

static rpmperl rpmperlI(void)
	/*@globals _rpmperlI @*/
	/*@modifies _rpmperlI @*/
{
    if (_rpmperlI == NULL)
	_rpmperlI = rpmperlNew(NULL, 0);
    return _rpmperlI;
}

rpmperl rpmperlNew(char ** av, uint32_t flags)
{
    rpmperl perl =
#ifdef	NOTYET
	(flags & 0x80000000) ? rpmperlI() :
#endif
	rpmperlGetPool(_rpmperlPool);
#if defined(WITH_PERLEMBED)
    static const char * _av[] = { "rpmperl", NULL };
    static int initialized = 0;
    ARGV_t argv = NULL;
    int argc = 0;
    int xx;

    if (av == NULL) av = (char **) _av;

    /* Build argv(argc) for the interpreter. */
    xx = argvAdd(&argv, av[0]);
    {	static const char _perlI_init[] = "%{?_perlI_init}";
	const char * s = rpmExpand(_rpmperlI_init, _perlI_init, NULL);
	if (s && *s) {
	    xx = argvAdd(&argv, "-e");
	    xx = argvAdd(&argv, s);
	}
	s = _free(s);
    }
    if (av[1])
	xx = argvAppend(&argv, (ARGV_t)av+1);
    argc = argvCount(argv);

    if (!initialized) {
	/* XXX claimed necessary on HP-UX */
	PERL_SYS_INIT3(&argc, (char ***)&argv, &environ);
	initialized++;
    }
    perl->I = perl_alloc();
    PERL_SET_CONTEXT(my_perl);
    PL_perl_destruct_level = 1;
    perl_construct(my_perl);

    PL_origalen = 1; /* don't let $0 assignment update proctitle/embedding[0] */
    xx = perl_parse(my_perl, xs_init, argc, (char **)argv, NULL);
    PL_exit_flags |= PERL_EXIT_DESTRUCT_END;
    perl_run(my_perl);

    argv = argvFree(argv);
#endif

    return rpmperlLink(perl);
}

rpmRC rpmperlRun(rpmperl perl, const char * str, const char ** resultp)
{
    rpmRC rc = RPMRC_FAIL;

if (_rpmperl_debug)
fprintf(stderr, "==> %s(%p,%s)\n", __FUNCTION__, perl, str);

    if (perl == NULL) perl = rpmperlI();

    if (str != NULL) {
#if defined(WITH_PERLEMBED)
	STRLEN n_a;
	SV * retSV;

	retSV = Perl_eval_pv(my_perl, str, TRUE);
	if (SvTRUE(ERRSV)) {
	    fprintf(stderr, "==> FIXME #1: %d %s\n",
		(int)SvTRUE(ERRSV), SvPV(ERRSV, n_a));
	} else {
	    if (resultp) {
		retSV = Perl_eval_pv(my_perl, "${$io->string_ref}", TRUE);
		if (SvTRUE(ERRSV)) {
		    fprintf(stderr, "==> FIXME #2: %d %s\n",
			(int)SvTRUE(ERRSV), SvPV(ERRSV, n_a));
		} else {
		    *resultp = SvPV(retSV, n_a);
		    rc = RPMRC_OK;
		}
	    } else
		rc = RPMRC_OK;
	}
#endif
    }
    return rc;
}
