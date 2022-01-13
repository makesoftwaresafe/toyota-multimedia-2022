
#include "system.h"

#include <rpmio.h>
#include <rpmiotypes.h>
#include <rpmtypes.h>

#include <rpmxar.h>
#define	_RPMWF_INTERNAL
#include <rpmwf.h>

#include "debug.h"

/*@access FD_t @*/
/*@access rpmxar @*/	/* XXX fprintf */

/*@unchecked@*/
int _rpmwf_debug = 0;

rpmRC rpmwfPushXAR(rpmwf wf, const char * fn)
{
    char * b = NULL;
    size_t nb = 0;
    int xx;

    if (!strcmp(fn, "Lead")) {
	b = wf->l;
	nb = wf->nl;
    } else
    if (!strcmp(fn, "Signature")) {
	b = wf->s;
	nb = wf->ns;
    } else
    if (!strcmp(fn, "Header")) {
	b = wf->h;
	nb = wf->nh;
    } else
    if (!strcmp(fn, "Payload")) {
	b = wf->p;
	nb = wf->np;
    }

if (_rpmwf_debug)
fprintf(stderr, "==> rpmwfPushXAR(%p, %s) %p[%u]\n", wf, fn, b, (unsigned) nb);

    if (!(b && nb > 0))
	return RPMRC_FAIL;
    xx = rpmxarPush(wf->xar, fn, (unsigned char *)b, nb);
    return (xx == 0 ? RPMRC_OK : RPMRC_FAIL);
}

rpmRC rpmwfPullXAR(rpmwf wf, const char * fn)
{
    rpmRC rc = RPMRC_OK;
    char * b = NULL;
    size_t nb = 0;
    int xx;

    xx = rpmxarPull(wf->xar, fn);
    if (xx == 1)
	return RPMRC_NOTFOUND;
    xx = rpmxarSwapBuf(wf->xar, NULL, 0, (unsigned char **)&b, &nb);

if (_rpmwf_debug)
fprintf(stderr, "==> rpmwfPullXAR(%p, %s) %p[%u]\n", wf, fn, b, (unsigned) nb);

    if (!strcmp(fn, "Lead")) {
	wf->l = b;
	wf->nl = nb;
    } else
    if (!strcmp(fn, "Signature")) {
	wf->s = b;
	wf->ns = nb;
    } else
    if (!strcmp(fn, "Header")) {
	wf->h = b;
	wf->nh = nb;
    } else
    if (!strcmp(fn, "Payload")) {
	wf->p = b;
	wf->np = nb;
    } else
	rc = RPMRC_NOTFOUND;

    return rc;
}

rpmRC rpmwfFini(rpmwf wf)
{
    int xx;

if (_rpmwf_debug)
fprintf(stderr, "==> rpmwfFini(%p)\n", wf);

    if (wf->b && wf->b != (void *)-1) {
	xx = munmap(wf->b, wf->nb);
	wf->b = NULL;
    }
    if (wf->fd) {
	(void) Fclose(wf->fd);
	wf->fd = NULL;
    }
    return RPMRC_OK;
}

static size_t hSize(rpmuint32_t *p)
	/*@*/
{
    return (8 + 8 + 16 * ntohl(p[2]) + ntohl(p[3]));
}

rpmRC rpmwfInit(rpmwf wf, const char * fn, const char * fmode)
{
if (_rpmwf_debug)
fprintf(stderr, "==> rpmwfInit(%p, %s, %s)\n", wf, fn, fmode);
    if (fn == NULL)
	fn = wf->fn;
assert(fn != NULL);

/*@-globs@*/
    wf->fd = Fopen(fn, fmode);
/*@=globs@*/
    if (wf->fd == NULL || Ferror(wf->fd)) {
	(void) rpmwfFini(wf);
	return RPMRC_NOTFOUND;
    }

    if (fmode && *fmode == 'r') {
	wf->b = mmap(NULL, wf->nb, PROT_READ, MAP_SHARED, Fileno(wf->fd), 0L);

	if (wf->b == (void *)-1) {
	    wf->b = NULL;
	    (void) rpmwfFini(wf);
	    return RPMRC_NOTFOUND;
	}

	wf->l = (char *) wf->b;
assert(wf->l != NULL);
	wf->nl = 96;

	wf->s = wf->l + wf->nl;
	wf->ns = hSize((rpmuint32_t *)wf->s);
	wf->ns += ((8 - (wf->ns % 8)) % 8);	/* padding */

	wf->h = wf->s + wf->ns;
	wf->nh = hSize((rpmuint32_t *)wf->h);

	wf->p = wf->h + wf->nh;
	wf->np = wf->nb;
	wf->np -= wf->nl + wf->ns + wf->nh;
    }

    return RPMRC_OK;
}

rpmRC rpmwfPushRPM(rpmwf wf, const char * fn)
{
    char * b = NULL;
    size_t nb = 0;

    if (!strcmp(fn, "Lead")) {
	b = wf->l;
	nb = wf->nl;
    } else
    if (!strcmp(fn, "Signature")) {
	b = wf->s;
	nb = wf->ns;
    } else
    if (!strcmp(fn, "Header")) {
	b = wf->h;
	nb = wf->nh;
    } else
    if (!strcmp(fn, "Payload")) {
	b = wf->p;
	nb = wf->np;
    }

    if (!(b && nb > 0))
	return RPMRC_NOTFOUND;

if (_rpmwf_debug)
fprintf(stderr, "==> rpmwfPushRPM(%p, %s) %p[%u]\n", wf, fn, b, (unsigned) nb);

    if (Fwrite(b, sizeof(b[0]), nb, wf->fd) != nb)
	return RPMRC_FAIL;

    return RPMRC_OK;
}

/*@-mustmod@*/
static void rpmwfScrub(void *_wf)
	/*@globals fileSystem, internalState @*/
	/*@modifies _wf, fileSystem, internalState @*/
{
    rpmwf wf = (rpmwf) _wf;

    if (wf->b == NULL) {
/*@-dependenttrans -onlytrans @*/	/* rpm needs dependent, xar needs only */
	wf->l = _free(wf->l);
	wf->s = _free(wf->s);
	wf->h = _free(wf->h);
	wf->p = _free(wf->p);
/*@=dependenttrans =onlytrans @*/
    }

    wf->xar = rpmxarFree(wf->xar, "rpmwfFree");
    (void) rpmwfFini(wf);

    wf->fn = _free(wf->fn);
}
/*@=mustmod@*/

/*@unchecked@*/ /*@only@*/ /*@null@*/
rpmioPool _rpmwfPool;

static rpmwf rpmwfGetPool(/*@null@*/ rpmioPool pool)
	/*@globals _rpmwfPool, fileSystem @*/
	/*@modifies pool, _rpmwfPool, fileSystem @*/
{
    rpmwf wf;

    if (_rpmwfPool == NULL) {
	_rpmwfPool = rpmioNewPool("wf", sizeof(*wf), -1, _rpmwf_debug,
			NULL, NULL, rpmwfScrub);
	pool = _rpmwfPool;
    }
    return (rpmwf) rpmioGetPool(pool, sizeof(*wf));
}

rpmwf rpmwfNew(const char * fn)
{
    struct stat sb, *st = &sb;
    rpmwf wf;
    int xx;

/*@-globs@*/
    if ((xx = Stat(fn, st)) < 0)
	return NULL;
/*@=globs@*/
    wf = rpmwfGetPool(_rpmwfPool);
    wf->fn = xstrdup(fn);
    wf->nb = (size_t)st->st_size;

    return rpmwfLink(wf, "rpmwfNew");
}

static void rpmwfDumpItem(const char * item, unsigned char * b, size_t bsize)
	/*@*/
{
/*@+charint -modfilesys @*/
    fprintf(stderr, "\t%s:\t%p[%u]\t%02x%02x%02x%02x%02x%02x%02x%02x\n", item, b, (unsigned)bsize, b[0], b[1], b[2], b[3], b[4], b[5], b[6], b[7]);
/*@=charint =modfilesys @*/
}

static void rpmwfDump(rpmwf wf, const char * msg, const char * fn)
	/*@*/
{
/*@-modfilesys @*/
    fprintf(stderr, "==> %s(%s) wf %p\n", msg, fn, wf);
/*@=modfilesys @*/
/*@-noeffect@*/
    rpmwfDumpItem("     Lead", (unsigned char *)wf->l, wf->nl);
    rpmwfDumpItem("Signature", (unsigned char *)wf->s, wf->ns);
    rpmwfDumpItem("   Header", (unsigned char *)wf->h, wf->nh);
    rpmwfDumpItem("  Payload", (unsigned char *)wf->p, wf->np);
/*@=noeffect@*/
}

rpmwf rdRPM(const char * rpmfn)
{
    rpmwf wf;
    rpmRC rc;

    if ((wf = rpmwfNew(rpmfn)) == NULL)
	return wf;

    if ((rc = rpmwfInit(wf, NULL, "r")) != RPMRC_OK) {
	wf = rpmwfFree(wf);
	return NULL;
    }

/*@-noeffect@*/
if (_rpmwf_debug) rpmwfDump(wf, "rdRPM", rpmfn);
/*@=noeffect@*/

    return wf;
}

rpmwf rdXAR(const char * xarfn)
{
    rpmwf wf;
    rpmRC rc;

    if ((wf = rpmwfNew(xarfn)) == NULL)
	return wf;

    wf->xar = rpmxarNew(wf->fn, "r");
    if (wf->xar == NULL) {
	wf = rpmwfFree(wf);
	return NULL;
    }

    while (rpmxarNext(wf->xar) == 0)
	rc = rpmwfPullXAR(wf, NULL);
    wf->xar = rpmxarFree(wf->xar, "rdXAR");

/*@-noeffect@*/
if (_rpmwf_debug) rpmwfDump(wf, "rdXAR", xarfn);
/*@=noeffect@*/

    return wf;
}

rpmRC wrXAR(const char * xarfn, rpmwf wf)
{
    rpmRC rc;

/*@-noeffect@*/
if (_rpmwf_debug) rpmwfDump(wf, "wrXAR", xarfn);
/*@=noeffect@*/

    wf->xar = rpmxarNew(xarfn, "w");
    if (wf->xar == NULL)
	return RPMRC_FAIL;

    if ((rc = rpmwfPushXAR(wf, "Lead")) != RPMRC_OK)
	goto exit;
    if ((rc = rpmwfPushXAR(wf, "Signature")) != RPMRC_OK)
	goto exit;
    if ((rc = rpmwfPushXAR(wf, "Header")) != RPMRC_OK)
	goto exit;
    if ((rc = rpmwfPushXAR(wf, "Payload")) != RPMRC_OK)
	goto exit;

exit:
    wf->xar = rpmxarFree(wf->xar, "wrXAR");
    return rc;
}

rpmRC wrRPM(const char * rpmfn, rpmwf wf)
{
    rpmRC rc;

    if ((rc = rpmwfInit(wf, rpmfn, "w")) != RPMRC_OK)
	goto exit;

if (_rpmwf_debug)
fprintf(stderr, "==> wrRPM(%s) wf %p\n\tLead %p[%u]\n\tSignature %p[%u]\n\tHeader %p[%u]\n\tPayload %p[%u]\n", rpmfn, wf, wf->l, (unsigned)wf->nl, wf->s, (unsigned) wf->ns, wf->h, (unsigned) wf->nh, wf->p, (unsigned) wf->np);

    if ((rc = rpmwfPushRPM(wf, "Lead")) != RPMRC_OK)
	goto exit;
    if ((rc = rpmwfPushRPM(wf, "Signature")) != RPMRC_OK)
	goto exit;
    if ((rc = rpmwfPushRPM(wf, "Header")) != RPMRC_OK)
	goto exit;
    if ((rc = rpmwfPushRPM(wf, "Payload")) != RPMRC_OK)
	goto exit;

exit:
    (void) rpmwfFini(wf);

    return rc;
}
