#include "system.h"

#include <rpmnix.h>

#include "debug.h"

int
main(int argc, char *argv[])
{
    rpmnix nix = rpmnixNew(argv, 0, NULL);
    int ec = rpmnixPull(nix);

    nix = rpmnixFree(nix);

    return ec;
}
