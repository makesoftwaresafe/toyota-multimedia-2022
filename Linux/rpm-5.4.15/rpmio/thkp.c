#include "system.h"

#define	_RPMHKP_INTERNAL
#include <rpmhkp.h>
#include <rpmlog.h>

#include <poptIO.h>

#include "debug.h"

extern int _rpmhkp_spew;
static int _rpmhkp_stats;

static const char * _hkp_keyserver;
/* XXX the "0x" is sometimes in macro and sometimes in keyname. */
static const char * _hkp_keyserver_query =
	"hkp://%{_hkp_keyserver}/pks/lookup?op=get&search=";

static const char * _keyids[] = {
    "0xfeedfacedeadbeef",	/* NOTFOUND */
    "0x58e727c4c621be0f",	/* REVOKE */
    "0x5D385582001AE622",	/* REVOKE */
    "0xa520e8f1cba29bf9",	/* V3 RSA (V2 revocations) */
    "0x9AC53D4D",		/* V3 (unsupported) */
    "0x7AD0BECB",		/* bad */
    "0x7C611479",		/* hash fail, phptoid */
    "0x1CFC22F3363DEAE3",	/* XXX DSASHA224/DSA-SHA256 */
    "0xb873641b2039b291",	/* Jeff Johnson (ARS N3NPQ) <jbj@redhat.com> */
    "Jeff Johnson (ARS N3NPQ) <jbj@redhat.com>",	/* 0xb873641b2039b291 */
    "Russell Coker <russell@coker.com.au>",	/* 0xc2b079fcf5c75256 */
    "0xD5CA9B04F2C423BC",	/* Stefano Zacchiroli <zack@pps.jussieu.fr> */
    "0xBC916AF40D001429",	/* Enrico Scholz <enrico.scholz@sigma-chemnitz.de> */
    "0x6bddfe8e54a2acf1",	/* Warren Togami (Linux) <warren@togami.com> */
    "Fedora Project (Test Software) <rawhide@redhat.com>", /* 0xda84cbd430c9ecf8 */
    "Fedora Linux (RPMS) <security@fedora.us>",	/* 0x29d5ba248df56d05 */
    "Fedora Project automated build signing key (2003) <rawhide@redhat.com>", /* 0xe1385d4e1cddbca9 */
    "Red Hat, Inc. automated build signing key (2003) <rawhide@redhat.com>", /* 0x94cd5742e418e3aa */
    "0xb44269d04f2a6fd2",	/* Fedora Project <fedora@redhat.com> */
    "0x219180cddb42a60e",	/* Red Hat, Inc <security@redhat.com> */
    "0xfd372689897da07a",	/* Red Hat, Inc. (Beta Test Software) <rawhide@redhat.com> */
/* --- RHEL6 */
    "0x37017186",	/* Red Hat, Inc. (release key) <security@redhat.com> */
    "Red Hat, Inc. (RHX key) <rhx-support@redhat.com>",		/* 0x42193e6b */
    "Red Hat, Inc. (Beta Test Software) <rawhide@redhat.com>",	/* 0x897da07a */
    "Red Hat, Inc <security@redhat.com>",			/* 0xdb42a60e */
    "Red Hat, Inc. (beta key 2) <security@redhat.com>",		/* 0xf21541eb */
    "Red Hat, Inc. (release key 2) <security@redhat.com>",	/* 0xfd431d51 */
/* --- Fedorable */
    "Fedora (11) <fedora@fedoraproject.org>",	/* 0xd22e77f2 */
    "0x57bbccba",	/* Fedora (12) <fedora@fedoraproject.org> */
    "0xe8e40fde",	/* Fedora (13) <fedora@fedoraproject.org> */
/* --- IDMS 2010-05-22 */
    "0x1edca302",	/* IDMS Linux (Trunk) <gpgkeys@idms-linux.org> */
/* --- CentOS 5.4 */
    "0xe8562897",	/* CentOS-5 Key (CentOS 5 Official Signing Key) <centos-5-key@centos.org> */
/* --- Mandriva */
    "0x70771FF3",	/* Mandriva Linux <mandriva@mandriva.com> */
    "0x22458A98",	/* Mandriva Security Team <security@mandriva.com> */
/* --- Caixa Magica */
    "0xC45A42F1",	/* Caixa Magica Repository <geral@caixamagica.pt> */
/* --- rpmforge */
    "0x6B8D79E6",	/* Dag Wieers (Dag Apt Repository v1.0) <dag@wieers.com> */
    "0x1AA78495",	/* Dries Verachtert <dries@ulyssis.org> */
    "0xE42D547B",	/* Matthias Saou (Thias) <matthias.saou@est.une.marmotte.net> */
/* --- JPackage */
    "0xC431416D",	/* JPackage Project (JPP Official Keys) <jpackage@zarb.org> */
    NULL,
};

/*==============================================================*/
static const char * rc2str(rpmRC rc)
{
    switch (rc) {
    case RPMRC_OK:		return("OK");		break;
    case RPMRC_NOTFOUND:	return("NOTFOUND");	break;
    case RPMRC_FAIL:		return("BAD");		break;
    case RPMRC_NOTTRUSTED:	return("NOTTRUSTED");	break;
    case RPMRC_NOKEY:		return("NOKEY");	break;
    case RPMRC_NOSIG:		return("NOSIG");	break;
    }
    return("UNKNOWN");
}

static rpmRC rpmhkpReadKeys(const char ** keyids)
{
    const char ** kip;
    rpmRC rc;
    int ec = 0;

    for (kip = keyids; *kip; kip++) {
	if (rpmIsVerbose())
	    fprintf(stderr, "=============== %s\n", *kip);
	rc = rpmhkpValidate(NULL, *kip);	/* XXX 0 on success */
	if (!rpmIsVerbose())
	    fprintf(stdout, "%-8s\t%s\n", rc2str(rc), *kip);
	if (rc)
	    ec++;
    }

    return ec;
}

static struct poptOption optionsTable[] = {

 { "spew", '\0', POPT_ARG_VAL|POPT_ARGFLAG_DOC_HIDDEN,	&_rpmhkp_spew, -1,
	NULL, NULL },
 { "stats", '\0', POPT_ARG_VAL|POPT_ARGFLAG_DOC_HIDDEN,	&_rpmhkp_stats, -1,
	NULL, NULL },
 { "host", 'H', POPT_ARG_STRING,			&_hkp_keyserver, 0,
	N_("Set the SKS KEYSERVER to use"), N_("KEYSERVER") },

 { NULL, '\0', POPT_ARG_INCLUDE_TABLE, rpmioAllPoptTable, 0,
        N_("Common options:"), NULL },

  POPT_AUTOHELP
  POPT_TABLEEND
};

int
main(int argc, char *argv[])
{
    poptContext con = rpmioInit(argc, argv, optionsTable);
    ARGV_t av = poptGetArgs(con);
    int ac = argvCount(av);;
    const char ** keyids = NULL;
    int ec = 0;
    int xx;

_rpmhkp_lvl = RPMLOG_INFO;	/* XXX default is RPMLOG_DEBUG */

    if (_hkp_keyserver == NULL)
	_hkp_keyserver = xstrdup("keys.rpm5.org");
    if (_rpmhkp_spew)
	_rpmhkp_debug = -1;

    /* XXX no macros are loaded if using poptIO. */
    addMacro(NULL, "_hkp_keyserver",		NULL, _hkp_keyserver, -1);
    addMacro(NULL, "_hkp_keyserver_query",	NULL, _hkp_keyserver_query, -1);

    if (ac == 0) {
	xx = argvAppend(&keyids, _keyids);
    } else {
	int gotstdin = 0;
	int i;
	for (i = 0; i < ac; i++) {
	    if (strcmp(av[i], "-")) {
		xx = argvAdd(&keyids, (ARGstr_t)av[i]);
		continue;
	    }
	    if (gotstdin)
		continue;
	    gotstdin++;
	    if (argvFgets(&keyids, NULL))
		goto exit;
	}
    }

    ec = rpmhkpReadKeys(keyids);

    if (_rpmhkp_stats)
	_rpmhkpPrintStats(NULL);

exit:
    keyids = argvFree(keyids);
    _hkp_keyserver = _free(_hkp_keyserver);

/*@i@*/ urlFreeCache();

    con = rpmioFini(con);

    return ec;
}
