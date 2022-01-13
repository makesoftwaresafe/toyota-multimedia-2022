/**
 * @file package-rb.c
 * @ingroup rb_c
 */


#include "system.h"
#include "rpm-rb.h"
#include "rpmds-rb.h"
#include "package-rb.h"

#define _RPMFI_INTERNAL
#define _RPMDS_INTERNAL
#include <rpmtag.h>
#include <rpmtypes.h>
#include <rpmio.h>
#include <rpmfi.h>
#include <rpmspec.h>
#include <rpmds.h>


VALUE packageClass;


/**
 * Returns the dependency set associated with the package.
 *
 * call-seq:
 *  RPM::Package#ds -> RPM::Ds
 *
 * @return The package's dependency set.
 * @see rpmdsClass
 */
static VALUE
package_get_ds(VALUE self)
{
    Package pkg;
    Data_Get_Struct(self, struct Package_s, pkg);

    /* TODO: Replace -1 with rpmdsFree */
    return Data_Wrap_Struct(rpmdsClass, 0, 0, pkg->ds);
}


void
Init_Package(void)
{
    packageClass = rb_define_class_under(rpmModule, "Package", rb_cObject);

    rb_define_method(packageClass, "ds", &package_get_ds, 0);
}
