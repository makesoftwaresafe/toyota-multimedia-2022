#ifndef H_RPMMPW_PY
#define H_RPMMPW_PY

#include "rpmio_internal.h"

/** \ingroup py_c
 * \file python/rpmmpw-py.h
 */

/** \name Type: _rpm.mpw */
/*@{*/

/** \ingroup py_c
 */
typedef struct mpwObject_s {
    PyObject_HEAD
    int ob_size;
    mpw data[1];
} mpwObject;

/** \ingroup py_c
 */
/*@unchecked@*/
extern PyTypeObject mpw_Type;

#define	mpw_Check(_o)		PyObject_TypeCheck((_o), &mpw_Type)
#define mpw_CheckExact(_o)	((_o)->ob_type == &mpw_Type)

#define	MP_ROUND_B2W(_b)	MP_BITS_TO_WORDS((_b) + MP_WBITS - 1)

#define	MPW_SIZE(_a)	(size_t)((_a)->ob_size < 0 ? -(_a)->ob_size : (_a)->ob_size)
#define	MPW_DATA(_a)	((_a)->data)

#ifdef __cplusplus
extern "C" {
#endif

/** \ingroup py_c
 */
mpwObject * mpw_New(int ob_size)
	/*@*/;

/** \ingroup py_c
 */
mpwObject * mpw_FromMPW(size_t size, mpw* data, int normalize)
	/*@*/;

#ifdef __cplusplus      
}
#endif

/*@}*/

#endif
