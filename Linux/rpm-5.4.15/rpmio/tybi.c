static int _ybi_debug = 0;

#include "system.h"

#include <rpmio.h>
#include <poptIO.h>

#include "debug.h"

/* https://github.com/Yubico/yubikey-val/wiki/ValidationProtocolV20 */

enum {
    RPMYBI_OK				=  0,
    RPMYBI_BAD_OTP			= -1,
    RPMYBI_REPLAYED_OTP			= -2,
    RPMYBI_BAD_SIGNATURE		= -3,
    RPMYBI_MISSING_PARAMETER		= -4,
    RPMYBI_NO_SUCH_CLIENT		= -5,
    RPMYBI_OPERATION_NOT_ALLOWED	= -6,
    RPMYBI_BACKEND_ERROR		= -7,
    RPMYBI_NOT_ENOUGH_ANSWERS		= -8,
    RPMYBI_REPLAYED_REQUEST		= -9
};

static char * _ybi_host = "api.yubico.com";
static char * _ybi_path = "wsapi/2.0/verify";

static char * _ybi_uri = "http://%{_ybi_host}/%{_ybi_path}";

static char * _ybi_id = "87";
static char * _ybi_otp = "vvvvvvcucrlcietctckflvnncdgckubflugerlnr";
static char * _ybi_h = "???";
static char * _ybi_timestamp = "1";
static char * _ybi_nonce = "askjdnkajsndjkasndkjsnad";
static char * _ybi_sl = "50";
static char * _ybi_timeout = "8";

/*==============================================================*/
static const char * rpmybiEscape(const char * uri)
{
    /* essentially curl_escape() */
    /* XXX doubles as hex encode string */
    static char ok[] =
	"0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    const char * s;
    char * t, *te;
    size_t nb = 0;

    for (s = uri; *s; s++)
	nb += (strchr(ok, *s) == NULL ? 4 : 1);

    te = t = (char *) xmalloc(nb + 1);
    for (s = uri; *s; s++) {
	if (strchr(ok, *s) == NULL) {
	    *te++ = '%';	/* XXX extra '%' macro expansion escaping. */
	    *te++ = '%';
	    *te++ = ok[(*s >> 4) & 0x0f];
	    *te++ = ok[(*s     ) & 0x0f];
	} else
	    *te++ = *s;
    }
    *te = '\0';
    return t;
}

/*==============================================================*/
static int doit(void)
{
    const char * _parms = NULL;
    const char * req = NULL;
    const char * uri = NULL;
    rpmiob iob = NULL;
    int rc = -1;	/* assume failure */
    int xx;

    /* Read a OTP from the yubikey. */

    /* Construct sorted K=V parameter list. */
    _parms = rpmExpand(
	"id=",		_ybi_id,
	"&nonce=",	_ybi_nonce,
	"&otp=",	_ybi_otp,
	"&sl=",		_ybi_sl,
	"&timeout=",	_ybi_timeout,
	NULL);

    /* Calculate HMAC-SHA1 (using API key salt) across parameters */

    /* Convert the signature to base64. */

    /* Construct a HTTP GET v2.0 validation request. */
    req = rpmExpand(_ybi_uri, "?", _parms, NULL);
    if (req == NULL)
	goto exit;
#ifdef	NOTYET
    /* Ensure the URI is properly escaped. */
    uri = rpmybiEscape(req);
    if (uri == NULL)
	goto exit;
#else
    uri = xstrdup(req);
#endif
fprintf(stderr, "*** Fopen(%s)\n", uri);

    /* Send the request and read the response. */
    xx = rpmiobSlurp(uri, &iob);
    if (xx || iob == NULL)
	goto exit;
fprintf(stderr, "*** Fread\n%s", rpmiobStr(iob));

    /* Parse the response. */

exit:
    iob = rpmiobFree(iob);
    req = _free(req);
    uri = _free(uri);
    return rc;
}

static struct poptOption rpmasnOptionsTable[] = {

 { "host", 'H',	POPT_ARG_STRING|POPT_ARGFLAG_SHOW_DEFAULT,	&_ybi_host, 0,
	N_("Set the Yubico HOST"),	N_("HOST") },
 { "path", 'P',	POPT_ARG_STRING|POPT_ARGFLAG_SHOW_DEFAULT,	&_ybi_path, 0,
	N_("Set the Yubico PATH"),	N_("PATH") },

 { "id", 'I',	POPT_ARG_STRING|POPT_ARGFLAG_SHOW_DEFAULT,	&_ybi_id, 0,
	N_("Set ID"),	N_("ID") },
 { "nonce", 'N',POPT_ARG_STRING|POPT_ARGFLAG_SHOW_DEFAULT,	&_ybi_nonce, 0,
	N_("Set NONCE"),	N_("NONCE") },
 { "otp", 'O',	POPT_ARG_STRING|POPT_ARGFLAG_SHOW_DEFAULT,	&_ybi_otp, 0,
	N_("Set OTP"),	N_("OTP") },
 { "sl", 'L',	POPT_ARG_STRING|POPT_ARGFLAG_SHOW_DEFAULT,	&_ybi_sl, 0,
	N_("Set SL (sync requirements"),	N_("{0-100|fast|secure}") },
 { "timeout", 'T',POPT_ARG_STRING|POPT_ARGFLAG_SHOW_DEFAULT,	&_ybi_timeout, 0,
	N_("Set TIMEOUT in SECS"),	N_("SECS") },
 { "timestamp", 'S',	POPT_ARG_STRING|POPT_ARGFLAG_SHOW_DEFAULT, &_ybi_timestamp, 0,
	N_("Timestamp the reply?"),	N_("{0|1}") },

 { NULL, '\0', POPT_ARG_INCLUDE_TABLE, rpmioAllPoptTable, 0,
	N_(" Common options for all rpmio executables:"), NULL },

  POPT_AUTOALIAS
  POPT_AUTOHELP

  { NULL, (char)-1, POPT_ARG_INCLUDE_TABLE, NULL, 0,
        N_("\
"), NULL },

  POPT_TABLEEND
};

int
main(int argc, char *argv[])
{
    poptContext con = rpmioInit(argc, argv, rpmasnOptionsTable);
    ARGV_t av = poptGetArgs(con);
    int ac = argvCount(av);;
    int ec = -1;	/* assume failure */

    /* XXX no macros are loaded if using poptIO. */
    addMacro(NULL, "_ybi_host",		NULL, _ybi_host, -1);
    addMacro(NULL, "_ybi_path",		NULL, _ybi_path, -1);

    ec = doit();

    con = rpmioFini(con);

    return ec;
}
