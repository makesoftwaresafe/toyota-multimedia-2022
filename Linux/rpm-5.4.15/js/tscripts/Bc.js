if (loglvl) print("--> Bc.js");

var GPSEE = require('rpmbc');

var bc = new GPSEE.Bc();
ack("typeof bc;", "object");
ack("bc instanceof GPSEE.Bc;", true);
ack("bc.debug = 1;", 1);
ack("bc.debug = 0;", 0);

if (loglvl) print("<-- Bc.js");
