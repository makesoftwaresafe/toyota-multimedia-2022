#ifndef H_RPMDS_RB
#define H_RPMDS_RB

/**
 * \file ruby/rpmds-rb.h
 */

#include "rpm-rb.h"

extern VALUE rpmdsClass;

#ifdef __cplusplus
extern "C" {
#endif

extern void
Init_rpmds(void);

extern VALUE
rpmrb_NewDs(void *_ds);

#ifdef __cplusplus      
}
#endif

#endif	/* H_RPMDS_RB */
