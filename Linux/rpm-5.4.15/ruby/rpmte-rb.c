/** \ingroup rb_c
 * \file ruby/rpmte-rb.c
 */

#include "system.h"

#include "rpm-rb.h"
#include "rpmte-rb.h"

#ifdef	NOTYET
#include <argv.h>
#include <mire.h>
#endif

#define _RPMFI_INTERNAL
#include <rpmtag.h>
#include <rpmtypes.h>
#include <rpmio.h>
#include <rpmfi.h>
#include <rpmte.h>

#include "../debug.h"

VALUE rpmteClass;

/*@unchecked@*/
static int _debug = 0;

/* --- helpers */
static void *
rpmte_ptr(VALUE s)
{
    void *ptr;
    Data_Get_Struct(s, void, ptr);
    return ptr;
}

/* --- Object methods */

static void
initMethods(VALUE klass)
{
}

/* --- Object properties */
static VALUE
rpmte_debug_get(VALUE s)
{
if (_debug)
fprintf(stderr, "==> %s(0x%lx)\n", __FUNCTION__, s);
    return INT2FIX(_debug);
}

static VALUE
rpmte_debug_set(VALUE s, VALUE v)
{
if (_debug)
fprintf(stderr, "==> %s(0x%lx, 0x%lx)\n", __FUNCTION__, s, v);
    return INT2FIX(_debug = FIX2INT(v));
}

#ifdef	NOTYET
static VALUE
rpmte_rootdir_get(VALUE s)
{
    void *ptr = rpmte_ptr(s);
    rpmte te = ptr;
if (_debug)
fprintf(stderr, "==> %s(0x%lx) ptr %p\n", __FUNCTION__, s, ptr);
    return rb_str_new2(rpmteRootDir(te));
}

static VALUE
rpmte_rootdir_set(VALUE s, VALUE v)
{
    void *ptr = rpmte_ptr(s);
    rpmte te = ptr;
if (_debug)
fprintf(stderr, "==> %s(0x%lx, 0x%lx) ptr %p\n", __FUNCTION__, s, v, ptr);
    rpmteSetRootDir(te, StringValueCStr(v));
    return rb_str_new2(rpmteRootDir(te));
}

static VALUE
rpmte_vsflags_get(VALUE s)
{
    void *ptr = rpmte_ptr(s);
    rpmte te = ptr;
if (_debug)
fprintf(stderr, "==> %s(0x%lx) ptr %p\n", __FUNCTION__, s, ptr);
    return INT2FIX(_debug);
}

static VALUE
rpmte_vsflags_set(VALUE s, VALUE v)
{
    void *ptr = rpmte_ptr(s);
    rpmte te = ptr;
if (_debug)
fprintf(stderr, "==> %s(0x%lx, 0x%lx) ptr %p\n", __FUNCTION__, s, v, ptr);
    rpmteSetVSFlags(te, FIX2INT(v));
    return INT2FIX(rpmteVSFlags(te));
}
#endif	/* NOTYET */

static void
initProperties(VALUE klass)
{
    rb_define_method(klass, "debug", rpmte_debug_get, 0);
    rb_define_method(klass, "debug=", rpmte_debug_set, 1);
#ifdef	NOTYET
    rb_define_method(klass, "rootdir", rpmte_rootdir_get, 0);
    rb_define_method(klass, "rootdir=", rpmte_rootdir_set, 1);
    rb_define_method(klass, "vsflags", rpmte_vsflags_get, 0);
    rb_define_method(klass, "vsflags=", rpmte_vsflags_set, 1);
#endif	/* NOTYET */
}

/* --- Object ctors/dtors */
static void
rpmte_free(rpmte te)
{
if (_debug)
fprintf(stderr, "==> %s(%p)\n", __FUNCTION__, te);
#ifdef	NOTYET
    te = rpmteFree(te);
#else
    te = _free(te);
#endif
}

static VALUE
rpmte_alloc(VALUE klass)
{
#ifdef	NOTYET
    rpmte te = NULL;
#else
    rpmte te = xcalloc(1, sizeof(void *));
#endif
    VALUE obj = Data_Wrap_Struct(klass, 0, rpmte_free, te);
if (_debug)
fprintf(stderr, "==> %s(0x%lx) obj 0x%lx te %p\n", __FUNCTION__, klass, obj, te);
    return obj;
}

/* --- Class initialization */

void
Init_rpmte(void)
{
    rpmteClass = rb_define_class("Te", rb_cObject);
if (_debug)
fprintf(stderr, "==> %s() rpmteClass 0x%lx\n", __FUNCTION__, rpmteClass);
    rb_define_alloc_func(rpmteClass, rpmte_alloc);
    initProperties(rpmteClass);
    initMethods(rpmteClass);
}
