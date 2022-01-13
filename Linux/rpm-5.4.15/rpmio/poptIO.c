/** \ingroup rpmio
 * \file rpmio/poptIO.c
 *  Popt tables for all rpmio-only executables.
 */

#include "system.h"

#include <poptIO.h>

#if defined(WITH_BEECRYPT)
#include <rpmbc.h>
#endif
#if defined(WITH_CDSA)
#include <rpmcdsa.h>
#endif
#if defined(WITH_GCRYPT)
#include <rpmgc.h>
#endif
#if defined(WITH_NSS)
#include <rpmnss.h>
#endif
#if defined(WITH_SSL)
#include <rpmssl.h>
#endif
#if defined(WITH_TOMCRYPT)
#include <rpmltc.h>
#endif

#include <rpmasn.h>
#include <rpmtpm.h>
#include <mongo.h>

#include <rpmaug.h>
#include <rpmbag.h>
#include <rpmbf.h>
#include <rpmcudf.h>
#include <rpmcvs.h>
#include <rpmficl.h>
#include <rpmgit.h>
#include <rpmhkp.h>
#include <rpmjs.h>
#include <rpmmrb.h>
#include <rpmnix.h>
#include <rpmodbc.h>
#include <rpmperl.h>
#include <rpmpython.h>
#include <rpmruby.h>
#include <rpmsql.h>
#include <rpmsquirrel.h>
#include <rpmsvn.h>
extern int _rpmsvn_debug;
#include <rpmtcl.h>

#include <rpmsm.h>
#include <rpmsp.h>
#include <rpmsx.h>

#include <set.h>

#include "debug.h"

#ifdef __cplusplus
GENfree(rpmioP)
#endif	/* __cplusplus */

const char *__progname;

#if !defined(POPT_ARGFLAG_TOGGLE)	/* XXX compat with popt < 1.15 */
#define	POPT_ARGFLAG_TOGGLE	0
#endif
#define POPT_SHOWVERSION	-999
#define POPT_UNDEFINE		-994
#define	POPT_CRYPTO		-993

/*@unchecked@*/
int __debug = 0;

/*@-exportheadervar@*/
/*@-redecl@*/
/*@unchecked@*/
extern int _ar_debug;
/*@unchecked@*/
extern int _av_debug;
/*@unchecked@*/
extern int _cpio_debug;
/*@unchecked@*/
extern int _ctx_debug;
/*@unchecked@*/
extern int _dav_debug;
/*@unchecked@*/
extern int _ftp_debug;
/*@unchecked@*/
extern int _fts_debug;
/*@unchecked@*/
extern int _ht_debug;
/*@unchecked@*/
extern int _html_debug;
/*@unchecked@*/
extern int _iosm_debug;
/*@unchecked@*/
extern int noLibio;
/*@unchecked@*/
extern int _pgp_debug;
/*@unchecked@*/
extern int _pgp_print;
/*@unchecked@*/
extern int _rpmio_debug;
/*@unchecked@*/
extern int _rpmiob_debug;
/*@unchecked@*/
extern int _rpmlua_debug;
/*@unchecked@*/
extern int _rpmsq_debug;
/*@unchecked@*/
extern int _rpmzq_debug;
/*@unchecked@*/
extern int _tar_debug;
/*@unchecked@*/
extern int _xar_debug;
/*@=redecl@*/
/*@=exportheadervar@*/

/*@unchecked@*/ /*@null@*/
const char * rpmioPipeOutput = NULL;

/*@unchecked@*/
const char * rpmioRootDir = "/";

/*@observer@*/ /*@unchecked@*/
const char *rpmioEVR = VERSION;

/*@unchecked@*/
static int rpmioInitialized = -1;

#ifdef	NOTYET
#ifdef WITH_LUA
/*@unchecked@*/
extern const char *rpmluaFiles;
#endif

/*@unchecked@*/
static char *rpmpoptfiles = RPMPOPTFILES;
#endif

int _rpmio_popt_context_flags = 0;

pgpHashAlgo rpmioDigestHashAlgo = (pgpHashAlgo) -1;

/**
 * Digest options using popt.
 */
struct poptOption rpmioDigestPoptTable[] = {
 { "md2", '\0', POPT_ARG_VAL, 	&rpmioDigestHashAlgo, PGPHASHALGO_MD2,
	N_("MD2 digest (RFC-1319)"), NULL },
 { "md4", '\0', POPT_ARG_VAL, 	&rpmioDigestHashAlgo, PGPHASHALGO_MD4,
	N_("MD4 digest"), NULL },
 { "md5", '\0', POPT_ARG_VAL, 	&rpmioDigestHashAlgo, PGPHASHALGO_MD5,
	N_("MD5 digest (RFC-1321)"), NULL },
 { "sha1",'\0', POPT_ARG_VAL, 	&rpmioDigestHashAlgo, PGPHASHALGO_SHA1,
	N_("SHA-1 digest (FIPS-180-1)"), NULL },
 { "sha224",'\0', POPT_ARG_VAL, &rpmioDigestHashAlgo, PGPHASHALGO_SHA224,
	N_("SHA-224 digest (FIPS-180-2)"), NULL },
 { "sha256",'\0', POPT_ARG_VAL, &rpmioDigestHashAlgo, PGPHASHALGO_SHA256,
	N_("SHA-256 digest (FIPS-180-2)"), NULL },
 { "sha384",'\0', POPT_ARG_VAL, &rpmioDigestHashAlgo, PGPHASHALGO_SHA384,
	N_("SHA-384 digest (FIPS-180-2)"), NULL },
 { "sha512",'\0', POPT_ARG_VAL, &rpmioDigestHashAlgo, PGPHASHALGO_SHA512,
	N_("SHA-512 digest (FIPS-180-2)"), NULL },
 { "skein-224",'\0', POPT_ARG_VAL, &rpmioDigestHashAlgo, PGPHASHALGO_SKEIN_224,
	N_("SKEIN-224 digest"), NULL },
 { "skein-256",'\0', POPT_ARG_VAL, &rpmioDigestHashAlgo, PGPHASHALGO_SKEIN_256,
	N_("SKEIN-256 digest"), NULL },
 { "skein-384",'\0', POPT_ARG_VAL, &rpmioDigestHashAlgo, PGPHASHALGO_SKEIN_384,
	N_("SKEIN-384 digest"), NULL },
 { "skein-512",'\0', POPT_ARG_VAL, &rpmioDigestHashAlgo, PGPHASHALGO_SKEIN_512,
	N_("SKEIN-512 digest"), NULL },
 { "skein-1024",'\0', POPT_ARG_VAL, &rpmioDigestHashAlgo, PGPHASHALGO_SKEIN_1024,
	N_("SKEIN-1024 digest"), NULL },
 { "arirang-224",'\0', POPT_ARG_VAL, &rpmioDigestHashAlgo, PGPHASHALGO_ARIRANG_224,
	N_("Arirang-224 digest"), NULL },
 { "arirang-256",'\0', POPT_ARG_VAL, &rpmioDigestHashAlgo, PGPHASHALGO_ARIRANG_256,
	N_("Arirang-256 digest"), NULL },
 { "arirang-384",'\0', POPT_ARG_VAL, &rpmioDigestHashAlgo, PGPHASHALGO_ARIRANG_384,
	N_("Arirang-384 digest"), NULL },
 { "arirang-512",'\0', POPT_ARG_VAL, &rpmioDigestHashAlgo, PGPHASHALGO_ARIRANG_512,
	N_("Arirang-512 digest"), NULL },
 { "blake-224",'\0', POPT_ARG_VAL, &rpmioDigestHashAlgo, PGPHASHALGO_BLAKE_224,
	N_("Blake-224 digest"), NULL },
 { "blake-256",'\0', POPT_ARG_VAL, &rpmioDigestHashAlgo, PGPHASHALGO_BLAKE_256,
	N_("Blake-256 digest"), NULL },
 { "blake-384",'\0', POPT_ARG_VAL, &rpmioDigestHashAlgo, PGPHASHALGO_BLAKE_384,
	N_("Blake-384 digest"), NULL },
 { "blake-512",'\0', POPT_ARG_VAL, &rpmioDigestHashAlgo, PGPHASHALGO_BLAKE_512,
	N_("Blake-512 digest"), NULL },
 { "blake2b",'\0', POPT_ARG_VAL, &rpmioDigestHashAlgo, PGPHASHALGO_BLAKE2B,
	N_("Blake2b digest"), NULL },
 { "blake2bp",'\0', POPT_ARG_VAL, &rpmioDigestHashAlgo, PGPHASHALGO_BLAKE2BP,
	N_("Blake2bp digest"), NULL },
 { "blake2s",'\0', POPT_ARG_VAL, &rpmioDigestHashAlgo, PGPHASHALGO_BLAKE2S,
	N_("Blake2s digest"), NULL },
 { "blake2sp",'\0', POPT_ARG_VAL, &rpmioDigestHashAlgo, PGPHASHALGO_BLAKE2SP,
	N_("Blake2sp digest"), NULL },
 { "bmw-224",'\0', POPT_ARG_VAL, &rpmioDigestHashAlgo, PGPHASHALGO_BMW_224,
	N_("BlueMidnightWish-224 digest"), NULL },
 { "bmw-256",'\0', POPT_ARG_VAL, &rpmioDigestHashAlgo, PGPHASHALGO_BMW_256,
	N_("BlueMidnightWish-256 digest"), NULL },
 { "bmw-384",'\0', POPT_ARG_VAL, &rpmioDigestHashAlgo, PGPHASHALGO_BMW_384,
	N_("BlueMidnightWish-384 digest"), NULL },
 { "bmw-512",'\0', POPT_ARG_VAL, &rpmioDigestHashAlgo, PGPHASHALGO_BMW_512,
	N_("BlueMidnightWish-512 digest"), NULL },
 { "chi-224",'\0', POPT_ARG_VAL, &rpmioDigestHashAlgo, PGPHASHALGO_CHI_224,
	N_("Chi-224 digest"), NULL },
 { "chi-256",'\0', POPT_ARG_VAL, &rpmioDigestHashAlgo, PGPHASHALGO_CHI_256,
	N_("Chi-256 digest"), NULL },
 { "chi-384",'\0', POPT_ARG_VAL, &rpmioDigestHashAlgo, PGPHASHALGO_CHI_384,
	N_("Chi-384 digest"), NULL },
 { "chi-512",'\0', POPT_ARG_VAL, &rpmioDigestHashAlgo, PGPHASHALGO_CHI_512,
	N_("Chi-512 digest"), NULL },
 { "cubehash-224",'\0', POPT_ARG_VAL, &rpmioDigestHashAlgo, PGPHASHALGO_CUBEHASH_224,
	N_("CubeHash-224 digest"), NULL },
 { "cubehash-256",'\0', POPT_ARG_VAL, &rpmioDigestHashAlgo, PGPHASHALGO_CUBEHASH_256,
	N_("CubeHash-256 digest"), NULL },
 { "cubehash-384",'\0', POPT_ARG_VAL, &rpmioDigestHashAlgo, PGPHASHALGO_CUBEHASH_384,
	N_("CubeHash-384 digest"), NULL },
 { "cubehash-512",'\0', POPT_ARG_VAL, &rpmioDigestHashAlgo, PGPHASHALGO_CUBEHASH_512,
	N_("CubeHash-512 digest"), NULL },
 { "echo-224",'\0', POPT_ARG_VAL, &rpmioDigestHashAlgo, PGPHASHALGO_ECHO_224,
	N_("Echo-224 digest"), NULL },
 { "echo-256",'\0', POPT_ARG_VAL, &rpmioDigestHashAlgo, PGPHASHALGO_ECHO_256,
	N_("Echo-256 digest"), NULL },
 { "echo-384",'\0', POPT_ARG_VAL, &rpmioDigestHashAlgo, PGPHASHALGO_ECHO_384,
	N_("Echo-384 digest"), NULL },
 { "echo-512",'\0', POPT_ARG_VAL, &rpmioDigestHashAlgo, PGPHASHALGO_ECHO_512,
	N_("Echo-512 digest"), NULL },
 { "edonr-224",'\0', POPT_ARG_VAL, &rpmioDigestHashAlgo, PGPHASHALGO_EDONR_224,
	N_("EdonR-224 digest"), NULL },
 { "edonr-256",'\0', POPT_ARG_VAL, &rpmioDigestHashAlgo, PGPHASHALGO_EDONR_256,
	N_("EdonR-256 digest"), NULL },
 { "edonr-384",'\0', POPT_ARG_VAL, &rpmioDigestHashAlgo, PGPHASHALGO_EDONR_384,
	N_("EdonR-384 digest"), NULL },
 { "edonr-512",'\0', POPT_ARG_VAL, &rpmioDigestHashAlgo, PGPHASHALGO_EDONR_512,
	N_("EdonR-512 digest"), NULL },
 { "fugue-224",'\0', POPT_ARG_VAL, &rpmioDigestHashAlgo, PGPHASHALGO_FUGUE_224,
	N_("Fugue-224 digest"), NULL },
 { "fugue-256",'\0', POPT_ARG_VAL, &rpmioDigestHashAlgo, PGPHASHALGO_FUGUE_256,
	N_("Fugue-256 digest"), NULL },
 { "fugue-384",'\0', POPT_ARG_VAL, &rpmioDigestHashAlgo, PGPHASHALGO_FUGUE_384,
	N_("Fugue-384 digest"), NULL },
 { "fugue-512",'\0', POPT_ARG_VAL, &rpmioDigestHashAlgo, PGPHASHALGO_FUGUE_512,
	N_("Fugue-512 digest"), NULL },
 { "groestl-224",'\0', POPT_ARG_VAL, &rpmioDigestHashAlgo, PGPHASHALGO_GROESTL_224,
	N_("Groestl-224 digest"), NULL },
 { "groestl-256",'\0', POPT_ARG_VAL, &rpmioDigestHashAlgo, PGPHASHALGO_GROESTL_256,
	N_("Groestl-256 digest"), NULL },
 { "groestl-384",'\0', POPT_ARG_VAL, &rpmioDigestHashAlgo, PGPHASHALGO_GROESTL_384,
	N_("Groestl-384 digest"), NULL },
 { "groestl-512",'\0', POPT_ARG_VAL, &rpmioDigestHashAlgo, PGPHASHALGO_GROESTL_512,
	N_("Groestl-512 digest"), NULL },
 { "hamsi-224",'\0', POPT_ARG_VAL, &rpmioDigestHashAlgo, PGPHASHALGO_HAMSI_224,
	N_("Hamsi-224 digest"), NULL },
 { "hamsi-256",'\0', POPT_ARG_VAL, &rpmioDigestHashAlgo, PGPHASHALGO_HAMSI_256,
	N_("Hamsi-256 digest"), NULL },
 { "hamsi-384",'\0', POPT_ARG_VAL, &rpmioDigestHashAlgo, PGPHASHALGO_HAMSI_384,
	N_("Hamsi-384 digest"), NULL },
 { "hamsi-512",'\0', POPT_ARG_VAL, &rpmioDigestHashAlgo, PGPHASHALGO_HAMSI_512,
	N_("Hamsi-512 digest"), NULL },
 { "jh-224",'\0', POPT_ARG_VAL, &rpmioDigestHashAlgo, PGPHASHALGO_JH_224,
	N_("JH-224 digest"), NULL },
 { "jh-256",'\0', POPT_ARG_VAL, &rpmioDigestHashAlgo, PGPHASHALGO_JH_256,
	N_("JH-256 digest"), NULL },
 { "jh-384",'\0', POPT_ARG_VAL, &rpmioDigestHashAlgo, PGPHASHALGO_JH_384,
	N_("JH-384 digest"), NULL },
 { "jh-512",'\0', POPT_ARG_VAL, &rpmioDigestHashAlgo, PGPHASHALGO_JH_512,
	N_("JH-512 digest"), NULL },
 { "keccak-224",'\0', POPT_ARG_VAL, &rpmioDigestHashAlgo, PGPHASHALGO_KECCAK_224,
	N_("Keccak-224 digest"), NULL },
 { "keccak-256",'\0', POPT_ARG_VAL, &rpmioDigestHashAlgo, PGPHASHALGO_KECCAK_256,
	N_("Keccak-256 digest"), NULL },
 { "keccak-384",'\0', POPT_ARG_VAL, &rpmioDigestHashAlgo, PGPHASHALGO_KECCAK_384,
	N_("Keccak-384 digest"), NULL },
 { "keccak-512",'\0', POPT_ARG_VAL, &rpmioDigestHashAlgo, PGPHASHALGO_KECCAK_512,
	N_("Keccak-512 digest"), NULL },
 { "lane-224",'\0', POPT_ARG_VAL, &rpmioDigestHashAlgo, PGPHASHALGO_LANE_224,
	N_("Lane-224 digest"), NULL },
 { "lane-256",'\0', POPT_ARG_VAL, &rpmioDigestHashAlgo, PGPHASHALGO_LANE_256,
	N_("Lane-256 digest"), NULL },
 { "lane-384",'\0', POPT_ARG_VAL, &rpmioDigestHashAlgo, PGPHASHALGO_LANE_384,
	N_("Lane-384 digest"), NULL },
 { "lane-512",'\0', POPT_ARG_VAL, &rpmioDigestHashAlgo, PGPHASHALGO_LANE_512,
	N_("Lane-512 digest"), NULL },
 { "luffa-224",'\0', POPT_ARG_VAL, &rpmioDigestHashAlgo, PGPHASHALGO_LUFFA_224,
	N_("Luffa-224 digest"), NULL },
 { "luffa-256",'\0', POPT_ARG_VAL, &rpmioDigestHashAlgo, PGPHASHALGO_LUFFA_256,
	N_("Luffa-256 digest"), NULL },
 { "luffa-384",'\0', POPT_ARG_VAL, &rpmioDigestHashAlgo, PGPHASHALGO_LUFFA_384,
	N_("Luffa-384 digest"), NULL },
 { "luffa-512",'\0', POPT_ARG_VAL, &rpmioDigestHashAlgo, PGPHASHALGO_LUFFA_512,
	N_("Luffa-512 digest"), NULL },
 { "md6-224",'\0', POPT_ARG_VAL, &rpmioDigestHashAlgo, PGPHASHALGO_MD6_224,
	N_("MD6-224 digest"), NULL },
 { "md6-256",'\0', POPT_ARG_VAL, &rpmioDigestHashAlgo, PGPHASHALGO_MD6_256,
	N_("MD6-256 digest"), NULL },
 { "md6-384",'\0', POPT_ARG_VAL, &rpmioDigestHashAlgo, PGPHASHALGO_MD6_384,
	N_("MD6-384 digest"), NULL },
 { "md6-512",'\0', POPT_ARG_VAL, &rpmioDigestHashAlgo, PGPHASHALGO_MD6_512,
	N_("MD6-512digest"), NULL },
 { "rg32-256",'\0', POPT_ARG_VAL, &rpmioDigestHashAlgo, PGPHASHALGO_RG32_256,
	N_("RadioGatun32-256 digest"), NULL },
 { "rg64-256",'\0', POPT_ARG_VAL, &rpmioDigestHashAlgo, PGPHASHALGO_RG64_256,
	N_("RadioGatun64-256 digest"), NULL },
 { "shabal-224",'\0', POPT_ARG_VAL, &rpmioDigestHashAlgo, PGPHASHALGO_SHABAL_224,
	N_("Shabal-224 digest"), NULL },
 { "shabal-256",'\0', POPT_ARG_VAL, &rpmioDigestHashAlgo, PGPHASHALGO_SHABAL_256,
	N_("Shabal-256 digest"), NULL },
 { "shabal-384",'\0', POPT_ARG_VAL, &rpmioDigestHashAlgo, PGPHASHALGO_SHABAL_384,
	N_("Shabal-384 digest"), NULL },
 { "shabal-512",'\0', POPT_ARG_VAL, &rpmioDigestHashAlgo, PGPHASHALGO_SHABAL_512,
	N_("Shabal-512 digest"), NULL },
 { "shavite3-224",'\0', POPT_ARG_VAL, &rpmioDigestHashAlgo, PGPHASHALGO_SHAVITE3_224,
	N_("SHAvite3-224 digest"), NULL },
 { "shavite3-256",'\0', POPT_ARG_VAL, &rpmioDigestHashAlgo, PGPHASHALGO_SHAVITE3_256,
	N_("SHAvite3-256 digest"), NULL },
 { "shavite3-384",'\0', POPT_ARG_VAL, &rpmioDigestHashAlgo, PGPHASHALGO_SHAVITE3_384,
	N_("SHAvite3-384 digest"), NULL },
 { "shavite3-512",'\0', POPT_ARG_VAL, &rpmioDigestHashAlgo, PGPHASHALGO_SHAVITE3_512,
	N_("SHAvite3-512 digest"), NULL },
 { "simd-224",'\0', POPT_ARG_VAL, &rpmioDigestHashAlgo, PGPHASHALGO_SIMD_224,
	N_("SIMD-224 digest"), NULL },
 { "simd-256",'\0', POPT_ARG_VAL, &rpmioDigestHashAlgo, PGPHASHALGO_SIMD_256,
	N_("SIMD-256 digest"), NULL },
 { "simd-384",'\0', POPT_ARG_VAL, &rpmioDigestHashAlgo, PGPHASHALGO_SIMD_384,
	N_("SIMD-384 digest"), NULL },
 { "simd-512",'\0', POPT_ARG_VAL, &rpmioDigestHashAlgo, PGPHASHALGO_SIMD_512,
	N_("SIMD-512 digest"), NULL },
 { "tib3-224",'\0', POPT_ARG_VAL, &rpmioDigestHashAlgo, PGPHASHALGO_TIB3_224,
	N_("TIB3-224 digest"), NULL },
 { "tib3-256",'\0', POPT_ARG_VAL, &rpmioDigestHashAlgo, PGPHASHALGO_TIB3_256,
	N_("TIB3-256 digest"), NULL },
 { "tib3-384",'\0', POPT_ARG_VAL, &rpmioDigestHashAlgo, PGPHASHALGO_TIB3_384,
	N_("TIB3-384 digest"), NULL },
 { "tib3-512",'\0', POPT_ARG_VAL, &rpmioDigestHashAlgo, PGPHASHALGO_TIB3_512,
	N_("TIB3-512 digest"), NULL },
 { "salsa10",'\0', POPT_ARG_VAL,&rpmioDigestHashAlgo, PGPHASHALGO_SALSA10,
	N_("SALSA-10 hash"), NULL },
 { "salsa20",'\0', POPT_ARG_VAL,&rpmioDigestHashAlgo, PGPHASHALGO_SALSA20,
	N_("SALSA-20 hash"), NULL },
 { "rmd128",'\0', POPT_ARG_VAL,	&rpmioDigestHashAlgo, PGPHASHALGO_RIPEMD128,
	N_("RIPEMD-128 digest"), NULL },
 { "rmd160",'\0', POPT_ARG_VAL,	&rpmioDigestHashAlgo, PGPHASHALGO_RIPEMD160,
	N_("RIPEMD-160 digest"), NULL },
 { "rmd256",'\0', POPT_ARG_VAL,	&rpmioDigestHashAlgo, PGPHASHALGO_RIPEMD256,
	N_("RIPEMD-256 digest"), NULL },
 { "rmd320",'\0', POPT_ARG_VAL,	&rpmioDigestHashAlgo, PGPHASHALGO_RIPEMD320,
	N_("RIPEMD-320 digest"), NULL },
 { "tiger",'\0', POPT_ARG_VAL,	&rpmioDigestHashAlgo, PGPHASHALGO_TIGER192,
	N_("TIGER digest"), NULL },
 { "crc32",'\0', POPT_ARG_VAL,	&rpmioDigestHashAlgo, PGPHASHALGO_CRC32,
	N_("CRC-32 checksum"), NULL },
 { "crc64",'\0', POPT_ARG_VAL,	&rpmioDigestHashAlgo, PGPHASHALGO_CRC64,
	N_("CRC-64 checksum"), NULL },
 { "adler32",'\0', POPT_ARG_VAL,&rpmioDigestHashAlgo, PGPHASHALGO_ADLER32,
	N_("ADLER-32 checksum"), NULL },
 { "jlu32",'\0', POPT_ARG_VAL,	&rpmioDigestHashAlgo, PGPHASHALGO_JLU32,
	N_("Lookup3 hash"), NULL },
 { "nodigest",'\0', POPT_ARG_VAL|POPT_ARGFLAG_DOC_HIDDEN, &rpmioDigestHashAlgo, PGPHASHALGO_NONE,
	N_("No hash algorithm"), NULL },
 { "alldigests",'\0', POPT_ARG_VAL|POPT_ARGFLAG_DOC_HIDDEN, &rpmioDigestHashAlgo, 256,
	N_("All hash algorithm(s)"), NULL },
    POPT_TABLEEND
};

/**
 * Display rpm version.
 */
static void printVersion(FILE * fp)
	/*@globals rpmioEVR, fileSystem @*/
	/*@modifies *fp, fileSystem @*/
{
    fprintf(fp, _("%s (" RPM_NAME ") %s\n"), __progname, rpmioEVR);
}

void rpmioConfigured(void)
	/*@globals rpmioInitialized @*/
	/*@modifies rpmioInitialized @*/
{

    if (rpmioInitialized < 0) {
	/* XXX TODO: add initialization side-effects. */
	rpmioInitialized = 0;
    }
    if (rpmioInitialized)
	exit(EXIT_FAILURE);
}

/**
 */
static void rpmioAllArgCallback(poptContext con,
                /*@unused@*/ enum poptCallbackReason reason,
                const struct poptOption * opt, const char * arg,
                /*@unused@*/ const void * data)
	/*@globals pgpImplVecs,
 		rpmGlobalMacroContext,
		fileSystem, internalState @*/
	/*@modifies con, pgpImplVecs,
 		rpmGlobalMacroContext,
		fileSystem, internalState @*/
{

    /* XXX avoid accidental collisions with POPT_BIT_SET for flags */
    if (opt->arg == NULL)
    switch (opt->val) {
    case 'q':
	rpmSetVerbosity(RPMLOG_WARNING);
	break;
    case 'v':
	rpmIncreaseVerbosity();
	break;
#ifdef	NOTYET
    case 'D':
    {	char *s, *t;
	/* XXX Convert '-' in macro name to underscore, skip leading %. */
	s = t = xstrdup(arg);
	while (*t && !xisspace((int)*t)) {
	    if (*t == '-') *t = '_';
	    t++;
	}
	t = s;
	if (*t == '%') t++;
	rpmioConfigured();
/*@-type@*/
	/* XXX adding macro to global context isn't Right Thing Todo. */
	(void) rpmDefineMacro(NULL, t, RMIL_CMDLINE);
	(void) rpmDefineMacro(rpmCLIMacroContext, t, RMIL_CMDLINE);
/*@=type@*/
	s = _free(s);
    }	break;
    case POPT_UNDEFINE:
    {	char *s, *t;
	/* XXX Convert '-' in macro name to underscore, skip leading %. */
	s = t = xstrdup(arg);
	while (*t && !xisspace((int)*t)) {
	    if (*t == '-') *t = '_';
	    t++;
	}
	t = s;
	if (*t == '%') t++;
/*@-type@*/
	rpmioConfigured();
	(void) rpmUndefineMacro(NULL, t);
	(void) rpmUndefineMacro(rpmCLIMacroContext, t);
/*@=type@*/
	s = _free(s);
    }	break;
    case 'E':
	rpmioConfigured();
	{   const char *val = rpmExpand(arg, NULL);
            size_t val_len;
            val_len = strlen(val);
            if (val[val_len - 1] == '\n')
                fwrite(val, val_len, 1, stdout);
            else
		fprintf(stdout, "%s\n", val);
	    val = _free(val);
	}
	break;
#endif	/* NOTYET */
    case POPT_CRYPTO:
	{   const char *val;
#ifdef	NOTYET
	    rpmioConfigured();
	    val = rpmExpand(arg, NULL);
#else
	    val = xstrdup(arg);
#endif	/* NOTYET */
	    if (!xstrcasecmp(val, "beecrypt") || !xstrcasecmp(val, "bc")) {
#if defined(WITH_BEECRYPT)
		pgpImplVecs = &rpmbcImplVecs;
#else
                rpmlog(RPMLOG_ERR, "BeeCrypt (\"beecrypt\") based cryptography implementation not available\n");
                exit(EXIT_FAILURE);
#endif
            }
	    else if (!xstrcasecmp(val, "gcrypt") || !xstrcasecmp(val, "gc")) {
#if defined(WITH_GCRYPT)
		pgpImplVecs = &rpmgcImplVecs;
#else
                rpmlog(RPMLOG_ERR, "GCrypt (\"gcrypt\") based cryptography implementation not available\n");
                exit(EXIT_FAILURE);
#endif
            }
	    else if (!xstrcasecmp(val, "tomcrypt") || !xstrcasecmp(val, "ltc")) {
#if defined(WITH_TOMCRYPT)
		pgpImplVecs = &rpmltcImplVecs;
#else
                rpmlog(RPMLOG_ERR, "TomCrypt (\"tomcrypt\") based cryptography implementation not available\n");
                exit(EXIT_FAILURE);
#endif
            }
	    else if (!xstrcasecmp(val, "NSS")) {
#if defined(WITH_NSS)
		pgpImplVecs = &rpmnssImplVecs;
#else
                rpmlog(RPMLOG_ERR, "Mozilla NSS (\"nss\") based cryptography implementation not available\n");
                exit(EXIT_FAILURE);
#endif
            }
	    else if (!xstrcasecmp(val, "OpenSSL") || !xstrcasecmp(val, "ssl")) {
#if defined(WITH_SSL)
		pgpImplVecs = &rpmsslImplVecs;
#else
                rpmlog(RPMLOG_ERR, "OpenSSL (\"openssl\") based cryptography implementation not available\n");
                exit(EXIT_FAILURE);
#endif
            }
	    else if (!xstrcasecmp(val, "CDSA")) {
#if defined(WITH_CDSA)
		pgpImplVecs = &rpmcdsaImplVecs;
#else
                rpmlog(RPMLOG_ERR, "CDSA (\"cdsa\") based cryptography implementation not available\n");
                exit(EXIT_FAILURE);
#endif
            }
            else {
                rpmlog(RPMLOG_ERR, "cryptography implementation \"%s\" not known\n", val);
                exit(EXIT_FAILURE);
            }
	    val = _free(val);
	}
	break;
    case POPT_SHOWVERSION:
	printVersion(stdout);
/*@i@*/	con = rpmioFini(con);
	exit(EXIT_SUCCESS);
	/*@notreached@*/ break;
    }
}

/*@unchecked@*/
int rpmioFtsOpts = 0;

/*@unchecked@*/
struct poptOption rpmioFtsPoptTable[] = {
 { "comfollow", '\0', POPT_BIT_SET,	&rpmioFtsOpts, FTS_COMFOLLOW,
	N_("FTS_COMFOLLOW: follow command line symlinks"), NULL },
 { "logical", '\0', POPT_BIT_SET,	&rpmioFtsOpts, FTS_LOGICAL,
	N_("FTS_LOGICAL: logical walk"), NULL },
 { "nochdir", '\0', POPT_BIT_SET,	&rpmioFtsOpts, FTS_NOCHDIR,
	N_("FTS_NOCHDIR: don't change directories"), NULL },
 { "nostat", '\0', POPT_BIT_SET,	&rpmioFtsOpts, FTS_NOSTAT,
	N_("FTS_NOSTAT: don't get stat info"), NULL },
 { "physical", '\0', POPT_BIT_SET,	&rpmioFtsOpts, FTS_PHYSICAL,
	N_("FTS_PHYSICAL: physical walk"), NULL },
 { "seedot", '\0', POPT_BIT_SET,	&rpmioFtsOpts, FTS_SEEDOT,
	N_("FTS_SEEDOT: return dot and dot-dot"), NULL },
 { "xdev", '\0', POPT_BIT_SET,		&rpmioFtsOpts, FTS_XDEV,
	N_("FTS_XDEV: don't cross devices"), NULL },
 { "whiteout", '\0', POPT_BIT_SET,	&rpmioFtsOpts, FTS_WHITEOUT,
	N_("FTS_WHITEOUT: return whiteout information"), NULL },
   POPT_TABLEEND
};

/*@-bitwisesigned -compmempass @*/
/*@unchecked@*/
struct poptOption rpmioAllPoptTable[] = {
/*@-type@*/ /* FIX: cast? */
 { NULL, '\0', POPT_ARG_CALLBACK | POPT_CBFLAG_INC_DATA | POPT_CBFLAG_CONTINUE,
        (void *)rpmioAllArgCallback, 0, NULL, NULL },
/*@=type@*/

 { "debug", 'd', POPT_ARG_VAL|POPT_ARGFLAG_DOC_HIDDEN, &__debug, -1,
	N_("Debug generic operations"), NULL },

#ifdef	NOTYET
 { "define", 'D', POPT_ARG_STRING, NULL, (int)'D',
	N_("Define MACRO with value EXPR"),
	N_("'MACRO EXPR'") },
 { "undefine", '\0', POPT_ARG_STRING, NULL, POPT_UNDEFINE,
	N_("Undefine MACRO"),
	N_("'MACRO'") },
 { "eval", 'E', POPT_ARG_STRING, NULL, (int)'E',
	N_("Print macro expansion of EXPR"),
	N_("'EXPR'") },

 { "macros", '\0', POPT_ARG_STRING, &rpmMacrofiles, 0,
	N_("Read <FILE:...> instead of default file(s)"),
	N_("<FILE:...>") },
#ifdef WITH_LUA
 { "rpmlua", '\0', POPT_ARG_STRING, &rpmluaFiles, 0,
	N_("Read <FILE:...> instead of default RPM Lua file(s)"),
	N_("<FILE:...>") },
#endif
 { "rpmpopt", '\0', POPT_ARG_STRING, NULL, 0,
	N_("Read <FILE:...> instead of default POPT file(s)"),
	N_("<FILE:...>") },
#endif	/* NOTYET */

 { "pipe", '\0', POPT_ARG_STRING|POPT_ARGFLAG_DOC_HIDDEN, &rpmioPipeOutput, 0,
	N_("Send stdout to CMD"),
	N_("CMD") },
 { "root", 'r', POPT_ARG_STRING|POPT_ARGFLAG_SHOW_DEFAULT, &rpmioRootDir, 0,
	N_("Use ROOT as top level directory"),
	N_("ROOT") },

 { "quiet", '\0', 0, NULL, (int)'q',
	N_("Provide less detailed output"), NULL},
 { "verbose", 'v', 0, NULL, (int)'v',
	N_("Provide more detailed output"), NULL},
 { "version", '\0', 0, NULL, POPT_SHOWVERSION,
	N_("Print the version"), NULL },

#if defined(HAVE_LIBIO_H) && defined(_G_IO_IO_FILE_VERSION)
 { "nolibio", '\0', POPT_ARG_VAL|POPT_ARGFLAG_TOGGLE|POPT_ARGFLAG_DOC_HIDDEN, &noLibio, -1,
	N_("Disable use of libio(3) API"), NULL},
#endif

 { "usecrypto",'\0', POPT_ARG_STRING|POPT_ARGFLAG_DOC_HIDDEN, NULL, POPT_CRYPTO,
        N_("Select cryptography implementation"),
	N_("CRYPTO") },

 { "ardebug", '\0', POPT_ARG_VAL|POPT_ARGFLAG_DOC_HIDDEN, &_ar_debug, -1,
	N_("Debug ar archives"), NULL},
 { "avdebug", '\0', POPT_ARG_VAL|POPT_ARGFLAG_DOC_HIDDEN, &_av_debug, -1,
	N_("Debug argv collections"), NULL},
 { "cpiodebug", '\0', POPT_ARG_VAL|POPT_ARGFLAG_DOC_HIDDEN, &_cpio_debug, -1,
	N_("Debug cpio archives"), NULL},
 { "ctxdebug", '\0', POPT_ARG_VAL|POPT_ARGFLAG_DOC_HIDDEN, &_ctx_debug, -1,
	N_("Debug digest contexts"), NULL},
 { "davdebug", '\0', POPT_ARG_VAL|POPT_ARGFLAG_DOC_HIDDEN, &_dav_debug, -1,
	N_("Debug WebDAV data stream"), NULL},
 { "ftpdebug", '\0', POPT_ARG_VAL|POPT_ARGFLAG_DOC_HIDDEN, &_ftp_debug, -1,
	N_("Debug FTP/HTTP data stream"), NULL},
 { "ftsdebug", '\0', POPT_ARG_VAL|POPT_ARGFLAG_DOC_HIDDEN, &_fts_debug, -1,
	N_("Debug Fts(3) traverse"), NULL},
 { "htdebug", '\0', POPT_ARG_VAL|POPT_ARGFLAG_DOC_HIDDEN, &_ht_debug, -1,
	N_("Debug hash tables"), NULL},
 { "htmldebug", '\0', POPT_ARG_VAL|POPT_ARGFLAG_DOC_HIDDEN, &_html_debug, -1,
	N_("Debug HTML parsing"), NULL},
 { "iosmdebug", '\0', POPT_ARG_VAL|POPT_ARGFLAG_DOC_HIDDEN, &_iosm_debug, -1,
	N_("Debug I/O state machine"), NULL},
 { "miredebug", '\0', POPT_ARG_VAL|POPT_ARGFLAG_DOC_HIDDEN, &_mire_debug, -1,
	N_("Debug miRE patterns"), NULL},
 { "pgpdebug", '\0', POPT_ARG_VAL|POPT_ARGFLAG_DOC_HIDDEN, &_pgp_debug, -1,
	N_("Debug PGP usage"), NULL},
 { "pgpprint", '\0', POPT_ARG_VAL|POPT_ARGFLAG_DOC_HIDDEN, &_pgp_print, -1,
	N_("Print PGP keys"), NULL},
 { "rpmiodebug", '\0', POPT_ARG_VAL|POPT_ARGFLAG_DOC_HIDDEN, &_rpmio_debug, -1,
	N_("Debug rpmio I/O"), NULL},
 { "rpmiobdebug", '\0', POPT_ARG_VAL|POPT_ARGFLAG_DOC_HIDDEN, &_rpmiob_debug, -1,
	N_("Debug rpmio I/O buffers"), NULL},

 { "rpmasndebug", '\0', POPT_ARG_VAL|POPT_ARGFLAG_DOC_HIDDEN, &_rpmasn_debug, -1,
	N_("Debug embedded ASN.1 interpreter"), NULL},
 { "rpmbagdebug", '\0', POPT_ARG_VAL|POPT_ARGFLAG_DOC_HIDDEN, &_rpmbag_debug, -1,
	N_("Debug depsolver wrappers "), NULL},
 { "rpmcvsdebug", '\0', POPT_ARG_VAL|POPT_ARGFLAG_DOC_HIDDEN, &_rpmcvs_debug, -1,
	N_("Debug CVS wrappers "), NULL},
 { "rpmgitdebug", '\0', POPT_ARG_VAL|POPT_ARGFLAG_DOC_HIDDEN, &_rpmgit_debug, -1,
	N_("Debug GIT wrappers "), NULL},
 { "rpmsetdebug", '\0', POPT_ARG_VAL|POPT_ARGFLAG_DOC_HIDDEN, &_rpmset_debug, -1,
	N_("Debug SET-VERSION wrappers "), NULL},
 { "rpmsvndebug", '\0', POPT_ARG_VAL|POPT_ARGFLAG_DOC_HIDDEN, &_rpmsvn_debug, -1,
	N_("Debug Subversion wrappers "), NULL},
 { "rpmtpmdebug", '\0', POPT_ARG_VAL|POPT_ARGFLAG_DOC_HIDDEN, &_rpmtpm_debug, -1,
	N_("Debug TPM emulator"), NULL},

 { "rpmaugdebug", '\0', POPT_ARG_VAL|POPT_ARGFLAG_DOC_HIDDEN, &_rpmaug_debug, -1,
	N_("Debug embedded Augeas interpreter"), NULL},
 { "rpmbfdebug", '\0', POPT_ARG_VAL|POPT_ARGFLAG_DOC_HIDDEN, &_rpmbf_debug, -1,
	N_("Debug Bloom filters"), NULL},
 { "rpmcudfdebug", '\0', POPT_ARG_VAL|POPT_ARGFLAG_DOC_HIDDEN, &_rpmcudf_debug, -1,
	N_("Debug embedded CUDF parser"), NULL},
 { "rpmficldebug", '\0', POPT_ARG_VAL|POPT_ARGFLAG_DOC_HIDDEN, &_rpmficl_debug, -1,
	N_("Debug embedded FICL interpreter"), NULL},
 { "rpmhkpdebug", '\0', POPT_ARG_VAL|POPT_ARGFLAG_DOC_HIDDEN, &_rpmhkp_debug, -1,
	N_("Debug hkp:// keyring"), NULL},
 { "rpmjsdebug", '\0', POPT_ARG_VAL|POPT_ARGFLAG_DOC_HIDDEN, &_rpmjs_debug, -1,
	N_("Debug embedded JavaScript interpreter"), NULL},
#ifdef WITH_LUA
 { "rpmluadebug", '\0', POPT_ARG_VAL|POPT_ARGFLAG_DOC_HIDDEN, &_rpmlua_debug, -1,
	N_("Debug embedded Lua interpreter"), NULL},
#endif
 { "rpmmgdebug", '\0', POPT_ARG_VAL|POPT_ARGFLAG_DOC_HIDDEN, &_rpmmg_debug, -1,
	N_("Debug rpmmg magic"), NULL},
 { "rpmmrbdebug", '\0', POPT_ARG_VAL|POPT_ARGFLAG_DOC_HIDDEN, &_rpmmrb_debug, -1,
	N_("Debug embedded MRuby interpreter"), NULL},
 { "mongodebug", '\0', POPT_ARG_VAL|POPT_ARGFLAG_DOC_HIDDEN, &_rpmmgo_debug, -1,
	N_("Debug Mongo DB connection"), NULL},
 { "nixdebug", '\0', POPT_ARG_VAL|POPT_ARGFLAG_DOC_HIDDEN, &_rpmnix_debug, -1,
	N_("Debug embedded Nix interpreter"), NULL},
 { "odbcdebug", '\0', POPT_ARG_VAL|POPT_ARGFLAG_DOC_HIDDEN, &_odbc_debug, -1,
	N_("Debug embedded ODBC interface"), NULL},
 { "rpmperldebug", '\0', POPT_ARG_VAL|POPT_ARGFLAG_DOC_HIDDEN, &_rpmperl_debug, -1,
	N_("Debug embedded Perl interpreter"), NULL},
 { "rpmpythondebug", '\0', POPT_ARG_VAL|POPT_ARGFLAG_DOC_HIDDEN, &_rpmpython_debug, -1,
	N_("Debug embedded Python interpreter"), NULL},
 { "rpmrubydebug", '\0', POPT_ARG_VAL|POPT_ARGFLAG_DOC_HIDDEN, &_rpmruby_debug, -1,
	N_("Debug embedded Ruby interpreter"), NULL},
 { "rpmsqdebug", '\0', POPT_ARG_VAL|POPT_ARGFLAG_DOC_HIDDEN, &_rpmsq_debug, -1,
	N_("Debug rpmsq Signal Queue"), NULL},
 { "rpmsqldebug", '\0', POPT_ARG_VAL|POPT_ARGFLAG_DOC_HIDDEN, &_rpmsql_debug, -1,
	N_("Debug embedded SQL interpreter"), NULL},
 { "rpmsquirreldebug", '\0', POPT_ARG_VAL|POPT_ARGFLAG_DOC_HIDDEN, &_rpmsquirrel_debug, -1,
	N_("Debug embedded SQUIRREL interpreter"), NULL},
 { "rpmsmdebug", '\0', POPT_ARG_VAL|POPT_ARGFLAG_DOC_HIDDEN, &_rpmsm_debug, 1,
	N_("Debug semanage"), NULL},
 { "rpmspdebug", '\0', POPT_ARG_VAL|POPT_ARGFLAG_DOC_HIDDEN, &_rpmsp_debug, 1,
	N_("Debug sepol"), NULL},
 { "rpmsxdebug", '\0', POPT_ARG_VAL|POPT_ARGFLAG_DOC_HIDDEN, &_rpmsx_debug, 1,
	N_("Debug selinux"), NULL},
 { "rpmtcldebug", '\0', POPT_ARG_VAL|POPT_ARGFLAG_DOC_HIDDEN, &_rpmtcl_debug, -1,
	N_("Debug embedded TCL interpreter"), NULL},
 { "rpmvcdebug", '\0', POPT_ARG_VAL|POPT_ARGFLAG_DOC_HIDDEN, &_rpmvc_debug, -1,
	N_("Debug embedded SQL virtual cursor"), NULL},
 { "rpmvtdebug", '\0', POPT_ARG_VAL|POPT_ARGFLAG_DOC_HIDDEN, &_rpmvt_debug, -1,
	N_("Debug embedded SQL virtual table"), NULL},
#ifdef WITH_BZIP2
 { "rpmzqdebug", '\0', POPT_ARG_VAL|POPT_ARGFLAG_DOC_HIDDEN, &_rpmzq_debug, -1,
	N_("Debug rpmzq Job Queuing"), NULL},
#endif
 { "xardebug", '\0', POPT_ARG_VAL|POPT_ARGFLAG_DOC_HIDDEN, &_xar_debug, -1,
	N_("Debug xar archives"), NULL},
 { "tardebug", '\0', POPT_ARG_VAL|POPT_ARGFLAG_DOC_HIDDEN, &_tar_debug, -1,
	N_("Debug tar archives"), NULL},
 { "stats", '\0', POPT_ARG_VAL,				&_rpmsw_stats, -1,
	N_("Display operation statistics"), NULL},
 { "urldebug", '\0', POPT_ARG_VAL|POPT_ARGFLAG_DOC_HIDDEN, &_url_debug, -1,
	N_("Debug URL cache handling"), NULL},

   POPT_TABLEEND
};
/*@=bitwisesigned =compmempass @*/

poptContext
rpmioFini(poptContext optCon)
{
    /* XXX this should be done in the rpmioClean() wrapper. */
    /* keeps memory leak checkers quiet */
    rpmFreeMacros(NULL);
/*@i@*/	rpmFreeMacros(rpmCLIMacroContext);

    rpmioClean();

    optCon = poptFreeContext(optCon);

#if defined(HAVE_MCHECK_H) && defined(HAVE_MTRACE)
    /*@-noeffect@*/
    muntrace();   /* Trace malloc only if MALLOC_TRACE=mtrace-output-file. */
    /*@=noeffect@*/
#endif

    return NULL;
}

static inline int checkfd(const char * devnull, int fdno, int flags)
	/*@*/
{
    struct stat sb;
    int ret = 0;

    if (fstat(fdno, &sb) == -1 && errno == EBADF)
	ret = (open(devnull, flags) == fdno) ? 1 : 2;
    return ret;
}

/*@-globstate@*/
poptContext
rpmioInit(int argc, char *const argv[], struct poptOption * optionsTable)
{
    poptContext optCon;
    int rc;
#ifdef	NOTYET
    int i;
#endif

#if defined(HAVE_MCHECK_H) && defined(HAVE_MTRACE)
    /*@-noeffect@*/
    mtrace();   /* Trace malloc only if MALLOC_TRACE=mtrace-output-file. */
    /*@=noeffect@*/
#endif
/*@-globs -mods@*/
    setprogname(argv[0]);       /* Retrofit glibc __progname */

    /* XXX glibc churn sanity */
    if (__progname == NULL) {
	if ((__progname = strrchr(argv[0], '/')) != NULL) __progname++;
	else __progname = argv[0];
    }
/*@=globs =mods@*/

    /* Insure that stdin/stdout/stderr are open, lest stderr end up in rpmdb. */
   {	static const char _devnull[] = "/dev/null";
#if defined(STDIN_FILENO)
	(void) checkfd(_devnull, STDIN_FILENO, O_RDONLY);
#endif
#if defined(STDOUT_FILENO)
	(void) checkfd(_devnull, STDOUT_FILENO, O_WRONLY);
#endif
#if defined(STDERR_FILENO)
	(void) checkfd(_devnull, STDERR_FILENO, O_WRONLY);
#endif
   }

#if defined(ENABLE_NLS) && !defined(__LCLINT__)
    (void) setlocale(LC_ALL, "" );
    (void) bindtextdomain(PACKAGE, LOCALEDIR);
    (void) textdomain(PACKAGE);
#endif

    rpmSetVerbosity(RPMLOG_NOTICE);

    if (optionsTable == NULL) {
	/* Read rpm configuration (if not already read). */
	rpmioConfigured();
	return NULL;
    }

#ifdef	NOTYET
    /* read all RPM POPT configuration files */
    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--rpmpopt") == 0 && i+1 < argc) {
            rpmpoptfiles = argv[i+1];
            break;
        }
        else if (strncmp(argv[i], "--rpmpopt=", 10) == 0) {
            rpmpoptfiles = argv[i]+10;
            break;
        }
    }
#endif	/* NOTYET */

/*@-nullpass -temptrans@*/
    optCon = poptGetContext(__progname, argc, (const char **)argv, optionsTable, _rpmio_popt_context_flags);
/*@=nullpass =temptrans@*/

#ifdef	NOTYET
#if defined(RPM_VENDOR_OPENPKG) /* stick-with-rpm-file-sanity-checking */ || \
    !defined(POPT_ERROR_BADCONFIG)	/* XXX POPT 1.15 retrofit */
  { char * path_buf = xstrdup(rpmpoptfiles);
    char *path;
    char *path_next;

    for (path = path_buf; path != NULL && *path != '\0'; path = path_next) {
        const char **av;
        int ac, i;

        /* locate start of next path element */
        path_next = strchr(path, ':');
        if (path_next != NULL && *path_next == ':')
            *path_next++ = '\0';
        else
            path_next = path + strlen(path);

        /* glob-expand the path element */
        ac = 0;
        av = NULL;
        if ((i = rpmGlob(path, &ac, &av)) != 0)
            continue;

        /* work-off each resulting file from the path element */
        for (i = 0; i < ac; i++) {
            const char *fn = av[i];
            if (fn[0] == '@' /* attention */) {
                fn++;
                if (!rpmSecuritySaneFile(fn)) {
                    rpmlog(RPMLOG_WARNING, "existing POPT configuration file \"%s\" considered INSECURE -- not loaded\n", fn);
                    continue;
                }
            }
            (void)poptReadConfigFile(optCon, fn);
            av[i] = _free(av[i]);
        }
        av = _free(av);
    }
    path_buf = _free(path_buf);
  }
#else
    /* XXX FIXME: better error message is needed. */
    if ((xx = poptReadConfigFiles(optCon, rpmpoptfiles)) != 0)
       rpmlog(RPMLOG_WARNING, "existing POPT configuration file \"%s\" considered INSECURE -- not loaded\n", rpmpoptfiles);
#endif

    /* read standard POPT configuration files */
    /* XXX FIXME: the 2nd arg useEnv flag is UNUSED. */
    (void) poptReadDefaultConfig(optCon, 1);

    poptSetExecPath(optCon, USRLIBRPM, 1);
#endif	/* NOTYET */

    /* Process all options, whine if unknown. */
    while ((rc = poptGetNextOpt(optCon)) > 0) {
	const char * optArg = poptGetOptArg(optCon);
/*@-dependenttrans -modobserver -observertrans @*/
	optArg = _free(optArg);
/*@=dependenttrans =modobserver =observertrans @*/
	switch (rc) {
	default:
/*@-nullpass@*/
	    fprintf(stderr, _("%s: option table misconfigured (%d)\n"),
		__progname, rc);
/*@=nullpass@*/
	    exit(EXIT_FAILURE);

	    /*@notreached@*/ /*@switchbreak@*/ break;
        }
    }

    if (rc < -1) {
/*@-nullpass@*/
	fprintf(stderr, "%s: %s: %s\n", __progname,
		poptBadOption(optCon, POPT_BADOPTION_NOALIAS),
		poptStrerror(rc));
/*@=nullpass@*/
	exit(EXIT_FAILURE);
    }

    /* Read rpm configuration (if not already read). */
    rpmioConfigured();

    if (__debug) {
	rpmIncreaseVerbosity();
	rpmIncreaseVerbosity();
    }

    return optCon;
}
/*@=globstate@*/

/**
 * Return text between pl and matching pr characters.
 * @param p		start of text
 * @param pl		left char, i.e. '[', '(', '{', etc.
 * @param pr		right char, i.e. ']', ')', '}', etc.
 * @return		pointer to matching pr char in string (or NULL)
 */
/*@null@*/
static char *
matchchar(char * p, char pl, char pr)
        /*@*/
{
    int lvl = 0;
    char c;

    while ((c = *p++) != '\0') {
        if (c == '\\') {	/* Ignore escaped chars */
            p++;
            continue;
        }
        if (c == pr) {
            if (--lvl <= 0)     return --p;
        } else if (c == pl)
            lvl++;
    }
    return (char *)NULL;
}

rpmioP rpmioPFree(rpmioP P)
{
    if (P != NULL) {
	P->str = _free(P->str);
	P->av = argvFree(P->av);
	P = _free(P);
    }
    return NULL;
}

rpmRC rpmioParse(rpmioP *Pptr, const char * str)
{
    static char whitespace[] = " \t\n\r";
    rpmioP P;
    rpmRC rc = RPMRC_FAIL;	/* assume failure */
    char *b;
    char *be;
    int xx;
    int c;

    if ((P = *Pptr) == NULL)
	*Pptr = P = (rpmioP) xcalloc(1, sizeof(*P));

    if (str != NULL) {
	P->str = _free(P->str);
	P->next = P->str = xstrdup(str);
    }

    /* Ltrim whitespace. Anything left to parse? */
    if ((b = P->next) != NULL)
    while (*b && strchr(whitespace, *b))
	*b++ = '\0';
    if (b == NULL || *b == '\0')
	return RPMRC_NOTFOUND;

    /* Parse next command into an argv. */
    P->ac = 0;
    P->av = argvFree(P->av);
    if ((be = b) != NULL)
  while (1) {
    c = *be++;
    switch (c) {
    default:
	break;
    case '\\':		/* escaped character. */
	if (*be != '\0')
	    be++;
	break;
    case '\0':		/* end-of-command termination. */
    case '\n':
    case '\r':
    case ';':	
	if (be[-1] != '\0')
	    be[-1] = '\0';
	else
	    be--;			/* XXX one too far */
	if ((be - b) > 1) {
	    xx = argvAdd(&P->av, b);
	    P->ac++;
	}
	goto exit;
	break;
    case '[':		/* XPath construct with '[' balancing. */
	if ((be = matchchar(be, '[', ']')) == NULL) {
	    be += strlen(b);	/* XXX unmatched ']' */
	    goto exit;
	}
	be++;
	break;
    case '"':		/* quoted string */
	while (1) {
	    if ((be = strchr(be, '"')) == NULL) {
		be += strlen(b);	/* XXX unmatched '"' */
		goto exit;
	    }
	    be++;
	    if (be[-2] == '\\')	/* escaped quote */
		continue;
	    break;
	}
	break;
    case ' ':		/* argument separator */
    case '\t':
	be[-1] = '\0';
	if ((be - b) > 1) {
	    xx = argvAdd(&P->av, b);
	    P->ac++;
	}
	b = be;
	while (*b && (*b == ' ' || *b == '\t'))
	    *b++ = '\0';
	be = b;
	break;
    }
  }
    rc = RPMRC_OK;

exit:
    P->next = be;
    return rc;
}
