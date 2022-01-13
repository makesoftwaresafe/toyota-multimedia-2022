#ifndef H_RPMEVR
#define H_RPMEVR

/** \ingroup rpmds
 * \file rpmdb/rpmevr.h
 * Structure(s) and routine(s) used for EVR parsing and comparison.
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 */
/*@-exportlocal@*/
/*@unchecked@*/
extern int _rpmevr_debug;
/*@=exportlocal@*/

typedef	/*@abstract@*/ struct EVR_s * EVR_t;

/**
 * Dependency Attributes.
 */
/*@-matchfields@*/
typedef enum evrFlags_e {
#if defined(_RPMEVR_INTERNAL)
    RPMSENSE_ANY	= 0,
/*@-enummemuse@*/
    RPMSENSE_SERIAL	= (1 << 0),	/*!< (obsolete). */
/*@=enummemuse@*/
#endif
    RPMSENSE_LESS	= (1 << 1),
    RPMSENSE_GREATER	= (1 << 2),
    RPMSENSE_EQUAL	= (1 << 3),
#if defined(_RPMEVR_INTERNAL)
    RPMSENSE_PROVIDES	= (1 << 4), /* only used internally by builds */
    RPMSENSE_CONFLICTS	= (1 << 5), /* only used internally by builds */
#endif
    RPMSENSE_PREREQ	= (1 << 6),	/*!< (obsolete). */
#if defined(_RPMEVR_INTERNAL)
    RPMSENSE_OBSOLETES	= (1 << 7), /* only used internally by builds */
    RPMSENSE_INTERP	= (1 << 8),	/*!< Interpreter used by scriptlet. */
    RPMSENSE_SCRIPT_PRE	= (1 << 9),	/*!< %pre dependency. */
    RPMSENSE_SCRIPT_POST = (1 << 10),	/*!< %post dependency. */
    RPMSENSE_SCRIPT_PREUN = (1 << 11),	/*!< %preun dependency. */
    RPMSENSE_SCRIPT_POSTUN = (1 << 12), /*!< %postun dependency. */
    RPMSENSE_SCRIPT_VERIFY = (1 << 13),	/*!< %verify dependency. */
    RPMSENSE_FIND_REQUIRES = (1 << 14), /*!< find-requires dependency. */
    RPMSENSE_FIND_PROVIDES = (1 << 15), /*!< find-provides dependency. */

    RPMSENSE_TRIGGERIN	= (1 << 16),	/*!< %triggerin dependency. */
    RPMSENSE_TRIGGERUN	= (1 << 17),	/*!< %triggerun dependency. */
    RPMSENSE_TRIGGERPOSTUN = (1 << 18),	/*!< %triggerpostun dependency. */
    RPMSENSE_MISSINGOK	= (1 << 19),	/*!< suggests/enhances hint. */
    RPMSENSE_SCRIPT_PREP = (1 << 20),	/*!< %prep build dependency. */
    RPMSENSE_SCRIPT_BUILD = (1 << 21),	/*!< %build build dependency. */
    RPMSENSE_SCRIPT_INSTALL = (1 << 22),/*!< %install build dependency. */
    RPMSENSE_SCRIPT_CLEAN = (1 << 23),	/*!< %clean build dependency. */
    RPMSENSE_RPMLIB = (1 << 24),	/*!< rpmlib(feature) dependency. */
    RPMSENSE_TRIGGERPREIN = (1 << 25),	/*!< %triggerprein dependency. */
    RPMSENSE_KEYRING	= (1 << 26),
    RPMSENSE_STRONG	= (1 << 27),	/*!< placeholder SuSE */
    RPMSENSE_CONFIG	= (1 << 28),
    RPMSENSE_PROBE	= (1 << 29),
    RPMSENSE_PACKAGE	= (1 << 30),
    RPMSENSE_SCRIPT_SANITYCHECK = (1 << 31) /*!< %sanitycheck dependency. */
#endif
} evrFlags;
/*@=matchfields@*/

typedef enum evrFlags_e rpmsenseFlags;

#define	RPMSENSE_SENSEMASK	\
	(RPMSENSE_EQUAL | RPMSENSE_GREATER | RPMSENSE_LESS)
#define	RPMSENSE_NOTEQUAL	\
	(RPMSENSE_EQUAL ^ RPMSENSE_SENSEMASK)

#if defined(_RPMEVR_INTERNAL)
/** \ingroup rpmds
 * An EVR parsing container.
 */
struct EVR_s {
/*@owned@*/
    const char * str;		/*!< EVR storage */
#ifndef	DYING
    unsigned long Elong;	/*!< E converted to integer. */
#endif
    evrFlags Flags;		/*!< EVR comparison flags. */
    const char * F[6];		/*!< Fields: (\\1=E, \\2=V, \\3=T, \\4=R, \\5=D). */
#define	RPMEVR_E	1
#define	RPMEVR_V	2
#define	RPMEVR_T	3
#define	RPMEVR_R	4
#define	RPMEVR_D	5
};

#define	RPMSENSE_TRIGGER	\
	(RPMSENSE_TRIGGERPREIN | RPMSENSE_TRIGGERIN | RPMSENSE_TRIGGERUN | RPMSENSE_TRIGGERPOSTUN)

#define	_ALL_REQUIRES_MASK	(\
    RPMSENSE_INTERP | \
    RPMSENSE_SCRIPT_PRE | \
    RPMSENSE_SCRIPT_POST | \
    RPMSENSE_SCRIPT_PREUN | \
    RPMSENSE_SCRIPT_POSTUN | \
    RPMSENSE_SCRIPT_VERIFY | \
    RPMSENSE_FIND_REQUIRES | \
    RPMSENSE_MISSINGOK | \
    RPMSENSE_SCRIPT_PREP | \
    RPMSENSE_SCRIPT_BUILD | \
    RPMSENSE_SCRIPT_INSTALL | \
    RPMSENSE_SCRIPT_CLEAN | \
    RPMSENSE_RPMLIB | \
    RPMSENSE_KEYRING | \
    RPMSENSE_PACKAGE )

#define	_notpre(_x)		((_x) & ~RPMSENSE_PREREQ)
#define	_INSTALL_ONLY_MASK \
    _notpre(RPMSENSE_SCRIPT_PRE|RPMSENSE_SCRIPT_POST|RPMSENSE_RPMLIB|RPMSENSE_KEYRING)
#define	_ERASE_ONLY_MASK  \
    _notpre(RPMSENSE_SCRIPT_PREUN|RPMSENSE_SCRIPT_POSTUN)

#define	isInstallPreReq(_x)	((_x) & _INSTALL_ONLY_MASK)
#define	isErasePreReq(_x)	((_x) & _ERASE_ONLY_MASK)
#endif	/* _RPMEVR_INTERNAL */

/** \ingroup rpmds
 * Create a new EVR container.
 * @param Flags		EVR inequality flags
 * @param initialize	Should empty defaults be initialized?
 * @return		initialized EVR container
 */
EVR_t rpmEVRnew(uint32_t Flags, int initialize)
        /*@*/;

/** \ingroup rpmds
 * Destroy an EVR container.
 * @param evr		EVR container
 * @return		NULL always
 */
/*@null@*/
EVR_t rpmEVRfree(/*@only@*/ EVR_t evr)
        /*@modifies evr @*/;

/** \ingroup rpmds
 * Segmented string compare.
 * @param a		1st string
 * @param b		2nd string
 * @return		+1 if a is "newer", 0 if equal, -1 if b is "newer"
 */
int rpmEVRcmp(const char *a, const char *b)
	/*@*/;

/** \ingroup rpmds
 * Split EVR string into epoch, version, and release components.
 * @param evrstr	[epoch:]version[-release] string
 * @retval *evr		parse results
 * @return		0 always
 */
int rpmEVRparse(const char * evrstr, EVR_t evr)
	/*@modifies evrstr, evr @*/;

/** \ingroup rpmds
 * Compare EVR containers for equality.
 * @param a		1st EVR container
 * @param b		2nd EVR container
 * @return		+1 if a is "newer", 0 if equal, -1 if b is "newer"
 */
int rpmEVRcompare(const EVR_t a, const EVR_t b)
	/*@*/;

/** \ingroup rpmds
 * Compare EVR containers for overlap.
 * @param a		1st EVR container
 * @param b		2nd EVR container
 * @return		1 if EVR inequalities overlap, 0 otherwise
 */
int rpmEVRoverlap(EVR_t a, EVR_t b)
	/*@*/;

/** \ingroup rpmds
 * Segmented string compare vector.
 * @param a		1st string
 * @param b		2nd string
 * @return		+1 if a is "newer", 0 if equal, -1 if b is "newer"
 */
extern int (*rpmvercmp)(const char *a, const char *b)
	/*@*/;

/**
 * Return comparison operator sense flags.
 * @param op		operator string (NULL or "" uses RPMSENSE_EQUAL)
 * @param *end		pointer to 1st character after operator (or NULL)
 * @return		sense flags
 */
rpmsenseFlags rpmEVRflags(/*@null@*/const char *op, /*@null@*/const char **end)
	/*@modifies *end @*/;

#if defined(_RPMEVR_INTERNAL)
/**
 * Compare headers to determine which header is "newer".
 * @deprecated Use rpmdsCompare instead.
 * @todo	Eliminate in rpm-5.1.
 * @param A		1st header
 * @param B		2nd header
 * @return		result of comparison
 */
int rpmVersionCompare(Header A, Header B)
	/*@globals internalState @*/
	/*@modifies internalState @*/;
#endif

#ifdef __cplusplus
}
#endif

#endif	/* H_RPMEVR */
