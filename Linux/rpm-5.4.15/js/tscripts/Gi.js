if (loglvl) print("--> Gi.js");

var GPSEE = require('rpmgi');

var gi = new GPSEE.Gi();
ack("typeof gi;", "object");
ack("gi instanceof GPSEE.Gi;", true);
ack("gi.debug = 1;", 1);
ack("gi.debug = 0;", 0);

if (loglvl) print("<-- Gi.js");
