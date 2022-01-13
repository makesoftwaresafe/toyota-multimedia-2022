if (loglvl) print("--> Io.js");

var GPSEE = require('rpmio');
var rpmst = require('rpmst');

var io = new GPSEE.Io();
ack("typeof io;", "object");
ack("io instanceof GPSEE.Io;", true);
ack("io.debug = 1;", 1);
ack("io.debug = 0;", 0);

ack("io() instanceof GPSEE.Io;", true);

var tmpdir = '/tmp';
var fn = tmpdir + '/io.file';
var yadda = 'yadda yadda';

ack("io(fn, 'w') instanceof GPSEE.Io;", true);
ack("io.path", fn);
ack("io.flags", 0x241);
ack("io.mode", 0666);
ack("io.cpioPos", 0);
ack("io.bytesRemain", -1);
ack("io.lastModified", 0);
ack("io.contentLength", -1);
ack("io.contentType", null);
ack("io.contentDisposition", null);
ack("io.digestinit() instanceof GPSEE.Io;", true);
ack("io.ndigests", 1);
ack("io.fwrite(yadda);", true);
ack("io.fflush();", true);
ack("io.fstat() instanceof rpmst.St;", true);
ack("io.fchown(-1,-1);", 0);
ack("io.ferror();", true);
ack("io.fileno();", 3);
ack("io.digestfini();", 'e1a8e5c3fdc6995bbfdc721dcf16c52b');
ack("io.cpioPos", 0);
ack("io.bytesRemain", -1);
ack("io.lastModified", 0);
ack("io.contentLength", -1);
ack("io.contentType", null);
ack("io.contentDisposition", null);
ack("io() instanceof GPSEE.Io;", true);

ack("io(fn, 'r') instanceof GPSEE.Io;", true);
ack("io.path", fn);
ack("io.flags", 0x0);
ack("io.mode", 0666);
ack("io.cpioPos", 0);
ack("io.bytesRemain", -1);
ack("io.lastModified", 0);
ack("io.contentLength", -1);
ack("io.contentType", null);
ack("io.contentDisposition", null);
ack("io.digestinit() instanceof GPSEE.Io;", true);
ack("io.ndigests", 1);
ack("io.fread();", yadda);
ack("io.fstat() instanceof rpmst.St;", true);
ack("io.fchown(-1,-1);", 0);
ack("io.ferror();", true);
ack("io.fileno();", 3);
ack("io.digestfini();", 'e1a8e5c3fdc6995bbfdc721dcf16c52b');
ack("io.cpioPos", 0);
ack("io.bytesRemain", -1);
ack("io.lastModified", 0);
ack("io.contentLength", -1);
ack("io.contentType", null);
ack("io.contentDisposition", null);
ack("io() instanceof GPSEE.Io;", true);

ack("io(fn, 'r.fpio') instanceof GPSEE.Io;", true);
ack("io.path", fn);
ack("io.flags", 0x0);
ack("io.mode", 0666);
ack("io.cpioPos", 0);
ack("io.bytesRemain", -1);
ack("io.lastModified", 0);
ack("io.contentLength", -1);
ack("io.contentType", null);
ack("io.contentDisposition", null);

ack("io.fseek(5);", 0);
ack("io.ftell();", 5);
ack("io.rewind();", true);
ack("io.ftell();", 0);

ack("io.fread();", yadda);
ack("io.fstat() instanceof rpmst.St;", true);
ack("io.fchown(-1,-1);", 0);
ack("io.ferror();", true);
ack("io.fileno();", 3);

ack("io.cpioPos", 0);
ack("io.bytesRemain", -1);
ack("io.lastModified", 0);
ack("io.contentLength", -1);
ack("io.contentType", null);
ack("io.contentDisposition", null);
ack("io() instanceof GPSEE.Io;", true);

delete io;

if (loglvl) print("<-- Io.js");
