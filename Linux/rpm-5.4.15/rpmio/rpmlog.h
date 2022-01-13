#ifndef H_RPMLOG
#define H_RPMLOG 1

/** \ingroup rpmio
 * \file rpmio/rpmlog.h
 * Yet Another syslog(3) API clone.
 */

#include <stdio.h>
#include <stdarg.h>

/**
 * RPM Log levels.
 * priorities/facilities are encoded into a single 32-bit quantity, where the
 * bottom 3 bits are the priority (0-7) and the top 28 bits are the facility
 * (0-big number).  Both the priorities and the facilities map roughly
 * one-to-one to strings in the syslogd(8) source code.  This mapping is
 * included in this file.
 *
 * priorities (these are ordered)
 */
/*@-typeuse@*/
typedef enum rpmlogLvl_e {
    RPMLOG_EMERG	= 0,	/*!< system is unusable */
    RPMLOG_ALERT	= 1,	/*!< action must be taken immediately */
    RPMLOG_CRIT		= 2,	/*!< critical conditions */
    RPMLOG_ERR		= 3,	/*!< error conditions */
    RPMLOG_WARNING	= 4,	/*!< warning conditions */
    RPMLOG_NOTICE	= 5,	/*!< normal but significant condition */
    RPMLOG_INFO		= 6,	/*!< informational */
    RPMLOG_DEBUG	= 7	/*!< debug-level messages */
} rpmlogLvl;
/*@=typeuse@*/

#define	RPMLOG_PRIMASK	0x07	/* mask to extract priority part (internal) */
				/* extract priority */
#define	RPMLOG_PRI(p)	((p) & RPMLOG_PRIMASK)
#define	RPMLOG_MAKEPRI(fac, pri)	((((unsigned)(fac)) << 3) | (pri))

#ifdef RPMLOG_NAMES
#define	_RPMLOG_NOPRI	0x10	/* the "no priority" priority */
				/* mark "facility" */
#define	_RPMLOG_MARK	RPMLOG_MAKEPRI(RPMLOG_NFACILITIES, 0)
typedef struct _rpmcode {
	const char	*c_name;
	int		c_val;
} RPMCODE;

RPMCODE rpmprioritynames[] =
  {
    { "alert",	RPMLOG_ALERT },
    { "crit",	RPMLOG_CRIT },
    { "debug",	RPMLOG_DEBUG },
    { "emerg",	RPMLOG_EMERG },
    { "err",	RPMLOG_ERR },
    { "error",	RPMLOG_ERR },		/* DEPRECATED */
    { "info",	RPMLOG_INFO },
    { "none",	_RPMLOG_NOPRI },	/* INTERNAL */
    { "notice",	RPMLOG_NOTICE },
    { "panic",	RPMLOG_EMERG },		/* DEPRECATED */
    { "warn",	RPMLOG_WARNING },	/* DEPRECATED */
    { "warning",RPMLOG_WARNING },
    { NULL, -1 }
  };
#endif

/**
 * facility codes
 */
/*@-enummemuse -typeuse@*/
typedef	enum rpmlogFac_e {
    RPMLOG_KERN		= (0<<3),	/*!< kernel messages */
    RPMLOG_USER		= (1<<3),	/*!< random user-level messages */
    RPMLOG_MAIL		= (2<<3),	/*!< mail system */
    RPMLOG_DAEMON	= (3<<3),	/*!< system daemons */
    RPMLOG_AUTH		= (4<<3),	/*!< security/authorization messages */
    RPMLOG_SYSLOG	= (5<<3),	/*!< messages generated internally by syslogd */
    RPMLOG_LPR		= (6<<3),	/*!< line printer subsystem */
    RPMLOG_NEWS		= (7<<3),	/*!< network news subsystem */
    RPMLOG_UUCP		= (8<<3),	/*!< UUCP subsystem */
    RPMLOG_CRON		= (9<<3),	/*!< clock daemon */
    RPMLOG_AUTHPRIV	= (10<<3),	/*!< security/authorization messages (private) */
    RPMLOG_FTP		= (11<<3),	/*!< ftp daemon */

	/* other codes through 15 reserved for system use */
    RPMLOG_LOCAL0	= (16<<3),	/*!< reserved for local use */
    RPMLOG_LOCAL1	= (17<<3),	/*!< reserved for local use */
    RPMLOG_LOCAL2	= (18<<3),	/*!< reserved for local use */
    RPMLOG_LOCAL3	= (19<<3),	/*!< reserved for local use */
    RPMLOG_LOCAL4	= (20<<3),	/*!< reserved for local use */
    RPMLOG_LOCAL5	= (21<<3),	/*!< reserved for local use */
    RPMLOG_LOCAL6	= (22<<3),	/*!< reserved for local use */
    RPMLOG_LOCAL7	= (23<<3),	/*!< reserved for local use */

#define	RPMLOG_NFACILITIES 24	/*!< current number of facilities */
    RPMLOG_ERRMSG	= (((unsigned)(RPMLOG_NFACILITIES+0))<<3)
} rpmlogFac;
/*@=enummemuse =typeuse@*/

#define	RPMLOG_FACMASK	0x03f8	/*!< mask to extract facility part */
#define	RPMLOG_FAC(p)	(((p) & RPMLOG_FACMASK) >> 3)


#ifdef RPMLOG_NAMES
RPMCODE facilitynames[] =
  {
    { "auth",	RPMLOG_AUTH },
    { "authpriv",RPMLOG_AUTHPRIV },
    { "cron",	RPMLOG_CRON },
    { "daemon",	RPMLOG_DAEMON },
    { "ftp",	RPMLOG_FTP },
    { "kern",	RPMLOG_KERN },
    { "lpr",	RPMLOG_LPR },
    { "mail",	RPMLOG_MAIL },
    { "mark",	_RPMLOG_MARK },		/* INTERNAL */
    { "news",	RPMLOG_NEWS },
    { "security",RPMLOG_AUTH },		/* DEPRECATED */
    { "syslog",	RPMLOG_SYSLOG },
    { "user",	RPMLOG_USER },
    { "uucp",	RPMLOG_UUCP },
    { "local0",	RPMLOG_LOCAL0 },
    { "local1",	RPMLOG_LOCAL1 },
    { "local2",	RPMLOG_LOCAL2 },
    { "local3",	RPMLOG_LOCAL3 },
    { "local4",	RPMLOG_LOCAL4 },
    { "local5",	RPMLOG_LOCAL5 },
    { "local6",	RPMLOG_LOCAL6 },
    { "local7",	RPMLOG_LOCAL7 },
    { NULL, -1 }
  };
#endif

/*
 * arguments to setlogmask.
 */
#define	RPMLOG_MASK(pri) (1 << ((unsigned)(pri)))	/*!< mask for one priority */
#define	RPMLOG_UPTO(pri) ((1 << (((unsigned)(pri))+1)) - 1)	/*!< all priorities through pri */

/*
 * Option flags for openlog.
 *
 * RPMLOG_ODELAY no longer does anything.
 * RPMLOG_NDELAY is the inverse of what it used to be.
 */
#define	RPMLOG_PID	0x01	/*!< log the pid with each message */
#define	RPMLOG_CONS	0x02	/*!< log on the console if errors in sending */
#define	RPMLOG_ODELAY	0x04	/*!< delay open until first syslog() (default) */
#define	RPMLOG_NDELAY	0x08	/*!< don't delay open */
#define	RPMLOG_NOWAIT	0x10	/*!< don't wait for console forks: DEPRECATED */
#define	RPMLOG_PERROR	0x20	/*!< log to stderr as well */

/**
 */
typedef /*@abstract@*/ struct rpmlogRec_s * rpmlogRec;

/**
 */
typedef /*@abstract@*/ void * rpmlogCallbackData;

/**
  * @param rec		rpmlog record
  * @param data		private callback data
  * @return		flags to define further behavior:
  * 			RPMLOG_DEFAULT to continue to default logger,
  * 			RPMLOG_EXIT to exit after processing
  *			0 to return after callback.
  */
typedef int (*rpmlogCallback) (rpmlogRec rec, rpmlogCallbackData data)
	/*@*/;

/**
 * Option flags for callback return value.
 */
#define RPMLOG_DEFAULT	0x01	/*!< perform default logging */	
#define RPMLOG_EXIT	0x02	/*!< exit after logging */

#if defined(_RPMLOG_INTERNAL)
/**
 */
struct rpmlogRec_s {
    int		code;
    rpmlogLvl	pri;		/* priority */
/*@owned@*/ /*@relnull@*/
    const char * message;
};
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Return translated prefix string (if any) given log level.
 * @param pri		log priority
 * @return		message prefix (or "" for none)
 */
/*@observer@*/
const char * rpmlogLevelPrefix(rpmlogLvl pri)
	/*@*/;

/**
 * Set rpmlog callback function.
 * @param cb		rpmlog callback function
 * @param data		callback private (user) data
 * @return		previous rpmlog callback function
 */
rpmlogCallback rpmlogSetCallback(rpmlogCallback cb, rpmlogCallbackData data)
	/*@globals internalState@*/
	/*@modifies internalState @*/;

/**
 * Get rpmlog callback function and data.
 * @param cb		pointer to rpmlog callback function
 * @param data		pointer to callback private (user) data
 * @return		none
 */
void rpmlogGetCallback(rpmlogCallback *cb, rpmlogCallbackData *data)
	/*@globals internalState @*/
	/*@modifies *cb, *data, internalState @*/;

/**
 * Return number of messages.
 * @return		number of messages
 */
int rpmlogGetNrecs(void)
	/*@*/;

/**
 * Retrieve log message string from rpmlog record
 * @param rec		rpmlog record
 * @return		log message
 */
/*@observer@*/ /*@retexpose@*/ /*@null@*/
const char * rpmlogRecMessage(/*@null@*/ rpmlogRec rec)
	/*@*/;

/**
 * Retrieve log priority from rpmlog record
 * @param rec		rpmlog record
 * @return		log priority
 */
rpmlogLvl rpmlogRecPriority(/*@null@*/ rpmlogRec rec)
	/*@*/;

/**
 * Print all rpmError() messages.
 * @param f		file handle (NULL uses stderr)
 */
void rpmlogPrint(/*@null@*/ FILE *f)
	/*@modifies *f @*/;

/**
 * Close desriptor used to write to system logger.
 * @todo Implement.
 */
/*@unused@*/
void rpmlogClose (void)
	/*@globals internalState@*/
	/*@modifies internalState @*/;

/**
 * Open connection to system logger.
 * @todo Implement.
 */
/*@unused@*/
void rpmlogOpen (const char * ident, int option, int facility)
	/*@globals internalState@*/
	/*@modifies internalState @*/;

/**
 * Set the log mask level.
 * @param mask		log mask (0 is no operation)
 * @return		previous log mask
 */
int rpmlogSetMask (int mask)
	/*@globals internalState@*/
	/*@modifies internalState @*/;

/**
 * Generate a log message using FMT string and option arguments.
 * Note: inline'd to avoid debugging insturmentation overhead.
 */
/*@mayexit@*/ /*@printflike@*/
void _rpmlog (int code, const char *fmt, ...)
#if defined(__GNUC__) && __GNUC__ >= 2
	/* issue a warning if the format string doesn't match arguments */
	__attribute__((format (printf, 2, 3)))
#endif
	/*@*/;

/**
 * Same as _rpmlog with stdarg argument list.
 */
void vrpmlog (unsigned code, const char * fmt, va_list ap)
	/*@*/;

/*@mayexit@*/ /*@printflike@*/
static inline
void rpmlog (int code, const char *fmt, ...)
	/*@*/
{
    unsigned pri = RPMLOG_PRI(code);
    unsigned mask = RPMLOG_MASK(pri);

    if (mask & rpmlogSetMask(0)) {
	va_list ap;
	va_start(ap, fmt);
	vrpmlog(code, fmt, ap);
	va_end(ap);
    }
}

/*@-exportlocal@*/
/**
 * Return text of last rpmError() message.
 * @return		text of last message
 */
/*@-redecl@*/
/*@observer@*/ /*@null@*/ const char * rpmlogMessage(void)
	/*@*/;
/*@=redecl@*/

/**
 * Return error code from last rpmError() message.
 * @deprecated Perl-RPM needs, what's really needed is predictable, non-i18n
 *	encumbered, error text that can be retrieved through rpmlogMessage()
 *	and parsed IMHO.
 * @return		code from last message
 */
int rpmlogCode(void)
	/*@*/;

/**
 * Set rpmlog file handle.
 * @param fp		rpmlog file handle (NULL uses stdout/stderr)
 * @return		previous rpmlog file handle
 */
/*@null@*/
FILE * rpmlogSetFile(/*@null@*/ FILE * fp)
	/*@globals internalState@*/
	/*@modifies internalState @*/;
/*@=exportlocal@*/

#ifdef __cplusplus
}
#endif

#endif /* H_RPMLOG */
