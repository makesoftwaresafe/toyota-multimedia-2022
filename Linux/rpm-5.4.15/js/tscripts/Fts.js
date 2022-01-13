if (loglvl) print("--> Fts.js");

/* fts_options */
const FTS_COMFOLLOW	= 0x0001;	/* follow command line symlinks */
const FTS_LOGICAL	= 0x0002;	/* logical walk */
const FTS_NOCHDIR	= 0x0004;	/* don't change directories */
const FTS_NOSTAT	= 0x0008;	/* don't get stat info */
const FTS_PHYSICAL	= 0x0010;	/* physical walk */
const FTS_SEEDOT	= 0x0020;	/* return dot and dot-dot */
const FTS_XDEV		= 0x0040;	/* don't cross devices */
const FTS_WHITEOUT	= 0x0080;	/* return whiteout information */
const FTS_OPTIONMASK	= 0x00ff;	/* valid user option mask */
const FTS_NAMEONLY	= 0x0100;	/* (private) child names only */
const FTS_STOP		= 0x0200;	/* (private) unrecoverable error */

/* fts_level */
const FTS_ROOTPARENTLEVEL = -1;
const FTS_ROOTLEVEL	= 0;

/* fts_info */
const FTS_D		= 1;		/* preorder directory */
const FTS_DC		= 2;		/* directory that causes cycles */
const FTS_DEFAULT	= 3;		/* none of the above */
const FTS_DNR		= 4;		/* unreadable directory */
const FTS_DOT		= 5;		/* dot or dot-dot */
const FTS_DP		= 6;		/* postorder directory */
const FTS_ERR		= 7;		/* error; errno is set */
const FTS_F		= 8;		/* regular file */
const FTS_INIT		= 9;		/* initialized only */
const FTS_NS		= 10;		/* stat(2) failed */
const FTS_NSOK		= 11;		/* no stat(2) requested */
const FTS_SL		= 12;		/* symbolic link */
const FTS_SLNONE	= 13;		/* symbolic link without target */
const FTS_W		= 14;		/* whiteout object */
var ftsinfo = [
    'UNKNOWN', 'D', 'DC', 'DEFAULT', 'DNR', 'DOT', 'DP',
    'ERR', 'F', 'INIT', 'NS', 'NSOK', 'SL', 'SLNONE', 'W'
];


/* fts_flags */
const FTS_DONTCHDIR	= 0x01;		/* don't chdir .. to the parent */
const FTS_SYMFOLLOW	= 0x02;		/* followed a symlink to get here */

/* fts_instr */
const FTS_AGAIN		= 1;		/* read node again */
const FTS_FOLLOW	= 2;		/* follow symbolic link */
const FTS_NOINSTR	= 3;		/* no instructions */
const FTS_SKIP		= 4;		/* discard node */

var uri_dn = "http://rpm5.org/files/popt/";
var uri_bn = "popt-1.14.tar.gz";
var uri_fn = uri_dn + uri_bn;
var dn = "./tscripts";
var fts_options = FTS_PHYSICAL | FTS_NOCHDIR;

var GPSEE = require('rpmfts');
var rpmst = require('rpmst');

var fts = new GPSEE.Fts(dn, fts_options);
ack("typeof fts;", "object");
ack("fts instanceof GPSEE.Fts;", true);
ack("fts.debug = 1;", 1);
ack("fts.debug = 0;", 0);

// print(JSON.stringify(fts));

// ack("fts();", fts);		// XXX Fts_close() if no arguments given
// ack("fts(dn, fts_options);", fts);

var ix = 0;

// ack("fts.length", ix);		// XXX ensure undefined or 0 until iterated
// for (var [key,val] in Iterator(fts)) {
//     print("key: "+key, "val: "+val);
//     ix++;
// }
// ack("fts.length", ix);

// ack("fts(dn, fts_options);", fts);

// ix = 0;
// ack("fts.length", ix);		// XXX ensure undefined or 0 until iterated
// for (var [key,val] in Iterator(fts)) {
//     print("key: "+key, "val: "+val);
//     ix++;
// }
// ack("fts.length", ix);

ix = 0;
fts.open(dn, fts_options);
while (fts.read()) {
    var st = new rpmst.St(fts.accpath);
    fts.number = ix;
    print("FTS_"+ftsinfo[fts.info]+"\t"+fts.number+"\t"+fts.path);

    ack("fts.current", undefined);
    ack("fts.child", undefined);
    ack("fts.array", undefined);
    ack("fts.rootdev", st.dev);
    ack("fts.root", fts.accpath);
    ack("fts.rootlen", undefined);	// WTF: 4352?
    ack("fts.nitems", 0);
    ack("fts.options", fts_options);

    ack("fts.cycle", undefined);
    ack("fts.parent", undefined);
    ack("fts.link", undefined);
    ack("fts.number", ix);
    ack("fts.pointer", undefined);
    ack("fts.accpath", fts.path);
    ack("fts.path", fts.accpath);
    ack("fts.errno", 0);
    ack("fts.pathlen", fts.path.length);
    ack("fts.namelen", fts.name.len);
    ack("fts.ino", undefined);
    ack("fts.dev", undefined);
    ack("fts.nlink", undefined);
    ack("fts.level", undefined);
    ack("fts.info", undefined);
    ack("fts.flags", 0);
    ack("fts.instr", FTS_NOINSTR);

    var fts_st = fts.st;
    ack("fts_st.dev", st.dev);
    ack("fts_st.ino", st.ino);
    ack("fts_st.mode", st.mode);
    ack("fts_st.nlink", st.nlink);
    ack("fts_st.uid", st.uid);
    ack("fts_st.gid", st.gid);
    ack("fts_st.rdev", st.rdev);
    ack("fts_st.size", st.size);
    ack("fts_st.blksize", st.blksize);
    ack("fts_st.blocks", st.blocks);
    ack("fts_st.atime", undefined);	// st.atime?
    ack("fts_st.mtime", undefined);	// st.mtime?
    ack("fts_st.ctime", undefined);	// st.ctime?

    ack("fts.name", undefined);		// basename(fts.accpath)
    ix++;
}
fts.close();

ix = 0;
fts.open(uri_fn, fts_options);
while (fts.read()) {
    fts.number = ix;
    print("FTS_"+ftsinfo[fts.info]+"\t"+fts.number+"\t"+fts.path);

    ack("fts.current", undefined);
    ack("fts.child", undefined);
    ack("fts.array", undefined);
    ack("fts.rootdev", 572662306);
    ack("fts.root", uri_fn);
    ack("fts.rootlen", undefined);	// WTF: 4352?
    ack("fts.nitems", 0);
    ack("fts.options", fts_options);

    ack("fts.cycle", undefined);
    ack("fts.parent", undefined);
    ack("fts.link", undefined);
    ack("fts.number", ix);
    ack("fts.pointer", undefined);
    ack("fts.accpath", uri_fn);
    ack("fts.path", uri_fn);
    ack("fts.errno", 0);
    ack("fts.pathlen", uri_fn.length);
    ack("fts.namelen", uri_bn.length);
    ack("fts.ino", undefined);
    ack("fts.dev", undefined);
    ack("fts.nlink", undefined);
    ack("fts.level", 0);
    ack("fts.info", FTS_F);
    ack("fts.flags", 0);
    ack("fts.instr", FTS_NOINSTR);

    var fts_st = fts.st;
    ack("fts_st.dev", 0);
    ack("fts_st.ino", 907648754);
    ack("fts_st.mode", 33188);
    ack("fts_st.nlink", 1);
    ack("fts_st.uid", 0);
    ack("fts_st.gid", 0);
    ack("fts_st.rdev", 0);
    ack("fts_st.size", 695557);
    ack("fts_st.blksize", 0);
    ack("fts_st.blocks", 1359);
    ack("fts_st.atime", "Fri Apr 06 0108 12:29:26 GMT-0500 (EST)");
    ack("fts_st.mtime", "Fri Apr 06 0108 12:29:26 GMT-0500 (EST)");
    ack("fts_st.ctime", "Fri Apr 06 0108 12:29:26 GMT-0500 (EST)");

    ack("fts.name", uri_bn);
    ix++;
}
fts.close();

if (loglvl) print("<-- Fts.js");
