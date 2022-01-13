if (loglvl) print("--> St.js");

var rpmst = require('rpmst');

var st = new rpmst.St('.');
ack("typeof st;", "object");
ack("st instanceof rpmst.St;", true);
ack("st.debug = 1;", 1);
ack("st.debug = 0;", 0);

st();

var dndot = '.';
var dnroot = '/';
var dnfurl = 'file:///';
var popturl = 'http://rpm5.org/files/popt/popt-1.14.tar.gz';

st(dndot);

// print("=============");
// print(JSON.stringify(rpmst.St));
// print(JSON.stringify(rpmst.St.prototype));
// print(JSON.stringify(rpmst.St.prototype));
// print(JSON.stringify(st));
// print(JSON.stringify(st.__proto__));
// for (let i in st)
//      print(i + ': ' + st[i] + ': ' +st.__proto__[i]);
// print("=============");

print("    dev: 0x" + st.dev.toString(16));
print("    ino: " + st.ino);
print("   mode: 0" + st.mode.toString(8));
print("  nlink: " + st.nlink);
print("    uid: " + st.uid);
print("    gid: " + st.gid);
print("   rdev: 0x" + st.rdev.toString(16));
print("   size: " + st.size);
print("blksize: " + st.blksize);
print(" blocks: " + st.blocks);
print("  atime: " + st.atime);
print("  mtime: " + st.mtime);
print("  ctime: " + st.ctime);

delete st;

var rst = new rpmst.St(dnroot);
var fst = new rpmst.St(dnfurl);

// print("=============");
// ack('JSON.stringify(rst.__proto__)', JSON.stringify(fst.__proto__));
// print("=============");
ack("rst.dev", fst.dev);
ack("rst.ino", fst.ino);
ack("rst.mode", fst.mode);
ack("rst.nlink", fst.nlink);
ack("rst.uid", fst.uid);
ack("rst.gid", fst.gid);
ack("rst.rdev", fst.rdev);
ack("rst.size", fst.size);
ack("rst.blksize", fst.blksize);
ack("rst.blocks", fst.blocks);

// grrr
// ack("rst.atime", fst.atime);
// ack("rst.mtime", fst.mtime);
// ack("rst.ctime", fst.ctime);

delete fst;
delete rst;

var st = new rpmst.St(popturl);

// print(JSON.stringify(st.__proto__));
ack("st.dev", 0x0);
ack("st.ino", 907648754);
ack("st.mode", 0100644);
ack("st.nlink", 1);
ack("st.uid", 0);
ack("st.gid", 0);
ack("st.rdev", 0x0);
ack("st.size", 695557);
ack("st.blksize", 0);
ack("st.blocks", 1359);

ack("st.atime", 'Sun Apr 06 2008 12:29:26 GMT-0400 (EST)');
ack("st.mtime", 'Sun Apr 06 2008 12:29:26 GMT-0400 (EST)');
ack("st.ctime", 'Sun Apr 06 2008 12:29:26 GMT-0400 (EST)');

delete st;

if (loglvl) print("<-- St.js");
