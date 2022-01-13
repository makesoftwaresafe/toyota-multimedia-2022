#include "system.h"

#include <rpmio.h>
#include <poptIO.h>

#include "debug.h"

#define	_CFB(n)	((1U << (n)) | 0x40000000)
#define CF_ISSET(_FLAG) ((curlFlags & ((CURL_FLAGS_##_FLAG) & ~0x40000000)) != CURL_FLAGS_NONE)

enum curlFlags_e {
    CURL_FLAGS_NONE		= 0,

    CURL_FLAGS_APPEND		= _CFB( 0), /*!< -a,--append ... */
    CURL_FLAGS_ASCII		= _CFB( 1), /*!< -B,--use-ascii ... */
    CURL_FLAGS_COMPRESSED	= _CFB( 2), /*!<    --compressed ... */
    CURL_FLAGS_CREATEDIRS	= _CFB( 3), /*!<    --create-dirs ... */
    CURL_FLAGS_CRLF		= _CFB( 4), /*!<    --crlf ... */
};

/*@unchecked@*/
static enum curlFlags_e curlFlags = CURL_FLAGS_NONE;

enum curlAuth_e {
    CURL_AUTH_ANY		= 0,
    CURL_AUTH_BASIC		= 1,
    CURL_AUTH_DIGEST		= 2,
    CURL_AUTH_NTLM		= 3,
    CURL_AUTH_NEGOTIATE		= 4,
};

/*@unchecked@*/
static enum curlAuth_e curlAuth = CURL_AUTH_ANY;
/*@unchecked@*/
static enum curlAuth_e curlProxyAuth = CURL_AUTH_ANY;

static const char * _user_agent;
static const char * _cacert;
static const char * _capath;
static const char * _cert;
static const char * _cert_type;
static const char * _ciphers;
static const char * _cookie_jar;
static int _connect_timeout;
static long long _connect_at;

static int _max_time;

/**
 */
static void rpmcurlArgCallback(poptContext con,
                /*@unused@*/ enum poptCallbackReason reason,
                const struct poptOption * opt, const char * arg,
                /*@unused@*/ void * data)
	/*@*/
{
    /* XXX avoid accidental collisions with POPT_BIT_SET for flags */
    if (opt->arg == NULL)
    switch (opt->val) {
    default:
	fprintf(stderr, _("%s: Unknown option -%c\n"), __progname, opt->val);
	poptPrintUsage(con, stderr, 0);
	/*@-exitarg@*/ exit(2); /*@=exitarg@*/
	/*@notreached@*/ break;
    }
}

/*==============================================================*/

#define	POPTCURL_XXX	0

/*@unchecked@*/ /*@observer@*/
static struct poptOption optionsTable[] = {
/*@-type@*/ /* FIX: cast? */
 { NULL, '\0', POPT_ARG_CALLBACK | POPT_CBFLAG_INC_DATA | POPT_CBFLAG_CONTINUE,
        rpmcurlArgCallback, 0, NULL, NULL },
/*@=type@*/

  { "anyauth", '\0', POPT_ARG_VAL,	&curlAuth, CURL_AUTH_ANY,
	N_("Pick \"any\" authentication method (H)"), NULL },
  { "append", 'a', POPT_ARGFLAG_TOGGLE,	&curlFlags, CURL_FLAGS_APPEND,
	N_("Append to target file when uploading (F)"), NULL },
  { "basic", '\0', POPT_ARG_VAL,	&curlAuth, CURL_AUTH_BASIC,
	N_("Use HTTP Basic Authentication (H)"), NULL },

  { "cacert", '\0', POPT_ARG_STRING,	&_cacert, 0,
	N_("CA certificate to verify peer against (SSL)"), N_("<file>") },
  { "capath", '\0', POPT_ARG_STRING,	&_capath, 0,
	N_("CA directory to verify peer against (SSL)"), N_("<directory>") },
  { "cert", 'E', POPT_ARG_STRING,	&_cert, 0,
	N_("Client certificate file and password (SSL)"), N_("<cert[:passwd]>") },
  { "cert-type", '\0', POPT_ARG_STRING,	&_cert_type, 0,
	N_("Certificate file type (DER/PEM/ENG) (SSL)"), N_("<type>") },
/* XXX ARGV */
  { "ciphers", '\0', POPT_ARG_STRING,	&_ciphers, 0,
	N_("<list> SSL ciphers to use (SSL)"), NULL },
  { "compressed", '\0', POPT_ARGFLAG_TOGGLE,&curlFlags, CURL_FLAGS_COMPRESSED,
	N_("Request compressed response (using deflate or gzip)"), NULL },
  { "config", 'K', POPT_ARG_NONE,	NULL, 'K',
	N_("Specify which config file to read"), NULL },
  { "connect-timeout", '\0', POPT_ARG_INT,	&_connect_timeout, 0,
	N_("Maximum time allowed for connection"), N_("<seconds>") },
/* XXX grrr "-C -" prevents LONGLONG */
  { "continue-at", 'C', POPT_ARG_LONGLONG,	&_connect_at, 0,
	N_("Resumed transfer offset"), N_("<offset>") },
/* XXX ARGV */
  { "cookie", 'b', POPT_ARG_NONE,	NULL, 'b',
	N_("Cookie string or file to read cookies from (H)"), N_("<name=string/file>") },
  { "cookie-jar", 'c', POPT_ARG_STRING,	&_cookie_jar, 0,
	N_("Write cookies to this file after operation (H)"), N_("<file>") },
  { "create-dirs", '\0', POPT_ARGFLAG_TOGGLE,	&curlFlags, CURL_FLAGS_CREATEDIRS,
	N_("Create necessary local directory hierarchy"), NULL },
  { "crlf", '\0', POPT_BIT_SET,	&curlFlags, CURL_FLAGS_CRLF,
	N_("Convert LF to CRLF in upload"), NULL },

  { "data", 'd', POPT_ARG_NONE,	NULL, 'd',
	N_("   HTTP POST data (H)"), N_("<data>") },
  { "data-ascii", '\0', POPT_ARG_NONE,	NULL, POPTCURL_XXX,
	N_("  HTTP POST ASCII data (H)"), N_("<data>") },
  { "data-binary", '\0', POPT_ARG_NONE,	NULL, POPTCURL_XXX,
	N_(" HTTP POST binary data (H)"), N_("<data>") },
  { "data-urlencode", '\0', POPT_ARG_NONE,	NULL, POPTCURL_XXX,
	N_("<name=data/name@filename> HTTP POST data url encoded (H)"), NULL },

  { "digest", '\0', POPT_ARG_VAL,	&curlAuth, CURL_AUTH_DIGEST,
	N_("Use HTTP Digest Authentication (H)"), NULL },
  { "disable-eprt", '\0', POPT_ARG_NONE,	NULL, POPTCURL_XXX,
	N_("Inhibit using EPRT or LPRT (F)"), NULL },
  { "disable-epsv", '\0', POPT_ARG_NONE,	NULL, POPTCURL_XXX,
	N_("Inhibit using EPSV (F)"), NULL },
  { "dump-header", 'D', POPT_ARG_NONE,	NULL, 'D',
	N_("Write the headers to this file"), N_("<file>") },
  { "egd-file", '\0', POPT_ARG_NONE,	NULL, POPTCURL_XXX,
	N_("EGD socket path for random data (SSL)"), N_("<file>") },
  { "engine", '\0', POPT_ARG_NONE,	NULL, POPTCURL_XXX,
	N_("<eng>  Crypto engine to use (SSL). \"--engine list\" for list"), NULL },
  { "fail", 'f', POPT_ARG_NONE,	NULL, 'f',
	N_("Fail silently (no output at all) on HTTP errors (H)"), NULL },
  { "form", 'F', POPT_ARG_NONE,	NULL, 'F',
	N_("<name=content> Specify HTTP multipart POST data (H)"), NULL },
  { "form-string", '\0', POPT_ARG_NONE,	NULL, POPTCURL_XXX,
	N_("<name=string> Specify HTTP multipart POST data (H)"), NULL },
  { "ftp-account", '\0', POPT_ARG_NONE,	NULL, POPTCURL_XXX,
	N_("<data> Account data to send when requested by server (F)"), NULL },
  { "ftp-alternative-to-user", '\0', POPT_ARG_NONE,	NULL, POPTCURL_XXX,
	N_("String to replace \"USER [name]\" (F)"), NULL },
  { "ftp-create-dirs", '\0', POPT_ARG_NONE,	NULL, POPTCURL_XXX,
	N_("Create the remote dirs if not present (F)"), NULL },
  { "ftp-method", '\0', POPT_ARG_NONE,	NULL, POPTCURL_XXX,
	N_("[multicwd/nocwd/singlecwd] Control CWD usage (F)"), NULL },
  { "ftp-pasv", '\0', POPT_ARG_NONE,	NULL, POPTCURL_XXX,
	N_("Use PASV/EPSV instead of PORT (F)"), NULL },
  { "ftp-port", 'P', POPT_ARG_NONE,	NULL, 'P',
	N_("<address> Use PORT with address instead of PASV (F)"), NULL },
  { "ftp-skip-pasv-ip", '\0', POPT_ARG_NONE,	NULL, POPTCURL_XXX,
	N_("Skip the IP address for PASV (F)"), NULL },
  { "ftp-ssl", '\0', POPT_ARG_NONE,	NULL, POPTCURL_XXX,
	N_("Try SSL/TLS for ftp transfer (F)"), NULL },
  { "ftp-ssl-ccc", '\0', POPT_ARG_NONE,	NULL, POPTCURL_XXX,
	N_("Send CCC after authenticating (F)"), NULL },
  { "ftp-ssl-ccc-mode", '\0', POPT_ARG_NONE,	NULL, POPTCURL_XXX,
	N_("[active/passive] Set CCC mode (F)"), NULL },
  { "ftp-ssl-control", '\0', POPT_ARG_NONE,	NULL, POPTCURL_XXX,
	N_("Require SSL/TLS for ftp login, clear for transfer (F)"), NULL },
  { "ftp-ssl-reqd", '\0', POPT_ARG_NONE,	NULL, POPTCURL_XXX,
	N_("Require SSL/TLS for ftp transfer (F)"), NULL },
  { "get", 'G', POPT_ARG_NONE,	NULL, 'G',
	N_("Send the -d data with a HTTP GET (H)"), NULL },
  { "globoff", 'g', POPT_ARG_NONE,	NULL, 'g',
	N_("Disable URL sequences and ranges using {} and []"), NULL },
  { "header", 'H', POPT_ARG_NONE,	NULL, 'H',
	N_("Custom header to pass to server (H)"), N_("<line>") },
  { "head", 'I', POPT_ARG_NONE,	NULL, 'I',
	N_("Show document info only"), NULL },
  { "help", 'h', POPT_ARG_NONE,	NULL, 'h',
	N_("This help text"), NULL },
  { "hostpubmd5", '\0', POPT_ARG_NONE,	NULL, POPTCURL_XXX,
	N_("Hex encoded MD5 string of the host public key. (SSH)"), N_("<md5>") },
  { "http1.0", '0', POPT_ARG_NONE,	NULL, '0',
	N_("Use HTTP 1.0 (H)"), NULL },
  { "ignore-content-length", '\0', POPT_ARG_NONE,	NULL, POPTCURL_XXX,
	N_("Ignore the HTTP Content-Length header"), NULL },
  { "include", 'i', POPT_ARG_NONE,	NULL, 'i',
	N_("Include protocol headers in the output (H/F)"), NULL },
  { "insecure", 'k', POPT_ARG_NONE,	NULL, 'k',
	N_("Allow connections to SSL sites without certs (H)"), NULL },
  { "interface", '\0', POPT_ARG_NONE,	NULL, POPTCURL_XXX,
	N_("Specify network interface/address to use"), N_("<interface>") },
  { "ipv4", '4', POPT_ARG_NONE,	NULL, '4',
	N_("Resolve name to IPv4 address"), NULL },
  { "ipv6", '6', POPT_ARG_NONE,	NULL, '6',
	N_("Resolve name to IPv6 address"), NULL },
  { "junk-session-cookies", 'j', POPT_ARG_NONE,	NULL, 'j',
	N_("Ignore session cookies read from file (H)"), NULL },
  { "keepalive-time", '\0', POPT_ARG_NONE,	NULL, POPTCURL_XXX,
	N_("Interval between keepalive probes"), N_("<seconds>") },
  { "key", '\0', POPT_ARG_NONE,	NULL, POPTCURL_XXX,
	N_("Private key file name (SSL/SSH)"), N_("<key>") },
  { "key-type", '\0', POPT_ARG_NONE,	NULL, POPTCURL_XXX,
	N_("Private key file type (DER/PEM/ENG) (SSL)"), N_("<type>") },
  { "krb", '\0', POPT_ARG_NONE,	NULL, POPTCURL_XXX,
	N_("Enable kerberos with specified security level (F)"), N_("level>") },
  { "libcurl", '\0', POPT_ARG_NONE,	NULL, POPTCURL_XXX,
	N_("Dump libcurl equivalent code of this command line"), N_("<file>") },
  { "limit-rate", '\0', POPT_ARG_NONE,	NULL, POPTCURL_XXX,
	N_("Limit transfer speed to this rate"), N_("<rate>") },
  { "list-only", 'l', POPT_ARG_NONE,	NULL, 'l',
	N_("List only names of an FTP directory (F)"), NULL },
  { "local-port", '\0', POPT_ARG_NONE,	NULL, POPTCURL_XXX,
	N_("Force use of these local port numbers"), N_("<num>[-num]") },
  { "location", 'L', POPT_ARG_NONE,	NULL, 'L',
	N_("Follow Location: hints (H)"), NULL },
  { "location-trusted", '\0', POPT_ARG_NONE,	NULL, POPTCURL_XXX,
	N_("Follow Location: and send auth to other hosts (H)"), NULL },
  { "manual", 'M', POPT_ARG_NONE,	NULL, 'M',
	N_("Display the full manual"), NULL },
  { "max-filesize", '\0', POPT_ARG_NONE,	NULL, POPTCURL_XXX,
	N_("Maximum file size to download (H/F)"), N_("<bytes>") },
  { "max-redirs", '\0', POPT_ARG_NONE,	NULL, POPTCURL_XXX,
	N_("Maximum number of redirects allowed (H)"), N_("<num>") },
  { "max-time", 'm', POPT_ARG_INT,	&_max_time, 0,
	N_("Maximum time allowed for the transfer"), N_("<seconds>") },
  { "negotiate", '\0', POPT_ARG_NONE,	NULL, POPTCURL_XXX,
	N_("Use HTTP Negotiate Authentication (H)"), NULL },
  { "netrc", 'n', POPT_ARG_NONE,	NULL, 'n',
	N_("Must read .netrc for user name and password"), NULL },
  { "netrc-optional", '\0', POPT_ARG_NONE,	NULL, POPTCURL_XXX,
	N_("Use either .netrc or URL; overrides -n"), NULL },
  { "no-buffer", 'N', POPT_ARG_NONE,	NULL, 'N',
	N_("Disable buffering of the output stream"), NULL },
  { "no-keepalive", '\0', POPT_ARG_NONE,	NULL, POPTCURL_XXX,
	N_("Disable keepalive use on the connection"), NULL },
  { "no-sessionid", '\0', POPT_ARG_NONE,	NULL, POPTCURL_XXX,
	N_("Disable SSL session-ID reusing (SSL)"), NULL },
  { "ntlm", '\0', POPT_ARG_VAL,	&curlAuth, CURL_AUTH_NTLM,
	N_("Use HTTP NTLM authentication (H)"), NULL },
  { "output", 'o', POPT_ARG_NONE,	NULL, 'o',
	N_("Write output to <file> instead of stdout"), N_("<file>") },
  { "pass", '\0', POPT_ARG_NONE,	NULL, POPTCURL_XXX,
	N_("Pass phrase for the private key (SSL/SSH)"), N_("<pass>") },
  { "post301", '\0', POPT_ARG_NONE,	NULL, POPTCURL_XXX,
	N_("Do not switch to GET after following a 301 redirect (H)"), NULL },
  { "progress-bar", '#', POPT_ARG_NONE,	NULL, '#',
	N_("Display transfer progress as a progress bar"), NULL },
  { "proxy", 'x', POPT_ARG_NONE,	NULL, 'x',
	N_("Use HTTP proxy on given port"), N_("<host[:port]>") },
  { "proxy-anyauth", '\0', POPT_ARG_VAL,	&curlProxyAuth, CURL_AUTH_ANY,
	N_("Pick \"any\" proxy authentication method (H)"), NULL },
  { "proxy-basic", '\0', POPT_ARG_VAL,		&curlProxyAuth, CURL_AUTH_BASIC,
	N_("Use Basic authentication on the proxy (H)"), NULL },
  { "proxy-digest", '\0', POPT_ARG_VAL,		&curlProxyAuth, CURL_AUTH_DIGEST,
	N_("Use Digest authentication on the proxy (H)"), NULL },
  { "proxy-negotiate", '\0', POPT_ARG_VAL,	&curlProxyAuth, CURL_AUTH_NEGOTIATE,
	N_("Use Negotiate authentication on the proxy (H)"), NULL },
  { "proxy-ntlm", '\0', POPT_ARG_VAL,		&curlProxyAuth, CURL_AUTH_NTLM,
	N_("Use NTLM authentication on the proxy (H)"), NULL },
  { "proxy-user", 'U', POPT_ARG_NONE,	NULL, 'U',
	N_("Set proxy user and password"), N_("<user[:password]>") },
  { "proxytunnel", 'p', POPT_ARG_NONE,	NULL, 'p',
	N_("Operate through a HTTP proxy tunnel (using CONNECT)"), NULL },
  { "pubkey", '\0', POPT_ARG_NONE,	NULL, POPTCURL_XXX,
	N_("Public key file name (SSH)"), N_("<key>") },
  { "quote", 'Q', POPT_ARG_NONE,	NULL, 'Q',
	N_("Send command(s) to server before file transfer (F/SFTP)"), N_("<cmd>") },
  { "random-file", '\0', POPT_ARG_NONE,	NULL, POPTCURL_XXX,
	N_("File for reading random data from (SSL)"), N_("<file>") },
  { "range", 'r', POPT_ARG_NONE,	NULL, 'r',
	N_("Retrieve a byte range from a HTTP/1.1 or FTP server"), N_("<range>") },
  { "raw", '\0', POPT_ARG_NONE,	NULL, POPTCURL_XXX,
	N_("Pass HTTP \"raw\", without any transfer decoding (H)"), NULL },
  { "referer", 'e', POPT_ARG_NONE,	NULL, 'e',
	N_("Referer URL (H)"), NULL },
  { "remote-name", 'O', POPT_ARG_NONE,	NULL, 'O',
	N_("Write output to a file named as the remote file"), NULL },
  { "remote-time", 'R', POPT_ARG_NONE,	NULL, 'R',
	N_("Set the remote file's time on the local output"), NULL },
  { "request", 'X', POPT_ARG_NONE,	NULL, 'X',
	N_("Specify request command to use"), N_("<command>") },
  { "retry", '\0', POPT_ARG_NONE,	NULL, POPTCURL_XXX,
	N_("Retry request <num> times if transient problems occur"), N_("<num>") },
  { "retry-delay", '\0', POPT_ARG_NONE,	NULL, POPTCURL_XXX,
	N_("When retrying, wait this many seconds between each"), N_("<seconds>") },
  { "retry-max-time", '\0', POPT_ARG_NONE,	NULL, POPTCURL_XXX,
	N_("Retry only within this period"), N_("<seconds>") },
  { "show-error", 'S', POPT_ARG_NONE,	NULL, 'S',
	N_("Show error. With -s, make curl show errors when they occur"), NULL },
  { "silent", 's', POPT_ARG_NONE,	NULL, 's',
	N_("Silent mode. Don't output anything"), NULL },
  { "socks4", '\0', POPT_ARG_NONE,	NULL, POPTCURL_XXX,
	N_("SOCKS4 proxy on given host + port"), N_("<host[:port]>") },
  { "socks4a", '\0', POPT_ARG_NONE,	NULL, POPTCURL_XXX,
	N_("SOCKS4a proxy on given host + port"), N_("<host[:port]>") },
  { "socks5", '\0', POPT_ARG_NONE,	NULL, POPTCURL_XXX,
	N_("SOCKS5 proxy on given host + port"), N_("<host[:port]>") },
  { "socks5-hostname", '\0', POPT_ARG_NONE,	NULL, POPTCURL_XXX,
	N_("SOCKS5 proxy, pass host name to proxy"), N_("<host[:port]>") },
  { "speed-limit", 'Y', POPT_ARG_NONE,	NULL, 'Y',
	N_("Stop transfer if below speed-limit for 'speed-time' secs"), NULL },
  { "speed-time", 'y', POPT_ARG_NONE,	NULL, 'y',
	N_("Time needed to trig speed-limit abort. Defaults to 30"), NULL },
  { "sslv2", '2', POPT_ARG_NONE,	NULL, '2',
	N_("Use SSLv2 (SSL)"), NULL },
  { "sslv3", '3', POPT_ARG_NONE,	NULL, '3',
	N_("Use SSLv3 (SSL)"), NULL },
  { "stderr", '\0', POPT_ARG_NONE,	NULL, POPTCURL_XXX,
	N_("Where to redirect stderr. - means stdout"), N_("<file>") },
  { "tcp-nodelay", '\0', POPT_ARG_NONE,	NULL, POPTCURL_XXX,
	N_("Use the TCP_NODELAY option"), NULL },
  { "telnet-option", 't', POPT_ARG_NONE,	NULL, 't',
	N_("Set telnet option"), N_("<OPT=val>") },
  { "time-cond", 'z', POPT_ARG_NONE,	NULL, 'z',
	N_("Transfer based on a time condition"), N_("<time>") },
  { "tlsv1", '1', POPT_ARG_NONE,	NULL, '1',
	N_("Use TLSv1 (SSL)"), NULL },
  { "trace", '\0', POPT_ARG_NONE,	NULL, POPTCURL_XXX,
	N_("Write a debug trace to the given file"), N_("<file>") },
  { "trace-ascii", '\0', POPT_ARG_NONE,	NULL, POPTCURL_XXX,
	N_("Like --trace but without the hex output"), N_("<file>") },
  { "trace-time", '\0', POPT_ARG_NONE,	NULL, POPTCURL_XXX,
	N_("Add time stamps to trace/verbose output"), NULL },
  { "upload-file", 'T', POPT_ARG_NONE,	NULL, 'T',
	N_("Transfer <file> to remote site"), N_("<file>") },
  { "url", '\0', POPT_ARG_NONE,	NULL, POPTCURL_XXX,
	N_("Set URL to work with"), N_("<URL>") },
  { "use-ascii", 'B', POPT_ARGFLAG_TOGGLE,	&curlFlags, CURL_FLAGS_ASCII,
	N_("Use ASCII/text transfer"), NULL },
  { "user", 'u', POPT_ARG_NONE,	NULL, 'u',
	N_("Set server user and password"), N_("<user[:password]>") },
  { "user-agent", 'A', POPT_ARG_STRING,	&_user_agent, 0,
	N_("User-Agent to send to server (H)"), N_("<user_agent>") },
  { "verbose", 'v', POPT_ARG_NONE,	NULL, 'v',
	N_("Make the operation more talkative"), NULL },
  { "version", 'V', POPT_ARG_NONE,	NULL, 'V',
	N_("Show version number and quit"), NULL },
  { "write-out", 'w', POPT_ARG_NONE,	NULL, 'w',
	N_("What to output after completion"), N_("<format>") },
  { NULL, 'q', POPT_ARG_NONE,	NULL, 'q',
	N_("If used as the first parameter disables .curlrc"), NULL },

  POPT_AUTOALIAS
  POPT_AUTOHELP

  { NULL, (char)-1, POPT_ARG_INCLUDE_TABLE, NULL, 0,
	N_("\
Usage: rpmcurl [options...] <url>\n\
Options: (H) means HTTP/HTTPS only, (F) means FTP only\n\
\n\
"), NULL },

  POPT_TABLEEND

};

int
main(int argc, char *argv[])
	/*@globals __assert_program_name,
		rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
	/*@modifies __assert_program_name, _rpmrepo,
		rpmGlobalMacroContext, fileSystem, internalState @*/
{
    poptContext optCon = rpmioInit(argc, argv, optionsTable);
    const char ** av = NULL;
    int ac;
    int rc = 1;		/* assume failure. */
    int i;

/*@-observertrans -readonlytrans @*/
    __progname = "rpmgenpkglist";
/*@=observertrans =readonlytrans @*/

    av = poptGetArgs(optCon);
    if (av == NULL || av[0] == NULL) {
	poptPrintUsage(optCon, stderr, 0);
	goto exit;
    }
    ac = argvCount(av);

    if (av != NULL)
    for (i = 0; i < ac; i++) {
    }
    rc = 0;

exit:
    optCon = rpmioFini(optCon);

    return rc;
}
