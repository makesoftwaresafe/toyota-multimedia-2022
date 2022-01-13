#ifndef H_RPMPS_RB
#define H_RPMPS_RB

/**
 * @file ruby/rpmps-rb.h
 * @ingroup rb_c
 */


#include "rpm-rb.h"


extern VALUE rpmpsClass;


#ifdef __cplusplus
extern "C" {
#endif


void
Init_rpmps(void);


#ifdef __cplusplus      
}
#endif

#endif	/* H_RPMPS_RB */
