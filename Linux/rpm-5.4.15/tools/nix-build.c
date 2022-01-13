#include "system.h"

#define	_RPMNIX_INTERNAL
#include <rpmnix.h>

#include "debug.h"

int
main(int argc, char *argv[])
{
    rpmnix nix = rpmnixNew(argv, RPMNIX_FLAGS_NOOUTLINK, NULL);
    int ec = rpmnixBuild(nix);
    nix = rpmnixFree(nix);
    return ec;
}
