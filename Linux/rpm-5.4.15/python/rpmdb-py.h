#ifndef H_RPMDB_PY
#define H_RPMDB_PY

#include "rpmdb.h"

/** \ingroup py_c
 * \file python/rpmdb-py.h
 */

/** \name Type: _rpm.db */
/*@{*/

/** \ingroup py_c
 */
typedef struct rpmdbObject_s rpmdbObject;

/** \ingroup py_c
 */
struct rpmdbObject_s {
    PyObject_HEAD
    PyObject *md_dict;		/*!< to look like PyModuleObject */
    rpmdb db;
    int offx;
    int noffs;
    int *offsets;
} ;

/** \ingroup py_c
 */
/*@unchecked@*/
extern PyTypeObject rpmdb_Type;

#ifdef __cplusplus
extern "C" {
#endif

#ifdef  _LEGACY_BINDINGS_TOO
/** \ingroup py_c
 */
rpmdb dbFromDb(rpmdbObject * db)
	/*@*/;

/** \ingroup py_c
 */
rpmdbObject * rpmOpenDB(PyObject * self, PyObject * args, PyObject * kwds)
	/*@*/;
/** \ingroup py_c
 */
PyObject * rebuildDB (PyObject * self, PyObject * args, PyObject * kwds)
	/*@globals rpmGlobalMacroContext @*/
	/*@modifies rpmGlobalMacroContext @*/;
#endif

#ifdef __cplusplus      
}
#endif

/*@}*/

#endif
