/* File automatically generate by ./rpmh2tbl
 * Olivier Thauvin <thauvin at aerov.jussieu.fr>
 * Parsed files:
 * ../lib/filetriggers.h
 * ../lib/fs.h
 * ../lib/fsm.h
 * ../lib/manifest.h
 * ../lib/misc.h
 * ../lib/psm.h
 * ../lib/rpm46compat.h
 * ../lib/rpm4compat.h
 * ../lib/rpmal.h
 * ../lib/rpmcli.h
 * ../lib/rpmds.h
 * ../lib/rpmfc.h
 * ../lib/rpmfi.h
 * ../lib/rpmgi.h
 * ../lib/rpmlib.h
 * ../lib/rpmlock.h
 * ../lib/rpmluaext.h
 * ../lib/rpmps.h
 * ../lib/rpmrc.h
 * ../lib/rpmrollback.h
 * ../lib/rpmte.h
 * ../lib/rpmts.h
 * ../lib/rpmversion.h
 * ../build/buildio.h
 * ../build/rpmbuild.h
 * ../build/rpmspec.h
 * ../rpmdb/db_emu.h
 * ../rpmdb/fprint.h
 * ../rpmdb/header_internal.h
 * ../rpmdb/legacy.h
 * ../rpmdb/logio_auto.h
 * ../rpmdb/logio.h
 * ../rpmdb/pkgio.h
 * ../rpmdb/rpmdb.h
 * ../rpmdb/rpmdpkg.h
 * ../rpmdb/rpmevr.h
 * ../rpmdb/rpmlio.h
 * ../rpmdb/rpmmdb.h
 * ../rpmdb/rpmns.h
 * ../rpmdb/rpmrepo.h
 * ../rpmdb/rpmtag.h
 * ../rpmdb/rpmtd.h
 * ../rpmdb/rpmtxn.h
 * ../rpmdb/rpmtypes.h
 * ../rpmdb/rpmwf.h
 * ../rpmdb/signature.h
 * ../rpmio/argv.h
 * ../rpmio/ar.h
 * ../rpmio/arirang.h
 * ../rpmio/blake2.h
 * ../rpmio/blake2-impl.h
 * ../rpmio/blake2-kat.h
 * ../rpmio/blake2-rpm.h
 * ../rpmio/blake.h
 * ../rpmio/bmw.h
 * ../rpmio/bson.h
 * ../rpmio/chi.h
 * ../rpmio/cpio.h
 * ../rpmio/crc.h
 * ../rpmio/cubehash.h
 * ../rpmio/echo.h
 * ../rpmio/edon-r.h
 * ../rpmio/envvar.h
 * ../rpmio/fnmatch.h
 * ../rpmio/fts.h
 * ../rpmio/fugue.h
 * ../rpmio/gengpg.h
 * ../rpmio/glob.h
 * ../rpmio/groestl.h
 * ../rpmio/hamsi.h
 * ../rpmio/html-parse.h
 * ../rpmio/iosm.h
 * ../rpmio/jh.h
 * ../rpmio/keccak.h
 * ../rpmio/lane.h
 * ../rpmio/luffa.h
 * ../rpmio/md2.h
 * ../rpmio/md6.h
 * ../rpmio/mire.h
 * ../rpmio/mongo.h
 * ../rpmio/poptIO.h
 * ../rpmio/radiogatun.h
 * ../rpmio/rpmacl.h
 * ../rpmio/rpmasn.h
 * ../rpmio/rpmaug.h
 * ../rpmio/rpmbag.h
 * ../rpmio/rpmbc.h
 * ../rpmio/rpmbf.h
 * ../rpmio/rpmbz.h
 * ../rpmio/rpmcb.h
 * ../rpmio/rpmcdsa.h
 * ../rpmio/rpmcpio.h
 * ../rpmio/rpmcudf.h
 * ../rpmio/rpmcvs.h
 * ../rpmio/rpmdav.h
 * ../rpmio/rpmdir.h
 * ../rpmio/rpmficl.h
 * ../rpmio/rpmgc.h
 * ../rpmio/rpmgit.h
 * ../rpmio/rpmhash.h
 * ../rpmio/rpmhkp.h
 * ../rpmio/rpmhook.h
 * ../rpmio/rpmio.h
 * ../rpmio/rpmio_internal.h
 * ../rpmio/rpmio-stub.h
 * ../rpmio/rpmiotypes.h
 * ../rpmio/rpmjni.h
 * ../rpmio/rpmjs.h
 * ../rpmio/rpmjsio.h
 * ../rpmio/rpmkeyring.h
 * ../rpmio/rpmku.h
 * ../rpmio/rpmlog.h
 * ../rpmio/rpmltc.h
 * ../rpmio/rpmlua.h
 * ../rpmio/rpmmacro.h
 * ../rpmio/rpmmg.h
 * ../rpmio/rpmmrb.h
 * ../rpmio/rpmnix.h
 * ../rpmio/rpmnss.h
 * ../rpmio/rpmodbc.h
 * ../rpmio/rpmperl.h
 * ../rpmio/rpmpgp.h
 * ../rpmio/rpmpython.h
 * ../rpmio/rpmruby.h
 * ../rpmio/rpmsm.h
 * ../rpmio/rpmsp.h
 * ../rpmio/rpmsq.h
 * ../rpmio/rpmsql.h
 * ../rpmio/rpmsquirrel.h
 * ../rpmio/rpmssl.h
 * ../rpmio/rpmsvn.h
 * ../rpmio/rpmsw.h
 * ../rpmio/rpmsx.h
 * ../rpmio/rpmsyck.h
 * ../rpmio/rpmtar.h
 * ../rpmio/rpmtcl.h
 * ../rpmio/rpmtpm.h
 * ../rpmio/rpmurl.h
 * ../rpmio/rpmuuid.h
 * ../rpmio/rpmxar.h
 * ../rpmio/rpmz.h
 * ../rpmio/rpmzlog.h
 * ../rpmio/rpmzq.h
 * ../rpmio/salsa10.h
 * ../rpmio/salsa20.h
 * ../rpmio/set.h
 * ../rpmio/shabal.h
 * ../rpmio/shavite3.h
 * ../rpmio/simd.h
 * ../rpmio/skein.h
 * ../rpmio/tar.h
 * ../rpmio/tib3.h
 * ../rpmio/tiger.h
 * ../rpmio/ugid.h
 * ../rpmio/yarn.h
 * ../misc/err.h
 * ../misc/error.h
 * ../misc/tempname.h
 */

#include "system.h"
#define RPMCONSTANT_INTERNAL
#define _RPMEVR_INTERNAL
#include "rpmconstant.h"
 

/* From rpmcli.h
 * prefix tag: RPMVERIFY_
 * table: rpmVerifyAttrs */
#include "rpmcli.h"
static const struct rpmconstant_s rpmVerifyAttrsctbl[] = {
#ifdef H_RPMCLI
	{ "RPMVERIFY_NONE", RPMVERIFY_NONE }, /*!< */
	{ "RPMVERIFY_FDIGEST", RPMVERIFY_FDIGEST }, 
	{ "RPMVERIFY_FILESIZE", RPMVERIFY_FILESIZE }, 
	{ "RPMVERIFY_LINKTO", RPMVERIFY_LINKTO }, 
	{ "RPMVERIFY_USER", RPMVERIFY_USER }, 
	{ "RPMVERIFY_GROUP", RPMVERIFY_GROUP }, 
	{ "RPMVERIFY_MTIME", RPMVERIFY_MTIME }, 
	{ "RPMVERIFY_MODE", RPMVERIFY_MODE }, 
	{ "RPMVERIFY_RDEV", RPMVERIFY_RDEV }, 
	{ "RPMVERIFY_CAPS", RPMVERIFY_CAPS }, 
	{ "RPMVERIFY_HMAC", RPMVERIFY_HMAC }, 
	{ "RPMVERIFY_CONTEXTS", RPMVERIFY_CONTEXTS }, 
	{ "RPMVERIFY_READLINKFAIL", RPMVERIFY_READLINKFAIL }, 
	{ "RPMVERIFY_READFAIL", RPMVERIFY_READFAIL }, 
	{ "RPMVERIFY_LSTATFAIL", RPMVERIFY_LSTATFAIL }, 
	{ "RPMVERIFY_LGETFILECONFAIL", RPMVERIFY_LGETFILECONFAIL }, 
	{ "RPMVERIFY_ALL", RPMVERIFY_ALL }, 
	{ "RPMVERIFY_FAILURES", RPMVERIFY_FAILURES }, 
#endif /* H_RPMCLI */
	{ NULL, 0 } /* NULL terminated (rpmVerifyAttrs) */
};
const struct rpmconstant_s * rpmVerifyAttrsctable = rpmVerifyAttrsctbl;


/* From rpmcli.h
 * prefix tag: RPMQV_
 * table: rpmQVSources */
#include "rpmcli.h"
static const struct rpmconstant_s rpmQVSourcesctbl[] = {
#ifdef H_RPMCLI
	{ "RPMQV_PACKAGE", RPMQV_PACKAGE }, /*!< ... from package name db search. */
	{ "RPMQV_PATH", RPMQV_PATH }, /*!< ... from file path db search. */
	{ "RPMQV_ALL", RPMQV_ALL }, /*!< ... from each installed package. */
	{ "RPMQV_RPM", RPMQV_RPM }, /*!< ... from reading binary rpm package. */
	{ "RPMQV_GROUP", RPMQV_GROUP }, 
	{ "RPMQV_WHATPROVIDES", RPMQV_WHATPROVIDES }, /*!< ... from provides db search. */
	{ "RPMQV_WHATREQUIRES", RPMQV_WHATREQUIRES }, /*!< ... from requires db search. */
	{ "RPMQV_TRIGGEREDBY", RPMQV_TRIGGEREDBY }, 
	{ "RPMQV_DBOFFSET", RPMQV_DBOFFSET }, /*!< ... from database header instance. */
	{ "RPMQV_SPECFILE", RPMQV_SPECFILE }, /*!< ... from spec file parse (query only). */
	{ "RPMQV_PKGID", RPMQV_PKGID }, /*!< ... from package id (header+payload MD5). */
	{ "RPMQV_HDRID", RPMQV_HDRID }, /*!< ... from header id (immutable header SHA1). */
	{ "RPMQV_FILEID", RPMQV_FILEID }, /*!< ... from file id (file digest, usually MD5). */
	{ "RPMQV_TID", RPMQV_TID }, /*!< ... from install transaction id (time stamp). */
	{ "RPMQV_HDLIST", RPMQV_HDLIST }, /*!< ... from system hdlist. */
	{ "RPMQV_FTSWALK", RPMQV_FTSWALK }, /*!< ... from fts(3) walk. */
	{ "RPMQV_WHATNEEDS", RPMQV_WHATNEEDS }, /*!< ... from requires using contained provides. */
	{ "RPMQV_SPECSRPM", RPMQV_SPECSRPM }, /*!< ... srpm from spec file parse (query only). */
	{ "RPMQV_SOURCEPKGID", RPMQV_SOURCEPKGID }, /*!< ... from source package id (header+payload MD5). */
	{ "RPMQV_WHATCONFLICTS", RPMQV_WHATCONFLICTS }, 
	{ "RPMQV_WHATOBSOLETES", RPMQV_WHATOBSOLETES }, 
#endif /* H_RPMCLI */
	{ NULL, 0 } /* NULL terminated (rpmQVSources) */
};
const struct rpmconstant_s * rpmQVSourcesctable = rpmQVSourcesctbl;


/* From rpmcli.h
 * prefix tag: QUERY_
 * table: rpmQueryFlags */
#include "rpmcli.h"
static const struct rpmconstant_s rpmQueryFlagsctbl[] = {
#ifdef H_RPMCLI
	{ "QUERY_FOR_DEFAULT", QUERY_FOR_DEFAULT }, /*!< */
	{ "QUERY_FDIGEST", QUERY_FDIGEST }, 
	{ "QUERY_SIZE", QUERY_SIZE }, 
	{ "QUERY_LINKTO", QUERY_LINKTO }, 
	{ "QUERY_USER", QUERY_USER }, 
	{ "QUERY_GROUP", QUERY_GROUP }, 
	{ "QUERY_MTIME", QUERY_MTIME }, 
	{ "QUERY_MODE", QUERY_MODE }, 
	{ "QUERY_RDEV", QUERY_RDEV }, 
	{ "QUERY_CAPS", QUERY_CAPS }, 
	{ "QUERY_HMAC", QUERY_HMAC }, 
	{ "QUERY_CONTEXTS", QUERY_CONTEXTS }, 
	{ "QUERY_FILES", QUERY_FILES }, 
	{ "QUERY_DEPS", QUERY_DEPS }, 
	{ "QUERY_SCRIPT", QUERY_SCRIPT }, 
	{ "QUERY_DIGEST", QUERY_DIGEST }, 
	{ "QUERY_SIGNATURE", QUERY_SIGNATURE }, 
	{ "QUERY_PATCHES", QUERY_PATCHES }, 
	{ "QUERY_HDRCHK", QUERY_HDRCHK }, 
	{ "QUERY_FOR_LIST", QUERY_FOR_LIST }, 
	{ "QUERY_FOR_STATE", QUERY_FOR_STATE }, 
	{ "QUERY_FOR_DOCS", QUERY_FOR_DOCS }, 
	{ "QUERY_FOR_CONFIG", QUERY_FOR_CONFIG }, 
	{ "QUERY_FOR_DUMPFILES", QUERY_FOR_DUMPFILES }, 
#endif /* H_RPMCLI */
	{ NULL, 0 } /* NULL terminated (rpmQueryFlags) */
};
const struct rpmconstant_s * rpmQueryFlagsctable = rpmQueryFlagsctbl;


/* From rpmcli.h
 * prefix tag: VERIFY_
 * table: rpmVerifyFlags */
#include "rpmcli.h"
static const struct rpmconstant_s rpmVerifyFlagsctbl[] = {
#ifdef H_RPMCLI
	{ "VERIFY_DEFAULT", VERIFY_DEFAULT }, /*!< */
	{ "VERIFY_FDIGEST", VERIFY_FDIGEST }, 
	{ "VERIFY_SIZE", VERIFY_SIZE }, 
	{ "VERIFY_LINKTO", VERIFY_LINKTO }, 
	{ "VERIFY_USER", VERIFY_USER }, 
	{ "VERIFY_GROUP", VERIFY_GROUP }, 
	{ "VERIFY_MTIME", VERIFY_MTIME }, 
	{ "VERIFY_MODE", VERIFY_MODE }, 
	{ "VERIFY_RDEV", VERIFY_RDEV }, 
	{ "VERIFY_CAPS", VERIFY_CAPS }, 
	{ "VERIFY_HMAC", VERIFY_HMAC }, 
	{ "VERIFY_CONTEXTS", VERIFY_CONTEXTS }, 
	{ "VERIFY_FILES", VERIFY_FILES }, 
	{ "VERIFY_DEPS", VERIFY_DEPS }, 
	{ "VERIFY_SCRIPT", VERIFY_SCRIPT }, 
	{ "VERIFY_DIGEST", VERIFY_DIGEST }, 
	{ "VERIFY_SIGNATURE", VERIFY_SIGNATURE }, 
	{ "VERIFY_PATCHES", VERIFY_PATCHES }, 
	{ "VERIFY_HDRCHK", VERIFY_HDRCHK }, 
	{ "VERIFY_FOR_LIST", VERIFY_FOR_LIST }, 
	{ "VERIFY_FOR_STATE", VERIFY_FOR_STATE }, 
	{ "VERIFY_FOR_DOCS", VERIFY_FOR_DOCS }, 
	{ "VERIFY_FOR_CONFIG", VERIFY_FOR_CONFIG }, 
	{ "VERIFY_FOR_DUMPFILES", VERIFY_FOR_DUMPFILES }, 
	{ "VERIFY_ATTRS", VERIFY_ATTRS }, 
	{ "VERIFY_ALL", VERIFY_ALL }, 
#endif /* H_RPMCLI */
	{ NULL, 0 } /* NULL terminated (rpmVerifyFlags) */
};
const struct rpmconstant_s * rpmVerifyFlagsctable = rpmVerifyFlagsctbl;


/* From rpmcli.h
 * prefix tag: RPMSIGN_
 * table: rpmSignFlags */
#include "rpmcli.h"
static const struct rpmconstant_s rpmSignFlagsctbl[] = {
#ifdef H_RPMCLI
	{ "RPMSIGN_NONE", RPMSIGN_NONE }, 
	{ "RPMSIGN_CHK_SIGNATURE", RPMSIGN_CHK_SIGNATURE }, /*!< from --checksig */
	{ "RPMSIGN_NEW_SIGNATURE", RPMSIGN_NEW_SIGNATURE }, /*!< from --resign */
	{ "RPMSIGN_ADD_SIGNATURE", RPMSIGN_ADD_SIGNATURE }, /*!< from --addsign */
	{ "RPMSIGN_DEL_SIGNATURE", RPMSIGN_DEL_SIGNATURE }, /*!< from --delsign */
	{ "RPMSIGN_IMPORT_PUBKEY", RPMSIGN_IMPORT_PUBKEY }, /*!< from --import */
#endif /* H_RPMCLI */
	{ NULL, 0 } /* NULL terminated (rpmSignFlags) */
};
const struct rpmconstant_s * rpmSignFlagsctable = rpmSignFlagsctbl;


/* From rpmcli.h
 * prefix tag: INSTALL_
 * table: rpmInstallInterfaceFlags */
#include "rpmcli.h"
static const struct rpmconstant_s rpmInstallInterfaceFlagsctbl[] = {
#ifdef H_RPMCLI
	{ "INSTALL_NONE", INSTALL_NONE }, 
	{ "INSTALL_PERCENT", INSTALL_PERCENT }, 
	{ "INSTALL_HASH", INSTALL_HASH }, 
	{ "INSTALL_NODEPS", INSTALL_NODEPS }, 
	{ "INSTALL_NOORDER", INSTALL_NOORDER }, 
	{ "INSTALL_LABEL", INSTALL_LABEL }, 
	{ "INSTALL_UPGRADE", INSTALL_UPGRADE }, 
	{ "INSTALL_FRESHEN", INSTALL_FRESHEN }, 
	{ "INSTALL_INSTALL", INSTALL_INSTALL }, 
	{ "INSTALL_ERASE", INSTALL_ERASE }, 
	{ "INSTALL_ALLMATCHES", INSTALL_ALLMATCHES }, 
#endif /* H_RPMCLI */
	{ NULL, 0 } /* NULL terminated (rpmInstallInterfaceFlags) */
};
const struct rpmconstant_s * rpmInstallInterfaceFlagsctable = rpmInstallInterfaceFlagsctbl;


/* From rpmfi.h
 * prefix tag: RPMFILE_
 * table: rpmfileState */
#include "rpmfi.h"
static const struct rpmconstant_s rpmfileStatectbl[] = {
#ifdef H_RPMFI
	{ "RPMFILE_STATE_NORMAL", RPMFILE_STATE_NORMAL }, 
	{ "RPMFILE_STATE_REPLACED", RPMFILE_STATE_REPLACED }, 
	{ "RPMFILE_STATE_NOTINSTALLED", RPMFILE_STATE_NOTINSTALLED }, 
	{ "RPMFILE_STATE_NETSHARED", RPMFILE_STATE_NETSHARED }, 
	{ "RPMFILE_STATE_WRONGCOLOR", RPMFILE_STATE_WRONGCOLOR }, 
	{ "RPMFILE_STATE_MISSING", RPMFILE_STATE_MISSING }, /* XXX used for unavailable data */
#endif /* H_RPMFI */
	{ NULL, 0 } /* NULL terminated (rpmfileState) */
};
const struct rpmconstant_s * rpmfileStatectable = rpmfileStatectbl;


/* From rpmfi.h
 * prefix tag: 
 * table: rpmfileAttrs */
#include "rpmfi.h"
static const struct rpmconstant_s rpmfileAttrsctbl[] = {
#ifdef H_RPMFI
	{ "RPMFILE_NONE", RPMFILE_NONE }, 
	{ "RPMFILE_CONFIG", RPMFILE_CONFIG }, 
	{ "RPMFILE_DOC", RPMFILE_DOC }, 
	{ "RPMFILE_ICON", RPMFILE_ICON }, 
	{ "RPMFILE_MISSINGOK", RPMFILE_MISSINGOK }, 
	{ "RPMFILE_NOREPLACE", RPMFILE_NOREPLACE }, 
	{ "RPMFILE_GHOST", RPMFILE_GHOST }, 
	{ "RPMFILE_LICENSE", RPMFILE_LICENSE }, 
	{ "RPMFILE_README", RPMFILE_README }, 
	{ "RPMFILE_UNPATCHED", RPMFILE_UNPATCHED }, 
	{ "RPMFILE_PUBKEY", RPMFILE_PUBKEY }, 
	{ "RPMFILE_POLICY", RPMFILE_POLICY }, 
	{ "RPMFILE_PATCH", RPMFILE_PATCH }, 
	{ "RPMFILE_OPTIONAL", RPMFILE_OPTIONAL }, 
	{ "RPMFILE_REMOVE", RPMFILE_REMOVE }, 
#endif /* H_RPMFI */
	{ NULL, 0 } /* NULL terminated (rpmfileAttrs) */
};
const struct rpmconstant_s * rpmfileAttrsctable = rpmfileAttrsctbl;


/* From rpmfi.h
 * prefix tag: 
 * table: rpmfileTypes */
#include "rpmfi.h"
static const struct rpmconstant_s rpmfileTypesctbl[] = {
#ifdef H_RPMFI
	{ "PIPE", PIPE }, /*!< pipe/fifo */
	{ "CDEV", CDEV }, /*!< character device */
	{ "XDIR", XDIR }, /*!< directory */
	{ "BDEV", BDEV }, /*!< block device */
	{ "REG", REG }, /*!< regular file */
	{ "LINK", LINK }, /*!< hard link */
	{ "SOCK", SOCK }, /*!< socket */
#endif /* H_RPMFI */
	{ NULL, 0 } /* NULL terminated (rpmfileTypes) */
};
const struct rpmconstant_s * rpmfileTypesctable = rpmfileTypesctbl;


/* From rpmps.h
 * prefix tag: RPMPROB_FILTER_
 * table: rpmprobFilterFlags */
#include "rpmps.h"
static const struct rpmconstant_s rpmprobFilterFlagsctbl[] = {
#ifdef H_RPMPS
	{ "RPMPROB_FILTER_NONE", RPMPROB_FILTER_NONE }, 
	{ "RPMPROB_FILTER_IGNOREOS", RPMPROB_FILTER_IGNOREOS }, 
	{ "RPMPROB_FILTER_IGNOREARCH", RPMPROB_FILTER_IGNOREARCH }, 
	{ "RPMPROB_FILTER_REPLACEPKG", RPMPROB_FILTER_REPLACEPKG }, 
	{ "RPMPROB_FILTER_FORCERELOCATE", RPMPROB_FILTER_FORCERELOCATE }, 
	{ "RPMPROB_FILTER_REPLACENEWFILES", RPMPROB_FILTER_REPLACENEWFILES }, 
	{ "RPMPROB_FILTER_REPLACEOLDFILES", RPMPROB_FILTER_REPLACEOLDFILES }, 
	{ "RPMPROB_FILTER_OLDPACKAGE", RPMPROB_FILTER_OLDPACKAGE }, 
	{ "RPMPROB_FILTER_DISKSPACE", RPMPROB_FILTER_DISKSPACE }, 
	{ "RPMPROB_FILTER_DISKNODES", RPMPROB_FILTER_DISKNODES }, 
#endif /* H_RPMPS */
	{ NULL, 0 } /* NULL terminated (rpmprobFilterFlags) */
};
const struct rpmconstant_s * rpmprobFilterFlagsctable = rpmprobFilterFlagsctbl;


/* From rpmps.h
 * prefix tag: RPMPROB_
 * table: rpmProblemType */
#include "rpmps.h"
static const struct rpmconstant_s rpmProblemTypectbl[] = {
#ifdef H_RPMPS
	{ "RPMPROB_FILTER_NONE", RPMPROB_FILTER_NONE }, 
	{ "RPMPROB_FILTER_IGNOREOS", RPMPROB_FILTER_IGNOREOS }, 
	{ "RPMPROB_FILTER_IGNOREARCH", RPMPROB_FILTER_IGNOREARCH }, 
	{ "RPMPROB_FILTER_REPLACEPKG", RPMPROB_FILTER_REPLACEPKG }, 
	{ "RPMPROB_FILTER_FORCERELOCATE", RPMPROB_FILTER_FORCERELOCATE }, 
	{ "RPMPROB_FILTER_REPLACENEWFILES", RPMPROB_FILTER_REPLACENEWFILES }, 
	{ "RPMPROB_FILTER_REPLACEOLDFILES", RPMPROB_FILTER_REPLACEOLDFILES }, 
	{ "RPMPROB_FILTER_OLDPACKAGE", RPMPROB_FILTER_OLDPACKAGE }, 
	{ "RPMPROB_FILTER_DISKSPACE", RPMPROB_FILTER_DISKSPACE }, 
	{ "RPMPROB_FILTER_DISKNODES", RPMPROB_FILTER_DISKNODES }, 
	{ "RPMPROB_BADARCH", RPMPROB_BADARCH }, /*!< (unused) package ... is for a different architecture */
	{ "RPMPROB_BADOS", RPMPROB_BADOS }, /*!< (unused) package ... is for a different operating system */
	{ "RPMPROB_PKG_INSTALLED", RPMPROB_PKG_INSTALLED }, /*!< package ... is already installed */
	{ "RPMPROB_BADRELOCATE", RPMPROB_BADRELOCATE }, /*!< path ... is not relocatable for package ... */
	{ "RPMPROB_REQUIRES", RPMPROB_REQUIRES }, /*!< package ... has unsatisfied Requires: ... */
	{ "RPMPROB_CONFLICT", RPMPROB_CONFLICT }, /*!< package ... has unsatisfied Conflicts: ... */
	{ "RPMPROB_NEW_FILE_CONFLICT", RPMPROB_NEW_FILE_CONFLICT }, /*!< file ... conflicts between attempted installs of ... */
	{ "RPMPROB_FILE_CONFLICT", RPMPROB_FILE_CONFLICT }, /*!< file ... from install of ... conflicts with file from package ... */
	{ "RPMPROB_OLDPACKAGE", RPMPROB_OLDPACKAGE }, /*!< package ... (which is newer than ...) is already installed */
	{ "RPMPROB_DISKSPACE", RPMPROB_DISKSPACE }, /*!< installing package ... needs ... on the ... filesystem */
	{ "RPMPROB_DISKNODES", RPMPROB_DISKNODES }, /*!< installing package ... needs ... on the ... filesystem */
	{ "RPMPROB_RDONLY", RPMPROB_RDONLY }, /*!< installing package ... on ... rdonly filesystem */
	{ "RPMPROB_BADPRETRANS", RPMPROB_BADPRETRANS }, /*!< (unimplemented) */
	{ "RPMPROB_BADPLATFORM", RPMPROB_BADPLATFORM }, /*!< package ... is for a different platform */
	{ "RPMPROB_NOREPACKAGE", RPMPROB_NOREPACKAGE }, /*!< re-packaged package ... is missing */
#endif /* H_RPMPS */
	{ NULL, 0 } /* NULL terminated (rpmProblemType) */
};
const struct rpmconstant_s * rpmProblemTypectable = rpmProblemTypectbl;


/* From rpmte.h
 * prefix tag: TR_
 * table: rpmElementType */
#include "rpmte.h"
static const struct rpmconstant_s rpmElementTypectbl[] = {
#ifdef H_RPMTE
	{ "TR_ADDED", TR_ADDED }, 
	{ "TR_REMOVED", TR_REMOVED }, 
#endif /* H_RPMTE */
	{ NULL, 0 } /* NULL terminated (rpmElementType) */
};
const struct rpmconstant_s * rpmElementTypectable = rpmElementTypectbl;


/* From rpmts.h
 * prefix tag: RPMTRANS_FLAG_
 * table: rpmtransFlags */
#include "rpmts.h"
static const struct rpmconstant_s rpmtransFlagsctbl[] = {
#ifdef H_RPMTS
	{ "RPMTRANS_FLAG_NONE", RPMTRANS_FLAG_NONE }, 
	{ "RPMTRANS_FLAG_TEST", RPMTRANS_FLAG_TEST }, 
	{ "RPMTRANS_FLAG_BUILD_PROBS", RPMTRANS_FLAG_BUILD_PROBS }, 
	{ "RPMTRANS_FLAG_NOSCRIPTS", RPMTRANS_FLAG_NOSCRIPTS }, 
	{ "RPMTRANS_FLAG_JUSTDB", RPMTRANS_FLAG_JUSTDB }, 
	{ "RPMTRANS_FLAG_NOTRIGGERS", RPMTRANS_FLAG_NOTRIGGERS }, 
	{ "RPMTRANS_FLAG_NODOCS", RPMTRANS_FLAG_NODOCS }, 
	{ "RPMTRANS_FLAG_ALLFILES", RPMTRANS_FLAG_ALLFILES }, 
	{ "RPMTRANS_FLAG_NOCONTEXTS", RPMTRANS_FLAG_NOCONTEXTS }, 
	{ "RPMTRANS_FLAG_DIRSTASH", RPMTRANS_FLAG_DIRSTASH }, 
	{ "RPMTRANS_FLAG_REPACKAGE", RPMTRANS_FLAG_REPACKAGE }, 
	{ "RPMTRANS_FLAG_PKGCOMMIT", RPMTRANS_FLAG_PKGCOMMIT }, 
	{ "RPMTRANS_FLAG_PKGUNDO", RPMTRANS_FLAG_PKGUNDO }, 
	{ "RPMTRANS_FLAG_COMMIT", RPMTRANS_FLAG_COMMIT }, 
	{ "RPMTRANS_FLAG_UNDO", RPMTRANS_FLAG_UNDO }, 
	{ "RPMTRANS_FLAG_APPLYONLY", RPMTRANS_FLAG_APPLYONLY }, 
	{ "RPMTRANS_FLAG_NOTRIGGERPREIN", RPMTRANS_FLAG_NOTRIGGERPREIN }, 
	{ "RPMTRANS_FLAG_NOPRE", RPMTRANS_FLAG_NOPRE }, 
	{ "RPMTRANS_FLAG_NOPOST", RPMTRANS_FLAG_NOPOST }, 
	{ "RPMTRANS_FLAG_NOTRIGGERIN", RPMTRANS_FLAG_NOTRIGGERIN }, 
	{ "RPMTRANS_FLAG_NOTRIGGERUN", RPMTRANS_FLAG_NOTRIGGERUN }, 
	{ "RPMTRANS_FLAG_NOPREUN", RPMTRANS_FLAG_NOPREUN }, 
	{ "RPMTRANS_FLAG_NOPOSTUN", RPMTRANS_FLAG_NOPOSTUN }, 
	{ "RPMTRANS_FLAG_NOTRIGGERPOSTUN", RPMTRANS_FLAG_NOTRIGGERPOSTUN }, 
	{ "RPMTRANS_FLAG_NOPAYLOAD", RPMTRANS_FLAG_NOPAYLOAD }, 
	{ "RPMTRANS_FLAG_NORPMDB", RPMTRANS_FLAG_NORPMDB }, 
	{ "RPMTRANS_FLAG_NOPOLICY", RPMTRANS_FLAG_NOPOLICY }, 
	{ "RPMTRANS_FLAG_NOFDIGESTS", RPMTRANS_FLAG_NOFDIGESTS }, 
	{ "RPMTRANS_FLAG_NOPRETRANS", RPMTRANS_FLAG_NOPRETRANS }, 
	{ "RPMTRANS_FLAG_NOPOSTTRANS", RPMTRANS_FLAG_NOPOSTTRANS }, 
	{ "RPMTRANS_FLAG_NOCONFIGS", RPMTRANS_FLAG_NOCONFIGS }, 
#endif /* H_RPMTS */
	{ NULL, 0 } /* NULL terminated (rpmtransFlags) */
};
const struct rpmconstant_s * rpmtransFlagsctable = rpmtransFlagsctbl;


/* From rpmts.h
 * prefix tag: TSM_
 * table: rpmtsmStage */
#include "rpmts.h"
static const struct rpmconstant_s rpmtsmStagectbl[] = {
#ifdef H_RPMTS
	{ "TSM_UNKNOWN", TSM_UNKNOWN }, 
	{ "TSM_INSTALL", TSM_INSTALL }, 
	{ "TSM_ERASE", TSM_ERASE }, 
#endif /* H_RPMTS */
	{ NULL, 0 } /* NULL terminated (rpmtsmStage) */
};
const struct rpmconstant_s * rpmtsmStagectable = rpmtsmStagectbl;


/* From rpmts.h
 * prefix tag: RPMTS_OP_
 * table: rpmtsOpX */
#include "rpmts.h"
static const struct rpmconstant_s rpmtsOpXctbl[] = {
#ifdef H_RPMTS
	{ "RPMTS_OP_TOTAL", RPMTS_OP_TOTAL }, 
	{ "RPMTS_OP_CHECK", RPMTS_OP_CHECK }, 
	{ "RPMTS_OP_ORDER", RPMTS_OP_ORDER }, 
	{ "RPMTS_OP_FINGERPRINT", RPMTS_OP_FINGERPRINT }, 
	{ "RPMTS_OP_REPACKAGE", RPMTS_OP_REPACKAGE }, 
	{ "RPMTS_OP_INSTALL", RPMTS_OP_INSTALL }, 
	{ "RPMTS_OP_ERASE", RPMTS_OP_ERASE }, 
	{ "RPMTS_OP_SCRIPTLETS", RPMTS_OP_SCRIPTLETS }, 
	{ "RPMTS_OP_COMPRESS", RPMTS_OP_COMPRESS }, 
	{ "RPMTS_OP_UNCOMPRESS", RPMTS_OP_UNCOMPRESS }, 
	{ "RPMTS_OP_DIGEST", RPMTS_OP_DIGEST }, 
	{ "RPMTS_OP_SIGNATURE", RPMTS_OP_SIGNATURE }, 
	{ "RPMTS_OP_DBADD", RPMTS_OP_DBADD }, 
	{ "RPMTS_OP_DBREMOVE", RPMTS_OP_DBREMOVE }, 
	{ "RPMTS_OP_DBGET", RPMTS_OP_DBGET }, 
	{ "RPMTS_OP_DBPUT", RPMTS_OP_DBPUT }, 
	{ "RPMTS_OP_DBDEL", RPMTS_OP_DBDEL }, 
	{ "RPMTS_OP_READHDR", RPMTS_OP_READHDR }, 
	{ "RPMTS_OP_HDRLOAD", RPMTS_OP_HDRLOAD }, 
	{ "RPMTS_OP_HDRGET", RPMTS_OP_HDRGET }, 
	{ "RPMTS_OP_DEBUG", RPMTS_OP_DEBUG }, 
	{ "RPMTS_OP_MAX", RPMTS_OP_MAX }, 
#endif /* H_RPMTS */
	{ NULL, 0 } /* NULL terminated (rpmtsOpX) */
};
const struct rpmconstant_s * rpmtsOpXctable = rpmtsOpXctbl;


/* From rpmts.h
 * prefix tag: RPMDEPS_
 * table: rpmdepFlags */
#include "rpmts.h"
static const struct rpmconstant_s rpmdepFlagsctbl[] = {
#ifdef H_RPMTS
	{ "RPMDEPS_FLAG_NONE", RPMDEPS_FLAG_NONE }, 
	{ "RPMDEPS_FLAG_NOUPGRADE", RPMDEPS_FLAG_NOUPGRADE }, 
	{ "RPMDEPS_FLAG_NOREQUIRES", RPMDEPS_FLAG_NOREQUIRES }, 
	{ "RPMDEPS_FLAG_NOCONFLICTS", RPMDEPS_FLAG_NOCONFLICTS }, 
	{ "RPMDEPS_FLAG_NOOBSOLETES", RPMDEPS_FLAG_NOOBSOLETES }, 
	{ "RPMDEPS_FLAG_NOPARENTDIRS", RPMDEPS_FLAG_NOPARENTDIRS }, 
	{ "RPMDEPS_FLAG_NOLINKTOS", RPMDEPS_FLAG_NOLINKTOS }, 
	{ "RPMDEPS_FLAG_ANACONDA", RPMDEPS_FLAG_ANACONDA }, 
	{ "RPMDEPS_FLAG_NOSUGGEST", RPMDEPS_FLAG_NOSUGGEST }, 
	{ "RPMDEPS_FLAG_ADDINDEPS", RPMDEPS_FLAG_ADDINDEPS }, 
	{ "RPMDEPS_FLAG_DEPLOOPS", RPMDEPS_FLAG_DEPLOOPS }, 
#endif /* H_RPMTS */
	{ NULL, 0 } /* NULL terminated (rpmdepFlags) */
};
const struct rpmconstant_s * rpmdepFlagsctable = rpmdepFlagsctbl;


/* From rpmts.h
 * prefix tag: RPMTRANS_TYPE_
 * table: rpmtsType */
#include "rpmts.h"
static const struct rpmconstant_s rpmtsTypectbl[] = {
#ifdef H_RPMTS
	{ "RPMTRANS_TYPE_NORMAL", RPMTRANS_TYPE_NORMAL }, 
	{ "RPMTRANS_TYPE_ROLLBACK", RPMTRANS_TYPE_ROLLBACK }, 
	{ "RPMTRANS_TYPE_AUTOROLLBACK", RPMTRANS_TYPE_AUTOROLLBACK }, 
#endif /* H_RPMTS */
	{ NULL, 0 } /* NULL terminated (rpmtsType) */
};
const struct rpmconstant_s * rpmtsTypectable = rpmtsTypectbl;


/* From rpmbuild.h
 * prefix tag: RPMBUILD_
 * table: rpmBuildFlags */
#include "rpmbuild.h"
static const struct rpmconstant_s rpmBuildFlagsctbl[] = {
#ifdef _H_RPMBUILD_
	{ "RPMBUILD_NONE", RPMBUILD_NONE }, 
	{ "RPMBUILD_PREP", RPMBUILD_PREP }, 
	{ "RPMBUILD_BUILD", RPMBUILD_BUILD }, 
	{ "RPMBUILD_INSTALL", RPMBUILD_INSTALL }, 
	{ "RPMBUILD_CHECK", RPMBUILD_CHECK }, 
	{ "RPMBUILD_CLEAN", RPMBUILD_CLEAN }, 
	{ "RPMBUILD_FILECHECK", RPMBUILD_FILECHECK }, 
	{ "RPMBUILD_PACKAGESOURCE", RPMBUILD_PACKAGESOURCE }, 
	{ "RPMBUILD_PACKAGEBINARY", RPMBUILD_PACKAGEBINARY }, 
	{ "RPMBUILD_RMSOURCE", RPMBUILD_RMSOURCE }, 
	{ "RPMBUILD_RMBUILD", RPMBUILD_RMBUILD }, 
	{ "RPMBUILD_STRINGBUF", RPMBUILD_STRINGBUF }, 
	{ "RPMBUILD_TRACK", RPMBUILD_TRACK }, 
	{ "RPMBUILD_RMSPEC", RPMBUILD_RMSPEC }, 
	{ "RPMBUILD_FETCHSOURCE", RPMBUILD_FETCHSOURCE }, 
#endif /* _H_RPMBUILD_ */
	{ NULL, 0 } /* NULL terminated (rpmBuildFlags) */
};
const struct rpmconstant_s * rpmBuildFlagsctable = rpmBuildFlagsctbl;


/* From rpmbuild.h
 * prefix tag: PART_
 * table: rpmParseState */
#include "rpmbuild.h"
static const struct rpmconstant_s rpmParseStatectbl[] = {
#ifdef _H_RPMBUILD_
	{ "PART_SUBNAME", PART_SUBNAME }, 
	{ "PART_NAME", PART_NAME }, 
	{ "PART_BASE", PART_BASE }, 
	{ "PART_NONE", PART_NONE }, /*!< */
	{ "PART_PREAMBLE", PART_PREAMBLE }, /*!< */
	{ "PART_PREP", PART_PREP }, /*!< */
	{ "PART_BUILD", PART_BUILD }, /*!< */
	{ "PART_INSTALL", PART_INSTALL }, /*!< */
	{ "PART_CHECK", PART_CHECK }, /*!< */
	{ "PART_CLEAN", PART_CLEAN }, /*!< */
	{ "PART_FILES", PART_FILES }, /*!< */
	{ "PART_PRE", PART_PRE }, /*!< */
	{ "PART_POST", PART_POST }, /*!< */
	{ "PART_PREUN", PART_PREUN }, /*!< */
	{ "PART_POSTUN", PART_POSTUN }, /*!< */
	{ "PART_PRETRANS", PART_PRETRANS }, /*!< */
	{ "PART_POSTTRANS", PART_POSTTRANS }, /*!< */
	{ "PART_DESCRIPTION", PART_DESCRIPTION }, /*!< */
	{ "PART_CHANGELOG", PART_CHANGELOG }, /*!< */
	{ "PART_TRIGGERIN", PART_TRIGGERIN }, /*!< */
	{ "PART_TRIGGERUN", PART_TRIGGERUN }, /*!< */
	{ "PART_VERIFYSCRIPT", PART_VERIFYSCRIPT }, /*!< */
	{ "PART_BUILDARCHITECTURES", PART_BUILDARCHITECTURES }, /*!< */
	{ "PART_TRIGGERPOSTUN", PART_TRIGGERPOSTUN }, /*!< */
	{ "PART_TRIGGERPREIN", PART_TRIGGERPREIN }, /*!< */
	{ "PART_SANITYCHECK", PART_SANITYCHECK }, /*!< */
	{ "PART_ARBITRARY", PART_ARBITRARY }, /*!< */
	{ "PART_LAST", PART_LAST }, /*!< */
#endif /* _H_RPMBUILD_ */
	{ NULL, 0 } /* NULL terminated (rpmParseState) */
};
const struct rpmconstant_s * rpmParseStatectable = rpmParseStatectbl;


/* From rpmevr.h
 * prefix tag: RPMSENSE_
 * table: rpmsenseflags */
#include "rpmevr.h"
static const struct rpmconstant_s rpmsenseflagsctbl[] = {
#ifdef H_RPMEVR
	{ "RPMSENSE_ANY", RPMSENSE_ANY }, 
	{ "RPMSENSE_SERIAL", RPMSENSE_SERIAL }, 
	{ "RPMSENSE_LESS", RPMSENSE_LESS }, 
	{ "RPMSENSE_GREATER", RPMSENSE_GREATER }, 
	{ "RPMSENSE_EQUAL", RPMSENSE_EQUAL }, 
	{ "RPMSENSE_PROVIDES", RPMSENSE_PROVIDES }, 
	{ "RPMSENSE_CONFLICTS", RPMSENSE_CONFLICTS }, 
	{ "RPMSENSE_PREREQ", RPMSENSE_PREREQ }, 
	{ "RPMSENSE_OBSOLETES", RPMSENSE_OBSOLETES }, 
	{ "RPMSENSE_INTERP", RPMSENSE_INTERP }, 
	{ "RPMSENSE_SCRIPT_PRE", RPMSENSE_SCRIPT_PRE }, 
	{ "RPMSENSE_SCRIPT_POST", RPMSENSE_SCRIPT_POST }, 
	{ "RPMSENSE_SCRIPT_PREUN", RPMSENSE_SCRIPT_PREUN }, 
	{ "RPMSENSE_SCRIPT_POSTUN", RPMSENSE_SCRIPT_POSTUN }, 
	{ "RPMSENSE_SCRIPT_VERIFY", RPMSENSE_SCRIPT_VERIFY }, 
	{ "RPMSENSE_FIND_REQUIRES", RPMSENSE_FIND_REQUIRES }, 
	{ "RPMSENSE_FIND_PROVIDES", RPMSENSE_FIND_PROVIDES }, 
	{ "RPMSENSE_TRIGGERIN", RPMSENSE_TRIGGERIN }, 
	{ "RPMSENSE_TRIGGERUN", RPMSENSE_TRIGGERUN }, 
	{ "RPMSENSE_TRIGGERPOSTUN", RPMSENSE_TRIGGERPOSTUN }, 
	{ "RPMSENSE_MISSINGOK", RPMSENSE_MISSINGOK }, 
	{ "RPMSENSE_SCRIPT_PREP", RPMSENSE_SCRIPT_PREP }, 
	{ "RPMSENSE_SCRIPT_BUILD", RPMSENSE_SCRIPT_BUILD }, 
	{ "RPMSENSE_SCRIPT_INSTALL", RPMSENSE_SCRIPT_INSTALL }, 
	{ "RPMSENSE_SCRIPT_CLEAN", RPMSENSE_SCRIPT_CLEAN }, 
	{ "RPMSENSE_RPMLIB", RPMSENSE_RPMLIB }, 
	{ "RPMSENSE_TRIGGERPREIN", RPMSENSE_TRIGGERPREIN }, 
	{ "RPMSENSE_KEYRING", RPMSENSE_KEYRING }, 
	{ "RPMSENSE_STRONG", RPMSENSE_STRONG }, 
	{ "RPMSENSE_CONFIG", RPMSENSE_CONFIG }, 
	{ "RPMSENSE_PROBE", RPMSENSE_PROBE }, 
	{ "RPMSENSE_PACKAGE", RPMSENSE_PACKAGE }, 
	{ "RPMSENSE_SCRIPT_SANITYCHECK", RPMSENSE_SCRIPT_SANITYCHECK }, 
	{ "RPMSENSE_SENSEMASK", RPMSENSE_SENSEMASK }, 
	{ "RPMSENSE_NOTEQUAL", RPMSENSE_NOTEQUAL }, 
	{ "RPMSENSE_TRIGGER", RPMSENSE_TRIGGER }, 
#endif /* H_RPMEVR */
	{ NULL, 0 } /* NULL terminated (rpmsenseflags) */
};
const struct rpmconstant_s * rpmsenseflagsctable = rpmsenseflagsctbl;


/* From rpmns.h
 * prefix tag: RPMNS_
 * table: rpmnsType */
#include "rpmns.h"
static const struct rpmconstant_s rpmnsTypectbl[] = {
#ifdef H_RPMNS
	{ "RPMNS_TYPE_UNKNOWN", RPMNS_TYPE_UNKNOWN }, 
	{ "RPMNS_TYPE_STRING", RPMNS_TYPE_STRING }, 
	{ "RPMNS_TYPE_PATH", RPMNS_TYPE_PATH }, 
	{ "RPMNS_TYPE_DSO", RPMNS_TYPE_DSO }, 
	{ "RPMNS_TYPE_FUNCTION", RPMNS_TYPE_FUNCTION }, 
	{ "RPMNS_TYPE_ARCH", RPMNS_TYPE_ARCH }, 
	{ "RPMNS_TYPE_VERSION", RPMNS_TYPE_VERSION }, 
	{ "RPMNS_TYPE_COMPOUND", RPMNS_TYPE_COMPOUND }, 
	{ "RPMNS_TYPE_NAMESPACE", RPMNS_TYPE_NAMESPACE }, 
	{ "RPMNS_TYPE_RPMLIB", RPMNS_TYPE_RPMLIB }, 
	{ "RPMNS_TYPE_CPUINFO", RPMNS_TYPE_CPUINFO }, 
	{ "RPMNS_TYPE_GETCONF", RPMNS_TYPE_GETCONF }, 
	{ "RPMNS_TYPE_UNAME", RPMNS_TYPE_UNAME }, 
	{ "RPMNS_TYPE_SONAME", RPMNS_TYPE_SONAME }, 
	{ "RPMNS_TYPE_ACCESS", RPMNS_TYPE_ACCESS }, 
	{ "RPMNS_TYPE_TAG", RPMNS_TYPE_TAG }, 
	{ "RPMNS_TYPE_USER", RPMNS_TYPE_USER }, 
	{ "RPMNS_TYPE_GROUP", RPMNS_TYPE_GROUP }, 
	{ "RPMNS_TYPE_MOUNTED", RPMNS_TYPE_MOUNTED }, 
	{ "RPMNS_TYPE_DISKSPACE", RPMNS_TYPE_DISKSPACE }, 
	{ "RPMNS_TYPE_DIGEST", RPMNS_TYPE_DIGEST }, 
	{ "RPMNS_TYPE_GNUPG", RPMNS_TYPE_GNUPG }, 
	{ "RPMNS_TYPE_MACRO", RPMNS_TYPE_MACRO }, 
	{ "RPMNS_TYPE_ENVVAR", RPMNS_TYPE_ENVVAR }, 
	{ "RPMNS_TYPE_RUNNING", RPMNS_TYPE_RUNNING }, 
	{ "RPMNS_TYPE_SANITY", RPMNS_TYPE_SANITY }, 
	{ "RPMNS_TYPE_VCHECK", RPMNS_TYPE_VCHECK }, 
	{ "RPMNS_TYPE_SIGNATURE", RPMNS_TYPE_SIGNATURE }, 
	{ "RPMNS_TYPE_VERIFY", RPMNS_TYPE_VERIFY }, 
	{ "RPMNS_TYPE_CONFIG", RPMNS_TYPE_CONFIG }, 
	{ "RPMNS_TYPE_SET", RPMNS_TYPE_SET }, 
	{ "RPMNS_TYPE_DPKG", RPMNS_TYPE_DPKG }, 
#endif /* H_RPMNS */
	{ NULL, 0 } /* NULL terminated (rpmnsType) */
};
const struct rpmconstant_s * rpmnsTypectable = rpmnsTypectbl;


/* From rpmtag.h
 * prefix tag: RPM_
 * table: rpmTagType */
#include "rpmtag.h"
static const struct rpmconstant_s rpmTagTypectbl[] = {
#ifdef H_RPMTAG
	{ "RPM_UINT8", RPM_UINT8_TYPE }, 
	{ "RPM_UINT16", RPM_UINT16_TYPE }, 
	{ "RPM_UINT32", RPM_UINT32_TYPE }, 
	{ "RPM_UINT64", RPM_UINT64_TYPE }, 
	{ "RPM_STRING", RPM_STRING_TYPE }, 
	{ "RPM_BIN", RPM_BIN_TYPE }, 
	{ "RPM_STRING_ARRAY", RPM_STRING_ARRAY_TYPE }, 
	{ "RPM_I18NSTRING", RPM_I18NSTRING_TYPE }, 
	{ "RPM_MIN", RPM_MIN_TYPE }, 
	{ "RPM_MAX", RPM_MAX_TYPE }, 
	{ "RPM_MASK", RPM_MASK_TYPE }, 
	{ "RPM_REGION", RPM_REGION_TYPE }, 
	{ "RPM_BIN_ARRAY", RPM_BIN_ARRAY_TYPE }, 
	{ "RPM_XREF", RPM_XREF_TYPE }, 
	{ "RPM_ANY_RETURN", RPM_ANY_RETURN_TYPE }, 
	{ "RPM_SCALAR_RETURN", RPM_SCALAR_RETURN_TYPE }, 
	{ "RPM_ARRAY_RETURN", RPM_ARRAY_RETURN_TYPE }, 
	{ "RPM_MAPPING_RETURN", RPM_MAPPING_RETURN_TYPE }, 
	{ "RPM_PROBE_RETURN", RPM_PROBE_RETURN_TYPE }, 
	{ "RPM_TREE_RETURN", RPM_TREE_RETURN_TYPE }, 
	{ "RPM_OPENPGP_RETURN", RPM_OPENPGP_RETURN_TYPE }, 
	{ "RPM_X509_RETURN", RPM_X509_RETURN_TYPE }, 
	{ "RPM_ASN1_RETURN", RPM_ASN1_RETURN_TYPE }, 
	{ "RPM_OPAQUE_RETURN", RPM_OPAQUE_RETURN_TYPE }, 
	{ "RPM_MASK_RETURN", RPM_MASK_RETURN_TYPE }, 
#endif /* H_RPMTAG */
	{ NULL, 0 } /* NULL terminated (rpmTagType) */
};
const struct rpmconstant_s * rpmTagTypectable = rpmTagTypectbl;


/* From rpmtag.h
 * prefix tag: RPMSIGTAG_
 * table: rpmtagSignature */
#include "rpmtag.h"
static const struct rpmconstant_s rpmtagSignaturectbl[] = {
#ifdef H_RPMTAG
	{ "RPMSIGTAG_SIZE", RPMSIGTAG_SIZE }, /*!< internal Header+Payload size in bytes. */
	{ "RPMSIGTAG_LEMD5_1", RPMSIGTAG_LEMD5_1 }, /*!< internal Broken MD5, take 1 @deprecated legacy. */
	{ "RPMSIGTAG_PGP", RPMSIGTAG_PGP }, /*!< internal PGP 2.6.3 signature. */
	{ "RPMSIGTAG_LEMD5_2", RPMSIGTAG_LEMD5_2 }, /*!< internal Broken MD5, take 2 @deprecated legacy. */
	{ "RPMSIGTAG_MD5", RPMSIGTAG_MD5 }, /*!< internal MD5 signature. */
	{ "RPMSIGTAG_GPG", RPMSIGTAG_GPG }, /*!< internal GnuPG signature. */
	{ "RPMSIGTAG_PGP5", RPMSIGTAG_PGP5 }, /*!< internal PGP5 signature @deprecated legacy. */
	{ "RPMSIGTAG_PAYLOADSIZE", RPMSIGTAG_PAYLOADSIZE }, /*!< internal uncompressed payload size in bytes. */
	{ "RPMSIGTAG_BADSHA1_1", RPMSIGTAG_BADSHA1_1 }, /*!< internal Broken SHA1, take 1. */
	{ "RPMSIGTAG_BADSHA1_2", RPMSIGTAG_BADSHA1_2 }, /*!< internal Broken SHA1, take 2. */
	{ "RPMSIGTAG_SHA1", RPMSIGTAG_SHA1 }, /*!< internal sha1 header digest. */
	{ "RPMSIGTAG_DSA", RPMSIGTAG_DSA }, /*!< internal DSA header signature. */
	{ "RPMSIGTAG_RSA", RPMSIGTAG_RSA }, /*!< internal RSA header signature. */
	{ "RPMSIGTAG_ECDSA", RPMSIGTAG_ECDSA }, /*!< internal ECDSA header signature. */
	{ "RPMSIGTAG_PADDING", RPMSIGTAG_PADDING }, /*!< signature header padding */
#endif /* H_RPMTAG */
	{ NULL, 0 } /* NULL terminated (rpmtagSignature) */
};
const struct rpmconstant_s * rpmtagSignaturectable = rpmtagSignaturectbl;


/* From rpmtag.h
 * prefix tag: RPMTAG_
 * table: rpmTag */
#include "rpmtag.h"
static const struct rpmconstant_s rpmTagctbl[] = {
#ifdef H_RPMTAG
	{ "RPMTAG_HEADERIMAGE", RPMTAG_HEADERIMAGE }, /*!< internal Current image. */
	{ "RPMTAG_HEADERSIGNATURES", RPMTAG_HEADERSIGNATURES }, /*!< internal Signatures. */
	{ "RPMTAG_HEADERIMMUTABLE", RPMTAG_HEADERIMMUTABLE }, /*!< x Original image. */
	{ "RPMTAG_HEADERREGIONS", RPMTAG_HEADERREGIONS }, /*!< internal Regions. */
	{ "RPMTAG_HEADERI18NTABLE", RPMTAG_HEADERI18NTABLE }, /*!< s[] I18N string locales. */
	{ "RPMTAG_SIG_BASE", RPMTAG_SIG_BASE }, 
	{ "RPMTAG_SIGSIZE", RPMTAG_SIGSIZE }, /* i */
	{ "RPMTAG_SIGLEMD5_1", RPMTAG_SIGLEMD5_1 }, /* internal - obsolete */
	{ "RPMTAG_SIGPGP", RPMTAG_SIGPGP }, /* x */
	{ "RPMTAG_SIGLEMD5_2", RPMTAG_SIGLEMD5_2 }, /* x internal - obsolete */
	{ "RPMTAG_SIGMD5", RPMTAG_SIGMD5 }, /* x */
	{ "RPMTAG_PKGID", RPMTAG_PKGID }, /* x */
	{ "RPMTAG_SIGGPG", RPMTAG_SIGGPG }, /* x */
	{ "RPMTAG_SIGPGP5", RPMTAG_SIGPGP5 }, /* internal - obsolete */
	{ "RPMTAG_BADSHA1_1", RPMTAG_BADSHA1_1 }, /* internal - obsolete */
	{ "RPMTAG_BADSHA1_2", RPMTAG_BADSHA1_2 }, /* internal - obsolete */
	{ "RPMTAG_PUBKEYS", RPMTAG_PUBKEYS }, /* s[] */
	{ "RPMTAG_DSAHEADER", RPMTAG_DSAHEADER }, /* x */
	{ "RPMTAG_RSAHEADER", RPMTAG_RSAHEADER }, /* x */
	{ "RPMTAG_SHA1HEADER", RPMTAG_SHA1HEADER }, /* s */
	{ "RPMTAG_HDRID", RPMTAG_HDRID }, /* s */
	{ "RPMTAG_ECDSAHEADER", RPMTAG_ECDSAHEADER }, /* x */
	{ "RPMTAG_NAME", RPMTAG_NAME }, /* s */
	{ "RPMTAG_N", RPMTAG_N }, /* s */
	{ "RPMTAG_VERSION", RPMTAG_VERSION }, /* s */
	{ "RPMTAG_V", RPMTAG_V }, /* s */
	{ "RPMTAG_RELEASE", RPMTAG_RELEASE }, /* s */
	{ "RPMTAG_R", RPMTAG_R }, /* s */
	{ "RPMTAG_EPOCH", RPMTAG_EPOCH }, /* i */
	{ "RPMTAG_E", RPMTAG_E }, /* i */
	{ "RPMTAG_SUMMARY", RPMTAG_SUMMARY }, /* s */
	{ "RPMTAG_DESCRIPTION", RPMTAG_DESCRIPTION }, /* s */
	{ "RPMTAG_BUILDTIME", RPMTAG_BUILDTIME }, /* i */
	{ "RPMTAG_BUILDHOST", RPMTAG_BUILDHOST }, /* s */
	{ "RPMTAG_INSTALLTIME", RPMTAG_INSTALLTIME }, /* i[] */
	{ "RPMTAG_SIZE", RPMTAG_SIZE }, /* i */
	{ "RPMTAG_DISTRIBUTION", RPMTAG_DISTRIBUTION }, /* s */
	{ "RPMTAG_VENDOR", RPMTAG_VENDOR }, /* s */
	{ "RPMTAG_GIF", RPMTAG_GIF }, /* x */
	{ "RPMTAG_XPM", RPMTAG_XPM }, /* x */
	{ "RPMTAG_LICENSE", RPMTAG_LICENSE }, /* s */
	{ "RPMTAG_PACKAGER", RPMTAG_PACKAGER }, /* s */
	{ "RPMTAG_GROUP", RPMTAG_GROUP }, /* s */
	{ "RPMTAG_CHANGELOG", RPMTAG_CHANGELOG }, /* s[] internal */
	{ "RPMTAG_SOURCE", RPMTAG_SOURCE }, /* s[] */
	{ "RPMTAG_PATCH", RPMTAG_PATCH }, /* s[] */
	{ "RPMTAG_URL", RPMTAG_URL }, /* s */
	{ "RPMTAG_OS", RPMTAG_OS }, /* s legacy used int */
	{ "RPMTAG_ARCH", RPMTAG_ARCH }, /* s legacy used int */
	{ "RPMTAG_PREIN", RPMTAG_PREIN }, /* s */
	{ "RPMTAG_POSTIN", RPMTAG_POSTIN }, /* s */
	{ "RPMTAG_PREUN", RPMTAG_PREUN }, /* s */
	{ "RPMTAG_POSTUN", RPMTAG_POSTUN }, /* s */
	{ "RPMTAG_OLDFILENAMES", RPMTAG_OLDFILENAMES }, /* s[] obsolete */
	{ "RPMTAG_FILESIZES", RPMTAG_FILESIZES }, /* i[] */
	{ "RPMTAG_FILESTATES", RPMTAG_FILESTATES }, /* c[] */
	{ "RPMTAG_FILEMODES", RPMTAG_FILEMODES }, /* h[] */
	{ "RPMTAG_FILEUIDS", RPMTAG_FILEUIDS }, /* i[] internal */
	{ "RPMTAG_FILEGIDS", RPMTAG_FILEGIDS }, /* i[] internal */
	{ "RPMTAG_FILERDEVS", RPMTAG_FILERDEVS }, /* h[] */
	{ "RPMTAG_FILEMTIMES", RPMTAG_FILEMTIMES }, /* i[] */
	{ "RPMTAG_FILEDIGESTS", RPMTAG_FILEDIGESTS }, /* s[] */
	{ "RPMTAG_FILEMD5S", RPMTAG_FILEMD5S }, /* s[] */
	{ "RPMTAG_FILELINKTOS", RPMTAG_FILELINKTOS }, /* s[] */
	{ "RPMTAG_FILEFLAGS", RPMTAG_FILEFLAGS }, /* i[] */
	{ "RPMTAG_ROOT", RPMTAG_ROOT }, /* internal - obsolete */
	{ "RPMTAG_FILEUSERNAME", RPMTAG_FILEUSERNAME }, /* s[] */
	{ "RPMTAG_FILEGROUPNAME", RPMTAG_FILEGROUPNAME }, /* s[] */
	{ "RPMTAG_EXCLUDE", RPMTAG_EXCLUDE }, /* internal - obsolete */
	{ "RPMTAG_EXCLUSIVE", RPMTAG_EXCLUSIVE }, /* internal - obsolete */
	{ "RPMTAG_ICON", RPMTAG_ICON }, /* x */
	{ "RPMTAG_SOURCERPM", RPMTAG_SOURCERPM }, /* s */
	{ "RPMTAG_FILEVERIFYFLAGS", RPMTAG_FILEVERIFYFLAGS }, /* i[] */
	{ "RPMTAG_ARCHIVESIZE", RPMTAG_ARCHIVESIZE }, /* i */
	{ "RPMTAG_PROVIDENAME", RPMTAG_PROVIDENAME }, /* s[] */
	{ "RPMTAG_PROVIDES", RPMTAG_PROVIDES }, /* s[] */
	{ "RPMTAG_P", RPMTAG_P }, /* s[] */
	{ "RPMTAG_REQUIREFLAGS", RPMTAG_REQUIREFLAGS }, /* i[] */
	{ "RPMTAG_REQUIRENAME", RPMTAG_REQUIRENAME }, /* s[] */
	{ "RPMTAG_REQUIRES", RPMTAG_REQUIRES }, /* s[] */
	{ "RPMTAG_REQUIREVERSION", RPMTAG_REQUIREVERSION }, /* s[] */
	{ "RPMTAG_NOSOURCE", RPMTAG_NOSOURCE }, /* i[] internal */
	{ "RPMTAG_NOPATCH", RPMTAG_NOPATCH }, /* i[] internal */
	{ "RPMTAG_CONFLICTFLAGS", RPMTAG_CONFLICTFLAGS }, /* i[] */
	{ "RPMTAG_CONFLICTNAME", RPMTAG_CONFLICTNAME }, /* s[] */
	{ "RPMTAG_CONFLICTS", RPMTAG_CONFLICTS }, /* s[] */
	{ "RPMTAG_C", RPMTAG_C }, /* s[] */
	{ "RPMTAG_CONFLICTVERSION", RPMTAG_CONFLICTVERSION }, /* s[] */
	{ "RPMTAG_DEFAULTPREFIX", RPMTAG_DEFAULTPREFIX }, /* s internal - deprecated */
	{ "RPMTAG_BUILDROOT", RPMTAG_BUILDROOT }, /* s internal */
	{ "RPMTAG_INSTALLPREFIX", RPMTAG_INSTALLPREFIX }, /* s internal - deprecated */
	{ "RPMTAG_EXCLUDEARCH", RPMTAG_EXCLUDEARCH }, /* s[] */
	{ "RPMTAG_EXCLUDEOS", RPMTAG_EXCLUDEOS }, /* s[] */
	{ "RPMTAG_EXCLUSIVEARCH", RPMTAG_EXCLUSIVEARCH }, /* s[] */
	{ "RPMTAG_EXCLUSIVEOS", RPMTAG_EXCLUSIVEOS }, /* s[] */
	{ "RPMTAG_AUTOREQPROV", RPMTAG_AUTOREQPROV }, /* s internal */
	{ "RPMTAG_RPMVERSION", RPMTAG_RPMVERSION }, /* s */
	{ "RPMTAG_TRIGGERSCRIPTS", RPMTAG_TRIGGERSCRIPTS }, /* s[] */
	{ "RPMTAG_TRIGGERNAME", RPMTAG_TRIGGERNAME }, /* s[] */
	{ "RPMTAG_TRIGGERVERSION", RPMTAG_TRIGGERVERSION }, /* s[] */
	{ "RPMTAG_TRIGGERFLAGS", RPMTAG_TRIGGERFLAGS }, /* i[] */
	{ "RPMTAG_TRIGGERINDEX", RPMTAG_TRIGGERINDEX }, /* i[] */
	{ "RPMTAG_VERIFYSCRIPT", RPMTAG_VERIFYSCRIPT }, /* s */
	{ "RPMTAG_CHANGELOGTIME", RPMTAG_CHANGELOGTIME }, /* i[] */
	{ "RPMTAG_CHANGELOGNAME", RPMTAG_CHANGELOGNAME }, /* s[] */
	{ "RPMTAG_CHANGELOGTEXT", RPMTAG_CHANGELOGTEXT }, /* s[] */
	{ "RPMTAG_BROKENMD5", RPMTAG_BROKENMD5 }, /* internal - obsolete */
	{ "RPMTAG_PREREQ", RPMTAG_PREREQ }, /* internal */
	{ "RPMTAG_PREINPROG", RPMTAG_PREINPROG }, /* s */
	{ "RPMTAG_POSTINPROG", RPMTAG_POSTINPROG }, /* s */
	{ "RPMTAG_PREUNPROG", RPMTAG_PREUNPROG }, /* s */
	{ "RPMTAG_POSTUNPROG", RPMTAG_POSTUNPROG }, /* s */
	{ "RPMTAG_BUILDARCHS", RPMTAG_BUILDARCHS }, /* s[] */
	{ "RPMTAG_OBSOLETENAME", RPMTAG_OBSOLETENAME }, /* s[] */
	{ "RPMTAG_OBSOLETES", RPMTAG_OBSOLETES }, /* s[] */
	{ "RPMTAG_O", RPMTAG_O }, /* s[] */
	{ "RPMTAG_VERIFYSCRIPTPROG", RPMTAG_VERIFYSCRIPTPROG }, /* s */
	{ "RPMTAG_TRIGGERSCRIPTPROG", RPMTAG_TRIGGERSCRIPTPROG }, /* s[] */
	{ "RPMTAG_DOCDIR", RPMTAG_DOCDIR }, /* internal */
	{ "RPMTAG_COOKIE", RPMTAG_COOKIE }, /* s */
	{ "RPMTAG_FILEDEVICES", RPMTAG_FILEDEVICES }, /* i[] */
	{ "RPMTAG_FILEINODES", RPMTAG_FILEINODES }, /* i[] */
	{ "RPMTAG_FILELANGS", RPMTAG_FILELANGS }, /* s[] */
	{ "RPMTAG_PREFIXES", RPMTAG_PREFIXES }, /* s[] */
	{ "RPMTAG_INSTPREFIXES", RPMTAG_INSTPREFIXES }, /* s[] */
	{ "RPMTAG_TRIGGERIN", RPMTAG_TRIGGERIN }, /* internal */
	{ "RPMTAG_TRIGGERUN", RPMTAG_TRIGGERUN }, /* internal */
	{ "RPMTAG_TRIGGERPOSTUN", RPMTAG_TRIGGERPOSTUN }, /* internal */
	{ "RPMTAG_AUTOREQ", RPMTAG_AUTOREQ }, /* internal */
	{ "RPMTAG_AUTOPROV", RPMTAG_AUTOPROV }, /* internal */
	{ "RPMTAG_CAPABILITY", RPMTAG_CAPABILITY }, /* i legacy - obsolete */
	{ "RPMTAG_SOURCEPACKAGE", RPMTAG_SOURCEPACKAGE }, /* i legacy - obsolete */
	{ "RPMTAG_OLDORIGFILENAMES", RPMTAG_OLDORIGFILENAMES }, /* internal - obsolete */
	{ "RPMTAG_BUILDPREREQ", RPMTAG_BUILDPREREQ }, /* internal */
	{ "RPMTAG_BUILDREQUIRES", RPMTAG_BUILDREQUIRES }, /* internal */
	{ "RPMTAG_BUILDCONFLICTS", RPMTAG_BUILDCONFLICTS }, /* internal */
	{ "RPMTAG_BUILDMACROS", RPMTAG_BUILDMACROS }, /* s[] srpms only */
	{ "RPMTAG_PROVIDEFLAGS", RPMTAG_PROVIDEFLAGS }, /* i[] */
	{ "RPMTAG_PROVIDEVERSION", RPMTAG_PROVIDEVERSION }, /* s[] */
	{ "RPMTAG_OBSOLETEFLAGS", RPMTAG_OBSOLETEFLAGS }, /* i[] */
	{ "RPMTAG_OBSOLETEVERSION", RPMTAG_OBSOLETEVERSION }, /* s[] */
	{ "RPMTAG_DIRINDEXES", RPMTAG_DIRINDEXES }, /* i[] */
	{ "RPMTAG_BASENAMES", RPMTAG_BASENAMES }, /* s[] */
	{ "RPMTAG_DIRNAMES", RPMTAG_DIRNAMES }, /* s[] */
	{ "RPMTAG_ORIGDIRINDEXES", RPMTAG_ORIGDIRINDEXES }, /* i[] relocation */
	{ "RPMTAG_ORIGBASENAMES", RPMTAG_ORIGBASENAMES }, /* s[] relocation */
	{ "RPMTAG_ORIGDIRNAMES", RPMTAG_ORIGDIRNAMES }, /* s[] relocation */
	{ "RPMTAG_OPTFLAGS", RPMTAG_OPTFLAGS }, /* s */
	{ "RPMTAG_DISTURL", RPMTAG_DISTURL }, /* s */
	{ "RPMTAG_PAYLOADFORMAT", RPMTAG_PAYLOADFORMAT }, /* s */
	{ "RPMTAG_PAYLOADCOMPRESSOR", RPMTAG_PAYLOADCOMPRESSOR }, /* s */
	{ "RPMTAG_PAYLOADFLAGS", RPMTAG_PAYLOADFLAGS }, /* s */
	{ "RPMTAG_INSTALLCOLOR", RPMTAG_INSTALLCOLOR }, /* i transaction color when installed */
	{ "RPMTAG_INSTALLTID", RPMTAG_INSTALLTID }, /* i[] */
	{ "RPMTAG_REMOVETID", RPMTAG_REMOVETID }, /* i[] */
	{ "RPMTAG_SHA1RHN", RPMTAG_SHA1RHN }, /* internal - obsolete */
	{ "RPMTAG_RHNPLATFORM", RPMTAG_RHNPLATFORM }, /* s deprecated */
	{ "RPMTAG_PLATFORM", RPMTAG_PLATFORM }, /* s */
	{ "RPMTAG_PATCHESNAME", RPMTAG_PATCHESNAME }, /* s[] deprecated placeholder (SuSE) */
	{ "RPMTAG_PATCHESFLAGS", RPMTAG_PATCHESFLAGS }, /* i[] deprecated placeholder (SuSE) */
	{ "RPMTAG_PATCHESVERSION", RPMTAG_PATCHESVERSION }, /* s[] deprecated placeholder (SuSE) */
	{ "RPMTAG_CACHECTIME", RPMTAG_CACHECTIME }, /* i rpmcache(8) only */
	{ "RPMTAG_CACHEPKGPATH", RPMTAG_CACHEPKGPATH }, /* s rpmcache(8) only */
	{ "RPMTAG_CACHEPKGSIZE", RPMTAG_CACHEPKGSIZE }, /* i rpmcache(8) only */
	{ "RPMTAG_CACHEPKGMTIME", RPMTAG_CACHEPKGMTIME }, /* i rpmcache(8) only */
	{ "RPMTAG_FILECOLORS", RPMTAG_FILECOLORS }, /* i[] */
	{ "RPMTAG_FILECLASS", RPMTAG_FILECLASS }, /* i[] */
	{ "RPMTAG_CLASSDICT", RPMTAG_CLASSDICT }, /* s[] */
	{ "RPMTAG_FILEDEPENDSX", RPMTAG_FILEDEPENDSX }, /* i[] */
	{ "RPMTAG_FILEDEPENDSN", RPMTAG_FILEDEPENDSN }, /* i[] */
	{ "RPMTAG_DEPENDSDICT", RPMTAG_DEPENDSDICT }, /* i[] */
	{ "RPMTAG_SOURCEPKGID", RPMTAG_SOURCEPKGID }, /* x */
	{ "RPMTAG_FILECONTEXTS", RPMTAG_FILECONTEXTS }, /* s[] */
	{ "RPMTAG_FSCONTEXTS", RPMTAG_FSCONTEXTS }, /* s[] extension */
	{ "RPMTAG_RECONTEXTS", RPMTAG_RECONTEXTS }, /* s[] extension */
	{ "RPMTAG_POLICIES", RPMTAG_POLICIES }, /* s[] selinux *.te policy file. */
	{ "RPMTAG_PRETRANS", RPMTAG_PRETRANS }, /* s */
	{ "RPMTAG_POSTTRANS", RPMTAG_POSTTRANS }, /* s */
	{ "RPMTAG_PRETRANSPROG", RPMTAG_PRETRANSPROG }, /* s */
	{ "RPMTAG_POSTTRANSPROG", RPMTAG_POSTTRANSPROG }, /* s */
	{ "RPMTAG_DISTTAG", RPMTAG_DISTTAG }, /* s */
	{ "RPMTAG_SUGGESTSNAME", RPMTAG_SUGGESTSNAME }, /* s[] extension */
	{ "RPMTAG_SUGGESTS", RPMTAG_SUGGESTS }, /* s[] */
	{ "RPMTAG_SUGGESTSVERSION", RPMTAG_SUGGESTSVERSION }, /* s[] extension */
	{ "RPMTAG_SUGGESTSFLAGS", RPMTAG_SUGGESTSFLAGS }, /* i[] extension */
	{ "RPMTAG_ENHANCESNAME", RPMTAG_ENHANCESNAME }, /* s[] extension placeholder */
	{ "RPMTAG_ENHANCES", RPMTAG_ENHANCES }, /* s[] */
	{ "RPMTAG_ENHANCESVERSION", RPMTAG_ENHANCESVERSION }, /* s[] extension placeholder */
	{ "RPMTAG_ENHANCESFLAGS", RPMTAG_ENHANCESFLAGS }, /* i[] extension placeholder */
	{ "RPMTAG_PRIORITY", RPMTAG_PRIORITY }, /* i[] extension placeholder */
	{ "RPMTAG_CVSID", RPMTAG_CVSID }, /* s */
	{ "RPMTAG_SVNID", RPMTAG_SVNID }, /* s */
	{ "RPMTAG_BLINKPKGID", RPMTAG_BLINKPKGID }, /* s[] */
	{ "RPMTAG_BLINKHDRID", RPMTAG_BLINKHDRID }, /* s[] */
	{ "RPMTAG_BLINKNEVRA", RPMTAG_BLINKNEVRA }, /* s[] */
	{ "RPMTAG_FLINKPKGID", RPMTAG_FLINKPKGID }, /* s[] */
	{ "RPMTAG_FLINKHDRID", RPMTAG_FLINKHDRID }, /* s[] */
	{ "RPMTAG_FLINKNEVRA", RPMTAG_FLINKNEVRA }, /* s[] */
	{ "RPMTAG_PACKAGEORIGIN", RPMTAG_PACKAGEORIGIN }, /* s */
	{ "RPMTAG_TRIGGERPREIN", RPMTAG_TRIGGERPREIN }, /* internal */
	{ "RPMTAG_BUILDSUGGESTS", RPMTAG_BUILDSUGGESTS }, /* internal */
	{ "RPMTAG_BUILDENHANCES", RPMTAG_BUILDENHANCES }, /* internal */
	{ "RPMTAG_SCRIPTSTATES", RPMTAG_SCRIPTSTATES }, /* i[] scriptlet exit codes */
	{ "RPMTAG_SCRIPTMETRICS", RPMTAG_SCRIPTMETRICS }, /* i[] scriptlet execution times */
	{ "RPMTAG_BUILDCPUCLOCK", RPMTAG_BUILDCPUCLOCK }, /* i */
	{ "RPMTAG_FILEDIGESTALGOS", RPMTAG_FILEDIGESTALGOS }, /* i[] */
	{ "RPMTAG_VARIANTS", RPMTAG_VARIANTS }, /* s[] */
	{ "RPMTAG_XMAJOR", RPMTAG_XMAJOR }, /* i */
	{ "RPMTAG_XMINOR", RPMTAG_XMINOR }, /* i */
	{ "RPMTAG_REPOTAG", RPMTAG_REPOTAG }, /* s */
	{ "RPMTAG_KEYWORDS", RPMTAG_KEYWORDS }, /* s[] */
	{ "RPMTAG_BUILDPLATFORMS", RPMTAG_BUILDPLATFORMS }, /* s[] */
	{ "RPMTAG_PACKAGECOLOR", RPMTAG_PACKAGECOLOR }, /* i */
	{ "RPMTAG_PACKAGEPREFCOLOR", RPMTAG_PACKAGEPREFCOLOR }, /* i (unimplemented) */
	{ "RPMTAG_XATTRSDICT", RPMTAG_XATTRSDICT }, /* s[] (unimplemented) */
	{ "RPMTAG_FILEXATTRSX", RPMTAG_FILEXATTRSX }, /* i[] (unimplemented) */
	{ "RPMTAG_DEPATTRSDICT", RPMTAG_DEPATTRSDICT }, /* s[] (unimplemented) */
	{ "RPMTAG_CONFLICTATTRSX", RPMTAG_CONFLICTATTRSX }, /* i[] (unimplemented) */
	{ "RPMTAG_OBSOLETEATTRSX", RPMTAG_OBSOLETEATTRSX }, /* i[] (unimplemented) */
	{ "RPMTAG_PROVIDEATTRSX", RPMTAG_PROVIDEATTRSX }, /* i[] (unimplemented) */
	{ "RPMTAG_REQUIREATTRSX", RPMTAG_REQUIREATTRSX }, /* i[] (unimplemented) */
	{ "RPMTAG_BUILDPROVIDES", RPMTAG_BUILDPROVIDES }, /* internal */
	{ "RPMTAG_BUILDOBSOLETES", RPMTAG_BUILDOBSOLETES }, /* internal */
	{ "RPMTAG_DBINSTANCE", RPMTAG_DBINSTANCE }, /* i */
	{ "RPMTAG_NVRA", RPMTAG_NVRA }, /* s */
	{ "RPMTAG_FILEPATHS", RPMTAG_FILEPATHS }, /* s[] */
	{ "RPMTAG_ORIGPATHS", RPMTAG_ORIGPATHS }, /* s[] */
	{ "RPMTAG_RPMLIBVERSION", RPMTAG_RPMLIBVERSION }, /* i */
	{ "RPMTAG_RPMLIBTIMESTAMP", RPMTAG_RPMLIBTIMESTAMP }, /* i */
	{ "RPMTAG_RPMLIBVENDOR", RPMTAG_RPMLIBVENDOR }, /* i */
	{ "RPMTAG_CLASS", RPMTAG_CLASS }, /* s arbitrary */
	{ "RPMTAG_TRACK", RPMTAG_TRACK }, /* s internal arbitrary */
	{ "RPMTAG_TRACKPROG", RPMTAG_TRACKPROG }, /* s internal arbitrary */
	{ "RPMTAG_SANITYCHECK", RPMTAG_SANITYCHECK }, /* s */
	{ "RPMTAG_SANITYCHECKPROG", RPMTAG_SANITYCHECKPROG }, /* s */
	{ "RPMTAG_FILESTAT", RPMTAG_FILESTAT }, /* s[] stat(2) from metadata extension*/
	{ "RPMTAG_STAT", RPMTAG_STAT }, /* s[] stat(2) from disk extension */
	{ "RPMTAG_ORIGINTID", RPMTAG_ORIGINTID }, /* i[] */
	{ "RPMTAG_ORIGINTIME", RPMTAG_ORIGINTIME }, /* i[] */
	{ "RPMTAG_HEADERSTARTOFF", RPMTAG_HEADERSTARTOFF }, /* l */
	{ "RPMTAG_HEADERENDOFF", RPMTAG_HEADERENDOFF }, /* l */
	{ "RPMTAG_PACKAGETIME", RPMTAG_PACKAGETIME }, /* l */
	{ "RPMTAG_PACKAGESIZE", RPMTAG_PACKAGESIZE }, /* l */
	{ "RPMTAG_PACKAGEDIGEST", RPMTAG_PACKAGEDIGEST }, /* s */
	{ "RPMTAG_PACKAGESTAT", RPMTAG_PACKAGESTAT }, /* x */
	{ "RPMTAG_PACKAGEBASEURL", RPMTAG_PACKAGEBASEURL }, /* s */
	{ "RPMTAG_DISTEPOCH", RPMTAG_DISTEPOCH }, /* s */
	{ "RPMTAG_D", RPMTAG_D }, /* s */
	{ "RPMTAG_CONFLICTYAMLENTRY", RPMTAG_CONFLICTYAMLENTRY }, /* s[] */
	{ "RPMTAG_OBSOLETEYAMLENTRY", RPMTAG_OBSOLETEYAMLENTRY }, /* s[] */
	{ "RPMTAG_PROVIDEYAMLENTRY", RPMTAG_PROVIDEYAMLENTRY }, /* s[] */
	{ "RPMTAG_REQUIREYAMLENTRY", RPMTAG_REQUIREYAMLENTRY }, /* s[] */
	{ "RPMTAG_FILEDIGESTALGO", RPMTAG_FILEDIGESTALGO }, /* i file checksum algorithm */
	{ "RPMTAG_BUGURL", RPMTAG_BUGURL }, /* s */
	{ "RPMTAG_FIRSTFREE_TAG", RPMTAG_FIRSTFREE_TAG }, /*!< internal */
	{ "RPMTAG_PACKAGETRANSFLAGS", RPMTAG_PACKAGETRANSFLAGS }, /* s[] arbitrary */
	{ "RPMTAG_PACKAGEDEPFLAGS", RPMTAG_PACKAGEDEPFLAGS }, /* s[] arbitrary */
	{ "RPMTAG_BUILDPREPPROG", RPMTAG_BUILDPREPPROG }, /* s[] arbitrary */
	{ "RPMTAG_BUILDPREP", RPMTAG_BUILDPREP }, /* s[] arbitrary */
	{ "RPMTAG_BUILDBUILDPROG", RPMTAG_BUILDBUILDPROG }, /* s[] arbitrary */
	{ "RPMTAG_BUILDBUILD", RPMTAG_BUILDBUILD }, /* s[] arbitrary */
	{ "RPMTAG_BUILDINSTALLPROG", RPMTAG_BUILDINSTALLPROG }, /* s[] arbitrary */
	{ "RPMTAG_BUILDINSTALL", RPMTAG_BUILDINSTALL }, /* s[] arbitrary */
	{ "RPMTAG_BUILDCHECKPROG", RPMTAG_BUILDCHECKPROG }, /* s[] arbitrary */
	{ "RPMTAG_BUILDCHECK", RPMTAG_BUILDCHECK }, /* s[] arbitrary */
	{ "RPMTAG_BUILDCLEANPROG", RPMTAG_BUILDCLEANPROG }, /* s[] arbitrary */
	{ "RPMTAG_BUILDCLEAN", RPMTAG_BUILDCLEAN }, /* s[] arbitrary */
	{ "RPMTAG_LASTARBITRARY_TAG", RPMTAG_LASTARBITRARY_TAG }, /*!< internal */
	{ "RPMTAG_EXTERNAL_TAG", RPMTAG_EXTERNAL_TAG }, 
#endif /* H_RPMTAG */
	{ NULL, 0 } /* NULL terminated (rpmTag) */
};
const struct rpmconstant_s * rpmTagctable = rpmTagctbl;


/* From rpmtag.h
 * prefix tag: RPMDBI_
 * table: rpmdbi */
#include "rpmtag.h"
static const struct rpmconstant_s rpmdbictbl[] = {
#ifdef H_RPMTAG
	{ "RPMDBI_PACKAGES", RPMDBI_PACKAGES }, 
	{ "RPMDBI_DEPCACHE", RPMDBI_DEPCACHE }, 
	{ "RPMDBI_ADDED", RPMDBI_ADDED }, 
	{ "RPMDBI_REMOVED", RPMDBI_REMOVED }, 
	{ "RPMDBI_AVAILABLE", RPMDBI_AVAILABLE }, 
	{ "RPMDBI_HDLIST", RPMDBI_HDLIST }, 
	{ "RPMDBI_ARGLIST", RPMDBI_ARGLIST }, 
	{ "RPMDBI_FTSWALK", RPMDBI_FTSWALK }, 
	{ "RPMDBI_SEQNO", RPMDBI_SEQNO }, 
	{ "RPMDBI_BTREE", RPMDBI_BTREE }, 
	{ "RPMDBI_HASH", RPMDBI_HASH }, 
	{ "RPMDBI_QUEUE", RPMDBI_QUEUE }, 
	{ "RPMDBI_RECNO", RPMDBI_RECNO }, 
	{ "RPMDBI_HEAP", RPMDBI_HEAP }, 
	{ "RPMDBI_REPACKAGES", RPMDBI_REPACKAGES }, 
	{ "RPMDBI_REPOSITORY", RPMDBI_REPOSITORY }, 
#endif /* H_RPMTAG */
	{ NULL, 0 } /* NULL terminated (rpmdbi) */
};
const struct rpmconstant_s * rpmdbictable = rpmdbictbl;


/* From rpmtypes.h
 * prefix tag: RPMSCRIPT_STATE_
 * table: rpmScriptState */
#include "rpmtypes.h"
static const struct rpmconstant_s rpmScriptStatectbl[] = {
#ifdef _H_RPMTYPES_
	{ "RPMSCRIPT_STATE_UNKNOWN", RPMSCRIPT_STATE_UNKNOWN }, 
	{ "RPMSCRIPT_STATE_EXEC", RPMSCRIPT_STATE_EXEC }, 
	{ "RPMSCRIPT_STATE_REAPED", RPMSCRIPT_STATE_REAPED }, 
	{ "RPMSCRIPT_STATE_SELINUX", RPMSCRIPT_STATE_SELINUX }, 
	{ "RPMSCRIPT_STATE_EMBEDDED", RPMSCRIPT_STATE_EMBEDDED }, 
#endif /* _H_RPMTYPES_ */
	{ NULL, 0 } /* NULL terminated (rpmScriptState) */
};
const struct rpmconstant_s * rpmScriptStatectable = rpmScriptStatectbl;


/* From rpmtypes.h
 * prefix tag: RPMSCRIPT_
 * table: rpmScriptID */
#include "rpmtypes.h"
static const struct rpmconstant_s rpmScriptIDctbl[] = {
#ifdef _H_RPMTYPES_
	{ "RPMSCRIPT_PRETRANS", RPMSCRIPT_PRETRANS }, /*!< %pretrans scriptlet */
	{ "RPMSCRIPT_PREIN", RPMSCRIPT_PREIN }, /*!< %pre scriptlet */
	{ "RPMSCRIPT_POSTIN", RPMSCRIPT_POSTIN }, /*!< %post scriptlet */
	{ "RPMSCRIPT_PREUN", RPMSCRIPT_PREUN }, /*!< %preun scriptlet */
	{ "RPMSCRIPT_POSTUN", RPMSCRIPT_POSTUN }, /*!< %postun scriptlet */
	{ "RPMSCRIPT_POSTTRANS", RPMSCRIPT_POSTTRANS }, /*!< %posttrans scriptlet */
	{ "RPMSCRIPT_VERIFY", RPMSCRIPT_VERIFY }, /*!< %verify scriptlet */
	{ "RPMSCRIPT_PREP", RPMSCRIPT_PREP }, /*!< %prep build scriptlet */
	{ "RPMSCRIPT_BUILD", RPMSCRIPT_BUILD }, /*!< %build build scriptlet */
	{ "RPMSCRIPT_INSTALL", RPMSCRIPT_INSTALL }, /*!< %install build scriptlet */
	{ "RPMSCRIPT_CHECK", RPMSCRIPT_CHECK }, /*!< %check build scriptlet */
	{ "RPMSCRIPT_MAX", RPMSCRIPT_MAX }, 
#endif /* _H_RPMTYPES_ */
	{ NULL, 0 } /* NULL terminated (rpmScriptID) */
};
const struct rpmconstant_s * rpmScriptIDctable = rpmScriptIDctbl;


/* From iosm.h
 * prefix tag: FA_
 * table: rpmfileAction */
#include "iosm.h"
static const struct rpmconstant_s rpmfileActionctbl[] = {
#ifdef H_IOSM
	{ "FA_UNKNOWN", FA_UNKNOWN }, /*!< initial action for file ... */
	{ "FA_CREATE", FA_CREATE }, /*!< ... copy in from payload. */
	{ "FA_COPYIN", FA_COPYIN }, /*!< ... copy in from payload. */
	{ "FA_COPYOUT", FA_COPYOUT }, /*!< ... copy out to payload. */
	{ "FA_BACKUP", FA_BACKUP }, /*!< ... renamed with ".rpmorig" extension. */
	{ "FA_SAVE", FA_SAVE }, /*!< ... renamed with ".rpmsave" extension. */
	{ "FA_SKIP", FA_SKIP }, /*!< ... already replaced, don't remove. */
	{ "FA_ALTNAME", FA_ALTNAME }, /*!< ... create with ".rpmnew" extension. */
	{ "FA_ERASE", FA_ERASE }, /*!< ... to be removed. */
	{ "FA_SKIPNSTATE", FA_SKIPNSTATE }, /*!< ... untouched, state "not installed". */
	{ "FA_SKIPNETSHARED", FA_SKIPNETSHARED }, /*!< ... untouched, state "netshared". */
	{ "FA_SKIPCOLOR", FA_SKIPCOLOR }, /*!< ... untouched, state "wrong color". */
#endif /* H_IOSM */
	{ NULL, 0 } /* NULL terminated (rpmfileAction) */
};
const struct rpmconstant_s * rpmfileActionctable = rpmfileActionctbl;


/* From mire.h
 * prefix tag: RPMMIRE_
 * table: rpmMireMode */
#include "mire.h"
static const struct rpmconstant_s rpmMireModectbl[] = {
#ifdef H_MIRE
	{ "RPMMIRE_DEFAULT", RPMMIRE_DEFAULT }, /*!< posix regex with \., .* and ^...$ added */
	{ "RPMMIRE_STRCMP", RPMMIRE_STRCMP }, /*!< strings using strcmp(3) */
	{ "RPMMIRE_REGEX", RPMMIRE_REGEX }, /*!< posix regex(7) patterns using regcomp(3) */
	{ "RPMMIRE_GLOB", RPMMIRE_GLOB }, /*!< glob(7) patterns using fnmatch(3) */
	{ "RPMMIRE_PCRE", RPMMIRE_PCRE }, /*!< pcre patterns using pcre_compile2(3) */
#endif /* H_MIRE */
	{ NULL, 0 } /* NULL terminated (rpmMireMode) */
};
const struct rpmconstant_s * rpmMireModectable = rpmMireModectbl;


/* From rpmiotypes.h
 * prefix tag: RPMCALLBACK_
 * table: rpmCallbackType */
#include "rpmiotypes.h"
static const struct rpmconstant_s rpmCallbackTypectbl[] = {
#ifdef _H_RPMIOTYPES_
	{ "RPMCALLBACK_UNKNOWN", RPMCALLBACK_UNKNOWN }, 
	{ "RPMCALLBACK_INST_PROGRESS", RPMCALLBACK_INST_PROGRESS }, 
	{ "RPMCALLBACK_INST_START", RPMCALLBACK_INST_START }, 
	{ "RPMCALLBACK_INST_OPEN_FILE", RPMCALLBACK_INST_OPEN_FILE }, 
	{ "RPMCALLBACK_INST_CLOSE_FILE", RPMCALLBACK_INST_CLOSE_FILE }, 
	{ "RPMCALLBACK_TRANS_PROGRESS", RPMCALLBACK_TRANS_PROGRESS }, 
	{ "RPMCALLBACK_TRANS_START", RPMCALLBACK_TRANS_START }, 
	{ "RPMCALLBACK_TRANS_STOP", RPMCALLBACK_TRANS_STOP }, 
	{ "RPMCALLBACK_UNINST_PROGRESS", RPMCALLBACK_UNINST_PROGRESS }, 
	{ "RPMCALLBACK_UNINST_START", RPMCALLBACK_UNINST_START }, 
	{ "RPMCALLBACK_UNINST_STOP", RPMCALLBACK_UNINST_STOP }, 
	{ "RPMCALLBACK_REPACKAGE_PROGRESS", RPMCALLBACK_REPACKAGE_PROGRESS }, 
	{ "RPMCALLBACK_REPACKAGE_START", RPMCALLBACK_REPACKAGE_START }, 
	{ "RPMCALLBACK_REPACKAGE_STOP", RPMCALLBACK_REPACKAGE_STOP }, 
	{ "RPMCALLBACK_UNPACK_ERROR", RPMCALLBACK_UNPACK_ERROR }, 
	{ "RPMCALLBACK_CPIO_ERROR", RPMCALLBACK_CPIO_ERROR }, 
	{ "RPMCALLBACK_SCRIPT_ERROR", RPMCALLBACK_SCRIPT_ERROR }, 
#endif /* _H_RPMIOTYPES_ */
	{ NULL, 0 } /* NULL terminated (rpmCallbackType) */
};
const struct rpmconstant_s * rpmCallbackTypectable = rpmCallbackTypectbl;


/* From rpmiotypes.h
 * prefix tag: RPMVSF_
 * table: rpmvsflags */
#include "rpmiotypes.h"
static const struct rpmconstant_s rpmvsflagsctbl[] = {
#ifdef _H_RPMIOTYPES_
	{ "RPMVSF_DEFAULT", RPMVSF_DEFAULT }, 
	{ "RPMVSF_NOHDRCHK", RPMVSF_NOHDRCHK }, 
	{ "RPMVSF_NEEDPAYLOAD", RPMVSF_NEEDPAYLOAD }, 
	{ "RPMVSF_NOSHA1HEADER", RPMVSF_NOSHA1HEADER }, 
	{ "RPMVSF_NOMD5HEADER", RPMVSF_NOMD5HEADER }, 
	{ "RPMVSF_NODSAHEADER", RPMVSF_NODSAHEADER }, 
	{ "RPMVSF_NORSAHEADER", RPMVSF_NORSAHEADER }, 
	{ "RPMVSF_NOECDSAHEADER", RPMVSF_NOECDSAHEADER }, 
	{ "RPMVSF_NOSHA1", RPMVSF_NOSHA1 }, 
	{ "RPMVSF_NOMD5", RPMVSF_NOMD5 }, 
	{ "RPMVSF_NODSA", RPMVSF_NODSA }, 
	{ "RPMVSF_NORSA", RPMVSF_NORSA }, 
	{ "RPMVSF_NOECDSA", RPMVSF_NOECDSA }, 
#endif /* _H_RPMIOTYPES_ */
	{ NULL, 0 } /* NULL terminated (rpmvsflags) */
};
const struct rpmconstant_s * rpmvsflagsctable = rpmvsflagsctbl;


/* From rpmiotypes.h
 * prefix tag: RPMRC_
 * table: rpmRC */
#include "rpmiotypes.h"
static const struct rpmconstant_s rpmRCctbl[] = {
#ifdef _H_RPMIOTYPES_
	{ "RPMRC_OK", RPMRC_OK }, /*!< Generic success code */
	{ "RPMRC_NOTFOUND", RPMRC_NOTFOUND }, /*!< Generic not found code. */
	{ "RPMRC_FAIL", RPMRC_FAIL }, /*!< Generic failure code. */
	{ "RPMRC_NOTTRUSTED", RPMRC_NOTTRUSTED }, /*!< Signature is OK, but key is not trusted. */
	{ "RPMRC_NOKEY", RPMRC_NOKEY }, /*!< Public key is unavailable. */
	{ "RPMRC_NOSIG", RPMRC_NOSIG }, /*!< Signature is unavailable. */
#endif /* _H_RPMIOTYPES_ */
	{ NULL, 0 } /* NULL terminated (rpmRC) */
};
const struct rpmconstant_s * rpmRCctable = rpmRCctbl;


/* From rpmlog.h
 * prefix tag: RPMLOG_
 * table: rpmlog */
#include "rpmlog.h"
static const struct rpmconstant_s rpmlogctbl[] = {
#ifdef H_RPMLOG
	{ "RPMLOG_EMERG", RPMLOG_EMERG }, /*!< system is unusable */
	{ "RPMLOG_ALERT", RPMLOG_ALERT }, /*!< action must be taken immediately */
	{ "RPMLOG_CRIT", RPMLOG_CRIT }, /*!< critical conditions */
	{ "RPMLOG_ERR", RPMLOG_ERR }, /*!< error conditions */
	{ "RPMLOG_WARNING", RPMLOG_WARNING }, /*!< warning conditions */
	{ "RPMLOG_NOTICE", RPMLOG_NOTICE }, /*!< normal but significant condition */
	{ "RPMLOG_INFO", RPMLOG_INFO }, /*!< informational */
	{ "RPMLOG_DEBUG", RPMLOG_DEBUG }, /*!< debug-level messages */
	{ "RPMLOG_PRIMASK", RPMLOG_PRIMASK }, /* mask to extract priority part (internal) */
	{ "RPMLOG_KERN", RPMLOG_KERN }, 
	{ "RPMLOG_USER", RPMLOG_USER }, 
	{ "RPMLOG_MAIL", RPMLOG_MAIL }, 
	{ "RPMLOG_DAEMON", RPMLOG_DAEMON }, 
	{ "RPMLOG_AUTH", RPMLOG_AUTH }, 
	{ "RPMLOG_SYSLOG", RPMLOG_SYSLOG }, 
	{ "RPMLOG_LPR", RPMLOG_LPR }, 
	{ "RPMLOG_NEWS", RPMLOG_NEWS }, 
	{ "RPMLOG_UUCP", RPMLOG_UUCP }, 
	{ "RPMLOG_CRON", RPMLOG_CRON }, 
	{ "RPMLOG_AUTHPRIV", RPMLOG_AUTHPRIV }, 
	{ "RPMLOG_FTP", RPMLOG_FTP }, 
	{ "RPMLOG_LOCAL0", RPMLOG_LOCAL0 }, 
	{ "RPMLOG_LOCAL1", RPMLOG_LOCAL1 }, 
	{ "RPMLOG_LOCAL2", RPMLOG_LOCAL2 }, 
	{ "RPMLOG_LOCAL3", RPMLOG_LOCAL3 }, 
	{ "RPMLOG_LOCAL4", RPMLOG_LOCAL4 }, 
	{ "RPMLOG_LOCAL5", RPMLOG_LOCAL5 }, 
	{ "RPMLOG_LOCAL6", RPMLOG_LOCAL6 }, 
	{ "RPMLOG_LOCAL7", RPMLOG_LOCAL7 }, 
	{ "RPMLOG_NFACILITIES", RPMLOG_NFACILITIES }, /*!< current number of facilities */
	{ "RPMLOG_ERRMSG", RPMLOG_ERRMSG }, 
	{ "RPMLOG_FACMASK", RPMLOG_FACMASK }, /*!< mask to extract facility part */
	{ "RPMLOG_ODELAY", RPMLOG_ODELAY }, /*!< delay open until first syslog() (default) */
	{ "RPMLOG_NDELAY", RPMLOG_NDELAY }, /*!< don't delay open */
	{ "RPMLOG_PID", RPMLOG_PID }, /*!< log the pid with each message */
	{ "RPMLOG_CONS", RPMLOG_CONS }, /*!< log on the console if errors in sending */
	{ "RPMLOG_NOWAIT", RPMLOG_NOWAIT }, /*!< don't wait for console forks: DEPRECATED */
	{ "RPMLOG_PERROR", RPMLOG_PERROR }, /*!< log to stderr as well */
	{ "RPMLOG_DEFAULT", RPMLOG_DEFAULT }, /*!< perform default logging */
	{ "RPMLOG_EXIT", RPMLOG_EXIT }, /*!< exit after logging */
#endif /* H_RPMLOG */
	{ NULL, 0 } /* NULL terminated (rpmlog) */
};
const struct rpmconstant_s * rpmlogctable = rpmlogctbl;


/* From rpmmacro.h
 * prefix tag: RMIL_
 * table: rpmMacrosContext */
#include "rpmmacro.h"
static const struct rpmconstant_s rpmMacrosContextctbl[] = {
#ifdef _H_MACRO_
	{ "RMIL_DEFAULT", RMIL_DEFAULT }, 
	{ "RMIL_MACROFILES", RMIL_MACROFILES }, 
	{ "RMIL_RPMRC", RMIL_RPMRC }, 
	{ "RMIL_CMDLINE", RMIL_CMDLINE }, 
	{ "RMIL_TARBALL", RMIL_TARBALL }, 
	{ "RMIL_SPEC", RMIL_SPEC }, 
	{ "RMIL_OLDSPEC", RMIL_OLDSPEC }, 
	{ "RMIL_GLOBAL", RMIL_GLOBAL }, 
#endif /* _H_MACRO_ */
	{ NULL, 0 } /* NULL terminated (rpmMacrosContext) */
};
const struct rpmconstant_s * rpmMacrosContextctable = rpmMacrosContextctbl;

static const struct rpmconstantlist_s rpmconstanttp[] = {
	{ (void *)rpmBuildFlagsctbl, "rpmbuildflags", "RPMBUILD_" },
	{ (void *)rpmCallbackTypectbl, "rpmcallbacktype", "RPMCALLBACK_" },
	{ (void *)rpmElementTypectbl, "rpmelementtype", "TR_" },
	{ (void *)rpmInstallInterfaceFlagsctbl, "rpminstallinterfaceflags", "INSTALL_" },
	{ (void *)rpmMacrosContextctbl, "rpmmacroscontext", "RMIL_" },
	{ (void *)rpmMireModectbl, "rpmmiremode", "RPMMIRE_" },
	{ (void *)rpmParseStatectbl, "rpmparsestate", "PART_" },
	{ (void *)rpmProblemTypectbl, "rpmproblemtype", "RPMPROB_" },
	{ (void *)rpmQVSourcesctbl, "rpmqvsources", "RPMQV_" },
	{ (void *)rpmQueryFlagsctbl, "rpmqueryflags", "QUERY_" },
	{ (void *)rpmRCctbl, "rpmrc", "RPMRC_" },
	{ (void *)rpmScriptIDctbl, "rpmscriptid", "RPMSCRIPT_" },
	{ (void *)rpmScriptStatectbl, "rpmscriptstate", "RPMSCRIPT_STATE_" },
	{ (void *)rpmSignFlagsctbl, "rpmsignflags", "RPMSIGN_" },
	{ (void *)rpmTagctbl, "rpmtag", "RPMTAG_" },
	{ (void *)rpmTagTypectbl, "rpmtagtype", "RPM_" },
	{ (void *)rpmVerifyAttrsctbl, "rpmverifyattrs", "RPMVERIFY_" },
	{ (void *)rpmVerifyFlagsctbl, "rpmverifyflags", "VERIFY_" },
	{ (void *)rpmdbictbl, "rpmdbi", "RPMDBI_" },
	{ (void *)rpmdepFlagsctbl, "rpmdepflags", "RPMDEPS_" },
	{ (void *)rpmfileActionctbl, "rpmfileaction", "FA_" },
	{ (void *)rpmfileAttrsctbl, "rpmfileattrs", NULL },
	{ (void *)rpmfileStatectbl, "rpmfilestate", "RPMFILE_" },
	{ (void *)rpmfileTypesctbl, "rpmfiletypes", NULL },
	{ (void *)rpmlogctbl, "rpmlog", "RPMLOG_" },
	{ (void *)rpmnsTypectbl, "rpmnstype", "RPMNS_" },
	{ (void *)rpmprobFilterFlagsctbl, "rpmprobfilterflags", "RPMPROB_FILTER_" },
	{ (void *)rpmsenseflagsctbl, "rpmsenseflags", "RPMSENSE_" },
	{ (void *)rpmtagSignaturectbl, "rpmtagsignature", "RPMSIGTAG_" },
	{ (void *)rpmtransFlagsctbl, "rpmtransflags", "RPMTRANS_FLAG_" },
	{ (void *)rpmtsOpXctbl, "rpmtsopx", "RPMTS_OP_" },
	{ (void *)rpmtsTypectbl, "rpmtstype", "RPMTRANS_TYPE_" },
	{ (void *)rpmtsmStagectbl, "rpmtsmstage", "TSM_" },
	{ (void *)rpmvsflagsctbl, "rpmvsflags", "RPMVSF_" },
	{ (void *) NULL, NULL, NULL } /* NULL terminated */
};
const struct rpmconstantlist_s * rpmconstanttype = rpmconstanttp;
const int rpmconstanttypesize = sizeof(rpmconstanttp) / sizeof(rpmconstanttp[0]) -1;
