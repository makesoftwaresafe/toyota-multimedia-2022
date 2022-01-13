#include "system.h"
#include <stdarg.h>

#define _RPMIOB_INTERNAL	/* XXX necessary? */
#include <rpmiotypes.h>
#include <argv.h>

#ifdef	WITH_SQUIRREL
#include <squirrel.h>
#include <sqstdaux.h>
#include <sqstdblob.h>
#include <sqstdio.h>
#include <sqstdmath.h>
#include <sqstdstring.h>
#include <sqstdsystem.h>
#endif

#define _RPMSQUIRREL_INTERNAL
#include "rpmsquirrel.h"

#include "debug.h"

/*@unchecked@*/
int _rpmsquirrel_debug = 0;

/*@unchecked@*/ /*@relnull@*/
rpmsquirrel _rpmsquirrelI = NULL;

static void rpmsquirrelFini(void * _squirrel)
        /*@globals fileSystem @*/
        /*@modifies *_squirrel, fileSystem @*/
{
    rpmsquirrel squirrel = (rpmsquirrel) _squirrel;

#if defined(WITH_SQUIRREL)
    sq_close((HSQUIRRELVM)squirrel->I);
#endif
    squirrel->I = NULL;
    (void)rpmiobFree(squirrel->iob);
    squirrel->iob = NULL;
}

/*@unchecked@*/ /*@only@*/ /*@null@*/
rpmioPool _rpmsquirrelPool;

static rpmsquirrel rpmsquirrelGetPool(/*@null@*/ rpmioPool pool)
        /*@globals _rpmsquirrelPool, fileSystem @*/
        /*@modifies pool, _rpmsquirrelPool, fileSystem @*/
{
    rpmsquirrel squirrel;

    if (_rpmsquirrelPool == NULL) {
        _rpmsquirrelPool = rpmioNewPool("squirrel", sizeof(*squirrel), -1, _rpmsquirrel_debug,
                        NULL, NULL, rpmsquirrelFini);
        pool = _rpmsquirrelPool;
    }
    return (rpmsquirrel) rpmioGetPool(pool, sizeof(*squirrel));
}

#if defined(WITH_SQUIRREL)
static void rpmsquirrelPrint(HSQUIRRELVM v, const SQChar *s, ...)
{
    rpmsquirrel squirrel = sq_getforeignptr(v);
    size_t nb = 1024;
    char * b = xmalloc(nb);
    va_list va;

    va_start(va, s);
    while(1) {
	int nw = vsnprintf(b, nb, s, va);
	if (nw > -1 && (size_t)nw < nb)
	    break;
	if (nw > -1)		/* glibc 2.1 (and later) */
	    nb = nw+1;
	else			/* glibc 2.0 */
	    nb *= 2;
	b = xrealloc(b, nb);
    }
    va_end(va);

    (void) rpmiobAppend(squirrel->iob, b, 0);
    b = _free(b);
}

#if defined(SQUIRREL_VERSION_NUMBER) && SQUIRREL_VERSION_NUMBER >= 300
static void rpmsquirrelStderr(HSQUIRRELVM v, const SQChar *s,...)
{
    va_list vl;
    va_start(vl, s);
    vfprintf(stderr, s, vl);
    va_end(vl);
}
#endif
#endif

/* XXX FIXME: honor 0x8000000 in flags to use global interpreter */
static rpmsquirrel rpmsquirrelI(void)
	/*@globals _rpmsquirrelI @*/
	/*@modifies _rpmsquirrelI @*/
{
    if (_rpmsquirrelI == NULL)
	_rpmsquirrelI = rpmsquirrelNew(NULL, 0);
    return _rpmsquirrelI;
}

rpmsquirrel rpmsquirrelNew(char ** av, uint32_t flags)
{
    rpmsquirrel squirrel =
#ifdef	NOTYET
	(flags & 0x80000000) ? rpmsquirrelI() :
#endif
	rpmsquirrelGetPool(_rpmsquirrelPool);

#if defined(WITH_SQUIRREL)
    static char * _av[] = { "rpmsquirrel", NULL };
    SQInteger stacksize = 1024;
    HSQUIRRELVM v = sq_open(stacksize);
    int ac;
    int i;

    if (av == NULL) av = _av;
    ac = argvCount((ARGV_t)av);

assert(v);
    squirrel->I = v;
    sq_setforeignptr(v, squirrel);

#if defined(SQUIRREL_VERSION_NUMBER) && SQUIRREL_VERSION_NUMBER >= 300
    sq_setprintfunc(v, rpmsquirrelPrint, rpmsquirrelStderr);
#else
    sq_setprintfunc(v, rpmsquirrelPrint);
#endif

    sqstd_seterrorhandlers(v);

    /* Initialize libraries. */
    sqstd_register_bloblib(v);
    sqstd_register_iolib(v);
    sqstd_register_mathlib(v);
    sqstd_register_stringlib(v);
    sqstd_register_systemlib(v);

    /* Register globals (using squirrelsh conventions). */
#ifdef	NOTYET
    SetSqString(v, "SHELL_VERSION", SHELL_VERSION_STR, SQTrue);
    SetSqString(v, "SQUIRREL_VERSION", SQUIRREL_VERSION_SHORT, SQTrue);
    SetSqString(v, "PLATFORM", SHELL_PLATFORM, SQTrue);
    SetSqString(v, "CPU_ARCH", SHELL_CPUARCH, SQTrue);
#else
    sq_pushconsttable(v);
    sq_pushstring(v, "SQUIRREL_VERSION", -1);
    sq_pushstring(v, SQUIRREL_VERSION, -1);
assert(!SQ_FAILED(sq_newslot(v, -3, SQFalse)));
    sq_pop(v, 1);
#endif

    /* Pass argv[argc] (using squirrelsh conventions). */
    sq_pushroottable(v);
    sq_pushstring(v, "__argc", -1);
    sq_pushinteger(v, (SQInteger)ac);
assert(!SQ_FAILED(sq_newslot(v, -3, SQFalse)));
    sq_pushstring(v, "__argv", -1);
    sq_newarray(v, 0);
    for (i = 0; av[i]; ++i) {
	sq_pushstring(v, av[i], -1);
	sq_arrayappend(v, -2);
    }
assert(!SQ_FAILED(sq_newslot(v, -3, SQFalse)));
    sq_pop(v, 1);
#endif	/* defined(WITH_SQUIRREL) */

    squirrel->iob = rpmiobNew(0);

    return rpmsquirrelLink(squirrel);
}

rpmRC rpmsquirrelRunFile(rpmsquirrel squirrel, const char * fn, const char ** resultp)
{
    rpmRC rc = RPMRC_FAIL;
    rpmiob iob = NULL;
    char * b = NULL;

if (_rpmsquirrel_debug)
fprintf(stderr, "==> %s(%p,%s)\n", __FUNCTION__, squirrel, fn);

    if (squirrel == NULL) squirrel = rpmsquirrelI();

    if (fn == NULL)
	goto exit;

    /* Read the file */
    rc = rpmiobSlurp(fn, &iob);
    if (rc)
	goto exit;

    /* XXX Change #! into a comment. */
    for (b = rpmiobStr(iob); *b; b++) {
	if (xisspace(*b))
	    continue;
	if (b[0] == '#' && b[1] == '!')
	    b[0] = b[1] = '/';
	break;
    }
    rc = rpmsquirrelRun(squirrel, b, resultp);

exit:
    iob = rpmiobFree(iob);
    return rc;
}

rpmRC rpmsquirrelRun(rpmsquirrel squirrel, const char * str, const char ** resultp)
{
    rpmRC rc = RPMRC_FAIL;

if (_rpmsquirrel_debug)
fprintf(stderr, "==> %s(%p,%s)\n", __FUNCTION__, squirrel, str);

    if (squirrel == NULL) squirrel = rpmsquirrelI();

#if defined(WITH_SQUIRREL)
    if (str != NULL) {
	size_t ns = strlen(str);
	if (ns > 0) {
	    HSQUIRRELVM v = squirrel->I;
	    SQBool raise = SQFalse;
	    SQInteger oldtop = sq_gettop(v);
	    SQRESULT res = sq_compilebuffer(v, str, ns, __FUNCTION__, raise);

	    if (SQ_SUCCEEDED(res)) {
		SQInteger retval = 0;
		sq_pushroottable(v);
		res = sq_call(v, 1, retval, raise);
	    }

	    sq_settop(v, oldtop);
	}
	rc = RPMRC_OK;
	if (resultp)
	    *resultp = rpmiobStr(squirrel->iob);
    }
#endif
    return rc;
}
