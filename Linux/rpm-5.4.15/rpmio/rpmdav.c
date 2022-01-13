/*@-modfilesys@*/
/** \ingroup rpmio
 * \file rpmio/rpmdav.c
 */

#include "system.h"

#ifdef WITH_NEON

#include "ne_alloc.h"
#include "ne_auth.h"
#include "ne_basic.h"
#include "ne_dates.h"
#include "ne_locks.h"

#define	NEONBLOWSCHUNKS
#ifndef	NEONBLOWSCHUNKS
/* HACK: include ne_private.h to access sess->socket for now. */
#include "../neon/src/ne_private.h"
#endif

#include "ne_props.h"
#include "ne_request.h"
#include "ne_socket.h"
#include "ne_string.h"

#include "ne_utils.h"
#if !defined(HEADER_ERR_H)
/* cheats to avoid having to explicitly build against OpenSSL */
/*@-exportheader -redecl @*/
#ifdef __cplusplus
extern "C" {
#endif
extern void ERR_remove_state(int foo);
extern void ENGINE_cleanup(void);
extern void CONF_modules_unload(int foo);
extern void ERR_free_strings(void);
extern void EVP_cleanup(void);
extern void CRYPTO_cleanup_all_ex_data(void);
extern void CRYPTO_mem_leaks(void * ptr);
#ifdef __cplusplus
}
#endif
/*@=exportheader =redecl @*/
#endif

#include "ne_md5.h" /* for version detection only */

/* poor-man's NEON version determination */
#if defined(NE_MD5_H)
#define WITH_NEON_MIN_VERSION 0x002700
#elif defined(NE_FEATURE_I18N)
#define WITH_NEON_MIN_VERSION 0x002600
#else
#define WITH_NEON_MIN_VERSION 0x002500
#endif

/* XXX API changes for NEON 0.26 */
#if WITH_NEON_MIN_VERSION >= 0x002600
#define	ne_propfind_set_private(_pfh, _create_item, NULL) \
	ne_propfind_set_private(_pfh, _create_item, NULL, NULL)
#endif

#endif /* WITH_NEON */

#include <rpmio_internal.h>

#include <rpmhash.h>
#include <rpmmacro.h>		/* XXX rpmExpand */
#include <ugid.h>

#define	_RPMDIR_INTERNAL
#include <rpmdir.h>
#define _RPMDAV_INTERNAL
#include <rpmdav.h>
#include <mire.h>

#include "debug.h"

#define	DAVDEBUG(_f, _list) \
    if (((_f) < 0 && _dav_debug < 0) || ((_f) > 0 && _dav_debug)) \
	fprintf _list

/*@access DIR @*/
/*@access FD_t @*/
/*@access urlinfo @*/
/*@access miRE @*/

/* HACK: reasonable value needed (wget uses 900 as default). */
#if 0
#define READ_TIMEOUT_SECS	120	/* neon-0.28.5 default */
#define CONNECT_TIMEOUT_SECS	0	/* neon-0.28.5 default */
#else
#define READ_TIMEOUT_SECS	120
#define CONNECT_TIMEOUT_SECS	0	/* connect(2) EINPROGRESS if too low. */
#endif

/*@unchecked@*/ /*@observer@*/
static const char _rpmioHttpUserAgent[] = PACKAGE "/" PACKAGE_VERSION;

/*@unchecked@*/
static int rpmioHttpPersist = 1;
/*@unchecked@*/
int rpmioHttpReadTimeoutSecs = READ_TIMEOUT_SECS;
/*@unchecked@*/
int rpmioHttpConnectTimeoutSecs = CONNECT_TIMEOUT_SECS;
#ifdef	NOTYET
int rpmioHttpRetries = 20;
int rpmioHttpRecurseMax = 5;
int rpmioHttpMaxRedirect = 20;
#endif

/*@unchecked@*/ /*@null@*/
const char * rpmioHttpAccept;
/*@unchecked@*/ /*@null@*/
const char * rpmioHttpUserAgent;

#ifdef WITH_NEON
/* =============================================================== */
/*@-mustmod@*/
int davDisconnect(/*@unused@*/ void * _u)
{
    urlinfo u = (urlinfo) _u;
    int rc = 0;

#if WITH_NEON_MIN_VERSION >= 0x002700
    rc = (u->info.status == ne_status_sending || u->info.status == ne_status_recving);
#endif
    if (u != NULL) {
#ifdef	NOTYET
	if (u->ctrl->req != NULL) {
	    if (u->ctrl && u->ctrl->req) {
		ne_request_destroy((ne_request *)u->ctrl->req);
		u->ctrl->req = NULL;
	    }
	    if (u->data && u->data->req) {
		ne_request_destroy(u->data->req);
		u->data->req = NULL;
	    }
	}
#else
#ifdef	STILL_NOTYET	/* XXX closer but no cigar */
	if (u->sess != NULL)
	    ne_close_connection((ne_session *)u->sess);
#endif
#endif
    }
DAVDEBUG(-1, (stderr, "<-- %s(%p) active %d\n", __FUNCTION__, u, rc));
    rc = 0;	/* XXX return active state? */
    return rc;
}
/*@=mustmod@*/

int davFree(urlinfo u)
{
    if (u != NULL) {
	if (u->sess != NULL) {
	    ne_session_destroy((ne_session *)u->sess);
	    u->sess = NULL;
	}
	switch (urlType(u)) {
	default:
	    /*@notreached@*/ break;
	case URL_IS_HTTPS:
	case URL_IS_HTTP:
	case URL_IS_HKP:
	    u->capabilities = _free(u->capabilities);
	    if (u->lockstore != NULL)
		ne_lockstore_destroy((ne_lock_store *)u->lockstore);
	    u->lockstore = NULL;
	    u->info.status = 0;
	    ne_sock_exit();	/* XXX refcounted. oneshot? */
	    break;
	}
    }
DAVDEBUG(-1, (stderr, "<-- %s(%p)\n", __FUNCTION__, u));
    return 0;
}

void davDestroy(void)
{
#if defined(NE_FEATURE_SSL)
    if (ne_has_support(NE_FEATURE_SSL)) {
#if defined(WITH_OPENSSL)	/* XXX FIXME: hard AutoFu to get right. */
/* XXX http://www.nabble.com/Memory-Leaks-in-SSL_Library_init()-t3431875.html */
	ENGINE_cleanup();
	CRYPTO_cleanup_all_ex_data();
	ERR_free_strings();
	ERR_remove_state(0);
	EVP_cleanup();
	CRYPTO_mem_leaks(NULL);
	CONF_modules_unload(1);
#endif	/* WITH_OPENSSL */
    }
#endif	/* NE_FEATURE_SSL */
DAVDEBUG(-1, (stderr, "<-- %s()\n", __FUNCTION__));
}

static void davProgress(void * userdata, off_t progress, off_t total)
	/*@*/
{
    urlinfo u = (urlinfo) userdata;
    ne_session * sess;

assert(u != NULL);
    sess = (ne_session *) u->sess;
assert(sess != NULL);
/*@-sefuncon@*/
assert(u == ne_get_session_private(sess, "urlinfo"));
/*@=sefuncon@*/

    u->info.progress = progress;
    u->info.total = total;

DAVDEBUG(-1, (stderr, "<-- %s(%p,0x%x:0x%x) sess %p u %p\n", __FUNCTION__, userdata, (unsigned int)progress, (unsigned int)total, sess, u));
}

#if WITH_NEON_MIN_VERSION >= 0x002700
static void davNotify(void * userdata,
		ne_session_status status, const ne_session_status_info *info)
#else
static void davNotify(void * userdata,
		ne_conn_status status, const char * info)
#endif
	/*@*/
{
    char buf[64];
    urlinfo u = (urlinfo) userdata;
    ne_session * sess;

assert(u != NULL);
    sess = (ne_session *) u->sess;
assert(sess != NULL);
/*@-sefuncon@*/
assert(u == ne_get_session_private(sess, "urlinfo"));
/*@=sefuncon@*/

    u->info.hostname = NULL;
    u->info.address = NULL;
    u->info.progress = 0;
    u->info.total = 0;

#if WITH_NEON_MIN_VERSION >= 0x002700
#ifdef	REFERENCE
typedef enum {
    ne_status_lookup = 0, /* looking up hostname */
    ne_status_connecting, /* connecting to host */
    ne_status_connected, /* connected to host */
    ne_status_sending, /* sending a request body */
    ne_status_recving, /* receiving a response body */
    ne_status_disconnected /* disconnected from host */
} ne_session_status;
#endif
    switch (status) {
    default:
	break;
    case ne_status_lookup:	/* looking up hostname */
	u->info.hostname = info->ci.hostname;
	break;
    case ne_status_connecting:	/* connecting to host */
	u->info.hostname = info->ci.hostname;
	(void) ne_iaddr_print(info->ci.address, buf, sizeof(buf));
	buf[sizeof(buf)-1] = '\0';
	u->info.address = buf;
    	break;
    case ne_status_connected:	/* connected to host */
	u->info.hostname = info->ci.hostname;
	break;
    case ne_status_sending:	/* sending a request body */
	u->info.progress = info->sr.progress;
	u->info.total = info->sr.total;
	break;
    case ne_status_recving:	/* receiving a response body */
	u->info.progress = info->sr.progress;
	u->info.total = info->sr.total;
	break;
    case ne_status_disconnected:
	u->info.hostname = info->ci.hostname;
	break;
    }

    if (u->notify != NULL)
	(void) (*u->notify) (u, status);

#else
#ifdef	REFERENCE
typedef enum {
    ne_conn_namelookup, /* lookup up hostname (info = hostname) */
    ne_conn_connecting, /* connecting to host (info = hostname) */
    ne_conn_connected, /* connected to host (info = hostname) */
    ne_conn_secure /* connection now secure (info = crypto level) */
} ne_conn_status;
#endif

    {
/*@observer@*/
	static const char * connstates[] = {
	    "namelookup",
	    "connecting",
	    "connected",
	    "secure",
	    "unknown"
	};

DAVDEBUG(-1, (stderr, "--> %s(%p,%d,%p) sess %p u %p %s\n", __FUNCTION__, userdata, status, info, sess, u, connstates[ (status < 4 ? status : 4)]));
    }
#endif

    u->info.status = status;
    u->info.hostname = NULL;
    u->info.address = NULL;
    u->info.progress = 0;
    u->info.total = 0;
}

static void davCreateRequest(ne_request * req, void * userdata,
		const char * method, const char * uri)
	/*@*/
{
    urlinfo u = (urlinfo) userdata;
    ne_session * sess;
    void * myprivate = NULL;
    const char * id = "urlinfo";

assert(u != NULL);
assert(u->sess != NULL);
assert(req != NULL);
    sess = ne_get_session(req);
assert(sess == u->sess);
/*@-sefuncon@*/
assert(u == ne_get_session_private(sess, "urlinfo"));
/*@=sefuncon@*/

assert(sess != NULL);
    myprivate = ne_get_session_private(sess, id);
assert(u == myprivate);

DAVDEBUG(-1, (stderr, "<-- %s(%p,%p,%s,%s) %s:%p\n", __FUNCTION__, req, userdata, method, uri, id, myprivate));
}

static void davPreSend(ne_request * req, void * userdata, ne_buffer * buf)
{
    urlinfo u = (urlinfo) userdata;
    ne_session * sess;
    const char * id = "fd";
    FD_t fd = NULL;

/*@-modunconnomods@*/
assert(u != NULL);
assert(u->sess != NULL);
assert(req != NULL);
    sess = ne_get_session(req);
assert(sess == u->sess);
/*@-sefuncon@*/
assert(u == ne_get_session_private(sess, "urlinfo"));
/*@=sefuncon@*/

    fd = (FD_t) ne_get_request_private(req, id);
/*@=modunconnomods@*/

DAVDEBUG(1, (stderr, "-> %s\n", buf->data));
DAVDEBUG(-1, (stderr, "<-- %s(%p,%p,%p) sess %p %s %p\n", __FUNCTION__, req, userdata, buf, sess, id, fd));

}

static int davPostSend(ne_request * req, void * userdata, const ne_status * status)
	/*@*/
{
    urlinfo u = (urlinfo) userdata;
    ne_session * sess;
    const char * id = "fd";
    FD_t fd = NULL;

assert(u != NULL);
assert(u->sess != NULL);
assert(req != NULL);
    sess = ne_get_session(req);
assert(sess == u->sess);
/*@-sefuncon@*/
assert(u == ne_get_session_private(sess, "urlinfo"));
/*@=sefuncon@*/

    fd = (FD_t) ne_get_request_private(req, id);

/*@-evalorder@*/
DAVDEBUG(-1, (stderr, "<-- %s(%p,%p,%p) sess %p %s %p %s\n", __FUNCTION__, req, userdata, status, sess, id, fd, ne_get_error(sess)));
/*@=evalorder@*/
    return NE_OK;
}

static void davDestroyRequest(ne_request * req, void * userdata)
	/*@*/
{
    urlinfo u = (urlinfo) userdata;
    ne_session * sess;
    const char * id = "fd";
    FD_t fd = NULL;

assert(u != NULL);
assert(u->sess != NULL);
assert(req != NULL);
    sess = ne_get_session(req);
assert(sess == u->sess);
/*@-sefuncon@*/
assert(u == ne_get_session_private(sess, "urlinfo"));
/*@=sefuncon@*/

    fd = (FD_t) ne_get_request_private(req, id);

DAVDEBUG(-1, (stderr, "<-- %s(%p,%p) sess %p %s %p\n", __FUNCTION__, req, userdata, sess, id, fd));
}

static void davDestroySession(void * userdata)
	/*@*/
{
    urlinfo u = (urlinfo) userdata;
    ne_session * sess;
    void * myprivate = NULL;
    const char * id = "urlinfo";

assert(u != NULL);
assert(u->sess != NULL);
    sess = (ne_session *) u->sess;
/*@-sefuncon@*/
assert(u == ne_get_session_private(sess, "urlinfo"));
/*@=sefuncon@*/

assert(sess != NULL);
    myprivate = ne_get_session_private(sess, id);
assert(u == myprivate);

DAVDEBUG(-1, (stderr, "<-- %s(%p) sess %p %s %p\n", __FUNCTION__, userdata, sess, id, myprivate));
}

static int
davVerifyCert(void *userdata, int failures, const ne_ssl_certificate *cert)
	/*@*/
{
    const char *hostname = (const char *) userdata;

DAVDEBUG(-1, (stderr, "--> %s(%p,%d,%p) %s\n", __FUNCTION__, userdata, failures, cert, hostname));

    return 0;	/* HACK: trust all server certificates. */
}

static int davConnect(urlinfo u)
	/*@globals errno, internalState @*/
	/*@modifies u, errno, internalState @*/
{
    const char * path = NULL;
    int rc;

    /* HACK: hkp:// has no steenkin' options */
    switch (urlType(u)) {
    case URL_IS_HKP:
    default:
	return 0;
	/*@notreached@*/ break;
    case URL_IS_HTTP:
    case URL_IS_HTTPS:
	break;
    }

    /* HACK: where should server capabilities be read? */
    (void) urlPath(u->url, &path);
    if (path == NULL || *path == '\0')
	path = "/";

#ifdef NOTYET	/* XXX too many new directories while recursing. */
    /* Repeat OPTIONS for new directories. */
    if (path != NULL && path[strlen(path)-1] == '/')
	u->allow &= ~RPMURL_SERVER_OPTIONSDONE;
#endif
    /* Have options been run? */
    if (u->allow & RPMURL_SERVER_OPTIONSDONE)
	return 0;

    u->allow &= ~(RPMURL_SERVER_HASDAVCLASS1 |
		  RPMURL_SERVER_HASDAVCLASS2 |
		  RPMURL_SERVER_HASDAVEXEC);

    /* HACK: perhaps capture Allow: tag, look for PUT permitted. */
    /* XXX [hdr] Allow: GET,HEAD,POST,OPTIONS,TRACE */
    rc = ne_options((ne_session *)u->sess, path, (ne_server_capabilities *)u->capabilities);
    switch (rc) {
    case NE_OK:
	u->allow |= RPMURL_SERVER_OPTIONSDONE;
    {	ne_server_capabilities *cap = (ne_server_capabilities *)u->capabilities;
	if (cap->dav_class1)
	    u->allow |= RPMURL_SERVER_HASDAVCLASS1;
	else
	    u->allow &= ~RPMURL_SERVER_HASDAVCLASS1;
	if (cap->dav_class2)
	    u->allow |= RPMURL_SERVER_HASDAVCLASS2;
	else
	    u->allow &= ~RPMURL_SERVER_HASDAVCLASS2;
	if (cap->dav_executable)
	    u->allow |= RPMURL_SERVER_HASDAVEXEC;
	else
	    u->allow &= ~RPMURL_SERVER_HASDAVEXEC;
    }	break;
    case NE_ERROR:
	/* HACK: "501 Not Implemented" if OPTIONS not permitted. */
	if (!strncmp("501 ", ne_get_error((ne_session *)u->sess), sizeof("501 ")-1)) {
	    u->allow |= RPMURL_SERVER_OPTIONSDONE;
	    rc = NE_OK;
	    break;
	}
	/* HACK: "301 Moved Permanently" on empty subdir. */
	if (!strncmp("301 ", ne_get_error((ne_session *)u->sess), sizeof("301 ")-1))
	    break;
#ifdef	HACK	/* XXX need davHEAD changes here? */
	/* HACK: "302 Found" if URI is missing pesky trailing '/'. */
	if (!strncmp("302 ", ne_get_error((ne_session *)u->sess), sizeof("302 ")-1)) {
	    char * t;
	    if ((t = strchr(u->url, '\0')) != NULL)
		*t = '/';
	    break;
	}
#endif
	errno = EIO;		/* HACK: more precise errno. */
	goto bottom;
    case NE_LOOKUP:
	errno = ENOENT;		/* HACK: errno same as non-existent path. */
	goto bottom;
    case NE_CONNECT:		/* HACK: errno set already? */
    default:
bottom:
/*@-evalorderuncon@*/
DAVDEBUG(-1, (stderr, "*** Connect to %s:%d failed(%d):\n\t%s\n", u->host, u->port, rc, ne_get_error((ne_session *)u->sess)));
/*@=evalorderuncon@*/
	break;
    }

    /* HACK: sensitive to error returns? */
    u->httpVersion = (ne_version_pre_http11((ne_session *)u->sess) ? 0 : 1);

    return rc;
}

static int davInit(const char * url, urlinfo * uret)
	/*@globals internalState @*/
	/*@modifies *uret, internalState @*/
{
    urlinfo u = NULL;
    int rc = 0;

/*@-globs@*/	/* FIX: h_errno annoyance. */
    if (urlSplit(url, &u))
	return -1;	/* XXX error returns needed. */
/*@=globs@*/

    if (u->url != NULL && u->sess == NULL)
    switch (u->ut) {
    default:
	assert(u->ut != u->ut);
	/*@notreached@*/ break;
    case URL_IS_HTTPS:
    case URL_IS_HTTP:
    case URL_IS_HKP:
      {	ne_server_capabilities * capabilities;

	/* HACK: oneshots should be done Somewhere Else Instead. */
/*@-noeffect@*/
	rc = ((_dav_debug < 0) ? NE_DBG_HTTP : 0);
	ne_debug_init(stderr, rc);		/* XXX oneshot? */
/*@=noeffect@*/
	rc = ne_sock_init();	/* XXX refcounted. oneshot? */

	u->lockstore = ne_lockstore_create();	/* XXX oneshot? */

	u->capabilities = capabilities = (ne_server_capabilities *) xcalloc(1, sizeof(*capabilities));
	u->sess = ne_session_create(u->scheme, u->host, u->port);

	ne_lockstore_register((ne_lock_store *)u->lockstore, (ne_session *)u->sess);

	if (u->proxyh != NULL)
	    ne_session_proxy((ne_session *)u->sess, u->proxyh, u->proxyp);

#if 0
	{   const ne_inet_addr ** addrs;
	    unsigned int n;
	    ne_set_addrlist((ne_session *)u->sess, addrs, n);
	}
#endif

	ne_set_progress((ne_session *)u->sess, davProgress, u);
#if WITH_NEON_MIN_VERSION >= 0x002700
	ne_set_notifier((ne_session *)u->sess, davNotify, u);
#else
	ne_set_status((ne_session *)u->sess, davNotify, u);
#endif

#if WITH_NEON_MIN_VERSION >= 0x002600
	ne_set_session_flag((ne_session *)u->sess, NE_SESSFLAG_PERSIST, rpmioHttpPersist);
	ne_set_connect_timeout((ne_session *)u->sess, rpmioHttpConnectTimeoutSecs);
#else
	ne_set_persist((ne_session *)u->sess, rpmioHttpPersist);
#endif
	ne_set_read_timeout((ne_session *)u->sess, rpmioHttpReadTimeoutSecs);
	ne_set_useragent((ne_session *)u->sess,
	    (rpmioHttpUserAgent ? rpmioHttpUserAgent : _rpmioHttpUserAgent));

	/* XXX check that neon is ssl enabled. */
	if (!strcasecmp(u->scheme, "https"))
	    ne_ssl_set_verify((ne_session *)u->sess, davVerifyCert, (char *)u->host);

	ne_set_session_private((ne_session *)u->sess, "urlinfo", u);

	ne_hook_destroy_session((ne_session *)u->sess, davDestroySession, u);

	ne_hook_create_request((ne_session *)u->sess, davCreateRequest, u);
	ne_hook_pre_send((ne_session *)u->sess, davPreSend, u);
	ne_hook_post_send((ne_session *)u->sess, davPostSend, u);
	ne_hook_destroy_request((ne_session *)u->sess, davDestroyRequest, u);

	/* HACK: where should server capabilities be read? */
	rc = davConnect(u);
	if (rc)
	    goto exit;
      }	break;
    }

exit:
DAVDEBUG(-1, (stderr, "<-- %s(%s) u->url %s\n", __FUNCTION__, url, u->url));
    if (uret != NULL)
	*uret = urlLink(u, "davInit");
    u = urlFree(u, "urlSplit (davInit)");

    return rc;
}

/* =============================================================== */
enum fetch_rtype_e {
    resr_normal = 0,
    resr_collection,
    resr_reference,
    resr_error
};

struct fetch_resource_s {
/*@dependent@*/
    struct fetch_resource_s *next;
    char *uri;
/*@unused@*/
    char *displayname;
    enum fetch_rtype_e type;
    size_t size;
    time_t modtime;
    int is_executable;
    int is_vcr;    /* Is version resource. 0: no vcr, 1 checkin 2 checkout */
    char *error_reason; /* error string returned for this resource */
    int error_status; /* error status returned for this resource */
};

#ifdef __cplusplus
GENfree(struct fetch_resource_s *)
#endif	/* __cplusplus */

/*@null@*/
static void *fetch_destroy_item(/*@only@*/ struct fetch_resource_s *res)
	/*@modifies res @*/
{
    ne_free(res->uri);
    ne_free(res->error_reason);
    res = _free(res);
    return NULL;
}

#ifdef	NOTUSED
/*@null@*/
static void *fetch_destroy_list(/*@only@*/ struct fetch_resource_s *res)
	/*@modifies res @*/
{
    struct fetch_resource_s *next;
    for (; res != NULL; res = next) {
	next = res->next;
	res = fetch_destroy_item(res);
    }
    return NULL;
}
#endif

#if WITH_NEON_MIN_VERSION >= 0x002600
static void *fetch_create_item(/*@unused@*/ void *userdata, /*@unused@*/ const ne_uri *uri)
#else
static void *fetch_create_item(/*@unused@*/ void *userdata, /*@unused@*/ const char *uri)
#endif
        /*@*/
{
    struct fetch_resource_s * res = (struct fetch_resource_s *) ne_calloc(sizeof(*res));
    return res;
}

/* =============================================================== */

/*@-nullassign -readonlytrans@*/
/*@unchecked@*/ /*@observer@*/
static const ne_propname fetch_props[] = {
    { "DAV:", "getcontentlength" },
    { "DAV:", "getlastmodified" },
    { "http://apache.org/dav/props/", "executable" },
    { "DAV:", "resourcetype" },
    { "DAV:", "checked-in" },
    { "DAV:", "checked-out" },
    { NULL, NULL }
};
/*@=nullassign =readonlytrans@*/

#define ELM_resourcetype (NE_PROPS_STATE_TOP + 1)
#define ELM_collection (NE_PROPS_STATE_TOP + 2)

/*@-readonlytrans@*/
/*@unchecked@*/ /*@observer@*/
static const struct ne_xml_idmap fetch_idmap[] = {
    { "DAV:", "resourcetype", ELM_resourcetype },
    { "DAV:", "collection", ELM_collection }
};
/*@=readonlytrans@*/

static int fetch_startelm(void *userdata, int parent,
		const char *nspace, const char *name,
		/*@unused@*/ const char **atts)
	/*@*/
{
    ne_propfind_handler *pfh = (ne_propfind_handler *) userdata;
    struct fetch_resource_s *r = (struct fetch_resource_s *)
	ne_propfind_current_private(pfh);
/*@-sizeoftype@*/
    int state = ne_xml_mapid(fetch_idmap, NE_XML_MAPLEN(fetch_idmap),
                             nspace, name);
/*@=sizeoftype@*/

    if (r == NULL ||
        !((parent == NE_207_STATE_PROP && state == ELM_resourcetype) ||
          (parent == ELM_resourcetype && state == ELM_collection)))
        return NE_XML_DECLINE;

    if (state == ELM_collection) {
	r->type = resr_collection;
    }

    return state;
}

static int fetch_compare(const struct fetch_resource_s *r1,
			    const struct fetch_resource_s *r2)
	/*@*/
{
    /* Sort errors first, then collections, then alphabetically */
    if (r1->type == resr_error) {
	return -1;
    } else if (r2->type == resr_error) {
	return 1;
    } else if (r1->type == resr_collection) {
	if (r2->type != resr_collection) {
	    return -1;
	} else {
	    return strcmp(r1->uri, r2->uri);
	}
    } else {
	if (r2->type != resr_collection) {
	    return strcmp(r1->uri, r2->uri);
	} else {
	    return 1;
	}
    }
}

#if WITH_NEON_MIN_VERSION >= 0x002600
static void fetch_results(void *userdata, const ne_uri *uarg,
		    const ne_prop_result_set *set)
#else
static void fetch_results(void *userdata, void *uarg,
		    const ne_prop_result_set *set)
#endif
	/*@*/
{
    rpmavx avx = (rpmavx) userdata;
    struct fetch_resource_s *current, *previous, *newres;
    const char *clength, *modtime, *isexec;
    const char *checkin, *checkout;
    const ne_status *status = NULL;
    const char * path = NULL;

#if WITH_NEON_MIN_VERSION >= 0x002600
    const ne_uri * uri = uarg;
    (void) urlPath(uri->path, &path);
#else
    const char * uri = uarg;
    (void) urlPath(uri, &path);
#endif
    if (path == NULL)
	return;

    newres = (struct fetch_resource_s *) ne_propset_private(set);

DAVDEBUG(-1, (stderr, "==> %s in uri %s\n", path, avx->uri));

    if (ne_path_compare(avx->uri, path) == 0) {
	/* This is the target URI */
DAVDEBUG(-1, (stderr, "==> %s skipping target resource.\n", path));
	/* Free the private structure. */
/*@-dependenttrans -exposetrans@*/
	free(newres);
/*@=dependenttrans =exposetrans@*/
	return;
    }

    newres->uri = ne_strdup(path);

    clength = ne_propset_value(set, &fetch_props[0]);
    modtime = ne_propset_value(set, &fetch_props[1]);
    isexec = ne_propset_value(set, &fetch_props[2]);
    checkin = ne_propset_value(set, &fetch_props[4]);
    checkout = ne_propset_value(set, &fetch_props[5]);

    if (clength == NULL)
	status = ne_propset_status(set, &fetch_props[0]);
    if (modtime == NULL)
	status = ne_propset_status(set, &fetch_props[1]);

    if (newres->type == resr_normal && status != NULL) {
	/* It's an error! */
	newres->error_status = status->code;

	/* Special hack for Apache 1.3/mod_dav */
	if (strcmp(status->reason_phrase, "status text goes here") == 0) {
	    const char *desc;
	    if (status->code == 401) {
		desc = _("Authorization Required");
	    } else if (status->klass == 3) {
		desc = _("Redirect");
	    } else if (status->klass == 5) {
		desc = _("Server Error");
	    } else {
		desc = _("Unknown Error");
	    }
	    newres->error_reason = ne_strdup(desc);
	} else {
	    newres->error_reason = ne_strdup(status->reason_phrase);
	}
	newres->type = resr_error;
    }

    if (isexec && strcasecmp(isexec, "T") == 0) {
	newres->is_executable = 1;
    } else {
	newres->is_executable = 0;
    }

    if (modtime)
	newres->modtime = ne_httpdate_parse(modtime);

    if (clength)
	newres->size = atoi(clength);

    /* is vcr */
    if (checkin) {
	newres->is_vcr = 1;
    } else if (checkout) {
	newres->is_vcr = 2;
    } else {
	newres->is_vcr = 0;
    }

    current = *(struct fetch_resource_s **)avx->resrock;
    for (current = (struct fetch_resource_s *) *avx->resrock, previous = NULL;
	current != NULL;
	previous = current, current = current->next)
    {
	if (fetch_compare(current, newres) >= 0) {
	    break;
	}
    }
    if (previous) {
	previous->next = newres;
    } else {
/*@-dependenttrans @*/
        *(struct fetch_resource_s **)avx->resrock = newres;
/*@=dependenttrans @*/
    }
    newres->next = current;
}

static int davFetch(const urlinfo u, rpmavx avx)
	/*@globals internalState @*/
	/*@modifies avx, internalState @*/
{
    const char * path = NULL;
    int depth = 1;					/* XXX passed arg? */
    struct fetch_resource_s * resitem = NULL;
    ne_propfind_handler *pfh;
    struct fetch_resource_s *current, *next;
    struct stat * st = avx->st;
    mode_t st_mode;
    int rc = 0;
    int xx;

    (void) urlPath(u->url, &path);
    pfh = ne_propfind_create((ne_session *)u->sess, avx->uri, depth);

    /* HACK: need to set RPMURL_SERVER_HASRANGE in u->allow here. */

    avx->resrock = (void **) &resitem;

    ne_xml_push_handler(ne_propfind_get_parser(pfh),
                        fetch_startelm, NULL, NULL, pfh);

    ne_propfind_set_private(pfh, fetch_create_item, NULL);

    rc = ne_propfind_named(pfh, fetch_props, fetch_results, avx);

    ne_propfind_destroy(pfh);

    for (current = resitem; current != NULL; current = next) {
	const char *s, *se;
	char * val;

	next = current->next;

	/* Collections have trailing '/' that needs trim. */
	/* The top level collection is returned as well. */
	se = current->uri + strlen(current->uri);
	if (se[-1] == '/') {
	    if (strlen(current->uri) <= strlen(path)) {
		st->st_mode = (S_IFDIR|0755);
		st->st_nlink += 2;
		/* XXX TODO: current-size is 0 here. */
		st->st_size = current->size;
		st->st_blocks = (st->st_size + 511)/512;
		st->st_mtime = current->modtime;
		st->st_atime = st->st_ctime = st->st_mtime;        /* HACK */
		current = (struct fetch_resource_s *)
			fetch_destroy_item(current);
		continue;
	    }
	    se--;
	}
	s = se;
	while (s > current->uri && s[-1] != '/')
	    s--;

	val = ne_strndup(s, (se - s));

/*@-nullpass@*/
	val = ne_path_unescape(val);
/*@=nullpass@*/

	switch (current->type) {
	case resr_normal:
	    st_mode = S_IFREG | 0644;
	    /*@switchbreak@*/ break;
	case resr_collection:
	    st_mode = S_IFDIR | 0755;
	    if (S_ISDIR(st->st_mode))
		st->st_nlink++;
	    /*@switchbreak@*/ break;
	case resr_reference:
	case resr_error:
	default:
	    st_mode = 0;
	    /*@switchbreak@*/ break;
	}

	xx = rpmavxAdd(avx, val, st_mode, current->size, current->modtime);
	ne_free(val);

	if (current == resitem && next == NULL) {
	    st->st_mode = st_mode;
	    st->st_nlink = S_ISDIR(st_mode) ? 2 : 1;
	    st->st_size = current->size;
	    st->st_blocks = (st->st_size + 511)/512;
	    st->st_mtime = current->modtime;
	    st->st_atime = st->st_ctime = st->st_mtime;        /* HACK */
	}

	current = (struct fetch_resource_s *)
		fetch_destroy_item(current);
    }
    avx->resrock = NULL;	/* HACK: avoid leaving stack reference. */
    /* HACK realloc to truncate modes/sizes/mtimes */

    return rc;
}

/* HACK davHEAD() should be rewritten to use davReq/davResp w callbacks. */
static int davHEAD(urlinfo u, struct stat *st) 
	/*@modifies u, *st @*/
{
    ne_request *req;
    const ne_status *status = NULL;
    const char *htag;
    const char *value = NULL;
    int rc;
int printing = 0;

    /* XXX HACK: URI's with pesky trailing '/' are directories. */
    {	size_t nb = strlen(u->url);
	st->st_mode = (u->url[nb-1] == '/' ? S_IFDIR : S_IFREG);
    }
    st->st_blksize = 4 * 1024;	/* HACK correct for linux ext */
    st->st_atime = -1;
    st->st_mtime = -1;
    st->st_ctime = -1;

    req = ne_request_create((ne_session *)u->sess, "HEAD", u->url);
    if (rpmioHttpAccept != NULL)
	ne_add_request_header(req, "Accept", rpmioHttpAccept);

    /* XXX if !defined(HAVE_NEON_NE_GET_RESPONSE_HEADER) handlers? */

    rc = ne_request_dispatch(req);
    status = ne_get_status(req);

/* XXX somewhere else instead? */
DAVDEBUG(1, (stderr, "HTTP request sent, awaiting response... %d %s\n", status->code, status->reason_phrase));

    switch (rc) {
    default:
	goto exit;
	/*@notreached@*/ break;
    case NE_OK:
	if (status->klass != 2)		/* XXX is this necessary? */
	    rc = NE_ERROR;
	break;
    }

#if defined(HAVE_NEON_NE_GET_RESPONSE_HEADER)
    htag = "ETag";
    value = ne_get_response_header(req, htag); 
    if (value) {
	/* inode-size-mtime */
	u->etag = _free(u->etag);
	u->etag = xstrdup(value);
    }

    /* XXX limit to 3xx returns? */
    htag = "Location";
    value = ne_get_response_header(req, htag); 
    if (value) {
	u->location = _free(u->location);
	u->location = xstrdup(value);
    }

/* XXX Content-Length: is returned only for files. */
    htag = "Content-Length";
    value = ne_get_response_header(req, htag); 
    if (value) {
/* XXX should wget's "... (1.2K)..." be added? */
if (_dav_debug && ++printing)
fprintf(stderr, "Length: %s", value);

/*@-unrecog@*/	/* XXX LCLINT needs stdlib.h update. */
	st->st_size = strtoll(value, NULL, 10);
/*@=unrecog@*/
	st->st_blocks = (st->st_size + 511)/512;
    } else {
	st->st_size = 0;
	st->st_blocks = 0;
    }

    htag = "Content-Type";
    value = ne_get_response_header(req, htag); 
    if (value) {
if (_dav_debug && printing)
fprintf(stderr, " [%s]", value);
	if (!strcmp(value, "text/html")
	 || !strcmp(value, "application/xhtml+xml"))
	    st->st_blksize = 2 * 1024;
    }

    htag = "Last-Modified";
    value = ne_get_response_header(req, htag); 
    if (value) {
if (_dav_debug && printing)
fprintf(stderr, " [%s]", value);
	st->st_mtime = ne_httpdate_parse(value);
	st->st_atime = st->st_ctime = st->st_mtime;	/* HACK */
    }

if (_dav_debug && printing)
fprintf(stderr, "\n");
#endif

exit:
    ne_request_destroy(req);
    return rc;
}

static int my_result(const char * msg, int ret, /*@null@*/ FILE * fp)
	/*@modifies *fp @*/
{
    /* HACK: don't print unless debugging. */
    if (_dav_debug >= 0)
	return ret;
    if (fp == NULL)
	fp = stderr;
    if (msg != NULL)
	fprintf(fp, "*** %s: ", msg);

    /* HACK FTPERR_NE_FOO == -NE_FOO error impedance match */
#ifdef	HACK
    fprintf(fp, "%s: %s\n", ftpStrerror(-ret), ne_get_error(sess));
#else
    fprintf(fp, "%s\n", ftpStrerror(-ret));
#endif
    return ret;
}

/* XXX TODO move to rpmhtml.c */
/**
 */
typedef struct rpmhtml_s * rpmhtml;
#endif	/* WITH_NEON */

int _html_debug = 0;

/*@unchecked@*/ /*@only@*/ /*@null@*/
rpmioPool _htmlPool = NULL;

#ifdef WITH_NEON
/**
 */
struct rpmhtml_s {
    struct rpmioItem_s _item;	/*!< usage mutex and pool identifier. */
/*@kept@*/
    rpmavx avx;
    ne_request *req;

/*@observer@*/
    const char * pattern;
/*@relnull@*/
    miRE mires;
    int nmires;

    char * buf;
    size_t nbuf;
/*@null@*/
    char * b;
    size_t nb;
#if defined(__LCLINT__)
/*@refs@*/
    int nrefs;			/*!< (unused) keep splint happy */
#endif
};

/**
 * Unreference a html wrapper instance.
 * @param html		html wrapper
 * @return		NULL on last derefernce
 */
/*@unused@*/ /*@null@*/
rpmhtml htmlUnlink (/*@null@*/ rpmhtml html)
        /*@modifies html @*/;
#define htmlUnlink(_html)  \
    ((rpmhtml)rpmioUnlinkPoolItem((rpmioItem)(_html), __FUNCTION__, __FILE__, __LINE__))

/**
 * Reference a html wrapper instance.
 * @param html		html wrapper
 * @return		new html wrapper reference
 */
/*@unused@*/ /*@newref@*/ /*@null@*/
rpmhtml htmlLink (/*@null@*/ rpmhtml html)
        /*@modifies html @*/;
#define htmlLink(_html)  \
    ((rpmhtml)rpmioLinkPoolItem((rpmioItem)(_html), __FUNCTION__, __FILE__, __LINE__))

/**
 * Destroy a html wrapper instance.
 * @param html		html wrapper
 * @return		NULL on last derefernce
 */
/*@null@*/
rpmhtml htmlFree (/*@null@*/ rpmhtml html)
        /*@modifies html @*/;
#define htmlFree(_html)  \
    ((rpmhtml)rpmioFreePoolItem((rpmioItem)(_html), __FUNCTION__, __FILE__, __LINE__))

/**
 */
static void htmlFini(void * _html)
	/*@globals fileSystem @*/
	/*@modifies *_html, fileSystem @*/
{
    rpmhtml html = (rpmhtml) _html;

    html->avx = NULL;
    if (html->req != NULL) {
	ne_request_destroy(html->req);
	html->req = NULL;
    }
    html->pattern = NULL;
    html->mires = NULL;
    html->nmires = 0;
    html->b = html->buf = _free(html->buf);
    html->nb = html->nbuf = 0;
}

static rpmhtml htmlGetPool(/*@null@*/ rpmioPool pool)
	/*@globals _htmlPool, fileSystem @*/
	/*@modifies pool, _htmlPool, fileSystem @*/
{
    rpmhtml html;

    if (_htmlPool == NULL) {
	_htmlPool = rpmioNewPool("html", sizeof(*html), -1, _html_debug,
			NULL, NULL, htmlFini);
	pool = _htmlPool;
    }
    html = (rpmhtml) rpmioGetPool(pool, sizeof(*html));
    memset(((char *)html)+sizeof(html->_item), 0, sizeof(*html)-sizeof(html->_item));
    return html;
}

/**
 */
static
rpmhtml htmlNew(urlinfo u, /*@kept@*/ rpmavx avx) 
	/*@*/
{
    rpmhtml html = htmlGetPool(_htmlPool);
    html->avx = avx;
    html->req = ne_request_create((ne_session *)u->sess, "GET", u->url);
    html->pattern = NULL;
    html->mires = NULL;
    html->nmires = 0;
    html->nbuf = BUFSIZ;	/* XXX larger buffer? */
    html->buf = (char *) xmalloc(html->nbuf + 1 + 1);
    html->b = NULL;
    html->nb = 0;
    return htmlLink(html);
}

/**
 */
static ssize_t htmlFill(rpmhtml html)
	/*@modifies html @*/
{
    char * b = html->buf;
    size_t nb = html->nbuf;
    ssize_t rc;

    if (html->b != NULL && html->nb > 0 && html->b > html->buf) {
	memmove(html->buf, html->b, html->nb);
	b += html->nb;
	nb -= html->nb;
    }
DAVDEBUG(-1, (stderr, "--> %s(%p) %p[%u]\n", __FUNCTION__, html, b, (unsigned)nb));

    /* XXX FIXME: "server awol" segfaults here. gud enuf atm ... */
    rc = ne_read_response_block(html->req, b, nb) ;
    if (rc > 0) {
	html->nb += rc;
	b += rc;
	nb -= rc;
    }
    html->b = html->buf;

DAVDEBUG(-1, (stderr, "<-- %s(%p) %p[%u] rc %d\n", __FUNCTION__, html, b, (unsigned)nb, (int)rc));
    return rc;
}

/**
 * Convert hex to binary nibble.
 * @param c            hex character
 * @return             binary nibble
 */
static
unsigned char nibble(char c)
	/*@*/
{
    if (c >= '0' && c <= '9')
	return (unsigned char) (c - '0');
    if (c >= 'A' && c <= 'F')
	return (unsigned char)((int)(c - 'A') + 10);
    if (c >= 'a' && c <= 'f')
	return (unsigned char)((int)(c - 'a') + 10);
    return (unsigned char) '\0';
}

/*@observer@*/
static const char * hrefpat = "(?i)<a(?:\\s+[a-z][a-z0-9_]*(?:=(?:\"[^\"]*\"|\\S+))?)*?\\s+href=(?:\"([^\"]*)\"|(\\S+))";

/**
 */
static int htmlParse(rpmhtml html)
	/*@globals hrefpat, internalState @*/
	/*@modifies html, internalState @*/
{
    struct stat * st = html->avx->st;
    miRE mire;
    int noffsets = 3;
    int offsets[3];
    ssize_t nr = (html->b != NULL ? (ssize_t)html->nb : htmlFill(html));
    size_t contentLength = (nr >= 0 ? nr : 0);
    int rc = 0;
    int xx;

DAVDEBUG(-1, (stderr, "--> %s(%p) %p[%u]\n", __FUNCTION__, html, html->buf, (unsigned)html->nbuf));

    if (st) {
	st->st_mode |= 0755;	/* htmlParse() is always a directory. */
	st->st_nlink = 2;	/* count . and .. links */
    }

    html->pattern = hrefpat;
    xx = mireAppend(RPMMIRE_PCRE, 0, html->pattern, NULL, &html->mires, &html->nmires);
    mire = html->mires;

    xx = mireSetEOptions(mire, offsets, noffsets);

    while (html->nb > 0) {
	char * gbn, * href;
	const char * hbn, * lpath;
	char * be;
	char * f, * fe;
	char * g, * ge;
	size_t ng;
	char * h, * he;
	size_t nh;
	char * t;
	mode_t st_mode = S_IFREG | 0644;
	int ut;

assert(html->b != NULL);
	be = html->b + html->nb;
	*be = '\0';
	offsets[0] = offsets[1] = -1;
	xx = mireRegexec(mire, html->b, html->nb);
	if (xx == 0 && offsets[0] != -1 && offsets[1] != -1) {

	    /* [f:fe) contains |<a href="..."| match. */
	    f = html->b + offsets[0];
	    fe = html->b + offsets[1];

	    he = fe;
	    if (he[-1] == '"') he--;
	    h = he;
	    while (h > f && h[-1] != '"')
		h--;
	    /* [h:he) contains the href. */
assert(he > h);
	    nh = (size_t)(he - h);
	    href = t = (char *) xmalloc(nh + 1 + 1);	/* XXX +1 for trailing '/' */
	    *t = '\0';
	    while (h < he) {
		char c = *h++;
		switch (c) {
		default:
		    /*@switchbreak@*/ break;
		case '%':
		    if (isxdigit((int)h[0]) && isxdigit((int)h[1])) {
			c = (char) (nibble(h[0]) << 4) | nibble(h[1]);
			h += 2;
		    }
		    /*@switchbreak@*/ break;
		}
		*t++ = c;
	    }
	    *t = '\0';

	    /* Determine type of href. */
	    switch ((ut = urlPath(href, &lpath))) {
	    case URL_IS_UNKNOWN:
	    default:
		/* XXX verify "same tree" as root URI. */
		if (href[nh-1] == '/') {
		    st_mode = S_IFDIR | 0755;
		    href[nh-1] = '\0';
		} else
		    st_mode = S_IFREG | 0644;
		/*@switchbreak@*/ break;
	    case URL_IS_FTP:
	    case URL_IS_HTTPS:
	    case URL_IS_HTTP:
#ifdef	NOTYET	/* XXX rpmavx needs to save linktos first. */
		st_mode = S_IFLNK | 0755;
		/*@switchbreak@*/ break;
#endif
	    case URL_IS_PATH:
	    case URL_IS_DASH:
	    case URL_IS_HKP:
		href[0] = '\0';
		/*@switchbreak@*/ break;
	    }
	    if ((hbn = strrchr(href, '/')) != NULL)
		hbn++;
	    else
		hbn = href;
assert(hbn != NULL);

	    /* Parse the URI path. */
	    g = fe;
	    while (g < be && *g && *g != '>')
		g++;
	    if (g >= be || *g != '>') {
		href = _free(href);
		goto refill;
	    }
	    ge = ++g;
	    while (ge < be && *ge && *ge != '<')
		ge++;
	    if (ge >= be || *ge != '<') {	
		href = _free(href);
		goto refill;
	    }
	    /* [g:ge) contains the URI basename. */
	    ng = (size_t)(ge - g);
	    gbn = t = (char *) xmalloc(ng + 1 + 1);
	    while (g < ge && *g != '/')		/* XXX prohibit '/' in gbn. */
		*t++ = *g++;
	    *t = '\0';

if (_dav_debug)
if (*hbn != '\0' && *gbn != '\0' && strcasecmp(hbn, gbn))
fprintf(stderr, "\t[%s] != [%s]\n", hbn, gbn);

	    /*
	     * Heuristics to identify HTML sub-directories:
	     *   Avoid empty strings.
	     *   Both "." and ".." will be added by rpmavx.
	     *
	     * Assume (case insensitive) basename(href) == basename(URI) is
	     * a subdirectory.
	     */
	    if (*hbn != '\0' && *gbn != '\0')
	    if (strcmp(hbn, ".") && strcmp(hbn, ".."))
	    if (!strcasecmp(hbn, gbn)) {
		size_t _st_size = (size_t)0;	/* XXX HACK */
		time_t _st_mtime = (time_t)0;	/* XXX HACK */
		xx = rpmavxAdd(html->avx, gbn, st_mode, _st_size, _st_mtime);
		/* count subdir links */
		if (st && S_ISDIR(st_mode)) st->st_nlink++;
	    }

	    gbn = _free(gbn);
	    href = _free(href);

	    offsets[1] += (ge - fe);
	    html->b += offsets[1];
	    html->nb -= offsets[1];
	} else {
	    size_t nb = html->nb;
	    if (nr > 0) nb -= 256;	/* XXX overlap a bit if filling. */
	    html->b += nb;
	    html->nb -= nb;
	}

	/* XXX Refill iff lowater reaches nbuf/4 (~2kB) */
	if (nr <= 0 || html->nb >= (html->nbuf/4))
	    continue;
refill:
	if ((nr = htmlFill(html)) >= 0)
	    contentLength += nr;
    }

    /* XXX Set directory length to no. of bytes of HTML parsed. */
    if (st) {
	if (st->st_size == 0) {
	    st->st_size = contentLength;
	    st->st_blocks = (st->st_size + 511)/512;
	}
    }

    xx = mireSetEOptions(mire, NULL, 0);

    html->mires = (miRE) mireFreeAll(html->mires, html->nmires);
    html->nmires = 0;

DAVDEBUG(-1, (stderr, "<-- %s(%p) rc %d\n", __FUNCTION__, html, rc));
    return rc;
}

/* HACK htmlNLST() should be rewritten to use davReq/davResp w callbacks. */
/*@-mustmod@*/
static int htmlNLST(urlinfo u, rpmavx avx) 
	/*@globals hrefpat, internalState @*/
	/*@modifies avx, internalState @*/
{
    rpmhtml html = htmlNew(u, avx);
    int rc = 0;

    do {
	rc = ne_begin_request(html->req);
	rc = my_result("ne_begin_req(html->req)", rc, NULL);
	switch (rc) {
	case NE_OK:
	    /*@switchbreak@*/ break;
	case NE_TIMEOUT:
	    errno = ETIMEDOUT;
	    /*@fallthrough@*/
	default:
	    goto exit;
	    /*@notreached@*/ /*@switchbreak@*/ break;
	}

	(void) htmlParse(html);		/* XXX error code needs handling. */

	rc = ne_end_request(html->req);
	rc = my_result("ne_end_req(html->req)", rc, NULL);
    } while (rc == NE_RETRY);

exit:
    html = htmlFree(html);
    return rc;
}
/*@=mustmod@*/

static int davNLST(rpmavx avx)
	/*@globals hrefpat, internalState @*/
	/*@modifies avx, internalState @*/
{
    urlinfo u = NULL;
const char * u_url = NULL;	/* XXX FIXME: urlFind should save current URI */
    int rc;
    int xx;

retry:
    rc = davInit(avx->uri, &u);
    if (rc || u == NULL)
	goto exit;

if (u_url == NULL) {		/* XXX FIXME: urlFind should save current URI */
u_url = u->url;
u->url = avx->uri;
}
    /*
     * Do PROPFIND through davFetch iff server supports.
     * Otherwise, do HEAD to get Content-length/ETag/Last-Modified,
     * followed by GET through htmlNLST() to find the contained href's.
     */
    if (u->allow & RPMURL_SERVER_HASDAV)
	rc = davFetch(u, avx);	/* use PROPFIND to get contentLength */
    else {
/*@-nullpass@*/	/* XXX annotate avx->st correctly */
	rc = davHEAD(u, avx->st);	/* use HEAD to get contentLength */
/*@=nullpass@*/
	/* Parse directory elements. */
	if (rc == NE_OK && S_ISDIR(avx->st->st_mode))
	    rc = htmlNLST(u, avx);
    }

    switch (rc) {
    case NE_OK:
        break;
    case NE_ERROR:
	/* HACK: "405 Method Not Allowed" for PROPFIND on non-DAV servers. */
	/* XXX #206066 OPTIONS is ok, but PROPFIND from Stat() fails. */
	/* rpm -qp --rpmiodebug --davdebug http://people.freedesktop.org/~sandmann/metacity-2.16.0-2.fc6/i386/metacity-2.16.0-2.fc6.i386.rpm */

	/* HACK: "301 Moved Permanently" on empty subdir. */
	if (!strncmp("301 ", ne_get_error((ne_session *)u->sess), sizeof("301 ")-1))
	    break;

	/* HACK: "302 Found" if URI is missing pesky trailing '/'. */
	if (!strncmp("302 ", ne_get_error((ne_session *)u->sess), sizeof("302 ")-1)) {
	    const char * path = NULL;
	    int ut = urlPath(u->url, &path);
	    size_t nb = strlen(path);
	    ut = ut;	/* XXX keep gcc happy */
	    if (u->location != NULL && !strncmp(path, u->location, nb)
	     && u->location[nb] == '/' && u->location[nb+1] == '\0')
	    {
		char * te = (char *) strchr(u->url, '\0');
		/* Append the pesky trailing '/'. */
		if (te != NULL && te[-1] != '/') {
		    /* XXX u->uri malloc'd w room for +1b */
		    *te++ = '/';
		    *te = '\0';
		    u->location = _free(u->location);
		    /* XXX retry here needed iff ContentLength:. */
if (u_url != NULL) {		/* XXX FIXME: urlFind should save current URI */
u->url = u_url;
u_url = NULL;
}
		    xx = davFree(u);
		    goto retry;
		    /*@notreached@*/ break;
		}
	    }
	}
	/*@fallthrough@*/
    default:
/*@-evalorderuncon@*/
DAVDEBUG(1, (stderr, "*** Fetch from %s:%d failed:\n\t%s\n",
		   u->host, u->port, ne_get_error((ne_session *)u->sess)));
/*@=evalorderuncon@*/
        break;
    }

exit:
if (u_url != NULL) {		/* XXX FIXME: urlFind should save current URI */
u->url = u_url;
u_url = NULL;
}
    /* XXX Destroy the session iff not OK, otherwise persist. */
    if (rc)
	xx = davFree(u);
    return rc;
}

/* =============================================================== */
/*@-mustmod@*/
static void davAcceptRanges(void * userdata, /*@null@*/ const char * value)
	/*@modifies userdata @*/
{
    urlinfo u = (urlinfo) userdata;

    if (!(u != NULL && value != NULL)) return;
DAVDEBUG(-1, (stderr, "*** u %p Accept-Ranges: %s\n", u, value));
    if (!strcmp(value, "bytes"))
	u->allow |= RPMURL_SERVER_HASRANGE;
    if (!strcmp(value, "none"))
	u->allow &= ~RPMURL_SERVER_HASRANGE;
}
/*@=mustmod@*/

#if !defined(HAVE_NEON_NE_GET_RESPONSE_HEADER)
static void davAllHeaders(void * userdata, const char * value)
{
    FD_t ctrl = (FD_t) userdata;

    if (!(ctrl != NULL && value != NULL)) return;
DAVDEBUG(1, (stderr, "<- %s\n", value));
}
#endif

/*@-mustmod@*/
static void davContentLength(void * userdata, /*@null@*/ const char * value)
	/*@modifies userdata @*/
{
    FD_t ctrl = (FD_t) userdata;

    if (!(ctrl != NULL && value != NULL)) return;
DAVDEBUG(-1, (stderr, "*** fd %p Content-Length: %s\n", ctrl, value));
/*@-unrecog@*/
   ctrl->contentLength = strtoll(value, NULL, 10);
/*@=unrecog@*/
}
/*@=mustmod@*/

/*@-mustmod@*/
static void davContentType(void * userdata, /*@null@*/ const char * value)
	/*@modifies userdata @*/
{
    FD_t ctrl = (FD_t) userdata;

    if (!(ctrl != NULL && value != NULL)) return;
DAVDEBUG(-1, (stderr, "*** fd %p Content-Type: %s\n", ctrl, value));
   ctrl->contentType = _free(ctrl->contentType);
   ctrl->contentType = xstrdup(value);
}
/*@=mustmod@*/

/*@-mustmod@*/
static void davContentDisposition(void * userdata, /*@null@*/ const char * value)
	/*@modifies userdata @*/
{
    FD_t ctrl = (FD_t) userdata;

    if (!(ctrl != NULL && value != NULL)) return;
DAVDEBUG(-1, (stderr, "*** fd %p Content-Disposition: %s\n", ctrl, value));
   ctrl->contentDisposition = _free(ctrl->contentDisposition);
   ctrl->contentDisposition = xstrdup(value);
}
/*@=mustmod@*/

/*@-mustmod@*/
static void davLastModified(void * userdata, /*@null@*/ const char * value)
	/*@modifies userdata @*/
{
    FD_t ctrl = (FD_t) userdata;

    if (!(ctrl != NULL && value != NULL)) return;
DAVDEBUG(-1, (stderr, "*** fd %p Last-Modified: %s\n", ctrl, value));
/*@-unrecog@*/
   ctrl->lastModified = ne_httpdate_parse(value);
/*@=unrecog@*/
}
/*@=mustmod@*/

/*@-mustmod@*/
static void davConnection(void * userdata, /*@null@*/ const char * value)
	/*@modifies userdata @*/
{
    FD_t ctrl = (FD_t) userdata;

    if (!(ctrl != NULL && value != NULL)) return;
DAVDEBUG(-1, (stderr, "*** fd %p Connection: %s\n", ctrl, value));
    if (!strcasecmp(value, "close"))
	ctrl->persist = 0;
    else if (!strcasecmp(value, "Keep-Alive"))
	ctrl->persist = 1;
}
/*@=mustmod@*/

/*@-mustmod@*/ /* HACK: stash error in *str. */
int davResp(urlinfo u, FD_t ctrl, /*@unused@*/ char *const * str)
{
    int rc = 0;

DAVDEBUG(-1, (stderr, "--> %s(%p,%p,%p) sess %p req %p\n", __FUNCTION__, u, ctrl, str, u->sess, ctrl->req));

    rc = ne_begin_request((ne_request *)ctrl->req);
    rc = my_result("ne_begin_req(ctrl->req)", rc, NULL);

    /* HACK FTPERR_NE_FOO == -NE_FOO error impedance match */
/*@-observertrans@*/
    if (rc)
	fdSetSyserrno(ctrl, errno, ftpStrerror(-rc));
/*@=observertrans@*/

DAVDEBUG(-1, (stderr, "<-- %s(%p,%p,%p) sess %p req %p rc %d\n", __FUNCTION__, u, ctrl, str, u->sess, ctrl->req, rc));

    return rc;
}
/*@=mustmod@*/

int davReq(FD_t ctrl, const char * httpCmd, const char * httpArg)
{
    urlinfo u;
    int rc = 0;

assert(ctrl != NULL);
    u = (urlinfo) ctrl->u;
    URLSANE(u);

DAVDEBUG(-1, (stderr, "--> %s(%p,%s,\"%s\") entry sess %p req %p\n", __FUNCTION__, ctrl, httpCmd, (httpArg ? httpArg : ""), u->sess, ctrl->req));

    ctrl->persist = (u->httpVersion > 0 ? 1 : 0);
    ctrl = fdLink(ctrl, "open ctrl (davReq)");
assert(ctrl != NULL);

assert(u->sess != NULL);
    /* XXX reset disconnected handle to NULL. should never happen ... */
    if (ctrl->req == (void *)-1)
	ctrl->req = NULL;
/*@-nullderef@*/
assert(ctrl->req == NULL);
/*@=nullderef@*/
/*@-nullpass@*/
    ctrl->req = ne_request_create((ne_session *)u->sess, httpCmd, httpArg);
/*@=nullpass@*/
assert(ctrl->req != NULL);

    ne_set_request_private((ne_request *)ctrl->req, "fd", ctrl);

#if !defined(HAVE_NEON_NE_GET_RESPONSE_HEADER)
    ne_add_response_header_catcher((ne_request *)ctrl->req, davAllHeaders, ctrl);

    ne_add_response_header_handler((ne_request *)ctrl->req, "Content-Length",
		davContentLength, ctrl);
    ne_add_response_header_handler((ne_request *)ctrl->req, "Content-Type",
		davContentType, ctrl);
    ne_add_response_header_handler((ne_request *)ctrl->req, "Content-Disposition",
		davContentDisposition, ctrl);
    ne_add_response_header_handler((ne_request *)ctrl->req, "Last-Modified",
		davLastModified, ctrl);
    ne_add_response_header_handler((ne_request *)ctrl->req, "Connection",
		davConnection, ctrl);
#endif

    if (!strcmp(httpCmd, "PUT")) {
#if defined(HAVE_NEON_NE_SEND_REQUEST_CHUNK)
	ctrl->wr_chunked = 1;
	ne_add_request_header((ne_request *)ctrl->req, "Transfer-Encoding", "chunked");
	ne_set_request_chunked((ne_request *)ctrl->req, 1);
	/* HACK: no retries if/when chunking. */
	rc = davResp(u, ctrl, NULL);
#else
	rc = FTPERR_SERVER_IO_ERROR;
#endif
    } else {
	/* HACK: possible ETag: "inode-size-mtime" */
#if !defined(HAVE_NEON_NE_GET_RESPONSE_HEADER)
	ne_add_response_header_handler((ne_request *)ctrl->req, "Accept-Ranges",
			davAcceptRanges, u);
#endif
	/* HACK: possible Transfer-Encoding: on GET. */

	/* HACK: other errors may need retry too. */
	/* HACK: neon retries once, gud enuf. */
	/* HACK: retry counter? */
	do {
	    rc = davResp(u, ctrl, NULL);
	} while (rc == NE_RETRY);
    }

/* XXX somewhere else instead? */
if (_dav_debug) {
    const ne_status *status = ne_get_status((ne_request *)ctrl->req);
fprintf(stderr, "HTTP request sent, awaiting response... %d %s\n", status->code, status->reason_phrase);
}

    if (rc)
	goto errxit;

DAVDEBUG(-1, (stderr, "<-- %s(%p,%s,\"%s\") exit sess %p req %p rc %d\n", __FUNCTION__, ctrl, httpCmd, (httpArg ? httpArg : ""), u->sess, ctrl->req, rc));

#if defined(HAVE_NEON_NE_GET_RESPONSE_HEADER)
    davContentLength(ctrl,
		ne_get_response_header((ne_request *)ctrl->req, "Content-Length"));
    davContentType(ctrl,
		ne_get_response_header((ne_request *)ctrl->req, "Content-Type"));
    davContentDisposition(ctrl,
		ne_get_response_header((ne_request *)ctrl->req, "Content-Disposition"));
    davLastModified(ctrl,
		ne_get_response_header((ne_request *)ctrl->req, "Last-Modified"));
    davConnection(ctrl,
		ne_get_response_header((ne_request *)ctrl->req, "Connection"));
    if (strcmp(httpCmd, "PUT"))
	davAcceptRanges(u,
		ne_get_response_header((ne_request *)ctrl->req, "Accept-Ranges"));
#endif

    ctrl = fdLink(ctrl, "open data (davReq)");
    return 0;

errxit:
/*@-observertrans@*/
    fdSetSyserrno(ctrl, errno, ftpStrerror(rc));
/*@=observertrans@*/

    /* HACK balance fd refs. ne_session_destroy to tear down non-keepalive? */
    ctrl = fdLink(ctrl, "error data (davReq)");

    return rc;
}

FD_t davOpen(const char * url, /*@unused@*/ int flags,
		/*@unused@*/ mode_t mode, /*@out@*/ urlinfo * uret)
{
    const char * path = NULL;
    urltype ut = urlPath(url, &path);
    urlinfo u = NULL;
    FD_t fd = NULL;
    int rc;

#if 0	/* XXX makeTempFile() heartburn */
    assert(!(flags & O_RDWR));
#endif

DAVDEBUG(-1, (stderr, "--> %s(%s,0x%x,0%o,%p)\n", __FUNCTION__, url, flags, (unsigned)mode, uret));
    rc = davInit(url, &u);
    if (rc || u == NULL || u->sess == NULL)
	goto exit;

    if (u->ctrl == NULL)
	u->ctrl = fdNew("persist ctrl (davOpen)");
    else {
	yarnLock use = u->ctrl->_item.use;
	yarnPossess(use);
	if (yarnPeekLock(use) > 2L && u->data == NULL)
	    u->data = fdNew("persist data (davOpen)");
	yarnRelease(use);
    }

    if (u->ctrl->u == NULL)
	fd = u->ctrl = fdLink(u->ctrl, "grab ctrl (davOpen persist ctrl)");
    else if (u->data->u == NULL)
	fd = u->data = fdLink(u->data, "grab ctrl (davOpen persist data)");
    else
	fd = fdNew("grab ctrl (davOpen)");

    if (fd) {
	fdSetOpen(fd, url, flags, mode);
	fdSetIo(fd, ufdio);

	fd->ftpFileDoneNeeded = 0;
	fd->rd_timeoutsecs = rpmioHttpReadTimeoutSecs;
	fd->contentLength = fd->bytesRemain = -1;
assert(ut == URL_IS_HTTPS || ut == URL_IS_HTTP || ut == URL_IS_HKP);
	fd->u = urlLink(u, "url (davOpen)");
	fd = fdLink(fd, "grab data (davOpen)");
    }

exit:
    if (uret)
	*uret = u;
    /*@-refcounttrans@*/
    return fd;
    /*@=refcounttrans@*/
}

/*@-mustmod@*/
ssize_t davRead(void * cookie, /*@out@*/ char * buf, size_t count)
{
    FD_t fd = (FD_t) cookie;
    ssize_t rc;

#if WITH_NEON_MIN_VERSION >= 0x002700
  { urlinfo u = NULL;
    u = urlLink(fd->u, "url (davRead)");
    if (u->info.status == ne_status_recving)
	rc = ne_read_response_block((ne_request *)fd->req, buf, count);
    else {
	/* If server has disconnected, then tear down the neon request. */
	if (u->info.status == ne_status_disconnected) {
	    int xx;
	    xx = ne_end_request((ne_request *)fd->req);
	    xx = my_result("davRead: ne_end_request(req)", xx, NULL);
	    ne_request_destroy((ne_request *)fd->req);
	    fd->req = (void *)-1;
	}
	errno = EIO;       /* XXX what to do? */
	rc = -1;
    }
    u = urlFree(u, "url (davRead)");
  }
#else
    rc = ne_read_response_block((ne_request *)fd->req, buf, count);
#endif

DAVDEBUG(-1, (stderr, "<-- %s(%p,%p,0x%x) rc 0x%x\n", __FUNCTION__, cookie, buf, (unsigned)count, (unsigned)rc));

    return rc;
}
/*@=mustmod@*/

ssize_t davWrite(void * cookie, const char * buf, size_t count)
{
#if !defined(NEONBLOWSCHUNKS) || defined(HAVE_NEON_NE_SEND_REQUEST_CHUNK) || defined(__LCLINT__)
    FD_t fd = (FD_t) cookie;
#endif
    ssize_t rc;
    int xx = -1;

#if !defined(NEONBLOWSCHUNKS)
    ne_session * sess;

assert(fd->req != NULL);
    sess = ne_get_session((ne_request *)fd->req);
assert(sess != NULL);

    /* HACK: include ne_private.h to access sess->socket for now. */
    xx = ne_sock_fullwrite(sess->socket, buf, count);
#else
#if defined(HAVE_NEON_NE_SEND_REQUEST_CHUNK) || defined(__LCLINT__)
assert(fd->req != NULL);
/*@-unrecog@*/
    xx = ne_send_request_chunk((ne_request *)fd->req, buf, count);
/*@=unrecog@*/
#else
    errno = EIO;       /* HACK */
    return -1;
#endif
#endif

    /* HACK: stupid error impedance matching. */
    rc = (xx == 0 ? (ssize_t)count : -1);

DAVDEBUG(-1, (stderr, "<-- %s(%p,%p,0x%x) rc 0x%x\n", __FUNCTION__, cookie, buf, (unsigned)count, (unsigned)rc));

    return rc;
}

int davSeek(void * cookie, /*@unused@*/ _libio_pos_t pos, int whence)
{
    int rc = -1;
DAVDEBUG(-1, (stderr, "<-- %s(%p,pos,%d) rc %d\n", __FUNCTION__, cookie, whence, rc));
    return rc;
}

/*@-mustmod@*/	/* HACK: fd->req is modified. */
int davClose(void * cookie)
{
/*@-onlytrans@*/
    FD_t fd = (FD_t) cookie;
/*@=onlytrans@*/
    int rc = 0;

DAVDEBUG(-1, (stderr, "--> %s(%p) rc %d clen %d req %p u %p\n", __FUNCTION__, fd, rc, (int)fd->bytesRemain, fd->req, fd->u));

assert(fd->req != NULL);
    if (fd->req != (void *)-1) {
	rc = ne_end_request((ne_request *)fd->req);
	rc = my_result("ne_end_request(req)", rc, NULL);

	ne_request_destroy((ne_request *)fd->req);
    }
    fd->req = NULL;

DAVDEBUG(-1, (stderr, "<-- %s(%p) rc %d\n", __FUNCTION__, fd, rc));
    return rc;
}
/*@=mustmod@*/

/* =============================================================== */
int davMkdir(const char * path, mode_t mode)
{
    urlinfo u = NULL;
    const char * src = NULL;
    int rc;

    rc = davInit(path, &u);
    if (rc)
	goto exit;
assert(u != NULL);

    (void) urlPath(path, &src);

    rc = ne_mkcol((ne_session *)u->sess, path);

    if (rc) rc = -1;	/* XXX HACK: errno impedance match */

    /* XXX HACK: verify getrestype(remote) == resr_collection */

exit:
DAVDEBUG(1, (stderr, "<-- %s(%s,0%o) rc %d\n", __FUNCTION__, path, (unsigned)mode, rc));
    return rc;
}

int davRmdir(const char * path)
{
    urlinfo u = NULL;
    const char * src = NULL;
    int rc;

    rc = davInit(path, &u);
    if (rc)
	goto exit;
assert(u != NULL);

    (void) urlPath(path, &src);

    /* XXX HACK: only getrestype(remote) == resr_collection */

    rc = ne_delete((ne_session *)u->sess, path);

    if (rc) rc = -1;	/* XXX HACK: errno impedance match */

exit:
DAVDEBUG(1, (stderr, "<-- %s(%s) rc %d\n", __FUNCTION__, path, rc));
    return rc;
}

int davRename(const char * oldpath, const char * newpath)
{
    urlinfo u = NULL;
    const char * src = NULL;
    const char * dst = NULL;
    int overwrite = 1;		/* HACK: set this correctly. */
    int rc;

    rc = davInit(oldpath, &u);
    if (rc)
	goto exit;
assert(u != NULL);

    (void) urlPath(oldpath, &src);
    (void) urlPath(newpath, &dst);

    /* XXX HACK: only getrestype(remote) != resr_collection */

    rc = ne_move((ne_session *)u->sess, overwrite, src, dst);

    if (rc) rc = -1;	/* XXX HACK: errno impedance match */

exit:
DAVDEBUG(1, (stderr, "<-- %s(%s,%s) rc %d\n", __FUNCTION__, oldpath, newpath, rc));
    return rc;
}

int davUnlink(const char * path)
{
    urlinfo u = NULL;
    const char * src = NULL;
    int rc;

    rc = davInit(path, &u);
    if (rc)
	goto exit;
assert(u != NULL);

    (void) urlPath(path, &src);

    /* XXX HACK: only getrestype(remote) != resr_collection */

    rc = ne_delete((ne_session *)u->sess, src);

exit:
    if (rc) rc = -1;	/* XXX HACK: errno impedance match */

DAVDEBUG(1, (stderr, "<-- %s(%s) rc %d\n", __FUNCTION__, path, rc));
    return rc;
}

#ifdef	NOTYET
static int davChdir(const char * path)
	/*@globals h_errno, fileSystem, internalState @*/
	/*@modifies fileSystem, internalState @*/
{
    return davCommand("CWD", path, NULL);
}
#endif	/* NOTYET */

/* =============================================================== */

static const char * statstr(const struct stat * st,
		/*@returned@*/ /*@out@*/ char * buf)
	/*@modifies *buf @*/
{
    sprintf(buf,
	"dev 0x%x ino 0x%x mode 0%0o nlink %d uid %d gid %d rdev 0x%x size %u",
	(unsigned)st->st_dev,
	(unsigned)st->st_ino,
	(unsigned)st->st_mode,
	(unsigned)st->st_nlink,
	(unsigned)st->st_uid,
	(unsigned)st->st_gid,
	(unsigned)st->st_rdev,
	(unsigned)st->st_size);
    return buf;
}

int davStat(const char * path, /*@out@*/ struct stat *st)
	/*@globals hrefpat, fileSystem, internalState @*/
	/*@modifies *st, fileSystem, internalState @*/
{
    rpmavx avx = NULL;
    char buf[1024];
    int rc = -1;

DAVDEBUG(-1, (stderr, "--> %s(%s)\n", __FUNCTION__, path));
    if (path == NULL || *path == '\0') {
	errno = ENOENT;
	goto exit;
    }
    avx = (rpmavx) rpmavxNew(path, st);
    if (avx == NULL) {
	errno = ENOENT;		/* Note: avx is NULL iff urlSplit() fails. */
	goto exit;
    }
    rc = davNLST(avx);
    if (rc) {
	if (errno == 0)	errno = EAGAIN;	/* HACK: errno = ??? */
	rc = -1;
	goto exit;
    }

    /* XXX fts(3) needs/uses st_ino. */
    /* Hash the path to generate a st_ino analogue. */
    st->st_ino = hashFunctionString(0, path, 0);

exit:
DAVDEBUG(-1, (stderr, "<-- %s(%s) rc %d\n\t%s\n", __FUNCTION__, path, rc, statstr(st, buf)));
    avx = rpmavxFree(avx);
    return rc;
}

int davLstat(const char * path, /*@out@*/ struct stat *st)
	/*@globals hrefpat, fileSystem, internalState @*/
	/*@modifies *st, fileSystem, internalState @*/
{
    rpmavx avx = NULL;
    char buf[1024];
    int rc = -1;

    if (path == NULL || *path == '\0') {
	errno = ENOENT;
	goto exit;
    }
    avx = (rpmavx) rpmavxNew(path, st);
    if (avx == NULL) {
	errno = ENOENT;		/* Note: avx is NULL iff urlSplit() fails. */
	goto exit;
    }
    rc = davNLST(avx);
    if (rc) {
	if (errno == 0)	errno = EAGAIN;	/* HACK: errno = ??? */
	rc = -1;
	goto exit;
    }

    /* XXX fts(3) needs/uses st_ino. */
    /* Hash the path to generate a st_ino analogue. */
    st->st_ino = hashFunctionString(0, path, 0);

DAVDEBUG(-1, (stderr, "<-- %s(%s) rc %d\n\t%s\n", __FUNCTION__, path, rc, statstr(st, buf)));
exit:
    avx = rpmavxFree(avx);
    return rc;
}

#ifdef	NOTYET
static int davReadlink(const char * path, /*@out@*/ char * buf, size_t bufsiz)
	/*@globals h_errno, fileSystem, internalState @*/
	/*@modifies *buf, fileSystem, internalState @*/
{
    int rc;
    rc = davNLST(path, DO_FTP_READLINK, NULL, buf, bufsiz);
DAVDEBUG(-1, (stderr, "<-- %s(%s) rc %d\n", __FUNCTION__, path, rc));
    return rc;
}
#endif	/* NOTYET */

#endif /* WITH_NEON */

/* =============================================================== */
/*@unchecked@*/
int avmagicdir = 0x3607113;

#ifndef WITH_NEON
/*@-nullstate@*/        /* FIX: u->{ctrl,data}->url undef after XurlLink. */
FD_t httpOpen(const char * url, /*@unused@*/ int flags,
                /*@unused@*/ mode_t mode, /*@out@*/ urlinfo * uret)
        /*@globals internalState @*/
        /*@modifies *uret, internalState @*/
{
    urlinfo u = NULL;
    FD_t fd = NULL;

#if 0   /* XXX makeTempFile() heartburn */
    assert(!(flags & O_RDWR));
#endif
    if (urlSplit(url, &u))
        goto exit;

    if (u->ctrl == NULL)
        u->ctrl = fdNew("persist ctrl (httpOpen)");
    if (u->ctrl != NULL) {	/* XXX can't happen */
	yarnLock use = u->ctrl->_item.use;
	yarnPossess(use);
	if (yarnPeekLock(use) > 2L && u->data == NULL)
	    u->data = fdNew("persist data (httpOpen)");
	yarnRelease(use);
    }

    if (u->ctrl->u == NULL)
        fd = fdLink(u->ctrl, "grab ctrl (httpOpen persist ctrl)");
    else if (u->data->u == NULL)
        fd = fdLink(u->data, "grab ctrl (httpOpen persist data)");
    else
        fd = fdNew("grab ctrl (httpOpen)");

    if (fd) {
        fdSetIo(fd, ufdio);
        fd->ftpFileDoneNeeded = 0;
	fd->rd_timeoutsecs = rpmioHttpReadTimeoutSecs;
        fd->contentLength = fd->bytesRemain = -1;
        fd->u = urlLink(u, "url (httpOpen)");
        fd = fdLink(fd, "grab data (httpOpen)");
    }

exit:
    if (uret)
        *uret = u;
    /*@-refcounttrans@*/
    return fd;
    /*@=refcounttrans@*/
}
/*@=nullstate@*/
#endif

#ifdef WITH_NEON
/* =============================================================== */
int davClosedir(/*@only@*/ DIR * dir)
{
    return avClosedir(dir);
}

struct dirent * davReaddir(DIR * dir)
{
    return avReaddir(dir);
}

DIR * davOpendir(const char * path)
	/*@globals hrefpat @*/
{
    AVDIR avdir = NULL;
    rpmavx avx = NULL;
    struct stat sb, *st = &sb; /* XXX HACK: davHEAD needs avx->st. */
    const char * uri = NULL;
    int rc;

DAVDEBUG(-1, (stderr, "--> %s(%s)\n", __FUNCTION__, path));

    if (path == NULL || *path == '\0') {
	errno = ENOENT;
	goto exit;
    }

    /* Note: all Opendir(3) URI's need pesky trailing '/' */
/*@-globs -mods@*/
    if (path[strlen(path)-1] != '/')
	uri = rpmExpand(path, "/", NULL);
    else
	uri = xstrdup(path);
/*@=globs =mods@*/

    /* Load DAV collection into argv. */
    /* XXX HACK: davHEAD needs avx->st. */
    avx = (rpmavx) rpmavxNew(uri, st);
    if (avx == NULL) {
	errno = ENOENT;		/* Note: avx is NULL iff urlSplit() fails. */
	goto exit;
    }

    rc = davNLST(avx);
    if (rc) {
	if (errno == 0)	errno = EAGAIN;	/* HACK: errno = ??? */
	goto exit;
    } else
	avdir = (AVDIR) avOpendir(uri, avx->av, avx->modes);

exit:
    uri = _free(uri);
    avx = rpmavxFree(avx);
/*@-kepttrans@*/
    return (DIR *) avdir;
/*@=kepttrans@*/
}
/*@=modfilesys@*/

/*@-mustmod@*/
char * davRealpath(const char * path, char * resolved_path)
{
assert(resolved_path == NULL);	/* XXX no POSIXly broken realpath(3) here. */
    /* XXX TODO: handle redirects. For now, just dupe the path. */
    return xstrdup(path);
}
/*@=mustmod@*/

#endif /* WITH_NEON */
