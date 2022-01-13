/** \ingroup rb_c
 * \file ruby/rpmfi-rb.c
 */

#include "system.h"

#include "rpm-rb.h"
#include "rpmfi-rb.h"

#ifdef	NOTYET
#include <argv.h>
#include <mire.h>
#endif

#define _RPMFI_INTERNAL
#include <rpmtag.h>
#include <rpmtypes.h>
#include <rpmio.h>
#include <rpmfi.h>
#include <rpmts.h>


#include "../debug.h"


VALUE rpmfiClass;


/*@unchecked@*/
static int _debug = 0;


/* --- helpers */
static void *
rpmfi_ptr(VALUE s)
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
rpmfi_debug_get(VALUE s)
{
if (_debug)
fprintf(stderr, "==> %s(0x%lx)\n", __FUNCTION__, s);
    return INT2FIX(_debug);
}

static VALUE
rpmfi_debug_set(VALUE s, VALUE v)
{
if (_debug)
fprintf(stderr, "==> %s(0x%lx, 0x%lx)\n", __FUNCTION__, s, v);
    return INT2FIX(_debug = FIX2INT(v));
}

static VALUE
rpmfi_FC_get(VALUE s)
{
    rpmfi fi = rpmfi_ptr(s);
if (_debug)
fprintf(stderr, "==> %s(0x%lx) ptr %p\n", __FUNCTION__, s, fi);
    return INT2FIX(rpmfiFC(fi));
}

static VALUE
rpmfi_FX_get(VALUE s)
{
    rpmfi fi = rpmfi_ptr(s);
if (_debug)
fprintf(stderr, "==> %s(0x%lx) ptr %p\n", __FUNCTION__, s, fi);
    return INT2FIX(rpmfiFX(fi));
}

static VALUE
rpmfi_FX_set(VALUE s, VALUE v)
{
    rpmfi fi = rpmfi_ptr(s);
    int ix = FIX2INT(v);
if (_debug)
fprintf(stderr, "==> %s(0x%lx) ptr %p\n", __FUNCTION__, s, fi);
    if (ix != rpmfiFX(fi)) {
        (void) rpmfiSetFX(fi, ix-1);
	/* XXX flush and recreate {BN,DN,FN} with a rpmfiNext() step */
        (void) rpmfiNext(fi);
    }
    return INT2FIX(rpmfiFX(fi));
}

static VALUE
rpmfi_DC_get(VALUE s)
{
    rpmfi fi = rpmfi_ptr(s);
if (_debug)
fprintf(stderr, "==> %s(0x%lx) ptr %p\n", __FUNCTION__, s, fi);
    return INT2FIX(rpmfiDC(fi));
}

static VALUE
rpmfi_DX_get(VALUE s)
{
    rpmfi fi = rpmfi_ptr(s);
if (_debug)
fprintf(stderr, "==> %s(0x%lx) ptr %p\n", __FUNCTION__, s, fi);
    return INT2FIX(rpmfiDX(fi));
}

static VALUE
rpmfi_DX_set(VALUE s, VALUE v)
{
    rpmfi fi = rpmfi_ptr(s);
    int ix = FIX2INT(v);
if (_debug)
fprintf(stderr, "==> %s(0x%lx) ptr %p\n", __FUNCTION__, s, fi);
    (void) rpmfiSetDX(fi, ix);
    if (ix != rpmfiDX(fi)) {
        (void) rpmfiSetDX(fi, ix-1);
	/* XXX flush and recreate {BN,DN,FN} with a rpmfiNextD() step */
        (void) rpmfiNextD(fi);
    }
    return INT2FIX(rpmfiDX(fi));
}

static VALUE
rpmfi_BN_get(VALUE s)
{
    rpmfi fi = rpmfi_ptr(s);
if (_debug)
fprintf(stderr, "==> %s(0x%lx) ptr %p\n", __FUNCTION__, s, fi);
    return rb_str_new2(rpmfiBN(fi));
}

static VALUE
rpmfi_DN_get(VALUE s)
{
    rpmfi fi = rpmfi_ptr(s);
if (_debug)
fprintf(stderr, "==> %s(0x%lx) ptr %p\n", __FUNCTION__, s, fi);
    return rb_str_new2(rpmfiDN(fi));
}

static VALUE
rpmfi_FN_get(VALUE s)
{
    rpmfi fi = rpmfi_ptr(s);
if (_debug)
fprintf(stderr, "==> %s(0x%lx) ptr %p\n", __FUNCTION__, s, fi);
    return rb_str_new2(rpmfiFN(fi));
}

static VALUE
rpmfi_VFlags_get(VALUE s)
{
    rpmfi fi = rpmfi_ptr(s);
if (_debug)
fprintf(stderr, "==> %s(0x%lx) ptr %p\n", __FUNCTION__, s, fi);
    return INT2FIX(rpmfiVFlags(fi));
}

static VALUE
rpmfi_FFlags_get(VALUE s)
{
    rpmfi fi = rpmfi_ptr(s);
if (_debug)
fprintf(stderr, "==> %s(0x%lx) ptr %p\n", __FUNCTION__, s, fi);
    return INT2FIX(rpmfiFFlags(fi));
}

static VALUE
rpmfi_FMode_get(VALUE s)
{
    rpmfi fi = rpmfi_ptr(s);
if (_debug)
fprintf(stderr, "==> %s(0x%lx) ptr %p\n", __FUNCTION__, s, fi);
    return INT2FIX(rpmfiFMode(fi));
}

static VALUE
rpmfi_FState_get(VALUE s)
{
    rpmfi fi = rpmfi_ptr(s);
if (_debug)
fprintf(stderr, "==> %s(0x%lx) ptr %p\n", __FUNCTION__, s, fi);
    return INT2FIX(rpmfiFState(fi));
}

static VALUE
rpmfi_FDigest_get(VALUE self)
{
    rpmfi fi = rpmfi_ptr(self);
    int dalgo = 0;
    size_t dlen = 0;
    const unsigned char * digest = rpmfiDigest(fi, &dalgo, &dlen);
    const unsigned char * s = digest;
    size_t nb = 2 * dlen;
    char * fdigest = memset(alloca(nb+1), 0, nb+1);
    char *t = fdigest;
    int i;

if (_debug)
fprintf(stderr, "==> %s(0x%lx) ptr %p\n", __FUNCTION__, self, fi);
    for (i = 0, s = digest, t = fdigest; i < (int)dlen; i++, s++, t+= 2) {
	static const char hex[] = "0123456789abcdef";
	t[0] = hex[(s[0] >> 4) & 0xf];
	t[1] = hex[(s[0]     ) & 0xf];
    }
    *t = '\0';

    return rb_str_new2(fdigest);
}

static VALUE
rpmfi_FLink_get(VALUE s)
{
    rpmfi fi = rpmfi_ptr(s);
if (_debug)
fprintf(stderr, "==> %s(0x%lx) ptr %p\n", __FUNCTION__, s, fi);
    return INT2FIX(rpmfiFLink(fi));
}

static VALUE
rpmfi_FSize_get(VALUE s)
{
    rpmfi fi = rpmfi_ptr(s);
if (_debug)
fprintf(stderr, "==> %s(0x%lx) ptr %p\n", __FUNCTION__, s, fi);
    return INT2FIX(rpmfiFSize(fi));
}

static VALUE
rpmfi_FRdev_get(VALUE s)
{
    rpmfi fi = rpmfi_ptr(s);
if (_debug)
fprintf(stderr, "==> %s(0x%lx) ptr %p\n", __FUNCTION__, s, fi);
    return INT2FIX(rpmfiFRdev(fi));
}

static VALUE
rpmfi_FMtime_get(VALUE s)
{
    rpmfi fi = rpmfi_ptr(s);
if (_debug)
fprintf(stderr, "==> %s(0x%lx) ptr %p\n", __FUNCTION__, s, fi);
    return INT2FIX(rpmfiFMtime(fi));
}

static VALUE
rpmfi_FUser_get(VALUE s)
{
    rpmfi fi = rpmfi_ptr(s);
if (_debug)
fprintf(stderr, "==> %s(0x%lx) ptr %p\n", __FUNCTION__, s, fi);
    return rb_str_new2(rpmfiFUser(fi));
}

static VALUE
rpmfi_FGroup_get(VALUE s)
{
    rpmfi fi = rpmfi_ptr(s);
if (_debug)
fprintf(stderr, "==> %s(0x%lx) ptr %p\n", __FUNCTION__, s, fi);
    return rb_str_new2(rpmfiFGroup(fi));
}

static VALUE
rpmfi_FColor_get(VALUE s)
{
    rpmfi fi = rpmfi_ptr(s);
if (_debug)
fprintf(stderr, "==> %s(0x%lx) ptr %p\n", __FUNCTION__, s, fi);
    return INT2FIX(rpmfiFColor(fi));
}

static VALUE
rpmfi_FClass_get(VALUE s)
{
    rpmfi fi = rpmfi_ptr(s);
    const char * FClass = rpmfiFClass(fi);
    char * t;
    VALUE v;
if (_debug)
fprintf(stderr, "==> %s(0x%lx) ptr %p\n", __FUNCTION__, s, fi);
    if (FClass == NULL) FClass = "";
    /* XXX spot fix for known embedded single quotes. */
    t = xstrdup(FClass);
    if (!strncmp(t, "symbolic link to `", sizeof("symbolic link to `")-1))
	t[sizeof("symbolic link")-1] = '\0';
    v = rb_str_new2(FClass);
    t = _free(t);
    return v;
}

static void
initProperties(VALUE klass)
{
    rb_define_method(klass, "debug", rpmfi_debug_get, 0);
    rb_define_method(klass, "debug=", rpmfi_debug_set, 1);
    rb_define_method(klass, "length", rpmfi_FC_get, 0);
    rb_define_method(klass, "fc", rpmfi_FC_get, 0);
    rb_define_method(klass, "fx", rpmfi_FX_get, 0);
    rb_define_method(klass, "fx=", rpmfi_FX_set, 1);
    rb_define_method(klass, "dc", rpmfi_DC_get, 0);
    rb_define_method(klass, "dx", rpmfi_DX_get, 0);
    rb_define_method(klass, "dx=", rpmfi_DX_set, 1);
    rb_define_method(klass, "bn", rpmfi_BN_get, 0);
    rb_define_method(klass, "dn", rpmfi_DN_get, 0);
    rb_define_method(klass, "fn", rpmfi_FN_get, 0);
    rb_define_method(klass, "vflags", rpmfi_VFlags_get, 0);
    rb_define_method(klass, "fflags", rpmfi_FFlags_get, 0);
    rb_define_method(klass, "fmode", rpmfi_FMode_get, 0);
    rb_define_method(klass, "fstate", rpmfi_FState_get, 0);
    rb_define_method(klass, "fdigest", rpmfi_FDigest_get, 0);
    rb_define_method(klass, "flink", rpmfi_FLink_get, 0);
    rb_define_method(klass, "fsize", rpmfi_FSize_get, 0);
    rb_define_method(klass, "frdev", rpmfi_FRdev_get, 0);
    rb_define_method(klass, "fmtime", rpmfi_FMtime_get, 0);
    rb_define_method(klass, "fuser", rpmfi_FUser_get, 0);
    rb_define_method(klass, "fgroup", rpmfi_FGroup_get, 0);
    rb_define_method(klass, "fcolor", rpmfi_FColor_get, 0);
    rb_define_method(klass, "fclass", rpmfi_FClass_get, 0);
}

/* --- Object ctors/dtors */
static void
rpmfi_free(rpmfi fi)
{
if (_debug)
fprintf(stderr, "==> %s(%p)\n", __FUNCTION__, fi);
    fi = rpmfiFree(fi);
}

static VALUE
rpmfi_new(int argc, VALUE *argv, VALUE s)
{
    VALUE v_ts, v_hdr, v_tag;
    rpmts ts;
    Header h;
    rpmTag tag = RPMTAG_BASENAMES;
    int flags = 0;
    rpmfi fi;

    rb_scan_args(argc, argv, "21", &v_ts, &v_hdr, &v_tag);

    ts = rpmfi_ptr(v_ts);
    h = rpmfi_ptr(v_hdr);

    if (!NIL_P(v_tag))
	tag = FIX2INT(v_tag);

    fi = rpmfiNew(ts, h, tag, flags);

if (_debug)
fprintf(stderr, "==> %s(%p[%d], 0x%lx) mi %p\n", __FUNCTION__, argv, argc, s, fi);
    return Data_Wrap_Struct(s, 0, rpmfi_free, fi);
}

/* --- Class initialization */

void
Init_rpmfi(void)
{
    rpmfiClass = rb_define_class("Fi", rb_cObject);
if (_debug)
fprintf(stderr, "==> %s() rpmfiClass 0x%lx\n", __FUNCTION__, rpmfiClass);
#ifdef  NOTYET
    rb_include_module(rpmfiClass, rb_mEnumerable);
#endif
    rb_define_singleton_method(rpmfiClass, "new", rpmfi_new, -1);
    initProperties(rpmfiClass);
    initMethods(rpmfiClass);
}

VALUE
rpmrb_NewFi(void *_fi)
{
    return Data_Wrap_Struct(rpmfiClass, 0, rpmfi_free, _fi);
}

