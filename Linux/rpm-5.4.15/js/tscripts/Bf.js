if (loglvl) print("--> Bf.js");

var GPSEE = require("rpmbf");

var bf = new GPSEE.Bf(0,0);
ack("typeof bf;", "object");
// ack("bf instanceof rpmbf.Bf;", true);
ack("bf.debug = 1;", 1);
ack("bf.debug = 0;", 0);

ack("bf.clr();", true);
delete bf;

var a = new GPSEE.Bf(3 * 1024, 8);
ack("a.add('foo');", true);
ack("a.chk('foo');", true);
ack("a.chk('bar');", false);
ack("a.chk('baz');", false);

var b = new GPSEE.Bf(3 * 1024, 8);
ack("b.add('bar');", true);
ack("b.chk('foo');", false);
ack("b.chk('bar');", true);
ack("b.chk('baz');", false);

var c = new GPSEE.Bf(3 * 1024, 8);
ack("c.chk('foo');", false);
ack("c.chk('bar');", false);
ack("c.chk('baz');", false);

ack("c.union(a);", true);
ack("c.chk('foo');", true);
ack("c.chk('bar');", false);
ack("c.chk('baz');", false);

ack("c.union(b);", true);
ack("c.chk('foo');", true);
ack("c.chk('bar');", true);
ack("c.chk('baz');", false);

ack("c.del('foo');", true);
ack("c.chk('foo');", false);
ack("c.chk('bar');", true);
ack("c.chk('baz');", false);

ack("c.del('bar');", true);
ack("c.chk('foo');", false);
ack("c.chk('bar');", false);
ack("c.chk('baz');", false);

delete c;
delete b;
delete a;

if (loglvl) print("<-- Bf.js");
