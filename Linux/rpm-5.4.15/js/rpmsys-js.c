/** \ingroup js_c
 * \file js/rpmsys-js.c
 */

#include "system.h"

#include "rpmsys-js.h"
#include "rpmst-js.h"
#include "rpmjs-debug.h"
#include <rpmio.h>
#if defined(HAVE_PTHREAD_H)
#include <pthread.h>
#endif

#include "debug.h"

/*@unchecked@*/
static int _debug = 0;

/* Required JSClass vectors */
#define	rpmsys_addprop		JS_PropertyStub
#define	rpmsys_delprop		JS_PropertyStub
#define	rpmsys_convert		JS_ConvertStub

/* Optional JSClass vectors */
#define	rpmsys_getobjectops	NULL
#define	rpmsys_checkaccess	NULL
#define	rpmsys_call		NULL
#define	rpmsys_construct	rpmsys_ctor
#define	rpmsys_xdrobject	NULL
#define	rpmsys_hasinstance	NULL
#define	rpmsys_mark		NULL
#define	rpmsys_reserveslots	NULL

/* Extended JSClass vectors */
#define rpmsys_equality		NULL
#define rpmsys_outerobject	NULL
#define rpmsys_innerobject	NULL
#define rpmsys_iteratorobject	NULL
#define rpmsys_wrappedobject	NULL

typedef void * rpmsys;

/* --- helpers */
static rpmsys
rpmsys_init(JSContext *cx, JSObject *obj)
{
    rpmsys sys = xcalloc(1, sizeof(sys)); /* XXX distinguish class/instance? */

    if (!JS_SetPrivate(cx, obj, (void *)sys)) {
	/* XXX error msg */
	sys = NULL;
    }

if (_debug)
fprintf(stderr, "<== %s(%p,%p) sys %p\n", __FUNCTION__, cx, obj, sys);

    return sys;
}

/* --- Object methods */
static JSBool
rpmsys_access(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmsysClass, NULL);
    rpmsys sys = ptr;
    const char * _path = NULL;
    jsuint _mode = F_OK;
    JSBool ok;

_METHOD_DEBUG_ENTRY(_debug);
    if ((ok = JS_ConvertArguments(cx, argc, argv, "s/u", &_path, &_mode))) {
	mode_t mode = _mode;
	*vp = (sys && !Access(_path, mode)
		? JSVAL_ZERO : INT_TO_JSVAL(errno));
    }
    return ok;
}

static JSBool
rpmsys_chmod(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmsysClass, NULL);
    rpmsys sys = ptr;
    const char * _path = NULL;
    jsuint _mode = F_OK;
    JSBool ok;

_METHOD_DEBUG_ENTRY(_debug);
    if ((ok = JS_ConvertArguments(cx, argc, argv, "su", &_path, &_mode))) {
	mode_t mode = _mode;
	*vp = (sys && !Chmod(_path, mode)
		? JSVAL_ZERO : INT_TO_JSVAL(errno));
    }
    return ok;
}

static JSBool
rpmsys_chown(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmsysClass, NULL);
    rpmsys sys = ptr;
    const char * _path = NULL;
    jsint _uid = -1;
    jsint _gid = -1;
    JSBool ok;

_METHOD_DEBUG_ENTRY(_debug);
    if ((ok = JS_ConvertArguments(cx, argc, argv, "s/ii", &_path, &_uid, &_gid))) {
	uid_t uid = _uid;
	gid_t gid = _gid;
	*vp = (sys && !Chown(_path, uid, gid)
		? JSVAL_ZERO : INT_TO_JSVAL(errno));
    }
    return ok;
}

/* XXX Chdir */
/* XXX Chroot */

static JSBool
rpmsys_creat(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmsysClass, NULL);
    rpmsys sys = ptr;
    const char * _path = NULL;
    jsuint _mode = 0644;
    JSBool ok;

_METHOD_DEBUG_ENTRY(_debug);
    if ((ok = JS_ConvertArguments(cx, argc, argv, "s/u", &_path, &_mode))) {
	mode_t mode = _mode;
	int flags = O_CREAT|O_WRONLY|O_TRUNC;
	int fdno = -1;
	*vp = (sys && (fdno = open(_path, flags, mode)) >= 0
		? JSVAL_ZERO : INT_TO_JSVAL(errno));
	if (fdno >= 0) close(fdno);
    }
    return ok;
}

/* XXX Fallocate */
/* XXX Fchmod */
/* XXX Fchown */
/* XXX Fstat */

static JSBool
rpmsys_lchown(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmsysClass, NULL);
    rpmsys sys = ptr;
    const char * _path = NULL;
    jsint _uid = -1;
    jsint _gid = -1;
    JSBool ok;

_METHOD_DEBUG_ENTRY(_debug);
    if ((ok = JS_ConvertArguments(cx, argc, argv, "s/ii", &_path, &_uid, &_gid))) {
	uid_t uid = _uid;
	uid_t gid = _gid;
	*vp = (sys && !Lchown(_path, uid, gid)
		? JSVAL_ZERO : INT_TO_JSVAL(errno));
    }
    return ok;
}

static JSBool
rpmsys_link(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmsysClass, NULL);
    rpmsys sys = ptr;
    const char * _opath = NULL;
    const char * _npath = NULL;
    JSBool ok;

_METHOD_DEBUG_ENTRY(_debug);
    if ((ok = JS_ConvertArguments(cx, argc, argv, "ss", &_opath, &_npath))) {
	*vp = (sys && !Link(_opath, _npath)
		? JSVAL_ZERO : INT_TO_JSVAL(errno));
    }
    return ok;
}

static JSBool
rpmsys_lstat(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmsysClass, NULL);
    rpmsys sys = ptr;
    const char * _path = NULL;
    JSBool ok;

_METHOD_DEBUG_ENTRY(_debug);
    if ((ok = JS_ConvertArguments(cx, argc, argv, "s", &_path))) {
	struct stat sb;
	if (sys && !Lstat(_path, &sb)) {
	    JSObject *o;
	    struct stat *st = NULL;
	    size_t nb = sizeof(*st);
	    if ((st = memcpy(xmalloc(nb), &sb, nb)) != NULL
	     && (o = JS_NewObject(cx, &rpmstClass, NULL, NULL)) != NULL
	     && JS_SetPrivate(cx, o, (void *)st))
		*vp = OBJECT_TO_JSVAL(o);
	    else {
		if (st)	st = _free(st);
		*vp = JSVAL_VOID;		/* XXX goofy? */
	    }
	} else
	    *vp = INT_TO_JSVAL(errno);	/* XXX goofy? */
    }
    return ok;
}

static JSBool
rpmsys_mkdir(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmsysClass, NULL);
    rpmsys sys = ptr;
    const char * _path = NULL;
    jsuint _mode = 0755;
    JSBool ok;

_METHOD_DEBUG_ENTRY(_debug);
    if ((ok = JS_ConvertArguments(cx, argc, argv, "s/u", &_path, &_mode))) {
	mode_t mode = _mode;
	*vp = (sys && !Mkdir(_path, mode)
		? JSVAL_ZERO : INT_TO_JSVAL(errno));
    }
    return ok;
}

static JSBool
rpmsys_mkfifo(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmsysClass, NULL);
    rpmsys sys = ptr;
    const char * _path = NULL;
    jsuint _mode = 0755;
    JSBool ok;

_METHOD_DEBUG_ENTRY(_debug);
    if ((ok = JS_ConvertArguments(cx, argc, argv, "su", &_path, &_mode))) {
	mode_t mode = _mode;
	*vp = (sys && !Mkfifo(_path, mode)
		? JSVAL_ZERO : INT_TO_JSVAL(errno));
    }
    return ok;
}

static JSBool
rpmsys_mknod(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmsysClass, NULL);
    rpmsys sys = ptr;
    const char * _path = NULL;
    jsuint _mode = 0755;
    jsuint _dev = 0;
    JSBool ok;

_METHOD_DEBUG_ENTRY(_debug);
    if ((ok = JS_ConvertArguments(cx, argc, argv, "suu", &_path, &_mode, &_dev))) {
	mode_t mode = _mode;
	dev_t dev = _dev;
	*vp = (sys && !Mknod(_path, mode, dev)
		? JSVAL_ZERO : INT_TO_JSVAL(errno));
    }
    return ok;
}

static JSBool
rpmsys_readlink(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmsysClass, NULL);
    rpmsys sys = ptr;
    const char * _path = NULL;
    JSBool ok;

_METHOD_DEBUG_ENTRY(_debug);
    if ((ok = JS_ConvertArguments(cx, argc, argv, "s", &_path))) {
	char b[BUFSIZ];
	size_t nb = sizeof(b);
	ssize_t rc;

	if (sys && (rc = Readlink(_path, b, nb)) >= 0) {
	    b[rc] = '\0';
	    *vp = STRING_TO_JSVAL(JS_NewStringCopyZ(cx, b));
	} else
	    *vp = INT_TO_JSVAL(errno);
    }
    return ok;
}

/* XXX Realpath */

static JSBool
rpmsys_rename(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmsysClass, NULL);
    rpmsys sys = ptr;
    const char * _opath = NULL;
    const char * _npath = NULL;
    JSBool ok;

_METHOD_DEBUG_ENTRY(_debug);
    if ((ok = JS_ConvertArguments(cx, argc, argv, "ss", &_opath, &_npath))) {
	*vp = (sys && !Rename(_opath, _npath)
		? JSVAL_ZERO : INT_TO_JSVAL(errno));
    }
    return ok;
}

static JSBool
rpmsys_rmdir(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmsysClass, NULL);
    rpmsys sys = ptr;
    const char * _path = NULL;
    JSBool ok;

_METHOD_DEBUG_ENTRY(_debug);
    if ((ok = JS_ConvertArguments(cx, argc, argv, "s", &_path))) {
	*vp = (sys && !Rmdir(_path)
		? JSVAL_ZERO : INT_TO_JSVAL(errno));
    }
    return ok;
}

static JSBool
rpmsys_stat(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmsysClass, NULL);
    rpmsys sys = ptr;
    const char * _path = NULL;
    JSBool ok;

_METHOD_DEBUG_ENTRY(_debug);
    if ((ok = JS_ConvertArguments(cx, argc, argv, "s", &_path))) {
	struct stat sb;
	if (sys && !Stat(_path, &sb)) {
	    JSObject *o;
	    struct stat *st = NULL;
	    size_t nb = sizeof(*st);
	    if ((st = memcpy(xmalloc(nb), &sb, nb)) != NULL
	     && (o = JS_NewObject(cx, &rpmstClass, NULL, NULL)) != NULL
	     && JS_SetPrivate(cx, o, (void *)st))
		*vp = OBJECT_TO_JSVAL(o);
	    else {
		if (st)	st = _free(st);
		*vp = JSVAL_VOID;		/* XXX goofy? */
	    }
	} else
	    *vp = INT_TO_JSVAL(errno);	/* XXX goofy? */
    }
    return ok;
}

static JSBool
rpmsys_symlink(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmsysClass, NULL);
    rpmsys sys = ptr;
    const char * _opath = NULL;
    const char * _npath = NULL;
    JSBool ok;

_METHOD_DEBUG_ENTRY(_debug);
    if ((ok = JS_ConvertArguments(cx, argc, argv, "ss", &_opath, &_npath))) {
	*vp = (sys && !Symlink(_opath, _npath)
		? JSVAL_ZERO : INT_TO_JSVAL(errno));
    }
    return ok;
}

static JSBool
rpmsys_unlink(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmsysClass, NULL);
    rpmsys sys = ptr;
    const char * _path = NULL;
    JSBool ok;

_METHOD_DEBUG_ENTRY(_debug);
    if ((ok = JS_ConvertArguments(cx, argc, argv, "s", &_path))) {
	*vp = (sys && !Unlink(_path)
		? JSVAL_ZERO : INT_TO_JSVAL(errno));
    }
    return ok;
}

/* XXX Utime */
/* XXX Utimes */

static JSFunctionSpec rpmsys_funcs[] = {
    JS_FS("access",	rpmsys_access,		0,0),
    JS_FS("chmod",	rpmsys_chmod,		0,0),
    JS_FS("chown",	rpmsys_chown,		0,0),
    JS_FS("creat",	rpmsys_creat,		0,0),
    JS_FS("lchown",	rpmsys_lchown,		0,0),
    JS_FS("link",	rpmsys_link,		0,0),
    JS_FS("lstat",	rpmsys_lstat,		0,0),
    JS_FS("mkdir",	rpmsys_mkdir,		0,0),
    JS_FS("mkfifo",	rpmsys_mkfifo,		0,0),
    JS_FS("mknod",	rpmsys_mknod,		0,0),
    JS_FS("readlink",	rpmsys_readlink,	0,0),
    JS_FS("rename",	rpmsys_rename,		0,0),
    JS_FS("rmdir",	rpmsys_rmdir,		0,0),
    JS_FS("stat",	rpmsys_stat,		0,0),
    JS_FS("symlink",	rpmsys_symlink,		0,0),
    JS_FS("unlink",	rpmsys_unlink,		0,0),
    JS_FS_END
};

/* --- Object properties */
enum rpmsys_tinyid {
    _DEBUG	= -2,
    _CTERMID	= -3,
    _CTIME	= -4,
    _CWD	= -5,
    _DOMAINNAME	= -6,
    _EGID	= -7,
    _EUID	= -8,
    _GID	= -9,
    _GROUPS	= -10,
    _HOSTID	= -11,
    _HOSTNAME	= -12,
    _PGID	= -13,
    _PID	= -14,
    _PPID	= -15,
    _SID	= -16,
    _TID	= -17,
    _TIME	= -18,
    _TIMEOFDAY	= -19,
    _UID	= -20,
    _UMASK	= -21,
    _UNAME	= -22,

    _RLIMIT	= -31,	/* todo++ */
    _RUSAGE	= -32,	/* todo++ */
};

static JSPropertySpec rpmsys_props[] = {
    {"debug",	_DEBUG,		JSPROP_ENUMERATE,	NULL,	NULL},
    {"ctermid",	_CTERMID,	JSPROP_ENUMERATE,	NULL,	NULL},
    {"ctime",	_CTIME,		JSPROP_ENUMERATE,	NULL,	NULL},
    {"cwd",	_CWD,		JSPROP_ENUMERATE,	NULL,	NULL},
    {"domainname",_DOMAINNAME,	JSPROP_ENUMERATE,	NULL,	NULL},
    {"egid",	_EGID,		JSPROP_ENUMERATE,	NULL,	NULL},
    {"euid",	_EUID,		JSPROP_ENUMERATE,	NULL,	NULL},
    {"gid",	_GID,		JSPROP_ENUMERATE,	NULL,	NULL},
    {"groups",	_GROUPS,	JSPROP_ENUMERATE,	NULL,	NULL},
    {"hostid",	_HOSTID,	JSPROP_ENUMERATE,	NULL,	NULL},
    {"hostname",_HOSTNAME,	JSPROP_ENUMERATE,	NULL,	NULL},
    {"pgid",	_PGID,		JSPROP_ENUMERATE,	NULL,	NULL},
    {"pid",	_PID,		JSPROP_ENUMERATE,	NULL,	NULL},
    {"ppid",	_PPID,		JSPROP_ENUMERATE,	NULL,	NULL},
    {"sid",	_TID,		JSPROP_ENUMERATE,	NULL,	NULL},
    {"tid",	_TID,		JSPROP_ENUMERATE,	NULL,	NULL},
    {"time",	_TIME,		JSPROP_ENUMERATE,	NULL,	NULL},
    {"timeofday",_TIMEOFDAY,	JSPROP_ENUMERATE,	NULL,	NULL},
    {"uid",	_UID,		JSPROP_ENUMERATE,	NULL,	NULL},
    {"umask",	_UMASK,		JSPROP_ENUMERATE,	NULL,	NULL},
    {"uname",	_UNAME,		JSPROP_ENUMERATE,	NULL,	NULL},
    {NULL, 0, 0, NULL, NULL}
};

static JSBool
rpmsys_getprop(JSContext *cx, JSObject *obj, jsid id, jsval *vp)
{
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmsysClass, NULL);
    jsint tiny = JSVAL_TO_INT(id);

_PROP_DEBUG_ENTRY(_debug < 0);

    /* XXX the class has ptr == NULL, instances have ptr != NULL. */
    if (ptr == NULL)
	return JS_TRUE;

    switch (tiny) {
    case _DEBUG:	*vp = INT_TO_JSVAL(_debug);		break;
    case _CTERMID:
    {	char b[L_ctermid+1];
	*vp = STRING_TO_JSVAL(JS_NewStringCopyZ(cx, ctermid(b)));
    }	break;
    case _CTIME:
    {	char b[128];
	time_t secs = time(NULL);
	char * t = strchr(ctime_r(&secs, b), '\n');
	if (t) *t = '\0';
	*vp = STRING_TO_JSVAL(JS_NewStringCopyZ(cx, b));
    }	break;
    case _CWD:
    {	char b[PATH_MAX+1];
	*vp = STRING_TO_JSVAL(JS_NewStringCopyZ(cx, getcwd(b, sizeof(b))));
    }	break;
    case _DOMAINNAME:
    {	char b[PATH_MAX+1];
	*vp = (!getdomainname(b, sizeof(b))
	    ? STRING_TO_JSVAL(JS_NewStringCopyZ(cx, b)) : JSVAL_NULL);
    }	break;
    case _EGID:		*vp = INT_TO_JSVAL((int)getegid());	break;
    case _EUID:		*vp = INT_TO_JSVAL((int)geteuid());	break;
    case _GID:		*vp = INT_TO_JSVAL((int)getgid());	break;
    case _GROUPS:
    {	gid_t gids[NGROUPS_MAX];
	int ng = getgroups(NGROUPS_MAX, gids);
	if (ng > 0) {
	    jsval jsvec[NGROUPS_MAX];
	    int i;
	    for (i = 0; i < ng; i++)
		jsvec[i] = INT_TO_JSVAL((int)gids[i]);
	    *vp = OBJECT_TO_JSVAL(JS_NewArrayObject(cx, ng, jsvec));
	} else
	    *vp = JSVAL_NULL;
    }	break;
    case _HOSTID:	*vp = INT_TO_JSVAL((int)gethostid());	break;/* XXX */
    case _HOSTNAME:
    {	char b[PATH_MAX+1];
	*vp = (!gethostname(b, sizeof(b))
	    ? STRING_TO_JSVAL(JS_NewStringCopyZ(cx, b)) : JSVAL_NULL);
    }	break;
    case _PGID:		*vp = INT_TO_JSVAL((int)getpgid((pid_t)0));	break;
    case _PID:		*vp = INT_TO_JSVAL((int)getpid());	break;
    case _PPID:		*vp = INT_TO_JSVAL((int)getppid());	break;
    case _SID:		*vp = INT_TO_JSVAL((int)getsid((pid_t)0));	break;
#if defined(WITH_PTHREADS)
    case _TID:		*vp = INT_TO_JSVAL((int)((long)pthread_self())); break;
#endif
    case _TIME:		*vp = INT_TO_JSVAL((int)time(NULL));	break;
    case _TIMEOFDAY:
    {	struct timeval tv;
	if (!gettimeofday(&tv, NULL)) {
	    jsval jsvec[2];
	    jsvec[0] = INT_TO_JSVAL((int)tv.tv_sec);
	    jsvec[1] = INT_TO_JSVAL((int)tv.tv_usec);
	    *vp = OBJECT_TO_JSVAL(JS_NewArrayObject(cx, 2, jsvec));
	} else
	    *vp = JSVAL_NULL;
    }	break;
    case _UID:		*vp = INT_TO_JSVAL((int)getuid());	break;
    case _UMASK:
    {	mode_t mode = umask(0000);
	(void) umask(mode);
	*vp = INT_TO_JSVAL(mode);
    }	break;
    case _UNAME:	/* XXX FIXME */
    {	struct utsname uts;
	if (!uname(&uts)) {
	    jsval jsvec[5];
	    jsvec[0] = STRING_TO_JSVAL(
		JS_NewStringCopyN(cx, uts.sysname, sizeof(uts.sysname)));
	    jsvec[1] = STRING_TO_JSVAL(
		JS_NewStringCopyN(cx, uts.nodename, sizeof(uts.nodename)));
	    jsvec[2] = STRING_TO_JSVAL(
		JS_NewStringCopyN(cx, uts.release, sizeof(uts.release)));
	    jsvec[3] = STRING_TO_JSVAL(
		JS_NewStringCopyN(cx, uts.version, sizeof(uts.version)));
	    jsvec[4] = STRING_TO_JSVAL(
		JS_NewStringCopyN(cx, uts.machine, sizeof(uts.machine)));
	    *vp = OBJECT_TO_JSVAL(JS_NewArrayObject(cx, 5, jsvec));
	} else
	    *vp = JSVAL_NULL;
    }	break;
    default:
	break;
    }

    return JS_TRUE;
}

static JSBool
rpmsys_setprop(JSContext *cx, JSObject *obj, jsid id, JSBool strict, jsval *vp)
{
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmsysClass, NULL);
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
rpmsys_resolve(JSContext *cx, JSObject *obj, jsid id, uintN flags,
		JSObject **objp)
{
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmsysClass, NULL);

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
rpmsys_enumerate(JSContext *cx, JSObject *obj, JSIterateOp op,
		  jsval *statep, jsid *idp)
{
#ifdef	NOTYET
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmsysClass, NULL);
    rpmsys sys = ptr;
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
fprintf(stderr, "\tINIT sys %p\n", sys);
        break;
    case JSENUMERATE_NEXT:
	ix = JSVAL_TO_INT(*statep);
	if ((dp = Readdir(dir)) != NULL) {
	    (void) JS_DefineElement(cx, obj,
			ix, STRING_TO_JSVAL(JS_NewStringCopyZ(cx, dp->d_name)),
			NULL, NULL, JSPROP_ENUMERATE);
	    JS_ValueToId(cx, *statep, idp);
if (_debug)
fprintf(stderr, "\tNEXT sys %p[%u] dirent %p \"%s\"\n", sys, ix, dp, dp->d_name);
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
fprintf(stderr, "\tFINI sys %p[%u]\n", sys, ix);
	*statep = JSVAL_NULL;
        break;
    }
#endif
    return JS_TRUE;
}

/* --- Object ctors/dtors */
static void
rpmsys_dtor(JSContext *cx, JSObject *obj)
{
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmsysClass, NULL);
    rpmsys sys = ptr;

_DTOR_DEBUG_ENTRY(_debug);

    sys = _free(sys);
}

static JSBool
rpmsys_ctor(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JSObject *obj = JS_NewObjectForConstructor(cx, vp);
    JSBool ok = JS_FALSE;

_CTOR_DEBUG_ENTRY(_debug);

    if (JS_IsConstructing(cx, vp)) {
	(void) rpmsys_init(cx, obj);
    } else {
	if ((obj = JS_NewObject(cx, &rpmsysClass, NULL, NULL)) == NULL)
	    goto exit;
	*vp = OBJECT_TO_JSVAL(obj);
    }
    ok = JS_TRUE;

exit:
    return ok;
}

/* --- Class initialization */
JSClass rpmsysClass = {
    "Sys", JSCLASS_NEW_RESOLVE | JSCLASS_NEW_ENUMERATE | JSCLASS_HAS_PRIVATE,
    rpmsys_addprop,   rpmsys_delprop, rpmsys_getprop, rpmsys_setprop,
    (JSEnumerateOp)rpmsys_enumerate, (JSResolveOp)rpmsys_resolve,
    rpmsys_convert,	rpmsys_dtor,

    rpmsys_getobjectops,	rpmsys_checkaccess,
    rpmsys_call,		rpmsys_construct,
    rpmsys_xdrobject,	rpmsys_hasinstance,
    rpmsys_mark,		rpmsys_reserveslots,
};

JSObject *
rpmjs_InitSysClass(JSContext *cx, JSObject* obj)
{
    JSObject * proto = JS_InitClass(cx, obj, NULL, &rpmsysClass, rpmsys_ctor, 1,
		rpmsys_props, rpmsys_funcs, NULL, NULL);

if (_debug)
fprintf(stderr, "<== %s(%p,%p) proto %p\n", __FUNCTION__, cx, obj, proto);

assert(proto != NULL);
    return proto;
}

JSObject *
rpmjs_NewSysObject(JSContext *cx)
{
    JSObject *obj;
    rpmsys sys;

    if ((obj = JS_NewObject(cx, &rpmsysClass, NULL, NULL)) == NULL) {
	/* XXX error msg */
	return NULL;
    }
    if ((sys = rpmsys_init(cx, obj)) == NULL) {
	/* XXX error msg */
	return NULL;
    }
    return obj;
}

GPSEE_MODULE_WRAP(rpmsys, Sys, JS_TRUE)
