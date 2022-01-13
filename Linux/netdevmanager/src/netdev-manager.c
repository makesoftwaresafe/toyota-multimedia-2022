/*
 * Copyright (C)2017 NTT DATA MSE CORPORATION
 * Copyright (C)2015, 2017 MIRACLE LINUX CORPORATION
 * Copyright (C)2004 USAGI/WIDE Project
 * Copyright (c) 1989 The Regents of the University of California.
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/*
 * VERSION HISTORY
 * ---------------
 * v0.4 - [LTS-235] some options(-anmg) turned optional
 * v0.3 - [LTS-234] added option -l for targeting HWAddr
 * v0.2 - lots of various cleanups
 * v0.1 - First beta release
 */

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <net/if.h>
#include <time.h>
#include <errno.h>

#include <sys/ioctl.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <net/route.h>

/* Function.prototype */
int32_t rtnl_open(void);
int32_t rtnl_dump_request(int32_t fd);
int32_t parse_rtattr(struct rtattr *tb[], int32_t max, struct rtattr *rta, int32_t len);
static int32_t do_ifup(const char *netdev);
static int32_t set_addr(int32_t skfd, const char *netdev, in_addr_t ip);
int32_t print_linkinfo(const struct sockaddr_nl *who, struct nlmsghdr *n, char *netdev);
int32_t rtnl_listen(int32_t fd, char *netdev);
static int32_t get_ifname_by_hwaddr(const char* hwaddr_str, char* ifname);
static const char* get_ifname(const char *line, char* ifname);
static int32_t set_netmask(int32_t skfd, const char *netdev, in_addr_t ip);
static int32_t set_mtu(int32_t skfd, const char *netdev, int mtu);
static int32_t set_gw(int32_t skfd, char *netdev, in_addr_t gw);
static int32_t do_changename(const char *dev, const char *newdev);
static void usage(void);


/* gcc -Wall netdev-manager.c -o netdev-manager */

#define HWADDR_STR_LEN (17)
#define HWADDR_BYTE_LEN (6)
#define PATH_PROCNET_DEV "/proc/net/dev"
static int debug = 0 ;

#define dprintf(fmt, args...) \
	do { if (debug) printf(fmt, ##args); } while (0)


int32_t rtnl_open(void)
{
	socklen_t addr_len;
	int32_t fd;
	struct sockaddr_nl      local;

	/* create rtnetlink socket */
	fd = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
	if (fd < 0) {
		perror("Cannot open netlink socket");
		return -1;
	}

	memset(&local, 0, sizeof(local));
	local.nl_family = AF_NETLINK;
	/* monitor link */
	local.nl_groups = RTMGRP_LINK;

	if (bind(fd, (struct sockaddr*)&local, sizeof(local)) < 0) {
		perror("Cannot bind netlink socket");
		close(fd);
		return -1;
	}
	addr_len = sizeof(local);
	if (getsockname(fd, (struct sockaddr*)&local, &addr_len) < 0) {
		perror("Cannot getsockname");
		close(fd);
		return -1;
	}
	if (addr_len != sizeof(local)) {
		fprintf(stderr, "Wrong address length %d\n", addr_len);
		close(fd);
		return -1;
	}
	if (local.nl_family != AF_NETLINK) {
		fprintf(stderr, "Wrong address family %d\n", local.nl_family);
		close(fd);
		return -1;
	}
	/* seq = time(NULL); */
	return fd;
}

int32_t rtnl_dump_request(int32_t fd)
{
	struct {
		struct nlmsghdr nlh;
		struct rtgenmsg g;
	} req;
	struct sockaddr_nl nladdr;

	memset(&nladdr, 0, sizeof(nladdr));
	nladdr.nl_family = AF_NETLINK;

	memset(&req, 0, sizeof(req));
	req.nlh.nlmsg_len = sizeof(req);
	req.nlh.nlmsg_type = RTM_GETLINK;
	/* req.nlh.nlmsg_flags = NLM_F_ROOT|NLM_F_MATCH|NLM_F_REQUEST; */
	req.nlh.nlmsg_flags = NLM_F_DUMP|NLM_F_REQUEST;
	req.nlh.nlmsg_pid = 0;
	/* req.nlh.nlmsg_seq = ++seq; */
	req.g.rtgen_family = AF_UNSPEC;

	return sendto(fd, (void*)&req, sizeof(req), 0,
			(struct sockaddr*)&nladdr, sizeof(nladdr));
}

int32_t parse_rtattr(struct rtattr *tb[], int32_t max, struct rtattr *rta, int32_t len)
{
	memset(tb, 0, sizeof(struct rtattr *) * (max + 1));
	while (RTA_OK(rta, len))
	{
		if (rta->rta_type <= max)
		{
			tb[rta->rta_type] = rta;
			rta = RTA_NEXT(rta,len);
		}
	}
	if (len)
	{
		fprintf(stderr, "!!!Deficit %d, rta_len=%d\n", len, rta->rta_len);
	}
	return 0;
}

static int32_t do_ifup(const char *netdev)
{
	struct ifreq ifr;
	int32_t fd;
	int32_t err;

	strncpy(ifr.ifr_name, netdev, IFNAMSIZ);

	fd = socket(PF_INET, SOCK_DGRAM, 0);
	if (fd < 0) {
		perror("socket");
		return -1;
	}

	err = ioctl(fd, SIOCGIFFLAGS, &ifr);
	if (err) {
		perror("SIOCGIFFLAGS");
		err = -1;
		goto ifup_err;
	}
	ifr.ifr_flags |= IFF_UP;
	err = ioctl(fd, SIOCSIFFLAGS, &ifr);
	if (err) {
		perror("SIOCSIFFLAGS");
		err = -1;
		goto ifup_err;
	}

	dprintf("Interface '%s': flags set to %04X.\n", netdev, ifr.ifr_flags);

ifup_err:
	close(fd);
	return err;
}

static int32_t set_addr(int32_t skfd, const char *netdev, in_addr_t ip)
{
	struct ifreq ifr;
	struct sockaddr_in sin;

	memset(&ifr, 0, sizeof(ifr));
	strncpy(ifr.ifr_name, netdev, IFNAMSIZ-1);

	memset(&sin, 0, sizeof(struct sockaddr));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = ip;

	dprintf("addr %s\n", inet_ntoa(*(struct in_addr *)&(sin.sin_addr.s_addr)));
	memcpy(&ifr.ifr_addr, &sin, sizeof(struct sockaddr));

	if (ioctl(skfd, SIOCSIFADDR, &ifr) < 0) {
		perror("SIOCSIFADDR");
		return -1;
	}
	return 0;
}

#define IFF_LOWER_UP	0x10000	/* driver signals L1 up	*/
#define IFF_DORMANT	0x20000	/* driver signals dormant	*/
#define IFF_ECHO	0x40000	/* echo sent packets	*/

#define NEED_IFUP	2

/* print_linkinfo:
 * returns: 0 if netlink message of netdev and IFF_UP is set.
 * returns: 1 if netlink message's device different to netdev.
 * returns: 2 if netlink message of netdev and IFF_UP is NOT set.
 * returns: -1 is error.
 */
int32_t print_linkinfo(const struct sockaddr_nl *who,
			struct nlmsghdr *n, char *netdev)
{
	struct ifinfomsg *ifi = NLMSG_DATA(n);
	struct rtattr *tb[IFLA_MAX+1];
	int32_t len = n->nlmsg_len;

/*
	if (n->nlmsg_type != RTM_NEWLINK && n->nlmsg_type != RTM_DELLINK)
		return 1;
*/

	len -= NLMSG_LENGTH(sizeof(*ifi));
	if (len < 0)
	{
		return -1;
	}

	dprintf("ifi_index %d\n", ifi->ifi_index);
	dprintf("ifi_flags 0x%x\n", ifi->ifi_flags);

	parse_rtattr(tb, IFLA_MAX, IFLA_RTA(ifi), len);
	if (tb[IFLA_IFNAME] == NULL) {
		printf("BUG: nil ifname\n");
		return -1;
	}

	if (strcmp(netdev, (char*)RTA_DATA(tb[IFLA_IFNAME]))) {
		dprintf("reseive nlmsg of %s\n", (char*)RTA_DATA(tb[IFLA_IFNAME]));
		return 1;
	}

	dprintf("netdev %s\n", netdev);

	if (n->nlmsg_type == RTM_NEWLINK){
		dprintf("Newlink %d: ", n->nlmsg_type);
		if (ifi->ifi_flags & IFF_UP)
		{
			dprintf("UP ");
			if (ifi->ifi_flags & IFF_BROADCAST)
			{
				dprintf("| BROADCAST ");
			}
			if (ifi->ifi_flags & IFF_RUNNING)
			{
				dprintf("| RUNNING ");
			}
			if (ifi->ifi_flags & IFF_MULTICAST)
			{
				dprintf("| MULTICAST ");
			}
			if (ifi->ifi_flags & IFF_LOWER_UP)
			{
				dprintf("| LOWER_UP ");
			}
			dprintf("\n");
			return 0;
		}
		else
		{
			return NEED_IFUP;
		}
	}

	return 1;
}

int32_t rtnl_listen(int32_t fd, char *netdev)
{
	ssize_t status;
	struct nlmsghdr *h;
	struct sockaddr_nl nladdr;
	struct iovec iov;
	struct msghdr msg = {
		.msg_name = &nladdr,
		.msg_namelen = sizeof(nladdr),
		.msg_iov = &iov,
		.msg_iovlen = 1,
	};
	char   buf[8192];

	memset(&nladdr, 0, sizeof(nladdr));
	nladdr.nl_family = AF_NETLINK;
	nladdr.nl_pid = 0;
	nladdr.nl_groups = 0;

	iov.iov_base = buf;
	while (1) {
		iov.iov_len = sizeof(buf);
		status = recvmsg(fd, &msg, 0);

		if (status < 0) {
			if (errno == EINTR)
			{
				continue;
			}
			perror("OVERRUN");
			continue;
		}
		if (status == 0) {
			fprintf(stderr, "EOF on netlink\n");
			return -1;
		}
		if (msg.msg_namelen != sizeof(nladdr)) {
			fprintf(stderr, "Sender address length == %d\n", msg.msg_namelen);
			return 1;
		}
		for (h = (struct nlmsghdr*)buf; status >= sizeof(*h); ) {
			int32_t ret;
			int32_t len = h->nlmsg_len;
			int32_t l = len - sizeof(*h);

			if ((l<0) || (len>status)) {
				if (msg.msg_flags & MSG_TRUNC) {
					fprintf(stderr, "Truncated message\n");
					return -1;
				}
				fprintf(stderr, "!!!malformed message: len=%d\n", len);
				return 1;
			}

			/* this time, RTM_NEWLINK only */
			if (h->nlmsg_type == RTM_NEWLINK) {
				ret = print_linkinfo(&nladdr, h, netdev);
				/* specified netdev become the IFF_UP state now */
				if (ret == 0) {
					dprintf("RETURN\n");
					return 0;
				}
				/* need to ifup */
				if (ret == NEED_IFUP) {
					if (do_ifup(netdev))
					{
						return -1;
					}
					dprintf("do ifup %s\n", netdev);
				}
			}

			status -= NLMSG_ALIGN(len);
			h = (struct nlmsghdr*)((char*)h + NLMSG_ALIGN(len));
		}
		if (msg.msg_flags & MSG_TRUNC) {
			fprintf(stderr, "Message truncated\n");
			continue;
		}
		if (status) {
			fprintf(stderr, "!!!Remnant of size %zd\n", status);
			return 1;
		}
	}
}


#define READ_BUF_LEN (256)
static const char* const LOOPBACK_IFNAME = "lo";

static int32_t get_ifname_by_hwaddr(const char* hwaddr_str, char* ifname)
{
	char hwaddr[HWADDR_BYTE_LEN] = {};
	struct ifreq ifr;
	int32_t fd;
	FILE* fh;
	char buf[READ_BUF_LEN];
	int i;

	if (strlen(hwaddr_str) != HWADDR_STR_LEN) {
		fprintf(stderr, "Illegal hwaddr length\n");
		return -1;
	}
	for (i = 0; i < HWADDR_BYTE_LEN; i++) {
		char tmp[3] = {};
		tmp[0] = hwaddr_str[i*3];
		tmp[1] = hwaddr_str[i*3+1];
		hwaddr[i] = strtol(tmp, NULL, 16) & 0x000000FF;
	}

	if ((fd = socket(PF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("socket");
		return -1;
	}

	int err = 0;
	fh = fopen(PATH_PROCNET_DEV, "r");
	if (!fh) {
		fprintf(stderr, "Cannot open %s. %s\n",
			PATH_PROCNET_DEV, strerror(errno));
		err = -1;
		goto fopen_err;
	}

	/* eat 2lines */
	fgets(buf, READ_BUF_LEN, fh);
	fgets(buf, READ_BUF_LEN, fh);

	while (fgets(buf, READ_BUF_LEN, fh)) {
		get_ifname(buf, ifr.ifr_name);
		if (strcmp(LOOPBACK_IFNAME, &(ifr.ifr_name[0]))) {
			dprintf("found if. name:%s\n", ifr.ifr_name);
			if (ioctl(fd, SIOCGIFHWADDR, &ifr) < 0) {
				perror("SIOCGIFHWADDR");
				err = -1;
				goto ioctr_err;
			}
			if(memcmp(hwaddr, ifr.ifr_hwaddr.sa_data, HWADDR_BYTE_LEN) == 0) {
				strncpy(ifname, ifr.ifr_name, IFNAMSIZ);
				if (ifname[IFNAMSIZ - 1] != '\0')
					ifname[IFNAMSIZ - 1] = '\0';
				break;
			}
		}
	}
	if (ferror(fh)) {
		perror(PATH_PROCNET_DEV);
		err = -1;
	}

ioctr_err:
	fclose(fh);
fopen_err:
	close(fd);
	return err;
}


static const char *get_ifname(const char *line, char *ifname)
{
	while (isspace(*line))
		line++;
	while (*line) {
		if (isspace(*line))
			break;
		if (*line == ':') {
			const char *tmp_cur = line++;
			while (*line && isdigit(*line))
				line++;
			if (*line == ':') {
				line = tmp_cur;
				*ifname++ = *line++;
				while (*line && isdigit(*line)) {
					*ifname++ = *line++;
				}
			} else {
				line = tmp_cur;
			}
			line++;
			break;
		}
		*ifname++ = *line++;
	}
	*ifname = '\0';
	return line;
}


static int32_t set_netmask(int32_t skfd, const char *netdev, in_addr_t ip)
{
	struct ifreq ifr;
	struct sockaddr_in sin;

	memset(&ifr, 0, sizeof(ifr));
	strncpy(ifr.ifr_name, netdev, IFNAMSIZ-1);

	memset(&sin, 0, sizeof(struct sockaddr));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = ip;
	memcpy(&ifr.ifr_netmask, &sin, sizeof(struct sockaddr));

	dprintf("netmask %s\n", inet_ntoa(*(struct in_addr *)&(sin.sin_addr.s_addr)));

	if (ioctl(skfd, SIOCSIFNETMASK, &ifr) < 0) {
		perror("SIOCSIFNETMASK");
		return -1;
	}
	return 0;
}


static int32_t set_mtu(int32_t skfd, const char *netdev, int mtu)
{
	struct ifreq ifr;

	memset(&ifr, 0, sizeof(ifr));
	strncpy(ifr.ifr_name, netdev, IFNAMSIZ-1);

	ifr.ifr_mtu = mtu;
	dprintf("mtu %d\n", mtu);

	if (ioctl(skfd, SIOCSIFMTU, &ifr) < 0) {
		perror("SIOCSIFMTU");
		return -1;
	}
	return 0;
}

static int32_t set_gw(int32_t skfd, char *netdev, in_addr_t gw)
{
	struct rtentry rt;
	struct sockaddr_in dst;
	struct sockaddr_in genmask;
	struct sockaddr_in gateway;

	/* Clean out the RTREQ structure. */
	memset((char *) &rt, 0, sizeof(struct rtentry));

	memset(&dst, 0, sizeof(struct sockaddr));
	dst.sin_family = AF_INET;
	dst.sin_addr.s_addr = INADDR_ANY;
	memcpy(&rt.rt_dst, &dst, sizeof(struct sockaddr));

	memset(&genmask, 0, sizeof(struct sockaddr));
	genmask.sin_family = AF_INET;
	/* Default is special, meaning 0.0.0.0. */
	genmask.sin_addr.s_addr = INADDR_ANY;
	memcpy(&rt.rt_genmask, &genmask, sizeof(struct sockaddr));

	memset(&gateway, 0, sizeof(struct sockaddr));
	gateway.sin_family = AF_INET;
	gateway.sin_addr.s_addr = gw;
	memcpy(&rt.rt_gateway, &gateway, sizeof(struct sockaddr));

	dprintf("gateway %s\n", inet_ntoa(*(struct in_addr *)&(gateway.sin_addr.s_addr)));
	rt.rt_dev = netdev;
	rt.rt_flags = RTF_UP | RTF_GATEWAY;

	if (ioctl(skfd, SIOCADDRT, &rt) < 0) {
		perror("SIOCADDRT");
		return -1;
	}
	return 0;
}

static int32_t do_changename(const char *dev, const char *newdev)
{
	struct ifreq ifr;
	int32_t fd;
	int32_t err;

	strncpy(ifr.ifr_name, dev, IFNAMSIZ);
	strncpy(ifr.ifr_newname, newdev, IFNAMSIZ);

	/* fd = get_ctl_fd(); */
	if ((fd = socket(PF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("socket");
		return -1;
	}
/*
	if (fd < 0)
	return -1; 
*/
	err = ioctl(fd, SIOCSIFNAME, &ifr);
	if (err) {
		perror("SIOCSIFNAME");
		close(fd);
		return -1;
	}
	close(fd);
	return err;
}

static void usage(void)
{
	fprintf(stderr,
"Usage: netdev-manager [-dvh] -i device [-c name] [-a addr] [-n netmask]\n"
"            [-m mtu] [-g gateway]\n"
"   or: netdev-manager [-dvh] -l hwaddr [-c name] [-a addr] [-n netmask]\n"
"            [-m mtu] [-g gateway]\n");
}

int32_t main(int32_t argc, char *argv[])
{
	char netdev[IFNAMSIZ] = {};
	int mtu = 1500;
	int32_t fd;
	int32_t ret = 0;
	int ch;
	int32_t skfd = -1;
	struct sockaddr_in addr;
	struct sockaddr_in netmask;
	struct sockaddr_in gateway;
	char *newname = NULL;
	int addr_setting = 0;
	int nm_setting = 0;
	int mtu_setting = 0;
	int gw_setting = 0;

	memset(&addr, 0, sizeof(struct sockaddr_in));
	memset(&netmask, 0, sizeof(struct sockaddr_in));
	memset(&gateway, 0, sizeof(struct sockaddr_in));

	while ((ch = getopt(argc, argv, "i:l:a:c:n:m:g:dvh")) != EOF) {
		switch(ch) {
		case 'i':
			if (netdev[0]) {
				usage();
				return 1;
			}
			strncpy(netdev, optarg, IFNAMSIZ);
			if (netdev[IFNAMSIZ - 1] != '\0')
				netdev[IFNAMSIZ - 1] = '\0';
			break;
		case 'l':
			if (netdev[0]) {
				usage();
				return 1;
			}
			if (get_ifname_by_hwaddr(optarg, netdev) < 0) {
				fprintf(stderr, "HWAddr error\n");
				return -1;
			}
			break;
		case 'c':
			newname = optarg;
			break;
		case 'a':
			if (inet_pton(AF_INET, optarg, &addr.sin_addr) <= 0) {
				fprintf(stderr, "inet_pton ip addr error\n");
				return -1;
			}
			addr_setting = 1;
			break;
		case 'n':
			if (inet_pton(AF_INET, optarg, &netmask.sin_addr) <= 0) {
				fprintf(stderr, "inet_pton netmask error\n");
				return -1;
			}
			nm_setting = 1;
			break;
		case 'm':
			mtu = atoi(optarg);
			mtu_setting = 1;
			break;
		case 'g':
			if (inet_pton(AF_INET, optarg, &gateway.sin_addr) <= 0) {
				fprintf(stderr, "inet_pton gw addr error\n");
				return -1;
			}
			gw_setting = 1;
			break;
		case 'd':
			debug = 1;
			break;
		case 'v':
			/* version 0.4 */
			printf("version 0.4\n");
			return 0;
			break;
		case 'h':
			break;
		case '?':
		default:
			usage();
			return 0;
			break;
		}
	}

	if ( !netdev[0] ) {
		usage();
		return 1;
	}

	dprintf("monitor net device name: %s\n", netdev);

	/* bind rtnetlink socket */
	fd = rtnl_open();
	if (fd < 0){
		perror("rtnl_open");
		return 1;
	}

	/* for get the current state */
	if (rtnl_dump_request(fd) < 0) {
		perror("Cannot send dump request");
		ret = 1;
		goto err;
	}

	if (newname && strcmp(&netdev[0], newname)) {
		if (do_changename(netdev, newname) < 0) {
			ret = 1;
			goto err;
		}
		strncpy(netdev, newname, IFNAMSIZ);
		if (netdev[IFNAMSIZ - 1] != '\0')
			netdev[IFNAMSIZ - 1] = '\0';
	}

	rtnl_listen(fd, netdev);

	/* Create a channel to the NET kernel. */
	if ((skfd = socket(PF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("socket");
		ret = 1;
		goto err;
	}

	if (addr_setting) {
		ret = set_addr(skfd, netdev, addr.sin_addr.s_addr);
		if (ret)
		{
			goto set_err;
		}
	}

	if (nm_setting) {
		ret = set_netmask(skfd, netdev, netmask.sin_addr.s_addr);
		if (ret)
		{
			goto set_err;
		}
	}

	if (mtu_setting) {
		ret = set_mtu(skfd, netdev, mtu);
		if (ret)
		{
			goto set_err;
		}
	}

	if (gw_setting) {
		ret = set_gw(skfd, netdev, gateway.sin_addr.s_addr);
		if (ret)
		{
			goto set_err;
		}
	}

set_err:
	close(skfd);
err:
	close(fd);

	return ret;
}
