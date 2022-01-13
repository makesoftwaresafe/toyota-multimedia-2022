#include "system.h"

/* XXX Get rid of the pugly #ifdef's */
#if defined(WITH_XAR) && defined(HAVE_XAR_H)

#include "xar.h"

#if defined(__LCLINT__)
/*@-incondefs -redecl@*/
/*@null@*/
xar_t xar_open(const char *file, int32_t flags)
	/*@*/;
int xar_close(/*@only@*/ xar_t x)
	/*@globals fileSystem @*/
	/*@modifies x, fileSystem @*/;
/*@null@*/
xar_iter_t xar_iter_new(void)
	/*@*/;
/*@null@*/
xar_file_t xar_file_first(xar_t x, xar_iter_t i)
	/*@modifies x, i @*/;
/*@null@*/
xar_file_t xar_file_next(xar_iter_t i)
	/*@modifies i @*/;
/*@null@*/
xar_file_t xar_add_frombuffer(xar_t x, /*@null@*/ xar_file_t parent,
		const char *name, char *buffer, size_t length)
	/*@globals fileSystem @*/
	/*@modifies x, fileSystem @*/;
int32_t xar_extract_tobuffersz(xar_t x, xar_file_t f,
		char **buffer, size_t *size)
	/*@globals fileSystem @*/
	/*@modifies x, f, *buffer, *size @*/;
/*@only@*/
char *xar_get_path(xar_file_t f)
	/*@*/;
/*@=incondefs =redecl@*/

#endif	/* __LCLINT__ */

#else	/* WITH_XAR */
#define	READ	0
#define	WRITE	1
#define	xar_open(_fn, _f)	(NULL)
#define	xar_close(_x)	(1)
#define	xar_iter_new()		(NULL)
#define	xar_iter_free(_i)
#define	xar_file_first(_x, _i)	(NULL)
#define	xar_file_next(_i)	(NULL)
#define	xar_add_frombuffer(_x, _parent, _fn, _b, _bsize)	(NULL)
#define	xar_extract_tobuffersz(_x, _f, _b, _bsize)	(1)
#define	xar_get_path(_f)	"*No XAR*"
#define xar_opt_set(_a1, _a2, _a3) (1)
#define XAR_OPT_COMPRESSION 0
#define XAR_OPT_VAL_NONE 0
#define XAR_OPT_VAL_GZIP 0
#endif	/* WITH_XAR */

#define	_RPMXAR_INTERNAL
#include <rpmxar.h>
#include <rpmio_internal.h>	/* for fdGetXAR */
#include <rpmhash.h>		/* hashFunctionString */
#include <ugid.h>

#include "debug.h"

/*@access FD_t @*/

/*@unchecked@*/
int _xar_debug = 0;

/*@unchecked@*/ /*@only@*/ /*@null@*/
rpmioPool _xarPool;

/*@-globuse -mustmod@*/
static void rpmxarFini(void * _xar)
	/*@globals fileSystem @*/
        /*@modifies _xar, fileSystem @*/
{
    rpmxar xar = (rpmxar) _xar;
    if (xar->i) {
	xar_iter_free(xar->i);
	xar->i = NULL;
    }
    if (xar->x) {
	int xx;
	xx = xar_close(xar->x);
	xar->x = NULL;
    }

    xar->member = _free(xar->member);
    xar->b = _free(xar->b);
}
/*@=globuse =mustmod@*/

static rpmxar rpmxarGetPool(/*@null@*/ rpmioPool pool)
	/*@globals _xarPool, fileSystem @*/
	/*@modifies pool, _xarPool, fileSystem @*/
{
    rpmxar xar;

    if (_xarPool == NULL) {
	_xarPool = rpmioNewPool("xar", sizeof(*xar), -1, _xar_debug,
			NULL, NULL, rpmxarFini);
	pool = _xarPool;
    }
    xar = (rpmxar) rpmioGetPool(pool, sizeof(*xar));
    memset(((char *)xar)+sizeof(xar->_item), 0, sizeof(*xar)-sizeof(xar->_item));
    return xar;
}

rpmxar rpmxarNew(const char * fn, const char * fmode)
{
    rpmxar xar = rpmxarGetPool(_xarPool);
    int flags = ((fmode && *fmode == 'w') ? WRITE : READ);

assert(fn != NULL);
    xar->x = xar_open(fn, flags);
    if (flags == READ) {
	xar->i = xar_iter_new();
	xar->first = 1;
    }
if (_xar_debug)
fprintf(stderr, "<-- %s(%s,%s) xar %p i %p x %p first %d\n", __FUNCTION__, fn, fmode, xar, xar->i, xar->x, xar->first);
    return rpmxarLink(xar, __FUNCTION__);
}

int rpmxarNext(rpmxar xar)
{
    int rc = 1;		/* assume failure */

if (_xar_debug)
fprintf(stderr, "--> %s(%p) i %p x %p first %d\n", __FUNCTION__, xar, (xar ? xar->i : NULL), (xar ? xar->x : NULL), (xar ? xar->first : -1));
    if (xar && xar->x) {
	if (xar->first) {
	    xar->f = xar_file_first(xar->x, xar->i);
	    xar->first = 0;
	} else
	    xar->f = xar_file_next(xar->i);
    }

    rc = (xar && xar->f ? 0 : 1);
if (_xar_debug)fprintf(stderr, "<-- %s(%p) rc %d\n", __FUNCTION__, xar, rc);
    return rc;
}

int rpmxarPush(rpmxar xar, const char * fn, unsigned char * b, size_t bsize)
{
    int payload = !strcmp(fn, "Payload");

/*@+charint@*/
if (_xar_debug)
fprintf(stderr, "--> rpmxarPush(%p, %s) %p[%u] %02x%02x%02x%02x%02x%02x%02x%02x\n", xar, fn, b, (unsigned)bsize, b[0], b[1], b[2], b[3], b[4], b[5], b[6], b[7]);
/*@=charint@*/

    if (xar->x && b != NULL) {
	if (payload) /* payload is already compressed */
	    (void) xar_opt_set(xar->x, XAR_OPT_COMPRESSION, XAR_OPT_VAL_NONE);
	xar->f = xar_add_frombuffer(xar->x, NULL, fn, (char *)b, bsize);
	if (payload) /* restore default xar compression */
	    (void) xar_opt_set(xar->x, XAR_OPT_COMPRESSION, XAR_OPT_VAL_GZIP);
	if (xar->f == NULL)
	    return 2;
    }
    return 0;
}

int rpmxarPull(rpmxar xar, const char * fn)
{
    int rc = 1;
#ifdef	WITH_XAR
    const char * path = xar_get_path(xar->f);

    if (fn != NULL && strcmp(fn, path)) {
	path = _free(path);
	return rc;
    }
    xar->member = _free(xar->member);
    xar->member = path;

    xar->b = _free(xar->b);
    xar->bsize = xar->bx = 0;

/*@-nullstate @*/
    rc = (int) xar_extract_tobuffersz(xar->x, xar->f, (char **)&xar->b, &xar->bsize);
/*@=nullstate @*/
    if (rc)
	return 1;

/*@+charint -nullpass -nullderef @*/
if (_xar_debug) {
unsigned char * b = xar->b;
size_t bsize = xar->bsize;
fprintf(stderr, "<-- rpmxarPull(%p, %s) %p[%u] %02x%02x%02x%02x%02x%02x%02x%02x\n", xar, fn, b, (unsigned)bsize, b[0], b[1], b[2], b[3], b[4], b[5], b[6], b[7]);
}
/*@=charint =nullpass =nullderef @*/
    rc = 0;
#endif

    return rc;
}

int rpmxarSwapBuf(rpmxar xar, unsigned char * b, size_t bsize,
		unsigned char ** obp, size_t * obsizep)
{
if (_xar_debug)
fprintf(stderr, "--> rpmxarSwapBuf(%p, %p[%u], %p, %p) %p[%u]\n", xar, b, (unsigned) bsize, obp, obsizep, xar->b, (unsigned) xar->bsize);

    if (xar) {
	if (obsizep != NULL)
	    *obsizep = xar->bsize;
	if (obp != NULL) {
/*@-onlytrans@*/
	    *obp = xar->b;
/*@=onlytrans@*/
	    xar->b = NULL;
	}
	xar->b = _free(xar->b);
/*@-assignexpose -temptrans @*/
	xar->b = b;
/*@=assignexpose =temptrans @*/
	xar->bsize = bsize;
    }
/*@-nullstate@*/
    return 0;
/*@=nullstate@*/
}

ssize_t xarRead(void * cookie, /*@out@*/ char * buf, size_t count)
{
    FD_t fd = (FD_t) cookie;
    rpmxar xar = fdGetXAR(fd);
    ssize_t rc = 0;

assert(xar != NULL);
#if 0
    if ((xx = rpmxarNext(xar)) != 0)    return RPMRC_FAIL;
    if ((xx = rpmxarPull(xar, "Signature")) != 0) return RPMRC_FAIL;
    (void) rpmxarSwapBuf(xar, NULL, 0, &b, &nb);
#endif

    rc = xar->bsize - xar->bx;
    if (rc > 0) {
	if (count < (size_t)rc) rc = count;
assert(xar->b != NULL);
	memmove(buf, &xar->b[xar->bx], rc);
	xar->bx += rc;
    } else
    if (rc < 0) {
	rc = -1;
    } else
	rc = 0;

if (_xar_debug)
fprintf(stderr, "<-- %s(%p,%p,0x%x) %s %p[%u:%u] rc 0x%x\n", __FUNCTION__, cookie, buf, (unsigned)count, (xar->member ? xar->member : "(nil)"), xar->b, (unsigned)xar->bx, (unsigned)xar->bsize, (unsigned)rc);

    return rc;
}

const char * rpmxarPath(rpmxar xar)
{
    const char * path = (xar && xar->f ? xar_get_path(xar->f) : NULL);
if (_xar_debug)
fprintf(stderr, "<-- %s(%p) %s\n", __FUNCTION__, xar, path);
    return path;
}

static mode_t xarMode(rpmxar xar)
	/*@*/
{
    mode_t m = 0;
#ifdef	WITH_XAR
    const char * t = NULL;

    xar_prop_get(xar->f, "mode", &t);
    m = (t ? (mode_t) strtoll(t, NULL, 8) : 0);

    xar_prop_get(xar->f, "type", &t);
    if (!strcmp(t, "file"))
	m |= S_IFREG;
    else if (!strcmp(t, "hardlink"))
	m |= S_IFREG;
    else if (!strcmp(t, "directory"))
	m |= S_IFDIR;
    else if (!strcmp(t, "symlink"))
	m |= S_IFLNK;
    else if (!strcmp(t, "fifo"))
	m |= S_IFIFO;
    else if (!strcmp(t, "character special"))
	m |= S_IFCHR;
    else if (!strcmp(t, "block special"))
	m |= S_IFBLK;
    else if (!strcmp(t, "socket"))
	m |= S_IFSOCK;
#ifdef S_IFWHT
    else if (!strcmp(t, "whiteout"))
	m |= S_IFWHT;
#endif
#endif

    return m;
}

static dev_t xarDev(rpmxar xar)
	/*@*/
{
    unsigned major = 0;
    unsigned minor = 0;
#ifdef	WITH_XAR
    const char *t = NULL;

    xar_prop_get(xar->f, "device/major", &t);
    major = (t ? (unsigned) strtoll(t, NULL, 0) : 0);
    xar_prop_get(xar->f, "device/minor", &t);
    minor = (t ? (unsigned) strtoll(t, NULL, 0) : 0);
#endif
#ifdef makedev
    return makedev(major, minor);
#else
    return (major << 8) | minor;
#endif
}

static long long xarSize(rpmxar xar)
	/*@*/
{
    long long ll = 0L;
#ifdef	WITH_XAR
    char * t = NULL;
#if defined(HAVE_XAR_GET_SIZE)
    t = xar_get_size(xar->x, xar->f);
    ll = strtoll(t, NULL, 0);
    t = _free(t);
#else
    xar_prop_get(xar->f, "data/size", &t);
    if (t)
	ll = strtoll(t, NULL, 0);
#endif
#endif
    return ll;
}

static uid_t xarUid(rpmxar xar)
	/*@*/
{
    uid_t u = 0;
#ifdef	WITH_XAR
    const char * t = NULL;

    xar_prop_get(xar->f, "user", &t);
    if (t == NULL || unameToUid(t, &u) < 0) {
	xar_prop_get(xar->f, "uid", &t);
	u = (t ? (uid_t) strtoll(t, NULL, 0) : getuid());
    }
#endif
    return u;
}

static gid_t xarGid(rpmxar xar)
	/*@*/
{
    gid_t g = 0;
#ifdef	WITH_XAR
    const char * t = NULL;

    xar_prop_get(xar->f, "group", &t);
    if (t == NULL || gnameToGid(t, &g) < 0) {
	xar_prop_get(xar->f, "gid", &t);
	g = (t ? (gid_t) strtoll(t, NULL, 0) : getgid());
    }
#endif
    return g;
}

static void xarTime(rpmxar xar, const char * tprop, struct timeval *tv)
	/*@modifies *tvp */
{
#ifdef	WITH_XAR
    const char * t = NULL;

    xar_prop_get(xar->f, tprop, &t);
    if (t) {
	struct tm tm;
	strptime(t, "%FT%T", &tm);
	tv->tv_sec = timegm(&tm);
	tv->tv_usec = 0;
    } else
#endif
    {
	tv->tv_sec = time(NULL);
	tv->tv_usec = 0;
    }
}

int rpmxarStat(rpmxar xar, struct stat * st)
{
    int rc = -1;	/* assume failure */

    if (xar && xar->f) {
	const char * path = rpmxarPath(xar);
	memset(st, 0, sizeof(*st));
	st->st_dev = (dev_t)0;
	st->st_ino = hashFunctionString(0, path, 0);;
	path = _free(path);
	st->st_mode = xarMode(xar);
	st->st_nlink = (S_ISDIR(st->st_mode) ? 2 : 1);	/* XXX FIXME */
	st->st_uid = xarUid(xar);
	st->st_gid = xarGid(xar);
	st->st_rdev = (S_ISCHR(st->st_mode) || S_ISBLK(st->st_mode))
		? xarDev(xar) : (dev_t)0;
	st->st_size = xarSize(xar);
	st->st_blksize = (blksize_t) 0;
	st->st_blocks = (blkcnt_t) 0;
	xarTime(xar, "atime", (struct timeval *)&st->st_atime);
	xarTime(xar, "ctime", (struct timeval *)&st->st_ctime);
	xarTime(xar, "mtime", (struct timeval *)&st->st_mtime);
	rc = 0;
    }

if (_xar_debug)
fprintf(stderr, "<-- %s(%p,%p) rc %d\n", __FUNCTION__, xar, st, rc);
    return rc;
}
