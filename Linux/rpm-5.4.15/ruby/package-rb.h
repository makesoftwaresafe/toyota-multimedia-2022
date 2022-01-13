/**
 * @file package-rb.h
 * @ingroup rb_c
 *
 * Ruby access to RPM's Package struct.
 *
 * @see ::Package_s
 */


#ifndef _H_PACKAGE_RB_
#define _H_PACKAGE_RB_


#include "rpm-rb.h"


/**
 * RPM::Package class that represents a package during build.
 *
 * The general way of acquiring a RPM::Package class is to get it from the
 * spec file after it has been parsed, e.g.
 *
 *  ts = RPM::Ts.new
 *  spec = ts.parse_spec 'foo.spec'
 *  spec.packages.each do |pkg|
 *      # ...
 *  end
 *
 * @see ::Package_s
 */
extern VALUE packageClass;


#ifdef __cplusplus
extern "C" {
#endif


/**
 * Creates a new RPM::Package instance. Called alone it does not make much
 * sense; use it together with the factory RPM::Spec#packages.
 *
 * @see spec_get_packages()
 */
void
Init_Package(void);


#ifdef __cplusplus
}
#endif


#endif /* #ifndef _H_PACKAGE_RB_ */
