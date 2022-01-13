#ifndef H_RPMNS
#define H_RPMNS

/** \ingroup rpmds
 * \file rpmdb/rpmns.h
 * Structure(s) and routine(s) used for classifying and parsing names.
 */

/**
 */
/*@-exportlocal@*/
/*@unchecked@*/
extern int _rpmns_debug;
/*@=exportlocal@*/

/*@unchecked@*/ /*@observer@*/ /*@relnull@*/
extern const char *_rpmns_N_at_A;

typedef	/*@abstract@*/ struct rpmns_s * rpmns;

/**
 * Dependency types
 */
typedef enum nsType_e {
    RPMNS_TYPE_UNKNOWN	=  0,
    RPMNS_TYPE_STRING	=  (1 <<  0),	/*!< unclassified string */
    RPMNS_TYPE_PATH	=  (1 <<  1),	/*!< /bin */
    RPMNS_TYPE_DSO	=  (1 <<  2),	/*!< libc.so.6 */
    RPMNS_TYPE_FUNCTION	=  (1 <<  3),	/*!< %{foo} */
    RPMNS_TYPE_ARCH	=  (1 <<  4),	/*!< foo.arch */
    RPMNS_TYPE_VERSION	=  (1 <<  5),	/*!< foo-1.2.3-bar */
    RPMNS_TYPE_COMPOUND	=  (1 <<  6),	/*!< foo.bar */
	/* 7 unused */
    RPMNS_TYPE_NAMESPACE=  (1 <<  8),	/*!< foo(bar) */
    RPMNS_TYPE_RPMLIB	=  (1 <<  9),	/*!< rpmlib(bar) */
    RPMNS_TYPE_CPUINFO	=  (1 << 10),	/*!< cpuinfo(bar) */
    RPMNS_TYPE_GETCONF	=  (1 << 11),	/*!< getconf(bar) */
    RPMNS_TYPE_UNAME	=  (1 << 12),	/*!< uname(bar) */
    RPMNS_TYPE_SONAME	=  (1 << 13),	/*!< soname(bar) */
    RPMNS_TYPE_ACCESS	=  (1 << 14),	/*!< exists(bar) */
    RPMNS_TYPE_TAG	=  (1 << 15),	/*!< Tag(bar) */
    RPMNS_TYPE_USER	=  (1 << 16),	/*!< user(bar) */
    RPMNS_TYPE_GROUP	=  (1 << 17),	/*!< group(bar) */
    RPMNS_TYPE_MOUNTED	=  (1 << 18),	/*!< mounted(/path) */
    RPMNS_TYPE_DISKSPACE=  (1 << 19),	/*!< diskspace(/path) */
    RPMNS_TYPE_DIGEST	=  (1 << 20),	/*!< digest(/path) = hex */
    RPMNS_TYPE_GNUPG	=  (1 << 21),	/*!< gnupg(/path/file.asc) */
    RPMNS_TYPE_MACRO	=  (1 << 22),	/*!< macro(foo) */
    RPMNS_TYPE_ENVVAR	=  (1 << 23),	/*!< envvar(foo) */
    RPMNS_TYPE_RUNNING	=  (1 << 24),	/*!< running(foo) */
    RPMNS_TYPE_SANITY	=  (1 << 25),	/*!< sanitycheck(foo) */
    RPMNS_TYPE_VCHECK	=  (1 << 26),	/*!< vcheck(foo) */
    RPMNS_TYPE_SIGNATURE=  (1 << 27),	/*!< signature(/text:/sig) = /pub:id */
    RPMNS_TYPE_VERIFY	=  (1 << 28),	/*!< verify(N) = E:V-R */
    RPMNS_TYPE_CONFIG	=  (1 << 29),	/*!< config(N) = E:V-R */
    RPMNS_TYPE_SET	=  (1 << 30),	/*!< set(N) = E:V-R */
    RPMNS_TYPE_DPKG	=  (1 << 31)	/*!< dpkg(N) = E:V-R */
} nsType;

#if defined(_RPMNS_INTERNAL)
/** \ingroup rpmds
 * An NS parsing container.
 */
struct rpmns_s {
/*@owned@*/
    const char * str;		/*!< string storage */
    nsType Type;		/*!< Type */
/*@dependent@*/ /*@null@*/
    const char * NS;		/*!< Namespace */
/*@dependent@*/ /*@relnull@*/
    const char * N;		/*!< Name */
/*@dependent@*/ /*@null@*/
    const char * A;		/*!< Arch */
    evrFlags Flags;		/*!< EVR comparison flags. */
};
#endif	/* _RPMNS_INTERNAL */

#ifdef __cplusplus
extern "C" {
#endif

/** \ingroup rpmds
 * Is string a known arch suffix?
 * @param str		string
 * @return		RPMNS_TYPE_ARCH if known arch, else RPMNS_TYPE_UNKNOWN
 */
nsType rpmnsArch(const char * str)
	/*@*/;

/** \ingroup rpmds
 * Is string a known probe namespace?
 * @param s		string
 * @param slen		string length (0 uses strlen(s))
 * @return		nsType if known probe, else RPMNS_TYPE_UNKNOWN
 */
nsType rpmnsProbe(const char * s, size_t slen)
	/*@*/;

/** \ingroup rpmds
 * Classify a string as a dependency type.
 * @param s		string like "bing(bang).boom"
 * @param slen		string length (0 uses strlen(s))
 * @return		dependency type
 */
nsType rpmnsClassify(const char * s, size_t slen)
	/*@*/;

/** \ingroup rpmds
 * Expand and split NS(N).A string into namespace, name and arch components.
 * @param str		string like "bing(bang).boom"
 * @retval *ns		parse results
 * @return		0 always
 */
int rpmnsParse(const char * str, rpmns ns)
	/*@globals rpmGlobalMacroContext, h_errno, internalState @*/
	/*@modifies ns, rpmGlobalMacroContext, internalState @*/;

/** \ingroup rpmns
 * Clean global name space dependency sets.
 */
void rpmnsClean(void)
	/*@globals internalState @*/
	/*@modifies internalState @*/;

/** \ingroup rpmns
 * Verify OpenPGP signature on a file.
 * @param _ts		transaction set
 * @param fn		plaintext (or clearsign) file
 * @param sigfn		binary/pem encoded signature file (NULL iff clearsign)
 * @param pubfn		binary/pem encoded pubkey file (NULL uses rpmdb keyring)
 * @param pubid		pubkey fingerprint hex string (NULL disables check)
 * @param flags		(unused)
 * @return		RPMRC_OK if verified, RPMRC_FAIL if not verified
 */
rpmRC rpmnsProbeSignature(void * _ts, const char * fn,
		/*@null@*/ const char * sigfn,
		/*@null@*/ const char * pubfn,
		/*@null@*/ const char * pubid,
		int flags)
	/*@globals rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
	/*@modifies _ts, rpmGlobalMacroContext, fileSystem, internalState @*/;

#ifdef __cplusplus
}
#endif

#endif	/* H_RPMNS */
