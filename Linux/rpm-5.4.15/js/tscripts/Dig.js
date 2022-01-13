if (loglvl) print("--> Dig.js");

var GPSEE = require('rpmdig');

var dig = new GPSEE.Dig();
ack("typeof dig;", "object");
ack("dig instanceof GPSEE.Dig;", true);
ack("dig.debug = 1;", 1);
ack("dig.debug = 0;", 0);

if (loglvl) print("<-- Dig.js");
