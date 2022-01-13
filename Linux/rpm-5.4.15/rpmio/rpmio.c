/** \ingroup rpmio
 * \file rpmio/rpmio.c
 */

#include "system.h"
#include <stdarg.h>

#if defined(HAVE_MACHINE_TYPES_H)
# include <machine/types.h>
#endif

#if defined(HAVE_SYS_SOCKET_H)
# include <sys/socket.h>
#endif

#ifndef NI_MAXHOST
#define NI_MAXHOST      1025
#endif

#if defined(__LCLINT__)
struct addrinfo
{
  int ai_flags;			/* Input flags.  */
  int ai_family;		/* Protocol family for socket.  */
  int ai_socktype;		/* Socket type.  */
  int ai_protocol;		/* Protocol for socket.  */
  socklen_t ai_addrlen;		/* Length of socket address.  */
  struct sockaddr *ai_addr;	/* Socket address for socket.  */
  char *ai_canonname;		/* Canonical name for service location.  */
  struct addrinfo *ai_next;	/* Pointer to next in list.  */
};

/*@-exportheader -incondefs @*/
extern int getaddrinfo (__const char *__restrict __name,
			__const char *__restrict __service,
			__const struct addrinfo *__restrict __req,
			/*@out@*/ struct addrinfo **__restrict __pai)
	/*@modifies *__pai @*/;

extern int getnameinfo (__const struct sockaddr *__restrict __sa,
			socklen_t __salen, /*@out@*/ char *__restrict __host,
			socklen_t __hostlen, /*@out@*/ char *__restrict __serv,
			socklen_t __servlen, unsigned int __flags)
	/*@modifies __host, __serv @*/;

extern void freeaddrinfo (/*@only@*/ struct addrinfo *__ai)
	/*@modifies __ai @*/;
/*@=exportheader =incondefs @*/
#else
#include <netdb.h>		/* XXX getaddrinfo et al */
#endif

#include <netinet/in.h>
#include <arpa/inet.h>		/* XXX for inet_aton and HP-UX */

#if defined(HAVE_NETINET_IN_SYSTM_H)
# include <sys/types.h>
# include <netinet/in_systm.h>
#endif

#if defined(WITH_XZ)
#include <lzma.h>
#endif

#include <rpmiotypes.h>
#include <rpmmacro.h>		/* XXX rpmioAccess needs rpmCleanPath() */

#include <mongo.h>

#include <rpmaug.h>
#include <rpmficl.h>
#include <rpmgit.h>
#include <rpmjni.h>
#include <rpmjs.h>
#include <rpmlua.h>		/* XXX rpmioClean() calls rpmluaFree() */
#include <rpmmrb.h>
#include <rpmnix.h>
#include <rpmodbc.h>
#include <rpmperl.h>
#include <rpmpython.h>
#include <rpmruby.h>
#include <rpmsql.h>
#include <rpmsquirrel.h>
#include <rpmtcl.h>

#define	_RPMHKP_INTERNAL	/* XXX awol/crl bloom filters */
#include <rpmhkp.h>

#include <rpmsm.h>
#include <rpmsp.h>
#include <rpmsx.h>

#if defined(HAVE_LIBIO_H) && defined(_G_IO_IO_FILE_VERSION)
#define	_USE_LIBIO	1
#endif

/* XXX HP-UX w/o -D_XOPEN_SOURCE needs */
#if !defined(HAVE_HERRNO) && (defined(hpux) || defined(__hpux) || defined(__LCLINT__))
/*@unchecked@*/
extern int h_errno;
#endif

#ifndef IPPORT_FTP
#define IPPORT_FTP	21
#endif
#ifndef	IPPORT_HTTP
#define	IPPORT_HTTP	80
#endif

#if !defined(HAVE_INET_ATON)
#define inet_aton(cp,inp) rpm_inet_aton(cp,inp)
static int rpm_inet_aton(const char *cp, struct in_addr *inp)
	/*@modifies *inp @*/
{
    long addr;

    addr = inet_addr(cp);
    if (addr == ((long) -1)) return 0;

    memcpy(inp, &addr, sizeof(addr));
    return 1;
}
#endif

#if defined(USE_ALT_DNS) && USE_ALT_DNS
#include "dns.h"
#endif

#include <rpmio_internal.h>
#undef	fdFileno
#undef	fdOpen
#define	fdOpen	__fdOpen
#undef	fdRead
#define	fdRead	__fdRead
#undef	fdWrite
#define	fdWrite	__fdWrite
#undef	fdClose
#define	fdClose	__fdClose

#include <ugid.h>
#include <rpmcb.h>
#include <rpmdav.h>

#include "debug.h"

/*@access FILE @*/	/* XXX to permit comparison/conversion with void *. */
/*@access urlinfo @*/
/*@access FDSTAT_t @*/
/*@access rpmxar @*/
/*@access pgpDig @*/

#define FDTO(fd)	(fd ? ((FD_t)fd)->rd_timeoutsecs : -99)
#define FDCPIOPOS(fd)	(fd ? ((FD_t)fd)->fd_cpioPos : -99)

#define	FDONLY(fd)	assert(fdGetIo(fd) == fdio)

#define	UFDONLY(fd)	/* assert(fdGetIo(fd) == ufdio) */

#define	fdGetFILE(_fd)	((FILE *)fdGetFp(_fd))

/**
 */
/*@unchecked@*/
#if _USE_LIBIO
int noLibio = 0;
#else
int noLibio = 1;
#endif

#define TIMEOUT_SECS 60

/**
 */
/*@unchecked@*/
static int ftpTimeoutSecs = TIMEOUT_SECS;

/**
 */
/*@unchecked@*/
int _rpmio_debug = 0;

/**
 */
/*@unchecked@*/
int _av_debug = 0;

/**
 */
/*@unchecked@*/
int _ftp_debug = 0;

/**
 */
/*@unchecked@*/
int _dav_debug = 0;

/* =============================================================== */

const char * fdbg(FD_t fd)
{
    static char buf[BUFSIZ];
    char *be = buf;
    int i;

    buf[0] = '\0';
    if (fd == NULL)
	return buf;

#ifdef DYING
    sprintf(be, "fd %p", fd);	be += strlen(be);
    if (fd->rd_timeoutsecs >= 0) {
	sprintf(be, " secs %d", fd->rd_timeoutsecs);
	be += strlen(be);
    }
#endif
    if (fd->bytesRemain != -1) {
	sprintf(be, " clen %d", (int)fd->bytesRemain);
	be += strlen(be);
    }
    if (fd->wr_chunked) {
	strcpy(be, " chunked");
	be += strlen(be);
    }
    *be++ = '\t';
    for (i = fd->nfps; i >= 0; i--) {
	FDSTACK_t * fps = &fd->fps[i];
	if (i != fd->nfps)
	    *be++ = ' ';
	*be++ = '|';
	*be++ = ' ';
	if (fps->io == fdio) {
	    sprintf(be, "FD %d fp %p", fps->fdno, fps->fp);
	} else if (fps->io == ufdio) {
	    sprintf(be, "UFD %d fp %p", fps->fdno, fps->fp);
#if defined(WITH_ZLIB)
	} else if (fps->io == gzdio) {
	    sprintf(be, "GZD %p fdno %d", fps->fp, fps->fdno);
#endif
#if defined(WITH_BZIP2)
	} else if (fps->io == bzdio) {
	    sprintf(be, "BZD %p fdno %d", fps->fp, fps->fdno);
#endif
#if defined(WITH_XZ)
	} else if (fps->io == lzdio) {
	    sprintf(be, "LZD %p fdno %d", fps->fp, fps->fdno);
	} else if (fps->io == xzdio) {
	    sprintf(be, "XZD %p fdno %d", fps->fp, fps->fdno);
#endif
	} else if (fps->io == fpio) {
	    /*@+voidabstract@*/
	    sprintf(be, "%s %p(%d) fdno %d",
		(fps->fdno < 0 ? "LIBIO" : "FP"),
		fps->fp, fileno(((FILE *)fps->fp)), fps->fdno);
	    /*@=voidabstract@*/
	} else {
	    sprintf(be, "??? io %p fp %p fdno %d ???",
		fps->io, fps->fp, fps->fdno);
	}
	be += strlen(be);
	*be = '\0';
    }
    return buf;
}

/* =============================================================== */
FD_t fdDup(int fdno)
{
    FD_t fd;
    int nfdno;

    if ((nfdno = dup(fdno)) < 0)
	return NULL;
    if (fcntl(nfdno, F_SETFD, FD_CLOEXEC)) {
	(void) close(nfdno);
	return NULL;
    }
    fd = fdNew("open (fdDup)");
    fdSetOpen(fd, "fdDup", nfdno, 0);	/* XXX bogus */
    fdSetFdno(fd, nfdno);
DBGIO(fd, (stderr, "<-- fdDup(%d) fd %p %s\n", fdno, (fd ? fd : NULL), fdbg(fd)));
    /*@-refcounttrans@*/ return fd; /*@=refcounttrans@*/
}

static inline /*@unused@*/
int fdSeekNot(void * cookie,
		/*@unused@*/ _libio_pos_t pos,
		/*@unused@*/ int whence)
	/*@*/
{
    FD_t fd = c2f(cookie);
    FDSANE(fd);		/* XXX keep gcc quiet */
    return -2;
}

/* =============================================================== */

static void fdFini(void * _fd)
	/*@globals fileSystem @*/
	/*@modifies _fd, fileSystem @*/
{
    FD_t fd = (FD_t) _fd;
    int i;

assert(fd != NULL);
    fd->opath = _free(fd->opath);
    if (fd->stats) free(fd->stats); fd->stats = NULL;
    if (fd->ndigests > 0)
    for (i = fd->ndigests - 1; i >= 0; i--) {
	DIGEST_CTX ctx = fd->digests[i];
	if (ctx == NULL)
	    continue;
	(void) rpmDigestFinal(ctx, NULL, NULL, 0);
	fd->digests[i] = NULL;
    }
    fd->digests = _free(fd->digests);
    fd->ndigests = 0;
    fd->contentType = _free(fd->contentType);
    fd->contentDisposition = _free(fd->contentDisposition);
/*@-onlytrans@*/
#ifdef WITH_XAR
    fd->xar = rpmxarFree(fd->xar, "fdFini");
#endif
#ifdef WITH_NEON
#ifndef	NOTYET
if (fd->req != NULL)
fprintf(stderr, "*** %s: fd->req %p\n", __FUNCTION__, fd->req);
fd->req = NULL;
#else
assert(fd->req == NULL);
#endif
#endif
    fd->dig = pgpDigFree(fd->dig);
/*@=onlytrans@*/
}

/*@unchecked@*/ /*@only@*/ /*@null@*/
rpmioPool _fdPool;

static FD_t fdGetPool(/*@null@*/ rpmioPool pool)
	/*@globals _fdPool, fileSystem @*/
	/*@modifies pool, _fdPool, fileSystem @*/
{
    FD_t fd;

    if (_fdPool == NULL) {
	_fdPool = rpmioNewPool("fd", sizeof(*fd), -1, _rpmio_debug,
		(char * (*)(void *))fdbg, NULL, fdFini);
	pool = _fdPool;
    }
    fd = (FD_t) rpmioGetPool(pool, sizeof(*fd));
    memset(((char *)fd)+sizeof(fd->_item), 0, sizeof(*fd)-sizeof(fd->_item));
    return fd;
}

/*@-incondefs@*/
/*@null@*/
FD_t XfdNew(const char * msg, const char * fn, unsigned ln)
{
    FD_t fd = fdGetPool(_fdPool);
    if (fd == NULL) /* XXX xmalloc never returns NULL */
	return NULL;
    fd->flags = 0;
    fd->magic = FDMAGIC;

    fd->nfps = 0;
    memset(fd->fps, 0, sizeof(fd->fps));

    fd->fps[0].io = ufdio;
    fd->fps[0].fp = NULL;
    fd->fps[0].fdno = -1;

    fd->u = NULL;
    fd->req = NULL;
    fd->rd_timeoutsecs = 1;	/* XXX default value used to be -1 */
    fd->bytesRemain = -1;
    fd->contentLength = -1;
    fd->persist = 0;
    fd->wr_chunked = 0;

    fd->syserrno = 0;
    fd->errcookie = NULL;

    fd->opath = NULL;
    fd->oflags = 0;
    fd->omode = 0;

    fd->xar = NULL;
    fd->dig = NULL;
    fd->stats = (FDSTAT_t) xcalloc(1, sizeof(*fd->stats));
    fd->ndigests = 0;
    fd->digests = NULL;

    fd->contentType = NULL;
    fd->contentDisposition = NULL;
    fd->lastModified = 0;
    fd->ftpFileDoneNeeded = 0;
    fd->fd_cpioPos = 0;

    return (FD_t)rpmioLinkPoolItem((rpmioItem)fd, msg, fn, ln);
}
/*@=incondefs@*/

static ssize_t fdRead(void * cookie, /*@out@*/ char * buf, size_t count)
	/*@globals errno, fileSystem, internalState @*/
	/*@modifies buf, errno, fileSystem, internalState @*/
	/*@requires maxSet(buf) >= (count - 1) @*/
{
    FD_t fd = c2f(cookie);
    ssize_t rc;

    if (fd->bytesRemain == 0) return 0;	/* XXX simulate EOF */

    fdstat_enter(fd, FDSTAT_READ);
    /* HACK: flimsy wiring for davRead */
    if (fd->req != NULL) {
#ifdef WITH_NEON
	if (fd->req != (void *)-1)
	    rc = davRead(fd, buf, (count > (size_t)fd->bytesRemain ? (size_t)fd->bytesRemain : count));
	else
	    rc = -1;
#else
	rc = -1;
#endif
	/* XXX Chunked davRead EOF. */
	if (rc == 0)
	    fd->bytesRemain = 0;
    } else
    if (fd->xar != NULL) {
#ifdef WITH_XAR
	rc = xarRead(fd, buf, (count > (size_t)fd->bytesRemain ? (size_t)fd->bytesRemain : count));
#else
	rc = -1;
#endif
    } else
	rc = read(fdFileno(fd), buf, (count > (size_t)fd->bytesRemain ? (size_t)fd->bytesRemain : count));
    fdstat_exit(fd, FDSTAT_READ, rc);

    if (fd->ndigests > 0 && rc > 0) fdUpdateDigests(fd, (const unsigned char *)buf, rc);

DBGIO(fd, (stderr, "<--\tfdRead(%p,%p,%ld) rc %ld %s\n", cookie, buf, (long)count, (long)rc, fdbg(fd)));

    return rc;
}

static ssize_t fdWrite(void * cookie, const char * buf, size_t count)
	/*@globals errno, fileSystem, internalState @*/
	/*@modifies errno, fileSystem, internalState @*/
{
    FD_t fd = c2f(cookie);
    int fdno = fdFileno(fd);
    ssize_t rc;

    if (fd->bytesRemain == 0) return 0;	/* XXX simulate EOF */

    if (fd->ndigests > 0 && count > 0) fdUpdateDigests(fd, (const unsigned char *)buf, count);

    if (count == 0) return 0;

    fdstat_enter(fd, FDSTAT_WRITE);
    /* HACK: flimsy wiring for davWrite */
    if (fd->req != NULL)
#ifdef WITH_NEON
	if (fd->req != (void *)-1)
	    rc = davWrite(fd, buf, (count > (size_t)fd->bytesRemain ? (size_t)fd->bytesRemain : count));
	else
	    rc = -1;
#else
	rc = -1;
#endif
    else
	rc = write(fdno, buf, (count > (size_t)fd->bytesRemain ? (size_t)fd->bytesRemain : count));
    fdstat_exit(fd, FDSTAT_WRITE, rc);

DBGIO(fd, (stderr, "<--\tfdWrite(%p,%p,%ld) rc %ld %s\n", cookie, buf, (long)count, (long)rc, fdbg(fd)));

    return rc;
}

static int fdSeek(void * cookie, _libio_pos_t pos, int whence)
	/*@globals fileSystem, internalState @*/
	/*@modifies fileSystem, internalState @*/
{
#ifdef USE_COOKIE_SEEK_POINTER
    _IO_off64_t p = *pos;
#else
    off_t p = pos;
#endif
    FD_t fd = c2f(cookie);
    off_t rc;

    assert(fd->bytesRemain == -1);	/* XXX FIXME fadio only for now */
    fdstat_enter(fd, FDSTAT_SEEK);
    rc = lseek(fdFileno(fd), p, whence);
    fdstat_exit(fd, FDSTAT_SEEK, rc);

DBGIO(fd, (stderr, "<--\tfdSeek(%p,%ld,%d) rc %lx %s\n", cookie, (long)p, whence, (unsigned long)rc, fdbg(fd)));

    return (int) rc;
}

static int fdClose( /*@only@*/ void * cookie)
	/*@globals errno, fileSystem, systemState, internalState @*/
	/*@modifies errno, fileSystem, systemState, internalState @*/
{
    FD_t fd;
    int fdno;
    int rc;

    if (cookie == NULL) return -2;
    fd = c2f(cookie);
    fdno = fdFileno(fd);

    fdSetFdno(fd, -1);

    fdstat_enter(fd, FDSTAT_CLOSE);
    /* HACK: flimsy wiring for davClose */
    if (fd->req != NULL)
#ifdef WITH_NEON
	rc = davClose(fd);
#else
	rc = -1;
#endif
    else
	rc = ((fdno >= 0) ? close(fdno) : -2);
    fdstat_exit(fd, FDSTAT_CLOSE, rc);

DBGIO(fd, (stderr, "<--\tfdClose(%p) rc %lx %s\n", (fd ? fd : NULL), (unsigned long)rc, fdbg(fd)));

    fd = fdFree(fd, "open (fdClose)");
    return rc;
}

static /*@null@*/ FD_t fdOpen(const char *path, int flags, mode_t mode)
	/*@globals errno, fileSystem, internalState @*/
	/*@modifies errno, fileSystem, internalState @*/
{
    FD_t fd;
    int fdno;

    fdno = open(path, flags, mode);
    if (fdno < 0) return NULL;
    if (fcntl(fdno, F_SETFD, FD_CLOEXEC)) {
	(void) close(fdno);
	return NULL;
    }
    fd = fdNew("open (fdOpen)");
    fdSetOpen(fd, path, flags, mode);
    fdSetFdno(fd, fdno);
assert(fd != NULL);
    fd->flags = flags;
DBGIO(fd, (stderr, "<--\tfdOpen(\"%s\",%x,0%o) %s\n", path, (unsigned)flags, (unsigned)mode, fdbg(fd)));
    /*@-refcounttrans@*/ return fd; /*@=refcounttrans@*/
}

#ifdef NOTUSED
FILE *fdFdopen(void * cookie, const char *fmode)
{
    FD_t fd = c2f(cookie);
    int fdno;
    FILE * fp;

    if (fmode == NULL) return NULL;
    fdno = fdFileno(fd);
    if (fdno < 0) return NULL;
    fp = fdopen(fdno, fmode);
DBGIO(fd, (stderr, "<-- fdFdopen(%p,\"%s\") fdno %d -> fp %p fdno %d\n", cookie, fmode, fdno, fp, fileno(fp)));
    fd = fdFree(fd, "open (fdFdopen)");
    return fp;
}
#endif

/*@-type@*/ /* LCL: function typedefs */
static struct FDIO_s fdio_s = {
  fdRead, fdWrite, fdSeek, fdClose, NULL, NULL, NULL,
};
/*@=type@*/

FDIO_t fdio = /*@-compmempass@*/ &fdio_s /*@=compmempass@*/ ;

int fdWritable(FD_t fd, int secs)
{
    int fdno;
    int rc;
#if defined(HAVE_POLL_H)
    int msecs = (secs >= 0 ? (1000 * secs) : -1);
    struct pollfd wrfds;
#else
    struct timeval timeout, *tvp = (secs >= 0 ? &timeout : NULL);
    fd_set wrfds;
    FD_ZERO(&wrfds);
#endif
	
    /* HACK: flimsy wiring for davWrite */
    if (fd->req != NULL)
	return (fd->req == (void *)-1 ? -1 : 1);

    if ((fdno = fdFileno(fd)) < 0)
	return -1;	/* XXX W2DO? */
	
    do {
#if defined(HAVE_POLL_H)
	wrfds.fd = fdno;
	wrfds.events = POLLOUT;
	wrfds.revents = 0;
	rc = poll(&wrfds, 1, msecs);
#else
	if (tvp) {
	    tvp->tv_sec = secs;
	    tvp->tv_usec = 0;
	}
	FD_SET(fdno, &wrfds);
/*@-compdef -nullpass@*/
	rc = select(fdno + 1, NULL, &wrfds, NULL, tvp);
/*@=compdef =nullpass@*/
#endif

	/* HACK: EBADF on PUT chunked termination from ufdClose. */
if (_rpmio_debug && !(rc == 1 && errno == 0))
fprintf(stderr, "*** fdWritable fdno %d rc %d %s\n", fdno, rc, strerror(errno));
	if (rc < 0) {
	    switch (errno) {
	    case EINTR:
		continue;
		/*@notreached@*/ /*@switchbreak@*/ break;
	    default:
		return rc;
		/*@notreached@*/ /*@switchbreak@*/ break;
	    }
	}
	return rc;
    } while (1);
    /*@notreached@*/
}

int fdReadable(FD_t fd, int secs)
{
    int fdno;
    int rc;
#if defined(HAVE_POLL_H)
    int msecs = (secs >= 0 ? (1000 * secs) : -1);
    struct pollfd rdfds;
#else
    struct timeval timeout, *tvp = (secs >= 0 ? &timeout : NULL);
    fd_set rdfds;
    FD_ZERO(&rdfds);
#endif

    /* HACK: flimsy wiring for davRead */
    if (fd->req != NULL)
	return (fd->req == (void *)-1 ? -1 : 1);

    if ((fdno = fdFileno(fd)) < 0)
	return -1;	/* XXX W2DO? */
	
    do {
#if defined(HAVE_POLL_H)
	rdfds.fd = fdno;
	rdfds.events = POLLIN;
	rdfds.revents = 0;
	rc = poll(&rdfds, 1, msecs);
#else
	if (tvp) {
	    tvp->tv_sec = secs;
	    tvp->tv_usec = 0;
	}
	FD_SET(fdno, &rdfds);
	/*@-compdef -nullpass@*/
	rc = select(fdno + 1, &rdfds, NULL, NULL, tvp);
	/*@=compdef =nullpass@*/
#endif

	if (rc < 0) {
	    switch (errno) {
	    case EINTR:
		continue;
		/*@notreached@*/ /*@switchbreak@*/ break;
	    default:
		return rc;
		/*@notreached@*/ /*@switchbreak@*/ break;
	    }
	}
	return rc;
    } while (1);
    /*@notreached@*/
}

int fdFgets(FD_t fd, char * buf, size_t len)
{
    int fdno;
    int secs = fd->rd_timeoutsecs;
    size_t nb = 0;
    int ec = 0;
    char lastchar = '\0';

    if ((fdno = fdFileno(fd)) < 0)
	return 0;	/* XXX W2DO? */
	
    do {
	int rc;

	/* Is there data to read? */
	rc = fdReadable(fd, secs);

	switch (rc) {
	case -1:	/* error */
	    ec = -1;
	    continue;
	    /*@notreached@*/ /*@switchbreak@*/ break;
	case  0:	/* timeout */
	    ec = -1;
	    continue;
	    /*@notreached@*/ /*@switchbreak@*/ break;
	default:	/* data to read */
	    /*@switchbreak@*/ break;
	}

	errno = 0;
#ifdef	NOISY
	rc = fdRead(fd, buf + nb, 1);
#else
	rc = (int)read(fdFileno(fd), buf + nb, 1);
#endif
	if (rc < 0) {
	    fd->syserrno = errno;
	    switch (errno) {
	    case EWOULDBLOCK:
		continue;
		/*@notreached@*/ /*@switchbreak@*/ break;
	    default:
		/*@switchbreak@*/ break;
	    }
if (_rpmio_debug)
fprintf(stderr, "*** read: fd %p rc %d errno %d %s \"%s\"\n", fd, rc, errno, strerror(errno), buf);
	    ec = -1;
	    break;
	} else if (rc == 0) {
if (_rpmio_debug)
fprintf(stderr, "*** read: fd %p rc %d EOF errno %d %s \"%s\"\n", fd, rc, errno, strerror(errno), buf);
	    break;
	} else {
	    nb += rc;
	    buf[nb] = '\0';
	    lastchar = buf[nb - 1];
	}
    } while (ec == 0 && nb < len && lastchar != '\n');

    return (ec >= 0 ? (int)nb : ec);
}

/* =============================================================== */
/* Support for FTP/HTTP I/O.
 */
const char * ftpStrerror(int errorNumber)
{
    switch (errorNumber) {
    case 0:
	return _("Success");

    /* HACK error impediance match, coalesce and rename. */
    case FTPERR_NE_ERROR:
	return ("NE_ERROR: Generic error.");
    case FTPERR_NE_LOOKUP:
	return ("NE_LOOKUP: Hostname lookup failed.");
    case FTPERR_NE_AUTH:
	return ("NE_AUTH: Server authentication failed.");
    case FTPERR_NE_PROXYAUTH:
	return ("NE_PROXYAUTH: Proxy authentication failed.");
    case FTPERR_NE_CONNECT:
	return ("NE_CONNECT: Could not connect to server.");
    case FTPERR_NE_TIMEOUT:
	return ("NE_TIMEOUT: Connection timed out.");
    case FTPERR_NE_FAILED:
	return ("NE_FAILED: The precondition failed.");
    case FTPERR_NE_RETRY:
	return ("NE_RETRY: Retry request.");
    case FTPERR_NE_REDIRECT:
	return ("NE_REDIRECT: Redirect received.");

    case FTPERR_BAD_SERVER_RESPONSE:
	return _("Bad server response");
    case FTPERR_SERVER_IO_ERROR:
	return _("Server I/O error");
    case FTPERR_SERVER_TIMEOUT:
	return _("Server timeout");
    case FTPERR_BAD_HOST_ADDR:
	return _("Unable to lookup server host address");
    case FTPERR_BAD_HOSTNAME:
	return _("Unable to lookup server host name");
    case FTPERR_FAILED_CONNECT:
	return _("Failed to connect to server");
    case FTPERR_FAILED_DATA_CONNECT:
	return _("Failed to establish data connection to server");
    case FTPERR_FILE_IO_ERROR:
	return _("I/O error to local file");
    case FTPERR_PASSIVE_ERROR:
	return _("Error setting remote server to passive mode");
    case FTPERR_FILE_NOT_FOUND:
	return _("File not found on server");
    case FTPERR_NIC_ABORT_IN_PROGRESS:
	return _("Abort in progress");

    case FTPERR_UNKNOWN:
    default:
	return _("Unknown or unexpected error");
    }
}

const char *urlStrerror(const char *url)
{
    const char *retstr;
    switch (urlIsURL(url)) {
    case URL_IS_HKP:
    case URL_IS_FTP:
    case URL_IS_HTTP:
    case URL_IS_HTTPS:
    {	urlinfo u;
/* XXX This only works for httpReq/ftpLogin/ftpReq failures */
	if (urlSplit(url, &u) == 0)
	    retstr = ftpStrerror(u->openError);
	else
	    retstr = _("Malformed URL");
    }	break;
    case URL_IS_MONGO:	/* XXX FIXME */
    default:
	retstr = strerror(errno);
	break;
    }
    return retstr;
}

#if !defined(HAVE_GETADDRINFO)
#if !defined(USE_ALT_DNS) || !USE_ALT_DNS
static int mygethostbyname(const char * host,
		/*@out@*/ struct in_addr * address)
	/*@globals h_errno @*/
	/*@modifies *address @*/
{
    struct hostent * hostinfo;

    /*@-multithreaded @*/
    hostinfo = gethostbyname(host);
    /*@=multithreaded @*/
    if (!hostinfo) return 1;

    memcpy(address, hostinfo->h_addr_list[0], sizeof(*address));
    return 0;
}
#endif

/*@-compdef@*/	/* FIX: address->s_addr undefined. */
static int getHostAddress(const char * host, /*@out@*/ struct in_addr * address)
	/*@globals errno, h_errno @*/
	/*@modifies *address, errno @*/
{
#if 0	/* XXX workaround nss_foo module hand-off using valgrind. */
    if (!strcmp(host, "localhost")) {
	/*@-moduncon @*/
	if (!inet_aton("127.0.0.1", address))
	    return FTPERR_BAD_HOST_ADDR;
	/*@=moduncon @*/
    } else
#endif
    if (xisdigit(host[0])) {
	/*@-moduncon @*/
	if (!inet_aton(host, address))
	    return FTPERR_BAD_HOST_ADDR;
	/*@=moduncon @*/
    } else {
	if (mygethostbyname(host, address)) {
	    errno = h_errno;
	    return FTPERR_BAD_HOSTNAME;
	}
    }

    return 0;
}
/*@=compdef@*/
#endif	/* HAVE_GETADDRINFO */

static int tcpConnect(FD_t ctrl, const char * host, int port)
	/*@globals fileSystem, internalState @*/
	/*@modifies ctrl, fileSystem, internalState @*/
{
    int fdno = -1;
    int rc;
#ifdef	HAVE_GETADDRINFO
/*@-unrecog@*/
    struct addrinfo hints, *res, *res0;
#ifndef	NI_MAXSERV
#define	NI_MAXSERV	32
#endif
    char pbuf[NI_MAXSERV];
    int xx;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    sprintf(pbuf, "%d", port);
    pbuf[sizeof(pbuf)-1] = '\0';
    rc = FTPERR_FAILED_CONNECT;
    if (getaddrinfo(host, pbuf, &hints, &res0) == 0) {
	for (res = res0; res != NULL; res = res->ai_next) {
	    if ((fdno = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) < 0)
		continue;
	    if (connect(fdno, res->ai_addr, (int)res->ai_addrlen) < 0) {
		xx = close(fdno);
		fdno = -1;	/* XXX coverity #1036724 */
		continue;
	    }
	    /* success */
	    rc = 0;
	    if (_ftp_debug) {
		char hbuf[NI_MAXHOST];
		hbuf[0] = '\0';
		xx = getnameinfo(res->ai_addr, res->ai_addrlen, hbuf, sizeof(hbuf),
				NULL, 0, NI_NUMERICHOST);
		fprintf(stderr,"++ connect [%s]:%d on fdno %d\n",
				/*@-unrecog@*/ hbuf /*@=unrecog@*/, port, fdno);
	    }
	    break;
	}
	freeaddrinfo(res0);
    }
    if (rc < 0)
	goto errxit;
/*@=unrecog@*/
#else	/* HAVE_GETADDRINFO */
    struct sockaddr_in sin;

    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(port);
    sin.sin_addr.s_addr = INADDR_ANY;

  do {
    if ((rc = getHostAddress(host, &sin.sin_addr)) < 0)
	break;

    if ((fdno = socket(sin.sin_family, SOCK_STREAM, IPPROTO_IP)) < 0) {
	rc = FTPERR_FAILED_CONNECT;
	break;
    }

    /*@-internalglobs@*/
    if (connect(fdno, (struct sockaddr *) &sin, sizeof(sin))) {
	rc = FTPERR_FAILED_CONNECT;
	break;
    }
    /*@=internalglobs@*/
  } while (0);

    if (rc < 0)
	goto errxit;

if (_ftp_debug)
fprintf(stderr,"++ connect %s:%d on fdno %d\n",
/*@-unrecog -moduncon -evalorderuncon @*/
inet_ntoa(sin.sin_addr)
/*@=unrecog =moduncon =evalorderuncon @*/ ,
(int)ntohs(sin.sin_port), fdno);
#endif	/* HAVE_GETADDRINFO */

    fdSetFdno(ctrl, (fdno >= 0 ? fdno : -1));
    return 0;

errxit:
    /*@-observertrans@*/
    fdSetSyserrno(ctrl, errno, ftpStrerror(rc));
    /*@=observertrans@*/
    if (fdno >= 0)
	(void) close(fdno);
    return rc;
}

static int checkResponse(void * _u, FD_t ctrl,
		/*@out@*/ int *ecp, /*@out@*/ char ** str)
	/*@globals fileSystem @*/
	/*@modifies ctrl, *ecp, *str, fileSystem @*/
{
    urlinfo u = (urlinfo) _u;
    char *buf;
    size_t bufAlloced;
    int bufLength = 0;
    const char *s;
    char *se;
    int ec = 0;
    int moretodo = 1;
    char errorCode[4];

    URLSANE(u);
    if (u->bufAlloced == 0 || u->buf == NULL) {
	u->bufAlloced = _url_iobuf_size;
	u->buf = (char *) xcalloc(u->bufAlloced, sizeof(u->buf[0]));
    }
    buf = u->buf;
    bufAlloced = u->bufAlloced;
    *buf = '\0';

    errorCode[0] = '\0';

    do {
	int rc;

	/*
	 * Read next line from server.
	 */
	se = buf + bufLength;
	*se = '\0';
	rc = fdFgets(ctrl, se, (bufAlloced - bufLength));
	if (rc < 0) {
	    ec = FTPERR_BAD_SERVER_RESPONSE;
	    continue;
	} else if (rc == 0 || fdWritable(ctrl, 0) < 1)
	    moretodo = 0;

	/*
	 * Process next line from server.
	 */
	for (s = se; *s != '\0'; s = se) {
		const char *e;

		while (*se && *se != '\n') se++;

		if (se > s && se[-1] == '\r')
		   se[-1] = '\0';
		if (*se == '\0')
		    /*@innerbreak@*/ break;

if (_ftp_debug)
fprintf(stderr, "<- %s\n", s);

		/* HTTP: header termination on empty line */
		if (*s == '\0') {
		    moretodo = 0;
		    /*@innerbreak@*/ break;
		}
		*se++ = '\0';

		/* HTTP: look for "HTTP/1.1 123 ..." */
		if (!strncmp(s, "HTTP", sizeof("HTTP")-1)) {
		    ctrl->contentLength = -1;
		    if ((e = strchr(s, '.')) != NULL) {
			e++;
			u->httpVersion = (int)(*e - '0');
			if (u->httpVersion < 1 || u->httpVersion > 2)
			    ctrl->persist = u->httpVersion = 0;
			else
			    ctrl->persist = 1;
		    }
		    if ((e = strchr(s, ' ')) != NULL) {
			e++;
			if (strchr("0123456789", *e))
			    strncpy(errorCode, e, 3);
			errorCode[3] = '\0';
		    }
		    /*@innercontinue@*/ continue;
		}

		/* HTTP: look for "token: ..." */
		for (e = s; *e && !(*e == ' ' || *e == ':'); e++)
		    {};
		if (e > s && *e++ == ':') {
		    size_t ne = (e - s);
		    while (*e && *e == ' ') e++;
#if 0
		    if (!strncmp(s, "Date:", ne)) {
		    } else
		    if (!strncmp(s, "Server:", ne)) {
		    } else
		    if (!strncmp(s, "Last-Modified:", ne)) {
		    } else
		    if (!strncmp(s, "ETag:", ne)) {
		    } else
#endif
		    if (!strncmp(s, "Accept-Ranges:", ne)) {
			if (!strcmp(e, "bytes"))
			    u->allow |= RPMURL_SERVER_HASRANGE;
			if (!strcmp(e, "none"))
			    u->allow &= ~RPMURL_SERVER_HASRANGE;
		    } else
		    if (!strncmp(s, "Content-Length:", ne)) {
			if (strchr("0123456789", *e))
			    ctrl->contentLength = atol(e);
		    } else
		    if (!strncmp(s, "Connection:", ne)) {
			if (!strcmp(e, "close"))
			    ctrl->persist = 0;
		    }
#if 0
		    else
		    if (!strncmp(s, "Content-Type:", ne)) {
		    } else
		    if (!strncmp(s, "Transfer-Encoding:", ne)) {
			if (!strcmp(e, "chunked"))
			    ctrl->wr_chunked = 1;
			else
			    ctrl->wr_chunked = 0;
		    } else
		    if (!strncmp(s, "Allow:", ne)) {
		    }
#endif
		    /*@innercontinue@*/ continue;
		}

		/* HTTP: look for "<TITLE>501 ... </TITLE>" */
		if (!strncmp(s, "<TITLE>", sizeof("<TITLE>")-1))
		    s += sizeof("<TITLE>") - 1;

		/* FTP: look for "123-" and/or "123 " */
		if (strchr("0123456789", *s)) {
		    if (errorCode[0] != '\0') {
			if (!strncmp(s, errorCode, sizeof("123")-1) && s[3] == ' ')
			    moretodo = 0;
		    } else {
			strncpy(errorCode, s, sizeof("123")-1);
			errorCode[3] = '\0';
			if (s[3] != '-')
			    moretodo = 0;
		    }
		}
	}

	if (moretodo && se > s) {
	    bufLength = se - s - 1;
	    if (s != buf)
		memmove(buf, s, bufLength);
	} else {
	    bufLength = 0;
	}
    } while (moretodo && ec == 0);

    if (str)	*str = buf;
    if (ecp)	*ecp = atoi(errorCode);

    return ec;
}

static int ftpCheckResponse(urlinfo u, /*@out@*/ char ** str)
	/*@globals fileSystem @*/
	/*@modifies u, *str, fileSystem @*/
{
    int ec = 0;
    int rc;

    URLSANE(u);
    rc = checkResponse(u, u->ctrl, &ec, str);

    switch (ec) {
    case 550:
	return FTPERR_FILE_NOT_FOUND;
	/*@notreached@*/ break;
    case 552:
	return FTPERR_NIC_ABORT_IN_PROGRESS;
	/*@notreached@*/ break;
    default:
	if (ec >= 400 && ec <= 599) {
	    return FTPERR_BAD_SERVER_RESPONSE;
	}
	break;
    }
    return rc;
}

static int ftpCommand(urlinfo u, char ** str, ...)
	/*@globals fileSystem, internalState @*/
	/*@modifies u, *str, fileSystem, internalState @*/
{
    va_list ap;
    int len = 0;
    const char * s, * t;
    char * te;
    int rc;

    URLSANE(u);
    va_start(ap, str);
    while ((s = va_arg(ap, const char *)) != NULL) {
	if (len) len++;
	len += strlen(s);
    }
    len += sizeof("\r\n")-1;
    va_end(ap);

    t = te = (char *) alloca(len + 1);

    va_start(ap, str);
    while ((s = va_arg(ap, const char *)) != NULL) {
	if (te > t) *te++ = ' ';
	te = stpcpy(te, s);
    }
    te = stpcpy(te, "\r\n");
    va_end(ap);

if (_ftp_debug)
fprintf(stderr, "-> %s", t);
    if (fdWrite(u->ctrl, t, (te-t)) != (te-t))
	return FTPERR_SERVER_IO_ERROR;

    rc = ftpCheckResponse(u, str);
    return rc;
}

static int ftpLogin(urlinfo u)
	/*@globals fileSystem, internalState @*/
	/*@modifies u, fileSystem, internalState @*/
{
    const char * host;
    const char * user;
    const char * password;
    int port;
    int rc;

    URLSANE(u);
    u->ctrl = fdLink(u->ctrl, "open ctrl");

    if (((host = (u->proxyh ? u->proxyh : u->host)) == NULL)) {
	rc = FTPERR_BAD_HOSTNAME;
	goto errxit;
    }

    if ((port = (u->proxyp > 0 ? u->proxyp : u->port)) < 0) port = IPPORT_FTP;

    if ((user = (u->proxyu ? u->proxyu : u->user)) == NULL)
	user = "anonymous";

    if ((password = u->password) == NULL) {
 	uid_t uid = getuid();
	struct passwd * pw;
	if (uid && (pw = getpwuid(uid)) != NULL) {
	    char *myp = (char *) alloca(strlen(pw->pw_name) + sizeof("@"));
	    strcpy(myp, pw->pw_name);
	    strcat(myp, "@");
	    password = myp;
	} else {
	    password = "root@";
	}
    }

    if (fdFileno(u->ctrl) >= 0 && fdWritable(u->ctrl, 0) < 1)
	/*@-refcounttrans@*/ (void) fdClose(u->ctrl); /*@=refcounttrans@*/

/*@-usereleased@*/
    if (fdFileno(u->ctrl) < 0) {
	rc = tcpConnect(u->ctrl, host, port);
	if (rc < 0)
	    goto errxit2;
    }

    if ((rc = ftpCheckResponse(u, NULL)))
	goto errxit;

    if ((rc = ftpCommand(u, NULL, "USER", user, NULL)))
	goto errxit;

    if ((rc = ftpCommand(u, NULL, "PASS", password, NULL)))
	goto errxit;

    if ((rc = ftpCommand(u, NULL, "TYPE", "I", NULL)))
	goto errxit;

    /*@-compdef@*/
    return 0;
    /*@=compdef@*/

errxit:
    /*@-observertrans@*/
    fdSetSyserrno(u->ctrl, errno, ftpStrerror(rc));
    /*@=observertrans@*/
errxit2:
    if (fdFileno(u->ctrl) >= 0)
	/*@-refcounttrans@*/ (void) fdClose(u->ctrl); /*@=refcounttrans@*/
    /*@-compdef@*/
    return rc;
    /*@=compdef@*/
/*@=usereleased@*/
}

int ftpReq(FD_t data, const char * ftpCmd, const char * ftpArg)
{
    urlinfo u = (urlinfo) data->u;
#if !defined(HAVE_GETADDRINFO)
    struct sockaddr_in dataAddress;
#endif	/* HAVE_GETADDRINFO */
    char remoteIP[NI_MAXHOST];
    char * cmd;
    size_t cmdlen;
    char * passReply;
    char * chptr;
    int rc;
    int epsv;
    int port;

    remoteIP[0] = '\0';
    URLSANE(u);
    if (ftpCmd == NULL)
	return FTPERR_UNKNOWN;	/* XXX W2DO? */

    cmdlen = strlen(ftpCmd) + (ftpArg ? 1+strlen(ftpArg) : 0) + sizeof("\r\n");
    cmd = (char *) alloca(cmdlen);	/* XXX coverity #1035968 */
    chptr = stpcpy(cmd, ftpCmd);
    if (ftpArg) {
	*chptr++ = ' ';
	chptr = stpcpy(chptr, ftpArg);
    }
    chptr = stpcpy(chptr, "\r\n");
    cmdlen = chptr - cmd;

/*
 * Get the ftp version of the Content-Length.
 */
    if (!strncmp(cmd, "RETR", 4)) {
	unsigned cl;

	passReply = NULL;
	rc = ftpCommand(u, &passReply, "SIZE", ftpArg, NULL);
	if (rc)
	    goto errxit;
	if (sscanf(passReply, "%d %u", &rc, &cl) != 2) {
	    rc = FTPERR_BAD_SERVER_RESPONSE;
	    goto errxit;
	}
	rc = 0;
	data->contentLength = cl;
    }

    epsv = 0;
    passReply = NULL;
#ifdef HAVE_GETNAMEINFO
    rc = ftpCommand(u, &passReply, "EPSV", NULL);
    if (rc == 0) {
#ifdef HAVE_GETADDRINFO
	struct sockaddr_storage ss;
#else /* HAVE_GETADDRINFO */
	struct sockaddr_in ss;
#endif /* HAVE_GETADDRINFO */
	socklen_t sslen = sizeof(ss);

	/* we need to know IP of remote host */
	if ((getpeername(fdFileno(c2f(u->ctrl)), (struct sockaddr *)&ss, &sslen) == 0)
	 && (getnameinfo((struct sockaddr *)&ss, sslen,
			remoteIP, sizeof(remoteIP),
			NULL, 0, NI_NUMERICHOST) == 0))
	{
		epsv++;
	} else {
		/* abort EPSV and fall back to PASV */
		rc = ftpCommand(u, &passReply, "ABOR", NULL);
		if (rc) {
		    rc = FTPERR_PASSIVE_ERROR;
		    goto errxit;
		}
	}
    }
   if (epsv == 0)
#endif /* HAVE_GETNAMEINFO */
	rc = ftpCommand(u, &passReply, "PASV", NULL);
    if (rc) {
	rc = FTPERR_PASSIVE_ERROR;
	goto errxit;
    }

    chptr = passReply;
assert(chptr != NULL);
    while (*chptr && *chptr != '(') chptr++;
    if (*chptr != '(') return FTPERR_PASSIVE_ERROR;
    chptr++;
    passReply = chptr;
    while (*chptr && *chptr != ')') chptr++;
    if (*chptr != ')') return FTPERR_PASSIVE_ERROR;
    *chptr-- = '\0';

  if (epsv) {
	int i;
	if(sscanf(passReply,"%*c%*c%*c%d%*c",&i) != 1) {
	   rc = FTPERR_PASSIVE_ERROR;
	   goto errxit;
	}
	port = i;
  } else {

    while (*chptr && *chptr != ',') chptr--;
    if (*chptr != ',') return FTPERR_PASSIVE_ERROR;
    chptr--;
    while (*chptr && *chptr != ',') chptr--;
    if (*chptr != ',') return FTPERR_PASSIVE_ERROR;
    *chptr++ = '\0';

    /* now passReply points to the IP portion, and chptr points to the
       port number portion */

    {	int i, j;
	if (sscanf(chptr, "%d,%d", &i, &j) != 2) {
	    rc = FTPERR_PASSIVE_ERROR;
	    goto errxit;
	}
	port = (((unsigned)i) << 8) + j;
    }

    chptr = passReply;
    while (*chptr++ != '\0') {
	if (*chptr == ',') *chptr = '.';
    }
    sprintf(remoteIP, "%s", passReply);
  } /* if (epsv) */

#ifdef HAVE_GETADDRINFO
/*@-unrecog@*/
    {
	struct addrinfo hints, *res, *res0;
	char pbuf[NI_MAXSERV];
	int xx;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_NUMERICHOST;
#if defined(AI_IDN)
	hints.ai_flags |= AI_IDN;
#endif
	sprintf(pbuf, "%d", port);
	pbuf[sizeof(pbuf)-1] = '\0';
	if (getaddrinfo(remoteIP, pbuf, &hints, &res0)) {
	    rc = FTPERR_PASSIVE_ERROR;
	    goto errxit;
	}

	for (res = res0; res != NULL; res = res->ai_next) {
	    rc = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	    fdSetFdno(data, (rc >= 0 ? rc : -1));
	    if (rc < 0) {
	        if (res->ai_next)
		    continue;
		else {
		    rc = FTPERR_FAILED_CONNECT;
		    freeaddrinfo(res0);
		    goto errxit;
		}
	    }
	    data = fdLink(data, "open data (ftpReq)");

	    /* XXX setsockopt SO_LINGER */
	    /* XXX setsockopt SO_KEEPALIVE */
	    /* XXX setsockopt SO_TOS IPTOS_THROUGHPUT */

	    {
		int criterr = 0;
	        while (connect(fdFileno(data), res->ai_addr, (int)res->ai_addrlen) < 0) {
	            if (errno == EINTR)
		        /*@innercontinue@*/ continue;
		    criterr++;
		}
		if (criterr) {
		    if (res->ai_addr) {
/*@-refcounttrans@*/
		        xx = fdClose(data);
/*@=refcounttrans@*/
		        continue;
		    } else {
		        rc = FTPERR_PASSIVE_ERROR;
		        freeaddrinfo(res0);
		        goto errxit;
		    }
		}
	    }
	    /* success */
	    rc = 0;
	    break;
	}
	freeaddrinfo(res0);
    }
/*@=unrecog@*/
#else /* HAVE_GETADDRINFO */
    memset(&dataAddress, 0, sizeof(dataAddress));
    dataAddress.sin_family = AF_INET;
    dataAddress.sin_port = htons(port);

    /*@-moduncon@*/
    if (!inet_aton(remoteIP, &dataAddress.sin_addr)) {
	rc = FTPERR_PASSIVE_ERROR;
	goto errxit;
    }
    /*@=moduncon@*/

    rc = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    fdSetFdno(data, (rc >= 0 ? rc : -1));
    if (rc < 0) {
	rc = FTPERR_FAILED_CONNECT;
	goto errxit;
    }
    data = fdLink(data, "open data (ftpReq)");

    /* XXX setsockopt SO_LINGER */
    /* XXX setsockopt SO_KEEPALIVE */
    /* XXX setsockopt SO_TOS IPTOS_THROUGHPUT */

    /*@-internalglobs@*/
    while (connect(fdFileno(data), (struct sockaddr *) &dataAddress,
	        sizeof(dataAddress)) < 0)
    {
	if (errno == EINTR)
	    continue;
	rc = FTPERR_FAILED_DATA_CONNECT;
	goto errxit;
    }
    /*@=internalglobs@*/
#endif /* HAVE_GETADDRINFO */

if (_ftp_debug)
fprintf(stderr, "-> %s", cmd);
    if ((size_t)fdWrite(u->ctrl, cmd, cmdlen) != cmdlen) {
	rc = FTPERR_SERVER_IO_ERROR;
	goto errxit;
    }

    if ((rc = ftpCheckResponse(u, NULL))) {
	goto errxit;
    }

    data->ftpFileDoneNeeded = 1;
    u->ctrl = fdLink(u->ctrl, "grab data (ftpReq)");
    u->ctrl = fdLink(u->ctrl, "open data (ftpReq)");
    return 0;

errxit:
    /*@-observertrans@*/
    fdSetSyserrno(u->ctrl, errno, ftpStrerror(rc));
    /*@=observertrans@*/
    if (fdFileno(data) >= 0)
	/*@-refcounttrans@*/ (void) fdClose(data); /*@=refcounttrans@*/
    return rc;
}

#ifdef	DYING
/*@unchecked@*/ /*@null@*/
static rpmCallbackFunction	_urlNotify = NULL;

/*@unchecked@*/ /*@null@*/
static void *			_urlNotifyData = NULL;

/*@unchecked@*/
static int			_urlNotifyCount = -1;

static void urlSetCallback(rpmCallbackFunction notify, void *notifyData, int notifyCount) {
    _urlNotify = notify;
    _urlNotifyData = notifyData;
    _urlNotifyCount = (notifyCount >= 0) ? notifyCount : 4096;
}
#endif

int ufdCopy(FD_t sfd, FD_t tfd)
{
    char buf[BUFSIZ];
    int itemsRead;
    int itemsCopied = 0;
    int rc = 0;
#ifdef	DYING
    int notifier = -1;

    if (_urlNotify) {
	/*@-noeffectuncon @*/ /* FIX: check rc */
	(void)(*_urlNotify) (NULL, RPMCALLBACK_INST_OPEN_FILE,
		0, 0, NULL, _urlNotifyData);
	/*@=noeffectuncon @*/
    }
#endif

    while (1) {
	rc = (int) Fread(buf, sizeof(buf[0]), sizeof(buf), sfd);
	if (rc < 0)	/* XXX never happens Fread returns size_t */
	    break;
	else if (rc == 0) {
	    rc = itemsCopied;
	    break;
	}
	itemsRead = rc;
	rc = (int) Fwrite(buf, sizeof(buf[0]), itemsRead, tfd);
	if (rc < 0)	/* XXX never happens Fwrite returns size_t */
	    break;
 	if (rc != itemsRead) {
	    rc = FTPERR_FILE_IO_ERROR;
	    break;
	}

	itemsCopied += itemsRead;
#ifdef	DYING
	if (_urlNotify && _urlNotifyCount > 0) {
	    int n = itemsCopied/_urlNotifyCount;
	    if (n != notifier) {
		/*@-noeffectuncon @*/ /* FIX: check rc */
		(void)(*_urlNotify) (NULL, RPMCALLBACK_INST_PROGRESS,
			itemsCopied, 0, NULL, _urlNotifyData);
		/*@=noeffectuncon @*/
		notifier = n;
	    }
	}
#endif
    }

    DBGIO(sfd, (stderr, "++ copied %d bytes: %s\n", itemsCopied,
	ftpStrerror(rc)));

#ifdef	DYING
    if (_urlNotify) {
	/*@-noeffectuncon @*/ /* FIX: check rc */
	(void)(*_urlNotify) (NULL, RPMCALLBACK_INST_OPEN_FILE,
		itemsCopied, itemsCopied, NULL, _urlNotifyData);
	/*@=noeffectuncon @*/
    }
#endif

    return rc;
}

static int urlConnect(const char * url, /*@out@*/ urlinfo * uret)
	/*@globals h_errno, fileSystem, internalState @*/
	/*@modifies *uret, fileSystem, internalState @*/
{
    urlinfo u;
    int rc = 0;

    if (urlSplit(url, &u) < 0)
	return -1;

    if (urlType(u) == URL_IS_FTP) {
	FD_t fd;

	if ((fd = u->ctrl) == NULL) {
	    fd = u->ctrl = fdNew("persist ctrl (urlConnect FTP)");
/*@-usereleased@*/
	    fdSetOpen(u->ctrl, url, 0, 0);
	    fdSetIo(u->ctrl, ufdio);
/*@=usereleased@*/
	}
	
assert(fd != NULL);
	fd->rd_timeoutsecs = ftpTimeoutSecs;
	fd->contentLength = fd->bytesRemain = -1;
	fd->u = NULL;		/* XXX FTP ctrl has not */
	fd->ftpFileDoneNeeded = 0;
	fd = fdLink(fd, "grab ctrl (urlConnect FTP)");

	if (fdFileno(u->ctrl) < 0) {
	    rpmlog(RPMLOG_DEBUG, D_("logging into %s as %s, pw %s\n"),
			u->host ? u->host : "???",
			u->user ? u->user : "ftp",
			u->password ? u->password : "(username)");

	    if ((rc = ftpLogin(u)) < 0) {	/* XXX save ftpLogin error */
		u->ctrl = fdFree(fd, "grab ctrl (urlConnect FTP)");
		u->openError = rc;
	    }
	}
    }

    if (uret != NULL)
	*uret = urlLink(u, "urlConnect");
    u = urlFree(u, "urlSplit (urlConnect)");	

    return rc;
}

int ufdGetFile(FD_t sfd, FD_t tfd)
{
    int rc;

    FDSANE(sfd);
    FDSANE(tfd);
    rc = ufdCopy(sfd, tfd);
    (void) Fclose(sfd);
    if (rc > 0)		/* XXX ufdCopy now returns no. bytes copied */
	rc = 0;
    return rc;
}

int ftpCmd(const char * cmd, const char * url, const char * arg2)
{
    urlinfo u;
    int rc;
    const char * path;

    if (urlConnect(url, &u) < 0)
	return -1;

    (void) urlPath(url, &path);

    rc = ftpCommand(u, NULL, cmd, path, arg2, NULL);
    u->ctrl = fdFree(u->ctrl, "grab ctrl (ftpCmd)");
    return rc;
}

/* XXX these aren't worth the pain of including correctly */
#if !defined(IAC)
#define	IAC	((unsigned char)255)	/* interpret as command: */
#endif
#if !defined(IP)
#define	IP	((unsigned char)244)	/* interrupt process--permanently */
#endif
#if !defined(DM)
#define	DM	((unsigned char)242)	/* data mark--for connect. cleaning */
#endif
#if !defined(SHUT_RDWR)
#define	SHUT_RDWR	1+1
#endif

static int ftpAbort(urlinfo u, FD_t data)
	/*@globals fileSystem, internalState @*/
	/*@modifies u, data, fileSystem, internalState @*/
{
    static unsigned char ipbuf[3] = { IAC, IP, IAC };
    FD_t ctrl;
    int rc;
    int tosecs;

    URLSANE(u);

    if (data != NULL) {
	data->ftpFileDoneNeeded = 0;
	if (fdFileno(data) >= 0)
	    u->ctrl = fdFree(u->ctrl, "open data (ftpAbort)");
	u->ctrl = fdFree(u->ctrl, "grab data (ftpAbort)");
    }
    ctrl = u->ctrl;

    DBGIO(0, (stderr, "-> ABOR\n"));

/*@-usereleased -compdef@*/
    if (send(fdFileno(ctrl), ipbuf, sizeof(ipbuf), MSG_OOB) != sizeof(ipbuf)) {
	/*@-refcounttrans@*/ (void) fdClose(ctrl); /*@=refcounttrans@*/
	return FTPERR_SERVER_IO_ERROR;
    }

    sprintf(u->buf, "%cABOR\r\n",(char) DM);
    if (fdWrite(ctrl, u->buf, 7) != 7) {
	/*@-refcounttrans@*/ (void) fdClose(ctrl); /*@=refcounttrans@*/
	return FTPERR_SERVER_IO_ERROR;
    }

    if (data && fdFileno(data) >= 0) {
	/* XXX shorten data drain time wait */
	tosecs = data->rd_timeoutsecs;
	data->rd_timeoutsecs = 10;
	if (fdReadable(data, data->rd_timeoutsecs) > 0) {
/*@-infloopsuncon@*/
	    while ((ufdio->read)(data, u->buf, u->bufAlloced) > 0)
		u->buf[0] = '\0';
/*@=infloopsuncon@*/
	}
	data->rd_timeoutsecs = tosecs;
	/* XXX ftp abort needs to close the data channel to receive status */
	(void) shutdown(fdFileno(data), SHUT_RDWR);
	(void) close(fdFileno(data));
	data->fps[0].fdno = -1;	/* XXX WRONG but expedient */
    }

    /* XXX shorten ctrl drain time wait */
assert(u->ctrl != NULL);
    tosecs = u->ctrl->rd_timeoutsecs;
    u->ctrl->rd_timeoutsecs = 10;
    if ((rc = ftpCheckResponse(u, NULL)) == FTPERR_NIC_ABORT_IN_PROGRESS) {
	rc = ftpCheckResponse(u, NULL);
    }
    rc = ftpCheckResponse(u, NULL);
    u->ctrl->rd_timeoutsecs = tosecs;

    return rc;
/*@=usereleased =compdef@*/
}

static int ftpFileDone(urlinfo u, FD_t data)
	/*@globals fileSystem @*/
	/*@modifies u, data, fileSystem @*/
{
    int rc = 0;

    URLSANE(u);
    assert(data->ftpFileDoneNeeded);

    if (data->ftpFileDoneNeeded) {
	data->ftpFileDoneNeeded = 0;
	u->ctrl = fdFree(u->ctrl, "open data (ftpFileDone)");
	u->ctrl = fdFree(u->ctrl, "grab data (ftpFileDone)");
	rc = ftpCheckResponse(u, NULL);
    }
    return rc;
}

#ifndef WITH_NEON
static int httpResp(urlinfo u, FD_t ctrl, /*@out@*/ char ** str)
	/*@globals fileSystem @*/
	/*@modifies ctrl, *str, fileSystem @*/
{
    int ec = 0;
    int rc;

    URLSANE(u);
    rc = checkResponse(u, ctrl, &ec, str);

if (_ftp_debug && !(rc == 0 && (ec == 200 || ec == 201)))
fprintf(stderr, "*** httpResp: rc %d ec %d\n", rc, ec);

    switch (ec) {
    case 200:
    case 201:			/* 201 Created. */
	break;
    case 204:			/* HACK: if overwriting, 204 No Content. */
    case 403:			/* 403 Forbidden. */
	ctrl->syserrno = EACCES;	/* HACK */
	rc = FTPERR_UNKNOWN;
	break;
    default:
	rc = FTPERR_FILE_NOT_FOUND;
	break;
    }
    return rc;
}

static int httpReq(FD_t ctrl, const char * httpCmd, const char * httpArg)
	/*@globals h_errno, fileSystem, internalState @*/
	/*@modifies ctrl, fileSystem, internalState @*/
{
    urlinfo u;
    const char * host;
    const char * path;
    char hthost[NI_MAXHOST];
    int port;
    int rc;
    char * req;
    size_t len;
    int retrying = 0;

assert(ctrl != NULL);
    u = ctrl->u;
    URLSANE(u);

    if (((host = (u->proxyh ? u->proxyh : u->host)) == NULL))
	return FTPERR_BAD_HOSTNAME;
    if (strchr(host, ':'))
	sprintf(hthost, "[%s]", host);
    else
	strcpy(hthost, host);

    if ((port = (u->proxyp > 0 ? u->proxyp : u->port)) < 0) port = 80;
    path = (u->proxyh || u->proxyp > 0) ? u->url : httpArg;
    if (path == NULL) path = "";

reopen:
    if (fdFileno(ctrl) >= 0 && (rc = fdWritable(ctrl, 0)) < 1) {
	/*@-refcounttrans@*/ (void) fdClose(ctrl); /*@=refcounttrans@*/
    }

/*@-usereleased@*/
    if (fdFileno(ctrl) < 0) {
	rc = tcpConnect(ctrl, host, port);
	if (rc < 0)
	    goto errxit2;
	ctrl = fdLink(ctrl, "open ctrl (httpReq)");
    }

    len = sizeof("\
req x HTTP/1.0\r\n\
User-Agent: rpm/3.0.4\r\n\
Host: y:z\r\n\
Accept: text/plain\r\n\
Transfer-Encoding: chunked\r\n\
\r\n\
") + strlen(httpCmd) + strlen(path) + sizeof(VERSION) + strlen(hthost) + 20;

    req = alloca(len);
    *req = '\0';

  if (!strcmp(httpCmd, "PUT")) {
    sprintf(req, "\
%s %s HTTP/1.%d\r\n\
User-Agent: rpm/%s\r\n\
Host: %s:%d\r\n\
Accept: text/plain\r\n\
Transfer-Encoding: chunked\r\n\
\r\n\
",	httpCmd, path, (u->httpVersion ? 1 : 0), VERSION, hthost, port);
} else {
    sprintf(req, "\
%s %s HTTP/1.%d\r\n\
User-Agent: rpm/%s\r\n\
Host: %s:%d\r\n\
Accept: text/plain\r\n\
\r\n\
",	httpCmd, path, (u->httpVersion ? 1 : 0), VERSION, hthost, port);
}

if (_ftp_debug)
fprintf(stderr, "-> %s", req);

    len = strlen(req);
    if (fdWrite(ctrl, req, len) != (ssize_t)len) {
	rc = FTPERR_SERVER_IO_ERROR;
	goto errxit;
    }

    if (!strcmp(httpCmd, "PUT")) {
	ctrl->wr_chunked = 1;
    } else {

	rc = httpResp(u, ctrl, NULL);

	if (rc) {
	    if (!retrying) {	/* not HTTP_OK */
		retrying = 1;
		/*@-refcounttrans@*/ (void) fdClose(ctrl); /*@=refcounttrans@*/
		goto reopen;
	    }
	    goto errxit;
	}
    }

    ctrl = fdLink(ctrl, "open data (httpReq)");
    return 0;

errxit:
    /*@-observertrans@*/
    fdSetSyserrno(ctrl, errno, ftpStrerror(rc));
    /*@=observertrans@*/
errxit2:
    if (fdFileno(ctrl) >= 0)
	/*@-refcounttrans@*/ (void) fdClose(ctrl); /*@=refcounttrans@*/
    return rc;
/*@=usereleased@*/
}
#endif /* WITH_NEON */

/* XXX DYING: unused */
void * ufdGetUrlinfo(FD_t fd)
{
    FDSANE(fd);
    if (fd->u == NULL)
	return NULL;
/*@-retexpose@*/
    return urlLink(fd->u, "ufdGetUrlinfo");
/*@=retexpose@*/
}

/* =============================================================== */
static ssize_t ufdRead(void * cookie, /*@out@*/ char * buf, size_t count)
	/*@globals fileSystem, internalState @*/
	/*@modifies buf, fileSystem, internalState @*/
	/*@requires maxSet(buf) >= (count - 1) @*/
{
    FD_t fd = c2f(cookie);
    size_t bytesRead;
    size_t total;

    if (fdGetIo(fd) == fdio) {
	struct stat sb;
	int fdno = fdFileno(fd);
	(void) fstat(fdno, &sb);
	if (S_ISREG(sb.st_mode))
	    return fdRead(fd, buf, count);
    }

    UFDONLY(fd);
    assert(fd->rd_timeoutsecs >= 0);

    for (total = 0; total < count; total += bytesRead) {

	int rc;

	bytesRead = 0;

	/* Is there data to read? */
	if (fd->bytesRemain == 0) return (ssize_t) total; /* XXX simulate EOF */
	rc = fdReadable(fd, fd->rd_timeoutsecs);

	switch (rc) {
	case -1:	/* error */
	case  0:	/* timeout */
	    return (ssize_t) total;
	    /*@notreached@*/ /*@switchbreak@*/ break;
	default:	/* data to read */
	    /*@switchbreak@*/ break;
	}

	rc = (int) fdRead(fd, buf + total, count - total);

	if (rc < 0) {
	    switch (errno) {
	    case EWOULDBLOCK:
		continue;
		/*@notreached@*/ /*@switchbreak@*/ break;
	    default:
		/*@switchbreak@*/ break;
	    }
if (_rpmio_debug)
fprintf(stderr, "*** read: rc %d errno %d %s \"%s\"\n", rc, errno, strerror(errno), buf);
	    return rc;
	    /*@notreached@*/ break;
	} else if (rc == 0) {
	    return (ssize_t) total;
	    /*@notreached@*/ break;
	}
	bytesRead = (size_t) rc;
    }

    return (ssize_t) count;
}

static ssize_t ufdWrite(void * cookie, const char * buf, size_t count)
	/*@globals fileSystem, internalState @*/
	/*@modifies fileSystem, internalState @*/
{
    FD_t fd = c2f(cookie);
    size_t bytesWritten;
    size_t total = 0;

#ifdef	NOTYET
    if (fdGetIo(fd) == fdio) {
	struct stat sb;
	(void) fstat(fdGetFdno(fd), &sb);
	if (S_ISREG(sb.st_mode))
	    return fdWrite(fd, buf, count);
    }
#endif

    UFDONLY(fd);

    for (total = 0; total < count; total += bytesWritten) {

	int rc;

	bytesWritten = 0;

	/* Is there room to write data? */
	if (fd->bytesRemain == 0) {
fprintf(stderr, "*** ufdWrite fd %p WRITE PAST END OF CONTENT\n", fd);
	    return (ssize_t) total;	/* XXX simulate EOF */
	}
	rc = fdWritable(fd, 2);		/* XXX configurable? */

	switch (rc) {
	case -1:	/* error */
	case  0:	/* timeout */
	    return (ssize_t) total;
	    /*@notreached@*/ /*@switchbreak@*/ break;
	default:	/* data to write */
	    /*@switchbreak@*/ break;
	}

	rc = (int) fdWrite(fd, buf + total, count - total);

	if (rc < 0) {
	    switch (errno) {
	    case EWOULDBLOCK:
		continue;
		/*@notreached@*/ /*@switchbreak@*/ break;
	    default:
		/*@switchbreak@*/ break;
	    }
if (_rpmio_debug)
fprintf(stderr, "*** write: rc %d errno %d %s \"%s\"\n", rc, errno, strerror(errno), buf);
	    return rc;
	    /*@notreached@*/ break;
	} else if (rc == 0) {
	    return (ssize_t) total;
	    /*@notreached@*/ break;
	}
	bytesWritten = (size_t) rc;
    }

    return (ssize_t) count;
}

static int ufdSeek(void * cookie, _libio_pos_t pos, int whence)
	/*@globals fileSystem, internalState @*/
	/*@modifies fileSystem, internalState @*/
{
    FD_t fd = c2f(cookie);

    switch (urlType(fd->u)) {
    case URL_IS_UNKNOWN:
    case URL_IS_PATH:
	break;
    case URL_IS_DASH:
    case URL_IS_HKP:
    case URL_IS_FTP:
    case URL_IS_HTTP:
    case URL_IS_HTTPS:
    case URL_IS_MONGO:	/* XXX FIXME */
    default:
	return -2;
	/*@notreached@*/ break;
    }
    return fdSeek(cookie, pos, whence);
}

/*@-usereleased@*/	/* LCL: fd handling is tricky here. */
int ufdClose( /*@only@*/ void * cookie)
{
    FD_t fd = c2f(cookie);

    UFDONLY(fd);

    if (fd->u) {
	urlinfo u = (urlinfo) fd->u;

/*@-evalorder @*/
	if (fd == u->data)
	    fd = u->data = fdFree(fd, "grab data (ufdClose persist)");
	else
	    fd = fdFree(fd, "grab data (ufdClose)");
assert(fd != NULL);
	(void) urlFree(fd->u, "url (ufdClose)");
	fd->u = NULL;
	u->ctrl = fdFree(u->ctrl, "grab ctrl (ufdClose)");
/*@=evalorder @*/

	if (urlType(u) == URL_IS_FTP) {

	    /* XXX if not using libio, lose the fp from fpio */
	    {   FILE * fp;
		/*@+voidabstract -nullpass@*/
		fp = fdGetFILE(fd);
		if (noLibio && fp)
		    fdSetFp(fd, NULL);
		/*@=voidabstract =nullpass@*/
	    }

	    /*
	     * Non-error FTP has 4 refs on the data fd:
	     *	"persist data (ufdOpen FTP)"		rpmio.c:888
	     *	"grab data (ufdOpen FTP)"		rpmio.c:892
	     *	"open data (ftpReq)"			ftp.c:633
	     *	"fopencookie"				rpmio.c:1507
	     *
	     * Non-error FTP has 5 refs on the ctrl fd:
	     *	"persist ctrl"				url.c:176
	     *	"grab ctrl (urlConnect FTP)"		rpmio.c:404
	     *	"open ctrl"				ftp.c:504
	     *	"grab data (ftpReq)"			ftp.c:661
	     *	"open data (ftpReq)"			ftp.c:662
	     */
	    if (fd->bytesRemain > 0) {
		if (fd->ftpFileDoneNeeded) {
		    if (fdReadable(u->ctrl, 0) > 0)
			(void) ftpFileDone(u, fd);
		    else
			(void) ftpAbort(u, fd);
		}
	    } else {
		int rc;
		/* XXX STOR et al require close before ftpFileDone */
		/*@-refcounttrans@*/
		rc = fdClose(fd);
		/*@=refcounttrans@*/
#if 0	/* XXX error exit from ufdOpen does not have this set */
		assert(fd->ftpFileDoneNeeded != 0);
#endif
		/*@-compdef@*/ /* FIX: u->data undefined */
		if (fd->ftpFileDoneNeeded)
		    (void) ftpFileDone(u, fd);
		/*@=compdef@*/
		return rc;
	    }
	}

	/* XXX Why not (u->urltype == URL_IS_HTTP) ??? */
	/* XXX Why not (u->urltype == URL_IS_HTTPS) ??? */
	/* XXX Why not (u->urltype == URL_IS_HKP) ??? */
	if (u->scheme != NULL
	 && (!strncmp(u->scheme, "http", sizeof("http")-1) || !strncmp(u->scheme, "hkp", sizeof("hkp")-1)))
	{
	    /*
	     * HTTP has 4 (or 5 if persistent malloc) refs on the fd:
	     *	"persist ctrl"				url.c:177
	     *	"grab ctrl (ufdOpen HTTP)"		rpmio.c:924
	     *	"grab data (ufdOpen HTTP)"		rpmio.c:928
	     *	"open ctrl (httpReq)"			ftp.c:382
	     *	"open data (httpReq)"			ftp.c:435
	     */

/*@-evalorder @*/
	    if (fd == u->ctrl)
		fd = u->ctrl = fdFree(fd, "open data (ufdClose HTTP persist ctrl)");
	    else if (fd == u->data)
		fd = u->data = fdFree(fd, "open data (ufdClose HTTP persist data)");
	    else
		fd = fdFree(fd, "open data (ufdClose HTTP)");
/*@=evalorder @*/

	    /* XXX if not using libio, lose the fp from fpio */
	    {   FILE * fp;
		/*@+voidabstract -nullpass@*/
		fp = fdGetFILE(fd);
		if (noLibio && fp)
		    fdSetFp(fd, NULL);
		/*@=voidabstract =nullpass@*/
	    }

	    /* If content remains, then don't persist. */
assert(fd != NULL);
	    if (fd->bytesRemain > 0) {
		fd->persist = 0;
	    }
	    fd->contentLength = fd->bytesRemain = -1;

	    /* If persisting, then Fclose will juggle refcounts. */
	    if (fd->persist && (fd == u->ctrl || fd == u->data))
		return 0;
	}
    }
    return fdClose(fd);
}
/*@=usereleased@*/

/*@-nullstate@*/	/* FIX: u->{ctrl,data}->u undef after XurlLink. */
/*@null@*/ FD_t ftpOpen(const char *url, /*@unused@*/ int flags,
		/*@unused@*/ mode_t mode, /*@out@*/ urlinfo *uret)
	/*@modifies *uret @*/
{
    urlinfo u = NULL;
    FD_t fd = NULL;

#if 0	/* XXX makeTempFile() heartburn */
    assert(!(flags & O_RDWR));
#endif
    if (urlConnect(url, &u) < 0)
	goto exit;

    if (u->data == NULL)
	u->data = fdNew("persist data (ftpOpen)");

assert(u->data != NULL);
/*@-unqualifiedtrans@*/
    if (u->data->u == NULL)
	fd = u->data = fdLink(u->data, "grab data (ftpOpen persist data)");
    else
	fd = fdNew("grab data (ftpOpen)");
/*@=unqualifiedtrans@*/

    if (fd != NULL) {
	fdSetOpen(fd, url, flags, mode);
	fdSetIo(fd, ufdio);
	fd->ftpFileDoneNeeded = 0;
	fd->rd_timeoutsecs = ftpTimeoutSecs;
	fd->contentLength = fd->bytesRemain = -1;
/*@-usereleased@*/
	fd->u = urlLink(u, "url (ufdOpen FTP)");
/*@=usereleased@*/
    }

exit:
    if (uret)
	*uret = u;
    /*@-refcounttrans@*/
    return fd;
    /*@=refcounttrans@*/
}
/*@=nullstate@*/

static /*@null@*/ FD_t ufdOpen(const char * url, int flags, mode_t mode)
	/*@globals h_errno, fileSystem, internalState @*/
	/*@modifies fileSystem, internalState @*/
{
    FD_t fd = NULL;
    const char * cmd;
    urlinfo u;
    const char * path;
    urltype ut = urlPath(url, &path);

if (_rpmio_debug)
fprintf(stderr, "*** ufdOpen(%s,0x%x,0%o)\n", url, (unsigned)flags, (unsigned)mode);

/*@-usereleased@*/
    switch (ut) {
    case URL_IS_FTP:
	fd = ftpOpen(url, flags, mode, &u);
	if (fd == NULL || u == NULL)
	    break;

	/* XXX W2DO? use STOU rather than STOR to prevent clobbering */
	cmd = ((flags & O_WRONLY)
		?  ((flags & O_APPEND) ? "APPE" :
		   ((flags & O_CREAT) ? "STOR" : "STOR"))
		:  ((flags & O_CREAT) ? "STOR" : "RETR"));
	u->openError = ftpReq(fd, cmd, path);
	if (u->openError < 0) {
	    /* XXX make sure that we can exit through ufdClose */
	    fd = fdLink(fd, "error data (ufdOpen FTP)");
	} else {
	    fd->bytesRemain = ((!strcmp(cmd, "RETR"))
		?  fd->contentLength : -1);
	    fd->wr_chunked = 0;
	}
	break;
    case URL_IS_HKP:
    case URL_IS_HTTP:
    case URL_IS_HTTPS:
#ifdef WITH_NEON
	fd = davOpen(url, flags, mode, &u);
#else
	fd = httpOpen(url, flags, mode, &u);
#endif
	if (fd == NULL || u == NULL)
	    break;

	cmd = ((flags & O_WRONLY)
		?  ((flags & O_APPEND) ? "PUT" :
		   ((flags & O_CREAT) ? "PUT" : "PUT"))
		: "GET");
#ifdef WITH_NEON
	u->openError = davReq(fd, cmd, path);
#else
	u->openError = httpReq(fd, cmd, path);
#endif
	if (u->openError < 0) {
	    /* XXX make sure that we can exit through ufdClose */
	    fd = fdLink(fd, "error ctrl (ufdOpen HTTP)");
	    fd = fdLink(fd, "error data (ufdOpen HTTP)");
	} else {
	    fd->bytesRemain = ((!strcmp(cmd, "GET"))
		?  fd->contentLength : -1);
	    fd->wr_chunked = ((!strcmp(cmd, "PUT"))
		?  fd->wr_chunked : 0);
	}
	break;
    case URL_IS_MONGO:	/* XXX FIXME */
	break;
    case URL_IS_DASH:
	assert(!(flags & O_RDWR));
	fd = fdDup( ((flags & O_WRONLY) ? STDOUT_FILENO : STDIN_FILENO) );
	if (fd) {
	    fdSetOpen(fd, url, flags, mode);
	    fdSetIo(fd, ufdio);
	    fd->rd_timeoutsecs = 600;	/* XXX W2DO? 10 mins? */
	    fd->contentLength = fd->bytesRemain = -1;
	}
	break;
    case URL_IS_PATH:
    case URL_IS_UNKNOWN:
    default:
	fd = fdOpen(path, flags, mode);
	if (fd) {
	    fdSetIo(fd, ufdio);
	    fd->rd_timeoutsecs = 1;
	    fd->contentLength = fd->bytesRemain = -1;
	}
	break;
    }

    if (fd == NULL) return NULL;
    if (Fileno(fd) < 0) {
	(void) ufdClose(fd);
	return NULL;
    }
/*@=usereleased@*/
DBGIO(fd, (stderr, "<--\tufdOpen(\"%s\",%x,0%o) %s\n", url, (unsigned)flags, (unsigned)mode, fdbg(fd)));
    return fd;
}

/*@-type@*/ /* LCL: function typedefs */
static struct FDIO_s ufdio_s = {
  ufdRead, ufdWrite, ufdSeek, ufdClose,	NULL, NULL, NULL,
};
/*@=type@*/

FDIO_t ufdio = /*@-compmempass@*/ &ufdio_s /*@=compmempass@*/ ;

/* =============================================================== */
/*@observer@*/
static const char * getFdErrstr (FD_t fd)
	/*@*/
{
    const char *errstr = NULL;

#if defined(WITH_ZLIB)
    if (fdGetIo(fd) == gzdio) {
	errstr = (const char *)fd->errcookie;
    } else
#endif	/* WITH_ZLIB */

#if defined(WITH_BZIP2)
    if (fdGetIo(fd) == bzdio) {
	errstr = (const char *)fd->errcookie;
    } else
#endif

#if defined(WITH_XZ)
    if (fdGetIo(fd) == lzdio) {
	errstr = (const char *)fd->errcookie;
    } else
    if (fdGetIo(fd) == xzdio) {
	errstr = (const char *)fd->errcookie;
    } else
#endif

    {
	errstr = (fd->syserrno ? strerror(fd->syserrno) : "");
    }

    return errstr;
}

/* =============================================================== */

const char *Fstrerror(FD_t fd)
{
    if (fd == NULL)
	return (errno ? strerror(errno) : "");
    FDSANE(fd);
    return getFdErrstr(fd);
}

#define	FDIOVEC(_fd, _vec)	\
  ((fdGetIo(_fd) && fdGetIo(_fd)->_vec) ? fdGetIo(_fd)->_vec : NULL)

size_t Fread(void *buf, size_t size, size_t nmemb, FD_t fd) {
    fdio_read_function_t _read;
    int rc;

    FDSANE(fd);
DBGIO(fd, (stderr, "==> Fread(%p,%u,%u,%p) %s\n", buf, (unsigned)size, (unsigned)nmemb, (fd ? fd : NULL), fdbg(fd)));

    if (fdGetIo(fd) == fpio) {
	/*@+voidabstract -nullpass@*/
	rc = (int) fread(buf, size, nmemb, fdGetFILE(fd));
	/*@=voidabstract =nullpass@*/
	return (size_t) rc;
    }

    /*@-nullderef@*/
    _read = FDIOVEC(fd, read);
    /*@=nullderef@*/

    rc = (int) (_read ? (*_read) (fd, (char *)buf, size * nmemb) : -2);
    return (size_t) rc;
}

size_t Fwrite(const void *buf, size_t size, size_t nmemb, FD_t fd)
{
    fdio_write_function_t _write;
    int rc;

    FDSANE(fd);
DBGIO(fd, (stderr, "==> Fwrite(%p,%u,%u,%p) %s\n", buf, (unsigned)size, (unsigned)nmemb, (fd ? fd : NULL), fdbg(fd)));

    if (fdGetIo(fd) == fpio) {
	/*@+voidabstract -nullpass@*/
	rc = (int) fwrite(buf, size, nmemb, fdGetFILE(fd));
	/*@=voidabstract =nullpass@*/
	return (size_t) rc;
    }

    /*@-nullderef@*/
    _write = FDIOVEC(fd, write);
    /*@=nullderef@*/

    rc = (int) (_write ? _write(fd, (const char *)buf, size * nmemb) : -2);
    return (size_t) rc;
}

int Fseek(FD_t fd, _libio_off_t offset, int whence)
{
    fdio_seek_function_t _seek;
#ifdef USE_COOKIE_SEEK_POINTER
    _IO_off64_t o64 = offset;
    _libio_pos_t pos = &o64;
#else
    _libio_pos_t pos = offset;
#endif

    long int rc;

    FDSANE(fd);
DBGIO(fd, (stderr, "==> Fseek(%p,%ld,%d) %s\n", fd, (long)offset, whence, fdbg(fd)));

    if (fdGetIo(fd) == fpio)
	return fseek(fdGetFILE(fd), (long)offset, whence);

    /*@-nullderef@*/
    _seek = FDIOVEC(fd, seek);
    /*@=nullderef@*/

    rc = (_seek ? _seek(fd, pos, whence) : -2);
    return rc;
}

long Ftell(FD_t fd)
{
    long int rc = -2;

    FDSANE(fd);

    if (fdGetIo(fd) == fpio)
	rc = ftell(fdGetFILE(fd));
    else
	errno = EBADF;
DBGIO(fd, (stderr, "<== Ftell(%p) rc %ld %s\n", fd, rc, fdbg(fd)));
    return rc;
}

void Rewind(FD_t fd)
{
    FDSANE(fd);
DBGIO(fd, (stderr, "==> Rewind(%p) %s\n", fd, fdbg(fd)));

    if (fdGetIo(fd) == fpio)
	rewind(fdGetFILE(fd));
}

int Fgetpos(FD_t fd, fpos_t *pos)
{
    int rc = -2;

    FDSANE(fd);

    if (fdGetIo(fd) == fpio)
	rc = fgetpos(fdGetFILE(fd), pos);
    else
	errno = EBADF;
DBGIO(fd, (stderr, "<== Fgetpos(%p,%p) rc %d %s\n", fd, pos, rc, fdbg(fd)));
    return rc;
}

int Fsetpos(FD_t fd, fpos_t *pos)
{
    int rc = -2;

    FDSANE(fd);

    if (fdGetIo(fd) == fpio)
	return fgetpos(fdGetFILE(fd), pos);

    errno = EBADF;
DBGIO(fd, (stderr, "<== Fsetpos(%p,%p) rc %d %s\n", fd, pos, rc, fdbg(fd)));
    return rc;
}

int Fclose(FD_t fd)
{
    int rc = 0, ec = 0;

    FDSANE(fd);
DBGIO(fd, (stderr, "==> Fclose(%p) %s\n", (fd ? fd : NULL), fdbg(fd)));

/*@-usereleased@*/
    fd = fdLink(fd, "Fclose");
    if (fd != NULL)
    while (fd->nfps >= 0) {
	FDSTACK_t * fps = &fd->fps[fd->nfps];
	
	if (fps->io == fpio) {
	    FILE *fp;
	    int fpno;

	    /*@+voidabstract -nullpass@*/
	    fp = fdGetFILE(fd);
	    fpno = fileno(fp);
	    /*@=voidabstract =nullpass@*/
	/* XXX persistent HTTP/1.1 returns the previously opened fp */
	    if (fd->nfps > 0 && fpno == -1 &&
		fd->fps[fd->nfps-1].io == ufdio &&
		fd->fps[fd->nfps-1].fp == fp &&
		(fd->fps[fd->nfps-1].fdno >= 0 || fd->req != NULL))
	    {
		int hadreqpersist = (fd->req != NULL);

		if (fp)
		    rc = fflush(fp);
		fd->nfps--;
		/*@-refcounttrans@*/
		rc = ufdClose(fd);
		/*@=refcounttrans@*/
		if (fdGetFdno(fd) >= 0)
		    break;
		if (!fd->persist)
		    hadreqpersist = 0;
		fdSetFp(fd, NULL);
		fd->nfps++;
		if (fp) {
		    /* HACK: flimsy Keepalive wiring. */
		    if (hadreqpersist) {
#ifdef	NOTYET	/* XXX not quite right yet. */
			(void) davDisconnect(fd);
			fd->req = NULL;
#endif
			fd->nfps--;
/*@-exposetrans@*/
			fdSetFp(fd, fp);
/*@=exposetrans@*/
/*@-refcounttrans@*/
			(void) fdClose(fd);
/*@=refcounttrans@*/
			fdSetFp(fd, NULL);
			fd->nfps++;
/*@-refcounttrans@*/
			(void) fdClose(fd);
/*@=refcounttrans@*/
		    } else
			rc = fclose(fp);
		}
		fdPop(fd);
		if (noLibio)
		    fdSetFp(fd, NULL);
	    } else {
		if (fp)
		    rc = fclose(fp);
		if (fpno == -1) {
		    fd = fdFree(fd, "fopencookie (Fclose)");
		    fdPop(fd);
		}
	    }
	} else {
	    /*@-nullderef@*/
	    fdio_close_function_t _close = FDIOVEC(fd, close);
	    /*@=nullderef@*/
	    rc = _close(fd);
	}
	if (fd == NULL || fd->nfps == 0)	/* XXX fd != NULL ever */
	    break;
	if (ec == 0 && rc)
	    ec = rc;
	fdPop(fd);
    }
    fd = fdFree(fd, "Fclose");
    return ec;
/*@=usereleased@*/
}

/**
 * Convert stdio fmode to open(2) mode, filtering out zlib/bzlib flags.
 *	returns stdio[0] = NUL on error.
 *
 * @todo glibc also supports ",ccs="
 *
 * - glibc:	m use mmap'd input
 * - glibc:	c no cancel
 * - gzopen:	[0-9] is compression level
 * - gzopen:	'f' is filtered (Z_FILTERED)
 * - gzopen:	'h' is Huffman encoding (Z_HUFFMAN_ONLY)
 * - bzopen:	[1-9] is block size (in 100K units)
 * - bzopen:	's' is smallmode
 * - bzopen:	'q' sets verbosity to 0
 * - bzopen:	'v' does verbosity++ (up to 4)
 * - HACK:	'.' terminates, rest is type of I/O
 */
static inline void cvtfmode (const char *m,
				/*@out@*/ char *stdio, size_t nstdio,
				/*@out@*/ char *other, size_t nother,
				/*@out@*/ const char **end, /*@out@*/ int * f)
	/*@modifies *stdio, *other, *end, *f @*/
{
    int flags = 0;
    char c;

    switch (*m) {
    case 'a':
	flags |= O_WRONLY | O_CREAT | O_APPEND;
	if (--nstdio > 0) *stdio++ = *m;
	break;
    case 'w':
	flags |= O_WRONLY | O_CREAT | O_TRUNC;
	if (--nstdio > 0) *stdio++ = *m;
	break;
    case 'r':
	flags |= O_RDONLY;
	if (--nstdio > 0) *stdio++ = *m;
	break;
    default:
	*stdio = '\0';
	return;
	/*@notreached@*/ break;
    }
    m++;

    while ((c = *m++) != '\0') {
	switch (c) {
	case '.':
	    /*@switchbreak@*/ break;
	case '+':
	    flags &= ~(O_RDONLY|O_WRONLY);
	    flags |= O_RDWR;
	    if (--nstdio > 0) *stdio++ = c;
	    continue;
	    /*@notreached@*/ /*@switchbreak@*/ break;
	case 'x':	/* glibc: open file exclusively. */
	    flags |= O_EXCL;
	    /*@fallthrough@*/
	case 'm':	/* glibc: mmap'd reads */
	case 'c':	/* glibc: no cancel */
#if defined(__GLIBC__) && __GLIBC__ == 2 && __GLIBC_MINOR__ >= 3
	    if (--nstdio > 0) *stdio++ = c;
#endif
	    continue;
	    /*@notreached@*/ /*@switchbreak@*/ break;
	case 'b':
	    if (--nstdio > 0) *stdio++ = c;
	    continue;
	    /*@notreached@*/ /*@switchbreak@*/ break;
	default:
	    if (--nother > 0) *other++ = c;
	    continue;
	    /*@notreached@*/ /*@switchbreak@*/ break;
	}
	break;
    }
    if (c == '\0') 	m--;	/* one too far */

    *stdio = *other = '\0';
    if (end != NULL)
	*end = (*m != '\0' ? m : NULL);
    if (f != NULL)
	*f = flags;
}

#if _USE_LIBIO
#if defined(__GLIBC__) && __GLIBC__ == 2 && __GLIBC_MINOR__ == 0
/* XXX retrofit glibc-2.1.x typedef on glibc-2.0.x systems */
typedef _IO_cookie_io_functions_t cookie_io_functions_t;
#endif
#endif

FD_t Fdopen(FD_t ofd, const char *fmode)
{
    char stdio[20], other[20], zstdio[40+1];
    const char *end = NULL;
    FDIO_t iof = NULL;
    FD_t fd = ofd;

if (_rpmio_debug)
fprintf(stderr, "*** Fdopen(%p,%s) %s\n", fd, fmode, fdbg(fd));
    FDSANE(fd);

    if (fmode == NULL)
	return NULL;

    cvtfmode(fmode, stdio, sizeof(stdio), other, sizeof(other), &end, NULL);
    if (stdio[0] == '\0')
	return NULL;
    zstdio[0] = '\0';
    (void) stpcpy( stpcpy(zstdio, stdio), other);

    if (end == NULL && other[0] == '\0')
	/*@-refcounttrans -retalias@*/ return fd; /*@=refcounttrans =retalias@*/

    if (end && *end) {
	if (!strcmp(end, "fdio")) {
	    iof = fdio;
#if defined(WITH_ZLIB)
	} else if (!strcmp(end, "gzdio")) {
	    iof = gzdio;
	    /*@-internalglobs@*/
	    fd = iof->_fdopen(fd, zstdio);
	    /*@=internalglobs@*/
#endif
#if defined(WITH_BZIP2)
	} else if (!strcmp(end, "bzdio")) {
	    iof = bzdio;
	    /*@-internalglobs@*/
	    fd = iof->_fdopen(fd, zstdio);
	    /*@=internalglobs@*/
#endif
#if defined(WITH_XZ)
	} else if (!strcmp(end, "lzdio")) {
	    iof = lzdio;
	    fd = iof->_fdopen(fd, zstdio);
	} else if (!strcmp(end, "xzdio")) {
	    iof = xzdio;
	    fd = iof->_fdopen(fd, zstdio);
#endif
	} else if (!strcmp(end, "ufdio")) {
	    iof = ufdio;
	} else if (!strcmp(end, "fpio")) {
	    iof = fpio;
	    if (noLibio) {
		int fdno = Fileno(fd);
		FILE * fp = fdopen(fdno, stdio);
/*@+voidabstract -nullpass@*/
if (_rpmio_debug)
fprintf(stderr, "*** Fdopen fpio fp %p\n", (void *)fp);
/*@=voidabstract =nullpass@*/
		if (fp == NULL)
		    return NULL;
		/* XXX gzdio/bzdio use fp for private data */
		/*@+voidabstract@*/
		if (fdGetFp(fd) == NULL)
		    fdSetFp(fd, fp);
		fdPush(fd, fpio, fp, fdno);	/* Push fpio onto stack */
		/*@=voidabstract@*/
	    }
	}
    } else if (other[0] != '\0') {
	for (end = other; *end && strchr("0123456789fh", *end); end++)
	    {};
	if (*end == '\0') {
#if defined(WITH_ZLIB)
	    iof = gzdio;
	    /*@-internalglobs@*/
	    fd = iof->_fdopen(fd, zstdio);
	    /*@=internalglobs@*/
#endif
	}
    }
    if (iof == NULL)
	/*@-refcounttrans -retalias@*/ return fd; /*@=refcounttrans =retalias@*/

    if (!noLibio) {
	FILE * fp = NULL;

#if _USE_LIBIO
	{   cookie_io_functions_t ciof;
	    ciof.read = iof->read;
	    ciof.write = iof->write;
	    ciof.seek = iof->seek;
	    ciof.close = iof->close;
	    fp = fopencookie(fd, stdio, ciof);
DBGIO(fd, (stderr, "<-- fopencookie(%p,\"%s\",*%p) returns fp %p\n", fd, stdio, iof, fp));
	}
#endif

	if (fp) {
	    /* XXX gzdio/bzdio use fp for private data */
	    /*@+voidabstract -nullpass@*/
	    if (fdGetFp(fd) == NULL)
		fdSetFp(fd, fp);
	    fdPush(fd, fpio, fp, fileno(fp));	/* Push fpio onto stack */
	    /*@=voidabstract =nullpass@*/
	    fd = fdLink(fd, "fopencookie");
	}
    }

/*@-refcounttrans -retalias -usereleased @*/
DBGIO(fd, (stderr, "<== Fdopen(%p,\"%s\") returns fd %p %s\n", ofd, fmode, (fd ? fd : NULL), fdbg(fd)));
    return fd;
/*@=refcounttrans =retalias =usereleased @*/
}

FD_t Fopen(const char *path, const char *_fmode)
{
    const char * fmode = NULL;
    char stdio[20], other[20];
    const char *end = NULL;
    mode_t perms = 0666;
    int flags = 0;
    FD_t fd = NULL;

    if (path == NULL || _fmode == NULL)
	goto exit;
/*@-globs -mods@*/
    fmode = rpmExpand(_fmode, NULL);
/*@=globs =mods@*/

if (_rpmio_debug)
fprintf(stderr, "==> Fopen(%s, %s)\n", path, fmode);

    stdio[0] = '\0';
    cvtfmode(fmode, stdio, sizeof(stdio), other, sizeof(other), &end, &flags);
    if (stdio[0] == '\0')
	goto exit;

    if (end && !strcmp(end, "fdio")) {
	fd = fdOpen(path, flags, perms);
	if (fdFileno(fd) < 0) {
	    if (fd) (void) fdClose(fd);
	    fd = NULL;
	    goto exit;
	}
    } else {
	FILE *fp;
	int fdno;
	int isHTTP = 0;

	/* XXX gzdio/bzdio/lzdio through here too */

	switch (urlIsURL(path)) {
	case URL_IS_HKP:
	case URL_IS_HTTP:
	case URL_IS_HTTPS:
	    isHTTP = 1;
	    /*@fallthrough@*/
	case URL_IS_PATH:
	case URL_IS_DASH:
	case URL_IS_FTP:
	case URL_IS_UNKNOWN:
	    fd = ufdOpen(path, flags, perms);
	    if (fd == NULL || !(fdFileno(fd) >= 0 || fd->req != NULL)) {
		if (fd) (void) fdClose(fd);
		fd = NULL;
		goto exit;
	    }
	    break;
	case URL_IS_MONGO:	/* XXX FIXME */
	default:
	    if (fd) (void) fdClose(fd);
	    fd = NULL;
	    goto exit;
	    /*@notreached@*/ break;
	}

	/* XXX persistent HTTP/1.1 returns the previously opened fp */
	if (isHTTP && ((fp = (FILE *) fdGetFp(fd)) != NULL)
	 && ((fdno = fdGetFdno(fd)) >= 0 || fd->req != NULL))
	{
	    /*@+voidabstract@*/
	    fdPush(fd, fpio, fp, fileno(fp));	/* Push fpio onto stack */
	    /*@=voidabstract@*/
	    goto exit;
	}
    }

    if (fd)
	fd = Fdopen(fd, fmode);
exit:

if (_rpmio_debug)
fprintf(stderr, "<== Fopen(%s, %s) fd %p\n", path, fmode, fd);
    fmode = _free(fmode);
    return fd;
}

int Fflush(FD_t fd)
{
    void * vh;
    if (fd == NULL) return -1;
    if (fdGetIo(fd) == fpio)
	/*@+voidabstract -nullpass@*/
	return fflush(fdGetFILE(fd));
	/*@=voidabstract =nullpass@*/

    vh = fdGetFp(fd);
#if defined(WITH_ZLIB)
    if (vh && fdGetIo(fd) == gzdio && gzdio->_flush != NULL)
	return (*gzdio->_flush) ((void *)fd);
#endif
#if defined(WITH_BZIP2)
    if (vh && fdGetIo(fd) == bzdio && bzdio->_flush != NULL)
	return (*bzdio->_flush) ((void *)fd);
#endif
#if defined(WITH_XZ)
    if (vh && fdGetIo(fd) == lzdio && lzdio->_flush != NULL)
	return (*lzdio->_flush) ((void *)fd);
    if (vh && fdGetIo(fd) == xzdio && xzdio->_flush != NULL)
	return (*xzdio->_flush) ((void *)fd);
#endif

    return 0;
}

int Ferror(FD_t fd)
{
    int i, rc = 0;

    if (fd == NULL) return -1;
    if (fd->req != NULL) {
	/* HACK: flimsy wiring for neon errors. */
	rc = (fd->req == (void *)-1 || fd->syserrno  || fd->errcookie != NULL) ? -1 : 0;
    } else
    for (i = fd->nfps; rc == 0 && i >= 0; i--) {
	FDSTACK_t * fps = &fd->fps[i];
	int ec;
	
	if (fps->io == fpio) {
	    /*@+voidabstract -nullpass@*/
	    ec = ferror(fdGetFILE(fd));
	    /*@=voidabstract =nullpass@*/
#if defined(WITH_ZLIB)
	} else if (fps->io == gzdio) {
	    ec = (fd->syserrno || fd->errcookie != NULL) ? -1 : 0;
	    i--;	/* XXX fdio under gzdio always has fdno == -1 */
#endif
#if defined(WITH_BZIP2)
	} else if (fps->io == bzdio) {
	    ec = (fd->syserrno  || fd->errcookie != NULL) ? -1 : 0;
	    i--;	/* XXX fdio under bzdio always has fdno == -1 */
#endif
#if defined(WITH_XZ)
	} else if (fps->io == lzdio) {
	    ec = (fd->syserrno  || fd->errcookie != NULL) ? -1 : 0;
	    i--;	/* XXX fdio under lzdio always has fdno == -1 */
	} else if (fps->io == xzdio) {
	    ec = (fd->syserrno  || fd->errcookie != NULL) ? -1 : 0;
	    i--;	/* XXX fdio under xzdio always has fdno == -1 */
#endif
	} else {
	/* XXX need to check ufdio/gzdio/bzdio/fdio errors correctly. */
	    ec = (fdFileno(fd) < 0 ? -1 : 0);
	}

	if (rc == 0 && ec)
	    rc = ec;
    }
DBGIO(fd, (stderr, "<== Ferror(%p) rc %d %s\n", fd, rc, fdbg(fd)));
    return rc;
}

int Fileno(FD_t fd)
{
    int i, rc = -1;

    if (fd == NULL)
        return -1;
    if (fd->req != NULL)
	rc = 123456789;	/* HACK: https has no steenkin fileno. */
    else
    for (i = fd->nfps ; rc == -1 && i >= 0; i--) {
	rc = fd->fps[i].fdno;
    }

DBGIO(fd, (stderr, "<== Fileno(%p) rc %d %s\n", (fd ? fd : NULL), rc, fdbg(fd)));
    return rc;
}

/* XXX this is naive */
int Fcntl(FD_t fd, int op, void *lip)
{
    return fcntl(Fileno(fd), op, lip);
}

/* =============================================================== */
/* Helper routines that may be generally useful.
 */
int rpmioMkpath(const char * path, mode_t mode, uid_t uid, gid_t gid)
{
    char * d, * de;
    int created = 0;
    int rc;

    if (path == NULL || *path == '\0')
	return -1;
    d = (char *) alloca(strlen(path)+2);
    de = stpcpy(d, path);
    de[1] = '\0';
    for (de = d; *de != '\0'; de++) {
	struct stat st;
	char savec;

	while (*de && *de != '/') de++;
	savec = de[1];
	de[1] = '\0';

	rc = Stat(d, &st);
	if (rc) {
	    switch(errno) {
	    default:
		return errno;
		/*@notreached@*/ /*@switchbreak@*/ break;
	    case ENOENT:
		/*@switchbreak@*/ break;
	    }
	    rc = Mkdir(d, mode);
	    if (rc)
		return errno;
	    created = 1;
	    if (!(uid == (uid_t) -1 && gid == (gid_t) -1)) {
		rc = Chown(d, uid, gid);
		if (rc)
		    return errno;
	    }
	} else if (!S_ISDIR(st.st_mode)) {
	    return ENOTDIR;
	}
	de[1] = savec;
    }
    rc = 0;
    if (created)
	rpmlog(RPMLOG_DEBUG, D_("created directory(s) %s mode 0%o\n"),
			path, (unsigned)mode);
    return rc;
}

#define	_PATH	"/bin:/usr/bin:/sbin:/usr/sbin"
/*@unchecked@*/ /*@observer@*/
static const char *_path = _PATH;

#define alloca_strdup(_s)       strcpy((char *)alloca(strlen(_s)+1), (_s))

int rpmioAccess(const char * FN, const char * path, int mode)
{
    char fn[4096];
    char * bn;
    char * r, * re;
    char * t, * te;
    int negate = 0;
    int rc = 0;

    /* Empty paths are always accessible. */
    if (FN == NULL || *FN == '\0')
	return 0;

    if (mode == 0)
	mode = X_OK;

    /* Strip filename out of its name space wrapper. */
    bn = (char *) alloca_strdup(FN);
    for (t = bn; t && *t; t++) {
	if (*t != '(')
	    continue;
	*t++ = '\0';

	/* Permit negation on name space tests. */
	if (*bn == '!') {
	    negate = 1;
	    bn++;
	}

	/* Set access flags from name space marker. */
	if (strlen(bn) == 3
	 && strchr("Rr_", bn[0]) != NULL
	 && strchr("Ww_", bn[1]) != NULL
	 && strchr("Xx_", bn[2]) != NULL) {
	    mode = 0;
	    if (strchr("Rr", bn[0]) != NULL)
		mode |= R_OK;
	    if (strchr("Ww", bn[1]) != NULL)
		mode |= W_OK;
	    if (strchr("Xx", bn[2]) != NULL)
		mode |= X_OK;
	    if (mode == 0)
		mode = F_OK;
	} else if (!strcmp(bn, "exists"))
	    mode = F_OK;
	else if (!strcmp(bn, "executable"))
	    mode = X_OK;
	else if (!strcmp(bn, "readable"))
	    mode = R_OK;
	else if (!strcmp(bn, "writable"))
	    mode = W_OK;

	bn = t;
	te = bn + strlen(t) - 1;
	if (*te != ')')		/* XXX syntax error, never exists */
	    return 1;
	*te = '\0';
	break;
    }

    /* Empty paths are always accessible. */
    if (*bn == '\0')
	goto exit;

    /* Check absolute path for access. */
    if (*bn == '/') {
	rc = (Access(bn, mode) != 0 ? 1 : 0);
if (_rpmio_debug)
fprintf(stderr, "*** rpmioAccess(\"%s\", 0x%x) rc %d\n", bn, mode, rc);
	goto exit;
    }

    /* Find path to search. */
    if (path == NULL)
	path = getenv("PATH");
    if (path == NULL)
	path = _path;
    if (path == NULL) {
	rc = 1;
	goto exit;
    }

    /* Look for relative basename on PATH. */
    for (r = alloca_strdup(path); r != NULL && *r != '\0'; r = re) {

	/* Find next element, terminate current element. */
	for (re = r; (re = strchr(re, ':')) != NULL; re++) {
	    if (!(re[1] == '/' && re[2] == '/'))
		/*@innerbreak@*/ break;
	}
	if (re && *re == ':')
	    *re++ = '\0';
	else
	    re = r + strlen(r);

	/* Expand ~/ to $HOME/ */
	fn[0] = '\0';
	t = fn;
	*t = '\0';	/* XXX redundant. */
	if (r[0] == '~' && r[1] == '/') {
	    const char * home = getenv("HOME");
	    if (home == NULL)	/* XXX No HOME? */
		continue;
	    if (strlen(home) > (sizeof(fn) - strlen(r))) /* XXX too big */
		continue;
	    t = stpcpy(t, home);
	    r++;	/* skip ~ */
	}
	t = stpcpy(t, r);
	if (t[-1] != '/' && *bn != '/')
	    *t++ = '/';
	t = stpcpy(t, bn);
	t = rpmCleanPath(fn);
	if (t == NULL)	/* XXX can't happen */
	    continue;

	/* Check absolute path for access. */
	rc = (Access(t, mode) != 0 ? 1 : 0);
if (_rpmio_debug)
fprintf(stderr, "*** rpmioAccess(\"%s\", 0x%x) rc %d\n", t, mode, rc);
	if (rc == 0)
	    goto exit;
    }

    rc = 1;

exit:
    if (negate)
	rc ^= 1;
    return rc;
}

#if defined(WITH_NSS) && !defined(__LCLINT__)	/* XXX TODO: add nssDestroy */
#ifdef __cplusplus
extern "C" {
#endif
/*@-exportheader@*/
#ifdef	HAVE_NSS_INITCONTEXT
extern void NSS_ShutdownContext(void * _nss_context);
#else
extern void NSS_Shutdown(void);
#endif
extern void PL_ArenaFinish(void);
extern void PR_Cleanup(void);
/*@=exportheader@*/
#ifdef __cplusplus
}
#endif

/*@unchecked@*/
int _rpmnss_init = 0;

/*@unchecked@*/
void * _rpmnss_context = NULL;
#endif

void rpmioClean(void)
{
/*@-nestedextern@*/
    extern rpmioPool _avxPool;
    extern rpmioPool _urlPool;
    extern rpmioPool _xarPool;
    extern rpmioPool _digPool;
    extern rpmioPool _rpmiobPool;
    extern rpmioPool _rpmvcPool;
    extern rpmioPool _rpmvtPool;
/*@-shadow@*/
    extern rpmioPool _mirePool;
    extern rpmioPool _rpmbfPool;
    extern rpmioPool _rpmhkpPool;
    extern rpmioPool _htmlPool;
    extern rpmioPool _htPool;
    extern rpmioPool _cphPool;
    extern rpmioPool _ctxPool;
    extern rpmioPool _rpmsmPool;
    extern rpmioPool _rpmspPool;
    extern rpmioPool _rpmsxPool;
    extern rpmioPool _rpmsyckPool;
/*@=shadow@*/

    extern rpmioPool _rpmasnPool;
    extern rpmioPool _rpmbagPool;
    extern rpmioPool _rpmcvsPool;
    extern rpmioPool _rpmgitPool;
    extern rpmioPool _rpmsetPool;
    extern rpmioPool _rpmsvnPool;
    extern rpmioPool _rpmtpmPool;

    extern rpmioPool _rpmaugPool;
    extern rpmioPool _rpmcudfPool;
    extern rpmioPool _rpmficlPool;
    extern rpmioPool _rpmjniPool;
    extern rpmioPool _rpmjsPool;
    extern rpmioPool _rpmluavPool;
    extern rpmioPool _rpmluaPool;
    extern rpmioPool _rpmmgPool;
    extern rpmioPool _rpmmgoPool;
    extern rpmioPool _rpmmrbPool;
#ifdef	NOTYET
    extern rpmioPool _rpmnixPool;
#endif
    extern rpmioPool _odbcPool;
    extern rpmioPool _rpmperlPool;
    extern rpmioPool _rpmpythonPool;
    extern rpmioPool _rpmrubyPool;
    extern rpmioPool _rpmsqlPool;
    extern rpmioPool _rpmsquirrelPool;
    extern rpmioPool _rpmtclPool;
/*@=nestedextern@*/

#if defined(WITH_LUA)
    (void) rpmluaFree(NULL);
#endif
#if defined(WITH_NEON)
    davDestroy();
#endif
#if defined(WITH_NSS) && !defined(__LCLINT__)
    if (_rpmnss_init) {
#ifdef	HAVE_NSS_INITCONTEXT
	if (_rpmnss_context != NULL)
	    (void) NSS_ShutdownContext(_rpmnss_context);
#else
	(void) NSS_Shutdown();
#endif
	PL_ArenaFinish();
	PR_Cleanup();
	_rpmnss_context = NULL;
	_rpmnss_init = 0;
    }
#endif
    urlFreeCache();

    _rpmtclI = rpmtclFree(_rpmtclI);
    _rpmtclPool = rpmioFreePool(_rpmtclPool);
    _rpmsquirrelI = rpmsquirrelFree(_rpmsquirrelI);
    _rpmsquirrelPool = rpmioFreePool(_rpmsquirrelPool);
    _rpmsqlI = rpmsqlFree(_rpmsqlI);
    _rpmsqlPool = rpmioFreePool(_rpmsqlPool);
    _rpmrubyI = rpmrubyFree(_rpmrubyI);
    _rpmrubyPool = rpmioFreePool(_rpmrubyPool);
    _rpmpythonI = rpmpythonFree(_rpmpythonI);
    _rpmpythonPool = rpmioFreePool(_rpmpythonPool);
    _rpmperlI = rpmperlFree(_rpmperlI);
    _rpmperlPool = rpmioFreePool(_rpmperlPool);
    _rpmmrbI = rpmmrbFree(_rpmmrbI);
    _rpmmrbPool = rpmioFreePool(_rpmmrbPool);
    _rpmjsI = rpmjsFree(_rpmjsI);
    _rpmjsPool = rpmioFreePool(_rpmjsPool);
    _rpmjniI = rpmjniFree(_rpmjniI);
    _rpmjniPool = rpmioFreePool(_rpmjniPool);
    _rpmficlI = rpmficlFree(_rpmficlI);
    _rpmficlPool = rpmioFreePool(_rpmficlPool);

    _rpmgitI = rpmgitFree(_rpmgitI);
    _rpmgitPool = rpmioFreePool(_rpmgitPool);

    _rpmaugI = rpmaugFree(_rpmaugI);
    _rpmaugPool = rpmioFreePool(_rpmaugPool);
    _rpmmgoI = rpmmgoFree(_rpmmgoI);
    _rpmmgoPool = rpmioFreePool(_rpmmgoPool);

    _rpmasnPool = rpmioFreePool(_rpmasnPool);
    _rpmbagPool = rpmioFreePool(_rpmbagPool);
    _rpmcvsPool = rpmioFreePool(_rpmcvsPool);
    _odbcPool = rpmioFreePool(_odbcPool);
    _rpmsetPool = rpmioFreePool(_rpmsetPool);
    _rpmsvnPool = rpmioFreePool(_rpmsvnPool);
    _rpmtpmPool = rpmioFreePool(_rpmtpmPool);

#ifdef	NOTYET	/* XXX FIXME: dig out the recursion deadlock. */
    _rpmnixI = rpmnixFree(_rpmnixI);
    _rpmnixPool = rpmioFreePool(_rpmnixPool);
#endif

    _rpmcudfPool = rpmioFreePool(_rpmcudfPool);
    _rpmluavPool = rpmioFreePool(_rpmluavPool);
    _rpmluaPool = rpmioFreePool(_rpmluaPool);

    _rpmhkpI = rpmhkpFree(_rpmhkpI);
    _rpmhkpPool = rpmioFreePool(_rpmhkpPool);
    _rpmhkp_awol.bf = rpmbfFree(_rpmhkp_awol.bf);
    _rpmhkp_crl.bf = rpmbfFree(_rpmhkp_crl.bf);

    _rpmvcPool = rpmioFreePool(_rpmvcPool);
    _rpmvtPool = rpmioFreePool(_rpmvtPool);

    _rpmsmI = rpmsmFree(_rpmsmI);
    _rpmsmPool = rpmioFreePool(_rpmsmPool);
    _rpmspPool = rpmioFreePool(_rpmspPool);
    _rpmsxI = rpmsxFree(_rpmsxI);
    _rpmsxPool = rpmioFreePool(_rpmsxPool);

    _htmlPool = rpmioFreePool(_htmlPool);
    _mirePool = rpmioFreePool(_mirePool);
    _rpmmgPool = rpmioFreePool(_rpmmgPool);
    _rpmbfPool = rpmioFreePool(_rpmbfPool);
    _htPool = rpmioFreePool(_htPool);
    _cphPool = rpmioFreePool(_cphPool);
    _ctxPool = rpmioFreePool(_ctxPool);
    _rpmsyckPool = rpmioFreePool(_rpmsyckPool);
    _rpmiobPool = rpmioFreePool(_rpmiobPool);
    _digPool = rpmioFreePool(_digPool);
    _xarPool = rpmioFreePool(_xarPool);
    _avxPool = rpmioFreePool(_avxPool);
    _urlPool = rpmioFreePool(_urlPool);
    _fdPool = rpmioFreePool(_fdPool);

    rpmlogClose();
}

/*@-type@*/ /* LCL: function typedefs */
static struct FDIO_s fpio_s = {
  ufdRead, ufdWrite, fdSeek, ufdClose, NULL, NULL, NULL,
};
/*@=type@*/

FDIO_t fpio = /*@-compmempass@*/ &fpio_s /*@=compmempass@*/ ;
