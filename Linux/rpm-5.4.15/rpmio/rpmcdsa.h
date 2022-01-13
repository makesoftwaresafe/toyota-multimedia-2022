#ifndef	H_RPMCDSA
#define	H_RPMCDSA

/** \ingroup rpmpgp
 * \file rpmio/rpmcdsa.h
 */

#include <rpmiotypes.h>
#include <rpmpgp.h>
#include <rpmsw.h>

/* Implementation specific includes. */
#if defined(_RPMCDSA_INTERNAL)
#include <Security/Security.h>
#endif

/**
 */
typedef	/*abstract@*/ struct rpmcdsa_s * rpmcdsa;

/**
 * Implementation specific parameter storage.
 */
#if defined(_RPMCDSA_INTERNAL)
struct rpmcdsa_s {
    int in_fips_mode;	/* XXX trsa */
    int nbits;		/* XXX trsa */
    int qbits;		/* XXX trsa */
    int badok;		/* XXX trsa */
    int err;

    void * digest;
    size_t digestlen;

const CSSM_GUID * ModuleGuid;
CSSM_API_ModuleEventHandler AppNotifyCallback;	/* XXX NULL */
void * AppNotifyCallbackCtx;			/* XXX NULL */
CSSM_FUNC_NAME_ADDR * FunctionTable;		/* XXX NULL */
uint32 NumFunctionTable;			/* XXX 0 */
const void * Reserved;				/* XXX NULL */

CSSM_CSP_HANDLE cspHandle;
CSSM_CC_HANDLE ccHandle;

CSSM_ALGORITHMS algid;
const CSSM_CRYPTO_DATA *Seed;			/* XXX NULL */
const CSSM_DATA *Salt;				/* XXX NULL */
const CSSM_DATE *StartDate;			/* XXX NULL */
const CSSM_DATE *EndDate;			/* XXX NULL */
const CSSM_DATA *Params;

uint32 PublicKeyUsage;
uint32 PublicKeyAttr;
CSSM_DATA PublicKeyLabel;
uint32 PrivateKeyUsage;
uint32 PrivateKeyAttr;
CSSM_DATA PrivateKeyLabel;

CSSM_RESOURCE_CONTROL_CONTEXT * CredAndAclEntry;/* XXX NULL */
CSSM_ACCESS_CREDENTIALS * AccessCred;		/* XXX NULL */

CSSM_KEY sec_key;
CSSM_KEY pub_key;

CSSM_DATA signature;

    /* DSA parameters. */

    /* RSA parameters. */

    /* ECDSA parameters. */

};
#endif

/*@unchecked@*/
extern pgpImplVecs_t rpmcdsaImplVecs;

#endif	/* H_RPMCDSA */
