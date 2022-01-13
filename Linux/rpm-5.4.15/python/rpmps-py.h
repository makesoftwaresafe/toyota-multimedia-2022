#ifndef H_RPMPS_PY
#define H_RPMPS_PY

#include "rpmps.h"

/** \ingroup py_c
 * \file python/rpmps-py.h
 */

/** \name Type: _rpm.ps */
/*@{*/

/** \ingroup py_c
 */
typedef struct rpmpsObject_s {
    PyObject_HEAD
    PyObject *md_dict;		/*!< to look like PyModuleObject */
/*@relnull@*/
    rpmps	ps;
/*@relnull@*/
    rpmpsi	psi;
} rpmpsObject;

/** \ingroup py_c
 */
/*@unchecked@*/
extern PyTypeObject rpmps_Type;

#ifdef __cplusplus
extern "C" {
#endif

/** \ingroup py_c
 */
/*@null@*/
rpmps psFromPs(rpmpsObject * ps)
	/*@*/;

/** \ingroup py_c
 */
/*@null@*/
rpmpsObject * rpmps_Wrap(rpmps ps)
	/*@*/;

#ifdef __cplusplus      
}
#endif

/*@}*/

#endif
