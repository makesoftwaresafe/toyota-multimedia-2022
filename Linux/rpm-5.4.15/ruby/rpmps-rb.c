/**
 * @ingroup rb_c
 * @file ruby/rpmps-rb.c
 */


#include "system.h"

#include "rpm-rb.h"
#include "rpmps-rb.h"

#ifdef	NOTYET
#include <argv.h>
#include <mire.h>
#endif

#define _RPMFI_INTERNAL
#include <rpmtag.h>
#include <rpmtypes.h>
#include <rpmio.h>
#include <rpmfi.h>
#include <rpmps.h>

#include "../debug.h"


VALUE rpmpsClass;

/*@unchecked@*/
static int _debug = 0;


/* --- helpers */
static void *
rpmps_ptr(VALUE s)
{
    void *ptr;
    Data_Get_Struct(s, void, ptr);
    return ptr;
}

/* --- Object methods */
static VALUE
rpmps_push(VALUE s)
{
    void *ptr = rpmps_ptr(s);
    rpmps ps = ptr;
    char *pkgNEVR = "PKG";
    char *altNEVR = "ALT";
    char *key = "KEY";
    rpmProblemType type = RPMPROB_PKG_INSTALLED;
    char *dn = "/root/";
    char *bn = "file";
    unsigned ui = 1234;

if (_debug)
fprintf(stderr, "==> %s(0x%lx) ptr %p\n", __FUNCTION__, s, ptr);
    rpmpsAppend(ps, type, pkgNEVR, key, dn, bn, altNEVR, ui);
    return INT2FIX(rpmpsNumProblems(ps));
}

static VALUE
rpmps_print(VALUE s)
{
    void *ptr = rpmps_ptr(s);
    rpmps ps = ptr;

if (_debug)
fprintf(stderr, "==> %s(0x%lx) ptr %p\n", __FUNCTION__, s, ptr);
    rpmpsPrint(NULL, ps);
    return INT2FIX(rpmpsNumProblems(ps));
}

static void
initMethods(VALUE klass)
{
    rb_define_method(klass, "push", rpmps_push, 7);
    rb_define_method(klass, "print", rpmps_print, 0);
}

/* --- Object properties */
static VALUE
rpmps_debug_get(VALUE s)
{
if (_debug)
fprintf(stderr, "==> %s(0x%lx)\n", __FUNCTION__, s);
    return INT2FIX(_debug);
}

static VALUE
rpmps_debug_set(VALUE s, VALUE v)
{
if (_debug)
fprintf(stderr, "==> %s(0x%lx, 0x%lx)\n", __FUNCTION__, s, v);
    return INT2FIX(_debug = FIX2INT(v));
}

static VALUE
rpmps_length_get(VALUE s)
{
    void *ptr = rpmps_ptr(s);
    rpmps ps = ptr;
if (_debug)
fprintf(stderr, "==> %s(0x%lx) ptr %p\n", __FUNCTION__, s, ptr);
    return INT2FIX(rpmpsNumProblems(ps));
}

#ifdef	NOTYET
static VALUE
rpmps_rootdir_set(VALUE s, VALUE v)
{
    void *ptr = rpmps_ptr(s);
    rpmps ps = ptr;
if (_debug)
fprintf(stderr, "==> %s(0x%lx, 0x%lx) ptr %p\n", __FUNCTION__, s, v, ptr);
    rpmpsSetRootDir(ps, StringValueCStr(v));
    return rb_str_new2(rpmpsRootDir(ps));
}

static VALUE
rpmps_vsflags_get(VALUE s)
{
    void *ptr = rpmps_ptr(s);
    rpmps ps = ptr;
if (_debug)
fprintf(stderr, "==> %s(0x%lx) ptr %p\n", __FUNCTION__, s, ptr);
    return INT2FIX(_debug);
}

static VALUE
rpmps_vsflags_set(VALUE s, VALUE v)
{
    void *ptr = rpmps_ptr(s);
    rpmps ps = ptr;
if (_debug)
fprintf(stderr, "==> %s(0x%lx, 0x%lx) ptr %p\n", __FUNCTION__, s, v, ptr);
    rpmpsSetVSFlags(ps, FIX2INT(v));
    return INT2FIX(rpmpsVSFlags(ps));
}
#endif	/* NOTYET */

static void
initProperties(VALUE klass)
{
    rb_define_method(klass, "debug", rpmps_debug_get, 0);
    rb_define_method(klass, "debug=", rpmps_debug_set, 1);
    rb_define_method(klass, "length", rpmps_length_get, 0);
#ifdef	NOTYET
    rb_define_method(klass, "rootdir", rpmps_rootdir_get, 0);
    rb_define_method(klass, "rootdir=", rpmps_rootdir_set, 1);
    rb_define_method(klass, "vsflags", rpmps_vsflags_get, 0);
    rb_define_method(klass, "vsflags=", rpmps_vsflags_set, 1);
#endif	/* NOTYET */
}

/* --- Object ctors/dtors */
static void
rpmps_free(rpmps ps)
{
if (_debug)
fprintf(stderr, "==> %s(%p)\n", __FUNCTION__, ps);
    ps = rpmpsFree(ps);
}

static VALUE
rpmps_alloc(VALUE klass)
{
    rpmps ps = rpmpsCreate();
    VALUE obj = Data_Wrap_Struct(klass, 0, rpmps_free, ps);
if (_debug)
fprintf(stderr, "==> %s(0x%lx) obj 0x%lx ps %p\n", __FUNCTION__, klass, obj, ps);
    return obj;
}

/* --- Class initialization */

void
Init_rpmps(void)
{
    rpmpsClass = rb_define_class("Ps", rb_cObject);
if (_debug)
fprintf(stderr, "==> %s() rpmpsClass 0x%lx\n", __FUNCTION__, rpmpsClass);
    rb_define_alloc_func(rpmpsClass, rpmps_alloc);
    initProperties(rpmpsClass);
    initMethods(rpmpsClass);
}
