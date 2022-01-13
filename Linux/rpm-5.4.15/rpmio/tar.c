/** \ingroup payload
 * \file rpmio/tar.c
 *  Handle ustar archives.
 */

#undef JBJ_WRITEPAD

#include "system.h"

#include <rpmio.h>
#include <ugid.h>
#include <tar.h>
#define	_IOSM_INTERNAL
#include <iosm.h>

#include "debug.h"

/*@access IOSM_t @*/

/*@unchecked@*/
int _tar_debug = 0;

/*@unchecked@*/
static int nochksum = 0;

/**
 * Convert string to unsigned integer (with buffer size check).
 * @param str		input string
 * @retval *endptr	1st character not processed
 * @param base		numerical conversion base
 * @param num		max no. of bytes to read
 * @return		converted integer
 */
static int strntoul(const char *str, /*@null@*/ /*@out@*/char **endptr,
		int base, size_t num)
	/*@modifies *endptr @*/
	/*@requires maxSet(endptr) >= 0 @*/
{
    char * buf, * end;
    unsigned long ret;

    buf = (char *) alloca(num + 1);
    strncpy(buf, str, num);
    buf[num] = '\0';

    ret = strtoul(buf, &end, base);
    if (endptr != NULL) {
	if (*end != '\0')
	    *endptr = ((char *)str) + (end - buf);	/* XXX discards const */
	else
	    *endptr = ((char *)str) + strlen(buf);
    }

    return ret;
}

/* Translate archive read/write ssize_t return for iosmStage(). */
#define	_IOSMRC(_rc)	\
	if ((_rc) <= 0)	return ((_rc) ? (int) -rc : IOSMERR_HDR_TRAILER)

static ssize_t tarRead(void * _iosm, void * buf, size_t count)
	/*@globals fileSystem @*/
	/*@modifies _iosm, *buf, fileSystem @*/
{
    IOSM_t iosm = (IOSM_t) _iosm;
    char * t = (char *) buf;
    size_t nb = 0;

if (_tar_debug)
fprintf(stderr, "\ttarRead(%p, %p[%u])\n", iosm, buf, (unsigned)count);

    while (count > 0) {
	size_t rc;

	/* Read next tar block. */
	iosm->wrlen = count;
	rc = _iosmNext(iosm, IOSM_DREAD);
	if (!rc && iosm->rdnb != iosm->wrlen)
	    rc = IOSMERR_READ_FAILED;
	if (rc) return -rc;

	/* Append to buffer. */
	rc = (count > iosm->rdnb ? iosm->rdnb : count);
	if (buf != iosm->wrbuf)
	     memcpy(t + nb, iosm->wrbuf, rc);
	nb += rc;
	count -= rc;
    }
    return nb;
}

/**
 * Read long file/link name from tar archive.
 * @param _iosm		file state machine
 * @param len		no. bytes of name
 * @retval *fnp		long file/link name
 * @return		no. bytes read (rc < 0 on error)
 */
static ssize_t tarHeaderReadName(void * _iosm, size_t len,
		/*@out@*/ const char ** fnp)
	/*@globals fileSystem, internalState @*/
	/*@modifies _iosm, *fnp, fileSystem, internalState @*/
{
    IOSM_t iosm = (IOSM_t) _iosm;
    size_t nb = len + 1;
    char * t = (char *) xmalloc(nb);
    ssize_t rc = tarRead(iosm, t, nb);

    if (rc > 0)		/* success */
	t[rc] = '\0';
     else		/* failure or EOF */
	t = _free(t);
    if (fnp != NULL)
	*fnp = t;

if (_tar_debug)
fprintf(stderr, "\ttarHeaderReadName(%p, %u, %p) rc 0x%x\n", _iosm, (unsigned)len, fnp, (unsigned)rc);

    return rc;
}

int tarHeaderRead(void * _iosm, struct stat * st)
	/*@modifies _iosm, *st @*/
{
    IOSM_t iosm = (IOSM_t) _iosm;
    tarHeader hdr = (tarHeader) iosm->wrbuf;
    char * t;
    size_t nb;
    int major, minor;
    ssize_t rc = 0;
    int zblk = 0;

if (_tar_debug)
fprintf(stderr, "  tarHeaderRead(%p, %p)\n", iosm, st);

top:
    do {
	/* Read next header. */
	rc = tarRead(_iosm, hdr, TAR_BLOCK_SIZE);
	_IOSMRC(rc);

	/* Look for end-of-archive, i.e. 2 (or more) zero blocks. */
	if (hdr->name[0] == '\0' && hdr->checksum[0] == '\0') {
	    if (++zblk == 2)
		return IOSMERR_HDR_TRAILER;
	}
    } while (zblk > 0);

    /* Verify header checksum. */
    {	const unsigned char * hp = (const unsigned char *) hdr;
	char checksum[8];
	char hdrchecksum[8];
	long sum = 0;
	int i;

	memcpy(hdrchecksum, hdr->checksum, sizeof(hdrchecksum));
	memset(hdr->checksum, (int)' ', sizeof(hdr->checksum));

	for (i = 0; i < TAR_BLOCK_SIZE; i++)
	    sum += (long)*hp++;

	memset(checksum, (int)' ', sizeof(checksum));
	sprintf(checksum, "%06o", (unsigned) (sum & 07777777));
if (_tar_debug)
fprintf(stderr, "\tmemcmp(\"%s\", \"%s\", %u)\n", hdrchecksum, checksum, (unsigned)sizeof(hdrchecksum));
	if (memcmp(hdrchecksum, checksum, sizeof(hdrchecksum)))
	    if (!nochksum)
		return IOSMERR_BAD_HEADER;

    }

    /* Verify header magic. */
    if (strncmp(hdr->magic, TAR_MAGIC, sizeof(TAR_MAGIC)-1))
	return IOSMERR_BAD_MAGIC;

    /* Convert header to stat(2). */
    st->st_size = strntoul(hdr->filesize, NULL, 8, sizeof(hdr->filesize));

    st->st_nlink = 1;
    st->st_mode = strntoul(hdr->mode, NULL, 8, sizeof(hdr->mode));
    st->st_mode &= ~S_IFMT;
    switch (hdr->typeflag) {
    case 'x':		/* Extended header referring to next file in archive. */
    case 'g':		/* Global extended header. */
    default:
	break;
    case '7':		/* reserved (contiguous files?) */
    case '\0':		/* (ancient) regular file */
    case '0':		/* regular file */
	st->st_mode |= S_IFREG;
	break;
    case '1':		/* hard link */
	st->st_mode |= S_IFREG;
#ifdef DYING
	st->st_nlink++;
#endif
	break;
    case '2':		/* symbolic link */
	st->st_mode |= S_IFLNK;
	break;
    case '3':		/* character special */
	st->st_mode |= S_IFCHR;
	break;
    case '4':		/* block special */
	st->st_mode |= S_IFBLK;
	break;
    case '5':		/* directory */
	st->st_mode |= S_IFDIR;
	st->st_nlink++;
	break;
    case '6':		/* FIFO special */
	st->st_mode |= S_IFIFO;
	break;
#ifdef	REFERENCE
    case 'A':		/* Solaris ACL */
    case 'E':		/* Solaris XATTR */
    case 'I':		/* Inode only, as in 'star' */
    case 'X':		/* POSIX 1003.1-2001 eXtended (VU version) */
    case 'D':		/* GNU dumpdir (with -G, --incremental) */
    case 'M':		/* GNU multivol (with -M, --multi-volume) */
    case 'N':		/* GNU names */
    case 'S':		/* GNU sparse  (with -S, --sparse) */
    case 'V':		/* GNU tape/volume header (with -Vlll, --label=lll) */
#endif
    case 'K':		/* GNU long (>100 chars) link name */
	rc = tarHeaderReadName(iosm, st->st_size, &iosm->lpath);
	_IOSMRC(rc);
	goto top;
	/*@notreached@*/ break;
    case 'L':		/* GNU long (>100 chars) file name */
	rc = tarHeaderReadName(iosm, st->st_size, &iosm->path);
	_IOSMRC(rc);
	goto top;
	/*@notreached@*/ break;
    }

    st->st_uid = strntoul(hdr->uid, NULL, 8, sizeof(hdr->uid));
    st->st_gid = strntoul(hdr->gid, NULL, 8, sizeof(hdr->gid));
    st->st_mtime = strntoul(hdr->mtime, NULL, 8, sizeof(hdr->mtime));
    st->st_ctime = st->st_atime = st->st_mtime;		/* XXX compat? */

    major = strntoul(hdr->devMajor, NULL, 8, sizeof(hdr->devMajor));
    minor = strntoul(hdr->devMinor, NULL, 8, sizeof(hdr->devMinor));
    /*@-shiftimplementation@*/
    st->st_dev = Makedev(major, minor);
    /*@=shiftimplementation@*/
    st->st_rdev = st->st_dev;		/* XXX compat? */

    /* char prefix[155]; */
    /* char padding[12]; */

    /* Read short file name. */
    if (iosm->path == NULL && hdr->name[0] != '\0') {
	nb = strlen(hdr->name);
	t = (char *) xmalloc(nb + 1);
	memcpy(t, hdr->name, nb);
	t[nb] = '\0';
	iosm->path = t;
    }

    /* Read short link name. */
    if (iosm->lpath == NULL && hdr->linkname[0] != '\0') {
	nb = strlen(hdr->linkname);
	t = (char *) xmalloc(nb + 1);
	memcpy(t, hdr->linkname, nb);
	t[nb] = '\0';
	iosm->lpath = t;
    }

    rc = 0;

if (_tar_debug)
fprintf(stderr, "\t     %06o%3d (%4d,%4d)%12lu %s\n\t-> %s\n",
                (unsigned)st->st_mode, (int)st->st_nlink,
                (int)st->st_uid, (int)st->st_gid, (unsigned long)st->st_size,
                (iosm->path ? iosm->path : ""), (iosm->lpath ? iosm->lpath : ""));

    return (int) rc;
}

static ssize_t tarWrite(void * _iosm, const void *buf, size_t count)
	/*@globals fileSystem @*/
	/*@modifies _iosm, fileSystem @*/
{
    IOSM_t iosm = (IOSM_t) _iosm;
    const char * s = (const char *) buf;
    size_t nb = 0;
    size_t rc;

if (_tar_debug)
fprintf(stderr, "\t   tarWrite(%p, %p[%u])\n", iosm, buf, (unsigned)count);

    while (count > 0) {

	/* XXX DWRITE uses rdnb for I/O length. */
	iosm->rdnb = count;
	if (s != iosm->rdbuf)
	    memmove(iosm->rdbuf, s + nb, iosm->rdnb);

	rc = _iosmNext(iosm, IOSM_DWRITE);
	if (!rc && iosm->rdnb != iosm->wrnb)
		rc = IOSMERR_WRITE_FAILED;
	if (rc) return -rc;

	nb += iosm->rdnb;
	count -= iosm->rdnb;
    }

#if defined(JBJ_WRITEPAD)
    /* Pad to next block boundary. */
    if ((rc = _iosmNext(iosm, IOSM_PAD)) != 0) return rc;
#endif

    return nb;
}

/**
 * Write long file/link name into tar archive.
 * @param _iosm		file state machine
 * @param path		long file/link name
 * @return		no. bytes written (rc < 0 on error)
 */
static ssize_t tarHeaderWriteName(void * _iosm, const char * path)
	/*@globals fileSystem, internalState @*/
	/*@modifies _iosm, fileSystem, internalState @*/
{
    ssize_t rc = tarWrite(_iosm, path, strlen(path));

#if !defined(JBJ_WRITEPAD)
    if (rc >= 0) {
	IOSM_t iosm = (IOSM_t) _iosm;
	rc = _iosmNext(iosm, IOSM_PAD);
	if (rc) rc = -rc;
    }
#endif

if (_tar_debug)
fprintf(stderr, "\ttarHeaderWriteName(%p, %s) rc 0x%x\n", _iosm, path, (unsigned)rc);

    return rc;
}

/**
 * Write tar header block with checksum  into tar archive.
 * @param _iosm		file state machine
 * @param st		file info
 * @param hdr		tar header block
 * @return		no. bytes written (rc < 0 on error)
 */
static ssize_t tarHeaderWriteBlock(void * _iosm, struct stat * st, tarHeader hdr)
	/*@globals fileSystem, internalState @*/
	/*@modifies _iosm, hdr, fileSystem, internalState @*/
{
    IOSM_t iosm = (IOSM_t) _iosm;
    const char * path = (iosm && iosm->path ? iosm->path : "");
    ssize_t rc;

if (_tar_debug)
fprintf(stderr, "\ttarHeaderWriteBlock(%p, %p) type %c\n", iosm, hdr, hdr->typeflag);
if (_tar_debug)
fprintf(stderr, "\t     %06o%3d (%4d,%4d)%12lu %s\n",
                (unsigned)st->st_mode, (int)st->st_nlink,
                (int)st->st_uid, (int)st->st_gid, (unsigned long)st->st_size,
                path);


    (void) stpcpy( stpcpy(hdr->magic, TAR_MAGIC), TAR_VERSION);

    /* Calculate header checksum. */
    {	const unsigned char * hp = (const unsigned char *) hdr;
	long sum = 0;
	int i;

	memset(hdr->checksum, (int)' ', sizeof(hdr->checksum));
	for (i = 0; i < TAR_BLOCK_SIZE; i++)
	    sum += (long) *hp++;
	sprintf(hdr->checksum, "%06o", (unsigned)(sum & 07777777));
if (_tar_debug)
fprintf(stderr, "\thdrchksum \"%s\"\n", hdr->checksum);
    }

    rc = tarWrite(_iosm, hdr, TAR_BLOCK_SIZE);

    return rc;
}

int tarHeaderWrite(void * _iosm, struct stat * st)
{
    IOSM_t iosm = (IOSM_t) _iosm;
/*@observer@*/
    static const char * llname = "././@LongLink";
    tarHeader hdr = (tarHeader) iosm->rdbuf;
    const char * path = (iosm && iosm->path ? iosm->path : "");
    const char * lpath = (iosm && iosm->lpath ? iosm->lpath : "");
    const char * s;
    dev_t dev;
    size_t nb;
    ssize_t rc = 0;

if (_tar_debug)
fprintf(stderr, "    tarHeaderWrite(%p, %p)\n", iosm, st);

    nb = strlen(path);
    if (nb > sizeof(hdr->name)) {
	memset(hdr, 0, sizeof(*hdr));
	strcpy(hdr->name, llname);
	sprintf(hdr->mode, "%07o", 0);
	sprintf(hdr->uid, "%07o", 0);
	sprintf(hdr->gid, "%07o", 0);
	sprintf(hdr->filesize, "%011o", (unsigned) (nb & 037777777777));
	sprintf(hdr->mtime, "%011o", 0);
	hdr->typeflag = 'L';
	strncpy(hdr->uname, "root", sizeof(hdr->uname));
	strncpy(hdr->gname, "root", sizeof(hdr->gname));
	rc = tarHeaderWriteBlock(iosm, st, hdr);
	_IOSMRC(rc);
	rc = tarHeaderWriteName(iosm, path);
	_IOSMRC(rc);
    }

    if (lpath && lpath[0] != '0') {
	nb = strlen(lpath);
	if (nb > sizeof(hdr->name)) {
	    memset(hdr, 0, sizeof(*hdr));
	    strncpy(hdr->linkname, llname, sizeof(hdr->linkname));
	    sprintf(hdr->mode, "%07o", 0);
	    sprintf(hdr->uid, "%07o", 0);
	    sprintf(hdr->gid, "%07o", 0);
	    sprintf(hdr->filesize, "%011o", (unsigned) (nb & 037777777777));
	    sprintf(hdr->mtime, "%011o", 0);
	    hdr->typeflag = 'K';
	    strncpy(hdr->uname, "root", sizeof(hdr->uname));
	    strncpy(hdr->gname, "root", sizeof(hdr->gname));
	    rc = tarHeaderWriteBlock(iosm, st, hdr);
	    _IOSMRC(rc);
	    rc = tarHeaderWriteName(iosm, path);
	    _IOSMRC(rc);
	}
    }

    memset(hdr, 0, sizeof(*hdr));

    strncpy(hdr->name, path, sizeof(hdr->name));

    if (lpath && lpath[0] != '\0')
	strncpy(hdr->linkname, lpath, sizeof(hdr->linkname));

    sprintf(hdr->mode, "%07o", (unsigned int)(st->st_mode & 00007777));
    sprintf(hdr->uid, "%07o", (unsigned int)(st->st_uid & 07777777));
    sprintf(hdr->gid, "%07o", (unsigned int)(st->st_gid & 07777777));

    sprintf(hdr->filesize, "%011o", (unsigned) (st->st_size & 037777777777));
    sprintf(hdr->mtime, "%011o", (unsigned) (st->st_mtime & 037777777777));

    hdr->typeflag = '0';	/* XXX wrong! */
    if (S_ISLNK(st->st_mode))
	hdr->typeflag = '2';
    else if (S_ISCHR(st->st_mode))
	hdr->typeflag = '3';
    else if (S_ISBLK(st->st_mode))
	hdr->typeflag = '4';
    else if (S_ISDIR(st->st_mode))
	hdr->typeflag = '5';
    else if (S_ISFIFO(st->st_mode))
	hdr->typeflag = '6';
#ifdef WHAT2DO
    else if (S_ISSOCK(st->st_mode))
	hdr->typeflag = '?';
#endif
    else if (S_ISREG(st->st_mode))
	hdr->typeflag = (lpath && lpath[0] != '\0' ? '1' : '0');

    /* XXX FIXME: map uname/gname from uid/gid. */
    s = uidToUname(st->st_uid);
    if (s == NULL) s = "root";
    strncpy(hdr->uname, s, sizeof(hdr->uname));
    s = gidToGname(st->st_gid);
    if (s == NULL) s = "root";
    strncpy(hdr->gname, s, sizeof(hdr->gname));

    /* XXX W2DO? st_dev or st_rdev? (likely st->st_rdev) */
    dev = major((unsigned)st->st_dev);
    sprintf(hdr->devMajor, "%07o", (unsigned) (dev & 07777777));
    dev = minor((unsigned)st->st_dev);
    sprintf(hdr->devMinor, "%07o", (unsigned) (dev & 07777777));

    rc = tarHeaderWriteBlock(iosm, st, hdr);
    _IOSMRC(rc);
    rc = 0;

#if !defined(JBJ_WRITEPAD)
    /* XXX Padding is unnecessary but shouldn't hurt. */
    rc = _iosmNext(iosm, IOSM_PAD);
#endif

    return (int) rc;
}

int tarTrailerWrite(void * _iosm)
{
    IOSM_t iosm = (IOSM_t) _iosm;
    ssize_t rc = 0;

if (_tar_debug)
fprintf(stderr, "    tarTrailerWrite(%p)\n", iosm);

    /* Pad up to 20 blocks (10Kb) of zeroes. */
    iosm->blksize *= 20;
#if defined(JBJ_WRITEPAD)
    rc = tarWrite(iosm, NULL, 0);	/* XXX _iosmNext(iosm, IOSM_PAD) */
#else
    rc = _iosmNext(iosm, IOSM_PAD);
#endif
    iosm->blksize /= 20;
#if defined(JBJ_WRITEPAD)
    _IOSMRC(rc);
#endif

    return (int) -rc;
}
