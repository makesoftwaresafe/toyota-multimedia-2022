#ifndef H_RPMHDR_RB
#define H_RPMHDR_RB

/**
 * \file ruby/rpmhdr-rb.h
 */

#include "rpm-rb.h"

extern VALUE rpmhdrClass;

#ifdef __cplusplus
extern "C" {
#endif

extern void
Init_rpmhdr(void);

extern VALUE
rpmrb_NewHdr(void *_h);

#ifdef __cplusplus      
}
#endif

#endif	/* H_RPMHDR_RB */
