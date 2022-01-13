#ifndef H_RPMBC_PY
#define H_RPMBC_PY

#include "rpmio_internal.h"

/** \ingroup py_c
 * \file python/rpmbc-py.h
 */

/** \name _rpm.bc */
/*@{*/

/** \ingroup py_c
 */
typedef struct rpmbcObject_s {
    PyObject_HEAD
    PyObject *md_dict;		/*!< to look like PyModuleObject */
} rpmbcObject;

/** \ingroup py_c
 */
/*@unchecked@*/
extern PyTypeObject rpmbc_Type;

/*@}*/

#endif
