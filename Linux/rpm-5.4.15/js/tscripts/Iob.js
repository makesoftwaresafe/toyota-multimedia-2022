if (loglvl) print("--> Iob.js");

var GPSEE = require('rpmiob');

var iob = new GPSEE.Iob();
ack("typeof iob;", "object");
ack("iob instanceof GPSEE.Iob;", true);
ack("iob.debug = 1;", 1);
ack("iob.debug = 0;", 0);

if (loglvl) print("<-- Iob.js");
