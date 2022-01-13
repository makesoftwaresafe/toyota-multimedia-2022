
/*-
 * Copyright (c) 1989, 1990, 1991, 1993, 1994
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Cimarron D. Taylor of the University of California, Berkeley.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef lint
char copyright[] =
"@(#) Copyright (c) 1989, 1990, 1991, 1993, 1994\n\
	The Regents of the University of California.  All rights reserved.\n";
#endif /* not lint */

#include "system.h"

#if defined(HAVE_SYS_ACL_H)
#include <sys/acl.h>
#endif
#include <sys/mount.h>

#include <langinfo.h>

#include <rpmio.h>
#include <rpmdir.h>
#include <fts.h>
#define	_MIRE_INTERNAL
#include <mire.h>
#include <ugid.h>		/* XXX HACK */
#define	user_from_uid(_a, _b)	uidToUname(_a)
#define	group_from_gid(_a, _b)	gidToGname(_a)

#include "debug.h"

#define	__unused	__attribute__((__unused__))

time_t now;			/* time find was run */
int dotfd;			/* starting directory */
int ftsoptions;			/* options for the ftsopen(3) call */
int isdeprecated;		/* using deprecated syntax */
int isdepth;			/* do directories on post-order visit */
int isoutput;			/* user specified output operator */
int issort;         		/* do hierarchies in lexicographical order */
int isxargs;			/* don't permit xargs delimiting chars */
int mindepth = -1, maxdepth = -1; /* minimum and maximum depth */
int regexp_flags = 0;		/* use the REG_BASIC "basic" regexp by default*/

FTS *tree;			/* pointer to top of FTS hierarchy */

extern char **environ;

struct timeb;		/* XXX #include <sys/timeb.h> ? */
extern time_t	 get_date(char *, struct timeb *);

/*==============================================================*/

/* forward declarations */
struct _plandata;
struct _option;

/* execute function */
typedef int exec_f(struct _plandata *, FTSENT *);
/* create function */
typedef	struct _plandata *creat_f(struct _option *, char ***);

/* function modifiers */
#define	F_NEEDOK	0x00000001	/* -ok vs. -exec */
#define	F_EXECDIR	0x00000002	/* -execdir vs. -exec */
#define F_TIME_A	0x00000004	/* one of -atime, -anewer, -newera* */
#define F_TIME_C	0x00000008	/* one of -ctime, -cnewer, -newerc* */
#define	F_TIME2_A	0x00000010	/* one of -newer?a */
#define	F_TIME2_C	0x00000020	/* one of -newer?c */
#define	F_TIME2_T	0x00000040	/* one of -newer?t */
#define F_MAXDEPTH	F_TIME_A	/* maxdepth vs. mindepth */
#define F_DEPTH		F_TIME_A	/* -depth n vs. -d */
/* command line function modifiers */
#define	F_EQUAL		0x00000000	/* [acm]min [acm]time inum links size */
#define	F_LESSTHAN	0x00000100
#define	F_GREATER	0x00000200
#define F_ELG_MASK	0x00000300
#define	F_ATLEAST	0x00000400	/* flags perm */
#define F_ANY		0x00000800	/* perm */
#define	F_MTMASK	0x00003000
#define	F_MTFLAG	0x00000000	/* fstype */
#define	F_MTTYPE	0x00001000
#define	F_MTUNKNOWN	0x00002000
#define	F_IGNCASE	0x00010000	/* iname ipath iregex */
#define	F_EXACTTIME	F_IGNCASE	/* -[acm]time units syntax */
#define F_EXECPLUS	0x00020000	/* -exec ... {} + */
#define	F_TIME_B	0x00040000	/* one of -Btime, -Bnewer, -newerB* */
#define	F_TIME2_B	0x00080000	/* one of -newer?B */
#define F_LINK		0x00100000	/* lname or ilname */

/* node definition */
typedef struct _plandata {
    struct _plandata *next;		/* next node */
    exec_f	*execute;		/* node evaluation function */
    int flags;				/* private flags */
    union {
	gid_t _g_data;			/* gid */
	ino_t _i_data;			/* inode */
	mode_t _m_data;			/* mode mask */
	struct {
	    unsigned long _f_flags;
	    unsigned long _f_notflags;
	} fl;
	nlink_t _l_data;		/* link count */
	short _d_data;			/* level depth (-1 to N) */
	off_t _o_data;			/* file size */
	time_t _t_data;			/* time value */
	uid_t _u_data;			/* uid */
	short _mt_data;			/* mount flags */
	struct _plandata *_p_data[2];	/* PLAN trees */
	struct _ex {
	    char **_e_argv;		/* argv array */
	    char **_e_orig;		/* original strings */
	    int *_e_len;		/* allocated length */
	    int _e_pbnum;		/* base num. of args. used */
	    int _e_ppos;		/* number of arguments used */
	    int _e_pnummax;		/* max. number of arguments */
	    int _e_psize;		/* number of bytes of args. */
	    int _e_pbsize;		/* base num. of bytes of args */
	    int _e_psizemax;		/* max num. of bytes of args */
	    struct _plandata *_e_next;	/* next F_EXECPLUS in tree */
	} ex;
	char *_a_data[2];		/* array of char pointers */
	char *_c_data;			/* char pointer */
	regex_t *_re_data;		/* regex */
    } p_un;
} PLAN;
#define	a_data	p_un._a_data
#define	c_data	p_un._c_data
#define	d_data	p_un._d_data
#define fl_flags	p_un.fl._f_flags
#define fl_notflags	p_un.fl._f_notflags
#define	g_data	p_un._g_data
#define	i_data	p_un._i_data
#define	l_data	p_un._l_data
#define	m_data	p_un._m_data
#define	mt_data	p_un._mt_data
#define	o_data	p_un._o_data
#define	p_data	p_un._p_data
#define	t_data	p_un._t_data
#define	u_data	p_un._u_data
#define	re_data	p_un._re_data
#define	e_argv	p_un.ex._e_argv
#define	e_orig	p_un.ex._e_orig
#define	e_len	p_un.ex._e_len
#define e_pbnum	p_un.ex._e_pbnum
#define e_ppos	p_un.ex._e_ppos
#define e_pnummax p_un.ex._e_pnummax
#define e_psize p_un.ex._e_psize
#define e_pbsize p_un.ex._e_pbsize
#define e_psizemax p_un.ex._e_psizemax
#define e_next p_un.ex._e_next

typedef struct _option {
    const char *name;		/* option name */
    creat_f *create;		/* create function */
    exec_f *execute;		/* execute function */
    int flags;
} OPTION;

/*==============================================================*/

/* XXX *BSD systems already have getmode(3) and setmode(3) */
#if defined(__linux__) || defined(__sun) || defined(__LCLINT__) || defined(__QNXNTO__)
#if !defined(HAVE_GETMODE) || !defined(HAVE_SETMODE)

#define	SET_LEN	6		/* initial # of bitcmd struct to malloc */
#define	SET_LEN_INCR 4		/* # of bitcmd structs to add as needed */

typedef struct bitcmd {
    char	cmd;
    char	cmd2;
    mode_t	bits;
} BITCMD;

#define	CMD2_CLR	0x01
#define	CMD2_SET	0x02
#define	CMD2_GBITS	0x04
#define	CMD2_OBITS	0x08
#define	CMD2_UBITS	0x10

#if !defined(HAVE_GETMODE)
/*
 * Given the old mode and an array of bitcmd structures, apply the operations
 * described in the bitcmd structures to the old mode, and return the new mode.
 * Note that there is no '=' command; a strict assignment is just a '-' (clear
 * bits) followed by a '+' (set bits).
 */
static mode_t
getmode(const void * bbox, mode_t omode)
	/*@*/
{
    const BITCMD *set;
    mode_t clrval, newmode, value;

    set = (const BITCMD *)bbox;
    newmode = omode;
    for (value = 0;; set++)
	switch(set->cmd) {
	/*
	 * When copying the user, group or other bits around, we "know"
	 * where the bits are in the mode so that we can do shifts to
	 * copy them around.  If we don't use shifts, it gets real
	 * grundgy with lots of single bit checks and bit sets.
	 */
	case 'u':
	    value = (newmode & S_IRWXU) >> 6;
	    goto common;

	case 'g':
	    value = (newmode & S_IRWXG) >> 3;
	    goto common;

	case 'o':
	    value = newmode & S_IRWXO;
common:	    if ((set->cmd2 & CMD2_CLR) != '\0') {
		clrval = (set->cmd2 & CMD2_SET) != '\0' ?  S_IRWXO : value;
		if ((set->cmd2 & CMD2_UBITS) != '\0')
		    newmode &= ~((clrval<<6) & set->bits);
		if ((set->cmd2 & CMD2_GBITS) != '\0')
		    newmode &= ~((clrval<<3) & set->bits);
		if ((set->cmd2 & CMD2_OBITS) != '\0')
		    newmode &= ~(clrval & set->bits);
	    }
	    if ((set->cmd2 & CMD2_SET) != '\0') {
		if ((set->cmd2 & CMD2_UBITS) != '\0')
		    newmode |= (value<<6) & set->bits;
		if ((set->cmd2 & CMD2_GBITS) != '\0')
		    newmode |= (value<<3) & set->bits;
		if ((set->cmd2 & CMD2_OBITS) != '\0')
		    newmode |= value & set->bits;
	    }
	    /*@switchbreak@*/ break;

	case '+':
	    newmode |= set->bits;
	    /*@switchbreak@*/ break;

	case '-':
	    newmode &= ~set->bits;
	    /*@switchbreak@*/ break;

	case 'X':
	    if (omode & (S_IFDIR|S_IXUSR|S_IXGRP|S_IXOTH))
			newmode |= set->bits;
	    /*@switchbreak@*/ break;

	case '\0':
	default:
#ifdef SETMODE_DEBUG
	    (void) printf("getmode:%04o -> %04o\n", omode, newmode);
#endif
	    return newmode;
	}
}
#endif	/* !defined(HAVE_GETMODE) */

#if !defined(HAVE_SETMODE)
#ifdef SETMODE_DEBUG
static void
dumpmode(BITCMD *set)
{
    for (; set->cmd; ++set)
	(void) printf("cmd: '%c' bits %04o%s%s%s%s%s%s\n",
	    set->cmd, set->bits, set->cmd2 ? " cmd2:" : "",
	    set->cmd2 & CMD2_CLR ? " CLR" : "",
	    set->cmd2 & CMD2_SET ? " SET" : "",
	    set->cmd2 & CMD2_UBITS ? " UBITS" : "",
	    set->cmd2 & CMD2_GBITS ? " GBITS" : "",
	    set->cmd2 & CMD2_OBITS ? " OBITS" : "");
}
#endif

#define	ADDCMD(a, b, c, d)					\
    if (set >= endset) {					\
	BITCMD *newset;						\
	setlen += SET_LEN_INCR;					\
	newset = realloc(saveset, sizeof(*newset) * setlen);	\
	if (newset == NULL) {					\
		if (saveset != NULL)				\
			free(saveset);				\
		saveset = NULL;					\
		return NULL;					\
	}							\
	set = newset + (set - saveset);				\
	saveset = newset;					\
	endset = newset + (setlen - 2);				\
    }								\
    set = addcmd(set, (a), (b), (c), (d))

#define	STANDARD_BITS	(S_ISUID|S_ISGID|S_IRWXU|S_IRWXG|S_IRWXO)

static BITCMD *
addcmd(/*@returned@*/ BITCMD *set, int op, int who, int oparg, unsigned mask)
	/*@modifies set @*/
{
    switch (op) {
    case '=':
	set->cmd = '-';
	set->bits = who ? who : (int) STANDARD_BITS;
	set++;

	op = (int)'+';
	/*@fallthrough@*/
    case '+':
    case '-':
    case 'X':
	set->cmd = (char)op;
	set->bits = (who ? (unsigned)who : mask) & oparg;
	break;

    case 'u':
    case 'g':
    case 'o':
	set->cmd = (char)op;
	if (who) {
	    set->cmd2 = (char)( ((who & S_IRUSR) ? CMD2_UBITS : 0) |
				((who & S_IRGRP) ? CMD2_GBITS : 0) |
				((who & S_IROTH) ? CMD2_OBITS : 0));
	    set->bits = (mode_t)~0;
	} else {
	    set->cmd2 =(char)(CMD2_UBITS | CMD2_GBITS | CMD2_OBITS);
	    set->bits = mask;
	}
	
	if (oparg == (int)'+')
	    set->cmd2 |= CMD2_SET;
	else if (oparg == (int)'-')
	    set->cmd2 |= CMD2_CLR;
	else if (oparg == (int)'=')
	    set->cmd2 |= CMD2_SET|CMD2_CLR;
	break;
    }
    return set + 1;
}

/*
 * Given an array of bitcmd structures, compress by compacting consecutive
 * '+', '-' and 'X' commands into at most 3 commands, one of each.  The 'u',
 * 'g' and 'o' commands continue to be separate.  They could probably be
 * compacted, but it's not worth the effort.
 */
static void
compress_mode(/*@out@*/ BITCMD *set)
	/*@modifies set @*/
{
    BITCMD *nset;
    int setbits, clrbits, Xbits, op;

    for (nset = set;;) {
	/* Copy over any 'u', 'g' and 'o' commands. */
	while ((op = (int)nset->cmd) != (int)'+' && op != (int)'-' && op != (int)'X') {
	    *set++ = *nset++;
	    if (!op)
		return;
	}

	for (setbits = clrbits = Xbits = 0;; nset++) {
	    if ((op = (int)nset->cmd) == (int)'-') {
		clrbits |= nset->bits;
		setbits &= ~nset->bits;
		Xbits &= ~nset->bits;
	    } else if (op == (int)'+') {
		setbits |= nset->bits;
		clrbits &= ~nset->bits;
		Xbits &= ~nset->bits;
	    } else if (op == (int)'X')
		Xbits |= nset->bits & ~setbits;
	    else
		/*@innerbreak@*/ break;
	}
	if (clrbits) {
	    set->cmd = '-';
	    set->cmd2 = '\0';
	    set->bits = clrbits;
	    set++;
	}
	if (setbits) {
	    set->cmd = '+';
	    set->cmd2 = '\0';
	    set->bits = setbits;
	    set++;
	}
	if (Xbits) {
	    set->cmd = 'X';
	    set->cmd2 = '\0';
	    set->bits = Xbits;
	    set++;
	}
    }
}

/*@-usereleased@*/
/*@null@*/
static void *
setmode(const char * p)
	/*@globals fileSystem @*/
	/*@modifies fileSystem @*/
{
    int perm, who;
    char op;
    BITCMD *set, *saveset, *endset;
    sigset_t sigset, sigoset;
    mode_t mask;
    int equalopdone = 0;
    int permXbits, setlen;
    long perml;

    if (!*p)
	return NULL;

    /*
     * Get a copy of the mask for the permissions that are mask relative.
     * Flip the bits, we want what's not set.  Since it's possible that
     * the caller is opening files inside a signal handler, protect them
     * as best we can.
     */
    (void) sigfillset(&sigset);
    (void) sigprocmask(SIG_BLOCK, &sigset, &sigoset);
    (void) umask(mask = umask(0));
    mask = ~mask;
    (void) sigprocmask(SIG_SETMASK, &sigoset, NULL);

    setlen = SET_LEN + 2;
	
    if ((set = malloc((unsigned)(sizeof(*set) * setlen))) == NULL)
	return NULL;
    saveset = set;
    endset = set + (setlen - 2);

    /*
     * If an absolute number, get it and return; disallow non-octal digits
     * or illegal bits.
     */
    if (isdigit(*p)) {
	perml = strtol(p, NULL, 8);
/*@-unrecog@*/
	if (perml < 0 || (perml & ~(STANDARD_BITS|S_ISTXT)))
/*@=unrecog@*/
	{
	    free(saveset);
	    return NULL;
	}
	perm = (int)(mode_t)perml;
	while (*++p != '\0')
	    if (*p < '0' || *p > '7') {
		free(saveset);
		return NULL;
	    }
	ADDCMD((int)'=', (int)(STANDARD_BITS|S_ISTXT), perm, (unsigned)mask);
	return saveset;
    }

    /*
     * Build list of structures to set/clear/copy bits as described by
     * each clause of the symbolic mode.
     */
    for (;;) {
	/* First, find out which bits might be modified. */
	for (who = 0;; ++p) {
	    switch (*p) {
	    case 'a':
		who |= STANDARD_BITS;
		/*@switchbreak@*/ break;
	    case 'u':
		who |= S_ISUID|S_IRWXU;
		/*@switchbreak@*/ break;
	    case 'g':
		who |= S_ISGID|S_IRWXG;
		/*@switchbreak@*/ break;
	    case 'o':
		who |= S_IRWXO;
		/*@switchbreak@*/ break;
	    default:
		goto getop;
	    }
	}

getop:	if ((op = *p++) != '+' && op != '-' && op != '=') {
	    free(saveset);
	    return NULL;
	}
	if (op == '=')
	    equalopdone = 0;

	who &= ~S_ISTXT;
	for (perm = 0, permXbits = 0;; ++p) {
	    switch (*p) {
	    case 'r':
		perm |= S_IRUSR|S_IRGRP|S_IROTH;
		/*@switchbreak@*/ break;
	    case 's':
		/*
		 * If specific bits where requested and
		 * only "other" bits ignore set-id.
		 */
		if (who == 0 || (who & ~S_IRWXO))
		    perm |= S_ISUID|S_ISGID;
		/*@switchbreak@*/ break;
	    case 't':
		/*
		 * If specific bits where requested and
		 * only "other" bits ignore sticky.
		 */
		if (who == 0 || (who & ~S_IRWXO)) {
		    who |= S_ISTXT;
		    perm |= S_ISTXT;
		}
		/*@switchbreak@*/ break;
	    case 'w':
		perm |= S_IWUSR|S_IWGRP|S_IWOTH;
		/*@switchbreak@*/ break;
	    case 'X':
		permXbits = (int)(S_IXUSR|S_IXGRP|S_IXOTH);
		/*@switchbreak@*/ break;
	    case 'x':
		perm |= S_IXUSR|S_IXGRP|S_IXOTH;
		/*@switchbreak@*/ break;
	    case 'u':
	    case 'g':
	    case 'o':
		/*
		 * When ever we hit 'u', 'g', or 'o', we have
		 * to flush out any partial mode that we have,
		 * and then do the copying of the mode bits.
		 */
		if (perm) {
		    ADDCMD((int)op, who, perm, (unsigned)mask);
		    perm = 0;
		}
		if (op == '=')
		    equalopdone = 1;
		if (op == '+' && permXbits) {
		    ADDCMD((int)'X', who, permXbits, (unsigned)mask);
		    permXbits = 0;
		}
		ADDCMD((int)*p, who, (int)op, (unsigned)mask);
		/*@switchbreak@*/ break;

	    default:
		/*
		 * Add any permissions that we haven't already
		 * done.
		 */
		if (perm || (op == '=' && !equalopdone)) {
		    if (op == '=')
			equalopdone = 1;
		    ADDCMD((int)op, who, perm, (unsigned)mask);
		    perm = 0;
		}
		if (permXbits) {
		    ADDCMD((int)'X', who, permXbits, (unsigned)mask);
		    permXbits = 0;
		}
		goto apply;
	    }
	}

apply:	if (!*p)
	    break;
	if (*p != ',')
	    goto getop;
	++p;
    }
    set->cmd = '\0';
#ifdef SETMODE_DEBUG
    (void) printf("Before compress_mode()\n");
    dumpmode(saveset);
#endif
    compress_mode(saveset);
#ifdef SETMODE_DEBUG
    (void) printf("After compress_mode()\n");
    dumpmode(saveset);
#endif
    return saveset;
}
/*@=usereleased@*/
#endif	/* !defined(HAVE_SETMODE) */
#endif	/* !defined(HAVE_GETMODE) || !defined(HAVE_SETMODE) */
#endif	/* __linux__ */

/*==============================================================*/

/*
 * brace_subst --
 *	Replace occurrences of {} in s1 with s2 and return the result string.
 */
static void
brace_subst(char *orig, char **store, char *path, int len)
{
    int plen = strlen(path);
    char ch, *p;

    for (p = *store; (ch = *orig) != '\0'; ++orig)
	if (ch == '{' && orig[1] == '}') {
	    while ((p - *store) + plen > len)
		*store = xrealloc(*store, len *= 2);
	    memmove(p, path, plen);
	    p += plen;
	    ++orig;
	} else
	    *p++ = ch;
    *p = '\0';
}

/*
 * ( expression ) functions --
 *
 *	True if expression is true.
 */
static int
f_expr(PLAN *plan, FTSENT *entry)
{
    PLAN *p;
    int state = 0;

    for (p = plan->p_data[0];
	p && (state = (p->execute)(p, entry)); p = p->next);
    return state;
}

/*
 * f_openparen and f_closeparen nodes are temporary place markers.  They are
 * eliminated during phase 2 of find_formplan() --- the '(' node is converted
 * to a f_expr node containing the expression and the ')' node is discarded.
 * The functions themselves are only used as constants.
 */

static int
f_openparen(PLAN *plan __unused, FTSENT *entry __unused)
{
    abort();
}

static int
f_closeparen(PLAN *plan __unused, FTSENT *entry __unused)
{
    abort();
}

/* c_openparen == c_simple */
/* c_closeparen == c_simple */

/*
 * AND operator. Since AND is implicit, no node is allocated.
 */
static PLAN *
c_and(OPTION *option __unused, char ***argvp __unused)
{
    return NULL;
}

/*
 * ! expression functions --
 *
 *	Negation of a primary; the unary NOT operator.
 */
static int
f_not(PLAN *plan, FTSENT *entry)
{
    PLAN *p;
    int state = 0;

    for (p = plan->p_data[0];
	p && (state = (p->execute)(p, entry)); p = p->next);
    return !state;
}

/* c_not == c_simple */

/*
 * expression -o expression functions --
 *
 *	Alternation of primaries; the OR operator.  The second expression is
 * not evaluated if the first expression is true.
 */
static int
f_or(PLAN *plan, FTSENT *entry)
{
    PLAN *p;
    int state = 0;

    for (p = plan->p_data[0];
	p && (state = (p->execute)(p, entry)); p = p->next);

    if (state)
	return 1;

    for (p = plan->p_data[1];
	p && (state = (p->execute)(p, entry)); p = p->next);
    return state;
}

/* c_or == c_simple */

/*
 * -false
 *
 *	Always false.
 */
static int
f_false(PLAN *plan __unused, FTSENT *entry __unused)
{
    return 0;
}

/* c_false == c_simple */

/*
 * -quit
 *
 *	Exits the program
 */
static int
f_quit(PLAN *plan __unused, FTSENT *entry __unused)
{
    exit(0);
}

/* c_quit == c_simple */

/*
 * yanknode --
 *	destructively removes the top from the plan
 */
static PLAN *
yanknode(PLAN **planp)
{
    PLAN *node;		/* top node removed from the plan */

    if ((node = (*planp)) == NULL)
	return NULL;
    (*planp) = (*planp)->next;
    node->next = NULL;
    return node;
}

/*
 * yankexpr --
 *	Removes one expression from the plan.  This is used mainly by
 *	paren_squish.  In comments below, an expression is either a
 *	simple node or a f_expr node containing a list of simple nodes.
 */
static PLAN *
yankexpr(PLAN **planp)
{
    PLAN *next;		/* temp node holding subexpression results */
    PLAN *node;		/* pointer to returned node or expression */
    PLAN *tail;		/* pointer to tail of subplan */
    PLAN *subplan;	/* pointer to head of ( ) expression */

    /* first pull the top node from the plan */
    if ((node = yanknode(planp)) == NULL)
	return NULL;

    /*
     * If the node is an '(' then we recursively slurp up expressions
     * until we find its associated ')'.  If it's a closing paren we
     * just return it and unwind our recursion; all other nodes are
     * complete expressions, so just return them.
     */
    if (node->execute == f_openparen)
	for (tail = subplan = NULL;;) {
	    if ((next = yankexpr(planp)) == NULL)
		errx(1, "(: missing closing ')'");
	    /*
	     * If we find a closing ')' we store the collected
	     * subplan in our '(' node and convert the node to
	     * a f_expr.  The ')' we found is ignored.  Otherwise,
	     * we just continue to add whatever we get to our
	     * subplan.
	     */
	    if (next->execute == f_closeparen) {
		if (subplan == NULL)
		    errx(1, "(): empty inner expression");
		node->p_data[0] = subplan;
		node->execute = f_expr;
		break;
	    } else {
		if (subplan == NULL)
		    tail = subplan = next;
		else {
		    tail->next = next;
		    tail = next;
		}
		tail->next = NULL;
	    }
	}
    return node;
}

/*
 * paren_squish --
 *	replaces "parenthesized" plans in our search plan with "expr" nodes.
 */
static PLAN *
paren_squish(PLAN *plan)
{
    PLAN *expr;		/* pointer to next expression */
    PLAN *tail = NULL;	/* pointer to tail of result plan */
    PLAN *result = NULL;	/* pointer to head of result plan */

    /*
     * the basic idea is to have yankexpr do all our work and just
     * collect its results together.
     */
    while ((expr = yankexpr(&plan)) != NULL) {
	/*
	 * if we find an unclaimed ')' it means there is a missing
	 * '(' someplace.
	 */
	if (expr->execute == f_closeparen)
	    errx(1, "): no beginning '('");

	/* add the expression to our result plan */
	if (result == NULL)
	    tail = result = expr;
	else {
	    tail->next = expr;
	    tail = expr;
	}
	tail->next = NULL;
    }
    return result;
}

/*
 * not_squish --
 *	compresses "!" expressions in our search plan.
 */
static PLAN *
not_squish(PLAN *plan)
{
    PLAN *next;		/* next node being processed */
    PLAN *node;		/* temporary node used in f_not processing */
    PLAN *tail = NULL;	/* pointer to tail of result plan */
    PLAN *result = NULL;	/* pointer to head of result plan */

    while ((next = yanknode(&plan))) {
	/*
	 * if we encounter a ( expression ) then look for nots in
	 * the expr subplan.
	 */
	if (next->execute == f_expr)
	    next->p_data[0] = not_squish(next->p_data[0]);

	/*
	 * if we encounter a not, then snag the next node and place
	 * it in the not's subplan.  As an optimization we compress
	 * several not's to zero or one not.
	 */
	if (next->execute == f_not) {
	    int notlevel = 1;

	    node = yanknode(&plan);
	    while (node != NULL && node->execute == f_not) {
		++notlevel;
		node = yanknode(&plan);
	    }
	    if (node == NULL)
		errx(1, "!: no following expression");
	    if (node->execute == f_or)
		errx(1, "!: nothing between ! and -o");
	    /*
	     * If we encounter ! ( expr ) then look for nots in
	     * the expr subplan.
	     */
	    if (node->execute == f_expr)
		node->p_data[0] = not_squish(node->p_data[0]);
	    if (notlevel % 2 != 1)
		next = node;
	    else
		next->p_data[0] = node;
	}

	/* add the node to our result plan */
	if (result == NULL)
	    tail = result = next;
	else {
	    tail->next = next;
	    tail = next;
	}
	tail->next = NULL;
    }
    return result;
}

/*
 * or_squish --
 *	compresses -o expressions in our search plan.
 */
static PLAN *
or_squish(PLAN *plan)
{
    PLAN *next;		/* next node being processed */
    PLAN *tail = NULL;	/* pointer to tail of result plan */
    PLAN *result = NULL;	/* pointer to head of result plan */

    while ((next = yanknode(&plan)) != NULL) {
	/*
	 * if we encounter a ( expression ) then look for or's in
	 * the expr subplan.
	 */
	if (next->execute == f_expr)
	    next->p_data[0] = or_squish(next->p_data[0]);

	/* if we encounter a not then look for or's in the subplan */
	if (next->execute == f_not)
	    next->p_data[0] = or_squish(next->p_data[0]);

	/*
	 * if we encounter an or, then place our collected plan in the
	 * or's first subplan and then recursively collect the
	 * remaining stuff into the second subplan and return the or.
	 */
	if (next->execute == f_or) {
	    if (result == NULL)
		errx(1, "-o: no expression before -o");
	    next->p_data[0] = result;
	    next->p_data[1] = or_squish(plan);
	    if (next->p_data[1] == NULL)
		errx(1, "-o: no expression after -o");
	    return next;
	}

	/* add the node to our result plan */
	if (result == NULL)
	    tail = result = next;
	else {
	    tail->next = next;
	    tail = next;
	}
	tail->next = NULL;
    }
    return result;
}

/* Derived from the print routines in the ls(1) source code. */

static void
printlink(char *name)
{
    int lnklen;
    char path[MAXPATHLEN];

    if ((lnklen = readlink(name, path, MAXPATHLEN - 1)) == -1) {
	warn("%s", name);
	return;
    }
    path[lnklen] = '\0';
    (void)printf(" -> %s", path);
}

static void
printtime(time_t ftime)
{
    char longstring[80];
    static time_t lnow;
    const char *format;
#if defined(D_MD_ORDER)
    static int d_first = -1;

    if (d_first < 0)
	d_first = (*nl_langinfo(D_MD_ORDER) == 'd');
#else
    static int d_first = 0;
#endif
    if (lnow == 0)
	lnow = time(NULL);

#define	SIXMONTHS	((365 / 2) * 86400)
    if (ftime + SIXMONTHS > lnow && ftime < lnow + SIXMONTHS)
	/* mmm dd hh:mm || dd mmm hh:mm */
	format = d_first ? "%e %b %R " : "%b %e %R ";
    else
	/* mmm dd  yyyy || dd mmm  yyyy */
	format = d_first ? "%e %b  %Y " : "%b %e  %Y ";
    strftime(longstring, sizeof(longstring), format, localtime(&ftime));
    fputs(longstring, stdout);
}

#if !defined(HAVE_STRUCT_STAT_ST_FLAGS)	/* XXX HACK */
static int strmode(mode_t mode, char *perms)
{
    strcpy(perms, "----------");
   
    if (S_ISREG(mode)) 
	perms[0] = '-';
    else if (S_ISDIR(mode)) 
	perms[0] = 'd';
    else if (S_ISLNK(mode))
	perms[0] = 'l';
    else if (S_ISFIFO(mode)) 
	perms[0] = 'p';
    /*@-unrecog@*/
    else if (S_ISSOCK(mode)) 
	perms[0] = 's';
    /*@=unrecog@*/
    else if (S_ISCHR(mode))
	perms[0] = 'c';
    else if (S_ISBLK(mode))
	perms[0] = 'b';
    else
	perms[0] = '?';

    if (mode & S_IRUSR) perms[1] = 'r';
    if (mode & S_IWUSR) perms[2] = 'w';
    if (mode & S_IXUSR) perms[3] = 'x';
 
    if (mode & S_IRGRP) perms[4] = 'r';
    if (mode & S_IWGRP) perms[5] = 'w';
    if (mode & S_IXGRP) perms[6] = 'x';

    if (mode & S_IROTH) perms[7] = 'r';
    if (mode & S_IWOTH) perms[8] = 'w';
    if (mode & S_IXOTH) perms[9] = 'x';

    if (mode & S_ISUID)
	perms[3] = ((mode & S_IXUSR) ? 's' : 'S'); 

    if (mode & S_ISGID)
	perms[6] = ((mode & S_IXGRP) ? 's' : 'S'); 

    if (mode & S_ISVTX)
	perms[9] = ((mode & S_IXOTH) ? 't' : 'T');

    return 0;
}
#endif

static void
printlong(char *name, char *accpath, struct stat *sb)
{
    static int ugwidth = 8;
    char modep[15];

    (void)printf("%10lu %8lld ", (unsigned long) sb->st_ino, (long long)sb->st_blocks);
    (void)strmode(sb->st_mode, modep);
    (void)printf("%s %3u %-*s %-*s ", modep, (unsigned)sb->st_nlink,
	    ugwidth, user_from_uid(sb->st_uid, 0),
	    ugwidth, group_from_gid(sb->st_gid, 0));

    if (S_ISCHR(sb->st_mode) || S_ISBLK(sb->st_mode))
	(void)printf("%3d, %3d ", major(sb->st_rdev), minor(sb->st_rdev));
    else
	(void)printf("%8lld ", (long long)sb->st_size);
    printtime(sb->st_mtime);
    (void)printf("%s", name);
    if (S_ISLNK(sb->st_mode))
	printlink(accpath);
    (void)putchar('\n');
}

/*
 * queryuser --
 *	print a message to standard error and then read input from standard
 *	input. If the input is an affirmative response (according to the
 *	current locale) then 1 is returned.
 */
static int
queryuser(char *argv[])
{
    char *p, resp[256];

    (void)fprintf(stderr, "\"%s", *argv);
    while (*++argv)
	(void)fprintf(stderr, " %s", *argv);
    (void)fprintf(stderr, "\"? ");
    (void)fflush(stderr);

    if (fgets(resp, sizeof(resp), stdin) == NULL)
	*resp = '\0';
    if ((p = strchr(resp, '\n')) != NULL)
	*p = '\0';
    else {
	(void)fprintf(stderr, "\n");
	(void)fflush(stderr);
    }
#if defined(__APPLE__) || defined(__sun)
    return ((resp[0] == 'Y' || resp[0] == 'y') ? 1 : 0);
#else
    return (rpmatch(resp) == 1);
#endif
}

/*==============================================================*/

static PLAN *lastexecplus = NULL;

#define	COMPARE(a, b) do {						\
	switch (plan->flags & F_ELG_MASK) {				\
	case F_EQUAL:							\
		return (a == b);					\
	case F_LESSTHAN:						\
		return (a < b);						\
	case F_GREATER:							\
		return (a > b);						\
	default:							\
		abort();						\
	}								\
} while(0)

static PLAN *
palloc(OPTION *option)
{
    PLAN *new = xmalloc(sizeof(*new));

    new->execute = option->execute;
    new->flags = option->flags;
    new->next = NULL;
    return new;
}

/*
 * find_parsenum --
 *	Parse a string of the form [+-]# and return the value.
 */
static long long
find_parsenum(PLAN *plan, const char *option, char *vp, char *endch)
{
    long long value;
    char *endchar, *str;	/* Pointer to character ending conversion. */

    /* Determine comparison from leading + or -. */
    str = vp;
    switch (*str) {
    case '+':
	++str;
	plan->flags |= F_GREATER;
	break;
    case '-':
	++str;
	plan->flags |= F_LESSTHAN;
	break;
    default:
	plan->flags |= F_EQUAL;
	break;
    }

    /*
     * Convert the string with strtoq().  Note, if strtoq() returns zero
     * and endchar points to the beginning of the string we know we have
     * a syntax error.
     */
#if defined(__sun)
    value = strtoll(str, &endchar, 10);
#else
    value = strtoq(str, &endchar, 10);
#endif
    if (value == 0 && endchar == str)
	errx(1, "%s: %s: illegal numeric value", option, vp);
    if (endchar[0] && endch == NULL)
	errx(1, "%s: %s: illegal trailing character", option, vp);
    if (endch)
	*endch = endchar[0];
    return value;
}

/*
 * find_parsetime --
 *	Parse a string of the form [+-]([0-9]+[smhdw]?)+ and return the value.
 */
static long long
find_parsetime(PLAN *plan, const char *option, char *vp)
{
    long long secs, value;
    char * str = vp;
    char *unit;		/* Pointer to character ending conversion. */

    /* Determine comparison from leading + or -. */
    switch (*str) {
    case '+':
	++str;
	plan->flags |= F_GREATER;
	break;
    case '-':
	++str;
	plan->flags |= F_LESSTHAN;
	break;
    default:
	plan->flags |= F_EQUAL;
	break;
    }

#if defined(__sun)
    value = strtoll(str, &unit, 10);
#else
    value = strtoq(str, &unit, 10);
#endif
    if (value == 0 && unit == str) {
	errx(1, "%s: %s: illegal time value", option, vp);
	/* NOTREACHED */
    }
    if (*unit == '\0')
	return value;

    /* Units syntax. */
    secs = 0;
    for (;;) {
	switch(*unit) {
	case 's':	/* seconds */
	    secs += value;
	    break;
	case 'm':	/* minutes */
	    secs += value * 60;
	    break;
	case 'h':	/* hours */
	    secs += value * 3600;
	    break;
	case 'd':	/* days */
	    secs += value * 86400;
	    break;
	case 'w':	/* weeks */
	    secs += value * 604800;
	    break;
	default:
	    errx(1, "%s: %s: bad unit '%c'", option, vp, *unit);
	    /* NOTREACHED */
	}
	str = unit + 1;
	if (*str == '\0')	/* EOS */
	    break;
#if defined(__sun)
	value = strtoll(str, &unit, 10);
#else
	value = strtoq(str, &unit, 10);
#endif
	if (value == 0 && unit == str) {
	    errx(1, "%s: %s: illegal time value", option, vp);
	    /* NOTREACHED */
	}
	if (*unit == '\0') {
	    errx(1, "%s: %s: missing trailing unit", option, vp);
	    /* NOTREACHED */
	}
    }
    plan->flags |= F_EXACTTIME;
    return secs;
}

/*
 * nextarg --
 *	Check that another argument still exists, return a pointer to it,
 *	and increment the argument vector pointer.
 */
static char *
nextarg(OPTION *option, char ***argvp)
{
    char *arg;

    if ((arg = **argvp) == 0)
	errx(1, "%s: requires additional arguments", option->name);
    (*argvp)++;
    return arg;
} /* nextarg() */

/*
 * The value of n for the inode times (atime, birthtime, ctime, mtime) is a
 * range, i.e. n matches from (n - 1) to n 24 hour periods.  This interacts
 * with -n, such that "-mtime -1" would be less than 0 days, which isn't what
 * the user wanted.  Correct so that -1 is "less than 1".
 */
#define	TIME_CORRECT(p) \
	if (((p)->flags & F_ELG_MASK) == F_LESSTHAN) \
		++((p)->t_data);

/*
 * -[acm]min n functions --
 *
 *    True if the difference between the
 *		file access time (-amin)
 *		file birth time (-Bmin)
 *		last change of file status information (-cmin)
 *		file modification time (-mmin)
 *    and the current time is n min periods.
 */
static int
f_Xmin(PLAN *plan, FTSENT *entry)
{
    if (plan->flags & F_TIME_C) {
	COMPARE((now - entry->fts_statp->st_ctime +
	    60 - 1) / 60, plan->t_data);
    } else if (plan->flags & F_TIME_A) {
	COMPARE((now - entry->fts_statp->st_atime +
	    60 - 1) / 60, plan->t_data);
    } else if (plan->flags & F_TIME_B) {
	COMPARE((now - entry->fts_statp->st_birthtime +
	    60 - 1) / 60, plan->t_data);
    } else {
	COMPARE((now - entry->fts_statp->st_mtime +
	    60 - 1) / 60, plan->t_data);
    }
}

static PLAN *
c_Xmin(OPTION *option, char ***argvp)
{
    char * nmins = nextarg(option, argvp);
    PLAN * new = palloc(option);

    ftsoptions &= ~FTS_NOSTAT;

    new->t_data = find_parsenum(new, option->name, nmins, NULL);
    TIME_CORRECT(new);
    return new;
}

/*
 * -[acm]time n functions --
 *
 *	True if the difference between the
 *		file access time (-atime)
 *		file birth time (-Btime)
 *		last change of file status information (-ctime)
 *		file modification time (-mtime)
 *	and the current time is n 24 hour periods.
 */

static int
f_Xtime(PLAN *plan, FTSENT *entry)
{
    time_t xtime;

    if (plan->flags & F_TIME_A)
	xtime = entry->fts_statp->st_atime;
    else if (plan->flags & F_TIME_B)
	xtime = entry->fts_statp->st_birthtime;
    else if (plan->flags & F_TIME_C)
	xtime = entry->fts_statp->st_ctime;
    else
	xtime = entry->fts_statp->st_mtime;

    if (plan->flags & F_EXACTTIME)
	COMPARE(now - xtime, plan->t_data);
    else
	COMPARE((now - xtime + 86400 - 1) / 86400, plan->t_data);
}

static PLAN *
c_Xtime(OPTION *option, char ***argvp)
{
    char * value = nextarg(option, argvp);
    PLAN * new = palloc(option);

    ftsoptions &= ~FTS_NOSTAT;

    new->t_data = find_parsetime(new, option->name, value);
    if (!(new->flags & F_EXACTTIME))
	TIME_CORRECT(new);
    return new;
}

/*
 * -maxdepth/-mindepth n functions --
 *
 *        Does the same as -prune if the level of the current file is
 *        greater/less than the specified maximum/minimum depth.
 *
 *        Note that -maxdepth and -mindepth are handled specially in
 *        find_execute() so their f_* functions are set to f_always_true().
 */
static PLAN *
c_mXXdepth(OPTION *option, char ***argvp)
{
    char * dstr = nextarg(option, argvp);
    PLAN * new = palloc(option);

    if (dstr[0] == '-')
	/* all other errors handled by find_parsenum() */
	errx(1, "%s: %s: value must be positive", option->name, dstr);

    if (option->flags & F_MAXDEPTH)
	maxdepth = find_parsenum(new, option->name, dstr, NULL);
    else
	mindepth = find_parsenum(new, option->name, dstr, NULL);
    return new;
}

/*
 * -acl function --
 *
 *	Show files with EXTENDED ACL attributes.
 */
static int
f_acl(PLAN *plan __unused, FTSENT *entry)
{
    int match = 0;
#if defined(HAVE_SYS_ACL_H)
    int entries;
    acl_entry_t ae;
    acl_t facl;

    if (S_ISLNK(entry->fts_statp->st_mode))
	return 0;
#if defined(_PC_ACL_EXTENDED)	/* XXX HACK */
    if ((match = pathconf(entry->fts_accpath, _PC_ACL_EXTENDED)) <= 0) {
	if (match < 0 && errno != EINVAL)
	    warn("%s", entry->fts_accpath);
	else
	    return 0;
    }
#endif
    match = 0;
    if ((facl = acl_get_file(entry->fts_accpath,ACL_TYPE_ACCESS)) != NULL) {
	if (acl_get_entry(facl, ACL_FIRST_ENTRY, &ae) == 1) {
	    /*
	     * POSIX.1e requires that ACLs of type ACL_TYPE_ACCESS
	     * must have at least three entries (owner, group,
	     * other).
	     */
	    entries = 1;
	    while (acl_get_entry(facl, ACL_NEXT_ENTRY, &ae) == 1) {
		if (++entries > 3) {
		    match = 1;
		    break;
		}
	    }
	}
	acl_free(facl);
    } else
	warn("%s", entry->fts_accpath);
#endif
    return match;
}

static PLAN *
c_acl(OPTION *option, char ***argvp __unused)
{
    PLAN * new = palloc(option);
    ftsoptions &= ~FTS_NOSTAT;
    return new;
}

/*
 * -delete functions --
 *
 *	True always.  Makes its best shot and continues on regardless.
 */
static int
f_delete(PLAN *plan __unused, FTSENT *entry)
{
    /* ignore these from fts */
    if (strcmp(entry->fts_accpath, ".") == 0
     || strcmp(entry->fts_accpath, "..") == 0)
	return 1;

    /* sanity check */
    if (isdepth == 0			/* depth off */
     || (ftsoptions & FTS_NOSTAT))	/* not stat()ing */
	errx(1, "-delete: insecure options got turned on");

    if (!(ftsoptions & FTS_PHYSICAL)	/* physical off */
     || (ftsoptions & FTS_LOGICAL))	/* or finally, logical on */
	errx(1, "-delete: forbidden when symlinks are followed");

    /* Potentially unsafe - do not accept relative paths whatsoever */
    if (strchr(entry->fts_accpath, '/') != NULL)
	errx(1, "-delete: %s: relative path potentially not safe",
		entry->fts_accpath);

#if defined(HAVE_STRUCT_STAT_ST_FLAGS)
    /* Turn off user immutable bits if running as root */
    if ((entry->fts_statp->st_flags & (UF_APPEND|UF_IMMUTABLE))
     && !(entry->fts_statp->st_flags & (SF_APPEND|SF_IMMUTABLE))
     && geteuid() == 0)
	Lchflags(entry->fts_accpath,
	       entry->fts_statp->st_flags &= ~(UF_APPEND|UF_IMMUTABLE));
#endif

    /* rmdir directories, unlink everything else */
    if (S_ISDIR(entry->fts_statp->st_mode)) {
	if (Rmdir(entry->fts_accpath) < 0 && errno != ENOTEMPTY)
	    warn("-delete: rmdir(%s)", entry->fts_path);
    } else {
	if (Unlink(entry->fts_accpath) < 0)
	    warn("-delete: unlink(%s)", entry->fts_path);
    }

    /* "succeed" */
    return 1;
}

static PLAN *
c_delete(OPTION *option, char ***argvp __unused)
{
    PLAN * new = palloc(option);
    ftsoptions &= ~FTS_NOSTAT;		/* no optimise */
    isoutput = 1;			/* possible output */
    isdepth = 1;			/* -depth implied */
    return new;
}


/*
 * always_true --
 *
 *	Always true, used for -maxdepth, -mindepth, -xdev, -follow, and -true
 */
static int
f_always_true(PLAN *plan __unused, FTSENT *entry __unused)
{
    return 1;
}

/*
 * -depth functions --
 *
 *	With argument: True if the file is at level n.
 *	Without argument: Always true, causes descent of the directory hierarchy
 *	to be done so that all entries in a directory are acted on before the
 *	directory itself.
 */
static int
f_depth(PLAN *plan, FTSENT *entry)
{
    if (plan->flags & F_DEPTH)
	COMPARE(entry->fts_level, plan->d_data);
    else
	return 1;
}

static PLAN *
c_depth(OPTION *option, char ***argvp)
{
    char * str = **argvp;
    PLAN * new = palloc(option);

    if (str && !(new->flags & F_DEPTH)) {
	/* skip leading + or - */
	if (*str == '+' || *str == '-')
	    str++;
	/* skip sign */
	if (*str == '+' || *str == '-')
	    str++;
	if (isdigit(*str))
	    new->flags |= F_DEPTH;
    }

    if (new->flags & F_DEPTH) {	/* -depth n */
	char *ndepth;

	ndepth = nextarg(option, argvp);
	new->d_data = find_parsenum(new, option->name, ndepth, NULL);
    } else {			/* -d */
	isdepth = 1;
    }

    return new;
}
 
/*
 * -empty functions --
 *
 *	True if the file or directory is empty
 */
static int
f_empty(PLAN *plan __unused, FTSENT *entry)
{
    if (S_ISREG(entry->fts_statp->st_mode) && entry->fts_statp->st_size == 0)
	return 1;
    if (S_ISDIR(entry->fts_statp->st_mode)) {
	struct dirent *dp;
	int empty;
	DIR *dir;

	empty = 1;
	dir = Opendir(entry->fts_accpath);
	if (dir == NULL)
	    err(1, "%s", entry->fts_accpath);
	for (dp = Readdir(dir); dp; dp = readdir(dir))
	    if (dp->d_name[0] != '.' ||
		(dp->d_name[1] != '\0' &&
		     (dp->d_name[1] != '.' || dp->d_name[2] != '\0')))
	    {
		empty = 0;
		break;
	    }
	Closedir(dir);
	return empty;
    }
    return 0;
}

static PLAN *
c_empty(OPTION *option, char ***argvp __unused)
{
    PLAN * new = palloc(option);
    ftsoptions &= ~FTS_NOSTAT;
    return new;
}

/*
 * [-exec | -execdir | -ok] utility [arg ... ] ; functions --
 *
 *	True if the executed utility returns a zero value as exit status.
 *	The end of the primary expression is delimited by a semicolon.  If
 *	"{}" occurs anywhere, it gets replaced by the current pathname,
 *	or, in the case of -execdir, the current basename (filename
 *	without leading directory prefix). For -exec and -ok,
 *	the current directory for the execution of utility is the same as
 *	the current directory when the find utility was started, whereas
 *	for -execdir, it is the directory the file resides in.
 *
 *	The primary -ok differs from -exec in that it requests affirmation
 *	of the user before executing the utility.
 */
static int
f_exec(PLAN *plan, FTSENT *entry)
{
    int cnt;
    pid_t pid;
    int status;
    char *file;

    if (entry == NULL && plan->flags & F_EXECPLUS) {
	if (plan->e_ppos == plan->e_pbnum)
	    return 1;
	plan->e_argv[plan->e_ppos] = NULL;
	goto doexec;
    }
assert(entry);	/* XXX coverity #1009103 */

    /* XXX - if file/dir ends in '/' this will not work -- can it? */
    if ((plan->flags & F_EXECDIR) && (file = strrchr(entry->fts_path, '/')))
	file++;
    else
	file = entry->fts_path;

    if (plan->flags & F_EXECPLUS) {
	if ((plan->e_argv[plan->e_ppos] = strdup(file)) == NULL)
	    err(1, NULL);
	plan->e_len[plan->e_ppos] = strlen(file);
	plan->e_psize += plan->e_len[plan->e_ppos];
	if (++plan->e_ppos < plan->e_pnummax
	 && plan->e_psize < plan->e_psizemax)
	    return 1;
	plan->e_argv[plan->e_ppos] = NULL;
    } else {
	for (cnt = 0; plan->e_argv[cnt]; ++cnt)
	    if (plan->e_len[cnt])
		brace_subst(plan->e_orig[cnt], &plan->e_argv[cnt], file,
			    plan->e_len[cnt]);
    }

doexec:
    if ((plan->flags & F_NEEDOK) && !queryuser(plan->e_argv))
	    return 0;

    /* make sure find output is interspersed correctly with subprocesses */
    fflush(stdout);
    fflush(stderr);

    switch (pid = fork()) {
    case -1:
	err(1, "fork");
	/* NOTREACHED */
    case 0:
	/* change dir back from where we started */
	if (!(plan->flags & F_EXECDIR) && fchdir(dotfd)) {
	    warn("chdir");
	    _exit(1);
	}
	execvp(plan->e_argv[0], plan->e_argv);
	warn("%s", plan->e_argv[0]);
	_exit(1);
    }
    if (plan->flags & F_EXECPLUS) {
	while (--plan->e_ppos >= plan->e_pbnum)
	    free(plan->e_argv[plan->e_ppos]);
	plan->e_ppos = plan->e_pbnum;
	plan->e_psize = plan->e_pbsize;
    }
    pid = waitpid(pid, &status, 0);
    return (pid != -1 && WIFEXITED(status) && !WEXITSTATUS(status));
}

/*
 * c_exec, c_execdir, c_ok --
 *	build three parallel arrays, one with pointers to the strings passed
 *	on the command line, one with (possibly duplicated) pointers to the
 *	argv array, and one with integer values that are lengths of the
 *	strings, but also flags meaning that the string has to be massaged.
 */
static PLAN *
c_exec(OPTION *option, char ***argvp)
{
    PLAN * new = palloc(option);
    long argmax;
    int cnt, i;
    char **argv, **ap, **ep, *p;

#if 0	/* XXX - was in c_execdir, but seems unnecessary!? */
    ftsoptions &= ~FTS_NOSTAT;
#endif
    isoutput = 1;

    for (ap = argv = *argvp;; ++ap) {
	if (!*ap)
	    errx(1, "%s: no terminating \";\" or \"+\"", option->name);
	if (**ap == ';')
	    break;
	if (**ap == '+' && ap != argv && strcmp(*(ap - 1), "{}") == 0) {
	    new->flags |= F_EXECPLUS;
	    break;
	}
    }

    if (ap == argv)
	errx(1, "%s: no command specified", option->name);

    cnt = ap - *argvp + 1;
    if (new->flags & F_EXECPLUS) {
	new->e_ppos = new->e_pbnum = cnt - 2;
	if ((argmax = sysconf(_SC_ARG_MAX)) == -1) {
	    warn("sysconf(_SC_ARG_MAX)");
	    argmax = _POSIX_ARG_MAX;
	}
	argmax -= 1024;
	for (ep = environ; *ep != NULL; ep++)
	    argmax -= strlen(*ep) + 1 + sizeof(*ep);
	argmax -= 1 + sizeof(*ep);
	new->e_pnummax = argmax / 16;
	argmax -= sizeof(char *) * new->e_pnummax;
	if (argmax <= 0)
	    errx(1, "no space for arguments");
	new->e_psizemax = argmax;
	new->e_pbsize = 0;
	cnt += new->e_pnummax + 1;
	new->e_next = lastexecplus;
	lastexecplus = new;
    }
    new->e_argv = xmalloc(cnt * sizeof(*new->e_argv));
    new->e_orig = xmalloc(cnt * sizeof(*new->e_orig));
    new->e_len = xmalloc(cnt * sizeof(*new->e_len));

    for (argv = *argvp, cnt = 0; argv < ap; ++argv, ++cnt) {
	new->e_orig[cnt] = *argv;
	if (new->flags & F_EXECPLUS)
	    new->e_pbsize += strlen(*argv) + 1;
	for (p = *argv; *p; ++p)
	    if (!(new->flags & F_EXECPLUS) && p[0] == '{' && p[1] == '}') {
		new->e_argv[cnt] = xmalloc(MAXPATHLEN);
		new->e_len[cnt] = MAXPATHLEN;
		break;
	    }
	if (!*p) {
	    new->e_argv[cnt] = *argv;
	    new->e_len[cnt] = 0;
	}
    }
    if (new->flags & F_EXECPLUS) {
	new->e_psize = new->e_pbsize;
	cnt--;
	for (i = 0; i < new->e_pnummax; i++) {
	    new->e_argv[cnt] = NULL;
	    new->e_len[cnt] = 0;
	    cnt++;
	}
	argv = ap;
	goto done;
    }
    new->e_argv[cnt] = new->e_orig[cnt] = NULL;

done:
    *argvp = argv + 1;
    return new;
}

/* Finish any pending -exec ... {} + functions. */
static void
finish_execplus(void)
{
    PLAN *p;

    for(p = lastexecplus; p != NULL; p = p->e_next)
	(p->execute)(p, NULL);
}

static int
f_flags(PLAN *plan, FTSENT *entry)
{
    unsigned long flags;

#if defined(HAVE_STRUCT_STAT_ST_FLAGS)
    flags = entry->fts_statp->st_flags;
#else
    flags = 0;	/* XXX HACK */
#endif
    if (plan->flags & F_ATLEAST)
	return (flags | plan->fl_flags) == flags &&
		    !(flags & plan->fl_notflags);
    else if (plan->flags & F_ANY)
	return (flags & plan->fl_flags) ||
		    (flags | plan->fl_notflags) != flags;
    else
	return flags == plan->fl_flags &&
		    !(plan->fl_flags & plan->fl_notflags);
}

static PLAN *
c_flags(OPTION *option, char ***argvp)
{
    char * flags_str = nextarg(option, argvp);
    PLAN * new = palloc(option);
    unsigned long flags, notflags;

    ftsoptions &= ~FTS_NOSTAT;

    if (*flags_str == '-') {
	new->flags |= F_ATLEAST;
	flags_str++;
    } else if (*flags_str == '+') {
	new->flags |= F_ANY;
	flags_str++;
    }
#if defined(HAVE_STRUCT_STAT_ST_FLAGS)	/* XXX HACK */
    if (strtofflags(&flags_str, &flags, &notflags) == 1)
	errx(1, "%s: %s: illegal flags string", option->name, flags_str);
#else
    flags = 0;
    notflags = ~flags;
#endif

    new->fl_flags = flags;
    new->fl_notflags = notflags;
    return new;
}

/*
 * -follow functions --
 *
 *	Always true, causes symbolic links to be followed on a global
 *	basis.
 */
static PLAN *
c_follow(OPTION *option, char ***argvp __unused)
{
    PLAN * new = palloc(option);
    ftsoptions &= ~FTS_PHYSICAL;
    ftsoptions |= FTS_LOGICAL;
    return new;
}

/*
 * -fstype functions --
 *
 *	True if the file is of a certain type.
 */
static int
f_fstype(PLAN *plan, FTSENT *entry)
{
    static dev_t curdev;	/* need a guaranteed illegal dev value */
    static int first = 1;
#if defined(HAVE_STRUCT_STAT_ST_FLAGS)	/* XXX HACK */
    struct statfs sb;
#endif
    static int val_type, val_flags;
    char *p, save[2] = {0,0};

    if ((plan->flags & F_MTMASK) == F_MTUNKNOWN)
	return 0;

    /* Only check when we cross mount point. */
    if (first || curdev != entry->fts_statp->st_dev) {
	curdev = entry->fts_statp->st_dev;

	/*
	 * Statfs follows symlinks; find wants the link's filesystem,
	 * not where it points.
	 */
	if (entry->fts_info == FTS_SL || entry->fts_info == FTS_SLNONE) {
	    if ((p = strrchr(entry->fts_accpath, '/')) != NULL)
		++p;
	    else
		p = entry->fts_accpath;
	    save[0] = p[0];
	    p[0] = '.';
	    save[1] = p[1];
	    p[1] = '\0';
	} else
	    p = NULL;

#if defined(HAVE_STRUCT_STAT_ST_FLAGS)	/* XXX HACK */
	if (statfs(entry->fts_accpath, &sb))
	    err(1, "%s", entry->fts_accpath);
#endif

	if (p) {
	    p[0] = save[0];
	    p[1] = save[1];
	}

	first = 0;

	/*
	 * Further tests may need both of these values, so
	 * always copy both of them.
	 */
#if defined(HAVE_STRUCT_STAT_ST_FLAGS)	/* XXX HACK */
	val_flags = sb.f_flags;
	val_type = sb.f_type;
#endif
    }
    switch (plan->flags & F_MTMASK) {
    case F_MTFLAG:
	return val_flags & plan->mt_data;
    case F_MTTYPE:
	return val_type == plan->mt_data;
    default:
	abort();
    }
}

static PLAN *
c_fstype(OPTION *option, char ***argvp)
{
    char * fsname = nextarg(option, argvp);
    PLAN * new = palloc(option);
#if defined(__APPLE__)
#define	xvfsconf	vfsconf
#endif
#if defined(HAVE_STRUCT_STAT_ST_FLAGS)	/* XXX HACK */
    struct xvfsconf vfc;
#endif

    ftsoptions &= ~FTS_NOSTAT;

#if defined(HAVE_STRUCT_STAT_ST_FLAGS)	/* XXX HACK */
    /*
     * Check first for a filesystem name.
     */
    if (getvfsbyname(fsname, &vfc) == 0) {
	new->flags |= F_MTTYPE;
	new->mt_data = vfc.vfc_typenum;
	return new;
    }
#endif

    switch (*fsname) {
    case 'l':
	if (!strcmp(fsname, "local")) {
	    new->flags |= F_MTFLAG;
#if defined(MNT_LOCAL)	/* XXX HACK */
	    new->mt_data = MNT_LOCAL;
#endif
	    return new;
	}
	break;
    case 'r':
	if (!strcmp(fsname, "rdonly")) {
	    new->flags |= F_MTFLAG;
#if defined(MNT_RDONLY)	/* XXX HACK: map to MS_RDONLY? */
	    new->mt_data = MNT_RDONLY;
#endif
	    return new;
	}
	break;
    }

    /*
     * We need to make filesystem checks for filesystems
     * that exists but aren't in the kernel work.
     */
    fprintf(stderr, "Warning: Unknown filesystem type %s\n", fsname);
    new->flags |= F_MTUNKNOWN;
    return new;
}

/*
 * -group gname functions --
 *
 *	True if the file belongs to the group gname.  If gname is numeric and
 *	an equivalent of the getgrnam() function does not return a valid group
 *	name, gname is taken as a group ID.
 */
static int
f_group(PLAN *plan, FTSENT *entry)
{
    COMPARE(entry->fts_statp->st_gid, plan->g_data);
}

static PLAN *
c_group(OPTION *option, char ***argvp)
{
    char * gname = nextarg(option, argvp);
    PLAN * new = palloc(option);
    struct group *g;
    gid_t gid;

    ftsoptions &= ~FTS_NOSTAT;

    g = getgrnam(gname);
    if (g == NULL) {
	char* cp = gname;
	if (gname[0] == '-' || gname[0] == '+')
	    gname++;
	gid = atoi(gname);
	if (gid == 0 && gname[0] != '0')
	    errx(1, "%s: %s: no such group", option->name, gname);
	gid = find_parsenum(new, option->name, cp, NULL);
    } else
	gid = g->gr_gid;

    new->g_data = gid;
    return new;
}

/*
 * -inum n functions --
 *
 *	True if the file has inode # n.
 */
static int
f_inum(PLAN *plan, FTSENT *entry)
{
    COMPARE(entry->fts_statp->st_ino, plan->i_data);
}

static PLAN *
c_inum(OPTION *option, char ***argvp)
{
    char * inum_str = nextarg(option, argvp);
    PLAN * new = palloc(option);

    ftsoptions &= ~FTS_NOSTAT;

    new->i_data = find_parsenum(new, option->name, inum_str, NULL);
    return new;
}

/*
 * -samefile FN
 *
 *	True if the file has the same inode (eg hard link) FN
 */

/* f_samefile is just f_inum */
static PLAN *
c_samefile(OPTION *option, char ***argvp)
{
    char * fn = nextarg(option, argvp);
    PLAN * new = palloc(option);
    struct stat sb;

    ftsoptions &= ~FTS_NOSTAT;

    if (Stat(fn, &sb))
	err(1, "%s", fn);
    new->i_data = sb.st_ino;
    return new;
}

/*
 * -links n functions --
 *
 *	True if the file has n links.
 */
static int
f_links(PLAN *plan, FTSENT *entry)
{
    COMPARE(entry->fts_statp->st_nlink, plan->l_data);
}

static PLAN *
c_links(OPTION *option, char ***argvp)
{
    char * nlinks = nextarg(option, argvp);
    PLAN * new = palloc(option);

    ftsoptions &= ~FTS_NOSTAT;

    new->l_data = (nlink_t)find_parsenum(new, option->name, nlinks, NULL);
    return new;
}

/*
 * -ls functions --
 *
 *	Always true - prints the current entry to stdout in "ls" format.
 */
static int
f_ls(PLAN *plan __unused, FTSENT *entry)
{
    printlong(entry->fts_path, entry->fts_accpath, entry->fts_statp);
    return 1;
}

static PLAN *
c_ls(OPTION *option, char ***argvp __unused)
{
    PLAN * new = palloc(option);
    ftsoptions &= ~FTS_NOSTAT;
    isoutput = 1;
    return new;
}

/*
 * -name functions --
 *
 *	True if the basename of the filename being examined
 *	matches pattern using Pattern Matching Notation S3.14
 */
static int
f_name(PLAN *plan, FTSENT *entry)
{
    char fn[PATH_MAX];
    const char *name;

    if (plan->flags & F_LINK) {
	name = fn;
	if (readlink(entry->fts_path, fn, sizeof(fn)) == -1)
	    return 0;
    } else
	name = entry->fts_name;
    return !fnmatch(plan->c_data, name,
	    plan->flags & F_IGNCASE ? FNM_CASEFOLD : 0);
}

static PLAN *
c_name(OPTION *option, char ***argvp)
{
    char * pattern = nextarg(option, argvp);
    PLAN * new = palloc(option);

    new->c_data = pattern;
    return new;
}

/*
 * -newer file functions --
 *
 *	True if the current file has been modified more recently
 *	then the modification time of the file named by the pathname
 *	file.
 */
static int
f_newer(PLAN *plan, FTSENT *entry)
{
    if (plan->flags & F_TIME_C)
	return entry->fts_statp->st_ctime > plan->t_data;
    else if (plan->flags & F_TIME_A)
	return entry->fts_statp->st_atime > plan->t_data;
    else if (plan->flags & F_TIME_B)
	return entry->fts_statp->st_birthtime > plan->t_data;
    else
	return entry->fts_statp->st_mtime > plan->t_data;
}

static PLAN *
c_newer(OPTION *option, char ***argvp)
{
    char * fn_or_tspec = nextarg(option, argvp);
    PLAN * new = palloc(option);
    struct stat sb;

    ftsoptions &= ~FTS_NOSTAT;

    /* compare against what */
    if (option->flags & F_TIME2_T) {
	new->t_data = get_date(fn_or_tspec, NULL);
	if (new->t_data == (time_t) -1)
		errx(1, "Can't parse date/time: %s", fn_or_tspec);
    } else {
	if (Stat(fn_or_tspec, &sb))
	    err(1, "%s", fn_or_tspec);
	if (option->flags & F_TIME2_C)
	    new->t_data = sb.st_ctime;
	else if (option->flags & F_TIME2_A)
	    new->t_data = sb.st_atime;
	else
	    new->t_data = sb.st_mtime;
    }
    return new;
}

/*
 * -nogroup functions --
 *
 *	True if file belongs to a user ID for which the equivalent
 *	of the getgrnam() 9.2.1 [POSIX.1] function returns NULL.
 */
static int
f_nogroup(PLAN *plan __unused, FTSENT *entry)
{
    return group_from_gid(entry->fts_statp->st_gid, 1) == NULL;
}

static PLAN *
c_nogroup(OPTION *option, char ***argvp __unused)
{
    PLAN * new = palloc(option);
    ftsoptions &= ~FTS_NOSTAT;
    return new;
}

/*
 * -nouser functions --
 *
 *	True if file belongs to a user ID for which the equivalent
 *	of the getpwuid() 9.2.2 [POSIX.1] function returns NULL.
 */
static int
f_nouser(PLAN *plan __unused, FTSENT *entry)
{
    return user_from_uid(entry->fts_statp->st_uid, 1) == NULL;
}

static PLAN *
c_nouser(OPTION *option, char ***argvp __unused)
{
    PLAN * new = palloc(option);
    ftsoptions &= ~FTS_NOSTAT;
    return new;
}

/*
 * -path functions --
 *
 *	True if the path of the filename being examined
 *	matches pattern using Pattern Matching Notation S3.14
 */
static int
f_path(PLAN *plan, FTSENT *entry)
{
    return !fnmatch(plan->c_data, entry->fts_path,
	    plan->flags & F_IGNCASE ? FNM_CASEFOLD : 0);
}

/* c_path is the same as c_name */

/*
 * -perm functions --
 *
 *	The mode argument is used to represent file mode bits.  If it starts
 *	with a leading digit, it's treated as an octal mode, otherwise as a
 *	symbolic mode.
 */
static int
f_perm(PLAN *plan, FTSENT *entry)
{
    mode_t mode;

    mode = entry->fts_statp->st_mode &
	    (S_ISUID|S_ISGID|S_ISTXT|S_IRWXU|S_IRWXG|S_IRWXO);
    if (plan->flags & F_ATLEAST)
	return (plan->m_data | mode) == mode;
    else if (plan->flags & F_ANY)
	return (mode & plan->m_data);
    else
	return mode == plan->m_data;
    /* NOTREACHED */
}

static PLAN *
c_perm(OPTION *option, char ***argvp)
{
    char * perm = nextarg(option, argvp);
    PLAN * new = palloc(option);
    mode_t *set;

    ftsoptions &= ~FTS_NOSTAT;

    if (*perm == '-') {
	new->flags |= F_ATLEAST;
	++perm;
    } else if (*perm == '+') {
	new->flags |= F_ANY;
	++perm;
    }

    if ((set = setmode(perm)) == NULL)
	errx(1, "%s: %s: illegal mode string", option->name, perm);

    new->m_data = getmode(set, 0);
    free(set);
    return new;
}

/*
 * -print functions --
 *
 *	Always true, causes the current pathname to be written to
 *	standard output.
 */
static int
f_print(PLAN *plan __unused, FTSENT *entry)
{
    (void)puts(entry->fts_path);
    return 1;
}

static PLAN *
c_print(OPTION *option, char ***argvp __unused)
{
    PLAN * new = palloc(option);
    isoutput = 1;
    return new;
}

/*
 * -print0 functions --
 *
 *	Always true, causes the current pathname to be written to
 *	standard output followed by a NUL character
 */
static int
f_print0(PLAN *plan __unused, FTSENT *entry)
{
    fputs(entry->fts_path, stdout);
    fputc('\0', stdout);
    return 1;
}

/* c_print0 is the same as c_print */

/*
 * -prune functions --
 *
 *	Prune a portion of the hierarchy.
 */
static int
f_prune(PLAN *plan __unused, FTSENT *entry)
{
    if (Fts_set(tree, entry, FTS_SKIP))
	err(1, "%s", entry->fts_path);
    return 1;
}

/* c_prune == c_simple */

/*
 * -regex functions --
 *
 *	True if the whole path of the file matches pattern using
 *	regular expression.
 */
static int
f_regex(PLAN *plan, FTSENT *entry)
{
    regex_t * pre = plan->re_data;
    char * str = entry->fts_path;
    int len = strlen(str);
    regmatch_t pmatch = { .rm_so = 0, .rm_eo = len };
    int errcode = regexec(pre, str, 1, &pmatch, REG_STARTEND);
    int matched = 0;

    if (errcode != 0 && errcode != REG_NOMATCH) {
	char errbuf[LINE_MAX];
	regerror(errcode, pre, errbuf, sizeof errbuf);
	errx(1, "%s: %s",
	     plan->flags & F_IGNCASE ? "-iregex" : "-regex", errbuf);
    }

    if (errcode == 0 && pmatch.rm_so == 0 && pmatch.rm_eo == len)
	matched = 1;

    return matched;
}

static PLAN *
c_regex(OPTION *option, char ***argvp)
{
    char * pattern = nextarg(option, argvp);
    PLAN * new = palloc(option);
    regex_t * pre = xmalloc(sizeof(*pre));
    int errcode;

    if ((errcode = regcomp(pre, pattern,
	    regexp_flags | (option->flags & F_IGNCASE ? REG_ICASE : 0))) != 0)
    {
	char errbuf[LINE_MAX];
	regerror(errcode, pre, errbuf, sizeof errbuf);
	errx(1, "%s: %s: %s",
		     option->flags & F_IGNCASE ? "-iregex" : "-regex",
		     pattern, errbuf);
    }

    new->re_data = pre;
    return new;
}

/* c_simple covers c_prune, c_openparen, c_closeparen, c_not, c_or, c_true, c_false */

static PLAN *
c_simple(OPTION *option, char ***argvp __unused)
{
    PLAN * new = palloc(option);
    return new;
}

/*
 * -size n[c] functions --
 *
 *	True if the file size in bytes, divided by an implementation defined
 *	value and rounded up to the next integer, is n.  If n is followed by
 *      one of c k M G T P, the size is in bytes, kilobytes,
 *      megabytes, gigabytes, terabytes or petabytes respectively.
 */
#define	FIND_SIZE	512
static int divsize = 1;

static int
f_size(PLAN *plan, FTSENT *entry)
{
    off_t size;

    size = divsize ? (entry->fts_statp->st_size + FIND_SIZE - 1) /
	    FIND_SIZE : entry->fts_statp->st_size;
    COMPARE(size, plan->o_data);
}

static PLAN *
c_size(OPTION *option, char ***argvp)
{
    char * size_str = nextarg(option, argvp);
    PLAN * new = palloc(option);
    char endch;
    long long scale;

    ftsoptions &= ~FTS_NOSTAT;

    endch = 'c';
    new->o_data = find_parsenum(new, option->name, size_str, &endch);
    if (endch != '\0') {
	divsize = 0;

	switch (endch) {
	case 'c':                       /* characters */
	    scale = 0x1LL;
	    break;
	case 'k':                       /* kilobytes 1<<10 */
	    scale = 0x400LL;
	    break;
	case 'M':                       /* megabytes 1<<20 */
	    scale = 0x100000LL;
	    break;
	case 'G':                       /* gigabytes 1<<30 */
	    scale = 0x40000000LL;
	    break;
	case 'T':                       /* terabytes 1<<40 */
	    scale = 0x1000000000LL;
	    break;
	case 'P':                       /* petabytes 1<<50 */
	    scale = 0x4000000000000LL;
	    break;
	default:
	    errx(1, "%s: %s: illegal trailing character",
			option->name, size_str);
	    break;
	}
	if (new->o_data > (LLONG_MAX / scale))
	    errx(1, "%s: %s: value too large", option->name, size_str);
	new->o_data *= scale;
    }
    return new;
}

/*
 * -type c functions --
 *
 *	True if the type of the file is c, where c is b, c, d, p, f or w
 *	for block special file, character special file, directory, FIFO,
 *	regular file or whiteout respectively.
 */
static int
f_type(PLAN *plan, FTSENT *entry)
{
    return (entry->fts_statp->st_mode & S_IFMT) == plan->m_data;
}

static PLAN *
c_type(OPTION *option, char ***argvp)
{
    char * typestring = nextarg(option, argvp);
    PLAN * new = palloc(option);
    mode_t  mask;

    ftsoptions &= ~FTS_NOSTAT;

    switch (typestring[0]) {
    case 'b':
	mask = S_IFBLK;
	break;
    case 'c':
	mask = S_IFCHR;
	break;
    case 'd':
	mask = S_IFDIR;
	break;
    case 'f':
	mask = S_IFREG;
	break;
    case 'l':
	mask = S_IFLNK;
	break;
    case 'p':
	mask = S_IFIFO;
	break;
    case 's':
	mask = S_IFSOCK;
	break;
#if defined(FTS_WHITEOUT) && defined(S_IFWHT)
    case 'w':
	mask = S_IFWHT;
	ftsoptions |= FTS_WHITEOUT;
	break;
#endif /* FTS_WHITEOUT */
    default:
	errx(1, "%s: %s: unknown type", option->name, typestring);
    }
    new->m_data = mask;
    return new;
}

/*
 * -user uname functions --
 *
 *	True if the file belongs to the user uname.  If uname is numeric and
 *	an equivalent of the getpwnam() S9.2.2 [POSIX.1] function does not
 *	return a valid user name, uname is taken as a user ID.
 */
static int
f_user(PLAN *plan, FTSENT *entry)
{
    COMPARE(entry->fts_statp->st_uid, plan->u_data);
}

static PLAN *
c_user(OPTION *option, char ***argvp)
{
    char * username = nextarg(option, argvp);
    PLAN * new = palloc(option);
    struct passwd *p;
    uid_t uid;

    ftsoptions &= ~FTS_NOSTAT;

    p = getpwnam(username);
    if (p == NULL) {
	char* cp = username;
	if( username[0] == '-' || username[0] == '+' )
	    username++;
	uid = atoi(username);
	if (uid == 0 && username[0] != '0')
	    errx(1, "%s: %s: no such user", option->name, username);
	uid = find_parsenum(new, option->name, cp, NULL);
    } else
	uid = p->pw_uid;

    new->u_data = uid;
    return new;
}

/*
 * -xdev functions --
 *
 *	Always true, causes find not to descend past directories that have a
 *	different device ID (st_dev, see stat() S5.6.2 [POSIX.1])
 */
static PLAN *
c_xdev(OPTION *option, char ***argvp __unused)
{
    PLAN * new = palloc(option);
    ftsoptions |= FTS_XDEV;
    return new;
}

/*==============================================================*/

/* NB: the following table must be sorted lexically. */
/* Options listed with C++ comments are in gnu find, but not our find */
static OPTION const options[] = {
    { "!",		c_simple,	f_not,		0 },
    { "(",		c_simple,	f_openparen,	0 },
    { ")",		c_simple,	f_closeparen,	0 },
    { "-Bmin",		c_Xmin,		f_Xmin,		F_TIME_B },
    { "-Bnewer",	c_newer,	f_newer,	F_TIME_B },
    { "-Btime",		c_Xtime,	f_Xtime,	F_TIME_B },
    { "-a",		c_and,		NULL,		0 },
    { "-acl",		c_acl,		f_acl,		0 },
    { "-amin",		c_Xmin,		f_Xmin,		F_TIME_A },
    { "-and",		c_and,		NULL,		0 },
    { "-anewer",	c_newer,	f_newer,	F_TIME_A },
    { "-atime",		c_Xtime,	f_Xtime,	F_TIME_A },
    { "-cmin",		c_Xmin,		f_Xmin,		F_TIME_C },
    { "-cnewer",	c_newer,	f_newer,	F_TIME_C },
    { "-ctime",		c_Xtime,	f_Xtime,	F_TIME_C },
    { "-d",		c_depth,	f_depth,	0 },
// -daystart
    { "-delete",	c_delete,	f_delete,	0 },
    { "-depth",		c_depth,	f_depth,	0 },
    { "-empty",		c_empty,	f_empty,	0 },
    { "-exec",		c_exec,		f_exec,		0 },
    { "-execdir",	c_exec,		f_exec,		F_EXECDIR },
    { "-false",		c_simple,	f_false,	0 },
    { "-flags",		c_flags,	f_flags,	0 },
// -fls
    { "-follow",	c_follow,	f_always_true,	0 },
// -fprint
// -fprint0
// -fprintf
    { "-fstype",	c_fstype,	f_fstype,	0 },
    { "-gid",		c_group,	f_group,	0 },
    { "-group",		c_group,	f_group,	0 },
    { "-ignore_readdir_race",c_simple,	f_always_true,	0 },
    { "-ilname",	c_name,		f_name,		F_LINK | F_IGNCASE },
    { "-iname",		c_name,		f_name,		F_IGNCASE },
    { "-inum",		c_inum,		f_inum,		0 },
    { "-ipath",		c_name,		f_path,		F_IGNCASE },
    { "-iregex",	c_regex,	f_regex,	F_IGNCASE },
    { "-iwholename",	c_name,		f_path,		F_IGNCASE },
    { "-links",		c_links,	f_links,	0 },
    { "-lname",		c_name,		f_name,		F_LINK },
    { "-ls",		c_ls,		f_ls,		0 },
    { "-maxdepth",	c_mXXdepth,	f_always_true,	F_MAXDEPTH },
    { "-mindepth",	c_mXXdepth,	f_always_true,	0 },
    { "-mmin",		c_Xmin,		f_Xmin,		0 },
    { "-mnewer",	c_newer,	f_newer,	0 },
    { "-mount",		c_xdev,		f_always_true,	0 },
    { "-mtime",		c_Xtime,	f_Xtime,	0 },
    { "-name",		c_name,		f_name,		0 },
    { "-newer",		c_newer,	f_newer,	0 },
    { "-newerBB",	c_newer,	f_newer,	F_TIME_B | F_TIME2_B },
    { "-newerBa",	c_newer,	f_newer,	F_TIME_B | F_TIME2_A },
    { "-newerBc",	c_newer,	f_newer,	F_TIME_B | F_TIME2_C },
    { "-newerBm",	c_newer,	f_newer,	F_TIME_B },
    { "-newerBt",	c_newer,	f_newer,	F_TIME_B | F_TIME2_T },
    { "-neweraB",	c_newer,	f_newer,	F_TIME_A | F_TIME2_B },
    { "-neweraa",	c_newer,	f_newer,	F_TIME_A | F_TIME2_A },
    { "-newerac",	c_newer,	f_newer,	F_TIME_A | F_TIME2_C },
    { "-neweram",	c_newer,	f_newer,	F_TIME_A },
    { "-newerat",	c_newer,	f_newer,	F_TIME_A | F_TIME2_T },
    { "-newercB",	c_newer,	f_newer,	F_TIME_C | F_TIME2_B },
    { "-newerca",	c_newer,	f_newer,	F_TIME_C | F_TIME2_A },
    { "-newercc",	c_newer,	f_newer,	F_TIME_C | F_TIME2_C },
    { "-newercm",	c_newer,	f_newer,	F_TIME_C },
    { "-newerct",	c_newer,	f_newer,	F_TIME_C | F_TIME2_T },
    { "-newermB",	c_newer,	f_newer,	F_TIME2_B },
    { "-newerma",	c_newer,	f_newer,	F_TIME2_A },
    { "-newermc",	c_newer,	f_newer,	F_TIME2_C },
    { "-newermm",	c_newer,	f_newer,	0 },
    { "-newermt",	c_newer,	f_newer,	F_TIME2_T },
    { "-nogroup",	c_nogroup,	f_nogroup,	0 },
    { "-noignore_readdir_race",c_simple, f_always_true,0 },
    { "-noleaf",	c_simple,	f_always_true,	0 },
    { "-not",		c_simple,	f_not,		0 },
    { "-nouser",	c_nouser,	f_nouser,	0 },
    { "-o",		c_simple,	f_or,		0 },
    { "-ok",		c_exec,		f_exec,		F_NEEDOK },
    { "-okdir",		c_exec,		f_exec,		F_NEEDOK | F_EXECDIR },
    { "-or",		c_simple,	f_or,		0 },
    { "-path", 		c_name,		f_path,		0 },
    { "-perm",		c_perm,		f_perm,		0 },
    { "-print",		c_print,	f_print,	0 },
    { "-print0",	c_print,	f_print0,	0 },
// -printf
    { "-prune",		c_simple,	f_prune,	0 },
    { "-quit",		c_simple,	f_quit,		0 },
    { "-regex",		c_regex,	f_regex,	0 },
    { "-samefile",	c_samefile,	f_inum,		0 },
    { "-size",		c_size,		f_size,		0 },
    { "-true",		c_simple,	f_always_true,	0 },
    { "-type",		c_type,		f_type,		0 },
    { "-uid",		c_user,		f_user,		0 },
    { "-user",		c_user,		f_user,		0 },
    { "-wholename",	c_name,		f_path,		0 },
    { "-xdev",		c_xdev,		f_always_true,	0 },
// -xtype
};

static int
typecompare(const void *a, const void *b)
{
    return strcmp(((const OPTION *)a)->name, ((const OPTION *)b)->name);
}

static OPTION *
lookup_option(const char *name)
{
    OPTION tmp;

    tmp.name = name;
    return ((OPTION *)bsearch(&tmp, options,
		sizeof(options)/sizeof(OPTION), sizeof(OPTION), typecompare));
}

/*
 * find_create --
 *	create a node corresponding to a command line argument.
 *
 * TODO:
 *	add create/process function pointers to node, so we can skip
 *	this switch stuff.
 */
static PLAN *
find_create(char ***argvp)
{
    OPTION *p;
    PLAN *new;
    char ** argv = *argvp;

    if ((p = lookup_option(*argv)) == NULL)
	errx(1, "%s: unknown option", *argv);
    ++argv;

    new = (p->create)(p, &argv);
    *argvp = argv;
    return new;
}

/*
 * find_compare --
 *	tell fts_open() how to order the traversal of the hierarchy. 
 *	This variant gives lexicographical order, i.e., alphabetical
 *	order within each directory.
 */
static int
find_compare(const FTSENT * const *s1, const FTSENT * const *s2)
{

    return strcoll((*s1)->fts_name, (*s2)->fts_name);
}

/*
 * find_formplan --
 *	process the command line and create a "plan" corresponding to the
 *	command arguments.
 */
static PLAN *
find_formplan(char *argv[])
{
    PLAN *plan = NULL;
    PLAN *tail = NULL;
    PLAN *new;

    /*
     * for each argument in the command line, determine what kind of node
     * it is, create the appropriate node type and add the new plan node
     * to the end of the existing plan.  The resulting plan is a linked
     * list of plan nodes.  For example, the string:
     *
     *	% find . -name foo -newer bar -print
     *
     * results in the plan:
     *
     *	[-name foo]--> [-newer bar]--> [-print]
     *
     * in this diagram, `[-name foo]' represents the plan node generated
     * by c_name() with an argument of foo and `-->' represents the
     * plan->next pointer.
     */
    while (*argv) {
	if (!(new = find_create(&argv)))
	    continue;
	if (plan == NULL)
	    tail = plan = new;
	else {
	    tail->next = new;
	    tail = new;
	}
    }

    /*
     * if the user didn't specify one of -print, -ok or -exec, then -print
     * is assumed so we bracket the current expression with parens, if
     * necessary, and add a -print node on the end.
     */
    if (!isoutput) {
	OPTION *p;
	char **argv1 = 0;

	if (plan == NULL) {
	    p = lookup_option("-print");
	    new = (p->create)(p, &argv1);
	    tail = plan = new;
	} else {
	    p = lookup_option("(");
	    new = (p->create)(p, &argv1);
	    new->next = plan;
	    plan = new;
	    p = lookup_option(")");
	    new = (p->create)(p, &argv1);
	    tail->next = new;
	    tail = new;
	    p = lookup_option("-print");
	    new = (p->create)(p, &argv1);
	    tail->next = new;
	    tail = new;
	}
    }

    /*
     * the command line has been completely processed into a search plan
     * except for the (, ), !, and -o operators.  Rearrange the plan so
     * that the portions of the plan which are affected by the operators
     * are moved into operator nodes themselves.  For example:
     *
     *	[!]--> [-name foo]--> [-print]
     *
     * becomes
     *
     *	[! [-name foo] ]--> [-print]
     *
     * and
     *
     *	[(]--> [-depth]--> [-name foo]--> [)]--> [-print]
     *
     * becomes
     *
     *	[expr [-depth]-->[-name foo] ]--> [-print]
     *
     * operators are handled in order of precedence.
     */

    plan = paren_squish(plan);		/* ()'s */
    plan = not_squish(plan);		/* !'s */
    plan = or_squish(plan);			/* -o's */
    return plan;
}

/*
 * find_execute --
 *	take a search plan and an array of search paths and executes the plan
 *	over all FTSENT's returned for the given search paths.
 */
static int
find_execute(PLAN *plan, char *paths[])
{
    FTSENT *entry;
    PLAN *p;
    int rval = 0;

    tree = Fts_open(paths, ftsoptions, (issort ? (int (*)(const FTSENT **, const FTSENT **))find_compare : NULL));
    if (tree == NULL)
	err(1, "Fts_open");

    while ((entry = Fts_read(tree)) != NULL) {
	if (maxdepth != -1 && entry->fts_level >= maxdepth) {
	    if (Fts_set(tree, entry, FTS_SKIP))
		err(1, "%s", entry->fts_path);
	}

	switch (entry->fts_info) {
	case FTS_D:
	    if (isdepth)
		continue;
	    break;
	case FTS_DP:
	    if (!isdepth)
		continue;
	    break;
	case FTS_DNR:
	case FTS_ERR:
	case FTS_NS:
	    (void)fflush(stdout);
	    warnx("%s: %s", entry->fts_path, strerror(entry->fts_errno));
	    rval = 1;
	    continue;
#ifdef FTS_W
	case FTS_W:
	    continue;
#endif /* FTS_W */
	}
#define	BADCH	" \t\n\\'\""
	if (isxargs && strpbrk(entry->fts_path, BADCH)) {
	    (void)fflush(stdout);
	    warnx("%s: illegal path", entry->fts_path);
	    rval = 1;
	    continue;
	}

	if (mindepth != -1 && entry->fts_level < mindepth)
	    continue;

	/*
	 * Call all the functions in the execution plan until one is
	 * false or all have been executed.  This is where we do all
	 * the work specified by the user on the command line.
	 */
	for (p = plan; p && (p->execute)(p, entry); p = p->next);
    }
    finish_execplus();
    if (errno)
		err(1, "Fts_read");
    return rval;
}

/*==============================================================*/

static void
usage(void)
{
	(void)fprintf(stderr, "%s\n%s\n",
"usage: find [-H | -L | -P] [-EXdsx] [-f path] path ... [expression]",
"       find [-H | -L | -P] [-EXdsx] -f path [path ...] [expression]");
	exit(1);
}

int
main(int argc, char *argv[])
{
    char **p, **start;
    int Hflag, Lflag, ch;

    (void)setlocale(LC_ALL, "");

    (void)time(&now);	/* initialize the time-of-day */

    p = start = argv;
    Hflag = Lflag = 0;
    ftsoptions = FTS_NOSTAT | FTS_PHYSICAL;
    while ((ch = getopt(argc, argv, "+EHLPXdf:sx")) != -1)
	switch (ch) {
	case 'E':
	    regexp_flags |= REG_EXTENDED;
	    break;
	case 'H':
	    Hflag = 1;
	    Lflag = 0;
	    break;
	case 'L':
	    Lflag = 1;
	    Hflag = 0;
	    break;
	case 'P':
	    Hflag = Lflag = 0;
	    break;
	case 'X':
	    isxargs = 1;
	    break;
	case 'd':
	    isdepth = 1;
	    break;
	case 'f':
	    *p++ = optarg;
	    break;
	case 's':
	    issort = 1;
	    break;
	case 'x':
	    ftsoptions |= FTS_XDEV;
	    break;
	case '?':
	default:
	    break;
	}

    argc -= optind;
    argv += optind;

    if (Hflag)
	ftsoptions |= FTS_COMFOLLOW;
    if (Lflag) {
	ftsoptions &= ~FTS_PHYSICAL;
	ftsoptions |= FTS_LOGICAL;
    }

    /*
     * Find first option to delimit the file list.  The first argument
     * that starts with a -, or is a ! or a ( must be interpreted as a
     * part of the find expression, according to POSIX .2.
     */
    for (; *argv != NULL; *p++ = *argv++) {
	if (argv[0][0] == '-')
	    break;
	if ((argv[0][0] == '!' || argv[0][0] == '(') && argv[0][1] == '\0')
	    break;
    }

    if (p == start)
	usage();
    *p = NULL;

    if ((dotfd = open(".", O_RDONLY, 0)) < 0)
	err(1, ".");

    exit(find_execute(find_formplan(argv), start));
}
