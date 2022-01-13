if (loglvl) print("--> Sp.js");

const SYM_COMMONS = 0;
const SYM_CLASSES = 1;
const SYM_ROLES   = 2;
const SYM_TYPES   = 3;
const SYM_USERS   = 4;
const SYM_BOOLS   = 5;
const SYM_LEVELS  = 6;
const SYM_CATS    = 7;
const SYM_NUM     = 8;

var dn = "targeted";

var rpmsp = require('rpmsp');

var sp = new rpmsp.Sp(dn);
ack("typeof sp;", "object");
ack("sp instanceof rpmsp.Sp;", true);
ack("sp.debug = 1;", 1);
ack("sp.debug = 0;", 0);

delete sp;

if (loglvl) print("<-- Sp.js");
