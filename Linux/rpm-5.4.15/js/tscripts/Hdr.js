if (loglvl) print("--> Hdr.js");

var RPMTAG_NAME = 1000;
var RPMTAG_REQUIRENAME = 1049;
var RPMTAG_BASENAMES = 1117;
var RPMTAG_NVRA = 1196;

var N = "popt";

var NVRA = N;
var bingo = 0;

var rpmhdr = require('rpmhdr');
var rpmts = require('rpmts');
var rpmmi = require('rpmmi');
var rpmds = require('rpmds');
var rpmfi = require('rpmfi');

var h = new rpmhdr.Hdr();
ack("typeof h;", "object");
ack("h instanceof rpmhdr.Hdr;", true);
ack("h.debug = 1;", 1);
ack("h.debug = 0;", 0);
delete h;

var ts = new rpmts.Ts();

var mi = new rpmmi.Mi(ts, RPMTAG_NVRA, N);
ack("mi.length", 1);
ack("mi.count", 1);
ack("mi.instance", 0);  // zero before iterating

bingo = 0;
for (let [dbkey,h] in Iterator(mi)) {
    nack("mi.instance", 0); // non-zero when iterating

    ack("typeof h;", "object");
    ack("h instanceof rpmhdr.Hdr;", true);

// for (var [tagno,tagval] in Iterator(h,true))
//     print(h.tag(tagno),JSON.stringify(tagval));

    var ds = h.ds();
    ack("typeof ds;", "object");
    ack("ds instanceof rpmds.Ds;", true);
    var ds = h.ds(RPMTAG_NAME);
    ack("typeof ds;", "object");
    ack("ds instanceof rpmds.Ds;", true);
    var ds = h.ds(RPMTAG_REQUIRENAME);
    ack("typeof ds;", "object");
    ack("ds instanceof rpmds.Ds;", true);

    var fi = h.fi();
    ack("typeof fi;", "object");
    ack("fi instanceof rpmfi.Fi;", true);
    var fi = h.fi(RPMTAG_BASENAMES);
    ack("typeof fi;", "object");
    ack("fi instanceof rpmfi.Fi;", true);

    ack("h.name", N);
    ack("h[RPMTAG_NAME]", N);
//  ack("h['name']", N);	// not yet ...
    ack("h.epoch", undefined);
    ack("h.version", undefined);
    ack("h.release", undefined);

    ack("h.summary", undefined);
    ack("h.description", undefined);
    ack("h.buildtime", undefined);
    ack("h.buildhost", undefined);
    ack("h.installtime", undefined);
    ack("h.size", undefined);
    ack("h.distribution", undefined);
    ack("h.vendor", undefined);
    ack("h.license", undefined);
    ack("h.packager", undefined);
    ack("h.group", undefined);

    ack("h.changelogtime", undefined);
    ack("h.changelogname", undefined);

    ack("h.url", undefined);
    ack("h.os", undefined);
    ack("h.arch", undefined);

    ack("h.prein", undefined);
    ack("h.preinprog", undefined);
    ack("h.postin", undefined);
    ack("h.postinprog", undefined);
    ack("h.preun", undefined);
    ack("h.preunprog", undefined);
    ack("h.postun", undefined);
    ack("h.postunprog", undefined);

    ack("h.filenames", undefined);
    ack("h.dirnames", undefined);
    ack("h.basenames", undefined);

    ack("h.filesizes", undefined);
    ack("h.filestates", undefined);
    ack("h.filemodes", undefined);
    ack("h.filerdevs", undefined);

    ack("h.sourcerpm", undefined);
    ack("h.archivesize", undefined);

    ack("h.providename", undefined);
    ack("h.requirename", undefined);
    ack("h.conflictname", undefined);
    ack("h.obsoletesname", undefined);

//  ack("h.keys()", undefined);
//  var legacyHeader = true;
//  ack("h.unload(legacyHeader)", undefined);
    var origin = "http://rpm5.org/files/popt/popt-1.14-1.i386.rpm";
    ack("h.setorigin(origin)", origin);
    ack("h.getorigin()", origin);
    var qfmt = "%{buildtime:date}";
    ack("h.sprintf(qfmt)", undefined);
    bingo = 1;
}
ack('bingo', 1);

delete mi;	// GCZeal?
delete ts;	// GCZeal?

if (loglvl) print("<-- Hdr.js");
