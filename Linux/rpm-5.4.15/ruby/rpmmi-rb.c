/** \ingroup rb_c
 * \file ruby/rpmmi-rb.c
 */

#include "system.h"

#include "rpm-rb.h"
#include "rpmts-rb.h"
#include "rpmmi-rb.h"
#include "rpmhdr-rb.h"

#ifdef	NOTYET
#include <argv.h>
#endif
#include <mire.h>

#include <rpmdb.h>
#include <rpmts.h>
#include <rpmio.h>

#include "../debug.h"

VALUE rpmmiClass;

/*@unchecked@*/
static int _debug = 0;

/* --- helpers */
static void *
rpmmi_ptr(VALUE s)
{
    void *ptr;
    Data_Get_Struct(s, void, ptr);
    return ptr;
}

/* --- Object methods */
static VALUE
rpmmi_each(VALUE s)
{
    rpmmi mi = rpmmi_ptr(s);
    Header h;
    while((h = rpmmiNext(mi)) != NULL)
	rb_yield (rpmrb_NewHdr(headerLink(h)));
    return Qnil;
}

static VALUE
rpmmi_next(VALUE s)
{
    rpmmi mi = rpmmi_ptr(s);
    Header h = rpmmiNext(mi);
    return (h != NULL ? rpmrb_NewHdr(headerLink(h)) : Qnil);
}

static VALUE
rpmmi_pattern(int argc, VALUE *argv, VALUE s)
{
    rpmmi mi = rpmmi_ptr(s);
    VALUE v_tag, v_pattern;

    rb_scan_args(argc, argv, "20", &v_tag, &v_pattern);

    rpmmiAddPattern(mi, FIX2INT(v_tag), RPMMIRE_REGEX,
		StringValueCStr(v_pattern));

    return Qtrue;
}

static void
initMethods(VALUE klass)
{
    rb_define_method(klass, "each", rpmmi_each, 0);
    rb_define_method(klass, "next", rpmmi_next, 0);
    rb_define_method(klass, "pattern", rpmmi_pattern, -1);
}

/* --- Object properties */
static VALUE
rpmmi_debug_get(VALUE s)
{
if (_debug)
fprintf(stderr, "==> %s(0x%lx)\n", __FUNCTION__, s);
    return INT2FIX(_debug);
}

static VALUE
rpmmi_debug_set(VALUE s, VALUE v)
{
    return INT2FIX(_debug = FIX2INT(v));
}

static VALUE
rpmmi_count_get(VALUE s)
{
    rpmmi mi = rpmmi_ptr(s);
    return INT2FIX(rpmmiCount(mi));
}

static VALUE
rpmmi_offset_get(VALUE s)
{
    rpmmi mi = rpmmi_ptr(s);
    return INT2FIX(rpmmiInstance(mi));
}

static void
initProperties(VALUE klass)
{
    rb_define_method(klass, "debug", rpmmi_debug_get, 0);
    rb_define_method(klass, "debug=", rpmmi_debug_set, 1);
    rb_define_method(klass, "length", rpmmi_count_get, 0);
    rb_define_method(klass, "count", rpmmi_count_get, 0);
    rb_define_method(klass, "offset", rpmmi_offset_get, 0);
    rb_define_method(klass, "instance", rpmmi_offset_get, 0);
}

/* --- Object ctors/dtors */
static void
rpmmi_free(rpmmi mi)
{
if (_debug)
fprintf(stderr, "==> %s(%p)\n", __FUNCTION__, mi);
    mi = rpmmiFree(mi);
}

static VALUE
rpmmi_new(int argc, VALUE *argv, VALUE s)
{
    VALUE v_ts, v_tag, v_key;
    rpmts ts;
    rpmTag _tag = RPMDBI_PACKAGES;
    void * _key = NULL;
    int _len = 0;
    rpmmi mi;

    rb_scan_args(argc, argv, "12", &v_ts, &v_tag, &v_key);

    ts = rpmmi_ptr(v_ts);
    if (!NIL_P(v_tag))
	_tag = FIX2INT(v_tag);
    if (!NIL_P(v_key))
	_key = StringValueCStr(v_key);

    mi = rpmtsInitIterator(ts, _tag, _key, _len);

if (_debug)
fprintf(stderr, "==> %s(%p[%d], 0x%lx) mi %p\n", __FUNCTION__, argv, argc, s, mi);
    return Data_Wrap_Struct(s, 0, rpmmi_free, mi);
}

/* --- Class initialization */

void
Init_rpmmi(void)
{
    rpmmiClass = rb_define_class("Mi", rb_cObject);
if (_debug)
fprintf(stderr, "==> %s() rpmmiClass 0x%lx\n", __FUNCTION__, rpmmiClass);
    rb_include_module(rpmmiClass, rb_mEnumerable);
    rb_define_singleton_method(rpmmiClass, "new", rpmmi_new, -1);
    initProperties(rpmmiClass);
    initMethods(rpmmiClass);
}

VALUE
rpmrb_NewMi(void * _ts, int _tag, void * _key, int _len)
{
    rpmmi mi = rpmtsInitIterator(_ts, _tag, _key, _len);
    return Data_Wrap_Struct(rpmmiClass, 0, rpmmi_free, mi);
}
