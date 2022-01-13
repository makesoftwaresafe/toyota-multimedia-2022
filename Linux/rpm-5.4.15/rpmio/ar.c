/** \ingroup payload
 * \file rpmio/ar.c
 *  Handle ar(1) archives.
 */

#undef	JBJ_WRITEPAD

#include "system.h"

#include <rpmio_internal.h>	/* XXX fdGetCpioPos writing AR_MAGIC */

#include <rpmmacro.h>
#include <ugid.h>
#include <ar.h>
#define	_IOSM_INTERNAL
#include <iosm.h>

#include "debug.h"

/*@access IOSM_t @*/

/*@unchecked@*/
int _ar_debug = 0;

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

static ssize_t arRead(void * _iosm, void * buf, size_t count)
	/*@globals fileSystem @*/
	/*@modifies _iosm, *buf, fileSystem @*/
{
    IOSM_t iosm = (IOSM_t) _iosm;
    char * t = (char *) buf;
    size_t nb = 0;
    size_t rc;

if (_ar_debug)
fprintf(stderr, "\t  arRead(%p, %p[%u])\n", iosm, buf, (unsigned)count);

    while (count > 0) {

	/* Read next ar block. */
	iosm->wrlen = count;
	rc = _iosmNext(iosm, IOSM_DREAD);
	if (!rc && iosm->rdnb != iosm->wrlen) {
	    if (iosm->rdnb == 0) return -IOSMERR_HDR_TRAILER;	/* EOF */
	    rc = IOSMERR_READ_FAILED;
	}
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

int arHeaderRead(void * _iosm, struct stat * st)
	/*@modifies _iosm, *st @*/
{
    IOSM_t iosm = (IOSM_t) _iosm;
    arHeader hdr = (arHeader) iosm->wrbuf;
    ssize_t rc = 0;

if (_ar_debug)
fprintf(stderr, "    arHeaderRead(%p, %p)\n", iosm, st);

    /* XXX Read AR_MAGIC to beginning of ar(1) archive. */
    if (fdGetCpioPos(iosm->cfd) == 0) {
	rc = arRead(iosm, iosm->wrbuf, sizeof(AR_MAGIC)-1);
	_IOSMRC(rc);

	/* Verify archive magic. */
	if (strncmp(iosm->wrbuf, AR_MAGIC, sizeof(AR_MAGIC)-1))
	    return IOSMERR_BAD_MAGIC;
    }

top:
    /* Make sure block aligned. */
    rc = _iosmNext(iosm, IOSM_POS);
    if (rc) return (int) rc;

    rc = arRead(iosm, hdr, sizeof(*hdr));
    _IOSMRC(rc);
if (_ar_debug)
fprintf(stderr, "==> %p[%u] \"%.*s\"\n", hdr, (unsigned)rc, (int)sizeof(*hdr)-2, (char *)hdr);

    /* Verify header marker. */
    if (strncmp(hdr->marker, AR_MARKER, sizeof(AR_MARKER)-1))
	return IOSMERR_BAD_MAGIC;

    st->st_size = strntoul(hdr->filesize, NULL, 10, sizeof(hdr->filesize));

    /* Special ar(1) archive members. */
    if (hdr->name[0] == '/') {
	/* GNU: on "//":	Read long member name string table. */
	if (hdr->name[1] == '/' && hdr->name[2] == ' ') {
	    char * t;
	    size_t i;

	    rc = arRead(iosm, iosm->wrbuf, st->st_size);
	    _IOSMRC(rc);

	    iosm->wrbuf[rc] = '\0';
	    iosm->lmtab = t = xstrdup(iosm->wrbuf);
	    iosm->lmtablen = rc;
	    iosm->lmtaboff = 0;

	    for (i = 1; i < iosm->lmtablen; i++) {
		t++;
		if (t[0] != '\n') continue;
		t[0] = '\0';
		/* GNU: trailing '/' to permit file names with trailing ' '. */
		if (t[-1] == '/') t[-1] = '\0';
	    }
	    goto top;
	}
	/* GNU: on "/":	Skip symbol table. */
	if (hdr->name[1] == ' ') {
	    rc = arRead(iosm, iosm->wrbuf, st->st_size);
	    _IOSMRC(rc);
	    goto top;
	}
	/* GNU: on "/123": Read "123" offset to substitute long member name. */
	if (xisdigit((int)hdr->name[1])) {
	    char * te = NULL;
	    int i = strntoul(&hdr->name[1], &te, 10, sizeof(hdr->name)-2);
	    if (*te == ' ' && iosm->lmtab != NULL && i < (int)iosm->lmtablen)
		iosm->path = xstrdup(iosm->lmtab + i);
	}
    } else
    if (hdr->name[0] != ' ') {	/* Short member name. */
	size_t nb = sizeof(hdr->name);
	char t[sizeof(hdr->name)+1];
	memcpy(t, hdr->name, nb);
	t[nb] = '\0';
	while (nb > 0 && t[nb-1] == ' ')
	    t[--nb] = '\0';
	/* GNU: trailing '/' to permit file names with trailing ' '. */
	if (nb > 0 && t[nb - 1] == '/')
	    t[--nb] = '\0';
	iosm->path = xstrdup(t);
    }

    st->st_mtime = strntoul(hdr->mtime, NULL, 10, sizeof(hdr->mtime));
    st->st_ctime = st->st_atime = st->st_mtime;

    st->st_uid = strntoul(hdr->uid, NULL, 10, sizeof(hdr->uid));
    st->st_gid = strntoul(hdr->gid, NULL, 10, sizeof(hdr->gid));

    st->st_mode = strntoul(hdr->mode, NULL, 8, sizeof(hdr->mode));

    st->st_nlink = 1;
    rc = 0;

if (_ar_debug)
fprintf(stderr, "\t     %06o%3d (%4d,%4d)%12lu %s\n",
                (unsigned)st->st_mode, (int)st->st_nlink,
                (int)st->st_uid, (int)st->st_gid, (unsigned long)st->st_size,
                (iosm->path ? iosm->path : ""));

    return (int) rc;
}

static ssize_t arWrite(void * _iosm, const void *buf, size_t count)
	/*@globals fileSystem @*/
	/*@modifies _iosm, fileSystem @*/
{
    IOSM_t iosm =(IOSM_t) _iosm;
    const char * s = (const char *) buf;
    size_t nb = 0;
    size_t rc;

if (_ar_debug)
fprintf(stderr, "\tarWrite(%p, %p[%u])\n", iosm, buf, (unsigned)count);

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

int arHeaderWrite(void * _iosm, struct stat * st)
{
    IOSM_t iosm = (IOSM_t) _iosm;
    arHeader hdr = (arHeader) iosm->rdbuf;
    const char * path = (iosm && iosm->path ? iosm->path : "");
    size_t nb;
    ssize_t rc = 0;

if (_ar_debug)
fprintf(stderr, "    arHeaderWrite(%p, %p)\n", iosm, st);

    /* At beginning of ar(1) archive, write magic and long member table. */
    if (fdGetCpioPos(iosm->cfd) == 0) {
	/* Write ar(1) magic. */
	rc = arWrite(iosm, AR_MAGIC, sizeof(AR_MAGIC)-1);
	_IOSMRC(rc);
	/* GNU: on "//":	Write long member name string table. */
	if (iosm->lmtab != NULL) {
	    memset(hdr, (int) ' ', sizeof(*hdr));
	    hdr->name[0] = '/';
	    hdr->name[1] = '/';
	    sprintf(hdr->filesize, "%-10d", (unsigned) (iosm->lmtablen & 037777777777));
	    strncpy(hdr->marker, AR_MARKER, sizeof(AR_MARKER)-1);

	    rc = arWrite(iosm, hdr, sizeof(*hdr));
	    _IOSMRC(rc);
	    rc = arWrite(iosm, iosm->lmtab, iosm->lmtablen);
	    _IOSMRC(rc);
#if !defined(JBJ_WRITEPAD)
	    rc = _iosmNext(iosm, IOSM_PAD);
	    if (rc) return (int)rc;
#endif
	}
    }

    memset(hdr, (int)' ', sizeof(*hdr));

    nb = strlen(path);
    if (nb >= sizeof(hdr->name)) {
	const char * t;
	const char * te;
assert(iosm->lmtab != NULL);
	t = iosm->lmtab + iosm->lmtaboff;
	te = strchr(t, '\n');
	/* GNU: on "/123": Write "/123" offset for long member name. */
	nb = snprintf(hdr->name, sizeof(hdr->name)-1, "/%u", (unsigned)iosm->lmtaboff);
	hdr->name[nb] = ' ';
	if (te != NULL)
	    iosm->lmtaboff += (te - t) + 1;
    } else {
	strncpy(hdr->name, path, nb);
	hdr->name[nb] = '/';
    }

    sprintf(hdr->mtime, "%-12u", (unsigned) (st->st_mtime & 037777777777));
    sprintf(hdr->uid, "%-6u", (unsigned int)(st->st_uid & 07777777));
    sprintf(hdr->gid, "%-6u", (unsigned int)(st->st_gid & 07777777));

    sprintf(hdr->mode, "%-8o", (unsigned int)(st->st_mode & 07777777));
    sprintf(hdr->filesize, "%-10u", (unsigned) (st->st_size & 037777777777));

    strncpy(hdr->marker, AR_MARKER, sizeof(AR_MARKER)-1);

rc = (int) sizeof(*hdr);
if (_ar_debug)
fprintf(stderr, "==> %p[%u] \"%.*s\"\n", hdr, (unsigned)rc, (int)sizeof(*hdr), (char *)hdr);

    rc = arWrite(iosm, hdr, sizeof(*hdr));
    _IOSMRC(rc);
    rc = 0;

    return (int) rc;
}

int arTrailerWrite(void * _iosm)
{
    IOSM_t iosm = (IOSM_t) _iosm;
    size_t rc = 0;

if (_ar_debug)
fprintf(stderr, "    arTrailerWrite(%p)\n", iosm);

#if defined(JBJ_WRITEPAD)
    rc = arWrite(iosm, NULL, 0);	/* XXX _iosmNext(iosm, IOSM_PAD) */
    _IOSMRC(rc);
#else
    rc = _iosmNext(iosm, IOSM_PAD);	/* XXX likely unnecessary. */
#endif

    return (int) rc;
}
