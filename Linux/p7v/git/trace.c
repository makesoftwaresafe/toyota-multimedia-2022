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

#include "internal.h"

#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

/* Instantiate __TRACE_CLASS_<x> variables. */
#define  TRACE_CLASS(x) int TRACE_CONCAT2(__TRACE_CLASS_,x) = 0;
#include TRACE_TRC_FILE
#undef   TRACE_CLASS

extern int
trace_vsnprintf(char *str, size_t size, const char *format, va_list args);

/* Build a string array of environment variable names. */

struct trace_class {
   const char *name;
   int *p_level;
};

#define  TRACE_CLASS(x) { TRACE_ENV_PREFIX #x, & TRACE_CONCAT2(__TRACE_CLASS_,x) },
static const struct trace_class trace_classes[] = {
#include TRACE_TRC_FILE
#undef   TRACE_CLASS
   { 0, 0 }
};

static pthread_mutex_t trace_lock = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;
static char trace_buf [256];

void
trace_init () {
   const struct trace_class *p_trace_class;
   for (p_trace_class = trace_classes; p_trace_class->name != 0; p_trace_class ++) {
      const char *setting = getenv (p_trace_class->name);
      int *p_level = p_trace_class->p_level;
      if (setting != 0) {
         long value = strtol (setting, 0, 0);
         *p_level = (int) (value);
      }
   }
}

void
trace_start (const char *file, int line, const char *func) {
   pthread_mutex_lock (&trace_lock);
   trace ("# %s (%s:%d) [thread %p]\n# ", func, file, line, pthread_self ());
}

void
trace_end () {
   fputc ('\n', stderr);
   pthread_mutex_unlock (&trace_lock);
}

void
trace (const char* fmt, ...) {
   va_list args;

   va_start (args, fmt);
   trace_vsnprintf (trace_buf, sizeof (trace_buf), fmt, args);
   trace_buf [sizeof (trace_buf) - 1] = '\0';
   va_end (args);

   fputs (trace_buf, stderr);
}

