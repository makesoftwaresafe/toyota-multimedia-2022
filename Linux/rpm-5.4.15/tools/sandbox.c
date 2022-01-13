/*
 * Copyright (c) 2009 The Chromium Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * http://code.google.com/p/chromium/wiki/LinuxSUIDSandbox
 */

#include "system.h"

#include <asm/unistd.h>
#include <sched.h>
#include <stdarg.h>
#include <sys/prctl.h>

#include "debug.h"

#if !defined(CLONE_NEWPID)
#define CLONE_NEWPID 0x20000000
#endif

static char tempdirTemplate[] = "/tmp/rpm-sandbox-chroot-XXXXXX";

static void _err(int code, const char *msg, ...)
	__attribute__((noreturn, format(printf, 2, 3)));

static void
_err(int code, const char *fmt, ...)
{
    FILE * fp = stderr;
    va_list ap;
    va_start(ap, fmt);
    if (fmt != NULL) {
	vfprintf(fp, fmt, ap);
	fprintf(fp, ": ");
    }
    fprintf(fp, "%s\n", strerror(errno));
    va_end(ap);
    exit(code);
}

static int CloneChrootHelperProcess(void)
{
    int sv[2] = { -1, -1 };
    const char * temp_dir = NULL;
    int chroot_dir_fd = -1;
    char proc_self_fd_str[128];
    int printed;
    pid_t pid;
    int rc = -1;	/* assume failure */

    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sv) == -1) {
	perror("socketpair");
	goto exit;
    }

    /*
     * We create a temp directory for our chroot. Nobody should ever write into
     * it, so it's root:root mode 000.
     */
    temp_dir = mkdtemp(tempdirTemplate);
    if (!temp_dir) {
	perror("Failed to create temp directory for chroot");
	goto exit;
    }

    chroot_dir_fd = open(temp_dir, O_DIRECTORY | O_RDONLY);
    if (chroot_dir_fd < 0) {
	rmdir(temp_dir);
	perror("Failed to open chroot temp directory");
	goto exit;
    }

    if (rmdir(temp_dir)) {
	perror("rmdir");
	goto exit;
    }

    printed = snprintf(proc_self_fd_str, sizeof(proc_self_fd_str),
                         "/proc/self/fd/%d", chroot_dir_fd);
    if (printed < 0 || printed >= (int)sizeof(proc_self_fd_str)) {
	fprintf(stderr, "Error in snprintf");
	goto exit;
    }

    if (fchown(chroot_dir_fd, 0 /* root */, 0 /* root */)) {
	perror("fchown");
	goto exit;
    }

    if (fchmod(chroot_dir_fd, 0000 /* no-access */)) {
	perror("fchmod");
	goto exit;
    }

    pid = syscall(__NR_clone, CLONE_FS | SIGCHLD, 0, 0, 0);
    if (pid == -1) {
	perror("clone");
	close(sv[0]);
	close(sv[1]);
	goto exit;
    }

    if (pid == 0) {
	static const struct rlimit nofile = { 0, 0 };
	ssize_t bytes;
	struct stat st;
	char msg;
	char reply;

	/*
	 * The files structure is shared with an untrusted process.
	 * For additional security, the file resource limits are set to 0.
	 * TODO: drop CAP_SYS_RESOURCE / use SECURE_NOROOT
	 */
	if (setrlimit(RLIMIT_NOFILE, &nofile))
	    _err(EXIT_FAILURE, "Setting RLIMIT_NOFILE");

	if (close(sv[1]))
	    _err(EXIT_FAILURE, "close");

	/* wait for message */
	do {
	    bytes = read(sv[0], &msg, 1);
	} while (bytes == -1 && errno == EINTR);

	if (bytes == 0)
	    _exit(EXIT_SUCCESS);
	if (bytes != 1)
	    err(1, "read");

	/* do chrooting */
	if (msg != 'C')
	    _err(EXIT_FAILURE, "Unknown message from sandboxed process");

	if (fchdir(chroot_dir_fd))
	    _err(EXIT_FAILURE, "Cannot chdir into chroot temp directory");

	if (fstat(chroot_dir_fd, &st))
	    _err(EXIT_FAILURE, "stat");

	if (st.st_uid || st.st_gid || st.st_mode & 0777)
	    _err(EXIT_FAILURE, "Bad permissions on chroot temp directory");

	if (chroot(proc_self_fd_str))
	    _err(EXIT_FAILURE, "Cannot chroot into temp directory");

	if (chdir("/"))
	    _err(EXIT_FAILURE, "Cannot chdir to / after chroot");

	reply = 'O';
	do {
	    bytes = write(sv[0], &reply, 1);
	} while (bytes == -1 && errno == EINTR);

	if (bytes != 1)
	    _err(EXIT_FAILURE, "Writing reply");

	_exit(EXIT_SUCCESS);
    }

    if (close(chroot_dir_fd)) {
	close(sv[0]);
	close(sv[1]);
	perror("close(chroot_dir_fd)");
	goto exit;
    }

    if (close(sv[0])) {
	close(sv[1]);
	perror("close");
	goto exit;
    }
    rc = sv[1];

exit:
    return rc;
}

static int SpawnChrootHelper(void)
{
    static const char SBX_D[] = "SBX_D";
    const int chroot_signal_fd = CloneChrootHelperProcess();
    char desc_str[64];
    int printed;
    int rc = -1;	/* assume failure */

    if (chroot_signal_fd == -1)
	goto exit;

    /*
     * In the parent process, we install an environment variable containing the
     * number of the file descriptor.
     */
    printed = snprintf(desc_str, sizeof(desc_str), "%d", chroot_signal_fd);
    if (printed < 0 || printed >= (int)sizeof(desc_str)) {
	fprintf(stderr, "Failed to snprintf\n");
	goto exit;
    }

    if (setenv(SBX_D, desc_str, 1)) {
	perror("setenv");
	close(chroot_signal_fd);
	goto exit;
    }
    rc = 0;
exit:
    return rc;
}

static int MoveToNewPIDNamespace(void)
{
    const pid_t pid = syscall(__NR_clone, CLONE_NEWPID | SIGCHLD, 0, 0, 0);
    int rc = -1;	/* assume failure */

    if (pid == -1) {
	/* System doesn't support NEWPID. We soldier on anyway. */
	if (errno == EINVAL)
	    rc = 0;
	else
	    perror("Failed to move to new PID namespace");
	goto exit;
    }

    if (pid)
	_exit(EXIT_SUCCESS);
    rc = 0;
exit:
    return rc;
}

static int DropRoot(void)
{
    uid_t ruid, euid, suid;
    gid_t rgid, egid, sgid;
    int rc = -1;	/* assume failure */

    if (prctl(PR_SET_DUMPABLE, 0, 0, 0, 0)) {
	perror("prctl(PR_SET_DUMPABLE)");
	goto exit;
    }

    if (prctl(PR_GET_DUMPABLE, 0, 0, 0, 0)) {
	perror("Still dumpable after prctl(PR_SET_DUMPABLE)");
	goto exit;
    }

    if (getresgid(&rgid, &egid, &sgid)) {
	perror("getresgid");
	goto exit;
    }

    if (setresgid(rgid, rgid, rgid)) {
	perror("setresgid");
	goto exit;
    }

    if (getresuid(&ruid, &euid, &suid)) {
	perror("getresuid");
	goto exit;
    }

    if (setresuid(ruid, ruid, ruid)) {
	perror("setresuid");
	goto exit;
    }
    rc = 0;
exit:
    return rc;
}

static const char * envvars[] = {
  "LD_AOUT_LIBRARY_PATH",
  "LD_AOUT_PRELOAD",
  "GCONV_PATH",
  "GETCONF_DIR",
  "HOSTALIASES",
  "LD_AUDIT",
  "LD_DEBUG",
  "LD_DEBUG_OUTPUT",
  "LD_DYNAMIC_WEAK",
  "LD_LIBRARY_PATH",
  "LD_ORIGIN_PATH",
  "LD_PRELOAD",
  "LD_PROFILE",
  "LD_SHOW_AUXV",
  "LD_USE_LOAD_BIAS",
  "LOCALDOMAIN",
  "LOCPATH",
  "MALLOC_TRACE",
  "NIS_PATH",
  "NLSPATH",
  "RESOLV_HOST_CONF",
  "RES_OPTIONS",
  "TMPDIR",
  "TZDIR",
  NULL,
};

#define	SANDBOX_ "SANDBOX_"
static int SetupChildEnvironment(void)
{
    unsigned i;
    int rc = -1;	/* assume failure */

    /*
     * ld.so may have cleared several environment variables because we are SUID.
     * However, the child process might need them so zygote_host_linux.cc saves
     * a copy in SANDBOX_$x. This is safe because we have dropped root by this
     * point, so we can only exec a binary with the permissions of the user who
     * ran us in the first place.
     */
    for (i = 0; envvars[i] != NULL; ++i) {
	const char * const s = envvars[i];
	const size_t nt = sizeof(SANDBOX_) + strlen(s);
	char * t;
	const char * value;

	if ((t = malloc(nt)) == NULL)
	    goto exit;
	(void) stpcpy( stpcpy(t, SANDBOX_), s);
	if ((value = getenv(t)) != NULL) {
	    setenv(s, value, 1 /* overwrite */);
	    unsetenv(t);
	}
	free(t);
    }
    rc = 0;
exit:
    return rc;
}
#undef	SANDBOX_

int main(int argc, char **argv)
{
    if (argc <= 1) {
	fprintf(stderr, "Usage: %s <renderer process> <args...>\n", argv[0]);
	return EXIT_FAILURE;
    }

    if (MoveToNewPIDNamespace())
	return EXIT_FAILURE;
    if (SpawnChrootHelper())
	return EXIT_FAILURE;
    if (DropRoot())
	return EXIT_FAILURE;
    if (SetupChildEnvironment())
	return EXIT_FAILURE;

    execv(argv[1], &argv[1]);
    _err(EXIT_FAILURE, "execv failed");

    return EXIT_FAILURE;
}
