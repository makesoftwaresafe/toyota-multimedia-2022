/**
 * \ingroup rb_c
 * \file spec-rb.c
 *
 * Ruby bindings for spec file access
 */


#include "system.h"

#include "rpm-rb.h"
#include "spec-rb.h"
#include "rpmmc-rb.h"
#include "package-rb.h"

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <stdio.h>

#define	_RPMTS_INTERNAL
#define _RPMFI_INTERNAL
#include <rpmtag.h>
#include <rpmtypes.h>
#include <rpmio.h>
#include <rpmbuild.h>
#include <rpmspec.h>
#include <rpmmacro.h>


VALUE specClass;


/**
 * C destructor for the Spec class.
 */
static void
_spec_free(Spec spec)
{
    freeSpec(spec);
}


/**
 * Returns the wrapped Spec structure.
 */
static Spec
_spec_get_spec(VALUE self)
{
    Spec spec;
    Data_Get_Struct(self, struct Spec_s, spec);
    return spec;
}


/**
 * Returns the hiddenly associated transaction set.
 */
static rpmts
_spec_get_ts(VALUE self)
{
    rpmts ts;
    Data_Get_Struct(rb_iv_get(self, "ts"), struct rpmts_s, ts);
    return ts;
}


/**
 * A helper routine that returns a Ruby array containing all sources that
 * match a specific set of OR'ed flags.
 */
static VALUE
_spec_get_sources(VALUE self, int flags)
{
    Spec spec = _spec_get_spec(self);

    VALUE ary = rb_ary_new();

    SpecSource src;
    for(src = spec->sources; src != NULL; src = src->next)
        if(src->flags & flags)
            rb_ary_push(ary, rb_str_new2(src->fullSource));

    return ary;
}


/**
 * Returns an array of all sources defined in the spec file.
 *
 * call-seq:
 *  RPM::Spec#sources -> Array
 *
 * @return An array containing the full name of all source URIs
 */
static
VALUE spec_get_sources(VALUE self)
{
    return _spec_get_sources(self, RPMFILE_SOURCE);
}


/**
 * Returns an array of all patches defined in the spec file.
 *
 * call-seq:
 *  RPM::Spec#patches -> Array
 *
 * @return An array containing the full name of all patch URIs
 */
static VALUE
spec_get_patches(VALUE self)
{
    return _spec_get_sources(self, RPMFILE_PATCH);
}


/**
 * Returns all packages associated with the spec file.
 *
 * This returns all packages that are the result of the spec file would it be
 * built. It serves as a factory method creating RPM::Package classes.
 *
 * call-seq:
 *  RPM::Spec#packages -> Array
 *
 * @return An array of all packages
 * @see packageClass
 */
static VALUE
spec_get_packages(VALUE self)
{
    VALUE pkg_ary = rb_ary_new();
    Spec spec = _spec_get_spec(self);
    
    Package pkg;
    for(pkg = spec->packages; pkg != NULL; pkg = pkg->next)
        rb_ary_push(pkg_ary, Data_Wrap_Struct(packageClass, 0, -1, pkg));

    return pkg_ary;
}


/**
 * Returns the macro context of the spec file.
 *
 * call-seq:
 *  RPM::Spec#macros -> RPM::Mc
 *
 * @return The macro context associated with the spec file.
 */
static VALUE
spec_get_macros(VALUE self)
{
    Spec spec = _spec_get_spec(self);
    return rpmmc_wrap(spec->macros);
}


/**
 * Builds a part of the spec file.
 *
 * This method will call buildSpec() to build parts of the spec file. Its mode
 * of operation is determined by the constants in ::rpmBuildFlags_e, which are
 * mapped in the RPM Ruby module. Will throw an exception if the build fails.
 *
 * call-seq:
 *  RPM::Spec#build(flags, test = false) -> RPM::Spec
 *
 * @param flags A combination of flags that control the build.
 * @param test  If set to true, the scriptlets won't actually be run.
 * @return      The RPM::Spec instance
 * @see         buildSpec, ::rpmBuildFlags_e
 */
static VALUE
spec_build(VALUE argc, VALUE *argv, VALUE self)
{
    VALUE test_v = T_FALSE, flags_v;
    rb_scan_args(argc, argv, "11", &flags_v, &test_v);

    int test = 0;
    switch(TYPE(test_v)) {
        case T_TRUE:
            test = 1;
            break;
        case T_NIL:
        case T_FALSE:
            test = 0;
            break;
        default:
            rb_raise(rb_eTypeError,
                "Value for test must be either true or false");
            break;
    }

    Check_Type(flags_v, T_FIXNUM);
    int flags = FIX2INT(flags_v);

    rpmts ts = _spec_get_ts(self);
    Spec spec = _spec_get_spec(self);

    rpmRC error = buildSpec(ts, spec, flags, test);
    if(error) rpm_rb_raise(error, "Building spec file failed");

    return self;
}


VALUE
spec_wrap(Spec spec)
{
    return Data_Wrap_Struct(specClass, 0, &_spec_free, spec);
}


void
Init_spec(void)
{
    specClass = rb_define_class_under(rpmModule, "Spec", rb_cObject);

    rb_define_method(specClass, "sources", &spec_get_sources, 0);
    rb_define_method(specClass, "patches", &spec_get_patches, 0);
    rb_define_method(specClass, "packages", &spec_get_packages, 0);
    rb_define_method(specClass, "macros", &spec_get_macros, 0);
    rb_define_method(specClass, "build", &spec_build, -1);
}
