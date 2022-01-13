/*
 * Prints out following information in same format as %dump for builder:
 * $ rpmbuild --nodigest --nosignature --nobuild -bp --define 'prep %{echo:dummy: PACKAGE_NAME %{name} }%dump' qemu.spec 2>&1 | awk '$2 ~ /^SOURCEURL/ {print} $2 ~ /^PATCHURL/  {print} $2 ~ /^nosource/ {print} $2 ~ /^PACKAGE_/ {print}'
 * dummy: PACKAGE_NAME qemu ========================
 * -2: PACKAGE_RELEASE    60@2.6.16.59_2
 * -1: PACKAGE_VERSION    1.3.0pre11
 * -3: PATCHURL0  qemu-nostatic.patch
 * -3: PATCHURL1  qemu-cc.patch
 * -3: PATCHURL11 qemu-0.7.2-gcc4-opts.patch
 * -3: PATCHURL13 qemu-dosguest.patch
 * -3: PATCHURL3  qemu-dot.patch
 * -3: PATCHURL4  qemu-gcc4_x86.patch
 * -3: PATCHURL5  qemu-gcc4_ppc.patch
 * -3: PATCHURL6  qemu-nosdlgui.patch
 * -3: PATCHURL8  qemu-kde_virtual_workspaces_hack.patch
 * -3: PATCHURL9  qemu-0.8.0-gcc4-hacks.patch
 * -3: SOURCEURL0 http://fabrice.bellard.free.fr/qemu/qemu-0.9.0.tar.gz
 * -3: SOURCEURL1 http://fabrice.bellard.free.fr/qemu/kqemu-1.3.0pre11.tar.gz
 *
 *  $ rpm-specdump qemu.spec
 *  h PACKAGE_NAME qemu
 *  h PACKAGE_VERSION 0.9.0
 *  h PACKAGE_RELEASE 60k
 *  s PATCHURL13 qemu-dosguest.patch
 *  s PATCHURL11 qemu-0.7.2-gcc4-opts.patch
 *  s PATCHURL9 qemu-0.8.0-gcc4-hacks.patch
 *  s PATCHURL8 qemu-kde_virtual_workspaces_hack.patch
 *  s PATCHURL6 qemu-nosdlgui.patch
 *  s PATCHURL5 qemu-gcc4_ppc.patch
 *  s PATCHURL4 qemu-gcc4_x86.patch
 *  s PATCHURL3 qemu-dot.patch
 *  s PATCHURL1 qemu-cc.patch
 *  s PATCHURL0 qemu-nostatic.patch
 *  s SOURCEURL1 http://fabrice.bellard.free.fr/qemu/kqemu-1.3.0pre11.tar.gz
 *  s SOURCEURL0 http://fabrice.bellard.free.fr/qemu/qemu-0.9.0.tar.gz
 *
 * And with NoSource: 1, NoSource: 2
 *
 *  $ rpmbuild --nodigest --nosignature --nobuild -bp --define 'prep %{echo:dummy: PACKAGE_NAME %{name} }%dump' ZendDebugger.spec 2>&1 | awk '$2 ~ /^SOURCEURL/ {print} $2 ~ /^PATCHURL/  {print} $2 ~ /^nosource/ {print} $2 ~ /^PACKAGE_/ {print}'
 *  dummy: PACKAGE_NAME ZendDebugger ========================
 *   -2: PACKAGE_RELEASE    0.4
 *   -1: PACKAGE_VERSION    5.2.10
 *   -3: SOURCEURL0 http://downloads.zend.com/pdt/server-debugger/ZendDebugger-5.2.10-linux-glibc21-i386.tar.gz
 *   -3: SOURCEURL1 http://downloads.zend.com/pdt/server-debugger/ZendDebugger-5.2.10-linux-glibc23-x86_64.tar.gz
 *   -3: nosource   1
 *
 *  $ rpm-specdump ZendDebugger.spec
 *  h PACKAGE_NAME ZendDebugger
 *  h PACKAGE_VERSION 5.2.10
 *  h PACKAGE_RELEASE 0.4
 *  s SOURCEURL1 http://downloads.zend.com/pdt/server-debugger/ZendDebugger-5.2.10-linux-glibc23-x86_64.tar.gz
 *  s nosource 1
 *  s SOURCEURL0 http://downloads.zend.com/pdt/server-debugger/ZendDebugger-5.2.10-linux-glibc21-i386.tar.gz
 *  s nosource 0
 */

#include "system.h"

#include <getopt.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <grp.h>
#include <sys/time.h>
#include <sys/types.h>

#include <rpmio.h>
#include <rpmiotypes.h>
#include <rpmbuild.h>
#include <rpmts.h>

#define ARG_WITH	1024
#define ARG_WITHOUT	1025
#define ARG_DEFINE	1026
#define ARG_TARGET	1027
#define ARG_RCFILE	1028
#define ARG_CHROOT	1029
#define ARG_UID		1030
#define ARG_GID		1031

static struct option const
CMDLINE_OPTIONS[] = {
  { "help",     no_argument,  0, 'h' },
  { "version",  no_argument,  0, 'v' },
  { "with",     required_argument, 0, ARG_WITH    },
  { "without",  required_argument, 0, ARG_WITHOUT },
  { "define",   required_argument, 0, ARG_DEFINE  },
  { "target",   required_argument, 0, ARG_TARGET  },
  { "rcfile",   required_argument, 0, ARG_RCFILE  },
  { "chroot",   required_argument, 0, ARG_CHROOT  },
  { "uid",	required_argument, 0, ARG_UID },
  { "gid",	required_argument, 0, ARG_GID },
  { 0,0,0,0 }
};

struct Arguments
{
    char const *	target;
    char const *	rcfile;
    char const *	chroot;
    uid_t		uid;
    gid_t		gid;
    
    struct {
	char const **	values;
	size_t		cnt;
	size_t		reserved;
    }			macros;

    char const *	specfile;
};

struct DepSet {
    int32_t const *	flags;
    char const **	name;
    char const **	version;
    ssize_t		cnt;
};

static inline void safe_write(int fd, const char *buf, size_t count)
{
  ssize_t written;

  do {
    written = write(fd, buf, count);
    if ((size_t)written == count)
      break;
    if (written > 0) {
      buf += written;
      count -= written;
    }
  } while (written >= 0 || errno == EINTR);
  if (written < 0) {
	  perror("write");
  }
}

inline static void 
writeStr(int fd, char const *cmd)
{
  safe_write(fd, cmd, strlen(cmd));
}

#define WRITE_MSG(FD,X)		safe_write(FD,X,sizeof(X)-1)
#define WRITE_STR(FD,X)		writeStr(FD,X)

static void
showHelp(int fd, char const *cmd, int res)
{
  char		tmp[strlen(cmd)+1];
  strcpy(tmp, cmd);
  
  WRITE_MSG(fd, "Usage:  ");
  WRITE_STR(fd, basename(tmp));
  WRITE_MSG(fd,
	    " [--define '<macro> <value>']* [--with[out] <key>]* [--chroot <dir>]\n"
	    "                [--target <target>] [--rcfile <rcfile>] [--] <specfile>\n");
  exit(res);
}

static void
addDefine(struct Arguments *args, char const *val)
{
  register size_t	c = args->macros.cnt;
  if (args->macros.reserved <= c) {
    args->macros.reserved *= 2;
    args->macros.reserved += 1;
  
    args->macros.values = realloc(args->macros.values,
				  args->macros.reserved * sizeof(char const *));
    if (args->macros.values==0) {
      perror("realloc()");
      exit(1);
    }
  }

  args->macros.values[c] = strdup(val);
  ++args->macros.cnt;
}

static void
setWithMacro(struct Arguments *args,
	     char const *name, char const *prefix, size_t prefix_len)
{
  size_t	len = strlen(name);
  char		tmp[2*len + 2*prefix_len + sizeof("__ ---")];
  char *	ptr = tmp;

  // set '_<prefix>_<name>'
  *ptr++ = '_';
  memcpy(ptr, prefix, prefix_len); ptr += prefix_len;
  *ptr++ = '_';
  memcpy(ptr, name,   len);        ptr += len;
  *ptr++ = ' ';

  // append ' --<prefix>-<name>'
  *ptr++ = '-';
  *ptr++ = '-';
  memcpy(ptr, prefix, prefix_len); ptr += prefix_len;
  *ptr++ = '-';
  memcpy(ptr, name,   len);        ptr += len;
  *ptr   = '\0';

  addDefine(args, tmp);
}


static void
parseArgs(struct Arguments *args, int argc, char *argv[])
{
  while (1) {
    int		c = getopt_long(argc, argv, "", CMDLINE_OPTIONS, 0);
    if (c==-1) break;
    switch (c) {
      case 'h'		:  showHelp(1, argv[0], 0);
      case ARG_TARGET	:  args->target = optarg; break;
      case ARG_RCFILE	:  args->rcfile = optarg; break;
      case ARG_CHROOT	:  args->chroot = optarg; break;
      case ARG_UID	:  args->uid    = atoi(optarg); break;
      case ARG_GID	:  args->gid    = atoi(optarg); break;
      case ARG_DEFINE	:  addDefine(args, optarg); break;
      case ARG_WITH	:  setWithMacro(args, optarg, "with",    4); break;
      case ARG_WITHOUT	:  setWithMacro(args, optarg, "without", 7); break;
      default:
	WRITE_MSG(2, "Try '");
	WRITE_STR(2, argv[0]);
	WRITE_MSG(2, " --help\" for more information.\n");
	exit(1);
    }
  }

  if (optind+1!=argc) {
    safe_write(2, "No/too much specfile(s) given; aborting\n", 40);
    exit(1);
  }

  if (args->gid==(gid_t)(-1))
    args->gid = args->uid;

  args->specfile = argv[optind];
}

static void
setMacros(char const * const *macros, size_t cnt)
{
  size_t	i;
  for (i=0; i<cnt; ++i)
    rpmDefineMacro(rpmGlobalMacroContext, macros[i], 0);
}

int main(int argc, char *argv[])
{
struct Arguments args = { 0,0,0,-1,-1, {0,0,0}, 0 };
Spec s;

	parseArgs(&args, argc, argv);

	if ((args.chroot && chroot(args.chroot)==-1) ||
		(args.uid!=(uid_t)(-1) && (setgroups(0,0)  ==-1 || getgroups(0,0)!=0))  ||
		(args.gid!=(gid_t)(-1) && (setgid(args.gid)==-1 || getgid()!=args.gid)) ||
		(args.uid!=(uid_t)(-1) && (setuid(args.uid)==-1 || getuid()!=args.uid))) {

		perror("chroot/setuid/setgid()");
		return EXIT_FAILURE;
	}
  
	rpmReadConfigFiles(args.rcfile, args.target);
	setMacros(args.macros.values, args.macros.cnt);

	rpmts ts = rpmtsCreate();
	if (parseSpec(ts, args.specfile, NULL, 0, NULL, NULL, 1, 1, 0) != 0) {
		return EXIT_FAILURE;
	}
  
	s = rpmtsSpec(ts);

	// here starts the code for builder
	const char *name = NULL, *version = NULL, *release = NULL;

	initSourceHeader(s, NULL);
	Header h = s->sourceHeader;

	HE_t he;
	int rc;

	he = (HE_s*)memset(alloca(sizeof(*he)), 0, sizeof(*he));
	he->tag = (rpmTag) RPMTAG_NAME;
	rc = headerGet(h, he, 0);
	if (!rc) {
		fprintf(stderr, "Name (NVR) query failed\n");
		return EXIT_FAILURE;
	}
	name = (char *)he->p.ptr;

	he = (HE_s*)memset(alloca(sizeof(*he)), 0, sizeof(*he));
	he->tag = (rpmTag) RPMTAG_VERSION;
	rc = headerGet(h, he, 0);
	if (!rc) {
		fprintf(stderr, "Version (NVR) query failed\n");
		return EXIT_FAILURE;
	}
	version = (char *)he->p.ptr;

	he = (HE_s*)memset(alloca(sizeof(*he)), 0, sizeof(*he));
	he->tag = (rpmTag) RPMTAG_RELEASE;
	rc = headerGet(h, he, 0);
	if (!rc) {
		fprintf(stderr, "Release (NVR) query failed\n");
		return EXIT_FAILURE;
	}
	release = (char *)he->p.ptr;

	printf("h PACKAGE_NAME %s\n", name);
	printf("h PACKAGE_VERSION %s\n", version);
	printf("h PACKAGE_RELEASE %s\n", release);

	struct Source *ps = s->sources;
	while (ps) {
		const char *type = (ps->flags & RPMFILE_SOURCE) ? "SOURCE" : "PATCH";
		printf("s %sURL%d %s\n", type, ps->num, ps->fullSource);
		if (ps->flags & RPMFILE_GHOST) {
			printf("s nosource %d\n", ps->num);
		}
		ps = ps->next;
	}

	const char *arch = rpmExpand("%{_target_cpu}", NULL);
	printf("m _target_cpu %s\n", arch);

	return(0);
}
