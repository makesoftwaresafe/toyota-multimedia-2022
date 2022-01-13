#ifndef H_RPM_RB
#define H_RPM_RB

/**
 * \file ruby/rpm-rb.h
 * \ingroup rb_c
 *
 * RPM Ruby bindings "RPM" module
 */


#include "system.h"

#include <rpmiotypes.h> 

#include <rpmtypes.h>
#include <rpmtag.h>


/**
 * The "RPM" Ruby module.
 */
extern VALUE rpmModule;


#ifdef __cplusplus
extern "C" {
#endif


/**
 * Defines the "RPM" Ruby module and makes it known to the Interpreter.
 */
void Init_rpm(void);


/**
 * Raises a Ruby exception (RPM::Error).
 *
 * @param error     The return code leading to the exception
 * @param message   A message to include in the exception.
 */
void rpm_rb_raise(rpmRC error, char *message);


#ifdef __cplusplus      
}
#endif

#endif	/* H_RPM_RB */
