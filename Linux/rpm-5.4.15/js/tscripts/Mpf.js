if (loglvl) print("--> Mpf.js");

var rpmmpf = require('rpmmpf');

var mpf = new rpmmpf.Mpf();
ack("typeof mpf;", "object");
ack("mpf instanceof rpmmpf.Mpf;", true);
ack("mpf.debug = 1;", 1);
ack("mpf.debug = 0;", 0);

if (loglvl) print("<-- Mpf.js");
