#ifndef H_RPMFD_PY
#define H_RPMFD_PY

/** \ingroup py_c
 * \file python/rpmfd-py.h
 */

/** \name Type: _rpm.fd */
/*@{*/

/** \ingroup py_c
 */
typedef struct rpmfdObject_s {
    PyObject_HEAD
    PyObject *md_dict;		/*!< to look like PyModuleObject */
/*@relnull@*/
    FD_t	fd;
} rpmfdObject;

/** \ingroup py_c
 */
/*@unchecked@*/
extern PyTypeObject rpmfd_Type;
#define rpmfdObject_Check(v)    ((v)->ob_type == &rpmfd_Type)

#ifdef __cplusplus
extern "C" {
#endif

/** \ingroup py_c
 */
/*@null@*/
rpmfdObject * rpmfd_Wrap(FD_t fd)
	/*@*/;

#ifdef __cplusplus      
}
#endif

/*@}*/

#endif
