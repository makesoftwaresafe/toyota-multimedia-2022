if (loglvl) print("--> Ds.js");

var RPMNS_TYPE_RPMLIB = (1 << 9);

var GPSEE = require('rpmds');

var ds = new GPSEE.Ds('rpmlib');
ack("typeof ds;", "object");
ack("ds instanceof GPSEE.Ds;", true);
ack("ds.debug = 1;", 1);
ack("ds.debug = 0;", 0);

ack("ds.length", 25);
ack("ds.type", "Provides");
ack("ds.A", undefined);
ack("ds.buildtime = 1234;", 1234);
ack("ds.color = 4;", 4);
ack("ds.nopromote = 1;", 1);
ack("ds.nopromote = 0;", 0);

var deps = new Array();
var ix = 0;
for (let [key,val] in Iterator(ds)) {
  deps[ix++] = val;
}
ack("ix == ds.length", true);

ix = 0
for (var [key,val] in Iterator(ds)) {
    ack("key == ix", true);
    ack("deps[ix] == val", true);
    ack("ds[ix] == val", true);
    ack("ds[ix] == deps[ix]", true);
    ix++;
}
ack("ix == ds.length", true);

ack("ds.ix = -1;", -1);
ack("ds[-1] == undefined", true);
ack("ds.N == undefined", true);
ack("ds.EVR == undefined", true);
ack("ds.F == undefined", true);
ack("ds.DNEVR == undefined", true);
ack("ds.NS == undefined", true);
ack("ds.refs == undefined", true);
ack("ds.result == undefined", true);

ack("ds.ix += 1;", 0);
ack("ds[0] != undefined", true);
ack("ds.ix", 0);
ack("ds.N == ds[0][0]", true);
ack("ds.N", "BuiltinAugeasScripts");
ack("ds.N == ds[0][0]", true);
ack("ds.EVR", "5.3-1");
ack("ds.EVR == ds[0][1]", true);
ack("ds.F", 16777224);
ack("ds.F == ds[0][2]", true);
ack("ds.DNEVR", "P rpmlib(BuiltinAugeasScripts) = 5.3-1");
ack("ds.NS == RPMNS_TYPE_RPMLIB", true);
ack("ds.refs += 1;", 1);
ack("ds.result = 1;", 1);

ack("ds.ix += 1;", 1);
ack("ds[1] != undefined", true);
ack("ds.ix", 1);
ack("ds.N", "BuiltinFiclScripts");
ack("ds.N == ds[1][0]", true);
ack("ds.EVR", "5.2-1");
ack("ds.EVR == ds[1][1]", true);
ack("ds.F", 16777224);
ack("ds.F == ds[1][2]", true);
ack("ds.DNEVR", "P rpmlib(BuiltinFiclScripts) = 5.2-1");
ack("ds.NS == RPMNS_TYPE_RPMLIB", true);
ack("ds.refs += 1;", 1);
ack("ds.result = 1;", 1);

// for (var [key,val] in Iterator(ds))
//     print("\t"+key+": "+val);

// print(JSON.stringify(ds));

delete ds;

if (loglvl) print("<-- Ds.js");
