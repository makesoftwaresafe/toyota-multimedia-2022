#ifndef RPMPYTHON_HEADER
#define RPMPYTHON_HEADER

/** \ingroup py_c
 * \file python/header-py.h
 */

/** \name Type: _rpm.hdr */
/*@{*/

/** \ingroup py_c
 */
typedef struct hdrObject_s hdrObject;

/** \ingroup py_c
 */
/*@unchecked@*/
extern PyTypeObject hdr_Type;
#define hdrObject_Check(v)      ((v)->ob_type == &hdr_Type)

/** \ingroup py_c
 */
extern PyObject * pyrpmError;

#ifdef __cplusplus
extern "C" {
#endif

/** \ingroup py_c
 */
hdrObject * hdr_Wrap(Header h)
	/*@*/;

/** \ingroup py_c
 */
Header hdrGetHeader(hdrObject * h)
	/*@*/;

/** \ingroup py_c
 */
rpmTag tagNumFromPyObject (PyObject *item)
	/*@*/;

/** \ingroup py_c
 */
PyObject * evrSplit (PyObject * self, PyObject * args, PyObject * kwds)
	/*@*/;

/** \ingroup py_c
 */
PyObject * evrCompare (PyObject * self, PyObject * args, PyObject * kwds)
	/*@*/;

/** \ingroup py_c
 */
PyObject * labelCompare (PyObject * self, PyObject * args)
	/*@*/;

/** \ingroup py_c
 */
PyObject * versionCompare (PyObject * self, PyObject * args, PyObject * kwds)
	/*@*/;

/** \ingroup py_c
 */
PyObject * rpmHeaderFromFile(PyObject * self, PyObject * args, PyObject * kwds)
	/*@*/;

/** \ingroup py_c
 */
PyObject * rpmHeaderFromFD(PyObject * self, PyObject * args, PyObject * kwds)
	/*@*/;

/** \ingroup py_c
 */
PyObject * rpmSingleHeaderFromFD(PyObject * self, PyObject * args, PyObject * kwds)
	/*@*/;

/** \ingroup py_c
 */
PyObject * rpmHeaderToFile(PyObject * self, PyObject * args, PyObject * kwds)
	/*@*/;

/** \ingroup py_c
 */
PyObject * rpmHeaderToFD(PyObject * self, PyObject * args, PyObject * kwds)
	/*@*/;

/** \ingroup py_c
 */
PyObject * rpmReadHeaders (FD_t fd)
	/*@*/;

/** \ingroup py_c
 */
PyObject * rpmWriteHeaders (PyObject * list, FD_t fd)
	/*@*/;

/** \ingroup py_c
 */
PyObject * rhnLoad(PyObject * self, PyObject * args, PyObject * kwds)
	/*@*/;

/** \ingroup py_c
 */
PyObject * hdrLoad(PyObject * self, PyObject * args, PyObject * kwds)
	/*@*/;

#ifdef __cplusplus      
}
#endif

/*@}*/

#endif
