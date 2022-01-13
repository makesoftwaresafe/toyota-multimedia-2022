if (loglvl) print("--> Ps.js");

var RPMPROB_BADARCH	= 0;    /*!< (unused) package ... is for a different architecture */
var RPMPROB_BADOS	= 1;      /*!< (unused) package ... is for a different operating system */
var RPMPROB_PKG_INSTALLED = 2; /*!< package ... is already installed */
var RPMPROB_BADRELOCATE	= 3; /*!<path ... is not relocatable for package ... */
var RPMPROB_REQUIRES	= 4;   /*!< package ... has unsatisfied Requires: ... */
var RPMPROB_CONFLICT	= 5;   /*!< package ... has unsatisfied Conflicts: ... */
var RPMPROB_NEW_FILE_CONFLICT = 6; /*!< file ... conflicts between attempted installs of ... */
var RPMPROB_FILE_CONFLICT = 7; /*!<file ... from install of ... conflicts with file from package ... */
var RPMPROB_OLDPACKAGE	= 8; /*!< package ... (which is newer than ...) is already installed */
var RPMPROB_DISKSPACE	= 9;  /*!< installing package ... needs ... on the ... filesystem */
var RPMPROB_DISKNODES	= 10;  /*!< installing package ... needs ... on the ... filesystem */
var RPMPROB_RDONLY	= 11;     /*!< installing package ... on ... rdonly filesystem */
var RPMPROB_BADPRETRANS	= 12; /*!< (unimplemented) */
var RPMPROB_BADPLATFORM	= 13; /*!< package ... is for a different platform */
var RPMPROB_NOREPACKAGE	= 14; /*!< re-packaged package ... is missing */

var rpmps = require('rpmps');

var ps = new rpmps.Ps();
ack("typeof ps;", "object");
ack("ps instanceof rpmps.Ps;", true);
ack("ps.debug = 1;", 1);
ack("ps.debug = 0;", 0);

var PKG = "PKG";
var ALT = "ALT";
var KEY = "/path/pkg";
var dn = "/root/";
var bn = "file";
var ui = 1234;
var R = "R ALT";
var C = "C ALT";
var A = "ARCH/";
var O = "OS";

ack("ps.length", 0);
// ack("ps.push(PKG, ALT, KEY, RPMPROB_BADARCH, dn, bn, ui)", undefined);
// ack("ps.push(PKG, ALT, KEY, RPMPROB_BADOS, dn, bn, ui)", undefined);
// ack("ps.push(PKG, ALT, KEY, RPMPROB_PKG_INSTALLED, dn, bn, ui)", undefined);
// ack("ps.push(PKG, ALT, KEY, RPMPROB_BADRELOCATE, dn, bn, ui)", undefined);
// ack("ps.push(PKG,   R, KEY, RPMPROB_REQUIRES, dn, bn, ui)", undefined);
// ack("ps.push(PKG,   C, KEY, RPMPROB_CONFLICT, dn, bn, ui)", undefined);
// ack("ps.push(PKG, ALT, KEY, RPMPROB_NEW_FILE_CONFLICT, dn, bn, ui)", undefined);
// ack("ps.push(PKG, ALT, KEY, RPMPROB_FILE_CONFLICT, dn, bn, ui)", undefined);
// ack("ps.push(PKG, ALT, KEY, RPMPROB_OLDPACKAGE, dn, bn, ui)", undefined);
// ack("ps.push(PKG, ALT, KEY, RPMPROB_DISKSPACE, dn, bn, ui)", undefined);
// ack("ps.push(PKG, ALT, KEY, RPMPROB_DISKNODES, dn, bn, ui)", undefined);
// ack("ps.push(PKG, ALT, KEY, RPMPROB_RDONLY, dn, bn, ui)", undefined);
// ack("ps.push(PKG, ALT, KEY, RPMPROB_BADPRETRANS, dn, bn, ui)", undefined);
// ack("ps.push(PKG, ALT, KEY, RPMPROB_BADPLATFORM, A, O, ui)", undefined);
// ack("ps.push(PKG, ALT, KEY, RPMPROB_NOREPACKAGE, dn, bn, ui)", undefined);
// ack("ps.length", 12);

// ack("ps.print()", undefined);

delete ps;

if (loglvl) print("<-- Ps.js");
