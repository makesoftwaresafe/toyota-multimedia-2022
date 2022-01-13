#include "system.h"

#include <rpmio.h>
#include <poptIO.h>

#include "debug.h"

/**
 */
static void rpmgpgArgCallback(poptContext con,
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

#define	POPT_XXX	0
#if !defined(POPT_BIT_TOGGLE)
#define	POPT_BIT_TOGGLE	(POPT_ARG_VAL|POPT_ARGFLAG_XOR)
#endif

/*@unchecked@*/ /*@observer@*/
static struct poptOption rpmgpgCommandsPoptTable[] = {
/*@-type@*/ /* FIX: cast? */
 { NULL, '\0', POPT_ARG_CALLBACK | POPT_CBFLAG_INC_DATA | POPT_CBFLAG_CONTINUE,
        rpmgpgArgCallback, 0, NULL, NULL },
/*@=type@*/

  { "sign", 's', POPT_ARG_NONE,	NULL, 's',
	N_("make a signature"), N_("[file]") },
  { "clearsign", '\0', POPT_ARG_NONE,	NULL, POPT_XXX,
	N_("make a clear text signature"), N_("[file]") },
  { "detach-sign", 'b', POPT_ARG_NONE,	NULL, 'b',
	N_("make a detached signature"), NULL },
  { "encrypt", 'e', POPT_ARG_NONE,	NULL, 'e',
	N_("encrypt data"), NULL },
  { "symmetric", 'c', POPT_ARG_NONE,	NULL, 'c',
	N_("encryption only with symmetric cipher"), NULL },
  { "decrypt", 'd', POPT_ARG_NONE,	NULL, 'd',
	N_("decrypt data (default)"), NULL },
  { "verify", '\0', POPT_ARG_NONE,	NULL, POPT_XXX,
	N_("verify a signature"), NULL },
  { "list-keys", '\0', POPT_ARG_NONE,	NULL, POPT_XXX,
	N_("list keys"), NULL },
  { "list-sigs", '\0', POPT_ARG_NONE,	NULL, POPT_XXX,
	N_("list keys and signatures"), NULL },
  { "check-sigs", '\0', POPT_ARG_NONE,	NULL, POPT_XXX,
	N_("list and check key signatures"), NULL },
  { "fingerprint", '\0', POPT_ARG_NONE,	NULL, POPT_XXX,
	N_("list keys and fingerprints"), NULL },
  { "list-secret-keys", 'K', POPT_ARG_NONE,	NULL, 'K',
	N_("list secret keys"), NULL },
  { "gen-key", '\0', POPT_ARG_NONE,	NULL, POPT_XXX,
	N_("generate a new key pair"), NULL },
  { "delete-keys", '\0', POPT_ARG_NONE,	NULL, POPT_XXX,
	N_("remove keys from the public keyring"), NULL },
  { "delete-secret-keys", '\0', POPT_ARG_NONE,	NULL, POPT_XXX,
	N_("remove keys from the secret keyring"), NULL },
  { "sign-key", '\0', POPT_ARG_NONE,	NULL, POPT_XXX,
	N_("sign a key"), NULL },
  { "lsign-key", '\0', POPT_ARG_NONE,	NULL, POPT_XXX,
	N_("sign a key locally"), NULL },
  { "edit-key", '\0', POPT_ARG_NONE,	NULL, POPT_XXX,
	N_("sign or edit a key"), NULL },
  { "gen-revoke", '\0', POPT_ARG_NONE,	NULL, POPT_XXX,
	N_("generate a revocation certificate"), NULL },
  { "export", '\0', POPT_ARG_NONE,	NULL, POPT_XXX,
	N_("export keys"), NULL },
  { "send-keys", '\0', POPT_ARG_NONE,	NULL, POPT_XXX,
	N_("export keys to a key server"), NULL },
  { "recv-keys", '\0', POPT_ARG_NONE,	NULL, POPT_XXX,
	N_("import keys from a key server"), NULL },
  { "search-keys", '\0', POPT_ARG_NONE,	NULL, POPT_XXX,
	N_("search for keys on a key server"), NULL },
  { "refresh-keys", '\0', POPT_ARG_NONE,	NULL, POPT_XXX,
	N_("update all keys from a keyserver"), NULL },
  { "import", '\0', POPT_ARG_NONE,	NULL, POPT_XXX,
	N_("import/merge keys"), NULL },
  { "card-status", '\0', POPT_ARG_NONE,	NULL, POPT_XXX,
	N_("print the card status"), NULL },
  { "card-edit", '\0', POPT_ARG_NONE,	NULL, POPT_XXX,
	N_("change data on a card"), NULL },
  { "change-pin", '\0', POPT_ARG_NONE,	NULL, POPT_XXX,
	N_("change a card's PIN"), NULL },
  { "update-trustdb", '\0', POPT_ARG_NONE,	NULL, POPT_XXX,
	N_("update the trust database"), NULL },
  { "print-md", '\0', POPT_ARG_NONE,	NULL, POPT_XXX,
	N_("algo [files]   print message digests"), NULL },

  POPT_TABLEEND

};

/*@unchecked@*/ /*@observer@*/
static struct poptOption rpmgpgOptionsPoptTable[] = {
/*@-type@*/ /* FIX: cast? */
 { NULL, '\0', POPT_ARG_CALLBACK | POPT_CBFLAG_INC_DATA | POPT_CBFLAG_CONTINUE,
        rpmgpgArgCallback, 0, NULL, NULL },
/*@=type@*/

  { "armor", 'a', POPT_ARG_NONE,	NULL, 'a',
	N_("create ascii armored output"), NULL },
  { "recipient", 'r', POPT_ARG_NONE,	NULL, 'r',
	N_("NAME          encrypt for NAME"), NULL },
  { "local-user", 'u', POPT_ARG_NONE,	NULL, 'u',
	N_("use this user-id to sign or decrypt"), NULL },
  { NULL, 'z', POPT_ARG_NONE,	NULL, 'z',
	N_("set compress level N (0 disables)"), N_("N") },
  { "textmode", '\0', POPT_ARG_NONE,	NULL, POPT_XXX,
	N_("use canonical text mode"), NULL },
  { "output", 'o', POPT_ARG_NONE,	NULL, 'o',
	N_("use as output file"), NULL },
  { "verbose", 'v', POPT_ARG_NONE,	NULL, 'v',
	N_("verbose"), NULL },
  { "dry-run", 'n', POPT_ARG_NONE,	NULL, 'n',
	N_("do not make any changes"), NULL },
  { "interactive", 'i', POPT_ARG_NONE,	NULL, 'i',
	N_("prompt before overwriting"), NULL },
  { "openpgp", '\0', POPT_ARG_NONE,	NULL, POPT_XXX,
	N_("use strict OpenPGP behavior"), NULL },
  { "pgp2", '\0', POPT_ARG_NONE,	NULL, POPT_XXX,
	N_("generate PGP 2.x compatible messages"), NULL },

  POPT_TABLEEND

};

/*@unchecked@*/ /*@observer@*/
static struct poptOption optionsTable[] = {
/*@-type@*/ /* FIX: cast? */
 { NULL, '\0', POPT_ARG_CALLBACK | POPT_CBFLAG_INC_DATA | POPT_CBFLAG_CONTINUE,
        rpmgpgArgCallback, 0, NULL, NULL },
/*@=type@*/

  { NULL, (char)-1, POPT_ARG_INCLUDE_TABLE, NULL, 0,
	N_("\
gpg (GnuPG) 1.4.9\n\
Copyright (C) 2008 Free Software Foundation, Inc.\n\
License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>\n\
This is free software: you are free to change and redistribute it.\n\
There is NO WARRANTY, to the extent permitted by law.\n\
\n\
Home: ~/.gnupg\n\
Supported algorithms:\n\
Pubkey: RSA, RSA-E, RSA-S, ELG-E, DSA\n\
Cipher: 3DES, CAST5, BLOWFISH, AES, AES192, AES256, TWOFISH\n\
Hash: MD5, SHA1, RIPEMD160, SHA256, SHA384, SHA512, SHA224\n\
Compression: Uncompressed, ZIP, ZLIB, BZIP2\n\
\n\
Syntax: gpg [options] [files]\n\
sign, check, encrypt or decrypt\n\
default operation depends on the input data\n\
"), NULL },

  { NULL, '\0', POPT_ARG_INCLUDE_TABLE, rpmgpgCommandsPoptTable, 0,
        N_("Commands:"), NULL },

  { NULL, '\0', POPT_ARG_INCLUDE_TABLE, rpmgpgOptionsPoptTable, 0,
        N_("Options:"), NULL },

  POPT_AUTOALIAS
  POPT_AUTOHELP

  { NULL, (char)-1, POPT_ARG_INCLUDE_TABLE, NULL, 0,
	N_("\
(See the man page for a complete listing of all commands and options)\n\
\n\
Examples:\n\
\n\
 -se -r Bob [file]          sign and encrypt for user Bob\n\
 --clearsign [file]         make a clear text signature\n\
 --detach-sign [file]       make a detached signature\n\
 --list-keys [names]        show keys\n\
 --fingerprint [names]      show fingerprints\n\
\n\
Please report bugs to <gnupg-bugs@gnu.org>.\n\
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
    __progname = "rpmgpg";
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
