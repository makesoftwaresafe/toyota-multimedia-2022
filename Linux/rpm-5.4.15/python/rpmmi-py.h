#ifndef H_RPMMI_PY
#define H_RPMMI_PY

/** \ingroup py_c
 * \file python/rpmmi-py.h
 */

/** \name Type: _rpm.mi */
/*@{*/

/** \ingroup py_c
 */
typedef struct rpmmiObject_s rpmmiObject;

/** \ingroup py_c
 */
struct rpmmiObject_s {
    PyObject_HEAD
    PyObject *md_dict;		/*!< to look like PyModuleObject */
    rpmmi mi;
} ;

/** \ingroup py_c
 */
/*@unchecked@*/
extern PyTypeObject rpmmi_Type;
#define rpmmiObject_Check(v)    ((v)->ob_type == &rpmmi_Type)

#ifdef __cplusplus
extern "C" {
#endif

/** \ingroup py_c
 */
/*@null@*/
rpmmiObject * rpmmi_Wrap(rpmmi mi)
	/*@*/;

#ifdef __cplusplus      
}
#endif

/*@}*/

#endif
