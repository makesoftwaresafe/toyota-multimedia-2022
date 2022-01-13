/**
 * \file ruby/rpmds-rb.c
 * \ingroup rb_c
 */

#include "system.h"
#include "rpm-rb.h"

#include "rpmds-rb.h"

#ifdef	NOTYET
#include <argv.h>
#include <mire.h>
#endif

#define _RPMDS_INTERNAL
#include <rpmtag.h>
#include <rpmtypes.h>
#include <rpmtag.h>
#include <rpmio.h>
#include <rpmds.h>

#include "../debug.h"


VALUE rpmdsClass;


/*@unchecked@*/
static int _debug = 0;

/* --- helpers */
static void *
rpmds_ptr(VALUE s)
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
rpmds_debug_get(VALUE s)
{
if (_debug)
fprintf(stderr, "==> %s(0x%lx)\n", __FUNCTION__, s);
    return INT2FIX(_debug);
}

static VALUE
rpmds_debug_set(VALUE s, VALUE v)
{
if (_debug)
fprintf(stderr, "==> %s(0x%lx, 0x%lx)\n", __FUNCTION__, s, v);
    return INT2FIX(_debug = FIX2INT(v));
}

static VALUE
rpmds_Count_get(VALUE s)
{
    rpmds ds = rpmds_ptr(s);
if (_debug)
fprintf(stderr, "==> %s(0x%lx) ptr %p\n", __FUNCTION__, s, ds);
    return INT2FIX(rpmdsCount(ds));
}

/* XXX FIXME: what to call the method? */
static VALUE
rpmds_Type_get(VALUE s)
{
    rpmds ds = rpmds_ptr(s);
if (_debug)
fprintf(stderr, "==> %s(0x%lx) ptr %p\n", __FUNCTION__, s, ds);
    return rb_str_new2(rpmdsType(ds));
}

static VALUE
rpmds_Ix_get(VALUE s)
{
    rpmds ds = rpmds_ptr(s);
if (_debug)
fprintf(stderr, "==> %s(0x%lx) ptr %p\n", __FUNCTION__, s, ds);
    return INT2FIX(rpmdsIx(ds));
}

static VALUE
rpmds_Ix_set(VALUE s, VALUE v)
{
    rpmds ds = rpmds_ptr(s);
    int ix = FIX2INT(v);
if (_debug)
fprintf(stderr, "==> %s(0x%lx) ptr %p\n", __FUNCTION__, s, ds);
    if (ix != rpmdsIx(ds)) {
	(void) rpmdsSetIx(ds, ix-1);
	/* XXX flush and recreate N and DNEVR with a rpmdsNext() step */
        (void) rpmdsNext(ds);
    }
    return INT2FIX(rpmdsIx(ds));
}

static VALUE
rpmds_BT_get(VALUE s)
{
    rpmds ds = rpmds_ptr(s);
if (_debug)
fprintf(stderr, "==> %s(0x%lx) ptr %p\n", __FUNCTION__, s, ds);
    return INT2FIX(rpmdsBT(ds));
}

static VALUE
rpmds_BT_set(VALUE s, VALUE v)
{
    rpmds ds = rpmds_ptr(s);
    int BT = FIX2INT(v);
if (_debug)
fprintf(stderr, "==> %s(0x%lx) ptr %p\n", __FUNCTION__, s, ds);
    (void) rpmdsSetBT(ds, BT);
    return INT2FIX(rpmdsBT(ds));
}

static VALUE
rpmds_Color_get(VALUE s)
{
    rpmds ds = rpmds_ptr(s);
if (_debug)
fprintf(stderr, "==> %s(0x%lx) ptr %p\n", __FUNCTION__, s, ds);
    return INT2FIX(rpmdsColor(ds));
}

static VALUE
rpmds_Color_set(VALUE s, VALUE v)
{
    rpmds ds = rpmds_ptr(s);
    int color = FIX2INT(v);
if (_debug)
fprintf(stderr, "==> %s(0x%lx) ptr %p\n", __FUNCTION__, s, ds);
    (void) rpmdsSetColor(ds, color);
    return INT2FIX(rpmdsColor(ds));
}

static VALUE
rpmds_NoPromote_get(VALUE s)
{
    rpmds ds = rpmds_ptr(s);
if (_debug)
fprintf(stderr, "==> %s(0x%lx) ptr %p\n", __FUNCTION__, s, ds);
    return INT2FIX(rpmdsNoPromote(ds));
}

static VALUE
rpmds_NoPromote_set(VALUE s, VALUE v)
{
    rpmds ds = rpmds_ptr(s);
    int nopromote = FIX2INT(v);
if (_debug)
fprintf(stderr, "==> %s(0x%lx) ptr %p\n", __FUNCTION__, s, ds);
    (void) rpmdsSetNoPromote(ds, nopromote);
    return INT2FIX(rpmdsNoPromote(ds));
}

static VALUE
rpmds_N_get(VALUE s)
{
    rpmds ds = rpmds_ptr(s);
if (_debug)
fprintf(stderr, "==> %s(0x%lx) ptr %p\n", __FUNCTION__, s, ds);
    return rb_str_new2(rpmdsN(ds));
}

static VALUE
rpmds_EVR_get(VALUE s)
{
    rpmds ds = rpmds_ptr(s);
if (_debug)
fprintf(stderr, "==> %s(0x%lx) ptr %p\n", __FUNCTION__, s, ds);
    return rb_str_new2(rpmdsEVR(ds));
}

static VALUE
rpmds_Flags_get(VALUE s)
{
    rpmds ds = rpmds_ptr(s);
if (_debug)
fprintf(stderr, "==> %s(0x%lx) ptr %p\n", __FUNCTION__, s, ds);
    return INT2FIX(rpmdsFlags(ds));
}

static VALUE
rpmds_DNEVR_get(VALUE s)
{
    rpmds ds = rpmds_ptr(s);
if (_debug)
fprintf(stderr, "==> %s(0x%lx) ptr %p\n", __FUNCTION__, s, ds);
    return rb_str_new2(rpmdsDNEVR(ds));
}

static void
initProperties(VALUE klass)
{
    rb_define_method(klass, "debug", rpmds_debug_get, 0);
    rb_define_method(klass, "debug=", rpmds_debug_set, 1);
    rb_define_method(klass, "length", rpmds_Count_get, 0);
    rb_define_method(klass, "ix", rpmds_Ix_get, 0);
    rb_define_method(klass, "ix=", rpmds_Ix_set, 1);
    rb_define_method(klass, "Type", rpmds_Type_get, 0);
    rb_define_method(klass, "buildtime", rpmds_BT_get, 0);
    rb_define_method(klass, "buildtime=", rpmds_BT_set, 1);
    rb_define_method(klass, "color", rpmds_Color_get, 0);
    rb_define_method(klass, "color=", rpmds_Color_set, 1);
    rb_define_method(klass, "nopromote", rpmds_NoPromote_get, 0);
    rb_define_method(klass, "nopromote=", rpmds_NoPromote_set, 1);
    rb_define_method(klass, "N", rpmds_N_get, 0);
    rb_define_method(klass, "EVR", rpmds_EVR_get, 0);
    rb_define_method(klass, "F", rpmds_Flags_get, 0);
    rb_define_method(klass, "DNEVR", rpmds_DNEVR_get, 0);
}

/* --- Object ctors/dtors */
static void
rpmds_free(rpmds ds)
{
if (_debug)
fprintf(stderr, "==> %s(%p)\n", __FUNCTION__, ds);
    ds = rpmdsFree(ds);
}

static VALUE
rpmds_new(int argc, VALUE *argv, VALUE s)
{
    VALUE v_hdr, v_tag;
    rpmTag tag = RPMTAG_NAME;
    int flags = 0;
    rpmds ds;

    rb_scan_args(argc, argv, "02", &v_hdr, &v_tag);

    if (!NIL_P(v_tag))
	tag = FIX2INT(v_tag);

    if (!NIL_P(v_hdr)) {
	Header h = rpmds_ptr(v_hdr);
	ds = rpmdsNew(h, tag, flags);
    } else
	(void) rpmdsRpmlib(&ds, NULL);

if (_debug)
fprintf(stderr, "==> %s(%p[%d], 0x%lx) mi %p\n", __FUNCTION__, argv, argc, s, ds);
    return Data_Wrap_Struct(s, 0, rpmds_free, ds);
}

/* --- Class initialization */

void
Init_rpmds(void)
{
    rpmdsClass = rb_define_class_under(rpmModule, "Ds", rb_cObject);
if (_debug)
fprintf(stderr, "==> %s() rpmdsClass 0x%lx\n", __FUNCTION__, rpmdsClass);
#ifdef  NOTYET
    rb_include_module(rpmdsClass, rb_mEnumerable);
#endif
    rb_define_singleton_method(rpmdsClass, "new", rpmds_new, -1);
    initProperties(rpmdsClass);
    initMethods(rpmdsClass);
}

VALUE
rpmrb_NewDs(void *_ds)
{
    return Data_Wrap_Struct(rpmdsClass, 0, rpmds_free, _ds);
}

