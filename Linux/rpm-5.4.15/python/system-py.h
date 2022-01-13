/** \ingroup py_c
 * \file python/system.h
 */

#ifndef H_SYSTEM_PYTHON
#define	H_SYSTEM_PYTHON

#if defined(__APPLE__)
#include <sys/types.h>
#endif

#define	uuid_t	unistd_uuid_t	/* XXX Mac OS X dares to be different. */
#include <Python.h>
#undef	unistd_uuid_t		/* XXX Mac OS X dares to be different. */
#include <structmember.h>

#if ((PY_MAJOR_VERSION << 8) | (PY_MINOR_VERSION << 0)) < 0x0204
#define Py_RETURN_NONE return Py_INCREF(Py_None), Py_None
#define Py_RETURN_TRUE return Py_INCREF(Py_True), Py_True
#endif

#ifdef __LCLINT__
#undef  PyObject_HEAD
#define PyObject_HEAD   int _PyObjectHead;
#endif

#if ((PY_MAJOR_VERSION << 8) | (PY_MINOR_VERSION << 0)) < 0x0205
typedef int Py_ssize_t;
typedef Py_ssize_t (*lenfunc)(PyObject *);
#endif

/* Compatibility macros for Python < 2.6 */
#ifndef PyVarObject_HEAD_INIT
#define PyVarObject_HEAD_INIT(type, size) \
        PyObject_HEAD_INIT(type) size,
#endif

#ifndef Py_TYPE
#define Py_TYPE(o) ((o)->ob_type)
#endif

#if ((PY_MAJOR_VERSION << 8) | (PY_MINOR_VERSION << 0)) < 0x0206
#define PyBytes_Check PyString_Check
#define PyBytes_FromString PyString_FromString
#define PyBytes_FromStringAndSize PyString_FromStringAndSize
#define PyBytes_Size PyString_Size
#define PyBytes_AsString PyString_AsString
#endif

#if ((PY_MAJOR_VERSION << 8) | (PY_MINOR_VERSION << 0)) >= 0x0207
#define CAPSULE_BUILD(ptr,name) PyCapsule_New(ptr, name, NULL)
#define CAPSULE_CHECK(obj) PyCapsule_CheckExact(obj)
#define CAPSULE_EXTRACT(obj,name) PyCapsule_GetPointer(obj, name)
#else
#define CAPSULE_BUILD(ptr,name) PyCObject_FromVoidPtr(ptr, NULL)
#define CAPSULE_CHECK(obj) PyCObject_Check(obj)
#define CAPSULE_EXTRACT(obj,name) PyCObject_AsVoidPtr(obj)
#endif

/* For Python 3, use the PyLong type throughout in place of PyInt */
#if PY_MAJOR_VERSION >= 3
#define PyInt_Check PyLong_Check
#define PyInt_AsLong PyLong_AsLong
#define PyInt_FromLong PyLong_FromLong
#define PyInt_AsUnsignedLongMask PyLong_AsUnsignedLongMask
#define PyInt_AsUnsignedLongLongMask PyLong_AsUnsignedLongLongMask
#define PyInt_AsSsize_t PyLong_AsSsize_t
#endif

#define DEPRECATED_METHOD(_msg) \
    PyErr_WarnEx(PyExc_PendingDeprecationWarning, (_msg), 2);

#include "../system.h"

#endif	/* H_SYSTEM_PYTHON */
