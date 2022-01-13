if (loglvl) print("--> Uuid.js");

var ns = "ns:URL";
var abc = "http://rpm5.org/abc";

var GPSEE = require('uuid');
var uuid = new GPSEE.Uuid(ns);
ack("typeof uuid;", "object");
ack("uuid instanceof GPSEE.Uuid;", true);
ack("uuid.debug = 1;", 1);
ack("uuid.debug = 0;", 0);

ack("uuid.describe(uuid.generate(1))",		undefined);
ack("uuid.describe(uuid.generate(3, ns, abc));",undefined);
ack("uuid.describe(uuid.generate(4))",		undefined);
ack("uuid.describe(uuid.generate(5, ns, abc));",undefined);

delete uuid;

if (loglvl) print("<-- Uuid.js");
