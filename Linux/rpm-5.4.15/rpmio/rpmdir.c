/** \ingroup rpmio
 * \file rpmio/rpmdir.c
 */

#include "system.h"

#include <rpmio.h>
#include <rpmurl.h>
#include <argv.h>

#define	_RPMDIR_INTERNAL
#include <rpmdir.h>
#include <rpmdav.h>		/* ftpOpendir/davOpendir */
#include <rpmhash.h>		/* hashFunctionString */

#include "debug.h"

/*@access DIR @*/

#ifdef __cplusplus
GENfree(time_t *)
GENfree(size_t *)
GENfree(rpmuint16_t *)
GENfree(AVDIR)
#endif	/* __cplusplus */

/* =============================================================== */
static void avxFini(void * _avx)
{
    rpmavx avx = (rpmavx) _avx;

    avx->mtimes = _free(avx->mtimes);
    avx->sizes = _free(avx->sizes);
    avx->modes = _free(avx->modes);
    avx->st = NULL;
    avx->av = argvFree(avx->av);
    avx->nalloced = 0;
    avx->ac = 0;
    (void) urlFree(avx->u, __FUNCTION__);
    avx->u = NULL;
    avx->uri = _free(avx->uri);
    avx->resrock = NULL;
}

/*@unchecked@*/ /*@only@*/ /*@null@*/
rpmioPool _avxPool;

static rpmavx avxGetPool(/*@null@*/ rpmioPool pool)
	/*@globals _avxPool, fileSystem @*/
	/*@modifies pool, _avxPool, fileSystem @*/
{
    rpmavx avx;

    if (_avxPool == NULL) {
	_avxPool = rpmioNewPool("avx", sizeof(*avx), -1, _av_debug,
			NULL, NULL, avxFini);
	pool = _avxPool;
    }
    avx = (rpmavx) rpmioGetPool(pool, sizeof(*avx));
    memset(((char *)avx)+sizeof(avx->_item), 0, sizeof(*avx)-sizeof(avx->_item));
    return avx;
}

void * rpmavxNew(const char *uri, struct stat *st)
{
    rpmavx avx;
    urlinfo u;

/*@-globs@*/	/* FIX: h_errno annoyance. */
    if (urlSplit(uri, &u))
	return NULL;
/*@=globs@*/

    avx = avxGetPool(_avxPool);
    avxFini(avx);		/* XXX trash-and-burn */

    /* XXX +1 byte for pesky trailing '/' */
    avx->uri = strcpy((char *)xmalloc(strlen(uri) + 1 + 1), uri);

    avx->u = urlLink(u, __FUNCTION__);
/*@-temptrans@*/	/* XXX note the assignment */
    if ((avx->st = st) != NULL)
	memset(avx->st, 0, sizeof(*avx->st));
/*@=temptrans@*/
    
    return rpmavxLink(avx);
}

int rpmavxAdd(rpmavx avx, const char * path,
		mode_t mode, size_t size, time_t mtime)
{
    int xx;

if (_av_debug < 0)
fprintf(stderr, "--> %s(%p,\"%s\", %06o, 0x%x, 0x%x)\n", __FUNCTION__, avx, path, (unsigned)mode, (unsigned)size, (unsigned)mtime);

    xx = argvAdd(&avx->av, path);

    while (avx->ac >= avx->nalloced) {
	if (avx->nalloced <= 0)
	    avx->nalloced = 1;
	avx->nalloced *= 2;
	avx->modes = (rpmuint16_t *) xrealloc(avx->modes,
				(sizeof(*avx->modes) * avx->nalloced));
	avx->sizes = (size_t *) xrealloc(avx->sizes,
				(sizeof(*avx->sizes) * avx->nalloced));
	avx->mtimes = (time_t *) xrealloc(avx->mtimes,
				(sizeof(*avx->mtimes) * avx->nalloced));
    }

    avx->modes[avx->ac] = (rpmuint16_t)mode;
    avx->sizes[avx->ac] = size;
    avx->mtimes[avx->ac] = mtime;
    avx->ac++;
    return 0;
}

int avClosedir(/*@only@*/ DIR * dir)
{
    AVDIR avdir = (AVDIR)dir;

if (_av_debug)
fprintf(stderr, "--> avClosedir(%p)\n", avdir);

#if defined(WITH_PTHREADS)
/*@-moduncon -noeffectuncon @*/
    (void) pthread_mutex_destroy(&avdir->lock);
/*@=moduncon =noeffectuncon @*/
#endif

    avdir = _free(avdir);
    return 0;
}

DIR * avOpendir(const char * path, const char ** av, rpmuint16_t * modes)
{
    AVDIR avdir;
    struct dirent * dp;
    size_t nb;
    const char ** nav;
    unsigned char * dt;
    char * t;
    int ac, nac;

if (_av_debug)
fprintf(stderr, "--> avOpendir(%s, %p, %p)\n", path, av, modes);

    nb = 0;
    ac = 0;
    if (av != NULL)
    while (av[ac] != NULL)
	nb += strlen(av[ac++]) + 1;
    ac += 2;	/* for "." and ".." */
    nb += sizeof(".") + sizeof("..");

    nb += sizeof(*avdir) + sizeof(*dp) + ((ac + 1) * sizeof(*av)) + (ac + 1);
    avdir = (AVDIR) xcalloc(1, nb);
/*@-abstract@*/
    dp = (struct dirent *) (avdir + 1);
    nav = (const char **) (dp + 1);
    dt = (unsigned char *) (nav + (ac + 1));
    t = (char *) (dt + ac + 1);
/*@=abstract@*/

    avdir->fd = avmagicdir;
/*@-usereleased@*/
    avdir->data = (char *) dp;
/*@=usereleased@*/
    avdir->allocation = nb;
    avdir->size = ac;
    avdir->offset = -1;
    /* Hash the directory path for a d_ino analogue. */
    avdir->filepos = hashFunctionString(0, path, 0);

#if defined(WITH_PTHREADS)
/*@-moduncon -noeffectuncon -nullpass @*/
    (void) pthread_mutex_init(&avdir->lock, NULL);
/*@=moduncon =noeffectuncon =nullpass @*/
#endif

    nac = 0;
    /*@-dependenttrans -unrecog@*/
    dt[nac] = (unsigned char)DT_DIR; nav[nac++] = t; t = stpcpy(t, "."); t++;
    dt[nac] = (unsigned char)DT_DIR; nav[nac++] = t; t = stpcpy(t, ".."); t++;
    /*@=dependenttrans =unrecog@*/

    /* Append av strings to DIR elements. */
    ac = 0;
    if (av != NULL)
    while (av[ac] != NULL) {
	if (modes != NULL)
	    switch (modes[ac] & S_IFMT) {
	    case S_IFIFO: dt[nac]=(unsigned char)DT_FIFO;/*@switchbreak@*/break;
	    case S_IFCHR: dt[nac]=(unsigned char)DT_CHR; /*@switchbreak@*/break;
	    case S_IFDIR: dt[nac]=(unsigned char)DT_DIR; /*@switchbreak@*/break;
	    case S_IFBLK: dt[nac]=(unsigned char)DT_BLK; /*@switchbreak@*/break;
	    case S_IFREG: dt[nac]=(unsigned char)DT_REG; /*@switchbreak@*/break;
	    case S_IFLNK: dt[nac]=(unsigned char)DT_LNK; /*@switchbreak@*/break;
/*@-unrecog@*/
	    case S_IFSOCK:dt[nac]=(unsigned char)DT_SOCK;/*@switchbreak@*/break;
/*@=unrecog@*/
	    default:	  dt[nac]=(unsigned char)DT_UNKNOWN;/*@switchbreak@*/break;
	    }
	else
	    dt[nac] = (unsigned char)DT_UNKNOWN;
/*@-dependenttrans@*/
	nav[nac++] = t;
/*@=dependenttrans@*/
	t = stpcpy(t, av[ac++]);
	t++;	/* trailing \0 */
    }
    nav[nac] = NULL;

/*@-kepttrans@*/
    return (DIR *) avdir;
/*@=kepttrans@*/
}

struct dirent * avReaddir(DIR * dir)
{
    /* XXX Enabling breaks Readdir(3) compatibility. */
    /* XXX Disabling triggers 302 retries with forced reconnects. */
    static int _append_pesky_trailing_slash = 1;
    AVDIR avdir = (AVDIR)dir;
    struct dirent * dp;
    const char ** av;
    unsigned char * dt;
    char * t;
    int ac;
    int i;

    if (avdir == NULL || !ISAVMAGIC(avdir) || avdir->data == NULL) {
	errno = EFAULT;		/* XXX better errno's */
	dp = NULL;
	goto exit;
    }

    dp = (struct dirent *) avdir->data;
    av = (const char **) (dp + 1);
    ac = (int)avdir->size;
    dt = (unsigned char *) (av + (ac + 1));
    i = avdir->offset + 1;

    if (i < 0 || i >= ac || av[i] == NULL) {
	errno = EFAULT;		/* XXX better errno's */
	dp = NULL;
	goto exit;
    }

    avdir->offset = i;

    /* XXX glob(3) uses REAL_DIR_ENTRY(dp) test on d_ino */
/*@-type@*/
    /* Hash the name (starting with parent hash) for a d_ino analogue. */
    dp->d_ino = hashFunctionString(avdir->filepos, dp->d_name, 0);

#if !defined(__DragonFly__) && !defined(__CYGWIN__)
    dp->d_reclen = 0;		/* W2DO? */
#endif

#if !(defined(hpux) || defined(__hpux) || defined(sun) || defined(RPM_OS_AIX) || defined(__CYGWIN__) || defined(__QNXNTO__))
#if !defined(__APPLE__) && !defined(__FreeBSD_kernel__) && !defined(__FreeBSD__) && !defined(__NetBSD__) && !defined(__DragonFly__) && !defined(__OpenBSD__)
    dp->d_off = (off_t)i;
#endif
    dp->d_type = dt[i];
#endif
/*@=type@*/

    t = stpncpy(dp->d_name, av[i], sizeof(dp->d_name));

    /* XXX Always append the pesky trailing '/'? */
    if (_append_pesky_trailing_slash) {
	size_t nt = (t - dp->d_name);
	if (nt > 0 && nt < sizeof(dp->d_name))
	switch (dt[i]) {
	case DT_DIR:
	    if (t[nt-1] != '/')
		*t++ = '/';
	    *t = '\0';
	    /*@fallthrough@*/
	default:
	    break;
	}
    }

exit:
if (_av_debug)
fprintf(stderr, "<-- avReaddir(%p) %p %s\n", (void *)avdir, dp, (dp ? dp->d_name : ""));

    return dp;
}

static void avRewinddir(DIR * dir)
	/*@*/
{
    AVDIR avdir = (AVDIR)dir;

    if (avdir != NULL && ISAVMAGIC(avdir))
	avdir->offset = (off_t)-1;
    return;
}

static int avScandir(const char * path, struct dirent *** nl,
		int (*filter) (const struct dirent *),
		int (*compar) (const void *, const void *))
	/*@*/
{
    DIR * dir;
    struct dirent * dp;
    int rc = -1;	/* assume failure */

    if ((dir = Opendir(path)) == NULL)
	goto exit;

    rc = 0;
    while ((dp = Readdir(dir)) != NULL) {
	if (filter != NULL && !(*filter)(dp))
	    continue;
	rc++;
	if (nl == NULL)
	    continue;
	*nl = (struct dirent **) (rc == 1
		? xmalloc(rc * sizeof(**nl))
		: xrealloc(*nl, rc * sizeof(**nl)));
	(*nl)[rc - 1] = (struct dirent *)
		memcpy(xmalloc(sizeof(*dp)), dp, sizeof(*dp));
    }

    (void) Closedir(dir);

    if (compar != NULL && nl != NULL && rc > 1)
	qsort(*nl, rc, sizeof(**nl), compar);

exit:
    return rc;
}

static void avSeekdir(DIR * dir, off_t offset)
	/*@*/
{
    AVDIR avdir = (AVDIR)dir;
    struct dirent * dp;
    const char ** av;
    int ac;

    if (avdir == NULL || !ISAVMAGIC(avdir) || avdir->data == NULL)
	return;

    dp = (struct dirent *) avdir->data;
    av = (const char **) (dp + 1);
    ac = (int)avdir->size;

    if (offset < 0 || offset >= ac || av[offset] == NULL)
	return;

    avdir->offset = offset - 1;		/* XXX set to previous entry */

    return;
}

static off_t avTelldir(DIR * dir)
	/*@globals errno @*/
	/*@modifies errno @*/
{
    AVDIR avdir = (AVDIR)dir;
    off_t offset = -1;
    struct dirent * dp;
    const char ** av;
    int ac;

    if (avdir != NULL && ISAVMAGIC(avdir) && avdir->data != NULL) {
	dp = (struct dirent *) avdir->data;
	av = (const char **) (dp + 1);
	ac = (int)avdir->size;
	offset = avdir->offset;
    }

    if (offset < 0 || offset >= ac || av[offset] == NULL)
	errno = EBADF;

    return offset;
}

/* =============================================================== */
int Closedir(DIR * dir)
{
if (_rpmio_debug)
fprintf(stderr, "*** Closedir(%p)\n", (void *)dir);
    if (dir == NULL)
	return 0;
    if (ISAVMAGIC(dir))
	return avClosedir(dir);
    return closedir(dir);
}

DIR * Opendir(const char * path)
	/*@globals ftpBufAlloced, ftpBuf @*/
	/*@modifies ftpBufAlloced, ftpBuf @*/
{
    const char * lpath;
    int ut = urlPath(path, &lpath);

if (_rpmio_debug)
fprintf(stderr, "*** Opendir(%s)\n", path);
    switch (ut) {
    case URL_IS_FTP:
	return ftpOpendir(path);
	/*@notreached@*/ break;
    case URL_IS_HTTPS:	
    case URL_IS_HTTP:
#ifdef WITH_NEON
	return davOpendir(path);
#endif
	/*@notreached@*/ break;
    case URL_IS_PATH:
	path = lpath;
	/*@fallthrough@*/
    case URL_IS_UNKNOWN:
	break;
    case URL_IS_DASH:
    case URL_IS_HKP:
    case URL_IS_MONGO:		/* XXX FIXME */
    default:
	return NULL;
	/*@notreached@*/ break;
    }
    /*@-dependenttrans@*/
    return opendir(path);
    /*@=dependenttrans@*/
}

struct dirent * Readdir(DIR * dir)
{
if (_rpmio_debug)
fprintf(stderr, "*** Readdir(%p)\n", (void *)dir);
    if (dir == NULL)
	return NULL;
    if (ISAVMAGIC(dir))
	return avReaddir(dir);
    return readdir(dir);
}

void Rewinddir(DIR * dir)
{
if (_rpmio_debug)
fprintf(stderr, "*** Rewinddir(%p)\n", (void *)dir);
    if (ISAVMAGIC(dir))
	avRewinddir(dir);
    else
	rewinddir(dir);
    return;
}

int Scandir(const char * path, struct dirent *** nl,
		int (*filter) (const struct dirent *),
		int (*compar) (const void *, const void *))
{
    const char * lpath;
    int ut = urlPath(path, &lpath);
    int rc = 0;

    switch (ut) {
    case URL_IS_DASH:
    case URL_IS_HKP:
    case URL_IS_FTP:
    case URL_IS_HTTP:
    case URL_IS_HTTPS:	
    case URL_IS_MONGO:		/* XXX FIXME */
    default:
	rc = avScandir(path, nl, filter, compar);
	break;
    case URL_IS_UNKNOWN:
	lpath = path;
	/*@fallthrough@*/
    case URL_IS_PATH:
	break;
    }

    if (!rc)
	rc = scandir(lpath, nl, filter,
	    (int (*)(const struct dirent **, const struct dirent **))compar);

if (_rpmio_debug)
fprintf(stderr, "*** Scandir(\"%s\", %p, %p, %p) rc %d\n", path, nl, filter, compar, rc);
    return rc;
}

int Alphasort(const void * a, const void * b)
{
    struct dirent *const * adp = (struct dirent *const *) a;
    struct dirent *const * bdp = (struct dirent *const *) b;
#if defined(HAVE_STRCOLL)
    return strcoll((*adp)->d_name, (*bdp)->d_name);
#else
    return strcmp((*adp)->d_name, (*bdp)->d_name);
#endif
}

int Versionsort(const void * a, const void * b)
{
    struct dirent *const * adp = (struct dirent *const *) a;
    struct dirent *const * bdp = (struct dirent *const *) b;
#if defined(HAVE_STRVERSCMP)
    return strverscmp((*adp)->d_name, (*bdp)->d_name);
#else
    return strcmp((*adp)->d_name, (*bdp)->d_name);
#endif
}

void Seekdir(DIR * dir, off_t offset)
{
if (_rpmio_debug)
fprintf(stderr, "*** Seekdir(%p,%ld)\n", (void *)dir, (long)offset);
    if (ISAVMAGIC(dir))
	avSeekdir(dir, offset);
    else
	seekdir(dir, offset);
    return;
}

off_t Telldir(DIR * dir)
{
    off_t offset = 0;

    offset = (ISAVMAGIC(dir) ? avTelldir(dir) : telldir(dir));
if (_rpmio_debug)
fprintf(stderr, "*** Telldir(%p) off %ld\n", (void *)dir, (long)offset);
    return offset;
}
