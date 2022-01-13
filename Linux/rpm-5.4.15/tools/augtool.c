/*
 * augtool.c:
 *
 * Copyright (C) 2007, 2008 Red Hat Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
 *
 * Author: David Lutterkort <dlutter@redhat.com>
 */

#include "system.h"

#if defined(WITH_READLINE)
#if defined(HAVE_READLINE_READLINE_H)
#include <readline/readline.h>
#endif
#if defined(HAVE_READLINE_HISTORY_H)
#include <readline/history.h>
#endif
#endif

#include <rpmiotypes.h>
#include <poptIO.h>
#include <argv.h>

#define	_RPMAUG_INTERNAL
#include <rpmaug.h>

#include "debug.h"

#if !defined(SEP)
#define SEP     '/'
#endif

/* Define: PATH_SEP_CHAR
 * Character separating paths in a list of paths
 */
#define PATH_SEP_CHAR ':'

static const char *const progname = "augtool";

static char *cleanstr(char *path, const char sep)
{
    if (path && *path) {
	char *e = path + strlen(path) - 1;
	while (e >= path && (*e == sep || xisspace(*e)))
	    *e-- = '\0';
    }
    return path;
}

#if defined(WITH_READLINE)
static char *ls_pattern(const char *path)
{
    char *q;
    int r;

    if (path[strlen(path)-1] == SEP)
	r = asprintf(&q, "%s*", path);
    else
	r = asprintf(&q, "%s/*", path);
    if (r == -1)
	return NULL;
    return q;
}

static int child_count(const char *path)
{
    char *q = ls_pattern(path);
    int cnt;

    if (q == NULL)
	return 0;
    cnt = rpmaugMatch(NULL, q, NULL);
    free(q);
    return cnt;
}

static char *readline_path_generator(const char *text, int state)
{
    static int current = 0;
    static char **children = NULL;
    static int nchildren = 0;

    if (state == 0) {
        char *end = strrchr(text, SEP);
        char *path;
        if (end == NULL) {
            if ((path = strdup("/*")) == NULL)
                return NULL;
        } else {
            end += 1;
	    path = xcalloc(1, end - text + 2);
            if (path == NULL)
                return NULL;
            strncpy(path, text, end - text);
            strcat(path, "*");
        }

        for (;current < nchildren; current++)
            free((void *) children[current]);
        free((void *) children);
        nchildren = rpmaugMatch(NULL, path, &children);
        current = 0;
        free(path);
    }

    while (current < nchildren) {
        char *child = children[current];
        current += 1;
        if (!strncmp(child, text, strlen(text))) {
            if (child_count(child) > 0) {
                char *c = realloc(child, strlen(child)+2);
                if (c == NULL)
                    return NULL;
                child = c;
                strcat(child, "/");
            }
            rl_filename_completion_desired = 1;
            rl_completion_append_character = '\0';
            return child;
        } else {
            free(child);
        }
    }
    return NULL;
}

static char *readline_command_generator(const char *text, int state)
{
    static int current = 0;
    const char * longName;

    if (state == 0)
        current = 0;

    rl_completion_append_character = ' ';
    while ((longName = _rpmaugCommandTable[current].longName) != NULL) {
        current += 1;
        if (!strncmp(text, longName, strlen(text)))
            return strdup(longName);
    }
    return NULL;
}

#if !defined(HAVE_RL_COMPLETION_MATCHES)
typedef char *rl_compentry_func_t(const char *, int);
static char **rl_completion_matches(/*@unused@*/ const char *text,
                           /*@unused@*/ rl_compentry_func_t *func)
{
    return NULL;
}
#endif

static char **readline_completion(const char *text, int start,
		/*@unused@*/ int end)
{
    if (start == 0)
        return rl_completion_matches(text, readline_command_generator);
    else
        return rl_completion_matches(text, readline_path_generator);

    return NULL;
}

static void readline_init(void)
{
    rl_readline_name = progname;
    rl_attempted_completion_function = readline_completion;
    rl_completion_entry_function = readline_path_generator;
}
#endif

#if !defined(HAVE_GETLINE)
#if !defined(HAVE_GETDELIM)
# define flockfile(x) ((void) 0)
# define funlockfile(x) ((void) 0)
# define getc_maybe_unlocked(fp)        getc(fp)

static ssize_t
getdelim (char **lineptr, size_t *n, int delimiter, FILE *fp)
{
  ssize_t result = -1;
  size_t cur_len = 0;

  if (lineptr == NULL || n == NULL || fp == NULL)
    {
      errno = EINVAL;
      return -1;
    }

  flockfile (fp);

  if (*lineptr == NULL || *n == 0)
    {
      char *new_lineptr;
      *n = 120;
      new_lineptr = (char *) realloc (*lineptr, *n);
      if (new_lineptr == NULL)
        {
          result = -1;
          goto unlock_return;
        }
      *lineptr = new_lineptr;
    }

  for (;;)
    {
      int i;

      i = getc_maybe_unlocked (fp);
      if (i == EOF)
        {
          result = -1;
          break;
        }

      /* Make enough space for len+1 (for final NUL) bytes.  */
      if (cur_len + 1 >= *n)
        {
          size_t needed_max =
            SSIZE_MAX < SIZE_MAX ? (size_t) SSIZE_MAX + 1 : SIZE_MAX;
          size_t needed = 2 * *n + 1;   /* Be generous. */
          char *new_lineptr;

          if (needed_max < needed)
            needed = needed_max;
          if (cur_len + 1 >= needed)
            {
              result = -1;
              errno = EOVERFLOW;
              goto unlock_return;
            }

          new_lineptr = (char *) realloc (*lineptr, needed);
          if (new_lineptr == NULL)
            {
              result = -1;
              goto unlock_return;
            }

          *lineptr = new_lineptr;
          *n = needed;
        }

      (*lineptr)[cur_len] = i;
      cur_len++;

      if (i == delimiter)
        break;
    }
  (*lineptr)[cur_len] = '\0';
  result = cur_len ? (ssize_t)cur_len : result;

 unlock_return:
  funlockfile (fp); /* doesn't set errno */

  return result;
}
#endif

static ssize_t
getline (char **lineptr, size_t *n, FILE *stream)
{
  return getdelim (lineptr, n, '\n', stream);
}
#endif

static int main_loop(void)
{
    char *line = NULL;
    size_t len = 0;
    int ret = 0;	/* assume success */

    while (1) {
	const char *buf;

#if defined(WITH_READLINE)
        if (isatty(fileno(stdin))) {
            line = readline("augtool> ");
        } else
#endif
	if (getline(&line, &len, stdin) == -1)
	    break;
        cleanstr(line, '\n');
        if (line == NULL) {
            fprintf(stdout, "\n");
	    break;
        }
        if (line[0] == '#')
            continue;

	buf = NULL;
	switch (rpmaugRun(NULL, line, &buf)) {
	case RPMRC_OK:
#if defined(WITH_READLINE)
	    if (isatty(fileno(stdin)))
		add_history(line);
#endif
	    break;
	case RPMRC_NOTFOUND:
	    goto exit;
	    /*@notreached@*/ break;
	default:
	    break;
	}
	if (buf && *buf)
	    fprintf(stdout, "%s", buf);
    }
exit:
    line = _free(line);
    return ret;
}

/*@unchecked@*/ /*@null@*/
extern const char ** _rpmaugLoadargv;

static struct poptOption _optionsTable[] = {

 { NULL, '\0', POPT_ARG_INCLUDE_TABLE, rpmaugPoptTable, 0,
        N_("Options:"), NULL },

#ifdef	NOTYET	/* XXX dueling --root options */
 { NULL, '\0', POPT_ARG_INCLUDE_TABLE, rpmioAllPoptTable, 0,
        N_("Common options for all rpmio executables:"),
        NULL },

    POPT_AUTOALIAS
#endif

    POPT_AUTOHELP
    POPT_TABLEEND
};

static struct poptOption *optionsTable = &_optionsTable[0];

int main(int argc, char **argv)
{
    poptContext optCon = rpmioInit(argc, argv, optionsTable);
    const char ** av = NULL;
    int ac;
    int r = -1;

_rpmaug_debug = -1;
    if (_rpmaugLoadargv != NULL)
	_rpmaugLoadpath = argvJoin(_rpmaugLoadargv, PATH_SEP_CHAR);

    _rpmaugI = rpmaugNew(_rpmaugRoot, _rpmaugLoadpath, _rpmaugFlags);
    if (_rpmaugI == NULL) {
        fprintf(stderr, "Failed to initialize Augeas\n");
	goto exit;
    }

#if defined(WITH_READLINE)
    readline_init();
#endif

    av = poptGetArgs(optCon);
    ac = argvCount(av);
    if (ac > 0) {	// Accept one command from the command line
	const char * cmd = argvJoin(av, ' ');
	const char *buf;

	buf = NULL;
        r = rpmaugRun(NULL, cmd, &buf);
	cmd = _free(cmd);
	if (buf && *buf)
	    fprintf(stdout, "%s", buf);
    } else {
        r = main_loop();
    }

exit:
    if (_rpmaugLoadargv)
	_rpmaugLoadpath = _free(_rpmaugLoadpath);
    _rpmaugLoadargv = argvFree(_rpmaugLoadargv);
    _rpmaugI = rpmaugFree(_rpmaugI);

    optCon = rpmioFini(optCon);

    return (r == 0 ? EXIT_SUCCESS : EXIT_FAILURE);
}
