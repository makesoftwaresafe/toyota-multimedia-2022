#ifndef H_RPMMACRO_PY
#define H_RPMMACRO_PY

/** \ingroup py_c
 * \file python/rpmmacro-py.h
 */

/** \name Type: _rpm.macro */
/*@{*/

#ifdef __cplusplus
extern "C" {
#endif

/** \ingroup py_c
 */
/*@null@*/
PyObject * rpmmacro_AddMacro(PyObject * self, PyObject * args, PyObject * kwds)
	/*@globals rpmGlobalMacroContext, _Py_NoneStruct @*/
	/*@modifies rpmGlobalMacroContext, _Py_NoneStruct @*/;

/** \ingroup py_c
 */
/*@null@*/
PyObject * rpmmacro_DelMacro(PyObject * self, PyObject * args, PyObject * kwds)
	/*@globals rpmGlobalMacroContext, _Py_NoneStruct @*/
	/*@modifies rpmGlobalMacroContext, _Py_NoneStruct @*/;

/** \ingroup py_c
 */
/*@null@*/
PyObject * rpmmacro_ExpandMacro(PyObject * self, PyObject * args, PyObject * kwds)
	/*@globals rpmGlobalMacroContext, _Py_NoneStruct @*/
	/*@modifies rpmGlobalMacroContext, _Py_NoneStruct @*/;

/** \ingroup py_c
 */
/*@null@*/
PyObject * rpmmacro_GetMacros(PyObject * self, PyObject * args, PyObject * kwds)
	/*@globals rpmGlobalMacroContext, _Py_NoneStruct @*/
	/*@modifies rpmGlobalMacroContext, _Py_NoneStruct @*/;

#ifdef __cplusplus      
}
#endif

/*@}*/

#endif
