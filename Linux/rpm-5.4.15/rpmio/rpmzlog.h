#ifndef _H_RPMZLOG_
#define _H_RPMZLOG_

/** \ingroup rpmio
 * \file rpmio/rpmzlog.h
 */
#include <stdio.h>
#include <sys/time.h>

/** trace log pointer */
typedef /*@abstract@*/ /*@refcounted@*/ struct rpmzLog_s * rpmzLog;

#ifdef	_RPMZLOG_INTERNAL
#include <yarn.h>

/** trace msg pointer */
typedef /*@abstract@*/ struct rpmzMsg_s * rpmzMsg;

/** trace msg */
struct rpmzMsg_s {
    struct timeval when;	/* time of entry */
    char *msg;			/* message */
    rpmzMsg next;		/* next entry */
};

/** trace log */
struct rpmzLog_s {
    struct rpmioItem_s _item;	/*!< usage mutex and pool identifier. */
    struct timeval start;	/*!< starting time of day for tracing */
/*@null@*/
    rpmzMsg msg_head;
/*@shared@*/ /*@relnull@*/
    rpmzMsg *msg_tail;
    int msg_count;
#if defined(__LCLINT__)
/*@refs@*/
    int nrefs;			/*!< (unused) keep splint happy */
#endif
};
#endif	/* _RPMZLOG_INTERNAL */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Reference the log data.
 */
/*@newref@*/ /*@null@*/
rpmzLog rpmzLogLink(/*@null@*/ rpmzLog zlog)
	/*@globals fileSystem, internalState @*/
	/*@modifies zlog, fileSystem, internalState @*/;

/**
 * Set up log (call from main thread before other threads launched).
 */
/*@newref@*/
rpmzLog rpmzLogNew(/*@null@*/ struct timeval *tv)
	/*@globals fileSystem, internalState @*/
	/*@modifies fileSystem, internalState @*/;

/**
 * Add entry to trace log.
 */
void rpmzLogAdd(/*@null@*/ rpmzLog zlog, const char *fmt, ...)
	/*@globals fileSystem, internalState @*/
	/*@modifies zlog, fileSystem, internalState @*/;

/**
 * Release a reference to the log data.
 */
/*@null@*/
rpmzLog rpmzLogFree(/*@killref@*/ /*@null@*/ rpmzLog zlog)
	/*@globals fileSystem, internalState @*/
	/*@modifies zlog, fileSystem, internalState @*/;

/**
 * Show entries until no more, free log.
 */
/*@null@*/
rpmzLog rpmzLogDump(/*@killref@*/ /*@null@*/ rpmzLog zlog, /*@null@*/ FILE * fp)
	/*@globals fileSystem, internalState @*/
	/*@modifies zlog, *fp, fileSystem, internalState @*/;

#ifdef __cplusplus
}
#endif

#endif /* _H_RPMZLOG_ */
