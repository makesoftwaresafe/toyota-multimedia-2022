#include "system.h"

#define _RPMHKP_INTERNAL
#include <rpmhkp.h>

#define	_RPMPGP_INTERNAL
#include <rpmpgp.h>

#include <rpmlog.h>
#include <rpmmacro.h>

#include "debug.h"

#ifdef __cplusplus
GENfree(rpmuint8_t **)
#endif	/* __cplusplus */

/*@unchecked@*/
int _rpmhkp_debug;

/*@unchecked@*/ /*@relnull@*/
rpmhkp _rpmhkpI = NULL;

struct _filter_s _rpmhkp_awol	= {};
struct _filter_s _rpmhkp_crl	= {};

typedef struct _Astats_s {
    size_t good;
    size_t bad;
} _Astats;

typedef struct _BAstats_s {
    _Astats RSA;
    _Astats DSA;
    _Astats ECDSA;
    _Astats HASH;
    _Astats AWOL;
    _Astats SKIP;
    size_t lookups;
    size_t certs;
    size_t sigs;
    size_t expired;
    size_t pubbound;
    size_t subbound;
    size_t pubrevoked;
    size_t subrevoked;
    size_t filtered;
    size_t keyexpired;
} _BAstats;

_BAstats _rpmhkp_stats;

/* XXX renaming work-in-progress */
#define	SUM	_rpmhkp_stats

int _rpmhkp_spew;
#define	SPEW(_list)	if (_rpmhkp_spew) fprintf _list
#if 0
#define	DESPEW(_list)	           fprintf _list
#else
#define	DESPEW(_list)	SPEW(_list)
#endif
#define HKPDEBUG(_list)   if (_rpmhkp_debug) fprintf _list

int _rpmhkp_lvl = RPMLOG_DEBUG;

/*==============================================================*/

static rpmhkp rpmhkpI(void)
	/*@globals _rpmhkpI @*/
	/*@modifies _rpmhkpI @*/
{
    if (_rpmhkpI == NULL)
	_rpmhkpI = rpmhkpNew(NULL, 0);
    return _rpmhkpI;
}

static void rpmhkpFini(void * _hkp)
        /*@globals fileSystem @*/
        /*@modifies *_hkp, fileSystem @*/
{
    rpmhkp hkp = (rpmhkp) _hkp;

assert(hkp);
    hkp->pkt = _free(hkp->pkt);
    hkp->pktlen = 0;
    hkp->pkts = _free(hkp->pkts);
    hkp->npkts = 0;
    hkp->awol = rpmbfFree(hkp->awol);
    hkp->crl = rpmbfFree(hkp->crl);
}

/*@unchecked@*/ /*@only@*/ /*@null@*/
rpmioPool _rpmhkpPool;

static rpmhkp rpmhkpGetPool(/*@null@*/ rpmioPool pool)
        /*@globals _rpmhkpPool, fileSystem @*/
        /*@modifies pool, _rpmhkpPool, fileSystem @*/
{
    rpmhkp hkp;

    if (_rpmhkpPool == NULL) {
        _rpmhkpPool = rpmioNewPool("hkp", sizeof(*hkp), -1, _rpmhkp_debug,
                        NULL, NULL, rpmhkpFini);
        pool = _rpmhkpPool;
    }
    hkp = (rpmhkp) rpmioGetPool(pool, sizeof(*hkp));
    memset(((char *)hkp)+sizeof(hkp->_item), 0, sizeof(*hkp)-sizeof(hkp->_item));
    return hkp;
}

rpmhkp rpmhkpNew(const rpmuint8_t * keyid, uint32_t flags)
{
    static int oneshot;
    rpmhkp hkp;

    if (!oneshot) {
	_rpmhkp_awol.n = 100000;
	_rpmhkp_awol.e = 1.0e-4;
	rpmbfParams(_rpmhkp_awol.n, _rpmhkp_awol.e,
		&_rpmhkp_awol.m, &_rpmhkp_awol.k);
	_rpmhkp_awol.bf = rpmbfNew(_rpmhkp_awol.m, _rpmhkp_awol.k, 0);
	_rpmhkp_crl.n = 100000;
	_rpmhkp_crl.e = 1.0e-4;
	rpmbfParams(_rpmhkp_crl.n, _rpmhkp_crl.e,
		&_rpmhkp_crl.m, &_rpmhkp_crl.k);
	_rpmhkp_crl.bf = rpmbfNew(_rpmhkp_crl.m, _rpmhkp_crl.k, 0);
	oneshot++;
    }

    /* XXX watchout for recursive call. */
    hkp = (flags & 0x80000000) ? rpmhkpI() : rpmhkpGetPool(_rpmhkpPool);

hkp->pkt = NULL;
hkp->pktlen = 0;
hkp->pkts = NULL;
hkp->npkts = 0;

hkp->pubx = -1;
hkp->uidx = -1;
hkp->subx = -1;
hkp->sigx = -1;

    if (keyid)
	memcpy(hkp->keyid, keyid, sizeof(hkp->keyid));
    else
	memset(hkp->keyid, 0, sizeof(hkp->keyid));
    memset(hkp->subid, 0, sizeof(hkp->subid));
    memset(hkp->signid, 0, sizeof(hkp->signid));

hkp->tvalid = 0;
hkp->uvalidx = -1;

    /* XXX watchout for recursive call. */
    if (_rpmhkp_awol.bf && hkp->awol == NULL)
	hkp->awol = rpmbfLink(_rpmhkp_awol.bf);
    if (_rpmhkp_crl.bf && hkp->crl == NULL)
	hkp->crl = rpmbfLink(_rpmhkp_crl.bf);

    return rpmhkpLink(hkp);
}

/*==============================================================*/

#ifdef	DYING
struct pgpPkt_s {
    pgpTag tag;
    unsigned int pktlen;
    union {
	const rpmuint8_t * h;
	const pgpPktKeyV3 j;
	const pgpPktKeyV4 k;
	const pgpPktSigV3 r;
	const pgpPktSigV4 s;
	const pgpPktUid * u;
    } u;
    unsigned int hlen;
};
#endif

static const rpmuint8_t * pgpGrabSubTagVal(const rpmuint8_t * h, size_t hlen,
		rpmuint8_t subtag, /*@null@*/ size_t * tlenp)
{
    const rpmuint8_t * p = h;
    const rpmuint8_t * pend = h + hlen;
    unsigned plen = 0;
    unsigned len;
    rpmuint8_t stag;

    if (tlenp)
	*tlenp = 0;

    while (p < pend) {
	len = pgpLen(p, &plen);
	p += len;

	stag = (*p & ~PGPSUBTYPE_CRITICAL);

	if (stag == subtag) {
SPEW((stderr, "\tSUBTAG %02X %p[%2u]\t%s\n", stag, p+1, plen-1, pgpHexStr(p+1, plen-1)));
	    if (tlenp)
		*tlenp = plen-1;
	    return p+1;
	}
	p += plen;
    }
    return NULL;
}

static const rpmuint8_t * ppSigHash(pgpPkt pp, size_t * plen)
{
    const rpmuint8_t * p = NULL;

assert(pp->tag == PGPTAG_SIGNATURE);
    switch (pp->u.h[0]) {
    case 4:
	*plen = pgpGrab(pp->u.s->hashlen, sizeof(pp->u.s->hashlen));
	p = pp->u.h + sizeof(*pp->u.s);
	break;
    }
    return p;
}

static const rpmuint8_t * ppSigUnhash(pgpPkt pp, size_t * plen)
{
    const rpmuint8_t * p = NULL;

assert(pp->tag == PGPTAG_SIGNATURE);
    switch (pp->u.h[0]) {
    case 4:
	p = pp->u.h + sizeof(*pp->u.s);
	p += pgpGrab(pp->u.s->hashlen, sizeof(pp->u.s->hashlen));
	*plen = pgpGrab(p, 2);
	p += 2;
	break;
    }
    return p;
}

static const rpmuint8_t * ppSignid(pgpPkt pp)
{
    const rpmuint8_t * p = NULL;
    size_t nunhash = 0;
    const rpmuint8_t * punhash;
    size_t tlen = 0;

assert(pp->tag == PGPTAG_SIGNATURE);
    switch (pp->u.h[0]) {
    case 3:	 p = pp->u.r->signid;		break;
    case 4:
	punhash = ppSigUnhash(pp, &nunhash);
	p = pgpGrabSubTagVal(punhash, nunhash, PGPSUBTYPE_ISSUER_KEYID, &tlen);
assert(p == NULL || tlen == 8);
	break;
    }
    return p;
}

static rpmuint32_t ppSigTime(pgpPkt pp)
{
    const rpmuint8_t * p = NULL;
    size_t nhash = 0;
    const rpmuint8_t * phash;
    size_t tlen = 0;
    rpmuint32_t sigtime = 0;

assert(pp->tag == PGPTAG_SIGNATURE);
    switch (pp->u.h[0]) {
    case 3:	sigtime = pgpGrab(pp->u.r->time, sizeof(pp->u.r->time)); break;
    case 4:
	phash = ppSigHash(pp, &nhash);
	p = pgpGrabSubTagVal(phash, nhash, PGPSUBTYPE_SIG_CREATE_TIME, &tlen);
	if (p)	sigtime = pgpGrab(p, 4);
	break;
    }
    return sigtime;
}

static rpmuint8_t ppSigType(pgpPkt pp)
{
    rpmuint8_t sigtype = 0;
assert(pp->tag == PGPTAG_SIGNATURE);
    switch (pp->u.h[0]) {
    case 3:	sigtype = pp->u.r->sigtype;	break;
    case 4:	sigtype = pp->u.s->sigtype;	break;
    }
    return sigtype;
}

/*==============================================================*/
static const char * rpmhkpEscape(const char * keyname)
{
    /* essentially curl_escape() */
    /* XXX doubles as hex encode string */
    static char ok[] =
	"0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    const char * s;
    char * t, *te;
    size_t nb = 0;

    for (s = keyname; *s; s++)
	nb += (strchr(ok, *s) == NULL ? 4 : 1);

    te = t = (char *) xmalloc(nb + 1);
    for (s = keyname; *s; s++) {
	if (strchr(ok, *s) == NULL) {
	    *te++ = '%';	/* XXX extra '%' macro expansion escaping. */
	    *te++ = '%';
	    *te++ = ok[(*s >> 4) & 0x0f];
	    *te++ = ok[(*s     ) & 0x0f];
	} else
	    *te++ = *s;
    }
    *te = '\0';
    return t;
}

rpmhkp rpmhkpLookup(const char * keyname)
{
    const char * kn = rpmhkpEscape(keyname);
    /* XXX the "0x" is sometimes in macro and sometimes in keyname. */
    const char * fn = rpmExpand("%{_hkp_keyserver_query}", kn, NULL);
    rpmhkp hkp = NULL;
    pgpArmor pa;
    int rc = 1;	/* assume failure */

HKPDEBUG((stderr, "--> %s(%s)\n", __FUNCTION__, keyname));

    /* Are hkp:// lookups disabled? */
    if (fn && *fn && *fn == '%')
	goto exit;

    SUM.lookups++;

    hkp = rpmhkpNew(NULL, 0);

    /* Strip off the base64 and verify the crc32. */
    pa = pgpReadPkts(fn, &hkp->pkt, &hkp->pktlen);
    if (pa == PGPARMOR_ERROR || pa == PGPARMOR_NONE
     || hkp->pkt == NULL || hkp->pktlen == 0)
	goto exit;

    /* Split the result into packet array. */
    rc = pgpGrabPkts(hkp->pkt, hkp->pktlen, &hkp->pkts, &hkp->npkts);

    /* XXX make sure this works with lazy web-of-trust loading. */
    /* XXX make sure 1st entry is a PUB. */
    /* XXX sloppy queries have multiple PUB's */
    if (!rc)
	(void) pgpPubkeyFingerprint(hkp->pkt, hkp->pktlen, hkp->keyid);

exit:
    if (rc && hkp)
	hkp = rpmhkpFree(hkp);
    fn = _free(fn);
    kn = _free(kn);

HKPDEBUG((stderr, "<-- %s(%s) hkp %p\n", __FUNCTION__, keyname, hkp));

    return hkp;
}

int rpmhkpLoadKey(rpmhkp hkp, pgpDig dig,
		int keyx, rpmuint8_t pubkey_algo)
{
    pgpDigParams pubp = pgpGetPubkey(dig);
    pgpPkt pp = (pgpPkt) alloca(sizeof(*pp));
    /* XXX "best effort" use primary pubkey if keyx is bogus */
    int ix = (keyx >= 0 && keyx < hkp->npkts) ? keyx : 0;
    size_t pleft = hkp->pktlen - (hkp->pkts[ix] - hkp->pkt);
    int len = pgpPktLen(hkp->pkts[ix], pleft, pp);
    const rpmuint8_t * p;
    int rc = 0;	/* assume success */
len = len;

HKPDEBUG((stderr, "--> %s(%p,%p,%d,%u) ix %d V%u\n", __FUNCTION__, hkp, dig, keyx, pubkey_algo, ix, pp->u.h[0]));

    pubp->tag = pp->tag;
    if (pp->u.h[0] == 3
     && (pubkey_algo == 0 || pubkey_algo == pp->u.j->pubkey_algo))
    {
	pubp->version = pp->u.j->version;
	memcpy(pubp->time, pp->u.j->time, sizeof(pubp->time));
	pubp->pubkey_algo = pp->u.j->pubkey_algo;
	p = ((rpmuint8_t *)pp->u.j) + sizeof(*pp->u.j);
	p = pgpPrtPubkeyParams(dig, pp, (pgpPubkeyAlgo)pp->u.j->pubkey_algo, p);
    } else
    if (pp->u.h[0] == 4
     && (pubkey_algo == 0 || pubkey_algo == pp->u.k->pubkey_algo))
    {
	pubp->version = pp->u.k->version;
	memcpy(pubp->time, pp->u.k->time, sizeof(pubp->time));
	pubp->pubkey_algo = pp->u.k->pubkey_algo;
	p = ((rpmuint8_t *)pp->u.k) + sizeof(*pp->u.k);
	p = pgpPrtPubkeyParams(dig, pp, (pgpPubkeyAlgo)pp->u.k->pubkey_algo, p);
    } else
	rc = -1;

HKPDEBUG((stderr, "<-- %s(%p,%p,%d,%u) rc %d\n", __FUNCTION__, hkp, dig, keyx, pubkey_algo, rc));

    return rc;
}

int rpmhkpFindKey(rpmhkp hkp, pgpDig dig,
		const rpmuint8_t * signid, rpmuint8_t pubkey_algo)
{
    pgpDigParams sigp = pgpGetSignature(dig);
    int keyx = -1;	/* assume notfound (in this cert) */
int xx;

HKPDEBUG((stderr, "--> %s(%p,%p,%p,%u)\n", __FUNCTION__, hkp, dig, signid, pubkey_algo));

    /* Try to load matching pubkey. */
    if (hkp->pubx >= 0 && hkp->pubx < hkp->npkts
     && !memcmp(hkp->keyid, signid, sizeof(hkp->keyid))) {
	if (!rpmhkpLoadKey(hkp, dig, hkp->pubx, sigp->pubkey_algo))
	    keyx = hkp->pubx;
	goto exit;
    }

    /* Try to load matching subkey. */
    if (hkp->subx >= 0 && hkp->subx < hkp->npkts
     && !memcmp(hkp->subid, signid, sizeof(hkp->subid))) {
	if (!rpmhkpLoadKey(hkp, dig, hkp->subx, sigp->pubkey_algo))
	    keyx = hkp->subx;
	goto exit;
    }

    /* Avoid repeating known signid lookup failures in awol list. */
    if (hkp->awol && rpmbfChk(hkp->awol, signid, 8)) {
	keyx = -2;
	SUM.AWOL.good++;
	goto exit;
    }

    /* Try to retrieve the pubkey. */
    {	char * keyname = rpmExpand("0x", pgpHexStr(signid, 8), NULL);
	rpmhkp ohkp = rpmhkpLookup(keyname);

	keyname = _free(keyname);	/* XXX coverity #1035902 */
	/* Add failed lookups to awol list. */
	if (ohkp == NULL) {
	    xx = rpmbfAdd(hkp->awol, signid, 8);
DESPEW((stderr, "\tAWOL\n"));
	    SUM.AWOL.bad++;
	    keyx = -2;
	    goto exit;
	}
	if (rpmhkpLoadKey(ohkp, dig, 0, sigp->pubkey_algo))
	    keyx = -2;		/* XXX skip V2 certs */
	ohkp = rpmhkpFree(ohkp);
    }

exit:

HKPDEBUG((stderr, "<-- %s(%p,%p,%p,%u) keyx %d\n", __FUNCTION__, hkp, dig, signid, pubkey_algo, keyx));

    return keyx;
}

int rpmhkpLoadSignature(rpmhkp hkp, pgpDig dig, pgpPkt pp)
{
    pgpDigParams sigp = pgpGetSignature(dig);
    const rpmuint8_t * p = NULL;
int rc;

    sigp->version = pp->u.h[0];

    if (pp->u.h[0] == 3) {
	sigp->version = pp->u.r->version;
	sigp->pubkey_algo = pp->u.r->pubkey_algo;
	sigp->hash_algo = pp->u.r->hash_algo;
	sigp->sigtype = pp->u.r->sigtype;
	memcpy(sigp->time, pp->u.r->time, sizeof(sigp->time));
	memset(sigp->expire, 0, sizeof(sigp->expire));
	sigp->hashlen = (size_t)pp->u.r->hashlen;
assert(sigp->hashlen == 5);
	sigp->hash = ((const rpmuint8_t *)&pp->u.r->hashlen) + 1;
	memcpy(sigp->signid, pp->u.r->signid, sizeof(sigp->signid));
	memcpy(sigp->signhash16, pp->u.r->signhash16, sizeof(sigp->signhash16));

/* XXX set pointer to signature parameters. */
p = ((rpmuint8_t *)pp->u.r) + sizeof(*pp->u.r);

    }

    if (pp->u.h[0] == 4) {
	const rpmuint8_t * phash;
	size_t nhash;
	const rpmuint8_t * punhash;
	size_t nunhash;
	size_t tlen;

	sigp->pubkey_algo = pp->u.s->pubkey_algo;
	sigp->hash_algo = pp->u.s->hash_algo;
	sigp->sigtype = pp->u.s->sigtype;

	phash = ((const rpmuint8_t *)pp->u.s) + sizeof(*pp->u.s);
	nhash = pgpGrab(pp->u.s->hashlen, sizeof(pp->u.s->hashlen));
	sigp->hash = (const rpmuint8_t *)pp->u.s;
	sigp->hashlen = sizeof(*pp->u.s) + nhash;

	nunhash = pgpGrab(phash+nhash, 2);
	punhash = phash + nhash + 2;
	memcpy(sigp->signhash16, punhash+nunhash, sizeof(sigp->signhash16));

#ifdef	DYING
tlen = 0;
p = pgpGrabSubTagVal(phash, nhash, PGPSUBTYPE_SIG_TARGET, &tlen);
if (p) fprintf(stderr, "*** SIG_TARGET %s\n", pgpHexStr(p, tlen));
tlen = 0;
p = pgpGrabSubTagVal(phash, nhash, PGPSUBTYPE_EMBEDDED_SIG, &tlen);
if (p) fprintf(stderr, "*** EMBEDDED_SIG %s\n", pgpHexStr(p, tlen));
tlen = 0;
p = pgpGrabSubTagVal(phash, nhash, PGPSUBTYPE_REVOKE_KEY, &tlen);
if (p) fprintf(stderr, "*** REVOKE_KEY %02X %02X %s\n", p[0], p[1], pgpHexStr(p+2, tlen-2));
tlen = 0;
p = pgpGrabSubTagVal(phash, nhash, PGPSUBTYPE_REVOKE_REASON, &tlen);
if (p) fprintf(stderr, "*** REVOKE_REASON %02X %s\n", *p, p+1);
#endif

	tlen = 0;
	p = pgpGrabSubTagVal(phash, nhash, PGPSUBTYPE_SIG_CREATE_TIME, &tlen);
	if (p)	memcpy(sigp->time, p, sizeof(sigp->time));
	else	memset(sigp->time, 0, sizeof(sigp->time));

	tlen = 0;
	p = pgpGrabSubTagVal(phash, nhash, PGPSUBTYPE_SIG_EXPIRE_TIME, &tlen);
	if (p)	memcpy(sigp->expire, p, sizeof(sigp->expire));
	else	memset(sigp->expire, 0, sizeof(sigp->expire));

	/* XXX only on self-signature. */
	tlen = 0;
	p = pgpGrabSubTagVal(phash, nhash, PGPSUBTYPE_KEY_EXPIRE_TIME, &tlen);
	if (p)	memcpy(sigp->keyexpire, p, sizeof(sigp->keyexpire));
	else	memset(sigp->keyexpire, 0, sizeof(sigp->keyexpire));

	tlen = 0;
	p = pgpGrabSubTagVal(punhash, nunhash, PGPSUBTYPE_ISSUER_KEYID, &tlen);

/* Certain (some @pgp.com) signatures are missing signature keyid packet. */
if (hkp && (p == NULL || tlen != 8)) p = hkp->keyid;

	if (p)	memcpy(sigp->signid, p, sizeof(sigp->signid));
	else	memset(sigp->signid, 0, sizeof(sigp->signid));

/* XXX set pointer to signature parameters. */
p = punhash + nunhash + 2;

    }

    /* XXX Load signature paramaters. */
    pgpPrtSigParams(dig, pp,
		(pgpPubkeyAlgo)sigp->pubkey_algo, (pgpSigType)sigp->sigtype, p);

rc = 0;
HKPDEBUG((stderr, "<-- %s(%p,%p,%p) rc %d V%u\n", __FUNCTION__, hkp, dig, pp, rc, sigp->version));

    return rc;
}

int rpmhkpUpdate(DIGEST_CTX ctx, const void * data, size_t len)
{
    int rc = rpmDigestUpdate(ctx, data, len);
SPEW((stderr, "*** Update(%5u): %s\n", (unsigned)len, pgpHexStr((const rpmuint8_t *)data, len)));
    return rc;
}

static DIGEST_CTX rpmhkpHashKey(rpmhkp hkp, int ix, pgpHashAlgo dalgo)
{
    DIGEST_CTX ctx = rpmDigestInit(dalgo, RPMDIGEST_NONE);
    pgpPkt pp = (pgpPkt) alloca(sizeof(*pp));

assert(ix >= 0 && ix < hkp->npkts);
switch (*hkp->pkts[ix]) {
default: fprintf(stderr, "*** %s: %02X\n", __FUNCTION__, *hkp->pkts[ix]);
case 0x99: case 0x98: case 0xb9: case 0xb8: break;
}
    (void) pgpPktLen(hkp->pkts[ix], hkp->pktlen, pp);

    hkp->goop[0] = 0x99;	/* XXX correct for revocation? */
    hkp->goop[1] = (pp->hlen >>  8) & 0xff;
    hkp->goop[2] = (pp->hlen      ) & 0xff;
    rpmhkpUpdate(ctx, hkp->goop, 3);
    rpmhkpUpdate(ctx, pp->u.h, pp->hlen);

HKPDEBUG((stderr, "<-- %s(%p,%d,%u) ctx %p\n", __FUNCTION__, hkp, ix, dalgo, ctx));

    return ctx;
}

static DIGEST_CTX rpmhkpHashUid(rpmhkp hkp, int ix, pgpHashAlgo dalgo)
{
    DIGEST_CTX ctx = rpmhkpHashKey(hkp, hkp->pubx, dalgo);
    pgpPkt pp = (pgpPkt) alloca(sizeof(*pp));

assert(ix > 0 && ix < hkp->npkts);
switch (*hkp->pkts[ix]) {
default: fprintf(stderr, "*** %s: %02X\n", __FUNCTION__, *hkp->pkts[ix]);
case 0xb4: break;
}
    (void) pgpPktLen(hkp->pkts[ix], hkp->pktlen, pp);

    hkp->goop[0] = *hkp->pkts[ix];
    hkp->goop[1] = (pp->hlen >> 24) & 0xff;
    hkp->goop[2] = (pp->hlen >> 16) & 0xff;
    hkp->goop[3] = (pp->hlen >>  8) & 0xff;
    hkp->goop[4] = (pp->hlen      ) & 0xff;
    rpmhkpUpdate(ctx, hkp->goop, 5);
    rpmhkpUpdate(ctx, pp->u.h, pp->hlen);

HKPDEBUG((stderr, "<-- %s(%p,%d,%u) ctx %p\n", __FUNCTION__, hkp, ix, dalgo, ctx));

    return ctx;
}

static DIGEST_CTX rpmhkpHashSubkey(rpmhkp hkp, int ix, pgpHashAlgo dalgo)
{
    DIGEST_CTX ctx = rpmhkpHashKey(hkp, hkp->pubx, dalgo);
    pgpPkt pp = (pgpPkt) alloca(sizeof(*pp));

assert(ix > 0 && ix < hkp->npkts);
switch (*hkp->pkts[ix]) {
default: fprintf(stderr, "*** %s: %02X\n", __FUNCTION__, *hkp->pkts[ix]);
case 0xb9: case 0xb8: break;
}
    (void) pgpPktLen(hkp->pkts[ix], hkp->pktlen, pp);

    hkp->goop[0] = 0x99;
    hkp->goop[1] = (pp->hlen >>  8) & 0xff;
    hkp->goop[2] = (pp->hlen      ) & 0xff;
    rpmhkpUpdate(ctx, hkp->goop, 3);
    rpmhkpUpdate(ctx, pp->u.h, pp->hlen);

HKPDEBUG((stderr, "<-- %s(%p,%d,%u) ctx %p\n", __FUNCTION__, hkp, ix, dalgo, ctx));

    return ctx;
}

static DIGEST_CTX rpmhkpHash(rpmhkp hkp, int keyx,
		pgpSigType sigtype, pgpHashAlgo dalgo)
{
    DIGEST_CTX ctx = NULL;

HKPDEBUG((stderr, "--> %s(%p,%d,%u,%u)\n", __FUNCTION__, hkp, keyx, sigtype, dalgo));

    switch (sigtype) {
    case PGPSIGTYPE_BINARY:
    case PGPSIGTYPE_TEXT:
    case PGPSIGTYPE_STANDALONE:
    default:
	break;
    case PGPSIGTYPE_GENERIC_CERT:
    case PGPSIGTYPE_PERSONA_CERT:
    case PGPSIGTYPE_CASUAL_CERT:
    case PGPSIGTYPE_POSITIVE_CERT:
	if (hkp->pubx >= 0 && hkp->uidx >= 0)
	    ctx = rpmhkpHashUid(hkp, hkp->uidx, dalgo);
	break;
    case PGPSIGTYPE_SUBKEY_BINDING:
	if (hkp->pubx >= 0 && hkp->subx >= 0)
	    ctx = rpmhkpHashSubkey(hkp, hkp->subx, dalgo);
	break;
    case PGPSIGTYPE_KEY_BINDING:
	if (hkp->pubx >= 0)
	    ctx = rpmhkpHashSubkey(hkp, hkp->pubx, dalgo);
	break;
    case PGPSIGTYPE_SIGNED_KEY:
	/* XXX search for signid amongst the packets? */
	break;
    case PGPSIGTYPE_KEY_REVOKE:
	/* XXX only primary key */
	/* XXX authorized revocation key too. */
	if (hkp->pubx >= 0)
	    ctx = rpmhkpHashKey(hkp, hkp->pubx, dalgo);
	break;
    case PGPSIGTYPE_SUBKEY_REVOKE:
	/* XXX only primary key */
	/* XXX authorized revocation key too. */
	if (hkp->pubx >= 0 && hkp->subx >= 0)
	    ctx = rpmhkpHashKey(hkp, hkp->subx, dalgo);
	break;
    case PGPSIGTYPE_CERT_REVOKE:
    case PGPSIGTYPE_TIMESTAMP:
    case PGPSIGTYPE_CONFIRM:
	break;
    }

HKPDEBUG((stderr, "<-- %s(%p,%d,%u,%u) ctx %p\n", __FUNCTION__, hkp, keyx, sigtype, dalgo, ctx));

    return ctx;
}

static int rpmhkpVerifyHash(rpmhkp hkp, pgpDig dig, DIGEST_CTX ctx)
{
    pgpDigParams sigp = pgpGetSignature(dig);
    const char * dname = xstrdup(rpmDigestName(ctx));
    rpmuint8_t * digest = NULL;
    size_t digestlen = 0;
    int rc = rpmDigestFinal(ctx, &digest, &digestlen, 0);

HKPDEBUG((stderr, "--> %s(%p,%p,%p)\n", __FUNCTION__, hkp, dig, ctx));

    rc = memcmp(sigp->signhash16, digest, sizeof(sigp->signhash16));

if (rc)
SPEW((stderr, "\t%s\t%s\n", dname, pgpHexStr(digest, digestlen)));
SPEW((stderr, "%s\t%s\n", (!rc ? "\tGOOD" : "------> BAD"), pgpHexStr(sigp->signhash16, sizeof(sigp->signhash16))));

    if (rc)
	SUM.HASH.bad++;
    else
	SUM.HASH.good++;

    digest = _free(digest);
    digestlen = 0;
    dname = _free(dname);

HKPDEBUG((stderr, "<-- %s(%p,%p,%p) rc %d\n", __FUNCTION__, hkp, dig, ctx, rc));
    return rc;
}

static int rpmhkpVerifySignature(rpmhkp hkp, pgpDig dig, DIGEST_CTX ctx)
{
    pgpDigParams sigp = pgpGetSignature(dig);
    int rc = 0;		/* XXX assume failure */

HKPDEBUG((stderr, "--> %s(%p,%p,%p)\n", __FUNCTION__, hkp, dig, ctx));

    switch (sigp->pubkey_algo) {
    case PGPPUBKEYALGO_RSA:
	if (pgpImplSetRSA(ctx, dig, sigp)) {
DESPEW((stderr, "------> BAD\t%s\n", pgpHexStr(sigp->signhash16, 2)));
	    SUM.HASH.bad++;
	    goto exit;
	}
	if (!pgpImplVerify(dig)) {
DESPEW((stderr, "------> BAD\tV%u %s-%s\n",
		sigp->version,
		_pgpPubkeyAlgo2Name(sigp->pubkey_algo),
		_pgpHashAlgo2Name(sigp->hash_algo)));
	    SUM.RSA.bad++;
	} else {
DESPEW((stderr, "\tGOOD\tV%u %s-%s\n",
		sigp->version,
		_pgpPubkeyAlgo2Name(sigp->pubkey_algo),
		_pgpHashAlgo2Name(sigp->hash_algo)));
	    SUM.RSA.good++;
	    rc = 1;
	}
	break;
    case PGPPUBKEYALGO_DSA:
	if (pgpImplSetDSA(ctx, dig, sigp)) {
DESPEW((stderr, "------> BAD\t%s\n", pgpHexStr(sigp->signhash16, 2)));
	    SUM.HASH.bad++;
	    goto exit;
	}
	if (!pgpImplVerify(dig)) {
DESPEW((stderr, "------> BAD\tV%u %s-%s\n",
		sigp->version,
		_pgpPubkeyAlgo2Name(sigp->pubkey_algo),
		_pgpHashAlgo2Name(sigp->hash_algo)));
	    SUM.DSA.bad++;
	} else {
DESPEW((stderr, "\tGOOD\tV%u %s-%s\n",
		sigp->version,
		_pgpPubkeyAlgo2Name(sigp->pubkey_algo),
		_pgpHashAlgo2Name(sigp->hash_algo)));
	    SUM.DSA.good++;
	    rc = 1;
	}
	break;
    case PGPPUBKEYALGO_ECDSA:
	if (pgpImplSetECDSA(ctx, dig, sigp)) {
DESPEW((stderr, "------> BAD\t%s\n", pgpHexStr(sigp->signhash16, 2)));
	    SUM.HASH.bad++;
	    goto exit;
	}
	if (!pgpImplVerify(dig)) {
DESPEW((stderr, "------> BAD\tV%u %s-%s\n",
		sigp->version,
		_pgpPubkeyAlgo2Name(sigp->pubkey_algo),
		_pgpHashAlgo2Name(sigp->hash_algo)));
	    SUM.ECDSA.bad++;
	} else {
DESPEW((stderr, "\tGOOD\tV%u %s-%s\n",
		sigp->version,
		_pgpPubkeyAlgo2Name(sigp->pubkey_algo),
		_pgpHashAlgo2Name(sigp->hash_algo)));
	    SUM.ECDSA.good++;
	    rc = 1;
	}
	break;
    }

exit:
HKPDEBUG((stderr, "<-- %s(%p,%p,%p) rc %d\n", __FUNCTION__, hkp, dig, ctx, rc));
    return rc;
}

static int rpmhkpVerify(rpmhkp hkp, pgpPkt pp)
{
    pgpDig dig = pgpDigNew(RPMVSF_DEFAULT, (pgpPubkeyAlgo)0);
    pgpDigParams sigp = pgpGetSignature(dig);
    pgpDigParams pubp = pgpGetPubkey(dig);
    DIGEST_CTX ctx = NULL;
    int keyx;
    int rc = 1;		/* assume failure */
int xx;

HKPDEBUG((stderr, "--> %s(%p,%p)\n", __FUNCTION__, hkp, pp));

    SUM.sigs++;

    /* XXX Load signature paramaters. */
    xx = rpmhkpLoadSignature(hkp, dig, pp);

    /* Ignore expired signatures. */
    {	time_t expire = pgpGrab(sigp->expire, sizeof(sigp->expire));
	time_t ctime = pgpGrab(sigp->time, sizeof(sigp->time));
	if (expire && (expire + ctime) < time(NULL)) {
	    SUM.expired++;
	    goto exit;
	}
    }

    /*
     * Skip PGP Global Directory Verification signatures.
     * http://www.kfwebs.net/articles/article/17/GPG-mass-cleaning-and-the-PGP-Corp.-Global-Directory
     */
    if (pgpGrab(sigp->signid+4, 4) == 0xCA57AD7C
     || (hkp->crl && rpmbfChk(hkp->crl, sigp->signid, 8))) {
	SUM.filtered++;
	goto exit;
    }

    rpmlog(_rpmhkp_lvl, "  SIG: %08X %08X V%u %s-%s %s\n",
		pgpGrab(sigp->signid, 4), pgpGrab(sigp->signid+4, 4),
		sigp->version,
		_pgpPubkeyAlgo2Name(sigp->pubkey_algo),
		_pgpHashAlgo2Name(sigp->hash_algo),
		_pgpSigType2Name(sigp->sigtype)
    );

    /* XXX Load pubkey paramaters. */
    keyx = rpmhkpFindKey(hkp, dig, sigp->signid, sigp->pubkey_algo);
    if (keyx == -2)	/* XXX AWOL */
	goto exit;

    /* Ignore expired keys (self-certs only). */
    {	time_t keyexpire = pgpGrab(sigp->keyexpire, sizeof(sigp->keyexpire));
	time_t keyctime = pgpGrab(pubp->time, sizeof(pubp->time));
	if (keyexpire && (keyexpire + keyctime) < time(NULL)) {
	    SUM.keyexpired++;
	    goto exit;
	}
    }

    ctx = rpmhkpHash(hkp, keyx,
		(pgpSigType)sigp->sigtype, (pgpHashAlgo)sigp->hash_algo);

    if (ctx) {

	if (sigp->hash)
	    rpmhkpUpdate(ctx, sigp->hash, sigp->hashlen);

	if (sigp->version == 4) {
	    hkp->goop[0] = sigp->version;
	    hkp->goop[1] = (rpmuint8_t)0xff;
	    hkp->goop[2] = (sigp->hashlen >> 24) & 0xff;
	    hkp->goop[3] = (sigp->hashlen >> 16) & 0xff;
	    hkp->goop[4] = (sigp->hashlen >>  8) & 0xff;
	    hkp->goop[5] = (sigp->hashlen      ) & 0xff;
	    rpmhkpUpdate(ctx, hkp->goop, 6);
	}

	switch (sigp->pubkey_algo) {
	/* XXX handle only RSA/DSA? */
	case PGPPUBKEYALGO_RSA:
	case PGPPUBKEYALGO_DSA:
	case PGPPUBKEYALGO_ECDSA:
	/* XXX don't fuss V3 signatures for validation yet. */
	    if (sigp->version == 4) {
		rc = rpmhkpVerifySignature(hkp, dig, ctx);
		break;
	    }
	    /*@fallthrough@*/
	default:
	    rc = rpmhkpVerifyHash(hkp, dig, ctx);
	    break;
	}
	ctx = NULL;
    }

exit:
    dig = pgpDigFree(dig);
HKPDEBUG((stderr, "<-- %s(%p,%p) rc %d\n", __FUNCTION__, hkp, pp, rc));
    return rc;	/* XXX 1 on success */
}

rpmRC rpmhkpValidate(rpmhkp hkp, const char * keyname)
{
    pgpPkt pp = (pgpPkt) alloca(sizeof(*pp));
    size_t pleft;
    rpmRC rc = RPMRC_NOKEY;		/* assume failure */
    int xx;
    int i;
const rpmuint8_t * signid;
rpmuint32_t thistime;

char tbuf[BUFSIZ];
char * t, * te;
te = t = tbuf;
*te = '\0';

HKPDEBUG((stderr, "--> %s(%p,%s)\n", __FUNCTION__, hkp, keyname));

    /* Do a lazy lookup before validating. */
    if (hkp == NULL && keyname && *keyname) {
	if ((hkp = rpmhkpLookup(keyname)) == NULL) {
	    rc = RPMRC_NOTFOUND;
	    return rc;
	}
    } else
    if ((hkp = rpmhkpLink(hkp)) == NULL)
	return rc;

    SUM.certs++;
assert(hkp->pkts);

    pleft = hkp->pktlen;
HKPDEBUG((stderr, "\tpkts %p[%u] pleft %u\n", hkp->pkts, (unsigned)hkp->npkts, (unsigned)pleft));
    for (i = 0; i < hkp->npkts; i++) {
	xx = pgpPktLen(hkp->pkts[i], pleft, pp);
assert(xx > 0);
	pleft -= pp->pktlen;
SPEW((stderr, "%6d %p[%3u] %02X %s\n", i, hkp->pkts[i], (unsigned)pp->pktlen, *hkp->pkts[i], _pgpTag2Name(pp->tag)));
SPEW((stderr, "\t%s\n", pgpHexStr(hkp->pkts[i], pp->pktlen)));

	switch (pp->tag) {
	default:
	    break;
	case PGPTAG_PUBLIC_KEY:
	    hkp->pubx = i;
	    /* XXX sloppy hkp:// queries can/will have multiple PUB's */
	    xx = pgpPubkeyFingerprint(hkp->pkts[i], pp->pktlen, hkp->keyid);
	    sprintf(te, "  PUB: %08X %08X",
			pgpGrab(hkp->keyid, 4), pgpGrab(hkp->keyid+4, 4));
	    te += strlen(te);
	    if (pp->u.h[0] == 3) {
		sprintf(te, " V%u %s", pp->u.j->version,
			_pgpPubkeyAlgo2Name(pp->u.j->pubkey_algo));
		te += strlen(te);
		if (pp->u.j->valid[0] || pp->u.j->valid[1]) {
		    rpmuint32_t days =
			pgpGrab(pp->u.j->valid, sizeof(pp->u.j->valid));
		    time_t expired =
			pgpGrab(pp->u.j->time, 4) + (24 * 60 * 60 * days);
		    if (expired < time(NULL)) {
			sprintf(te, " EXPIRED");
			te += strlen(te);
			/* XXX handle V3 expired */
		    }
		}
	    }
	    if (pp->u.h[0] == 4) {
		sprintf(te, " V%u %s", pp->u.k->version,
			_pgpPubkeyAlgo2Name(pp->u.k->pubkey_algo));
		te += strlen(te);
	    }
	    rpmlog(_rpmhkp_lvl, "%s\n", t);
	    te = t = tbuf;

	    break;
	case PGPTAG_USER_ID:
	    hkp->uidx = i;
	    break;
	case PGPTAG_PUBLIC_SUBKEY:
	    hkp->subx = i;
	    xx = pgpPubkeyFingerprint(hkp->pkts[i], pp->pktlen, hkp->subid);
	    sprintf(te, "  SUB: %08X %08X",
			pgpGrab(hkp->keyid, 4), pgpGrab(hkp->keyid+4, 4));
	    te += strlen(te);
	    if (pp->u.h[0] == 3) {
		sprintf(te, " V%u %s", pp->u.j->version,
			_pgpPubkeyAlgo2Name(pp->u.j->pubkey_algo));
		te += strlen(te);
		if (pp->u.j->valid[0] || pp->u.j->valid[1]) {
		    rpmuint32_t days =
			pgpGrab(pp->u.j->valid, sizeof(pp->u.j->valid));
		    time_t expired =
			pgpGrab(pp->u.j->time, 4) + (24 * 60 * 60 * days);
		    if (expired < time(NULL)) {
			sprintf(te, " EXPIRED");
			te += strlen(te);
			/* XXX handle V3 expired */
		    }
		}
	    }
	    if (pp->u.h[0] == 4) {
		sprintf(te, " V%u %s", pp->u.k->version,
			_pgpPubkeyAlgo2Name(pp->u.k->pubkey_algo));
		te += strlen(te);
	    }
	    rpmlog(_rpmhkp_lvl, "%s\n", t);
	    te = t = tbuf;
	    break;
	case PGPTAG_SIGNATURE:
	    switch (ppSigType(pp)) {
	    case PGPSIGTYPE_BINARY:
	    case PGPSIGTYPE_TEXT:
	    case PGPSIGTYPE_STANDALONE:
	    default:
		break;
	    case PGPSIGTYPE_GENERIC_CERT:
	    case PGPSIGTYPE_PERSONA_CERT:
	    case PGPSIGTYPE_CASUAL_CERT:
		break;
	    case PGPSIGTYPE_POSITIVE_CERT:
		signid = ppSignid(pp);
		/* XXX treat missing issuer as "this" pubkey signature. */
		if (signid && memcmp(hkp->keyid, signid, sizeof(hkp->keyid)))
		    break;
		hkp->sigx = i;
		if (!rpmhkpVerify(hkp, pp)) {	/* XXX 1 on success */
		    if (rc == RPMRC_NOKEY) rc = RPMRC_FAIL;
		    break;
		}
		thistime = ppSigTime(pp);
		if (thistime < hkp->tvalid)
		    break;
		hkp->tvalid = thistime;
		hkp->uvalidx = hkp->uidx;
		break;
	    case PGPSIGTYPE_SUBKEY_BINDING:
		if (!rpmhkpVerify(hkp, pp)) {	/* XXX 1 on success */
		    if (rc == RPMRC_NOKEY) rc = RPMRC_FAIL;
		    break;
		}
		SUM.subbound++;
		if (rc == RPMRC_NOKEY) rc = RPMRC_OK;
		break;
	    case PGPSIGTYPE_KEY_BINDING:
		if (!rpmhkpVerify(hkp, pp)) {	/* XXX 1 on success */
		    if (rc == RPMRC_NOKEY) rc = RPMRC_FAIL;
		    break;
		}
		SUM.pubbound++;
		if (rc == RPMRC_NOKEY) rc = RPMRC_OK;
		break;
	    case PGPSIGTYPE_KEY_REVOKE:
		if (!rpmhkpVerify(hkp, pp))	/* XXX 1 on success */
		    break;
		SUM.pubrevoked++;
		if (rc == RPMRC_NOKEY) rc = RPMRC_NOTTRUSTED;
		if (hkp->crl)
		    xx = rpmbfAdd(hkp->crl, hkp->keyid, sizeof(hkp->keyid));
		goto exit;	/* XXX stop validating revoked cert. */
		/*@notreached@*/ break;
	    case PGPSIGTYPE_SUBKEY_REVOKE:
		if (!rpmhkpVerify(hkp, pp))	/* XXX 1 on success */
		    break;
		SUM.subrevoked++;
#ifdef	NOTYET	/* XXX subid not loaded correctly yet. */
		if (rc == RPMRC_NOKEY) rc = RPMRC_NOTTRUSTED;
		if (hkp->crl)
		    xx = rpmbfAdd(hkp->crl, hkp->subid, sizeof(hkp->subid));
#endif
		break;
	    case PGPSIGTYPE_SIGNED_KEY:
	    case PGPSIGTYPE_CERT_REVOKE:
	    case PGPSIGTYPE_TIMESTAMP:
	    case PGPSIGTYPE_CONFIRM:
		break;
	    }
	    break;
	}
    }

exit:
    if ((hkp->uidx >= 0 && hkp->uidx < hkp->npkts) && hkp->tvalid > 0) {
	char user[256+1];
	size_t nuser;
	pgpPktUid * u;
	xx = pgpPktLen(hkp->pkts[hkp->uvalidx], hkp->pktlen, pp);
	/* XXX append a NUL avoiding encoding issues. */
	nuser = (pp->hlen > sizeof(user)-1 ? sizeof(user)-1 : pp->hlen);
	memset(user, 0, sizeof(user));
	u = (pgpPktUid *) pp->u.h;
	memcpy(user, u->userid, nuser);
	user[nuser] = '\0';
	rpmlog(_rpmhkp_lvl, "  UID: %s\n", user);
	/* Some POSITIVE cert succeeded, so mark OK. */
	rc = RPMRC_OK;
    }

    hkp = rpmhkpFree(hkp);
HKPDEBUG((stderr, "<-- %s(%p,%s) rc %d\n", __FUNCTION__, hkp, keyname, rc));

    return rc;
}

void _rpmhkpPrintStats(FILE * fp)
{
    if (fp == NULL) fp = stderr;
    fprintf(fp, "============\n");
    fprintf(fp, "    LOOKUPS:%10u\n", (unsigned) SUM.lookups);
    fprintf(fp, "    PUBKEYS:%10u\n", (unsigned) SUM.certs);
    fprintf(fp, " SIGNATURES:%10u\n", (unsigned) SUM.sigs);
    fprintf(fp, "  PUB bound:%10u\trevoked:%10u\texpired:%10u\n",
		(unsigned) SUM.pubbound,
		(unsigned) SUM.pubrevoked,
		(unsigned) SUM.keyexpired);
    fprintf(fp, "  SUB bound:%10u\trevoked:%10u\n",
		(unsigned) SUM.subbound,
		(unsigned) SUM.subrevoked);
    fprintf(fp, "    expired:%10u\n", (unsigned) SUM.expired);
    fprintf(fp, "   filtered:%10u\n", (unsigned) SUM.filtered);
    fprintf(fp, "  RSA:%10u:%-10u\n",
		(unsigned) SUM.RSA.good, (unsigned) (SUM.RSA.good+SUM.RSA.bad));
    fprintf(fp, "  DSA:%10u:%-10u\n",
		(unsigned) SUM.DSA.good, (unsigned) (SUM.DSA.good+SUM.DSA.bad));
    fprintf(fp, "ECDSA:%10u:%-10u\n",
		(unsigned) SUM.ECDSA.good, (unsigned) (SUM.ECDSA.good+SUM.ECDSA.bad));
    fprintf(fp, " HASH:%10u:%-10u\n",
		(unsigned) SUM.HASH.good, (unsigned) (SUM.HASH.good+SUM.HASH.bad));
    fprintf(fp, " AWOL:%10u:%-10u\n",
		(unsigned) SUM.AWOL.good, (unsigned) (SUM.AWOL.good+SUM.AWOL.bad));
    fprintf(fp, " SKIP:%10u:%-10u\n",
		(unsigned) SUM.SKIP.good, (unsigned) (SUM.SKIP.good+SUM.SKIP.bad));
}

void _rpmhkpDumpDigParams(const char * msg, pgpDigParams sigp, FILE * fp)
{
    if (fp == NULL) fp = stderr;
    fprintf(fp, "%s: %p\n", msg, sigp);
    fprintf(fp, "\t     userid: %s\n", sigp->userid);
    fprintf(fp, "\t       hash: %p[%u]\n", sigp->hash, (unsigned) sigp->hashlen);
    fprintf(fp, "\t        tag: %02X\n", sigp->tag);
    fprintf(fp, "\t    version: %02X\n", sigp->version);
    fprintf(fp, "\t       time: %08X\n",
		pgpGrab(sigp->time, sizeof(sigp->time)));
    fprintf(fp, "\tpubkey_algo: %02X %s\n",
		sigp->pubkey_algo, _pgpPubkeyAlgo2Name(sigp->pubkey_algo));
    fprintf(fp, "\t  hash_algo: %02X %s\n",
		sigp->hash_algo, _pgpHashAlgo2Name(sigp->hash_algo));
    fprintf(fp, "\t    sigtype: %02X %s\n",
		sigp->sigtype, _pgpSigType2Name(sigp->sigtype));
    fprintf(fp, "\t signhash16: %04X\n",
		pgpGrab(sigp->signhash16, sizeof(sigp->signhash16)));
    fprintf(fp, "\t     signid: %08X %08X\n",
		pgpGrab(sigp->signid, 4), pgpGrab(sigp->signid+4, 4));
    fprintf(fp, "\t      saved: %02X\n", sigp->saved);
}

void _rpmhkpDumpDig(const char * msg, pgpDig dig, FILE * fp)
{
    if (fp == NULL) fp = stderr;
    fprintf(fp, "%s: dig %p\n", msg, dig);

    fprintf(fp, "\t build_sign: %s\n", dig->build_sign);
    fprintf(fp, "\tpubkey_algo: %s\n", dig->pubkey_algoN);
    fprintf(fp, "\t  hash_algo: %s\n", dig->hash_algoN);

    fprintf(fp, "\t     sigtag: 0x%08x\n", dig->sigtag);
    fprintf(fp, "\t    sigtype: 0x%08x\n", dig->sigtype);
    fprintf(fp, "\t        sig: %p[%u]\n", dig->sig, (unsigned) dig->siglen);
    fprintf(fp, "\t    vsflags: 0x%08x\n", dig->vsflags);
    fprintf(fp, "\t findPubkey: %p\n", dig->findPubkey);
    fprintf(fp, "\t        _ts: %p\n", dig->_ts);
    fprintf(fp, "\t      ppkts: %p[%u]\n", dig->ppkts, dig->npkts);
    fprintf(fp, "\t     nbytes: 0x%08x\n", (unsigned) dig->nbytes);

    fprintf(fp, "\t       hsha: %p\n", dig->hsha);
    fprintf(fp, "\t       hdsa: %p\n", dig->hdsa);
    fprintf(fp, "\t       sha1: %p[%u]\n", dig->sha1, (unsigned) dig->sha1len);

    fprintf(fp, "\t     hecdsa: %p\n", dig->hecdsa);

    fprintf(fp, "\t     md5ctx: %p\n", dig->md5ctx);
    fprintf(fp, "\t       hrsa: %p\n", dig->hrsa);
    fprintf(fp, "\t        md5: %p[%u]\n", dig->md5, (unsigned) dig->md5len);
    fprintf(fp, "\t       impl: %p\n", dig->impl);

    _rpmhkpDumpDigParams("PUB", pgpGetPubkey(dig), fp);
    _rpmhkpDumpDigParams("SIG", pgpGetSignature(dig), fp);
}
