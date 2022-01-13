#ifndef H_RPMTS_PY
#define H_RPMTS_PY

#include "rpmts.h"

/** \ingroup py_c
 * \file python/rpmts-py.h
 */

/** \name Type: _rpm.ts */
/*@{*/

/** \ingroup py_c
 */
typedef struct rpmtsObject_s {
    PyObject_HEAD
    PyObject *md_dict;		/*!< to look like PyModuleObject */
    rpmts	ts;
    PyObject * keyList;		/* keeps reference counts correct */
    FD_t scriptFd;
/*@relnull@*/
    rpmtsi tsi;
    rpmElementType tsiFilter;
    rpmprobFilterFlags ignoreSet;
} rpmtsObject;

/** \ingroup py_c
 */
/*@unchecked@*/
extern PyTypeObject rpmts_Type;
#define rpmtsObject_Check(v)    ((v)->ob_type == &rpmts_Type)

/* XXX These names/constants have been removed from the rpmlib API. */
enum {
   RPMDEP_SENSE_REQUIRES,		/*!< requirement not satisfied. */
   RPMDEP_SENSE_CONFLICTS		/*!< conflict was found. */
};

#ifdef __cplusplus
extern "C" {
#endif

/** \ingroup py_c
 */
PyObject * rpmts_Create(PyObject * s, PyObject * args, PyObject * kwds)
	/*@globals rpmGlobalMacroContext @*/
	/*@modifies rpmGlobalMacroContext @*/;

#ifdef __cplusplus      
}
#endif

/*@}*/

#endif
