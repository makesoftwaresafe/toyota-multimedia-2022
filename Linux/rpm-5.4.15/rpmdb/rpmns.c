/** \ingroup rpmds
 * \file rpmdb/rpmns.c
 */
#include "system.h"

#define	_RPMIOB_INTERNAL	/* XXX rpmiobSlurp */
#include <rpmiotypes.h>
#include <rpmio.h>
#define	_RPMHKP_INTERNAL
#include <rpmhkp.h>
#include <rpmmacro.h>
#include <rpmcb.h>

#define	_RPMPGP_INTERNAL
#include <rpmpgp.h>

#include <rpmtypes.h>
#include <rpmtag.h>
#define	_RPMEVR_INTERNAL
#include <rpmevr.h>
#define	_RPMNS_INTERNAL
#include <rpmns.h>
#include <rpmdb.h>

#include <rpmps.h>
#define	_RPMTS_INTERNAL		/* XXX ts->hkp */
#include <rpmts.h>

#include "debug.h"

/*@access rpmts @*/
/*@access pgpDigParams @*/
/*@access rpmiob @*/

#ifdef __cplusplus
GENfree(rpmuint8_t **)
#endif	/* __cplusplus */

/*@unchecked@*/
#ifdef	DEBUGGING
int _rpmns_debug = -1;
#define SPEW(_list)     if (_rpmns_debug) fprintf _list
#else
int _rpmns_debug;
#define SPEW(_list)
#endif

/*@unchecked@*/ /*@observer@*/ /*@relnull@*/
const char *_rpmns_N_at_A = ".";

/*@-nullassign@*/
/*@unchecked@*/ /*@observer@*/
static const char *rpmnsArches[] = {
    "i386", "i486", "i586", "i686", "athlon", "pentium3", "pentium4",
    "x86_64", "amd64", "ia32e",
    "alpha", "alphaev5", "alphaev56", "alphapca56", "alphaev6", "alphaev67",
    "sparc", "sun4", "sun4m", "sun4c", "sun4d", "sparcv8",
    "sparcv9", "sparcv9b", "sparcv9v", "sparcv9v2",
    "sparc64", "sun4u", "sparc64v",
    "mips", "mipsel", "IP",
    "ppc", "ppciseries", "ppcpseries",
    "ppc64", "ppc64iseries", "ppc64pseries",
    "m68k",
    "rs6000",
    "ia64",
    "armv3l", "armv4b", "armv4l",
    "armv5teb", "armv5tel", "armv5tejl",
    "armv6l",
    "s390", "i370", "s390x",
    "sh", "sh3", "sh4", "sh4a", "xtensa",
    "noarch", "fat",
    NULL,
};
/*@=nullassign@*/

nsType rpmnsArch(const char * str)
{
    nsType rc = RPMNS_TYPE_UNKNOWN;
    const char ** av;

#if defined(RPM_VENDOR_WINDRIVER)
    const char * known_arch = rpmExpand("%{?_known_arch}", NULL);
    const char *p, *pe, *t;
    for (p = pe = known_arch ; rc == RPMNS_TYPE_UNKNOWN && pe && *pe ; ) {
	while (*p && xisspace(*p)) p++;
	pe = p ; while (*pe && !xisspace(*pe)) pe++;
	if (p == pe)
	    break;
	t = strndup(p, (pe - p));
	p = pe;
	if (!strcmp(str, t))
	    rc = RPMNS_TYPE_ARCH;
	t = _free(t);
    }
    known_arch = _free(known_arch);
#endif

    if (rc == RPMNS_TYPE_UNKNOWN)
    for (av = rpmnsArches; *av != NULL; av++) {
	if (strcmp(str, *av))
	    continue;
	rc = RPMNS_TYPE_ARCH;
	break;
    }

    return rc;
}

/**
 * Dependency probe table (must be sorted).
 */
/*@unchecked@*/ /*@observer@*/
#define	_ENTRY(_s, _type)	{ #_s, sizeof(#_s)-1, _type }
static struct _rpmnsProbes_s {
/*@observer@*/ /*@relnull@*/
    const char * NS;
    size_t NSlen;
    nsType Type;
} rpmnsProbes[] = {
    _ENTRY(RWX,		RPMNS_TYPE_ACCESS),
    _ENTRY(RW_,		RPMNS_TYPE_ACCESS),
    _ENTRY(RWx,		RPMNS_TYPE_ACCESS),
    _ENTRY(R_X,		RPMNS_TYPE_ACCESS),
    _ENTRY(R__,		RPMNS_TYPE_ACCESS),
    _ENTRY(R_x,		RPMNS_TYPE_ACCESS),
    _ENTRY(RwX,		RPMNS_TYPE_ACCESS),
    _ENTRY(Rw_,		RPMNS_TYPE_ACCESS),
    _ENTRY(Rwx,		RPMNS_TYPE_ACCESS),
    _ENTRY(_WX,		RPMNS_TYPE_ACCESS),
    _ENTRY(_W_,		RPMNS_TYPE_ACCESS),
    _ENTRY(_Wx,		RPMNS_TYPE_ACCESS),
    _ENTRY(__X,		RPMNS_TYPE_ACCESS),
    _ENTRY(___,		RPMNS_TYPE_ACCESS),
    _ENTRY(__x,		RPMNS_TYPE_ACCESS),
    _ENTRY(_wX,		RPMNS_TYPE_ACCESS),
    _ENTRY(_w_,		RPMNS_TYPE_ACCESS),
    _ENTRY(_wx,		RPMNS_TYPE_ACCESS),
    _ENTRY(config,	RPMNS_TYPE_CONFIG),
    _ENTRY(cpuinfo,	RPMNS_TYPE_CPUINFO),
    _ENTRY(digest,	RPMNS_TYPE_DIGEST),
    _ENTRY(diskspace,	RPMNS_TYPE_DISKSPACE),
    _ENTRY(dpkg,	RPMNS_TYPE_DPKG),
    _ENTRY(envvar,	RPMNS_TYPE_ENVVAR),
    _ENTRY(executable,	RPMNS_TYPE_ACCESS),
    _ENTRY(exists,	RPMNS_TYPE_ACCESS),
    _ENTRY(getconf,	RPMNS_TYPE_GETCONF),
    _ENTRY(gnupg,	RPMNS_TYPE_GNUPG),
    _ENTRY(group,	RPMNS_TYPE_GROUP),
    _ENTRY(macro,	RPMNS_TYPE_MACRO),
    _ENTRY(mounted,	RPMNS_TYPE_MOUNTED),
    _ENTRY(rWX,		RPMNS_TYPE_ACCESS),
    _ENTRY(rW_,		RPMNS_TYPE_ACCESS),
    _ENTRY(rWx,		RPMNS_TYPE_ACCESS),
    _ENTRY(r_X,		RPMNS_TYPE_ACCESS),
    _ENTRY(r__,		RPMNS_TYPE_ACCESS),
    _ENTRY(r_x,		RPMNS_TYPE_ACCESS),
    _ENTRY(readable,	RPMNS_TYPE_ACCESS),
    _ENTRY(rpmlib,	RPMNS_TYPE_RPMLIB),
    _ENTRY(running,	RPMNS_TYPE_RUNNING),
    _ENTRY(rwX,		RPMNS_TYPE_ACCESS),
    _ENTRY(rw_,		RPMNS_TYPE_ACCESS),
    _ENTRY(rwx,		RPMNS_TYPE_ACCESS),
    _ENTRY(sanitycheck,	RPMNS_TYPE_SANITY),
    _ENTRY(set,		RPMNS_TYPE_SET),
    _ENTRY(signature,	RPMNS_TYPE_SIGNATURE),
    _ENTRY(soname,	RPMNS_TYPE_SONAME),
    _ENTRY(uname,	RPMNS_TYPE_UNAME),
    _ENTRY(user,	RPMNS_TYPE_USER),
    _ENTRY(vcheck,	RPMNS_TYPE_VCHECK),
    _ENTRY(verify,	RPMNS_TYPE_VERIFY),
    _ENTRY(writable,	RPMNS_TYPE_ACCESS),
};
#undef	_ENTRY
static size_t rpmnsProbesCount = sizeof(rpmnsProbes)/sizeof(rpmnsProbes[0]); 

nsType rpmnsProbe(const char * s, size_t slen)
{
    const char * se = strchr(s, ')');
    size_t l, u;

    /* XXX Don't bother if string cannot be "NS(N).A" */
    if (se == NULL || (slen = (se - s)) < sizeof("ABC()")-1)
	return RPMNS_TYPE_UNKNOWN;

    l = 0;
    u = rpmnsProbesCount;
    while (l < u) {
	size_t i = (l + u)/2;
	struct _rpmnsProbes_s * av = rpmnsProbes + i;
	size_t NSlen = (slen > av->NSlen) ? av->NSlen : slen;
	int rc = strncmp(s, av->NS, NSlen);

	if (rc == 0)
	    return (s[NSlen] == '(' ? rpmnsProbes[i].Type : RPMNS_TYPE_UNKNOWN);
	else if (rc < 0)
	    u = i;
	else if (rc > 0)
	    l = i + 1;
    }
    return RPMNS_TYPE_UNKNOWN;
}

nsType rpmnsClassify(const char * s, size_t slen)
{
    const char * se;
    nsType Type;

    if (slen == 0) slen = strlen(s);
    if (*s == '!') {
	s++;
	slen--;
    }
    if (*s == '/')
	return RPMNS_TYPE_PATH;
    se = s + slen;
    if (s[0] == '%' && s[1] == '{' && se[-1] == '}')
	return RPMNS_TYPE_FUNCTION;
    if ((se - s) > 3 && se[-3] == '.' && se[-2] == 's' && se[-1] == 'o')
	return RPMNS_TYPE_DSO;
    Type = rpmnsProbe(s, slen);
    if (Type != RPMNS_TYPE_UNKNOWN)
	return Type;
    for (se = s; *se != '\0'; se++) {
	if (se[0] == '(' || se[--slen] == ')')
	    return RPMNS_TYPE_NAMESPACE;
	if (se[0] == '.' && se[1] == 's' && se[2] == 'o')
	    return RPMNS_TYPE_DSO;
	if (se[0] == '.' && xisdigit((int)se[-1]) && xisdigit((int)se[1]))
	    return RPMNS_TYPE_VERSION;
	if (_rpmns_N_at_A && _rpmns_N_at_A[0]) {
	    if (se[0] == _rpmns_N_at_A[0] && rpmnsArch(se+1))
		return RPMNS_TYPE_ARCH;
	}
/*@-globstate@*/
	if (se[0] == '.')
	    return RPMNS_TYPE_COMPOUND;
    }
    return RPMNS_TYPE_STRING;
/*@=globstate@*/
}

int rpmnsParse(const char * s, rpmns ns)
{
    char * t = rpmExpand(s, NULL);
    size_t tlen = strlen(t);

    ns->Flags = (evrFlags) 0;
    ns->str = t;
    ns->Type = rpmnsClassify(t, tlen);

    switch (ns->Type) {
    case RPMNS_TYPE_ARCH:
	ns->NS = NULL;
	ns->N = ns->str;
	if (ns->N[0] == '!')
	    ns->N++;
	if ((t = strrchr(t, _rpmns_N_at_A[0])) != NULL)
	    *t++ = '\0';
	ns->A = t;
	break;
    case RPMNS_TYPE_RPMLIB:
    case RPMNS_TYPE_CPUINFO:
    case RPMNS_TYPE_GETCONF:
    case RPMNS_TYPE_UNAME:
    case RPMNS_TYPE_SONAME:
    case RPMNS_TYPE_ACCESS:
    case RPMNS_TYPE_USER:
    case RPMNS_TYPE_GROUP:
    case RPMNS_TYPE_MOUNTED:
    case RPMNS_TYPE_DISKSPACE:
    case RPMNS_TYPE_DIGEST:
    case RPMNS_TYPE_GNUPG:
    case RPMNS_TYPE_MACRO:
    case RPMNS_TYPE_ENVVAR:
    case RPMNS_TYPE_RUNNING:
    case RPMNS_TYPE_SANITY:
    case RPMNS_TYPE_VCHECK:
    case RPMNS_TYPE_SIGNATURE:
    case RPMNS_TYPE_VERIFY:
	ns->NS = ns->str;
	if (ns->NS[0] == '!')
	    ns->NS++;
	if ((t = strchr(t, '(')) != NULL) {
	    *t++ = '\0';
	    ns->N = t;
	    t[strlen(t)-1] = '\0';
	} else
	   ns->N = NULL;
	ns->A = NULL;
	break;
    case RPMNS_TYPE_UNKNOWN:
    case RPMNS_TYPE_STRING:
    case RPMNS_TYPE_PATH:
    case RPMNS_TYPE_DSO:
    case RPMNS_TYPE_FUNCTION:
    case RPMNS_TYPE_VERSION:
    case RPMNS_TYPE_COMPOUND:
    case RPMNS_TYPE_NAMESPACE:
    case RPMNS_TYPE_TAG:
    case RPMNS_TYPE_CONFIG:
    default:
	ns->NS = NULL;
	ns->N = ns->str;
	if (ns->N[0] == '!')
	    ns->N++;
	ns->A = NULL;
	break;
    }
    return 0;
}

/**
 * Convert hex to binary nibble.
 * @param c            hex character
 * @return             binary nibble
 */
static inline unsigned char nibble(char c)
	/*@*/
{
    if (c >= '0' && c <= '9')
	return (unsigned char)(c - '0');
    if (c >= 'A' && c <= 'F')
	return (unsigned char)((int)(c - 'A') + 10);
    if (c >= 'a' && c <= 'f')
	return (unsigned char)((int)(c - 'a') + 10);
    return '\0';
}

rpmRC rpmnsProbeSignature(void * _ts, const char * fn, const char * sigfn,
		const char * pubfn, const char * pubid,
		/*@unused@*/ int flags)
{
    rpmts ts = (rpmts) _ts;
    pgpDig dig = rpmtsDig(ts);
    pgpDigParams sigp = pgpGetSignature(dig);
    pgpDigParams pubp = pgpGetPubkey(dig);
    rpmuint8_t * sigpkt = NULL;
    size_t sigpktlen = 0;
    DIGEST_CTX ctx = NULL;
    rpmRC rc = RPMRC_FAIL;	/* assume failure */
    int xx;
rpmhkp hkp = NULL;
pgpPkt pp = (pgpPkt) alloca(sizeof(*pp));
size_t pleft;
int validate = 1;

SPEW((stderr, "==> check(%s, %s, %s, %s)\n", fn,
	(sigfn ? sigfn : "(null)"),
	(pubfn ? pubfn : "(null)"),
	(pubid ? pubid : "(null)")));

    /* Choose signature location: clearsign from fn if sigfn is NULL */
assert(fn && *fn);
    if (!(sigfn && *sigfn))
	sigfn = fn;

    /* Load the signature from the file. */
    {	const char * _sigfn = rpmExpand(sigfn, NULL);
	xx = pgpReadPkts(_sigfn, &sigpkt, &sigpktlen);
	if (xx != PGPARMOR_SIGNATURE) {
SPEW((stderr, "==> pgpReadPkts(%s) SIG %p[%u] ret %d\n", _sigfn, sigpkt, (unsigned)sigpktlen, xx));
	    _sigfn = _free(_sigfn);
	    goto exit;
	}
	_sigfn = _free(_sigfn);
    }

    pleft = sigpktlen;
    xx = pgpPktLen(sigpkt, pleft, pp);
    xx = rpmhkpLoadSignature(NULL, dig, pp);
    if (xx) goto exit;

    if (sigp->version != (rpmuint8_t)3 && sigp->version != (rpmuint8_t)4) {
SPEW((stderr, "==> unverifiable V%u\n", (unsigned)sigp->version));
	goto exit;
    }

    if (ts->hkp == NULL)
	ts->hkp = rpmhkpNew(NULL, 0);
    hkp = rpmhkpLink(ts->hkp);

    /* Load the pubkey. Use pubfn if specified, otherwise rpmdb keyring. */
    if (pubfn && *pubfn) {
	const char * _pubfn = rpmExpand(pubfn, NULL);
/*@-type@*/
hkp->pkt = _free(hkp->pkt);	/* XXX memleaks */
hkp->pktlen = 0;
	xx = pgpReadPkts(_pubfn, &hkp->pkt, &hkp->pktlen);
/*@=type@*/

	if (xx != PGPARMOR_PUBKEY) {
SPEW((stderr, "==> pgpReadPkts(%s) PUB %p[%u] rc %d\n", _pubfn, hkp->pkt, (unsigned)hkp->pktlen, xx));
	    _pubfn = _free(_pubfn);
	    goto exit;
	}
	_pubfn = _free(_pubfn);

	/* Split the result into packet array. */
hkp->pkts = _free(hkp->pkts);	/* XXX memleaks */
hkp->npkts = 0;
	xx = pgpGrabPkts(hkp->pkt, hkp->pktlen, &hkp->pkts, &hkp->npkts);

#ifdef	DYING
_rpmhkpDumpDig(__FUNCTION__, dig, NULL);
#endif

	if (!xx)
	    (void) pgpPubkeyFingerprint(hkp->pkt, hkp->pktlen, hkp->keyid);
	memcpy(pubp->signid, hkp->keyid, sizeof(pubp->signid));/* XXX useless */

	/* Validate pubkey self-signatures (if any). */
	/* XXX TODO: only validate once, then cache using rpmku */
	/* XXX need at least 3 packets to validate a pubkey */
	if (validate && hkp->npkts >= 3) {
#ifdef	DYING
pgpPrtPkts(hkp->pkt, hkp->pktlen, NULL, 1);
#endif
	    xx = rpmhkpValidate(hkp, NULL);
	    switch (xx) {
	    case RPMRC_OK:
		break;
	    case RPMRC_NOTFOUND:
	    case RPMRC_FAIL:	/* XXX remap to NOTFOUND? */
	    case RPMRC_NOTTRUSTED:
	    case RPMRC_NOKEY:
	    default:
SPEW((stderr, "\t<-- rpmhkpValidate() rc %d\n", xx));
		rc = (rpmRC)xx;
		goto exit;
	    }
	}

	/* Retrieve parameters from pubkey/subkey packet(s). */
	xx = rpmhkpFindKey(hkp, dig, sigp->signid, sigp->pubkey_algo);
	if (xx) {
SPEW((stderr, "\t<-- rpmhkpFindKey() rc %d\n", xx));
	    goto exit;
	}

    } else {
	rc = (rpmRC)pgpFindPubkey(dig);
	if (rc != RPMRC_OK) {
SPEW((stderr, "\t<-- pgpFindPubkey() rc %d\n", rc));
	    goto exit;
	}
    }

    /* Is this the requested pubkey? */
    if (pubid && *pubid) {
	size_t ns = strlen(pubid);
	const char * s;
	char * t;
	size_t i;

	/* At least 8 hex digits please. */
	for (i = 0, s = pubid; *s && isxdigit(*s); s++, i++)
	    {};
	if (!(*s == '\0' && i > 8 && (i%2) == 0)) {
SPEW((stderr, "==> invalid pubid:  %s\n", pubid));
	    goto exit;
	}

	/* Truncate to key id size. */
	s = pubid;
	if (ns > 16) {
	    s += (ns - 16);
	    ns = 16;
	}
	ns >>= 1;
	t = (char *) memset(alloca(ns), 0, ns);
	for (i = 0; i < ns; i++)
	    t[i] = (char)((nibble(s[2*i]) << 4) | nibble(s[2*i+1]));

	/* Compare the pubkey id. */
	s = (const char *)pubp->signid;
	xx = memcmp(t, s + (8 - ns), ns);

#ifdef	DYING
	/* XXX HACK: V4 RSA key id's are wonky atm. */
	if (pubp->pubkey_algo == (rpmuint8_t)PGPPUBKEYALGO_RSA)
	    xx = 0;
#endif

	if (xx) {
SPEW((stderr, "==> mismatched: pubkey id (%08x %08x) != %s\n",
pgpGrab(pubp->signid, 4), pgpGrab(pubp->signid+4, 4), pubid));
	    goto exit;
	}
    }

    /* Do the parameters match the signature? */
    if (!(sigp->pubkey_algo == pubp->pubkey_algo
#ifdef  NOTYET
     && sigp->hash_algo == pubp->hash_algo
#endif
     && !memcmp(sigp->signid, pubp->signid, sizeof(sigp->signid)) ) ) {

	SPEW((stderr, "==> mismatch between signature and pubkey\n"));
	SPEW((stderr, "\tpubkey_algo: %u  %u\n",
		(unsigned)sigp->pubkey_algo, (unsigned)pubp->pubkey_algo));
	SPEW((stderr, "\tsignid: %08X %08X    %08X %08X\n",
		pgpGrab(sigp->signid, 4), pgpGrab(sigp->signid+4, 4), 
		pgpGrab(pubp->signid, 4), pgpGrab(pubp->signid+4, 4)));
	goto exit;
    }

    /* Compute the message digest. */
    ctx = rpmDigestInit((pgpHashAlgo)sigp->hash_algo, RPMDIGEST_NONE);

    {	
	static const char clrtxt[] = "-----BEGIN PGP SIGNED MESSAGE-----";
	static const char sigtxt[] = "-----BEGIN PGP SIGNATURE-----";
	const char * _fn = rpmExpand(fn, NULL);
	rpmiob iob = NULL;
	int _rc = rpmiobSlurp(_fn, &iob);

	if (!(_rc == 0 && iob != NULL)) {
SPEW((stderr, "==> rpmiobSlurp(%s) MSG rc %d\n", _fn, _rc));
	    iob = rpmiobFree(iob);
	    _fn = _free(_fn);
	    goto exit;
	}
	_fn = _free(_fn);

	/* XXX clearsign sig is PGPSIGTYPE_TEXT not PGPSIGTYPE_BINARY. */
	if (!strncmp((char *)iob->b, clrtxt, strlen(clrtxt))) {
	    const char * be = (char *) (iob->b + iob->blen);
	    const char * t;

	    /* Skip to '\n\n' start-of-plaintext */
	    t = (char *) iob->b;
	    while (t && t < be && *t != '\n')
		t = strchr(t, '\n') + 1;
	    if (!(t && t < be))
		goto exit;
	    t++;

	    /* Clearsign digest rtrims " \t\r\n", inserts "\r\n" inter-lines. */
	    while (t < be) {
		const char * teol;
		const char * te;
		if (strncmp(t, "- ", 2) == 0)
			t += 2;
		if ((teol = te = strchr(t, '\n')) == NULL)
		    break;
		while (te > t && strchr(" \t\r\n", te[-1]))
		    te--;
		xx = rpmDigestUpdate(ctx, t, (te - t));
 		if (!strncmp((t = teol + 1), sigtxt, strlen(sigtxt)))
		    break;
		xx = rpmDigestUpdate(ctx, "\r\n", sizeof("\r\n")-1);
	    }
	} else
	    xx = rpmDigestUpdate(ctx, iob->b, iob->blen);

	iob = rpmiobFree(iob);
    }

    if (sigp->hash != NULL)
	xx = rpmDigestUpdate(ctx, sigp->hash, sigp->hashlen);

    if (sigp->version == (rpmuint8_t)4) {
	rpmuint8_t trailer[6];
	trailer[0] = sigp->version;
	trailer[1] = (rpmuint8_t)0xff;
	trailer[2] = (sigp->hashlen >> 24) & 0xff;
	trailer[3] = (sigp->hashlen >> 16) & 0xff;
	trailer[4] = (sigp->hashlen >>  8) & 0xff;
	trailer[5] = (sigp->hashlen      ) & 0xff;
	xx = rpmDigestUpdate(ctx, trailer, sizeof(trailer));
    }

    /* Load the message digest. */
    switch(sigp->pubkey_algo) {
    default:
	rc = RPMRC_FAIL;
	break;
    case PGPPUBKEYALGO_RSA:
	rc = (pgpImplSetRSA(ctx, dig, sigp) ? RPMRC_FAIL : RPMRC_OK);
	break;
    case PGPPUBKEYALGO_DSA:
	rc = (pgpImplSetDSA(ctx, dig, sigp) ? RPMRC_FAIL : RPMRC_OK);
	break;
    case PGPPUBKEYALGO_ECDSA:
	rc = (pgpImplSetECDSA(ctx, dig, sigp) ? RPMRC_FAIL : RPMRC_OK);
	break;
    }
    if (rc != RPMRC_OK) {
SPEW((stderr, "==> can't load pubkey_algo(%u)\n", (unsigned)sigp->pubkey_algo));
	goto exit;
    }

    /* Verify the signature. */
    switch(sigp->pubkey_algo) {
    default:
	rc = RPMRC_FAIL;
	break;
    case PGPPUBKEYALGO_RSA:
    case PGPPUBKEYALGO_DSA:
    case PGPPUBKEYALGO_ECDSA:
	rc = (pgpImplVerify(dig) ? RPMRC_OK : RPMRC_FAIL);
	break;
    }

exit:
    sigpkt = _free(sigpkt);
    (void) rpmhkpFree(hkp);
    hkp = NULL;
/*@-nullstate@*/
    rpmtsCleanDig(ts);
/*@=nullstate@*/

SPEW((stderr, "============================ verify: %s\n",
	(rc == RPMRC_OK ? "OK" :
	(rc == RPMRC_NOKEY ? "NOKEY" :
	"FAIL"))));

    return rc;
}
