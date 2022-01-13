if (loglvl) print("--> Txn.js");

var rpmtxn = require('rpmtxn');

var txn = new rpmtxn.Txn();
ack("typeof txn;", "object");
ack("txn instanceof rpmtxn.Txn;", true);
ack("txn.debug = 1;", 1);
ack("txn.debug = 0;", 0);

if (loglvl) print("<-- Txn.js");
