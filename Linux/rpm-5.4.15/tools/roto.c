
#include "system.h"
#include <pwd.h>
#include <grp.h>
#include <sys/sysmacros.h>
#include <sys/personality.h>

#include <rpmiotypes.h>
#include <rpmio.h>
#include <rpmdir.h>
#include <rpmlog.h>
#include <fts.h>
#include <argv.h>
#include <poptIO.h>

#include <rpmevr.h>

#include "debug.h"

const char *__progname;

#define	_UIDNONE	((uid_t)-1)
#define	_GIDNONE	((gid_t)-1)

#define	ROTO_VERSION	"1.1.12+roto"
#define	SYSCONFDIR	"/etc"
/* XXX lib64? */
#define	PYTHONDIR	"/usr/lib/python2.6/site-packages"
#define	PKGPYTHONDIR	PYTHONDIR "/mock"
#define	MOCKCONFDIR	SYSCONFDIR "/mock"

typedef /*@abstract@*/ /*@refcounted@*/ struct ROTO_s * ROTO_t;

#define F_ISSET(_roto, _FLAG) ((_roto)->flags & (ROTO_FLAGS_##_FLAG))
#define ROTODBG(_l) if (_roto_debug) fprintf _l

enum rotoFlags_e {
    ROTO_FLAGS_NONE		= 0,
	/* 0 unused */
    ROTO_FLAGS_INIT		= (1 <<  1),	/*    --init */
    ROTO_FLAGS_CLEAN		= (1 <<  2),	/*    --clean */
    ROTO_FLAGS_SCRUB		= (1 <<  3),	/*    --scrub */
    ROTO_FLAGS_REBUILD		= (1 <<  4),	/*    --rebuild */
    ROTO_FLAGS_BUILDSRPM	= (1 <<  5),	/*    --buildsrpm */
    ROTO_FLAGS_SHELL		= (1 <<  6),	/*    --shell */
    ROTO_FLAGS_CHROOT		= (1 <<  7),	/*    --chroot */
    ROTO_FLAGS_INSTALLDEPS	= (1 <<  8),	/*    --installdeps */
    ROTO_FLAGS_INSTALL		= (1 <<  9),	/*    --install */
    ROTO_FLAGS_UPDATE		= (1 << 10),	/*    --update */
    ROTO_FLAGS_ORPHANSKILL	= (1 << 11),	/*    --orphanskill */
    ROTO_FLAGS_COPYIN		= (1 << 12),	/*    --copyin */
    ROTO_FLAGS_COPYOUT		= (1 << 13),	/*    --copyout */
    ROTO_FLAGS_SCM_ENABLE	= (1 << 14),	/*    --scm-enable */
	/* 15 unused */
    ROTO_FLAGS_OFFLINE		= (1 << 16),	/*    --offline */
    ROTO_FLAGS_NO_CLEAN		= (1 << 17),	/*    --no-clean */
    ROTO_FLAGS_CLEANUP_AFTER	= (1 << 18),	/*    --cleanup-after */
    ROTO_FLAGS_UNPRIV		= (1 << 19),	/*    --unpriv */
    ROTO_FLAGS_TRACE		= (1 << 20),	/*    --trace */
    ROTO_FLAGS_PRINT_ROOT_PATH	= (1 << 21),	/*    --print-root-path */
    ROTO_FLAGS_DIRSONLY		= (1 << 22),	/* XXX remove */
    ROTO_FLAGS_NOCOMMENT	= (1 << 23),	/* XXX remove */
    ROTO_FLAGS_NOUNSHARE	= (1 << 24),	/* XXX remove */
	/* 25-31 unused */
};

struct ROTO_s {
    struct rpmioItem_s _item;	/*!< usage mutex and pool identifier. */

    int ftsoptions;		/*!< global Fts(3) traversal options. */
    FTS * t;			/*!< global Fts(3) traversal data. */
    FTSENT * p;			/*!< current node Fts(3) traversal data. */
    struct stat sb;		/*!< current node stat(2) data. */
    ARGV_t paths;		/*!< array of file paths. */
    const char * fn;	/* XXX remove? */
    int (*visitD) (void *);
    int (*visitF) (void *);
    int (*visitDP) (void *);
    mode_t mask;
    const char * opath;		/* source path */
    size_t opathlen;
    const char * npath;		/* target path */
    size_t npathlen;

    enum rotoFlags_e flags;
    const char * version;
    const char * basedir;	/* XXX /var/lib/mock + "/root" */
    const char * resultdir;		/*    --resultdir */
    const char * cache_topdir;
    const char * chroothome;
    const char * log_config_file;
    int rpmbuild_timeout;		/*    --rpmbuild_timeout */
    uid_t chrootuid;
    const char * chrootuser;
    gid_t chrootgid;
    const char * chrootgroup;
    uid_t unprivUid;
    gid_t unprivGid;
    const char * build_log_fmt_name;
    const char * root_log_fmt_name;
    const char * state_log_fmt_name;

    int online;
    int internal_dev_setup;
    int internal_setarch;
    int cleanup_on_success;
    int cleanup_on_failure;

    int createrepo_on_rpms;
    const char * createrepo_command;

    const char ** plugins;
    const char * plugin_dir;
    const char ** plugin_conf;

    /* SCM options */
    const char ** scm_opts;		/*    --scm-option */

    /* dependent on guest OS */
    const char * useradd;
    int use_host_resolv;
    const char ** chroot_setup_cmd;
    const char * target_arch;	/* XXX same as arch? */
    const char * arch;			/*    --arch */
    const char * rpmbuild_arch;		/*    --target */

    const char * yum_path;
    const char * yum_builddep_path;
    const char * yum_conf_content;
    const char ** more_buildreqs;
    const char ** files;
    const char ** macros;	/* XXX same as rpmmacros? */
    const char ** rpmmacros;		/* -D,--define */

    const char ** rpmwith;			/*    --with */
    const char ** rpmwithout;		/*    --without */
    const char * uniqueext;		/*    --uniqueext */
    const char * configdir;		/*    --configdir */
    const char * cwd;			/*    --cwd */
    const char * spec;			/*    --spec */
    const char * sources;		/*    --sources */

    const char ** enabled_plugins;	/*    --enable-plugin */
    const char ** disabled_plugins;	/*    --disable-plugin */

/* --- internal */
    const char * _state;
    int selinux;
    const char * chroot_name;	/* XXX same as options.chroot? */
    const char * sharedRootName;	/* "OS-A" */
    const char * homedir;	/* roto->chroothome */
    int chrootWasCached;
    int chrootWasCleaned;
    int logging_initialized;
    FILE * buildrootLock;

/* --- hacks */
    const char *_root;		/* "OS-A" */
    const char *_rootdir;	/* roto->basedir + "/root" */
    const char * builddir;	/* roto->homedir + "/build" */
    const char * cachedir;	/* roto->cache_topdir + roto->sharedRootName */
    const char ** configs;	/* *.cfg INI files */
    const char ** umountCmds;
    const char ** mountCmds;

    const char ** _hooks;

/* --- plugins */
    int ccache_enable;
    const char * ccache_opts;
    const char * ccache_max_cache_size;
    const char * ccache_dir;
    const char * ccachePath;

    int yum_cache_enable;
    const char * yum_cache_opts;
    int yum_cache_max_age_days;
    int yum_cache_max_metadata_age_days;
    const char * yum_cache_dir;
    int yum_cache_online;
    const char * yumSharedCachePath;
    const char * yumCacheLockPath;
    FILE * yumCacheLock;

    int root_cache_enable;
    const char * root_cache_opts;
    int root_cache_max_age_days;
    const char * root_cache_dir;
    const char * root_cache_compress_program;
    const char * root_cache_extension;
    const char * rootSharedCachePath;
    const char * rootCacheLockPath;
    FILE * rootCacheLock;

    int bind_mount_enable;
    const char * bind_mount_opts;
    const char ** bind_mount_dirs;

    int tmpfs_enable;
    const char * tmpfs_opts;
    int tmpfs_required_ram_mb;
    int tmpfs_max_fs_size;

    int selinux_enable;
    const char * selinux_opts;

    rpmiob iob;			/*!< output collector */
#if defined(__LCLINT__)
/*@refs@*/
    int nrefs;			/*!< (unused) keep splint happy */
#endif
};

#define	Pd(_f)	fprintf(fp, "  %s:\t%d\n", #_f, roto->_f)
#define	Px(_f)	fprintf(fp, "  %s:\t%x\n", #_f, roto->_f)
#define	Ps(_f)	fprintf(fp, "  %s:\t%s\n", #_f, roto->_f)
#define	Pp(_f)	fprintf(fp, "  %s:\t%p\n", #_f, roto->_f)
#define	Pav(_f)	argvPrint(#_f, (ARGV_t)roto->_f, fp)

static void rotoPrint(ROTO_t roto, FILE * fp)
{
    if (fp == NULL)
	fp = stderr;

    Px(flags);
    Ps(version);
    Ps(basedir);
    Ps(resultdir);
    Ps(cache_topdir);
    Ps(chroothome);
    Ps(log_config_file);
    Pd(rpmbuild_timeout);
    Pd(chrootuid);
    Ps(chrootuser);
    Pd(chrootgid);
    Ps(chrootgroup);
    Pd(unprivUid);
    Pd(unprivGid);
    Ps(build_log_fmt_name);
    Ps(root_log_fmt_name);
    Ps(state_log_fmt_name);

    Pd(online);
    Pd(internal_dev_setup);
    Pd(internal_setarch);
    Pd(cleanup_on_success);
    Pd(cleanup_on_failure);

    Pd(createrepo_on_rpms);
    Ps(createrepo_command);

    Pav(plugins);
    Ps(plugin_dir);
    Pav(plugin_conf);

    /* SCM options */
    Pav(scm_opts);

    /* dependent on guest OS */
    Ps(useradd);
    Pd(use_host_resolv);
    Pav(chroot_setup_cmd);
    Ps(target_arch);
    Ps(arch);
    Ps(rpmbuild_arch);

    /* dependent on depsolver */
    Ps(yum_path);
    Ps(yum_builddep_path);
    Ps(yum_conf_content);

    Pav(more_buildreqs);
    Pav(files);
    Pav(macros);
    Pav(rpmmacros);

    Pav(rpmwith);
    Pav(rpmwithout);
    Ps(uniqueext);
    Ps(configdir);
    Ps(cwd);
    Ps(spec);
    Ps(sources);
    Pav(enabled_plugins);
    Pav(disabled_plugins);

/* --- internal */
    Ps(_state);
    Pd(selinux);
    Ps(chroot_name);
    Ps(sharedRootName);
    Ps(homedir);
    Pd(chrootWasCached);
    Pd(chrootWasCleaned);
    Pd(logging_initialized);
    Pp(buildrootLock);

/* --- hacks */
    Ps(_root);
    Ps(_rootdir);
    Ps(builddir);
    Ps(cachedir);
    Pav(configs);
    Pav(umountCmds);
    Pav(mountCmds);

    Pav(_hooks);

/* --- plugins */
    Pd(ccache_enable);
    Ps(ccache_opts);
    Ps(ccache_max_cache_size);
    Ps(ccache_dir);
    Ps(ccachePath);

    Pd(yum_cache_enable);
    Ps(yum_cache_opts);
    Pd(yum_cache_max_age_days);
    Pd(yum_cache_max_metadata_age_days);
    Ps(yum_cache_dir);
    Pd(yum_cache_online);
    Ps(yumSharedCachePath);
    Ps(yumCacheLockPath);

    Pd(root_cache_enable);
    Ps(root_cache_opts);
    Pd(root_cache_max_age_days);
    Ps(root_cache_dir);
    Ps(root_cache_compress_program);
    Ps(root_cache_extension);
    Ps(rootSharedCachePath);
    Ps(rootCacheLockPath);

    Pd(bind_mount_enable);
    Ps(bind_mount_opts);
    Pav(bind_mount_dirs);

    Pd(tmpfs_enable);
    Ps(tmpfs_opts);
    Pd(tmpfs_required_ram_mb);
    Pd(tmpfs_max_fs_size);

    Pd(selinux_enable);
    Ps(selinux_opts);

/* --- macros */
    rpmDumpMacroTable(NULL, fp);

}
#undef	Pav
#undef	Pp
#undef	Ps
#undef	Px
#undef	Pd


/*@unchecked@*/
int _roto_debug = -1;

static const char * _roto_plugins[] = {
    "tmpfs", "root_cache", "yum_cache", "bind_mount", "ccache", "selinux",
    NULL,
};
static const char * _roto_plugin_conf[] = {
    "'ccache_enable': True",
    "'ccache_opts': {\n\
 	'max_cache_size': '4G',\n\
	'dir': '%(cache_topdir)s/%(root)s/ccache/'\n\
    }",
    "'yum_cache_enable': True",
    "'yum_cache_opts': {\n\
	'max_age_days': 30,\n\
	'max_metadata_age_days': 30,\n\
	'dir': '%(cache_topdir)s/%(root)s/yum_cache/',\n\
	'online': True,\n\
    }",
    "'root_cache_enable': True",
    "'root_cache_opts': {\n\
	'max_age_days': 15,\n\
	'dir': '%(cache_topdir)s/%(root)s/root_cache/',\n\
	'compress_program': 'pigz',\n\
	'extension': '.gz'\n\
    }",
    "'bind_mount_enable': True",
    "'bind_mount_opts': {\n\
	'dirs': [\n\
	# specify like this:\n\
	# ('/host/path', '/bind/mount/path/in/chroot/' ),\n\
	# ('/another/host/path', '/another/bind/mount/path/in/chroot/'),\n\
	]\n\
    }",
    "'tmpfs_enable': False",
    "'tmpfs_opts': {\n\
	'required_ram_mb': 900,\n\
	'max_fs_size': None\n\
    }",
    "'selinux_enable': True",
    "'selinux_opts': {}",
    NULL,
};
static const char * __roto_scm_opts[] = {
    "'method': 'git'",
    "'cvs_get': 'cvs -d /srv/cvs co SCM_BRN SCM_PKG'",
    "'git_get': 'git clone SCM_BRN git://localhost/SCM_PKG.git SCM_PKG'",
    "'svn_get': 'svn co file:///srv/svn/SCM_PKG/SCM_BRN SCM_PKG'",
    "'spec': 'SCM_PKG.spec'",
    "'ext_src_dir': '/dev/null'",
    "'write_tar': False",
    NULL,
};
static const char * _roto_chroot_setup_cmd[] = {
    "groupinstall", "buildsys-build",
    NULL,
};
static const char * __roto_more_buildreqs[] = {
    NULL,
};
static const char * __roto_files[] = {
    NULL,
};
static const char * __roto_macros[] = {
    "%_topdir	%{__roto_chroothome}/build",
    "%_rpmfilename	%%{NAME}-%%{VERSION}-%%{RELEASE}.%%{ARCH}.rpm",
    NULL,
};

static const char _roto_yum_conf_content[] = "\
[main]\n\
cachedir=/var/cache/yum\n\
debuglevel=1\n\
reposdir=/dev/null\n\
logfile=/var/log/yum.log\n\
retries=20\n\
obsoletes=1\n\
gpgcheck=0\n\
assumeyes=1\n\
syslog_ident=mock\n\
syslog_device=\n\
\n\
# repos\n\
[base]\n\
name=BaseOS\n\
enabled=1\n\
#mirrorlist=http://mirrorlist.centos.org/?release=6&arch=x86_64&repo=os\n\
#baseurl=http://wp3.jbj.org/scientific/6.0/x86_64/os\n\
baseurl=http://www.gtlib.gatech.edu/pub/scientific/6.0/x86_64/os/\n\
failovermethod=priority\n\
\n\
[updates-fastbugs]\n\
name=updates-fastbugs\n\
enabled=1\n\
#mirrorlist=http://mirrorlist.centos.org/?release=6&arch=x86_64&repo=updates\n\
#baseurl=http://wp3.jbj.org/scientific/6.0/x86_64/updates/fastbugs\n\
baseurl=http://www.gtlib.gatech.edu/pub/scientific/6.0/x86_64/updates/fastbugs/\n\
failovermethod=priority\n\
\n\
[updates-security]\n\
name=updates-security\n\
enabled=1\n\
#mirrorlist=http://mirrorlist.centos.org/?release=6&arch=x86_64&repo=updates\n\
#baseurl=http://wp3.jbj.org/scientific/6.0/x86_64/updates/security\n\
baseurl=http://www.gtlib.gatech.edu/pub/scientific/6.0/x86_64/updates/security/\n\
failovermethod=priority\n\
\n\
#[epel]\n\
#name=epel\n\
#mirrorlist=http://mirrors.fedoraproject.org/mirrorlist?repo=epel-6&arch=x86_64\n\
#failovermethod=priority\n\
\n\
#[testing]\n\
#name=epel-testing\n\
#enabled=0\n\
#mirrorlist=http://mirrors.fedoraproject.org/mirrorlist?repo=testing-epel6&arch=x86_64\n\
#failovermethod=priority\n\
\n\
#[local]\n\
#name=local\n\
#baseurl=http://kojipkgs.fedoraproject.org/repos/dist-6E-epel-build/latest/x86_64/\n\
#cost=2000\n\
#enabled=0\n\
";

/*@unchecked@*/
static struct ROTO_s _roto = {
    .flags =	ROTO_FLAGS_NONE,
    .version =	ROTO_VERSION,
    .basedir = 	"/var/lib/%{__roto_progname}",	/* XXX FIXME: "/root" is appended */

    .resultdir = "%{__roto_basedir}/%{__roto_root}/result",
    .cache_topdir = "/var/cache/%{__roto_progname}",
    .chroothome = "/builddir",
    .log_config_file = "logging.ini",
    .rpmbuild_timeout = 0,
    .chrootuid = 0,	/* XXX unprivUid */
    .chrootuser = "mockbuild",
    .chrootgid = 0,	/* XXX grp.getgrnam("mock")[2] */
    .chrootgroup = "mockbuild",
    .build_log_fmt_name = "unadorned",
    .root_log_fmt_name = "detailed",
    .state_log_fmt_name = "state",
    .online = 1,	/* XXX bool True */

    .internal_dev_setup = 1, /* XXX bool True */
    .internal_setarch = 1, /* XXX bool True */

    .cleanup_on_success = 1, /* XXX bool True */
    .cleanup_on_failure = 1, /* XXX bool True */

    .createrepo_on_rpms = 0, /* XXX bool False */
    .createrepo_command = "/usr/bin/createrepo -d -q -x *.src.rpm",

    .plugins = _roto_plugins,
    .plugin_dir = PKGPYTHONDIR "/plugins",
    .plugin_conf = _roto_plugin_conf,

    /* SCM defaults */
    .scm_opts = __roto_scm_opts,

    /* dependent on guest OS */
    .useradd = "/usr/sbin/useradd -o -m -u %{__roto_chrootuid} -g %{__roto_chrootgid}  -d %{__roto_chroot_home} -N %{__roto_chrootuser}",
    .use_host_resolv = 1,	/* XXX bool True */
    .chroot_setup_cmd = _roto_chroot_setup_cmd,
    .target_arch = "i386",
    .rpmbuild_arch = NULL,
    .yum_path = "/usr/bin/yum",
    .yum_builddep_path = "/usr/bin/yum-builddep",
    .yum_conf_content = _roto_yum_conf_content,
    .more_buildreqs = __roto_more_buildreqs,
    .files = __roto_files,
    .macros = __roto_macros,

    /* --- internal hacks */
    .sharedRootName =	"OS-A",	/* XXX /etc/mock/OS-A.cfg config['root'] */
    .homedir =	"/builddir",

    ._root =	"OS-A",		/* XXX /etc/mock/OS-A.cfg config['root'] */
    ._rootdir = "/var/lib/%{__roto_progname}/%{__roto_root}/root",
    .builddir = "/builddir/build",
    .cachedir = "/var/cache/%{__roto_progname}/%{__roto_root}",	/* roto->cache_topdir + roto->sharedRootName */

};

/*==============================================================*/

static int shutilCopy(const char * s, const char * t)
{
    int rc = 0;

    rc = urlGetFile(s, t);

ROTODBG((stderr, "<-- %s(%s, %s) rc %d\n", __FUNCTION__, s, t, rc));
    return rc;
}

static int shutilCopy2(const char * s, const char * t)
{
    int rc = 0;

    /* XXX FIXME: shutil.copy2 copies metadata as well. */
    rc = urlGetFile(s, t);

ROTODBG((stderr, "<-- %s(%s, %s) rc %d\n", __FUNCTION__, s, t, rc));
    return rc;
}

/*==============================================================*/

static int
rotoCWalk(ROTO_t roto)
{
    char *const * paths = (char * const *) roto->paths;
    int ftsoptions = roto->ftsoptions;
    int rval = 0;
int xx;

    roto->t = Fts_open(paths, ftsoptions, NULL);
    if (roto->t == NULL) {
	fprintf(stderr, "Fts_open: %s", strerror(errno));
	return -1;
    }

    while ((roto->p = Fts_read(roto->t)) != NULL) {

	roto->fn = roto->p->fts_path;	/* XXX eliminate roto->fn */
	memcpy(&roto->sb, roto->p->fts_statp, sizeof(roto->sb));

	switch(roto->p->fts_info) {
	case FTS_D:
	    if (roto->visitD) xx = (*roto->visitD)(roto);
	    /*@switchbreak@*/ break;
	case FTS_DP:
	    if (roto->visitDP) xx = (*roto->visitDP)(roto);
	    /*@switchbreak@*/ break;
	case FTS_DNR:
	case FTS_ERR:
	case FTS_NS:
	    (void) fprintf(stderr, "%s: %s: %s\n", __progname,
			roto->p->fts_path, strerror(roto->p->fts_errno));
	    /*@switchbreak@*/ break;
	default:
	    if (roto->visitF) xx = (*roto->visitF)(roto);
	    /*@switchbreak@*/ break;
	}
    }
    xx = Fts_close(roto->t);
    roto->p = NULL;
    roto->t = NULL;
    return rval;
}

static int _cpTreeVisit(void * _roto)
{
    ROTO_t roto = (ROTO_t) _roto;
    int rc = 0;

    switch (roto->p->fts_statp->st_mode & S_IFMT) {
    case S_IFLNK:
	break;
    case S_IFDIR:
	break;
    case S_IFBLK:
    case S_IFCHR:
	break;
    case S_IFSOCK:
	break;
    case S_IFIFO:
	break;
    case S_IFREG:
	break;
    default:
	break;
    }

ROTODBG((stderr, "<-- %s(%s) rc %d\n", __FUNCTION__, roto->fn, rc));
    return rc;
}

static int _cpTreeVisitDP(void * _roto)
{
    ROTO_t roto = (ROTO_t) _roto;
    int rc = 0;

assert (roto->p->fts_info == FTS_DP);

ROTODBG((stderr, "<-- %s(%s) rc %d\n", __FUNCTION__, roto->fn, rc));
    return rc;
}

static int shutilCopyTree(ROTO_t roto, const char * s, const char * t)
{
    int rc = 0;
int xx;

    roto->opath = s;
    roto->opathlen = strlen(s);
    roto->npath = t;
    roto->npathlen = strlen(t);

    roto->paths = NULL;
    xx = argvAdd(&roto->paths, s);

    roto->ftsoptions = rpmioFtsOpts;
    if (!(roto->ftsoptions & (FTS_LOGICAL|FTS_PHYSICAL)))
	roto->ftsoptions |= FTS_PHYSICAL;
    roto->ftsoptions |= FTS_NOCHDIR;

    roto->visitD = _cpTreeVisit;
    roto->visitF = _cpTreeVisit;
    roto->visitDP = _cpTreeVisitDP;

    rc = rotoCWalk(roto);

    roto->visitD = NULL;
    roto->visitF = NULL;
    roto->visitDP = NULL;

    roto->paths = argvFree(roto->paths);

ROTODBG((stderr, "<-- %s(%s, %s) rc %d\n", __FUNCTION__, s, t, rc));
    return rc;
}

static int _rmTreeVisitDP(void * _roto)
{
    ROTO_t roto = (ROTO_t) _roto;
    int rc = 0;

assert (roto->p->fts_info == FTS_DP);
    rc = Rmdir(roto->fn);

ROTODBG((stderr, "<-- %s(%s) rc %d\n", __FUNCTION__, roto->fn, rc));
    return rc;
}

static int _rmTreeVisitF(void * _roto)
{
    ROTO_t roto = (ROTO_t) _roto;
    int rc = 0;

    rc = Unlink(roto->fn);

ROTODBG((stderr, "<-- %s(%s) rc %d\n", __FUNCTION__, roto->fn, rc));
    return rc;
}

static int shutilRmTree(ROTO_t roto, const char * s)
{
    int rc = 0;
int xx;

    roto->paths = NULL;
    xx = argvAdd(&roto->paths, s);

    roto->ftsoptions = rpmioFtsOpts;
    if (!(roto->ftsoptions & (FTS_LOGICAL|FTS_PHYSICAL)))
	roto->ftsoptions |= FTS_PHYSICAL;
    roto->ftsoptions |= FTS_NOCHDIR;

    roto->visitD = NULL;
    roto->visitF = _rmTreeVisitF;
    roto->visitDP = _rmTreeVisitDP;

    rc = rotoCWalk(roto);

    roto->visitD = NULL;
    roto->visitF = NULL;
    roto->visitDP = NULL;

    roto->paths = argvFree(roto->paths);

ROTODBG((stderr, "<-- %s(%s) rc %d\n", __FUNCTION__, s, rc));
    return rc;
}

/*==============================================================*/

#ifdef	REFERENCE
# python library imports
#from exceptions import Exception

# our imports

# classes
class Error(Exception):
    "base class for our errors."
    def __init__(self, msg, status=None):
        Exception.__init__(self)
        self.msg = msg
        self.resultcode = 1
        if status is not None:
            self.resultcode = status

    def __str__(self):
        return self.msg

# result/exit codes
# 0 = yay!
# 1 = something happened  - it bad
# 5 = cmdline processing error
# 10 = problem building the package
# 20 = error in the chroot of some kind
# 30 = Yum emitted an error of some sort
# 40 = some error in the pkg we are building
# 50 = tried to fork a subcommand and it errored out
# 60 = buildroot locked
# 70 = result dir could not be created

class BuildError(Error):
    "rpmbuild failed."
    def __init__(self, msg):
        Error.__init__(self, msg)
        self.msg = msg
        self.resultcode = 10

class RootError(Error):
    "failed to set up chroot"
    def __init__(self, msg):
        Error.__init__(self, msg)
        self.msg = msg
        self.resultcode = 20

class YumError(RootError):
    "yum failed."
    def __init__(self, msg):
        RootError.__init__(self, msg)
        self.msg = msg
        self.resultcode = 30

class PkgError(Error):
    "error with the srpm given to us."
    def __init__(self, msg):
        Error.__init__(self, msg)
        self.msg = msg
        self.resultcode = 40

class BuildRootLocked(Error):
    "build root in use by another process."
    def __init__(self, msg):
        Error.__init__(self, msg)
        self.msg = msg
        self.resultcode = 60

class BadCmdline(Error):
    "user gave bad/inconsistent command line."
    def __init__(self, msg):
        Error.__init__(self, msg)
        self.msg = msg
        self.resultcode = 05

class InvalidArchitecture(Error):
    "invalid host/target architecture specified."
    def __init__(self, msg):
        Error.__init__(self, msg)
        self.msg = msg
        self.resultcode = 06

class ResultDirNotAccessible(Error):
    "directory not accessible"
Could not create output directory for built rpms. The directory specified was:
    %s

Try using the --resultdir= option to select another location. Recommended location is --resultdir=~/mock/.
""
    def __init__(self, msg):
        Error.__init__(self, msg)
        self.msg = msg
        self.resultcode = 70

class UnshareFailed(Error):
    "call to C library unshare(2) syscall failed"

    def __init__(self, msg):
        Error.__init__(self, msg)
        self.msg = msg
        self.resultcode = 80
#endif	/* REFERENCE */

/*==============================================================*/

#ifdef	REFERENCE
# python library imports
import os

# our imports
from mock.trace_decorator import traceLog, decorate

# class
class uidManager(object):
#endif	/* REFERENCE */

#ifdef	REFERENCE
    def __init__(self, unprivUid=-1, unprivGid=-1):
        self.privStack = []
        self.unprivUid = unprivUid
        self.unprivGid = unprivGid
#endif	/* REFERENCE */

static int uidManager_elevatePrivs(ROTO_t roto)
{
    int rc = 0;
int xx;
    xx = setresuid(0, 0, 0);
    xx = setregid(0, 0);
ROTODBG((stderr, "<-- %s(%p) %ld:%ld\n", __FUNCTION__, roto, (long)getuid(), (long)getgid()));
    return rc;
}

static int uidManager_becomeUser(ROTO_t roto, uid_t uid, gid_t gid)
{
    int rc = 0;
int xx;
    xx = uidManager_elevatePrivs(roto);
    if (gid != _GIDNONE)
	xx = setregid(gid, gid);
    xx = setresuid(uid, uid, 0);
ROTODBG((stderr, "<-- %s(%p) %ld:%ld\n", __FUNCTION__, roto, (long)getuid(), (long)getgid()));
    return rc;
}

static int uidManagerChangeOwner(ROTO_t roto, const char * path, uid_t uid, gid_t gid)
{
    int rc = 0;
int xx;
    xx = uidManager_elevatePrivs(roto);
    if (uid == _UIDNONE)
	uid = roto->unprivUid;
    if (gid == _GIDNONE)
	gid = uid;
    xx = Chown(path, uid, gid);
ROTODBG((stderr, "<-- %s(%p) rc %d\n", __FUNCTION__, roto, rc));
    return rc;
}

static int uidManager_push(ROTO_t roto)
{
    int rc = 0;
#ifdef	REFERENCE
    /* save current ruid, euid, rgid, egid */
    self.privStack.append({
	"ruid": getuid(),
	"euid": geteuid(),
	"rgid": getgid(),
	"egid": getegid(),
	})
#endif	/* REFERENCE */
ROTODBG((stderr, "<-- %s(%p) rc %d\n", __FUNCTION__, roto, rc));
    return rc;
}

static int uidManagerBecomeUser(ROTO_t roto, uid_t uid, gid_t gid)
{
    int rc = 0;
int xx;
    /* save current ruid, euid, rgid, egid */
    xx = uidManager_push(roto);
    xx = uidManager_becomeUser(roto, uid, gid);
ROTODBG((stderr, "<-- %s(%p) rc %d\n", __FUNCTION__, roto, rc));
    return rc;
}

static int uidManagerDropPrivsTemp(ROTO_t roto)
{
    int rc = 0;
int xx;
    /* save current ruid, euid, rgid, egid */
    xx = uidManager_push(roto);
    xx = uidManager_becomeUser(roto, roto->unprivUid, roto->unprivGid);
ROTODBG((stderr, "<-- %s(%p) rc %d\n", __FUNCTION__, roto, rc));
    return rc;
}

static int uidManagerRestorePrivs(ROTO_t roto)
{
    int rc = 0;
int xx;

    /* back to root first */
    xx = uidManager_elevatePrivs(roto);

#ifdef	REFERENCE
    /* then set saved */
    privs = self.privStack.pop()
    xx = setregid(privs['rgid'], privs['egid'])
    xx = setresuid(privs['ruid'], privs['euid'])
#endif	/* REFERENCE */
ROTODBG((stderr, "<-- %s(%p) %ld:%ld\n", __FUNCTION__, roto, (long)getuid(), (long)getgid()));
    return rc;
}

static int uidManagerDropPrivsForever(ROTO_t roto)
{
    int rc = 0;
int xx;
    xx = uidManager_elevatePrivs(roto);
    xx = setregid(roto->unprivGid, roto->unprivGid);
    xx = setreuid(roto->unprivUid, roto->unprivUid);
ROTODBG((stderr, "<-- %s(%p) %ld:%ld\n", __FUNCTION__, roto, (long)getuid(), (long)getgid()));
    return rc;
}

/*==============================================================*/

#ifdef	REFERENCE
# python library imports
import ctypes
import fcntl
import os
import os.path
import rpm
import rpmUtils
import rpmUtils.transaction
import select
import subprocess
import time
import errno

# our imports
import mock.exception
from mock.trace_decorator import traceLog, decorate, getLog
import mock.uid as uid

_libc = ctypes.cdll.LoadLibrary(None)
_errno = ctypes.c_int.in_dll(_libc, "errno")
_libc.personality.argtypes = [ctypes.c_ulong]
_libc.personality.restype = ctypes.c_int
_libc.unshare.argtypes = [ctypes.c_int,]
_libc.unshare.restype = ctypes.c_int
CLONE_NEWNS = 0x00020000

# taken from sys/personality.h
PER_LINUX32=0x0008
PER_LINUX=0x0000
personality_defs = {
    'x86_64': PER_LINUX, 'ppc64': PER_LINUX, 'sparc64': PER_LINUX,
    'i386': PER_LINUX32, 'i586': PER_LINUX32, 'i686': PER_LINUX32,
    'ppc': PER_LINUX32, 'sparc': PER_LINUX32, 'sparcv9': PER_LINUX32,
    'ia64' : PER_LINUX, 'alpha' : PER_LINUX,
    's390' : PER_LINUX32, 's390x' : PER_LINUX,
}
#endif	/* REFERENCE */

#ifdef	REFERENCE
# classes
class commandTimeoutExpired(mock.exception.Error):
    def __init__(self, msg):
        mock.exception.Error.__init__(self, msg)
        self.msg = msg
        self.resultcode = 10
#endif	/* REFERENCE */

static int utilMkpath(const char * _dn, mode_t mode, uid_t uid, gid_t gid)
{
    const char * dn = rpmExpand(_dn, NULL);
    int rc = 0;

    /* XXX FIXME? mock permitted array of dirnames but never uses. */
    rpmlog(RPMLOG_DEBUG, "ensuring that dir exists: %s\n", dn);
    rc = rpmioMkpath(dn, mode, uid, gid);
ROTODBG((stderr, "<-- %s(%s) rc %d\n", __FUNCTION__, dn, rc));
    dn = _free(dn);
    return rc;
}

static int utilTouch(const char * fn)
{
    FILE * fp;
    int rc = 0;
int xx;

    rpmlog(RPMLOG_DEBUG, "touching file: %s\n", fn);
    fp = fopen(fn, "w");
    if (fp)
	xx = fclose(fp);
ROTODBG((stderr, "<-- %s(%s) rc %d\n", __FUNCTION__, fn, rc));
    return rc;
}

/**
 * Version of shutil.rmtree that ignores no-such-file-or-directory errors,
 *	and tries harder if it finds immutable files.
 */
static int utilRmtree(ROTO_t roto, const char * path, int selinux)
{
    int rc = 0;
#ifdef	REFERENCE
    do_selinux_ops = False
    if kargs.has_key('selinux'):
        do_selinux_ops = kargs['selinux']
        del kargs['selinux']
    tryAgain = 1
    retries = 0
    failedFilename = None
    getLog().debug("remove tree: %s" % path)
    while tryAgain:
        tryAgain = 0
/*      try: */
            xx = shutilRmTree(roto, path, *args, **kargs);
        except OSError, e:
            if e.errno == errno.ENOENT: # no such file or directory
                pass
            elif do_selinux_ops and (e.errno==errno.EPERM or e.errno==errno.EACCES):
                tryAgain = 1
                if failedFilename == e.filename:
                    raise
                failedFilename = e.filename
                os.system("chattr -R -i %s" % path)
            elif e.errno == errno.EBUSY:
                retries += 1
                if retries > 1:
                    raise
                tryAgain = 1
                getLog().debug("retrying failed tree remove after sleeping a bit")
                time.sleep(2)
            else:
                raise
#else
	    rc = shutilRmTree(roto, path);
#endif	/* REFERENCE */
ROTODBG((stderr, "<-- %s(%s) rc %d\n", __FUNCTION__, path, rc));
    return rc;
}

/**
 * Kill off anything that is still chrooted.
 */
static int utilOrphansKill(const char * rootToKill, int killsig)
{
    static const char _procdir[] = "/proc";
    DIR * dir = Opendir(_procdir);
    struct dirent * dp;
    const char * dn = NULL;
    int rc = 0;
int xx;

    rpmlog(RPMLOG_DEBUG, "kill orphans\n");

    if (dir == NULL)	/* XXX /proc not mounted?!? */
	goto exit;
    dn = rpmGetPath(rootToKill, NULL);
    while ((dp = Readdir(dir)) != NULL) {
	const char * fn = dp->d_name;
	char b[BUFSIZ];
	int status;
	pid_t pid;

	/* Skip "." and ".." and non-numeric directories. */
	if (fn[0] == '.' && (fn[1] == '\0' || (fn[1] == '.' && fn[2] == '\0')))
	    continue;
	while (*fn && xisdigit(*fn))
	    fn++;
	if (*fn != '\0')
	    continue;

	/* Find the pid's root directory. */
	pid = atol(dp->d_name);
	fn = rpmGetPath(_procdir, "/", dp->d_name, "/root", NULL);
	xx = Readlink(fn, b, sizeof(b));
	fn = _free(fn);
	if (xx)
	    continue;
	b[sizeof(b)-1] = '\0';
	if ((fn = Realpath(b, NULL)) == NULL)
	    continue;
	xx = strcmp(fn, dn);
	fn = _free(fn);
	if (xx)
	    continue;

	/* Skip if not using the chroot. */
	if (strcmp(b, dn))
	    continue;

	/* Terminate and reap. */
	if (kill(pid, killsig))
	    continue;
	status = 0;
	xx = waitpid(pid, &status, 0);	/* XXX WNOHANG? */
    }
    xx = Closedir(dir);
    
exit:
ROTODBG((stderr, "<-- %s(%s) rc %d\n", __FUNCTION__, dn, rc));
    dn = _free(dn);
    return rc;
}

#ifdef	REFERENCE
def yieldSrpmHeaders(srpms, plainRpmOk=0):
    ts = rpmUtils.transaction.initReadOnlyTransaction()
    for srpm in srpms:
/*      try: */
            hdr = rpmUtils.miscutils.hdrFromPackage(ts, srpm)
        except (rpmUtils.RpmUtilsError,), e:
            raise mock.exception.Error, "Cannot find/open srpm: %s. Error: %s" % (srpm, ''.join(e))

        if not plainRpmOk and hdr[rpm.RPMTAG_SOURCEPACKAGE] != 1:
            raise mock.exception.Error("File is not an srpm: %s." % srpm )

        yield hdr
#endif	/* REFERENCE */

#ifdef	REFERENCE
def getNEVRA(hdr):
    name = hdr[rpm.RPMTAG_NAME]
    ver  = hdr[rpm.RPMTAG_VERSION]
    rel  = hdr[rpm.RPMTAG_RELEASE]
    epoch = hdr[rpm.RPMTAG_EPOCH]
    arch = hdr[rpm.RPMTAG_ARCH]
    if epoch is None: epoch = 0
    return (name, epoch, ver, rel, arch)
#endif	/* REFERENCE */

#ifdef	REFERENCE
def getAddtlReqs(hdr, conf):
    # Add the 'more_buildreqs' for this SRPM (if defined in config file)
    (name, epoch, ver, rel, arch) = getNEVRA(hdr)
    reqlist = []
    for this_srpm in ['-'.join([name, ver, rel]),
                      '-'.join([name, ver]),
                      '-'.join([name]),]:
        if conf.has_key(this_srpm):
            more_reqs = conf[this_srpm]
            if type(more_reqs) in (type(u''), type(''),):
                reqlist.append(more_reqs)
            else:
                reqlist.extend(more_reqs)
            break

    return rpmUtils.miscutils.unique(reqlist)
#endif	/* REFERENCE */

static int utilUnshare(ROTO_t roto, int flags)
{
    int rc = 0;
    rpmlog(RPMLOG_DEBUG, "Unsharing. Flags: 0x%x\n", flags);
    if (!F_ISSET(roto, NOUNSHARE))
	rc = unshare(flags);
ROTODBG((stderr, "<-- %s(0x%x) rc %d\n", __FUNCTION__, flags, rc));
    return rc;
}

static const char * sudoCmd(const char * _cmd)
{
    char * cmd = rpmExpand((getuid() ? "sudo " : ""), _cmd,  NULL);
    char * out = rpmExpand("%(", cmd, " 2>&1 )", NULL);
ROTODBG((stderr, "<-- %s(%s) out |%s|\n", __FUNCTION__, cmd, out));
    cmd = _free(cmd);
    return out;
}

typedef struct exec_s * EXEC_t;

struct exec_s {
    const char * personality;
    const char * chrootPath;
    const char * cwd;
    uid_t uid;
    gid_t gid;

    ARGV_t av;
    ARGV_t env;
    int shell;
    int returnOutput;
    int timeout;
    int raiseExc;
};

static int execChroot(EXEC_t xp)
{
    int rc = 0;
    uid_t uid;
    uid_t euid;
int xx;

    if (xp->chrootPath == NULL
     || xp->chrootPath[0] == '\0'
     || (xp->chrootPath[0] == '/' && xp->chrootPath[1] == '\0'))
	goto exit;

    uid = getuid();
    euid = geteuid();
    xx = setresuid(0, 0, 0);
    xx = chdir(xp->chrootPath);
    xx = chroot(xp->chrootPath);
    xx = chdir("/");
    xx = setresuid(uid, euid, 0);

exit:
ROTODBG((stderr, "<-- %s(%p) rc %d\n", __FUNCTION__, xp, rc));
    return rc;
}

static int execChdir(EXEC_t xp)
{
    int rc = 0;
    if (xp->cwd)
	rc = chdir(xp->cwd);
ROTODBG((stderr, "<-- %s(%p) rc %d\n", __FUNCTION__, xp, rc));
    return rc;
}

static int execDropPrivs(EXEC_t xp)
{
    int rc = 0;
    if (xp->gid != _GIDNONE && (rc = setregid(xp->gid, xp->gid)) != 0)
	goto exit;
    if (xp->uid != _UIDNONE && (rc = setreuid(xp->uid, xp->uid)) != 0)
	goto exit;
exit:
ROTODBG((stderr, "<-- %s(%p) rc %d\n", __FUNCTION__, xp, rc));
    return rc;
}

static int execPersonality(EXEC_t xp)
{
    int rc = 0;
#ifdef	REFERENCE
def condPersonality(per=None):
    if per is None or per in ('noarch',):
        return
    if personality_defs.get(per, None) is None:
        return
    res = _libc.personality(personality_defs[per])
    if res == -1:
        raise OSError(_errno.value, os.strerror(_errno.value))
#else
    /* XXX FIXME: look up flags for xp->personality */
    rc = personality(PER_LINUX);	/* XXX PER_LINUX32 is the other case */
#endif	/* REFERENCE */
ROTODBG((stderr, "<-- %s(%p) rc %d\n", __FUNCTION__, xp, rc));
    return rc;
}

static EXEC_t execFree(EXEC_t xp)
{
ROTODBG((stderr, "<-- %s(%p)\n", __FUNCTION__, xp));
    if (xp) {
	xp->personality = _free(xp->personality);
	xp->chrootPath = _free(xp->chrootPath);
	xp->cwd = _free(xp->cwd);
	xp = _free(xp);
    }
    return NULL;
}

#ifdef	REFERENCE
class ChildPreExec(object):
    def __init__(self, personality, chrootPath, cwd, uid, gid):
        self.personality = personality
        self.chrootPath  = chrootPath
        self.cwd = cwd
        self.uid = uid
        self.gid = gid
#else
static EXEC_t execNew(const char * personality, const char * chrootPath,
		const char * cwd, uid_t uid, gid_t gid)
{
    EXEC_t xp = xcalloc(1, sizeof(*xp));
    xp->personality = (personality ? xstrdup(personality) : NULL);
    xp->chrootPath = (chrootPath ? xstrdup(chrootPath) : NULL);
    xp->cwd = (cwd ? xstrdup(cwd) : NULL);
    xp->uid = uid;
    xp->gid = gid;
ROTODBG((stderr, "<-- %s() xp %p\n", __FUNCTION__, xp));
    return xp;
}
#endif	/* REFERENCE */

static int execCall(EXEC_t xp)
{
    pid_t pid = setsid();
    int rc = 0;
int xx;
pid = pid;

    xx = execPersonality(xp);
    xx = execChroot(xp);
    xx = execDropPrivs(xp);
    xx = execChdir(xp);

ROTODBG((stderr, "<-- %s(%p) rc %d\n", __FUNCTION__, xp, rc));
    return rc;
}

#ifdef	REFERENCE
def logOutput(fds, logger, returnOutput=1, start=0, timeout=0):
    out=""
    done = 0

    # set all fds to nonblocking
    for fd in fds:
        flags = fcntl.fcntl(fd, fcntl.F_GETFL)
        if not fd.closed:
            fcntl.fcntl(fd, fcntl.F_SETFL, flags| os.O_NONBLOCK)

    tail = ""
    while not done:
        if (time.time() - start)>timeout and timeout!=0:
            done = 1
            break

        i_rdy,o_rdy,e_rdy = select.select(fds,[],[],1)
        for s in i_rdy:
            # slurp as much input as is ready
            input = s.read()
            if input == "":
                done = 1
                break
            if logger is not None:
                lines = input.split("\n")
                if tail:
                    lines[0] = tail + lines[0]
                # we may not have all of the last line
                tail = lines.pop()
                for line in lines:
                    if line == '': continue
                    logger.debug(line)
                for h in logger.handlers:
                    h.flush()
            if returnOutput:
                out += input
    if tail and logger is not None:
        logger.debug(tail)
    return out
#endif	/* REFERENCE */

#ifdef	REFERENCE
def selinuxEnabled():
    """Check if SELinux is enabled (enforcing or permissive)."""
/*  try: */
        if open("/selinux/enforce").read().strip() in ("1", "0"):
            return True
    except:
        pass
    return False
#endif	/* REFERENCE */

/*
 * logger =
 * out = [1|0]
 * chrootPath
 *
 * The "Not-as-complicated" version
 *
 */
#ifdef	REFERENCE
def do(command, shell=False, chrootPath=None, cwd=None, timeout=0, raiseExc=True, returnOutput=0, uid=None, gid=None, personality=None, *args, **kargs):
#endif	/* REFERENCE */
static const char * utilDo(EXEC_t xp)
{
    char * out = NULL;

ROTODBG((stderr, "--> %s(%p)\n", __FUNCTION__, xp));
argvPrint(__FUNCTION__, xp->av, NULL);

#ifdef	REFERENCE
    logger = kargs.get("logger", getLog())
    out = ""
    start = time.time()
    preexec = ChildPreExec(personality, chrootPath, cwd, uid, gid)
/*  try: */
        child = None
        logger.debug("Executing command: %s" % command)
        child = subprocess.Popen(
            command,
            shell=shell,
            bufsize=0, close_fds=True,
            stdin=open("/dev/null", "r"),
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            preexec_fn = preexec,
            )

        # use select() to poll for output so we dont block
        out = logOutput([child.stdout, child.stderr],
                           logger, returnOutput, start, timeout)

    except:
        # kill children if they arent done
        if child is not None and child.returncode is None:
            os.killpg(child.pid, 9)
/*      try: */
            if child is not None:
                os.waitpid(child.pid, 0)
        except:
            pass
        raise

    # wait until child is done, kill it if it passes timeout
    niceExit=1
    while child.poll() is None:
        if (time.time() - start)>timeout and timeout!=0:
            niceExit=0
            os.killpg(child.pid, 15)
        if (time.time() - start)>(timeout+1) and timeout!=0:
            niceExit=0
            os.killpg(child.pid, 9)

    if not niceExit:
        raise commandTimeoutExpired, ("Timeout(%s) expired for command:\n # %s\n%s" % (timeout, command, out))

    logger.debug("Child returncode was: %s" % str(child.returncode))
    if raiseExc and child.returncode:
        if returnOutput:
            raise mock.exception.Error, ("Command failed: \n # %s\n%s" % (command, out), child.returncode)
        else:
            raise mock.exception.Error, ("Command failed. See logs for output.\n # %s" % (command,), child.returncode)
#endif	/* REFERENCE */

ROTODBG((stderr, "<-- %s(%p) out %p\n", __FUNCTION__, xp, out));
    return out;
}

#ifdef	REFERENCE
def is_in_dir(path, directory):
    """Tests whether `path` is inside `directory`."""
    # use realpath to expand symlinks
    path = os.path.realpath(path)
    directory = os.path.realpath(directory)

    return os.path.commonprefix([path, directory]) == directory
#endif	/* REFERENCE */

/*==============================================================*/

static int rotoLock(ROTO_t roto, const char * fn, FILE ** fpp)
{
    struct flock info;
    int rc = 0;

ROTODBG((stderr, "--> %s(%p,%s,%p) fp %p\n", __FUNCTION__, roto, fn, fpp, (fpp ? *fpp : NULL)));
assert(*fpp == NULL);
    (*fpp) = fopen(fn, "a+");
    if (*fpp == NULL) {
	rc = 0;
	goto exit;
    }

    info.l_type = F_WRLCK;
    info.l_whence = SEEK_SET;
    info.l_start = 0;
    info.l_len = 0;
    info.l_pid = 0;

    /* XXX FIXME? mock does fcntl.LOCK_NB with IOerr exception */
    rc = fcntl(fileno(*fpp), F_SETLKW, &info);
    rc = (rc ? 0 : 1);	/* XXX impedance match the return */

exit:
ROTODBG((stderr, "<-- %s(%s) rc %d\n", __FUNCTION__, fn, rc));
    return rc;
}

static int rotoUnlock(ROTO_t roto, const char * fn, FILE ** fpp)
{
    int rc = 0;
int xx;

ROTODBG((stderr, "--> %s(%p,%s,%p) fp %p\n", __FUNCTION__, roto, fn, fpp, (fpp ? *fpp : NULL)));
assert(*fpp != NULL);
    if (*fpp != NULL) {
	xx = fclose(*fpp);
	(*fpp) = NULL;
	xx = Unlink(fn);
    }

ROTODBG((stderr, "<-- %s(%s) rc %d\n", __FUNCTION__, fn, rc));
    return rc;
}

/*==============================================================*/

#ifdef	REFERENCE
try:
    import uuid
    gotuuid = True
except:
    gotuuid = False


# our imports
import mock.util
import mock.exception
from mock.trace_decorator import traceLog, decorate, getLog

# classes
class Root(object):
    """controls setup of chroot environment"""
#endif	/* REFERENCE */

#ifdef	REFERENCE
//    def __init__(self, config, uidManager):
//        roto->_state = "unstarted";
//        self.uidManager = uidManager
//        self._hooks = {}
//        roto->chrootWasCached = False
//        roto->chrootWasCleaned = False
//        self.preExistingDeps = []
//        roto->logging_initialized = 0;
//        roto->buildrootLock = NULL;
//        roto->version = roto->version;
//
//        roto->sharedRootName = roto->_root;
//        if config.has_key('unique-ext'):
//            roto->_root = "%s-%s" % (roto->_root, roto->uniqueext)
//
//        roto->basedir = os.path.join(roto->basedir, roto->_root)
//        roto->rpmbuild_arch = roto->rpmbuild_arch;
//        roto->_rootdir = os.path.join(roto->basedir, 'root')
//        roto->homedir = roto->chroothome;
//        roto->builddir = os.path.join(roto->homedir, 'build')
//
//        /* result dir */
//        roto->resultdir = roto->resultdir % config
//
//        self.root_log = getLog("mock")
//        self.build_log = getLog("mock.Root.build")
//        self._state_log = getLog("mock.Root.state")
//
//        # config options
//        roto->configs = roto->configs;
//        self.config_name = roto->chroot_name;
//        roto->chrootuid = roto->chrootuid;
//        roto->chrootuser = 'mockbuild'
//        roto->chrootgid = roto->chrootgid;
//        roto->chrootgroup = 'mockbuild'
//        roto->yum_conf_content = roto->yum_conf_content;
//        roto->use_host_resolv = roto->use_host_resolv;
//        roto->files = roto->files;
//        roto->chroot_setup_cmd = roto->chroot_setup_cmd;
//        if isinstance(roto->chroot_setup_cmd, basestring):
//            # accept strings in addition to other sequence types
//            roto->chroot_setup_cmd = roto->chroot_setup_cmd.split()
//        #roto->yum_path = '/usr/bin/yum'
//        #roto->yum_builddep_path = '/usr/bin/yum-builddep'
//        roto->yum_path = roto->yum_path;
//        roto->yum_builddep_path = roto->yum_builddep_path;
//        roto->macros = roto->macros;
//        roto->more_buildreqs = roto->more_buildreqs;
//        roto->cache_topdir = roto->cache_topdir;
//        roto->cachedir = os.path.join(roto->cache_topdir, roto->sharedRootName)
//        roto->useradd = roto->useradd;
//        roto->online = roto->online;
//        roto->internal_dev_setup = roto->internal_dev_setup;
//
//        roto->plugins = roto->plugins;
//        roto->plugin_conf = roto->plugin_conf;
//        roto->plugin_dir = roto->plugin_dir;
//        for key in roto->plugin_conf.keys():
//            if not key.endswith('_opts'): continue
//            roto->plugin_conf[key]['basedir'] = roto->basedir
//            roto->plugin_conf[key]['cache_topdir'] = roto->cache_topdir
//            roto->plugin_conf[key]['cachedir'] = roto->cachedir
//            roto->plugin_conf[key]['root'] = roto->sharedRootName
//
//        # mount/umount
//        roto->umountCmds = ['umount -n %s' % chrootMakeChrootPath(roto, 'proc', NULL),
//                'umount -n %s' % chrootMakeChrootPath(roto, 'sys', NULL)
//               ]
//        roto->mountCmds = ['mount -n -t proc   mock_chroot_proc   %s' % chrootMakeChrootPath(roto, 'proc', NULL),
//                'mount -n -t sysfs  mock_chroot_sysfs  %s' % chrootMakeChrootPath(roto, 'sys', NULL),
//               ]
//
//        self.build_log_fmt_str = config['build_log_fmt_str']
//        self.root_log_fmt_str = config['root_log_fmt_str']
//        self._state_log_fmt_str = config['state_log_fmt_str']
//
//        (void) chrootState(roto, "init plugins");
//        xx = chroot_initPlugins(roto);
//
//        # default to not doing selinux things
//        roto->selinux = False
//
//        # if the selinux plugin is disabled and we have SELinux enabled
//        # on the host, we need to do SELinux things, so set the selinux
//        # state variable to true
//        if roto->plugin_conf['selinux_enable'] == False and mock.util.selinuxEnabled():
//            roto->selinux = 1;
//
//        # officially set state so it is logged
//        (void) chrootState(roto, "start");
#endif	/* REFERENCE */

static int chroot_resetLogging(ROTO_t roto)
{
    int rc = 0;
int xx;

    /* ensure we dont attach the handlers multiple times. */
    if (roto->logging_initialized)
	return
    roto->logging_initialized = 1;

/*  try: */
	xx = uidManagerDropPrivsTemp(roto);

	/*
	 * attach logs to log files.
	 * This happens in addition to anything that
	 * is set up in the config file... ie. logs go everywhere
	 */
#ifdef	REFERENCE
	for (log, filename, fmt_str) in (
                    (self._state_log, "state.log", self._state_log_fmt_str),
                    (self.build_log, "build.log", self.build_log_fmt_str),
                    (self.root_log, "root.log", self.root_log_fmt_str)):
                fullPath = os.path.join(roto->resultdir, filename)
                fh = logging.FileHandler(fullPath, "a+")
                formatter = logging.Formatter(fmt_str)
                fh.setFormatter(formatter)
                fh.setLevel(logging.NOTSET)
                log.addHandler(fh)
#endif	/* REFERENCE */
                rpmlog(RPMLOG_INFO, "Mock Version: %s\n", roto->version);
/*  finally: */
	xx = uidManagerRestorePrivs(roto);

ROTODBG((stderr, "<-- %s() rc %d\n", __FUNCTION__, rc));
    return rc;
}

static const char * chroot_show_path_user(ROTO_t roto, const char * path)
{
    const char * out = NULL;
    const char * cmd = rpmExpand("/sbin/fuser -a -v ", path, NULL);
    rpmlog(RPMLOG_DEBUG, "using 'fuser' to find users of %s\n", path);
    out = sudoCmd(cmd);
    rpmlog(RPMLOG_DEBUG, "%s\n", out);
    cmd = _free(cmd);
ROTODBG((stderr, "<-- %s(%s) out %s\n", __FUNCTION__, path, out));
    return out;
}

static int chroot_unlock_and_rm_chroot(ROTO_t roto)
{
    int rc = 0;
    const char * dn = rpmGetPath(roto->basedir, NULL);
    const char * dntmp = NULL;
    struct stat sb;
int xx;

    if (Stat(dn, &sb))
	goto exit;
    dntmp = rpmGetPath(dn, ".tmp", NULL);
    if (Stat(dntmp, &sb) == 0)
	xx = utilRmtree(roto, dntmp, roto->selinux);
    xx = Rename(dn, dntmp);

    if (roto->buildrootLock) {	/* XXX best effort only */
	const char * fn = rpmGetPath(roto->basedir, "/buildroot.lock", NULL);
	xx = rotoUnlock(roto, fn, &roto->buildrootLock);
	fn = _free(fn);
    }

    xx = utilRmtree(roto, dntmp, roto->selinux);

/*  try: */
	xx = utilRmtree(roto, dntmp, roto->selinux);
#ifdef	REFERENCE
    except OSError, e:
	rpmlog(RPMLOG_ERR, "%s\n", e);
	rpmlog(RPMLOG_ERR, "contents of /proc/mounts:\n%s\n" % open('/proc/mounts').read())
	rpmlog(RPMLOG_ERR, "looking for users of %s", t);
	xx = chroot_show_path_user(roto, t);
	raise
#endif	/* REFERENCE */
    rpmlog(RPMLOG_INFO, "chroot (%s) unlocked and deleted\n", dn);

exit:
    dntmp = _free(dntmp);
    dn = _free(dn);
ROTODBG((stderr, "<-- %s(%p) rc %d\n", __FUNCTION__, roto, rc));
    return rc;
}

/**
 * Umount all mounted chroot fs.
 */
static int chroot_umountall(ROTO_t roto)
{
    ARGV_t mountpoints = NULL;
    int nmountpoints = 0;
    const char * cmd = NULL;
    const char * out = NULL;
    const char * fn;
    int rc = 0;
    int i;
int xx;

    /* first try removing all expected mountpoints. */
    for (i = argvCount(roto->umountCmds) - 1; i >= 0; i--) {
	cmd = rpmExpand(roto->umountCmds[i], NULL);
	out = sudoCmd(cmd);
	if (out && *out)
	    rpmlog(RPMLOG_DEBUG, "umountall: %s\n%s\n", cmd, out);
	out = _free(out);
	cmd = _free(cmd);
    }

    /* then remove anything that might be left around. */
    FD_t fd = Fopen("/proc/mounts", "r.fpio");
    if (fd) {
	mountpoints = NULL;
	xx = argvFgets(&mountpoints, fd);
	xx = Fclose(fd);
    }
    nmountpoints = argvCount(mountpoints);

    /*
     * umount in reverse mount order to prevent nested mount issues that
     * may prevent clean unmount.
     */
    const char * dn = rpmGetPath(roto->_rootdir, NULL);
    const char * rootdir = Realpath(dn, NULL);
    size_t nb = (rootdir ? strlen(rootdir) : 0);

    for (i = nmountpoints - 1; i >= 0;  i--) {
	ARGV_t mav = NULL;
	size_t nfn;

	xx = argvSplit(&mav, mountpoints[i], NULL);
	fn = Realpath(mav[1], NULL);
assert(fn);	/* XXX coverity #1060675 */
	nfn = strlen(fn);

	if (nfn >= nb && !strncmp(fn, dn, nb)
	 && (fn[nb] == '/' || fn[nb] == '\0'))
	{
	    cmd = rpmExpand("umount -n ", fn, NULL);
	    rpmlog(RPMLOG_WARNING, "Forcibly unmounting '%s' from chroot.\n", fn);
	    out = sudoCmd(cmd);
	    if (out && *out)
		rpmlog(RPMLOG_DEBUG, "umountall (forced): %s\n%s\n", cmd, out);
	    out = _free(out);
	    cmd = _free(cmd);
	}
	fn = _free(fn);
	mav = argvFree(mav);
    }
    rootdir = _free(rootdir);
    dn = _free(dn);

    mountpoints = argvFree(mountpoints);	/* XXX coverity #1060682 */
    nmountpoints = 0;

ROTODBG((stderr, "<-- %s() rc %d\n", __FUNCTION__, rc));
    return rc;
}

/**
 * Mount 'normal' fs like /dev/ /proc/ /sys.
 */
static int chroot_mountall(ROTO_t roto)
{
    int rc = 0;
    int i;

    if (roto->mountCmds)
    for (i = 0; roto->mountCmds[i]; i++) {
	const char * cmd = rpmExpand(roto->mountCmds[i], NULL);
	const char * out = sudoCmd(cmd);
	if (out && *out)
	    rpmlog(RPMLOG_DEBUG, "mountall: %s\n%s\n", cmd, out);
	out = _free(out);
	cmd = _free(cmd);
    }

ROTODBG((stderr, "<-- %s() rc %d\n", __FUNCTION__, rc));
    return rc;
}

#ifdef	DYING
static char * chrootMakeChrootPath(ROTO_t roto, ...)
{
    char * fn = NULL;
    ARGV_t av = NULL;

    va_list ap;
int xx;

    xx = argvAdd(&av, roto->_rootdir);
    
    va_start(ap, roto);
    while ((fn = va_arg(ap, char *)) != NULL)
	xx = argvAdd(&av, fn);
    va_end(ap);

    fn = rpmCleanPath(argvJoin(av, '/'));
    av = argvFree(av);
    
ROTODBG((stderr, "<-- %s(%p, ...) fn %s\n", __FUNCTION__, roto, fn));
    return fn;
}
#endif	/* DYING */

static int chroot_setupDev(ROTO_t roto, int interactive)
{
    struct passwd * pw = NULL;
    struct group * gr = NULL;
    const char * fn;
    const char * cmd;
    mode_t prevMask;
    struct utsname _uts;
    const char * kver;
    struct stat sb;
    int rc = 0;
    int i;
int xx;

    /* files in /dev */
    fn = rpmGetPath(roto->_rootdir, "/dev", NULL);
    xx = utilRmtree(roto, fn, roto->selinux);
    fn = _free(fn);

    fn = rpmGetPath(roto->_rootdir, "/dev/pts", NULL);
    xx = utilMkpath(fn, 0755, _UIDNONE, _GIDNONE);
    fn = _free(fn);
    fn = rpmGetPath(roto->_rootdir, "/dev/shm", NULL);
    xx = utilMkpath(fn, 0755, _UIDNONE, _GIDNONE);
    fn = _free(fn);

    prevMask = umask(0000);

    kver = ((xx = uname(&_uts)) == 0 ? _uts.release : NULL);
    rpmlog(RPMLOG_DEBUG, "kver == %s\n", _uts.release);

#define _makedev(__major, __minor) \
  (dev_t)((__minor & 0xff) | ((__major & 0xfff) << 8) \
          | (((unsigned long long) (__minor & ~0xff)) << 12) \
          | (((unsigned long long) (__major & ~0xfff)) << 32))
    static struct devFiles_s {
	const char * path;
	mode_t mode;
	unsigned long long major;
	unsigned long long minor;
	const char * user;
	const char * group;
    } devFiles[] = {
	{ "/dev/null",		S_IFCHR | 0666, 1, 3,	NULL, NULL },
	{ "/dev/full",		S_IFCHR | 0666, 1, 7,	NULL, NULL },
	{ "/dev/zero",		S_IFCHR | 0666, 1, 5,	NULL, NULL },
	{ "/dev/random",	S_IFCHR | 0666, 1, 8,	NULL, NULL },
	{ "/dev/urandom",	S_IFCHR | 0444, 1, 9,	NULL, NULL },
	{ "/dev/tty",		S_IFCHR | 0666, 5, 0,	"root", "tty" },
	{ "/dev/console",	S_IFCHR | 0600, 5, 1,	NULL, NULL },
	{ "/dev/ptmx",		S_IFCHR | 0666, 5, 2,	"root", "tty" },
	{ NULL, 0, 0, 0, NULL, NULL },
    };
    for (i = 0; devFiles[i].path != NULL; i++) {
	const char * _path = devFiles[i].path;
	int yy = Stat(_path, &sb);
	mode_t _mode = (yy ? devFiles[i].mode : sb.st_mode);
	dev_t _dev =
	    (yy ? _makedev(devFiles[i].major, devFiles[i].minor) : sb.st_rdev);
	const char * _user = devFiles[i].user;
	const char * _group = devFiles[i].group;
	uid_t _uid = (yy ? _UIDNONE : sb.st_uid);
	gid_t _gid = (yy ? _GIDNONE : sb.st_gid);
	    
	if (_uid == _UIDNONE && _user && *_user && (pw = getpwnam(_user)))
	    _uid = pw->pw_uid;
	if (_gid == _GIDNONE && _group && *_group && (gr = getgrnam(_group)))
	    _gid = gr->gr_gid;

	fn = rpmGetPath(roto->_rootdir, _path, NULL);

	/* create node */
	xx = Mknod(fn, _mode, _dev);

	if (xx == 0 && !(_uid == _UIDNONE && _gid == _GIDNONE))
	    xx = Chown(fn, _uid, _gid);

	/*
	 * set context. (only necessary if host running selinux enabled.)
	 * fails gracefully if chcon not installed.
	 */
	if (roto->selinux) {
#ifdef	NOTYET
	    utilDo(
                    ["chcon", "--reference=/%s"% i[2], fn]
                    , raiseExc=0, shell=False)
#else
	    cmd = rpmExpand("chcon --reference=/", fn, NULL);
fprintf(stderr, "==> cmd: %s\n", cmd);
	    cmd = _free(cmd);
#endif
	}

	fn = _free(fn);
    }
#undef	_makedev

    fn = rpmGetPath(roto->_rootdir, "/dev/stdin", NULL);
    xx = Symlink("/proc/self/fd/0", fn);
    fn = _free(fn);

    fn = rpmGetPath(roto->_rootdir, "/dev/stdout", NULL);
    xx = Symlink("/proc/self/fd/1", fn);
    fn = _free(fn);

    fn = rpmGetPath(roto->_rootdir, "/dev/stderr", NULL);
    xx = Symlink("/proc/self/fd/2", fn);
    fn = _free(fn);

    /* symlink /dev/fd in the chroot for everything except RHEL4 */
    if (rpmEVRcmp(kver, "2.6.9") > 0) {
	fn = rpmGetPath(roto->_rootdir, "/dev/fd", NULL);
	xx = Symlink("/proc/self/fd", fn);
	fn = _free(fn);
    }

    (void) umask(prevMask);

    /* mount/umount */
    static const char * _devUmountCmds[] = {
	"umount -n %{__roto_basedir}/%{__roto_root}/root/dev/pts",
	"umount -n %{__roto_basedir}/%{__roto_root}/root/dev/shm",
	NULL
    };
    /* XXX FIXME: check to see whether already added hack ... */
    if (argvCount(roto->umountCmds) <= 2)
	argvAppend(&roto->umountCmds, _devUmountCmds);

    gr = getgrnam("tty");
    char b[BUFSIZ];
    xx = snprintf(b, sizeof(b), "gid=%d,mode=0620,ptmxmode=0666%s",
	(gr ? gr->gr_gid : 0),
	(rpmEVRcmp(kver, "2.6.29") >= 0 ? ",newinstance" : ""));
    addMacro(NULL, "__roto_devpts_mountopts",		NULL, b, -1);

    static const char * _devMountCmds[] = {
	"mount -n -t devpts -o %{__roto_devpts_mountopts} mock_chroot_devpts %{__roto_basedir}/%{__roto_root}/root/dev/pts",
	"mount -n -t tmpfs mock_chroot_shmfs %{__roto_basedir}/%{__roto_root}/root/dev/shm",
	NULL
    };
    /* XXX FIXME: check to see whether already added hack ... */
    if (argvCount(roto->mountCmds) <= 2)
	argvAppend(&roto->mountCmds, _devMountCmds);

    if (rpmEVRcmp(kver, "2.6.29") >= 0) {
	fn = rpmGetPath(roto->_rootdir, "/dev/ptmx", NULL);
	xx = Unlink(fn);
	xx = Symlink("pts/ptmx", fn);
	fn = _free(fn);
    }

ROTODBG((stderr, "<-- %s(%p) rc %d\n", __FUNCTION__, roto, rc));
    return rc;
}

/**
 */
static int chroot_enable_chrootuser_account(ROTO_t roto)
{
    int rc = 0;
    char * passwd = rpmGetPath(roto->_rootdir, "/etc/passwd", NULL);
#ifdef	REFERENCE
    lines = open(passwd).readlines()
    disabled = False
    newlines = []
    for l in lines:
	parts = l.strip().split(':')
	if parts[0] == roto->chrootuser and parts[1].startswith('!!'):
	    disabled = True
	    parts[1] = parts[1][2:]
	newlines.append(':'.join(parts))
    if disabled:
	f = open(passwd, "w")
	for l in newlines:
	    f.write(l+'\n')
	f.close()
#endif	/* REFERENCE */
    passwd = _free(passwd);
ROTODBG((stderr, "<-- %s(%p) rc %d\n", __FUNCTION__, roto, rc));
    return rc;
}

static int chrootAddHook(ROTO_t roto, const char * stage, void * function)
{
    int rc = 0;

#ifdef	REFERENCE
        hooks = self._hooks.get(stage, [])
        if function not in hooks:
            hooks.append(function)
            self._hooks[stage] = hooks
#else
int xx;
    char b[BUFSIZ];
    size_t nb = snprintf(b, sizeof(b), "%s: %s", stage, (const char *)function);
    b[nb] = '\0';
    
    xx = argvAdd(&roto->_hooks, b);

#endif	/* REFERENCE */

ROTODBG((stderr, "<-- %s(%p) rc %d\n", __FUNCTION__, roto, rc));
    return rc;
}

static int chroot_initPlugins(ROTO_t roto)
{
    int rc = 0;
int xx;

    roto->cache_topdir = "/var/cache/%{__roto_progname}";
    addMacro(NULL, "__roto_cache_topdir",	NULL, roto->cache_topdir, -1);

/* --- ccache_plugin */
    roto->ccache_enable = 1;
#ifdef	REFERENCE
        'ccache_opts': {
                'max_cache_size': "4G",
                'dir': "%(cache_topdir)s/%(root)s/ccache/"
        },
#endif	/* REFERENCE */
    roto->ccache_max_cache_size = "4G";
    roto->ccache_dir = "%{__roto_cache_topdir}/%{__roto_root}/ccache";

    /* macros */
    addMacro(NULL, "__roto_ccache_max_cache_size",	NULL, roto->ccache_max_cache_size, -1);
    roto->ccachePath = "%{__roto_cache_topdir}/%{__roto_root}/ccache";
    addMacro(NULL, "__roto_ccachePath",	NULL, roto->ccachePath, -1);

    /* hooks */
    xx = chrootAddHook(roto, "prebuild", "ccache -M 4G");
    xx = chrootAddHook(roto, "preinit", "CCACHE_DIR=/tmp/ccache CCACHE_UMASK=002");

    /* mount/umount */
    argvAdd(&roto->umountCmds,
	"umount -n %{__roto_basedir}/%{__roto_root}/root/tmp/ccache");
    argvAdd(&roto->mountCmds,
	"mount -n --bind %{__roto_ccachePath} %{__roto_basedir}/%{__roto_root}/root/tmp/ccache");

/* --- yum_plugin */
    roto->yum_cache_enable = 1;
#ifdef	REFERENCE
	'yum_cache_opts': {
		'max_age_days': 30,
		'max_metadata_age_days': 30,
		'dir': "%(cache_topdir)s/%(root)s/yum_cache/",
		'online': True,
	},
#endif	/* REFERENCE */
    roto->yum_cache_max_age_days = 30;
    roto->yum_cache_max_metadata_age_days = 30;
    roto->yum_cache_dir = "%{__roto_cache_topdir}/%{__roto_root}/yum_cache";
    roto->yum_cache_online = 1;

    /* macros */
    roto->yumSharedCachePath =
	"%{__roto_cache_topdir}/%{__roto_root}/yum_cache";
    addMacro(NULL, "__roto_yumSharedCachePath",	NULL, roto->yumSharedCachePath, -1);
    roto->yumCacheLockPath =
	"%{__roto_yumSharedCachePath}/yumcache.lock";
    addMacro(NULL, "__roto_yumCacheLockPath",	NULL, roto->yumCacheLockPath, -1);

    /* hooks */
    xx = chrootAddHook(roto, "preyum", "acquire yumcache lock");
    xx = chrootAddHook(roto, "postyum", "release yumcache lock");
    xx = chrootAddHook(roto, "preinit", "cleaning yum metadata");

    /* mount/umount */
    argvAdd(&roto->umountCmds,
	"umount -n %{__roto_basedir}/%{__roto_root}/root/var/cache/yum");

    argvAdd(&roto->mountCmds,
	"mount -n --bind %{__roto_yumSharedCachePath} %{__roto_basedir}/%{__roto_root}/root/var/cache/yum");

/* --- root_plugin */
    roto->root_cache_enable = 1;
#ifdef	REFERENCE
        'root_cache_opts': {
                'max_age_days': 15,
                'dir': "%(cache_topdir)s/%(root)s/root_cache/",
                'compress_program': 'pigz',
                'extension': '.gz'
        },
#endif	/* REFERENCE */
    roto->root_cache_max_age_days = 15;
    roto->root_cache_dir = "%{__roto_cache_topdir}/%{__roto_root}/yum_cache";
    roto->root_cache_compress_program = "pigz";
    roto->root_cache_extension = ".gz";

    /* macros */
    roto->rootSharedCachePath =
	"%{__roto_cache_topdir}/%{__roto_root}/yum_cache";
    addMacro(NULL, "__roto_rootSharedCachePath",	NULL, roto->rootSharedCachePath, -1);
    roto->rootCacheLockPath = 
	"%{__roto_rootSharedCachePath}/rootcache.lock";
    addMacro(NULL, "__roto_rootCacheLockPath",	NULL, roto->rootCacheLockPath, -1);

    /* hooks */
    xx = chrootAddHook(roto, "preinit", "acquire rootcache lock");
    xx = chrootAddHook(roto, "postinit", "release rootcache lock");

    /* mount/umount */

/* --- bind_mount_plugin */
    roto->bind_mount_enable = 0;
#ifdef	REFERENCE
        'bind_mount_opts': {
                'dirs': [
                # specify like this:
                # ('/host/path', '/bind/mount/path/in/chroot/' ),
                # ('/another/host/path', '/another/bind/mount/path/in/chroot/'),
                ]},
#endif	/* REFERENCE */
    roto->bind_mount_dirs = NULL;

    /* macros */
    /* hooks */
    xx = chrootAddHook(roto, "preinit", "add dirs to bind mount");
    /* mount/umount */

/* --- tmpfs_plugin */
    roto->tmpfs_enable = 0;
#ifdef	REFERENCE
        'tmpfs_opts': {
                'required_ram_mb': 900,
                'max_fs_size': None},
#endif	/* REFERENCE */
    roto->tmpfs_required_ram_mb = 900;
    roto->tmpfs_max_fs_size = 0;
    /* macros */
    /* hooks */
    /* mount/umount */

/* --- selinux_plugin */
    roto->selinux_enable = 0;
#ifdef	REFERENCE
	'selinux_opts': {}
#endif	/* REFERENCE */
    /* macros */
    /* hooks */
    /* mount/umount */

ROTODBG((stderr, "<-- %s() rc %d\n", __FUNCTION__, rc));
    return rc;
}

static int chroot_callHooks(ROTO_t roto, const char * stage)
{
    int rc = 0;
    const char * fn;
    const char * cmd;
int xx;

#ifdef	REFERENCE
        hooks = self._hooks.get(stage, [])
        for hook in hooks:
            hook()
#else
    size_t nstage = strlen(stage);
    int ac = argvCount(roto->_hooks);
    int i;

    if (roto->_hooks)
    for (i = 0; i < ac; i++) {
	const char * hook = roto->_hooks[i];
	size_t nhook = strrchr(hook, ':') - hook;

	if (nhook != nstage)
	    continue;

	if (!strncmp("preinit", hook, nstage)) {
fprintf(stderr, "*** %s: %s\n", __FUNCTION__, hook);
	    if (roto->root_cache_enable) {
	      if (roto->rootCacheLock == NULL) { /* XXX best effort only */
		fn = rpmGetPath(roto->rootCacheLockPath, NULL);
		xx = rotoLock(roto, fn, &roto->rootCacheLock);
		fn = _free(fn);
	      }
	    }
	    if (roto->ccache_enable) {
		static const char _tmp_ccache[] = "/tmp/ccache";
		rpmlog(RPMLOG_INFO, "enabled ccache\n");
		fn = rpmGetPath(roto->_rootdir, _tmp_ccache, NULL);
		xx = utilMkpath(fn, 0755, _UIDNONE, _GIDNONE);
		fn = _free(fn);
		xx = setenv("CCACHE_DIR", _tmp_ccache, 1);
		xx = setenv("CCACHE_UMASK", "002", 1);
		fn = rpmGetPath(roto->ccachePath, NULL);
		xx = utilMkpath(fn, 0755, _UIDNONE, _GIDNONE);
		fn = _free(fn);
	    }
	    if (roto->bind_mount_enable) {
	    }
	    if (roto->tmpfs_enable) {
	    }
	    continue;
	}
	if (!strncmp("prebuild", hook, nstage)) {
fprintf(stderr, "*** %s: %s\n", __FUNCTION__, hook);
	    if (roto->ccache_enable) {
		cmd = rpmExpand("ccache -M %{__roto_ccache_max_cache_size}", NULL);
fprintf(stderr, "*** prebuild cmd: %s\n", cmd);
		cmd = _free(cmd);
	    }
	    continue;
	}
	if (!strncmp("postinit", hook, nstage)) {
fprintf(stderr, "*** %s: %s\n", __FUNCTION__, hook);
	    if (roto->root_cache_enable) {
	      if (roto->rootCacheLock) {	/* XXX best effort only */
		fn = rpmGetPath(roto->rootCacheLockPath, NULL);
		xx = rotoUnlock(roto, fn, &roto->rootCacheLock);
		fn = _free(fn);
	      }
	    }
	    continue;
	}
	if (!strncmp("preyum", hook, nstage)) {
fprintf(stderr, "*** %s: %s\n", __FUNCTION__, hook);
	    if (roto->yum_cache_enable) {
	      if (roto->yumCacheLock == NULL) {	/* XXX best effort only */
		fn = rpmGetPath(roto->yumCacheLockPath, NULL);
		xx = rotoLock(roto, fn, &roto->yumCacheLock);
		fn = _free(fn);
	      }
	    }
	    continue;
	}
	if (!strncmp("postyum", hook, nstage)) {
fprintf(stderr, "*** %s: %s\n", __FUNCTION__, hook);
	    if (roto->yum_cache_enable) {
	      if (roto->yumCacheLock) {	/* XXX best effort only */
		fn = rpmGetPath(roto->yumCacheLockPath, NULL);
		xx = rotoUnlock(roto, fn, &roto->yumCacheLock);
		fn = _free(fn);
	      }
	    }
	    continue;
	}
fprintf(stderr, "*** %s: FIXME: %s\n", __FUNCTION__, hook);
    }
#endif	/* REFERENCE */

ROTODBG((stderr, "<-- %s(%s) rc %d\n", __FUNCTION__, stage, rc));
    return rc;
}

/**
 * Use yum to install packages/package groups into the chroot.
 */
static const char * chroot_yum(ROTO_t roto, const char ** av, int returnOutput)
{
    ARGV_t nav = NULL;
    const char * out = NULL;
    const char * cmd = NULL;
    const char * fn;
int xx;

#ifdef	REFERENCE
    nav = [roto->yum_path]
    ix = 0
    /* invoke yum-builddep instead of yum if av is builddep */
    if av[0] == "builddep":
	nav[0] = roto->yum_builddep_path
	ix = 1
    nav.extend(('--installroot', roto->_rootdir));
    if not roto->online:
	nav.append("-C")
    nav.extend(av[ix:])
    rpmlog(RPMLOG_DEBUG, "%s\n", nav);
    out = "";
#else
    nav = NULL;
    if (!strcmp(av[0], "builddep")) {
	xx = argvAdd(&nav, roto->yum_path);
	av++;	/* XXX watchout */
    } else {
	xx = argvAdd(&nav, roto->yum_path);
    }
    xx = argvAdd(&nav, "--installroot");
    fn = rpmGetPath(roto->_rootdir, NULL);
    xx = argvAdd(&nav, fn);
    fn = _free(fn);
    if (!F_ISSET(roto, OFFLINE))
	xx = argvAdd(&nav, "-C");
    xx = argvAppend(&nav, av);

    cmd = argvJoin(nav, ' ');
    rpmlog(RPMLOG_DEBUG, "%s\n", cmd);
#endif	/* REFERENCE */

/*  try: */

	xx = chroot_callHooks(roto, "preyum");

#ifdef	NOTYET
	out = utilDo(nav, returnOutput=returnOutput)
#else
	out = sudoCmd(cmd);
#endif

	xx = chroot_callHooks(roto, "postyum");

#ifdef	REFERENCE
    except mock.exception.Error, e:
	raise mock.exception.YumError, str(e)
#endif	/* REFERENCE */

ROTODBG((stderr, "<-- %s() out |%s|\n", __FUNCTION__, out));
    cmd = _free(cmd);
    nav = argvFree(nav);
    return out;
}

/*
 * =============
 *  'Public' API
 * =============
 */
static const char * chrootState(ROTO_t roto, const char * newState)
{
    if (newState != NULL) {
	roto->_state = newState;
	rpmlog(RPMLOG_INFO, "State Changed: %s\n", roto->_state);
    }

ROTODBG((stderr, "<-- %s(%p) state %s\n", __FUNCTION__, roto, roto->_state));
    return roto->_state;
}

static int chrootTryLockBuildRoot(ROTO_t roto)
{
    int rc = 0;
    const char * fn = rpmGetPath(roto->basedir, "/buildroot.lock", NULL);

    (void) chrootState(roto, "lock buildroot");

    if (roto->buildrootLock == NULL)	/* XXX best effort only */
	rc = rotoLock(roto, fn, &roto->buildrootLock);

ROTODBG((stderr, "<-- %s(%s) rc %d\n", __FUNCTION__, fn, rc));
    fn = _free(fn);
    return rc;
}

static int chrootUnlockBuildRoot(ROTO_t roto)
{
    int rc = 0;
    const char * fn = rpmGetPath(roto->basedir, "/buildroot.lock", NULL);

    (void) chrootState(roto, "unlock buildroot");
    
    if (roto->buildrootLock)	/* XXX best effort only */
	rc = rotoUnlock(roto, fn, &roto->buildrootLock);

ROTODBG((stderr, "<-- %s(%s) rc %d\n", __FUNCTION__, fn, rc));
    fn = _free(fn);
    return rc;
}

/**
 * Clean out chroot with extreme prejudice.
 */
static int chrootClean(ROTO_t roto)
{
    int rc = 0;
int xx;

    xx = chrootTryLockBuildRoot(roto);
    (void) chrootState(roto, "clean");
    xx = chroot_callHooks(roto, "clean");
    xx = utilOrphansKill(roto->_rootdir, SIGTERM);
    xx = chroot_unlock_and_rm_chroot(roto);
    roto->chrootWasCleaned = 1;
    xx = chrootUnlockBuildRoot(roto);
ROTODBG((stderr, "<-- %s(%p) rc %d\n", __FUNCTION__, roto, rc));
    return rc;
}

/**
 * Clean out chroot and/or cache dirs with extreme prejudice.
 */
static int chrootScrub(ROTO_t roto, const char ** scrub_opts)
{
    int rc = 0;
int xx;

    xx = chrootTryLockBuildRoot(roto);
    (void) chrootState(roto, "clean");
    xx = chroot_resetLogging(roto);
    xx = chroot_callHooks(roto, "clean");

    const char * config_name = "config_name";	/* XXX FIXME */
    const char * scrub;
    const char * fn;
    int i;
    if (scrub_opts)
    for (i = 0; (scrub = scrub_opts[i]) != NULL; i++) {
	if (!strcmp(scrub, "all")) {
	    rpmlog(RPMLOG_INFO, "scrubbing everything for %s\n", config_name);
	    xx = chroot_unlock_and_rm_chroot(roto);
	    roto->chrootWasCleaned = 1;
	    xx = utilRmtree(roto, roto->cachedir, roto->selinux);
	} else
	if (!strcmp(scrub, "chroot")) {
	    rpmlog(RPMLOG_INFO, "scrubbing chroot for %s]n", config_name);
	    xx = chroot_unlock_and_rm_chroot(roto);
	    roto->chrootWasCleaned = 1;
	} else
	if (!strcmp(scrub, "cache")) {
	    rpmlog(RPMLOG_INFO, "scrubbing cache for %s\n", config_name);
	    xx = utilRmtree(roto, roto->cachedir, roto->selinux);
	} else
	if (!strcmp(scrub, "c-cache")) {
	    rpmlog(RPMLOG_INFO, "scrubbing c-cache for %s\n", config_name);
	    fn = rpmGetPath(roto->cachedir, "/ccache", NULL);
	    xx = utilRmtree(roto, fn, roto->selinux);
	    fn = _free(fn);
	} else
	if (!strcmp(scrub, "root-cache")) {
	    rpmlog(RPMLOG_INFO, "scrubbing root-cache for %s\n", config_name);
	    fn = rpmGetPath(roto->cachedir, "/root_cache", NULL);
	    xx = utilRmtree(roto, fn, roto->selinux);
	    fn = _free(fn);
	} else
	if (!strcmp(scrub, "yum-cache")) {
	    rpmlog(RPMLOG_INFO, "scrubbing yum-cache for %s\n", config_name);
	    fn = rpmGetPath(roto->cachedir, "/yum_cache", NULL);
	    xx = utilRmtree(roto, fn, roto->selinux);
	    fn = _free(fn);
	}
    }

    xx = chrootUnlockBuildRoot(roto);
ROTODBG((stderr, "<-- %s(%p) rc %d\n", __FUNCTION__, roto, rc));
    return rc;
}

/**
 * Execute given command in root
 * XXX FIXME: lots of *args and *kargs here
 */
static const char * chrootDoChroot(ROTO_t roto, ARGV_t command, ARGV_t env, int shell, int returnOutput)
{
#ifdef	REFERENCE
    def doChroot(self, command, env="", shell=True, returnOutput=False, *args, **kargs):
        return utilDo(command, chrootPath=roto->_rootdir,
                            returnOutput=returnOutput, shell=shell, *args, **kargs )
#else
    const char * out = NULL;
    const char * _personality = "x86_64";	/* XXX FIXME */
    const char * _cwd = NULL;
    uid_t _uid = roto->chrootuid;
    gid_t _gid = roto->chrootgid;
    EXEC_t xp = execNew(_personality, roto->_rootdir, _cwd, _uid, _gid);
    xp->av = command;
    xp->env = env;
    xp->shell = shell;
    xp->returnOutput = returnOutput;

    out = utilDo(xp);

    xp = execFree(xp);
#endif	/* REFERENCE */

ROTODBG((stderr, "<-- %s(%p) out |%s|\n", __FUNCTION__, roto, out));
    return out;
}

static int chroot_makeBuildUser(ROTO_t roto)
{
    int rc = 0;
int xx;
    const char * fn;
    const char * cmd;
    const char * out = NULL;

ROTODBG((stderr, "--> %s(%p)\n", __FUNCTION__, roto));

#ifdef	NOTYET	/* XXX avoid abend for now. */
    fn = rpmGetPath(roto->_rootdir, "/usr/sbin/useradd", NULL);
    if (Stat(fn, &sb)) {
#ifdef	REFERENCE	/* XXX segfault */
	raise mock.exception.RootError, "Could not find useradd in chroot, maybe the install failed?"
#endif	/* REFERENCE */
	fn = _free(fn);
	rc = 1;		/* XXX FAIL */
	goto exit;
    }
    fn = _free(fn);
#endif

    /* safe and easy. blow away existing /builddir and completely re-create. */
    fn = rpmGetPath(roto->_rootdir, "/", roto->homedir, NULL);
    xx = utilRmtree(roto, fn, roto->selinux);
    fn = _free(fn);

#ifdef	REFERENCE
    dets = { 'uid': str(roto->chrootuid), 'gid': str(roto->chrootgid), 'user': roto->chrootuser, 'group': roto->chrootgroup, 'home': roto->homedir }

    /* ok for these two to fail */
    out = chrootDoChroot(roto, ['/usr/sbin/userdel', '-r', '-f', dets['user']], shell=False, raiseExc=False)
    out = chrootDoChroot(roto, ['/usr/sbin/groupdel', dets['group']], shell=False, raiseExc=False)
#else
    static const char * _userdel[] = {
	"/usr/sbin/userdel",
	"-r",
	"-f",
	"%(user)s",
	NULL
    };
    _userdel[3] = roto->chrootuser;
    cmd = rpmExpand("/usr/sbin/userdel -r -f ", roto->chrootuser, NULL);
fprintf(stderr, "==> cmd: %s\n", cmd);
    out = chrootDoChroot(roto, (ARGV_t)_userdel, (ARGV_t)NULL, 0, 0);
    out = _free(out);
    cmd = _free(cmd);

    static const char * _groupdel[] = {
	"/usr/sbin/groupdel",
	"%(group)s",
	NULL
    };
    _groupdel[1] = roto->chrootgroup;
    cmd = rpmExpand("/usr/sbin/groupdel ", roto->chrootgroup, NULL);
fprintf(stderr, "==> cmd: %s\n", cmd);
    out = chrootDoChroot(roto, (ARGV_t)_groupdel, (ARGV_t)NULL, 0, 0);
    out = _free(out);
    cmd = _free(cmd);
#endif	/* REFERENCE */

#ifdef	REFERENCE
    out = chrootDoChroot(roto, ['/usr/sbin/groupadd', '-g', dets['gid'], dets['group']], shell=False)
    out = chrootDoChroot(roto, roto->useradd % dets, shell=True)
#else
    char _uidbuf[32];
    xx = snprintf(_uidbuf, sizeof(_uidbuf), "%lu", (long unsigned)roto->chrootuid);
    char _gidbuf[32];
    xx = snprintf(_gidbuf, sizeof(_gidbuf), "%lu", (long unsigned)roto->chrootgid);
    static const char * _groupadd[] = {
	"/usr/sbin/groupadd",
	"-g",
	"%(gid)s",
	"%(group)s",
	NULL
    };
    _groupadd[2] = _gidbuf;
    _groupadd[3] = roto->chrootgroup;
    cmd = rpmExpand("/usr/sbin/groupadd -g ", _gidbuf, " ", roto->chrootgroup, NULL);
fprintf(stderr, "==> cmd: %s\n", cmd);
    out = chrootDoChroot(roto, (ARGV_t)_groupadd, (ARGV_t)NULL, 0, 0);
    out = _free(out);
    cmd = _free(cmd);

    static const char * _useradd[] = {
	"/usr/sbin/useradd",
	"-o",
	"-m",
	"-u",
	"%(uid)s",
	"-g",
	"%(gid)s",
	"-d",
	"%(home)s",
	"-N",
	"%(user)s",
	NULL,
    };
    _useradd[4] = _uidbuf;
    _useradd[6] = _gidbuf;
    _useradd[8] = roto->chroothome;
    _useradd[10] = roto->chrootuser;
    cmd = rpmExpand("/usr/sbin/useradd -o -m",
		" -u ", _uidbuf, " -g ", _gidbuf,
		" -d ", roto->homedir, " -N ", roto->chrootuser, NULL);
fprintf(stderr, "==> cmd: %s\n", cmd);
    out = chrootDoChroot(roto, (ARGV_t)_useradd, (ARGV_t)NULL, 1, 0);
    out = _free(out);
    cmd = _free(cmd);
#endif	/* REFERENCE */

    xx = chroot_enable_chrootuser_account(roto);

ROTODBG((stderr, "<-- %s(%p) rc %d\n", __FUNCTION__, roto, rc));
    return rc;
}

/**
 * UNPRIVILEGED:
 *   Everything in this function runs as the build user
 */
static int chroot_buildDirSetup(ROTO_t roto)
{
    char * fn;
    int rc = 0;
    int i;
int xx;

    /* create all dirs as the user who will be dropping things there. */
    xx = uidManagerBecomeUser(roto, roto->chrootuid, roto->chrootgid);

/*  try: */

	/* create dir structure */
	static const char * _subdirs[] = {
		"/RPMS", "/SRPMS", "/SOURCES", "/SPECS", "/BUILD",
		"/BUILDROOT", "/originals", NULL,
	};
	for (i = 0; _subdirs[i] != NULL; i++) {
	    const char * subdir =
		rpmGetPath(roto->_rootdir, roto->builddir, _subdirs[i], NULL);
	    xx = utilMkpath(subdir, 0755, _UIDNONE, _GIDNONE);
	    subdir = _free(subdir);
	}

	/* change ownership so we can write to build home dir */
#ifdef	REFERENCE
	for (dirpath, dirnames, filenames) in os.walk(chrootMakeChrootPath(roto, roto->homedir, NULL)):
	    for path in dirnames + filenames:
		xx = Chown(os.path.join(dirpath, path), roto->chrootuid, -1)
		xx = Chmod(os.path.join(dirpath, path), 0755)
#endif	/* REFERENCE */

	/* rpmmacros default */
	if (roto->macros) {
	    fn = rpmGetPath(roto->_rootdir, roto->homedir, "/.rpmmacros", NULL);
	    FILE * fp = fopen(fn, "w+");
	    if (fp) {
		for (i = 0; roto->macros[i]; i++) {
		    const char * b = roto->macros[i];
		    size_t nb = strlen(b);
		    size_t nw = fwrite(b, 1, nb, fp);
assert(nb == nw);
		    nw = fwrite("\n", 1, 1, fp);
		}
		xx = fclose(fp);
	    }
	    fn = _free(fn);
	}

/*  finally: */
	xx = uidManagerRestorePrivs(roto);

ROTODBG((stderr, "<-- %s() rc %d\n", __FUNCTION__, rc));
    return rc;
}

static int chroot_init(ROTO_t roto)
{
    const char * fn;
    struct stat sb;
    int i;
    int rc = 0;
int xx;

    (void) chrootState(roto, "init");

    /*
     * NOTE: removed the following stuff vs mock v0:
     *   --> /etc/ is no longer 02775 (new privs model)
     *   --> no /etc/yum.conf symlink (F7 and above)
     */

    /* create our base directory hierarchy */
    xx = utilMkpath(roto->basedir, 0755, _UIDNONE, _GIDNONE);
    xx = utilMkpath(roto->_rootdir, 0755, _UIDNONE, _GIDNONE);

    xx = uidManagerDropPrivsTemp(roto);

/*  try: */
	xx = utilMkpath(roto->resultdir, 0755, _UIDNONE, _GIDNONE);
#ifdef	REFERENCE
    except (OSError,), e:
	if e.errno == 13:
	    raise mock.exception.ResultDirNotAccessible( ResultDirNotAccessible.__doc__ % roto->resultdir )
#endif	/* REFERENCE */

    xx = uidManagerRestorePrivs(roto);

    /* lock this buildroot so we dont get stomped on. */
    xx = chrootTryLockBuildRoot(roto);

    /* create our log files. (if they havent already) */
    xx = chroot_resetLogging(roto);

    /* write out config details */
    fn = rpmExpand(roto->_rootdir, NULL);
    rpmlog(RPMLOG_DEBUG, "rootdir = %s\n", fn);	 /* XXX roto->_rootdir */
    fn = _free(fn);
    fn = rpmExpand(roto->resultdir, NULL);
    rpmlog(RPMLOG_DEBUG, "resultdir = %s\n", fn); /* XXX roto->resultdir */
    fn = _free(fn);

    /* set up plugins */
    xx = chroot_callHooks(roto, "preinit");

    /* create skeleton dirs */
    rpmlog(RPMLOG_DEBUG, "create skeleton dirs\n");
#ifdef	REFERENCE
    for item in [
                     'var/lib/rpm',
                     'var/lib/yum',
                     'var/lib/dbus',
                     'var/log',
                     'var/lock/rpm',
                     'etc/rpm',
                     'tmp',
                     'var/tmp',
                     'etc/yum.repos.d',
                     'etc/yum',
                     'proc',
                     'sys',
                    ]:
	xx = utilMkpath(chrootMakeChrootPath(roto, item, NULL), 0755, _UIDNONE, _GIDNONE);
#else
    static const char * _dirs[] = {
	"/var/cache/yum/",		/* XXX FIXME: yum_cache plugin. */
	"/var/lib/dbus/",		/* XXX FIXME: needs UUIDv4. */

	"/var/lib/rpm/",
	"/var/lib/yum/",
	"/var/lib/dbus/",
	"/var/log/",
	"/var/lock/rpm/",
	"/etc/rpm/",

	"/tmp/ccache/",			/* XXX FIXME: ccache plugin. */

	"/tmp/",
	"/var/tmp/",
	"/etc/yum.repos.d/",
	"/etc/yum/",
       	"/proc/",
	"/sys/",
	NULL,
    };
    for (i = 0; _dirs[i] != NULL; i++) {
	/* XXX figger out how to keep the pesky trailing slash again ... */
	fn = rpmGetPath(roto->_rootdir, _dirs[i], NULL);
	xx = utilMkpath(fn, 0755, _UIDNONE, _GIDNONE);
	fn = _free(fn);
    }
#endif	/* REFERENCE */

    /* touch required files */
    rpmlog(RPMLOG_DEBUG, "touch required files\n");
#ifdef	REFERENCE
    for item in [chrootMakeChrootPath(roto, "etc", "mtab", NULL),
                     chrootMakeChrootPath(roto, "etc", "fstab", NULL),
                     chrootMakeChrootPath(roto, "var", "log", "yum.log", NULL)]:
	xx = utilTouch(item);
#else
    static const char * _files[] = {
	"/etc/mtab",
	"/etc/fstab",
	"/var/log/yum.log",
	NULL,
    };
    for (i = 0; _files[i] != NULL; i++) {
	fn = rpmGetPath(roto->_rootdir, _files[i], NULL);
	xx = utilTouch(fn);
	fn = _free(fn);
    }
#ifdef	DYING
    fn = rpmGetPath(roto->_rootdir, "/etc/mtab", NULL);
    xx = utilTouch(fn);
    fn = _free(fn);

    fn = rpmGetPath(roto->_rootdir, "/etc/fstab", NULL);
    xx = utilTouch(fn);
    fn = _free(fn);

    fn = rpmGetPath(roto->_rootdir, "/var/log/yum.log", NULL);
    xx = utilTouch(fn);
    fn = _free(fn);
#endif
#endif	/* REFERENCE */

    /* write in yum.conf into chroot (always truncate and overwrite using w+) */
    rpmlog(RPMLOG_DEBUG, "configure yum\n");
    fn = rpmGetPath(roto->_rootdir, "/etc/yum/yum.conf", NULL);
    FILE * fp = fopen(fn, "w+");
    if (fp) {
	const char * b = roto->yum_conf_content;
	size_t nb = strlen(b);
	size_t nw = fwrite(b, 1, nb, fp);
assert(nb == nw);
	/* XXX final newline? */
	xx = fclose(fp);
    }
    fn = _free(fn);

    /* symlink /etc/yum.conf to /etc/yum/yum.conf (FC6 requires) */
    fn = rpmGetPath(roto->_rootdir, "/etc/yum.conf", NULL);
/*  try: */
	xx = Unlink(fn);
#ifdef	REFERENCE
    except OSError:
	pass
#endif	/* REFERENCE */
    xx = Symlink("yum/yum.conf", fn);
    fn = _free(fn);

    /* set up resolver configuration */
    if (roto->use_host_resolv) {
	fn = rpmGetPath(roto->_rootdir, "/etc/resolv.conf", NULL);
	if (!Stat(fn, &sb))
	    xx = Unlink(fn);
	xx = shutilCopy2("/etc/resolv.conf", fn);
	fn = _free(fn);

	fn = rpmGetPath(roto->_rootdir, "/etc/hosts", NULL);
	if (!Stat(fn, &sb))
	    xx = Unlink(fn);
	xx = shutilCopy2("/etc/hosts", fn);
	fn = _free(fn);
    }

#ifdef	REFERENCE
    if gotuuid:
	# Anything that tries to use libdbus inside the chroot will require this
	# FIXME - merge this code with other OS-image building code
	machine_uuid = uuid.uuid4().hex
	fn = rpmGetPath(roto->_rootdir, "/var/lib/dbus/machine-id", NULL);
	f = open(fn, 'w')
	f.write(machine_uuid)
	f.write('\n')
	f.close()
	fn = _free(fn);
#endif	/* REFERENCE */

    /* files that need doing */
#ifdef	REFERENCE
    for key in roto->files:
	p = rpmGetPath(roto->_rootdir, key, NULL);
	if not os.path.exists(p):
	    /* create directory if necessary */
	    xx = utilMkpath(os.path.dirname(p), 0755, _UIDNONE, _GIDNONE);
	    /* write file */
	    fo = open(p, 'w+')
	    fo.write(roto->files[key])
	    fo.close()
#else
    if (roto->files)
    for (i = 0; roto->files[i]; i++) {
	const char * ofn = roto->files[i];
	char * nfn = rpmGetPath(roto->_rootdir, ofn, NULL);

	/* create directory if necessary */
	int ix = strrchr(nfn, '/') - nfn;
	nfn[ix] = '\0';
	xx = utilMkpath(nfn, 0755, _UIDNONE, _GIDNONE);
	nfn[ix] = '/';
	
	/* write file */
	/* XXX FIXME: mock permits here documemts; do file copy for now. */
	xx = shutilCopy2(ofn, nfn);
	nfn = _free(nfn);
    }
#endif	/* REFERENCE */

    if (roto->internal_dev_setup)
	xx = chroot_setupDev(roto, 0);

    /* yum stuff */
    (void) chrootState(roto, "running yum");
/*  try: */
	xx = chroot_mountall(roto);

	if (roto->chrootWasCleaned) {
	    static int _returnOutput = 1;
	    const char * out = chroot_yum(roto, roto->chroot_setup_cmd, _returnOutput);
	    out = _free(out);
	}
	if (roto->chrootWasCached) {
	    static const char * _av[] = { "update", NULL };
	    static int _returnOutput = 1;
	    const char * out = chroot_yum(roto, _av, _returnOutput);
	    out = _free(out);
	}

	/* create user */
	xx = chroot_makeBuildUser(roto);
	/* XXX FIXME: returns non-zero if useradd not found. */

	/* create rpmbuild dir */
	xx = chroot_buildDirSetup(roto);

	/* set up timezone to match host */
	fn = rpmGetPath(roto->_rootdir, "/etc/localtime", NULL);
	if (!Stat(fn, &sb))
	    xx = Unlink(fn);
	xx = shutilCopy2("/etc/localtime", fn);
	fn = _free(fn);

	/* done with init */
	xx = chroot_callHooks(roto, "postinit");

/*  finally: */
	xx = chroot_umountall(roto);

    xx = chrootUnlockBuildRoot(roto);

ROTODBG((stderr, "<-- %s(%p) rc %d\n", __FUNCTION__, roto, rc));
    return rc;
}

static int chrootInit(ROTO_t roto)
{
    int rc = 0;
int xx;

/*  try: */
	xx = chroot_init(roto);
#ifdef	REFERENCE
    except (KeyboardInterrupt, Exception):
        xx = chroot_callHooks(roto, "initfailed");
	raise
#endif	/* REFERENCE */
ROTODBG((stderr, "<-- %s(%p) rc %d\n", __FUNCTION__, roto, rc));
    return rc;
}

/**
 * Call yum to install the input rpms into the chroot.
 */
static const char * chrootYumInstall(ROTO_t roto, ARGV_t rpms)
{
    const char * cmd = NULL;
    const char * out = NULL;
int xx;

    /* pass build reqs (as strings) to installer */
    cmd = argvJoin(rpms, ' ');
    rpmlog(RPMLOG_INFO, "installing package(s): %s\n", cmd);
    cmd = _free(cmd);

/*  try: */
        xx = chroot_mountall(roto);

	ARGV_t av = NULL;
	xx = argvAdd(&av, "install");
	xx = argvAppend(&av, rpms);
	cmd = argvJoin(av, ' ');
	static int _returnOutput = 1;
        out = chroot_yum(roto, av, _returnOutput);
	av = argvFree(av);
        rpmlog(RPMLOG_INFO, "%s", out);

/*  finally: */
        xx = chroot_umountall(roto);

ROTODBG((stderr, "<-- %s(%s) out |%s|\n", __FUNCTION__, cmd, out));
    cmd = _free(cmd);
    return out;
}

/**
 * Use yum to update the chroot.
 */
static int chrootYumUpdate(ROTO_t roto)
{
    const char * cmd = NULL;
    const char * out = NULL;
    int rc = 0;
int xx;

/*  try: */
	xx = chroot_mountall(roto);

	ARGV_t av = NULL;
	xx = argvAdd(&av, "update");
	cmd = argvJoin(av, ' ');
	static int _returnOutput = 1;

        out = chroot_yum(roto, av, _returnOutput);

	cmd = _free(cmd);
	av = argvFree(av);
        rpmlog(RPMLOG_INFO, "%s", out);

	out = _free(out);	/* XXX coverity #1060681 */

/*  finally: */
	xx = chroot_umountall(roto);
ROTODBG((stderr, "<-- %s(%p) rc %d\n", __FUNCTION__, roto, rc));
    return rc;
}

/**
 * Figure out deps from srpm. call yum to install them.
 */
/* XXX FIXME: multiple srpms? */
static int chrootInstallSrpmDeps(ROTO_t roto, const char * srpms)
{
    int rc = 0;
int xx;

/*  try: */
	xx = uidManagerBecomeUser(roto, 0, 0);

#ifdef	REFERENCE
	def _yum_and_check(cmd):
	    out = chroot_yum(roto, cmd, returnOutput=1)
	    for line in out.split('\n'):
		if line.lower().find('No Package found for'.lower()) != -1:
		    raise mock.exception.BuildError, "Bad build req: %s. Exiting." % line

	# first, install pre-existing deps and configured additional ones
	deps = list(self.preExistingDeps)
	for hdr in mock.util.yieldSrpmHeaders(srpms, plainRpmOk=1):
	    # get text buildreqs
	    deps.extend(mock.util.getAddtlReqs(hdr, roto->more_buildreqs))
	if deps:
	    # everything exists, okay, install them all.
	    # pass build reqs to installer
	    args = ['resolvedep'] + deps
	    _yum_and_check(args)
	    # nothing made us exit, so we continue
	    args[0] = 'install'
	    chroot_yum(roto, args, returnOutput=1)

	# install actual build dependencies
	_yum_and_check(['builddep'] + list(srpms))
#endif	/* REFERENCE */
/*  finally: */
	xx = uidManagerRestorePrivs(roto);
ROTODBG((stderr, "<-- %s(%p) rc %d\n", __FUNCTION__, roto, rc));
    return rc;
}

/**
 * UNPRIVILEGED:
 *   Everything in this function runs as the build user
 */
static char * chroot_copySrpmIntoChroot(ROTO_t roto, const char * srpm)
{
    const char * srpmFilename = basename((char *)srpm);	/* XXX FIXME */
    const char * dest = rpmGetPath(roto->_rootdir, roto->builddir, "/originals", NULL);
    int xx = shutilCopy2(srpm, dest);
    char * fn = rpmGetPath(roto->builddir, "/originals/", srpmFilename, NULL);
xx = xx;
    dest = _free(dest);
ROTODBG((stderr, "<-- %s() fn %s\n", __FUNCTION__, fn));
    return fn;
}

/**
 * Build an srpm into binary rpms, capture log.
 * UNPRIVILEGED:
 *   Everything in this function runs as the build user
 *       -> except hooks. :)
 */
static int chrootBuild(ROTO_t roto, const char * srpm, int timeout)
{
    char * dn;
    char * fn;
    char * bn;
    const char * pat;
    ARGV_t specs;
    int nspecs;
    ARGV_t srpms;
    int nsrpms;
    ARGV_t rpms;
    int nrpms;
    ARGV_t packages;
    const char * cmd;
    int rc = 0;
    int i;
int xx;

    /* tell caching we are building */
    xx = chroot_callHooks(roto, "earlyprebuild");

/*  try: */
	xx = chroot_setupDev(roto, 0);
	xx = chroot_mountall(roto);
	xx = uidManagerBecomeUser(roto, roto->chrootuid, roto->chrootgid);
	(void) chrootState(roto, "setup");

	fn = chroot_copySrpmIntoChroot(roto, srpm);
	bn = basename(fn);

	/* Completely/Permanently drop privs while running the following: */
#ifdef	REFERENCE
	xx = chrootDoChroot(roto,
                ["rpm", "-Uvh", "--nodeps", fn],
                shell=False,
                uid=roto->chrootuid,
                gid=roto->chrootgid,
                )
#else
	cmd = rpmExpand("rpm -Uvh --nodeps ", fn, NULL);
fprintf(stderr, "==> cmd: %s\n", cmd);
	cmd = _free(cmd);
#endif	/* REFERENCE */

	/* rebuild srpm/rpm from SPEC file */
	pat = rpmGetPath(roto->_rootdir, roto->builddir, "/SPECS/*.spec", NULL);
	xx = rpmGlob(pat, &nspecs, &specs);
	pat = _free(pat);

	/* XXX if there's more than one then someone is an idiot */
	if (nspecs < 1) {
	    rpmlog(RPMLOG_DEBUG, "No Spec file found in srpm: %s\n", bn);
#ifdef	REFERENCE
	    raise mock.exception.PkgError, "No Spec file found in srpm: %s" % bn
#endif	/* REFERENCE */
	    rc = 1;	/* XXX FAIL? */
	    specs = argvFree(specs);
	    fn = _free(fn);
	    goto exit;
	}

	specs = argvFree(specs);
	fn = _free(fn);		/* XXX watchout: bn has reference into fn */

	const char * chrootspec = specs[0] + strlen(roto->_rootdir);

	/* Completely/Permanently drop privs while running the following: */
#ifdef	REFERENCE
	xx = chrootDoChroot(roto,
                ["bash", "--login", "-c", 'rpmbuild -bs --target %s --nodeps %s' % (roto->rpmbuild_arch, chrootspec)],
                shell=False,
                logger=self.build_log, timeout=timeout,
                uid=roto->chrootuid,
                gid=roto->chrootgid,
                )
#else
	cmd = rpmExpand("bash --login -c '",
		"rpmbuild -bs",
			" --target ", roto->rpmbuild_arch,
			" --nodeps ", chrootspec,
		"'", NULL);
fprintf(stderr, "==> cmd: %s\n", cmd);
	cmd = _free(cmd);
#endif	/* REFERENCE */

	pat = rpmGetPath(roto->_rootdir, roto->builddir, "/SRPMS/*.rpm", NULL);
	xx = rpmGlob(pat, &nsrpms, &srpms);
	pat = _free(pat);

	if (nsrpms != 1) {
	    rpmlog(RPMLOG_WARNING, "Expected to find single rebuilt srpm, found %d.\n", nsrpms);
#ifdef	REFERENCE
	    raise mock.exception.PkgError, "Expected to find single rebuilt srpm, found %d." % len(rebuiltSrpmFile)
#endif	/* REFERENCE */
	}

	xx = chrootInstallSrpmDeps(roto, srpms[0]);

	srpms = argvFree(srpms);

	/* have to permanently drop privs or rpmbuild regains them */
	(void) chrootState(roto, "build");

	/* tell caching we are building */
	xx = chroot_callHooks(roto, "prebuild");

	/*
	 * --nodeps because rpm in the root may not be able to read rpmdb
	 * created by rpm that created it (outside of chroot)
	 */
#ifdef	REFERENCE
	xx = chrootDoChroot(roto,
                ["bash", "--login", "-c", 'rpmbuild -bb --target %s --nodeps %s' % (roto->rpmbuild_arch, chrootspec)],
                shell=False,
                logger=self.build_log, timeout=timeout,
                uid=roto->chrootuid,
                gid=roto->chrootgid,
                )
#else
	cmd = rpmExpand("bash --login -c '",
                "rpmbuild -bb",
			" --target ", roto->rpmbuild_arch,
			" --nodeps ", chrootspec,
		"'", NULL);
fprintf(stderr, "==> cmd: %s\n", cmd);
	cmd = _free(cmd);
#endif	/* REFERENCE */

	dn = rpmGetPath(roto->_rootdir, roto->builddir, NULL);

	pat = rpmGetPath(dn, "/RPMS/*.rpm", NULL);
	xx = rpmGlob(pat, &nrpms, &rpms);
	pat = _free(pat);

	pat = rpmGetPath(dn, "/SRPMS/*.rpm", NULL);
	xx = rpmGlob(pat, &nsrpms, &srpms);
	pat = _free(pat);

	dn = _free(dn);

	packages = NULL;
	argvAppend(&packages, rpms);
	rpms = argvFree(rpms);
	argvAppend(&packages, srpms);
	srpms = argvFree(srpms);

	dn = rpmGetPath(roto->resultdir, NULL);
	rpmlog(RPMLOG_DEBUG, "Copying packages to result dir]n");
	for (i = 0; packages[i]; i++)
	    xx = shutilCopy2(packages[i], dn);
	dn = _free(dn);

	packages = argvFree(packages);

/*  finally: */
	xx = uidManagerRestorePrivs(roto);
	xx = chroot_umountall(roto);

	/* tell caching we are done building */
	xx = chroot_callHooks(roto, "postbuild");

exit:
ROTODBG((stderr, "<-- %s(%p) rc %d\n", __FUNCTION__, roto, rc));
    return rc;
}

/**
 * Build an srpm, capture log.
 *
 * UNPRIVILEGED:
 *   Everything in this function runs as the build user
 *       -> except hooks. :)
 *
 */
static const char * chrootBuildsrpm(ROTO_t roto, const char * _spec,
		const char * _sources, int timeout)
{
    const char * resultSrpmFile = NULL;
    const char * pat;
    ARGV_t srpms;
    int nsrpms;
    const char * cmd;
    const char * fn;
    struct stat sb;
    char * sources;
int xx;

    /* tell caching we are building */
    xx = chroot_callHooks(roto, "earlyprebuild");

/*  try: */
	xx = chroot_mountall(roto);
	xx = uidManagerBecomeUser(roto, roto->chrootuid, roto->chrootgid);
	(void) chrootState(roto, "setup");

	/* copy spec/sources */
	fn = rpmGetPath(roto->_rootdir, roto->builddir, "/SPECS", NULL);
	xx = shutilCopy(_spec, fn);
	fn = _free(fn);

	/* Resolve any symlinks */
	fn = rpmGetPath(roto->_rootdir, roto->builddir, "/SOURCES", NULL);
	sources = Realpath(_sources, NULL);

        if (sources && (xx = Stat(sources, &sb)) == 0 && S_ISDIR(sb.st_mode)) {
	    xx = rmdir(fn);
	    xx = shutilCopyTree(roto, sources, fn);
	} else {
	    xx = shutilCopy(sources, fn);
	}
	fn = _free(fn);

	/* XXX FIXME: basename */
	fn = rpmGetPath(roto->_rootdir, roto->builddir, "/SPECS", basename((char *)_spec), NULL);
	const char * chrootspec = fn + strlen(roto->_rootdir);

	/* Completely/Permanently drop privs while running the following: */
	(void) chrootState(roto, "buildsrpm");

#ifdef	REFERENCE
	xx = chrootDoChroot(roto,
                ["bash", "--login", "-c", 'rpmbuild -bs --target %s --nodeps %s' % (roto->rpmbuild_arch, chrootspec)],
                shell=False,
                logger=self.build_log, timeout=timeout,
                uid=roto->chrootuid,
                gid=roto->chrootgid,
                )
#else
	cmd = rpmExpand("bash --login -c '",
		"rpmbuild -bs",
			" --target ", roto->rpmbuild_arch,
			" --nodeps ", chrootspec,
		"'", NULL);
fprintf(stderr, "==> cmd: %s\n", cmd);
	cmd = _free(cmd);
#endif	/* REFERENCE */
	fn = _free(fn);

	pat = rpmGetPath(roto->_rootdir, roto->builddir, "/SRPMS/*.rpm", NULL);
	xx = rpmGlob(pat, &nsrpms, &srpms);
	pat = _free(pat);

	if (nsrpms != 1) {
	    rpmlog(RPMLOG_WARNING, "Expected to find single rebuilt srpm, found %d.\n", nsrpms);
assert(0);
	}

	rpmlog(RPMLOG_DEBUG, "Copying package to result dir\n");
	fn = xstrdup(srpms[0]);
	/* XXX FIXME: basename */
	resultSrpmFile = rpmGetPath(roto->resultdir, "/", basename((char *)srpms[0]), NULL);
	xx = shutilCopy2(fn, resultSrpmFile);
	fn = _free(fn);
	srpms = argvFree(srpms);

/*  finally: */
	xx = uidManagerRestorePrivs(roto);
	xx = chroot_umountall(roto);

	/* tell caching we are done building */
	xx = chroot_callHooks(roto, "postbuild");

#ifdef	REFERENCE
	try:
	    return resultSrpmFile
	except:
	    return None
#endif	/* REFERENCE */

ROTODBG((stderr, "<-- %s(%p) resultSrpmFile %s\n", __FUNCTION__, roto, resultSrpmFile));
    return resultSrpmFile;
}

/*
 * =============
 * 'Private' API
 * =============
 */
/*==============================================================*/
#ifdef	REFERENCE
# python library imports
import tempfile
import shlex
import sys
import pwd
import os

# our imports
from mock.trace_decorator import traceLog, decorate
import mock.util

# class
class scmWorker(object):
    """Build RPMs from SCM"""
#endif	/* REFERENCE */

#ifdef	REFERENCE
    def __init__(self, log, opts):
        self.log = log
        self.log.debug("Initializing SCM integration...")

        self.method = opts['method']
        if self.method == "cvs":
            self.get = opts['cvs_get']
        elif self.method == "svn":
            self.get = opts['svn_get']
        elif self.method == "git":
            self.get = opts['git_get']
        else:
            rpmlog(RPMLOG_ERR, "Unsupported SCM method: %s\n", self.method);
            sys.exit(5)

        self.branch = None
        self.postget = None
        if 'branch' in opts:
            self.branch = opts['branch']
        if self.branch:
            if self.method == "cvs":
                self.get = self.get.replace("SCM_BRN", "-r " + self.branch)
            elif self.method == "git":
                self.postget = "git checkout " + self.branch
            elif self.method == "svn":
                self.get = self.get.replace("SCM_BRN", self.branch)
            else:
                rpmlog(RPMLOG_ERR, "Unsupported SCM method: %s\n", self.method);
                sys.exit(5)
        elif self.method == "svn":
            self.get = self.get.replace("SCM_BRN", "trunk")
        self.get = self.get.replace("SCM_BRN", "")

        if 'package' in opts:
            self.pkg = opts['package']
        else:
            rpmlog(RPMLOG_ERR, "Trying to use SCM, package not defined\n");
            sys.exit(5)
        self.get = self.get.replace("SCM_PKG", self.pkg)

        self.spec = opts['spec']
        self.spec = self.spec.replace("SCM_PKG", self.pkg)

        self.ext_src_dir = opts['ext_src_dir']
        self.write_tar = opts['write_tar']

        rpmlog(RPMLOG_DEBUG, "SCM checkout command: %s\n", self.get)
        rpmlog(RPMLOG_DEBUG, "SCM checkout post command: %s\n", str(self.postget))
#endif	/* REFERENCE */

#ifdef	REFERENCE
    def get_sources(self):
        self.wrk_dir = tempfile.mkdtemp(".mock-scm." + self.pkg)
        self.src_dir = self.wrk_dir + "/" + self.pkg
        rpmlog(RPMLOG_DEBUG, "SCM checkout directory: %s\n", self.wrk_dir);
        os.environ['CVS_RSH'] = "ssh"
        os.environ['SSH_AUTH_SOCK'] = pwd.getpwuid(os.getuid()).pw_dir + "/.ssh/auth_sock"
        utilDo(shlex.split(self.get), shell=False, cwd=self.wrk_dir)
        if self.postget:
            utilDo(shlex.split(self.postget), shell=False, cwd=self.src_dir)

        rpmlog(RPMLOG_DEBUG, "Fetched sources from SCM\n");
#endif	/* REFERENCE */

#ifdef	REFERENCE
    def prepare_sources(self):
        # import rpm after setarch
        import rpm
        rpmlog(RPMLOG_DEBUG, "Preparing SCM sources\n");

        /* Check some helper files */
        if os.path.exists(self.src_dir + "/.write_tar"):
            rpmlog(RPMLOG_DEBUG, ".write_tar detected, will write tarball on the fly\n");
            self.write_tar = True

        # Figure out the spec file
        sf = self.src_dir + "/" + self.spec
        if not os.path.exists(sf):
            sf = self.src_dir + "/" + self.spec.lower()
        if not os.path.exists(sf):
            rpmlog(RPMLOG_ERR, "Can't find spec file %s\n" % self.src_dir + "/" + self.spec)
            self.clean()
            sys.exit(5)
        self.spec = sf

        # Dig out some basic information from the spec file
        self.sources = []
        self.name = self.version = None
        ts = rpm.ts()
        rpm_spec = ts.parseSpec(self.spec)
        self.name = rpm.expandMacro("%{name}")
        self.version = rpm.expandMacro("%{version}")
        for (filename, num, flags) in rpm_spec.sources:
            self.sources.append(filename.split("/")[-1])
        rpmlog(RPMLOG_DEBUG, "Sources: %s\n", self.sources);

        # Generate a tarball from the checked out sources if needed
        if self.write_tar:
            tardir = self.name + "-" + self.version
            tarball = tardir + ".tar.gz"
            rpmlog(RPMLOG_DEBUG, "Writing " + self.src_dir + "/" + tarball + "...\n")
            if os.path.exists(self.src_dir + "/" + tarball):
                os.unlink(self.src_dir + "/" + tarball)
            open(self.src_dir + "/" + tarball, 'w').close()
            cmd = "tar czf " + self.src_dir + "/" + tarball + \
                  " --exclude " + self.src_dir + "/" + tarball + \
                  " --xform='s,^" + self.pkg + "," + tardir + ",' " + self.pkg
            utilDo(shlex.split(cmd), shell=False, cwd=self.wrk_dir)

        # Get possible external sources from an external sources directory
        for f in self.sources:
            if not os.path.exists(self.src_dir + "/" + f) and \
                   os.path.exists(self.ext_src_dir + "/" + f):
                rpmlog(RPMLOG_DEBUG, "Copying " + self.ext_src_dir + "/" + f + " to " + self.src_dir + "/" + f + "\n")
                xx = shutilCopy2(self.ext_src_dir + "/" + f, self.src_dir + "/" + f)

        rpmlog(RPMLOG_DEBUG, "Prepared sources for building src.rpm\n");

        return (self.src_dir, self.spec)
#endif	/* REFERENCE */

#ifdef	REFERENCE
    def clean(self):
        rpmlog(RPMLOG_DEBUG, "Clean SCM checkout directory\n");
        xx = utilRmtree(roto, self.wrk_dir, 0);	/* XXX roto->selinux? */
#endif	/* REFERENCE */

/*==============================================================*/
/**
 * Sets up default configuration.
 */
static int setup_default_config_opts(ROTO_t roto, void * config_opts, uid_t unprivUid)
{
    int rc = 0;
    /* global */
    roto->version = ROTO_VERSION;
    roto->basedir = "/var/lib/%{__roto_progname}"; /* root name is automatically added to this */
    roto->resultdir = "%{__roto_basedir}/%{__roto_root}/result";
    roto->cache_topdir = "/var/cache/%{__roto_progname}",
    roto->flags |= ROTO_FLAGS_CLEAN;
    roto->chroothome = "/builddir";
    roto->log_config_file = "logging.ini";
    roto->rpmbuild_timeout = 0;
    roto->chrootuid = unprivUid;

/*  try: */
#ifdef	REFERENCE
        roto->chrootgid = grp.getgrnam("mock")[2]
    except KeyError:
        #  mock group doesn't exist, must set in config file
        pass
#else
    {	struct group * gr = getgrnam("mock");
	roto->chrootgid = (gr ? gr->gr_gid : 0);
    }
#endif	/* REFERENCE */
    roto->build_log_fmt_name = "unadorned";
    roto->root_log_fmt_name  = "detailed";
    roto->state_log_fmt_name = "state";
    roto->online = 1;

    roto->internal_dev_setup = 1;
    roto->internal_setarch = 1;

    /*
     * cleanup_on_* only take effect for separate --resultdir
     * config_opts provides fine-grained control. cmdline only has big hammer
     */
    roto->cleanup_on_success = 1;
    roto->cleanup_on_failure = 1;

    roto->createrepo_on_rpms = 0;
    roto->createrepo_command = "/usr/bin/createrepo -d -q -x *.src.rpm"; /* default command */
#ifdef	REFERENCE
    /*
     * (global) plugins and plugin configs.
     * ordering constraings: tmpfs must be first.
     *    root_cache next.
     *    after that, any plugins that must create dirs (yum_cache)
     *    any plugins without preinit hooks should be last.
     */
    roto->plugins = ['tmpfs', 'root_cache', 'yum_cache', 'bind_mount', 'ccache', 'selinux']
#else
    roto->plugins = _roto_plugins;
#endif	/* REFERENCE */
    roto->plugin_dir = PKGPYTHONDIR "/plugins";
#ifdef	REFERENCE
    roto->plugin_conf = {
            'ccache_enable': True,
            'ccache_opts': {
                'max_cache_size': "4G",
                'dir': "%(cache_topdir)s/%(root)s/ccache/"},
            'yum_cache_enable': True,
            'yum_cache_opts': {
                'max_age_days': 30,
                'max_metadata_age_days': 30,
                'dir': "%(cache_topdir)s/%(root)s/yum_cache/",
                'online': True,},
            'root_cache_enable': True,
            'root_cache_opts': {
                'max_age_days': 15,
                'dir': "%(cache_topdir)s/%(root)s/root_cache/",
                'compress_program': 'pigz',
                'extension': '.gz'},
            'bind_mount_enable': True,
            'bind_mount_opts': {'dirs': [
                # specify like this:
                # ('/host/path', '/bind/mount/path/in/chroot/' ),
                # ('/another/host/path', '/another/bind/mount/path/in/chroot/'),
                ]},
            'tmpfs_enable': False,
            'tmpfs_opts': {
                'required_ram_mb': 900,
                'max_fs_size': None},
            'selinux_enable': True,
            'selinux_opts': {},
            }

    runtime_plugins = [runtime_plugin
                       for (runtime_plugin, _)
                       in [os.path.splitext(os.path.basename(tmp_path))
                           for tmp_path
                           in glob(roto->plugin_dir + "/*.py")]
                       if runtime_plugin not in roto->plugins]
    for runtime_plugin in sorted(runtime_plugins):
        roto->plugins.append(runtime_plugin)
        roto->plugin_conf[runtime_plugin + "_enable"] = False
        roto->plugin_conf[runtime_plugin + "_opts"] = {}
#else
    roto->plugin_conf = _roto_plugin_conf;
#endif	/* REFERENCE */

    /* SCM defaults */
    roto->scm_opts = __roto_scm_opts;

    /* dependent on guest OS */
    roto->useradd = 
	"/usr/sbin/useradd -o -m -u %{__roto_chrootuid} -g %{__roto_chrootgid}  -d %{__roto_chroot_home} -N %{__roto_chrootuser}",
    roto->use_host_resolv = 1;
    roto->chroot_setup_cmd = _roto_chroot_setup_cmd;
    roto->target_arch = "x86_64";
    roto->rpmbuild_arch = NULL; /* <-- NULL means set automatically from target_arch */
    roto->yum_path = "/usr/bin/yum";
    roto->yum_builddep_path = "/usr/bin/yum-builddep";
    roto->yum_conf_content = _roto_yum_conf_content;
    roto->more_buildreqs = __roto_more_buildreqs;	/* XXX NULL? */
    roto->files = __roto_files;	/* XXX NULL? */
    roto->macros = __roto_macros;	/* XXX NULL? */

ROTODBG((stderr, "<-- %s() rc %d\n", __FUNCTION__, rc));
    return rc;
}

/**
 * Takes processed cmdline args and sets config options.
 */
static int set_config_opts_per_cmdline(ROTO_t roto, void * config_opts, void * options, void * args)
{
    int rc = 0;
#ifdef	REFERENCE
    # do some other options and stuff
    if options.arch:
        roto->target_arch = options.arch
    if options.rpmbuild_arch:
        roto->rpmbuild_arch = options.rpmbuild_arch
    elif (roto->rpmbuild_arch == NULL)
        roto->rpmbuild_arch = roto->target_arch;

    if not options.clean:
        roto->clean = options.clean

    for option in options.rpmwith:
        options.rpmmacros.append("_with_%s --with-%s" %
                                 (option.replace("-", "_"), option))

    for option in options.rpmwithout:
        options.rpmmacros.append("_without_%s --without-%s" %
                                 (option.replace("-", "_"), option))

    for macro in options.rpmmacros:
        try:
            k, v = macro.split(" ", 1)
            if not k.startswith('%'):
                k = '%%%s' % k
            config_opts['macros'].update({k: v})
        except:
            raise mock.exception.BadCmdline(
                "Bad option for '--define' (%s).  Use --define 'macro expr'"
                % macro)

    if options.resultdir:
        roto->resultdir = os.path.expanduser(options.resultdir)
    if options.uniqueext:
        roto->uniqueext = options.uniqueext
    if options.rpmbuild_timeout is not None:
        roto->rpmbuild_timeout = options.rpmbuild_timeout

    for i in options.disabled_plugins:
        if i not in roto->plugins:
            raise mock.exception.BadCmdline(
                "Bad option for '--disable-plugin=%s'. Expecting one of: %s"
                % (i, roto->plugins))
        roto->plugin_conf['%s_enable' % i] = False
    for i in options.enabled_plugins:
        if i not in roto->plugins:
            raise mock.exception.BadCmdline(
                "Bad option for '--enable-plugin=%s'. Expecting one of: %s"
                % (i, roto->plugins))
        roto->plugin_conf['%s_enable' % i] = True

    if options.flags in ("rebuild",) and ac > 1 and not options.resultdir:
        raise mock.exception.BadCmdline(
            "Must specify --resultdir when building multiple RPMS.")

    if options.cleanup_after == False:
        roto->cleanup_on_success = 0;
        roto->cleanup_on_failure = 0;

    if options.cleanup_after == True:
        roto->cleanup_on_success = 1;
        roto->cleanup_on_failure = 1;
    /* cannot","can not","can't cleanup unless resultdir is separate from the root dir  */
    rootdir = os.path.join(roto->basedir, roto->_root) 
    if mock.util.is_in_dir(roto->resultdir % config_opts, rootdir): 
        roto->cleanup_on_success = 0;
        roto->cleanup_on_failure = 0;

    roto->online = options.online

    if (F_ISSET(roto, SCM_ENABLE)) {
        for option in options.scm_opts:
            try:
                k, v = option.split("=", 1)
                roto->scm_opts.update({k: v})
            except:
                raise mock.exception.BadCmdline(
                "Bad option for '--scm-option' (%s).  Use --scm-option 'key=value'"
                % option)
    }
#endif	/* REFERENCE */
ROTODBG((stderr, "<-- %s() rc %d\n", __FUNCTION__, rc));
    return rc;
}

/**
 */
static int check_arch_combination(ROTO_t roto, const char * target_arch, void * config_opts)
{
    int rc = 0;

/*  try: */
#ifdef	REFERENCE
        legal = config_opts['legal_host_arches']
    except KeyError:
        return
    host_arch = os.uname()[-1]
    if host_arch not in legal:
        raise mock.exception.InvalidArchitecture(
            "Cannot build target %s on arch %s" % (target_arch, host_arch))
#endif	/* REFERENCE */

ROTODBG((stderr, "<-- %s() rc %d\n", __FUNCTION__, rc));
    return rc;
}

/**
 * Rebuilds a list of srpms using provided chroot.
 */
static int do_rebuild(ROTO_t roto, const char ** srpms)
{
    int rc = 0;
    int ac = argvCount(srpms);
    time_t start = time(NULL);
    time_t elapsed;
int xx;

ROTODBG((stderr, "--> %s(%p) srpms %p[%d]\n", __FUNCTION__, roto, srpms, ac));
argvPrint(__FUNCTION__, srpms, NULL);

    if (ac < 1) {
        rpmlog(RPMLOG_CRIT, "No package specified to rebuild command.\n");
	rc = 50;
	goto exit;
    }

#ifdef	REFERENCE
    # check that everything is kosher. Raises exception on error
    for hdr in mock.util.yieldSrpmHeaders(srpms):
        pass
#endif	/* REFERENCE */

/*  try: */
	int i;
	for (i = 0; i < ac; i++) {
	    const char * srpm = srpms[i];
	    const char * _state = chrootState(roto, NULL);
	    start = time(NULL);
            rpmlog(RPMLOG_INFO, "Start(%s)  Config(%s)\n", srpm, roto->sharedRootName);
            if (F_ISSET(roto, CLEAN) && _state && strcmp(_state, "clean") && !F_ISSET(roto, SCM_ENABLE))
                xx = chrootClean(roto);
            xx = chrootInit(roto);
            xx = chrootBuild(roto, srpm, roto->rpmbuild_timeout);
            elapsed = time(NULL) - start;
            rpmlog(RPMLOG_INFO, "Done(%s) Config(%s) %d minutes %d seconds\n",
                srpm, roto->chroot_name, elapsed/60, elapsed%60);
            rpmlog(RPMLOG_INFO, "Results and/or logs in: %s\n", roto->resultdir);
	}

        if (roto->cleanup_on_success) {
            rpmlog(RPMLOG_INFO, "Cleaning up build root ('clean_on_success=True')\n");
            xx = chrootClean(roto);
	}

        if (roto->createrepo_on_rpms) {
            rpmlog(RPMLOG_INFO, "Running createrepo on binary rpms in resultdir\n");
            xx = uidManagerDropPrivsTemp(roto);
#ifdef	REFERENCE
	    /* XXX argvSplit */
            cmd = roto->createrepo_command.split()
            cmd.append(roto->resultdir)
            utilDo(cmd)
#endif	/* REFERENCE */
            xx = uidManagerRestorePrivs(roto);
	}
            
#ifdef	REFERENCE
    except (Exception, KeyboardInterrupt):
        elapsed = time(NULL) - start;
        rpmlog(RPMLOG_ERR, "Exception(%s) Config(%s) %d minutes %d seconds\n",
            srpm, roto->sharedRootName, elapsed/60, elapsed%60);
        rpmlog(RPMLOG_INFO, "Results and/or logs in: %s\n", roto->resultdir);
        if (roto->cleanup_on_failure) {
            rpmlog(RPMLOG_INFO, "Cleaning up build root ('clean_on_failure=True')\n");
            xx = chrootClean(roto);
	}
        raise
#endif	/* REFERENCE */

exit:
ROTODBG((stderr, "<-- %s(%p) rc %d\n", __FUNCTION__, roto, rc));
    return rc;
}

static const char * do_buildsrpm(ROTO_t roto)
{
    const char * srpm = NULL;
    const char * dn;
int xx;
    time_t start = time(NULL);
    time_t elapsed;

ROTODBG((stderr, "--> %s(%p) spec %s sources %s\n", __FUNCTION__, roto, roto->spec, roto->sources));

/*  try: */
	/*
         * TODO: validate spec path (exists)
         * TODO: validate SOURCES path (exists)
	 */
	const char * _state = chrootState(roto, NULL);
	/* XXX FIXME: basename */
        rpmlog(RPMLOG_INFO, "Start(%s)  Config(%s)\n", basename((char *)roto->spec), roto->sharedRootName);
	if (F_ISSET(roto, CLEAN) && _state && strcmp(_state, "clean"))
	    xx = chrootClean(roto);
	xx = chrootInit(roto);

	srpm = chrootBuildsrpm(roto, roto->spec, roto->sources, roto->rpmbuild_timeout);
	elapsed = time(NULL) - start;

	/* XXX FIXME: basename */
        rpmlog(RPMLOG_INFO, "Done(%s) Config(%s) %d minutes %d seconds\n",
            basename((char *)roto->spec), roto->chroot_name, elapsed/60, elapsed%60);
	dn = rpmExpand(roto->resultdir, NULL);
        rpmlog(RPMLOG_INFO, "Results and/or logs in: %s\n", dn);
	dn = _free(dn);

	if (roto->cleanup_on_success) {
	    rpmlog(RPMLOG_INFO, "Cleaning up build root ('clean_on_success=True')\n");
	    xx = chrootClean(roto);
	}
#ifdef	NOTYET
    except (Exception, KeyboardInterrupt):
        elapsed = time(NULL) - start;
        log.error(RPMLOG_ERR, "Exception(%s) Config(%s) %d minutes %d seconds\n",
            basename(roto->spec), roto->sharedRootName, elapsed/60, elapsed%60);
	dn = rpmExpand(roto->resultdir, NULL);
        rpmlog(RPMLOG_INFO, "Results and/or logs in: %s\n", dn);
	dn = _free(dn);
        if (roto->cleanup_on_failure) {
            rpmlog(RPMLOG_INFO, "Cleaning up build root ('clean_on_failure=True')\n")
            xx = chrootClean(roto);
	}
        raise
#endif	/* NOTYET */

ROTODBG((stderr, "<-- %s(%p) srpm %s\n", __FUNCTION__, roto, srpm));
    return srpm;
}

/**
 * Verify mock was started correctly (either by sudo or consolehelper).
 * If we're root due to sudo or consolehelper, we're ok.
 * If not raise an exception and bail.
 */
static int rootcheck(void)
{
    int rc = 0;
#ifdef	REFERENCE
    if os.getuid() == 0 and not (os.environ.get("SUDO_UID") or os.environ.get("USERHELPER_UID")):
        raise RuntimeError, "mock will not run from the root account (needs an unprivileged uid so it can drop privs)"
#endif	/* REFERENCE */
ROTODBG((stderr, "<-- %s() rc %d\n", __FUNCTION__, rc));
    return rc;
}

/**
 * Verify that the user running mock is part of the mock group.
 */
static int groupcheck(void)
{
    int rc = 0;
#ifdef	REFERENCE
    inmockgrp = False
    members = []
    for g in os.getgroups():
        name = grp.getgrgid(g).gr_name
        members.append(name)
        if name == "mock":
            inmockgrp = True
            break
    if not inmockgrp:
        raise RuntimeError, "Must be member of 'mock' group to run mock! (%s)" % members
#endif	/* REFERENCE */
ROTODBG((stderr, "<-- %s() rc %d\n", __FUNCTION__, rc));
    return rc;
}

/*==============================================================*/

/**
 * Unreference a roto interpreter instance.
 * @param roto	roto interpreter
 * @return		NULL on last dereference
 */
/*@unused@*/ /*@null@*/
ROTO_t rotoUnlink (/*@killref@*/ /*@only@*/ /*@null@*/ ROTO_t roto)
	/*@modifies roto @*/;
#define	rotoUnlink(_roto)	\
    ((ROTO_t)rpmioUnlinkPoolItem((rpmioItem)(_roto), __FUNCTION__, __FILE__, __LINE__))

/**
 * Reference a roto interpreter instance.
 * @param roto	roto interpreter
 * @return		new roto interpreter reference
 */
/*@unused@*/ /*@newref@*/ /*@null@*/
ROTO_t rotoLink (/*@null@*/ ROTO_t roto)
	/*@modifies roto @*/;
#define	rotoLink(_roto)	\
    ((ROTO_t)rpmioLinkPoolItem((rpmioItem)(_roto), __FUNCTION__, __FILE__, __LINE__))

/**
 * Destroy a roto interpreter.
 * @param roto		roto interpreter
 * @return		NULL on last dereference
 */
/*@null@*/
ROTO_t rotoFree(/*@killref@*/ /*@null@*/ROTO_t roto)
	/*@globals fileSystem @*/
	/*@modifies roto, fileSystem @*/;
#define	rotoFree(_roto)	\
    ((ROTO_t)rpmioFreePoolItem((rpmioItem)(_roto), __FUNCTION__, __FILE__, __LINE__))

static void rotoFini(void * _roto)
        /*@globals fileSystem @*/
        /*@modifies *_roto, fileSystem @*/
{
    ROTO_t roto = (ROTO_t) _roto;

    (void)rpmiobFree(roto->iob);
    roto->iob = NULL;
}

/*@unchecked@*/ /*@only@*/ /*@null@*/
rpmioPool _rotoPool;

static ROTO_t rotoGetPool(/*@null@*/ rpmioPool pool)
        /*@globals _rotoPool, fileSystem @*/
        /*@modifies pool, _rotoPool, fileSystem @*/
{
    ROTO_t roto;

    if (_rotoPool == NULL) {
        _rotoPool = rpmioNewPool("roto", sizeof(*roto), -1, _roto_debug,
                        NULL, NULL, rotoFini);
        pool = _rotoPool;
    }
    roto = (ROTO_t) rpmioGetPool(pool, sizeof(*roto));
    memset(((char *)roto)+sizeof(roto->_item), 0, sizeof(*roto)-sizeof(roto->_item));
    return roto;
}

/**
 * Create and load a roto interpreter.
 * @param *av		roto interpreter args (or NULL)
 * @param flags		roto interpreter flags ((1<<31) == use global interpreter)
 * @return		new roto interpreter
 */
/*@newref@*/ /*@null@*/ static
ROTO_t rotoNew(char ** av, uint32_t flags)
        /*@globals fileSystem, internalState @*/
        /*@modifies fileSystem, internalState @*/
{
    ROTO_t roto = rotoGetPool(_rotoPool);
    const char * fn = (av ? av[0] : NULL);
    int typ = flags;
    static int oneshot = 0;

    if (!oneshot) {
	fn = fn;
	typ = typ;
	oneshot++;
    }

ROTODBG((stderr, "==> %s(%p, %d) roto %p\n", __FUNCTION__, av, flags, roto));

    roto->iob = rpmiobNew(0);
    return rotoLink(roto);
}

/*==============================================================*/

static struct poptOption optionsTable[] = {
 { "init", '\0', POPT_BIT_SET,		&_roto.flags, ROTO_FLAGS_INIT,
	N_("initialize the chroot, do not build anything"), NULL },
 { "clean", '\0', POPT_BIT_SET,		&_roto.flags, ROTO_FLAGS_CLEAN,
	N_("completely remove the specified chroot"), NULL },
/* XXX --scrub=[all,chroot,cache,root-cache,c-cache,yum-cache]} */
 { "scrub", '\0', POPT_BIT_SET,		&_roto.flags, ROTO_FLAGS_SCRUB,
	N_("completely remove the specified chroot or cache dir or all of the chroot and cache"), NULL },
/* XXX [--rebuild] /path/to/srpm(s) */
 { "rebuild", '\0', POPT_BIT_SET,	&_roto.flags, ROTO_FLAGS_REBUILD,
	N_("rebuild the specified SRPM(s)"), NULL },
/* XXX --buildsrpm {--spec /path/to/spec --sources /path/to/src|--scm-enable [--scm-option key=value]} */
 { "buildsrpm", '\0', POPT_BIT_SET,	&_roto.flags, ROTO_FLAGS_BUILDSRPM,
	N_("Build a SRPM from spec (--spec ...) and sources (--sources ...) or from SCM"), NULL },
/* XXX {--shell|--chroot} <cmd> */
 { "shell", '\0', POPT_BIT_SET,		&_roto.flags, ROTO_FLAGS_SHELL,
	N_("run the specified command interactively within the chroot. Default command: /bin/sh"), NULL },
 { "chroot", '\0', POPT_BIT_SET,	&_roto.flags, ROTO_FLAGS_CHROOT,
	N_("run the specified command noninteractively within the chroot."), NULL },
/* XXX --installdeps {SRPM|RPM} */
 { "installdeps", '\0', POPT_BIT_SET,	&_roto.flags, ROTO_FLAGS_INSTALLDEPS,
	N_("install build dependencies for a specified SRPM"), NULL },
/* XXX --install PACKAGE */
 { "install", '\0', POPT_BIT_SET,	&_roto.flags, ROTO_FLAGS_INSTALL,
	N_("install packages using yum"), NULL },
 { "update", '\0', POPT_BIT_SET,	&_roto.flags, ROTO_FLAGS_UPDATE,
	N_("update installed packages using yum"), NULL },
 { "orphanskill", '\0', POPT_BIT_SET,	&_roto.flags, ROTO_FLAGS_ORPHANSKILL,
	N_("Kill all processes using specified buildroot."), NULL },
/* XXX --copyin path [..path] destination */
 { "copyin", '\0', POPT_BIT_SET,	&_roto.flags, ROTO_FLAGS_COPYIN,
	N_("Copy file(s) into the specified chroot"), NULL },
/* XXX --copyout path [..path] destination */
 { "copyout", '\0', POPT_BIT_SET,	&_roto.flags, ROTO_FLAGS_COPYOUT,
	N_("Copy file(s) from the specified chroot"), NULL },

 { "offline", '\0', POPT_BIT_SET,	&_roto.flags, ROTO_FLAGS_OFFLINE,
	N_("activate 'offline' mode."), NULL },
 { "no-clean", '\0', POPT_BIT_SET,	&_roto.flags, ROTO_FLAGS_NO_CLEAN,
	N_("do not clean chroot before building"), NULL },
 { "cleanup-after", '\0', POPT_BIT_SET,	&_roto.flags, ROTO_FLAGS_NO_CLEAN,
	N_("Clean chroot after building. Use with --resultdir. Only active for 'rebuild'."), NULL },

 { "arch", '\0', POPT_ARG_STRING,	&_roto.arch, 0,
	N_("Sets kernel personality()."), N_("ARCH") },
 { "target", '\0', POPT_ARG_STRING,	&_roto.rpmbuild_arch, 0,
	N_("passed to rpmbuild as --target"), N_("ARCH") },
 { "define", 'D', POPT_ARG_ARGV,	&_roto.rpmmacros, 0,
	N_("define an rpm macro (may be used more than once)"), N_("MACRO EXPR") },

 { "with", '\0', POPT_ARG_ARGV,		&_roto.rpmwith, 0,
	N_("enable configure option for build (may be used more than once)"), N_("OPTION") },
 { "without", '\0', POPT_ARG_ARGV,	&_roto.rpmwithout, 0,
	N_("disable configure option for build (may be used more than once)"), N_("OPTION") },

 { "resultdir", '\0', POPT_ARG_STRING,	&_roto.resultdir, 0,
	N_("path for resulting files to be put"), N_("DIR") },
 { "uniqueext", '\0', POPT_ARG_STRING,	&_roto.uniqueext, 0,
	N_("Arbitrary, unique extension to append to buildroot directory name"), N_("EXT") },
 { "configdir", '\0', POPT_ARG_STRING,	&_roto.configdir, 0,
	N_("Change where config files are found"), N_("DIR") },
 { "rpmbuild_timeout", '\0', POPT_ARG_INT,	&_roto.rpmbuild_timeout, 0,
	N_("Fail build if rpmbuild takes longer than 'timeout' seconds"), N_("SECONDS") },

 { "unpriv", '\0', POPT_BIT_SET,	&_roto.flags, ROTO_FLAGS_UNPRIV,
	N_("Drop privileges before running command when using --chroot"), NULL },
 { "cwd", '\0', POPT_ARG_STRING,	&_roto.cwd, 0,
	N_("Change to the specified directory (relative to the chroot) before running command when using --chroot"), N_("DIR") },

 { "spec", '\0', POPT_ARG_STRING,	&_roto.spec, 0,
	N_("Specifies spec file to use to build an SRPM (used only with --buildsrpm)"), N_("SPEC") },
 { "sources", '\0', POPT_ARG_STRING,	&_roto.sources, 0,
	N_("Specifies sources (either a single file or a directory of files) to use to build an SRPM (used only with --buildsrpm)"), N_("SOURCES") },

 { "trace", '\0', POPT_BIT_SET,		&_roto.flags, ROTO_FLAGS_TRACE,
	N_("Enable internal mock tracing output."), NULL },

 { "enable-plugin", '\0', POPT_ARG_ARGV,	&_roto.enabled_plugins, 0,
	N_("Enable plugin."), NULL },
 { "disable-plugin", '\0', POPT_ARG_ARGV,	&_roto.disabled_plugins, 0,
	N_("Disable plugin."), NULL },

 { "print-root-path", '\0', POPT_BIT_SET,	&_roto.flags, ROTO_FLAGS_PRINT_ROOT_PATH,
	N_("print path to chroot root"), NULL },

 { "scm-enable", '\0', POPT_BIT_SET,	&_roto.flags, ROTO_FLAGS_SCM_ENABLE,
	N_("build from SCM repository"), NULL },
 { "scm-option", '\0', POPT_ARG_ARGV,	&_roto.scm_opts, 0,
	N_("define an SCM option (may be used more than once)"), N_("OPTION") },

 { "nounshare", '\0', POPT_BIT_SET,	&_roto.flags, ROTO_FLAGS_NOUNSHARE,
	N_("disable unshare(2)"), NULL },

 { NULL, '\0', POPT_ARG_INCLUDE_TABLE, rpmioAllPoptTable, 0,
	N_("Common options for all rpmio executables:"),
	NULL },

  POPT_AUTOALIAS
  POPT_AUTOHELP

  { NULL, (char)-1, POPT_ARG_INCLUDE_TABLE, NULL, 0,
        "\
usage:\n\
       roto [options] {--init|--clean|--scrub=[all,chroot,cache,root-cache,c-cache,yum-cache]}\n\
       roto [options] [--rebuild] /path/to/srpm(s)\n\
       roto [options] --buildsrpm {--spec /path/to/spec --sources /path/to/src|--scm-enable [--scm-option key=value]}\n\
       roto [options] {--shell|--chroot} <cmd>\n\
       roto [options] --installdeps {SRPM|RPM}\n\
       roto [options] --install PACKAGE\n\
       roto [options] --copyin path [..path] destination\n\
       roto [options] --copyout path [..path] destination\n\
       roto [options] --scm-enable [--scm-option key=value]\n\
", NULL },
  POPT_TABLEEND
};

int main(int argc, char * argv[])
{
    poptContext optCon = rpmioInit(argc, argv, optionsTable);
    ARGV_t av = poptGetArgs(optCon);
    int ac = argvCount(av);
#ifdef	NOTYET
    ROTO_t roto = rotoNew((char **)av, 0);
#else
    ROTO_t roto = &_roto;
#endif
    char b[BUFSIZ];
    uid_t unprivUid = getuid();
    gid_t unprivGid = getgid();
    const char * username = NULL;
    const char * srpm = NULL;
    struct stat sb;
    const char * cmd = NULL;
    const char * out = NULL;
    char * t;
    int ec = 1;		/* assume failure */
int xx;

_rpmio_debug = -1;

    /* initial sanity check for correct invocation method */
    rootcheck();

    /*
     * drop unprivileged to parse args, etc.
     * uidManager saves current real uid/gid which are unprivileged (callers)
     * due to suid helper, our current effective uid is 0
     * also supports being run by sudo
     *
     * setuid wrapper has real uid = unpriv,  effective uid = 0
     * sudo sets real/effective = 0, and sets env vars
     * setuid wrapper clears environment, so there wont be any conflict between these two
     */

    /* sudo */
    if ((t = getenv("SUDO_UID")) != NULL) {
	unprivUid = atol(t);
	username = ((t = getenv("SUDO_USER")) ? xstrdup(t) : NULL);
#ifdef	REFERENCE
	groups = [ g[2] for g in grp.getgrall() if username in g[3]]
	os.setgroups(groups)
#endif	/* REFERENCE */
	if ((t = getenv("SUDO_GID")) != NULL)
	    unprivGid = atol(t);
    }

    /* consolehelper */
    if ((t = getenv("USERHELPER_UID")) != NULL) {
	struct passwd * pw;
	unprivUid = atol(t);
	if ((pw = getpwuid(unprivUid)) != NULL) {
	    username = (pw->pw_name ? xstrdup(pw->pw_name) : NULL);
#ifdef	REFERENCE
	    groups = [ g[2] for g in grp.getgrall() if username in g[3]]
	    os.setgroups(groups)
#endif	/* REFERENCE */
	    unprivGid = pw->pw_gid;
	}
    }

#ifdef	REFERENCE
    uidManager = mock.uid.uidManager(unprivUid, unprivGid)
#else
    /* XXX borrowed from uidManager __init__ */
    roto->unprivUid = unprivUid;
    roto->unprivGid = unprivGid;
#endif	/* REFERENCE */

    /* go unpriv only when root to make --help etc work for non-mock users */
    if (geteuid() == 0)
        xx = uidManager_becomeUser(roto, unprivUid, unprivGid);

    /* verify that our unprivileged uid is in the mock group */
    groupcheck();

    /* defaults */
    void * config_opts = NULL;
    setup_default_config_opts(roto, config_opts, unprivUid);
#ifdef	REFERENCE
    (options, args) = command_parse(config_opts)

    if options.printrootpath:
        options.verbose = 0

    # config path -- can be overridden on cmdline
    config_path = MOCKCONFDIR
    if options.configdir:
        config_path = options.configdir

    /* array to save config paths */
    roto->configs = NULL;

    /* Read in the config files: default, and then user specified */
    for cfg in ( os.path.join(config_path, 'site-defaults.cfg'), '%s/%s.cfg' % (config_path, options.chroot)):
        if os.path.exists(cfg):
            roto->configs.append(cfg)
            execfile(cfg)
        else:
            rpmlog(RPMLOG_ERR, "Could not find required config file: %s\n", cfg);
            if options.chroot == "default": rpmlog(RPMLOG_ERR, "  Did you forget to specify the chroot to use with '-r'?\n");
            sys.exit(1)

    /* configure logging */
    roto->chroot_name = options.chroot
    log_ini = os.path.join(config_path, roto->log_config_file)
    if not os.path.exists(log_ini):
        rpmlog(RPMLOG_ERR, "Could not find required logging config file: %s\n", log_ini);
        sys.exit(50)
/*  try: */
        if not os.path.exists(log_ini): raise IOError, "Could not find log config file %s" % log_ini
        log_cfg = ConfigParser.ConfigParser()
        logging.config.fileConfig(log_ini)
        log_cfg.read(log_ini)
    except (IOError, OSError, ConfigParser.NoSectionError), exc:
        rpmlog(RPMLOG_ERR, "Log config file(%s) not correctly configured: %s\n", log_ini, exc);
        sys.exit(50)

/*  try: */
        /* set up logging format strings */
        config_opts['build_log_fmt_str'] = log_cfg.get("formatter_%s" % roto->build_log_fmt_name, "format", raw=1)
        config_opts['root_log_fmt_str'] = log_cfg.get("formatter_%s" % roto->root_log_fmt_name, "format", raw=1)
        config_opts['state_log_fmt_str'] = log_cfg.get("formatter_%s" % roto->state_log_fmt_name, "format", raw=1)
    except ConfigParser.NoSectionError, exc:
        rpmlog(RPMLOG_ERR, "Log config file (%s) missing required section: %s\n", log_ini, exc);
        sys.exit(50)

    /* set logging verbosity */
    if options.verbose == 0:
        log.handlers[0].setLevel(logging.WARNING)
        logging.getLogger("mock.Root.state").handlers[0].setLevel(logging.WARNING)
    elif options.verbose == 1:
        log.handlers[0].setLevel(logging.INFO)
    elif options.verbose == 2:
        log.handlers[0].setLevel(logging.DEBUG)
        logging.getLogger("mock.Root.build").propagate = 1
        logging.getLogger("mock").propagate = 1

    /* enable tracing if requested */
    logging.getLogger("trace").propagate=0
    if options.trace:
        logging.getLogger("trace").propagate=1
#endif	/* REFERENCE */

    /* cmdline options override config options */
    set_config_opts_per_cmdline(roto, NULL, NULL, NULL);

    /* verify that we're not trying to build an arch that we can't */
    check_arch_combination(roto, roto->rpmbuild_arch, NULL);

    /* default /etc/hosts contents */
#ifdef	REFERENCE
    if not roto->use_host_resolv and not roto->files.has_key('etc/hosts'):
        roto->files['etc/hosts'] = "\
127.0.0.1 localhost localhost.localdomain\n\
::1       localhost localhost.localdomain localhost6 localhost6.localdomain6\n\
";
#endif	/* REFERENCE */

    /* Fetch and prepare sources from SCM */
#ifdef	REFERENCE
    if (F_ISSET(roto, SCM_ENABLE)) {
        scmWorker = mock.scm.scmWorker(log, roto->scm_opts)
        scmWorker.get_sources()
        (options.sources, options.spec) = scmWorker.prepare_sources()
    }
#endif	/* REFERENCE */

    /* elevate privs */
    xx = uidManager_becomeUser(roto, 0, 0);

    /* do whatever we're here to do */
    rpmlog(RPMLOG_INFO, "%s version(%s) starting...\n", __progname, roto->version);

#ifdef	REFERENCE	/* XXX this is where chroot.__init__ is run. */
    chroot = mock.backend.Root(config_opts, uidManager)
#endif	/* REFERENCE */

    if (F_ISSET(roto, PRINT_ROOT_PATH)) {
	fprintf(stdout, "%s\n", roto->_rootdir);
	ec = 0;
	goto exit;
    }

    /* Add macros for templating. */
    /* XXX FIXME: MUSTBE before roto->basedir + "/roto" appending */
    addMacro(NULL, "__roto_progname",		NULL, __progname, -1);
    addMacro(NULL, "__roto_basedir",		NULL, roto->basedir, -1);
    roto->basedir = rpmGetPath(roto->basedir, "/root", NULL);
    addMacro(NULL, "__roto_root",		NULL, roto->_root, -1);

    addMacro(NULL, "__roto_chroothome",		NULL, roto->chroothome, -1);
    addMacro(NULL, "__roto_chrootuser",		NULL, roto->chrootuser, -1);
    xx = snprintf(b, sizeof(b), "%d", roto->chrootuid);
    addMacro(NULL, "__roto_chrootuid",		NULL, b, -1);
    xx = snprintf(b, sizeof(b), "%d", roto->chrootgid);
    addMacro(NULL, "__roto_chrootgid",		NULL, b, -1);

    /* XXX swiped from Root.__init__ */

    /* mount/umount */
    roto->umountCmds = NULL;
    argvAdd(&roto->umountCmds,
	"umount -n %{__roto_basedir}/%{__roto_root}/root/proc");
    argvAdd(&roto->umountCmds,
	"umount -n %{__roto_basedir}/%{__roto_root}/root/sys");

    roto->mountCmds = NULL;
    argvAdd(&roto->mountCmds,
	"mount -n -t proc mock_chroot_proc %{__roto_basedir}/%{__roto_root}/root/proc");
    argvAdd(&roto->mountCmds,
	"mount -n -t sysfs mock_chroot_sysfs %{__roto_basedir}/%{__roto_root}/root/sys");

    (void) chrootState(roto, "init plugins");
    xx = chroot_initPlugins(roto);

    (void) chrootState(roto, "start");

    /* dump configuration to log */
    rpmlog(RPMLOG_DEBUG, "%s final configuration:\n", __progname);
#ifdef	REFERENCE
    for k, v in config_opts.items():
        rpmlog(RPMLOG_DEBUG, "    %s:  %s\n", k, v);
#else
rotoPrint(roto, NULL);
#endif	/* REFERENCE */

#ifdef	REFERENCE
    ret["chroot"] = chroot
    ret["config_opts"] = config_opts
#endif	/* REFERENCE */

    (void) umask(002);
    xx = setenv("HOME", roto->homedir, 1);

    /* New namespace starting from here */
    if ((xx = utilUnshare(roto, CLONE_NEWNS))) {
	rpmlog(RPMLOG_INFO, "Namespace unshare failed.\n");
	goto exit;
    }

    /* set personality (ie. setarch) */
    EXEC_t xp = NULL;
/* XXX FIXME: xp->personality = roto->target_arch */
    if (roto->internal_setarch)
        xx = execPersonality(xp);

    if (F_ISSET(roto, INIT)) {
	if (F_ISSET(roto, CLEAN))
            xx = chrootClean(roto);
assert(roto->chrootWasCleaned);
        xx = chrootInit(roto);
    } else
    if (F_ISSET(roto, CLEAN)) {
#ifdef	REFERENCE
        if len(options.scrub) == 0:
            xx = chrootClean(roto);
        else:
#endif	/* REFERENCE */
            chrootScrub(roto, NULL);	/* XXX FIXME: arg2 was options.scrub */
    } else
    if (F_ISSET(roto, SHELL)) {
        xx = chrootTryLockBuildRoot(roto);

	if (!((xx = Stat(roto->_rootdir, &sb)) == 0 && S_ISDIR(sb.st_mode))) {
            rpmlog(RPMLOG_ERR, "chroot %s not initialized!\n", roto->_rootdir);
#ifdef	REFERENCE
            raise RuntimeError, "chroot %s not initialized!" % roto->_rootdir
#endif	/* REFERENCE */
	    ec = 50;
	    goto exit;
	}

/*      try: */
            xx = chroot_setupDev(roto, 1);
            xx = chroot_mountall(roto);
#ifdef	REFERENCE
            cmd = ' '.join(args)
            if (F_ISSET(roto, UNPRIV)) {
                arg = '--userspec=%s:%s' % (roto->chrootuid, roto->chrootgid)
	    } else {
                arg = ''
	    }
            status = os.system("SHELL='/bin/sh' PS1='mock-chroot> ' /usr/sbin/chroot %s %s %s" % (arg, roto->_rootdir, cmd))
            ret['exitStatus'] = os.WEXITSTATUS(status)
#else
	    cmd = rpmExpand("SHELL='/bin/sh' PS1='mock-chroot> ' ",
		"/usr/sbin/chroot ",
		(F_ISSET(roto, UNPRIV)
		    ? "--userspec=%{__roto_chrootuid}:%{__roto_chrootgid} "
		    : ""),
		roto->_rootdir, " ",
		(av && *av ? argvJoin(av, ' ') : ""),	/* XXX memory leak */
		NULL);
fprintf(stderr, "==>  shell cmd: %s\n", cmd);
	    cmd = _free(cmd);
#endif	/* REFERENCE */

/*  finally: */
            xx = chroot_umountall(roto);
        xx = chrootUnlockBuildRoot(roto);
    } else
    if (F_ISSET(roto, CHROOT)) {
        if (ac == 0) {
            rpmlog(RPMLOG_CRIT, "You must specify a command to run\n");
	    ec = 50;
	    goto exit;
	}
#ifdef	REFERENCE
        if (ac == 1) {
            args = args[0]
            shell=True
	}
#endif	/* REFERENCE */

	cmd = argvJoin(av, ' ');
        rpmlog(RPMLOG_INFO, "Running in chroot: %s\n", cmd);

        xx = chrootTryLockBuildRoot(roto);
        xx = chroot_resetLogging(roto);
/*      try: */
            xx = chroot_mountall(roto);
            if (F_ISSET(roto, UNPRIV)) {
		/* XXX FIXME: use iob for output */
		/* XXX FIXME: mock sets chroot uid/gid and cwd. */
fprintf(stderr, "==> unpriv cmd: %s\n", cmd);
	    } else {
		/* XXX FIXME: use iob for output */
		/* XXX FIXME: mock sets cwd. */
fprintf(stderr, "==>   priv cmd: %s\n", cmd);
	    }
/*  finally: */
            xx = chroot_umountall(roto);
        xx = chrootUnlockBuildRoot(roto);
        if (out && *out)
            fprintf(stdout, "%s", out);
	out = _free(out);
	cmd = _free(cmd);
    } else
    if (F_ISSET(roto, INSTALLDEPS)) {
        if (ac == 0) {
            rpmlog(RPMLOG_CRIT, "You must specify an SRPM file.\n");
	    ec = 50;
	    goto exit;
	}

#ifdef	REFERENCE
        for hdr in mock.util.yieldSrpmHeaders(args, plainRpmOk=1):
            pass
#endif	/* REFERENCE */

        xx = chrootTryLockBuildRoot(roto);
/*      try: */
            xx = chroot_mountall(roto);
/* XXX FIXME: multiple srpms? */
            xx = chrootInstallSrpmDeps(roto, av[0]);
/*  finally: */
            xx = chroot_umountall(roto);
        xx = chrootUnlockBuildRoot(roto);
    } else
    if (F_ISSET(roto, INSTALL)) {
        if (ac == 0) {
            rpmlog(RPMLOG_CRIT, "You must specify a package list to install.\n");
	    ec = 50;
	    goto exit;
	}

        xx = chroot_resetLogging(roto);
        xx = chrootTryLockBuildRoot(roto);
        out = chrootYumInstall(roto, av);
	out = _free(out);
        xx = chrootUnlockBuildRoot(roto);
    } else
    if (F_ISSET(roto, UPDATE)) {
        xx = chroot_resetLogging(roto);
        xx = chrootTryLockBuildRoot(roto);
        xx = chrootYumUpdate(roto);
        xx = chrootUnlockBuildRoot(roto);
    } else
    if (F_ISSET(roto, REBUILD)) {
#ifdef	REFERENCE
	if (F_ISSET(roto, SCM)) {
            srpm = do_buildsrpm(roto)
            if srpm:
                args.append(srpm)
            scmWorker.clean()
	}
#endif	/* REFERENCE */
        xx = do_rebuild(roto, av);
    } else
    if (F_ISSET(roto, BUILDSRPM)) {
        srpm = do_buildsrpm(roto);
    } else
    if (F_ISSET(roto, ORPHANSKILL)) {
        utilOrphansKill(roto->_rootdir, SIGTERM);
    } else
    if (F_ISSET(roto, COPYIN)) {
        xx = chrootTryLockBuildRoot(roto);
        xx = chroot_resetLogging(roto);
        //xx = uidManagerDropPrivsForever(roto);
        if (ac < 2) {
            rpmlog(RPMLOG_CRIT, "Must have source and destinations for copyin\n");
	    ec = 50;
	    goto exit;
	}
        const char * dest = rpmGetPath(roto->_rootdir, "/", av[ac-1], NULL);
	int i;
        if (ac > 2 && !((xx = Stat(dest, &sb)) == 0 && S_ISDIR(sb.st_mode))) {
            rpmlog(RPMLOG_CRIT, "multiple source files and %s is not a directory!\n", dest);
	    ec = 50;
	    goto exit;
	}
	for (i = 0; i < (ac -1); i++) {
	    const char * src = av[i];
            rpmlog(RPMLOG_INFO, "copying %s to %s\n", src, dest);
	    if ((xx = Stat(src, &sb)) == 0 && S_ISDIR(sb.st_mode))
                xx = shutilCopyTree(roto, src, dest);
	    else
                xx = shutilCopy(src, dest);
	}
	dest = _free(dest);
        xx = chrootUnlockBuildRoot(roto);
    } else
    if (F_ISSET(roto, COPYOUT)) {
        xx = chrootTryLockBuildRoot(roto);
        xx = chroot_resetLogging(roto);
        xx = uidManagerDropPrivsForever(roto);
        if (ac < 2) {
            rpmlog(RPMLOG_CRIT, "Must have source and destinations for copyout\n");
	    ec = 50;
	    goto exit;
	}
        const char * dest = av[ac-1];
	int i;
        if (ac > 2 && !((xx = Stat(dest, &sb)) == 0 && S_ISDIR(sb.st_mode))) {
            rpmlog(RPMLOG_CRIT, "multiple source files and %s is not a directory!", dest);
	    ec = 50;
	    goto exit;
	}
	for (i = 0; i < (ac -1); i++) {
	    const char * src = rpmGetPath(roto->_rootdir, "/", av[i], NULL);
            rpmlog(RPMLOG_INFO, "copying %s to %s\n", src, dest);
	    if ((xx = Stat(src, &sb)) == 0 && S_ISDIR(sb.st_mode))
                xx = shutilCopyTree(roto, src, dest);
	    else
                xx = shutilCopy(src, dest);
	    src = _free(src);
	}
        xx = chrootUnlockBuildRoot(roto);
    }
	/* XXX usage? help? */

    (void) chrootState(roto, "end");

    ec = 0;

exit:
    roto->basedir = _free(roto->basedir);
    roto->umountCmds = argvFree(roto->umountCmds);
    roto->mountCmds = argvFree(roto->mountCmds);
#ifdef	NOTYET
    roto = rotoFree(roto);
#endif
    optCon = rpmioFini(optCon);

    return ec;
}
