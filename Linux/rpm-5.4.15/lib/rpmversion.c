/** \ingroup rpmversion
 * \file lib/rpmversion.c
 */

#include "system.h"
#include "rpmiotypes.h"
#include "rpmversion.h"
#include "debug.h"

/*@-shiftimplementation @*/
rpmuint32_t rpmlibVersion(void)
{
    return (rpmuint32_t)RPMLIB_VERSION;
}

rpmuint32_t rpmlibTimestamp(void)
{
    return (rpmuint32_t)RPMLIB_TIMESTAMP;
}

rpmuint32_t rpmlibVendor(void)
{
    return (rpmuint32_t)RPMLIB_VENDOR;
}
/*@=shiftimplementation @*/

