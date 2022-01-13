if (loglvl) print("--> Syck.js");

var GPSEE = require('syck');

var syck = new GPSEE.Syck()
ack("typeof syck;", "object");
ack("syck instanceof Syck;", true);
ack("syck.debug = 1;", 1);
ack("syck.debug = 0;", 0);

if (loglvl) print("<-- Syck.js");
