if (loglvl) print("--> Fc.js");

var GPSEE = require('rpmfc');

var fc = new GPSEE.Fc();
ack("typeof fc;", "object");
ack("fc instanceof GPSEE.Fc;", true);
ack("fc.debug = 1;", 1);
ack("fc.debug = 0;", 0);

if (loglvl) print("<-- Fc.js");
