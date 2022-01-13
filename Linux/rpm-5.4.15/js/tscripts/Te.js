if (loglvl) print("--> Te.js");

var RPMTAG_PROVIDENAME	= 1047;

var RPMNS_TYPE_UNKNOWN  =  0;
var RPMNS_TYPE_STRING   =  (1 <<  0);   /*!< unclassified string */
var RPMNS_TYPE_PATH     =  (1 <<  1);   /*!< /bin */
var RPMNS_TYPE_DSO      =  (1 <<  2);   /*!< libc.so.6 */
var RPMNS_TYPE_FUNCTION =  (1 <<  3);   /*!< %{foo} */
var RPMNS_TYPE_ARCH     =  (1 <<  4);   /*!< foo.arch */
var RPMNS_TYPE_VERSION  =  (1 <<  5);   /*!< foo-1.2.3-bar */
var RPMNS_TYPE_COMPOUND =  (1 <<  6);   /*!< foo.bar */
        /* 7 unused */
var RPMNS_TYPE_NAMESPACE=  (1 <<  8);   /*!< foo(bar) */
var RPMNS_TYPE_RPMLIB   =  (1 <<  9);   /*!< rpmlib(bar) */
var RPMNS_TYPE_CPUINFO  =  (1 << 10);   /*!< cpuinfo(bar) */
var RPMNS_TYPE_GETCONF  =  (1 << 11);   /*!< getconf(bar) */
var RPMNS_TYPE_UNAME    =  (1 << 12);   /*!< uname(bar) */
var RPMNS_TYPE_SONAME   =  (1 << 13);   /*!< soname(bar) */
var RPMNS_TYPE_ACCESS   =  (1 << 14);   /*!< exists(bar) */
var RPMNS_TYPE_TAG      =  (1 << 15);   /*!< Tag(bar) */
var RPMNS_TYPE_USER     =  (1 << 16);   /*!< user(bar) */
var RPMNS_TYPE_GROUP    =  (1 << 17);   /*!< group(bar) */
var RPMNS_TYPE_MOUNTED  =  (1 << 18);   /*!< mounted(/path) */
var RPMNS_TYPE_DISKSPACE=  (1 << 19);   /*!< diskspace(/path) */
var RPMNS_TYPE_DIGEST   =  (1 << 20);   /*!< digest(/path) = hex */
var RPMNS_TYPE_GNUPG    =  (1 << 21);   /*!< gnupg(/path/file.asc) */
var RPMNS_TYPE_MACRO    =  (1 << 22);   /*!< macro(foo) */
var RPMNS_TYPE_ENVVAR   =  (1 << 23);   /*!< envvar(foo) */
var RPMNS_TYPE_RUNNING  =  (1 << 24);   /*!< running(foo) */
var RPMNS_TYPE_SANITY   =  (1 << 25);   /*!< sanitycheck(foo) */
var RPMNS_TYPE_VCHECK   =  (1 << 26);   /*!< vcheck(foo) */
var RPMNS_TYPE_SIGNATURE=  (1 << 27);   /*!< signature(/text:/sig) = /pub:id */
var RPMNS_TYPE_VERIFY   =  (1 << 28);   /*!< verify(N) = E:V-R */
var RPMNS_TYPE_CONFIG   =  (1 << 29);   /*!< config(N) = E:V-R */

var RPMSENSE_ANY        = 0;
var RPMSENSE_SERIAL     = (1 << 0);     /*!< (obsolete). */
var RPMSENSE_LESS       = (1 << 1);
var RPMSENSE_GREATER    = (1 << 2);
var RPMSENSE_EQUAL      = (1 << 3);
var RPMSENSE_PROVIDES   = (1 << 4); /* only used internally by builds */
var RPMSENSE_CONFLICTS  = (1 << 5); /* only used internally by builds */
var RPMSENSE_PREREQ     = (1 << 6);     /*!< (obsolete). */
var RPMSENSE_OBSOLETES  = (1 << 7); /* only used internally by builds */
var RPMSENSE_INTERP     = (1 << 8);     /*!< Interpreter used by scriptlet. */
var RPMSENSE_SCRIPT_PRE = (1 << 9);     /*!< %pre dependency. */
var RPMSENSE_SCRIPT_POST = (1 << 10);   /*!< %post dependency. */
var RPMSENSE_SCRIPT_PREUN = (1 << 11);  /*!< %preun dependency. */
var RPMSENSE_SCRIPT_POSTUN = (1 << 12); /*!< %postun dependency. */
var RPMSENSE_SCRIPT_VERIFY = (1 << 13); /*!< %verify dependency. */
var RPMSENSE_FIND_REQUIRES = (1 << 14); /*!< find-requires dependency. */
var RPMSENSE_FIND_PROVIDES = (1 << 15); /*!< find-provides dependency. */

var RPMSENSE_TRIGGERIN  = (1 << 16);    /*!< %triggerin dependency. */
var RPMSENSE_TRIGGERUN  = (1 << 17);    /*!< %triggerun dependency. */
var RPMSENSE_TRIGGERPOSTUN = (1 << 18); /*!< %triggerpostun dependency. */
var RPMSENSE_MISSINGOK  = (1 << 19);    /*!< suggests/enhances hint. */
var RPMSENSE_SCRIPT_PREP = (1 << 20);   /*!< %prep build dependency. */
var RPMSENSE_SCRIPT_BUILD = (1 << 21);  /*!< %build build dependency. */
var RPMSENSE_SCRIPT_INSTALL = (1 << 22);/*!< %install build dependency. */
var RPMSENSE_SCRIPT_CLEAN = (1 << 23);  /*!< %clean build dependency. */
var RPMSENSE_RPMLIB = (1 << 24);        /*!< rpmlib(feature) dependency. */
var RPMSENSE_TRIGGERPREIN = (1 << 25);  /*!< %triggerprein dependency. */
var RPMSENSE_KEYRING    = (1 << 26);
var RPMSENSE_STRONG     = (1 << 27);    /*!< placeholder SuSE */
var RPMSENSE_CONFIG     = (1 << 28);
var RPMSENSE_PROBE      = (1 << 29);
var RPMSENSE_PACKAGE    = (1 << 30);
var RPMSENSE_SCRIPT_SANITYCHECK = (1 << 31); /*!< %sanitycheck dependency. */

var rpmts = require('rpmts');
var rpmte = require('rpmte');
var rpmds = require('rpmds');
var rpmfi = require('rpmfi');
var rpmbf = require('rpmbf');

var ts = new rpmts.Ts();

var te = new rpmte.Te(ts)
ack("typeof te;", "object");
ack("te instanceof rpmte.Te;", true);
ack("te.debug = 1;", 1);
ack("te.debug = 0;", 0);
delete te;

ack("ts.add('+time')", true);
ack("ts.add('+popt')", true);
ack("ts.check()", true);
ack("ts.order()", true);

te = ts[0];
ack("typeof te;", "object");
ack("te instanceof rpmte.Te;", true);

ack("te.N", "popt");
ack("te.E", 0);
ack("te.V", "1.13");
ack("te.R", "5.fc11");
ack("te.A", "i586");
ack("te.O", "linux");
ack("te.NEVR", "popt-1.13-5.fc11.i586");
ack("te.NEVRA", "popt-1.13-5.fc11.i586");
ack("te.pkgid", "239098a0368d5589e9dd432944d5eb06");
ack("te.hdrid", "ca6058bf3cbde1ab84dfdb901a1b32b3dc152f8e");
ack("te.color", 1);
ack("te.fsize", 0);
ack("te.breadth", 0);
ack("te.depth", 0);
ack("te.npreds", 0);
ack("te.degree", 0);
ack("te.parent", 0);
ack("te.tree", 2);
ack("te.addedkey", 1);
ack("te.dboffset", 0);
ack("te.key", 0);
ack("te.sourcerpm", "popt-1.13-5.fc11.src.rpm");

    var ds = te.ds(RPMTAG_PROVIDENAME);
ack("typeof ds;", "object");
ack("ds instanceof rpmds.Ds;", true);

    ack("ds.ix", 0);
    ack("ds.N", "libpopt.so.0");
    ack("ds.EVR", '');
    ack("ds.F", RPMSENSE_FIND_PROVIDES);
    ack("ds.DNEVR", "P libpopt.so.0");
    ack("ds.NS", RPMNS_TYPE_DSO);
    ack("ds.refs += 1;", 1);
    ack("ds.result = 1;", 1);

    ack("ds.ix += 1", 1);
    ack("ds.N", "libpopt.so.0(LIBPOPT_0)");
    ack("ds.EVR", '');
    ack("ds.F", RPMSENSE_FIND_PROVIDES);
    ack("ds.DNEVR", "P libpopt.so.0(LIBPOPT_0)");
    ack("ds.NS", RPMNS_TYPE_NAMESPACE);
    ack("ds.refs += 1;", 1);
    ack("ds.result = 1;", 1);

    ack("ds.ix += 1", 2);
    ack("ds.N", "popt");
    ack("ds.EVR", '1.13-5.fc11');
    ack("ds.F", RPMSENSE_EQUAL);
    ack("ds.DNEVR", "P popt = 1.13-5.fc11");
    ack("ds.NS", RPMNS_TYPE_STRING);
    ack("ds.refs += 1;", 1);
    ack("ds.result = 1;", 1);

    delete ds;

    var fi = te.fi();
ack("typeof fi;", "object");
ack("fi instanceof rpmfi.Fi;", true);

    var files = new Array();
    var ix = 0;
    for (let [fikey,fival] in Iterator(fi)) {
        files[ix++] = fival;
    }
    ack("ix == fi.length", true);

    ix = 0
    for (var [key,val] in Iterator(fi)) {
        ack("key == ix", true);
        ack("files[key] == val", true);
        ack("fi[key] == val", true);
        ack("fi[key] == files[key]", true);
        ix++;
    }
    ack("ix == fi.length", true);

    ack("fi.fc", 32);
    ack("fi.fx", -1);
    ack("fi.dc", 30);
    ack("fi.dx", 29);

    ack("fi.dx = -1;", -1);
    ack("++fi.dx;", 0);
    ack("++fi.dx;", 1);

    ack("fi.fx = -1;", -1);
    ack("fi.fx += 1;", 0);
    ack("fi.fx += 1;", 1);
    ack("fi.fx += 1;", 2);

var bn = "libpopt.so.0.0.0";
var dn = "/lib/";
var fn = dn + bn;
    ack("fi.bn", bn);
    ack("fi.dn", dn);
    ack("fi.fn", fn);

var bf = fi.fnbf;
ack("typeof bf;", "object");
ack("bf instanceof rpmbf.Bf;", true);
ack('bf.chk(bn)', false);
ack('bf.chk(dn)', false);
ack('bf.chk(fn)', true);

    ack("fi.vflags", -1);
    ack("fi.fflags", 0);
    ack("fi.fmode", 33261);
    ack("fi.fstate", 0);
    ack("fi.fdigest", "27cc0f6d99f0fb391ddcbaafb7a020da2a265aa687e0b3c317aae80b0d728777");
    ack("fi.flink", '');
    ack("fi.fsize", 32832);
    ack("fi.frdev", 0);
    ack("fi.fmtime", 0x0);	// needs full uint32_t return
    ack("fi.fuser", 'root');
    ack("fi.fgroup", 'root');
    ack("fi.fcolor", 1);
    ack("fi.fclass", "ELF 32-bit LSB shared object, Intel 80386, version 1 (SYSV), dynamically linked, stripped");

    delete fi;

delete te;

delete ts;

if (loglvl) print("<-- Te.js");
