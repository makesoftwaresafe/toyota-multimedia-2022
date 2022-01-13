#ifndef H_RPMMC_RB
#define H_RPMMC_RB


/**
 * \file ruby/rpmmc-rb.h
 * \ingroup rb_c
 *
 * Ruby bindings to RPM's macro context facility.
 */


#include "rpm-rb.h"

#define	_MACRO_INTERNAL
#include <rpmmacro.h>
#include <rpmio/argv.h>


/**
 * RPM::Mc macro context Ruby class.
 */
extern VALUE rpmmcClass;


#ifdef __cplusplus
extern "C" {
#endif


void
Init_rpmmc(void);


/**
 * Wraps an already existing ::MacroContext struct in a Ruby class.
 *
 * @param mc    The allocated ::MacroContext
 * @return      The RPM::Mc Ruby class
 */
VALUE
rpmmc_wrap(MacroContext mc);


#if 0
/**
 * Frees an allocated macro context.
 *
 * This deallocation method is used in the C code only and accessible when
 * wrapping an ::rpmmc_s struct that already exists.
 *
 * @param mc    The macro context that is to be freed
 */
void
rpmmc_free(rpmmc mc);
#endif


#ifdef __cplusplus      
}
#endif

#endif	/* H_RPMMC_RB */
