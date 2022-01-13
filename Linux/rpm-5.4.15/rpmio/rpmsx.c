/** \ingroup rpmio
 * \file rpmio/rpmsx.c
 */

#include "system.h"

#if defined(WITH_SELINUX)
#include <selinux/selinux.h>
#if defined(__LCLINT__)
/*@-incondefs@*/
extern void freecon(/*@only@*/ security_context_t con)
	/*@modifies con @*/;

extern int getfilecon(const char *path, /*@out@*/ security_context_t *con)
	/*@modifies *con @*/;
extern int lgetfilecon(const char *path, /*@out@*/ security_context_t *con)
	/*@modifies *con @*/;
extern int fgetfilecon(int fd, /*@out@*/ security_context_t *con)
	/*@modifies *con @*/;

extern int setfilecon(const char *path, security_context_t con)
	/*@globals fileSystem @*/
	/*@modifies fileSystem @*/;
extern int lsetfilecon(const char *path, security_context_t con)
	/*@globals fileSystem @*/
	/*@modifies fileSystem @*/;
extern int fsetfilecon(int fd, security_context_t con)
	/*@globals fileSystem @*/
	/*@modifies fileSystem @*/;

extern int getcon(/*@out@*/ security_context_t *con)
	/*@modifies *con @*/;
extern int getexeccon(/*@out@*/ security_context_t *con)
	/*@modifies *con @*/;
extern int setexeccon(security_context_t con)
	/*@globals fileSystem @*/
	/*@modifies fileSystem @*/;

extern int security_check_context(security_context_t con)
	/*@globals fileSystem @*/
	/*@modifies fileSystem @*/;
extern int security_getenforce(void)
	/*@globals fileSystem @*/
	/*@modifies fileSystem @*/;

extern int is_selinux_enabled(void)
	/*@globals fileSystem @*/
	/*@modifies fileSystem @*/;
/*@=incondefs@*/
#endif
#endif

#define	_RPMSX_INTERNAL
#include <rpmsx.h>
#include <rpmlog.h>
#include <rpmmacro.h>

#include "debug.h"

/*@unchecked@*/
int _rpmsx_debug = 0;

/*@unchecked@*/ /*@relnull@*/
rpmsx _rpmsxI = NULL;

static void rpmsxFini(void * _sx)
	/*@globals fileSystem @*/
	/*@modifies *_sx, fileSystem @*/
{
    rpmsx sx = (rpmsx) _sx;

#if defined(WITH_SELINUX)
    if (sx->fn)
	matchpathcon_fini();
#endif
    sx->flags = 0;
    sx->fn = _free(sx->fn);
}

/*@unchecked@*/ /*@only@*/ /*@null@*/
rpmioPool _rpmsxPool = NULL;

static rpmsx rpmsxGetPool(/*@null@*/ rpmioPool pool)
	/*@globals _rpmsxPool, fileSystem @*/
	/*@modifies pool, _rpmsxPool, fileSystem @*/
{
    rpmsx sx;

    if (_rpmsxPool == NULL) {
	_rpmsxPool = rpmioNewPool("sx", sizeof(*sx), -1, _rpmsx_debug,
			NULL, NULL, rpmsxFini);
	pool = _rpmsxPool;
    }
    return (rpmsx) rpmioGetPool(pool, sizeof(*sx));
}

rpmsx rpmsxNew(const char * fn, unsigned int flags)
{
    rpmsx sx = rpmsxGetPool(_rpmsxPool);

    sx->fn = NULL;
    sx->flags = flags;

#if defined(WITH_SELINUX)
    if (fn == NULL)
	fn = selinux_file_context_path();
    if (sx->flags)
	set_matchpathcon_flags(sx->flags);
    {	int rc;
	sx->fn = rpmGetPath(fn, NULL);
	rc = matchpathcon_init(sx->fn);
	/* If matchpathcon_init fails, turn off SELinux functionality. */
	if (rc < 0)
	    sx->fn = _free(sx->fn);
    }
#endif
    return rpmsxLink(sx);
}

/*@unchecked@*/ /*@null@*/
static const char * _rpmsxI_fn;
/*@unchecked@*/
static int _rpmsxI_flags;

static rpmsx rpmsxI(void)
	/*@globals _rpmsxI @*/
	/*@modifies _rpmsxI @*/
{
    if (_rpmsxI == NULL)
	_rpmsxI = rpmsxNew(_rpmsxI_fn, _rpmsxI_flags);
    return _rpmsxI;
}

int rpmsxEnabled(/*@null@*/ rpmsx sx)
{
    static int rc = 0;
#if defined(WITH_SELINUX)
    static int oneshot = 0;

    if (!oneshot) {
	rc = is_selinux_enabled();
if (_rpmsx_debug)
fprintf(stderr, "<-- %s(%p) rc %d\n", __FUNCTION__, sx, rc);
	oneshot++;
    }
#endif

    return rc;
}

const char * rpmsxMatch(rpmsx sx, const char *fn, mode_t mode)
{
    const char * scon = NULL;

    if (sx == NULL) sx = rpmsxI();

#if defined(WITH_SELINUX)
    if (sx->fn) {
	static char nocon[] = "";
	int rc = matchpathcon(fn, mode, (security_context_t *)&scon);
	if (rc < 0)
	    scon = xstrdup(nocon);
    }
#endif

if (_rpmsx_debug < 0 || (_rpmsx_debug > 0 && scon != NULL && *scon != '\0' &&strcmp("(null)", scon)))
fprintf(stderr, "<-- %s(%p,%s,0%o) \"%s\"\n", __FUNCTION__, sx, fn, mode, scon);
    return scon;
}

const char * rpmsxGetfilecon(rpmsx sx, const char *fn)
{
    const char * scon = NULL;

    if (sx == NULL) sx = rpmsxI();

if (_rpmsx_debug)
fprintf(stderr, "--> %s(%p,%s) sxfn %s\n", __FUNCTION__, sx, fn, sx->fn);

#if defined(WITH_SELINUX)
    if (sx->fn && fn) {
	security_context_t _con = NULL;
	int rc = getfilecon(fn, &_con);
	if (rc > 0 && _con != NULL)
	    scon = (const char *) _con;
	else
	    freecon(_con);
    }
#endif

if (_rpmsx_debug)
fprintf(stderr, "<-- %s(%p,%s) scon %s\n", __FUNCTION__, sx, fn, scon);
    return scon;
}

int rpmsxSetfilecon(rpmsx sx, const char *fn, mode_t mode,
		const char * scon)
{
    int rc = 0;

    if (sx == NULL) sx = rpmsxI();

if (_rpmsx_debug)
fprintf(stderr, "--> %s(%p,%s,0%o,%s) sxfn %s\n", __FUNCTION__, sx, fn, mode, scon, sx->fn);

#if defined(WITH_SELINUX)
    if (sx->fn) {
	security_context_t _con = (security_context_t)
		(scon ? scon : rpmsxMatch(sx, fn, mode));
	rc = setfilecon(fn, _con);
	if (scon == NULL) {	/* XXX free lazy rpmsxMatch() string */
	    freecon(_con);
	    _con = NULL;
	}
    }
#endif

if (_rpmsx_debug)
fprintf(stderr, "<-- %s(%p,%s,0%o,%s) rc %d\n", __FUNCTION__, sx, fn, mode, scon, rc);
    return rc;
}

const char * rpmsxLgetfilecon(rpmsx sx, const char *fn)
{
    const char * scon = NULL;

    if (sx == NULL) sx = rpmsxI();

if (_rpmsx_debug)
fprintf(stderr, "--> %s(%p,%s) sxfn %s\n", __FUNCTION__, sx, fn, sx->fn);

#if defined(WITH_SELINUX)
    if (sx->fn && fn) {
	security_context_t _con = NULL;
	int rc = lgetfilecon(fn, &_con);
	if (rc > 0 && _con != NULL)
	    scon = (const char *) _con;
	else
	    freecon(_con);
    }
#endif

if (_rpmsx_debug)
fprintf(stderr, "<-- %s(%p,%s) scon %s\n", __FUNCTION__, sx, fn, scon);
    return scon;
}

int rpmsxLsetfilecon(rpmsx sx, const char *fn, mode_t mode,
		const char * scon)
{
    int rc = 0;

    if (sx == NULL) sx = rpmsxI();

if (_rpmsx_debug)
fprintf(stderr, "--> %s(%p,%s,0%o,%s) sxfn %s\n", __FUNCTION__, sx, fn, mode, scon, sx->fn);

#if defined(WITH_SELINUX)
    if (sx->fn) {
	security_context_t _con = (security_context_t)
		(scon ? scon : rpmsxMatch(sx, fn, mode));
	rc = lsetfilecon(fn, _con);
	if (scon == NULL) {	/* XXX free lazy rpmsxMatch() string */
	    freecon(_con);
	    _con = NULL;
	}
    }
#endif

if (_rpmsx_debug)
fprintf(stderr, "<-- %s(%p,%s,0%o,%s) rc %d\n", __FUNCTION__, sx, fn, mode, scon, rc);
    return rc;
}

int rpmsxExec(rpmsx sx, int verified, const char ** argv)
{
    int rc = -1;

    if (sx == NULL) sx = rpmsxI();

if (_rpmsx_debug)
fprintf(stderr, "--> %s(%p,%d,%p)\n", __FUNCTION__, sx, verified, argv);

#if defined(WITH_SELINUX)
    rc = rpm_execcon(verified, argv[0], (char *const *)argv, environ);
#endif

if (_rpmsx_debug)
fprintf(stderr, "<-- %s(%p,%d,%p) rc %d\n", __FUNCTION__, sx, verified, argv, rc);
    return rc;
}
