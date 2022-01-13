if (loglvl) print("--> Ts.js");

var RPMTAG_NAME		= 1000;
var RPMTAG_PROVIDENAME	= 1047;
var RPMTAG_BASENAMES	= 1117;

var RPMMIRE_STRCMP	= 1;

var rpmts = require('rpmts');
var rpmte = require('rpmte');

var ts = new rpmts.Ts();
ack("typeof ts;", "object");
ack("ts instanceof rpmts.Ts;", true);
ack("ts.debug = 1;", 1);
ack("ts.debug = 0;", 0);

var rootdir = "/path/to/rootdir";
ack("ts.rootdir = rootdir", rootdir);
ack("ts.rootdir = \"/\"", "/");

var vsflags = 0x1234;
ack("ts.vsflags = vsflags", vsflags);
ack("ts.vsflags = 0", 0);

ack("ts.dbrebuild()", true);

var pat = '^a.*$';
var a = ts.dbkeys(pat);
var b = ts.dbkeys(pat, RPMTAG_NAME);
var c = ts.dbkeys(pat, RPMTAG_NAME, RPMMIRE_STRCMP);
ack('a.length == b.length', true);
ack('b.length == c.length', true);

ack("ts.add('-popt')", true);
ack("ts[ts.length-1].N", "popt");
ack("ts.add('+bash')", true);
ack("ts[ts.length-1].N", "bash");
ack("ts.add('glibc')", true);
ack("ts[ts.length-1].N", "glibc");

var ix = 0;
for (var [key,te] in Iterator(ts)) {
    ack("typeof te;", "object");
    ack("te instanceof rpmte.Te;", true);
    ack("key == ix", true);
//  print(JSON.stringify(te.ds(RPMTAG_NAME)));
//  print(JSON.stringify(te.ds(RPMTAG_PROVIDENAME)));
//  print(JSON.stringify(te.fi(RPMTAG_BASENAMES)));
    ix++;
}
ack("ts.length == ix", true);

// ack("ts.check()", true);
// ack("ts.order()", true);
// ack("ts.run()", true);

delete ts;	// GCZeal?

if (loglvl) print("<-- Ts.js");
