/* Function return value location for ARM EABI.
   Copyright (C) 2009-2010 Red Hat, Inc.
   This file is part of Red Hat elfutils.

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

#include <assert.h>
#include <dwarf.h>

#define BACKEND arm_
#include "libebl_CPU.h"


/* r0, or pair r0, r1, or aggregate up to r0-r3.  */
static const Dwarf_Op loc_intreg[] =
  {
    { .atom = DW_OP_reg0 }, { .atom = DW_OP_piece, .number = 4 },
    { .atom = DW_OP_reg1 }, { .atom = DW_OP_piece, .number = 4 },
    { .atom = DW_OP_reg2 }, { .atom = DW_OP_piece, .number = 4 },
    { .atom = DW_OP_reg3 }, { .atom = DW_OP_piece, .number = 4 },
  };
#define nloc_intreg	1
#define nloc_intregs(n)	(2 * (n))

/* f1  */ /* XXX TODO: f0 can also have number 96 if program was compiled with -mabi=aapcs */
static const Dwarf_Op loc_fpreg[] =
  {
    { .atom = DW_OP_reg16 },
  };
#define nloc_fpreg  1

/* The return value is a structure and is actually stored in stack space
   passed in a hidden argument by the caller.  But, the compiler
   helpfully returns the address of that space in r0.  */
static const Dwarf_Op loc_aggregate[] =
  {
    { .atom = DW_OP_breg0, .number = 0 }
  };
#define nloc_aggregate 1


static int
arm_return_value_location_ (Dwarf_Die *functypedie, const Dwarf_Op **locp,
		            int soft_float)
{
  /* Start with the function's type, and get the DW_AT_type attribute,
     which is the type of the return value.  */

  Dwarf_Attribute attr_mem;
  Dwarf_Attribute *attr = dwarf_attr_integrate (functypedie, DW_AT_type,
						&attr_mem);
  if (attr == NULL)
    /* The function has no return value, like a `void' function in C.  */
    return 0;

  Dwarf_Die die_mem;
  Dwarf_Die *typedie = dwarf_formref_die (attr, &die_mem);
  int tag = dwarf_tag (typedie);

  /* Follow typedefs and qualifiers to get to the actual type.  */
  while (tag == DW_TAG_typedef
	 || tag == DW_TAG_const_type || tag == DW_TAG_volatile_type
	 || tag == DW_TAG_restrict_type || tag == DW_TAG_mutable_type)
    {
      attr = dwarf_attr_integrate (typedie, DW_AT_type, &attr_mem);
      typedie = dwarf_formref_die (attr, &die_mem);
      tag = dwarf_tag (typedie);
    }

  Dwarf_Word size;
  switch (tag)
    {
    case -1:
      return -1;

    case DW_TAG_subrange_type:
      if (! dwarf_hasattr_integrate (typedie, DW_AT_byte_size))
	{
	  attr = dwarf_attr_integrate (typedie, DW_AT_type, &attr_mem);
	  typedie = dwarf_formref_die (attr, &die_mem);
	  tag = dwarf_tag (typedie);
	}
      /* Fall through.  */

    case DW_TAG_base_type:
    case DW_TAG_enumeration_type:
    case DW_TAG_pointer_type:
    case DW_TAG_ptr_to_member_type:
      if (dwarf_formudata (dwarf_attr_integrate (typedie, DW_AT_byte_size,
						 &attr_mem), &size) != 0)
	{
	  if (tag == DW_TAG_pointer_type || tag == DW_TAG_ptr_to_member_type)
	    size = 4;
	  else
	    return -1;
	}
      if (tag == DW_TAG_base_type)
	{
	  Dwarf_Word encoding;
	  if (dwarf_formudata (dwarf_attr_integrate (typedie, DW_AT_encoding,
	                       &attr_mem), &encoding) != 0)
	    return -1;

	  if ((encoding == DW_ATE_float) && !soft_float)
	    {
	      *locp = loc_fpreg;
	      if (size <= 8)
		return nloc_fpreg;
	      goto aggregate;
	    }
	}
      if (size <= 16)
	{
	intreg:
	  *locp = loc_intreg;
	  return size <= 4 ? nloc_intreg : nloc_intregs ((size + 3) / 4);
	}
      /* fall through. */

    aggregate:
      /* XXX TODO sometimes aggregates are returned in r0 (-mabi=aapcs) */
      *locp = loc_aggregate;
      return nloc_aggregate;

    case DW_TAG_structure_type:
    case DW_TAG_class_type:
    case DW_TAG_union_type:
    case DW_TAG_array_type:
      if (dwarf_aggregate_size (typedie, &size) == 0
	  && size > 0 && size <= 4)
	goto intreg;
      goto aggregate;
    }

  /* XXX We don't have a good way to return specific errors from ebl calls.
     This value means we do not understand the type, but it is well-formed
     DWARF and might be valid.  */
  return -2;
}

/* return location for -mabi=apcs-gnu -msoft-float */
int
arm_return_value_location_soft (Dwarf_Die *functypedie, const Dwarf_Op **locp)
{
   return arm_return_value_location_ (functypedie, locp, 1);
}

/* return location for -mabi=apcs-gnu -mhard-float (current default) */
int
arm_return_value_location_hard (Dwarf_Die *functypedie, const Dwarf_Op **locp)
{
   return arm_return_value_location_ (functypedie, locp, 0);
}

