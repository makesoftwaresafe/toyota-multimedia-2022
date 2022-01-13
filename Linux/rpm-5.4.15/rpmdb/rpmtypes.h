#ifndef _H_RPMTYPES_
#define	_H_RPMTYPES_

/**
 * \file rpmdb/rpmtypes.h
 */

/** \ingroup rpmts
 * The RPM Transaction Set.
 * Transaction sets are inherently unordered! RPM may reorder transaction
 * sets to reduce errors. In general, installs/upgrades are done before
 * strict removals, and prerequisite ordering is done on installs/upgrades.
 */
typedef /*@abstract@*/ /*@refcounted@*/ struct rpmts_s * rpmts;

/** \ingroup rpmts
 * An added/available package retrieval key.
 */
typedef /*@abstract@*/ void * alKey;

/** \ingroup rpmbuild
 */
typedef /*@abstract@*/ /*@refcounted@*/ struct Spec_s * Spec;

/** \ingroup rpmds 
 * Dependency tag sets from a header, so that a header can be discarded early.
 */
typedef /*@abstract@*/ /*@refcounted@*/ struct rpmds_s * rpmds;

/** \ingroup rpmds 
 * Container for commonly extracted dependency set(s).
 */
typedef struct rpmPRCO_s * rpmPRCO;

/** \ingroup rpmte
 * An element of a transaction set, i.e. a TR_ADDED or TR_REMOVED package.
 */
typedef /*@abstract@*/ struct rpmte_s * rpmte;

/** \ingroup rpmdb
 * Database of headers and tag value indices.
 */
typedef /*@abstract@*/ /*@refcounted@*/ struct rpmdb_s * rpmdb;

/** \ingroup rpmdb
 * Database iterator.
 */
typedef /*@abstract@*/ struct rpmmi_s * rpmmi;

/** \ingroup rpmgi
 * Generalized iterator.
 */
typedef /*@abstract@*/ /*@refcounted@*/ struct rpmgi_s * rpmgi;

/**
 */
typedef struct rpmRelocation_s * rpmRelocation;

/**
 * Scriptlet identifiers.
 */
typedef enum rpmScriptID_e {
    RPMSCRIPT_PRETRANS		=  0,	/*!< %pretrans scriptlet */
    RPMSCRIPT_TRIGGERPREIN	=  1,	/*!< %triggerprein scriptlet */
    RPMSCRIPT_PREIN		=  2,	/*!< %pre scriptlet */
    RPMSCRIPT_POSTIN		=  3,	/*!< %post scriptlet */
    RPMSCRIPT_TRIGGERIN		=  4,	/*!< %triggerin scriptlet */
    RPMSCRIPT_TRIGGERUN		=  5,	/*!< %triggerun scriptlet */
    RPMSCRIPT_PREUN		=  6,	/*!< %preun scriptlet */
    RPMSCRIPT_POSTUN		=  7,	/*!< %postun scriptlet */
    RPMSCRIPT_TRIGGERPOSTUN	=  8,	/*!< %triggerpostun scriptlet */
    RPMSCRIPT_POSTTRANS		=  9,	/*!< %posttrans scriptlet */
	/* 10-15 unused */
    RPMSCRIPT_VERIFY		= 16,	/*!< %verify scriptlet */
    RPMSCRIPT_SANITYCHECK	= 17,	/*!< %sanitycheck scriptlet */
	/* 18-23 unused */
    RPMSCRIPT_PREP		= 24,	/*!< %prep build scriptlet */
    RPMSCRIPT_BUILD		= 25,	/*!< %build build scriptlet */
    RPMSCRIPT_INSTALL		= 26,	/*!< %install build scriptlet */
    RPMSCRIPT_CHECK		= 27,	/*!< %check build scriptlet */
	/* 28-31 unused */
    RPMSCRIPT_MAX		= 32
} rpmScriptID;

/**
 * Scriptlet states (when installed).
 */
typedef enum rpmScriptState_e {
    RPMSCRIPT_STATE_UNKNOWN	= 0,
	/* 0-15 reserved for waitpid return. */
    RPMSCRIPT_STATE_EXEC	= (1 << 16), /*!< scriptlet was exec'd */
    RPMSCRIPT_STATE_REAPED	= (1 << 17), /*!< scriptlet was reaped */
	/* 18-23 unused */
    RPMSCRIPT_STATE_SELINUX	= (1 << 24), /*!< scriptlet exec by SELinux */
    RPMSCRIPT_STATE_EMBEDDED	= (1 << 25) /*!< scriptlet exec by lua et al */
	/* 26-31 unused */
} rpmScriptState;

#endif /* _H_RPMTYPES_ */
