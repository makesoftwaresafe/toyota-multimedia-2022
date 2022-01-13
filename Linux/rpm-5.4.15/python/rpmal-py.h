#ifndef H_RPMAL_PY
#define H_RPMAL_PY

#include "rpmal.h"

/** \ingroup py_c
 * \file python/rpmal-py.h
 */

/** \name Type: _rpm.al */
/*@{*/

/** \ingroup py_c
 */
typedef struct rpmalObject_s {
    PyObject_HEAD
    PyObject *md_dict;		/*!< to look like PyModuleObject */
    rpmal	al;
} rpmalObject;

/** \ingroup py_c
 */
/*@unchecked@*/
extern PyTypeObject rpmal_Type;

#ifdef __cplusplus
extern "C" {
#endif

/** \ingroup py_c
 */
/*@null@*/
rpmalObject * rpmal_Wrap(rpmal al)
	/*@*/;

#ifdef __cplusplus      
}
#endif

/*@}*/

#endif
