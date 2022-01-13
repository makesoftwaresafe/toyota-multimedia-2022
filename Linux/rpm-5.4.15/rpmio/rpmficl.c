#include "system.h"

#include <argv.h>

#ifdef	WITH_FICL
#include <ficl.h>
#endif
#define _RPMFICL_INTERNAL
#include "rpmficl.h"

#include "debug.h"

/*@unchecked@*/
int _rpmficl_debug = 0;

/*@unchecked@*/ /*@relnull@*/
rpmficl _rpmficlI = NULL;

#if defined(WITH_FICL)
/* Capture stdout from FICL VM in a buffer. */
static void rpmficlTextOut(ficlCallback *callback, char *text)
	/*@*/
{
    rpmficl ficl = (rpmficl) callback->context;

    if (ficl->iob == NULL)
	ficl->iob = rpmiobNew(0);
    (void) rpmiobAppend(ficl->iob, text, 0);
}
#endif

static void rpmficlFini(void * _ficl)
        /*@globals fileSystem @*/
        /*@modifies *_ficl, fileSystem @*/
{
    rpmficl ficl = (rpmficl) _ficl;

#if defined(WITH_FICL)
    ficlSystem * sys = ficl->sys;
    ficlSystemDestroy(sys);
#endif
    ficl->sys = NULL;
    ficl->vm = NULL;
    (void) rpmiobFree(ficl->iob);
    ficl->iob = NULL;
}

/*@unchecked@*/ /*@only@*/ /*@null@*/
rpmioPool _rpmficlPool;

static rpmficl rpmficlGetPool(/*@null@*/ rpmioPool pool)
        /*@globals _rpmficlPool, fileSystem @*/
        /*@modifies pool, _rpmficlPool, fileSystem @*/
{
    rpmficl ficl;

    if (_rpmficlPool == NULL) {
        _rpmficlPool = rpmioNewPool("ficl", sizeof(*ficl), -1, _rpmficl_debug,
                        NULL, NULL, rpmficlFini);
        pool = _rpmficlPool;
    }
    return (rpmficl) rpmioGetPool(pool, sizeof(*ficl));
}

static rpmficl rpmficlI(void)
	/*@globals _rpmficlI @*/
	/*@modifies _rpmficlI @*/
{
    if (_rpmficlI == NULL)
	_rpmficlI = rpmficlNew(NULL, 0);
    return _rpmficlI;
}

rpmficl rpmficlNew(char ** av, uint32_t flags)
{
    rpmficl ficl =
#ifdef	NOTYET
	(flags & 0x80000000) ? rpmficlI() :
#endif
	rpmficlGetPool(_rpmficlPool);
#if defined(WITH_FICL)
    static const char * _av[] = { "rpmficl", NULL };
    ficlSystemInformation fsi;
    ficlSystem *sys;
    ficlVm *vm;
    int ac;
    int xx;

    if (av == NULL) av = _av;
    ac = argvCount((ARGV_t)av);

    ficlSystemInformationInitialize(&fsi);
    fsi.context = (void *)ficl;
    fsi.textOut = rpmficlTextOut;

    sys = ficlSystemCreate(&fsi);
    ficl->sys = sys;
    ficlSystemCompileExtras(sys);
    vm = ficlSystemCreateVm(sys);
    ficl->vm = vm;

    xx = ficlVmEvaluate(vm, ".ver .( " __DATE__ " ) cr quit");

#ifdef	NOTYET
    if (ac > 1) {
	char b[256];
        sprintf(b, ".( loading %s ) cr load %s\n cr", av[1], av[1]);
        xx = ficlVmEvaluate(vm, b);
    }
#endif

    if (ficl->iob) {
	const char * s = rpmiobStr(ficl->iob);
if (_rpmficl_debug && s && *s)
fprintf(stderr, "%s", s);
	(void)rpmiobEmpty(ficl->iob);
    }
#endif

    return rpmficlLink(ficl);
}

rpmRC rpmficlRunFile(rpmficl ficl, const char * fn, const char ** resultp)
{
    rpmRC rc = RPMRC_FAIL;

if (_rpmficl_debug)
fprintf(stderr, "==> %s(%p,%s)\n", __FUNCTION__, ficl, fn);

    if (ficl == NULL) ficl = rpmficlI();

    if (fn != NULL) {
#if defined(WITH_FICL)
	rc = RPMRC_OK;
#endif
    }
    return rc;
}

rpmRC rpmficlRun(rpmficl ficl, const char * str, const char ** resultp)
{
    rpmRC rc = RPMRC_FAIL;

if (_rpmficl_debug)
fprintf(stderr, "==> %s(%p,%s)\n", __FUNCTION__, ficl, str);

    if (ficl == NULL) ficl = rpmficlI();

    if (str != NULL) {
#if defined(WITH_FICL)
	ficlVm *vm = ficl->vm;
	switch (ficlVmEvaluate(vm, (char *)str)) {
	default:
	    break;
	case FICL_VM_STATUS_OUT_OF_TEXT:
	    rc = RPMRC_OK;
	    if (resultp && ficl->iob)
		*resultp = rpmiobStr(ficl->iob);
	    break;
	}
#endif
    }
    return rc;
}
