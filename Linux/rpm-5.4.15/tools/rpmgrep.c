/*************************************************
*               pcregrep program                 *
*************************************************/

/* This is a grep program that uses the PCRE regular expression library to do
its pattern matching. On a Unix or Win32 system it can recurse into
directories.

           Copyright (c) 1997-2008 University of Cambridge

-----------------------------------------------------------------------------
Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice,
      this list of conditions and the following disclaimer.

    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.

    * Neither the name of the University of Cambridge nor the names of its
      contributors may be used to endorse or promote products derived from
      this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
-----------------------------------------------------------------------------
*/

#include "system.h"

#define _MIRE_INTERNAL
#include <rpmio_internal.h>	/* XXX fdGetFILE */
#include <rpmdir.h>
#include <poptIO.h>

#include "debug.h"

/*@access miRE @*/

typedef unsigned BOOL;
#define FALSE	((BOOL)0)
#define TRUE	((BOOL)1)

#define MAX_PATTERN_COUNT 100

#if BUFSIZ > 8192
#define MBUFTHIRD BUFSIZ
#else
#define MBUFTHIRD 8192
#endif

static inline void fwrite_check(const void *ptr, size_t size, size_t nmemb, FILE *stream)
{
    if(fwrite(ptr, size, nmemb, stream) != nmemb)
	perror("fwrite");
}

/*************************************************
*               Global variables                 *
*************************************************/

/*@unchecked@*/ /*@only@*/ /*@null@*/
static const char *newline = NULL;

/*@unchecked@*/ /*@only@*/ /*@null@*/
static const char *color_string = NULL;
/*@unchecked@*/ /*@only@*/ /*@null@*/
static ARGV_t pattern_filenames = NULL;
/*@unchecked@*/ /*@only@*/ /*@null@*/
static const char *stdin_name = NULL;

/*@unchecked@*/ /*@only@*/ /*@null@*/
static const char *locale = NULL;

/*@unchecked@*/ /*@only@*/ /*@relnull@*/
static ARGV_t patterns = NULL;
/*@unchecked@*/ /*@only@*/ /*@relnull@*/
static miRE pattern_list = NULL;
/*@unchecked@*/
static int  pattern_count = 0;

/*@unchecked@*/ /*@only@*/ /*@null@*/
static ARGV_t exclude_patterns = NULL;
/*@unchecked@*/ /*@only@*/ /*@relnull@*/
static miRE excludeMire = NULL;
/*@unchecked@*/
static int nexcludes = 0;

/*@unchecked@*/ /*@only@*/ /*@null@*/
static ARGV_t include_patterns = NULL;
/*@unchecked@*/ /*@only@*/ /*@relnull@*/
static miRE includeMire = NULL;
/*@unchecked@*/
static int nincludes = 0;

/*@unchecked@*/
static int after_context = 0;
/*@unchecked@*/
static int before_context = 0;
/*@unchecked@*/
static int both_context = 0;

/** Actions for the -d option */
enum dee_e { dee_READ=1, dee_SKIP, dee_RECURSE };
/*@unchecked@*/
static enum dee_e dee_action = dee_READ;

/** Actions for the -D option */
enum DEE_e { DEE_READ=1, DEE_SKIP };
/*@unchecked@*/
static enum DEE_e DEE_action = DEE_READ;

/*@unchecked@*/
static int error_count = 0;

/**
 * Values for the "filenames" variable, which specifies options for file name
 * output. The order is important; it is assumed that a file name is wanted for
 * all values greater than FN_DEFAULT.
 */
enum FN_e { FN_NONE, FN_DEFAULT, FN_ONLY, FN_NOMATCH_ONLY, FN_FORCE };
/*@unchecked@*/
static enum FN_e filenames = FN_DEFAULT;

#define	_GFB(n)	((1U << (n)) | 0x40000000)
#define GF_ISSET(_FLAG) ((grepFlags & ((GREP_FLAGS_##_FLAG) & ~0x40000000)) != GREP_FLAGS_NONE)

enum grepFlags_e {
    GREP_FLAGS_NONE		= 0,

/* XXX WATCHOUT: the next 3 bits are also used as an index, do not change!!! */
    GREP_FLAGS_WORD_MATCH	= _GFB( 0), /*!< -w,--word-regex ... */
    GREP_FLAGS_LINE_MATCH	= _GFB( 1), /*!< -x,--line-regex ... */
    GREP_FLAGS_FIXED_STRINGS	= _GFB( 2), /*!< -F,--fixed-strings ... */

    GREP_FLAGS_COUNT		= _GFB( 3), /*!< -c,--count ... */
    GREP_FLAGS_COLOR		= _GFB( 4), /*!< --color ... */
    GREP_FLAGS_FOFFSETS		= _GFB( 5), /*!< --file-offsets ... */
    GREP_FLAGS_LOFFSETS		= _GFB( 6), /*!< --line-offsets ... */
    GREP_FLAGS_LNUMBER		= _GFB( 7), /*!< -n,--line-number ... */
    GREP_FLAGS_MULTILINE	= _GFB( 8), /*!< -M,--multiline ... */
    GREP_FLAGS_ONLY_MATCHING	= _GFB( 9), /*!< -o,--only-matching ... */
    GREP_FLAGS_INVERT		= _GFB(10), /*!< -v,--invert ... */
    GREP_FLAGS_QUIET		= _GFB(11), /*!< -q,--quiet ... */
    GREP_FLAGS_SILENT		= _GFB(12), /*!< -s,--no-messages ... */
    GREP_FLAGS_UTF8		= _GFB(13), /*!< -u,--utf8 ... */
    GREP_FLAGS_CASELESS		= _GFB(14), /*!< -i,--ignore-case ... */
};

/*@unchecked@*/
static enum grepFlags_e grepFlags = GREP_FLAGS_NONE;

#if defined(WITH_PCRE)
/*@unchecked@*/
static rpmMireMode grepMode = RPMMIRE_PCRE;
#else
static rpmMireMode grepMode = RPMMIRE_REGEX;
#endif

/*@unchecked@*/
static struct rpmop_s grep_totalops;
/*@unchecked@*/
static struct rpmop_s grep_readops;

/**
 * Tables for prefixing and suffixing patterns, according to the -w, -x, and -F
 * options. These set the 1, 2, and 4 bits in grepFlags, respectively.
 * Note that the combination of -w and -x has the same effect as -x on its own,
 * so we can treat them as the same.
 */
/*@unchecked@*/ /*@observer@*/
static const char *prefix[] = {
    "", "\\b", "^(?:", "^(?:", "\\Q", "\\b\\Q", "^(?:\\Q", "^(?:\\Q"
};

/*@unchecked@*/ /*@observer@*/
static const char *suffix[] = {
    "", "\\b", ")$",   ")$",   "\\E", "\\E\\b", "\\E)$",   "\\E)$"
};

/** UTF-8 tables - used only when the newline setting is "any". */
/*@unchecked@*/ /*@observer@*/
static const unsigned utf8_table3[] = {
    0xff, 0x1f, 0x0f, 0x07, 0x03, 0x01
};

/*@+charint@*/
/*@unchecked@*/ /*@observer@*/
static const char utf8_table4[] = {
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
    3,3,3,3,3,3,3,3,4,4,4,4,5,5,5,5
};
/*@=charint@*/

/*************************************************
 * Find end of line.
 *
 * The length of the endline sequence that is found is set via lenptr.
 * This may be zero at the very end of the file if there is no line-ending
 * sequence there.
 *
 * @param p		current position in line
 * @param endptr	end of available data
 * @retval *lenptr	length of the eol sequence
 * @return		pointer to the last byte of the line
*/
/*@observer@*/
static const char *
end_of_line(const char *p, const char *endptr, /*@out@*/ size_t *lenptr)
	/*@modifies *lenptr @*/
{
    switch(_mireEL) {
    default:	/* Just in case */
    case EL_LF:
	while (p < endptr && *p != '\n') p++;
	if (p < endptr) {
	    *lenptr = 1;
	    return p + 1;
	}
	*lenptr = 0;
	return endptr;
	/*@notreached@*/ break;

    case EL_CR:
	while (p < endptr && *p != '\r') p++;
	if (p < endptr) {
	    *lenptr = 1;
	    return p + 1;
	}
	*lenptr = 0;
	return endptr;
	/*@notreached@*/ break;

    case EL_CRLF:
	for (;;) {
	    while (p < endptr && *p != '\r') p++;
	    if (++p >= endptr) {
		*lenptr = 0;
		return endptr;
	    }
	    if (*p == '\n') {
		*lenptr = 2;
		return p + 1;
	    }
	}
	/*@notreached@*/ break;

    case EL_ANYCRLF:
	while (p < endptr) {
	    size_t extra = 0;
	    unsigned int c = (unsigned)*((unsigned char *)p);

	    if (GF_ISSET(UTF8) && c >= 0xc0) {
		size_t gcii, gcss;
		extra = (size_t)utf8_table4[c & 0x3f];  /* No. of additional bytes */
		gcss = 6*extra;
		c = (c & utf8_table3[extra]) << gcss;
		for (gcii = 1; gcii <= extra; gcii++) {
		    gcss -= 6;
		    c |= ((unsigned)p[gcii] & 0x3f) << gcss;
		}
	    }

	    p += 1 + extra;

	    switch (c) {
	    case 0x0a:    /* LF */
		*lenptr = 1;
		return p;
		/*@notreached@*/ /*@switchbreak@*/ break;

	    case 0x0d:    /* CR */
		if (p < endptr && (unsigned)*p == 0x0a) {
		    *lenptr = 2;
		    p++;
		}
		else *lenptr = 1;
		return p;
		/*@notreached@*/ /*@switchbreak@*/ break;

	    default:
		/*@switchbreak@*/ break;
		}
	}   /* End of loop for ANYCRLF case */

	*lenptr = 0;  /* Must have hit the end */
	return endptr;
	/*@notreached@*/ break;

    case EL_ANY:
	while (p < endptr) {
	    size_t extra = 0;
	    unsigned int c = (unsigned)*((unsigned char *)p);

	    if (GF_ISSET(UTF8) && c >= 0xc0) {
		size_t gcii, gcss;
		extra = (size_t)utf8_table4[c & 0x3f];  /* No. of additional bytes */
		gcss = 6*extra;
		c = (c & utf8_table3[extra]) << gcss;
		for (gcii = 1; gcii <= extra; gcii++) {
		    gcss -= 6;
		    c |= ((unsigned)p[gcii] & 0x3f) << gcss;
		}
	    }

	    p += 1 + extra;

	    switch (c) {
	    case 0x0a:    /* LF */
	    case 0x0b:    /* VT */
	    case 0x0c:    /* FF */
		*lenptr = 1;
		return p;
		/*@notreached@*/ /*@switchbreak@*/ break;

	    case 0x0d:    /* CR */
		if (p < endptr && (unsigned)*p == 0x0a) {
		    *lenptr = 2;
		    p++;
		}
		else *lenptr = 1;
		return p;
		/*@notreached@*/ /*@switchbreak@*/ break;

	    case 0x85:    /* NEL */
		*lenptr = GF_ISSET(UTF8) ? 2 : 1;
		return p;
		/*@notreached@*/ /*@switchbreak@*/ break;

	    case 0x2028:  /* LS */
	    case 0x2029:  /* PS */
		*lenptr = 3;
		return p;
		/*@notreached@*/ /*@switchbreak@*/ break;

	    default:
		/*@switchbreak@*/ break;
	    }
	}   /* End of loop for ANY case */

	*lenptr = 0;  /* Must have hit the end */
	return endptr;
	/*@notreached@*/ break;
    }	/* End of overall switch */
    /*@notreached@*/
}

/*************************************************
 * Find start of previous line
 *
 * This is called when looking back for before lines to print.
 *
 * @param p		start of the subsequent line
 * @param startptr	start of available data
 * @return 		pointer to the start of the previous line
 */
/*@observer@*/
static const char *
previous_line(const char *p, const char *startptr)
	/*@*/
{
    switch (_mireEL) {
    default:      /* Just in case */
    case EL_LF:
	p--;
	while (p > startptr && p[-1] != '\n') p--;
	return p;
	/*@notreached@*/ break;

    case EL_CR:
	p--;
	while (p > startptr && p[-1] != '\n') p--;
	return p;
	/*@notreached@*/ break;

    case EL_CRLF:
	for (;;) {
	    p -= 2;
	    while (p > startptr && p[-1] != '\n') p--;
	    if (p <= startptr + 1 || p[-2] == '\r') return p;
	}
	/*@notreached@*/ return p;   /* But control should never get here */
	/*@notreached@*/ break;

    case EL_ANY:
    case EL_ANYCRLF:
	if (*(--p) == '\n' && p > startptr && p[-1] == '\r') p--;
	if (GF_ISSET(UTF8)) while ((((unsigned)*p) & 0xc0) == 0x80) p--;

	while (p > startptr) {
	    const char *pp = p - 1;
	    unsigned int c;

	    if (GF_ISSET(UTF8)) {
		size_t extra = 0;
		while ((((unsigned)*pp) & 0xc0) == 0x80) pp--;
		c = (unsigned)*((unsigned char *)pp);
		if (c >= 0xc0) {
		    size_t gcii, gcss;
		    extra = (size_t)utf8_table4[c & 0x3f]; /* No. of additional bytes */
		    gcss = 6*extra;
		    c = (c & utf8_table3[extra]) << gcss;
		    for (gcii = 1; gcii <= extra; gcii++) {
			gcss -= 6;
			c |= ((unsigned)pp[gcii] & 0x3f) << gcss;
		    }
		}
	    } else
		c = (unsigned)*((unsigned char *)pp);

	    if (_mireEL == EL_ANYCRLF) {
		switch (c) {
		case 0x0a:    /* LF */
		case 0x0d:    /* CR */
		    return p;
		    /*@notreached@*/ /*@switchbreak@*/ break;

		default:
		    /*@switchbreak@*/ break;
		}
	    } else {
		switch (c) {
		case 0x0a:    /* LF */
		case 0x0b:    /* VT */
		case 0x0c:    /* FF */
		case 0x0d:    /* CR */
		case 0x85:    /* NEL */
		case 0x2028:  /* LS */
		case 0x2029:  /* PS */
		    return p;
		    /*@notreached@*/ /*@switchbreak@*/ break;

		default:
		    /*@switchbreak@*/ break;
		}
	    }

	    p = pp;  /* Back one character */
	}	/* End of loop for ANY case */

	return startptr;  /* Hit start of data */
	/*@notreached@*/ break;
    }	/* End of overall switch */
    /*@notreached@*/
}

/*************************************************
 * Print the previous "after" lines
 *
 * This is called if we are about to lose said lines because of buffer filling,
 * and at the end of the file. The data in the line is written using fwrite() so
 * that a binary zero does not terminate it.
 *
 * @param lastmatchnumber	the number of the last matching line, plus one
 * @param lastmatchrestart	where we restarted after the last match
 * @param endptr		end of available data
 * @param printname		filename for printing (or NULL)
 */
static void do_after_lines(int lastmatchnumber, const char *lastmatchrestart,
		const char *endptr, /*@null@*/ const char *printname)
	/*@globals fileSystem @*/
	/*@modifies fileSystem @*/
{
    int count = 0;
    while (lastmatchrestart < endptr && count++ < after_context) {
	const char *pp = lastmatchrestart;
	size_t ellength;
	if (printname != NULL) fprintf(stdout, "%s-", printname);
	if (GF_ISSET(LNUMBER)) fprintf(stdout, "%d-", lastmatchnumber++);
	pp = end_of_line(pp, endptr, &ellength);
	fwrite_check(lastmatchrestart, 1, pp - lastmatchrestart, stdout);
	lastmatchrestart = pp;
    }
}

/*************************************************
 * Grep an individual file
 *
 * This is called from grep_or_recurse() below. It uses a buffer that is three
 * times the value of MBUFTHIRD. The matching point is never allowed to stray
 * into the top third of the buffer, thus keeping more of the file available
 * for context printing or for multiline scanning. For large files, the pointer
 * will be in the middle third most of the time, so the bottom third is
 * available for "before" context printing.
 *
 * @param  handle	the fopen'd FILE stream for a normal file
 *			the gzFile pointer when reading is via libz
 *			the BZFILE pointer when reading is via libbz2
 * @param frtype	FR_PLAIN, FR_LIBZ, or FR_LIBBZ2
 * @param printname	the file name if it is to be printed for each match
 *			or NULL if the file name is not to be printed
 *			it cannot be NULL if filenames[_nomatch]_only is set
 * @return		0: at least one match, 1: no match, 2: read error (bz2)
 */
static int
pcregrep(FD_t fd, const char *printname)
	/*@globals error_count, pattern_list, fileSystem @*/
	/*@modifies fd, error_count, pattern_list, fileSystem @*/
{
    int rc = 1;
    int linenumber = 1;
    int lastmatchnumber = 0;
    int count = 0;
    int filepos = 0;
    int offsets[99];
    const char *lastmatchrestart = NULL;
    char buffer[3*MBUFTHIRD];
    const char *ptr = buffer;
    const char *endptr;
    size_t bufflength;
    static BOOL hyphenpending = FALSE;
    BOOL endhyphenpending = FALSE;
    BOOL invert = (GF_ISSET(INVERT) ? TRUE : FALSE);

    bufflength = Fread(buffer, 1, 3*MBUFTHIRD, fd);
    endptr = buffer + bufflength;

    /*
     * Loop while the current pointer is not at the end of the file. For large
     * files, endptr will be at the end of the buffer when we are in the middle
     * of the file, but ptr will never get there, because as soon as it gets
     * over 2/3 of the way, the buffer is shifted left and re-filled.
     */
    while (ptr < endptr) {
	int i;
	int mrc = 0;
	BOOL match = FALSE;
	const char *matchptr = ptr;
	const char *t = ptr;
	size_t length, linelength;
	size_t endlinelength;

	/*
	 * At this point, ptr is at the start of a line. We need to find the
	 * length of the subject string to pass to mireRegexec(). In multiline
	 * mode, it is the length remainder of the data in the buffer.
	 * Otherwise, it is the length of the next line. After matching, we
	 * always advance by the length of the next line. In multiline mode
	 * the PCRE_FIRSTLINE option is used for compiling, so that any match
	 * is constrained to be in the first line.
	 */
	t = end_of_line(t, endptr, &endlinelength);
	linelength = t - ptr - endlinelength;
	length = GF_ISSET(MULTILINE) ? (size_t)(endptr - ptr) : linelength;

	/*
	 * We come back here after a match when the -o,--only-matching option
	 * is set, in order to find any further matches in the same line.
	 */
ONLY_MATCHING_RESTART:

	/*
	 * Run through all the patterns until one matches. Note that we don't
	 * include the final newline in the subject string.
	 */
	for (i = 0; i < pattern_count; i++) {
	    miRE mire = pattern_list + i;
	    int xx;

/*@-onlytrans@*/
	    /* Set sub-string offset array. */
	    xx = mireSetEOptions(mire, offsets, 99);

	    /* XXX WATCHOUT: mireRegexec w length=0 does strlen(matchptr)! */
	    mrc = (length > 0 ? mireRegexec(mire, matchptr, length) : -1);
/*@=onlytrans@*/
	    if (mrc >= 0) { match = TRUE; /*@innerbreak@*/ break; }
	    if (mrc < -1) {	/* XXX -1 == NOMATCH, otherwise error. */
		fprintf(stderr, _("%s: pcre_exec() error %d while matching "), __progname, mrc);
		if (pattern_count > 1) fprintf(stderr, _("pattern number %d to "), i+1);
		fprintf(stderr, _("this line:\n"));
		fwrite_check(matchptr, 1, linelength, stderr);  /* In case binary zero included */
		fprintf(stderr, "\n");
#if defined(PCRE_ERROR_MATCHLIMIT)
		if (error_count == 0 &&
			(mrc == PCRE_ERROR_MATCHLIMIT || mrc == PCRE_ERROR_RECURSIONLIMIT))
		{
		    fprintf(stderr,
			_("%s: error %d means that a resource limit was exceeded\n"),
			__progname, mrc);
		    fprintf(stderr,
			_("%s: check your regex for nested unlimited loops\n"),
			__progname);
		}
#endif
		if (error_count++ > 20) {
		    fprintf(stderr, _("%s: too many errors - abandoned\n"),
			__progname);
/*@-exitarg@*/
		    exit(2);
/*@=exitarg@*/
		}
		match = invert;    /* No more matching; don't show the line again */
		/*@innerbreak@*/ break;
	    }
	}

	/* If it's a match or a not-match (as required), do what's wanted. */
	if (match != invert) {
	    BOOL hyphenprinted = FALSE;

	    /* We've failed if we want a file that doesn't have any matches. */
	    if (filenames == FN_NOMATCH_ONLY) {
		rc = 1;
		goto exit;
	    }

	    /* Just count if just counting is wanted. */
	    if (GF_ISSET(COUNT)) count++;

	    /*
	     * If all we want is a file name, there is no need to scan any
	     * more lines in the file.
	     */
	    else if (filenames == FN_ONLY) {
		if (printname != NULL) fprintf(stdout, "%s\n", printname);
		rc = 0;
		goto exit;
	    }

	    /* Likewise, if all we want is a yes/no answer. */
	    else if (GF_ISSET(QUIET)) {
		rc = 0;
		goto exit;
	    }

	    /*
	     * The --only-matching option prints just the substring that
	     * matched, and the --file-offsets and --line-offsets options
	     * output offsets for the matching substring (they both force
	     * --only-matching). None of these options prints any context.
	     * Afterwards, adjust the start and length, and then jump back
	     * to look for further matches in the same line. If we are in
	     * invert mode, however, nothing is printed - this could be
	     * still useful because the return code is set.
	     */
	    else if (GF_ISSET(ONLY_MATCHING)) {
		if (!GF_ISSET(INVERT)) {
		    if (printname != NULL) fprintf(stdout, "%s:", printname);
		    if (GF_ISSET(LNUMBER)) fprintf(stdout, "%d:", linenumber);
		    if (GF_ISSET(LOFFSETS))
			fprintf(stdout, "%d,%d", (int)(matchptr + offsets[0] - ptr),
			    offsets[1] - offsets[0]);
		    else if (GF_ISSET(FOFFSETS))
			fprintf(stdout, "%d,%d", (int)(filepos + matchptr + offsets[0] - ptr),
			    offsets[1] - offsets[0]);
		    else
			fwrite_check(matchptr + offsets[0], 1, offsets[1] - offsets[0], stdout);
		    fprintf(stdout, "\n");
		    matchptr += offsets[1];
		    length -= offsets[1];
		    match = FALSE;
		    goto ONLY_MATCHING_RESTART;
		}
	    }

	    /*
	     * This is the default case when none of the above options is set.
	     * We print the matching lines(s), possibly preceded and/or
	     * followed by other lines of context.
	     */
	    else {
		/*
		 * See if there is a requirement to print some "after" lines
		 * from a previous match. We never print any overlaps.
		 */
		if (after_context > 0
		 && lastmatchnumber > 0 && lastmatchrestart != NULL)
		{
		    size_t ellength;
		    int linecount = 0;
		    const char *p = lastmatchrestart;

		    while (p < ptr && linecount < after_context) {
			p = end_of_line(p, ptr, &ellength);
			linecount++;
		    }

		    /*
		     * It is important to advance lastmatchrestart during this
		     * printing so that it interacts correctly with any
		     * "before" printing below. Print each line's data using
		     * fwrite() in case there are binary zeroes.
		     */
		    while (lastmatchrestart < p) {
			const char *pp = lastmatchrestart;
			if (printname != NULL) fprintf(stdout, "%s-", printname);
			if (GF_ISSET(LNUMBER)) fprintf(stdout, "%d-", lastmatchnumber++);
			pp = end_of_line(pp, endptr, &ellength);
			fwrite_check(lastmatchrestart, 1, pp - lastmatchrestart, stdout);
			lastmatchrestart = pp;
		    }
		    if (lastmatchrestart != ptr) hyphenpending = TRUE;
		}

		/* If there were non-contiguous lines printed above, insert hyphens. */
		if (hyphenpending) {
		    fprintf(stdout, "--\n");
		    hyphenpending = FALSE;
		    hyphenprinted = TRUE;
		}

		/*
		 * See if there is a requirement to print some "before" lines
		 * for this match. Again, don't print overlaps.
		 */
		if (before_context > 0) {
		    int linecount = 0;
		    const char *p = ptr;

		    while (p > buffer && (lastmatchnumber == 0 || p > lastmatchrestart) &&
			       linecount < before_context)
		    {
			linecount++;
			p = previous_line(p, buffer);
		    }

		    if (lastmatchnumber > 0 && p > lastmatchrestart && !hyphenprinted)
			fprintf(stdout, "--\n");

		    while (p < ptr) {
			size_t ellength;
			const char *pp = p;
			if (printname != NULL) fprintf(stdout, "%s-", printname);
			if (GF_ISSET(LNUMBER)) fprintf(stdout, "%d-", linenumber - linecount--);
			pp = end_of_line(pp, endptr, &ellength);
			fwrite_check(p, 1, pp - p, stdout);
			p = pp;
		    }
		}

		/*
		 * Now print the matching line(s); ensure we set hyphenpending
		 * at the end of the file if any context lines are being output.
		 */
		if (after_context > 0 || before_context > 0)
		    endhyphenpending = TRUE;

		if (printname != NULL) fprintf(stdout, "%s:", printname);
		if (GF_ISSET(LNUMBER)) fprintf(stdout, "%d:", linenumber);

		/*
		 * In multiline mode, we want to print to the end of the line
		 * in which the end of the matched string is found, so we
		 * adjust linelength and the line number appropriately, but
		 * only when there actually was a match (invert not set).
		 * Because the PCRE_FIRSTLINE option is set, the start of
		 * the match will always be before the first newline sequence.
		 * */
		if (GF_ISSET(MULTILINE)) {
		    size_t ellength;
		    const char *endmatch = ptr;
		    if (!GF_ISSET(INVERT)) {
			endmatch += offsets[1];
			t = ptr;
			while (t < endmatch) {
			    t = end_of_line(t, endptr, &ellength);
			    if (t <= endmatch) linenumber++; else /*@innerbreak@*/ break;
			}
		    }
		    endmatch = end_of_line(endmatch, endptr, &ellength);
		    linelength = endmatch - ptr - ellength;
		}
		/*
		 * NOTE: Use only fwrite() to output the data line, so that
		 * binary zeroes are treated as just another data character.
		 */

		/* We have to split the line(s) up if coloring. */
		if (GF_ISSET(COLOR) && color_string != NULL) {
		    fwrite_check(ptr, 1, offsets[0], stdout);
		    fprintf(stdout, "%c[%sm", 0x1b, color_string);
		    fwrite_check(ptr + offsets[0], 1, offsets[1] - offsets[0], stdout);
		    fprintf(stdout, "%c[00m", 0x1b);
		    fwrite_check(ptr + offsets[1], 1, (linelength + endlinelength) - offsets[1],
			stdout);
		}
		else fwrite_check(ptr, 1, linelength + endlinelength, stdout);
	    }

	    /* End of doing what has to be done for a match */
	    rc = 0;    /* Had some success */

	    /*
	     * Remember where the last match happened for after_context.
	     * We remember where we are about to restart, and that line's
	     * number.
	     */
	    lastmatchrestart = ptr + linelength + endlinelength;
	    lastmatchnumber = linenumber + 1;
	}

	/*
	 * For a match in multiline inverted mode (which of course did not
	 * cause anything to be printed), we have to move on to the end of
	 * the match before proceeding.
	 */
	if (GF_ISSET(MULTILINE) && GF_ISSET(INVERT) && match) {
	    size_t ellength;
	    const char *endmatch = ptr + offsets[1];
	    t = ptr;
	    while (t < endmatch) {
		t = end_of_line(t, endptr, &ellength);
		if (t <= endmatch) linenumber++; else /*@innerbreak@*/ break;
	    }
	    endmatch = end_of_line(endmatch, endptr, &ellength);
	    linelength = endmatch - ptr - ellength;
	}

	/*
	 * Advance to after the newline and increment the line number. The
	 * file offset to the current line is maintained in filepos.
	 */
	ptr += linelength + endlinelength;
	filepos += linelength + endlinelength;
	linenumber++;

	/*
	 * If we haven't yet reached the end of the file (the buffer is full),
	 * and the current point is in the top 1/3 of the buffer, slide the
	 * buffer down by 1/3 and refill it. Before we do this, if some
	 * unprinted "after" lines are about to be lost, print them.
	 */
	if (bufflength >= sizeof(buffer) && ptr > buffer + 2*MBUFTHIRD) {
	    if (after_context > 0 &&
		lastmatchnumber > 0 && lastmatchrestart != NULL &&
		lastmatchrestart < buffer + MBUFTHIRD)
	    {
		if (after_context > 0
		 && lastmatchnumber > 0 && lastmatchrestart != NULL)
		{
		    do_after_lines(lastmatchnumber, lastmatchrestart,
				endptr, printname);
		    hyphenpending = TRUE;
		}
		lastmatchnumber = 0;
	    }

	    /* Now do the shuffle */

/*@-modobserver@*/	/* XXX buffer <=> t aliasing */
	    memmove(buffer, buffer + MBUFTHIRD, 2*MBUFTHIRD);
	    ptr -= MBUFTHIRD;

	    bufflength = 2*MBUFTHIRD;
	    bufflength += Fread(buffer + bufflength, 1, MBUFTHIRD, fd);
	    endptr = buffer + bufflength;
/*@=modobserver@*/

	    /* Adjust any last match point */
	    if (lastmatchnumber > 0 && lastmatchrestart != NULL)
		lastmatchrestart -= MBUFTHIRD;
	}
    }	/* Loop through the whole file */

    /*
     * End of file; print final "after" lines if wanted; do_after_lines sets
     * hyphenpending if it prints something.
     */
    if (!GF_ISSET(ONLY_MATCHING) && !GF_ISSET(COUNT)) {
	if (after_context > 0
	 && lastmatchnumber > 0 && lastmatchrestart != NULL)
	{
	    do_after_lines(lastmatchnumber, lastmatchrestart, endptr, printname);
	    hyphenpending = TRUE;
	}
	hyphenpending |= endhyphenpending;
    }

    /*
     * Print the file name if we are looking for those without matches and
     * there were none. If we found a match, we won't have got this far.
     */
    if (filenames == FN_NOMATCH_ONLY) {
	if (printname != NULL) fprintf(stdout, "%s\n", printname);
	rc = 0;
	goto exit;
    }

    /* Print the match count if wanted */
    if (GF_ISSET(COUNT)) {
	if (printname != NULL) fprintf(stdout, "%s:", printname);
	fprintf(stdout, "%d\n", count);
    }

exit:
    return rc;
}

/**
 * Check file name for a suffix.
 * @param fn		file name
 * @param suffix	suffix
 * @return		1 if file name ends with suffix
 */
static int chkSuffix(const char * fn, const char * suffix)
	/*@*/
{
    size_t flen = strlen(fn);
    size_t slen = strlen(suffix);
    return (flen > slen && !strcmp(fn + flen - slen, suffix));
}

/*************************************************
 * Grep a file or recurse into a directory.
 *
 * Given a path name, if it's a directory, scan all the files if we are
 * recursing; if it's a file, grep it.
 *
 * @param pathname		the path to investigate
 * @param dir_recurse		TRUE if recursing is wanted (-r or -drecurse)
 * @param only_one_at_top	TRUE if the path is the only one at toplevel
 * @return		0: at least one match, 1: no match, 2: read error (bz2)
 *
 * @note file opening failures are suppressed if "silent" is set.
 */
static int
grep_or_recurse(const char *pathname, BOOL dir_recurse, BOOL only_one_at_top)
	/*@globals h_errno, fileSystem, internalState @*/
	/*@modifies h_errno, fileSystem, internalState @*/
{
    struct stat sb, *st = &sb;
    int rc = 1;
    size_t pathlen;
    FD_t fd = NULL;
    const char * fmode = "r.ufdio";
    int xx;

    /* If the file name is "-" we scan stdin */
    if (!strcmp(pathname, "-")) {
	fd = fdDup(STDIN_FILENO);
	goto openthestream;
    }

    if ((xx = Stat(pathname, st)) != 0)
	goto openthestream;	/* XXX exit with Strerror(3) message. */

    /*
     * If the file is a directory, skip if skipping or if we are recursing,
     * scan each file within it, subject to any include or exclude patterns
     * that were set.  The scanning code is localized so it can be made
     * system-specific.
     */
    if (S_ISDIR(st->st_mode))
    switch (dee_action) {
    case dee_READ:
	break;
    case dee_SKIP:
	rc = 1;
	goto exit;
	/*@notreached@*/ break;
    case dee_RECURSE:
	{   char buffer[1024];
	    DIR *dir = Opendir(pathname);
	    struct dirent *dp;

	    if (dir == NULL) {
		if (!GF_ISSET(SILENT))
		    fprintf(stderr, _("%s: Failed to open directory %s: %s\n"),
			__progname, pathname, strerror(errno));
		rc = 2;
		goto exit;
	    }

	    while ((dp = Readdir(dir)) != NULL) {
		char sep = '/';

		if (!strcmp(dp->d_name, ".") || !strcmp(dp->d_name, ".."))
		    continue;

		xx = snprintf(buffer, sizeof(buffer), "%.512s%c%.128s",
			pathname, sep, dp->d_name);
		buffer[sizeof(buffer)-1] = '\0';

/*@-onlytrans@*/
		if (mireApply(excludeMire, nexcludes, buffer, 0, -1) >= 0)
		    continue;

		if (mireApply(includeMire, nincludes, buffer, 0, +1) < 0)
		    continue;
/*@=onlytrans@*/

		xx = grep_or_recurse(buffer, dir_recurse, FALSE);
		if (xx > 1) rc = xx;
		else if (xx == 0 && rc == 1) rc = 0;
	    }
	    xx = Closedir(dir);
	    goto exit;
	} /*@notreached@*/ break;
    }

    /*
     * If the file is not a directory and not a regular file, skip it if
     * that's been requested.
     */
    else if (!S_ISREG(st->st_mode) && DEE_action == DEE_SKIP) {
	rc = 1;
	goto exit;
    }

    /*
     * Control reaches here if we have a regular file, or if we have a
     * directory and recursion or skipping was not requested, or if we have
     * anything else and skipping was not requested. The scan proceeds.
     * If this is the first and only argument at top level, we don't show
     * the file name, unless we are only showing the file name, or the
     * filename was forced (-H).
     */
    pathlen = strlen(pathname);

    /* Identify how to Fopen the file from the suffix. */
    if (chkSuffix(pathname, ".gz"))
	fmode = "r.gzdio";	/* Open with zlib decompression. */
    else if (chkSuffix(pathname, ".bz2"))
	fmode = "r.bzdio";	/* Open with bzip2 decompression. */
    else if (chkSuffix(pathname, ".lzma"))
	fmode = "r.lzdio";	/* Open with lzma decompression. */
    else if (chkSuffix(pathname, ".xz"))
	fmode = "r.xzdio";	/* Open with xz decompression. */
    else
	fmode = "r.ufdio";

    /* Open the stream. */
    fd = Fopen(pathname, fmode);
openthestream:
    if (fd == NULL || Ferror(fd)) {
	if (!GF_ISSET(SILENT))
	    fprintf(stderr, _("%s: Failed to open %s: %s\n"),
			__progname, pathname, Fstrerror(fd));
	rc = 2;
	goto exit;
    }

    /* Now grep the file */
    rc = pcregrep(fd, (filenames > FN_DEFAULT ||
	(filenames == FN_DEFAULT && !only_one_at_top))? pathname : NULL);

    if (fd != NULL)
	(void) rpmswAdd(&grep_readops, fdstat_op(fd, FDSTAT_READ));

exit:
    if (fd != NULL)
	xx = Fclose(fd);
    return rc;	/* Pass back the yield from pcregrep(). */
}

/*************************************************
 * Compile a single pattern.
 *
 * When the -F option has been used, this is called for each substring.
 * Otherwise it's called for each supplied pattern.
 *
 * @param pattern	the pattern string
 * @param filename	the file name (NULL for a command-line pattern)
 * @param count		pattern index (0 is single pattern)
 * @return		TRUE on success, FALSE after an error
 */
static BOOL
compile_single_pattern(const char *pattern,
		/*@null@*/ const char *filename, int count)
	/*@globals pattern_list, pattern_count, fileSystem @*/
	/*@modifies pattern_list, pattern_count, fileSystem @*/
{
    miRE mire;
    char buffer[MBUFTHIRD + 16];
    int xx;

    if (pattern_count >= MAX_PATTERN_COUNT) {
	fprintf(stderr, _("%s: Too many patterns (max %d)\n"), __progname,
		MAX_PATTERN_COUNT);
	return FALSE;
    }

    sprintf(buffer, "%s%.*s%s", prefix[(int)(grepFlags & 0x7)],
	MBUFTHIRD, pattern, suffix[(int)(grepFlags & 0x7)]);

    mire = pattern_list + pattern_count;
/*@-onlytrans@*/
    /* XXX initialize mire->{mode,tag,options,table}. */
    xx = mireSetCOptions(mire, grepMode, 0, 0, _mirePCREtables);

    if (!mireRegcomp(mire, buffer)) {
	pattern_count++;
	return TRUE;
    }
/*@=onlytrans@*/
    /* Handle compile errors */
    mire->erroff -= (int)strlen(prefix[(int)(grepFlags & 0x7)]);
    if (mire->erroff < 0)
	mire->erroff = 0;
    if (mire->erroff > (int)strlen(pattern))
	mire->erroff = (int)strlen(pattern);

    fprintf(stderr, _("%s: Error in"), __progname);
    if (filename == NULL)
	fprintf(stderr, _(" command-line %d"), count);
    else
	fprintf(stderr, _(" file:line %s:%d"), filename, count);
    fprintf(stderr, _(" regex at offset %d: %s\n"), mire->erroff, mire->errmsg);
    return FALSE;
}

/*************************************************
 * Compile one supplied pattern.
 *
 * When the -F option has been used, each string may be a list of strings,
 * separated by line breaks. They will be matched literally.
 *
 * @param pattern	the pattern string
 * @param filename	the file name, or NULL for a command-line pattern
 * @param count		pattern index (0 is single pattern)
 * @return		TRUE on success, FALSE after an error
 */
static BOOL
compile_pattern(const char *pattern, /*@null@*/ const char *filename, int count)
	/*@globals fileSystem @*/
	/*@modifies fileSystem @*/
{
    if (GF_ISSET(FIXED_STRINGS) != 0) {
	const char *eop = pattern + strlen(pattern);
	char buffer[MBUFTHIRD];
	for(;;) {
	    size_t ellength;
	    const char *p = end_of_line(pattern, eop, &ellength);
	    if (ellength == 0)
		return compile_single_pattern(pattern, filename, count);
	    sprintf(buffer, "%.*s", (int)(p - pattern - ellength), pattern);
	    pattern = p;
	    if (!compile_single_pattern(buffer, filename, count))
		return FALSE;
	}
    }
    else return compile_single_pattern(pattern, filename, count);
}

/**
 * Load patterns from files.
 * @param files		array of file names
 * @return		0 on success
 */
static int mireLoadPatternFiles(/*@null@*/ ARGV_t files)
	/*@globals h_errno, fileSystem, internalState @*/
	/*@modifies h_errno, fileSystem, internalState @*/
{
    const char *fn;
    int rc = -1;	/* assume failure */

    if (files != NULL)	/* note rc=0 return with no files to load. */
    while ((fn = *files++) != NULL) {
	char buffer[MBUFTHIRD];
	int linenumber;
	FD_t fd = NULL;
	FILE *fp;

	if (strcmp(fn, "-") == 0) {
	    fd = NULL;
	    fp = stdin;
	    fn = stdin_name;	/* XXX use the stdin display name */
	} else {
	    /* XXX .fpio is needed because of fgets(3) usage. */
	    fd = Fopen(fn, "r.fpio");
	    if (fd == NULL || Ferror(fd) || (fp = fdGetFILE(fd)) == NULL) {
		fprintf(stderr, _("%s: Failed to open %s: %s\n"),
				__progname, fn, Fstrerror(fd));
		if (fd != NULL) (void) Fclose(fd);
		fd = NULL;
		fp = NULL;
		goto exit;
	    }
	}

	linenumber = 0;
	while (fgets(buffer, MBUFTHIRD, fp) != NULL) {
	    char *se = buffer + (int)strlen(buffer);
	    while (se > buffer && xisspace((int)se[-1]))
		se--;
	    *se = '\0';
	    linenumber++;
	    /* Skip blank lines */
	    if (buffer[0] == '\0')	/*@innercontinue@*/ continue;
	    if (!compile_pattern(buffer, fn, linenumber))
		goto exit;
	}

	if (fd != NULL) {
	    (void) rpmswAdd(&grep_readops, fdstat_op(fd, FDSTAT_READ));
	    (void) Fclose(fd);
	    fd = NULL;
	}
    }
    rc = 0;

exit:
    return rc;
}

/* Options without a single-letter equivalent get a negative value. This can be
used to identify them. */

/**
 */
static void grepArgCallback(poptContext con,
                /*@unused@*/ enum poptCallbackReason reason,
                const struct poptOption * opt, const char * arg,
                /*@unused@*/ void * data)
	/*@globals color_string, dee_action, DEE_action, grepFlags, fileSystem @*/
	/*@modifies color_string, dee_action, DEE_action, grepFlags, fileSystem @*/
{
    /* XXX avoid accidental collisions with POPT_BIT_SET for flags */
    if (opt->arg == NULL)
    switch (opt->val) {

    case 'd':
	if (!strcmp(arg, "read")) dee_action = dee_READ;
	else if (!strcmp(arg, "recurse")) dee_action = dee_RECURSE;
	else if (!strcmp(arg, "skip")) dee_action = dee_SKIP;
	else {
	    fprintf(stderr, _("%s: Invalid value \"%s\" for -d\n"),
		__progname, arg);
	    /*@-exitarg@*/ exit(2); /*@=exitarg@*/
	    /*@notreached@*/
	}
	break;
    case 'D':
	if (!strcmp(arg, "read")) DEE_action = DEE_READ;
	else if (!strcmp(arg, "skip")) DEE_action = DEE_SKIP;
	else {
	    fprintf(stderr, _("%s: Invalid value \"%s\" for -D\n"),
		__progname, arg);
	    /*@-exitarg@*/ exit(2); /*@=exitarg@*/
	    /*@notreached@*/
	}
	break;
    case 'C':
	if (!strcmp(arg, "never"))
	    grepFlags &= ~GREP_FLAGS_COLOR;
	else if (!strcmp(arg, "always"))
	    grepFlags |= GREP_FLAGS_COLOR;
	else if (!strcmp(arg, "auto")) {
	    if (isatty(fileno(stdout)))
		grepFlags |= GREP_FLAGS_COLOR;
	    else
		grepFlags &= ~GREP_FLAGS_COLOR;
	} else {
	    fprintf(stderr, _("%s: Unknown color setting \"%s\"\n"),
		__progname, arg);
	    /*@-exitarg@*/ exit(2); /*@=exitarg@*/
	    /*@notreached@*/
	}
	color_string = _free(color_string);
	if (GF_ISSET(COLOR)) {
	    char *cs = getenv("PCREGREP_COLOUR");
	    if (cs == NULL) cs = getenv("PCREGREP_COLOR");
	    color_string = xstrdup(cs != NULL ? cs : "1;31");
	}
	break;

    case 'V':
#if defined(WITH_PCRE)
/*@-evalorderuncon -moduncon @*/
	fprintf(stderr, _("%s %s (PCRE version %s)\n"), __progname, VERSION, pcre_version());
/*@=evalorderuncon =moduncon @*/
#else
	fprintf(stderr, _("%s %s (without PCRE)\n"), __progname, VERSION);
#endif
	exit(0);
	/*@notreached@*/ break;
    default:
	fprintf(stderr, _("%s: Unknown option -%c\n"), __progname, opt->val);
	poptPrintUsage(con, stderr, 0);
	/*@-exitarg@*/ exit(2); /*@=exitarg@*/
	/*@notreached@*/ break;
    }
}

/**
 */
/*@+enumint@*/
/*@unchecked@*/
static struct poptOption optionsTable[] = {
/*@-type@*/ /* FIX: cast? */
 { NULL, '\0', POPT_ARG_CALLBACK | POPT_CBFLAG_INC_DATA | POPT_CBFLAG_CONTINUE,
        grepArgCallback, 0, NULL, NULL },
/*@=type@*/

  { "after-context", 'A', POPT_ARG_INT,		&after_context, 0,
	N_("set number of following context lines"), N_("=number") },
  { "before-context", 'B', POPT_ARG_INT,	&before_context, 0,
	N_("set number of prior context lines"), N_("=number") },
  { "context", 'C', POPT_ARG_INT,		&both_context, 0,
	N_("set number of context lines, before & after"), N_("=number") },
  { "count", 'c', POPT_BIT_SET,		&grepFlags, GREP_FLAGS_COUNT,
	N_("print only a count of matching lines per FILE"), NULL },
  { "color", '\0', POPT_ARG_STRING,		NULL, (int)'C',
	N_("matched text color option (auto|always|never)"), N_("=option") },
  { "colour", '\0', POPT_ARG_STRING|POPT_ARGFLAG_DOC_HIDDEN, NULL, (int)'C',
	N_("matched text colour option (auto|always|never)"), N_("=option") },
/* XXX HACK: there is a shortName option conflict with -D,--define */
  { "devices", 'D', POPT_ARG_STRING,		NULL, (int)'D',
	N_("device, FIFO, or socket action (read|skip)"), N_("=action") },
  { "directories", 'd',	POPT_ARG_STRING,	NULL, (int)'d',
	N_("directory action (read|skip|recurse)"), N_("=action") },
  { "regex", 'e', POPT_ARG_ARGV,		&patterns, 0,
	N_("specify pattern (may be used more than once)"), N_("(p)") },
  { "fixed_strings", 'F', POPT_BIT_SET,	&grepFlags, GREP_FLAGS_FIXED_STRINGS,
	N_("patterns are sets of newline-separated strings"), NULL },
  { "file", 'f', POPT_ARG_ARGV,		&pattern_filenames, 0,
	N_("read patterns from file (may be used more than once)"),
	N_("=path") },
  { "file-offsets", '\0', POPT_BIT_SET,	&grepFlags, GREP_FLAGS_FOFFSETS,
	N_("output file offsets, not text"), NULL },
  { "with-filename", 'H', POPT_ARG_VAL,	&filenames, FN_FORCE,
	N_("force the prefixing filename on output"), NULL },
  { "no-filename", 'h',	POPT_ARG_VAL,	&filenames, FN_NONE,
	N_("suppress the prefixing filename on output"), NULL },
  { "ignore-case", 'i',	POPT_BIT_SET,	&grepFlags, GREP_FLAGS_CASELESS,
	N_("ignore case distinctions"), NULL },
  { "files-with-matches", 'l', POPT_ARG_VAL,	&filenames, FN_ONLY,
	N_("print only FILE names containing matches"), NULL },
  { "files-without-match", 'L',	POPT_ARG_VAL,	&filenames, FN_NOMATCH_ONLY,
	N_("print only FILE names not containing matches"), NULL },
  { "label", '\0', POPT_ARG_STRING,		&stdin_name, 0,
	N_("set name for standard input"), N_("=name") },
  { "line-offsets", '\0', POPT_BIT_SET,	&grepFlags, (GREP_FLAGS_LOFFSETS|GREP_FLAGS_LNUMBER),
	N_("output line numbers and offsets, not text"), NULL },
  /* XXX TODO: --locale jiggery-pokery should be done env LC_ALL=C rpmgrep */
  { "locale", '\0', POPT_ARG_STRING,		&locale, 0,
	N_("use the named locale"), N_("=locale") },
  { "multiline", 'M', POPT_BIT_SET,	&grepFlags, GREP_FLAGS_MULTILINE,
	N_("run in multiline mode"), NULL },
  { "newline", 'N',	POPT_ARG_STRING,	&newline, 0,
	N_("set newline type (CR|LF|CRLF|ANYCRLF|ANY)"), N_("=type") },
  { "line-number", 'n',	POPT_BIT_SET,	&grepFlags, GREP_FLAGS_LNUMBER,
	N_("print line number with output lines"), NULL },
  { "only-matching", 'o', POPT_BIT_SET,	&grepFlags, GREP_FLAGS_ONLY_MATCHING,
	N_("show only the part of the line that matched"), NULL },
/* XXX HACK: there is a longName option conflict with --quiet */
  { "quiet", 'q', POPT_BIT_SET,		&grepFlags, GREP_FLAGS_QUIET,
	N_("suppress output, just set return code"), NULL },
  { "recursive", 'r',	POPT_ARG_VAL,		&dee_action, dee_RECURSE,
	N_("recursively scan sub-directories"), NULL },
  { "exclude", '\0', POPT_ARG_ARGV,		&exclude_patterns, 0,
	N_("exclude matching files when recursing (may be used more than once)"),
	N_("=pattern") },
  { "include", '\0', POPT_ARG_ARGV,		&include_patterns, 0,
	N_("include matching files when recursing (may be used more than once)"),
	N_("=pattern") },
  { "no-messages", 's',	POPT_BIT_SET,	&grepFlags, GREP_FLAGS_SILENT,
	N_("suppress error messages"), NULL },
  { "silent", '\0', POPT_BIT_SET|POPT_ARGFLAG_DOC_HIDDEN, &grepFlags, GREP_FLAGS_SILENT,
	N_("suppress error messages"), NULL },
  { "utf-8", 'u',	POPT_BIT_SET,	&grepFlags, GREP_FLAGS_UTF8,
	N_("use UTF-8 mode"), NULL },
/* XXX HACK: there is a longName option conflict with --version */
  { "version", 'V',	POPT_ARG_NONE,		NULL, (int)'V',
	N_("print version information and exit"), NULL },
/* XXX HACK: there is a shortName option conflict with -v, --verbose */
  { "invert-match", 'v', POPT_BIT_SET,	&grepFlags, GREP_FLAGS_INVERT,
	N_("select non-matching lines"), NULL },
  { "word-regex", 'w', POPT_BIT_SET,	&grepFlags, GREP_FLAGS_WORD_MATCH,
	N_("force patterns to match only as words") , N_("(p)") },
  { "line-regex", 'x', POPT_BIT_SET,	&grepFlags, GREP_FLAGS_LINE_MATCH,
	N_("force patterns to match only whole lines"), N_("(p)") },

  POPT_AUTOALIAS

 { NULL, '\0', POPT_ARG_INCLUDE_TABLE, rpmioAllPoptTable, 0,
	N_("Common options for all rpmio executables:"),
	NULL },

  POPT_AUTOHELP

  { NULL, (char)-1, POPT_ARG_INCLUDE_TABLE, NULL, 0,
	N_("\
Usage: rpmgrep [OPTION...] [PATTERN] [FILE1 FILE2 ...]\n\n\
  Search for PATTERN in each FILE or standard input.\n\
  PATTERN must be present if neither -e nor -f is used.\n\
  \"-\" can be used as a file name to mean STDIN.\n\
  All files are read as plain files, without any interpretation.\n\n\
Example: rpmgrep -i 'hello.*world' menu.h main.c\
") , NULL },

  { NULL, (char)-1, POPT_ARG_INCLUDE_TABLE, NULL, 0,
	N_("\
  When reading patterns from a file instead of using a command line option,\n\
  trailing white space is removed and blank lines are ignored.\n\
\n\
  With no FILEs, read standard input. If fewer than two FILEs given, assume -h.\n\
") , NULL },

  POPT_TABLEEND
};
/*@=enumint@*/

/*************************************************
 * Main program.
 * @return		0: match found, 1: no match, 2: error.
 */
int
main(int argc, char **argv)
	/*@globals __assert_program_name, after_context, before_context,
		color_string, dee_action,
 		exclude_patterns, excludeMire, grepFlags,
		include_patterns, includeMire, locale, newline,
		patterns, pattern_count, pattern_filenames, pattern_list,
		stdin_name,
 		rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
	/*@modifies __assert_program_name, after_context, before_context,
		color_string, dee_action,
 		exclude_patterns, excludeMire, grepFlags,
		include_patterns, includeMire, locale, newline,
		patterns, pattern_count, pattern_filenames, pattern_list,
		stdin_name,
 		rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
{
    poptContext optCon = rpmioInit(argc, argv, optionsTable);
    ARGV_t av = NULL;
    int ac = 0;
    int i = 0;		/* assume av[0] is 1st argument. */
    int rc = 1;		/* assume not found. */
    int j;
    int xx;

    xx = rpmswEnter(&grep_totalops, -1);

/*@-observertrans -readonlytrans@*/
    __progname = "pcregrep";	/* XXX HACK in expected name. */
/*@=observertrans =readonlytrans@*/


    if (stdin_name == NULL)
	stdin_name = xstrdup("(standard input)");

    av = poptGetArgs(optCon);
    ac = argvCount(av);

    /* If -C was used, its value is used as a default for -A and -B.  */
    if (both_context > 0) {
	if (after_context == 0) after_context = both_context;
	if (before_context == 0) before_context = both_context;
    }

    /*
     * Only one of --only-matching, --file-offsets, or --line-offsets is
     * permitted.  However, the latter two imply the --only-matching option.
     */
    if (((GF_ISSET(FOFFSETS) || GF_ISSET(LOFFSETS)) && GF_ISSET(ONLY_MATCHING))
     ||  (GF_ISSET(FOFFSETS) && GF_ISSET(LOFFSETS)))
    {
	fprintf(stderr,
_("%s: Cannot mix --only-matching, --file-offsets and/or --line-offsets\n"),
		__progname);
	poptPrintUsage(optCon, stderr, 0);
	goto errxit;
    }

    if (GF_ISSET(FOFFSETS) || GF_ISSET(LOFFSETS))
	grepFlags |= GREP_FLAGS_ONLY_MATCHING;

    /* Compile locale-specific PCRE tables. */
    if ((xx = mireSetLocale(NULL, locale)) != 0)
	goto errxit;

    /* Initialize global pattern options. */
    /* Interpret the newline type; the default settings are Unix-like. */
/*@-moduncon@*/	/* LCL: something fishy. */
    xx = mireSetGOptions(newline, GF_ISSET(CASELESS), GF_ISSET(MULTILINE),
		GF_ISSET(UTF8));
/*@=moduncon@*/
    if (xx != 0) {
	fprintf(stderr, _("%s: Invalid newline specifier \"%s\"\n"),
		__progname, (newline != NULL ? newline : "lf"));
	goto errxit;
    }

    /* Get memory to store the pattern and hints lists. */
    /* XXX FIXME: rpmmireNew needs to be used here. */
    pattern_list = xcalloc(MAX_PATTERN_COUNT, sizeof(*pattern_list));

    /*
     * If no patterns were provided by -e, and there is no file provided by -f,
     * the first argument is the one and only pattern, and it must exist.
     */
    {	int npatterns = argvCount(patterns);

	/*
	 * If no patterns were provided by -e, and no file was provided by -f,
	 * the first argument is the one and only pattern, and it must exist.
	 */
	if (npatterns == 0 && pattern_filenames == NULL) {
	    if (av == NULL|| i >= ac) {
		poptPrintUsage(optCon, stderr, 0);
		goto errxit;
	    }
	    xx = poptSaveString(&patterns, POPT_ARG_ARGV, av[i]);
	    i++;
	}

	/*
	 * Compile the patterns that were provided on the command line, either
	 * by multiple uses of -e or as a single unkeyed pattern.
	 */
	npatterns = argvCount(patterns);
	if (patterns != NULL)
	for (j = 0; j < npatterns; j++) {
	    if (!compile_pattern(patterns[j], NULL,
			(j == 0 && npatterns == 1)? 0 : j + 1))
		goto errxit;
	}
    }

    /* Compile the regular expressions that are provided from file(s). */
    if (mireLoadPatternFiles(pattern_filenames))
	goto errxit;

    /* Study the regular expressions, as we will be running them many times */
/*@-onlytrans@*/
    if (mireStudy(pattern_list, pattern_count))
	goto errxit;
/*@=onlytrans@*/

    /* If there are include or exclude patterns, compile them. */
/*@-compmempass@*/
    if (mireLoadPatterns(grepMode, 0, exclude_patterns, NULL,
		&excludeMire, &nexcludes))
    {
/*@-nullptrarith@*/
	miRE mire = excludeMire + (nexcludes - 1);
/*@=nullptrarith@*/
	fprintf(stderr, _("%s: Error in 'exclude' regex at offset %d: %s\n"),
			__progname, mire->erroff, mire->errmsg);
	goto errxit;
    }
/*@=compmempass@*/
/*@-compmempass@*/
    if (mireLoadPatterns(grepMode, 0, include_patterns, NULL,
		&includeMire, &nincludes))
    {
/*@-nullptrarith@*/
	miRE mire = includeMire + (nincludes - 1);
/*@=nullptrarith@*/
	fprintf(stderr, _("%s: Error in 'include' regex at offset %d: %s\n"),
			__progname, mire->erroff, mire->errmsg);
	goto errxit;
    }
/*@=compmempass@*/

    /* If there are no further arguments, do the business on stdin and exit. */
    if (i >= ac) {
	rc = grep_or_recurse("-", 0, 1);
    } else

    /*
     * Otherwise, work through the remaining arguments as files or directories.
     * Pass in the fact that there is only one argument at top level - this
     * suppresses the file name if the argument is not a directory and
     * filenames are not otherwise forced.
     */
    {	BOOL only_one_at_top = (i == ac -1);	/* Catch initial value of i */

	if (av != NULL)
	for (; i < ac; i++) {
	    int frc = grep_or_recurse(av[i], dee_action == dee_RECURSE,
			only_one_at_top);
	    if (frc > 1) rc = frc;
	    else if (frc == 0 && rc == 1) rc = 0;
	}
    }

exit:
/*@-statictrans@*/
    pattern_list = mireFreeAll(pattern_list, pattern_count);
    patterns = argvFree(patterns);
    excludeMire = mireFreeAll(excludeMire, nexcludes);
    exclude_patterns = argvFree(exclude_patterns);
    includeMire = mireFreeAll(includeMire, nincludes);
    include_patterns = argvFree(include_patterns);

    pattern_filenames = argvFree(pattern_filenames);

/*@-observertrans@*/
    color_string = _free(color_string);
    locale = _free(locale);
    newline = _free(newline);
    stdin_name = _free(stdin_name);
/*@=observertrans@*/
/*@=statictrans@*/

    xx = rpmswExit(&grep_totalops, 0);
    if (_rpmsw_stats) {
	rpmswPrint(" total:", &grep_totalops, NULL);
	rpmswPrint("  read:", &grep_readops, NULL);
    }

    optCon = rpmioFini(optCon);

    return rc;

errxit:
    rc = 2;
    goto exit;
}
