#ifndef H_RPMTS_RB
#define H_RPMTS_RB

/**
 * @file ruby/rpmts-rb.h
 * @ingroup rb_c
 *
 * Ruby bindings to the RPM Transaction Set API
 */


#include "system.h"


/**
 * Consitutes the RPM::Ts class, binding to RPM's TransactionSet API.
 */
extern VALUE rpmtsClass;

#ifdef __cplusplus
extern "C" {
#endif

 
void Init_rpmts(void);


#ifdef __cplusplus      
}
#endif

#endif	/* H_RPMTS_RB */
