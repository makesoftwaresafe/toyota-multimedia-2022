#include "system.h"

#include <rpmnix.h>

#include "debug.h"

int
main(int argc, char *argv[])
{
    rpmnix nix = rpmnixNew(argv, RPMNIX_FLAGS_INTERACTIVE, NULL);
    int ec = rpmnixInstallPackage(nix);

    nix = rpmnixFree(nix);

    return ec;
}
