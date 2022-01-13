/** \ingroup js_c
 * \file js/rpmfts-js.c
 */

#include "system.h"

#include "rpmfts-js.h"
#include "rpmst-js.h"
#include "rpmjs-debug.h"

#include <fts.h>
#include <argv.h>

#include "debug.h"

/*@unchecked@*/
static int _debug = 0;

/* Required JSClass vectors */
#define	rpmfts_addprop		JS_PropertyStub
#define	rpmfts_delprop		JS_PropertyStub
#define	rpmfts_convert		JS_ConvertStub

/* Optional JSClass vectors */
#define	rpmfts_getobjectops	NULL
#define	rpmfts_checkaccess	NULL
#define	rpmfts_call		rpmfts_call
#define	rpmfts_construct	rpmfts_ctor
#define	rpmfts_xdrobject	NULL
#define	rpmfts_hasinstance	NULL
#define	rpmfts_mark		NULL
#define	rpmfts_reserveslots	NULL

/* Extended JSClass vectors */
#define rpmfts_equality		NULL
#define rpmfts_outerobject	NULL
#define rpmfts_innerobject	NULL
#define rpmfts_iteratorobject	NULL
#define rpmfts_wrappedobject	NULL

/* --- helpers */
static FTS *
rpmfts_init(JSContext *cx, JSObject *obj, JSObject *dno, int _options)
{
    FTS * fts = NULL;

    if (dno) {
	ARGV_t av = NULL;
	const char * s = NULL;
	if (JS_IsArrayObject(cx, dno)) {
	    jsuint length = 0;
	    jsuint i;

	    if (JS_GetArrayLength(cx, dno, &length))
	    for (i = 0; i < length; i++) {
		jsval v;
		if (JS_GetElement(cx, dno, (jsint)i, &v)) {
		    s = JS_EncodeString(cx, JSVAL_TO_STRING(v));
		    argvAdd(&av, s);
		    s = _free(s);
		}
	    }
	} else {
	    s = JS_EncodeString(cx, JS_ValueToString(cx, OBJECT_TO_JSVAL(dno)));
	    argvAdd(&av, s);
	    s = _free(s);
	}
	
	if (_options == -1) _options = 0;
	_options &= FTS_OPTIONMASK;
	/* XXX FIXME: validate _options */
	if (av)
	    fts = Fts_open((char *const *)av, _options, NULL);
	    /* XXX error msg */
	av = argvFree(av);
	if (!JS_SetPrivate(cx, obj, (void *)fts)) {
	    /* XXX error msg */
	    if (fts) {
		(void) Fts_close(fts);
		/* XXX error msg */
	    }
	    fts = NULL;
	}
    }

if (_debug)
fprintf(stderr, "<== %s(%p,%p,%p) fts %p\n", __FUNCTION__, cx, obj, dno, fts);

    return fts;
}

/* --- Object methods */
static JSBool
rpmfts_children(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmftsClass, NULL);
    FTS * fts = ptr;
    FTSENT * p;
    int _instr = FTS_NOINSTR;
    JSBool ok = JS_FALSE;

_METHOD_DEBUG_ENTRY(_debug);

    if (!(ok = JS_ConvertArguments(cx, argc, argv, "/u", &_instr)))
        goto exit;

    /* XXX FIXME: FTS_children() return? */
    *vp = (fts && (p = Fts_children(fts, _instr)) != NULL
		? OBJECT_TO_JSVAL(obj) : JSVAL_FALSE);

    ok = JS_TRUE;
exit:
    return ok;
}

static JSBool
rpmfts_close(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmftsClass, NULL);
    FTS * fts = ptr;
    JSBool ok = JS_FALSE;

_METHOD_DEBUG_ENTRY(_debug);

    /* XXX FIXME: _options are not persistent across fts.close() */
    if (fts) {
	(void) Fts_close(fts);
	/* XXX error msg */
	fts = ptr = NULL;
	(void) JS_SetPrivate(cx, obj, (void *)fts);
    }
    *vp = OBJECT_TO_JSVAL(obj);

    ok = JS_TRUE;
    return ok;
}

static JSBool
rpmfts_open(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmftsClass, NULL);
    FTS * fts = ptr;
    JSObject *dno = NULL;
    int _options = -1;
    JSBool ok = JS_FALSE;

_METHOD_DEBUG_ENTRY(_debug);

    if (!(ok = JS_ConvertArguments(cx, argc, argv, "o/u", &dno, &_options)))
        goto exit;

    if (fts) {
	if (_options == -1) _options = fts->fts_options;
	(void) Fts_close(fts);
	/* XXX error msg */
	fts = ptr = NULL;
	(void) JS_SetPrivate(cx, obj, (void *)fts);
    }

    fts = ptr = rpmfts_init(cx, obj, dno, _options);

    *vp = OBJECT_TO_JSVAL(obj);

    ok = JS_TRUE;
exit:
    return ok;
}

static JSBool
rpmfts_read(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmftsClass, NULL);
    FTS * fts = ptr;
    JSBool ok = JS_FALSE;

_METHOD_DEBUG_ENTRY(_debug);

    *vp = (fts && Fts_read(fts) ? OBJECT_TO_JSVAL(obj) : JSVAL_FALSE);

    ok = JS_TRUE;
    return ok;
}

static JSBool
rpmfts_set(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmftsClass, NULL);
    FTS * fts = ptr;
    FTSENT * p = (fts ? fts->fts_cur : NULL);
    int _instr = FTS_NOINSTR;
    JSBool ok = JS_FALSE;

_METHOD_DEBUG_ENTRY(_debug);

    if (!(ok = JS_ConvertArguments(cx, argc, argv, "/u", &_instr)))
        goto exit;

    *vp = (fts && p && !Fts_set(fts, p, _instr)
		? OBJECT_TO_JSVAL(obj) : JSVAL_FALSE);

    ok = JS_TRUE;
exit:
    return ok;
}

static JSFunctionSpec rpmfts_funcs[] = {
    JS_FS("children",	rpmfts_children,	0,0),
    JS_FS("close",	rpmfts_close,		0,0),
    JS_FS("open",	rpmfts_open,		0,0),
    JS_FS("read",	rpmfts_read,		0,0),
    JS_FS("set",	rpmfts_set,		0,0),
    JS_FS_END
};

/* --- Object properties */
enum rpmfts_tinyid {
    _DEBUG	= -2,

    /* FTS fields */
    _CURRENT	= -10,
    _CHILD	= -11,
    _ARRAY	= -12,
    _ROOTDEV	= -13,
    _ROOT	= -14,
    _ROOTLEN	= -15,
    _NITEMS	= -16,
    _OPTIONS	= -17,

    /* FTSENT fields */
    _CYCLE	= -20,
    _PARENT	= -21,
    _LINK	= -22,
    _NUMBER	= -23,
    _POINTER	= -24,
    _ACCPATH	= -25,
    _PATH	= -26,
    _ERRNO	= -27,
    _PATHLEN	= -28,
    _NAMELEN	= -29,
    _INO	= -30,
    _DEV	= -31,
    _NLINK	= -32,
    _LEVEL	= -33,
    _INFO	= -34,
    _FLAGS	= -35,
    _INSTR	= -36,
    _STATP	= -37,
    _NAME	= -38,
};

static JSPropertySpec rpmfts_props[] = {
    {"debug",	_DEBUG,		JSPROP_ENUMERATE,	NULL,	NULL},

    /* FTS fields */
    {"current",	_CURRENT,	JSPROP_ENUMERATE,	NULL,	NULL},
    {"child",	_CHILD,		JSPROP_ENUMERATE,	NULL,	NULL},
    {"array",	_ARRAY,		JSPROP_ENUMERATE,	NULL,	NULL},
    {"rootdev",	_ROOTDEV,	JSPROP_ENUMERATE,	NULL,	NULL},
    {"root",	_ROOT,		JSPROP_ENUMERATE,	NULL,	NULL},
    {"rootlen",	_ROOTLEN,	JSPROP_ENUMERATE,	NULL,	NULL},
    {"nitems",	_NITEMS,	JSPROP_ENUMERATE,	NULL,	NULL},
    {"options",	_OPTIONS,	JSPROP_ENUMERATE,	NULL,	NULL},

    /* FTSENT fields */
    {"cycle",	_CYCLE,		JSPROP_ENUMERATE,	NULL,	NULL},
    {"parent",	_PARENT,	JSPROP_ENUMERATE,	NULL,	NULL},
    {"link",	_LINK,		JSPROP_ENUMERATE,	NULL,	NULL},
    {"number",	_NUMBER,	JSPROP_ENUMERATE,	NULL,	NULL},
    {"pointer",	_POINTER,	JSPROP_ENUMERATE,	NULL,	NULL},
    {"accpath",	_ACCPATH,	JSPROP_ENUMERATE,	NULL,	NULL},
    {"path",	_PATH,		JSPROP_ENUMERATE,	NULL,	NULL},
    {"errno",	_ERRNO,		JSPROP_ENUMERATE,	NULL,	NULL},
    {"pathlen",	_PATHLEN,	JSPROP_ENUMERATE,	NULL,	NULL},
    {"namelen",	_NAMELEN,	JSPROP_ENUMERATE,	NULL,	NULL},
    {"ino",	_INO,		JSPROP_ENUMERATE,	NULL,	NULL},
    {"dev",	_DEV,		JSPROP_ENUMERATE,	NULL,	NULL},
    {"nlink",	_NLINK,		JSPROP_ENUMERATE,	NULL,	NULL},
    {"level",	_LEVEL,		JSPROP_ENUMERATE,	NULL,	NULL},
    {"info",	_INFO,		JSPROP_ENUMERATE,	NULL,	NULL},
    {"flags",	_FLAGS,		JSPROP_ENUMERATE,	NULL,	NULL},
    {"instr",	_INSTR,		JSPROP_ENUMERATE,	NULL,	NULL},
    {"st",	_STATP,		JSPROP_ENUMERATE,	NULL,	NULL},
    {"name",	_NAME,		JSPROP_ENUMERATE,	NULL,	NULL},

    {NULL, 0, 0, NULL, NULL}
};

#define	_GET_I(_p, _f)   ((_p) ? INT_TO_JSVAL((int)(_p)->_f) : JSVAL_VOID)
#define	_GET_S(_p, _f) \
    ((_p) ? STRING_TO_JSVAL(JS_NewStringCopyZ(cx, (_p)->_f)) : JSVAL_VOID)

static JSBool
rpmfts_getprop(JSContext *cx, JSObject *obj, jsid id, jsval *vp)
{
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmftsClass, NULL);
    FTS * fts = ptr;
    FTSENT * p = (fts ? fts->fts_cur : NULL);
    jsint tiny = JSVAL_TO_INT(id);

_PROP_DEBUG_ENTRY(_debug < 0);

    /* XXX the class has ptr == NULL, instances have ptr != NULL. */
    if (ptr == NULL)
	return JS_TRUE;

    switch (tiny) {
    case _DEBUG:	*vp = INT_TO_JSVAL(_debug);		break;

    /* FTS fields */
    case _CURRENT:	break;	/* unimplemented */
    case _CHILD:	break;	/* unimplemented */
    case _ARRAY:	break;	/* unimplemented */
    case _ROOTDEV:	*vp = _GET_I(fts, fts_dev);		break;
    case _ROOT:		*vp = _GET_S(fts, fts_path);		break;
    case _ROOTLEN:	*vp = _GET_I(fts, fts_pathlen);		break;
    case _NITEMS:	*vp = _GET_I(fts, fts_nitems);		break;
    case _OPTIONS:	*vp = _GET_I(fts, fts_options);		break;

    /* FTSENT fields */
    case _CYCLE:	break;	/* unimplemented */
    case _PARENT:	break;	/* unimplemented */
    case _LINK:		break;	/* unimplemented */
    case _NUMBER:	*vp = _GET_I(p, fts_number);		break;
    case _POINTER:	break;	/* unimplemented */
    case _ACCPATH:	*vp = _GET_S(p, fts_accpath);		break;
    case _PATH:		*vp = _GET_S(p, fts_path);		break;
    case _ERRNO:	*vp = _GET_I(p, fts_errno);		break;
    case _PATHLEN:	*vp = _GET_I(p, fts_pathlen);		break;
    case _NAMELEN:	*vp = _GET_I(p, fts_namelen);		break;
    case _INO:		*vp = _GET_I(p, fts_ino);		break;
    case _DEV:		*vp = _GET_I(p, fts_dev);		break;
    case _NLINK:	*vp = _GET_I(p, fts_nlink);		break;
    case _LEVEL:	*vp = _GET_I(p, fts_level);		break;
    case _INFO:		*vp = _GET_I(p, fts_info);		break;
    case _FLAGS:	*vp = _GET_I(p, fts_flags);		break;
    case _INSTR:	*vp = _GET_I(p, fts_instr);		break;
    case _STATP:
	if (fts && p && p->fts_statp) {
	    JSObject *o;
	    struct stat *st;
	    size_t nb = sizeof(*st);
	    if ((st = memcpy(xmalloc(nb), p->fts_statp, nb)) != NULL
	     && (o = JS_NewObject(cx, &rpmstClass, NULL, obj)) != NULL
	     && JS_SetPrivate(cx, o, (void *)st))
		*vp = OBJECT_TO_JSVAL(o);
	    else
		*vp = JSVAL_VOID;
	} else
	    *vp = JSVAL_VOID;
	break;
    case _NAME:		*vp = _GET_S(p, fts_name);		break;

    default:
	break;
    }

    return JS_TRUE;
}

#define	_SET_I(_p, _f) \
    if ((_p) && JS_ValueToInt32(cx, *vp, &myint)) (_p)->_f = myint

static JSBool
rpmfts_setprop(JSContext *cx, JSObject *obj, jsid id, JSBool strict, jsval *vp)
{
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmftsClass, NULL);
    FTS * fts = ptr;
    FTSENT * p = (fts ? fts->fts_cur : NULL);
    jsint tiny = JSVAL_TO_INT(id);
    int myint = 0;

_PROP_DEBUG_ENTRY(_debug < 0);

    /* XXX the class has ptr == NULL, instances have ptr != NULL. */
    if (ptr == NULL)
	return JS_TRUE;

    switch (tiny) {
    case _DEBUG:
	if (!JS_ValueToInt32(cx, *vp, &_debug))
	    break;
	break;

    /* FTS fields */
    case _CURRENT:	break;	/* unimplemented */
    case _CHILD:	break;	/* unimplemented */
    case _ARRAY:	break;	/* unimplemented */
    case _ROOTDEV:	break;	/* unimplemented */
    case _ROOT:		break;	/* unimplemented */
    case _ROOTLEN:	break;	/* unimplemented */
    case _NITEMS:	break;	/* unimplemented */
    case _OPTIONS:	break;	/* unimplemented */

    /* FTSENT fields */
    case _CYCLE:	break;	/* unimplemented */
    case _PARENT:	break;	/* unimplemented */
    case _LINK:		break;	/* unimplemented */
    case _NUMBER:	_SET_I(p, fts_number);		break;
    case _POINTER:	break;	/* unimplemented */
    case _ACCPATH:	break;	/* unimplemented */
    case _PATH:		break;	/* unimplemented */
    case _ERRNO:	break;	/* unimplemented */
    case _PATHLEN:	break;	/* unimplemented */
    case _NAMELEN:	break;	/* unimplemented */
    case _INO:		break;	/* unimplemented */
    case _DEV:		break;	/* unimplemented */
    case _NLINK:	break;	/* unimplemented */
    case _LEVEL:	break;	/* unimplemented */
    case _INFO:		break;	/* unimplemented */
    case _FLAGS:	break;	/* unimplemented */
    case _INSTR:	break;	/* unimplemented */
    case _STATP:	break;	/* unimplemented */
    case _NAME:		break;	/* unimplemented */

    default:
	break;
    }

    return JS_TRUE;
}

static JSBool
rpmfts_resolve(JSContext *cx, JSObject *obj, jsid id, uintN flags,
		JSObject **objp)
{
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmftsClass, NULL);

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
rpmfts_enumerate(JSContext *cx, JSObject *obj, JSIterateOp op,
		  jsval *statep, jsid *idp)
{
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmftsClass, NULL);
    FTS * fts = ptr;
    FTSENT * p;
    unsigned int ix = 0;

_ENUMERATE_DEBUG_ENTRY(_debug < 0);

    switch (op) {
    case JSENUMERATE_INIT:
    case JSENUMERATE_INIT_ALL:
        if (idp)
            *idp = JSVAL_ZERO;
	*statep = INT_TO_JSVAL(ix);
if (_debug)
fprintf(stderr, "\tINIT fts %p\n", fts);
        break;
    case JSENUMERATE_NEXT:
	ix = JSVAL_TO_INT(*statep);
	if ((p = Fts_read(fts)) != NULL) {
	    (void) JS_DefineElement(cx, obj,
			ix, STRING_TO_JSVAL(JS_NewStringCopyZ(cx, p->fts_path)),
			NULL, NULL, JSPROP_ENUMERATE);
	    JS_ValueToId(cx, *statep, idp);
if (_debug)
fprintf(stderr, "\tNEXT fts %p[%u] ftsent %p \"%s\"\n", fts, ix, p, p->fts_path);
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
fprintf(stderr, "\tFINI fts %p[%u]\n", fts, ix);
	*statep = JSVAL_NULL;
        break;
    }
    return JS_TRUE;
}

/* --- Object ctors/dtors */
static void
rpmfts_dtor(JSContext *cx, JSObject *obj)
{
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmftsClass, NULL);
    FTS * fts = ptr;

_DTOR_DEBUG_ENTRY(_debug);

    if (fts)
	(void) Fts_close(fts);
}

static JSBool
rpmfts_ctor(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_NewObjectForConstructor(cx, vp);
    JSBool ok = JS_FALSE;
    JSObject *dno = NULL;
    int _options = 0;

_CTOR_DEBUG_ENTRY(_debug);

    if (!(ok = JS_ConvertArguments(cx, argc, argv, "/ou", &dno, &_options)))
        goto exit;

    if (JS_IsConstructing(cx, vp)) {
	(void) rpmfts_init(cx, obj, dno, _options);
    } else {
	if ((obj = JS_NewObject(cx, &rpmftsClass, NULL, NULL)) == NULL)
	    goto exit;
	*vp = OBJECT_TO_JSVAL(obj);
    }
    ok = JS_TRUE;

exit:
    return ok;
}

static JSBool
rpmfts_call(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    /* XXX obj is the global object so lookup "this" object. */
    JSObject * o = JSVAL_TO_OBJECT(argv[-2]);
    void * ptr = JS_GetInstancePrivate(cx, o, &rpmftsClass, NULL);
    FTS * fts = ptr;
    JSBool ok = JS_FALSE;
    JSObject *dno = NULL;
    int _options = -1;

    if (!(ok = JS_ConvertArguments(cx, argc, argv, "/ou", &dno, &_options)))
        goto exit;

    if (fts) {
	if (_options == -1) _options = fts->fts_options;
	(void) Fts_close(fts);
	/* XXX error msg */
	fts = ptr = NULL;
	(void) JS_SetPrivate(cx, o, (void *)fts);
    }

    fts = ptr = rpmfts_init(cx, o, dno, _options);

    *vp = OBJECT_TO_JSVAL(o);

    ok = JS_TRUE;

exit:
if (_debug)
fprintf(stderr, "<== %s(%p,%p[%u],%p) o %p ptr %p\n", __FUNCTION__, cx, argv, (unsigned)argc, vp, o, ptr);

    return ok;
}

/* --- Class initialization */
JSClass rpmftsClass = {
    "Fts", JSCLASS_NEW_RESOLVE | JSCLASS_NEW_ENUMERATE | JSCLASS_HAS_PRIVATE,
    rpmfts_addprop,   rpmfts_delprop, rpmfts_getprop, rpmfts_setprop,
    (JSEnumerateOp)rpmfts_enumerate, (JSResolveOp)rpmfts_resolve,
    rpmfts_convert,	rpmfts_dtor,

    rpmfts_getobjectops,rpmfts_checkaccess,
    rpmfts_call,	rpmfts_construct,
    rpmfts_xdrobject,	rpmfts_hasinstance,
    rpmfts_mark,	rpmfts_reserveslots,
};

JSObject *
rpmjs_InitFtsClass(JSContext *cx, JSObject* obj)
{
    JSObject * proto = JS_InitClass(cx, obj, NULL, &rpmftsClass, rpmfts_ctor, 1,
		rpmfts_props, rpmfts_funcs, NULL, NULL);

if (_debug)
fprintf(stderr, "==> %s(%p,%p) proto %p\n", __FUNCTION__, cx, obj, proto);

assert(proto != NULL);
    return proto;
}

JSObject *
rpmjs_NewFtsObject(JSContext *cx, JSObject * dno, int _options)
{
    JSObject *obj;
    FTS * fts;

    if ((obj = JS_NewObject(cx, &rpmftsClass, NULL, NULL)) == NULL) {
	/* XXX error msg */
	return NULL;
    }
    if ((fts = rpmfts_init(cx, obj, dno, _options)) == NULL) {
	/* XXX error msg */
	return NULL;
    }
    return obj;
}

GPSEE_MODULE_WRAP(rpmfts, Fts, JS_TRUE)
