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

#ifndef MEMTRAQ_TRACE_H
#define MEMTRAQ_TRACE_H

#include <pthread.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef TRACE_CLASS_DEFAULT
#error "TRACE_CLASS_DEFAULT shall be defined!"
#endif

#ifndef TRACE_TRC_FILE
#error "TRACE_TRC_FILE shall be defined!"
#endif

#define _TRACE_CONCAT2(a,b) a ## b
#define TRACE_CONCAT2(a,b) _TRACE_CONCAT2(a,b)

/* Declare trace classes. */
#define  TRACE_CLASS(x) extern int __TRACE_CLASS_ ## x;
#include TRACE_TRC_FILE
#undef   TRACE_CLASS

extern void trace_init ();
extern void trace_start (const char *file, int line, const char *func);
extern void trace (const char *fmt, ...);
extern void trace_end ();

#define TRACE1(x) do {							\
   if (TRACE_CONCAT2 (__TRACE_CLASS_,TRACE_CLASS_DEFAULT) >= 1) {	\
      trace_start (__FILE__, __LINE__, __PRETTY_FUNCTION__);		\
      trace x;								\
      trace_end ();							\
   }									\
} while (0)

#define TRACE2(x) do {							\
   if (TRACE_CONCAT2 (__TRACE_CLASS_,TRACE_CLASS_DEFAULT) >= 2) {	\
      trace_start (__FILE__, __LINE__, __PRETTY_FUNCTION__);		\
      trace x;								\
      trace_end ();							\
   }									\
} while (0)

#define TRACE3(x) do {							\
   if (TRACE_CONCAT2 (__TRACE_CLASS_,TRACE_CLASS_DEFAULT) >= 3) {	\
      trace_start (__FILE__, __LINE__, __PRETTY_FUNCTION__);		\
      trace x;								\
      trace_end ();							\
   }									\
} while (0)

#define TRACE4(x) do {							\
   if (TRACE_CONCAT2 (__TRACE_CLASS_,TRACE_CLASS_DEFAULT) >= 4) {	\
      trace_start (__FILE__, __LINE__, __PRETTY_FUNCTION__);		\
      trace x;								\
      trace_end ();							\
   }									\
} while (0)

#ifdef __cplusplus
}
#endif

#endif /* MEMTRAQ_TRACE_H */

