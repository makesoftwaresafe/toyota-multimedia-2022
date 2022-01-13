if (loglvl) print("--> Mi.js");

var RPMDBI_PACKAGES = 0;
var RPMTAG_NAME = 1000;
var RPMTAG_VERSION = 1001;
var RPMTAG_RELEASE = 1002;
var RPMTAG_GROUP = 1016;
var RPMTAG_OS = 1021;
var RPMTAG_ARCH = 1022;
var RPMTAG_INSTALLTID = 1128;
var RPMTAG_REMOVETID = 1129;
var RPMTAG_PACKAGECOLOR = 1184;
var RPMTAG_NVRA = 1196;

var RPMMIRE_DEFAULT     = 0;    /*!< posix regex with \., .* and ^...$ added */
var RPMMIRE_STRCMP      = 1;    /*!< strings using strcmp(3) */
var RPMMIRE_REGEX       = 2;    /*!< posix regex(7) patterns using regcomp(3) */
var RPMMIRE_GLOB        = 3;    /*!< glob(7) patterns using fnmatch(3) */
var RPMMIRE_PCRE        = 4;    /*!< pcre patterns using pcre_compile2(3) */

var N = "popt";
var V = "";
var R = "";
var A = "";
var O = "";
var G = "";
var IID = 0;
var color = -1;

var NVRA = N;
var hdrNum = 0;
var bingo = 0;
var npkgs = 0;
var IIDcounts = [];
var IIDcount = 0;

var rpmts = require('rpmts');
var rpmmi = require('rpmmi');
var rpmhdr = require('rpmhdr');

var ts = new rpmts.Ts();

var mi = new rpmmi.Mi(ts);
ack("typeof mi;", "object");
ack("mi instanceof rpmmi.Mi;", true);
ack("mi.debug = 1;", 1);
ack("mi.debug = 0;", 0);
delete mi;

// --- Iterate over packages, counting, grab hdrNum and NVRA on the fly.
var mi = new rpmmi.Mi(ts);
bingo = 0;
for (var [dbkey,h] in Iterator(mi)) {
//    ack("mi.instance != 0", true);
//    ack("mi.instance < 0x0000ffff", true);
//    ack("mi.instance == h.dbinstance", true);
    ++npkgs;
    if (IIDcounts[h.installtid[0].toString(16)] > 0)
	++IIDcounts[h.installtid[0].toString(16)];
    else
	IIDcounts[h.installtid[0].toString(16)] = 1;
    if (h.name == N) {
	hdrNum = mi.instance;
	V = h.version;
	R = h.release;
	O = h.os;
	A = h.arch;
	G = h.group;
	IID = h.installtid[0];
	color = h.packagecolor;
	NVRA = h.nvra;
    }
    delete h;
    bingo = 1;
}
ack("bingo", 1);
delete mi;

function doITER(ts, tag, key) {
    this.mi = new rpmmi.Mi(ts, tag, key);
    this.bingo = 0;
    for ([this.dbkey,this.h] in Iterator(this.mi)) {
	ack("this.h.name", N);
	ack("this.h.nvra", NVRA);
	ack("this.mi.instance == hdrNum", true);
	ack("this.mi.instance == this.h.dbinstance", true);
	delete this.h;
	this.bingo = 1;
    }
    ack("this.bingo", 1);
    delete this.mi;
    return true;
}

// --- Retrieve by package primary key
doITER(ts, RPMDBI_PACKAGES, hdrNum);

// --- Retrieve by N and NVRA string key(s).
doITER(ts, RPMTAG_NAME, N);
doITER(ts, RPMTAG_NVRA, NVRA);

// --- Retrieve by N, N-V, N-V-R, and N-V-R.A strings.
doITER(ts, RPMTAG_NVRA, N);
doITER(ts, RPMTAG_NVRA, N+"-"+V);
doITER(ts, RPMTAG_NVRA, N+"-"+V+"-"+R);
doITER(ts, RPMTAG_NVRA, N+"-"+V+"-"+R+"."+A);

// --- Retrieve by N pattern.
doITER(ts, RPMTAG_NVRA, "\^"+N+"-[0-9].*$");
doITER(ts, RPMTAG_NVRA, "\^"+N+"-[0-9].*"+"\."+A+"$");

// --- Retrieve by N with various mire pattern selectors.
var mi = new rpmmi.Mi(ts, RPMTAG_NAME, N);
ack("mi.pattern(RPMTAG_VERSION, V)", true);
ack("mi.pattern('release', R, RPMMIRE_STRCMP)", true);
ack("mi.pattern(RPMTAG_OS, O, RPMMIRE_REGEX)", true);
ack("mi.pattern('arch', A, RPMMIRE_GLOB)", true);
ack("mi.pattern(RPMTAG_GROUP, G, RPMMIRE_PCRE)", true);
bingo = 0;
for (var [dbkey,h] in Iterator(mi)) {
    ack("h.name", N);
    ack("h.nvra", NVRA);
    ack("mi.instance == hdrNum", true);
    ack("mi.instance == h.dbinstance", true);
    delete h;
    bingo = 1;
}
ack("bingo", 1);
delete mi;

// --- Retrieve by N, filtering the instance (i.e. nothing found).
var mi = new rpmmi.Mi(ts, RPMTAG_NAME, N);
ack("mi.prune(hdrNum)", true);
bingo = 0;
for (var [dbkey,h] in Iterator(mi)) {
    nack("h.name", N);
    nack("h.nvra", NVRA);
    nack("mi.instance == hdrNum", true);
    nack("mi.instance == h.dbinstance", true);
    delete h;
    bingo = 1;
}
ack("bingo", 0);
delete mi;

// --- Retrieve by adding the instance.
var mi = new rpmmi.Mi(ts);
ack("mi.grow(hdrNum)", true);
bingo = 0;
for (var [dbkey,h] in Iterator(mi)) {
    ack("h.name", N);
    ack("h.nvra", NVRA);
    ack("mi.instance == hdrNum", true);
    ack("mi.instance == h.dbinstance", true);
    delete h;
    bingo = 1;
}
ack("bingo", 1);
delete mi;

// --- Count the install transaction set members.
var mi = new rpmmi.Mi(ts, RPMTAG_INSTALLTID, IID);
bingo = 0;
IIDcount = 0;
for (var [dbkey,h] in Iterator(mi)) {
    ++IIDcount;
    delete h;
    bingo = 1;
}
ack("bingo", 1);
ack("IIDcount", IIDcounts[IID.toString(16)]);
delete mi;

// --- Print packages that contain "README".
var bn = "README";
var mi = new rpmmi.Mi(ts, RPMTAG_BASENAMES, null);
ack("mi.growbn(bn)", true);
bingo = 0;
for (var [dbkey,h] in Iterator(mi)) {
//    print(h.nvra);
    delete h;
    bingo = 1;
}
ack("bingo", 1);
delete mi;

delete ts;

if (loglvl) print("<-- Mi.js");
