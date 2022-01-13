
/*
  Copyright (C) 2004-2006  Dmitry V. Levin <ldv@altlinux.org>

  The setproctitle library interface.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "system.h"
#include <stdarg.h>

#ifdef HAVE_SYS_PRCTL_H
#include <sys/prctl.h>
#endif

#if !defined(HAVE_SETPROCTITLE)

static size_t title_buffer_size = 0;
static char *title_buffer = 0;
static char *title_progname;
static char *title_progname_full;

int
setproctitle(const char *fmt, ...)
{
	if (!title_buffer || !title_buffer_size)
	{
		errno = ENOMEM;
		return -1;
	}

	memset(title_buffer, '\0', title_buffer_size);

	ssize_t written;

	if (fmt)
	{
		ssize_t written2;
		va_list ap;

		written =
			snprintf(title_buffer, title_buffer_size, "%s: ",
				 title_progname);
		if (written < 0 || (size_t) written >= title_buffer_size)
			return -1;

		va_start(ap, fmt);
		written2 =
			vsnprintf(title_buffer + written,
				  title_buffer_size - written, fmt, ap);
		va_end(ap);
		if (written2 < 0
		    || (size_t) written2 >= title_buffer_size - written)
			return -1;
	} else
	{
		written =
			snprintf(title_buffer, title_buffer_size, "%s",
				 title_progname);
		if (written < 0 || (size_t) written >= title_buffer_size)
			return -1;
	}

	written = strlen(title_buffer);
	memset(title_buffer + written, '\0', title_buffer_size - written);
#if defined(__linux__)
    {	char procname[16+1];
	const char * s;
	if ((s = strchr(title_buffer, ' ')) != NULL)
	    s++;
	else
	    s = title_buffer;
	strncpy(procname, s, sizeof(procname));
	procname[sizeof(procname)-1] = '\0';
#if !defined(PR_SET_NAME)
#define	PR_SET_NAME	15
#endif
	(void)prctl(PR_SET_NAME, procname, 0UL, 0UL, 0UL);
    }
#endif

	return 0;
}

/*
 * Rename to _init/_fini for automagic ELF weak symbol library symbol override.
 * Note: __attribute__((constructor)) on _init function is
 * called without arguments.
 */

int
finiproctitle(void)
{
/* XXX limit the fiddle up to linux for now. */
#if defined(__linux__)
    if (title_buffer != NULL) {
	char ** envp;
	for (envp = environ; *envp != NULL; envp++) {
	    free(*envp);
	    *envp = NULL;
	}
	free(environ);
	environ = NULL;
	free(title_progname_full);
	title_progname_full = NULL;
    }
#endif
    return 0;
}

int
initproctitle(int argc, char *argv[], char *envp[])
{

/* XXX limit the fiddle up to linux for now. */
#if defined(__linux__)
	char   *begin_of_buffer = 0, *end_of_buffer = 0;
	int     i;

	for (i = 0; i < argc; ++i)
	{
		if (!begin_of_buffer)
			begin_of_buffer = argv[i];
		if (!end_of_buffer || end_of_buffer + 1 == argv[i])
			end_of_buffer = argv[i] + strlen(argv[i]);
	}

	for (i = 0; envp[i]; ++i)
	{
		if (!begin_of_buffer)
			begin_of_buffer = envp[i];
		if (!end_of_buffer || end_of_buffer + 1 == envp[i])
			end_of_buffer = envp[i] + strlen(envp[i]);
	}

	if (!end_of_buffer)
		return 0;

	char  **new_environ = (char **) malloc((i + 1) * sizeof(envp[0]));

	if (!new_environ)
		return 0;

	for (i = 0; envp[i]; ++i)
		if (!(new_environ[i] = strdup(envp[i])))
			goto cleanup_enomem;
	new_environ[i] = 0;

	if (program_invocation_name)
	{
		title_progname_full = strdup(program_invocation_name);

		if (!title_progname_full)
			goto cleanup_enomem;

		char   *p = strrchr(title_progname_full, '/');

		if (p)
			title_progname = p + 1;
		else
			title_progname = title_progname_full;

		program_invocation_name = title_progname_full;
		program_invocation_short_name = title_progname;
	}

	environ = new_environ;
	title_buffer = begin_of_buffer;
	title_buffer_size = end_of_buffer - begin_of_buffer;

	return 0;

      cleanup_enomem:
	for (--i; i >= 0; --i)
		free(new_environ[i]);
	free(new_environ);
#endif	/* defined(__linux__) */
	return 0;
}
#endif	/* !defined(HAVE_SETPROCTITLE) */
