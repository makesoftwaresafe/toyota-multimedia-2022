/********************************************************************************/
/*			     	TPM Debug					*/
/*			     Written by S. Berger				*/
/*		       IBM Thomas J. Watson Research Center			*/
/*        $Id: debug.c 4073 2010-04-30 14:44:14Z kgoldman $			*/
/********************************************************************************/

#include "copyright.h"

#include "system.h"

#include <tpmfunc.h>

#include "debug.h"

void print_array(const char *name, const unsigned char *data,
		 unsigned int len)
{
    unsigned int i = 0;
    printf("%s \n", name);
    while (i < len) {
	printf("0x%02X ", data[i]);
	i++;
	if ((i & 0xf) == 0)
	    printf("\n");
    }
    printf("\n");
}
