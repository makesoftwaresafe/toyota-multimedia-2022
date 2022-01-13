
/** \ingroup rpmio
 * \file rpmio/mount.c
 */

#if defined(__linux__)
#include "system.h"
#include <sys/mount.h>
#include "rpmio.h"
#include "debug.h"

int Mount(const char *source, const char *target,
		const char *filesystemtype, unsigned long mountflags,
		const void *data)
{
    return mount(source, target, filesystemtype, mountflags, data);
}

int Umount(const char *target)
{
    return umount(target);
}

int Umount2(const char *target, int flags)
{
    return umount2(target, flags);
}
#endif
