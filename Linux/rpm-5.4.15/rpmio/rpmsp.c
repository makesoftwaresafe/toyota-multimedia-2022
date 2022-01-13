/** \ingroup rpmio
 * \file rpmio/rpmsp.c
 */

#include "system.h"

#if defined(WITH_SEPOL)
#ifdef __cplusplus
extern "C" {
#endif
#include <sepol/sepol.h>
#ifdef __cplusplus
}
#endif
#endif

#define	_RPMSP_INTERNAL
#include <rpmsp.h>
#include <rpmlog.h>
#include <rpmmacro.h>

#include "debug.h"

/*@unchecked@*/
int _rpmsp_debug = 0;

/*@unchecked@*/ /*@relnull@*/
rpmsp _rpmspI = NULL;

static void rpmspFini(void * _sp)
	/*@globals fileSystem @*/
	/*@modifies *_sp, fileSystem @*/
{
    rpmsp sp = (rpmsp) _sp;

#if defined(WITH_SEPOL)
    if (sp->P)
	sepol_module_package_free(sp->P);
    if (sp->C)
	sepol_context_free(sp->C);
    if (sp->F)
	sepol_policy_file_free(sp->F);
    if (sp->DB)
	sepol_policydb_free(sp->DB);
    if (sp->I)
        sepol_handle_destroy(sp->I);
#endif
    sp->fn = _free(sp->fn);
    sp->flags = 0;
    sp->I = NULL;
    sp->DB = NULL;
    sp->F = NULL;
    sp->C = NULL;
    sp->P = NULL;
}

/*@unchecked@*/ /*@only@*/ /*@null@*/
rpmioPool _rpmspPool = NULL;

static rpmsp rpmspGetPool(/*@null@*/ rpmioPool pool)
	/*@globals _rpmspPool, fileSystem @*/
	/*@modifies pool, _rpmspPool, fileSystem @*/
{
    rpmsp sp;

    if (_rpmspPool == NULL) {
	_rpmspPool = rpmioNewPool("sp", sizeof(*sp), -1, _rpmsp_debug,
			NULL, NULL, rpmspFini);
	pool = _rpmspPool;
    }
    return (rpmsp) rpmioGetPool(pool, sizeof(*sp));
}

rpmsp rpmspNew(const char * fn, unsigned int flags)
{
    rpmsp sp = rpmspGetPool(_rpmspPool);

    sp->fn = NULL;
    sp->flags = 0;
    sp->I = NULL;
    sp->DB = NULL;
    sp->F = NULL;
    sp->C = NULL;
    sp->P = NULL;

#if defined(WITH_SEPOL)
  { sepol_handle_t *I = sp->I = sepol_handle_create();
    int xx;

    if (I == NULL) {
if (_rpmsp_debug)
fprintf(stderr, "--> %s(%s,0x%x): sepol_handle_create() failed\n", __FUNCTION__, fn, flags);
	(void)rpmspFree(sp);
	return NULL;
    }

    if ((xx = sepol_context_create(sp->I, (sepol_context_t **)&sp->C)) < 0) {
if (_rpmsp_debug)
fprintf(stderr, "--> %s: sepol_context_create: %s\n", __FUNCTION__, strerror(errno));	/* XXX errno? */
	(void)rpmspFree(sp);
	return NULL;
    }

    if ((xx = sepol_module_package_create(&sp->P)) < 0) {
if (_rpmsp_debug)
fprintf(stderr, "--> %s: sepol_module_package_create: %s\n", __FUNCTION__, strerror(errno));	/* XXX errno? */
	(void)rpmspFree(sp);
	return NULL;
    }

    if (fn != NULL) {
	FILE * fp = fopen(fn, "r");

	if (fp == NULL || ferror(fp)) {
if (_rpmsp_debug)
fprintf(stderr, "--> %s: fopen(%s)\n", __FUNCTION__, fn);
	    if (fp) xx = fclose(fp);
	    fp = NULL;
	    (void)rpmspFree(sp);
	    return NULL;
	}

	if ((xx = sepol_policy_file_create(&sp->F)) < 0) {
if (_rpmsp_debug)
fprintf(stderr, "--> %s: sepol_policy_file_create: %s\n", __FUNCTION__, strerror(errno));	/* XXX errno? */
	    if (fp) xx = fclose(fp);
	    fp = NULL;
	    (void)rpmspFree(sp);
	    return NULL;
	}
	sepol_policy_file_set_handle(sp->F, sp->I);
	sepol_policy_file_set_fp(sp->F, fp);

	if ((xx = sepol_policydb_create(&sp->DB)) < 0) {
if (_rpmsp_debug)
fprintf(stderr, "--> %s: sepol_policydb_create: %s\n", __FUNCTION__, strerror(errno));	/* XXX errno? */
	    (void)rpmspFree(sp);
	    return NULL;
	}

	if ((xx = sepol_policydb_read(sp->DB, sp->F)) < 0) {
if (_rpmsp_debug)
fprintf(stderr, "--> %s: sepol_policydb_read: %s\n", __FUNCTION__, strerror(errno));	/* XXX errno? */
	}

	(void) fclose(fp);

    }
  }
#endif

    return rpmspLink(sp);
}

#if defined(WITH_SEPOL)
/*@unchecked@*/ /*@null@*/
static const char * _rpmspI_fn = "minimum";

/*@unchecked@*/
static int _rpmspI_flags;

static rpmsp rpmspI(void)
	/*@globals _rpmspI @*/
	/*@modifies _rpmspI @*/
{
    if (_rpmspI == NULL)
	_rpmspI = rpmspNew(_rpmspI_fn, _rpmspI_flags);
    return _rpmspI;
}
#endif
