if (loglvl) print("--> Sys.js");

const R_OK	= 4;		/* Test for read permission.  */
const W_OK	= 2;		/* Test for write permission.  */
const X_OK	= 1;		/* Test for execute permission.  */
const F_OK	= 0;		/* Test for existence.  */

var rpmsys = require('rpmsys');
var rpmst = require('rpmst');

var sys = new rpmsys.Sys();
ack("typeof sys;", "object");
ack("sys instanceof rpmsys.Sys;", true);
ack("sys.debug = 1;", 1);
ack("sys.debug = 0;", 0);

var tmpdir = "/tmp";
var dn = tmpdir + "/sys.d";
var fn = tmpdir + "/sys.file";
var lfn = tmpdir + "/sys.link";
var rfn = tmpdir + "/sys.rename";
var sfn = tmpdir + "/sys.symlink";

ack("sys.ctermid", '/dev/tty');
print("     ctime: "+sys.ctime);
print("       cwd: "+sys.cwd);
ack("sys.domainname", null);
ack("sys.egid", sys.gid);
ack("sys.euid", sys.uid);
ack("sys.gid", sys.egid);
ack("sys.gid", sys.groups[0]);
nack("sys.hostid", 0x00000000);
print("  hostname: "+sys.hostname);
ack("sys.pid", sys.pgid);
nack("sys.pid", sys.ppid);
ack("sys.sid", sys.tid);
ack("sys.time", sys.timeofday[0]);	// timeval class?
ack("sys.uid", sys.euid);
ack("sys.umask", 0022);
// print("     uname: "+sys.uname);	// BUGGY

nack("sys.access(dn, F_OK);", 0);
ack("sys.mkdir(dn);", 0);
ack("sys.access(dn, F_OK);", 0);
ack("sys.rmdir(dn);", 0);

var st;

ack("sys.creat(fn,0644);", 0);
st = sys.stat(fn);
ack("st instanceof rpmst.St;", true);
ack("st.uid", sys.uid);
ack("st.gid", sys.gid);
ack("st.mode", 0100644);
ack("sys.chmod(fn,0640);", 0);

st = sys.lstat(fn);
ack("st instanceof rpmst.St;", true);
print("    ino: " + st.ino);
// print("   mode: 0" + st.mode.toString(8));
// print("  nlink: " + st.nlink);
// print("    uid: " + st.uid);
// print("    gid: " + st.gid);
// print("   rdev: 0x" + st.rdev.toString(16));
// print("   size: " + st.size);
// print("blksize: " + st.blksize);
// print(" blocks: " + st.blocks);
// print("  atime: " + st.atime);
// print("  mtime: " + st.mtime);
// print("  ctime: " + st.ctime);

ack("st.mode", 0100640);
ack("sys.chown(fn,-1,-1);", 0);
ack("sys.chown(fn,sys.uid,-1);", 0);
ack("sys.chown(fn,-1,sys.gid);", 0);
ack("sys.chown(fn,sys.uid,sys.gid);", 0);
ack("(st = sys.lstat(fn)) instanceof rpmst.St;", true);
ack("st.uid", sys.uid);
ack("st.gid", sys.gid);

ack("sys.link(fn,lfn);", 0);
ack("sys.symlink(lfn,sfn);", 0);
ack("(st = sys.stat(sfn)) instanceof rpmst.St;", true);
ack("st.mode", 0100640);
ack("(st = sys.lstat(sfn)) instanceof rpmst.St;", true);
ack("st.mode", 0120777);
ack("sys.readlink(sfn);", lfn);
ack("sys.rename(lfn,rfn);", 0);

ack("sys.unlink(fn);", 0);
nack("sys.unlink(lfn);", 0);
ack("sys.unlink(rfn);", 0);
ack("sys.unlink(sfn);", 0);

delete sys;

if (loglvl) print("<-- Sys.js");
