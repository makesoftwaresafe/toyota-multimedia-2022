#ifndef H_RPMDS_PY
#define H_RPMDS_PY

#include "rpmds.h"

/** \ingroup py_c
 * \file python/rpmds-py.h
 */

/** \name Type: _rpm.ds */
/*@{*/

/** \ingroup py_c
 */
typedef struct rpmdsObject_s {
    PyObject_HEAD
    PyObject *md_dict;		/*!< to look like PyModuleObject */
    int		active;
/*@null@*/
    rpmds	ds;
} rpmdsObject;

/** \ingroup py_c
 */
/*@unchecked@*/
extern PyTypeObject rpmds_Type;
#define rpmdsObject_Check(v)    ((v)->ob_type == &rpmds_Type)

#ifdef __cplusplus
extern "C" {
#endif

/** \ingroup py_c
 */
/*@null@*/
rpmds dsFromDs(rpmdsObject * ds)
	/*@*/;

/** \ingroup py_c
 */
/*@null@*/
rpmdsObject * rpmds_Wrap(rpmds ds)
	/*@*/;

/** \ingroup py_c
 */
/*@null@*/
rpmdsObject * rpmds_Single(PyObject * s, PyObject * args, PyObject * kwds)
	/*@*/;

/** \ingroup py_c
 */
/*@null@*/
rpmdsObject * hdr_dsFromHeader(PyObject * s, PyObject * args, PyObject * kwds)
	/*@*/;

/** \ingroup py_c
 */
/*@null@*/
rpmdsObject * hdr_dsOfHeader(PyObject * s)
	/*@*/;

#ifdef __cplusplus      
}
#endif

/*@}*/

#endif
