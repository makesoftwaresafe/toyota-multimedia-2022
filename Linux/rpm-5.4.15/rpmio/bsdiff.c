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

static void split(int64_t * I, int64_t * V, int64_t start, int64_t len, int64_t h)
{
    int64_t i, j, k, x, tmp, jj, kk;

    if (len < 16) {
	for (k = start; k < start + len; k += j) {
	    j = 1;
	    x = V[I[k] + h];
	    for (i = 1; k + i < start + len; i++) {
		if (V[I[k + i] + h] < x) {
		    x = V[I[k + i] + h];
		    j = 0;
		}
		if (V[I[k + i] + h] == x) {
		    tmp = I[k + j];
		    I[k + j] = I[k + i];
		    I[k + i] = tmp;
		    j++;
		}
	    }
	    for (i = 0; i < j; i++)
		V[I[k + i]] = k + j - 1;
	    if (j == 1)
		I[k] = -1;
	}
	return;
    }

    x = V[I[start + len / 2] + h];
    jj = 0;
    kk = 0;
    for (i = start; i < start + len; i++) {
	if (V[I[i] + h] < x)
	    jj++;
	if (V[I[i] + h] == x)
	    kk++;
    }
    jj += start;
    kk += jj;

    i = start;
    j = 0;
    k = 0;
    while (i < jj) {
	if (V[I[i] + h] < x) {
	    i++;
	} else if (V[I[i] + h] == x) {
	    tmp = I[i];
	    I[i] = I[jj + j];
	    I[jj + j] = tmp;
	    j++;
	} else {
	    tmp = I[i];
	    I[i] = I[kk + k];
	    I[kk + k] = tmp;
	    k++;
	}
    }

    while (jj + j < kk) {
	if (V[I[jj + j] + h] == x) {
	    j++;
	} else {
	    tmp = I[jj + j];
	    I[jj + j] = I[kk + k];
	    I[kk + k] = tmp;
	    k++;
	}
    }

    if (jj > start)
	split(I, V, start, jj - start, h);

    for (i = 0; i < kk - jj; i++)
	V[I[jj + i]] = kk - 1;
    if (jj == kk - 1)
	I[jj] = -1;

    if (start + len > kk)
	split(I, V, kk, start + len - kk, h);
}

static void qsufsort(int64_t * I, int64_t * V, uint8_t * old, int64_t oldsize)
{
    int64_t buckets[256];
    int64_t i, h, len;

    for (i = 0; i < 256; i++)
	buckets[i] = 0;
    for (i = 0; i < oldsize; i++)
	buckets[old[i]]++;
    for (i = 1; i < 256; i++)
	buckets[i] += buckets[i - 1];
    for (i = 255; i > 0; i--)
	buckets[i] = buckets[i - 1];
    buckets[0] = 0;

    for (i = 0; i < oldsize; i++)
	I[++buckets[old[i]]] = i;
    I[0] = oldsize;
    for (i = 0; i < oldsize; i++)
	V[i] = buckets[old[i]];
    V[oldsize] = 0;
    for (i = 1; i < 256; i++)
	if (buckets[i] == buckets[i - 1] + 1)
	    I[buckets[i]] = -1;
    I[0] = -1;

    for (h = 1; I[0] != -(oldsize + 1); h += h) {
	len = 0;
	for (i = 0; i < oldsize + 1;) {
	    if (I[i] < 0) {
		len -= I[i];
		i -= I[i];
	    } else {
		if (len)
		    I[i - len] = -len;
		len = V[I[i]] + 1 - i;
		split(I, V, i, len, h);
		i += len;
		len = 0;
	    }
	}
	if (len)
	    I[i - len] = -len;
    }
    for (i = 0; i < oldsize + 1; i++)
	I[V[i]] = i;
}

static int64_t matchlen(uint8_t * old, int64_t oldsize, uint8_t * new,
		      int64_t newsize)
{
    int64_t i;

    for (i = 0; (i < oldsize) && (i < newsize); i++)
	if (old[i] != new[i])
	    break;

    return i;
}

static int64_t search(int64_t * I, uint8_t * old, int64_t oldsize,
		    uint8_t * new, int64_t newsize, int64_t st, int64_t en,
		    int64_t * pos)
{
    int64_t x, y;

    if (en - st < 2) {
	x = matchlen(old + I[st], oldsize - I[st], new, newsize);
	y = matchlen(old + I[en], oldsize - I[en], new, newsize);

	if (x > y) {
	    *pos = I[st];
	    return x;
	} else {
	    *pos = I[en];
	    return y;
	}
    }

    x = st + (en - st) / 2;
    if (memcmp(old + I[x], new, MIN(oldsize - I[x], newsize)) < 0)
	return search(I, old, oldsize, new, newsize, x, en, pos);
    else
	return search(I, old, oldsize, new, newsize, st, x, pos);
}

static void offtout(int64_t x, uint8_t * buf)
{
    int64_t y = (x < 0 ? -x : x);

    buf[0] = y & 0xff;	y >>= 8;
    buf[1] = y & 0xff;	y >>= 8;
    buf[2] = y & 0xff;	y >>= 8;
    buf[3] = y & 0xff;	y >>= 8;
    buf[4] = y & 0xff;	y >>= 8;
    buf[5] = y & 0xff;	y >>= 8;
    buf[6] = y & 0xff;	y >>= 8;
    buf[7] = y & 0xff;

    if (x < 0)
	buf[7] |= 0x80;
}

int main(int argc, char *argv[])
{
const char * ofn;
const char * nfn;
const char * pfn;
    rpmiob oldiob = NULL;
    uint8_t * old;
    int64_t oldsize = 0;
    rpmiob newiob = NULL;
    uint8_t * new;
    int64_t newsize = 0;

    int64_t * I = NULL;
    int64_t scan = 0;
    int64_t pos = 0;
    int64_t len = 0;
    int64_t lastscan = 0;
    int64_t lastpos = 0;
    int64_t lastoffset = 0;

    int64_t lenb;
    uint8_t * db = NULL;
    int64_t dblen = 0;
    uint8_t * eb = NULL;
    int64_t eblen = 0;
    uint8_t buf[8];
    uint8_t header[32];

FILE * fp = NULL;
const char * _errmsg = NULL;
rpmbz bz = NULL;

    int ec = 1;		/* assume error */
    int xx;

    if (argc != 4)
	errx(1, "usage: %s oldfile newfile patchfile\n", argv[0]);
    ofn = argv[1];
    nfn = argv[2];
    pfn = argv[3];

    /* Read the old file. */
    if ((xx = rpmiobSlurp(ofn, &oldiob)) != 0)
	goto exit;
    old = rpmiobBuf(oldiob);
    oldsize = rpmiobLen(oldiob);

    /* Read the new file. */
    if ((xx = rpmiobSlurp(nfn, &newiob)) != 0)
	goto exit;
    new = rpmiobBuf(newiob);
    newsize = rpmiobLen(newiob);

    {	int64_t * V = xmalloc((oldsize + 1) * sizeof(*V));
	I = xmalloc((oldsize + 1) * sizeof(*I));
	qsufsort(I, V, old, oldsize);
	V = _free(V);
    }

    db = xmalloc(newsize + 1);
    eb = xmalloc(newsize + 1);

    /* Create the patch file */
    fp = fopen(pfn, "w");
    if (fp == NULL) {
	fprintf(stderr, "fopen(%s)\n", pfn);
	goto exit;
    }

    /* Header is
       0    8        "BSDIFF40"
       8    8       length of bzip2ed ctrl block
       16   8       length of bzip2ed diff block
       24   8       length of new file */
    /* File is
       0    32      Header
       32   ??      Bzip2ed ctrl block
       ??   ??      Bzip2ed diff block
       ??   ??      Bzip2ed extra block */
    memcpy(header, "BSDIFF40", 8);
    offtout(0, header + 8);
    offtout(0, header + 16);
    offtout(newsize, header + 24);
    if (fwrite(header, 1, sizeof(header), fp) != sizeof(header)) {
	fprintf(stderr, "fwrite(%s)\n", pfn);
	goto exit;
    }

    /* Compute the differences, writing ctrl as we go */
    (void) fflush(fp);
    bz = rpmbzNew(pfn, "w", dup(fileno(fp)));
    if (bz == NULL) {
	fprintf(stderr, "rpmbzNew: %s\n", pfn);
	goto exit;
    }

    while (scan < newsize) {
	int64_t oldscore;
	int64_t scsc;
	int64_t i;

	oldscore = 0;

	for (scsc = scan += len; scan < newsize; scan++) {
	    len = search(I, old, oldsize, new + scan, newsize - scan,
			 0, oldsize, &pos);

	    for (; scsc < scan + len; scsc++)
		if (scsc + lastoffset < oldsize
		 && old[scsc + lastoffset] == new[scsc])
		    oldscore++;

	    if ((len == oldscore && len != 0) || len > oldscore + 8)
		break;

	    if (scan + lastoffset < oldsize
	     &&	old[scan + lastoffset] == new[scan])
		oldscore--;
	}

	if (len != oldscore || scan == newsize) {
	    int64_t s = 0;
	    int64_t Sf = 0;
	    int64_t lenf = 0;
	    for (i = 0; lastscan + i < scan && lastpos + i < oldsize;) {
		if (old[lastpos + i] == new[lastscan + i])
		    s++;
		i++;
		if (s * 2 - i > Sf * 2 - lenf) {
		    Sf = s;
		    lenf = i;
		}
	    }

	    lenb = 0;
	    if (scan < newsize) {
		int64_t s = 0;
		int64_t Sb = 0;
		for (i = 1; (scan >= lastscan + i) && (pos >= i); i++) {
		    if (old[pos - i] == new[scan - i])
			s++;
		    if (s * 2 - i > Sb * 2 - lenb) {
			Sb = s;
			lenb = i;
		    }
		}
	    }

	    if (lastscan + lenf > scan - lenb) {
		int64_t overlap = (lastscan + lenf) - (scan - lenb);
		int64_t s = 0;
		int64_t Ss = 0;
		int64_t lens = 0;

		for (i = 0; i < overlap; i++) {
		    if (new[lastscan + lenf - overlap + i] ==
			old[lastpos + lenf - overlap + i])
			s++;
		    if (new[scan - lenb + i] == old[pos - lenb + i])
			s--;
		    if (s > Ss) {
			Ss = s;
			lens = i + 1;
		    }
		}

		lenf += lens - overlap;
		lenb -= lens;
	    }

	    for (i = 0; i < lenf; i++)
		db[dblen + i] = new[lastscan + i] - old[lastpos + i];
	    for (i = 0; i < (scan - lenb) - (lastscan + lenf); i++)
		eb[eblen + i] = new[lastscan + lenf + i];

	    dblen += lenf;
	    eblen += (scan - lenb) - (lastscan + lenf);

	    offtout(lenf, buf);
	    if (rpmbzWrite(bz, (const char *)buf, 8, &_errmsg) != 8) {
		fprintf(stderr, "rpmbzWrite: %s\n", _errmsg);
		goto exit;
	    }

	    offtout((scan - lenb) - (lastscan + lenf), buf);
	    if (rpmbzWrite(bz, (const char *)buf, 8, &_errmsg) != 8) {
		fprintf(stderr, "rpmbzWrite: %s\n", _errmsg);
		goto exit;
	    }

	    offtout((pos - lenb) - (lastpos + lenf), buf);
	    if (rpmbzWrite(bz, (const char *)buf, 8, &_errmsg) != 8) {
		fprintf(stderr, "rpmbzWrite: %s\n", _errmsg);
		goto exit;
	    }

	    lastscan = scan - lenb;
	    lastpos = pos - lenb;
	    lastoffset = pos - scan;
	}
    }
    bz = rpmbzFree(bz, 0);

    /* Compute size of compressed ctrl data */
    if ((len = ftello(fp)) == -1) {
	fprintf(stderr, "ftello(%s)\n", pfn);
	goto exit;
    }
    offtout(len - 32, header + 8);

    /* Write compressed diff data */
    (void) fflush(fp);
    bz = rpmbzNew(pfn, "w", dup(fileno(fp)));
    if (bz == NULL) {
	fprintf(stderr, "rpmbzNew: %s\n", pfn);
	goto exit;
    }

    if (rpmbzWrite(bz, (const char *)db, dblen, &_errmsg) != dblen) {
	fprintf(stderr, "rpmbzWrite: %s\n", _errmsg);
	goto exit;
    }
    bz = rpmbzFree(bz, 0);

    /* Compute size of compressed diff data */
    newsize = ftello(fp);
    if (newsize == -1) {
	fprintf(stderr, "ftello(%s)\n", pfn);
	goto exit;
    }
    offtout(newsize - len, header + 16);

    /* Write compressed extra data */
    (void) fflush(fp);
    bz = rpmbzNew(pfn, "w", dup(fileno(fp)));
    if (bz == NULL) {
	fprintf(stderr, "rpmbzNew: %s\n", pfn);
	goto exit;
    }

    if (rpmbzWrite(bz, (const char *)eb, eblen, &_errmsg) != eblen) {
	fprintf(stderr, "rpmbzWrite: %s\n", _errmsg);
	goto exit;
    }
    bz = rpmbzFree(bz, 0);

    /* Seek to the beginning, write the header, and close the file */
    if (fseeko(fp, 0, SEEK_SET) != 0) {
	fprintf(stderr, "fseeko(%s)\n", pfn);
	goto exit;
    }
    if (fwrite(header, 1, sizeof(header), fp) != sizeof(header)) {
	fprintf(stderr, "fwrite(%s)\n", pfn);
	goto exit;
    }
    (void) fclose(fp);
    fp = NULL;
    ec = 0;

exit:
    /* Free the memory we used */
    db = _free(db);
    eb = _free(eb);
    I = _free(I);
    oldiob = rpmiobFree(oldiob);
    newiob = rpmiobFree(newiob);

    if (bz)
	bz = rpmbzFree(bz, (ec != 0));
    if (fp)
	(void) fclose(fp);

    return ec;
}
