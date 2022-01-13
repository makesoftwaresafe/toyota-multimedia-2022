/** \ingroup payload
 * \file rpmio/cpio.c
 *  Handle cpio(1) archives.
 */
#undef	JBJ_WRITEPAD

#include "system.h"

#include <rpmio.h>
#include <ugid.h>
#include <cpio.h>
#define	_IOSM_INTERNAL
#include <iosm.h>

#include "debug.h"

/*@access IOSM_t @*/

/*@unchecked@*/
int _cpio_debug = 0;

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
    char * buf;
    char * end;
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

static ssize_t cpioRead(void * _iosm, void * buf, size_t count)
	/*@globals fileSystem @*/
	/*@modifies _iosm, *buf, fileSystem @*/
{
    IOSM_t iosm = (IOSM_t) _iosm;
    char * t = (char *) buf;
    size_t nb = 0;
    size_t rc;

if (_cpio_debug)
fprintf(stderr, "          cpioRead(%p, %p[%u])\n", iosm, buf, (unsigned)count);

    while (count > 0) {

	/* Read next cpio block. */
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

#define GET_NUM_FIELD(phys, log) \
	log = strntoul(phys, &end, 16, sizeof(phys)); \
	if ( (end - phys) != sizeof(phys) ) return IOSMERR_BAD_HEADER;
#define SET_NUM_FIELD(phys, val, space) \
	sprintf(space, "%8.8lx", (unsigned long) (val)); \
	memcpy(phys, space, 8)

int cpioHeaderRead(void * _iosm, struct stat * st)
{
    IOSM_t iosm = (IOSM_t) _iosm;
    cpioHeader hdr = (cpioHeader) iosm->wrbuf;
    size_t nb;
    char * end;
    int major, minor;
    ssize_t rc = 0;

if (_cpio_debug)
fprintf(stderr, "    cpioHeaderRead(%p, %p)\n", iosm, st);

    /* Read next header. */
    rc = cpioRead(iosm, hdr, PHYS_HDR_SIZE);
    _IOSMRC(rc);

    /* Verify header magic. */
    if (strncmp(CPIO_CRC_MAGIC, hdr->magic, sizeof(CPIO_CRC_MAGIC)-1) &&
	strncmp(CPIO_NEWC_MAGIC, hdr->magic, sizeof(CPIO_NEWC_MAGIC)-1))
	return IOSMERR_BAD_MAGIC;

    /* Convert header to stat(2). */
    GET_NUM_FIELD(hdr->inode, st->st_ino);
    GET_NUM_FIELD(hdr->mode, st->st_mode);
    GET_NUM_FIELD(hdr->uid, st->st_uid);
    GET_NUM_FIELD(hdr->gid, st->st_gid);
    GET_NUM_FIELD(hdr->nlink, st->st_nlink);
    GET_NUM_FIELD(hdr->mtime, st->st_mtime);
    GET_NUM_FIELD(hdr->filesize, st->st_size);

    GET_NUM_FIELD(hdr->devMajor, major);
    GET_NUM_FIELD(hdr->devMinor, minor);
    /*@-shiftimplementation@*/
    st->st_dev = Makedev(major, minor);
    /*@=shiftimplementation@*/

    GET_NUM_FIELD(hdr->rdevMajor, major);
    GET_NUM_FIELD(hdr->rdevMinor, minor);
    /*@-shiftimplementation@*/
    st->st_rdev = Makedev(major, minor);
    /*@=shiftimplementation@*/

    GET_NUM_FIELD(hdr->namesize, nb);
    if (nb >= iosm->wrsize)
	return IOSMERR_BAD_HEADER;

    /* Read file name. */
    {	char * t = (char *) xmalloc(nb + 1);
	rc = cpioRead(iosm, t, nb);
	if (rc < 0) {
	    t = _free(t);
	    iosm->path = NULL;
	} else {
	    _IOSMRC(rc);
	    t[nb] = '\0';
	    iosm->path = t;
	}
    }

    /* Read link name. */
    if (S_ISLNK(st->st_mode)) {
	char * t;

	/* Make sure block aligned. */
	rc = _iosmNext(iosm, IOSM_POS);
	if (rc) return (int) -rc;

	nb = (size_t) st->st_size;
	t = (char *) xmalloc(nb + 1);
	rc = cpioRead(iosm, t, nb);
	if (rc < 0) {
	    t = _free(t);
	    iosm->lpath = NULL;
	} else {
	    _IOSMRC(rc);
	    t[nb] = '\0';
	    iosm->lpath = t;
	}
    }

    rc = 0;

/*@-usereleased@*/
if (_cpio_debug)
fprintf(stderr, "\t     %06o%3d (%4d,%4d)%12lu %s\n\t-> %s\n",
                (unsigned)st->st_mode, (int)st->st_nlink,
                (int)st->st_uid, (int)st->st_gid, (unsigned long)st->st_size,
                (iosm->path ? iosm->path : ""), (iosm->lpath ? iosm->lpath : ""));
/*@=usereleased@*/

    return (int) rc;
}

static ssize_t cpioWrite(void * _iosm, const void *buf, size_t count)
	/*@globals fileSystem @*/
	/*@modifies _iosm, fileSystem @*/
{
    IOSM_t iosm = (IOSM_t) _iosm;
    const char * s = (const char *) buf;
    size_t nb = 0;
    size_t rc;

if (_cpio_debug)
fprintf(stderr, "\t  cpioWrite(%p, %p[%u])\n", iosm, buf, (unsigned)count);

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
    if ((rc = _iosmNext(iosm, IOSM_PAD)) != 0) return -rc;
#endif

    return nb;
}

int cpioHeaderWrite(void * _iosm, struct stat * st)
{
    IOSM_t iosm = (IOSM_t) _iosm;
    cpioHeader hdr = (cpioHeader) iosm->rdbuf;
    const char * path = (iosm && iosm->path ? iosm->path : "");
    const char * lpath = (iosm && iosm->lpath ? iosm->lpath : "");
    char field[64];
    size_t nb;
    dev_t dev;
    ssize_t rc = 0;

if (_cpio_debug)
fprintf(stderr, "    cpioHeaderWrite(%p, %p)\n", iosm, st);

    memcpy(hdr->magic, CPIO_NEWC_MAGIC, sizeof(hdr->magic));
    SET_NUM_FIELD(hdr->inode, st->st_ino, field);
    SET_NUM_FIELD(hdr->mode, st->st_mode, field);
    SET_NUM_FIELD(hdr->uid, st->st_uid, field);
    SET_NUM_FIELD(hdr->gid, st->st_gid, field);
    SET_NUM_FIELD(hdr->nlink, st->st_nlink, field);
    SET_NUM_FIELD(hdr->mtime, st->st_mtime, field);
    SET_NUM_FIELD(hdr->filesize, st->st_size, field);

    dev = major((unsigned)st->st_dev); SET_NUM_FIELD(hdr->devMajor, dev, field);
    dev = minor((unsigned)st->st_dev); SET_NUM_FIELD(hdr->devMinor, dev, field);
    dev = major((unsigned)st->st_rdev); SET_NUM_FIELD(hdr->rdevMajor, dev, field);
    dev = minor((unsigned)st->st_rdev); SET_NUM_FIELD(hdr->rdevMinor, dev, field);

    nb = strlen(path) + 1; SET_NUM_FIELD(hdr->namesize, nb, field);
    memcpy(hdr->checksum, "00000000", 8);

    /* XXX Coalesce hdr+name into single I/O. */
    memcpy(iosm->rdbuf + PHYS_HDR_SIZE, path, nb);
    nb += PHYS_HDR_SIZE;
    rc = cpioWrite(iosm, hdr, nb);
    _IOSMRC(rc);

    if (S_ISLNK(st->st_mode)) {
#if !defined(JBJ_WRITEPAD)
	rc = _iosmNext(iosm, IOSM_PAD);
	if (rc) return (int) rc;
#endif

	nb = strlen(lpath);
	rc = cpioWrite(iosm, lpath, nb);
	_IOSMRC(rc);
    }

#if !defined(JBJ_WRITEPAD)
    rc = _iosmNext(iosm, IOSM_PAD);
#endif

    return (int) rc;
}

int cpioTrailerWrite(void * _iosm)
{
    IOSM_t iosm = (IOSM_t) _iosm;
    cpioHeader hdr = (cpioHeader) iosm->rdbuf;
    size_t nb;
    ssize_t rc = 0;

if (_cpio_debug)
fprintf(stderr, "   cpioTrailerWrite(%p)\n", iosm);

    memset(hdr, (int)'0', PHYS_HDR_SIZE);
    memcpy(hdr->magic, CPIO_NEWC_MAGIC, sizeof(hdr->magic));
    memcpy(hdr->nlink, "00000001", 8);
    memcpy(hdr->namesize, "0000000b", 8);

    nb = sizeof(CPIO_TRAILER);
    /* XXX Coalesce hdr+trailer into single I/O. */
    memcpy(iosm->rdbuf + PHYS_HDR_SIZE, CPIO_TRAILER, nb);
    nb += PHYS_HDR_SIZE;

    rc = cpioWrite(iosm, hdr, nb);
    _IOSMRC(rc);

    /*
     * GNU cpio pads to 512 bytes here, but we don't. This may matter for
     * tape device(s) and/or concatenated cpio archives. <shrug>
     */
#if !defined(JBJ_WRITEPAD)
    rc = _iosmNext(iosm, IOSM_PAD);
#endif
    rc = 0;

    return (int) rc;
}

