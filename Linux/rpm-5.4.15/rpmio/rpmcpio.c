/*-
 * Copyright (c) 2003-2007 Tim Kientzle
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer
 *    in this position and unchanged.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR(S) ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR(S) BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#define __FBSDID(_s)
#define BSDCPIO_VERSION_STRING "2.4.13"

#define	archive_version()	BSDCPIO_VERSION_STRING

/* Default: Do not try to set owner/group. */
#define ARCHIVE_EXTRACT_OWNER   (1)
/* Default: Do obey umask, do not restore SUID/SGID/SVTX bits. */
#define ARCHIVE_EXTRACT_PERM    (2)
/* Default: Do not restore mtime/atime. */
#define ARCHIVE_EXTRACT_TIME    (4)
/* Default: Replace existing files. */
#define ARCHIVE_EXTRACT_NO_OVERWRITE (8)
/* Default: Try create first, unlink only if create fails with EEXIST. */
#define ARCHIVE_EXTRACT_UNLINK  (16)
/* Default: Do not restore ACLs. */
#define ARCHIVE_EXTRACT_ACL     (32)
/* Default: Do not restore fflags. */
#define ARCHIVE_EXTRACT_FFLAGS  (64)
/* Default: Do not restore xattrs. */
#define ARCHIVE_EXTRACT_XATTR   (128)
/* Default: Do not try to guard against extracts redirected by symlinks. */
/* Note: With ARCHIVE_EXTRACT_UNLINK, will remove any intermediate symlink. */
#define ARCHIVE_EXTRACT_SECURE_SYMLINKS (256)
/* Default: Do not reject entries with '..' as path elements. */
#define ARCHIVE_EXTRACT_SECURE_NODOTDOT (512)
/* Default: Create parent directories as needed. */
#define ARCHIVE_EXTRACT_NO_AUTODIR (1024)
/* Default: Overwrite files, even if one on disk is newer. */
#define ARCHIVE_EXTRACT_NO_OVERWRITE_NEWER (2048)

#include "system.h"
__FBSDID("$FreeBSD$");

#include <getopt.h>
#include <stdarg.h>

#include <rpmio.h>
#include "rpmcpio.h"

#include "debug.h"

/*@unchecked@*/
static struct cpio _cpio;

/*==============================================================*/

static void
cpio_vwarnc(int code, const char *fmt, va_list ap)
	/*@globals fileSystem @*/
	/*@modifies ap, fileSystem @*/
{
    fprintf(stderr, "%s: ", cpio_progname);
    (void) vfprintf(stderr, fmt, ap);
    if (code != 0)
	fprintf(stderr, ": %s", strerror(code));
    fprintf(stderr, "\n");
}

void
cpio_warnc(int code, const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    cpio_vwarnc(code, fmt, ap);
    va_end(ap);
}

void
cpio_errc(int eval, int code, const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    cpio_vwarnc(code, fmt, ap);
    va_end(ap);
    exit(eval);
}

/*==============================================================*/
struct match {
	struct match	 *next;
	int		  matches;
	char		  pattern[1];
};

struct matching {
	struct match	 *exclusions;
	int		  exclusions_count;
	struct match	 *inclusions;
	int		  inclusions_count;
	int		  inclusions_unmatched_count;
};

static void
add_pattern(/*@unused@*/ struct cpio *cpio, struct match **list, const char *pattern)
	/*@globals fileSystem @*/
	/*@modifies *list, fileSystem @*/
{
    struct match *match;

    match = xmalloc(sizeof(*match) + strlen(pattern) + 1);
    if (pattern[0] == '/')
	pattern++;
/*@-mayaliasunique@*/
    strcpy(match->pattern, pattern);
/*@=mayaliasunique@*/
    /* Both "foo/" and "foo" should match "foo/bar". */
    if (match->pattern[strlen(match->pattern)-1] == '/')
	match->pattern[strlen(match->pattern)-1] = '\0';
/*@-compmempass -unqualifiedtrans@*/
    match->next = *list;
    *list = match;
    match->matches = 0;
    return;
/*@=compmempass =unqualifiedtrans@*/
}

static void
initialize_matching(struct cpio *cpio)
	/*@globals fileSystem @*/
	/*@modifies cpio, fileSystem @*/
{
    cpio->matching = xmalloc(sizeof(*cpio->matching));
    memset(cpio->matching, 0, sizeof(*cpio->matching));
}

/*==============================================================*/
/*
 * The matching logic here needs to be re-thought.  I started out to
 * try to mimic gtar's matching logic, but it's not entirely
 * consistent.  In particular 'tar -t' and 'tar -x' interpret patterns
 * on the command line as anchored, but --exclude doesn't.
 */

/*
 * Utility functions to manage exclusion/inclusion patterns
 */

static int
exclude(struct cpio *cpio, const char *pattern)
{
    struct matching *matching;

    if (cpio->matching == NULL)
	initialize_matching(cpio);
    matching = cpio->matching;
assert(matching != NULL);
    add_pattern(cpio, &(matching->exclusions), pattern);
    matching->exclusions_count++;
    return 0;
}

#ifdef	NOTYET
static int
exclude_from_file(struct cpio *cpio, const char *pathname)
{
    return process_lines(cpio, pathname, &exclude);
}
#endif

static int
include(struct cpio *cpio, const char *pattern)
{
    struct matching *matching;

    if (cpio->matching == NULL)
	initialize_matching(cpio);
    matching = cpio->matching;
assert(matching != NULL);
    add_pattern(cpio, &(matching->inclusions), pattern);
    matching->inclusions_count++;
    matching->inclusions_unmatched_count++;
    return 0;
}

#ifdef	NOTYET
static void
cleanup_exclusions(struct cpio *cpio)
{
    struct match *p, *q;

    if (cpio->matching) {
	p = cpio->matching->inclusions;
	while (p != NULL) {
	    q = p;
	    p = p->next;
	    q = _free(q);
	}
	p = cpio->matching->exclusions;
	while (p != NULL) {
	    q = p;
	    p = p->next;
	    q = _free(q);
	}
	cpio->matching = _free(cpio->matching);
    }
}
#endif

/*==============================================================*/

static void
mode_out(struct cpio *cpio)
	/*@globals fileSystem @*/
	/*@modifies cpio, fileSystem @*/
{
	fprintf(stderr, "==> mode_out(%p)\n", cpio);
}

static void
mode_in(struct cpio *cpio)
	/*@globals fileSystem @*/
	/*@modifies cpio, fileSystem @*/
{
	fprintf(stderr, "==> mode_in(%p)\n", cpio);
	exit(0);
}

static void
mode_list(struct cpio *cpio)
	/*@globals fileSystem @*/
	/*@modifies cpio, fileSystem @*/
{
	fprintf(stderr, "==> mode_list(%p)\n", cpio);
	exit(0);
}

static void
mode_pass(struct cpio *cpio, const char *destdir)
	/*@globals fileSystem @*/
	/*@modifies cpio, fileSystem @*/
{
	fprintf(stderr, "==> mode_pass(%p,%s)\n", cpio, destdir);
}

/*==============================================================*/

/*@unchecked@*/ /*@observer@*/
static const char *cpio_opts = "aBcdf:H:hijLlmopR:rtuvW:yz";

/*
 * On systems that lack getopt_long, long options can be specified
 * using -W longopt and -W longopt=value, e.g. "-W version" is the
 * same as "--version" and "-W format=ustar" is the same as "--format
 * ustar".  This does not rely the GNU getopt() "W;" extension, so
 * should work correctly on any system with a POSIX-compliant
 * getopt().
 */

/*
 * If you add anything, be very careful to keep this list properly
 * sorted, as the -W logic below relies on it.
 */
/*@-readonlytrans -nullassign @*/
/*@unchecked@*/ /*@observer@*/
static const struct option cpio_longopts[] = {
	{ "format",             required_argument, NULL, 'H' },
	{ "help",		no_argument,	   NULL, 'h' },
	{ "owner",		required_argument, NULL, 'R' },
	{ "quiet",		no_argument,	   NULL, OPTION_QUIET },
	{ "verbose",            no_argument,       NULL, 'v' },
	{ "version",            no_argument,       NULL, OPTION_VERSION },
	{ NULL, 0, NULL, 0 }
};
/*@=readonlytrans =nullassign @*/

/*
 * Parse command-line options using system-provided getopt() or getopt_long().
 * If option is -W, then parse argument as a long option.
 */
int
cpio_getopt(struct cpio *cpio)
{
	char *p, *q;
	const struct option *option, *option2;
	int opt;
	int option_index;
	size_t option_length;

	option_index = -1;

/*@-moduncon@*/
	opt = getopt_long(cpio->argc, cpio->argv, cpio_opts,
	    cpio_longopts, &option_index);
/*@=moduncon@*/

	/* Support long options through -W longopt=value */
	if (opt == 'W') {
		p = optarg;
		q = strchr(optarg, '=');
/*@-mods@*/
		if (q != NULL) {
			option_length = (size_t)(q - p);
			optarg = q + 1;
		} else {
			option_length = strlen(p);
			optarg = NULL;
		}
/*@=mods@*/
		option = cpio_longopts;
		while (option->name != NULL &&
		    (strlen(option->name) < option_length ||
		    strncmp(p, option->name, option_length) != 0 )) {
			option++;
		}

		if (option->name != NULL) {
			option2 = option;
			opt = option->val;

			/* If the first match was exact, we're done. */
			if (strncmp(p, option->name, strlen(option->name)) == 0) {
				while (option->name != NULL)
					option++;
			} else {
				/* Check if there's another match. */
				option++;
				while (option->name != NULL &&
				    (strlen(option->name) < option_length ||
				    strncmp(p, option->name, option_length) != 0)) {
					option++;
				}
			}
			if (option->name != NULL)
				cpio_errc(1, 0,
				    "Ambiguous option %s "
				    "(matches both %s and %s)",
				    p, option2->name, option->name);

			if (option2->has_arg == required_argument
			    && optarg == NULL)
				cpio_errc(1, 0,
				    "Option \"%s\" requires argument", p);
		} else {
			opt = '?';
		}
	}

/*@-globstate@*/
	return (opt);
/*@=globstate@*/
}

/*
 * Parse the argument to the -R or --owner flag.
 *
 * The format is one of the following:
 *   <user>    - Override user but not group
 *   <user>:   - Override both, group is user's default group
 *   <user>:<group> - Override both
 *   :<group>  - Override group but not user
 *
 * A period can be used instead of the colon.
 *
 * Sets uid/gid as appropriate, -1 indicates uid/gid not specified.
 *
 */
int
owner_parse(const char *spec, int *uid, int *gid)
{
	const char *u, *ue, *g;

	*uid = -1;
	*gid = -1;

	/*
	 * Split spec into [user][:.][group]
	 *  u -> first char of username, NULL if no username
	 *  ue -> first char after username (colon, period, or \0)
	 *  g -> first char of group name
	 */
	if (*spec == ':' || *spec == '.') {
		/* If spec starts with ':' or '.', then just group. */
		ue = u = NULL;
		g = spec + 1;
	} else {
		/* Otherwise, [user] or [user][:] or [user][:][group] */
		ue = u = spec;
		while (*ue != ':' && *ue != '.' && *ue != '\0')
			++ue;
		g = ue;
		if (*g != '\0') /* Skip : or . to find first char of group. */
			++g;
	}

	if (u != NULL) {
		/* Look up user: ue is first char after end of user. */
		char *user;
		struct passwd *pwent;

		user = (char *)malloc(ue - u + 1);
		if (user == NULL) {
			cpio_warnc(errno, "Couldn't allocate memory");
			return (1);
		}
		memcpy(user, u, ue - u);
		user[ue - u] = '\0';
		pwent = getpwnam(user);
		if (pwent == NULL) {
			cpio_warnc(errno, "Couldn't lookup user ``%s''", user);
			return (1);
		}
		free(user);
		*uid = pwent->pw_uid;
		if (*ue != '\0' && *g == '\0')
			*gid = pwent->pw_gid;
	}
	if (*g != '\0') {
		struct group *grp;
		grp = getgrnam(g);
		if (grp != NULL)
			*gid = grp->gr_gid;
		else {
			cpio_warnc(errno, "Couldn't look up group ``%s''", g);
			return (1);
		}
	}
	return (0);
}

static void
version(FILE *out)
	/*@globals fileSystem @*/
	/*@modifies out, fileSystem @*/
{
	fprintf(out,"bsdcpio %s -- %s\n",
	    BSDCPIO_VERSION_STRING,
	    archive_version());
	exit(EXIT_FAILURE);
}

static void
usage(void)
	/*@globals fileSystem @*/
	/*@modifies fileSystem @*/
{
	const char	*p;

	p = cpio_progname;

	fprintf(stderr, "Brief Usage:\n");
	fprintf(stderr, "  List:    %s -it < archive\n", p);
	fprintf(stderr, "  Extract: %s -i < archive\n", p);
	fprintf(stderr, "  Create:  %s -o < filenames > archive\n", p);
	fprintf(stderr, "  Help:    %s --help\n", p);
	exit(EXIT_FAILURE);
}

/*@unchecked@*/ /*@observer@*/
static const char *long_help_msg =
	"First option must be a mode specifier:\n"
	"  -i Input  -o Output  -p Pass\n"
	"Common Options:\n"
	"  -v    Verbose\n"
	"Create: %p -o [options]  < [list of files] > [archive]\n"
	"  -z, -y  Compress archive with gzip/bzip2\n"
	"  --format {odc|newc|ustar}  Select archive format\n"
	"List: %p -it < [archive]\n"
	"Extract: %p -i [options] < [archive]\n";


/*
 * Note that the word 'bsdcpio' will always appear in the first line
 * of output.
 *
 * In particular, /bin/sh scripts that need to test for the presence
 * of bsdcpio can use the following template:
 *
 * if (cpio --help 2>&1 | grep bsdcpio >/dev/null 2>&1 ) then \
 *          echo bsdcpio; else echo not bsdcpio; fi
 */
static void
long_help(void)
	/*@globals fileSystem @*/
	/*@modifies fileSystem @*/
{
	const char	*prog;
	const char	*p;

	prog = cpio_progname;

	fflush(stderr);

	p = (strcmp(prog,"bsdcpio") != 0) ? "(bsdcpio)" : "";
	printf("%s%s: manipulate archive files\n", prog, p);

	for (p = long_help_msg; *p != '\0'; p++) {
		if (*p == '%') {
			if (p[1] == 'p') {
				fputs(prog, stdout);
				p++;
			} else
				putchar('%');
		} else
			putchar(*p);
	}
	version(stdout);
}

/*@-nullstate@*/
int
main(int argc, char *argv[])
	/*@globals cpio_progname, fileSystem @*/
	/*@modifies argv, cpio_progname, fileSystem @*/
{
	struct cpio *cpio;
	int uid, gid;
	int opt;

	cpio = &_cpio;

	/* Need cpio_progname before calling cpio_warnc. */
	if (*argv == NULL)
		cpio_progname = "bsdcpio";
	else {
		cpio_progname = strrchr(*argv, '/');
		if (cpio_progname != NULL)
/*@-modobserver@*/
			cpio_progname++;
/*@=modobserver@*/
		else
			cpio_progname = *argv;
	}

	cpio->uid_override = -1;
	cpio->gid_override = -1;
	cpio->argv = argv;
	cpio->argc = argc;
	cpio->mode = '\0';
	cpio->verbose = 0;
	cpio->compress = '\0';
	/* TODO: Implement old binary format in libarchive, use that here. */
	cpio->format = "odc"; /* Default format */
	cpio->extract_flags = ARCHIVE_EXTRACT_NO_AUTODIR;
	cpio->extract_flags |= ARCHIVE_EXTRACT_NO_OVERWRITE_NEWER;
	/* TODO: If run by root, set owner as well. */
	cpio->bytes_per_block = 512;
	cpio->filename = NULL;

	while ((opt = cpio_getopt(cpio)) != -1) {
		switch (opt) {
		case 'a': /* POSIX 1997 */
			cpio->option_atime_restore = 1;
			/*@switchbreak@*/ break;
		case 'B': /* POSIX 1997 */
			cpio->bytes_per_block = 5120;
			/*@switchbreak@*/ break;
		case 'c': /* POSIX 1997 */
			cpio->format = "odc";
			/*@switchbreak@*/ break;
		case 'd': /* POSIX 1997 */
			cpio->extract_flags &= ~ARCHIVE_EXTRACT_NO_AUTODIR;
			/*@switchbreak@*/ break;
		case 'f': /* POSIX 1997 */
			exclude(cpio, optarg);
			/*@switchbreak@*/ break;
		case 'H': /* GNU cpio, also --format */
			cpio->format = optarg;
			/*@switchbreak@*/ break;
		case 'h':
			long_help();
			/*@switchbreak@*/ break;
		case 'i': /* POSIX 1997 */
			cpio->mode = opt;
			/*@switchbreak@*/ break;
		case 'L': /* GNU cpio, BSD convention */
			cpio->option_follow_links = 1;
			/*@switchbreak@*/ break;
		case 'l': /* POSIX 1997 */
			cpio->option_link = 1;
			/*@switchbreak@*/ break;
		case 'm': /* POSIX 1997 */
			cpio->extract_flags |= ARCHIVE_EXTRACT_TIME;
			/*@switchbreak@*/ break;
		case 'o': /* POSIX 1997 */
			cpio->mode = opt;
			/*@switchbreak@*/ break;
		case 'p': /* POSIX 1997 */
			cpio->mode = opt;
			/*@switchbreak@*/ break;
		case OPTION_QUIET: /* GNU cpio */
			cpio->quiet = 1;
			/*@switchbreak@*/ break;
		case 'R': /* GNU cpio, also --owner */
			if (owner_parse(optarg, &uid, &gid))
				usage();
			if (uid != -1)
				cpio->uid_override = uid;
			if (gid != -1)
				cpio->gid_override = gid;
			/*@switchbreak@*/ break;
		case 'r': /* POSIX 1997 */
			cpio->option_rename = 1;
			/*@switchbreak@*/ break;
		case 't': /* POSIX 1997 */
			cpio->option_list = 1;
			/*@switchbreak@*/ break;
		case 'u': /* POSIX 1997 */
			cpio->extract_flags
			    &= ~ARCHIVE_EXTRACT_NO_OVERWRITE_NEWER;
			/*@switchbreak@*/ break;
		case 'v': /* POSIX 1997 */
			cpio->verbose++;
			/*@switchbreak@*/ break;
		case OPTION_VERSION: /* GNU convention */
			version(stdout);
			/*@switchbreak@*/ break;
#if 0
	        /*
		 * cpio_getopt() handles -W specially, so it's not
		 * available here.
		 */
		case 'W': /* Obscure, but useful GNU convention. */
			/*@switchbreak@*/ break;
#endif
		case 'y': /* tar convention */
			cpio->compress = opt;
			/*@switchbreak@*/ break;
		case 'z': /* tar convention */
			cpio->compress = opt;
			/*@switchbreak@*/ break;
		default:
			usage();
		}
	}

	/* TODO: Sanity-check args, error out on nonsensical combinations. */

	cpio->argc -= optind;
	cpio->argv += optind;

	switch (cpio->mode) {
	case 'o':
		mode_out(cpio);
		break;
	case 'i':
		while (*cpio->argv != NULL) {
			include(cpio, *cpio->argv);
			--cpio->argc;
			++cpio->argv;
		}
		if (cpio->option_list)
			mode_list(cpio);
		else
			mode_in(cpio);
		break;
	case 'p':
		if (*cpio->argv == NULL || **cpio->argv == '\0')
			cpio_errc(1, 0,
			    "-p mode requires a target directory");
		mode_pass(cpio, *cpio->argv);
		break;
	default:
		cpio_errc(1, 0,
		    "Must specify at least one of -i, -o, or -p");
	}

/*@-globstate@*/
	return (0);
/*@=globstate@*/
}
/*@=nullstate@*/
