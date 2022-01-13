/** \ingroup rb_c
 * \file ruby/rpmhdr-rb.c
 */

#include "system.h"

#include "rpm-rb.h"
#include "rpmds-rb.h"
#include "rpmfi-rb.h"
#include "rpmhdr-rb.h"

#ifdef	NOTYET
#include <argv.h>
#include <mire.h>
#endif

#define _RPMDS_INTERNAL
#include <rpmtag.h>
#include <rpmtypes.h>
#include <rpmds.h>
#include <rpmfi.h>
#include <rpmio.h>

#include <rpmcli.h>	/* XXX rpmHeaderFormats */

#include "../debug.h"

VALUE rpmhdrClass;

/*@unchecked@*/
static int _debug = 0;

/* --- helpers */
static void *
rpmhdr_ptr(VALUE s)
{
    void *ptr;
    Data_Get_Struct(s, void, ptr);
    return ptr;
}

static VALUE
rpmhdrLoadTag(Header h, const char * name)
{
    HE_t he = memset(alloca(sizeof(*he)), 0, sizeof(*he));
    VALUE v = Qfalse;
    int i;

if (_debug)
fprintf(stderr, "==> %s(%p,%s)\n", __FUNCTION__, h, name);

    he->tag = tagValue(name);
    if (headerGet(h, he, 0)) {
if (_debug < 0)
fprintf(stderr, "\t%s(%u) %u %p[%u]\n", name, (unsigned)he->tag, (unsigned)he->t, he->p.ptr, (unsigned)he->c);
	switch (he->t) {
	default:
	    goto exit;
	    /*@notreached@*/ break;
	case RPM_BIN_TYPE:	/* XXX return as array of octets for now. */
	case RPM_UINT8_TYPE:
	    v = rb_ary_new();
	    for (i = 0; i < (int)he->c; i++)
		rb_ary_push(v, INT2FIX(he->p.ui8p[i]));
	    break;
	case RPM_UINT16_TYPE:
	    v = rb_ary_new();
	    for (i = 0; i < (int)he->c; i++)
		rb_ary_push(v, INT2FIX(he->p.ui16p[i]));
	    break;
	case RPM_UINT32_TYPE:
	    v = rb_ary_new();
	    for (i = 0; i < (int)he->c; i++)
		rb_ary_push(v, INT2FIX(he->p.ui32p[i]));
	    break;
	case RPM_UINT64_TYPE:
	    v = rb_ary_new();
	    for (i = 0; i < (int)he->c; i++)
		rb_ary_push(v, INT2FIX(he->p.ui64p[i]));
	    break;
	case RPM_STRING_ARRAY_TYPE:
	    v = rb_ary_new();
	    for (i = 0; i < (int)he->c; i++)
		rb_ary_push(v, rb_str_new2(he->p.argv[i]));
	    break;
	case RPM_I18NSTRING_TYPE:	/* XXX FIXME: is this ever seen? */
	case RPM_STRING_TYPE:
	    v = rb_str_new2(he->p.str);
	    break;
	}
    }

exit:
    he->p.ptr = _free(he->p.ptr);
    return v;
}
/* --- Object methods */
static VALUE
rpmhdr_sprintf(VALUE s, VALUE v)
{
    Header h = rpmhdr_ptr(s);
    const char *qfmt = StringValueCStr(v);
    const char *q;
    const char *errstr = NULL;

if (_debug)
fprintf(stderr, "==> %s(0x%lx, 0x%lx) h %p\n", __FUNCTION__, s, v, h);

    if ((q = headerSprintf(h, qfmt, NULL, rpmHeaderFormats, &errstr)) == NULL)
	q = errstr;

    return rb_str_new2(q);
}

static VALUE
rpmhdr_getorigin(VALUE s)
{
    Header h = rpmhdr_ptr(s);

if (_debug)
fprintf(stderr, "==> %s(0x%lx) h %p\n", __FUNCTION__, s, h);

    return rb_str_new2(headerGetOrigin(h));
}

static VALUE
rpmhdr_setorigin(VALUE s, VALUE v)
{
    Header h = rpmhdr_ptr(s);

if (_debug)
fprintf(stderr, "==> %s(0x%lx, 0x%lx) h %p\n", __FUNCTION__, s, v, h);

    (void) headerSetOrigin(h, StringValueCStr(v));
    return rb_str_new2(headerGetOrigin(h));
}

static VALUE
rpmhdr_ds(int argc, VALUE *argv, VALUE s)
{
    VALUE v_tag;
    Header h = rpmhdr_ptr(s);
    rpmTag tag = RPMTAG_PROVIDENAME;
    int flags = 0;

    rb_scan_args(argc, argv, "01", &v_tag);

    if (!NIL_P(v_tag))
	tag = FIX2INT(v_tag);

if (_debug)
fprintf(stderr, "==> %s(0x%lx) h %p\n", __FUNCTION__, s, h);

    return rpmrb_NewDs( rpmdsNew(h, tag, flags) );
}

static VALUE
rpmhdr_fi(int argc, VALUE *argv, VALUE s)
{
    VALUE v_tag;
    Header h = rpmhdr_ptr(s);
    rpmTag tag = RPMTAG_BASENAMES;
    int flags = 0;

    rb_scan_args(argc, argv, "01", &v_tag);

    if (!NIL_P(v_tag))
	tag = FIX2INT(v_tag);

if (_debug)
fprintf(stderr, "==> %s(0x%lx) h %p\n", __FUNCTION__, s, h);

    return rpmrb_NewFi( rpmfiNew(NULL, h, tag, flags) );
}

static void
initMethods(VALUE klass)
{
    rb_define_method(klass, "sprintf",	 rpmhdr_sprintf, 1);
    rb_define_method(klass, "getorigin", rpmhdr_getorigin, 0);
    rb_define_method(klass, "setorigin", rpmhdr_setorigin, 1);
    rb_define_method(klass, "ds",	 rpmhdr_ds, -1);
    rb_define_method(klass, "fi",	 rpmhdr_fi, -1);
}

/* --- Object properties */
static VALUE
rpmhdr_debug_get(VALUE s)
{
if (_debug)
fprintf(stderr, "==> %s(0x%lx)\n", __FUNCTION__, s);
    return INT2FIX(_debug);
}

static VALUE
rpmhdr_debug_set(VALUE s, VALUE v)
{
if (_debug)
fprintf(stderr, "==> %s(0x%lx, 0x%lx)\n", __FUNCTION__, s, v);
    return INT2FIX(_debug = FIX2INT(v));
}

static VALUE
rpmhdr__get(VALUE s, VALUE v)
{
    Header h = rpmhdr_ptr(s);
    char * vstr = StringValueCStr(v);
    
if (_debug)
fprintf(stderr, "==> %s(0x%lx) %s\n", __FUNCTION__, s, vstr);

    return rpmhdrLoadTag(h, vstr);
}

static void
initProperties(VALUE klass)
{
    rb_define_method(klass, "debug", rpmhdr_debug_get, 0);
    rb_define_method(klass, "debug=", rpmhdr_debug_set, 1);
    rb_define_method(klass, "[]", rpmhdr__get, 1);
}

/* --- Object ctors/dtors */
static void
rpmhdr_free(Header h)
{
if (_debug)
fprintf(stderr, "==> %s(%p)\n", __FUNCTION__, h);

    h = headerFree(h);
}

static VALUE
rpmhdr_new(int argc, VALUE *argv, VALUE s)
{
    Header h;

    rb_scan_args(argc, argv, "00");

    h = headerNew();

if (_debug)
fprintf(stderr, "==> %s(%p[%d], 0x%lx) mi %p\n", __FUNCTION__, argv, argc, s, h);
    return Data_Wrap_Struct(s, 0, rpmhdr_free, h);
}

/* --- Class initialization */
void
Init_rpmhdr(void)
{
    rpmhdrClass = rb_define_class("Hdr", rb_cObject);
if (_debug)
fprintf(stderr, "==> %s() rpmhdrClass 0x%lx\n", __FUNCTION__, rpmhdrClass);
#ifdef	NOTYET
    rb_include_module(rpmhdrClass, rb_mEnumerable);
#endif
    rb_define_singleton_method(rpmhdrClass, "new", rpmhdr_new, -1);
    initProperties(rpmhdrClass);
    initMethods(rpmhdrClass);
}

VALUE
rpmrb_NewHdr(void *_h)
{
    return Data_Wrap_Struct(rpmhdrClass, 0, rpmhdr_free, _h);
}
