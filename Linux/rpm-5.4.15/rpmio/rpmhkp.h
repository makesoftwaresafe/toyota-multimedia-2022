#ifndef RPMHKP_H
#define RPMHKP_H

/** \ingroup rpmio
 * \file rpmio/rpmhkp.h
 */

#include <rpmiotypes.h>
#include <rpmio.h>

typedef /*@abstract@*/ /*@refcounted@*/ struct rpmhkp_s * rpmhkp;

/*@unchecked@*/
extern int _rpmhkp_debug;

/*@unchecked@*/
extern rpmhkp _rpmhkpI;

#if defined(_RPMHKP_INTERNAL)
#include <rpmbf.h>
struct _filter_s {
    rpmbf bf;
    size_t n;
    double e;
    size_t m;
    size_t k;
};
extern struct _filter_s _rpmhkp_awol;
extern struct _filter_s _rpmhkp_crl;

extern int _rpmhkp_lvl;

struct rpmhkp_s {
    struct rpmioItem_s _item;	/*!< usage mutex and pool identifier. */
    rpmuint8_t * pkt;
    size_t pktlen;
    rpmuint8_t ** pkts;
    int npkts;

    int pubx;
    int uidx;
    int subx;
    int sigx;

    rpmuint8_t keyid[8];
    rpmuint8_t subid[8];
    rpmuint8_t signid[8];	/* XXX replaces ts->pksignid */
    rpmuint8_t goop[6];

    rpmuint32_t tvalid;
    int uvalidx;

    rpmbf awol;
    rpmbf crl;

#if defined(__LCLINT__)
/*@refs@*/
    int nrefs;			/*!< (unused) keep splint happy */
#endif
};
#endif /* _RPMHKP_INTERNAL */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Unreference a hkp handle instance.
 * @param hkp		hkp handle
 * @return		NULL on last dereference
 */
/*@unused@*/ /*@null@*/
rpmhkp rpmhkpUnlink (/*@killref@*/ /*@only@*/ /*@null@*/ rpmhkp hkp)
	/*@modifies hkp @*/;
#define	rpmhkpUnlink(_hkp)	\
    ((rpmhkp)rpmioUnlinkPoolItem((rpmioItem)(_hkp), __FUNCTION__, __FILE__, __LINE__))

/**
 * Reference a hkp handle instance.
 * @param hkp		hkp handle
 * @return		new hkp handle reference
 */
/*@unused@*/ /*@newref@*/ /*@null@*/
rpmhkp rpmhkpLink (/*@null@*/ rpmhkp hkp)
	/*@modifies hkp @*/;
#define	rpmhkpLink(_hkp)	\
    ((rpmhkp)rpmioLinkPoolItem((rpmioItem)(_hkp), __FUNCTION__, __FILE__, __LINE__))

/**
 * Destroy a hkp handle.
 * @param hkp		hkp handle
 * @return		NULL on last dereference
 */
/*@null@*/
rpmhkp rpmhkpFree(/*@killref@*/ /*@null@*/rpmhkp hkp)
	/*@globals fileSystem @*/
	/*@modifies hkp, fileSystem @*/;
#define	rpmhkpFree(_hkp)	\
    ((rpmhkp)rpmioFreePoolItem((rpmioItem)(_hkp), __FUNCTION__, __FILE__, __LINE__))

/**
 * Create a new hkp handle.
 * @param keyid		pubkey fingerprint (or NULL)
 * @param flags		hkp handle flags ((1<<31): use global handle)
 * @return		new hkp handle
 */
/*@newref@*/ /*@null@*/
rpmhkp rpmhkpNew(/*@null@*/ const rpmuint8_t * keyid, uint32_t flags)
	/*@globals fileSystem, internalState @*/
	/*@modifies fileSystem, internalState @*/;

/**
 * Retrieve a pubkey from a SKS server.
 * @param keyname	pubkey query string
 * @return		hkp handle
 */
rpmhkp rpmhkpLookup(const char * keyname)
	/*@*/;

/**
 * Retrieve/Validate binding and certification signatures on a pubkey.
 * @param hkp		hkp handle
 * @param keyname	pubkey query string
 * @return		OK/NOTFOUND/FAIL/UNTRUSTED (NOKEY? NOSIG?)
 */
rpmRC rpmhkpValidate(/*@null@*/ rpmhkp hkp, /*@null@*/ const char * keyname)
	/*@*/;

#if defined(_RPMHKP_INTERNAL)
/**
 * Load values into pubkey params from packet.
 * @param hkp		hkp handle
 * @param dig		pubkey/signature container
 * @param keyx		index of pubkey in packet array
 * @param pubkey_algo	openpgp pubkey algorithm 
 * @return		0 on success, -1 on failure
 */
int rpmhkpLoadKey(rpmhkp hkp, pgpDig dig,
                int keyx, rpmuint8_t pubkey_algo)
	/*@*/;

/**
 * Copy values into signature params.
 * @param hkp		hkp handle
 * @param dig		pubkey/signature container
 * @param pp		openpgp signature packet
 * @return		0 on success, -1 on failure
 */
int rpmhkpLoadSignature(/*@null@*/ rpmhkp hkp, pgpDig dig, pgpPkt pp)
	/*@*/;

/**
 * Retrieve/Load the pubkey associated with a signature.
 * @param hkp		hkp handle
 * @param dig		pubkey/signature container
 * @param signid	signature key id
 * @param pubkey_algo	signature key id
 * @return		key index
 */
int rpmhkpFindKey(rpmhkp hkp, pgpDig dig,
                const rpmuint8_t * signid, rpmuint8_t pubkey_algo)
	/*@*/;

/**
 * Display pubkey/signature parameters in dig container.
 * @param msg		identifier message
 * @param sigp		signature container
 * @param fp		file handle (NULL uses stderr)
 */
void _rpmhkpDumpDigParams(const char * msg, pgpDigParams sigp,
		/*@null@*/ FILE * fp)
	/*@*/;

/**
 * Display dig container.
 * @param msg		identifier message
 * @param dig		pubkey/signature container
 * @param fp		file handle (NULL uses stderr)
 */
void _rpmhkpDumpDig(const char * msg, pgpDig dig,
		/*@null@*/ FILE * fp)
	/*@*/;

/**
 * Update a digest with data.
 * @param ctx		digest context
 * @param data		data to add to digest
 * @param len		no. bytes of data to add to digest
 * @return		0 on success
 */
int rpmhkpUpdate(/*@null@*/ DIGEST_CTX ctx, const void * data, size_t len)
	/*@*/;
#endif /* _RPMHKP_INTERNAL */

/**
 * Display hkp usage statistics.
 * @param fp		file handle (NULL uses stderr)
 */
void _rpmhkpPrintStats(/*@null@*/ FILE * fp)
	/*@*/;

#ifdef __cplusplus
}
#endif

#endif /* RPMHKP_H */
