/** \ingroup js_c
 * \file js/rpmio-js.c
 */

#include "system.h"

#include "rpmio-js.h"
#include "rpmst-js.h"
#include "rpmjs-debug.h"

#include <rpmio_internal.h>

#include "debug.h"

/*@unchecked@*/
static int _debug = 0;

/* Required JSClass vectors */
#define	rpmio_addprop		JS_PropertyStub
#define	rpmio_delprop		JS_PropertyStub
#define	rpmio_convert		JS_ConvertStub

/* Optional JSClass vectors */
#define	rpmio_getobjectops	NULL
#define	rpmio_checkaccess	NULL
#define	rpmio_call		rpmio_call
#define	rpmio_construct	rpmio_ctor
#define	rpmio_xdrobject	NULL
#define	rpmio_hasinstance	NULL
#define	rpmio_mark		NULL
#define	rpmio_reserveslots	NULL

/* Extended JSClass vectors */
#define rpmio_equality		NULL
#define rpmio_outerobject	NULL
#define rpmio_innerobject	NULL
#define rpmio_iteratorobject	NULL
#define rpmio_wrappedobject	NULL

/* --- helpers */
static FD_t
rpmio_init(JSContext *cx, JSObject *obj, const char * _fn, const char * _fmode)
{
    FD_t fd = NULL;

    if (_fn) {
	fd = Fopen(_fn, _fmode);
	/* XXX error msg */
    }
    if (!JS_SetPrivate(cx, obj, (void *)fd)) {
	/* XXX error msg */
	if (fd) {
	    (void) Fclose(fd);
	    /* XXX error msg */
	}
	fd = NULL;
    }

if (_debug)
fprintf(stderr, "<== %s(%p,%p) Fopen(%s,%s) fd %p\n", __FUNCTION__, cx, obj, _fn, _fmode, fd);

    return fd;
}

/* --- Object methods */
static JSBool
rpmio_digestinit(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmioClass, NULL);
    FD_t fd = ptr;
    jsuint _dalgo = PGPHASHALGO_MD5;
    jsuint _flags = 0;
    JSBool ok;

_METHOD_DEBUG_ENTRY(_debug);

    if ((ok = JS_ConvertArguments(cx, argc, argv, "/uu", &_dalgo, &_flags))) {
	if (fd)
	    fdInitDigest(fd, _dalgo, _flags);
	*vp = OBJECT_TO_JSVAL(obj);
    }
    return ok;
}

static JSBool
rpmio_digestfini(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmioClass, NULL);
    FD_t fd = ptr;
    jsuint _dalgo = PGPHASHALGO_MD5;
    JSBool ok;

_METHOD_DEBUG_ENTRY(_debug);

    if ((ok = JS_ConvertArguments(cx, argc, argv, "/u", &_dalgo))) {
	if (fd) {
	    const char * s = NULL;
	    size_t ns = 0;
	    fdFiniDigest(fd, _dalgo, &s, &ns, 1);
	    *vp = STRING_TO_JSVAL(JS_NewStringCopyZ(cx, s));
	} else
	    *vp = JSVAL_VOID;
    }
    return ok;
}

static JSBool
rpmio_fchown(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmioClass, NULL);
    FD_t fd = ptr;
    jsint _uid = -1;
    jsint _gid = -1;
    JSBool ok;

_METHOD_DEBUG_ENTRY(_debug);

    if ((ok = JS_ConvertArguments(cx, argc, argv, "/ii", &_uid, &_gid))) {
        uid_t uid = _uid;
        uid_t gid = _gid;
        *vp = (fd && !Fchown(fd, uid, gid)
                ? JSVAL_ZERO : INT_TO_JSVAL(errno));
    }
    return ok;
}

static JSBool
rpmio_fclose(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmioClass, NULL);
    FD_t fd = ptr;
    JSBool ok = JS_FALSE;

_METHOD_DEBUG_ENTRY(_debug);

    /* XXX FIXME: _fmode is not persistent across io.fclose() */
    if (fd) {
	(void) Fclose(fd);
	/* XXX error msg */
	fd = ptr = NULL;
	(void) JS_SetPrivate(cx, obj, (void *)fd);
    }
    *vp = OBJECT_TO_JSVAL(obj);

    ok = JS_TRUE;
    return ok;
}

static JSBool
rpmio_fdopen(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmioClass, NULL);
    FD_t fd = ptr;
    const char * _fmode = "r.ufdio";
    JSBool ok = JS_FALSE;

_METHOD_DEBUG_ENTRY(_debug);

    if (!(ok = JS_ConvertArguments(cx, argc, argv, "s", &_fmode)))
        goto exit;

    /* XXX needs checking */
    if (fd && (fd = Fdopen(fd, _fmode)) != NULL) {
	JSObject *o;
	if ((o = JS_NewObject(cx, &rpmioClass, NULL, NULL)) != NULL
	 && JS_SetPrivate(cx, o, (void *)fd))
	    *vp = OBJECT_TO_JSVAL(obj);
	else
	    *vp = JSVAL_VOID;
    } else
	*vp = JSVAL_VOID;

    ok = JS_TRUE;
exit:
    return ok;
}

static JSBool
rpmio_ferror(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmioClass, NULL);
    FD_t fd = ptr;

_METHOD_DEBUG_ENTRY(_debug);
    *vp = (fd && !Ferror(fd) ? JSVAL_TRUE : JSVAL_FALSE);
    return JS_TRUE;
}

static JSBool
rpmio_fflush(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmioClass, NULL);
    FD_t fd = ptr;

_METHOD_DEBUG_ENTRY(_debug);
    *vp = (fd && !Fflush(fd) ? JSVAL_TRUE : JSVAL_FALSE);
    return JS_TRUE;
}

#ifdef	NOTYET
static JSBool
rpmio_fgetpos(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmioClass, NULL);
    FD_t fd = ptr;
    JSBool ok = JSVAL_TRUE;
    int rc = -3;

_METHOD_DEBUG_ENTRY(_debug);

_rpmio_debug = -1;
    if (fd) {
	fpos_t pos = 0;
	if (!Fgetpos(fd, &pos))
	    rc = pos;
    }
_rpmio_debug = 0;
    *vp = INT_TO_JSVAL(rc);
    return ok;
}
#endif

static JSBool
rpmio_fileno(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmioClass, NULL);
    FD_t fd = ptr;
    int fdno;

_METHOD_DEBUG_ENTRY(_debug);
    *vp = (fd && (fdno = Fileno(fd)) >= 0
		? INT_TO_JSVAL(fdno) : INT_TO_JSVAL(-1));
    return JS_TRUE;
}

static JSBool
rpmio_fopen(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmioClass, NULL);
    FD_t fd = ptr;
    const char * _fn = NULL;
    const char * _fmode = "r.ufdio";
    JSBool ok = JS_FALSE;

_METHOD_DEBUG_ENTRY(_debug);

    if (!(ok = JS_ConvertArguments(cx, argc, argv, "s/s", &_fn, &_fmode)))
        goto exit;

    if (fd) {
	(void) Fclose(fd);
	/* XXX error msg */
	fd = ptr = NULL;
	(void) JS_SetPrivate(cx, obj, (void *)fd);
    }

    fd = ptr = rpmio_init(cx, obj, _fn, _fmode);

    *vp = OBJECT_TO_JSVAL(obj);

    ok = JS_TRUE;
exit:
    return ok;
}

static JSBool
rpmio_fread(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmioClass, NULL);
    FD_t fd = ptr;
    JSBool ok = JS_FALSE;
    static jsuint _nbmax = 16 * BUFSIZ;
    jsuint _nb = BUFSIZ;

_METHOD_DEBUG_ENTRY(_debug);

    if (!(ok = JS_ConvertArguments(cx, argc, argv, "/u", &_nb)))
        goto exit;

    if (_nb > _nbmax) _nb = _nbmax;

    if (fd) {
	size_t nb = _nb;
	char * b = alloca(nb);
	size_t nr = Fread(b, 1, nb, fd);
	if (nr == 0)
	    *vp = JSVAL_VOID;		/* XXX goofy? */
	else
	if (Ferror(fd))
	    *vp = JSVAL_FALSE;	/* XXX goofy? */
	else {
	    b[nr] = '\0';
	    *vp = STRING_TO_JSVAL(JS_NewStringCopyZ(cx, b));
	}
    } else
	*vp = JSVAL_VOID;

    ok = JS_TRUE;
exit:
    return ok;
}

static JSBool
rpmio_fseek(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmioClass, NULL);
    FD_t fd = ptr;
    JSBool ok;
    jsint _offset = 0;
    jsint _whence = SEEK_CUR;
    int rc = -3;

_METHOD_DEBUG_ENTRY(_debug);
    if ((ok = JS_ConvertArguments(cx, argc, argv, "/ii", &_offset, _whence)))
	rc = (fd ? Fseek(fd, _offset, _whence) : -3);
    *vp = INT_TO_JSVAL(rc);
    return ok;
}

#ifdef	NOTYET
static JSBool
rpmio_fsetpos(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmioClass, NULL);
    FD_t fd = ptr;
    JSBool ok;
    jsint _offset = 0;
    int rc = -3;

_METHOD_DEBUG_ENTRY(_debug);
    if ((ok = JS_ConvertArguments(cx, argc, argv, "i", &_offset))) {
	fpos_t pos = _offset;
	rc = (fd ? Fsetpos(fd, &pos) : -3);
    }
    *vp = INT_TO_JSVAL(rc);
    return ok;
}
#endif

static JSBool
rpmio_fstat(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmioClass, NULL);
    FD_t fd = ptr;
    JSBool ok = JS_FALSE;
    struct stat sb;

_METHOD_DEBUG_ENTRY(_debug);

    if (fd && !Fstat(fd, &sb)) {
	JSObject *o;
	struct stat *st = NULL;
	size_t nb = sizeof(*st);
	if ((st = memcpy(xmalloc(nb), &sb, nb)) != NULL
	 && (o = JS_NewObject(cx, &rpmstClass, NULL, NULL)) != NULL
	 && JS_SetPrivate(cx, o, (void *)st))
	    *vp = OBJECT_TO_JSVAL(o);
	else {
	    st = _free(st);
	    *vp = JSVAL_VOID;		/* XXX goofy? */
	}
    } else
	*vp = JSVAL_VOID;		/* XXX goofy? */

    ok = JS_TRUE;
    return ok;
}

static JSBool
rpmio_ftell(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmioClass, NULL);
    FD_t fd = ptr;
    JSBool ok = JSVAL_TRUE;
    int rc = -3;

_METHOD_DEBUG_ENTRY(_debug);
    rc = (int)(fd ? Ftell(fd) : -3);
    *vp = INT_TO_JSVAL(rc);
    return ok;
}

static JSBool
rpmio_fwrite(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmioClass, NULL);
    FD_t fd = ptr;
    JSBool ok = JS_FALSE;
    const char * b = NULL;
    size_t nb;

_METHOD_DEBUG_ENTRY(_debug);

    if (!(ok = JS_ConvertArguments(cx, argc, argv, "s", &b)))
        goto exit;
assert(b != NULL);
    nb = strlen(b);

    if (fd) {
	size_t nw = Fwrite(b, 1, nb, fd);
	if (nw == 0)
	    *vp = JSVAL_VOID;		/* XXX goofy? */
	else
	    *vp = (nw != nb || Ferror(fd) ? JSVAL_FALSE : JSVAL_TRUE);
    } else
	*vp = JSVAL_FALSE;

    ok = JS_TRUE;
exit:
    return ok;
}

static JSBool
rpmio_rewind(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmioClass, NULL);
    FD_t fd = ptr;
    JSBool ok = JSVAL_TRUE;

_METHOD_DEBUG_ENTRY(_debug);
    /* XXX should return fd object */
    if (fd) {
	Rewind(fd);
	*vp = JSVAL_TRUE;
    } else
	*vp = JSVAL_FALSE;
    return ok;
}

static JSFunctionSpec rpmio_funcs[] = {
    JS_FS("digestinit",	rpmio_digestinit,	0,0),
    JS_FS("digestfini",	rpmio_digestfini,	0,0),
    JS_FS("fchown",	rpmio_fchown,		0,0),
    JS_FS("fclose",	rpmio_fclose,		0,0),
    JS_FS("fdopen",	rpmio_fdopen,		0,0),
    JS_FS("ferror",	rpmio_ferror,		0,0),
    JS_FS("fflush",	rpmio_fflush,		0,0),
#ifdef	NOTYET
    JS_FS("fgetpos",	rpmio_fgetpos,		0,0),
#endif
    JS_FS("fileno",	rpmio_fileno,		0,0),
    JS_FS("fopen",	rpmio_fopen,		0,0),
    JS_FS("fread",	rpmio_fread,		0,0),
    JS_FS("fseek",	rpmio_fseek,		0,0),
#ifdef	NOTYET
    JS_FS("fsetpos",	rpmio_fsetpos,		0,0),
#endif
    JS_FS("fstat",	rpmio_fstat,		0,0),
    JS_FS("ftell",	rpmio_ftell,		0,0),
    JS_FS("fwrite",	rpmio_fwrite,		0,0),
    JS_FS("rewind",	rpmio_rewind,		0,0),
    JS_FS_END
};

/* --- Object properties */
enum rpmio_tinyid {
    _DEBUG	= -2,
    _PATH	= -3,
    _FLAGS	= -4,
    _MODE	= -5,
    _NDIGESTS	= -6,
    _RDTIMEOUT	= -7,
    _CPIOPOS	= -8,
    _BYTESREMAIN= -9,
    _LASTMODIFIED= -10,
    _CONTENTTYPE= -11,
    _CONTENTLENGTH= -12,
    _CONTENTDISPOSITION= -13,
};

static JSPropertySpec rpmio_props[] = {
    {"debug",	_DEBUG,		JSPROP_ENUMERATE,	NULL,	NULL},
    {"path",	_PATH,		JSPROP_ENUMERATE,	NULL,	NULL},
    {"flags",	_FLAGS,		JSPROP_ENUMERATE,	NULL,	NULL},
    {"mode",	_MODE,		JSPROP_ENUMERATE,	NULL,	NULL},
    {"ndigests",_NDIGESTS,	JSPROP_ENUMERATE,	NULL,	NULL},
    {"rdtimeout",_RDTIMEOUT,	JSPROP_ENUMERATE,	NULL,	NULL},
    {"cpioPos",	_CPIOPOS,	JSPROP_ENUMERATE,	NULL,	NULL},
    {"bytesRemain",_BYTESREMAIN,JSPROP_ENUMERATE,	NULL,	NULL},
    {"lastModified",_LASTMODIFIED,JSPROP_ENUMERATE,	NULL,	NULL},
    {"contentType",_CONTENTTYPE,JSPROP_ENUMERATE,	NULL,	NULL},
    {"contentLength",_CONTENTLENGTH,JSPROP_ENUMERATE,	NULL,	NULL},
    {"contentDisposition",_CONTENTDISPOSITION,JSPROP_ENUMERATE,	NULL,	NULL},
    {NULL, 0, 0, NULL, NULL}
};

#define _GET_I(_p, _f)   ((_p) ? INT_TO_JSVAL((int)(_p)->_f) : JSVAL_VOID)
#define _GET_S(_p, _f) ((_p) && (_p)->_f \
	? STRING_TO_JSVAL(JS_NewStringCopyZ(cx, (_p)->_f)) : JSVAL_VOID)

static JSBool
rpmio_getprop(JSContext *cx, JSObject *obj, jsid id, jsval *vp)
{
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmioClass, NULL);
    FD_t fd = ptr;
    jsint tiny = JSVAL_TO_INT(id);

_PROP_DEBUG_ENTRY(_debug < 0);

    /* XXX the class has ptr == NULL, instances have ptr != NULL. */
    if (ptr == NULL)
	return JS_TRUE;

    switch (tiny) {
    case _DEBUG:	*vp = INT_TO_JSVAL(_debug);		break;
    case _PATH:		*vp = _GET_S(fd, opath);		break;
    case _FLAGS:	*vp = _GET_I(fd, oflags);		break;
    case _MODE:		*vp = _GET_I(fd, omode);		break;
    case _NDIGESTS:	*vp = _GET_I(fd, ndigests);		break;
    case _RDTIMEOUT:	*vp = _GET_I(fd, rd_timeoutsecs);	break;
    case _CPIOPOS:	*vp = _GET_I(fd, fd_cpioPos);		break;
    case _BYTESREMAIN:	*vp = _GET_I(fd, bytesRemain);		break;
    case _LASTMODIFIED:	*vp = _GET_I(fd, lastModified);		break;
    case _CONTENTTYPE:	*vp = _GET_S(fd, contentType);		break;
    case _CONTENTLENGTH:*vp = _GET_I(fd, contentLength);	break;
    case _CONTENTDISPOSITION:	*vp = _GET_S(fd, contentDisposition);	break;
    default:
	break;
    }

    return JS_TRUE;
}

static JSBool
rpmio_setprop(JSContext *cx, JSObject *obj, jsid id, JSBool strict, jsval *vp)
{
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmioClass, NULL);
    jsint tiny = JSVAL_TO_INT(id);

_PROP_DEBUG_ENTRY(_debug < 0);

    /* XXX the class has ptr == NULL, instances have ptr != NULL. */
    if (ptr == NULL)
	return JS_TRUE;

    switch (tiny) {
    case _DEBUG:
	if (!JS_ValueToInt32(cx, *vp, &_debug))
	    break;
	break;
    default:
	break;
    }

    return JS_TRUE;
}

static JSBool
rpmio_resolve(JSContext *cx, JSObject *obj, jsid id, uintN flags,
		JSObject **objp)
{
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmioClass, NULL);

_RESOLVE_DEBUG_ENTRY(_debug < 0);

    if ((flags & JSRESOLVE_ASSIGNING)
     || (ptr == NULL)) { /* don't resolve to parent prototypes objects. */
	*objp = NULL;
	goto exit;
    }

    *objp = obj;	/* XXX always resolve in this object. */

exit:
    return JS_TRUE;
}

static JSBool
rpmio_enumerate(JSContext *cx, JSObject *obj, JSIterateOp op,
		  jsval *statep, jsid *idp)
{
#ifdef	NOTYET
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmioClass, NULL);
    FD_t fd = ptr;
    unsigned int ix = 0;
#endif

    /* XXX VG: JS_Enumerate (jsobj.c:4211) doesn't initialize some fields. */
_ENUMERATE_DEBUG_ENTRY(_debug < 0);

#ifdef	NOTYET
    switch (op) {
    case JSENUMERATE_INIT:
    case JSENUMERATE_INIT_ALL:
	if (idp)
	    *idp = JSVAL_ZERO;
	*statep = INT_TO_JSVAL(ix);
if (_debug)
fprintf(stderr, "\tINIT fd %p\n", fd);
        break;
    case JSENUMERATE_NEXT:
	ix = JSVAL_TO_INT(*statep);
	if ((dp = Readdir(dir)) != NULL) {
	    (void) JS_DefineElement(cx, obj,
			ix, STRING_TO_JSVAL(JS_NewStringCopyZ(cx, dp->d_name)),
			NULL, NULL, JSPROP_ENUMERATE);
	    JS_ValueToId(cx, *statep, idp);
if (_debug)
fprintf(stderr, "\tNEXT fd %p[%u] dirent %p \"%s\"\n", fd, ix, dp, dp->d_name);
	    *statep = INT_TO_JSVAL(ix+1);
	} else
	    *idp = JSVAL_VOID;
	if (!JSID_IS_VOID(*idp))
            break;
        /*@fallthrough@*/
    case JSENUMERATE_DESTROY:
	ix = JSVAL_TO_INT(*statep);
	(void) JS_DefineProperty(cx, obj, "length", INT_TO_JSVAL(ix),
			NULL, NULL, JSPROP_ENUMERATE);
if (_debug)
fprintf(stderr, "\tFINI fd %p[%u]\n", fd, ix);
	*statep = JSVAL_NULL;
        break;
    }
#endif
    return JS_TRUE;
}

/* --- Object ctors/dtors */
static void
rpmio_dtor(JSContext *cx, JSObject *obj)
{
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmioClass, NULL);
    FD_t fd = ptr;

_DTOR_DEBUG_ENTRY(_debug);
    if (fd)
	(void) Fclose(fd);
}

static JSBool
rpmio_ctor(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_NewObjectForConstructor(cx, vp);
    JSBool ok = JS_FALSE;
    const char * _fn = NULL;
    const char * _fmode = "r.ufdio";

_CTOR_DEBUG_ENTRY(_debug);

    if (!(ok = JS_ConvertArguments(cx, argc, argv, "/ss", &_fn, &_fmode)))
        goto exit;

    if (JS_IsConstructing(cx, vp)) {
	(void) rpmio_init(cx, obj, _fn, _fmode);
    } else {
	if ((obj = JS_NewObject(cx, &rpmioClass, NULL, NULL)) == NULL)
	    goto exit;
	*vp = OBJECT_TO_JSVAL(obj);
    }
    ok = JS_TRUE;

exit:
    return ok;
}

static JSBool
rpmio_call(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    /* XXX obj is the global object so lookup "this" object. */
    JSObject * o = JSVAL_TO_OBJECT(argv[-2]);
    void * ptr = JS_GetInstancePrivate(cx, o, &rpmioClass, NULL);
    FD_t fd = ptr;
    JSBool ok = JS_FALSE;
    const char * _fn = NULL;
    const char * _fmode = "r.ufdio";

if (_debug)
fprintf(stderr, "==> %s(%p,%p[%u],%p) o %p ptr %p\n", __FUNCTION__, cx, argv, (unsigned)argc, vp, o, ptr);
    if (!(ok = JS_ConvertArguments(cx, argc, argv, "/ss", &_fn, &_fmode)))
        goto exit;

    if (fd) {
	(void) Fclose(fd);
	/* XXX error msg */
	fd = ptr = NULL;
	(void) JS_SetPrivate(cx, o, (void *)fd);
    }

    fd = ptr = rpmio_init(cx, o, _fn, _fmode);

    *vp = OBJECT_TO_JSVAL(o);

    ok = JS_TRUE;

exit:
if (_debug)
fprintf(stderr, "<== %s(%p,%p[%u],%p) o %p ptr %p\n", __FUNCTION__, cx, argv, (unsigned)argc, vp, o, ptr);

    return ok;
}

/* --- Class initialization */
JSClass rpmioClass = {
    "Io", JSCLASS_NEW_RESOLVE | JSCLASS_NEW_ENUMERATE | JSCLASS_HAS_PRIVATE,
    rpmio_addprop,   rpmio_delprop, rpmio_getprop, rpmio_setprop,
    (JSEnumerateOp)rpmio_enumerate, (JSResolveOp)rpmio_resolve,
    rpmio_convert,	rpmio_dtor,

    rpmio_getobjectops,	rpmio_checkaccess,
    rpmio_call,		rpmio_construct,
    rpmio_xdrobject,	rpmio_hasinstance,
    rpmio_mark,		rpmio_reserveslots,
};

JSObject *
rpmjs_InitIoClass(JSContext *cx, JSObject* obj)
{
    JSObject * proto = JS_InitClass(cx, obj, NULL, &rpmioClass, rpmio_ctor, 1,
		rpmio_props, rpmio_funcs, NULL, NULL);

if (_debug)
fprintf(stderr, "<== %s(%p,%p) proto %p\n", __FUNCTION__, cx, obj, proto);

assert(proto != NULL);
    return proto;
}

JSObject *
rpmjs_NewIoObject(JSContext *cx, const char * _fn, const char * _fmode)
{
    JSObject *obj;
    FD_t fd;

    if ((obj = JS_NewObject(cx, &rpmioClass, NULL, NULL)) == NULL) {
	/* XXX error msg */
	return NULL;
    }
    if ((fd = rpmio_init(cx, obj, _fn, _fmode)) == NULL) {
	/* XXX error msg */
	return NULL;
    }
    return obj;
}

GPSEE_MODULE_WRAP(rpmio, Io, JS_TRUE)
