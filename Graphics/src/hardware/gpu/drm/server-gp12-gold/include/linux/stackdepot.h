/*
 * A generic stack depot implementation
 *
 * Author: Alexander Potapenko <glider@google.com>
 * Copyright (C) 2016 Google, Inc.
 *
 * Based on code by Dmitry Chernenkov.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef _LINUX_STACKDEPOT_H
#define _LINUX_STACKDEPOT_H

typedef u32 depot_stack_handle_t;

struct stack_trace;

static inline depot_stack_handle_t depot_save_stack(struct stack_trace *trace, gfp_t flags)
{
    return 0;
}

static inline void depot_fetch_stack(depot_stack_handle_t handle, struct stack_trace *trace)
{
}

#endif

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/include/linux/stackdepot.h $ $Rev: 839737 $")
#endif
