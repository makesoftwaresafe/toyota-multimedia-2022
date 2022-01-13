#ifndef H_RPMLUAEXT
#define H_RPMLUAEXT

/** \ingroup rpmluaext
 * \file lib/rpmluaext.h
 */

#ifdef __cplusplus
extern "C" {
#endif

/** \ingroup rpmluaext
 * Add RPM _library_ based Lua extension
 * @param lua       Lua context
 * @return      none
 */
void rpmluaextActivate(rpmlua lua)
    /*@*/;

#ifdef __cplusplus
}
#endif

#endif/* H_RPMLUA */

