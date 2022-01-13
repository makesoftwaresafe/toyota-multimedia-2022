if (loglvl) print("--> Sw.js");

var GPSEE = require('rpmsw');

var sw = new GPSEE.Sw();
ack("typeof sw;", "object");
ack("sw instanceof GPSEE.Sw;", true);
ack("sw.debug = 1;", 1);
ack("sw.debug = 0;", 0);

if (loglvl) print("<-- Sw.js");
