/** \ingroup rpmcli
 * \file rpmdb/poptDB.c
 *  Popt tables for database modes.
 */

#include "system.h"

#include <rpmiotypes.h>

#include <rpmtypes.h>
#include <rpmtag.h>
#include <rpmdb.h>
#include <rpmlio.h>
#include <rpmmdb.h>
#include <rpmrepo.h>
#include <rpmtxn.h>

#include <rpmcli.h>	/* XXX rpmQVKArguments_s, <popt.h> */

#include "debug.h"

/*@-redecl@*/
/*@unchecked@*/
extern int _dbi_debug;
/*@unchecked@*/
extern int _scp_debug;
/*@unchecked@*/
extern int _sqldb_debug;
/*@=redecl@*/

/*@unchecked@*/
struct rpmQVKArguments_s rpmDBArgs;

/**
 */
struct poptOption rpmDatabasePoptTable[] = {

 { "rebuilddb", '\0', POPT_ARG_VAL, &rpmDBArgs.rebuild, 1,
	N_("rebuild database inverted lists from installed package headers"),
	NULL},

 { "rpmdbdebug", '\0', POPT_ARG_VAL|POPT_ARGFLAG_DOC_HIDDEN, &_rpmdb_debug, -1,
	N_("Debug rpmdb DataBase"), NULL},
 { "rpmdbidebug", '\0', POPT_ARG_VAL|POPT_ARGFLAG_DOC_HIDDEN, &_dbi_debug, -1,
	N_("Debug dbiIndex DataBase Index"), NULL},
 { "rpmliodebug", '\0', POPT_ARG_VAL|POPT_ARGFLAG_DOC_HIDDEN, &_rpmlio_debug, -1,
	N_("Debug rpmlio database Log I/O"), NULL},
 { "rpmmdbdebug", '\0', POPT_ARG_VAL|POPT_ARGFLAG_DOC_HIDDEN, &_rpmmdb_debug, -1,
	N_("Debug rpmmdb Mongo DB"), NULL},
 { "rpmrepodebug", '\0', POPT_ARG_VAL|POPT_ARGFLAG_DOC_HIDDEN, &_rpmrepo_debug, -1,
	N_("Debug rpmrepo repository wrappers "), NULL},
 { "rpmtxndebug", '\0', POPT_ARG_VAL|POPT_ARGFLAG_DOC_HIDDEN, &_rpmtxn_debug, -1,
	N_("Debug rpmtxn database Transaction"), NULL},
#if defined(HAVE_SQLITE3_H)	/* XXX test --with-sqlite=external */
 { "scpdebug", '\0', POPT_ARG_VAL|POPT_ARGFLAG_DOC_HIDDEN, &_scp_debug, -1,
	N_("Debug SQLDB cursors"), NULL},
 { "sqldbdebug", '\0', POPT_ARG_VAL|POPT_ARGFLAG_DOC_HIDDEN, &_sqldb_debug, -1,
	N_("Debug SQLDB"), NULL},
#endif

   POPT_TABLEEND
};
