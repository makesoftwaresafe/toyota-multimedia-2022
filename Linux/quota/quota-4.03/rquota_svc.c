/*
 * QUOTA    An implementation of the diskquota system for the LINUX operating
 *          system. QUOTA is implemented using the BSD systemcall interface
 *          as the means of communication with the user level. Should work for
 *          all filesystems because of integration into the VFS layer of the
 *          operating system. This is based on the Melbourne quota system wich
 *          uses both user and group quota files.
 *
 *          Rquota service handlers.
 *
 * Author:  Marco van Wieringen <mvw@planets.elm.net>
 *          changes for new utilities by Jan Kara <jack@suse.cz>
 *          patches by Jani Jaakkola <jjaakkol@cs.helsinki.fi>
 *
 *          This program is free software; you can redistribute it and/or
 *          modify it under the terms of the GNU General Public License as
 *          published by the Free Software Foundation; either version 2 of
 *          the License, or (at your option) any later version.
 */
                                                                                                          
#include "config.h"

#include <rpc/rpc.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <rpc/pmap_clnt.h>	/* for pmap_unset */
#include <stdio.h>
#include <stdlib.h>		/* getenv, exit */
#include <string.h>		/* strcmp */
#include <memory.h>
#include <unistd.h>
#include <getopt.h>
#include <signal.h>
#include <errno.h>
#ifdef HOSTS_ACCESS
#include <tcpd.h>
#include <netdb.h>

int deny_severity, allow_severity;	/* Needed by some versions of libwrap */
#endif

#ifdef __STDC__
#define SIG_PF void(*)(int)
#endif

extern int svctcp_socket (u_long __number, int __port, int __reuse);
extern int svcudp_socket (u_long __number, int __port, int __reuse);

#include "pot.h"
#include "common.h"
#include "rquota.h"
#include "quotasys.h"

char *progname;

/*
 * Global authentication credentials.
 */
struct authunix_parms *unix_cred;

#define FL_SETQUOTA 1	/* Enable setquota rpc */
#define FL_NODAEMON 2	/* Disable daemon() call */
#define FL_AUTOFS   4	/* Don't ignore autofs mountpoints */

int flags;				/* Options specified on command line */ 
static int port;			/* Port to use (0 for default one) */
static char xtab_path[PATH_MAX];	/* Path to NFSD export table */
char nfs_pseudoroot[PATH_MAX];		/* Root of the virtual NFS filesystem ('/' for NFSv3) */

static struct option options[]= {
	{ "version", 0, NULL, 'V' },
	{ "help", 0, NULL, 'h' },
	{ "foreground", 0 , NULL, 'F' },
#ifdef RPC_SETQUOTA
	{ "no-setquota", 0 , NULL, 's' },
	{ "setquota", 0, NULL, 'S' },
#endif
	{ "autofs", 0, NULL, 'I'},
	{ "port", 1, NULL, 'p' },
	{ "xtab", 1, NULL, 'x' },
	{ NULL, 0, NULL , 0 }
};

static void show_help(void)
{
#ifdef RPC_SETQUOTA
	errstr(_("Usage: %s [options]\nOptions are:\n\
 -h --help             shows this text\n\
 -V --version          shows version information\n\
 -F --foreground       starts the quota service in foreground\n\
 -I --autofs           do not ignore mountpoints mounted by automounter\n\
 -p --port <port>      listen on given port\n\
 -s --no-setquota      disables remote calls to setquota (default)\n\
 -S --setquota         enables remote calls to setquota\n\
 -x --xtab <path>      set an alternative file with NFSD export table\n"), progname);

#else
	errstr(_("Usage: %s [options]\nOptions are:\n\
 -h --help             shows this text\n\
 -V --version          shows version information\n\
 -F --foreground       starts the quota service in foreground\n\
 -I --autofs           do not ignore mountpoints mounted by automounter\n\
 -p --port <port>      listen on given port\n\
 -x --xtab <path>      set an alternative file with NFSD export table\n"), progname);
#endif
}

static void parse_options(int argc, char **argv)
{
	char ostr[128]="", *endptr;
	int i,opt;
	int j=0;

	sstrncpy(xtab_path, NFSD_XTAB_PATH, PATH_MAX);
	for(i=0; options[i].name; i++) {
		ostr[j++] = options[i].val;
		if (options[i].has_arg)
			ostr[j++] = ':';
	}
	while ((opt=getopt_long(argc, argv, ostr, options, NULL))>=0) {
		switch(opt) {
			case 'V': 
				version();
				exit(0);
			case 'h':
				show_help();
				exit(0);
			case 'F':
				flags |= FL_NODAEMON;
				break;
#ifdef RPC_SETQUOTA
			case 's':
				flags &= ~FL_SETQUOTA;
				break;
			case 'S':	
				flags |= FL_SETQUOTA;
				break;
#endif
			case 'I':
				flags |= FL_AUTOFS;
				break;
			case 'p': 
				port = strtol(optarg, &endptr, 0);
				if (*endptr || port <= 0) {
					errstr(_("Illegal port number: %s\n"), optarg);
					show_help();
					exit(1);
				}
				break;
			case 'x':
				if (access(optarg, R_OK) < 0) {
					errstr(_("Cannot access the specified xtab file %s: %s\n"), optarg, strerror(errno));
					show_help();
					exit(1);
				}
				sstrncpy(xtab_path, optarg, PATH_MAX);
				break;
			default:
				errstr(_("Unknown option '%c'.\n"), opt);
				show_help();
				exit(1);
		}
	}
}


/*
 * good_client checks if an quota client should be allowed to
 * execute the requested rpc call.
 */
static int good_client(struct sockaddr_in *addr, ulong rq_proc)
{
#ifdef HOSTS_ACCESS
	struct request_info req;
#endif
	char *remote = inet_ntoa(addr->sin_addr);

	if (rq_proc==RQUOTAPROC_SETQUOTA ||
	     rq_proc==RQUOTAPROC_SETACTIVEQUOTA) {
		/* If setquota is disabled, fail always */
		if (!(flags & FL_SETQUOTA)) {
			errstr(_("host %s attempted to call setquota when disabled\n"),
			       remote);

			return 0;
		}
		/* Require that SETQUOTA calls originate from port < 1024 */
		if (ntohs(addr->sin_port)>=1024) {
			errstr(_("host %s attempted to call setquota from port >= 1024\n"),
			       remote);
			return 0;
		}
		/* Setquota OK */
	}

#ifdef HOSTS_ACCESS
	/* NOTE: we could use different servicename for setquota calls to
	 * allow only some hosts to call setquota. */

	request_init(&req, RQ_DAEMON, "rquotad", RQ_CLIENT_SIN, addr, 0);
	sock_methods(&req);
	if (hosts_access(&req))
		return 1;
	errstr(_("Denied access to host %s\n"), remote);
	return 0;
#else
	/* If no access checking is available, OK always */
	return 1;
#endif
}

static void rquotaprog_1(struct svc_req *rqstp, register SVCXPRT * transp)
{
	union {
		getquota_args rquotaproc_getquota_1_arg;
		setquota_args rquotaproc_setquota_1_arg;
		getquota_args rquotaproc_getactivequota_1_arg;
		setquota_args rquotaproc_setactivequota_1_arg;
	} argument;
	char *result;
	xdrproc_t xdr_argument, xdr_result;
	char *(*local) (char *, struct svc_req *);

	/*
	 *  Authenticate host
	 */
	if (!good_client(svc_getcaller(rqstp->rq_xprt),rqstp->rq_proc)) {
		svcerr_auth (transp, AUTH_FAILED);
		return;
	}

	/*
	 * Don't bother authentication for NULLPROC.
	 */
	if (rqstp->rq_proc == NULLPROC) {
		(void)svc_sendreply(transp, (xdrproc_t) xdr_void, (char *)NULL);
		return;
	}

	/*
	 * Get authentication.
	 */
	switch (rqstp->rq_cred.oa_flavor) {
	  case AUTH_UNIX:
		  unix_cred = (struct authunix_parms *)rqstp->rq_clntcred;
		  break;
	  case AUTH_NULL:
	  default:
		  svcerr_weakauth(transp);
		  return;
	}

	switch (rqstp->rq_proc) {
	  case RQUOTAPROC_GETQUOTA:
		  xdr_argument = (xdrproc_t) xdr_getquota_args;
		  xdr_result = (xdrproc_t) xdr_getquota_rslt;
		  local = (char *(*)(char *, struct svc_req *))rquotaproc_getquota_1_svc;
		  break;

	  case RQUOTAPROC_SETQUOTA:
		  xdr_argument = (xdrproc_t) xdr_setquota_args;
		  xdr_result = (xdrproc_t) xdr_setquota_rslt;
		  local = (char *(*)(char *, struct svc_req *))rquotaproc_setquota_1_svc;
		  break;

	  case RQUOTAPROC_GETACTIVEQUOTA:
		  xdr_argument = (xdrproc_t) xdr_getquota_args;
		  xdr_result = (xdrproc_t) xdr_getquota_rslt;
		  local = (char *(*)(char *, struct svc_req *))rquotaproc_getactivequota_1_svc;
		  break;

	  case RQUOTAPROC_SETACTIVEQUOTA:
		  xdr_argument = (xdrproc_t) xdr_setquota_args;
		  xdr_result = (xdrproc_t) xdr_setquota_rslt;
		  local = (char *(*)(char *, struct svc_req *))rquotaproc_setactivequota_1_svc;
		  break;

	  default:
		  svcerr_noproc(transp);
		  return;
	}
	memset(&argument, 0, sizeof(argument));
	if (!svc_getargs(transp, xdr_argument, (caddr_t) & argument)) {
		svcerr_decode(transp);
		return;
	}
	result = (*local) ((char *)&argument, rqstp);
	if (result != NULL && !svc_sendreply(transp, xdr_result, result)) {
		svcerr_systemerr(transp);
	}
	if (!svc_freeargs(transp, xdr_argument, (caddr_t) & argument)) {
		errstr(_("unable to free arguments\n"));
		exit(1);
	}
	return;
}

static void rquotaprog_2(struct svc_req *rqstp, register SVCXPRT * transp)
{
	union {
		ext_getquota_args rquotaproc_getquota_2_arg;
		ext_setquota_args rquotaproc_setquota_2_arg;
		ext_getquota_args rquotaproc_getactivequota_2_arg;
		ext_setquota_args rquotaproc_setactivequota_2_arg;
	} argument;
	char *result;
	xdrproc_t xdr_argument, xdr_result;
	char *(*local) (char *, struct svc_req *);

	/*
	 *  Authenticate host
	 */
	if (!good_client(svc_getcaller(rqstp->rq_xprt),rqstp->rq_proc)) {
		svcerr_auth (transp, AUTH_FAILED);
		return;
	}

	/*
	 * Don't bother authentication for NULLPROC.
	 */
	if (rqstp->rq_proc == NULLPROC) {
		(void)svc_sendreply(transp, (xdrproc_t) xdr_void, (char *)NULL);
		return;
	}

	/*
	 * Get authentication.
	 */
	switch (rqstp->rq_cred.oa_flavor) {
	  case AUTH_UNIX:
		  unix_cred = (struct authunix_parms *)rqstp->rq_clntcred;
		  break;
	  case AUTH_NULL:
	  default:
		  svcerr_weakauth(transp);
		  return;
	}

	switch (rqstp->rq_proc) {
	  case RQUOTAPROC_GETQUOTA:
		  xdr_argument = (xdrproc_t) xdr_ext_getquota_args;
		  xdr_result = (xdrproc_t) xdr_getquota_rslt;
		  local = (char *(*)(char *, struct svc_req *))rquotaproc_getquota_2_svc;
		  break;

	  case RQUOTAPROC_SETQUOTA:
		  xdr_argument = (xdrproc_t) xdr_ext_setquota_args;
		  xdr_result = (xdrproc_t) xdr_setquota_rslt;
		  local = (char *(*)(char *, struct svc_req *))rquotaproc_setquota_2_svc;
		  break;

	  case RQUOTAPROC_GETACTIVEQUOTA:
		  xdr_argument = (xdrproc_t) xdr_ext_getquota_args;
		  xdr_result = (xdrproc_t) xdr_getquota_rslt;
		  local = (char *(*)(char *, struct svc_req *))rquotaproc_getactivequota_2_svc;
		  break;

	  case RQUOTAPROC_SETACTIVEQUOTA:
		  xdr_argument = (xdrproc_t) xdr_ext_setquota_args;
		  xdr_result = (xdrproc_t) xdr_setquota_rslt;
		  local = (char *(*)(char *, struct svc_req *))rquotaproc_setactivequota_2_svc;
		  break;

	  default:
		  svcerr_noproc(transp);
		  return;
	}
	memset(&argument, 0, sizeof(argument));
	if (!svc_getargs(transp, xdr_argument, (caddr_t) & argument)) {
		svcerr_decode(transp);
		return;
	}
	result = (*local) ((char *)&argument, rqstp);
	if (result != NULL && !svc_sendreply(transp, xdr_result, result)) {
		svcerr_systemerr(transp);
	}
	if (!svc_freeargs(transp, xdr_argument, (caddr_t) & argument)) {
		errstr(_("unable to free arguments\n"));
		exit(1);
	}
	return;
}

static void
unregister (int sig)
{
	pmap_unset(RQUOTAPROG, RQUOTAVERS);
	pmap_unset(RQUOTAPROG, EXT_RQUOTAVERS);
	exit(0);
}

/* Parse NFSD export table and find a filesystem pseudoroot if it is there */
static void get_pseudoroot(void)
{
	FILE *f;
	char exp_line[1024];
	char *c;

	strcpy(nfs_pseudoroot, "/");
	if (!(f = fopen(xtab_path, "r"))) {
		errstr(_("Warning: Cannot open export table %s: %s\nUsing '/' as a pseudofilesystem root.\n"), xtab_path, strerror(errno));
		return;
	}
	while (fgets(exp_line, sizeof(exp_line), f)) {
		if (exp_line[0] == '#' || exp_line[0] == '\n')	/* Comment, empty line? */
			continue;
		c = strchr(exp_line, '\t');
		if (!c)	/* Huh, line we don't understand... */
			continue;
		*c = 0;
		/* Find the beginning of export options */
		c = strchr(c+1, '(');
		if (!c)
			continue;
		c = strstr(c, "fsid=0");
		if (c) {
			sstrncpy(nfs_pseudoroot, exp_line, PATH_MAX);
			sstrncat(nfs_pseudoroot, "/", PATH_MAX);
			break;
		}
	}
	fclose(f);
}

int main(int argc, char **argv)
{
	register SVCXPRT *transp;
	struct sigaction sa;
	int sock;

	gettexton();
	progname = basename(argv[0]);
	parse_options(argc, argv);

	init_kernel_interface();
	get_pseudoroot();
	pmap_unset(RQUOTAPROG, RQUOTAVERS);
	pmap_unset(RQUOTAPROG, EXT_RQUOTAVERS);

	sa.sa_handler = SIG_IGN;
	sa.sa_flags = 0;
	sigemptyset(&sa.sa_mask);
	sigaction(SIGCHLD, &sa, NULL);

	sa.sa_handler = unregister;
	sigaction(SIGHUP, &sa, NULL);
	sigaction(SIGINT, &sa, NULL);
	sigaction(SIGTERM, &sa, NULL);

	sock = svcudp_socket(RQUOTAPROG, port, 1);
	transp = svcudp_create(sock == -1 ? RPC_ANYSOCK : sock);
	if (transp == NULL) {
		errstr(_("cannot create udp service.\n"));
		exit(1);
	}
	if (!svc_register(transp, RQUOTAPROG, RQUOTAVERS, rquotaprog_1, IPPROTO_UDP)) {
		errstr(_("unable to register (RQUOTAPROG, RQUOTAVERS, UDP).\n"));
		exit(1);
	}
	if (!svc_register(transp, RQUOTAPROG, EXT_RQUOTAVERS, rquotaprog_2, IPPROTO_UDP)) {
		errstr(_("unable to register (RQUOTAPROG, EXT_RQUOTAVERS, UDP).\n"));
		exit(1);
	}

	sock = svctcp_socket(RQUOTAPROG, port, 1);
	transp = svctcp_create(sock == -1 ? RPC_ANYSOCK : sock, 0, 0);
	if (transp == NULL) {
		errstr(_("cannot create TCP service.\n"));
		exit(1);
	}
	if (!svc_register(transp, RQUOTAPROG, RQUOTAVERS, rquotaprog_1, IPPROTO_TCP)) {
		errstr(_("unable to register (RQUOTAPROG, RQUOTAVERS, TCP).\n"));
		exit(1);
	}
	if (!svc_register(transp, RQUOTAPROG, EXT_RQUOTAVERS, rquotaprog_2, IPPROTO_TCP)) {
		errstr(_("unable to register (RQUOTAPROG, EXT_RQUOTAVERS, TCP).\n"));
		exit(1);
	}

	if (!(flags & FL_NODAEMON)) {
		use_syslog();
		daemon(0, 0);
	}
	svc_run();
	errstr(_("svc_run returned\n"));
	exit(1);
	/* NOTREACHED */
}
