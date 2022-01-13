/**
 * \file rpmdb/legacy.c
 */

#include "system.h"

#if defined(HAVE_GELF_H)
#if LIBELF_H_LFS_CONFLICT
/* some gelf.h/libelf.h implementations (Solaris) are
 * incompatible with the Large File API
 */
# undef _LARGEFILE64_SOURCE
# undef _LARGEFILE_SOURCE
# undef _FILE_OFFSET_BITS
# define _FILE_OFFSET_BITS 32
#endif
#if defined(__LCLINT__)
/*@-incondefs@*/
typedef long long loff_t;
/*@=incondefs@*/
#endif
#include <gelf.h>

#if !defined(DT_GNU_PRELINKED)
#define	DT_GNU_PRELINKED	0x6ffffdf5
#endif
#if !defined(DT_GNU_LIBLIST)
#define	DT_GNU_LIBLIST		0x6ffffef9
#endif

#endif

#include "rpmio_internal.h"
#include <rpmmacro.h>
#include "misc.h"
#include "legacy.h"
#include "debug.h"

#define alloca_strdup(_s)	strcpy(alloca(strlen(_s)+1), (_s))

/**
 * Open a file descriptor to verify file MD5 and size.
 * @param path		file path
 * @retval pidp		prelink helper pid or 0
 * @retval fsizep	file size
 * @return		-1 on error, otherwise, an open file descriptor
 */ 
/*@-compdef -moduncon -noeffectuncon @*/
static int open_dso(const char * path, /*@null@*/ pid_t * pidp, /*@null@*/ size_t *fsizep)
	/*@globals rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
	/*@modifies *pidp, *fsizep, rpmGlobalMacroContext,
		fileSystem, internalState @*/
{
    int fdno;

    if (pidp) *pidp = 0;

    if (fsizep) {
	struct stat sb, * st = &sb;
	if (stat(path, st) < 0)
	    return -1;
	*fsizep = (size_t)st->st_size;
    }

    fdno = open(path, O_RDONLY);
    if (fdno < 0)
	goto exit;

#if defined(HAVE_GELF_H) && defined(HAVE_LIBELF)
 {  Elf *elf = NULL;
    Elf_Scn *scn = NULL;
    Elf_Data *data = NULL;
    GElf_Ehdr ehdr;
    GElf_Shdr shdr;
    GElf_Dyn dyn;
    int bingo;
    static pthread_mutex_t _mutex = PTHREAD_MUTEX_INITIALIZER;
    static yarnLock oneshot = NULL;	/* XXX memleak */
    static const char ** cmd_av = NULL;	/* XXX memleak */
    static int cmd_ac = 0;
    int xx;

    xx = pthread_mutex_lock(&_mutex);
    if (oneshot == NULL)
	oneshot = yarnNewLock(0);
    if (cmd_av == NULL) {
	const char * cmd = rpmExpand("%{?__prelink_undo_cmd}", NULL);
	(void) poptParseArgvString(cmd, &cmd_ac, &cmd_av);
	cmd = _free(cmd);
    }
    xx = pthread_mutex_unlock(&_mutex);

    if (cmd_ac == 0)
	goto exit;

    yarnPossess(oneshot);	/* XXX thread-safe iff compiled w USE_LOCKS */
ANNOTATE_IGNORE_READS_AND_WRITES_BEGIN();
    (void) elf_version(EV_CURRENT);

/*@-evalorder@*/
    if ((elf = elf_begin (fdno, ELF_C_READ, NULL)) == NULL
     || elf_kind(elf) != ELF_K_ELF
     || gelf_getehdr(elf, &ehdr) == NULL
     || !(ehdr.e_type == ET_DYN || ehdr.e_type == ET_EXEC))
    {
	if (elf) (void) elf_end(elf);
ANNOTATE_IGNORE_READS_AND_WRITES_END();
	yarnRelease(oneshot);
	goto exit;
    }
/*@=evalorder@*/

    bingo = 0;
    while (!bingo && (scn = elf_nextscn(elf, scn)) != NULL) {
	(void) gelf_getshdr(scn, &shdr);
	if (shdr.sh_type != SHT_DYNAMIC)
	    continue;
	while (!bingo && (data = elf_getdata (scn, data)) != NULL) {
	    unsigned maxndx = (unsigned) (data->d_size / shdr.sh_entsize);
	    unsigned ndx;

            for (ndx = 0; ndx < maxndx; ++ndx) {
/*@-uniondef@*/
		(void) gelf_getdyn (data, ndx, &dyn);
/*@=uniondef@*/
		if (!(dyn.d_tag == DT_GNU_PRELINKED || dyn.d_tag == DT_GNU_LIBLIST))
		    /*@innercontinue@*/ continue;
		bingo = 1;
		/*@innerbreak@*/ break;
	    }
	}
    }

    (void) elf_end(elf);
ANNOTATE_IGNORE_READS_AND_WRITES_END();
    yarnRelease(oneshot);

    if (pidp != NULL && bingo) {
	int pipes[2] = { -1, -1 };
	pid_t pid;

	xx = close(fdno);
	xx = pipe(pipes);
	pid = fork();
	switch (pid) {
	case 0:
	  { const char ** av = NULL;
	    int ac = 0;
	    xx = close(pipes[0]);
	    xx = dup2(pipes[1], STDOUT_FILENO);
	    xx = close(pipes[1]);
	    if (!poptDupArgv(cmd_ac, cmd_av, &ac, &av)) {
		av[ac-1] = path;
		av[ac] = NULL;
		unsetenv("MALLOC_CHECK_");
		xx = execve(av[0], (char *const *)av+1, environ);
	    }
	    _exit(127);
	    /*@notreached@*/
	  } break;
	default:
	    fdno = pipes[0];
	    xx = close(pipes[1]);
	    *pidp = pid;
	    break;
	}
    }
 }
#endif

exit:
    return fdno;
}
/*@=compdef =moduncon =noeffectuncon @*/

static const char hmackey[] = "orboDeJITITejsirpADONivirpUkvarP";

int dodigest(int dalgo, const char * fn, unsigned char * digest,
		unsigned dflags, size_t *fsizep)
{
    int asAscii = dflags & 0x01;
    int doHmac = dflags & 0x02;
    const char * path;
    urltype ut = urlPath(fn, &path);
    unsigned char * dsum = NULL;
    size_t dlen;
    unsigned char buf[32*BUFSIZ];
    FD_t fd;
    size_t fsize = 0;
    pid_t pid = 0;
    int use_mmap;
    int rc = 0;
    int fdno;
#if defined(HAVE_MMAP)
    int xx;
#endif

/*@-globs -internalglobs -mods @*/
    fdno = open_dso(path, &pid, &fsize);
/*@=globs =internalglobs =mods @*/
    if (fdno < 0) {
	rc = 1;
	goto exit;
    }

    /* XXX 128 Mb resource cap for top(1) scrutiny, MADV_SEQUENTIAL better. */
    use_mmap = (pid == 0 && fsize <= 0x07ffffff);

    switch(ut) {
    case URL_IS_PATH:
    case URL_IS_UNKNOWN:
#if defined(HAVE_MMAP)
      if (use_mmap) {
	DIGEST_CTX ctx;
	void * mapped = NULL;

	if (fsize) {
	    mapped = mmap(NULL, fsize, PROT_READ, MAP_SHARED, fdno, 0);
	    if (mapped == (void *)-1) {
		xx = close(fdno);
		rc = 1;
		break;
	    }

#if defined(HAVE_MADVISE) && defined(MADV_SEQUENTIAL)
	    xx = madvise(mapped, fsize, MADV_SEQUENTIAL);
#endif
	}

	ctx = rpmDigestInit((pgpHashAlgo)dalgo, RPMDIGEST_NONE);
	if (doHmac)
	    xx = rpmHmacInit(ctx, hmackey, 0);
	if (fsize)
	    xx = rpmDigestUpdate(ctx, mapped, fsize);
	xx = rpmDigestFinal(ctx, &dsum, &dlen, asAscii);
	if (fsize)
	    xx = munmap(mapped, fsize);
	xx = close(fdno);
	break;
      }	/*@fallthrough@*/
#endif
    case URL_IS_HTTPS:
    case URL_IS_HTTP:
    case URL_IS_FTP:
    case URL_IS_HKP:
    case URL_IS_DASH:
    default:
	/* Either use the pipe to prelink -y or open the URL. */
	fd = (pid != 0) ? fdDup(fdno) : Fopen(fn, "r.fdio");
	(void) close(fdno);
	if (fd == NULL || Ferror(fd)) {
	    rc = 1;
	    if (fd != NULL)
		(void) Fclose(fd);
	    break;
	}
	
	fdInitDigest(fd, (pgpHashAlgo)dalgo, 0);
	if (doHmac)
	    fdInitHmac(fd, hmackey, 0);
	fsize = 0;
	while ((rc = (int) Fread(buf, sizeof(buf[0]), sizeof(buf), fd)) > 0)
	    fsize += rc;
	fdFiniDigest(fd, (pgpHashAlgo)dalgo, &dsum, &dlen, asAscii);
	if (Ferror(fd))
	    rc = 1;

	(void) Fclose(fd);
	break;
    }

    /* Reap the prelink -y helper. */
    if (pid) {
	int status;
/*@+longunsignedintegral@*/
	(void) waitpid(pid, &status, 0);
/*@=longunsignedintegral@*/
	if (!WIFEXITED(status) || WEXITSTATUS(status))
	    rc = 1;
    }

exit:
    if (fsizep)
	*fsizep = fsize;
    if (!rc)
	memcpy(digest, dsum, dlen);
    dsum = _free(dsum);

    return rc;
}
