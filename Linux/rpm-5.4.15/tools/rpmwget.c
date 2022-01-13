#include "system.h"
#include <stdarg.h>

#define	_RPMIOB_INTERNAL
#include <rpmiotypes.h>
#include <rpmio_internal.h>
#include <rpmdir.h>		/* _av_debug */
#include <rpmdav.h>
#include <poptIO.h>

#include "debug.h"

#define _KFB(n) (1U << (n))
#define _WFB(n) (_KFB(n) | 0x40000000)

/**
 */
typedef struct rpmwget_s * rpmwget;

#define F_ISSET(_f, _FLAG) (((_f) & ((WGET_FLAGS_##_FLAG) & ~0x40000000)) != WGET_FLAGS_NONE)
#define WF_ISSET(_FLAG) F_ISSET(wget->flags, _FLAG)

enum wgetFlags_e {
    WGET_FLAGS_NONE		= 0,
    /* --- Download --- */
    WGET_FLAGS_RETRYCONN	= _WFB( 0), /*!<    --retry-connrefused ... */
    WGET_FLAGS_NOCLOBBER	= _WFB( 1), /*!<    --no-clobber ... */
    WGET_FLAGS_RESUME		= _WFB( 2), /*!<    --continue ... */
    WGET_FLAGS_NEWERONLY	= _WFB( 3), /*!<    --timestamping ... */
    WGET_FLAGS_NODOWNLOAD	= _WFB( 4), /*!<    --spider ... */
};

enum wgetRFlags_e {
    WGET_RFLAGS_NONE		= 0,
    WGET_RFLAGS_TOLOWER		= _KFB( 0), /*!<    --restrict...=lowercase */
    WGET_RFLAGS_TOUPPER		= _KFB( 1), /*!<    --restrict...=uppercase */
    WGET_RFLAGS_NOCONTROL	= _KFB( 2), /*!<    --restrict...=nocontrol */
    WGET_RFLAGS_UNIX		= _KFB( 3), /*!<    --restrict...=unix */
    WGET_RFLAGS_WINDOWS		= _KFB( 4), /*!<    --restrict...=windows */
};

enum rpmioWCTLFlags_e {
    WCTL_FLAGS_NONE		= 0,
    WCTL_FLAGS_BACKGROUND	= _WFB( 0), /*!< -b,--background ... */
    WCTL_FLAGS_FORCEHTML	= _WFB( 1), /*!< -F,--force-html ... */

    WCTL_FLAGS_RECURSE		= _WFB( 2), /*!< -r,--recurse ... */
    WCTL_FLAGS_DELETEAFTER	= _WFB( 3), /*!<    --delete-after ... */
    WCTL_FLAGS_CONVERTLINKS	= _WFB( 4), /*!< -k,--convert-links ... */
    WCTL_FLAGS_BACKUP		= _WFB( 5), /*!< -K,--backup-converted ... */
/*XXX popt alias? */
    WCTL_FLAGS_MIRROR		= _WFB( 6), /*!< -m,--mirror ... */
    WCTL_FLAGS_PAGEREQUISITES	= _WFB( 7), /*!< -p,--page-requisites ... */
    WCTL_FLAGS_STRICTCOMMENTS	= _WFB( 8), /*!<    --strict-comments ... */

    WCTL_FLAGS_FOLLOWFTP	= _WFB( 9), /*!<    --follow-ftp ... */
    WCTL_FLAGS_SPANHOSTS	= _WFB(10), /*!<    --span-hosts ... */
    WCTL_FLAGS_RELATIVEONLY	= _WFB(11), /*!< -L,--relative ... */
    WCTL_FLAGS_NOPARENT		= _WFB(12), /*!< -np,--no-parent ... */
};
/*@unchecked@*/
static enum rpmioWCTLFlags_e rpmioWCTLFlags = WCTL_FLAGS_NONE;

enum rpmioWDLFlags_e {
    WDL_FLAGS_NONE		= 0,
    WDL_FLAGS_SERVERRESPONSE	= _WFB( 5), /*!<    --server-response ... */
    WDL_FLAGS_NOPROXY		= _WFB( 7), /*!<    --no-proxy ... */
    WDL_FLAGS_NODNSCACHE	= _WFB( 8), /*!<    --no-dns-cache ... */
    WDL_FLAGS_IGNORECASE	= _WFB(10), /*!<    --ignore-case ... */
    WDL_FLAGS_INET4		= _WFB(11), /*!<    --inet4-only ... */
    WDL_FLAGS_INET6		= _WFB(12), /*!<    --inet6-only ... */
};
/*@unchecked@*/
static enum rpmioWDLFlags_e rpmioWDLFlags = WDL_FLAGS_NONE;

enum rpmioWDirFlags_e {
    WDIR_FLAGS_NONE		= 0,
    WDIR_FLAGS_NOCREATE		= _WFB( 0), /*!<    --no-directories ... */
    WDIR_FLAGS_FORCE		= _WFB( 1), /*!<    --force-directories ... */
    WDIR_FLAGS_NOHOST		= _WFB( 2), /*!<    --no-host-directories ... */
    WDIR_FLAGS_ADDPROTO		= _WFB( 3), /*!<    --protocol-directories ... */
};
/*@unchecked@*/
static enum rpmioWDirFlags_e rpmioWDirFlags = WDIR_FLAGS_NONE;

enum rpmioHttpFlags_e {
    HTTP_FLAGS_NONE		= 0,
    HTTP_FLAGS_NOCACHE		= _WFB( 0), /*!<    --no-cache ... */
    HTTP_FLAGS_IGNORELENGTH	= _WFB( 1), /*!<    --ignore-length ... */
    HTTP_FLAGS_NOKEEPALIVE	= _WFB( 2), /*!<    --no-http-keep-alive ... */
    HTTP_FLAGS_NOCOOKIES	= _WFB( 3), /*!<    --no-cookies ... */
    HTTP_FLAGS_KEEPCOOKIES	= _WFB( 4), /*!<    --keep-session-cookies ... */
    HTTP_FLAGS_CONTENTDISPOSITION =_WFB( 5),/*!<    --content-dispostion ... */
    HTTP_FLAGS_NOCHALLENGE	= _WFB( 6), /*!<    --auth-no-challenge ... */
    HTTP_FLAGS_SAVEHEADERS	= _WFB( 7), /*!<    --save-headers ... */
};
/*@unchecked@*/
static enum rpmioHttpFlags_e rpmioHttpFlags = HTTP_FLAGS_NONE;

enum rpmioHttpsFlags_e {
    HTTPS_FLAGS_NONE		= 0,
    HTTPS_FLAGS_NOCHECK		= _WFB( 0), /*!<    --no-check-certificate ... */
};
/*@unchecked@*/
static enum rpmioHttpsFlags_e rpmioHttpsFlags = HTTPS_FLAGS_NONE;

enum rpmioFtpFlags_e {
    FTP_FLAGS_NONE		= 0,
    FTP_FLAGS_KEEPLISTING	= _WFB( 0), /*!<    --no-remove-listing ... */
    FTP_FLAGS_NOGLOB		= _WFB( 1), /*!<    --no-glob ... */
    FTP_FLAGS_ACTIVE		= _WFB( 2), /*!<    --no-passive-ftp ... */
    FTP_FLAGS_FOLLOW		= _WFB( 3), /*!<    --retr-symlinks ... */
    FTP_FLAGS_PRESERVE		= _WFB( 4), /*!<    --preserve-permissions ... */
};
/*@unchecked@*/
static enum rpmioFtpFlags_e rpmioFtpFlags = FTP_FLAGS_NONE;

/**
 */
struct rpmwget_s {
    enum wgetFlags_e flags;		/*!< Control bits. */
    enum wgetRFlags_e rflags;		/*!< Ouput path Control bits. */

    ARGV_t argv;			/*!< URI's to process. */

    char * b;				/*!< I/O buffer */
    size_t blen;			/*!< I/O buffer size (bytes) */
    const char * lfn;			/*!< Logging file name. */
    const char * lfmode;		/*!< Logging file mode. */
    FD_t lfd;				/*!< Logging file handle. */
    const char * ifn;			/*!< Input file name. */
    FD_t ifd;				/*!< Input file handle. */
    const char * ofn;			/*!< Output file name. */
    FD_t ofd;				/*!< Output file handle. */
    struct stat * st;			/*!< Ouput file stat(2) */

/*@null@*/
    const char * bartype;		/*!< Progress bar display type. */
    int maxcols;			/*!< Display width. */
    rpmop rop;				/*!< URI Receive accumulator. */
    rpmop sop;				/*!< URI Send accumulator. */
    rpmop top;				/*!< URI Total accumulator. */
    double rate_kbps;			/*!< URI Transfer rate. */
    double eta_msecs;			/*!< URI ETA. */
    double pct;				/*!< URI Transfer percentage. */
    struct rpmop_s Top;			/*!< WGET Total accumulator. */
    double rate;			/*!< Download rate. */
    struct timespec tv;			/*!< Per-operation delay. */

    int ntry;				/*!< Retry counter. */
   
    /* --- Startup --- */
    const char * execute_cmd;		/*!< -e,--execute ... */

    /* --- Logging & Input --- */
    int debug;				/*!< -d,--debug ... */
    int quiet;				/*!< -q,--quiet ... */
    int verbose;			/*!< -v,--verbose ... */
    ARGV_t manifests;			/*!< -i,--input-file ... */
    const char * base_prefix;		/*!< -B,--base ... */

    /* --- Download --- */
    int ntries;				/*!< -t,--tries ... */
    int timeout_secs;			/*!< -T,--timeout ... */
    int dns_timeout_secs;		/*!<    --dns-timeout ... */
    int connect_timeout_secs;		/*!<    --connect-timeout ... */
    int read_timeout_secs;		/*!<    --read-timeout ... */
    unsigned long long quota;		/*!< -Q,--quota ... (in kB) */
    int wait_secs;			/*!< -w,--wait ... */
    int waitretry_secs;			/*!<    --waitretry ... */
    int randomwaitretry_secs;		/*!<    --random-wait ... */
    double limit_rate;			/*!<    --limit-rate ... (in kB/s) */
    int bind_address;			/*!<    --bind-address ... */
    const char * prefer;		/*!<    --prefer-family ... */
    const char * document_file;		/*!< -O,--output-document ... */
    const char * user;			/*!<    --user ... */
    const char * password;		/*!<    --password ... */

    /* --- Directories --- */
    const char * dir_prefix;		/*!< -P,--directory-prefix ... */
    int dir_cut;			/*!<    --cut-dirs ... */

    /* --- HTTP --- */
    const char * http_user;		/*!<    --http-user ... */
    const char * http_password;		/*!<    --http-password ... */
    const char * http_ext;		/*!< -E,--http-extension ... */
    const char * http_header;		/*!<    --header ... */
    int http_max_redirect;		/*!<    --max-redirect ... */
    const char * http_proxy_user;	/*!<    --proxy-user ... */
    const char * http_proxy_password;	/*!<    --proxy-password ... */
    const char * http_referer;		/*!<    --referer ... */
    const char * http_user_agent;	/*!< -U,--user-agent ... */
    const char * http_load_cookies_file;/*!<    --load-cookies ... */
    const char * http_save_cookies_file;/*!<    --save-cookies ... */
    const char * http_post_data;	/*!<    --post-data ... */
    const char * http_post_file;	/*!<    --post-file ... */

    /* --- HTTPS --- */
    const char * https_protocol;	/*!<    --secure-protocol ... */
    const char * https_certificate_file;/*!<    --certificate ... */
    const char * https_certificate_type;/*!<    --certificate-type ... */
    const char * https_privatekey_file;	/*!<    --private-key ... */
    const char * https_privatekey_type;	/*!<    --private-key-type ... */
    const char * https_cacertificate_file;/*!<  --ca-certificate ... */
    const char * https_cacertificate_dir;/*!<   --ca-directory ... */
    const char * https_random_file;	/*!<    --random-file ... */
    const char * https_egd_file;	/*!<    --egd-file ... */

    /* --- FTP --- */
    const char * ftp_user;		/*!<    --ftp-user ... */
    const char * ftp_password;		/*!<    --ftp-password ... */

    /* --- Recursive --- */
    int recurse_max;			/*!< -l,--level ... */
    const char * accept_exts;		/*!< -A,--accept ... */
    const char * reject_exts;		/*!< -R,--reject ... */
    const char * accept_domains;	/*!< -D,--domains ... */
    const char * reject_domains;	/*!<    --exclude-domains ... */
    const char * accept_tags;		/*!<    --follow-tags ... */
    const char * reject_tags;		/*!<    --ignore-tags ... */
    const char * accept_dirs;		/*!< -I,--include-directories ... */
    const char * reject_dirs;		/*!< -X,--exclude-directories ... */
};

/*@unchecked@*/
static struct rpmwget_s __rpmwget = {
    .debug = -1,
    .verbose = -1,

    .maxcols = 80,

#ifdef	NOTYET
    .ntries = 20,
#else
    .ntries = 1,
#endif
    .recurse_max = 5,
    .read_timeout_secs = 900,
    .http_max_redirect = 20,
    .http_ext = ".html"
};

/*@unchecked@*/
static rpmwget _rpmwget = &__rpmwget;

/*==============================================================*/
/**
 */
static const char * wgetSuffix(const char * s)
	/*@*/
{
    const char * se = s + strlen(s);

    while (se > s && !(se[-1] == '/' || se[-1] == '.'))
	se--;
    return (se > s && *se && se[-1] == '.' ? se : NULL);
}

/**
 */
static char * wgetStpcpy(rpmwget wget, char * te, const char * s)
{
    int c;

    if (s != NULL)
    while ((c = (int)*s++) != 0) {
	if (wget->rflags & WGET_RFLAGS_TOLOWER)
	    *te++ = (char) xtolower(c);
	else if (wget->rflags & WGET_RFLAGS_TOUPPER)
	    *te++ = (char) xtoupper(c);
	else if (wget->rflags & WGET_RFLAGS_NOCONTROL) {
	    *te++ = (char) c;	/* XXX unimplemented */
	} else
	    *te++ = (char) c;
    }
    *te = '\0';
    return te;
}

/**
 */
static const char * wgetTstamp(rpmwget wget, time_t t)
{
    static const char tfmt[] = "%Y-%m-%d %H:%M:%S";
    static char tstamp[32];
    struct tm * tm = localtime(&t);
    (void) strftime(tstamp, sizeof(tstamp), tfmt, tm);
    return tstamp;
}

/*==============================================================*/

/**
 */
static int wgetLog(rpmwget wget, const char * fmt, ...)
{
    int rc = 0;
    va_list ap;

    if (wget->lfd == NULL) {
	va_start(ap, fmt);
	rc = vfprintf(stderr, fmt, ap);
	va_end(ap);
    } else {
	size_t msgnb = 256;
	char *msgbuf = xmalloc(msgnb);
	int nb;

	/* Loop to size the output buffer. */
	va_start(ap, fmt);
	while (1) {
	    nb = vsnprintf(msgbuf, msgnb, fmt, ap);
	    if (nb > -1 && (size_t)nb < msgnb)
		break;
	    if (nb > -1)            /* glibc 2.1 (and later) */
		msgnb = nb+1;
	    else                    /* glibc 2.0 */
		msgnb *= 2;
	}
	va_end(ap);

	if (nb > 0) {
	    msgnb = (size_t) nb;
	    (void) Fwrite(msgbuf, 1, (size_t)nb, wget->lfd);
	}
        msgbuf = _free(msgbuf);
    }

    return rc;
}

/**
 */
static const char * wgetProgress(rpmwget wget, /*@null@*/ rpmop op)
{
    unsigned long long cur = (op ? op->bytes : 0);
    static char * tbuf = NULL;
    double rate = wget->rate_kbps;
    double etamsecs = wget->eta_msecs;
    double pct = wget->pct;
    int ipct = (int)((100 * pct) + 0.5);
    char * t, * te;
    size_t nb;
    int barcols;
    int i;

    nb = 100;
    if (tbuf == NULL)
	tbuf = xmalloc(nb+1);

    te = t = tbuf;
    *t = '\0';
    te += nb;

  /* The progress bar should look like this:
     xx% [=======>             ] nn,nnn 12.34K/s  eta 36m 51s

     Calculate the geometry.  The idea is to assign as much room as
     possible to the progress bar.  The other idea is to never let
     things "jitter", i.e. pad elements that vary in size so that
     their variance does not affect the placement of other elements.
     It would be especially bad for the progress bar to be resized
     randomly.

     "xx% " or "100%"  - percentage               - 4 chars
     "[]"              - progress bar decorations - 2 chars
     " nnn,nnn,nnn"    - downloaded bytes         - 12 chars or very rarely more
     " 12.5K/s"        - download rate             - 8 chars
     "  eta 36m 51s"   - ETA                      - 14 chars

     "=====>..."       - progress bar             - the rest
  */

    barcols = wget->maxcols;
    barcols -= sizeof("100%") - 1;
    barcols -= sizeof("[]") - 1;
    barcols -= sizeof(" nnn,nnn,nnn") - 1;
    barcols -= sizeof(" 12.5K/s") - 1;
    barcols -= sizeof("  eta 36m 51s") - 1;

    t += sprintf(t, "%3d", ipct);

    t = stpcpy(t, "% [");
    for (i = 0; i < barcols; i++) {
	int ix = (100 * i) / barcols;
	char c = (ix <= ipct ? '=' : ' ');
        if (ix-1 <= ipct && ix >= ipct)
	    c = '>';
	*t++ = c;
    }
    t = stpcpy(t, "]");

    if (cur >= (1000 * 1000 * 1000) || cur < 1000)
	t += sprintf(t, "%12ld", (long)cur);
    else if (cur > (1000 * 1000))
	t += sprintf(t, " %3d,%03d,%03d", (int)(cur/(1000*1000)),
			(int)((cur % (1000*1000))/1000), (int)(cur % 1000));
    else
	t += sprintf(t, "     %3d,%03d", (int)(cur/1000), (int)(cur % 1000));

    if (rate > 0.0) {
	if (rate >= 100000.)
	    t += sprintf(t, "%5.0fM/s", (rate/1024));
	else if (rate >= 10000.)
	    t += sprintf(t, "%5.1fM/s", (rate/1024));
	else if (rate >= 1000.)
	    t += sprintf(t, "%5.2fM/s", (rate/1024));
	else if (rate >= 100.)
	    t += sprintf(t, "%5.0fK/s", rate);
	else if (rate >= 10.)
	    t += sprintf(t, "%5.1fK/s", rate);
	else
	    t += sprintf(t, "%5.2fK/s", rate);
    } else
	t = stpcpy(t, " __._K/s");

    if (etamsecs > 0.0) {
	int secs = (int) (etamsecs/1000);
	int mins = secs/60;
	int hours = mins/60;
	if (hours > 0)
	    t += sprintf(t, " eta %2dh %2dm", hours, (mins % 60));
	else if (mins > 0)
	    t += sprintf(t, " eta %2dm %2ds", mins, (secs % 60));
	else if (secs > 0)
	    t += sprintf(t, " eta %2ds", (secs % 60));
    }

    *t = '\0';

    return tbuf;
}

/**
 */
static int wgetSetRate(rpmwget wget, /*@null@*/ const urlinfo u, rpmop op)
	/*@modifies wget, op @*/
{
    unsigned long long progress;
    unsigned long long total;
    rpmtime_t usecs;

    if (op == NULL)
	return 0;

    if (u != NULL) {
	struct rpmsw_s end;
	if (op->count == 0)
	    rpmswEnter(op, -1);
	else
	    op->count++;
	op->bytes = u->info.progress;
	progress = u->info.progress;
	total = u->info.total;
	usecs = rpmswDiff(rpmswNow(&end), &op->begin);
    } else {
	progress = op->bytes;
	total = op->bytes;
	usecs = op->usecs;
    }
    
    if (usecs > 0) {
	wget->rate_kbps = (double) progress / usecs;
	wget->rate_kbps *= (double) (1000 * 1000)/1024;
	if (total > 0) {
	    wget->eta_msecs = (wget->rate_kbps > 0.0)
		? (total - progress) / (1024 * wget->rate_kbps)
		: 0.0;
	    wget->eta_msecs *= (double) 1000;
	    wget->pct = (double) progress / total;
	} else {
	    wget->eta_msecs = 0.0;
	    wget->pct = 0.0;
	}
    } else {
	wget->rate_kbps = 0.0;
	wget->eta_msecs = 0.0;
	wget->pct = 0.0;
    }
    return 0;
}

/**
 */
static int wgetSetDelay(rpmwget wget, /*@null@*/ const urlinfo u,
		const rpmop op)
	/*@modifies wget @*/
{
    static int fac = 9;
    rpmop top;
    long long dusecs, ousecs, nusecs;
    unsigned long long kbytes;
    int opcount;
    double d;
    int xx;

    /* Update per-operation delay estimate if rate limiting. */
    if (!(wget->limit_rate > 0.0 && wget->rate_kbps > 0.0))
	return 0;

    /* Delay if over limit rate. */
    if (wget->tv.tv_sec || wget->tv.tv_nsec)
	xx = nanosleep(&wget->tv, NULL);

    top = wget->top;
    dusecs = (top->usecs + op->usecs);
    kbytes = (top->bytes + op->bytes) / 1024;
    opcount = (top->count + op->count);

    if (opcount < 10 || kbytes < 32)
	return 0;
    if ((opcount % 5) != 0)
	return 0;

    d = kbytes - (dusecs * wget->limit_rate)/(1000 * 1000);
    d /= wget->limit_rate;
    d /= opcount;	/* d = time correction delta per operation in seconds */

    dusecs = (long long)(d * 1000 * 1000);
    ousecs = (long long)(wget->tv.tv_sec * 1000 * 1000) + wget->tv.tv_nsec/1000;
    nusecs = ousecs + dusecs;
    if (nusecs < 0)
	nusecs = ousecs/2;

    if (fac >= 0 && fac <= 10) {
	/* EWMA: new = ((10 - fac) * old + (fac * delta)) / 10 */
	ousecs *= (10 - fac);
	ousecs += nusecs * fac;
	ousecs /= 10;
    } else
	ousecs = nusecs;

    wget->tv.tv_sec = ousecs/(1000*1000);
    wget->tv.tv_nsec = (ousecs % (1000 * 1000)) * 1000;

    return 0;
}

/* XXX must agree with ne_session_status enum values. */
typedef enum {
    wget_status_lookup = 0,	/* looking up hostname */
    wget_status_connecting = 1,	/* connecting to host */
    wget_status_connected = 2,	/* connected to host */
    wget_status_sending = 3,	/* sending a request body */
    wget_status_recving = 4,	/* receiving a response body */
    wget_status_disconnected = 5/* disconnected from host */
} wget_session_status;

/**
 */
static int wgetNotify(const urlinfo u, unsigned _status)
{
    static const char * cstates[] = {
	"LU", "CI", "CD", "SI", "RI", "DD", "?6", "?7"
    };
    wget_session_status status = _status;
    rpmwget wget;
    rpmop rop;
    rpmop sop;
    rpmop top;

assert(u != NULL);
assert(u->arg != NULL);
    wget = (rpmwget) u->arg;
    wget->rop = rop = u->rop;
    wget->sop = sop = u->sop;
    wget->top = top = u->top;

    switch (status) {
    default:
	break;
    case wget_status_lookup:		/* looking up hostname */
	wgetLog(wget, "Resolving %s...", u->info.hostname);
	break;
    case wget_status_connecting:	/* connecting to host */
	/* Finish the lookup -> connecting state transition. */
	if (u->info.status == wget_status_lookup)
	    wgetLog(wget, " %s\n", u->info.address);
	else if (u->info.status != wget_status_disconnected)
fprintf(stderr, "%s: **TODO** %s => %s\n", __progname, cstates[u->info.status & 0x7], cstates[status & 0x7]);
	wgetLog(wget, "Connecting to %s|%s|:%u...",
			   u->info.hostname, u->info.address, u->port);
	break;
    case wget_status_connected:		/* connected to host */
	/* Finish the connecting -> connected state transition. */
	if (u->info.status == wget_status_connecting)
	    wgetLog(wget, " connected.\n");
	else
fprintf(stderr, "**TODO** %s => %s\n", cstates[u->info.status & 0x7], cstates[status & 0x7]);
	break;
    case wget_status_sending:		/* sending a request body */
	(void) wgetSetRate(wget, u, sop);
	if (u->info.status != status)
	if (u->info.status != wget_status_connected)
fprintf(stderr, "%s: **TODO** %s => %s\n", __progname, cstates[u->info.status & 0x7], cstates[status & 0x7]);
#ifdef NOTYET	/* XXX noisy, wait for progress bar ... */
	wgetLog(wget, "Sending ... (%ld:%ld)\n",
		(long) u->info.progress, (long) u->info.total);
#endif
	/* Delay if over limit rate. */
	(void) wgetSetDelay(wget, u, sop);
	break;
    case wget_status_recving:		/* receiving a response body */
	(void) wgetSetRate(wget, u, rop);
	if (u->info.status != status)
	if (u->info.status != wget_status_connected)
	if (u->info.status != wget_status_sending)
fprintf(stderr, "%s: **TODO** %s => %s\n", __progname, cstates[u->info.status & 0x7], cstates[status & 0x7]);
#ifdef NOTYET	/* XXX noisy, wait for progress bar ... */
	wgetLog(wget, "Recving ... (%ld:%ld)\n",
		(long) u->info.progress, (long) u->info.total);
#endif
	/* Delay if over limit rate. */
	(void) wgetSetDelay(wget, u, rop);
	break;
    case wget_status_disconnected:
	if (rop && rop->count > 0) {
	    rpmswExit(rop, 0);
	    rpmswAdd(top, rop);
	    rop->count = 0;
	}
	if (sop && sop->count > 0) {
	    rpmswExit(sop, 0);
	    rpmswAdd(top, sop);
	    sop->count = 0;
	}
	(void) wgetSetRate(wget, NULL, top);
	if (u->info.status != wget_status_recving)
fprintf(stderr, "**TODO** %s => %s\n", cstates[u->info.status & 0x7], cstates[status & 0x7]);
	wgetLog(wget, "Disconnected from %s:%u\n", u->info.hostname, u->port);
	break;
    }
    return 0;
}

/*==============================================================*/

/**
 */
static int isHtml(const char * contentType)
{
    int ishtml = 0;

    if (contentType) {
	char * t = xstrdup(contentType);
	char * te = strchr(t, ';');
	if (te) *te = '\0';
	if (!strcmp(t, "text/html") || !strcmp(t, "application/xhtml+xml"))
	    ishtml = 1;
	t = _free(t);
    }
    return ishtml;
}

/**
 */
static const char * wgetOPath(rpmwget wget)
{
    const char * s = wget->document_file;
    const char * sext = NULL;
    char * t = NULL;
    int ishtml = isHtml(wget->ifd->contentType);
    char * te;
    size_t nb;

    if (s == NULL) {
	if ((s = strrchr(wget->ifn, '/')) != NULL)
	    s++;
	else
	    s = wget->ifn;

	if (*s == '\0')
	    s = "index.html";
    }

    /* Add the .html extension (if requested). */
    switch (urlPath(wget->ifn, NULL)) {
    case URL_IS_HTTPS:
    case URL_IS_HTTP:
	/* ishtml = 1 iff Content-Type: ( text/html | application/xhtml+xml ) */
    	if (wget->http_ext != NULL && ishtml) {
	    const char * f = wgetSuffix(s);
	    if (f == NULL 
	     || !(!strcasecmp(f, "html") || !strcasecmp(f, "htm") || !strcasecmp(f+1, "html")))
		sext = wget->http_ext;
	}
	break;
    default:
	break;
    }

    nb = 0;
    if (wget->dir_prefix)
	nb += strlen(wget->dir_prefix) + sizeof("/") - 1;;
    nb += strlen(s);
    if (sext)
	nb += strlen(sext);
    nb++;
	
    te = t = xmalloc(nb);

    if (wget->dir_prefix)
	te = stpcpy( stpcpy(te, wget->dir_prefix), "/");

    te = wgetStpcpy(wget, wgetStpcpy(wget, te, s), sext);

    return t;
}

/**
 */
static int wgetCopyFile(rpmwget wget)
{
    size_t nw = 0;
    size_t nr;
    struct stat * st = wget->st;
    ssize_t contentLength = 0;
    time_t lastModified = 0;
    int rc;

    /* Verify input URI exists, don't transfer content. */
    if (WF_ISSET(NODOWNLOAD)) {
	wgetLog(wget, _("Spider mode enabled. Check if remote file exists.\n"));
	wgetLog(wget, "--%s--  %s\n", wgetTstamp(wget, time(NULL)), wget->ifn);
	rc = Stat(wget->ifn, st);
	if (rc == 0) {
	    wgetLog(wget, "Length: %lu (%luK) [content/type] [%24.24s]\n",
		(unsigned long)st->st_size,
		(unsigned long)(st->st_size + 1023)/1024,
		ctime(&st->st_mtime));
	    wgetLog(wget, _("Remote file exists.\n"));
	} else {
	    wgetLog(wget, _("Remote file does not exist -- broken link!!!\n"));
	}
	goto exit;
    }

    wgetLog(wget, "--%s--  %s\n", wgetTstamp(wget, time(NULL)), wget->ifn);
    /* Open input URI for content transfer. */
    wget->ifd = Fopen(wget->ifn, "r.ufdio");
    rc = (wget->ifd == NULL || Ferror(wget->ifd)) ? 1 : 0;
    if (rc) {
#ifdef	NOTYET
	if (!WF_ISSET(RETRYCONN) && "connection refused")
#endif
	goto exit;
    }

    if (wget->read_timeout_secs > 0)
	wget->ifd->rd_timeoutsecs = wget->read_timeout_secs;

#ifdef	NOTYET
#ifdef SO_RCVBUF
    if (wget->limit_rate > 0.0) {
	int bufsiz = (int) (wget->limit_rate * 1024);
	if (bufsiz < 512)
	    bufsiz = 512;
	if (bufsiz > 8192)
	    bufsiz = 8192;
	setsockopt (sock, SOL_SOCKET, SO_RCVBUF,
                  (void *)&bufsize, (socklen_t)sizeof (bufsize));
    }
#endif
#endif

    contentLength = wget->ifd->contentLength;
    lastModified = wget->ifd->lastModified;

if (wget->debug < 0) {
    const char * contentType = wget->ifd->contentType;
    const char * contentDisposition = wget->ifd->contentDisposition;

    /* XXX rely on rpmio debugging? */
    fprintf(stderr, "Length:");
    if (contentLength < 0)
	fprintf(stderr, " unspecified");
    else
	fprintf(stderr, " %lu", (unsigned long)contentLength);
    if (contentType)
	fprintf(stderr, " [%s]", contentType);
    if (lastModified > 0)
	fprintf(stderr, " [%24.24s]", ctime(&lastModified));
    fprintf(stderr, "\n");
    if (contentDisposition)
	fprintf(stderr, "Content-Disposition: %s\n", contentDisposition);
}

    wget->ofn = wgetOPath(wget);

    if ((WF_ISSET(NOCLOBBER) || WF_ISSET(NEWERONLY)) && !Stat(wget->ofn, st)) {
	/* Don't clobber pre-existing output files. */
	if (WF_ISSET(NOCLOBBER)) {
	    wgetLog(wget,
		_("Ouptut file \"%s\" exists, skipping retrieve.\n"),
		wget->ofn);
	    lastModified = 0;	/* XXX disable timestamping. */
	    rc = 0;
	    goto exit;
	}

	/* Download content iff newer. */
	if (WF_ISSET(NEWERONLY)
	 && lastModified > 0 && lastModified <= st->st_mtime)
	{
	    wgetLog(wget,
		_("Input for file \"%s\" is not newer, skipping retrieve.\n"),
		wget->ofn);
	    lastModified = 0;	/* XXX disable timestamping. */
	    rc = 0;
	    goto exit;
	}
    }

    wgetLog(wget, "Saving to: `%s'\n", wget->ofn);
    wget->ofd = Fopen(wget->ofn, "w.ufdio");
    if (wget->ofd == NULL || Ferror(wget->ofd)) {
	rc = 1;
	goto exit;
    }

#ifdef	NOTYET
#ifdef SO_SNDBUF
    if (wget->limit_rate > 0.0) {
	int bufsiz = (int) (wget->limit_rate * 1024);
	if (bufsiz < 512)
	    bufsiz = 512;
	if (bufsiz > 8192)
	    bufsiz = 8192;
	setsockopt (sock, SOL_SOCKET, SO_SNDBUF,
                  (void *)&bufsize, (socklen_t)sizeof (bufsize));
    }
#endif
#endif

#ifdef	NOTYET
    /* Reposition I/O handles if resuming. */
    if (WF_ISSET(RESUME) && "partially downloaded")
#endif

    /* XXX Ferror(wget->ifd) ? */
    while ((nr = Fread(wget->b, 1, wget->blen, wget->ifd)) > 0) {
	
	/* XXX Ferror(wget->ofd) ? */
	if ((nw = Fwrite(wget->b, 1, nr, wget->ofd)) != nr)
	    break;

	/* Display progress. */
	if (wget->bartype != NULL && strcmp(wget->bartype, "none"))
	    fprintf(stderr, "\r%s", wgetProgress(wget, wget->rop));

    }
if (wget->debug < 0)
fprintf(stderr, "\n");

    /* Save the total rate (calculated when returning to disconnected state) */
    wget->rate = wget->rate_kbps;

    /* OK iff EOF was read. */
    if (nr == 0) {
	wgetLog(wget, _("--%s-- (%3.1f KB/s) - `%s' saved [%lu]\n"),
		wgetTstamp(wget, time(NULL)), wget->rate,
		wget->ofn, (unsigned long)nw);
	rc = 0;
    } else {
	rc = 1;
    }

exit:

    /* XXX dereference from _url_cache needed? */
    if (wget->ifd != NULL && strcmp(wget->ifn, "-"))
	(void) Fclose(wget->ifd);	/* XXX is stdin closed here? */
    wget->ifd = NULL;

    if (wget->ofd != NULL && strcmp(wget->ofn, "-"))
	(void) Fclose(wget->ofd);	/* XXX is stdout closed here? */
    wget->ofd = NULL;

    /* Timestamp the output file on success. */
    if (rc == 0 && lastModified > 0) {
	struct utimbuf stamp;
	stamp.actime = lastModified;
	stamp.modtime = lastModified;
	(void) Utime(wget->ofn, &stamp);
    }

    /* XXX if (rc) Unlink(wget->ofn); */

    wget->ofn = _free(wget->ofn);
    return rc;
}

/**
 */
static int wgetCopyRetry(rpmwget wget)
{
    int rc;

    wget->ntry = 0;
    do {
	rc = wgetCopyFile(wget);
    } while (rc != 0 && (wget->ntries <= 0 || ++wget->ntry < wget->ntries));

    /* Quota check (if requested) */
    if (wget->quota > 0) {
	rpmop top = wget->top;
	if (top && (top->bytes/1024) > wget->quota) {
	    wgetLog(wget, _("Download quota of %lluK EXCEEDED!\n"),
		(unsigned long long) wget->quota);
	    rc = 1;
	}
    }

    return rc;
}

/**
 */
static int wgetLoadManifests(rpmwget wget)
	/*@modifies wget @*/
{
    ARGV_t manifests;
    const char * fn;
    int rc = 0;	/* assume success */

    if ((manifests = wget->manifests) != NULL)	/* note rc=0 return with no files to load. */
    while ((fn = *manifests++) != NULL) {
	unsigned lineno;
	char * be = NULL;
	rpmiob iob = NULL;
	int xx = rpmiobSlurp(fn, &iob);
	char * f;
	char * fe;

	if (!(xx == 0 && iob != NULL)) {
	    fprintf(stderr, _("%s: Failed to open %s\n"), __progname, fn);
	    rc = -1;
	    goto bottom;
	}

	be = (char *)(iob->b + iob->blen);
	while (be > (char *)iob->b && (be[-1] == '\n' || be[-1] == '\r')) {
	  be--;
	  *be = '\0';
	}

	/* Parse and save manifest items. */
	lineno = 0;
	for (f = (char *)iob->b; *f; f = fe) {
	    const char * path;
	    char * g, * ge;
	    lineno++;

	    fe = f;
	    while (*fe && !(*fe == '\n' || *fe == '\r'))
		fe++;
	    g = f;
	    ge = fe;
	    while (*fe && (*fe == '\n' || *fe == '\r'))
		*fe++ = '\0';

	    while (*g && xisspace((int)*g))
		*g++ = '\0';
	    /* Skip comment lines. */
	    if (*g == '#')
		continue;

	    while (ge > g && xisspace(ge[-1]))
		*--ge = '\0';
	    /* Skip empty lines. */
	    if (ge == g)
		continue;

	    /* Prepend wget->base_prefix if specified. */
	    if (wget->base_prefix)
		path = rpmExpand(wget->base_prefix, g, NULL);
	    else
		path = rpmExpand(g, NULL);
	    (void) argvAdd(&wget->argv, path);
	    path = _free(path);
	}

bottom:
	iob = rpmiobFree(iob);
	if (rc != 0)
	    goto exit;
    }

exit:
    return rc;
}

/*==============================================================*/
enum {
    POPTWGET_RESTRICT	= -2048,	/*    --restrict-file-names */
    POPTWGET_PROGRESS	= -2049,	/*    --progress */
    POPTWGET_LIMITRATE	= -2050,	/*    --limit-rate */
    POPTWGET_QUOTA	= -2051,	/* -Q,--quota */
    POPTWGET_LOGFILE	= -2052,	/* -o,--output-file */
    POPTWGET_LOGAPPEND	= -2053,	/* -a,--append-output */
};

/**
 */
static void rpmwgetArgCallback(poptContext con,
                /*@unused@*/ enum poptCallbackReason reason,
                const struct poptOption * opt, const char * arg,
                void * data)
	/*@modifies data @*/
{
    /* XXX make sure wget!=NULL even if popt table callback item doesn't set */
    rpmwget wget = (rpmwget) (data ? data : _rpmwget);
    ARGV_t av = NULL;
    int i;

fprintf(stderr, "--> rpmwgetCallback(%p): val %d/0x%x arg %s\n", wget, opt->val, (unsigned)opt->val, arg);
    /* XXX avoid accidental collisions with POPT_BIT_SET for flags */
    if (opt->arg == NULL)
    switch (opt->val) {
    case POPTWGET_LOGAPPEND:
	wget->lfmode = "a";
	/*@fallthrough@*/
    case POPTWGET_LOGFILE:
	wget->lfn = _free(wget->lfn);
	wget->lfn = xstrdup(arg);
	break;
    case POPTWGET_QUOTA:
    {
	char * end = NULL;
	unsigned long long d = strtoll(arg, &end, 0);

	if (*end == 'g' || *end == 'G') {
	    d *= (1024 * 1024 * 1024); end++;
	} else
	if (*end == 'm' || *end == 'M') {
	    d *= (1024 * 1024); end++;
	} else
	if (*end == 'k' || *end == 'K') {
	    d *= 1024; end++;
	} else {
	    d += 1023;	/* round up to next kB */
	}
	 
	if (*end == '\0')
	    wget->quota = d / 1024;
	else
fprintf(stderr, "Invalid --quota arg ignored: %s\n", arg);
    }	break;
    case POPTWGET_LIMITRATE:
    {
	char * end = NULL;
	double d = strtod(arg, &end);
	if (*end == 'k' || *end == 'K') {
	    d *= 1024;
	    end++;
	}
	if (*end == '\0')
	    wget->limit_rate = d / 1024;
	else
fprintf(stderr, "Invalid --limit-rate arg ignored: %s\n", arg);
    }	break;
    case POPTWGET_PROGRESS:
	if (arg == NULL || !strcmp(arg, "none")) {
	    wget->bartype = _free(wget->bartype);
	    wget->bartype = xstrdup("none");
	} else if (!strcmp(arg, "bar")) {
	    wget->bartype = _free(wget->bartype);
	    wget->bartype = xstrdup(arg);
	} else if (!strcmp(arg, "dot") 
	 || !strcmp(arg, "dot:default") 
	 || !strcmp(arg, "dot:binary") 
	 || !strcmp(arg, "dot:mega"))
	{
	    wget->bartype = _free(wget->bartype);
	    wget->bartype = xstrdup(arg);
	} else
fprintf(stderr, "Invalid --progress arg ignored: %s\n", arg);
	break;
    case POPTWGET_RESTRICT:	/* --restrict-file-names */
	(void) argvSplit(&av, arg, ",");
	for (i = 0; av[i] != NULL; i++) {
	   if (!strcmp(av[i], "lowercase"))
		wget->rflags |= WGET_RFLAGS_TOLOWER;
	   else if (!strcmp(av[i], "uppercase"))
		wget->rflags |= WGET_RFLAGS_TOUPPER;
	   else if (!strcmp(av[i], "nocontrol"))
		wget->rflags |= WGET_RFLAGS_NOCONTROL;
	   else if (!strcmp(av[i], "unix"))
		wget->rflags |= WGET_RFLAGS_UNIX;
	   else if (!strcmp(av[i], "windows"))
		wget->rflags |= WGET_RFLAGS_WINDOWS;
	   else
fprintf(stderr, "Invalid --restrict-file-names item ignored: %s\n", av[i]);
	}
	av = argvFree(av);
	break;
    default:
	fprintf(stderr, _("%s: Unknown option -%c\n"), __progname, opt->val);
	poptPrintUsage(con, stderr, 0);
	/*@-exitarg@*/ exit(2); /*@=exitarg@*/
	/*@notreached@*/ break;
    }
}

/*==============================================================*/
/*@unchecked@*/ /*@observer@*/
static struct poptOption rpmioWCTLStartupPoptTable[] = {
  { "version", 'V', POPT_ARG_NONE,	NULL, 'V',
	N_("display the version of Wget and exit."), NULL },
#ifdef	NOTYET	/* XXX FIXME: lt-rpmwget: option table misconfigured (104) */
  { "help", 'h', POPT_ARG_NONE,	NULL, 'h',
	N_("print this help."), NULL },
#endif	/* NOTYET */
  { "background", 'b', POPT_BIT_SET,	&rpmioWCTLFlags, WCTL_FLAGS_BACKGROUND,
	N_("go to background after startup."), NULL },
  { "execute", 'e', POPT_ARG_STRING,	&__rpmwget.execute_cmd, 0,
	N_("execute a `.wgetrc'-style command."), N_("COMMAND") },
  POPT_TABLEEND
};

/*@unchecked@*/ /*@observer@*/
static struct poptOption rpmioWCTLLoggingPoptTable[] = {
#ifndef	NOTYET /* XXX popt sub-tables should inherit callback from parent? */
/*@-type@*/ /* FIX: cast? */
 { NULL, '\0', POPT_ARG_CALLBACK | POPT_CBFLAG_INC_DATA | POPT_CBFLAG_CONTINUE,
        rpmwgetArgCallback, 0, (const char *)&__rpmwget, NULL },
/*@=type@*/
#endif

  { "output-file", 'o', POPT_ARG_STRING,	0, POPTWGET_LOGFILE,
	N_("log messages to FILE."), N_("FILE") },
  { "append-output", 'a', POPT_ARG_STRING,	0, POPTWGET_LOGAPPEND,
	N_("append log messages to FILE."), N_("FILE") },
  { "debug", 'd', POPT_ARG_VAL,	&__rpmwget.debug, -1,
	N_("print lots of debugging information."), NULL },
  { "quiet", 'q', POPT_ARG_VAL,	&__rpmwget.quiet, -1,
	N_("quiet (no output)."), NULL },
  { "verbose", 'v', POPT_ARG_VAL,	&__rpmwget.verbose, -1,
	N_("be verbose (this is the default)."), NULL },
  { "no-verbose", '\0', POPT_ARG_VAL,	&__rpmwget.verbose, 0,
	N_("turn off verboseness, without being quiet."), NULL },
  { "nv", '\0', POPT_ARG_VAL|POPT_ARGFLAG_ONEDASH|POPT_ARGFLAG_DOC_HIDDEN,	&__rpmwget.verbose, 0,
	N_("turn off verboseness, without being quiet."), NULL },
  { "input-file", 'i', POPT_ARG_ARGV,	&__rpmwget.manifests, 0,
	N_("download URLs found in FILE."), N_("FILE") },
  { "force-html", 'F', POPT_BIT_SET,	&rpmioWCTLFlags, WCTL_FLAGS_FORCEHTML,
	N_("treat input file as HTML."), NULL },
  { "base", 'B', POPT_ARG_STRING,	&__rpmwget.base_prefix, 0,
	N_("prepends URL to relative links in -F -i file."), N_("URL") },
  POPT_TABLEEND
};

/*@unchecked@*/ /*@observer@*/
static struct poptOption rpmioWDLPoptTable[] = {

 /* XXX popt sub-tables should inherit callback from parent? */
 { NULL, '\0', POPT_ARG_CALLBACK | POPT_CBFLAG_INC_DATA | POPT_CBFLAG_CONTINUE,
        rpmwgetArgCallback, 0, (const char *)&__rpmwget, NULL },

  { "tries", 't', POPT_ARG_INT,	&__rpmwget.ntries, 0,
	N_("set number of retries to NUMBER (0 unlimits)."), N_("NUMBER") },
  { "retry-connrefused", '\0', POPT_BIT_SET,	&__rpmwget.flags, WGET_FLAGS_RETRYCONN,
	N_("retry even if connection is refused."), NULL },
  { "output-document", 'O', POPT_ARG_STRING,	&__rpmwget.document_file, 0,
	N_("write documents to FILE."), N_("FILE") },
  { "no-clobber", '\0', POPT_BIT_SET,	&__rpmwget.flags, WGET_FLAGS_NOCLOBBER,
	N_("skip downloads that would download to existing files."), NULL },
  { "nc", '\0', POPT_BIT_SET|POPT_ARGFLAG_ONEDASH|POPT_ARGFLAG_DOC_HIDDEN,	&__rpmwget.flags, WGET_FLAGS_NOCLOBBER,
	N_("skip downloads that would download to existing files."), NULL },
  { "continue", 'c', POPT_BIT_SET,	&__rpmwget.flags, WGET_FLAGS_RESUME,
	N_("resume getting a partially-downloaded file."), NULL },
  { "progress", '\0', POPT_ARG_STRING,		NULL, POPTWGET_PROGRESS,
	N_("select progress gauge type."), N_("TYPE") },
  { "timestamping", 'N', POPT_BIT_SET,	&__rpmwget.flags, WGET_FLAGS_NEWERONLY,
	N_("don't re-retrieve files unless newer than local."), NULL },
  { "server-response", 'S', POPT_BIT_SET,	&rpmioWDLFlags, WDL_FLAGS_SERVERRESPONSE,
	N_("print server response."), NULL },
  { "spider", '\0', POPT_BIT_SET,	&__rpmwget.flags, WGET_FLAGS_NODOWNLOAD,
	N_("don't download anything."), NULL },
  { "timeout", 'T', POPT_ARG_INT,	&__rpmwget.timeout_secs, 0,
	N_("set all timeout values to SECONDS."), N_("SECONDS") },
  { "dns-timeout", '\0', POPT_ARG_INT,	&__rpmwget.dns_timeout_secs, 0,
	N_("set the DNS lookup timeout to SECS."), N_("SECONDS") },
  { "connect-timeout", '\0', POPT_ARG_INT,	&__rpmwget.connect_timeout_secs, 0,
	N_("set the connect timeout to SECS."), N_("SECONDS") },
  { "read-timeout", '\0', POPT_ARG_INT,	&__rpmwget.read_timeout_secs, 0,
	N_("set the read timeout to SECS."), N_("SECONDS") },
  { "wait", 'w', POPT_ARG_INT,	&__rpmwget.wait_secs, 0,
	N_("wait SECONDS between retrievals."), N_("SECONDS") },
/* XXX POPT_ARGFLAG_RANDOM? */
  { "waitretry", '\0', POPT_ARG_INT,	&__rpmwget.waitretry_secs, 0,
	N_("wait 1..SECONDS between retries of a retrieval."), N_("SECONDS") },
/* XXX POPT_ARGFLAG_RANDOM */
  { "random-wait", '\0', POPT_ARG_INT,	&__rpmwget.randomwaitretry_secs, 0,
	N_("wait from 0...2*WAIT secs between retrievals."), NULL },
  { "no-proxy", '\0', POPT_BIT_SET,	&rpmioWDLFlags, WDL_FLAGS_NOPROXY,
	N_("explicitly turn off proxy."), NULL },
  { "quota", 'Q', POPT_ARG_STRING,	0, POPTWGET_QUOTA,
	N_("set retrieval quota to NUMBER."), N_("NUMBER") },

/* XXX add IP addr parsing */
  { "bind-address", '\0', POPT_ARG_INT,	&__rpmwget.bind_address, 0,
	N_("bind to ADDRESS (hostname or IP) on local host."), N_("ADDRESS") },

  { "limit-rate", '\0', POPT_ARG_STRING,	NULL, POPTWGET_LIMITRATE,
	N_("limit download rate to RATE."), N_("RATE") },

  { "no-dns-cache", '\0', POPT_BIT_SET,	&rpmioWDLFlags, WDL_FLAGS_NODNSCACHE,
	N_("disable caching DNS lookups."), NULL },
  { "restrict-file-names", '\0', POPT_ARG_STRING,	NULL, POPTWGET_RESTRICT,
	N_("restrict chars in file names to ones OS allows."), N_("OS") },
  { "ignore-case", '\0', POPT_BIT_SET,	&rpmioWDLFlags, WDL_FLAGS_IGNORECASE,
	N_("ignore case when matching files/directories."), NULL },
  { "inet4-only", '4', POPT_BIT_SET,	&rpmioWDLFlags, WDL_FLAGS_INET4,
	N_("connect only to IPv4 addresses."), NULL },
  { "inet6-only", '6', POPT_BIT_SET,	&rpmioWDLFlags, WDL_FLAGS_INET6,
	N_("connect only to IPv6 addresses."), NULL },
  { "prefer-family", '\0', POPT_ARG_STRING,	&__rpmwget.prefer, 0,
	N_("connect first to addresses of specified family, one of IPv6, IPv4, or none."), N_("FAMILY") },
  { "user", '\0', POPT_ARG_STRING,	&__rpmwget.user, 0,
	N_("set both ftp and http user to USER."), N_("USER") },
  { "password", '\0', POPT_ARG_STRING,	&__rpmwget.password, 0,
	N_("set both ftp and http password to PASS."), N_("PASS") },
  POPT_TABLEEND
};

/*@unchecked@*/ /*@observer@*/
static struct poptOption rpmioWDirPoptTable[] = {
  { "no-directories", '\0', POPT_BIT_SET,	&rpmioWDirFlags, WDIR_FLAGS_NOCREATE,
	N_("don't create directories."), NULL },
  { "nd", '\0', POPT_BIT_SET|POPT_ARGFLAG_ONEDASH|POPT_ARGFLAG_DOC_HIDDEN,	&rpmioWDirFlags, WDIR_FLAGS_NOCREATE,
	N_("don't create directories."), NULL },
  { "force-directories", 'x', POPT_BIT_SET,	&rpmioWDirFlags, WDIR_FLAGS_FORCE,
	N_("force creation of directories."), NULL },
  { "no-host-directories", '\0', POPT_BIT_SET,	&rpmioWDirFlags, WDIR_FLAGS_NOHOST,
	N_("don't create host directories."), NULL },
  { "nH", '\0', POPT_BIT_SET|POPT_ARGFLAG_ONEDASH|POPT_ARGFLAG_DOC_HIDDEN, &rpmioWDirFlags, WDIR_FLAGS_NOHOST,
	N_("don't create host directories."), NULL },
  { "protocol-directories", '\0', POPT_BIT_SET,	&rpmioWDirFlags, WDIR_FLAGS_ADDPROTO,
	N_("use protocol name in directories."), NULL },
  { "directory-prefix", 'P', POPT_ARG_STRING,	&__rpmwget.dir_prefix, 0,
	N_("save files to PREFIX/..."), N_("PREFIX") },
  { "cut-dirs", '\0', POPT_ARG_INT,	&__rpmwget.dir_cut, 0,
	N_("ignore NUMBER remote directory components."), N_("NUMBER") },
  POPT_TABLEEND
};

/*@unchecked@*/ /*@observer@*/
static struct poptOption rpmioHttpPoptTable[] = {
  { "http-user", '\0', POPT_ARG_STRING,	&__rpmwget.http_user, 0,
	N_("set http user to USER."), N_("USER") },
  { "http-password", '\0', POPT_ARG_STRING,	&__rpmwget.http_password, 0,
	N_("set http password to PASS."), N_("PASS") },
  { "no-cache", '\0', POPT_BIT_SET,	&rpmioHttpFlags, HTTP_FLAGS_NOCACHE,
	N_("disallow server-cached data."), NULL },
  { "html-extension", 'E', POPT_ARG_STRING,	&__rpmwget.http_ext, 0,
	N_("save HTML documents with `.html' extension."), NULL },
  { "ignore-length", '\0', POPT_BIT_SET,&rpmioHttpFlags, HTTP_FLAGS_IGNORELENGTH,
	N_("ignore `Content-Length' header field."), NULL },
/* XXX ARGV */
  { "header", '\0', POPT_ARG_STRING,	&__rpmwget.http_header, 0,
	N_("insert STRING among the headers."), N_("STRING") },
  { "max-redirect", '\0', POPT_ARG_INT,	&__rpmwget.http_max_redirect, 0,
	N_("maximum redirections allowed per page."), N_("NUM") },
  { "proxy-user", '\0', POPT_ARG_STRING,	&__rpmwget.http_proxy_user, 0,
	N_("set USER as proxy username."), N_("USER") },
  { "proxy-password", '\0', POPT_ARG_STRING,	&__rpmwget.http_proxy_password, 0,
	N_("set PASS as proxy password."), N_("PASS") },
  { "referer", '\0', POPT_ARG_STRING,	&__rpmwget.http_referer, 0,
	N_("include `Referer: URL' header in HTTP request."), N_("URL") },
  { "save-headers", '\0', POPT_BIT_SET,	&rpmioHttpFlags, HTTP_FLAGS_SAVEHEADERS,
	N_("save the HTTP headers to file."), NULL },
  { "user-agent", 'U', POPT_ARG_STRING,	&__rpmwget.http_user_agent, 0,
	N_("identify as AGENT instead of Wget/VERSION."), N_("AGENT") },
  { "no-http-keep-alive", '\0', POPT_BIT_SET,	&rpmioHttpFlags, HTTP_FLAGS_NOKEEPALIVE,
	N_("disable HTTP keep-alive (persistent connections)."), NULL },
  { "no-cookies", '\0', POPT_BIT_SET,	&rpmioHttpFlags, HTTP_FLAGS_NOCOOKIES,
	N_("don't use cookies."), NULL },
  { "load-cookies", '\0', POPT_ARG_STRING,	&__rpmwget.http_load_cookies_file, 0,
	N_("load cookies from FILE before session."), N_("FILE") },
  { "save-cookies", '\0', POPT_ARG_STRING,	&__rpmwget.http_save_cookies_file, 0,
	N_("save cookies to FILE after session."), N_("FILE") },
  { "keep-session-cookies", '\0', POPT_BIT_SET,	&rpmioHttpFlags, HTTP_FLAGS_KEEPCOOKIES,
	N_("load and save session (non-permanent) cookies."), NULL },
  { "post-data", '\0', POPT_ARG_STRING,	&__rpmwget.http_post_data, 0,
	N_("use the POST method; send STRING as the data."), N_("STRING") },
  { "post-file", '\0', POPT_ARG_STRING,	&__rpmwget.http_post_file, 0,
	N_("use the POST method; send contents of FILE."), N_("FILE") },
  { "content-disposition", '\0', POPT_BIT_SET,	&rpmioHttpFlags, 0,
	N_("honor the Content-Disposition header when choosing local file names (EXPERIMENTAL)."), NULL },
/* XXX negated options */
  { "no-content-disposition", '\0', POPT_BIT_CLR|POPT_ARGFLAG_DOC_HIDDEN, &rpmioHttpFlags, 0,
	N_("honor the Content-Disposition header when choosing local file names (EXPERIMENTAL)."), NULL },
  { "auth-no-challenge", '\0', POPT_BIT_SET,	&rpmioHttpFlags, HTTP_FLAGS_NOCHALLENGE,
	N_("Send Basic HTTP authentication information without first waiting for the server's challenge."), NULL },
  POPT_TABLEEND
};

/*@unchecked@*/ /*@observer@*/
static struct poptOption rpmioHttpsPoptTable[] = {
  { "secure-protocol", '\0', POPT_ARG_STRING,	&__rpmwget.https_protocol, 0,
	N_("choose secure protocol, one of auto, SSLv2, SSLv3, and TLSv1."), N_("PR") },
  { "no-check-certificate", '\0', POPT_BIT_SET,	&rpmioHttpsFlags, HTTPS_FLAGS_NOCHECK,
	N_("don't validate the server's certificate."), NULL },
  { "certificate", '\0', POPT_ARG_STRING,	&__rpmwget.https_certificate_file, 0,
	N_("client certificate file."), N_("FILE") },
  { "certificate-type", '\0', POPT_ARG_STRING,	&__rpmwget.https_certificate_type, 0,
	N_("client certificate type, PEM or DER."), N_("TYPE") },
  { "private-key", '\0', POPT_ARG_STRING,	&__rpmwget.https_privatekey_file, 0,
	N_("private key file."), N_("FILE") },
  { "private-key-type", '\0', POPT_ARG_STRING,	&__rpmwget.https_privatekey_type, 0,
	N_("private key type, PEM or DER."), N_("TYPE") },
  { "ca-certificate", '\0', POPT_ARG_STRING,	&__rpmwget.https_cacertificate_file, 0,
	N_("file with the bundle of CA's."), N_("FILE") },
  { "ca-directory", '\0', POPT_ARG_STRING,	&__rpmwget.https_cacertificate_dir, 0,
	N_("directory where hash list of CA's is stored."), N_("DIR") },
  { "random-file", '\0', POPT_ARG_STRING,	&__rpmwget.https_random_file, 0,
	N_("file with random data for seeding the SSL PRNG."), N_("FILE") },
  { "egd-file", '\0', POPT_ARG_NONE,		&__rpmwget.https_egd_file, 0,
	N_("file naming the EGD socket with random data."), N_("FILE") },
  POPT_TABLEEND
};

/*@unchecked@*/ /*@observer@*/
static struct poptOption rpmioFtpPoptTable[] = {
  { "ftp-user", '\0', POPT_ARG_STRING,		&__rpmwget.ftp_user, 0,
	N_("set ftp user to USER."), N_("USER") },
  { "ftp-password", '\0', POPT_ARG_STRING,	&__rpmwget.ftp_password, 0,
	N_("set ftp password to PASS."), N_("PASS") },
  { "no-remove-listing", '\0', POPT_BIT_SET,	&rpmioFtpFlags, FTP_FLAGS_KEEPLISTING,
	N_("don't remove `.listing' files."), NULL },
  { "no-glob", '\0', POPT_BIT_SET,		&rpmioFtpFlags, FTP_FLAGS_NOGLOB,
	N_("turn off FTP file name globbing."), NULL },
  { "no-passive-ftp", '\0', POPT_BIT_SET,	&rpmioFtpFlags, FTP_FLAGS_ACTIVE,
	N_("disable the \"passive\" transfer mode."), NULL },
  { "retr-symlinks", '\0', POPT_BIT_SET,	&rpmioFtpFlags, FTP_FLAGS_FOLLOW,
	N_("when recursing, get linked-to files (not dir)."), NULL },
  { "preserve-permissions", '\0', POPT_BIT_SET,	&rpmioFtpFlags, FTP_FLAGS_PRESERVE,
	N_("preserve remote file permissions."), NULL },
  POPT_TABLEEND
};

/*@unchecked@*/ /*@observer@*/
static struct poptOption rpmioWCTLDownloadPoptTable[] = {
  { "recursive", 'r', POPT_BIT_SET,	&rpmioWCTLFlags, WCTL_FLAGS_RECURSE,
	N_("specify recursive download."), NULL },
  { "level", 'l', POPT_ARG_INT,	&__rpmwget.recurse_max, 0,
	N_("maximum recursion depth (inf or 0 for infinite)."), N_("NUMBER") },
  { "delete-after", '\0', POPT_BIT_SET,	&rpmioWCTLFlags, WCTL_FLAGS_DELETEAFTER,
	N_("delete files locally after downloading them."), NULL },
  { "convert-links", 'k', POPT_BIT_SET,	&rpmioWCTLFlags, WCTL_FLAGS_CONVERTLINKS,
	N_("make links in downloaded HTML point to local files."), NULL },
  { "backup-converted", 'K', POPT_BIT_SET,	&rpmioWCTLFlags, WCTL_FLAGS_BACKUP,
	N_("before converting file X, back up as X.orig."), NULL },
/*XXX popt alias? */
  { "mirror", 'm', POPT_BIT_SET,	&rpmioWCTLFlags, WCTL_FLAGS_MIRROR,
	N_("shortcut for -N -r -l inf --no-remove-listing."), NULL },
  { "page-requisites", 'p', POPT_BIT_SET,	&rpmioWCTLFlags, WCTL_FLAGS_PAGEREQUISITES,
	N_("get all images, etc. needed to display HTML page."), NULL },
  { "strict-comments", '\0', POPT_BIT_SET,	&rpmioWCTLFlags, WCTL_FLAGS_STRICTCOMMENTS,
	N_("turn on strict (SGML) handling of HTML comments."), NULL },
  POPT_TABLEEND
};

/*@unchecked@*/ /*@observer@*/
static struct poptOption rpmioWCTLAcceptPoptTable[] = {
  { "accept", 'A', POPT_ARG_STRING,	&__rpmwget.accept_exts, 0,
	N_("comma-separated list of accepted extensions."), N_("LIST") },
  { "reject", 'R', POPT_ARG_STRING,	&__rpmwget.reject_exts, 0,
	N_("comma-separated list of rejected extensions."), N_("LIST") },
  { "domains", 'D', POPT_ARG_STRING,	&__rpmwget.accept_domains, 0,
	N_("comma-separated list of accepted domains."), N_("LIST") },
  { "exclude-domains", '\0', POPT_ARG_STRING,	&__rpmwget.reject_domains, 0,
	N_("comma-separated list of rejected domains."), N_("LIST") },
  { "follow-ftp", '\0', POPT_BIT_SET,	&rpmioWCTLFlags, WCTL_FLAGS_FOLLOWFTP,
	N_("follow FTP links from HTML documents."), NULL },
  { "follow-tags", '\0', POPT_ARG_STRING,	&__rpmwget.accept_tags, 0,
	N_("comma-separated list of followed HTML tags."), N_("LIST") },
  { "ignore-tags", '\0', POPT_ARG_STRING,	&__rpmwget.reject_tags, 0,
	N_("comma-separated list of ignored HTML tags."), N_("LIST") },
  { "span-hosts", 'H', POPT_BIT_SET,	&rpmioWCTLFlags, WCTL_FLAGS_SPANHOSTS,
	N_("go to foreign hosts when recursive."), NULL },
  { "relative", 'L', POPT_BIT_SET,	&rpmioWCTLFlags, WCTL_FLAGS_RELATIVEONLY,
	N_("follow relative links only."), NULL },
  { "include-directories", 'I', POPT_ARG_STRING,	&__rpmwget.accept_dirs, 0,
	N_("list of allowed directories."), N_("LIST") },
  { "exclude-directories", 'X', POPT_ARG_STRING,	&__rpmwget.reject_dirs, 0,
	N_("list of excluded directories."), N_("LIST") },
  { "no-parent", '\0', POPT_BIT_SET,	&rpmioWCTLFlags, WCTL_FLAGS_NOPARENT,
	N_("don't ascend to the parent directory."), NULL },
  { "np", '\0', POPT_BIT_SET|POPT_ARGFLAG_ONEDASH|POPT_ARGFLAG_DOC_HIDDEN,	&rpmioWCTLFlags, WCTL_FLAGS_NOPARENT,
	N_("don't ascend to the parent directory."), NULL },
  POPT_TABLEEND
};

/*@unchecked@*/ /*@observer@*/
static struct poptOption optionsTable[] = {
#ifdef	NOTYET /* XXX popt sub-tables should inherit callback from parent? */
/*@-type@*/ /* FIX: cast? */
 { NULL, '\0', POPT_ARG_CALLBACK | POPT_CBFLAG_INC_DATA | POPT_CBFLAG_CONTINUE,
        rpmwgetArgCallback, 0, (const char *)&__rpmwget, NULL },
/*@=type@*/
#endif

  { NULL, '\0', POPT_ARG_INCLUDE_TABLE, rpmioWCTLStartupPoptTable, 0,
	N_("Startup:"), NULL },

  { NULL, '\0', POPT_ARG_INCLUDE_TABLE, rpmioWCTLLoggingPoptTable, 0,
	N_("Logging and input file:"), NULL },

  { NULL, '\0', POPT_ARG_INCLUDE_TABLE, rpmioWDLPoptTable, 0,
	N_("Download:"), NULL },

  { NULL, '\0', POPT_ARG_INCLUDE_TABLE, rpmioWDirPoptTable, 0,
	N_("Directories:"), NULL },

  { NULL, '\0', POPT_ARG_INCLUDE_TABLE, rpmioHttpPoptTable, 0,
	N_("HTTP options:"), NULL },

  { NULL, '\0', POPT_ARG_INCLUDE_TABLE, rpmioHttpsPoptTable, 0,
	N_("HTTPS (SSL/TLS) options:"), NULL },

  { NULL, '\0', POPT_ARG_INCLUDE_TABLE, rpmioFtpPoptTable, 0,
	N_("FTP options:"), NULL },

  { NULL, '\0', POPT_ARG_INCLUDE_TABLE, rpmioWCTLDownloadPoptTable, 0,
	N_("Recursive download:"), NULL },

  { NULL, '\0', POPT_ARG_INCLUDE_TABLE, rpmioWCTLAcceptPoptTable, 0,
	N_("Recursive accept/reject:"), NULL },

 { NULL, '\0', POPT_ARG_INCLUDE_TABLE, rpmioAllPoptTable, 0,
	N_("Common options for all rpmio executables:"), NULL },

  POPT_AUTOALIAS
  POPT_AUTOHELP

  { NULL, (char)-1, POPT_ARG_INCLUDE_TABLE, NULL, 0,
	N_("\
Usage: rpmwget [OPTION]... [URL]...\n\
\n\
Mandatory arguments to long options are mandatory for short options too.\n\
\n\
Mail bug reports and suggestions to <rpm-devel@rpm5.org>.\n\
"), NULL },

  POPT_TABLEEND

};

int
main(int argc, char *argv[])
	/*@globals __assert_program_name,
		rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
	/*@modifies __assert_program_name, _rpmrepo,
		rpmGlobalMacroContext, fileSystem, internalState @*/
{
    rpmwget wget = _rpmwget;
    poptContext optCon;
    const char ** av = NULL;
    int rc = 0;		/* assume success. */
    int xx;
    int i;

/*@-observertrans -readonlytrans @*/
    __progname = "rpmwget";
/*@=observertrans =readonlytrans @*/

    /* Initialize configuration. */
    /* XXX read system/user wgetrc */
    wget->blen = 32 * BUFSIZ;
    wget->b = xmalloc(wget->blen);
    wget->b[0] = '\0';
    wget->st = xmalloc(sizeof(*wget->st));

rpmioHttpAccept = "*/*";	/* XXX wget compatible. */
if (wget->debug < 0) {
_av_debug = -1;
_ftp_debug = -1;
_dav_debug = 1;
#if 0
_url_debug = -1;
_rpmio_debug = -1;
#endif
}

    /* Set default configuration (if not already specified). */

    optCon = rpmioInit(argc, argv, optionsTable);
    if (wget->debug < 0) {
	fprintf(stderr, "==> %s", __progname);
	for (i = 1; argv[i] != NULL; i++) {
	    fprintf(stderr, " %s", argv[i]);
	}
	fprintf(stderr, "\n");
    }

    /* Sanity check configuration. */
    if (wget->bartype == NULL)
	wget->bartype = xstrdup("bar");

    /* Open log file (if requested). */
    if (wget->lfn != NULL) {
	if (wget->lfmode == NULL)
	    wget->lfmode = "w";
	wget->lfd = Fopen(wget->lfn, wget->lfmode);
	if (wget->lfd == NULL || Ferror(wget->lfd)) {
fprintf(stderr, "%s: %s: %s\n", __progname, wget->lfn, Fstrerror(wget->lfd));
	    goto exit;
	}
    }

    /* Initialize the progress callback. */
    urlNotify = wgetNotify;
    urlNotifyArg = wget;

    /* Gather arguments. */
    av = poptGetArgs(optCon);
    if (av != NULL)
	xx = argvAppend(&wget->argv, av);
    if (wget->manifests != NULL)
	xx = wgetLoadManifests(wget);

    if (wget->argv == NULL || wget->argv[0] == NULL) {
	poptPrintUsage(optCon, stderr, 0);
	goto exit;
    }

    xx = rpmswEnter(&wget->Top, 1);
    if (wget->argv != NULL)
    for (i = 0; wget->argv[i] != NULL; i++) {
	wget->ifn = xstrdup(wget->argv[i]);
	if ((xx = wgetCopyRetry(_rpmwget)) != 0)
	    rc = 256;
	wget->ifn = _free(wget->ifn);
    }
    xx = rpmswExit(&wget->Top, 1);

exit:
    if (wget->lfd !=NULL) {
	(void) Fclose(wget->lfd);
	wget->lfd = NULL;
    }
    wget->b = _free(wget->b);
    wget->st = _free(wget->st);
    wget->lfn = _free(wget->lfn);
    wget->ifn = _free(wget->ifn);
    wget->ofn = _free(wget->ofn);

    wget->argv = argvFree(wget->argv);
    wget->manifests = argvFree(wget->manifests);
    wget->bartype = _free(wget->bartype);

    optCon = rpmioFini(optCon);

    return rc;
}
