#ifndef H_RPMTD_PY
#define H_RPMTD_PY

#include "rpmtd.h"

/** \ingroup py_c
 * \file python/rpmtd-py.h
 */

/** \ingroup py_c
 */
typedef struct rpmtdObject_s rpmtdObject;

/** \ingroup py_c
 */
struct rpmtdObject_s {
    PyObject_HEAD
    PyObject *md_dict;		/*!< to look like PyModuleObject */
    rpmtd td;
};

/*@unchecked@*/
extern PyTypeObject rpmtd_Type;
#define rpmtdObject_Check(v)    ((v)->ob_type == &rpmtd_Type)

/**
 */
PyObject * rpmtd_ItemAsPyobj(rpmtd td)
	/*@*/;

/**
 */
PyObject * rpmtd_AsPyobj(rpmtd td)
	/*@*/;

/**
 */
rpmtdObject * rpmtd_Wrap(rpmtd td)
	/*@*/;

#endif	/* H_RPMTD_PY */
