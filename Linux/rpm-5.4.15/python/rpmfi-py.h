#ifndef H_RPMFI_PY
#define H_RPMFI_PY

#include "rpmfi.h"

/** \ingroup py_c
 * \file python/rpmfi-py.h
 */

/** \name Type: _rpm.fi */
/*@{*/

/** \ingroup py_c
 */
typedef struct rpmfiObject_s {
    PyObject_HEAD
    PyObject *md_dict;		/*!< to look like PyModuleObject */
    int active;
/*@null@*/
    rpmfi fi;
} rpmfiObject;

/** \ingroup py_c
 */
/*@unchecked@*/
extern PyTypeObject rpmfi_Type;
#define rpmfiObject_Check(v)    ((v)->ob_type == &rpmfi_Type)

#ifdef __cplusplus
extern "C" {
#endif

/** \ingroup py_c
 */
/*@null@*/
rpmfi fiFromFi(rpmfiObject * fi)
	/*@*/;

/** \ingroup py_c
 */
/*@null@*/
rpmfiObject * rpmfi_Wrap(rpmfi fi)
	/*@*/;

/** \ingroup py_c
 */
/*@null@*/
rpmfiObject * hdr_fiFromHeader(PyObject * s, PyObject * args, PyObject * kwds)
	/*@globals rpmGlobalMacroContext @*/
	/*@modifies rpmGlobalMacroContext @*/;

#ifdef __cplusplus      
}
#endif

/*@}*/

#endif
