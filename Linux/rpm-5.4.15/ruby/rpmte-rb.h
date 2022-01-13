#ifndef H_RPMTE_RB
#define H_RPMTE_RB

/**
 * @file ruby/rpmte-rb.h
 * @ingroup rb_c
 */


#include "system.h"


extern VALUE rpmteClass;


#ifdef __cplusplus
extern "C" {
#endif


void
Init_rpmte(void);


#ifdef __cplusplus      
}
#endif

#endif	/* H_RPMTE_RB */
