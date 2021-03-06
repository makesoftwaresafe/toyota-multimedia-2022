/* Initialization of Arm specific backend library.
   Copyright (C) 2002, 2005, 2009 Red Hat, Inc.
   This file is part of Red Hat elfutils.
   Written by Ulrich Drepper <drepper@redhat.com>, 2002.

   Red Hat elfutils is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by the
   Free Software Foundation; version 2 of the License.

   Red Hat elfutils is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with Red Hat elfutils; if not, write to the Free Software Foundation,
   Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301 USA.

   Red Hat elfutils is an included package of the Open Invention Network.
   An included package of the Open Invention Network is a package for which
   Open Invention Network licensees cross-license their patents.  No patent
   license is granted, either expressly or impliedly, by designation as an
   included package.  Should you wish to participate in the Open Invention
   Network licensing program, please visit www.openinventionnetwork.com
   <http://www.openinventionnetwork.com>.  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#define BACKEND		arm_
#define RELOC_PREFIX	R_ARM_
#include "libebl_CPU.h"

#include "libebl_arm.h"

/* This defines the common reloc hooks based on arm_reloc.def.  */
#include "common-reloc.c"


const char *
arm_init (elf, machine, eh, ehlen)
     Elf *elf;
     GElf_Half machine __attribute__ ((unused));
     Ebl *eh;
     size_t ehlen;
{
  int soft_float = 0;

  /* Check whether the Elf_BH object has a sufficent size.  */
  if (ehlen < sizeof (Ebl))
    return NULL;

  if (elf) {
    GElf_Ehdr ehdr_mem;
    GElf_Ehdr *ehdr = gelf_getehdr (elf, &ehdr_mem);
    if (ehdr && (ehdr->e_flags & EF_ARM_SOFT_FLOAT))
      soft_float = 1;
  }

  /* We handle it.  */
  eh->name = "ARM";
  arm_init_reloc (eh);
  HOOK (eh, segment_type_name);
  HOOK (eh, section_type_name);
  HOOK (eh, machine_flag_check);
  HOOK (eh, reloc_simple_type);
  HOOK (eh, register_info);
  HOOK (eh, core_note);
  HOOK (eh, auxv_info);
  HOOK (eh, check_object_attribute);
  if (soft_float)
    eh->return_value_location = arm_return_value_location_soft;
  else
    eh->return_value_location = arm_return_value_location_hard;

  return MODVERSION;
}
