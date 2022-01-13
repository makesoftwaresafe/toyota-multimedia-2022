#ifndef	H_RPMTPM
#define	H_RPMTPM

/** \ingroup rpmio
 * \file rpmio/rpmtpm.h
 */

#include <stdlib.h>		/* XXX libtpm bootstrapping */
#include <stdint.h>		/* XXX libtpm bootstrapping */
#include <rpmio.h>
#include <rpmiotypes.h>
#include <rpmlog.h>
#include <rpmcb.h>
#include <argv.h>
#include <poptIO.h>

/** \ingroup rpmio
 */
/*@unchecked@*/
extern int _rpmtpm_debug;

/** \ingroup rpmio
 */
typedef /*@refcounted@*/ struct rpmtpm_s * rpmtpm;

#if defined(_RPMTPM_INTERNAL)

#define	TPM_POSIX			1
#define	TPM_V12				1
#define	TPM_NV_DISK			1
#define	TPM_MAXIMUM_KEY_SIZE		4096
#define	TPM_AES				1
#define	TPM_USE_TAG_IN_STRUCTURE	1

/** \ingroup rpmio
 */
struct rpmtpm_s {
    struct rpmioItem_s _item;	/*!< usage mutex and pool identifier. */

    int in_fips_mode;	/* XXX trsa */
    int nbits;		/* XXX trsa */
    int qbits;		/* XXX trsa */
    int badok;		/* XXX trsa */
    int err;

    void * digest;
    size_t digestlen;

    poptContext con;
    ARGV_t av;
    int ac;
    FILE *fp;

    int enabled;

    unsigned pkcsv15;
    unsigned ownflag;
    unsigned allkeys;
    unsigned keephandle;
    unsigned oldversion;
    unsigned zeroauth;
    unsigned addversion;
    unsigned use_long;
    unsigned use_struct;
    unsigned inc;
    unsigned activate;
    unsigned use_ca;
    unsigned v12;
    unsigned disable;
    unsigned deactivated;

    int ix;
    unsigned keysize;
    unsigned exponent;
    unsigned per1;
    unsigned per2;
    unsigned familyID;
    unsigned migscheme;
    int mode;
    int ordinal;
    unsigned audit;
    unsigned restype;
    unsigned cap;
    unsigned scap;
    unsigned bitname;
    unsigned bitvalue;
    unsigned size;
    unsigned offset;
    int expectederror;
    unsigned val;
    unsigned type;

    char *ic_str;;
    ARGV_t av_ix;
    char *label;

    unsigned char *b;
    uint32_t nb;

    char * ifn;
    char * ofn;
    char * kfn;
    char * sfn;
    char * msafn;

    char * ownerpass;
    char * keypass;
    char * parpass;
    char * certpass;
    char * newpass;
    char * areapass;
    char * sigpass;
    char * migpass;
    char * datpass;

    unsigned char * pwdo;
    unsigned char * pwdk;
    unsigned char * pwdp;
    unsigned char * pwdc;
    unsigned char * pwdn;
    unsigned char * pwda;
    unsigned char * pwds;
    unsigned char * pwdm;
    unsigned char * pwdd;

    unsigned char pwdohash[20];
    unsigned char pwdkhash[20];
    unsigned char pwdphash[20];
    unsigned char pwdchash[20];
    unsigned char pwdnhash[20];
    unsigned char pwdahash[20];
    unsigned char pwdshash[20];
    unsigned char pwdmhash[20];
    unsigned char pwddhash[20];

    uint32_t keyhandle;
    uint32_t parhandle;
    uint32_t certhandle;
    uint32_t sighandle;
    uint32_t mighandle;
    uint32_t handle;

    char *hk_str;
    char *hp_str;
    char *hc_str;
    char *hs_str;
    char *hm_str;
    char *ha_str;
    char *ix_str;

    char *cap_str;
    char *scap_str;
    char *scapd_str;

    char *per1_str;
    char *per2_str;
    char *es_str;

    char *bm_str;
    uint32_t restrictions;

    char *kt_str;
    char keytype;

#if defined(__LCLINT__)
/*@refs@*/
    int nrefs;			/*!< (unused) keep splint happy */
#endif
};

extern struct rpmtpm_s __tpm;
extern rpmtpm _tpm;

#endif	/* _RPMTPM_INTERNAL */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Unreference a TPM wrapper instance.
 * @param tpm		TPM wrapper
 * @return		NULL on last dereference
 */
/*@unused@*/ /*@null@*/
rpmtpm rpmtpmUnlink (/*@killref@*/ /*@only@*/ /*@null@*/ rpmtpm tpm)
	/*@modifies tpm @*/;
#define	rpmtpmUnlink(_tpm)	\
    ((rpmtpm)rpmioUnlinkPoolItem((rpmioItem)(_tpm), __FUNCTION__, __FILE__, __LINE__))

/**
 * Reference a TPM wrapper instance.
 * @param tpm		TPM wrapper
 * @return		new TPM wrapper reference
 */
/*@unused@*/ /*@newref@*/ /*@null@*/
rpmtpm rpmtpmLink (/*@null@*/ rpmtpm tpm)
	/*@modifies tpm @*/;
#define	rpmtpmLink(_tpm)	\
    ((rpmtpm)rpmioLinkPoolItem((rpmioItem)(_tpm), __FUNCTION__, __FILE__, __LINE__))

/**
 * Destroy a TPM wrapper.
 * @param tpm		TPM wrapper
 * @return		NULL on last dereference
 */
/*@null@*/
rpmtpm rpmtpmFree(/*@killref@*/ /*@null@*/rpmtpm tpm)
	/*@globals fileSystem @*/
	/*@modifies tpm, fileSystem @*/;
#define	rpmtpmFree(_tpm)	\
    ((rpmtpm)rpmioFreePoolItem((rpmioItem)(_tpm), __FUNCTION__, __FILE__, __LINE__))

/**
 * Create and load a TPM wrapper.
 * @param ac		TPM arg count
 * @param av		TPM args
 * @param tbl		TPM option table
 * @param flags		TPM flags
 * @return		new TPM wrapper
 */
/*@newref@*/ /*@null@*/
rpmtpm rpmtpmNew(int ac, char ** av, struct poptOption *tbl, uint32_t flags)
	/*@globals fileSystem, internalState @*/
	/*@modifies fileSystem, internalState @*/;

int rpmtpmErr(rpmtpm tpm, const char * msg, uint32_t mask, uint32_t rc)
	/*@*/;
void rpmtpmDump(rpmtpm tpm, const char * msg, unsigned char * b, size_t nb)
	/*@*/;

#ifdef __cplusplus
}
#endif

#endif	/* H_RPMTPM */
