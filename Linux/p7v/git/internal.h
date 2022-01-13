/*
 * P7V - Verify PKCS#7 Signed Packages
 * Copyright (C) 2012 Cedric Hombourger <chombourger@gmail.com>
 * License: GNU GPL (GNU General Public License, see COPYING-GPL)
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef P7V_INTERNAL_H
#define P7V_INTERNAL_H

#define _GNU_SOURCE 1

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifndef  TRACE_CLASS_DEFAULT
#define  TRACE_CLASS_DEFAULT APPLICATION
#endif

#define  TRACE_ENV_PREFIX    "P7V_TRACE_"
#define  TRACE_TRC_FILE      "p7v.trc"
#include "trace.h"

#endif /* P7V_INTERNAL_H */

