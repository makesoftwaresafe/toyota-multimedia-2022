/** \ingroup rpmio
 * \file rpmio/rpmiob.c
 */
#include "system.h"
#define	_RPMIOB_INTERNAL
#include <rpmiotypes.h>
#include <rpmio.h>
#include "debug.h"

/*@unchecked@*/
size_t _rpmiob_chunk = 1024;

/*@unchecked@*/
int _rpmiob_debug;

static void rpmiobFini(void * _iob)
{
    rpmiob iob = (rpmiob) _iob;

if (_rpmiob_debug)
fprintf(stderr, "--> %s(%p) %p[%u:%u]\n", __FUNCTION__, iob, iob->b, (unsigned)iob->blen, (unsigned)iob->allocated);
    iob->b = _free(iob->b);
    iob->blen = 0;
    iob->allocated = 0;
}

/*@unchecked@*/ /*@only@*/ /*@null@*/
rpmioPool _rpmiobPool;

static rpmiob rpmiobGetPool(/*@null@*/ rpmioPool pool)
	/*@globals _rpmiobPool, fileSystem @*/
	/*@modifies pool, _rpmiobPool, fileSystem @*/
{
    rpmiob iob;

    if (_rpmiobPool == NULL) {
	_rpmiobPool = rpmioNewPool("iob", sizeof(*iob), -1, _rpmiob_debug,
			NULL, NULL, rpmiobFini);
	pool = _rpmiobPool;
    }
    return (rpmiob) rpmioGetPool(pool, sizeof(*iob));
}

rpmiob rpmiobNew(size_t len)
{
    rpmiob iob = rpmiobGetPool(_rpmiobPool);
if (_rpmiob_debug)
fprintf(stderr, "--> %s(%p) %p[%u:%u]\n", __FUNCTION__, iob, iob->b, (unsigned)iob->blen, (unsigned)iob->allocated);
    if (len == 0)
	len = _rpmiob_chunk;
    iob->allocated = len;
    iob->blen = 0;
    iob->b = (rpmuint8_t *) xcalloc(iob->allocated+1, sizeof(*iob->b));
    return rpmiobLink(iob);
}

rpmiob rpmiobEmpty(rpmiob iob)
{
assert(iob != NULL);
    iob->b[0] = '\0';
    iob->blen = 0;
if (_rpmiob_debug)
fprintf(stderr, "<-- %s(%p) %p[%u:%u]\n", __FUNCTION__, iob, iob->b, (unsigned)iob->blen, (unsigned)iob->allocated);
    return iob;
}

rpmiob rpmiobRTrim(rpmiob iob)
{
assert(iob != NULL);
    while (iob->blen > 0 && xisspace((int)iob->b[iob->blen-1]))
	iob->b[--iob->blen] = (rpmuint8_t) '\0';
if (_rpmiob_debug)
fprintf(stderr, "<-- %s(%p) %p[%u:%u]\n", __FUNCTION__, iob, iob->b, (unsigned)iob->blen, (unsigned)iob->allocated);
    return iob;
}

rpmiob rpmiobAppend(rpmiob iob, const char * s, size_t nl)
{
    size_t ns = strlen(s);
    rpmuint8_t * tail;

    if (nl > 0) ns++;

assert(iob != NULL);
    if ((iob->blen + ns) > iob->allocated) {
	iob->allocated += ((ns+_rpmiob_chunk-1)/_rpmiob_chunk) * _rpmiob_chunk;
	iob->b = (rpmuint8_t *) xrealloc(iob->b, iob->allocated+1);
    }

    tail = iob->b + iob->blen;
    tail = (rpmuint8_t *) stpcpy((char *)tail, s);
    if (nl > 0) {
	*tail++ = (rpmuint8_t) '\n';
	*tail = (rpmuint8_t) '\0';
    }
    iob->blen += ns;
if (_rpmiob_debug)
fprintf(stderr, "<-- %s(%p,%p,%u) %p[%u:%u] \"%s\"\n", __FUNCTION__, iob, s, (unsigned)nl, iob->b, (unsigned)iob->blen, (unsigned)iob->allocated, s);
    return iob;
}

rpmuint8_t * rpmiobBuf(rpmiob iob)
{
assert(iob != NULL);
if (_rpmiob_debug)
fprintf(stderr, "<-- %s(%p) %p[%u:%u]\n", __FUNCTION__, iob, iob->b, (unsigned)iob->blen, (unsigned)iob->allocated);
/*@-retalias -retexpose -usereleased @*/
    return iob->b;
/*@=retalias =retexpose =usereleased @*/
}

char * rpmiobStr(rpmiob iob)
{
assert(iob != NULL);
if (_rpmiob_debug)
fprintf(stderr, "<-- %s(%p) %p[%u:%u]\n===============\n%s\n===============\n", __FUNCTION__, iob, iob->b, (unsigned)iob->blen, (unsigned)iob->allocated, iob->b);
/*@-retalias -retexpose -usereleased @*/
    return (char *) iob->b;
/*@=retalias =retexpose =usereleased @*/
}

size_t rpmiobLen(rpmiob iob)
{
if (_rpmiob_debug)
fprintf(stderr, "<-- %s(%p) %p[%u:%u]\n", __FUNCTION__, iob, iob->b, (unsigned)iob->blen, (unsigned)iob->allocated);
    return (iob != NULL ? iob->blen : 0);
}

int rpmiobSlurp(const char * fn, rpmiob * iobp)
{
    static size_t blenmax = (128 * BUFSIZ);	/* XXX 1Mb with glibc */
    rpmuint8_t * b = NULL;
    size_t blen = 0;
    struct stat sb;
    FD_t fd;
    int rc = 0;
    int xx;

    fd = Fopen(fn, "r.ufdio");
    if (fd == NULL || Ferror(fd)) {
	rc = 2;
	goto exit;
    }
    sb.st_size = 0;
    if ((xx = Fstat(fd, &sb)) < 0 || sb.st_size == 0)
	sb.st_size = blenmax;
#if defined(__linux__)
    /* XXX st->st_size = 0 for /proc files on linux, see stat(2). */
    /* XXX glibc mmap'd libio no workie for /proc files on linux?!? */
    if (sb.st_size == 0 && !strncmp(fn, "/proc/", sizeof("/proc/")-1)) {
	blen = blenmax;
	b = (rpmuint8_t *) xmalloc(blen+1);
	b[0] = (rpmuint8_t) '\0';

	xx = read(Fileno(fd), b, blen);
	blen = (size_t) (xx >= 0 ? xx : 0); 
    } else
#endif
    {
	blen = sb.st_size;
	b = (rpmuint8_t *) xmalloc(blen+1);
	b[0] = (rpmuint8_t) '\0';

	blen = Fread(b, sizeof(*b), blen, fd);
	if (Ferror(fd)) {
	    rc = 1;
	    goto exit;
	}
    }
    if (blen < (size_t)sb.st_size)
	b = (rpmuint8_t *) xrealloc(b, blen+1);
    b[blen] = (rpmuint8_t) '\0';

exit:
    if (fd != NULL) (void) Fclose(fd);

    if (rc == 0 && iobp != NULL) {
	/* XXX use rpmiobNew() if/when lazy iop->b alloc is implemented. */
	rpmiob iob = rpmiobGetPool(_rpmiobPool);
	iob->b = b;
	iob->blen = blen;
	iob->allocated = blen;
	*iobp = iob;
    } else {
	if (iobp)
	    *iobp = NULL;
	b = _free(b);
    }

    return rc;
}
