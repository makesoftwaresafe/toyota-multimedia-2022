/** \ingroup rpmio
 * \file rpmio/rpmrpc.c
 */

#include "system.h"

#include <rpmio_internal.h>
#include <rpmmacro.h>

#define	_RPMDIR_INTERNAL
#include <rpmdir.h>
#define	_RPMDAV_INTERNAL
#include <rpmdav.h>

#include <rpmhash.h>
#include <ugid.h>

#include "debug.h"

/*@access DIR @*/
/*@access FD_t @*/
/*@access urlinfo @*/

/* =============================================================== */
static int ftpMkdir(const char * path, /*@unused@*/ mode_t mode)
	/*@globals h_errno, fileSystem, internalState @*/
	/*@modifies fileSystem, internalState @*/
{
    int rc;
    if ((rc = ftpCmd("MKD", path, NULL)) != 0)
	return rc;
#if NOTYET
    {	char buf[20];
	sprintf(buf, " 0%o", mode);
	(void) ftpCmd("SITE CHMOD", path, buf);
    }
#endif
    return rc;
}

static int ftpChdir(const char * path)
	/*@globals h_errno, fileSystem, internalState @*/
	/*@modifies fileSystem, internalState @*/
{
    return ftpCmd("CWD", path, NULL);
}

static int ftpRmdir(const char * path)
	/*@globals h_errno, fileSystem, internalState @*/
	/*@modifies fileSystem, internalState @*/
{
    return ftpCmd("RMD", path, NULL);
}

static int ftpRename(const char * oldpath, const char * newpath)
	/*@globals h_errno, fileSystem, internalState @*/
	/*@modifies fileSystem, internalState @*/
{
    int rc;
    if ((rc = ftpCmd("RNFR", oldpath, NULL)) != 0)
	return rc;
    return ftpCmd("RNTO", newpath, NULL);
}

static int ftpUnlink(const char * path)
	/*@globals h_errno, fileSystem, internalState @*/
	/*@modifies fileSystem, internalState @*/
{
    return ftpCmd("DELE", path, NULL);
}

/* =============================================================== */
int Mkdir (const char * path, mode_t mode)
{
    const char * lpath;
    int ut = urlPath(path, &lpath);

if (_rpmio_debug)
fprintf(stderr, "*** Mkdir(%s, 0%o)\n", path, (unsigned)mode);
    switch (ut) {
    case URL_IS_FTP:
	return ftpMkdir(path, mode);
	/*@notreached@*/ break;
    case URL_IS_HTTPS:
    case URL_IS_HTTP:
#ifdef WITH_NEON
	return davMkdir(path, mode);
#endif
	/*@notreached@*/ break;
    case URL_IS_PATH:
	path = lpath;
	/*@fallthrough@*/
    case URL_IS_UNKNOWN:
	break;
    case URL_IS_DASH:
    case URL_IS_HKP:
    case URL_IS_MONGO:	/* XXX FIXME */
    default:
	return -2;
	/*@notreached@*/ break;
    }
    return mkdir(path, mode);
}

int Chdir (const char * path)
{
    const char * lpath;
    int ut = urlPath(path, &lpath);

if (_rpmio_debug)
fprintf(stderr, "*** Chdir(%s)\n", path);
    switch (ut) {
    case URL_IS_FTP:
	return ftpChdir(path);
	/*@notreached@*/ break;
    case URL_IS_HTTPS:
    case URL_IS_HTTP:
#ifdef	NOTYET
	return davChdir(path);
#else
	errno = EINVAL;		/* XXX W2DO? */
	return -2;
#endif
	/*@notreached@*/ break;
    case URL_IS_PATH:
	path = lpath;
	/*@fallthrough@*/
    case URL_IS_UNKNOWN:
	break;
    case URL_IS_DASH:
    case URL_IS_HKP:
    case URL_IS_MONGO:	/* XXX FIXME */
    default:
	errno = EINVAL;		/* XXX W2DO? */
	return -2;
	/*@notreached@*/ break;
    }
    return chdir(path);
}

int Rmdir (const char * path)
{
    const char * lpath;
    int ut = urlPath(path, &lpath);

if (_rpmio_debug)
fprintf(stderr, "*** Rmdir(%s)\n", path);
    switch (ut) {
    case URL_IS_FTP:
	return ftpRmdir(path);
	/*@notreached@*/ break;
    case URL_IS_HTTPS:
    case URL_IS_HTTP:
#ifdef WITH_NEON
	return davRmdir(path);
#endif
	/*@notreached@*/ break;
    case URL_IS_PATH:
	path = lpath;
	/*@fallthrough@*/
    case URL_IS_UNKNOWN:
	break;
    case URL_IS_DASH:
    case URL_IS_HKP:
    case URL_IS_MONGO:	/* XXX FIXME */
    default:
	return -2;
	/*@notreached@*/ break;
    }
    return rmdir(path);
}

/*@unchecked@*/
const char * _chroot_prefix = NULL;

int Chroot(const char * path)
{
    const char * lpath;
    int ut = urlPath(path, &lpath);
    int rc;

    switch (ut) {
    case URL_IS_PATH:
	path = lpath;
	/*@fallthrough@*/
    case URL_IS_UNKNOWN:
	break;
    case URL_IS_DASH:
    case URL_IS_HKP:
    case URL_IS_FTP:		/* XXX TODO: implement. */
    case URL_IS_HTTP:		/* XXX TODO: implement. */
    case URL_IS_HTTPS:		/* XXX TODO: implement. */
    case URL_IS_MONGO:	/* XXX FIXME */
    default:
	errno = EINVAL;		/* XXX W2DO? */
	return -2;
	/*@notreached@*/ break;
    }

/*@-dependenttrans -modobserver -observertrans @*/
    _chroot_prefix = _free(_chroot_prefix);
/*@=dependenttrans =modobserver =observertrans @*/
/*@-globs -mods@*/	/* XXX hide rpmGlobalMacroContext mods for now. */
    if (strcmp(path, "."))
	_chroot_prefix = rpmGetPath(path, NULL);
/*@=globs =mods@*/

/*@-superuser@*/
    rc = chroot(path);
/*@=superuser@*/

if (_rpmio_debug)
fprintf(stderr, "<-- %s(%s) prefix %s rc %d\n", __FUNCTION__, path, _chroot_prefix, rc);

    return rc;
}
/*@=mods@*/

int Open(const char * path, int flags, mode_t mode)
{
    const char * lpath;
    int ut = urlPath(path, &lpath);
    int fdno;

    switch (ut) {
    case URL_IS_PATH:
	path = lpath;
	/*@fallthrough@*/
    case URL_IS_UNKNOWN:
	break;
    case URL_IS_DASH:
    case URL_IS_HKP:
    case URL_IS_FTP:		/* XXX TODO: implement. */
    case URL_IS_HTTP:		/* XXX TODO: implement. */
    case URL_IS_HTTPS:		/* XXX TODO: implement. */
    case URL_IS_MONGO:	/* XXX FIXME */
    default:
	errno = EINVAL;		/* XXX W2DO? */
	return -2;
	/*@notreached@*/ break;
    }

    if (_chroot_prefix && _chroot_prefix[0] == '/' && _chroot_prefix[1] != '\0')
    {
	size_t nb = strlen(_chroot_prefix);
	size_t ob = strlen(path);
	while (nb > 0 && _chroot_prefix[nb-1] == '/')
	    nb--;
	if (ob > nb && !strncmp(path, _chroot_prefix, nb) && path[nb] == '/')
	    path += nb;
    }
#ifdef	NOTYET	/* XXX likely sane default. */
    if (mode == 0)
	mode = 0644;
#endif
    fdno = open(path, flags, mode);
    /* XXX if the open(2) fails, try to strip a possible chroot(2) prefix. */
    if (fdno < 0 && errno == ENOENT) {
	const char *dbpath = rpmExpand("%{?_dbpath}/", NULL);
	const char * fn = strstr(path + 1, dbpath);
	if (fn)
	    fdno = open(fn, flags, mode);
	dbpath = _free(dbpath);
    }
    if (fdno >= 0) {
	if (fcntl(fdno, F_SETFD, FD_CLOEXEC) < 0) {
	    (void) close(fdno);
	    fdno = -1;
	}
    }

#ifdef	NOTYET	/* XXX db-5.0.26 is slower using POSIX_FADV_RANDOM. */
#if defined(HAVE_POSIX_FADVISE) && defined(POSIX_FADV_RANDOM)
    (void) posix_fadvise(fdno, 0, 0, POSIX_FADV_RANDOM);
#endif
#endif

if (_rpmio_debug)
fprintf(stderr, "<-- %s(%s, 0x%x, 0%o) prefix %s fdno %d\n", __FUNCTION__, path, flags, (unsigned)mode, _chroot_prefix, fdno);

    return fdno;
}

/* XXX rpmdb.c: analogue to rename(2). */

int Rename (const char * oldpath, const char * newpath)
{
    const char *oe = NULL;
    const char *ne = NULL;
    int oldut, newut;

if (_rpmio_debug)
fprintf(stderr, "*** Rename(%s, %s)\n", oldpath, newpath);
    /* XXX lib/install.c used to rely on this behavior. */
    if (!strcmp(oldpath, newpath)) return 0;

    oldut = urlPath(oldpath, &oe);
    switch (oldut) {
    case URL_IS_HTTPS:
    case URL_IS_HTTP:
#ifdef WITH_NEON
	return davRename(oldpath, newpath);
#endif
	/*@notreached@*/ break;
    case URL_IS_FTP:		/* XXX WRONG WRONG WRONG */
    case URL_IS_PATH:
    case URL_IS_UNKNOWN:
	break;
    case URL_IS_DASH:
    case URL_IS_HKP:
    case URL_IS_MONGO:	/* XXX FIXME */
    default:
	return -2;
	/*@notreached@*/ break;
    }

    newut = urlPath(newpath, &ne);
    switch (newut) {
    case URL_IS_FTP:
if (_rpmio_debug)
fprintf(stderr, "*** rename old %*s new %*s\n", (int)(oe - oldpath), oldpath, (int)(ne - newpath), newpath);
	if (!(oldut == newut && oe && ne && (oe - oldpath) == (ne - newpath) &&
	    !xstrncasecmp(oldpath, newpath, (oe - oldpath))))
	    return -2;
	return ftpRename(oldpath, newpath);
	/*@notreached@*/ break;
    case URL_IS_HTTPS:		/* XXX WRONG WRONG WRONG */
    case URL_IS_HTTP:		/* XXX WRONG WRONG WRONG */
    case URL_IS_PATH:
	oldpath = oe;
	newpath = ne;
	break;
    case URL_IS_UNKNOWN:
	break;
    case URL_IS_DASH:
    case URL_IS_HKP:
    case URL_IS_MONGO:	/* XXX FIXME */
    default:
	return -2;
	/*@notreached@*/ break;
    }
    return rename(oldpath, newpath);
}

int Link (const char * oldpath, const char * newpath)
{
    const char *oe = NULL;
    const char *ne = NULL;
    int oldut, newut;

if (_rpmio_debug)
fprintf(stderr, "*** Link(%s, %s)\n", oldpath, newpath);
    oldut = urlPath(oldpath, &oe);
    switch (oldut) {
    case URL_IS_HTTPS:		/* XXX WRONG WRONG WRONG */
    case URL_IS_HTTP:		/* XXX WRONG WRONG WRONG */
    case URL_IS_FTP:		/* XXX WRONG WRONG WRONG */
    case URL_IS_PATH:
    case URL_IS_UNKNOWN:
	break;
    case URL_IS_DASH:
    case URL_IS_HKP:
    case URL_IS_MONGO:	/* XXX FIXME */
    default:
	return -2;
	/*@notreached@*/ break;
    }

    newut = urlPath(newpath, &ne);
    switch (newut) {
    case URL_IS_HTTPS:		/* XXX WRONG WRONG WRONG */
    case URL_IS_HTTP:		/* XXX WRONG WRONG WRONG */
    case URL_IS_FTP:		/* XXX WRONG WRONG WRONG */
    case URL_IS_PATH:
if (_rpmio_debug)
fprintf(stderr, "*** link old %*s new %*s\n", (int)(oe - oldpath), oldpath, (int)(ne - newpath), newpath);
	if (!(oldut == newut && oe && ne && (oe - oldpath) == (ne - newpath) &&
	    !xstrncasecmp(oldpath, newpath, (oe - oldpath))))
	    return -2;
	oldpath = oe;
	newpath = ne;
	break;
    case URL_IS_UNKNOWN:
	break;
    case URL_IS_DASH:
    case URL_IS_HKP:
    case URL_IS_MONGO:	/* XXX FIXME */
    default:
	return -2;
	/*@notreached@*/ break;
    }
    return link(oldpath, newpath);
}

/* XXX build/build.c: analogue to unlink(2). */

int Unlink(const char * path) {
    const char * lpath;
    int ut = urlPath(path, &lpath);
    int rc = -2;

    switch (ut) {
    case URL_IS_FTP:
	rc = ftpUnlink(path);
	/*@notreached@*/ break;
    case URL_IS_HTTPS:
    case URL_IS_HTTP:
#ifdef WITH_NEON
	rc = davUnlink(path);
#endif
	/*@notreached@*/ break;
    case URL_IS_PATH:
	path = lpath;
	/*@fallthrough@*/
    case URL_IS_UNKNOWN:
	break;
    case URL_IS_DASH:
    case URL_IS_HKP:
    case URL_IS_MONGO:	/* XXX FIXME */
    default:
	errno = EINVAL;
	goto exit;
	/*@notreached@*/ break;
    }
    rc = unlink(path);
exit:
if (_rpmio_debug)
fprintf(stderr, "<-- %s(%s) rc %d\n", __FUNCTION__, path, rc);
    return rc;
}

/* XXX swiped from mc-4.5.39-pre9 vfs/ftpfs.c */

#define	g_strdup	xstrdup
#define	g_free		free

/*
 * FIXME: this is broken. It depends on mc not crossing border on month!
 */
/*@unchecked@*/
static int current_mday;
/*@unchecked@*/
static int current_mon;
/*@unchecked@*/
static int current_year;

/* Following stuff (parse_ls_lga) is used by ftpfs and extfs */
#define MAXCOLS		30

/*@unchecked@*/
static char *columns [MAXCOLS];	/* Points to the string in column n */
/*@unchecked@*/
static int   column_ptr [MAXCOLS]; /* Index from 0 to the starting positions of the columns */

static int
vfs_split_text (char *p)
	/*@globals columns, column_ptr @*/
	/*@modifies *p, columns, column_ptr @*/
{
    char *original = p;
    int  numcols;


    for (numcols = 0; *p && numcols < MAXCOLS; numcols++){
	while (*p == ' ' || *p == '\r' || *p == '\n'){
	    *p = '\0';
	    p++;
	}
	columns [numcols] = p;
	column_ptr [numcols] = p - original;
	while (*p && *p != ' ' && *p != '\r' && *p != '\n')
	    p++;
    }
    return numcols;
}

static int
is_num (int idx)
	/*@*/
{
    if (!columns [idx] || columns [idx][0] < '0' || columns [idx][0] > '9')
	return 0;
    return 1;
}

static int
is_dos_date(/*@null@*/ const char *str)
	/*@*/
{
    if (str != NULL && strlen(str) == 8 &&
		str[2] == str[5] && strchr("\\-/", (int)str[2]) != NULL)
	return 1;
    return 0;
}

static int
is_week (/*@null@*/ const char * str, /*@out@*/ struct tm * tim)
	/*@modifies *tim @*/
{
/*@observer@*/ static const char * week = "SunMonTueWedThuFriSat";
    const char * pos;

    /*@-observertrans -mayaliasunique@*/
    if (str != NULL && (pos=strstr(week, str)) != NULL) {
    /*@=observertrans =mayaliasunique@*/
        if (tim != NULL)
	    tim->tm_wday = (pos - week)/3;
	return 1;
    }
    return 0;
}

static int
is_month (/*@null@*/ const char * str, /*@out@*/ struct tm * tim)
	/*@modifies *tim @*/
{
/*@observer@*/ static const char * month = "JanFebMarAprMayJunJulAugSepOctNovDec";
    const char * pos;

    /*@-observertrans -mayaliasunique@*/
    if (str != NULL && (pos = strstr(month, str)) != NULL) {
    /*@=observertrans -mayaliasunique@*/
        if (tim != NULL)
	    tim->tm_mon = (pos - month)/3;
	return 1;
    }
    return 0;
}

static int
is_time (/*@null@*/ const char * str, /*@out@*/ struct tm * tim)
	/*@modifies *tim @*/
{
    const char * p, * p2;

    if (str != NULL && (p = strchr(str, ':')) && (p2 = strrchr(str, ':'))) {
	if (p != p2) {
    	    if (sscanf (str, "%2d:%2d:%2d", &tim->tm_hour, &tim->tm_min, &tim->tm_sec) != 3)
		return 0;
	} else {
	    if (sscanf (str, "%2d:%2d", &tim->tm_hour, &tim->tm_min) != 2)
		return 0;
	}
    } else
        return 0;

    return 1;
}

static int is_year(/*@null@*/ const char * str, /*@out@*/ struct tm * tim)
	/*@modifies *tim @*/
{
    long year;

    if (str == NULL)
	return 0;

    if (strchr(str,':'))
        return 0;

    if (strlen(str) != 4)
        return 0;

    if (sscanf(str, "%ld", &year) != 1)
        return 0;

    if (year < 1900 || year > 3000)
        return 0;

    tim->tm_year = (int) (year - 1900);

    return 1;
}

/*
 * FIXME: this is broken. Consider following entry:
 * -rwx------   1 root     root            1 Aug 31 10:04 2904 1234
 * where "2904 1234" is filename. Well, this code decodes it as year :-(.
 */

static int
vfs_parse_filetype (char c)
	/*@*/
{
    switch (c) {
        case 'd': return (int)S_IFDIR;
        case 'b': return (int)S_IFBLK;
        case 'c': return (int)S_IFCHR;
        case 'l': return (int)S_IFLNK;
        case 's':
#ifdef IS_IFSOCK /* And if not, we fall through to IFIFO, which is pretty close */
	          return (int)S_IFSOCK;
#endif
        case 'p': return (int)S_IFIFO;
        case 'm': case 'n':		/* Don't know what these are :-) */
        case '-': case '?': return (int)S_IFREG;
        default: return -1;
    }
}

static int vfs_parse_filemode (const char *p)
	/*@*/
{	/* converts rw-rw-rw- into 0666 */
    int res = 0;
    switch (*(p++)) {
	case 'r': res |= 0400; break;
	case '-': break;
	default: return -1;
    }
    switch (*(p++)) {
	case 'w': res |= 0200; break;
	case '-': break;
	default: return -1;
    }
    switch (*(p++)) {
	case 'x': res |= 0100; break;
	case 's': res |= 0100 | S_ISUID; break;
	case 'S': res |= S_ISUID; break;
	case '-': break;
	default: return -1;
    }
    switch (*(p++)) {
	case 'r': res |= 0040; break;
	case '-': break;
	default: return -1;
    }
    switch (*(p++)) {
	case 'w': res |= 0020; break;
	case '-': break;
	default: return -1;
    }
    switch (*(p++)) {
	case 'x': res |= 0010; break;
	case 's': res |= 0010 | S_ISGID; break;
        case 'l': /* Solaris produces these */
	case 'S': res |= S_ISGID; break;
	case '-': break;
	default: return -1;
    }
    switch (*(p++)) {
	case 'r': res |= 0004; break;
	case '-': break;
	default: return -1;
    }
    switch (*(p++)) {
	case 'w': res |= 0002; break;
	case '-': break;
	default: return -1;
    }
    switch (*(p++)) {
	case 'x': res |= 0001; break;
	case 't': res |= 0001 | S_ISVTX; break;
	case 'T': res |= S_ISVTX; break;
	case '-': break;
	default: return -1;
    }
    return res;
}

static int vfs_parse_filedate(int idx, /*@out@*/ time_t *t)
	/*@modifies *t @*/
{	/* This thing parses from idx in columns[] array */

    char *p;
    struct tm tim;
    int d[3];
    int	got_year = 0;

    /* Let's setup default time values */
    tim.tm_year = current_year;
    tim.tm_mon  = current_mon;
    tim.tm_mday = current_mday;
    tim.tm_hour = 0;
    tim.tm_min  = 0;
    tim.tm_sec  = 0;
    tim.tm_isdst = -1; /* Let mktime() try to guess correct dst offset */

    p = columns [idx++];

    /* We eat weekday name in case of extfs */
    if(is_week(p, &tim))
	p = columns [idx++];

    /* Month name */
    if(is_month(p, &tim)){
	/* And we expect, it followed by day number */
	if (is_num (idx))
    	    tim.tm_mday = (int)atol (columns [idx++]);
	else
	    return 0; /* No day */

    } else {
        /* We usually expect:
           Mon DD hh:mm
           Mon DD  YYYY
	   But in case of extfs we allow these date formats:
           Mon DD YYYY hh:mm
	   Mon DD hh:mm YYYY
	   Wek Mon DD hh:mm:ss YYYY
           MM-DD-YY hh:mm
           where Mon is Jan-Dec, DD, MM, YY two digit day, month, year,
           YYYY four digit year, hh, mm, ss two digit hour, minute or second. */

	/* Here just this special case with MM-DD-YY */
        if (is_dos_date(p)){
/*@-mods@*/
            p[2] = p[5] = '-';
/*@=mods@*/

	    memset(d, 0, sizeof(d));
	    if (sscanf(p, "%2d-%2d-%2d", &d[0], &d[1], &d[2]) == 3){
	    /*  We expect to get:
		1. MM-DD-YY
		2. DD-MM-YY
		3. YY-MM-DD
		4. YY-DD-MM  */
		
		/* Hmm... maybe, next time :)*/
		
		/* At last, MM-DD-YY */
		d[0]--; /* Months are zerobased */
		/* Y2K madness */
		if(d[2] < 70)
		    d[2] += 100;

        	tim.tm_mon  = d[0];
        	tim.tm_mday = d[1];
        	tim.tm_year = d[2];
		got_year = 1;
	    } else
		return 0; /* sscanf failed */
        } else
            return 0; /* unsupported format */
    }

    /* Here we expect to find time and/or year */

    if (is_num (idx)) {
        if(is_time(columns[idx], &tim) || (got_year = is_year(columns[idx], &tim))) {
	idx++;

	/* This is a special case for ctime() or Mon DD YYYY hh:mm */
	if(is_num (idx) &&
	    ((got_year = is_year(columns[idx], &tim)) || is_time(columns[idx], &tim)))
		idx++; /* time & year or reverse */
	} /* only time or date */
    }
    else
        return 0; /* Nor time or date */

    /*
     * If the date is less than 6 months in the past, it is shown without year
     * other dates in the past or future are shown with year but without time
     * This does not check for years before 1900 ... I don't know, how
     * to represent them at all
     */
    if (!got_year &&
	current_mon < 6 && current_mon < tim.tm_mon &&
	tim.tm_mon - current_mon >= 6)

	tim.tm_year--;

    if ((*t = mktime(&tim)) < 0)
        *t = 0;
    return idx;
}

static int
vfs_parse_ls_lga (char * p, /*@out@*/ struct stat * st,
		/*@out@*/ const char ** filename,
		/*@out@*/ const char ** linkname)
	/*@modifies *p, *st, *filename, *linkname @*/
{
    int idx, idx2, num_cols;
    int i;
    char *p_copy;
    long n;

    if (strncmp (p, "total", 5) == 0)
        return 0;

    p_copy = g_strdup(p);
/* XXX FIXME: parse out inode number from "NLST -lai ." */
/* XXX FIXME: parse out sizein blocks from "NLST -lais ." */

    if ((i = vfs_parse_filetype(*(p++))) == -1)
        goto error;

    st->st_mode = i;
    if (*p == ' ')	/* Notwell 4 */
        p++;
    if (*p == '['){
	if (strlen (p) <= 8 || p [8] != ']')
	    goto error;
	/* Should parse here the Notwell permissions :) */
	/*@-unrecog@*/
	if (S_ISDIR (st->st_mode))
	    st->st_mode |= (S_IRUSR | S_IRGRP | S_IROTH | S_IWUSR | S_IXUSR | S_IXGRP | S_IXOTH);
	else
	    st->st_mode |= (S_IRUSR | S_IRGRP | S_IROTH | S_IWUSR);
	p += 9;
	/*@=unrecog@*/
    } else {
	if ((i = vfs_parse_filemode(p)) == -1)
	    goto error;
        st->st_mode |= i;
	p += 9;

        /* This is for an extra ACL attribute (HP-UX) */
        if (*p == '+')
            p++;
    }

    g_free(p_copy);
    p_copy = g_strdup(p);
    num_cols = vfs_split_text (p);

    n = atol(columns[0]);
    st->st_nlink = n;
    if (n < 0)
        goto error;

    if (!is_num (1))
#ifdef	HACK
	st->st_uid = finduid (columns [1]);
#else
	(void) unameToUid (columns [1], &st->st_uid);
#endif
    else
        st->st_uid = (uid_t) atol (columns [1]);

    /* Mhm, the ls -lg did not produce a group field */
    for (idx = 3; idx <= 5; idx++)
        if (is_month(columns [idx], NULL) || is_week(columns [idx], NULL) || is_dos_date(columns[idx]))
            break;

    if (idx == 6 || (idx == 5 && !S_ISCHR (st->st_mode) && !S_ISBLK (st->st_mode)))
	goto error;

    /* We don't have gid */	
    if (idx == 3 || (idx == 4 && (S_ISCHR(st->st_mode) || S_ISBLK (st->st_mode))))
        idx2 = 2;
    else {
	/* We have gid field */
	if (is_num (2))
	    st->st_gid = (gid_t) atol (columns [2]);
	else
#ifdef	HACK
	    st->st_gid = findgid (columns [2]);
#else
	    (void) gnameToGid (columns [1], &st->st_gid);
#endif
	idx2 = 3;
    }

    /* This is device */
    if (S_ISCHR (st->st_mode) || S_ISBLK (st->st_mode)){
	unsigned maj, min;
	
	if (!is_num (idx2) || sscanf(columns [idx2], " %d,", &maj) != 1)
	    goto error;
	
	if (!is_num (++idx2) || sscanf(columns [idx2], " %d", &min) != 1)
	    goto error;
	
#ifdef HAVE_ST_RDEV
	st->st_rdev = ((maj & 0x000000ffU) << 8) | (min & 0x000000ffU);
#endif
	st->st_size = 0;
	
    } else {
	/* Common file size */
	if (!is_num (idx2))
	    goto error;
	
	st->st_size = (size_t) atol (columns [idx2]);
#ifdef HAVE_ST_RDEV
	st->st_rdev = 0;
#endif
    }

    idx = vfs_parse_filedate(idx, &st->st_mtime);
    if (!idx)
        goto error;
    /* Use resulting time value */
    st->st_atime = st->st_ctime = st->st_mtime;
    st->st_dev = 0;
    st->st_ino = 0;
#ifdef HAVE_ST_BLKSIZE
    st->st_blksize = 512;
#endif
#ifdef HAVE_ST_BLOCKS
    st->st_blocks = (st->st_size + 511) / 512;
#endif

    for (i = idx + 1, idx2 = 0; i < num_cols; i++ )
	if (strcmp (columns [i], "->") == 0){
	    idx2 = i;
	    break;
	}

    if (((S_ISLNK (st->st_mode) ||
        (num_cols == idx + 3 && st->st_nlink > 1))) /* Maybe a hardlink? (in extfs) */
        && idx2)
    {
	size_t tlen;
	char *t;

	if (filename){
	    size_t nb = column_ptr [idx2] - column_ptr [idx] - 1;
	    t = strncpy((char *)xcalloc(1, nb+1), p_copy + column_ptr [idx], nb);
	    *filename = t;
	}
	if (linkname){
	    t = g_strdup (p_copy + column_ptr [idx2+1]);
	    tlen = strlen (t);
	    if (t [tlen-1] == '\r' || t [tlen-1] == '\n')
		t [tlen-1] = '\0';
	    if (t [tlen-2] == '\r' || t [tlen-2] == '\n')
		t [tlen-2] = '\0';
		
	    *linkname = t;
	}
    } else {
	/* Extract the filename from the string copy, not from the columns
	 * this way we have a chance of entering hidden directories like ". ."
	 */
	if (filename){
	    /*
	    *filename = g_strdup (columns [idx++]);
	    */
	    size_t tlen;
	    char *t;

	    t = g_strdup (p_copy + column_ptr [idx]); idx++;
	    tlen = strlen (t);
	    /* g_strchomp(); */
	    if (t [tlen-1] == '\r' || t [tlen-1] == '\n')
	        t [tlen-1] = '\0';
	    if (t [tlen-2] == '\r' || t [tlen-2] == '\n')
		t [tlen-2] = '\0';

	    *filename = t;
	}
	if (linkname)
	    *linkname = NULL;
    }
    g_free (p_copy);
    return 1;

error:
#ifdef	HACK
    {
      static int errorcount = 0;

      if (++errorcount < 5) {
	message_1s (1, "Could not parse:", p_copy);
      } else if (errorcount == 5)
	message_1s (1, "More parsing errors will be ignored.", "(sorry)" );
    }
#endif

    /*@-usereleased@*/
    if (p_copy != p)		/* Carefull! */
    /*@=usereleased@*/
	g_free (p_copy);
    return 0;
}

typedef enum {
	DO_FTP_STAT	= 1,
	DO_FTP_LSTAT	= 2,
	DO_FTP_READLINK	= 3,
	DO_FTP_ACCESS	= 4,
	DO_FTP_GLOB	= 5
} ftpSysCall_t;

/**
 */
static size_t ftpBufAlloced;

/**
 */
/*@only@*/ /*@relnull@*/
static char * ftpBuf;
	
#define alloca_strdup(_s)       strcpy((char *)alloca(strlen(_s)+1), (_s))

static int ftpNLST(const char * url, ftpSysCall_t ftpSysCall,
		/*@out@*/ /*@null@*/ struct stat * st,
		/*@out@*/ /*@null@*/ char * rlbuf, size_t rlbufsiz)
	/*@globals ftpBufAlloced, ftpBuf,
		h_errno, fileSystem, internalState @*/
	/*@modifies *st, *rlbuf, ftpBufAlloced, ftpBuf,
		fileSystem, internalState @*/
{
    FD_t fd;
    const char * path;
    int bufLength, moretodo;
    const char *n, *ne, *o, *oe;
    char * s;
    char * se;
    const char * urldn;
    char * bn = NULL;
    size_t nbn = 0;
    urlinfo u;
    int rc;

    n = ne = o = oe = NULL;
    (void) urlPath(url, &path);
    if (*path == '\0')
	return -2;

    switch (ftpSysCall) {
    case DO_FTP_GLOB:
	fd = ftpOpen(url, 0, 0, &u);
	if (fd == NULL || u == NULL)
	    return -1;

	u->openError = ftpReq(fd, "LIST", path);
	break;
    default:
	urldn = alloca_strdup(url);
	if ((bn = (char *) strrchr(urldn, '/')) == NULL)
	    return -2;
	else if (bn == path)
	    bn = (char *) ".";
	else
	    *bn++ = '\0';
	nbn = strlen(bn);

	rc = ftpChdir(urldn);		/* XXX don't care about CWD */
	if (rc < 0)
	    return rc;

	fd = ftpOpen(url, 0, 0, &u);
	if (fd == NULL || u == NULL)
	    return -1;

	/* XXX possibly should do "NLST -lais" to get st_ino/st_blocks also */
	u->openError = ftpReq(fd, "NLST", "-la");

	if (bn == NULL || nbn == 0) {
	    rc = -2;
	    goto exit;
	}
	break;
    }

    if (u->openError < 0) {
	fd = fdLink(fd, "error data (ftpStat)");
	rc = -2;
	goto exit;
    }

    if (ftpBufAlloced == 0 || ftpBuf == NULL) {
        ftpBufAlloced = _url_iobuf_size;
        ftpBuf = (char *) xcalloc(ftpBufAlloced, sizeof(ftpBuf[0]));
    }
    *ftpBuf = '\0';

    bufLength = 0;
    moretodo = 1;

    do {

	/* XXX FIXME: realloc ftpBuf if < ~128 chars remain */
	if ((ftpBufAlloced - bufLength) < (1024+80)) {
	    ftpBufAlloced <<= 2;
	    assert(ftpBufAlloced < (8*1024*1024));
	    ftpBuf = (char *) xrealloc(ftpBuf, ftpBufAlloced);
	}
	s = se = ftpBuf + bufLength;
	*se = '\0';

	rc = fdFgets(fd, se, (ftpBufAlloced - bufLength));
	if (rc <= 0) {
	    moretodo = 0;
	    break;
	}
	if (ftpSysCall == DO_FTP_GLOB) {	/* XXX HACK */
	    bufLength += strlen(se);
	    continue;
	}

	for (s = se; *s != '\0'; s = se) {
	    int bingo;

	    while (*se && *se != '\n') se++;
	    if (se > s && se[-1] == '\r') se[-1] = '\0';
	    if (*se == '\0')
		/*@innerbreak@*/ break;
	    *se++ = '\0';

	    if (!strncmp(s, "total ", sizeof("total ")-1))
		/*@innercontinue@*/ continue;

	    o = NULL;
	    for (bingo = 0, n = se; n >= s; n--) {
		switch (*n) {
		case '\0':
		    oe = ne = n;
		    /*@switchbreak@*/ break;
		case ' ':
		    if (o || !(n[-3] == ' ' && n[-2] == '-' && n[-1] == '>')) {
			while (*(++n) == ' ')
			    {};
			bingo++;
			/*@switchbreak@*/ break;
		    }
		    for (o = n + 1; *o == ' '; o++)
			{};
		    n -= 3;
		    ne = n;
		    /*@switchbreak@*/ break;
		default:
		    /*@switchbreak@*/ break;
		}
		if (bingo)
		    /*@innerbreak@*/ break;
	    }

	    if (nbn != (size_t)(ne - n))	/* Same name length? */
		/*@innercontinue@*/ continue;
	    if (strncmp(n, bn, nbn))	/* Same name? */
		/*@innercontinue@*/ continue;

	    moretodo = 0;
	    /*@innerbreak@*/ break;
	}

        if (moretodo && se > s) {
            bufLength = se - s - 1;
            if (s != ftpBuf)
                memmove(ftpBuf, s, bufLength);
        } else {
	    bufLength = 0;
	}
    } while (moretodo);

    switch (ftpSysCall) {
    case DO_FTP_STAT:
	if (o && oe) {
	    /* XXX FIXME: symlink, replace urldn/bn from [o,oe) and restart */
	}
	/*@fallthrough@*/
    case DO_FTP_LSTAT:
	if (st == NULL || !(n && ne)) {
	    rc = -1;
	} else {
	    rc = ((vfs_parse_ls_lga(s, st, NULL, NULL) > 0) ? 0 : -1);
	}
	break;
    case DO_FTP_READLINK:
	if (rlbuf == NULL || !(o && oe)) {
	    rc = -1;
	} else {
	    rc = oe - o;
assert(rc >= 0);
	    if (rc > (int)rlbufsiz)
		rc = (int)rlbufsiz;
	    memcpy(rlbuf, o, (size_t)rc);
	    if (rc < (int)rlbufsiz)
		rlbuf[rc] = '\0';
	}
	break;
    case DO_FTP_ACCESS:
	rc = 0;		/* XXX WRONG WRONG WRONG */
	break;
    case DO_FTP_GLOB:
	rc = 0;		/* XXX WRONG WRONG WRONG */
	break;
    }

exit:
    (void) ufdClose(fd);
    return rc;
}

static const char * statstr(const struct stat * st,
		/*@returned@*/ /*@out@*/ char * buf)
	/*@modifies *buf @*/
{
    char * t = buf;
    sprintf(t, "*** dev %x", (unsigned int)st->st_dev);
	t += strlen(t);
    sprintf(t, " ino %x", (unsigned int)st->st_ino);
	t += strlen(t);
    sprintf(t, " mode %0o", (unsigned int)st->st_mode);
	t += strlen(t);
    sprintf(t, " nlink %d", (unsigned int)st->st_nlink);
	t += strlen(t);
    sprintf(t, " uid %d", (unsigned int)st->st_uid);
	t += strlen(t);
    sprintf(t, " gid %d", (unsigned int)st->st_gid);
	t += strlen(t);
    sprintf(t, " rdev %x", (unsigned int)st->st_rdev);
	t += strlen(t);
    sprintf(t, " size %x", (unsigned int)st->st_size);
	t += strlen(t);
    sprintf(t, "\n");
    return buf;
}

/* FIXME: borked for path with trailing '/' */
static int ftpStat(const char * path, /*@out@*/ struct stat *st)
	/*@globals ftpBufAlloced, ftpBuf, h_errno, fileSystem, internalState @*/
	/*@modifies ftpBufAlloced, ftpBuf, *st, fileSystem, internalState @*/
{
    char buf[1024];
    int rc;
    rc = ftpNLST(path, DO_FTP_STAT, st, NULL, 0);

    /* XXX fts(3) needs/uses st_ino. */
    /* Hash the path to generate a st_ino analogue. */
    if (st->st_ino == 0)
	st->st_ino = hashFunctionString(0, path, 0);

if (_ftp_debug)
fprintf(stderr, "<-- %s(%s) rc %d\n%s", __FUNCTION__, path, rc, statstr(st, buf));
    return rc;
}

/* FIXME: borked for path with trailing '/' */
static int ftpLstat(const char * path, /*@out@*/ struct stat *st)
	/*@globals ftpBufAlloced, ftpBuf, h_errno, fileSystem, internalState @*/
	/*@modifies ftpBufAlloced, ftpBuf, *st, fileSystem, internalState @*/
{
    char buf[1024];
    int rc;
    rc = ftpNLST(path, DO_FTP_LSTAT, st, NULL, 0);

    /* XXX fts(3) needs/uses st_ino. */
    /* Hash the path to generate a st_ino analogue. */
    if (st->st_ino == 0)
	st->st_ino = hashFunctionString(0, path, 0);

if (_ftp_debug)
fprintf(stderr, "<-- %s(%s) rc %d\n%s\n", __FUNCTION__, path, rc, statstr(st, buf));
    return rc;
}

static int ftpReadlink(const char * path, /*@out@*/ char * buf, size_t bufsiz)
	/*@globals ftpBufAlloced, ftpBuf, h_errno, fileSystem, internalState @*/
	/*@modifies ftpBufAlloced, ftpBuf, *buf, fileSystem, internalState @*/
{
    int rc;
    rc = ftpNLST(path, DO_FTP_READLINK, NULL, buf, bufsiz);
if (_ftp_debug)
fprintf(stderr, "<-- %s(%s) rc %d\n", __FUNCTION__, path, rc);
    return rc;
}

DIR * ftpOpendir(const char * path)
	/*@globals ftpBufAlloced, ftpBuf @*/
	/*@modifies ftpBufAlloced, ftpBuf @*/
{
    AVDIR avdir;
    rpmavx avx;
    struct stat * st = NULL;
    const char * s, * sb, * se;
    int nac;
    int c;
    int rc;

if (_ftp_debug)
fprintf(stderr, "*** ftpOpendir(%s)\n", path);

    /* Load FTP collection into argv. */
    avx = (rpmavx) rpmavxNew(path, st);
    if (avx == NULL) {
        errno = ENOENT;         /* Note: avx is NULL iff urlSplit() fails. */
        return NULL;
    }

    rc = ftpNLST(path, DO_FTP_GLOB, NULL, NULL, 0);
    if (rc)
	return NULL;

    nac = 0;
    sb = NULL;
    s = se = ftpBuf;
    while ((c = (int) *se++) != (int) '\0') {
	switch (c) {
	case '/':
	    sb = se;
	    /*@switchbreak@*/ break;
	case '\r':
	    if (sb == NULL) {
		for (sb = se; sb > s && sb[-1] != ' '; sb--)
		    {};
	    }
	    nac++;

	    if (*se == '\n') se++;
	    sb = NULL;
	    s = se;
	    /*@switchbreak@*/ break;
	default:
	    /*@switchbreak@*/ break;
	}
    }

    avx->av = (const char **) xcalloc(nac+1, sizeof(*avx->av));
    avx->modes = (rpmuint16_t *) xcalloc(nac, sizeof(*avx->modes));

    nac = 0;
    sb = NULL;
    s = se = ftpBuf;
    while ((c = (int) *se) != (int) '\0') {
	se++;
	switch (c) {
	case '/':
	    sb = se;
	    /*@switchbreak@*/ break;
	case '\r':
	    if (sb == NULL) {
		avx->modes[nac] = (*s == 'd' ? 0755 : 0644);
		/*@-unrecog@*/
		switch(*s) {
		case 'p': avx->modes[nac] |= S_IFIFO; /*@innerbreak@*/ break;
		case 'c': avx->modes[nac] |= S_IFCHR; /*@innerbreak@*/ break;
		case 'd': avx->modes[nac] |= S_IFDIR; /*@innerbreak@*/ break;
		case 'b': avx->modes[nac] |= S_IFBLK; /*@innerbreak@*/ break;
		case '-': avx->modes[nac] |= S_IFREG; /*@innerbreak@*/ break;
		case 'l': avx->modes[nac] |= S_IFLNK; /*@innerbreak@*/ break;
		case 's': avx->modes[nac] |= S_IFSOCK; /*@innerbreak@*/ break;
		default:  avx->modes[nac] |= S_IFREG; /*@innerbreak@*/ break;
		}
		/*@=unrecog@*/
		for (sb = se; sb > s && sb[-1] != ' '; sb--)
		    {};
	    }
	    avx->av[nac++] = strncpy((char *)xcalloc(1, (se-sb-1)+1), sb, (se-sb-1));
	    if (*se == '\n') se++;
	    sb = NULL;
	    s = se;
	    /*@switchbreak@*/ break;
	default:
	    /*@switchbreak@*/ break;
	}
    }

    avdir = (AVDIR) avOpendir(path, avx->av, avx->modes);

    avx = rpmavxFree(avx);

/*@-kepttrans@*/
    return (DIR *) avdir;
/*@=kepttrans@*/
}

static char * ftpRealpath(const char * path, /*@null@*/ char * resolved_path)
	/*@*/
{
assert(resolved_path == NULL);	/* XXX no POSIXly broken realpath(3) here. */
    /* XXX TODO: handle redirects. For now, just dupe the path. */
    return xstrdup(path);
}

int Stat(const char * path, struct stat * st)
	/*@globals ftpBufAlloced, ftpBuf @*/
	/*@modifies ftpBufAlloced, ftpBuf @*/
{
    const char * lpath;
    int ut = urlPath(path, &lpath);
    int rc = -2;

    switch (ut) {
    case URL_IS_FTP:
	rc = ftpStat(path, st);
	goto exit;
	/*@notreached@*/ break;
    case URL_IS_HTTPS:
    case URL_IS_HTTP:
#ifdef WITH_NEON
	rc = davStat(path, st);
#endif
	goto exit;
	/*@notreached@*/ break;
    case URL_IS_PATH:
	path = lpath;
	/*@fallthrough@*/
    case URL_IS_UNKNOWN:
	break;
    case URL_IS_DASH:
    case URL_IS_HKP:
    case URL_IS_MONGO:	/* XXX FIXME */
    default:
	errno = ENOENT;	
	goto exit;
	/*@notreached@*/ break;
    }
    rc = stat(path, st);
exit:
if (_rpmio_debug)
fprintf(stderr, "<-- %s(%s,%p) rc %d\n", __FUNCTION__, path, st, rc);
    return rc;
}

int Lstat(const char * path, struct stat * st)
	/*@globals ftpBufAlloced, ftpBuf @*/
	/*@modifies ftpBufAlloced, ftpBuf @*/
{
    const char * lpath;
    int ut = urlPath(path, &lpath);
    int rc = -2;

    switch (ut) {
    case URL_IS_FTP:
	rc = ftpLstat(path, st);
	goto exit;
	/*@notreached@*/ break;
    case URL_IS_HTTPS:
    case URL_IS_HTTP:
#ifdef WITH_NEON
	rc = davLstat(path, st);
#endif
	goto exit;
	/*@notreached@*/ break;
    case URL_IS_PATH:
	path = lpath;
	/*@fallthrough@*/
    case URL_IS_UNKNOWN:
	break;
    case URL_IS_DASH:
    case URL_IS_HKP:
    case URL_IS_MONGO:	/* XXX FIXME */
    default:
	errno = ENOENT;	
	goto exit;
	/*@notreached@*/ break;
    }
    rc = lstat(path, st);
exit:
if (_rpmio_debug)
fprintf(stderr, "<-- %s(%s,%p) rc %d\n", __FUNCTION__, path, st, rc);
    return rc;
}

int Fstat(FD_t fd, struct stat * st)
{
    const char * path = fdGetOPath(fd);
    const char * lpath;
    int ut = urlPath(path, &lpath);
    int rc = -2;

    if (fd == NULL || path == NULL || *path == '\0' || st == NULL) {
	errno = ENOENT;
	goto exit;
    }

    switch (ut) {
    case URL_IS_DASH:
    case URL_IS_PATH:
    case URL_IS_UNKNOWN:
	break;
    case URL_IS_FTP:
    case URL_IS_HTTPS:
    case URL_IS_HTTP:
    case URL_IS_HKP:
	if (fd->contentLength < 0) {
	    errno = ENOENT;
	    goto exit;
	}
	memset(st, 0, sizeof(*st));
	if (path[strlen(path)-1] == '/') {
	    st->st_nlink = 2;
	    st->st_mode = (S_IFDIR | 0755);
	} else {
	    st->st_nlink = 1;
	    st->st_mode = (S_IFREG | 0644);
	}
	st->st_ino = hashFunctionString(0, path, 0);;
	st->st_size = fd->contentLength;
	st->st_mtime = fd->lastModified;

	st->st_atime = st->st_ctime = st->st_mtime;
	st->st_blksize = 4 * 1024;  /* HACK correct for linux ext */
	st->st_blocks = (st->st_size + 511)/512;
	break;
    case URL_IS_MONGO:	/* XXX FIXME */
    default:
	errno = ENOENT;	
	goto exit;
	/*@notreached@*/ break;
    }
    rc = fstat(Fileno(fd), st);
exit:
if (_rpmio_debug)
fprintf(stderr, "<-- %s(%p,%p) path %s rc %d\n", __FUNCTION__, fd, st, path, rc);
    return rc;
}

int Fadvise(FD_t fd, off_t offset, off_t len, int advice)
{
    const char * path = fdGetOPath(fd);
    const char * lpath;
    int ut = urlPath(path, &lpath);
    int fdno = Fileno(fd);
    int rc;

if (_rpmio_debug)
fprintf(stderr, "*** %s(%p,0x%x,0x%x,0x%x) fdno %d path %s\n", __FUNCTION__, fd, (unsigned)offset, (unsigned)len, advice, fdno, path);

    /* XXX errno is not set by fallocate/posix_fallocate */
    if (fd == NULL || fdno < 0) {
	rc = EBADF;
	return rc;
    }

    switch (ut) {
    case URL_IS_PATH:
    case URL_IS_UNKNOWN:
	break;
    default:
	rc = ENODEV;	
	return rc;
	/*@notreached@*/ break;
    }

    switch (advice) {
#if defined(HAVE_POSIX_FADVISE)
    case POSIX_FADV_NORMAL:
    case POSIX_FADV_SEQUENTIAL:
    case POSIX_FADV_RANDOM:
    case POSIX_FADV_NOREUSE:
    case POSIX_FADV_WILLNEED:
    case POSIX_FADV_DONTNEED:
	rc = posix_fadvise(fdno, offset, len, advice);
#else
	rc = ENOSYS;
#endif
	break;
    default:
	rc = EINVAL;
	break;
    }

    if (rc != 0)
	rpmlog(RPMLOG_DEBUG, _("%s(%d,0x%x,0x%x) failed: rc %d\n"),
		__FUNCTION__, fdno, (unsigned)offset, (unsigned)len, rc);

    return rc;
}

#undef	HAVE_FALLOCATE	/* XXX hmmm, fallocate64 is AWOL in F11. */
int Fallocate(FD_t fd, off_t offset, off_t len)
{
    const char * path = fdGetOPath(fd);
    const char * lpath;
    int ut = urlPath(path, &lpath);
    int fdno = Fileno(fd);
    int rc;

if (_rpmio_debug)
fprintf(stderr, "*** %s(%p,0x%x,0x%x) fdno %d path %s\n", __FUNCTION__, fd, (unsigned)offset, (unsigned)len, fdno, path);

    /* XXX errno is not set by fallocate/posix_fallocate */
    if (fd == NULL || fdno < 0) {
	rc = EBADF;
	return rc;
    }

    switch (ut) {
    case URL_IS_PATH:
    case URL_IS_UNKNOWN:
	break;
    default:
	rc = ENODEV;	
	return rc;
	/*@notreached@*/ break;
    }

#if defined(HAVE_FALLOCATE)
    /* XXX linux FALLOC_FL_KEEP_SIZE zeroes allocated blocks */
    rc = (int) fallocate(fdno, 0, (loff_t)offset, (loff_t)len);
#elif defined(HAVE_POSIX_FALLOCATE)
    rc = posix_fallocate(fdno, offset, len);
#else
    rc = ENOSYS;
#endif

    if (rc != 0)
	rpmlog(RPMLOG_DEBUG, _("%s(%d,0x%x,0x%x) failed: rc %d\n"),
		__FUNCTION__, fdno, (unsigned)offset, (unsigned)len, rc);

    return rc;
}

#ifdef	NOTYET	/* XXX figger mmap or posix_memalign first */
void *Mmap(void *addr, size_t length, int prot, int flags,
                  FD_t fd, off_t offset)
{
    int fdno = (fd ? Fileno(fd) : -1);
    void * ret = mmap(addr, length, prot, flags, fdno, offset);
    if (ret == NULL || ret == (void *)-1)
	rpmlog(RPMLOG_ERR, _("%s(%p[%u],0x%x,0x%x,%p,0x%x) failed: %m\n"),
		__FUNCTION__, addr, (unsigned)len, prot, flags, fd,
		(unsigned)offset);
if (_rpmio_debug)
fprintf(stderr, "<-- %s(%p[%u],0x%x,0x%x,%p,0x%x) ret %p\n", __FUNCTION__, addr, (unsigned)len, prot, flags, fd, (unsigned)offset, ret);
    return ret;
}

int Munmap(const void * addr, size_t len)
{
    int rc = munmap(addr, len);
    if (rc < 0)
	rpmlog(RPMLOG_ERR, _("%s(%p[%u]) failed: %m\n"),
		__FUNCTION__, addr, (unsigned)len);
if (_rpmio_debug)
fprintf(stderr, "<-- %s(%p[%u]) rc %d\n", __FUNCTION__, addr, (unsigned)len, rc);
    return rc;
}

int Mprotect(const void * addr, size_t len, int prot)
{
    int rc = mprotect(addr, len, prot);
    if (rc < 0)
	rpmlog(RPMLOG_ERR, _("%s(%p[%u],%d) failed: %m\n"),
		__FUNCTION__, addr, (unsigned)len, prot);
if (_rpmio_debug)
fprintf(stderr, "<-- %s(%p[%u],%d) rc %d\n", __FUNCTION__, addr, len, prot, rc);
    return rc;
}
#endif

#ifdef	NOTYET	/* XXX figger posix_fadvise or posix_madvise first */
int Madvise(void *addr, size_t len, int advice)
{
    int rc = madvise(addr, len, advice);
    if (rc < 0)
	rpmlog(RPMLOG_ERR, _("%s(%p[%u],%d) failed: %m\n"),
		__FUNCTION__, addr, (unsigned)len, advice);
if (_rpmio_debug)
fprintf(stderr, "<-- %s(%p[%u],%d) rc %d\n", __FUNCTION__, addr, len, advice, rc);
    return rc;
}

int Fadvise(FD_t fd, off_t offset, off_t len, int advice)
{
    const char * path = fdGetOPath(fd);
    const char * lpath;
    int ut = urlPath(path, &lpath);
    int fdno = Fileno(fd);
    int rc = -2;

    if (fd == NULL || fdno < 0) {
	errno = EBADF;
	goto exit;
    }

    switch (ut) {
    case URL_IS_PATH:
    case URL_IS_UNKNOWN:
	break;
    default:
	errno = EINVAL;	
	goto exit;
	/*@notreached@*/ break;
    }
    rc = posix_fadvise(fdno, offset, len, advice);
    if (rc != 0)
	rpmlog(RPMLOG_ERR, _("%s(%d,%d,0x%x,0x%x) failed: %m\n"),
		__FUNCTION__, fdno, (unsigned)offset, (unsigned)len, advice);
exit:
if (_rpmio_debug)
fprintf(stderr, "<-- %s(%p,0x%x,0x%x,%d) fdno %d path %s rc %d\n", __FUNCTION__, fd, (unsigned)offset, (unsigned)len, advice, fdno, path, rc);
    return rc;
}
#endif

int Chown(const char * path, uid_t owner, gid_t group)
{
    const char * lpath;
    int ut = urlPath(path, &lpath);
    int rc = -2;

    switch (ut) {
    case URL_IS_PATH:
	path = lpath;
	/*@fallthrough@*/
    case URL_IS_UNKNOWN:
	break;
    case URL_IS_DASH:
    case URL_IS_HKP:
    case URL_IS_FTP:		/* XXX TODO: implement. */
    case URL_IS_HTTP:		/* XXX TODO: implement. */
    case URL_IS_HTTPS:		/* XXX TODO: implement. */
    case URL_IS_MONGO:	/* XXX FIXME */
    default:
	errno = EINVAL;		/* XXX W2DO? */
	goto exit;
	/*@notreached@*/ break;
    }
    rc = chown(path, owner, group);
exit:
if (_rpmio_debug)
fprintf(stderr, "<-- %s(%s,%u,%u) rc %d\n", __FUNCTION__, path, (unsigned)owner, (unsigned)group, rc);
    return rc;
}

int Fchown(FD_t fd, uid_t owner, gid_t group)
{
    const char * path = fdGetOPath(fd);
    const char * lpath;
    int ut = urlPath(path, &lpath);
    int rc = -2;

    switch (ut) {
    case URL_IS_PATH:
	path = lpath;
	/*@fallthrough@*/
    case URL_IS_UNKNOWN:
	break;
    case URL_IS_DASH:
    case URL_IS_HKP:
    case URL_IS_FTP:		/* XXX TODO: implement. */
    case URL_IS_HTTP:		/* XXX TODO: implement. */
    case URL_IS_HTTPS:		/* XXX TODO: implement. */
    case URL_IS_MONGO:	/* XXX FIXME */
    default:
	errno = EINVAL;		/* XXX W2DO? */
	goto exit;
	/*@notreached@*/ break;
    }
    rc = fchown(Fileno(fd), owner, group);
exit:
if (_rpmio_debug)
fprintf(stderr, "<-- %s(%p,%u,%u) path %s rc %d\n", __FUNCTION__, fd, (unsigned)owner, (unsigned)group, path, rc);
    return rc;
}

int Lchown(const char * path, uid_t owner, gid_t group)
{
    const char * lpath;
    int ut = urlPath(path, &lpath);
    int rc = -2;

    switch (ut) {
    case URL_IS_PATH:
	path = lpath;
	/*@fallthrough@*/
    case URL_IS_UNKNOWN:
	break;
    case URL_IS_DASH:
    case URL_IS_HKP:
    case URL_IS_FTP:		/* XXX TODO: implement. */
    case URL_IS_HTTP:		/* XXX TODO: implement. */
    case URL_IS_HTTPS:		/* XXX TODO: implement. */
    case URL_IS_MONGO:	/* XXX FIXME */
    default:
	errno = EINVAL;		/* XXX W2DO? */
	goto exit;
	/*@notreached@*/ break;
    }
    rc = lchown(path, owner, group);
exit:
if (_rpmio_debug)
fprintf(stderr, "*** %s(%s,%u,%u)\n", __FUNCTION__, path, (unsigned)owner, (unsigned)group);
    return rc;
}

int Chmod(const char * path, mode_t mode)
{
    const char * lpath;
    int ut = urlPath(path, &lpath);
    int rc = -2;

    switch (ut) {
    case URL_IS_PATH:
	path = lpath;
	/*@fallthrough@*/
    case URL_IS_UNKNOWN:
	break;
    case URL_IS_DASH:
    case URL_IS_HKP:
    case URL_IS_FTP:		/* XXX TODO: implement. */
    case URL_IS_HTTP:		/* XXX TODO: implement. */
    case URL_IS_HTTPS:		/* XXX TODO: implement. */
    case URL_IS_MONGO:	/* XXX FIXME */
    default:
	errno = EINVAL;		/* XXX W2DO? */
	goto exit;
	/*@notreached@*/ break;
    }
    rc = chmod(path, mode);
exit:
if (_rpmio_debug)
fprintf(stderr, "<-- %s(%s,%0o) rc %d\n", __FUNCTION__, path, (int)mode, rc);
    return rc;
}

int Lchmod(const char * path, mode_t mode)
{
#if defined(HAVE_LCHMOD)
    const char * lpath;
    int ut = urlPath(path, &lpath);
    int rc = -2;

    switch (ut) {
    case URL_IS_PATH:
	path = lpath;
	/*@fallthrough@*/
    case URL_IS_UNKNOWN:
	break;
    case URL_IS_DASH:
    case URL_IS_HKP:
    case URL_IS_FTP:		/* XXX TODO: implement. */
    case URL_IS_HTTP:		/* XXX TODO: implement. */
    case URL_IS_HTTPS:		/* XXX TODO: implement. */
    case URL_IS_MONGO:	/* XXX FIXME */
    default:
	errno = EINVAL;		/* XXX W2DO? */
	goto exit;
	/*@notreached@*/ break;
    }
    rc = lchmod(path, mode);
exit:
if (_rpmio_debug)
fprintf(stderr, "<-- %s(%s,%0o) rc %d\n", __FUNCTION__, path, (int)mode, rc);
    return rc;
#else
    errno = ENOSYS;
    return -2;
#endif
}

int Fchmod(FD_t fd, mode_t mode)
{
    const char * path = fdGetOPath(fd);
    const char * lpath;
    int ut = urlPath(path, &lpath);
    int rc = -2;

    switch (ut) {
    case URL_IS_PATH:
	path = lpath;
	/*@fallthrough@*/
    case URL_IS_UNKNOWN:
	break;
    case URL_IS_DASH:
    case URL_IS_HKP:
    case URL_IS_FTP:		/* XXX TODO: implement. */
    case URL_IS_HTTP:		/* XXX TODO: implement. */
    case URL_IS_HTTPS:		/* XXX TODO: implement. */
    case URL_IS_MONGO:	/* XXX FIXME */
    default:
	errno = EINVAL;		/* XXX W2DO? */
	goto exit;
	/*@notreached@*/ break;
    }
    rc = fchmod(Fileno(fd), mode);
exit:
if (_rpmio_debug)
fprintf(stderr, "*** %s(%p,%0o) path %s rc %d\n", __FUNCTION__, fd, (int)mode, path, rc);
    return rc;
}

int Chflags(const char * path, unsigned int flags)
{
#if defined(HAVE_CHFLAGS)
    const char * lpath;
    int ut = urlPath(path, &lpath);

if (_rpmio_debug)
fprintf(stderr, "*** Chflags(%s,0x%x)\n", path, flags);
    switch (ut) {
    case URL_IS_PATH:
	path = lpath;
	/*@fallthrough@*/
    case URL_IS_UNKNOWN:
	break;
    case URL_IS_DASH:
    case URL_IS_HKP:
    case URL_IS_FTP:		/* XXX TODO: implement. */
    case URL_IS_HTTP:		/* XXX TODO: implement. */
    case URL_IS_HTTPS:		/* XXX TODO: implement. */
    case URL_IS_MONGO:	/* XXX FIXME */
    default:
	errno = EINVAL;		/* XXX W2DO? */
	return -2;
	/*@notreached@*/ break;
    }
    return chflags(path, flags);
#else
    errno = ENOSYS;
    return -2;
#endif
}

int Lchflags(const char * path, unsigned int flags)
{
    const char * lpath;
    int ut = urlPath(path, &lpath);
    int rc = -2;

#if defined(HAVE_LCHFLAGS)
if (_rpmio_debug)
fprintf(stderr, "*** Lchflags(%s,0x%x)\n", path, flags);
    switch (ut) {
    case URL_IS_PATH:
	path = lpath;
	/*@fallthrough@*/
    case URL_IS_UNKNOWN:
	break;
    case URL_IS_DASH:
    case URL_IS_HKP:
    case URL_IS_FTP:		/* XXX TODO: implement. */
    case URL_IS_HTTP:		/* XXX TODO: implement. */
    case URL_IS_HTTPS:		/* XXX TODO: implement. */
    case URL_IS_MONGO:	/* XXX FIXME */
    default:
	errno = EINVAL;		/* XXX W2DO? */
	return -2;
	/*@notreached@*/ break;
    }
    return lchflags(path, flags);
#else
    ut = ut;	/* keep gcc happy */
    errno = ENOSYS;
#endif
    return rc;
}

int Fchflags(FD_t fd, unsigned int flags)
{
#if defined(HAVE_FCHFLAGS)
    const char * path = fdGetOPath(fd);
    const char * lpath;
    int ut = urlPath(path, &lpath);

if (_rpmio_debug)
fprintf(stderr, "*** Fchflags(%p,0x%x) path %s\n", fd, flags, path);
    switch (ut) {
    case URL_IS_PATH:
	path = lpath;
	/*@fallthrough@*/
    case URL_IS_UNKNOWN:
	break;
    case URL_IS_DASH:
    case URL_IS_HKP:
    case URL_IS_FTP:		/* XXX TODO: implement. */
    case URL_IS_HTTP:		/* XXX TODO: implement. */
    case URL_IS_HTTPS:		/* XXX TODO: implement. */
    case URL_IS_MONGO:	/* XXX FIXME */
    default:
	errno = EINVAL;		/* XXX W2DO? */
	return -2;
	/*@notreached@*/ break;
    }
    return fchflags(Fileno(fd), flags);
#else
    errno = ENOSYS;
    return -2;
#endif
}
int Mkfifo(const char * path, mode_t mode)
{
    const char * lpath;
    int ut = urlPath(path, &lpath);
    int rc = -2;

    switch (ut) {
    case URL_IS_PATH:
	path = lpath;
	/*@fallthrough@*/
    case URL_IS_UNKNOWN:
	break;
    case URL_IS_DASH:
    case URL_IS_HKP:
    case URL_IS_FTP:		/* XXX TODO: implement. */
    case URL_IS_HTTP:		/* XXX TODO: implement. */
    case URL_IS_HTTPS:		/* XXX TODO: implement. */
    case URL_IS_MONGO:	/* XXX FIXME */
    default:
	errno = EINVAL;		/* XXX W2DO? */
	goto exit;
	/*@notreached@*/ break;
    }
    rc = mkfifo(path, mode);
exit:
if (_rpmio_debug)
fprintf(stderr, "<-- %s(%s,%0o) rc %d\n", __FUNCTION__, path, (int)mode, rc);
    return rc;
}

int Mknod(const char * path, mode_t mode, dev_t dev)
{
    const char * lpath;
    int ut = urlPath(path, &lpath);
    int rc = -2;

    switch (ut) {
    case URL_IS_PATH:
	path = lpath;
	/*@fallthrough@*/
    case URL_IS_UNKNOWN:
	break;
    case URL_IS_DASH:
    case URL_IS_HKP:
    case URL_IS_FTP:		/* XXX TODO: implement. */
    case URL_IS_HTTP:		/* XXX TODO: implement. */
    case URL_IS_HTTPS:		/* XXX TODO: implement. */
    case URL_IS_MONGO:	/* XXX FIXME */
    default:
	errno = EINVAL;		/* XXX W2DO? */
	goto exit;
	/*@notreached@*/ break;
    }
/*@-portability@*/
    rc = mknod(path, mode, dev);
/*@=portability@*/
exit:
if (_rpmio_debug)
fprintf(stderr, "<-- %s(%s,%0o, 0x%x) rc %d\n", __FUNCTION__, path, (int)mode, (int)dev, rc);
    return rc;
}

int Utime(const char * path, const struct utimbuf *buf)
{
    const char * lpath;
    int ut = urlPath(path, &lpath);
    int rc = -2;

    switch (ut) {
    case URL_IS_PATH:
	path = lpath;
	/*@fallthrough@*/
    case URL_IS_UNKNOWN:
	break;
    case URL_IS_DASH:
    case URL_IS_HKP:
    case URL_IS_FTP:		/* XXX TODO: implement. */
    case URL_IS_HTTP:		/* XXX TODO: implement. */
    case URL_IS_HTTPS:		/* XXX TODO: implement. */
    case URL_IS_MONGO:	/* XXX FIXME */
    default:
	errno = EINVAL;		/* XXX W2DO? */
	goto exit;
	/*@notreached@*/ break;
    }
    rc = utime(path, buf);
exit:
if (_rpmio_debug)
fprintf(stderr, "<-- %s(%s,%p) rc %d\n", __FUNCTION__, path, buf, rc);
    return rc;
}

/*@-fixedformalarray@*/
int Utimes(const char * path, const struct timeval times[2])
{
    const char * lpath;
    int ut = urlPath(path, &lpath);
    int rc = -2;

    switch (ut) {
    case URL_IS_PATH:
	path = lpath;
	/*@fallthrough@*/
    case URL_IS_UNKNOWN:
	break;
    case URL_IS_DASH:
    case URL_IS_HKP:
    case URL_IS_FTP:		/* XXX TODO: implement. */
    case URL_IS_HTTP:		/* XXX TODO: implement. */
    case URL_IS_HTTPS:		/* XXX TODO: implement. */
    case URL_IS_MONGO:	/* XXX FIXME */
    default:
	errno = EINVAL;		/* XXX W2DO? */
	goto exit;
	/*@notreached@*/ break;
    }
    rc = utimes(path, times);
exit:
if (_rpmio_debug)
fprintf(stderr, "<-- %s(%s,%p) rc %d\n", __FUNCTION__, path, times, rc);
    return rc;
}
/*@=fixedformalarray@*/

/*@-fixedformalarray@*/
int Lutimes(const char * path, const struct timeval times[2])
{
#ifdef HAVE_LUTIMES
    const char * lpath;
    int ut = urlPath(path, &lpath);
    int rc = -2;

    switch (ut) {
    case URL_IS_PATH:
	path = lpath;
	/*@fallthrough@*/
    case URL_IS_UNKNOWN:
	break;
    case URL_IS_DASH:
    case URL_IS_HKP:
    case URL_IS_FTP:		/* XXX TODO: implement. */
    case URL_IS_HTTP:		/* XXX TODO: implement. */
    case URL_IS_HTTPS:		/* XXX TODO: implement. */
    case URL_IS_MONGO:	/* XXX FIXME */
    default:
	errno = EINVAL;		/* XXX W2DO? */
	goto exit;
	/*@notreached@*/ break;
    }
    rc = lutimes(path, times);
exit:
if (_rpmio_debug)
fprintf(stderr, "<-- %s(%s,%p) rc %d\n", __FUNCTION__, path, times, rc);
    return rc;
#else
    errno = ENOSYS;
    return -2;
#endif
}
/*@=fixedformalarray@*/

int Symlink(const char * oldpath, const char * newpath)
{
    const char * opath;
    int out = urlPath(oldpath, &opath);
    const char * npath;
    int nut = urlPath(newpath, &npath);
    int rc = -2;

    nut = nut;	/* XXX keep gcc quiet. */
    switch (out) {
    case URL_IS_PATH:
	oldpath = opath;
	newpath = npath;
	/*@fallthrough@*/
    case URL_IS_UNKNOWN:
	break;
    case URL_IS_DASH:
    case URL_IS_HKP:
    case URL_IS_FTP:		/* XXX TODO: implement. */
    case URL_IS_HTTP:		/* XXX TODO: implement. */
    case URL_IS_HTTPS:		/* XXX TODO: implement. */
    case URL_IS_MONGO:	/* XXX FIXME */
    default:
	errno = EINVAL;		/* XXX W2DO? */
	goto exit;
	/*@notreached@*/ break;
    }
    rc = symlink(oldpath, newpath);
exit:
if (_rpmio_debug)
fprintf(stderr, "<-- %s(%s,%s) rc %d\n", __FUNCTION__, oldpath, newpath, rc);
    return rc;
}

int Readlink(const char * path, char * buf, size_t bufsiz)
	/*@globals ftpBufAlloced, ftpBuf @*/
	/*@modifies ftpBufAlloced, ftpBuf @*/
{
    const char * lpath;
    int ut = urlPath(path, &lpath);
    int rc = -2;

    switch (ut) {
    case URL_IS_FTP:
	rc = ftpReadlink(path, buf, bufsiz);
	goto exit;
	/*@notreached@*/ break;
    case URL_IS_HTTPS:
    case URL_IS_HTTP:
#ifdef	NOTYET
	rc = davReadlink(path, buf, bufsiz);
#endif
	goto exit;
	/*@notreached@*/ break;
    case URL_IS_PATH:
	path = lpath;
	/*@fallthrough@*/
    case URL_IS_UNKNOWN:
	break;
    case URL_IS_DASH:
    case URL_IS_HKP:
    case URL_IS_MONGO:	/* XXX FIXME */
    default:
	errno = EINVAL;		/* XXX W2DO? */
	goto exit;
	/*@notreached@*/ break;
    }
/*@-compdef@*/ /* FIX: *buf is undefined */
    rc = readlink(path, buf, bufsiz);
/*@=compdef@*/
exit:
if (_rpmio_debug)
fprintf(stderr, "<-- %s(%s,%p[%u]) rc %d\n", __FUNCTION__, path, buf, (unsigned)bufsiz, rc);
    return rc;
}

int Access(const char * path, int amode)
{
    const char * lpath;
    int ut = urlPath(path, &lpath);
    int rc = -2;

    switch (ut) {
    case URL_IS_PATH:
	path = lpath;
	/*@fallthrough@*/
    case URL_IS_UNKNOWN:
	break;
    case URL_IS_DASH:
    case URL_IS_HKP:
    case URL_IS_FTP:		/* XXX TODO: implement. */
    case URL_IS_HTTP:		/* XXX TODO: implement. */
    case URL_IS_HTTPS:		/* XXX TODO: implement. */
    case URL_IS_MONGO:	/* XXX FIXME */
    default:
	errno = EINVAL;		/* XXX W2DO? */
	goto exit;
	/*@notreached@*/ break;
    }
    rc = access(path, amode);
exit:
if (_rpmio_debug)
fprintf(stderr, "<-- %s(%s,%d) rc %d\n", __FUNCTION__, path, amode, rc);
    return rc;
}

/* glob_pattern_p() taken from bash
 * Copyright (C) 1985, 1988, 1989 Free Software Foundation, Inc.
 *
 * Return nonzero if PATTERN has any special globbing chars in it.
 */
int Glob_pattern_p (const char * pattern, int quote)
{
    const char *p;
    int ut = urlPath(pattern, &p);
    int open = 0;
    char c;

    while ((c = *p++) != '\0')
	switch (c) {
	case '?':
	    /* Don't treat '?' as a glob char in HTTP URL's */
	    if (ut == URL_IS_HTTPS || ut == URL_IS_HTTP || ut == URL_IS_HKP)
		continue;
	    /*@fallthrough@*/
	case '*':
	    return (1);
	case '\\':
	    if (quote && *p != '\0')
		p++;
	    continue;

	case '[':
	    open = 1;
	    continue;
	case ']':
	    if (open)
		return (1);
	    continue;

	case '+':
	case '@':
	case '!':
	    if (*p == '(')
		return (1);
	    continue;
	}

    return (0);
}

int Glob_error(/*@unused@*/ const char * epath,
		/*@unused@*/ int eerrno)
{
    return 1;
}

int Glob(const char *pattern, int flags,
	int errfunc(const char * epath, int eerrno), void *_pglob)
{
    glob_t *pglob = (glob_t *) _pglob;
    const char * lpath;
    int ut = urlPath(pattern, &lpath);
    const char *home = getenv("HOME");

/*@-castfcnptr@*/
if (_rpmio_debug)
fprintf(stderr, "*** Glob(%s,0x%x,%p,%p)\n", pattern, (unsigned)flags, (void *)errfunc, pglob);
/*@=castfcnptr@*/
    switch (ut) {
    case URL_IS_HTTPS:
    case URL_IS_HTTP:
    case URL_IS_FTP:
/*@-type@*/
	pglob->gl_closedir = (void (*)(void *)) Closedir;
	pglob->gl_readdir = (struct dirent * (*)(void *)) Readdir;
	pglob->gl_opendir = (void * (*)(const char *)) Opendir;
	pglob->gl_lstat = Lstat;
	pglob->gl_stat = Stat;
/*@=type@*/
	flags |= GLOB_ALTDIRFUNC;
	flags &= ~GLOB_TILDE;
	break;
    case URL_IS_PATH:
	pattern = lpath;
	/*@fallthrough@*/
    case URL_IS_UNKNOWN:
	if (home && home[0])
	    flags |= GLOB_TILDE;
	else
	    flags &= ~GLOB_TILDE;
	break;
    case URL_IS_DASH:
    case URL_IS_HKP:
    case URL_IS_MONGO:	/* XXX FIXME */
    default:
	return -2;
	/*@notreached@*/ break;
    }
    return glob(pattern, flags, errfunc, pglob);
}

void Globfree(void *_pglob)
{
    glob_t *pglob = (glob_t *) _pglob;
if (_rpmio_debug)
fprintf(stderr, "*** Globfree(%p)\n", pglob);
    globfree(pglob);
}

char * Realpath(const char * path, /*@null@*/ char * resolved_path)
{
    const char * lpath;
    int ut = urlPath(path, &lpath);
    char * rpath = NULL;

if (_rpmio_debug)
fprintf(stderr, "*** Realpath(%s, %s)\n", path, (resolved_path ? resolved_path : "NULL"));
#if !defined(__LCLINT__) /* XXX LCL: realpath(3) annotations are buggy. */
/*@-nullpass@*/
    /* XXX if POSIXly broken realpath(3) is desired, do that. */
    /* XXX note: preserves current rpmlib realpath(3) usage cases. */
    if (path == NULL || resolved_path != NULL)
	return realpath(path, resolved_path);
/*@=nullpass@*/
#endif	/* !__LCLINT__ */

    switch (ut) {
    case URL_IS_FTP:
	return ftpRealpath(path, resolved_path);
	/*@notreached@*/ break;
    case URL_IS_HTTPS:	
    case URL_IS_HTTP:
    case URL_IS_HKP:
#ifdef WITH_NEON
	return davRealpath(path, resolved_path);
	/*@notreached@*/ break;
#endif
	/*@fallthrough@*/
    case URL_IS_MONGO:	/* XXX FIXME */
    default:
	return xstrdup(path);
	/*@notreached@*/ break;
    case URL_IS_DASH:
	/* non-GLIBC systems => EINVAL. non-linux systems => EINVAL */
#if defined(__linux__)
	lpath = "/dev/stdin";
#else
	lpath = NULL;
#endif
	break;
    case URL_IS_PATH:		/* XXX note: file:/// prefix is dropped. */
    case URL_IS_UNKNOWN:
	path = lpath;
	break;
    }

#if !defined(__LCLINT__) /* XXX LCL: realpath(3) annotations are buggy. */
    if (lpath == NULL || *lpath == '/')
/*@-nullpass@*/	/* XXX glibc extension */
	rpath = realpath(lpath, resolved_path);
/*@=nullpass@*/
    else {
	char * t;
#if defined(__GLIBC__)
	char * dn = NULL;
#else
	char dn[PATH_MAX];
	dn[0] = '\0';
#endif
	/*
	 * Using realpath on lpath isn't correct if the lpath is a symlink,
	 * especially if the symlink is a dangling link.  What we 
	 * do instead is use realpath() on `.' and then append lpath to
	 * the result.
	 */
	if ((t = realpath(".", dn)) != NULL) {
/*@-globs -mods@*/	/* XXX no rpmGlobalMacroContext mods please. */
	    rpath = (char *) rpmGetPath(t, "/", lpath, NULL);
	    /* XXX preserve the pesky trailing '/' */
	    if (lpath[strlen(lpath)-1] == '/') {
		char * s = rpath;
		rpath = rpmExpand(s, "/", NULL);
		s = _free(s);
	    }
/*@=globs =mods@*/
	} else
	    rpath = NULL;
#if defined(__GLIBC__)
	t = _free(t);
#endif
    }
#endif	/* !__LCLINT__ */

    return rpath;
}

off_t Lseek(int fdno, off_t offset, int whence)
{
if (_rpmio_debug)
fprintf(stderr, "*** Lseek(%d,0x%lx,%d)\n", fdno, (long)offset, whence);
    return lseek(fdno, offset, whence);
}
