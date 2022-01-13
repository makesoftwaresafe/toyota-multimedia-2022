#include "system.h"

#include <argv.h>

#ifdef	WITH_TCL
#include <tcl.h>
#endif
#define _RPMTCL_INTERNAL
#include "rpmtcl.h"

#include "debug.h"

/*@unchecked@*/
int _rpmtcl_debug = 0;

/*@unchecked@*/ /*@relnull@*/
rpmtcl _rpmtclI = NULL;

static void rpmtclFini(void * _tcl)
        /*@globals fileSystem @*/
        /*@modifies *_tcl, fileSystem @*/
{
    rpmtcl tcl = (rpmtcl) _tcl;

#if defined(WITH_TCL)
    Tcl_DeleteInterp((Tcl_Interp *)tcl->I);
#endif
    tcl->I = NULL;
    (void)rpmiobFree(tcl->iob);
    tcl->iob = NULL;
}

/*@unchecked@*/ /*@only@*/ /*@null@*/
rpmioPool _rpmtclPool;

static rpmtcl rpmtclGetPool(/*@null@*/ rpmioPool pool)
        /*@globals _rpmtclPool, fileSystem @*/
        /*@modifies pool, _rpmtclPool, fileSystem @*/
{
    rpmtcl tcl;

    if (_rpmtclPool == NULL) {
        _rpmtclPool = rpmioNewPool("tcl", sizeof(*tcl), -1, _rpmtcl_debug,
                        NULL, NULL, rpmtclFini);
        pool = _rpmtclPool;
    }
    return (rpmtcl) rpmioGetPool(pool, sizeof(*tcl));
}

#if defined(WITH_TCL)
static int rpmtclIOclose(ClientData CD, Tcl_Interp *I)
	/*@*/
{
if (_rpmtcl_debug)
fprintf(stderr, "==> %s(%p, %p)\n", __FUNCTION__, CD, I);
    return 0;
}

static int rpmtclIOread(ClientData CD, char *b, int nb, int *errnop)
	/*@*/
{
if (_rpmtcl_debug)
fprintf(stderr, "==> %s(%p, %p[%d], %p)\n", __FUNCTION__, CD, b, nb, errnop);
    *errnop = EINVAL;
    return -1;
}

static int rpmtclIOwrite(ClientData CD, const char *b, int nb, int *errnop)
	/*@*/
{
    rpmtcl tcl = (rpmtcl) CD;
if (_rpmtcl_debug)
fprintf(stderr, "==> %s(%p, %p[%d], %p)\n", __FUNCTION__, CD, b, nb, errnop);
    if (nb > 0) {
	char * t = (char *)b;
	int c = t[nb];
	if (c) t[nb] = '\0';
	(void) rpmiobAppend(tcl->iob, b, 0);
	if (c) t[nb] = c;
    }
    return nb;
}

static int rpmtclIOseek(ClientData CD, long off, int mode, int *errnop)
	/*@*/
{
if (_rpmtcl_debug)
fprintf(stderr, "==> %s(%p, %ld, %d, %p)\n", __FUNCTION__, CD, off, mode, errnop);
    *errnop = EINVAL;
    return -1;
}

static Tcl_ChannelType rpmtclIO = {
    (char *)"rpmtclIO",		/* Type name */
    TCL_CHANNEL_VERSION_2,	/* Tcl_ChannelTypeVersion */
    rpmtclIOclose,		/* Tcl_DriverCloseProc */
    rpmtclIOread,		/* Tcl_DriverInputProc */
    rpmtclIOwrite,		/* Tcl_DriverOutputProc */
    rpmtclIOseek,		/* Tcl_DriverSeekProc */
    NULL,			/* Tcl_DriverSetOptionProc */
    NULL,			/* Tcl_DriverGetOptionProc */
    NULL,			/* Tcl_DriverWatchProc */
    NULL,			/* Tcl_DriverGetHandleProc */
    NULL,			/* Tcl_DriverClose2Proc */
    NULL,			/* Tcl_DriverBlockModeProc */
    NULL,			/* Tcl_DriverFlushProc */
    NULL,			/* Tcl_DriverHandlerProc */
    NULL,			/* Tcl_DriverWideSeekProc */
    NULL,			/* Tcl_DriverThreadActionProc */
#if TCL_MAJOR_VERSION == 8 && TCL_MINOR_VERSION > 4
    NULL,			/* Tcl_DriverTruncateProc */
#endif
};
#endif

static rpmtcl rpmtclI(void)
	/*@globals _rpmtclI @*/
	/*@modifies _rpmtclI @*/
{
    if (_rpmtclI == NULL)
	_rpmtclI = rpmtclNew(NULL, 0);
    return _rpmtclI;
}

rpmtcl rpmtclNew(char ** av, uint32_t flags)
{
    rpmtcl tcl =
#ifdef	NOTYET
	(flags & 0x80000000) ? rpmtclI() :
#endif
	rpmtclGetPool(_rpmtclPool);

#if defined(WITH_TCL)
    static char * _av[] = { "rpmtcl", NULL };
    Tcl_Interp * tclI = Tcl_CreateInterp();
    char b[32];
    int ac;

    if (av == NULL) av = _av;
    ac = argvCount((ARGV_t)av);

    Tcl_SetVar(tclI, "argv", Tcl_Merge(ac-1, (const char *const *)av+1), TCL_GLOBAL_ONLY);
    (void)sprintf(b, "%d", ac-1);
    Tcl_SetVar(tclI, "argc", b, TCL_GLOBAL_ONLY);
    Tcl_SetVar(tclI, "argv0", av[0], TCL_GLOBAL_ONLY);
    Tcl_SetVar(tclI, "tcl_interactive", "0", TCL_GLOBAL_ONLY);

    tcl->I = tclI;
    {	Tcl_Channel tclout = Tcl_GetStdChannel(TCL_STDOUT);
	Tcl_SetChannelOption(tclI, tclout, "-translation", "auto");
	Tcl_StackChannel(tclI, &rpmtclIO, tcl, TCL_WRITABLE, tclout);
	tcl->tclout = (void *) tclout;
    }
#endif
    tcl->iob = rpmiobNew(0);

    return rpmtclLink(tcl);
}

rpmRC rpmtclRunFile(rpmtcl tcl, const char * fn, const char ** resultp)
{
    rpmRC rc = RPMRC_FAIL;

if (_rpmtcl_debug)
fprintf(stderr, "==> %s(%p,%s)\n", __FUNCTION__, tcl, fn);

    if (tcl == NULL) tcl = rpmtclI();

#if defined(WITH_TCL)
    if (fn != NULL && Tcl_EvalFile((Tcl_Interp *)tcl->I, fn) == TCL_OK) {
	rc = RPMRC_OK;
	if (resultp)
	    *resultp = rpmiobStr(tcl->iob);
    }
#endif
    return rc;
}

rpmRC rpmtclRun(rpmtcl tcl, const char * str, const char ** resultp)
{
    rpmRC rc = RPMRC_FAIL;

if (_rpmtcl_debug)
fprintf(stderr, "==> %s(%p,%s)\n", __FUNCTION__, tcl, str);

    if (tcl == NULL) tcl = rpmtclI();

#if defined(WITH_TCL)
    if (str != NULL && Tcl_Eval((Tcl_Interp *)tcl->I, str) == TCL_OK) {
	rc = RPMRC_OK;
	if (resultp)
	    *resultp = rpmiobStr(tcl->iob);
    }
#endif
    return rc;
}
