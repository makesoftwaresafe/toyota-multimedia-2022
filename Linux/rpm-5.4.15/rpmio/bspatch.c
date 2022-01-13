/*-
 * Copyright 2003-2005 Colin Percival
 * All rights reserved
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted providing that the following conditions 
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "system.h"

#define	_RPMIOB_INTERNAL	/* XXX rpmiobSlurp */
#include <rpmiotypes.h>
#include <rpmio.h>
#include <poptIO.h>

#include <rpmbz.h>

#include "debug.h"

static rpmbz rpmbzSeekNew(const char * fn, off_t off)
{
    rpmbz bz = NULL;
    FILE * fp = fopen(fn, "r");

    if (fp == NULL)
	fprintf(stderr, "fopen(%s)", fn);
    else {
	if (fseeko(fp, off, SEEK_SET))
	    fprintf(stderr, "fseeko(%s)", fn);
	else
	    bz = rpmbzNew(fn, "r", dup(fileno(fp)));
	(void) fclose(fp);
    }
    return bz;
}

static int64_t offtin(uint8_t * buf)
{
    int64_t y = 0;

    y |= buf[7] & 0x7F;	y <<= 8;
    y |= buf[6];	y <<= 8;
    y |= buf[5];	y <<= 8;
    y |= buf[4];	y <<= 8;
    y |= buf[3];	y <<= 8;
    y |= buf[2];	y <<= 8;
    y |= buf[1];	y <<= 8;
    y |= buf[0];

    if (buf[7] & 0x80)
	y = -y;

    return y;
}

int main(int argc, char *argv[])
{
const char * ofn;
const char * nfn;
const char * pfn;
    rpmiob oldiob = NULL;
    uint8_t * old;
    int64_t oldsize = 0;
    int64_t oldpos = 0;
    rpmiob newiob = NULL;
    uint8_t * new;
    int64_t newsize = 0;
    int64_t newpos = 0;

FILE * fp = NULL;
const char * _errmsg = NULL;
off_t poff;
rpmbz cbz = NULL;
rpmbz dbz = NULL;
rpmbz ebz = NULL;
    
    int64_t bzctrllen;
    int64_t bzdatalen;
    uint8_t buf[8];
    uint8_t header[32];
    int64_t ctrl[3];
    ssize_t nr;
    int64_t i;
    int ec = 1;		/* assume error */
    int xx;

    if (argc != 4)
	errx(1, "usage: %s oldfile newfile patchfile\n", argv[0]);
    ofn = argv[1];
    nfn = argv[2];
    pfn = argv[3];

    /*
       File format:
       0    8       "BSDIFF40"
       8    8       X
       16   8       Y
       24   8       sizeof(newfile)
       32   X       bzip2(control block)
       32+X Y       bzip2(diff block)
       32+X+Y       ???     bzip2(extra block)
       with control block a set of triples (x,y,z) meaning "add x bytes
       from oldfile to x bytes from the diff block; copy y bytes from the
       extra block; seek forwards in oldfile by z bytes".
     */

    /* Open patch file */
    if ((fp = fopen(pfn, "r")) == NULL)
	err(1, "fopen(%s)", pfn);

    /* Read header */
    if (fread(header, 1, sizeof(header), fp) < sizeof(header)) {
	if (feof(fp))
	    errx(1, "Corrupt patch\n");
	err(1, "fread(%s)", pfn);
    }
    (void)fclose(fp);

    /* Check for appropriate magic */
    if (memcmp(header, "BSDIFF40", 8) != 0)
	goto exit;

    /* Read lengths from header */
    bzctrllen = offtin(header + 8);
    bzdatalen = offtin(header + 16);
    newsize = offtin(header + 24);
    if ((bzctrllen < 0) || (bzdatalen < 0) || (newsize < 0))
	goto exit;

    /* Re-open patch file via libbzip2 at the right places */
    poff = sizeof(header);
    if ((cbz = rpmbzSeekNew(pfn, poff)) == NULL)
        goto exit;

    poff += bzctrllen;
    if ((dbz = rpmbzSeekNew(pfn, poff)) == NULL)
        goto exit;

    poff += bzdatalen;
    if ((ebz = rpmbzSeekNew(pfn, poff)) == NULL)
        goto exit;

    /* Read the old file. */
    if ((xx = rpmiobSlurp(ofn, &oldiob)) != 0)
        goto exit;
    old = rpmiobBuf(oldiob);
    oldsize = rpmiobLen(oldiob);

    /* Allocate the new buffer. */
    newiob = rpmiobNew(newsize + 1);
    new = rpmiobBuf(newiob);

    while (newpos < newsize) {
	/* Read control data */
	for (i = 0; i <= 2; i++) {
	    _errmsg = NULL;
	    nr = rpmbzRead(cbz, (char *)buf, sizeof(buf), &_errmsg);
	    if (nr < 8)
		goto exit;
	    ctrl[i] = offtin(buf);
	}

	/* Sanity-check */
	if (newpos + ctrl[0] > newsize)
	    goto exit;

	/* Read diff string */
	_errmsg = NULL;
	nr = rpmbzRead(dbz, (char *)(new + newpos), ctrl[0], &_errmsg);
	if (nr < ctrl[0])
	    goto exit;

	/* Add old data to diff string */
	for (i = 0; i < ctrl[0]; i++)
	    if ((oldpos + i >= 0) && (oldpos + i < oldsize))
		new[newpos + i] += old[oldpos + i];

	/* Adjust pointers */
	newpos += ctrl[0];
	oldpos += ctrl[0];

	/* Sanity-check */
	if (newpos + ctrl[1] > newsize)
	    goto exit;

	/* Read extra string */
	_errmsg = NULL;
	nr = rpmbzRead(ebz, (char *)(new + newpos), ctrl[1], &_errmsg);
	if (nr < ctrl[1])
	    goto exit;

	/* Adjust pointers */
	newpos += ctrl[1];
	oldpos += ctrl[2];
    }

    /* Write the new file */
    {	int fdno = open(nfn, O_CREAT | O_TRUNC | O_WRONLY, 0666);
	if (fdno < 0
	 || write(fdno, new, newsize) != newsize
	 || close(fdno) == -1)
	    err(1, "%s", nfn);
    }

    ec = 0;

exit:
    if (ec == 1)
	fprintf(stderr, "%s\n", (_errmsg ? _errmsg : "Corrupt patch"));

    /* Clean up the bzip2 reads */
    cbz = rpmbzFree(cbz, (ec != 0));
    dbz = rpmbzFree(dbz, (ec != 0));
    ebz = rpmbzFree(ebz, (ec != 0));

    /* Free the memory we used */
    oldiob = rpmiobFree(oldiob);
    newiob = rpmiobFree(newiob);

    return ec;
}
