#ifndef H_RPMURL
#define H_RPMURL

/** \ingroup rpmio
 * \file rpmio/rpmurl.h
 */

#include <assert.h>
#include <rpmio.h>
#include <rpmsw.h>

/**
 * Supported URL types.
 */
typedef enum urltype_e {
    URL_IS_UNKNOWN	= 0,	/*!< unknown (aka a file) */
    URL_IS_DASH		= 1,	/*!< stdin/stdout */
    URL_IS_PATH		= 2,	/*!< file://... */
    URL_IS_FTP		= 3,	/*!< ftp://... */
    URL_IS_HTTP		= 4,	/*!< http://... */
    URL_IS_HTTPS	= 5,	/*!< https://... */
    URL_IS_HKP		= 6,	/*!< hkp://... */
    URL_IS_MONGO	= 7,	/*!< mongo://... and mongodb://... */
} urltype;
#define	URL_IS_GIT	(urltype)16
#define	URL_IS_SVN	(urltype)17
#define	URL_IS_SQLITE	(urltype)32
#define	URL_IS_MYSQL	(urltype)33
#define	URL_IS_POSTGRES	(urltype)34
#define	URL_IS_SQLSERVER (urltype)35

#define	URLMAGIC	0xd00b1ed0U
#define	URLSANE(u)	assert(u && u->magic == URLMAGIC)

/**
 */
typedef /*@abstract@*/ /*@refcounted@*/ struct urlinfo_s * urlinfo;

/**
 */
extern int (*urlNotify) (const urlinfo u, unsigned status)
	/*@*/;

/**
 */
/*@unchecked@*/ /*@null@*/ /*@shared@*/
extern void * urlNotifyArg;

/**
 * URL control structure.
 */
struct urlinfo_s {
    struct rpmioItem_s _item;	/*!< usage mutex and pool identifier. */
/*@owned@*/ /*@relnull@*/
    const char * url;		/*!< copy of original url */
/*@owned@*/ /*@relnull@*/
    const char * scheme;	/*!< URI scheme. */
/*@owned@*/ /*@null@*/
    const char * user;		/*!< URI user. */
/*@owned@*/ /*@null@*/
    const char * password;	/*!< URI password. */
/*@owned@*/ /*@relnull@*/
    const char * host;		/*!< URI host. */
/*@owned@*/ /*@null@*/
    const char * portstr;	/*!< URI port string. */
/*@owned@*/ /*@null@*/
    const char * query;		/*!< URI ?query. */
/*@owned@*/ /*@null@*/
    const char * fragment;	/*!< URI #fragment. */
/*@owned@*/ /*@null@*/
    const char * proxyu;	/*!< FTP: proxy user */
/*@owned@*/ /*@null@*/
    const char * proxyh;	/*!< FTP/HTTP: proxy host */
    int proxyp;			/*!< FTP/HTTP: proxy port */
    int	port;			/*!< URI port. */
    urltype ut;			/*!< URI type. */
/*@relnull@*/
    FD_t ctrl;			/*!< control channel */
/*@relnull@*/
    FD_t data;			/*!< per-xfer data channel */

/*@relnull@*/
    void * capabilities;	/*!< neon: ne_server_capabilities ptr */
/*@relnull@*/
    void * lockstore;		/*!< neon: ne_lock_store ptr */
/*@relnull@*/
    void * sess;		/*!< neon: ne_session ptr */

/*@null@*/
    const char * location;	/*!< Location: tag. */
/*@null@*/
    const char * etag;		/*!< ETag: tag. */
/*@null@*/
    int (*notify) (const urlinfo u, unsigned status);
/*@null@*/ /*@shared@*/
    void * arg;
    struct fdNotify_s {
	unsigned status;
/*@null@*/
	const char * hostname;
/*@null@*/
	const char * address;
	int64_t progress;
	int64_t total;
    } info;
/*@null@*/
    rpmop rop;			/*!< Receive accumulator. */
/*@null@*/
    rpmop sop;			/*!< Send accumulator. */
/*@null@*/
    rpmop top;			/*!< Total accumulator. */

    int bufAlloced;		/*!< sizeof I/O buffer */
/*@owned@*/
    char * buf;			/*!< I/O buffer */
    int openError;		/*!< Type of open failure */
    int httpVersion;
    int allow;
#define	RPMURL_SERVER_HASRANGE		( 1 << 0)
#define	RPMURL_SERVER_HASDAVCLASS1	( 1 << 1)
#define	RPMURL_SERVER_HASDAVCLASS2	( 1 << 2)
#define	RPMURL_SERVER_HASDAVEXEC	( 1 << 3)

#define	RPMURL_SERVER_OPTIONSDONE	( 1 << 8)

#define	RPMURL_SERVER_HASDAV	(RPMURL_SERVER_HASDAVCLASS1|RPMURL_SERVER_HASDAVCLASS2|RPMURL_SERVER_HASDAVEXEC)
    unsigned magic;
#if defined(__LCLINT__)
/*@refs@*/
    int nrefs;			/*!< (unused) keep splint happy */
#endif
};

#ifdef __cplusplus
extern "C" {
#endif

/*@unchecked@*/
extern int _url_count;		/*!< No. of cached URL's. */

/*@unchecked@*/
/*@only@*/ /*@null@*/
extern urlinfo * _url_cache;	/*!< URL cache. */

/*@unchecked@*/
extern int _url_iobuf_size;	/*!< Initial size of URL I/O buffer. */
#define RPMURL_IOBUF_SIZE	4096

/*@unchecked@*/
extern int _url_debug;		/*!< URL debugging? */
#define RPMURL_DEBUG_IO		0x40000000
#define RPMURL_DEBUG_REFS	0x20000000

/**
 * Create a URL control structure instance.
 * @param msg		debugging identifier (unused)
 * @return		new instance
 */
/*@unused@*/ /*@null@*/
urlinfo	urlNew(const char * msg)
	/*@*/;

/** @todo Remove debugging entry from the ABI. */
/*@null@*/
urlinfo	XurlNew(const char * msg, const char * fn, unsigned ln)
	/*@globals fileSystem @*/
	/*@modifies fileSystem @*/;
#define	urlNew(_msg) XurlNew(_msg, __FILE__, __LINE__)

/**
 * Reference a URL control structure instance.
 * @param u		URL control structure
 * @param msg		debugging identifier (unused)
 * @return		referenced instance
 */
/*@unused@*/ /*@newref@*/
urlinfo	urlLink(/*@returned@*/ urlinfo u, const char * msg)
	/*@modifies u @*/;
#define	urlLink(_u, _msg)	\
	(urlinfo) rpmioLinkPoolItem((rpmioItem)(_u), _msg, __FILE__, __LINE__)

/**
 * Dereference a URL control structure instance.
 * @param u		URL control structure
 * @param msg		debugging identifier (unused)
 * @return		dereferenced instance (NULL if freed)
 */
/*@unused@*/ /*@null@*/
urlinfo	urlFree( /*@killref@*/ urlinfo u, const char * msg)
	/*@globals fileSystem, internalState @*/
	/*@modifies u, fileSystem, internalState @*/;
#define	urlFree(_u, _msg)	\
	((urlinfo)rpmioFreePoolItem((rpmioItem)(_u), _msg, __FILE__, __LINE__))

/**
 * Free cached URL control structures.
 */
void urlFreeCache(void)
	/*@globals _url_cache, _url_count, fileSystem, internalState @*/
	/*@modifies _url_cache, _url_count, fileSystem, internalState @*/;

/**
 * Return type of URL.
 * @param url		url string
 * @return		type of url
 */
urltype	urlIsURL(const char * url)
	/*@*/;
urltype	urlType(void * _u)
	/*@*/;

/**
 * Return path component of URL.
 * @param url		url string
 * @retval pathp	pointer to path component of url
 * @return		type of url
 */
/*@-incondefs@*/
urltype	urlPath(const char * url, /*@out@*/ const char ** pathp)
	/*@modifies *pathp @*/;
/*@=incondefs@*/

/**
 * Parse URL string into a control structure.
 * @param url		url string
 * @retval uret		address of new control instance pointer
 * @return		0 on success, -1 on error
 */
int urlSplit(const char * url, /*@out@*/ urlinfo * uret)
	/*@globals h_errno, internalState @*/
	/*@modifies *uret, internalState @*/;

/**
 * Copy data from URL to local file.
 * @param url		url string of source
 * @param dest		file name of destination
 * @return		0 on success, otherwise FTPERR_* code
 */
int urlGetFile(const char * url, /*@null@*/ const char * dest)
	/*@globals h_errno, fileSystem, internalState @*/
	/*@modifies fileSystem, internalState @*/;

#ifdef __cplusplus
}
#endif

#endif	/* H_RPMURL */
